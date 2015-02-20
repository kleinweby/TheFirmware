//
// Copyright (c) 2014, Christian Speich
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#include <config/config.h>
#include <log.h>
#include <string.h>
#include <scheduler.h>
#include <crc/crc8_maxim.h>

eeprom_t config_eeprom;
struct config_t config;

void config_init(eeprom_t eeprom)
{
	config_eeprom = eeprom;
}

void config_load()
{
	eeprom_read(config_eeprom, 0x0, (uint8_t*)&config, sizeof(config));

	uint8_t crc_expected = crc8_maxim((uint8_t*)&config, sizeof(config));
	uint8_t crc_actual;
	eeprom_read(config_eeprom, sizeof(config), &crc_actual, sizeof(crc_actual));
	if (crc_actual != crc_expected) {
		log(LOG_LEVEL_INFO, "EEprom is invalid: %x expected %x", crc_actual, crc_expected);
		
		config_load_defaults();
	}
}

void config_load_defaults()
{
	memset(&config, 0, sizeof(config));

#if HAVE_CAN_NODE
	can_node_config_defaults(&config.can_node);
#endif
}

void config_save()
{
	eeprom_write(config_eeprom, 0x0, (uint8_t*)&config, sizeof(config));
	uint8_t crc = crc8_maxim((uint8_t*)&config, sizeof(config));
	// delay(10); // Need delay here, as the eeprom wri
	eeprom_write(config_eeprom, sizeof(config), &crc, sizeof(crc));

	log(LOG_LEVEL_INFO, "Written eeprom (crc=%x)", crc);
}
