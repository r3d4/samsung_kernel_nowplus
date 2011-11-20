
unsigned int output_gpio[][3] = {
//	{ GPIO NUMBER, DEFAULT VALUE, LABEL-STRING  }

/*
 * Note that in 2.6.35 kernel, gpio_request() can only be made once, for pins added in output_pins[][]
 * omap_gpio_out_init() actually does gpio_request(), so if a driver is using that pin does a gpio_request()
 * again it will fail and driver will not initialize. Such pins shouldn't be included in output_pins[][]
 *
 * If certain pins needs to be put here pls do so.
 */
#if 0
//	{ GPIO NUMBER      , DEFAULT VALUE }	
{ OMAP_GPIO_FM_INT	    ,0 },
//{ OMAP_GPIO_USBSW_NINT        ,0 },
{ OMAP_GPIO_AP_NAND_INT       ,0 },
{ OMAP_GPIO_AP_ALARM          ,0 },
//{ OMAP_GPIO_INT_ONEDRAM_AP    ,0 },
{ OMAP_GPIO_CHG_ING_N         ,0 },
{ OMAP_GPIO_WLAN_IRQ          ,0 },
{ OMAP_GPIO_SYS_DRM_MSECURE   ,0 },
//{ OMAP_GPIO_TF_DETECT         ,0 },
{ OMAP_GPIO_PS_HOLD_PU        ,1 },
//{ OMAP_GPIO_EAR_KEY           ,0 },
//{ OMAP_GPIO_DET_3_5           ,0 },
{ OMAP_GPIO_PCM_SEL	    ,0 },
{ OMAP_GPIO_PS_OUT            ,0 },
{ OMAP_GPIO_IF_CON_SENSE      ,0 },
{ OMAP_GPIO_VF	 	    ,0 },
{ OMAP_GPIO_BT_NSHUTDOWN      ,0 },
{ OMAP_GPIO_VGA_RST        ,0 },
{ OMAP_GPIO_FM_nRST	    ,0 },
{ OMAP_GPIO_CAM_RST           ,0 },
{ OMAP_GPIO_LCD_REG_RST       ,0 },
{ OMAP_GPIO_ISP_INT           ,0 },
{ OMAP_GPIO_VGA_STBY          ,0 },
{ OMAP_GPIO_PDA_ACTIVE        ,0 },
{ OMAP_GPIO_ACC_INT           ,0 },
{ OMAP_GPIO_UART_SEL          ,0 },
{ OMAP_GPIO_FONE_ON	    ,0 },
{ OMAP_GPIO_MOTOR_EN          ,0 },
{ OMAP_GPIO_TOUCH_IRQ         ,0 },
{ OMAP_GPIO_USB_SEL           ,0 },
{ OMAP_GPIO_RGB_RST	    ,1 },
{ OMAP_GPIO_CAM_EN            ,0 },
{ OMAP_GPIO_PHONE_ACTIVE      ,0 },
{ OMAP_GPIO_MOVI_EN	    ,0 },
{ OMAP_GPIO_HW_REV0           ,0 },
{ OMAP_GPIO_CHG_EN            ,1 },
{ OMAP_GPIO_HW_REV1           ,0 },
{ OMAP_GPIO_HW_REV2           ,0 },
{ OMAP_GPIO_WLAN_EN           ,0 },
{ OMAP_GPIO_LCD_ID            ,0 },
{ OMAP_GPIO_EAR_MIC_LDO_EN    ,0 },
{ OMAP_GPIO_CAMERA_LEVEL_CTRL ,0 },
{ OMAP_GPIO_MLCD_RST          ,1 },
{ OMAP_GPIO_TVOUT_SEL         ,0 },
{ OMAP_GPIO_MSM_RST18_N       ,1 },
#endif
};

unsigned int wakeup_gpio[] = {
	OMAP_GPIO_USBSW_NINT,
	OMAP_GPIO_INT_ONEDRAM_AP,
	OMAP_GPIO_TF_DETECT,
	OMAP_GPIO_KEY_PWRON,
	OMAP_GPIO_EAR_KEY,
	OMAP_GPIO_DET_3_5,
};
