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

#include "printk.h"
#include <clock.h>

#include "samd20.h"
#include <stdbool.h>
#include <string.h>

#define EDBG_CDC_MODULE              SERCOM3
#define EDBG_CDC_SERCOM_MUX_SETTING  USART_RX_3_TX_2_XCK_3
#define EDBG_CDC_SERCOM_PINMUX_PAD0  PINMUX_UNUSED
#define EDBG_CDC_SERCOM_PINMUX_PAD1  PINMUX_UNUSED
#define EDBG_CDC_SERCOM_PINMUX_PAD2  PINMUX_PA24C_SERCOM3_PAD2
#define EDBG_CDC_SERCOM_PINMUX_PAD3  PINMUX_PA25C_SERCOM3_PAD3

SercomUsart* hw = &SERCOM3->USART;

#define SHIFT 32

#define SYSTEM_PINMUX_GPIO    (1 << 7)

struct system_gclk_chan_config {
	/** Generic Clock Generator source channel. */
	gclock_generator_t source_generator;
	/** If \c true the clock configuration will be locked until the device is
	 *  reset. */
	bool write_lock;
};

static inline void system_gclk_chan_get_config_defaults(
		struct system_gclk_chan_config *const config)
{
	/* Sanity check arguments */

	/* Default configuration values */
	config->source_generator = GCLOCK_GENERATOR_0;
	config->write_lock       = false;
}

void system_gclk_chan_disable(
		const uint8_t channel)
{
	/* Select the requested generator channel */
	*((uint8_t*)&GCLK->CLKCTRL.reg) = channel;

	/* Switch to known-working source so that the channel can be disabled */
	uint32_t prev_gen_id = GCLK->CLKCTRL.bit.GEN;
	GCLK->CLKCTRL.bit.GEN = 0;

	/* Disable the generic clock */
	GCLK->CLKCTRL.reg &= ~GCLK_CLKCTRL_CLKEN;
	while (GCLK->CLKCTRL.reg & GCLK_CLKCTRL_CLKEN) {
		/* Wait for clock to become disabled */
	}

	/* Restore previous configured clock generator */
	GCLK->CLKCTRL.bit.GEN = prev_gen_id;
}

void system_gclk_chan_enable(
		const uint8_t channel)
{
	/* Select the requested generator channel */
	*((uint8_t*)&GCLK->CLKCTRL.reg) = channel;

	/* Enable the generic clock */
	GCLK->CLKCTRL.reg |= GCLK_CLKCTRL_CLKEN;
}

void system_gclk_chan_set_config(
		const uint8_t channel,
		struct system_gclk_chan_config *const config)
{
	/* Sanity check arguments */

	/* Cache the new config to reduce sync requirements */
	uint32_t new_clkctrl_config = (channel << GCLK_CLKCTRL_ID_Pos);

	/* Select the desired generic clock generator */
	new_clkctrl_config |= config->source_generator << GCLK_CLKCTRL_GEN_Pos;

	/* Enable write lock if requested to prevent further modification */
	if (config->write_lock) {
		new_clkctrl_config |= GCLK_CLKCTRL_WRTLOCK;
	}

	/* Disable generic clock channel */
	system_gclk_chan_disable(channel);

	/* Write the new configuration */
	GCLK->CLKCTRL.reg = new_clkctrl_config;
}

int _sercom_get_async_baud_val(
		const uint32_t baudrate,
		const uint32_t peripheral_clock,
		uint16_t *const baudval)
{
	/* Temporary variables  */
	uint64_t ratio = 0;
	uint64_t scale = 0;
	uint64_t baud_calculated = 0;

	/* Check if the baudrate is outside of valid range */
	if ((baudrate * 16) >= peripheral_clock) {
		/* Return with error code */
		return 1;
	}

	/* Calculate the BAUD value */
	ratio = ((16 * (uint64_t)baudrate) << SHIFT) / peripheral_clock;
	scale = ((uint64_t)1 << SHIFT) - ratio;
	baud_calculated = (65536 * scale) >> SHIFT;

	*baudval = baud_calculated;

	return 0;
}

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

struct _sercom_conf {
	/* Status of gclk generator initialization. */
	bool generator_is_set;
	/* Sercom gclk generator used. */
	gclock_generator_t generator_source;
};

static struct _sercom_conf _sercom_config;
#  define SERCOM_GCLK_ID SERCOM0_GCLK_ID_SLOW

int sercom_set_gclk_generator(
		const gclock_generator_t generator_source,
		const bool force_change)
{
	/* Check if valid option. */
	if (!_sercom_config.generator_is_set || force_change) {
		/* Create and fill a GCLK configuration structure for the new config. */
		struct system_gclk_chan_config gclk_chan_conf;
		system_gclk_chan_get_config_defaults(&gclk_chan_conf);
		gclk_chan_conf.source_generator = generator_source;
		system_gclk_chan_set_config(SERCOM_GCLK_ID, &gclk_chan_conf);
		system_gclk_chan_enable(SERCOM_GCLK_ID);

		/* Save config. */
		_sercom_config.generator_source = generator_source;
		_sercom_config.generator_is_set = true;

		return 0;
	} else if (generator_source == _sercom_config.generator_source) {
		/* Return status OK if same config. */
		return 0;
	}

	/* Return invalid config to already initialized GCLK. */
	return 01;
}

uint32_t system_gclk_chan_get_hz(
		const uint8_t channel);

uint32_t system_gclk_chan_get_hz(
		const uint8_t channel)
{
	uint8_t gen_id;

	/* Select the requested generic clock channel */
	*((uint8_t*)&GCLK->CLKCTRL.reg) = channel;
	gen_id = GCLK->CLKCTRL.bit.GEN;

	/* Return the clock speed of the associated GCLK generator */
	return gclock_get_generator(gen_id);
}

void printk_init(uint32_t baud)
{
	uint32_t pm_index     = 3 + PM_APBCMASK_SERCOM0_Pos;
	uint32_t gclk_index   = 3 + SERCOM0_GCLK_ID_CORE;

	// REG_PAC0_WPCLR |= PAC_WPCLR_WP(1);

	PM->APBCMASK.reg |= 1 << pm_index;

	pinmux_set_mux(PINMUX_PA24C_SERCOM3_PAD2);
	pinmux_set_mux(PINMUX_PA25C_SERCOM3_PAD3);

	struct system_gclk_chan_config gclk_chan_conf;
	system_gclk_chan_get_config_defaults(&gclk_chan_conf);
	gclk_chan_conf.source_generator = GCLOCK_GENERATOR_0;
	system_gclk_chan_set_config(gclk_index, &gclk_chan_conf);
	system_gclk_chan_enable(gclk_index);
	sercom_set_gclk_generator(GCLOCK_GENERATOR_0, false);

	/* USART mode with external or internal clock must be selected first by writing 0x0
	or 0x1 to the Operating Mode bit group in the Control A register (CTRLA.MODE) */

	hw->CTRLA.bit.MODE = 0x1; // Internal

	/* Communication mode (asynchronous or synchronous) must be selected by writing to
	the Communication Mode bit in the Control A register (CTRLA.CMODE) */

 	// is default no setting needed
	// USART_TRANSFER_ASYNCHRONOUSLY = 0

	/* SERCOM pad to use for the receiver must be selected by writing to the Receive
	Data Pinout bit group in the Control A register (CTRLA.RXPO) */
	hw->CTRLA.bit.RXPO = 0x3;

	/* SERCOM pads to use for the transmitter and external clock must be selected by
	writing to the Transmit Data Pinout bit in the Control A register (CTRLA.TXPO) */
	hw->CTRLA.bit.TXPO = 0x1;

	/* Character size must be selected by writing to the Character Size bit group
	in the Control B register (CTRLB.CHSIZE) */

	hw->CTRLB.reg |= SERCOM_USART_CTRLB_CHSIZE(0);
// USART_CHARACTER_SIZE_8BIT = SERCOM_USART_CTRLB_CHSIZE(0)

	/* MSB- or LSB-first data transmission must be selected by writing to the Data
	Order bit in the Control A register (CTRLA.DORD) */

	hw->CTRLA.reg |= SERCOM_USART_CTRLA_DORD;
// USART_DATAORDER_LSB = SERCOM_USART_CTRLA_DORD

	/* When parity mode is to be used, even or odd parity must be selected by writing
	to the Parity Mode bit in the Control B register (CTRLB.PMODE) and enabled by
	writing 0x1 to the Frame Format bit group in the Control A register (CTRLA.FORM) */

	hw->CTRLA.reg |= SERCOM_USART_CTRLA_FORM(0);
// USART_PARITY_NONE = 0xFF

	/* Number of stop bits must be selected by writing to the Stop Bit Mode bit in
	 the Control B register (CTRLB.SBMODE) */

	// Default
// USART_STOPBITS_1 = 0

	/* When using an internal clock, the Baud register (BAUD) must be written to
	generate the desired baud rate */

	uint16_t bval = 0;
	_sercom_get_async_baud_val(baud, system_gclk_chan_get_hz(gclk_index), &bval);
	hw->BAUD.reg = bval;

// 9600

	/* The transmitter and receiver can be enabled by writing ones to the Receiver
	Enable and Transmitter Enable bits in the Control B register (CTRLB.RXEN and CTRLB.TXEN) */
	hw->CTRLB.bit.RXEN = 1;
	hw->CTRLB.bit.TXEN = 1;
	hw->CTRLA.bit.ENABLE = 1;
}

void sync()
{
	while (hw->STATUS.reg & SERCOM_USART_STATUS_SYNCBUSY) {
		/* Wait until the synchronization is complete */
	}
}

void putchar(char c)
{
	sync();

	hw->DATA.reg = c;

	while (!(hw->INTFLAG.reg & SERCOM_USART_INTFLAG_TXC)) {
		/* Wait until data is sent */
	}
}

static int write_op(file_t f, const void* buf, size_t nbytes)
{
	for (size_t n = 0; n < nbytes; n++, buf++)
		putchar(*(char*)buf);

	return nbytes;
}

static const struct file_operations ops = {
	.write = write_op,
};

static struct file _debug_serial = {
	.ops = &ops,
};

file_t debug_serial = &_debug_serial;

void printk(const char* str)
{
	write(debug_serial, str, strlen(str));
}
