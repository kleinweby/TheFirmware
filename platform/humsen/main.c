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
#include <can_node/can_node.h>
#include <24xx64.h>
#include <peripheral/i2c_dev.h>
#include <log.h>
#include <scheduler.h>
#include <sht2x.h>

i2c_dev_t i2c_dev;
sht2x_t sht2x;

void platform_main(void) 
{
	i2c_dev = i2c_dev_create();
	eeprom_t config_eeprom = mcp24xx64_create(i2c_dev);
	config_init(config_eeprom);
	config_load();

	sht2x = sht2x_create(i2c_dev);
	sensors_register((sensor_t)sht2x);

	if (can_node_valid_id(config.can.node_id) && config.can.node_id != 0x0) {
		can_node_init(config.can.node_id, config.can.speed);

		for(;;)
			can_node_loop();
	}
	else {
		log(LOG_LEVEL_WARN, "No can node id set, not enabling can.");
	}

	for (;;)
		thread_block();
}