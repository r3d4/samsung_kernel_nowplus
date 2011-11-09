#ifndef _TDMB_DRV_GCT_H_
#define _TDMB_DRV_GCT_H_

typedef enum
{
	DAB_ENCAP_BUF_FIC = 0,
	DAB_ENCAP_BUF_TS,
	DAB_ENCAP_BUF_NTS,
	DAB_ENCAP_BUF_DM,
	
	DAB_ENCAP_BUF_MAX
} dab_encap_buf_e_type;


void TDMB_drv_gct_download( void );
char TDMB_drv_gct_scan(unsigned long FIG_Frequency);
char TDMB_drv_gct_scan_stop(char bSuccess);

signed char TDMB_drv_gct_dab_decapsulator
(
  unsigned char        *pSrc,
  dab_encap_buf_e_type *BufID,
  unsigned char        *SubChID,	
  unsigned char       **pOutAddr,
  unsigned int         *Size
);

void TDMB_drv_gct_set_channel
(
  unsigned long  dEnsembleFrequency,
  unsigned char  nSubChID
);

void TDMB_drv_gct_dm(unsigned char* Buf,unsigned int buf_size);
int TDMB_drv_gct_EnableDM(unsigned char bEnable, unsigned short wPeriod);
//void TDMB_drv_gct_int_handler(void);
void TDMB_drv_gct_resync(void);

signed short TDMB_drv_gct_get_rssi(void);
unsigned long TDMB_drv_gct_get_ber(void);
//byte TDMB_drv_gct_upgrade(void);
unsigned int TDMB_drv_gct_get_antenna(void);

#endif /* _TDMB_DRV_GCT_H_ */
