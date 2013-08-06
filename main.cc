#include "LPC11xx.h"
#include "Firmware/Log.h"
#include "Firmware/Runtime.h"
#include "Firmware/Target/LPC11xx/I2C.h"

using namespace TheFirmware::Log;
using namespace TheFirmware::LPC11xx;

uint8_t a[5];

extern "C" int main() {
	TheFirmware::Runtime::Init();

	LogInfo("Starting up");


	if (!I2C.enable()) {
		LogError("Could not enable I2C");
	}

	#define TEMP_ADDR 0x90

	while (1) {
	a[0] = TEMP_ADDR;
	a[1] = 0x2;
	a[2] = TEMP_ADDR | 0x1;

	a[3] = 0xFF;
	a[4] = 0xFF;

	I2C.send(a, 3, &a[3], 2);

	LogInfo("Got %x %x %x", a[2], a[3], a[4]);
}

	while (1)
	{}
}