#include "LPC11xx.h"
#include "Firmware/Log.h"
#include "Firmware/Runtime.h"
#include "Firmware/Schedule/Task.h"
#include "Firmware/Schedule/Waitable.h"
#include "Firmware/Schedule/Semaphore.h"

using namespace TheFirmware::Log;
using namespace TheFirmware::Schedule;

Task idleTask;

Task task2;

uint32_t task2Stack[200];

Task task3;

uint32_t task3Stack[200];

Waitable w;
Semaphore s(0);

TaskStack InitStack(void(*func)(void*),void *param, TaskStack stack)
{
    *(stack--) = (uint32_t)0x01000000L;      /* xPSR	        */
	*(stack--) = (uint32_t)func;             /* Entry point of task.                         */
	*(stack)   = (uint32_t)0xFFFFFFFEL;
    stack      = stack - 5;
	*(stack)   = (uint32_t)param;            /* r0: argument */
	stack      = stack - 8;
  	
    return (stack);                   /* Returns location of new stack top. */
}

void Blub(void* param)
{
	uint32_t a = (uint32_t)param;

	LogInfo("Start %u", a);

	for(int i =0;; i++) {
		LogWarn("Ya %i", i);
		if (i % 3 == 0)
			w.wakeup();
		if (i % 5 == 0) {
			s.signal();
			s.signal();
		}

		ForceTaskSwitch();
	}
}

void Blub2(void* param)
{
	uint32_t a = (uint32_t)param;

	LogInfo("Start %u", a);

	for(int i =0; i < 100; i++) {
		uint8_t j = WaitMultiple(2, &w, &s);
		LogDebug("Gna %i: %i", i, j);
	}

	defaultTask.setPriority(1);
}


extern "C" int main() {
	TheFirmware::Runtime::Init();

	TheFirmware::Schedule::Init();

	LogInfo("Starting up");

	task2.stack = InitStack(Blub, (void*)0xF1, task2Stack + 190);
	task2.priority = 0;
	task2.setState(kTaskStateReady);

	task3.stack = InitStack(Blub2, (void*)0xAB, task3Stack + 190);
	task3.priority = 0;
	task3.setState(kTaskStateReady);

	//while(0) {
		ForceTaskSwitch();
		LogInfo("blub");

		defaultTask.setPriority(-1);
		LogInfo("done");
	//}
}