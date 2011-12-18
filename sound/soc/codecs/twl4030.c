/*
 * ALSA SoC TWL4030 codec driver
 *
 * Author:      Steve Sakoman, <steve@sakoman.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/i2c/twl.h>


#include <mach/gpio.h>
#include <plat/mux.h>



#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>

#include <linux/workqueue.h>
#include <linux/wakelock.h>
//#include <mach/archer.h>
#include <mach/nowplus.h>

#include "twl4030.h"
#include "max9877.h"
#include "sec_gain.h"

#define SEC_AUDIO_DEBUG 0


#if SEC_AUDIO_DEBUG
#define P(format,...)\
  printk("[audio:%s]" format "\n", __func__, ## __VA_ARGS__);
#else
#define P(format,...)
#endif

struct delayed_work codec_control_work;
struct delayed_work codec_down_work;

struct wake_lock T2_wakelock;
#define OMAP_GPIO_MIC_SEL               58      //output


/*
 * twl4030 register cache & default register settings
 */
// swin.kim modify register default value
#if 1
static const u8 twl4030_reg_new[TWL4030_CACHEREGNUM] = {
  0x00, /* this register not used		*/
  0x92, /* REG_CODEC_MODE		(0x1)	*/
  0xFF, /* REG_OPTION	        (0x2)   */
  0x00, /* REG_UNKNOWN		    (0x3)	*/
  0x03, /* REG_MICBIAS_CTL	    (0x4)	*/
  0x13, /* REG_ANAMICL		    (0x5)	*/
  0x00, /* REG_ANAMICR		    (0x6)	*/
  0x0a, /* REG_AVADC_CTL		(0x7)	*/
  0x00, /* REG_ADCMICSEL		(0x8)	*/
  0x30, /* REG_DIGMIXING		(0x9)	*/
  0x0c, /* REG_ATXL1PGA		    (0xA)	*/
  0x0c, /* REG_ATXR1PGA		    (0xB)	*/
  0x08, /* REG_AVTXL2PGA		(0xC)	*/
  0x08, /* REG_AVTXR2PGA		(0xD)	*/
  0x01, /* REG_AUDIO_IF		    (0xE)	*/
  0xe1, /* REG_VOICE_IF		    (0xF)	*/
  0x3f, /* REG_ARXR1PGA		    (0x10)	*/
  0x3f, /* REG_ARXL1PGA		    (0x11)	*/
  0x3f, /* REG_ARXR2PGA		    (0x12)	*/
  0x3f, /* REG_ARXL2PGA		    (0x13)	*/
  0x2e, /* REG_VRXPGA           (0x14)  */
  0x00, /* REG_VSTPGA           (0x15)  */
  0x12, /* REG_VRX2ARXPGA		(0x16)	*/
  0x0c, /* REG_AVDAC_CTL		(0x17)	*/
  0x00, /* REG_ARX2VTXPGA		(0x18)	*/
  0x32, /* REG_ARXL1_APGA_CTL	(0x19)	*/
  0x32, /* REG_ARXR1_APGA_CTL	(0x1A)	*/
  0x0b, /* REG_ARXL2_APGA_CTL	(0x1B)	*/
  0x0b, /* REG_ARXR2_APGA_CTL	(0x1C)	*/
  0x00, /* REG_ATX2ARXPGA		(0x1D)	*/
  0x00, /* REG_BT_IF            (0x1E)	*/
  0x55, /* REG_BTPGA            (0x1F)	*/
  0x00, /* REG_BTSTPGA		    (0x20)	*/
  0x24, /* REG_EAR_CTL		    (0x21)	*/
  0x00, /* REG_HS_SEL           (0x22)	*/
  0x00, /* REG_HS_GAIN_SET	    (0x23)	*/
  0x00, /* REG_HS_POPN_SET	    (0x24)	*/
  0x00, /* REG_PREDL_CTL		(0x25)	*/
  0x00, /* REG_PREDR_CTL		(0x26)	*/
  0x00, /* REG_PRECKL_CTL		(0x27)	*/
  0x00, /* REG_PRECKR_CTL		(0x28)	*/
  0x00, /* REG_HFL_CTL		    (0x29)	*/
  0x00, /* REG_HFR_CTL		    (0x2A)	*/
  0x05, /* REG_ALC_CTL		    (0x2B)	*/
  0x00, /* REG_ALC_SET1		    (0x2C)	*/
  0x00, /* REG_ALC_SET2		    (0x2D)	*/
  0x00, /* REG_BOOST_CTL		(0x2E)	*/
  0x00, /* REG_SOFTVOL_CTL	    (0x2F)	*/
  0x13, /* REG_DTMF_FREQSEL	    (0x30)	*/
  0x00, /* REG_DTMF_TONEXT1H	(0x31)	*/
  0x00, /* REG_DTMF_TONEXT1L	(0x32)	*/
  0x00, /* REG_DTMF_TONEXT2H	(0x33)	*/
  0x00, /* REG_DTMF_TONEXT2L	(0x34)	*/
  0x79, /* REG_DTMF_TONOFF	    (0x35)	*/
  0x11, /* REG_DTMF_WANONOFF	(0x36)	*/
  0x00, /* REG_I2S_RX_SCRAMBLE_H	(0x37)	*/
  0x00, /* REG_I2S_RX_SCRAMBLE_M	(0x38)	*/
  0x00, /* REG_I2S_RX_SCRAMBLE_L	(0x39)	*/
  0x16, /* REG_APLL_CTL		    (0x3A)	*/
  0x00, /* REG_DTMF_CTL		    (0x3B)	*/
  0x44, /* REG_DTMF_PGA_CTL2	(0x3C)	*/
  0x69, /* REG_DTMF_PGA_CTL1	(0x3D)	*/
  0x02, /* REG_MISC_SET_1		(0x3E)	*/
  0x00, /* REG_PCMBTMUX		    (0x3F)	*/
  0x00, /* not used             (0x40)	*/
  0x00, /* not used             (0x41)	*/
  0x00, /* not used             (0x42)	*/
  0x20, /* REG_RX_PATH_SEL	    (0x43)	*/
  0x32, /* REG_VDL_APGA_CTL	    (0x44)	*/
  0x00, /* REG_VIBRA_CTL		(0x45)	*/
  0x00, /* REG_VIBRA_SET		(0x46)	*/
  0x00, /* REG_VIBRA_PWM_SET	(0x47)	*/
  0x00, /* REG_ANAMIC_GAIN	    (0x48)	*/
  0x08, /* REG_MISC_SET_2		(0x49)	*/
  0x00, /* REG_SW_SHADOW		(0x4A)	- Shadow, non HW register */
};
#endif

/*
 * twl4030 register cache & default register settings
 */
static const u8 twl4030_reg[TWL4030_CACHEREGNUM] = {
	0x00, /* this register not used		*/
	0x91, /* REG_CODEC_MODE		(0x1)	*/
	0xc3, /* REG_OPTION		    (0x2)	*/
	0x00, /* REG_UNKNOWN		(0x3)	*/
	0x00, /* REG_MICBIAS_CTL	(0x4)	*/
	0x20, /* REG_ANAMICL		(0x5)	*/
	0x00, /* REG_ANAMICR		(0x6)	*/
	0x00, /* REG_AVADC_CTL		(0x7)	*/
	0x00, /* REG_ADCMICSEL		(0x8)	*/
	0x00, /* REG_DIGMIXING		(0x9)	*/
	0x0c, /* REG_ATXL1PGA		(0xA)	*/
	0x0c, /* REG_ATXR1PGA		(0xB)	*/
	0x00, /* REG_AVTXL2PGA		(0xC)	*/
	0x00, /* REG_AVTXR2PGA		(0xD)	*/
	0x01, /* REG_AUDIO_IF		(0xE)	*/
    0x04, /* REG_VOICE_IF		(0xF)	*/
	0x00, /* REG_ARXR1PGA		(0x10)	*/
	0x00, /* REG_ARXL1PGA		(0x11)	*/
	0x6c, /* REG_ARXR2PGA		(0x12)	*/
	0x6c, /* REG_ARXL2PGA		(0x13)	*/
	0x00, /* REG_VRXPGA		    (0x14)	*/
	0x00, /* REG_VSTPGA		    (0x15)	*/
	0x00, /* REG_VRX2ARXPGA		(0x16)	*/
	0x0c, /* REG_AVDAC_CTL		(0x17)	*/
	0x00, /* REG_ARX2VTXPGA		(0x18)	*/
	0x00, /* REG_ARXL1_APGA_CTL	(0x19)	*/
	0x00, /* REG_ARXR1_APGA_CTL	(0x1A)	*/
	0x4b, /* REG_ARXL2_APGA_CTL	(0x1B)	*/
	0x4b, /* REG_ARXR2_APGA_CTL	(0x1C)	*/
	0x00, /* REG_ATX2ARXPGA		(0x1D)	*/
	0x00, /* REG_BT_IF		    (0x1E)	*/
	0x00, /* REG_BTPGA		    (0x1F)	*/
	0x00, /* REG_BTSTPGA		(0x20)	*/
	0x00, /* REG_EAR_CTL		(0x21)	*/
	0x24, /* REG_HS_SEL		    (0x22)	*/
	0x0a, /* REG_HS_GAIN_SET	(0x23)	*/
	0x00, /* REG_HS_POPN_SET	(0x24)	*/
	0x00, /* REG_PREDL_CTL		(0x25)	*/
	0x00, /* REG_PREDR_CTL		(0x26)	*/
	0x00, /* REG_PRECKL_CTL		(0x27)	*/
	0x00, /* REG_PRECKR_CTL		(0x28)	*/
	0x00, /* REG_HFL_CTL		(0x29)	*/
	0x00, /* REG_HFR_CTL		(0x2A)	*/
	0x00, /* REG_ALC_CTL		(0x2B)	*/
	0x00, /* REG_ALC_SET1		(0x2C)	*/
	0x00, /* REG_ALC_SET2		(0x2D)	*/
	0x00, /* REG_BOOST_CTL		(0x2E)	*/
	0x00, /* REG_SOFTVOL_CTL	(0x2F)	*/
	0x00, /* REG_DTMF_FREQSEL	(0x30)	*/
	0x00, /* REG_DTMF_TONEXT1H	(0x31)	*/
	0x00, /* REG_DTMF_TONEXT1L	(0x32)	*/
	0x00, /* REG_DTMF_TONEXT2H	(0x33)	*/
	0x00, /* REG_DTMF_TONEXT2L	(0x34)	*/
	0x00, /* REG_DTMF_TONOFF	(0x35)	*/
	0x00, /* REG_DTMF_WANONOFF	(0x36)	*/
	0x00, /* REG_I2S_RX_SCRAMBLE_H	(0x37)	*/
	0x00, /* REG_I2S_RX_SCRAMBLE_M	(0x38)	*/
	0x00, /* REG_I2S_RX_SCRAMBLE_L	(0x39)	*/
	0x16, /* REG_APLL_CTL		(0x3A)	*/
	0x00, /* REG_DTMF_CTL		(0x3B)	*/
	0x00, /* REG_DTMF_PGA_CTL2	(0x3C)	*/
	0x00, /* REG_DTMF_PGA_CTL1	(0x3D)	*/
	0x00, /* REG_MISC_SET_1		(0x3E)	*/
	0x00, /* REG_PCMBTMUX		(0x3F)	*/
	0x00, /* not used		    (0x40)	*/
	0x00, /* not used		    (0x41)	*/
	0x00, /* not used		    (0x42)	*/
	0x00, /* REG_RX_PATH_SEL	(0x43)	*/
	0x00, /* REG_VDL_APGA_CTL	(0x44)	*/
	0x00, /* REG_VIBRA_CTL		(0x45)	*/
	0x00, /* REG_VIBRA_SET		(0x46)	*/
	0x00, /* REG_VIBRA_PWM_SET	(0x47)	*/
	0x00, /* REG_ANAMIC_GAIN	(0x48)	*/
	0x00, /* REG_MISC_SET_2		(0x49)	*/
	0x00, /* REG_SW_SHADOW		(0x4A)	- Shadow, non HW register */
};



int twl4030_mode = 0;
int twl4030_old_mode = 0;
int twl4030_remap = 0;
int twl4030_call_device = 0;
int twl4030_playback_device = 0;
bool twl4030_codec_suspended = false;
int twl4030_voip_device = 0;	// hskwon-ss-cl31, added for FMC(VoIP) call path
int twl4030_fm_device = 0;
int twl4030_recording_device = 0;
bool twl4030_vr_mode = false;
bool twl4030_fm_radio_mute_enable = false;


static const char *audio_path[]     = {"Playback Path", "Voice Call Path", "Memo Path", "VOIP Call Path", "FM Radio Path", "Idle Mode","Mic Mute","VR Mode", "Loopback Path"};   // hskwon-ss-db05, to support mic mute/unmute for CTS test
static const char *playback_path[]  = {"off","RCV","SPK","HP","SPK_HP"};
static const char *voicecall_path[] = {"off","RCV","SPK","HP","BT"};
static const char *voicememo_path[] = {"off","MAIN","HP"}; 
static const char *voip_path[]    = {"off","RCV","SPK","HP"}; // hskwon-ss-cl31, added for FMC(VoIP) call path
static const char *fmradio_path[]   = {"off","SPK","HP"};
static const char *loopback_path[]=      {"off","RCV","SPK", "HP"};
static const char *idle_mode[]    = {"on","off"};
static const char *mic_mute[]    = {"off", "on"};   // hskwon-ss-db05, to support mic mute/unmute for CTS test
static const char *vr_mode[]    = {"off", "on"};


twl4030_codec_setting playback_off[]={
  {0x00, 0x00}
};
twl4030_codec_setting playback_rcv[]={
  {0x00, 0x00}
};
twl4030_codec_setting playback_spk[]={
  //  {0x0e, 0x01}, //TWL4030_REG_AUDIO_IF
  //  {0x3a, 0x16}, //TWL4030_REG_APLL_CTL
  {0x01, 0x93}, //TWL4030_REG_CODEC_MODE
  {0x13, 0x3f}, //TWL4030_REG_ARXL2PGA
  {0x12, 0x3f}, //TWL4030_REG_ARXR2PGA
  {0x17, 0x0c}, //TWL4030_REG_AVDAC_CTL
  {0x1b, 0x23}, //TWL4030_REG_ARXL2_APGA_CTL 
  {0x1c, 0x23}, //TWL4030_REG_ARXR2_APGA_CTL 
  {0x21, 0x00}, //TWL4030_REG_EAR_CTL
  {0x22, 0x00}, //TWL4030_REG_HS_SEL
  {0x26, 0x28}, //TWL4030_REG_PREDR_CTL
  {0x25, 0x28},  //TWL4030_REG_PREDL_CTL

  {0x43, 0x00}
};
twl4030_codec_setting playback_hp[]={    
  //  {0x0e, 0x01}, //TWL4030_REG_AUDIO_IF
  //  {0x3a, 0x16}, //TWL4030_REG_APLL_CTL
  {0x01, 0x93}, //TWL4030_REG_CODEC_MODE
  {0x13, 0x3f}, //TWL4030_REG_ARXL2PGA
  {0x12, 0x3f}, //TWL4030_REG_ARXR2PGA
  {0x17, 0x0c}, //TWL4030_REG_AVDAC_CTL    
  {0x1b, 0x23}, //TWL4030_REG_ARXL2_APGA_CTL
  {0x1c, 0x23}, //TWL4030_REG_ARXR2_APGA_CTL   
  {0x21, 0x00}, //TWL4030_REG_EAR_CTL  
  {0x23, 0x0A}, //TWL4030_REG_HS_GAIN_SET
  {0x24, 0x41}, //TWL4030_REG_HS_POPN_SET		     
  {0x24, 0x42}, //TWL4030_REG_HS_POPN_SET
  {0x22, 0x24},  //TWL4030_REG_HS_SEL
  {0x26, 0x00}, //TWL4030_REG_PREDR_CTL
  {0x25, 0x00},  //TWL4030_REG_PREDL_CTL

  {0x43, 0x00}
};
twl4030_codec_setting playback_spk_hp[]={
  //  {0x0e, 0x01}, //TWL4030_REG_AUDIO_IF
  //  {0x3a, 0x16}, //TWL4030_REG_APLL_CTL
  {0x01, 0x93}, //TWL4030_REG_CODEC_MODE
  {0x13, 0x3f}, //TWL4030_REG_ARXL2PGA
  {0x12, 0x3f}, //TWL4030_REG_ARXR2PGA
  {0x17, 0x0c}, //TWL4030_REG_AVDAC_CTL   
  {0x1b, 0x23}, //TWL4030_REG_ARXL2_APGA_CTL  
  {0x1c, 0x23}, //TWL4030_REG_ARXR2_APGA_CTL  
  {0x21, 0x00}, //TWL4030_REG_EAR_CTL  
  {0x22, 0x24}, //TWL4030_REG_HS_SEL   
  {0x23, 0x0A}, //TWL4030_REG_HS_GAIN_SET
  {0x24, 0x41}, //TWL4030_REG_HS_POPN_SET       
  {0x24, 0x42}, //TWL4030_REG_HS_POPN_SET   
  {0x25, 0x28}, //TWL4030_REG_PREDL_CTL   
  {0x26, 0x28}, //TWL4030_REG_PREDR_CTL   

  {0x43, 0x00}
};


//voicecall_path {"off","RCV","SPK","HP","BT"};
twl4030_codec_setting voicecall_off[]={ 
  {0x1e, 0x00},  //TWL4030_REG_BT_IF
  {0x20, 0x00},  //TWL4030_REG_BTSTPGA
  {0x3f, 0x00},  //TWL4030_REG_PCMBTMUX
  {0x43, 0x20},  //TWL4030_REG_RX_PATH_SEL
  {0x17, 0x0c}   //TWL4030_REG_AVDAC_CTL     
};

twl4030_codec_setting voicecall_rcv[]={
  {0x01, 0x92},  //TWL4030_REG_ANAMICR	
  //{0x21, 0x34},  //TWL4030_REG_EAR_CTL
  {0x3a, 0x16},  //TWL4030_REG_APLL_CTL
  {0x09, 0x30},  //TWL4030_REG_DIGMIXING
  {0x25, 0x20},  //TWL4030_REG_PREDL_CTL
  {0x26, 0x20},  //TWL4030_REG_PREDR_CTL    
  {0x0e, 0x01},  //TWL4030_REG_AUDIO_IF	 
  {0x0f, 0xe1},  //TWL4030_REG_VOICE_IF
  {0x14, 0x20},  //TWL4030_REG_VRXPGA
  {0x16, 0x19},  //TWL4030_REG_VRX2ARXPGA
  {0x17, 0x08},  //TWL4030_REG_AVDAC_CTL
  {0x1c, 0x1b},  //TWL4030_REG_ARXR2_APGA_CTL    
  {0x1b, 0x1b},  //TWL4030_REG_ARXL2_APGA_CTL

  {0x13, 0x6f},  //TWL4030_REG_ARXL2PGA
  {0x12, 0x6f},  //TWL4030_REG_ARXR2PGA     
  {0x48, 0x22},  //TWL4030_REG_ANAMIC_GAIN

  //reset other path
  {0x22, 0x00},  //TWL4030_REG_HS_SEL
  {0x23, 0x00},  //TWL4030_REG_HS_GAIN_SET
  {0x3f, 0x00},  //TWL4030_REG_PCMBTMUX
  {0x1e, 0x00},  //TWL4030_REG_BT_IF

  // main mic
#if (0)
  {0x04, 0x01},  //TWL4030_REG_MICBIAS_CTL
#else
  {0x04, 0x02},  //TWL4030_REG_MICBIAS_CTL 
#endif


  {0x05, 0x11},  //TWL4030_REG_ANAMICL
  {0x02, 0xf5},  //TWL4030_REG_ANAMICR
  {0x06, 0x00},  //TWL4030_REG_ANAMICR
  {0x07, 0x0a},  //TWL4030_REG_AVADC_CTL	
  {0x08, 0x00},  //TWL4030_REG_ADCMICSEL    

  {0x0c, 0x11},
};


twl4030_codec_setting voicecall_spk[] ={
  {0x01, 0x92},  //TWL4030_REG_CODEC_MODE    
  {0x3a, 0x16},  //TWL4030_REG_APLL_CTL
  //{0x26, 0x28},  //TWL4030_REG_PREDR_CTL
  //{0x25, 0x24},  //TWL4030_REG_PREDL_CTL
  {0x09, 0x30},  //TWL4030_REG_DIGMIXING
  {0x13, 0x7f},  //TWL4030_REG_ARXL2PGA
  {0x12, 0x7f},  //TWL4030_REG_ARXR2PGA
  {0x17, 0x08},  //TWL4030_REG_AVDAC_CTL

  //reset other path
  {0x21, 0x00},  //TWL4030_REG_EAR_CTL
  {0x22, 0x00},  //TWL4030_REG_HS_SEL
  {0x23, 0x00},  //TWL4030_REG_HS_GAIN_SET
  {0x3f, 0x00},  //TWL4030_REG_PCMBTMUX
  {0x1e, 0x00},  //TWL4030_REG_BT_IF

  // main mic
#if (0)
  {0x04, 0x01},  //TWL4030_REG_MICBIAS_CTL
#else
  {0x04, 0x02},  //TWL4030_REG_MICBIAS_CTL
#endif

  {0x05, 0x11},  //TWL4030_REG_ANAMICL
  {0x06, 0x00},  //TWL4030_REG_ANAMICR
  {0x07, 0x0a},  //TWL4030_REG_AVADC_CTL
  {0x08, 0x00},  //TWL4030_REG_ADCMICSEL    
  {0x48, 0x24},  //TWL4030_REG_ANAMIC_GAIN    

  {0x16, 0x17},
  {0x1b, 0x1b},
  {0x1c, 0x1b},
  {0x14, 0x29},
  {0x0c, 0x08},
};

twl4030_codec_setting voicecall_hp3p[]= {
  {0x01, 0x92},  //TWL4030_REG_CODEC_MODE    
  {0x02, 0xf5},  //TWL4030_REG_OPTION
  {0x09, 0x30},  //TWL4030_REG_DIGMIXING	
  {0x0e, 0x01},  //TWL4030_REG_AUDIO_IF    
  {0x12, 0x00},  //TWL4030_REG_ARXR2PGA       
  {0x13, 0x00},  //TWL4030_REG_ARXL2PGA
  {0x17, 0x0c},  //TWL4030_REG_AVDAC_CTL
  {0x1b, 0x1b}, //TWL4030_REG_ARXL2_APGA_CTL
  {0x1c, 0x1b},  //TWL4030_REG_ARXR2_APGA_CTL    
  {0x22, 0x24},  //TWL4030_REG_HS_SEL
  {0x23, 0x0a}, //TWL4030_REG_HS_GAIN_SET  
  {0x24, 0x41},  //TWL4030_REG_HS_POPN_SET     
  {0x24, 0x42}, //TWL4030_REG_HS_POPN_SET
  {0x3a, 0x16},  //TWL4030_REG_APLL_CTL

  //reset other path
  {0x3f, 0x00},  //TWL4030_REG_PCMBTMUX              
  {0x21, 0x00}, //TWL4030_REG_EAR_CTL 	 
  {0x25, 0x00},  //TWL4030_REG_PREDL_CTL
  {0x26, 0x00},  //TWL4030_REG_PREDR_CTL
  {0x1e, 0x00},  //TWL4030_REG_BT_IF

  // main mic
  {0x48, 0x04},  //TWL4030_REG_ANAMIC_GAIN

#if (0)
  {0x04, 0x01},  //TWL4030_REG_MICBIAS_CTL
#else
  {0x04, 0x02},  //TWL4030_REG_MICBIAS_CTL 
#endif

  {0x05, 0x11},  //TWL4030_REG_ANAMICL
  {0x06, 0x00},  //TWL4030_REG_ANAMICR
  {0x07, 0x0a},  //TWL4030_REG_AVADC_CTL	
  {0x08, 0x00},  //TWL4030_REG_ADCMICSEL    

  {0x16, 0x17},
  {0x14, 0x2b},
  {0x0c, 0x08},
};


twl4030_codec_setting voicecall_hp4p[]= {
  {0x01, 0x92},  //TWL4030_REG_CODEC_MODE    
  {0x02, 0xf5},  //TWL4030_REG_OPTION
  {0x09, 0x30},  //TWL4030_REG_DIGMIXING
  {0x0e, 0x01},  //TWL4030_REG_AUDIO_IF    
  {0x12, 0x00},  //TWL4030_REG_ARXR2PGA       
  {0x13, 0x00},  //TWL4030_REG_ARXL2PGA
  {0x17, 0x0c},  //TWL4030_REG_AVDAC_CTL
  {0x1b, 0x1b}, //TWL4030_REG_ARXL2_APGA_CTL
  {0x1c, 0x1b},  //TWL4030_REG_ARXR2_APGA_CTL    
  {0x22, 0x24},  //TWL4030_REG_HS_SEL
  {0x23, 0x0a}, //TWL4030_REG_HS_GAIN_SET       
  {0x24, 0x41},  //TWL4030_REG_HS_POPN_SET     
  {0x24, 0x42}, //TWL4030_REG_HS_POPN_SET
  {0x3a, 0x16},  //TWL4030_REG_APLL_CTL

  //reset other path
  {0x3f, 0x00},  //TWL4030_REG_PCMBTMUX    
  {0x25, 0x00},  //TWL4030_REG_PREDL_CTL
  {0x26, 0x00},  //TWL4030_REG_PREDR_CTL	
  {0x21, 0x00}, //TWL4030_REG_EAR_CTL  
  {0x1e, 0x00},  //TWL4030_REG_BT_IF

  //mic control
  {0x48, 0x04},  //TWL4030_REG_ANAMIC_GAIN
  {0x04, 0x04},  //TWL4030_REG_MICBIAS_CTL 
  {0x05, 0x12},  //TWL4030_REG_ANAMICL
  {0x06, 0x00},  //TWL4030_REG_ANAMICR
  {0x07, 0x0a},  //TWL4030_REG_AVADC_CTL	
  {0x08, 0x00},  //TWL4030_REG_ADCMICSEL   	

  {0x16, 0x17},
  {0x14, 0x2b},
  {0x0c, 0x08},
};


twl4030_codec_setting voicecall_bt[]={
  //RX volume
  {0x1f, 0x5b},  //TWL4030_REG_BTPGA
  //output
  {0x17, 0x00},  //TWL4030_REG_AVDAC_CTL
  {0x43, 0x10},  //TWL4030_REG_RX_PATH_SEL
  {0x3f, 0xa0},  //TWL4030_REG_PCMBTMUX
  {0x1e, 0x61},  //TWL4030_REG_BT_IF
  //reset other path
  {0x22, 0x00},  //TWL4030_REG_HS_SEL
  {0x23, 0x00},  //TWL4030_REG_HS_GAIN_SET
  {0x25, 0x00},  //TWL4030_REG_PREDL_CTL
  {0x26, 0x00},  //TWL4030_REG_PREDR_CTL
  {0x21, 0x00}, //TWL4030_REG_EAR_CTL 
  //input    
  {0x04, 0x00},  //TWL4030_REG_MICBIAS_CTL
  {0x05, 0x00},  //TWL4030_REG_ANAMICL
  {0x06, 0x00},  //TWL4030_REG_ANAMICR
  {0x08, 0x00},  //TWL4030_REG_ADCMICSEL    
};


// hskwon-ss-cl31, added for FMC(VoIP) call path
//voipcall_path {"off","RCV","SPK","HP"};
twl4030_codec_setting voipcall_off[]={ 
  {0x1e, 0x00},  //TWL4030_REG_BT_IF
  {0x20, 0x00},  //TWL4030_REG_BTSTPGA
  {0x3f, 0x00},  //TWL4030_REG_PCMBTMUX
  {0x43, 0x20},  //TWL4030_REG_RX_PATH_SEL
  {0x17, 0x0c}   //TWL4030_REG_AVDAC_CTL     
};

twl4030_codec_setting voipcall_rcv[]={
  {0x01, 0x93},  //TWL4030_REG_CODEC_MODE	
  //{0x21, 0x34},  //TWL4030_REG_EAR_CTL
  {0x3a, 0x16},  //TWL4030_REG_APLL_CTL
  {0x25, 0x20},  //TWL4030_REG_PREDL_CTL
  {0x26, 0x20},  //TWL4030_REG_PREDR_CTL    
  {0x0e, 0x01},  //TWL4030_REG_AUDIO_IF	 
  {0x17, 0x08},  //TWL4030_REG_AVDAC_CTL
  {0x1c, 0x13},  //TWL4030_REG_ARXR2_APGA_CTL    
  {0x1b, 0x13},  //TWL4030_REG_ARXL2_APGA_CTL

  {0x13, 0xb9},  //TWL4030_REG_ARXL2PGA
  {0x12, 0xb9},  //TWL4030_REG_ARXR2PGA     
  {0x48, 0x1b},  //TWL4030_REG_ANAMIC_GAIN

  //reset other path
  {0x22, 0x00},  //TWL4030_REG_HS_SEL
  {0x23, 0x00},  //TWL4030_REG_HS_GAIN_SET
  {0x3f, 0x00},  //TWL4030_REG_PCMBTMUX
  {0x1e, 0x00},  //TWL4030_REG_BT_IF

  // main mic
#if (0)
  {0x04, 0x01},  //TWL4030_REG_MICBIAS_CTL
#else
  {0x04, 0x02},  //TWL4030_REG_MICBIAS_CTL 
#endif

  {0x05, 0x11},  //TWL4030_REG_ANAMICL
  {0x02, 0xf5},  //TWL4030_REG_OPTION
  {0x06, 0x00},  //TWL4030_REG_ANAMICR
  {0x07, 0x0a},  //TWL4030_REG_AVADC_CTL
  {0x08, 0x00},  //TWL4030_REG_ADCMICSEL    

  {0x0c, 0x08},  //TWL4030_REG_AVTXL2PGA 
  {0x21, 0x34}   //TWL4030_REG_EAR_CTL   
};

twl4030_codec_setting voipcall_spk[] ={
  {0x01, 0x93},  //TWL4030_REG_CODEC_MODE    
  {0x3a, 0x16},  //TWL4030_REG_APLL_CTL
  {0x13, 0xbf},  //TWL4030_REG_ARXL2PGA
  {0x12, 0xbf},  //TWL4030_REG_ARXR2PGA
  {0x17, 0x08},  //TWL4030_REG_AVDAC_CTL

  //reset other path
  {0x21, 0x00},  //TWL4030_REG_EAR_CTL
  {0x22, 0x00},  //TWL4030_REG_HS_SEL
  {0x23, 0x00},  //TWL4030_REG_HS_GAIN_SET
  {0x3f, 0x00},  //TWL4030_REG_PCMBTMUX
  {0x1e, 0x00},  //TWL4030_REG_BT_IF

  // main mic
#if (0)
  {0x04, 0x01},  //TWL4030_REG_MICBIAS_CTL
#else
  {0x04, 0x02},  //TWL4030_REG_MICBIAS_CTL
#endif

  {0x05, 0x11},  //TWL4030_REG_ANAMICL
  {0x06, 0x00},  //TWL4030_REG_ANAMICR
  {0x07, 0x0a},  //TWL4030_REG_AVADC_CTL	
  {0x08, 0x00},  //TWL4030_REG_ADCMICSEL    
  {0x48, 0x2d},  //TWL4030_REG_ANAMIC_GAIN    

  {0x1b, 0x0B},
  {0x1c, 0x0B},
  {0x0c, 0x06},

  {0x26, 0x28},  //TWL4030_PREDR_CTL
  {0x25, 0x28}   //TWL4030_PREDL_CTL   
};


twl4030_codec_setting voipcall_hp3p[]= {
  {0x01, 0x93},  //TWL4030_REG_CODEC_MODE    
  {0x02, 0xf5},  //TWL4030_REG_OPTION
  {0x0e, 0x01},  //TWL4030_REG_AUDIO_IF    
  {0x12, 0x3f},  //TWL4030_REG_ARXR2PGA       
  {0x13, 0x3f},  //TWL4030_REG_ARXL2PGA
  {0x17, 0x0c},  //TWL4030_REG_AVDAC_CTL
  {0x1b, 0x13},  //TWL4030_REG_ARXL2_APGA_CTL
  {0x1c, 0x13},  //TWL4030_REG_ARXR2_APGA_CTL    
  {0x22, 0x24},  //TWL4030_REG_HS_SEL
  {0x23, 0x0a},  //TWL4030_REG_HS_GAIN_SET  
  {0x24, 0x41},  //TWL4030_REG_HS_POPN_SET     
  {0x24, 0x42},  //TWL4030_REG_HS_POPN_SET
  {0x3a, 0x16},  //TWL4030_REG_APLL_CTL

  //reset other path
  {0x3f, 0x00},  //TWL4030_REG_PCMBTMUX              
  {0x21, 0x00},  //TWL4030_REG_EAR_CTL 	 
  {0x25, 0x00},  //TWL4030_REG_PREDL_CTL
  {0x26, 0x00},  //TWL4030_REG_PREDR_CTL
  {0x1e, 0x00},  //TWL4030_REG_BT_IF

  //mic control
#if (0)
  {0x04, 0x01},  //TWL4030_REG_MICBIAS_CTL
#else
  {0x04, 0x02},  //TWL4030_REG_MICBIAS_CTL 
#endif

  {0x48, 0x24},  //TWL4030_REG_ANAMIC_GAIN    
  {0x05, 0x11},  //TWL4030_REG_ANAMICL
  {0x06, 0x00},  //TWL4030_REG_ANAMICR
  {0x07, 0x0a},  //TWL4030_REG_AVADC_CTL	
  {0x08, 0x00},  //TWL4030_REG_ADCMICSEL        

  {0x0c, 0x08},
};


twl4030_codec_setting voipcall_hp4p[]= {
  {0x01, 0x93},  //TWL4030_REG_CODEC_MODE    
  {0x02, 0xf5},  //TWL4030_REG_OPTION
  {0x0e, 0x01},  //TWL4030_REG_AUDIO_IF    
  {0x12, 0x3f},  //TWL4030_REG_ARXR2PGA       
  {0x13, 0x3f},  //TWL4030_REG_ARXL2PGA
  {0x17, 0x0c},  //TWL4030_REG_AVDAC_CTL
  {0x1b, 0x13},  //TWL4030_REG_ARXL2_APGA_CTL
  {0x1c, 0x13},  //TWL4030_REG_ARXR2_APGA_CTL    
  {0x22, 0x24},  //TWL4030_REG_HS_SEL
  {0x23, 0x0a},  //TWL4030_REG_HS_GAIN_SET       
  {0x24, 0x41},  //TWL4030_REG_HS_POPN_SET     
  {0x24, 0x42},  //TWL4030_REG_HS_POPN_SET
  {0x3a, 0x16},  //TWL4030_REG_APLL_CTL

  //reset other path
  {0x3f, 0x00},  //TWL4030_REG_PCMBTMUX    
  {0x25, 0x00},  //TWL4030_REG_PREDL_CTL
  {0x26, 0x00},  //TWL4030_REG_PREDR_CTL	
  {0x21, 0x00},  //TWL4030_REG_EAR_CTL  
  {0x1e, 0x00},  //TWL4030_REG_BT_IF

  //mic control
  {0x48, 0x2D},  //TWL4030_REG_ANAMIC_GAIN
  {0x04, 0x04},  //TWL4030_REG_MICBIAS_CTL 
  {0x05, 0x12},  //TWL4030_REG_ANAMICL
  {0x06, 0x00},  //TWL4030_REG_ANAMICR
  {0x07, 0x0a},  //TWL4030_REG_AVADC_CTL	
  {0x08, 0x00},  //TWL4030_REG_ADCMICSEL   	

  {0x0c, 0x06},
};
// hskwon-ss-cl31, added for FMC(VoIP) call path

twl4030_codec_setting fmradio_off[] ={
  {0x01, 0x92},  //TWL4030_REG_CODEC_MODE    
  //{0x3a, 0x16},  //TWL4030_REG_APLL_CTL

  //{0x04, 0x00},  //TWL4030_REG_MICBIAS_CTL
  {0x05,0x14},		//KHoAudioANAMICL mic ampL enable, AUXL enable
  {0x06,0x14},		//KHoAudioANAMICR mic ampR enable, AUXR enable
  {0x07,0x00},		//KHoAudioAVADC_CTL
  {0x08, 0x00},		//TWL4030_REG_ADCMICSEL    
  //{0x02,0x00},		//KHoAudioOPTION
  {0x3e,0x20},		//KHoAudioMISC_SET_1
 // {0x17, 0x0c},  // not used in galax a
  {0x13, 0x7f},  //TWL4030_REG_ARXL2PGA
  {0x12, 0x7f},  //TWL4030_REG_ARXR2PGA

  //reset other path
  {0x21, 0x00},  //TWL4030_REG_EAR_CTL		
  {0x3f, 0x00},  //TWL4030_REG_PCMBTMUX	

  {0x1c, 0x07},  //TWL4030_REG_ARXR2_APGA_CTL    
  {0x1b, 0x07},  //TWL4030_REG_ARXL2_APGA_CTL

};
    
twl4030_codec_setting fmradio_spk[] ={
  {0x01, 0x92},  //TWL4030_REG_CODEC_MODE    
  //{0x3a, 0x16},  //TWL4030_REG_APLL_CTL

  //{0x04, 0x00},  //TWL4030_REG_MICBIAS_CTL
  {0x05,0x14},		//KHoAudioANAMICL mic ampL enable, AUXL enable
  {0x06,0x14},		//KHoAudioANAMICR mic ampR enable, AUXR enable
  {0x07,0x00},		//KHoAudioAVADC_CTL
  {0x08, 0x00},		//TWL4030_REG_ADCMICSEL    
  //{0x02,0x00},		//KHoAudioOPTION
  {0x3e,0x20},		//KHoAudioMISC_SET_1
 // {0x17, 0x0c},  // not used in galax a
  {0x13, (0x2 << 6) | 0x3f},  //TWL4030_REG_ARXL2PGA
  {0x12, (0x2 << 6) | 0x3f},  //TWL4030_REG_ARXR2PGA

  //reset other path
  {0x21, 0x00},  //TWL4030_REG_EAR_CTL		
  {0x3f, 0x00},  //TWL4030_REG_PCMBTMUX	

  {0x1c, (0x9 << 3)| 0x07},  //TWL4030_REG_ARXR2_APGA_CTL    
  {0x1b, (0x9 << 3)| 0x07},  //TWL4030_REG_ARXL2_APGA_CTL

  {0x26, (0x3<<4)|0x7},  //TWL4030_PREDR_CTL
  {0x25, (0x3<<4)|0x7},   //TWL4030_PREDL_CTL   
   
  {0x48, (0x4 << 3) | 0x4},   // ANAMIC_GAIN (L[0:2], R[3:5])
  

};

twl4030_codec_setting fmradio_hp[]= {
  {0x01, 0x92},  //TWL4030_REG_CODEC_MODE    
  //{0x3a, 0x16},  //TWL4030_REG_APLL_CTL
  //{0x04, 0x00},  //TWL4030_REG_MICBIAS_CTL

  {0x05,0x14},	//KHoAudioANAMICL mic ampL enable, AUXL enable
  {0x06,0x14},		//KHoAudioANAMICR mic ampR enable, AUXR enable
  {0x07,0x00},		//KHoAudioAVADC_CTL
  {0x08, 0x00},  //TWL4030_REG_ADCMICSEL    
  //{0x02,0x00},		//KHoAudioOPTION
  {0x3e,0x20},		//KHoAudioMISC_SET_1
 // {0x17, 0x0c},  // not used in galax a

  {0x13, 0x7f},  //TWL4030_REG_ARXL2PGA
  {0x12, 0x7f},  //TWL4030_REG_ARXR2PGA

  //reset other path
  {0x21, 0x00},  //TWL4030_REG_EAR_CTL		
  {0x3f, 0x00},  //TWL4030_REG_PCMBTMUX		

  {0x1c, 0x07},  //TWL4030_REG_ARXR2_APGA_CTL    
  {0x1b, 0x07},  //TWL4030_REG_ARXL2_APGA_CTL

  {0x22, 0x24},  //TWL4030_REG_HS_SEL    
  {0x24, 0x41},  //TWL4030_REG_HS_POPN_SET     
  {0x24, 0x42}, //TWL4030_REG_HS_POPN_SET
  
  {0x48, (0x1<<3)|0x1},   // ANAMIC_GAIN (L[0:2], R[3:5])
};

twl4030_codec_setting volume_off[]={ 
  {0x44, 0x00},  //TWL4030_REG_VDL_APGA_CTL
  {0x1b, 0x00},  //TWL4030_REG_ARXL2_APGA_CTL
  {0x1c, 0x00},  //TWL4030_REG_ARXR2_APGA_CTL
  {0x22, 0x00},  //TWL4030_REG_HS_SEL  
  {0x22, 0x00},  //TWL4030_REG_HS_SEL     
  {0x25, 0x00},  //TWL4030_REG_PREDL_CTL
  {0x26, 0x00},  //TWL4030_REG_PREDR_CTL  
};


//set voip routing for loopback dealy
// recording and play was controled by app
twl4030_codec_setting Loopback_rcv[]={
  {0x01, 0x93},  //TWL4030_REG_ANAMICR	
   {0x3a, 0x16},  //TWL4030_REG_APLL_CTL
  {0x09, 0x10},  //TWL4030_REG_DIGMIXING
  {0x25, 0x20},  //TWL4030_REG_PREDL_CTL
  {0x26, 0x20},  //TWL4030_REG_PREDR_CTL    
  {0x0e, 0x01},  //TWL4030_REG_AUDIO_IF	 
  {0x0f, 0xe1},  //TWL4030_REG_VOICE_IF
  {0x14, 0x2e},  //TWL4030_REG_VRXPGA
  {0x16, 0x12},  //TWL4030_REG_VRX2ARXPGA
  {0x17, 0x08},  //TWL4030_REG_AVDAC_CTL
  {0x1c, 0x1b},  //TWL4030_REG_ARXR2_APGA_CTL    
  {0x1b, 0x1b},  //TWL4030_REG_ARXL2_APGA_CTL

  {0x13, 0x6f},  //TWL4030_REG_ARXL2PGA
  {0x12, 0x6f},  //TWL4030_REG_ARXR2PGA     
  {0x48, 0x24},  //TWL4030_REG_ANAMIC_GAIN

  //reset other path
  {0x22, 0x00},  //TWL4030_REG_HS_SEL
  {0x23, 0x00},  //TWL4030_REG_HS_GAIN_SET
  {0x3f, 0x00},  //TWL4030_REG_PCMBTMUX
  {0x1e, 0x00},  //TWL4030_REG_BT_IF

  // main mic
  {0x04, 0x02},  //TWL4030_REG_MICBIAS_CTL 

  {0x05, 0x11},  //TWL4030_REG_ANAMICL
  {0x02, 0xf5},  //TWL4030_REG_ANAMICR
  {0x06, 0x00},  //TWL4030_REG_ANAMICR
  {0x07, 0x0a},  //TWL4030_REG_AVADC_CTL
  {0x08, 0x00},  //TWL4030_REG_ADCMICSEL    

  {0x0a, 0x0c},

  {0x21, 0x34}   //TWL4030_REG_EAR_CTL   
};


twl4030_codec_setting Loopback_spk[]={
  {0x01, 0x92},  //TWL4030_REG_CODEC_MODE    
  {0x3a, 0x16},  //TWL4030_REG_APLL_CTL
  {0x09, 0x30},  //TWL4030_REG_DIGMIXING
  {0x13, 0x7f},  //TWL4030_REG_ARXL2PGA
  {0x12, 0x7f},  //TWL4030_REG_ARXR2PGA
  {0x17, 0x08},  //TWL4030_REG_AVDAC_CTL

  //reset other path
  {0x21, 0x00},  //TWL4030_REG_EAR_CTL
  {0x22, 0x00},  //TWL4030_REG_HS_SEL
  {0x23, 0x00},  //TWL4030_REG_HS_GAIN_SET
  {0x3f, 0x00},  //TWL4030_REG_PCMBTMUX
  {0x1e, 0x00},  //TWL4030_REG_BT_IF

  {0x05, 0x11},  //TWL4030_REG_ANAMICL
  {0x06, 0x00},  //TWL4030_REG_ANAMICR
  {0x07, 0x0a},  //TWL4030_REG_AVADC_CTL
  {0x08, 0x00},  //TWL4030_REG_ADCMICSEL    
  {0x48, 0x12},  //TWL4030_REG_ANAMIC_GAIN    

  {0x16, 0x17},
  
  // main mic
  {0x04, 0x02},  //TWL4030_REG_MICBIAS_CTL 
  
  //for loopback
  {0x3e, 0x20},	  //KHoAudioMISC_SET_1
  {0x1c, 0x25},  //TWL4030_REG_ARXR2_APGA_CTL 4dB
  {0x1b, 0x25},  //TWL4030_REG_ARXL2_APGA_CTL 4dB
  
  {0x14, 0x29},
  {0x0c, 0x08},
  {0x26, 0x24},  //TWL4030_PREDR_CTL
  {0x25, 0x24}   //TWL4030_PREDL_CTL 
};

twl4030_codec_setting Loopback_headset[]={
  {0x01, 0x93},  //TWL4030_REG_CODEC_MODE    
  {0x02, 0xf5},  //TWL4030_REG_OPTION
  {0x09, 0x00},  //TWL4030_REG_DIGMIXING
  {0x0e, 0x01},  //TWL4030_REG_AUDIO_IF    
  {0x12, 0x6f},  //TWL4030_REG_ARXR2PGA       
  {0x13, 0x6f},  //TWL4030_REG_ARXL2PGA
  {0x17, 0x0c},  //TWL4030_REG_AVDAC_CTL

 //for loopback
  {0x3e, 0x20},	  //KHoAudioMISC_SET_1
  {0x1c, 0x25},  //TWL4030_REG_ARXR2_APGA_CTL 4dB
  {0x1b, 0x25},  //TWL4030_REG_ARXL2_APGA_CTL 4dB
  {0x44, 0x25},  //TWL4030_REG_ARXL2_APGA_CTL 4dB
  
  //{0x22, 0x24},  //TWL4030_REG_HS_SEL
  {0x22, 0x2c},  //TWL4030_REG_HS_SEL
  {0x23, 0x05},  //TWL4030_REG_HS_GAIN_SET       
  {0x24, 0x41},  //TWL4030_REG_HS_POPN_SET     
  {0x24, 0x42},  //TWL4030_REG_HS_POPN_SET
  {0x3a, 0x16},  //TWL4030_REG_APLL_CTL

  //reset other path
  {0x3f, 0x00},  //TWL4030_REG_PCMBTMUX    
  {0x25, 0x00},  //TWL4030_REG_PREDL_CTL
  {0x26, 0x00},  //TWL4030_REG_PREDR_CTL	
  {0x21, 0x00},  //TWL4030_REG_EAR_CTL  
  {0x1e, 0x00},  //TWL4030_REG_BT_IF

  //mic control
  {0x48, 0x04},  //TWL4030_REG_ANAMIC_GAIN
  {0x04, 0x02},  //TWL4030_REG_MICBIAS_CTL 
  {0x05, 0x12},  //TWL4030_REG_ANAMICL
  {0x06, 0x00},  //TWL4030_REG_ANAMICR
  {0x07, 0x0a},  //TWL4030_REG_AVADC_CTL	
  {0x08, 0x00},  //TWL4030_REG_ADCMICSEL   	

};
struct substream_item {
  struct list_head started;
  struct list_head configured;
  struct snd_pcm_substream *substream;
  int use256FS;
};

/* codec private data */
struct twl4030_priv {
  struct mutex mutex;

  unsigned int extClock;
  unsigned int bypass_state;
  unsigned int codec_muted;

  struct list_head started_list;
  struct list_head config_list;

  unsigned int sysclk;

  /* Headset output state handling */
  unsigned int hsl_enabled;
  unsigned int hsr_enabled;

  struct snd_pcm_hw_params params;
};

int twl4030_get_voicecall_state(void)
{
	if(twl4030_remap== 1)
		return 1;
	else
		return 0;
}

EXPORT_SYMBOL_GPL(twl4030_get_voicecall_state);

static void twl4030_set_remap(void)
{
	u8 data = 0x00; 

	twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_ACTIVE , TWL4030_VINTANA1_REMAP ); // VINTANA1
	twl_i2c_read_u8(TWL4030_MODULE_PM_RECEIVER, &data, TWL4030_VINTANA1_REMAP );
	P("after setting voicecall path : VINTANA1 = %x\n", data);

	twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_ACTIVE , TWL4030_VINTANA2_REMAP ); // VINTANA2
	twl_i2c_read_u8(TWL4030_MODULE_PM_RECEIVER, &data, TWL4030_VINTANA2_REMAP );
	P("after setting voicecall path : VINTANA2 = %x\n", data);

	twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_ACTIVE , TWL4030_VINTDIG_REMAP ); // EVINTDIG
	twl_i2c_read_u8(TWL4030_MODULE_PM_RECEIVER, &data, TWL4030_VINTDIG_REMAP );
	P("after setting voicecall path : VINTDIG = %x\n", data);

	twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_ACTIVE , TWL4030_CLKEN_REMAP ); // EVCLKEN
	twl_i2c_read_u8(TWL4030_MODULE_PM_RECEIVER, &data, TWL4030_CLKEN_REMAP );
	P("after setting voicecall path : CLKEN = %x\n", data);

	twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_ACTIVE , TWL4030_HFCLKOUT_REMAP ); // EVHFCLKOUT
	twl_i2c_read_u8(TWL4030_MODULE_PM_RECEIVER, &data, TWL4030_HFCLKOUT_REMAP );
	P("after setting voicecall path : HFCLKEN = %x\n", data);

	twl4030_remap = 1;
}

static void twl4030_unset_remap(void)
{
	if(twl4030_remap)
	{      
		u8 data = 0x00; 
		int ret = 0;
		int retn = 0;

		P("IDLE mode - Power resources has to be unset \n");

		/*
		   twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_OFF , TWL4030_VINTANA1_REMAP ); // VINTANA1
		   twl_i2c_read_u8(TWL4030_MODULE_PM_RECEIVER,&data, TWL4030_VINTANA1_REMAP );
		   printk("after setting idle path : VINTANA1 = %x\n", data);
		*/

		ret = twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_OFF , TWL4030_VINTANA2_REMAP ); // VINTANA2
		retn = twl_i2c_read_u8(TWL4030_MODULE_PM_RECEIVER, &data, TWL4030_VINTANA2_REMAP );
		P("after setting idle path : VINTANA2 = %x, ret = %d, retn = %d\n", data, ret, retn);

		ret = twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_SLEEP , TWL4030_VINTDIG_REMAP ); // EVINTDIG
		retn = twl_i2c_read_u8(TWL4030_MODULE_PM_RECEIVER, &data, TWL4030_VINTDIG_REMAP );
		P("after setting idle path : VINTDIG = %x, ret = %d, retn = %d\n", data, ret, retn);

		ret = twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_OFF , TWL4030_CLKEN_REMAP ); // EVCLKEN
		retn = twl_i2c_read_u8(TWL4030_MODULE_PM_RECEIVER, &data, TWL4030_CLKEN_REMAP );
		P("after setting idle path : CLKEN = %x, ret = %d, retn = %d\n", data, ret, retn);

		ret = twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_OFF , TWL4030_HFCLKOUT_REMAP ); // EVHFCLKOUT
		retn = twl_i2c_read_u8(TWL4030_MODULE_PM_RECEIVER, &data, TWL4030_HFCLKOUT_REMAP );
		P("after setting idle path : HFCLKEN = %x, ret = %d, retn = %d\n", data, ret, retn);

		twl4030_remap = 0;
	}
	else 
	{
		P("IDLE mode - Power resources are not set\n");
	}
}


static void codec_control_work_handler(struct work_struct *work )
{
	P("");

	switch(twl4030_call_device)
	{
		  case OFF: 
              break;

		  case RCV:
              twl_i2c_write_u8(TWL4030_MODULE_AUDIO_VOICE, 0x35, 0x21);
              break;         

		  case SPK:
              twl_i2c_write_u8(TWL4030_MODULE_AUDIO_VOICE, 0x28, 0x26);
              twl_i2c_write_u8(TWL4030_MODULE_AUDIO_VOICE, 0x24, 0x25);
			  break;            

		  case HP3P:
		  case HP4P:
              twl_i2c_write_u8(TWL4030_MODULE_AUDIO_VOICE, 0x30, 0x09);
              twl_i2c_write_u8(TWL4030_MODULE_AUDIO_VOICE, 0x6f, 0x12);
              twl_i2c_write_u8(TWL4030_MODULE_AUDIO_VOICE, 0x6f, 0x13);
              twl_i2c_write_u8(TWL4030_MODULE_AUDIO_VOICE, 0x1b, 0x1b);
              twl_i2c_write_u8(TWL4030_MODULE_AUDIO_VOICE, 0x1b, 0x1c);
              break;                                 

		  case BT:
              //PMBR1
              twl_i2c_write_u8(TWL4030_MODULE_INTBR, 0x40, 0x0d);
              break;                    

		  default:
              P("!!!!codec_control_work_handle setting failr!!!\n");
              break;        
	}

	wake_unlock( &T2_wakelock ); 	

}

static void codec_down_work_handler(struct work_struct *work )
{
	u8 mode = 0;

	P("");

	mode &= ~TWL4030_CODECPDZ;

	twl_i2c_write_u8(TWL4030_MODULE_AUDIO_VOICE, mode, TWL4030_REG_CODEC_MODE);
	printk("!!!! codec bias off !!!\n");
}



/*
 * read twl4030 register cache
 */
static inline unsigned int twl4030_read_reg_cache(struct snd_soc_codec *codec,
	unsigned int reg)
{
	u8 *cache = codec->reg_cache;

	if (reg >= TWL4030_CACHEREGNUM)
		return -EIO;

	return cache[reg];
}

/*
 * write twl4030 register cache
 */
static inline void twl4030_write_reg_cache(struct snd_soc_codec *codec,
						u8 reg, u8 value)
{
	u8 *cache = codec->reg_cache;

	if (reg >= TWL4030_CACHEREGNUM)
		return;
		
	cache[reg] = value;
}

/*
 * write to the twl4030 register space
 */
/*static*/ int twl4030_write(struct snd_soc_codec *codec,
			unsigned int reg, unsigned int value)
{
     twl4030_write_reg_cache(codec, reg, value);
   
     if (likely(reg < TWL4030_REG_SW_SHADOW))
     {
         u8 read_val = 0x00;

         twl_i2c_write_u8(TWL4030_MODULE_AUDIO_VOICE, value, reg);
         //printk("twl4030_write reg = 0x%x value = 0x%x \n", reg, value);       

         /* modify by swin.kim to trace T2 Register setting */
         twl_i2c_read_u8(TWL4030_MODULE_AUDIO_VOICE, &read_val, reg);
         //printk(" Offset 0x%02x : 0x%02x \n", reg, value);
         return 0;
     }
     else
         return 0;
}

/*
 * modified to the twl4030 register space
 */
int twl4030_modify(struct snd_soc_codec *codec,
  unsigned int reg, unsigned int value, unsigned int mask)
{
     u8 *cache = codec->reg_cache;
   
     if (reg >= TWL4030_CACHEREGNUM)
        return -EIO;
   
     value = ((cache[reg]&~mask)|(value&mask));
     P("ID : 0x%x, Value : 0x%x ",reg,value );
   
     return twl4030_write(codec, reg, value);
}

static void twl4030_codec_enable(struct snd_soc_codec *codec, int enable)
{
	 u8 mode;
     // GK Changes
     unsigned int i=0;
   
     P(" enable = %d ", enable);

     //if (enable == twl4030->codec_powered)
     //	return;

     mode = twl4030_read_reg_cache(codec, TWL4030_REG_CODEC_MODE);
   
     if (enable)
     {
         mode |= TWL4030_CODECPDZ;
         cancel_delayed_work( &codec_down_work );		
     }
     else
     {
         if(!twl4030_codec_suspended)
         {
              for(i=0;i<ARRAY_SIZE(volume_off);i++)
              {
                   twl4030_write(codec, volume_off[i].reg,volume_off[i].value);                        
              }
        
              //schedule_delayed_work( &codec_down_work, 100 );
              mode &= ~TWL4030_CODECPDZ;
              twl4030_write(codec, TWL4030_REG_CODEC_MODE, mode);
              
              return;
         }
         else
         {
              mode &= ~TWL4030_CODECPDZ;
              cancel_delayed_work( &codec_down_work );
         }
     }
	
     twl4030_write(codec, TWL4030_REG_CODEC_MODE, mode);

	 /* REVISIT: this delay is present in TI sample drivers */
	 /* but there seems to be no TRM requirement for it     */
	 udelay(10);
}

static void twl4030_init_chip(struct snd_soc_codec *codec)
{
	int i;

    P("");

	/* clear CODECPDZ prior to setting register defaults */
	twl4030_codec_enable(codec, 0);

    /* set all audio section registers to reasonable defaults */
    for (i = TWL4030_REG_OPTION; i <= TWL4030_REG_MISC_SET_2; i++)
         twl4030_write(codec, i, twl4030_reg_new[i]);
}

static void twl4030_codec_mute(struct snd_soc_codec *codec, int mute)
{
	 struct twl4030_priv *twl4030 = codec->private_data;
     u8 reg_val = 0x00;
   
     P("mute : %d", mute);

	 if (mute == twl4030->codec_muted)
	     return;

     if (mute) 
     {
          /* Bypass the reg_cache and mute the volumes
          * Headset mute is done in it's own event handler
          * Things to mute:  Earpiece, PreDrivL/R, CarkitL/R
          */
          reg_val = twl4030_read_reg_cache(codec, TWL4030_REG_EAR_CTL);
          twl_i2c_write_u8(TWL4030_MODULE_AUDIO_VOICE,reg_val & (~TWL4030_EAR_GAIN),TWL4030_REG_EAR_CTL);
      
          reg_val = twl4030_read_reg_cache(codec, TWL4030_REG_PREDL_CTL);
          twl_i2c_write_u8(TWL4030_MODULE_AUDIO_VOICE,reg_val & (~TWL4030_PREDL_GAIN),TWL4030_REG_PREDL_CTL);
      
          reg_val = twl4030_read_reg_cache(codec, TWL4030_REG_PREDR_CTL);
          twl_i2c_write_u8(TWL4030_MODULE_AUDIO_VOICE,reg_val & (~TWL4030_PREDR_GAIN),TWL4030_REG_PREDL_CTL);
      
          // add swin.kim for Headset output ctr 
          reg_val = twl4030_read_reg_cache(codec, TWL4030_REG_HS_GAIN_SET);
          twl_i2c_write_u8(TWL4030_MODULE_AUDIO_VOICE,reg_val & (~TWL4030_REG_HS_GAIN_SET),TWL4030_REG_HS_GAIN_SET);
      
          /* Disable PLL */
          reg_val = twl4030_read_reg_cache(codec, TWL4030_REG_APLL_CTL);
          reg_val &= ~TWL4030_APLL_EN;
          twl4030_write(codec, TWL4030_REG_APLL_CTL, reg_val);
     } 
     else 
     {
          /* Restore the volumes
          * Headset mute is done in it's own event handler
          * Things to restore:  Earpiece, PreDrivL/R, CarkitL/R
          */
          twl4030_write(codec, TWL4030_REG_EAR_CTL,twl4030_read_reg_cache(codec, TWL4030_REG_EAR_CTL));
          twl4030_write(codec, TWL4030_REG_PREDL_CTL,twl4030_read_reg_cache(codec, TWL4030_REG_PREDL_CTL));
          twl4030_write(codec, TWL4030_REG_PREDR_CTL,twl4030_read_reg_cache(codec, TWL4030_REG_PREDR_CTL));
          // add swin.kim for Headset output ctr 
          twl4030_write(codec, TWL4030_REG_HS_GAIN_SET,twl4030_read_reg_cache(codec, TWL4030_REG_HS_GAIN_SET));
      
          /* Enable PLL */
          reg_val = twl4030_read_reg_cache(codec, TWL4030_REG_APLL_CTL);
          reg_val |= TWL4030_APLL_EN;
          twl4030_write(codec, TWL4030_REG_APLL_CTL, reg_val);
	}

	twl4030->codec_muted = mute;

}

#if 0
static void twl4030_power_up(struct snd_soc_codec *codec)
{
	u8 anamicl, regmisc1, byte;
	int i = 0;

     P("");

	/* set CODECPDZ to turn on codec */
	twl4030_codec_enable(codec, 1);

	/* initiate offset cancellation */
	anamicl = twl4030_read_reg_cache(codec, TWL4030_REG_ANAMICL);
	twl4030_write(codec, TWL4030_REG_ANAMICL,
		anamicl | TWL4030_CNCL_OFFSET_START);

	/* wait for offset cancellation to complete */
	do {
		/* this takes a little while, so don't slam i2c */
		udelay(2000);
		twl_i2c_read_u8(TWL4030_MODULE_AUDIO_VOICE, &byte,
				    TWL4030_REG_ANAMICL);
	} while ((i++ < 100) &&
		 ((byte & TWL4030_CNCL_OFFSET_START) ==
		  TWL4030_CNCL_OFFSET_START));

	/* Make sure that the reg_cache has the same value as the HW */
	twl4030_write_reg_cache(codec, TWL4030_REG_ANAMICL, byte);

	/* anti-pop when changing analog gain */
	regmisc1 = twl4030_read_reg_cache(codec, TWL4030_REG_MISC_SET_1);
	twl4030_write(codec, TWL4030_REG_MISC_SET_1,
		regmisc1 | TWL4030_SMOOTH_ANAVOL_EN);

	/* toggle CODECPDZ as per TRM */
	twl4030_codec_enable(codec, 0);
	twl4030_codec_enable(codec, 1);
}
#endif

/*
 * Unconditional power down
 */
static void twl4030_power_down(struct snd_soc_codec *codec)
{
    P("");
	/* power down */
    printk("twl4030_write twl4030_power_down \n");       
	twl4030_codec_enable(codec, 0);
}

/* Earpiece */
static const struct snd_kcontrol_new twl4030_dapm_earpiece_controls[] = {
	SOC_DAPM_SINGLE("Voice", TWL4030_REG_EAR_CTL, 0, 1, 0),
	SOC_DAPM_SINGLE("AudioL1", TWL4030_REG_EAR_CTL, 1, 1, 0),
	SOC_DAPM_SINGLE("AudioL2", TWL4030_REG_EAR_CTL, 2, 1, 0),
	SOC_DAPM_SINGLE("AudioR1", TWL4030_REG_EAR_CTL, 3, 1, 0),
};

/* PreDrive Left */
static const struct snd_kcontrol_new twl4030_dapm_predrivel_controls[] = {
	SOC_DAPM_SINGLE("Voice", TWL4030_REG_PREDL_CTL, 0, 1, 0),
	SOC_DAPM_SINGLE("AudioL1", TWL4030_REG_PREDL_CTL, 1, 1, 0),
	SOC_DAPM_SINGLE("AudioL2", TWL4030_REG_PREDL_CTL, 2, 1, 0),
	SOC_DAPM_SINGLE("AudioR2", TWL4030_REG_PREDL_CTL, 3, 1, 0),
};

/* PreDrive Right */
static const struct snd_kcontrol_new twl4030_dapm_predriver_controls[] = {
	SOC_DAPM_SINGLE("Voice", TWL4030_REG_PREDR_CTL, 0, 1, 0),
	SOC_DAPM_SINGLE("AudioR1", TWL4030_REG_PREDR_CTL, 1, 1, 0),
	SOC_DAPM_SINGLE("AudioR2", TWL4030_REG_PREDR_CTL, 2, 1, 0),
	SOC_DAPM_SINGLE("AudioL2", TWL4030_REG_PREDR_CTL, 3, 1, 0),
};

/* Headset Left */
static const struct snd_kcontrol_new twl4030_dapm_hsol_controls[] = {
	SOC_DAPM_SINGLE("Voice", TWL4030_REG_HS_SEL, 0, 1, 0),
	SOC_DAPM_SINGLE("AudioL1", TWL4030_REG_HS_SEL, 1, 1, 0),
	SOC_DAPM_SINGLE("AudioL2", TWL4030_REG_HS_SEL, 2, 1, 0),
};

/* Headset Right */
static const struct snd_kcontrol_new twl4030_dapm_hsor_controls[] = {
	SOC_DAPM_SINGLE("Voice", TWL4030_REG_HS_SEL, 3, 1, 0),
	SOC_DAPM_SINGLE("AudioR1", TWL4030_REG_HS_SEL, 4, 1, 0),
	SOC_DAPM_SINGLE("AudioR2", TWL4030_REG_HS_SEL, 5, 1, 0),
};

/* Carkit Left */
static const struct snd_kcontrol_new twl4030_dapm_carkitl_controls[] = {
	SOC_DAPM_SINGLE("Voice", TWL4030_REG_PRECKL_CTL, 0, 1, 0),
	SOC_DAPM_SINGLE("AudioL1", TWL4030_REG_PRECKL_CTL, 1, 1, 0),
	SOC_DAPM_SINGLE("AudioL2", TWL4030_REG_PRECKL_CTL, 2, 1, 0),
};

/* Carkit Right */
static const struct snd_kcontrol_new twl4030_dapm_carkitr_controls[] = {
	SOC_DAPM_SINGLE("Voice", TWL4030_REG_PRECKR_CTL, 0, 1, 0),
	SOC_DAPM_SINGLE("AudioR1", TWL4030_REG_PRECKR_CTL, 1, 1, 0),
	SOC_DAPM_SINGLE("AudioR2", TWL4030_REG_PRECKR_CTL, 2, 1, 0),
};

/* Handsfree Left */
static const char *twl4030_handsfreel_texts[] =
{"Voice", "AudioL1", "AudioL2", "AudioR2"};

static const struct soc_enum twl4030_handsfreel_enum =
	SOC_ENUM_SINGLE(TWL4030_REG_HFL_CTL, 0,
			ARRAY_SIZE(twl4030_handsfreel_texts),
			twl4030_handsfreel_texts);

static const struct snd_kcontrol_new twl4030_dapm_handsfreel_control =
    SOC_DAPM_ENUM("Route", twl4030_handsfreel_enum);

/* Handsfree Left virtual mute */
static const struct snd_kcontrol_new twl4030_dapm_handsfreelmute_control =
	SOC_DAPM_SINGLE("Switch", TWL4030_REG_SW_SHADOW, 0, 1, 0);

/* Handsfree Right */
static const char *twl4030_handsfreer_texts[] =
{"Voice", "AudioR1", "AudioR2", "AudioL2"};

static const struct soc_enum twl4030_handsfreer_enum =
	SOC_ENUM_SINGLE(TWL4030_REG_HFR_CTL, 0,
			ARRAY_SIZE(twl4030_handsfreer_texts),
			twl4030_handsfreer_texts);

static const struct snd_kcontrol_new twl4030_dapm_handsfreer_control =
    SOC_DAPM_ENUM("Route", twl4030_handsfreer_enum);

/* Handsfree Right virtual mute */
static const struct snd_kcontrol_new twl4030_dapm_handsfreermute_control =
	SOC_DAPM_SINGLE("Switch", TWL4030_REG_SW_SHADOW, 1, 1, 0);

/* Vibra */
/* Vibra audio path selection */
static const char *twl4030_vibra_texts[] =
{"AudioL1", "AudioR1", "AudioL2", "AudioR2"};

static const struct soc_enum twl4030_vibra_enum =
	SOC_ENUM_SINGLE(TWL4030_REG_VIBRA_CTL, 2,
			ARRAY_SIZE(twl4030_vibra_texts),
			twl4030_vibra_texts);

static const struct snd_kcontrol_new twl4030_dapm_vibra_control =
    SOC_DAPM_ENUM("Route", twl4030_vibra_enum);

/* Vibra path selection: local vibrator (PWM) or audio driven */
static const char *twl4030_vibrapath_texts[] =
		{"Local vibrator", "Audio"};

static const struct soc_enum twl4030_vibrapath_enum =
	SOC_ENUM_SINGLE(TWL4030_REG_VIBRA_CTL, 4,
			ARRAY_SIZE(twl4030_vibrapath_texts),
			twl4030_vibrapath_texts);

static const struct snd_kcontrol_new twl4030_dapm_vibrapath_control =
    SOC_DAPM_ENUM("Route", twl4030_vibrapath_enum);

/* Left analog microphone selection */
static const struct snd_kcontrol_new twl4030_dapm_analoglmic_controls[] = {
    SOC_DAPM_SINGLE("Main mic", TWL4030_REG_ANAMICL, 0, 1, 0),
    SOC_DAPM_SINGLE("Headset mic", TWL4030_REG_ANAMICL, 1, 1, 0),
    SOC_DAPM_SINGLE("AUXL", TWL4030_REG_ANAMICL, 2, 1, 0),
    SOC_DAPM_SINGLE("Carkit mic", TWL4030_REG_ANAMICL, 3, 1, 0),
};

/* Right analog microphone selection */
static const struct snd_kcontrol_new twl4030_dapm_analogrmic_controls[] = {
    SOC_DAPM_SINGLE("Sub mic", TWL4030_REG_ANAMICR, 0, 1, 0),
    SOC_DAPM_SINGLE("AUXR", TWL4030_REG_ANAMICR, 2, 1, 0),
};

/* TX1 L/R Analog/Digital microphone selection */
static const char *twl4030_micpathtx1_texts[] =
{"Analog", "Digimic0"};

static const struct soc_enum twl4030_micpathtx1_enum =
	SOC_ENUM_SINGLE(TWL4030_REG_ADCMICSEL, 0,
			ARRAY_SIZE(twl4030_micpathtx1_texts),
			twl4030_micpathtx1_texts);

static const struct snd_kcontrol_new twl4030_dapm_micpathtx1_control =
    SOC_DAPM_ENUM("Route", twl4030_micpathtx1_enum);

/* TX2 L/R Analog/Digital microphone selection */
static const char *twl4030_micpathtx2_texts[] =
{"Analog", "Digimic1"};

static const struct soc_enum twl4030_micpathtx2_enum =
	SOC_ENUM_SINGLE(TWL4030_REG_ADCMICSEL, 2,
			ARRAY_SIZE(twl4030_micpathtx2_texts),
			twl4030_micpathtx2_texts);

static const struct snd_kcontrol_new twl4030_dapm_micpathtx2_control =
    SOC_DAPM_ENUM("Route", twl4030_micpathtx2_enum);

/* Analog bypass for AudioR1 */
static const struct snd_kcontrol_new twl4030_dapm_abypassr1_control =
    SOC_DAPM_SINGLE("Switch", TWL4030_REG_ARXR1_APGA_CTL, 2, 1, 0);

/* Analog bypass for AudioL1 */
static const struct snd_kcontrol_new twl4030_dapm_abypassl1_control =
    SOC_DAPM_SINGLE("Switch", TWL4030_REG_ARXL1_APGA_CTL, 2, 1, 0);

//&twl4030_dapm_hspga_control,
//programmable gain/attenuation (volume)
static const struct snd_kcontrol_new twl4030_dapm_headsetlpga_control =
    SOC_DAPM_SINGLE("Ramp", TWL4030_REG_HS_POPN_SET, 2, 1, 0);

//programmable gain/attenuation (volume)
static const struct snd_kcontrol_new twl4030_dapm_headsetrpga_control =
    SOC_DAPM_SINGLE("Ramp", TWL4030_REG_HS_POPN_SET, 2, 1, 0);

/* Analog bypass for AudioR2 */
static const struct snd_kcontrol_new twl4030_dapm_abypassr2_control =
	SOC_DAPM_SINGLE("Switch", TWL4030_REG_ARXR2_APGA_CTL, 2, 1, 0);

/* Analog bypass for AudioL2 */
static const struct snd_kcontrol_new twl4030_dapm_abypassl2_control =
	SOC_DAPM_SINGLE("Switch", TWL4030_REG_ARXL2_APGA_CTL, 2, 1, 0);

/* Analog bypass for Voice */
static const struct snd_kcontrol_new twl4030_dapm_abypassv_control =
	SOC_DAPM_SINGLE("Switch", TWL4030_REG_VDL_APGA_CTL, 2, 1, 0);

/* Digital bypass gain, 0 mutes the bypass */
static const unsigned int twl4030_dapm_dbypass_tlv[] = {
	TLV_DB_RANGE_HEAD(2),
	0, 3, TLV_DB_SCALE_ITEM(-2400, 0, 1),
	4, 7, TLV_DB_SCALE_ITEM(-1800, 600, 0),
};

/* Digital bypass left (TX1L -> RX2L) */
static const struct snd_kcontrol_new twl4030_dapm_dbypassl_control =
	SOC_DAPM_SINGLE_TLV("Volume",
			TWL4030_REG_ATX2ARXPGA, 3, 7, 0,
			twl4030_dapm_dbypass_tlv);

/* Digital bypass right (TX1R -> RX2R) */
static const struct snd_kcontrol_new twl4030_dapm_dbypassr_control =
	SOC_DAPM_SINGLE_TLV("Volume",
			TWL4030_REG_ATX2ARXPGA, 0, 7, 0,
			twl4030_dapm_dbypass_tlv);

/*
 * Voice Sidetone GAIN volume control:
 * from -51 to -10 dB in 1 dB steps (mute instead of -51 dB)
 */
static DECLARE_TLV_DB_SCALE(twl4030_dapm_dbypassv_tlv, -5100, 100, 1);

/* Digital bypass voice: sidetone (VUL -> VDL)*/
static const struct snd_kcontrol_new twl4030_dapm_dbypassv_control =
	SOC_DAPM_SINGLE_TLV("Volume",
			TWL4030_REG_VSTPGA, 0, 0x29, 0,
			twl4030_dapm_dbypassv_tlv);

static int micpath_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	struct soc_enum *e = (struct soc_enum *)w->kcontrols->private_value;
	unsigned char adcmicsel, micbias_ctl;

    P("event : %d", event);
   
	adcmicsel = twl4030_read_reg_cache(w->codec, TWL4030_REG_ADCMICSEL);
	micbias_ctl = twl4030_read_reg_cache(w->codec, TWL4030_REG_MICBIAS_CTL);
	/* Prepare the bits for the given TX path:
	 * shift_l == 0: TX1 microphone path
	 * shift_l == 2: TX2 microphone path */

	if (e->shift_l) 
	{
		/* TX2 microphone path */
		if (adcmicsel & TWL4030_TX2IN_SEL)
			micbias_ctl |= TWL4030_MICBIAS2_CTL; /* digimic */
		else
			micbias_ctl &= ~TWL4030_MICBIAS2_CTL;
	} 
	else
	{
		/* TX1 microphone path */
		if (adcmicsel & TWL4030_TX1IN_SEL)
			micbias_ctl |= TWL4030_MICBIAS1_CTL; /* digimic */
		else
			micbias_ctl &= ~TWL4030_MICBIAS1_CTL;
	}

    P(" twl4030 : micpath_event micbias_ctl = %d \n",micbias_ctl);

	twl4030_write(w->codec, TWL4030_REG_MICBIAS_CTL, micbias_ctl);

	return 0;
}

/*
 * Output PGA builder:
 * Handle the muting and unmuting of the given output (turning off the
 * amplifier associated with the output pin)
 * On mute bypass the reg_cache and mute the volume
 * On unmute: restore the register content
 * Outputs handled in this way:  Earpiece, PreDrivL/R, CarkitL/R
 */
#define TWL4030_OUTPUT_PGA(pin_name, reg, mask)				\
static int pin_name##pga_event(struct snd_soc_dapm_widget *w,		\
		struct snd_kcontrol *kcontrol, int event)		\
{									\
	u8 reg_val;							\
									\
	switch (event) {						\
	case SND_SOC_DAPM_POST_PMU:					\
		twl4030_write(w->codec, reg,				\
			twl4030_read_reg_cache(w->codec, reg));		\
		break;							\
	case SND_SOC_DAPM_POST_PMD:					\
		reg_val = twl4030_read_reg_cache(w->codec, reg);	\
		twl_i2c_write_u8(TWL4030_MODULE_AUDIO_VOICE,	\
					reg_val & (~mask),		\
					reg);				\
		break;							\
	}								\
	return 0;							\
}

TWL4030_OUTPUT_PGA(earpiece, TWL4030_REG_EAR_CTL, TWL4030_EAR_GAIN);
TWL4030_OUTPUT_PGA(predrivel, TWL4030_REG_PREDL_CTL, TWL4030_PREDL_GAIN);
TWL4030_OUTPUT_PGA(predriver, TWL4030_REG_PREDR_CTL, TWL4030_PREDR_GAIN);
TWL4030_OUTPUT_PGA(carkitl, TWL4030_REG_PRECKL_CTL, TWL4030_PRECKL_GAIN);
TWL4030_OUTPUT_PGA(carkitr, TWL4030_REG_PRECKR_CTL, TWL4030_PRECKR_GAIN);

static void handsfree_ramp(struct snd_soc_codec *codec, int reg, int ramp)
{
	unsigned char hs_ctl;

    P("reg : ox%x, ramp : %d", reg, ramp);
   
	hs_ctl = twl4030_read_reg_cache(codec, reg);

    P(" twl4030 : handsfree_ramp  \n");
   
	if (ramp) 
	{
		/* HF ramp-up */
		hs_ctl |= TWL4030_HF_CTL_REF_EN;
		twl4030_write(codec, reg, hs_ctl);
		udelay(10);
		hs_ctl |= TWL4030_HF_CTL_RAMP_EN;
		twl4030_write(codec, reg, hs_ctl);
		udelay(40);
		hs_ctl |= TWL4030_HF_CTL_LOOP_EN;
		hs_ctl |= TWL4030_HF_CTL_HB_EN;
		twl4030_write(codec, reg, hs_ctl);
	} 
	else
	{
		/* HF ramp-down */
		hs_ctl &= ~TWL4030_HF_CTL_LOOP_EN;
		hs_ctl &= ~TWL4030_HF_CTL_HB_EN;
		twl4030_write(codec, reg, hs_ctl);
		hs_ctl &= ~TWL4030_HF_CTL_RAMP_EN;
		twl4030_write(codec, reg, hs_ctl);
		udelay(40);
		hs_ctl &= ~TWL4030_HF_CTL_REF_EN;
		twl4030_write(codec, reg, hs_ctl);
	}
}

static int handsfreelpga_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
    P("event %d", event);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		handsfree_ramp(w->codec, TWL4030_REG_HFL_CTL, 1);
		break;
	case SND_SOC_DAPM_POST_PMD:
		handsfree_ramp(w->codec, TWL4030_REG_HFL_CTL, 0);
		break;
	}
	return 0;
}

static int handsfreerpga_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
    P("event : %d", event);
     
	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		handsfree_ramp(w->codec, TWL4030_REG_HFR_CTL, 1);
		break;
	case SND_SOC_DAPM_POST_PMD:
		handsfree_ramp(w->codec, TWL4030_REG_HFR_CTL, 0);
		break;
	}
	return 0;
}

static void headset_ramp(struct snd_soc_codec *codec, int ramp)
{
	struct snd_soc_device *socdev = codec->socdev;
	struct twl4030_setup_data *setup = socdev->codec_data;

	unsigned char hs_gain, hs_pop;
	struct twl4030_priv *twl4030 = codec->private_data;
	/* Base values for ramp delay calculation: 2^19 - 2^26 */
	unsigned int ramp_base[] = {524288, 1048576, 2097152, 4194304,
				    8388608, 16777216, 33554432, 67108864};

    P("ramp : %d", ramp);
   
	hs_gain = twl4030_read_reg_cache(codec, TWL4030_REG_HS_GAIN_SET);
	hs_pop = twl4030_read_reg_cache(codec, TWL4030_REG_HS_POPN_SET);

	/* Enable external mute control, this dramatically reduces
	 * the pop-noise */
	if (setup && setup->hs_extmute) {
		if (setup->set_hs_extmute) {
			setup->set_hs_extmute(1);
		} else {
			hs_pop |= TWL4030_EXTMUTE;
			twl4030_write(codec, TWL4030_REG_HS_POPN_SET, hs_pop);
		}
	}

	if (ramp) {
		/* Headset ramp-up according to the TRM */
		hs_pop |= TWL4030_VMID_EN;
		twl4030_write(codec, TWL4030_REG_HS_POPN_SET, hs_pop);
		twl4030_write(codec, TWL4030_REG_HS_GAIN_SET, hs_gain);
		hs_pop |= TWL4030_RAMP_EN;
		twl4030_write(codec, TWL4030_REG_HS_POPN_SET, hs_pop);
		/* Wait ramp delay time + 1, so the VMID can settle */
		mdelay((ramp_base[(hs_pop & TWL4030_RAMP_DELAY) >> 2] /
			twl4030->sysclk) + 1);
	} else {
		/* Headset ramp-down _not_ according to
		 * the TRM, but in a way that it is working */
		hs_pop &= ~TWL4030_RAMP_EN;
		twl4030_write(codec, TWL4030_REG_HS_POPN_SET, hs_pop);
		/* Wait ramp delay time + 1, so the VMID can settle */
		mdelay((ramp_base[(hs_pop & TWL4030_RAMP_DELAY) >> 2] /
			twl4030->sysclk) + 1);
		/* Bypass the reg_cache to mute the headset */
		twl_i2c_write_u8(TWL4030_MODULE_AUDIO_VOICE,
					hs_gain & (~0x0f),
					TWL4030_REG_HS_GAIN_SET);

		hs_pop &= ~TWL4030_VMID_EN;
		twl4030_write(codec, TWL4030_REG_HS_POPN_SET, hs_pop);
	}

	/* Disable external mute */
	if (setup && setup->hs_extmute) {
		if (setup->set_hs_extmute) {
			setup->set_hs_extmute(0);
		} else {
			hs_pop &= ~TWL4030_EXTMUTE;
			twl4030_write(codec, TWL4030_REG_HS_POPN_SET, hs_pop);
		}
	}
}

static int headsetlpga_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct twl4030_priv *twl4030 = w->codec->private_data;
    P("event : %d", event);
    P(" twl4030 : headsetlpga_event \n");

    event=SND_SOC_DAPM_POST_PMU;

    switch (event) 
    {
	case SND_SOC_DAPM_POST_PMU:
		/* Do the ramp-up only once */
		if (!twl4030->hsr_enabled)
			headset_ramp(w->codec, 1);
		twl4030->hsl_enabled = 1;
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* Do the ramp-down only if both headsetL/R is disabled */
		if (!twl4030->hsr_enabled)
			headset_ramp(w->codec, 0);
		twl4030->hsl_enabled = 0;
		break;
	}
	return 0;
}

static int headsetrpga_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct twl4030_priv *twl4030 = w->codec->private_data;
    P("event : %d", event);
    P(" twl4030 : headsetrpga_event \n");
    event= SND_SOC_DAPM_POST_PMU;

    switch (event) 
    {
	case SND_SOC_DAPM_POST_PMU:
		/* Do the ramp-up only once */
		if (!twl4030->hsl_enabled)
			headset_ramp(w->codec, 1);
		twl4030->hsr_enabled = 1;
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* Do the ramp-down only if both headsetL/R is disabled */
		if (!twl4030->hsl_enabled)
			headset_ramp(w->codec, 0);
		twl4030->hsr_enabled = 0;
		break;
	}
	return 0;
}

static int bypass_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct soc_mixer_control *m =
		(struct soc_mixer_control *)w->kcontrols->private_value;
	struct twl4030_priv *twl4030 = w->codec->private_data;
	unsigned char reg, misc;

    P("event : %d", event);

	reg = twl4030_read_reg_cache(w->codec, m->reg);

	/*
	 * bypass_state[0:3] - analog HiFi bypass
	 * bypass_state[4]   - analog voice bypass
	 * bypass_state[5]   - digital voice bypass
	 * bypass_state[6:7] - digital HiFi bypass
	 */
	if (m->reg == TWL4030_REG_VSTPGA) {
		/* Voice digital bypass */
		if (reg)
			twl4030->bypass_state |= (1 << 5);
		else
			twl4030->bypass_state &= ~(1 << 5);
	} else if (m->reg <= TWL4030_REG_ARXR2_APGA_CTL) {
		/* Analog bypass */
		if (reg & (1 << m->shift))
			twl4030->bypass_state |=
				(1 << (m->reg - TWL4030_REG_ARXL1_APGA_CTL));
		else
			twl4030->bypass_state &=
				~(1 << (m->reg - TWL4030_REG_ARXL1_APGA_CTL));
	} else if (m->reg == TWL4030_REG_VDL_APGA_CTL) {
		/* Analog voice bypass */
		if (reg & (1 << m->shift))
			twl4030->bypass_state |= (1 << 4);
		else
			twl4030->bypass_state &= ~(1 << 4);
	} else {
		/* Digital bypass */
		if (reg & (0x7 << m->shift))
			twl4030->bypass_state |= (1 << (m->shift ? 7 : 6));
		else
			twl4030->bypass_state &= ~(1 << (m->shift ? 7 : 6));
	}

	/* Enable master analog loopback mode if any analog switch is enabled*/
	misc = twl4030_read_reg_cache(w->codec, TWL4030_REG_MISC_SET_1);
	if (twl4030->bypass_state & 0x1F)
		misc |= TWL4030_FMLOOP_EN;
	else
		misc &= ~TWL4030_FMLOOP_EN;
	twl4030_write(w->codec, TWL4030_REG_MISC_SET_1, misc);

	if (w->codec->bias_level == SND_SOC_BIAS_STANDBY) {
		if (twl4030->bypass_state)
			twl4030_codec_mute(w->codec, 0);
		else
			twl4030_codec_mute(w->codec, 1);
	}
	return 0;
}

/*
 * Some of the gain controls in TWL (mostly those which are associated with
 * the outputs) are implemented in an interesting way:
 * 0x0 : Power down (mute)
 * 0x1 : 6dB
 * 0x2 : 0 dB
 * 0x3 : -6 dB
 * Inverting not going to help with these.
 * Custom volsw and volsw_2r get/put functions to handle these gain bits.
 */
#define SOC_DOUBLE_TLV_TWL4030(xname, xreg, shift_left, shift_right, xmax,\
			       xinvert, tlv_array) \
{\
  .iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname),\
	.access = SNDRV_CTL_ELEM_ACCESS_TLV_READ |\
		 SNDRV_CTL_ELEM_ACCESS_READWRITE,\
	.tlv.p = (tlv_array), \
	.info = snd_soc_info_volsw, \
	.get = snd_soc_get_volsw_twl4030, \
	.put = snd_soc_put_volsw_twl4030, \
	.private_value = (unsigned long)&(struct soc_mixer_control) \
		{.reg = xreg, .shift = shift_left, .rshift = shift_right,\
   .max = xmax, .invert = xinvert}\
}
#define SOC_DOUBLE_R_TLV_TWL4030(xname, reg_left, reg_right, xshift, xmax,\
				 xinvert, tlv_array) \
{\
  .iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname),\
	.access = SNDRV_CTL_ELEM_ACCESS_TLV_READ |\
		 SNDRV_CTL_ELEM_ACCESS_READWRITE,\
	.tlv.p = (tlv_array), \
	.info = snd_soc_info_volsw_2r, \
	.get = snd_soc_get_volsw_r2_twl4030,\
	.put = snd_soc_put_volsw_r2_twl4030, \
	.private_value = (unsigned long)&(struct soc_mixer_control) \
		{.reg = reg_left, .rreg = reg_right, .shift = xshift, \
   .rshift = xshift, .max = xmax, .invert = xinvert}\
}
#define SOC_SINGLE_TLV_TWL4030(xname, xreg, xshift, xmax, xinvert, tlv_array) \
	SOC_DOUBLE_TLV_TWL4030(xname, xreg, xshift, xshift, xmax, \
			       xinvert, tlv_array)

static int snd_soc_get_volsw_twl4030(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	unsigned int reg = mc->reg;
	unsigned int shift = mc->shift;
	unsigned int rshift = mc->rshift;
	int max = mc->max;
	int mask = (1 << fls(max)) - 1;

	ucontrol->value.integer.value[0] =
		(snd_soc_read(codec, reg) >> shift) & mask;
	if (ucontrol->value.integer.value[0])
		ucontrol->value.integer.value[0] =
			max + 1 - ucontrol->value.integer.value[0];

	if (shift != rshift) {
		ucontrol->value.integer.value[1] =
			(snd_soc_read(codec, reg) >> rshift) & mask;
		if (ucontrol->value.integer.value[1])
			ucontrol->value.integer.value[1] =
				max + 1 - ucontrol->value.integer.value[1];
	}

	return 0;
}

static int snd_soc_put_volsw_twl4030(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	unsigned int reg = mc->reg;
	unsigned int shift = mc->shift;
	unsigned int rshift = mc->rshift;
	int max = mc->max;
	int mask = (1 << fls(max)) - 1;
	unsigned short val, val2, val_mask;

	val = (ucontrol->value.integer.value[0] & mask);

	val_mask = mask << shift;
	if (val)
		val = max + 1 - val;
	val = val << shift;
	if (shift != rshift) {
		val2 = (ucontrol->value.integer.value[1] & mask);
		val_mask |= mask << rshift;
		if (val2)
			val2 = max + 1 - val2;
		val |= val2 << rshift;
	}
	return snd_soc_update_bits(codec, reg, val_mask, val);
}

static int snd_soc_get_volsw_r2_twl4030(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	unsigned int reg = mc->reg;
	unsigned int reg2 = mc->rreg;
	unsigned int shift = mc->shift;
	int max = mc->max;
	int mask = (1<<fls(max))-1;

	ucontrol->value.integer.value[0] =
		(snd_soc_read(codec, reg) >> shift) & mask;
	ucontrol->value.integer.value[1] =
		(snd_soc_read(codec, reg2) >> shift) & mask;

	if (ucontrol->value.integer.value[0])
		ucontrol->value.integer.value[0] =
			max + 1 - ucontrol->value.integer.value[0];
	if (ucontrol->value.integer.value[1])
		ucontrol->value.integer.value[1] =
			max + 1 - ucontrol->value.integer.value[1];

	return 0;
}

static int snd_soc_put_volsw_r2_twl4030(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	unsigned int reg = mc->reg;
	unsigned int reg2 = mc->rreg;
	unsigned int shift = mc->shift;
	int max = mc->max;
	int mask = (1 << fls(max)) - 1;
	int err;
	unsigned short val, val2, val_mask;

	val_mask = mask << shift;
	val = (ucontrol->value.integer.value[0] & mask);
	val2 = (ucontrol->value.integer.value[1] & mask);

	if (val)
		val = max + 1 - val;
	if (val2)
		val2 = max + 1 - val2;

	val = val << shift;
	val2 = val2 << shift;

	err = snd_soc_update_bits(codec, reg, val_mask, val);
	if (err < 0)
		return err;

	err = snd_soc_update_bits(codec, reg2, val_mask, val2);
	return err;
}

/* Codec operation modes */
static const char *twl4030_op_modes_texts[] = {
	"Option 2 (voice/audio)", "Option 1 (audio)"
};

static const struct soc_enum twl4030_op_modes_enum =
	SOC_ENUM_SINGLE(TWL4030_REG_CODEC_MODE, 0,
			ARRAY_SIZE(twl4030_op_modes_texts),
			twl4030_op_modes_texts);

int snd_soc_put_twl4030_opmode_enum_double(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct twl4030_priv *twl4030 = codec->private_data;
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	unsigned short val;
	unsigned short mask, bitmask;

  if (!list_empty(&twl4030->config_list)) {
		P(KERN_ERR "twl4030 operation mode cannot be "
                "changed on-the-fly\n");
		return -EBUSY;
	}

	for (bitmask = 1; bitmask < e->max; bitmask <<= 1)
		;
	if (ucontrol->value.enumerated.item[0] > e->max - 1)
		return -EINVAL;

	val = ucontrol->value.enumerated.item[0] << e->shift_l;
	mask = (bitmask - 1) << e->shift_l;
	if (e->shift_l != e->shift_r) {
		if (ucontrol->value.enumerated.item[1] > e->max - 1)
			return -EINVAL;
		val |= ucontrol->value.enumerated.item[1] << e->shift_r;
		mask |= (bitmask - 1) << e->shift_r;
	}

	return snd_soc_update_bits(codec, e->reg, mask, val);
}

/*
 * FGAIN volume control:
 * from -62 to 0 dB in 1 dB steps (mute instead of -63 dB)
 */
static DECLARE_TLV_DB_SCALE(digital_fine_tlv, -6300, 100, 1);

/*
 * CGAIN volume control:
 * 0 dB to 12 dB in 6 dB steps
 * value 2 and 3 means 12 dB
 */
static DECLARE_TLV_DB_SCALE(digital_coarse_tlv, 0, 600, 0);

/*
 * Voice Downlink GAIN volume control:
 * from -37 to 12 dB in 1 dB steps (mute instead of -37 dB)
 */
static DECLARE_TLV_DB_SCALE(digital_voice_downlink_tlv, -3700, 100, 1);

/*
 * Analog playback gain
 * -24 dB to 12 dB in 2 dB steps
 */
static DECLARE_TLV_DB_SCALE(analog_tlv, -2400, 200, 0);

/*
 * Gain controls tied to outputs
 * -6 dB to 6 dB in 6 dB steps (mute instead of -12)
 */
static DECLARE_TLV_DB_SCALE(output_tvl, -1200, 600, 1);

/*
 * Gain control for earpiece amplifier
 * 0 dB to 12 dB in 6 dB steps (mute instead of -6)
 */
static DECLARE_TLV_DB_SCALE(output_ear_tvl, -600, 600, 1);

/*
 * Capture gain after the ADCs
 * from 0 dB to 31 dB in 1 dB steps
 */
static DECLARE_TLV_DB_SCALE(digital_capture_tlv, 0, 100, 0);

/*
 * Gain control for input amplifiers
 * 0 dB to 30 dB in 6 dB steps
 */
static DECLARE_TLV_DB_SCALE(input_gain_tlv, 0, 600, 0);

/* AVADC clock priority */
static const char *twl4030_avadc_clk_priority_texts[] = {
	"Voice high priority", "HiFi high priority"
};

static const struct soc_enum twl4030_avadc_clk_priority_enum =
	SOC_ENUM_SINGLE(TWL4030_REG_AVADC_CTL, 2,
			ARRAY_SIZE(twl4030_avadc_clk_priority_texts),
			twl4030_avadc_clk_priority_texts);

static const char *twl4030_rampdelay_texts[] = {
	"27/20/14 ms", "55/40/27 ms", "109/81/55 ms", "218/161/109 ms",
	"437/323/218 ms", "874/645/437 ms", "1748/1291/874 ms",
	"3495/2581/1748 ms"
};

static const struct soc_enum twl4030_rampdelay_enum =
	SOC_ENUM_SINGLE(TWL4030_REG_HS_POPN_SET, 2,
			ARRAY_SIZE(twl4030_rampdelay_texts),
			twl4030_rampdelay_texts);

/* Vibra H-bridge direction mode */
static const char *twl4030_vibradirmode_texts[] = {
	"Vibra H-bridge direction", "Audio data MSB",
};

static const struct soc_enum twl4030_vibradirmode_enum =
	SOC_ENUM_SINGLE(TWL4030_REG_VIBRA_CTL, 5,
			ARRAY_SIZE(twl4030_vibradirmode_texts),
			twl4030_vibradirmode_texts);

/* Vibra H-bridge direction */
static const char *twl4030_vibradir_texts[] = {
	"Positive polarity", "Negative polarity",
};

static const struct soc_enum twl4030_vibradir_enum =
	SOC_ENUM_SINGLE(TWL4030_REG_VIBRA_CTL, 1,
			ARRAY_SIZE(twl4030_vibradir_texts),
			twl4030_vibradir_texts);


static int twl4030_set_playback_path(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol,unsigned int codec_mode)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	unsigned int i=0;

	printk("twl4030_set_playback_path value = %ld device= %d mode = %d codec_mode = %d \n",ucontrol->value.integer.value[0], twl4030_playback_device, twl4030_mode, codec_mode);  

	twl4030_playback_device = ucontrol->value.integer.value[0];
	twl4030_call_device = 0;	
	twl4030_fm_device = 0;

	twl4030_unset_remap();

	switch(ucontrol->value.integer.value[0])
	{
		case OFF: 
			for(i=0;i<ARRAY_SIZE(playback_off);i++)
			{
				twl4030_write(codec, playback_off[i].reg,playback_off[i].value);                        
			}

			break;

		case RCV:
			for(i=0;i<ARRAY_SIZE(playback_rcv);i++)
			{
				twl4030_write(codec, playback_rcv[i].reg,playback_rcv[i].value);                        
			}

			break;         

		case SPK:
			for(i=0;i<ARRAY_SIZE(playback_spk);i++)
			{
				twl4030_write(codec, playback_spk[i].reg,playback_spk[i].value);                        
			}

			break;            

		case HP3P:
		case HP4P:          
			for(i=0;i<ARRAY_SIZE(playback_hp);i++)
			{
				twl4030_write(codec, playback_hp[i].reg,playback_hp[i].value);                        
			}

			break;                                 

		case SPK_HP:
			for(i=0;i<ARRAY_SIZE(playback_spk_hp);i++)
			{
				twl4030_write(codec, playback_spk_hp[i].reg,playback_spk_hp[i].value);                        
			}
			break;                    

		default:
			P("!!!!playback path setting failed!!!\n");
			break;            
	}

	return 0;
}


static int twl4030_set_voicecall_path(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol,unsigned int codec_mode)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	unsigned int i=0;

	printk("twl4030_set_voicecall_path1 value = %d call_device = %d \n", (int)ucontrol->value.integer.value[0], twl4030_call_device);  

	if(twl4030_call_device == ucontrol->value.integer.value[0]) //DB24 sec_lilkan
	{
		P(" same device d=%d  \n",twl4030_call_device);
		return 0;
	}

	twl4030_call_device = ucontrol->value.integer.value[0];
	twl4030_playback_device = 0;		


	switch(ucontrol->value.integer.value[0])
	{
		case OFF: 
			for(i=0;i<ARRAY_SIZE(voicecall_off);i++)
			{
				twl4030_write(codec, voicecall_off[i].reg,voicecall_off[i].value);                        
			}

			break;

		case RCV:
			for(i=0;i<ARRAY_SIZE(voicecall_rcv);i++)
			{
				twl4030_write(codec, voicecall_rcv[i].reg,voicecall_rcv[i].value);                        
			}			  

            #if (0)
			gpio_set_value(OMAP_GPIO_MIC_SEL, 0);
            #endif

			//update cache reg
            //twl4030_write_reg_cache(codec, 0x21, 0x35);


			break;         

		case SPK:          
			for(i=0;i<ARRAY_SIZE(voicecall_spk);i++)
			{
				twl4030_write(codec, voicecall_spk[i].reg,voicecall_spk[i].value);                        
			}

            #if (0)
			gpio_set_value(OMAP_GPIO_MIC_SEL, 1);   
            #endif

			//update cache reg
            //twl4030_write_reg_cache(codec, 0x26, 0x28);
            //twl4030_write_reg_cache(codec, 0x25, 0x24);             

			break;            

		case HP3P:               
			for(i=0;i<ARRAY_SIZE(voicecall_hp3p);i++)
			{
				twl4030_write(codec, voicecall_hp3p[i].reg,voicecall_hp3p[i].value);                        
			}

			//update cache reg
            //twl4030_write_reg_cache(codec, 0x09, 0x30);
            //twl4030_write_reg_cache(codec, 0x12, 0x6f);
            //twl4030_write_reg_cache(codec, 0x13, 0x6f);

			break;                                 

		case HP4P:
			for(i=0;i<ARRAY_SIZE(voicecall_hp4p);i++)
			{
				twl4030_write(codec, voicecall_hp4p[i].reg,voicecall_hp4p[i].value);                        
			}

			//update cache reg
            //twl4030_write_reg_cache(codec, 0x09, 0x30);
            //twl4030_write_reg_cache(codec, 0x12, 0x6f);
            //twl4030_write_reg_cache(codec, 0x13, 0x6f);

			break;                                 

		case BT:               
			for(i=0;i<ARRAY_SIZE(voicecall_bt);i++)
			{
				twl4030_write(codec, voicecall_bt[i].reg,voicecall_bt[i].value);                        
			}

			//update cache reg
            //twl4030_write_reg_cache(codec, 0x0d, 0x40);

			break;                    

		default:
			P("!!!!playback path setting failed!!!\n");

			break;
	}

	twl4030_set_remap();

	wake_lock( &T2_wakelock ); 	
	schedule_delayed_work( &codec_control_work, 100 );

	return 0;

}

static int twl4030_set_fmradio_path(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol,unsigned int codec_mode)
{
    struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
    int iOutDevice = ucontrol->value.integer.value[0];

	if((twl4030_fm_device == iOutDevice)) //DB24 sec_lilkan
	{
		printk("twl4030_set_fmradio_path same device d=%d mute = %d \n",twl4030_fm_device, twl4030_fm_radio_mute_enable);
		return 0;
	}

    printk("twl4030_set_fmradio_path value = %ld output_device = %d, mute = %d \n",
		iOutDevice, twl4030_fm_device, twl4030_fm_radio_mute_enable);  
 /*limo: headphone
		{.flag = 0,.reg = ARXR2PGA_COARSE_GAIN,.gain = 1},
		{.flag = 0,.reg = ARXR2PGA_FINE_GAIN,.gain = 55},
		{.flag = 0,.reg = ARXL2PGA_COARSE_GAIN,.gain = 1},
		{.flag = 0,.reg = ARXL2PGA_FINE_GAIN,.gain = 55},
		{.flag = 1,.reg = ARXL2_GAIN,.gain = 11},
		{.flag = 1,.reg = ARXR2_GAIN,.gain = 11},
		{.flag = 1,.reg = PREDL_GAIN,.gain = 2},
		{.flag = 1,.reg = PREDR_GAIN,.gain = 2},
		{.flag = 0,.reg = HSL_GAIN,.gain = 3},
		{.flag = 0,.reg = HSR_GAIN,.gain = 3},
		{.flag = 1,.reg = MICAMPL_GAIN,.gain = 1},
		{.flag = 1,.reg = MICAMPR_GAIN,.gain = 1},
		{.flag = 0,.reg = ATXL1PGA_GAIN,.gain = 9},
		{.flag = 0,.reg = ATXR1PGA_GAIN,.gain = 9},
		{.flag = 0,.reg = VRXPGA_GAIN,.gain = 48},
		{.flag = 0,.reg = VDL_GAIN_SET,.gain = 6},
		{.flag = 0,.reg = VRX2ARXPGA_GAIN,.gain = 0},
		{.flag = 0,.reg = EAR_GAIN,.gain = 2},
		{.flag = 0,.reg = BTTXPGA_GAIN,.gain = 5},
		{.flag = 0,.reg = BTRXPGA_GAIN,.gain = 10},
		{.flag = 0,.reg = ARX2VTXPGA_GAIN,.gain = 36},
*/  
       
    twl4030_fm_device = iOutDevice;
	twl4030_playback_device = 0;
	twl4030_call_device = 0;
	twl4030_recording_device = 0;

	twl4030_write(codec, 0x01,0x93);         
  	twl4030_write(codec, 0x05,0x14); //AUXL input enabled, Left microphone amplifier enabled 
  
	twl4030_write(codec, 0x05,0x14); //AUXL input enabled, Left microphone amplifier enabled 
	twl4030_write(codec, 0x06,0x14); //AUXR input enabled, Right microphone amplifier enabled
   	twl4030_write(codec, 0x07,0x00); //Audio/voice ADC power down
	twl4030_write(codec, 0x08,0x00); //DIGMIC power down
	if(!twl4030_fm_radio_mute_enable)
		twl4030_write(codec, 0x3e,0x20); //Analog FM radio loop enable
	twl4030_write(codec, 0x13,(0<<6)|0x35 /*0x7f*/);  // Audio RXL2 gain: 12dB
	twl4030_write(codec, 0x12,(0<<6)|0x35 /*0x7f*/);  // Audio RXR2 gain: 12dB
	twl4030_write(codec, 0x17, 0x0c); // ADACR2_EN, ADACL2_EN enable
	twl4030_write(codec, 0x21,0x00);  // Earphone amplifier disable
	twl4030_write(codec, 0x3f,0x00);  // PCM/BT mux reset  

	switch(iOutDevice)
	{   
        case 0: //OFF
        break;
        
		case 1://SPK:   //1
            twl4030_write(codec, 0x48, (0x0<<3) | 0x00);    // 0dB, Gain control of the microphone amplifier (use for AUX_IN also)

            twl4030_write(codec, 0x1c, (9<<3)|0x7 /*0x07*/);  // Audio RXR2: aGain ..dB, Digital-to-analog path enable, FM loop path enable
            twl4030_write(codec, 0x1b, (9<<3)|0x7 /*0x07*/);  // Audio RXL2: aGain ..dB, Digital-to-analog path enable, FM loop path enable

			twl4030_write(codec, 0x26, (3<<4)|0x8 /*0x28*/);  // Predriver class D output right, Audio L2, ..dB
			twl4030_write(codec, 0x25, (3<<4)|0x4 /*0x24*/);  // Predriver class D output left, Audio L2, ..dB
		break;
			
		case 2://HP:		//2
            twl4030_write(codec, 0x48, (0x1<<3) | 0x01);    // 6dB, Gain control of the microphone amplifier (use for AUX_IN also)

        	twl4030_write(codec, 0x1c, (0xb<<3)|0x7 /*0x07*/);  // Audio RXR2: aGain ..dB, Digital-to-analog path enable, FM loop path enable
            twl4030_write(codec, 0x1b, (0xb<<3)|0x7 /*0x07*/);  // Audio RXL2: aGain ..dB, Digital-to-analog path enable, FM loop path enable
            
			twl4030_write(codec, 0x22,0x24);   //Headset output L+R: Audio L2
            twl4030_write(codec, 0x23,0x0a);   //Headset gain 0dB
			twl4030_write(codec, 0x24,0x41);   //HS pop-noise: VMID buffer enable, Ramp down 
			twl4030_write(codec, 0x24,0x42);   //HS pop-noise: VMID buffer enable, Ramp up
		break;   
		default:
			printk("!!!!fmradio path setting failed!!!\n");        
	}
    
 
    return 0;
}
//fm recording ?
static int twl4030_set_loopback_path(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
    struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
    unsigned int i=0;   

    printk("twl4030_set_loopback_path value = %ld output_device = %d \n",ucontrol->value.integer.value[0], twl4030_fm_device);  

    twl4030_fm_device = ucontrol->value.integer.value[0];

	switch(ucontrol->value.integer.value[0])
	{
		case RCV: 
		//mic_enable(codec, MAIN_MIC, 1);
		for(i=0;i<ARRAY_SIZE(Loopback_rcv);i++)
		{
			twl4030_write(codec, Loopback_rcv[i].reg, Loopback_rcv[i].value);                        
		}
		break;	  

		case SPK:
		//mic_enable(codec, SUB_MIC, 1);
		for(i=0;i<ARRAY_SIZE(Loopback_spk);i++)
		{
			  twl4030_write(codec, Loopback_spk[i].reg, Loopback_spk[i].value);                        
		}
		
		break;

		case HP3P:
		case HP4P:
		//mic_enable(codec, HP_MIC, 1);
		for(i=0;i<ARRAY_SIZE(Loopback_headset);i++)
		{
			 twl4030_write(codec, Loopback_headset[i].reg, Loopback_headset[i].value);                        
		}
		break;                           
		
		default:
			printk("!!!!loopback path setting failed!!!\n");

		break;
	}
    
    return 0;
}

static int twl4030_set_voicememo_path(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol,unsigned int codec_mode)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	twl4030_call_device = 0;
	twl4030_fm_device = 0;

	twl4030_recording_device =ucontrol->value.integer.value[0] ;
	printk("twl4030_set_voicememo_path value = %ld \n",ucontrol->value.integer.value[0]);  

	twl4030_write(codec, 0x01, 0x93);
	twl4030_write(codec, 0x1b, 0x33);
	twl4030_write(codec, 0x1c, 0x33);	
	twl4030_write(codec, 0x25, 0x28);
	twl4030_write(codec, 0x26, 0x28);
	twl4030_write(codec, 0x44, 0x32);	


	// Tx gain control
	if(!twl4030_vr_mode)
	{
		twl4030_write(codec, 0x0a, 0x0a);
		twl4030_write(codec, 0x0b, 0x0a);
		twl4030_write(codec, 0x48, 0x2d);
	}

	switch(ucontrol->value.integer.value[0])  
	{
		case OFF:
			break;

		case MAIN_MIC:
            #if (0)
			twl4030_write(codec, 0x04, 0x01);
            #else
			twl4030_write(codec, 0x04, 0x02);
            #endif

			twl4030_write(codec, 0x05, 0x11);
			twl4030_write(codec, 0x06, 0x00);
			twl4030_write(codec, 0x07, 0x0a);
			twl4030_write(codec, 0x08, 0x00);


			// Tx gain control
			if(twl4030_vr_mode)
			{
				twl4030_write(codec, 0x0a, 0x02);
				twl4030_write(codec, 0x0b, 0x02);
				twl4030_write(codec, 0x48, 0x24);
			}
               else if(twl4030_voip_device == RCV)
               {
                   twl4030_write(codec, 0x0c, 0x08);
                   twl4030_write(codec, 0x48, 0x1b);
                   twl4030_write(codec, 0x1b, 0x13);
                   twl4030_write(codec, 0x1c, 0x13);	
                   twl4030_write(codec, 0x25, 0x20);
                   twl4030_write(codec, 0x26, 0x20);	
               }
               else if(twl4030_voip_device == SPK)
               {
                   twl4030_write(codec, 0x0c, 0x06);
                   twl4030_write(codec, 0x48, 0x2d);
                   twl4030_write(codec, 0x1b, 0x0b);
                   twl4030_write(codec, 0x1c, 0x0b);	
               }
               else if(twl4030_voip_device == HP3P)
               {
                   twl4030_write(codec, 0x0c, 0x08);
                   twl4030_write(codec, 0x48, 0x24);
                   twl4030_write(codec, 0x1b, 0x13);
                   twl4030_write(codec, 0x1c, 0x13);	
                   twl4030_write(codec, 0x25, 0x00);
                   twl4030_write(codec, 0x26, 0x00);	
               }
               else if(twl4030_voip_device == HP4P)
               {
                   twl4030_write(codec, 0x0c, 0x06);
                   twl4030_write(codec, 0x48, 0x2d);
                   twl4030_write(codec, 0x1b, 0x13);
                   twl4030_write(codec, 0x1c, 0x13);	
                   twl4030_write(codec, 0x25, 0x00);
                   twl4030_write(codec, 0x26, 0x00);	
               }

            #if (0)
			gpio_set_value(OMAP_GPIO_MIC_SEL, 0);
            #endif

			break;

		case HP_MIC:
			twl4030_write(codec, 0x04, 0x04);
			twl4030_write(codec, 0x05, 0x12);
			twl4030_write(codec, 0x07, 0x0a);
			twl4030_write(codec, 0x08, 0x00);

			// Tx gain control
			if(twl4030_vr_mode)
			{
				twl4030_write(codec, 0x0a, 0x0c);
				twl4030_write(codec, 0x0b, 0x0c);
				twl4030_write(codec, 0x48, 0x2d);
			}
               else if(twl4030_voip_device == RCV)
               {
                   twl4030_write(codec, 0x0c, 0x08);
                   twl4030_write(codec, 0x48, 0x1b);
                   twl4030_write(codec, 0x1b, 0x13);
                   twl4030_write(codec, 0x1c, 0x13);	
                   twl4030_write(codec, 0x25, 0x20);
                   twl4030_write(codec, 0x26, 0x20);	
               }
               else if(twl4030_voip_device == SPK)
               {
                   twl4030_write(codec, 0x0c, 0x06);
                   twl4030_write(codec, 0x48, 0x2d);
                   twl4030_write(codec, 0x1b, 0x0b);
                   twl4030_write(codec, 0x1c, 0x0b);	
               }
               else if(twl4030_voip_device == HP3P)
               {
                   twl4030_write(codec, 0x0c, 0x08);
                   twl4030_write(codec, 0x48, 0x24);
                   twl4030_write(codec, 0x1b, 0x13);
                   twl4030_write(codec, 0x1c, 0x13);	
                   twl4030_write(codec, 0x25, 0x00);
                   twl4030_write(codec, 0x26, 0x00);	
               }
               else if(twl4030_voip_device == HP4P)
               {
                   twl4030_write(codec, 0x0c, 0x06);
                   twl4030_write(codec, 0x48, 0x2d);
                   twl4030_write(codec, 0x1b, 0x13);
                   twl4030_write(codec, 0x1c, 0x13);	
                   twl4030_write(codec, 0x25, 0x00);
                   twl4030_write(codec, 0x26, 0x00);	
               }

			break;
	}

	return 0;
}

// hskwon-ss-cl31, added for FMC(VoIP) call path
static int twl4030_set_voipcall_path(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol,unsigned int codec_mode)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	unsigned int i=0;

	printk("twl4030_set_voipcall_path1 value = %d call_device = %d \n",(int)ucontrol->value.integer.value[0], twl4030_voip_device);  

	twl4030_voip_device = ucontrol->value.integer.value[0];
	twl4030_call_device = 0;		


	switch(ucontrol->value.integer.value[0])
	{
		case OFF: 
			for(i=0;i<ARRAY_SIZE(voipcall_off);i++)
			{
				twl4030_write(codec, voipcall_off[i].reg,voipcall_off[i].value);                        
			}

			break;

		case RCV:
			for(i=0;i<ARRAY_SIZE(voipcall_rcv);i++)
			{
				twl4030_write(codec, voipcall_rcv[i].reg,voipcall_rcv[i].value);                        
			}

            #if (0)
			gpio_set_value(OMAP_GPIO_MIC_SEL, 0);
            #endif

			break;         

		case SPK:
			for(i=0;i<ARRAY_SIZE(voipcall_spk);i++)
			{
				twl4030_write(codec, voipcall_spk[i].reg,voipcall_spk[i].value);                        
			}

            #if (0)
			gpio_set_value(OMAP_GPIO_MIC_SEL, 1);   
            #endif

			break;            

		case HP3P:
			for(i=0;i<ARRAY_SIZE(voipcall_hp3p);i++)
			{
				twl4030_write(codec, voipcall_hp3p[i].reg,voipcall_hp3p[i].value);                        
			}

			break;                                 

		case HP4P:
			for(i=0;i<ARRAY_SIZE(voipcall_hp4p);i++)
			{
				twl4030_write(codec, voipcall_hp4p[i].reg,voipcall_hp4p[i].value);                        
			}

			break;                                 

		case BT:
			break;                    

		default:
			P("!!!!playback path setting failed!!!\n");

			break;
	}

	return 0;
}

static int twl4030_set_idle_mode(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol,unsigned int codec_mode)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

	printk("twl4030_set_idle_mode value = %ld \n",ucontrol->value.integer.value[0]);  

	if(ucontrol->value.integer.value[0])  //off
	{
		twl4030_playback_device = 0;
		twl4030_call_device = 0;
		twl4030_voip_device = 0; // hskwon-ss-cl31, added for FMC(VoIP) call path
		twl4030_recording_device = 0;
		twl4030_fm_device = 0;	
		twl4030_vr_mode = false;
		twl4030_power_down(codec);
	}

	twl4030_unset_remap();

	return 0;
}

// hskwon-ss-db05, to support mic mute/unmute for CTS test
static int twl4030_set_mic_mute(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol,unsigned int codec_mode)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

	P("twl4030_set_mic_mute value = %ld \n",ucontrol->value.integer.value[0]);  

	if(ucontrol->value.integer.value[0])    // on, mute
	{
		twl4030_write(codec, 0x04, 0x00);  //TWL4030_REG_MICBIAS_CTL
	}
	else    // off, unmute
	{
        #if (0)
		twl4030_write(codec, 0x04, 0x01);  //TWL4030_REG_MICBIAS_CTL
        #else
		twl4030_write(codec, 0x04, 0x02);  //TWL4030_REG_MICBIAS_CTL
        #endif
	}

	return 0;
}


static int twl4030_set_vr_mode(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol,unsigned int codec_mode)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

	P("twl4030_set_vr_mode value = %ld \n",ucontrol->value.integer.value[0]);  

	if(ucontrol->value.integer.value[0])    // on, mute
	{

		if( twl4030_recording_device == MAIN_MIC)
		{
			twl4030_write(codec, 0x0a, 0x02);
			twl4030_write(codec, 0x0b, 0x02);
			twl4030_write(codec, 0x48, 0x24);
		}
		else   //HP_MIC
		{
			twl4030_write(codec, 0x0a, 0x0c);
			twl4030_write(codec, 0x0b, 0x0c);
			twl4030_write(codec, 0x48, 0x2d);
		}

		twl4030_vr_mode = true;
	}
	else    // off, unmute
	{
		twl4030_vr_mode = false;
	}

	return 0;
}

static int twl4030_get_path(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	P("twl4030_get_path !!!\n");
	return 0;
}
static int twl4030_get_status(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int codec_mode = 0;

	if(twl4030_playback_device)
		codec_mode = PLAY_BACK;
	else if(twl4030_call_device)
		codec_mode = VOICE_CALL;    
	else if(twl4030_recording_device)
		codec_mode = VOICE_MEMO;    	
	else if(twl4030_voip_device)
		codec_mode = VOIP_CALL; 
    else if(twl4030_fm_device)
		 codec_mode = FM_RADIO; 		
	else
		codec_mode = IDLE_MODE;

	P("twl4030_get_status codec_mode = %d !!!\n", codec_mode);
	ucontrol->value.integer.value[0] =codec_mode+1;

	return 0;
}

static int twl4030_set_path(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
//	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

	int i =0;
	
	printk("twl4030_set audio_path= %s name=%s \n",audio_path[0], kcontrol->id.name);

	cancel_delayed_work( &codec_down_work );

	while(audio_path[i] !=NULL) 
	{
		if(!strncmp(audio_path[i], kcontrol->id.name,sizeof(audio_path[i])))
		{ 
			twl4030_old_mode = twl4030_mode;
			twl4030_mode = i;

			switch(i)
			{
				case PLAY_BACK:
					twl4030_set_playback_path(kcontrol, ucontrol,i);
					return 0;

				case VOICE_CALL:
					twl4030_set_voicecall_path(kcontrol, ucontrol,i);
					return 0;

				case VOICE_MEMO:
					twl4030_set_voicememo_path(kcontrol, ucontrol, i);
					return 0;

				case VOIP_CALL:
					twl4030_set_voipcall_path(kcontrol, ucontrol, i);	// hskwon-ss-cl31, added for FMC(VoIP) call path
					return 0;
					
                case FM_RADIO:
				    twl4030_unset_remap();
					twl4030_set_fmradio_path(kcontrol, ucontrol,i);
					twl4030_set_remap();
					return 0;
					
				case IDLE_MODE:
					twl4030_set_idle_mode(kcontrol, ucontrol,i);
					return 0;

                // hskwon-ss-db05, to support mic mute/unmute for CTS test
				case MIC_MUTE:
					twl4030_set_mic_mute(kcontrol, ucontrol, i);
					return 0;

				case VR_MODE:
					twl4030_set_vr_mode(kcontrol, ucontrol, i);
					return 0;

				default:
					return 0;

			}
		}

		i++;
	}

	return 0;
}

static int twl4030_get_fm_radio_mute_enable(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return twl4030_fm_radio_mute_enable;
}
static int twl4030_set_fm_radio_mute_enable(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

	if(twl4030_fm_radio_mute_enable == ucontrol->value.integer.value[0])
	{
		P("twl4030_fm_radio_mute_enable is same\n");
		return 0;
	}
    
    printk("twl4030_set_fm_radio_mute_enable %ld\n", ucontrol->value.integer.value[0]);

	twl4030_fm_radio_mute_enable = ucontrol->value.integer.value[0];
	twl4030_write(codec, 0x3e, twl4030_fm_radio_mute_enable?0x00:0x20);   
    
	return 0;
}


static const struct soc_enum path_control_enum[]=
{
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(playback_path),playback_path),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(voicecall_path),voicecall_path),        
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(voicememo_path),voicememo_path),       
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(voip_path),voip_path),      
    SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(fmradio_path),fmradio_path),	
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(idle_mode),idle_mode),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(mic_mute),mic_mute), // hskwon-ss-db05, to support mic mute/unmute for CTS test
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(vr_mode),vr_mode),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(loopback_path),loopback_path), 
};

static const struct snd_kcontrol_new twl4030_snd_controls[] = {
	/* Codec operation mode control */
	SOC_ENUM_EXT("Codec Operation Mode", twl4030_op_modes_enum,
		snd_soc_get_enum_double,
		snd_soc_put_twl4030_opmode_enum_double),

	/* Common playback gain controls */
	SOC_DOUBLE_R_TLV("DAC1 Digital Fine Playback Volume",
		TWL4030_REG_ARXL1PGA, TWL4030_REG_ARXR1PGA,
		0, 0x3f, 0, digital_fine_tlv),
	SOC_DOUBLE_R_TLV("DAC2 Digital Fine Playback Volume",
		TWL4030_REG_ARXL2PGA, TWL4030_REG_ARXR2PGA,
		0, 0x3f, 0, digital_fine_tlv),

	SOC_DOUBLE_R_TLV("DAC1 Digital Coarse Playback Volume",
		TWL4030_REG_ARXL1PGA, TWL4030_REG_ARXR1PGA,
		6, 0x2, 0, digital_coarse_tlv),
	SOC_DOUBLE_R_TLV("DAC2 Digital Coarse Playback Volume",
		TWL4030_REG_ARXL2PGA, TWL4030_REG_ARXR2PGA,
		6, 0x2, 0, digital_coarse_tlv),

	SOC_DOUBLE_R_TLV("DAC1 Analog Playback Volume",
		TWL4030_REG_ARXL1_APGA_CTL, TWL4030_REG_ARXR1_APGA_CTL,
		3, 0x12, 1, analog_tlv),
	SOC_DOUBLE_R_TLV("DAC2 Analog Playback Volume",
		TWL4030_REG_ARXL2_APGA_CTL, TWL4030_REG_ARXR2_APGA_CTL,
		3, 0x12, 1, analog_tlv),
  
	SOC_DOUBLE_R("DAC1 Analog Playback Switch",
		TWL4030_REG_ARXL1_APGA_CTL, TWL4030_REG_ARXR1_APGA_CTL,
		1, 1, 0),
	SOC_DOUBLE_R("DAC2 Analog Playback Switch",
		TWL4030_REG_ARXL2_APGA_CTL, TWL4030_REG_ARXR2_APGA_CTL,
		1, 1, 0),

	/* Common voice downlink gain controls */
	SOC_SINGLE_TLV("DAC Voice Digital Downlink Volume",
		TWL4030_REG_VRXPGA, 0, 0x31, 0, digital_voice_downlink_tlv),

	SOC_SINGLE_TLV("DAC Voice Analog Downlink Volume",
		TWL4030_REG_VDL_APGA_CTL, 3, 0x12, 1, analog_tlv),

	SOC_SINGLE("DAC Voice Analog Downlink Switch",
		TWL4030_REG_VDL_APGA_CTL, 1, 1, 0),

	/* Separate output gain controls */
	SOC_DOUBLE_R_TLV_TWL4030("PreDriv Playback Volume",
		TWL4030_REG_PREDL_CTL, TWL4030_REG_PREDR_CTL,
		4, 3, 0, output_tvl),

	SOC_DOUBLE_TLV_TWL4030("Headset Playback Volume",
		TWL4030_REG_HS_GAIN_SET, 0, 2, 3, 0, output_tvl),

	SOC_DOUBLE_R_TLV_TWL4030("Carkit Playback Volume",
		TWL4030_REG_PRECKL_CTL, TWL4030_REG_PRECKR_CTL,
		4, 3, 0, output_tvl),

	SOC_SINGLE_TLV_TWL4030("Earpiece Playback Volume",
		TWL4030_REG_EAR_CTL, 4, 3, 0, output_ear_tvl),

	/* Common capture gain controls */
	SOC_DOUBLE_R_TLV("TX1 Digital Capture Volume",
		TWL4030_REG_ATXL1PGA, TWL4030_REG_ATXR1PGA,
		0, 0x1f, 0, digital_capture_tlv),
	SOC_DOUBLE_R_TLV("TX2 Digital Capture Volume",
		TWL4030_REG_AVTXL2PGA, TWL4030_REG_AVTXR2PGA,
		0, 0x1f, 0, digital_capture_tlv),

	SOC_DOUBLE_TLV("Analog Capture Volume", TWL4030_REG_ANAMIC_GAIN,
		0, 3, 5, 0, input_gain_tlv),

	SOC_ENUM("AVADC Clock Priority", twl4030_avadc_clk_priority_enum),

	SOC_ENUM("HS ramp delay", twl4030_rampdelay_enum),

	SOC_ENUM("Vibra H-bridge mode", twl4030_vibradirmode_enum),
	SOC_ENUM("Vibra H-bridge direction", twl4030_vibradir_enum),

	/* normal play setting */
    SOC_ENUM_EXT("Playback Path", path_control_enum[0],
                twl4030_get_path,twl4030_set_path),

	/* voice call setting */
	SOC_ENUM_EXT("Voice Call Path", path_control_enum[1],
			twl4030_get_path,twl4030_set_path),

	/* voice memol setting */
	SOC_ENUM_EXT("Memo Path", path_control_enum[2],
			twl4030_get_path,twl4030_set_path),

	/* VOIP call setting */
	SOC_ENUM_EXT("VOIP Call Path", path_control_enum[3],
			twl4030_get_path,twl4030_set_path),
    /* FM Radio Path setting */ 
	
	SOC_ENUM_EXT("FM Radio Path", path_control_enum[4],
		twl4030_get_path,twl4030_set_path),
		
	/* Idle Mode setting */
	SOC_ENUM_EXT("Idle Mode", path_control_enum[5],
			twl4030_get_status,twl4030_set_path),

	/* Mic Mute setting, hskwon-ss-db05, to support mic mute/unmute for CTS test */
	SOC_ENUM_EXT("Mic Mute", path_control_enum[6],
			twl4030_get_path,twl4030_set_path),

	SOC_ENUM_EXT("VR Mode", path_control_enum[7],
			twl4030_get_path,twl4030_set_path),
	
	SOC_ENUM_EXT("Loopback Path", path_control_enum[8],
        twl4030_get_path, twl4030_set_loopback_path),
			
    SOC_ENUM_EXT("FM Radio Mute Enable", path_control_enum[6],
        twl4030_get_fm_radio_mute_enable, twl4030_set_fm_radio_mute_enable),                 
};
 
/* add non dapm controls */
static int twl4030_add_controls(struct snd_soc_codec *codec)
{
  int err, i;

  for (i = 0; i < ARRAY_SIZE(twl4030_snd_controls); i++) {
    err = snd_ctl_add(codec->card,
    snd_soc_cnew(&twl4030_snd_controls[i],
    codec, NULL));
    if (err < 0)
      return err;
  }

  return 0;
}

static const struct snd_soc_dapm_widget twl4030_dapm_widgets[] = {
	/* Left channel inputs */
	SND_SOC_DAPM_INPUT("MAINMIC"),
	SND_SOC_DAPM_INPUT("HSMIC"),
	SND_SOC_DAPM_INPUT("AUXL"),
	SND_SOC_DAPM_INPUT("CARKITMIC"),
	/* Right channel inputs */
	SND_SOC_DAPM_INPUT("SUBMIC"),
	SND_SOC_DAPM_INPUT("AUXR"),
	/* Digital microphones (Stereo) */
	SND_SOC_DAPM_INPUT("DIGIMIC0"),
	SND_SOC_DAPM_INPUT("DIGIMIC1"),

	/* Outputs */
	SND_SOC_DAPM_OUTPUT("OUTL"),
	SND_SOC_DAPM_OUTPUT("OUTR"),
	SND_SOC_DAPM_OUTPUT("EARPIECE"),
	SND_SOC_DAPM_OUTPUT("PREDRIVEL"),
	SND_SOC_DAPM_OUTPUT("PREDRIVER"),
	SND_SOC_DAPM_OUTPUT("HSOL"),
	SND_SOC_DAPM_OUTPUT("HSOR"),
	SND_SOC_DAPM_OUTPUT("CARKITL"),
	SND_SOC_DAPM_OUTPUT("CARKITR"),
	SND_SOC_DAPM_OUTPUT("HFL"),
	SND_SOC_DAPM_OUTPUT("HFR"),
	SND_SOC_DAPM_OUTPUT("VIBRA"),

	/* DACs */
	SND_SOC_DAPM_DAC("DAC Right1", "Right Front HiFi Playback",
			SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_DAC("DAC Left1", "Left Front HiFi Playback",
			SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_DAC("DAC Right2", "Right Rear HiFi Playback",
			SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_DAC("DAC Left2", "Left Rear HiFi Playback",
			SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_DAC("DAC Voice", "Voice Playback",
			SND_SOC_NOPM, 0, 0),

	/* Analog bypasses */
	SND_SOC_DAPM_SWITCH_E("Right1 Analog Loopback", SND_SOC_NOPM, 0, 0,
			&twl4030_dapm_abypassr1_control, bypass_event,
			SND_SOC_DAPM_POST_REG),
	SND_SOC_DAPM_SWITCH_E("Left1 Analog Loopback", SND_SOC_NOPM, 0, 0,
			&twl4030_dapm_abypassl1_control,
			bypass_event, SND_SOC_DAPM_POST_REG),
	SND_SOC_DAPM_SWITCH_E("Right2 Analog Loopback", SND_SOC_NOPM, 0, 0,
			&twl4030_dapm_abypassr2_control,
			bypass_event, SND_SOC_DAPM_POST_REG),
	SND_SOC_DAPM_SWITCH_E("Left2 Analog Loopback", SND_SOC_NOPM, 0, 0,
			&twl4030_dapm_abypassl2_control,
			bypass_event, SND_SOC_DAPM_POST_REG),
	SND_SOC_DAPM_SWITCH_E("Voice Analog Loopback", SND_SOC_NOPM, 0, 0,
			&twl4030_dapm_abypassv_control,
			bypass_event, SND_SOC_DAPM_POST_REG),

	/* Digital bypasses */
	SND_SOC_DAPM_SWITCH_E("Left Digital Loopback", SND_SOC_NOPM, 0, 0,
			&twl4030_dapm_dbypassl_control, bypass_event,
			SND_SOC_DAPM_POST_REG),
	SND_SOC_DAPM_SWITCH_E("Right Digital Loopback", SND_SOC_NOPM, 0, 0,
			&twl4030_dapm_dbypassr_control, bypass_event,
			SND_SOC_DAPM_POST_REG),
	SND_SOC_DAPM_SWITCH_E("Voice Digital Loopback", SND_SOC_NOPM, 0, 0,
			&twl4030_dapm_dbypassv_control, bypass_event,
			SND_SOC_DAPM_POST_REG),

	/* Digital mixers, power control for the physical DACs */
	SND_SOC_DAPM_MIXER("Digital R1 Playback Mixer",
			TWL4030_REG_AVDAC_CTL, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("Digital L1 Playback Mixer",
			TWL4030_REG_AVDAC_CTL, 1, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("Digital R2 Playback Mixer",
			TWL4030_REG_AVDAC_CTL, 2, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("Digital L2 Playback Mixer",
			TWL4030_REG_AVDAC_CTL, 3, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("Digital Voice Playback Mixer",
			TWL4030_REG_AVDAC_CTL, 4, 0, NULL, 0),

	/* Analog mixers, power control for the physical PGAs */
	SND_SOC_DAPM_MIXER("Analog R1 Playback Mixer",
			TWL4030_REG_ARXR1_APGA_CTL, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("Analog L1 Playback Mixer",
			TWL4030_REG_ARXL1_APGA_CTL, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("Analog R2 Playback Mixer",
			TWL4030_REG_ARXR2_APGA_CTL, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("Analog L2 Playback Mixer",
			TWL4030_REG_ARXL2_APGA_CTL, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("Analog Voice Playback Mixer",
			TWL4030_REG_VDL_APGA_CTL, 0, 0, NULL, 0),

	/* Output MIXER controls */
	/* Earpiece */
	SND_SOC_DAPM_MIXER("Earpiece Mixer", SND_SOC_NOPM, 0, 0,
			&twl4030_dapm_earpiece_controls[0],
			ARRAY_SIZE(twl4030_dapm_earpiece_controls)),
	SND_SOC_DAPM_PGA_E("Earpiece PGA", SND_SOC_NOPM,
			0, 0, NULL, 0, earpiecepga_event,
			SND_SOC_DAPM_POST_PMU|SND_SOC_DAPM_POST_PMD),
	/* PreDrivL/R */
	SND_SOC_DAPM_MIXER("PredriveL Mixer", SND_SOC_NOPM, 0, 0,
			&twl4030_dapm_predrivel_controls[0],
			ARRAY_SIZE(twl4030_dapm_predrivel_controls)),
	SND_SOC_DAPM_PGA_E("PredriveL PGA", SND_SOC_NOPM,
			0, 0, NULL, 0, predrivelpga_event,
			SND_SOC_DAPM_POST_PMU|SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MIXER("PredriveR Mixer", SND_SOC_NOPM, 0, 0,
			&twl4030_dapm_predriver_controls[0],
			ARRAY_SIZE(twl4030_dapm_predriver_controls)),
	SND_SOC_DAPM_PGA_E("PredriveR PGA", SND_SOC_NOPM,
			0, 0, NULL, 0, predriverpga_event,
			SND_SOC_DAPM_POST_PMU|SND_SOC_DAPM_POST_PMD),
	/* HeadsetL/R */
	SND_SOC_DAPM_MIXER("HeadsetL Mixer", SND_SOC_NOPM, 0, 0,
			&twl4030_dapm_hsol_controls[0],
			ARRAY_SIZE(twl4030_dapm_hsol_controls)),
	SND_SOC_DAPM_PGA_E("HeadsetL PGA", SND_SOC_NOPM,
			0, 0, NULL, 0, headsetlpga_event,
			SND_SOC_DAPM_POST_PMU|SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MIXER("HeadsetR Mixer", SND_SOC_NOPM, 0, 0,
			&twl4030_dapm_hsor_controls[0],
			ARRAY_SIZE(twl4030_dapm_hsor_controls)),
	SND_SOC_DAPM_PGA_E("HeadsetR PGA", SND_SOC_NOPM,
			0, 0, NULL, 0, headsetrpga_event,
			SND_SOC_DAPM_POST_PMU|SND_SOC_DAPM_POST_PMD),
	/* CarkitL/R */
	SND_SOC_DAPM_MIXER("CarkitL Mixer", SND_SOC_NOPM, 0, 0,
			&twl4030_dapm_carkitl_controls[0],
			ARRAY_SIZE(twl4030_dapm_carkitl_controls)),
	SND_SOC_DAPM_PGA_E("CarkitL PGA", SND_SOC_NOPM,
			0, 0, NULL, 0, carkitlpga_event,
			SND_SOC_DAPM_POST_PMU|SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MIXER("CarkitR Mixer", SND_SOC_NOPM, 0, 0,
			&twl4030_dapm_carkitr_controls[0],
			ARRAY_SIZE(twl4030_dapm_carkitr_controls)),
	SND_SOC_DAPM_PGA_E("CarkitR PGA", SND_SOC_NOPM,
			0, 0, NULL, 0, carkitrpga_event,
			SND_SOC_DAPM_POST_PMU|SND_SOC_DAPM_POST_PMD),

	/* Output MUX controls */
	/* HandsfreeL/R */
	SND_SOC_DAPM_MUX("HandsfreeL Mux", SND_SOC_NOPM, 0, 0,
		&twl4030_dapm_handsfreel_control),
	SND_SOC_DAPM_SWITCH("HandsfreeL", SND_SOC_NOPM, 0, 0,
			&twl4030_dapm_handsfreelmute_control),
	SND_SOC_DAPM_PGA_E("HandsfreeL PGA", SND_SOC_NOPM,
			0, 0, NULL, 0, handsfreelpga_event,
			SND_SOC_DAPM_POST_PMU|SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MUX("HandsfreeR Mux", SND_SOC_NOPM, 5, 0,
		&twl4030_dapm_handsfreer_control),
	SND_SOC_DAPM_SWITCH("HandsfreeR", SND_SOC_NOPM, 0, 0,
			&twl4030_dapm_handsfreermute_control),
	SND_SOC_DAPM_PGA_E("HandsfreeR PGA", SND_SOC_NOPM,
			0, 0, NULL, 0, handsfreerpga_event,
			SND_SOC_DAPM_POST_PMU|SND_SOC_DAPM_POST_PMD),
	/* Vibra */
	SND_SOC_DAPM_MUX("Vibra Mux", TWL4030_REG_VIBRA_CTL, 0, 0,
		&twl4030_dapm_vibra_control),
	SND_SOC_DAPM_MUX("Vibra Route", SND_SOC_NOPM, 0, 0,
		&twl4030_dapm_vibrapath_control),

	/* Introducing four virtual ADC, since TWL4030 have four channel for
	   capture */
	SND_SOC_DAPM_ADC("ADC Virtual Left1", "Left Front Capture",
		SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_ADC("ADC Virtual Right1", "Right Front Capture",
		SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_ADC("ADC Virtual Left2", "Left Rear Capture",
		SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_ADC("ADC Virtual Right2", "Right Rear Capture",
		SND_SOC_NOPM, 0, 0),

	/* Analog/Digital mic path selection.
	   TX1 Left/Right: either analog Left/Right or Digimic0
	   TX2 Left/Right: either analog Left/Right or Digimic1 */
	SND_SOC_DAPM_MUX_E("TX1 Capture Route", SND_SOC_NOPM, 0, 0,
		&twl4030_dapm_micpathtx1_control, micpath_event,
		SND_SOC_DAPM_POST_PMU|SND_SOC_DAPM_POST_PMD|
		SND_SOC_DAPM_POST_REG),
	SND_SOC_DAPM_MUX_E("TX2 Capture Route", SND_SOC_NOPM, 0, 0,
		&twl4030_dapm_micpathtx2_control, micpath_event,
		SND_SOC_DAPM_POST_PMU|SND_SOC_DAPM_POST_PMD|
		SND_SOC_DAPM_POST_REG),

	/* Analog input mixers for the capture amplifiers */
    SND_SOC_DAPM_MIXER("Analog Left Capture Route",
		TWL4030_REG_ANAMICL, 4, 0,
		&twl4030_dapm_analoglmic_controls[0],
		ARRAY_SIZE(twl4030_dapm_analoglmic_controls)),
    SND_SOC_DAPM_MIXER("Analog Right Capture Route",
		TWL4030_REG_ANAMICR, 4, 0,
		&twl4030_dapm_analogrmic_controls[0],
		ARRAY_SIZE(twl4030_dapm_analogrmic_controls)),

	SND_SOC_DAPM_PGA("ADC Physical Left",
		TWL4030_REG_AVADC_CTL, 3, 0, NULL, 0),
	SND_SOC_DAPM_PGA("ADC Physical Right",
		TWL4030_REG_AVADC_CTL, 1, 0, NULL, 0),

	SND_SOC_DAPM_PGA("Digimic0 Enable",
		TWL4030_REG_ADCMICSEL, 1, 0, NULL, 0),
	SND_SOC_DAPM_PGA("Digimic1 Enable",
		TWL4030_REG_ADCMICSEL, 3, 0, NULL, 0),

	SND_SOC_DAPM_MICBIAS("Mic Bias 1", TWL4030_REG_MICBIAS_CTL, 0, 0),
	SND_SOC_DAPM_MICBIAS("Mic Bias 2", TWL4030_REG_MICBIAS_CTL, 1, 0),
	SND_SOC_DAPM_MICBIAS("Headset Mic Bias", TWL4030_REG_MICBIAS_CTL, 2, 0),

};

static const struct snd_soc_dapm_route intercon[] = {
	{"Digital L1 Playback Mixer", NULL, "DAC Left1"},
	{"Digital R1 Playback Mixer", NULL, "DAC Right1"},
	{"Digital L2 Playback Mixer", NULL, "DAC Left2"},
	{"Digital R2 Playback Mixer", NULL, "DAC Right2"},
	{"Digital Voice Playback Mixer", NULL, "DAC Voice"},

	{"Analog L1 Playback Mixer", NULL, "Digital L1 Playback Mixer"},
	{"Analog R1 Playback Mixer", NULL, "Digital R1 Playback Mixer"},
	{"Analog L2 Playback Mixer", NULL, "Digital L2 Playback Mixer"},
	{"Analog R2 Playback Mixer", NULL, "Digital R2 Playback Mixer"},
	{"Analog Voice Playback Mixer", NULL, "Digital Voice Playback Mixer"},

	/* Internal playback routings */
	/* Earpiece */
	{"Earpiece Mixer", "Voice", "Analog Voice Playback Mixer"},
	{"Earpiece Mixer", "AudioL1", "Analog L1 Playback Mixer"},
	{"Earpiece Mixer", "AudioL2", "Analog L2 Playback Mixer"},
	{"Earpiece Mixer", "AudioR1", "Analog R1 Playback Mixer"},
	{"Earpiece PGA", NULL, "Earpiece Mixer"},
	/* PreDrivL */
	{"PredriveL Mixer", "Voice", "Analog Voice Playback Mixer"},
	{"PredriveL Mixer", "AudioL1", "Analog L1 Playback Mixer"},
	{"PredriveL Mixer", "AudioL2", "Analog L2 Playback Mixer"},
	{"PredriveL Mixer", "AudioR2", "Analog R2 Playback Mixer"},
	{"PredriveL PGA", NULL, "PredriveL Mixer"},
	/* PreDrivR */
	{"PredriveR Mixer", "Voice", "Analog Voice Playback Mixer"},
	{"PredriveR Mixer", "AudioR1", "Analog R1 Playback Mixer"},
	{"PredriveR Mixer", "AudioR2", "Analog R2 Playback Mixer"},
	{"PredriveR Mixer", "AudioL2", "Analog L2 Playback Mixer"},
	{"PredriveR PGA", NULL, "PredriveR Mixer"},
	/* HeadsetL */
	{"HeadsetL Mixer", "Voice", "Analog Voice Playback Mixer"},
	{"HeadsetL Mixer", "AudioL1", "Analog L1 Playback Mixer"},
	{"HeadsetL Mixer", "AudioL2", "Analog L2 Playback Mixer"},
	{"HeadsetL PGA", NULL, "HeadsetL Mixer"},
	/* HeadsetR */
	{"HeadsetR Mixer", "Voice", "Analog Voice Playback Mixer"},
	{"HeadsetR Mixer", "AudioR1", "Analog R1 Playback Mixer"},
	{"HeadsetR Mixer", "AudioR2", "Analog R2 Playback Mixer"},
	{"HeadsetR PGA", NULL, "HeadsetR Mixer"},
	/* CarkitL */
	{"CarkitL Mixer", "Voice", "Analog Voice Playback Mixer"},
	{"CarkitL Mixer", "AudioL1", "Analog L1 Playback Mixer"},
	{"CarkitL Mixer", "AudioL2", "Analog L2 Playback Mixer"},
	{"CarkitL PGA", NULL, "CarkitL Mixer"},
	/* CarkitR */
	{"CarkitR Mixer", "Voice", "Analog Voice Playback Mixer"},
	{"CarkitR Mixer", "AudioR1", "Analog R1 Playback Mixer"},
	{"CarkitR Mixer", "AudioR2", "Analog R2 Playback Mixer"},
	{"CarkitR PGA", NULL, "CarkitR Mixer"},
	/* HandsfreeL */
	{"HandsfreeL Mux", "Voice", "Analog Voice Playback Mixer"},
	{"HandsfreeL Mux", "AudioL1", "Analog L1 Playback Mixer"},
	{"HandsfreeL Mux", "AudioL2", "Analog L2 Playback Mixer"},
	{"HandsfreeL Mux", "AudioR2", "Analog R2 Playback Mixer"},
	{"HandsfreeL", "Switch", "HandsfreeL Mux"},
	{"HandsfreeL PGA", NULL, "HandsfreeL"},
	/* HandsfreeR */
	{"HandsfreeR Mux", "Voice", "Analog Voice Playback Mixer"},
	{"HandsfreeR Mux", "AudioR1", "Analog R1 Playback Mixer"},
	{"HandsfreeR Mux", "AudioR2", "Analog R2 Playback Mixer"},
	{"HandsfreeR Mux", "AudioL2", "Analog L2 Playback Mixer"},
	{"HandsfreeR", "Switch", "HandsfreeR Mux"},
	{"HandsfreeR PGA", NULL, "HandsfreeR"},
	/* Vibra */
	{"Vibra Mux", "AudioL1", "DAC Left1"},
	{"Vibra Mux", "AudioR1", "DAC Right1"},
	{"Vibra Mux", "AudioL2", "DAC Left2"},
	{"Vibra Mux", "AudioR2", "DAC Right2"},

	/* outputs */
	{"OUTL", NULL, "Analog L2 Playback Mixer"},
	{"OUTR", NULL, "Analog R2 Playback Mixer"},
    {"EARPIECE", NULL, "Earpiece Mixer"},
    {"PREDRIVEL", NULL, "PredriveL Mixer"},
    {"PREDRIVER", NULL, "PredriveR Mixer"},
	{"HSOL", NULL, "HeadsetL PGA"},
	{"HSOR", NULL, "HeadsetR PGA"},
    {"CARKITL", NULL, "CarkitL Mixer"},
    {"CARKITR", NULL, "CarkitR Mixer"},
	{"HFL", NULL, "HandsfreeL PGA"},
	{"HFR", NULL, "HandsfreeR PGA"},
	{"Vibra Route", "Audio", "Vibra Mux"},
	{"VIBRA", NULL, "Vibra Route"},

	/* Capture path */
    {"Analog Left Capture Route", "Main mic", "MAINMIC"},
    {"Analog Left Capture Route", "Headset mic", "HSMIC"},
    {"Analog Left Capture Route", "AUXL", "AUXL"},
    {"Analog Left Capture Route", "Carkit mic", "CARKITMIC"},

    {"Analog Right Capture Route", "Sub mic", "SUBMIC"},
    {"Analog Right Capture Route", "AUXR", "AUXR"},

    {"ADC Physical Left", NULL, "Analog Left Capture Route"},
    {"ADC Physical Right", NULL, "Analog Right Capture Route"},

	{"Digimic0 Enable", NULL, "DIGIMIC0"},
	{"Digimic1 Enable", NULL, "DIGIMIC1"},

	/* TX1 Left capture path */
	{"TX1 Capture Route", "Analog", "ADC Physical Left"},
	{"TX1 Capture Route", "Digimic0", "Digimic0 Enable"},
	/* TX1 Right capture path */
	{"TX1 Capture Route", "Analog", "ADC Physical Right"},
	{"TX1 Capture Route", "Digimic0", "Digimic0 Enable"},
	/* TX2 Left capture path */
	{"TX2 Capture Route", "Analog", "ADC Physical Left"},
	{"TX2 Capture Route", "Digimic1", "Digimic1 Enable"},
	/* TX2 Right capture path */
	{"TX2 Capture Route", "Analog", "ADC Physical Right"},
	{"TX2 Capture Route", "Digimic1", "Digimic1 Enable"},

	{"ADC Virtual Left1", NULL, "TX1 Capture Route"},
	{"ADC Virtual Right1", NULL, "TX1 Capture Route"},
	{"ADC Virtual Left2", NULL, "TX2 Capture Route"},
	{"ADC Virtual Right2", NULL, "TX2 Capture Route"},

    /* Analog bypass routes */
    {"Right1 Analog Loopback", "Switch", "Analog Right Capture Route"},
    {"Left1 Analog Loopback", "Switch", "Analog Left Capture Route"},
    {"Right2 Analog Loopback", "Switch", "Analog Right Capture Route"},
    {"Left2 Analog Loopback", "Switch", "Analog Left Capture Route"},
    {"Voice Analog Loopback", "Switch", "Analog Left Capture Route"},

	{"Analog R1 Playback Mixer", NULL, "Right1 Analog Loopback"},
	{"Analog L1 Playback Mixer", NULL, "Left1 Analog Loopback"},
	{"Analog R2 Playback Mixer", NULL, "Right2 Analog Loopback"},
	{"Analog L2 Playback Mixer", NULL, "Left2 Analog Loopback"},
	{"Analog Voice Playback Mixer", NULL, "Voice Analog Loopback"},

	/* Digital bypass routes */
	{"Right Digital Loopback", "Volume", "TX1 Capture Route"},
	{"Left Digital Loopback", "Volume", "TX1 Capture Route"},
	{"Voice Digital Loopback", "Volume", "TX2 Capture Route"},

	{"Digital R2 Playback Mixer", NULL, "Right Digital Loopback"},
	{"Digital L2 Playback Mixer", NULL, "Left Digital Loopback"},
	{"Digital Voice Playback Mixer", NULL, "Voice Digital Loopback"},

};

#if 0
static int twl4030_add_widgets(struct snd_soc_codec *codec)
{
  P("");

	snd_soc_dapm_new_controls(codec, twl4030_dapm_widgets,
				 ARRAY_SIZE(twl4030_dapm_widgets));

	snd_soc_dapm_add_routes(codec, intercon, ARRAY_SIZE(intercon));

	snd_soc_dapm_new_widgets(codec);
	return 0;
}
#endif

static int twl4030_set_bias_level(struct snd_soc_codec *codec,
				  enum snd_soc_bias_level level)
{
    P("twl4030_set_bias_level level = %d  \n",level);

	switch (level) {
	case SND_SOC_BIAS_ON:
        //twl4030_codec_mute(codec, 0);
		break;

	case SND_SOC_BIAS_PREPARE:
        #if 0        
		twl4030_power_up(codec);
		if (twl4030->bypass_state)
			twl4030_codec_mute(codec, 0);
		else
			twl4030_codec_mute(codec, 1);
        #endif        
		break;

	case SND_SOC_BIAS_STANDBY:
        #if 0
		twl4030_power_up(codec);
		if (twl4030->bypass_state)
			twl4030_codec_mute(codec, 0);
		else
			twl4030_codec_mute(codec, 1);
        #endif
		break;

	case SND_SOC_BIAS_OFF:
		twl4030_power_down(codec);
		break;
	}
	codec->bias_level = level;

	return 0;
}

static unsigned int twl4030_rate_min(struct substream_item *item, unsigned int rate)
{
  static const unsigned int table[] = {
      8000, 11025, 12000, 16000, 22050,
      24000, 32000, 44100, 48000, 96000};
  unsigned int value = rate;

  P(" rate : %d", rate);

  if (item->use256FS) {
    int i;
    rate *= 256;
    for (i = 0; i < ARRAY_SIZE(table); i++)
      if (rate % table[i] == 0) {
        value = table[i];
        break;
      }
  }
  return value;
}

static unsigned int twl4030_rate_max(struct substream_item *item, unsigned int rate)
{
  static const unsigned int table[] = {
      96000, 48000, 44100, 32000, 24000,
      22050, 16000, 12000, 11025, 8000};
  unsigned int value = rate;

  P("rate : %d", rate);

  if (item->use256FS) {
    int i;
    rate *= 256;
    for (i = 0; i < ARRAY_SIZE(table); i++)
      if (rate % table[i] == 0) {
        value = table[i];
        break;
      }
  }
  return value;
}

static void twl4030_constraints(struct twl4030_priv *twl4030)
{
    struct substream_item *item;
    unsigned int value;

    list_for_each_entry(item, &twl4030->started_list, started) {

	/* Set the constraints according to the already configured stream */
    value = params_rate(&twl4030->params);
    if (value)
      snd_pcm_hw_constraint_minmax(item->substream->runtime,
				SNDRV_PCM_HW_PARAM_RATE,
                                  twl4030_rate_min(item, value),
                                  twl4030_rate_max(item, value));

    value = hw_param_interval(&twl4030->params,
                              SNDRV_PCM_HW_PARAM_SAMPLE_BITS)->min;
    if (value && !item->use256FS)
      snd_pcm_hw_constraint_minmax(item->substream->runtime,
				SNDRV_PCM_HW_PARAM_SAMPLE_BITS,
                                  value, value);
  }
}

/* In case of 4 channel mode, the RX1 L/R for playback and the TX2 L/R for
 * capture has to be enabled/disabled. */
static void twl4030_tdm_enable(struct snd_soc_codec *codec, int direction,
				int enable)
{
	u8 reg, mask;

    P("enable : %d", enable);

	reg = twl4030_read_reg_cache(codec, TWL4030_REG_OPTION);

	if (direction == SNDRV_PCM_STREAM_PLAYBACK)
		mask = TWL4030_ARXL1_VRX_EN | TWL4030_ARXR1_EN;
	else
		mask = TWL4030_ATXL2_VTXL_EN | TWL4030_ATXR2_VTXR_EN;

	if (enable)
		reg |= mask;
	else
		reg &= ~mask;

	twl4030_write(codec, TWL4030_REG_OPTION, reg);
}

static int twl4030_new_substream(struct twl4030_priv *twl4030,
  struct snd_pcm_substream *substream, int use256FS)
{
  struct substream_item *item;

  P("use256FS : %d", use256FS);

  item = kzalloc(sizeof(struct snd_pcm_substream), GFP_KERNEL);
  if (!item)
    return -ENOMEM;

  item->substream = substream;
  item->use256FS = use256FS;

  mutex_lock(&twl4030->mutex);
  list_add_tail(&item->started, &twl4030->started_list);
  twl4030->extClock += item->use256FS;
  mutex_unlock(&twl4030->mutex);

  return 0;
}

static void twl4030_del_substream(struct twl4030_priv *twl4030,
  struct snd_pcm_substream *substream)
{
  struct substream_item *item;

  P("");

  mutex_lock(&twl4030->mutex);

  list_for_each_entry(item, &twl4030->config_list, configured) {
    if (item->substream == substream) {
      printk(KERN_ERR "TWL4030 deleted substream still configured!\n");
      list_del(&item->configured);
      break;
    }
  }

  list_for_each_entry(item, &twl4030->started_list, started) {
    if (item->substream == substream) {
      list_del(&item->started);
      twl4030->extClock -= item->use256FS;
      kfree(item);
      break;
    }
  }

  mutex_unlock(&twl4030->mutex);
}

static int twl4030_startup(struct snd_pcm_substream *substream,
			   struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_device *socdev = rtd->socdev;
	struct snd_soc_codec *codec = socdev->card->codec;
	struct twl4030_priv *twl4030 = codec->private_data;

  P("");

  cancel_delayed_work( &codec_down_work );

		if (!(twl4030_read_reg_cache(codec, TWL4030_REG_CODEC_MODE) &
                    TWL4030_OPTION_2)) {
			/* In option2 4 channel is not supported, set the
			 * constraint for the first stream for channels, the
			 * second stream will 'inherit' this cosntraint */
			snd_pcm_hw_constraint_minmax(substream->runtime,
						SNDRV_PCM_HW_PARAM_CHANNELS,
						2, 2);
		}

  return twl4030_new_substream(twl4030, substream, 0);
}

static void twl4030_shutdown(struct snd_pcm_substream *substream,
			     struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_device *socdev = rtd->socdev;
	struct snd_soc_codec *codec = socdev->card->codec;
	struct twl4030_priv *twl4030 = codec->private_data;

        P("");

  twl4030_del_substream(twl4030, substream);

	 /* If the closing substream had 4 channel, do the necessary cleanup */
	if (substream->runtime->channels == 4)
		twl4030_tdm_enable(codec, substream->stream, 0);
}

int twl4030_set_rate(struct snd_soc_codec *codec,
  struct snd_pcm_hw_params *params)
{
  struct twl4030_priv *twl4030 = codec->private_data;
  u8 mode, old_mode;

  P("");

  if (params_rate(&twl4030->params) &&
    params_rate(&twl4030->params) != params_rate(params)) {
    return -EBUSY;
  }

  /* bit rate */
  old_mode = twl4030_read_reg_cache(codec,
  TWL4030_REG_CODEC_MODE) & ~TWL4030_CODECPDZ;
  mode = old_mode & ~TWL4030_APLL_RATE;

  switch (params_rate(params)) {
    case 8000:
      mode |= TWL4030_APLL_RATE_8000;
      break;
    case 11025:
      mode |= TWL4030_APLL_RATE_11025;
      break;
    case 12000:
      mode |= TWL4030_APLL_RATE_12000;
      break;
    case 16000:
      mode |= TWL4030_APLL_RATE_16000;
      break;
    case 22050:
      mode |= TWL4030_APLL_RATE_22050;
      break;
    case 24000:
      mode |= TWL4030_APLL_RATE_24000;
      break;
    case 32000:
      mode |= TWL4030_APLL_RATE_32000;
      break;
    case 44100:
      mode |= TWL4030_APLL_RATE_44100;
      break;
    case 48000:
      mode |= TWL4030_APLL_RATE_48000;
      break;
    case 96000:
      mode |= TWL4030_APLL_RATE_96000;
      break;
    default:
      printk(KERN_ERR "TWL4030 hw params: unknown rate %d\n",
      params_rate(params));
      return -EINVAL;
  }

  params_rate(&twl4030->params) = params_rate(params);

  if (mode != old_mode) {
    /* change rate and set CODECPDZ */
    twl4030_codec_enable(codec, 0);
    printk("twl 4030 twl4030_set_rate mode = 0x%x \n",mode);              
    twl4030_write(codec, TWL4030_REG_CODEC_MODE, mode);
    twl4030_codec_enable(codec, 1);
  }

  return 0;
}
EXPORT_SYMBOL_GPL(twl4030_set_rate);

int twl4030_get_clock_divisor(struct snd_soc_codec *codec,
  struct snd_pcm_hw_params *params)
{
  struct twl4030_priv *twl4030 = codec->private_data;
  int clock, divisor;

  P("");

  clock = params_rate(&twl4030->params) * 256;
  divisor = clock / params_rate(params);
  divisor /= params_channels(params);

  switch (params_format(params)) {
    case SNDRV_PCM_FORMAT_U8:
    case SNDRV_PCM_FORMAT_S8:
      divisor /= 8;
      break;
    case SNDRV_PCM_FORMAT_S16_LE:
      divisor /= 16;
      break;
    case SNDRV_PCM_FORMAT_S24_LE:
      divisor /= 24;
      break;
    default:
      printk(KERN_ERR "TWL4030 get_clock_divisor: unknown format %d\n",
      params_format(params));
      return -EINVAL;
  }

  return divisor;
}
EXPORT_SYMBOL_GPL(twl4030_get_clock_divisor);

static int twl4030_set_format(struct snd_soc_codec *codec,
  struct snd_pcm_hw_params *params)
{
  struct twl4030_priv *twl4030 = codec->private_data;
  u8 format, old_format;

  P("");

  /* sample size */
  old_format = twl4030_read_reg_cache(codec, TWL4030_REG_AUDIO_IF);
  format = old_format & ~TWL4030_DATA_WIDTH;

  switch (params_format(params)) {
    case SNDRV_PCM_FORMAT_S16_LE:
      format |= TWL4030_DATA_WIDTH_16S_16W;
      break;
    case SNDRV_PCM_FORMAT_S24_LE:
      format |= TWL4030_DATA_WIDTH_32S_24W;
      break;
    default:
      printk(KERN_ERR "TWL4030 hw params: unknown format %d\n",
      params_format(params));
      return -EINVAL;
  }

  if (format == old_format)
    return 0;

  if (params_format(&twl4030->params) &&
      params_format(&twl4030->params) != params_format(params))
    return -EBUSY;

  *hw_param_mask(&twl4030->params, SNDRV_PCM_HW_PARAM_FORMAT) =
      *hw_param_mask(params, SNDRV_PCM_HW_PARAM_FORMAT);

  /* clear CODECPDZ before changing format (codec requirement) */
  twl4030_codec_enable(codec, 0);

  /* change format */
  P(" format = 0x%x \n",format);                  
  twl4030_write(codec, TWL4030_REG_AUDIO_IF, format);

  /* set CODECPDZ afterwards */
  twl4030_codec_enable(codec, 1);

  return 0;
}
static int twl4030_hw_params(struct snd_pcm_substream *substream,
			   struct snd_pcm_hw_params *params,
			   struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_device *socdev = rtd->socdev;
	struct snd_soc_codec *codec = socdev->card->codec;
	struct twl4030_priv *twl4030 = codec->private_data;
    int rval;

    P("");

    mutex_lock(&twl4030->mutex);

	 /* If the substream has 4 channel, do the necessary setup */
	if (params_channels(params) == 4) {
    /* Safety check: are we in the correct operating mode? */
    if ((twl4030_read_reg_cache(codec, TWL4030_REG_CODEC_MODE) &
    TWL4030_OPTION_2)) {
			twl4030_tdm_enable(codec, substream->stream, 1);
    } else {
      mutex_unlock(&twl4030->mutex);
			return -EINVAL;
	}
  }

  rval = twl4030_set_rate(codec, params);
  if (rval < 0) {
    mutex_unlock(&twl4030->mutex);
    printk(KERN_ERR "twl4030_hw_params: set rate failed, rate = %d\n",
           params_rate(params));
    return rval;
  }

  rval = twl4030_set_format(codec, params);
  if (rval < 0) {
    mutex_unlock(&twl4030->mutex);
    printk(KERN_ERR "twl4030_hw_params: set format failed %d\n", rval);
    return rval;
  }

  /* If any other streams are currently open, and one of them
  * is setting the hw parameters right now (since we are here), set
  * constraints to the other stream(s) to match the current one. */
  twl4030_constraints(twl4030);

  mutex_unlock(&twl4030->mutex);

  return 0;
}

static int twl4030_hw_free(struct snd_pcm_substream *substream,
  struct snd_soc_dai *dai)
{
  struct snd_soc_pcm_runtime *rtd = substream->private_data;
  struct snd_soc_device *socdev = rtd->socdev;
  struct snd_soc_codec *codec = socdev->card->codec;
  struct twl4030_priv *twl4030 = codec->private_data;
  struct substream_item *item;

  P("");

  mutex_lock(&twl4030->mutex);

  list_for_each_entry(item, &twl4030->config_list, configured) {
    if (item->substream == substream) {
      list_del (&item->configured);
      break;
    }
	}

  if (list_empty(&twl4030->config_list)) 
  {
    memset(&twl4030->params, 0, sizeof(twl4030->params));
  }

  mutex_unlock(&twl4030->mutex);

	return 0;
}

static int twl4030_set_dai_sysclk(struct snd_soc_dai *codec_dai,
		int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct twl4030_priv *twl4030 = codec->private_data;
	u8 infreq;

    P("freq : %d, dir : %d", freq, dir);

	switch (freq) {
	case 19200000:
		infreq = TWL4030_APLL_INFREQ_19200KHZ;
		twl4030->sysclk = 19200;
		break;
	case 26000000:
		infreq = TWL4030_APLL_INFREQ_26000KHZ;
		twl4030->sysclk = 26000;
		break;
	case 38400000:
		infreq = TWL4030_APLL_INFREQ_38400KHZ;
		twl4030->sysclk = 38400;
		break;
	default:
		printk(KERN_ERR "TWL4030 set sysclk: unknown rate %d\n",
			freq);
		return -EINVAL;
	}

	infreq |= TWL4030_APLL_EN;
	twl4030_write(codec, TWL4030_REG_APLL_CTL, infreq);

	return 0;
}

int twl4030_set_ext_clock(struct snd_soc_codec *codec, int enable)
{
     u8 old_format, format;
   
     P("enable : %d", enable);
   
     /* get format */
     old_format = twl4030_read_reg_cache(codec, TWL4030_REG_AUDIO_IF);
   
     if (enable)
       format = old_format | TWL4030_CLK256FS_EN;
     else
       format = old_format & ~TWL4030_CLK256FS_EN;
   
     if (format != old_format) {
       /* clear CODECPDZ before changing format (codec requirement) */
       //twl4030_codec_enable(codec, 0);
   
       /* change format */
       twl4030_write(codec, TWL4030_REG_AUDIO_IF, format);
   
       /* set CODECPDZ afterwards */
       //twl4030_codec_enable(codec, 1);
     }
   
     return 0;
}
EXPORT_SYMBOL_GPL(twl4030_set_ext_clock);

static int twl4030_set_dai_fmt(struct snd_soc_dai *codec_dai,
			     unsigned int fmt)
{
	struct snd_soc_codec *codec = codec_dai->codec;
    struct twl4030_priv *twl4030 = codec->private_data;
    int use256FS = 0;
	u8 old_format, format;

    P("fmt : %d", fmt);

	/* get format */
	old_format = twl4030_read_reg_cache(codec, TWL4030_REG_AUDIO_IF);
	format = old_format;

	/* set master/slave audio interface */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		format &= ~(TWL4030_AIF_SLAVE_EN);
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		format |= TWL4030_AIF_SLAVE_EN;
      use256FS = 1;
		break;
	default:
		return -EINVAL;
	}

	/* interface format */
	format &= ~TWL4030_AIF_FORMAT;
    format |= TWL4030_CLK256FS_EN;
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		format |= TWL4030_AIF_FORMAT_CODEC;
		break;
	case SND_SOC_DAIFMT_DSP_A:
		format |= TWL4030_AIF_FORMAT_TDM;
		break;
	default:
		return -EINVAL;
	}

	if (format != old_format) {
           /* clear CODECPDZ before changing format (codec requirement) */
           //twl4030_codec_enable(codec, 0);

           /* change format */
           twl4030_write(codec, TWL4030_REG_AUDIO_IF, format);

           /* set CODECPDZ afterwards */
           //twl4030_codec_enable(codec, 1);
	}

  return twl4030_set_ext_clock(codec, use256FS | twl4030->extClock);
}

static int twl4030_set_tristate(struct snd_soc_dai *dai, int tristate)
{
	struct snd_soc_codec *codec = dai->codec;
	u8 reg = twl4030_read_reg_cache(codec, TWL4030_REG_AUDIO_IF);

    P("tristate : %d", tristate);
   
	if (tristate)
		reg |= TWL4030_AIF_TRI_EN;
	else
		reg &= ~TWL4030_AIF_TRI_EN;

	return twl4030_write(codec, TWL4030_REG_AUDIO_IF, reg);
}

/* In case of voice mode, the RX1 L(VRX) for downlink and the TX2 L/R
 * (VTXL, VTXR) for uplink has to be enabled/disabled. */
static void twl4030_voice_enable(struct snd_soc_codec *codec, int direction,
				int enable)
{
	u8 reg, mask;

    P("enable : %d", enable);
   
	reg = twl4030_read_reg_cache(codec, TWL4030_REG_OPTION);

	if (direction == SNDRV_PCM_STREAM_PLAYBACK)
		mask = TWL4030_ARXL1_VRX_EN;
	else
		mask = TWL4030_ATXL2_VTXL_EN | TWL4030_ATXR2_VTXR_EN;

	if (enable)
		reg |= mask;
	else
		reg &= ~mask;

    P(" reg = 0x%x ",reg);
   
	twl4030_write(codec, TWL4030_REG_OPTION, reg);
}

static int twl4030_voice_startup(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_device *socdev = rtd->socdev;
	struct snd_soc_codec *codec = socdev->card->codec;
        struct twl4030_priv *twl4030 = codec->private_data;
	u8 infreq;
	u8 mode;

    P("");
   
	/* If the system master clock is not 26MHz, the voice PCM interface is
	 * not avilable.
	 */
	infreq = twl4030_read_reg_cache(codec, TWL4030_REG_APLL_CTL)
		& TWL4030_APLL_INFREQ;

	if (infreq != TWL4030_APLL_INFREQ_26000KHZ) {
		printk(KERN_ERR "TWL4030 voice startup: "
			"MCLK is not 26MHz, call set_sysclk() on init\n");
		return -EINVAL;
	}

	/* If the codec mode is not option2, the voice PCM interface is not
	 * avilable.
	 */
	mode = twl4030_read_reg_cache(codec, TWL4030_REG_CODEC_MODE)
		& TWL4030_OPT_MODE;

	if (mode != TWL4030_OPTION_2) {
		printk(KERN_ERR "TWL4030 voice startup: "
			"the codec mode is not option2\n");
		return -EINVAL;
	}

     return twl4030_new_substream(twl4030, substream, 1);
}

static void twl4030_voice_shutdown(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_device *socdev = rtd->socdev;
	struct snd_soc_codec *codec = socdev->card->codec;
    struct twl4030_priv *twl4030 = codec->private_data;
     
    P("");

    twl4030_del_substream(twl4030, substream);
   
	/* Enable voice digital filters */
	twl4030_voice_enable(codec, substream->stream, 0);
}

static int twl4030_voice_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_device *socdev = rtd->socdev;
	struct snd_soc_codec *codec = socdev->card->codec;
    struct twl4030_priv *twl4030 = codec->private_data;
	u8 old_mode, mode;

    P("");

	/* Enable voice digital filters */
	twl4030_voice_enable(codec, substream->stream, 1);

    mutex_lock(&twl4030->mutex);
   
	/* bit rate */
    old_mode = twl4030_read_reg_cache(codec,
    TWL4030_REG_CODEC_MODE) & ~TWL4030_CODECPDZ;
    mode = old_mode & ~TWL4030_APLL_RATE;

	switch (params_rate(params)) {
	case 8000:
		mode &= ~(TWL4030_SEL_16K);
		break;
	case 16000:
		mode |= TWL4030_SEL_16K;
		break;
	default:
         mutex_unlock(&twl4030->mutex);
		printk(KERN_ERR "TWL4030 voice hw params: unknown rate %d\n",
			params_rate(params));
		return -EINVAL;
	}

	if (mode != old_mode) {
		/* change rate and set CODECPDZ */
		twl4030_codec_enable(codec, 0);
   
       P(" mode = 0x%x ",mode);
   
		twl4030_write(codec, TWL4030_REG_CODEC_MODE, mode);
		twl4030_codec_enable(codec, 1);
	}

     mutex_unlock(&twl4030->mutex);
	return 0;
}

static int twl4030_voice_set_dai_sysclk(struct snd_soc_dai *codec_dai,
		int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	u8 infreq;

    P(" freq : %d, dir : %d", freq, dir);
   
	switch (freq) {
	case 26000000:
		infreq = TWL4030_APLL_INFREQ_26000KHZ;
		break;
	default:
		printk(KERN_ERR "TWL4030 voice set sysclk: unknown rate %d\n",
			freq);
		return -EINVAL;
	}

	infreq |= TWL4030_APLL_EN;
   
    printk("twl4030 twl4030_voice_set_dai_sysclk infreq = 0x%x ",infreq);
   
	twl4030_write(codec, TWL4030_REG_APLL_CTL, infreq);

	return 0;
}

static int twl4030_voice_set_dai_fmt(struct snd_soc_dai *codec_dai,
		unsigned int fmt)
{
	struct snd_soc_codec *codec = codec_dai->codec;
    struct twl4030_priv *twl4030 = codec->private_data;
    int use256FS = 0;
	u8 old_format, format;

    P("fmt : %d", fmt);
   
	/* get format */
	old_format = twl4030_read_reg_cache(codec, TWL4030_REG_VOICE_IF);
    format = old_format & ~TWL4030_VIF_TRI_EN;

	/* set master/slave audio interface */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
       case SND_SOC_DAIFMT_CBS_CFM:
		format &= ~(TWL4030_VIF_SLAVE_EN);
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		format |= TWL4030_VIF_SLAVE_EN;
         use256FS = 1;
		break;
	default:
		return -EINVAL;
	}

	/* clock inversion */
	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_IB_NF:
		format &= ~(TWL4030_VIF_FORMAT);
		break;
	case SND_SOC_DAIFMT_NB_IF:
		format |= TWL4030_VIF_FORMAT;
		break;
	default:
		return -EINVAL;
	}

	if (format != old_format) {
		/* change format and set CODECPDZ */
		twl4030_codec_enable(codec, 0);
   
                P(" format = 0x%x ",format);
   
		twl4030_write(codec, TWL4030_REG_VOICE_IF, format);
		twl4030_codec_enable(codec, 1);
	}

     return twl4030_set_ext_clock(codec, use256FS | twl4030->extClock);
}

static int twl4030_clock_startup(struct snd_pcm_substream *substream,
  struct snd_soc_dai *dai)
{
     struct snd_soc_pcm_runtime *rtd = substream->private_data;
     struct snd_soc_device *socdev = rtd->socdev;
     struct snd_soc_codec *codec = socdev->card->codec;
     struct twl4030_priv *twl4030 = codec->private_data;
   
     P("");
   
     return twl4030_new_substream(twl4030, substream, 1);
}

static int twl4030_clock_hw_params(struct snd_pcm_substream *substream,
  struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
     struct snd_soc_pcm_runtime *rtd = substream->private_data;
     struct snd_soc_device *socdev = rtd->socdev;
     struct snd_soc_codec *codec = socdev->card->codec;
     struct twl4030_priv *twl4030 = codec->private_data;
     int rval;
   
     P("");
   
     mutex_lock(&twl4030->mutex);
   
     rval = twl4030_set_rate(codec, params);
   
     /* See if we are a multiple of the current FS. If so, then still OK. */
     if (rval) {
       int divisor = twl4030_get_clock_divisor(codec, params);
       int clock = params_rate(&twl4030->params) * 256;
       int remainder = clock % params_rate(params);
   
       if (remainder == 0 && divisor <= 256)
         rval = 0;
     }
   
     /* If any other streams are currently open, and one of them
     * is setting the hw parameters right now (since we are here), set
     * constraints to the other stream(s) to match the current one. */
     twl4030_constraints(twl4030);
   
     mutex_unlock(&twl4030->mutex);
   
     return rval;
}

static int twl4030_clock_set_dai_fmt(struct snd_soc_dai *codec_dai,
  unsigned int fmt)
{
     struct snd_soc_codec *codec = codec_dai->codec;

     P("");
   
     return twl4030_set_ext_clock(codec, 1);
}

static int twl4030_voice_set_tristate(struct snd_soc_dai *dai, int tristate)
{
	struct snd_soc_codec *codec = dai->codec;
	u8 reg = twl4030_read_reg_cache(codec, TWL4030_REG_VOICE_IF);

    P("");
   
	if (tristate)
		reg |= TWL4030_VIF_TRI_EN;
	else
		reg &= ~TWL4030_VIF_TRI_EN;

    P(" reg = 0x%x ",reg);
   
	return twl4030_write(codec, TWL4030_REG_VOICE_IF, reg);
}

#define TWL4030_RATES	 (SNDRV_PCM_RATE_8000_48000)
#define TWL4030_FORMATS	 (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FORMAT_S24_LE)

static struct snd_soc_dai_ops twl4030_dai_ops = {
	.startup	= twl4030_startup,
	.shutdown	= twl4030_shutdown,
	.hw_params	= twl4030_hw_params,
	.hw_free    = twl4030_hw_free,
	.set_sysclk	= twl4030_set_dai_sysclk,
	.set_fmt	= twl4030_set_dai_fmt,
	.set_tristate	= twl4030_set_tristate,
};

static struct snd_soc_dai_ops twl4030_dai_voice_ops = {
	.startup	= twl4030_voice_startup,
	.shutdown	= twl4030_voice_shutdown,
	.hw_params	= twl4030_voice_hw_params,
    .hw_free    = twl4030_hw_free,	
	.set_sysclk	= twl4030_voice_set_dai_sysclk,
	.set_fmt	= twl4030_voice_set_dai_fmt,
	.set_tristate	= twl4030_voice_set_tristate,
};

static struct snd_soc_dai_ops twl4030_dai_clock_ops = {
    .startup    = twl4030_clock_startup,
    .shutdown   = twl4030_shutdown,
    .hw_params  = twl4030_clock_hw_params,
    .hw_free    = twl4030_hw_free,
    .set_sysclk  = twl4030_set_dai_sysclk,
    .set_fmt    = twl4030_clock_set_dai_fmt,
};

struct snd_soc_dai twl4030_dai[] = {
{
	.name = "twl4030",
	.playback = {
		.stream_name = "HiFi Playback",
		.channels_min = 2,
		.channels_max = 4,
        /*.rates = TWL4030_RATES | SNDRV_PCM_RATE_96000, */
        .rates = SNDRV_PCM_RATE_44100,
		.formats = TWL4030_FORMATS,},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 2,
		.channels_max = 4,
        /*.rates = TWL4030_RATES, */
        .rates = SNDRV_PCM_RATE_44100,
		.formats = TWL4030_FORMATS,},
    .ops = &twl4030_dai_ops,
},
{
	.name = "twl4030 Voice",
	.playback = {
		.stream_name = "Voice Playback",
		.channels_min = 1,
        .channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000,
		.formats = SNDRV_PCM_FMTBIT_S16_LE,},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000,
		.formats = SNDRV_PCM_FMTBIT_S16_LE,},
    .ops = &twl4030_dai_voice_ops,
  },
  {
    .name = "twl4030 Clock",
    .playback = {
        .stream_name = "Playback",
        .channels_min = 1,
        .channels_max = 2,
        .rates = TWL4030_RATES,
        .formats = SNDRV_PCM_FMTBIT_U8 | TWL4030_FORMATS,},
    .capture = {
        .stream_name = "Capture",
        .channels_min = 1,
        .channels_max = 2,
        .rates = TWL4030_RATES,
        .formats = SNDRV_PCM_FMTBIT_U8 | TWL4030_FORMATS,},
    .ops = &twl4030_dai_clock_ops,
},
};
EXPORT_SYMBOL_GPL(twl4030_dai);

static int twl4030_suspend(struct platform_device *pdev, pm_message_t state)
{
	 struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	 struct snd_soc_codec *codec = socdev->card->codec;

     P("");

     if((twl4030_mode !=VOICE_CALL)&&(twl4030_mode !=VOICE_MEMO)
		&&(twl4030_mode !=VOIP_CALL)&&(twl4030_mode !=FM_RADIO))
     {
          twl4030_playback_device = 0; //device off
          twl4030_call_device = 0;
          twl4030_voip_device = 0;	// hskwon-ss-cl31, added for FMC(VoIP) call path
		  twl4030_fm_device = 0;
          twl4030_codec_suspended = true;
          twl4030_set_bias_level(codec, SND_SOC_BIAS_OFF);
     }
	 return 0;
}

static int twl4030_resume(struct platform_device *pdev)
{
	 struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	 struct snd_soc_codec *codec = socdev->card->codec;

     P("");

     if((twl4030_mode !=VOICE_CALL)&&(twl4030_mode !=VOICE_MEMO)
		&&(twl4030_mode !=VOIP_CALL)&&(twl4030_mode !=FM_RADIO))
     {
          twl4030_codec_suspended = false;
      
          twl4030_set_bias_level(codec, SND_SOC_BIAS_STANDBY);
          twl4030_set_bias_level(codec, codec->suspend_bias_level);
     }
   
     return 0;
}

/*
 * initialize the driver
 * register the mixer and dsp interfaces with the kernel
 */

static int twl4030_init(struct snd_soc_device *socdev)
{
	struct snd_soc_codec *codec = socdev->card->codec;
    //struct twl4030_setup_data *setup = socdev->codec_data;
    struct twl4030_setup_data *setup;
	struct twl4030_priv *twl4030 = codec->private_data;
	int ret = 0;

    setup = socdev->codec_data;

    P("");

    printk(KERN_INFO "TWL4030 Audio Codec init \n");
   
	codec->name = "twl4030";
	codec->owner = THIS_MODULE;
	codec->read = twl4030_read_reg_cache;
	codec->write = twl4030_write;
	codec->set_bias_level = twl4030_set_bias_level;
	codec->dai = twl4030_dai;
	codec->num_dai = ARRAY_SIZE(twl4030_dai),
	codec->reg_cache_size = sizeof(twl4030_reg);
	codec->reg_cache = kmemdup(twl4030_reg, sizeof(twl4030_reg),
					GFP_KERNEL);
	if (codec->reg_cache == NULL)
		return -ENOMEM;
   
    INIT_DELAYED_WORK( &codec_control_work, codec_control_work_handler ); //sec_lilkan
    INIT_DELAYED_WORK( &codec_down_work, codec_down_work_handler ); //sec_lilkan
   
    wake_lock_init( &T2_wakelock, WAKE_LOCK_SUSPEND, "twl4030_codec" );
     
    twl4030_codec_suspended = false;
   
	/* Configuration for headset ramp delay from setup data */
	if (setup) {
		unsigned char hs_pop;

		if (setup->sysclk)
			twl4030->sysclk = setup->sysclk;
		else
			twl4030->sysclk = 26000;

		hs_pop = twl4030_read_reg_cache(codec, TWL4030_REG_HS_POPN_SET);
		hs_pop &= ~TWL4030_RAMP_DELAY;
		hs_pop |= (setup->ramp_delay_value << 2);
		twl4030_write_reg_cache(codec, TWL4030_REG_HS_POPN_SET, hs_pop);
	} else {
		twl4030->sysclk = 26000;
	}

	/* register pcms */
	ret = snd_soc_new_pcms(socdev, SNDRV_DEFAULT_IDX1, SNDRV_DEFAULT_STR1);
	if (ret < 0) {
		printk(KERN_ERR "twl4030: failed to create pcms\n");
		goto pcm_err;
	}

	twl4030_init_chip(codec);

	/* power on device */
	twl4030_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

    twl4030_add_controls(codec);
    //twl4030_add_widgets(codec);

	//printk("[light] power up VINTIANA2....\n");	
	twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,0x41, TWL4030_VINTANA2_DEDICATED); // ryun. 0x46 : VINTIANA2_DEDICATED
	twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,DEV_GRP_BELONG_P1, TWL4030_VINTANA2_DEV_GRP) ;// ryun. 0x43 : VINTANA2_DEV_GRP NA2_DEV_GRP 
	
	ret = snd_soc_init_card(socdev);
	if (ret < 0) {
		printk(KERN_ERR "twl4030: failed to register card\n");
		goto card_err;
	}

	return ret;

card_err:
	snd_soc_free_pcms(socdev);
	snd_soc_dapm_free(socdev);
pcm_err:
	kfree(codec->reg_cache);
	return ret;
}

static struct snd_soc_device *twl4030_socdev;

static int twl4030_probe(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec;
	struct twl4030_priv *twl4030;

    P("");
   
	codec = kzalloc(sizeof(struct snd_soc_codec), GFP_KERNEL);
	if (codec == NULL)
		return -ENOMEM;

	twl4030 = kzalloc(sizeof(struct twl4030_priv), GFP_KERNEL);
	if (twl4030 == NULL) {
		kfree(codec);
		return -ENOMEM;
	}

    mutex_init(&twl4030->mutex);
    INIT_LIST_HEAD(&twl4030->started_list);
    INIT_LIST_HEAD(&twl4030->config_list);
    codec->private_data = twl4030;
	socdev->card->codec = codec;
	mutex_init(&codec->mutex);
	INIT_LIST_HEAD(&codec->dapm_widgets);
	INIT_LIST_HEAD(&codec->dapm_paths);

	twl4030_socdev = socdev;
	twl4030_init(socdev);

	return 0;
}

static int twl4030_remove(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->card->codec;


    P("");

	twl4030_set_bias_level(codec, SND_SOC_BIAS_OFF);
	snd_soc_free_pcms(socdev);
	snd_soc_dapm_free(socdev);
	kfree(codec->private_data);
	kfree(codec);

	return 0;
}

struct snd_soc_codec_device soc_codec_dev_twl4030 = {
	.probe = twl4030_probe,
	.remove = twl4030_remove,
	.suspend = twl4030_suspend,
	.resume = twl4030_resume,
};
EXPORT_SYMBOL_GPL(soc_codec_dev_twl4030);

static int __init twl4030_modinit(void)
{
    return snd_soc_register_dais(twl4030_dai, ARRAY_SIZE(twl4030_dai));
}
module_init(twl4030_modinit);

static void __exit twl4030_exit(void)
{
    snd_soc_unregister_dais(twl4030_dai, ARRAY_SIZE(twl4030_dai));
}
module_exit(twl4030_exit);

MODULE_DESCRIPTION("ASoC TWL4030 codec driver");
MODULE_AUTHOR("Steve Sakoman");
MODULE_LICENSE("GPL");
