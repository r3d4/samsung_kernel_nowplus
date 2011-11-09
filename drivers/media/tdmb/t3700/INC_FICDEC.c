#include "INC_INCLUDES.h"


INC_UINT8				g_bFIC_OK;
ST_FIC_DB				g_astFicDb;
ST_UTC_INFO				g_stUTC_Info;

INC_UINT8 INC_CHECK_SERVICE_DB16(INC_UINT16* ptr, INC_UINT16 wVal, INC_UINT16 wNum)
{
    INC_INT32 nLoop;
    INC_UINT8 ucDbNum = 0;

    for(nLoop = 0; nLoop < wNum ; nLoop++) {
        if(ptr[nLoop] != wVal) ucDbNum++;
    }
    return ucDbNum;
}

INC_UINT8 INC_CHECK_SERVICE_DB8(INC_UINT8* ptr, INC_UINT8 cVal, INC_UINT8 cNum) 
{
    INC_UINT8 ucLoop, ucDbNum = 0;
    for(ucLoop = 0; ucLoop < cNum ; ucLoop++) {
        if(ptr[ucLoop] != cVal)
            ucDbNum++;
    }
    return ucDbNum;
}

INC_INT16 INC_CHECK_SERVICE_CNT16(INC_UINT16* ptr, INC_UINT16 wVal, INC_UINT8 cNum, INC_UINT16 wMask) 
{
    INC_INT16 cLoop;
    for(cLoop = 0; cLoop < cNum ; cLoop++) {
        if((ptr[cLoop] & wMask) == (wVal & wMask))
            return cLoop;
    }
    return -1;
}

INC_INT16 INC_CHECK_SERVICE_CNT8(INC_UINT8* ptr, INC_UINT8 cVal, INC_UINT8 cNum, INC_UINT8 cMask) 
{
    INC_INT16 cLoop;
    for(cLoop = 0; cLoop < cNum ; cLoop++) {
        if((*(ptr+cLoop) & cMask) == (cVal & cMask))
            return cLoop;
    }
    return -1;
}

INC_UINT8 INC_GET_BYTEDATA(ST_FIB_INFO* pFibInfo)
{
    return pFibInfo->aucBuff[pFibInfo->ucDataPos++] & 0xff;
}

INC_UINT8 INC_GETAT_BYTEDATA(ST_FIB_INFO* pFibInfo)
{
    return pFibInfo->aucBuff[pFibInfo->ucDataPos] & 0xff;
}

INC_UINT16 INC_GET_WORDDATA(ST_FIB_INFO* pFibInfo)
{
    INC_UINT16 uiData;
    uiData = (((INC_UINT16)pFibInfo->aucBuff[pFibInfo->ucDataPos++] << 8) & 0xff00);
    uiData |= ((INC_UINT16)pFibInfo->aucBuff[pFibInfo->ucDataPos++] & 0x00ff);
    return (uiData & 0xffff);
}

INC_UINT16 INC_GETAT_WORDDATA(ST_FIB_INFO* pFibInfo)
{
    INC_UINT16 uiData;
    uiData = (((INC_UINT16)pFibInfo->aucBuff[pFibInfo->ucDataPos] << 8) & 0xff00) |
   		((INC_UINT16)pFibInfo->aucBuff[pFibInfo->ucDataPos+1] & 0x00ff);
    return (uiData & 0xffff);
}

INC_UINT32 INC_GET_LONGDATA(ST_FIB_INFO* pFibInfo)
{
    INC_UINT32 ulMsb, ulLsb;

    ulMsb = (INC_UINT32)INC_GET_WORDDATA(pFibInfo);
    ulLsb = (INC_UINT32)INC_GET_WORDDATA(pFibInfo);
    return (ulMsb << 16 | ulLsb);
}

INC_UINT32 INC_GETAT_LONGDATA(ST_FIB_INFO* pFibInfo)
{
    INC_UINT32 ulMsb, ulLsb;
    ulMsb = (INC_UINT32)INC_GETAT_WORDDATA(pFibInfo);
    pFibInfo->ucDataPos += 2;
    ulLsb = (INC_UINT32)INC_GETAT_WORDDATA(pFibInfo);
    pFibInfo->ucDataPos -= 2;
    return (ulMsb << 16 | ulLsb);
}

INC_UINT8 INC_GET_HEADER(ST_FIB_INFO* pInfo)
{
    return pInfo->aucBuff[pInfo->ucDataPos++];
}

INC_UINT8 INC_GETAT_HEADER(ST_FIB_INFO* pInfo)
{
    return pInfo->aucBuff[pInfo->ucDataPos];
}

INC_UINT8 INC_GET_TYPE(ST_FIB_INFO* pInfo)
{
    return pInfo->aucBuff[pInfo->ucDataPos++];
}

INC_UINT8 INC_GETAT_TYPE(ST_FIB_INFO* pInfo)
{
    return pInfo->aucBuff[pInfo->ucDataPos+1];
}

INC_UINT8 INC_GET_NULLBLOCK(ST_FIG_HEAD* pInfo)
{
    if(pInfo->ucInfo == END_MARKER) return INC_ERROR;
    return INC_SUCCESS;
}

INC_UINT8 INC_GET_FINDLENGTH(ST_FIG_HEAD* pInfo)
{
    if(!pInfo->ITEM.bitLength || pInfo->ITEM.bitLength > FIB_SIZE-3) 
        return INC_ERROR;
    return INC_SUCCESS;
}

INC_UINT16 INC_CRC_CHECK(INC_UINT8 *pBuf, INC_UINT8 ucSize) 
{
    INC_UINT8	ucLoop;
    INC_UINT16	nCrc = 0xFFFF;
	
    for(ucLoop = 0; ucLoop < ucSize; ucLoop++){
        nCrc  = 0xFFFF & (((nCrc<<8) | (nCrc>>8)) ^ (0xFF & pBuf[ucLoop]));                        
        nCrc  = nCrc ^ ((0xFF & nCrc) >> 4);
        nCrc  = 0xFFFF & (nCrc ^ ( (((((0xFF & nCrc))<<8)|(((0xFF & nCrc))>>8))) << 4) ^ ((0xFF & nCrc) << 5));
    }
	
    return( (INC_UINT16) 0xFFFF & (nCrc ^ 0xFFFF));
}

INC_UINT16 INC_GET_BITRATE(SCH_DB_STRT * pstSchDb)
{
    INC_UINT16 uiBitRate = 0;

    if(!pstSchDb->ucShortLong){
        if(pstSchDb->ucTableIndex <= 4)  uiBitRate = 32;
        else if(pstSchDb->ucTableIndex >= 5  && pstSchDb->ucTableIndex <= 9)  uiBitRate = 48;
        else if(pstSchDb->ucTableIndex >= 10 && pstSchDb->ucTableIndex <= 13) uiBitRate = 56;
        else if(pstSchDb->ucTableIndex >= 14 && pstSchDb->ucTableIndex <= 18) uiBitRate = 64;
        else if(pstSchDb->ucTableIndex >= 19 && pstSchDb->ucTableIndex <= 23) uiBitRate = 80;
        else if(pstSchDb->ucTableIndex >= 24 && pstSchDb->ucTableIndex <= 28) uiBitRate = 96;
        else if(pstSchDb->ucTableIndex >= 29 && pstSchDb->ucTableIndex <= 32) uiBitRate = 112;
        else if(pstSchDb->ucTableIndex >= 33 && pstSchDb->ucTableIndex <= 37) uiBitRate = 128;
        else if(pstSchDb->ucTableIndex >= 38 && pstSchDb->ucTableIndex <= 42) uiBitRate = 160;
        else if(pstSchDb->ucTableIndex >= 43 && pstSchDb->ucTableIndex <= 47) uiBitRate = 192;
        else if(pstSchDb->ucTableIndex >= 48 && pstSchDb->ucTableIndex <= 52) uiBitRate = 224;
        else if(pstSchDb->ucTableIndex >= 53 && pstSchDb->ucTableIndex <= 57) uiBitRate = 256;
        else if(pstSchDb->ucTableIndex >= 58 && pstSchDb->ucTableIndex <= 60) uiBitRate = 320;
        else if(pstSchDb->ucTableIndex >= 61 && pstSchDb->ucTableIndex <= 63) uiBitRate = 384;
        else uiBitRate = 0;
    }
    else{
        if(pstSchDb->ucOption == OPTION_INDICATE0) {
            switch(pstSchDb->ucProtectionLevel){
            case PROTECTION_LEVEL0: uiBitRate = (pstSchDb->uiSubChSize/12)*8;break;
            case PROTECTION_LEVEL1: uiBitRate = (pstSchDb->uiSubChSize/8)*8;break;
            case PROTECTION_LEVEL2: uiBitRate = (pstSchDb->uiSubChSize/6)*8;break;
            case PROTECTION_LEVEL3: uiBitRate = (pstSchDb->uiSubChSize/4)*8;break;
            }
        }
        else if(pstSchDb->ucOption == OPTION_INDICATE1){
            switch(pstSchDb->ucProtectionLevel){
            case PROTECTION_LEVEL0: uiBitRate = (pstSchDb->uiSubChSize/27)*32;break;
            case PROTECTION_LEVEL1: uiBitRate = (pstSchDb->uiSubChSize/21)*32;break;
            case PROTECTION_LEVEL2: uiBitRate = (pstSchDb->uiSubChSize/18)*32;break;
            case PROTECTION_LEVEL3: uiBitRate = (pstSchDb->uiSubChSize/15)*32;break;
            }
        }
    }
    return uiBitRate;
}

INC_UINT16 INC_FIND_SUBCH_SIZE(INC_UINT8 ucTableIndex) 
{
    INC_UINT16 wSubCHSize = 0;

    switch(ucTableIndex)
    {
    case 0:	wSubCHSize = 16; break;
    case 1: wSubCHSize = 21; break;
    case 2: wSubCHSize = 24; break;
    case 3: wSubCHSize = 29; break;
    case 4: wSubCHSize = 35; break;
    case 5: wSubCHSize = 24; break;
    case 6: wSubCHSize = 29; break;
    case 7: wSubCHSize = 35; break;
    case 8: wSubCHSize = 42; break;
    case 9: wSubCHSize = 52; break;
    case 10: wSubCHSize = 29; break;
    case 11: wSubCHSize = 35; break;
    case 12: wSubCHSize = 42; break;
    case 13: wSubCHSize = 52; break;
    case 14: wSubCHSize = 32; break;
    case 15: wSubCHSize = 42; break;  
    case 16: wSubCHSize = 48; break;
    case 17: wSubCHSize = 58; break;
    case 18: wSubCHSize = 70; break;
    case 19: wSubCHSize = 40; break;
    case 20: wSubCHSize = 52; break;
    case 21: wSubCHSize = 58; break;
    case 22: wSubCHSize = 70; break;
    case 23: wSubCHSize = 84; break;
    case 24: wSubCHSize = 48; break;
    case 25: wSubCHSize = 58; break;
    case 26: wSubCHSize = 70; break;
    case 27: wSubCHSize = 84; break;
    case 28: wSubCHSize = 104; break;
    case 29: wSubCHSize = 58; break;
    case 30: wSubCHSize = 70; break;
    case 31: wSubCHSize = 84; break;
    case 32: wSubCHSize = 104; break;
    case 33: wSubCHSize = 64; break;  
    case 34: wSubCHSize = 84; break;
    case 35: wSubCHSize = 96; break;
    case 36: wSubCHSize = 116; break;
    case 37: wSubCHSize = 140; break;
    case 38: wSubCHSize = 80; break;
    case 39: wSubCHSize = 104; break;
    case 40: wSubCHSize = 116; break;
    case 41: wSubCHSize = 140; break;
    case 42: wSubCHSize = 168; break;
    case 43: wSubCHSize = 96; break;
    case 44: wSubCHSize = 116; break;
    case 45: wSubCHSize = 140; break;
    case 46: wSubCHSize = 168; break;
    case 47: wSubCHSize = 208; break;
    case 48: wSubCHSize = 116; break;
    case 49: wSubCHSize = 140; break;
    case 50: wSubCHSize = 168; break;
    case 51: wSubCHSize = 208; break;  
    case 52: wSubCHSize = 232; break;
    case 53: wSubCHSize = 128; break;
    case 54: wSubCHSize = 168; break;
    case 55: wSubCHSize = 192; break;
    case 56: wSubCHSize = 232; break;
    case 57: wSubCHSize = 280; break;
    case 58: wSubCHSize = 160; break;
    case 59: wSubCHSize = 208; break;
    case 60: wSubCHSize = 280; break;
    case 61: wSubCHSize = 192; break;
    case 62: wSubCHSize = 280; break;
    case 63: wSubCHSize = 416; break;
    }
    return wSubCHSize;
}

void INC_SET_SHORTFORM(ST_FIC_DB* pstFicDB, INC_INT32 nCh, ST_TYPE0of1Short_INFO* pShort)
{
    SCH_DB_STRT* pSch_db;

    pSch_db = &pstFicDB->astSchDb[nCh];
    pSch_db->ucShortLong = pShort->ITEM.bitShortLong;

    if(pSch_db->ucShortLong)	return;

    pSch_db->ucTableSW = pShort->ITEM.bitTableSw;
    pSch_db->ucTableIndex = pShort->ITEM.bitTableIndex;
    pSch_db->ucOption = 0;
    pSch_db->ucProtectionLevel = 0;
    pSch_db->uiSubChSize = INC_FIND_SUBCH_SIZE(pSch_db->ucTableIndex);
    pSch_db->uiDifferentRate  = 0;

    pSch_db->uiBitRate = INC_GET_BITRATE(pSch_db);
}

void INC_SET_LONGFORM(ST_FIC_DB* pstFicDB, INC_INT32 nCh, ST_TYPE0of1Long_INFO* pLong)
{
    SCH_DB_STRT* pSch_db;
	
    pSch_db = &pstFicDB->astSchDb[nCh];
    pSch_db->ucShortLong = pLong->ITEM.bitShortLong;

    if(!pSch_db->ucShortLong)	return;

    pSch_db->ucTableSW = 0;
    pSch_db->ucTableIndex = 0;
    pSch_db->ucOption = pLong->ITEM.bitOption;
    pSch_db->ucProtectionLevel = pLong->ITEM.bitProtecLevel;
    pSch_db->uiSubChSize = pLong->ITEM.bitSubChanSize;

    if(pSch_db->ucOption == 0 ){
        switch(pSch_db->ucProtectionLevel) {
        case  0 : pSch_db->uiDifferentRate = (pSch_db->uiSubChSize/12); break;
        case  1 : pSch_db->uiDifferentRate = (pSch_db->uiSubChSize/8); break;
        case  2 : pSch_db->uiDifferentRate = (pSch_db->uiSubChSize/6); break;
        case  3 : pSch_db->uiDifferentRate = (pSch_db->uiSubChSize/4); break;
        default : pSch_db->uiDifferentRate = 0; break; 
        }
    }
    else {
        switch(pSch_db->ucProtectionLevel) {
        case  0 : pSch_db->uiDifferentRate = (pSch_db->uiSubChSize/27); break;
        case  1 : pSch_db->uiDifferentRate = (pSch_db->uiSubChSize/21); break;
        case  2 : pSch_db->uiDifferentRate = (pSch_db->uiSubChSize/18); break;
        case  3 : pSch_db->uiDifferentRate = (pSch_db->uiSubChSize/15); break;
        default : pSch_db->uiDifferentRate = 0; break; 
        }
    }
    pSch_db->uiBitRate = INC_GET_BITRATE(pSch_db);
}

void INC_EXTENSION_000(ST_FIB_INFO* pFibInfo, ST_FIC_DB* pstFicDB)
{
    ST_FIG_HEAD*			pstHeader;
    ST_TYPE_0*				pstType;
    ST_TYPE0of0_INFO*		pBitStarm;
    INC_UINT32 ulBitStram = 0;

    pstHeader = (ST_FIG_HEAD*)&pFibInfo->aucBuff[pFibInfo->ucDataPos++];
    (void)pstHeader;  // smy_cg20
    pstType = (ST_TYPE_0*)&pFibInfo->aucBuff[pFibInfo->ucDataPos++];
    (void)pstType;

    ulBitStram = INC_GET_LONGDATA(pFibInfo);
    pBitStarm = (ST_TYPE0of0_INFO*)&ulBitStram;
    pstFicDB->uiEnsembleID = pBitStarm->ITEM.bitEld;

    if(pBitStarm->ITEM.bitChangFlag) pFibInfo->ucDataPos += 1;
}

void INC_EXTENSION_001(ST_FIB_INFO* pFibInfo, ST_FIC_DB* pstFicDB)
{
    ST_FIG_HEAD*			pstHeader;
    ST_TYPE_0*				pstType;
    ST_TYPE0of1Short_INFO*	pShortInfo = INC_NULL;
    ST_TYPE0of1Long_INFO*	pLongInfo = INC_NULL;

    INC_UINT32 ulTypeInfo;
    INC_UINT16 uiStartAddr;
    INC_UINT8 ucSubChId, ucLoop, ucIndex, ucShortLongFlag, ucIsSubCh;

    INC_UINT16 uSubCHTick = 0;

    pstHeader = (ST_FIG_HEAD*)&pFibInfo->aucBuff[pFibInfo->ucDataPos++];
    pstType = (ST_TYPE_0*)&pFibInfo->aucBuff[pFibInfo->ucDataPos++];
    (void)pstType;

    pstFicDB->ucSubChCnt = INC_CHECK_SERVICE_DB8(pstFicDB->aucSubChID, INIT_FIC_DB08, MAX_SUBCHANNEL);

    for(ucIndex = 0; ucIndex < pstHeader->ITEM.bitLength-1;)
    {
        uSubCHTick++;
        ucShortLongFlag = (pFibInfo->aucBuff[pFibInfo->ucDataPos+2] >> 7) & 0x01;

        if(ucShortLongFlag == 0)
        {
            ulTypeInfo = INC_GET_LONGDATA(pFibInfo);
            pShortInfo = (ST_TYPE0of1Short_INFO*)&ulTypeInfo;
            pFibInfo->ucDataPos -= 1; 

            ucSubChId = pShortInfo->ITEM.bitSubChId;
            uiStartAddr = pShortInfo->ITEM.bitStartAddr;
        }
        else
        {	
            ulTypeInfo = INC_GET_LONGDATA(pFibInfo);
            pLongInfo = (ST_TYPE0of1Long_INFO*)&ulTypeInfo;

            ucSubChId = pLongInfo->ITEM.bitSubChId;
            uiStartAddr = pLongInfo->ITEM.bitStartAddr;
        }

        ucIsSubCh = 0;
        for(ucLoop = 0 ; ucLoop < pstFicDB->ucSubChCnt; ucLoop++)
        {
            if(((pstFicDB->aucSubChID[ucLoop] & BIT_MASK) == (ucSubChId & BIT_MASK)) 
         				|| (pstFicDB->auiStartAddr[ucLoop] == uiStartAddr)) 
            {
                if(ucShortLongFlag == 0) INC_SET_SHORTFORM(pstFicDB, ucLoop, pShortInfo);
                else INC_SET_LONGFORM(pstFicDB, ucLoop, pLongInfo);
			
                ucIsSubCh = 1;
                break;
            }
        }

        if(!ucIsSubCh) 
        {
            pstFicDB->auiStartAddr[pstFicDB->ucSubChCnt] = uiStartAddr;
            pstFicDB->aucSubChID[pstFicDB->ucSubChCnt] = ucSubChId;

            if(ucShortLongFlag == 0) INC_SET_SHORTFORM(pstFicDB, pstFicDB->ucSubChCnt, pShortInfo);
            else INC_SET_LONGFORM(pstFicDB, pstFicDB->ucSubChCnt, pLongInfo);
	
            pstFicDB->ucSubChCnt++;
        }

        ucIndex += (!ucShortLongFlag) ? 3 : 4;
    }

    if(pstFicDB->ucSubChCnt)
        g_bFIC_OK = 1;
}

void INC_EXTENSION_002(ST_FIB_INFO* pFibInfo, ST_FIC_DB* pstFicDB)
{
    ST_FIG_HEAD*			pstHeader;
    ST_TYPE_0*				pstType;
    ST_SERVICE_COMPONENT*	pService;
    ST_TMId_MSCnFIDC*		pMscnFidc;
    ST_MSC_PACKET_INFO*		pMscPacket;

    INC_UINT32 ulServiceId;
    INC_UINT16 uiData;
    INC_INT16 nChanPos;
    INC_UINT8 ucService, ucLoop, ucIndex, ucFrame, ucTMID, ucSubCnt;

    INC_UINT32 uServiceTick = 0;

    pstHeader = (ST_FIG_HEAD*)&pFibInfo->aucBuff[pFibInfo->ucDataPos++];
    pstType = (ST_TYPE_0*)&pFibInfo->aucBuff[pFibInfo->ucDataPos++];

    for(ucFrame = 0 ; ucFrame < pstHeader->ITEM.bitLength-1; )
    {
        uServiceTick++;

        if(pstType->ITEM.bitPD == 0) {
            ulServiceId = (INC_UINT32)INC_GET_WORDDATA(pFibInfo);
            ucFrame += 2;
        }
        else {
            ulServiceId = INC_GET_LONGDATA(pFibInfo);
            ucFrame += 4;
        }

        ucService = INC_GET_BYTEDATA(pFibInfo);
        ucFrame += 1;
        pService = (ST_SERVICE_COMPONENT*)&ucService;

        for(ucLoop = 0; ucLoop < pService->ITEM.bitNumComponent; ucLoop++, ucFrame+=2)
        {
            uiData = INC_GET_WORDDATA(pFibInfo);
            ucTMID = (INC_UINT8)(uiData >> 14) & 0x0003;

            switch(ucTMID) 
            {
            case TMID_0 :
            case TMID_1 :
            case TMID_2 :
                pMscnFidc = (ST_TMId_MSCnFIDC*)&uiData;
                nChanPos = INC_CHECK_SERVICE_CNT8(pstFicDB->aucSubChID, (INC_UINT8)pMscnFidc->ITEM.bitSubChld, pstFicDB->ucSubChCnt, BIT_MASK);

                if(nChanPos != -1)
                {
                    if(pMscnFidc->ITEM.bitTMId == TMID_0) 
                        pstFicDB->aucSubChID[nChanPos] &= 0x7f;

                    pstFicDB->aulServiceID[nChanPos] = ulServiceId;
                    pstFicDB->aucDSCType[nChanPos] = (INC_UINT8)pMscnFidc->ITEM.bitAscDscTy;
                    pstFicDB->uiSubChOk |= 1 << nChanPos;
                    pstFicDB->aucTmID[nChanPos] = (INC_UINT8)pMscnFidc->ITEM.bitTMId;
                }
                break;

            default :
                pMscPacket = (ST_MSC_PACKET_INFO*)&uiData;

                for(ucIndex = 0; ucIndex < (INC_INT32)pstFicDB->ucServiceComponentCnt; ucIndex++){
                    if((pstFicDB->auiServicePackMode[ucIndex] & 0xfff) != pMscPacket->ITEM.bitSCId) continue;
                    for(ucSubCnt = 0 ; ucSubCnt < pstFicDB->ucSubChCnt ; ucSubCnt++)
                    {
                        if((pstFicDB->aucSubChID[ucSubCnt] & BIT_MASK) != pstFicDB->aucSubChPackMode[ucIndex])  continue;

                        pstFicDB->aulServiceID[ucSubCnt] = ulServiceId;
                        pstFicDB->aucDSCType[ucSubCnt] = pstFicDB->aucServiceTypePackMode[ucIndex];
                        pstFicDB->aucSetPackAddr[ucSubCnt] = pstFicDB->auiPacketAddr[ucIndex];
						
                        pstFicDB->uiSubChOk |= 1 << ucSubCnt;
                        pstFicDB->aucTmID[ucSubCnt] = TMID_3;
                    }
                }
                break;
            }
        }
    }
}

void INC_EXTENSION_003(ST_FIB_INFO* pFibInfo, ST_FIC_DB* pstFicDB)
{
    ST_FIG_HEAD*			pstHeader;
    ST_TYPE_0*				pstType;
    ST_TYPE0of3_INFO*		pTypeInfo;
    ST_TYPE0of3Id_INFO*		pIdInfo;

    INC_UINT32 uiId, uiTypeInfo;
    INC_UINT8 ucLoop, ucIsSubCh, ucIndex;

    pstHeader = (ST_FIG_HEAD*)&pFibInfo->aucBuff[pFibInfo->ucDataPos++];
    pstType = (ST_TYPE_0*)&pFibInfo->aucBuff[pFibInfo->ucDataPos++];
    (void)pstType;

    for(ucIndex = 0 ; ucIndex < pstHeader->ITEM.bitLength-1; )
    {
        uiTypeInfo = INC_GET_WORDDATA(pFibInfo);
        pTypeInfo = (ST_TYPE0of3_INFO*)&uiTypeInfo;

        uiId = INC_GET_WORDDATA(pFibInfo);
        uiId = (uiId << 16) | (INC_GET_BYTEDATA(pFibInfo)<<8);
        pIdInfo = (ST_TYPE0of3Id_INFO*)&uiId;

        ucIndex += 5;

        pstFicDB->ucServiceComponentCnt = INC_CHECK_SERVICE_DB16(pstFicDB->auiServicePackMode, 0xffff, MAX_SUBCHANNEL); 
        ucIsSubCh = INC_ERROR;

        for(ucLoop = 0 ; ucLoop < pstFicDB->ucServiceComponentCnt ; ucLoop++)
        {
            if(((pstFicDB->auiServicePackMode[ucLoop] & BIT_MASK) == (pTypeInfo->ITEM.bitScid & BIT_MASK)) && 
         				((pstFicDB->aucSubChPackMode[ucLoop] & BIT_MASK) == ((INC_UINT8)pIdInfo->ITEM.bitSubChId & BIT_MASK))) 
            {
                ucIsSubCh = INC_SUCCESS;
                break;
            }
        }

        if(!ucIsSubCh)
        {
            pstFicDB->auiServicePackMode[pstFicDB->ucServiceComponentCnt] = pTypeInfo->ITEM.bitScid;
            pstFicDB->aucServiceTypePackMode[pstFicDB->ucServiceComponentCnt] = (INC_UINT8)pIdInfo->ITEM.bitDScType;
            pstFicDB->aucSubChPackMode[pstFicDB->ucServiceComponentCnt] = (INC_UINT8)pIdInfo->ITEM.bitSubChId;
            pstFicDB->auiPacketAddr[pstFicDB->ucServiceComponentCnt] = pIdInfo->ITEM.bitPacketAddr;
            pstFicDB->ucServiceComponentCnt++; 
        }

        if(pTypeInfo->ITEM.bitSccaFlag == 1){
            ucIndex += 2;
            pFibInfo->ucDataPos += 2;
        }
    }
}

void INC_EXTENSION_008(ST_FIB_INFO* pFibInfo, ST_FIC_DB* pstFicDB)
{
    ST_FIG_HEAD*			pstHeader;
    ST_TYPE_0*				pstType;
    ST_MSC_BIT*				pstMsc;
    ST_MSC_SHORT*			pstMscShort;
    ST_MSC_LONG*			pstMscLong;

    INC_UINT16 uiMsgBit;
    INC_UINT32 ulSerId = 0;
    INC_UINT8 ucMscInfo, ucIndex, ucLSFlag;
    INC_INT16 cResult;

    pstHeader = (ST_FIG_HEAD*)&pFibInfo->aucBuff[pFibInfo->ucDataPos++];
    pstType = (ST_TYPE_0*)&pFibInfo->aucBuff[pFibInfo->ucDataPos++];

    for(ucIndex = 0 ; ucIndex < pstHeader->ITEM.bitLength-1; )
    {
        if(!pstType->ITEM.bitPD){
            ulSerId = (INC_UINT32)INC_GET_WORDDATA(pFibInfo);
            ucIndex += 2;
        }
        else {
            ulSerId = INC_GET_LONGDATA(pFibInfo);
            ucIndex += 4;
        }

        ucMscInfo = INC_GET_BYTEDATA(pFibInfo); 
        pstMsc = (ST_MSC_BIT*)&ucMscInfo;
        ucIndex += 1;
        ucLSFlag = (INC_GETAT_BYTEDATA(pFibInfo) >> 7) & 0x01;

        if(ucLSFlag) {
            uiMsgBit = INC_GET_WORDDATA(pFibInfo);
            ucIndex += 2;
            pstMscLong = (ST_MSC_LONG*)&uiMsgBit;

            cResult = INC_CHECK_SERVICE_CNT16(pstFicDB->auiServicePackMode, 
         				pstMscLong->ITEM.bitScId, pstFicDB->ucServiceComponentCnt, 0xfff);
            if(cResult != -1){
                cResult = INC_CHECK_SERVICE_CNT8(pstFicDB->aucSubChID, 
            					pstFicDB->aucSubChPackMode[cResult], pstFicDB->ucSubChCnt, BIT_MASK);
                if(cResult != -1) {
                    pstFicDB->aucServiceComponID[cResult] = pstMsc->ITEM.bitScIds;
                    pstFicDB->aulServiceID[cResult] = ulSerId;
                }
            }
        }
        else{
            uiMsgBit = (INC_UINT16)INC_GET_BYTEDATA(pFibInfo);
            ucIndex += 1;
            pstMscShort = (ST_MSC_SHORT*)&uiMsgBit;
            if(!pstMscShort->ITEM.bitMscFicFlag){
                cResult = INC_CHECK_SERVICE_CNT8(pstFicDB->aucSubChID, 
            					pstMscShort->ITEM.bitSUBnFIDCId, pstFicDB->ucSubChCnt, BIT_MASK);
                if(cResult >= 0) {
                    pstFicDB->aucServiceComponID[cResult] = pstMsc->ITEM.bitScIds;
                    pstFicDB->aulServiceID[cResult] = ulSerId;
                }
            }
        }

        if(pstMsc->ITEM.bitExtFlag) {
            ucIndex += 1;
            pFibInfo->ucDataPos += 1;
        }
    }
}

void INC_EXTENSION_010(ST_FIB_INFO* pFibInfo, ST_FIC_DB* pstFicDB)
{
    ST_FIG_HEAD*		pstHeader;
    ST_TYPE_0*			pstType;
    ST_UTC_SHORT_INFO*	pstUTC_Short_Info;
    ST_UTC_LONG_INFO*	pstUTC_Long_Info;
    ST_UTC_INFO*		pstUTC_Info;
	
    INC_UINT32		ulUtcInfo;
    INC_UINT16		uiUtcLongForm;

    pstHeader = (ST_FIG_HEAD*)&pFibInfo->aucBuff[pFibInfo->ucDataPos++];
    pstType = (ST_TYPE_0*)&pFibInfo->aucBuff[pFibInfo->ucDataPos++];
    (void)pstHeader;
    (void)pstType;

    ulUtcInfo = INC_GET_LONGDATA(pFibInfo);
    pstUTC_Short_Info = (ST_UTC_SHORT_INFO*)&ulUtcInfo;

    pstUTC_Info = (ST_UTC_INFO*)&g_stUTC_Info;

    pstUTC_Info->uiHours = pstUTC_Short_Info->ITEM.bitHours;
    pstUTC_Info->uiMinutes = pstUTC_Short_Info->ITEM.bitMinutes;
    pstUTC_Info->ucUTC_Flag = pstUTC_Short_Info->ITEM.bitUTC_Flag;

    if(pstUTC_Info->ucUTC_Flag){
        uiUtcLongForm = INC_GET_WORDDATA(pFibInfo);
        pstUTC_Long_Info = (ST_UTC_LONG_INFO*)&uiUtcLongForm;

        pstUTC_Info->uiSeconds = pstUTC_Long_Info->ITEM.bitSeconds;
        pstUTC_Info->uiMilliseconds = pstUTC_Long_Info->ITEM.bitMilliseconds;
    }
    pstUTC_Info->ucGet_Time = 1;
}

#ifdef USER_APPLICATION_TYPE
void INC_EXTENSION_013(ST_FIB_INFO* pFibInfo, ST_FIC_DB* pstFicDB)
{
    ST_FIG_HEAD*		pstHeader;
    ST_TYPE_0*			pstType;
    ST_USER_APP_IDnNUM*	pUserAppIdNum;
    ST_USER_APPTYPE*	pUserAppType;
    INC_UINT32	ulSid;
    INC_UINT16	uiUserAppType;
    INC_UINT8	ucHeader, ucType, ucUserAppIdNum;
    INC_UINT8	ucFrame, ucIndex, ucLoop, ucDataCnt;

    ucHeader = INC_GET_HEADER(pFibInfo);
    pstHeader = (ST_FIG_HEAD*)&ucHeader;
	
    ucType = INC_GET_TYPE(pFibInfo);
    pstType = (ST_TYPE_0*)&ucType;


    for(ucFrame = 0 ; ucFrame < pstHeader->ITEM.bitLength-1; ){

        if(pstType->ITEM.bitPD){
            ulSid = INC_GET_LONGDATA(pFibInfo);
            ucFrame += 4;
        }
        else {
            ulSid = INC_GET_WORDDATA(pFibInfo);
            ucFrame += 2;
        }
		
        ucUserAppIdNum = INC_GET_BYTEDATA(pFibInfo);
        pUserAppIdNum = (ST_USER_APP_IDnNUM*)&ucUserAppIdNum;
        ucFrame += 1;
        pstFicDB->astUserAppInfo.ucSCIdS = pUserAppIdNum->ITEM.bitSCIdS;
        pstFicDB->astUserAppInfo.ucNomOfUserApp = pUserAppIdNum->ITEM.bitNomUserApp;

        for(ucIndex = 0; ucIndex < pstFicDB->astUserAppInfo.ucNomOfUserApp; ucIndex++){
            uiUserAppType = INC_GET_WORDDATA(pFibInfo);
            pUserAppType = (ST_USER_APPTYPE*)&uiUserAppType;
            ucFrame += 2;

            for(ucLoop = 0; ucLoop < pstFicDB->ucSubChCnt; ucLoop++)
            {
                if(ulSid != pstFicDB->aulServiceID[ucLoop]) continue;

                pstFicDB->astUserAppInfo.uiUserAppType[ucLoop] = pUserAppType->ITEM.bitUserAppType;
                pstFicDB->astUserAppInfo.ucUserAppDataLength[ucLoop] = pUserAppType->ITEM.bitUserDataLength;
				
                for(ucDataCnt = 0; ucDataCnt < pstFicDB->astUserAppInfo.ucUserAppDataLength[ucLoop]; ucDataCnt++){
                    pstFicDB->astUserAppInfo.aucUserAppData[ucLoop][ucDataCnt] = INC_GET_BYTEDATA(pFibInfo);
                    ucFrame += 1;
                }
				
                if(pstFicDB->aucTmID[ucLoop] == TMID_3)
                    pstFicDB->uiUserAppTypeOk |= 1 << ucLoop;
                break;
            }
        }
    }
}
#endif


INC_UINT16 INC_SET_FICTYPE_0(ST_FIB_INFO* pFibInfo, ST_FIC_DB* pstFicDB)
{
    ST_FIG_HEAD* pHeader;
    ST_TYPE_0*   pExtern;
    INC_UINT8 ucHeader, ucType;
	
    ucHeader = INC_GETAT_HEADER(pFibInfo);
    pHeader = (ST_FIG_HEAD*)&ucHeader;

    ucType = INC_GETAT_TYPE(pFibInfo);
    pExtern = (ST_TYPE_0*)&ucType;

    switch(pExtern->ITEM.bitExtension)
    {
    case EXTENSION_0: INC_EXTENSION_000(pFibInfo, pstFicDB); break;
    case EXTENSION_1: INC_EXTENSION_001(pFibInfo, pstFicDB); break;
    case EXTENSION_2: INC_EXTENSION_002(pFibInfo, pstFicDB); break;
    case EXTENSION_3: INC_EXTENSION_003(pFibInfo, pstFicDB); break;
    case EXTENSION_8: INC_EXTENSION_008(pFibInfo, pstFicDB); break;
    case EXTENSION_10: INC_EXTENSION_010(pFibInfo, pstFicDB); break;
#ifdef USER_APPLICATION_TYPE
    case EXTENSION_13: INC_EXTENSION_013(pFibInfo, pstFicDB); break;
#endif
    default: pFibInfo->ucDataPos += pHeader->ITEM.bitLength + 1; break;
    }
    return INC_SUCCESS;
}

void INC_EXTENSION_110(ST_FIB_INFO* pFibInfo, ST_FIC_DB* pstFicDB)
{
    ST_FIG_HEAD*	pHeader;
    ST_TYPE_1*		pType;
    INC_UINT8		ucLoop;
    INC_UINT16		uiEId;

    pHeader = (ST_FIG_HEAD*)&pFibInfo->aucBuff[pFibInfo->ucDataPos++];
    pType	= (ST_TYPE_1*)&pFibInfo->aucBuff[pFibInfo->ucDataPos++];
    (void)pHeader;
    (void)pType;

    uiEId = INC_GET_WORDDATA(pFibInfo);

    for(ucLoop = 0; ucLoop < MAX_LABEL_CHAR ; ucLoop++){
        pstFicDB->aucEnsembleLabel[ucLoop] = INC_GET_BYTEDATA(pFibInfo);
    }
    pFibInfo->ucDataPos += 2;
    if(uiEId == pstFicDB->uiEnsembleID)	
        pstFicDB->uiSubChInfoOk |= 1 << 15;
}

void INC_EXTENSION_111(ST_FIB_INFO* pFibInfo, ST_FIC_DB* pstFicDB)
{
    ST_FIG_HEAD*	pHeader;
    ST_TYPE_1*		pType;

    INC_UINT16 uiSId;
    INC_UINT8 ucLoop, ucIndex, ucIsLable = 0;

    pHeader = (ST_FIG_HEAD*)&pFibInfo->aucBuff[pFibInfo->ucDataPos++];
    pType = (ST_TYPE_1*)&pFibInfo->aucBuff[pFibInfo->ucDataPos++];
    (void)pHeader;
    (void)pType;

    uiSId = INC_GET_WORDDATA(pFibInfo);
    for(ucLoop = 0 ; ucLoop < (INC_UINT8)pstFicDB->ucSubChCnt ; ucLoop++) 
    {
        if(uiSId == (INC_UINT16)pstFicDB->aulServiceID[ucLoop] && pstFicDB->aucServiceExtension[ucLoop] == 0xff) 
        {
            for(ucIndex = 0 ; ucIndex < MAX_LABEL_CHAR ; ucIndex++) {
                pstFicDB->aucServiceLabel[ucLoop][ucIndex] = INC_GET_BYTEDATA(pFibInfo);
            }

            pstFicDB->uiSubChInfoOk |= 1 << ucLoop;
            pFibInfo->ucDataPos += 2;
            ucIsLable = INC_SUCCESS;
        }
    }
    if(!ucIsLable) pFibInfo->ucDataPos += 18;
}

void INC_EXTENSION_112(ST_FIB_INFO* pFibInfo)
{
    ST_FIG_HEAD*			pHeader;
    ST_TYPE_1*				pType;
    ST_EXTENSION_TYPE12*	pType12;
    INC_UINT8 ucData;

    pHeader = (ST_FIG_HEAD*)&pFibInfo->aucBuff[pFibInfo->ucDataPos++];
    pType = (ST_TYPE_1*)&pFibInfo->aucBuff[pFibInfo->ucDataPos++];
    (void)pHeader;
    (void)pType;

    ucData = INC_GET_BYTEDATA(pFibInfo);
    pType12 = (ST_EXTENSION_TYPE12*)&ucData;

    if(pType12->ITEM.bitCF_flag) pFibInfo->ucDataPos += 1;

    pFibInfo->ucDataPos += 19;
    if(pType12->ITEM.bitCountry == 1) 
        pFibInfo->ucDataPos += 2;
}

void INC_EXTENSION_113(ST_FIB_INFO* pFibInfo)
{
    ST_FIG_HEAD*			pHeader;
    ST_TYPE_1*				pType;
	
    pHeader = (ST_FIG_HEAD*)&pFibInfo->aucBuff[pFibInfo->ucDataPos++];
    pType = (ST_TYPE_1*)&pFibInfo->aucBuff[pFibInfo->ucDataPos++];
    (void)pHeader;
    (void)pType;
    
    pFibInfo->ucDataPos += 19;
}

void INC_EXTENSION_114(ST_FIB_INFO* pFibInfo, ST_FIC_DB* pstFicDB)
{
    ST_FIG_HEAD*		pHeader;
    ST_TYPE_1*			pType;
    ST_EXTENSION_TYPE14* pExtenType;
    INC_UINT32 ulSId;
    INC_UINT8 ucData, ucLoop, ucIndex, ucIsLable = 0;

    pHeader = (ST_FIG_HEAD*)&pFibInfo->aucBuff[pFibInfo->ucDataPos++];
    pType = (ST_TYPE_1*)&pFibInfo->aucBuff[pFibInfo->ucDataPos++];
    (void)pHeader;
    (void)pType;

    ucData = INC_GET_BYTEDATA(pFibInfo);
    pExtenType = (ST_EXTENSION_TYPE14*)&ucData;

    if(!pExtenType->ITEM.bitPD) 
        ulSId = (INC_UINT32)INC_GET_WORDDATA(pFibInfo); 
    else 
        ulSId = INC_GET_LONGDATA(pFibInfo);

    for(ucLoop = 0 ; ucLoop < (INC_UINT8)pstFicDB->ucSubChCnt ; ucLoop++)
    {
        if(ulSId == pstFicDB->aulServiceID[ucLoop] && 
			pExtenType->ITEM.bitSCidS == pstFicDB->aucServiceComponID[ucLoop])
        {
            for(ucIndex = 0 ; ucIndex < MAX_LABEL_CHAR ; ucIndex++) {
                pstFicDB->aucServiceLabel[ucLoop][ucIndex] = INC_GET_BYTEDATA(pFibInfo);
            }
            
            pstFicDB->uiSubChInfoOk |= 1 << ucLoop;
            pFibInfo->ucDataPos += 2;
            ucIsLable = INC_SUCCESS;
        }
    }
    if(!ucIsLable) pFibInfo->ucDataPos += 18;
}

void INC_EXTENSION_115(ST_FIB_INFO* pFibInfo, ST_FIC_DB* pstFicDB)
{
    ST_FIG_HEAD*		pHeader;
    ST_TYPE_1*			pType;
    INC_UINT32 ulSId;
    INC_UINT8 ucLoop, ucIndex, ucIsLable = 0;
	
    pHeader = (ST_FIG_HEAD*)&pFibInfo->aucBuff[pFibInfo->ucDataPos++];
    pType = (ST_TYPE_1*)&pFibInfo->aucBuff[pFibInfo->ucDataPos++];
    (void)pHeader;
    (void)pType;

    ulSId = INC_GET_LONGDATA(pFibInfo);
    for(ucLoop = 0 ; ucLoop < (INC_UINT8)pstFicDB->ucSubChCnt ; ucLoop++)
    { 
        if(ulSId == pstFicDB->aulServiceID[ucLoop] && pstFicDB->aucServiceExtension[ucLoop] == 0xff){

            for(ucIndex = 0; ucIndex < MAX_LABEL_CHAR; ucIndex++) {
                pstFicDB->aucServiceLabel[ucLoop][ucIndex] = INC_GET_BYTEDATA(pFibInfo);
            }

            pstFicDB->uiSubChInfoOk |= 1 << ucLoop;
            pFibInfo->ucDataPos += 2;
            ucIsLable = INC_SUCCESS;
        }
    }
    if(!ucIsLable) pFibInfo->ucDataPos += 18;
}

void INC_SET_FICTYPE_1(ST_FIB_INFO* pFibInfo, ST_FIC_DB* pstFicDB)
{
    ST_TYPE_1* pExtern;
    ST_FIG_HEAD* pHeader;
    INC_UINT8 ucType, ucHeader, ucLoop, ucIndex;
	
    ucHeader = INC_GETAT_HEADER(pFibInfo);
    ucType = INC_GETAT_TYPE(pFibInfo);

    pHeader = (ST_FIG_HEAD*)&ucHeader;
    pExtern = (ST_TYPE_1*)&ucType;

    for(ucLoop = 0 ; ucLoop < (INC_UINT8)pstFicDB->ucSubChCnt ; ucLoop++) {
        for(ucIndex = (ucLoop + 1) ; ucIndex < (INC_UINT8)pstFicDB->ucSubChCnt ; ucIndex++) {
            if((pstFicDB->aulServiceID[ucLoop] == pstFicDB->aulServiceID[ucIndex]) && 
         				(pstFicDB->aulServiceID[ucLoop] != 0xffffffff )) {
                pstFicDB->aucServiceExtension[ucLoop] = 1;
                pstFicDB->aucServiceExtension[ucIndex] = 1;
            }
        }
    }

    switch(pExtern->ITEM.bitExtension){
    case EXTENSION_0: INC_EXTENSION_110(pFibInfo, pstFicDB); break;
    case EXTENSION_1: INC_EXTENSION_111(pFibInfo, pstFicDB); break;
    case EXTENSION_2: INC_EXTENSION_112(pFibInfo); break;
    case EXTENSION_3: INC_EXTENSION_113(pFibInfo); break;
    case EXTENSION_4: INC_EXTENSION_114(pFibInfo, pstFicDB); break;
    case EXTENSION_5: INC_EXTENSION_115(pFibInfo, pstFicDB); break;
    default: pFibInfo->ucDataPos += (pHeader->ITEM.bitLength + 1); break;
    }
}

void INC_SET_UPDATEFIC(ST_FIB_INFO* pstDestData, INC_UINT8* pSourData)
{
    INC_UINT8 cuIndex;
    INC_UINT16 wCRC, wCRCData;
    for(cuIndex = 0; cuIndex < FIB_SIZE; cuIndex++) 
        pstDestData->aucBuff[cuIndex] = pSourData[cuIndex];

    wCRC = INC_CRC_CHECK((INC_UINT8*)pstDestData->aucBuff, FIB_SIZE-2);
    wCRCData = ((INC_UINT16)pstDestData->aucBuff[30] << 8) | pstDestData->aucBuff[31];

    pstDestData->ucDataPos = 0;
    pstDestData->uiIsCRC = wCRC == wCRCData;
}

INC_UINT8 INC_GET_FINDTYPE(ST_FIG_HEAD* pInfo)
{
    if(pInfo->ITEM.bitType <= FIG_IN_HOUSE) return INC_SUCCESS;
    return INC_ERROR;
}

INC_UINT8 INC_FICPARSING(INC_UINT8 ucI2CID, INC_UINT8* pucFicBuff, INC_INT32 uFicLength, ST_SIMPLE_FIC bSimpleFIC)
{
    ST_FIC 			stFIC;
    ST_FIB_INFO* 	pstFib;
    ST_FIG_HEAD* 	pHeader;
    ST_FIC_DB* 		pFicDb;
	
    INC_UINT8 ucLoop, ucHeader;
    INC_UINT16 uiTempIndex = 0;

    pFicDb = INC_GETFIC_DB(ucI2CID);
    stFIC.ucBlockNum = uFicLength/FIB_SIZE;
    pstFib = &stFIC.stBlock;

    for(ucLoop = 0; ucLoop < stFIC.ucBlockNum; ucLoop++)
    {
        INC_SET_UPDATEFIC(pstFib, &pucFicBuff[ucLoop*FIB_SIZE]);
		
        if(!pstFib->uiIsCRC)
            continue;

        while(pstFib->ucDataPos < FIB_SIZE-2)
        {
            ucHeader = INC_GETAT_HEADER(pstFib);
            pHeader = (ST_FIG_HEAD*)&ucHeader;

            if(!INC_GET_FINDTYPE(pHeader) || !INC_GET_NULLBLOCK(pHeader) || !INC_GET_FINDLENGTH(pHeader)) break;

            switch(pHeader->ITEM.bitType) {
            case FIG_MCI_SI			: INC_SET_FICTYPE_0(pstFib, pFicDb);break;
            case FIG_LABLE			: INC_SET_FICTYPE_1(pstFib, pFicDb);break;
            case FIG_RESERVED_0		: 
            case FIG_RESERVED_1		: 
            case FIG_RESERVED_2		: 
            case FIG_FICDATA_CHANNEL:
            case FIG_CONDITION_ACCESS:
            case FIG_IN_HOUSE		: 
            default					: pstFib->ucDataPos += pHeader->ITEM.bitLength + 1;break;
            }

            if(pstFib->ucDataPos == FIB_SIZE-1) break;
        }
    }

    if(stFIC.ucBlockNum && stFIC.ucBlockNum == ucLoop){
        uiTempIndex = (1 << pFicDb->ucSubChCnt) - 1;
        uiTempIndex |= 0x8000;

        if(bSimpleFIC == SIMPLE_FIC_ENABLE && g_bFIC_OK){
            return INC_SUCCESS;
        }

        if((uiTempIndex == pFicDb->uiSubChInfoOk) && pFicDb->ucSubChCnt )
            return INC_SUCCESS;

    }

    return INC_ERROR;
}

void INC_INITDB(INC_UINT8 ucI2CID) 
{
    INC_UINT8 ucLoop;
    SCH_DB_STRT* astSchDb;
    ST_FIC_DB* pFicDb;
    ST_UTC_INFO*	pstUTC_Info;

    pFicDb = INC_GETFIC_DB(ucI2CID);
    pstUTC_Info = (ST_UTC_INFO*)&g_stUTC_Info;

    g_bFIC_OK = 0;

    memset(pFicDb->aucEnsembleLabel, 0, sizeof(INC_UINT8)*MAX_LABEL_CHAR);
    memset(pFicDb->aucServiceLabel, 0, sizeof(INC_UINT8)*MAX_LABEL_CHAR*MAX_SUBCHANNEL);
	
    pstUTC_Info->ucGet_Time = 0;
	
    pFicDb->ucSubChCnt = 0;
    pFicDb->uiEnsembleID = 0;
    pFicDb->uiSubChOk = 0;
    pFicDb->uiSubChInfoOk = 0;
    pFicDb->ucServiceComponentCnt = 0; 

    pFicDb->uiUserAppTypeOk = 0;
#ifdef USER_APPLICATION_TYPE	
    pFicDb->astUserAppInfo.ucNomOfUserApp = 0;
#endif


    for(ucLoop = 0 ; ucLoop < MAX_SUBCHANNEL ; ucLoop++){              
        astSchDb = &pFicDb->astSchDb[ucLoop];
        memset(astSchDb, -1, sizeof(SCH_DB_STRT));

        pFicDb->aucDSCType[ucLoop] 				= 0xff;
        pFicDb->aucSubChID[ucLoop] 				= 0xff;
        pFicDb->aucTmID[ucLoop] 				= 0xff;
        pFicDb->auiStartAddr[ucLoop] 			= 0xffff;
        pFicDb->aulServiceID[ucLoop] 			= 0xffffffff;
        pFicDb->auiServicePackMode[ucLoop] 		= 0xffff;
        pFicDb->aucSubChPackMode[ucLoop] 		= 0xff;
        pFicDb->aucServiceTypePackMode[ucLoop] 	= 0xff;
		
        pFicDb->aucServiceComponID[ucLoop] 		= 0xff;
        pFicDb->aucServiceExtension[ucLoop] 	= 0xff;
        pFicDb->auiUserAppType[ucLoop] 			= 0xffff;
#ifdef USER_APPLICATION_TYPE	
        pFicDb->astUserAppInfo.uiUserAppType[ucLoop] = 0xffff;
        pFicDb->astUserAppInfo.ucUserAppDataLength[ucLoop] = 0;
#endif

    }
}

ST_FIC_DB* INC_GETFIC_DB(INC_UINT8 ucI2CID)
{
    return &g_astFicDb;
}

INC_UINT8 INC_FIC_UPDATE(INC_UINT8 ucI2CID, INC_CHANNEL_INFO* pChInfo)
{
    INC_INT16	nLoop = 0;
    ST_FIC_DB*	pstFicDb;
	
    pstFicDb = INC_GETFIC_DB(ucI2CID);
	
    for(nLoop = 0; nLoop < pstFicDb->ucSubChCnt; nLoop++)
    {
        if(pChInfo->ucSubChID != (pstFicDb->aucSubChID[nLoop]&0x7F)) 
            continue;
		
        pChInfo->uiStarAddr			= pstFicDb->auiStartAddr[nLoop];
        pChInfo->uiBitRate			= pstFicDb->astSchDb[nLoop].uiBitRate;
        pChInfo->ucSlFlag			= pstFicDb->astSchDb[nLoop].ucShortLong;
        pChInfo->ucTableIndex		= pstFicDb->astSchDb[nLoop].ucTableIndex;
        pChInfo->ucOption			= pstFicDb->astSchDb[nLoop].ucOption;
        pChInfo->ucProtectionLevel	= pstFicDb->astSchDb[nLoop].ucProtectionLevel;
        pChInfo->uiDifferentRate	= pstFicDb->astSchDb[nLoop].uiDifferentRate;
        pChInfo->uiSchSize			= pstFicDb->astSchDb[nLoop].uiSubChSize;
        break;
    }
	
    if(nLoop == pstFicDb->ucSubChCnt){
        return INC_ERROR;
    }
    return INC_SUCCESS;
}


