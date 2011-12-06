/*
 * drivers/media/video/mt9p012.c
 *
 * mt9p012 sensor driver
 *
 *
 * Copyright (C) 2008 Texas Instruments.
 *
 * Leverage OV9640.c
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 *****************************************************
 *****************************************************
 * modules/camera/m4mo.c
 *
 * M4MO sensor driver source file
 *
 * Modified by paladin in Samsung Electronics
 *
 * [30-SEP-09] - Modified [ZEUS_CAM]
 */

#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/firmware.h>
#include <mach/gpio.h>
#include <mach/hardware.h>

#include <media/v4l2-int-device.h>

#include <linux/syscalls.h>
#include <linux/file.h>
#include <plat/mux.h>

#include "isp/isp.h"
#include "isp/ispreg.h"
#include "isp/ispccdc.h"


#include "m4mo.h"
#include "omap34xxcam.h"

#if (CAM_M4MO_DBG_MSG)
#define	dprintk(x, y...)	\
	do{\
	printk("["#x"] "y);\
	}while(0)
#else
#define dprintk(x, y...)
#endif

// ZEUS_CAM defined in m4mo.h
#ifndef ZEUS_CAM 
/* used for sensor firmware update */
#include <mach/microusbic.h>
#endif

//#define ENABLE_FIRMWARE_UPDATE
//#define ENABLE_FIRMWARE_DUMP

//forward declaration
static int m4mo_set_focus_mode(s32 value);
static int m4mo_start_capture(void);
static int m4mo_set_jpeg_quality(s32 value);
static int m4mo_set_thumbnail_size(s32 value);

/* Wait Queue For Fujitsu ISP Interrupt */
static DECLARE_WAIT_QUEUE_HEAD(cam_wait);

static int cam_interrupted          = 0;
static u32 jpeg_capture_size        = 0;
static u32 thumbnail_capture_size   = 0;
static int fw_update_status         = 100;
static int fw_dump_status           = 100;
static u8* cam_fw_name              = "RS_M4Mo_RD.bin";;
static int cam_fw_major_version     = 0xFB;;
static int cam_fw_minor_version     = 0xAA;
int camfw_update                    = 0;
static int af_used 					= 0;	//1 if AF was use at some time

static u32 m4mo_curr_state          = M4MO_STATE_INVALID;
static u32 m4mo_pre_state           = M4MO_STATE_INVALID;
static bool m4mo_720p_enable        = false;


extern u32 hw_revision;



/**
 * Array of image sizes supported by M4MO.  These must be ordered from
 * smallest image size to largest.
 */
 /* Preview sizes */
const static struct m4mo_capture_size m4mo_preview_sizes[] = {
	{0,0},          //  
	{160,120},      //  M4MO_QQVGA_SIZE		    0x1	
	{176,144},      //  M4MO_QCIF_SIZE			0x2	
	{320,240},      //  M4MO_QVGA_SIZE			0x3
	{320,240},      //  M4MO_SL1_QVGA_SIZE	    0x4
	{800,600},      //  M4MO_800_600_SIZE		0x5	
	{2560,1920},    //  M4MO_5M_SIZE			0x6	
	{640,482},      //  M4MO_640_482_SIZE		0x7	
	{800,602},      //  M4MO_800_602_SIZE		0x8	
	{320,240},      //  M4MO_SL2_QVGA_SIZE	    0x9	
	{176,176},      //  M4MO_176_176_SIZE		0xA	
	{352,288},      //  M4MO_CIF_SIZE			0xB	
	{400,240},      //  M4MO_WQVGA_SIZE		    0xC	
	{640,480 },	    //  M4MO_VGA_SIZE			0xD	
	{240,320},	    //  M4MO_REV_QVGA_SIZE	    0xE	
	{432,240},	    //  M4MO_REV_WQVGA_SIZE	    0xF	
	{432,240},	    //  M4MO_432_240_SIZE		0x10
	{800,480},	    //  M4MO_WVGA_SIZE		    0x11
	{144,176},	    //  M4MO_REV_QCIF_SIZE		0x12
	{240,432},	    //  M4MO_240_432_SIZE		0x13
	{1024,768},	    //  M4MO_XGA_SIZE			0x14
	{1280,720},	    //  M4MO_1280_720_SIZE		0x15
	{720,480},	    //  M4MO_720_480_SIZE		0x16
	{720,576},	    //  M4MO_720_576_SIZE		0x15
};

/* Image sizes */
const static struct m4mo_capture_size m4mo_image_sizes[] = {
	{0,0},          //  
	{320,240},      //  M4MO_SHOT_QVGA_SIZE		    0x1
	{640,480},      //  M4MO_SHOT_VGA_SIZE		    0x2
	{1280,960},     //  M4MO_SHOT_1M_SIZE	        0x3
	{1600,1200},    //  M4MO_SHOT_2M_SIZE		    0x4
	{2048,1536},    //  M4MO_SHOT_3M_SIZE		    0x5
	{2560,1920},    //  M4MO_SHOT_5M_SIZE		    0x6
	{400,240},      //  M4MO_SHOT_WQVGA_SIZE	    0x7
	{432,240},      //  M4MO_SHOT_432_240_SIZE	    0x8
	{640,360},      //  M4MO_SHOT_640_360_SIZE	    0x9
	{800,480},      //  M4MO_SHOT_WVGA_SIZE		    0xA
	{1280,720},     //  M4MO_SHOT_720P_SIZE		    0xB
	{1920,1080},    //  M4MO_SHOT_1920_1080_SIZE    0xC
	{2560,1440},    //  M4MO_SHOT_2560_1440_SIZE	0xD
	{160,120},      //  M4MO_SHOT_160_120_SIZE	    0xE
	{176,144},      //  M4MO_SHOT_QCIF_SIZE		    0xF
	{144,176},      //  M4MO_SHOT_144_176_SIZE	    0x10
	{176,176},      //  M4MO_SHOT_176_176_SIZE	    0x11
	{240,320},      //  M4MO_SHOT_240_320_SIZE	    0x12
	{240,400},      //  M4MO_SHOT_240_400_SIZE	    0x13
	{352,288},      //  M4MO_SHOT_CIF_SIZE			0x14
	{800,600},      //  M4MO_SHOT_SVGA_SIZE		    0x15
	{1024,768},     //  M4MO_SHOT_1024_768_SIZE	    0x16
	{2304,1728}     //  M4MO_SHOT_4M_SIZE			0x17
};      
            

//0xa..0xfa = 10..250= 1x..25x
static int m4mo_zoom_step [M4MO_ZOOM_STAGES] = {
    10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
    30, 31, 32, 33, 34, 35, 36, 37, 38, 39
};
   

static struct work_struct camera_firmup_work;
static struct work_struct camera_firmdump_work;
static struct workqueue_struct *camera_wq;

static struct m4mo_sensor m4mo = {
	.timeperframe = {
		.numerator   = 0,
		.denominator = 0,
	},
	.fps            = M4MO_UNINIT_VALUE,
	.state          = M4MO_UNINIT_VALUE,
    .mode           = M4MO_UNINIT_VALUE,
    .detect         = SENSOR_NOT_DETECTED,
	.effect         = M4MO_UNINIT_VALUE,
	.iso            = M4MO_UNINIT_VALUE,
	.photometry     = M4MO_UNINIT_VALUE,
	.ev             = M4MO_UNINIT_VALUE,
	.wdr            = M4MO_UNINIT_VALUE,
	.saturation     = M4MO_UNINIT_VALUE,
	.sharpness      = M4MO_UNINIT_VALUE,
	.contrast       = M4MO_UNINIT_VALUE,
	.wb             = M4MO_UNINIT_VALUE,
	.isc            = M4MO_UNINIT_VALUE,
	.scene          = M4MO_UNINIT_VALUE,
	.aewb           = M4MO_UNINIT_VALUE,
	.flash_capture  = M4MO_UNINIT_VALUE,
	.face_detection = M4MO_UNINIT_VALUE,
	.jpeg_quality   = M4MO_UNINIT_VALUE, 
	.zoom           = M4MO_UNINIT_VALUE,
	.thumbnail_size = M4MO_UNINIT_VALUE,
	.preview_size   = M4MO_UNINIT_VALUE,
	.capture_size   = M4MO_UNINIT_VALUE,
	.af_mode        = M4MO_UNINIT_VALUE,
    .jpeg_capture_w = JPEG_CAPTURE_WIDTH,
	.jpeg_capture_h = JPEG_CAPTURE_HEIGHT,
	.focus_mode = 0,
	.focus_auto = 0,
	.exposre_auto = 0,
	.exposure = 4,
//	.zoom_mode = 0,
	.zoom_absolute = 0,
	.auto_white_balance = 0,
//	.white_balance_preset = 0,
	.strobe_conf_mode = V4L2_STROBE_OFF,
};

struct v4l2_queryctrl m4mo_ctrl_list[] = {
      {
        .id            = V4L2_CID_SELECT_MODE,
        .type          = V4L2_CTRL_TYPE_INTEGER,
        .name          = "select mode",
        .minimum       = M4MO_MODE_CAMERA,
        .maximum       = M4MO_MODE_VT,
        .step          = 1,
        .default_value = M4MO_MODE_CAMERA,
      },   
      {
        .id            = V4L2_CID_SELECT_STATE,
        .type          = V4L2_CTRL_TYPE_INTEGER,
        .name          = "select state",
        .minimum       = M4MO_STATE_PREVIEW,
        .maximum       = M4MO_STATE_CAPTURE,
        .step          = 1,
        .default_value = M4MO_STATE_PREVIEW,
      },
	{
		.id = V4L2_CID_AF,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Focus Status",
		.minimum = M4MO_AF_START ,
		.maximum = M4MO_AF_STOP,
		.step = 1,
		.default_value = M4MO_AF_STOP,
	},	
	{
		.id = V4L2_CID_ZOOM,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Digital Zoom",
		.minimum = 10,
		.maximum = 250,
		.step = 1,
		.default_value = 10,
	},	
	{
		.id = V4L2_CID_JPEG_SIZE,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "JPEG Size",
		.flags = V4L2_CTRL_FLAG_READ_ONLY,
	},	
	{
		.id = V4L2_CID_THUMBNAIL_SIZE,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "Thumbnail Size",
		.minimum = M4MO_THUMB_QVGA_SIZE,
		.maximum = M4MO_THUMB_WVGA_SIZE,
		.step = 1,
		.default_value = M4MO_THUMB_QVGA_SIZE,
	},	
	{
		.id = V4L2_CID_JPEG_QUALITY,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "JPEG Quality",
		.minimum = M4MO_JPEG_SUPERFINE,
		.maximum = M4MO_JPEG_ECONOMY,
		.step = 1,
		.default_value = M4MO_JPEG_SUPERFINE,
	},	
	{
		.id = V4L2_CID_ISO,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "ISO",
		.minimum = M4MO_ISO_AUTO,
		.maximum = M4MO_ISO_1000,
		.step = 1,
		.default_value = M4MO_ISO_AUTO,
	},	
	{
		.id = V4L2_CID_WB,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "White Balance",
		.minimum = M4MO_WB_AUTO,
		.maximum = M4MO_WB_HORIZON,
		.step = 1,
		.default_value = M4MO_WB_AUTO,
	},	
	{
		.id = V4L2_CID_EFFECT,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "Image Effect",
		.minimum = M4MO_EFFECT_OFF,
		.maximum = M4MO_EFFECT_AQUA,
		.step = 1,
		.default_value = M4MO_EFFECT_OFF,
	},	
	{
		.id = V4L2_CID_SCENE,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "Scene",
		.minimum = M4MO_SCENE_NORMAL,
		.maximum = M4MO_SCENE_BACKLIGHT,
		.step = 1,
		.default_value = M4MO_SCENE_NORMAL,
	},	
	{
		.id = V4L2_CID_PHOTOMETRY,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "Photometry(Metering)",
		.minimum = M4MO_PHOTOMETRY_AVERAGE,
		.maximum = M4MO_PHOTOMETRY_SPOT,
		.step = 1,
		.default_value = M4MO_PHOTOMETRY_CENTER,
	},	
	{
		.id = V4L2_CID_WDR,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "Wide Dynamic Range",
		.minimum = M4MO_WDR_OFF,
		.maximum = M4MO_WDR_ON,
		.step = 1,
		.default_value = M4MO_WDR_OFF,
	},	
	{
		.id = V4L2_CID_ISC,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "Image Stabilization Control",
		.minimum = M4MO_ISC_STILL_OFF,
		.maximum = M4MO_ISC_STILL_ON,
		.step = 1,
		.default_value = M4MO_ISC_STILL_OFF,
	},	
	{
		.id = V4L2_CID_AEWB,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "AE/AWB Lock/Unlock",
		.minimum = M4MO_AE_LOCK_AWB_LOCK,
		.maximum = M4MO_AE_UNLOCK_AWB_UNLOCK,
		.step = 1,
		.default_value = M4MO_AE_UNLOCK_AWB_UNLOCK,
	},	
	{
		.id = V4L2_CID_FW_UPDATE,
		.type = V4L2_CTRL_TYPE_BUTTON,
		.name = "Firmware Update",
	},	
	{
		.id = V4L2_CID_FLASH_CAPTURE,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "Flash Control",
		.minimum = M4MO_FLASH_CAPTURE_OFF,
		.maximum = M4MO_FLASH_MOVIE_ON,
		.step = 1,
		.default_value = M4MO_FLASH_CAPTURE_OFF,
	},	
	{
		.id = V4L2_CID_FACE_DETECTION,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.name = "Face Detection Control",
		.minimum = M4MO_FACE_DETECTION_OFF,
		.maximum = M4MO_FACE_DETECTION_ON,
		.step = 1,
		.default_value = M4MO_FACE_DETECTION_OFF,
	},	
	{
		.id = V4L2_CID_ESD_INT,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.name = "ESD Interrupt",
		.flags = V4L2_CTRL_FLAG_READ_ONLY,
	},	
	{
		.id = V4L2_CID_BRIGHTNESS,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "Exposure Control",
		.minimum = M4MO_EV_MINUS_4,
		.maximum = M4MO_EV_PLUS_4,
		.step = 1,
		.default_value = M4MO_EV_DEFAULT,
	},	
	{
		.id = V4L2_CID_CONTRAST,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "Contrast Control",
		.minimum = M4MO_CONTRAST_MINUS_3,
		.maximum = M4MO_CONTRAST_PLUS_3,
		.step = 1,
		.default_value = M4MO_CONTRAST_DEFAULT,
	},	
	{
		.id = V4L2_CID_SATURATION,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "Saturation Control",
		.minimum = M4MO_SATURATION_MINUS_3,
		.maximum = M4MO_SATURATION_PLUS_3,
		.step = 1,
		.default_value = M4MO_SATURATION_DEFAULT,
	},	
	{
		.id = V4L2_CID_SHARPNESS,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "Sharpness Control",
		.minimum = M4MO_SHARPNESS_MINUS_3,
		.maximum = M4MO_SHARPNESS_PLUS_3,
		.step = 1,
		.default_value = M4MO_SHARPNESS_DEFAULT,
	},	
	{
		.id = V4L2_CID_EXPOSURE_AUTO,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "Set Exposure Mode",
		.minimum = 0,
		.maximum = 1,
		.step = 1,
		.default_value = 0,
	},	
	{
		.id = V4L2_CID_EXPOSURE,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "Set Exposure",
		.minimum = 0,
		.maximum = 8,
		.step = 1,
		.default_value = 4,
	},
	{
		.id = V4L2_CID_FOCUS_MODE,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Set Focus Mode",
		.minimum = M4MO_AF_MODE_NORMAL,
		.maximum = M4MO_AF_MODE_MACRO,
		.step = 1,
		.default_value = M4MO_AF_MODE_NORMAL,
	},
	{
		.id = V4L2_CID_FOCUS_AUTO,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.name = "Start/Stop Auto Focus",
		.minimum = 0,
		.maximum = 1,
		.step = 1,
		.default_value = 0,
	},
#if 0    
	{
		.id = V4L2_CID_ZOOM_MODE,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "Set Zoom Mode",
		.minimum = 0,
		.maximum = 0,
		.step = 1,
		.default_value = 0,
	},
#endif
	{
		.id = V4L2_CID_ZOOM_ABSOLUTE,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "Set Digital Zoom",
		.minimum = 0,
		.maximum = 8,
		.step = 1,
		.default_value = 0,
	},
	{
		.id = V4L2_CID_AUTO_WHITE_BALANCE,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "Set WB Mode",
		.minimum = 0,
		.maximum = 1,
		.step = 1,
		.default_value = 0,
	},
#if 0
	{
		.id = V4L2_CID_WHITE_BALANCE_PRESET,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "Set WB",
		.minimum = 0,
		.maximum = 6,
		.step = 1,
		.default_value = 0,
	},
#endif
	{
		.id = V4L2_CID_THUMBNAIL_SIZE,
		.name = "Thumbnail Image Size",
		.flags = V4L2_CTRL_FLAG_READ_ONLY,
	},
	{
		.id = V4L2_CID_FW_VERSION,
		.name = "Camera Firmware Version",
		.flags = V4L2_CTRL_FLAG_READ_ONLY,
	},
};
#define NUM_M4MO_CONTROL ARRAY_SIZE(m4mo_ctrl_list)

struct v4l2_querymenu m4mo_menu_list[] = {
	{.id = V4L2_CID_AF, .index = M4MO_AF_STOP, .name = "AF Stop"},
	{.id = V4L2_CID_AF, .index = M4MO_AF_START,.name = "AF Start"},
	{.id = V4L2_CID_AF, .index = M4MO_AF_MODE_NORMAL,.name = "Set AF normal mode"},
	{.id = V4L2_CID_AF, .index = M4MO_AF_MODE_MACRO, .name = "Set AF macro mode"},
    
	{.id = V4L2_CID_THUMBNAIL_SIZE, .index = M4MO_THUMB_QVGA_SIZE, .name = "QVGA Size"},
	{.id = V4L2_CID_THUMBNAIL_SIZE, .index = M4MO_THUMB_400_225_SIZE, .name = "400 X 225 Size"},
	{.id = V4L2_CID_THUMBNAIL_SIZE, .index = M4MO_THUMB_WQVGA_SIZE, .name = "WQVGA Size"},
	{.id = V4L2_CID_THUMBNAIL_SIZE, .index = M4MO_THUMB_VGA_SIZE, .name = "VGA Size"},
	{.id = V4L2_CID_THUMBNAIL_SIZE, .index = M4MO_THUMB_WVGA_SIZE, .name = "WVGA Size"},
	
	{.id = V4L2_CID_JPEG_QUALITY, .index = M4MO_JPEG_SUPERFINE, .name = "Superfine"},
	{.id = V4L2_CID_JPEG_QUALITY, .index = M4MO_JPEG_FINE, .name = "Fine"},
	{.id = V4L2_CID_JPEG_QUALITY, .index = M4MO_JPEG_NORMAL, .name = "Normal"},
	{.id = V4L2_CID_JPEG_QUALITY, .index = M4MO_JPEG_ECONOMY, .name = "Economy"},

	{.id = V4L2_CID_ISO, .index = M4MO_ISO_AUTO, .name = "ISO Auto"},
	{.id = V4L2_CID_ISO, .index = M4MO_ISO_50, .name = "ISO 50"},
	{.id = V4L2_CID_ISO, .index = M4MO_ISO_100, .name = "ISO 100"},
	{.id = V4L2_CID_ISO, .index = M4MO_ISO_200, .name = "ISO 200"},
	{.id = V4L2_CID_ISO, .index = M4MO_ISO_400, .name = "ISO 400"},
	{.id = V4L2_CID_ISO, .index = M4MO_ISO_800, .name = "ISO 800"},
	{.id = V4L2_CID_ISO, .index = M4MO_ISO_1000, .name = "ISO 1000"},

	{.id = V4L2_CID_WB, .index = M4MO_WB_AUTO, .name = "WB Auto"},
	{.id = V4L2_CID_WB, .index = M4MO_WB_INCANDESCENT, .name = "Incandescent"},
	{.id = V4L2_CID_WB, .index = M4MO_WB_FLUORESCENT, .name = "Fluorescent"},
	{.id = V4L2_CID_WB, .index = M4MO_WB_DAYLIGHT, .name = "Daylight"},
	{.id = V4L2_CID_WB, .index = M4MO_WB_CLOUDY, .name = "Cloudy"},
	{.id = V4L2_CID_WB, .index = M4MO_WB_SHADE, .name = "Shade"},
	{.id = V4L2_CID_WB, .index = M4MO_WB_HORIZON, .name = "Horizon"},

	{.id = V4L2_CID_EFFECT, .index = M4MO_EFFECT_OFF, .name = "Effect Off"},
	{.id = V4L2_CID_EFFECT, .index = M4MO_EFFECT_SEPIA, .name = "Sepia"},
	{.id = V4L2_CID_EFFECT, .index = M4MO_EFFECT_GRAY, .name = "Gray"},
	{.id = V4L2_CID_EFFECT, .index = M4MO_EFFECT_RED, .name = "Red"},
	{.id = V4L2_CID_EFFECT, .index = M4MO_EFFECT_GREEN, .name = "Green"},
	{.id = V4L2_CID_EFFECT, .index = M4MO_EFFECT_BLUE, .name = "Blue"},
	{.id = V4L2_CID_EFFECT, .index = M4MO_EFFECT_PINK, .name = "Pink"},
	{.id = V4L2_CID_EFFECT, .index = M4MO_EFFECT_YELLOW, .name = "Yellow"},
	{.id = V4L2_CID_EFFECT, .index = M4MO_EFFECT_PURPLE, .name = "Purple"},
	{.id = V4L2_CID_EFFECT, .index = M4MO_EFFECT_ANTIQUE, .name = "Antique"},
	{.id = V4L2_CID_EFFECT, .index = M4MO_EFFECT_NEGATIVE, .name = "Negative"},
	{.id = V4L2_CID_EFFECT, .index = M4MO_EFFECT_SOLARIZATION1, .name = "Solarization1"},
	{.id = V4L2_CID_EFFECT, .index = M4MO_EFFECT_SOLARIZATION2, .name = "Solarization2"},
	{.id = V4L2_CID_EFFECT, .index = M4MO_EFFECT_SOLARIZATION3, .name = "Solarization3"},
	{.id = V4L2_CID_EFFECT, .index = M4MO_EFFECT_SOLARIZATION4, .name = "Solarization4"},
	{.id = V4L2_CID_EFFECT, .index = M4MO_EFFECT_EMBOSS, .name = "Emboss"},
	{.id = V4L2_CID_EFFECT, .index = M4MO_EFFECT_OUTLINE, .name = "Outline"},
	{.id = V4L2_CID_EFFECT, .index = M4MO_EFFECT_AQUA, .name = "Aqua"},

	{.id = V4L2_CID_SCENE, .index = M4MO_SCENE_NORMAL, .name = "Scene Normal"},
	{.id = V4L2_CID_SCENE, .index = M4MO_SCENE_PORTRAIT, .name = "Portrait"},
	{.id = V4L2_CID_SCENE, .index = M4MO_SCENE_LANDSCAPE, .name = "Landscape"},
	{.id = V4L2_CID_SCENE, .index = M4MO_SCENE_SPORTS, .name = "Sports"},
	{.id = V4L2_CID_SCENE, .index = M4MO_SCENE_PARTY_INDOOR, .name = "Party or Indoor"},
	{.id = V4L2_CID_SCENE, .index = M4MO_SCENE_BEACH_SNOW, .name = "Beach or Snow"},
	{.id = V4L2_CID_SCENE, .index = M4MO_SCENE_SUNSET, .name = "Sunset"},
	{.id = V4L2_CID_SCENE, .index = M4MO_SCENE_DUSK_DAWN, .name = "Dusk and Dawn"},
	{.id = V4L2_CID_SCENE, .index = M4MO_SCENE_FALL_COLOR, .name = "Fall Color"},
	{.id = V4L2_CID_SCENE, .index = M4MO_SCENE_NIGHT, .name = "Night"},
	{.id = V4L2_CID_SCENE, .index = M4MO_SCENE_FIREWORK, .name = "Firework"},
	{.id = V4L2_CID_SCENE, .index = M4MO_SCENE_TEXT, .name = "Text"},
	{.id = V4L2_CID_SCENE, .index = M4MO_SCENE_CANDLELIGHT, .name = "Candlelight"},
	{.id = V4L2_CID_SCENE, .index = M4MO_SCENE_BACKLIGHT, .name = "Backlight"},
	
	{.id = V4L2_CID_PHOTOMETRY, .index = M4MO_PHOTOMETRY_AVERAGE, .name = "Average"},
	{.id = V4L2_CID_PHOTOMETRY, .index = M4MO_PHOTOMETRY_CENTER, .name = "Center"},
	{.id = V4L2_CID_PHOTOMETRY, .index = M4MO_PHOTOMETRY_SPOT, .name = "Spot"},
	
	{.id = V4L2_CID_WDR, .index = M4MO_WDR_OFF, .name = "WDR Off"},
	{.id = V4L2_CID_WDR, .index = M4MO_WDR_ON, .name = "WDR On"},
	
	{.id = V4L2_CID_ISC, .index = M4MO_ISC_STILL_OFF, .name = "ISC Off"},
	{.id = V4L2_CID_ISC, .index = M4MO_ISC_STILL_ON, .name = "ISC On"},
	
	{.id = V4L2_CID_AEWB, .index = M4MO_AE_LOCK_AWB_LOCK, .name = "AE Lock/AWB Lock"},
	{.id = V4L2_CID_AEWB, .index = M4MO_AE_LOCK_AWB_UNLOCK, .name = "AE Lock/AWB Unlock"},
	{.id = V4L2_CID_AEWB, .index = M4MO_AE_UNLOCK_AWB_LOCK, .name = "AE Unlock/AWB Lock"},
	{.id = V4L2_CID_AEWB, .index = M4MO_AE_UNLOCK_AWB_UNLOCK, .name = "AE Unlock/AWB Unlock"},
	
	{.id = V4L2_CID_FLASH_CAPTURE, .index = M4MO_FLASH_CAPTURE_OFF, .name = "Flash Off"},
	{.id = V4L2_CID_FLASH_CAPTURE, .index = M4MO_FLASH_CAPTURE_ON, .name = "Flash On"},
	{.id = V4L2_CID_FLASH_CAPTURE, .index = M4MO_FLASH_CAPTURE_AUTO, .name = "Flash Auto"},
	{.id = V4L2_CID_FLASH_CAPTURE, .index = M4MO_FLASH_MOVIE_ON, .name = "Flash Always On"},

	{.id = V4L2_CID_BRIGHTNESS, .index = M4MO_EV_MINUS_4, .name = "Exposure -4"},
	{.id = V4L2_CID_BRIGHTNESS, .index = M4MO_EV_MINUS_3, .name = "Exposure -3"},
	{.id = V4L2_CID_BRIGHTNESS, .index = M4MO_EV_MINUS_2, .name = "Exposure -2"},
	{.id = V4L2_CID_BRIGHTNESS, .index = M4MO_EV_MINUS_1, .name = "Exposure -1"},
	{.id = V4L2_CID_BRIGHTNESS, .index = M4MO_EV_DEFAULT, .name = "Exposure +0"},
	{.id = V4L2_CID_BRIGHTNESS, .index = M4MO_EV_PLUS_1, .name = "Exposure +1"},
	{.id = V4L2_CID_BRIGHTNESS, .index = M4MO_EV_PLUS_2, .name = "Exposure +2"},
	{.id = V4L2_CID_BRIGHTNESS, .index = M4MO_EV_PLUS_3, .name = "Exposure +3"},
	{.id = V4L2_CID_BRIGHTNESS, .index = M4MO_EV_PLUS_4, .name = "Exposure +4"},
	
	{.id = V4L2_CID_CONTRAST, .index = M4MO_CONTRAST_MINUS_3, .name = "Contrast -3"},
	{.id = V4L2_CID_CONTRAST, .index = M4MO_CONTRAST_MINUS_2, .name = "Contrast -2"},
	{.id = V4L2_CID_CONTRAST, .index = M4MO_CONTRAST_MINUS_1, .name = "Contrast -1"},
	{.id = V4L2_CID_CONTRAST, .index = M4MO_CONTRAST_DEFAULT, .name = "Contrast +0"},
	{.id = V4L2_CID_CONTRAST, .index = M4MO_CONTRAST_PLUS_1, .name = "Contrast +1"},
	{.id = V4L2_CID_CONTRAST, .index = M4MO_CONTRAST_PLUS_2, .name = "Contrast +2"},
	{.id = V4L2_CID_CONTRAST, .index = M4MO_CONTRAST_PLUS_3, .name = "Contrast +3"},

	{.id = V4L2_CID_SATURATION, .index = M4MO_SATURATION_MINUS_3, .name = "Saturation -3"},
	{.id = V4L2_CID_SATURATION, .index = M4MO_SATURATION_MINUS_2, .name = "Saturation -2"},
	{.id = V4L2_CID_SATURATION, .index = M4MO_SATURATION_MINUS_1, .name = "Saturation -1"},
	{.id = V4L2_CID_SATURATION, .index = M4MO_SATURATION_DEFAULT, .name = "Saturation +0"},
	{.id = V4L2_CID_SATURATION, .index = M4MO_SATURATION_PLUS_1, .name = "Saturation +1"},
	{.id = V4L2_CID_SATURATION, .index = M4MO_SATURATION_PLUS_2, .name = "Saturation +2"},
	{.id = V4L2_CID_SATURATION, .index = M4MO_SATURATION_PLUS_3, .name = "Saturation +3"},

	{.id = V4L2_CID_SHARPNESS, .index = M4MO_SHARPNESS_MINUS_3, .name = "Sharpness -3"},
	{.id = V4L2_CID_SHARPNESS, .index = M4MO_SHARPNESS_MINUS_2, .name = "Sharpness -2"},
	{.id = V4L2_CID_SHARPNESS, .index = M4MO_SHARPNESS_MINUS_1, .name = "Sharpness -1"},
	{.id = V4L2_CID_SHARPNESS, .index = M4MO_SHARPNESS_DEFAULT, .name = "Sharpness +0"},
	{.id = V4L2_CID_SHARPNESS, .index = M4MO_SHARPNESS_PLUS_1, .name = "Sharpness +1"},
	{.id = V4L2_CID_SHARPNESS, .index = M4MO_SHARPNESS_PLUS_2, .name = "Sharpness +2"},
	{.id = V4L2_CID_SHARPNESS, .index = M4MO_SHARPNESS_PLUS_3, .name = "Sharpness +3"},

	{.id = V4L2_CID_EXPOSURE_AUTO, .index = 0, .name = "Auto Exposure Unlock"},
	{.id = V4L2_CID_EXPOSURE_AUTO, .index = 1, .name = "Auto Exposure Lock"},
		
	{.id = V4L2_CID_EXPOSURE, .index = 0, .name = "Exposure[-4]"},
	{.id = V4L2_CID_EXPOSURE, .index = 1, .name = "Exposure[-3]"},
	{.id = V4L2_CID_EXPOSURE, .index = 2, .name = "Exposure[-2]"},
	{.id = V4L2_CID_EXPOSURE, .index = 3, .name = "Exposure[-1]"},
	{.id = V4L2_CID_EXPOSURE, .index = 4, .name = "Exposure[0]"},
	{.id = V4L2_CID_EXPOSURE, .index = 5, .name = "Exposure[+1]"},
	{.id = V4L2_CID_EXPOSURE, .index = 6, .name = "Exposure[+2]"},
	{.id = V4L2_CID_EXPOSURE, .index = 7, .name = "Exposure[+3]"},
	{.id = V4L2_CID_EXPOSURE, .index = 8, .name = "Exposure[+4]"},

	{.id = V4L2_CID_FOCUS_MODE, .index = 0, .name = "Normal Auto Focus"},
	{.id = V4L2_CID_FOCUS_MODE, .index = 1, .name = "Macro Auto Focus"},
#if 0
	{.id = V4L2_CID_ZOOM_MODE, .index = 0, .name = "Digital Zoom"},
#endif
	{.id = V4L2_CID_ZOOM_ABSOLUTE, .index = 0, .name = "1.0X"},
	{.id = V4L2_CID_ZOOM_ABSOLUTE, .index = 1, .name = "1.3X"},
	{.id = V4L2_CID_ZOOM_ABSOLUTE, .index = 2, .name = "1.7X"},
	{.id = V4L2_CID_ZOOM_ABSOLUTE, .index = 3, .name = "2.1X"},
	{.id = V4L2_CID_ZOOM_ABSOLUTE, .index = 4, .name = "2.5X"},
	{.id = V4L2_CID_ZOOM_ABSOLUTE, .index = 5, .name = "2.8X"},
	{.id = V4L2_CID_ZOOM_ABSOLUTE, .index = 6, .name = "3.2X"},
	{.id = V4L2_CID_ZOOM_ABSOLUTE, .index = 7, .name = "3.6X"},
	{.id = V4L2_CID_ZOOM_ABSOLUTE, .index = 8, .name = "4.0X"},

	{.id = V4L2_CID_AUTO_WHITE_BALANCE, .index = 0, .name = "Auto WB Unlock"},
	{.id = V4L2_CID_AUTO_WHITE_BALANCE, .index = 1, .name = "Auto WB Lock"},
#if 0
	{.id = V4L2_CID_WHITE_BALANCE_PRESET, .index = 0, .name = "Auto WB"},
	{.id = V4L2_CID_WHITE_BALANCE_PRESET, .index = 1, .name = "Incandescent"},
	{.id = V4L2_CID_WHITE_BALANCE_PRESET, .index = 2, .name = "Fluorescent"},
	{.id = V4L2_CID_WHITE_BALANCE_PRESET, .index = 3, .name = "Daylight"},
	{.id = V4L2_CID_WHITE_BALANCE_PRESET, .index = 4, .name = "Cloudy"},
	{.id = V4L2_CID_WHITE_BALANCE_PRESET, .index = 5, .name = "Shade"},
	{.id = V4L2_CID_WHITE_BALANCE_PRESET, .index = 6, .name = "Horizon"},
#endif
};
#define NUM_M4MO_MENU ARRAY_SIZE(m4mo_menu_list)

/* list of image formats supported by m4mo sensor */
const static struct v4l2_fmtdesc m4mo_formats[] = {
	{
		.description	= "YUV422 (UYVY)",
		.pixelformat	= V4L2_PIX_FMT_UYVY,
	},
	{
		.description	= "JPEG(without header)+ JPEG",
		.pixelformat	= V4L2_PIX_FMT_JPEG,
	},
	{
		.description	= "JPEG(without header)+ YUV",
		.pixelformat	= V4L2_PIX_FMT_MJPEG,
	}
};
#define NUM_CAPTURE_FORMATS ARRAY_SIZE(m4mo_formats)

static struct i2c_driver m4mosensor_i2c_driver;
extern struct m4mo_platform_data z_m4mo_platform_data;

void print_currenttime(void)
{
	struct timeval tv;

	do_gettimeofday(&tv);
	dprintk(CAM_DBG, M4MO_MOD_NAME "Current Time(sec): %lu.%06lu\n", tv.tv_sec, tv.tv_usec);

	return;
}

irqreturn_t isp_irq_handler(int irq, void* dev_id)
{
	cam_interrupted = 1;
	wake_up_interruptible(&cam_wait);
	dprintk(CAM_DBG, M4MO_MOD_NAME "ISP IRQ HANDLER is called\n");

	return IRQ_HANDLED;
}

static int m4mo_read_category_parm(struct i2c_client *client, 
								u8 data_length, u8 category, u8 byte, u32* val)
{
	struct i2c_msg msg[1];
	unsigned char data[5];
	unsigned char recv_data[data_length + 1];
	int err, retry=0;

	if (!client->adapter)
		return -ENODEV;
	if (data_length != M4MO_8BIT && data_length != M4MO_16BIT	&& data_length != M4MO_32BIT)		
		return -EINVAL;

read_again:
	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = 5;
	msg->buf = data;

	/* high byte goes out first */
	data[0] = 0x05;
	data[1] = 0x01;
	data[2] = category;
	data[3] = byte;
	data[4] = data_length;
	
	err = i2c_transfer(client->adapter, msg, 1);
	if (err >= 0) 
	{
		msg->flags = I2C_M_RD;
		msg->len = data_length + 1;
		msg->buf = recv_data; 
		
		err = i2c_transfer(client->adapter, msg, 1);
	}
	if (err >= 0) 
	{
		/* high byte comes first */		
		if (data_length == M4MO_8BIT)			
			*val = recv_data[1];		
		else if (data_length == M4MO_16BIT)			
			*val = recv_data[2] + (recv_data[1] << 8);		
		else			
			*val = recv_data[4] + (recv_data[3] << 8) + (recv_data[2] << 16) + (recv_data[1] << 24);

/*		dprintk(CAM_DBG, M4MO_MOD_NAME "read value from [category:0x%x], [Byte:0x%x] is 0x%x\n", 
			category, byte, *val);*/
		//print_currenttime();
		
		return 0;
	}

/*	dprintk(CAM_ERR, M4MO_MOD_NAME "read from offset [category:0x%x], [Byte:0x%x] error %d\n", 
		category, byte, err);*/
	if (retry <= M4MO_I2C_RETRY) {
		dprintk(CAM_ERR, M4MO_MOD_NAME "retry ... %d\n", retry);
		retry++;
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout(msecs_to_jiffies(20));
		goto read_again;
	}

	return err;
}

static int m4mo_write_category_parm(struct i2c_client *client,  
										u8 data_length, u8 category, u8 byte, u32 val)
{
	struct i2c_msg msg[1];
	unsigned char data[data_length+4];
	int retry = 0, err;

	if (!client->adapter)
		return -ENODEV;

	if (data_length != M4MO_8BIT && data_length != M4MO_16BIT
					&& data_length != M4MO_32BIT)
		return -EINVAL; 


again:
/*	dprintk(CAM_DBG, M4MO_MOD_NAME "write value from [category:0x%x], [Byte:0x%x] is value:0x%x\n", 
		category, byte, val);*/
	//print_currenttime();
	
	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = data_length + 4;
	msg->buf = data;

	/* high byte goes out first */
	data[0] = data_length + 4;
	data[1] = 0x02;
	data[2] = category;
	data[3] = byte;
	if (data_length == M4MO_8BIT)
		data[4] = (u8) (val & 0xff);
	else if (data_length == M4MO_16BIT) {
		data[4] = (u8) (val >> 8);
		data[5] = (u8) (val & 0xff);
	} else {
		data[4] = (u8) (val >> 24);
		data[5] = (u8) ( (val >> 16) & 0xff );
		data[6] = (u8) ( (val >> 8) & 0xff );
		data[7] = (u8) (val & 0xff);
	}
	
	err = i2c_transfer(client->adapter, msg, 1);
	if (err >= 0)
		return 0;

/*	dprintk(CAM_ERR, M4MO_MOD_NAME "wrote 0x%x to offset [category:0x%x], [Byte:0x%x] error %d", 
		val, category, byte, err);*/
	if (retry <= M4MO_I2C_RETRY) {
		dprintk(CAM_ERR, M4MO_MOD_NAME "retry ... %d", retry);
		retry++;
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout(msecs_to_jiffies(20));
		goto again;
	}
	return err;
}

static int m4mo_i2c_verify(struct i2c_client *client, u8 category, u8 byte, u32 value)
{
	u32 val, i;

	for(i= 0; i < M4MO_I2C_VERIFY_RETRY; i++) 
	{
		m4mo_read_category_parm(client, M4MO_8BIT, category, byte, &val);
		if(val == value) {
			dprintk(CAM_DBG, M4MO_MOD_NAME "[m4mo] i2c verified [c,b,v] [%02x, %02x, %02x] (try = %d)\n", 
							category, byte, value, i);
			return 0;
		}
		msleep(1);
	}

	return -EBUSY;
}


static int m4mo_read_mem(struct i2c_client *client, u16 data_length, u32 reg, u8 *val)
{
	struct i2c_msg msg[1];
	unsigned char data[8];
	unsigned char recv_data[data_length + 3];
	int err;

	if (!client->adapter)
		return -ENODEV;
	
	if (data_length < 1)
	{
		dprintk(CAM_ERR, M4MO_MOD_NAME "Data Length to read is out of range !\n");
		return -EINVAL;
	}
	
	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = 8;
	msg->buf = data;

	/* high byte goes out first */
	data[0] = 0x00;
	data[1] = 0x03;
	data[2] = (u8) (reg >> 24);
	data[3] = (u8) ( (reg >> 16) & 0xff );
	data[4] = (u8) ( (reg >> 8) & 0xff );
	data[5] = (u8) (reg & 0xff);
	data[6] = (u8) (data_length >> 8);
	data[7] = (u8) (data_length & 0xff);
	
	err = i2c_transfer(client->adapter, msg, 1);
	if (err >= 0) 
	{
		msg->flags = I2C_M_RD;
		msg->len = data_length + 3;
		msg->buf = recv_data; 

		err = i2c_transfer(client->adapter, msg, 1);
	}
	if (err >= 0) 
	{
		memcpy(val, recv_data+3, data_length);
		return 0;
	}

	dprintk(CAM_ERR, M4MO_MOD_NAME "read from offset 0x%x error %d", reg, err);
	return err;
}

static int m4mo_write_mem(struct i2c_client *client, u16 data_length, u32 reg, u8* val)
{
	struct i2c_msg msg[1];
	u8 *data;
	int retry = 0, err;

	if (!client->adapter)
		return -ENODEV;

	data = kmalloc(8 + data_length, GFP_KERNEL);

again:
	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = data_length + 8;
	msg->buf = data;

	/* high byte goes out first */
	data[0] = 0x00;
	data[1] = 0x04;
	data[2] = (u8) (reg >> 24);
	data[3] = (u8) ( (reg >> 16) & 0xff );
	data[4] = (u8) ( (reg >> 8) & 0xff );
	data[5] = (u8) (reg & 0xff);
	data[6] = (u8) (data_length >> 8);
	data[7] = (u8) (data_length & 0xff);
	memcpy(data+8 , val, data_length);

	err = i2c_transfer(client->adapter, msg, 1);
	if (err >= 0)
	{
		kfree(data);
		return 0;
	}

	dprintk(CAM_ERR, M4MO_MOD_NAME "wrote to offset 0x%x error %d", reg, err);
	if (retry <= M4MO_I2C_RETRY) {
		dprintk(CAM_ERR, M4MO_MOD_NAME "retry ... %d", retry);
		retry++;
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout(msecs_to_jiffies(20));
		goto again;
	}

	kfree(data);
	return err;
}


static int m4mo_i2c_update_firmware(void)
{
#ifdef ENABLE_FIRMWARE_UPDATE
	struct m4mo_sensor *sensor = &m4mo;
	struct i2c_client *client = sensor->i2c_client;
	
	struct device *dev = &m4mo.i2c_client->adapter->dev;
	const struct firmware * fw_entry;
	
	u8 pin1[] = { 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x7E, 0x00, 0x00, 0x00, 0x00};
    	u8 pin2[] = { 0xF3, 0x00, 0x1F, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0xFF, 0x00, 0x04, 0x05, 0xBF};
    	u8 pin3[] = { 0x02, 0x00, 0x0C, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x76, 0x00, 0x18, 0x00, 0x00};
	int i, j, retry, flash_addr, ret;
	u32 val;
	
	ret = request_firmware(&fw_entry, cam_fw_name, dev);
	if(ret != 0) {
		dprintk(CAM_ERR, M4MO_MOD_NAME "firmware request error!\n");
		return -EINVAL;
	}
	if(fw_entry->size != 1048576) {
		dprintk(CAM_ERR, M4MO_MOD_NAME "firmware size is not correct\n");
		return -EINVAL;
	}
		
	gpio_direction_output(OMAP_GPIO_CAM_RST, 0);
	mdelay(10);
	/* Release Reset */
	gpio_direction_output(OMAP_GPIO_CAM_RST, 1);
	mdelay(10);

	m4mo_write_mem(client, 0x000D, 0x50000100, pin1);
	m4mo_write_mem(client, 0x000D, 0x50000200, pin2);
	m4mo_write_mem(client, 0x000D, 0x50000300, pin3);
	
	flash_addr = 0x00100000;
	printk(KERN_INFO M4MO_MOD_NAME "Firmware Update Start!!\n");
	for(i = 0; i< 0xf0000; i+= 0x10000) 
	{
		printk(KERN_INFO M4MO_MOD_NAME "Firmware Update is processing... %d!!\n", i/0x10000);
		/*Set FLASH ROM programming address */	
		m4mo_write_category_parm(client, 4, 0x0F, 0x00, flash_addr);	
		flash_addr += 0x10000;
		mdelay(10);

		/* Erase FLASH ROM entire memory */
		m4mo_write_category_parm(client, M4MO_8BIT, 0x0F, 0x06, 0x01);				//3

		/* Confirm chip-erase has been completed */
		retry = 0;
		while(1) 
		{
			m4mo_read_category_parm(client, M4MO_8BIT, 0x0F, 0x06, &val);
			if(val == 0)	//4
				break;

			mdelay(10);
			retry++;
		}
		
		/* Set FLASH ROM programming size */
		m4mo_write_category_parm(client, M4MO_16BIT, 0x0F, 0x04, 0x0000);	
		mdelay(10);

		/* Clear m4mo internal RAM */
		m4mo_write_category_parm(client, M4MO_8BIT, 0x0F, 0x08, 0x01);
		mdelay(10);

		/* Send programmed firmware */
		for(j=0; j<0x10000; j+=0x1000)
			m4mo_write_mem(client, 0x1000, 0x68000000+j, (u8*)(fw_entry->data + i + j));

		/* Start Programming */
		m4mo_write_category_parm(client, M4MO_8BIT, 0x0F, 0x07, 0x01);				

		/* Confirm programming has been completed */
		retry = 0;
		while(1) 
		{
			m4mo_read_category_parm(client, M4MO_8BIT, 0x0F, 0x07, &val);
			if(val == 0)			//check programming process
				break;

			mdelay(10);
			retry++;
		}
		if(( i/0x10000) % 2)
			fw_update_status += 7;
		else
			fw_update_status += 6;
	}
	printk(KERN_INFO M4MO_MOD_NAME "Firmware Update Success!!\n");
	release_firmware(fw_entry);
	
	fw_update_status = 100;

    return 0;
#else
    printk(KERN_INFO M4MO_MOD_NAME "Firmware update disabled!!\n");
    return 0;
#endif
}

static int m4mo_i2c_dump_firmware(void)
{
#ifdef ENABLE_FIRMWARE_DUMP
	struct m4mo_sensor *sensor = &m4mo;
	struct i2c_client *client = sensor->i2c_client;
	
	u8 pin1[] = { 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x7E, 0x00, 0x00, 0x00, 0x00};
    	u8 pin2[] = { 0xF3, 0x00, 0x1F, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0xFF, 0x00, 0x04, 0x05, 0xBF};
    	u8 pin3[] = { 0x02, 0x00, 0x0C, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x76, 0x00, 0x18, 0x00, 0x00};
	int i, j, flash_addr, err = 0;
	u8* readdata;
	int fd;
	loff_t pos = 0;
	struct file *file = NULL;
	mm_segment_t old_fs = get_fs ();
	

	readdata = kmalloc(0x1000, GFP_KERNEL);

	set_fs (KERNEL_DS);

	fd = sys_open ("/tmp/cam_fw.bin", O_WRONLY | O_CREAT | O_TRUNC , 0644);
	if(fd < 0 )
	{
		err = -EFAULT;
		goto DUMP_ERR;
	}
	file = fget (fd);
	if(!file)
	{
		err = -EFAULT;
		goto DUMP_ERR;
	}
		
	gpio_direction_output(OMAP_GPIO_CAM_RST, 0);
	mdelay(10);
	/* Release Reset */
	gpio_direction_output(OMAP_GPIO_CAM_RST, 1);
	mdelay(10);

	m4mo_write_mem(client, 0x000D, 0x50000100, pin1);
	m4mo_write_mem(client, 0x000D, 0x50000200, pin2);
	m4mo_write_mem(client, 0x000D, 0x50000300, pin3);

	printk(KERN_INFO M4MO_MOD_NAME "Firmware Dump Start!!\n");
	flash_addr = 0x00100000;
	for(i = 0; i< 0x100000; i+= 0x10000) 
	{
		printk(KERN_INFO M4MO_MOD_NAME "Firmware Dump is processing... %d!!\n", i/0x10000);
		/*Set FLASH ROM read address and size; read data*/	
		for(j=0; j<0x10000; j+=0x1000)
		{
			pos = i + j;
			m4mo_read_mem(client, 0x1000, flash_addr + i + j, readdata);
			vfs_write (file, readdata, 0x1000, &pos);
		}
		fw_dump_status += 6;
	}
	
	printk(KERN_INFO M4MO_MOD_NAME "Firmware Dump End!!\n");

DUMP_ERR:
	if(file)
		fput (file);
	if(fd)
		sys_close (fd);
      set_fs (old_fs);
	kfree(readdata);  

	fw_dump_status = 100;
    return err;
#else
    printk(KERN_INFO M4MO_MOD_NAME "Firmware dump disabled!!\n");
    return 0;
#endif
}

static int m4mo_get_mode(void)
{
	struct m4mo_sensor *sensor = &m4mo;
	struct i2c_client *client = sensor->i2c_client;
	
	int err, mode;
	u32 val;

	err = m4mo_read_category_parm(client, M4MO_8BIT, 0x00, 0x0B, &val);
	if(err)
	{
		dprintk(CAM_ERR, M4MO_MOD_NAME "Can not read I2C command\n");
		return -EPERM;
	}

	mode = val;

	return mode;
}
	
 
static int m4mo_set_mode_common(u32 mode )
{
	struct m4mo_sensor *sensor = &m4mo;
	struct i2c_client *client = sensor->i2c_client;
	
	int err = 0;

	m4mo_write_category_parm(client, M4MO_8BIT, 0x00, 0x0B, mode);

	if(mode != M4MO_MONITOR_MODE)
	{
	err = m4mo_i2c_verify(client, 0x00, 0x0B, mode);
	if(err)
	{
		dprintk(CAM_ERR, M4MO_MOD_NAME "Can not Set Sensor Mode\n");
		return err;
	}
	}

	//msleep(10);

	return err;
}
static int m4mo_set_mode(u32 mode )
{	
	int org_value;
	int err = -EINVAL;
	
	org_value = m4mo_get_mode();

	switch(org_value)
	{
		case M4MO_PARMSET_MODE :
			if(mode == M4MO_PARMSET_MODE)
			{
				return 0;
			}
			else if(mode == M4MO_MONITOR_MODE)
			{
				if((err = m4mo_set_mode_common(M4MO_MONITOR_MODE)))
					return err;
			}
			else if(mode == M4MO_STILLCAP_MODE)
			{
				if((err = m4mo_set_mode_common(M4MO_MONITOR_MODE)))
					return err;
				if((err = m4mo_set_mode_common(M4MO_STILLCAP_MODE)))
					return err;
			}
			else
			{
				dprintk(CAM_ERR, M4MO_MOD_NAME "Requested Sensor Mode is incorrect!\n");
				return -EINVAL;
			}

			break;

		case M4MO_MONITOR_MODE :
			if(mode == M4MO_PARMSET_MODE)
			{
				if((err = m4mo_set_mode_common(M4MO_PARMSET_MODE)))
					return err;
			}
			else if(mode == M4MO_MONITOR_MODE)
			{
				return 0;
			}
			else if(mode == M4MO_STILLCAP_MODE)
			{
				if((err = m4mo_set_mode_common(M4MO_STILLCAP_MODE)))
					return err;
			}
			else
			{
				dprintk(CAM_ERR, M4MO_MOD_NAME "Requested Sensor Mode is incorrect!\n");
				return -EINVAL;
			}

			break;

		case M4MO_STILLCAP_MODE :
			if(mode == M4MO_PARMSET_MODE)
			{
				if((err = m4mo_set_mode_common(M4MO_MONITOR_MODE)))
					return err;
				if((err = m4mo_set_mode_common(M4MO_PARMSET_MODE)))
					return err;
			}
			else if(mode == M4MO_MONITOR_MODE)
			{
				if((err = m4mo_set_mode_common(M4MO_MONITOR_MODE)))
					return err;
			}
			else if(mode == M4MO_STILLCAP_MODE)
			{
				return 0;
			}
			else
			{
				dprintk(CAM_ERR, M4MO_MOD_NAME "Requested Sensor Mode is incorrect!\n");
				return -EINVAL;
			}

			break;
	}

	return err;
}

static int  m4mo_set_preview(void)
{
    struct m4mo_sensor *sensor = &m4mo;
    struct i2c_client *client = sensor->i2c_client;
      
    dprintk(CAM_DBG, M4MO_MOD_NAME "%s is called...\n", __func__);

    m4mo_pre_state = m4mo_curr_state;
    m4mo_curr_state = M4MO_STATE_PREVIEW;   
    
    //m4mo_set_mode(M4MO_PARMSET_MODE );
 
     
    /*080917[paladin] Set the fps to the sensor. @LDK@*/
    m4mo_write_category_parm(client, M4MO_8BIT, 0x01, 0x02, sensor->fps);
    /*080917[paladin] Set the Monitor Size to the sensor. @LDK@*/
    m4mo_write_category_parm(client, M4MO_8BIT, 0x01, 0x01, sensor->preview_size);
    
//    m4mo_set_mode(M4MO_MONITOR_MODE );

return 0;
}

void m4mo_dump(void)
{
	struct m4mo_sensor *sensor = &m4mo;
	struct i2c_client *client = sensor->i2c_client;
    u8 category, reg;
    u32 val;
    const u8 NUMREGS=0x30;
    const u8 NUMCATS=0x0c;

    printk("\nM4MO DUMP >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
    printk("\nreg:           ");
    for(reg=0; reg<NUMREGS; reg++)
        printk("0x%02x, ", reg);
    printk("\n---------------");  
    for(reg=0; reg<NUMREGS; reg++)
        printk("------"); 
        
    for(category=0; category<=NUMCATS; category++)
    {
        printk("\ncategory[0x%02x] ", category);
        for(reg=0; reg<NUMREGS; reg++)
        {
            m4mo_read_category_parm(client, M4MO_8BIT, category, reg, &val); 
            printk("0x%02x, ", val);
        }
    }
    printk("\nM4MO DUMP <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
}

static int  m4mo_set_capture(int pixelformat)
{
    struct m4mo_sensor *sensor = &m4mo;
    struct i2c_client *client = sensor->i2c_client;
    unsigned char format_byte = 0x00;   //YUV422
   	u32 val; 
    
    dprintk(CAM_DBG, M4MO_MOD_NAME "%s is called...\n", __func__);

    m4mo_pre_state = m4mo_curr_state;
    m4mo_curr_state = M4MO_STATE_CAPTURE;   
    
    //m4mo_set_mode(M4MO_PARMSET_MODE );

    /*080917[paladin] Set the fps to the sensor. @LDK@*/
    m4mo_write_category_parm(client, M4MO_8BIT, 0x01, 0x02, sensor->fps);
    
    if(pixelformat == V4L2_PIX_FMT_JPEG) 
    {
        format_byte = 0x04;
        dprintk(CAM_DBG, M4MO_MOD_NAME "STILL format : JPEG + JPEG Thumnail\n");
    } 
    else if(pixelformat == V4L2_PIX_FMT_MJPEG) 
    {
        format_byte = 0x05;
        dprintk(CAM_DBG, M4MO_MOD_NAME "STILL format : JPEG + YUV Thumnail\n");
    } 
    else 
        dprintk(CAM_DBG, M4MO_MOD_NAME "STILL format : YUV422\n");

    // set capture format
    m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x00, format_byte); 
     
    /* Set Capture Image Size */
    m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x04, sensor->capture_size); 
    
    
    
    m4mo_read_category_parm(client, M4MO_8BIT, 0x01, 0x02, &val); 
    dprintk(CAM_DBG, M4MO_MOD_NAME "verify fps=0x%02x\n", val);
    m4mo_read_category_parm(client, M4MO_8BIT, 0x02, 0x00, &val); 
    dprintk(CAM_DBG, M4MO_MOD_NAME "verify format=0x%02x\n", val);
    m4mo_read_category_parm(client, M4MO_8BIT, 0x02, 0x04, &val); 
    dprintk(CAM_DBG, M4MO_MOD_NAME "verify capture size = 0x%02x\n", val);


    //m4mo_set_jpeg_quality(M4MO_JPEG_SUPERFINE);
    //m4mo_set_thumbnail_size(M4MO_THUMB_QVGA_SIZE);

    //m4mo_dump();
    m4mo_start_capture();

//    m4mo_set_mode(M4MO_STILLCAP_MODE );
    return 0;
}

static int m4mo_configure(void)
{
    struct m4mo_sensor *sensor = &m4mo;
	struct i2c_client *client = sensor->i2c_client;
    int err;
	/* Request GPIO IRQ */
	err = request_irq(OMAP_GPIO_IRQ(OMAP_GPIO_ISP_INT), 
						isp_irq_handler, IRQ_TYPE_EDGE_RISING, M4MO_DRIVER_NAME, &m4mo);
	if(err){
		dprintk(CAM_ERR, M4MO_MOD_NAME "Failed to request IRQ, error: %d\n", err);
		return err;
	}

	/* Parameter Setting Mode */
	err = m4mo_set_mode(M4MO_PARMSET_MODE);
	if(err)
	{
		dprintk(CAM_ERR, M4MO_MOD_NAME "Can not set operation mode\n");
		return err;
	}
#if 1
// LIMO
// --- ??????
	m4mo_write_category_parm(client, M4MO_8BIT, 0x0B, 0x08, 0x8);
	m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x06, 0x02);
    
/* VSYNC x HSYNC => 1024 x 2048 */
	m4mo_write_category_parm(client, M4MO_8BIT, 0x01, 0x18, 0x04); 
	m4mo_write_category_parm(client, M4MO_8BIT, 0x01, 0x19, 0x00); 
    
#endif     

#if 0  //(CONFIG_NOWPLUS_REV < CONFIG_NOWPLUS_REV03_N02)  
    /* VSYNC x HSYNC => 1536 x 1200 */
    m4mo_write_category_parm(client, M4MO_8BIT, 0x01, 0x18, 0x06); 
    m4mo_write_category_parm(client, M4MO_8BIT, 0x01, 0x19, 0x00); 
#endif

#if 0   //(CONFIG_NOWPLUS_REV < CONFIG_NOWPLUS_REV02_N01)    
    /* Set vertical & hirizontal flip */
    m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x05, 0x00);
    m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x06, 0x00);
    m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x07, 0x00);
    m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x08, 0x00);
#endif
   
    return 0;   
}
  
static int m4mo_detect(void)
{
    struct m4mo_sensor *sensor = &m4mo;
    struct i2c_client *client = sensor->i2c_client;
    
    u32 fvh = 0, fvl = 0;
    int err;

    /* Start Camera Program */
    err = m4mo_write_category_parm(client, M4MO_8BIT, 0x0F, 0x12, 0x01);
    if(err)
    {
        dprintk(CAM_DBG, M4MO_MOD_NAME " There is no Camera Module!!!\n");
        return -ENODEV;
    }
    msleep(100);

    /* Read Firmware Version */
    err = m4mo_read_category_parm(client, M4MO_8BIT, 0x00, 0x01, &fvh);
    m4mo_read_category_parm(client, M4MO_8BIT, 0x00, 0x02, &fvl);

    if(fvh == 0x0 && fvl == 0x0)
        return -ENODEV;
    else
    {
        dprintk(CAM_DBG, M4MO_MOD_NAME " Sensor is Detected!!!\n");
        dprintk(CAM_DBG, M4MO_MOD_NAME " Fimrware Version: 0x%02x 0x%02x\n", fvh, fvl);
        return 0;
    }
}

static int m4mo_read_interrupt(void)
{
	struct m4mo_sensor *sensor = &m4mo;
	struct i2c_client *client = sensor->i2c_client;
	
	int int_value = 0;

	if(cam_interrupted != 1) {
		dprintk(CAM_ERR, M4MO_MOD_NAME "wait interrupt error.\n");
		return -EPERM;
	}

	m4mo_read_category_parm(client, M4MO_8BIT, 0x00, 0x10, &int_value);
	dprintk(CAM_DBG, M4MO_MOD_NAME "Interrupt : 0x%02x.\n", int_value);

	cam_interrupted = 0;

	return int_value;
}

static int m4mo_set_effect_gamma(s32 value)
{
	struct m4mo_sensor *sensor = &m4mo;
	struct i2c_client *client = sensor->i2c_client;

	int rval, orgmode;
	u32 readval;

	/* Read Color Effect ON/OFF */
	m4mo_read_category_parm(client, M4MO_8BIT, 0x02, 0x0B, &readval);
	/* If Color Effect is on, turn it off */
	if(readval)
		m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x0B, 0x00); 
	/* Check Original Mode */
	orgmode =m4mo_get_mode();
	/* Set Parameter Mode */
	rval = m4mo_set_mode(M4MO_PARMSET_MODE);
	if(rval)
	{
		dprintk(CAM_ERR, M4MO_MOD_NAME "Can not set operation mode\n");
		return rval;
	}
	
	switch(value)
	{
		case M4MO_EFFECT_NEGATIVE:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x01, 0x0B, 0x01);
			break;
		case M4MO_EFFECT_SOLARIZATION1:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x01, 0x0B, 0x02);
			break;
		case M4MO_EFFECT_SOLARIZATION2:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x01, 0x0B, 0x03);
			break;
		case M4MO_EFFECT_SOLARIZATION3:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x01, 0x0B, 0x04);
			break;
		case M4MO_EFFECT_SOLARIZATION4:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x01, 0x0B, 0x05);
			break;
		case M4MO_EFFECT_EMBOSS:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x01, 0x0B, 0x06);
			break;
		case M4MO_EFFECT_OUTLINE:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x01, 0x0B, 0x07);
			break;
		case M4MO_EFFECT_AQUA:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x01, 0x0B, 0x08);
			break;
		default:
			break;	
	}

	/* Set Monitor Mode */
	if(orgmode == M4MO_MONITOR_MODE)
	{
	rval = m4mo_set_mode(M4MO_MONITOR_MODE);
	if(rval)
	{
		dprintk(CAM_ERR, M4MO_MOD_NAME "Can not set operation mode\n");
		return rval;
	}
	}
	return 0;
}

static int m4mo_set_effect_color(s32 value)
{
	struct m4mo_sensor *sensor = &m4mo;
	struct i2c_client *client = sensor->i2c_client;

	int rval, orgmode;
	u32 readval;

	/* Read Gamma Effect ON/OFF */
	m4mo_read_category_parm(client, M4MO_8BIT, 0x01, 0x0B, &readval);
	/* If Gamma Effect is on, turn it off */
	if(readval)
	{
		orgmode = m4mo_get_mode();
		rval = m4mo_set_mode(M4MO_PARMSET_MODE);
		if(rval)
		{
			dprintk(CAM_ERR, M4MO_MOD_NAME "Can not set operation mode\n");
			return rval;
		}
		m4mo_write_category_parm(client, M4MO_8BIT, 0x01, 0x0B, 0x00); 

		if(orgmode == M4MO_MONITOR_MODE)
		{
		rval = m4mo_set_mode(M4MO_MONITOR_MODE);
		if(rval)
		{
			dprintk(CAM_ERR, M4MO_MOD_NAME "Can not set operation mode\n");
			return rval;
		}
	}
	}

	switch(value)
	{
		case M4MO_EFFECT_OFF:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x0B, 0x00);
			break;
		case M4MO_EFFECT_SEPIA:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x0B, 0x01); 
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x09, 0xD8);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x0A, 0x18);
			break;
		case M4MO_EFFECT_GRAY:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x0B, 0x01); 
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x09, 0x00);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x0A, 0x00);
			break;
		case M4MO_EFFECT_RED:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x0B, 0x01); 
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x09, 0x00);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x0A, 0x6B);
			break;
		case M4MO_EFFECT_GREEN:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x0B, 0x01); 
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x09, 0xE0);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x0A, 0xE0);
			break;
		case M4MO_EFFECT_BLUE:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x0B, 0x01); 
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x09, 0x40);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x0A, 0x00);
			break;
		case M4MO_EFFECT_PINK:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x0B, 0x01); 
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x09, 0x20);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x0A, 0x40);
			break;
		case M4MO_EFFECT_YELLOW:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x0B, 0x01); 
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x09, 0x80);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x0A, 0x15);
			break;
		case M4MO_EFFECT_PURPLE:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x0B, 0x01); 
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x09, 0x50);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x0A, 0x20);
			break;
		case M4MO_EFFECT_ANTIQUE:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x0B, 0x01); 
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x09, 0xD0);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x0A, 0x30);
			break;	
		default:
			break;
	}
		
	return 0;
}

static int m4mo_set_effect(s32 value)
{
	struct m4mo_sensor *sensor = &m4mo;

	int err = -EINVAL;

	if(sensor->effect == value)
		return 0;

	switch(value)
	{
		case M4MO_EFFECT_OFF:
		case M4MO_EFFECT_SEPIA:
		case M4MO_EFFECT_GRAY:
		case M4MO_EFFECT_RED:
		case M4MO_EFFECT_GREEN:
		case M4MO_EFFECT_BLUE:
		case M4MO_EFFECT_PINK:
		case M4MO_EFFECT_YELLOW:
		case M4MO_EFFECT_PURPLE:
		case M4MO_EFFECT_ANTIQUE:
			err = m4mo_set_effect_color(value);
			break;
		case M4MO_EFFECT_NEGATIVE:
		case M4MO_EFFECT_SOLARIZATION1:
		case M4MO_EFFECT_SOLARIZATION2:
		case M4MO_EFFECT_SOLARIZATION3:
		case M4MO_EFFECT_SOLARIZATION4:
		case M4MO_EFFECT_EMBOSS:
		case M4MO_EFFECT_OUTLINE:
		case M4MO_EFFECT_AQUA:
			err = m4mo_set_effect_gamma(value);
			break;
		default:
			dprintk(CAM_ERR, M4MO_MOD_NAME "[Effect]Invalid value is ordered!!!\n");
			return -EINVAL;
	}

	if(!err)
		sensor->effect = value;
	
	return err;
}

static int m4mo_set_iso(s32 value)
{
	struct m4mo_sensor *sensor = &m4mo;
	struct i2c_client *client = sensor->i2c_client;

	if(sensor->iso == value)
		return 0;

	switch(value)
	{
		case M4MO_ISO_AUTO:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x05, 0x00);
			break;
		case M4MO_ISO_50:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x05, 0x01);
			break;
		case M4MO_ISO_100:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x05, 0x02);
			break;
		case M4MO_ISO_200:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x05, 0x03);
			break;
		case M4MO_ISO_400:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x05, 0x04);
			break;
		case M4MO_ISO_800:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x05, 0x05);
			break;
		case M4MO_ISO_1000:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x05, 0x06);
			break;
		default:
			dprintk(CAM_ERR, M4MO_MOD_NAME "[ISO]Invalid value is ordered!!!\n");
			return -EINVAL;
	}

	sensor->iso = value;
	
	return 0;
}

static int m4mo_set_ev(s32 value)
{
	struct m4mo_sensor *sensor = &m4mo;
	struct i2c_client *client = sensor->i2c_client;

	if(sensor->ev == value)
		return 0;

	switch(value)
	{
		case M4MO_EV_MINUS_4:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x04, 0xFC);
			break;
		case M4MO_EV_MINUS_3:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x04, 0xFD);
			break;
		case M4MO_EV_MINUS_2:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x04, 0xFE);
			break;
		case M4MO_EV_MINUS_1:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x04, 0xFF);
			break;
		case M4MO_EV_DEFAULT:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x04, 0x00);
			break;
		case M4MO_EV_PLUS_1:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x04, 0x01);
			break;
		case M4MO_EV_PLUS_2:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x04, 0x02);
			break;	
		case M4MO_EV_PLUS_3:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x04, 0x03);
			break;
		case M4MO_EV_PLUS_4:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x04, 0x04);
			break;
		default:
			dprintk(CAM_ERR, M4MO_MOD_NAME "[EV]Invalid value is ordered!!!\n");
			return -EINVAL;
	}

	sensor->ev = value;

	return 0;
}

static int m4mo_set_saturation(s32 value)
{
	struct m4mo_sensor *sensor = &m4mo;
	struct i2c_client *client = sensor->i2c_client;

	if(sensor->saturation== value)
		return 0;

	switch(value)
	{
		case M4MO_SATURATION_MINUS_3:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x0F, 0x1C);
			/* Enable Control*/
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x10, 0x00);
			break;
		case M4MO_SATURATION_MINUS_2:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x0F, 0x3E);
			/* Enable Control*/
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x10, 0x00);
			break;
		case M4MO_SATURATION_MINUS_1:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x0F, 0x5F);
			/* Enable Control*/
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x10, 0x00);
			break;
		case M4MO_SATURATION_DEFAULT:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x0F, 0x80);
			/* Enable Control*/
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x10, 0x01);
			break;
		case M4MO_SATURATION_PLUS_1:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x0F, 0xA1);
			/* Enable Control*/
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x10, 0x00);
			break;
		case M4MO_SATURATION_PLUS_2:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x0F, 0xC2);
			/* Enable Control*/
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x10, 0x00);
			break;
		case M4MO_SATURATION_PLUS_3:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x0F, 0xE4);
			/* Enable Control*/
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x10, 0x00);
			break;
		default:
			dprintk(CAM_ERR, M4MO_MOD_NAME "[Saturation]Invalid value is ordered!!!\n");
			return -EINVAL;
	}

	sensor->saturation = value;

	return 0;
}

static int m4mo_set_sharpness(s32 value)
{
	struct m4mo_sensor *sensor = &m4mo;
	struct i2c_client *client = sensor->i2c_client;

	if(sensor->sharpness== value)
		return 0;

	switch(value)
	{
		case M4MO_SHARPNESS_MINUS_3:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x11, 0x1C);
			/* Enable Control*/
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x12, 0x00);
			break;
		case M4MO_SHARPNESS_MINUS_2:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x11, 0x3E);
			/* Enable Control*/
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x12, 0x00);
			break;
		case M4MO_SHARPNESS_MINUS_1:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x11, 0x5F);
			/* Enable Control*/
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x12, 0x00);
			break;
		case M4MO_SHARPNESS_DEFAULT:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x11, 0x80);
			/* Enable Control*/
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x12, 0x01);
			break;
		case M4MO_SHARPNESS_PLUS_1:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x11, 0xA1);
			/* Enable Control*/
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x12, 0x00);
			break;
		case M4MO_SHARPNESS_PLUS_2:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x11, 0xC2);
			/* Enable Control*/
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x12, 0x00);
			break;
		case M4MO_SHARPNESS_PLUS_3:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x11, 0xE4);
			/* Enable Control*/
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x12, 0x00);
			break;
		default:
			dprintk(CAM_ERR, M4MO_MOD_NAME "[Sharpness]Invalid value is ordered!!!\n");
			return -EINVAL;
	}
	
	sensor->sharpness= value;

	return 0;
}

static int m4mo_set_contrast(s32 value)
{
	struct m4mo_sensor *sensor = &m4mo;
	struct i2c_client *client = sensor->i2c_client;

	int rval, orgmode;

	if(sensor->contrast== value)
		return 0;

	orgmode = m4mo_get_mode();
	rval = m4mo_set_mode(M4MO_PARMSET_MODE);
	if(rval)
	{
		dprintk(CAM_ERR, M4MO_MOD_NAME "Can not set operation mode\n");
		return rval;
	}

	switch(value)
	{
		case M4MO_CONTRAST_MINUS_3:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x01, 0x04, 0x1C);
			break;
		case M4MO_CONTRAST_MINUS_2:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x01, 0x04, 0x3E);
			break;
		case M4MO_CONTRAST_MINUS_1:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x01, 0x04, 0x5F);
			break;
		case M4MO_CONTRAST_DEFAULT:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x01, 0x04, 0x80);
			break;
		case M4MO_CONTRAST_PLUS_1:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x01, 0x04, 0xA1);
			break;
		case M4MO_CONTRAST_PLUS_2:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x01, 0x04, 0xC2);
			break;
		case M4MO_CONTRAST_PLUS_3:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x01, 0x04, 0xE4);
			break;
		default:
			dprintk(CAM_ERR, M4MO_MOD_NAME "[Contrast]Invalid value is ordered!!!\n");
			return -EINVAL;
	}

	sensor->contrast= value;

	if(orgmode == M4MO_MONITOR_MODE)
	{
	rval = m4mo_set_mode(M4MO_MONITOR_MODE);
	if(rval)
	{
		dprintk(CAM_ERR, M4MO_MOD_NAME "Can not set operation mode\n");
		return rval;
	}
	}
	
	return 0;
}

static int m4mo_set_wb(s32 value)
{
	struct m4mo_sensor *sensor = &m4mo;
	struct i2c_client *client = sensor->i2c_client;

	if(sensor->wb == value)
		return 0;

	switch(value)
	{
		case M4MO_WB_AUTO:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x02, 0x01);
			break;
		case M4MO_WB_INCANDESCENT:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x02, 0x02);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x03, 0x01);
			break;
		case M4MO_WB_FLUORESCENT:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x02, 0x02);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x03, 0x02);
			break;
		case M4MO_WB_DAYLIGHT:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x02, 0x02);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x03, 0x03);
			break;
		case M4MO_WB_CLOUDY:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x02, 0x02);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x03, 0x04);
			break;
		case M4MO_WB_SHADE:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x02, 0x02);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x03, 0x05);
			break;
		case M4MO_WB_HORIZON:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x02, 0x02);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x03, 0x06);
			break;
		default:
			dprintk(CAM_ERR, M4MO_MOD_NAME "[WB]Invalid value is ordered!!!\n");
			return -EINVAL;
	}

	sensor->wb= value;
	
	return 0;
}

static int m4mo_set_aewb(s32 value)
{
	struct m4mo_sensor *sensor = &m4mo;
	struct i2c_client *client = sensor->i2c_client;

	if(sensor->aewb == value)
		return 0;

	switch(value)
	{
		case M4MO_AE_LOCK_AWB_LOCK:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x00, 0x01);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x00, 0x01);
			break;
		case M4MO_AE_LOCK_AWB_UNLOCK:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x00, 0x01);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x00, 0x00);
			break;
		case M4MO_AE_UNLOCK_AWB_LOCK:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x00, 0x00);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x00, 0x01);
			break;
		case M4MO_AE_UNLOCK_AWB_UNLOCK:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x00, 0x00);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x00, 0x00);
			break;
		default:
			dprintk(CAM_ERR, M4MO_MOD_NAME "[AEWB]Invalid value is ordered!!!\n");
			return -EINVAL;
	}

	sensor->aewb= value;

	return 0;
}

static int m4mo_set_photometry(s32 value)
{
	struct m4mo_sensor *sensor = &m4mo;
	struct i2c_client *client = sensor->i2c_client;

	if(sensor->photometry == value)
		return 0;

	switch(value)
	{
		case M4MO_PHOTOMETRY_AVERAGE:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x01, 0x01);
			break;
		case M4MO_PHOTOMETRY_CENTER:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x01, 0x03);
			break;
		case M4MO_PHOTOMETRY_SPOT:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x01, 0x05);
			break;
		default:
			dprintk(CAM_ERR, M4MO_MOD_NAME "[Photometry]Invalid value is ordered!!!\n");
			return -EINVAL;
	}

	sensor->photometry= value;
	
	return 0;
}

static int m4mo_set_face_detection(s32 value)
{
	struct m4mo_sensor *sensor = &m4mo;
	struct i2c_client *client = sensor->i2c_client;

	int rval, orgmode;

	if(sensor->face_detection== value)
		return 0;

	orgmode = m4mo_get_mode();
	rval = m4mo_set_mode(M4MO_PARMSET_MODE);
	if(rval)
	{
		dprintk(CAM_ERR, M4MO_MOD_NAME "Can not set operation mode\n");
		return rval;
	}

	switch(value)
	{
		case M4MO_FACE_DETECTION_OFF:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x09, 0x01, 0x00);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x09, 0x00, 0x00);
			break;
		case M4MO_FACE_DETECTION_ON:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x09, 0x01, 0x01);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x09, 0x00, 0x11);
			break;
		default:
			dprintk(CAM_ERR, M4MO_MOD_NAME "[Face Detection]Invalid value is ordered!!!\n");
			return -EINVAL;
	}

	sensor->face_detection= value;

	if(orgmode == M4MO_MONITOR_MODE)
	{
		rval = m4mo_set_mode(M4MO_MONITOR_MODE);
		if(rval)
		{
			dprintk(CAM_ERR, M4MO_MOD_NAME "Can not set operation mode\n");
			return rval;
		}
	}

	return 0;
}


static int m4mo_set_jpeg_quality(s32 value)
{
	struct m4mo_sensor *sensor = &m4mo;
	struct i2c_client *client = sensor->i2c_client;

	if(sensor->jpeg_quality == value)
		return 0;
    dprintk(CAM_ERR, M4MO_MOD_NAME "[JPEG Quality] set value=%d!!!\n", value);
    
	switch(value)
	{
		case M4MO_JPEG_SUPERFINE:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0C, 0x08, 0x62);
			break;
		case M4MO_JPEG_FINE:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0C, 0x08, 0x55);
			break;
		case M4MO_JPEG_NORMAL:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0C, 0x08, 0x4B);
			break;
		case M4MO_JPEG_ECONOMY:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0C, 0x08, 0x41);
			break;
		default:
			dprintk(CAM_ERR, M4MO_MOD_NAME "[JPEG Quality]Invalid value is ordered!!!\n");
			return -EINVAL;
	}

	sensor->jpeg_quality= value;

	return 0;
}

static int m4mo_set_wdr(s32 value)
{
	struct m4mo_sensor *sensor = &m4mo;
	struct i2c_client *client = sensor->i2c_client;
	/* WDR & ISC are orthogonal!*/
	
	if(sensor->wdr == value)
		return 0;
	
	switch(value)
	{
		case M4MO_WDR_OFF:
			if(sensor->isc == M4MO_ISC_STILL_ON)
				m4mo_write_category_parm(client, M4MO_8BIT, 0x0C, 0x00, 0x06);
			else if(sensor->isc == M4MO_ISC_STILL_AUTO)
				m4mo_write_category_parm(client, M4MO_8BIT, 0x0C, 0x00, 0x02);
			else
				m4mo_write_category_parm(client, M4MO_8BIT, 0x0C, 0x00, 0x00);
			break;
		case M4MO_WDR_ON:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0C, 0x00, 0x04);
			sensor->isc = M4MO_ISC_STILL_OFF;
			break;
		default:
			dprintk(CAM_ERR, M4MO_MOD_NAME "[WDR]Invalid value is ordered!!!\n");
			return -EINVAL;
	}

	sensor->wdr= value;

	return 0;
}

static int m4mo_set_isc(s32 value)
{
	struct m4mo_sensor *sensor = &m4mo;
	struct i2c_client *client = sensor->i2c_client;
	
	if(sensor->isc == value)
		return 0;
	/* WDR & ISC are orthogonal!*/
	switch(value)
	{
		case M4MO_ISC_STILL_OFF:
			if(sensor->wdr == M4MO_WDR_ON)
				m4mo_write_category_parm(client, M4MO_8BIT, 0x0C, 0x00, 0x04);
			else
				m4mo_write_category_parm(client, M4MO_8BIT, 0x0C, 0x00, 0x00);
			break;
		case M4MO_ISC_STILL_ON:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0C, 0x00, 0x06);
			sensor->wdr = M4MO_WDR_OFF;
			break;
		case M4MO_ISC_STILL_AUTO:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0C, 0x00, 0x02);
			sensor->wdr = M4MO_WDR_OFF;
			break;
		default:
			dprintk(CAM_ERR, M4MO_MOD_NAME "[ISC]Invalid value is ordered!!!\n");
			return -EINVAL;
	}

	sensor->isc= value;

	return 0;
}

static int m4mo_set_flash_capture(s32 value)
{
	struct m4mo_sensor *sensor = &m4mo;
	struct i2c_client *client = sensor->i2c_client;
    static int is_torch = 0;
    
	if(sensor->flash_capture== value)
		return 0;
	/* WDR & ISC are orthogonal!*/
	switch(value)
	{
		case M4MO_FLASH_CAPTURE_OFF:
            if(is_torch)
            {
                dprintk(CAM_ERR, M4MO_MOD_NAME "torch disabled \n");
                is_torch = 0;
            }
            m4mo_write_category_parm(client, M4MO_8BIT, 0x0B, 0x00, 0x00);
            m4mo_write_category_parm(client, M4MO_8BIT, 0x0B, 0x01, 0x00);
            if(sensor->scene == M4MO_SCENE_BACKLIGHT)
                m4mo_set_photometry(M4MO_PHOTOMETRY_SPOT);
			break;
		case M4MO_FLASH_CAPTURE_ON:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0B, 0x00, 0x01);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0B, 0x01, 0x01);
			if(sensor->scene == M4MO_SCENE_BACKLIGHT)
				m4mo_set_photometry(M4MO_PHOTOMETRY_CENTER);
			break;
		case M4MO_FLASH_CAPTURE_AUTO:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0B, 0x00, 0x02);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0B, 0x01, 0x02);
			break;
        case M4MO_FLASH_MOVIE_ON:
        	m4mo_write_category_parm(client, M4MO_8BIT, 0x0B, 0x00, 0x03);
            is_torch = 1;
            dprintk(CAM_ERR, M4MO_MOD_NAME "torch enabled \n");
			break;
            
#if 0            
		case M4MO_FLASH_MOVIE_ON:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0B, 0x00, 0x03);
			break;
		case M4MO_FLASH_LEVEL_1PER2:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0B, 0x06, 1);
			break;
		case M4MO_FLASH_LEVEL_1PER4:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0B, 0x06, 2);
			break;
		case M4MO_FLASH_LEVEL_1PER6:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0B, 0x06, 3);
			break;
		case M4MO_FLASH_LEVEL_1PER8:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0B, 0x06, 4);
			break;
		case M4MO_FLASH_LEVEL_1PER10:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0B, 0x06, 5);
			break;
		case M4MO_FLASH_LEVEL_1PER12:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0B, 0x06, 6);
			break;
		case M4MO_FLASH_LEVEL_1PER14:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0B, 0x06, 7);
			break;
		case M4MO_FLASH_LEVEL_1PER16:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0B, 0x06, 8);
			break;
		case M4MO_FLASH_LEVEL_1PER18:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0B, 0x06, 9);
			break;
		case M4MO_FLASH_LEVEL_1PER20:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0B, 0x06, 10);
			break;
		case M4MO_FLASH_LEVEL_1PER22:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0B, 0x06, 11);
			break;
		case M4MO_FLASH_LEVEL_1PER24:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0B, 0x06, 12);
			break;
		case M4MO_FLASH_LEVEL_1PER26:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0B, 0x06, 13);
			break;
		case M4MO_FLASH_LEVEL_1PER28:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0B, 0x06, 14);
			break;
		case M4MO_FLASH_LEVEL_1PER30:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0B, 0x06, 15);
			break;
		case M4MO_FLASH_LEVEL_OFF:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0B, 0x06, 16);
			break;
#endif
		default:
			dprintk(CAM_ERR, M4MO_MOD_NAME "[FLASH CAPTURE]Invalid value is ordered!!!\n");
			return -EINVAL;
	}

	sensor->flash_capture = value;

	return 0;
}

static int m4mo_get_auto_focus(struct v4l2_control *vc)
{
	struct m4mo_sensor *sensor = &m4mo;
	struct i2c_client *client = sensor->i2c_client;
	
	u32 val;

	m4mo_read_category_parm(client, M4MO_8BIT, 0x0A, 0x03, &val);
	vc->value = val;
	dprintk(CAM_DBG, M4MO_MOD_NAME "AF status reading... %d \n", val);

	if(val == 1 || val == 2) 
	{	
		dprintk(CAM_DBG, M4MO_MOD_NAME "AF result = %d (1.success, 2.fail)\n", val);
	}

	return 0;
}

static int m4mo_set_auto_focus(s32 value)
{
	struct m4mo_sensor *sensor = &m4mo;
	struct i2c_client *client = sensor->i2c_client;
	
	int ret;

	dprintk(CAM_DBG, M4MO_MOD_NAME "%s: value = %d\n", __func__, value);
    
    if(m4mo_curr_state != M4MO_STATE_PREVIEW)
	{
		printk(M4MO_MOD_NAME "AF error: sensor is not preview state!!");
		//return -EINVAL;	// allo AF_STOP
	}
    
	switch(value) 
	{
		case M4MO_AF_START :
			af_used = 1;
            dprintk(CAM_DBG, M4MO_MOD_NAME "AF start.\n");
            
			/*AF VCM Driver Standby Mode OFF */
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0A, 0x00, 0x01); 
			/* AF operation start */
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0A, 0x02, 0x01); 

			sensor->af_mode = M4MO_AF_START;
			break;
		case M4MO_AF_STOP :
			af_used = 0;
            dprintk(CAM_DBG, M4MO_MOD_NAME "AF stop.\n");
            
			/* Release Auto Focus */
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0A, 0x02, 0x00); 
            
			/* Wait till the staus is changed to monitor mode */
			if(sensor->face_detection == M4MO_FACE_DETECTION_ON)
			{
				ret = m4mo_i2c_verify(client, 0x00, 0x0C, 0x04);
				if(ret)  
				{
					dprintk(CAM_DBG, M4MO_MOD_NAME "Failed to wait changing to face detection status!\n");
					return ret;
				}
			}
			else
			{
				ret = m4mo_i2c_verify(client, 0x00, 0x0C, 0x02);
				if(ret)  
				{
					dprintk(CAM_DBG, M4MO_MOD_NAME "Failed to wait changing to monitor status!\n");
					return ret;
				}
			}

			if(sensor->focus_mode == M4MO_AF_MODE_MACRO) 	// Macro off
				m4mo_write_category_parm(client, M4MO_8BIT, 0x0A, 0x10, 0x02); // Set Macro Base Position
			else
				m4mo_write_category_parm(client, M4MO_8BIT, 0x0A, 0x10, 0x01); // Set Base Position

			ret =m4mo_i2c_verify(client, 0x0A, 0x10, 0x00);
			if(ret)  
			{
				dprintk(CAM_DBG, M4MO_MOD_NAME "Failed to wait a AF stop!\n");
				return ret;
			}

			if(sensor->focus_mode == M4MO_AF_MODE_MACRO)
				m4mo_write_category_parm(client, M4MO_8BIT, 0x0A, 0x10, 0x01); // Set Base Position

			/* AF VCM Driver Standby Mode ON */
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0A, 0x00, 0x00); 

			sensor->af_mode = M4MO_AF_STOP;

			dprintk(CAM_DBG, M4MO_MOD_NAME "AF stop.\n");
			break;
		case M4MO_AF_RELEASE :
			/* Release Auto Focus */
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0A, 0x02, 0x00); 
			/* Wait till the staus is changed to monitor mode */
			if(sensor->face_detection == M4MO_FACE_DETECTION_ON)
			{
				ret = m4mo_i2c_verify(client, 0x00, 0x0C, 0x04);
				if(ret)  
				{
					dprintk(CAM_DBG, M4MO_MOD_NAME "Failed to wait changing to face detection status!\n");
					return ret;
				}
			}
			else
			{
			ret = m4mo_i2c_verify(client, 0x00, 0x0C, 0x02);
			if(ret)  
			{
				dprintk(CAM_DBG, M4MO_MOD_NAME "Failed to wait changing to monitor status!\n");
				return ret;
			}
			}

			sensor->af_mode = M4MO_AF_RELEASE;

			dprintk(CAM_DBG, M4MO_MOD_NAME "AF release\n");
			break;
	}

    return 0;
}

static int m4mo_set_scene(s32 value)
{
	struct m4mo_sensor *sensor = &m4mo;
	struct i2c_client *client = sensor->i2c_client;

	int err = 0;
	
	if(sensor->scene == value)
		return 0;
	
	switch(value)
	{
		case M4MO_SCENE_NORMAL:
			/* AF mode / Lens Position */
			err = m4mo_set_focus_mode(M4MO_AF_MODE_NORMAL);
			/* Face Detection */
			err = m4mo_set_face_detection(M4MO_FACE_DETECTION_OFF);
			/*Photometry */
			err = m4mo_set_photometry(M4MO_PHOTOMETRY_CENTER);
			/* EV-P */
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x0A, 0x00);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x0B, 0x00);
			/* EV Bias */
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x02, 0x13);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x09, 0x03);
			/* AWB */
			err = m4mo_set_wb(M4MO_WB_AUTO);
			/* Chroma Saturation */
			err = m4mo_set_saturation(M4MO_SATURATION_DEFAULT);
			/* Sharpness */
			err = m4mo_set_sharpness(M4MO_SHARPNESS_DEFAULT);
			/* Flash */
			err = m4mo_set_flash_capture(M4MO_FLASH_CAPTURE_OFF);
			/* ISO */
			sensor->iso = M4MO_ISO_AUTO;
			break;
		case M4MO_SCENE_PORTRAIT:
			/* AF mode / Lens Position */
			err = m4mo_set_focus_mode(M4MO_AF_MODE_NORMAL);
			/* Face Detection */
			err = m4mo_set_face_detection(M4MO_FACE_DETECTION_ON);
			/*Photometry */
			err = m4mo_set_photometry(M4MO_PHOTOMETRY_CENTER);
			/* EV-P */
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x0A, 0x01);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x0B, 0x01);
			/* EV Bias */
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x02, 0x13);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x09, 0x03);
			/* AWB */
			err = m4mo_set_wb(M4MO_WB_AUTO);
			/* Chroma Saturation */
			err = m4mo_set_saturation(M4MO_SATURATION_DEFAULT);
			/* Sharpness */
			err = m4mo_set_sharpness(M4MO_SHARPNESS_MINUS_1);
			/* Flash */
			err = m4mo_set_flash_capture(M4MO_FLASH_CAPTURE_OFF);
			/* ISO */
			sensor->iso = M4MO_ISO_AUTO;
			break;
		case M4MO_SCENE_LANDSCAPE:
			/* AF mode / Lens Position */
			err = m4mo_set_focus_mode(M4MO_AF_MODE_NORMAL);
			/* Face Detection */
			err = m4mo_set_face_detection(M4MO_FACE_DETECTION_OFF);
			/*Photometry */
			err = m4mo_set_photometry(M4MO_PHOTOMETRY_AVERAGE);
			/* EV-P */
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x0A, 0x02);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x0B, 0x02);
			/* EV Bias */
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x02, 0x13);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x09, 0x03);
			/* AWB */
			err = m4mo_set_wb(M4MO_WB_AUTO);
			/* Chroma Saturation */
			err = m4mo_set_saturation(M4MO_SATURATION_PLUS_1);
			/* Sharpness */
			err = m4mo_set_sharpness(M4MO_SHARPNESS_PLUS_1);
			/* Flash */
			err = m4mo_set_flash_capture(M4MO_FLASH_CAPTURE_OFF);
			/* ISO */
			sensor->iso = M4MO_ISO_AUTO;
			break;
		case M4MO_SCENE_SPORTS:
			/* AF mode / Lens Position */
			err = m4mo_set_focus_mode(M4MO_AF_MODE_NORMAL);
			/* Face Detection */
			err = m4mo_set_face_detection(M4MO_FACE_DETECTION_OFF);
			/*Photometry */
			err = m4mo_set_photometry(M4MO_PHOTOMETRY_CENTER);
			/* EV-P */
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x0A, 0x03);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x0B, 0x03);
			/* EV Bias */
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x02, 0x13);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x09, 0x03);
			/* AWB */
			err = m4mo_set_wb(M4MO_WB_AUTO);
			/* Chroma Saturation */
			err = m4mo_set_saturation(M4MO_SATURATION_DEFAULT);
			/* Sharpness */
			err = m4mo_set_sharpness(M4MO_SHARPNESS_DEFAULT);
			/* Flash */
			err = m4mo_set_flash_capture(M4MO_FLASH_CAPTURE_OFF);
			/* ISO */
			sensor->iso = M4MO_ISO_AUTO;
			break;
		case M4MO_SCENE_PARTY_INDOOR:
			/* AF mode / Lens Position */
			err = m4mo_set_focus_mode(M4MO_AF_MODE_NORMAL);
			/* Face Detection */
			err = m4mo_set_face_detection(M4MO_FACE_DETECTION_OFF);
			/*Photometry */
			err = m4mo_set_photometry(M4MO_PHOTOMETRY_CENTER);
			/* EV-P */
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x0A, 0x04);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x0B, 0x04);
			/* EV Bias */
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x02, 0x13);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x09, 0x03);
			/* AWB */
			err = m4mo_set_wb(M4MO_WB_AUTO);
			/* Chroma Saturation */
			err = m4mo_set_saturation(M4MO_SATURATION_PLUS_1);
			/* Sharpness */
			err = m4mo_set_sharpness(M4MO_SHARPNESS_DEFAULT);
			/* Flash */
			err = m4mo_set_flash_capture(M4MO_FLASH_CAPTURE_OFF);
			/* ISO */
			sensor->iso = M4MO_ISO_200;
			break;
		case M4MO_SCENE_BEACH_SNOW:
			/* AF mode / Lens Position */
			err = m4mo_set_focus_mode(M4MO_AF_MODE_NORMAL);
			/* Face Detection */
			err = m4mo_set_face_detection(M4MO_FACE_DETECTION_OFF);
			/*Photometry */
			err = m4mo_set_photometry(M4MO_PHOTOMETRY_CENTER);
			/* EV-P */
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x0A, 0x05);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x0B, 0x05);
			/* EV Bias */
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x02, 0x13);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x09, 0x04);
			/* AWB */
			err = m4mo_set_wb(M4MO_WB_AUTO);
			/* Chroma Saturation */
			err = m4mo_set_saturation(M4MO_SATURATION_PLUS_1);
			/* Sharpness */
			err = m4mo_set_sharpness(M4MO_SHARPNESS_DEFAULT);
			/* Flash */
			err = m4mo_set_flash_capture(M4MO_FLASH_CAPTURE_OFF);
			/* ISO */
			sensor->iso = M4MO_ISO_50;
			break;
		case M4MO_SCENE_SUNSET:
			/* AF mode / Lens Position */
			err = m4mo_set_focus_mode(M4MO_AF_MODE_NORMAL);
			/* Face Detection */
			err = m4mo_set_face_detection(M4MO_FACE_DETECTION_OFF);
			/*Photometry */
			err = m4mo_set_photometry(M4MO_PHOTOMETRY_CENTER);
			/* EV-P */
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x0A, 0x06);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x0B, 0x06);
			/* EV Bias */
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x02, 0x13);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x09, 0x03);
			/* AWB */
			err = m4mo_set_wb(M4MO_WB_DAYLIGHT);
			/* Chroma Saturation */
			err = m4mo_set_saturation(M4MO_SATURATION_DEFAULT);
			/* Sharpness */
			err = m4mo_set_sharpness(M4MO_SHARPNESS_DEFAULT);
			/* Flash */
			err = m4mo_set_flash_capture(M4MO_FLASH_CAPTURE_OFF);
			/* ISO */
			sensor->iso = M4MO_ISO_AUTO;
			break;
		case M4MO_SCENE_DUSK_DAWN:
			/* AF mode / Lens Position */
			err = m4mo_set_focus_mode(M4MO_AF_MODE_NORMAL);
			/* Face Detection */
			err = m4mo_set_face_detection(M4MO_FACE_DETECTION_OFF);
			/*Photometry */
			err = m4mo_set_photometry(M4MO_PHOTOMETRY_CENTER);
			/* EV-P */
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x0A, 0x07);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x0B, 0x07);
			/* EV Bias */
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x02, 0x13);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x09, 0x03);
			/* AWB */
			err = m4mo_set_wb(M4MO_WB_FLUORESCENT);
			/* Chroma Saturation */
			err = m4mo_set_saturation(M4MO_SATURATION_DEFAULT);
			/* Sharpness */
			err = m4mo_set_sharpness(M4MO_SHARPNESS_DEFAULT);
			/* Flash */
			err = m4mo_set_flash_capture(M4MO_FLASH_CAPTURE_OFF);
			/* ISO */
			sensor->iso = M4MO_ISO_AUTO;
			break;
		case M4MO_SCENE_FALL_COLOR:
			/* AF mode / Lens Position */
			err = m4mo_set_focus_mode(M4MO_AF_MODE_NORMAL);
			/* Face Detection */
			err = m4mo_set_face_detection(M4MO_FACE_DETECTION_OFF);
			/*Photometry */
			err = m4mo_set_photometry(M4MO_PHOTOMETRY_CENTER);
			/* EV-P */
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x0A, 0x08);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x0B, 0x08);
			/* EV Bias */
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x02, 0x13);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x09, 0x03);
			/* AWB */
			err = m4mo_set_wb(M4MO_WB_AUTO);
			/* Chroma Saturation */
			err = m4mo_set_saturation(M4MO_SATURATION_PLUS_2);
			/* Sharpness */
			err = m4mo_set_sharpness(M4MO_SHARPNESS_DEFAULT);
			/* Flash */
			err = m4mo_set_flash_capture(M4MO_FLASH_CAPTURE_OFF);
			/* ISO */
			sensor->iso = M4MO_ISO_AUTO;
			break;
		case M4MO_SCENE_NIGHT:
			/* AF mode / Lens Position */
			err = m4mo_set_focus_mode(M4MO_AF_MODE_NORMAL);
			/* Face Detection */
			err = m4mo_set_face_detection(M4MO_FACE_DETECTION_OFF);
			/*Photometry */
			err = m4mo_set_photometry(M4MO_PHOTOMETRY_CENTER);
			/* EV-P */
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x0A, 0x09);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x0B, 0x09);
			/* EV Bias */
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x02, 0x13);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x09, 0x03);
			/* AWB */
			err = m4mo_set_wb(M4MO_WB_AUTO);
			/* Chroma Saturation */
			err = m4mo_set_saturation(M4MO_SATURATION_DEFAULT);
			/* Sharpness */
			err = m4mo_set_sharpness(M4MO_SHARPNESS_DEFAULT);
			/* Flash */
			err = m4mo_set_flash_capture(M4MO_FLASH_CAPTURE_OFF);
			/* ISO */
			sensor->iso = M4MO_ISO_AUTO;
			break;
		case M4MO_SCENE_FIREWORK:
			/* AF mode / Lens Position */
			err = m4mo_set_focus_mode(M4MO_AF_MODE_NORMAL);
			/* Face Detection */
			err = m4mo_set_face_detection(M4MO_FACE_DETECTION_OFF);
			/*Photometry */
			err = m4mo_set_photometry(M4MO_PHOTOMETRY_CENTER);
			/* EV-P */
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x0A, 0x0A);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x0B, 0x0A);
			/* EV Bias */
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x02, 0x13);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x09, 0x03);
			/* AWB */
			err = m4mo_set_wb(M4MO_WB_AUTO);
			/* Chroma Saturation */
			err = m4mo_set_saturation(M4MO_SATURATION_DEFAULT);
			/* Sharpness */
			err = m4mo_set_sharpness(M4MO_SHARPNESS_DEFAULT);
			/* Flash */
			err = m4mo_set_flash_capture(M4MO_FLASH_CAPTURE_OFF);
			/* ISO */
			sensor->iso = M4MO_ISO_50;
			break;
		case M4MO_SCENE_TEXT:
			/* AF mode / Lens Position */
			err = m4mo_set_focus_mode(M4MO_AF_MODE_MACRO);
			/* Face Detection */
			err = m4mo_set_face_detection(M4MO_FACE_DETECTION_OFF);
			/*Photometry */
			err = m4mo_set_photometry(M4MO_PHOTOMETRY_CENTER);
			/* EV-P */
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x0A, 0x0B);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x0B, 0x0B);
			/* EV Bias */
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x02, 0x13);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x09, 0x03);
			/* AWB */
			err = m4mo_set_wb(M4MO_WB_AUTO);
			/* Chroma Saturation */
			err = m4mo_set_saturation(M4MO_SATURATION_DEFAULT);
			/* Sharpness */
			err = m4mo_set_sharpness(M4MO_SHARPNESS_PLUS_2);
			/* Flash */
			err = m4mo_set_flash_capture(M4MO_FLASH_CAPTURE_OFF);
			/* ISO */
			sensor->iso = M4MO_ISO_AUTO;
			break;			
		case M4MO_SCENE_CANDLELIGHT:
			/* AF mode / Lens Position */
			err = m4mo_set_focus_mode(M4MO_AF_MODE_NORMAL);
			/* Face Detection */
			err = m4mo_set_face_detection(M4MO_FACE_DETECTION_OFF);
			/*Photometry */
			err = m4mo_set_photometry(M4MO_PHOTOMETRY_CENTER);
			/* EV-P */
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x0A, 0x0D);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x0B, 0x0D);
			/* EV Bias */
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x02, 0x13);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x09, 0x03);
			/* AWB */
			err = m4mo_set_wb(M4MO_WB_DAYLIGHT);
			/* Chroma Saturation */
			err = m4mo_set_saturation(M4MO_SATURATION_DEFAULT);
			/* Sharpness */
			err = m4mo_set_sharpness(M4MO_SHARPNESS_DEFAULT);
			/* Flash */
			err = m4mo_set_flash_capture(M4MO_FLASH_CAPTURE_OFF);
			/* ISO */
			sensor->iso = M4MO_ISO_AUTO;
			break;
		case M4MO_SCENE_BACKLIGHT:
			/* AF mode / Lens Position */
			err = m4mo_set_focus_mode(M4MO_AF_MODE_NORMAL);
			/* Face Detection */
			err = m4mo_set_face_detection(M4MO_FACE_DETECTION_OFF);
			/*Photometry */
			err = m4mo_set_photometry(M4MO_PHOTOMETRY_CENTER);
			/* EV-P */
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x0A, 0x0E);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x0B, 0x0E);
			/* EV Bias */
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x02, 0x13);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x09, 0x03);
			/* AWB */
			err = m4mo_set_wb(M4MO_WB_AUTO);
			/* Chroma Saturation */
			err = m4mo_set_saturation(M4MO_SATURATION_DEFAULT);
			/* Sharpness */
			err = m4mo_set_sharpness(M4MO_SHARPNESS_DEFAULT);
			/* Flash */
			err = m4mo_set_flash_capture(M4MO_FLASH_CAPTURE_ON);
			/* ISO */
			sensor->iso = M4MO_ISO_AUTO;
			break;
		default:
			dprintk(CAM_ERR, M4MO_MOD_NAME "[Scene]Invalid value is ordered!!!\n");
			return -EINVAL;
	}

	sensor->scene= value;

	return err;
}

static int m4mo_get_zoom(struct v4l2_control *vc)
{
	struct m4mo_sensor *sensor = &m4mo;
	struct i2c_client *client = sensor->i2c_client;
	
	u32 val;

	m4mo_read_category_parm(client, M4MO_8BIT, 0x02, 0x01, &val);
	vc->value = (u8)(val*10); // M4MO 10=1x, ANDROID 100=1x
	dprintk(CAM_DBG, M4MO_MOD_NAME "Zoom Value reading... 0x%x \n", val);

	return 0;
}

static int m4mo_set_zoom(s32 value)
{
	struct m4mo_sensor *sensor = &m4mo;
	struct i2c_client *client = sensor->i2c_client;
    int zoom_val;
    
   	if(value > M4MO_ZOOM_STAGES)
	{
		dprintk(CAM_ERR, M4MO_MOD_NAME "Error, Zoom Value is out of range: %d!.\n", value );
		return -EINVAL;
	}
    zoom_val = m4mo_zoom_step[value];

    dprintk(CAM_DBG, M4MO_MOD_NAME "%s is called... value=%d, zoom_val=%d\n", __func__, value, zoom_val);
      
	if(sensor->zoom == zoom_val)
		return 0;
	
	sensor->zoom=zoom_val;
#ifdef ZOOM_USE_IRQ
	/* Interrupt Clear */
	m4mo_read_category_parm(client, M4MO_8BIT, 0x00, 0x10, &intr_value);
	/* Enable Digital Zoom Interrupt */
	m4mo_write_category_parm(client, M4MO_8BIT, 0x00, 0x11, 0x04);
	/* Enable Interrupt Control */
	m4mo_write_category_parm(client, M4MO_8BIT, 0x00, 0x12, 0x01);
#endif
	/* Start Digital Zoom */
	m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x01, zoom_val); 
	mdelay(30);
#ifdef ZOOM_USE_IRQ
	/* Wait "Digital Zoom Finish" Interrupt */
	dprintk(CAM_DBG, M4MO_MOD_NAME "Waiting for interrupt... \n");
	wait_event_interruptible(cam_wait, cam_interrupted == 1);
	if(m4mo_read_interrupt() == 0x04) 
	{
		dprintk(CAM_DBG, M4MO_MOD_NAME "OK... 'Digital Zoom Finish' interrupt!.\n");
		return 0;
	}
	else
	{
		dprintk(CAM_ERR, M4MO_MOD_NAME "Error! Interrupt signal is not  'Digital Zoom Finish' signal!.\n");
		return -EPERM;
	}	
#endif
	return 0;
}

static int m4mo_get_capture_size(struct v4l2_control *vc)
{

//	struct m4mo_sensor *sensor = &m4mo;
//	struct i2c_client *client = sensor->i2c_client;
//	u32 val1, val2, val3, val4;
   	
    dprintk(CAM_INF, M4MO_MOD_NAME "%s is called...\n", __func__); 
 #if 0
	m4mo_read_category_parm(client, M4MO_8BIT, 0x0C, 0x10, &val1); //Read JPEG Main Frame Length
	m4mo_read_category_parm(client, M4MO_8BIT, 0x0C, 0x11, &val2); //Read JPEG Main Frame Length
	m4mo_read_category_parm(client, M4MO_8BIT, 0x0C, 0x12, &val3); //Read JPEG Main Frame Length
	m4mo_read_category_parm(client, M4MO_8BIT, 0x0C, 0x13, &val4); //Read JPEG Main Frame Length
	
	vc->value = (val1<<24) + (val2<<16) + (val3<<8) + val4;
	dprintk(CAM_DBG, M4MO_MOD_NAME "Capture Image Size reading... 0x%x \n", vc->value);
#endif
	vc->value = jpeg_capture_size;

	return 0;
}

static int m4mo_get_thumbnail_size(struct v4l2_control *vc)
{
//    struct m4mo_sensor *sensor = &m4mo;
//	struct i2c_client *client = sensor->i2c_client;
//	struct v4l2_pix_format pix = sensor->pix;

//	u32 val1, val2, val3;
    
    dprintk(CAM_INF, M4MO_MOD_NAME "%s is called...\n", __func__); 
#if 0
	if(pix.pixelformat == V4L2_PIX_FMT_JPEG) 
	{
		m4mo_read_category_parm(client, M4MO_8BIT, 0x0C, 0x1E, &val1); //Read JPEG Thumbnail Length
		m4mo_read_category_parm(client, M4MO_8BIT, 0x0C, 0x1F, &val2); //Read JPEG Thumbnail Length
		m4mo_read_category_parm(client, M4MO_8BIT, 0x0C, 0x20, &val3); //Read JPEG Thumbnail Length
	
		vc->value = (val1<<16) + (val2<<8) + val3;
		dprintk(CAM_DBG, M4MO_MOD_NAME "Thumbnail Size reading... 0x%x \n", vc->value);
	}
	else if(pix.pixelformat == V4L2_PIX_FMT_MJPEG) 
	{
		vc->value = 320 * 240 * 2;
		dprintk(CAM_DBG, M4MO_MOD_NAME "Thumbnail Size reading... 0x%x \n", vc->value);
	}
#endif
// is set in m4mo_start_capture_transfer()
	vc->value = thumbnail_capture_size;
	
	return 0;
}
static int m4mo_set_thumbnail_size(s32 value)
{
	struct m4mo_sensor *sensor = &m4mo;
	struct i2c_client *client = sensor->i2c_client;
	
	if(sensor->thumbnail_size == value)
		return 0;
	
	switch(value)
	{
		case M4MO_THUMB_QVGA_SIZE:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x25, 0x01);
			break;
		case M4MO_THUMB_400_225_SIZE:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x25, 0x02);
			break;
		case M4MO_THUMB_WQVGA_SIZE:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x25, 0x03);
			break;
		case M4MO_THUMB_VGA_SIZE:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x25, 0x04);
			break;
		case M4MO_THUMB_WVGA_SIZE:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x02, 0x25, 0x05);
			break;
		default:
			dprintk(CAM_ERR, M4MO_MOD_NAME "[Thumbnail Size]Invalid value is ordered!!!\n");
			return -EINVAL;
	}

	sensor->thumbnail_size= value;

	return 0;
}

static int m4mo_get_esd_int(struct v4l2_control *vc)
{
	struct m4mo_sensor *sensor = &m4mo;
	struct i2c_client *client = sensor->i2c_client;
	
	u32 val;

	m4mo_read_category_parm(client, M4MO_8BIT, 0x00, 0x0E, &val); //Read JPEG Main Frame Length
	
	vc->value = val;

	return 0;
}

static int m4mo_get_fw_version(struct v4l2_control *vc)
{
	struct m4mo_sensor *sensor = &m4mo;
	struct i2c_client *client = sensor->i2c_client;
	
	u32 fvh = 0, fvl = 0;

	/* Read Firmware Version */
	m4mo_read_category_parm(client, M4MO_8BIT, 0x00, 0x01, &fvh);
	m4mo_read_category_parm(client, M4MO_8BIT, 0x00, 0x02, &fvl);
	vc->value = fvl + (fvh << 8); 

	return 0;
}

static void m4mo_wq_fw_dump(struct work_struct *work)
{
	m4mo_i2c_dump_firmware();

	return;
}

static void m4mo_wq_fw_update(struct work_struct *work)
{
	m4mo_i2c_update_firmware();

	return;
}

#if 1
static int m4mo_set_focus_mode(s32 value)
{
	struct m4mo_sensor *sensor = &m4mo;
	struct i2c_client *client = sensor->i2c_client;
	int err = 0;
	
	dprintk(CAM_DBG, M4MO_MOD_NAME "AF set focus mode = %d\n", value);
	if(sensor->focus_mode == value)
		return 0;

	switch(value) 
	{
		case M4MO_AF_MODE_NORMAL :
			/* Release Auto Focus */
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0A, 0x02, 0x00); 
	
			/*AF VCM Driver Standby Mode OFF */
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0A, 0x00, 0x01); 
			/* Set Normal AF Mode */		
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0A, 0x01, 0x00); 
			/* Set Base Position */
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0A, 0x10, 0x01); 

			err = m4mo_i2c_verify(client, 0x0A, 0x10, 0x00);
			if(err)  
			{
				dprintk(CAM_DBG, M4MO_MOD_NAME "Failed to wait a AF init!\n");
				return err;
			}
			/*AF VCM Driver Standby Mode ON */
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0A, 0x00, 0x00); 
		
			sensor->af_mode = M4MO_AF_MODE_NORMAL;
			break;
		case M4MO_AF_MODE_MACRO  :
			/* Release Auto Focus */
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0A, 0x02, 0x00); 

			/*AF VCM Driver Standby Mode OFF */
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0A, 0x00, 0x01); 
			/* Set Macro AF Mode */	
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0A, 0x01, 0x01); 
			/* Set Macro Base Position */
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0A, 0x10, 0x02); 
			
			err =m4mo_i2c_verify(client, 0x0A, 0x10, 0x00);
			if(err) 
			{
				dprintk(CAM_DBG, M4MO_MOD_NAME "Failed to wait a AF init!\n");
				return err;
			}
			break;
            
        default:
            dprintk(CAM_DBG, M4MO_MOD_NAME "unsupported focus mode: %d!\n", value);
            return 1;
	}


    sensor->af_mode = value;
	sensor->focus_mode = value;
	
	return err;
}

static int m4mo_set_focus_auto(struct v4l2_control *vc)
{
	struct m4mo_sensor *sensor = &m4mo;
	struct i2c_client *client = sensor->i2c_client;
	int err = 0;
    //int    count = 0;
	u32 data;
	
	switch(vc->value)
	{
		case 1:
			if(!(m4mo_ctrl_list[3].flags & V4L2_CTRL_FLAG_GRABBED))
			err = m4mo_set_auto_focus(M4MO_AF_START);
			
			m4mo_read_category_parm(client, M4MO_8BIT, 0x0A, 0x03, &data);	
			if(data == 0)
			{
				m4mo_ctrl_list[3].flags |= V4L2_CTRL_FLAG_GRABBED;
				msleep(10);
				return -EAGAIN;
			}
			else
			{
				m4mo_ctrl_list[3].flags &= (~V4L2_CTRL_FLAG_GRABBED);
				return 0;
			}
		case 0:
			err = m4mo_set_auto_focus(M4MO_AF_STOP);
			break;
		default:
			dprintk(CAM_ERR, M4MO_MOD_NAME "[focus auto]Invalid value is ordered!!!\n");
			return -EINVAL;
	}

	if(!err)
		sensor->focus_auto = vc->value;

	return err;
}

static int m4mo_set_exposure_auto(s32 value)
{
	struct m4mo_sensor *sensor = &m4mo;
	struct i2c_client *client = sensor->i2c_client;

	switch(value)
	{
		case 0:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x00, 0x00);
			break;
		case 1:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x03, 0x00, 0x01);
			break;
		default:
			dprintk(CAM_ERR, M4MO_MOD_NAME "[EXPOSURE_AUTO]Invalid value is ordered!!!\n");
			return -EINVAL;
	}

	sensor->exposre_auto = value;
	
	return 0;
}

static int m4mo_set_exposure(s32 value)
{
	struct m4mo_sensor *sensor = &m4mo;
	struct i2c_client *client = sensor->i2c_client;

	switch(value)
	{
		case 0:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x04, 0xFC);
			break;
		case 1:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x04, 0xFD);
			break;
		case 2:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x04, 0xFE);
			break;
		case 3:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x04, 0xFF);
			break;
		case 4:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x04, 0x00);
			break;
		case 5:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x04, 0x01);
			break;
		case 6:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x04, 0x02);
			break;	
		case 7:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x04, 0x03);
			break;
		case 8:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x04, 0x04);
			break;
		default:
			dprintk(CAM_ERR, M4MO_MOD_NAME "[EXPOSURE]Invalid value is ordered!!!\n");
			return -EINVAL;
	}

	sensor->exposure = value;
	
	return 0;
}
#if 0
static int m4mo_set_zoom_mode(s32 value)
{
	struct m4mo_sensor *sensor = &m4mo;
	struct i2c_client *client = sensor->i2c_client;

	switch(value)
	{
		case 0:
			break;
		default:
			dprintk(CAM_ERR, M4MO_MOD_NAME "[ZOOM_MODE]Invalid value is ordered!!!\n");
			return -EINVAL;
	}

	sensor->zoom_mode = value;

	return 0;
}
#endif

static int m4mo_set_auto_white_balance(s32 value)
{
	struct m4mo_sensor *sensor = &m4mo;
	struct i2c_client *client = sensor->i2c_client;

	switch(value)
	{
		case 0:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x00, 0x00);
			break;
		case 1:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x00, 0x01);
			break;
		default:
			dprintk(CAM_ERR, M4MO_MOD_NAME "[AUTO_WHITE_BALANCE]Invalid value is ordered!!!\n");
			return -EINVAL;
	}

	sensor->auto_white_balance = value;

	return 0;
}
#if 0
static int m4mo_set_white_balance_preset(s32 value)
{
	struct m4mo_sensor *sensor = &m4mo;
	struct i2c_client *client = sensor->i2c_client;

	switch(value)
	{
		case 0:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x02, 0x01);
			break;
		case 1:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x02, 0x02);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x03, 0x01);
			break;
		case 2:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x02, 0x02);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x03, 0x02);
			break;
		case 3:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x02, 0x02);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x03, 0x03);
			break;
		case 4:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x02, 0x02);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x03, 0x04);
			break;
		case 5:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x02, 0x02);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x03, 0x05);
			break;
		case 6:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x02, 0x02);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x06, 0x03, 0x06);
			break;
		default:
			dprintk(CAM_ERR, M4MO_MOD_NAME "[WHITE_BALANCE_PRESET]Invalid value is ordered!!!\n");
			return -EINVAL;
	}

	sensor->white_balance_preset = value;

	return 0;
}
#endif
static int m4mo_set_strobe(enum v4l2_strobe_conf mode)
{
	struct m4mo_sensor *sensor = &m4mo;
	struct i2c_client *client = sensor->i2c_client;

	switch(mode)
	{
		case V4L2_STROBE_OFF:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0B, 0x00, 0x00);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0B, 0x01, 0x00);
			break;
		case V4L2_STROBE_ON:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0B, 0x00, 0x01);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0B, 0x01, 0x01);
			break;
		case V4L2_STROBE_AUTO:
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0B, 0x00, 0x02);
			m4mo_write_category_parm(client, M4MO_8BIT, 0x0B, 0x01, 0x02);
			break;
		default:
			dprintk(CAM_ERR, M4MO_MOD_NAME "[S_STROBE]Invalid value is ordered!!!\n");
			return -EINVAL;
	}

	sensor->strobe_conf_mode = mode;

	return 0;
}
#endif


static void m4mo_set_skip(struct v4l2_pix_format *pix)
{
    struct m4mo_sensor *sensor = &m4mo;

    int skip_frame = 0; 

    dprintk(CAM_INF, M4MO_MOD_NAME "m4mo_set_skip is called for '%s'\n",
        (sensor->state == M4MO_STATE_PREVIEW)?"preview":"capture");

    //wait for overlay creation (250ms ~ 300ms)
    if(sensor->state == M4MO_STATE_PREVIEW)
    { 
        if(m4mo_curr_state == M4MO_STATE_INVALID)
        {
            skip_frame = sensor->fps / 6 + 1; 
        }
        else
        {
            skip_frame = sensor->fps / 4 + 1; 
        }
    }
    else
    {
        // if(pix->pixelformat == V4L2_PIX_FMT_JPEG)
            // skip_frame = 0;
        // else
            skip_frame = 2;
    }

    dprintk(CAM_INF, M4MO_MOD_NAME "skip frame = %d frame\n", skip_frame);

    isp_set_hs_vs(skip_frame);

}


static int m4mo_start_preview(void)
{
	struct m4mo_sensor *sensor = &m4mo;
	struct i2c_client *client = sensor->i2c_client;

//	int mode;
    int rval;
	u32 intr_value;

    dprintk(CAM_DBG, M4MO_MOD_NAME "%s is called...\n", __func__);

	/* If mode is already Monitor, return now */
	if( m4mo_get_mode() == M4MO_MONITOR_MODE)
	{
		dprintk(CAM_DBG, M4MO_MOD_NAME "m4mo already in monitior mode!\n");
		return 0;
	}


	/* Interrupt Clear */
	m4mo_read_category_parm(client, M4MO_8BIT, 0x00, 0x10, &intr_value);
	/* Enable YUV-Output Interrupt */
	m4mo_write_category_parm(client, M4MO_8BIT, 0x00, 0x11, 0x01);
	/* Enable Interrupt Control */
	m4mo_write_category_parm(client, M4MO_8BIT, 0x00, 0x12, 0x01);

	/* Set Monitor Mode */
	rval = m4mo_set_mode(M4MO_MONITOR_MODE);
	if(rval)
	{
		dprintk(CAM_ERR, M4MO_MOD_NAME "Can not set operation mode\n");
		m4mo_write_category_parm(client, M4MO_8BIT, 0x00, 0x11, 0x00);
		return rval;
	}

	/* Wait YUV-Output Interrupt */
	dprintk(CAM_DBG, M4MO_MOD_NAME "Waiting for interrupt... \n");

	wait_event_interruptible(cam_wait, cam_interrupted == 1); 
	if(m4mo_read_interrupt() == 0x01) 
	{
		dprintk(CAM_DBG, M4MO_MOD_NAME "OK... 'YUV-Output' interrupt!.\n");
		m4mo_write_category_parm(client, M4MO_8BIT, 0x00, 0x11, 0x00);
		return 0;
	}
	else
	{
		dprintk(CAM_ERR, M4MO_MOD_NAME "Error! Interrupt signal is not  'YUV-Output' signal!.\n");
		m4mo_write_category_parm(client, M4MO_8BIT, 0x00, 0x11, 0x00);
		return -EPERM;
	}	

}

static int m4mo_start_capture_transfer(void)
{
	struct m4mo_sensor *sensor = &m4mo;
	struct i2c_client *client = sensor->i2c_client;
	struct v4l2_pix_format pix = sensor->pix;
    u32 reg;
	int i, size=0, count=0;
	u32 val1, val2, val3, val4;
       
	dprintk(CAM_DBG, M4MO_MOD_NAME "%s is called...\n", __func__);  

// DPRINTK_ISPCCDC("ccdc status: -----------------------------------------\n");
// ispccdc_print_status();
// DPRINTK_ISPCCDC("-------------------------------------------------------\n");

	/* Select main frame image and wait until capture size is non zero*/
	m4mo_write_category_parm(client, M4MO_8BIT, 0x0C, 0x04, 0x01);
	while(size == 0)
	{
		m4mo_read_category_parm(client, M4MO_8BIT, 0x0C, 0x10, &val1); //Read JPEG Main Frame Length
		m4mo_read_category_parm(client, M4MO_8BIT, 0x0C, 0x11, &val2); //Read JPEG Main Frame Length
		m4mo_read_category_parm(client, M4MO_8BIT, 0x0C, 0x12, &val3); //Read JPEG Main Frame Length
		m4mo_read_category_parm(client, M4MO_8BIT, 0x0C, 0x13, &val4); //Read JPEG Main Frame Length
	
		size= (val1<<24) + (val2<<16) + (val3<<8) + val4;
		dprintk(CAM_DBG, M4MO_MOD_NAME "Capture Image Size reading... 0x%x \n", size);

		if(size != 0)
			break;
		
		if((count++)==100) 
			return -EBUSY;

		msleep(10);
	}
	jpeg_capture_size = size;

	/* Read thumbnail capture size */
	if(pix.pixelformat == V4L2_PIX_FMT_JPEG) 
	{
		m4mo_read_category_parm(client, M4MO_8BIT, 0x0C, 0x1E, &val1); //Read JPEG Thumbnail Length
		m4mo_read_category_parm(client, M4MO_8BIT, 0x0C, 0x1F, &val2); //Read JPEG Thumbnail Length
		m4mo_read_category_parm(client, M4MO_8BIT, 0x0C, 0x20, &val3); //Read JPEG Thumbnail Length
	
		size = (val1<<16) + (val2<<8) + val3;
		dprintk(CAM_DBG, M4MO_MOD_NAME "Thumbnail Size reading... 0x%x \n", size);
	}
	else if(pix.pixelformat == V4L2_PIX_FMT_MJPEG) 
	{
		size = 320 * 240 * 2;
		dprintk(CAM_DBG, M4MO_MOD_NAME "Thumbnail Size reading... 0x%x \n", size);
	}
	thumbnail_capture_size = size;

	for(i=0; i<10; i++)
	{
		/* Start to transmit main image */
		m4mo_write_category_parm(client, M4MO_8BIT, 0x0C, 0x05, 0x01);
		msleep(10);
	}
	

	printk(KERN_INFO M4MO_MOD_NAME "Capture Image Transfer Complete!!\n");
    

// //check if ccdc is busy
// reg = isp_reg_readl(OMAP3_ISP_IOMEM_CCDC, ISPCCDC_PCR); 
// dprintk(CAM_DBG, M4MO_MOD_NAME "ccdc module is state is %s\n", reg & ISPCCDC_PCR_BUSY?"busy":"idle"); 


	return 0;
}

static int m4mo_start_capture(void)
{
	struct m4mo_sensor *sensor = &m4mo;
	struct i2c_client *client = sensor->i2c_client;
	
//	struct v4l2_pix_format pix = sensor->pix;
	
	u32 intr_value;
	
    dprintk(CAM_DBG, M4MO_MOD_NAME "%s is called...\n", __func__);  

	/* Interrupt Clear */
	m4mo_read_category_parm(client, M4MO_8BIT, 0x00, 0x10, &intr_value);
	/* Enable Capture Interrupt */
	m4mo_write_category_parm(client, M4MO_8BIT, 0x00, 0x11, 0x08);
	/* Enable Interrupt Control */
	m4mo_write_category_parm(client, M4MO_8BIT, 0x00, 0x12, 0x01);
	
	/* Start Single Capture */
	m4mo_set_mode(M4MO_STILLCAP_MODE);
    
	/* Wait Capture Interrupt */
	dprintk(CAM_DBG, M4MO_MOD_NAME "Waiting for interrupt... \n");
	wait_event_interruptible(cam_wait, cam_interrupted == 1);
	if(m4mo_read_interrupt() == 0x08) 
	{
		m4mo_write_category_parm(client, M4MO_8BIT, 0x00, 0x11, 0x00);
		dprintk(CAM_DBG, M4MO_MOD_NAME "OK... 'capture' interrupt!.\n");
	}
	else
	{
		m4mo_write_category_parm(client, M4MO_8BIT, 0x00, 0x11, 0x00);
		dprintk(CAM_ERR, M4MO_MOD_NAME "Error! Interrupt signal is not capture signal!.\n");
		return -EPERM;
	}	

	printk(KERN_INFO M4MO_MOD_NAME "Capture Image Storing and Processing Complete!!\n");

	return 0; 
}

static int ioctl_streamoff(struct v4l2_int_device *s)
{
//	struct m4mo_sensor *sensor = s->priv;
//	struct i2c_client *client = sensor->i2c_client;
	int err = 0;
dprintk(CAM_INF, M4MO_MOD_NAME "%s called...\n", __func__); 
		
	return err;
}

static int ioctl_streamon(struct v4l2_int_device *s)
{
	struct m4mo_sensor *sensor = s->priv;
#if 0    
u32 reg;
reg = isp_reg_readl(OMAP3_ISP_IOMEM_CCDC, ISPCCDC_VDINT);   
dprintk(CAM_DBG, M4MO_MOD_NAME "ISPCCDC_VDINT =0x%x\n", reg); 
dprintk(CAM_DBG, M4MO_MOD_NAME "VDINT0 =%d, VDINT1 =%d\n", (reg>>16)&0x7fff, reg&0x7fff); 
 
reg = isp_reg_readl(OMAP3_ISP_IOMEM_MAIN, ISP_IRQ0ENABLE);
dprintk(CAM_DBG, M4MO_MOD_NAME "ISP_IRQ0ENABLE =0x%x\n", reg); 
dprintk(CAM_DBG, M4MO_MOD_NAME "    HS_VS_IRQ =%s\n", reg & IRQ0ENABLE_HS_VS_IRQ?"on":"off"); 
dprintk(CAM_DBG, M4MO_MOD_NAME "    VD0_IRQ =%s\n",   reg & IRQ0ENABLE_CCDC_VD0_IRQ?"on":"off"); 
dprintk(CAM_DBG, M4MO_MOD_NAME "    VD1_IRQ =%s\n",   reg & IRQ0ENABLE_CCDC_VD1_IRQ?"on":"off"); 

reg = isp_reg_readl(OMAP3_ISP_IOMEM_MAIN, ISP_CTRL);  
dprintk(CAM_DBG, M4MO_MOD_NAME "ISP_CTRL =0x%x\n", reg); 
dprintk(CAM_DBG, M4MO_MOD_NAME "    CCDC_FLUSH = %s\n", reg & ISPCTRL_CCDC_FLUSH?"on":"off"); 
dprintk(CAM_DBG, M4MO_MOD_NAME "    JPEG_FLUSH = %s\n", reg & ISPCTRL_JPEG_FLUSH?"on":"off"); 
dprintk(CAM_DBG, M4MO_MOD_NAME "    SYNC_DETECT = %d\n", reg>>14&0x3);  

// reg = isp_reg_readl(OMAP3_ISP_IOMEM_CCP2, ISPCSI1_CTRL);
// dprintk(CAM_DBG, M4MO_MOD_NAME "ISPCSI1_CTRL =0x%x\n", reg); 

// reg = isp_reg_readl(OMAP3_ISP_IOMEM_CSI2A, ISPCSI2_CTRL);
// dprintk(CAM_DBG, M4MO_MOD_NAME "ISPCSI1_CTRL =0x%x\n", reg); 
#endif
//	int rval;
    
    dprintk(CAM_INF, M4MO_MOD_NAME "ioctl_streamon is called (%x)\n",sensor->capture_mode);
    
    if(sensor->state != M4MO_STATE_CAPTURE)
    {
        if(sensor->mode == M4MO_MODE_CAMCORDER)    
        {
            /* Lens focus setting */
            if(m4mo_set_focus_mode(M4MO_AF_MODE_NORMAL))
            goto streamon_fail;       
        }
        
        dprintk(CAM_DBG, M4MO_MOD_NAME "start preview....................\n");
        if(m4mo_start_preview())
        goto streamon_fail;    
        
        /* Zoom setting */
        //if(m4mo_set_zoom(sensor->zoom))
        //goto streamon_fail;      
    }
    else
    {
        dprintk(CAM_DBG, M4MO_MOD_NAME "start capture transfer....................\n");
        //m4mo_set_capture(V4L2_PIX_FMT_JPEG);
        if(m4mo_start_capture_transfer())
            goto streamon_fail;
    }

    return 0;

    streamon_fail:
    printk(M4MO_MOD_NAME "ioctl_streamon is failed\n"); 
    return -EINVAL;  
}

/**
 * ioctl_queryctrl - V4L2 sensor interface handler for VIDIOC_QUERYCTRL ioctl
 * @s: pointer to standard V4L2 device structure
 * @qc: standard V4L2 VIDIOC_QUERYCTRL ioctl structure
 *
 * If the requested control is supported, returns the control information
 * from the m4mosensor_video_control[] array.
 * Otherwise, returns -EINVAL if the control is not supported.
 */
static int ioctl_queryctrl(struct v4l2_int_device *s,
				struct v4l2_queryctrl *qc)
{
	int i;
    
    dprintk(CAM_INF, M4MO_MOD_NAME "%s called...\n", __func__); 

	for (i = 0; i < NUM_M4MO_CONTROL; i++) {
		if (qc->id == m4mo_ctrl_list[i].id)
			break;
	}
	if (i == NUM_M4MO_CONTROL)
	{
		dprintk(CAM_DBG, M4MO_MOD_NAME "Control ID is not supported!!\n");
		qc->flags |= V4L2_CTRL_FLAG_DISABLED;

		return -EINVAL;
	}

	*qc = m4mo_ctrl_list[i];

	return 0;
}

static int ioctl_querymenu(struct v4l2_int_device *s,
				struct v4l2_querymenu *qm)
{
	int i;
    
    dprintk(CAM_INF, M4MO_MOD_NAME "%s called...\n", __func__); 

	for (i = 0; i < NUM_M4MO_MENU; i++) {
		if ((qm->id == m4mo_menu_list[i].id) && (qm->index == m4mo_menu_list[i].index))
			break;
	}
	if (i == NUM_M4MO_MENU)
	{
		dprintk(CAM_DBG, M4MO_MOD_NAME "Control ID or Menu Index is not supported!!\n");
		return -EINVAL;
	}

	*qm = m4mo_menu_list[i];

	return 0;
}

static int ioctl_querycap(struct v4l2_int_device *s,
				struct v4l2_capability* cap)
{

    dprintk(CAM_INF, M4MO_MOD_NAME "%s called...\n", __func__); 

	cap->capabilities = V4L2_CAP_STREAMING | V4L2_CAP_VIDEO_CAPTURE;
    strlcpy(cap->card, MOD_STW_NAME, sizeof(cap->card));

	return 0;
}

/**
 * ioctl_g_ctrl - V4L2 sensor interface handler for VIDIOC_G_CTRL ioctl
 * @s: pointer to standard V4L2 device structure
 * @vc: standard V4L2 VIDIOC_G_CTRL ioctl structure
 *
 * If the requested control is supported, returns the control's current
 * value from the m4mosensor_video_control[] array.
 * Otherwise, returns -EINVAL if the control is not supported.
 */
static int ioctl_g_ctrl(struct v4l2_int_device *s,
			     struct v4l2_control *vc)
{	
	struct m4mo_sensor *sensor = s->priv;
	int retval = 0;
    
    dprintk(CAM_INF, M4MO_MOD_NAME "%s called...\n", __func__); 

	switch (vc->id) {
      case V4L2_CID_SELECT_MODE:
        vc->value = sensor->mode;
      break;  
     case V4L2_CID_SELECT_STATE:
        vc->value = sensor->state;
      break;  
	case V4L2_CID_AF:
		retval = m4mo_get_auto_focus(vc);
		break;
	case V4L2_CID_ZOOM:
		retval = m4mo_get_zoom(vc);
		break;
	case V4L2_CID_JPEG_SIZE:
		retval = m4mo_get_capture_size(vc);
		break;
    case V4L2_CID_JPEG_CAPTURE_WIDTH:
        vc->value = sensor->jpeg_capture_w;
        break; 
    case V4L2_CID_JPEG_CAPTURE_HEIGHT:
        vc->value = sensor->jpeg_capture_h;
        break;
	case V4L2_CID_THUMBNAIL_SIZE:
		retval = m4mo_get_thumbnail_size(vc);
		break;
	case V4L2_CID_EFFECT:
		vc->value = sensor->effect;
		break;
	case V4L2_CID_ISO:
		vc->value = sensor->iso;
		break;
	case V4L2_CID_BRIGHTNESS:
		vc->value = sensor->ev;
		break;
	case V4L2_CID_SATURATION:
		vc->value = sensor->saturation;
		break;
	case V4L2_CID_SHARPNESS:
		vc->value = sensor->sharpness;
		break;
	case V4L2_CID_CONTRAST:
		vc->value = sensor->contrast;
		break;
	case V4L2_CID_WB:
		vc->value = sensor->wb;
		break;
	case V4L2_CID_AEWB:
		vc->value = sensor->aewb;
		break;
	case V4L2_CID_PHOTOMETRY:
		vc->value = sensor->photometry;
		break;	
	case V4L2_CID_FACE_DETECTION:
		vc->value = sensor->face_detection;
		break;	
	case V4L2_CID_SCENE:
		vc->value = sensor->scene;
		break;
	case V4L2_CID_JPEG_QUALITY:
		vc->value = sensor->jpeg_quality;
		break;
	case V4L2_CID_WDR:
		vc->value = sensor->wdr;
		break;
	case V4L2_CID_ISC:
		vc->value = sensor->isc;
		break;
	case V4L2_CID_FLASH_CAPTURE:
		vc->value = sensor->flash_capture;
		break;
	case V4L2_CID_ESD_INT:
		retval = m4mo_get_esd_int(vc);
		break;
	case V4L2_CID_FW_VERSION:
		retval = m4mo_get_fw_version(vc);
		break;
	case V4L2_CID_FW_UPDATE:
		vc->value = fw_update_status;
		break;
	case V4L2_CID_FW_DUMP:
		vc->value = fw_dump_status;
		break;
	case V4L2_CID_PRA_VERSION:
		vc->value = 0x0;
		break;
	case V4L2_CID_AF_VERSION:
		vc->value = 0x0;
		break;

	/* Mobile Communication Lab. Request */
	case V4L2_CID_FOCUS_MODE:
		vc->value = sensor->focus_mode;
		break;
	case V4L2_CID_FOCUS_AUTO:
		vc->value = sensor->focus_auto;
		break;
	case V4L2_CID_EXPOSURE_AUTO:
		vc->value = sensor->exposre_auto;
		break;
	case V4L2_CID_EXPOSURE:
		vc->value = sensor->exposure;
		break;
#if 0        
	case V4L2_CID_ZOOM_MODE:
		vc->value = sensor->zoom_mode;
		break;
#endif
	case V4L2_CID_ZOOM_ABSOLUTE:
		vc->value = sensor->zoom_absolute;
		break;
	case V4L2_CID_AUTO_WHITE_BALANCE:
		vc->value = sensor->auto_white_balance;
		break;
#if 0
	case V4L2_CID_WHITE_BALANCE_PRESET:
		vc->value = sensor->white_balance_preset;
		break;
#endif
	

	default:
        dprintk(CAM_ERR, M4MO_MOD_NAME "unsupported ioctl_g: %d\n",vc->id); 

		break;
	}

	return retval;
}
static int m4mo_set_mode_v4l(s32 value)
{
  struct m4mo_sensor *sensor = &m4mo;
  sensor->mode = value;
  dprintk(CAM_INF, M4MO_MOD_NAME "m4mo_set_mode is called... mode = %d\n", sensor->mode); 
  return 0;
}

static int m4mo_set_state_v4l(s32 value)
{
  struct m4mo_sensor *sensor = &m4mo;
  sensor->state = value;
  dprintk(CAM_INF, M4MO_MOD_NAME "m4mo_set_state is called... state = %d\n", sensor->state); 
  return 0;
}

/**
 * ioctl_s_ctrl - V4L2 sensor interface handler for VIDIOC_S_CTRL ioctl
 * @s: pointer to standard V4L2 device structure
 * @vc: standard V4L2 VIDIOC_S_CTRL ioctl structure
 *
 * If the requested control is supported, sets the control's current
 * value in HW (and updates the m4mosensor_video_control[] array).
 * Otherwise, * returns -EINVAL if the control is not supported.
 */
static int ioctl_s_ctrl(struct v4l2_int_device *s,
			     struct v4l2_control *vc)
{	
	struct m4mo_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;
	
	int retval = -EINVAL;
	u32 fvh = 0, fvl = 0;
    
    dprintk(CAM_INF, M4MO_MOD_NAME "%s called...\n", __func__); 

	switch (vc->id) {
        case V4L2_CID_SELECT_MODE:
        retval = m4mo_set_mode_v4l(vc->value);
        break;  
    case V4L2_CID_SELECT_STATE:
        retval = m4mo_set_state_v4l(vc->value);
        break; 
	case V4L2_CID_AF:
		retval = m4mo_set_auto_focus(vc->value);
		break;
	case V4L2_CID_ZOOM:
		retval = m4mo_set_zoom(vc->value);
		break;
	case V4L2_CID_EFFECT:
		retval = m4mo_set_effect(vc->value);
		break;
	case V4L2_CID_ISO:
		retval = m4mo_set_iso(vc->value);
		break;
	case V4L2_CID_BRIGHTNESS:
		retval = m4mo_set_ev(vc->value);
		break;
	case V4L2_CID_SATURATION:
		retval = m4mo_set_saturation(vc->value);
		break;
	case V4L2_CID_SHARPNESS:
		retval = m4mo_set_sharpness(vc->value);
		break;
	case V4L2_CID_CONTRAST:
		retval = m4mo_set_contrast(vc->value);
		break;
	case V4L2_CID_WB:
		retval = m4mo_set_wb(vc->value);
		break;
	case V4L2_CID_AEWB:
		retval = m4mo_set_aewb(vc->value);
		break;
	case V4L2_CID_PHOTOMETRY:
		retval = m4mo_set_photometry(vc->value);
		break;	
	case V4L2_CID_FACE_DETECTION:
		retval = m4mo_set_face_detection(vc->value);
		break;	
	case V4L2_CID_SCENE:
		retval = m4mo_set_scene(vc->value);
		break;
	case V4L2_CID_JPEG_QUALITY:
		retval = m4mo_set_jpeg_quality(vc->value);
		break;
	case V4L2_CID_FW_UPDATE:
		if(camfw_update == 0)
		{
			retval = 0;
			break;
		}
		else if(camfw_update ==1)
		{
			retval = 0;
			/* Read Firmware Version */
			m4mo_read_category_parm(client, M4MO_8BIT, 0x00, 0x01, &fvh);
			m4mo_read_category_parm(client, M4MO_8BIT, 0x00, 0x02, &fvl);
			if((fvh == cam_fw_major_version) && (fvl == cam_fw_minor_version))
			{
				printk(KERN_INFO M4MO_MOD_NAME "Current Firmware is the newest one!!\n");
				break;
			}
			fw_update_status = 0;
			queue_work(camera_wq, &camera_firmup_work);
			break;
		}
		else if(camfw_update == 2 || camfw_update == 3)
		{
			fw_update_status = 0;
			queue_work(camera_wq, &camera_firmup_work);
			retval = 0;
            break;
		} 
        else
        break;
	case V4L2_CID_WDR:
		retval = m4mo_set_wdr(vc->value);
		break;
	case V4L2_CID_ISC:
		retval = m4mo_set_isc(vc->value);
		break;
	case V4L2_CID_FLASH_CAPTURE:
		retval = m4mo_set_flash_capture(vc->value);
		break;
	case V4L2_CID_THUMBNAIL_SIZE:
		retval = m4mo_set_thumbnail_size(vc->value);
		break;
	case V4L2_CID_FW_DUMP:
		fw_dump_status = 0;
		queue_work(camera_wq, &camera_firmdump_work);
		retval = 0;
		break;

	/* Mobile Communication Lab. Request */
	case V4L2_CID_FOCUS_MODE:
		retval = m4mo_set_focus_mode(vc->value);
		break;
	case V4L2_CID_FOCUS_AUTO:
		retval = m4mo_set_focus_auto(vc);
		break;
	case V4L2_CID_EXPOSURE_AUTO:
		retval = m4mo_set_exposure_auto(vc->value);
		break;
	case V4L2_CID_EXPOSURE:
		retval = m4mo_set_exposure(vc->value);
		break;
#if 0        
	case V4L2_CID_ZOOM_MODE:
		retval = m4mo_set_zoom_mode(vc->value);
		break;
#endif
	case V4L2_CID_AUTO_WHITE_BALANCE:
		retval = m4mo_set_auto_white_balance(vc->value);
		break;
#if 0
	case V4L2_CID_WHITE_BALANCE_PRESET:
		retval = m4mo_set_white_balance_preset(vc->value);
		break;
#endif     


	default:
		break;
	}

	return retval;
}


/**
 * ioctl_enum_fmt_cap - Implement the CAPTURE buffer VIDIOC_ENUM_FMT ioctl
 * @s: pointer to standard V4L2 device structure
 * @fmt: standard V4L2 VIDIOC_ENUM_FMT ioctl structure
 *
 * Implement the VIDIOC_ENUM_FMT ioctl for the CAPTURE buffer type.
 */
static int ioctl_enum_fmt_cap(struct v4l2_int_device *s,
				   struct v4l2_fmtdesc *fmt)
{
    int index = fmt->index;
 //   enum v4l2_buf_type type = fmt->type;

    dprintk(CAM_INF, M4MO_MOD_NAME "%s called...\n", __func__); 

    switch (fmt->type) 
    {
    case V4L2_BUF_TYPE_VIDEO_CAPTURE:
        switch(fmt->pixelformat)
        {
        case V4L2_PIX_FMT_UYVY:
            index = 0;
            break;

        case V4L2_PIX_FMT_JPEG:
            index = 1;
            break;

        case V4L2_PIX_FMT_MJPEG:
            index = 2;
            break;

        default:
            printk(M4MO_MOD_NAME "[format]Invalid value is ordered!!!\n");
            goto enum_fmt_cap_fail;
        }
        break;
        
    default:
        printk(M4MO_MOD_NAME "[type]Invalid value is ordered!!!\n");
        goto enum_fmt_cap_fail;
    }

    fmt->flags = m4mo_formats[index].flags;
    fmt->pixelformat = m4mo_formats[index].pixelformat;
    strlcpy(fmt->description, m4mo_formats[index].description, sizeof(fmt->description));

	return 0;
    
enum_fmt_cap_fail:
    printk(M4MO_MOD_NAME "ioctl_enum_fmt_cap is failed\n"); 
    return -EINVAL;  
}

/**
 * ioctl_try_fmt_cap - Implement the CAPTURE buffer VIDIOC_TRY_FMT ioctl
 * @s: pointer to standard V4L2 device structure
 * @f: pointer to standard V4L2 VIDIOC_TRY_FMT ioctl structure
 *
 * Implement the VIDIOC_TRY_FMT ioctl for the CAPTURE buffer type.  This
 * ioctl is used to negotiate the image capture size and pixel format
 * without actually making it take effect.
 */
static int ioctl_try_fmt_cap(struct v4l2_int_device *s,
			     struct v4l2_format *f)
{
    struct v4l2_pix_format *pix = &f->fmt.pix;
    struct m4mo_sensor *sensor = s->priv;
    struct v4l2_pix_format *pix2 = &sensor->pix;

    int index = 0;

    dprintk(CAM_INF, M4MO_MOD_NAME "ioctl_try_fmt_cap is called...\n");
    dprintk(CAM_DBG, M4MO_MOD_NAME "ioctl_try_fmt_cap. mode : %d\n", sensor->mode);
    dprintk(CAM_DBG, M4MO_MOD_NAME "ioctl_try_fmt_cap. state : %d\n", sensor->state);

    m4mo_set_skip(pix);  

    if(sensor->state == M4MO_STATE_CAPTURE)
    { 
        for(index = 0; index < ARRAY_SIZE(m4mo_image_sizes); index++)
        {
            if(m4mo_image_sizes[index].width == pix->width && m4mo_image_sizes[index].height == pix->height)
            {
                sensor->capture_size = index;
                break;
            }
        }   

        if(index == ARRAY_SIZE(m4mo_image_sizes))
        {
            printk(M4MO_MOD_NAME "Capture Image Size is not supported!\n");
            goto try_fmt_fail;
        }

        dprintk(CAM_DBG, M4MO_MOD_NAME "M4MO--capture size = %d\n", sensor->capture_size);  
        dprintk(CAM_DBG, M4MO_MOD_NAME "M4MO--capture width : %d\n", m4mo_image_sizes[index].width);
        dprintk(CAM_DBG, M4MO_MOD_NAME "M4MO--capture height : %d\n", m4mo_image_sizes[index].height);      

        if(pix->pixelformat == V4L2_PIX_FMT_UYVY || pix->pixelformat == V4L2_PIX_FMT_YUYV)
        {
            pix->field = V4L2_FIELD_NONE;
            pix->bytesperline = pix->width * 2;
            pix->sizeimage = pix->bytesperline * pix->height;
            dprintk(CAM_DBG, M4MO_MOD_NAME "V4L2_PIX_FMT_UYVY\n");
        }
        else
        {
            pix->field = V4L2_FIELD_NONE;
            pix->bytesperline = JPEG_CAPTURE_WIDTH * 2;
            pix->sizeimage = pix->bytesperline * JPEG_CAPTURE_HEIGHT;
            dprintk(CAM_DBG, M4MO_MOD_NAME "V4L2_PIX_FMT_JPEG\n");
        }

        dprintk(CAM_DBG, M4MO_MOD_NAME "set capture....................\n");

        if(m4mo_set_capture(pix->pixelformat))
            goto try_fmt_fail;
    }  

    switch (pix->pixelformat) 
    {
    case V4L2_PIX_FMT_YUYV:
    case V4L2_PIX_FMT_UYVY:
    case V4L2_PIX_FMT_JPEG:
    case V4L2_PIX_FMT_MJPEG:
        pix->colorspace = V4L2_COLORSPACE_JPEG;
        break;
    case V4L2_PIX_FMT_RGB565:
    case V4L2_PIX_FMT_RGB565X:
    case V4L2_PIX_FMT_RGB555:
    case V4L2_PIX_FMT_SGRBG10:
    case V4L2_PIX_FMT_RGB555X:
    default:
        pix->colorspace = V4L2_COLORSPACE_SRGB;
        break;
    }

    *pix2 = *pix;

    return 0;

    try_fmt_fail:
    printk(M4MO_MOD_NAME "ioctl_try_fmt_cap is failed\n"); 
    return -EINVAL;
   
}



static char *getPxFmtName(int fmt)
{
    int i;
    
    for(i=0; i<NUM_CAPTURE_FORMATS; i++)
        if(m4mo_formats[i].pixelformat == fmt)
            return (char *)m4mo_formats[i].description;
            
    return "unknown";
}
/**
 * ioctl_s_fmt_cap - V4L2 sensor interface handler for VIDIOC_S_FMT ioctl
 * @s: pointer to standard V4L2 device structure
 * @f: pointer to standard V4L2 VIDIOC_S_FMT ioctl structure
 *
 * If the requested format is supported, configures the HW to use that
 * format, returns error code if format not supported or HW can't be
 * correctly configured.
 */
static int ioctl_s_fmt_cap(struct v4l2_int_device *s, struct v4l2_format *f)
{
    struct v4l2_pix_format *pix = &f->fmt.pix;
    struct m4mo_sensor *sensor = s->priv;
    struct v4l2_pix_format *pix2 = &sensor->pix;

    int index = 0;
    
    dprintk(CAM_INF, M4MO_MOD_NAME "ioctl_s_fmt_cap is called...\n");

    printk(M4MO_MOD_NAME "camera mode  : %d (1:camera , 2:camcorder, 3:vt)\n", sensor->mode);
    printk(M4MO_MOD_NAME "camera state : %d (0:preview, 1:snapshot)\n", sensor->state);
    printk(M4MO_MOD_NAME "set width  : %d\n", pix->width);
    printk(M4MO_MOD_NAME "set height : %d\n", pix->height); 
    printk(M4MO_MOD_NAME "set format : %s\n", getPxFmtName(pix->pixelformat)); 


    m4mo_720p_enable = false;
    
    m4mo_set_skip(pix);  
    
    if(sensor->state == M4MO_STATE_CAPTURE)
    { 
        /* check for capture */
        // if(m4mo_prepare_capture())
        // goto s_fmt_fail;   
        for(index = 0; index < ARRAY_SIZE(m4mo_image_sizes); index++)
        {
            if(m4mo_image_sizes[index].width == pix->width && m4mo_image_sizes[index].height == pix->height)
            {
                sensor->capture_size = index;
                break;
            }
        }   

        if(index == ARRAY_SIZE(m4mo_image_sizes))
        {
            printk(M4MO_MOD_NAME "Capture Image %d x %d Size is not supported!\n", pix->width, pix->height);
            goto s_fmt_fail;
        }

        dprintk(CAM_DBG, M4MO_MOD_NAME "M4MO--capture size = %d\n", sensor->capture_size);  
        dprintk(CAM_DBG, M4MO_MOD_NAME "M4MO--capture width : %d\n", m4mo_image_sizes[index].width);
        dprintk(CAM_DBG, M4MO_MOD_NAME "M4MO--capture height : %d\n", m4mo_image_sizes[index].height);      

        if(pix->pixelformat == V4L2_PIX_FMT_UYVY || pix->pixelformat == V4L2_PIX_FMT_YUYV)
        {
            pix->field = V4L2_FIELD_NONE;
            pix->bytesperline = pix->width * 2;
            pix->sizeimage = pix->bytesperline * pix->height;
            dprintk(CAM_DBG, M4MO_MOD_NAME "V4L2_PIX_FMT_UYVY\n");
        }
        else
        {
            pix->field = V4L2_FIELD_NONE;
            pix->bytesperline = JPEG_CAPTURE_WIDTH * 2;
            pix->sizeimage = pix->bytesperline * JPEG_CAPTURE_HEIGHT;
            dprintk(CAM_DBG, M4MO_MOD_NAME "V4L2_PIX_FMT_JPEG\n");
        }

        if(m4mo_set_capture(pix->pixelformat))
            goto s_fmt_fail;
        
    }  
    else
    {  
        /* check for preview */
        // if(m4mo_prepare_preview())
        // goto s_fmt_fail;

        for(index = 0; index < ARRAY_SIZE(m4mo_preview_sizes); index++)
        {
            if(m4mo_preview_sizes[index].width == pix->width && m4mo_preview_sizes[index].height == pix->height)
            {
                sensor->preview_size = index;
                break;
            }
        }   

        if(index == ARRAY_SIZE(m4mo_preview_sizes))
        {
            printk(M4MO_MOD_NAME "Preview Image %d x %d Size is not supported!\n", pix->width, pix->height);
            goto s_fmt_fail;
        }

        if(sensor->mode == M4MO_MODE_CAMCORDER)
        {
            if(pix->width == 1280 && pix->height == 720)
            {
                dprintk(CAM_DBG, M4MO_MOD_NAME "Preview Image Size is 720P!\n");
                m4mo_720p_enable = true;
            }
        }

        dprintk(CAM_DBG, M4MO_MOD_NAME "M4MO--preview size = %d\n", sensor->preview_size); 
        dprintk(CAM_DBG, M4MO_MOD_NAME "M4MO--preview width : %d\n", m4mo_preview_sizes[index].width);
        dprintk(CAM_DBG, M4MO_MOD_NAME "M4MO--preview height : %d\n", m4mo_preview_sizes[index].height);      

        pix->field = V4L2_FIELD_NONE;
        pix->bytesperline = pix->width * 2;
        pix->sizeimage = pix->bytesperline * pix->height;  
        dprintk(CAM_DBG, M4MO_MOD_NAME "V4L2_PIX_FMT_UYVY\n");

        if(m4mo_set_preview())

        goto s_fmt_fail;
    }      

    switch (pix->pixelformat) 
    {
    case V4L2_PIX_FMT_YUYV:
    case V4L2_PIX_FMT_UYVY:
    case V4L2_PIX_FMT_JPEG:
    case V4L2_PIX_FMT_MJPEG:
        pix->colorspace = V4L2_COLORSPACE_JPEG;
        break;
    case V4L2_PIX_FMT_RGB565:
    case V4L2_PIX_FMT_RGB565X:
    case V4L2_PIX_FMT_RGB555:
    case V4L2_PIX_FMT_SGRBG10:
    case V4L2_PIX_FMT_RGB555X:
    default:
        pix->colorspace = V4L2_COLORSPACE_SRGB;
        break;
    }

    *pix2 = *pix;

    return 0;

    s_fmt_fail:
    printk(M4MO_MOD_NAME "ioctl_s_fmt_cap is failed\n"); 
    return -EINVAL;  
    }

/**
 * ioctl_g_fmt_cap - V4L2 sensor interface handler for ioctl_g_fmt_cap
 * @s: pointer to standard V4L2 device structure
 * @f: pointer to standard V4L2 v4l2_format structure
 *
 * Returns the sensor's current pixel format in the v4l2_format
 * parameter.
 */
static int ioctl_g_fmt_cap(struct v4l2_int_device *s,
				struct v4l2_format *f)
{
	struct m4mo_sensor *sensor = s->priv;
    
    dprintk(CAM_INF, M4MO_MOD_NAME "%s called...\n", __func__); 

	f->fmt.pix = sensor->pix;

	return 0;
}

/**
 * ioctl_g_parm - V4L2 sensor interface handler for VIDIOC_G_PARM ioctl
 * @s: pointer to standard V4L2 device structure
 * @a: pointer to standard V4L2 VIDIOC_G_PARM ioctl structure *
 * Returns the sensor's video CAPTURE parameters.
 */
static int ioctl_g_parm(struct v4l2_int_device *s,
			     struct v4l2_streamparm *a)
{
	struct m4mo_sensor *sensor = s->priv;
	struct v4l2_captureparm *cparm = &a->parm.capture;
    
    dprintk(CAM_INF, M4MO_MOD_NAME "%s called...\n", __func__); 

	if (a->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	memset(a, 0, sizeof(*a));
	a->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	cparm->capability = V4L2_CAP_TIMEPERFRAME;
	cparm->timeperframe = sensor->timeperframe;

	return 0;
}

/**
 * ioctl_s_parm - V4L2 sensor interface handler for VIDIOC_S_PARM ioctl
 * @s: pointer to standard V4L2 device structure
 * @a: pointer to standard V4L2 VIDIOC_S_PARM ioctl structure
 *
 * Configures the sensor to use the input parameters, if possible.  If
 * not possible, reverts to the old parameters and returns the
 * appropriate error code.
 */
static int ioctl_s_parm(struct v4l2_int_device *s,
			     struct v4l2_streamparm *a)
{
	struct m4mo_sensor *sensor = s->priv;
	struct v4l2_fract *timeperframe = &a->parm.capture.timeperframe;
  
    dprintk(CAM_INF, M4MO_MOD_NAME "%s called...\n", __func__); 
  
    /* Set mode (camera/camcorder/vt) & state (preview/capture) */
    sensor->mode = a->parm.capture.capturemode;
    sensor->state = a->parm.capture.currentstate;
    
    if(sensor->mode < 1 || sensor->mode > 3) sensor->mode = M4MO_MODE_CAMERA;
    dprintk(CAM_DBG, M4MO_MOD_NAME "mode = %d, state = %d\n", sensor->mode, sensor->state);   
 
    /* Set time per frame (FPS) */
    if((timeperframe->numerator == 0)&&(timeperframe->denominator == 0))
    {
        sensor->fps = 30;
    }
    else
    {
        sensor->fps = timeperframe->denominator / timeperframe->numerator;
    }
    sensor->timeperframe = *timeperframe;
    dprintk(CAM_DBG, M4MO_MOD_NAME "fps = %d\n", sensor->fps);  

	return 0;
}

/**
 * ioctl_g_ifparm - V4L2 sensor interface handler for vidioc_int_g_ifparm_num
 * @s: pointer to standard V4L2 device structure
 * @p: pointer to standard V4L2 vidioc_int_g_ifparm_num ioctl structure
 *
 * Gets slave interface parameters.
 * Calculates the required xclk value to support the requested
 * clock parameters in p.  This value is returned in the p
 * parameter.
 */
static int ioctl_g_ifparm(struct v4l2_int_device *s, struct v4l2_ifparm *p)
{
	struct m4mo_sensor *sensor = s->priv;
	int rval;
    
    dprintk(CAM_INF, M4MO_MOD_NAME "%s called...\n", __func__); 
	
	rval = sensor->pdata->ifparm(p);
	if (rval)
    {
        return rval;
    }
        
    p->u.bt656.clock_curr = M4MO_XCLK;
    
	return 0;
}

/**
 * ioctl_g_priv - V4L2 sensor interface handler for vidioc_int_g_priv_num
 * @s: pointer to standard V4L2 device structure
 * @p: void pointer to hold sensor's private data address
 *
 * Returns device's (sensor's) private data area address in p parameter
 */
static int ioctl_g_priv(struct v4l2_int_device *s, void *p)
{
	struct m4mo_sensor *sensor = s->priv;
    
    dprintk(CAM_INF, M4MO_MOD_NAME "%s called...\n", __func__); 
    
    if(p == NULL)
    {
        printk(M4MO_MOD_NAME "ioctl_g_priv is failed because of null pointer\n"); 
        return -EINVAL;
    }
    
	return sensor->pdata->priv_data_set(p);

}

/**
 * ioctl_s_power - V4L2 sensor interface handler for vidioc_int_s_power_num
 * @s: pointer to standard V4L2 device structure
 * @on: power state to which device is to be set
 *
 * Sets devices power state to requrested state, if possible.
 */
static int ioctl_s_power(struct v4l2_int_device *s, enum v4l2_power on)
{
    struct m4mo_sensor *sensor = s->priv;
    
    dprintk(CAM_INF, M4MO_MOD_NAME "%s called...\n", __func__); 

	if(sensor->pdata->power_set(on))
    {
            printk(M4MO_MOD_NAME "Can not power on/off " M4MO_DRIVER_NAME " sensor\n"); 
        goto s_power_fail;
    }

    switch(on)
    {
    case V4L2_POWER_ON:
        {
            dprintk(CAM_DBG, M4MO_MOD_NAME "pwr on-----!\n");
            if(m4mo_detect()) 
            {
                printk(M4MO_MOD_NAME "Unable to detect " M4MO_DRIVER_NAME " sensor\n");
                sensor->pdata->power_set(V4L2_POWER_OFF);
                goto s_power_fail;
            }

            /* Make the default detect */
            sensor->detect = SENSOR_DETECTED;     
        }
        break;

    case V4L2_POWER_RESUME:
        {
            dprintk(CAM_DBG, M4MO_MOD_NAME "pwr resume-----!\n");
        }  
        break;

    case V4L2_POWER_STANDBY:
        {
            dprintk(CAM_DBG, M4MO_MOD_NAME "pwr stanby-----!\n");
        }
        break;

    case V4L2_POWER_OFF:
        {
            dprintk(CAM_DBG, M4MO_MOD_NAME "pwr off-----!\n");

            /* Make the default detect */
            sensor->detect = SENSOR_NOT_DETECTED;    
        }
        break;
    }

    return 0;

s_power_fail:
    printk(M4MO_MOD_NAME "ioctl_s_power is failed\n");
    return -EINVAL;
}

static int ioctl_s_strobe(struct v4l2_int_device *s, struct v4l2_strobe *strobe)
{
//	struct m4mo_sensor *sensor = s->priv;
	int err;
    
    dprintk(CAM_INF, M4MO_MOD_NAME "%s called...\n", __func__); 

	err = m4mo_set_strobe(strobe->mode);

	return err;
}

static int ioctl_g_strobe(struct v4l2_int_device *s, struct v4l2_strobe *strobe)
{
	struct m4mo_sensor *sensor = s->priv;
    
    dprintk(CAM_INF, M4MO_MOD_NAME "%s called...\n", __func__); 

	strobe->capabilities = V4L2_STROBE_CAP_OFF | V4L2_STROBE_CAP_ON 
						| V4L2_STROBE_CAP_AUTO;
	strobe->mode = sensor->strobe_conf_mode;

	return 0;
}

static int ioctl_g_exif(struct v4l2_int_device *s, struct v4l2_exif *exif)
{
	struct m4mo_sensor *sensor = s->priv;
	struct i2c_client *client = sensor->i2c_client;

	int ret;
	u32 val;
    
    dprintk(CAM_INF, M4MO_MOD_NAME "%s called...\n", __func__); 

	ret = m4mo_read_category_parm(client, 4, 0x07, 0x00, &val);
	exif->exposure_time_numerator = val;
	ret = m4mo_read_category_parm(client, 4, 0x07, 0x04, &val);
	exif->exposure_time_denominator = val;
	ret = m4mo_read_category_parm(client, 4, 0x07, 0x08, &val);
	exif->shutter_speed_numerator = (s32)val;
	ret = m4mo_read_category_parm(client, 4, 0x07, 0x0C, &val);
	exif->shutter_speed_denominator = (s32)val;
	ret = m4mo_read_category_parm(client, 4, 0x07, 0x18, &val);
	exif->brigtness_numerator= (s32)val;
	ret = m4mo_read_category_parm(client, 4, 0x07, 0x1C, &val);
	exif->brightness_denominator = (s32)val;
	ret = m4mo_read_category_parm(client, 2, 0x07, 0x28, &val);
	exif->iso = (u16)val;
	ret = m4mo_read_category_parm(client, 2, 0x07, 0x2A, &val);
	if(val == 0x09 || val == 0x19)
		exif->flash = M4MO_FLASH_FIRED;
	else
		exif->flash = M4MO_FLASH_NO_FIRED;

	return 0;
}


/**
 * ioctl_deinit - V4L2 sensor interface handler for VIDIOC_INT_DEINIT
 * @s: pointer to standard V4L2 device structure
 *
 * Deinitialize the sensor device
 */
static int ioctl_deinit(struct v4l2_int_device *s)
{		
    struct m4mo_sensor *sensor = s->priv;
    
    dprintk(CAM_INF, M4MO_MOD_NAME "%s called...\n", __func__); 
    
    sensor->state = M4MO_STATE_INVALID; //init problem

  /* Make the state init */
    m4mo_curr_state = M4MO_STATE_INVALID;  
    
	free_irq(OMAP_GPIO_IRQ(OMAP_GPIO_ISP_INT), &m4mo);

	return 0;
}


/**
 * ioctl_init - V4L2 sensor interface handler for VIDIOC_INT_INIT
 * @s: pointer to standard V4L2 device structure
 *
 * Initialize the sensor device (call m4mo_configure())
 */
static int ioctl_init(struct v4l2_int_device *s)
{
	struct m4mo_sensor *sensor = s->priv;
	int err;
    
    dprintk(CAM_INF, M4MO_MOD_NAME "%s called...\n", __func__); 

	/* Initialize flag variable */
	sensor->timeperframe.numerator          = 1;
	sensor->timeperframe.denominator        = 30;
	sensor->fps                             = 30;
	sensor->state                           = M4MO_STATE_INVALID;
    sensor->mode                            = M4MO_MODE_CAMERA;
	sensor->effect                          = M4MO_EFFECT_OFF;
	sensor->iso                             = M4MO_ISO_AUTO;
	sensor->photometry                      = M4MO_UNINIT_VALUE;
	sensor->ev                              = M4MO_EV_DEFAULT;
	sensor->wdr                             = M4MO_WDR_OFF;
	sensor->saturation                      = M4MO_SATURATION_DEFAULT;
	sensor->sharpness                       = M4MO_SHARPNESS_DEFAULT;
	sensor->contrast                        = M4MO_CONTRAST_DEFAULT;
	sensor->wb                              = M4MO_WB_AUTO;
	sensor->isc                             = M4MO_ISC_STILL_OFF;
	sensor->scene                           = M4MO_SCENE_NORMAL;
	sensor->aewb                            = M4MO_AE_UNLOCK_AWB_UNLOCK;
	sensor->flash_capture                   = M4MO_FLASH_CAPTURE_OFF;
	sensor->face_detection                  = M4MO_FACE_DETECTION_OFF;
	sensor->jpeg_quality                    = M4MO_UNINIT_VALUE; 
	sensor->zoom                            = M4MO_ZOOM_1X_VALUE;
	sensor->thumbnail_size                  = M4MO_THUMB_QVGA_SIZE;
	sensor->preview_size                    = M4MO_QVGA_SIZE;
	sensor->capture_size                    = M4MO_SHOT_5M_SIZE;
	sensor->af_mode                         = M4MO_UNINIT_VALUE;
    sensor->jpeg_capture_w                  = JPEG_CAPTURE_WIDTH;
	sensor->jpeg_capture_h                  = JPEG_CAPTURE_HEIGHT;  
	sensor->focus_mode                      = 0,
	sensor->focus_auto                      = 0,
	sensor->exposre_auto                    = 0,
	sensor->exposure                        = 4,
//	sensor->zoom_mode                       = 0,
	sensor->zoom_absolute                   = 0,
	sensor->auto_white_balance              = 0,
//	sensor->white_balance_preset            = 0,
	sensor->strobe_conf_mode                = V4L2_STROBE_OFF,

	cam_interrupted = 0;

    memcpy(&m4mo, sensor, sizeof(struct m4mo_sensor));

    err = m4mo_configure();

    m4mo_curr_state = M4MO_STATE_INVALID;  
    
	return err;         
}

#ifdef ZEUS_CAM
/* added following functins for v4l2 compatibility with omap34xxcam */

/**
 * ioctl_enum_framesizes - V4L2 sensor if handler for vidioc_int_enum_framesizes
 * @s: pointer to standard V4L2 device structure
 * @frms: pointer to standard V4L2 framesizes enumeration structure
 *
 * Returns possible framesizes depending on choosen pixel format
 **/
static int ioctl_enum_framesizes(struct v4l2_int_device *s,
					struct v4l2_frmsizeenum *frms)
{
//	int ifmt;
	struct m4mo_sensor* sensor = s->priv; //rs:

    dprintk(CAM_INF, M4MO_MOD_NAME "ioctl_enum_framesizes called...\n");   

    if (sensor->state == M4MO_STATE_CAPTURE)
    {    
        dprintk(CAM_DBG, M4MO_MOD_NAME "Size enumeration for image capture size = %d\n", sensor->capture_size);

        if(sensor->capture_size == ARRAY_SIZE(m4mo_image_sizes))
        goto enum_framesizes_fail;

        frms->index = sensor->capture_size;
        frms->type = V4L2_FRMSIZE_TYPE_DISCRETE;
        frms->discrete.width = m4mo_image_sizes[sensor->capture_size].width;
        frms->discrete.height = m4mo_image_sizes[sensor->capture_size].height;        
    }
    else
    {
        dprintk(CAM_DBG, M4MO_MOD_NAME "Size enumeration for image preview size = %d\n", sensor->preview_size);

        if(sensor->preview_size == ARRAY_SIZE(m4mo_preview_sizes))
        goto enum_framesizes_fail;
        
        frms->index = sensor->preview_size;
        frms->type = V4L2_FRMSIZE_TYPE_DISCRETE;
        frms->discrete.width = m4mo_preview_sizes[sensor->preview_size].width;
        frms->discrete.height = m4mo_preview_sizes[sensor->preview_size].height;        
    }

    dprintk(CAM_DBG, M4MO_MOD_NAME "framesizes width : %d\n", frms->discrete.width); 
    dprintk(CAM_DBG, M4MO_MOD_NAME "framesizes height : %d\n", frms->discrete.height); 

    return 0;

enum_framesizes_fail:
    printk(M4MO_MOD_NAME "ioctl_enum_framesizes is failed\n"); 
    return -EINVAL;   
}


static int ioctl_enum_frameintervals(struct v4l2_int_device *s,
					struct v4l2_frmivalenum *frmi)
{
//	int ifmt;

    dprintk(CAM_INF, M4MO_MOD_NAME "ioctl_enum_frameintervals \n"); 
    dprintk(CAM_DBG, M4MO_MOD_NAME "ioctl_enum_frameintervals numerator : %d\n", frmi->discrete.numerator); 
    dprintk(CAM_DBG, M4MO_MOD_NAME "ioctl_enum_frameintervals denominator : %d\n", frmi->discrete.denominator); 

	return 0;
}
#endif  /* end of zeus_cam */

static struct v4l2_int_ioctl_desc m4mo_ioctl_desc[] = {
#ifdef ZEUS_CAM
	{ .num = vidioc_int_enum_framesizes_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_enum_framesizes},
  	{ .num = vidioc_int_enum_frameintervals_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_enum_frameintervals},
#endif	  
	{ .num = vidioc_int_s_power_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_s_power },
	{ .num = vidioc_int_g_priv_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_g_priv },
	{ .num = vidioc_int_g_ifparm_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_g_ifparm },
	{ .num = vidioc_int_init_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_init },
	{ .num = vidioc_int_deinit_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_deinit },
	{ .num = vidioc_int_enum_fmt_cap_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_enum_fmt_cap },
	{ .num = vidioc_int_try_fmt_cap_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_try_fmt_cap },
	{ .num = vidioc_int_g_fmt_cap_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_g_fmt_cap },
	{ .num = vidioc_int_s_fmt_cap_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_s_fmt_cap },
	{ .num = vidioc_int_g_parm_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_g_parm },
	{ .num = vidioc_int_s_parm_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_s_parm },
	{ .num = vidioc_int_queryctrl_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_queryctrl },
	{ .num = vidioc_int_g_ctrl_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_g_ctrl },
	{ .num = vidioc_int_s_ctrl_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_s_ctrl },
	{ .num = vidioc_int_streamon_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_streamon },
	{ .num = vidioc_int_streamoff_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_streamoff },
	{ .num = vidioc_int_querymenu_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_querymenu },
	{ .num = vidioc_int_querycap_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_querycap },  
	{ .num = vidioc_int_s_strobe_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_s_strobe },  
	{ .num = vidioc_int_g_strobe_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_g_strobe },
	{ .num = vidioc_int_g_exif_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_g_exif },
};

static struct v4l2_int_slave m4mo_slave = {
	.ioctls = m4mo_ioctl_desc,
	.num_ioctls = ARRAY_SIZE(m4mo_ioctl_desc),
};

static struct v4l2_int_device m4mo_int_device = {
	.module = THIS_MODULE,
	.name = M4MO_DRIVER_NAME,
	.priv = &m4mo,
	.type = v4l2_int_type_slave,
	.u = {
		.slave = &m4mo_slave,
	},
};


/**
 * m4mo_probe - sensor driver i2c probe handler
 * @client: i2c driver client device structure
 *
 * Register sensor as an i2c client device and V4L2
 * device.
 */
static int
m4mo_probe(struct i2c_client *client, const struct i2c_device_id *device)
{
	struct m4mo_sensor *sensor = &m4mo;
	int err;

	if (i2c_get_clientdata(client))
		return -EBUSY;

	sensor->pdata = &z_m4mo_platform_data;

	if (!sensor->pdata) {
		dprintk(CAM_ERR, M4MO_MOD_NAME "no platform data?\n");
		return -ENODEV;
	}

	sensor->v4l2_int_device = &m4mo_int_device;
	sensor->i2c_client = client;

	i2c_set_clientdata(client, sensor);

	/* Make the default capture format WVGA, V4L2_PIX_FMT_YUYV */
	sensor->pix.width = 800;
	sensor->pix.height = 480;
	sensor->pix.pixelformat = V4L2_PIX_FMT_UYVY;

	err = v4l2_int_device_register(sensor->v4l2_int_device);
	if (err)
		i2c_set_clientdata(client, NULL);

	camera_wq = create_singlethread_workqueue("camera_wq");
	INIT_WORK(&camera_firmup_work, m4mo_wq_fw_update);
	INIT_WORK(&camera_firmdump_work, m4mo_wq_fw_dump);
	
	return 0;
}

/**
 * m4mo_remove - sensor driver i2c remove handler
 * @client: i2c driver client device structure
 *
 * Unregister sensor as an i2c client device and V4L2
 * device.  Complement of m4mo_probe().
 */
static int __exit
m4mo_remove(struct i2c_client *client)
{
	struct m4mo_sensor *sensor = i2c_get_clientdata(client);

	if (!client->adapter)
		return -ENODEV;	/* our client isn't attached */

	v4l2_int_device_unregister(sensor->v4l2_int_device);
	i2c_set_clientdata(client, NULL);

	return 0;
}

#ifdef ZEUS_CAM
static const struct i2c_device_id m4mo_id[] = {
	{ M4MO_DRIVER_NAME, 0 },
	{ },
};

MODULE_DEVICE_TABLE(i2c, m4mo_id);
#endif /* end of zeus_cam */

static struct i2c_driver m4mosensor_i2c_driver = {
	.driver = {
		.name = M4MO_DRIVER_NAME,
		.owner = THIS_MODULE,
	},
	.probe = m4mo_probe,
//	.remove = __exit_p(m4mo_remove),
    .remove = m4mo_remove,
#ifdef ZEUS_CAM	
	.id_table = m4mo_id,
#endif /* end of zeus_cam */
};

/**
 * m4mosensor_init - sensor driver module_init handler
 *
 * Registers driver as an i2c client driver.  Returns 0 on success,
 * error code otherwise.
 */
static int __init m4mosensor_init(void)
{
	int err;

	err = i2c_add_driver(&m4mosensor_i2c_driver);
	if (err) {
		dprintk(CAM_ERR, M4MO_MOD_NAME "Failed to register" M4MO_DRIVER_NAME ".\n");
		return err;
	}

	return 0;
}
module_init(m4mosensor_init);

/**
 * m4mosensor_cleanup - sensor driver module_exit handler
 *
 * Unregisters/deletes driver as an i2c client driver.
 * Complement of m4mosensor_init.
 */
static void __exit m4mosensor_cleanup(void)
{
	i2c_del_driver(&m4mosensor_i2c_driver);
}
module_exit(m4mosensor_cleanup);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("M4MO camera sensor driver");

