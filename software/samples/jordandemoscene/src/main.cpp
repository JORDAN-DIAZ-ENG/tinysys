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

	audioInit();
	audioPlay(0, s_audioData, s_audioByteCt, true, 255, 255);


	for (;;)
	{
		audioTick();
	}


	if (true)
	{		
		Test_Scene();
	}

	return 0;
}
