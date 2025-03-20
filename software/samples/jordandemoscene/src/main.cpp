#include "basesystem.h"
#include "core.h"
#include "vpu.h"

#include "audio.h"

#include "audio.inl"
#include "explode.inl"


#include "system.h"

#include "basesystem.h"
#include "uart.h"
#include "task.h"

#include "jmath.h"



extern void Test_Scene(void);

void AudioTask()
{
	while (1)
	{
		audioTick();
		TaskYield();
	}
}

int main(int argc, char* argv[])
{
	audioInit();
	audioDecompressPlay(0, s_audioData, s_audioByteCt, true, 255, 255);
	//audioPlay(0, s_explodeData, s_explodeByteCt, true);


	//audioPlay(0, s_explodeData, s_explodeByteCt);

	InitTimeSystem();

	PrecomputeTrigLUT();

	// Grab task context of CPU#1
	struct STaskContext* taskctx1 = TaskGetContext(1);
	// Add a new tasks to run for each HART
	uint32_t* stackAddress = new uint32_t[1024];
	int taskID1 = TaskAdd(taskctx1, "MyTaskOne", AudioTask, TS_RUNNING,  QUARTER_MILLISECOND_IN_TICKS, (uint32_t)stackAddress);
	if (taskID1 == 0)
	{
		printf("Error: No room to add new task on CPU 1\n");
	}

	//for (;;)
	//{
	//	audioTick();
	//}

	if (true)
	{		
		Test_Scene();
	}

	return 0;
}
