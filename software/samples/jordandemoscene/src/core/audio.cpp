#include "audio.h"
#include "apu.h"

#include <cstdio>
#include <core.h>

void decompress(uint8_t* src, uint32_t srcBytes, uint8_t* dest, uint32_t* destBytes)
{
	int rc = uncompress(dest, destBytes, src, srcBytes);
	if (rc != Z_OK)
	{
		printf("Decompression failed: %d\n", Z_OK);
	}
	printf("decompressed %d bytes\n", *destBytes);
}

class Channel
{
public:
	uint8_t* m_playPtrOrig;
	uint8_t* m_playPtr;
	uint8_t* m_playLimit;
	int32_t             m_bytesLeft;
	uint8_t             m_vols[2];
	bool                m_loop;

	int16_t             m_prevSample;
	int16_t             m_stepSize;
};


class Mixer
{
public:
	Mixer(void);

	void                generateBuffer(uint32_t* buf, int32_t sampleCt);
	void                play(int32_t channel, uint8_t* adpcm, int32_t byteCt, bool loop, uint8_t vl, uint8_t vr);
	bool                channelPlaying(int32_t channel)
	{
		return(!!m_channels[channel].m_playPtr);
	}

private:
	Channel             m_channels[MAX_CHANNELS];

	void                reset(void);
};



// https://github.com/superctr/adpcm/blob/master/bs_codec.c

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

// step ADPCM algorithm
static inline int16_t bs_step(int8_t step, int16_t* history, int16_t* step_size)
{
	static const int16_t adpcm_table[16] = {
		154, 154, 128, 102, 77, 58, 58, 58, // 2.4, 2.4, 2.0, 1.6, 1.2, 0.9, 0.9, 0.9
		58, 58, 58, 58, 77, 102, 128, 154   // 0.9, 0.9, 0.9, 0.9, 1.2, 1.6, 2.0, 2.4
	};

	int32_t scale = *step_size;
	int32_t delta = ((1 + abs(step << 1)) * scale) >> 1; // (0.5 + abs(step)) * scale
	int32_t out = *history;
	if (step <= 0)
		delta = -delta;
	out += delta;
	out = CLAMP(out, -32768, 32767);

	scale = (scale * adpcm_table[8 + step]) >> 6;
	*step_size = CLAMP(scale, 1, 2000);
	*history = out;

	return out;
}

// step high pass filter
static inline int16_t bs_hpf_step(int16_t in, int16_t* history, int32_t* state)
{
	int32_t out;
	*state = (*state >> 2) + in - *history;
	*history = in;
	out = (*state >> 1) + in;
	return CLAMP(out, -32768, 32767);
}

// encode ADPCM with high pass filter. gets better results, i think
void bs_encode(int16_t* buffer, uint8_t* outbuffer, long len)
{
	long i;

	int16_t step_size = 10;
	int16_t history = 0;
	uint8_t buf_sample = 0, nibble = 0;
	int16_t filter_history = 0;
	int32_t filter_state = 0;

	for (i = 0; i < len; i++)
	{
		int step = bs_hpf_step(*buffer++, &filter_history, &filter_state) - history;
		step = (step / step_size) >> 1;
		step = CLAMP(step, -8, 7);
		if (nibble)
			*outbuffer++ = buf_sample | (step & 15);
		else
			buf_sample = (step & 15) << 4;
		nibble ^= 1;
		bs_step(step, &history, &step_size);
	}
}

void bs_decode(uint8_t* buffer, int16_t* outbuffer, long len)
{
	long i;

	int16_t step_size = 10;
	int16_t history = 0;
	uint8_t nibble = 0;

	for (i = 0; i < len; i++)
	{
		int8_t step = (*(int8_t*)buffer) << nibble;
		step >>= 4;
		if (nibble)
			buffer++;
		nibble ^= 4;
		*outbuffer++ = bs_step(step, &history, &step_size);
	}
}

Mixer::Mixer(void)
{
	reset();
}


void Mixer::reset(void)
{
	memset(m_channels, 0, sizeof(m_channels));
}

#define MAX_SAMPLES_PER_BUFFER      ( 2048 )
static int32_t  s_accL[MAX_SAMPLES_PER_BUFFER];
static int32_t  s_accR[MAX_SAMPLES_PER_BUFFER];

void Mixer::generateBuffer(uint32_t* buf, int32_t sampleCt)
{
	// compiler bug... 
	// int8_t      b3 = ( (b&0xf) << 4 ) >> 4;
	// this does not do the sign-extension!

	static int8_t nibbleTable[] = { 0,1,2,3,4,5,6,7,-8,-7,-6,-5,-4,-3,-2,-1 };

	if (sampleCt & 1)
	{
		printf("odd sample ct\n");
		exit(-1);
	}
	if (sampleCt > MAX_SAMPLES_PER_BUFFER)
	{
		printf("too many samples\n");
		exit(-1);
	}

	bool first = true;

	for (int i = 0; i < MAX_CHANNELS; i++)
	{
		Channel* c = &m_channels[i];
		if (!c->m_playPtr)
		{
			continue;
		}

		int         volL = c->m_vols[0];
		int         volR = c->m_vols[1];

		if (first)
		{
			for (int j = 0; j < sampleCt; )
			{
				int8_t      b = (int8_t) * (c->m_playPtr++);

				int16_t     r;
				r = bs_step(b >> 4, &c->m_prevSample, &c->m_stepSize);
				s_accL[j] = (r * volL) / 256;
				s_accR[j] = (r * volR) / 256;
				j++;
				r = bs_step(nibbleTable[b & 0xf], &c->m_prevSample, &c->m_stepSize);
				s_accL[j] = (r * volL) / 256;
				s_accR[j] = (r * volR) / 256;
				j++;
				if (c->m_playPtr == c->m_playLimit)
				{
					if (c->m_loop) { c->m_playPtr = c->m_playPtrOrig; }
					else { c->m_playPtr = NULL; break; }
				}
			}
			first = false;
		}
		else
		{
			for (int j = 0; j < sampleCt; )
			{
				int8_t      b = (int8_t) * (c->m_playPtr++);
				int16_t     r;
				r = bs_step((b >> 4), &c->m_prevSample, &c->m_stepSize);
				s_accL[j] += (r * volL) / 256;
				s_accR[j] += (r * volR) / 256;
				j++;
				r = bs_step(nibbleTable[b & 0xf], &c->m_prevSample, &c->m_stepSize);
				s_accL[j] += (r * volL) / 256;
				s_accR[j] += (r * volR) / 256;
				j++;
				if (c->m_playPtr == c->m_playLimit)
				{
					if (c->m_loop) { c->m_playPtr = c->m_playPtrOrig; }
					else { c->m_playPtr = NULL; break; }
				}
			}
		}
	}

	if (first)
	{
		memset(s_accL, 0, sampleCt * sizeof(uint32_t));
		memset(s_accR, 0, sampleCt * sizeof(uint32_t));
	}

	for (int j = 0; j < sampleCt; j++)
	{
		uint16_t    l = (uint16_t)CLAMP(s_accL[j], -32768, 32767);
		uint16_t    r = (uint16_t)CLAMP(s_accR[j], -32768, 32767);
		*(buf++) = ((uint32_t)l << 16) | r;
	}
}


void Mixer::play(int32_t channel, uint8_t* adpcm, int32_t byteCt, bool loop, uint8_t vl, uint8_t vr)
{
	m_channels[channel].m_playPtrOrig = adpcm;
	m_channels[channel].m_playPtr = adpcm;
	m_channels[channel].m_playLimit = adpcm + byteCt;
	m_channels[channel].m_prevSample = 0;
	m_channels[channel].m_stepSize = 10;
	m_channels[channel].m_vols[0] = vl;
	m_channels[channel].m_vols[1] = vr;
	m_channels[channel].m_loop = loop;
}



Mixer* s_mixer = NULL;
uint32_t* s_playbackBuffers[2];
uint32_t        s_playbackBufferId = 0;
uint32_t        s_frame = -1;

#define BUF_SAMPLES     ( 1024 )

#include "audio.inl"

#define MAX_AUDIO_BYTES     ( 2 * 1024 * 1024 )
static uint8_t* s_audioDecomp;
static uint32_t     s_audioDecompByteCt;



void audioInit(void)
{
	s_audioDecomp = (uint8_t*)malloc(MAX_AUDIO_BYTES);
	s_audioDecompByteCt = MAX_AUDIO_BYTES;
	decompress(s_audioData, s_audioByteCt, s_audioDecomp, &s_audioDecompByteCt);

	s_mixer = new Mixer();
	s_playbackBuffers[0] = (uint32_t*)APUAllocateBuffer(BUF_SAMPLES * 2 * sizeof(uint16_t));
	s_playbackBuffers[1] = (uint32_t*)APUAllocateBuffer(BUF_SAMPLES * 2 * sizeof(uint16_t));
	s_playbackBufferId = 0;
	APUSetBufferSize(BUF_SAMPLES);
	APUSetSampleRate(ASR_22_050_Hz);
	//APUSetSampleRate(ASR_44_100_Hz);
}

void audioTick(void)
{
	if (!s_mixer)
		return;

	uint32_t tick = APUFrame();
	if (tick != s_frame)
	{
		// Ensure memory writes are flushed before DMA transfer
		CFLUSH_D_L1;

		// Start DMA transfer with properly aligned buffer
		APUStartDMA((uint32_t)s_playbackBuffers[s_playbackBufferId]);

		// Generate new audio data for the next frame
		s_playbackBufferId ^= 1;
		memset(s_playbackBuffers[s_playbackBufferId], 0, BUF_SAMPLES * 2 * sizeof(uint16_t)); // Clear buffer
		s_mixer->generateBuffer(s_playbackBuffers[s_playbackBufferId], BUF_SAMPLES);

		s_frame = tick;
	}
}



void audioPlay(int32_t channel, uint8_t* compressedAdpcm, int32_t compressedSize, bool loop, uint8_t vl, uint8_t vr)
{
	static uint8_t decompressedBuffer[MAX_AUDIO_BYTES];
	uint32_t decompressedSize = MAX_AUDIO_BYTES;

	// Decompress the ADPCM data
	decompress(compressedAdpcm, compressedSize, decompressedBuffer, &decompressedSize);

	if (decompressedSize == 0)
	{
		printf("Decompression failed!\n");
		return;
	}

	// Ensure decompressed PCM data is valid
	s_mixer->play(channel, decompressedBuffer, decompressedSize, loop, vl, vr);
}


bool audioChannelPlaying(int32_t channel)
{
	return(s_mixer->channelPlaying(channel));
}

void audioStartMusic(void)
{
	audioPlay(0, s_audioDecomp, s_audioDecompByteCt);
}