
/**
   %COPYRIGHT%
 */

/**
 * @version     LinuStoreIII_1.2.0_b038-FSR_1.2.1p1_b139_RTM
 * @file        FSR_PAM_Memcpy.c
 * @brief       This file contain the Platform Adaptation Modules for Poseidon
 *
 */


#include "FSR.h"

/*****************************************************************************/
/* Function Implementation                                                   */
/*****************************************************************************/

/**
 * @brief           memcpy
 *
 * @return          none
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 *
 */
PUBLIC VOID
memcpy32 (VOID       *pDst,
          VOID       *pSrc,
          UINT32     nSize)
{
    UINT32   nIdx;
    UINT32  *pSrc32;
    UINT32  *pDst32;
    UINT32   nSize32;


    pSrc32  = (UINT32 *)(pSrc);
    pDst32  = (UINT32 *)(pDst);
    nSize32 = nSize / sizeof (UINT32);

    for(nIdx = 0; nIdx < nSize32; nIdx++)
    {
        pDst32[nIdx] = pSrc32[nIdx];
    }
}
