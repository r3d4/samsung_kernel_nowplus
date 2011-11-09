/*****************************************************************************/
/*                                                                           */
/* PROJECT : AnyStore II                                                     */
/* MODULE  : XSR BML                                                         */
/* NAME    : Bad Block Manager                                               */
/* FILE    : BadBlkMgr.cpp                                                   */
/* PURPOSE : This file contains the routine for managing reservoir           */
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
/*   07-JUL-2003 [SongHo Yoon]: First writing                                */
/*   11-NOV-2003 [Jaesung Jung]: added FiLM inferface MACRO                  */
/*   02-DEC-2003 [SongHo Yoon]: removed refreshmenet in BBM_HandleBadBlk()   */
/*   02-DEC-2003 [SongHo Yoon]: changed function name BML_Open to BML_Mount  */
/*                              removeed BML_Close                           */
/*   13-DEC-2003 [SongHo Yoon]: whole code is rewritten to adapt new write   */
/*                              protection scheme                            */
/*   29-DEC-2003 [SongHo Yoon]: fixed bug in _MakeBadBlk()/_DeletePcb()      */
/*   12-JAN-2004 [SongHo Yoon]: fixed sequential write operation problem     */
/*                              in large block flash memory                  */
/*                              - _UpdateLPCA() / _MakeUPCB()                */
/*   19-JAN-2004 [SongHo Yoon]: changed BMI maintanance scheme               */
/*                              - register bad block of reservoir into BMI   */
/*                                to search free reserved block              */
/*                            changed free reserved block searching algorithm*/
/*                              - use reserved Block Allocation BitMap that  */
/*                                is built by BMI and initial bad block      */
/*   28-JAN-2004 [SongHo Yoon]: fixed a bug of BBM_HandleBadBlk()            */
/*                              - A bug is that the replaced block becomes   */
/*                                a bad block                                */
/*   28-JAN-2004 [SongHo Yoon]: fixed a bug of _CopyBlk()                    */
/*                              - A bug is that destination page has invalid */
/*                                data mark during coping write-error page   */
/*   29-JAN-2004 [SongHo Yoon]: fixed a bug of _CopyBlk()                    */
/*                              - A bug is that data can have 1/2bit error   */
/*                                when write-error page is read              */
/*   29-APR-2004 [SongHo Yoon]: fixed a bug of _LoadUBMS() / _LoadLBMS()     */
/*                              - unexpected data changes of nUPcbAge        */
/*   29-MAY-2004 [SongHo Yoon]; fixed a bug of _RegBMF()                     */
/*   05-OCT-2004 [Janghwan Kim]; reorganized codes                           */
/*   30-SEP-2005 [Younwon Park]: reorganized codes for XSR v1.5              */
/*   01-JAN-2006 [ByoungYoung Ahn] : reduced stack usage		     */
/*   01-JAN-2006 [ByoungYoung Ahn] : added codes for refresh by erase scheme */
/*   03-AUG-2006 [YulWon Cho] : modified in BBM_RecalAllocBlkPtr()           */
/*                              AllocPCB()                                   */
/*   03-AUG-2006 [YulWon Cho] : added codes for initial bad block            */
/*                              register of ERL, REF Block                   */
/*   10-JAN-2007 [WooYoung Yang] : fixed a bug unhandling value of certain   */
/*                                 range for writing a confirm mark          */
/*                                 in _WriteBMI()                            */
/*   31-JAN-2007 [Younwon Park] : modified erase refresh scheme              */ 
/*                                in _CopyBlkRefresh                         */
/*   30-APR-2007 [Younwon Park] : modified _LoadBMS() to prevent program fail*/
/*                                caused by soft program                     */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* NOTE                                                                      */
/*    << Reservior structure >>                                              */
/*                                                                           */
/*           +----------------------+ <---End of NAND flash block--- highest */
/*           |       LPCB           |             |             |    addr    */
/*    +--->  +----------------------+ <---+       |             |            */
/*    |      | Reserved Block m+n-1 |     |  2nd reservoir      |            */
/*    |      +----------------------+     |       |             |            */
/*    |      | Reserved Block m+n-2 |     |       |             |            */
/*    |      +----------------------+     |       |             |            */
/*    |      |        ....          |     |       |             |            */
/*    |      +----------------------+     |       |             |            */
/*    |      |        ....          |     |       |             |            */
/*    |      +----------------------+    (n)      |        the size of       */
/*    |      |        ....          |     |     LOCKED      reservoir        */
/*  Reserved +----------------------+     |     AREA            |            */
/*   Block   |   Reserved Block m   |     |       |             |            */
/*    Pool   +- - - - - - - - - - - + <---+ - - - + -           |            */
/*    |      |   Reserved Block m-1 |     |       |             |            */
/*    |      +----------------------+     |       |             |            */
/*    |      |        ....          |    (m)  UNLOCKED          |            */
/*    |      +----------------------+     |      AREA           |            */
/*    |      |   Reserved Block 1   |     |       |             |            */
/*    |      +----------------------+     |       |             |            */
/*    |      |   Reserved Block 0   |     |  1st reservoir      |            */
/*    +--->  +----------------------+ <---+       |             |            */
/*           |        UPCB #2       |             |             |            */
/*           +----------------------+             |             |            */
/*           |        UPCB #1       |             |             |            */
/*           +----------------------+ <----------------------------- lowest  */
/*                                                                   addr    */
/*       (n + m + 3) Blocks must be allocated for Bad Block Management.      */
/*                                                                           */
/*    << Terminology >>                                                      */
/*                                                                           */
/*      a. GBBM  : Global Bad Block Management                               */
/*      b. UPCB  : Unlocked reserved block Pool Control Block                */
/*      c. LPCB  : Locked reserved block Pool Control Block                  */
/*      d. BMS   : Block Map Sector                                          */
/*      e. PIA   : Partition Information Area                                */
/*                                                                           */
/*****************************************************************************/

#include <tbml_common.h>
#include <os_adapt.h>
#include <onenand_interface.h>
#include <tbml_interface.h>
#include <tbml_type.h>
#include <gbbm.h>
#include "../TinyBML/onenand.h"

#ifdef KERNEL_CODE
#include <linux/kernel.h>
#endif

/*****************************************************************************/
/* Local Debug configuration                                                 */
/*****************************************************************************/


/*****************************************************************************/
/* nScanDir parameter of _GetFreeRB()                                        */
/*****************************************************************************/
#define     BBM_SCAN2HIGH               (0x00000001)
#define     BBM_SCAN2LOW                (0x00000002)

/*****************************************************************************/
/* Type of PCB                                                               */
/*****************************************************************************/
#define     TYPE_LPCA                   (0x00000001)
#define     TYPE_UPCA                   (0x00000002)

/*****************************************************************************/
/* Type of PI                                                                */
/*****************************************************************************/
#define     TYPE_PI                     (0x00000003)
#define     TYPE_PIEXT                  (0x00000004)

/*****************************************************************************/
/* nUpdateType                                                               */
/*****************************************************************************/
#define     TYPE_INIT_CREATE            (UINT16) (0xFCFE)
#define     TYPE_UPDATE_PIEXT           (UINT16) (0xFAFE)
#define     TYPE_READ_ERR               (UINT16) (0xFBFE)
#define     TYPE_WRITE_ERR              (UINT16) (0xFDFE)
#define     TYPE_ERASE_ERR              (UINT16) (0xFEFE)

/*****************************************************************************/
/* BAD mark value                                                            */
/*****************************************************************************/
#define     BADMARKV_READERR            (UINT8) (0x44)
#define     BADMARKV_WRITEERR           (UINT8) (0x22)
#define     BADMARKV_ERASEERR           (UINT8) (0x11)

/*****************************************************************************/
/* Refresh by erase                                                          */
/*****************************************************************************/
#define		REF_SPARE_PBN_POS			(0)
#define		REF_SPARE_COUNT_POS			(6)
#define		REF_SPARE_VALIDMARK_POS		(4)
#define		REF_SPARE_VALIDMARK_1TIME	(0xF0)
#define		REF_SPARE_VALIDMARK_2TIME	(0x00)
#define		REF_SPARE_MAIN_ECC			(8)
#define		REF_SPARE_SPARE_ECC			(11)

#define		ERL_CNT_POS					(6)
#define		ERL_AGE_POS					(7)
#define		ERL_DATA_POS				(8)
#define		ERL_SPARE_VALIDMARK_POS		(5)
#define		ERL_SPARE_VALIDMARK			(0x00)
#define		MASK_ERL_VALIDMARK			(0xFF)

/*****************************************************************************/
/* swap macro for supporting big endianess                                   */
/*****************************************************************************/

/*****************************************************************************/
/* Local typedefs                                                            */
/*****************************************************************************/
typedef struct {
    UINT16   n1stPbn;
    UINT16   nNumOfBlks;
} Area;

/*****************************************************************************/
/* Local variables definitions                                               */
/*****************************************************************************/
                           
/*****************************************************************************/
/* Static function prototypes                                                */
/*****************************************************************************/
static VOID    _Init           (flash_dev *pstDev, flash_vol *pstVol);
static INT32   _ScanReservoir  (flash_dev *pstDev, flash_vol *pstVol);
static VOID    _LoadPIExt      (flash_dev *pstDev, flash_vol *pstVol, part_exinfo *pstPExt);
static BOOL32  _LoadBMS        (flash_dev *pstDev, flash_vol *pstVol, UINT32 nPCAType);
static BOOL32  _LoadPI         (flash_dev *pstDev, flash_vol *pstVol, part_info *pstPI);
static VOID    _SortBMI        (bmi *pstBMI);
static VOID    _SetAllocRB     (flash_dev *pstDev, flash_vol *pstVol, UINT32 nPbn);
static VOID    _PrintPI        (part_info *pstPI);
static BOOL32  _ChkBMIValidity (flash_vol *pstVol, reservoir *pstRsv);
static VOID    _ReconstructBUMap(flash_vol *pstVol, reservoir *pstRsv);

static INT32   _ReadMetaData   (UINT32 nPsn, UINT8 *pMBuf, flash_dev *pstDev, flash_vol *pstVol);

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      _LLDRead                                                             */
/* DESCRIPTION                                                               */
/*      This function call LLD_Read() with the given sector                  */
/* PARAMETERS                                                                */
/*      pstVol     [IN]                                                      */
/*            pointer to flash_vol structure                                  */
/*      nPDev      [IN]                                                      */
/*            physical device number                                         */
/*      nPSN       [IN]                                                      */
/*            physical sector number                                         */
/*      nSctsInPg  [IN]                                                      */
/*            number of sectors to be read.                                  */
/*            0 < nSctsInPg < sectors per page                               */
/*      pMBuf      [IN]                                                      */
/*            pointer to buffer for main area to be read                     */
/*      pSBuf      [IN]                                                      */
/*            pointer to buffer for spare area to be read                    */
/* RETURN VALUES                                                             */
/*      LLD_SUCCESS                                                          */
/*      LLD_READ_ERROR                                                       */
/* NOTES                                                                     */
/*      pSBuf shoule not be NULL                                             */
/*                                                                           */
/*****************************************************************************/
static INT32
_LLDRead(flash_vol *pstVol, UINT32 nPDev, UINT32 nPSN, UINT32 nSctsInPg,
         UINT8 *pMBuf, UINT8 *pSBuf, UINT32 nFlag)
{
    INT32  nLLDRe;

    nLLDRe = pstVol->lld_read(nPDev, nPSN, nSctsInPg, pMBuf, pSBuf, nFlag);
    if (nLLDRe != LLD_SUCCESS)
    {
        return nLLDRe;
    }
    
    return nLLDRe;
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      _ReadMetaData                                                        */
/* DESCRIPTION                                                               */
/*      This function reads data from the given nPsn and this function checks*/
/*      confirmation mark.                                                   */
/* PARAMETERS                                                                */
/*      nPsn         [IN]                                                    */
/*            Physical sector number                                         */
/*      pMBuf        [IN]                                                    */
/*            Pointer of meta data array                                     */
/*      pstDev       [IN]                                                    */
/*            Pointer of flash_dev structure                                  */
/*      pstVol       [IN]                                                    */
/*            Pointer of flash_vol structure                                  */
/* RETURN VALUES                                                             */
/*      TRUE32                                                               */
/*            data is valid                                                  */
/*      FALSE32                                                              */
/*            data is invalid                                                */
/* NOTES                                                                     */
/*      This function uses the temporary buffer of flash_dev structure.       */
/*                                                                           */
/*****************************************************************************/
static INT32
_ReadMetaData(UINT32 nPsn, UINT8 *pMBuf, flash_dev *pstDev, flash_vol *pstVol)
{
    UINT32       nPDev;
    INT32        nLLDRe;
    INT32        nMajorErr;
    INT32        nMinorErr;
    UINT16       nIdx;
    UINT8       *pSBuf;
    UINT8        nReadCnt;
    INT32        bValid = FALSE32;

    BBM_LOG_PRINT((TEXT("[TBBM: IN] ++_ReadMetaData(PDev:%d,nPbn:%d,SecOff:%d)\r\n"),
                   pstDev->nDevNo,
                   nPsn / pstVol->nSctsPerBlk,
                   nPsn % pstVol->nSctsPerBlk));

    nPDev  = pstDev->nDevNo;
    pSBuf  = (UINT8 *) pstDev->pTmpSBuf;

    do
    {
        nReadCnt = 0;
        do
        {
            nLLDRe = _LLDRead(pstVol,               /* Volume Number            */
                              nPDev,                /* Physical Device Number   */
                              nPsn,                 /* Psn                      */
                              2,                    /* Number of Sectors        */
                              (UINT8 *) pMBuf,      /* Buffer for Main          */
                              (UINT8 *) pSBuf,      /* Buffer for Spare         */
                              LLD_FLAG_ECC_ON);     /* Operation Flag           */   
            if (XSR_RETURN_MAJOR(nLLDRe) == LLD_SUCCESS || XSR_RETURN_MAJOR(nLLDRe) == LLD_READ_DISTURBANCE)
            {
                break;         
            }
                        
        } while(++nReadCnt < BML_READ_RETRY_CNT);

        /*
           First, check whether the mirror sector has confirmation mark or not
           If the mirror sector has no confirm-mark, sector has invalid information 
        
           The original sector has always confirm-mark
           if the mirror sector has confirm-mark.
        */
        if (((UINT8) ~pSBuf[pstVol->nLsnPos + CFM_OFFSET + XSR_SPARE_SIZE] & CFM_INV_DATA) != CFM_INV_DATA)
        {
            BBM_INF_PRINT((TEXT("[TBBM:   ]  _ReadMetaData(PDev:%d,nPbn:%3d,SecOff:%3d) / Invalid Confirm(0x%x)\r\n"),
                           pstDev->nDevNo,
                           nPsn / pstVol->nSctsPerBlk,
                           nPsn  % pstVol->nSctsPerBlk,
                           pSBuf[pstVol->nLsnPos + CFM_OFFSET]));
            break;
        }
#if defined(BBM_INF_MSG_ON)
        else
        {
            BBM_INF_PRINT((TEXT("[TBBM:   ]  _ReadMetaData(PDev:%d,nPbn:%3d,SecOff:%3d) /   Valid Confirm(0x%x)\r\n"),
                           pstDev->nDevNo,
                           nPsn / pstVol->nSctsPerBlk,
                           nPsn % pstVol->nSctsPerBlk,
                           pSBuf[pstVol->nLsnPos + CFM_OFFSET]));
        }
#endif /* (BBM_INF_MSG_ON) */

        nMajorErr = XSR_RETURN_MAJOR(nLLDRe);
        nMinorErr = XSR_RETURN_MINOR(nLLDRe);
        
        /* if unrecoverable error occurs, it is error */
        if (nMajorErr == LLD_READ_ERROR)
        {
            BBM_INF_PRINT((TEXT("[TBBM:ERR]  U_ReadErr at Blk:%d,Sec:%d\r\n"),
                           nPsn / pstVol->nSctsPerBlk,
                           nPsn % pstVol->nSctsPerBlk));

            break;
        }
        
        for(nIdx = 0; nIdx < XSR_SECTOR_SIZE; nIdx++)
        {
            if (pMBuf[nIdx] != pMBuf[nIdx + XSR_SECTOR_SIZE])
                break;
        }
        
        if(nIdx != XSR_SECTOR_SIZE)
        {
            break;
        }
        if (((UINT8) ~pSBuf[pstVol->nLsnPos + CFM_OFFSET] & CFM_INV_DATA) != CFM_INV_DATA)
        {
            bValid = -1;
            break;  
        }               
        bValid = TRUE32;
        break;
        
    } while(0);

    BBM_LOG_PRINT((TEXT("[TBBM:OUT] --_ReadMetaData(PDev:%d,nPbn:%d,SecOff:%d,bRet=%d)\r\n"),
                   pstDev->nDevNo,
                   nPsn / pstVol->nSctsPerBlk,
                   nPsn % pstVol->nSctsPerBlk,
                   bValid));

    return bValid;
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      _Init                                                                */
/* DESCRIPTION                                                               */
/*      This function initializes the variables of reservoir structure       */
/* PARAMETERS                                                                */
/*      pstDev    [IN]                                                       */
/*            flash_dev structure pointer                                     */
/* RETURN VALUES                                                             */
/*      none                                                                 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
static VOID
_Init(flash_dev *pstDev, flash_vol *pstVol)
{
    reservoir *pstRsv;

    pstRsv = &(pstDev->stRsv);

    pstRsv->nAllocBlkPtrL       = (UINT16) pstVol->nBlksPerDev;
    pstRsv->nAllocBlkPtrU       = (UINT16) pstVol->n1stSbnOfRsvr;

    pstRsv->nCurUPCBIdx         = 0;
    pstRsv->nCurLPCBIdx         = 0;    /* it is not used */

    pstRsv->nUPcb[0]            = 0;
    pstRsv->nUPcb[1]            = 0;
    pstRsv->nLPcb[0]            = 0;
    pstRsv->nLPcb[1]            = 0;    /* it is not used */

    pstRsv->nLPcbAge            = 0;
    pstRsv->nUPcbAge            = 0;

    pstRsv->nNextUBMSOff        = 0;
    pstRsv->nNextLBMSOff        = 0;    /* it is not used */

    pstRsv->stBMI.nLAge          = 0;
    pstRsv->stBMI.nUAge          = 0;
    pstRsv->stBMI.nNumOfBMFs     = 0;

    /* Clear all Block Map Field. 0xFFFF means non-mapping information */
    AD_Memset(pstRsv->stBMI.pstBMF, 0xFF, (pstVol->nRsvrPerDev - RSV_META_BLKS) * sizeof(bmf));

    /* initialize pstRsv->pBABitMap by 0x00 */
    AD_Memset(pstRsv->pBABitMap, 0x00, BABITMAP_SIZE);
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      _ScanReservoir                                                       */
/* DESCRIPTION                                                               */
/*      This function scans reservoir to search latest UPCB/LPCB.            */
/* PARAMETERS                                                                */
/*      pstDev      [IN]                                                     */
/*            flash_dev structure pointer                                     */
/*      pstVol      [IN]                                                     */
/*            flash_vol structure pointer                                     */
/* RETURN VALUES                                                             */
/*      BBM_SUCCESS                                                          */
/*            UPCA and LPCA are found                                        */
/*      BBM_NO_UPCA                                                          */
/*            there is no UPCA                                               */
/*      BBM_NO_LPCA                                                          */
/*            there is no LPCA                                               */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
static INT32
_ScanReservoir(flash_dev *pstDev, flash_vol *pstVol)
{
    UINT32      nPDev;
    UINT32      nPbn;
    BOOL32      bUPCA = FALSE32;
    BOOL32      bLPCA = FALSE32;
    reservoir  *pstRsv;
    pch *pstPCH;

    BBM_LOG_PRINT((TEXT("[TBBM: IN] ++_ScanReservoir(nPDev:%d)\r\n"), 
                   pstDev->nDevNo));

    nPDev  = pstDev->nDevNo;
    pstRsv = &(pstDev->stRsv);
    /* allocated temporary buffer for main area */
    pstPCH = (pch *) pstDev->pTmpMBuf;

    BBM_INF_PRINT((TEXT("[TBBM:   ]  Scan Blk:%d ==> Blk:%d for UPCA\r\n"),
                   pstVol->n1stSbnOfRsvr + RSV_PCB_OFFSET, pstVol->nBlksPerDev - 1));

    pstRsv->nUPcbAge = 0;
    pstRsv->nLPcbAge = 0;
    
    /* Scan reservoir by bottom-up fashion 
       to search for the latest two copies of UPCB 
                 and the latest one copy   of LPCB */
    for (nPbn = pstVol->n1stSbnOfRsvr + RSV_PCB_OFFSET; nPbn < pstVol->nBlksPerDev; nPbn++)
    {
        /* if Blk is bad block, skip */
        if (pstVol->lld_check_bad(nPDev, nPbn) == LLD_INIT_BADBLOCK)
        {
            continue;
        }
        
        /*

           +----------------+
           |      PCH       | <== second, checks validity of header information
           +----------------+
           |      PCH'      |
           +----------------+
           |      PIA       |
           +----------------+
           |      PIA'      |
           +----------------+
           |    1st BMS     |
           +----------------+
           |    1st BMS'    |
           +----------------+
           |    2nd BMS     |
           +----------------+
           |    2nd BMS'    | <== first, checks confirm mark of 2nd BMS'
           +----------------+

        */

        /*  read 2nd BMS or 2nd BMS' data
            if data has 2bit ECC error or data has no confirmatin mark, skip */
        if (_ReadMetaData(nPbn * pstVol->nSctsPerBlk + SCTS_PER_PCH + SCTS_PER_PIA + SCTS_PER_BMS + SCTS_PER_BMI * 2,
                          (UINT8 *) pstPCH,
                          pstDev,
                          pstVol) == FALSE32)
        {
             if (_ReadMetaData(nPbn * pstVol->nSctsPerBlk + SCTS_PER_PCH + SCTS_PER_PIA + SCTS_PER_BMS + SCTS_PER_BMI,
                              (UINT8 *) pstPCH,
                              pstDev,
                              pstVol) == FALSE32)
            {
                if (_ReadMetaData(nPbn * pstVol->nSctsPerBlk + SCTS_PER_PCH + SCTS_PER_PIA + SCTS_PER_BMS,
                                  (UINT8 *) pstPCH,
                                  pstDev,
                                  pstVol) == FALSE32)
                {
                    continue;
                }
            }
        }

        /*  read header data
            if data has 2bit ECC error or data has no confirmatin mark, skip */
        if (_ReadMetaData(nPbn * pstVol->nSctsPerBlk,
                        (UINT8 *) pstPCH,
                        pstDev,
                        pstVol) == FALSE32)
        {
            continue;
        }

        /* check erase signature of UPCB */
        if ((pstPCH->nEraseSig1 != 0x00000000) ||
            (pstPCH->nEraseSig2 != 0x00000000))
        {
            BBM_INF_PRINT((TEXT("[TBBM:   ]   Invalid Erase Signature(0x%x/0x%x)\r\n"),
                           pstPCH->nEraseSig1, pstPCH->nEraseSig2));
            continue;
        }

        /* check signature of UPCB and LPCB */
        if (AD_Memcmp(pstPCH->aSig, UPCH_SIG, MAX_PCH_SIG) == 0)
        {
            /* check age of UPCB */
            if (pstPCH->nAge > pstRsv->nUPcbAge)
            {
                pstRsv->nUPcb[0]    = (UINT16) nPbn;
                pstRsv->nUPcb[1]    = pstPCH->nAltPcb;
                pstRsv->nUPcbAge    = pstPCH->nAge;
                pstRsv->nCurUPCBIdx = 0;

                BBM_INF_PRINT((TEXT("[TBBM:   ]   UPCB(   nPbn:%d)\r\n"), nPbn));
                BBM_INF_PRINT((TEXT("[TBBM:   ]   UPCB(nAltPcb:%d)\r\n"), pstPCH->nAltPcb));
                BBM_INF_PRINT((TEXT("[TBBM:   ]   UPCB(   nAge:%d)\r\n"), pstPCH->nAge));
                BBM_INF_PRINT((TEXT("[TBBM:   ]   UPCB is updated\r\n")));
            }
            else
            {
                BBM_INF_PRINT((TEXT("[TBBM:   ]   UPCB(   nPbn:%d)\r\n"), nPbn));
                BBM_INF_PRINT((TEXT("[TBBM:   ]   UPCB(nAltPcb:%d)\r\n"), pstPCH->nAltPcb));
                BBM_INF_PRINT((TEXT("[TBBM:   ]   UPCB(   nAge:%d)\r\n"), pstPCH->nAge));
                BBM_INF_PRINT((TEXT("[TBBM:   ]   UPCB isn't updated\r\n")));
            }

            bUPCA = TRUE32;
        }
        else if (AD_Memcmp(pstPCH->aSig, LPCH_SIG, MAX_PCH_SIG) == 0)
        {
            /* check age of LPCB */
            if (pstPCH->nAge > pstRsv->nLPcbAge)
            {
                pstRsv->nLPcb[0]    = (UINT16) nPbn;
                pstRsv->nLPcb[1]    = pstPCH->nAltPcb;
                pstRsv->nLPcbAge    = pstPCH->nAge;
                pstRsv->nCurLPCBIdx = 0;

                BBM_INF_PRINT((TEXT("[TBBM:   ]   LPCB(   nPbn:%d)\r\n"), nPbn));
                BBM_INF_PRINT((TEXT("[TBBM:   ]   LPCB(nAltPcb:%d)\r\n"), pstPCH->nAltPcb));
                BBM_INF_PRINT((TEXT("[TBBM:   ]   LPCB(   nAge:%d)\r\n"), pstPCH->nAge));
                BBM_INF_PRINT((TEXT("[TBBM:   ]   LPCB is updated\r\n")));
            }
            else
            {
                BBM_INF_PRINT((TEXT("[TBBM:   ]   LPCB(   nPbn:%d)\r\n"), nPbn));
                BBM_INF_PRINT((TEXT("[TBBM:   ]   LPCB(nAltPcb:%d)\r\n"), pstPCH->nAltPcb));
                BBM_INF_PRINT((TEXT("[TBBM:   ]   LPCB(   nAge:%d)\r\n"), pstPCH->nAge));
                BBM_INF_PRINT((TEXT("[TBBM:   ]   LPCB isn't updated\r\n")));
            }

            bLPCA = TRUE32;

        }
        else
        {
            BBM_INF_PRINT((TEXT("[TBBM:   ]   Invalid PCH Signature(%02x %02x %02x %02x %02x %02x %02x %02x)\r\n"),
                           pstPCH->aSig[0],
                           pstPCH->aSig[1],
                           pstPCH->aSig[2],
                           pstPCH->aSig[3],
                           pstPCH->aSig[4],
                           pstPCH->aSig[5],
                           pstPCH->aSig[6],
                           pstPCH->aSig[7]));
        }
    }

    if (bUPCA == FALSE32)
    {
        BBM_ERR_PRINT((TEXT("[TBBM:ERR]   NO UPCA\r\n")));
        BBM_LOG_PRINT((TEXT("[TBBM:OUT] --_ScanReservoir(nPDev:%d)\r\n"), 
                       pstDev->nDevNo));
        return BBM_NO_UPCA;
    }

    if (bLPCA == FALSE32)
    {
        BBM_ERR_PRINT((TEXT("[TBBM:ERR]   NO LPCA\r\n")));
        BBM_LOG_PRINT((TEXT("[TBBM:OUT] --_ScanReservoir(nPDev:%d)\r\n"), 
                       pstDev->nDevNo));
        return BBM_NO_LPCA;
    }
#if defined(BBM_INF_MSG_ON)
    else
    {
        BBM_INF_PRINT((TEXT("[TBBM:   ]   UPcb[0] : %d\r\n"), pstRsv->nUPcb[0]));
        BBM_INF_PRINT((TEXT("[TBBM:   ]   UPcb[1] : %d\r\n"), pstRsv->nUPcb[1]));
        BBM_INF_PRINT((TEXT("[TBBM:   ]   UPcbAge : %d\r\n"), pstRsv->nUPcbAge));
        BBM_INF_PRINT((TEXT("[TBBM:   ]   LPcb[0] : %d\r\n"), pstRsv->nLPcb[0]));
        BBM_INF_PRINT((TEXT("[TBBM:   ]   LPcb[0] : %d\r\n"), pstRsv->nLPcb[1]));
        BBM_INF_PRINT((TEXT("[TBBM:   ]   LPcbAge : %d\r\n"), pstRsv->nLPcbAge));
    }
#endif /* (BBM_INF_MSG_ON) */

    BBM_LOG_PRINT((TEXT("[TBBM:OUT] --_ScanReservoir(nPDev:%d)\r\n"), 
                   pstDev->nDevNo));

    return BBM_SUCCESS;
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      _LoadPIExt                                                           */
/* DESCRIPTION                                                               */
/*      This function loads the partition information extension              */
/*                                                                           */
/* PARAMETERS                                                                */
/*      pstDev     [IN]                                                      */
/*           Pointer of flash_dev structure                                   */
/*      pstVol     [IN]                                                      */
/*           Pointer of flash_vol structure                                   */
/*      pstPExt    [IN]                                                      */
/*           Pointer of part_exinfo structure                                   */
/* RETURN VALUES                                                             */
/*      none                                                                 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
static VOID
_LoadPIExt(flash_dev *pstDev, flash_vol *pstVol, part_exinfo *pstPExt)
{
    reservoir *pstRsv;
    UINT32     n1stPsnOfPExt;
    UINT8     *pMBuf;


    BBM_LOG_PRINT((TEXT("[TBBM: IN] ++_LoadPIExt(nPDev:%d,pstPExt=0x%x)\r\n"), 
                   pstDev->nDevNo, pstPExt));
    
    pstRsv         = &(pstDev->stRsv);
    n1stPsnOfPExt  = pstRsv->nUPcb[0] * pstVol->nSctsPerBlk + SCTS_PER_PCH;
    pMBuf          = pstDev->pTmpMBuf;

    if (pstPExt != NULL)
    {
        if (_ReadMetaData(n1stPsnOfPExt,
                          (UINT8 *) pMBuf,
                          pstDev,
                          pstVol) == TRUE32)
        {
            AD_Memcpy(&pstPExt->nID,         &pMBuf[0], 4);
            AD_Memcpy(&pstPExt->nSizeOfData, &pMBuf[4], 4);

            if (pstPExt->nSizeOfData > BML_MAX_PIEXT_DATA)
                pstPExt->nSizeOfData = BML_MAX_PIEXT_DATA;

            AD_Memcpy(pstPExt->pData,       &pMBuf[8], pstPExt->nSizeOfData);
        }
        else
        {
            pstPExt->nID         = 0;
            pstPExt->nSizeOfData = 0;
            AD_Memset(pstPExt->pData, 0xFF, BML_MAX_PIEXT_DATA);
        }
    }

    BBM_LOG_PRINT((TEXT("[TBBM:OUT] --_LoadPIExt(nPDev:%d,pstPExt=0x%x)\r\n"), 
                   pstDev->nDevNo, pstPExt));
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      _LoadBMS                                                             */
/* DESCRIPTION                                                               */
/*      This function loads the latest UBMS/LBMS                             */
/*                                                                           */
/* PARAMETERS                                                                */
/*      pstDev       [IN]                                                    */
/*           Pointer to flash_dev                                             */
/*      pstVol       [IN]                                                    */
/*           Pointer to flash_vol                                             */
/*      nPCAType                                                             */
/*           TYPE_LPCA or TYPE_UPCA                                          */
/* RETURN VALUES                                                             */
/*      TRUE32                                                               */
/*            _LoadBMS is completed                                          */
/*      FALSE32                                                              */
/*            There is no valid BMS                                          */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
static BOOL32
_LoadBMS(flash_dev *pstDev, flash_vol *pstVol, UINT32 nPCAType)
{
    bms       *pstBms;
    bmi       *pstBMI;
    bmf       *pSrcBMF;
    bmf       *pDstBMF;
    reservoir *pstRsv;
    UINT32     nPDev;
    UINT32     nIdx1;
    UINT32     nIdx2;
    UINT32     nSctsPerBMSG;
    UINT32     n1stPsnOfBMSG;
    BOOL32     bValidBMS;
	BOOL32	   bLastBMS;
    BOOL32     bFound;
    BOOL32     bIsAgeOverflow;
    UINT16     nMaxAge;
    UINT16     nTmpMaxAge;
    UINT16     nCurAge;
    UINT32     nCurBMSOffset;
    UINT32     nTmpBMSOffset;
    UINT16     nRBIdx = 0;
    UINT8      nNumOfLoadBMI = 0;
    UINT8      nNumOfBMS = 0;
    INT32      bRet = FALSE32;    
    
    BBM_LOG_PRINT((TEXT("[TBBM: IN] ++_LoadBMS(nPDev:%d)\r\n"), pstDev->nDevNo));
    
    nPDev              = pstDev->nDevNo;
    pstRsv             = &(pstDev->stRsv);
    pstBMI             = &(pstRsv->stBMI);
    pstBms             = (bms *) (pstDev->pTmpMBuf);
    nSctsPerBMSG       = pstVol->nSctsPerBlk - SCTS_PER_PCH - SCTS_PER_PIA;
    
    if (nPCAType == TYPE_UPCA)
    {
        n1stPsnOfBMSG      = pstRsv->nUPcb[pstRsv->nCurUPCBIdx] * pstVol->nSctsPerBlk +
                        SCTS_PER_PCH + SCTS_PER_PIA;
    }
    else /* TYPE_LPCA */
    {
        n1stPsnOfBMSG      = pstRsv->nLPcb[pstRsv->nCurLPCBIdx] * pstVol->nSctsPerBlk +
                        SCTS_PER_PCH + SCTS_PER_PIA;
    }
    nMaxAge            = 0;
    nCurBMSOffset      = 0;
    nTmpMaxAge         = 0;
    nTmpBMSOffset      = 0;
    bFound             = FALSE32;
    bIsAgeOverflow     = FALSE32;
	bLastBMS		   = FALSE32;

    /**************************************************************
      [NOTE]
      In order to reduce the usage of STACK,
      first, search for a sector, which contains the latest BMS.
      second, read the sector, which contains the latest BMS.
    **************************************************************/
    nIdx1 = 0;
    while (nIdx1 < nSctsPerBMSG)
    {
        bValidBMS = TRUE32;
        for (nIdx2 = 0; nIdx2 < SCTS_PER_BMI; nIdx2 += NUM_OF_MIRRORS)
        {
            bRet = _ReadMetaData(n1stPsnOfBMSG + nIdx1 + nIdx2,
                              (UINT8 *) pstBms,
                              pstDev,
                              pstVol);
            if (bRet == FALSE32)
            {
                bValidBMS = FALSE32;
                break;
            }

            if ((pstBms->nInf != TYPE_INIT_CREATE)  &&
                (pstBms->nInf != TYPE_WRITE_ERR)    &&
                (pstBms->nInf != TYPE_ERASE_ERR)    &&
                (pstBms->nInf != TYPE_UPDATE_PIEXT))
            {
                bValidBMS = FALSE32;
                break;
            }
        }

        if (bValidBMS == FALSE32)
        {
            nIdx1 += SCTS_PER_BMI;
            continue;
        }

        nCurAge = pstBms->nAge;
        
        if (bIsAgeOverflow == FALSE32)
        {
            if (nCurAge == (UINT16) 0xFFFF)
            {
                /* if the vaue of nCurAge is overflowed,
                   retry to compare nCurAge with nMaxAge from beginning. */
                bIsAgeOverflow = TRUE32;
                nIdx1          = 0;
                continue;
            }
        }
    
        if (bIsAgeOverflow == FALSE32)
        {
            if ((UINT16) nTmpMaxAge < (UINT16) nCurAge)
            {
                nTmpBMSOffset = (UINT16) nIdx1;
                nTmpMaxAge    = nCurAge;
            }
        }
        else
        {
            /**********************************************************
            if the vaue of nCurAge is overflowed,
            retry to compare (INT16) nCurAge with
            (INT16) nMaxAge using signed short casting. 

            Ex)
            unsigned short casting   0xFFFE 0xFFFF 0x0000 0x0001 0x0002
              signed short casting       -2     -1      0      1      2

            In case of using unsigned short casting, the largest value
            is 0xFFFF. But in case of using signed short casting, the
            largest value is 2.
            **********************************************************/
            if ((INT16) nTmpMaxAge < (INT16) nCurAge)
            {
                nTmpBMSOffset = (UINT16) nIdx1;
                nTmpMaxAge    = nCurAge;
            }
        }

        nIdx1 += SCTS_PER_BMI;

        if (bRet == TRUE32)
        {
            nNumOfBMS = nIdx1 - nTmpBMSOffset;
            bFound = TRUE32;
            nCurBMSOffset = nTmpBMSOffset;
            nMaxAge =nTmpMaxAge;
        }
       
    }

    if (bFound == FALSE32)
    {
        BBM_ERR_PRINT((TEXT("[TBBM:ERR]   NO VALID BMS\r\n")));
        BBM_ERR_PRINT((TEXT("[TBBM:ERR]   <<CRITICAL ERROR>>\r\n")));

        BBM_LOG_PRINT((TEXT("[TBBM:OUT] --_LoadBMS(nPDev:%d)\r\n"), pstDev->nDevNo));
        return FALSE32;
    }

    pSrcBMF          = (bmf *) (pstBms->stBMF);
    pDstBMF          = (bmf *) (pstRsv->stBMI.pstBMF);

    for (nIdx1 = 0; nIdx1 < nNumOfBMS; nIdx1 += NUM_OF_MIRRORS)
    {
        /* There is no need to check the return-value of _ReadMetaData,
           because the return-value is already checked
           during search for latest BMS */
        _ReadMetaData(n1stPsnOfBMSG + nCurBMSOffset + nIdx1,
                      (UINT8 *) pstBms,
                      pstDev,
                      pstVol);    

        /* construct Block Map Information using BMS */
        for (nIdx2 = 0; nIdx2 < BMFS_PER_BMS; nIdx2++)
        {
            nRBIdx = pSrcBMF[nIdx2].nRbn;

            /* if nRBIdx is 0xFFFF, slot (nIdx2) is empty */
            if (nRBIdx == (UINT16) 0xFFFF && pSrcBMF[nIdx2].nSbn == (UINT16) 0xFFFF)
			{
				bLastBMS = TRUE32;
                break;
			}
            if (pstBMI->nNumOfBMFs >= (UINT16) (pstVol->nRsvrPerDev - RSV_META_BLKS))
            {
                BBM_ERR_PRINT((TEXT("[TBBM:ERR] pstBMI->nNumOfBMFs(%d) >= (UINT16) (pstVol->nRsvrPerDev(%d) - RSV_META_BLKS)\r\n"), 
                               pstBMI->nNumOfBMFs, pstVol->nRsvrPerDev));
                BBM_LOG_PRINT((TEXT("[TBBM:OUT] --_LoadBMS(nPDev:%d)\r\n"), pstDev->nDevNo));
                return FALSE32;
            }

            /* save Sbn and Rbn */
            pDstBMF[pstBMI->nNumOfBMFs].nSbn = pSrcBMF[nIdx2].nSbn;
            pDstBMF[pstBMI->nNumOfBMFs].nRbn = (UINT16) (nRBIdx + pstVol->n1stSbnOfRsvr);

            /* increase the number of BMFs */
            pstBMI->nNumOfBMFs++;
            nNumOfLoadBMI = (nIdx1 >> 2) + 1;

        }
        /* if nRBIdx is 0xFFFF, slot (nIdx2) is empty */
        if (bLastBMS == TRUE32)
        {
            break;
        }
    }

    if (nPCAType == TYPE_UPCA)
    {      
        /* in order to prevent program fail caused by soft program  POR scheme is applied during BML_Open */
        pstRsv->nNextUBMSOff = 0;
    
        /* store the age of UBMS */
        pstRsv->stBMI.nUAge = nMaxAge;

    }
    else /* TYPE_LPCA */
    {
        /* in order to prevent program fail caused by soft program  POR scheme is applied during BML_Open */
        pstRsv->nNextLBMSOff = 0;
    
        /* store the age of UBMS */
        pstRsv->stBMI.nLAge = nMaxAge;
      
    }
    BBM_LOG_PRINT((TEXT("[TBBM:OUT] --_LoadBMS(nPDev:%d)\r\n"), pstDev->nDevNo));

    return TRUE32;
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      _LoadPI                                                              */
/* DESCRIPTION                                                               */
/*      This function loads PI from PIA in LPCA                              */
/*                                                                           */
/* PARAMETERS                                                                */
/*      pstDev       [IN]                                                    */
/*           Pointer to flash_dev                                             */
/*      pstVol       [IN]                                                    */
/*           Pointer to flash_vol                                             */
/*      pstPI        [OUT]                                                   */
/*           part_info structure pointer                                      */
/* RETURN VALUES                                                             */
/*      If _LoadPI is completed, it returns TRUE32.                          */
/*      Otherwise, it returns FALSE32.                                       */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
static BOOL32
_LoadPI(flash_dev *pstDev, flash_vol *pstVol, part_info *pstPI)
{
    BOOL32     bRe = TRUE32;
    UINT8     *pMBuf;

    BBM_LOG_PRINT((TEXT("[TBBM: IN] ++_LoadPI(nPDev:%d)\r\n"), pstDev->nDevNo));

    pMBuf = pstDev->pTmpMBuf;
    
    if (pstPI != NULL)
    {
        /* load PI */
        if (_ReadMetaData(pstDev->stRsv.nLPcb[0] * pstVol->nSctsPerBlk + SCTS_PER_PCH,
                        (UINT8 *) pMBuf,
                        pstDev,
                        pstVol) == FALSE32)
        {
            BBM_ERR_PRINT((TEXT("[TBBM:ERR]   NO VALID Partition Information\r\n")));
            BBM_ERR_PRINT((TEXT("[TBBM:ERR]   <<CRITICAL ERROR>>\r\n")));
            bRe = FALSE32;
        }

        AD_Memcpy(pstPI, pMBuf, sizeof(part_info));

    }

    BBM_LOG_PRINT((TEXT("[TBBM:OUT] --_LoadPI(nPDev:%d,bRe=%d)\r\n"), pstDev->nDevNo, bRe));

    return bRe;
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      BBM_RecalAllocBlkPtr                                                 */
/* DESCRIPTION                                                               */
/*      This function reallocates allocated block pointer of locked area and */
/*      unlocked area (pstRsv->nAllocBlkPtrU, pstRsv->nAllocBlkPtrL)         */
/*                                                                           */
/* PARAMETERS                                                                */
/*      nPDev        [IN]                                                    */
/*            physical device number                                         */
/* RETURN VALUES                                                             */
/*      none                                                                 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
VOID
BBM_RecalAllocBlkPtr(flash_dev *pstDev)
{
    UINT32     nIdx1;
    reservoir *pstRsv;
    bmf       *pstBMF;


    BBM_LOG_PRINT((TEXT("[TBBM: IN] ++BBM_RecalAllocBlkPtr(nPDev:%d)\r\n"), 
                   pstDev->nDevNo));
	
	
    pstRsv = &(pstDev->stRsv);
    pstBMF = pstRsv->stBMI.pstBMF;

    if (pstRsv->nLPcb[0] < pstRsv->nLPcb[1])
    {
        if (pstRsv->nAllocBlkPtrL > pstRsv->nLPcb[0])
        {
            pstRsv->nAllocBlkPtrL = pstRsv->nLPcb[0];
        }        
    }
    else
    {
        if (pstRsv->nAllocBlkPtrL > pstRsv->nLPcb[1])
        {
            pstRsv->nAllocBlkPtrL = pstRsv->nLPcb[1];
        }        
    }
    
    if (pstRsv->nUPcb[0] < pstRsv->nUPcb[1])
    {
        if (pstRsv->nAllocBlkPtrU < pstRsv->nUPcb[1])
        {
            pstRsv->nAllocBlkPtrU = pstRsv->nUPcb[1];
        }        
    }
    else
    {
        if (pstRsv->nAllocBlkPtrU < pstRsv->nUPcb[0])
        {
            pstRsv->nAllocBlkPtrU = pstRsv->nUPcb[0];
        }        
    }
    
    for (nIdx1 = 0; nIdx1 < pstRsv->stBMI.nNumOfBMFs; nIdx1++)
    {
        if (pstBMF[nIdx1].nSbn == (UINT16) 0xFFFF)
        {
            break;
        }
        if ((pstDev->n1stSbnOfULArea <= pstBMF[nIdx1].nSbn) &&
            (pstRsv->nAllocBlkPtrU   < pstBMF[nIdx1].nRbn))
        {
        	if (pstRsv->nAllocBlkPtrL <= pstBMF[nIdx1].nSbn) continue;
        	
            pstRsv->nAllocBlkPtrU = pstBMF[nIdx1].nRbn;
        }
        
        if ((pstDev->n1stSbnOfULArea > pstBMF[nIdx1].nSbn) &&
            (pstRsv->nAllocBlkPtrL   > pstBMF[nIdx1].nRbn))
        {
            pstRsv->nAllocBlkPtrL = pstBMF[nIdx1].nRbn;
        }        
    }

    BBM_INF_PRINT((TEXT("[TBBM:   ]   pstRsv->nAllocBlkPtrL:%d\r\n"), 
        pstRsv->nAllocBlkPtrL));
    BBM_INF_PRINT((TEXT("[TBBM:   ]   pstRsv->nAllocBlkPtrU:%d\r\n"), 
        pstRsv->nAllocBlkPtrU));

    BBM_LOG_PRINT((TEXT("[TBBM:OUT] --BBM_RecalAllocBlkPtr(nPDev:%d)\r\n"), 
        pstDev->nDevNo));
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      _SortBMI                                                             */
/* DESCRIPTION                                                               */
/*      This function sorts by ascending power of pstBMI->pstBMF[x].nSbn     */
/* PARAMETERS                                                                */
/*      pstBMI        [IN]                                                   */
/*            BMI structure pointer                                          */
/* RETURN VALUES                                                             */
/*      none                                                                 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
static VOID
_SortBMI(bmi *pstBMI)
{
    UINT32  nIdx1;
    UINT32  nIdx2;
    UINT16  nMinSbn;
    bmf     stBmf;

    /* Sorts by ascending power of pstBMI->pstBMF[x].nSbn */
    BBM_INF_PRINT((TEXT("[TBBM:MSG] ** Sorts by ascending power of pstBMI->pstBMF[].nSbn\r\n")));

    for (nIdx1 = 0; nIdx1 < pstBMI->nNumOfBMFs; nIdx1++)
    {
        nMinSbn = pstBMI->pstBMF[nIdx1].nSbn;

        for (nIdx2 = nIdx1 + 1; nIdx2 < pstBMI->nNumOfBMFs; nIdx2++)
        {
            if (nMinSbn > pstBMI->pstBMF[nIdx2].nSbn)
            {
                nMinSbn               = pstBMI->pstBMF[nIdx2].nSbn;

                stBmf                 = pstBMI->pstBMF[nIdx1];
                pstBMI->pstBMF[nIdx1] = pstBMI->pstBMF[nIdx2];
                pstBMI->pstBMF[nIdx2] = stBmf;
            }
        }
    }
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      _SetAllocRB                                                          */
/* DESCRIPTION                                                               */
/*      This function makes the allocation status of the given nPbn          */
/*      to the allocated status                                              */
/* PARAMETERS                                                                */
/*      flash_dev   [IN]                                                      */
/*            flash_dev structure pointer                                     */
/*      flash_vol   [IN]                                                      */
/*            flash_vol structure pointer                                     */
/*      nPbn       [IN]                                                      */
/*            physical block number                                          */
/* RETURN VALUES                                                             */
/*      none                                                                 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
static VOID
_SetAllocRB(flash_dev *pstDev, flash_vol *pstVol, UINT32 nPbn)
{
    UINT32  nRBIdx;

    /* get reserved block index */
    nRBIdx = nPbn - pstVol->n1stSbnOfRsvr;

    /* set the bit of Block Allocation BitMap as 1 */
    pstDev->stRsv.pBABitMap[nRBIdx / 8] |= (UINT8) (0x80 >> (nRBIdx % 8));
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      _PrintPI                                                             */
/* DESCRIPTION                                                               */
/*      This function prints partition information                           */
/* PARAMETERS                                                                */
/*      pstPI        [IN]                                                    */
/*            part_info structure pointer                                     */
/* RETURN VALUES                                                             */
/*      none                                                                 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
static VOID
_PrintPI(part_info *pstPI)
{   
    UINT32  nIdx1;

    if (pstPI == NULL)
    {
        return;
    }
      
    BBM_INF_PRINT((TEXT("[TBBM:MSG]  << PARTITION INFORMATION >>\r\n")));    
    
    for (nIdx1 = 0; nIdx1 < pstPI->nNumOfPartEntry; nIdx1++)
    {
        BBM_INF_PRINT((TEXT("[TBBM:   ]  nID        : 0x%x\r\n"), 
                       pstPI->stPEntry[nIdx1].nID));

        switch (pstPI->stPEntry[nIdx1].nAttr)
        {
        case (BML_PI_ATTR_FROZEN | BML_PI_ATTR_RO):
            BBM_INF_PRINT((TEXT("[TBBM:   ]  nAttr      : RO + FROZEN (0x%x)\r\n"),
                           pstPI->stPEntry[nIdx1].nAttr));
            break;
        case BML_PI_ATTR_RO:
            BBM_INF_PRINT((TEXT("[TBBM:   ]  nAttr      : RO (0x%x)\r\n"),
                           pstPI->stPEntry[nIdx1].nAttr));
            break;
        case BML_PI_ATTR_RW:
            BBM_INF_PRINT((TEXT("[TBBM:   ]  nAttr      : RW (0x%x)\r\n"),
                           pstPI->stPEntry[nIdx1].nAttr));
            break;
        }

        BBM_INF_PRINT((TEXT("[TBBM:   ]  n1stVbn    : %d\r\n"),
                       pstPI->stPEntry[nIdx1].n1stVbn));
        BBM_INF_PRINT((TEXT("[TBBM:   ]  nNumOfBlks : %d\r\n"),
                       pstPI->stPEntry[nIdx1].nNumOfBlks));
        BBM_INF_PRINT((TEXT("[TBBM:   ]  ---------------------  \r\n")));
    }
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      BBM_PrintBMI                                                         */
/* DESCRIPTION                                                               */
/*      This function prints block mapping information                       */
/* PARAMETERS                                                                */
/*      pstVol        [IN]                                                   */
/*            pointer to flash_vol structure                                  */
/*      pstDev        [IN]                                                   */
/*            pointer to flash_dev structure                                  */
/* RETURN VALUES                                                             */
/*      none                                                                 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
VOID
BBM_PrintBMI(flash_vol *pstVol, flash_dev *pstDev)
{
    bmi       *pstBMI;
    UINT32     nIdx1;
    UINT32     nPgIdx;
    UINT8      aSBuf[XSR_SPARE_SIZE];
    UINT8      nBadMark = 0x00;

    pstBMI = &(pstDev->stRsv.stBMI);

    BBM_RTL_PRINT((TEXT("[TBBM:   ]  << DevNO:%d MAPPING INFORMATION >>\r\n"), pstDev->nDevNo));

    BBM_RTL_PRINT((TEXT("[TBBM:   ]   Bad Mark Information\r\n")));
    BBM_RTL_PRINT((TEXT("[TBBM:   ]      - Bad Mark (0x%02x) by write error\r\n"), BADMARKV_WRITEERR));
    BBM_RTL_PRINT((TEXT("[TBBM:   ]      - Bad Mark (0x%02x) by erase error\r\n"), BADMARKV_ERASEERR));

    BBM_RTL_PRINT((TEXT("[TBBM:   ]   pstDev->n1stSbnOfULArea = %d\r\n"), pstDev->n1stSbnOfULArea));

	AD_Memset(aSBuf, 0xFF, XSR_SPARE_SIZE);
	
    for (nIdx1 = 0; nIdx1 < pstBMI->nNumOfBMFs; nIdx1++)
    {
        for (nPgIdx = 0; nPgIdx < 2; nPgIdx++)
        {
            /* read bad mark */
            _LLDRead(pstVol,
                     pstDev->nDevNo,
                     pstBMI->pstBMF[nIdx1].nSbn * pstVol->nSctsPerBlk + nPgIdx * pstVol->nSctsPerPg,
                     1,
                     NULL,
                     aSBuf,
                     LLD_FLAG_ECC_OFF);

            nBadMark = aSBuf[pstVol->nBadPos];

            if ((nBadMark == 0x00)              ||
                (nBadMark == BADMARKV_WRITEERR) ||
                (nBadMark == BADMARKV_ERASEERR))
            {
                break;
            }
        }

        if (pstDev->n1stSbnOfULArea <= pstBMI->pstBMF[nIdx1].nSbn)
        {
            BBM_RTL_PRINT((TEXT("[TBBM:   ]   %03d: Sbn[%4d] ==> Rbn[%4d] / UnLocked / BadMark:0x%02x\r\n"),
                           nIdx1,
                           pstBMI->pstBMF[nIdx1].nSbn,
                           pstBMI->pstBMF[nIdx1].nRbn,
                           nBadMark));
        }
        else
        {
            BBM_RTL_PRINT((TEXT("[TBBM:   ]   %03d: Sbn[%4d] ==> Rbn[%4d] / Locked   / BadMark:0x%02x\r\n"),
                           nIdx1,
                           pstBMI->pstBMF[nIdx1].nSbn,
                           pstBMI->pstBMF[nIdx1].nRbn,
                           nBadMark));
        }
    }   

    BBM_RTL_PRINT((TEXT("[TBBM:   ]   << Total : %d BAD-MAPPING INFORMATION >>\r\n"), pstBMI->nNumOfBMFs));
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      _ChkBMIValidity                                                      */
/* DESCRIPTION                                                               */
/*      This function checks the validity of BMI.                            */
/* PARAMETERS                                                                */
/*      pstVol      [IN]                                                     */
/*            pointer to flash_vol structure                                  */
/*      pstRsv      [IN]                                                     */
/*            pointer to reservoir structure                                 */
/* RETURN VALUES                                                             */
/*      TRUE32                                                               */
/*      FALSE32                                                              */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
static BOOL32
_ChkBMIValidity(flash_vol *pstVol, reservoir *pstRsv)
{
    UINT32 nIdx;
    bmf   *pBmf;
    bmi   *pBMI;

    pBMI = &(pstRsv->stBMI);
    pBmf = pBMI->pstBMF;

    for (nIdx = 0; nIdx < pBMI->nNumOfBMFs; nIdx++)
    {
        if (pBmf[nIdx].nSbn >= pstVol->nBlksPerDev)
        {
            BBM_ERR_PRINT((TEXT("[TBBM:ERR]   nSbn(%d) >= pstDev->nNumOfBlks(%d)\r\n"),
                           pBmf[nIdx].nSbn, pstVol->nBlksPerDev));
            return FALSE32;
        }

        if ((pBmf[nIdx].nRbn >= pstVol->nBlksPerDev) ||
            (pBmf[nIdx].nRbn <  pstVol->n1stSbnOfRsvr + RSV_PCB_OFFSET))
        {
            BBM_ERR_PRINT((TEXT("[TBBM:ERR]   nRbn(%d) >= pstDev->nBlksPerDev(%d)\r\n"),
                           pBmf[nIdx].nRbn, pstVol->nBlksPerDev));
            BBM_ERR_PRINT((TEXT("[TBBM:ERR]   nRbn(%d) <  pstDev->n1stSbnOfRsvr + RSV_PCB_OFFSET(%d)\r\n"),
                           pBmf[nIdx].nRbn, pstVol->n1stSbnOfRsvr + RSV_PCB_OFFSET));
            return FALSE32;
        }

        if ((pBmf[nIdx].nRbn == pstRsv->nUPcb[0]) ||
            (pBmf[nIdx].nRbn == pstRsv->nUPcb[1]))
        {
            if (pBmf[nIdx].nSbn < pstVol->n1stSbnOfRsvr)
            {
                BBM_ERR_PRINT((TEXT("[TBBM:ERR]   nRbn(%d) == pstRsv->nUPcb[0]\r\n"),
                               pBmf[nIdx].nRbn, pstRsv->nUPcb[0]));
                BBM_ERR_PRINT((TEXT("[TBBM:ERR]   nSbn(%d) <  pstDev->n1stSbnOfRsvr(%d)\r\n"),
                               pBmf[nIdx].nSbn, pstVol->n1stSbnOfRsvr));

                return FALSE32;
            }
        }

        if ((pBmf[nIdx].nSbn == pstRsv->nUPcb[0]) ||
            (pBmf[nIdx].nSbn == pstRsv->nUPcb[1]))
        {
            BBM_ERR_PRINT((TEXT("[TBBM:ERR]   nSbn(%d) == pstRsv->nUPcb[0]\r\n"),
                           pBmf[nIdx].nSbn, pstRsv->nUPcb[0]));
            BBM_ERR_PRINT((TEXT("[TBBM:ERR]   nSbn(%d) == pstRsv->nUPcb[1]\r\n"),
                           pBmf[nIdx].nSbn, pstRsv->nUPcb[1]));

            return FALSE32;
        }

        if ((pBmf[nIdx].nRbn == pstRsv->nLPcb[0]))
        {
            if (pBmf[nIdx].nSbn < pstVol->n1stSbnOfRsvr)
            {
                BBM_ERR_PRINT((TEXT("[TBBM:ERR]   nRbn(%d) == pstRsv->nLPcb[0]\r\n"),
                               pBmf[nIdx].nRbn, pstRsv->nLPcb[0]));
                BBM_ERR_PRINT((TEXT("[TBBM:ERR]   nSbn(%d) <  pstDev->n1stSbnOfRsvr(%d)\r\n"),
                               pBmf[nIdx].nSbn, pstVol->n1stSbnOfRsvr));

                return FALSE32;
            }
        }

        if (pBmf[nIdx].nSbn == pstRsv->nLPcb[0])
        {
            BBM_ERR_PRINT((TEXT("[TBBM:ERR]   nSbn(%d) == pstRsv->nLPcb[0]\r\n"),
                           pBmf[nIdx].nSbn, pstRsv->nLPcb[0]));
            return FALSE32;
        }
    }

    return TRUE32;
}

static VOID
_ReconstructBUMap(flash_vol *pstVol, reservoir *pstRsv)
{
    UINT32 nIdx;
    UINT32 nBUIdx;
    UINT32 nMallocSize;


    nMallocSize = (pstVol->nBlksPerDev > BLKS_PER_BADUNIT) ? pstVol->nBlksPerDev / BLKS_PER_BADUNIT : 1;
    AD_Memset(pstRsv->pBUMap, 0x00, sizeof(badunit) * nMallocSize);

    /* pstBMI->pstBMF[].nSbn should be sorted by ascending power */
    for (nIdx = 0; nIdx < pstRsv->stBMI.nNumOfBMFs; nIdx++)
    {
        nBUIdx = pstRsv->stBMI.pstBMF[nIdx].nSbn / BLKS_PER_BADUNIT;

        if (pstRsv->pBUMap[nBUIdx].nNumOfBMFs == 0)
            pstRsv->pBUMap[nBUIdx].n1stBMFIdx = (UINT16) nIdx;

        pstRsv->pBUMap[nBUIdx].nNumOfBMFs++;
    }
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      BBM_Mount                                                            */
/* DESCRIPTION                                                               */
/*      This function mounts reservoir of the given (nPDev) device.          */
/*      Actually, it search for latest LPCB, UPCB. And load PI and PIExt.    */
/*      Then, it loads the latest UBMI and LBMI.                             */
/*                                                                           */
/* PARAMETERS                                                                */
/*      pstVol      [IN]                                                     */
/*            pointer to flash_vol structure                                  */
/*      pstDev      [IN]                                                     */
/*            pointer to flash_dev structure                                  */
/*      pstPI       [OUT]                                                    */
/*            part_info structure pointer. If it is NULL, PI isn't loaded     */
/*      pstPExt     [OUT]                                                    */
/*            part_exinfo structure pointer. If it is NULL, PIExt isn't loaded  */
/* RETURN VALUES                                                             */
/*      BBM_SUCCESS                                                          */
/*      BBM_NO_LPCA                                                          */
/*      BBM_NO_UPCA                                                          */
/*      BBM_LOAD_PI_FAILURE                                                  */
/*      BBM_LOAD_LBMS_FAILURE                                                */
/*      BBM_LOAD_UBMS_FAILURE                                                */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
INT32
BBM_Mount(flash_vol *pstVol, flash_dev *pstDev, part_info *pstPI, part_exinfo *pstPExt)
{
    UINT32      nIdx;
    reservoir  *pstRsv;
    INT32       nBBMRe;


    BBM_LOG_PRINT((TEXT("[TBBM: IN] ++BBM_Mount(nPDev:%d,pstPI=0x%x,pstPExt=0x%x)\r\n"),
        pstDev->nDevNo, pstPI, pstPExt));

    pstRsv  = &(pstDev->stRsv);

    /* initializes the variables of reservoir structure */
    _Init(pstDev, pstVol);

    /* In order to search latest UPCA and latest LPCA, scan reservoir */
    nBBMRe = _ScanReservoir(pstDev, pstVol);
    /* If there is no LPCA or no UPCA,
       it is unformated status or critical error */
    if (nBBMRe != BBM_SUCCESS)
    {
        BBM_LOG_PRINT((TEXT("[TBBM:OUT] --BBM_Mount(nPDev:%d,pstPI=0x%x,pstPExt=0x%x)\r\n"),
            pstDev->nDevNo, pstPI, pstPExt));

        return nBBMRe;
    }

    /* load PI from LPCA */
    if (_LoadPI(pstDev, pstVol, pstPI) != TRUE32)
    {
        BBM_ERR_PRINT((TEXT("[TBBM:ERR]   _LoadPI is failed\r\n")));

        BBM_LOG_PRINT((TEXT("[TBBM:OUT] --BBM_Mount(nPDev:%d,pstPI=0x%x,pstPExt=0x%x)\r\n"),
            pstDev->nDevNo, pstPI, pstPExt));

        return BBM_LOAD_PI_FAILURE;
    }

    /* load LBMS from LPCA */
    if (_LoadBMS(pstDev, pstVol, TYPE_LPCA) != TRUE32)
    {
        BBM_ERR_PRINT((TEXT("[TBBM:ERR]   _LoadLBMS is failed\r\n")));

        BBM_LOG_PRINT((TEXT("[TBBM:OUT] --BBM_Mount(nPDev:%d,pstPI=0x%x,pstPExt=0x%x)\r\n"),
            pstDev->nDevNo, pstPI, pstPExt));

        return BBM_LOAD_LBMS_FAILURE;
    }

    /* load PIExt from UPCA */
    _LoadPIExt(pstDev, pstVol, pstPExt);

    /* load UBMS from UPCA */
    if (_LoadBMS(pstDev, pstVol, TYPE_UPCA) != TRUE32)
    {
        BBM_ERR_PRINT((TEXT("[TBBM:ERR]   _LoadUBMS is failed\r\n")));

        BBM_LOG_PRINT((TEXT("[TBBM:OUT] --BBM_Mount(nPDev:%d,pstPI=0x%x,pstPExt=0x%x)\r\n"),
            pstDev->nDevNo, pstPI, pstPExt));

        return BBM_LOAD_UBMS_FAILURE;
    }

    if (_ChkBMIValidity(pstVol, pstRsv) == FALSE32)
    {
        BBM_ERR_PRINT((TEXT("[TBBM:ERR]   _ChkBMIValidity is failed\r\n")));

        BBM_LOG_PRINT((TEXT("[TBBM:OUT] --BBM_Mount(nPDev:%d)\r\n"),
            pstDev->nDevNo));

        return BBM_LOAD_UBMS_FAILURE;
    }

    /* sorts by ascending power of pstBMI->pstBMF[].nSbn */
    _SortBMI(&(pstRsv->stBMI));


    BBM_INF_PRINT((TEXT("[TBBM:   ]   constructing reserved Block Allocation BitMAP\r\n")));

    /* construct pBABitMap using pBmi->pstBMF[nIdx].nRbn */
    for (nIdx = 0; nIdx < pstRsv->stBMI.nNumOfBMFs; nIdx++)
    {
        _SetAllocRB(pstDev, pstVol, pstRsv->stBMI.pstBMF[nIdx].nRbn);
    }
    
    /* construct pBABitMap using initial bad blocks */
    for (nIdx = pstVol->n1stSbnOfRsvr; nIdx < pstVol->nBlksPerDev; nIdx++)
    {
        if (pstVol->lld_check_bad(pstDev->nDevNo, nIdx) == LLD_INIT_GOODBLOCK)
            continue;

        _SetAllocRB(pstDev, pstVol, nIdx);
    }

    /* construct pBABitMap using PCB */
    _SetAllocRB(pstDev, pstVol, pstVol->n1stSbnOfRsvr);  /* for ERL block */
    _SetAllocRB(pstDev, pstVol, pstVol->n1stSbnOfRsvr + 1); /* for REF block */
    _SetAllocRB(pstDev, pstVol, pstRsv->nLPcb[0]);
    _SetAllocRB(pstDev, pstVol, pstRsv->nLPcb[1]);
    _SetAllocRB(pstDev, pstVol, pstRsv->nUPcb[0]);
    _SetAllocRB(pstDev, pstVol, pstRsv->nUPcb[1]);

    BBM_INF_PRINT((TEXT("[TBBM:   ]   constructing is completed\r\n")));

    _ReconstructBUMap(pstVol, pstRsv);

    BBM_INF_PRINT((TEXT("[TBBM:   ]   constructing bad unit map is completed\r\n")));

    /* print partition information */
    _PrintPI(pstPI);

    BBM_LOG_PRINT((TEXT("[TBBM:OUT] --BBM_Mount(nPDev:%d,pstPI=0x%x,pstPExt=0x%x)\r\n"),
        pstDev->nDevNo, pstPI, pstPExt));

    return BBM_SUCCESS;
}

