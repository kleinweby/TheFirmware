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
#include <semaphore.h>
#include <string.h>

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

struct _lpc11_can_receive_conf {
	can_receive_callback_t callback;
	void* context;
};

struct _can {
	// LPC11 supports 32 message objects, msg object with id 0
	// is reserved for sending.
	struct _lpc11_can_receive_conf receive_conf[31];

	struct semaphore send_lock;
	struct semaphore send_done;
	bool send_async;
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
	can_rom_msg_t msg;

	msg.msgobj = msg_obj_num;
	can_rom_driver->can_receive(&msg);

	struct _lpc11_can_receive_conf* conf = &can.receive_conf[msg_obj_num - 1];

	if (conf->callback) {
		can_frame_t frame;

		frame.id = msg.mode_id;
		frame.data_length = msg.dlc;
		memcpy(frame.data, msg.data, frame.data_length);
		conf->callback(frame, conf->context);
	}
}

static void can_rom_callback_tx(uint8_t msg_obj_num)
{
	assert(msg_obj_num == 0, "Wrong tx callback?!");

	semaphore_signal(&can.send_done);
	if (can.send_async) {
		semaphore_signal(&can.send_lock);
	}
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

// Calculated via http://www.bittiming.can-wiki.info/#C_CAN
// BTR usually is correct, but DIV is always wrong so those
// have been solved by try and error
static const struct can_speed_table_entry can_speed_table[] = {
	{.clock = 12000000, .can = 50000, .div = 2/*15*/, .btr = 0x1c0e},
	// Calculated using 6mhz and adjusted div by -1
	{.clock = 12000000, .can = 125000, .div = 2, .btr = 0x1c02},

	// Adjust div by -7
	{.clock = 16000000, .can = 125000, .div = 8-7, .btr = 0x1c07},
};

status_t can_init(can_speed_t speed)
{
	if (!irq_register(IRQ13, isr)) {
		assert(false, "Could not register can irq");
		return false;
	}

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
	  entry->div - 1, // There is no divider of zero
	  entry->btr
	};

	struct _rom_t** rom = (struct _rom_t**)0x1fff1ff8;

	can_rom_driver = (*rom)->can;

	can_rom_driver->init(ClkInitTable, 1);
	can_rom_driver->config_calb(&can_rom_callbacks);

	// printf("Test mode\r\n");
	// LPC_CAN->CNTL |= (1<<7);
	// LPC_CAN->TEST |= (1<<4);

	semaphore_init(&can.send_lock, 1);
	semaphore_init(&can.send_done, 0);

	irq_enable(IRQ13);

	return STATUS_OK;
}

void can_reset(can_speed_t speed)
{
	unimplemented();
}

status_t can_send(const can_frame_t frame, can_flags_t flags)
{
	semaphore_wait(&can.send_lock);

	can_rom_msg_t send;
	send.msgobj  = 0;
	send.mode_id = frame.id;
	send.mask    = 0x0;
	send.dlc     = frame.data_length;
	memcpy(&send.data, &frame.data, 8);

	if (flags & CAN_FLAG_NOWAIT) {
		can.send_async = true;
		can_rom_driver->can_transmit(&send);

		return STATUS_OK;
	}
	else {
		can.send_async = false;
		can_rom_driver->can_transmit(&send);
		semaphore_wait(&can.send_done);
		semaphore_signal(&can.send_lock);

		return STATUS_OK;
	}
}

status_t can_set_receive_callback(can_id_t id, can_id_t id_mask, can_receive_callback_t callback, void* context)
{
	int32_t idx = -1;

	for (int i = 0; i < 31; ++i) {
		if (can.receive_conf[i].callback == NULL) {
			idx = i;
			break;
		}
	}

	if (idx < 0) {
		return STATUS_ERR(0);
	}

	can.receive_conf[idx].callback = callback;
	can.receive_conf[idx].context = context;

	can_rom_msg_t recv;
	recv.msgobj = idx + 1;
	recv.mode_id = id;
	recv.mask = id_mask;

	can_rom_driver->config_rxmsgobj(&recv);	

	return STATUS_OK;
}

status_t can_unset_receive_callback(can_id_t id, can_id_t id_mask, can_receive_callback_t callback, void* context)
{
	// TODO: this is not entierly correct
	// and does not reset the hardware filter (seems like an impossible thing?)
	for (int i = 0; i < 31; ++i) {
		if (can.receive_conf[i].callback == callback && can.receive_conf[i].context == context) {
			can.receive_conf[i].callback = NULL;
			can.receive_conf[i].context = NULL;
			return STATUS_OK;
		}
	}

	return STATUS_ERR(0);
}
