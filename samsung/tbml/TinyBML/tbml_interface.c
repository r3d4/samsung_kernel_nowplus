/*****************************************************************************/
/*                                                                           */
/* PROJECT : AnyStore II                                                     */
/* MODULE  : XSR BML                                                         */
/* NAME    : BML Interface                                                   */
/* FILE    : BMLInterface.cpp                                                */
/* PURPOSE : This file contains the exported routine for interfacing with    */
/*           the upper layer of BML.                                         */
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
/*   15-OCT-2003 [SongHo Yoon]: Bug fixed in tbml_read()                      */
/*   11-NOV-2003 [Jaesung Jung]: added FiLM inferface MACRO                  */
/*   09-DEC-2003 [SongHo Yoon]: removed _HandleFILPrevProgErr()              */
/*                                      _HandleFILProgErr()                  */
/*                                      _HandleFILReadErr()                  */
/*   09-DEC-2003 [SongHo Yoon]: added   _HandlePrevWrErr()                   */
/*                                      _HandlePrevErErr()                   */
/*   15-DEC-2003 [SongHo Yoon]: removed BML_ProtectROArea()                  */
/*   18-DEC-2003 [SongHo Yoon]: fixed bug in BML_CopyBack()                  */
/*   06-JAN-2003 [SongHo Yoon]: changed previous error handling routine      */
/*   12-FEB-2004 [SongHo Yoon]: fixed bug in tbml_init()                      */
/*                           tbml_init doesn't return BML_ALREADY_INITIALIZED */
/*                           when tbml_init is called 2 times.                */
/*   23-MAR-2004 [SongHo Yoon]: merged from FlashVolMgr.cpp to optimize code */
/*   30-MAR-2004 [SongHo Yoon]: fixed a bug in _RandomIn()                   */
/*   21-APR-2004 [SongHo Yoon]: added BML_Copy()                             */
/*   06-MAY-2004 [SongHo Yoon]: removed BML_GetPlaneNum()                    */
/*   07-MAY-2004 [SongHo Yoon]: added tbml_mread()/BML_MWrite                 */
/*   10-MAY-2004 [SongHo Yoon]: added S/W ECC module from FIL                */
/*   08-JUN-2004 [SongHo Yoon]: fixed a bug of tbml_ioctl in case that nCode  */
/*                              is BML_IOCTL_UNLOCK_WHOLEAREA                */
/*   11-JUN-2004 [Janghwan Kim]: _ChkSWECC bug fix                           */
/*   02-SEP-2004 [SeWook Na]: added BML_MErasBlk()                           */
/*   06-SEP-2004 [Janghwan Kim]: modified BML_MErasBlk()                     */
/*   07-SEP-2004 [Janghwan Kim]: in BML_MWrite(), addded code for flush      */
/*                               operation performing for all devices        */
/*                               of volume.                                  */
/*   07-SEP-2004 [Janghwan Kim]: modified error checking code of             */
/*                               LLD_MWrite() in BML_MWrite()                */
/*   08-SEP-2004 [Janghwan Kim]: modified _Init(), _Open() code for MEList   */
/*                               structure instances                         */
/*   08-SEP-2004 [Janghwan Kim]: fixed the defect of BML_Copy()              */
/*                               In case of 2-bit corruption, illegal        */
/*                               bad-block handling can performs.            */
/*   10-SEP-2004 [Janghwan Kim]: added nEccPos in tbml_vol_spec structure       */
/*   07-OCT-2004 [Janghwan Kim]: fixed a bug of BML_Copy (random-in miss)    */
/*   16-NOV-2004 [Janghwan Kim]: changed BML_MEraseBlk() code to enable      */
/*                               deferred check operation.                   */
/*   16-NOV-2004 [Janghwan Kim]: addedd _HandlePrevMErErr()                  */
/*   02-FEB-2005 [MinYoung Kim]: fixed a defect of BML_Write() about         */
/*                               about previous Multi-block erase handling   */
/*   09-MAR-2005 [MinYoung Kim]: fixed a defect of BML_MEraseBlk() about     */
/*                               checking Read Only partition                */
/*   09-MAR-2005 [MinYoung Kim]: fixed a defect of BML_CopyBack() about      */
/*                               handling uncorrectable read error           */
/*   05-OCT-2005 [Younwon Park]: added tbml_sgl_read()/BML_SGLWrite            */
/*   04-NOV-2005 [Younwon Park]: modified _IsROPartition()                   */
/*   01-DEC-2005 [Younwon Park]: support three numbers of device in volume   */
/*   31-JAN-2006 [ByoungYoung Ahn] : moved PAM_Init() from tbml_open to       */
/*                                   tbml_init()                              */
/*   31-JAN-2006 [ByoungYoung Ahn] : added codes for refresh by erase scheme */
/*   31-JAN-2006 [ByoungYoung Ahn] : added tbml_get_dev_spec()                  */
/*   14-FEB-2006 [WooYoung Yang] : added BML_GetBBInfo() and _GetBBInfo      */
/*   15-JUN-2006 [Younwon Park]: fixed a defect of S/W ECC about byte_align  */
/*   18-OCT-2006 [Younwon Park]: fixed a defect of _HandlePrevMErErr() about */
/*                               previous Multi-block erase fail handling    */
/*                               in asynchronous mode                        */
/*   09-FEB-2007 [Younwon Park]: fixed a defect of _ChkSWECC about           */
/*                               read error code handling                    */
/*                               modified BML_Copy/BML_CopyBack code to      */
/*                               write with ECC_OFF in case of 2bit ecc error*/
/*                               occurs during reading                       */
/*                               changed from whole area unlock to reservoir */
/*                               area unlock during BML_Format()             */
/*  30-APR-2007 [Younwon Park] : modified tbml_sgl_read() to support read ahead*/
/*  30-APR-2007 [Younwon Park] : modified BML_SGLWrite() to support fully    */
/*                               dual buffering in multiple write operation  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/* Header file inclusions                                                    */
/*****************************************************************************/
#include <tbml_common.h>
#include <os_adapt.h>
#include <onenand_interface.h>
#include <onenand_lld.h>
#include <tbml_interface.h>
#include <tbml_type.h>
#include <gbbm.h>
#if defined(CONFIG_RFS_TINYBML)
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 19)
#include <linux/config.h>
#endif
#endif /* CONFIG_RFS_TINYBML */

/*****************************************************************************/
/* Debug Print #defines                                                      */
/*****************************************************************************/

/*****************************************************************************/
/* Macros                                                                    */
/*****************************************************************************/
#if defined(CHK_VOLUME_VALIDATION)
#define     CHK_VOL_RANGE(Vol)                                              \
    if (nVol >= XSR_MAX_VOL)                                                \
    {                                                                       \
        BIF_ERR_PRINT((TEXT("[BIF:ERR] volume(nVol:%d) is out of bound\r\n"), Vol));    \
        return BML_INVALID_PARAM;                                           \
    }

#define     CHK_VOL_OPEN(VolOpen)                                           \
    if (VolOpen != TRUE32)                                                  \
    {                                                                       \
        BIF_ERR_PRINT((TEXT("[BIF:ERR] volume(nVol:%d) isn't opened\r\n"), nVol));  \
        return BML_VOLUME_NOT_OPENED;                                       \
    }
#else /* CHK_VOLUME_VALIDATION */
#define     CHK_VOL_RANGE(Vol)
#define     CHK_VOL_OPEN(VolOpen)
#endif /* CHK_VOLUME_VALIDATION */

#define TRANS_VA_SA(Vol, Vsn, pVol, Vbn, SecOff, Sbn, PDev)                 \
        {                                                                   \
            Vbn    = Vsn >> pVol->nSftSctsPerBlk;                           \
            SecOff = Vsn &  (pVol->nSctsPerBlk - 1);                        \
            Sbn    = (pVol->nNumOfDev == 3) ? (Vbn / pVol->nSftnNumOfDev) : (Vbn >> pVol->nSftnNumOfDev); \
            PDev   = (pVol->nNumOfDev == 3) ? ((Vol << DEVS_PER_VOL_SHIFT) + (Vbn % pVol->nNumOfDev)) \
                                            : ((Vol << DEVS_PER_VOL_SHIFT) + (Vbn & (pVol->nNumOfDev - 1)));   \
        }

#define GET_PBN(nPbn, nSbNum, BUMAPPtr, BMIPtr)                             \
        {                                                                   \
            register UINT32   nIdx;                                         \
            register UINT32   nNumOFBMFs;                                   \
            register bmf     *pBMF;                                         \
            register badunit *pBUMap;                                       \
            register bmi     *pBMI;                                         \
            nPbn    = nSbNum;                                               \
            pBUMap  = BUMAPPtr;                                             \
            pBMI    = BMIPtr;                                               \
            pBUMap += (nSbNum >> SFT_BLKS_PER_BADUNIT);                     \
            if (pBUMap->nNumOfBMFs > 0)                                     \
            {                                                               \
                nNumOFBMFs = pBUMap->nNumOfBMFs;                            \
                pBMF       = (bmf *) pBMI->pstBMF + pBUMap->n1stBMFIdx;     \
                for (nIdx = 0; nIdx < nNumOFBMFs; nIdx++)                   \
                {                                                           \
                    if (pBMF->nSbn == nSbNum)                               \
                    {                                                       \
                        BIF_INF_PRINT((TEXT("[BIF:   ]   Sbn(%d) ==> Pbn(%d)\r\n"), nSbNum, pBMF->nRbn)); \
                        nPbn = pBMF->nRbn;                                  \
                        break;                                              \
                    }                                                       \
                    pBMF++;                                                 \
                }                                                           \
            }                                                               \
        }


/*****************************************************************************/
/* Static variables definitions                                              */
/*****************************************************************************/
PRIVATE flash_vol    gstVolCxt[XSR_MAX_VOL];
PRIVATE flash_dev   *pstDevCxt[XSR_MAX_DEV];
PRIVATE lld_ftable  gstLFT[XSR_MAX_VOL];


/*****************************************************************************/
/* Definitions                                                               */
/*****************************************************************************/
#define GET_VOLCXT(v)       (&gstVolCxt[v])
#define GET_DEVCXT(d)       (pstDevCxt[d])

/*****************************************************************************/
/* Static function prototypes                                                */
/*****************************************************************************/
PRIVATE BOOL32    _IsOpenedDev(UINT32 nPDev);

PRIVATE VOID      _TrimMainBuf      (UINT32 nWriteScts, SGL *pstSGL);

PRIVATE VOID      _CalPhyULArea     (UINT32    nVol,
                                    part_info  *pstPartI,
                                    UINT16     na1stSbnOfULA[DEVS_PER_VOL]);
PRIVATE VOID      _ProtectLockedArea(UINT32    nVol,
                                     BOOL32    bOn);
PRIVATE VOID      _RecalAllocBlkPtr (UINT32    nVol);
PRIVATE BOOL32    _MountRsvr        (UINT32    nVol,
                                     part_info *pstLoadingPI,
                                     part_exinfo *pstLoadingPExt);
PRIVATE UINT32    _GetPbn           (register UINT32    nSbn,
                                     register badunit  *pBUMap,
                                     register bmi      *pBMI);
PRIVATE BOOL32    _ChkLLDSpecValidity(LLDSpec *pstLLDSpec);
PRIVATE INT32     _Open             (UINT32    nVol);
PRIVATE INT32     _Close            (UINT32    nVol);

/*****************************************************************************/
/* Code Implementation                                                       */
/*****************************************************************************/
/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      _IsOpenedDev                                                         */
/* DESCRIPTION                                                               */
/*      This function checks whether the given device is valid or not.       */
/* PARAMETERS                                                                */
/*      nVol    volume number                                                */
/*      nDevIdx device index in volume                                       */
/* RETURN VALUES                                                             */
/*      If the given device is valid, it returns FALSE32. Otherwise, it      */
/*      returns FALSE32.                                                     */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
PRIVATE BOOL32
_IsOpenedDev(UINT32 nPDev)
{
    flash_dev *pstDev;

    pstDev = GET_DEVCXT(nPDev);
    if (pstDev == NULL)
        return FALSE32;

    return TRUE32;
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      _CalPhyULArea                                                        */
/* DESCRIPTION                                                               */
/*      This function calculates 1st virtual block number of Unlocked Area   */
/*      of the each physical device in the given volume                      */
/* PARAMETERS                                                                */
/*      nVol                        [IN]                                     */
/*            volume number                                                  */
/*      pstPartI                    [IN]                                     */
/*            pointer to partition Information                               */
/*      na1stSbnOfULA[DEVS_PER_VOL] [OUT]                                    */
/*            memory location for the first Sbn of Unlocked Area of each dev */
/* RETURN VALUES                                                             */
/*      none                                                                 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
PRIVATE VOID
_CalPhyULArea(UINT32    nVol,
              part_info *pstPartI,
              UINT16    na1stSbnOfULA[DEVS_PER_VOL])
{
    UINT32  nCnt;
    UINT32  nMin1stVbn;
    UINT32  nMaxLastVbn;
    UINT32  n1stVbn;
    UINT32  nLastVbn;
    UINT32  n1stVbnOfUnFzArea;

    register flash_vol      *pstVol;
    register flash_dev      *pstDev;
    register XSRPartEntry  *pstPEntry;

    BIF_LOG_PRINT((TEXT("[BIF: IN] ++_CalPhyULArea(nVol:%d)\r\n"), nVol));

    pstVol      = GET_VOLCXT(nVol);
    nMin1stVbn  = 0xFFFFFFFF;
    nMaxLastVbn = 0;
    pstPEntry   = &(pstPartI->stPEntry[0]);

    /* search for the minimum 1stVbn  in Unfrozen Area
       search for the maximum lastVbn in Frozen Area */
    for (nCnt = 0; nCnt < pstPartI->nNumOfPartEntry; nCnt++)
    {
        n1stVbn  = pstPEntry[nCnt].n1stVbn;
        nLastVbn = n1stVbn + pstPEntry[nCnt].nNumOfBlks - 1;

        if ((pstPEntry[nCnt].nAttr & BML_PI_ATTR_FROZEN) != 0)
        {
            /* part-entry has the frozen attribute,
               compare the maximum last vbn with
               the last vbn of the part entry
               to get the maximum last vbn among frozen partition */
            if (nMaxLastVbn < nLastVbn)
                nMaxLastVbn = nLastVbn;
        }
        else
        {
            /* part-entry dosen't has the frozen attribute,
               compare the minmum 1st vbn with
               the 1st vbn of the part entry
               to get the minimum 1st vbn among non-frozen partition */
            if (nMin1stVbn > n1stVbn)
                nMin1stVbn = n1stVbn;
        }
    }

    /* if there is no Unfrozen area,
       n1stVbnOfUnFrozenArea becomes the maximum lastVbn of Frozen Area */
    if (nMin1stVbn == 0xFFFFFFFF)
    {
        n1stVbnOfUnFzArea = nMaxLastVbn + 1;
    }
    /* if there is Unfrozen area,
       n1stVbnOfUnFrozenArea becomes the minimum 1stVbn of UnFrozen Area */
    else
    {
        n1stVbnOfUnFzArea = nMin1stVbn;
    }

    /* --------------------------------------------------------------------- */
    /* "n1stSbnOfULArea" vs "n1stVbnOfRWArea"                                */
    /* --------------------------------------                                */
    /*                                                                       */
    /* The relationship between "n1stSbnOfULArea" and "n1stVbnOfRWArea".     */
    /* We can assume that a cetain volume has 4 identical devices.           */
    /* Vbn(Virtual block number) and Pbn(Physical block number) can be       */
    /* organized as follows.                                                 */
    /*                                                                       */
    /*  ----  ----  ----  ----                                               */
    /*  [ 8]  [ 9]  [10]  [11]   <= Pbn #2    Legend                         */
    /*  ' 4'  ' 5'  [ 6]  [ 7]   <= Pbn #1    [ ] : Unfrozen(Unlocked) Area  */
    /*  ' 0'  ' 1'  ' 2'  ' 3'   <= Pbn #0    ' ' : Frozen  (Locked)   Area  */
    /*  ----  ----  ----  ----                                               */
    /*  ^     ^     ^     ^                                                  */
    /*  |     |     |     |                                                  */
    /*  VD#0  VD#1  VD#2  VD#3   (VD : Virtual Device)                       */
    /*                                                                       */
    /*  In view-point of virtual address,                                    */
    /*  If n1stVbnOfRWArea is #6, the area (Vbn#0 ~ Vbn#5) is Frozen(Locked) */
    /*  and the area (Vbn#6 ~ Vbn#11) is Unfrozen(Unlocked) area.            */
    /*                                                                       */
    /*  In view-point of VD#0,                                               */
    /*  pstDevCxt[VD#0]->n1stSbnOfULArea is "Pbn#2"                          */
    /*                                                                       */
    /*  In view-point of VD#1,                                               */
    /*  pstDevCxt[VD#0]->n1stSbnOfULArea is "Pbn#2"                          */
    /*                                                                       */
    /*  In view-point of VD#2,                                               */
    /*  pstDevCxt[VD#0]->n1stSbnOfULArea is "Pbn#1"                          */
    /*                                                                       */
    /*  In view-point of VD#3,                                               */
    /*  pstDevCxt[VD#0]->n1stSbnOfULArea is "Pbn#1"                          */
    /*                                                                       */
    /*  If (virtual device #X) is greater than                               */
    /*     (n1stVbnOfRWArea % nNumOfDev per volume),                         */
    /*      n1stSbnOfULArea is n1stVbnOfLV1Area / nNumOfDev per volume.      */
    /*  Else                                                                 */
    /*      n1stSbnOfULArea is n1stVbnOfLV1Area / nNumOfDev per volume + 1.  */
    /*                                                                       */
    /* --------------------------------------------------------------------- */
    for (nCnt = 0; nCnt < DEVS_PER_VOL; nCnt++)
    {
        pstDev = GET_DEVCXT(nVol * DEVS_PER_VOL + nCnt);
        if (pstDev == NULL)
            continue;

        if ((n1stVbnOfUnFzArea % pstVol->nNumOfDev) <= nCnt)
        {
            na1stSbnOfULA[nCnt] = (UINT16)(n1stVbnOfUnFzArea / pstVol->nNumOfDev);
        }
        else
        {
            na1stSbnOfULA[nCnt] = (UINT16)(n1stVbnOfUnFzArea / pstVol->nNumOfDev + 1);

            if (pstVol->n1stSbnOfRsvr < na1stSbnOfULA[nCnt])
            {
                na1stSbnOfULA[nCnt] = pstVol->n1stSbnOfRsvr;
            }
        }

        pstDev->n1stSbnOfULArea = na1stSbnOfULA[nCnt];

        BIF_INF_PRINT((TEXT("[BIF:MSG]  Dev#%d : n1stSbnOfULArea=SBlk#%d\r\n"),
                      nCnt, pstDev->n1stSbnOfULArea));
    }

    BIF_LOG_PRINT((TEXT("[BIF:OUT] --_CalPhyULArea(nVol:%d)\r\n"), nVol));
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      _ProtectLockedArea                                                   */
/* DESCRIPTION                                                               */
/*      This function sets the partial area of volume to Locked Area by      */
/*      Write Protection Scheme to protect system data such as OS Image.     */
/* PARAMETERS                                                                */
/*      nVol       [IN]                                                      */
/*            volume number                                                  */
/*      bOn        [IN]                                                      */
/*       If bOn is TRUE32, sets the partial area of the volume to            */
/*       Read Only Area. Otherwise, sets the whole area of the               */
/*       volume to Read/Write Area.                                          */
/* RETURN VALUES                                                             */
/*       none                                                                */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
PRIVATE VOID
_ProtectLockedArea(UINT32 nVol, BOOL32 bOn)
{
    UINT32    nCnt;
    UINT32    nPDev;
    flash_vol *pstVol;
    flash_dev *pstDev;

    BIF_LOG_PRINT((TEXT("[BIF: IN] ++_ProtectLockedArea(nVol=%d)\r\n"), nVol));

    pstVol = GET_VOLCXT(nVol);

    for (nCnt = 0; nCnt < DEVS_PER_VOL; nCnt++)
    {
        nPDev = nVol * DEVS_PER_VOL + nCnt;

        if (_IsOpenedDev(nPDev) == FALSE32)
            continue;

        pstDev = GET_DEVCXT(nPDev);
        if (bOn == TRUE32)
        {
            /* Unlocked area of device is set as R/W area */
            pstDev->n1stPbnOfULArea = pstDev->n1stSbnOfULArea;
            pstDev->nBlksInULArea   = (UINT16)(pstDev->stRsv.nAllocBlkPtrL -
                                               pstDev->n1stSbnOfULArea);
        }
        else /* if (bOn == FALSE32) */
        {
            /* Whole area of device is set as R/W area */
            pstDev->n1stPbnOfULArea = pstVol->n1stSbnOfRsvr;
            pstDev->nBlksInULArea   = pstVol->nRsvrPerDev;
        }

        BIF_INF_PRINT((TEXT("[BIF:MSG]  Dev=%d is unlocked (Blk#%d ~ Blk#%d)\r\n"),
                       nPDev, pstDev->n1stPbnOfULArea, pstDev->nBlksInULArea));

        pstVol->lld_set_rw(nPDev, pstDev->n1stPbnOfULArea, pstDev->nBlksInULArea);
    }

    BIF_LOG_PRINT((TEXT("[BIF:OUT] --_ProtectLockedArea(nVol=%d)\r\n"), nVol));
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      _RecalAllocBlkPtr                                                    */
/* DESCRIPTION                                                               */
/*      This function recalculates the allocated block pointer in each device*/
/* PARAMETERS                                                                */
/*      nVol       [IN]                                                      */
/*          volume number                                                    */
/* RETURN VALUES                                                             */
/*      none                                                                 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
PRIVATE VOID
_RecalAllocBlkPtr(UINT32 nVol)
{
    UINT32    nCnt;
    UINT32    nPDev;

    flash_dev *pstDev;

    for (nCnt = 0; nCnt < DEVS_PER_VOL; nCnt++)
    {
        nPDev = nVol * DEVS_PER_VOL + nCnt;

        if (_IsOpenedDev(nPDev) == FALSE32)
            continue;

        pstDev = GET_DEVCXT(nPDev);

        BBM_RecalAllocBlkPtr(pstDev);
    }
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*    _MountRsvr                                                             */
/* DESCRIPTION                                                               */
/*    This function mounts reservoir of device in the given volume           */
/* PARAMETERS                                                                */
/*      nVol       [IN]                                                      */
/*            volume number                                                  */
/*      pstLoadingPI   [OUT]                                                 */
/*            part_info structure pointer                                     */
/*      pstLoadingPExt [OUT]                                                 */
/*            part_exinfo structure pointer                                     */
/* RETURN VALUES                                                             */
/*     TRUE32                                                                */
/*            FVM_MountRsvr is completed                                     */
/*     FALSE32                                                               */
/*            FVM_MountRsvr is failed                                        */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
PRIVATE BOOL32
_MountRsvr(UINT32 nVol, part_info *pstLoadingPI, part_exinfo *pstLoadingPExt)
{
    BOOL32    bBMLRe;
    UINT32    nDevIdx;
    UINT32    nPDev = 0;

    flash_vol *pstVol;
    flash_dev *pstDev;
    part_info *pstPI;
    part_exinfo *pstPExt;

    BIF_LOG_PRINT((TEXT("[BIF: IN] ++_MountRsvr(nPDev:%d)\r\n"), nPDev));

    bBMLRe = TRUE32;

    pstVol = GET_VOLCXT(nVol);

    /* this function assume that volume has always device,
       thus, this function doesn't check whether volume has device or not */
    for (nDevIdx = 0; nDevIdx < DEVS_PER_VOL; nDevIdx++)
    {
        nPDev = nVol * DEVS_PER_VOL + nDevIdx;
        if (_IsOpenedDev(nPDev) == FALSE32)
            continue;

        pstDev = GET_DEVCXT(nPDev);

        /* Only 1st device has valid partition information */
        if (nDevIdx == 0)
        {
            pstPI   = pstLoadingPI;
            pstPExt = pstLoadingPExt;
        }
        else
        {
            pstPI   = NULL;
            pstPExt = NULL;
        }

        if (BBM_Mount(pstVol, pstDev, pstPI, pstPExt) != BBM_SUCCESS)
        {
            BIF_ERR_PRINT((TEXT("[BIF:ERR]  BBM_Mount(nPDev:%d) Failure\r\n"), nPDev));
            bBMLRe = FALSE32;
            break;
        }

        BIF_INF_PRINT((TEXT("[BIF:MSG]  Mounting Reservoir(nPDev:%d) is completed\r\n"), nPDev));
    }

    BIF_LOG_PRINT((TEXT("[BIF:OUT] --_MountRsvr(nPDev:%d)\r\n"), nPDev));

    return bBMLRe;
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*    _GetPbn                                                                */
/* DESCRIPTION                                                               */
/*    This function get Pbn from nSbn using pBUMap and pBMI                  */
/* PARAMETERS                                                                */
/*      nSbn       [IN]                                                      */
/*            semi physical block number                                     */
/*      pBUMap     [IN]                                                      */
/*            pointer to structure badunit                                   */
/*      pBMI     [IN]                                                        */
/*            pointer to structure BMI                                       */
/* RETURN VALUES                                                             */
/*     physical block number                                                 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
PRIVATE UINT32
_GetPbn(register UINT32 nSbn, register badunit *pBUMap, register bmi *pBMI)
{
    register UINT32  nCnt;
    register UINT32  nNumOFBMFs;
    register bmf    *pBMF;

    pBUMap += (nSbn >> SFT_BLKS_PER_BADUNIT);

    if (pBUMap->nNumOfBMFs > 0)
    {
        nNumOFBMFs = pBUMap->nNumOfBMFs;

        pBMF       = (bmf *) pBMI->pstBMF + pBUMap->n1stBMFIdx;

        for (nCnt = 0; nCnt < nNumOFBMFs; nCnt++)
        {
            if (pBMF->nSbn == nSbn)
            {
                BIF_INF_PRINT((TEXT("[BIF:   ]   Sbn(%d) ==> Pbn(%d)\r\n"),
                    nSbn, pBMF->nRbn));

                return (UINT32) pBMF->nRbn;
            }

            pBMF++;
        }
    }

    return nSbn;
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      _ChkLLDSpecValidity                                                  */
/* DESCRIPTION                                                               */
/*      This function checks whether LLDSpec is valid or not                 */
/*                                                                           */
/* PARAMETERS                                                                */
/*      pstLLDSpec [IN]                                                      */
/*            pointer of LLDSpec structure                                   */
/*            the specification of NAND device                               */
/* RETURN VALUES                                                             */
/*      TRUE32                                                               */
/*            valid LLDSpec                                                  */
/*      FALSE32                                                              */
/*            invalid LLDSpec                                                */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
PRIVATE BOOL32
_ChkLLDSpecValidity(LLDSpec *pstLLDSpec)
{
    BOOL32 bRet = TRUE32;

    BIF_LOG_PRINT((TEXT("[BIF: IN] ++_ChkLLDSpecValidity()\r\n")));

    do {
        /* Pages per Block validity check */
        if ((pstLLDSpec->nPgsPerBlk != 16) &&
            (pstLLDSpec->nPgsPerBlk != 32) &&
            (pstLLDSpec->nPgsPerBlk != 64))

        {
            BIF_ERR_PRINT((TEXT("[BIF:ERR]  pstLLDSpec->nPgsPerBlk(%d) is invalid\r\n"), pstLLDSpec->nPgsPerBlk));
            BIF_ERR_PRINT((TEXT("[BIF:ERR]  pstLLDSpec->nPgsPerBlk should be 16 or 32 or 64\r\n")));


            bRet = FALSE32;
            break;
        }

        /* Sectors per Page validity check */
        if ((pstLLDSpec->nSctsPerPg != 1) &&
            (pstLLDSpec->nSctsPerPg != 2) &&
            (pstLLDSpec->nSctsPerPg != 4))

        {
            BIF_ERR_PRINT((TEXT("[BIF:ERR]  pstLLDSpec->nSctsPerPg(%d) is invalid\r\n"), pstLLDSpec->nSctsPerPg));
            BIF_ERR_PRINT((TEXT("[BIF:ERR]  pstLLDSpec->nSctsPerPg should be 1 or 2 or 4\r\n")));

            bRet = FALSE32;
            break;
        }

        /* Lsn Position of Spare aere validity check */
        if (pstLLDSpec->nLsnPos >= XSR_SPARE_SIZE)
        {
            BIF_ERR_PRINT((TEXT("[BIF:ERR]  pstLLDSpec->nLsnPos(%d) is out of bound\r\n"), pstLLDSpec->nLsnPos));

            bRet = FALSE32;
            break;
        }

        /* BadInfo Position of Spare aere validity check */
        if (pstLLDSpec->nBadPos >= XSR_SPARE_SIZE)
        {
            BIF_ERR_PRINT((TEXT("[BIF:ERR]  pstLLDSpec->nBadPos(%d) is out of bound\r\n"), pstLLDSpec->nBadPos));

            bRet = FALSE32;
            break;
        }

        /* Number of blocks of Reservoir size limitation check */
        if ((pstLLDSpec->nNumOfBlks / 4) <= pstLLDSpec->nBlksInRsv)
        {
            BIF_ERR_PRINT((TEXT("[BIF:ERR]  (pstLLDSpec->nNumOfBlks / 4)(=%d) <= pstLLDSpec->nBlksInRsv(%d)\r\n"),
                           pstLLDSpec->nNumOfBlks / 4, pstLLDSpec->nBlksInRsv));

            bRet = FALSE32;
            break;
        }

        /* Number of blocks of Reservoir validity check */
        if ((pstLLDSpec->nBlksInRsv > MAX_BMF) || (pstLLDSpec->nBlksInRsv == 0))
        {
            BIF_ERR_PRINT((TEXT("[BIF:ERR]  nBlksInRsv(%d) is out of bound\r\n"), pstLLDSpec->nBlksInRsv));
            BIF_ERR_PRINT((TEXT("[BIF:ERR]  0 < nBlksInRsv(%d) <= MAX_BMF(%d)\r\n"), pstLLDSpec->nBlksInRsv));

            bRet = FALSE32;
            break;
        }

        /* Number of blocks validity check */
        if (pstLLDSpec->nNumOfBlks == 0)
        {
            BIF_ERR_PRINT((TEXT("[BIF:ERR]  nNumOfBlks(%d) is out of bound\r\n"), pstLLDSpec->nNumOfBlks));

            bRet = FALSE32;
            break;
        }

    } while(0);

    BIF_LOG_PRINT((TEXT("[BIF:OUT] --_ChkLLDSpecValidity()\r\n")));

    return bRet;
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      _Open                                                                */
/* DESCRIPTION                                                               */
/*      This function opens the given volume.                                */
/* PARAMETERS                                                                */
/*      nVol       [IN]                                                      */
/*            volume number                                                  */
/* RETURN VALUES                                                             */
/*      BML_SUCCESS                                                          */
/*            _Open is completed.                                            */
/*      BML_OAM_ACCESS_ERROR                                                 */
/*            Acccessing OAM is failed                                       */
/*      BML_PAM_ACCESS_ERROR                                                 */
/*            Acccessing PAM is failed                                       */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
PRIVATE INT32
_Open(UINT32 nVol)
{
    INT32       nLLDRe;
    UINT32      nDevIdx;
    UINT32      nPDev;
    UINT32      nIdx;
    UINT32      nMallocSize;
    UINT32     *pTmpBuf;

    LLDSpec     stLLDSpec;
    flash_vol   *pstVol;
    flash_dev   *pstDev;
    vol_param *pstPAM;

    BIF_LOG_PRINT((TEXT("[BIF: IN] ++_Open(nVol=%d)\r\n"), nVol));

    pstVol = GET_VOLCXT(nVol);

    /* ---------------------------------------- */
    /* Volume 0 : PDev0, PDev1, PDev2, PDev3    */
    /* Volume 1 : PDev4, PDev5, PDev6, PDev7    */
    /* ---------------------------------------- */

    pstVol->nNumOfDev    = 0;
    pstVol->nNumOfUsBlks = 0;

    /* LLD Function Table initialization */
    AD_Memset(gstLFT, 0x00, sizeof(lld_ftable) * XSR_MAX_VOL);

    /* Registering the LLD function into the LLD Function Table */
    inter_low_ftable(gstLFT);

    pstVol->lld_init          = gstLFT[nVol].init;
    pstVol->lld_open          = gstLFT[nVol].open;
    pstVol->lld_close         = gstLFT[nVol].close;
    pstVol->lld_read          = gstLFT[nVol].read;
    pstVol->lld_mread         = gstLFT[nVol].mread;
    pstVol->lld_dev_info      = gstLFT[nVol].dev_info;
    pstVol->lld_check_bad     = gstLFT[nVol].check_bad;
    pstVol->lld_flush         = gstLFT[nVol].flush;
    pstVol->lld_set_rw        = gstLFT[nVol].set_rw;
    pstVol->lld_ioctl         = gstLFT[nVol].ioctl;
    pstVol->lld_platform_info = gstLFT[nVol].platform_info;

    /* -------------------------------------------------------------------- */
    /* LFT Link completion check                                            */
    /*                                                                      */
    /* Check whether low level function table(gstLFT) is registered or not  */
    /* if gstLFT is not registered, it is CRITICAL ERROR.                   */
    /* -------------------------------------------------------------------- */
    pTmpBuf = (UINT32 *) &gstLFT[nVol];
    for (nIdx = 0; nIdx < (sizeof(lld_ftable) / sizeof(UINT32)); nIdx++)
    {
        if (pTmpBuf[nIdx] == 0x00000000)
        {
            BIF_ERR_PRINT((TEXT("[BIF:ERR]  LLD(%dth) is not registered\r\n"), nIdx));
            BIF_LOG_PRINT((TEXT("[BIF:OUT] --_Open(Vol:%d) nRe=0x%x\r\n"), nVol, BML_PAM_ACCESS_ERROR));
            return BML_PAM_ACCESS_ERROR;
        }
    }

    pstPAM                = (vol_param *) inter_getparam();
    pstVol->bBAlign       = pstPAM[nVol].bByteAlign;
    pstVol->nNumOfDev     = pstPAM[nVol].nDevsInVol;
    pstVol->nEccPol       = pstPAM[nVol].nEccPol;

    for (nDevIdx = 0; nDevIdx < pstVol->nNumOfDev; nDevIdx++)
    {
        nPDev = nVol * DEVS_PER_VOL + nDevIdx;

        nLLDRe = pstVol->lld_open(nPDev);
        if (nLLDRe != LLD_SUCCESS)
        {
            BIF_ERR_PRINT((TEXT("[BIF:ERR] LLD_Open(PDev:%d) is failed, nLLDRe=0x%X\r\n"), nPDev, nLLDRe));
            BIF_LOG_PRINT((TEXT("[BIF:OUT] --_Open(Vol:%d) nRe=0x%x\r\n"), nVol, BML_DEVICE_ACCESS_ERROR));
            return BML_DEVICE_ACCESS_ERROR;
        }

        BIF_INF_PRINT((TEXT("[BIF:   ]   LLD_Open(PDev:%d) is completed\r\n"), nPDev));

        /* get NAND device specification using LLD_GetDevInfo() */
        nLLDRe = pstVol->lld_dev_info(nPDev, &stLLDSpec);
        if (nLLDRe != LLD_SUCCESS)
        {
            BIF_ERR_PRINT((TEXT("[BIF:ERR]  LLD_GetDevInfo(PDev:%d) is failed , nLLDRe=0x%X\r\n"), nPDev, nLLDRe));
            BIF_LOG_PRINT((TEXT("[BIF:OUT] --_Open(Vol:%d) nRe=0x%x\r\n"), nVol, BML_DEVICE_ACCESS_ERROR));
            return BML_DEVICE_ACCESS_ERROR;
        }

        BIF_INF_PRINT((TEXT("[BIF:   ] LLD_GetDevInfo(PDev:%d) is completed\r\n"), nPDev));

        /* check the validity of LLDSpec */
        if (_ChkLLDSpecValidity(&stLLDSpec) == FALSE32)
        {
            BIF_ERR_PRINT((TEXT("[BIF:ERR]  Invalid LLDSpec\r\n")));
            BIF_LOG_PRINT((TEXT("[BIF:OUT] --_Open(Vol:%d) nRe=0x%x\r\n"), nVol, BML_DEVICE_ACCESS_ERROR));
            return BML_DEVICE_ACCESS_ERROR;
        }

        /* create Device Context */
        pstDevCxt[nPDev] = (flash_dev *) AD_Malloc(sizeof(flash_dev));
        if (pstDevCxt[nPDev] == NULL)
        {
            BIF_ERR_PRINT((TEXT("[BIF:ERR]  AD_Malloc Error:pstDev[%d]\r\n"), nPDev));
            BIF_LOG_PRINT((TEXT("[BIF:OUT] --_Open(Vol:%d) nRe=0x%x\r\n"), nVol, BML_OAM_ACCESS_ERROR));
            return BML_OAM_ACCESS_ERROR;
        }

        pstDev                = pstDevCxt[nPDev];
        pstDev->nDevNo        = (UINT16) nPDev;

        /* initialize variables of Volume Context */
        pstVol->nBlksPerDev   = stLLDSpec.nNumOfBlks;
        pstVol->nRsvrPerDev   = (UINT16) (stLLDSpec.nBlksInRsv + RSV_META_BLKS);
        pstVol->nSctsPerBlk   = (UINT16) (stLLDSpec.nSctsPerPg * stLLDSpec.nPgsPerBlk);
        pstVol->nPgsPerBlk    = stLLDSpec.nPgsPerBlk;

        /* initialize variables of Device Context */
        pstVol->n1stSbnOfRsvr = (UINT16) (stLLDSpec.nNumOfBlks - pstVol->nRsvrPerDev);
        pstVol->nERLBlkSbn    = pstVol->n1stSbnOfRsvr;
        pstVol->nRefBlkSbn    = pstVol->n1stSbnOfRsvr + 1;
        pstVol->nLsnPos       = stLLDSpec.nLsnPos;
        pstVol->nBadPos       = stLLDSpec.nBadPos;
        pstVol->nEccPos       = stLLDSpec.nEccPos;
        pstVol->nBWidth       = stLLDSpec.nBWidth;
        pstVol->nSctsPerPg    = stLLDSpec.nSctsPerPg;
        pstVol->nPlanesPerDev = stLLDSpec.nNumOfPlane;
        pstVol->nMEFlag       = stLLDSpec.nMEFlag;

        BIF_INF_PRINT((TEXT("[BIF:MSG]   pstVol->nBlksPerDev   : %d\r\n"), pstVol->nBlksPerDev));
        BIF_INF_PRINT((TEXT("[BIF:MSG]   pstVol->nRsvrPerDev   : %d\r\n"), pstVol->nRsvrPerDev));
        BIF_INF_PRINT((TEXT("[BIF:MSG]   pstVol->nSctsPerBlk   : %d\r\n"), pstVol->nSctsPerBlk));
        BIF_INF_PRINT((TEXT("[BIF:MSG]   pstVol->nPgsPerBlk    : %d\r\n"), pstVol->nPgsPerBlk));
        BIF_INF_PRINT((TEXT("[BIF:MSG]   pstVol->n1stSbnOfRsvr : %d\r\n"), pstVol->n1stSbnOfRsvr));
        BIF_INF_PRINT((TEXT("[BIF:MSG]   pstVol->nLsnPos       : %d\r\n"), pstVol->nLsnPos));
        BIF_INF_PRINT((TEXT("[BIF:MSG]   pstVol->nBadPos       : %d\r\n"), pstVol->nBadPos));
        BIF_INF_PRINT((TEXT("[BIF:MSG]   pstVol->nEccPos       : %d\r\n"), pstVol->nEccPos));
        BIF_INF_PRINT((TEXT("[BIF:MSG]   pstVol->nEccPol       : %d  0:NO_ECC 1:SW_ECC 2:HW_ECC\r\n"), pstVol->nEccPol));
        BIF_INF_PRINT((TEXT("[BIF:MSG]   pstVol->nBWidth       : %d\r\n"), pstVol->nBWidth));
        BIF_INF_PRINT((TEXT("[BIF:MSG]   pstVol->nSctsPerPg    : %d\r\n"), pstVol->nSctsPerPg));
        BIF_INF_PRINT((TEXT("[BIF:MSG]   pstVol->nPlanesPerDev : %d\r\n"), pstVol->nPlanesPerDev));
        BIF_INF_PRINT((TEXT("[BIF:MSG]   pstVol->nMEFlag       : %d\r\n"), pstVol->nMEFlag));

        /* allocate memory for bad block mapping field */
        pstDev->stRsv.stBMI.pstBMF = (bmf *) AD_Malloc(stLLDSpec.nBlksInRsv * sizeof(bmf));
        if (pstDev->stRsv.stBMI.pstBMF == NULL)
        {
            BIF_ERR_PRINT((TEXT("[BIF:ERR]  AD_Malloc Error:pstBMF\r\n")));
            BIF_LOG_PRINT((TEXT("[BIF:OUT] --_Open(Vol:%d) nRe=0x%x\r\n"), nVol, BML_OAM_ACCESS_ERROR));
            return BML_OAM_ACCESS_ERROR;
        }

        /* allocte memory for reserved block allocation bitmap */
        pstDev->stRsv.pBABitMap = (UINT8 *) AD_Malloc(BABITMAP_SIZE);
        if (pstDev->stRsv.pBABitMap == NULL)
        {
            BIF_ERR_PRINT((TEXT("[BIF:ERR]  AD_Malloc Error:pBABitMap\r\n")));
            BIF_LOG_PRINT((TEXT("[BIF:OUT] --_Open(Vol:%d) nRe=0x%x\r\n"), nVol, BML_OAM_ACCESS_ERROR));
            return BML_OAM_ACCESS_ERROR;
        }

        nMallocSize = (stLLDSpec.nNumOfBlks > BLKS_PER_BADUNIT) ? stLLDSpec.nNumOfBlks / BLKS_PER_BADUNIT : 1;
        pstDev->stRsv.pBUMap = (badunit *) AD_Malloc(nMallocSize * sizeof(badunit));
        if (pstDev->stRsv.pBUMap == NULL)
        {
            BIF_ERR_PRINT((TEXT("[BIF:ERR]  AD_Malloc Error:pBUMap\r\n")));
            BIF_LOG_PRINT((TEXT("[BIF:OUT] --_Open(Vol:%d) nRe=0x%x\r\n"), nVol, BML_OAM_ACCESS_ERROR));
            return BML_OAM_ACCESS_ERROR;
        }

        pstDev->pTmpMBuf = (UINT8 *) AD_Malloc(XSR_SECTOR_SIZE * pstVol->nSctsPerPg);
        pstDev->pTmpSBuf = (UINT8 *) AD_Malloc(XSR_SPARE_SIZE * pstVol->nSctsPerBlk);
        if ((pstDev->pTmpMBuf == NULL) ||
            (pstDev->pTmpSBuf == NULL))
        {
            BIF_ERR_PRINT((TEXT("[BIF:ERR]  AD_Malloc Error:pTmpMBuf or pTmpSBuf\r\n")));
            BIF_LOG_PRINT((TEXT("[BIF:OUT] --_Open(Vol:%d) nRe=0x%x\r\n"), nVol, BML_OAM_ACCESS_ERROR));
            return BML_OAM_ACCESS_ERROR;
        }

        pstVol->nNumOfUsBlks += pstVol->n1stSbnOfRsvr;
    }

    pstVol->nMskSctsPerPg   = (UINT8)((pstVol->nSctsPerPg   ==   1) ? 0 :
                                      (pstVol->nSctsPerPg   ==   2) ? 1 : 3);
                                      /* 3 means that nSctsPerPg is 4 */

    pstVol->nSftSctsPerBlk  = (UINT8)((pstVol->nSctsPerBlk  == 256) ? 8 :
                                      (pstVol->nSctsPerBlk  ==  32) ? 5 :
                                      (pstVol->nSctsPerBlk  ==  16) ? 4 : 7);
                                      /* 7 means that nSctsPerBlk is 128. */

    pstVol->nSftSctsPerPg   = (UINT8)((pstVol->nSctsPerPg   ==   1) ? 0 :
                                      (pstVol->nSctsPerPg   ==   2) ? 1 : 2);
                                      /* 2 means that nSctsPerPg is 4 */

    pstVol->nSftnNumOfDev   = (UINT8)((pstVol->nNumOfDev    ==   1) ? 0 :
                                      (pstVol->nNumOfDev    ==   2) ? 1 :
                                      (pstVol->nNumOfDev    ==   3) ? 3 : 2);
                                      /* 2 means that nNumOfDev is 4 */

    pstVol->pstPartI        = (part_info *)AD_Malloc(sizeof(part_info));
    pstVol->pPIExt          = (part_exinfo *)AD_Malloc(sizeof(part_exinfo));

    if ((pstVol->pstPartI      == NULL) ||
        (pstVol->pPIExt        == NULL))
    {
        BIF_ERR_PRINT((TEXT("[BIF:ERR]  AD_Malloc Error for PI/PIExt\r\n")));
        BIF_LOG_PRINT((TEXT("[BIF:OUT] --_Open(Vol:%d) nRe=0x%x\r\n"), nVol, BML_OAM_ACCESS_ERROR));
        return BML_OAM_ACCESS_ERROR;
    }

    pstVol->pPIExt->pData   = (VOID     *)AD_Malloc(BML_MAX_PIEXT_DATA);

    if (pstVol->pPIExt->pData == NULL)
    {
        BIF_ERR_PRINT((TEXT("[BIF:ERR]  AD_Malloc Error for PI/PIExt\r\n")));
        BIF_LOG_PRINT((TEXT("[BIF:OUT] --_Open(Vol:%d) nRe=0x%x\r\n"), nVol, BML_OAM_ACCESS_ERROR));
        return BML_OAM_ACCESS_ERROR;
    }

    pstVol->pPIExt->nID         = 0xFFFFFFFF;
    pstVol->pPIExt->nSizeOfData = 0xFFFFFFFF;

    AD_Memset(pstVol->pPIExt->pData, 0xFF, BML_MAX_PIEXT_DATA);

    BIF_LOG_PRINT((TEXT("[BIF:OUT] --_Open(Vol:%d) nRe=0x%x\r\n"), nVol, BML_SUCCESS));

    return BML_SUCCESS;
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      _Close                                                               */
/* DESCRIPTION                                                               */
/*      This function closes the volume.                                     */
/* PARAMETERS                                                                */
/*      nVol       [IN]                                                      */
/*            volume number                                                  */
/* RETURN VALUES                                                             */
/*      BML_SUCCESS                                                          */
/*            _Close is completed                                            */
/*      BML_CRITICAL_ERROR                                                   */
/*            _Close is failed                                               */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
PRIVATE INT32
_Close(UINT32 nVol)
{
    UINT32      nDevIdx;
    UINT32      nPDev;
    INT32       nLLDRe;
    INT32       nBMLRe = BML_SUCCESS;

    flash_vol   *pstVol;
    flash_dev   *pstDev;

    BIF_LOG_PRINT((TEXT("[BIF: IN] ++_Close(Vol:%d)\r\n"), nVol));

    pstVol = GET_VOLCXT(nVol);

    for (nDevIdx = 0; nDevIdx < DEVS_PER_VOL; nDevIdx++)
    {
        nPDev       = nVol * DEVS_PER_VOL + nDevIdx;

        if (pstDevCxt[nPDev] == NULL)
            continue;

        pstDev      = pstDevCxt[nPDev];

        /* LLD_Close call */
        nLLDRe      = pstVol->lld_close(nPDev);
        if (nLLDRe != LLD_SUCCESS)
        {
            nBMLRe = BML_CRITICAL_ERROR;

            BIF_ERR_PRINT((TEXT("[BIF:ERR]  LLD_Close(nPDev:%d) is failed (nRe=0x%x)\r\n"),
                nPDev, nLLDRe));
        }

        /* Free the allocated memory */
        if (pstDev->stRsv.pBUMap != NULL)
        {
            AD_Free(pstDev->stRsv.pBUMap);
            pstDev->stRsv.pBUMap = NULL;
        }

        /* Free the allocated memory */
        if (pstDev->stRsv.pBABitMap != NULL)
        {
            AD_Free(pstDev->stRsv.pBABitMap);
            pstDev->stRsv.pBABitMap = NULL;
        }

        /* Free the allocated memory */
        if (pstDev->stRsv.stBMI.pstBMF != NULL)
        {
            AD_Free(pstDev->stRsv.stBMI.pstBMF);
            pstDev->stRsv.stBMI.pstBMF = NULL;
        }

        if (pstDev->pTmpMBuf != NULL)
        {
            AD_Free(pstDev->pTmpMBuf);
            pstDev->pTmpMBuf = NULL;
        }

        if (pstDev->pTmpSBuf != NULL)
        {
            AD_Free(pstDev->pTmpSBuf);
            pstDev->pTmpSBuf = NULL;
        }

        AD_Free(pstDev);
        pstDevCxt[nPDev] = NULL;

    }

/* This is not using code, TinyBML by cramfs booting is not close */
//#ifndef CONFIG_RFS_XSR_MODULE
#ifndef CONFIG_RFS_XSR
    /* Check whether the device is busy or not */
    if (AD_AcquireSM(pstVol->nSm) == FALSE32)
    {
            BIF_ERR_PRINT((TEXT("[BIF:ERR]  Acquiring semaphore is failed\r\n")));
            nBMLRe = BML_ACQUIRE_SM_ERROR;
    }

    /* Destroy the semaphore handle for nVDevNo */
    AD_DestroySM(pstVol->nSm);
#endif

    if (pstVol->pstPartI != NULL)
    {
        AD_Free(pstVol->pstPartI);
        pstVol->pstPartI = NULL;
    }

    if (pstVol->pPIExt != NULL)
    {
        if (pstVol->pPIExt->pData != NULL)
        {
            AD_Free(pstVol->pPIExt->pData);
            pstVol->pPIExt->pData = NULL;
        }

        AD_Free(pstVol->pPIExt);
        pstVol->pPIExt = NULL;
    }

    AD_Memset(pstVol, 0x00, sizeof(flash_vol));
    pstVol->bVolOpen = FALSE32;

    BIF_LOG_PRINT((TEXT("[BIF:OUT] --_Close(Vol:%d)\r\n"), nVol));

    return nBMLRe;
}


PRIVATE VOID
_TrimMainBuf(UINT32 nWriteScts, SGL *pstSGL)
{
    UINT32      nSGLWriteScts = 0;
    UINT32      nTmpSGLWriteScts;
    UINT8       nSGLIdx1 = 0;
    UINT8       nSGLIdx2 = 0;
    SGL         stTmpSGL;
    stTmpSGL = *pstSGL;

    /* Pointer to Main buffer increasement */
    do
    {
        nSGLWriteScts += (UINT32)stTmpSGL.stSGLEntry[nSGLIdx1].nSectors;
        if (nSGLWriteScts > nWriteScts || stTmpSGL.nElements == nSGLIdx1 + 1)
        {
            break;
        }
        nSGLIdx1++;
    } while(1);

    /* If MWrite() is completely terminated, SGL is not changed */
    if (nWriteScts == nSGLWriteScts && stTmpSGL.nElements == nSGLIdx1 + 1)
    {
        return;
    }

    do
    {

        if (nSGLIdx2 == 0)
        {
            pstSGL->nElements = 0;
            nTmpSGLWriteScts = nSGLWriteScts - stTmpSGL.stSGLEntry[nSGLIdx1].nSectors;
            pstSGL->stSGLEntry[nSGLIdx2].pBuf = stTmpSGL.stSGLEntry[nSGLIdx1].pBuf + ((nWriteScts - nTmpSGLWriteScts) * XSR_SECTOR_SIZE);
            pstSGL->stSGLEntry[nSGLIdx2].nSectors = nSGLWriteScts - nWriteScts;
            pstSGL->stSGLEntry[nSGLIdx2++].nFlag = stTmpSGL.stSGLEntry[nSGLIdx1].nFlag;

        }
        else
        {
            pstSGL->stSGLEntry[nSGLIdx2].pBuf = stTmpSGL.stSGLEntry[nSGLIdx1].pBuf ;
            pstSGL->stSGLEntry[nSGLIdx2].nSectors = stTmpSGL.stSGLEntry[nSGLIdx1].nSectors;
            pstSGL->stSGLEntry[nSGLIdx2++].nFlag = stTmpSGL.stSGLEntry[nSGLIdx1].nFlag;

        }
        pstSGL->nElements++;

        if (nSGLIdx1 + 1 == stTmpSGL.nElements)
        {
            break;
        }

        nSGLIdx1++;


    } while(1);

    return;
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      tbml_init                                                             */
/* DESCRIPTION                                                               */
/*      This function initializes the data structure of BML and              */
/*      calls LLD_Init                                                       */
/* PARAMETERS                                                                */
/*      none                                                                 */
/* RETURN VALUES                                                             */
/*      BML_ALREADY_INITIALIZED                                              */
/*      BML_DEVICE_ACCESS_ERROR                                              */
/*      BML_CRITICAL_ERROR                                                   */
/*      BML_SUCCESS                                                          */
/* NOTES                                                                     */
/*      Before all of other functions of BML is called, tbml_init() should be */
/*      called.                                                              */
/*                                                                           */
/*****************************************************************************/
INT32
tbml_init(VOID)
{
    UINT32         nCnt;
    UINT32         nVol;
    INT32          nLLDRe;
    static BOOL32  bBMLInitFlag = FALSE32;

    BIF_LOG_PRINT((TEXT("[BIF:IN ] ++tbml_init\r\n")));

    if (bBMLInitFlag == TRUE32)
    {
        BIF_ERR_PRINT((TEXT("[BIF:WRN] tbml_init is already called\r\n")));
        BIF_LOG_PRINT((TEXT("[BIF:OUT] --tbml_init() nRe=0x%x\r\n"), BML_ALREADY_INITIALIZED));

        return BML_ALREADY_INITIALIZED;
    }

    AD_Memset(gstVolCxt, 0x00, sizeof(flash_vol) * XSR_MAX_VOL);

    for (nCnt = 0; nCnt < XSR_MAX_VOL; nCnt++)
    {
        gstVolCxt[nCnt].bVolOpen = FALSE32;
        gstVolCxt[nCnt].nOpenCnt = 0;
    }

    for (nCnt = 0; nCnt < XSR_MAX_DEV; nCnt++)
    {
        pstDevCxt[nCnt]     = NULL;
    }

    AD_Memset(gstLFT, 0x00, sizeof(lld_ftable) * XSR_MAX_VOL);

    /* Registering the LLD function into the LLD Function Table */
    inter_low_ftable(gstLFT);

    for (nVol = 0; nVol < XSR_MAX_VOL; nVol++)
    {
        if (gstLFT[nVol].init == NULL)
            continue;

        nLLDRe = gstLFT[nVol].init((VOID *) inter_getparam());
        if (nLLDRe != LLD_SUCCESS)
        {
            BIF_ERR_PRINT((TEXT("[BIF:ERR] LLD_Init() is failed, nLLDRe=0x%X\r\n"), nLLDRe));
            return BML_DEVICE_ACCESS_ERROR;
        }
    }

    bBMLInitFlag = TRUE32;

    BIF_LOG_PRINT((TEXT("[BIF:OUT] --tbml_init() nRe=0x%x\r\n"), BML_SUCCESS));

    return BML_SUCCESS;
}

int display_dbg_msg(char* txtMsg, unsigned char fontSize, unsigned char retainPrevMsg)
{
    return 0;
}
extern void omap_prcm_arch_reset(char mode);

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      tbml_open                                                             */
/* DESCRIPTION                                                               */
/*      This function opens the volume.                                      */
/* PARAMETERS                                                                */
/*      nVol       [IN]                                                      */
/*            volume number                                                  */
/* RETURN VALUES                                                             */
/*      BML_SUCCESS                                                          */
/*            tbml_open is completed                                          */
/*      BML_INVALID_PARAM                                                    */
/*            invalid parameters                                             */
/*      BML_UNFORMATTED                                                      */
/*            volume is unformatted                                          */
/* NOTES                                                                     */
/*      Before tbml_open() is called, tbml_init() should be called.            */
/*                                                                           */
/*****************************************************************************/
INT32
tbml_open(UINT32 nVol)
{
	INT32 try_count = 0;
    INT32     nBMLRe = BML_SUCCESS;
    UINT16    na1stSbnOfULA[DEVS_PER_VOL];
    flash_vol *pstVol;

retry:
    BIF_LOG_PRINT((TEXT("[BIF: IN] ++tbml_open(Vol:%d)\r\n"), nVol));

    CHK_VOL_RANGE(nVol);

    pstVol = GET_VOLCXT(nVol);

    do {
        /* If tbml_open is already called, tbml_open just is returned. */
        if (pstVol->bVolOpen == TRUE32)
        {
            pstVol->nOpenCnt++;

            BIF_INF_PRINT((TEXT("[BIF:MSG]  BML is already opened. Open count of volume(%d) is %d\r\n"),
                           nVol, pstVol->nOpenCnt));
           break;
        }


		/* Create the semaphore handle for nDevIdx */
		if (AD_CreateSM (&(pstVol->nSm)) == FALSE32)
		{
			BIF_ERR_PRINT((TEXT("[BIF:ERR]  AD_CreateSM Error\r\n")));
			BIF_LOG_PRINT((TEXT("[BIF:OUT] --_Open(Vol:%d) nRe=0x%x\r\n"), nVol, BML_OAM_ACCESS_ERROR));
			nBMLRe = BML_OAM_ACCESS_ERROR;
#if 1 /* added by jtjang for debug */
			display_dbg_msg("ONENAND Error : AD_CreateSM() fail", 0, 0);
#endif
			break;
		}

        nBMLRe = _Open(nVol);
        if (nBMLRe != BML_SUCCESS)
        {
            BIF_ERR_PRINT((TEXT("[BIF:ERR]  _Open(nVol:%d) is failed, (nRe:0x%x)\r\n"), nVol, nBMLRe));

            if (_Close(nVol) != BML_SUCCESS)
            {
                BIF_ERR_PRINT((TEXT("[BIF:ERR]  _Close is failed\r\n")));
            }
#if 1 /* added by jtjang for debug */
	display_dbg_msg("ONENAND Error : _Open() fail", 0, 0);
#endif

            break;
        }

        if (_MountRsvr(nVol, pstVol->pstPartI, pstVol->pPIExt) != TRUE32)
        {
            BIF_ERR_PRINT((TEXT("[BIF:ERR]  _MountRsvr(nVol:%d) is failed\r\n"), nVol));

            if (_Close(nVol) != BML_SUCCESS)
            {
                BIF_ERR_PRINT((TEXT("[BIF:ERR]  _Close is failed\r\n")));
                nBMLRe = BML_CRITICAL_ERROR;
            }
            else
            {
                BIF_ERR_PRINT((TEXT("[BIF:ERR]  nVol:%d is BML unformatted\r\n"), nVol));
                nBMLRe = BML_UNFORMATTED;
            }
#if 1 /* added by jtjang for debug */
		display_dbg_msg("ONENAND Error : _MountRsvr() fail", 0, 0);
#endif
            break;
        }

        _CalPhyULArea(nVol, pstVol->pstPartI, na1stSbnOfULA);

        _RecalAllocBlkPtr(nVol);

        /* Protect RO-Area */
        _ProtectLockedArea(nVol, TRUE32);

        /* diable pre-programming flag */
        pstVol->bPreProgram = FALSE32;
        /* enable volume open flag */
        pstVol->bVolOpen    = TRUE32;
        /* set open count */
        pstVol->nOpenCnt    = 1;

    } while(0);

    BIF_LOG_PRINT((TEXT("[BIF:OUT] --tbml_open() nRe=0x%x\r\n"), nBMLRe));

	if(nBMLRe) {
		if(try_count == 3) {
			display_dbg_msg("ONENAND Error : tbml_open", 0, 0);
			printk("ONENANE Error : tbml_open()\n");
			omap_prcm_arch_reset(0);
		} 
		printk("tbml_open() fail. Retry\n");
		try_count++;
		goto retry;
	}

    return nBMLRe;
}


#if !defined(XSR_NBL2)
/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      tbml_close                                                            */
/* DESCRIPTION                                                               */
/*      This function closes the volume.                                     */
/*                                                                           */
/* PARAMETERS                                                                */
/*      nVol       [IN]                                                      */
/*          volume number                                                    */
/* RETURN VALUES                                                             */
/*      BML_SUCCESS                                                          */
/*          tbml_close is completed                                           */
/*      BML_CRITICAL_ERROR                                                   */
/*          critical error                                                   */
/*      BML_INVALID_PARAM                                                    */
/*          invalid parameter                                                */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
INT32
tbml_close(UINT32 nVol)
{
    INT32     nLLDRe;
    INT32     nBMLRe = BML_SUCCESS;
    UINT32    nDevIdx;
    UINT32    nPDev;
    flash_vol *pstVol;

    BIF_LOG_PRINT((TEXT("[BIF: IN] ++tbml_close(Vol:%d)\r\n"), nVol));

    CHK_VOL_RANGE(nVol);

    pstVol = GET_VOLCXT(nVol);

    CHK_VOL_OPEN(pstVol->bVolOpen);

    if (--pstVol->nOpenCnt != 0)
    {
        BIF_INF_PRINT((TEXT("[BIF:MSG]   Open count of volume(%d) is %d\r\n"),
            nVol, pstVol->nOpenCnt));

        BIF_LOG_PRINT((TEXT("[BIF:OUT] --tbml_close() nRe=0x%x\r\n"), BML_SUCCESS));
        return BML_SUCCESS;
    }

    for (nDevIdx = 0; nDevIdx < DEVS_PER_VOL; nDevIdx++)
    {
        nPDev = nVol * DEVS_PER_VOL + nDevIdx;

        if (_IsOpenedDev(nPDev) == FALSE32)
            continue;

        nLLDRe = pstVol->lld_flush(nPDev);

		if ( nLLDRe != BML_SUCCESS )
        {
			BIF_ERR_PRINT((TEXT("[BIF:ERR]  low level flush error (nVol:%d) is failed\r\n"), nVol));
			nBMLRe = nLLDRe;
        }

    }

    if (_Close(nVol) != BML_SUCCESS)
    {
        nBMLRe = BML_CRITICAL_ERROR;
        BIF_ERR_PRINT((TEXT("[BIF:ERR]  _Close(nVol:%d) is failed\r\n"), nVol));
    }

    BIF_LOG_PRINT((TEXT("[BIF:OUT] --tbml_close() nRe=0x%x\r\n"), nBMLRe));

    return nBMLRe;
}
#endif /* XSR_NBL2 */


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      tbml_get_vol_spec                                                       */
/* DESCRIPTION                                                               */
/*      This function get the volume information, which is as follows.       */
/*        pstVolSpec->nPgsPerBlk                                             */
/*        pstVolSpec->nSctsPerPg                                             */
/*        pstVolSpec->nLsnPos                                                */
/*        pstVolSpec->nTrTime                                                */
/*        pstVolSpec->nTwTime                                                */
/*        pstVolSpec->nTeTime                                                */
/*        pstVolSpec->nTfTime                                                */
/*                                                                           */
/* PARAMETERS                                                                */
/*      nVol                                                                 */
/*          volume number                                                    */
/*      pstVolSpec                                                           */
/*          pointer which the volume information is stored                   */
/* RETURN VALUES                                                             */
/*      BML_SUCCESS                                                          */
/*          tbml_get_vol_spec is completed                                      */
/*      BML_INVALID_PARAM                                                    */
/*          invalid parameter                                                */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
INT32
tbml_get_vol_spec(UINT32 nVol, tbml_vol_spec *pstVolSpec)
{
    UINT32    nPDev;
    INT32     nLLDRe;
    INT32     nBMLRe = BML_SUCCESS;

    flash_vol *pstVol;
    flash_dev *pstDev;
    LLDSpec   stDevInfo;

    BIF_LOG_PRINT((TEXT("[BIF: IN] ++tbml_get_vol_spec(nVol:%d)\r\n"), nVol));

    CHK_VOL_RANGE(nVol);

    pstVol = GET_VOLCXT(nVol);

    CHK_VOL_OPEN(pstVol->bVolOpen);

    do {
        if (pstVolSpec == NULL)
        {
            BIF_ERR_PRINT((TEXT("[BIF:ERR]  pstVolSpec is NULL\r\n")));
            nBMLRe = BML_INVALID_PARAM;
            break;
        }

        nPDev  = nVol * DEVS_PER_VOL +  0;
        pstDev = GET_DEVCXT(nPDev);

        if (AD_AcquireSM(pstVol->nSm) == FALSE32)
        {
            BIF_ERR_PRINT((TEXT("[BIF:ERR]  Acquiring semaphore is failed\r\n")));
            nBMLRe = BML_ACQUIRE_SM_ERROR;
            break;
        }

        nLLDRe = pstVol->lld_dev_info(nPDev, &stDevInfo);
        if (nLLDRe != LLD_SUCCESS)
        {
            BIF_ERR_PRINT((TEXT("[BIF:ERR]  LLD_GetDevInfo returns error(0x%x)\r\n"), nLLDRe));
            nBMLRe = BML_CRITICAL_ERROR;
            break;
        }

        pstVolSpec->nPgsPerBlk   = (UINT16) pstVol->nPgsPerBlk;

        pstVolSpec->nLsnPos      = (UINT8)  pstVol->nLsnPos;

        pstVolSpec->nNumOfUsBlks = pstVol->nNumOfUsBlks;
        pstVolSpec->nSctsPerPg   = (UINT8)  pstVol->nSctsPerPg;
        pstVolSpec->nReserved16  = (((UINT16)stDevInfo.nMCode) << 8) |
                                    (((UINT16)stDevInfo.nDCode) & 0x00FF);

        pstVolSpec->nEccPos      = (UINT8)  pstVol->nEccPos;
        pstVolSpec->nEccPol      = (UINT32) pstVol->nEccPol;

        pstVolSpec->nTrTime      = stDevInfo.nTrTime;
        pstVolSpec->nTwTime      = stDevInfo.nTwTime;
        pstVolSpec->nTeTime      = stDevInfo.nTeTime;
        pstVolSpec->nTfTime      = stDevInfo.nTfTime;

        pstVolSpec->bMErasePol   = (stDevInfo.nMEFlag == LLD_ME_OK) ? TRUE32 : FALSE32;

        AD_Memcpy(pstVolSpec->aUID, stDevInfo.aUID, XSR_UID_SIZE);

        if (AD_ReleaseSM(pstVol->nSm) == FALSE32)
        {
            BIF_ERR_PRINT((TEXT("[BIF:ERR]  Releasing semaphore is failed\r\n")));
            nBMLRe = BML_RELEASE_SM_ERROR;
            break;
        }

    } while(0);

    BIF_LOG_PRINT((TEXT("[BIF:OUT] --tbml_get_vol_spec() nRe=0x%x\r\n"), nBMLRe));

    return nBMLRe;
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      tbml_get_dev_spec                                                       */
/* DESCRIPTION                                                               */
/*      This function gets the device information assuming that same device  */
/*      is used in a volume                                                  */
/*                                                                           */
/* PARAMETERS                                                                */
/*      nVol                                                                 */
/*          volume number                                                    */
/*      pstLLDSpec                                                           */
/*          pointer which the device information is stored                   */
/* RETURN VALUES                                                             */
/*      BML_SUCCESS                                                          */
/*          tbml_get_dev_spec is completed                                      */
/*      BML_INVALID_PARAM                                                    */
/*          invalid parameter                                                */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
INT32
tbml_get_dev_spec(UINT32 nVol, LLDSpec *pstLLDSpec)
{
    UINT32    nPDev;
    INT32     nLLDRe;
    INT32     nBMLRe = BML_SUCCESS;

    flash_vol *pstVol;

    BIF_LOG_PRINT((TEXT("[BIF: IN] ++tbml_get_dev_spec(nVol:%d)\r\n"), nVol));

    CHK_VOL_RANGE(nVol);

    pstVol = GET_VOLCXT(nVol);

    CHK_VOL_OPEN(pstVol->bVolOpen);

    do {
        if (pstLLDSpec == NULL)
        {
            BIF_ERR_PRINT((TEXT("[BIF:ERR]  pstLLDSpec is NULL\r\n")));
            nBMLRe = BML_INVALID_PARAM;
            break;
        }

        nPDev  = nVol * DEVS_PER_VOL +  0;

        if (AD_AcquireSM(pstVol->nSm) == FALSE32)
        {
            BIF_ERR_PRINT((TEXT("[BIF:ERR]  Acquiring semaphore is failed\r\n")));
            nBMLRe = BML_ACQUIRE_SM_ERROR;
            break;
        }

        nLLDRe = pstVol->lld_dev_info(nPDev, pstLLDSpec);
        if (nLLDRe != LLD_SUCCESS)
        {
            BIF_ERR_PRINT((TEXT("[BIF:ERR]  LLD_GetDevInfo returns error(0x%x)\r\n"), nLLDRe));
            nBMLRe = BML_CRITICAL_ERROR;
            break;
        }

        if (AD_ReleaseSM(pstVol->nSm) == FALSE32)
        {
            BIF_ERR_PRINT((TEXT("[BIF:ERR]  Releasing semaphore is failed\r\n")));
            nBMLRe = BML_RELEASE_SM_ERROR;
            break;
        }

    } while(0);

    BIF_LOG_PRINT((TEXT("[BIF:OUT] --tbml_get_dev_spec() nRe=0x%x\r\n"), nBMLRe));

    return nBMLRe;
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      tbml_read                                                             */
/* DESCRIPTION                                                               */
/*      This function reads virtual sectors                                  */
/* PARAMETERS                                                                */
/*      nVol       [IN]                                                      */
/*            Volume number                                                  */
/*      nVsn       [IN]                                                      */
/*            First virtual sector number for reading                        */
/*      nSctsInPg  [IN]                                                      */
/*            The number of sectors to read                                  */
/*      pMBuf      [IN]                                                      */
/*            Main sector buffer for reading                                 */
/*      pSBuf      [IN]                                                      */
/*            Spare sector buffer for reading                                */
/*      nFlag      [IN]                                                      */
/*            BML_FLAG_ECC_ON                                                */
/*            BML_FLAG_ECC_OFF                                               */
/*            BML_FLAG_BBM_ON                                                */
/*            BML_FLAG_BBM_OFF                                               */
/* RETURN VALUES                                                             */
/*      BML_SUCCESS                                                          */
/*            tbml_read is completed                                          */
/*      BML_READ_ERROR                                                       */
/*            read data has the uncorrectable read error                     */
/*      BML_INVALID_DATA_ERROR                                               */
/*            read data already has the uncorrectable read error             */
/*      BML_CRITICAL_ERROR                                                   */
/*            critical error                                                 */
/*      BML_INVALID_PARAM                                                    */
/*            invalid parameter                                              */
/*      BML_ACQUIRE_SM_ERROR                                                 */
/*            acquiring semaphore is failed                                  */
/*      BML_RELEASE_SM_ERROR                                                 */
/*            releasing semaphore is failed                                  */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
INT32
tbml_read(UINT32 nVol,  UINT32 nVsn,  UINT32 nSctsInPg,
         UINT8 *pMBuf, UINT8 *pSBuf, UINT32 nFlag)
{
    INT32     nLLDRe;       /* LLD return value                          */
    INT32     nBMLRe = BML_SUCCESS;
    INT32     nMajorErr;    /* major error of LLD return value           */
    INT32     nMinorErr;    /* minor error of LLD return value           */
    UINT32    nVbn;         /* virtual block number                      */
    UINT32    nSecOff;      /* sector offset                             */
    UINT32    nSbn;         /* semi physical block number                */
    UINT32    nPDev;        /* physical device number                    */
    UINT32    nPbn;         /* physical block number                     */
    UINT8    *pTmpSBuf;
    BOOL32    bExit;

    register flash_vol *pstVol;
    register flash_dev *pstDev;

    BIF_LOG_PRINT((TEXT("[BIF: IN] ++tbml_read(nVol:%d,Vsn:%d,Flag:0x%x)\r\n"),
                   nVol, nVsn, nFlag));

    CHK_VOL_RANGE(nVol);

    pstVol = GET_VOLCXT(nVol);

    CHK_VOL_OPEN(pstVol->bVolOpen);

    /* -------------------------------------------------------- */
    /*    Translates Vsn to Vbn                                 */
    /*       if nSctsPerBlk is 256, nSftSctsPerBlk is 8         */
    /*       if nSctsPerBlk is 128, nSftSctsPerBlk is 7         */
    /*       if nSctsPerBlk is 32 , nSftSctsPerBlk is 5         */
    /*       if nSctsPerBlk is 16,  nSftSctsPerBlk is 4         */
    /*       There is no other case.                            */
    /*                                                          */
    /*    nVbn    = nVsn >> pstVol->nSftSctsPerBlk;             */
    /*    nSecOff = nVsn &  (pstVol->nSctsPerBlk - 1);          */
    /*                                                          */
    /*    Translates nVbn to nSbn                               */
    /*       nNumOfDev == 1 ? nSftnNumOfDev = 0                 */
    /*       nNumOfDev == 2 ? nSftnNumOfDev = 1                 */
    /*       nNumOfDev == 4 ? nSftnNumOfDev = 2                 */
    /*                                                          */
    /*    nSbn  = nVbn >> pstVol->nSftnNumOfDev;                */
    /*                                                          */
    /*    nPDev = (nVol << DEVS_PER_VOL_SHIFT) +                */
    /*            (nVbn & (pstVol->nNumOfDev - 1));             */
    /*                                                          */
    /*    Input  : nVol / nVsn    / pstVol                      */
    /*    Output : nVbn / nSecOff / nSbn / nPDev                */
    /* -------------------------------------------------------- */
    TRANS_VA_SA(nVol, nVsn, pstVol, nVbn, nSecOff, nSbn, nPDev);

    pstDev  = GET_DEVCXT(nPDev);

#if defined(CHK_SECTOR_VALIDATION)
    /* check sector boundary */
    if (pstVol->nSctsPerPg < nSctsInPg)
    {
        BIF_ERR_PRINT((TEXT("[BIF:ERR]  Invalid access(nVol:%d,Vsn:%d)\r\n"), nVol, nVsn));
        BIF_LOG_PRINT((TEXT("[BIF:OUT] --tbml_read() nRe=0x%x\r\n"), BML_INVALID_PARAM));

        return BML_INVALID_PARAM;
    }

    if (pstVol->nNumOfUsBlks <= nVbn)
    {
        BIF_ERR_PRINT((TEXT("[BIF:ERR]  Out of bound(nVol:%d,Vsn:%d)\r\n"), nVol, nVsn));
        BIF_LOG_PRINT((TEXT("[BIF:OUT] --tbml_read() nRe=0x%x\r\n"), BML_INVALID_PARAM));

        return BML_INVALID_PARAM;
    }
#endif /* CHK_SECTOR_VALIDATION */

    /* lock the device */
    if (AD_AcquireSM(pstVol->nSm) == FALSE32)
    {
        BIF_ERR_PRINT((TEXT("[BIF:ERR]  Acquiring semaphore is failed\r\n")));
        BIF_LOG_PRINT((TEXT("[BIF:OUT] --tbml_read() nRe=0x%x\r\n"), BML_ACQUIRE_SM_ERROR));

        return BML_ACQUIRE_SM_ERROR;
    }

    bExit = FALSE32;

    do
    {
        /* translate semi-physical address to physical address */
        GET_PBN(nPbn, nSbn, pstDev->stRsv.pBUMap, &(pstDev->stRsv.stBMI));

        pTmpSBuf = pSBuf;


        /* if pSBuf is NULL */
        if (pSBuf == NULL)
        {
        }

        /* do physical-read */
        nLLDRe = pstVol->lld_read(nPDev,
                              (nPbn << pstVol->nSftSctsPerBlk) + nSecOff,
                              nSctsInPg,
                              pMBuf,
                              pTmpSBuf,
                              nFlag);

        if (nLLDRe == LLD_SUCCESS)
        {
            bExit = TRUE32;
        }
        else
        {
            nMajorErr = XSR_RETURN_MAJOR(nLLDRe);
            nMinorErr = XSR_RETURN_MINOR(nLLDRe);

            switch (nMajorErr)
            {
            case LLD_READ_ERROR:

                BIF_INF_PRINT((TEXT("[BIF:ERR]  Uncorrectable Read Error Occurs during Read (LLDErr=0x%x)\r\n"),
                               nLLDRe));
                BIF_INF_PRINT((TEXT("[BIF:ERR]  at Vsn = %d, Psn = %d, NumberOfScts = %d \r\n"),
                               nVsn, (nPbn << pstVol->nSftSctsPerBlk) + nSecOff, nSctsInPg));

                nBMLRe = BML_READ_ERROR | XSR_RETURN_MINOR(nLLDRe);

                bExit = TRUE32;
                break;

            case LLD_READ_DISTURBANCE:
               bExit = TRUE32;
                break;

            default:
                bExit  = TRUE32;
                nBMLRe = BML_CRITICAL_ERROR;
                BIF_ERR_PRINT((TEXT("[BIF:ERR]  Unexpected nLLDRe=0x%X\r\n"), nLLDRe));
                break;
            }
        }

    } while (bExit == FALSE32);

    if (AD_ReleaseSM(pstVol->nSm) == FALSE32)
    {
        BIF_ERR_PRINT((TEXT("[BIF:ERR]  Releasing semaphore is failed\r\n")));
        BIF_LOG_PRINT((TEXT("[BIF:OUT] --tbml_read() nRe=0x%x\r\n"), BML_RELEASE_SM_ERROR));

        return BML_RELEASE_SM_ERROR;
    }

    BIF_LOG_PRINT((TEXT("[BIF:OUT] --tbml_read() nRe=0x%x\r\n"), nBMLRe));

    return nBMLRe;
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      tbml_mread                                                            */
/* DESCRIPTION                                                               */
/*      This function reads virtual sectors                                  */
/* PARAMETERS                                                                */
/*      nVol       [IN]                                                      */
/*            Volume number                                                  */
/*      nVsn       [IN]                                                      */
/*            First virtual sector number for reading                        */
/*      nSctsInPg  [IN]                                                      */
/*            The number of sectors to read                                  */
/*      pMBuf      [IN]                                                      */
/*            Main sector buffer for reading                                 */
/*      pSBuf      [IN]                                                      */
/*            Spare sector buffer for reading                                */
/*      nFlag      [IN]                                                      */
/*            BML_FLAG_ECC_ON                                                */
/*            BML_FLAG_ECC_OFF                                               */
/*            BML_FLAG_BBM_ON                                                */
/*            BML_FLAG_BBM_OFF                                               */
/* RETURN VALUES                                                             */
/*      BML_SUCCESS                                                          */
/*            tbml_read is completed                                          */
/*      BML_READ_ERROR                                                       */
/*            read data has the uncorrectable read error                     */
/*      BML_INVALID_DATA_ERROR                                               */
/*            read data already has the uncorrectable read error             */
/*      BML_CRITICAL_ERROR                                                   */
/*            critical error                                                 */
/*      BML_INVALID_PARAM                                                    */
/*            invalid parameter                                              */
/*      BML_ACQUIRE_SM_ERROR                                                 */
/*            acquiring semaphore is failed                                  */
/*      BML_RELEASE_SM_ERROR                                                 */
/*            releasing semaphore is failed                                  */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
INT32
tbml_mread(UINT32 nVol, UINT32 nVsn, UINT32 nNumOfScts,
         UINT8 *pMBuf, UINT8 *pSBuf, UINT32 nFlag)
{
    INT32       nBMLRe = BML_SUCCESS;
    SGL         stSGL;

    BIF_LOG_PRINT((TEXT("[BIF: IN] ++tbml_mread(nVol:%d,Vsn:%d,nNumOfScts:%d,Flag:0x%x)\r\n"),
               nVol, nVsn, nNumOfScts, nFlag));

    /* fills SGL argument for write operation */
    stSGL.stSGLEntry[0].nFlag       = SGL_ENTRY_USER_DATA;
    stSGL.stSGLEntry[0].nSectors    = nNumOfScts;
    stSGL.stSGLEntry[0].pBuf        = pMBuf;
    stSGL.nElements                 = 1;

    nBMLRe = tbml_sgl_read(nVol, nVsn, nNumOfScts, &stSGL, pSBuf, nFlag);

    BIF_LOG_PRINT((TEXT("[BIF:OUT] --tbml_mread() nRe=0x%x\r\n"), nBMLRe));

    return nBMLRe;
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      tbml_sgl_read                                                          */
/* DESCRIPTION                                                               */
/*      This function reads virtual sectors                                  */
/* PARAMETERS                                                                */
/*      nVol       [IN]                                                      */
/*            Volume number                                                  */
/*      nVsn       [IN]                                                      */
/*            First virtual sector number for reading                        */
/*      nSctsInPg  [IN]                                                      */
/*            The number of sectors to read                                  */
/*      pstSGL     [IN]                                                      */
/*            Main sector buffer for reading                                 */
/*      pSBuf      [IN]                                                      */
/*            Spare sector buffer for reading                                */
/*      nFlag      [IN]                                                      */
/*            BML_FLAG_ECC_ON                                                */
/*            BML_FLAG_ECC_OFF                                               */
/*            BML_FLAG_BBM_ON                                                */
/*            BML_FLAG_BBM_OFF                                               */
/* RETURN VALUES                                                             */
/*      BML_SUCCESS                                                          */
/*            tbml_sgl_read is completed                                       */
/*      BML_READ_ERROR                                                       */
/*            read data has the uncorrectable read error                     */
/*      BML_INVALID_DATA_ERROR                                               */
/*            read data already has the uncorrectable read error             */
/*      BML_CRITICAL_ERROR                                                   */
/*            critical error                                                 */
/*      BML_INVALID_PARAM                                                    */
/*            invalid parameter                                              */
/*      BML_ACQUIRE_SM_ERROR                                                 */
/*            acquiring semaphore is failed                                  */
/*      BML_RELEASE_SM_ERROR                                                 */
/*            releasing semaphore is failed                                  */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
INT32
tbml_sgl_read(UINT32 nVol, UINT32 nVsn, UINT32 nNumOfScts,
            SGL *pstSGL, UINT8 *pSBuf, UINT32 nFlag)
{
    INT32       nBMLRe = BML_SUCCESS;
    INT32       nLLDRe;       /* LLD return value                          */
    INT32       nMajorErr = 0;
    INT32       nMinorErr = 0;
    UINT8      *pTmpSBuf;
    UINT8      *pTmpMBuf = NULL;
    UINT32      nReadVsn;
    UINT32      nReadScts;
    UINT32      nVbn;         /* virtual block number                      */
    UINT32      nSecOff;      /* sector offset                             */
    UINT32      nSbn;         /* semi physical block number                */
    UINT32      nPDev;        /* physical device number                    */
    UINT32      nPbn;         /* physical block number                     */
    BOOL32      bExit = FALSE32;

    register    flash_vol *pstVol;
    register    flash_dev *pstDev;


    BIF_LOG_PRINT((TEXT("[BIF: IN] ++tbml_sgl_read(nVol:%d,Vsn:%d,nNumOfScts:%d,Flag:0x%x)\r\n"),
                   nVol, nVsn, nNumOfScts, nFlag));

    CHK_VOL_RANGE(nVol);

    pstVol = GET_VOLCXT(nVol);

    CHK_VOL_OPEN(pstVol->bVolOpen);

    /* lock the device */
    if (AD_AcquireSM(pstVol->nSm) == FALSE32)
    {
        BIF_ERR_PRINT((TEXT("[BIF:ERR]  Acquiring semaphore is failed\r\n")));
        BIF_LOG_PRINT((TEXT("[BIF:OUT] --tbml_sgl_read() nRe=0x%x\r\n"), BML_ACQUIRE_SM_ERROR));

        return BML_ACQUIRE_SM_ERROR;
    }

    if (pstSGL != NULL)
    {
        pTmpMBuf = pstSGL->stSGLEntry[0].pBuf;
    }

    nReadVsn   = nVsn;

    TRANS_VA_SA(nVol, nReadVsn, pstVol, nVbn, nSecOff, nSbn, nPDev);

    nReadScts  = ((nVbn + 1) << pstVol->nSftSctsPerBlk) - nReadVsn;

    if (nReadScts > nNumOfScts)
        nReadScts = nNumOfScts;

    while (nNumOfScts > 0)
    {
#if defined(CHK_SECTOR_VALIDATION)
        if (pstVol->nNumOfUsBlks <= nVbn)
        {
            BIF_ERR_PRINT((TEXT("[BIF:ERR]  Out of bound(nVol:%d,Vsn:%d)\r\n"), nVol, nVsn));
            nBMLRe = BML_INVALID_PARAM;
            break;
        }
#endif /* CHK_SECTOR_VALIDATION */

        pstDev  = GET_DEVCXT(nPDev);

        /* translate semi-physical address to physical address */
        GET_PBN(nPbn, nSbn, pstDev->stRsv.pBUMap, &(pstDev->stRsv.stBMI));

        pTmpSBuf = pSBuf;

        nLLDRe = pstVol->lld_mread(nPDev,
                               (nPbn << pstVol->nSftSctsPerBlk) + nSecOff,
                                nReadScts,
                                pstSGL,
                                pTmpSBuf,
                                nFlag);

        if (nLLDRe == LLD_SUCCESS)
        {
        }

        if (nLLDRe != LLD_SUCCESS)
        {
            nMajorErr = XSR_RETURN_MAJOR(nLLDRe);
            nMinorErr = XSR_RETURN_MINOR(nLLDRe);
            /* ------------------------------------------------ */
            /* NOTE                                             */
            /*  LLD only returns the uncorrectable read error.  */
            /*  LLD doesn't return the correctable read error.  */
            /*                                                  */
            /*  If LLD returns the uncorrectable read error,    */
            /* ------------------------------------------------ */

            switch (nMajorErr)
            {
                case LLD_READ_ERROR:

                    BIF_INF_PRINT((TEXT("[BIF:ERR]  Uncorrectable Read Error Occurs during MRead (LLDErr=0x%x)\r\n"),
                                   nLLDRe));
                    BIF_INF_PRINT((TEXT("[BIF:ERR]  at Vsn = %d, Psn = %d, NumberOfScts = %d \r\n"),
                                   nReadVsn, (nPbn << pstVol->nSftSctsPerBlk) + nSecOff, nReadScts));

                    nBMLRe = BML_READ_ERROR;

                    break;

                case LLD_READ_DISTURBANCE:

                    break;

               default:

                    nBMLRe = BML_CRITICAL_ERROR;
                    BIF_ERR_PRINT((TEXT("[BIF:ERR]  Unexpected nLLDRe=0x%X\r\n"), nLLDRe));

                    bExit = TRUE32;

                    break;
            } /* switch(nMajorErr) */
        } /* if (nLLDRe != LLD_SUCCESS) */

        if (bExit == TRUE32)
        {
            break;
        }

        if (nMinorErr == LLD_PREV_OP_RESULT)
        {
            nMinorErr = 0;
            continue;
        }

        if (pstSGL != NULL)
        {
            _TrimMainBuf(nReadScts, pstSGL);
        }

        if (pSBuf != NULL)
        {
            pSBuf += (nReadScts * XSR_SPARE_SIZE);
        }

        nNumOfScts -= nReadScts;
        nReadVsn   += nReadScts;

        TRANS_VA_SA(nVol, nReadVsn, pstVol, nVbn, nSecOff, nSbn, nPDev);

        nReadScts  = ((nVbn + 1) << pstVol->nSftSctsPerBlk) - nReadVsn;
        if (nReadScts > nNumOfScts)
            nReadScts = nNumOfScts;
    }

    if (pstSGL != NULL)
    {
        pstSGL->stSGLEntry[0].pBuf = pTmpMBuf;
    }

    if (AD_ReleaseSM(pstVol->nSm) == FALSE32)
    {
        BIF_ERR_PRINT((TEXT("[BIF:ERR]  Releasing semaphore is failed\r\n")));
        BIF_LOG_PRINT((TEXT("[BIF:OUT] --tbml_read() nRe=0x%x\r\n"), BML_RELEASE_SM_ERROR));

        return BML_RELEASE_SM_ERROR;
    }

    BIF_LOG_PRINT((TEXT("[BIF:OUT] --tbml_sgl_read() nRe=0x%x\r\n"), nBMLRe));

    return nBMLRe;
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      tbml_flushop                                                          */
/* DESCRIPTION                                                               */
/*      This function flushes the current operation in the given volume.     */
/* PARAMETERS                                                                */
/*      nVol       [IN]                                                      */
/*            Volume number                                                  */
/*      nFlag      [IN]                                                      */
/*            If nFlag is BML_NORMAL_MODE, it handles the LLD error.         */
/*            If nFlag is BML_EMERGENCY_MODE, it handles the LLD error.      */
/* RETURN VALUES                                                             */
/*      BML_SUCCESS                                                          */
/*          tbml_flushop is completed,                                        */
/*      BML_CRITICAL_ERROR                                                   */
/*          critical error occurs                                            */
/*      BML_INVALID_PARAM                                                    */
/*          If volume data structure isn't initilized, it returns            */
/*      BML_ACQUIRE_SM_ERROR                                                 */
/*          acquiring semaphore is failed                                    */
/*      BML_RELEASE_SM_ERROR                                                 */
/*          releasing semaphore is failed                                    */
/* NOTES                                                                     */
/*      if nFlag is BML_EMERGENCY_MODE, tbml_flushop waits for 2 * erase time */
/*      maximally.                                                           */
/*                                                                           */
/*****************************************************************************/
INT32
tbml_flush(UINT32 nVol, UINT32 nFlag)
{
    INT32    nLLDRe;
    INT32    nMajorErr;
    INT32    nMinorErr;
    INT32    nBMLRe = BML_SUCCESS;
    UINT32   nDevIdx;
    UINT32   nPDev;

    register flash_vol *pstVol;
    register flash_dev *pstDev;

    BIF_LOG_PRINT((TEXT("[BIF: IN] ++tbml_flushop(nVol:%d,nFlag:0x%x)\r\n"),
        nVol, nFlag));

    CHK_VOL_RANGE(nVol);

    pstVol = GET_VOLCXT(nVol);

    CHK_VOL_OPEN(pstVol->bVolOpen);

    /* lock the device */
    if (AD_AcquireSM(pstVol->nSm) == FALSE32)
    {
        BIF_ERR_PRINT((TEXT("[BIF:ERR]  Acquiring semaphore is failed\r\n")));
        BIF_LOG_PRINT((TEXT("[BIF:OUT] --tbml_flushop() nRe=0x%x\r\n"), BML_ACQUIRE_SM_ERROR));

        return BML_ACQUIRE_SM_ERROR;
    }

    for (nDevIdx = 0; nDevIdx < DEVS_PER_VOL; nDevIdx++)
    {
        nPDev = nVol * DEVS_PER_VOL + nDevIdx;

        if (_IsOpenedDev(nPDev) == FALSE32)
            continue;

        pstDev = GET_DEVCXT(nPDev);
        nLLDRe = pstVol->lld_flush(nPDev);

        if ((BML_NORMAL_MODE & nFlag) == BML_NORMAL_MODE)
        {
            nMajorErr = XSR_RETURN_MAJOR(nLLDRe);
            nMinorErr = XSR_RETURN_MINOR(nLLDRe);

            switch (nMajorErr)
            {
            case LLD_SUCCESS:
                break;

            default:
                nBMLRe = BML_CRITICAL_ERROR;
                BIF_ERR_PRINT((TEXT("[BIF:ERR]  Unexpected nLLDRe=0x%X\r\n"), nLLDRe));
                break;
            }
        }

        if (nBMLRe == BML_CRITICAL_ERROR)
            break;
    }

    if (AD_ReleaseSM(pstVol->nSm) == FALSE32)
    {
        BIF_ERR_PRINT((TEXT("[BIF:ERR]  Releasing semaphore is failed\r\n")));
        BIF_LOG_PRINT((TEXT("[BIF:OUT] --tbml_flushop() nRe=0x%x\r\n"), BML_RELEASE_SM_ERROR));

        return BML_RELEASE_SM_ERROR;
    }

    BIF_LOG_PRINT((TEXT("[BIF:OUT] --tbml_flushop() nRe=0x%x\r\n"), nBMLRe));

    return nBMLRe;
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      tbml_load_partition                                                      */
/* DESCRIPTION                                                               */
/*      This function loads Partition Entry that corresponds with the given  */
/*      nID.                                                                 */
/* PARAMETERS                                                                */
/*      nVol       [IN]                                                      */
/*            volume number                                                  */
/*      nID        [IN]                                                      */
/*            ID of Partition Entry                                          */
/*      pstPartEntry [OUT]                                                   */
/*            pointer of XSRPartEntry structure                              */
/* RETURN VALUES                                                             */
/*      BML_SUCCESS                                                          */
/*            tbml_load_partition is completed                                   */
/*      BML_NO_PIENTRY                                                       */
/*            there is no partion information that corresponds with          */
/*            the given nID                                                  */
/*      BML_INVALID_PARAM                                                    */
/*            invalid parameters                                             */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
INT32
tbml_load_partition(UINT32 nVol, UINT32 nID, XSRPartEntry *pstPartEntry)
{
    UINT32    nCnt;
    BOOL32    bFound;
    INT32     nBMLRe;

    register flash_vol *pstVol;
    register part_info *pstPI;

    BIF_LOG_PRINT((TEXT("[BIF: IN] ++tbml_load_partition(nVol:%d,nID:0x%x)\r\n"),
        nVol, nID));

    CHK_VOL_RANGE(nVol);

    pstVol = GET_VOLCXT(nVol);

    CHK_VOL_OPEN(pstVol->bVolOpen);

    pstPI  = pstVol->pstPartI;
    nBMLRe = BML_SUCCESS;
    bFound = FALSE32;

    if (pstPartEntry == NULL)
    {
        BIF_ERR_PRINT((TEXT("[BIF:ERR]  pstPartEntry is NULL\r\n")));
        BIF_LOG_PRINT((TEXT("[BIF:OUT] --tbml_load_partition() nRe=0x%x\r\n"), BML_INVALID_PARAM));

        return BML_INVALID_PARAM;
    }

    for (nCnt = 0; nCnt < pstPI->nNumOfPartEntry; nCnt++)
    {
        if (pstPI->stPEntry[nCnt].nID == nID)
        {
            AD_Memcpy( pstPartEntry,
                        &(pstPI->stPEntry[nCnt]),
                        sizeof(XSRPartEntry));

            bFound = TRUE32;
            break;
        }
    }

    if (bFound == FALSE32)
    {
        BIF_ERR_PRINT((TEXT("[BIF:ERR]  NO PartEntry(ID:0x%x)\r\n"), nID));
        nBMLRe = BML_NO_PIENTRY;
    }

    BIF_LOG_PRINT((TEXT("[BIF:OUT] --tbml_load_partition() nRe=0x%x\r\n"), nBMLRe));

    return nBMLRe;
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      BML_LoadPIExt                                                        */
/* DESCRIPTION                                                               */
/*      This function loads Partition Information Extension from PIA.        */
/* PARAMETERS                                                                */
/*      nVol       [IN]                                                      */
/*            volume number                                                  */
/*      nID        [IN]                                                      */
/*            ID of Partition Information Extension Entry                    */
/*      pPIExt     [OUT]                                                     */
/*            pointer of part_exinfo structure                                  */
/* RETURN VALUES                                                             */
/*      BML_SUCCESS                                                          */
/*            BML_LoadPIExt is completed                                     */
/*      BML_LOAD_PIEXT_ERROR                                                 */
/*            BML_LoadPIExt is failed                                        */
/*      BML_INVALID_PARAM                                                    */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
INT32
BML_LoadPIExt(UINT32 nVol, UINT32 nID, part_exinfo   *pPIExt)
{
    INT32    nRe = BML_SUCCESS;

    register flash_vol *pstVol;

    BIF_LOG_PRINT((TEXT("[BIF: IN] ++BML_LoadPIExt(nVol:%d,nID:0x%x)\r\n"),
                   nVol, nID));

    CHK_VOL_RANGE(nVol);

    pstVol = GET_VOLCXT(nVol);

    CHK_VOL_OPEN(pstVol->bVolOpen);

    do {
        if (pPIExt == NULL)
        {
            BIF_ERR_PRINT((TEXT("[BIF:ERR]  pPIExt is NULL\r\n")));

            nRe = BML_INVALID_PARAM;
            break;
        }

        if (pstVol->pPIExt->nID != nID)
        {
            BIF_ERR_PRINT((TEXT("[BIF:ERR]  NO ID(0x%x)\r\n"), nID));

            nRe = BML_NO_PIENTRY;
            break;
        }

        pPIExt->nID         = nID;
        pPIExt->nSizeOfData = pstVol->pPIExt->nSizeOfData;

        /* if pstVol->pPIExt->nSizeOfData is greater than BML_MAX_PIEXT_DATA,
           there is no data. Thus, set pstVol->pPIExt->nSizeOfData as 0 */
        if (pPIExt->nSizeOfData > BML_MAX_PIEXT_DATA)
            pPIExt->nSizeOfData = 0;

        AD_Memcpy(pPIExt->pData, pstVol->pPIExt->pData, pPIExt->nSizeOfData);

    } while(0);

    BIF_LOG_PRINT((TEXT("[BIF:OUT] --BML_LoadPIExt() nRe=0x%x\r\n"), nRe));

    return nRe;
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      tbml_ioctl                                                            */
/* DESCRIPTION                                                               */
/*      This function provides a generic I/O control code (IOCTL) for getting*/
/*      or storing information.                                              */
/* PARAMETERS                                                                */
/*      nVol       [IN]                                                      */
/*            volume number                                                  */
/*      nCode      [IN]                                                      */
/*           IO control code, which can be pre-defined. For more information,*/
/*           see 'nCode value of BML_IOControl' of BML header file.          */
/*           there are 8 IO control codes as follows.                        */
/*           BML_IOCTL_LOCK_FOREVER                                          */
/*           BML_IOCTL_SET_BLOCK_LOCK                                        */
/*           BML_IOCTL_UNLOCK_WHOLEAREA                                      */
/*           BML_IOCTL_CHANGE_PART_ATTR                                      */
/*           BML_IOCTL_GET_BMI                                               */
/*           BML_IOCTL_GET_INVALID_BLOCK_CNT                                 */
/*           BML_IOCTL_GET_FULL_PI                                           */
/*           BML_IOCTL_SET_WTIME_FOR_ERR                                     */
/*      pBufIn     [IN]                                                      */
/*           Pointer to the input buffer that can contain additional         */
/*           information associated with the nCode parameter.                */
/*      nLenIn     [IN]                                                      */
/*           Size in bytes of pBufIn                                         */
/*      pBufOut    [OUT]                                                     */
/*           Pointer to the output buffer supplied by the caller             */
/*      nLenOut    [IN]                                                      */
/*           Specifies the maximum number of bytes that can be returned in   */
/*           lpOutBuf. The caller sets this value.                           */
/*      pBytesReturned [OUT]                                                 */
/*           Number of bytes returned in lpOutBuf.                           */
/* RETURN VALUES                                                             */
/*      If tbml_ioctl is completed, it returns BML_SUCCESS.                   */
/*      Otherwise, it returns the error number, which corresponds to nCode.  */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
INT32
tbml_ioctl(UINT32  nVol,     UINT32  nCode,
          UINT8  *pBufIn,   UINT32  nLenIn,
          UINT8  *pBufOut,  UINT32  nLenOut,
          UINT32 *pBytesReturned)
{
    INT32     nRe;
    INT32     nLLDRe;
    flash_vol *pstVol;
    flash_dev *pstDev;

    CHK_VOL_RANGE(nVol);

    pstVol = GET_VOLCXT(nVol);

    CHK_VOL_OPEN(pstVol->bVolOpen);

    BIF_LOG_PRINT((TEXT("[BIF: IN] ++tbml_ioctl(nVol:%d,nCode:0x%x)\r\n"), nVol, nCode));


    if (AD_AcquireSM(pstVol->nSm) == FALSE32)
    {
        BIF_ERR_PRINT((TEXT("[BIF:ERR]  Acquiring semaphore is failed\r\n")));
        BIF_LOG_PRINT((TEXT("[BIF:OUT] --tbml_ioctl() nRe=0x%x\r\n"), BML_ACQUIRE_SM_ERROR));

        return BML_ACQUIRE_SM_ERROR;
    }

    nRe = BML_SUCCESS;
    switch (nCode)
    {
    case BML_IOCTL_LOCK_FOREVER:
        {
            UINT32    nPDev;
            UINT32    nByteRet;
            BOOL32    bComplete = TRUE32;
            UINT32    nPbn;
            UINT32    nSbn;
            UINT32    nBlks;
            UINT32    nCnt;

            BIF_INF_PRINT((TEXT("[BIF:MSG]  nCode = BML_IOCTL_LOCK_FOREVER\r\n")));

            nPDev = *(UINT32*)pBufIn;
            pBufIn += sizeof(nPDev);

            pstDev = GET_DEVCXT(nPDev);

            do
            {
                if (_IsOpenedDev(nPDev) == FALSE32)
                {
                    bComplete = FALSE32;
                    break;
                }

                nLLDRe = pstVol->lld_ioctl(nPDev,
                                       LLD_IOC_SET_SECURE_LT,
                                       pBufIn, nLenIn,
                                       NULL, 0,
                                       &nByteRet);

                if (nLLDRe != LLD_SUCCESS)
                {
                    BIF_ERR_PRINT((TEXT("[BIF:ERR]  LLD_IOCtl(PDev:%d,LLD_IOC_SET_SECURE_LT) is failed\r\n"),
                                   nPDev));
                    BIF_ERR_PRINT((TEXT("[BIF:ERR]  ErrCode=0x%x\r\n"), nLLDRe));
                    bComplete = FALSE32;
                    break;
                }

                nSbn  = *(UINT32*)pBufIn;
                nBlks = *((UINT32*)(pBufIn+4));

                for (nCnt = 0; nCnt < nBlks; nCnt++)
                {
                    nPbn = _GetPbn((nSbn + nCnt), pstDev->stRsv.pBUMap, &(pstDev->stRsv.stBMI));

                    if (nPbn != (nSbn + nCnt))
                    {
                        *(UINT32*)pBufIn = nPbn;
                        *((UINT32*)(pBufIn+4)) = 1;

                        nLLDRe = pstVol->lld_ioctl(nPDev,
                                               LLD_IOC_SET_SECURE_LT,
                                               pBufIn, nLenIn,
                                               NULL, 0,
                                               &nByteRet);

                        if (nLLDRe != LLD_SUCCESS)
                        {
                            BIF_ERR_PRINT((TEXT("[BIF:ERR]  LLD_IOCtl(PDev:%d,LLD_IOC_SET_SECURE_LT) is failed\r\n"),
                                           nPDev));
                            BIF_ERR_PRINT((TEXT("[BIF:ERR]  ErrCode=0x%x\r\n"), nLLDRe));
                            bComplete = FALSE32;
                            break;
                        }
                    }
                }

            } while(0);

            if (pBytesReturned != NULL)
            {
                *pBytesReturned = 0;
            }

            if (bComplete == TRUE32)
            {
                BIF_INF_PRINT((TEXT("[BIF:MSG]  Lock-tighting is completed\r\n")));
            }
            else
            {
                BIF_INF_PRINT((TEXT("[BIF:MSG]  Lock-tighting is failed\r\n")));
                nRe = BML_CANT_LOCK_FOREVER;
            }
        }
        break;

    case BML_IOCTL_SET_BLOCK_LOCK:
        {
            UINT32    nPDev;
            UINT32    nByteRet;
            BOOL32    bComplete = TRUE32;
            UINT32    nPbn;
            UINT32    nSbn;
            UINT32    nBlks;
            UINT32    nCnt;

            BIF_INF_PRINT((TEXT("[BIF:MSG]  nCode = BML_IOCTL_SET_BLOCK_LOCK\r\n")));


            nPDev = *(UINT32*)pBufIn;
            pBufIn += sizeof(nPDev);

            pstDev = GET_DEVCXT(nPDev);

            do
            {
                if (_IsOpenedDev(nPDev) == FALSE32)
                {
                    bComplete = FALSE32;
                    break;
                }

                nLLDRe = pstVol->lld_ioctl(nPDev,
                                       LLD_IOC_SET_BLOCK_LOCK,
                                       pBufIn, nLenIn,
                                       NULL, 0,
                                       &nByteRet);

                if (nLLDRe != LLD_SUCCESS)
                {
                    BIF_ERR_PRINT((TEXT("[BIF:ERR]  LLD_IOCtl(PDev:%d,LLD_IOC_SET_BLOCK_LOCK) is failed\r\n"),
                                   nPDev));
                    BIF_ERR_PRINT((TEXT("[BIF:ERR]  ErrCode=0x%x\r\n"), nLLDRe));
                    bComplete = FALSE32;
                    break;
                }

                nSbn  = *(UINT32*)pBufIn;
                nBlks = *((UINT32*)(pBufIn+4));

                for (nCnt = 0; nCnt < nBlks; nCnt++)
                {
                    nPbn = _GetPbn((nSbn + nCnt), pstDev->stRsv.pBUMap, &(pstDev->stRsv.stBMI));
                    if (nPbn != (nSbn + nCnt))
                    {
                        *(UINT32*)pBufIn = nPbn;
                        *((UINT32*)(pBufIn+4)) = 1;

                        nLLDRe = pstVol->lld_ioctl(nPDev,
                                               LLD_IOC_SET_BLOCK_LOCK,
                                               pBufIn, nLenIn,
                                               NULL, 0,
                                               &nByteRet);

                        if (nLLDRe != LLD_SUCCESS)
                        {
                            BIF_ERR_PRINT((TEXT("[BIF:ERR]  LLD_IOCtl(PDev:%d,LLD_IOC_SET_BLOCK_LOCK) is failed\r\n"),
                                           nPDev));
                            BIF_ERR_PRINT((TEXT("[BIF:ERR]  ErrCode=0x%x\r\n"), nLLDRe));
                            bComplete = FALSE32;
                            break;
                        }
                    }
                }

            } while(0);

            if (pBytesReturned != NULL)
            {
                *pBytesReturned = 0;
            }

            if (bComplete == TRUE32)
            {
                BIF_INF_PRINT((TEXT("[BIF:MSG]  Lock is completed\r\n")));
            }
            else
            {
                BIF_INF_PRINT((TEXT("[BIF:MSG]  Lock is failed\r\n")));
                nRe = BML_CANT_LOCK_BLOCK;
            }
        }
        break;

    case BML_IOCTL_SET_BLOCK_UNLOCK:
        {
            BOOL32    bComplete = TRUE32;
            UINT32    nPDev;
            UINT32    nPbn;
            UINT32    nSbn;
            UINT32    nBlks;
            UINT32    nCnt;


            BIF_INF_PRINT((TEXT("[BIF:MSG]  nCode = BML_IOCTL_SET_BLOCK_UNLOCK\r\n")));

            nPDev   = *(UINT32*)pBufIn;
            pBufIn += sizeof(nPDev);
            nSbn    = *(UINT32*)pBufIn;
            pBufIn += sizeof(nSbn);
            nBlks   = *(UINT32*)pBufIn;

            do
            {
                if (_IsOpenedDev(nPDev) == FALSE32)
                {
                    bComplete = FALSE32;
                    break;
                }

                pstDev    = GET_DEVCXT(nPDev);

                /* unlock the whole area */
                nLLDRe = pstVol->lld_set_rw(nPDev, nSbn, nBlks);

                if (nLLDRe != LLD_SUCCESS)
                {
                    BIF_ERR_PRINT((TEXT("[BIF:ERR]  LLD_SetRWArea(PDev:%d) is failed\r\n"), nPDev));
                    bComplete = FALSE32;
                    break;
                }

                for (nCnt = 0; nCnt < nBlks; nCnt++)
                {
                    nPbn = _GetPbn((nSbn + nCnt), pstDev->stRsv.pBUMap, &(pstDev->stRsv.stBMI));
                    if (nPbn != (nSbn + nCnt))
                    {
                        /* unlock the whole area */
                        nLLDRe = pstVol->lld_set_rw(nPDev, nPbn, 1);

                        if (nLLDRe != LLD_SUCCESS)
                        {
                            BIF_ERR_PRINT((TEXT("[BIF:ERR]  LLD_SetRWArea(PDev:%d) is failed\r\n"), nPDev));
                            bComplete = FALSE32;
                            break;
                        }
                    }
                }
            } while(0);

            if (bComplete == TRUE32)
            {
                BIF_INF_PRINT((TEXT("[BIF:MSG]  Unlock is completed\r\n")));
            }
            else
            {
                nRe = BML_CANT_UNLOCK_BLOCK;
                BIF_ERR_PRINT((TEXT("[BIF:ERR]  Unlock is failed\r\n")));
            }

            if (pBytesReturned != NULL)
            {
                *pBytesReturned = 0;
            }
        }
        break;
#if defined(XSR_NW)
    case BML_IOCTL_UNLOCK_WHOLEAREA:
        {
            BOOL32    bComplete;
            UINT32    nPDev;
            UINT32    nIdx;
            UINT32    nByteRet;
            UINT16    nLockStat;
            flash_dev *pstDev;

            BIF_INF_PRINT((TEXT("[BIF:MSG]  nCode = BML_IOCTL_UNLOCK_WHOLEAREA\r\n")));

            bComplete = FALSE32;
            for (nIdx = 0; nIdx < DEVS_PER_VOL; nIdx++)
            {
                nPDev     = nVol * DEVS_PER_VOL + nIdx;

                if (_IsOpenedDev(nPDev) == FALSE32)
                    continue;

                bComplete = FALSE32;

                pstDev    = GET_DEVCXT(nPDev);

                nLLDRe = pstVol->lld_ioctl(nPDev,
                                       LLD_IOC_GET_SECURE_STAT,
                                       NULL, 0,
                                       (UINT8*)&nLockStat,
                                       sizeof(UINT16),
                                       &nByteRet);

                if (nLLDRe != LLD_SUCCESS)
                {
                    BIF_ERR_PRINT((TEXT("[BIF:ERR]  LLD_IOCtl(PDev:%d,LLD_IOC_GET_SECURE_STAT) is failed\r\n"),
                                   nPDev));
                    BIF_ERR_PRINT((TEXT("[BIF:ERR]  ErrCode=0x%x\r\n"), nLLDRe));
                    break;
                }

                /* if status is Lock-Tighten, can't unlock whole area */
                if (nLockStat == LLD_IOC_SECURE_LT)
                {
                    BIF_ERR_PRINT((TEXT("[BIF:ERR]  LockStat(PDev:%d) is LockTight\r\n"), nPDev));

                    nRe = BML_VOLUME_ALREADY_LOCKTIGHT;
                    break;
                }

                /* unlock the whole area */
                nLLDRe = pstVol->lld_set_rw(nPDev, 0, pstVol->nBlksPerDev);
                if (nLLDRe != LLD_SUCCESS)
                {
                    BIF_ERR_PRINT((TEXT("[BIF:ERR]  LLD_SetRWArea(PDev:%d) is failed\r\n"), nPDev));
                    break;
                }

                pstDev->n1stPbnOfULArea = 0;
                pstDev->nBlksInULArea   = pstVol->nBlksPerDev;

                bComplete = TRUE32;
            }

            if (bComplete == TRUE32)
            {
                pstVol = GET_VOLCXT(nVol);

                pstVol->bPreProgram = TRUE32;
                BIF_INF_PRINT((TEXT("[BIF:MSG]  Unlocking whole area is completed\r\n")));
            }
            else
            {
                nRe = BML_CANT_UNLOCK_WHOLEAREA;
                BIF_ERR_PRINT((TEXT("[BIF:ERR]  Unlocking whole area is failed\r\n")));
            }

            if (pBytesReturned != NULL)
            {
                *pBytesReturned = 0;
            }
        }
        break;

    case BML_IOCTL_CHANGE_PART_ATTR:
        {
            UINT32    nCnt;
            UINT32    nSpecID;
            UINT32    nSpecAttr;
            BOOL32    bFound;
            part_info *pstPI;

            BIF_INF_PRINT((TEXT("[BIF:MSG]  nCode = BML_IOCTL_CHANGE_PART_ATTR\r\n")));

            if (pBufIn == NULL)
            {
                nRe = BML_INVALID_PARAM;

                BIF_ERR_PRINT((TEXT("[BIF:ERR]  pBufIn is NULL\r\n")));
                break;
            }

            if (nLenIn != (sizeof(UINT32) * 2))
            {
                nRe = BML_INVALID_PARAM;

                BIF_ERR_PRINT((TEXT("[BIF:ERR]  nLenIn isn't equal to %d\r\n"),
                               sizeof(UINT32) * 2));
                break;
            }

            AD_Memcpy(&nSpecID,   pBufIn + 0, sizeof(UINT32));
            AD_Memcpy(&nSpecAttr, pBufIn + 4, sizeof(UINT32));

            BIF_INF_PRINT((TEXT("[BIF:MSG]  nSpecID   : 0x%x\r\n"), nSpecID));
            BIF_INF_PRINT((TEXT("[BIF:MSG]  nSpecAttr : %s\r\n"),
                ((nSpecAttr & BML_PI_ATTR_RO)!= 0) ? TEXT("RO") : TEXT("RW")));

            pstVol = GET_VOLCXT (nVol);
            pstPI  = pstVol->pstPartI;
            bFound = FALSE32;

            for (nCnt = 0; nCnt < pstPI->nNumOfPartEntry; nCnt++)
            {
                if (pstPI->stPEntry[nCnt].nID == nSpecID)
                {
                    if ((pstPI->stPEntry[nCnt].nAttr & BML_PI_ATTR_FROZEN) != 0)
                    {
                        BIF_ERR_PRINT((TEXT("[BIF:ERR]  Partition(ID:0x%x) has Frozen Attribute\r\n"),
                            nSpecID));
                        BIF_ERR_PRINT((TEXT("[BIF:ERR]  Attribute can't be changed\r\n")));
                        break;
                    }

                    BIF_INF_PRINT((TEXT("[BIF:MSG]  nCurrent Attr : %s\r\n"),
                        ((pstPI->stPEntry[nCnt].nAttr & BML_PI_ATTR_RO)!= 0) ? TEXT("RO") : TEXT("RW")));

                    pstPI->stPEntry[nCnt].nAttr &= ~(BML_PI_ATTR_RO | BML_PI_ATTR_RW);

                    if ((nSpecAttr & BML_PI_ATTR_RO) != 0)
                    {
                        pstPI->stPEntry[nCnt].nAttr |=  BML_PI_ATTR_RO;
                    }
                    else if ((nSpecAttr & BML_PI_ATTR_RW) != 0)
                    {
                        pstPI->stPEntry[nCnt].nAttr |=  BML_PI_ATTR_RW;
                    }

                    BIF_INF_PRINT((TEXT("[BIF:MSG]  nChanged Attr : %s\r\n"),
                        ((pstPI->stPEntry[nCnt].nAttr & BML_PI_ATTR_RO)!= 0) ? TEXT("RO") : TEXT("RW")));

                    bFound = TRUE32;
                    break;
                }
            }

            if (bFound == TRUE32)
            {
                nRe = BML_SUCCESS;
            }
            else
            {
                nRe = BML_CANT_CHANGE_PART_ATTR;

                BIF_ERR_PRINT((TEXT("[BIF:ERR]  Can't change nAttr of ID(0x%X)\r\n"),
                    nSpecID));
            }

            if (pBytesReturned != NULL)
            {
                *pBytesReturned = 0;
            }
        }
        break;

#endif /* (XSR_NW) */

#if !defined(XSR_NBL2)
    case BML_IOCTL_GET_BMI:
        {
            UINT32    nDevIdx;
            flash_dev *pstDev;

            BIF_INF_PRINT((TEXT("[BIF:MSG]  nCode = BML_IOCTL_GET_BMI\r\n")));

            if ((pBufIn == NULL) || (pBufOut == NULL))
            {
                nRe = BML_INVALID_PARAM;

                BIF_ERR_PRINT((TEXT("[BIF:ERR]  pBufIn or pBufOut is NULL\r\n")));
                break;
            }

            if (nLenIn != sizeof(UINT32))
            {
                nRe = BML_INVALID_PARAM;

                BIF_ERR_PRINT((TEXT("[BIF:ERR]  nLenIn(%d) isn't equal to %d\r\n"),
                               nLenIn, sizeof(UINT32)));
                break;
            }

            if (nLenOut != (sizeof(UINT16) * 512))
            {
                nRe = BML_INVALID_PARAM;

                BIF_ERR_PRINT((TEXT("[BIF:ERR]  nLenOut(%d) isn't equal to %d\r\n"),
                               nLenOut, sizeof(UINT16) * 512));
                break;
            }

            AD_Memcpy(&nDevIdx, pBufIn, sizeof(UINT32));
            if (nDevIdx >= DEVS_PER_VOL)
            {
                nRe = BML_INVALID_PARAM;

                BIF_ERR_PRINT((TEXT("[BIF:ERR]  nDevIdx(%d) is greater than %d\r\n"),
                               nDevIdx, DEVS_PER_VOL));
                break;
            }

            pstVol = GET_VOLCXT(nVol);
            if (nDevIdx >= pstVol->nNumOfDev)
            {
                nRe = BML_INVALID_PARAM;

                BIF_ERR_PRINT((TEXT("[BIF:ERR]  Volume(%d) has %d device(s)\r\n"),
                               nVol, pstVol->nNumOfDev));
                break;
            }

            AD_Memset(pBufOut, 0xFF, sizeof(UINT16) * 512);

            pstDev = GET_DEVCXT(nVol * DEVS_PER_VOL + nDevIdx);

            AD_Memcpy(pBufOut,
                       pstDev->stRsv.stBMI.pstBMF,
                       sizeof(bmf) * pstDev->stRsv.stBMI.nNumOfBMFs);
            if (pBytesReturned != NULL)
            {
                *pBytesReturned = sizeof(bmf) * pstDev->stRsv.stBMI.nNumOfBMFs;
            }

            BIF_INF_PRINT((TEXT("[BIF:MSG]  %d mapping information\r\n"),
                           pstDev->stRsv.stBMI.nNumOfBMFs));
        }
        break;

    case BML_IOCTL_INVALID_BLOCK_CNT:
        {
            UINT32    nDevIdx;
            UINT32    nIdx;
            UINT32    nByteCnt;
            UINT32    nBitIdx;
            UINT32    nInvalidBlkCnt = 0;
            flash_dev *pstDev;

            BIF_INF_PRINT((TEXT("[BIF:MSG]  nCode = BML_IOCTL_INVALID_BLOCK_CNT\r\n")));

            if (pBufIn == NULL)
            {
                nRe = BML_INVALID_PARAM;

                BIF_ERR_PRINT((TEXT("[BIF:ERR]  pBufIn or pBufOut is NULL\r\n")));
                break;
            }

            if (pBufOut == NULL)
            {
                nRe = BML_INVALID_PARAM;

                BIF_ERR_PRINT((TEXT("[BIF:ERR]  pBufIn or pBufOut is NULL\r\n")));
                break;
            }

            if (nLenIn != sizeof(UINT32))
            {
                nRe = BML_INVALID_PARAM;

                BIF_ERR_PRINT((TEXT("[BIF:ERR]  nLenIn(%d) isn't equal to %d\r\n"),
                               nLenIn, sizeof(UINT32)));
                break;
            }

            if (nLenOut != sizeof(UINT32))
            {
                nRe = BML_INVALID_PARAM;

                BIF_ERR_PRINT((TEXT("[BIF:ERR]  nLenOut(%d) isn't equal to %d\r\n"),
                               nLenOut, sizeof(UINT16) * 512));
                break;
            }

            AD_Memcpy(&nDevIdx, pBufIn, sizeof(UINT32));

            if (nDevIdx >= DEVS_PER_VOL)
            {
                nRe = BML_INVALID_PARAM;

                BIF_ERR_PRINT((TEXT("[BIF:ERR]  nDevIdx(%d) is greater than %d\r\n"),
                               nDevIdx, DEVS_PER_VOL));
                break;
            }

            pstVol = GET_VOLCXT(nVol);
            if (nDevIdx >= pstVol->nNumOfDev)
            {
                nRe = BML_INVALID_PARAM;

                BIF_ERR_PRINT((TEXT("[BIF:ERR]  Volume(%d) has %d device(s)\r\n"),
                               nVol, pstVol->nNumOfDev));
                break;
            }

            AD_Memset(pBufOut, 0xFF, sizeof(UINT32));

            pstDev = GET_DEVCXT(nVol * DEVS_PER_VOL + nDevIdx);

            nByteCnt = (pstVol->nRsvrPerDev / 8) + (((pstVol->nRsvrPerDev % 8)!= 0) ? 1 : 0);

            for (nIdx = 0; nIdx < nByteCnt; nIdx++)
            {
                for (nBitIdx = 0; nBitIdx < 8; nBitIdx++)
                {
                     if (((pstDev->stRsv.pBABitMap[nIdx] >> nBitIdx) & 0x1)!= 0)
                     {
                        nInvalidBlkCnt++;
                     }

                }
            }

            nInvalidBlkCnt -= RSV_META_BLKS;
            AD_Memcpy(pBufOut, &nInvalidBlkCnt, sizeof(UINT32));
            if (pBytesReturned != NULL)
            {
                *pBytesReturned = sizeof(UINT32);
            }

            BIF_INF_PRINT((TEXT("[BIF:MSG]  %d mapping information\r\n"),
                           nInvalidBlkCnt));
        }
        break;

    case BML_IOCTL_GET_FULL_PI:
        {
            BIF_INF_PRINT((TEXT("[BIF:MSG]  nCode = BML_IOCTL_GET_FULL_PI\r\n")));

            if (pBufOut == NULL)
            {
                nRe = BML_INVALID_PARAM;

                BIF_ERR_PRINT((TEXT("[BIF:ERR]  pBufIn is NULL\r\n")));
                break;
            }

            if (nLenOut != sizeof(part_info))
            {
                nRe = BML_INVALID_PARAM;

                BIF_ERR_PRINT((TEXT("[BIF:ERR]  nLenIn(%d) isn't equal to %d\r\n"),
                    nLenIn, sizeof(part_info)));
                break;
            }

            pstVol = GET_VOLCXT(nVol);

            AD_Memcpy(pBufOut, pstVol->pstPartI, sizeof(part_info));

            if (pBytesReturned != NULL)
            {
                *pBytesReturned = sizeof(part_info);
            }
        }

        break;

#endif /* (XSR_NBL2) */

    default:
        BIF_INF_PRINT((TEXT("[BIF:MSG]  Unsupported IOCtl\r\n")));
        nRe = BML_UNSUPPORTED_IOCTL;

        if (pBytesReturned != NULL)
        {
            *pBytesReturned = 0;
        }
        break;
    }

    if (AD_ReleaseSM(pstVol->nSm) == FALSE32)
    {
        BIF_ERR_PRINT((TEXT("[BIF:ERR]  Releasing semaphore is failed\r\n")));
        BIF_LOG_PRINT((TEXT("[BIF:OUT] --tbml_ioctl() nRe=0x%x\r\n"), BML_RELEASE_SM_ERROR));

        return BML_RELEASE_SM_ERROR;
    }

    BIF_LOG_PRINT((TEXT("[BIF:OUT] --tbml_ioctl() nRe=0x%x\r\n"), nRe));

    return nRe;
}



