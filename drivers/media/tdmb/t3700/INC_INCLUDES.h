#ifndef _INC_INCLUDES_H_
#define _INC_INCLUDES_H_


//TODO 
#include <linux/kernel.h>
#include <linux/string.h>
#include <mach/hardware.h>
#include <mach/gpio.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/module.h>

#include <linux/hwmon.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/spi/spi.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/irq.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/stat.h>
#include <asm/system.h>
#include <asm/mach-types.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/mach/map.h>
#include <linux/timer.h>
//#include <mach/irq.h>
#include <asm/mach/time.h>
#include <linux/dma-mapping.h>


//#include <string.h>

 
typedef unsigned char			INC_UINT8, 			*INC_PUINT8;
typedef unsigned char			INC_BYTE, 			*INC_PBYTE;
typedef unsigned short			INC_UINT16, 		*INC_PUINT16;
typedef unsigned short			INC_WORD, 			*INC_PWORD;
typedef unsigned int			INC_UINT32, 		*INC_PUINT32;
typedef unsigned int			INC_DWORD, 			*INC_PDWORD;
typedef unsigned long			INC_ULONG32, 		*INC_PULONG32;

typedef char					INC_INT8, 			*INC_PINT8;
typedef char					INC_CHAR, 			*INC_PCHAR;
typedef short					INC_INT16, 			*INC_PINT16;
typedef int						INC_INT32, 			*INC_PINT32;
typedef long					INC_LONG32, 		*INC_PLONG32;
typedef unsigned long			INC_DOUBLE32, 		*INC_PDOUBLE32;

//#define INC_MULTI_CHANNEL_ENABLE
#define INC_MULTI_CHANNEL_FIC_UPLOAD
#define INC_MULTI_MAX_CHANNEL			3
/************************************************************************/
/* FIFO Source 사용시                                                   */
/************************************************************************/
//#define INC_FIFO_SOURCE_ENABLE

/************************************************************************/
/* GPIO Source 사용시                                                   */
/************************************************************************/
//#define INC_I2C_GPIO_CTRL_ENABLE

/************************************************************************/
/* DLS Source 사용시                                                    */
/************************************************************************/
//#define INC_DLS_SOURCE_ENABLE

/************************************************************************/
/* USer Application type 사용시                                         */
/************************************************************************/
//#define USER_APPLICATION_TYPE

/************************************************************************/
/* EWS 사용시           			                                    */
/************************************************************************/
//#define INC_EWS_SOURCE_ENABLE


#define INC_MPI_INTERRUPT_ENABLE 	0x0001	//	MPI interrupt
#define INC_I2C_INTERRUPT_ENABLE	0x0002	//	I2C interrupt
#define INC_EWS_INTERRUPT_ENABLE	0x0040	//	EWS interrupt
#define INC_REC_INTERRUPT_ENABLE	0x0080	//	Reconfigration interrupt
#define INC_TII_INTERRUPT_ENABLE	0x0100	//	TII interrupt
#define INC_FIC_INTERRUPT_ENABLE	0x0200	//	FIC Channel TX END interrupt
#define INC_CIFS_INTERRUPT_ENABLE	0x0400	//	CIF Start Interrupt
#define INC_FS_INTERRUPT_ENABLE		0x0800	//	Frame start interrupt
#define INC_EXT_INTERRUPT_ENABLE	0x1000	//	External Interrupt
#define INC_MS_INTERRUPT_ENABLE		0x2000	//	Modem Status interrupt
#define INC_DPLLU_INTERRUPT_ENABLE	0x4000	//	DPLL unlock interrupt
#define INC_SSI_INTERRUPT_ENABLE	0x8000	//	Signal strength indicator interrupt


#define INC_INTERRUPT_POLARITY_HIGH			0x0000
#define INC_INTERRUPT_POLARITY_LOW			0x8000
#define INC_INTERRUPT_PULSE					0x0000
#define INC_INTERRUPT_LEVEL					0x4000
#define INC_INTERRUPT_AUTOCLEAR_DISABLE		0x0000
#define INC_INTERRUPT_AUTOCLEAR_ENABLE		0x2000
#define INC_EXT_INTERRUPT_POLARITY_HIGH		0x0000
#define INC_EXT_INTERRUPT_POLARITY_LOW		0x1000
#define INC_INTERRUPT_PULSE_COUNT			0x00FF
#define INC_INTERRUPT_PULSE_COUNT_MASK		0x03FF


#define INC_ADDRESS_IFDELAY_LSB						0x58
#define INC_ADDRESS_IFDELAY_MSB						0x59
#define INC_ADDRESS_RFDELAY_LSB						0x53
#define INC_ADDRESS_RFDELAY_MSB						0x54

#define INC_AIRPLAY_IF_DLEAY_MAX					1000
#define INC_AIRPLAY_RF_DLEAY_MAX					1100
#define INC_SCAN_IF_DLEAY_MAX							400
#define INC_SCAN_RF_DLEAY_MAX							1500


#define WORD_SWAP(X)			(((X)>>8&0xff)|(((X)<<8)&0xff00))
#define DWORD_SWAP(X)			(((X)>>24&0xff)|(((X)>>8)&0xff00)|(((X)<<8)&0xff0000)|(((X)<<24)&0xff000000))

#define INC_REGISTER_CTRL(X)			((X)*0x1000)
#define STREAM_PARALLEL_ADDRESS			(0x40000000)
#define FIC_REF_TIME_OUT				2500

#define INC_SUCCESS						1
#define INC_ERROR						0
#define INC_NULL						0
#define INC_RETRY						0xff

#define T3700_I2C_ID80					0x80
#define T3700_I2C_ID82					0x82

#define INC_MPI_MAX_BUFF				(1024*8)
#define MPI_CS_SIZE												(188*8)
//#define INC_INTERRUPT_SIZE				(188*32)// W7750 timing optimized	(188*32)
#define INC_INTERRUPT_SIZE				(188*16)
#define MAX_SUBCH_SIZE					32
#define MAX_SUBCHANNEL					64
#define MAX_LABEL_CHAR					16

#define MAX_KOREABAND_FULL_CHANNEL		21
#define MAX_KOREABAND_NORMAL_CHANNEL	6
#define MAX_BAND_III_CHANNEL			41
#define MAX_L_BAND_CHANNEL				23
#define MAX_CHINA_CHANNEL				31
#define MAX_ROAMING_CHANNEL				12


#define RF500_REG_CTRL					105

#define APB_INT_BASE					0x0100
#define APB_GEN_BASE					0x0200
#define APB_PHY_BASE					0x0500
#define APB_DEINT_BASE					0x0600
#define APB_VTB_BASE					0x0700
#define APB_I2S_BASE					0x0800
#define APB_RDI_BASE					0x0900
#define APB_MPI_BASE					0x0A00
#define APB_RS_BASE						0x0B00
#define APB_SPI_BASE					0x0C00
#define APB_I2C_BASE					0x0D00
#define APB_RF_BASE						0x0E00

#define APB_FIC_BASE					0x1000
#define APB_STREAM_BASE										0x2000


#define INC_INT_FIC						0xEFFD
#define INC_INT_MPI						0xEFFB
#define INC_INT_SPI						0xEFDF
#define INC_INT_DISABLE					0xEFFF

#define TS_ERR_THRESHOLD				0x014C
#define INC_ADC_MAX						17

#define INIT_FIC_DB08					0xff
#define INIT_FIC_DB16					0xffff
#define BIT_MASK						0x3f
#define END_MARKER						0xff
#define FIB_SIZE						32
#define FIB_WORD_SIZE					(FIB_SIZE/2)
#define MAX_FIB_NUM						12
#define MAX_FIC_SIZE					(MAX_FIB_NUM*FIB_SIZE)
#define MAX_FRAME_DURATION				96
#define MAX_USER_APP_DATA				32

typedef enum _tagTRANSMISSION
{
	TRANSMISSION_MODE1 = 1,
	TRANSMISSION_MODE2,
	TRANSMISSION_MODE3,
	TRANSMISSION_MODE4,
	TRANSMISSION_AUTO,
	TRANSMISSION_AUTOFAST,
}ST_TRANSMISSION, *PST_TRANSMISSION;

typedef enum _tagDPD_MODE
{
	INC_DPD_OFF = 0,
	INC_DPD_ON,
}INC_DPD_MODE, *PINC_DPD_MODE;

typedef enum _tagPLL_MODE
{
	INPUT_CLOCK_24576KHZ = 0,
	INPUT_CLOCK_12000KHZ,
	INPUT_CLOCK_19200KHZ,
	INPUT_CLOCK_27000KHZ,
}PLL_MODE, *PPLL_MODE;


typedef enum {
	EXTENSION_0	= 0,
	EXTENSION_1,
	EXTENSION_2,
	EXTENSION_3,
	EXTENSION_4,
	EXTENSION_5,
	EXTENSION_6,
	EXTENSION_7,
	EXTENSION_8,
	EXTENSION_9,
	EXTENSION_10,
	EXTENSION_11,
	EXTENSION_12,
	EXTENSION_13,
	EXTENSION_14,
	EXTENSION_15,
	EXTENSION_16,
	EXTENSION_17,
	EXTENSION_18,
	EXTENSION_19,
	EXTENSION_20,
	EXTENSION_21,
	EXTENSION_22,
	EXTENSION_23,
	EXTENSION_24,
}ST_EXTENSION_TYPE, *PST_EXTENSION_TYPE;

typedef enum 
{
	SPI_REGREAD_CMD	= 0,
	SPI_REGWRITE_CMD,
	SPI_MEMREAD_CMD,
	SPI_MEMWRITE_CMD,
	
}ST_SPI_CONTROL, *PST_SPI_CONTROL;

typedef enum{
	TMID_0 = 0,
	TMID_1,
	TMID_2,
	TMID_3,
}ST_TMID, *PST_TMID;

typedef enum{
	PROTECTION_LEVEL0 = 0,
	PROTECTION_LEVEL1,
	PROTECTION_LEVEL2,
	PROTECTION_LEVEL3,
}ST_PROTECTION_LEVEL, *PST_PROTECTION_LEVEL;

typedef enum{
	OPTION_INDICATE0 = 0,
	OPTION_INDICATE1,
	
}ST_INDICATE, *PST_INDICATE;


typedef enum{
	FIG_MCI_SI = 0,
	FIG_LABLE,
	FIG_RESERVED_0,
	FIG_RESERVED_1,
	FIG_RESERVED_2,
	FIG_FICDATA_CHANNEL,
	FIG_CONDITION_ACCESS,
	FIG_IN_HOUSE,	
}ST_FICHEADER_TYPE,*PST_FICHEADER_TYPE;


typedef enum{
	SIMPLE_FIC_ENABLE = 1,
	SIMPLE_FIC_DISABLE,
	
}ST_SIMPLE_FIC, *PST_SIMPLE_FIC;

typedef enum {

	INC_DMB = 1,
	INC_DAB,
	INC_DATA,
	INC_MULTI,
	INC_SINGLE,

	FREQ_FREE = 0,
	FREQ_LOCK,
	FIC_OK = 1,

}INC_CTRL, *PINC_CTRL;

typedef enum 
{
	ERROR_NON 				= 0x0000,
	ERROR_PLL 				= 0xE000,
	ERROR_STOP 				= 0xF000,
	ERROR_READY 			= 0xFF00,
	ERROR_SYNC_NO_SIGNAL	= 0xFC01,
	ERROR_SYNC_LOW_SIGNAL	= 0xFD01,
	ERROR_SYNC_NULL 		= 0xFE01,
	ERROR_SYNC_TIMEOUT 		= 0xFF01,
	ERROR_FICDECODER 		= 0xFF02,
	ERROR_START_MODEM_CLEAR = 0xFF05,
	ERROR_USER_STOP 		= 0xFA00,

	ERROR_MULTI_CHANNEL_COUNT_OVER 	= 0x8000,
	ERROR_MULTI_CHANNEL_COUNT_LOW	= 0x8001,
	ERROR_MULTI_CHANNEL_NULL		= 0x8002,
	ERROR_MULTI_CHANNEL_FREQ		= 0x8003,
	ERROR_MULTI_CHANNEL_DMB_MAX 	= 0x8004,

}INC_ERROR_INFO, *PINC_ERROR_INFO;

typedef struct _tagCHANNEL_INFO
{
	INC_UINT32	ulRFFreq;
	INC_UINT16	uiEnsembleID;
	INC_UINT16	uiBitRate;
	INC_UINT8	uiTmID;
	INC_INT8	aucLabel[MAX_LABEL_CHAR+1];
	INC_INT8	aucEnsembleLabel[MAX_LABEL_CHAR+1];

	INC_UINT8	ucSubChID;
	INC_UINT8	ucServiceType;
	INC_UINT16	uiStarAddr;
	INC_UINT8	ucSlFlag;
	INC_UINT8	ucTableIndex;
	INC_UINT8	ucOption;
	INC_UINT8	ucProtectionLevel;
	INC_UINT16	uiDifferentRate;
	INC_UINT16	uiSchSize;

	INC_UINT32	ulServiceID;
	INC_UINT16	uiPacketAddr;
	
#ifdef USER_APPLICATION_TYPE
	INC_UINT16	uiUserAppType;
	INC_UINT16	uiUserAppDataLength;
	INC_UINT8	aucUserAppData[MAX_USER_APP_DATA];
#endif

}INC_CHANNEL_INFO, *PINC_CHANNEL_INFO;

typedef struct _tagST_SUBCH_INFO
{
	INC_INT16			nSetCnt;
	INC_CHANNEL_INFO	astSubChInfo[MAX_SUBCH_SIZE];
}ST_SUBCH_INFO, *PST_SUBCH_INFO;

typedef enum _tagUPLOAD_MODE_INFO
{
	STREAM_UPLOAD_MASTER_SERIAL = 0,
	STREAM_UPLOAD_MASTER_PARALLEL,
	STREAM_UPLOAD_SLAVE_SERIAL,
	STREAM_UPLOAD_SLAVE_PARALLEL,
	STREAM_UPLOAD_SPI,
	STREAM_UPLOAD_TS,

}UPLOAD_MODE_INFO, *PUPLOAD_MODE_INFO;

typedef enum _tagCLOCK_SPEED
{
	INC_OUTPUT_CLOCK_4096 = 1,
	INC_OUTPUT_CLOCK_2048,
	INC_OUTPUT_CLOCK_1024,
	
}CLOCK_SPEED, *PCLOCK_SPEED;

typedef enum _tagENSEMBLE_BAND
{
	KOREA_BAND_ENABLE = 0,
	BANDIII_ENABLE,
	LBAND_ENABLE,
	CHINA_ENABLE,
	ROAMING_ENABLE,
	EXTERNAL_ENABLE,
	
}ENSEMBLE_BAND, *PENSEMBLE_BAND;

typedef enum _tagFREQ_LOCKINFO
{
	INC_FREQUENCY_UNLOCK = 0,
	INC_FREQUENCY_LOCK,	
}FREQ_LOCKINFO, *PFREQ_LOCKINFO;

typedef enum _tagCTRL_MODE
{
	INC_I2C_CTRL = 0,
	INC_SPI_CTRL,
	INC_EBI_CTRL,
}CTRL_MODE, *PCTRL_MODE;

typedef enum _tagACTIVE_MODE
{
	INC_ACTIVE_LOW = 0,
	INC_ACTIVE_HIGH,

}INC_ACTIVE_MODE, *PINC_ACTIVE_MODE;

#define BER_BUFFER_MAX		3
#define BER_REF_VALUE		35

typedef struct _tagST_BBPINFO
{
	INC_UINT32		ulFreq;
	INC_ERROR_INFO	nBbpStatus;
	INC_UINT8		ucStop;
	ST_TRANSMISSION		ucTransMode;

	INC_UINT8		ucAntLevel;
	INC_UINT8		ucSnr;
	INC_UINT8		ucVber;
	INC_UINT16  	uiCER;
	INC_DOUBLE32 	dPreBER;
	INC_DOUBLE32 	dPostBER;

	INC_UINT32		ulReConfigTime;

	INC_UINT16		uiInCAntTick;
	INC_UINT16		uiInCERAvg;
	INC_UINT16		uiIncPostBER;
	INC_UINT16		auiANTBuff[BER_BUFFER_MAX];

	INC_UINT8		ucProtectionLevel;
    INC_UINT16      uiInCBERTick;
	INC_UINT16		uiBerSum;
    INC_UINT16      auiBERBuff[BER_BUFFER_MAX];

	ST_SUBCH_INFO	stSubChInfo;
}ST_BBPINFO, *PST_BBPINFO;

typedef struct _tagST_FIB_INFO{
	INC_UINT16 uiIsCRC;
	INC_UINT8 ucDataPos;
	INC_UINT8 aucBuff[FIB_SIZE];
}ST_FIB_INFO;

typedef struct _tagST_FIC{
	INC_UINT8	ucBlockNum;
	ST_FIB_INFO  stBlock;
}ST_FIC;

typedef struct  _tagSCH_DB_STRT
{
    INC_UINT8   ucShortLong;
    INC_UINT8   ucTableSW;
    INC_UINT8   ucTableIndex;
    INC_UINT8   ucOption;
    INC_UINT8	ucProtectionLevel; 
    INC_UINT16  uiSubChSize; 
    INC_UINT16  uiBitRate; 
    INC_UINT16  uiDifferentRate;
}SCH_DB_STRT, *PSCH_DB_STRT;

typedef struct  _tagST_UTC_INFO
{
	INC_UINT8	ucGet_Time;
	
	INC_UINT8	ucUTC_Flag;
    INC_UINT16  uiHours;
    INC_UINT16  uiMinutes;
    INC_UINT16  uiSeconds;
    INC_UINT16  uiMilliseconds;
}ST_UTC_INFO, *PST_UTC_INFO;

#ifdef USER_APPLICATION_TYPE	
typedef struct	_tagST_USER_APP_INFO
{
	INC_UINT8	ucSCIdS;
	INC_UINT8	ucNomOfUserApp;
	INC_UINT16	uiUserAppType[MAX_SUBCHANNEL];
	INC_UINT8	ucUserAppDataLength[MAX_SUBCHANNEL];
	INC_UINT8	aucUserAppData[MAX_SUBCHANNEL][MAX_USER_APP_DATA];
}ST_USER_APP_INFO, *PST_USER_APP_INFO;
#endif

typedef struct _tagST_FIC_DB
{
	INC_UINT32		ulRFFreq;
	INC_UINT8		ucSubChCnt;
	INC_UINT8		ucServiceComponentCnt;
	INC_UINT16  	uiEnsembleID;
	INC_UINT16  	uiSubChOk;
	INC_UINT16  	uiSubChInfoOk;
	INC_UINT16  	uiUserAppTypeOk;

	INC_UINT16		aucSetPackAddr[MAX_SUBCHANNEL];
	INC_UINT8		aucDSCType[MAX_SUBCHANNEL];
	INC_UINT8		aucSubChID[MAX_SUBCHANNEL];
	INC_UINT8		aucTmID[MAX_SUBCHANNEL];
	INC_UINT8		aucEnsembleLabel[MAX_LABEL_CHAR];
	INC_UINT8		aucServiceLabel[MAX_SUBCHANNEL][MAX_LABEL_CHAR];
	INC_UINT16		auiStartAddr[MAX_SUBCHANNEL];
	INC_UINT32		aulServiceID[MAX_SUBCHANNEL];
	INC_UINT16		auiServicePackMode[MAX_SUBCHANNEL];
	INC_UINT8		aucSubChPackMode[MAX_SUBCHANNEL];
	INC_UINT16		auiPacketAddr[MAX_SUBCHANNEL];
	INC_UINT8		aucServiceTypePackMode[MAX_SUBCHANNEL];

	INC_UINT8		aucServiceComponID[MAX_SUBCHANNEL];
	INC_UINT8		aucServiceExtension[MAX_SUBCHANNEL];
	INC_UINT16		auiUserAppType[MAX_SUBCHANNEL];

	SCH_DB_STRT			astSchDb[MAX_SUBCHANNEL];
#ifdef USER_APPLICATION_TYPE	
	ST_USER_APP_INFO	astUserAppInfo;
#endif
}ST_FIC_DB, *PST_FIC_DB;

typedef union _tagST_FIG_HEAD{
	INC_UINT8 ucInfo;
	struct {
		INC_UINT8 bitLength		:	5;
		INC_UINT8 bitType		:	3;
	}ITEM;
}ST_FIG_HEAD, *PST_FIG_HEAD;

typedef union _tagST_TYPE_0{
	INC_UINT8 ucInfo;
	struct{
		INC_UINT8 bitExtension	:	5;
		INC_UINT8 bitPD			:	1;
		INC_UINT8 bitOE			:	1;
		INC_UINT8 bitCN			:	1;
	}ITEM;
}ST_TYPE_0, *PST_TYPE_0;

typedef union _tagST_TYPE_1{
	INC_UINT8 ucInfo;
	struct {
		INC_UINT8 bitExtension	:	3;
		INC_UINT8 bitOE			:	1;
		INC_UINT8 bitCharset	:	4;
	}ITEM;
}ST_TYPE_1, *PST_TYPE_1;


typedef union _tagST_USER_APPSERID_16
{
	INC_UINT16 uiInfo;
	struct {
		INC_UINT16 bitServiceID	:	16;
	}ITEM;	
	
}ST_USER_APPSERID_16, *PST_USER_APPSERID_16;

typedef union _tagST_USER_APPSERID_32
{
	INC_UINT32 ulInfo;
	struct {
		INC_UINT32 bitServiceID	:	32;
	}ITEM;	
	
}ST_USER_APPSERID_32, *PST_USER_APPSERID_32;

typedef union _tagST_USER_APP_IDnNUM
{
	INC_UINT8 ucInfo;
	struct{
		INC_UINT8 bitNomUserApp	: 4;
		INC_UINT8 bitSCIdS		: 4;
	}ITEM;
}ST_USER_APP_IDnNUM, *PST_USER_APP_IDnNUM;

typedef union _tagST_USER_APPTYPE
{
	INC_UINT16 uiInfo;
	struct{
		INC_UINT16 bitUserDataLength 	: 5;
		INC_UINT16 bitUserAppType 		: 11;
	}ITEM;
}ST_USER_APPTYPE, *PST_USER_APPTYPE;


typedef union _tagST_TYPE0of0_INFO{
	INC_UINT32 ulBuff;
	struct {
		INC_UINT32 bitLow_CIFCnt	:	8;
		INC_UINT32 bitHigh_CIFCnt	:	5;
		INC_UINT32 bitAlFlag		:	1;
		INC_UINT32 bitChangFlag		:	2;
		INC_UINT32 bitEld			:	16;
	}ITEM;
}ST_TYPE0of0_INFO, *PST_TYPE0of0_INFO;

typedef union _tagST_TYPE0of1Short_INFO{
	INC_UINT32 nBuff;
	struct {
		INC_UINT32 bitReserved		:	8;
		INC_UINT32 bitTableIndex	:	6;
		INC_UINT32 bitTableSw		:	1;
		INC_UINT32 bitShortLong		:	1;
		INC_UINT32 bitStartAddr		:	10;
		INC_UINT32 bitSubChId		:	6;
	}ITEM;
}ST_TYPE0of1Short_INFO, *PST_TYPE0of1Short_INFO;

typedef union _tagST_TYPE0of1Long_INFO{
	INC_UINT32 nBuff;
	struct {
		INC_UINT32 bitSubChanSize	:	10;
		INC_UINT32 bitProtecLevel	:	2;
		INC_UINT32 bitOption		:	3;
		INC_UINT32 bitShortLong		:	1;
		INC_UINT32 bitStartAddr		:	10;
		INC_UINT32 bitSubChId		:	6;
	}ITEM;
}ST_TYPE0of1Long_INFO, *PST_TYPE0of1Long_INFO;

typedef union _tagST_TYPE0of3Id_INFO{
	INC_UINT32 ulData;
	struct {
		INC_UINT32 bitReserved	:	8;
		INC_UINT32 bitPacketAddr:	10;
		INC_UINT32 bitSubChId	:	6;
		INC_UINT32 bitDScType	:	6;
		INC_UINT32 bitRfu		:	1;
		INC_UINT32 bitFlag		:	1;
	}ITEM;
}ST_TYPE0of3Id_INFO, *PST_TYPE0of3Id_INFO;

typedef union _tagST_TYPE0of3_INFO{
	INC_UINT16 nData;
	struct {
		INC_UINT16 bitSccaFlag	:	1;
		INC_UINT16 bitReserved	:	3;
		INC_UINT16 bitScid		:	12;
	}ITEM;
}ST_TYPE0of3_INFO, *PST_TYPE0of3_INFO;

typedef union _tagST_SERVICE_COMPONENT{
	INC_UINT8 ucData;
	struct {
		INC_UINT8 bitNumComponent:	4;
		INC_UINT8 bitCAId		:	3;
		INC_UINT8 bitLocalFlag	:	1;
	}ITEM;
}ST_SERVICE_COMPONENT, *PST_SERVICE_COMPONENT;

typedef union _tagST_TMId_MSCnFIDC{
	INC_UINT16 uiData;
	struct {
		INC_UINT16 bitCAflag	:	1;
		INC_UINT16 bitPS		:	1;
		INC_UINT16 bitSubChld	:	6;
		INC_UINT16 bitAscDscTy	:	6;
		INC_UINT16 bitTMId		:	2;
	}ITEM;
}ST_TMId_MSCnFIDC, *PST_TMId_MSCnFIDC;

typedef union _tagST_MSC_PACKET_INFO{
	INC_UINT16 usData;
	struct {
		INC_UINT16 bitCAflag	:	1;
		INC_UINT16 bitPS		:	1;
		INC_UINT16 bitSCId		:	12;
		INC_UINT16 bitTMId		:	2;
	}ITEM;
}ST_MSC_PACKET_INFO, *PST_MSC_PACKET_INFO;

typedef union _tagST_MSC_BIT{
	INC_UINT8 ucData;
	struct {
		INC_UINT8 bitScIds		:	4;
		INC_UINT8 bitRfa		:	3;
		INC_UINT8 bitExtFlag	:	1;
	}ITEM;
}ST_MSC_BIT, *PST_MSC_BIT;

typedef union _tagST_MSC_LONG{
	INC_UINT16 usData;
	struct {
		INC_UINT16 bitScId		:	12;
		INC_UINT16 bitDummy		:	3;
		INC_UINT16 bitLsFlag	:	1;
	}ITEM;
}ST_MSC_LONG, *PST_MSC_LONG;

typedef union _tagST_MSC_SHORT{
	INC_UINT8 ucData;
	struct {
		INC_UINT8 bitSUBnFIDCId	:	6;
		INC_UINT8 bitMscFicFlag	:	1;
		INC_UINT8 bitLsFlag		:	1;
	}ITEM;
}ST_MSC_SHORT, *PST_MSC_SHORT;

typedef union _tagST_EXTENSION_TYPE14{
	INC_UINT8 ucData;
	struct {
		INC_UINT8 bitSCidS		:	4;
		INC_UINT8 bitRfa		:	3;
		INC_UINT8 bitPD			:	1;
	}ITEM;
}ST_EXTENSION_TYPE14, *PST_EXTENSION_TYPE14;

typedef union _tagST_EXTENSION_TYPE12{
	INC_UINT8 ucData;
	struct {
		INC_UINT8 bitReserved1	:	6;
		INC_UINT8 bitCF_flag	:	1;
		INC_UINT8 bitCountry	:	1;
	}ITEM;
}ST_EXTENSION_TYPE12, *PST_EXTENSION_TYPE12;

typedef union _tagST_UTC_SHORT_INFO{
	INC_UINT32 ulBuff;
	struct {
		INC_UINT32 bitMinutes	:	6;
		INC_UINT32 bitHours		:	5;
		INC_UINT32 bitUTC_Flag	:	1;
		INC_UINT32 bitConf_Ind	:	1;
		INC_UINT32 bitLSI		:	1;
		INC_UINT32 bitMJD		:	17;
		INC_UINT32 bitRfu		:	1;		
	}ITEM;
}ST_UTC_SHORT_INFO, *PST_UTC_SHORT_INFO;

typedef union _tagST_UTC_LONG_INFO{
	INC_UINT16 unBuff;
	struct {
		INC_UINT16 bitMilliseconds	:	10;
		INC_UINT16 bitSeconds		:	6;
	}ITEM;
}ST_UTC_LONG_INFO, *PST_UTC_LONG_INFO;




#ifdef INC_FIFO_SOURCE_ENABLE
#define		INC_CIF_MAX_SIZE		(188*8)
#define INC_FIFO_DEPTH	(1024*10)
#define		MAX_CHANNEL_FIFO		5
#define 	MAX_HEADER_SIZE			16
#define 	HEADER_SERACH_SIZE		(INC_CIF_MAX_SIZE + MAX_HEADER_SIZE)
#define		HEADER_ID_0x33			0x33
#define		HEADER_ID_0x00			0x00
#define 	HEADER_SIZE_BITMASK		0x3FF

typedef enum _tagMULTI_CHANNEL_INFO
{
	MAIN_INPUT_DATA = 0,
	FIC_STREAM_DATA,
	CHANNEL1_STREAM_DATA,
	CHANNEL2_STREAM_DATA,
	CHANNEL3_STREAM_DATA,
}MULTI_CHANNEL_INFO, *PMULTI_CHANNEL_INFO;

typedef enum _tagST_HEADER_INFO
{
	INC_HEADER_SIZE_ERROR = 0,
	INC_HEADER_NOT_SEARACH,
	INC_HEADER_GOOD,
}ST_HEADER_INFO, *PST_HEADER_INFO;


typedef struct _tagST_FIFO{
	INC_UINT32	ulDepth;
	INC_UINT32	ulFront;
	INC_UINT32	ulRear;
	INC_UINT8	acBuff[INC_FIFO_DEPTH+1];
}ST_FIFO, *PST_FIFO;

INC_UINT8	INC_QFIFO_INIT(PST_FIFO pFF,  INC_UINT32 ulDepth);
INC_UINT32 INC_QFIFO_FREE_SIZE(PST_FIFO pFF);
INC_UINT32 INC_QFIFO_GET_SIZE(PST_FIFO pFF);
INC_UINT8	INC_QFIFO_ADD(PST_FIFO pFF, INC_UINT8* pData, INC_UINT32 ulSize);
INC_UINT8	INC_QFIFO_AT(PST_FIFO pFF, INC_UINT8* pData, INC_UINT32 ulSize);
INC_UINT8	INC_QFIFO_BRING(PST_FIFO pFF, INC_UINT8* pData, INC_UINT32 ulSize);
void INC_MULTI_SORT_INIT(void);
ST_FIFO* INC_GET_CHANNEL_FIFO(MULTI_CHANNEL_INFO ucIndex);
INC_UINT8 INC_GET_CHANNEL_DATA(MULTI_CHANNEL_INFO ucIndex, INC_UINT8* pData, INC_UINT32 ulSize);
INC_UINT32	INC_GET_CHANNEL_COUNT(MULTI_CHANNEL_INFO ucIndex);
INC_UINT8	INC_MULTI_FIFO_PROCESS(INC_UINT8* pData, INC_UINT32 ulSize);
ST_HEADER_INFO INC_HEADER_CHECK(ST_FIFO* pMainFifo);

#endif


#ifdef INC_I2C_GPIO_CTRL_ENABLE
#define I2C_SCL_PIN						0x01
#define I2C_SDA_PIN						0x02
#define I2C_BIT_HIGH					1
#define I2C_BIT_LOW						0

#define INTERRUPT_LOCK()
#define INTERRUPT_FREE()

#define INC_I2C_IO_MASK       			0x01
#define INC_I2C_IO_MASK_READ     		0x01
#define INC_I2C_IO_MASK_WRITE     		0x00
#define INC_I2C_ADDRMASK_R(X)       	(((X) & ~INC_I2C_IO_MASK) | INC_I2C_IO_MASK_READ)
#define INC_I2C_ADDRMASK_W(X)       	(((X) & ~INC_I2C_IO_MASK) | INC_I2C_IO_MASK_WRITE)
#define I2C_DATA_BIT_MAX				8
typedef enum _tagINC_I2C_ACK
{
	I2C_ACK_SUCCESS = 0,
	I2C_ACK_ERROR,
}INC_I2C_ACK;

INC_UINT8 I2C_SDA_READ();
void I2C_SDA_WRITE(INC_UINT8 ucBit);
void I2C_SCL_WRITE(INC_UINT8 ucBit);

void		INC_GPIO_I2C_INIT(void);
void		INC_GPIO_I2C_START_COMMAND(void);
void		INC_GPIO_I2C_STOP_COMMAND(void);
void		INC_GPIO_I2C_READ_ACK(void);
INC_UINT8	INC_GPIO_READ_BYTE_IO(void);
INC_I2C_ACK INC_GPIO_I2C_ACK(void);
INC_I2C_ACK INC_GPIO_WRITE_BYTE_IO(INC_UINT8 ucData);
INC_I2C_ACK INC_GPIO_READ_BYTES(INC_UINT8* pBuff, INC_UINT16 uiSize);
INC_I2C_ACK INC_GPIO_WRITE_BYTES(INC_UINT8* pBuff, INC_UINT16 uiSize);
INC_I2C_ACK INC_GPIO_CTRL_WRITE(INC_UINT8 ucI2CID, INC_UINT16 uiAddr, INC_UINT8* pBuff, INC_UINT16 uiSize);
INC_I2C_ACK INC_GPIO_CTRL_READ(INC_UINT8 ucI2CID, INC_UINT16 uiAddr, INC_UINT8* pBuff, INC_UINT16 uiSize);
#endif


#ifdef INC_EWS_SOURCE_ENABLE
typedef union _tagST_TYPE_5{
	INC_UINT8 ucInfo;
	struct {
		INC_UINT8 bitExtension	:	3;
		INC_UINT8 bitTCId		:	3;
		INC_UINT8 bitD2			:	1;
		INC_UINT8 bitD1			:	1;
	}ITEM;
}ST_TYPE_5, *PST_TYPE_5;

typedef union _tagST_EWS_INFO
{
	INC_UINT16 unData;
	struct  
	{
		INC_UINT16	bitID			: 5;
		INC_UINT16	bitMsgGovernment : 3;
		INC_UINT16	bitTotalNo		: 4;
		INC_UINT16	bitThisSeqNo	: 4;
	}ITEM;
	
}ST_EWS_INFO, *PST_EWS_INFO;


typedef union _tagST_EWS_TIME
{
	INC_UINT32 unData;
	struct  
	{
		INC_UINT32	bitUTCMinutes	: 6;
		INC_UINT32	bitUTCHours		: 5;
		INC_UINT32	bitMJD			: 17;
		INC_UINT32	bitReserved		: 4;
	}ITEM;
	
}ST_EWS_TIME, *PST_EWS_TIME;

typedef struct _tagST_DATE_T
{
	INC_UINT16		usYear;
	INC_UINT8		ucMonth;
	INC_UINT8		ucDay;
	INC_UINT8		ucHour;
	INC_UINT8		ucMinutes;
}ST_DATE_T, *PST_DATE_T;


#define EWS_OUTPUT_BUFF_MAX	(1024)
typedef struct _tagST_OUTPUT_EWS
{
	INC_INT16	nNextSeqNo;			//Next Segment No 
	INC_INT16	nTotalSeqNo;		//Total Segment No
	INC_UINT8	ucMsgGovernment;	//메시지 발령기관
	INC_UINT8	ucMsgID;			//메시지 고유번호
	ST_DATE_T	stDate;				//일시
	
	INC_INT8	acKinds[4];			//재난종류
	INC_UINT8	cPrecedence;		//우선순위
	INC_UINT32	ulTime;				//재난시간
	INC_UINT8	ucForm;				//재난지역형식
	INC_UINT8	ucResionCnt;		//재난 지역수
	INC_UINT8	aucResionCode[11];
	INC_DOUBLE32	fEWSCode;

	INC_UINT16	nDataPos;
	INC_UINT8	ucEWSStartEn;
	INC_UINT8	ucIsEWSGood;		//EWS Parsing Good == INC_SUCCESS
	INC_INT8	acOutputBuff[EWS_OUTPUT_BUFF_MAX];	// EWS Message ANSI

}ST_OUTPUT_EWS, *PST_OUTPUT_EWS;



void MJDtoYMD(INC_UINT16 wMJD, ST_DATE_T *pstDate);
void INC_EWS_INIT(void);
void INC_TYPE5_EXTENSION2(ST_FIB_INFO* pFibInfo);
void INC_SET_TYPE_5(ST_FIB_INFO* pFibInfo);
INC_UINT8 INC_EWS_PARSING(INC_UINT8* pucFicBuff, INC_INT32 uFicLength);
INC_UINT8 INC_EWS_FRAMECHECK(INC_UINT8 ucI2CID);
INC_UINT32 YMDtoMJD(ST_DATE_T stDate);
ST_OUTPUT_EWS* INC_GET_EWS_DB(void);

#endif


#ifdef INC_DLS_SOURCE_ENABLE

#define XPAD_INIT				1
#define XPAD_SUCCESS			2
#define XPAD_FRAME_CHECK		4

#define XPAD_TPYE_DLS_ST		2
#define XPAD_TPYE_DLS_CT		3
#define XPAD_END_MARKER			0
#define XPAD_SIZE				250

#define FPAD_POS_BYTE_L			0
#define FPAD_POS_BYTE_L_1		1
#define XPAD_POS_TYPE			6

typedef struct _tagST_DLS{
	INC_INT8	cTransData[XPAD_SIZE];
	INC_UINT8	ucSegment;
	INC_UINT8	ucPadStart;
	INC_UINT8	ucPadCnt;
	INC_UINT8	ucPadOk;
	
}ST_DLS, *PST_DLS;

void		INC_DLS_INIT(void);
void		INC_PAD_UNICODE_CHECK(void);
INC_UINT8	INC_DLS_DECODER(INC_UINT8 *pucData, INC_UINT16 wBitrate);
INC_UINT16	INC_PAD_CRC_CHECK(INC_UINT8 *pBuf, INC_UINT16 wPos, INC_UINT8 ucSize); 
#endif


typedef enum tagINC_SORT_OPTION{

	INC_SUB_CHANNEL_ID = 0,
	INC_START_ADDRESS,
	INC_BIRRATE,
	INC_FREQUENCY,
}INC_SORT_OPTION, *PINC_SORT_OPTION;


void		INC_DELAY(INC_UINT16 uiDelay);
void		INC_MPICLOCK_SET(INC_UINT8 ucI2CID);
void		INC_UPLOAD_MODE(INC_UINT8 ucI2CID);
void		INC_INTERRUPT_CLEAR(INC_UINT8 ucI2CID);
void		INC_INTERRUPT_INIT(INC_UINT8 ucI2CID);
void		INC_SET_CHANNEL(INC_UINT8 ucI2CID, INC_CTRL nMode);
void    INC_MULTI_SET_CHANNEL(INC_UINT8 ucI2CID, ST_SUBCH_INFO* pMultiInfo);
void		INC_BUBBLE_SORT(ST_SUBCH_INFO* pMainInfo, INC_SORT_OPTION Opt);
void 		INC_SWAP(ST_SUBCH_INFO* pMainInfo, INC_UINT16 nNo1, INC_UINT16 nNo2);
void		INTERFACE_USER_STOP(INC_UINT8 ucI2CID);
void		INTERFACE_AUDIO_I2S_ENABLE(INC_UINT8 ucI2CID);
void		INTERFACE_INT_ENABLE(INC_UINT8 ucI2CID);
void		INTERFACE_INT_CLEAR(INC_UINT8 ucI2CID);
void 		INC_INITDB(INC_UINT8 ucI2CID);
void 		INC_SET_SHORTFORM(ST_FIC_DB* pstFicDB, INC_INT32 nCh, ST_TYPE0of1Short_INFO* pShort);
void 		INC_SET_LONGFORM(ST_FIC_DB* pstFicDB, INC_INT32 nCh, ST_TYPE0of1Long_INFO* pLong);
void 		INC_EXTENSION_000(ST_FIB_INFO* pFibInfo, ST_FIC_DB* pstFicDB);
void 		INC_EXTENSION_001(ST_FIB_INFO* pFibInfo, ST_FIC_DB* pstFicDB);
void 		INC_EXTENSION_002(ST_FIB_INFO* pFibInfo, ST_FIC_DB* pstFicDB);
void 		INC_EXTENSION_003(ST_FIB_INFO* pFibInfo, ST_FIC_DB* pstFicDB);
void 		INC_EXTENSION_008(ST_FIB_INFO* pFibInfo, ST_FIC_DB* pstFicDB);
ST_SUBCH_INFO* INC_GET_SETTING_DB(INC_UINT8 ucI2CID);



#ifdef USER_APPLICATION_TYPE
void 		INC_EXTENSION_013(ST_FIB_INFO* pFibInfo, ST_FIC_DB* pstFicDB);
#endif




void 		INC_EXTENSION_110(ST_FIB_INFO* pFibInfo, ST_FIC_DB* pstFicDB);
void 		INC_EXTENSION_111(ST_FIB_INFO* pFibInfo, ST_FIC_DB* pstFicDB);
void 		INC_EXTENSION_112(ST_FIB_INFO* pFibInfo);
void 		INC_EXTENSION_113(ST_FIB_INFO* pFibInfo);
void 		INC_EXTENSION_114(ST_FIB_INFO* pFibInfo, ST_FIC_DB* pstFicDB);
void 		INC_EXTENSION_115(ST_FIB_INFO* pFibInfo, ST_FIC_DB* pstFicDB);
void 		INC_SET_FICTYPE_1(ST_FIB_INFO* pFibInfo, ST_FIC_DB* pstFicDB);
void 		INC_SET_FICTYPE_5(ST_FIB_INFO* pFibInfo, ST_FIC_DB* pstFicDB);
void 		INC_SET_UPDATEFIC(ST_FIB_INFO* pstDestData, INC_UINT8* pSourData);

INC_UINT8	INTERFACE_DBINIT(void);
INC_UINT8	INC_UPDATE(INC_CHANNEL_INFO* pChInfo, ST_FIC_DB* pFicDb, INC_INT16 nIndex);
INC_UINT8	INTERFACE_INIT(INC_UINT8 ucI2CID);
INC_UINT8	INTERFACE_RECONFIG(INC_UINT8 ucI2CID);
INC_UINT8	INTERFACE_STATUS_CHECK(INC_UINT8 ucI2CID);
INC_UINT8	INTERFACE_START(INC_UINT8 ucI2CID, INC_CHANNEL_INFO* pChInfo);
INC_UINT8	INTERFACE_START_TEST(INC_UINT8 ucI2CID, INC_CHANNEL_INFO* pChInfo);
INC_UINT8   INTERFACE_MULTISUB_CHANNEL_START(INC_UINT8 ucI2CID, ST_SUBCH_INFO* pMultiInfo);
ST_SUBCH_INFO* INTERFACE_GET_MULTI_SETTING_DB(INC_UINT8 ucI2CID);

INC_UINT8	INTERFACE_SCAN(INC_UINT8 ucI2CID, INC_UINT32 ulFreq);
INC_UINT8	INTERFACE_GET_SNR(INC_UINT8 ucI2CID);
INC_UINT8	INTERFACE_ISR(INC_UINT8 ucI2CID, INC_UINT8* pBuff);
INC_UINT8 	INC_CHECK_SERVICE_DB16(INC_UINT16* ptr, INC_UINT16 wVal, INC_UINT16 wNum);
INC_UINT8 	INC_CHECK_SERVICE_DB8(INC_UINT8* ptr, INC_UINT8 cVal, INC_UINT8 cNum);
INC_UINT8 	INC_GET_BYTEDATA(ST_FIB_INFO* pFibInfo);
INC_UINT8	INC_GETAT_BYTEDATA(ST_FIB_INFO* pFibInfo);
INC_UINT8 	INC_GET_HEADER(ST_FIB_INFO* pInfo);
INC_UINT8 	INC_GET_TYPE(ST_FIB_INFO* pInfo);
INC_UINT8 	INC_GETAT_HEADER(ST_FIB_INFO* pInfo);
INC_UINT8 	INC_GETAT_TYPE(ST_FIB_INFO* pInfo);
INC_UINT8	INC_INIT(INC_UINT8 ucI2CID);
INC_UINT8	INC_READY(INC_UINT8 ucI2CID, INC_UINT32 ulFreq);
INC_UINT8	INC_SYNCDETECTOR(INC_UINT8 ucI2CID, INC_UINT32 ulFreq, INC_UINT8 ucScanMode);
INC_UINT8	INC_FICDECODER(INC_UINT8 ucI2CID, ST_SIMPLE_FIC bSimpleFIC);
INC_UINT8	INC_START(INC_UINT8 ucI2CID, INC_CHANNEL_INFO* pChInfo, INC_UINT16 IsEnsembleSame);
INC_UINT8   INC_MULTI_START(INC_UINT8 ucI2CID, ST_SUBCH_INFO* pMultiInfo, INC_UINT8 IsEnsembleSame);
INC_UINT8	INC_STOP(INC_UINT8 ucI2CID);
INC_UINT8	INC_CHANNEL_START(INC_UINT8 ucI2CID, INC_CHANNEL_INFO* pChInfo);
INC_UINT8	INC_CHANNEL_START_TEST(INC_UINT8 ucI2CID, INC_CHANNEL_INFO* pChInfo);
INC_UINT8   INC_MULTI_START_CTRL(INC_UINT8 ucI2CID, ST_SUBCH_INFO* pMultiInfo);
INC_ERROR_INFO INC_MULTI_ERROR_CHECK(ST_SUBCH_INFO* pMultiInfo);

INC_UINT8	INC_ENSEMBLE_SCAN(INC_UINT8 ucI2CID, INC_UINT32 ulFreq);
INC_UINT8	INC_FIC_RECONFIGURATION_HW_CHECK(INC_UINT8 ucI2CID);
INC_UINT8	INC_STATUS_CHECK(INC_UINT8 ucI2CID);
INC_UINT8	INC_GET_ANT_LEVEL(INC_UINT8 ucI2CID);
INC_UINT8	INC_GET_SNR(INC_UINT8 ucI2CID);
INC_UINT8	INC_GET_VBER(INC_UINT8 ucI2CID);

INC_UINT8 	INC_GET_NULLBLOCK(ST_FIG_HEAD* pInfo);
INC_UINT8 	INC_GET_FINDLENGTH(ST_FIG_HEAD* pInfo);
INC_UINT8 	INC_SET_TRANSMIT_MODE(INC_UINT8 ucMode);
INC_UINT8 	INC_GET_FINDTYPE(ST_FIG_HEAD* pInfo);
INC_UINT8 	INC_FICPARSING(INC_UINT8 ucI2CID, INC_UINT8* pucFicBuff, INC_INT32 uFicLength, ST_SIMPLE_FIC bSimpleFIC);
INC_UINT8	INC_CMD_WRITE(INC_UINT8 ucI2CID, INC_UINT16 uiAddr, INC_UINT16 uiData);
INC_UINT8	INC_I2C_WRITE(INC_UINT8 ucI2CID, INC_UINT16 uiAddr, INC_UINT16 uiData);
INC_UINT8	INC_EBI_WRITE(INC_UINT8 ucI2CID, INC_UINT32 ulAddr, INC_UINT16 uiData);
INC_UINT16  INC_EBI_READ(INC_UINT8 ucI2CID, INC_UINT16 uiAddr);

INC_UINT8	INC_I2C_READ_BURST(INC_UINT8 ucI2CID, INC_UINT16 uiAddr, INC_UINT8* pData, INC_UINT16 nSize);
INC_UINT8	INC_SPI_REG_WRITE(INC_UINT8 ucI2CID, INC_UINT16 uiAddr, INC_UINT16 uiData);
INC_UINT8	INC_CMD_READ_BURST(INC_UINT8 ucI2CID,  INC_UINT16 uiAddr, INC_UINT8* pData, INC_UINT16 nSize);
INC_UINT8	INC_SPI_READ_BURST(INC_UINT8 ucI2CID, INC_UINT16 uiAddr, INC_UINT8* pBuff, INC_UINT16 wSize);
INC_UINT8	INC_CHIP_STATUS(INC_UINT8 ucI2CID);
INC_UINT8 	INC_RF500_START(INC_UINT8 ucI2CID, INC_UINT32 ulRFChannel, ENSEMBLE_BAND ucBand);
INC_UINT8 	INC_RF500_I2C_WRITE(INC_UINT8 ucI2CID, INC_UINT8 *pucData, INC_UINT32 uLength);
INC_UINT8 	INC_FIC_UPDATE(INC_UINT8 ucI2CID, INC_CHANNEL_INFO* pChInfo);
INC_UINT8	INC_GET_FIB_CNT(ST_TRANSMISSION ucMode);
INC_UINT8   INC_EBI_READ_BURST(INC_UINT8 ucI2CID,  INC_UINT16 uiAddr, INC_UINT8* pData, INC_UINT16 nSize);


INC_INT16	INC_GET_RSSI(INC_UINT8 ucI2CID);
INC_UINT16	INC_GET_SAMSUNG_BER(INC_UINT8 ucI2CID);

INC_INT16 	INC_CHECK_SERVICE_CNT16(INC_UINT16* ptr, INC_UINT16 wVal, INC_UINT8 cNum, INC_UINT16 wMask);
INC_INT16 	INC_CHECK_SERVICE_CNT8(INC_UINT8* ptr, INC_UINT8 cVal, INC_UINT8 cNum, INC_UINT8 cMask);

INC_UINT16 	INC_FIND_KOR_FREQ(INC_UINT32 ulFreq);
INC_UINT16 	INC_FIND_LBAND_FREQ(INC_UINT32 ulFreq);
INC_UINT16 	INC_FIND_CHINA_FREQ(INC_UINT32 ulFreq);
INC_UINT16 	INC_FIND_BANDIII_FREQ(INC_UINT32 ulFreq);
INC_UINT16 	INC_FIND_ROAMING_FREQ(INC_UINT32 ulFreq);
INC_UINT16 	INC_CRC_CHECK(INC_UINT8 *pBuf, INC_UINT8 ucSize) ;
INC_UINT16 	INC_GET_BITRATE(SCH_DB_STRT * pstSchDb);
INC_UINT16 	INC_SET_FICTYPE_0(ST_FIB_INFO* pFibInfo, ST_FIC_DB* pstFicDB);
INC_UINT16 	INC_GET_WORDDATA(ST_FIB_INFO* pFibInfo);
INC_UINT16 	INC_GETAT_WORDDATA(ST_FIB_INFO* pFibInfo);
INC_UINT16 	INC_FIND_SUBCH_SIZE(INC_UINT8 ucTableIndex);

INC_UINT16	INTERFACE_GET_CER(INC_UINT8 ucI2CID);
INC_UINT16	INTERFACE_GETDMB_CNT(void);
INC_UINT16	INTERFACE_GETDAB_CNT(void);
INC_UINT16	INTERFACE_GETDATA_CNT(void);
INC_UINT16	INC_CMD_READ(INC_UINT8 ucI2CID, INC_UINT16 uiAddr);
INC_UINT16	INC_I2C_READ(INC_UINT8 ucI2CID, INC_UINT16 ulAddr);
INC_UINT16	INC_SPI_REG_READ(INC_UINT8 ucI2CID, INC_UINT16 uiAddr);
INC_UINT16	INC_GET_FRAME_DURATION(ST_TRANSMISSION cTrnsMode);
INC_UINT16	INC_GET_CER(INC_UINT8 ucI2CID);

INC_UINT32 	INC_GET_KOREABAND_FULL_TABLE(INC_UINT16 uiIndex);
INC_UINT32 	INC_GET_KOREABAND_NORMAL_TABLE(INC_UINT16 uiIndex);
INC_UINT32 	INC_GET_LONGDATA(ST_FIB_INFO* pFibInfo);
INC_UINT32 	INC_GETAT_LONGDATA(ST_FIB_INFO* pFibInfo);

INC_DOUBLE32 INTERFACE_GET_POSTBER(INC_UINT8 ucI2CID);
INC_DOUBLE32 INTERFACE_GET_PREBER(INC_UINT8 ucI2CID);
INC_DOUBLE32 INC_GET_PREBER(INC_UINT8 ucI2CID);
INC_DOUBLE32 INC_GET_POSTBER(INC_UINT8 ucI2CID);

ST_FIC_DB* 			INC_GETFIC_DB(INC_UINT8 ucI2CID);
ST_BBPINFO* 		INC_GET_STRINFO(INC_UINT8 ucI2CID);
INC_ERROR_INFO		INTERFACE_ERROR_STATUS(INC_UINT8 ucI2CID);
INC_CHANNEL_INFO* 	INTERFACE_GETDB_DMB(INC_INT16 uiPos);
INC_CHANNEL_INFO* 	INTERFACE_GETDB_DAB(INC_INT16 uiPos);
INC_CHANNEL_INFO* 	INTERFACE_GETDB_DATA(INC_INT16 uiPos);
INC_UINT8*          INTERFACE_GETENSEMBLE_LABEL(INC_UINT8 ucI2CID);

INC_UINT8 INC_PLL_SET(INC_UINT8 ucI2CID);

#endif

