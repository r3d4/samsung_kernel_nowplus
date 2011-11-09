/*
   %COPYRIGHT%
*/   
/**
 * @file    os_adapt.h
 * @brief   This file is os adaptation hesder file
 *
 */

#ifndef _OS_ADAPT_H_
#define	_OS_ADAPT_H_

/* RTL Debug Message */
#if defined(OAM_RTLMSG_DISABLE)
	#define	XSR_RTL_PRINT(x)	
#else
	#define	XSR_RTL_PRINT(x)		AD_Debug x	
#endif /* OAM_RTLMSG_DISABLE */

/* DBG Debug Message */
#if defined(OAM_DBGMSG_ENABLE)
	#define	XSR_DBG_PRINT(x)		AD_Debug x	
#else
	#define	XSR_DBG_PRINT(x)		
#endif /* OAM_DBGMSG_ENABLE */

#undef	TEXT
#define	TEXT(x)					(VOID *) (x)

/*****************************************************************************/
/* NULL #defines                                                             */
/*   If Each OAM header file defines NULL, following define is not used.     */
/*****************************************************************************/
#ifndef		NULL
#ifdef      __cplusplus
#define     NULL                0
#else
#define     NULL                ((void *)0)
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*****************************************************************************/
/* exported function prototype of AD                                         */
/*****************************************************************************/
#ifdef ASYNC_MODE
VOID	 AD_InitInt   		(UINT32 nLogIntId);
VOID	 AD_BindInt   		(UINT32 nLogIntId, UINT32 nPhyIntId); 
VOID	 AD_EnableInt 		(UINT32 nLogIntId, UINT32 nPhyIntId); 
VOID	 AD_DisableInt		(UINT32 nLogIntId, UINT32 nPhyIntId); 
VOID	 AD_ClearInt  		(UINT32 nLogIntId, UINT32 nPhyIntId);
#endif

VOID    *AD_Malloc    		(UINT32 nSize);
VOID     AD_Free      		(VOID  *pMem);
VOID     AD_Memcpy    		(VOID  *pDst, VOID  *pSrc,   UINT32 nLen);
VOID     AD_Memset    		(VOID  *pDst, UINT8  nData,  UINT32 nLen);
INT32    AD_Memcmp    		(VOID  *pCmp1,VOID  *pCmp2,  UINT32 nLen);

BOOL32   AD_CreateSM  		(SM32  *pHandle);
BOOL32   AD_DestroySM 		(SM32   nHandle);
BOOL32   AD_AcquireSM 		(SM32   nHandle);
BOOL32   AD_ReleaseSM 		(SM32   nHandle);
UINT32   AD_Pa2Va     		(UINT32 nPAddr);

VOID     AD_Debug     		(VOID *pStr, ...);

VOID 	 AD_ResetTimer		(VOID);
UINT32   AD_GetTime   		(VOID);
VOID     AD_WaitNMSec 		(UINT32 nNMSec);

BOOL32 	 AD_GetROLockFlag	(VOID);
VOID	 AD_Idle		(UINT32 nMode);

#endif	/* _OS_ADAPT_H_ */
