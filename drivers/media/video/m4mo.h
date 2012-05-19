/*
 * drivers/media/video/mt9p012.h
 *
 * Register definitions for the MT9P012 CameraChip.
 *
 * Author: Sameer Venkatraman, Mohit Jalori (ti.com)
 *
 * Copyright (C) 2008 Texas Instruments.
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 *****************************************************
 *****************************************************
 * modules/camera/m4mo.h
 *
 * M4MO sensor driver header file
 *
 * Modified by paladin in Samsung Electronics
 *
 */

#ifndef M4MO_H
#define M4MO_H

#define ZEUS_CAM
//#define ZOOM_USE_IRQ

#define CAM_M4MO_DBG_MSG                1
#define M4MO_DRIVER_NAME                "m-4mo"
#define M4MO_MOD_NAME                   "M-4MO: "
#define MOD_SEHF_NAME                   "OP:FUJITSU_M4MO:CD01"
#define MOD_STW_NAME                    "TE:FUJITSU_M4MO:CG27"

#define M4MO_I2C_ADDR   				0x1F
#define M4MO_I2C_RETRY					3
#define M4MO_I2C_VERIFY_RETRY			200
#define M4MO_XCLK   					24000000  

#define M4MO_8BIT				        1
#define M4MO_16BIT				        2
#define M4MO_32BIT				        4

#define SENSOR_DETECTED		            1
#define SENSOR_NOT_DETECTED         	0

#define M4MO_STATE_PREVIEW	            0	/*  preview state */
#define M4MO_STATE_CAPTURE	            1	/*  capture state */
#define M4MO_STATE_INVALID	            2	/*  invalid state */

#define M4MO_MODE_CAMERA                1
#define M4MO_MODE_CAMCORDER             2
#define M4MO_MODE_VT                    3


/**
 * struct m4mo_platform_data - platform data values and access functions
 * @power_set: Power state access function, zero is off, non-zero is on.
 * @ifparm: Interface parameters access function
 * @priv_data_set: device private data (pointer) access function
 */
struct m4mo_platform_data {
	int (*power_set)(enum v4l2_power power);
	int (*ifparm)(struct v4l2_ifparm *p);
	int (*priv_data_set)(void *);
};

/**   
 * struct m4mo_sensor - main structure for storage of sensor information
 * @pdata: access functions and data for platform level information
 * @v4l2_int_device: V4L2 device structure structure
 * @i2c_client: iic client device structure
 * @pix: V4L2 pixel format information structure
 * @timeperframe: time per frame expressed as V4L fraction
 * @scaler:
 * @ver: ce131 chip version
 * @fps: frames per second value   
 */
struct m4mo_sensor {
	const struct m4mo_platform_data *pdata;
	struct v4l2_int_device *v4l2_int_device;
	struct i2c_client *i2c_client;
	struct v4l2_pix_format pix;
	struct v4l2_fract timeperframe;
    u32 state;
    u8 mode;
    u8 detect;
	u32 capture_mode;
	int scaler;
	int ver;
	u8 fps;
	u8 effect;
	u8 iso;
	u8 photometry;
	u8 ev;
	u8 wdr;
	u8 saturation;
	u8 sharpness;
	u8 contrast;
	u8 wb;
	u8 isc;
	u8 scene;
	u8 aewb;
	u8 flash_capture;
	u8 face_detection;
	u8 jpeg_quality;
	u8 thumbnail_size;
	u8 preview_size;
	u8 capture_size;
	s32 zoom;
	u8 capture_flag;
	u8 af_mode;
    u32 jpeg_capture_w;
	u32 jpeg_capture_h;
	u8 focus_mode;
	u8 focus_auto;
	u8 exposre_auto;
	u8 exposure;
//	u8 zoom_mode;
	u8 zoom_absolute;
	u8 auto_white_balance;
//	u8 white_balance_preset;
	u8 strobe_conf_mode;
};

#define M4MO_UNINIT_VALUE 		    0xFF

/* FPS Capabilities */
#define M4MO_5_FPS				    0x8
#define M4MO_7_FPS				    0x7
#define M4MO_10_FPS			        0x5
#define M4MO_12_FPS			        0x4
#define M4MO_15_FPS			        0x3
#define M4MO_30_FPS			        0x2
#define M4MO_AUTO_FPS			    0x1

/* M4MO Sensor Mode */
#define M4MO_SYSINIT_MODE		    0x0
#define M4MO_PARMSET_MODE	        0x1
#define M4MO_MONITOR_MODE	        0x2
#define M4MO_STILLCAP_MODE	        0x3

/* M4MO Preview Size */
#define M4MO_QQVGA_SIZE		        0x1	
#define M4MO_QCIF_SIZE			    0x2		
#define M4MO_QVGA_SIZE			    0x3
#define M4MO_SL1_QVGA_SIZE	        0x4
#define M4MO_800_600_SIZE		    0x5		
#define M4MO_5M_SIZE			    0x6		
#define M4MO_640_482_SIZE		    0x7		
#define M4MO_800_602_SIZE		    0x8		
#define M4MO_SL2_QVGA_SIZE	        0x9		
#define M4MO_176_176_SIZE		    0xA		
#define M4MO_CIF_SIZE			    0xB		
#define M4MO_WQVGA_SIZE		        0xC		
#define M4MO_VGA_SIZE			    0xD		
#define M4MO_REV_QVGA_SIZE	        0xE		
#define M4MO_REV_WQVGA_SIZE	        0xF		
#define M4MO_432_240_SIZE		    0x10		
#define M4MO_WVGA_SIZE		        0x11
#define M4MO_REV_QCIF_SIZE		    0x12		
#define M4MO_240_432_SIZE		    0x13		
#define M4MO_XGA_SIZE			    0x14		
#define M4MO_1280_720_SIZE		    0x15
#define M4MO_720_480_SIZE		    0x16 // D1 NTSC
#define M4MO_720_576_SIZE		    0x15 // D1 PAL

/* M4MO Capture Size */
#define M4MO_SHOT_QVGA_SIZE		    0x1
#define M4MO_SHOT_VGA_SIZE		    0x2
#define M4MO_SHOT_1M_SIZE	        0x3
#define M4MO_SHOT_2M_SIZE		    0x4
#define M4MO_SHOT_3M_SIZE		    0x5
#define M4MO_SHOT_5M_SIZE		    0x6
#define M4MO_SHOT_WQVGA_SIZE	    0x7
#define M4MO_SHOT_432_240_SIZE	    0x8
#define M4MO_SHOT_640_360_SIZE	    0x9
#define M4MO_SHOT_WVGA_SIZE		    0xA
#define M4MO_SHOT_720P_SIZE		    0xB
#define M4MO_SHOT_1920_1080_SIZE    0xC
#define M4MO_SHOT_2560_1440_SIZE	0xD
#define M4MO_SHOT_160_120_SIZE	    0xE
#define M4MO_SHOT_QCIF_SIZE		    0xF
#define M4MO_SHOT_144_176_SIZE	    0x10
#define M4MO_SHOT_176_176_SIZE	    0x11
#define M4MO_SHOT_240_320_SIZE	    0x12
#define M4MO_SHOT_240_400_SIZE	    0x13
#define M4MO_SHOT_CIF_SIZE			0x14
#define M4MO_SHOT_SVGA_SIZE		    0x15
#define M4MO_SHOT_1024_768_SIZE	    0x16
#define M4MO_SHOT_4M_SIZE			0x17

/* M4MO Thumbnail Size */
#define M4MO_THUMB_QVGA_SIZE		0x1
#define M4MO_THUMB_400_225_SIZE	    0x2
#define M4MO_THUMB_WQVGA_SIZE	    0x3
#define M4MO_THUMB_VGA_SIZE		    0x4
#define M4MO_THUMB_WVGA_SIZE		0x5

/* M4MO Auto Focus Mode */
#define M4MO_AF_START 			    1
#define M4MO_AF_STOP  			    2
#define M4MO_AF_RELEASE		        3

#define M4MO_AF_MODE_NORMAL 	    1
#define M4MO_AF_MODE_MACRO  	    2

/* Image Effect */
#define M4MO_EFFECT_OFF				    1
#define M4MO_EFFECT_SEPIA				2
#define M4MO_EFFECT_GRAY				3
#define M4MO_EFFECT_RED				    4
#define M4MO_EFFECT_GREEN				5
#define M4MO_EFFECT_BLUE				6
#define M4MO_EFFECT_PINK				7
#define M4MO_EFFECT_YELLOW			    8
#define M4MO_EFFECT_PURPLE			    9
#define M4MO_EFFECT_ANTIQUE			    10
#define M4MO_EFFECT_NEGATIVE			11
#define M4MO_EFFECT_SOLARIZATION1		12
#define M4MO_EFFECT_SOLARIZATION2		13
#define M4MO_EFFECT_SOLARIZATION3		14
#define M4MO_EFFECT_SOLARIZATION4		15
#define M4MO_EFFECT_EMBOSS			    16
#define M4MO_EFFECT_OUTLINE			    17
#define M4MO_EFFECT_AQUA			   	18  

/* ISO */
#define M4MO_ISO_AUTO				1
#define M4MO_ISO_50					2
#define M4MO_ISO_100				3
#define M4MO_ISO_200				4
#define M4MO_ISO_400				5
#define M4MO_ISO_800				6
#define M4MO_ISO_1000				7

/* EV */
#define M4MO_EV_MINUS_4				1
#define M4MO_EV_MINUS_3				2
#define M4MO_EV_MINUS_2				3
#define M4MO_EV_MINUS_1				4
#define M4MO_EV_DEFAULT				5
#define M4MO_EV_PLUS_1				6
#define M4MO_EV_PLUS_2				7
#define M4MO_EV_PLUS_3				8
#define M4MO_EV_PLUS_4				9

/* Saturation*/
#define M4MO_SATURATION_MINUS_3		1
#define M4MO_SATURATION_MINUS_2		2
#define M4MO_SATURATION_MINUS_1		3
#define M4MO_SATURATION_DEFAULT		4
#define M4MO_SATURATION_PLUS_1		5
#define M4MO_SATURATION_PLUS_2		6
#define M4MO_SATURATION_PLUS_3		7

/* Sharpness */
#define M4MO_SHARPNESS_MINUS_3		1
#define M4MO_SHARPNESS_MINUS_2		2
#define M4MO_SHARPNESS_MINUS_1		3
#define M4MO_SHARPNESS_DEFAULT		4
#define M4MO_SHARPNESS_PLUS_1		5
#define M4MO_SHARPNESS_PLUS_2		6
#define M4MO_SHARPNESS_PLUS_3		7

/* Contrast */
#define M4MO_CONTRAST_MINUS_3		1
#define M4MO_CONTRAST_MINUS_2		2
#define M4MO_CONTRAST_MINUS_1		3
#define M4MO_CONTRAST_DEFAULT		4
#define M4MO_CONTRAST_PLUS_1		5
#define M4MO_CONTRAST_PLUS_2		6
#define M4MO_CONTRAST_PLUS_3		7

/* White Balance */
#define M4MO_WB_AUTO				1
#define M4MO_WB_INCANDESCENT		2
#define M4MO_WB_FLUORESCENT			3
#define M4MO_WB_DAYLIGHT			4
#define M4MO_WB_CLOUDY				5
#define M4MO_WB_SHADE				6
#define M4MO_WB_HORIZON				7

/* Auto Exposure & Auto White Balance */
#define M4MO_AE_LOCK_AWB_LOCK		1
#define M4MO_AE_LOCK_AWB_UNLOCK		2
#define M4MO_AE_UNLOCK_AWB_LOCK		3
#define M4MO_AE_UNLOCK_AWB_UNLOCK	4

/* Zoom */
#define M4MO_ZOOM_1X_VALUE			0x0A

/* Photometry */  
#define M4MO_PHOTOMETRY_AVERAGE		1
#define M4MO_PHOTOMETRY_CENTER		2
#define M4MO_PHOTOMETRY_SPOT		3

/* Face Detection */
#define M4MO_FACE_DETECTION_OFF		0
#define M4MO_FACE_DETECTION_ON		1

/* Scene Mode */ 
#define M4MO_SCENE_NORMAL			1
#define M4MO_SCENE_PORTRAIT			2
#define M4MO_SCENE_LANDSCAPE		3
#define M4MO_SCENE_SPORTS			4
#define M4MO_SCENE_PARTY_INDOOR		5
#define M4MO_SCENE_BEACH_SNOW		6
#define M4MO_SCENE_SUNSET			7
#define M4MO_SCENE_DUSK_DAWN		8
#define M4MO_SCENE_FALL_COLOR		9
#define M4MO_SCENE_NIGHT			10
#define M4MO_SCENE_FIREWORK			11	
#define M4MO_SCENE_TEXT				12
#define M4MO_SCENE_CANDLELIGHT		13
#define M4MO_SCENE_BACKLIGHT		14

/* JPEG Quality */ 
#define M4MO_JPEG_SUPERFINE			1
#define M4MO_JPEG_FINE				2
#define M4MO_JPEG_NORMAL			3
#define M4MO_JPEG_ECONOMY			4

/* WDR */
#define M4MO_WDR_OFF				1
#define M4MO_WDR_ON					2

/* Image Stabilization */
#define M4MO_ISC_STILL_OFF			1
#define M4MO_ISC_STILL_ON			2
#define M4MO_ISC_STILL_AUTO			3 /* Not Used */
#define M4MO_ISC_MOVIE_ON			4 /* Not Used */

/* Flash Fire Yes/No */
#define M4MO_FLASH_FIRED			1
#define M4MO_FLASH_NO_FIRED			0

/* Flash Setting */
#define M4MO_FLASH_CAPTURE_OFF		1
#define M4MO_FLASH_CAPTURE_ON		2
#define M4MO_FLASH_CAPTURE_AUTO		3
#define M4MO_FLASH_MOVIE_ON			4
// #define M4MO_FLASH_LEVEL_1PER2		5
// #define M4MO_FLASH_LEVEL_1PER4		6
// #define M4MO_FLASH_LEVEL_1PER6		7
// #define M4MO_FLASH_LEVEL_1PER8		8
// #define M4MO_FLASH_LEVEL_1PER10		9
// #define M4MO_FLASH_LEVEL_1PER12		10
// #define M4MO_FLASH_LEVEL_1PER14		11
// #define M4MO_FLASH_LEVEL_1PER16		12
// #define M4MO_FLASH_LEVEL_1PER18		13
// #define M4MO_FLASH_LEVEL_1PER20		14
// #define M4MO_FLASH_LEVEL_1PER22		15
// #define M4MO_FLASH_LEVEL_1PER24		16
// #define M4MO_FLASH_LEVEL_1PER26		17
// #define M4MO_FLASH_LEVEL_1PER28		18
// #define M4MO_FLASH_LEVEL_1PER30		19
// #define M4MO_FLASH_LEVEL_OFF	    20

#define M4MO_ZOOM_STAGES            30

/**
 * struct m4mo_capture_size - image capture size information
 * @width: image width in pixels
 * @height: image height in pixels
 */
struct m4mo_capture_size {
	unsigned int width;
	unsigned int height;
};

#endif /* ifndef M4MO_H */

