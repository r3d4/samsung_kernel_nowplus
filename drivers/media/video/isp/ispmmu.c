/*
 * omap iommu wrapper for TI's OMAP3430 Camera ISP
 *
 * Copyright (C) 2008--2009 Nokia.
 *
 * Contributors:
 *	Hiroshi Doyu <hiroshi.doyu@nokia.com>
 *	Sakari Ailus <sakari.ailus@nokia.com>
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <linux/module.h>

#include "ispmmu.h"
#include "isp.h"

//#include <mach/iommu.h>
//#include <mach/iovmm.h>
#include <plat/iommu.h>
#include <plat/iovmm.h>

#define IOMMU_FLAG (IOVMF_ENDIAN_LITTLE | IOVMF_ELSZ_8)

static struct iommu *isp_iommu;

dma_addr_t ispmmu_vmalloc(size_t bytes)
{
	return (dma_addr_t)iommu_vmalloc(isp_iommu, 0, bytes, IOMMU_FLAG);
}

void ispmmu_vfree(const dma_addr_t da)
{
	iommu_vfree(isp_iommu, (u32)da);
}

dma_addr_t ispmmu_kmap(u32 pa, int size)
{
	void *da;

	da = (void *)iommu_kmap(isp_iommu, 0, pa, size, IOMMU_FLAG);
	if (IS_ERR(da))
		return PTR_ERR(da);

	return (dma_addr_t)da;
}

void ispmmu_kunmap(dma_addr_t da)
{
	iommu_kunmap(isp_iommu, (u32)da);
}

void ispmmu_save_context(void)
{
	if (isp_iommu)
		iommu_save_ctx(isp_iommu);
}

void ispmmu_restore_context(void)
{
	if (isp_iommu)
		iommu_restore_ctx(isp_iommu);
}

int __init ispmmu_init(void)
{
	int err = 0;

	isp_get();
	isp_iommu = iommu_get("isp");
	if (IS_ERR(isp_iommu)) {
		err = PTR_ERR(isp_iommu);
		isp_iommu = NULL;
	}
	isp_put();

	return err;
}

void ispmmu_cleanup(void)
{
	isp_get();
	if (isp_iommu)
		iommu_put(isp_iommu);
	isp_put();
	isp_iommu = NULL;
}
