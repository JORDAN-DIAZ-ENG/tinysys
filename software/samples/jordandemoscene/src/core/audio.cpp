#include "audio.h"
#include "apu.h"
#include "task.h"
#include <cstdio>
#include <cstdlib>  // Needed for malloc()
#include <cstring>
#include <cmath>    // Needed for abs()
#include <core.h>

#include "audio.inl"

#define BUF_SAMPLES (1024)
#define MAX_AUDIO_BYTES (2 * 1024 * 1024)

// Ensure CLAMP is defined
#define CLAMP(x, low, high) (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

// Decompression function using zlib
void decompress(uint8_t* src, uint32_t srcBytes, uint8_t* dest, uint32_t* destBytes)
{
	int rc = uncompress(dest, destBytes, src, srcBytes);
	if (rc != Z_OK)
	{
		printf("Decompression failed: %d\n", rc);
	}
	printf("Decompressed %d bytes\n", *destBytes);
}

// Function that runs the audio processing task
void audioTask()
{
	while (true)
	{
		audioTick();  // Process audio in the separate thread
		TaskYield();  // Yield to allow other tasks to run
	}
}

// Graceful shutdown of the audio system
void audioShutdown()
{
	if (audioTaskCtx && audioTaskID)
	{
		TaskExitTaskWithID(audioTaskCtx, audioTaskID, 0);
		audioTaskID = 0;
	}
}

// ADPCM Decoding Step Function
static inline int16_t bs_step(int8_t step, int16_t* history, int16_t* step_size)
{
	static const int16_t adpcm_table[16] = {
		154, 154, 128, 102, 77, 58, 58, 58,
		58, 58, 58, 58, 77, 102, 128, 154
	};

	int32_t scale = *step_size;
	int32_t delta = ((1 + abs(step << 1)) * scale) >> 1;
	int32_t out = *history;

	if (step <= 0) delta = -delta;
	out += delta;
	out = CLAMP(out, -32768, 32767);

	scale = (scale * adpcm_table[8 + step]) >> 6;
	*step_size = CLAMP(scale, 1, 2000);
	*history = out;

	return out;
}

class Channel
{
public:
	uint8_t* m_playPtrOrig;
	uint8_t* m_playPtr;
	uint8_t* m_playLimit;
	bool     m_loop;
	uint8_t  m_vols[2];

	int16_t  m_prevSample;
	int16_t  m_stepSize;

	bool isActive() { return m_playPtr != nullptr; }
};

class Mixer
{
public:
	Mixer() { reset(); }
	void generateBuffer(uint16_t* buf, int32_t sampleCt);
	void play(int32_t channel, uint8_t* adpcm, int32_t byteCt, bool loop, uint8_t vl, uint8_t vr);
	bool channelPlaying(int32_t channel) { return m_channels[channel].isActive(); }

private:
	Channel m_channels[MAX_CHANNELS];
	void reset() { memset(m_channels, 0, sizeof(m_channels)); }
};

void Mixer::generateBuffer(uint16_t* buf, int32_t sampleCt)
{
	static int32_t s_accL[BUF_SAMPLES];
	static int32_t s_accR[BUF_SAMPLES];

	memset(s_accL, 0, sampleCt * sizeof(int32_t));
	memset(s_accR, 0, sampleCt * sizeof(int32_t));

	for (int i = 0; i < MAX_CHANNELS; i++)
	{
		Channel* c = &m_channels[i];
		if (!c->isActive()) continue;

		for (int j = 0; j < sampleCt; j++)
		{
			if (c->m_playPtr >= c->m_playLimit)
			{
				if (c->m_loop) c->m_playPtr = c->m_playPtrOrig;
				else { c->m_playPtr = nullptr; break; }
			}

			int8_t b = (int8_t) * (c->m_playPtr++);
			int16_t sample = bs_step(b >> 4, &c->m_prevSample, &c->m_stepSize);

			s_accL[j] += (sample * c->m_vols[0]) / 256;
			s_accR[j] += (sample * c->m_vols[1]) / 256;

			sample = bs_step(b & 0xF, &c->m_prevSample, &c->m_stepSize);

			s_accL[j] += (sample * c->m_vols[0]) / 256;
			s_accR[j] += (sample * c->m_vols[1]) / 256;
		}
	}

	for (int j = 0; j < sampleCt; j++)
	{
		buf[j * 2] = (uint16_t)CLAMP(s_accL[j], -32768, 32767);
		buf[j * 2 + 1] = (uint16_t)CLAMP(s_accR[j], -32768, 32767);
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



// Global Mixer
Mixer* s_mixer = nullptr;
uint16_t* s_playbackBuffers[2];  // Correct type
uint32_t s_playbackBufferId = 0;
uint32_t s_frame = -1;

static uint8_t* s_audioDecomp;
static uint32_t s_audioDecompByteCt;

// Audio Initialization
void audioInit( uint8_t *audioData, uint32_t audioByteCt )
{
	// Allocate memory for decompression
	s_audioDecomp = (uint8_t*)malloc(MAX_AUDIO_BYTES); // Make sure this is freed later
	s_audioDecompByteCt = MAX_AUDIO_BYTES;

	// Ensure audio data exists before decompression
	if (audioData && audioByteCt > 0)
	{
		decompress(audioData, audioByteCt, s_audioDecomp, &s_audioDecompByteCt);
	}

	// Initialize the mixer
	s_mixer = new Mixer();

	// Allocate playback buffers correctly
	s_playbackBuffers[0] = (uint16_t*)APUAllocateBuffer(BUF_SAMPLES * 2 * sizeof(uint16_t));
	s_playbackBuffers[1] = (uint16_t*)APUAllocateBuffer(BUF_SAMPLES * 2 * sizeof(uint16_t));

	s_playbackBufferId = 0;

	APUSetBufferSize(BUF_SAMPLES);
	APUSetSampleRate(ASR_22_050_Hz);

	// Get the task context for the current CPU (HART 0)
	audioTaskCtx = TaskGetContext(0);

	// Start the audio task with proper TaskAdd() parameters
	uint32_t stackAddress = 0;  // Replace with an actual stack address if required
	audioTaskID = TaskAdd(audioTaskCtx, "Audio Task", audioTask, TS_RUNNING, 1000, stackAddress);
}

// Per-frame audio processing
void audioTick(void)
{
	if (!s_mixer) return;

	uint32_t tick = APUFrame();
	if (tick != s_frame)
	{
		CFLUSH_D_L1;
		APUStartDMA((uint32_t)s_playbackBuffers[s_playbackBufferId]);

		s_playbackBufferId ^= 1;
		memset(s_playbackBuffers[s_playbackBufferId], 0, BUF_SAMPLES * 2 * sizeof(uint16_t)); // Clear buffer
		s_mixer->generateBuffer(s_playbackBuffers[s_playbackBufferId], BUF_SAMPLES);

		s_frame = tick;
	}
}

// Play audio on a channel
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

	s_mixer->play(channel, decompressedBuffer, decompressedSize, loop, vl, vr);
}

// Check if a channel is playing
bool audioChannelPlaying(int32_t channel)
{
	return s_mixer->channelPlaying(channel);
}

// Start music playback
void audioStartMusic(void)
{
	audioPlay(0, s_audioDecomp, s_audioDecompByteCt);
}
