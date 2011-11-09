/*****************************************************************************/
/*                                                                           */
/* PROJECT : ANYSTORE II                                                     */
/* MODULE  : LLD                                                             */
/* NAME    : OneNAND Low-level Driver header file                            */
/* FILE    : ONLDReg.h                                                       */
/* PURPOSE : Register, Command Set, and Local Definitions for OneNAND        */
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
/* - 19/MAY/2003 [Janghwan Kim]  : First writing                             */
/* - 06/OCT/2003 [Janghwan Kim]  : Reorganization                            */
/* - 18/JAN/2006 [WooYoung Yang] : Add OneNAND SysConfig Reg Value           */
/*                                                                           */
/*****************************************************************************/

#ifndef _ONENAND_REGISTER_H_
#define _ONENAND_REGISTER_H_


/*****************************************************************************/
/* OneNAND Base Address Definitions                                          */
/*****************************************************************************/
#define REG_BASE_ADDR(x)            ((x) + 0x0001E000)


/*****************************************************************************/
/* OneNAND Register Address Definitions                                      */
/*****************************************************************************/
#define ONLD_REG_MANUF_ID(x)        (*(volatile UINT16*)(REG_BASE_ADDR(x) + 0x0000))
#define ONLD_REG_DEV_ID(x)          (*(volatile UINT16*)(REG_BASE_ADDR(x) + 0x0002))
#define ONLD_REG_VER_ID(x)          (*(volatile UINT16*)(REG_BASE_ADDR(x) + 0x0004))
#define ONLD_REG_DATABUF_SIZE(x)    (*(volatile UINT16*)(REG_BASE_ADDR(x) + 0x0006))
#define ONLD_REG_BOOTBUF_SIZE(x)    (*(volatile UINT16*)(REG_BASE_ADDR(x) + 0x0008))
#define ONLD_REG_BUF_AMOUNT(x)      (*(volatile UINT16*)(REG_BASE_ADDR(x) + 0x000A))
#define ONLD_REG_TECH(x)            (*(volatile UINT16*)(REG_BASE_ADDR(x) + 0x000C))

#define ONLD_REG_START_ADDR1(x)     (*(volatile UINT16*)(REG_BASE_ADDR(x) + 0x0200))
#define ONLD_REG_START_ADDR2(x)     (*(volatile UINT16*)(REG_BASE_ADDR(x) + 0x0202))
#define ONLD_REG_START_ADDR3(x)     (*(volatile UINT16*)(REG_BASE_ADDR(x) + 0x0204))
#define ONLD_REG_START_ADDR4(x)     (*(volatile UINT16*)(REG_BASE_ADDR(x) + 0x0206))
#define ONLD_REG_START_ADDR5(x)     (*(volatile UINT16*)(REG_BASE_ADDR(x) + 0x0208))
#define ONLD_REG_START_ADDR6(x)     (*(volatile UINT16*)(REG_BASE_ADDR(x) + 0x020A))
#define ONLD_REG_START_ADDR7(x)     (*(volatile UINT16*)(REG_BASE_ADDR(x) + 0x020C))
#define ONLD_REG_START_ADDR8(x)     (*(volatile UINT16*)(REG_BASE_ADDR(x) + 0x020E))

#define ONLD_REG_START_BUF(x)       (*(volatile UINT16*)(REG_BASE_ADDR(x) + 0x0400))

#define ONLD_REG_CMD(x)             (*(volatile UINT16*)(REG_BASE_ADDR(x) + 0x0440))
#define ONLD_REG_SYS_CONF1(x)       (*(volatile UINT16*)(REG_BASE_ADDR(x) + 0x0442))
#define ONLD_REG_SYS_CONF2(x)       (*(volatile UINT16*)(REG_BASE_ADDR(x) + 0x0444))

#define ONLD_REG_CTRL_STAT(x)       (*(volatile UINT16*)(REG_BASE_ADDR(x) + 0x0480))
#define ONLD_REG_INT(x)             (*(volatile UINT16*)(REG_BASE_ADDR(x) + 0x0482))

#define ONLD_REG_ULOCK_START_BA(x)  (*(volatile UINT16*)(REG_BASE_ADDR(x) + 0x0498))
#define ONLD_REG_ULOCK_END_BA(x)    (*(volatile UINT16*)(REG_BASE_ADDR(x) + 0x049A))
#define ONLD_REG_WR_PROTECT_STAT(x) (*(volatile UINT16*)(REG_BASE_ADDR(x) + 0x049C))

#define ONLD_REG_ECC_STAT(x)        (*(volatile UINT16*)(REG_BASE_ADDR(x) + 0x1E00))
#define ONLD_REG_ECC_RSLT_MB0(x)    (*(volatile UINT16*)(REG_BASE_ADDR(x) + 0x1E02))
#define ONLD_REG_ECC_RSLT_SB0(x)    (*(volatile UINT16*)(REG_BASE_ADDR(x) + 0x1E04))
#define ONLD_REG_ECC_RSLT_MB1(x)    (*(volatile UINT16*)(REG_BASE_ADDR(x) + 0x1E06))
#define ONLD_REG_ECC_RSLT_SB1(x)    (*(volatile UINT16*)(REG_BASE_ADDR(x) + 0x1E08))
#define ONLD_REG_ECC_RSLT_MB2(x)    (*(volatile UINT16*)(REG_BASE_ADDR(x) + 0x1E0A))
#define ONLD_REG_ECC_RSLT_SB2(x)    (*(volatile UINT16*)(REG_BASE_ADDR(x) + 0x1E0C))
#define ONLD_REG_ECC_RSLT_MB3(x)    (*(volatile UINT16*)(REG_BASE_ADDR(x) + 0x1E0E))
#define ONLD_REG_ECC_RSLT_SB3(x)    (*(volatile UINT16*)(REG_BASE_ADDR(x) + 0x1E10))


/*****************************************************************************/
/* OneNAND Main Buffer Address                                               */
/*****************************************************************************/
#define ONLD_BT_MB0_ADDR(x)         ((x) + 0x00000)
#define ONLD_BT_MB1_ADDR(x)         ((x) + 0x00200)
#define ONLD_DT_MB00_ADDR(x)        ((x) + 0x00400)
#define ONLD_DT_MB01_ADDR(x)        ((x) + 0x00600)
#define ONLD_DT_MB02_ADDR(x)        ((x) + 0x00800)
#define ONLD_DT_MB03_ADDR(x)        ((x) + 0x00A00)
#define ONLD_DT_MB10_ADDR(x)        ((x) + 0x00C00)
#define ONLD_DT_MB11_ADDR(x)        ((x) + 0x00E00)
#define ONLD_DT_MB12_ADDR(x)        ((x) + 0x01000)
#define ONLD_DT_MB13_ADDR(x)        ((x) + 0x01200)

#define ONLD_BT_SB0_ADDR(x)         ((x) + 0x10000)
#define ONLD_BT_SB1_ADDR(x)         ((x) + 0x10010)
#define ONLD_DT_SB00_ADDR(x)        ((x) + 0x10020)
#define ONLD_DT_SB01_ADDR(x)        ((x) + 0x10030)
#define ONLD_DT_SB02_ADDR(x)        ((x) + 0x10040)
#define ONLD_DT_SB03_ADDR(x)        ((x) + 0x10050)
#define ONLD_DT_SB10_ADDR(x)        ((x) + 0x10060)
#define ONLD_DT_SB11_ADDR(x)        ((x) + 0x10070)
#define ONLD_DT_SB12_ADDR(x)        ((x) + 0x10080)
#define ONLD_DT_SB13_ADDR(x)        ((x) + 0x10090)


/*****************************************************************************/
/* OneNAND Register Masking values                                           */
/*****************************************************************************/
#define MASK_DFS                    0x8000
#define MASK_FBA                    0x7FFF
#define MASK_DBS                    0x8000
#define MASK_FCBA                   0x7FFF
#define MASK_FCPA                   0x00FC
#define MASK_FCSA                   0x0003
#define MASK_FPA                    0x00FC
#define MASK_BSA                    0x0F00
#define MASK_BSC                    0x0003
#define MASK_FPC                    0x00FF

/*****************************************************************************/
/* OneNAND Start Buffer Register Setting values                              */
/*****************************************************************************/
#define BSA_BT_BUF0                 0x0000
#define BSA_BT_BUF1                 0x0100
#define BSA_DT_BUF00                0x0800
#define BSA_DT_BUF01                0x0900

#define BSA_DT_BUF02                0x0A00
#define BSA_DT_BUF03                0x0B00
#define BSA_DT_BUF10                0x0C00
#define BSA_DT_BUF11                0x0D00
#define BSA_DT_BUF12                0x0E00
#define BSA_DT_BUF13                0x0F00

#define BSC_1_SECTOR                0x0001
#define BSC_2_SECTOR                0x0002
#define BSC_3_SECTOR                0x0003
#define BSC_4_SECTOR                0x0000

/*****************************************************************************/
/* OneNAND Command Set                                                       */
/*****************************************************************************/
#define ONLD_CMD_READ_PAGE          0x0000
#define ONLD_CMD_CACHE_READ			0x000E
#define ONLD_CMD_FINISH_CACHE_READ  0x000C

#define ONLD_CMD_READ_SPARE         0x0013
#define ONLD_CMD_WRITE_PAGE         0x0080
#define ONLD_CMD_WRITE_SPARE        0x001A
#define ONLD_CMD_CPBACK_PRGM        0x001B
#define ONLD_CMD_ULOCK_NAND         0x0023
#define ONLD_CMD_ERASE_RESUME		0x0030
#define ONLD_CMD_ERASE_VERIFY		0x0071
#define ONLD_CMD_ERASE_BLK          0x0094
#define ONLD_CMD_ERASE_MBLK			0x0095
#define ONLD_CMD_ERASE_SUSPEND		0x00B0
#define ONLD_CMD_RST_NF_CORE        0x00F0
#define ONLD_CMD_RESET              0x00F3
#define ONLD_CMD_OTP_ACCESS         0x0065

#define ONLD_CMD_SYNC_BURST_BREAD   0x000A
#define ONLD_CMD_LOCK_TIGHT_BLK     0x002C
#define ONLD_CMD_LOCK_BLK           0x002A
/*****************************************************************************/
/* OneNAND System Configureation1 Register Values                            */
/*****************************************************************************/
#define SYNC_READ_MODE              0x8000
#define ASYNC_READ_MODE             0x0000

#define BST_RD_LATENCY_8            0x0000      /*   N/A   */
#define BST_RD_LATENCY_9            0x1000      /*   N/A   */
#define BST_RD_LATENCY_10           0x2000      /*   N/A   */
#define BST_RD_LATENCY_3            0x3000      /*   min   */
#define BST_RD_LATENCY_4            0x4000      /* default */
#define BST_RD_LATENCY_5            0x5000      
#define BST_RD_LATENCY_6            0x6000
#define BST_RD_LATENCY_7            0x7000

#define BST_LENGTH_CONT             0x0000      /* default */
#define BST_LENGTH_4WD              0x0200
#define BST_LENGTH_8WD              0x0400
#define BST_LENGTH_16WD             0x0600
#define BST_LENGTH_32WD             0x0800      /* N/A on spare */
#define BST_LENGTH_1KWD             0x0A00      /* N/A on spare, sync. burst block read only */

#define CONF1_ECC_ON                0xFEFF
#define CONF1_ECC_OFF               0x0100      //(~CONF1_ECC_ON)   //0x0100

#define RDY_POLAR                   0x0080
#define INT_POLAR                   0x0040
#define IOBE_ENABLE                 0x0020

#define BWPS_UNLOCKED               0x0001

#define HF_ON                0x0004
#define HF_OFF               0xFFFB
#define RDY_CONF             0x0010

/*****************************************************************************/
/* OneNAND Controller Status Register Values                                 */
/*****************************************************************************/
#define CTRL_ONGO                   0x8000
#define LOCK_STATE                  0x4000
#define LOAD_STATE                  0x2000
#define PROG_STATE                  0x1000
#define ERASE_STATE					0x0800
#define ERROR_STATE					0x0400
#define SUSPEND_STATE				0x0200
#define RESET_STATE					0x0080
#define OTPL_STATE					0x0040
#define OTPBL_STATE					0x0020
#define TIME_OUT                    0x0001

#define PROG_LOCK					(LOCK_STATE | PROG_STATE | ERROR_STATE)
#define ERASE_LOCK					(LOCK_STATE | ERASE_STATE | ERROR_STATE)
#define PROG_FAIL					(PROG_STATE | ERROR_STATE)
#define ERASE_FAIL					(ERASE_STATE | ERROR_STATE)

/*****************************************************************************/
/* OneNAND Interrupt Status Register Values                                  */
/*****************************************************************************/
#define INT_MASTER                  0x8000
#define INT_CLEAR                   0x0000
#define INT_MASK                    0xFFFF

#define INT_READ                    0x0080
#define INT_WRITE                   0x0040
#define INT_ERASE                   0x0020
#define INT_RESET                   0x0010

#define PEND_INT                    (INT_MASTER)
#define PEND_READ                   (INT_MASTER | INT_READ)
#define PEND_WRITE                  (INT_MASTER | INT_WRITE)
#define PEND_ERASE                  (INT_MASTER | INT_ERASE)
#define PEND_RESET                  (INT_MASTER | INT_RESET)


/*****************************************************************************/
/* OneNAND Write Protection Status Register Values                           */
/*****************************************************************************/
#define UNLOCKED_STAT               0x0004
#define LOCKED_STAT                 0x0002
#define LOCK_TIGHTEN_STAT           0x0001


/*****************************************************************************/
/* OneNAND ECC Status Register Valuies                                       */
/*****************************************************************************/
#define ECC_SB0_NO_ERR              0x0000
#define ECC_SB0_1BIT_ERR            0x0001
#define ECC_SB0_2BIT_ERR            0x0002
#define ECC_MB0_NO_ERR              0x0000
#define ECC_MB0_1BIT_ERR            0x0004
#define ECC_MB0_2BIT_ERR            0x0008

#define ECC_SB1_NO_ERR              0x0000
#define ECC_SB1_1BIT_ERR            0x0010
#define ECC_SB1_2BIT_ERR            0x0020
#define ECC_MB1_NO_ERR              0x0000
#define ECC_MB1_1BIT_ERR            0x0040
#define ECC_MB1_2BIT_ERR            0x0080

#define ECC_SB2_NO_ERR              0x0000
#define ECC_SB2_1BIT_ERR            0x0100
#define ECC_SB2_2BIT_ERR            0x0200
#define ECC_MB2_NO_ERR              0x0000
#define ECC_MB2_1BIT_ERR            0x0400
#define ECC_MB2_2BIT_ERR            0x0800

#define ECC_SB3_NO_ERR              0x0000
#define ECC_SB3_1BIT_ERR            0x1000
#define ECC_SB3_2BIT_ERR            0x2000
#define ECC_MB3_NO_ERR              0x0000
#define ECC_MB3_1BIT_ERR            0x4000
#define ECC_MB3_2BIT_ERR            0x8000

#define ECC_ANY_2BIT_ERR            0xAAAA
#define ECC_ANY_BIT_ERR             0xFFFF
#define ECC_MAIN_BIT_ERR            0xCCCC
#define ECC_SPARE_BIT_ERR           0x3333
#define ECC_REG_CLEAR               0x0000


/*****************************************************************************/
/* OneNAND Misc Values                                                       */
/*****************************************************************************/
#define SECTOR0                     0x0000
#define SECTOR1                     0x0001
#define SECTOR2                     0x0002
#define SECTOR3                     0x0003

#define DATA_BUF0                   0x0000
#define DATA_BUF1                   0x0001

#define SECTOR0_OFFSET              0x0000
#define SECTOR1_OFFSET              0x0200
#define SECTOR2_OFFSET              0x0400
#define SECTOR3_OFFSET              0x0600

#define VALID_BLK_MARK              0xFFFF

#endif /* _ONENAND_REGISTER_H_ */

