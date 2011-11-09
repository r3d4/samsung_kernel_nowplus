/**
 * @file    onenand_interface.c
 * @brief   This file is onenand interface file
 *
 *---------------------------------------------------------------------------*
 *                                                                           *
 *          COPYRIGHT 2003-2007 SAMSUNG ELECTRONICS CO., LTD.                *
 *                          ALL RIGHTS RESERVED                              *
 *                                                                           *
 *   Permission is hereby granted to licensees of Samsung Electronics        *
 *   Co., Ltd. products to use or abstract this computer program only in     *
 *   accordance with the terms of the NAND FLASH MEMORY SOFTWARE LICENSE     *
 *   AGREEMENT for the sole purpose of implementing a product based on       *
 *   Samsung Electronics Co., Ltd. products. No other rights to reproduce,   *
 *   use, or disseminate this computer program, whether in part or in        *
 *   whole, are granted.                                                     *
 *                                                                           *
 *   Samsung Electronics Co., Ltd. makes no representation or warranties     *
 *   with respect to the performance of this computer program, and           *
 *   specifically disclaims any responsibility for any damages,              *
 *   special or consequential, connected with the use of this program.       *
 *                                                                           *
 *---------------------------------------------------------------------------*
 *
 */

#include <linux/kernel.h>

#include <tbml_common.h>
#include <os_adapt.h>
#include <onenand_interface.h>
#include <onenand_lld.h>
#include "../TinyBML/onenand_reg.h"
#include "../TinyBML/onenand.h"

void inter_low_ftable(void *pstFunc)
{
	lld_ftable *pstLFT;

	pstLFT = (lld_ftable*)pstFunc;
	
    pstLFT[0].init          = onld_init;
    pstLFT[0].open          = onld_open;
    pstLFT[0].close         = onld_close;
    pstLFT[0].read          = onld_read;
    pstLFT[0].mread         = onld_mread;
    pstLFT[0].dev_info      = onld_getdevinfo;
    pstLFT[0].check_bad     = onld_chkinitbadblk;
    pstLFT[0].flush         = onld_flushop;
    pstLFT[0].set_rw        = onld_setrwarea;
    pstLFT[0].ioctl         = onld_ioctl;
    pstLFT[0].platform_info = onld_getplatforminfo;

#ifdef CONFIG_XSR_DUAL_DEVICE
    pstLFT[1].init          = onld_init;
    pstLFT[1].open          = onld_open;
    pstLFT[1].close         = onld_close;
    pstLFT[1].read          = onld_read;
    pstLFT[1].mread         = onld_mread;
    pstLFT[1].dev_info      = onld_getdevinfo;
    pstLFT[1].check_bad     = onld_chkinitbadblk;
    pstLFT[1].flush         = onld_flushop;
    pstLFT[1].set_rw        = onld_setrwarea;
    pstLFT[1].ioctl         = onld_ioctl;
    pstLFT[1].platform_info = onld_getplatforminfo;
#endif

}    

static vol_param gstParm[XSR_MAX_VOL];

void* inter_getparam(void)
{
	gstParm[0].nBaseAddr[0]	= CONFIG_TINY_FLASH_PHYS_ADDR; 
	gstParm[0].nBaseAddr[1]	= NOT_MAPPED;
	gstParm[0].nBaseAddr[2]	= NOT_MAPPED;
	gstParm[0].nBaseAddr[3]	= NOT_MAPPED;

	gstParm[0].nEccPol	= HW_ECC;
	gstParm[0].nLSchemePol	= HW_LOCK_SCHEME;
	gstParm[0].bByteAlign	= TRUE32;
	gstParm[0].nDevsInVol	= 1;
	gstParm[0].pExInfo	= NULL;

#ifdef CONFIG_XSR_DUAL_DEVICE
	gstParm[1].nBaseAddr[0]	= CONFIG_XSR_FLASH_PHYS_ADDR2;
	gstParm[1].nBaseAddr[1]	= NOT_MAPPED;
	gstParm[1].nBaseAddr[2]	= NOT_MAPPED;
	gstParm[1].nBaseAddr[3]	= NOT_MAPPED;

	gstParm[1].nEccPol	= HW_ECC;
	gstParm[1].nLSchemePol	= HW_LOCK_SCHEME;
	gstParm[1].bByteAlign	= TRUE32;
	gstParm[1].nDevsInVol	= 1;
	gstParm[1].pExInfo	= NULL;
#else
	gstParm[1].nBaseAddr[0]	= NOT_MAPPED;
	gstParm[1].nBaseAddr[1]	= NOT_MAPPED;
	gstParm[1].nBaseAddr[2]	= NOT_MAPPED;
	gstParm[1].nBaseAddr[3]	= NOT_MAPPED;

	gstParm[1].nEccPol	= HW_ECC;
	gstParm[1].nLSchemePol	= HW_LOCK_SCHEME;
	gstParm[1].bByteAlign	= TRUE32;
	gstParm[1].nDevsInVol	= 0;
	gstParm[1].pExInfo	= NULL;
#endif

	return (void *) gstParm;
}


