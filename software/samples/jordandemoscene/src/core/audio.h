#ifndef AUDIO_H
#define AUDIO_H
#include "zlib/zlib.h"

#define MAX_CHANNELS        4


extern void audioInit(void);
extern void audioTick(void);

extern void audioPlay(int32_t channel, uint8_t* adpcm, int32_t byteCt, bool loop = false, uint8_t vl = 255, uint8_t vr = 255);
extern bool audioChannelPlaying(int32_t channel);

extern void audioStartMusic(void);

#endif