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

#include <sht2x.h>

#include <malloc.h>

static const uint8_t kSHT2xAddress = 0x80;
static const uint8_t kSHT2x_TRIG_T_MEASURE_HOLD = 0xE3;
static const uint8_t kSHT2x_TRIG_RH_MEASURE_HOLD = 0xE5;

struct sht2x {
	i2c_dev_t i2c;
};

// No need for locking the device. We only do one transfer which in itself
// is already atomic, so we don't need to ensure that ourself
sht2x_t sht2x_create(i2c_dev_t bus)
{
	sht2x_t dev = malloc_raw(sizeof(struct sht2x));

	if (!dev) {
		return NULL;
	}

	dev->i2c = bus;

	return dev;
}

int16_t sht2x_measure_temperature(sht2x_t dev)
{
	uint8_t buf[3];

	if (!i2c_dev_transfer(dev->i2c, kSHT2xAddress, &kSHT2x_TRIG_T_MEASURE_HOLD, 1, buf, 3)) {
		return -1;
	}

	int16_t raw = (buf[0] << 8) | buf[1];

	return ((21965 * raw) >> 13) - 46850;
}

int16_t sht2x_measure_humidity(sht2x_t dev)
{
	uint8_t buf[3];

	if (!i2c_dev_transfer(dev->i2c, kSHT2xAddress, &kSHT2x_TRIG_RH_MEASURE_HOLD, 1, buf, 3)) {
		return -1;
	}

	int16_t raw = (buf[0] << 8) | buf[1];

	return ((15625 * raw) >> 13) - 6000;
}
