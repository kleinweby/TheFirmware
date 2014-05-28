//
// Copyright (c) 2013, Christian Speich
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

#include "bootstrap.h"

#include <arch.h>
#include <irq.h>

#include <stdint.h>

#include <test.h>
#include <log.h>
#include <printk.h>
#include <malloc.h>
#include <thread.h>
#include <scheduler.h>
#include <string.h>
#include <systick.h>
#include <console.h>

#include "LPC11xx.h"
#include "system_LPC11xx.h"

void* test_staticnode = NULL;

int hello(int argc, const char** argv) {
	log(LOG_LEVEL_INFO, "Hello");
	return 0;
}

size_t printf(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	size_t len = vfprintf(debug_serial, format, args);
	va_end(args);

	return len;
}

static bool adc_inited = false;
static const uint8_t chan = 7;

uint32_t adc_read()
{
	if (!adc_inited) {
		// Disable Power down bit to the ADC block.
		LPC_SYSCON->PDRUNCFG &= ~(0x1<<4);
		// Enable ADC
		LPC_SYSCON->SYSAHBCLKCTRL |= (1<<13);
		// Set clock
		LPC_ADC->CR = ((SystemCoreClock/LPC_SYSCON->SYSAHBCLKDIV)/4500000-1)<<8;
		LPC_IOCON->PIO1_11   = 0x01;
	}

	// Start conv
	LPC_ADC->CR &= 0xFFFFFF00;
	LPC_ADC->CR |= (1 << 24) | (1 << chan);

	uint32_t val;

	do {
		val = LPC_ADC->GDR;
	} while (!(val & (1 << 31))); // Done bit

	// Stop
	LPC_ADC->CR &= 0xF8FFFFFF;

	int human_val = (((val >> 6) & 0x3FF) * 16) * 3300;

	return human_val / 1000;
}

static LPC_GPIO_TypeDef (* const LPC_GPIO[4]) = { LPC_GPIO0, LPC_GPIO1, LPC_GPIO2, LPC_GPIO3 };

#define PIN(port, pin) ((port << 16) | pin)

void gpio_set_out(int pin, bool out)
{
	int port = pin >> 16;
	pin &= 0xFFFF;

	if (out) {
		LPC_GPIO[port]->DIR |= 1<<pin;
	}
	else {
		LPC_GPIO[port]->DIR &= ~(1<<pin);
	}
}

void gpio_set(int pin, bool on)
{
	int port = pin >> 16;
	pin &= 0xFFFF;

	LPC_GPIO[port]->MASKED_ACCESS[(1<<pin)] = on << pin;
}


int adc(int argc, const char** argv)
{
	printf("ADC: %d mV\r\n", adc_read());

	return 0;
}

struct {
	bool open;
	int polarity;
	uint32_t open_power;
	uint32_t close_power;
	int pin_a;
	int pin_b;
	int min_voltage;
	int max_voltage;
} valve = {
	.open = false,
	.polarity = 0,
	.open_power = 20 * 10000,
	.close_power = 5.5 * 10000,
	.min_voltage = 7000,
	.max_voltage = 14000,
	.pin_a = PIN(2,7),
	.pin_b = PIN(2,8),
};

void valve_help()
{
	printf("HELLP!!!\r\n");
}

int valve_cmd(int argc, const char** argv)
{
	if (argc < 2) {
		valve_help();
		return -1;
	}

	if (strcmp(argv[1], "help") == 0) {
		valve_help();
		return 0;
	}
	else if (strcmp(argv[1], "open") == 0) {
		uint32_t volt = adc_read();
		uint32_t delay_timeout = valve.open_power / volt;
		int pin = valve.polarity ? valve.pin_a : valve.pin_b;
		gpio_set_out(pin, true);

		printf("Opening (%d mV, %d ms, polarity = %d)...", volt, delay_timeout, valve.polarity);

		gpio_set(pin, true);
		delay(delay_timeout);
		gpio_set(pin, false);

		printf("done\r\n");
	}
	else if (strcmp(argv[1], "close") == 0) {
		uint32_t volt = adc_read();
		uint32_t delay_timeout = valve.close_power / volt;
		int pin = valve.polarity ? valve.pin_b : valve.pin_a;
		gpio_set_out(pin, true);

		printf("Closing (%d mV, %d ms, polarity = %d)...", volt, delay_timeout, valve.polarity);

		gpio_set(pin, true);
		delay(delay_timeout);
		gpio_set(pin, false);

		printf("done\r\n");
	}
	else if (strcmp(argv[1], "show") == 0) {
		printf("open-power: %d\r\n", valve.open_power);
		printf("close-power: %d\r\n", valve.close_power);
		printf("polarity: %d\r\n", valve.polarity);
	}
	else if (strcmp(argv[1], "set") == 0) {
		if (argc != 4) {
			valve_help();
			return -1;
		}

		if (strcmp(argv[2], "open-power") == 0) {
			valve.open_power = atoi(argv[3]);
			printf("open-power: %d\r\n", valve.open_power);
		}
		else if (strcmp(argv[2], "close-power") == 0) {
			valve.close_power = atoi(argv[3]);
			printf("close-power: %d\r\n", valve.close_power);
		}
		else if (strcmp(argv[2], "polarity") == 0) {
			valve.polarity = atoi(argv[3]);
			printf("polarity: %d\r\n", valve.polarity);
		}
		else
			printf("Unkown\r\n");
	}
	else {
		valve_help();
	}

	return 0;
}

STATICFS_DIR(bla,
	STATICFS_DIR_ENTRY("blabla", test),
	STATICFS_DIR_ENTRY("blabla", test),
	STATICFS_DIR_ENTRY_LAST,
);

STATICFS_DIR(root,
	STATICFS_DIR_ENTRY("test", bla),
	STATICFS_DIR_ENTRY("test2", bla),
	STATICFS_DIR_ENTRY_CALLABLE("hello", hello),
	STATICFS_DIR_ENTRY_CALLABLE("adc", adc),
	STATICFS_DIR_ENTRY_CALLABLE("valve", valve_cmd),
	STATICFS_DIR_ENTRY_LAST,
);

void error()
{
	assert(false, "some error");
}

void bootstrap()
{
	arch_early_init();
	test_do(TEST_AFTER_ARCH_EARLY_INIT);

	irq_register(IRQ_HARDFAULT, error);

	printk_init(38400);
	irq_init();

	log(LOG_LEVEL_INFO, "Starting up TheFirmware...");

	arch_late_init();
	test_do(TEST_AFTER_ARCH_LATE_INIT);

	thread_init();
	scheduler_init();

	size_t free_mem = get_free_size();

	log(LOG_LEVEL_INFO, "Bootstrap complete. (%u free)", free_mem);

	test_do(TEST_IN_MAIN_TASK);

	vfs_set_root(root);
	vfs_dump(debug_serial);

	console_spawn(debug_serial);

	while(1)
		yield(); //__asm("WFI");
}
