#include <plat/mux_archer_rev_r03.h>

#define IMPROVE_SOURCE // KMJ_DA18

typedef enum
{
    DEFAULT_MODEL,
    ARCHER_KOR_SK,
    TICKER_TAPE,
    OSCAR,
    MODEL_TYPE_MAX
} Atmel_model_type;

typedef enum
{
    VERSION_1_2,
    VERSION_1_4,
    VERSION_1_5,
} Atmel_fw_version_type;


// [[ ryun
#define ATEML_TOUCH_DEBUG 0
#if ATEML_TOUCH_DEBUG
#define dprintk(flag, fmt, args...) printk( "[ATMEL]%s: " fmt, __func__ , ## args)	// ryun !!!  ???
static bool en_touch_log = 1;
#else
#define dprintk(flag, fmt, args...) /* */
static bool en_touch_log = 0;
#endif
#define LONG(x) ((x)/BITS_PER_LONG)
// ]] ryun

/* firmware 2009.09.24 CHJ - start 1/2 */
#define QT602240_I2C_BOOT_ADDR 0x24
#define QT_WAITING_BOOTLOAD_COMMAND 0xC0
#define QT_WAITING_FRAME_DATA       0x80
#define QT_FRAME_CRC_CHECK          0x02
#define QT_FRAME_CRC_PASS           0x04
#define QT_FRAME_CRC_FAIL           0x03

#define WRITE_MEM_OK                1u
#define WRITE_MEM_FAILED            2u
#define READ_MEM_OK                 1u
#define READ_MEM_FAILED             2u

/* firmware 2009.09.24 CHJ - end 1/2 */

#ifdef CONFIG_SAMSUNG_ARCHER_TARGET_SK

// This feature is about making qt chip enter sleep mode by gating power of Driver IC
// , not sending I2C sleep command.
//#define FEATURE_SUSPEND_BY_DISABLING_POWER


//#define FEATURE_LOGGING_TOUCH_EVENT


#define FEATURE_TSP_FOR_TA


#define FEATURE_DYNAMIC_SLEEP


#define FEATURE_CALIBRATE_BY_TEMP


#define FEATURE_CAL_BY_INCORRECT_PRESS


#define FEATURE_VERIFY_INCORRECT_PRESS

#ifdef FEATURE_VERIFY_INCORRECT_PRESS
#define CHECK_COUNT_FOR_NORMAL 4
#define CHECK_COUNT_FOR_CALL 3
#endif
#endif

    
