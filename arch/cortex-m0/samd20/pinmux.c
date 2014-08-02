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

#include <pinmux.h>

#include <stdbool.h>

#include "samd20.h"

#define SYSTEM_PINMUX_GPIO    (1 << 7)


enum system_pinmux_pin_dir {
	/** The pin's input buffer should be enabled, so that the pin state can
	 *  be read. */
	SYSTEM_PINMUX_PIN_DIR_INPUT,
	/** The pin's output buffer should be enabled, so that the pin state can
	 *  be set (but not read back). */
	SYSTEM_PINMUX_PIN_DIR_OUTPUT,
	/** The pin's output and input buffers should both be enabled, so that the
	 *  pin state can be set and read back. */
	SYSTEM_PINMUX_PIN_DIR_OUTPUT_WITH_READBACK,
};

/**
 * \brief Port pin input pull configuration enum.
 *
 * Enum for the possible pin pull settings of the port pin configuration
 * structure, to indicate the type of logic level pull the pin should use.
 */
enum system_pinmux_pin_pull {
	/** No logical pull should be applied to the pin. */
	SYSTEM_PINMUX_PIN_PULL_NONE,
	/** Pin should be pulled up when idle. */
	SYSTEM_PINMUX_PIN_PULL_UP,
	/** Pin should be pulled down when idle. */
	SYSTEM_PINMUX_PIN_PULL_DOWN,
};

struct system_pinmux_config {
	/** MUX index of the peripheral that should control the pin, if peripheral
	 *  control is desired. For GPIO use, this should be set to
	 *  \ref SYSTEM_PINMUX_GPIO. */
	uint8_t mux_position;

	/** Port buffer input/output direction. */
	enum system_pinmux_pin_dir direction;

	/** Logic level pull of the input buffer. */
	enum system_pinmux_pin_pull input_pull;

	/** Enable lowest possible powerstate on the pin
	 *
	 *  \note All other configurations will be ignored, the pin will be disabled
	 */
	bool powersave;
};

static inline PortGroup* system_pinmux_get_group_from_gpio_pin(
		const uint8_t gpio_pin)
{
	uint8_t port_index  = (gpio_pin / 128);
	uint8_t group_index = (gpio_pin / 32);

	/* Array of available ports. */
	Port *const ports[PORT_INST_NUM] = PORT_INSTS;

	if (port_index < PORT_INST_NUM) {
		return &(ports[port_index]->Group[group_index]);
	} else {
		return 0;
	}
}


static void _system_pinmux_config(
		PortGroup *const port,
		const uint32_t pin_mask,
		const struct system_pinmux_config *const config)
{
	/* Track the configuration bits into a temporary variable before writing */
	uint32_t pin_cfg = 0;

	/* Enabled powersave mode, don't create configuration */
	if (!config->powersave) {
		/* Enable the pin peripheral mux flag if non-GPIO selected (pin mux will
		 * be written later) and store the new mux mask */
		if (config->mux_position != SYSTEM_PINMUX_GPIO) {
			pin_cfg |= PORT_WRCONFIG_PMUXEN;
			pin_cfg |= (config->mux_position << PORT_WRCONFIG_PMUX_Pos);
		}

		/* Check if the user has requested that the input buffer be enabled */
		if ((config->direction == SYSTEM_PINMUX_PIN_DIR_INPUT) ||
				(config->direction == SYSTEM_PINMUX_PIN_DIR_OUTPUT_WITH_READBACK)) {
			/* Enable input buffer flag */
			pin_cfg |= PORT_WRCONFIG_INEN;

			/* Enable pull-up/pull-down control flag if requested */
			if (config->input_pull != SYSTEM_PINMUX_PIN_PULL_NONE) {
				pin_cfg |= PORT_WRCONFIG_PULLEN;
			}

			/* Clear the port DIR bits to disable the output buffer */
			port->DIRCLR.reg = pin_mask;
		}

		/* Check if the user has requested that the output buffer be enabled */
		if ((config->direction == SYSTEM_PINMUX_PIN_DIR_OUTPUT) ||
				(config->direction == SYSTEM_PINMUX_PIN_DIR_OUTPUT_WITH_READBACK)) {
			/* Cannot use a pullup if the output driver is enabled,
			 * if requested the input buffer can only sample the current
			 * output state */
			pin_cfg &= ~PORT_WRCONFIG_PULLEN;
		}
	}

	/* The Write Configuration register (WRCONFIG) requires the
	 * pins to to grouped into two 16-bit half-words - split them out here */
	uint32_t lower_pin_mask = (pin_mask & 0xFFFF);
	uint32_t upper_pin_mask = (pin_mask >> 16);

	/* Configure the lower 16-bits of the port to the desired configuration,
	 * including the pin peripheral multiplexer just in case it is enabled */
	port->WRCONFIG.reg
		= (lower_pin_mask << PORT_WRCONFIG_PINMASK_Pos) |
			pin_cfg | PORT_WRCONFIG_WRPMUX | PORT_WRCONFIG_WRPINCFG;

	/* Configure the upper 16-bits of the port to the desired configuration,
	 * including the pin peripheral multiplexer just in case it is enabled */
	port->WRCONFIG.reg
		= (upper_pin_mask << PORT_WRCONFIG_PINMASK_Pos) |
			pin_cfg | PORT_WRCONFIG_WRPMUX | PORT_WRCONFIG_WRPINCFG |
			PORT_WRCONFIG_HWSEL;

	if(!config->powersave) {
		/* Set the pull-up state once the port pins are configured if one was
		 * requested and it does not violate the valid set of port
		 * configurations */
		if (pin_cfg & PORT_WRCONFIG_PULLEN) {
			/* Set the OUT register bits to enable the pullup if requested,
			 * clear to enable pull-down */
			if (config->input_pull == SYSTEM_PINMUX_PIN_PULL_UP) {
				port->OUTSET.reg = pin_mask;
			} else {
				port->OUTCLR.reg = pin_mask;
			}
		}

		/* Check if the user has requested that the output buffer be enabled */
		if ((config->direction == SYSTEM_PINMUX_PIN_DIR_OUTPUT) ||
				(config->direction == SYSTEM_PINMUX_PIN_DIR_OUTPUT_WITH_READBACK)) {
			/* Set the port DIR bits to enable the output buffer */
			port->DIRSET.reg = pin_mask;
		}
	}
}

void pinmux_set_mux(uint32_t mux)
{
	uint32_t pin = mux >> 16;
	PortGroup* portGroup = system_pinmux_get_group_from_gpio_pin(pin);
	uint32_t pin_mask = (1UL << (pin % 32));

	struct system_pinmux_config config = {
		.direction = SYSTEM_PINMUX_PIN_DIR_INPUT,
		.input_pull = SYSTEM_PINMUX_PIN_PULL_NONE,
		.mux_position = mux & 0xFFFF,
	};

	_system_pinmux_config(portGroup, pin_mask, &config);
}