
#include <linux/kernel.h>

#include <linux/errno.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/string.h>

#include <linux/workqueue.h>
#include <linux/irq.h>
#include <asm/mach/irq.h>
#include <linux/interrupt.h>

#include <asm/io.h>
#include <mach/gpio.h>

#include <linux/fs.h>
#include <asm/uaccess.h>

#include "tdmb.h"
#include "gdm_headers.h"
#include "tdmb_drv_gct.h"


//#define GCT_DOWNLOAD_CHECK
//#define FEATURE_GCT_DM

#define INT_DAB_FIC                (0x0001)
#define INT_DAB_MSC                (0x0002)
#define INT_DAB_MSC_OV             (0x0004)
#define INT_DAB_SYNC_LOSS          (0x0008)
#define INT_DAB_FIC_RECONF         (0x0010)
#define INT_DAB_INVALID_SUBCH      (0x0020)
#define INT_DAB_DM			 	 	(0x8000)

#define TDMB_DRV_GCT_BIN_CODE_SIZE 40*1024

#define TDMB_DRV_GCT_CODE_BIN "/system/main.bin_iram_aligned.bin"

#define GDM_SPI_READ_PKT_NUM            (40) //(20)
//-------------------------------------------------------------------
// DBG
//#define TDMB_DEBUG

#ifdef TDMB_DEBUG
#define DPRINTK(x...) printk("TDMB " x)
#else /* TDMB_DEBUG */
#define DPRINTK(x...)  /* null */
#endif /* TDMB_DEBUG */

// dbg features
#ifdef TDMB_DEBUG
#define FEATURE_GCT_DEBUG
#define GCT_DOWNLOAD_CHECK
#endif

#ifdef FEATURE_GCT_DEBUG
// klaatu - check later
#define GCT_MSG_HIGH printk
#else
#define GCT_MSG_HIGH(x...)  /* null */
#endif


const unsigned int KOREA_BAND_TABLE[] = {
	175280,177008,178736,
	181280,183008,184736,
	187280,189008,190736,
	193280,195008,196736,
	199280,201008,202736,
	205280,207008,208736,
	211280,213008,214736
};

unsigned int tdmb_gct_fic_parsing = 0;

#ifndef COMMERCIAL_RELEASE
char * tdmb_drv_gct_bin_code = NULL;
unsigned int tdmb_drv_gct_bin_code_size = NULL;
#endif

static signed short rssi=100;
static unsigned long BER=0,PER=0;
static unsigned char gBufDM[512];

unsigned char  m_nSubChID = 65;
unsigned long  m_dEnsembleFreq = 208736000;

extern int TDMB_AddDataToRing(unsigned char* pData, unsigned long dwDataSize);

extern char FIG_Done_count;
static unsigned int fic_done_count = 0;

signed short TDMB_drv_gct_get_rssi(void) { return rssi; }
unsigned long TDMB_drv_gct_get_ber(void) { return BER; }
unsigned long TDMB_drv_gct_get_per(void) { return PER; }

// [ temp 
static unsigned short const tBER_Table[6] = { 1200, 1100, 1000, 900, 800, 700}; 


void gdm_hal_read_msc( void );// klaatu test

unsigned int TDMB_drv_gct_get_antenna(void)
{
	unsigned char i;
	for(i=0;i<6;i++)
	{
		if(BER > tBER_Table[i])
			return i;
	}
	return 6;
}
// ]

#if 0 // klaatu - check later - related to upgrade
void TDMB_drv_gct_read_binary_info
(
  unsigned char *buf, 
  unsigned int   buf_size,
  char          *info
)
{
  char *TDMB_HEX="0123456789";
  unsigned int i = 0,multiple = 1;

  memset(info, 0, 13);
  for( i = 0 ; i < 12 ; i++ )
  {
    info[i] = TDMB_HEX[buf[buf_size-12+i-1]-0x30];
  }
}
#endif


void TDMB_drv_gct_download()
{
	int ret = 0;
	int k =0;
	int i=0;

	G_Uint32 chip_id = 0;
	G_Uint16 dev_id = 0;
	G_Uint16 chip_ver = GDM_CHIP_VER_R0;

	struct file *fp_irom = NULL;

	mm_segment_t oldfs;

	unsigned char *tdmb_drv_gct_bin_code = NULL ;

	G_Uint8 *code_bin;
#ifdef GCT_DOWNLOAD_CHECK
	G_Uint8 *verify_buf = NULL;
#endif

	//----------------------------------
	// read chip id
	gdm_read_chip_id(&chip_id);
	DPRINTK("TDMB_INIT => chip_id(%x)\r\n", chip_id);

	if(chip_id != 0x7024777)
	{
		return G_ERROR_ID;
	}

	//----------------------------------
	// read chip ver
	DPRINTK("reading chip ver.. \n");
	gdm_read_chip_ver(&chip_ver);
	DPRINTK("gdm_read_chip_ver (chip_ver : %x) \r\n", chip_ver);

	gdm_fic_init_db(DAB_0_PATH);

	// -----------------------------------------------------------
	// prepare binaries to download 

	if (!tdmb_drv_gct_bin_code )
	{
		tdmb_drv_gct_bin_code = kmalloc(TDMB_DRV_GCT_BIN_CODE_SIZE, GFP_ATOMIC);

		if (tdmb_drv_gct_bin_code == NULL)
		{
			DPRINTK("GCT_FIRMWARE kmalloc Fail! \r\n");
			return G_ERROR_MEMORY_INSTART ;
		}
	}

	// ---------------- read binary
	DPRINTK("GDM7024 FIRMWARE : %s \r\n",TDMB_DRV_GCT_CODE_BIN ) ;
	fp_irom = filp_open(TDMB_DRV_GCT_CODE_BIN ,O_RDONLY, 0) ;
	if (fp_irom)
	{
		oldfs = get_fs();
		set_fs(KERNEL_DS);
		ret = fp_irom->f_op->read(fp_irom,tdmb_drv_gct_bin_code, TDMB_DRV_GCT_BIN_CODE_SIZE, &fp_irom->f_pos);
		filp_close(fp_irom, current->files);	
		set_fs(oldfs);
	}
	else
	{
		for (i = 0 ; i < 10 ; i ++)
		{
			DPRINTK("firmware download ERROR!!(iram) \r\n") ;
		}
		return G_ERROR_FIRMWARE_NOTFOUND ;
	}

	code_bin = (G_Uint8 *)tdmb_drv_gct_bin_code; // klaatu test

	// ------------------------------------------------------
	// download binary
	DPRINTK("call gdm_program_download() \n");
	if( G_SUCCESS != gdm_program_download((G_Uint8*)code_bin,TDMB_DRV_GCT_BIN_CODE_SIZE))
		return G_ERROR;

	DPRINTK("gdm_program_download() - suceeded!!!!!!!!!!!!!!!!!!!!!!!!!!! \n");

	// ------------------------------------------------------------
	// check whether binary is downloaded with no error
#ifdef GCT_DOWNLOAD_CHECK
	verify_buf = kmalloc(TDMB_DRV_GCT_BIN_CODE_SIZE, GFP_ATOMIC);
	if(verify_buf == NULL)
	{
		return G_ERROR_MEMORY_INSTART;
		}
	
	if( gdm_program_download_verify((G_Uint8*)code_bin,
				TDMB_DRV_GCT_BIN_CODE_SIZE,
				verify_buf,
				TDMB_DRV_GCT_BIN_CODE_SIZE) != G_SUCCESS)
	{
		DPRINTK("#### GCT ########## download error ######### \n");
		return G_ERROR;
	}
	kfree(verify_buf);
	printk("[%s:%d] Binary verification success!! \n", __FUNCTION__, __LINE__);
#endif

	DPRINTK("xx : %x \n", tdmb_drv_gct_bin_code);
	kfree(tdmb_drv_gct_bin_code);

	DPRINTK(" mask to use : %x", ~(INT_DAB_MSC
							| INT_DAB_INVALID_SUBCH
							| INT_DAB_FIC_RECONF
							| INT_DAB_SYNC_LOSS
							| INT_DAB_MSC_OV ));

	msleep(50);
	if( G_SUCCESS != gdm_run())
	{
		DPRINTK("gdm_run() Failed !!!!!!!!!!!!!!!!!!!!!!!\n");
		return ;
	}
	
	DPRINTK("start reading device id \n");
	if( G_ERROR == gdm_drv_cr_get_device_id(&dev_id) )
	{
		DPRINTK("Error in reading device id %x \n", dev_id);
	}
	else
	{
		DPRINTK("device id : %x \n", dev_id);
	}

	msleep(50);

	//  DM
#ifdef FEATURE_GCT_DM // klaatu
  gdm_drv_dab_set_encap_dm_fic(DAB_0_PATH, DAB_ENCAP_DM_ENABLE, DAB_ENCAP_FIC_ENABLE);
#else
  gdm_drv_dab_set_encap_dm_fic(DAB_0_PATH, DAB_ENCAP_DM_DISABLE, DAB_ENCAP_FIC_ENABLE);
#endif
	TDMB_drv_gct_EnableDM(DM_FULL_MODE, 500);
	//gdm_drv_dab_set_enable_serial_ts(DAB_0_PATH, 1);
	
} /* TDMB_drv_gct_download */

/*-------------------------------------------------------------------------

Function    : TDMB_DRV_GCT_DAB_DECAPSULATOR

Description :

Parameter   :

Return      :
-------------------------------------------------------------------------*/
signed char TDMB_drv_gct_dab_decapsulator
(
  unsigned char        *pSrc,
  dab_encap_buf_e_type *BufID,
  unsigned char        *SubChID,	
  unsigned char       **pOutAddr,
  unsigned int         *Size
)
{
 
	if(*pSrc == 0x80 && *(pSrc+1) == 0x01)
	{
		//FIC
		*BufID = DAB_ENCAP_BUF_FIC;
		*SubChID = 0x00;
		*pOutAddr = pSrc+4;
		*Size = 128;

		return G_SUCCESS;
	}
	else if((*pSrc & 0x80) == 0x00)
	{
		//TS
		*BufID = DAB_ENCAP_BUF_TS;
		*SubChID = *pSrc & 0x3f;
		*pOutAddr = pSrc;
		*Size = 188;
		
		//check & change header
		if(*pSrc & 0x40)
			*pSrc = 0x40; //Sync Byte Error. Change Sync Byte except 0x47
		else
			*pSrc = 0x47;

		return G_SUCCESS;
	}
	else if(*pSrc & 0x80 && *(pSrc+1) == 0x00)
	{
		//NTS
		*BufID = DAB_ENCAP_BUF_NTS;
		*SubChID = *pSrc & 0x3f;
		*pOutAddr = pSrc+4;
		*Size = 184;

		return G_SUCCESS;
	} 
	else if(*pSrc == 0x80 && *(pSrc+1) == 0x02)
	{
		//DM
		*BufID = DAB_ENCAP_BUF_DM;
		*SubChID = 0x00;
		*pOutAddr = pSrc+4;
		*Size = 128;

		return G_SUCCESS;
	}
	else
	{
		GCT_MSG_HIGH("Error %x,%x,%x\n",pSrc[0],pSrc[1],pSrc[2]);
		return G_ERROR;
	}
}

//test
unsigned int fic_done_max_count = 100;
unsigned char tdmb_gct_read_msc_buffer[50*188 + 32];
static void __SubProcTSIFData(unsigned char *pData, unsigned int nDataSize);
unsigned int fic_decode_done = FALSE; 
static int __SortChannel(const void *a, const void *b)
{
	SubChInfoType *x = (SubChInfoType*) a;
	SubChInfoType *y = (SubChInfoType*) b;

	if (x->SubChID < y->SubChID)
    {
        return -1;
    }
    else if (x->SubChID > y->SubChID)
    {
        return 1;
    }
    else
    {
    	return 0;
    }
}

static int RTrim(char* s)
{
    int i;
    int len = strlen(s);
    for (i = len - 1; i > 0; i--)
    {
        if (s[i] == ' ')
        {
            s[i] = '\0';
        }
        else
        {
            break;
        }
    }
    return i;
}

int UpdateEnsembleInfo(EnsembleInfoType* ensembleInfo, unsigned long freq)
{
    extern void qsort(void *base, size_t nmemb, size_t size, int(*compar)(const void *, const void *));
    extern gdm_fic_ensbl_s_type gdm_fic_ensbl[MAX_DAB_NUM];
    unsigned char TMID, Type;
    int i;
    int nSubChIdx = 0;

    ensembleInfo->EnsembleFrequency = freq;
    ensembleInfo->EnsembleID        = gdm_fic_ensbl[0].eid;
    strncpy((char *)ensembleInfo->EnsembleLabelCharField, (char *)gdm_fic_ensbl[0].ensemble_label, ENSEMBLE_LABEL_SIZE_MAX);
      RTrim((char *)ensembleInfo->EnsembleLabelCharField);
    for (i = 0; i < gdm_fic_ensbl[0].num_of_svc ; i++)
    {
        TMID = gdm_fic_ensbl[0].svc[i].sc[0].tmid;
        Type = gdm_fic_ensbl[0].svc[i].sc[0].scty_tmp;

        if ( (TMID == TMID_MSC_STREAM_DATA && Type == DSCTy_TDMB) || (TMID == TMID_MSC_STREAM_AUDIO) )
        {
            ensembleInfo->SubChInfo[nSubChIdx].SubChID      = gdm_fic_ensbl[0].svc[i].sc[0].sch.sch_id;
            ensembleInfo->SubChInfo[nSubChIdx].StartAddress = gdm_fic_ensbl[0].svc[i].sc[0].sch.sta_add;
            ensembleInfo->SubChInfo[nSubChIdx].TMId         = TMID;
            ensembleInfo->SubChInfo[nSubChIdx].Type         = Type;
            ensembleInfo->SubChInfo[nSubChIdx].ServiceID    = gdm_fic_ensbl[0].svc[i].sid;
            memcpy(ensembleInfo->SubChInfo[nSubChIdx].ServiceLabel, gdm_fic_ensbl[0].svc[i].label, SERVICE_LABEL_SIZE_MAX);
            RTrim((char *)ensembleInfo->SubChInfo[nSubChIdx].ServiceLabel);
            nSubChIdx++;
        }
    }
    
    ensembleInfo->TotalSubChNumber = nSubChIdx;

//    if ( nSubChIdx > 1 )
//    {
//        qsort(ensembleInfo->SubChInfo, nSubChIdx, sizeof(SubChInfoType), __SortChannel);
//    }

    return nSubChIdx;
}

/*-------------------------------------------------------------------------

Function    : TDMB_DRV_GCT_SCAN

Description :

Parameter   :

Return      :
-------------------------------------------------------------------------*/
char TDMB_drv_gct_scan(unsigned long FIG_Frequency)
{
  unsigned int freq_idx = 0;
  unsigned int wSyncRefTime;
  unsigned int uiTimeOut = 0;
  
  dab_demod_status_e_type demod_status;
  dab_sync_status_e_type sync_status;

  do{
  	if(FIG_Frequency/1000 == KOREA_BAND_TABLE[freq_idx])
  		break;
  }while( freq_idx++ < 0x14 );

  wSyncRefTime = 100;//800;
  
  gdm_dab_stop(DAB_0_PATH);
  mdelay(100);

  gdm_fic_init_db(DAB_0_PATH);

#ifdef FEATURE_GCT_DM // klaatu
  gdm_drv_dab_set_encap_dm_fic(DAB_0_PATH, DAB_ENCAP_DM_ENABLE, DAB_ENCAP_FIC_ENABLE);
#else
  gdm_drv_dab_set_encap_dm_fic(DAB_0_PATH, DAB_ENCAP_DM_DISABLE, DAB_ENCAP_FIC_ENABLE);
#endif

  gdm_drv_dab_set_predefined_freq(DAB_0_PATH, freq_idx, DAB_BAND_KOREAN_TDMB);
  
  GCT_MSG_HIGH("SYNC START %d,%d\n",freq_idx,FIG_Frequency);
   
  //gdm_drv_dab_set_enable_serial_ts(DAB_0_PATH, 1); // GCT
  gdm_dab_scan(DAB_0_PATH);
  tdmb_gct_fic_parsing = TRUE;

#if 1  
  fic_decode_done = FALSE;
  fic_done_count = 0;
  
  // klaatu - check later
  //while(!DoesItCancelOperation())
  while( 1 ) // TODO : klaatu - stay in loop because there's no cancel operation yet
  {
    mdelay(10);
  
    gdm_dab_get_demod_status(DAB_0_PATH, &demod_status, &sync_status);    
    GCT_MSG_HIGH("# GCT # Demod Status = %d,%d[%d]\n", demod_status,sync_status,freq_idx);

    if(demod_status == DAB_DEMOD_DECODING_S &&  fic_decode_done == TRUE)
    {    
      GCT_MSG_HIGH("GCT SYNC GOOD\n");
      gdm_dab_stop(DAB_0_PATH); //GT_ssjeong_CK17
//      gdm_drv_dab_set_enable_serial_ts(DAB_0_PATH, 0);
//      gdm_fic_init_db(DAB_0_PATH);
      return TRUE;           
    }
    
    if ( tdmb_gct_fic_parsing == FALSE )
    {
      return FALSE;  
    }
    
    if(sync_status == DAB_SYNC_FAILURE )
    {
      GCT_MSG_HIGH("GCT SYNC FAIL\n");
      gdm_dab_stop(DAB_0_PATH);
      gdm_drv_dab_set_enable_serial_ts(DAB_0_PATH, 0);
//      gdm_fic_init_db(DAB_0_PATH);

	  //TDMB_NotifyScanNext();// klaatu - check later 
      tdmb_gct_fic_parsing = FALSE;
      return FALSE;
    }
  
    if(++uiTimeOut > 200) // wait 2sec per channel
    {
      GCT_MSG_HIGH("GCT SYNC TIMEOUT\n"); 
      gdm_dab_stop(DAB_0_PATH);

//      gdm_drv_dab_set_enable_serial_ts(DAB_0_PATH, 0);
	  // TDMB_NotifyScanNext();// klaatu - check later 
      tdmb_gct_fic_parsing = FALSE;
      break;
    }
  }
  
//  gdm_dab_stop(DAB_0_PATH);
#endif
  return FALSE;
}

char TDMB_drv_gct_scan_stop(char bSuccess)
{
    gdm_dab_stop(DAB_0_PATH);
    gdm_drv_dab_set_enable_serial_ts(DAB_0_PATH, 0);
    GCT_MSG_HIGH("ENCAP BUF FIC STOP--------------------------- \n");
//    gdm_fic_init_db(DAB_0_PATH);
    if ( bSuccess )
    {
        fic_decode_done = TRUE;
    }
    else
    {
        tdmb_gct_fic_parsing = FALSE;
    }

    return TRUE;  
}

/*-------------------------------------------------------------------------

Function    : TDMB_DRV_GCT_FIC_DECODE

Description :

Parameter   :

Return      :
-------------------------------------------------------------------------*/
void TDMB_drv_gct_fic_decode
(
  unsigned char *fic_data,
  unsigned int   size
)
{
  G_Int32 iRetValue;
  unsigned char fic_tmp[386];

  if(tdmb_gct_fic_parsing == FALSE) return;
#if 0
  memset ( fic_tmp, 0, sizeof(fic_tmp));
  fic_tmp[0] = fic_tmp[1] = 0xcf;
  memcpy(&fic_tmp[2], fic_data, size );
  TDMB_AddDataToRing(fic_tmp, 188);
  return;
#else
  iRetValue = gdm_fic_run_decoder(DAB_0_PATH, fic_data, size);

  if(iRetValue == 0x03)
  {
    fic_decode_done = TRUE;
    GCT_MSG_HIGH("ENCAP BUF FIC DONE--------------------------- \n");
#if 0    
    //gdm_dab_stop(DAB_0_PATH); //GT_ssjeong_CK17
    gdm_drv_dab_set_enable_serial_ts(DAB_0_PATH, 0);
    gdm_fic_init_db(DAB_0_PATH);
    tdmb_gct_fic_parsing = FALSE;
#endif    
  }
  else if(fic_done_count++ > fic_done_max_count)
  {
    gdm_dab_stop(DAB_0_PATH);
    gdm_drv_dab_set_enable_serial_ts(DAB_0_PATH, 0);
    GCT_MSG_HIGH("ENCAP BUF FIC STOP--------------------------- \n");
    gdm_fic_init_db(DAB_0_PATH);
    tdmb_gct_fic_parsing = FALSE;

    // TDMB_NotifyScanNext();// klaatu - check later 
  }
#endif
}

#ifdef GCT_DEBUG
void print_buf(char *name, unsigned char *test_buf, int len)
{
	int i;
	GCT_MSG_HIGH("%s ", name);
	for(i = 0; i < len; i++)
	{
		GCT_MSG_HIGH("%.2X ",test_buf[i]);
	}

	GCT_MSG_HIGH("[%.2X %.2X %.2X %.2X]\n",test_buf[i], test_buf[i+1], test_buf[i+2], test_buf[i+3]);
}

void gdm_test_helper(int read, int total)
{

	unsigned int rp=0,wp=0,base=0;

	unsigned char test_buf[256],test_buf2[256];

	gdm_drv_ghost_mm_read(0x3c037e40,&rp);
	gdm_drv_ghost_mm_read(0x3c037e44,&wp);
	gdm_drv_ghost_mm_read(0x3c037e4c,&base);

	printk("\n=========== each %d bytes read 4 times , total %d ============ \n", read, total);

	GCT_MSG_HIGH("rp[%x] ,wp[%x], msc[%x] \n",rp,wp,base); // base : 0x00000e94

	gdm_drv_ghost_mm_burst_read(0x3c030e94,test_buf,total);

	print_buf("test buf", test_buf, total);

	gdm_drv_ghost_mm_write(0x3c037e40,0x0e94);
	gdm_drv_ghost_mm_read(0x3c037e40,&rp);  
	GCT_MSG_HIGH("rp[%X] \n",rp); // rp : 0x00000e94

	gdm_dab_read_msc(DAB_0_PATH,test_buf2, read);
//	GCT_MSG_HIGH("test_buf2-1 %x %x %x %x\n",test_buf2[0],test_buf2[1],test_buf2[2],test_buf2[3]);
	print_buf("test_buf2-1", test_buf2+2, read);
	gdm_drv_ghost_mm_read(0x3c037e40,&rp);  
	GCT_MSG_HIGH("rp[%X]\n",rp); // rp : 0x00000e98


	gdm_dab_read_msc(DAB_0_PATH,test_buf2, read);
	//GCT_MSG_HIGH("test_buf2-2 %x %x %x %x\n",test_buf2[0],test_buf2[1],test_buf2[2],test_buf2[3]);
	print_buf("test_buf2-2", test_buf2+2, read);
	gdm_drv_ghost_mm_read(0x3c037e40,&rp);  
	GCT_MSG_HIGH("rp[%X]\n",rp); // rp : 0x00000e9c


	gdm_dab_read_msc(DAB_0_PATH,test_buf2, read);
	//GCT_MSG_HIGH("test_buf2-3 %x %x %x %x\n",test_buf2[0],test_buf2[1],test_buf2[2],test_buf2[3]);
	print_buf("test_buf2-3", test_buf2+2, read);
	gdm_drv_ghost_mm_read(0x3c037e40,&rp);  
	GCT_MSG_HIGH("rp[%X]\n",rp); // rp : 0x00000ea0


	gdm_dab_read_msc(DAB_0_PATH,test_buf2, read);
	//GCT_MSG_HIGH("test_buf2-4 %x %x %x %x\n",test_buf2[0],test_buf2[1],test_buf2[2],test_buf2[3]);
	print_buf("test_buf2-4", test_buf2+2, read);
	gdm_drv_ghost_mm_read(0x3c037e40,&rp);  
	GCT_MSG_HIGH("rp[%X]\n",rp); // rp : 0x00000ea4
}

void gdm_test()
{
	//unsigned int rp=0,wp=0,base=0;

	//unsigned char test_buf[256],test_buf2[256];
#if 0
	gdm_dab_change_freq(0 , 0 , 1 , 0x321e );
	gdm_dab_config_subch(0x2304, 1, 4);
	gdm_dab_run(DAB_0_PATH);

	mdelay(1000*10); // 10sec
#endif
	gdm_dab_stop(DAB_0_PATH);

	gdm_test_helper( 4, 16);
	gdm_test_helper( 8, 32);
	gdm_test_helper( 10, 40);
	gdm_test_helper( 12, 48);
	gdm_test_helper( 16, 52);
	gdm_test_helper( 32, 128);

#if 0
	gdm_drv_ghost_mm_read(0x3c037e40,&rp);
	gdm_drv_ghost_mm_read(0x3c037e44,&wp);
	gdm_drv_ghost_mm_read(0x3c037e4c,&base);


	GCT_MSG_HIGH("rp[%x] ,wp[%x], msc[%x] \n",rp,wp,base); // base : 0x00000e94

	gdm_drv_ghost_mm_burst_read(0x3c030e94,test_buf,16);

#if 0
	GCT_MSG_HIGH("test buf %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x \n",
			test_buf[0] ,test_buf[1] ,test_buf[2] ,test_buf[3],
			test_buf[4] ,test_buf[5] ,test_buf[6] ,test_buf[7],
			test_buf[8] ,test_buf[9] ,test_buf[10],test_buf[11],
			test_buf[12],test_buf[13],test_buf[14],test_buf[15]);
#else
	print_buf("test buf(16) ", test_buf, 16);
#endif

	gdm_drv_ghost_mm_write(0x3c037e40,0x0e94);
	gdm_drv_ghost_mm_read(0x3c037e40,&rp);  
	GCT_MSG_HIGH("rp[%X] \n",rp); // rp : 0x00000e94

	gdm_dab_read_msc(DAB_0_PATH,test_buf2,4);
	GCT_MSG_HIGH("test_buf(16)2-1 %x %x %x %x\n",test_buf2[0],test_buf2[1],test_buf2[2],test_buf2[3]);
	gdm_drv_ghost_mm_read(0x3c037e40,&rp);  
	GCT_MSG_HIGH("rp[%X]\n",rp); // rp : 0x00000e98


	gdm_dab_read_msc(DAB_0_PATH,test_buf2,4);
	GCT_MSG_HIGH("test_buf(16)2-2 %x %x %x %x\n",test_buf2[0],test_buf2[1],test_buf2[2],test_buf2[3]);
	gdm_drv_ghost_mm_read(0x3c037e40,&rp);  
	GCT_MSG_HIGH("rp[%X]\n",rp); // rp : 0x00000e9c


	gdm_dab_read_msc(DAB_0_PATH,test_buf2,4);
	GCT_MSG_HIGH("test_buf(16)2-3 %x %x %x %x\n",test_buf2[0],test_buf2[1],test_buf2[2],test_buf2[3]);
	gdm_drv_ghost_mm_read(0x3c037e40,&rp);  
	GCT_MSG_HIGH("rp[%X]\n",rp); // rp : 0x00000ea0


	gdm_dab_read_msc(DAB_0_PATH,test_buf2,4);
	GCT_MSG_HIGH("test_buf(16)2-4 %x %x %x %x\n",test_buf2[0],test_buf2[1],test_buf2[2],test_buf2[3]);
	gdm_drv_ghost_mm_read(0x3c037e40,&rp);  
	GCT_MSG_HIGH("rp[%X]\n",rp); // rp : 0x00000ea4

#endif
}

int gdm_read_test()
{
#define TEST_MM_READ_SIZE (188*15)

  unsigned short stored_ts;
  G_Uint32 rp=0,wp=0,base=0;
  static G_Uint8 *g_gct_mm_read_buf = NULL, *g_gct_burst_read_buf = NULL;
  int i;

  if(gdm_drv_dab_get_stored_msc_size(DAB_0_PATH,&stored_ts) != G_SUCCESS)
  {
    return -1;
  }
  if(stored_ts < 15) 
  {
    return -3;
  }

  gdm_dab_stop(DAB_0_PATH);

  msleep(100); // 1sec

  gdm_drv_ghost_mm_read(0x3c037e40,&rp);
  gdm_drv_ghost_mm_read(0x3c037e44,&wp);
  gdm_drv_ghost_mm_read(0x3c037e4c,&base);

  GCT_MSG_HIGH("rp[%X] ,wp[%X], msc[%X]",rp,wp,base); // base : 0x00000e94

  g_gct_mm_read_buf    = kmalloc(TEST_MM_READ_SIZE + 32, GFP_ATOMIC);
  g_gct_burst_read_buf = kmalloc(TEST_MM_READ_SIZE + 32, GFP_ATOMIC);

  if(g_gct_mm_read_buf == NULL || g_gct_burst_read_buf == NULL)
  {
    if(g_gct_mm_read_buf)
      kfree(g_gct_mm_read_buf);

    if(g_gct_burst_read_buf)
      kfree(g_gct_burst_read_buf);

	printk("[%s] alloc error \n", __FUNCTION__);
    return -2;
  }

  for(i = 0; i < 5; i++)
  {
	  gdm_drv_ghost_mm_burst_read(0x3c030e94,g_gct_mm_read_buf,TEST_MM_READ_SIZE);

	  gct_dump_ts_data("/system/mm_read.ts", g_gct_mm_read_buf, TEST_MM_READ_SIZE);

	  // Initialize MSC_RP.
	  gdm_drv_ghost_mm_write(0x3c037e40,0x0e94);
	  gdm_drv_ghost_mm_read(0x3c037e40,&rp);  
	  GCT_MSG_HIGH("rp[%X]",rp); // rp : 0x00000e94
  }
  gct_dump_ts_data("/system/mm_read.ts", g_gct_mm_read_buf, 0);

  for(i = 0; i < 5; i++)
  {
	  gdm_dab_read_msc(DAB_0_PATH,g_gct_burst_read_buf,TEST_MM_READ_SIZE);

	  gct_dump_ts_data("/system/msc_read.ts", g_gct_burst_read_buf, TEST_MM_READ_SIZE);

	  // Initialize MSC_RP.
	  gdm_drv_ghost_mm_write(0x3c037e40,0x0e94);
	  gdm_drv_ghost_mm_read(0x3c037e40,&rp);  
	  GCT_MSG_HIGH("rp[%X]",rp); // rp : 0x00000e94
  }	
  gct_dump_ts_data("/system/msc_read.ts", g_gct_burst_read_buf, 0);

  kfree(g_gct_mm_read_buf);
  kfree(g_gct_burst_read_buf);

  return 0;
}


int gdm_read_test2( void )
{
#define TEST_MM_READ_SIZE (188*15)

  unsigned short stored_ts;
  G_Uint32 rp=0,wp=0,base=0;
  static G_Uint8 *g_gct_mm_read_buf = NULL, *g_gct_burst_read_buf[15] = {NULL,};
  unsigned int i = 0;
  char fname[50];

  if(gdm_drv_dab_get_stored_msc_size(DAB_0_PATH,&stored_ts) != G_SUCCESS)
    return -1;

  if(stored_ts < 15) 
    return -3;

  gdm_dab_stop(DAB_0_PATH);

  msleep(100); // 1sec

  gdm_drv_ghost_mm_read(0x3c037e40,&rp);
  gdm_drv_ghost_mm_read(0x3c037e44,&wp);
  gdm_drv_ghost_mm_read(0x3c037e4c,&base);

  GCT_MSG_HIGH("rp[%X] ,wp[%X], msc[%X]\n",rp,wp,base); // base : 0x00000e94

  g_gct_mm_read_buf    = kmalloc(TEST_MM_READ_SIZE + 32, GFP_ATOMIC);

  for(i=0;i<(TEST_MM_READ_SIZE/188);i++)
  {
    g_gct_burst_read_buf[i] = kmalloc(188 + 32, GFP_ATOMIC);

    if(g_gct_burst_read_buf[i])
      memset(g_gct_burst_read_buf[i],0,188 + 32);
  }

  if(g_gct_mm_read_buf == NULL)
  {
    return -2;
  }

  for(i=0;i<(TEST_MM_READ_SIZE/188);i++)
  {
    if(g_gct_burst_read_buf[i] == NULL)
    {
      kfree(g_gct_mm_read_buf);
      for(i=0;i<(TEST_MM_READ_SIZE/188);i++)
      {
        if(g_gct_burst_read_buf[i])
        {
          kfree(g_gct_burst_read_buf[i]);
        }
      }
      return -2;
    }
  }

   // Initialize MSC_RP.
  gdm_drv_ghost_mm_write(0x3c037e40,0x0e94);
  gdm_drv_ghost_mm_read(0x3c037e40,&rp);  
  GCT_MSG_HIGH("rp[%X]",rp); // rp : 0x00000e94
 
  for(i=0;i<(TEST_MM_READ_SIZE/188);i++)
  {
    gdm_dab_read_msc(DAB_0_PATH,g_gct_burst_read_buf[i], 188);
	sprintf(fname, "/system/msc_read2-%.2d.ts", i);
  	gct_dump_ts_data(fname, g_gct_burst_read_buf[i], 188+32);
    gct_dump_ts_data(fname, NULL, 0);
  }

  gdm_drv_ghost_mm_burst_read(0x3c030e94,g_gct_mm_read_buf,TEST_MM_READ_SIZE);
  gct_dump_ts_data("/system/mm_read2.ts", g_gct_mm_read_buf, TEST_MM_READ_SIZE);
  gct_dump_ts_data("/system/mm_read2.ts", NULL, 0);

  kfree(g_gct_mm_read_buf);

  for(i=0;i<(TEST_MM_READ_SIZE/188);i++)
  {
    kfree(g_gct_burst_read_buf[i]);
  }

  return 0;
}



#endif //GCT_DEBUG

void TDMB_drv_gct_set_channel
(
  unsigned long  dEnsembleFrequency,
  unsigned char  nSubChID
)
{
  unsigned int index = 0;
  dab_demod_status_e_type demod_status;
  dab_sync_status_e_type sync_status;
  
  gdm_dab_stop(DAB_0_PATH);

  do{
  	if(dEnsembleFrequency/1000 == KOREA_BAND_TABLE[index])
  		break;
  }while( index++ < 0x14 );

  GCT_MSG_HIGH("## TDMB_drv_gct_set_channel ## %d, %d, %d\n",dEnsembleFrequency,index,nSubChID);
  gdm_dab_change_freq(DAB_0_PATH, DAB_MODE_1, DAB_FREQ_MODE_PREDEFINED_CH_SELECTOR, index);
#if 0
  //gdm_dab_change_freq(0 , 0 , 1 , 0x321e );
  gdm_dab_change_freq(DAB_0_PATH, DAB_MODE_1, DAB_FREQ_MODE_PREDEFINED_CH_SELECTOR, 4);
  gdm_dab_config_subch(CR_DAB0_SET_SUBCH_TS0, 1, 1);
#endif

#if 1
  if (nSubChID < 64)
    gdm_dab_config_subch(CR_DAB0_SET_SUBCH_NTS0, 1 /* Enable */, nSubChID);
  else
    gdm_dab_config_subch(CR_DAB0_SET_SUBCH_TS0, 1 /* Enable */, (nSubChID-64));
#endif

#ifdef FEATURE_GCT_DM // klaatu
  gdm_drv_dab_set_encap_dm_fic(DAB_0_PATH, DAB_ENCAP_DM_ENABLE, DAB_ENCAP_FIC_DISABLE);
#else
  gdm_drv_dab_set_encap_dm_fic(DAB_0_PATH, DAB_ENCAP_DM_DISABLE, DAB_ENCAP_FIC_DISABLE);
#endif

//  gdm_drv_dab_set_enable_serial_ts(DAB_0_PATH, 1);		
//  tdmb_bb_set_fic_isr(TRUE);
  gdm_dab_run(DAB_0_PATH);

  msleep(100);

  gdm_dab_get_demod_status(DAB_0_PATH, &demod_status, &sync_status);    
  GCT_MSG_HIGH("# GCT # Demod Status = %d,%d\n", demod_status,sync_status,0);

  // test code for power on and run 
#if 0
  {

	extern void tdmb_work_function(void);
#if 0
	  int i = 0;
	  G_Uint32 temp;
	  unsigned short read_temp;
	  hwait_down_e_type hwait_down = 0;
	  host_int_e_type host_int = 0;
	  spi_clk_polarity_e_type spi_clk_polarity = 0;
	  spi_clk_phase_e_type spi_clk_phase = 0;
#endif
	  while(1)
	  {
		  //temp = 0;
		  //gdm_dab_get_demod_status(DAB_0_PATH, &demod_status, &sync_status);    
		  //GCT_MSG_HIGH("[%d] # GCT # Demod Status = %d,%d\n",i,demod_status,sync_status);

		  // klaatu
//		  gdm_drv_ghost_get_int_mask(&read_temp); 
		  //GCT_MSG_HIGH("gdm_drv_ghost_get_int_mask : %x\n", read_temp);
//		  gdm_drv_ghost_get_int_status(&read_temp);

		//TDMB_drv_gct_int_handler();
		tdmb_work_function();
#if 0
		  temp =0;
		  gdm_drv_ghost_read(0,&temp);
	      GCT_MSG_HIGH("  -- [7024 pc : %x]  \n",temp );

		  // klaatu
		  gdm_drv_ghost_get_int_mask(&read_temp); 
		  GCT_MSG_HIGH("gdm_drv_ghost_get_int_mask : %x\n", read_temp);
		  gdm_drv_ghost_get_int_status(&read_temp);
		  GCT_MSG_HIGH("gdm_drv_ghost_get_int_status :<<<  %x >>>\n", read_temp);
		  gdm_drv_ghost_set_int_clear(0xFFFF);
		  //
#endif
	  }
  }
#endif
}

int TDMB_drv_gct_EnableDM(unsigned char bEnable, unsigned short wPeriod)
{
	unsigned short	wIM,wIS;
	dm_mode_e_type dm_mode;

	gdm_drv_ghost_get_int_status(&wIS);
	wIS &= ~(0x8000);
	gdm_drv_ghost_set_int_clear(wIS|0x8000);
	gdm_drv_ghost_get_int_mask(&wIM);
	DPRINTK(" %s get_int_mask : %x \n", __FUNCTION__, wIM);

	if(bEnable)
	{// enable full DM Report
		dm_mode = DM_FULL_MODE;
//		dm_mode = DM_SHORT_MODE;
		wIM = wIM & 0x7FFF;
	}
	else
	{
		dm_mode = DM_DISABLED_MODE;
		wIM = wIM | 0x8000;
	}
	gdm_set_dm(dm_mode,wPeriod);
	gdm_drv_ghost_set_int_mask(wIM);
	
	return TRUE;
}


#if 1 // klaatu - check later
void TDMB_drv_gct_dm(unsigned char* Buf,unsigned int buf_size)	
{
  unsigned int count = 0;
  t_tdmb_Dm *BufDM;
  //t_tdmb_ShortDm *ShortBufDM;

  for(count=0;count<buf_size;count+=2)
  {
  	gBufDM[count]   = Buf[count+1];
  	gBufDM[count+1] = Buf[count];
  }

//  if(iDMSize == DM_FULL_MODE)
  {
    BufDM = (t_tdmb_Dm *) gBufDM;
 
    rssi = BufDM->agc_gain;

    BER  = (BufDM->unc_sym_size == 0)?0xFFFF:((BufDM->unc_bit_err*10000)/(BufDM->unc_sym_size/*  *1024  */));
    if (BER < 70)
    {
        BER = 0;
    }
    else
    {
        BER -= 70;
    }

    PER = (BufDM->ber_ts_num[0] == 0)? 0xFFFF:((BufDM->ts_pkt_err[0] * 100000)/(BufDM->ber_ts_num[0]));

    GCT_MSG_HIGH(">> RSSI:%d UnC Bit err:%d/BER %d sym size:%d\n", rssi, BufDM->unc_bit_err,BER,  BufDM->unc_sym_size);
  }
}
#endif

void gct_dump_ts_data(char* fname, unsigned char *pData, unsigned long dwDataSize)
{
#define DUMP_FILE_NAME "/system/ts_data.ts"
	mm_segment_t oldfs;
	int ret;
	static struct file *fp_ts_data  = NULL;

	if(fp_ts_data == NULL) 
	{
		fp_ts_data = filp_open(fname, O_APPEND | O_CREAT, 0);
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

//extern void TDMB_NotifyFetchTSData(void); // klaatu - check later
#define MSCBuff_Size 1024
int bfirst=1;
unsigned char MSCBuff[MSCBuff_Size];
int MSCBuffpos=0;
int mp2len=0;
static const int bitRateTable[2][16] = { {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 0},  /* MPEG1 for id=1*/ 
                                                           {0,  8,  16, 24, 32, 40, 48,  56,   64,   80,  96,  112, 128, 144, 160, 0} };/* MPEG2 for id=0 */

int MP2_GetPacketLength(unsigned char *pkt)
{
    int id, layer_index, bitrate_index, fs_index, samplerate,protection;
    int bitrate, length;
                                              
    id = (pkt[1]>>3) &0x01; /* 1: ISO/IEC 11172-3, 0:ISO/IEC 13818-3 */
    layer_index = (pkt[1]>>1)&0x03; /* 2 */
    protection = pkt[1]&0x1;
    if (protection != 0)
    {
        //QTV_F("protection_bit is *NOT* 0");
    }
    bitrate_index = (pkt[2]>>4);
    fs_index = (pkt[2]>>2)&0x3; /* 1 */

    if(pkt[0]==0xff && (pkt[1]>>4)==0xf) /* sync word check */
    {
        if ( (bitrate_index > 0 && bitrate_index < 15) && (layer_index==2) && (fs_index ==1) )
        {
            if (id ==1 && layer_index ==2) /* Fs==48 KHz*/
            {
                bitrate=1000*bitRateTable[0][bitrate_index];
                samplerate=48000;
            }
            else if (id==0 && layer_index ==2) /* Fs=24 KHz */
            {
                bitrate=1000*bitRateTable[1][bitrate_index];
                samplerate=24000;
            }
            else
                return -1;
        }
        else
            return -1;
    }
    else
        return -1;

    if ( (pkt[2]&0x02)!=0) /* padding bit */
    {
        return -1;            
    }
    
    length = (144*bitrate)/(samplerate);
    
    return length;
}
int __AddDataMSC(unsigned char* pData, unsigned long dwDataSize, int SubChID)
{
    int j,readpos =0;
    unsigned char pOutAddr[188];
    static int first=1;
    int remainbyte = 0;
    if(bfirst)
    {
        for(j=0;j<dwDataSize-2;j++)
        {
            if(pData[j]==0xFF && (pData[j+1]>>4==0xF))
            {                
                mp2len = MP2_GetPacketLength(&pData[j]);
                printk("!!!!!!!!!!!!! first sync=%d !!!!!!!!!!!!",mp2len);
                if(mp2len<0)
                    return -1;
                memcpy(MSCBuff, &pData[j], dwDataSize-j);
                MSCBuffpos = dwDataSize-j;
                bfirst = 0;
                first =1;
                return 0;
            }
        }
    }
    else
    {
        remainbyte = dwDataSize;
        if(mp2len-MSCBuffpos>=dwDataSize)
        {
            memcpy(MSCBuff+MSCBuffpos, pData,  dwDataSize);
            MSCBuffpos += dwDataSize;
            remainbyte = 0;
        }
        else if(mp2len-MSCBuffpos>0)
        {
            memcpy(MSCBuff+MSCBuffpos, pData,  mp2len-MSCBuffpos);
            remainbyte = dwDataSize - (mp2len -MSCBuffpos);
            MSCBuffpos = mp2len;
        }

        if(MSCBuffpos==mp2len)
        {
            while(MSCBuffpos>readpos)
            {
                //memset(pOutAddr, 0, 188);
                if(first)
                {
                    pOutAddr[0]=0xDF;
                    pOutAddr[1]=0xDF;
                    pOutAddr[2]= (SubChID<<2);
                    pOutAddr[2] |=(((MSCBuffpos>>3)>>8)&0x03);
                    pOutAddr[3] = (MSCBuffpos>>3)&0xFF;
                    if(!(MSCBuff[0]==0xFF && (MSCBuff[1]>>4==0xF)))
                    {
                        memset(MSCBuff, 0, MSCBuff_Size);
                        MSCBuffpos =0;
                        bfirst =1;
                        printk("!!!!!!!!!!!!! error 0x%x,0x%x!!!!!!!!!!!!",MSCBuff[0], MSCBuff[1]);
                        return -1;
                    }
                    memcpy(pOutAddr+4, MSCBuff, 184);
                    readpos = 184;
                    first =0;
                }
                else
                {
                    pOutAddr[0]=0xDF;
                    pOutAddr[1]=0xD0;
                    if(MSCBuffpos-readpos>=184)
                    {
                        memcpy(pOutAddr+4, MSCBuff+readpos, 184);
                        readpos +=184;
                    }
                    else
                    {
                        memcpy(pOutAddr+4, MSCBuff+readpos, MSCBuffpos-readpos);
                        readpos +=(MSCBuffpos-readpos);
                    }

                }
                TDMB_AddDataToRing(pOutAddr, 188);
            }
            first =1;
            MSCBuffpos =0;
            if(remainbyte>0)
            {
                memcpy(MSCBuff, pData+dwDataSize-remainbyte, remainbyte);
                MSCBuffpos = remainbyte;                
            }
        }
    }
    return 0;
}
static void __SubProcTSIFData(unsigned char *pData, unsigned int nDataSize)
{
	dab_encap_buf_e_type BufID;
	unsigned char SubChID; 
	unsigned int Size;
	unsigned int Size_Stream = 0;    
	unsigned char *pOutAddr;
  unsigned int i = 0, pkt_num = nDataSize/188;
  static int pktcount = 0;
  GCT_MSG_HIGH("##### pkt_num %d , data size:%d #######\n",pkt_num, nDataSize,0);  

  for( i = 0 ; i < pkt_num ;i++, pData += 188 )
  {

    if(TDMB_drv_gct_dab_decapsulator( pData,
                                      &BufID,
                                      &SubChID,
                                      &pOutAddr,
                                      &Size) == G_SUCCESS)
    {
        if ( BufID == DAB_ENCAP_BUF_TS )
        {
            TDMB_AddDataToRing(pOutAddr, Size);
	  		// gct_dump_ts_data(DUMP_FILE_NAME,pData, 188);
        }
        else if( BufID == DAB_ENCAP_BUF_NTS )
        {
            __AddDataMSC(pOutAddr,Size, SubChID);
        }
        else if(BufID == DAB_ENCAP_BUF_FIC)
        {
            GCT_MSG_HIGH("##### DAB_ENCAP_BUF_FIC #######\n",0,0,0);          
            TDMB_drv_gct_fic_decode(pOutAddr,Size);
        }
        else if(BufID == DAB_ENCAP_BUF_DM)
        {
            GCT_MSG_HIGH("##### DAB_ENCAP_BUF_DM #######\n",0,0,0);  
            TDMB_drv_gct_dm(pOutAddr,Size);
        }
        else
        {
            GCT_MSG_HIGH("##### Error #######\n",0,0,0);          
        }
    }
  }
}


static unsigned int gdm_hal_read_msc_test = 1;

/* --------------------------------------------------------------------------------------------

FUNCTION    


DESCRIPTION
           

PARAMETER


RETURN VALUE

-------------------------------------------------------------------------------------------- */
void gdm_hal_read_msc( void )
{
  unsigned short stored_ts;
  unsigned char *ts_pkt = NULL;
  unsigned int i = 0;
  unsigned short read_ts_num;
  unsigned char tmp;
  static int bnon = 0;

  gdm_drv_dab_get_stored_msc_size(DAB_0_PATH,&stored_ts);
  stored_ts = min(sizeof(tdmb_gct_read_msc_buffer)/188,stored_ts);

  GCT_MSG_HIGH("=====stored_ts = %d=====\n", stored_ts);
#ifdef GCT_DEBUG
  if(bnon == 1) return;

  if(stored_ts >= 15)
  {
	  bnon = 1;
	//printk("### [%s] res : %d\n",__FUNCTION__, gdm_read_test2());
	  gdm_test();
	return;
  }
#endif
  while(stored_ts)
  {
    read_ts_num = min(stored_ts,GDM_SPI_READ_PKT_NUM);
    memset(tdmb_gct_read_msc_buffer,0,sizeof(tdmb_gct_read_msc_buffer));
//    GCT_MSG_HIGH("=====gdm_dab_msc_int_handler=====\n");
    gdm_dab_msc_int_handler(DAB_0_PATH,tdmb_gct_read_msc_buffer,read_ts_num*188);

#if 0 // GCT_20090622 
    ts_pkt = &tdmb_gct_read_msc_buffer[2];
    for( i = 0 ; i < (read_ts_num*188)/2 ; i++,ts_pkt += 2 )
    {
       tmp       = ts_pkt[0];
       ts_pkt[0] = ts_pkt[1];
       ts_pkt[1] = tmp;
    }
#endif

    if(gdm_hal_read_msc_test) // klaatu made this off
    {
		// klaatu - check later this part
      __SubProcTSIFData(&tdmb_gct_read_msc_buffer[2], read_ts_num*188); // klaatu - check later
      //TDMB_NotifyFetchTSData(); // klaatu - check later
    }
    stored_ts -= read_ts_num;
//    GCT_MSG_HIGH("=====read_ts_num = %d=====\n", read_ts_num);    
  }
//    gdm_drv_dab_get_stored_msc_size(DAB_0_PATH,&stored_ts);
//    GCT_MSG_HIGH("size of stored_ts after read : %d\n", stored_ts);    
}

/*-------------------------------------------------------------------------

Function    : TDMB_DRV_GCT_INT_HANDLER

Description :

Parameter   :

Return      :
-------------------------------------------------------------------------*/
// klaatu - check later
#if 1 // TDMB_drv_gct_int_handler
void tdmb_work_function(void)
{
	static int counter = 0;
  G_Uint16 usInterruptMask=0,usInterruptStatus=0,usCurrentInterrupt=0;
  dab_demod_status_e_type demod_status;
  dab_sync_status_e_type sync_status;
  gdm_drv_ghost_get_int_mask(&usInterruptMask); 
  gdm_drv_ghost_get_int_status(&usInterruptStatus);

  usCurrentInterrupt = usInterruptStatus & (~usInterruptMask);
  //GCT_MSG_HIGH("usCurrentInterrupt=0x%.8x, usInterruptMask = 0x%.8x, usInterruptStatus = 0x%.8x\n",
//		  usCurrentInterrupt, usInterruptMask, usInterruptStatus); 

#if 1 // case of IRQ
  while(usCurrentInterrupt)
#endif
  {
      gdm_drv_ghost_set_int_clear(usCurrentInterrupt);  
#if 0
      if(usCurrentInterrupt & INT_DAB_FIC)
	  {
        GCT_MSG_HIGH("##### INT_DAB_FIC #######\n");  
	  }
#endif 
      if(usCurrentInterrupt & INT_DAB_MSC)
      {
        GCT_MSG_HIGH("##### INT_DAB_MSC #######\n");  
        gdm_hal_read_msc();
      }
      
      if(usCurrentInterrupt & INT_DAB_INVALID_SUBCH)
      {
        GCT_MSG_HIGH("##### INT_DAB_INVALID_SUBCH #######\n");
        gdm_dab_invalid_subch_int_handler(DAB_0_PATH);
      }
      
      if(usCurrentInterrupt & INT_DAB_FIC_RECONF)
      {
        GCT_MSG_HIGH("##### INT_DAB_FIC_RECONF #######\n");
        gdm_dab_fic_reconf_int_handler(DAB_0_PATH);
      }
      
      if(usCurrentInterrupt & INT_DAB_SYNC_LOSS)
      {
        GCT_MSG_HIGH("##### INT_DAB_SYNC_LOSS #######\n");
        gdm_dab_sync_loss_int_handler(DAB_0_PATH);
      }
      
      if(usCurrentInterrupt & INT_DAB_MSC_OV)
      {
		printk("##### INT_DAB_MSC_OV #######\n");
        GCT_MSG_HIGH("##### INT_DAB_MSC_OV #######\n");
        gdm_dab_msc_ov_int_handler(DAB_0_PATH);
      }
      if(usCurrentInterrupt & INT_DAB_DM)
	  {
		  GCT_MSG_HIGH("#### INT_DAB_DM #######\n");

		  gdm_dm_int_handler(gBufDM, sizeof(t_tdmb_Dm));
		  TDMB_drv_gct_dm(&gBufDM[2], sizeof(t_tdmb_Dm));
	  }

#if 1 // case of IRQ
      gdm_drv_ghost_get_int_mask(&usInterruptMask); 
      gdm_drv_ghost_get_int_status(&usInterruptStatus);
      usCurrentInterrupt = usInterruptStatus & (~usInterruptMask);
      //gdm_drv_ghost_set_int_clear(usCurrentInterrupt);
#endif
  }
}
#endif


void TDMB_drv_gct_resync(void)
{
  GCT_MSG_HIGH("******INT_DAB0_SYNC_LOSS****\n");

  gdm_dab_sync_loss_int_handler(DAB_0_PATH);
}


#if 0 //ndef COMMERCIAL_RELEASE // klaatu - don't understand if this is needed
char TDMB_drv_gct_read_fw(const char *fname,char *read_buf,unsigned int fsize)
{
  fs_open_xparms_type  open_options;
  fs_rsp_msg_type      open_msg,rsp_msg;

  open_options.create.cleanup_option   = FS_OC_CLOSE;
  open_options.create.buffering_option = FS_OB_PROHIBIT;
  open_options.create.attribute_mask   = FS_FA_UNRESTRICTED;

  fs_open(fname,
          FS_OA_READONLY,
          &open_options,
          NULL,
          &open_msg);
  if(open_msg.open.status != FS_OKAY_S)
  {
    GCT_MSG_HIGH("f open fail",0,0,0);
    return FALSE;
  }

  fs_read(open_msg.open.handle,read_buf,fsize,NULL,&rsp_msg);
  if(rsp_msg.read.status != FS_OKAY_S)
  {
    GCT_MSG_HIGH("f read fail",0,0,0);
    fs_close(open_msg.open.handle, NULL, &rsp_msg);
    return FALSE;
  }

  fs_close(open_msg.open.handle, NULL, &rsp_msg);
  return TRUE;
}

char TDMB_drv_gct_upgrade(void)
{
  unsigned int    fsize;
  fs_rsp_msg_type rsp_msg;

  // CODE
  if(tdmb_drv_gct_bin_code)
  {
    GCT_MSG_HIGH("tdmb_drv_gct_bin_code free",0,0,0);
    free(tdmb_drv_gct_bin_code);
    tdmb_drv_gct_bin_code = NULL;
  }

  fs_file_size(TDMB_GCT_FNAME_IROM,NULL,&rsp_msg);
  if(rsp_msg.file_size.status != FS_OKAY_S)
  {
    GCT_MSG_HIGH("f size error",0,0,0);
    return FALSE;
  }

  tdmb_drv_gct_bin_code = (char *)malloc(rsp_msg.file_size.size);
  tdmb_drv_gct_bin_code_size = rsp_msg.file_size.size;

  if(tdmb_drv_gct_bin_code == NULL)
  {
    GCT_MSG_HIGH("malloc fail",0,0,0);
    return FALSE;
  }

  if(FALSE == TDMB_drv_gct_read_fw(TDMB_GCT_FNAME_IROM,tdmb_drv_gct_bin_code,tdmb_drv_gct_bin_code_size))
  {
    GCT_MSG_HIGH("TDMB_drv_gct_read_fw FAIL",0,0,0);
    return FALSE;
  }


  // YMEM
  if(tdmb_drv_gct_bin_ymem)
  {
    GCT_MSG_HIGH("free ymem",0,0,0);
    free(tdmb_drv_gct_bin_ymem);
    tdmb_drv_gct_bin_ymem = NULL;
  }

  fs_file_size(TDMB_GCT_FNAME_YMEM,NULL,&rsp_msg);
  if(rsp_msg.file_size.status != FS_OKAY_S)
  {
    GCT_MSG_HIGH("fs_file_size FAIL",0,0,0);
    return FALSE;
  }

  tdmb_drv_gct_bin_ymem = (char *)malloc(rsp_msg.file_size.size);
  tdmb_drv_gct_bin_ymem_size = rsp_msg.file_size.size;

  if(tdmb_drv_gct_bin_ymem == NULL)
  {
    GCT_MSG_HIGH("malloc FAIL",0,0,0);
    return FALSE;
  }

  if(FALSE == TDMB_drv_gct_read_fw(TDMB_GCT_FNAME_YMEM,tdmb_drv_gct_bin_ymem,tdmb_drv_gct_bin_ymem_size))
  {
    GCT_MSG_HIGH("TDMB_drv_gct_read_fw FAIL",0,0,0);
    return FALSE;
  }


  GCT_MSG_HIGH("success TDMB_drv_gct_upgrade",0,0,0);
  return TRUE;
}
#endif

void TDMB_drv_gct_set_freq_channel(unsigned long dEnsembleFreq_subCh)
{
	unsigned char  nSubChID = 0;
	unsigned long  dEnsembleFreq;

	dEnsembleFreq = (dEnsembleFreq_subCh/1000)*1000;
	nSubChID = dEnsembleFreq_subCh%1000;

	DPRINTK("## TDMB_drv_gct_set_channel ## %d, %d",dEnsembleFreq,nSubChID);
	
	TDMB_drv_gct_set_channel(dEnsembleFreq, nSubChID);
}
#if 0 // klaatu - check later
const char * TDMB_drv_gct_get_version(void)
{
    static const char * version = "2.5r1";
    return version;
}
#endif 
