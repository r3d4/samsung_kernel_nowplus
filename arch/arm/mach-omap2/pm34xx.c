/*
 * OMAP3 Power Management Routines
 *
 * Copyright (C) 2006-2008 Nokia Corporation
 * Tony Lindgren <tony@atomide.com>
 * Jouni Hogander
 *
 * Copyright (C) 2007 Texas Instruments, Inc.
 * Rajendra Nayak <rnayak@ti.com>
 *
 * Copyright (C) 2005 Texas Instruments, Inc.
 * Richard Woodruff <r-woodruff2@ti.com>
 *
 * Based on pm.c for omap1
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/pm.h>
#include <linux/suspend.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/reboot.h>

#include <plat/sram.h>
#include <plat/clockdomain.h>
#include <plat/powerdomain.h>
#include <plat/control.h>
#include <plat/serial.h>
#include <plat/sdrc.h>
#include <plat/prcm.h>
#include <plat/gpmc.h>
#include <plat/dma.h>
#include <plat/dmtimer.h>
#include <plat/usb.h>
#include <plat/omap-pm.h>
#include <plat/omap_device.h>
#include <plat/omap3-gptimer12.h> // SAMSUNG egkim

#include <asm/tlbflush.h>

#include "cm.h"
#include "cm-regbits-34xx.h"
#include "prm-regbits-34xx.h"
#include "clock.h"
#include "prm.h"
#include "pm.h"
#include "sdrc.h"

#if 0
struct cm_regdump {
	char *reg_name;
	char api;
	unsigned reg_addr;
	unsigned reg_val;
} cm_regs[] = {
	{"SCM"},
	{"CONTROL_SYSCONFIG:   ", 'V', (0x48002010)},
	{"PLL"},
	{"CM_CLKEN_PLL:     ", 'V', (0x48004D00)},
	{"CM_CLKEN2_PLL:    ", 'V', (0x48004D04)},
	{"CM_IDLEST_CKGEN:  ", 'V', (0x48004D20)},
	{"CM_IDLEST2_CKGEN: ", 'V', (0x48004D24)},
	{"CM_AUTOIDLE_PLL:  ", 'V', (0x48004D30)},
	{"CM_AUTOIDLE2_PLL: ", 'V', (0x48004D34)},
	{"CM_CLKSEL1_PLL:   ", 'V', (0x48004D40)},
	{"CM_CLKSEL2_PLL:   ", 'V', (0x48004D44)},
	{"CM_CLKSEL3_PLL:   ", 'V', (0x48004D48)},
	{"CM_CLKSEL4_PLL:   ", 'V', (0x48004D4C)},
	{"CM_CLKSEL5_PLL:   ", 'V', (0x48004D50)},
	{"CM_CLKOUT_CTRL:   ", 'V', (0x48004D70)},
	{"\n"},
	#if 1
	{"DSS"},
	{"CM_FCLKEN_DSS:       ", 'V', (0x48004E00)},
	{"CM_ICLKEN_DSS:       ", 'V', (0x48004E10)},
	{"CM_IDLEST_DSS:       ", 'V', (0x48004E20)},
	{"CM_AUTOIDLE_DSS:     ", 'V', (0x48004E30)},
	{"CM_CLKSEL_DSS:       ", 'V', (0x48004E40)},
	{"CM_SLEEPDEP_DSS:     ", 'V', (0x48004E44)},
	{"CM_CLKSTCTRL_DSS:    ", 'V', (0x48004E48)},
	{"CM_CLKSTST_DSS:      ", 'V', (0x48004E4C)},
	{"\n"},
	{"RM_RSTST_DSS:        ", 'V', (0x48306E58)},
	{"PM_WKEN_DSS:         ", 'V', (0x48306EA0)},
	{"PM_WKDEP_DSS:        ", 'V', (0x48306EC8)},
	{"PM_PWSTCTRL_DSS:     ", 'V', (0x48306EE0)},
	{"PM_PWSTST_DSS:       ", 'V', (0x48306EE4)},
	{"PM_PREPWSTST_DSS:    ", 'V', (0x48306EE8)},
	#endif
	{"MPU"},
	{"CM_CLKEN_PLL_MPU:    ", 'V', (0x48004904)},
	{"CM_IDLEST_MPU:       ", 'V', (0x48004920)},
	{"CM_IDLEST_PLL_MPU:   ", 'V', (0x48004924)},
	{"CM_AUTOIDLE_PLL_MPU: ", 'V', (0x48004934)},
	{"CM_CLKSEL1_PLL_MPU:  ", 'V', (0x48004940)},
	{"CM_CLKSEL2_PLL_MPU:  ", 'V', (0x48004944)},
	{"CM_CLKSTCTRL_MPU:    ", 'V', (0x48004948)},
	{"CM_CLKSTST_MPU:      ", 'V', (0x4800494C)},
	{"\n"},
	{"RM_RSTST_MPU:        ", 'V', (0x48306958)},
	{"PM_WKDEP_MPU:        ", 'V', (0x483069C8)},
	{"PM_EVGENCTRL_MPU:    ", 'V', (0x483069D4)},
	{"PM_EVGENONTIM_MPU:   ", 'V', (0x483069D8)},
	{"PM_EVGENOFFTIM_MPU:  ", 'V', (0x483069DC)},
	{"PM_PWSTCTRL_MPU:     ", 'V', (0x483069E0)},
	{"PM_PWSTST_MPU:       ", 'V', (0x483069E4)},
	{"PM_PREPWSTST_MPU:    ", 'V', (0x483069E8)},
	#if 1
	{"CORE"},
	{"CM_FCLKEN1_CORE:     ", 'V', (0x48004A00)},
	{"CM_FCLKEN3_CORE:     ", 'V', (0x48004A08)},
	{"CM_ICLKEN1_CORE:     ", 'V', (0x48004A10)},
	{"CM_ICLKEN3_CORE:     ", 'V', (0x48004A18)},
	{"CM_IDLEST1_CORE:     ", 'V', (0x48004A20)},
	{"CM_IDLEST3_CORE:     ", 'V', (0x48004A28)},
	{"CM_AUTOIDLE1_CORE:   ", 'V', (0x48004A30)},
	{"CM_AUTOIDLE3_CORE:   ", 'V', (0x48004A38)},
	{"CM_CLKSEL_CORE:      ", 'V', (0x48004A40)},
	{"CM_CLKSTCTRL_CORE:   ", 'V', (0x48004A48)},
	{"CM_CLKSTST_CORE:     ", 'V', (0x48004A4C)},
	{"\n"},
	{"RM_RSTST_CORE:       ", 'V', (0x48306A58)},
	{"PM_WKEN1_CORE:       ", 'V', (0x48306AA0)},
	{"PM_MPUGRPSEL1_CORE:  ", 'V', (0x48306AA4)},
	{"PM_IVA2GRPSEL1_CORE: ", 'V', (0x48306AA8)},
	{"PM_WKST1_CORE:       ", 'V', (0x48306AB0)},
	{"PM_WKST3_CORE:       ", 'V', (0x48306AB8)},
	{"PM_PWSTCTRL_CORE:    ", 'V', (0x48306AE0)},
	{"PM_PWSTST_CORE:      ", 'V', (0x48306AE4)},
	{"PM_PREPWSTST_CORE:   ", 'V', (0x48306AE8)},
	{"PM_WKEN3_CORE:       ", 'V', (0x48306AF0)},
	{"PM_IVA2GRPSEL3_CORE: ", 'V', (0x48306AF4)},
	{"PM_MPUGRPSEL3_CORE:  ", 'V', (0x48306AF8)},
	{"CAM"},
	{"CM_FCLKEN_CAM:       ", 'V', (0x48004F00)},
	{"CM_ICLKEN_CAM:       ", 'V', (0x48004F10)},
	{"CM_IDLEST_CAM:       ", 'V', (0x48004F20)},
	{"CM_AUTOIDLE_CAM:     ", 'V', (0x48004F30)},
	{"CM_CLKSEL_CAM:       ", 'V', (0x48004F40)},
	{"CM_SLEEPDEP_CAM:     ", 'V', (0x48004F44)},
	{"CM_CLKSTCTRL_CAM:    ", 'V', (0x48004F48)},
	{"CM_CLKSTST_CAM:      ", 'V', (0x48004F4C)},
	{"\n"},
	{"RM_RSTST_CAM:        ", 'V', (0x48306F58)},
	{"PM_WKDEP_CAM:        ", 'V', (0x48306FC8)},
	{"PM_PWSTCTRL_CAM:     ", 'V', (0x48306FE0)},
	{"PM_PWSTST_CAM:       ", 'V', (0x48306FE4)},
	{"PM_PREPWSTST_CAM:    ", 'V', (0x48306FE8)},
	{"\n"},
	{"PER"},
	{"CM_FCLKEN_PER:       ", 'V', (0x48005000)},
	{"CM_ICLKEN_PER:       ", 'V', (0x48005010)},
	{"CM_IDLEST_PER:       ", 'V', (0x48005020)},
	{"CM_AUTOIDLE_PER:     ", 'V', (0x48005030)},
	{"CM_CLKSEL_PER:       ", 'V', (0x48005040)},
	{"CM_SLEEPDEP_PER:     ", 'V', (0x48005044)},
	{"CM_CLKSTCTRL_PER:    ", 'V', (0x48005048)},
	{"CM_CLKSTST_PER:      ", 'V', (0x4800504C)},
	{"\n"},
	{"RM_RSTST_PER:        ", 'V', (0x48307058)},
	{"PM_WKEN_PER:         ", 'V', (0x483070A0)},
	{"PM_MPUGRPSEL_PER:    ", 'V', (0x483070A4)},
	{"PM_IVA2GRPSEL_PER:   ", 'V', (0x483070A8)},
	{"PM_WKST_PER:         ", 'V', (0x483070B0)},
	{"PM_WKDEP_PER:        ", 'V', (0x483070C8)},
	{"PM_PWSTCTRL_PER:     ", 'V', (0x483070E0)},
	{"PM_PWSTST_PER:       ", 'V', (0x483070E4)},
	{"PM_PREPWSTST_PER:    ", 'V', (0x483070E8)},
	{"NEON"},
	{"CM_IDLEST_NEON:      ", 'V', (0x48005320)},
	{"CM_CLKSTCTRL_NEON:   ", 'V', (0x48005348)},
	{"\n"},
	{"RM_RSTST_NEON:       ", 'V', (0x48307358)},
	{"PM_WKDEP_NEON:       ", 'V', (0x483073C8)},
	{"PM_PWSTCTRL_NEON:    ", 'V', (0x483073E0)},
	{"PM_PWSTST_NEON:      ", 'V', (0x483073E4)},
	{"PM_PREPWSTST_NEON:   ", 'V', (0x483073E8)},
	{"IVA2"},
	{"CM_FCLKEN_IVA2:      ", 'V', (0x48004000)},
	{"CM_CLKEN_PLL_IVA2:   ", 'V', (0x48004004)},
	{"CM_IDLEST_IVA2:      ", 'V', (0x48004020)},
	{"CM_IDLEST_PLL_IVA2:  ", 'V', (0x48004024)},
	{"CM_AUTOIDLE_PLL_IVA2:", 'V', (0x48004034)},
	{"CM_CLKSEL1_PLL_IVA2: ", 'V', (0x48004040)},
	{"CM_CLKSEL2_PLL_IVA2: ", 'V', (0x48004044)},
	{"CM_CLKSTCTRL_IVA2:   ", 'V', (0x48004048)},
	{"CM_CLKSTST_IVA2:     ", 'V', (0x4800404C)},
	{"\n"},
	{"RM_RSTCTRL_IVA2:     ", 'V', (0x48306050)},
	{"PM_RSTST_PLL_IVA2:   ", 'V', (0x48306058)},
	{"PM_WKDEP_IVA2:       ", 'V', (0x483060C8)},
	{"PM_PWSTCTRL_IVA2:    ", 'V', (0x483060E0)},
	{"PM_PWSTST_IVA2:      ", 'V', (0x483060E4)},
	{"PM_PREPWSTST_IVA2:   ", 'V', (0x483060E8)},
	{"PM_IRQSTATUS_IVA2:   ", 'V', (0x483060F8)},
	{"PM_IRQENABLE_IVA2:   ", 'V', (0x483060FC)},
	{"SGX"},
	{"CM_FCLKEN_SGX:       ", 'V', (0x48004B00)},
	{"CM_ICLKEN_SGX:       ", 'V', (0x48004B10)},
	{"CM_IDLEST_SGX:       ", 'V', (0x48004B20)},
	{"CM_CLKSEL_SGX:       ", 'V', (0x48004B40)},
	{"CM_SLEEPDEP_SGX:     ", 'V', (0x48004B44)},
	{"CM_CLKSTCTRL_SGX:    ", 'V', (0x48004B48)},
	{"CM_CLKSTST_SGX:      ", 'V', (0x48004B4C)},
	{"\n"},
	{"RM_RSTST_SGX:        ", 'V', (0x48306B58)},
	{"PM_WKDEP_SGX:        ", 'V', (0x48306BC8)},
	{"PM_PWSTCTRL_SGX:     ", 'V', (0x48306BE0)},
	{"PM_PWSTST_SGX:       ", 'V', (0x48306BE4)},
	{"PM_PREPWSTST_SGX:    ", 'V', (0x48306BE8)},
	{"WKUP"},
	{"CM_FCLKEN_WKUP:      ", 'V', (0x48004C00)},
	{"CM_ICLKEN_WKUP:      ", 'V', (0x48004C10)},
	{"CM_IDLEST_WKUP:      ", 'V', (0x48004C20)},
	{"CM_AUTOIDLE_WKUP:    ", 'V', (0x48004C30)},
	{"CM_CLKSEL_WKUP:      ", 'V', (0x48004C40)},
	{"\n"},
	{"RM_WKEN_WKUP:        ", 'V', (0x48306CA0)},
	{"PM_MPUGRPSEL_WKUP:   ", 'V', (0x48306CA4)},
	{"PM_IVA2GRPSEL_WKUP:  ", 'V', (0x48306CA8)},
	{"PM_WKST_WKUP:        ", 'V', (0x48306CB0)},
	{"USBHOST"},
	{"CM_FCLKEN_USBHOST:   ", 'V', (0x48005400)},
	{"CM_ICLKEN_USBHOST:   ", 'V', (0x48005410)},
	{"CM_IDLEST_USBHOST:   ", 'V', (0x48005420)},
	{"CM_AUTOIDLE_USBHOST: ", 'V', (0x48005430)},
	{"CM_SLEEPDEP_USBHOST: ", 'V', (0x48005444)},
	{"CM_CLKSTCTRL_USBHOST:", 'V', (0x48005448)},
	{"CM_CLKSTST_USBHOST:  ", 'V', (0x4800544C)},
	{"\n"},
	{"RM_RSTST_USBHOST:    ", 'V', (0x48307458)},
	{"PM_WKEN_USBHOST:     ", 'V', (0x483074A0)},
	{"PM_MPUGRPSEL_USBHOST:", 'V', (0x483074A4)},
	{"PM_IVA2GRPSEL_USBHOST:", 'V', (0x483074A8)},
	{"PM_WKST_USBHOST:     ", 'V', (0x483074B0)},
	{"PM_WKDEP_USBHOST:    ", 'V', (0x483074C8)},
	{"PM_PWSTCTRL_USBHOST: ", 'V', (0x483074E0)},
	{"PM_PWSTST_USBHOST:   ", 'V', (0x483074E4)},
	{"PM_PREPWSTST_USBHOST:", 'V', (0x483074E8)},
	{"EMU"},
	{"CM_CLKSEL1_EMU:      ", 'V', (0x48005140)},
	{"CM_CLKSTCTRL_EMU:    ", 'V', (0x48005148)},
	{"CM_CLKSTST_EMU:      ", 'V', (0x4800514C)},
	{"CM_CLKSEL1_EMU:      ", 'V', (0x48005150)},
	{"CM_CLKSEL3_EMU:      ", 'V', (0x48005154)},
	{"\n"},
	{"RM_RSTST_EMU:        ", 'V', (0x48307158)},
	{"PM_PWSTST_EMU:       ", 'V', (0x483071E4)},
	{"GPIO 1"},
	{"GPIO_REVISION:       ", 'V', (0x48310000)},
	{"GPIO_SYSCONFIG:      ", 'V', (0x48310010)},
	{"GPIO_SYSSTATUS:      ", 'V', (0x48310014)},
	{"GPIO_IRQSTATUS1:     ", 'V', (0x48310018)},
	{"GPIO_IRQENABLE1:     ", 'V', (0x4831001C)},
	{"GPIO_WAKEUPENABLE:   ", 'V', (0x48310020)},
	{"GPIO_IRQSTATUS2:     ", 'V', (0x48310028)},
	{"GPIO_IRQENABLE2:     ", 'V', (0x4831002C)},
	{"GPIO_CTRL:           ", 'V', (0x48310030)},
	{"GPIO_OE:             ", 'V', (0x48310034)},
	{"GPIO_DATAIN:         ", 'V', (0x48310038)},
	{"GPIO_DATAOUT:        ", 'V', (0x4831003C)},
	{"GPIO_LEVELDETECT0:   ", 'V', (0x48310040)},
	{"GPIO_LEVELDETECT1:   ", 'V', (0x48310044)},
	{"GPIO_RISINGDETECT:   ", 'V', (0x48310048)},
	{"GPIO_FALLINGDETECT:  ", 'V', (0x4831004C)},
	{"GPIO_DEBOUNCENABLE:  ", 'V', (0x48310050)},
	{"GPIO_DEBOUNCINGTIME: ", 'V', (0x48310054)},
	{"GPIO_CLEARIRQENABLE1:", 'V', (0x48310060)},
	{"GPIO_SETIRQENABLE1:  ", 'V', (0x48310064)},
	{"GPIO_CLEARIRQENABLE2:", 'V', (0x48310070)},
	{"GPIO_SETIRQENABLE2:  ", 'V', (0x48310074)},
	{"GPIO_CLEARWKUENA:    ", 'V', (0x48310080)},
	{"GPIO_SETWKUENA:      ", 'V', (0x48310084)},
	{"GPIO_CLEARDATAOUT:   ", 'V', (0x48310090)},
	{"GPIO_SETDATAOUT:     ", 'V', (0x48310094)},
	{"GPIO 2"},
	{"GPIO_REVISION:       ", 'V', (0x49050000)},
	{"GPIO_SYSCONFIG:      ", 'V', (0x49050010)},
	{"GPIO_SYSSTATUS:      ", 'V', (0x49050014)},
	{"GPIO_IRQSTATUS1:     ", 'V', (0x49050018)},
	{"GPIO_IRQENABLE1:     ", 'V', (0x4905001C)},
	{"GPIO_WAKEUPENABLE:   ", 'V', (0x49050020)},
	{"GPIO_IRQSTATUS2:     ", 'V', (0x49050028)},
	{"GPIO_IRQENABLE2:     ", 'V', (0x4905002C)},
	{"GPIO_CTRL:           ", 'V', (0x49050030)},
	{"GPIO_OE:             ", 'V', (0x49050034)},
	{"GPIO_DATAIN:         ", 'V', (0x49050038)},
	{"GPIO_DATAOUT:        ", 'V', (0x4905003C)},
	{"GPIO_LEVELDETECT0:   ", 'V', (0x49050040)},
	{"GPIO_LEVELDETECT1:   ", 'V', (0x49050044)},
	{"GPIO_RISINGDETECT:   ", 'V', (0x49050048)},
	{"GPIO_FALLINGDETECT:  ", 'V', (0x4905004C)},
	{"GPIO_DEBOUNCENABLE:  ", 'V', (0x49050050)},
	{"GPIO_DEBOUNCINGTIME: ", 'V', (0x49050054)},
	{"GPIO_CLEARIRQENABLE1:", 'V', (0x49050060)},
	{"GPIO_SETIRQENABLE1:  ", 'V', (0x49050064)},
	{"GPIO_CLEARIRQENABLE2:", 'V', (0x49050070)},
	{"GPIO_SETIRQENABLE2:  ", 'V', (0x49050074)},
	{"GPIO_CLEARWKUENA:    ", 'V', (0x49050080)},
	{"GPIO_SETWKUENA:      ", 'V', (0x49050084)},
	{"GPIO_CLEARDATAOUT:   ", 'V', (0x49050090)},
	{"GPIO_SETDATAOUT:     ", 'V', (0x49050094)},
	{"GPIO 3"},
	{"GPIO_REVISION:       ", 'V', (0x49052000)},
	{"GPIO_SYSCONFIG:      ", 'V', (0x49052010)},
	{"GPIO_SYSSTATUS:      ", 'V', (0x49052014)},
	{"GPIO_IRQSTATUS1:     ", 'V', (0x49052018)},
	{"GPIO_IRQENABLE1:     ", 'V', (0x4905201C)},
	{"GPIO_WAKEUPENABLE:   ", 'V', (0x49052020)},
	{"GPIO_IRQSTATUS2:     ", 'V', (0x49052028)},
	{"GPIO_IRQENABLE2:     ", 'V', (0x4905202C)},
	{"GPIO_CTRL:           ", 'V', (0x49052030)},
	{"GPIO_OE:             ", 'V', (0x49052034)},
	{"GPIO_DATAIN:         ", 'V', (0x49052038)},
	{"GPIO_DATAOUT:        ", 'V', (0x4905203C)},
	{"GPIO_LEVELDETECT0:   ", 'V', (0x49052040)},
	{"GPIO_LEVELDETECT1:   ", 'V', (0x49052044)},
	{"GPIO_RISINGDETECT:   ", 'V', (0x49052048)},
	{"GPIO_FALLINGDETECT:  ", 'V', (0x4905204C)},
	{"GPIO_DEBOUNCENABLE:  ", 'V', (0x49052050)},
	{"GPIO_DEBOUNCINGTIME: ", 'V', (0x49052054)},
	{"GPIO_CLEARIRQENABLE1:", 'V', (0x49052060)},
	{"GPIO_SETIRQENABLE1:  ", 'V', (0x49052064)},
	{"GPIO_CLEARIRQENABLE2:", 'V', (0x49052070)},
	{"GPIO_SETIRQENABLE2:  ", 'V', (0x49052074)},
	{"GPIO_CLEARWKUENA:    ", 'V', (0x49052080)},
	{"GPIO_SETWKUENA:      ", 'V', (0x49052084)},
	{"GPIO_CLEARDATAOUT:   ", 'V', (0x49052090)},
	{"GPIO_SETDATAOUT:     ", 'V', (0x49052094)},
	{"GPIO 4"},
	{"GPIO_REVISION:       ", 'V', (0x49054000)},
	{"GPIO_SYSCONFIG:      ", 'V', (0x49054010)},
	{"GPIO_SYSSTATUS:      ", 'V', (0x49054014)},
	{"GPIO_IRQSTATUS1:     ", 'V', (0x49054018)},
	{"GPIO_IRQENABLE1:     ", 'V', (0x4905401C)},
	{"GPIO_WAKEUPENABLE:   ", 'V', (0x49054020)},
	{"GPIO_IRQSTATUS2:     ", 'V', (0x49054028)},
	{"GPIO_IRQENABLE2:     ", 'V', (0x4905402C)},
	{"GPIO_CTRL:           ", 'V', (0x49054030)},
	{"GPIO_OE:             ", 'V', (0x49054034)},
	{"GPIO_DATAIN:         ", 'V', (0x49054038)},
	{"GPIO_DATAOUT:        ", 'V', (0x4905403C)},
	{"GPIO_LEVELDETECT0:   ", 'V', (0x49054040)},
	{"GPIO_LEVELDETECT1:   ", 'V', (0x49054044)},
	{"GPIO_RISINGDETECT:   ", 'V', (0x49054048)},
	{"GPIO_FALLINGDETECT:  ", 'V', (0x4905404C)},
	{"GPIO_DEBOUNCENABLE:  ", 'V', (0x49054050)},
	{"GPIO_DEBOUNCINGTIME: ", 'V', (0x49054054)},
	{"GPIO_CLEARIRQENABLE1:", 'V', (0x49054060)},
	{"GPIO_SETIRQENABLE1:  ", 'V', (0x49054064)},
	{"GPIO_CLEARIRQENABLE2:", 'V', (0x49054070)},
	{"GPIO_SETIRQENABLE2:  ", 'V', (0x49054074)},
	{"GPIO_CLEARWKUENA:    ", 'V', (0x49054080)},
	{"GPIO_SETWKUENA:      ", 'V', (0x49054084)},
	{"GPIO_CLEARDATAOUT:   ", 'V', (0x49054090)},
	{"GPIO_SETDATAOUT:     ", 'V', (0x49054094)},
	{"GPIO 5"},
	{"GPIO_REVISION:       ", 'V', (0x49056000)},
	{"GPIO_SYSCONFIG:      ", 'V', (0x49056010)},
	{"GPIO_SYSSTATUS:      ", 'V', (0x49056014)},
	{"GPIO_IRQSTATUS1:     ", 'V', (0x49056018)},
	{"GPIO_IRQENABLE1:     ", 'V', (0x4905601C)},
	{"GPIO_WAKEUPENABLE:   ", 'V', (0x49056020)},
	{"GPIO_IRQSTATUS2:     ", 'V', (0x49056028)},
	{"GPIO_IRQENABLE2:     ", 'V', (0x4905602C)},
	{"GPIO_CTRL:           ", 'V', (0x49056030)},
	{"GPIO_OE:             ", 'V', (0x49056034)},
	{"GPIO_DATAIN:         ", 'V', (0x49056038)},
	{"GPIO_DATAOUT:        ", 'V', (0x4905603C)},
	{"GPIO_LEVELDETECT0:   ", 'V', (0x49056040)},
	{"GPIO_LEVELDETECT1:   ", 'V', (0x49056044)},
	{"GPIO_RISINGDETECT:   ", 'V', (0x49056048)},
	{"GPIO_FALLINGDETECT:  ", 'V', (0x4905604C)},
	{"GPIO_DEBOUNCENABLE:  ", 'V', (0x49056050)},
	{"GPIO_DEBOUNCINGTIME: ", 'V', (0x49056054)},
	{"GPIO_CLEARIRQENABLE1:", 'V', (0x49056060)},
	{"GPIO_SETIRQENABLE1:  ", 'V', (0x49056064)},
	{"GPIO_CLEARIRQENABLE2:", 'V', (0x49056070)},
	{"GPIO_SETIRQENABLE2:  ", 'V', (0x49056074)},
	{"GPIO_CLEARWKUENA:    ", 'V', (0x49056080)},
	{"GPIO_SETWKUENA:      ", 'V', (0x49056084)},
	{"GPIO_CLEARDATAOUT:   ", 'V', (0x49056090)},
	{"GPIO_SETDATAOUT:     ", 'V', (0x49056094)},
	{"GPIO 6"},
	{"GPIO_REVISION:       ", 'V', (0x49058000)},
	{"GPIO_SYSCONFIG:      ", 'V', (0x49058010)},
	{"GPIO_SYSSTATUS:      ", 'V', (0x49058014)},
	{"GPIO_IRQSTATUS1:     ", 'V', (0x49058018)},
	{"GPIO_IRQENABLE1:     ", 'V', (0x4905801C)},
	{"GPIO_WAKEUPENABLE:   ", 'V', (0x49058020)},
	{"GPIO_IRQSTATUS2:     ", 'V', (0x49058028)},
	{"GPIO_IRQENABLE2:     ", 'V', (0x4905802C)},
	{"GPIO_CTRL:           ", 'V', (0x49058030)},
	{"GPIO_OE:             ", 'V', (0x49058034)},
	{"GPIO_DATAIN:         ", 'V', (0x49058038)},
	{"GPIO_DATAOUT:        ", 'V', (0x4905803C)},
	{"GPIO_LEVELDETECT0:   ", 'V', (0x49058040)},
	{"GPIO_LEVELDETECT1:   ", 'V', (0x49058044)},
	{"GPIO_RISINGDETECT:   ", 'V', (0x49058048)},
	{"GPIO_FALLINGDETECT:  ", 'V', (0x4905804C)},
	{"GPIO_DEBOUNCENABLE:  ", 'V', (0x49058050)},
	{"GPIO_DEBOUNCINGTIME: ", 'V', (0x49058054)},
	{"GPIO_CLEARIRQENABLE1:", 'V', (0x49058060)},
	{"GPIO_SETIRQENABLE1:  ", 'V', (0x49058064)},
	{"GPIO_CLEARIRQENABLE2:", 'V', (0x49058070)},
	{"GPIO_SETIRQENABLE2:  ", 'V', (0x49058074)},
	{"GPIO_CLEARWKUENA:    ", 'V', (0x49058080)},
	{"GPIO_SETWKUENA:      ", 'V', (0x49058084)},
	{"GPIO_CLEARDATAOUT:   ", 'V', (0x49058090)},
	{"GPIO_SETDATAOUT:     ", 'V', (0x49058094)},
	#endif
};

static void cm_reg_save(struct cm_regdump *rd, int size)
{
	int i;
	for (i = 0; i < size; ++i) {
		if (rd->reg_addr) {
			if (rd->api == 'V') {
				rd->reg_val = omap_readl(rd->reg_addr);
			} else {
			}
		}
		rd++;
	}
}
static void cm_reg_print(struct cm_regdump *rd, int size)
{
	int i;
	for (i = 0; i < size; ++i) {
		if (rd->reg_addr)
			printk(KERN_ERR "\t%s 0x%x\n", rd->reg_name, rd->reg_val);
		else if (rd->reg_name[0] == '\n')
			printk(KERN_ERR "\n");
		else
			printk(KERN_ERR "================ %s ================\n", rd->reg_name);
		++rd;
	}
	printk(KERN_ERR "=====================================\n");
	return;
}
#endif

static int regset_save_on_suspend;
// SAMSUNG egkim [
extern void omap_acknowledge_interrupts(void);
static spinlock_t fleeting_lock = SPIN_LOCK_UNLOCKED;
// ]

/* Scratchpad offsets */
#define OMAP343X_TABLE_ADDRESS_OFFSET	   0xC4
#define OMAP343X_TABLE_VALUE_OFFSET	   0xC0
#define OMAP343X_CONTROL_REG_VALUE_OFFSET  0xC8

struct power_state {
	struct powerdomain *pwrdm;
	u32 next_state;
#ifdef CONFIG_SUSPEND
	u32 saved_state;
#endif
	struct list_head node;
};

static LIST_HEAD(pwrst_list);

static void (*_omap_sram_idle)(u32 *addr, int save_state);

static int (*_omap_save_secure_sram)(u32 *addr);

static struct powerdomain *mpu_pwrdm, *neon_pwrdm;
static struct powerdomain *core_pwrdm, *per_pwrdm;
static struct powerdomain *cam_pwrdm;

unsigned get_last_off_on_transaction_id(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct omap_device *odev = to_omap_device(pdev);
	struct powerdomain *pwrdm;

	/*
	 * REVISIT complete hwmod implementation is not yet done for OMAP3
	 * So we are using basic clockframework for OMAP3
	 * for OMAP4 we continue using hwmod framework
	 */
	if (odev) {
		if (cpu_is_omap44xx())
		pwrdm = omap_device_get_pwrdm(odev);
		else {
			if (!strcmp(pdev->name, "omapdss"))
				pwrdm = pwrdm_lookup("dss_pwrdm");
			else if (!strcmp(pdev->name, "musb_hdrc"))
				pwrdm = pwrdm_lookup("core_pwrdm");
			else
				return 0;
		}

		if (pwrdm)
			return pwrdm->state_counter[0];
	}

	return 0;
}

static void omap3_enable_io_chain(void)
{
	int timeout = 0;

	if (omap_rev() >= OMAP3430_REV_ES3_1) {
		prm_set_mod_reg_bits(OMAP3430_EN_IO_CHAIN_MASK, WKUP_MOD,
				     PM_WKEN);
		/* Do a readback to assure write has been done */
		prm_read_mod_reg(WKUP_MOD, PM_WKEN);

		while (!(prm_read_mod_reg(WKUP_MOD, PM_WKEN) &
			 OMAP3430_ST_IO_CHAIN_MASK)) {
			timeout++;
			if (timeout > 1000) {
				printk(KERN_ERR "Wake up daisy chain "
				       "activation failed.\n");
				return;
			}
			prm_set_mod_reg_bits(OMAP3430_ST_IO_CHAIN_MASK,
					     WKUP_MOD, PM_WKEN);
		}

	}
}

static void omap3_disable_io_chain(void)
{
	if (omap_rev() >= OMAP3430_REV_ES3_1)
		prm_clear_mod_reg_bits(OMAP3430_EN_IO_CHAIN_MASK, WKUP_MOD,
				       PM_WKEN);
}

static void omap3_core_save_context(void)
{
	u32 control_padconf_off;

	/* Save the padconf registers */
	control_padconf_off = omap_ctrl_readl(OMAP343X_CONTROL_PADCONF_OFF);
	control_padconf_off |= START_PADCONF_SAVE;
	omap_ctrl_writel(control_padconf_off, OMAP343X_CONTROL_PADCONF_OFF);
#if 0
	/* wait for the save to complete */
	while (!(omap_ctrl_readl(OMAP343X_CONTROL_GENERAL_PURPOSE_STATUS)
			& PADCONF_SAVE_DONE))
		udelay(1);

	/*
	 * Force write last pad into memory, as this can fail in some
	 * cases according to erratas 1.157, 1.185
	 */
	omap_ctrl_writel(omap_ctrl_readl(OMAP343X_PADCONF_ETK_D14),
		OMAP343X_CONTROL_MEM_WKUP + 0x2a0);
#endif

	/* Save the Interrupt controller context */
	omap_intc_save_context();
	/* Save the GPMC context */
	omap3_gpmc_save_context();
	/* Save the system control module context, padconf already save above*/
	omap3_control_save_context();
}

static void omap3_core_restore_context(void)
{
	/* Restore the control module context, padconf restored by h/w */
	omap3_control_restore_context();
	/* Restore the GPMC context */
	omap3_gpmc_restore_context();
	/* Restore the interrupt controller context */
	omap_intc_restore_context();
}

/*
 * FIXME: This function should be called before entering off-mode after
 * OMAP3 secure services have been accessed. Currently it is only called
 * once during boot sequence, but this works as we are not using secure
 * services.
 */
static void omap3_save_secure_ram_context(u32 target_mpu_state)
{
	u32 ret;

	if (omap_type() != OMAP2_DEVICE_TYPE_GP) {
		/*
		 * MPU next state must be set to POWER_ON temporarily,
		 * otherwise the WFI executed inside the ROM code
		 * will hang the system.
		 */
		pwrdm_set_next_pwrst(mpu_pwrdm, PWRDM_POWER_ON);
		ret = _omap_save_secure_sram((u32 *)
				__pa(omap3_secure_ram_storage));
		pwrdm_set_next_pwrst(mpu_pwrdm, target_mpu_state);
		/* Following is for error tracking, it should not happen */
		if (ret) {
			printk(KERN_ERR "save_secure_sram() returns %08x\n",
				ret);
			while (1)
				;
		}
	}
}

/*
 * PRCM Interrupt Handler Helper Function
 *
 * The purpose of this function is to clear any wake-up events latched
 * in the PRCM PM_WKST_x registers. It is possible that a wake-up event
 * may occur whilst attempting to clear a PM_WKST_x register and thus
 * set another bit in this register. A while loop is used to ensure
 * that any peripheral wake-up events occurring while attempting to
 * clear the PM_WKST_x are detected and cleared.
 */
static int prcm_clear_mod_irqs(s16 module, u8 regs)
{
	u32 wkst, fclk, iclk, clken;
	u16 wkst_off = (regs == 3) ? OMAP3430ES2_PM_WKST3 : PM_WKST1;
	u16 fclk_off = (regs == 3) ? OMAP3430ES2_CM_FCLKEN3 : CM_FCLKEN1;
	u16 iclk_off = (regs == 3) ? CM_ICLKEN3 : CM_ICLKEN1;
	u16 grpsel_off = (regs == 3) ?
		OMAP3430ES2_PM_MPUGRPSEL3 : OMAP3430_PM_MPUGRPSEL;
	int c = 0;

	wkst = prm_read_mod_reg(module, wkst_off);
	wkst &= prm_read_mod_reg(module, grpsel_off);
	if (wkst) {
		iclk = cm_read_mod_reg(module, iclk_off);
		fclk = cm_read_mod_reg(module, fclk_off);
		while (wkst) {
			clken = wkst;
			cm_set_mod_reg_bits(clken, module, iclk_off);
			/*
			 * For USBHOST, we don't know whether HOST1 or
			 * HOST2 woke us up, so enable both f-clocks
			 */
			if (module == OMAP3430ES2_USBHOST_MOD)
				clken |= 1 << OMAP3430ES2_EN_USBHOST2_SHIFT;
			cm_set_mod_reg_bits(clken, module, fclk_off);
			prm_write_mod_reg(wkst, module, wkst_off);
			wkst = prm_read_mod_reg(module, wkst_off);
			c++;
		}
		cm_write_mod_reg(iclk, module, iclk_off);
		cm_write_mod_reg(fclk, module, fclk_off);
	}

	return c;
}

static int _prcm_int_handle_wakeup(void)
{
	int c;

	/* By OMAP3630ES1.x and OMAP3430ES3.1 TRM, S/W must clear
	 * the EN_IO and EN_IO_CHAIN bits of WKEN_WKUP. Those bits
	 * would be set again by S/W in sleep sequences.
	 */
	if (omap_rev() >= OMAP3430_REV_ES3_1)
		prm_clear_mod_reg_bits(OMAP3430_EN_IO_MASK |
				       OMAP3430_EN_IO_CHAIN_MASK,
				       WKUP_MOD, PM_WKEN);

	c = prcm_clear_mod_irqs(WKUP_MOD, 1);
	c += prcm_clear_mod_irqs(CORE_MOD, 1);
	c += prcm_clear_mod_irqs(OMAP3430_PER_MOD, 1);
	if (omap_rev() > OMAP3430_REV_ES1_0) {
		c += prcm_clear_mod_irqs(CORE_MOD, 3);
		c += prcm_clear_mod_irqs(OMAP3430ES2_USBHOST_MOD, 1);
	}

	return c;
}

/*
 * PRCM Interrupt Handler
 *
 * The PRM_IRQSTATUS_MPU register indicates if there are any pending
 * interrupts from the PRCM for the MPU. These bits must be cleared in
 * order to clear the PRCM interrupt. The PRCM interrupt handler is
 * implemented to simply clear the PRM_IRQSTATUS_MPU in order to clear
 * the PRCM interrupt. Please note that bit 0 of the PRM_IRQSTATUS_MPU
 * register indicates that a wake-up event is pending for the MPU and
 * this bit can only be cleared if the all the wake-up events latched
 * in the various PM_WKST_x registers have been cleared. The interrupt
 * handler is implemented using a do-while loop so that if a wake-up
 * event occurred during the processing of the prcm interrupt handler
 * (setting a bit in the corresponding PM_WKST_x register and thus
 * preventing us from clearing bit 0 of the PRM_IRQSTATUS_MPU register)
 * this would be handled.
 */
static irqreturn_t prcm_interrupt_handler (int irq, void *dev_id)
{
	u32 irqenable_mpu, irqstatus_mpu;
	int c = 0;

	irqenable_mpu = prm_read_mod_reg(OCP_MOD,
					 OMAP3_PRM_IRQENABLE_MPU_OFFSET);
	irqstatus_mpu = prm_read_mod_reg(OCP_MOD,
					 OMAP3_PRM_IRQSTATUS_MPU_OFFSET);
	irqstatus_mpu &= irqenable_mpu;

	do {
		if (irqstatus_mpu & (OMAP3430_WKUP_ST_MASK |
				     OMAP3430_IO_ST_MASK)) {
			c = _prcm_int_handle_wakeup();

			/*
			 * Is the MPU PRCM interrupt handler racing with the
			 * IVA2 PRCM interrupt handler ?
			 */
#if 1 //TI HS.Yoon 20100727 //changoh.heo 2010.09.03 confirmed by ti ((Maheshwari, Anubhava) for music play cutted by warn msg
			   //in other cases, mpu can detect peripheral interrupts without waking up by prcm 
		   if(c == 0 && ((omap_readl(0x483069E8) & 0x03) <= 0x02))
				printk("prcm: no wakeup source with PRCM wakeup interrupt %x, %x\n", irqstatus_mpu, omap_readl(0x483069E8) );
#else
			WARN(c == 0, "prcm: WARNING: PRCM indicated MPU wakeup "
			     "but no wakeup sources are marked\n");
#endif
		} else {
			/* XXX we need to expand our PRCM interrupt handler */
			WARN(1, "prcm: WARNING: PRCM interrupt received, but "
			     "no code to handle it (%08x)\n", irqstatus_mpu);
		}

		prm_write_mod_reg(irqstatus_mpu, OCP_MOD,
					OMAP3_PRM_IRQSTATUS_MPU_OFFSET);

		irqstatus_mpu = prm_read_mod_reg(OCP_MOD,
					OMAP3_PRM_IRQSTATUS_MPU_OFFSET);
		irqstatus_mpu &= irqenable_mpu;

	} while (irqstatus_mpu);

	return IRQ_HANDLED;
}

static void restore_control_register(u32 val)
{
	__asm__ __volatile__ ("mcr p15, 0, %0, c1, c0, 0" : : "r" (val));
}

/* Function to restore the table entry that was modified for enabling MMU */
static void restore_table_entry(void)
{
	void __iomem *scratchpad_address;
	u32 previous_value, control_reg_value;
	u32 *address;

	scratchpad_address = OMAP2_L4_IO_ADDRESS(OMAP343X_SCRATCHPAD);

	/* Get address of entry that was modified */
	address = (u32 *)__raw_readl(scratchpad_address +
				     OMAP343X_TABLE_ADDRESS_OFFSET);
	/* Get the previous value which needs to be restored */
	previous_value = __raw_readl(scratchpad_address +
				     OMAP343X_TABLE_VALUE_OFFSET);
	address = __va(address);
	*address = previous_value;
	flush_tlb_all();
	control_reg_value = __raw_readl(scratchpad_address
					+ OMAP343X_CONTROL_REG_VALUE_OFFSET);
	/* This will enable caches and prediction */
	restore_control_register(control_reg_value);
}

void omap_sram_idle(void)
{
	/* Variable to tell what needs to be saved and restored
	 * in omap_sram_idle*/
	/* save_state = 0 => Nothing to save and restored */
	/* save_state = 1 => Only L1 and logic lost */
	/* save_state = 2 => Only L2 lost */
	/* save_state = 3 => L1, L2 and logic lost */
	int save_state = 0;
	int mpu_next_state = PWRDM_POWER_ON;
	int per_next_state = PWRDM_POWER_ON;
	int core_next_state = PWRDM_POWER_ON;
	int core_prev_state, per_prev_state;
	u32 sdrc_pwr = 0;
	int per_state_modified = 0,i=0;

	if (!_omap_sram_idle)
		return;

	pwrdm_clear_all_prev_pwrst(mpu_pwrdm);
	pwrdm_clear_all_prev_pwrst(neon_pwrdm);
	pwrdm_clear_all_prev_pwrst(core_pwrdm);
	pwrdm_clear_all_prev_pwrst(per_pwrdm);

	mpu_next_state = pwrdm_read_next_pwrst(mpu_pwrdm);
	switch (mpu_next_state) {
	case PWRDM_POWER_ON:
	case PWRDM_POWER_RET:
		/* No need to save context */
		save_state = 0;
		break;
	case PWRDM_POWER_OFF:
		save_state = 3;
		break;
	default:
		/* Invalid state */
		printk(KERN_ERR "Invalid mpu state in sram_idle\n");
		return;
	}
	pwrdm_pre_transition();

	/* NEON control */
	if (pwrdm_read_pwrst(neon_pwrdm) == PWRDM_POWER_ON)
		pwrdm_set_next_pwrst(neon_pwrdm, mpu_next_state);

	/* Enable IO-PAD and IO-CHAIN wakeups */
	per_next_state = pwrdm_read_next_pwrst(per_pwrdm);
	core_next_state = pwrdm_read_next_pwrst(core_pwrdm);
#if 0
	if (per_next_state < PWRDM_POWER_ON ||
			core_next_state < PWRDM_POWER_ON) {
		prm_set_mod_reg_bits(OMAP3430_EN_IO_MASK, WKUP_MOD, PM_WKEN);
		omap3_enable_io_chain();
	}
#endif
	/* PER */
	if (per_next_state < PWRDM_POWER_ON) {
	//if (core_next_state == PWRDM_POWER_OFF) omap3_noncore_dpll_disable(dpll5_ck);
		omap_uart_prepare_idle(2);
		if (per_next_state == PWRDM_POWER_OFF) {
			if (core_next_state != PWRDM_POWER_OFF) { // PER_WAKEUP_ERRATA_i582, if (core_next_state == PWRDM_POWER_ON) {
				per_next_state = PWRDM_POWER_RET;
				pwrdm_set_next_pwrst(per_pwrdm, per_next_state);
				per_state_modified = 1;
			} else {
                                omap2_gpio_prepare_for_idle(true);
#ifdef __FIX_GPIO_OUTPUT_GLITCH_IN_OFF_MODE__
				omap3_gpio_save_pad_context();
#endif
                        }
		}
#if 0
		if (per_next_state == PWRDM_POWER_OFF)
			omap2_gpio_prepare_for_idle(true);
		else
			omap2_gpio_prepare_for_idle(false);
#endif
	}

#if 0
	if (pwrdm_read_pwrst(cam_pwrdm) == PWRDM_POWER_ON)
		omap2_clkdm_deny_idle(mpu_pwrdm->pwrdm_clkdms[0]);
#endif

	/* CORE */
	

	if (core_next_state < PWRDM_POWER_ON) {
		if ((core_next_state == PWRDM_POWER_OFF) &&
			(per_next_state > PWRDM_POWER_OFF)) {
			core_next_state = PWRDM_POWER_RET;
			pwrdm_set_next_pwrst(core_pwrdm,
						core_next_state);
		}
		omap_uart_prepare_idle(0);
		omap_uart_prepare_idle(1);
		if (core_next_state == PWRDM_POWER_OFF) {
			prm_set_mod_reg_bits(OMAP3430_AUTO_OFF_MASK,
					     OMAP3430_GR_MOD,
					     OMAP3_PRM_VOLTCTRL_OFFSET);
			omap3_core_save_context();
			omap3_prcm_save_context();
			/* Save MUSB context */
			musb_context_save_restore(save_context);
		} else if (core_next_state == PWRDM_POWER_RET) {
			prm_set_mod_reg_bits(OMAP3430_AUTO_RET_MASK,
						OMAP3430_GR_MOD,
						OMAP3_PRM_VOLTCTRL_OFFSET);
			musb_context_save_restore(disable_clk);
		} else {
			musb_context_save_restore(disable_clk);
		}
		/* Enable IO-PAD and IO-CHAIN wakeups */
		prm_set_mod_reg_bits(OMAP3430_EN_IO_MASK, WKUP_MOD, PM_WKEN);
		omap3_enable_io_chain();
		
	} else {
		omap_uart_prepare_idle(0);
		omap_uart_prepare_idle(1); 
	}

	omap3_intc_prepare_idle();

        if (core_next_state <= PWRDM_POWER_RET) {
		//set_dpll3_volt_freq(0);
		cm_rmw_mod_reg_bits(OMAP3430_AUTO_CORE_DPLL_MASK,
				0x1, PLL_MOD, CM_AUTOIDLE);
		cm_rmw_mod_reg_bits(OMAP3430_AUTO_PERIPH_DPLL_MASK,
				0x1, PLL_MOD, CM_AUTOIDLE);


		cm_write_mod_reg((1 << OMAP3430_AUTO_PERIPH_DPLL_SHIFT) |
				(1 << OMAP3430_AUTO_CORE_DPLL_SHIFT), PLL_MOD,	 CM_AUTOIDLE);
	}
	/*
	* On EMU/HS devices ROM code restores a SRDC value
	* from scratchpad which has automatic self refresh on timeout
	* of AUTO_CNT = 1 enabled. This takes care of errata 1.142.
	* Hence store/restore the SDRC_POWER register here.
	*/
	if (omap_rev() >= OMAP3430_REV_ES3_0 &&
	    omap_type() != OMAP2_DEVICE_TYPE_GP &&
	    core_next_state == PWRDM_POWER_OFF)
		sdrc_pwr = sdrc_read_reg(SDRC_POWER);

	/*
	 * omap3_arm_context is the location where ARM registers
	 * get saved. The restore path then reads from this
	 * location and restores them back.
	 */
	_omap_sram_idle(omap3_arm_context, save_state);

        //cm_reg_save(cm_regs, ARRAY_SIZE(cm_regs));
 
	cpu_init();
      
	/* Restore normal SDRC POWER settings */
	if (omap_rev() >= OMAP3430_REV_ES3_0 &&
	    omap_type() != OMAP2_DEVICE_TYPE_GP &&
	    core_next_state == PWRDM_POWER_OFF)
		sdrc_write_reg(sdrc_pwr, SDRC_POWER);

	/* Restore table entry modified during MMU restoration */
	if (pwrdm_read_prev_pwrst(mpu_pwrdm) == PWRDM_POWER_OFF)
		restore_table_entry();

	/* CORE */
	if (core_next_state < PWRDM_POWER_ON) {
		core_prev_state = pwrdm_read_prev_pwrst(core_pwrdm);
		if (core_prev_state == PWRDM_POWER_OFF) {
			omap3_core_restore_context();
			omap3_prcm_restore_context();
			omap3_sram_restore_context();
			omap2_sms_restore_context();
			/* Restore MUSB context */
			musb_context_save_restore(restore_context);
		} else {
			musb_context_save_restore(enable_clk);
		}

		omap_uart_resume_idle(0);
		omap_uart_resume_idle(1);
		if (core_next_state == PWRDM_POWER_OFF) {
			prm_clear_mod_reg_bits(OMAP3430_AUTO_OFF_MASK,
					OMAP3430_GR_MOD,
					OMAP3_PRM_VOLTCTRL_OFFSET);
		} else if (core_next_state == PWRDM_POWER_RET) {
			prm_clear_mod_reg_bits(OMAP3430_AUTO_RET_MASK,
					OMAP3430_GR_MOD,
					OMAP3_PRM_VOLTCTRL_OFFSET);
		}
	} else {
		omap_uart_resume_idle(0);
		omap_uart_resume_idle(1);	
	}

	omap3_intc_resume_idle();

	if (core_next_state <= PWRDM_POWER_RET) {
		cm_rmw_mod_reg_bits(OMAP3430_AUTO_CORE_DPLL_MASK,
						0x0, PLL_MOD, CM_AUTOIDLE);
		//set_dpll3_volt_freq(1);
	}

	/* PER */
	if (per_next_state < PWRDM_POWER_ON) {
		/* Don't attach mcbsp interrupt */
		prm_clear_mod_reg_bits(OMAP3430_EN_MCBSP2_MASK,OMAP3430_PER_MOD, OMAP3430_PM_MPUGRPSEL);
		if (per_next_state == PWRDM_POWER_OFF) {
			per_prev_state = pwrdm_read_prev_pwrst(per_pwrdm);
			if (per_prev_state == PWRDM_POWER_OFF){
				omap2_gpio_resume_after_idle(true);
#ifdef __FIX_GPIO_OUTPUT_GLITCH_IN_OFF_MODE__
				omap3_gpio_restore_pad_context();
#endif
			} else {
				omap2_gpio_resume_after_idle(true);
				omap3_gpio_restore_pad_context();
			}
		}
		omap_uart_resume_idle(2);

		if (per_state_modified)
			pwrdm_set_next_pwrst(per_pwrdm, PWRDM_POWER_OFF);
	}

	/* Disable IO-PAD and IO-CHAIN wakeup */
	if (core_next_state < PWRDM_POWER_ON) {
		prm_clear_mod_reg_bits(OMAP3430_EN_IO_MASK, WKUP_MOD, PM_WKEN);
		omap3_disable_io_chain();
	}

	pwrdm_post_transition();

	omap2_clkdm_allow_idle(mpu_pwrdm->pwrdm_clkdms[0]);
}

int omap3_can_sleep(void)
{
	if (!sleep_while_idle)
		return 0;
	if (!omap_uart_can_sleep())
		return 0;
	return 1;
}

/* This sets pwrdm state (other than mpu & core. Currently only ON &
 * RET are supported. Function is assuming that clkdm doesn't have
 * hw_sup mode enabled. */
int set_pwrdm_state(struct powerdomain *pwrdm, u32 state)
{
	u32 cur_state;
	int sleep_switch = 0;
	int ret = 0;

	if (pwrdm == NULL || IS_ERR(pwrdm))
		return -EINVAL;

	while (!(pwrdm->pwrsts & (1 << state))) {
		if (state == PWRDM_POWER_OFF)
			return ret;
		state--;
	}

	cur_state = pwrdm_read_next_pwrst(pwrdm);
	if (cur_state == state)
		return ret;

	/*
	 * Bridge pm handles dsp hibernation. just return success
	 * If OFF mode is not enabled, sleep switch is performed for IVA which is not
	 * necessary. This causes conflict between PM and bridge touching IVA reg.
	 * REVISIT: Bridge has to set powerstate based on enable_off_mode state.
	 */
	if (!strcmp(pwrdm->name, "iva2_pwrdm")) {
		if (cur_state == PWRDM_POWER_OFF)	//nirmala
		return 0;
	}

	if (pwrdm_read_pwrst(pwrdm) < PWRDM_POWER_ON) {
		omap2_clkdm_wakeup(pwrdm->pwrdm_clkdms[0]);
		sleep_switch = 1;
		pwrdm_wait_transition(pwrdm);
	}

	ret = pwrdm_set_next_pwrst(pwrdm, state);
	if (ret) {
		printk(KERN_ERR "Unable to set state of powerdomain: %s\n",
		       pwrdm->name);
		goto err;
	}

	if (sleep_switch) {
		omap2_clkdm_allow_idle(pwrdm->pwrdm_clkdms[0]);
		pwrdm_wait_transition(pwrdm);
		pwrdm_state_switch(pwrdm);
	}

err:
	return ret;
}

static void omap3_pm_idle(void)
{
	local_irq_disable();
	local_fiq_disable();

	if (!omap3_can_sleep())
		goto out;

	if (omap_irq_pending() || need_resched())
		goto out;

	omap_sram_idle();

out:
	local_fiq_enable();
	local_irq_enable();
}

#ifdef CONFIG_SUSPEND
static suspend_state_t suspend_state;

static int omap3_pm_prepare(void)
{
	disable_hlt();
	return 0;
}

static int omap3_pm_suspend(void)
{
	struct power_state *pwrst;
	int state, ret = 0;
// SAMSUNG egkim [
	static int i = 1;
	u32 wkup_st = 0;
	u32 ret_val = 0;
	unsigned long fleeting_flag = 0;
// ]

	if (wakeup_timer_seconds || wakeup_timer_milliseconds)
		omap2_pm_wakeup_on_timer(wakeup_timer_seconds,
					 wakeup_timer_milliseconds);
	/*
	 * OMAPS00224186: Sleep current is high after DSP MMU Fault.
	 * GPtimer7 was not reset during the MMU FAULT occurrence 
	 * before the first power cycle.So it needs to be reset.
	 */
	if( (omap_readl(0x482000A0) & 0x800) )// if GPT7 interrupt is pending
	{
		omap_writel( (omap_readl(0x48005030) & (~ 0x100)), 0x48005030);     
		omap_writel( (omap_readl(0x48005010) | 0x100), 0x48005010);        
		omap_writel( (omap_readl(0x48005000) | 0x100), 0x48005000);

		omap_writel( (omap_readl(0x4903C010) | 0x02), 0x4903C010); //reset GPT7        
		while( !(omap_readl(0x4903C014) & 0x1) );

		omap_writel( (omap_readl(0x48005030) | 0x100), 0x48005030);        
		omap_writel( (omap_readl(0x48005010) &(~0x100)), 0x48005010);        
		omap_writel( (omap_readl(0x48005000) &(~0x100)), 0x48005000); 
	}
pm_suspend_reenter: // SAMSUNG egkim

	/* Read current next_pwrsts */
	list_for_each_entry(pwrst, &pwrst_list, node)
		pwrst->saved_state = pwrdm_read_next_pwrst(pwrst->pwrdm);
	/* Set ones wanted by suspend */
	list_for_each_entry(pwrst, &pwrst_list, node) {
		if (set_pwrdm_state(pwrst->pwrdm, pwrst->next_state))
			goto restore;
		if (pwrdm_clear_all_prev_pwrst(pwrst->pwrdm))
			goto restore;
	}

	omap_uart_prepare_suspend();
	omap3_intc_suspend();
	regset_save_on_suspend = 1;
	omap_sram_idle();
	regset_save_on_suspend = 0;
// SAMSUNG egkim [
	wkup_st = prm_read_mod_reg( WKUP_MOD, PM_WKST1 ); //WKUP ST

	if( wkup_st & 0x2 )  // is wakeup reason GPTIMER12?
	{
		prm_write_mod_reg( 0x101CB, WKUP_MOD, PM_WKST1 ); //Clear WKST_WKUP REG

		ret_val = prm_read_mod_reg( OCP_MOD, OMAP3_PRM_IRQSTATUS_MPU_OFFSET );  //MPU IRQ STATUS
		prm_write_mod_reg( ret_val, OCP_MOD, OMAP3_PRM_IRQSTATUS_MPU_OFFSET ); //CLEAR MPU IRQ

		//printk("\nTimer ISR =%x",omap_readw(0x48304018));
		cm_write_mod_reg( cm_read_mod_reg( OCP_MOD,CM_ICLKEN3 ), OCP_MOD, CM_ICLKEN3 );

		spin_lock_irqsave( &fleeting_lock, fleeting_flag );
		ret_val = expire_gptimer12();
		spin_unlock_irqrestore( &fleeting_lock, fleeting_flag );

		omap_acknowledge_interrupts();
		i++;

		if ( !ret_val )
    		goto pm_suspend_reenter;
	} 
	//printk( KERN_ERR"\n  GPtimer12 count =%d\n", i );
	i=0;
// ]
restore:
	/* Restore next_pwrsts */
	list_for_each_entry(pwrst, &pwrst_list, node) {
		state = pwrdm_read_prev_pwrst(pwrst->pwrdm);
		if (state > pwrst->next_state) {
			printk(KERN_INFO "Powerdomain (%s) didn't enter "
			       "target state %d\n",
			       pwrst->pwrdm->name, pwrst->next_state);
			ret = -1;
		}
		set_pwrdm_state(pwrst->pwrdm, pwrst->saved_state);
	}
	if (ret)
		printk(KERN_ERR "Could not enter target state in pm_suspend\n");
	else
		printk(KERN_INFO "Successfully put all powerdomains "
		       "to target state: %d\n", state);
     
	//cm_reg_print(cm_regs, ARRAY_SIZE(cm_regs));

	return ret;
}

static int omap3_pm_enter(suspend_state_t unused)
{
	int ret = 0;

	switch (suspend_state) {
	case PM_SUSPEND_STANDBY:
	case PM_SUSPEND_MEM:
		ret = omap3_pm_suspend();
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static void omap3_pm_finish(void)
{
	enable_hlt();
}

/* Hooks to enable / disable UART interrupts during suspend */
static int omap3_pm_begin(suspend_state_t state)
{
	suspend_state = state;
	omap_uart_enable_irqs(0);
	return 0;
}

static void omap3_pm_end(void)
{
	suspend_state = PM_SUSPEND_ON;
	omap_uart_enable_irqs(1);
	return;
}

static struct platform_suspend_ops omap_pm_ops = {
	.begin		= omap3_pm_begin,
	.end		= omap3_pm_end,
	.prepare	= omap3_pm_prepare,
	.enter		= omap3_pm_enter,
	.finish		= omap3_pm_finish,
	.valid		= suspend_valid_only_mem,
};
#endif /* CONFIG_SUSPEND */


/**
 * omap3_iva_idle(): ensure IVA is in idle so it can be put into
 *                   retention
 *
 * In cases where IVA2 is activated by bootcode, it may prevent
 * full-chip retention or off-mode because it is not idle.  This
 * function forces the IVA2 into idle state so it can go
 * into retention/off and thus allow full-chip retention/off.
 *
 **/
static void __init omap3_iva_idle(void)
{
	/* ensure IVA2 clock is disabled */
	cm_write_mod_reg(0, OMAP3430_IVA2_MOD, CM_FCLKEN);

	/* if no clock activity, nothing else to do */
	if (!(cm_read_mod_reg(OMAP3430_IVA2_MOD, OMAP3430_CM_CLKSTST) &
	      OMAP3430_CLKACTIVITY_IVA2_MASK))
		return;

	/* Reset IVA2 */
	prm_write_mod_reg(OMAP3430_RST1_IVA2_MASK |
			  OMAP3430_RST2_IVA2_MASK |
			  OMAP3430_RST3_IVA2_MASK,
			  OMAP3430_IVA2_MOD, OMAP2_RM_RSTCTRL);

	/* Enable IVA2 clock */
	cm_write_mod_reg(OMAP3430_CM_FCLKEN_IVA2_EN_IVA2_MASK,
			 OMAP3430_IVA2_MOD, CM_FCLKEN);

	/* Set IVA2 boot mode to 'idle' */
	omap_ctrl_writel(OMAP3_IVA2_BOOTMOD_IDLE,
			 OMAP343X_CONTROL_IVA2_BOOTMOD);

	/* Un-reset IVA2 */
	prm_write_mod_reg(0, OMAP3430_IVA2_MOD, OMAP2_RM_RSTCTRL);

	/* Disable IVA2 clock */
	cm_write_mod_reg(0, OMAP3430_IVA2_MOD, CM_FCLKEN);

	/* Reset IVA2 */
	prm_write_mod_reg(OMAP3430_RST1_IVA2_MASK |
			  OMAP3430_RST2_IVA2_MASK |
			  OMAP3430_RST3_IVA2_MASK,
			  OMAP3430_IVA2_MOD, OMAP2_RM_RSTCTRL);
}

static void __init omap3_d2d_idle(void)
{
	u16 mask, padconf;

	/* In a stand alone OMAP3430 where there is not a stacked
	 * modem for the D2D Idle Ack and D2D MStandby must be pulled
	 * high. S CONTROL_PADCONF_SAD2D_IDLEACK and
	 * CONTROL_PADCONF_SAD2D_MSTDBY to have a pull up. */
	mask = (1 << 4) | (1 << 3); /* pull-up, enabled */
	padconf = omap_ctrl_readw(OMAP3_PADCONF_SAD2D_MSTANDBY);
	padconf |= mask;
	omap_ctrl_writew(padconf, OMAP3_PADCONF_SAD2D_MSTANDBY);

	padconf = omap_ctrl_readw(OMAP3_PADCONF_SAD2D_IDLEACK);
	padconf |= mask;
	omap_ctrl_writew(padconf, OMAP3_PADCONF_SAD2D_IDLEACK);

	/* reset modem */
	prm_write_mod_reg(OMAP3430_RM_RSTCTRL_CORE_MODEM_SW_RSTPWRON_MASK |
			  OMAP3430_RM_RSTCTRL_CORE_MODEM_SW_RST_MASK,
			  CORE_MOD, OMAP2_RM_RSTCTRL);
	prm_write_mod_reg(0, CORE_MOD, OMAP2_RM_RSTCTRL);
}

static void __init prcm_setup_regs(void)
{
	/* XXX Reset all wkdeps. This should be done when initializing
	 * powerdomains */
	prm_write_mod_reg(0, OMAP3430_IVA2_MOD, PM_WKDEP);
	prm_write_mod_reg(0, MPU_MOD, PM_WKDEP);
	prm_write_mod_reg(0, OMAP3430_DSS_MOD, PM_WKDEP);
	prm_write_mod_reg(0, OMAP3430_NEON_MOD, PM_WKDEP);
	prm_write_mod_reg(0, OMAP3430_CAM_MOD, PM_WKDEP);
	prm_write_mod_reg(0, OMAP3430_PER_MOD, PM_WKDEP);
	if (omap_rev() > OMAP3430_REV_ES1_0) {

		/*
		* This workaround is needed to prevent SGX and USBHOST from
		* failing to transition to RET/OFF after a warm reset in OFF
		* mode. Workaround sets a sleepdep of each of these domains
		* with MPU, waits for a min 2 sysclk cycles and clears the
		* sleepdep.
		*/
		cm_write_mod_reg(OMAP3430_CM_SLEEPDEP_PER_EN_MPU_MASK,
					OMAP3430ES2_USBHOST_MOD, OMAP3430_CM_SLEEPDEP);
		cm_write_mod_reg(OMAP3430_CM_SLEEPDEP_PER_EN_MPU_MASK,
					OMAP3430ES2_SGX_MOD, OMAP3430_CM_SLEEPDEP);
		
		cm_write_mod_reg(0, OMAP3430ES2_USBHOST_MOD,
					OMAP3430_CM_SLEEPDEP);
		cm_write_mod_reg(0, OMAP3430ES2_SGX_MOD,
					OMAP3430_CM_SLEEPDEP);

        


		prm_write_mod_reg(0, OMAP3430ES2_SGX_MOD, PM_WKDEP);
		prm_write_mod_reg(0, OMAP3430ES2_USBHOST_MOD, PM_WKDEP);
	} else
		prm_write_mod_reg(0, GFX_MOD, PM_WKDEP);

	/*
	 * Enable interface clock autoidle for all modules.
	 * Note that in the long run this should be done by clockfw
	 */
	cm_write_mod_reg(
		OMAP3430_AUTO_MODEM_MASK |
		OMAP3430ES2_AUTO_MMC3_MASK |
		OMAP3430ES2_AUTO_ICR_MASK |
		OMAP3430_AUTO_AES2_MASK |
		OMAP3430_AUTO_SHA12_MASK |
		OMAP3430_AUTO_DES2_MASK |
		OMAP3430_AUTO_MMC2_MASK |
		OMAP3430_AUTO_MMC1_MASK |
		OMAP3430_AUTO_MSPRO_MASK |
		OMAP3430_AUTO_HDQ_MASK |
		OMAP3430_AUTO_MCSPI4_MASK |
		OMAP3430_AUTO_MCSPI3_MASK |
		OMAP3430_AUTO_MCSPI2_MASK |
		OMAP3430_AUTO_MCSPI1_MASK |
		OMAP3430_AUTO_I2C3_MASK |
		OMAP3430_AUTO_I2C2_MASK |
		OMAP3430_AUTO_I2C1_MASK |
		OMAP3430_AUTO_UART2_MASK |
		OMAP3430_AUTO_UART1_MASK |
		OMAP3430_AUTO_GPT11_MASK |
		OMAP3430_AUTO_GPT10_MASK |
		OMAP3430_AUTO_MCBSP5_MASK |
		OMAP3430_AUTO_MCBSP1_MASK |
		OMAP3430ES1_AUTO_FAC_MASK | /* This is es1 only */
		OMAP3430_AUTO_MAILBOXES_MASK |
		OMAP3430_AUTO_OMAPCTRL_MASK |
		OMAP3430ES1_AUTO_FSHOSTUSB_MASK |
		OMAP3430_AUTO_HSOTGUSB_MASK |
		OMAP3430_AUTO_SAD2D_MASK |
		OMAP3430_AUTO_SSI_MASK,
		CORE_MOD, CM_AUTOIDLE1);

	cm_write_mod_reg(
		OMAP3430_AUTO_PKA_MASK |
		OMAP3430_AUTO_AES1_MASK |
		OMAP3430_AUTO_RNG_MASK |
		OMAP3430_AUTO_SHA11_MASK |
		OMAP3430_AUTO_DES1_MASK,
		CORE_MOD, CM_AUTOIDLE2);

	if (omap_rev() > OMAP3430_REV_ES1_0) {
		cm_write_mod_reg(
			OMAP3430_AUTO_MAD2D_MASK |
			OMAP3430ES2_AUTO_USBTLL_MASK,
			CORE_MOD, CM_AUTOIDLE3);
	}

	cm_write_mod_reg(
		OMAP3430_AUTO_WDT2_MASK |
		OMAP3430_AUTO_WDT1_MASK |
		OMAP3430_AUTO_GPIO1_MASK |
		OMAP3430_AUTO_32KSYNC_MASK |
		OMAP3430_AUTO_GPT12_MASK |
		OMAP3430_AUTO_GPT1_MASK,
		WKUP_MOD, CM_AUTOIDLE);

	cm_write_mod_reg(
		OMAP3430_AUTO_DSS_MASK,
		OMAP3430_DSS_MOD,
		CM_AUTOIDLE);

	cm_write_mod_reg(
		OMAP3430_AUTO_CAM_MASK,
		OMAP3430_CAM_MOD,
		CM_AUTOIDLE);

	cm_write_mod_reg(
		OMAP3430_AUTO_GPIO6_MASK |
		OMAP3430_AUTO_GPIO5_MASK |
		OMAP3430_AUTO_GPIO4_MASK |
		OMAP3430_AUTO_GPIO3_MASK |
		OMAP3430_AUTO_GPIO2_MASK |
		OMAP3430_AUTO_WDT3_MASK |
		OMAP3430_AUTO_UART3_MASK |
		OMAP3430_AUTO_GPT9_MASK |
		OMAP3430_AUTO_GPT8_MASK |
		OMAP3430_AUTO_GPT7_MASK |
		OMAP3430_AUTO_GPT6_MASK |
		OMAP3430_AUTO_GPT5_MASK |
		OMAP3430_AUTO_GPT4_MASK |
		OMAP3430_AUTO_GPT3_MASK |
		OMAP3430_AUTO_GPT2_MASK |
		OMAP3430_AUTO_MCBSP4_MASK |
		OMAP3430_AUTO_MCBSP3_MASK |
		OMAP3430_AUTO_MCBSP2_MASK,
		OMAP3430_PER_MOD,
		CM_AUTOIDLE);

	if (omap_rev() > OMAP3430_REV_ES1_0) {
		cm_write_mod_reg(
			OMAP3430ES2_AUTO_USBHOST_MASK,
			OMAP3430ES2_USBHOST_MOD,
			CM_AUTOIDLE);
	}

	omap_ctrl_writel(OMAP3430_AUTOIDLE_MASK, OMAP2_CONTROL_SYSCONFIG);
    //cm_write_mod_reg(0x3, OMAP3430_EMU_MOD, OMAP2_CM_CLKSTCTRL);
	/*
	 * Set all plls to autoidle. This is needed until autoidle is
	 * enabled by clockfw
	 */
	cm_write_mod_reg(1 << OMAP3430_AUTO_IVA2_DPLL_SHIFT,
			 OMAP3430_IVA2_MOD, CM_AUTOIDLE2);
	cm_write_mod_reg(1 << OMAP3430_AUTO_MPU_DPLL_SHIFT,
			 MPU_MOD,
			 CM_AUTOIDLE2);
	cm_write_mod_reg((1 << OMAP3430_AUTO_PERIPH_DPLL_SHIFT) |
			 (1 << OMAP3430_AUTO_CORE_DPLL_SHIFT),
			 PLL_MOD,
			 CM_AUTOIDLE);
	cm_write_mod_reg(1 << OMAP3430ES2_AUTO_PERIPH2_DPLL_SHIFT,
			 PLL_MOD,
			 CM_AUTOIDLE2);

	/*
	 * Enable control of expternal oscillator through
	 * sys_clkreq. In the long run clock framework should
	 * take care of this.
	 */
	prm_rmw_mod_reg_bits(OMAP_AUTOEXTCLKMODE_MASK,
			     1 << OMAP_AUTOEXTCLKMODE_SHIFT,
			     OMAP3430_GR_MOD,
			     OMAP3_PRM_CLKSRC_CTRL_OFFSET);

	/* setup wakup source:
	 *  By OMAP3630ES1.x and OMAP3430ES3.1 TRM, S/W must take care
	 *  the EN_IO and EN_IO_CHAIN bits in sleep-wakeup sequences.
	 */
	prm_write_mod_reg(OMAP3430_EN_GPIO1_MASK | OMAP3430_EN_GPT1_MASK |
			  OMAP3430_EN_GPT12_MASK, WKUP_MOD, PM_WKEN);
	if (omap_rev() < OMAP3430_REV_ES3_1)
		prm_set_mod_reg_bits(OMAP3430_EN_IO_MASK, WKUP_MOD, PM_WKEN);
	/* No need to write EN_IO, that is always enabled */
	prm_write_mod_reg(OMAP3430_GRPSEL_GPIO1_MASK |
			  OMAP3430_GRPSEL_GPT1_MASK |
			  OMAP3430_GRPSEL_GPT12_MASK,
			  WKUP_MOD, OMAP3430_PM_MPUGRPSEL);
	/* For some reason IO doesn't generate wakeup event even if
	 * it is selected to mpu wakeup goup */
	prm_write_mod_reg(OMAP3430_IO_EN_MASK | OMAP3430_WKUP_EN_MASK,
			  OCP_MOD, OMAP3_PRM_IRQENABLE_MPU_OFFSET);

	/* Enable PM_WKEN to support DSS LPR */
	prm_write_mod_reg(OMAP3430_PM_WKEN_DSS_EN_DSS_MASK,
				OMAP3430_DSS_MOD, PM_WKEN);

	/* Enable wakeups in PER */
	
	prm_write_mod_reg(OMAP3430_EN_GPIO2_MASK | OMAP3430_EN_GPIO3_MASK |
			  OMAP3430_EN_GPIO4_MASK | OMAP3430_EN_GPIO5_MASK |
			  OMAP3430_EN_GPIO6_MASK | OMAP3430_EN_UART3_MASK |
			  OMAP3430_EN_MCBSP2_MASK, OMAP3430_PER_MOD, PM_WKEN);

/* and allow them to wake up MPU */
	
	prm_write_mod_reg(OMAP3430_GRPSEL_GPIO2_MASK |
			  OMAP3430_GRPSEL_GPIO3_MASK |
			  OMAP3430_GRPSEL_GPIO4_MASK |
			  OMAP3430_GRPSEL_GPIO5_MASK |
			  OMAP3430_GRPSEL_GPIO6_MASK | 
			  OMAP3430_GRPSEL_UART3_MASK ,
			  OMAP3430_PER_MOD, OMAP3430_PM_MPUGRPSEL);

	/* Don't attach mcbsp interrupt */
	prm_clear_mod_reg_bits(OMAP3430_EN_MCBSP2_MASK,
			  OMAP3430_PER_MOD, OMAP3430_PM_MPUGRPSEL);

	/* Don't attach IVA interrupts */
	prm_write_mod_reg(0, WKUP_MOD, OMAP3430_PM_IVAGRPSEL);
	prm_write_mod_reg(0, CORE_MOD, OMAP3430_PM_IVAGRPSEL1);
	prm_write_mod_reg(0, CORE_MOD, OMAP3430ES2_PM_IVAGRPSEL3);
	prm_write_mod_reg(0, OMAP3430_PER_MOD, OMAP3430_PM_IVAGRPSEL);

	/* Clear any pending 'reset' flags */
	prm_write_mod_reg(0xffffffff, MPU_MOD, OMAP2_RM_RSTST);
	prm_write_mod_reg(0xffffffff, CORE_MOD, OMAP2_RM_RSTST);
	prm_write_mod_reg(0xffffffff, OMAP3430_PER_MOD, OMAP2_RM_RSTST);
	prm_write_mod_reg(0xffffffff, OMAP3430_EMU_MOD, OMAP2_RM_RSTST);
	prm_write_mod_reg(0xffffffff, OMAP3430_NEON_MOD, OMAP2_RM_RSTST);
	prm_write_mod_reg(0xffffffff, OMAP3430_DSS_MOD, OMAP2_RM_RSTST);
	prm_write_mod_reg(0xffffffff, OMAP3430ES2_USBHOST_MOD, OMAP2_RM_RSTST);

	/* Clear any pending PRCM interrupts */
	prm_write_mod_reg(0, OCP_MOD, OMAP3_PRM_IRQSTATUS_MPU_OFFSET);

	omap3_iva_idle();
	omap3_d2d_idle();
}

void omap3_pm_off_mode_enable(int enable)
{
	struct power_state *pwrst;
	u32 state;

	if (enable)
		state = PWRDM_POWER_OFF;
	else
		state = PWRDM_POWER_RET;

#ifdef CONFIG_CPU_IDLE
	omap3_cpuidle_update_states();
#endif

	list_for_each_entry(pwrst, &pwrst_list, node) {
		pwrst->next_state = state;
		set_pwrdm_state(pwrst->pwrdm, state);
	}
}

int omap3_pm_get_suspend_state(struct powerdomain *pwrdm)
{
	struct power_state *pwrst;

	list_for_each_entry(pwrst, &pwrst_list, node) {
		if (pwrst->pwrdm == pwrdm)
			return pwrst->next_state;
	}
	return -EINVAL;
}

int omap3_pm_set_suspend_state(struct powerdomain *pwrdm, int state)
{
	struct power_state *pwrst;

	list_for_each_entry(pwrst, &pwrst_list, node) {
		if (pwrst->pwrdm == pwrdm) {
			pwrst->next_state = state;
			return 0;
		}
	}
	return -EINVAL;
}

static int __init pwrdms_setup(struct powerdomain *pwrdm, void *unused)
{
	struct power_state *pwrst;

	if (!pwrdm->pwrsts)
		return 0;

	pwrst = kmalloc(sizeof(struct power_state), GFP_ATOMIC);
	if (!pwrst)
		return -ENOMEM;
	pwrst->pwrdm = pwrdm;
	pwrst->next_state = PWRDM_POWER_RET;
	list_add(&pwrst->node, &pwrst_list);

	if (pwrdm_has_hdwr_sar(pwrdm))
		pwrdm_enable_hdwr_sar(pwrdm);

	return set_pwrdm_state(pwrst->pwrdm, pwrst->next_state);
}

/*
 * Enable hw supervised mode for all clockdomains if it's
 * supported. Initiate sleep transition for other clockdomains, if
 * they are not used
 */
static int __init clkdms_setup(struct clockdomain *clkdm, void *unused)
{
	clkdm_clear_all_wkdeps(clkdm);
	clkdm_clear_all_sleepdeps(clkdm);

	if (clkdm->flags & CLKDM_CAN_ENABLE_AUTO)
		omap2_clkdm_allow_idle(clkdm);
	else if (clkdm->flags & CLKDM_CAN_FORCE_SLEEP &&
		 atomic_read(&clkdm->usecount) == 0)
		omap2_clkdm_sleep(clkdm);
	return 0;
}

void omap_push_sram_idle(void)
{
	_omap_sram_idle = omap_sram_push(omap34xx_cpu_suspend,
					omap34xx_cpu_suspend_sz);
	if (omap_type() != OMAP2_DEVICE_TYPE_GP)
		_omap_save_secure_sram = omap_sram_push(save_secure_ram_context,
				save_secure_ram_context_sz);
}

static int __init omap3_pm_init(void)
{
	struct power_state *pwrst, *tmp;
	struct clockdomain *neon_clkdm, *per_clkdm, *mpu_clkdm, *core_clkdm, *wkup_clkdm;
	int ret;

	if (!cpu_is_omap34xx())
		return -ENODEV;

	printk(KERN_ERR "Power Management for TI OMAP3.\n");

	/* XXX prcm_setup_regs needs to be before enabling hw
	 * supervised mode for powerdomains */
	prcm_setup_regs();

	ret = request_irq(INT_34XX_PRCM_MPU_IRQ,
			  (irq_handler_t)prcm_interrupt_handler,
			  IRQF_DISABLED, "prcm", NULL);
	if (ret) {
		printk(KERN_ERR "request_irq failed to register for 0x%x\n",
		       INT_34XX_PRCM_MPU_IRQ);
		goto err1;
	}

	ret = pwrdm_for_each(pwrdms_setup, NULL);
	if (ret) {
		printk(KERN_ERR "Failed to setup powerdomains\n");
		goto err2;
	}

	(void) clkdm_for_each(clkdms_setup, NULL);

	mpu_pwrdm = pwrdm_lookup("mpu_pwrdm");
	if (mpu_pwrdm == NULL) {
		printk(KERN_ERR "Failed to get mpu_pwrdm\n");
		goto err2;
	}

	neon_pwrdm = pwrdm_lookup("neon_pwrdm");
	per_pwrdm = pwrdm_lookup("per_pwrdm");
	core_pwrdm = pwrdm_lookup("core_pwrdm");
	cam_pwrdm = pwrdm_lookup("cam_pwrdm");

	neon_clkdm = clkdm_lookup("neon_clkdm");
	mpu_clkdm = clkdm_lookup("mpu_clkdm");
	per_clkdm = clkdm_lookup("per_clkdm");
	core_clkdm = clkdm_lookup("core_clkdm");
	wkup_clkdm = clkdm_lookup("wkup_clkdm");

	omap_push_sram_idle();
#ifdef CONFIG_SUSPEND
	suspend_set_ops(&omap_pm_ops);
#endif /* CONFIG_SUSPEND */

	pm_idle = omap3_pm_idle;
	omap3_idle_init();

	clkdm_add_wkdep(neon_clkdm, mpu_clkdm);
	/*
	 * REVISIT: This wkdep is only necessary when GPIO2-6 are enabled for
	 * IO-pad wakeup.  Otherwise it will unnecessarily waste power
	 * waking up PER with every CORE wakeup - see
	 * http://marc.info/?l=linux-omap&m=121852150710062&w=2
	*/
	clkdm_add_wkdep(per_clkdm, core_clkdm);
	/*
	 * A part of the fix for errata 1.158.
	 * GPIO pad spurious transition (glitch/spike) upon wakeup
	 * from SYSTEM OFF mode. The remaining fix is in:
	 * omap3_gpio_save_context, omap3_gpio_restore_context.
	 */
	if (omap_rev() <= OMAP3430_REV_ES3_1)
		clkdm_add_wkdep(per_clkdm, wkup_clkdm);

	if (omap_type() != OMAP2_DEVICE_TYPE_GP) {
		omap3_secure_ram_storage =
			kmalloc(0x803F, GFP_KERNEL);
		if (!omap3_secure_ram_storage)
			printk(KERN_ERR "Memory allocation failed when"
					"allocating for secure sram context\n");

		local_irq_disable();
		local_fiq_disable();

		omap2_dma_context_save();
		omap3_save_secure_ram_context(PWRDM_POWER_ON);
		omap2_dma_context_restore();

		local_irq_enable();
		local_fiq_enable();
	}

	omap3_save_scratchpad_contents();
err1:
	return ret;
err2:
	free_irq(INT_34XX_PRCM_MPU_IRQ, NULL);
	list_for_each_entry_safe(pwrst, tmp, &pwrst_list, node) {
		list_del(&pwrst->node);
		kfree(pwrst);
	}
	return ret;
}

static int __init omap3_pm_early_init(void)
{
	prm_clear_mod_reg_bits(OMAP3430_OFFMODE_POL_MASK, OMAP3430_GR_MOD,
				OMAP3_PRM_POLCTRL_OFFSET);
	return 0;
}

arch_initcall(omap3_pm_early_init);
late_initcall(omap3_pm_init);
