/**
 * @file    LinuxOAM.c
 * @brief   This file is Linux Adaptation file 
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
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/highmem.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>

#if defined(CONFIG_ARM)
#include <asm/io.h>
#include <asm/sizes.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)
#include <mach/hardware.h>
#else
#include <asm/hardware.h>
#endif
#endif /* CONFIG_ARM */

#include <tbml_common.h>
#include <os_adapt.h>
#include <onenand_interface.h>

#include "tiny_base.h"
/*****************************************************************************/
/* Global variables definitions                                              */
/*****************************************************************************/

/*****************************************************************************/
/* Local #defines                                                            */
/*****************************************************************************/
#define		LOCAL_MEM_SIZE		((100 * 1024) / sizeof(UINT32))
#if !defined(CONFIG_ARM)
#define		SZ_128K			0x20000
#endif

/*****************************************************************************/
/* Local typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local constant definitions                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Static variables definitions                                              */
/*****************************************************************************/

/*****************************************************************************/
/* Static function prototypes                                                */
/*****************************************************************************/

/*****************************************************************************/
/************* MUTUX Initialization for QAM_Create****************************/
/*****************************************************************************/
extern struct semaphore xsr_sem[XSR_MAX_VOLUME];
extern int semaphore_use[XSR_MAX_VOLUME];

static inline int free_vmem(void *addr)
{
	if (((u32)addr >= VMALLOC_START) && ((u32)addr <= VMALLOC_END)) {
		vfree(addr);
		return 0;
	}

	return -1;
}

static inline void *alloc_vmem(unsigned long size)
{
	return vmalloc(size);
}

VOID *
AD_Malloc(UINT32 nSize)
{
	void * ret;

	if (nSize > SZ_128K)
		ret = alloc_vmem(nSize);
	else
		ret = kmalloc(nSize, GFP_KERNEL);

	return ret;
}

VOID 
AD_Free(VOID  *pMem)
{
	if (free_vmem(pMem))
		kfree(pMem);

	return;
}

VOID
AD_Memcpy(VOID *pDst, VOID *pSrc, UINT32 nLen)
{
	memcpy(pDst, pSrc, nLen);		
}


VOID
AD_Memset(VOID *pDst, UINT8 nV, UINT32 nLen)
{
	memset(pDst, nV, nLen);
}

INT32
AD_Memcmp(VOID  *pSrc, VOID  *pDst, UINT32 nLen)
{
	return memcmp(pSrc, pDst, nLen);
}

BOOL32
AD_CreateSM(SM32 *pHandle)
{
	int sem_count = 0;
#ifdef CONFIG_RFS_XSR
	if (semaphore_use[*pHandle] == 1)
		return TRUE32;
#endif
	for (sem_count = 0; sem_count < XSR_MAX_VOLUME; sem_count++) {
		/* find semaphore */
		if (semaphore_use[sem_count] == 0) {
			semaphore_use[sem_count] = 1;
			break;
		}
	}

	/* can't find semaphore */
	if (sem_count == XSR_MAX_VOLUME) {
		return FALSE32;
	}

	*pHandle = sem_count;
	sema_init(&xsr_sem[sem_count], 1);

	return TRUE32;

}

BOOL32
AD_DestroySM(SM32 nHandle)
{
	if (semaphore_use[nHandle] != 1)
		return FALSE32;
	semaphore_use[nHandle] = 0;
	return TRUE32;

}

BOOL32
AD_AcquireSM(SM32 nHandle)
{
	if (semaphore_use[nHandle] != 1)
		return FALSE32;
	/* acquire the lock for file system critical section */
	down(&xsr_sem[nHandle]);
	return TRUE32;
}

BOOL32
AD_ReleaseSM(SM32 nHandle)
{
	if (semaphore_use[nHandle] != 1)
		return FALSE32;
	up(&xsr_sem[nHandle]);
	return(TRUE32);
}

static UINT8 gaStr_tiny[1024 * 8];

VOID
AD_Debug(VOID  *pFmt, ...)
{
	va_list ap;
	va_start(ap, pFmt);
	vsnprintf((char *) gaStr_tiny, (size_t) sizeof(gaStr_tiny), (char *) pFmt, ap);
	printk(gaStr_tiny);
	va_end(ap);

}

VOID
AD_Idle(UINT32 nMode)
{
#ifdef CONFIG_RFS_XSR_INT
    if (nMode == INT_MODE) {
        /* Do sleep if current mode is interrput mode */
        PAM_Idle(nMode);
        return;
    }
#endif
#ifdef XSR_FOR_2_6_14
    touch_softlockup_watchdog();
#endif
    cond_resched();
}

UINT32
AD_Pa2Va(UINT32 nPAddr)
{
	unsigned long ioaddr;
	ioaddr = (unsigned long) ioremap(nPAddr, SZ_128K);
	return ioaddr;
}

VOID
AD_ResetTimer(VOID)
{

}

UINT32 
AD_GetTime(VOID)
{
	static UINT32 nCounter = 0;

	return nCounter++;
}

VOID
AD_WaitNMSec(UINT32 nNMSec)
{
	mdelay(4);
}

BOOL32
AD_GetROLockFlag(VOID)
{
	return 1;
}

#ifdef ASYNC_MODE
VOID
AD_InitInt(UINT32 nLogIntId)
{   
}

VOID
AD_BindInt(UINT32 nLogIntId, UINT32 nPhyIntId)
{
}

VOID
AD_EnableInt(UINT32 nLogIntId, UINT32 nPhyIntId)
{
}
    
VOID
AD_DisableInt(UINT32 nLogIntId, UINT32 nPhyIntId)
{
}

VOID
AD_ClearInt(UINT32 nLogIntId, UINT32 nPhyIntId)
{
}
#endif
