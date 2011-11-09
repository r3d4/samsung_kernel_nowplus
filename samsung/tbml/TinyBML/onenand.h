/*****************************************************************************/
/*                                                                           */
/* PROJECT : ANYSTORE II                                                     */
/* MODULE  : LLD                                                             */
/* NAME    : OneNAND LLD header                                              */
/* FILE    : ONLD.h                                                          */
/* PURPOSE : This file implements the exported function declarations and     */
/*           the exported values return values, macros, types,...            */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/*          COPYRIGHT 2003-2005, SAMSUNG ELECTRONICS CO., LTD.               */
/*                          ALL RIGHTS RESERVED                              */
/*                                                                           */
/*   Permission is hereby granted to licensees of Samsung Electronics        */
/*   Co., Ltd. products to use or abstract this computer program for the     */
/*   sole purpose of implementing NAND/OneNAND based on Samsung              */
/*   Electronics Co., Ltd. products. No other rights to reproduce, use,      */
/*   or disseminate this computer program, whether in part or in whole,      */
/*   are granted.                                                            */
/*                                                                           */
/*   Samsung Electronics Co., Ltd. makes no representations or warranties    */
/*   with respect to the performance of this computer program, and           */
/*   specifically disclaims any responsibility for any damages,              */
/*   special or consequential, connected with the use of this program.       */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* REVISION HISTORY                                                          */
/*                                                                           */
/* - 03/JUN/2003 [Janghwan Kim] : first writing                              */
/* - 04/OCT/2003 [Janghwan Kim] : reorganization                             */
/* - 11/DEC/2003 [Janghwan Kim] : Add onld_ioctl() function                  */
/* - 11/DEC/2003 [Janghwan Kim] : Change parmameter of onld_init()           */
/*                                                                           */
/*****************************************************************************/

#ifndef _ONENAND_H_
#define _ONENAND_H_

/*****************************************************************************/
/* ONLD External Function declarations                                       */
/*****************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

INT32 onld_init         (VOID  *pParm);
INT32 onld_open         (UINT32 nDev);   
INT32 onld_close        (UINT32 nDev);   
INT32 onld_read         (UINT32 nDev,    UINT32   nPsn,     UINT32 nScts, 
                         UINT8 *pMBuf,   UINT8   *pSBuf,    UINT32 nFlag);
INT32 onld_write        (UINT32 nDev,    UINT32   nPsn,     UINT32 nScts, 
                         UINT8 *pMBuf,   UINT8   *pSBuf,    UINT32 nFlag);
INT32 onld_erase        (UINT32 nDev,    UINT32   nPbn,     UINT32 nFlag);
INT32 onld_copyback     (UINT32 nDev,    CpBkArg *pstCpArg, UINT32 nFlag);
INT32 onld_chkinitbadblk(UINT32 nDev,    UINT32   nPbn);
INT32 onld_setrwarea    (UINT32 nDev,    UINT32   nSUbn,    UINT32 nUBlks);
INT32 onld_flushop      (UINT32 nDev);   
INT32 onld_getdevinfo   (UINT32 nDev,    LLDSpec *pstLLDDev);
INT32 onld_getprevopdata(UINT32 nDev,    UINT8   *pMBuf,    UINT8 *pSBuf);
INT32 onld_ioctl        (UINT32 nDev,    UINT32   nCmd,
                         UINT8 *pBufI,   UINT32   nLenI, 
                         UINT8 *pBufO,   UINT32   nLenO,
                         UINT32 *pByteRet);
INT32 onld_mread        (UINT32 nDev,    UINT32   nPsn,     UINT32 nScts, 
                         SGL  *pstSGL,   UINT8   *pSBuf,    UINT32 nFlag);
INT32 onld_mwrite       (UINT32 nDev,    UINT32   nPsn,     UINT32 nScts, 
                         SGL  *pstSGL,   UINT8   *pSBuf,    UINT32 nFlag,
						 UINT32 *pErrPsn);
INT32 onld_eraseverify  (UINT32 nDev,    LLDMEArg *pstMEArg,
                         UINT32 nFlag);
INT32 onld_merase       (UINT32 nDev,    LLDMEArg *pstMEArg, 
                         UINT32 nFlag);
VOID  onld_getplatforminfo     
                        (LLDPlatformInfo*    pstLLDPlatformInfo);
						 
#ifdef __cplusplus
};
#endif // __cplusplus

#endif /*  _ONENAND_H_ */
