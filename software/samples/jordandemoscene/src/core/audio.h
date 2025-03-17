#ifndef AUDIO_H
#define AUDIO_H
#include "zlib/zlib.h"
#include "task.h"

#define MAX_CHANNELS        4

// Task Context and ID for the Audio Task
static struct STaskContext* audioTaskCtx = nullptr;
static uint32_t audioTaskID = 0;

extern void audioTask(void);  // Function running the audio loop
extern void audioShutdown(void); // Graceful shutdown

extern void audioInit(uint8_t* audioData, uint32_t audioByteCt);
extern void audioTick(void);
extern void audioPlay(int32_t channel, uint8_t* adpcm, int32_t byteCt, bool loop = false, uint8_t vl = 255, uint8_t vr = 255);
extern bool audioChannelPlaying(int32_t channel);
extern void audioStartMusic(void);

#endif
