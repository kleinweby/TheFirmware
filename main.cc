#include "LPC11xx.h"
#include "Firmware/Log.h"

using namespace TheFirmware::Log;

extern "C" int main() {
	LogInfo("Starting up");
	LogDebug("Debugging");
	LogVerbose("Verbosing");
	LogWarn("Woops, this may be not so good");
	while (1)
	LogError("Yieks!");
}