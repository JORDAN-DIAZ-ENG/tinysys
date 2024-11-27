/**
 * @file task.c
 * 
 * @brief Task management for the SDK
 */

#include "core.h"
#include "basesystem.h"
#include "task.h"
#include "leds.h"

#include <stdlib.h>

/**
 * @brief Initialize the task system
 * 
 * Initialize the task system with the given task context and HART ID.
 * @note Tasks are core local at this point, and won't migrate between cores.
 * 
 * @param _ctx Task context
 * @param _hartid HART ID
 */
void TaskInitSystem(struct STaskContext *_ctx, uint32_t _hartid)
{
	_ctx->currentTask = 0;
	_ctx->numTasks = 0;
	_ctx->interceptUART = 0;
	_ctx->kernelError = 0;
	_ctx->hartID = _hartid;

	// Clean out all tasks
	for (uint32_t i=0; i<TASK_MAX; ++i)
	{
		struct STask *task = &_ctx->tasks[i];
		task->HART = 0x0;				// Default affinity mask is HART#0
		task->regs[0] = 0x0;			// Initial PC
		task->regs[2] = 0x0;			// Initial stack pointer
		task->regs[8] = 0x0;			// Frame pointer
		task->state = TS_UNKNOWN;
		task->name[0] = 0; // No name
	}
}

/**
 * @brief Get the task context of a specific HART
 * 
 * Get the task context of a specific HART.
 * 
 * @param _hartid HART ID
 * @return Task context
 */
struct STaskContext *TaskGetContext(uint32_t _hartid)
{
	return (struct STaskContext *)(DEVICE_MAIL + sizeof(struct STaskContext)*_hartid);
}

/**
 * @brief Get the shared memory of a specific HART
 * 
 * Get the shared memory for all HARTs
 * 
 * @param _hartid HART ID
 * @return Shared memory address
 */
void *TaskGetSharedMemory()
{
	return (void *)(DEVICE_MAIL + sizeof(struct STaskContext)*MAX_HARTS);
}

/**
 * @brief Set the state of a task
 * 
 * Transition a task to a new state.
 * 
 * @param _ctx Task context
 * @param _taskid Task ID
 * @param _state New state
 */
void TaskSetState(struct STaskContext *_ctx, const uint32_t _taskid, enum ETaskState _state)
{
	_ctx->tasks[_taskid].state = _state;
}

/**
 * @brief Get the state of a task
 * 
 * Get the current state of a task.
 * 
 * @param _ctx Task context
 * @param _taskid Task ID
 * @return Task state
 */
enum ETaskState TaskGetState(struct STaskContext *_ctx, const uint32_t _taskid)
{
	return _ctx->tasks[_taskid].state;
}

/**
 * @brief Get the program counter of a task
 * 
 * Get the program counter of a task.
 * 
 * @param _ctx Task context
 * @param _taskid Task ID
 * @return Program counter
 *
*/
uint32_t TaskGetPC(struct STaskContext *_ctx, const uint32_t _taskid)
{
	return _ctx->tasks[_taskid].regs[0];
}

/**
 * @brief Add a new task to the task pool
 * 
 * This function adds a new task to the task pool of the given task context which belongs to a specific HART.
 * 
 * @param _ctx Task context
 * @param _name Task name
 * @param _task Task function
 * @param _initialState Initial state
 * @param _runLength Time slice dedicated to this task (can be overriden by calling TaskYield())
 * @return Task ID
 */
int TaskAdd(struct STaskContext *_ctx, const char *_name, taskfunc _task, enum ETaskState _initialState, const uint32_t _runLength)
{
	uint32_t context = (uint32_t)_ctx;
	uint32_t name = (uint32_t)_name;
	uint32_t task = (uint32_t)_task;
	int retval = 0;

	asm (
		"li a7, 16384;" // _task_add custom syscall
		"mv a0, %1;"
		"mv a1, %2;"
		"mv a2, %3;"
		"mv a3, %4;"
		"mv a4, %5;"
		"ecall;"
		"mv %0, a0;" :
		// Return values
		"=r" (retval) :
		// Input parameters
		"r" (context), "r" (name), "r" (task), "r" (_initialState), "r" (_runLength) :
		// Clobber list
		"a0", "a1", "a2", "a3", "a4", "a7"
	);

	return retval;
}

/**
 * @brief Exit the current task
 * 
 * Exit the current task as soon as possible by marking it as 'terminating'.
 * The scheduler will take care of the rest on next task switch.
 * 
 * @param _ctx Task context
 * @see TaskExitTaskWithID
 */
void TaskExitCurrentTask(struct STaskContext *_ctx)
{
	uint32_t context = (uint32_t)_ctx;

	asm (
		"li a7, 16387;" // _task_exit_current_task custom syscall
		"mv a0, %0;"
		"ecall;" :
		// Return values
		:
		// Input parameters
		"r" (context) :
		// Clobber list
		"a0", "a7"
	);
}

/**
 * @brief Exit a specific task
 * 
 * Exit a specific task by marking it as 'terminating'.
 * The scheduler will take care of the rest on next task switch.
 * 
 * @param _ctx Task context
 * @param _taskid Task ID
 * @param _signal Exit code to return from the task
 * @see TaskExitCurrentTask
 */
void TaskExitTaskWithID(struct STaskContext *_ctx, uint32_t _taskid, uint32_t _signal)
{
	uint32_t context = (uint32_t)_ctx;

	asm (
		"li a7, 16386;" // _task_exit_task_with_id custom syscall
		"mv a0, %0;"
		"mv a1, %1;"
		"mv a2, %2;"
		"ecall;" :
		// Return values
		:
		// Input parameters
		"r" (context), "r" (_taskid), "r" (_signal) :
		// Clobber list
		"a0", "a1", "a2", "a7"
	);
}

/**
 * @brief Yield leftover time back to the next task in chain
 * 
 * Yield leftover time back to the next task in chain.
 * Also returns the current time in clock ticks.
 * 
 * @see TaskSwitchToNext
 */
void TaskYield()
{
	asm (
		"li a7, 16388;" // _task_yield custom syscall
		"ecall;" : : :
		// Clobber list
		"a0", "a7"
	);
}

/**
 * @brief Switch to next task and return its total time slice
 * 
 * Switch to the next task in the task pool and return time slice of the next task.
 * This function is called by the scheduler to switch between tasks.
 * The current task's registers are stored in the task structure before switching.
 * 
 * @note Please call TaskYield() from user code to yield leftover time back to the next task instead.
 * 
 * @param _ctx Task context
 * @return Time slice of the next task
 * @see TaskYield
 */
uint32_t TaskSwitchToNext(struct STaskContext *_ctx)
{
	uint32_t context = (uint32_t)_ctx;
	int retval = 0;

	asm (
		"li a7, 16385;" // _task_switch_to_next custom syscall
		"mv a0, %1;"
		"ecall;"
		"mv %0, a0;" :
		// Return values
		"=r" (retval) :
		// Input parameters
		"r" (context) :
		// Clobber list
		"a0", "a7"
	);

	return retval;
}
