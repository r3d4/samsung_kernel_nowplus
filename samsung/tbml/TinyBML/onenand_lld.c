/*****************************************************************************/
/*                                                                           */
/* PROJECT : ANYSTORE II                                                     */
/* MODULE  : LLD                                                             */
/* NAME    : OneNAND Low-level Driver                                        */
/* FILE    : ONLD.cpp                                                        */
/* PURPOSE : OneNAND LLD Implementation                                      */
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
/* - 26/MAY/2003 [Janghwan Kim] : First writing                              */
/* - 15/JUN/2003 [Janghwan Kim] : Code modification                          */
/* - 06/OCT/2003 [Janghwan Kim] : Code reorganization                        */
/* - 27/OCT/2003 [Janghwan Kim] : Add walkaround code for OndNand device bug */
/* - 07/NOV/2003 [Janghwan Kim] : Add ONLD_ERR_PRINT                         */
/* - 11/DEC/2003 [Janghwan Kim] : Add ONLD_IOCtrl() function                 */
/* - 15/DEC/2003 [Janghwan Kim] : CACHED_READ implementation                 */
/* - 20/DEC/2003 [Janghwan Kim] : DEFERRED_CHK operation implementation      */
/* - 07/JAN/2004 [Janghwan Kim] : Add _ChkPrevOpResult()                     */
/* - 15/JAN/2004 [Janghwan Kim] : Code Modification according to the Code    */
/*                                Inspection Result                          */
/* - 19/FEB/2004 [Janghwan Kim] : use static array for FIL context in place  */
/*                                of using the pointer and the OAM_Malloc    */
/* - 16/APR/2004 [Janghwan Kim] : Change Cached Read op flag policy          */
/*                                (Always ECC_ON -> follwing previous Read   */
/*                                 operation flag)                           */
/*   11-FEB-2004 [SongHo Yoon]  : added XSR_BIG_ENDIAN definition to support */
/*                                big-endian CPU                             */
/* - 19/FEB/2004 [Janghwan Kim] : use static array for FIL context in place  */
/*                                of using the pointer and the OAM_Malloc    */
/* - 16/APR/2004 [Janghwan Kim] : Change Cached Read op flag policy          */
/*                                (Always ECC_ON -> follwing previous Read   */
/*                                 operation flag)                           */
/* - 19/AUG/2004 [Younwon Park] : Reorganization for 1G                      */
/*                                (Lock Scheme change)                       */
/* - 28/OCT/2004 [Younwon Park] : Add onld_eraseverify, onld_merase functions*/
/* - 17/JAN/2005 [Younwon Park] : Code Modification for XSR1.3.1             */
/* - 25/MAR/2005 [Younwon Park] : LLD code merge                             */
/*                                512M M/A-die, 1G DDP, 1G M-die, 2G DDP     */
/*                                256M M/A-die, 128M M-die                   */
/* - 09/JUN/2005 [Younwon Park] : Solve the mismatch between                 */
/*                                OAM_Malloc() and OAM_Free()                */
/* - 22/JUN/2005 [Younwon Park] : Modify _ChkPrevOpResult()                  */
/* - 30/JUN/2005 [SeWook Na]    : Change function redirection of MEMCPY()    */
/*                                OAM_Memcpy() --> PAM_Memcpy()              */
/* - 22/JUL/2005 [YounWon Park] : Modify onld_mwrite()                       */
/*                                *pErrPsn = pstPrevOpInfo[nDev]->nPsn;      */
/* - 05/SEP/2005 [SeWook NA]    : Modify onld_eraseverify to use only after  */
/*                                MErase                                     */
/* - 22/SEP/2005 [Younwon Park] : Modify ONLD_Copyback to handle read error  */
/* - 27/SEP/2005 [Younwon Park] : Modify onld_mread/onld_mwrite to use SGL   */
/* - 11/JAN/2006 [WooYoung Yang] : Remove CACHED_READ                        */
/*                                 Modify onld_merase()                      */
/* - 18/JAN/2006 [WooYoung Yang] : Remove SYNC_BURST_READ Setting Code in    */
/*                                 onld_open                                 */
/* - 20/JAN/2006 [WooYoung Yang] : Remove All SYNC_BURST_READ define phase   */
/*                                 Add SYNC_MODE Setting Status Check Code   */
/*                                 in _InitDevInfo                           */
/* - 31/JAN/2006 [ByoungYoung Ahn] : added LLD_READ_DISTURBANCE, 			 */
/*									_ChkReadDisturb()						 */
/* - 31/JAN/2006 [ByoungYoung Ahn] : added AD_Idle()						 */
/* - 05/APR/2006 [WooYoung Yang] : Remove _SyncBurstBRead()                  */
/* - 22/AUG/2006 [WooYoung Yang] : Modified interrupt signal order           */
/*                                 for STL_ASyncMode                         */
/* - 19/OCT/2006 [WooYoung Yang] : In case of DDP chip, operand of chip      */
/*                                 selection is modified in onld_merase      */
/*                                 Added device information in ONLDSpec      */
/* - 30/NOV/2006 [Younwon Park] : fixed the defect in _mread()               */
/*                               (In case of Snapshot 4 in 1KB page depth)   */
/* - 11/JAN/2007 [WooYoung Yang] : removed error code of MErase due to       */
/*                                 scheme of MErase error verification       */
/* - 01/FEB/2007 [Younwon Park] : Modified erase refresh scheme              */
/*                                in onld_read() and onld_mread()            */
/* - 14/FEB/2007 [Yulwon Cho]: _cacheread() is implemented, and              */
/*                              _InitDevInfo() is modified.                   */
/* - 02/MAY/2007 [Yulwon Cho]: _cacheahead() is added for read-ahead feature */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/* Header file inclusion                                                     */
/*****************************************************************************/
#include <tbml_common.h>
#include <os_adapt.h>
#include <onenand_interface.h>
#include <onenand_lld.h>
#if defined(CONFIG_RFS_TINYBML)
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 19)
#include <linux/config.h>
#endif
#endif /* CONFIG_RFS_TINYBML */

/*****************************************************************************/
/* ONLD FN90 dedicated header file inclusion                                 */
/*****************************************************************************/
#include "onenand.h"
#include "onenand_reg.h"

/*****************************************************************************/
/* Local Configurations                                                      */
/*****************************************************************************/
#undef STRICT_CHK

#ifdef  ASYNC_MODE
#define DEFERRED_CHK    /* Should be set defined */
#else /* SYNC_MODE */
#define  DEFERRED_CHK   /* User Can Modify it */
#endif

#define ALIGNED_MEMCPY  /* User Can Modify it */
#undef  XSR_BIG_ENDIAN  /* User Can Modify it */
#undef	LOCK_TIGHT		/* for erase refresh, lock-tight should be disabled */
#define  USE_CACHE_READ  /* User Can Modify it */
#ifdef  USE_CACHE_READ
#define  USE_CACHE_AHEAD /* User Can Modify it */
#endif

/*****************************************************************************/
/* Debug Configuration                                                       */
/*****************************************************************************/
//#define _ONLD_DEBUG
#define _ONLD_ERR_TRACE

#define ONLD_RTL_PRINT(x)           LLD_RTL_PRINT(x)    

#if defined(_ONLD_ERR_TRACE)
#define ONLD_ERR_PRINT(x)           LLD_RTL_PRINT(x)
#else
#define ONLD_ERR_PRINT(x)
#endif

#ifdef  _ONLD_DEBUG
#define ONLD_DBG_PRINT(x)           LLD_DBG_PRINT(x)
#else
#define ONLD_DBG_PRINT(x)    
#endif

/*****************************************************************************/
/* Function Redirection                                                      */
/*****************************************************************************/
#define MALLOC(x)                   AD_Malloc(x)
#define FREE(x)                     AD_Free(x)
#define MEMSET(x,y,z)               AD_Memset(x,y,z)
#define MEMCPY(x,y,z)               AD_Memcpy(x,y,z)

/*****************************************************************************/
/* Local Constant Definiations                                               */
/*****************************************************************************/
#define MAX_SUPPORT_DEVS            8
#define NUM_OF_DEVICES              1

#define ONLD_READ_UERROR_A          0xAAAA
#define ONLD_READ_CERROR_A          0x5555

#define ONLD_MAIN_SIZE              512
#define ONLD_SPARE_SIZE             16

#define BUFF_MAIN_SIZE              128

#define FIRSTCHIP                   0
#define SECONDCHIP                  512

#define CHIP_128M					0x0000
#define CHIP_256M				    0x0010
#define CHIP_512M					0x0020
#define CHIP_1G                     0x0030
#define CHIP_2G				        0x0040
#define CHIP_4G                     0x0050
#define CHIP_8G                     0x0060

#define DDP_3G                      3072

#define M_DIE						0x0000
#define A_DIE						0x0100
#define B_DIE                       0x0200

#define DDP_CHIP					0x0008

#define MASK_DEVICEID_DENSITY           0x00F0
#define MASK_VOLTAGE                    0x0003
#define MASK_VIRSIONID_GEN              0x0F00

/* OneNand Block Lock Scheme or Range Lock Scheme or none */
#define BLK_LK						0
#define RNG_LK						1
#define NON_LK                      2

/*****************************************************************************/
/* ONLD Return Code                                                          */
/*****************************************************************************/
/* Major Return Value */
#define ONLD_READ_DISTURBANCE		LLD_READ_DISTURBANCE
#define ONLD_IOC_NOT_SUPPORT        LLD_IOC_NOT_SUPPORT
#define ONLD_DEV_NOT_OPENED         LLD_DEV_NOT_OPENED   
#define ONLD_DEV_POWER_OFF          LLD_DEV_POWER_OFF    
#define ONLD_DEV_NO_LLD_FUNC        LLD_DEV_NO_LLD_FUNC  
#define ONLD_INIT_BADBLOCK          LLD_INIT_BADBLOCK    
#define ONLD_INIT_GOODBLOCK         LLD_INIT_GOODBLOCK   
#define ONLD_SUCCESS                LLD_SUCCESS          
#define ONLD_ERASE_ERROR            LLD_ERASE_ERROR  
#define ONLD_MERASE_ERROR           LLD_MERASE_ERROR    
#define ONLD_PREV_ERASE_ERROR       LLD_PREV_ERASE_ERROR 
#define ONLD_WRITE_ERROR            LLD_WRITE_ERROR      
#define ONLD_PREV_WRITE_ERROR       LLD_PREV_WRITE_ERROR 
#define ONLD_READ_ERROR             LLD_READ_ERROR       
#define ONLD_CRITICAL_ERROR         LLD_CRITICAL_ERROR   
#define ONLD_WR_PROTECT_ERROR       LLD_WR_PROTECT_ERROR 
#define ONLD_ILLEGAL_ACCESS         LLD_ILLEGAL_ACCESS   
#define ONLD_SPARE_WRITE_ERROR      LLD_SPARE_WRITE_ERROR
                                                        
#define ONLD_INIT_FAILURE           LLD_INIT_FAILURE     
#define ONLD_OPEN_FAILURE           LLD_OPEN_FAILURE     
#define ONLD_CLOSE_FAILURE          LLD_CLOSE_FAILURE    

/* Minor Retun Value */
/* Previous Operation Flag */
/* LLD_PREV_OP_RESULT is combined with Major Return Value, and means 
   Previous Operation Error except LLD_READ_ERROR */
#define ONLD_PREV_OP_RESULT         LLD_PREV_OP_RESULT


/*****************************************************************************/
/* ONLD Operation Flag Code                                                  */
/*****************************************************************************/
#define ONLD_FLAG_ASYNC_OP          LLD_FLAG_ASYNC_OP    /* Write/Erase/Copy */
#define ONLD_FLAG_SYNC_OP           LLD_FLAG_SYNC_OP     /* Write/Erase/Copy */
#define ONLD_FLAG_SYNC_MASK         (1 << 0)             /* Write/Erase/Copy  */  
#define ONLD_FLAG_ECC_ON            LLD_FLAG_ECC_ON
#define ONLD_FLAG_ECC_OFF           LLD_FLAG_ECC_OFF

#define X08                         LLD_BW_X08
#define X16                         LLD_BW_X16

/* Multiple Erase support or not */
#define ME_OK                       LLD_ME_OK
#define ME_NO                       LLD_ME_NO


/*****************************************************************************/
/* Local Data Structure                                                      */
/*****************************************************************************/
typedef struct
{
    UINT16      nMID;               /* Manufacturer ID                       */
    UINT16      nDID;               /* Device ID                             */
    UINT16      nGEN;               /* Version ID                            */

    UINT16      nNumOfBlks;         /* Number of Blocks                      */
    UINT16      nNumOfPlanes;       /* Number of Planes                      */
    UINT16      nBlksInRsv;         /* The Number of Blocks                  */
                                    /* in Reservior for Bad Blocks           */
    UINT8       nBadPos;            /* BadBlock Information Poisition        */
    UINT8       nLsnPos;            /* LSN Position                          */
    UINT8       nEccPos;            /* ECC Policy : HW_ECC, SW_ECC           */
    UINT8       nBWidth;            /* Device Band Width                     */
    
    UINT8       nMEFlag;            /* Multiple Erase Flag                   */
    UINT8       nLKFlag;            /* Lock scheme Flag                      */
    
    UINT32      nTrTime;            /* Typical Read Op Time                  */
    UINT32      nTwTime;            /* Typical Write Op Time                 */
    UINT32      nTeTime;            /* Typical Erase Op Time                 */
    UINT32      nTfTime;            /* Typical Transfer Op Time              */

} ONLDSpec;    

typedef struct
{
    UINT32      BaseAddr;           /* Device Mapped Base Address            */
    BOOL32      bOpen;              /* Device Open Flag                      */
    BOOL32      bPower;             /* Device Power Flag                     */
    ONLDSpec*   pstDevSpec;         /* Device Information(Spec) pointer      */
} ONLDDev;

typedef struct
{
    UINT8	nBufSelSft;
    UINT8	nSctSelSft;
    UINT8	nBlkSelSft;
    UINT8	nPgSelSft;
    UINT8	nDDPSelSft;
    UINT8	nFPASelSft;

    UINT8	nFSAMask;
    UINT8	nSctsPerPG;

    INT32  (*SetRWArea)  (UINT32 nDev, UINT32 nSUbn, UINT32 nUBlks);
    INT32  (*LockTight)  (UINT32  nDev, UINT8  *pBufI, UINT32 nLenI);
    INT32   (*MRead)      (UINT32 nDev, UINT32 nPsn, UINT32 nScts, 
             SGL *pstSGL, UINT8 *pSBuf, UINT32 nFlag);
}ONLDInfo;

#if defined(DEFERRED_CHK)
typedef enum {
    NONE    = 0,
    READ    = 1,
    WRITE   = 2, 
    ERASE   = 3,
    CPBACK  = 4,
    MERASE  = 5,
}OpType;
#endif /* #if defined(DEFERRED_CHK) */

typedef struct 
{
#if defined(DEFERRED_CHK)
    OpType ePreOp;
    UINT32 nPsn;
    UINT32 nScts;
    UINT32 nFlag;
    UINT16 nCmd;
    LLDMEArg* pstPreMEArg;
#endif /* #if defined(DEFERRED_CHK)*/
    UINT16 nBufSel;
}PrevOpInfo;

#if defined(USE_CACHE_READ) && defined(USE_CACHE_AHEAD)
typedef enum {
    CACHE_HIT0,
    CACHE_HIT1,
    CACHE_HIT2, 
    CACHE_MISS,
    CACHE_START,
}CacheCase;
#endif /* #if defined(USE_CACHE_READ) && defined(USE_CACHE_AHEAD) */

#ifdef CONFIG_RFS_TINYBML
/* shared previous operation result */
extern UINT16   result_ctrl_status[MAX_SUPPORT_DEVS];
extern UINT16   tiny_shared;
extern UINT16	xsr_shared;
extern UINT16	tiny_block_shared;
extern UINT16	xsr_block_shared;
#endif	/* CONFIG_RFS_TINYBML */
/*****************************************************************************/
/* Local Variable                                                            */
/*****************************************************************************/
static UINT16	     ONLD_CMD_LOCK_NAND;
static UINT16	     ONLD_CMD_LOCK_TIGHT;	
static ONLDInfo      astONLDInfo[MAX_SUPPORT_DEVS];
static PrevOpInfo   *pstPrevOpInfo[MAX_SUPPORT_DEVS];
static ONLDDev       astONLDDev[MAX_SUPPORT_DEVS];
static UINT8         aEmptySBuf[ONLD_SPARE_SIZE * 4];

static ONLDSpec      astNandSpec[] = {
   /*****************************************************************************/
   /*nMID, nDID, nGEN, nNumOfBlks,                                              */
   /*                      nNumOfPlanes                                         */
   /*                          nBlocksInRsv                                     */
   /*                              nBadPos                                      */
   /*                                 nLsnPos                                   */
   /*                                    nEccPos                                */
   /*                                       nBWidth                             */
   /*                                             nMEFlag                       */
   /*                                                   nLKFlag                 */
   /*                                                       nTrTime;(ns)        */
   /*                                                           nTwTime;(ns)    */
   /*                                                               nTeTime;(ns)*/
   /*                                                                   nTfTime */
   /*****************************************************************************/
    /* 1G DEMUX */
    {0xEC, 0x34, 0x00, 1024, 1, 20, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},
    {0xEC, 0x35, 0x00, 1024, 1, 20, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},

    /* 512M DEMUX */    
    {0xEC, 0x24, 0x00,  512, 1, 10, 0, 2, 8, X16, ME_NO, RNG_LK, 50000, 350000, 2000000, 50},
    {0xEC, 0x25, 0x00,  512, 1, 10, 0, 2, 8, X16, ME_NO, RNG_LK, 50000, 350000, 2000000, 50},   
    {0xEC, 0x24, 0x01,  512, 1, 10, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},
    {0xEC, 0x25, 0x01,  512, 1, 10, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},

    /* 1G DDP DEMUX */    
    {0xEC, 0x3c, 0x00, 1024, 2, 20, 0, 2, 8, X16, ME_NO, RNG_LK, 50000, 350000, 2000000, 50},
    {0xEC, 0x3d, 0x00, 1024, 2, 20, 0, 2, 8, X16, ME_NO, RNG_LK, 50000, 350000, 2000000, 50},  

    /* 2G DDP DEMUX */      
    {0xEC, 0x4c, 0x00, 2048, 2, 40, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},
    {0xEC, 0x4d, 0x00, 2048, 2, 40, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},

    /* 2G DDP DEMUX based on 1Gb A-die */      
    {0xEC, 0x4c, 0x01, 2048, 2, 40, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},
    {0xEC, 0x4d, 0x01, 2048, 2, 40, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},

    /* 256M DEMUX */    
    {0xEC, 0x14, 0x00,  512, 1, 10, 0, 2, 8, X16, ME_OK, RNG_LK, 50000, 350000, 2000000, 50},
    {0xEC, 0x15, 0x00,  512, 1, 10, 0, 2, 8, X16, ME_OK, RNG_LK, 50000, 350000, 2000000, 50},  
    {0xEC, 0x14, 0x01,  512, 1, 10, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},
    {0xEC, 0x15, 0x01,  512, 1, 10, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},

    /* 128M DEMUX */    
    {0xEC, 0x04, 0x00,  256, 1,  5, 0, 2, 8, X16, ME_OK, RNG_LK, 50000, 350000, 2000000, 50},
    {0xEC, 0x05, 0x00,  256, 1,  5, 0, 2, 8, X16, ME_OK, RNG_LK, 50000, 350000, 2000000, 50},

    /* 1G MUX */    
    {0xEC, 0x30, 0x00, 1024, 1, 20, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},
    {0xEC, 0x31, 0x00, 1024, 1, 20, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},

    /* 512M MUX */    
    {0xEC, 0x20, 0x00,  512, 1, 10, 0, 2, 8, X16, ME_NO, RNG_LK, 50000, 350000, 2000000, 50},
    {0xEC, 0x21, 0x00,  512, 1, 10, 0, 2, 8, X16, ME_NO, RNG_LK, 50000, 350000, 2000000, 50},
    {0xEC, 0x20, 0x01,  512, 1, 10, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},
    {0xEC, 0x21, 0x01,  512, 1, 10, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},

    /* 1G DDP MUX */        
    {0xEC, 0x38, 0x00, 1024, 2, 20, 0, 2, 8, X16, ME_NO, RNG_LK, 50000, 350000, 2000000, 50},
    {0xEC, 0x39, 0x00, 1024, 2, 20, 0, 2, 8, X16, ME_NO, RNG_LK, 50000, 350000, 2000000, 50}, 

    /* 2G DDP MUX */       
    {0xEC, 0x48, 0x00, 2048, 2, 40, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},
    {0xEC, 0x49, 0x00, 2048, 2, 40, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},

    /* 2G DDP MUX based on 1Gb A-die */       
    {0xEC, 0x48, 0x01, 2048, 2, 40, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},
    {0xEC, 0x49, 0x01, 2048, 2, 40, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},

    /* 256M MUX */    
    {0xEC, 0x10, 0x00,  512, 1, 10, 0, 2, 8, X16, ME_OK, RNG_LK, 50000, 350000, 2000000, 50},
    {0xEC, 0x11, 0x00,  512, 1, 10, 0, 2, 8, X16, ME_OK, RNG_LK, 50000, 350000, 2000000, 50},
    {0xEC, 0x10, 0x01,  512, 1, 10, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},
    {0xEC, 0x11, 0x01,  512, 1, 10, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},
    
    /* 128M MUX */    
    {0xEC, 0x00, 0x00,  256, 1,  5, 0, 2, 8, X16, ME_OK, RNG_LK, 50000, 350000, 2000000, 50},
    {0xEC, 0x01, 0x00,  256, 1,  5, 0, 2, 8, X16, ME_OK, RNG_LK, 50000, 350000, 2000000, 50},

    /* 2G DEMUX M-die */
    {0xEC, 0x44, 0x00, 2048, 1, 40, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},
    {0xEC, 0x45, 0x00, 2048, 1, 40, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},

    /* 2G DEMUX A-die */
    {0xEC, 0x44, 0x01, 2048, 1, 40, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},
    {0xEC, 0x45, 0x01, 2048, 1, 40, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},
    
    /* 4G DDP DEMUX  */
    {0xEC, 0x5c, 0x00, 4096, 2, 80, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},
    {0xEC, 0x5d, 0x00, 4096, 2, 80, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},
    
    /* 4G DDP DEMUX based on 2G A-die */
    {0xEC, 0x5c, 0x01, 4096, 2, 80, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},
    {0xEC, 0x5d, 0x01, 4096, 2, 80, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},

    /* 1G DEMUX A-die */
    {0xEC, 0x34, 0x01, 1024, 1, 20, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},
    {0xEC, 0x35, 0x01, 1024, 1, 20, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},
 
     /* 512M DEMUX B-die */    
    {0xEC, 0x24, 0x02,  512, 1, 10, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},
    {0xEC, 0x25, 0x02,  512, 1, 10, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},   
       
    /* 2G MUX M-die */
    {0xEC, 0x40, 0x00, 2048, 1, 40, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},
    {0xEC, 0x41, 0x00, 2048, 1, 40, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},

    /* 2G MUX A-die */
    {0xEC, 0x40, 0x01, 2048, 1, 40, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},
    {0xEC, 0x41, 0x01, 2048, 1, 40, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},

    /* 4G DDP MUX */
    {0xEC, 0x58, 0x00, 4096, 2, 80, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},
    {0xEC, 0x59, 0x00, 4096, 2, 80, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},
    
    /* 4G DDP MUX based on 2G A-die*/
    {0xEC, 0x58, 0x01, 4096, 2, 80, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},
    {0xEC, 0x59, 0x01, 4096, 2, 80, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},

    /* 1G MUX A-die */
    {0xEC, 0x30, 0x01, 1024, 1, 20, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},
    {0xEC, 0x31, 0x01, 1024, 1, 20, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},

     /* 512M MUX B-die */    
    {0xEC, 0x20, 0x02,  512, 1, 10, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},
    {0xEC, 0x21, 0x02,  512, 1, 10, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},   
    
    /* 1G DEMUX B-die */
    {0xEC, 0x34, 0x02, 1024, 1, 20, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},
    {0xEC, 0x35, 0x02, 1024, 1, 20, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},
    
    /* 1G MUX B-die */
    {0xEC, 0x30, 0x02, 1024, 1, 20, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},  
    {0xEC, 0x31, 0x02, 1024, 1, 20, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},                               

    /* 2G DDP DEMUX B-die */      
    {0xEC, 0x4c, 0x02, 2048, 2, 40, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},
    {0xEC, 0x4d, 0x02, 2048, 2, 40, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},
    
    /* 2G DDP MUX B-die */      
    {0xEC, 0x48, 0x02, 2048, 2, 40, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},
    {0xEC, 0x49, 0x02, 2048, 2, 40, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},

    /* 1G DEMUX C-die */
    {0xEC, 0x34, 0x03, 1024, 1, 20, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},
    {0xEC, 0x35, 0x03, 1024, 1, 20, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},
    
    /* 1G MUX C-die */
    {0xEC, 0x30, 0x03, 1024, 1, 20, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},  
    {0xEC, 0x31, 0x03, 1024, 1, 20, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50}, 
    
    /* 2G DDP DEMUX C-die */      
    {0xEC, 0x4c, 0x03, 2048, 2, 40, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},
    {0xEC, 0x4d, 0x03, 2048, 2, 40, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},
    
    /* 2G DDP MUX C-die */      
    {0xEC, 0x48, 0x03, 2048, 2, 40, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},
    {0xEC, 0x49, 0x03, 2048, 2, 40, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},

    /* 4G MUX M-die */
    {0xEC, 0x50, 0x00, 4096, 2, 80, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},
    {0xEC, 0x51, 0x00, 4096, 2, 80, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},

    /* 4G DEMUX M-die */
    {0xEC, 0x54, 0x00, 4096, 2, 80, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},
    {0xEC, 0x55, 0x00, 4096, 2, 80, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},

    /* 8G DDP MUX based on 4G M-die */
    {0xEC, 0x68, 0x00, 8192, 2,160, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},
    {0xEC, 0x69, 0x00, 8192, 2,160, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},

    /* 8G DDP DEMUX based on 4G M-die */
    {0xEC, 0x6C, 0x00, 8192, 2,160, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},
    {0xEC, 0x6D, 0x00, 8192, 2,160, 0, 2, 8, X16, ME_OK, BLK_LK, 50000, 350000, 2000000, 50},
                                
    {0x00, 0x00, 0x00,   0,  0,  0, 0, 0, 0,   0,  0,   0, 0,   0,  0,     0}

};


/*****************************************************************************/
/* Local Function Declarations                                               */
/*****************************************************************************/
static VOID     _GetDevID   (UINT32  nDev, UINT16 *nMID,    
                             UINT16 *nDID, UINT16 *nVID);                 
#if defined(STRICT_CHK)
static BOOL32   _StrictChk  (UINT32  nDev, UINT32  nPsn, UINT32 nScts);
#endif  /* #if defined(STRICT_CHK) */

#if defined(USE_CACHE_READ) && defined(USE_CACHE_AHEAD)
INT32
_cacheahead(UINT32 nDev, UINT32 nPsn, UINT32 nScts, SGL *pstSGL, UINT8 *pSBuf, UINT32 nFlag);
#endif /* #if defined(USE_CACHE_READ) && defined(USE_CACHE_AHEAD) */
#if defined(USE_CACHE_READ) && !defined(USE_CACHE_AHEAD)
INT32
_cacheread(UINT32 nDev, UINT32 nPsn, UINT32 nScts, SGL *pstSGL, UINT8 *pSBuf, UINT32 nFlag);
#endif /* #if defined(USE_CACHE_READ) && !defined(USE_CACHE_AHEAD) */
INT32
_mread(UINT32 nDev, UINT32 nPsn, UINT32 nScts, SGL *pstSGL, UINT8 *pSBuf, UINT32 nFlag);
static VOID _DumpRegister(UINT32 nBAddr);

/*****************************************************************************/
/* Local Macros                                                              */
/*****************************************************************************/
#define SET_PWR_FLAG(x)         {astONLDDev[x].bPower = TRUE32;}
#define CLEAR_PWR_FLAG(x)       {astONLDDev[x].bPower = FALSE32;}

/* Get Device Information */
#define GET_DEV_BADDR(n)        (astONLDDev[n].BaseAddr)
#define CHK_DEV_OPEN_FLAG(n)    (astONLDDev[n].bOpen)
#define CHK_DEV_PWR_FLAG(n)     (astONLDDev[n].bPower)
#define GET_DEV_MID(n)          (astONLDDev[n].pstDevSpec->nMID)
#define GET_DEV_DID(n)          (astONLDDev[n].pstDevSpec->nDID)
#define GET_DEV_VID(n)          (astONLDDev[n].pstDevSpec->nVID)
#define GET_NUM_OF_BLKS(n)      (astONLDDev[n].pstDevSpec->nNumOfBlks)
#define GET_PGS_PER_BLK(n)      (64)
#define GET_SCTS_PER_PG(n)       astONLDInfo[n].nSctsPerPG
#define GET_SCTS_PER_BLK(n)     (GET_SCTS_PER_PG(n) * GET_PGS_PER_BLK(n))
#define GET_NUM_OF_DIES(n)      (astONLDDev[n].pstDevSpec->nNumOfPlanes)

/* Get Operation Information */
#define GET_PREV_OP_TYPE(n)     (pstPrevOpInfo[n]->ePreOp)
#define GET_CUR_BUF_SEL(n)      (pstPrevOpInfo[n]->nBufSel)
#define GET_NXT_BUF_SEL(n)      ((pstPrevOpInfo[n]->nBufSel + 1) & 1)

#define GET_ONLD_MBUF_ADDR(x,nSct,nBuf)\
            (ONLD_DT_MB00_ADDR(x) + ((((nSct) & astONLDInfo[nDev].nSctSelSft) | ((nBuf) << astONLDInfo[nDev].nBufSelSft )) << 9))
#define GET_ONLD_SBUF_ADDR(x,nSct,nBuf)\
            (ONLD_DT_SB00_ADDR(x) + ((((nSct) & astONLDInfo[nDev].nSctSelSft) | ((nBuf) << astONLDInfo[nDev].nBufSelSft )) << 4))

#define MAKE_BSA(nSct,nBuf)\
            (((0x08) | ((nSct) & astONLDInfo[nDev].nSctSelSft) | (((nBuf) & 0x01) << 2)) << 8)
#define MAKE_FBA(nPBN)\
            ((nPBN & MASK_FBA) | ((nPBN << astONLDInfo[nDev].nDDPSelSft) & 0x8000))
#define MAKE_FCBA(nPBN) ((nPBN & MASK_FBA))
#define MAKE_DBS(nPBN)\
            ((nPBN << astONLDInfo[nDev].nDDPSelSft) & MASK_DBS)        
#define CHK_BLK_OOB(n,nBlk)     (((nBlk) > GET_NUM_OF_BLKS(n)) ? 1 : 0)
#define CHK_PG_OOB(n,nPg)       (((nPg)  > GET_PGS_PER_BLK(n)) ? 1 : 0)
#define CHK_SCT_OOB(n,nSct,nNum)\
        ((((((nSct) & astONLDInfo[nDev].nSctSelSft) + ((nNum) & astONLDInfo[nDev].nSctSelSft)) > GET_SCTS_PER_PG(n)) || (nNum == 0)) ? 1 : 0)

#define GET_ONLD_INT_STAT(x,a)  ((UINT16)(ONLD_REG_INT(x) & (UINT16)(a)))
#define GET_ONLD_CTRL_STAT(x,a) ((UINT16)(ONLD_REG_CTRL_STAT(x) & (UINT16)(a)))
#define GET_ONLD_SYS1_STAT(x,a) ((UINT16)(ONLD_REG_SYS_CONF1(x) & (UINT16)(a)))
#define CHK_ONLD_2B_ECC_STAT(x) ((ONLD_REG_ECC_STAT(x) & ECC_ANY_2BIT_ERR)  ? 1 : 0)
#define CHK_ONLD_ECC_STAT(x)    ((ONLD_REG_ECC_STAT(x) & ECC_ANY_BIT_ERR)   ? 1 : 0)
#define CHK_ONLD_M_ECC_STAT(x)  ((ONLD_REG_ECC_STAT(x) & ECC_MAIN_BIT_ERR)  ? 1 : 0)
#define CHK_ONLD_S_ECC_STAT(x)  ((ONLD_REG_ECC_STAT(x) & ECC_SPARE_BIT_ERR) ? 1 : 0)

#define GET_PBN(x, psn)         ((psn) >> astONLDInfo[(x)].nBlkSelSft)
#define GET_PPN(x, psn)         ((psn) >> astONLDInfo[(x)].nPgSelSft)

#ifndef __FUNCTION__
    #if (__CC_ARM == 1)
        #define __FUNCTION__ __func__
    #else
        #define __FUNCTION__ "[]"
    #endif
#endif

#define ONLD_BUSYWAIT_TIME_OUT          25000000
/*****************************************************************************/
/* Code Implementation                                                       */
/*****************************************************************************/

#define WAIT_ONLD_INT_STAT_CHECKING_PROTECTION(nBAddr, nVal)                                                \
        { INT32 nWaitingTimeOut = ONLD_BUSYWAIT_TIME_OUT;                                                   \
        while (GET_ONLD_INT_STAT(nBAddr, nVal) != ((UINT16)nVal))                                           \
        {                                                                                                   \
            if (--nWaitingTimeOut == 0)                                                                     \
            {                                                                                               \
                ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  busy wait time-out %s,%d,%s\r\n"),                      \
               (const unsigned char *) __FILE__, __LINE__, __FUNCTION__));                                  \
                _DumpRegister(nBAddr);                                                                      \
                return (ONLD_ILLEGAL_ACCESS);                                                               \
            }                                                                                               \
        }                                                                                                   \
        if (GET_ONLD_CTRL_STAT(nBAddr, (UINT16)LOCK_STATE) == (UINT16)(LOCK_STATE))                         \
        {                                                                                                   \
            ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  Write Protection Error %s,%d,%s\r\n"),                      \
            (const unsigned char *) __FILE__, __LINE__, __FUNCTION__));                                     \
            _DumpRegister(nBAddr);                                                                          \
            return (ONLD_WR_PROTECT_ERROR);                                                                 \
        }                                                                                                   \
        }

#define WAIT_ONLD_INT_STAT(nBAddr, nVal)                                                                    \
        { INT32 nWaitingTimeOut = ONLD_BUSYWAIT_TIME_OUT;                                                   \
        while (GET_ONLD_INT_STAT(nBAddr, nVal) != ((UINT16)nVal))                                           \
        {                                                                                                   \
            if (--nWaitingTimeOut == 0)                                                                     \
            {                                                                                               \
                ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  busy wait time-out %s,%d,%s\r\n"),                      \
                (const unsigned char *) __FILE__, __LINE__, __FUNCTION__));                                 \
                _DumpRegister(nBAddr);                                                                      \
                return (ONLD_ILLEGAL_ACCESS);                                                               \
            }                                                                                               \
        }                                                                                                   \
        }

#define WAIT_ONLD_WR_PROTECT_STAT(nBAddr, nVal)                                                             \
        { INT32 nWaitingTimeOut = ONLD_BUSYWAIT_TIME_OUT;                                                   \
        while (((UINT16)ONLD_REG_WR_PROTECT_STAT(nBAddr) & (UINT16)nVal) != ((UINT16)nVal))                 \
        {                                                                                                   \
            if (--nWaitingTimeOut == 0)                                                                     \
            {                                                                                               \
                ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  Write Protection Error\r\n")));                         \
                ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  busy wait time-out %s,%d,%s\r\n"),                      \
                (const unsigned char *) __FILE__, __LINE__, __FUNCTION__));                                 \
                _DumpRegister(nBAddr);                                                                      \
                return (ONLD_ILLEGAL_ACCESS);                                                               \
            }                                                                                               \
        }                                                                                                   \
        }

#define WAIT_ONLD_CTRL_STAT(nBAddr, nVal)                                                                       \
        { INT32 nWaitingTimeOut = ONLD_BUSYWAIT_TIME_OUT;                                                   \
        while (GET_ONLD_CTRL_STAT(nBAddr, nVal) != ((UINT16)nVal))                       \
        {                                                                                                   \
            if (--nWaitingTimeOut == 0)                                                                     \
            {                                                                                               \
                ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  Control Register Status Error\r\n")));                  \
                ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  busy wait time-out %s,%d,%s\r\n"),                      \
                (const unsigned char *) __FILE__, __LINE__, __FUNCTION__));                                 \
                _DumpRegister(nBAddr);                                                                      \
                return (ONLD_ILLEGAL_ACCESS);                                                               \
            }                                                                                               \
        }                                                                                                   \
        }

/* Delay Function */
#ifdef ADD_DELAY_FOR_2CYCLE_CMD

#define DUMMY_READ_CNT  2
static void
ADD_DELAY(UINT32 nBAddr, INT32 nCnt)
{
    INT32 nRet = ONLD_SUCCESS;
    
    while (nCnt-- > 0)
    {
    	nRet = GET_ONLD_CTRL_STAT(nBAddr, ERROR_STATE);
    	
    	if (nRet == ERROR_STATE)
    	{
        	ONLD_ERR_PRINT((TEXT("[ONLD : ERR] Error\r\n")));
        	return;
    	}
    }
}

#else   /* #ifdef ADD_DELAY_FOR_2CYCLE_CMD */

#define DUMMY_READ_CNT  0
#define ADD_DELAY(nBAddr, nCnt)

#endif /* ifdef ADD_DELAY_FOR_2CYCLE_CMD */

/*****************************************************************************/
/* Memory Copy Macros                                                        */
/*****************************************************************************/
#define MEMCPY16(x,y,z)                                                                                     \
{                                                                                                           \
    UINT16* pSrc = y;                                                                                       \
    UINT16* pDst = x;                                                                                       \
    UINT32  nLen = (z) / sizeof(UINT16);                                                                    \
    while(nLen--)                                                                                           \
    {                                                                                                       \
        *pDst++ = *pSrc++;                                                                                  \
    }                                                                                                       \
}

static VOID
_DumpRegister(UINT32 nBAddr)
{
    ONLD_ERR_PRINT((TEXT("[ONLD :  REG] ONLD_REG_CTRL_STAT = 0x%04x\r\n"), ONLD_REG_CTRL_STAT(nBAddr)));
    ONLD_ERR_PRINT((TEXT("[ONLD :  REG] ONLD_REG_START_ADDR1 = 0x%04x\r\n"), ONLD_REG_START_ADDR1(nBAddr)));
    ONLD_ERR_PRINT((TEXT("[ONLD :  REG] ONLD_REG_START_ADDR2 = 0x%04x\r\n"), ONLD_REG_START_ADDR2(nBAddr)));
    ONLD_ERR_PRINT((TEXT("[ONLD :  REG] ONLD_REG_START_ADDR3 = 0x%04x\r\n"), ONLD_REG_START_ADDR3(nBAddr)));
    ONLD_ERR_PRINT((TEXT("[ONLD :  REG] ONLD_REG_START_ADDR4 = 0x%04x\r\n"), ONLD_REG_START_ADDR4(nBAddr)));
    ONLD_ERR_PRINT((TEXT("[ONLD :  REG] ONLD_REG_START_ADDR5 = 0x%04x\r\n"), ONLD_REG_START_ADDR5(nBAddr)));
    ONLD_ERR_PRINT((TEXT("[ONLD :  REG] ONLD_REG_START_ADDR6 = 0x%04x\r\n"), ONLD_REG_START_ADDR6(nBAddr)));
    ONLD_ERR_PRINT((TEXT("[ONLD :  REG] ONLD_REG_START_ADDR7 = 0x%04x\r\n"), ONLD_REG_START_ADDR7(nBAddr)));
    ONLD_ERR_PRINT((TEXT("[ONLD :  REG] ONLD_REG_START_ADDR8 = 0x%04x\r\n"), ONLD_REG_START_ADDR8(nBAddr)));
    ONLD_ERR_PRINT((TEXT("[ONLD :  REG] ONLD_REG_START_BUF = 0x%04x\r\n"), ONLD_REG_START_BUF(nBAddr)));
    ONLD_ERR_PRINT((TEXT("[ONLD :  REG] ONLD_REG_CMD = 0x%04x\r\n"), ONLD_REG_CMD(nBAddr)));
    ONLD_ERR_PRINT((TEXT("[ONLD :  REG] ONLD_REG_SYS_CONF1 = 0x%04x\r\n"), ONLD_REG_SYS_CONF1(nBAddr)));

    ONLD_ERR_PRINT((TEXT("[ONLD :  REG] ONLD_REG_INT = 0x%04x\r\n"), ONLD_REG_INT(nBAddr)));
    ONLD_ERR_PRINT((TEXT("[ONLD :  REG] ONLD_REG_ULOCK_START_BA = 0x%04x\r\n"), ONLD_REG_ULOCK_START_BA(nBAddr)));
    ONLD_ERR_PRINT((TEXT("[ONLD :  REG] ONLD_REG_WR_PROTECT_STAT = 0x%04x\r\n"), ONLD_REG_WR_PROTECT_STAT(nBAddr)));
    ONLD_ERR_PRINT((TEXT("[ONLD :  REG] ONLD_REG_ECC_STAT = 0x%04x\r\n"), ONLD_REG_ECC_STAT(nBAddr)));
}
static INT32
_SetRWBlock(UINT32 nDev, UINT32 nSUbn, UINT32 nUBlks)
{
#if !defined(XSR_NBL2)	
    UINT32  nBAddr;
    
    ONLD_DBG_PRINT((TEXT("[ONLD : IN] ++onld_setrwarea()\r\n")));
	
    nBAddr = GET_DEV_BADDR(nDev);

    /*-----------------------------------------------------------------------*/
    /* Step 2. UNLOCK DEVICE                                                 */
    /*                                                                       */
    /*      a. Sequential Area Blocks are unlocked                           */
    /*-----------------------------------------------------------------------*/

    /* To skip otp locked first block */
    if ((nSUbn == 0) && (GET_ONLD_CTRL_STAT(nBAddr, OTPBL_STATE) == OTPBL_STATE))
    {
        nSUbn++;
        nUBlks--;
    }

    while (nUBlks > 0)
    {
        ONLD_REG_START_ADDR2(nBAddr) = (UINT16)MAKE_DBS(nSUbn);

        /* FBA(Flash Block Address) Set */
        ONLD_REG_START_ADDR1(nBAddr) = (UINT16)MAKE_FBA(nSUbn);
        /* Check block status */
        if (ONLD_REG_WR_PROTECT_STAT(nBAddr) == (UINT16)LOCK_TIGHTEN_STAT)
        {    
            ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  (SET_RW)nSUbn = %d is lock-tighten\r\n"), nSUbn));
            ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --ONLD_SetRWArea()\r\n")));
    
            return (ONLD_ILLEGAL_ACCESS);
        }

        /* Block Number Set */
        ONLD_REG_ULOCK_START_BA(nBAddr) = (UINT16)nSUbn;

        /* INT Stat Reg Clear */
        ONLD_REG_INT(nBAddr)            = (UINT16)INT_CLEAR;  

        /*-------------------------------------------------------------------*/
        /* ONLD Unlock CMD is issued                                         */
        /*-------------------------------------------------------------------*/
        ONLD_REG_CMD(nBAddr)            = (UINT16)ONLD_CMD_ULOCK_NAND;
    
        /* Physical Operation Time */
        while (GET_ONLD_INT_STAT(nBAddr, INT_MASTER) != (UINT16)INT_MASTER)
        {
            /* Wait until device ready */
            AD_Idle(INT_MODE);
        }

        if (GET_ONLD_CTRL_STAT(nBAddr, ERROR_STATE) == ERROR_STATE)
        {
            ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  Unlock Error(nPbn = %d)\r\n"), (UINT16)nSUbn));
   
            return (ONLD_CRITICAL_ERROR);
        }

        /* FBA(Flash Block Address) Set */
        ONLD_REG_START_ADDR1(nBAddr) = (UINT16)MAKE_FBA(nSUbn);

        /* Check Block status */
        WAIT_ONLD_WR_PROTECT_STAT(nBAddr, UNLOCKED_STAT);

        /* Dummy Function */
        ADD_DELAY(nBAddr, DUMMY_READ_CNT);

        ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  BlkNum = %d blocks are unlocked\r\n"),
                        nSUbn));
        nSUbn++;
        nUBlks--;
    }

    ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_setrwarea()\r\n")));
    return ONLD_SUCCESS;
    
#endif /* #if !defined(XSR_NBL2) */	
}

static INT32
_SetRWRange(UINT32 nDev, UINT32 nSUbn, UINT32 nUBlks)
{
#if !defined(XSR_NBL2)
    UINT32  nBAddr;
    UINT32  nTmpUBlks;
    UINT32  nTmpSUbn;
    UINT32  nLoop;
    UINT32  nCSValue  = FIRSTCHIP;
    BOOL32  bPreValue = FALSE32;

    ONLD_DBG_PRINT((TEXT("[ONLD: IN] ++onld_setrwarea() nDev = %d, nSUbn = %d, nUBlks = 0x%x\r\n"), 
                    nDev, nSUbn, nUBlks));

    nBAddr = GET_DEV_BADDR(nDev);


    nTmpSUbn  = nSUbn;
    nTmpUBlks = nUBlks;
    
    if (((FIRSTCHIP <= nSUbn) && (SECONDCHIP> nSUbn)) && ((nSUbn + nUBlks - 1) >= SECONDCHIP))
        nLoop = 2;
    else 
        nLoop = 1;
     
    while(nLoop > 0)
    {
        switch (nLoop)
        {
            case 2:
                ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  First Chip Selected\r\n")));
                nCSValue  = FIRSTCHIP;
                nUBlks    = SECONDCHIP - nSUbn;
                bPreValue = TRUE32;
                break;
            case 1:
                if (nSUbn >= SECONDCHIP)
                {
                    ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  Second Chip Selected\r\n")));
                    nCSValue = SECONDCHIP;
                    nSUbn   -= SECONDCHIP;
                }
                else if (bPreValue != TRUE32)
                {
                    ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  First Chip Selected\r\n")));
                    nCSValue = FIRSTCHIP;
                }
                
                else
                {
                    ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  Second Chip Selected\r\n")));
                    nCSValue = SECONDCHIP;
                    nSUbn = 0;
                    nUBlks = nTmpUBlks - nUBlks;
                }
                break;
        }    

        ONLD_REG_START_ADDR2(nBAddr) = (UINT16)MAKE_DBS(nCSValue);  
        ONLD_REG_START_ADDR1(nBAddr) = (UINT16)MAKE_FBA(nCSValue);

        /*-----------------------------------------------------------------------*/
        /* Step 2. UNLOCK DEVICE                                                 */
        /*                                                                       */
        /*      a. Sequential Area Blocks are unlocked                           */
        /*-----------------------------------------------------------------------*/

        /* To skip otp locked first block */
        if ((nSUbn == 0) && (GET_ONLD_CTRL_STAT(nBAddr, OTPBL_STATE) == OTPBL_STATE))
        {
            nSUbn++;
            nUBlks--;
        }

        if (nUBlks > 0)
        {	    
            /* Start Block Number Set */
            ONLD_REG_ULOCK_START_BA(nBAddr) = (UINT16)nSUbn;
            ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  ONLD_REG_ULOCK_START_BA(nBAddr) at SetRWArea 1: %x\r\n"),ONLD_REG_ULOCK_START_BA(nBAddr)));
            
            /* Last Block Number Set */
            ONLD_REG_ULOCK_END_BA(nBAddr)   = (UINT16)(nSUbn + nUBlks - 1);
            ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  ONLD_REG_ULOCK_END_BA(nBAddr) at SetRWArea 1: %x\r\n"),ONLD_REG_ULOCK_END_BA(nBAddr)));

            /* INT Stat Reg Clear */
            ONLD_REG_INT(nBAddr)            = (UINT16)INT_CLEAR;
            	            
            /*-------------------------------------------------------------------*/
            /* ONLD Unlock CMD is issued                                         */
            /*-------------------------------------------------------------------*/
            ONLD_REG_CMD(nBAddr)            = (UINT16)ONLD_CMD_ULOCK_NAND;
        
            /* Physical Operation Time */
            while (GET_ONLD_INT_STAT(nBAddr, INT_MASTER) != (UINT16)INT_MASTER)
            {
                /* Wait until device ready */
                AD_Idle(INT_MODE);
            }
        }
        nLoop--;
    }
    ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  From BlkNum = %d to BlkNum = %d blocks are unlocked\r\n"),
                            nTmpSUbn, (nTmpSUbn + nTmpUBlks -1)));
    ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_setrwarea()\r\n")));

    return ONLD_SUCCESS;
#endif /* #if !defined(XSR_NBL2) */
}
static INT32
_LockBlock (UINT32  nDev, UINT8  *pBufI, UINT32 nLenI)
{
    UINT32    nBAddr;
    UINT32    nSbn;
    UINT32    nBlks;

    nSbn                   = *(UINT32*)pBufI;
    nBlks                  = *((UINT32*)(pBufI+4));

    nBAddr = GET_DEV_BADDR(nDev);
	             
    while ( nBlks > 0 )
    {
    	ONLD_REG_START_ADDR2(nBAddr) = (UINT16)MAKE_DBS(nSbn);

        /* FBA(Flash Block Address) Set */
        ONLD_REG_START_ADDR1(nBAddr) = (UINT16)MAKE_FBA(nSbn);

        if (ONLD_REG_WR_PROTECT_STAT(nBAddr) == (UINT16)(LOCK_TIGHTEN_STAT))
        {    
            ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  (_LockBlock)nSbn = %d is lock-tighten\r\n"), nSbn));
            ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --_LockBlock()\r\n")));
    
            return (ONLD_ILLEGAL_ACCESS);
        }

        /* Block Number Set */
        ONLD_REG_ULOCK_START_BA(nBAddr) = (UINT16)nSbn;

        /* Int. Reg. set 0 */
        ONLD_REG_INT(nBAddr)            = (UINT16)INT_CLEAR;

        /*-------------------------------------------------------------------*/
        /* ONLD Unlock CMD is issued                                         */
        /*-------------------------------------------------------------------*/
        ONLD_REG_CMD(nBAddr)            = (UINT16)ONLD_CMD_LOCK_NAND;
                        
        while (GET_ONLD_INT_STAT(nBAddr, INT_MASTER) != (UINT16)INT_MASTER)
        {
         /* Wait until device ready */
            AD_Idle(INT_MODE);
        }
        
        if (GET_ONLD_CTRL_STAT(nBAddr, ERROR_STATE) == ERROR_STATE)
        {
            ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  Lock Block Error(nPbn = %d)\r\n"), (UINT16)nSbn));
   
            return (ONLD_CRITICAL_ERROR);
        }      

        /* FBA(Flash Block Address) Set */
        ONLD_REG_START_ADDR1(nBAddr) = (UINT16)MAKE_FBA(nSbn);

        /* Check Block status */
        WAIT_ONLD_WR_PROTECT_STAT(nBAddr, LOCKED_STAT);

        /* Dummy Function */
        ADD_DELAY(nBAddr, DUMMY_READ_CNT);

        nSbn++;
        nBlks--;
    }
    return ONLD_SUCCESS;
}

#ifdef LOCK_TIGHT        
static INT32
_LockTigBlock (UINT32  nDev, UINT8  *pBufI, UINT32 nLenI)
{
    UINT32    nBAddr;
    UINT32    nSbn;
    UINT32    nBlks;

    nSbn                   = *(UINT32*)pBufI;
    nBlks                  = *((UINT32*)(pBufI+4));

    nBAddr = GET_DEV_BADDR(nDev);
	             
    while ( nBlks > 0 )
    {

    	ONLD_REG_START_ADDR2(nBAddr) = (UINT16)MAKE_DBS(nSbn);

        /* FBA(Flash Block Address) Set */
        ONLD_REG_START_ADDR1(nBAddr) = (UINT16)MAKE_FBA(nSbn);

        if (ONLD_REG_WR_PROTECT_STAT(nBAddr) == (UINT16)(UNLOCKED_STAT))
        {    
            ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  (_LockTigBlock)nSbn = %d is unlock state\r\n"), nSbn));
            ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --_LockTigBlock()\r\n")));
    
            return (ONLD_ILLEGAL_ACCESS);
        }

        /* Block Number Set */
        ONLD_REG_ULOCK_START_BA(nBAddr) = (UINT16)nSbn; 

        /* Int. Reg. set 0 */
        ONLD_REG_INT(nBAddr)            = (UINT16)INT_CLEAR;

        /*-------------------------------------------------------------------*/
        /* ONLD Unlock CMD is issued                                         */
        /*-------------------------------------------------------------------*/
		            		            
        ONLD_REG_CMD(nBAddr)            = (UINT16)ONLD_CMD_LOCK_TIGHT;
		        
        while (GET_ONLD_INT_STAT(nBAddr, INT_MASTER) != (UINT16)INT_MASTER)
        {
		 /* Wait until device ready */
            AD_Idle(INT_MODE);
        }

        if (GET_ONLD_CTRL_STAT(nBAddr, ERROR_STATE) == ERROR_STATE)
        {
            ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  Lock Tight Error(nPbn = %d)\r\n"), (UINT16)nSbn));
   
            return (ONLD_CRITICAL_ERROR);
        }      

        /* FBA(Flash Block Address) Set */
        ONLD_REG_START_ADDR1(nBAddr) = (UINT16)MAKE_FBA(nSbn);

        /* Check Block status */
        WAIT_ONLD_WR_PROTECT_STAT(nBAddr, LOCK_TIGHTEN_STAT);

        /* Dummy Function */
        ADD_DELAY(nBAddr, DUMMY_READ_CNT);
        
        nSbn++;
        nBlks--;
    }
    return ONLD_SUCCESS;
}
#endif

static INT32
_LockTigRange (UINT32  nDev, UINT8  *pBufI, UINT32 nLenI)
{

    UINT32  nBAddr;
    nBAddr = GET_DEV_BADDR(nDev);
    if ((GET_DEV_DID(nDev) & 0x0008) == DDP_CHIP)
    {
        ONLD_REG_START_ADDR2(nBAddr) = (UINT16)(0x0000);
        ONLD_REG_START_ADDR1(nBAddr) = (UINT16)(0x0000);       
            
        ONLD_REG_INT(nBAddr)            = (UINT16)INT_CLEAR;
		    
       /*-------------------------------------------------------------------*/
       /* ONLD Unlock CMD is issued                                         */
       /*-------------------------------------------------------------------*/
        ONLD_REG_CMD(nBAddr)            = (UINT16)ONLD_CMD_LOCK_TIGHT;
		        
        while (GET_ONLD_INT_STAT(nBAddr, INT_MASTER) != (UINT16)INT_MASTER)
        {
		 /* Wait until device ready */
            AD_Idle(INT_MODE);
        }

        if (GET_ONLD_CTRL_STAT(nBAddr, ERROR_STATE) == ERROR_STATE)
        {
            ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  Lock Tight Error\r\n")));
   
            return (ONLD_CRITICAL_ERROR);
        }
        
        while(ONLD_REG_WR_PROTECT_STAT(nBAddr) != (UINT16)(LOCK_TIGHTEN_STAT ))
        {
         /* Wait until device Lock-tight */
        }

        ADD_DELAY(nBAddr, DUMMY_READ_CNT);
		
        ONLD_REG_START_ADDR2(nBAddr) = (UINT16)(0x8000);		
    	ONLD_REG_START_ADDR1(nBAddr) = (UINT16)(0x8000); 
    } /* if DDP_CHIP */
	    		
    ONLD_REG_INT(nBAddr)            = (UINT16)INT_CLEAR;
                	    
    /*-------------------------------------------------------------------*/
    /* ONLD Unlock CMD is issued                                         */
    /*-------------------------------------------------------------------*/
    ONLD_REG_CMD(nBAddr)            = (UINT16)ONLD_CMD_LOCK_TIGHT;
        
    while (GET_ONLD_INT_STAT(nBAddr, INT_MASTER) != (UINT16)INT_MASTER)
    {
	 /* Wait until device ready */
        AD_Idle(INT_MODE);
    }

    if (GET_ONLD_CTRL_STAT(nBAddr, ERROR_STATE) == ERROR_STATE)
    {
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  Lock Tight Error\r\n")));

        return (ONLD_CRITICAL_ERROR);
    }
    
    while(ONLD_REG_WR_PROTECT_STAT(nBAddr) != (UINT16)(LOCK_TIGHTEN_STAT ))
    {
     /* Wait until device Lock-tight */
    }

    ADD_DELAY(nBAddr, DUMMY_READ_CNT);

    return ONLD_SUCCESS; 	                
}
static VOID
_InitDevInfo(UINT32 nDev, UINT16 *pDID, UINT16 *pVID)
{
	UINT32 nBAddr;
	nBAddr = GET_DEV_BADDR(nDev);
	
	if(pDID == NULL || pVID == NULL)
	{
		ONLD_ERR_PRINT((TEXT("[ONLD : ERR] (_InitDevInfo) Illeagal Access\r\n")));
        return;
	}
	
    astONLDInfo[nDev].nBufSelSft       = 2;
    astONLDInfo[nDev].nSctSelSft       = 3;
    astONLDInfo[nDev].nBlkSelSft       = 8;
    astONLDInfo[nDev].nPgSelSft        = 2;
    astONLDInfo[nDev].nDDPSelSft       = 6;
    astONLDInfo[nDev].nFPASelSft       = 0;
    astONLDInfo[nDev].nFSAMask         = 3;
    astONLDInfo[nDev].nSctsPerPG       = 4;
    ONLD_CMD_LOCK_NAND                 = 0x002A;
    ONLD_CMD_LOCK_TIGHT                = 0x002C;
#if defined(USE_CACHE_READ) && !defined(USE_CACHE_AHEAD)
    astONLDInfo[nDev].MRead            = _cacheread;
#endif
#if defined(USE_CACHE_READ) && defined(USE_CACHE_AHEAD)
    astONLDInfo[nDev].MRead            = _cacheahead;
#endif
#if !defined(USE_CACHE_READ) && !defined(USE_CACHE_AHEAD)
    astONLDInfo[nDev].MRead            = _mread;
#endif
    
	if (((*pDID & 0x00F0) == CHIP_256M) || ((*pDID & 0x00F0) == CHIP_128M))
    {
    	astONLDInfo[nDev].nBufSelSft    = 1; 
    	astONLDInfo[nDev].nSctSelSft    = 1; 
    	astONLDInfo[nDev].nBlkSelSft    = 7; 
    	astONLDInfo[nDev].nPgSelSft     = 1; 
    	astONLDInfo[nDev].nFPASelSft    = 1;
    
    	astONLDInfo[nDev].nFSAMask   = 1;
    	astONLDInfo[nDev].nSctsPerPG = 2;
							
		if ((*pVID & 0x0F00) == M_DIE)
		{
			ONLD_CMD_LOCK_NAND       = 0x002B;
			ONLD_CMD_LOCK_TIGHT      = 0x002D; 
		}
        astONLDInfo[nDev].MRead      = _mread;
	}	

    if ((*pDID & 0x0003) == 0x01)
    {
        astONLDInfo[nDev].MRead  = _mread;
    }
    
    if ((*pDID & 0x00F0) == CHIP_512M)
    {
        /* Cache-read is enabled only for 512Mb B-die 1.8V */
		if ((*pVID & 0x0F00) == M_DIE || (*pVID & 0x0F00) == A_DIE)
		{
            astONLDInfo[nDev].MRead  = _mread;
		}
    }
	if ((*pDID & 0x00F0) == CHIP_1G)
	{
		if ((*pVID & 0x0F00) == M_DIE)
		{
            astONLDInfo[nDev].MRead  = _mread;
		}
	}
    
	if ((*pDID & 0x00F0) == CHIP_2G)
	{
		astONLDInfo[nDev].nDDPSelSft    = 5;
        
        if ((*pDID & 0x0008) == DDP_CHIP)
        {
    		if ((*pVID & 0x0F00) == M_DIE)
    		{
                astONLDInfo[nDev].MRead  = _mread;
    		}
        }
	}
	else if ((*pDID & 0x00F0) == CHIP_4G)
	{
		astONLDInfo[nDev].nDDPSelSft    = 4;	    
	}
		               
    if (astONLDDev[nDev].pstDevSpec->nLKFlag == BLK_LK)
    {                  
        astONLDInfo[nDev].SetRWArea = _SetRWBlock;
#ifdef LOCK_TIGHT        
        astONLDInfo[nDev].LockTight = _LockTigBlock;
#else
		/* for erase refresh, lock-tight is disabled */
		astONLDInfo[nDev].LockTight = _LockBlock;
#endif		        
    }                  
    else if (astONLDDev[nDev].pstDevSpec->nLKFlag == RNG_LK)              
    {                  
        astONLDInfo[nDev].SetRWArea = _SetRWRange;    
        astONLDInfo[nDev].LockTight = _LockTigRange;
    }
    else
    {
        astONLDInfo[nDev].SetRWArea = NULL;    
        astONLDInfo[nDev].LockTight = NULL;               
    }                        
}                      
                       
static VOID            
_WriteMain(register UINT8 *pBuf, UINT32 *pAddr, UINT32 nScts)
{                      
    register UINT32  nCnt;
    volatile UINT16 *pTgt16;
#if !defined(ALIGNED_MEMCPY)
    volatile UINT16 *pSrc16;
#endif                 
    register UINT16  nReg;
                       
    pTgt16 = (volatile UINT16*)pAddr;
                       
    if (((UINT32)pBuf & 0x01) != 0)
	{                  
        for (nCnt = 0; nCnt < (ONLD_MAIN_SIZE/2) * nScts; nCnt++)
        {              
#if defined(XSR_BIG_ENDIAN)
            nReg   = *pBuf++ << 8;
            nReg  |= *pBuf++;
#else
            nReg   = *pBuf++;
            nReg  |= *pBuf++ << 8;
#endif
            *pTgt16++ = nReg;
        }
    }
    else
    {
#if defined(ALIGNED_MEMCPY)
        MEMCPY(pAddr, pBuf, (ONLD_MAIN_SIZE * nScts));
#else        
        pSrc16 = (volatile UINT16*)pBuf;
        
        for (nCnt = 0; nCnt < (ONLD_MAIN_SIZE/2) * nScts; nCnt++)
            *pTgt16++ = *pSrc16++;
#endif    
    }
}

static VOID
_ReadMain(register UINT8 *pBuf, UINT32 *pAddr, UINT32 nScts)
{
    register UINT32  nCnt;
    volatile UINT16 *pTgt16;
#if !defined(ALIGNED_MEMCPY)
    volatile UINT16 *pSrc16;
#endif
    register UINT16  nReg;

    pTgt16 = (volatile UINT16*)pAddr;

    if (((UINT32)pBuf & 0x01) != 0)
	{
        for (nCnt = 0; nCnt < (ONLD_MAIN_SIZE/2) * nScts; nCnt++)
        {
            nReg   = *pTgt16++;
#if defined(XSR_BIG_ENDIAN)
            *pBuf++ = (UINT8)(nReg >> 8);
            *pBuf++ = (UINT8)(nReg);
#else
            *pBuf++ = (UINT8)(nReg);
            *pBuf++ = (UINT8)(nReg >> 8);
#endif
        }
    }
    else
    {
#if defined(ALIGNED_MEMCPY)
        MEMCPY(pBuf, pAddr, (ONLD_MAIN_SIZE * nScts));
#else        
        pSrc16 = (volatile UINT16*)pBuf;
        
        for (nCnt = 0; nCnt < (ONLD_MAIN_SIZE/2) * nScts; nCnt++)
            *pSrc16++ = *pTgt16++;
#endif    
    }
}

static VOID
_WriteSpare(register UINT8 *pBuf, UINT32 *pAddr, UINT32 nScts)
{
    register UINT32  nCnt;
    volatile UINT16 *pTgt16;
#if !defined(ALIGNED_MEMCPY)
    volatile UINT16 *pSrc16;
#endif    
    register UINT16  nReg;

#if defined(XSR_BIG_ENDIAN)
    UINT32  nIdx;
    UINT16  nVal16;
    volatile UINT16 *pAddr16 = (volatile UINT16*)(pAddr + 1);
#endif
        
    pTgt16 = (volatile UINT16*)pAddr;

    if (((UINT32)pBuf & 0x01) != 0)
	{
        for (nCnt = 0; nCnt < (ONLD_SPARE_SIZE/2) * nScts; nCnt++)
        {
#if defined(XSR_BIG_ENDIAN)
            nReg   = *pBuf++ << 8;
            nReg  |= *pBuf++;
#else
            nReg   = *pBuf++;
            nReg  |= *pBuf++ << 8;
#endif
            *pTgt16++ = nReg;
        }
    }
    else
    {
#if defined(ALIGNED_MEMCPY)
        MEMCPY(pAddr, pBuf, (ONLD_SPARE_SIZE * nScts));
#else
        pSrc16 = (volatile UINT16*)pBuf;
        
        for (nCnt = 0; nCnt < (ONLD_SPARE_SIZE/2) * nScts; nCnt++)
            *pTgt16++ = *pSrc16++;
#endif
}

#if defined(XSR_BIG_ENDIAN)
    for(nIdx = 0; nIdx < nScts; nIdx++)
    {
        nVal16   = *pAddr16;
       *pAddr16  = (nVal16 >> 8) | (nVal16 << 8);
        pAddr16 += (ONLD_SPARE_SIZE / 2);
    }
#endif

}

static VOID
_ReadSpare(register UINT8 *pBuf, UINT32 *pAddr, UINT32 nScts)
{
    register UINT32  nCnt;
    volatile UINT16 *pTgt16;
#if !defined(ALIGNED_MEMCPY)
    volatile UINT16 *pSrc16;
#endif    
    register UINT16  nReg;
    
#if defined(XSR_BIG_ENDIAN)
    UINT32  nIdx;
    UINT16  nVal16;
    volatile UINT16 *pAddr16 = (volatile UINT16*)(pAddr + 1);

    for(nIdx = 0; nIdx < nScts; nIdx++)
    {
        nVal16   = *pAddr16;
       *pAddr16  = (nVal16 >> 8) | (nVal16 << 8);
        if(nScts == 1)
        {
            pAddr16 += 2;
            nVal16   = *pAddr16;
           *pAddr16  = (nVal16 >> 8) | (nVal16 << 8);            
            pAddr16 += 1;
            nVal16   = *pAddr16;
           *pAddr16  = (nVal16 >> 8) | (nVal16 << 8);
            pAddr16  += 5;
        }
        else
        {
            pAddr16 += (ONLD_SPARE_SIZE / 2);
        }
    }
#endif
  
    pTgt16 = (volatile UINT16*)pAddr;

    if (((UINT32)pBuf & 0x01) != 0)
	{
        for (nCnt = 0; nCnt < (ONLD_SPARE_SIZE/2) * nScts; nCnt++)
        {
            nReg   = *pTgt16++;
#if defined(XSR_BIG_ENDIAN)
            *pBuf++ = (UINT8)(nReg >> 8);
            *pBuf++ = (UINT8)(nReg);
#else
            *pBuf++ = (UINT8)(nReg);
            *pBuf++ = (UINT8)(nReg >> 8);
#endif
        }
    }
    else
    {
#if defined(ALIGNED_MEMCPY)
        MEMCPY(pBuf, pAddr, (ONLD_SPARE_SIZE * nScts));
#else
        pSrc16 = (volatile UINT16*)pBuf;
        
        for (nCnt = 0; nCnt < (ONLD_SPARE_SIZE/2) * nScts; nCnt++)
            *pSrc16++ = *pTgt16++;
#endif
    }
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      _StrictChk                                                           */
/* DESCRIPTION                                                               */
/*      This function performs strict checks                                 */
/* PARAMETERS                                                                */
/*      nDev                                                                 */
/*          Physical Device Number (0 ~ 7)                                   */
/*      nPsn                                                                 */
/*          Physical Sector Number                                           */
/*      nScts                                                                */
/*          Number of Sectors                                                */
/* RETURN VALUES                                                             */
/*      TRUE32                                                               */
/*          Strict Check OK                                                  */
/*      FALSE32                                                              */
/*          Strcit Check Failure                                             */
/* NOTES                                                                     */
/*      Device Power Flag and Open Flag Check                                */
/*      Each of Out of Bound Check                                           */
/*                                                                           */
/*****************************************************************************/
#if defined(STRICT_CHK)
static BOOL32
_StrictChk(UINT32 nDev, UINT32 nPsn, UINT32 nScts)
{
    UINT32 nPbn;
    
    /* Check Device Number */
    if (nDev > MAX_SUPPORT_DEVS)
    {
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  Invalid Device Number (nDev = %d)\r\n"), 
                       nDev));
        return (FALSE32);
    }

    nPbn = GET_PBN(nDev, nPsn);

    /* Check Device Open Flag */
    if (CHK_DEV_OPEN_FLAG(nDev) == FALSE32)
    {
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  Device is not opened\r\n")));
        return (FALSE32);
    }

    /* Check Device Power Flag */
    if (CHK_DEV_PWR_FLAG(nDev) == FALSE32)
    {
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  Dev PwrFlg is not set\r\n")));
        return (FALSE32);
    }
    
    /* Check Block Out of Bound                      */
    /* if nBlk > GET_NUM_OF_BLKS then it is an error.*/
    if (CHK_BLK_OOB(nDev, nPbn))
    {
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  Pbn is Out of Bound\r\n")));
        return (FALSE32);
    }

    /* Check Sector Out of Bound                                            */
    /* if ((nPsn & 0x03) + nSct) > Sectors per Page(=4) then it is an error */
    if (CHK_SCT_OOB(nDev, nPsn, nScts))
    {
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  Psn + Scts is Out of Bound\r\n")));
        return (FALSE32);
    }

    /* Everything is OK, then TRUE32 can be returned */
    return (TRUE32);
}
#endif  /* #if defined(STRICT_CHK) */


#if defined(DEFERRED_CHK)
/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      _ChkPrevOpResult                                                     */
/* DESCRIPTION                                                               */
/*      This function checks the result of previous opreation                */
/* PARAMETERS                                                                */
/*      nDev                                                                 */
/*          Physical Device Number (0 ~ 7)                                   */
/* RETURN VALUES                                                             */
/*      ONLD_WRITE_ERROR | ONLD_PREV_OP_RESULT                               */
/*          Previous Write Failure                                           */
/*      ONLD_ERASE_ERROR | ONLD_PREV_OP_RESULT                               */
/*          Previous Erase Failure                                           */
/*      ONLD_WR_PROTECT_ERROR | ONLD_PREV_OP_RESULT                          */
/*          Previous Operation Write Protect Error                           */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
static INT32
_ChkPrevOpResult(UINT32 nDev)
{
    UINT32 nBAddr, nPbn;
    INT32  nRet = ONLD_SUCCESS;
    OpType ePrevOp;

    if (GET_PREV_OP_TYPE(nDev) == NONE)
    {
        return (nRet);
    }
    
    nBAddr  = GET_DEV_BADDR(nDev);
    nPbn = GET_PBN(nDev, pstPrevOpInfo[nDev]->nPsn);
    ePrevOp = GET_PREV_OP_TYPE(nDev);

    /* Previous Operation Type Clear */
    pstPrevOpInfo[nDev]->ePreOp = NONE;

    ONLD_REG_START_ADDR2(nBAddr) = (UINT16)MAKE_DBS(nPbn);

//#ifdef CONFIG_RFS_TINYBML_1
#ifdef CONFIG_RFS_TINYBML
	if ( xsr_shared )
	{
		return (nRet);
	}
#endif

    switch(ePrevOp)
    {
        case WRITE:
        case CPBACK:
                WAIT_ONLD_INT_STAT_CHECKING_PROTECTION(nBAddr, PEND_INT);
                
                if (GET_ONLD_CTRL_STAT(nBAddr, ERROR_STATE) == ERROR_STATE)
                {
                    ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  Previous Write Error\r\n")));
                return (ONLD_WRITE_ERROR | ONLD_PREV_OP_RESULT);
                }
                break;   

        case ERASE:
                WAIT_ONLD_INT_STAT_CHECKING_PROTECTION(nBAddr, PEND_INT);
        
                if (GET_ONLD_CTRL_STAT(nBAddr, ERROR_STATE) == ERROR_STATE)
                {
                    ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  Previous Erase Error\r\n")));
                return (ONLD_ERASE_ERROR | ONLD_PREV_OP_RESULT);
                }
                break;   
    
        case MERASE:
                WAIT_ONLD_INT_STAT_CHECKING_PROTECTION(nBAddr, PEND_INT);
                        
            if (onld_eraseverify(nDev, 
                                 pstPrevOpInfo[nDev]->pstPreMEArg, 
                                 pstPrevOpInfo[nDev]->nFlag) != ONLD_SUCCESS)
            {
                ONLD_ERR_PRINT((TEXT("[ONLD:ERR]  Previous MErase Error\r\n")));
                nRet = ONLD_MERASE_ERROR | ONLD_PREV_OP_RESULT;
            }		    
            return (nRet);
            break;

        case READ:
        /* Wait until device ready */
                WAIT_ONLD_INT_STAT(nBAddr, PEND_READ);
#if defined(USE_CACHE_READ) && defined(USE_CACHE_AHEAD)
            if (astONLDInfo[nDev].MRead != _mread)
            {
        	    ONLD_REG_INT (nBAddr) = (UINT16) INT_CLEAR;
        	    ONLD_REG_CMD (nBAddr) = (UINT16) ONLD_CMD_FINISH_CACHE_READ;
    
                    /* Wait until device ready */
                    WAIT_ONLD_INT_STAT(nBAddr, PEND_INT);                
            }
#endif /* #if defined(USE_CACHE_READ) && defined(USE_CACHE_AHEAD) */
            return (nRet);
            
        default:
        /* Wait until device ready */
            WAIT_ONLD_INT_STAT(nBAddr, PEND_INT);
            return (nRet);
    }
            
    ADD_DELAY(nBAddr, DUMMY_READ_CNT);

    return ONLD_SUCCESS;
}
#endif /* #if defined(DEFERRED_CHK) */

#if defined(USE_CACHE_READ) && defined(USE_CACHE_AHEAD)
static INT32
_ChkPrevOpResultByCacheRead(UINT32 nDev)
{
    UINT32 nBAddr, nPbn;
    INT32  nRet = ONLD_SUCCESS;
    OpType ePrevOp;

    if (GET_PREV_OP_TYPE(nDev) == NONE)
    {
        return (nRet);
    }
    
    nBAddr  = GET_DEV_BADDR(nDev);
    nPbn = GET_PBN(nDev, pstPrevOpInfo[nDev]->nPsn);
    ePrevOp = GET_PREV_OP_TYPE(nDev);

    /* Previous Operation Type Clear */
    pstPrevOpInfo[nDev]->ePreOp = NONE;

    ONLD_REG_START_ADDR2(nBAddr) = (UINT16)MAKE_DBS(nPbn);

#ifdef CONFIG_RFS_TINYBML
	if ( xsr_shared )
	{
		return (nRet);
	}
#endif

    switch(ePrevOp)
    {
        case WRITE:
        case CPBACK:
                WAIT_ONLD_INT_STAT_CHECKING_PROTECTION(nBAddr, PEND_INT);
                
                if (GET_ONLD_CTRL_STAT(nBAddr, ERROR_STATE) == ERROR_STATE)
                {
                    ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  Previous Write Error\r\n")));
                return (ONLD_WRITE_ERROR | ONLD_PREV_OP_RESULT);
                }
                break;   

        case ERASE:
                WAIT_ONLD_INT_STAT_CHECKING_PROTECTION(nBAddr, PEND_INT);
        
                if (GET_ONLD_CTRL_STAT(nBAddr, ERROR_STATE) == ERROR_STATE)
                {
                    ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  Previous Erase Error\r\n")));
                return (ONLD_ERASE_ERROR | ONLD_PREV_OP_RESULT);
                }
                break;  

        case MERASE:
               WAIT_ONLD_INT_STAT_CHECKING_PROTECTION(nBAddr, PEND_INT);
                        
            if (onld_eraseverify(nDev, 
                                 pstPrevOpInfo[nDev]->pstPreMEArg, 
                                 pstPrevOpInfo[nDev]->nFlag) != ONLD_SUCCESS)
            {
                ONLD_ERR_PRINT((TEXT("[ONLD:ERR]  Previous MErase Error\r\n")));
                nRet = ONLD_MERASE_ERROR | ONLD_PREV_OP_RESULT;
            }		    
            return (nRet);
            break;

        case READ:
            WAIT_ONLD_INT_STAT(nBAddr, PEND_READ);

            return (nRet);

        default:
        /* Wait until device ready */
            WAIT_ONLD_INT_STAT(nBAddr, PEND_INT);
            return (nRet);
    }

    ADD_DELAY(nBAddr, DUMMY_READ_CNT);

    return ONLD_SUCCESS;
}
#endif /* #if defined(USE_CACHE_READ) && defined(USE_CACHE_AHEAD) */

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      _GetDevID                                                            */
/* DESCRIPTION                                                               */
/*      This function gets Manufacturer ID, Device ID and Version ID of      */
/*      NAND flash device.                                                   */
/* PARAMETERS                                                                */
/*      nDev                                                                 */
/*          Physical Device Number (0 ~ 7)                                   */
/*      pMID                                                                 */
/*          Manufacturer ID                                                  */
/*      pDID                                                                 */
/*          Device ID                                                        */
/*      pVID                                                                 */
/*          Version ID                                                       */
/* RETURN VALUES                                                             */
/*      None                                                                 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
static VOID
_GetDevID(UINT32 nDev, UINT16 *pMID, UINT16 *pDID, UINT16 *pVID)
{
    UINT32  nBAddr;

    nBAddr = GET_DEV_BADDR(nDev);

    ONLD_DBG_PRINT((TEXT("[ONLD :  IN] ++_GetDevID() nBAddr = 0x%x\r\n"), nBAddr));
	
	if ((pMID == NULL) || (pDID == NULL) || (pVID == NULL))
    {
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (_GetDevID) NULL Pointer Error\r\n")));
		ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --_GetDevID()\r\n")));
        return;
    }
    
    *pMID = (UINT16)ONLD_REG_MANUF_ID(nBAddr);
    *pDID = (UINT16)ONLD_REG_DEV_ID  (nBAddr);
    *pVID = (UINT16)ONLD_REG_VER_ID  (nBAddr);
    ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --_GetDevID()\r\n")));
}
   
/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      _ChkReadDisturb                                                      */
/* DESCRIPTION                                                               */
/*      This function checks if ECC 1-bit error by read disturbance occurs   */
/* PARAMETERS                                                                */
/*		nScts																 */
/*			Number of sectors												 */
/*		nEccStat															 */
/*			Value of Ecc status register									 */
/*		aEccRes  															 */
/*			Array of the value of Ecc result register           			 */
/*      pMBuf                                                                */
/*          Buffer for main area                                             */
/*      pSBuf                                                                */
/*          Buffer for spare area                                            */
/* RETURN VALUES                                                             */
/*      TRUE32                                                               */
/*			If 1-bit error by read disturbance happens						 */
/*		FALSE32																 */
/*			If not															 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
BOOL32
_ChkReadDisturb(UINT32 nScts, UINT16 nEccStat, 
				UINT16* pEccRes, UINT8* pMBuf, UINT8* pSBuf)
{        
	UINT16	nEccMMask;
	UINT16	nEccSMask;
	UINT16	nEccRes;
	UINT8	nEccPosWord;
	UINT8	nEccPosBit;
	UINT16	nEccPosByte;
	UINT8 	nChkMask;
	UINT8	nIdx;
		
	ONLD_DBG_PRINT((TEXT("[ONLD :  IN] ++_ChkReadDisturb() \r\n")));
	
	if (nEccStat == 0)
	{
		ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --_ChkReadDisturb()\r\n")));
		return FALSE32;
	}
	
	for (nIdx = 0;nIdx < nScts; nIdx++)
	{
		nEccMMask = (0x04 << (nIdx * 4));
		nEccSMask = (0x01 << (nIdx * 4));
		
		if (pMBuf != NULL)
		{
			if (nEccStat & nEccMMask)
			{
				/* 1-Bit error */
				nEccRes = pEccRes[nIdx * 2];
				
				nEccPosWord = (UINT8)((nEccRes & 0x0ff0)>>4);
				nEccPosBit = (UINT8)(nEccRes & 0xf);
				
				nEccPosByte = nIdx * 512 + nEccPosWord * 2;
				
				if (nEccPosBit >= 8)
				{
					nEccPosByte++;					
					nEccPosBit -= 8;
				}
								
				nChkMask = 0x01 << nEccPosBit;
				
				if ((pMBuf[nEccPosByte] & nChkMask) != 0)
				{
					ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --_ChkReadDisturb()\r\n")));
					return TRUE32;
				}
			}
		}
		
		if (pSBuf != NULL)
		{
			if (nEccStat & nEccSMask)
			{
				/* 1-Bit error */
				nEccRes = pEccRes[nIdx * 2 + 1];
				
				nEccPosWord = (UINT8)((nEccRes & 0x30)>>4);
				nEccPosBit = (UINT8)(nEccRes & 0xf);
				
				nEccPosByte = nIdx * 16 + (nEccPosWord + 1)* 2;
				
				if (nEccPosBit >= 8)
				{
					nEccPosByte++;
					nEccPosBit -= 8;					
				}
								
				nChkMask = 0x01 << nEccPosBit;
				
				if ((pSBuf[nEccPosByte] & nChkMask) != 0)
				{
					ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --_ChkReadDisturb()\r\n")));
					return TRUE32;
				}
			}
		}
	}
	
	ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --_ChkReadDisturb()\r\n")));
	
	return FALSE32;
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      onld_flushop                                                         */
/* DESCRIPTION                                                               */
/*      This function waits until the completion of the last physical        */
/*      operation                                                            */
/* PARAMETERS                                                                */
/*      nDev                                                                 */
/*          Physical Device Number (0 ~ 7)                                   */
/* RETURN VALUES                                                             */
/*      ONLD_SUCCESS                                                         */
/*          Flush operation success                                          */
/*      ONLD_WRITE_ERROR | ONLD_PREV_OP_RESULT                               */
/*          Previous Write Failure                                           */
/*      ONLD_ERASE_ERROR | ONLD_PREV_OP_RESULT                               */
/*          Previous Erase Failure                                           */
/*      ONLD_WR_PROTECT_ERROR | ONLD_PREV_OP_RESULT                          */
/*          Previous Operation Write Protect Error                           */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
INT32
onld_flushop(UINT32 nDev)
{
	UINT32 nBAddr;

#if defined(DEFERRED_CHK)
    INT32  nRes;
#endif /* #if defined(DEFERRED_CHK) */

    nBAddr = GET_DEV_BADDR(nDev);
	
    ONLD_DBG_PRINT((TEXT("[ONLD :  IN] ++onld_flushop() nDev = %d\r\n"), nDev));

#if defined(DEFERRED_CHK)
    if ((nRes = _ChkPrevOpResult(nDev)) != ONLD_SUCCESS)
    {
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (FLUSH_OP)Previous Operation Error 0x%08x\r\n"), 
                        nRes));
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (FLUSH_OP)Dev = %d, Psn = %d, Scts = %d"),
                        nDev,
                        pstPrevOpInfo[nDev]->nPsn, 
                        pstPrevOpInfo[nDev]->nScts));
        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_flushop()\r\n")));
        return nRes;
    }
#endif /* #if defined(DEFERRED_CHK) */

    ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_flushop()\r\n")));

    return (ONLD_SUCCESS);
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      onld_getdevinfo                                                      */
/* DESCRIPTION                                                               */
/*      This function reports device information to upper layer.             */
/* PARAMETERS                                                                */
/*      nDev                                                                 */
/*          Physical Device Number (0 ~ 7)                                   */
/*      pstFILDev                                                            */
/*          Data structure of device information which is required from FIL  */
/* RETURN VALUES                                                             */
/*      ONLD_ILLEGAL_ACCESS                                                  */
/*          Illegal Access                                                   */
/*      ONLD_SUCCESS                                                         */
/*          Success                                                          */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
INT32 
onld_getdevinfo(UINT32 nDev, LLDSpec *pstFILDev)
{
	UINT32 nBAddr;
	
    ONLD_DBG_PRINT((TEXT("[ONLD :  IN] ++onld_getdevinfo() nDev = %d\r\n"), nDev));

#if defined(STRICT_CHK)    
    if (astONLDDev[nDev].pstDevSpec == NULL)
    {
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (GET_DEV_INFO)Illeagal Access\r\n")));
        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_getdevinfo()\r\n")));
        return(ONLD_ILLEGAL_ACCESS);
    }
    if (pstFILDev == NULL)
    {
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (GET_DEV_INFO)Illeagal Access\r\n")));
        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_getdevinfo()\r\n")));
        return(ONLD_ILLEGAL_ACCESS);
    }
#endif  /* #if defined(STRICT_CHK) */

    nBAddr = GET_DEV_BADDR(nDev);
    
    pstFILDev->nMCode       = (UINT8)astONLDDev[nDev].pstDevSpec->nMID;
    pstFILDev->nDCode       = (UINT8)astONLDDev[nDev].pstDevSpec->nDID;
    pstFILDev->nNumOfBlks   = (UINT16)astONLDDev[nDev].pstDevSpec->nNumOfBlks;
    pstFILDev->nPgsPerBlk   = (UINT16)GET_PGS_PER_BLK(nDev);
    pstFILDev->nBlksInRsv   = (UINT16)astONLDDev[nDev].pstDevSpec->nBlksInRsv;

    pstFILDev->nSctsPerPg   = (UINT8)GET_SCTS_PER_PG(nDev);
    pstFILDev->nNumOfPlane  = (UINT8)astONLDDev[nDev].pstDevSpec->nNumOfPlanes;

    pstFILDev->nBadPos      = (UINT8)astONLDDev[nDev].pstDevSpec->nBadPos;     
    pstFILDev->nLsnPos      = (UINT8)astONLDDev[nDev].pstDevSpec->nLsnPos;     
    pstFILDev->nEccPos      = (UINT8)astONLDDev[nDev].pstDevSpec->nEccPos;     
    pstFILDev->nBWidth      = (UINT8)astONLDDev[nDev].pstDevSpec->nBWidth;     
    pstFILDev->nMEFlag      = (UINT8)astONLDDev[nDev].pstDevSpec->nMEFlag;
       		
    pstFILDev->aUID[0]      = 0xEC;
    pstFILDev->aUID[1]      = 0x00;
    pstFILDev->aUID[2]      = 0xC5;
    pstFILDev->aUID[3]      = 0x5F;
    pstFILDev->aUID[4]      = 0x3D;
    pstFILDev->aUID[5]      = 0x12;
    pstFILDev->aUID[6]      = 0x34;
    pstFILDev->aUID[7]      = 0x37;
    pstFILDev->aUID[8]      = 0x34;
    pstFILDev->aUID[9]      = 0x33;
    pstFILDev->aUID[10]     = 0x30;
    pstFILDev->aUID[11]     = 0x30;
    pstFILDev->aUID[12]     = 0x03;
    pstFILDev->aUID[13]     = 0x03;
    pstFILDev->aUID[14]     = 0x06;
    pstFILDev->aUID[15]     = 0x02;

    pstFILDev->nTrTime      = (UINT32)astONLDDev[nDev].pstDevSpec->nTrTime;
    pstFILDev->nTwTime      = (UINT32)astONLDDev[nDev].pstDevSpec->nTwTime;
    pstFILDev->nTeTime      = (UINT32)astONLDDev[nDev].pstDevSpec->nTeTime;
    pstFILDev->nTfTime      = (UINT32)astONLDDev[nDev].pstDevSpec->nTfTime;

    ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_getdevinfo()\r\n")));
    
    return (ONLD_SUCCESS);
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      onld_init                                                            */
/* DESCRIPTION                                                               */
/*      This function initializes ONLD                                       */
/*          - constructioin and initialization of internal data structure    */
/* PARAMETERS                                                                */
/*      nDev                                                                 */
/*          Physical Device Number (0 ~ 7)                                   */
/*      pParm                                                                */
/*          Device Parmameter Pointer                                        */
/* RETURN VALUES                                                             */
/*      ONLD_INIT_FAILURE                                                    */
/*          ONLD initialization faiure                                       */
/*      ONLD_SUCCESS                                                         */
/*          ONLD initialization success                                      */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
INT32 
onld_init(VOID *pParm)
{ 
    UINT32        nCnt;
    UINT32        nIdx;
    UINT32        nVol;
    static BOOL32 nInitFlg = FALSE32;
    vol_param  *pstParm = (vol_param*)pParm;

    ONLD_DBG_PRINT((TEXT("[ONLD :  IN] ++onld_init()\r\n")));

	if (nInitFlg == TRUE32)
	{
	    ONLD_DBG_PRINT((TEXT("[ONLD:MSG]  already initialized\r\n")));
	    ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_init()\r\n")));

	    return(ONLD_SUCCESS);
	}

    for (nCnt = 0; nCnt < MAX_SUPPORT_DEVS; nCnt++)
    {
        pstPrevOpInfo[nCnt]         = (PrevOpInfo *) NULL;
	    astONLDDev[nCnt].bOpen      = FALSE32;
	    astONLDDev[nCnt].bPower     = FALSE32;
	    astONLDDev[nCnt].pstDevSpec = NULL;
    }

    for (nVol = 0; nVol < XSR_MAX_VOL; nVol++)
    {
        for (nIdx = 0; nIdx < (XSR_MAX_DEV / XSR_MAX_VOL); nIdx++)
        {
            nCnt = nVol * (XSR_MAX_DEV / XSR_MAX_VOL) + nIdx;

            ONLD_DBG_PRINT((TEXT("[ONLD:MSG]  nVol = %d nDev : %d Base Address : 0x%x\r\n"), nVol, nCnt, pstParm[nVol].nBaseAddr[nIdx]));

            if (pstParm[nVol].nBaseAddr[nIdx] != NOT_MAPPED)
    	        astONLDDev[nCnt].BaseAddr   = AD_Pa2Va(pstParm[nVol].nBaseAddr[nIdx]);
            else
                astONLDDev[nCnt].BaseAddr   = NOT_MAPPED;
        }
    }

    /* Initialize spare buffer with 0xff */
    MEMSET(aEmptySBuf, 0xff, sizeof(aEmptySBuf));

    nInitFlg = TRUE32;
    
    ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_init()\r\n")));

    return(ONLD_SUCCESS);
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      onld_open                                                            */
/* DESCRIPTION                                                               */
/*      This function opnes ONLD device driver                               */
/* PARAMETERS                                                                */
/*      nDev                                                                 */
/*          Physical Device Number (0 ~ 7)                                   */
/* RETURN VALUES                                                             */
/*      ONLD_OPEN_FAILURE                                                    */
/*          ONLD Open faiure                                                 */
/*      ONLD_SUCCESS                                                         */
/*          ONLD Open success                                                */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
INT32
onld_open(UINT32 nDev)
{
    INT32   nCnt;
    UINT16  nMID, nDID, nVID;
    LLDMEArg   *pstMEArg; 
#if defined(USE_CACHE_READ) && defined(USE_CACHE_AHEAD)
    UINT32 nBAddr;
#endif /* #if defined(USE_CACHE_READ) && defined(USE_CACHE_AHEAD) */

    ONLD_DBG_PRINT((TEXT("[ONLD :  IN] ++onld_open() nDev = %d\r\n"), nDev));

    SET_PWR_FLAG(nDev);

    _GetDevID(nDev, &nMID, &nDID, &nVID);

    ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  nDev = %02d\r\n"), nDev));
    ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  nMID = 0x%04x\r\n"), nMID));
    ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  nDID = 0x%04x\r\n"), nDID));
    ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  nVID = 0x%04x\r\n"), nVID));

    for (nCnt = 0; astNandSpec[nCnt].nMID != 0; nCnt++)
    {
        if (nDID == astNandSpec[nCnt].nDID)
        {
        	if ((nVID >> 8) == astNandSpec[nCnt].nGEN)
        	{
                astONLDDev[nDev].bOpen      = TRUE32;
                astONLDDev[nDev].pstDevSpec = &astNandSpec[nCnt];
                break;
            }
            else
            {
                continue;
            }
        }
    }

    _InitDevInfo(nDev, &nDID, &nVID);   
			   	
    if (astONLDDev[nDev].bOpen != TRUE32)
    {
        CLEAR_PWR_FLAG(nDev);

        ONLD_ERR_PRINT((TEXT("[ONLD:ERR]  (OPEN)Unknown Device\r\n")));
        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_open()\r\n")));

        return (ONLD_OPEN_FAILURE);
    }

    if (pstPrevOpInfo[nDev] == (PrevOpInfo*)NULL)
    {
        pstPrevOpInfo[nDev] = (PrevOpInfo*)MALLOC(sizeof(PrevOpInfo));

        if (pstPrevOpInfo[nDev] == (PrevOpInfo*)NULL)
        {
            ONLD_ERR_PRINT((TEXT("[ONLD:ERR]  (OPEN)Memory Alloc Error\r\n")));
            ONLD_DBG_PRINT((TEXT("[ONLD:OUT] --onld_open()\r\n")));

            return (ONLD_OPEN_FAILURE);
        }
    }

#if defined(DEFERRED_CHK)
    pstPrevOpInfo[nDev]->ePreOp      = NONE;
    pstPrevOpInfo[nDev]->nCmd        = 0;
    pstPrevOpInfo[nDev]->nPsn        = 0;
    pstPrevOpInfo[nDev]->nScts       = 0;
    pstPrevOpInfo[nDev]->nFlag       = 0;
    pstPrevOpInfo[nDev]->pstPreMEArg = (LLDMEArg*)MALLOC(sizeof(LLDMEArg));
    
    pstMEArg = pstPrevOpInfo[nDev]->pstPreMEArg;
    if (pstMEArg == NULL)
    {
        ONLD_ERR_PRINT((TEXT("[ONLD:ERR]  (OPEN)Memory Alloc Error\r\n")));
        ONLD_DBG_PRINT((TEXT("[ONLD:OUT] --onld_open()\r\n")));
        return (ONLD_OPEN_FAILURE);
    }
        
    MEMSET(pstMEArg, 0x00, sizeof(LLDMEArg));
    pstMEArg->pstMEList  = (LLDMEList*)MALLOC(sizeof(LLDMEList) * XSR_MAX_MEBLK);
    if (pstMEArg->pstMEList == NULL)
    {
        ONLD_ERR_PRINT((TEXT("[ONLD:ERR]  (OPEN)Memory Alloc Error\r\n")));
        ONLD_DBG_PRINT((TEXT("[ONLD:OUT] --onld_open()\r\n")));
        return (ONLD_OPEN_FAILURE);
    }
    
    pstMEArg->nNumOfMList = (UINT16)0x00;
    pstMEArg->nBitMapErr  = (UINT16)0x00;
    pstMEArg->bFlag       = FALSE32;
#endif /* #if defined(DEFERRED_CHK) */

    pstPrevOpInfo[nDev]->nBufSel = DATA_BUF0;

    LLD_DBG_PRINT((TEXT("[ONLD:MSG]  nNumOfBlks   = %d\r\n"), astONLDDev[nDev].pstDevSpec->nNumOfBlks));
    LLD_DBG_PRINT((TEXT("[ONLD:MSG]  nNumOfPlanes = %d\r\n"), astONLDDev[nDev].pstDevSpec->nNumOfPlanes));
    LLD_DBG_PRINT((TEXT("[ONLD:MSG]  nBlksInRsv   = %d\r\n"), astONLDDev[nDev].pstDevSpec->nBlksInRsv));
    LLD_DBG_PRINT((TEXT("[ONLD:MSG]  nBadPos      = %d\r\n"), astONLDDev[nDev].pstDevSpec->nBadPos));
    LLD_DBG_PRINT((TEXT("[ONLD:MSG]  nLsnPos      = %d\r\n"), astONLDDev[nDev].pstDevSpec->nLsnPos));
    LLD_DBG_PRINT((TEXT("[ONLD:MSG]  nECCPos      = %d\r\n"), astONLDDev[nDev].pstDevSpec->nEccPos));
    LLD_DBG_PRINT((TEXT("[ONLD:MSG]  nBWidth      = %d\r\n"), astONLDDev[nDev].pstDevSpec->nBWidth));
    LLD_DBG_PRINT((TEXT("[ONLD:MSG]  nMEFlag      = %d\r\n"), astONLDDev[nDev].pstDevSpec->nMEFlag));
    LLD_DBG_PRINT((TEXT("[ONLD:MSG]  nLKFlag      = %d\r\n"), astONLDDev[nDev].pstDevSpec->nLKFlag));


#if defined(USE_CACHE_READ) && defined(USE_CACHE_AHEAD)
    /* Reset OneNAND */
    nBAddr  = GET_DEV_BADDR(nDev);
    ONLD_REG_START_ADDR2(nBAddr) = (UINT16)(0x0000);
    ONLD_REG_START_ADDR1(nBAddr) = (UINT16)(0x0000);

    if ((GET_DEV_DID(nDev) & 0x0008) == DDP_CHIP)
    {
        ONLD_REG_INT(nBAddr)            = (UINT16)INT_CLEAR;
        ONLD_REG_CMD(nBAddr)            = (UINT16)ONLD_CMD_RST_NF_CORE;
    
        /* Wait until device ready */
        WAIT_ONLD_INT_STAT(nBAddr, PEND_INT);

        ONLD_REG_START_ADDR2(nBAddr) = (UINT16)(0x8000);
        ONLD_REG_START_ADDR1(nBAddr) = (UINT16)(0x8000);
    } /*if DDP_CHIP */

    ONLD_REG_INT(nBAddr)            = (UINT16)INT_CLEAR;
    ONLD_REG_CMD(nBAddr)            = (UINT16)ONLD_CMD_RST_NF_CORE;

    /* Wait until device ready */
    WAIT_ONLD_INT_STAT(nBAddr, PEND_INT);
#endif /*#if defined(USE_CACHE_READ) && defined(USE_CACHE_AHEAD) */

    ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_open()\r\n")));
    
    return (ONLD_SUCCESS);    
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      onld_close                                                           */
/* DESCRIPTION                                                               */
/*      This function closes ONLD device driver                              */
/* PARAMETERS                                                                */
/*      nDev                                                                 */
/*          Physical Device Number (0 ~ 7)                                   */
/* RETURN VALUES                                                             */
/*      ONLD_SUCCESS                                                         */
/*          Close Success                                                    */
/*      ONLD_CLOSE_FAILURE                                                   */
/*          Close Failure                                                    */
/*      ONLD_WRITE_ERROR | ONLD_PREV_OP_RESULT                               */
/*          Previous Write Failure                                           */
/*      ONLD_ERASE_ERROR | ONLD_PREV_OP_RESULT                               */
/*          Previous Erase Failure                                           */
/*      ONLD_WR_PROTECT_ERROR | ONLD_PREV_OP_RESULT                          */
/*          Previous Operation Write Protect Error                           */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
INT32
onld_close(UINT32 nDev)
{
	UINT32  nBAddr;
	INT32   nRes = ONLD_SUCCESS;
    
    ONLD_DBG_PRINT((TEXT("[ONLD :  IN] ++onld_close() nDev = %d\r\n"), nDev));

    nBAddr = GET_DEV_BADDR(nDev);

    if ((nRes =onld_flushop(nDev)) != ONLD_SUCCESS)
    {
        return nRes;
    }
   
    if (pstPrevOpInfo[nDev] != (PrevOpInfo*)NULL)
    {      
#if defined(DEFERRED_CHK)
        FREE(pstPrevOpInfo[nDev]->pstPreMEArg->pstMEList);
        pstPrevOpInfo[nDev]->pstPreMEArg->pstMEList = (LLDMEList*)NULL;
        FREE(pstPrevOpInfo[nDev]->pstPreMEArg);   
        pstPrevOpInfo[nDev]->pstPreMEArg = (LLDMEArg*)NULL;
#endif /* #if defined(DEFERRED_CHK) */
        FREE(pstPrevOpInfo[nDev]);
        pstPrevOpInfo[nDev] = (PrevOpInfo*)NULL;
    }
    
    astONLDDev[nDev].bOpen      = FALSE32;
    astONLDDev[nDev].bPower     = FALSE32;
    astONLDDev[nDev].pstDevSpec = NULL;
    
    ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_close()\r\n")));

    return (ONLD_SUCCESS);
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      onld_read                                                            */
/* DESCRIPTION                                                               */
/*      This function read data from NAND flash                              */
/* PARAMETERS                                                                */
/*      nDev                                                                 */
/*          Physical Device Number (0 ~ 7)                                   */
/*      nPsn                                                                 */
/*          Sector index to be read                                          */
/*      nScts                                                                */
/*          The number of sectors to be read                                 */
/*      pMBuf                                                                */
/*          Memory buffer for main array of NAND flash                       */
/*      pSBuf                                                                */
/*          Memory buffer for spare array of NAND flash                      */
/*      nFlag                                                                */
/*          Operation options such as ECC_ON, OFF                            */
/* RETURN VALUES                                                             */
/*      ONLD_SUCCESS                                                         */
/*          Data Read success                                                */
/*      ONLD_ILLEGAL_ACCESS                                                  */
/*          Illegal Read Operation Try                                       */
/*      ONLD_WRITE_ERROR | ONLD_PREV_OP_RESULT                               */
/*          Previous Write Failure                                           */
/*          DCOP(Deferred Check Operation)                                   */
/*      ONLD_ERASE_ERROR | ONLD_PREV_OP_RESULT                               */
/*          Previous Erase Failure.                                          */
/*          DCOP(Deferred Check Operation)                                   */
/*      ONLD_WR_PROTECT_ERROR | ONLD_PREV_OP_RESULT                          */
/*          Previous Operation Write Protect Error                           */
/*          DCOP(Deferred Check Operation)                                   */
/*      ONLD_READ_ERROR | ECC result code                                    */
/*          Data intigrity fault. (1 or 2bit ECC error)                      */
/* NOTES                                                                     */
/*      Deferred Check Operation : The method of operation perfromance       */
/*          optimization. The DCOP method is to terminate the function (write*/
/*          or erase) right after command issue. Then it checks the result of*/
/*          operation at next function call.                                 */
/*                                                                           */
/*****************************************************************************/
INT32
onld_read(UINT32 nDev, UINT32 nPsn, UINT32 nScts, UINT8 *pMBuf, UINT8 *pSBuf, 
          UINT32 nFlag)
{
    UINT32  nCnt;
    UINT16  nBSA;
    UINT32 *pONLDMBuf;
    UINT32 *pONLDSBuf;
    UINT32  nBAddr;
    UINT16  nPbn;
    UINT16  nEccRes     = (UINT16)0x0;
    UINT16  nEccMask    = (UINT16)0x0;
    UINT16	aEccRes[8];
    BOOL32  bIsOneBitError  = FALSE32;
#if defined(USE_CACHE_READ) && defined(USE_CACHE_AHEAD)
    INT32   nRet;
    SGL     stSGL;
#endif /* #if defined(USE_CACHE_READ) && defined(USE_CACHE_AHEAD) */

#if defined(DEFERRED_CHK)
    INT32   nRes;
#endif /* #if defined(DEFERRED_CHK) */

    ONLD_DBG_PRINT((TEXT("[ONLD :  IN] ++onld_read() nDev = %d, nPsn = %d, nScts = %d, nFlag = 0x%x\r\n"), 
                    nDev, nPsn, nScts, nFlag));

    nBAddr = GET_DEV_BADDR(nDev);

    ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  nBAddr = 0x%08x\r\n"), nBAddr));


#ifdef CONFIG_RFS_TINYBML
	/* shared operation result */
    /* If previous operation is not read, save previous operation result */
	tiny_shared = 1;
	if(xsr_shared)
	{
		do 
		{
			while (GET_ONLD_INT_STAT(nBAddr, PEND_INT) != (UINT16)PEND_INT)
			{
				/* Wait until device ready */
			}

			if ((GET_ONLD_INT_STAT(nBAddr, PEND_ERASE) == (UINT16)PEND_ERASE) ||
				(GET_ONLD_INT_STAT(nBAddr, PEND_WRITE) == (UINT16)PEND_WRITE))
			{
				result_ctrl_status[nDev] = GET_ONLD_CTRL_STAT(nBAddr, 0xFFFF);
				break;
			}

			/* Set tinyBML operation block and tinyBML shared value reset */
			ONLD_REG_START_ADDR2(nBAddr) = (UINT16)MAKE_DBS(xsr_block_shared);

			if (ONLD_REG_CMD (nBAddr) == 0x000E)
			{
				ONLD_REG_INT (nBAddr) = (UINT16) INT_CLEAR;
				ONLD_REG_CMD (nBAddr) = (UINT16) ONLD_CMD_FINISH_CACHE_READ;

				while (GET_ONLD_INT_STAT(nBAddr, PEND_INT) != (UINT16)PEND_INT)
				{
					/* Wait until device ready */
				}
			}
		} while(0);
	}
#endif	/* CONFIG_RFS_TINYBML */


    if ((pMBuf == NULL) && (pSBuf == NULL))
    {
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (READ)Buffer Pointer Error\r\n")));
        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_read(()\r\n")));
        return (ONLD_ILLEGAL_ACCESS);
    }

    nPbn = GET_PBN(nDev, nPsn);

    ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  nPbn = %d\r\n"), nPbn));

#if defined(STRICT_CHK)
    if (_StrictChk(nDev, nPsn, nScts) != TRUE32)
    {
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (READ)Strict Check Error\r\n")));
        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_read()\r\n")));
        return (ONLD_ILLEGAL_ACCESS);
    }
#endif  /* #if defined(STRICT_CHK) */

#if defined(USE_CACHE_READ) && defined(USE_CACHE_AHEAD)

    /* Check if the device supports cache read cmd. */
    if (astONLDInfo[nDev].MRead != _mread)
    {
        stSGL.nElements = 1;
        stSGL.stSGLEntry[0].pBuf = pMBuf;
        stSGL.stSGLEntry[0].nSectors = nScts;
        stSGL.stSGLEntry[0].nFlag = SGL_ENTRY_USER_DATA;
        
        if (astONLDInfo[nDev].MRead != NULL)
        {
            nRet = astONLDInfo[nDev].MRead(nDev, nPsn, nScts, &stSGL, pSBuf, nFlag);
        }
        else
        {
            return (ONLD_ILLEGAL_ACCESS);
        }
        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_read()\r\n")));

        return nRet;
    }
#endif /* #if defined(USE_CACHE_READ) && defined(USE_CACHE_AHEAD) */


	if ( (nFlag & ONLD_FLAG_ECC_ON) != 0)
	{
		/* ECC Value Bit Mask Setting */
    	for (nCnt = 0; nCnt < nScts; nCnt++)
    	{
        	if (pMBuf != NULL)
        	{
            	nEccMask |= (0x0C << (nCnt * 4));
        	}
        	if (pSBuf != NULL)
        	{
            	nEccMask |= (0x03 << (nCnt * 4));
        	}
    	}
    }

#if defined(DEFERRED_CHK)
    if ((nRes = _ChkPrevOpResult(nDev)) != ONLD_SUCCESS)
    {
        pstPrevOpInfo[nDev]->ePreOp = NONE;
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (READ)Previous Operation Error 0x%08x\r\n"), 
                        nRes));
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (READ)Dev = %d, Psn = %d, Scts = %d"),
                        nDev,
                        pstPrevOpInfo[nDev]->nPsn, 
                        pstPrevOpInfo[nDev]->nScts));
        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_write()\r\n")));
        return nRes;
    }
#endif /* #if defined(DEFERRED_CHK) */

    ONLD_REG_START_ADDR2(nBAddr) = (UINT16)MAKE_DBS(nPbn);
    /* Block Number Set */
    ONLD_REG_START_ADDR1(nBAddr) = (UINT16)MAKE_FBA(nPbn);
    
    /* Sector Number Set */
    ONLD_REG_START_ADDR8(nBAddr) = (UINT16)(((nPsn << astONLDInfo[nDev].nFPASelSft) & MASK_FPA) | (nPsn & astONLDInfo[nDev].nFSAMask));
    
    nBSA = (UINT16)MAKE_BSA(nPsn, GET_NXT_BUF_SEL(nDev));

    /* Start Buffer Selection */
    ONLD_REG_START_BUF(nBAddr)   = (UINT16)((nBSA & MASK_BSA) | (nScts & MASK_BSC));

    if ( (nFlag & ONLD_FLAG_ECC_ON) != 0)
    {
        /* System Configuration Reg set (ECC On)*/
        ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  ECC ON\r\n")));
        ONLD_REG_SYS_CONF1(nBAddr) &= CONF1_ECC_ON;
    }
    else
    {
        /* System Configuration Reg set (ECC Off)*/
        ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  ECC OFF\r\n")));
        ONLD_REG_SYS_CONF1(nBAddr) |= CONF1_ECC_OFF;
    }

    /* INT Stat Reg Clear */
    ONLD_REG_INT(nBAddr)         = (UINT16)INT_CLEAR;
    
    /*-----------------------------------------------------------------------*/
    /* ONLD Read CMD is issued                                               */
    /*-----------------------------------------------------------------------*/
    if (pMBuf != NULL)
    {
        /* Page Read Command issue */
        ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  Read Page\r\n")));
        ONLD_REG_CMD(nBAddr)        = (UINT16)ONLD_CMD_READ_PAGE;
    }
    else
    {
        /* Spare Read Command issue */
        ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  Read Spare\r\n")));
        ONLD_REG_CMD(nBAddr)        = (UINT16)ONLD_CMD_READ_SPARE;
    }

    /* Wait until device ready */
    WAIT_ONLD_INT_STAT(nBAddr, PEND_READ)

    pONLDMBuf = (UINT32*)GET_ONLD_MBUF_ADDR(nBAddr, nPsn, GET_NXT_BUF_SEL(nDev));
    pONLDSBuf = (UINT32*)GET_ONLD_SBUF_ADDR(nBAddr, nPsn, GET_NXT_BUF_SEL(nDev));

    if (pMBuf != NULL)
    {
        /* Memcopy for main data */
        _ReadMain(pMBuf, pONLDMBuf, nScts);

    }
    if (pSBuf != NULL)
    {
        /* Memcopy for spare data */
        _ReadSpare(pSBuf, pONLDSBuf, nScts);

    }

    pstPrevOpInfo[nDev]->nBufSel    = GET_NXT_BUF_SEL(nDev);

    if ( (nFlag & ONLD_FLAG_ECC_ON) != 0 )
    {
        nEccRes = (ONLD_REG_ECC_STAT(nBAddr) & nEccMask);

        if (nEccRes != 0)
        {
            nEccMask = 0;

    		/* ECC Value Bit Mask Setting */
        	for (nCnt = 0; nCnt < nScts; nCnt++)
        	{
            	if (pMBuf != NULL)
            	{
                	nEccMask |= (0x08 << (nCnt * 4));
            	}
            	if (pSBuf != NULL)
            	{
                	nEccMask |= (0x02 << (nCnt * 4));
            	}
        	}
        	
        	if (nEccRes & nEccMask)
        	{
        	    /* 2bit ECC error occurs */
                ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  ECC Error Occurs during Read (Pbn : %d, Psn : %d)\r\n"), nPbn, nPsn));
                ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  |ECC Error Code    : 0x%04x |\r\n"), ONLD_REG_ECC_STAT(nBAddr)));
                ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  |ECC Error Addr M0 : 0x%04x |\r\n"), ONLD_REG_ECC_RSLT_MB0(nBAddr)));
                ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  |ECC Error Addr S0 : 0x%04x |\r\n"), ONLD_REG_ECC_RSLT_SB0(nBAddr)));
                ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  |ECC Error Addr M1 : 0x%04x |\r\n"), ONLD_REG_ECC_RSLT_MB1(nBAddr)));
                ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  |ECC Error Addr S1 : 0x%04x |\r\n"), ONLD_REG_ECC_RSLT_SB1(nBAddr)));
                ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  |ECC Error Addr M2 : 0x%04x |\r\n"), ONLD_REG_ECC_RSLT_MB2(nBAddr)));
                ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  |ECC Error Addr S2 : 0x%04x |\r\n"), ONLD_REG_ECC_RSLT_SB2(nBAddr)));
                ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  |ECC Error Addr M3 : 0x%04x |\r\n"), ONLD_REG_ECC_RSLT_MB3(nBAddr)));
                ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  |ECC Error Addr S3 : 0x%04x |\r\n"), ONLD_REG_ECC_RSLT_SB3(nBAddr)));
                ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  +---------------------------+\r\n")));
                ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_read()\r\n")));
    
                return (ONLD_READ_ERROR | nEccRes);
            }
            else
            {
                /* 1bit ECC error detect */
    		    MEMCPY16(aEccRes, (UINT16*)&ONLD_REG_ECC_RSLT_MB0(nBAddr), sizeof(UINT16) * nScts * 2);           		
        		bIsOneBitError = TRUE32;                
            }
            
        }
        
        if (bIsOneBitError)
        {
            if (_ChkReadDisturb(nScts, nEccRes, aEccRes, pMBuf, pSBuf)
                 == TRUE32)
            {
            	ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_read()\r\n")));
            	return (ONLD_READ_DISTURBANCE);
            }
        }
    
    }

#ifdef CONFIG_RFS_TINYBML
	tiny_block_shared = nPbn;
#endif

    ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_read()\r\n")));
    
    return (ONLD_SUCCESS);
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      onld_write                                                           */
/* DESCRIPTION                                                               */
/*      This function write data into NAND flash                             */
/* PARAMETERS                                                                */
/*      nDev                                                                 */
/*          Physical Device Number (0 ~ 7)                                   */
/*      nPsn                                                                 */
/*          Sector index to be written                                       */
/*      nScts                                                                */
/*          The number of sectors to be written                              */
/*      pMBuf                                                                */
/*          Memory buffer for main array of NAND flash                       */
/*      pSBuf                                                                */
/*          Memory buffer for spare array of NAND flash                      */
/*      nFlag                                                                */
/*          Operation options such as ECC_ON, OFF                            */
/* RETURN VALUES                                                             */
/*      ONLD_SUCCESS                                                         */
/*          Data write success                                               */
/*      ONLD_ILLEGAL_ACCESS                                                  */
/*          Illegal Write Operation Try                                      */
/*      ONLD_WR_PROTECT_ERROR                                                */
/*          Attempt to write at Locked Area                                  */
/*      ONLD_WRITE_ERROR | ONLD_PREV_OP_RESULT                               */
/*          Previous Write Failure                                           */
/*          DCOP(Deferred Check Operation)                                   */
/*      ONLD_ERASE_ERROR | ONLD_PREV_OP_RESULT                               */
/*          Previous Erase Failure.                                          */
/*          DCOP(Deferred Check Operation)                                   */
/*      ONLD_WR_PROTECT_ERROR | ONLD_PREV_OP_RESULT                          */
/*          Previous Operation Write Protect Error                           */
/*          DCOP(Deferred Check Operation)                                   */
/*      ONLD_WRITE_ERROR                                                     */
/*          Data write failure                                               */
/* NOTES                                                                     */
/*      Deferred Check Operation : The method of operation perfromance       */
/*          optimization. The DCOP method is to terminate the function (write*/
/*          or erase) right after command issue. Then it checks the result of*/
/*          operation at next function call.                                 */
/*                                                                           */
/*****************************************************************************/
INT32
onld_write(UINT32 nDev, UINT32 nPsn, UINT32 nScts, UINT8 *pMBuf, UINT8 *pSBuf, 
           UINT32 nFlag)
{
#if !defined(XSR_NBL2)
    UINT16  nBSA;
    UINT32 *pONLDMBuf, *pONLDSBuf;
    UINT32  nBAddr;
    UINT32  nPbn;
    
#if defined(DEFERRED_CHK)
    INT32   nRes;
#endif /* #if defined(DEFERRED_CHK) */

    ONLD_DBG_PRINT((TEXT("[ONLD :  IN] ++onld_write() nDev = %d, nPsn = %d, nScts = %d, nFlag = 0x%x\r\n"), 
                    nDev, nPsn, nScts, nFlag));

    nBAddr = GET_DEV_BADDR(nDev);

    ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  nBAddr = 0x%08x\r\n"), nBAddr));
    
    if ((pMBuf == NULL) && (pSBuf == NULL))
    {
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (WRITE)Buffer Pointer Error during Write\r\n")));
        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_write()\r\n")));
        return (ONLD_ILLEGAL_ACCESS);
    }

    nPbn = GET_PBN(nDev, nPsn);

    ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  nPbn = %d\r\n"), nPbn));

#if defined(STRICT_CHK)
    if (_StrictChk(nDev, nPsn, nScts) != TRUE32)
    {
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (WRITE)Strict Check Error during Write\r\n")));
        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_write()\r\n")));
        return (ONLD_ILLEGAL_ACCESS);
    }
#endif  /* #if defined(STRICT_CHK) */

#if defined(DEFERRED_CHK)
    /* While cache ahead is enabled, the finish cmd should precede */
    if ((GET_DEV_DID(nDev) & 0x0008) == DDP_CHIP || pstPrevOpInfo[nDev]->ePreOp == READ)
    {
        if ((nRes = _ChkPrevOpResult(nDev)) != ONLD_SUCCESS)
        {
            ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (WRITE)Previous Operation Error 0x%08x\r\n"), 
                            nRes));
            ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (WRITE)Dev = %d, Psn = %d, Scts = %d"),
                            nDev,
                            pstPrevOpInfo[nDev]->nPsn, 
                            pstPrevOpInfo[nDev]->nScts));
            ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_write()\r\n")));
            return nRes;
        }
    }
#endif /* #if defined(DEFERRED_CHK) */

    /* Device BufferRam Select */
    ONLD_REG_START_ADDR2(nBAddr) = (UINT16)MAKE_DBS(nPbn);

    pONLDMBuf = (UINT32*)GET_ONLD_MBUF_ADDR(nBAddr, nPsn, GET_NXT_BUF_SEL(nDev));
    pONLDSBuf = (UINT32*)GET_ONLD_SBUF_ADDR(nBAddr, nPsn, GET_NXT_BUF_SEL(nDev));

    if (pMBuf != NULL) 
    {
        /* Memcopy for main data */
        _WriteMain(pMBuf, pONLDMBuf, nScts);
    }
        
    if (pSBuf != NULL)
    {
        /* Memcopy for spare data */
        _WriteSpare(pSBuf, pONLDSBuf, nScts);
    }
    else
    {
        /* Memset for spare data 0xff*/
        _WriteSpare(aEmptySBuf, pONLDSBuf, nScts);
    }

#if defined(DEFERRED_CHK)
    if ((GET_DEV_DID(nDev) & 0x0008) != DDP_CHIP)
    {
        if ((nRes = _ChkPrevOpResult(nDev)) != ONLD_SUCCESS)
        {
            ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (WRITE)Previous Operation Error 0x%08x\r\n"), 
                            nRes));
            ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (WRITE)Dev = %d, Psn = %d, Scts = %d"),
                            nDev,
                            pstPrevOpInfo[nDev]->nPsn, 
                            pstPrevOpInfo[nDev]->nScts));
            ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_write()\r\n")));
            return nRes;
        }
    }
#endif /* #if defined(DEFERRED_CHK) */

    /* Block Number Set */
    ONLD_REG_START_ADDR1(nBAddr) = (UINT16)MAKE_FBA(nPbn);

    /* Sector Number Set */
    ONLD_REG_START_ADDR8(nBAddr) = (UINT16)(((nPsn << astONLDInfo[nDev].nFPASelSft) & MASK_FPA) | (nPsn & astONLDInfo[nDev].nFSAMask));

    nBSA = (UINT16)MAKE_BSA(nPsn, GET_NXT_BUF_SEL(nDev));

    ONLD_REG_START_BUF(nBAddr)   = (UINT16)((nBSA & MASK_BSA) | (nScts & MASK_BSC));

    if ( (nFlag & ONLD_FLAG_ECC_ON) != 0 )
    {
        /* System Configuration Reg set (ECC On)*/
        ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  ECC ON\r\n")));
        ONLD_REG_SYS_CONF1(nBAddr) &= CONF1_ECC_ON;
    }
    else
    {
        /* System Configuration Reg set (ECC Off)*/
        ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  ECC OFF\r\n")));
        ONLD_REG_SYS_CONF1(nBAddr) |= CONF1_ECC_OFF;
    }

#ifdef ASYNC_MODE
    /* in case of async mode, interrupt should be enabled */
    if ( (nFlag & ONLD_FLAG_ASYNC_OP) != 0 )
    {
    	PAM_ClearInt((UINT32)XSR_INT_ID_NAND_0);
    	PAM_EnableInt((UINT32)XSR_INT_ID_NAND_0);    	
    }
#endif //ASYNC_MODE


    /* INT Stat Reg Clear */
    ONLD_REG_INT(nBAddr)         = (UINT16)INT_CLEAR;
    
    /*-----------------------------------------------------------------------*/
    /* ONLD Write CMD is issued                                              */   
    /*-----------------------------------------------------------------------*/
    if (pMBuf != NULL)
    {
        /* Main Write Command issue */
        ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  Write Page\r\n")));
        ONLD_REG_CMD(nBAddr) = (UINT16)ONLD_CMD_WRITE_PAGE;
    }
    else
    {
        /* Spare Write Command issue */
        ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  Write Spare\r\n")));
        ONLD_REG_CMD(nBAddr) = (UINT16)ONLD_CMD_WRITE_SPARE;
    }

#if !defined(DEFERRED_CHK)
    WAIT_ONLD_INT_STAT_CHECKING_PROTECTION(nBAddr, PEND_INT);
      
    /* Write Operation Error Check */
    if (GET_ONLD_CTRL_STAT(nBAddr, ERROR_STATE) == ERROR_STATE)
    {
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (WRITE)Write Error\r\n")));
        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_write()\r\n")));

        return (ONLD_WRITE_ERROR);
    }

    ADD_DELAY(nBAddr, DUMMY_READ_CNT);

#endif /* #if !defined(DEFERRED_CHK) */
  
#if defined(DEFERRED_CHK)
    pstPrevOpInfo[nDev]->ePreOp     = WRITE;
    pstPrevOpInfo[nDev]->nPsn       = nPsn;
    pstPrevOpInfo[nDev]->nScts      = nScts;
    pstPrevOpInfo[nDev]->nFlag      = nFlag;
#endif /* #if defined(DEFERRED_CHK) */
    pstPrevOpInfo[nDev]->nBufSel    = GET_NXT_BUF_SEL(nDev);

    ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_write()\r\n")));
#endif /* #if !defined(XSR_NBL2) */
    return (ONLD_SUCCESS);
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      onld_erase                                                           */
/* DESCRIPTION                                                               */
/*      This function erases a block                                         */
/* PARAMETERS                                                                */
/*      nDev                                                                 */
/*          Physical Device Number (0 ~ 7)                                   */
/*      nPbn                                                                 */
/*          Physical Block Number                                            */
/*      nFlag                                                                */
/*          Operation Option (Sync/Async)                                    */
/* RETURN VALUES                                                             */
/*      ONLD_ILLEGAL_ACCESS                                                  */
/*          Illegal Erase Operation Try                                      */
/*      ONLD_WR_PROTECT_ERROR                                                */
/*          Attempt to erase at Locked Area                                  */
/*      ONLD_WRITE_ERROR | ONLD_PREV_OP_RESULT                               */
/*          Previous Write Failure                                           */
/*          DCOP(Deferred Check Operation)                                   */
/*      ONLD_ERASE_ERROR | ONLD_PREV_OP_RESULT                               */
/*          Previous Erase Failure.                                          */
/*          DCOP(Deferred Check Operation)                                   */
/*      ONLD_WR_PROTECT_ERROR | ONLD_PREV_OP_RESULT                          */
/*          Previous Operation Write Protect Error                           */
/*          DCOP(Deferred Check Operation)                                   */
/*      ONLD_WRITE_ERROROR                                                   */
/*          Data write failure                                               */
/*      ONLD_SUCCESS                                                         */
/*          Data write success                                               */
/* NOTES                                                                     */
/*      Deferred Check Operation : The method of operation perfromance       */
/*          optimization. The DCOP method is to terminate the function (write*/
/*          or erase) right after command issue. Then it checks the result of*/
/*          operation at next function call.                                 */
/*                                                                           */
/*****************************************************************************/
INT32
onld_erase(UINT32 nDev, UINT32 nPbn, UINT32 nFlag)
{
#if !defined(XSR_NBL2)
    UINT32  nBAddr;
#if defined(DEFERRED_CHK)
    UINT32   nRes;
#endif /* #if defined(DEFERRED_CHK) */

    ONLD_DBG_PRINT((TEXT("[ONLD :  IN] ++onld_erase() nDev = %d, nPbn = %d, nFlag = 0x%x\r\n"), 
                    nDev, nPbn, nFlag));

    nBAddr = GET_DEV_BADDR(nDev);

    ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  nBAddr = 0x%08x\r\n"), nBAddr));
    
#if defined(STRICT_CHK)
    /*  Check Device Number */
    if (nDev > MAX_SUPPORT_DEVS)
    {
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (ERASE)Invalid Device Number (nDev = %d)\r\n"), nDev));
        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_erase()\r\n")));

        return (ONLD_ILLEGAL_ACCESS);
    }

    /* Check Device Open Flag */
    if (CHK_DEV_OPEN_FLAG(nDev) == FALSE32)
    {
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (ERASE)Device is not opened (nPbn = %d)\r\n"), nPbn));
        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_erase()\r\n")));

        return (ONLD_ILLEGAL_ACCESS);
    }

    /* Check Device Power Flag */
    if (CHK_DEV_PWR_FLAG(nDev) == FALSE32)
    {
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (ERASE)Dev PwrFlg is not set(nPbn = %d)\r\n"), nPbn));
        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_erase()\r\n")));

        return (ONLD_ILLEGAL_ACCESS);
    }

    /* Check Block Out of Bound                      */
    /* if nPbn > GET_NUM_OF_BLKS then it is an error.*/
    if (CHK_BLK_OOB(nDev, nPbn))
    {
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (ERASE)Pbn is Out of Bound(nPbn = %d)\r\n"), nPbn));
        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_erase()\r\n")));

        return (ONLD_ILLEGAL_ACCESS);
    }
#endif  /* #if defined(STRICT_CHK) */

#if defined(DEFERRED_CHK)
    if ((nRes = _ChkPrevOpResult(nDev)) != ONLD_SUCCESS)
    {
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (ERASE)Previous Operation Error 0x%08x\r\n"), 
                        nRes));
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (ERASE)Dev = %d, Psn = %d, Scts = %d"),
                        nDev,
                        pstPrevOpInfo[nDev]->nPsn, 
                        pstPrevOpInfo[nDev]->nScts));
        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_erase()\r\n")));
        return nRes;
    }
#endif /* #if defined(DEFERRED_CHK) */

    ONLD_REG_START_ADDR2(nBAddr) = (UINT16)MAKE_DBS(nPbn);
    /* Block Number Set */
    ONLD_REG_START_ADDR1(nBAddr) = (UINT16)MAKE_FBA(nPbn);

#ifdef ASYNC_MODE
    /* in case of async mode, interrupt should be enabled */
    if ( (nFlag & ONLD_FLAG_ASYNC_OP) != 0 )
    {
    	PAM_ClearInt((UINT32)XSR_INT_ID_NAND_0);
    	PAM_EnableInt((UINT32)XSR_INT_ID_NAND_0);    	
    }
#endif

    /* INT Stat Reg Clear */
    ONLD_REG_INT(nBAddr)         = (UINT16)INT_CLEAR;
    
    /*-----------------------------------------------------------------------*/
    /* ONLD Erase CMD is issued                                              */
    /*-----------------------------------------------------------------------*/
    ONLD_REG_CMD(nBAddr)         = (UINT16)ONLD_CMD_ERASE_BLK;

#if !defined(DEFERRED_CHK)

    WAIT_ONLD_INT_STAT_CHECKING_PROTECTION(nBAddr, PEND_INT);
        
    /* Erase Operation Error Check */
    if (GET_ONLD_CTRL_STAT(nBAddr, ERROR_STATE) == ERROR_STATE)
    {
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (ERASE)Erase Error(nPbn = %d)\r\n"), nPbn));
        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_erase()\r\n")));

        return (ONLD_ERASE_ERROR);
    }

    ADD_DELAY(nBAddr, DUMMY_READ_CNT);

#endif /* #if !defined(DEFERRED_CHK) */

#if defined(DEFERRED_CHK)
    pstPrevOpInfo[nDev]->ePreOp     = ERASE;
    pstPrevOpInfo[nDev]->nPsn       = nPbn * GET_SCTS_PER_BLK(nDev);
    pstPrevOpInfo[nDev]->nFlag      = nFlag;
#endif /* #if defined(DEFERRED_CHK) */

    ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_erase()\r\n")));
#endif /* #if !defined(XSR_NBL2) */

    return (ONLD_SUCCESS);
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      onld_copyback                                                        */
/* DESCRIPTION                                                               */
/*      This function read data from sector array of NAND flash              */
/* PARAMETERS                                                                */
/*      nSrcBlkNum                                                           */
/*          Source block index to be read                                    */
/*      nSrcPgNum                                                            */
/*          Source page index to be read                                     */
/*      nDestBlkNum                                                          */
/*          Destination block index to be written                            */
/*      nDestPgNum                                                           */
/*          Destination page index to be written                             */
/*      nCmdRdType                                                           */
/*          Read command type                                                */
/*      iCmdWrType                                                           */
/*          Write command type                                               */
/* RETURN VALUES                                                             */
/*      ONLD_SUCCESS                                                         */
/*          Data Read success                                                */
/*      ONLD_ILLEGAL_ACCESS                                                  */
/*          Illegal Read Operation Try                                       */
/*      ONLD_READ_ERROR | ECC result code                                    */
/*          Data intigrity fault. (1 or 2bit ECC error)                      */
/*      ONLD_WRITE_ERROR | ONLD_PREV_OP_RESULT                               */
/*          Previous Write Failure                                           */
/*          DCOP(Deferred Check Operation)                                   */
/*      ONLD_ERASE_ERROR | ONLD_PREV_OP_RESULT                               */
/*          Previous Erase Failure.                                          */
/*          DCOP(Deferred Check Operation)                                   */
/*      ONLD_WR_PROTECT_ERROR | ONLD_PREV_OP_RESULT                          */
/*          Previous Operation Write Protect Error                           */
/*          DCOP(Deferred Check Operation)                                   */
/*      ONLD_WR_PROTECT_ERROR                                                */
/*          Attempt to write at Locked Area                                  */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
INT32 
onld_copyback(UINT32 nDev, CpBkArg *pstCpArg, UINT32 nFlag)
{
#if !defined(XSR_NBL2)
    UINT8               aBABuf[2];
    UINT8              *pRIBuf;
    INT32               nSctNum, nOffset;
    UINT16              nBSA;
    UINT16              nEccRes;
    UINT16              nPbn;
    volatile UINT16    *pDevBuf;
    UINT32              nCnt;
    UINT32             *pONLDMBuf, *pONLDSBuf;
    UINT32              nBAddr;
    UINT32              nRISize;

#if defined(XSR_BIG_ENDIAN)
    volatile UINT16    *pAddr16;
    UINT16              nVal16;
#endif

#if defined(DEFERRED_CHK)
    INT32       nRes;
#endif /* #if defined(DEFERRED_CHK) */
    
    RndInArg   *pstRIArg;

    ONLD_DBG_PRINT((TEXT("[ONLD :  IN] ++onld_copyback() nDev = %d, nFlag = 0x%x\r\n"), 
                    nDev, nFlag));

    nBAddr = GET_DEV_BADDR(nDev);

    ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  nBAddr = 0x%08x\r\n"), nBAddr));

#if defined(STRICT_CHK)
    if (_StrictChk(nDev, pstCpArg->nSrcSn, GET_SCTS_PER_PG(nDev)) != TRUE32)
    {
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (CPBACK)Strict Check Error(nSrcSn)\r\n")));
        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_copyback()\r\n")));

        return (ONLD_ILLEGAL_ACCESS);
    }

    if (_StrictChk(nDev, pstCpArg->nDstSn, GET_SCTS_PER_PG(nDev)) != TRUE32)
    {
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (CPBACK)Strict Check Error(nDstSn)\r\n")));
        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_copyback()\r\n")));

        return (ONLD_ILLEGAL_ACCESS);
    }
    if (pstCpArg == NULL)
    {
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (CPBACK)Illeagal Access\r\n")));
        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_copyback()\r\n")));
        return(ONLD_ILLEGAL_ACCESS);
    }
#endif  /* #if defined(STRICT_CHK) */

#if defined(DEFERRED_CHK)
    if ((nRes = _ChkPrevOpResult(nDev)) != ONLD_SUCCESS)
    {
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (CPBACK)Previous Operation Error 0x%08x\r\n"), 
                        nRes));
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (CPBACK)Dev = %d, Psn = %d, Scts = %d"),
                        nDev,
                        pstPrevOpInfo[nDev]->nPsn, 
                        pstPrevOpInfo[nDev]->nScts));
        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_write()\r\n")));
        return nRes;
    }
#endif /* #if defined(DEFERRED_CHK) */

    ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  From SrcPsn = %d to DstPsn = %d\r\n"),
                    pstCpArg->nSrcSn, pstCpArg->nDstSn));

    /*-----------------------------------------------------------------------*/
    /* Step 1. READ                                                          */
    /*                                                                       */
    /*  a. Read command will be issued                                       */
    /*  b. Data will be loaded into OneNAND internal Buffer from NAND Cell   */
    /*  c. No memory transfer operation between Host memory and OneNAND      */
    /*     internal buffer                                                   */
    /*  d. ECC Read operation is possible                                    */
    /*-----------------------------------------------------------------------*/

    ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  +------------------------+\r\n"), nBAddr));
    ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  | Step1. ReadOp Start    |\r\n"), nBAddr));
    ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  +------------------------+\r\n"), nBAddr));

    nPbn = GET_PBN(nDev, pstCpArg->nSrcSn);

    ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  Source nPbn = %d\r\n"), nPbn));

    /* Device BufferRam Select */
    ONLD_REG_START_ADDR2(nBAddr) = (UINT16)MAKE_DBS(nPbn);
    /* Block Number Set */
    ONLD_REG_START_ADDR1(nBAddr) = (UINT16)MAKE_FBA(nPbn);

    /* Sector Number Set */
    ONLD_REG_START_ADDR8(nBAddr) = (UINT16)(((pstCpArg->nSrcSn << astONLDInfo[nDev].nFPASelSft) & MASK_FPA) | (pstCpArg->nSrcSn & astONLDInfo[nDev].nFSAMask));
    
    nBSA = (UINT16)MAKE_BSA(0, DATA_BUF0);
    
    ONLD_REG_START_BUF(nBAddr)   = (UINT16)((nBSA & MASK_BSA) | (GET_SCTS_PER_PG(nDev) & MASK_BSC));

    pONLDMBuf = (UINT32*)GET_ONLD_MBUF_ADDR(nBAddr, 0, DATA_BUF0);
    pONLDSBuf = (UINT32*)GET_ONLD_SBUF_ADDR(nBAddr, 0, DATA_BUF0);

    if ( (nFlag & ONLD_FLAG_ECC_ON) != 0 )
    {
        /* System Configuration Reg set (ECC On)*/
        ONLD_REG_SYS_CONF1(nBAddr) &= CONF1_ECC_ON;
    }
    else
    {
        /* System Configuration Reg set (ECC Off)*/
        ONLD_REG_SYS_CONF1(nBAddr) |= CONF1_ECC_OFF;
    }

    /* INT Stat Reg Clear */
    ONLD_REG_INT(nBAddr)         = (UINT16)INT_CLEAR;
    
    /*-----------------------------------------------------------------------*/
    /* ONLD Read CMD is issued                                               */   
    /*-----------------------------------------------------------------------*/
    /* Page Read Command issue */
    ONLD_REG_CMD(nBAddr) = (UINT16)ONLD_CMD_READ_PAGE;

    WAIT_ONLD_INT_STAT(nBAddr, PEND_READ);
    
    /*-----------------------------------------------------------------------*/
    /* Note                                                                  */
    /* If ECC 1bit error occurs, there is no an error return                 */
    /* Only 2bit ECC error case, there is an error return                    */
    /*-----------------------------------------------------------------------*/
    if ( (nFlag & ONLD_FLAG_ECC_ON) != 0 )
    {
        nEccRes = ONLD_REG_ECC_STAT(nBAddr);
        if ( (nEccRes & ONLD_READ_UERROR_A) != 0 )
        {
            AD_Debug("ECC Error Occurs during Read (Pbn : %d, Psn : %d)\r\n", nPbn, pstCpArg->nSrcSn);
            ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  (CPBACK)ECC Error Occurs during CPBack (Pbn : %d, Psn : %d)\r\n"), nPbn, pstCpArg->nSrcSn));
            ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  (CPBACK)+----------M3S3M2S2M1S1M0S0-+\r\n")));
            ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  (CPBACK)|ECC Error Code    : 0x04%x |\r\n"), ONLD_REG_ECC_STAT(nBAddr)));
            ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  (CPBACK)+---------------------------+\r\n")));
            ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_copyback()\r\n")));
            return (ONLD_READ_ERROR | nEccRes);
        }
    }

    ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  +------------------------+\r\n"), nBAddr));
    ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  | Step1. ReadOp Finish   |\r\n"), nBAddr));
    ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  +------------------------+\r\n"), nBAddr));

    /*-----------------------------------------------------------------------*/
    /* Step 2. DATA RANDOM IN                                                */
    /*                                                                       */
    /*  a. Data is loaded into OneNAND internal buffer from Host memory      */
    /*-----------------------------------------------------------------------*/

    ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  +------------------------+\r\n"), nBAddr));
    ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  | Step2. RandomIn Start  |\r\n"), nBAddr));
    ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  +------------------------+\r\n"), nBAddr));
    
    pstRIArg = pstCpArg->pstRndInArg;

    for (nCnt = 0 ; nCnt < pstCpArg->nRndInCnt ; nCnt++)
    {
        nSctNum = (pstRIArg[nCnt].nOffset / 1024);
        nOffset = (pstRIArg[nCnt].nOffset % 1024);    
        pRIBuf  = pstRIArg[nCnt].pBuf;
        nRISize = pstRIArg[nCnt].nNumOfBytes;

        if (nOffset < ONLD_MAIN_SIZE)
        {
            /*--------------------------------------------------------------*/
            /* Byte Align Problem                                           */
            /*                                                              */
            /* OneNand is a 16-bit BusWidth Device.                         */
            /* Therefore odd byte operation causes problem.                 */
            /*--------------------------------------------------------------*/
            if ((nOffset % sizeof(UINT16)) != 0)
            {
                pDevBuf = (volatile UINT16*)((UINT32)pONLDMBuf + 
                                             (nSctNum * ONLD_MAIN_SIZE) + 
                                             nOffset - 1);

                *(volatile UINT16*)aBABuf = *pDevBuf;
                aBABuf[1] = *pRIBuf++;
                *pDevBuf = *(volatile UINT16*)aBABuf;

                if (--nRISize <= 0)
                {
                    continue;
                }
                nOffset++;
            }
            
            if ((nRISize % sizeof(UINT16)) != 0)
            {
                pDevBuf = (volatile UINT16*)((UINT32)pONLDMBuf + 
                                             (nSctNum * ONLD_MAIN_SIZE) + 
                                             nOffset + nRISize - 1);

                *(volatile UINT16*)aBABuf = *pDevBuf;
                aBABuf[0] = *(pRIBuf + nRISize - 1);
                *pDevBuf = *(volatile UINT16*)aBABuf;

                if (--nRISize <= 0)
                {
                    continue;
                }
            }

            MEMCPY((UINT8*)((UINT32)pONLDMBuf + (nSctNum * ONLD_MAIN_SIZE) + nOffset), 
                   pRIBuf, 
                   nRISize);
        }
        else
        {
            /*--------------------------------------------------------------*/
            /* Byte Align Problem                                           */
            /*                                                              */
            /* OneNand is a 16-bit BusWidth Device.                         */
            /* Therefore odd byte operation causes problem.                 */
            /*--------------------------------------------------------------*/
            if ((nOffset % sizeof(UINT16)) != 0)
            {
                pDevBuf = (volatile UINT16*)((UINT32)pONLDSBuf + 
                                             (nSctNum * ONLD_SPARE_SIZE) + 
                                             (nOffset - ONLD_MAIN_SIZE) - 1);

                *(volatile UINT16*)aBABuf = *pDevBuf;
#if defined(XSR_BIG_ENDIAN)
                if (nOffset % ONLD_MAIN_SIZE == 5)
                {
                    aBABuf[0] = *pRIBuf++;
                }
#else               
                aBABuf[1] = *pRIBuf++;
#endif
                *pDevBuf = *(volatile UINT16*)aBABuf;
                if (--nRISize <= 0)
                {
                    continue;
                }
                nOffset++;
            }
            
            if ((nRISize % sizeof(UINT16)) != 0)
            {
                pDevBuf = (volatile UINT16*)((UINT32)pONLDSBuf + 
                                             (nSctNum * ONLD_SPARE_SIZE) + 
                                             (nOffset - ONLD_MAIN_SIZE) + 
                                             nRISize - 1);

                *(volatile UINT16*)aBABuf = *pDevBuf;
#if defined(XSR_BIG_ENDIAN)
                if ((nOffset + nRISize - 1)% ONLD_MAIN_SIZE == 4)
                {
                    aBABuf[0] = *(pRIBuf + nRISize - 1);
                }
#else   
                aBABuf[0] = *(pRIBuf + nRISize - 1);
#endif
                *pDevBuf = *(volatile UINT16*)aBABuf;

                if (--nRISize <= 0)
                {
                    continue;
                }
            }

#if defined(XSR_BIG_ENDIAN)
            pAddr16 = (volatile UINT16*)((UINT32)pONLDSBuf + 4 + (nSctNum * ONLD_SPARE_SIZE));
            nVal16  = *pAddr16;
           *pAddr16 = (nVal16 >> 8) | (nVal16 << 8);
            MEMCPY((UINT8*)((UINT32)pAddr16 + (nOffset - ONLD_MAIN_SIZE)), pRIBuf, nRISize);
            nVal16  = *pAddr16;
           *pAddr16 = (nVal16 >> 8) | (nVal16 << 8);
#else
            MEMCPY((UINT8*)((UINT32)pONLDSBuf + (nSctNum * ONLD_SPARE_SIZE) + (nOffset - ONLD_MAIN_SIZE)), 
                   pRIBuf, 
                   nRISize);
#endif
        }
    }

    ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  +------------------------+\r\n"), nBAddr));
    ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  | Step2. RandomIn Finish |\r\n"), nBAddr));
    ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  +------------------------+\r\n"), nBAddr));

    /*-----------------------------------------------------------------------*/
    /* Step 3. WRITE                                                         */
    /*                                                                       */
    /*  a. No memory transfer operation between Host memory and OneNAND      */
    /*     internal buffer                                                   */
    /*  b. Write command will be issued                                      */
    /*  c. ECC Read operation is possible                                    */
    /*-----------------------------------------------------------------------*/

    ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  +------------------------+\r\n"), nBAddr));
    ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  | Step3. WriteOp Start   |\r\n"), nBAddr));
    ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  +------------------------+\r\n"), nBAddr));

    nPbn = GET_PBN(nDev, pstCpArg->nDstSn);

    ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  Destination nPbn = %d\r\n"), nPbn));

    ONLD_REG_START_ADDR2(nBAddr) = (UINT16)MAKE_DBS(nPbn);
    /* Block Number Set */
    ONLD_REG_START_ADDR1(nBAddr) = (UINT16)MAKE_FBA(nPbn);
    /* Sector Number Set */
    ONLD_REG_START_ADDR8(nBAddr) = (UINT16)(((pstCpArg->nDstSn << astONLDInfo[nDev].nFPASelSft) & MASK_FPA) | (pstCpArg->nDstSn & astONLDInfo[nDev].nFSAMask));

    nBSA = (UINT16)MAKE_BSA(pstCpArg->nDstSn, DATA_BUF0);

    ONLD_REG_START_BUF(nBAddr)   = (UINT16)((nBSA & MASK_BSA) | (GET_SCTS_PER_PG(nDev) & MASK_BSC));

    if ( (nFlag & ONLD_FLAG_ECC_ON) != 0 )
    {
        /* System Configuration Reg set (ECC On)*/
        ONLD_REG_SYS_CONF1(nBAddr) &= CONF1_ECC_ON;
    }
    else
    {
        /* System Configuration Reg set (ECC Off)*/
        ONLD_REG_SYS_CONF1(nBAddr) |= CONF1_ECC_OFF;
    }

#ifdef ASYNC_MODE    
    /* in case async mode, interrupt should be enabled */
    if ( (nFlag & ONLD_FLAG_ASYNC_OP) != 0 )
    {
    	PAM_ClearInt((UINT32)XSR_INT_ID_NAND_0);
    	PAM_EnableInt((UINT32)XSR_INT_ID_NAND_0);    	
    }
#endif
    ONLD_REG_INT(nBAddr)         = (UINT16)INT_CLEAR;
    
    /*-----------------------------------------------------------------------*/
    /* ONLD Write CMD is issued                                              */   
    /*-----------------------------------------------------------------------*/

    /* Main Write Command issue */
    ONLD_REG_CMD(nBAddr) = (UINT16)ONLD_CMD_WRITE_PAGE;

#if !defined(DEFERRED_CHK)
    WAIT_ONLD_INT_STAT_CHECKING_PROTECTION(nBAddr, PEND_INT);
        
    /* Write Operation Error Check */
    if (GET_ONLD_CTRL_STAT(nBAddr, ERROR_STATE) == ERROR_STATE)
    {
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (CPBACK)Write Error\r\n")));
        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_copyback()\r\n")));

        return (ONLD_WRITE_ERROR);
    }
    
    ADD_DELAY(nBAddr, DUMMY_READ_CNT);
#endif

#if defined(DEFERRED_CHK)
    pstPrevOpInfo[nDev]->ePreOp     = CPBACK;
    pstPrevOpInfo[nDev]->nPsn       = pstCpArg->nDstSn;
    pstPrevOpInfo[nDev]->nScts      = GET_SCTS_PER_PG(nDev);
    pstPrevOpInfo[nDev]->nFlag      = nFlag;
#endif /* #if defined(DEFERRED_CHK) */

    pstPrevOpInfo[nDev]->nBufSel    = DATA_BUF0;

    ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_copyback()\r\n")));
#endif /* #if !defined(XSR_NBL2) */
    return (ONLD_SUCCESS);
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      onld_chkinitbadblk                                                   */
/* DESCRIPTION                                                               */
/*      This function checks a block if it is an initial bad block           */
/* PARAMETERS                                                                */
/*      nDev                                                                 */
/*          Physical Device Number (0 ~ 7)                                   */
/*      nPbn                                                                 */
/*          Physical Block Number                                            */
/* RETURN VALUES                                                             */
/*      ONLD_ILLEGAL_ACCESS                                                  */
/*          Illegal Erase Operation Try                                      */
/*      ONLD_WRITE_ERROR | ONLD_PREV_OP_RESULT                               */
/*          Previous Write Failure                                           */
/*          DCOP(Deferred Check Operation)                                   */
/*      ONLD_ERASE_ERROR | ONLD_PREV_OP_RESULT                               */
/*          Previous Erase Failure.                                          */
/*          DCOP(Deferred Check Operation)                                   */
/*      ONLD_WR_PROTECT_ERROR | ONLD_PREV_OP_RESULT                          */
/*          Previous Operation Write Protect Error                           */
/*          DCOP(Deferred Check Operation)                                   */
/*      ONLD_INIT_BADBLOCK                                                   */
/*          In case of an Initial Bad Block                                  */
/*      ONLD_INIT_GOODBLOCK                                                  */
/*          In case of a Good Block                                          */
/* NOTES                                                                     */
/*      Deferred Check Operation : The method of operation perfromance       */
/*          optimization. The DCOP method is to terminate the function (write*/
/*          or erase) right after command issue. Then it checks the result of*/
/*          operation at next function call.                                 */
/*                                                                           */
/*****************************************************************************/
INT32
onld_chkinitbadblk(UINT32 nDev, UINT32 nPbn)
{
    INT32   nRet;
    UINT16  aSpare[ONLD_SPARE_SIZE / 2]={0};
    UINT32  nPsn;
    UINT32  nBAddr;

    ONLD_DBG_PRINT((TEXT("[ONLD :  IN] ++onld_chkinitbadblk() nDev = %d, nPbn = %d\r\n"), 
                    nDev, nPbn));

    nBAddr = GET_DEV_BADDR(nDev);

    ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  nBAddr = 0x%08x\r\n"), nBAddr));
    
    nPsn = nPbn * GET_SCTS_PER_BLK(nDev);
    
    nRet = onld_read(nDev,                      /* Device Number                 */
                     nPsn,                      /* Physical Sector Number        */
                     (UINT32)1,                 /* Number of Sectors to be read  */
                     (UINT8*)NULL,              /* Buffer pointer for Main area  */
                     (UINT8*)aSpare,            /* Buffer pointer for Spare area */ 
                     (UINT32)ONLD_FLAG_ECC_OFF);/* flag                          */

    if ((nRet != ONLD_SUCCESS) && ((nRet & (0xFFFF0000)) != (UINT32)ONLD_READ_ERROR))
    {
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (CHK_INIT_BB)Previous Operation Error\r\n")));
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (CHK_INIT_BB)Return Val = 0x%08x\r\n"), nRet));
        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_chkinitbadblk()\r\n")));

        return (nRet);
    }

    if (aSpare[0] != (UINT16)VALID_BLK_MARK)
    {
        ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  nPbn = %d is an Initial BadBlk\r\n"), nPbn));
        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_chkinitbadblk()\r\n")));

        return (ONLD_INIT_BADBLOCK);
    }
    
    ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  nPbn = %d is a GoodBlk\r\n"), nPbn));
    ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_chkinitbadblk()\r\n")));

    return (ONLD_INIT_GOODBLOCK);
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      onld_setrwarea                                                       */
/* DESCRIPTION                                                               */
/*      This function sets RW Area                                           */
/* PARAMETERS                                                                */
/*      nDev                                                                 */
/*          Physical Device Number (0 ~ 7)                                   */
/*      nSUbn                                                                */
/*          Start block index of unlocked area                               */
/*      nUBlks                                                               */
/*          Total blocks of unlocked area                                    */
/*          If nNumOfUBs = 0, device is locked.                              */
/* RETURN VALUES                                                             */
/*      ONLD_ILLEGAL_ACCESS                                                  */
/*          In case of illegal access                                        */
/*      ONLD_SUCCESS                                                         */
/*          Lock/Unlock Operation success                                    */
/*      ONLD_WRITE_ERROR | ONLD_PREV_OP_RESULT                               */
/*          Previous Write Failure                                           */
/*          DCOP(Deferred Check Operation)                                   */
/*      ONLD_ERASE_ERROR | ONLD_PREV_OP_RESULT                               */
/*          Previous Erase Failure.                                          */
/*          DCOP(Deferred Check Operation)                                   */
/*      ONLD_WR_PROTECT_ERROR | ONLD_PREV_OP_RESULT                          */
/*          Previous Operation Write Protect Error                           */
/*          DCOP(Deferred Check Operation)                                   */
/* NOTES                                                                     */
/*      Deferred Check Operation : The method of operation perfromance       */
/*          optimization. The DCOP method is to terminate the function (write*/
/*          or erase) right after command issue. Then it checks the result of*/
/*          operation at next function call.                                 */
/*                                                                           */
/*****************************************************************************/
INT32 
onld_setrwarea(UINT32 nDev, UINT32 nSUbn, UINT32 nUBlks)
{
#if !defined(XSR_NBL2)
	
	#if defined(DEFERRED_CHK)
	    INT32   nRes;
	#endif /* #if defined(DEFERRED_CHK) */
	
	    ONLD_DBG_PRINT((TEXT("[ONLD :  IN] ++onld_setrwarea() nDev = %d, nSUbn = %d, nUBlks = 0x%x\r\n"), 
	                    nDev, nSUbn, nUBlks));
	
	#if defined(STRICT_CHK)
	    /*  Check Device Number */
	    if (nDev > MAX_SUPPORT_DEVS)
	    {
	        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (SET_RW)Invalid Device Number (nDev = %d)\r\n"), nDev));
	        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_setrwarea()\r\n")));
	
	        return (ONLD_ILLEGAL_ACCESS);
	    }
	
	    /* Check Device Open Flag */
	    if (CHK_DEV_OPEN_FLAG(nDev) == FALSE32)
	    {
	        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (SET_RW)Device is not opened\r\n")));
	        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_setrwarea()\r\n")));
	
	        return (ONLD_ILLEGAL_ACCESS);
	    }
	
	    /* Check Device Power Flag */
	    if (CHK_DEV_PWR_FLAG(nDev) == FALSE32)
	    {
	        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (SET_RW)Dev PwrFlg is not set\r\n")));
	        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_setrwarea()\r\n")));
	
	        return (ONLD_ILLEGAL_ACCESS);
	    }
	
	    /* Check Block Out of Bound                       */
	    /* if nSUbn > GET_NUM_OF_BLKS then it is an error.*/
	    if (CHK_BLK_OOB(nDev, nSUbn))
	    {
	        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (SET_RW)nSUbn = %d is Out of Bound\r\n"), nSUbn));
	        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_setrwarea()\r\n")));
	
	        return (ONLD_ILLEGAL_ACCESS);
	    }
	
	    /* Check Block Out of Bound                                  */
	    /* if (nSUbn + nUBlks) > GET_NUM_OF_BLKS then it is an error.*/
	    if (CHK_BLK_OOB(nDev, (nSUbn + nUBlks)))
	    {
	        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (SET_RW)(nSUbn + nUBlks - 1) = %d is Out of Bound\r\n"), 
	                        nSUbn + nUBlks - 1));
	        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_setrwarea()\r\n")));
	
	        return (ONLD_ILLEGAL_ACCESS);
	    }
	#endif  /* #if defined(STRICT_CHK) */
	
	#if defined(DEFERRED_CHK)
	    if ((nRes = _ChkPrevOpResult(nDev)) != ONLD_SUCCESS)
	    {
	        pstPrevOpInfo[nDev]->ePreOp = NONE;
	        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (SET_RW)Previous Operation Error 0x%08x\r\n"), 
	                        nRes));
	        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (SET_RW)Dev = %d, Psn = %d, Scts = %d"),
	                        nDev,
	                        pstPrevOpInfo[nDev]->nPsn, 
	                        pstPrevOpInfo[nDev]->nScts));
	        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_write()\r\n")));
	        return nRes;
	    }
	#endif /* #if defined(DEFERRED_CHK) */	

    if (astONLDInfo[nDev].SetRWArea != NULL)
    {
        return astONLDInfo[nDev].SetRWArea(nDev, nSUbn, nUBlks);
    }
    else
    {
        return (ONLD_ILLEGAL_ACCESS);
    }
    
#endif /* #if !defined(XSR_NBL2) */	
	
    return (ONLD_SUCCESS);   
}	
		
/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      onld_getprevopdata                                                   */
/* DESCRIPTION                                                               */
/*      This function copys data of previous write operation into a given    */
/*      buffer                                                               */
/* PARAMETERS                                                                */
/*      nDev                                                                 */
/*          Physical Device Number (0 ~ 7)                                   */
/*      pMBuf                                                                */
/*          Memory buffer for main array of NAND flash                       */
/*      pSBuf                                                                */
/*          Memory buffer for spare array of NAND flash                      */
/* RETURN VALUES                                                             */
/*      ONLD_SUCCESS                                                         */
/*          Previous Write Data copy success                                 */
/*      ONLD_ILLEGAL_ACCESS                                                  */
/*          In case of illegal access                                        */
/* NOTES                                                                     */
/*      This function can be called for the block management purpose         */
/*      under previous write error case.                                     */
/*                                                                           */
/*****************************************************************************/
INT32 
onld_getprevopdata(UINT32 nDev, UINT8 *pMBuf, UINT8 *pSBuf)
{
#if !defined(XSR_NBL2)
	UINT32  nBAddr;
	UINT32  nPbn;

#if defined(DEFERRED_CHK)
    UINT32 *pONLDMBuf, *pONLDSBuf;
#endif /* #if defined(DEFERRED_CHK) */
	
    ONLD_DBG_PRINT((TEXT("[ONLD :  IN] ++onld_getprevopdata() nDev = %d\r\n"), 
                    nDev));

    nBAddr = GET_DEV_BADDR(nDev);

#if defined(STRICT_CHK)
    /*  Check Device Number */
    if (nDev > MAX_SUPPORT_DEVS)
    {
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (GET_PREV_DAT)Invalid Device Number (nDev = %d)\r\n"), 
                        nDev));
        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_getprevopdata()\r\n")));

        return (ONLD_ILLEGAL_ACCESS);
    }

    /* Check Device Open Flag */
    if (CHK_DEV_OPEN_FLAG(nDev) == FALSE32)
    {
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (GET_PREV_DAT)Device is not opened\r\n")));
        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_getprevopdata()\r\n")));
        return (ONLD_ILLEGAL_ACCESS);
    }

    /* Check Device Power Flag */
    if (CHK_DEV_PWR_FLAG(nDev) == FALSE32)
    {
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (GET_PREV_DAT)Dev PwrFlg is not set\r\n")));
        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_getprevopdata()\r\n")));
        return (ONLD_ILLEGAL_ACCESS);
    }
#endif /* #if defined(STRICT_CHK) */

    if ((pMBuf == NULL) && (pSBuf == NULL))
    {
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (GET_PREV_DAT)Buffer Pointer Error during GetPrevOpData\r\n")));
        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_getprevopdata()\r\n")));
        return (ONLD_ILLEGAL_ACCESS);
    }


#if defined(DEFERRED_CHK)

    /* onld_getprevopdata does not return previous operation error
       because physical operation must be flushed before entering 
       this function. 
       Before calling onld_getprevopdata, actually, the write operation
       was called always */
    nPbn = GET_PBN(nDev, pstPrevOpInfo[nDev]->nPsn);
    ONLD_REG_START_ADDR2(nBAddr) = (UINT16)MAKE_DBS(nPbn);

    pONLDMBuf = (UINT32*)GET_ONLD_MBUF_ADDR(nBAddr, pstPrevOpInfo[nDev]->nPsn, 
                                          GET_CUR_BUF_SEL(nDev));
    pONLDSBuf = (UINT32*)GET_ONLD_SBUF_ADDR(nBAddr, pstPrevOpInfo[nDev]->nPsn, 
                                          GET_CUR_BUF_SEL(nDev));

    /*-------------------------------------------------------------------*/
    /* Data is loaded into Memory                                        */
    /*-------------------------------------------------------------------*/
    if (pMBuf != NULL)
    {
        /* Memcopy for main data */
        _ReadMain(pMBuf, pONLDMBuf, pstPrevOpInfo[nDev]->nScts);
    }
    if (pSBuf != NULL)
    {
        /* Memcopy for spare data */
        _ReadSpare(pSBuf, pONLDSBuf, pstPrevOpInfo[nDev]->nScts);
    }
#endif /* #if defined(DEFERRED_CHK) */

    ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_getprevopdata()\r\n")));
#endif /* #if !defined(XSR_NBL2) */
    return (ONLD_SUCCESS);
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      onld_ioctl                                                           */
/* DESCRIPTION                                                               */
/*      This function copys data of previous write operation into a given    */
/*      buffer                                                               */
/* PARAMETERS                                                                */
/*      nDev                                                                 */
/*          Physical Device Number (0 ~ 7)                                   */
/*      nCode                                                                */
/*          IO Control Command                                               */
/*      pBufI                                                                */
/*          Input Buffer pointer                                             */
/*      nLenI                                                                */
/*          Length of Input Buffer                                           */
/*      pBufO                                                                */
/*          Output Buffer pointer                                            */
/*      nLenO                                                                */
/*          Length of Output Buffer                                          */
/*      pByteRet                                                             */
/*          The number of bytes (length) of Output Buffer as the result      */
/*          of function call                                                 */
/* RETURN VALUES                                                             */
/*      ONLD_IOC_NOT_SUPPORT                                                 */
/*          In case of Command which is not supported                        */
/*      ONLD_ILLEGAL_ACCESS                                                  */
/*          In case of illegal access                                        */
/*      ONLD_SUCCESS                                                         */
/*          Data of previous write op copy success                           */
/*      ONLD_WRITE_ERROR | ONLD_PREV_OP_RESULT                               */
/*          Previous Write Failure                                           */
/*          DCOP(Deferred Check Operation)                                   */
/*      ONLD_ERASE_ERROR | ONLD_PREV_OP_RESULT                               */
/*          Previous Erase Failure.                                          */
/*          DCOP(Deferred Check Operation)                                   */
/*      ONLD_WR_PROTECT_ERROR | ONLD_PREV_OP_RESULT                          */
/*          Previous Operation Write Protect Error                           */
/*          DCOP(Deferred Check Operation)                                   */
/* NOTES                                                                     */
/*      This function can be called for LLD function extension               */
/*                                                                           */
/*      Deferred Check Operation : The method of operation perfromance       */
/*          optimization. The DCOP method is to terminate the function (write*/
/*          or erase) right after command issue. Then it checks the result of*/
/*          operation at next function call.                                 */
/*                                                                           */
/*****************************************************************************/
INT32
onld_ioctl(UINT32  nDev,  UINT32 nCode,
           UINT8  *pBufI, UINT32 nLenI,
           UINT8  *pBufO, UINT32 nLenO,
           UINT32 *pByteRet)
{
    INT32   nRet;
    UINT32  nBAddr;
    UINT32  nPbn;
    UINT16  nRegVal;

#if defined(DEFERRED_CHK)
    INT32   nRes;
#endif /* #if defined(DEFERRED_CHK) */
    
    ONLD_DBG_PRINT((TEXT("[ONLD :  IN] ++ONLD_IOCtrl() nDev = %d, nCode = 0x%08x\r\n"), 
                    nDev, nCode)); 
	
	if (pByteRet == NULL) /* To avoid refer to 0 Address */
    {
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (IO_CTRL)Illegal Access pByteRet = NULL\r\n")));
        return (ONLD_ILLEGAL_ACCESS);
    }
    
    nBAddr = GET_DEV_BADDR(nDev);

#if defined(STRICT_CHK)
    /*  Check Device Number */
    if (nDev > MAX_SUPPORT_DEVS)
    {
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (IO_CTRL)Invalid Device Number (nDev = %d)\r\n"), nDev));
        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_erase()\r\n")));

        return (ONLD_ILLEGAL_ACCESS);
    }

    /* Check Device Open Flag */
    if (CHK_DEV_OPEN_FLAG(nDev) == FALSE32)
    {
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (IO_CTRL)Device is not opened\r\n")));
        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_setrwarea()\r\n")));

        return (ONLD_ILLEGAL_ACCESS);
    }

    /* Check Device Power Flag */
    if (CHK_DEV_PWR_FLAG(nDev) == FALSE32)
    {
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (IO_CTRL)Dev PwrFlg is not set\r\n")));
        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_setrwarea()\r\n")));

        return (ONLD_ILLEGAL_ACCESS);
    }
    
#endif  /* #if defined(STRICT_CHK) */


#if defined(DEFERRED_CHK)
    if ((nRes = _ChkPrevOpResult(nDev)) != ONLD_SUCCESS)
    {
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (IO_CTRL)Previous Operation Error 0x%08x\r\n"), 
                        nRes));
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (IO_CTRL)Dev = %d, Psn = %d, Scts = %d"),
                        nDev,
                        pstPrevOpInfo[nDev]->nPsn, 
                        pstPrevOpInfo[nDev]->nScts));
        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_write()\r\n")));
        return nRes;
    }
#endif /* #if defined(DEFERRED_CHK) */

    switch (nCode)
    {
        case LLD_IOC_SET_SECURE_LT:
            if (astONLDInfo[nDev].LockTight != NULL)
                nRet = astONLDInfo[nDev].LockTight(nDev, pBufI, nLenI);
            else
                nRet = (ONLD_ILLEGAL_ACCESS);  	     
            *pByteRet = (UINT32)0;

            break;

        case LLD_IOC_SET_BLOCK_LOCK:

            nRet = _LockBlock(nDev, pBufI, nLenI);
	     
            *pByteRet = (UINT32)0;

            break;
            
        case LLD_IOC_GET_SECURE_STAT:
            if ((pBufO == NULL) || (nLenO < sizeof(UINT16)))
            {
                ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (IO_CTRL)Illegal Access pBufO = 0x%08x, nLenO = %d\r\n"), 
                                pBufO, nLenO));
                return (ONLD_ILLEGAL_ACCESS);
            }

            nPbn    = *(UINT32*)pBufI;

            ONLD_REG_START_ADDR2(nBAddr) = (UINT16)MAKE_DBS(nPbn);
            ONLD_REG_START_ADDR1(nBAddr) = (UINT16)MAKE_FBA(nPbn);

            nRegVal = ONLD_REG_WR_PROTECT_STAT(nBAddr);

            MEMCPY(pBufO, &nRegVal, sizeof(UINT16));

            *pByteRet = sizeof(UINT16);
            nRet = ONLD_SUCCESS;
            break;

        case LLD_IOC_RESET_NAND_DEV:

            if ((GET_DEV_DID(nDev) & 0x0008) == DDP_CHIP)
            {
	            ONLD_REG_START_ADDR2(nBAddr) = (UINT16)(0x0000);
		        ONLD_REG_START_ADDR1(nBAddr) = (UINT16)(0x0000);
	
	            ONLD_REG_INT(nBAddr)            = (UINT16)INT_CLEAR;
	    
	            /*-------------------------------------------------------------------*/
	            /* ONLD Unlock CMD is issued                                         */
	            /*-------------------------------------------------------------------*/
	            ONLD_REG_CMD(nBAddr)            = (UINT16)ONLD_CMD_RESET;
			
	            /* wait until device ready */
	            WAIT_ONLD_INT_STAT(nBAddr, PEND_INT);

	            ONLD_REG_START_ADDR2(nBAddr) = (UINT16)(0x8000);	
		        ONLD_REG_START_ADDR1(nBAddr) = (UINT16)(0x8000);
	        } /*if DDP_CHIP */
        
            ONLD_REG_INT(nBAddr)            = (UINT16)INT_CLEAR;
    
            /*-------------------------------------------------------------------*/
            /* ONLD Unlock CMD is issued                                         */
            /*-------------------------------------------------------------------*/
            ONLD_REG_CMD(nBAddr)            = (UINT16)ONLD_CMD_RESET;
        
            /* Wait until device ready */
            WAIT_ONLD_INT_STAT(nBAddr, PEND_INT);
            
            *pByteRet = (UINT32)0;
            nRet = ONLD_SUCCESS;
            break;

        default:
            nRet = ONLD_IOC_NOT_SUPPORT;
            break;
    }
    return nRet;
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      onld_merase                                                          */
/* DESCRIPTION                                                               */
/*      This function erases a List of physical blocks                       */
/* PARAMETERS                                                                */
/*      nDev                                                                 */
/*          Physical Device Number (0 ~ 7)                                   */
/*      pstMEArg                                                             */
/*          List of Physical Blocks                                          */
/*      nFlag                                                                */
/*          Operation Option (Sync/Async)                                    */
/* RETURN VALUES                                                             */
/*      ONLD_ILLEGAL_ACCESS                                                  */
/*          Illegal Erase Operation Try                                      */
/*      ONLD_WRITE_ERROR | ONLD_PREV_OP_RESULT                               */
/*          Previous Write Failure                                           */
/*          DCOP(Deferred Check Operation)                                   */
/*      ONLD_ERASE_ERROR | ONLD_PREV_OP_RESULT                               */
/*          Previous Erase Failure.                                          */
/*          DCOP(Deferred Check Operation)                                   */
/*      ONLD_WR_PROTECT_ERROR | ONLD_PREV_OP_RESULT                          */
/*          Previous Operation Write Protect Error                           */
/*          DCOP(Deferred Check Operation)                                   */
/*      ONLD_SUCCESS                                                         */
/*          Data write success                                               */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
INT32 
onld_merase(UINT32 nDev, LLDMEArg *pstMEArg, UINT32 nFlag)
{
    UINT32       nBAddr, nPbn, nFirstchipPbn = 0xffff, nSecondchipPbn = 0xffff;
    UINT16       nCnt;
#if defined(DEFERRED_CHK)
    INT32        nRes;

#endif /* #if defined(DEFERRED_CHK) */    
    INT32        nRet = ONLD_SUCCESS;
    LLDMEList    *pstMEPList; 
    
    ONLD_DBG_PRINT((TEXT("[ONLD: IN] ++ONLD_MultiBLKErase()\r\n")));
    
    nBAddr = GET_DEV_BADDR(nDev);

    ONLD_DBG_PRINT((TEXT("[ONLD:MSG]  nBAddr = 0x%08x\r\n"), nBAddr));

#if defined(DEFERRED_CHK)
    if ((nRes = _ChkPrevOpResult(nDev)) != ONLD_SUCCESS)
    {
        ONLD_ERR_PRINT((TEXT("[ONLD:ERR]  (MERASE)Previous Operation Error 0x%08x\r\n"), 
                        nRes));
        ONLD_ERR_PRINT((TEXT("[ONLD:ERR]  (MERASE)Dev = %d, Psn = %d, Scts = %d"),
                        nDev,
                        pstPrevOpInfo[nDev]->nPsn, 
                        pstPrevOpInfo[nDev]->nScts));
        ONLD_DBG_PRINT((TEXT("[ONLD:OUT] --ONLD_MultiBLKErase()\r\n")));
        return nRes;
    }
#endif /* #if defined(DEFERRED_CHK) */

#if defined(STRICT_CHK)
    if (pstMEArg == NULL)
    {
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (MERASE)Illeagal Access\r\n")));
        ONLD_DBG_PRINT((TEXT("[ONLD:OUT] --ONLD_MultiBLKErase()\r\n")));
        return(ONLD_ILLEGAL_ACCESS);
    }
#endif  /* #if defined(STRICT_CHK) */ 
    
    pstMEPList = pstMEArg->pstMEList;
    
    for(nCnt = 0; nCnt < pstMEArg->nNumOfMList; nCnt++)
    {
        if ((((pstMEPList[nCnt].nMEListPbn << astONLDInfo[nDev].nDDPSelSft) & MASK_DBS) == 0x0000)  && (nFirstchipPbn == 0xffff))
        {
             nFirstchipPbn = pstMEPList[nCnt].nMEListPbn;
             continue;
        }
        else if ((((pstMEPList[nCnt].nMEListPbn << astONLDInfo[nDev].nDDPSelSft) & MASK_DBS) == 0x8000)  && (nSecondchipPbn == 0xffff))
        {
             nSecondchipPbn = pstMEPList[nCnt].nMEListPbn;
             continue;
         }

        ONLD_REG_START_ADDR2(nBAddr) = (UINT16)MAKE_DBS(pstMEPList[nCnt].nMEListPbn);        
        /* Block Number Set */
        ONLD_REG_START_ADDR1(nBAddr) = (UINT16)MAKE_FBA(pstMEPList[nCnt].nMEListPbn);
	    
	    /* INT Stat Reg Clear */
        ONLD_REG_INT(nBAddr)         = (UINT16)INT_CLEAR;
        /*-----------------------------------------------------------------------*/
        /* ONLD Erase CMD is issued                                              */
        /*-----------------------------------------------------------------------*/
        ONLD_REG_CMD(nBAddr)         = (UINT16)ONLD_CMD_ERASE_MBLK;
        
		ONLD_DBG_PRINT((TEXT("[ONLD:MSG]  block number nPbn = %d)\r\n"), pstMEPList[nCnt].nMEListPbn));

        /* Wait until device ready */
        WAIT_ONLD_INT_STAT_CHECKING_PROTECTION(nBAddr, PEND_INT);

        if (GET_ONLD_CTRL_STAT(nBAddr, ERROR_STATE) == ERROR_STATE)
        {
            ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (MErase)MErase Error\r\n")));
            ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_merase()\r\n")));
        }

        ADD_DELAY(nBAddr, DUMMY_READ_CNT);
        
    }

    if ((nFirstchipPbn != 0xffff) && (nSecondchipPbn != 0xffff))
    {           
        ONLD_REG_START_ADDR2(nBAddr) = (UINT16)MAKE_DBS(nFirstchipPbn);
        /* Block Number Set */
        ONLD_REG_START_ADDR1(nBAddr) = (UINT16)MAKE_FBA(nFirstchipPbn);
      
        /* INT Stat Reg Clear */
        ONLD_REG_INT(nBAddr)         = (UINT16)INT_CLEAR;
                
        ONLD_REG_CMD(nBAddr)         = (UINT16)ONLD_CMD_ERASE_BLK;    

            WAIT_ONLD_INT_STAT_CHECKING_PROTECTION(nBAddr, PEND_INT);
    
	    if (GET_ONLD_CTRL_STAT(nBAddr, ERROR_STATE) == ERROR_STATE)
	    {
	        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (MErase)MErase Error\r\n")));
	        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_merase()\r\n")));
	    }

        ADD_DELAY(nBAddr, DUMMY_READ_CNT);
		        
        ONLD_REG_START_ADDR2(nBAddr) = (UINT16)MAKE_DBS(nSecondchipPbn);
        /* Block Number Set */
        ONLD_REG_START_ADDR1(nBAddr) = (UINT16)MAKE_FBA(nSecondchipPbn);

#ifdef ASYNC_MODE
            /* in case of async mode, interrupt should be enabled */
            if ( (nFlag & ONLD_FLAG_ASYNC_OP) != 0 )
            {
            	PAM_ClearInt((UINT32)XSR_INT_ID_NAND_0);
            	PAM_EnableInt((UINT32)XSR_INT_ID_NAND_0);    	
            }
#endif
	
	    /* INT Stat Reg Clear */
	    ONLD_REG_INT(nBAddr)         = (UINT16)INT_CLEAR;
	    	    
	    ONLD_REG_CMD(nBAddr)         = (UINT16)ONLD_CMD_ERASE_BLK;    

		nPbn = nSecondchipPbn;
			
#if !defined(DEFERRED_CHK) || defined(SYNC_MODE)	
        WAIT_ONLD_INT_STAT_CHECKING_PROTECTION(nBAddr, PEND_INT);

	    if (GET_ONLD_CTRL_STAT(nBAddr, ERROR_STATE) == ERROR_STATE)
	    {
	        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (MErase)MErase Error\r\n")));
	        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --ONLD_MultiBLKErase()\r\n")));
	    }

        ADD_DELAY(nBAddr, DUMMY_READ_CNT);
#endif /* #if !defined(DEFERRED_CHK) */
    }

    else if (nFirstchipPbn != 0xffff)
    {
	    ONLD_REG_START_ADDR2(nBAddr) = (UINT16)MAKE_DBS(nFirstchipPbn);	        
	    /* Block Number Set */
	    ONLD_REG_START_ADDR1(nBAddr) = (UINT16)MAKE_FBA(nFirstchipPbn);

#ifdef ASYNC_MODE
            /* in case of async mode, interrupt should be enabled */
            if ( (nFlag & ONLD_FLAG_ASYNC_OP) != 0 )
            {
            	PAM_ClearInt((UINT32)XSR_INT_ID_NAND_0);
            	PAM_EnableInt((UINT32)XSR_INT_ID_NAND_0);    	
            }
#endif
	
	   /* INT Stat Reg Clear */
	    ONLD_REG_INT(nBAddr)         = (UINT16)INT_CLEAR;
	    	    
	    ONLD_REG_CMD(nBAddr)         = (UINT16)ONLD_CMD_ERASE_BLK;    

		nPbn = nFirstchipPbn;
				
#if !defined(DEFERRED_CHK) || defined(SYNC_MODE)
        WAIT_ONLD_INT_STAT_CHECKING_PROTECTION(nBAddr, PEND_INT);

	    if (GET_ONLD_CTRL_STAT(nBAddr, ERROR_STATE) == ERROR_STATE)
	    {
	        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (MErase)MErase Error\r\n")));
	        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --ONLD_MultiBLKErase()\r\n")));
	    }
	    		
        ADD_DELAY(nBAddr, DUMMY_READ_CNT);
#endif /* #if !defined(DEFERRED_CHK) */
    }

    else 
    {
        ONLD_REG_START_ADDR2(nBAddr) = (UINT16)MAKE_DBS(nSecondchipPbn);
        /* Block Number Set */
        ONLD_REG_START_ADDR1(nBAddr) = (UINT16)MAKE_FBA(nSecondchipPbn);

#ifdef ASYNC_MODE
    /* in case of async mode, interrupt should be enabled */
    if ( (nFlag & ONLD_FLAG_ASYNC_OP) != 0 )
    {
    	PAM_ClearInt((UINT32)XSR_INT_ID_NAND_0);
    	PAM_EnableInt((UINT32)XSR_INT_ID_NAND_0);    	
    }
#endif
	
	   /* INT Stat Reg Clear */
	    ONLD_REG_INT(nBAddr)         = (UINT16)INT_CLEAR;
	    	    
	    ONLD_REG_CMD(nBAddr)         = (UINT16)ONLD_CMD_ERASE_BLK;    

		nPbn = nSecondchipPbn;
				
#if !defined(DEFERRED_CHK) || defined(SYNC_MODE)
        WAIT_ONLD_INT_STAT_CHECKING_PROTECTION(nBAddr, PEND_INT);
        
	    if (GET_ONLD_CTRL_STAT(nBAddr, ERROR_STATE) == ERROR_STATE)
	    {
	        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (MErase)MErase Error\r\n")));
	        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --ONLD_MultiBLKErase()\r\n")));
	    }
	    		
        ADD_DELAY(nBAddr, DUMMY_READ_CNT);
#endif /* #if !defined(DEFERRED_CHK) */
    }

#if defined(DEFERRED_CHK) && defined(ASYNC_MODE)
    pstPrevOpInfo[nDev]->ePreOp     = MERASE;
    pstPrevOpInfo[nDev]->nFlag      = nFlag;
    pstPrevOpInfo[nDev]->nPsn       = nPbn * GET_SCTS_PER_BLK(nDev);
    pstPrevOpInfo[nDev]->pstPreMEArg->nNumOfMList = pstMEArg->nNumOfMList;
    pstPrevOpInfo[nDev]->pstPreMEArg->nBitMapErr = pstMEArg->nBitMapErr;
    pstPrevOpInfo[nDev]->pstPreMEArg->bFlag = pstMEArg->bFlag;
    MEMCPY(pstPrevOpInfo[nDev]->pstPreMEArg->pstMEList, pstMEArg->pstMEList, sizeof(LLDMEList) * XSR_MAX_MEBLK);
#endif /* #if defined(DEFERRED_CHK) */

    ONLD_DBG_PRINT((TEXT("[ONLD:OUT] --ONLD_MultiBLKErase()\r\n")));    
    
    return nRet;

}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      onld_mread                                                           */
/* DESCRIPTION                                                               */
/*      This function read multiple sectors from NAND flash                  */
/* PARAMETERS                                                                */
/*      nDev                                                                 */
/*          Physical Device Number (0 ~ 7)                                   */
/*      nPsn                                                                 */
/*          Sector index to be read                                          */
/*      nScts                                                                */
/*          The number of sectors to be read                                 */
/*      pstSGL                                                               */
/*          SGL structure for main array of NAND flash                       */
/*      pSBuf                                                                */
/*          Memory buffer for spare array of NAND flash                      */
/*      nFlg                                                                 */
/*          Operation options such as ECC_ON, OFF                            */
/* RETURN VALUES                                                             */
/*      ONLD_ILLEGAL_ACCESS                                                  */
/*          Illegal Read Operation Try                                       */
/*      ONLD_WRITE_ERROR | ONLD_PREV_OP_RESULT                               */
/*          In case of Deferred Check Operation. Previous Write failure.     */
/*      ONLD_ERASE_ERROR | ONLD_PREV_OP_RESULT                               */
/*          In case of Deferred Check Operation. Previous Erase failure.     */
/*      ONLD_READ_ERROR                                                      */
/*          In case of ECC Error Occur.                                      */
/*      ONLD_SUCCESS                                                         */
/*          Data write success                                               */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
INT32
onld_mread(UINT32 nDev, UINT32 nPsn, UINT32 nScts, SGL *pstSGL, UINT8 *pSBuf, 
          UINT32 nFlag)
{

    UINT32  nBAddr;
    UINT32  nRet = ONLD_SUCCESS;
    
#if defined(DEFERRED_CHK)
    INT32   nRes;
#endif /* #if defined(DEFERRED_CHK) */

    ONLD_DBG_PRINT((TEXT("[ONLD :  IN] ++onld_mread() nDev = %d, nPsn = %d, nScts = %d, nFlag = 0x%x\r\n"), 
                    nDev, nPsn, nScts, nFlag));

    if (pstSGL == NULL)
    {
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (READ)Buffer Pointer Error\r\n")));
        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_mread(()\r\n")));
        return (ONLD_ILLEGAL_ACCESS);
    }
    

    nBAddr = GET_DEV_BADDR(nDev);

    ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  nBAddr = 0x%08x\r\n"), nBAddr));

#ifdef CONFIG_RFS_TINYBML
	/* shared operation result */
    /* If previous operation is not read, save previous operation result */
	tiny_shared = 1;
	if(xsr_shared)
	{
		do 
		{
			while (GET_ONLD_INT_STAT(nBAddr, PEND_INT) != (UINT16)PEND_INT)
			{
				/* Wait until device ready */
			}

			if ((GET_ONLD_INT_STAT(nBAddr, PEND_ERASE) == (UINT16)PEND_ERASE) ||
				(GET_ONLD_INT_STAT(nBAddr, PEND_WRITE) == (UINT16)PEND_WRITE))
			{
				result_ctrl_status[nDev] = GET_ONLD_CTRL_STAT(nBAddr, 0xFFFF);
				break;
			}

			/* Set tinyBML operation block and tinyBML shared value reset */
			ONLD_REG_START_ADDR2(nBAddr) = (UINT16)MAKE_DBS(xsr_block_shared);

			if (ONLD_REG_CMD (nBAddr) == 0x000E)
			{
				ONLD_REG_INT (nBAddr) = (UINT16) INT_CLEAR;
				ONLD_REG_CMD (nBAddr) = (UINT16) ONLD_CMD_FINISH_CACHE_READ;

				while (GET_ONLD_INT_STAT(nBAddr, PEND_INT) != (UINT16)PEND_INT)
				{
					/* Wait until device ready */
				}
			}
		} while(0);
	}
#endif	/* CONFIG_RFS_TINYBML */

 
#if defined(DEFERRED_CHK)
#if defined(USE_CACHE_AHEAD)
    if (astONLDInfo[nDev].MRead != _cacheahead)
#endif
    {
        if ((nRes = _ChkPrevOpResult(nDev)) != ONLD_SUCCESS)
        {
            pstPrevOpInfo[nDev]->ePreOp = NONE;
            ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (READ)Previous Operation Error 0x%08x\r\n"), 
                            nRes));
            ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (READ)Dev = %d, Psn = %d, Scts = %d"),
                            nDev,
                            pstPrevOpInfo[nDev]->nPsn, 
                            pstPrevOpInfo[nDev]->nScts));
            ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_write()\r\n")));
            return nRes;
        }
	}
#endif /* #if defined(DEFERRED_CHK) */ 


    if (astONLDInfo[nDev].MRead != NULL)
    {
        nRet = astONLDInfo[nDev].MRead(nDev, nPsn, nScts, pstSGL, pSBuf, nFlag);
    }
    else
    {
        return (ONLD_ILLEGAL_ACCESS);
    }
    
    ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_mread()\r\n")));
    
    return (nRet);
}    

#if defined(USE_CACHE_READ) && defined(USE_CACHE_AHEAD)
INT32
_cacheahead(UINT32 nDev, UINT32 nPsn, UINT32 nScts, SGL *pstSGL, UINT8 *pSBuf, 
          UINT32 nFlag)
{
    UINT16  nPbn;
    UINT16  nBSA;
    UINT32  nBAddr;
    UINT16  nEccRes      = (UINT16)0x0;
    UINT16  nEccMask     = (UINT16)0x0;
    UINT16  n2bitEccMask = (UINT16)0x0;
    UINT16	aEccRes[8];
    UINT8   nBitIdx;
    UINT8   nCnt;

    UINT32  nFlagO      = nFlag;

    UINT32  nRemainScts;               // the total # of remaining sectors to be loaded
    UINT32  nNumOfScts = 0;            // # of sectors to be loaded at a time

    UINT32  nPastSA[2];                // 1 and 2 cycle earlier sector address
    UINT32  nPastNumOfScts[2];         // 1 and 2 cycle earlier # of sectors, but those have been transferred
    UINT32  nPastIdx = 0;              // the index of circular buffer
    UINT8  *pMBuf;                     // main buffer pointer
    UINT16  nPastEccRes = 0;           // 2 cycle earlier ecc result
    UINT32 *pONLDMBuf;
    UINT32 *pONLDSBuf;

    UINT32  nSGLIdx = 0;
    UINT32  nSGLSctCnt = 0;

    INT32  	nRet = ONLD_SUCCESS;
    BOOL32  bIsOneBitError  = FALSE32;

    OpType  ePrevOp;
    INT32   nRes;
    INT32   nPrevPpn;
    INT32   nPpn;
    UINT32  nLastPsn;
    UINT8  *pCurMBuf;
    UINT8  *pCurSBuf;
    CacheCase nCacheCase;
    UINT32  nPageOffsetMask;

	ONLD_DBG_PRINT((TEXT("[ONLD :  IN] ++_cacheahead() nDev = %d, nPsn = %d, nScts = %d, nFlag = 0x%x\r\n"), 
                    nDev, nPsn, nScts, nFlag));

   
    ePrevOp = GET_PREV_OP_TYPE(nDev);

    if ((nRes = _ChkPrevOpResultByCacheRead(nDev)) != ONLD_SUCCESS)
    {
        pstPrevOpInfo[nDev]->ePreOp = NONE;
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (READ)Previous Operation Error 0x%08x\r\n"), 
                        nRes));
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (READ)Dev = %d, Psn = %d, Scts = %d"),
                        nDev,
                        pstPrevOpInfo[nDev]->nPsn, 
                        pstPrevOpInfo[nDev]->nScts));
        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_write()\r\n")));
        return nRes;
    }

    nBAddr = GET_DEV_BADDR(nDev);

	if ( (nFlag & ONLD_FLAG_ECC_ON) != 0 )
	{
		/* System Configuration Reg set (ECC On)*/
		ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  ECC ON\r\n")));
		ONLD_REG_SYS_CONF1(nBAddr) &= CONF1_ECC_ON;
	}
	else
	{
		/* System Configuration Reg set (ECC Off)*/
		ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  ECC OFF\r\n")));
		ONLD_REG_SYS_CONF1(nBAddr) |= CONF1_ECC_OFF;
	}
    
    /*--------------------------------------------------- */
    /*    Check if cache hit or miss                      */
    /*--------------------------------------------------- */
#ifdef CONFIG_RFS_TINYBML
	if ( xsr_shared )
	{
		xsr_shared = 0;
    	ePrevOp = NONE;
	}
#endif	/* CONFIG_RFS_TINYBML */

    if (ePrevOp == READ)
    {
        nPrevPpn = GET_PPN(nDev, pstPrevOpInfo[nDev]->nPsn);
        nPpn = GET_PPN(nDev, nPsn);

        switch (nPrevPpn - nPpn)
        {
        case 0:
            nCacheCase = CACHE_HIT2; // (n+2)th page hit
            break;
        case 1:
            nCacheCase = CACHE_HIT1; // (n+1)th page hit
            break;
        case 2:
            nCacheCase = CACHE_HIT0; // (n)th page hit
            break;
        default:
            nCacheCase = CACHE_MISS; // cache miss
            break;
        }
    }
    else nCacheCase = CACHE_START;

    /* Initialize */
    pMBuf = pstSGL->stSGLEntry[nSGLIdx].pBuf;    
    nRemainScts = nScts;
    nLastPsn = (GET_PPN(nDev, nPsn + nScts - 1) + 2) << astONLDInfo[nDev].nPgSelSft;   // with extra 2 pages
    nPbn = GET_PBN (nDev, nPsn);
    ONLD_DBG_PRINT ((TEXT ("[ONLD : MSG]  nPbn = %d\r\n"), nPbn));
    nPageOffsetMask = GET_PGS_PER_BLK(nDev) - 1;
        
   	/* Generate ECC bit mask */
   	if ((nFlagO & ONLD_FLAG_ECC_ON) != 0)
   	{
	    nEccMask = 0;

		if (pMBuf != NULL)
		{
			nEccMask |= ECC_MAIN_BIT_ERR;
		}
		if (pSBuf != NULL)
		{
			nEccMask |= ECC_SPARE_BIT_ERR;
		}
   	}

    /* Calculate nNumOfScts */
    nNumOfScts = astONLDInfo[nDev].nSctsPerPG - (nPsn & astONLDInfo[nDev].nSctSelSft);
    
    if (nNumOfScts > nRemainScts) nNumOfScts = nRemainScts;
    
    /*--------------------------------------------------- */
    /*             < CACHE HIT Case 0 >                   */
    /*--------------------------------------------------- */
    while (nCacheCase == CACHE_HIT0)
    {
		if ( (nFlagO & ONLD_FLAG_ECC_ON) != 0 )
		{
			nPastEccRes = (UINT16) pstPrevOpInfo[nDev]->nScts;
			nEccRes = (nPastEccRes & nEccMask);

			if (nEccRes != 0)
			{
    			nBitIdx = (nPsn & astONLDInfo[nDev].nFSAMask) * 4;
    			
                for (n2bitEccMask = 0, nCnt = nNumOfScts; nCnt > 0; nCnt--, nBitIdx += 4)
                {
                    if (pMBuf != NULL)
                    {
                    	n2bitEccMask |= (0x08 << nBitIdx);
                    }
                    if (pSBuf != NULL)
                    {
                    	n2bitEccMask |= (0x02 << nBitIdx);
                    }
                }

			    if (nEccRes & n2bitEccMask)
			    {
			        /* 2bit ECC error occurs */
    				ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  ECC Error Occurs during Read (Pbn : %d, Psn : %d)\r\n"), nPbn, nPsn));
    				ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  +----------M3S3M2S2M1S1M0S0-+\r\n")));
    				ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  |ECC Error Code    : 0x%04x |\r\n"), ONLD_REG_ECC_STAT(nBAddr)));
    				ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  +---------------------------+\r\n")));
    				ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_read()\r\n")));
    
    				nCacheCase = CACHE_MISS;
                    break;
    			}
    			else
    			{
    			    /* 1bit ECC error occurs */
        		    MEMCPY16(aEccRes, (UINT16*)(&ONLD_REG_ECC_RSLT_MB0(nBAddr) + (nPsn & astONLDInfo[nDev].nFSAMask) * 2),
        		           sizeof(UINT16) * nNumOfScts * 2);           		
            		bIsOneBitError = TRUE32;    			    
    			}
			}
		}

        /* Select the other BufferRAM */
        pONLDMBuf = (UINT32 *) GET_ONLD_MBUF_ADDR (nBAddr, nPsn, GET_NXT_BUF_SEL (nDev));
        pONLDSBuf = (UINT32 *) GET_ONLD_SBUF_ADDR (nBAddr, nPsn, GET_NXT_BUF_SEL (nDev));

        if (pMBuf != NULL)
        {
            /* Memcopy for main data */
            _ReadMain (pMBuf, pONLDMBuf, nNumOfScts);
            pCurMBuf = pMBuf;
            pMBuf += ONLD_MAIN_SIZE * nNumOfScts;
        }
		else
		{
			pCurMBuf = NULL;
		}

        if (pSBuf != NULL)
        {
            /* Memcopy for main data */
            _ReadSpare (pSBuf, pONLDSBuf, nNumOfScts);
            pCurSBuf = pSBuf;
            pSBuf += ONLD_SPARE_SIZE * nNumOfScts;
        }
		else
		{
			pCurSBuf = NULL;
		}

		if (bIsOneBitError && nRet == ONLD_SUCCESS)
		{
			if (_ChkReadDisturb(nNumOfScts, nEccRes, 
			                    aEccRes, pCurMBuf, pCurSBuf)
        	 	== TRUE32)
    		{
    			nRet = (ONLD_READ_DISTURBANCE);
    		}
    		bIsOneBitError = FALSE32;
    	}
        
   	    /* Update the variables */
    	nPsn += nNumOfScts;
    	nRemainScts -= nNumOfScts;
        nSGLSctCnt += nNumOfScts;

    	/* Increase nSGLSctCnt or nSGLIdx */
        while (nRemainScts > 0 && nSGLSctCnt >= pstSGL->stSGLEntry[nSGLIdx].nSectors)
        {
            nSGLSctCnt -= pstSGL->stSGLEntry[nSGLIdx].nSectors;
            nSGLIdx++;
        }
        
        if (nRemainScts <= 0)
        {
            pstPrevOpInfo[nDev]->ePreOp = READ;
            return nRet;
        }

        /* Calculate the number of sector to be read */
        nNumOfScts = (nRemainScts < astONLDInfo[nDev].nSctsPerPG) ? nRemainScts : astONLDInfo[nDev].nSctsPerPG;

    	/* Skip the area of meta data */
        if (pstSGL->stSGLEntry[nSGLIdx].nFlag == SGL_ENTRY_META_DATA)
        {
    		/* assuming that meta data is not located at any side of the user data */
            nPsn += pstSGL->stSGLEntry[nSGLIdx].nSectors;
            nNumOfScts -= pstSGL->stSGLEntry[nSGLIdx].nSectors;
            nRemainScts -= pstSGL->stSGLEntry[nSGLIdx].nSectors;
            nSGLIdx++;
            nSGLSctCnt = 0;
            
            if (GET_SCTS_PER_PG(nDev) == 2)
            {
        		nNumOfScts = (astONLDInfo[nDev].nSctsPerPG > nRemainScts) ? nRemainScts : astONLDInfo[nDev].nSctsPerPG;
            }
        }

        /* Now it becomes CACHE HIT Case 1 */
        nCacheCase = CACHE_HIT1;
        break;
    }
    /*--------------------------------------------------- */
    /*             < Cache Miss Case >                    */
    /*--------------------------------------------------- */
    while (nCacheCase == CACHE_MISS)
    {
        if ((GET_DEV_DID(nDev) & DDP_CHIP) == DDP_CHIP)
        {
            UINT32 nPrevOpBlk, nDDPBndBlk;
            
            nPrevOpBlk = GET_PBN(nDev, pstPrevOpInfo[nDev]->nPsn - 2 * GET_SCTS_PER_PG(nDev));
            nDDPBndBlk = GET_NUM_OF_BLKS(nDev) >> 1;

            if ((nPrevOpBlk >= nDDPBndBlk && nPbn <  nDDPBndBlk) ||
                (nPrevOpBlk <  nDDPBndBlk && nPbn >= nDDPBndBlk))
            {
                /* Finish the cache-read operation */
        	    ONLD_REG_INT (nBAddr) = (UINT16) INT_CLEAR;
        	    ONLD_REG_CMD (nBAddr) = (UINT16) ONLD_CMD_FINISH_CACHE_READ;

                WAIT_ONLD_INT_STAT(nBAddr, PEND_INT);

                nCacheCase = CACHE_START;
                break;
            }
        }

    	/**
    	 * Set the flash addr
    	 */
        /* Set block addr */
        ONLD_REG_START_ADDR2 (nBAddr) = (UINT16) MAKE_DBS (nPbn);
        ONLD_REG_START_ADDR1 (nBAddr) = (UINT16) MAKE_FBA (nPbn);
    	
        /* Set page addr */
        ONLD_REG_START_ADDR8 (nBAddr) = (UINT16) ((nPsn << astONLDInfo[nDev].nFPASelSft) & MASK_FPA);

        /* we don't need to check ecc of the previous pages */

    	/**
    	 * Clear INT / Issue command
    	 */
        ONLD_REG_INT (nBAddr) = (UINT16) INT_CLEAR;		 
        ONLD_REG_CMD (nBAddr) = (UINT16) ONLD_CMD_CACHE_READ;

        /* Skip DRAM copy */

        pstPrevOpInfo[nDev]->nBufSel = GET_NXT_BUF_SEL (nDev);

        nCacheCase = CACHE_HIT2;
        break;
    }

    /*--------------------------------------------------- */
    /*            < CACHE HIT Case 1, 2 >                 */
    /*                                                    */
    /*   nPsn is on the (n+1)th page or the (n+2)th page  */
    /*        of the previous cache read operation        */  
    /* -------------------------------------------------- */
    if (nCacheCase == CACHE_HIT1 || nCacheCase == CACHE_HIT2)
    {
        /* Preserve the variables */ 
    	nPastSA[nPastIdx] = nPsn & astONLDInfo[nDev].nFSAMask; 
    	nPastNumOfScts[nPastIdx] = nNumOfScts; 
    	nPastIdx = 1 - nPastIdx;  
     
        /* Update the variables */  
    	nPsn = (1 + GET_PPN(nDev, nPsn)) << astONLDInfo[nDev].nPgSelSft;  
     
    	nRemainScts -= nNumOfScts; 
        nSGLSctCnt += nNumOfScts; 
    
    	/* Increase nSGLSctCnt or nSGLIdx */ 
        while (nRemainScts > 0 && nSGLSctCnt >= pstSGL->stSGLEntry[nSGLIdx].nSectors) 
        {  
            nSGLSctCnt -= pstSGL->stSGLEntry[nSGLIdx].nSectors;  
            nSGLIdx++;  
        }  
     
        /* Calculate the number of sector to be read */		 
        nNumOfScts = (nRemainScts < astONLDInfo[nDev].nSctsPerPG) ? nRemainScts : astONLDInfo[nDev].nSctsPerPG; 
     
    	/* Skip the area of meta data */ 
        if (pstSGL->stSGLEntry[nSGLIdx].nFlag == SGL_ENTRY_META_DATA) 
        { 
    		/* assuming that meta data is not located at any side of the user data */ 
            nPsn += pstSGL->stSGLEntry[nSGLIdx].nSectors; 
            nNumOfScts -= pstSGL->stSGLEntry[nSGLIdx].nSectors; 
            nRemainScts -= pstSGL->stSGLEntry[nSGLIdx].nSectors; 
            nSGLIdx++;
            nSGLSctCnt = 0;
             
            if (GET_SCTS_PER_PG(nDev) == 2) 
            { 
        		nNumOfScts = (nRemainScts < astONLDInfo[nDev].nSctsPerPG) ? nRemainScts : astONLDInfo[nDev].nSctsPerPG; 
            } 
        }         
    }

    /*--------------------------------------------------- */
    /*   if the target page is the (n+2)th page,          */
    /*   it should be transferred                         */
    /* -------------------------------------------------- */
    if (nCacheCase == CACHE_HIT2)
    {
    	/**
    	 * Set the flash addr
    	 */
        /* Set block addr */
        ONLD_REG_START_ADDR2 (nBAddr) = (UINT16) MAKE_DBS (nPbn);
        ONLD_REG_START_ADDR1 (nBAddr) = (UINT16) MAKE_FBA (nPbn);
    	
        /* Set page addr */
        ONLD_REG_START_ADDR8 (nBAddr) = (UINT16) ((nPsn << astONLDInfo[nDev].nFPASelSft) & MASK_FPA);

        /* we don't need to check ecc of the (n+1)th page that has missed an opportunity */

    	/**
    	 * Wait until INT is high
    	 */	
        WAIT_ONLD_INT_STAT(nBAddr, PEND_READ);

    	/**
    	 * Clear INT / Issue command
    	 */
        ONLD_REG_INT (nBAddr) = (UINT16) INT_CLEAR;		 
        ONLD_REG_CMD (nBAddr) = (UINT16) ONLD_CMD_CACHE_READ;

        pstPrevOpInfo[nDev]->nBufSel = GET_NXT_BUF_SEL (nDev);
    }

    /*--------------------------------------------------- */
    /*             < New Start Case >                     */
    /*                                                    */
    /* Issue the 1st cache read command                   */
    /*--------------------------------------------------- */
    else if (nCacheCase == CACHE_START)
    {
        /*---------------------------- */
        /* The 1st Cache Read Command  */
        /* --------------------------- */
        /**
    	 * 1) Set the 1st flash addr
    	 */
        /* Set block addr */
        ONLD_REG_START_ADDR2 (nBAddr) = (UINT16) MAKE_DBS (nPbn); // DDP

        /* Set destination addr
         * In this case, the number of sectors to be transferred is always 4 */
        nBSA = (UINT16) MAKE_BSA (astONLDInfo[nDev].nSctsPerPG, GET_CUR_BUF_SEL (nDev));
        ONLD_REG_START_BUF (nBAddr) = (UINT16) (nBSA & MASK_BSA);

        ONLD_REG_START_ADDR3 (nBAddr) = (UINT16) MAKE_FCBA (nPbn); // block addr
        
        /* Set page addr */
        ONLD_REG_START_ADDR4 (nBAddr) = (UINT16)((nPsn << astONLDInfo[nDev].nFPASelSft) & MASK_FPA);

        /* Preserve the variables */ 
    	nPastSA[nPastIdx] = nPsn & astONLDInfo[nDev].nFSAMask; 
    	nPastNumOfScts[nPastIdx] = nNumOfScts; 
    	nPastIdx = 1 - nPastIdx;  
     
        /* Update the variables */  
    	nPsn = (1 + GET_PPN(nDev, nPsn)) << astONLDInfo[nDev].nPgSelSft;  
     
    	nRemainScts -= nNumOfScts; 
        nSGLSctCnt += nNumOfScts; 
    
    	/* Increase nSGLSctCnt or nSGLIdx */ 
        while (nRemainScts > 0 && nSGLSctCnt >= pstSGL->stSGLEntry[nSGLIdx].nSectors) 
        {  
            nSGLSctCnt -= pstSGL->stSGLEntry[nSGLIdx].nSectors;  
            nSGLIdx++;  
        }  
     
        /* Calculate the number of sector to be read */		 
        nNumOfScts = (nRemainScts < astONLDInfo[nDev].nSctsPerPG) ? nRemainScts : astONLDInfo[nDev].nSctsPerPG; 
     
    	/* Skip the area of meta data */ 
        if (pstSGL->stSGLEntry[nSGLIdx].nFlag == SGL_ENTRY_META_DATA) 
        { 
    		/* assuming that meta data is not located at any side of the user data */ 
            nPsn += pstSGL->stSGLEntry[nSGLIdx].nSectors; 
            nNumOfScts -= pstSGL->stSGLEntry[nSGLIdx].nSectors; 
            nRemainScts -= pstSGL->stSGLEntry[nSGLIdx].nSectors; 
            nSGLIdx++;
            nSGLSctCnt = 0;
             
            if (GET_SCTS_PER_PG(nDev) == 2) 
            { 
        		nNumOfScts = (nRemainScts < astONLDInfo[nDev].nSctsPerPG) ? nRemainScts : astONLDInfo[nDev].nSctsPerPG; 
            } 
        } 
    	
        /**
    	 * 2) Set the 2nd flash addr
    	 */
        /* Set block addr */
        ONLD_REG_START_ADDR1 (nBAddr) = (UINT16) MAKE_FBA (nPbn);

        /* Set page addr */
        ONLD_REG_START_ADDR8 (nBAddr) = (UINT16) ((nPsn << astONLDInfo[nDev].nFPASelSft) & MASK_FPA);

        /**
         * 3) Clear INT
         */
        ONLD_REG_INT (nBAddr) = (UINT16) INT_CLEAR;

        /**
         * 4) ONLD Cache Read CMD is issued   
         */
        ONLD_REG_CMD (nBAddr) = (UINT16) ONLD_CMD_CACHE_READ;

		/**
	     * 5) Read Controller Status Register (DQ[15] = Ongo & DQ[13] = Load)
	     */
		WAIT_ONLD_CTRL_STAT(nBAddr, (CTRL_ONGO | LOAD_STATE));
    }

    /*------------------------------------------------------ */
    /*                 < COMMON LOOP >                       */
    /*------------------------------------------------------ */
    while (1)
    {
    	/**
    	 * 1) Preserve & update the variables
    	 */
        /* Preserve the variables */ 
    	nPastSA[nPastIdx] = nPsn & astONLDInfo[nDev].nFSAMask; 
    	nPastNumOfScts[nPastIdx] = nNumOfScts; 
    	nPastIdx = 1 - nPastIdx;  
     
        /* Update the variables */  
    	nPsn = (1 + GET_PPN(nDev, nPsn)) << astONLDInfo[nDev].nPgSelSft;  
     
    	/* if all sectors are loaded */ 
    	if (nPsn > nLastPsn) break;  
     
    	nRemainScts -= nNumOfScts; 
        nSGLSctCnt += nNumOfScts; 
    
    	/* Increase nSGLSctCnt or nSGLIdx */ 
        while (nRemainScts > 0 && nSGLSctCnt >= pstSGL->stSGLEntry[nSGLIdx].nSectors) 
        {  
            nSGLSctCnt -= pstSGL->stSGLEntry[nSGLIdx].nSectors;  
            nSGLIdx++;  
        }  
     
        /* Calculate the number of sector to be read */		 
        nNumOfScts = (nRemainScts < astONLDInfo[nDev].nSctsPerPG) ? nRemainScts : astONLDInfo[nDev].nSctsPerPG; 
     
    	/* Skip the area of meta data */ 
        if (pstSGL->stSGLEntry[nSGLIdx].nFlag == SGL_ENTRY_META_DATA) 
        { 
    		/* assuming that meta data is not located at any side of the user data */ 
            nPsn += pstSGL->stSGLEntry[nSGLIdx].nSectors; 
            nNumOfScts -= pstSGL->stSGLEntry[nSGLIdx].nSectors; 
            nRemainScts -= pstSGL->stSGLEntry[nSGLIdx].nSectors; 
            nSGLIdx++;
            nSGLSctCnt = 0;
             
            if (GET_SCTS_PER_PG(nDev) == 2) 
            { 
        		nNumOfScts = (nRemainScts < astONLDInfo[nDev].nSctsPerPG) ? nRemainScts : astONLDInfo[nDev].nSctsPerPG; 
            } 
        } 

    	/**
    	 * 2) Set the flash addr
    	 */
        /* Set block addr */
        ONLD_REG_START_ADDR1 (nBAddr) = (UINT16) MAKE_FBA (nPbn);
        
        /* Set page addr */
        ONLD_REG_START_ADDR8 (nBAddr) = (UINT16) ((nPsn << astONLDInfo[nDev].nFPASelSft) & MASK_FPA);

        /**
         * 3) Wait until INT is high
         */ 
        WAIT_ONLD_INT_STAT(nBAddr, PEND_READ);
    	
        /**
         * 4) Check ECC error
         */
		if ( (nFlagO & ONLD_FLAG_ECC_ON) != 0 )
		{
			nPastEccRes = ONLD_REG_ECC_STAT(nBAddr);
			nEccRes = (nPastEccRes & nEccMask);

			if (nEccRes != 0)
			{
    			nBitIdx = nPastSA[nPastIdx] * 4;
    			
    			for (n2bitEccMask = 0, nCnt = nPastNumOfScts[nPastIdx]; nCnt > 0; nCnt--, nBitIdx += 4)
    			{
    				if (pMBuf != NULL)
    				{
    					n2bitEccMask |= (0x08 << nBitIdx);
    				}
    				if (pSBuf != NULL)
    				{
    					n2bitEccMask |= (0x02 << nBitIdx);
    				}
    			}

			    if (nEccRes & n2bitEccMask)
			    {
			        /* 2bit ECC error occurs */			    
    				ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  ECC Error Occurs during Read (Pbn : %d, Psn : %d)\r\n"), nPbn, nPsn));
    				ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  +----------M3S3M2S2M1S1M0S0-+\r\n")));
    				ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  |ECC Error Code    : 0x%04x |\r\n"), ONLD_REG_ECC_STAT(nBAddr)));
    				ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  +---------------------------+\r\n")));
    				ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_read()\r\n")));
    
    				nRet = ONLD_READ_ERROR | (nEccRes & n2bitEccMask);
    			}
    			else
    			{
    			    /* 1bit ECC error occurs */
                    MEMCPY16(aEccRes, (UINT16*)(&ONLD_REG_ECC_RSLT_MB0(nBAddr) + nPastSA[nPastIdx] * 2),
        		           sizeof(UINT16) * nPastNumOfScts[nPastIdx] * 2);           		
            		bIsOneBitError = TRUE32;    			    
    			}
			}
		}
    	
    	/**
    	 * 5) Clear INT / Issue command
    	 */
        ONLD_REG_INT (nBAddr) = (UINT16) INT_CLEAR;		 
        ONLD_REG_CMD (nBAddr) = (UINT16) ONLD_CMD_CACHE_READ;

    	/**
    	 * 6) Copy data to DRAM
    	 */
        pONLDMBuf = (UINT32 *) GET_ONLD_MBUF_ADDR (nBAddr, nPastSA[nPastIdx], GET_CUR_BUF_SEL (nDev));
        pONLDSBuf = (UINT32 *) GET_ONLD_SBUF_ADDR (nBAddr, nPastSA[nPastIdx], GET_CUR_BUF_SEL (nDev));
        pstPrevOpInfo[nDev]->nBufSel = GET_NXT_BUF_SEL (nDev);

        if (pMBuf != NULL)
        {
            /* Memcopy for main data */
            _ReadMain (pMBuf, pONLDMBuf, nPastNumOfScts[nPastIdx]);
            pCurMBuf = pMBuf;
            pMBuf += ONLD_MAIN_SIZE * nPastNumOfScts[nPastIdx];
        }
		else
		{
			pCurMBuf = NULL;
		}

        if (pSBuf != NULL)
        {
            /* Memcopy for main data */
            _ReadSpare (pSBuf, pONLDSBuf, nPastNumOfScts[nPastIdx]);
            pCurSBuf = pSBuf;
            pSBuf += ONLD_SPARE_SIZE * nPastNumOfScts[nPastIdx];
        }
		else
		{
			pCurSBuf = NULL;
		}

		if (bIsOneBitError && nRet == ONLD_SUCCESS)
		{
			if (_ChkReadDisturb(nPastNumOfScts[nPastIdx], nEccRes, 
			                    aEccRes, pCurMBuf, pCurSBuf)
        	 	== TRUE32)
    		{
    			nRet = (ONLD_READ_DISTURBANCE);
    		}
    		bIsOneBitError = FALSE32;
    	}
    }

    /* Check the boundary */
    if (((GET_PPN(nDev, nLastPsn) - 2) & nPageOffsetMask) >= GET_PGS_PER_BLK(nDev) - 2)
    {
        WAIT_ONLD_INT_STAT(nBAddr, PEND_READ);
        
        ONLD_REG_INT (nBAddr) = (UINT16) INT_CLEAR;
        ONLD_REG_CMD (nBAddr) = (UINT16) ONLD_CMD_FINISH_CACHE_READ;

        WAIT_ONLD_INT_STAT(nBAddr, PEND_READ);
        
        pstPrevOpInfo[nDev]->ePreOp = NONE;

        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --_cacheahead()\r\n")));
        return (ONLD_SUCCESS);
    }

	pstPrevOpInfo[nDev]->ePreOp     = READ;
	pstPrevOpInfo[nDev]->nPsn       = nLastPsn;
	pstPrevOpInfo[nDev]->nScts      = nPastEccRes;
	pstPrevOpInfo[nDev]->nFlag      = nFlag;

#ifdef CONFIG_RFS_TINYBML
	tiny_block_shared = GET_PBN(nDev, nLastPsn);
#endif

    ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --_cacheahead()\r\n")));
    return (nRet);
}
#endif /* #if defined(USE_CACHE_READ) && defined(USE_CACHE_AHEAD) */

#if defined(USE_CACHE_READ) && !defined(USE_CACHE_AHEAD)
INT32
_cacheread(UINT32 nDev, UINT32 nPsn, UINT32 nScts, SGL *pstSGL, UINT8 *pSBuf, 
          UINT32 nFlag)
{
	UINT16  nPbn;
    UINT16  nBSA;
    UINT32  nBAddr;
    UINT16  nEccRes     = (UINT16)0x0;
    UINT16  nEccMask    = (UINT16)0x0;
    UINT16  n2bitEccMask = (UINT16)0x0;    
    UINT16	aEccRes[8];

	UINT8  nBitIdx;
	UINT8  nCnt;
    UINT32  nFlagO      = nFlag;

	UINT32  nRemainScts;               // the total # of remaining sectors to be loaded
	UINT32  nNumOfScts = 0;            // # of sectors to be loaded at a time

    UINT32  nPastSA[2];                // two cycle earlier sector address
    UINT32  nPastNumOfScts[2];         // two cycle earlier # of sectors, but those have been transferred
    UINT32  nPastIdx = 0;              // the index of circular buffer
	UINT8  *pMBuf;                     // main buffer pointer
    UINT32 *pONLDMBuf;
	UINT32 *pONLDSBuf;

    UINT32  nSGLIdx = 0;
    UINT32  nSGLSctCnt = 0;
  
    INT32  	nRet = ONLD_SUCCESS;
    BOOL32  bIsOneBitError  = FALSE32;
    UINT8  *pCurMBuf;
    UINT8  *pCurSBuf;

    /* Select _mread or _cacheread */
    if (GET_PPN(nDev, nPsn + nScts - 1) == GET_PPN(nDev, nPsn))
    {
        return _mread(nDev, nPsn, nScts, pstSGL, pSBuf, nFlag);
    }

	ONLD_DBG_PRINT((TEXT("[ONLD :  IN] ++_cacheread() nDev = %d, nPsn = %d, nScts = %d, nFlag = 0x%x\r\n"), 
                    nDev, nPsn, nScts, nFlag));

    nBAddr = GET_DEV_BADDR(nDev);
    
	if ( (nFlag & ONLD_FLAG_ECC_ON) != 0 )
	{
		/* System Configuration Reg set (ECC On)*/
		ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  ECC ON\r\n")));
		ONLD_REG_SYS_CONF1(nBAddr) &= CONF1_ECC_ON;
	}
	else
	{
		/* System Configuration Reg set (ECC Off)*/
		ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  ECC OFF\r\n")));
		ONLD_REG_SYS_CONF1(nBAddr) |= CONF1_ECC_OFF;
	}
        
    pMBuf = pstSGL->stSGLEntry[nSGLIdx].pBuf;    
	
    /* Total number of sectors */        	
    nRemainScts = nScts;

   	/* Generate ECC bit mask */
   	if ((nFlagO & ONLD_FLAG_ECC_ON) != 0)
   	{
	    nEccMask = 0;

		if (pMBuf != NULL)
		{
			nEccMask |= ECC_MAIN_BIT_ERR;
		}
		if (pSBuf != NULL)
		{
			nEccMask |= ECC_SPARE_BIT_ERR;
		}
   	}

    /* Determine the # of sectors to be read at one load operation */
    nNumOfScts = astONLDInfo[nDev].nSctsPerPG - (nPsn & astONLDInfo[nDev].nSctSelSft);
    if (nNumOfScts > nRemainScts) nNumOfScts = nRemainScts;

    /*---------------------------- */
    /* The 1st Cache Read Command  */
    /* --------------------------- */
	
    /**
	 * 1) Set the 1st flash addr
	 */
    nPbn = GET_PBN (nDev, nPsn);
    ONLD_DBG_PRINT ((TEXT ("[ONLD : MSG]  nPbn = %d\r\n"), nPbn));
	
    /* Set block addr */
    ONLD_REG_START_ADDR2 (nBAddr) = (UINT16) MAKE_DBS (nPbn);

    /* BSA, BSC */
    nBSA = (UINT16) MAKE_BSA (astONLDInfo[nDev].nSctsPerPG, GET_CUR_BUF_SEL (nDev));
    ONLD_REG_START_BUF (nBAddr) = (UINT16) (nBSA & MASK_BSA);

    /* FCBA */
    ONLD_REG_START_ADDR3 (nBAddr) = (UINT16) MAKE_FCBA (nPbn);
    
    /* FCPA, FCSA */
    ONLD_REG_START_ADDR4 (nBAddr) = (UINT16)((nPsn << astONLDInfo[nDev].nFPASelSft) & MASK_FPA);

    /* Preserve the variables */
	nPastSA[nPastIdx] = nPsn & astONLDInfo[nDev].nFSAMask;
	nPastNumOfScts[nPastIdx] = nNumOfScts;
	nPastIdx = 1 - nPastIdx;

    /* Update the variables */
	nPsn += nNumOfScts;
	nRemainScts -= nNumOfScts;
    nSGLSctCnt += nNumOfScts;

	/* Increase nSGLSctCnt or nSGLIdx */
    while (nSGLSctCnt >= pstSGL->stSGLEntry[nSGLIdx].nSectors)
    {
        nSGLSctCnt -= pstSGL->stSGLEntry[nSGLIdx].nSectors;
        nSGLIdx++;
    }

    /* Calculate the number of sector to be read */
    nNumOfScts = (astONLDInfo[nDev].nSctsPerPG > nRemainScts) ? nRemainScts : astONLDInfo[nDev].nSctsPerPG;

	/* Skip the area of meta data */	
    if (pstSGL->stSGLEntry[nSGLIdx].nFlag == SGL_ENTRY_META_DATA)
    {
        nPsn += pstSGL->stSGLEntry[nSGLIdx].nSectors;
        nNumOfScts -= pstSGL->stSGLEntry[nSGLIdx].nSectors;
        nRemainScts -= pstSGL->stSGLEntry[nSGLIdx].nSectors;
        nSGLIdx++;
        nSGLSctCnt = 0;
        
        if (GET_SCTS_PER_PG(nDev) == 2) // Now that nNumOfScts becomes 0,
        {
    		nNumOfScts = (astONLDInfo[nDev].nSctsPerPG > nRemainScts) ? nRemainScts : astONLDInfo[nDev].nSctsPerPG;
        }
    }
	
    /**
	 * 2) Set the 2nd flash addr
	 */
    /* Set block addr */
    nPbn = GET_PBN (nDev, nPsn);

    /* DFS, FBA */
    ONLD_REG_START_ADDR1 (nBAddr) = (UINT16) MAKE_FBA (nPbn);

    /* FPA, FSA */
    ONLD_REG_START_ADDR8 (nBAddr) = (UINT16) ((nPsn << astONLDInfo[nDev].nFPASelSft) & MASK_FPA);

    /**
     * 3) Clear INT
     */
    ONLD_REG_INT (nBAddr) = (UINT16) INT_CLEAR;

    /**
     * 4) ONLD Cache Read CMD is issued   
     */
    ONLD_REG_CMD (nBAddr) = (UINT16) ONLD_CMD_CACHE_READ;

    /**
     * 5) Read Controller Status Register (DQ[15] = Ongo & DQ[13] = Load)
     */
     WAIT_ONLD_CTRL_STAT(nBAddr, (CTRL_ONGO | LOAD_STATE));

    /*------------------------------------*/
    /* Cache Read Loop                    */
	/*                                    */
	/* - set flash addr                   */
	/* - wait until INT is high           */
	/* - check error                      */
	/* - clear INT                        */
	/* - issue command                    */
	/* - copy data to DRAM                */
    /* -----------------------------------*/
    while (1)
    {
		/**
		 * 1) Preserve & update the variables
		 */
		 
	    /* Preserve the variables */
		nPastSA[nPastIdx] = nPsn & astONLDInfo[nDev].nFSAMask;
		nPastNumOfScts[nPastIdx] = nNumOfScts;
		nPastIdx = 1 - nPastIdx;

	    /* Update the variables */
		nPsn += nNumOfScts;
		nRemainScts -= nNumOfScts;

		/* if there is no remaining sector */
		if (nRemainScts <= 0) break;

	    nSGLSctCnt += nNumOfScts;

		/* Increase nSGLSctCnt or nSGLIdx */
	    while (nSGLSctCnt >= pstSGL->stSGLEntry[nSGLIdx].nSectors)
	    {
            nSGLSctCnt -= pstSGL->stSGLEntry[nSGLIdx].nSectors;
	        nSGLIdx++;
	    }

        /* Calculate the number of sector to be read */		
	    nNumOfScts = (astONLDInfo[nDev].nSctsPerPG > nRemainScts) ? nRemainScts : astONLDInfo[nDev].nSctsPerPG;

		/* Skip the area of meta data */
	    if (pstSGL->stSGLEntry[nSGLIdx].nFlag == SGL_ENTRY_META_DATA)
	    {
            /* assuming that meta data is not located at any side of the user data */
            nPsn += pstSGL->stSGLEntry[nSGLIdx].nSectors;
            nNumOfScts -= pstSGL->stSGLEntry[nSGLIdx].nSectors;
            nRemainScts -= pstSGL->stSGLEntry[nSGLIdx].nSectors;
            nSGLIdx++;
            nSGLSctCnt = 0;
	        
	        if (GET_SCTS_PER_PG(nDev) == 2)
	        {
	    		nNumOfScts = (astONLDInfo[nDev].nSctsPerPG > nRemainScts) ? nRemainScts : astONLDInfo[nDev].nSctsPerPG;
	        }
	    }

		/**
		 * 2) Set the flash addr
		 */
        nPbn = GET_PBN (nDev, nPsn);
		
        /* Set block addr */
        ONLD_REG_START_ADDR1 (nBAddr) = (UINT16) MAKE_FBA (nPbn);
        
        /* FPA, FSA */
        ONLD_REG_START_ADDR8 (nBAddr) = (UINT16) ((nPsn << astONLDInfo[nDev].nFPASelSft) & MASK_FPA);

        /**
        * 3) Wait until INT is high
         */ 
        WAIT_ONLD_INT_STAT(nBAddr, PEND_READ);
        
		
        /**
         * 4) Check ECC error
         */
		if ( (nFlagO & ONLD_FLAG_ECC_ON) != 0 )
		{
			/* No Cache Read */
			nEccRes = (ONLD_REG_ECC_STAT(nBAddr) & nEccMask);

			if (nEccRes != 0)
			{
    			nBitIdx = nPastSA[nPastIdx] * 4;
    			
    			for (n2bitEccMask = 0, nCnt = nPastNumOfScts[nPastIdx]; nCnt > 0; nCnt--, nBitIdx += 4)
    			{
    				if (pMBuf != NULL)
    				{
    					n2bitEccMask |= (0x08 << nBitIdx);
    				}
    				if (pSBuf != NULL)
    				{
    					n2bitEccMask |= (0x02 << nBitIdx);
    				}
    			}

			    if (nEccRes & n2bitEccMask)
			    {
			        /* 2bit ECC error occurs */			    
    				ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  ECC Error Occurs during Read (Pbn : %d, Psn : %d)\r\n"), nPbn, nPsn));
    				ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  +----------M3S3M2S2M1S1M0S0-+\r\n")));
    				ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  |ECC Error Code    : 0x%04x |\r\n"), ONLD_REG_ECC_STAT(nBAddr)));
    				ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  +---------------------------+\r\n")));
    				ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_read()\r\n")));
    
    				nRet = ONLD_READ_ERROR | (nEccRes & n2bitEccMask);
    			}
    			else
    			{
    			    /* 1bit ECC error occurs */
                    MEMCPY16(aEccRes, (UINT16*)(&ONLD_REG_ECC_RSLT_MB0(nBAddr) + nPastSA[nPastIdx] * 2),
        		           sizeof(UINT16) * nPastNumOfScts[nPastIdx] * 2);           		
            		bIsOneBitError = TRUE32;    			    
    			}
			}
		}
		
		/**
		 * 5) Clear INT / Issue command
		 */
        ONLD_REG_INT (nBAddr) = (UINT16) INT_CLEAR;		 
        ONLD_REG_CMD (nBAddr) = (UINT16) ONLD_CMD_CACHE_READ;

		/**
		 * 6) Copy data to DRAM
		 */
        pONLDMBuf = (UINT32 *) GET_ONLD_MBUF_ADDR (nBAddr, nPastSA[nPastIdx], GET_CUR_BUF_SEL (nDev));
        pONLDSBuf = (UINT32 *) GET_ONLD_SBUF_ADDR (nBAddr, nPastSA[nPastIdx], GET_CUR_BUF_SEL (nDev));
        pstPrevOpInfo[nDev]->nBufSel = GET_NXT_BUF_SEL (nDev);
	
        if (pMBuf != NULL)
        {
            /* Memcopy for main data */
            _ReadMain (pMBuf, pONLDMBuf, nPastNumOfScts[nPastIdx]);
            pCurMBuf = pMBuf;
            pMBuf += ONLD_MAIN_SIZE * nPastNumOfScts[nPastIdx];
        }
		else
		{
			pCurMBuf = NULL;
		}

        if (pSBuf != NULL)
        {
            /* Memcopy for main data */
            _ReadSpare (pSBuf, pONLDSBuf, nPastNumOfScts[nPastIdx]);
            pCurSBuf = pSBuf;
            pSBuf += ONLD_SPARE_SIZE * nPastNumOfScts[nPastIdx];
        }
		else
		{
			pCurSBuf = NULL;
		}

		if (bIsOneBitError && nRet == ONLD_SUCCESS)
		{
			if (_ChkReadDisturb(nPastNumOfScts[nPastIdx], nEccRes, 
			                    aEccRes, pCurMBuf, pCurSBuf)
        	 	== TRUE32)
    		{
    			nRet = (ONLD_READ_DISTURBANCE);
    		}
    		bIsOneBitError = FALSE32;
    	}

    }

    /*------------------------------ */
    /* Finish Cache Read Command     */
    /* ----------------------------- */
    {
	    /**
		 * Wait until INT is high
		 */
        WAIT_ONLD_INT_STAT(nBAddr, PEND_READ);

        /**
         * Check ECC error
         */
		if ( (nFlagO & ONLD_FLAG_ECC_ON) != 0 )
		{
			/* No Cache Read */
			nEccRes = (ONLD_REG_ECC_STAT(nBAddr) & nEccMask);

			if (nEccRes != 0)
			{
    			nBitIdx = nPastSA[nPastIdx] * 4;
    			
    			for (n2bitEccMask = 0, nCnt = nPastNumOfScts[nPastIdx]; nCnt > 0; nCnt--, nBitIdx += 4)
    			{
    				if (pMBuf != NULL)
    				{
    					n2bitEccMask |= (0x08 << nBitIdx);
    				}
    				if (pSBuf != NULL)
    				{
    					n2bitEccMask |= (0x02 << nBitIdx);
    				}
    			}
			
			    if (nEccRes & n2bitEccMask)
			    {
			        /* 2bit ECC error occurs */			    
    				ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  ECC Error Occurs during Read (Pbn : %d, Psn : %d)\r\n"), nPbn, nPsn));
    				ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  +----------M3S3M2S2M1S1M0S0-+\r\n")));
    				ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  |ECC Error Code    : 0x%04x |\r\n"), ONLD_REG_ECC_STAT(nBAddr)));
    				ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  +---------------------------+\r\n")));
    				ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_read()\r\n")));
    
    				nRet = ONLD_READ_ERROR | (nEccRes & n2bitEccMask);
    			}
    			else
    			{
    			    /* 1bit ECC error occurs */
                    MEMCPY16(aEccRes, (UINT16*)(&ONLD_REG_ECC_RSLT_MB0(nBAddr) + nPastSA[nPastIdx] * 2),
        		           sizeof(UINT16) * nPastNumOfScts[nPastIdx] * 2);
            		bIsOneBitError = TRUE32;    			    
    			}
			}
		}
		
		/**
		 * Clear INT / Issue command
		 */
	    ONLD_REG_INT (nBAddr) = (UINT16) INT_CLEAR;
	    ONLD_REG_CMD (nBAddr) = (UINT16) ONLD_CMD_FINISH_CACHE_READ;

		/**
		 * Copy data to DRAM
		 */
        pONLDMBuf = (UINT32 *) GET_ONLD_MBUF_ADDR (nBAddr, nPastSA[nPastIdx], GET_CUR_BUF_SEL (nDev));
        pONLDSBuf = (UINT32 *) GET_ONLD_SBUF_ADDR (nBAddr, nPastSA[nPastIdx], GET_CUR_BUF_SEL (nDev));
	    pstPrevOpInfo[nDev]->nBufSel = GET_NXT_BUF_SEL (nDev);

        if (pMBuf != NULL)
        {
            /* Memcopy for main data */
            _ReadMain (pMBuf, pONLDMBuf, nPastNumOfScts[nPastIdx]);
            pCurMBuf = pMBuf;            
            pMBuf += ONLD_MAIN_SIZE * nPastNumOfScts[nPastIdx];
        }
		else
		{
			pCurMBuf = NULL;
		}

        if (pSBuf != NULL)
        {
            /* Memcopy for main data */
            _ReadSpare (pSBuf, pONLDSBuf, nPastNumOfScts[nPastIdx]);
            pCurSBuf = pSBuf;
            pSBuf += ONLD_SPARE_SIZE * nPastNumOfScts[nPastIdx];
        }
		else
		{
			pCurSBuf = NULL;
		}

		if (bIsOneBitError && nRet == ONLD_SUCCESS)
		{
			if (_ChkReadDisturb(nPastNumOfScts[nPastIdx], nEccRes, 
			                    aEccRes, pCurMBuf, pCurSBuf)
        	 	== TRUE32)
    		{
    			nRet = (ONLD_READ_DISTURBANCE);
    		}
    	} 
    }
	
    /*----------------------------------- */
    /* the last memcopy from SRAM to DRAM */
    /* ---------------------------------- */
    {
		nPastIdx = 1 - nPastIdx;
		
        /**
		 * Wait until device ready
		 */
        WAIT_ONLD_INT_STAT(nBAddr, PEND_INT);

        /**
         * Check ECC error
         */
		if ( (nFlagO & ONLD_FLAG_ECC_ON) != 0 )
		{
			/* No Cache Read */
			nEccRes = (ONLD_REG_ECC_STAT(nBAddr) & nEccMask);

			if (nEccRes != 0)
			{
    			nBitIdx = nPastSA[nPastIdx] * 4;
    			
    			for (n2bitEccMask = 0, nCnt = nPastNumOfScts[nPastIdx]; nCnt > 0; nCnt--, nBitIdx += 4)
    			{
    				if (pMBuf != NULL)
    				{
    					n2bitEccMask |= (0x08 << nBitIdx);
    				}
    				if (pSBuf != NULL)
    				{
    					n2bitEccMask |= (0x02 << nBitIdx);
    				}
    			}
			    
			    if (nEccRes & n2bitEccMask)
			    {
			        /* 2bit ECC error occurs */			    
    				ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  ECC Error Occurs during Read (Pbn : %d, Psn : %d)\r\n"), nPbn, nPsn));
    				ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  +----------M3S3M2S2M1S1M0S0-+\r\n")));
    				ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  |ECC Error Code    : 0x%04x |\r\n"), ONLD_REG_ECC_STAT(nBAddr)));
    				ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  +---------------------------+\r\n")));
    				ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_read()\r\n")));
    
    				nRet = ONLD_READ_ERROR | (nEccRes & n2bitEccMask);
    			}
    			else
    			{
    			    /* 1bit ECC error occurs */
                    MEMCPY16(aEccRes, (UINT16*)(&ONLD_REG_ECC_RSLT_MB0(nBAddr) + nPastSA[nPastIdx] * 2), 
        		           sizeof(UINT16) * nPastNumOfScts[nPastIdx] * 2);           		
            		bIsOneBitError = TRUE32;    			    
    			}
			}
		}
		
        /**
		 * The last DRAM Copy
		 */
        pONLDMBuf = (UINT32 *) GET_ONLD_MBUF_ADDR (nBAddr, nPastSA[nPastIdx], GET_CUR_BUF_SEL (nDev));
        pONLDSBuf = (UINT32 *) GET_ONLD_SBUF_ADDR (nBAddr, nPastSA[nPastIdx], GET_CUR_BUF_SEL (nDev));
        pstPrevOpInfo[nDev]->nBufSel = GET_NXT_BUF_SEL (nDev);

        if (pMBuf != NULL)
        {
            /* Memcopy for main data */
            _ReadMain (pMBuf, pONLDMBuf, nPastNumOfScts[nPastIdx]);
            pCurMBuf = pMBuf;
            pMBuf += ONLD_MAIN_SIZE * nPastNumOfScts[nPastIdx];
        }
		else
		{
			pCurMBuf = NULL;
		}

        if (pSBuf != NULL)
        {
            /* Memcopy for main data */
            _ReadSpare (pSBuf, pONLDSBuf, nPastNumOfScts[nPastIdx]);
            pCurSBuf = pSBuf;
            pSBuf += ONLD_SPARE_SIZE * nPastNumOfScts[nPastIdx];
        }
		else
		{
			pCurSBuf = NULL;
		}

		if (bIsOneBitError && nRet == ONLD_SUCCESS)
		{
			if (_ChkReadDisturb(nPastNumOfScts[nPastIdx], nEccRes, 
			                    aEccRes, pCurMBuf, pCurSBuf)
        	 	== TRUE32)
    		{
    			nRet = (ONLD_READ_DISTURBANCE);
    		}
    	}
    }

    ONLD_DBG_PRINT ((TEXT ("[ONLD : OUT] --_cacheread()\r\n")));
    
    return (nRet);
}    
#endif /* #if defined(USE_CACHE_READ) && !defined(USE_CACHE_AHEAD) */


INT32
_mread(UINT32 nDev, UINT32 nPsn, UINT32 nScts, SGL *pstSGL, UINT8 *pSBuf, 
          UINT32 nFlag)
{
    UINT32  nCnt;
    UINT16  nBSA;
    UINT32 *pONLDMBuf;
	UINT32 *pONLDSBuf;
    UINT32  nBAddr;
    UINT16  nPbn;
    UINT16  nEccRes     = (UINT16)0x0;
    UINT16  nEccMask    = (UINT16)0x0;
    UINT16	aEccRes[8];

    UINT32  nFlagO      = nFlag;

	UINT32  nReadPsn;
	UINT32  nCurReadPsn;
	UINT32  nReadScts = 0;
	UINT32  nCurReadScts;
	UINT32  nTmpCurReadScts;
	UINT32  nRemainScts;
    INT32  	nRet = ONLD_SUCCESS;
    UINT8   nSGLIdx = 0;
    UINT32  nSctCount = 0;
    UINT8  *pMBuf = NULL;
    UINT8  *pCurMBuf;
    UINT8  *pCurSBuf;
    
    BOOL32  bIsOneBitError  = FALSE32;	

	ONLD_DBG_PRINT((TEXT("[ONLD :  IN] ++_mread() nDev = %d, nPsn = %d, nScts = %d, nFlag = 0x%x\r\n"), 
                    nDev, nPsn, nScts, nFlag));
                               
    nBAddr = GET_DEV_BADDR(nDev);

    if (pstSGL->stSGLEntry[nSGLIdx].nFlag == SGL_ENTRY_META_DATA)
    {
        nScts -= pstSGL->stSGLEntry[nSGLIdx].nSectors;
        nPsn  += pstSGL->stSGLEntry[nSGLIdx].nSectors;
        nSGLIdx++;
    }
    
    pMBuf = pstSGL->stSGLEntry[nSGLIdx].pBuf;    

	nRemainScts   = nScts;
	nReadPsn      = nPsn;
	switch (nReadPsn & astONLDInfo[nDev].nSctSelSft)
    {
        case 0:     nReadScts  = (astONLDInfo[nDev].nSctsPerPG > nRemainScts) ? nRemainScts : astONLDInfo[nDev].nSctsPerPG;  break;
        case 1:     nReadScts  = (astONLDInfo[nDev].nSctSelSft > nRemainScts) ? nRemainScts : astONLDInfo[nDev].nSctSelSft;  break;
        case 2:     nReadScts  = (2 > nRemainScts) ? nRemainScts : 2;  break;
        case 3:     nReadScts  = (1 > nRemainScts) ? nRemainScts : 1;  break;
    }

	{
		nPbn = GET_PBN(nDev, nReadPsn);

		ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  nPbn = %d\r\n"), nPbn));

		/* Block Number Set */
        ONLD_REG_START_ADDR2(nBAddr) = (UINT16)MAKE_DBS(nPbn);
        ONLD_REG_START_ADDR1(nBAddr) = (UINT16)MAKE_FBA(nPbn);

		/* Sector Number Set */
        ONLD_REG_START_ADDR8(nBAddr) = (UINT16)(((nReadPsn << astONLDInfo[nDev].nFPASelSft) & MASK_FPA) | (nReadPsn & astONLDInfo[nDev].nFSAMask));
    
		nBSA = (UINT16)MAKE_BSA(nReadPsn, GET_CUR_BUF_SEL(nDev));

		/* Start Buffer Selection */
		ONLD_REG_START_BUF(nBAddr)   = (UINT16)((nBSA & MASK_BSA) | (nReadScts & MASK_BSC));

		if ( (nFlag & ONLD_FLAG_ECC_ON) != 0 )
		{
			/* System Configuration Reg set (ECC On)*/
			ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  ECC ON\r\n")));
			ONLD_REG_SYS_CONF1(nBAddr) &= CONF1_ECC_ON;
		}
		else
		{
			/* System Configuration Reg set (ECC Off)*/
			ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  ECC OFF\r\n")));
			ONLD_REG_SYS_CONF1(nBAddr) |= CONF1_ECC_OFF;
		}

		/* INT Stat Reg Clear */
		ONLD_REG_INT(nBAddr)         = (UINT16)INT_CLEAR;
		
		/*-----------------------------------------------------------------------*/
		/* ONLD Read CMD is issued                                               */
		/*-----------------------------------------------------------------------*/
		if (pMBuf != NULL)
		{
			/* Page Read Command issue */
			ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  Read Page\r\n")));
			ONLD_REG_CMD(nBAddr)        = (UINT16)ONLD_CMD_READ_PAGE;
		}
		else
		{
			/* Spare Read Command issue */
			ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  Read Spare\r\n")));
			ONLD_REG_CMD(nBAddr)        = (UINT16)ONLD_CMD_READ_SPARE;
		}

		if ( (nFlagO & ONLD_FLAG_ECC_ON) != 0 )
		{
			/* ECC Value Bit Mask Setting */
			for (nCnt = 0; nCnt < nReadScts; nCnt++)
			{
				if (pMBuf != NULL)
				{
					nEccMask |= (0x0C << (nCnt * 4));
				}
				if (pSBuf != NULL)
				{
					nEccMask |= (0x03 << (nCnt * 4));
				}
			}

		}

		nCurReadPsn     = nReadPsn;
		nCurReadScts    = nReadScts;
		nTmpCurReadScts = nCurReadScts;

		nRemainScts  -= nReadScts;
		nReadPsn     += nReadScts;
		nReadScts     = (astONLDInfo[nDev].nSctsPerPG > nRemainScts) ? nRemainScts : astONLDInfo[nDev].nSctsPerPG;

        WAIT_ONLD_INT_STAT(nBAddr, PEND_READ);

		if ( (nFlagO & ONLD_FLAG_ECC_ON) != 0 )
		{
			/* No Cache Read */
			nEccRes = (ONLD_REG_ECC_STAT(nBAddr) & nEccMask);

			if (nEccRes != 0)
			{
			    nEccMask = 0;
    			/* ECC Value Bit Mask Setting */
    			for (nCnt = 0; nCnt < nCurReadScts; nCnt++)
    			{
    				if (pMBuf != NULL)
    				{
    					nEccMask |= (0x08 << (nCnt * 4));
    				}
    				if (pSBuf != NULL)
    				{
    					nEccMask |= (0x02 << (nCnt * 4));
    				}
    			}			    
			    
			    if (nEccRes & nEccMask)
			    {
			        /* 2bit ECC error occurs */
                                AD_Debug("ECC Error Occurs during Read (Pbn : %d, Psn : %d)\r\n", nPbn, nPsn);
    				ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  ECC Error Occurs during Read (Pbn : %d, Psn : %d)\r\n"), nPbn, nPsn));
    				ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  +----------M3S3M2S2M1S1M0S0-+\r\n")));
    				ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  |ECC Error Code    : 0x%04x |\r\n"), ONLD_REG_ECC_STAT(nBAddr)));
    				ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  +---------------------------+\r\n")));
    				ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_read()\r\n")));
    
    				nRet = ONLD_READ_ERROR | (nEccRes & nEccMask);
    			}
    			else
    			{
    			    /* 1bit ECC error occurs */
                    MEMCPY16(aEccRes, (UINT16*)&ONLD_REG_ECC_RSLT_MB0(nBAddr), sizeof(UINT16) * nCurReadScts * 2);                  
            		bIsOneBitError = TRUE32;    			    
    			}
    			
			}
        }
	}
    
	while (nRemainScts > 0)
	{
		nPbn = GET_PBN(nDev, nReadPsn);

		ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  nPbn = %d\r\n"), nPbn));

        nSctCount += nTmpCurReadScts;

        if (nSctCount == pstSGL->stSGLEntry[nSGLIdx].nSectors)
        {
            nSctCount = 0;
            nSGLIdx++;        
        } 
        if (pstSGL->stSGLEntry[nSGLIdx].nFlag == SGL_ENTRY_META_DATA)
        {
            nReadPsn += pstSGL->stSGLEntry[nSGLIdx].nSectors;
            nReadScts -= pstSGL->stSGLEntry[nSGLIdx].nSectors;
            nRemainScts -= pstSGL->stSGLEntry[nSGLIdx].nSectors;
            nSctCount = 0;
            nSGLIdx++;
            
            if (GET_SCTS_PER_PG(nDev) == 2)
            {
        		nReadScts     = (astONLDInfo[nDev].nSctsPerPG > nRemainScts) ? nRemainScts : astONLDInfo[nDev].nSctsPerPG;
        		nTmpCurReadScts = 0;
                continue;
            }
        }
       
		ONLD_REG_START_ADDR2(nBAddr) = (UINT16)MAKE_DBS(nPbn);       
		/* Block Number Set */
		ONLD_REG_START_ADDR1(nBAddr) = (UINT16)MAKE_FBA(nPbn);

		/* Sector Number Set */
        ONLD_REG_START_ADDR8(nBAddr) = (UINT16)(((nReadPsn << astONLDInfo[nDev].nFPASelSft) & MASK_FPA) | (nReadPsn & astONLDInfo[nDev].nFSAMask));
    
		nBSA = (UINT16)MAKE_BSA(nReadPsn, GET_NXT_BUF_SEL(nDev));

		/* Start Buffer Selection */
		ONLD_REG_START_BUF(nBAddr)   = (UINT16)((nBSA & MASK_BSA) | (nReadScts & MASK_BSC));

		/* INT Stat Reg Clear */
		ONLD_REG_INT(nBAddr)         = (UINT16)INT_CLEAR;
		
		/*-----------------------------------------------------------------------*/
		/* ONLD Read CMD is issued                                               */
		/*-----------------------------------------------------------------------*/
		if (pMBuf != NULL)
		{
			/* Page Read Command issue */
			ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  Read Page\r\n")));
			ONLD_REG_CMD(nBAddr)        = (UINT16)ONLD_CMD_READ_PAGE;
		}
		else
		{
			/* Spare Read Command issue */
			ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  Read Spare\r\n")));
			ONLD_REG_CMD(nBAddr)        = (UINT16)ONLD_CMD_READ_SPARE;
		}
        
        pONLDMBuf = (UINT32*)GET_ONLD_MBUF_ADDR(nBAddr, nCurReadPsn, GET_CUR_BUF_SEL(nDev));
        pONLDSBuf = (UINT32*)GET_ONLD_SBUF_ADDR(nBAddr, nCurReadPsn, GET_CUR_BUF_SEL(nDev));

		if (pMBuf != NULL)
		{
			/* Memcopy for main data */
			_ReadMain(pMBuf, pONLDMBuf, nCurReadScts);
			pCurMBuf = pMBuf;
			pMBuf += (ONLD_MAIN_SIZE * nCurReadScts);
		}
		else
		{
			pCurMBuf = NULL;
		}

		if (pSBuf != NULL)
		{
			/* Memcopy for main data */
			_ReadSpare(pSBuf, pONLDSBuf, nCurReadScts);
			pCurSBuf = pSBuf;
			pSBuf += (ONLD_SPARE_SIZE * nCurReadScts);
		}
		else
		{
			pCurSBuf = NULL;
		}

		if (bIsOneBitError && nRet == ONLD_SUCCESS)
		{
			if (_ChkReadDisturb(nCurReadScts, nEccRes, 
			                    aEccRes, pCurMBuf, pCurSBuf)
        	 	== TRUE32)
    		{
    			nRet = (ONLD_READ_DISTURBANCE);
    		}
    		bIsOneBitError = FALSE32;
    	}
        			
		if ( (nFlagO & ONLD_FLAG_ECC_ON) != 0 )
		{
			/* ECC Value Bit Mask Setting */
			for (nCnt = 0; nCnt < nReadScts; nCnt++)
			{
				if (pMBuf != NULL)
				{
					nEccMask |= (0x0C << (nCnt * 4));
				}
				if (pSBuf != NULL)
				{
					nEccMask |= (0x03 << (nCnt * 4));
				}
			}
		}

		pstPrevOpInfo[nDev]->nBufSel    = GET_NXT_BUF_SEL(nDev);

        WAIT_ONLD_INT_STAT(nBAddr, PEND_READ);

		if ( (nFlagO & ONLD_FLAG_ECC_ON) != 0 )
		{
			/* No Cache Read */
			nEccRes = (ONLD_REG_ECC_STAT(nBAddr) & nEccMask);

			if (nEccRes != 0)
			{
			    nEccMask = 0;
    			/* ECC Value Bit Mask Setting */
    			for (nCnt = 0; nCnt < nReadScts; nCnt++)
    			{
    				if (pMBuf != NULL)
    				{
    					nEccMask |= (0x08 << (nCnt * 4));
    				}
    				if (pSBuf != NULL)
    				{
    					nEccMask |= (0x02 << (nCnt * 4));
    				}
    			}			    
			    
			    if (nEccRes & nEccMask)
			    {
			        /* 2bit ECC error occurs */
                                AD_Debug("ECC Error Occurs during Read (Pbn : %d, Psn : %d)\r\n", nPbn, nPsn);
    				ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  ECC Error Occurs during Read (Pbn : %d, Psn : %d)\r\n"), nPbn, nReadPsn));
    				ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  +----------M3S3M2S2M1S1M0S0-+\r\n")));
    				ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  |ECC Error Code    : 0x%04x |\r\n"), ONLD_REG_ECC_STAT(nBAddr)));
    				ONLD_DBG_PRINT((TEXT("[ONLD : ERR]  +---------------------------+\r\n")));
    				ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_read()\r\n")));
    
    				nRet = ONLD_READ_ERROR | (nEccRes & nEccMask);
    			}
    			else
    			{
    			    /* 1bit ECC error occurs */
                    MEMCPY16(aEccRes, (UINT16*)&ONLD_REG_ECC_RSLT_MB0(nBAddr), sizeof(UINT16) * nReadScts * 2);                 
            		bIsOneBitError = TRUE32;    			    
    			}	
			}               	
		}
		
		nCurReadPsn   = nReadPsn;
		nCurReadScts  = nReadScts;
		nTmpCurReadScts = nCurReadScts;

		nRemainScts  -= nReadScts;
		nReadPsn     += nReadScts;
		nReadScts     = (astONLDInfo[nDev].nSctsPerPG > nRemainScts) ? nRemainScts : astONLDInfo[nDev].nSctsPerPG;

	}

	{
        pONLDMBuf = (UINT32*)GET_ONLD_MBUF_ADDR(nBAddr, nCurReadPsn, GET_CUR_BUF_SEL(nDev));
        pONLDSBuf = (UINT32*)GET_ONLD_SBUF_ADDR(nBAddr, nCurReadPsn, GET_CUR_BUF_SEL(nDev));

		if (pMBuf != NULL)
		{
			/* Memcopy for main data */
			_ReadMain(pMBuf, pONLDMBuf, nCurReadScts);
			pCurMBuf = pMBuf;
			pMBuf += (ONLD_MAIN_SIZE * nCurReadScts);
		}
		else
		{
			pCurMBuf = NULL;
		}

		if (pSBuf != NULL)
		{
			/* Memcopy for spare data */
			_ReadSpare(pSBuf, pONLDSBuf, nCurReadScts);
			pCurSBuf = pSBuf;
			pSBuf += (ONLD_SPARE_SIZE * nCurReadScts);
		}
		else
		{
			pCurSBuf = NULL;
		}

		if (bIsOneBitError && nRet == ONLD_SUCCESS)
		{
			if (_ChkReadDisturb(nCurReadScts, nEccRes, 
			                    aEccRes, pCurMBuf, pCurSBuf)
        	 	== TRUE32)
    		{
    			nRet = (ONLD_READ_DISTURBANCE);
    		}
    	} 
    	
		pstPrevOpInfo[nDev]->nBufSel    = GET_NXT_BUF_SEL(nDev);

	}
    
    ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --_mread()\r\n")));
    
    return (nRet);
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      onld_mwrite                                                          */
/* DESCRIPTION                                                               */
/*      This function write multiple sectors into NAND flash                 */
/* PARAMETERS                                                                */
/*      nDev                                                                 */
/*          Physical Device Number (0 ~ 7)                                   */
/*      nPsn                                                                 */
/*          Sector index to be written                                       */
/*      nScts                                                                */
/*          The number of sectors to be written                              */
/*      pMBuf                                                                */
/*          Memory buffer for main array of NAND flash                       */
/*      pSBuf                                                                */
/*          Memory buffer for spare array of NAND flash                      */
/*      nFlg                                                                 */
/*          Operation options such as ECC_ON, OFF                            */
/*      pErrPsn                                                              */
/*          Physical sector number which error occur                         */
/* RETURN VALUES                                                             */
/*      ONLD_ILLEGAL_ACCESS                                                  */
/*          Illegal Write Operation Try                                      */
/*      ONLD_WR_PROTECT_ERROR                                                */
/*          Attempt to write at Locked Area                                  */
/*      ONLD_WRITE_ERROR | ONLD_PREV_OP_RESULT                               */
/*          In case of Deferred Check Operation. Previous Write failure.     */
/*      ONLD_ERASE_ERROR | ONLD_PREV_OP_RESULT                               */
/*          In case of Deferred Check Operation. Previous Erase failure.     */
/*      ONLD_WRITE_ERROR                                                     */
/*          Data write failure                                               */
/*      ONLD_SUCCESS                                                         */
/*          Data write success                                               */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
INT32
onld_mwrite(UINT32 nDev, UINT32 nPsn, UINT32 nScts, SGL *pstSGL, UINT8 *pSBuf, 
           UINT32 nFlag, UINT32 *pErrPsn)
{
    UINT16  nBSA;
    UINT32 *pONLDMBuf;
	UINT32 *pONLDSBuf;
    UINT32  nBAddr;
    UINT32  nPbn;
	UINT32  nWritePsn;
	UINT32  nWriteScts = 0;
	UINT32  nTmpWriteScts;
	UINT8   nBiWriteScts;
	UINT8   nSnapWriteScts;
	UINT32  nRemainScts;
    INT32   nRes;
    UINT8   nSGLIdx;
    UINT8   nSGLCount;
    UINT32  nSctCount = 0;
    UINT32  nCurWritePsn = 0;
    UINT32  nCurWritenScts = 0;
    
    UINT8  *pMBuf = NULL;
    UINT8  *pSGLBuf = NULL;
    UINT8   nIdx;
    UINT8   nPreSGLIdx = 0;
    UINT8   nTmpRemainScts = 0;
   
    ONLD_DBG_PRINT((TEXT("[ONLD: IN] ++onld_mwrite() nDev = %d, nPsn = %d, nScts = %d, nFlag = 0x%x\r\n"), 
                    nDev, nPsn, nScts, nFlag));

    if (pstSGL == NULL || pErrPsn == NULL)
    {
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (WRITE)Buffer Pointer Error\r\n")));
        ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_mwrite(()\r\n")));
        return (ONLD_ILLEGAL_ACCESS);
    }
                        
    nBAddr = GET_DEV_BADDR(nDev);

    ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  nBAddr = 0x%08x\r\n"), nBAddr));


#if defined(DEFERRED_CHK)
    /* While cache ahead is enabled, the finish cmd should precede */
    if ((GET_DEV_DID(nDev) & 0x0008) == DDP_CHIP || pstPrevOpInfo[nDev]->ePreOp == READ)
    {
		if ((nRes = _ChkPrevOpResult(nDev)) != ONLD_SUCCESS)
		{
            ONLD_ERR_PRINT((TEXT("[ONLD:ERR]  (MWRITE)Previous Operation Error 0x%08x\r\n"), 
                            nRes));
            ONLD_ERR_PRINT((TEXT("[ONLD:ERR]  (MWRITE)Dev = %d, Psn = %d, Scts = %d"),
							nDev,
							pstPrevOpInfo[nDev]->nPsn, 
							pstPrevOpInfo[nDev]->nScts));
			ONLD_DBG_PRINT((TEXT("[ONLD:OUT] --onld_mwrite()\r\n")));
			return nRes;
		}
    }
#endif /* #if defined(DEFERRED_CHK) */
                    
    /* Previous Operation should be flushed at a upper layer */
    
    if ( (nFlag & ONLD_FLAG_ECC_ON) != 0 )
	{
		/* System Configuration Reg set (ECC On)*/
		ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  ECC ON\r\n")));
		ONLD_REG_SYS_CONF1(nBAddr) &= CONF1_ECC_ON;
	}
	else
	{
		/* System Configuration Reg set (ECC Off)*/
		ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  ECC OFF\r\n")));
		ONLD_REG_SYS_CONF1(nBAddr) |= CONF1_ECC_OFF;
	}

    /* Set user Data Buffer point */
    for (nSGLIdx = 0; nSGLIdx < pstSGL->nElements; nSGLIdx++)
    {
        if (pstSGL->stSGLEntry[nSGLIdx].nFlag == SGL_ENTRY_USER_DATA)
        {
            pMBuf = pstSGL->stSGLEntry[nSGLIdx].pBuf;
            break;
        }
    }

    nSGLIdx     = 0;
	nRemainScts = nScts;
	nWritePsn   = nPsn;
	
	switch (nWritePsn & astONLDInfo[nDev].nSctSelSft)
    {
        case 0:     nWriteScts  = (astONLDInfo[nDev].nSctsPerPG > nRemainScts) ? nRemainScts : astONLDInfo[nDev].nSctsPerPG;  break;
        case 1:     nWriteScts  = (astONLDInfo[nDev].nSctSelSft > nRemainScts) ? nRemainScts : astONLDInfo[nDev].nSctSelSft;  break;
        case 2:     nWriteScts  = (2 > nRemainScts) ? nRemainScts : 2;  break;
        case 3:     nWriteScts  = (1 > nRemainScts) ? nRemainScts : 1;  break;
    }

	while (nRemainScts > 0)
	{
		nPbn = GET_PBN(nDev, nWritePsn);

		ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  nPbn = %d\r\n"), nPbn));

        nTmpWriteScts = nWriteScts;
        nSGLCount = 0;

        if (nSctCount != 0 && pstSGL->stSGLEntry[nSGLIdx].nSectors - nSctCount < nTmpWriteScts)
        {
            nTmpRemainScts = pstSGL->stSGLEntry[nSGLIdx].nSectors - nSctCount;
        }
        
        do
        {
            nSctCount += 1;
            if (nSctCount >= pstSGL->stSGLEntry[nSGLIdx].nSectors)
            {
                nSctCount = 0;
                nSGLIdx++;
                nSGLCount++;
            }
            else if (nWriteScts == 1)
            {
                nSGLCount++;
            }            
        } while(--nWriteScts);

        nBiWriteScts = 0;
        nSnapWriteScts = 0;
        		
        ONLD_REG_START_ADDR2(nBAddr) = (UINT16)MAKE_DBS(nPbn);
        
		pONLDMBuf = (UINT32*)GET_ONLD_MBUF_ADDR(nBAddr, nWritePsn, GET_NXT_BUF_SEL(nDev));
		pONLDSBuf = (UINT32*)GET_ONLD_SBUF_ADDR(nBAddr, nWritePsn, GET_NXT_BUF_SEL(nDev));

        for (nIdx = 0; nIdx < nSGLCount; nIdx++)
        {
            switch(pstSGL->stSGLEntry[nPreSGLIdx + nIdx].nFlag)
            {
            case SGL_ENTRY_BISCT_VALID_DATA:
                pSGLBuf        = pstSGL->stSGLEntry[nPreSGLIdx + nIdx].pBuf;
                nBiWriteScts  += pstSGL->stSGLEntry[nPreSGLIdx + nIdx].nSectors;
                nWriteScts     = pstSGL->stSGLEntry[nPreSGLIdx + nIdx].nSectors;
                break;
            case SGL_ENTRY_BISCT_INVALID_DATA:
                pSGLBuf        = pstSGL->stSGLEntry[nPreSGLIdx + nIdx].pBuf;
                nBiWriteScts  += pstSGL->stSGLEntry[nPreSGLIdx + nIdx].nSectors;
                nWriteScts     = pstSGL->stSGLEntry[nPreSGLIdx + nIdx].nSectors;
                break;
            case SGL_ENTRY_META_DATA:
                pSGLBuf         = pstSGL->stSGLEntry[nPreSGLIdx + nIdx].pBuf;
                nSnapWriteScts += pstSGL->stSGLEntry[nPreSGLIdx + nIdx].nSectors;
                nWriteScts      = pstSGL->stSGLEntry[nPreSGLIdx + nIdx].nSectors;
                break;
            case SGL_ENTRY_USER_DATA:
                if (nTmpRemainScts != 0)
                {
                    nWriteScts      = nTmpRemainScts;
                    nTmpRemainScts = 0;
                }
                else
                {
                    nWriteScts      = nTmpWriteScts - nBiWriteScts - nSnapWriteScts;                
                    if (nWriteScts > pstSGL->stSGLEntry[nPreSGLIdx + nIdx].nSectors)
                    {
                        nWriteScts = pstSGL->stSGLEntry[nPreSGLIdx + nIdx].nSectors;                    
                    }
                }
                break;              
            default:
                ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  SGL nFlag Error\r\n")));
                ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_mwrite(()\r\n")));
                return (ONLD_ILLEGAL_ACCESS);               
            }
            if (pstSGL->stSGLEntry[nPreSGLIdx + nIdx].nFlag != SGL_ENTRY_USER_DATA && pSGLBuf != NULL)
            {
    			_WriteMain(pSGLBuf, pONLDMBuf, nWriteScts);
    			pONLDMBuf += (BUFF_MAIN_SIZE * nWriteScts);
            }
            else if (pMBuf != NULL)
            {
    			/* Memcopy for main data */
    			_WriteMain(pMBuf, pONLDMBuf, nWriteScts);
    			pMBuf     += (ONLD_MAIN_SIZE * nWriteScts); 
    			pONLDMBuf += (BUFF_MAIN_SIZE * nWriteScts);    			                
            }       
        }
        
        nPreSGLIdx = nSGLIdx;
        nWriteScts = nTmpWriteScts;
        
		if (pSBuf != NULL)
		{
			/* Memcopy for spare data */
			_WriteSpare(pSBuf, pONLDSBuf, nWriteScts);
			pSBuf += (ONLD_SPARE_SIZE * nWriteScts);
		}
		else
		{
			/* Memset for spare data 0xff*/
            _WriteSpare(aEmptySBuf, pONLDSBuf, nWriteScts);
		}

#if defined(DEFERRED_CHK)        
        if(nRemainScts == nScts) /* at first access */
        {
            if ((nRes = _ChkPrevOpResult(nDev)) != ONLD_SUCCESS)
            {
                *pErrPsn = pstPrevOpInfo[nDev]->nPsn;
                ONLD_ERR_PRINT((TEXT("[ONLD:ERR]  (MWRITE)Previous Operation Error 0x%08x\r\n"), 
                                nRes));
                ONLD_ERR_PRINT((TEXT("[ONLD:ERR]  (MWRITE)Dev = %d, Psn = %d, Scts = %d\r\n"),
                                nDev,
                                pstPrevOpInfo[nDev]->nPsn, 
                                pstPrevOpInfo[nDev]->nScts));
			ONLD_DBG_PRINT((TEXT("[ONLD:OUT] --onld_mwrite()\r\n")));
			return nRes;
		}
        }
        else
        {
            nPbn = GET_PBN(nDev, nCurWritePsn);
            ONLD_REG_START_ADDR2(nBAddr) = (UINT16)MAKE_DBS(nPbn);

            WAIT_ONLD_INT_STAT_CHECKING_PROTECTION(nBAddr, PEND_INT);
                  
            /* Write Operation Error Check */
            if (GET_ONLD_CTRL_STAT(nBAddr, ERROR_STATE) == ERROR_STATE)
            {
                *pErrPsn = nCurWritePsn;
                ONLD_ERR_PRINT((TEXT("[ONLD:ERR]  (MWRITE)Write Error\r\n")));
                ONLD_ERR_PRINT((TEXT("[ONLD:ERR]  (MWRITE)Dev = %d, Psn = %d, Scts = %d\r\n"),
                                nDev,
                                nCurWritePsn, 
                                nCurWritenScts));
                ONLD_DBG_PRINT((TEXT("[ONLD:OUT] --onld_mwrite()\r\n")));
        
                return (ONLD_WRITE_ERROR);
            }
        }
#endif /* #if defined(DEFERRED_CHK) */

        ONLD_REG_START_ADDR2(nBAddr) = (UINT16)MAKE_DBS(nPbn); /* for pair setting of F100h, F101h Reg */
		/* Block Number Set */
		ONLD_REG_START_ADDR1(nBAddr) = (UINT16)MAKE_FBA(nPbn);

		/* Sector Number Set */
        ONLD_REG_START_ADDR8(nBAddr) = (UINT16)(((nWritePsn << astONLDInfo[nDev].nFPASelSft) & MASK_FPA) | (nWritePsn & astONLDInfo[nDev].nFSAMask));

		nBSA = (UINT16)MAKE_BSA(nWritePsn, GET_NXT_BUF_SEL(nDev));

		ONLD_REG_START_BUF(nBAddr)   = (UINT16)((nBSA & MASK_BSA) | (nWriteScts & MASK_BSC));

		/* INT Stat Reg Clear */
		ONLD_REG_INT(nBAddr)         = (UINT16)INT_CLEAR;

		/*-----------------------------------------------------------------------*/
		/* ONLD Write CMD is issued                                              */   
		/*-----------------------------------------------------------------------*/
		if (pMBuf != NULL || pSGLBuf != NULL)
		{
			/* Main Write Command issue */
			ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  Write Page\r\n")));
			ONLD_REG_CMD(nBAddr) = (UINT16)ONLD_CMD_WRITE_PAGE;
		}
		else
		{
			/* Spare Write Command issue */
			ONLD_DBG_PRINT((TEXT("[ONLD : MSG]  Write Spare\r\n")));
			ONLD_REG_CMD(nBAddr) = (UINT16)ONLD_CMD_WRITE_SPARE;
		}



#if !defined(DEFERRED_CHK)
        WAIT_ONLD_INT_STAT_CHECKING_PROTECTION(nBAddr, PEND_INT);
              
        /* Write Operation Error Check */
        if (GET_ONLD_CTRL_STAT(nBAddr, ERROR_STATE) == ERROR_STATE)
        {
            *pErrPsn = nWritePsn;
            ONLD_ERR_PRINT((TEXT("[ONLD:ERR]  (MWRITE)Write Error\r\n")));
            ONLD_DBG_PRINT((TEXT("[ONLD:OUT] --onld_mwrite()\r\n")));
    
            return (ONLD_WRITE_ERROR);
        }
    
   	    ADD_DELAY(nBAddr, DUMMY_READ_CNT);
#endif /* #if !defined(DEFERRED_CHK) */

        nRemainScts -= nWriteScts;
        nCurWritePsn = nWritePsn;
        nCurWritenScts = nWriteScts;
        nWritePsn   += nWriteScts;
        nWriteScts   = (astONLDInfo[nDev].nSctsPerPG > nRemainScts) ? nRemainScts : astONLDInfo[nDev].nSctsPerPG;
        
        pstPrevOpInfo[nDev]->nBufSel    = GET_NXT_BUF_SEL(nDev);
    } /* while (nRemainScts > 0) */

#if defined(DEFERRED_CHK)
        pstPrevOpInfo[nDev]->ePreOp     = WRITE;
        pstPrevOpInfo[nDev]->nPsn       = nCurWritePsn;
        pstPrevOpInfo[nDev]->nScts      = nCurWritenScts;
        pstPrevOpInfo[nDev]->nFlag      = nFlag;
#endif /* #if defined(DEFERRED_CHK) */
    ONLD_DBG_PRINT((TEXT("[ONLD : OUT] --onld_mwrite()\r\n")));    


    return (ONLD_SUCCESS);
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      onld_eraseverify                                                     */
/* DESCRIPTION                                                               */
/*      This function verify a erase operation whether success or not        */
/* PARAMETERS                                                                */
/*      nDev                                                                 */
/*          Physical Device Number (0 ~ 7)                                   */
/*      pstMEArg                                                             */
/*          List of Physical Blocks                                          */
/*      nFlag                                                                */
/*          Operation Option (Sync/Async)                                    */
/* RETURN VALUES                                                             */
/*      ONLD_ERASE_ERROR                                                     */
/*          Erase Failure.                                                   */
/*      ONLD_WRITE_ERROR | ONLD_PREV_OP_RESULT                               */
/*          Previous Write Failure                                           */
/*          DCOP(Deferred Check Operation)                                   */
/*      ONLD_ERASE_ERROR | ONLD_PREV_OP_RESULT                               */
/*          Previous Erase Failure.                                          */
/*          DCOP(Deferred Check Operation)                                   */
/*      ONLD_WR_PROTECT_ERROR | ONLD_PREV_OP_RESULT                          */
/*          Previous Operation Write Protect Error                           */
/*          DCOP(Deferred Check Operation)                                   */
/*      ONLD_SUCCESS                                                         */
/*          Data write success                                               */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
INT32
onld_eraseverify(UINT32 nDev, LLDMEArg *pstMEArg, UINT32 nFlag)
{
    UINT32       nBAddr;
    UINT32       nCnt;  
    INT32        nRet = ONLD_SUCCESS;
    LLDMEList    *pstMEPList; 
    
    ONLD_DBG_PRINT((TEXT("[ONLD: IN] ++Erase Verify nDev = %d\r\n"),nDev));

    nBAddr = GET_DEV_BADDR(nDev);

    ONLD_DBG_PRINT((TEXT("[ONLD:MSG]  nBAddr = 0x%08x\r\n"), nBAddr));                     

#if defined(STRICT_CHK)
    if (pstMEArg == NULL)
    {
        ONLD_ERR_PRINT((TEXT("[ONLD : ERR]  (ERASE_VERIFY)Illeagal Access\r\n")));
        ONLD_DBG_PRINT((TEXT("[ONLD:OUT] --Erase Verify()\r\n"))); 
        return(ONLD_ILLEGAL_ACCESS);
    }
#endif  /* #if defined(STRICT_CHK) */                   
		       
    /* for verify */
    pstMEPList = pstMEArg->pstMEList;

    if((nFlag & ONLD_FLAG_SYNC_MASK) == ONLD_FLAG_SYNC_OP)
    {
        WAIT_ONLD_INT_STAT_CHECKING_PROTECTION(nBAddr, PEND_INT);
        pstPrevOpInfo[nDev]->ePreOp     = NONE;
    }
  
    for(nCnt = 0; nCnt < pstMEArg->nNumOfMList; nCnt++)
    {      
        ONLD_REG_START_ADDR2(nBAddr) = (UINT16)MAKE_DBS(pstMEPList[nCnt].nMEListPbn);
        /* Block Number Set */
        ONLD_REG_START_ADDR1(nBAddr) = (UINT16)MAKE_FBA(pstMEPList[nCnt].nMEListPbn);
        
        /* Erase Verify Command issue */
        ONLD_DBG_PRINT((TEXT("[ONLD:MSG]  MErase Verify\r\n")));
        
         /* INT Stat Reg Clear */
        ONLD_REG_INT(nBAddr)         = (UINT16)INT_CLEAR;
        
        ONLD_REG_CMD(nBAddr)        = (UINT16)ONLD_CMD_ERASE_VERIFY;
               
        ONLD_DBG_PRINT((TEXT("[ONLD:MSG]  Block number nPbn = %d\r\n"), pstMEPList[nCnt].nMEListPbn));

        /* Wait until device ready */
        WAIT_ONLD_INT_STAT(nBAddr, PEND_INT); 
                
        /* Erase Operation Verifying Error Check */
        if (GET_ONLD_CTRL_STAT(nBAddr, ERROR_STATE) == ERROR_STATE)
        {
            ONLD_ERR_PRINT((TEXT("[ONLD:ERR]  Erase Verifying Error(nPbn = %d)\r\n"), pstMEPList[nCnt].nMEListPbn));
            pstMEArg->nBitMapErr |= (1 << nCnt);
            nRet = LLD_MERASE_ERROR;
        }
    }
    
    ONLD_DBG_PRINT((TEXT("[ONLD:OUT] --Erase Verify()\r\n")));    

    return (nRet);
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      ONLD_GetPlatformInfo                                                 */
/* DESCRIPTION                                                               */
/*      This function returns platform information                           */
/* PARAMETERS                                                                */
/*      pstLLDPlantformInfo                                                  */
/*          Platform information                                             */
/* RETURN VALUES                                                             */
/*****************************************************************************/
	VOID
onld_getplatforminfo(LLDPlatformInfo* pstLLDPlatformInfo)
{
	/* Warning : Do not change !!!! */
	/* 0 : OneNAND, 1 : read ID from NAND directly, 2 : read ID from register of NAND controller */
	pstLLDPlatformInfo->nType = 0;
	/* Address of command register */
	pstLLDPlatformInfo->nAddrOfCmdReg = astONLDDev[0].BaseAddr + 0x1E440;
	/* Address of address register */
	pstLLDPlatformInfo->nAddrOfAdrReg = astONLDDev[0].BaseAddr + 0x1E200;
	/* Address of register for reading ID*/
	pstLLDPlatformInfo->nAddrOfReadIDReg = astONLDDev[0].BaseAddr + 0x1E000;
	/* Address of status register */
	pstLLDPlatformInfo->nAddrOfStatusReg = astONLDDev[0].BaseAddr + 0x1E480;
	/* Command of reading Device ID  */
	pstLLDPlatformInfo->nCmdOfReadID = (UINT32) NULL;
	/* Command of read page */
	pstLLDPlatformInfo->nCmdOfReadPage = (UINT32) NULL;
	/* Command of read status */
	pstLLDPlatformInfo->nCmdOfReadStatus = (UINT32) NULL;
	/* Mask value for Ready or Busy status */
	pstLLDPlatformInfo->nMaskOfRnB = (UINT32) NULL;

	return;
}
