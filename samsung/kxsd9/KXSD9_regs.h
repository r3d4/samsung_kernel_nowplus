#ifndef _KXSD9_REGS_H
#define _KXSD9_REGS_H

/*KXSD9 registers*/
#define    XOUT_H        0x00
#define    XOUT_L        0x01
#define    YOUT_H        0x02
#define    YOUT_L        0x03
#define    ZOUT_H        0x04
#define    ZOUT_L        0x05
#define    AUX_OUT_H     0x06
#define    AUX_OUT_L     0x07
#define    Reset_write   0x0A
#define    CTRL_REGSC    0x0C
#define    CTRL_REGSB    0x0D
#define    CTRL_REGSA    0x0E

/***********Reset_write***********/
#define Reset_write_key    0xCA

/*********** internal bit in CTRL_REGSC ************/
/* Joon.c.baek
 * LPx, Set a Operational Bandwidth
 * */
#define CTRL_REGSC_LP2      0x80
#define CTRL_REGSC_LP1      0x40
#define CTRL_REGSC_LP0      0x20

/* joon.c.baek
 * MOTLat, Related interrupt type, once or several
 * MOTLev, Related interrupt trigger level, 1G ~ 6G
 */
#define CTRL_REGSC_MOTLev   0x10
#define CTRL_REGSC_MOTLat   0x08

/* joon.c.baek
 * FSx, Set a GRAVITY
 * */
#define CTRL_REGSC_FS1      0x02
#define CTRL_REGSC_FS0      0x01
/************************************/

/*********** internal bit in CTRL_REGSB ************/
/* joon.c.baek
 * 	CLKhld,		Hold serial clock
 * 				HIGH= held low during A/D conversion
 * 				LOW= unaffected
 * 		!!! MUST set a 0 when ENABLE set LOW
 */
#define CTRL_REGSB_CLKhld   0x80

/* joon.c.baek
 * ENABLE,	HIGH= Normal Operation
 * 			LOW= Low power standby
 */
#define CTRL_REGSB_ENABLE   0x40

/* joon.c.baek
 * ST,	Self-test bit
 * 		doing self-test when ST=1, EN=1
 */
#define CTRL_REGSB_ST       0x20

/* joon.c.baek
 * MOTIen,	HIGH= Low power operation until interrupt, 
 * 			LOW= Normal Operation
 * */
#define CTRL_REGSB_MOTlen   0x04

/*********** internal bit in CTRL_REGSA ************/
/* joon.c.baek
 * MOTI,	HIGH= occur interrupt
 * 			LOW= do NOT occur interrupt
 */
#define CTRL_REGSA_MOTI     0x02

/*************************************************************/
static inline void switch_on_bits(unsigned char *data, unsigned char bits_on)
{
    *data |= bits_on;
}

static inline void switch_off_bits(unsigned char *data, unsigned char bits_off) 
{
    char aux = 0xFF;
    aux ^= bits_off; 
    *data &= aux;	 
}

#define BIT_ON   1
#define BIT_OFF  0

static inline int check_bit(unsigned char data, unsigned char bit)
{
    return (data|bit) ? BIT_ON : BIT_OFF;
}
/**************************************************************/


/********************************************************************/

/* Joon.c.baek
 * Control register C, bitset GRAVITY, Threshold X point X gravity
 */

static inline void CTRL_REGSC_BITSET_R2g_T1g(unsigned char *data)
{
    switch_on_bits(data, (CTRL_REGSC_MOTLev |CTRL_REGSC_FS1 |CTRL_REGSC_FS0));
}

static inline void CTRL_REGSC_BITSET_R2g_T1p5g(unsigned char *data)
{
    switch_on_bits(data, (CTRL_REGSC_FS1|CTRL_REGSC_FS0));
    switch_off_bits(data, (CTRL_REGSC_MOTLev));	
}

static inline void CTRL_REGSC_BITSET_R4g_T2g(unsigned char *data)
{
    switch_on_bits(data, (CTRL_REGSC_FS1|CTRL_REGSC_MOTLev));
    switch_off_bits(data, (CTRL_REGSC_FS0) );	
}

static inline void CTRL_REGSC_BITSET_R4g_T3g(unsigned char *data)
{
    switch_on_bits(data, (CTRL_REGSC_FS1));
    switch_off_bits(data, (CTRL_REGSC_FS0|CTRL_REGSC_MOTLev));	
}

static inline void CTRL_REGSC_BITSET_R6g_T3g(unsigned char *data)
{
    switch_on_bits(data, (CTRL_REGSC_FS0|CTRL_REGSC_MOTLev));
    switch_off_bits(data, (CTRL_REGSC_FS1));	
}

static inline void CTRL_REGSC_BITSET_R6g_T4p5g(unsigned char *data)
{
    switch_on_bits(data, (CTRL_REGSC_FS0));
    switch_off_bits(data, (CTRL_REGSC_FS1|CTRL_REGSC_MOTLev));	
}

static inline void CTRL_REGSC_BITSET_R8g_T4g(unsigned char *data)
{
    switch_on_bits(data, (CTRL_REGSC_MOTLev));
    switch_off_bits(data, (CTRL_REGSC_FS1|CTRL_REGSC_FS0));	
}

static inline void CTRL_REGSC_BITSET_R8g_T6g(unsigned char *data)
{
    switch_off_bits(data, (CTRL_REGSC_FS1|CTRL_REGSC_FS0|CTRL_REGSC_MOTLev));	
}

static inline void CTRL_REGSC_BITSET_BW50(unsigned char *data)
{
    switch_on_bits(data, (CTRL_REGSC_LP2|CTRL_REGSC_LP1|CTRL_REGSC_LP0));
}

static inline void CTRL_REGSC_BITSET_BW100(unsigned char *data)
{
    switch_on_bits(data, (CTRL_REGSC_LP2|CTRL_REGSC_LP1));
    switch_off_bits(data, (CTRL_REGSC_LP0));	
}

static inline void CTRL_REGSC_BITSET_BW500(unsigned char *data)
{
    switch_on_bits(data, (CTRL_REGSC_LP2|CTRL_REGSC_LP0));
    switch_off_bits(data, (CTRL_REGSC_LP1));	
}

static inline void CTRL_REGSC_BITSET_BW1000(unsigned char *data)
{
    switch_on_bits(data, (CTRL_REGSC_LP2));
    switch_off_bits(data, (CTRL_REGSC_LP1|CTRL_REGSC_LP0));	
}

static inline void CTRL_REGSC_BITSET_BW2000(unsigned char *data)
{
    switch_on_bits(data, (CTRL_REGSC_LP1|CTRL_REGSC_LP0));	
    switch_off_bits(data, (CTRL_REGSC_LP2));
}

static inline void CTRL_REGSC_BITSET_BW_no_filter(unsigned char *data)
{
    switch_off_bits(data, (CTRL_REGSC_LP2|CTRL_REGSC_LP1|CTRL_REGSC_LP0));
}

/* joon.c.baek
 * Control RegisterC Bitset Motion Wakeup
 */
static inline void CTRL_REGSC_BITSET_mwup_rsp_latchd(unsigned char *data)
{
    switch_on_bits(data, (CTRL_REGSC_MOTLat));
}

static inline void CTRL_REGSC_BITSET_mwup_rsp_unlatchd(unsigned char *data)
{
    switch_off_bits(data, (CTRL_REGSC_MOTLat));
}

/********************************************************************/

/********************************************************************/
static inline void CTRL_REGSB_BITSET_mwup_enb(unsigned char *data)
{
    switch_on_bits(data, (CTRL_REGSB_MOTlen));
}

static inline void CTRL_REGSB_BITSET_mwup_disb(unsigned char *data)
{
    switch_off_bits(data, (CTRL_REGSB_MOTlen));
}

static inline void CTRL_REGSB_BITSET_ST_actvt(unsigned char *data)
{
    switch_on_bits(data, (CTRL_REGSB_ST));
}

static inline void CTRL_REGSB_BITSET_ST_dactvt(unsigned char *data)
{
    switch_off_bits(data, (CTRL_REGSB_ST));
}

static inline void CTRL_REGSB_BITSET_clk_ctrl_on(unsigned char *data)
{
    switch_on_bits(data, (CTRL_REGSB_CLKhld));
}

static inline void CTRL_REGSB_BITSET_clk_ctrl_off(unsigned char *data)
{
    switch_off_bits(data, (CTRL_REGSB_CLKhld));
}

static inline void CTRL_REGSB_BITSET_KXSD9_nor_op(unsigned char *data)
{
    switch_on_bits(data, (CTRL_REGSB_ENABLE));
}

static inline void CTRL_REGSB_BITSET_KXSD9_standby(unsigned char *data)
{
    switch_off_bits(data, (CTRL_REGSB_ENABLE));
}
/********************************************************************/

#endif

