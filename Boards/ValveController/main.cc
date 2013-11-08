#include "LPC11xx.h"
#include "Firmware/Runtime.h"
#include "Firmware/Console/Console.h"
#include "Firmware/Schedule/Task.h"
#include "Firmware/Time/Systick.h"
#include "Firmware/Time/Delay.h"
#include "Firmware/Devices/MCP9800.h"
#include "Firmware/Devices/24XX64.h"
#include "Firmware/GPIO.h"
#include "Firmware/Console/Console.h"
#include "Firmware/Console/CLI.h"
#include "Valve.h"

using namespace TheFirmware;
using namespace TheFirmware::Console;
using namespace TheFirmware::LPC11xx;
using namespace TheFirmware::Time;

Devices::MCP9800 MCP9800;
Devices::MCP24XX64 MCP24XX64;

GPIO greenLED(1, 2);
GPIO yellowLED(1, 1);
GPIO redLED(1, 0);

uint32_t temp[2];

extern "C" int main() {
	TheFirmware::Runtime::Init();

	TheFirmware::Schedule::Init();

	TheFirmware::Console::Init();

	LogDebug("Starting up");

	GardenaValve::Init();

	SystemCoreClockUpdate();
	// LPC_SYSCON->SYSAHBCLKCTRL |= (1<<6);

	LPC_IOCON->R_PIO1_2 |= 0x1;
	LPC_IOCON->R_PIO1_1 |= 0x1;
	LPC_IOCON->R_PIO1_0 |= 0x1;

	greenLED.setDirection(GPIODirectionOutput);
	yellowLED.setDirection(GPIODirectionOutput);
	redLED.setDirection(GPIODirectionOutput);
	// for (uint32_t i = 0; i < 10000000; i++) {};

	greenLED.set(true);
	yellowLED.set(false);
	redLED.set(false);


	if (!I2C.enable()) {
		LogError("Could not enable I2C");
	}

	// {
	// 	char buffer[3] = {'H', 'a', 'l'};

	// 	MCP24XX64.write(0x0, (uint8_t*)buffer, 3);
	// }
	// 

	// char buffer[5] = {'x', 'x', 'x', 'x', '\0'};

	// MCP24XX64.read(0x0, (uint8_t*)buffer, 4);

	MCP9800.setResolution(12);
	MCP9800.setOneShot(true);

	I2C.disable();

	WaitableTimeout timeout(60 * 1000, SysTickTimer, true);

	while (1) {

		if (!I2C.enable()) {
			LogError("Could not enable I2C");
		}

		uint8_t temperature[2];
		uint16_t tmp;

		tmp = MCP9800.readTemperature();

		temperature[0] = (tmp >> 8) & 0xFF;
		temperature[1] = (tmp >> 0) & 0xFF;

		uint32_t fraction = 0;

		if ((temperature[1] & 0x80) != 0)
			fraction += 5000;
		if ((temperature[1] & 0x40) != 0)
			fraction += 2500;
		if ((temperature[1] & 0x20) != 0)
			fraction += 1250;
		if ((temperature[1] & 0x10) != 0)
			fraction += 625;

		// MCP9800.setOneShot(true);
		temp[0] = temperature[0];
		temp[1] = fraction;

		I2C.disable();

		Schedule::Wait(&timeout);
		yellowLED.set(!yellowLED.get());
	}

	while (1)
	{}
}

void temp_cmd(int argc, char** argv)
{
	printf("Temp: %u.%04u\r\n", temp[0], temp[1]);
}

REGISTER_COMMAND(temp, {
	.func = temp_cmd,
	.help = "Read the temperature"
});
