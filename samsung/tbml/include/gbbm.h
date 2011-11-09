/*****************************************************************************/
/*                                                                           */
/* PROJECT : AnyStore II                                                     */
/* MODULE  : XSR BML                                                         */
/* NAME    : Bad Block Manager                                               */
/* FILE    : BadBlkMgr.h                                                     */
/* PURPOSE : This file contains the definition and prototypes of exported    */
/*           functions for Bad Block Manager                                 */
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
/*   11-JUL-2003 [SongHo Yoon]: First writing                                */
/*   02-DEC-2003 [SongHo Yoon]: modified nFlag of BBM_HandleBadBlk           */
/*   02-DEC-2003 [SongHo Yoon]: change structure name                        */
/*                              (BBMPoolCtlHdr => PoolCtlHdr)                */
/*   10-DEC-2003 [SongHo Yoon]: added LPCH_SIG / UPCH_SIG / MAX_PCH_SIG /    */
/*                              BMFS_PER_BMS / SCTS_PER_BMI / PCH_CFM_POS /  */
/*                              PCH_CFM_INV_DATA definition                  */
/*	 31-JAN-2006 [ByoungYoung Ahn] : modified for refresh by erase scheme	 */
/*   31-Jan-2007 [Younwon Park] : added BBM_SKIP_ERASE_REFRESH               */
/*                                                                           */
/*****************************************************************************/

#ifndef     _BML_BADBLKMGR_H_
#define     _BML_BADBLKMGR_H_

/*****************************************************************************/
/* Return value of BBM_XXX()                                                 */
/*****************************************************************************/
#define     BBM_SUCCESS                 XSR_RETURN_VALUE(0, 0x0000, 0x0000)
#define     BBM_CRITICAL_ERROR          XSR_RETURN_VALUE(1, 0x0001, 0x0000)
#define     BBM_MOUNT_FAILURE           XSR_RETURN_VALUE(1, 0x0002, 0x0000)
#define     BBM_CONSTRUCT_BMF_FAILURE   XSR_RETURN_VALUE(1, 0x0003, 0x0000)
#define     BBM_NO_LPCA                 XSR_RETURN_VALUE(1, 0x0004, 0x0000)
#define     BBM_NO_UPCA                 XSR_RETURN_VALUE(1, 0x0004, 0x0000)
#define     BBM_LOAD_PI_FAILURE         XSR_RETURN_VALUE(1, 0x0004, 0x0000)
#define     BBM_LOAD_UBMS_FAILURE       XSR_RETURN_VALUE(1, 0x0004, 0x0000)
#define     BBM_LOAD_LBMS_FAILURE       XSR_RETURN_VALUE(1, 0x0004, 0x0000)
#define     BBM_NO_RSV_BLK_POOL         XSR_RETURN_VALUE(1, 0x0005, 0x0000)
#define     BBM_MAKE_NEW_PCB_FAILURE    XSR_RETURN_VALUE(1, 0x0006, 0x0000)
#define     BBM_UPDATE_PIEXT_FAILURE    XSR_RETURN_VALUE(1, 0x0007, 0x0000)
#define     BBM_WR_PROTECT_ERROR        XSR_RETURN_VALUE(1, 0x0008, 0x0000)
#define		BBM_REFRESH_BLK_ERROR		XSR_RETURN_VALUE(1, 0x0009, 0x0000)
#define		BBM_ERL_BLK_FULL			XSR_RETURN_VALUE(1, 0x000a, 0x0000)
#define		BBM_SKIP_ERASE_REFRESH		XSR_RETURN_VALUE(1, 0x001A, 0x0000)

#define     LLD_READ_CERROR_ALL     (LLD_READ_CERROR_S0 | LLD_READ_CERROR_M0 | \
                                     LLD_READ_CERROR_S1 | LLD_READ_CERROR_M1 | \
                                     LLD_READ_CERROR_S2 | LLD_READ_CERROR_M2 | \
                                     LLD_READ_CERROR_S3 | LLD_READ_CERROR_M3)


#define     LLD_READ_UERROR_ALL     (LLD_READ_UERROR_S0 | LLD_READ_UERROR_M0 | \
                                     LLD_READ_UERROR_S1 | LLD_READ_UERROR_M1 | \
                                     LLD_READ_UERROR_S2 | LLD_READ_UERROR_M2 | \
                                     LLD_READ_UERROR_S3 | LLD_READ_UERROR_M3)

#define     BML_READ_UERROR_ALL     (BML_READ_ERROR_S0 | BML_READ_ERROR_M0 | \
                                     BML_READ_ERROR_S1 | BML_READ_ERROR_M1 | \
                                     BML_READ_ERROR_S2 | BML_READ_ERROR_M2 | \
                                     BML_READ_ERROR_S3 | BML_READ_ERROR_M3)

/*****************************************************************************/
/* nFlag of BBM_HandleBadBlk                                                 */
/*****************************************************************************/
#define     BBM_HANDLE_READ_ERROR       (0x01)
#define     BBM_HANDLE_WRITE_ERROR      (0x02)
#define     BBM_HANDLE_ERASE_ERROR      (0x04)

/*****************************************************************************/
/* Spare area                                                                */
/*****************************************************************************/
#define     BBM_VALID_DATA              (UINT8) (0xFF)
#define     BBM_INVALID_DATA_MASK       (UINT8) (0xFF)
#define     BBM_INVALID_MDATA           (UINT8) (0x0F)
#define     BBM_INVALID_SDATA           (UINT8) (0xF0)
#define     BBM_INVALID_MARK_OFFSET     (15)

/*****************************************************************************/
/* Signature for Locked Pool Control Header                                  */
/*****************************************************************************/
#define     LPCH_SIG                (UINT8 *) "LOCKPCHD"

/*****************************************************************************/
/* Signature for Unlocked Pool Control Header                                */
/*****************************************************************************/
#define     UPCH_SIG                (UINT8 *) "ULOCKPCH"

/*****************************************************************************/
/* number of meta blocks in Reservoir                                        */
/*****************************************************************************/
#define     RSV_META_BLKS           (6)

/*****************************************************************************/
/* offset to start scanning PCB												 */
/*****************************************************************************/
#define		RSV_PCB_OFFSET			(2)

/*****************************************************************************/
/*  number of copies for BMI/PI/PCH mirroring                                */
/*****************************************************************************/
#define     NUM_OF_MIRRORS          (2)
#define     NUM_OF_BMS              (6)

#define     SCTS_PER_PCH            (2)
#define     SCTS_PER_PIA            (2)
#define     SCTS_PER_BMS            (2)
#define     BMS_PER_BMI             (2)
#define     SCTS_PER_BMI            (NUM_OF_MIRRORS * SCTS_PER_BMS)

/*****************************************************************************/
/* maximum number of Block Map Field                                         */
/*****************************************************************************/
#define     MAX_BMF                 (BMFS_PER_BMS * NUM_OF_BMS)

/*****************************************************************************/
/* the size of Block Allocation Bit Map                                      */
/*****************************************************************************/
#define     BABITMAP_SIZE           (MAX_BMF / 8) + ((MAX_BMF % 8) ? 1 : 0)

/*****************************************************************************/
/* the number of blocks per bad unit (pBUMap)                                */
/*****************************************************************************/
#define     BLKS_PER_BADUNIT        (32)
#define     SFT_BLKS_PER_BADUNIT    (5)

/*****************************************************************************/
/* offset for Confirmation from LsnPos                                       */
/*****************************************************************************/
#define     CFM_OFFSET              (3)

/*****************************************************************************/
/* inverted value for PCH confirmation from LsnPos                           */
/*****************************************************************************/
#define     CFM_INV_DATA            (UINT8) (0x01)
#define     BML_READ_RETRY_CNT          1

/*****************************************************************************/
/* Refresh block state														 */
/*****************************************************************************/
typedef enum
{
	REF_BLK_INVALID = 0, 
	REF_BLK_USER_DATA = 1,
	REF_BLK_ERL_DATA = 2
} RefBlkState;

/*****************************************************************************/
/* exported function prototype of Bad Block Manager                          */
/*****************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

INT32 BBM_Format           (flash_vol *pstVol, flash_dev *pstDev, part_info *pstPart);
INT32 BBM_Repartition      (flash_vol *pstVol, flash_dev *pstDev, part_info *pstPart,
                            UINT16 n1stSbnOfULA, UINT16 n1stSbnOfOldULA);
INT32 BBM_Mount            (flash_vol *pstVol, flash_dev *pstDev, 
                            part_info *pstPI, part_exinfo *pstPExt);
VOID  BBM_RecalAllocBlkPtr (flash_dev *pstDev);
INT32 BBM_UpdatePIExt      (flash_vol *pstVol, flash_dev *pstDev, part_exinfo *pstPExt);
INT32 BBM_HandleBadBlk     (flash_vol *pstVol, flash_dev *pstDev, UINT32 nBadSbn,
                            UINT32 nSecOff, UINT32 nFlag);
VOID  BBM_WaitUntilPowerDn      (VOID);
VOID  BBM_SetWaitTimeForErError (UINT32 nNMSec);
VOID  BBM_PrintBMI         (flash_vol *pstVol, flash_dev *pstDev);
INT32 BBM_RefreshByErase   (flash_vol *pstVol, flash_dev *pstDev, UINT32 nOrgSbn);
INT32 BBM_ChkRefreshBlk	   (flash_vol *pstVol, flash_dev *pstDev);
INT32 BBM_UpdateERL		   (flash_vol *pstVol, flash_dev *pstDev, UINT32 nOrgSbn);
INT32 BBM_LoadERL		   (flash_vol *pstVol, flash_dev *pstDev);
INT32 BBM_RenewERLBlk	   (flash_vol *pstVol, flash_dev *pstDev);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BML_BADBLKMGR_H_ */
