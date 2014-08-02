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

#include <can.h>

#include <irq.h>
#include <runtime.h>
#include <console.h>
#include <clock.h>
#include <scheduler.h>

#include "LPC11xx.h"
#include "system_LPC11xx.h"

typedef struct {
  uint32_t  mode_id;
  uint32_t  mask;
  uint8_t   data[8];
  uint8_t   dlc;
  uint8_t   msgobj;
} can_rom_msg_t;

typedef struct _CAN_ODCONSTENTRY {
  uint16_t index;
  uint8_t  subindex;
  uint8_t  len;
  uint32_t val;
}CAN_ODCONSTENTRY;

typedef struct _CAN_ODENTRY {
  uint16_t index;
  uint8_t  subindex;
  uint8_t  entrytype_len;
  uint8_t  *val;
}CAN_ODENTRY;

typedef struct _CAN_CANOPENCFG {
  uint8_t   node_id;
  uint8_t   msgobj_rx;
  uint8_t   msgobj_tx;
  uint8_t   isr_handled;
  uint32_t  od_const_num;
  CAN_ODCONSTENTRY *od_const_table;
  uint32_t  od_num;
  CAN_ODENTRY *od_table;
}CAN_CANOPENCFG;

typedef	struct {
  void (*rx)(uint8_t msg_obj_num);
  void (*tx)(uint8_t msg_obj_num);
  void (*error)(uint32_t error_info);
  uint32_t (*sdo_read)(uint16_t index, uint8_t subindex);
  uint32_t (*sdo_write)(uint16_t index, uint8_t subindex, uint8_t *dat_ptr);
  uint32_t (*sdo_seg_read)(uint16_t index, uint8_t subindex, uint8_t openclose, uint8_t *length, uint8_t *data, uint8_t *last);
  uint32_t (*sdo_seg_write)(uint16_t index, uint8_t subindex, uint8_t openclose, uint8_t length, uint8_t *data, uint8_t *fast_resp);
  uint8_t (*sdo_req)(uint8_t length_req, uint8_t *req_ptr, uint8_t *length_resp, uint8_t *resp_ptr);
} can_rom_callbacks_t;

struct _can_rom_driver_t{
  void (*init)(uint32_t * can_cfg, uint8_t isr_ena);
  void (*isr)(void);

  void (*config_rxmsgobj)(can_rom_msg_t* msg_obj);
  uint8_t (*can_receive)(can_rom_msg_t* msg_obj);
  void (*can_transmit)(can_rom_msg_t* msg_obj);
  void (*config_canopen)(CAN_CANOPENCFG * canopen_cfg);
  void (*canopen_handler)(void);
  void (*config_calb)(const can_rom_callbacks_t * callback_cfg);
};

typedef struct _can_rom_driver_t* can_rom_driver_t;


struct _can {
	uint8_t addr;
	can_rom_msg_t bind;
	can_rom_msg_t send;
};

static struct _can can;

struct _rom_t {
	void* unused1;
	void* unused2;
	can_rom_driver_t can;
};

static can_rom_driver_t can_rom_driver;

static void can_rom_callback_rx(uint8_t msg_obj_num)
{
	printf("rx callback\r\n");

	can_rom_msg_t msg;

	msg.msgobj = msg_obj_num;
	can_rom_driver->can_receive(&msg);

	  char str[] = {msg.data[0], msg.data[1], msg.data[2], msg.data[3], msg.data[4], msg.data[5], msg.data[6],  msg.data[7], '\0'};
	  printf("Got %s\r\n", str);
}

static void can_rom_callback_tx(uint8_t msg_obj_num)
{
	printf("tx callback\r\n");
}

#define CAN_ERROR_NONE 0x00000000UL
#define CAN_ERROR_PASS 0x00000001UL 
#define CAN_ERROR_WARN 0x00000002UL 
#define CAN_ERROR_BOFF 0x00000004UL 
#define CAN_ERROR_STUF 0x00000008UL 
#define CAN_ERROR_FORM 0x00000010UL 
#define CAN_ERROR_ACK 0x00000020UL 
#define CAN_ERROR_BIT1 0x00000040UL 
#define CAN_ERROR_BIT0 0x00000080UL 
#define CAN_ERROR_CRC 0x00000100UL

static void can_rom_callback_error(uint32_t error_info)
{
	printf("CAN ERROR %x", error_info);

	if (error_info & CAN_ERROR_PASS)
		printf("  [pass]");
	if (error_info & CAN_ERROR_WARN)
		printf("  [warn]");
	if (error_info & CAN_ERROR_BOFF)
		printf("  [boff]");
	if (error_info & CAN_ERROR_STUF)
		printf("  [stuf]");
	if (error_info & CAN_ERROR_FORM)
		printf("  [form]");
	if (error_info & CAN_ERROR_ACK)
		printf("  [ack]");
	if (error_info & CAN_ERROR_BIT1)
		printf("  [bit1]");
	if (error_info & CAN_ERROR_BIT0)
		printf("  [bit0]");
	if (error_info & CAN_ERROR_CRC)
		printf("  [crc]");
	printf("\r\n");
}

static const can_rom_callbacks_t can_rom_callbacks = {
	.rx = can_rom_callback_rx,
	.tx = can_rom_callback_tx,
	.error = can_rom_callback_error,
	.sdo_read = NULL,
  	.sdo_write = NULL,
  	.sdo_seg_read = NULL,
  	.sdo_seg_write = NULL,
  	.sdo_req = NULL
};

static void isr(void) {
	can_rom_driver->isr();
}

struct can_speed_table_entry {
	herz_t clock;
	can_speed_t can;
	uint32_t div;
	uint32_t btr;
};

static const struct can_speed_table_entry can_speed_table[] = {
	{.clock = 12000000, .can = 125000, .div = 0, .btr = 0x1c05},
};

bool can_init(can_speed_t speed)
{
	printf("Clock %d", clock_get_main());

	const struct can_speed_table_entry* entry = NULL;

	herz_t clock_speed = clock_get_main();

	for (uint8_t i = 0, size = sizeof(can_speed_table)/sizeof(struct can_speed_table_entry); i < size; i++) {
		if (can_speed_table[i].clock == clock_speed &&
			can_speed_table[i].can == speed) {
			entry = &can_speed_table[i];
			break;
		}
	}

	assert(entry, "Could not find a can speed entry for clock and speed requested");

	uint32_t ClkInitTable[2] = {
	  entry->div,
	  entry->btr
	};

	if (!irq_register(IRQ13, isr)) {
		assert(false, "Could not register can irq");
		return false;
	}

	struct _rom_t** rom = (struct _rom_t**)0x1fff1ff8;

	can_rom_driver = (*rom)->can;

	can_rom_driver->init(ClkInitTable, 1);
	can_rom_driver->config_calb(&can_rom_callbacks);

	// LPC_CAN->CNTL |= (1<<7);
	// LPC_CAN->TEST |= (1<<4);

	irq_enable(IRQ13);

	return true;
}

void can_bind(uint8_t addr)
{
	can.addr = addr;

	can.bind.msgobj = 1;
	can.bind.mode_id = 0x400;
	can.bind.mask = 0x700;
	can_rom_driver->config_rxmsgobj(&can.bind);	
}

void can_send()
{
	can_rom_msg_t send;
	send.msgobj  = 0;
	send.mode_id = 0x400+0x20;
	send.mask    = 0x0;
	send.dlc     = 4;
	send.data[0] = 'T';	//0x54
	send.data[1] = 'E';	//0x45
	send.data[2] = 'S';	//0x53
	send.data[3] = 'T';

	can_rom_driver->can_transmit(&send);
}
