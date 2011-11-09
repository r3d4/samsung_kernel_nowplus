#include "INC_INCLUDES.h"


#include <linux/fs.h>
#include <asm/uaccess.h>

static ST_BBPINFO			g_astBBPRun[2];
extern ENSEMBLE_BAND		m_ucRfBand;
extern UPLOAD_MODE_INFO		m_ucUploadMode;
extern CLOCK_SPEED			m_ucClockSpeed;
extern INC_ACTIVE_MODE		m_ucMPI_CS_Active;
extern INC_ACTIVE_MODE		m_ucMPI_CLK_Active;
extern CTRL_MODE 			m_ucCommandMode;
extern ST_TRANSMISSION		m_ucTransMode;
extern PLL_MODE				m_ucPLL_Mode;
extern INC_DPD_MODE			m_ucDPD_Mode;
extern INC_UINT16			m_unIntCtrl;


INC_UINT8 INC_CMD_WRITE(INC_UINT8 ucI2CID, INC_UINT16 uiAddr, INC_UINT16 uiData)
{
	if(m_ucCommandMode == INC_SPI_CTRL) return INC_SPI_REG_WRITE(ucI2CID, uiAddr, uiData);
	else if(m_ucCommandMode == INC_I2C_CTRL) return INC_I2C_WRITE(ucI2CID, uiAddr, uiData);
	else if(m_ucCommandMode == INC_EBI_CTRL) return INC_EBI_WRITE(ucI2CID, uiAddr, uiData);
	return INC_I2C_WRITE(ucI2CID, uiAddr, uiData);
}

INC_UINT16 INC_CMD_READ(INC_UINT8 ucI2CID, INC_UINT16 uiAddr)
{
	if(m_ucCommandMode == INC_SPI_CTRL) return INC_SPI_REG_READ(ucI2CID, uiAddr);
	else if(m_ucCommandMode == INC_I2C_CTRL) return INC_I2C_READ(ucI2CID, uiAddr);
	else if(m_ucCommandMode == INC_EBI_CTRL) return INC_EBI_READ(ucI2CID, uiAddr);
	return INC_I2C_READ(ucI2CID, uiAddr);
}

INC_UINT8 INC_CMD_READ_BURST(INC_UINT8 ucI2CID, INC_UINT16 uiAddr, INC_UINT8* pData, INC_UINT16 nSize)
{
	if(m_ucCommandMode == INC_SPI_CTRL) return INC_SPI_READ_BURST(ucI2CID, uiAddr, pData, nSize);
	else if(m_ucCommandMode == INC_I2C_CTRL) return INC_I2C_READ_BURST(ucI2CID, uiAddr, pData, nSize);
	else if(m_ucCommandMode == INC_EBI_CTRL) return INC_EBI_READ_BURST(ucI2CID, uiAddr, pData, nSize);
	return INC_I2C_READ_BURST(ucI2CID, uiAddr, pData, nSize);
}

void INC_MPICLOCK_SET(INC_UINT8 ucI2CID)
{
	if(m_ucUploadMode == STREAM_UPLOAD_TS){
		if(m_ucClockSpeed == INC_OUTPUT_CLOCK_1024) 	    INC_CMD_WRITE(ucI2CID, APB_MPI_BASE + 0x01, 0x9918);
		else if(m_ucClockSpeed == INC_OUTPUT_CLOCK_2048) 	INC_CMD_WRITE(ucI2CID, APB_MPI_BASE + 0x01, 0x440C);
		else if(m_ucClockSpeed == INC_OUTPUT_CLOCK_4096) 	INC_CMD_WRITE(ucI2CID, APB_MPI_BASE + 0x01, 0x3308);
		else INC_CMD_WRITE(ucI2CID, APB_MPI_BASE + 0x01, 0x3308);
		return;
	}

	if(m_ucClockSpeed == INC_OUTPUT_CLOCK_1024) 			INC_CMD_WRITE(ucI2CID, APB_MPI_BASE + 0x01, 0x9918);
	else if(m_ucClockSpeed == INC_OUTPUT_CLOCK_2048)		INC_CMD_WRITE(ucI2CID, APB_MPI_BASE + 0x01, 0x440C);
	else if(m_ucClockSpeed == INC_OUTPUT_CLOCK_4096)		INC_CMD_WRITE(ucI2CID, APB_MPI_BASE + 0x01, 0x2206);
	else INC_CMD_WRITE(ucI2CID, APB_MPI_BASE + 0x01, 0x2206);
}

void INC_UPLOAD_MODE(INC_UINT8 ucI2CID)
{
	INC_UINT16	uiStatus = 0x01;

	if(m_ucCommandMode != INC_EBI_CTRL){
		if(m_ucUploadMode == STREAM_UPLOAD_SPI) 			uiStatus = 0x05;
		if(m_ucUploadMode == STREAM_UPLOAD_SLAVE_PARALLEL) 	uiStatus = 0x04;

		if(m_ucUploadMode == STREAM_UPLOAD_TS) 				uiStatus = 0x101;
		if(m_ucMPI_CS_Active == INC_ACTIVE_HIGH) 			uiStatus |= 0x10;
		if(m_ucMPI_CLK_Active == INC_ACTIVE_HIGH) 			uiStatus |= 0x20;

		INC_CMD_WRITE(ucI2CID, APB_MPI_BASE+ 0x00, uiStatus);
	}

	if(m_ucUploadMode == STREAM_UPLOAD_TS){
		INC_CMD_WRITE(ucI2CID, APB_MPI_BASE+ 0x02, 188);
		INC_CMD_WRITE(ucI2CID, APB_MPI_BASE+ 0x03, 8);
		INC_CMD_WRITE(ucI2CID, APB_MPI_BASE+ 0x04, 188);
		INC_CMD_WRITE(ucI2CID, APB_MPI_BASE+ 0x05, 0);
	}
	else {
		INC_CMD_WRITE(ucI2CID, APB_MPI_BASE+ 0x02, MPI_CS_SIZE);
		INC_CMD_WRITE(ucI2CID, APB_MPI_BASE+ 0x04, INC_INTERRUPT_SIZE);
		INC_CMD_WRITE(ucI2CID, APB_MPI_BASE+ 0x05, INC_INTERRUPT_SIZE-188);
	}
}

ST_SUBCH_INFO* INC_GET_SETTING_DB(INC_UINT8 ucI2CID)
{
	ST_BBPINFO* pInfo;
	if(ucI2CID == T3700_I2C_ID80)
		pInfo = &g_astBBPRun[0];
	else 
		pInfo = &g_astBBPRun[1];

	return (ST_SUBCH_INFO*)&pInfo->stSubChInfo;
}


void INC_SWAP(ST_SUBCH_INFO* pMainInfo, INC_UINT16 nNo1, INC_UINT16 nNo2)
{
	INC_CHANNEL_INFO  stChInfo;
	stChInfo = pMainInfo->astSubChInfo[nNo1];
	pMainInfo->astSubChInfo[nNo1] = pMainInfo->astSubChInfo[nNo2];
	pMainInfo->astSubChInfo[nNo2] = stChInfo;
}

void INC_BUBBLE_SORT(ST_SUBCH_INFO* pMainInfo, INC_SORT_OPTION Opt)
{
	INC_INT16 nIndex, nLoop;
	INC_CHANNEL_INFO* pDest;
	INC_CHANNEL_INFO* pSour;

	if(pMainInfo->nSetCnt <= 1) return;

	for(nIndex = 0 ; nIndex < pMainInfo->nSetCnt-1; nIndex++) {
		pSour = &pMainInfo->astSubChInfo[nIndex];

		for(nLoop = nIndex + 1 ; nLoop < pMainInfo->nSetCnt; nLoop++) {
			pDest = &pMainInfo->astSubChInfo[nLoop];

			if(Opt == INC_SUB_CHANNEL_ID){
				if(pSour->ucSubChID > pDest->ucSubChID)
					INC_SWAP(pMainInfo, nIndex, nLoop);
			}
			else if(Opt == INC_START_ADDRESS){
				if(pSour->uiStarAddr > pDest->uiStarAddr)
					INC_SWAP(pMainInfo, nIndex, nLoop);
			}
			else if(Opt == INC_BIRRATE)	{
				if(pSour->uiBitRate > pDest->uiBitRate)
					INC_SWAP(pMainInfo, nIndex, nLoop);
			}

			else if(Opt == INC_FREQUENCY){
				if(pSour->ulRFFreq > pDest->ulRFFreq)
					INC_SWAP(pMainInfo, nIndex, nLoop);
			}
			else{
				if(pSour->uiStarAddr > pDest->uiStarAddr)
					INC_SWAP(pMainInfo, nIndex, nLoop);
			}
		}
	}
}

INC_UINT8 INC_UPDATE(INC_CHANNEL_INFO* pChInfo, ST_FIC_DB* pFicDb, INC_INT16 nIndex)
{
	INC_INT16 nLoop;

	pChInfo->ulRFFreq			= pFicDb->ulRFFreq;
	pChInfo->uiEnsembleID		= pFicDb->uiEnsembleID;
	pChInfo->ucSubChID			= pFicDb->aucSubChID[nIndex];
	pChInfo->ucServiceType		= pFicDb->aucDSCType[nIndex];
	pChInfo->uiStarAddr			= pFicDb->auiStartAddr[nIndex];
	pChInfo->uiTmID				= pFicDb->aucTmID[nIndex];
	pChInfo->ulServiceID		= pFicDb->aulServiceID[nIndex];
	pChInfo->uiPacketAddr		= pFicDb->aucSetPackAddr[nIndex];

#ifdef USER_APPLICATION_TYPE
	pChInfo->uiUserAppType		= pFicDb->astUserAppInfo.uiUserAppType[nIndex];
	pChInfo->uiUserAppDataLength= pFicDb->astUserAppInfo.ucUserAppDataLength[nIndex];
#endif

	pChInfo->uiBitRate			= pFicDb->astSchDb[nIndex].uiBitRate;
	pChInfo->ucSlFlag			= pFicDb->astSchDb[nIndex].ucShortLong;
	pChInfo->ucTableIndex		= pFicDb->astSchDb[nIndex].ucTableIndex;
	pChInfo->ucOption			= pFicDb->astSchDb[nIndex].ucOption;
	pChInfo->ucProtectionLevel	= pFicDb->astSchDb[nIndex].ucProtectionLevel;
	pChInfo->uiDifferentRate	= pFicDb->astSchDb[nIndex].uiDifferentRate;
	pChInfo->uiSchSize			= pFicDb->astSchDb[nIndex].uiSubChSize;

	for(nLoop = MAX_LABEL_CHAR-1; nLoop >= 0; nLoop--){
		if(pFicDb->aucServiceLabel[nIndex][nLoop] == 0x20)
			pFicDb->aucServiceLabel[nIndex][nLoop] = INC_NULL;
		else{
			//if(nLoop == MAX_LABEL_CHAR-1) 
				//pFicDb->aucServiceLabel[nIndex][nLoop] = INC_NULL;
			break;
		}
	}

	for(nLoop = MAX_LABEL_CHAR-1; nLoop >= 0; nLoop--){
		if(pFicDb->aucEnsembleLabel[nLoop] == 0x20)
			pFicDb->aucEnsembleLabel[nLoop] = INC_NULL;
		else{
			//if(nLoop == MAX_LABEL_CHAR-1) 
				//pFicDb->aucEnsembleLabel[nLoop] = INC_NULL;
			break;
		}
	}
#ifdef USER_APPLICATION_TYPE

	for(nLoop = MAX_USER_APP_DATA-1; nLoop >= 0; nLoop--){
		if(pFicDb->astUserAppInfo.aucUserAppData[nIndex][nLoop] == 0x20)
			pFicDb->astUserAppInfo.aucUserAppData[nIndex][nLoop] = INC_NULL;
		else{
			if(nLoop == MAX_USER_APP_DATA-1) 
				pFicDb->astUserAppInfo.aucUserAppData[nIndex][nLoop] = INC_NULL;
			break;
		}
	}
	memcpy(pChInfo->aucUserAppData,pFicDb->astUserAppInfo.aucUserAppData[nIndex],sizeof(INC_UINT8)* MAX_USER_APP_DATA);
#endif

	memcpy(pChInfo->aucLabel, pFicDb->aucServiceLabel[nIndex], sizeof(INC_UINT8)*MAX_LABEL_CHAR);
	memcpy(pChInfo->aucEnsembleLabel, pFicDb->aucEnsembleLabel, sizeof(INC_UINT8)*MAX_LABEL_CHAR);
	return INC_SUCCESS;
}

ST_BBPINFO* INC_GET_STRINFO(INC_UINT8 ucI2CID)
{
	if(ucI2CID == T3700_I2C_ID80) return &g_astBBPRun[0];
	return &g_astBBPRun[1];
}

INC_UINT16 INC_GET_FRAME_DURATION(ST_TRANSMISSION cTrnsMode)
{
	INC_UINT16 uPeriodFrame;
	switch(cTrnsMode){
		case TRANSMISSION_MODE1: uPeriodFrame = MAX_FRAME_DURATION; break;
		case TRANSMISSION_MODE2:
		case TRANSMISSION_MODE3: uPeriodFrame = MAX_FRAME_DURATION/4; break;
		case TRANSMISSION_MODE4: uPeriodFrame = MAX_FRAME_DURATION/2; break;
		default : uPeriodFrame = MAX_FRAME_DURATION; break;
	}
	return uPeriodFrame;
}

INC_UINT8 INC_GET_FIB_CNT(ST_TRANSMISSION ucMode)
{
	INC_UINT8 ucResult = 0;
	switch(ucMode){
		case TRANSMISSION_MODE1: ucResult = 12; break;
		case TRANSMISSION_MODE2: ucResult = 3; break;
		case TRANSMISSION_MODE3: ucResult = 4; break;
		case TRANSMISSION_MODE4: ucResult = 6; break;
		default: ucResult = 12; break;
	}
	return ucResult;
}

void INC_INTERRUPT_CLEAR(INC_UINT8 ucI2CID)
{
	INC_CMD_WRITE(ucI2CID, APB_INT_BASE+ 0x03, 0xFFFF);
}

void INC_INTERRUPT_INIT(INC_UINT8 ucI2CID)
{
	INC_UINT16 uiIntSet = INC_MPI_INTERRUPT_ENABLE;
	uiIntSet = ~uiIntSet;
	INC_CMD_WRITE(ucI2CID, APB_INT_BASE+ 0x00, m_unIntCtrl);
	INC_CMD_WRITE(ucI2CID, APB_INT_BASE+ 0x02, uiIntSet);
}

INC_UINT8 INC_CHIP_STATUS(INC_UINT8 ucI2CID)
{
	INC_UINT16 uiChipID;
	uiChipID = INC_CMD_READ(ucI2CID, APB_PHY_BASE+ 0x10);
	if((uiChipID & 0xF00) < 0xA00 || uiChipID == 0xFFFF)
	{
        printk(" [T3700] Chip ID Error : 0x%X\r\n", uiChipID);
	    return INC_ERROR;
    }
	return INC_SUCCESS;
}

INC_UINT8 INC_PLL_SET(INC_UINT8 ucI2CID) 
{
	INC_UINT16	wData;
	ST_BBPINFO* pInfo;
	pInfo = INC_GET_STRINFO(ucI2CID);

	wData = INC_CMD_READ(ucI2CID, APB_GEN_BASE+ 0x02) & 0xFE00;

	switch(m_ucPLL_Mode){
		case INPUT_CLOCK_24576KHZ:
			INC_CMD_WRITE(ucI2CID, APB_GEN_BASE+ 0x00, 0x7FF0);
			INC_CMD_WRITE(ucI2CID, APB_GEN_BASE+ 0x02, wData & 0xBFFF);
			break;
		case INPUT_CLOCK_27000KHZ:
			INC_CMD_WRITE(ucI2CID, APB_GEN_BASE+ 0x02, wData | 0x41BE);
			INC_CMD_WRITE(ucI2CID, APB_GEN_BASE+ 0x03, 0x310A);
			INC_CMD_WRITE(ucI2CID, APB_GEN_BASE+ 0x00, 0x7FF1);
			break;
		case INPUT_CLOCK_19200KHZ:
			INC_CMD_WRITE(ucI2CID, APB_GEN_BASE+ 0x02, wData | 0x413F);
			INC_CMD_WRITE(ucI2CID, APB_GEN_BASE+ 0x03, 0x1809);
			INC_CMD_WRITE(ucI2CID, APB_GEN_BASE+ 0x00, 0x7FF1);
			break;
		case INPUT_CLOCK_12000KHZ:
			INC_CMD_WRITE(ucI2CID, APB_GEN_BASE+ 0x02, wData | 0x4200);
			INC_CMD_WRITE(ucI2CID, APB_GEN_BASE+ 0x03, 0x190A);
			INC_CMD_WRITE(ucI2CID, APB_GEN_BASE+ 0x00, 0x7FF1);
			break;
	}
	if(m_ucPLL_Mode == INPUT_CLOCK_24576KHZ) return INC_SUCCESS;
	INC_DELAY(10);
	if(!(INC_CMD_READ(ucI2CID, APB_GEN_BASE+ 0x04) & 0x0001)){
		pInfo->nBbpStatus = ERROR_PLL;
		return INC_ERROR;
	}
	return INC_SUCCESS;
}

void INC_SCAN_SETTING(INC_UINT8 ucI2CID)
{
	INC_UINT16 uStatus;
	uStatus = INC_CMD_READ(ucI2CID, APB_PHY_BASE+ 0x41) & 0xFFF;
	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0x41, uStatus | 0xC000);

	INC_CMD_WRITE(ucI2CID, APB_RF_BASE+INC_ADDRESS_IFDELAY_LSB, INC_SCAN_IF_DLEAY_MAX&0xff);
	INC_CMD_WRITE(ucI2CID, APB_RF_BASE+INC_ADDRESS_IFDELAY_MSB, (INC_SCAN_IF_DLEAY_MAX>>8)&0xff);
	INC_CMD_WRITE(ucI2CID, APB_RF_BASE+INC_ADDRESS_RFDELAY_LSB, INC_SCAN_RF_DLEAY_MAX&0xff);
	INC_CMD_WRITE(ucI2CID, APB_RF_BASE+INC_ADDRESS_RFDELAY_MSB, (INC_SCAN_RF_DLEAY_MAX>>8)&0xff);
}

void INC_AIRPLAY_SETTING(INC_UINT8 ucI2CID)
{
	INC_UINT16 uStatus;
	uStatus = INC_CMD_READ(ucI2CID, APB_PHY_BASE+ 0x41) & 0xFFF;
	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0x41, uStatus | 0x4000);

	INC_CMD_WRITE(ucI2CID, APB_RF_BASE+INC_ADDRESS_IFDELAY_LSB, INC_AIRPLAY_IF_DLEAY_MAX&0xff);
	INC_CMD_WRITE(ucI2CID, APB_RF_BASE+INC_ADDRESS_IFDELAY_MSB, (INC_AIRPLAY_IF_DLEAY_MAX>>8)&0xff);
	INC_CMD_WRITE(ucI2CID, APB_RF_BASE+INC_ADDRESS_RFDELAY_LSB, INC_AIRPLAY_RF_DLEAY_MAX&0xff);
	INC_CMD_WRITE(ucI2CID, APB_RF_BASE+INC_ADDRESS_RFDELAY_MSB, (INC_AIRPLAY_RF_DLEAY_MAX>>8)&0xff);
}


void INC_MULTI_SET_CHANNEL(INC_UINT8 ucI2CID, ST_SUBCH_INFO* pMultiInfo)
{
	INC_UINT16  uiLoop, wData;
	INC_CHANNEL_INFO* pChInfo;
	INC_CTRL    wDmbMode;

	INC_CMD_WRITE(ucI2CID, APB_RS_BASE+ 0x00, 0x800);
	for(uiLoop = 0 ; uiLoop < pMultiInfo->nSetCnt; uiLoop++)
	{
		pChInfo = &pMultiInfo->astSubChInfo[uiLoop];
		wDmbMode = (pChInfo->ucServiceType == 0x18) ? INC_DMB : INC_DAB;
		wData = INC_CMD_READ(ucI2CID, APB_VTB_BASE+ 0x01) & (~(0x3 << uiLoop*2));

		if(wDmbMode == INC_DMB) 
			INC_CMD_WRITE(ucI2CID, APB_VTB_BASE+ 0x01, wData | 0x02 << (uiLoop*2));
		else  
			INC_CMD_WRITE(ucI2CID, APB_VTB_BASE+ 0x01, wData | 0x01 << (uiLoop*2));

		wData = INC_CMD_READ(ucI2CID, APB_RS_BASE+ 0x00);
		INC_CMD_WRITE(ucI2CID, APB_RS_BASE+ 0x00, wData | (0x40 >> uiLoop));
	}

	wData = INC_CMD_READ(ucI2CID, APB_RS_BASE+ 0x00);
#ifdef INC_MULTI_CHANNEL_FIC_UPLOAD
	INC_CMD_WRITE(ucI2CID, APB_RS_BASE+ 0x00, wData | 0x8080);
#else
	INC_CMD_WRITE(ucI2CID, APB_RS_BASE+ 0x00, wData | 0x8000);
#endif
}

void INC_SET_CHANNEL(INC_UINT8 ucI2CID, INC_CTRL nMode)
{
	INC_CMD_WRITE(ucI2CID, APB_RS_BASE + 0x00, 0);
	if(nMode == INC_DMB) INC_CMD_WRITE(ucI2CID, APB_VTB_BASE+ 0x01, 0x0042);
	else INC_CMD_WRITE(ucI2CID, APB_VTB_BASE+ 0x01, 0x0041);

	INC_CMD_WRITE(ucI2CID, APB_RS_BASE+ 0x00, 0x8040);
}

INC_UINT8 INC_INIT(INC_UINT8 ucI2CID)
{
	ST_BBPINFO* pInfo;
	INC_UINT16 unStatus;
	INC_CHANNEL_INFO stChInfo;

	pInfo = INC_GET_STRINFO(ucI2CID);
	memset(pInfo, 0 , sizeof(ST_BBPINFO));


#if 0
	INC_INT16 nLoop;
	for(nLoop = 0 ; nLoop < 20; nLoop++)
	{
		INC_CMD_WRITE(ucI2CID, APB_MPI_BASE + 0x2, 0x1234);
		unStatus = INC_CMD_READ(ucI2CID, APB_MPI_BASE + 0x2); 
		printk("\r\n S[ 0x%.4X] R[ 0x%.4X]",nLoop,unStatus );
	}

	printk("\r\n Read Message\n");
#endif

	if(m_ucTransMode < TRANSMISSION_MODE1 || m_ucTransMode > TRANSMISSION_AUTOFAST)
		return INC_ERROR;

	pInfo->ucTransMode = m_ucTransMode;
	if(INC_PLL_SET(ucI2CID) != INC_SUCCESS) 		return INC_ERROR;
	if(INC_CHIP_STATUS(ucI2CID) != INC_SUCCESS) 	return INC_ERROR;

	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0x3B, 0xFFFF);
	INC_DELAY(10);

	INC_CMD_WRITE(ucI2CID, APB_VTB_BASE+ 0x00, 0x8000);
	INC_CMD_WRITE(ucI2CID, APB_VTB_BASE+ 0x01, 0x01C1);
	INC_CMD_WRITE(ucI2CID, APB_VTB_BASE+ 0x05, 0x0008);
	INC_CMD_WRITE(ucI2CID, APB_RS_BASE+ 0x01, TS_ERR_THRESHOLD);
	INC_CMD_WRITE(ucI2CID, APB_RS_BASE+ 0x0A, 0x80F0);
	INC_CMD_WRITE(ucI2CID, APB_RS_BASE+ 0x09, 0x000C);

	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0x00, 0xF0FF);
	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0x88, 0x2210);
	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0x98, 0x0000);
	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0x41, 0x4CCC);
	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0xB0, 0x8320);
	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0xB4, 0x4C01);
	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0xBC, 0x4088);

	switch(m_ucTransMode){
		case TRANSMISSION_AUTO:
			INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0x98, 0x8000);
			break;
		case TRANSMISSION_AUTOFAST:
			INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0x98, 0xC000);
			break;
		case TRANSMISSION_MODE1:
			INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0xD0, 0x7F1F);
			INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0x80, 0x4082);
			INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0x90, 0x0430);
			INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0xC8, 0x3FF1);
			break;
		case TRANSMISSION_MODE2:
			INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0xD0, 0x1F07);
			INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0x80, 0x4182); 
			INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0x90, 0x0415); 
			INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0xC8, 0x1FF1);
			break;
		case TRANSMISSION_MODE3:
			INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0xD0, 0x0F03);
			INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0x80, 0x4282);
			INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0x90, 0x0408);
			INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0xC8, 0x03F1);
			break;
		case TRANSMISSION_MODE4:
			INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0xD0, 0x3F0F);
			INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0x80, 0x4382);
			INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0x90, 0x0420); 
			INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0xC8, 0x3FF1);
			break;
	}

	INC_MPICLOCK_SET(ucI2CID);
	INC_UPLOAD_MODE(ucI2CID);

	switch(m_ucRfBand){
		case KOREA_BAND_ENABLE	: INC_READY(ucI2CID,	208736); break;
		case BANDIII_ENABLE 	: INC_READY(ucI2CID,	174928); break;
		case LBAND_ENABLE		: INC_READY(ucI2CID,	1452960); break;
		case CHINA_ENABLE		: INC_READY(ucI2CID,	168160); break;
		case ROAMING_ENABLE 	: INC_READY(ucI2CID,	217280); break;
	}

	INC_DELAY(100);

	printk("INC_INIT START GOOD\n");

	return INC_SUCCESS;
}

INC_UINT8 INC_READY(INC_UINT8 ucI2CID, INC_UINT32 ulFreq)
{
	INC_UINT16	uStatus = 0;
	INC_UINT8   bModeFlag = 0;
	ST_BBPINFO* pInfo;
	pInfo = INC_GET_STRINFO(ucI2CID);


	if(ulFreq == 189008 || ulFreq == 196736 || ulFreq == 205280 || ulFreq == 213008		// KBAND
			|| ulFreq == 180064 || ulFreq == 188928 || ulFreq == 195936		// BAND3
			|| ulFreq == 204640 || ulFreq == 213360 || ulFreq == 220352
			|| ulFreq == 222064 || ulFreq == 229072 || ulFreq == 237488
			|| ulFreq == 180144 || ulFreq == 196144 || ulFreq == 205296		// CHINA
			|| ulFreq == 212144 || ulFreq == 213856) {
		bModeFlag = 1;
	}
	if(m_ucRfBand == ROAMING_ENABLE) bModeFlag = 1;

	if(bModeFlag)
	{
		uStatus = INC_CMD_READ(ucI2CID, APB_PHY_BASE+ 0xC6);
		INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0xC6, uStatus | 0x0008);
		uStatus = INC_CMD_READ(ucI2CID, APB_PHY_BASE+ 0x41) & 0xFC00;
		INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0x41, uStatus | 0x111);
	}
	else
	{
		uStatus = INC_CMD_READ(ucI2CID, APB_PHY_BASE+ 0xC6);
		INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0xC6, uStatus & 0xFFF7);
		uStatus = INC_CMD_READ(ucI2CID, APB_PHY_BASE+ 0x41) & 0xFC00;
		INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0x41, uStatus | 0xCC);
	}

	if(INC_RF500_START(ucI2CID, ulFreq, m_ucRfBand) != INC_SUCCESS){
		pInfo->nBbpStatus = ERROR_READY;
		return INC_ERROR;
	}

	if(m_ucDPD_Mode == INC_DPD_OFF){
		uStatus = INC_CMD_READ(ucI2CID, APB_RF_BASE+ 0x03);
		INC_CMD_WRITE(ucI2CID, APB_RF_BASE+ 0x03, uStatus | 0x80);
	}

	return INC_SUCCESS;
}

INC_UINT8 INC_SYNCDETECTOR(INC_UINT8 ucI2CID, INC_UINT32 ulFreq, INC_UINT8 ucScanMode)
{
	INC_UINT16 wOperState, wIsNullSync = 0, wSyncRefTime = 800;
	INC_UINT16 uiTimeOut = 0, uiRefSleep = 30;
	INC_UINT16 wData = 0;
	ST_BBPINFO* pInfo;
	pInfo = INC_GET_STRINFO(ucI2CID);

	INC_SCAN_SETTING(ucI2CID);

	if(m_ucDPD_Mode == INC_DPD_ON){
		INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0xA8, 0x3000);
		INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0xAA, 0x0000);
	}

	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0xBC, 0x4088);
	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0x3A, 0x1);
	INC_DELAY(200);

	while(1)
	{
		if(pInfo->ucStop){
			pInfo->nBbpStatus = ERROR_USER_STOP;
			break;
		}

		INC_DELAY(uiRefSleep);
		wOperState = INC_CMD_READ(ucI2CID, APB_PHY_BASE+ 0x10);
		wOperState = ((wOperState & 0x7000) >> 12);


		if((wIsNullSync == 0) && wOperState >= 0x2) wIsNullSync = 1;


		if(wIsNullSync == 0){
			wData = INC_CMD_READ(ucI2CID, APB_PHY_BASE+ 0x14) & 0x0F00; 			
			if(wData < 0x0200){
				pInfo->nBbpStatus = ERROR_SYNC_NO_SIGNAL;
				break;
			}
		}
		if(wOperState >= 0x5){
			INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0xBC, 0x4008);
			return INC_SUCCESS;
		}

		uiTimeOut++;
		if((wIsNullSync == 0) && (uiTimeOut >= (wSyncRefTime / uiRefSleep))){
			pInfo->nBbpStatus = ERROR_SYNC_NULL;
			break;
		}

		if((wIsNullSync == 0) && uiTimeOut < (1500 / uiRefSleep)){
			wData = INC_CMD_READ(ucI2CID, APB_PHY_BASE+ 0x2C) & 0xFC00;
			if(wData >= 0x3000 || wData >= 0x0000){
				pInfo->nBbpStatus = ERROR_SYNC_LOW_SIGNAL;
				break;
			}
		}

		if(uiTimeOut >= (3000 / uiRefSleep)){
			pInfo->nBbpStatus = ERROR_SYNC_TIMEOUT;
			break;
		}
	}

	return INC_ERROR;
}

void FILE_SAVE_FUNCTION(char* fname, unsigned char *pData, unsigned long dwDataSize)
{
#define DUMP_FILE_NAME "/system/ts_data.ts"
	mm_segment_t oldfs;
	int ret;
	static struct file *fp_ts_data  = NULL;

	if(fp_ts_data == NULL) 
	{
		fp_ts_data = filp_open(DUMP_FILE_NAME, O_APPEND | O_CREAT, 0);
		if(fp_ts_data == NULL) 
		{
			printk("[%s] file open error!\n", __FUNCTION__);
			return;
		}
	}

	oldfs = get_fs();
	set_fs(KERNEL_DS);

	if(fp_ts_data != NULL &&  dwDataSize == 0)
	{
		filp_close(fp_ts_data, current->files);	
		fp_ts_data = NULL;
		return;
	}

	//printk("[%s] save ts data \n", __FUNCTION__);
	ret = fp_ts_data->f_op->write(
			fp_ts_data, 
			pData, 
			dwDataSize, 
			&fp_ts_data->f_pos);

	//fp_ts_data->f_op->flush(fp_ts_data, current->files);
	set_fs(oldfs);
}

INC_UINT8 INC_FICDECODER(INC_UINT8 ucI2CID, ST_SIMPLE_FIC bSimpleFIC)
{
	INC_UINT16		wFicLen, uPeriodFrame, uFIBCnt, uiRefSleep = 20;
	INC_UINT16		nLoop, nFicCnt;
	INC_UINT8		abyBuff[MAX_FIC_SIZE];
	ST_BBPINFO*		pInfo;

#ifdef USER_APPLICATION_TYPE
	INC_UINT8		ucCnt, ucCompCnt;
	ST_FIC_DB* 		pFicDb;
	INC_UINT8		ucIsUserApp = INC_ERROR;
	pFicDb = INC_GETFIC_DB(ucI2CID);
#endif

	pInfo = INC_GET_STRINFO(ucI2CID);

	INC_INITDB(ucI2CID);
	uFIBCnt = INC_GET_FIB_CNT(m_ucTransMode);
	uPeriodFrame = INC_GET_FRAME_DURATION(m_ucTransMode);
	uiRefSleep = uPeriodFrame >> 2;

	nFicCnt = 2500 / uiRefSleep;

	for(nLoop = 0; nLoop < nFicCnt; nLoop++)
	{
		if(pInfo->ucStop == 1)	{
			pInfo->nBbpStatus = ERROR_USER_STOP;
			break;
		}

		INC_DELAY(uiRefSleep);

		if(!(INC_CMD_READ(ucI2CID, APB_VTB_BASE+ 0x00) & 0x4000))
			continue;

		wFicLen = INC_CMD_READ(ucI2CID, APB_VTB_BASE+ 0x09);
		if(!wFicLen) continue;
		wFicLen++;

		if(wFicLen != (uFIBCnt*FIB_SIZE)) continue;
		INC_CMD_READ_BURST(ucI2CID, APB_FIC_BASE, abyBuff, wFicLen);

		//        FILE_SAVE_FUNCTION("", abyBuff, wFicLen);

		if(INC_FICPARSING(ucI2CID, abyBuff, wFicLen, bSimpleFIC)){


#ifdef USER_APPLICATION_TYPE
			if(bSimpleFIC == SIMPLE_FIC_ENABLE) 
				return INC_SUCCESS;
			else {
				for(ucCompCnt = ucCnt = 0; ucCnt < pFicDb->ucSubChCnt; ucCnt++){
					if((pFicDb->uiUserAppTypeOk >> ucCnt) & 0x01) ucCompCnt++;
				}
				if(pFicDb->ucServiceComponentCnt == ucCompCnt) 
					return INC_SUCCESS;
			}
			ucIsUserApp = INC_SUCCESS;
#else
			return INC_SUCCESS;
#endif
		}
	}

#ifdef USER_APPLICATION_TYPE
	if(ucIsUserApp == INC_SUCCESS)
		return INC_SUCCESS;
#endif

	//	FILE_SAVE_FUNCTION("", abyBuff, 0);

	pInfo->nBbpStatus = ERROR_FICDECODER;

	return INC_ERROR;
}

INC_UINT8 INC_MULTI_START(INC_UINT8 ucI2CID, ST_SUBCH_INFO* pMultiInfo, INC_UINT8 IsEnsembleSame)
{
	INC_INT16 	nLoop, nSchID;
	INC_UINT16 	wData, wAddr;
	ST_BBPINFO* pInfo;
	INC_CHANNEL_INFO* pChInfo;

	INC_UINT16 wCeil, wIndex = 0, wStartAddr, wEndAddr;

	pInfo = INC_GET_STRINFO(ucI2CID);

	if(IsEnsembleSame){
		for(nLoop = 0; nLoop < 20; nLoop++){
			if(INC_CMD_READ(ucI2CID, APB_DEINT_BASE+ 0x01) & 0x8000) break;
			INC_DELAY(3);
		}
		if(nLoop == 20){
			pInfo->nBbpStatus = ERROR_START_MODEM_CLEAR;
			return INC_ERROR;
		}
	}

	for(nLoop = 0; nLoop < pMultiInfo->nSetCnt; nLoop++){

		pChInfo = &pMultiInfo->astSubChInfo[nLoop];

		nSchID = (INC_UINT16)pChInfo->ucSubChID & 0x3f;
		wAddr = 0x2 + nLoop * 2;
		INC_CMD_WRITE(ucI2CID, APB_DEINT_BASE+ wAddr, (INC_UINT16)(((INC_UINT16)nSchID << 10) + pChInfo->uiStarAddr));

		wAddr += 1;
		if(pChInfo->ucSlFlag == 0) INC_CMD_WRITE(ucI2CID, APB_DEINT_BASE+ wAddr, 0x8000 + (pChInfo->ucTableIndex & 0x3f));
		else INC_CMD_WRITE(ucI2CID, APB_DEINT_BASE + wAddr, 0x8000 + 0x400 + pChInfo->uiSchSize);

		wAddr = 0x2 + nLoop;
		if(pChInfo->ucSlFlag == 0) INC_CMD_WRITE(ucI2CID, APB_VTB_BASE+ wAddr , (pChInfo->ucTableIndex & 0x3f));
		else{
			wData = 0x8000 + ((pChInfo->ucOption & 0x7) << 12) + ((pChInfo->ucProtectionLevel & 0x3) << 10) + pChInfo->uiDifferentRate;
			INC_CMD_WRITE(ucI2CID, APB_VTB_BASE+ wAddr , wData);
		}

		if(m_ucDPD_Mode == INC_DPD_ON){
			switch(pInfo->ucTransMode){
				case TRANSMISSION_MODE1 : wCeil = 3072;	wIndex = 4; break; 
				case TRANSMISSION_MODE2 : wCeil = 768;	wIndex = 4;	break;
				case TRANSMISSION_MODE3 : wCeil = 384;	wIndex = 9;	break;
				case TRANSMISSION_MODE4 : wCeil = 1536;	wIndex = 4;	break;
				default : wCeil = 3072; break;
			}

			wStartAddr = ((pChInfo->uiStarAddr * 64) / wCeil) + wIndex - 2;
			wEndAddr = (INC_UINT16)(((pChInfo->uiStarAddr + pChInfo->uiSchSize) * 64) / wCeil) + wIndex + 2;
			wData = (wStartAddr & 0xFF) << 8 | (wEndAddr & 0xFF);

			INC_CMD_WRITE(ucI2CID, APB_PHY_BASE + 0xAA + (nLoop * 2), wData);
			wData = INC_CMD_READ(ucI2CID, APB_PHY_BASE + 0xA8);
			INC_CMD_WRITE(ucI2CID, APB_PHY_BASE + 0xA8, wData | (1<<nLoop));
		}
		else{
			INC_CMD_WRITE(ucI2CID, APB_PHY_BASE + 0xAA + (nLoop * 2), 0x0000);
			INC_CMD_WRITE(ucI2CID, APB_PHY_BASE + 0xA8, 0x3000);
		}

		wData = INC_CMD_READ(ucI2CID, APB_RS_BASE+ 0x00);
		INC_CMD_WRITE(ucI2CID, APB_RS_BASE+ 0x00, wData | (0x1 << (6-nLoop)));
	}

	INC_MULTI_SET_CHANNEL(ucI2CID, pMultiInfo);
	INC_CMD_WRITE(ucI2CID, APB_DEINT_BASE+ 0x01, 0x0011);

	return INC_SUCCESS;
}


INC_UINT8 INC_START(INC_UINT8 ucI2CID, INC_CHANNEL_INFO* pChInfo, INC_UINT16 IsEnsembleSame)
{
	INC_INT16 	nLoop, nSchID;
	INC_UINT16 	wData;
	INC_CTRL 	wSubChMode;
	ST_BBPINFO* pInfo;

	INC_UINT16 wCeil, wIndex = 0, wStartAddr, wEndAddr;

	pInfo = INC_GET_STRINFO(ucI2CID);

	if(IsEnsembleSame){
		for(nLoop = 0; nLoop < 20; nLoop++){
			if(INC_CMD_READ(ucI2CID, APB_DEINT_BASE+ 0x01) & 0x8000) break;
			INC_DELAY(3);
		}
		if(nLoop == 20){
			pInfo->nBbpStatus = ERROR_START_MODEM_CLEAR;
			return INC_ERROR;
		}
	}

	nSchID = (INC_UINT16)pChInfo->ucSubChID & 0x3f;
	INC_CMD_WRITE(ucI2CID, APB_DEINT_BASE+ 0x02, (INC_UINT16)(((INC_UINT16)nSchID << 10) + pChInfo->uiStarAddr));

	if(pChInfo->ucSlFlag == 0) INC_CMD_WRITE(ucI2CID, APB_DEINT_BASE+ 0x03, 0x8000 + (pChInfo->ucTableIndex & 0x3f));
	else INC_CMD_WRITE(ucI2CID, APB_DEINT_BASE + 0x03, 0x8000 + 0x400 + pChInfo->uiSchSize);

	if(pChInfo->ucSlFlag == 0) INC_CMD_WRITE(ucI2CID, APB_VTB_BASE+ 0x02, (pChInfo->ucTableIndex & 0x3f));
	else{
		wData = 0x8000 + ((pChInfo->ucOption & 0x7) << 12) + ((pChInfo->ucProtectionLevel & 0x3) << 10) + pChInfo->uiDifferentRate;
		INC_CMD_WRITE(ucI2CID, APB_VTB_BASE+ 0x02, wData);
	}


	if(m_ucDPD_Mode == INC_DPD_ON){
		switch(pInfo->ucTransMode){
			case TRANSMISSION_MODE1: wCeil = 3072;	wIndex = 4; break; 
			case TRANSMISSION_MODE2: wCeil = 768;	wIndex = 4;	break;
			case TRANSMISSION_MODE3: wCeil = 384;	wIndex = 9;	break;
			case TRANSMISSION_MODE4: wCeil = 1536;	wIndex = 4;	break;
			default : wCeil = 3072; break;
		}

		wStartAddr = ((pChInfo->uiStarAddr * 64) / wCeil) + wIndex - 2;
		wEndAddr = (INC_UINT16)(((pChInfo->uiStarAddr + pChInfo->uiSchSize) * 64) / wCeil) + wIndex + 2;
		wData = (wStartAddr & 0xFF) << 8 | (wEndAddr & 0xFF);

		INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0xAA, wData);
		INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0xA8, 0x3001);
	}
	else{
		INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0xAA, 0x0000);
		INC_CMD_WRITE(ucI2CID, APB_PHY_BASE+ 0xA8, 0x3000);
	}

	wSubChMode = (pChInfo->ucServiceType == 0x18) ? INC_DMB : INC_DAB;
	INC_SET_CHANNEL(ucI2CID, wSubChMode);
	INC_CMD_WRITE(ucI2CID, APB_DEINT_BASE+ 0x01, 0x0011);

	return INC_SUCCESS;
}

INC_UINT8 INC_STOP(INC_UINT8 ucI2CID)
{
	INC_INT16 nLoop;
	INC_UINT16 uStatus;
	ST_TRANSMISSION ucTransMode;
	ST_BBPINFO* pInfo;

	pInfo = INC_GET_STRINFO(ucI2CID);
	ucTransMode = pInfo->ucTransMode;
	memset(pInfo, 0, sizeof(ST_BBPINFO));
	pInfo->ucTransMode = ucTransMode;
	INC_CMD_WRITE(ucI2CID, APB_RS_BASE + 0x00, 0x0000);

	uStatus = INC_CMD_READ(ucI2CID, APB_DEINT_BASE+ 0x01) & 0xFFFF;
	if(uStatus == 0x0011){
		INC_CMD_WRITE(ucI2CID, APB_DEINT_BASE+ 0x01, 0x0001);
		for(nLoop = 0; nLoop < 10; nLoop++){
			uStatus = INC_CMD_READ(ucI2CID, APB_DEINT_BASE+ 0x01) & 0xFFFF;
			if(uStatus != 0xFFFF && (uStatus & 0x8000)) break;
			INC_DELAY(10);
		}
		if(nLoop >= 10){
			pInfo->nBbpStatus = ERROR_STOP;
			return INC_ERROR;
		}
	}

	INC_CMD_WRITE(ucI2CID, APB_RS_BASE   + 0x00, 0x0000);
	uStatus = INC_CMD_READ(ucI2CID, APB_PHY_BASE+ 0x10) & 0x7000;
	INC_CMD_WRITE(ucI2CID, APB_PHY_BASE  + 0x3B, 0x4000);

	if(uStatus == 0x5000) INC_DELAY(25);

	for(nLoop = 0; nLoop < 10; nLoop++){
		INC_DELAY(2);	
		uStatus = INC_CMD_READ(ucI2CID, APB_PHY_BASE+ 0x3B) & 0xFFFF;
		if(uStatus == 0x8000) break;
	}

	if(nLoop >= 10){
		pInfo->nBbpStatus = ERROR_STOP;
		return INC_RETRY;
	}

	return INC_SUCCESS;
}


INC_UINT8 INC_CHANNEL_START(INC_UINT8 ucI2CID, INC_CHANNEL_INFO* pChInfo)
{
	INC_UINT16 wEnsemble;
	INC_UINT16 wData;
	ST_BBPINFO* pInfo;
	INC_UINT8 reErr;

	INC_INTERRUPT_INIT(ucI2CID);
	INC_INTERRUPT_CLEAR(ucI2CID);
	pInfo = INC_GET_STRINFO(ucI2CID);

	pInfo->ucStop				= 0;

	wEnsemble = pInfo->ulFreq == pChInfo->ulRFFreq;

	printk("INC_CHANNEL_START  \n");

	if(!wEnsemble){
		if( (reErr = INC_STOP(ucI2CID)) != INC_SUCCESS) {
			printk("INC_STOP ERROR  \n");
			return reErr;
		}
		printk("INC_STOP GOOD  \n");
		if(INC_READY(ucI2CID, pChInfo->ulRFFreq) != INC_SUCCESS)  {
			printk("INC_READY ERROR  \n");
			return INC_ERROR;
		}
		printk("INC_READY GOOD  \n");
		if(INC_SYNCDETECTOR(ucI2CID, pChInfo->ulRFFreq, 0) != INC_SUCCESS) {
			printk("INC_SYNCDETECTOR ERROR  \n");
			return INC_ERROR;
		}
		printk("INC_SYNCDETECTOR GOOD  \n");
		if(INC_FICDECODER(ucI2CID, SIMPLE_FIC_ENABLE) != INC_SUCCESS) {
			printk("INC_FICDECODER ERROR  \n");
			return INC_ERROR;
		}
		printk("INC_FICDECODER GOOD  \n");
		if(INC_FIC_UPDATE(ucI2CID, pChInfo) != INC_SUCCESS)	 {
			printk("INC_FIC_UPDATE ERROR  \n");
			return INC_ERROR;
		}
		printk("INC_FIC_UPDATE GOOD  \n");
		if(INC_START(ucI2CID, pChInfo, wEnsemble) != INC_SUCCESS)  {
			printk("INC_START ERROR  \n");
			return INC_ERROR;
		}
		printk("INC_START GOOD  \n");
	}
	else{
		wData = INC_CMD_READ(ucI2CID, APB_DEINT_BASE+ 0x01);
		INC_CMD_WRITE(ucI2CID, APB_DEINT_BASE+ 0x01, wData & 0xFFEF);
		if(INC_FIC_UPDATE(ucI2CID, pChInfo) != INC_SUCCESS)  {
			printk("INC_FIC_UPDATE ERROR  \n");
			return INC_ERROR;
		}
		printk("INC_FIC_UPDATE GOOD  \n");
		if(INC_START(ucI2CID, pChInfo, wEnsemble) != INC_SUCCESS)  {
			printk("INC_START ERROR  \n");
			return INC_ERROR;
		}
		printk("INC_START GOOD  \n");
	}

	INC_AIRPLAY_SETTING(ucI2CID);

	pInfo->ucProtectionLevel = pChInfo->ucProtectionLevel;
	pInfo->ulFreq 	= pChInfo->ulRFFreq;
	return INC_SUCCESS;
}

INC_UINT8 INC_CHANNEL_START_TEST(INC_UINT8 ucI2CID, INC_CHANNEL_INFO* pChInfo)
{
	INC_UINT16 wEnsemble;
	INC_UINT16 wData;
	ST_BBPINFO* pInfo;
	INC_UINT8 reErr;

	INC_INTERRUPT_INIT(ucI2CID);
	INC_INTERRUPT_CLEAR(ucI2CID);
	pInfo = INC_GET_STRINFO(ucI2CID);

	pInfo->ucStop				= 0;

	wEnsemble = pInfo->ulFreq == pChInfo->ulRFFreq;

	printk("INC_CHANNEL_START  \n");

	if(!wEnsemble){
		if( (reErr = INC_STOP(ucI2CID)) != INC_SUCCESS) {
			printk("INC_STOP ERROR  \n");
			return reErr;
		}
		printk("INC_STOP GOOD  \n");
		if(INC_READY(ucI2CID, pChInfo->ulRFFreq) != INC_SUCCESS)  {
			printk("INC_READY ERROR  \n");
			return INC_ERROR;
		}
		printk("INC_READY GOOD  \n");
		if(INC_SYNCDETECTOR(ucI2CID, pChInfo->ulRFFreq, 0) != INC_SUCCESS) {
			printk("INC_SYNCDETECTOR ERROR  \n");
			return INC_ERROR;
		}
		printk("INC_SYNCDETECTOR GOOD  \n");
		if(INC_FICDECODER(ucI2CID, SIMPLE_FIC_DISABLE) != INC_SUCCESS) {
			printk("INC_FICDECODER ERROR  \n");
			return INC_ERROR;
		}
		printk("INC_FICDECODER GOOD  \n");
		if(INC_FIC_UPDATE(ucI2CID, pChInfo) != INC_SUCCESS)	 {
			printk("INC_FIC_UPDATE ERROR  \n");
			return INC_ERROR;
		}
		printk("INC_FIC_UPDATE GOOD  \n");
		if(INC_START(ucI2CID, pChInfo, wEnsemble) != INC_SUCCESS)  {
			printk("INC_START ERROR  \n");
			return INC_ERROR;
		}
		printk("INC_START GOOD  \n");
	}
	else{
		wData = INC_CMD_READ(ucI2CID, APB_DEINT_BASE+ 0x01);
		INC_CMD_WRITE(ucI2CID, APB_DEINT_BASE+ 0x01, wData & 0xFFEF);
		if(INC_FIC_UPDATE(ucI2CID, pChInfo) != INC_SUCCESS)  {
			printk("INC_FIC_UPDATE ERROR  \n");
			return INC_ERROR;
		}
		printk("INC_FIC_UPDATE GOOD  \n");
		if(INC_START(ucI2CID, pChInfo, wEnsemble) != INC_SUCCESS)  {
			printk("INC_START ERROR  \n");
			return INC_ERROR;
		}
		printk("INC_START GOOD  \n");
	}

	INC_AIRPLAY_SETTING(ucI2CID);

	pInfo->ucProtectionLevel = pChInfo->ucProtectionLevel;
	pInfo->ulFreq 	= pChInfo->ulRFFreq;
	return INC_SUCCESS;
}

INC_ERROR_INFO INC_MULTI_ERROR_CHECK(ST_SUBCH_INFO* pMultiInfo)
{
	INC_INT16 nLoop, nDmbChCnt = 0;
	INC_ERROR_INFO ErrorInfo = ERROR_NON;
	INC_CHANNEL_INFO* pChInfo;

	if(pMultiInfo == INC_NULL) 	
	{
		ErrorInfo = ERROR_MULTI_CHANNEL_NULL;
#ifdef FEATURE_PREVENT_BUG_FIX_CH14 
		return ErrorInfo;
#endif
	}
	if(pMultiInfo->astSubChInfo[0].ulRFFreq == 0) ErrorInfo = ERROR_MULTI_CHANNEL_FREQ;
	if(pMultiInfo->nSetCnt > INC_MULTI_MAX_CHANNEL) ErrorInfo = ERROR_MULTI_CHANNEL_COUNT_OVER;
	if(pMultiInfo->nSetCnt == 0) ErrorInfo = ERROR_MULTI_CHANNEL_COUNT_LOW;

	for(nLoop = 0 ;nLoop < pMultiInfo->nSetCnt; nLoop++)
	{
		pChInfo = &pMultiInfo->astSubChInfo[nLoop];
		if(pChInfo->ucServiceType == 0x18)
			nDmbChCnt++;
	}

	if(nDmbChCnt >= INC_MULTI_MAX_CHANNEL)
		ErrorInfo = ERROR_MULTI_CHANNEL_DMB_MAX;

	return ErrorInfo;
}

INC_UINT8 INC_MULTI_START_CTRL(INC_UINT8 ucI2CID, ST_SUBCH_INFO* pMultiInfo)
{
	INC_UINT16 wEnsemble, wData, wLoop;
	ST_BBPINFO* pInfo;
	INC_CHANNEL_INFO* pChInfo[INC_MULTI_MAX_CHANNEL];
	INC_ERROR_INFO ErrorInfo;

	INC_INTERRUPT_INIT(ucI2CID);
	INC_INTERRUPT_CLEAR(ucI2CID);
	pInfo = INC_GET_STRINFO(ucI2CID);

	pInfo->ucStop = 0;

	for(wLoop = 0 ; wLoop < INC_MULTI_MAX_CHANNEL; wLoop++)
		pChInfo[wLoop] = INC_NULL;

	ErrorInfo = INC_MULTI_ERROR_CHECK(pMultiInfo);
	if(ERROR_NON != ErrorInfo){
		pInfo->nBbpStatus = ErrorInfo;
		return INC_ERROR;
	}

	for(wLoop = 0 ; wLoop < pMultiInfo->nSetCnt; wLoop++){
		pChInfo[wLoop] = &pMultiInfo->astSubChInfo[wLoop];
		pChInfo[wLoop]->ulRFFreq = pChInfo[0]->ulRFFreq;
	}

	wEnsemble = pInfo->ulFreq == pChInfo[0]->ulRFFreq;

	if(!wEnsemble){
		if(INC_STOP(ucI2CID) != INC_SUCCESS)return INC_ERROR;
		if(INC_READY(ucI2CID, pChInfo[0]->ulRFFreq) != INC_SUCCESS) return INC_ERROR;
		if(INC_SYNCDETECTOR(ucI2CID, pChInfo[0]->ulRFFreq, 0) != INC_SUCCESS) return INC_ERROR;
		if(INC_FICDECODER(ucI2CID, SIMPLE_FIC_ENABLE) != INC_SUCCESS) return INC_ERROR;

		for(wLoop = 0 ; wLoop < pMultiInfo->nSetCnt; wLoop++)
			if(INC_FIC_UPDATE(ucI2CID, pChInfo[wLoop]) != INC_SUCCESS)	return INC_ERROR;

		INC_BUBBLE_SORT(pMultiInfo, INC_START_ADDRESS);
		if(INC_MULTI_START(ucI2CID, pMultiInfo, (INC_UINT8)wEnsemble) != INC_SUCCESS) return INC_ERROR;
	}
	else{
		wData = INC_CMD_READ(ucI2CID, APB_DEINT_BASE+ 0x01);
		INC_CMD_WRITE(ucI2CID, APB_DEINT_BASE+ 0x01, wData & 0xFFEF);
		for(wLoop = 0 ; wLoop < pMultiInfo->nSetCnt ; wLoop++){
			if(INC_FIC_UPDATE(ucI2CID, pChInfo[wLoop]) != INC_SUCCESS)	return INC_ERROR;
		}
		INC_BUBBLE_SORT(pMultiInfo, INC_START_ADDRESS);
		if(INC_MULTI_START(ucI2CID, pMultiInfo, (INC_UINT8)wEnsemble) != INC_SUCCESS) return INC_ERROR;
	}

	INC_AIRPLAY_SETTING(ucI2CID);


	pInfo->ucProtectionLevel = pChInfo[0]->ucProtectionLevel;
	pInfo->ulFreq = pChInfo[0]->ulRFFreq;

	pInfo->stSubChInfo = *pMultiInfo;

	return INC_SUCCESS;
}

INC_UINT8 INC_ENSEMBLE_SCAN(INC_UINT8 ucI2CID, INC_UINT32 ulFreq)
{
	ST_BBPINFO* pInfo;
	pInfo = INC_GET_STRINFO(ucI2CID);
	pInfo->nBbpStatus = ERROR_NON;
	pInfo->ucStop = 0;

	if(INC_STOP(ucI2CID) != INC_SUCCESS) return INC_ERROR;
	if(INC_READY(ucI2CID, ulFreq) != INC_SUCCESS)return INC_ERROR;
	if(INC_SYNCDETECTOR(ucI2CID, ulFreq, 1) != INC_SUCCESS)	return INC_ERROR;
	if(INC_FICDECODER(ucI2CID, SIMPLE_FIC_DISABLE) != INC_SUCCESS)	return INC_ERROR;

	return INC_SUCCESS;
}

INC_UINT8 INC_FIC_RECONFIGURATION_HW_CHECK(INC_UINT8 ucI2CID)
{
	INC_UINT16 wStatus, uiFicLen, wReconf;
	ST_BBPINFO* pInfo;
	pInfo = INC_GET_STRINFO(ucI2CID);

	wStatus = (INC_UINT16)INC_CMD_READ(ucI2CID, APB_VTB_BASE+ 0x00);

	if(!(wStatus & 0x4000)) return INC_ERROR;
	uiFicLen = INC_CMD_READ(ucI2CID, APB_VTB_BASE+ 0x09);

	if(uiFicLen != 384) return INC_ERROR;
	wReconf = (INC_UINT16)INC_CMD_READ(ucI2CID, APB_VTB_BASE+ 0x0A);

	if(wReconf & 0xC0){
		pInfo->ulReConfigTime = (INC_UINT16)(INC_CMD_READ(ucI2CID, APB_VTB_BASE+ 0x0B) & 0xff) * 24;
		return INC_SUCCESS;
	}
	return INC_ERROR;
}

INC_UINT8 INC_STATUS_CHECK(INC_UINT8 ucI2CID)
{
	INC_GET_CER(ucI2CID);
	INC_GET_PREBER(ucI2CID);
	INC_GET_POSTBER(ucI2CID);
	INC_GET_RSSI(ucI2CID);
	INC_GET_ANT_LEVEL(ucI2CID);

	return INC_SUCCESS;
}

INC_UINT16 INC_GET_CER(INC_UINT8 ucI2CID)
{
	INC_UINT16 	uiVtbErr;
	INC_UINT16	uiVtbData;
	ST_BBPINFO* pInfo;
	INC_INT16 nLoop;
	pInfo = INC_GET_STRINFO(ucI2CID);

	uiVtbErr 	= INC_CMD_READ(ucI2CID, APB_VTB_BASE+ 0x06);
	uiVtbData 	= INC_CMD_READ(ucI2CID, APB_VTB_BASE+ 0x08);

    pInfo->uiCER = (!uiVtbData) ? 0 : (INC_UINT16)((INC_UINT32)(uiVtbErr * 10000) / (uiVtbData * 64));
	if(uiVtbErr == 0) pInfo->uiCER = 2000;

	if(pInfo->uiCER <= BER_REF_VALUE) pInfo->auiANTBuff[pInfo->uiInCAntTick++ % BER_BUFFER_MAX] = 0;
	else pInfo->auiANTBuff[pInfo->uiInCAntTick++ % BER_BUFFER_MAX] = (pInfo->uiCER - BER_REF_VALUE);

	for(nLoop = 0 , pInfo->uiInCERAvg = 0; nLoop < BER_BUFFER_MAX; nLoop++)
		pInfo->uiInCERAvg += pInfo->auiANTBuff[nLoop];

	pInfo->uiInCERAvg /= BER_BUFFER_MAX;

	return pInfo->uiCER;
}

INC_UINT8 INC_GET_ANT_LEVEL(INC_UINT8 ucI2CID)
{
	ST_BBPINFO* pInfo;
	pInfo = INC_GET_STRINFO(ucI2CID);
	INC_GET_SNR(ucI2CID);

	if(pInfo->ucSnr > 16)		pInfo->ucAntLevel = 6;
	else if(pInfo->ucSnr > 13)	pInfo->ucAntLevel = 5;
	else if(pInfo->ucSnr > 10)	pInfo->ucAntLevel = 4;
	else if(pInfo->ucSnr > 7)	pInfo->ucAntLevel = 3;
	else if(pInfo->ucSnr > 5)	pInfo->ucAntLevel = 2;
	else if(pInfo->ucSnr > 3)	pInfo->ucAntLevel = 1;
	else						pInfo->ucAntLevel = 0;

	return pInfo->ucAntLevel;
}

INC_DOUBLE32 INC_GET_PREBER(INC_UINT8 ucI2CID)
{
	INC_UINT16 		uiVtbErr;
	INC_UINT16		uiVtbData;
    INC_UINT32		dPreBER;

	uiVtbErr 	= INC_CMD_READ(ucI2CID, APB_VTB_BASE+ 0x06);
	uiVtbData 	= INC_CMD_READ(ucI2CID, APB_VTB_BASE+ 0x08);

    dPreBER = (!uiVtbData) ? 0 : ((INC_UINT32)uiVtbErr / (uiVtbData * 64));
	return dPreBER;
}

INC_DOUBLE32 INC_GET_POSTBER(INC_UINT8 ucI2CID)
{
	INC_UINT16	uiRSErrBit;
	INC_UINT16	uiRSErrTS;
	INC_UINT16	uiError, uiRef;
	ST_BBPINFO* pInfo;
	pInfo = INC_GET_STRINFO(ucI2CID);

	uiRSErrBit 	= INC_CMD_READ(ucI2CID, APB_RS_BASE+ 0x02);
	uiRSErrTS 	= INC_CMD_READ(ucI2CID, APB_RS_BASE+ 0x03);
	uiRSErrBit += (uiRSErrTS * 8);

	if(uiRSErrTS == 0){
		uiError = ((uiRSErrBit * 50)  / 1000);
		uiRef = 0;
		if(uiError > 50) uiError = 50;
	}
	else if((uiRSErrTS > 0) && (uiRSErrTS < 10)){
		uiError = ((uiRSErrBit * 10)  / 2400);
		uiRef = 50;
		if(uiError > 50) uiError = 50;
	}
	else if((uiRSErrTS >= 10) && (uiRSErrTS < 30)){
		uiError = ((uiRSErrBit * 10)  / 2400);
		uiRef = 60;
		if(uiError > 40) uiError = 40;
	}
	else if((uiRSErrTS >= 30) && (uiRSErrTS < 80)){
		uiError = ((uiRSErrBit * 10)  / 2400);
		uiRef = 70;
		if(uiError > 30) uiError = 30;
	}
	else if((uiRSErrTS >= 80) && (uiRSErrTS < 100)){
		uiError = ((uiRSErrBit * 10)  / 2400);
		uiRef = 80;
		if(uiError > 20) uiError = 20;
	}
	else{
		uiError = ((uiRSErrBit * 10)  / 2400);
		uiRef = 90;
		if(uiError > 10) uiError = 10;
	}

	pInfo->ucVber = 100 - (uiError + uiRef);
    pInfo->dPostBER = (INC_UINT32)uiRSErrTS / TS_ERR_THRESHOLD;

	return pInfo->dPostBER;
}

INC_UINT8 INC_GET_SNR(INC_UINT8 ucI2CID)
{
	INC_UINT16	uiFftVal;
	ST_BBPINFO* pInfo;
	pInfo = INC_GET_STRINFO(ucI2CID);
	uiFftVal = INC_CMD_READ(ucI2CID, APB_VTB_BASE+ 0x07);

	if(uiFftVal == 0)		pInfo->ucSnr = 0;
	else if(uiFftVal < 11)	pInfo->ucSnr = 20;
	else if(uiFftVal < 15)	pInfo->ucSnr = 19;
	else if(uiFftVal < 20)	pInfo->ucSnr = 18;
	else if(uiFftVal < 30)	pInfo->ucSnr = 17;
	else if(uiFftVal < 45)	pInfo->ucSnr = 16;
	else if(uiFftVal < 60)	pInfo->ucSnr = 15;
	else if(uiFftVal < 75)	pInfo->ucSnr = 14;
	else if(uiFftVal < 90)	pInfo->ucSnr = 13;
	else if(uiFftVal < 110)	pInfo->ucSnr = 12;
	else if(uiFftVal < 130) pInfo->ucSnr = 11;
	else if(uiFftVal < 150) pInfo->ucSnr = 10;
	else if(uiFftVal < 200) pInfo->ucSnr = 9;
	else if(uiFftVal < 300) pInfo->ucSnr = 8;
	else if(uiFftVal < 400) pInfo->ucSnr = 7;
	else if(uiFftVal < 500) pInfo->ucSnr = 6;
	else if(uiFftVal < 600) pInfo->ucSnr = 5;
	else if(uiFftVal < 700) pInfo->ucSnr = 4;
	else if(uiFftVal < 800) pInfo->ucSnr = 3;
	else if(uiFftVal < 900) pInfo->ucSnr = 2;
	else if(uiFftVal < 1000) pInfo->ucSnr = 1;
	else pInfo->ucSnr = 0;

	return pInfo->ucSnr;
}

INC_INT16 INC_GET_RSSI(INC_UINT8 ucI2CID)
{
	INC_INT16 nLoop, nRSSI = 0;
	INC_INT16 nResolution, nGapVal;
	INC_UINT16 uiAdcValue;
	INC_UINT16 aRFAGCTable[INC_ADC_MAX][3] = {

		{103, 926,  940}, 
		{100,	889,	926},
		{95,  843,  889}, 
		{90,  798,  843}, 
		{85,  751,  798},

		{80,  688,  751}, 
		{75,  597,  688}, 
		{70,  492,  597}, 
		{65,  357,  492}, 
		{60,  296,  357},

		{55,  285,  296}, 
		{50,  273,  285}, 
		{45,  262,  273}, 
		{40,  246,  262}, 
		{35,  222,  246},

		{30,  178,  222}, 
		{25,    0,  178},
	};

	uiAdcValue  = (INC_CMD_READ(ucI2CID, APB_RF_BASE+ 0x74) >> 5) & 0x3FF;
	//    uiAdcValue  = (INC_INT16)(1.17302 * (INC_DOUBLE32)uiAdcValue );
		uiAdcValue	= (INC_INT16)((117302 * (INC_UINT32)uiAdcValue)/100000);


	if(uiAdcValue >= aRFAGCTable[0][1])
		nRSSI = (INC_INT16)aRFAGCTable[0][0];
	else if(uiAdcValue <= aRFAGCTable[INC_ADC_MAX-1][1])
		nRSSI = (INC_INT16)aRFAGCTable[INC_ADC_MAX-1][0];
	else 
	{
		for(nLoop = 0 ; nLoop < INC_ADC_MAX; nLoop++)
		{
			if(uiAdcValue >= aRFAGCTable[nLoop][2]) continue;
			if(uiAdcValue < aRFAGCTable[nLoop][1]) continue; 

			nResolution = (aRFAGCTable[nLoop][2] - aRFAGCTable[nLoop][1]) / 5;
			nGapVal = aRFAGCTable[nLoop][2] - uiAdcValue;
			nRSSI = (aRFAGCTable[nLoop][0]) - (nGapVal/nResolution);
			break;
		}
	}

	return nRSSI;
}

INC_UINT16 INC_GET_SAMSUNG_BER(INC_UINT8 ucI2CID)
{
	INC_UINT32	uiVtbErr, uiVtbData;
	INC_INT16	nLoop, nCER;
	INC_INT16	nBERValue;
	ST_BBPINFO* pInfo;

	pInfo = INC_GET_STRINFO(ucI2CID);

	uiVtbErr = INC_CMD_READ(ucI2CID, APB_VTB_BASE+ 0x06);	
	uiVtbData = INC_CMD_READ(ucI2CID, APB_VTB_BASE+ 0x08);

	if(pInfo->ucProtectionLevel != 0){
		//		nBERValue = (INC_INT16)(((INC_DOUBLE32)uiVtbErr*0.089)-30.0);
		nBERValue = (INC_INT16)((((INC_UINT32)uiVtbErr*89)/1000)-30);
		if(nBERValue <= 0)nBERValue = 0;
	}
	else {
		//		nCER = (!uiVtbData) ? 0 : (INC_INT32)((((INC_DOUBLE32)uiVtbErr*0.3394*10000.0)/(uiVtbData*32))-35);
		nCER = (!uiVtbData) ? 0 : (INC_INT32)((((INC_UINT32)uiVtbErr*3394)/(uiVtbData*32))-35);

		if(uiVtbData == 0) nCER = 2000;
		else{
			if(nCER <= 0) nCER = 0;
		}
		nBERValue = nCER;
	}

	pInfo->auiBERBuff[pInfo->uiInCBERTick++ % BER_BUFFER_MAX] = nBERValue;
	pInfo->uiBerSum = 0;
	for(nLoop = 0 ; nLoop < BER_BUFFER_MAX; nLoop++)
	{
		pInfo->uiBerSum += pInfo->auiBERBuff[nLoop];
	}
	pInfo->uiBerSum /= BER_BUFFER_MAX;

	return pInfo->uiBerSum;
}

INC_UINT8 INC_GET_SAMSUNG_ANT_LEVEL(INC_UINT8 ucI2CID)
{
	INC_UINT8	ucLevel = 0;
	INC_UINT16 unBER = INC_GET_CER(ucI2CID);

	if(unBER >= 1400)							ucLevel = 0;
	else if(unBER >= 700 && unBER < 1400)		ucLevel = 1; 
	else if(unBER >= 650 && unBER < 700)		ucLevel = 2;
	else if(unBER >= 550 && unBER < 650)		ucLevel = 3;
	else if(unBER >= 450 && unBER < 550)		ucLevel = 4;
	else if(unBER >= 300 && unBER < 450)		ucLevel = 5;
	else if(unBER < 300)						ucLevel = 6;
#ifndef FEATURE_PREVENT_BUG_FIX_CH14
	else ucLevel = 0;
#endif

	return ucLevel;
}

INC_UINT16 INC_GET_SAMSUNG_BER_FOR_FACTORY_MODE(INC_UINT8 ucI2CID)
{
	INC_UINT32	uiVtbErr, uiVtbData;
	INC_INT16	nLoop, nCER;
	INC_INT16	nBERValue;
	ST_BBPINFO* pInfo;

	pInfo = INC_GET_STRINFO(ucI2CID);

	uiVtbErr = INC_CMD_READ(ucI2CID, APB_VTB_BASE+ 0x06);	
	uiVtbData = INC_CMD_READ(ucI2CID, APB_VTB_BASE+ 0x08);

	if(pInfo->ucProtectionLevel != 0){
		//		nBERValue = (INC_INT16)(((INC_DOUBLE32)uiVtbErr*0.089)-30.0);
		nBERValue = (INC_INT16)((((INC_UINT32)uiVtbErr*89)/1000)-30);
		if(uiVtbData == 0) 
			nBERValue = 2000;
		if(nBERValue <= 0) 
			nBERValue = 0;
	}
	else {
		//		nCER = (!uiVtbData) ? 0 : (INC_INT32)((((INC_DOUBLE32)uiVtbErr*0.3394*10000.0)/(uiVtbData*32))-35);
		nCER = (!uiVtbData) ? 0 : (INC_INT32)((((INC_UINT32)uiVtbErr*3394)/(uiVtbData*32))-35);
		if(uiVtbData == 0) nCER = 2000;
		else{
			if(nCER <= 0) nCER = 0;
		}
		nBERValue = nCER;
	}
	return nBERValue;
}
