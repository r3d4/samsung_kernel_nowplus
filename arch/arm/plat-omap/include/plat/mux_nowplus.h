/*
 * arch/arm/plat-omap/include/mach/mux_oscar.h
 *
 */
 
 
#ifndef _MUX_NOWPLUS_H_
#define _MUX_NOWPLUS_H_

/* 24xx/34xx mux bit defines */
#define OMAP2_PULL_ENA          (1 << 3)
#define OMAP2_PULL_UP           (1 << 4)
#define OMAP2_ALTELECTRICALSEL  (1 << 5)

/* 34xx specific mux bit defines */
#define OMAP3_INPUT_EN          (1 << 8)
#define OMAP3_OFF_EN            (1 << 9)
#define OMAP3_OFFIN_EN          (1 << 10)
#define OMAP3_OFFOUT_VAL        (1 << 11)
#define OMAP3_OFF_PULL_EN       (1 << 12)
#define OMAP3_OFF_PULL_UP       (1 << 13)
#define OMAP3_WAKEUP_EN         (1 << 14)

/* 34xx mux mode options for each pin. See TRM for options */
#define OMAP34XX_MUX_MODE0      0
#define OMAP34XX_MUX_MODE1      1
#define OMAP34XX_MUX_MODE2      2
#define OMAP34XX_MUX_MODE3      3
#define OMAP34XX_MUX_MODE4      4
#define OMAP34XX_MUX_MODE5      5
#define OMAP34XX_MUX_MODE6      6
#define OMAP34XX_MUX_MODE7      7

/* 34xx active pin states */
#define OMAP34XX_PIN_OUTPUT             0
#define OMAP34XX_PIN_INPUT              OMAP3_INPUT_EN
#define OMAP34XX_PIN_INPUT_PULLUP       (OMAP2_PULL_ENA | OMAP3_INPUT_EN \
                                                | OMAP2_PULL_UP)
#define OMAP34XX_PIN_INPUT_PULLDOWN     (OMAP2_PULL_ENA | OMAP3_INPUT_EN)

/* 34xx off mode states */
#define OMAP34XX_PIN_OFF_NONE           0
#define OMAP34XX_PIN_OFF_OUTPUT_HIGH    (OMAP3_OFF_EN | OMAP3_OFFOUT_VAL)
#define OMAP34XX_PIN_OFF_OUTPUT_LOW     (OMAP3_OFF_EN)
#define OMAP34XX_PIN_OFF_INPUT          (OMAP3_OFF_EN | OMAP3_OFFIN_EN)
#define OMAP34XX_PIN_OFF_INPUT_PULLUP   (OMAP3_OFF_EN | OMAP3_OFFIN_EN \
                                                | OMAP3_OFF_PULL_EN \
                                                | OMAP3_OFF_PULL_UP)
#define OMAP34XX_PIN_OFF_INPUT_PULLDOWN (OMAP3_OFF_EN | OMAP3_OFFIN_EN \
                                                | OMAP3_OFF_PULL_EN)
#define OMAP34XX_PIN_OFF_WAKEUPENABLE   OMAP3_WAKEUP_EN

#define MUX_CFG_34XX(desc, reg_offset, mux_value) {             \
        .name           = desc,                                 \
        .debug          = 0,                                    \
        .mux_reg        = reg_offset,                           \
        .mux_val        = mux_value                             \
},

#define OMAP_GPIO_FM_INT            3    //r
#define OMAP_GPIO_USBSW_NINT        10    //r
#define OMAP_GPIO_AP_NAND_INT       13    //r
#define OMAP_GPIO_AP_ALARM          14    //r
#define OMAP_GPIO_INT_ONEDRAM_AP    15    //r
#define OMAP_GPIO_CHG_ING_N         16    //r
#define OMAP_GPIO_WLAN_IRQ          21    //r
#define OMAP_GPIO_SYS_DRM_MSECURE   22  //r  //output
#define OMAP_GPIO_TF_DETECT         23    //r
#define OMAP_GPIO_KEY_PWRON         24    //r
#define OMAP_GPIO_PS_HOLD_PU        25  //r  //output H
#define OMAP_GPIO_EAR_KEY           26    //r
#define OMAP_GPIO_DET_3_5           27    //r
#define OMAP_GPIO_PCM_SEL           52    //r
#define OMAP_GPIO_PS_OUT            53    //r
#define OMAP_GPIO_IF_CON_SENSE      54    //r
#define OMAP_GPIO_VF                56    //r
#define OMAP_GPIO_BT_NSHUTDOWN      63  //r  //output
#define OMAP_GPIO_VGA_RST           64  //r     
#define OMAP_GPIO_FM_nRST           65    //r
#define OMAP_GPIO_CAM_RST           98  //r  //output
#define OMAP_GPIO_LCD_REG_RST       99    //r
#define OMAP_GPIO_ISP_INT           100 //r
#define OMAP_GPIO_VGA_STBY          101    //r  
#define OMAP_GPIO_PDA_ACTIVE        111   //r    //output
#define OMAP_GPIO_ACC_INT           115      //r
#define OMAP_GPIO_UART_SEL          126   //r    //output
#define OMAP_GPIO_FONE_ON           140      //r    //output
#define OMAP_GPIO_MOTOR_EN          141   //r    //output
#define OMAP_GPIO_TOUCH_IRQ         142   //r
#define OMAP_GPIO_USB_SEL           150    //r //output
#define OMAP_GPIO_RGB_RST           151      //r //output
#define OMAP_GPIO_CAM_EN            152   //r //output
#define OMAP_GPIO_PHONE_ACTIVE      154      //r
#define OMAP_GPIO_MOVI_EN           155         //r
#define OMAP_GPIO_HW_REV0           156      //r
#define OMAP_GPIO_CHG_EN            157   //r    //output 
#define OMAP_GPIO_HW_REV1           158      //r
#define OMAP_GPIO_HW_REV2           159      //r
#define OMAP_GPIO_WLAN_EN           160   //r    //output
#define OMAP_GPIO_LCD_ID            161      //r
#define OMAP_GPIO_EAR_MIC_LDO_EN    162   //r    //output
#define OMAP_GPIO_CAMERA_LEVEL_CTRL 167   //output H //OMAP3430_GPIO_CAMERA_LEVEL_CTRL
#define OMAP_GPIO_MLCD_RST          170   //r    //output H
#define OMAP_GPIO_TVOUT_SEL         177
#define OMAP_GPIO_EAR_ADC_SEL       177
#define OMAP_GPIO_MSM_RST18_N       178   //r //output H


#endif

