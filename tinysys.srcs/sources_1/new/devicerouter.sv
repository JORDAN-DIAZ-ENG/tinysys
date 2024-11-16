`timescale 1ns / 1ps

// 1x5 Device Address Router

module devicerouter(
	input wire aclk,
	input wire aresetn,
    axi4if.slave axi_s,
    wire [3:0] addressmask[11:0], // Bits [19:16] of the physical address
    axi4if.master axi_m[11:0]);

// ------------------------------------------------------------------------------------
// Write router
// ------------------------------------------------------------------------------------

logic [11:0] validwaddr = 12'd0;

always_comb begin
	unique case(axi_s.awaddr[19:16])
		addressmask[0]:  validwaddr = 12'b000000000001;
		addressmask[1]:  validwaddr = 12'b000000000010;
		addressmask[2]:  validwaddr = 12'b000000000100;
		addressmask[3]:  validwaddr = 12'b000000001000;
		addressmask[4]:  validwaddr = 12'b000000010000;
		addressmask[5]:  validwaddr = 12'b000000100000;
		addressmask[6]:  validwaddr = 12'b000001000000;
		addressmask[7]:  validwaddr = 12'b000010000000;
		addressmask[8]:  validwaddr = 12'b000100000000;
		addressmask[9]:  validwaddr = 12'b001000000000;
		addressmask[10]: validwaddr = 12'b010000000000;
		addressmask[11]: validwaddr = 12'b100000000000;
		default:		 validwaddr = 12'b000000000000;
	endcase
end

genvar rgen;
generate
for (rgen=0; rgen<12; rgen++) begin
	always_comb begin
		axi_m[rgen].awaddr  = validwaddr[rgen] ? axi_s.awaddr	: 32'd0;
		axi_m[rgen].awvalid = validwaddr[rgen] ? axi_s.awvalid	: 1'b0;
		axi_m[rgen].awlen   = validwaddr[rgen] ? axi_s.awlen	: 0;
		axi_m[rgen].awsize  = validwaddr[rgen] ? axi_s.awsize	: 0;
		axi_m[rgen].awburst = validwaddr[rgen] ? axi_s.awburst	: 0;
		axi_m[rgen].wdata   = validwaddr[rgen] ? axi_s.wdata	: 0;
		axi_m[rgen].wstrb   = validwaddr[rgen] ? axi_s.wstrb	: 'd0;
		axi_m[rgen].wvalid  = validwaddr[rgen] ? axi_s.wvalid	: 1'b0;
		axi_m[rgen].bready  = validwaddr[rgen] ? axi_s.bready	: 1'b0;
		axi_m[rgen].wlast   = validwaddr[rgen] ? axi_s.wlast	: 1'b0;
	end
end
endgenerate

always_comb begin
	unique case(1'b1)
		validwaddr[11]: begin
			axi_s.awready = axi_m[11].awready;
			axi_s.bresp   = axi_m[11].bresp;
			axi_s.bvalid  = axi_m[11].bvalid;
			axi_s.wready  = axi_m[11].wready;
		end
		validwaddr[10]: begin
			axi_s.awready = axi_m[10].awready;
			axi_s.bresp   = axi_m[10].bresp;
			axi_s.bvalid  = axi_m[10].bvalid;
			axi_s.wready  = axi_m[10].wready;
		end
		validwaddr[9]: begin
			axi_s.awready = axi_m[9].awready;
			axi_s.bresp   = axi_m[9].bresp;
			axi_s.bvalid  = axi_m[9].bvalid;
			axi_s.wready  = axi_m[9].wready;
		end
		validwaddr[8]: begin
			axi_s.awready = axi_m[8].awready;
			axi_s.bresp   = axi_m[8].bresp;
			axi_s.bvalid  = axi_m[8].bvalid;
			axi_s.wready  = axi_m[8].wready;
		end
		validwaddr[7]: begin
			axi_s.awready = axi_m[7].awready;
			axi_s.bresp   = axi_m[7].bresp;
			axi_s.bvalid  = axi_m[7].bvalid;
			axi_s.wready  = axi_m[7].wready;
		end
		validwaddr[6]: begin
			axi_s.awready = axi_m[6].awready;
			axi_s.bresp   = axi_m[6].bresp;
			axi_s.bvalid  = axi_m[6].bvalid;
			axi_s.wready  = axi_m[6].wready;
		end
		validwaddr[5]: begin
			axi_s.awready = axi_m[5].awready;
			axi_s.bresp   = axi_m[5].bresp;
			axi_s.bvalid  = axi_m[5].bvalid;
			axi_s.wready  = axi_m[5].wready;
		end
		validwaddr[4]: begin
			axi_s.awready = axi_m[4].awready;
			axi_s.bresp   = axi_m[4].bresp;
			axi_s.bvalid  = axi_m[4].bvalid;
			axi_s.wready  = axi_m[4].wready;
		end
		validwaddr[3]: begin
			axi_s.awready = axi_m[3].awready;
			axi_s.bresp   = axi_m[3].bresp;
			axi_s.bvalid  = axi_m[3].bvalid;
			axi_s.wready  = axi_m[3].wready;
		end
		validwaddr[2]: begin
			axi_s.awready = axi_m[2].awready;
			axi_s.bresp   = axi_m[2].bresp;
			axi_s.bvalid  = axi_m[2].bvalid;
			axi_s.wready  = axi_m[2].wready;
		end
		validwaddr[1]: begin
			axi_s.awready = axi_m[1].awready;
			axi_s.bresp   = axi_m[1].bresp;
			axi_s.bvalid  = axi_m[1].bvalid;
			axi_s.wready  = axi_m[1].wready;
		end
		validwaddr[0]: begin
			axi_s.awready = axi_m[0].awready;
			axi_s.bresp   = axi_m[0].bresp;
			axi_s.bvalid  = axi_m[0].bvalid;
			axi_s.wready  = axi_m[0].wready;
		end
		default: begin
			// Assume we could write to no-man's land to not stall the bus
			axi_s.awready = 1'b1;
			axi_s.bresp = 2'b11; // No slave at address
			axi_s.bvalid = 1'b1;
			axi_s.wready = 1'b1;
		end
	endcase
end

// ------------------------------------------------------------------------------------
// Read router
// ------------------------------------------------------------------------------------

logic [11:0] validraddr = 12'd0;

always_comb begin
	unique case(axi_s.araddr[19:16])
		addressmask[0]:  validraddr = 12'b000000000001;
		addressmask[1]:  validraddr = 12'b000000000010;
		addressmask[2]:  validraddr = 12'b000000000100;
		addressmask[3]:  validraddr = 12'b000000001000;
		addressmask[4]:  validraddr = 12'b000000010000;
		addressmask[5]:  validraddr = 12'b000000100000;
		addressmask[6]:  validraddr = 12'b000001000000;
		addressmask[7]:  validraddr = 12'b000010000000;
		addressmask[8]:  validraddr = 12'b000100000000;
		addressmask[9]:  validraddr = 12'b001000000000;
		addressmask[10]: validraddr = 12'b010000000000;
		addressmask[11]: validraddr = 12'b100000000000;
		default:		 validraddr = 12'b000000000000;
	endcase
end

genvar wgen;
generate
for (wgen=0; wgen<12; wgen++) begin
	always_comb begin
		axi_m[wgen].araddr   = validraddr[wgen] ? axi_s.araddr	: 32'd0;
		axi_m[wgen].arlen    = validraddr[wgen] ? axi_s.arlen	: 0;
		axi_m[wgen].arsize   = validraddr[wgen] ? axi_s.arsize	: 0;
		axi_m[wgen].arburst  = validraddr[wgen] ? axi_s.arburst	: 0;
		axi_m[wgen].arvalid  = validraddr[wgen] ? axi_s.arvalid	: 1'b0;
		axi_m[wgen].rready   = validraddr[wgen] ? axi_s.rready	: 1'b0;
	end
end
endgenerate

always_comb begin
	unique case(1'b1)
		validraddr[11]: begin
			axi_s.arready = axi_m[11].arready;
			axi_s.rdata   = axi_m[11].rdata;
			axi_s.rresp   = axi_m[11].rresp;
			axi_s.rvalid  = axi_m[11].rvalid;
			axi_s.rlast   = axi_m[11].rlast;
		end
		validraddr[10]: begin
			axi_s.arready = axi_m[10].arready;
			axi_s.rdata   = axi_m[10].rdata;
			axi_s.rresp   = axi_m[10].rresp;
			axi_s.rvalid  = axi_m[10].rvalid;
			axi_s.rlast   = axi_m[10].rlast;
		end
		validraddr[9]: begin
			axi_s.arready = axi_m[9].arready;
			axi_s.rdata   = axi_m[9].rdata;
			axi_s.rresp   = axi_m[9].rresp;
			axi_s.rvalid  = axi_m[9].rvalid;
			axi_s.rlast   = axi_m[9].rlast;
		end
		validraddr[8]: begin
			axi_s.arready = axi_m[8].arready;
			axi_s.rdata   = axi_m[8].rdata;
			axi_s.rresp   = axi_m[8].rresp;
			axi_s.rvalid  = axi_m[8].rvalid;
			axi_s.rlast   = axi_m[8].rlast;
		end
		validraddr[7]: begin
			axi_s.arready = axi_m[7].arready;
			axi_s.rdata   = axi_m[7].rdata;
			axi_s.rresp   = axi_m[7].rresp;
			axi_s.rvalid  = axi_m[7].rvalid;
			axi_s.rlast   = axi_m[7].rlast;
		end
		validraddr[6]: begin
			axi_s.arready = axi_m[6].arready;
			axi_s.rdata   = axi_m[6].rdata;
			axi_s.rresp   = axi_m[6].rresp;
			axi_s.rvalid  = axi_m[6].rvalid;
			axi_s.rlast   = axi_m[6].rlast;
		end
		validraddr[5]: begin
			axi_s.arready = axi_m[5].arready;
			axi_s.rdata   = axi_m[5].rdata;
			axi_s.rresp   = axi_m[5].rresp;
			axi_s.rvalid  = axi_m[5].rvalid;
			axi_s.rlast   = axi_m[5].rlast;
		end
		validraddr[4]: begin
			axi_s.arready = axi_m[4].arready;
			axi_s.rdata   = axi_m[4].rdata;
			axi_s.rresp   = axi_m[4].rresp;
			axi_s.rvalid  = axi_m[4].rvalid;
			axi_s.rlast   = axi_m[4].rlast;
		end
		validraddr[3]: begin
			axi_s.arready = axi_m[3].arready;
			axi_s.rdata   = axi_m[3].rdata;
			axi_s.rresp   = axi_m[3].rresp;
			axi_s.rvalid  = axi_m[3].rvalid;
			axi_s.rlast   = axi_m[3].rlast;
		end
		validraddr[2]: begin
			axi_s.arready = axi_m[2].arready;
			axi_s.rdata   = axi_m[2].rdata;
			axi_s.rresp   = axi_m[2].rresp;
			axi_s.rvalid  = axi_m[2].rvalid;
			axi_s.rlast   = axi_m[2].rlast;
		end
		validraddr[1]: begin
			axi_s.arready = axi_m[1].arready;
			axi_s.rdata   = axi_m[1].rdata;
			axi_s.rresp   = axi_m[1].rresp;
			axi_s.rvalid  = axi_m[1].rvalid;
			axi_s.rlast   = axi_m[1].rlast;
		end
		validraddr[0]: begin
			axi_s.arready = axi_m[0].arready;
			axi_s.rdata   = axi_m[0].rdata;
			axi_s.rresp   = axi_m[0].rresp;
			axi_s.rvalid  = axi_m[0].rvalid;
			axi_s.rlast   = axi_m[0].rlast;
		end
		default: begin
			// Assume we could read from no-man's land to not stall the bus
			axi_s.arready = 1'b1;
			axi_s.rdata   = 'd0;
			axi_s.rresp   = 2'b11; // No slave at address
			axi_s.rvalid  = 1'b1;
			axi_s.rlast   = 1'b1;
		end
	endcase
end

endmodule
