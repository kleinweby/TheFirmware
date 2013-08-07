#include "LPC11xx.h"
#include "Firmware/Log.h"
#include "Firmware/Runtime.h"
#include "Firmware/Devices/MCP9800.h"
#include "Firmware/Devices/24XX64.h"

using namespace TheFirmware;
using namespace TheFirmware::Log;
using namespace TheFirmware::LPC11xx;

Devices::MCP9800 MCP9800;
Devices::MCP24XX64 MCP24XX64;

extern "C" int main() {
	TheFirmware::Runtime::Init();

	LogInfo("Starting up");


	if (!I2C.enable()) {
		LogError("Could not enable I2C");
	}

	// {
	// 	char buffer[3] = {'H', 'a', 'l'};

	// 	MCP24XX64.write(0x0, (uint8_t*)buffer, 3);
	// }
	// 

	for (uint32_t i = 0; i < 10000000; i++) {};

	char buffer[5] = {'x', 'x', 'x', 'x', '\0'};

	MCP24XX64.read(0x0, (uint8_t*)buffer, 4);

	MCP9800.setResolution(12);

	while (1) {

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

	LogInfo("Got %u.%04u", temperature[0], fraction);

	for (uint32_t i = 0; i < 10000000; i++) {};
	}

	while (1)
	{}
}