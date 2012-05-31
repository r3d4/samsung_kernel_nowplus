#ifndef __TOUCHSCREEN_H__
#define __TOUCHSCREEN_H__


//********************************************************************************************************
// function feature defines
//********************************************************************************************************

extern u32 hw_revision;

//#define __CONFIG_FIRMWARE_AUTO_UPDATE__

//#if (defined(CONFIG_MACH_NOWPLUS) && (CONFIG_NOWPLUS_REV == CONFIG_NOWPLUS_REV01_N03) ) \
	|| (defined(CONFIG_MACH_NOWPLUS) && (CONFIG_NOWPLUS_REV >= CONFIG_NOWPLUS_REV01_N04) )
//#define __CONFIG_MULTI_RELEASE_BUG_FIX__
#define __CONFIG_CYPRESS_USE_RELEASE_BIT__
//#endif // (defined(CONFIG_MACH_NOWPLUS) && (CONFIG_NOWPLUS_REV >= CONFIG_NOWPLUS_REV01_N03) ) || defined(CONFIG_MACH_NOWPLUS_MASS)

#define CONFIG_LATE_HANDLER_ENABLE
#define __TSP_I2C_ERROR_RECOVERY__
#define __SYNAPTICS_UNSTABLE_TSP_RECOVERY__
#define __SYNAPTICS_ALWAYS_ACTIVE_MODE__
#define __SYNAPTICS_PRINT_SCREEN_FIRMWARE_UPDATE__
//#define __SYNA_MULTI_TOUCH_SUPPORT__
#define __MAKE_MISSED_UP_EVENT__ 

//********************************************************************************************************
// shared defines
//********************************************************************************************************

// cypress firmware size
#if 0//(CONFIG_NOWPLUS_REV == CONFIG_NOWPLUS_REV01_N04) || (CONFIG_NOWPLUS_REV == CONFIG_NOWPLUS_REV02)
#define TSP_TOTAL_LINES 256	// universal OneDRam board firmware size
#else
#define TSP_TOTAL_LINES 512
#endif
#define TSP_LINE_LENGTH 64

#define CYPRESS_FIRMWARE_IMAGE_SIZE		TSP_TOTAL_LINES*TSP_LINE_LENGTH

// synaptics firmware size
#define SYNAPTICS_FIRMWARE_IMAGE_SIZE	11010


//********************************************************************************************************
// shared defines
//********************************************************************************************************

//#if defined (__SYNAPTICS_PRINT_SCREEN_TOUCH_RECOVERY_INFO__) || defined (__SYNAPTICS_PRINT_SCREEN_FIRMWARE_UPDATE__)
//#include "../../../kernel/kheart-beat/khb_main.h"
//#endif


#endif // __TOUCHSCREEN_H__
