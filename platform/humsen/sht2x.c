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
static const uint8_t kSHT2xTrigTMeasureHold = 0xE3;
static const uint8_t kSHT2xTrigRHMeasureHold = 0xE5;
static const uint8_t kSHT2xReadSerialNumber1[] = {0xFA, 0x0F};
static const uint8_t kSHT2xReadSerialNumber2[] = {0xFC, 0xC9};
static const uint8_t kSHT2xReadConfig = 0xE6;
static const uint8_t kSHT2xWriteConfig = 0xE7;
static const uint8_t kSHT2xSoftReset = 0xFE;

struct sht2x {
	i2c_dev_t i2c;

	uint32_t padding;
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

int32_t sht2x_measure_temperature(sht2x_t dev)
{
	uint8_t buf[3];

	if (!i2c_dev_transfer(dev->i2c, kSHT2xAddress, &kSHT2xTrigTMeasureHold, 1, buf, 3)) {
		return -1;
	}

	int16_t raw = (buf[0] << 8) | buf[1];

	return ((21965 * raw) >> 13) - 46850;
}

int32_t sht2x_measure_humidity(sht2x_t dev)
{
	uint8_t buf[3];

	if (!i2c_dev_transfer(dev->i2c, kSHT2xAddress, &kSHT2xTrigRHMeasureHold, 1, buf, 3)) {
		return -1;
	}

	int16_t raw = (buf[0] << 8) | buf[1];

	return ((15625 * raw) >> 13) - 6000;
}

uint64_t sht2x_read_serial_number(sht2x_t dev)
{
	uint8_t buf1[8];

	if (!i2c_dev_transfer(dev->i2c, kSHT2xAddress, kSHT2xReadSerialNumber1, 2, buf1, 8)) {
		return -1;
	}

	uint8_t buf2[6];

	if (!i2c_dev_transfer(dev->i2c, kSHT2xAddress, kSHT2xReadSerialNumber2, 2, buf2, 6)) {
		return -1;
	}

	return (uint64_t)buf2[3] << 56 | (uint64_t)buf2[4] << 48 | (uint64_t)buf1[0] << 40 
			| (uint64_t)buf1[2] << 32 | (uint64_t)buf1[4] << 24 | (uint64_t)buf1[6] << 16
			| (uint64_t)buf2[0] << 8 | (uint64_t)buf2[1];
}

uint8_t sht2x_read_config(sht2x_t dev)
{
	uint8_t conf;

	i2c_dev_transfer(dev->i2c, kSHT2xAddress, &kSHT2xReadConfig, 1, &conf, 1);

	return conf;
}

void sht2x_write_config(sht2x_t dev, uint8_t conf)
{
	uint8_t buf[2] = {
		kSHT2xWriteConfig,
		conf,
	};

	i2c_dev_transfer(dev->i2c, kSHT2xAddress, buf, 2, NULL, 0);
}

void sht2x_soft_reset(sht2x_t dev)
{
	i2c_dev_transfer(dev->i2c, kSHT2xAddress, &kSHT2xSoftReset, 1, NULL, 0);
}
