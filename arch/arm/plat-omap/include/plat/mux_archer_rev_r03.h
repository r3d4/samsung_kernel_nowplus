/*
 * arch/arm/plat-omap/include/mach/mux_archer.h
 *
 */

#ifndef _MUX_ARCHER_H_
#define _MUX_ARCHER_H_

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

#define OMAP_GPIO_USBSW_NINT		10
#define OMAP_GPIO_FUEL_INT_N		12
#define OMAP_GPIO_AP_NAND_INT		13
#define OMAP_GPIO_PS_HOLD_PU		14	//output
#define OMAP_GPIO_INT_ONEDRAM_AP	15
#define OMAP_GPIO_CHG_ING_N		16
#define OMAP_GPIO_WLAN_IRQ		21
#define OMAP_GPIO_SYS_DRM_MSECURE	22	//output
#define OMAP_GPIO_TF_DETECT		23
#define OMAP_GPIO_KEY_PWRON		24
#define OMAP_GPIO_AP_ALARM		25	//output H
#define OMAP_GPIO_EAR_KEY		26
#define OMAP_GPIO_DET_3_5		27
#define OMAP_GPIO_PS_OUT		28 // 53
#define OMAP_GPIO_TA_NCONNECTED		57
#define OMAP_GPIO_IF_CON_SENSE		54
#define OMAP_GPIO_MIC_SEL		58	//output
#define OMAP_GPIO_HAND_FLASH		60	//output
#define OMAP_GPIO_LED_EN		61	//output
#define OMAP_GPIO_CAM_1P8V_EN		62	//output
#define OMAP_GPIO_BT_NSHUTDOWN		63	//output
#define OMAP_GPIO_PM_INT_N18		64
#define OMAP_GPIO_CP_BOOT_SEL		65	//output
#define OMAP_GPIO_CAM_RST		98	//output
#define OMAP_GPIO_LCD_REG_RST		99
#define OMAP_GPIO_ISP_INT		100
#define OMAP_GPIO_USIM_BOOT		101	//output
#define OMAP_GPIO_CAM_1P2V_EN		102	//output
#define OMAP_GPIO_PDA_ACTIVE		111	//output

#ifdef CONFIG_TDMB
#define OMAP_GPIO_TDMB_INT		113
#endif

#define OMAP_GPIO_COMPASS_INT		114
#define OMAP_GPIO_ACC_INT 		115
#define OMAP_GPIO_UART_SEL		126	//output

#ifdef CONFIG_TDMB
#define OMAP_GPIO_TDMB_RST		136
#endif

#define OMAP_GPIO_FONE_ON		140	//output
#define OMAP_GPIO_MOTOR_EN		141	//output
#define OMAP_GPIO_TOUCH_IRQ		142
#define OMAP_GPIO_TSP_EN		149
#define OMAP_GPIO_USB_SEL		150	//output
#define OMAP_GPIO_CAM_EN		152	//output
#define OMAP_GPIO_CAM_STBY		153	//output
#define OMAP_GPIO_PHONE_ACTIVE		154

#ifdef CONFIG_TDMB
#define OMAP_GPIO_TDMB_EN		155	//output
#endif
#define OMAP_GPIO_HW_REV0		156
#define OMAP_GPIO_CHG_EN		157	//output 
#define OMAP_GPIO_HW_REV1		158
#define OMAP_GPIO_HW_REV2		159
#define OMAP_GPIO_WLAN_EN		160	//output
#define OMAP_GPIO_LCD_ID		161
#define OMAP_GPIO_EAR_MIC_LDO_EN	162	//output
#define OMAP_GPIO_COMPASS_RST		164	//output H
#define OMAP_GPIO_VGA_SEL		167	//output H
#define OMAP_GPIO_MLCD_RST		170	//output H
#define OMAP_GPIO_EAR_ADC_SEL		177	//output
#define OMAP_GPIO_MSM_RST18_N		178	//output H

#define OMAP_GPIO_VGA_RST		180	// output
#define OMAP_GPIO_VGA_STBY		181	// output

#ifdef CONFIG_SAMSUNG_ARCHER_TARGET_SK
#define OMAP_GPIO_TSP_SCL		184
#define OMAP_GPIO_TSP_SDA		185
#define OMAP_GPIO_TSP_INT		186
#endif
#endif
