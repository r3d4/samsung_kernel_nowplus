
#define _DEBUG
#define _ENABLE_ERROR_DEVICE
//#define _DPRAM_DEBUG_HEXDUMP
#define _ENABLE_SMS_FIX     // enable workaround for SMS multiple receive bug
#define _HSDPA_DPRAM

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/mm.h>

#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/tty_flip.h>
#include <linux/irq.h>
#include <linux/rtc.h>
#include <linux/wakelock.h>

#ifdef _ENABLE_ERROR_DEVICE
#include <linux/poll.h>
#include <linux/cdev.h>
#endif

#include <asm/irq.h>
#include <mach/hardware.h>
#include <asm/uaccess.h>
//#include <asm/arch/mux.h>
//#include <asm/arch/gpio.h>

#ifdef CONFIG_EVENT_LOGGING
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/klog.h>
#include <asm/unistd.h>
#endif

#ifdef CONFIG_PROC_FS
#include <linux/proc_fs.h>
#endif

#include <plat/mux.h>

#include "dpram.h"


#define DRIVER_ID			"[DPRAM] dpram driver is loaded."
#define DRIVER_NAME 		"DPRAM"
#define DRIVER_PROC_ENTRY	"driver/dpram"
#define DRIVER_MAJOR_NUM	255


#ifdef CONFIG_EVENT_LOGGING
#define DPRAM_ID			3
#define DPRAM_READ			3
#define DPRAM_WRITE			4
#endif

#ifdef _DEBUG
#define dprintk(s, args...) printk("[DPRAM] %s:%d - " s, __func__, __LINE__,  ##args)
#else
#define dprintk(s, args...)
#endif

#define WRITE_TO_DPRAM(dest, src, size) \
	_memcpy((void *)(DPRAM_VBASE + dest), src, size)

#define READ_FROM_DPRAM(dest, src, size) \
	_memcpy(dest, (void *)(DPRAM_VBASE + src), size)

#ifdef _ENABLE_ERROR_DEVICE
#define DPRAM_ERR_MSG_LEN			65
#define DPRAM_ERR_DEVICE			"dpramerr"
#endif

#define DPRAM_WAKELOCK_TIMEOUT	HZ

#define MAX_PDP_DATA_LEN		1500

#define MAX_PDP_PACKET_LEN		(MAX_PDP_DATA_LEN + 4 + 2)

#define MAX_RETRY_WRITE_COUNT		900

#include <mach/gpio.h>

#define GPIO_LEVEL_LOW				0
#define GPIO_LEVEL_HIGH				1


static void __iomem *dpram_base = 0;

static int phone_shutdown = 0;

static int dpram_phone_getstatus(void);
#define DPRAM_VBASE dpram_base

static struct tty_driver *dpram_tty_driver;
static dpram_tasklet_data_t dpram_tasklet_data[MAX_INDEX];
static dpram_device_t dpram_table[MAX_INDEX] = {
	{
		.in_head_addr = DPRAM_PHONE2PDA_FORMATTED_HEAD_ADDRESS,
		.in_tail_addr = DPRAM_PHONE2PDA_FORMATTED_TAIL_ADDRESS,
		.in_buff_addr = DPRAM_PHONE2PDA_FORMATTED_BUFFER_ADDRESS,
		.in_buff_size = DPRAM_PHONE2PDA_FORMATTED_SIZE,

		.out_head_addr = DPRAM_PDA2PHONE_FORMATTED_HEAD_ADDRESS,
		.out_tail_addr = DPRAM_PDA2PHONE_FORMATTED_TAIL_ADDRESS,
		.out_buff_addr = DPRAM_PDA2PHONE_FORMATTED_BUFFER_ADDRESS,
		.out_buff_size = DPRAM_PDA2PHONE_FORMATTED_SIZE,

		.mask_req_ack = INT_MASK_REQ_ACK_F,
		.mask_res_ack = INT_MASK_RES_ACK_F,
		.mask_send = INT_MASK_SEND_F,
	},
	{
		.in_head_addr = DPRAM_PHONE2PDA_RAW_HEAD_ADDRESS,
		.in_tail_addr = DPRAM_PHONE2PDA_RAW_TAIL_ADDRESS,
		.in_buff_addr = DPRAM_PHONE2PDA_RAW_BUFFER_ADDRESS,
		.in_buff_size = DPRAM_PHONE2PDA_RAW_SIZE,

		.out_head_addr = DPRAM_PDA2PHONE_RAW_HEAD_ADDRESS,
		.out_tail_addr = DPRAM_PDA2PHONE_RAW_TAIL_ADDRESS,
		.out_buff_addr = DPRAM_PDA2PHONE_RAW_BUFFER_ADDRESS,
		.out_buff_size = DPRAM_PDA2PHONE_RAW_SIZE,

		.mask_req_ack = INT_MASK_REQ_ACK_R,
		.mask_res_ack = INT_MASK_RES_ACK_R,
		.mask_send = INT_MASK_SEND_R,
	},
};

static struct tty_struct *dpram_tty[MAX_INDEX];
static struct ktermios *dpram_termios[MAX_INDEX];
static struct ktermios *dpram_termios_locked[MAX_INDEX];
unsigned int phone_boottype = 0;
unsigned int dpram_silent_reset = 0;
int temp_count = 0;
int forced_dpram_wakeup = 1;
unsigned int irq_flag;
int pdp_tx_flag=0;

EXPORT_SYMBOL(pdp_tx_flag);

//extern void enable_dpram_pins(void);
extern void (*__do_forced_modemoff)(void);
static void res_ack_tasklet_handler(unsigned long data);
static void send_tasklet_handler(unsigned long data);

static DECLARE_TASKLET(fmt_send_tasklet, send_tasklet_handler, 0);
static DECLARE_TASKLET(raw_send_tasklet, send_tasklet_handler, 0);

static DECLARE_TASKLET(fmt_res_ack_tasklet, res_ack_tasklet_handler,
		(unsigned long)&dpram_table[FORMATTED_INDEX]);
static DECLARE_TASKLET(raw_res_ack_tasklet, res_ack_tasklet_handler,
		(unsigned long)&dpram_table[RAW_INDEX]);

#define DPRAM_DGS_INFO_BLOCK_SIZE		0x100
static unsigned char aDGSBuf[512];

#ifdef _ENABLE_ERROR_DEVICE
static unsigned int dpram_err_len;
static char dpram_err_buf[DPRAM_ERR_MSG_LEN];

struct class *dpram_class;

static DECLARE_WAIT_QUEUE_HEAD(dpram_err_wait_q);
static struct fasync_struct *dpram_err_async_q;
#endif

static DECLARE_WAIT_QUEUE_HEAD(phone_power_wait);

static DECLARE_MUTEX(write_mutex);

struct wake_lock dpram_wake_lock;


#ifdef _ENABLE_SMS_FIX

#define SMS_BUFFER_MAXLEN  16 // only need to parse header
#define SMS_BUFFER_APPEND  1

// with code from ius libsamsung-ipc
// https://github.com/ius/libsamsung-ipc
#define FRAME_START	0x7f
#define FRAME_END	0x7e

#define IPC_TYPE_EXEC		                        0x01

#define IPC_COMMAND(f)  ((f->group << 8) | f->index)
#define FRAME_GRP(m)	(m >> 8)
#define FRAME_IDX(m)	(m & 0xff)

#define IPC_SMS_INCOMING_MSG            0x0402
#define IPC_SMS_DELIVER_REPORT		    0x0406

#define IPC_SMS_ACK_NO_ERROR            0x0000
#define IPC_SMS_ACK_PDA_FULL_ERROR      0x8080
#define IPC_SMS_ACK_MALFORMED_REQ_ERROR 0x8061
#define IPC_SMS_ACK_UNSPEC_ERROR        0x806F


struct ipc_header {
    unsigned short length;
    unsigned char mseq, aseq;
    unsigned char group, index, type;
} __attribute__((__packed__));

struct hdlc_header {
	unsigned short length;
	unsigned char unknown;
	struct ipc_header ipc;
} __attribute__((__packed__));

struct ipc_sms_incoming_msg {
    unsigned char type, unk, length;
} __attribute__((__packed__));


static unsigned char sms_buffer[SMS_BUFFER_MAXLEN+1];
static int sms_buffer_len=0;
int smsfix_enable = 0;  //enabled by ioctl

#ifdef _DPRAM_DEBUG_HEXDUMP
static void hexdump(const char *buf, int len);
#endif
static int dpram_write(dpram_device_t *device,	const unsigned char *buf, int len);

void sms_ack_send(dpram_device_t *device)
{
    // sms ack data
     unsigned char sms_ack_data[] = { 0x00, 0x00, 0x03, 0x00, 0x02 };
     unsigned char frame[300];  //>247 data +12 header
/*
size:
start+end:  2
hdlc:       3
ipc         7
          -> 12 bytes total
*/
    const int data_length=247;

	struct hdlc_header header;
	unsigned int hdr_len = sizeof(header);
	unsigned int frame_length = (hdr_len + data_length);
	//char *frame = malloc(frame_length);

    memset(&frame, 0, sizeof(frame));

    // build hdlc+ipc header
	memset(&header, 0x00, hdr_len);
	header.length = frame_length;
	header.ipc.mseq = 0; //fixme
	header.ipc.aseq = 0xff; //fixme
	header.ipc.length = (frame_length - 3);
	header.ipc.group = FRAME_GRP(IPC_SMS_DELIVER_REPORT);
	header.ipc.index = FRAME_IDX(IPC_SMS_DELIVER_REPORT);
	header.ipc.type = IPC_TYPE_EXEC;

	memcpy(frame+1, &header, sizeof(header));
	memcpy(frame+hdr_len+1, sms_ack_data, sizeof(sms_ack_data));

    frame[0] = FRAME_START;
    frame[frame_length+1] = FRAME_END;
#ifdef _DPRAM_DEBUG_HEXDUMP
    printk("[DPRAM] SMSFIX: >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
    hexdump(frame, frame_length+2);
    printk("[DPRAM] <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
#endif
	dpram_write(device, frame, frame_length+2); //+2bytes start/end

}


int sms_fix_add_data(unsigned char* buffer, int len, int append)
{
    //printk("[DPRAM] %s\n", __func__);
    if(!append)
    {
        sms_buffer_len = 0;

        if (len < SMS_BUFFER_MAXLEN)
        {
            memcpy(sms_buffer, buffer, len);
            sms_buffer_len = len;
        }
        else
        {
            memcpy(sms_buffer, buffer, SMS_BUFFER_MAXLEN);
            sms_buffer_len = SMS_BUFFER_MAXLEN;
        }
    }
    else
    {
         if (len+sms_buffer_len < SMS_BUFFER_MAXLEN)
        {
            memcpy(sms_buffer+sms_buffer_len, buffer, len);
            sms_buffer_len += len;
        }
        else
        {
            memcpy(sms_buffer+sms_buffer_len, buffer, SMS_BUFFER_MAXLEN-sms_buffer_len);
            sms_buffer_len = SMS_BUFFER_MAXLEN;
        }
    }
    return sms_buffer_len;

}
// 1 byte frame start
// --- hdlc header ---
// 2 bytes  length
// 1 byte   ?
// --- ipc_header ----
int sms_fix_process(dpram_device_t *device)
{
    unsigned char *buffer = sms_buffer;
    int len = sms_buffer_len;
    unsigned short *p_len;
    struct ipc_header* ipc;
    unsigned char *data;

//printk("[DPRAM] %s\n", __func__);

    // only process header
    if ((*buffer == FRAME_START) && (len >= sizeof(struct hdlc_header)))
    {
        p_len = (unsigned short*)&buffer[1];
        data = &buffer[4];
        ipc = (struct ipc_header*)data;

        //printk("[DPRAM] SMSFIX: frame start detected, frame length=%d\n", *p_len);

        // printk("        mseq  = 0x%02x\n"
               // "        aseq  = 0x%02x\n"
               // "        group = 0x%02x\n"
               // "        index = 0x%02x\n"
               // "        type  = 0x%02x\n"
               // "        length= 0x%02x\n",
            // ipc->mseq, ipc->aseq, ipc->group, ipc->index, ipc->type, ipc->length);

        //hack: samsung ril implementation lacks SMS acknowledge (maybe done by baseband itself?)
        //so send SMS acknowledge from here direct after INCOMING SMS
        if(IPC_COMMAND(ipc) == IPC_SMS_INCOMING_MSG)
        {
            printk("[DPRAM] SMSFIX: incoming SMS, send ACK\n");
            // send ACK to baseband
            sms_ack_send(device);
        }
    }

    return 0;
}

#endif



#ifdef CONFIG_EVENT_LOGGING
static inline EVENT_HEADER *getPayloadHeader(int flag, int size)
{
	EVENT_HEADER *header;
	struct timeval time_val;

	header = (EVENT_HEADER *)kmalloc(sizeof (EVENT_HEADER), GFP_ATOMIC);
	do_gettimeofday(&time_val);

	header->timeVal = time_val;
	header->class = (flag == DPRAM_READ ? DPRAM_READ : DPRAM_WRITE);
	header->repeat_count = 0;
	header->payload_length = size;

	return header;
}

static inline void dpram_event_logging(int direction, void *src, int size)
{
	EVENT_HEADER *header;
	unsigned long flags;

	header = getPayloadHeader(direction, size);

	local_irq_save(flags);
	klog(header, sizeof (EVENT_HEADER), DPRAM_ID);

	if (direction == DPRAM_WRITE) {
		klog(src, size, DPRAM_ID);
	}

	else if (direction == DPRAM_READ) {
		klog((void *)(DPRAM_VBASE + src), size, DPRAM_ID);
	}

	local_irq_restore(flags);
	kfree(header);
}
#endif

static inline void byte_align(unsigned long dest, unsigned long src)
{
	u16 *p_src;
	volatile u16 *p_dest;

	if (!(dest % 2) && !(src % 2)) {
		p_dest = (u16 *)dest;
		p_src = (u16 *)src;

		*p_dest = (*p_dest & (u16)0xFF00) | (*p_src & (u16)0x00FF);
	}

	else if ((dest % 2) && (src % 2)) {
		p_dest = (u16 *)(dest - 1);
		p_src = (u16 *)(src - 1);

		*p_dest = (*p_dest & (u16)0x00FF) | (*p_src & (u16)0xFF00);
	}

	else if (!(dest % 2) && (src % 2)) {
		p_dest = (u16 *)dest;
		p_src = (u16 *)(src - 1);

		*p_dest = (u16)((u16)(*p_dest & (u16)0xFF00) | (u16)((*p_src >> 8) & (u16)0x00FF));
	}

	else if ((dest % 2) && !(src % 2)) {
		p_dest = (u16 *)(dest - 1);
		p_src = (u16 *)src;

		*p_dest = (u16)((u16)(*p_dest & (u16)0x00FF) | (u16)((*p_src << 8) & (u16)0xFF00));
	}

	else {
		dprintk("oops.~\n");
	}
}

static inline void _memcpy(void *p_dest, const void *p_src, int size)
{
	unsigned long dest = (unsigned long)p_dest;
	unsigned long src = (unsigned long)p_src;

	int i;

	if(!(size % 2) && !(dest % 2) && !(src % 2)) {
		for(i = 0; i < (size/2); i++)
			*(((u16 *)dest) + i) = *(((u16 *)src) + i);
	}
	else {
		for(i = 0; i < size; i++)
			byte_align(dest+i, src+i);
	}
}

static inline int _memcmp(u16 *dest, u16 *src, int size)
{
	int i = 0;

	size =size>>1;
	while (i< size) {
		if (*(dest + i) != *(src + i)) {
			return 1;
		}
		i++ ;
	}

	return 0;
}

static inline int WRITE_TO_DPRAM_VERIFY(u32 dest, void *src, int size)
{

	int cnt = 3;

	while (cnt--) {
		_memcpy((void *)(DPRAM_VBASE + dest), (void *)src, size);

		if (!_memcmp((u16 *)(DPRAM_VBASE + dest), (u16 *)src, size))
			return 0;
	}

	return -1;

}

static inline int READ_FROM_DPRAM_VERIFY(void *dest, u32 src, int size)
{

	int cnt = 3;

	while (cnt--) {
		_memcpy((void *)dest, (void *)(DPRAM_VBASE + src), size);

		if (!_memcmp((u16 *)dest, (u16 *)(DPRAM_VBASE + src), size))
			return 0;
	}

	return -1;

}

static void send_interrupt_to_phone(u16 irq_mask)
{
	WRITE_TO_DPRAM(DPRAM_PDA2PHONE_INTERRUPT_ADDRESS, &irq_mask,
			DPRAM_INTERRUPT_PORT_SIZE);
}

#ifdef _DPRAM_DEBUG_HEXDUMP
#define isprint(c)	((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
static void hexdump(const char *buf, int len)
{
	char str[80], octet[10];
	int ofs, i, l;

	for (ofs = 0; ofs < len; ofs += 16) {
		sprintf( str, "%03d: ", ofs );

		for (i = 0; i < 16; i++) {
			if ((i + ofs) < len)
				sprintf( octet, "%02x ", buf[ofs + i] );
			else
				strcpy( octet, "   " );

			strcat( str, octet );
		}
			strcat( str, "  " );
			l = strlen( str );

		for (i = 0; (i < 16) && ((i + ofs) < len); i++)
			str[l++] = isprint( buf[ofs + i] ) ? buf[ofs + i] : '.';

		str[l] = '\0';
		printk( "%s\n", str );
	}
}

static void print_debug_current_time(void)
{
	struct rtc_time tm;
	struct timeval time;

	do_gettimeofday(&time);

	rtc_time_to_tm(time.tv_sec, &tm);

	printk(KERN_INFO"Kernel Current Time info - %02d%02d%02d%02d%02d.%ld \n", tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, time.tv_usec);

}
#endif

static int dpram_write(dpram_device_t *device,
		const unsigned char *buf, int len)
{
	int retval = 0;
	int size = 0;
	int count = 0;

	u16 freesize = 0;
	u16 next_head = 0;

	u16 head, tail;
	u16 irq_mask = 0;

	down(&write_mutex);

#ifdef _DPRAM_DEBUG_HEXDUMP
	printk("\n\n#######[dpram write start: head - %04d, tail - %04d]. %s ######\n", head, tail, device->serial.tty->name);
	hexdump(buf, len);
#endif

	while(1) {
		READ_FROM_DPRAM_VERIFY(&head, device->out_head_addr, sizeof(head));
		READ_FROM_DPRAM_VERIFY(&tail, device->out_tail_addr, sizeof(tail));

		if(head < tail)
			freesize = tail - head - 1;
		else
			freesize = device->out_buff_size - head + tail -1;

		if(freesize >= len){

			next_head = head + len;

			if(next_head < device->out_buff_size) {
				size = len;
				WRITE_TO_DPRAM(device->out_buff_addr + head, buf, size);
				retval = size;
			}
			else {
				next_head -= device->out_buff_size;

				size = device->out_buff_size - head;
				WRITE_TO_DPRAM(device->out_buff_addr + head, buf, size);
				retval = size;

				size = next_head;
				WRITE_TO_DPRAM(device->out_buff_addr, buf + retval, size);
				retval += size;
			}

			head = next_head;

			WRITE_TO_DPRAM_VERIFY(device->out_head_addr, &head, sizeof(head));

		}
		else {
			count++;
			if(count > MAX_RETRY_WRITE_COUNT){
				retval = -EAGAIN;
				break;
			}
		}

		irq_mask = INT_MASK_VALID;

		if (retval > 0)
			irq_mask |= INT_MASK_SEND_R;

		if (len > retval)
			irq_mask |= INT_MASK_REQ_ACK_R;

		if(retval >= len) {
			send_interrupt_to_phone(irq_mask);
			break;
		}

	}

#ifdef _DPRAM_DEBUG_HEXDUMP
	printk("#######[dpram write end: head - %04d, tail - %04d], %s ######\n\n", head, tail, device->serial.tty->name);
	print_debug_current_time();
#endif

	up(&write_mutex);

	return retval;

}

static inline
int dpram_tty_insert_data(dpram_device_t *device, const u8 *psrc, u16 size)
{
#define CLUSTER_SEGMENT	1500

	u16 copied_size = 0;
	int retval = 0;

	if (size > CLUSTER_SEGMENT && (device->serial.tty->index == 1)) {
		while (size) {
			copied_size = (size > CLUSTER_SEGMENT) ? CLUSTER_SEGMENT : size;
			tty_insert_flip_string(device->serial.tty, psrc + retval, copied_size);

			size = size - copied_size;
			retval += copied_size;
		}

		return retval;
	}

	return tty_insert_flip_string(device->serial.tty, psrc, size);
}

static int dpram_read(dpram_device_t *device, const u16 non_cmd)
{
	int retval = 0;
	int size = 0;
    unsigned char *p_data;
    int data_size;

	u16 head, tail, up_tail;

    // todo: only do sms fix on /dpram0
    // if (device==(unsigned long)&dpram_table[FORMATTED_INDEX])
    //  ...

	READ_FROM_DPRAM_VERIFY(&head, device->in_head_addr, sizeof(head));
	READ_FROM_DPRAM_VERIFY(&tail, device->in_tail_addr, sizeof(tail));

#ifdef _DPRAM_DEBUG_HEXDUMP
	printk("\n\n#######[dpram read start: head - %04d, tail - %04d], %s ######\n", head, tail, device->serial.tty->name);
#endif

#ifdef _HSDPA_DPRAM

	if (head != tail) {
		up_tail = 0;

		if (head > tail) {
			size = head - tail;
			retval = dpram_tty_insert_data(device, (unsigned char *)(DPRAM_VBASE + (device->in_buff_addr + tail)), size);

		    if (retval != size)
			dprintk("Size Mismatch : Real Size = %d, Returned Size = %d\n", size, retval);

            p_data = (unsigned char *)(DPRAM_VBASE + (device->in_buff_addr + tail));
            data_size = size;
#ifdef _DPRAM_DEBUG_HEXDUMP
			hexdump(p_data, data_size);

#endif
#ifdef _ENABLE_SMS_FIX
            if(smsfix_enable)
                sms_fix_add_data(p_data, data_size, 0);
#endif
		}
		else {
			int tmp_size = 0;

			size = device->in_buff_size - tail + head;

			tmp_size = device->in_buff_size - tail;
			retval = dpram_tty_insert_data(device, (unsigned char *)(DPRAM_VBASE + (device->in_buff_addr + tail)), tmp_size);

			if (retval != tmp_size)
				dprintk("Size Mismatch : Real Size = %d, Returned Size = %d\n", tmp_size, retval);

			p_data = (unsigned char *)(DPRAM_VBASE + (device->in_buff_addr + tail));
            data_size = tmp_size;
#ifdef _DPRAM_DEBUG_HEXDUMP
			hexdump(p_data, data_size);
#endif
#ifdef _ENABLE_SMS_FIX
            if(smsfix_enable)
                sms_fix_add_data(p_data, data_size, 0);
#endif

			if (size > tmp_size) {
				dpram_tty_insert_data(device, (unsigned char *)(DPRAM_VBASE + device->in_buff_addr), size - tmp_size);

                p_data = (unsigned char *)(DPRAM_VBASE + device->in_buff_addr);
                data_size = size - tmp_size;
#ifdef _DPRAM_DEBUG_HEXDUMP
                hexdump(p_data, data_size);
#endif
#ifdef _ENABLE_SMS_FIX
                if(smsfix_enable)
                    sms_fix_add_data(p_data, data_size, SMS_BUFFER_APPEND);
#endif
				retval += (size - tmp_size);
			}

		}

        up_tail = (u16)((tail + retval) % device->in_buff_size);
		WRITE_TO_DPRAM_VERIFY(device->in_tail_addr, &up_tail, sizeof(up_tail));

	}

#ifdef _DPRAM_DEBUG_HEXDUMP
	printk("#######[dpram read end: head - %04d, tail - %04d]######, %s \n\n", head, up_tail, device->serial.tty->name);
	print_debug_current_time();
#endif
	if (non_cmd & device->mask_req_ack)
		send_interrupt_to_phone(INT_NON_COMMAND(device->mask_res_ack));

#endif

#ifdef CONFIG_EVENT_LOGGING
	dpram_event_logging(DPRAM_READ, (void *)&tail, size);
#endif
    if(smsfix_enable)
        sms_fix_process(device);

	return retval;
}

static void dpram_clear(void)
{
	long i = 0;
	unsigned long flags;

	u16 value = 0;

	local_irq_save(flags);

	for (i = DPRAM_PDA2PHONE_FORMATTED_HEAD_ADDRESS;
			i < DPRAM_SIZE - (DPRAM_INTERRUPT_PORT_SIZE * 2);
			i += 2)
	{
		*((u16 *)(DPRAM_VBASE + i)) = 0;
	}

	local_irq_restore(flags);

	READ_FROM_DPRAM(&value, DPRAM_PHONE2PDA_INTERRUPT_ADDRESS,
			sizeof(value));
}

static void dpram_init_and_report(void)
{
	const u16 magic_code = 0x00aa;
	const u16 init_start = INT_COMMAND(INT_MASK_CMD_INIT_START);
	u16 init_end = INT_COMMAND(INT_MASK_CMD_INIT_END);

	u16 ac_code = 0;

	WRITE_TO_DPRAM(DPRAM_PDA2PHONE_INTERRUPT_ADDRESS,
			&init_start, DPRAM_INTERRUPT_PORT_SIZE);

	WRITE_TO_DPRAM(DPRAM_ACCESS_ENABLE_ADDRESS, &ac_code, sizeof(ac_code));

	dpram_clear();

	WRITE_TO_DPRAM(DPRAM_MAGIC_CODE_ADDRESS,
			&magic_code, sizeof(magic_code));

	ac_code = 0x0001;
	WRITE_TO_DPRAM(DPRAM_ACCESS_ENABLE_ADDRESS, &ac_code, sizeof(ac_code));

	if(dpram_silent_reset){
		WRITE_TO_DPRAM(DPRAM_SILENT_RESET, &dpram_silent_reset, sizeof(dpram_silent_reset));
		dpram_silent_reset = 0;
	}

	phone_boottype = 1;
	if(phone_boottype)
		init_end |= 0x1000;

	WRITE_TO_DPRAM(DPRAM_PDA2PHONE_INTERRUPT_ADDRESS, &init_end, DPRAM_INTERRUPT_PORT_SIZE);
}

static inline int dpram_get_read_available(dpram_device_t *device)
{
	u16 head, tail;

	READ_FROM_DPRAM_VERIFY(&head, device->in_head_addr, sizeof(head));
	READ_FROM_DPRAM_VERIFY(&tail, device->in_tail_addr, sizeof(tail));

	return head - tail;
}

static void dpram_drop_data(dpram_device_t *device)
{
	u16 head;

	READ_FROM_DPRAM_VERIFY(&head, device->in_head_addr, sizeof(head));
	WRITE_TO_DPRAM_VERIFY(device->in_tail_addr, &head, sizeof(head));
}

static void dpram_phone_power_on(void)
{
	int pin_active = gpio_get_value(OMAP_GPIO_PHONE_ACTIVE);

	printk(KERN_ERR "[DPRAM] phone power on is called.\n");

	switch (pin_active) {
		case GPIO_LEVEL_LOW:
		    printk(KERN_ERR "[DPRAM] phone power on GPIO_LEVEL_LOW\n");
			gpio_set_value(OMAP_GPIO_MSM_RST18_N, GPIO_LEVEL_HIGH);

			gpio_set_value(OMAP_GPIO_FONE_ON, GPIO_LEVEL_LOW);
			interruptible_sleep_on_timeout(&phone_power_wait, 50);
			gpio_set_value(OMAP_GPIO_FONE_ON, GPIO_LEVEL_HIGH);
			interruptible_sleep_on_timeout(&phone_power_wait, 50);
			gpio_set_value(OMAP_GPIO_FONE_ON, GPIO_LEVEL_LOW);
			break;

		case GPIO_LEVEL_HIGH:
		    printk(KERN_ERR "[DPRAM] phone power on GPIO_LEVEL_HIGH\n");
			gpio_set_value(OMAP_GPIO_MSM_RST18_N, GPIO_LEVEL_LOW);

			gpio_set_value(OMAP_GPIO_FONE_ON, GPIO_LEVEL_HIGH);
			interruptible_sleep_on_timeout(&phone_power_wait, 50);
			gpio_set_value(OMAP_GPIO_FONE_ON, GPIO_LEVEL_LOW);
			interruptible_sleep_on_timeout(&phone_power_wait, 50);
			gpio_set_value(OMAP_GPIO_FONE_ON, GPIO_LEVEL_HIGH);

			interruptible_sleep_on_timeout(&phone_power_wait, 50);

			gpio_set_value(OMAP_GPIO_MSM_RST18_N, GPIO_LEVEL_HIGH);

			gpio_set_value(OMAP_GPIO_FONE_ON, GPIO_LEVEL_LOW);
			interruptible_sleep_on_timeout(&phone_power_wait, 50);
			gpio_set_value(OMAP_GPIO_FONE_ON, GPIO_LEVEL_HIGH);
			interruptible_sleep_on_timeout(&phone_power_wait, 50);
			gpio_set_value(OMAP_GPIO_FONE_ON, GPIO_LEVEL_LOW);
			break;
	}

}

static void dpram_phone_power_off(void)
{
	const u16 ac_code = 0;
	u16 phone_off = 0x00CF;

	printk(KERN_ERR "[DPRAM] phone power off is called.\n");

	WRITE_TO_DPRAM(DPRAM_ACCESS_ENABLE_ADDRESS,
			&ac_code, sizeof(ac_code));

    WRITE_TO_DPRAM(DPRAM_PDA2PHONE_INTERRUPT_ADDRESS, &phone_off, DPRAM_INTERRUPT_PORT_SIZE);

}

static int dpram_phone_getstatus(void)
{
	return gpio_get_value(OMAP_GPIO_PHONE_ACTIVE);
}

static void dpram_phone_reset(void)
{
	gpio_set_value(OMAP_GPIO_MSM_RST18_N, 0);
	interruptible_sleep_on_timeout(&phone_power_wait, 50);
	gpio_set_value(OMAP_GPIO_MSM_RST18_N, 1);
}

static void dpram_phone_ramdump(void)
{
	const u_int64_t magic_code = 0x454D554C;

	WRITE_TO_DPRAM(DPRAM_MAGIC_CODE_ADDRESS, (void *)&magic_code, sizeof(magic_code));

	dpram_phone_reset();

}

static void dpram_mem_rw(struct _mem_param *param)
{
	if (param->dir) {
		WRITE_TO_DPRAM(param->addr, (void *)&param->data, sizeof(param->data));
	}
	else {
		READ_FROM_DPRAM((void *)&param->data, param->addr, sizeof(param->data));
	}
}

#ifdef CONFIG_PROC_FS

static int dpram_read_proc(char *page, char **start, off_t off,
		int count, int *eof, void *data)
{
	char *p = page;
	int len;

	u16 magic, enable;
	u16 fmt_in_head, fmt_in_tail, fmt_out_head, fmt_out_tail;
	u16 raw_in_head, raw_in_tail, raw_out_head, raw_out_tail;
	u16 in_interrupt = 0, out_interrupt = 0;
	unsigned int silent_reset_bit;

#ifdef _ENABLE_ERROR_DEVICE
	char buf[DPRAM_ERR_MSG_LEN];
	unsigned long flags;
#endif

	READ_FROM_DPRAM((void *)&magic, DPRAM_MAGIC_CODE_ADDRESS, sizeof(magic));
	READ_FROM_DPRAM((void *)&enable, DPRAM_ACCESS_ENABLE_ADDRESS, sizeof(enable));

	READ_FROM_DPRAM((void *)&fmt_in_head, DPRAM_PHONE2PDA_FORMATTED_HEAD_ADDRESS,
			sizeof(fmt_in_head));
	READ_FROM_DPRAM((void *)&fmt_in_tail, DPRAM_PHONE2PDA_FORMATTED_TAIL_ADDRESS,
		    sizeof(fmt_in_tail));
	READ_FROM_DPRAM((void *)&fmt_out_head, DPRAM_PDA2PHONE_FORMATTED_HEAD_ADDRESS,
		    sizeof(fmt_out_head));
	READ_FROM_DPRAM((void *)&fmt_out_tail, DPRAM_PDA2PHONE_FORMATTED_TAIL_ADDRESS,
		    sizeof(fmt_out_tail));

	READ_FROM_DPRAM((void *)&raw_in_head, DPRAM_PHONE2PDA_RAW_HEAD_ADDRESS,
		    sizeof(raw_in_head));
	READ_FROM_DPRAM((void *)&raw_in_tail, DPRAM_PHONE2PDA_RAW_TAIL_ADDRESS,
		    sizeof(raw_in_tail));
	READ_FROM_DPRAM((void *)&raw_out_head, DPRAM_PDA2PHONE_RAW_HEAD_ADDRESS,
		    sizeof(raw_out_head));
	READ_FROM_DPRAM((void *)&raw_out_tail, DPRAM_PDA2PHONE_RAW_TAIL_ADDRESS,
		    sizeof(raw_out_tail));

	READ_FROM_DPRAM((void *)&in_interrupt, DPRAM_PHONE2PDA_INTERRUPT_ADDRESS,
		    DPRAM_INTERRUPT_PORT_SIZE);
	READ_FROM_DPRAM((void *)&out_interrupt, DPRAM_PDA2PHONE_INTERRUPT_ADDRESS,
		    DPRAM_INTERRUPT_PORT_SIZE);

	READ_FROM_DPRAM((void *)&silent_reset_bit, DPRAM_SILENT_RESET, sizeof(silent_reset_bit));

#ifdef _ENABLE_ERROR_DEVICE
	memset((void *)buf, '\0', DPRAM_ERR_MSG_LEN);
	local_irq_save(flags);
	memcpy(buf, dpram_err_buf, DPRAM_ERR_MSG_LEN - 1);
	local_irq_restore(flags);
#endif

	p += sprintf(p,
			"-------------------------------------\n"
			"| NAME\t\t\t| VALUE\n"
			"-------------------------------------\n"
			"| MAGIC CODE\t\t| 0x%04x\n"
			"| ENABLE CODE\t\t| 0x%04x\n"
			"| PHONE->PDA FMT HEAD\t| %u\n"
			"| PHONE->PDA FMT TAIL\t| %u\n"
			"| PDA->PHONE FMT HEAD\t| %u\n"
			"| PDA->PHONE FMT TAIL\t| %u\n"
			"| PHONE->PDA RAW HEAD\t| %u\n"
			"| PHONE->PDA RAW TAIL\t| %u\n"
			"| PDA->PHONE RAW HEAD\t| %u\n"
			"| PDA->PHONE RAW TAIL\t| %u\n"
			"| PHONE->PDA INT.\t| 0x%04x\n"
			"| PDA->PHONE INT.\t| 0x%04x\n"
#ifdef _ENABLE_ERROR_DEVICE
			"| LAST PHONE ERR MSG\t| %s\n"
#endif
			"| PHONE ACTIVE\t\t| %s\n"
			"| DPRAM INT Level\t| %d\n"
			"| FORCED DPRAM WAKEUP\t| %d\n"
			"| PHONE SILENT RESET\t| %d\n"
			"-------------------------------------\n",
			magic, enable,
			fmt_in_head, fmt_in_tail, fmt_out_head, fmt_out_tail,
			raw_in_head, raw_in_tail, raw_out_head, raw_out_tail,
			in_interrupt, out_interrupt,

#ifdef _ENABLE_ERROR_DEVICE
			(buf[0] != '\0' ? buf : "NONE"),
#endif

			(dpram_phone_getstatus() ? "ACTIVE" : "INACTIVE"),
			gpio_get_value(OMAP_GPIO_INT_ONEDRAM_AP),
			forced_dpram_wakeup,
			silent_reset_bit
		);

	len = (p - page) - off;
	if (len < 0) {
		len = 0;
	}

	*eof = (len <= count) ? 1 : 0;
	*start = page + off;

	return len;
}
#endif

static int dpram_tty_open(struct tty_struct *tty, struct file *file)
{
	dpram_device_t *device = &dpram_table[tty->index];

	device->serial.tty = tty;
	device->serial.open_count++;

	if (device->serial.open_count > 1) {
		device->serial.open_count--;
		return -EBUSY;
	}

	tty->driver_data = (void *)device;
	tty->low_latency = 1;

	return 0;
}

static void dpram_tty_close(struct tty_struct *tty, struct file *file)
{
	dpram_device_t *device = (dpram_device_t *)tty->driver_data;

	if (device && (device == &dpram_table[tty->index])) {
		down(&device->serial.sem);
		device->serial.open_count--;
		device->serial.tty = NULL;
		up(&device->serial.sem);
	}
}

static int dpram_tty_write(struct tty_struct *tty,
		const unsigned char *buffer, int count)
{
	dpram_device_t *device = (dpram_device_t *)tty->driver_data;

	if (!device) {
		return 0;
	}

	return dpram_write(device, buffer, count);
}

static int dpram_tty_write_room(struct tty_struct *tty)
{
	int avail;
	u16 head, tail;

	dpram_device_t *device = (dpram_device_t *)tty->driver_data;

	if (device != NULL) {
		READ_FROM_DPRAM_VERIFY(&head, device->out_head_addr, sizeof(head));
		READ_FROM_DPRAM_VERIFY(&tail, device->out_tail_addr, sizeof(tail));

		avail = (head < tail) ? tail - head - 1 :
			device->out_buff_size + tail - head - 1;

		return avail;
	}

	return 0;
}

static int dpram_tty_ioctl(struct tty_struct *tty, struct file *file,
		unsigned int cmd, unsigned long arg)
{
	unsigned int val;
	void __user *argp = (void __user *)arg;
	//printk("[DPRAM] dpram_tty_ioctl: cmd=%d.\n", cmd);


	if(!phone_shutdown)
	switch (cmd) {
		case DPRAM_PHONE_ON:
			printk("DPRAM_PHONE_ON\n");
			return 0;

		case DPRAM_PHONE_GETSTATUS:
		    printk("DPRAM_PHONE_GETSTATUS\n");
			val = dpram_phone_getstatus();
			return copy_to_user((unsigned int *)arg, &val, sizeof(val));

		case DPRAM_PHONE_POWON:
    		printk("DPRAM_PHONE_POWON\n");
			dpram_phone_power_on();
			return 0;

		case DPRAM_PHONEIMG_LOAD:
		    printk("DPRAM_PHONEIMG_LOAD\n");
			return 0;

		case DPRAM_PHONE_BOOTSTART:
    		printk("DPRAM_PHONE_BOOTSTART\n");
            // enable smsfix as late as possible on ril startup
            // DPRAM_PHONE_BOOTSTART seems to be fired as one of the last commands
            smsfix_enable = 1;
            printk("[DPRAM] SMSFIX %s\n", smsfix_enable?"enabled":"disabled");
			return 0;

		case DPRAM_NVDATA_LOAD:
		    printk("DPRAM_NVDATA_LOAD\n");
			return 0;

		case DPRAM_NVPACKET_DATAREAD:
		    printk("DPRAM_NVPACKET_DATAREAD\n");
			return 0;

		case DPRAM_GET_DGS_INFO: {
		    printk("DPRAM_GET_DGS_INFO\n");
			// Copy data to user
			val = copy_to_user((void __user *)arg, aDGSBuf, DPRAM_DGS_INFO_BLOCK_SIZE);
			return 0;
		}

		case DPRAM_PHONE_RESET:
		    printk("DPRAM_PHONE_RESET\n");
			dpram_silent_reset = 1;

			dpram_phone_reset();

			WRITE_TO_DPRAM(DPRAM_SILENT_RESET, &dpram_silent_reset, sizeof(dpram_silent_reset));

			printk("[DPRAM] modem is silently rebooted.\n");

			return 0;

		case DPRAM_PHONE_OFF:
		    printk("DPRAM_PHONE_OFF\n");
			dpram_phone_power_off();
			return 0;

		case DPRAM_PHONE_RAMDUMP_ON:
		    printk("DPRAM_PHONE_RAMDUMP_ON\n");
			return 0;

		case DPRAM_PHONE_RAMDUMP_OFF:
		    printk("DPRAM_PHONE_RAMDUMP_OFF\n");
			return 0;

#ifdef _SAVE_TIME_ZONE_
		case DPRAM_SAVE_TIME_ZONE:
		    printk("DPRAM_SAVE_TIME_ZONE\n");
//			if(copy_from_user((void *)&sec_sys_tz, (void *)arg, sizeof(struct timezone)))
//        return -ENOIOCTLCMD;
			return 0;
#endif
#ifdef _ENABLE_SMS_FIX
		case DPRAM_PHONE_SMSFIX:
        	{
				int enable;
				printk("DPRAM_PHONE_SMSFIX\n");

				if(copy_from_user((void*) &enable, argp, sizeof(int)))
                {
					return -EFAULT;
                }
				else
                {
                    smsfix_enable = enable;
                    printk("[DPRAM] SMSFIX %s\n", smsfix_enable?"enabled":"disabled");
                    return 0;
                }
			}
#endif

		default:
			printk("[DPRAM] Unknown ioctl: %x\n", cmd);
			break;
	}

	return -ENOIOCTLCMD;

}

static int dpram_tty_chars_in_buffer(struct tty_struct *tty)
{
	int data;
	u16 head, tail;

	dpram_device_t *device = (dpram_device_t *)tty->driver_data;

	if (device != NULL) {
		READ_FROM_DPRAM_VERIFY(&head, device->out_head_addr, sizeof(head));
		READ_FROM_DPRAM_VERIFY(&tail, device->out_tail_addr, sizeof(tail));


		data = (head > tail) ? head - tail - 1 :
			device->out_buff_size - tail + head;

		return data;
	}

	return 0;
}

#ifdef _ENABLE_ERROR_DEVICE
static int dpram_err_read(struct file *filp, char *buf, size_t count, loff_t *ppos)
{
	DECLARE_WAITQUEUE(wait, current);

	unsigned long flags;
	ssize_t ret;
	size_t ncopy;

	add_wait_queue(&dpram_err_wait_q, &wait);
	set_current_state(TASK_INTERRUPTIBLE);

	while (1) {
		local_irq_save(flags);

		if (dpram_err_len) {
			ncopy = min(count, dpram_err_len);

			if (copy_to_user(buf, dpram_err_buf, ncopy)) {
				ret = -EFAULT;
			}

			else {
				ret = ncopy;
			}

			dpram_err_len = 0;

			local_irq_restore(flags);
			break;
		}

		local_irq_restore(flags);

		if (filp->f_flags & O_NONBLOCK) {
			ret = -EAGAIN;
			break;
		}

		if (signal_pending(current)) {
			ret = -ERESTARTSYS;
			break;
		}

		schedule();
	}

	set_current_state(TASK_RUNNING);
	remove_wait_queue(&dpram_err_wait_q, &wait);

	return ret;
}

static int dpram_err_fasync(int fd, struct file *filp, int mode)
{
	return fasync_helper(fd, filp, mode, &dpram_err_async_q);
}

static unsigned int dpram_err_poll(struct file *filp,
		struct poll_table_struct *wait)
{
	poll_wait(filp, &dpram_err_wait_q, wait);
	return ((dpram_err_len) ? (POLLIN | POLLRDNORM) : 0);
}
#endif

static void res_ack_tasklet_handler(unsigned long data)
{
	dpram_device_t *device = (dpram_device_t *)data;

	if (device && device->serial.tty) {
		struct tty_struct *tty = device->serial.tty;

		if ((tty->flags & (1 << TTY_DO_WRITE_WAKEUP)) &&
				tty->ldisc->ops->write_wakeup) {
			(tty->ldisc->ops->write_wakeup)(tty);
		}

		wake_up_interruptible(&tty->write_wait);
	}
}

static void send_tasklet_handler(unsigned long data)
{
	dpram_tasklet_data_t *tasklet_data = (dpram_tasklet_data_t *)data;

	dpram_device_t *device = tasklet_data->device;
	u16 non_cmd = tasklet_data->non_cmd;

	int ret = 0;

	if (device) {
		if(device->serial.tty) {
			struct tty_struct *tty = device->serial.tty;

#ifdef _HSDPA_DPRAM
			while (dpram_get_read_available(device)) {
				ret = dpram_read(device, non_cmd);

				if (ret < 0) {
					/* TODO: ... wrong.. */
				}

				tty_flip_buffer_push(tty);
			}
#endif
		}
	}
	else {
		dpram_drop_data(device);
	}

}

static void cmd_req_active_handler(void)
{
	send_interrupt_to_phone(INT_COMMAND(INT_MASK_CMD_RES_ACTIVE));
}

static void cmd_error_display_handler(void)
{
	char buf[DPRAM_ERR_MSG_LEN];

#ifdef _ENABLE_ERROR_DEVICE
	unsigned long flags;
#endif

	memset((void *)buf, 0, sizeof (buf));
	buf[0] = '0';
	buf[1] = ' ';
	READ_FROM_DPRAM((buf + 2), DPRAM_PHONE2PDA_FORMATTED_BUFFER_ADDRESS, sizeof (buf) - 3);

	printk("[PHONE ERROR] ->> %s\n", buf);

#ifdef _ENABLE_ERROR_DEVICE
	local_irq_save(flags);
	memcpy(dpram_err_buf, buf, DPRAM_ERR_MSG_LEN);
	dpram_err_len = 64;
	local_irq_restore(flags);

	wake_up_interruptible(&dpram_err_wait_q);
	kill_fasync(&dpram_err_async_q, SIGIO, POLL_IN);
#endif
}

static void cmd_phone_start_handler(void)
{
	dpram_init_and_report();
}

static void cmd_req_time_sync_handler(void)
{
	/* TODO: add your codes here.. */
}

static void cmd_phone_deep_sleep_handler(void)
{
	/* TODO: add your codes here.. */
}

static void cmd_nv_rebuilding_handler(void)
{
	/* TODO: add your codes here.. */
}

static void cmd_emer_down_handler(void)
{
	/* TODO: add your codes here.. */
}

static void command_handler(u16 cmd)
{
	switch (cmd) {
		case INT_MASK_CMD_REQ_ACTIVE:
			cmd_req_active_handler();
			break;

		case INT_MASK_CMD_ERR_DISPLAY:
			cmd_error_display_handler();
			break;

		case INT_MASK_CMD_PHONE_START:
			cmd_phone_start_handler();
			break;

		case INT_MASK_CMD_REQ_TIME_SYNC:
			cmd_req_time_sync_handler();
			break;

		case INT_MASK_CMD_PHONE_DEEP_SLEEP:
			cmd_phone_deep_sleep_handler();
			break;

		case INT_MASK_CMD_NV_REBUILDING:
			cmd_nv_rebuilding_handler();
			break;

		case INT_MASK_CMD_EMER_DOWN:
			cmd_emer_down_handler();
			break;

		default:
			dprintk("Unknown command.. 0x%04x\n", cmd);
	}
}

static void non_command_handler(u16 non_cmd)
{
	u16 head, tail;

	READ_FROM_DPRAM_VERIFY(&head, DPRAM_PHONE2PDA_FORMATTED_HEAD_ADDRESS, sizeof(head));
	READ_FROM_DPRAM_VERIFY(&tail, DPRAM_PHONE2PDA_FORMATTED_TAIL_ADDRESS, sizeof(tail));

	if (head != tail) {
		non_cmd |= INT_MASK_SEND_F;
		forced_dpram_wakeup = 1;
	}

	READ_FROM_DPRAM_VERIFY(&head, DPRAM_PHONE2PDA_RAW_HEAD_ADDRESS, sizeof(head));
	READ_FROM_DPRAM_VERIFY(&tail, DPRAM_PHONE2PDA_RAW_TAIL_ADDRESS, sizeof(tail));

	if (head != tail)
		non_cmd |= INT_MASK_SEND_R;

	if (non_cmd & INT_MASK_SEND_F) {
		wake_lock_timeout(&dpram_wake_lock, DPRAM_WAKELOCK_TIMEOUT/2);
		dpram_tasklet_data[FORMATTED_INDEX].device = &dpram_table[FORMATTED_INDEX];
		dpram_tasklet_data[FORMATTED_INDEX].non_cmd = non_cmd;

		fmt_send_tasklet.data = (unsigned long)&dpram_tasklet_data[FORMATTED_INDEX];
		tasklet_schedule(&fmt_send_tasklet);
	}

	if (non_cmd & INT_MASK_SEND_R) {
		wake_lock_timeout(&dpram_wake_lock, 4*DPRAM_WAKELOCK_TIMEOUT);
		dpram_tasklet_data[RAW_INDEX].device = &dpram_table[RAW_INDEX];
		dpram_tasklet_data[RAW_INDEX].non_cmd = non_cmd;

		raw_send_tasklet.data = (unsigned long)&dpram_tasklet_data[RAW_INDEX];

		tasklet_hi_schedule(&raw_send_tasklet);
	}

	if (non_cmd & INT_MASK_RES_ACK_F) {
		wake_lock_timeout(&dpram_wake_lock, DPRAM_WAKELOCK_TIMEOUT/2);
		tasklet_schedule(&fmt_res_ack_tasklet);
	}

	if (non_cmd & INT_MASK_RES_ACK_R) {
		printk(KERN_INFO"[DPRAM]non cmd handler -res_ack_raw. tasklet scheduled.\n");
		wake_lock_timeout(&dpram_wake_lock, 4*DPRAM_WAKELOCK_TIMEOUT);
		tasklet_hi_schedule(&raw_res_ack_tasklet);
	}
}

static inline
void check_int_pin_level(void)
{
	u16 mask = 0, cnt = 0;

	while (cnt++ < 3) {
		READ_FROM_DPRAM(&mask, DPRAM_PHONE2PDA_INTERRUPT_ADDRESS, sizeof(mask));

		if (!gpio_get_value(OMAP_GPIO_INT_ONEDRAM_AP))
			break;
	}
}

static irqreturn_t dpram_interrupt(int irq, void *dev_id)
{
	u16 irq_mask = 0;
	//printk("dpram_interrupt \n");
	READ_FROM_DPRAM(&irq_mask, DPRAM_PHONE2PDA_INTERRUPT_ADDRESS, sizeof(irq_mask));

#ifdef _DPRAM_DEBUG_HEXDUMP
	printk("[dpram] >> phone to PDA interrupt happend!- 0x%04x\n", irq_mask);
#endif

	check_int_pin_level();

	if (!(irq_mask & INT_MASK_VALID)) {
		printk("Invalid interrupt mask: 0x%04x\n", irq_mask);
		return IRQ_NONE;
	}

	if (irq_mask & INT_MASK_COMMAND) {
		irq_mask &= ~(INT_MASK_VALID | INT_MASK_COMMAND);
		wake_lock_timeout(&dpram_wake_lock, DPRAM_WAKELOCK_TIMEOUT/2);
		command_handler(irq_mask);
	}

	else {
		irq_mask &= ~INT_MASK_VALID;
		non_command_handler(irq_mask);
	}

	return IRQ_HANDLED;
}

extern u32 hw_revision;
static void phone_error_off_message_handler(int status)
{
	char buf[DPRAM_ERR_MSG_LEN];

#ifdef _ENABLE_ERROR_DEVICE
	unsigned long flags;
#endif

	memset((void *)buf, 0, sizeof (buf));

	if(status == MESG_PHONE_OFF) {
		sprintf(buf, "%d ModemOff", status);
	}

	if(status == MESG_PHONE_RESET) {
		sprintf(buf, "%d ModemRESET", status);
	}

	printk(KERN_INFO"[PHONE ERROR] ->> %s\n", buf);
#if 1

	if(1){
		printk(KERN_INFO"[DPRAM] alarm boot pin status : %d\n", gpio_get_value(OMAP_GPIO_AP_ALARM));
		if(status==MESG_PHONE_OFF && !gpio_get_value(OMAP_GPIO_AP_ALARM))
		{
			printk(KERN_INFO"[DPRAM] Keep Low Modem.\n");
			phone_shutdown = 1;
			gpio_set_value(OMAP_GPIO_FONE_ON, GPIO_LEVEL_LOW);
			gpio_set_value(OMAP_GPIO_MSM_RST18_N, GPIO_LEVEL_LOW);
		}
	}

#endif
#ifdef _ENABLE_ERROR_DEVICE
	local_irq_save(flags);
	memcpy(dpram_err_buf, buf, DPRAM_ERR_MSG_LEN);
	dpram_err_len = 64;
	local_irq_restore(flags);

	wake_up_interruptible(&dpram_err_wait_q);
	kill_fasync(&dpram_err_async_q, SIGIO, POLL_IN);
#endif
}

int prev_phone_state=0;
int curr_phone_state=0;

static irqreturn_t phone_active_irq_handler(int irq, void *dev_id)
{
	if (gpio_get_value(OMAP_GPIO_PHONE_ACTIVE))
		curr_phone_state = 2;
	else
		curr_phone_state = 1;

	if(prev_phone_state) {
		if(prev_phone_state > curr_phone_state)
			phone_error_off_message_handler(MESG_PHONE_OFF);

		if(prev_phone_state < curr_phone_state)
			phone_error_off_message_handler(MESG_PHONE_RESET);
	}

	prev_phone_state = curr_phone_state;
	return IRQ_HANDLED;
}

#ifdef _ENABLE_ERROR_DEVICE
static struct file_operations dpram_err_ops = {
	.owner = THIS_MODULE,
	.read = dpram_err_read,
	.fasync = dpram_err_fasync,
	.poll = dpram_err_poll,
	.llseek = no_llseek,
};
#endif

static struct tty_operations dpram_tty_ops = {
	.open = dpram_tty_open,
	.close = dpram_tty_close,
	.write = dpram_tty_write,
	.write_room = dpram_tty_write_room,
	.ioctl = dpram_tty_ioctl,
	.chars_in_buffer = dpram_tty_chars_in_buffer,
};

#ifdef _ENABLE_ERROR_DEVICE

static void unregister_dpram_err_device(void)
{
	unregister_chrdev(DRIVER_MAJOR_NUM, DPRAM_ERR_DEVICE);
	class_destroy(dpram_class);
}

static int register_dpram_err_device(void)
{
	struct device *dpram_err_dev_t;
	int ret = register_chrdev(DRIVER_MAJOR_NUM, DPRAM_ERR_DEVICE, &dpram_err_ops);

	if ( ret < 0 )
	{
		return ret;
	}

	dpram_class = class_create(THIS_MODULE, "err");

	if (IS_ERR(dpram_class))
	{
		unregister_dpram_err_device();
		return -EFAULT;
	}

	dpram_err_dev_t = device_create(dpram_class, NULL,
			MKDEV(DRIVER_MAJOR_NUM, 0), NULL, DPRAM_ERR_DEVICE);

	if (IS_ERR(dpram_err_dev_t))
	{
		unregister_dpram_err_device();
		return -EFAULT;
	}

	return 0;
}
#endif

static int register_dpram_driver(void)
{
	int retval = 0;

	dpram_tty_driver = alloc_tty_driver(MAX_INDEX);

	if (!dpram_tty_driver) {
		return -ENOMEM;
	}

	dpram_tty_driver->owner = THIS_MODULE;
	dpram_tty_driver->magic = TTY_DRIVER_MAGIC;
	dpram_tty_driver->driver_name = DRIVER_NAME;
	dpram_tty_driver->name = "dpram";
	dpram_tty_driver->major = DRIVER_MAJOR_NUM;
	dpram_tty_driver->minor_start = 1;
	dpram_tty_driver->num = 2;
	dpram_tty_driver->type = TTY_DRIVER_TYPE_SERIAL;
	dpram_tty_driver->subtype = SERIAL_TYPE_NORMAL;
	dpram_tty_driver->flags = TTY_DRIVER_REAL_RAW;
	dpram_tty_driver->init_termios = tty_std_termios;
	dpram_tty_driver->init_termios.c_cflag =
		(B115200 | CS8 | CREAD | CLOCAL | HUPCL);

	tty_set_operations(dpram_tty_driver, &dpram_tty_ops);

	dpram_tty_driver->ttys = dpram_tty;
	dpram_tty_driver->termios = dpram_termios;
	dpram_tty_driver->termios_locked = dpram_termios_locked;

	retval = tty_register_driver(dpram_tty_driver);

	if (retval) {
		dprintk("tty_register_driver error\n");
		put_tty_driver(dpram_tty_driver);
		return retval;
	}

	return 0;
}

static void unregister_dpram_driver(void)
{
	tty_unregister_driver(dpram_tty_driver);
}

static void init_devices(void)
{
	int i;

	for (i = 0; i < MAX_INDEX; i++) {
		init_MUTEX(&dpram_table[i].serial.sem);

		dpram_table[i].serial.open_count = 0;
		dpram_table[i].serial.tty = NULL;
	}
}

static void kill_tasklets(void)
{
	tasklet_kill(&fmt_res_ack_tasklet);
	tasklet_kill(&raw_res_ack_tasklet);

	tasklet_kill(&fmt_send_tasklet);
	tasklet_kill(&raw_send_tasklet);
}

static int register_interrupt_handler(void)
{
	unsigned int dpram_irq, phone_active_irq;
	int retval = 0;

	dpram_irq = OMAP_GPIO_IRQ(OMAP_GPIO_INT_ONEDRAM_AP);
	phone_active_irq = OMAP_GPIO_IRQ(OMAP_GPIO_PHONE_ACTIVE);

	dpram_clear();

	set_irq_type(OMAP_GPIO_IRQ(OMAP_GPIO_PHONE_ACTIVE), IRQ_TYPE_EDGE_BOTH);
	set_irq_type(OMAP_GPIO_IRQ(OMAP_GPIO_INT_ONEDRAM_AP), IRQ_TYPE_LEVEL_LOW);

	retval = request_irq(dpram_irq, dpram_interrupt, IRQF_DISABLED, DRIVER_NAME, NULL);

	if (retval) {
		dprintk("DPRAM interrupt handler failed.\n");
		unregister_dpram_driver();
		return -1;
	}

	retval = request_irq(phone_active_irq, phone_active_irq_handler, IRQF_DISABLED, "Phone Active", NULL);

	if (retval) {
		dprintk("Phone active interrupt handler failed.\n");
		free_irq(dpram_irq, NULL);
		unregister_dpram_driver();
		return -1;
	}

	return 0;
}

static void check_miss_interrupt(void)
{
	unsigned long flags;

	if (gpio_get_value(OMAP_GPIO_PHONE_ACTIVE) && !gpio_get_value(OMAP_GPIO_INT_ONEDRAM_AP)) {
		dprintk("there is a missed interrupt. try to read it!\n");

		local_irq_save(flags);
		dpram_interrupt(OMAP_GPIO_IRQ(OMAP_GPIO_INT_ONEDRAM_AP), NULL);
		local_irq_restore(flags);
	}

}

static int dpram_suspend(struct platform_device *dev, pm_message_t state)
{
	gpio_direction_output(OMAP_GPIO_PDA_ACTIVE, 0);
	return 0;
}

static int dpram_resume(struct platform_device *dev)
{
	gpio_direction_output(OMAP_GPIO_PDA_ACTIVE, 1);
	check_miss_interrupt();
	return 0;
}

void enable_dpram_pins(void)
{
	int ret;

	/*PDA ACTIVE*/
	if(gpio_request(OMAP_GPIO_PDA_ACTIVE, "OMAP_GPIO_PDA_ACTIVE") < 0 ){
    		printk(KERN_ERR "\n FAILED TO REQUEST GPIO %d \n",OMAP_GPIO_PDA_ACTIVE);
    		return;
  	}

	ret = gpio_request( OMAP_GPIO_PHONE_ACTIVE, "OMAP_GPIO_PHONE_ACTIVE");
	if (ret < 0)
	{
		printk( "fail to get gpio : %d, res : %d\n", OMAP_GPIO_PHONE_ACTIVE, ret );
		return;
	}

	ret = gpio_request( OMAP_GPIO_INT_ONEDRAM_AP, "OMAP_GPIO_INT_ONEDRAM_AP");
	if (ret < 0)
	{
		printk( "fail to get gpio : %d, res : %d\n", OMAP_GPIO_INT_ONEDRAM_AP, ret );
		return;
	}

	ret = gpio_request( OMAP_GPIO_FONE_ON, "OMAP_GPIO_FONE_ON");
	if (ret < 0)
	{
		printk( "fail to get gpio : %d, res : %d\n", OMAP_GPIO_FONE_ON, ret );
		return;
	}

	ret = gpio_request( OMAP_GPIO_MSM_RST18_N, "OMAP_GPIO_MSM_RST18_N");
	if (ret < 0)
	{
		printk( "fail to get gpio : %d, res : %d\n", OMAP_GPIO_MSM_RST18_N, ret );
		return;
	}

	ret = gpio_request( OMAP_GPIO_AP_ALARM, "OMAP_GPIO_AP_ALARM");
	if (ret < 0)
	{
		printk( "fail to get gpio : %d, res : %d\n", OMAP_GPIO_AP_ALARM, ret );
		//return;
	}

	gpio_direction_output(OMAP_GPIO_PDA_ACTIVE, 1);
	gpio_direction_input( OMAP_GPIO_PHONE_ACTIVE );
	gpio_direction_input( OMAP_GPIO_INT_ONEDRAM_AP );
	gpio_direction_output( OMAP_GPIO_FONE_ON, 0 );
	gpio_direction_output( OMAP_GPIO_MSM_RST18_N, 0 );
	gpio_direction_input(OMAP_GPIO_AP_ALARM);

	return;
}
void disable_dpram_pins(void)
{
	gpio_free( OMAP_GPIO_PDA_ACTIVE );
	gpio_free( OMAP_GPIO_PHONE_ACTIVE );
	gpio_free( OMAP_GPIO_INT_ONEDRAM_AP );
	gpio_free( OMAP_GPIO_FONE_ON );
	gpio_free( OMAP_GPIO_MSM_RST18_N );
    gpio_free( OMAP_GPIO_AP_ALARM );
}

static int dpram_shared_bank_remap(void)
{
#define DPRAM_START_ADDRESS_PHYS 		0x04000000
#define DPRAM_SHARED_BANK			0x0
#define DPRAM_SHARED_BANK_SIZE			0x4000

	dpram_base = ioremap_nocache(DPRAM_START_ADDRESS_PHYS + DPRAM_SHARED_BANK, DPRAM_SHARED_BANK_SIZE);
	if (dpram_base == NULL) {
		printk("failed ioremap\n");
		return -ENOENT;
	}

	return 0;
}

static int __devinit dpram_probe(struct platform_device *dev)
{
	int retval;

	retval = register_dpram_driver();

	if (retval) {
		dprintk("Failed to register dpram (tty) driver.\n");
		return -1;
	}

#ifdef _ENABLE_ERROR_DEVICE
	retval = register_dpram_err_device();

	if (retval) {
		dprintk("Failed to register dpram error device.\n");

		unregister_dpram_driver();
		return -1;
	}

	memset((void *)dpram_err_buf, '\0', sizeof dpram_err_buf);
#endif

	enable_dpram_pins();

	dpram_shared_bank_remap();

	if ((retval = register_interrupt_handler()) < 0) {
		return -1;
	}

	init_devices();

#ifdef CONFIG_PROC_FS
	create_proc_read_entry(DRIVER_PROC_ENTRY, 0, 0, dpram_read_proc, NULL);
#endif

	printk(DRIVER_ID "\n");

	return 0;
}

void dpram_forced_phoneoff(void)
{
	u16 phone_off = 0x00CF;
	dpram_device_t *device = &dpram_table[0];
	u8 buf[12] = {0x7F, 0x0A, 0x00, 0x50, 0x07, 0x00, 0xFF, 0x00, 0x01, 0x02, 0x01, 0x7E};
	dpram_write(device, buf, 12);

	if(gpio_get_value(OMAP_GPIO_PHONE_ACTIVE)){
		WRITE_TO_DPRAM(DPRAM_PDA2PHONE_INTERRUPT_ADDRESS, &phone_off, DPRAM_INTERRUPT_PORT_SIZE);
		printk(KERN_ERR "[dpram] forced phone power off called.\n");
	}
}

static int __devexit dpram_remove(struct platform_device *dev)
{
	unregister_dpram_driver();

#ifdef _ENABLE_ERROR_DEVICE
	unregister_dpram_err_device();
#endif

	free_irq(OMAP_GPIO_IRQ(OMAP_GPIO_INT_ONEDRAM_AP), NULL);
	free_irq(OMAP_GPIO_IRQ(OMAP_GPIO_PHONE_ACTIVE), NULL);

	kill_tasklets();

    remove_proc_entry(DRIVER_PROC_ENTRY, NULL);

    // can't disable here, pins needed in power down sequence
    // -> todo move enable_dpram_pins to boardfile
    //disable_dpram_pins();

	return 0;
}

static void dpram_shutdown(struct platform_device *dev)
{
    dpram_forced_phoneoff();

	printk("modem power off...\n");

}

static struct platform_driver platform_dpram_driver = {
	.probe = dpram_probe,
	.remove = __devexit_p(dpram_remove),
	.suspend = dpram_suspend,
	.resume = dpram_resume,
	.shutdown	= dpram_shutdown,
	.driver = {
		.name = "dpram-device",
	},
};

static int __init dpram_init(void)
{
	wake_lock_init(&dpram_wake_lock, WAKE_LOCK_SUSPEND, "DPRAM");
	__do_forced_modemoff = dpram_forced_phoneoff;
	return platform_driver_register(&platform_dpram_driver);


}

static void __exit dpram_exit(void)
{
	wake_lock_destroy(&dpram_wake_lock);
	platform_driver_unregister(&platform_dpram_driver);
}

module_init(dpram_init);
module_exit(dpram_exit);

MODULE_AUTHOR("SAMSUNG ELECTRONICS CO., LTD");
MODULE_DESCRIPTION("DPRAM Device Driver for Linux MITs.");
MODULE_LICENSE("GPL");

EXPORT_SYMBOL(dpram_forced_phoneoff);
