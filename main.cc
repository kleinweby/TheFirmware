#include "LPC11xx.h"
#include "Firmware/Log.h"
#include "Firmware/Runtime.h"
#include "Firmware/Task.h"

using namespace TheFirmware::Log;

extern "C" int main() {
	TheFirmware::Runtime::Init();

	TheFirmware::Task::Init();

	LogInfo("Starting up");

	//while(0) {
		TheFirmware::Task::ForceTaskSwitch();
		LogInfo("blub");

		TheFirmware::Task::defaultTask.setPriority(-1);
		LogInfo("done");
	//}
}