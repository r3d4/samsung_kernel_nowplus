/*
 * SDRC register values for the NOWPLUS
 *
 * Copyright (C) 2008 Texas Instruments, Inc.
 * Copyright (C) 2008-2009 Nokia Corporation
 *
 * Paul Walmsley
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef ARCH_ARM_MACH_OMAP2_SDRAM_NOWPLUS
#define ARCH_ARM_MACH_OMAP2_SDRAM_NOWPLUS

#include <plat/sdrc.h>

static struct omap_sdrc_params nowplus_sdrc_params[] = {
	[0] = {
		.rate	     = 166000000,
		.actim_ctrla = 0x629db485,
		.actim_ctrlb = 0x00022014,
		.rfr_ctrl    = 0x0004de01,
		.mr	     = 0x00000032,
	},
	[1] = {
		.rate	     = 165941176,
		.actim_ctrla = 0x629db485,
		.actim_ctrlb = 0x00022014,
		.rfr_ctrl    = 0x0004de01,
		.mr	     = 0x00000032,
	},
	[2] = {
		.rate	     = 83000000,
		.actim_ctrla = 0x39512243,
		.actim_ctrlb = 0x00022010,
		.rfr_ctrl    = 0x00025401,
		.mr	     = 0x00000032,
	},
	[3] = {
		.rate	     = 82970588,
		.actim_ctrla = 0x39512243,
		.actim_ctrlb = 0x00022010,
		.rfr_ctrl    = 0x00025401,
		.mr	     = 0x00000032,
	},
	[4] = {
		.rate	     = 0
	},
};

#endif
