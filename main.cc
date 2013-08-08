#include "LPC11xx.h"
#include "Firmware/Log.h"
#include "Firmware/Runtime.h"
#include "Firmware/Task.h"

using namespace TheFirmware::Log;

extern "C" int main() {
	TheFirmware::Runtime::Init();

	TheFirmware::Task::Init();

	LogInfo("Starting up");

	// Initial task switch
	__asm volatile (
		// Move to psp
		"MRS R1, MSP\n"
		"MSR PSP, R1\n"
		"MOVS R1, #2\n"
		"MSR CONTROL, R1\n"

		// trigger pendsv
		"STR %0, [%1]"
		:
		: "r" (0x10000000), "r" (0xE000ED04)
		: "r1"
	);
	while(1) {
		*((uint32_t volatile *)0xE000ED04) = 0x10000000; // trigger PendSV
		LogInfo("blub");
	}
}