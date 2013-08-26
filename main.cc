#include "LPC11xx.h"
#include "Firmware/Log.h"
#include "Firmware/Runtime.h"
#include "Firmware/Schedule/Task.h"
#include "Firmware/Time/Systick.h"
#include "Firmware/Time/Delay.h"
#include "Firmware/Devices/MCP9800.h"
#include "Firmware/Devices/24XX64.h"

using namespace TheFirmware;
using namespace TheFirmware::Log;
using namespace TheFirmware::LPC11xx;
using namespace TheFirmware::Time;

Devices::MCP9800 MCP9800;
Devices::MCP24XX64 MCP24XX64;

static LPC_GPIO_TypeDef (* const LPC_GPIO[4]) = { LPC_GPIO0, LPC_GPIO1, LPC_GPIO2, LPC_GPIO3 };

void GPIOSetValue( uint32_t portNum, uint32_t bitPosi, uint32_t bitVal )
{
  LPC_GPIO[portNum]->MASKED_ACCESS[(1<<bitPosi)] = (bitVal<<bitPosi);
}

void GPIOSetDir( uint32_t portNum, uint32_t bitPosi, uint32_t dir )
{
  if(dir)
	LPC_GPIO[portNum]->DIR |= 1<<bitPosi;
  else
	LPC_GPIO[portNum]->DIR &= ~(1<<bitPosi);
}

extern "C" int main() {
	TheFirmware::Runtime::Init();

	TheFirmware::Schedule::Init();

	LogDebug("Starting up");


	SystemCoreClockUpdate();
	// LPC_SYSCON->SYSAHBCLKCTRL |= (1<<6);


	LPC_IOCON->R_PIO1_2 |= 0x1;
	LPC_IOCON->R_PIO1_1 |= 0x1;
	LPC_IOCON->R_PIO1_0 |= 0x1;

	GPIOSetDir(1,2,1);
	GPIOSetDir(1,1,1);
	GPIOSetDir(1,0,1);
	// for (uint32_t i = 0; i < 10000000; i++) {};

	GPIOSetValue(1,2,1);
	GPIOSetValue(1,1,0);
	GPIOSetValue(1,0,0);


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
		LogInfo("Got %u.%04u", temperature[0], fraction);

		//LogDebug("Tick %i", CurrentSysTicks);

		I2C.disable();

		Schedule::Wait(&timeout);
	}

	while (1)
	{}
}