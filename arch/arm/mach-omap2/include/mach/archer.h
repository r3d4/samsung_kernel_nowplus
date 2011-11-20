#ifndef __ARCHER_HEADER__
#define __ARCHER_HEADER__

#include <generated/autoconf.h>

//#include "mux-omap3.h"

/* Check for carrier configuration flag */
#ifdef CONFIG_SAMSUNG_ARCHER_TARGET_SK
#elif defined(CONFIG_ARCHER_TARGET_EU)
#elif defined(CONFIG_ARCHER_TARGET_KT)
#error "Not yet supported target configuration!"
else
#error "Archer should select target type!"
#endif


/* ARCHER config definition */

#define ARCHER_REV00		0	/* REV00 */
#define ARCHER_REV05		5	/* REV00_N05 */
#define ARCHER_REV10		10	/* REAL_REV00 */
#define ARCHER_REV11		11	/* REAL_REV01 */
#define ARCHER_REV12		12	/* REAL_REV02 */
#define ARCHER_REV13		13	/* REAL_REV03 */
#define ARCHER_REV20		20	/* REAL_REV08 */

#define CONFIG_ARCHER_REV	CONFIG_SAMSUNG_REL_HW_REV

#if (CONFIG_ARCHER_REV == ARCHER_REV00)
#define CONFIG_REV_STR				"REV 00-EMUL"
#elif (CONFIG_ARCHER_REV == ARCHER_REV05)
#define CONFIG_REV_STR				"REV 05-EMUL"
#elif (CONFIG_ARCHER_REV == ARCHER_REV10)
#define CONFIG_REV_STR				"REV 00-REAL"
#elif (CONFIG_ARCHER_REV == ARCHER_REV11)
#define CONFIG_REV_STR				"REV 01-REAL"
#elif (CONFIG_ARCHER_REV == ARCHER_REV12)
#define CONFIG_REV_STR				"REV 02-REAL"
#elif (CONFIG_ARCHER_REV == ARCHER_REV13)
#define CONFIG_REV_STR				"REV 03-REAL"
#elif (CONFIG_ARCHER_REV == ARCHER_REV20)
#define CONFIG_REV_STR				"REV 08-REAL"
#else
#error "Not Suport ARCHER REVISION"
#endif

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
	__NATION_SEL		= 3,
	__DOWNLOAD_MODE 	= 4,
	__FLAG_EFS_CLEAR	= 5, 
	__LANG_SEL		= 6, 
	
	/* parameter 2 */
	__SERIAL_SPEED		= 7,
	__LOAD_RAMDISK		= 8,
	__BOOT_DELAY		= 9,
	__PHONE_DEBUG_ON	= 10,
	__BL_DIM_TIME		= 11,
	__MELODY_MODE		= 12,
	__PARAM_INT_11		= 13,
	__PARAM_INT_12		= 14,

	/* ADD INTEGER PARAMETER HERE:
	 * Integer parameter index must be less than MAX_INTEGER_PARAM. */
	__CONSOLE_MODE		= 15,
	__DEBUG_LEVEL		= 16,
	__DEBUG_BLOCKPOPUP	= 17,


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

#define LCD_LEVEL			6	/* lcd level */
#define BOOT_DELAY			0	/* wait 0 second */
#define LOAD_RAMDISK			0	/* do not load ramdisk into ram */
#define SWITCH_SEL			3	/* set uart and usb path to pda */
#define PHONE_DEBUG_ON			0	/* set phone debug mode */
#define BL_DIM_TIME			6	/* set backlight dimming time */
#define MELODY_MODE			1	/* set melody mode (sound -> 0, slient -> 1) */
#define DOWNLOAD_MODE			0	/* set download mode */
#define NATION_SEL			0	/* set nation specific configuration */
#define SET_DEFAULT_PARAM		0	/* set default param */
#define REBOOT_MODE			0	/* set reboot mode (normal, recovery, charging) */
#define FLAG_EFS_CLEAR			0
#define LANG_SEL			0
#define VERSION_LINE			"I8320BUIC1"	/* set image version info */
#if defined(CONFIG_SAMSUNG_ARCHER_TARGET_SK) || defined(CONFIG_ARCHER_TARGET_KT)
#define PARAM_INT_12			1	/* set adb mode */
#define CONSOLE_MODE			0	/* console mode (0:off, 1:on) */
#define DEBUG_LEVEL		        0   /* debug mode (0:high, 1:low) */
#define DEBUG_USE_POPUP			1   /* use battery popup (0:off, 1: on)*/
#endif
#define TERMINAL_SPEED                  baud_115200

#if defined(CONFIG_SAMSUNG_ARCHER_TARGET_SK) || defined(CONFIG_ARCHER_TARGET_KT)
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

#endif
