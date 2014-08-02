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

#if TARGET_SAMD20

#include <console.h>
#include <clock.h>

#include "samd20.h"
#include <pinmux.h>

static bool adc_inited = false;

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

static void system_gclk_chan_disable(
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

static void system_gclk_chan_enable(
		const uint8_t channel)
{
	/* Select the requested generator channel */
	*((uint8_t*)&GCLK->CLKCTRL.reg) = channel;

	/* Enable the generic clock */
	GCLK->CLKCTRL.reg |= GCLK_CLKCTRL_CLKEN;
}

static void system_gclk_chan_set_config(
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

static inline bool adc_is_syncing()
{
	Adc *const adc_module = ADC;

	if (adc_module->STATUS.reg & ADC_STATUS_SYNCBUSY) {
		return true;
	}

	return false;
}

static void _adc_set_config()
{
	/* Get the hardware module pointer */
	Adc *const adc_module = ADC;

	/* Configure GCLK channel and enable clock */
	struct system_gclk_chan_config gclk_chan_conf;
	system_gclk_chan_get_config_defaults(&gclk_chan_conf);
	gclk_chan_conf.source_generator = GCLOCK_GENERATOR_0;
	system_gclk_chan_set_config(ADC_GCLK_ID, &gclk_chan_conf);
	system_gclk_chan_enable(ADC_GCLK_ID);

	pinmux_set_mux(PINMUX_PA03B_DAC_VREFP);
	pinmux_set_mux(PIN_PA05B_ADC_AIN5); // pos
	pinmux_set_mux(PIN_PA04B_ADC_AIN4); // neg
	// _adc_configure_ain_pin(config->positive_input);
	// _adc_configure_ain_pin(config->negative_input);

	// disabled, no standby, no reset
	adc_module->CTRLA.reg = 0;

	// no ref compensation, ref AREFA
	adc_module->REFCTRL.reg = 0x3;

	// 256 avg samples
	adc_module->AVGCTRL.reg = (0x4 << 4) | 0x8;

	// no aditional sample time
	adc_module->SAMPCTRL.reg = 30;

	while (adc_is_syncing()) {
	}

	/* Configure CTRLB */
	adc_module->CTRLB.reg =
			0x0 | /* /4 prescaler */
			0x10 | /* 16bit */
			0x1 /* differential */;

	while (adc_is_syncing()) {
	}

	// No windowing
	adc_module->WINCTRL.reg = 0x0;

	while (adc_is_syncing()) {
		/* Wait for synchronization */
	}

	/* Configure pin scan mode and positive and negative input pins */
	adc_module->INPUTCTRL.reg =
			0x0| // No gain
			(0x5 << 8) |
			(0x4);

	// no events
	adc_module->EVCTRL.reg = 0x0;

	// No interrupts
	adc_module->INTENCLR.reg = 0x0;

	return;
}

static void adc_init() {
	if (adc_inited)
		return;

	printf("Initialize adc...\r\n");

	Adc *const hw = ADC;

	/* Turn on the digital interface clock */
	PM->APBCMASK.reg |= PM_APBCMASK_ADC;

	if (hw->CTRLA.reg & ADC_CTRLA_SWRST) {
		/* We are in the middle of a reset. Abort. */
		return;
	}

	if (hw->CTRLA.reg & ADC_CTRLA_ENABLE) {
		/* Module must be disabled before initialization. Abort. */
		return;
	}

	/* Write configuration to module */
	_adc_set_config();

	while (adc_is_syncing())
		;

	hw->CTRLA.reg |= ADC_CTRLA_ENABLE;

	adc_inited = true;
}

static uint16_t adc_read() {
	Adc *const hw = ADC;

	while (adc_is_syncing())
		;

	// Start
	hw->SWTRIG.reg |= ADC_SWTRIG_START;

	// Wait
	while (!(hw->INTFLAG.reg & ADC_INTFLAG_RESRDY))
		printf("Wait\r\n");

	while (adc_is_syncing())
		;

	uint16_t result = hw->RESULT.reg;
	printf("Result %x\r\n", result);

	hw->INTFLAG.reg = ADC_INTFLAG_RESRDY;

	return result;
}

int adc_test_cmd(int argc, const char** argv)
{
	adc_init();

	uint16_t value = adc_read();

	printf("Read %d\r\n", value);

	uint32_t res = 4700 * value/(0x8000 - value);

	printf("Res: %d\r\n", res);

	double r = res/1000 - 1;
	double t = (r*(255.8723+r*(9.6+r*0.878)));

	printf("Temp %d\r\n", (uint32_t)(t * 100));
	
	return 0;
}

int adc_dump_cmd(int argc, const char** argv)
{
	Adc *const hw = ADC;

	printf("CTRLA %x\r\n", hw->CTRLA.reg);
	printf("Ref %x\r\n", hw->REFCTRL.reg);
	printf("Avg %x\r\n", hw->AVGCTRL.reg);
	printf("CTRLB %x\r\n", hw->CTRLB.reg);
	printf("Input %x\r\n", hw->INPUTCTRL.reg);
	
	return 0;
}

#else

int adc_test_cmd(int argc, const char** argv)
{
	return -1;
}

int adc_dump_cmd(int argc, const char** argv)
{
	return -1;
}

#endif
