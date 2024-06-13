#include <stdio.h>
#include "rv32.h"
#include "bus.h"

const char *opnames[] = {
	"illegal",
	"",
	"",
	"auipc",
	"lui",
	"store",
	"load",
	"jal",
	"jalr",
	"branch",
	"fence",
	"system" };

const char *alunames[] = {
	"", // none
	"add",
	"sub",
	"sll",
	"slt",
	"sltu",
	"xor",
	"srl",
	"sra",
	"or",
	"and" };

const char *regnames[] = {
	"zero",
	"ra", "sp",
	"gp", "tp",
	"t0", "t1", "t2",
	"s0", "s1",
	"a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",
	"s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11",
	"t3", "t4", "t5", "t6" };

// This is an exact copy of the ISR ROM contents of the real hardware
uint32_t ISR_ROM[] = {
	0xfd079073,0x00000797,0x34179073,0x08000793,0x3047b7f3,0x3007a7f3,0x800007b7,0x00778793,
	0x34279073,0x00800793,0x3007b7f3,0xfd0027f3,0xfd079073,0x08000793,0x3007b7f3,0x3047a7f3,
	0x00800793,0x3007a7f3,0xfd0027f3,0xfd079073,0x00000797,0x34179073,0x000017b7,0x80078793,
	0x3047b7f3,0x0047d793,0x3007a7f3,0x800007b7,0x00b78793,0x34279073,0x00800793,0x3007b7f3,
	0xfd0027f3,0xfd079073,0x08000793,0x3007b7f3,0x00479793,0x3047a7f3,0x00800793,0x3007a7f3,
	0xfd0027f3,0xfd079073,0x00000797,0x34179073,0x00800793,0x3047b7f3,0x00479793,0x3007a7f3,
	0x00b00793,0x34279073,0x00800793,0x3007b7f3,0xfd0027f3,0xfd079073,0x08000793,0x3007b7f3,
	0x0047d793,0x3047a7f3,0x00800793,0x3007a7f3,0xfd0027f3,0xfd079073,0x00000797,0x34179073,
	0x00800793,0x3047b7f3,0x00479793,0x3007a7f3,0x00300793,0x34279073,0x00800793,0x3007b7f3,
	0xfd0027f3,0xfd079073,0x08000793,0x3007b7f3,0x0047d793,0x3047a7f3,0x00800793,0x3007a7f3,
	0xfd0027f3,0xfd079073,0x00000797,0x34179073,0x00800793,0x3047b7f3,0x00479793,0x3007a7f3,
	0x00200793,0x34279073,0x00800793,0x3007b7f3,0xfd0027f3,0xfd079073,0x08000793,0x3007b7f3,
	0x0047d793,0x3047a7f3,0x00800793,0x3007a7f3,0xfd0027f3 };

CRV32::CRV32()
{
}

CRV32::~CRV32()
{
}

void CRV32::Reset()
{
	m_state_next = m_state = ECPUReset;
}

uint32_t CRV32::ALU()
{
	uint32_t aluout = 0;

	uint32_t selected = m_decoded_next.m_selimm ? m_decoded_next.m_immed : m_rval2_next;

	switch(m_decoded_next.m_aluop)
	{
		case ALU_OR:
			aluout = m_rval1_next | selected;
		break;
		case ALU_SUB:
			aluout = m_rval1_next + (~selected + 1); // val1-val2
		break;
		case ALU_SLL:
			aluout = m_rval1_next << (selected&0x1F);
		break;
		case ALU_SLT:
			aluout = ((int32_t)m_rval1_next < (int32_t)selected) ? 1 : 0;
		break;
		case ALU_SLTU:
			aluout = (m_rval1_next < selected) ? 1 : 0;
		break;
		case ALU_XOR:
			aluout = m_rval1_next ^ selected;
		break;
		case ALU_SRL:
			aluout = m_rval1_next >> (selected&0x1F);
		break;
		case ALU_SRA:
			aluout = (int32_t)m_rval1_next >> (selected&0x1F);
		break;
		case ALU_ADD:
			aluout = m_rval1_next + selected;
		break;
		case ALU_AND:
			aluout = m_rval1_next & selected;
		break;
		case ALU_MUL:
		{
			uint64_t a, b;
			int64_t sign1ext = (int32_t)m_rval1_next;
			int64_t sign2ext = (int32_t)m_rval2_next;
			switch (m_decoded_next.m_f3)
			{
				case 0b000:
				case 0b001: { a = sign1ext; b = sign2ext; } break; // mul/mulh
				case 0b010: { a = sign1ext; b = m_rval2_next; } break; // mulhsu
				case 0b011: { a = m_rval1_next; b = m_rval2_next; } break; // mulhu
				default: { a = 0; b = 0; } break;
			}
			uint64_t result = a * b;
			aluout = m_decoded_next.m_f3 == 0b000 ? (uint32_t)((a*b)&0xFFFFFFFF) : (uint32_t)(((a*b)&0xFFFFFFFF00000000)>>32);
		}
		break;
		case ALU_DIV:
		case ALU_REM:
		{
			uint64_t a, b;
			int64_t sign1ext = (int32_t)m_rval1_next;
			int64_t sign2ext = (int32_t)m_rval2_next;
			switch (m_decoded_next.m_f3)
			{
				case 0b100: { a = sign1ext; b = sign2ext; aluout = (uint32_t)(a / b); } break; //  div
				case 0b101: { a = m_rval1_next; b = m_rval2_next; aluout = (uint32_t)(a / b); } break; // divu
				case 0b110: { a = sign1ext; b = sign2ext; aluout = (uint32_t)(a % b); } break; // rem
				case 0b111: { a = m_rval1_next; b = m_rval2_next; aluout = (uint32_t)(a % b); } break; // remu
				default: { a = 0; b = 1; } break;
			}
		}
		break;
	}

	return aluout;
}

uint32_t CRV32::BLU()
{
	uint32_t bluout = 0;

	switch (m_decoded_next.m_bluop)
	{
		case BLU_EQ:
			bluout = m_rval1_next == m_rval2_next ? 1 : 0;
		break;
		case BLU_NE:
			bluout = m_rval1_next != m_rval2_next ? 1 : 0;
		break;
		case BLU_L:
			bluout = (int32_t)m_rval1_next < (int32_t)m_rval2_next ? 1 : 0;
		break;
		case BLU_GE:
			bluout = (int32_t)m_rval1_next >= (int32_t)m_rval2_next ? 1 : 0;
		break;
		case BLU_LU:
			bluout = m_rval1_next < m_rval2_next ? 1 : 0;
		break;
		case BLU_GEU:
			bluout = m_rval1_next >= m_rval2_next ? 1 : 0;
		break;
	}

	return bluout;
}

void CRV32::DecodeInstruction(uint32_t instr, SDecodedInstruction& dec)
{
	dec.m_opcode = SelectBitRange(instr, 6, 0);
	dec.m_f3 = SelectBitRange(instr, 14, 12);
	dec.m_rs1 = SelectBitRange(instr, 19, 15);
	dec.m_rs2 = SelectBitRange(instr, 24, 20);
	dec.m_rd = SelectBitRange(instr, 11, 7);
	dec.m_f12 = SelectBitRange(instr, 31, 20);
	dec.m_csroffset = (SelectBitRange(instr, 31, 25) << 5) | dec.m_rs2;

#if defined(DEBUG)
	switch (dec.m_opcode)
	{
		case OP_OP:		dec.m_opindex = 1; break;
		case OP_OP_IMM:	dec.m_opindex = 2; break;
		case OP_AUIPC:	dec.m_opindex = 3; break;
		case OP_LUI:	dec.m_opindex = 4; break;
		case OP_STORE:	dec.m_opindex = 5; break;
		case OP_LOAD:	dec.m_opindex = 6; break;
		case OP_JAL:	dec.m_opindex = 7; break;
		case OP_JALR:	dec.m_opindex = 8; break;
		case OP_BRANCH:	dec.m_opindex = 9; break;
		case OP_FENCE:	dec.m_opindex = 10; break;
		case OP_SYSTEM:	dec.m_opindex = 11; break;
		default:		dec.m_opindex = 0; break;
	}
#endif

	switch (dec.m_opcode)
	{
		case OP_LUI:
		case OP_AUIPC:
		{
			// U-imm
			dec.m_immed = SelectBitRange(instr, 31, 12) << 12;
		}
		break;

		case OP_STORE:
		{
			// S-imm
			int32_t sign = int32_t(instr & 0x80000000) >> 20;	// 32-11 == 21
			uint32_t upper = SelectBitRange(instr, 30, 25);		// 6
			uint32_t lower = SelectBitRange(instr, 11, 7);		// +5 == 11
			dec.m_immed = sign | (upper<<5) | lower;
		}
		break;

		case OP_JAL:
		{
			// J-imm
			int32_t sign = int32_t(instr & 0x80000000) >> 11;	// 32-20 == 12
			uint32_t upper = SelectBitRange(instr, 19, 12);		// 8
			uint32_t middle = SelectBitRange(instr, 20, 20);	// +1
			uint32_t lower = SelectBitRange(instr, 30, 21);		// +10
			uint32_t zero = 0;									// +1 == 20
			dec.m_immed = sign | (upper<<12) | (middle<<11) | (lower<<1);
		}
		break;

		case OP_BRANCH:
		{
			// B-imm
			int32_t sign = int32_t(instr & 0x80000000) >> 19;	// 32-12 == 20
			uint32_t upper = SelectBitRange(instr, 7, 7);		// 1
			uint32_t middle = SelectBitRange(instr, 30, 25);	// +6
			uint32_t lower = SelectBitRange(instr, 11, 8);		// +4
			uint32_t zero = 0x0;								// +1 == 12
			dec.m_immed = sign | (upper<<11) | (middle<<5) | (lower<<1);
		}
		break;

		case OP_SYSTEM:
		{
			uint32_t lower = SelectBitRange(instr, 19, 15);	// 5
			dec.m_immed = lower;
		}
		break;

		case OP_OP_IMM:
		case OP_LOAD:
		case OP_JALR:
		{
			// I-imm
			int32_t sign = int32_t(instr & 0x80000000) >> 20;	// 32-11 == 21
			uint32_t lower = SelectBitRange(instr, 30, 20);		// 11
			dec.m_immed = sign | lower;
		}
		break;

		default: // OP_FENCE
		{
			dec.m_immed = 0;
		}
		break;
	}

	dec.m_aluop = ALU_NONE;
	dec.m_bluop = BLU_NONE;

	uint32_t mathopsel = SelectBitRange(instr, 30, 30);
	uint32_t muldiv = SelectBitRange(instr, 25, 25);
	if (dec.m_opcode == OP_OP)
	{
		switch (dec.m_f3)
		{
			case 0b000:
				dec.m_aluop = muldiv ? ALU_MUL : (mathopsel ? ALU_SUB : ALU_ADD);
			break;
			case 0b001:
				dec.m_aluop = muldiv ? ALU_MUL : ALU_SLL;
			break;
			case 0b011:
				dec.m_aluop = muldiv ? ALU_MUL : ALU_SLTU;
			break;
			case 0b010:
				dec.m_aluop = muldiv ? ALU_MUL : ALU_SLT;
			break;
			case 0b110:
				dec.m_aluop = muldiv ? ALU_REM : ALU_OR;
			break;
			case 0b111:
				dec.m_aluop = muldiv ? ALU_REM : ALU_AND;
			break;
			case 0b101:
				dec.m_aluop = muldiv ? ALU_DIV : (mathopsel ? ALU_SRA : ALU_SRL);
			break;
			case 0b100:
				dec.m_aluop = muldiv ? ALU_DIV : ALU_XOR;
			break;
			default:
				dec.m_aluop = ALU_NONE;
			break;
		}
	}

	if (dec.m_opcode == OP_OP_IMM)
	{
		switch (dec.m_f3)
		{
			case 0b000:	dec.m_aluop = ALU_ADD; break;
			case 0b001:	dec.m_aluop = ALU_SLL; break;
			case 0b011:	dec.m_aluop = ALU_SLTU; break;
			case 0b010:	dec.m_aluop = ALU_SLT; break;
			case 0b110:	dec.m_aluop = ALU_OR; break;
			case 0b111:	dec.m_aluop = ALU_AND; break;
			case 0b101:	dec.m_aluop = mathopsel ? ALU_SRA : ALU_SRL; break;
			case 0b100:	dec.m_aluop = ALU_XOR; break;
			default:	dec.m_aluop = ALU_NONE; break;
		}
	}

	if (dec.m_opcode == OP_BRANCH)
	{
		switch (dec.m_f3)
		{
			case 0b000:	dec.m_bluop = BLU_EQ; break;
			case 0b001:	dec.m_bluop = BLU_NE; break;
			case 0b011:	dec.m_bluop = BLU_NONE; break;
			case 0b010:	dec.m_bluop = BLU_NONE; break;
			case 0b110:	dec.m_bluop = BLU_LU; break;
			case 0b111:	dec.m_bluop = BLU_GEU; break;
			case 0b101:	dec.m_bluop = BLU_GE; break;
			case 0b100:	dec.m_bluop = BLU_L; break;
			default:	dec.m_bluop = BLU_NONE; break;
		}
	}

	dec.m_selimm = (dec.m_opcode==OP_JALR) || (dec.m_opcode==OP_OP_IMM) || (dec.m_opcode==OP_LOAD) || (dec.m_opcode==OP_STORE);

#if defined(DEBUG)
	//printf("%.8X: %s%s %s %s -> %s I=%d\n", m_PC, opnames[dec.m_opindex], alunames[dec.m_aluop], regnames[dec.m_rs1], regnames[dec.m_rs2], regnames[dec.m_rd], dec.m_immed);
#endif
}

void CRV32::SetExceptionHandlerStartEnd()
{
	switch (m_exceptionmode)
	{
		// SWI start
		case 0x00000001: {
			m_exceptionstart = 81;
			m_exceptionend = 92;
		}
		break;

		// SWI end
		case 0x80000001: {
			m_exceptionstart = 93;
			m_exceptionend = 100;
		}
		break;

		// HWI start
		case 0x00000002: {
			m_exceptionstart = 19;
			m_exceptionend = 32;
		}
		break;

		// HWI end
		case 0x80000002: {
			m_exceptionstart = 33;
			m_exceptionend = 40;
		}
		break;

		// TMI start
		case 0x00000003: {
			m_exceptionstart = 0;
			m_exceptionend = 11;
		}
		break;

		// TMI end
		case 0x80000003: {
			m_exceptionstart = 12;
			m_exceptionend = 18;
		}
		break;

		// ebreak start
		case 0x00000004: {
			m_exceptionstart = 61;
			m_exceptionend = 72;
		}
		break;

		// ebreak end
		case 0x80000004: {
			m_exceptionstart = 73;
			m_exceptionend = 80;
		}
		break;

		// ecall start
		case 0x00000005: {
			m_exceptionstart = 41;
			m_exceptionend = 52;
		}
		break;

		// ecall end
		case 0x80000005: {
			m_exceptionstart = 53;
			m_exceptionend = 60;
		}
		break;

		// Unknown
		default:
		{
			m_exceptionstart = 0;
			m_exceptionend = 0;
		}
		break;
	}
}

bool CRV32::Tick(CBus& bus, uint32_t irq)
{
	bool retval = true;
	bool softfault = false;
	bool ebreak = false;
	bool ecall = false;

	//if (cpuclock.m_edge == RisingEdge)
	{
		// We hack our way around with these CSR register
		// Normally they're shadowed to hardware counters
		// in the device so we don't have to emulate any latency here

		uint32_t csrbase = m_idx == 0 ? CSR0BASE : CSR1BASE;
		// Hardware will return hardwired or timer based values for these instead of overwriting the CSR
		{
			bus.Write(csrbase + (CSR_CYCLELO << 2), (uint32_t)(m_cyclecounter & 0x00000000FFFFFFFFU), 0xFFFFFFFF);
			bus.Write(csrbase + (CSR_CYCLEHI << 2), (uint32_t)((m_cyclecounter & 0xFFFFFFFF00000000U) >> 32), 0xFFFFFFFF);
			bus.Write(csrbase + (CSR_RETILO << 2), (uint32_t)(m_retired & 0x00000000FFFFFFFFU), 0xFFFFFFFF);
			bus.Write(csrbase + (CSR_RETIHI << 2), (uint32_t)((m_retired & 0xFFFFFFFF00000000U) >> 32), 0xFFFFFFFF);
			bus.Write(csrbase + (CSR_TIMELO << 2), (uint32_t)(m_wallclock & 0x00000000FFFFFFFFU), 0xFFFFFFFF);
			bus.Write(csrbase + (CSR_TIMEHI << 2), (uint32_t)((m_wallclock & 0xFFFFFFFF00000000U) >> 32), 0xFFFFFFFF);
			bus.Write(csrbase + (CSR_PROGRAMCOUNTER << 2), m_PC, 0xFFFFFFFF);
		}

		if (m_cyclecounter%15 == 0) // 150MHz vs 10Mhz
			++m_wallclock;
		++m_cyclecounter;

		// Process input and prepare intermediates
		switch (m_state)
		{
			case ECPUReset:
			{
				m_PC_next = m_resetvector;
				for (int i=0;i<32;++i)
					m_GPR_next[i] = 0;
				m_state_next = ECPUFetchDecode;
			}
			break;

			case ECPUFetchDecode:
			{
				if (m_exceptionmode)
				{
					// Inject ISR ROM instruction sequence when we're in ISR mode
					m_instruction_next = ISR_ROM[m_exceptionstart];
					// Done injecting instructions
					if (m_exceptionstart == m_exceptionend)
					{
						// Remember for exit time
						m_lasttrap = m_exceptionmode;
						// Are we returning from an exception?
						if (m_exceptionmode & 0x80000000)
							m_postmret = 1;
						else // Or we're ending header to branch to mtvec
							m_posteoi = 1;
						m_exceptionmode = 0;
					}
					++m_exceptionstart;
				}
				else
				{
					// Otherwise read from instruction memory as usual
					bus.Read(m_PC, m_instruction_next);
				}

				DecodeInstruction(m_instruction_next, m_decoded_next);
				m_rval1_next = m_GPR[m_decoded_next.m_rs1];
				m_rval2_next = m_GPR[m_decoded_next.m_rs2];
				m_aluout_next = ALU();
				m_branchout_next = BLU();

				m_state_next = ECPUExecute;
			}
			break;

			case ECPUExecute:
			{
				uint32_t adjacentpc = m_PC + 4;
				uint32_t rwaddress = m_rval1 + m_decoded.m_immed;
				uint32_t offsetpc = m_PC + m_decoded.m_immed;
				uint32_t jumpabs = m_rval1 + m_decoded.m_immed;
				uint32_t rdin = 0;
				uint32_t rwen = 0;
				uint32_t wdata = 0;
				uint32_t wstrobe = 0;

				if (m_decoded.m_opcode != OP_BRANCH && m_decoded.m_opcode != OP_JALR && m_decoded.m_opcode != OP_JAL)
					m_PC_next = adjacentpc;

				switch(m_decoded.m_opcode)
				{
					case OP_OP:
					case OP_OP_IMM:
						rdin = m_aluout;
						rwen = 1;
					break;

					case OP_AUIPC:
						rdin = offsetpc;
						rwen = 1;
					break;

					case OP_LUI:
						rdin = m_decoded.m_immed;
						rwen = 1;
					break;

					case OP_JAL:
						m_PC_next = offsetpc;
						rdin = adjacentpc;
						rwen = 1;
					break;

					case OP_JALR:
						m_PC_next = jumpabs;
						rdin = adjacentpc;
						rwen = 1;
					break;

					case OP_BRANCH:
						m_PC_next = m_branchout ? offsetpc : adjacentpc;
					break;

					case OP_FENCE:
						// NOOP for now
					break;

					case OP_SYSTEM:
						if (m_decoded.m_f12 == F12_CDISCARD)
						{
							// cacheop=0b01
							// NOOP for now, D$ not implemented yet
						}
						else if (m_decoded.m_f12 == F12_CFLUSH)
						{
							// cacheop=0b11
							// NOOP for now, D$ not implemented yet
						}
						else if (m_decoded.m_f12 == F12_MRET)
						{
							m_exceptionmode = 0x80000000 | m_lasttrap;
							SetExceptionHandlerStartEnd();
						}
						else if (m_decoded.m_f12 == F12_WFI)
						{
							// NOOP for now, ideally should wait for irq != 0 in a WFI state for about 16 clocks similar to real hardware
						}
						else if (m_decoded.m_f12 == F12_EBREAK)
						{
							ebreak = true;
						}
						else if (m_decoded.m_f12 == F12_ECALL)
						{
							ecall = true;
						}
						else // CSROP
						{
							// Read previous value
							uint32_t csraddress = csrbase + (m_decoded.m_csroffset << 2);
							uint32_t csrprevval;
							bus.Read(csraddress, csrprevval);

							// Keep it in a register
							rwen = 1;
							rdin = csrprevval;
							rwaddress = csraddress;

							// Apply operation
							wstrobe = 0b1111;
							switch (m_decoded.m_f3)
							{
								case 0b001: // csrrw
									wdata = m_rval1;
								break;
								case 0b101: // csrrwi
									wdata = m_decoded.m_immed;
								break;
								case 0b010: // csrrs / csrr
									wdata = csrprevval | m_rval1;
								break;
								case 0b110: // csrrsi
									wdata = csrprevval | m_decoded.m_immed;
								break;
								case 0b011: // csrrc
									wdata = csrprevval & (~m_rval1);
								break;
								case 0b111: // csrrci
									wdata = csrprevval & (~m_decoded.m_immed);
								break;
								default: // unknown - keep previous value
									wdata = csrprevval;
								break;
							}
						}
					break;

					case OP_STORE:
					{
						uint32_t byte = SelectBitRange(m_rval2, 7, 0);
						uint32_t half = SelectBitRange(m_rval2, 15, 0);
						switch (m_decoded.m_f3)
						{
							case 0b000:	wdata = (byte<<24)|(byte<<16)|(byte<<8)|byte; break;
							case 0b001:	wdata = (half<<16)|half; break;
							default:	wdata = m_rval2; break;
						}
						uint32_t ah = SelectBitRange(rwaddress, 1, 1);
						uint32_t ab = SelectBitRange(rwaddress, 0, 0);
						uint32_t himask = (ah << 3) | (ah << 2) | ((1 - ah) << 1) | (1 - ah);
						uint32_t lomask = ((ab << 3) | ((1 - ab) << 2) | (ab << 1) | (1 - ab));
						switch (m_decoded.m_f3)
						{
							case 0b000:	wstrobe = himask & lomask; break;
							case 0b001:	wstrobe = himask; break;
							default:	wstrobe = 0b1111; break;
						}
					}
					break;

					case OP_LOAD:
					{
						uint32_t dataword;
						bus.Read(rwaddress, dataword);

						uint32_t range1 = SelectBitRange(rwaddress,1,1);
						uint32_t range2 = SelectBitRange(rwaddress,1,0);

						uint32_t b3 = SelectBitRange(dataword,31,24);
						uint32_t b2 = SelectBitRange(dataword,23,16);
						uint32_t b1 = SelectBitRange(dataword,15,8);
						uint32_t b0 = SelectBitRange(dataword,7,0);

						uint32_t h1 = SelectBitRange(dataword,31,16);
						uint32_t h0 = SelectBitRange(dataword,15,0);

						int32_t sign3 = int32_t(dataword & 0x80000000);
						int32_t sign2 = int32_t((dataword<<8) & 0x80000000);
						int32_t sign1 = int32_t((dataword<<16) & 0x80000000);
						int32_t sign0 = int32_t((dataword<<24) & 0x80000000);

						switch(m_decoded.m_f3)
						{
							case 0b000: // BYTE with sign extension
							{
								switch(range2)
								{
									case 0b11: rdin = (sign3>>24) | b3; break;
									case 0b10: rdin = (sign2>>24) | b2; break;
									case 0b01: rdin = (sign1>>24) | b1; break;
									case 0b00: rdin = (sign0>>24) | b0; break;
								}
							}
							break;
							case 0b001: // HALF with sign extension
							{
								switch(range1)
								{
									case 0b1: rdin = (sign3>>16) | h1; break;
									case 0b0: rdin = (sign1>>16) | h0; break;
								}
							}
							break;
							case 0b100: // BYTE with zero extension
							{
								switch(range2)
								{
									case 0b11: rdin = b3; break;
									case 0b10: rdin = b2; break;
									case 0b01: rdin = b1; break;
									case 0b00: rdin = b0; break;
								}
							}
							break;
							case 0b101: // HALF with zero extension
							{
								switch(range1)
								{
									case 0b1: rdin = h1; break;
									case 0b0: rdin = h0; break;
								}
							}
							break;
							default: // WORD - 0b010
							{
								rdin = dataword;
							}
							break;
						}
						rwen = 1;
					}
					break;

					default:
						if (m_sie)
							softfault = true;
/*#if defined(DEBUG)
						printf("ILLEGAL_INSTRUCTION %.8X @PC 0x%.8X\n", m_instruction, m_PC);
						for (int i=0; i<32; ++i)
							printf("%s=%.8X ", regnames[i], m_GPR[i]);
						retval = false;
#endif*/
					break;
				}

				if ((ebreak || ecall || softfault || irq) && m_exceptionmode == 0)
				{
					if (softfault) // software
						m_exceptionmode = 0x00000001;
					else if (ebreak) // ebreak
						m_exceptionmode = 0x00000004;
					else if (ecall) // ecall
						m_exceptionmode = 0x00000005;
					else if (irq & 1) // hardware
						m_exceptionmode = 0x00000002;
					else if (irq & 2) // timer
						m_exceptionmode = 0x00000003;
					SetExceptionHandlerStartEnd();
				}

				if(wstrobe)
					bus.Write(rwaddress, wdata, wstrobe);

				if(rwen && m_decoded.m_rd != 0)
					m_GPR_next[m_decoded.m_rd] = rdin;

				if (m_postmret) // Resume from where we left off
				{
					m_postmret = 0;
					bus.Read(csrbase + (CSR_MEPC << 2), m_PC_next);
				}
				else if (m_posteoi) // Branch to ISR vector
				{
					m_posteoi = 0;
					bus.Read(csrbase + (CSR_MTVEC << 2), m_PC_next);
				}

				++m_retired;
				m_state_next = ECPUFetchDecode;
			}
			break;

			default:
#if defined(DEBUG)
				printf("  ! ILLEGAL CPU STATE !\n");
#endif
			break;
		}

		if (m_pendingCPUReset)
		{
			m_state_next = ECPUReset;
			m_pendingCPUReset = false;
		}
	}
	//else
	{
		// Propagate state
		m_state = m_state_next;

		// Propagate intermediates to registers
		m_PC = m_exceptionmode ? m_PC : m_PC_next;
		m_instruction = m_instruction_next;
		m_decoded = m_decoded_next;
		m_rval1 = m_rval1_next;
		m_rval2 = m_rval2_next;
		m_aluout = m_aluout_next;
		m_branchout = m_branchout_next;

		// Propagate GPR
		for (int i=0; i<32; ++i)
			m_GPR[i] = m_GPR_next[i];
	}

	return retval;
}
