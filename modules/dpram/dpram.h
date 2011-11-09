/****************************************************************************

**

** COPYRIGHT(C) : Samsung Electronics Co.Ltd, 2006-2010 ALL RIGHTS RESERVED

**

** AUTHOR       : Kim, Geun-Young <geunyoung.kim@samsung.com>			@LDK@

**                                                                      @LDK@

****************************************************************************/

#ifndef __DPRAM_H__
#define __DPRAM_H__

/* 16KB Size */
#define DPRAM_SIZE									0x4000

/* Memory Address */
#define DPRAM_START_ADDRESS 						0x0000
#define DPRAM_MAGIC_CODE_ADDRESS					(DPRAM_START_ADDRESS)
#define DPRAM_ACCESS_ENABLE_ADDRESS					(DPRAM_START_ADDRESS + 0x0002)

#define DPRAM_PDA2PHONE_FORMATTED_HEAD_ADDRESS		(DPRAM_START_ADDRESS + 0x0004)
#define DPRAM_PDA2PHONE_FORMATTED_TAIL_ADDRESS		(DPRAM_PDA2PHONE_FORMATTED_HEAD_ADDRESS + 0x0002)
#define DPRAM_PDA2PHONE_FORMATTED_BUFFER_ADDRESS	(DPRAM_PDA2PHONE_FORMATTED_HEAD_ADDRESS + 0x0004)
#define DPRAM_PDA2PHONE_FORMATTED_SIZE				1020	/* 0x03FC */

#define DPRAM_PDA2PHONE_RAW_HEAD_ADDRESS			(DPRAM_START_ADDRESS + 0x0404)
#define DPRAM_PDA2PHONE_RAW_TAIL_ADDRESS			(DPRAM_PDA2PHONE_RAW_HEAD_ADDRESS + 0x0002)
#define DPRAM_PDA2PHONE_RAW_BUFFER_ADDRESS			(DPRAM_PDA2PHONE_RAW_HEAD_ADDRESS + 0x0004)
#define DPRAM_PDA2PHONE_RAW_SIZE					7152	/*1BF0*/

#define DPRAM_PHONE2PDA_FORMATTED_HEAD_ADDRESS		(DPRAM_START_ADDRESS + 0x1FF8)
#define DPRAM_PHONE2PDA_FORMATTED_TAIL_ADDRESS		(DPRAM_PHONE2PDA_FORMATTED_HEAD_ADDRESS + 0x0002)
#define DPRAM_PHONE2PDA_FORMATTED_BUFFER_ADDRESS	(DPRAM_PHONE2PDA_FORMATTED_HEAD_ADDRESS + 0x0004)
#define DPRAM_PHONE2PDA_FORMATTED_SIZE				1020	/* 0x03FC */

#define DPRAM_PHONE2PDA_RAW_HEAD_ADDRESS			(DPRAM_START_ADDRESS + 0x23F8)
#define DPRAM_PHONE2PDA_RAW_TAIL_ADDRESS			(DPRAM_PHONE2PDA_RAW_HEAD_ADDRESS + 0x0002)
#define DPRAM_PHONE2PDA_RAW_BUFFER_ADDRESS			(DPRAM_PHONE2PDA_RAW_HEAD_ADDRESS + 0x0004)
#define DPRAM_PHONE2PDA_RAW_SIZE					7152	/* 1BF0 */

/* indicator area*/
#define DPRAM_INDICATOR_START						(DPRAM_START_ADDRESS + 0x3FEC)
#define DPRAM_INDICATOR_SIZE						16
#define DPRAM_SILENT_RESET					(DPRAM_INDICATOR_START + 0x0004)	

#define DPRAM_PDA2PHONE_INTERRUPT_ADDRESS			(DPRAM_START_ADDRESS + 0x3FFE)
#define DPRAM_PHONE2PDA_INTERRUPT_ADDRESS			(DPRAM_START_ADDRESS + 0x3FFC)
#define DPRAM_INTERRUPT_PORT_SIZE					2


/*
 * interrupt masks.
 */
#define INT_MASK_VALID					0x0080
#define INT_MASK_COMMAND				0x0040
#define INT_MASK_REQ_ACK_F				0x0020
#define INT_MASK_REQ_ACK_R				0x0010
#define INT_MASK_RES_ACK_F				0x0008
#define INT_MASK_RES_ACK_R				0x0004
#define INT_MASK_SEND_F					0x0002
#define INT_MASK_SEND_R					0x0001

#define INT_MASK_CMD_INIT_START			0x0001
#define INT_MASK_CMD_INIT_END			0x0002
#define INT_MASK_CMD_REQ_ACTIVE			0x0003
#define INT_MASK_CMD_RES_ACTIVE			0x0004
#define INT_MASK_CMD_REQ_TIME_SYNC		0x0005
#define INT_MASK_CMD_PHONE_START 		0x0008
#define INT_MASK_CMD_ERR_DISPLAY		0x0009
#define INT_MASK_CMD_PHONE_DEEP_SLEEP	0x000A
#define INT_MASK_CMD_NV_REBUILDING		0x000B
#define INT_MASK_CMD_EMER_DOWN			0x000C

#define INT_COMMAND(x)					(INT_MASK_VALID | INT_MASK_COMMAND | x)
#define INT_NON_COMMAND(x)				(INT_MASK_VALID | x)

#define FORMATTED_INDEX					0
#define RAW_INDEX						1
#define MAX_INDEX						2

/* ioctl command definitions. */
#define IOC_MZ_MAGIC				('h')
#define HN_DPRAM_PHONE_ON			_IO(IOC_MZ_MAGIC, 0xd0)
#define HN_DPRAM_PHONE_OFF			_IO(IOC_MZ_MAGIC, 0xd1)
#define HN_DPRAM_PHONE_GETSTATUS	_IOR(IOC_MZ_MAGIC, 0xd2, unsigned int)
#define HN_DPRAM_PHONE_RESET		_IO(IOC_MZ_MAGIC, 0xd3)
#define HN_DPRAM_PHONE_RAMDUMP		_IO(IOC_MZ_MAGIC, 0xd4)
#define HN_DPRAM_PHONE_BOOTTYPE		_IOW(IOC_MZ_MAGIC, 0xd5, unsigned int)
#define HN_DPRAM_MEM_RW				_IOWR(IOC_MZ_MAGIC, 0xd6, unsigned long)
#define HN_DPRAM_WAKEUP		_IOW(IOC_MZ_MAGIC, 0xd7, unsigned int)
#define HN_DPRAM_SILENT_RESET		_IO(IOC_MZ_MAGIC, 0xd8)

#define MESG_PHONE_OFF		1
#define MESG_PHONE_RESET		2

/*
 * structure definitions.
 */
typedef struct dpram_serial {
	/* pointer to the tty for this device */
	struct tty_struct *tty;

	/* number of times this port has been opened */
	int open_count;

	/* locks this structure */
	struct semaphore sem;
} dpram_serial_t;

typedef struct dpram_device {
	/* DPRAM memory addresses */
	unsigned long in_head_addr;
	unsigned long in_tail_addr;
	unsigned long in_buff_addr;
	unsigned long in_buff_size;

	unsigned long out_head_addr;
	unsigned long out_tail_addr;
	unsigned long out_buff_addr;
	unsigned long out_buff_size;

	u_int16_t mask_req_ack;
	u_int16_t mask_res_ack;
	u_int16_t mask_send;

	dpram_serial_t serial;
} dpram_device_t;

typedef struct dpram_tasklet_data {
	dpram_device_t *device;
	u_int16_t non_cmd;
} dpram_tasklet_data_t;

#ifdef CONFIG_EVENT_LOGGING
typedef struct event_header {
	struct timeval timeVal;
	__u16 class;
	__u16 repeat_count;
	__s32 payload_length;
} EVENT_HEADER;
#endif

struct _mem_param {
	unsigned short addr;
	unsigned long data;
	int dir;
};

/* TODO: add more definitions */

#endif	/* __DPRAM_H__ */

