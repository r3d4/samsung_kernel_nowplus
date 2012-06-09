
#include <mach/hardware.h>
#include <plat/mux.h>

#ifndef _SWITCH_OMAP_GPIO_H_
#define _SWITCH_OMAP_GPIO_H_

#define HEADSET_DISCONNET			0
#define HEADSET_3POLE				2 
#define HEADSET_4POLE_WITH_MIC	    1

extern short int get_headset_status(void);


#define EAR_MIC_BIAS_GPIO           OMAP_GPIO_EAR_MIC_LDO_EN
#define EAR_KEY_GPIO                OMAP_GPIO_EAR_KEY 
#define EAR_DET_GPIO                OMAP_GPIO_DET_3_5
#define EAR_ADC_SEL_GPIO            OMAP_GPIO_EAR_ADC_SEL
#define EAR_KEY_INVERT_ENABLE       1
#define EAR_DETECT_INVERT_ENABLE    0


#endif//_SWITCH_OMAP_GPIO_H_
