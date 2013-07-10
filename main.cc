#include "LPC11xx.h"
#include "Firmware/Log.h"
#include "Firmware/Runtime.h"

using namespace TheFirmware::Log;

extern "C" int main() {
	TheFirmware::Runtime::Init();
	
	LogInfo("Starting up");
	LogDebug("Debugging");
	LogVerbose("Verbosing");
	LogWarn("Woops, this may be not so good");
	while (1)
	LogError("Yieks!");
}