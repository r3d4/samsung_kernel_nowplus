#ifndef __NOWPLUS_HEADER__
#define __NOWPLUS_HEADER__

//#include "mux-omap3.h"

/* Check for carrier configuration flag */

/*
 * Partition Information
 */

#define BOOT_PART_ID			0
#define CSA_PART_ID			1	/* Configuration Save Area */
#define SBL_PART_ID			2
#define PARAM_PART_ID			3
#define KERNEL_PART_ID			4
#define RAMDISK_PART_ID			5
#define FILESYSTEM_PART_ID		6
#define FILESYSTEM2_PART_ID		7

#define TEMP_PART_ID			8	/* Temp Area for FOTA */

/*
 * Parameter Item Enumerators
 */
#define COUNT_PARAM_FILE1		7
#define COUNT_PARAM_FILE2               (MAX_PARAM - COUNT_PARAM_FILE1)
#define MAX_PARAM			50
#define MAX_INTEGER_PARAM               40
#define MAX_STRING_PARAM		10
 
typedef enum {
	/* parameter 1 */
	__LCD_LEVEL		= 0,
	__SWITCH_SEL		= 1,
	__SET_DEFAULT_PARAM	= 2,
	__CONSOLE_MODE		= 3,
	__DEBUG_LEVEL		= 4,
	__DEBUG_BLOCKPOPUP	= 5,
	__PARAM_INT_12		= 6,	
	/* START INDEX OF STRING PARAMETER */
	__STR_PARAM_START	= MAX_INTEGER_PARAM,

	/* STRING PARAMETER INDEX */
	__VERSION		= __STR_PARAM_START,
	__CMDLINE		= __STR_PARAM_START + 1,
	__DELTA_LOCATION	= __STR_PARAM_START + 2,
	__PARAM_STR_3		= __STR_PARAM_START + 3,	
	__PARAM_STR_4		= __STR_PARAM_START + 4,
} param_idx;


/*
 * Parameter Values
 */
#define PARAM_PART_BML_ID		4

#define PARAM_INT_12			1	/* set adb mode */
#define CONSOLE_MODE			0	/* console mode (0:off, 1:on) */
#define LCD_LEVEL			6	/* lcd level */
#define SWITCH_SEL			3	/* set uart and usb path to pda */
#define SET_DEFAULT_PARAM		0	/* set default param */
#define VERSION_LINE			"I8320BUIC1"	/* set image version info */

#if defined(CONFIG_ARCHER_TARGET_SK) || defined(CONFIG_ARCHER_TARGET_KT)
#define COMMAND_LINE			"root=/dev/tfsr5 rw console=ttyS2,115200n8 init=/init rootfstype=cramfs "\
					"videoout=omap_vout omap_vout.video1_numbuffers=6 omap_vout.vid1_static_vrfb_alloc=y "\
					"omap_vout.video2_numbuffers=6 omap_vout.vid2_static_vrfb_alloc=y omapfb.vram=\"0:4M\" "\
					"vram=4M,0x%08x omap_version=\"3440\""
#else	/* CONFIG_ARCHER_TARGET_EU */
#define COMMAND_LINE			"root=/dev/tfsr7 rw mem=208M console=ttyS2,115200n8 init=/init "\
					"rootfstype=cramfs videoout=omap_vout omap_vout.video1_numbuffers=6 "\
					"omap_vout.vid1_static_vrfb_alloc=y omapfb.vram=\"0:4M\""
#endif

/* REBOOT_MODE */
#define REBOOT_MODE_NONE                0
#define REBOOT_MODE_DOWNLOAD            1
#define REBOOT_MODE_CHARGING            3
#define REBOOT_MODE_RECOVERY            4
#define REBOOT_MODE_ARM11_FOTA          5
#define REBOOT_MODE_ARM9_FOTA           6

typedef enum {
	baud_1200,
	baud_2400,
	baud_4800,
	baud_9600,
	baud_19200,
	baud_38400,
	baud_57600,
	baud_115200,
	baud_230400,
	baud_380400,
	baud_460800,
	baud_921600
} serial_baud_t;

extern void (*sec_set_param_value)(int idx, void *value);
extern void (*sec_get_param_value)(int idx, void *value);
#endif
