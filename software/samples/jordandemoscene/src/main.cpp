#include "basesystem.h"
#include "core.h"
#include "vpu.h"

#include "audio.h"

#include "audio.inl"


#include "system.h"

extern void Test_Scene(void);

int main(int argc, char* argv[])
{
	PrecomputeTrigLUT();
	audioInit(s_audioData, s_audioByteCt);
	audioStartMusic();
	audioPlay(0, s_audioData, s_audioByteCt, false);


	if (true)
	{		
		Test_Scene();
	}

	audioShutdown();
	return 0;
}
