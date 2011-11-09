/*****************************************************************************/
/*                                                                           */
/* PROJECT : AnyStore II                                                     */
/* MODULE  : XSR BML                                                         */
/* NAME    : BML types definition header                                     */
/* FILE    : BMLTypes.h                                                      */
/* PURPOSE : This header defines Data types which are shared                 */
/*           by all BML submodules                                           */
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
/*   29-JUL-2003 [SongHo Yoon]: First writing                                */
/*   07-SEP-2004 [Janghwan Kim]: added (*EraseVerify), (*MErase) function    */
/*                               pointer to FlVolCxt structure               */
/*   07-SEP-2004 [Janghwan Kim]: changed type of nBWidth of FlVolCxt         */
/*                               (UINT32->UINT8)                             */
/*   07-SEP-2004 [Janghwan Kim]: added nMEFlag variable into FlVolCxt        */
/*   16-NOV-2004 [Janghwan Kim]: added PRELOG_MERASE                         */
/*   20-OCT-2005 [Younwon Park]: modified BMI                                */
/*                                                                           */
/*****************************************************************************/

#ifndef _BML_TYPES_H_
#define _BML_TYPES_H_

/*****************************************************************************/
/* Constant definition                                                       */
/*****************************************************************************/
#define     DEVS_PER_VOL            (XSR_MAX_DEV / XSR_MAX_VOL)
#define     DEVS_PER_VOL_SHIFT      (2)

/*****************************************************************************/
/* maximum number of blocks in Erase Refresh List							 */
/*****************************************************************************/
#define		MAX_ERL_ITEM			(126)		

/*****************************************************************************/
/* maximum string length of ERL signature									 */
/*****************************************************************************/
#define		MAX_ERL_SIG				(6)

/*****************************************************************************/
/* maximum string length of PCH signature                                    */
/*****************************************************************************/
#define     MAX_PCH_SIG             (8)

/*****************************************************************************/
/* maximum number of Pool Control Block                                      */
/*****************************************************************************/
#define     MAX_PCB                 (2)

/*****************************************************************************/
/* maximum number of Block Map Field per BMP                                 */
/*****************************************************************************/
#define     BMFS_PER_BMS            (127)

/*****************************************************************************/
/* Signature for ERL item													 */
/*****************************************************************************/
#define		ERL_SIG					(UINT8*)"ERLSIG"

#if !defined(XSR_NBL2)
/*****************************************************************************/
/* nOpType of PreOpLog structure                                             */
/*****************************************************************************/
#define     PRELOG_NOP              (UINT16) (0)
#define     PRELOG_WRITE            (UINT16) (1)
#define     PRELOG_ERASE            (UINT16) (2)
#define     PRELOG_MERASE           (UINT16) (4)

/*****************************************************************************/
/* Data structure for storing the info. about the previous operation         */
/*****************************************************************************/
typedef struct {
    UINT16      nOpType;            /* operation type                        */
    UINT16      nNumOfSec;          /* # of sectors                          */
    UINT32      nVol;               /* volume number                         */
    UINT32      nDstVn;             /* destination virtual sector/block number*/
    UINT8      *pMbuf;              /* main array buffer pointer             */
    UINT8      *pSBuf;              /* spare array buffer pointer            */
    UINT32      nFlag;              /* flag of operation                     */
} PreOpLog;
#endif  /* XSR_NBL2 */

/*****************************************************************************/
/* Data structure for storing the info. about ERL (Erase Refresh List)		 */
/*****************************************************************************/
typedef struct {
	UINT8		nCnt;				/* count of blocks to be refreshed		 */
	UINT8		nAge;				/* current age of ERL					 */
	UINT32		aSbn[MAX_ERL_ITEM];	/* Sbn of blocks to be refreshed		 */
} ERL;

/*****************************************************************************/
/* Data structure for storing the info. about BMF(Block Map Field)           */
/*****************************************************************************/
typedef struct {
    UINT16     nSbn;            /* Semi physical Block Number                */
    UINT16     nRbn;            /* Replaced Block Number in BMI structure
                                   Replaced Block offset in BMS structure    */
} bmf;

/*****************************************************************************/
/* Data structure for storing the info. about BMI(Block Map Page)            */
/*****************************************************************************/
typedef struct {
    UINT16     nNumOfBMFs;       /* the number of BMFs                         */
    UINT16     nLAge;            /* the age of LBMI                            */
    UINT16     nUAge;            /* the age of UBMI                            */
    bmf       *pstBMF;           /* Block Map Field Array                      */
} bmi;

/*****************************************************************************/
/* Data structure for storing the info. about BMS(Block Map Sector)          */
/*****************************************************************************/
typedef struct {
    UINT16     nInf;            /* nInf field shows a cause why BMS is created*/
    UINT16     nAge;            /* the age of BMS                            */
    bmf        stBMF[BMFS_PER_BMS]; /* Block Map Field Array                 */
} bms;

typedef struct {
    UINT16     n1stBMFIdx;
    UINT16     nNumOfBMFs;
} badunit;

/*****************************************************************************/
/* Data structure for storing the info. about Reservoir                      */
/*****************************************************************************/
typedef struct {
    UINT16     nAllocBlkPtrL;   /* allocated block pointer for Locked Area   */
    UINT16     nAllocBlkPtrU;   /* allocated block pointer for Unlocked Area */

    UINT16     nCurUPCBIdx;     /* current UPCB index                        */
    UINT16     nCurLPCBIdx;     /* current LPCB index                        */

    UINT16     nUPcb[MAX_PCB];  /* Unlocked Pool Control Block number        */
    UINT16     nLPcb[MAX_PCB];  /* Locked Pool Control Block number          */

    UINT16     nUPcbAge;        /* Age of latest UPCB                        */
    UINT16     nLPcbAge;        /* Age of latest LPCB                        */

    UINT16     nNextUBMSOff;    /* next LBMP offset in BMSG of current PCB   */
    UINT16     nNextLBMSOff;    /* next LBMP offset in BMSG of current PCB   */

    /* Sorts by ascending power of stBMI.pstBMF[x]->nSbn */
    bmi        stBMI;           /* Block Map Page                            */

    UINT8     *pBABitMap;       /* Block Allocation Bit Map in Reservoir     */

    badunit   *pBUMap;          /* Bad Unit Map in User Area                 
                                   This field is used 
                                   to decrease a loop-up time of BMF         */
	ERL		   stERL;			/* Erase Refresh List						 */                                   
} reservoir;

/*****************************************************************************/
/* typedefs for Pool Control Header                                          */
/* The size of PoolCtlHdr should be 512 Bytes.                               */
/*****************************************************************************/
typedef struct {    
    UINT8        aSig[MAX_PCH_SIG];                /* "LOCKPCHD" / "ULOCKPCH"*/
    UINT16       nAge;                             /* Age of PCH             */
    UINT16       nAltPcb;                          /* Pbn of alternative PCB */
    UINT32       nEraseSig1;                       /* 1st Erase Signature    */
    UINT32       nEraseSig2;                       /* 2nd Erase Signature    */
    UINT8        aPad[XSR_SECTOR_SIZE - 4 * 3 - MAX_PCH_SIG];
                                                   /* reserved               */
} pch;

/*****************************************************************************/
/* typedefs for Device Context                                               */
/*****************************************************************************/
typedef struct {
    UINT16      nDevNo;             /*  physical device number               */
    UINT16      n1stSbnOfULArea;    /*  1stSbn of Unlocked Area              */
    UINT16      n1stPbnOfULArea;    /*  1stPbn of Unlocked Area              */
    UINT16      nBlksInULArea;      /*  number of blocks in Unlocked Area    */

#if !defined(XSR_NBL2)
    PreOpLog    stPreOpContent;     /*  previous operation content           */
    UINT8      *pPreMBuf;           /*  main buffer poiner of previous
                                        write operation                      */
    UINT8      *pPreSBuf;           /*  spare buffer pointer of previous     
                                        write operation                      */
#endif  /* XSR_NBL2 */

    reservoir   stRsv;              /*  reservoir structure                  */

    UINT8      *pTmpMBuf;           /* temporary buffer                      */
    UINT8      *pTmpSBuf;           /* temporary buffer                      */
} flash_dev;

/*****************************************************************************/
/* Data structure for storng the info. about volume information              */
/*****************************************************************************/
typedef struct {
    BOOL32      bVolOpen;           /* volume open flag                      */
    SM32        nSm;                /*  handle # for semephore               */
    UINT32      nOpenCnt;           /* volume open count                     */
    UINT32      nNumOfDev;          /* number of devices in volume           */
    UINT32      nNumOfUsBlks;       /* number of usable blocks in volume     */
    BOOL32      bPreProgram;        /* pre-programming en-dis/able           */

    UINT16      nBlksPerDev;        /*  blocks per device                    */
    UINT16      nRsvrPerDev;        /*  # of blocks in Reservoir per device  */

    UINT16      nSctsPerBlk;        /*  sectors per block                    */
    UINT16      nPgsPerBlk;         /*  pages per block                      */

    UINT16      n1stSbnOfRsvr;      /*  1stSbn of  Reservoir                 */
    UINT16		nERLBlkSbn;			/*  Sbn of ERL block					 */
    UINT16		nRefBlkSbn;			/*	Sbn of refresh block				 */
    UINT8       nLsnPos;            /*  offset of Lsn in spara area          */
    UINT8       nBadPos;            /*  offset of bad position in spare area */

    UINT8       nEccPos;            /*  ECC position                         */
    UINT8       nSctsPerPg;         /*  sectors per page                     */
    UINT8       nMskSctsPerPg;      /*  mod value for sectors per page       */
    UINT8       nSftSctsPerPg;      /*  mod value for sectors per page       */

    UINT8       nPlanesPerDev;      /*  plane per device                     */
    UINT8       nSftSctsPerBlk;     /*  shift value for sectors per block    */
    UINT8       nSftnNumOfDev;      /*  shift value for number of devices    
                                        in volume                            */
    UINT8       nReserved;
    BOOL32      bBAlign;            /*  byte align                           */

    UINT16      nEccPol;            /*  ECC policy NO_ECC or SW_ECC or HW_ECC*/
    UINT8       nBWidth;            /*  BusWidth using as ECC function param */
    UINT8       nMEFlag;            /*  Multiple Erase Flag                  */
                                    /*   -LLD_ME_OK : Device has MErase Func */
                                    /*   -LLD_ME_NO : Device does not have   */
                                    /*                MErase Func            */
          
    INT32     (*lld_init)		(VOID *pParm);
    INT32     (*lld_open)		(UINT32 nDev);
    INT32     (*lld_close)		(UINT32 nDev);   
    INT32     (*lld_read)		(UINT32 nDev,  UINT32 nPsn,    UINT32 nScts, 
								UINT8 *pMBuf, UINT8 *pSBuf,   UINT32 nFlag);
    INT32     (*lld_mread)		(UINT32 nDev,  UINT32 nPsn,    UINT32 nScts, 
								SGL *pstSGL,  UINT8 *pSBuf,   UINT32 nFlag);
    INT32     (*lld_dev_info)	(UINT32 nDev,  LLDSpec *pstDevInfo);
    INT32     (*lld_check_bad)	(UINT32 nDev,  UINT32 Pbn);
    INT32     (*lld_flush)		(UINT32 nDev); 
    INT32     (*lld_set_rw)		(UINT32 nDev,  UINT32 nSUbn,   UINT32 nUBlks);
    INT32     (*lld_ioctl)		(UINT32 nDev,  UINT32 nCmd,
                                UINT8 *pBufI, UINT32 nLenI,
                                UINT8 *pBufO, UINT32 nLenO,
                                UINT32 *pByteRet);
    VOID      (*lld_platform_info)(LLDPlatformInfo*   pstLLDPlatformInfo);
    
    part_info   *pstPartI;           /* partition information                 */
    part_exinfo *pPIExt;             /* partition information extension       */

} flash_vol;

#endif /* _BML_TYPES_H_ */
