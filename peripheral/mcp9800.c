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

#include<mcp9800.h>
#include<malloc.h>

struct mcp9800 {
	i2c_dev_t i2c;

	uint8_t config;
};

static const uint8_t kMCP9800Address = 0x90;
static const uint8_t kMCP9800ReadConfigCMD = 0x1;
static const uint8_t kMCP9800WriteConfigCMD = 0x1;

mcp9800_t mcp9800_create(i2c_dev_t i2c)
{
	mcp9800_t dev = malloc_raw(sizeof(struct mcp9800));

	if (!dev) {
		return NULL;
	}

	dev->i2c = i2c;
	dev->config = 0;

	return dev;
}

status_t mcp9800_read_config(mcp9800_t dev)
{
	uint8_t config;

	if (i2c_dev_transfer(dev->i2c, kMCP9800Address, &kMCP9800ReadConfigCMD, 1, &config, 1) != STATUS_OK) {
		return STATUS_ERR(0);
	}

	dev->config = config;

	return STATUS_OK;
}

status_t mcp9800_write_config(mcp9800_t dev)
{
	uint8_t buf[2] = {
		kMCP9800WriteConfigCMD,
		dev->config,
	};

	if (i2c_dev_transfer(dev->i2c, kMCP9800Address, buf, 2, NULL, 0) != STATUS_OK) {
		return STATUS_ERR(0);
	}

	return STATUS_OK;
}

uint16_t mcp9800_read_temperature(mcp9800_t dev);
