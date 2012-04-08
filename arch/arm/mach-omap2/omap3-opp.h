#ifndef __OMAP3_OPP_H_
#define __OMAP3_OPP_H_

#include <plat/omap-pm.h>

/* MPU speeds */
#define S1000M	1000000000
#define S800M	800000000
#define S600M   600000000
#define S550M   550000000
#define S500M   500000000
#define S300M	300000000
#define S250M   250000000
#define S125M   125000000

/* DSP speeds */
#define S800M	800000000
#define S660M	660000000
#define S520M	520000000
#define S430M   430000000
#define S400M   400000000
#define S360M   360000000
#define S260M	260000000
#define S180M   180000000
#define S90M    90000000
#define S65M    65000000

/* L3 speeds */
#define S200M	200000000
#define S166M   166000000
#define S100M	100000000
#define S83M    83000000

#ifdef CONFIG_ARCH_OMAP3430

// OMAP3430 MPU rate table
// VDD1 nominal 0.950V .. 1.350V
// VOUT = VSEL*12.5 mV + 600 mV
static struct omap_opp omap3_mpu_rate_table[] = {
	{0, 0, 0},
	/*OPP1*/
	{S125M, VDD1_OPP1, 0x1E},	// 0.975V
	/*OPP2*/
	{S250M, VDD1_OPP2, 0x26},	// 1.075V
	/*OPP3*/
	{S500M, VDD1_OPP3, 0x30},	// 1.200V
	/*OPP4*/
	{S550M, VDD1_OPP4, 0x36},	// 1.275V
	/*OPP5*/
#ifdef CONFIG_MACH_NOWPLUS
	//{S720M, VDD1_OPP5, 0x40},	// 1.400V
	{S800M, VDD1_OPP5, 0x44},	// 1.450V
#else
	{S600M, VDD1_OPP5, 0x3C},	// 1.350V
#endif
};

// OMAP3430 L3 rate table
// VDD2 nominal 0.950V .. 1.150V
static struct omap_opp omap3_l3_rate_table[] = {
	{0, 0, 0},
	/*OPP1*/
	{0,     VDD2_OPP1, 0x1E},   // 0.975V
	/*OPP2*/
//    {S83M, VDD2_OPP2, 0x2c},	// 1.150V
	{S166M, VDD2_OPP2, 0x2c},	// 1.150V
	/*OPP3*/
	{S166M, VDD2_OPP3, 0x2c},	// 1.150V
};

// OMAP3430 IVA rate table
// VDD1 nominal 0.950V .. 1.350V
static struct omap_opp omap3_dsp_rate_table[] = {
	{0, 0, 0},
	/*OPP1*/
	{S90M,  VDD1_OPP1, 0x1E},   // 0.975V
	/*OPP2*/                    
	{S180M, VDD1_OPP2, 0x26},   // 1.075V
	/*OPP3*/                    
	{S360M, VDD1_OPP3, 0x30},   // 1.200V
	/*OPP4*/                    
	{S400M, VDD1_OPP4, 0x36},   // 1.275V
	/*OPP5*/                    
	{S430M, VDD1_OPP5, 0x3C},   // 1.350V
};

#else

static struct omap_opp omap3630_mpu_rate_table[] = {
	{0, 0, 0},
	/*OPP1 (OPP50) - 0.93mV*/
	{S300M, VDD1_OPP1, 0x1b},
	/*OPP2 (OPP100) - 1.1V*/
	{S600M, VDD1_OPP2, 0x28},
	/*OPP3 (OPP130) - 1.26V*/
	{S800M, VDD1_OPP3, 0x35},
};

static struct omap_opp omap3630_l3_rate_table[] = {
	{0, 0, 0},
	/*OPP1 (OPP50) - 0.93V*/
	{S100M, VDD2_OPP1, 0x1f},
	/*OPP2 (OPP100) - 1.1375V*/
	{S200M, VDD2_OPP2, 0x2f},
};

static struct omap_opp omap3630_dsp_rate_table[] = {
	{0, 0, 0},
	/*OPP1 (OPP50) - 0.93V*/
	{S260M, VDD1_OPP1, 0x1b},
	/*OPP2 (OPP100) - 1.1V*/
	{S520M, VDD1_OPP2, 0x28},
	/*OPP3 (OPP130) - 1.26V*/
	{S660M, VDD1_OPP3, 0x35},
};

#endif


#endif
