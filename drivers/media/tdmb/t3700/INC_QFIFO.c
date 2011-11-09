#include "INC_INCLUDES.h"

#ifdef INC_FIFO_SOURCE_ENABLE

ST_FIFO		g_astChFifo[MAX_CHANNEL_FIFO];

INC_UINT8 INC_QFIFO_INIT(PST_FIFO pFF, INC_UINT32 ulDepth)
{
	if(pFF == INC_NULL) return INC_ERROR;

	memset(pFF, 0, sizeof(ST_FIFO));
	if(ulDepth == 0 || ulDepth >= INC_FIFO_DEPTH) pFF->ulDepth = INC_FIFO_DEPTH + 1;
	else pFF->ulDepth = ulDepth + 1;
	return INC_SUCCESS;
}

INC_UINT32 INC_QFIFO_FREE_SIZE(PST_FIFO pFF)
{
	if(pFF == INC_NULL) return INC_ERROR;

	return (pFF->ulFront >= pFF->ulRear) ?
	  ((pFF->ulRear + pFF->ulDepth) - pFF->ulFront) - 1 : (pFF->ulRear - pFF->ulFront) - 1;
}

INC_UINT32 INC_QFIFO_GET_SIZE(PST_FIFO pFF)
{
	if(pFF == INC_NULL) return INC_ERROR;
	
	return (pFF->ulFront >= pFF->ulRear) ?
		(pFF->ulFront - pFF->ulRear) : (pFF->ulFront + pFF->ulDepth - pFF->ulRear);
}

INC_UINT8 INC_QFIFO_AT(PST_FIFO pFF, INC_UINT8* pData, INC_UINT32 ulSize)
{
	INC_UINT32 ulLoop, ulOldRear;
	
	if(pFF == INC_NULL || pData == INC_NULL || ulSize > INC_QFIFO_GET_SIZE(pFF)) 
		return INC_ERROR;

	ulOldRear = pFF->ulRear;
	for(ulLoop = 0 ; ulLoop < ulSize; ulLoop++)	{
		pData[ulLoop] = pFF->acBuff[ulOldRear++];
		ulOldRear %= pFF->ulDepth;
	}
	return INC_SUCCESS;
}

INC_UINT8 INC_QFIFO_ADD(PST_FIFO pFF, INC_UINT8* pData, INC_UINT32 ulSize)
{
	INC_UINT32 ulLoop;

	if(pFF == INC_NULL || pData == INC_NULL || ulSize > INC_QFIFO_FREE_SIZE(pFF)) 
		return INC_ERROR;

	for(ulLoop = 0 ; ulLoop < ulSize; ulLoop++)	{
		pFF->acBuff[pFF->ulFront++] = pData[ulLoop];
		pFF->ulFront %= pFF->ulDepth;
	}
	return INC_SUCCESS;
}

INC_UINT8 INC_QFIFO_BRING(PST_FIFO pFF, INC_UINT8* pData, INC_UINT32 ulSize)
{
	INC_UINT32 ulLoop;

	if(pFF == INC_NULL || pData == INC_NULL || ulSize > INC_QFIFO_GET_SIZE(pFF)) 
		return INC_ERROR;

	for(ulLoop = 0 ; ulLoop < ulSize; ulLoop++)	{
		pData[ulLoop] = pFF->acBuff[pFF->ulRear++];
		pFF->ulRear %= pFF->ulDepth;
	}
	return INC_SUCCESS;
}

void INC_MULTI_SORT_INIT(void)
{
	INC_UINT32 	ulLoop;
	ST_FIFO*	pFifo;

	for(ulLoop = 0 ; ulLoop < MAX_CHANNEL_FIFO; ulLoop++){
		pFifo = INC_GET_CHANNEL_FIFO((MULTI_CHANNEL_INFO)ulLoop);
		INC_QFIFO_INIT(pFifo, 0);
	}
}

ST_FIFO* INC_GET_CHANNEL_FIFO(MULTI_CHANNEL_INFO ucIndex)
{
	if(ucIndex >= MAX_CHANNEL_FIFO) return INC_NULL;
	return &g_astChFifo[ucIndex];
}

INC_UINT8 INC_GET_CHANNEL_DATA(MULTI_CHANNEL_INFO ucIndex, INC_UINT8* pData, INC_UINT32 ulSize)
{
	ST_FIFO* pstFF;
	if(ucIndex >= MAX_CHANNEL_FIFO) return INC_ERROR;
	pstFF = INC_GET_CHANNEL_FIFO(ucIndex);
	INC_QFIFO_BRING(pstFF, pData, ulSize);
	return INC_SUCCESS;
}

INC_UINT32	INC_GET_CHANNEL_COUNT(MULTI_CHANNEL_INFO ucIndex)
{
	ST_FIFO* pFifo;
	if(ucIndex >= MAX_CHANNEL_FIFO) return 0;
	pFifo = INC_GET_CHANNEL_FIFO(ucIndex);
	return (INC_UINT32)INC_QFIFO_GET_SIZE(pFifo);
}

ST_HEADER_INFO INC_HEADER_CHECK(ST_FIFO* pMainFifo)
{
	INC_UINT8	acBuff[HEADER_SERACH_SIZE];
	INC_INT16	nLoop;
	
	if(INC_QFIFO_GET_SIZE(pMainFifo) < HEADER_SERACH_SIZE) 
		return INC_HEADER_SIZE_ERROR;
	
	INC_QFIFO_AT(pMainFifo, acBuff, HEADER_SERACH_SIZE);

	for(nLoop = 0; nLoop < (HEADER_SERACH_SIZE-1); nLoop++) {
		if(acBuff[nLoop] == HEADER_ID_0x33 && acBuff[nLoop+1] == HEADER_ID_0x00){
			INC_QFIFO_BRING(pMainFifo, acBuff, nLoop);
			
			if(INC_QFIFO_GET_SIZE(pMainFifo) < HEADER_SERACH_SIZE) 
				return INC_HEADER_SIZE_ERROR;
			
			return INC_HEADER_GOOD;
		}
	}

	if(nLoop == (HEADER_SERACH_SIZE-1))
		INC_QFIFO_BRING(pMainFifo, acBuff, HEADER_SERACH_SIZE);

	return INC_HEADER_NOT_SEARACH;
}

INC_UINT8 INC_MULTI_FIFO_PROCESS(INC_UINT8* pData, INC_UINT32 ulSize)
{
	INC_UINT8	acBuff[HEADER_SERACH_SIZE], cIndex, bIsData = INC_ERROR;
	INC_UINT16 	aunChSize[MAX_CHANNEL_FIFO], unTotalSize;
	ST_HEADER_INFO stHeaderStatus;
	
	ST_FIFO*	pFifo;
	ST_FIFO*	pMainFifo;
	
	pMainFifo = INC_GET_CHANNEL_FIFO(MAIN_INPUT_DATA);
	INC_QFIFO_ADD(pMainFifo, pData, ulSize);
	
	while(1){
		
		if(INC_HEADER_CHECK(pMainFifo) != INC_HEADER_GOOD) {
			if(bIsData == INC_SUCCESS) return INC_SUCCESS;
			return INC_ERROR;
		}
		
		//FIFO에서 헤더정보 만큼 데이터를 꺼내오고...
		INC_QFIFO_BRING(pMainFifo, acBuff, MAX_HEADER_SIZE);

		//헤더의 전체크기를 계산하고...
		unTotalSize = (INC_UINT16)(acBuff[4] << 8) | acBuff[5];
		unTotalSize = (unTotalSize & 0x8000) ? (unTotalSize << 1) : unTotalSize;
		
		//각각의 채널별 크기를 구하고...
		aunChSize[0] = ((acBuff[0x8] << 8 | acBuff[0x9]) & 0x3FF) * 2;
		aunChSize[1] = ((acBuff[0xA] << 8 | acBuff[0xB]) & 0x3FF) * 2;
		aunChSize[2] = ((acBuff[0xC] << 8 | acBuff[0xD]) & 0x3FF) * 2;
		aunChSize[3] = ((acBuff[0xE] << 8 | acBuff[0xF]) & 0x3FF) * 2;
		
		//각가의 채널별 크기 만큼 분류한다...
		for(cIndex = 1; cIndex < MAX_CHANNEL_FIFO; cIndex++){
			if(!aunChSize[cIndex-1]) continue;
			pFifo = INC_GET_CHANNEL_FIFO((MULTI_CHANNEL_INFO)cIndex);
			INC_QFIFO_BRING(pMainFifo, acBuff, (INC_UINT32)aunChSize[cIndex-1]);
			INC_QFIFO_ADD(pFifo, acBuff, (INC_UINT32)aunChSize[cIndex-1]);
		}

		bIsData = INC_SUCCESS;
	}
	
	return INC_SUCCESS;
}



#endif

