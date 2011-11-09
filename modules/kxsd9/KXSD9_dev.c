
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/delay.h>

#include "KXSD9_regs.h"
#include "KXSD9_main.h"
#include "KXSD9_dev.h"
#include "common.h"

#define I2C_M_WR    0x0000

enum
{
    eTRUE,
    eFALSE,
}dev_struct_status_t;

typedef struct
{
    /*Range and Threshold*/
    unsigned short R_T;
    /*Operational Bandwidth*/	
    unsigned short BW;
    /*Sensitivity*/
    KXSD9_sensitivity_t sensitivity;
    /*zero-g offset*/
    KXSD9_zero_g_offset_t zero_g_offset;
}dev_settings_t;

typedef struct
{
    /*ENABLE in CTRL_REGSB*/
    unsigned short dev_state;
    /*MOTlen in CTRL_REGSB*/
    unsigned short mwup;
    /*MOTLat in CTRL_REGSC*/	
    unsigned short mwup_rsp;	
}dev_mode_t;

typedef struct 
{
    /*Any function which 
      - views/modifies the fields of this structure
      - does i2c communication 
        should lock the mutex before doing so.
        Recursive locking should not be done.
        In this file all the exported functions will take care
        of this. The static functions will not deal with the
        mutex*/
    struct mutex lock;
	
    struct i2c_client const *client;

    dev_mode_t mode;
    dev_settings_t settings;

    /* This field will be checked by all the functions
       exported by this file (except the init function), 
       to validate the the fields of this structure. 
       if eTRUE: the fileds are valid
       if eFALSE: do not trust the values of the fields 
       of this structure*/
    unsigned short valid;

    /* OMS use this value. If the gap between old x(y) and new x(y) is 
       bigger than threshhold, OMS know that new data was received */
    unsigned short threshold;
}KXSD9_device_t;

/*extern functions*/
/**********************************************/
/*All the exported functions which view or modify the device
  state/data, do i2c com will have to lock the mutex before 
  doing so*/
/**********************************************/
int KXSD9_dev_init(struct i2c_client *client, KXSD9_module_param_t *);
int KXSD9_dev_exit(void);

void KXSD9_dev_mutex_init(void);

int KXSD9_dev_mwup_status(void);

int KXSD9_dev_get_sensitivity(KXSD9_sensitivity_t *);
int KXSD9_dev_get_zero_g_offset(KXSD9_zero_g_offset_t *);
int KXSD9_dev_get_acc(KXSD9_acc_t *);
int KXSD9_dev_get_default(KXSD9_default_t *);

int KXSD9_dev_set_range( int ) ;
short KXSD9_dev_get_bandwidth( void ) ;
int KXSD9_dev_set_bandwidth( int ) ;
int KXSD9_dev_set_operation_mode( int, int ) ;

int KXSD9_dev_set_status( int, int ) ;
int KXSd9_dev_get_status( void ) ;

int KXSD9_dev_enable(void);
int KXSD9_dev_disable(void);

extern int KXSD9_dev_mwup_enb(unsigned short);
extern int KXSD9_dev_mwup_dsb(void);
/***********************************************/

/*static functions*/
/**********************************************/
static void set_val(unsigned char *, unsigned char *, 
                              KXSD9_module_param_t *); 

static inline int get_X_acc(unsigned short *);
static inline int get_Y_acc(unsigned short *);
static inline int get_Z_acc(unsigned short *);
static int get_acc(char , char , unsigned short *);

static int i2c_read(unsigned char, unsigned char *);
static int i2c_write(unsigned char, unsigned char);
/**********************************************/

/*KXSD9 device structure*/
static KXSD9_device_t KXSD9_dev =
{
    .client = NULL,
    .valid = eFALSE,
};    

int KXSD9_dev_init(struct i2c_client *client, KXSD9_module_param_t *param)
{
    int ret = 0;
    int i = 0;
    int check = 0;

    /*these have to be initialized to 0*/	 
    unsigned char ctrl_regsc=0, ctrl_regsb=0;
    trace_in() ;

    mutex_lock(&(KXSD9_dev.lock));   

    /*KXSD9_dev.client should be set before doing i2c communication
      At this point KXSD9_dev.valid = eFALSE. This is the only point
      where i2c communication will be done with KXSD9_dev.valid = eFALSE
      In all other places the caller should ensure that KXSD9_dev.valid =
      eTRUE before doing i2c communication*/
    KXSD9_dev.client = client;	

    CTRL_REGSB_BITSET_KXSD9_nor_op(&ctrl_regsb);
    CTRL_REGSB_BITSET_clk_ctrl_on(&ctrl_regsb);

    KXSD9_dev.mode.dev_state = ENABLED;	

    KXSD9_dev.settings.sensitivity.error_RT = 25;
    
    KXSD9_dev.settings.zero_g_offset.counts = 2048;
    KXSD9_dev.settings.zero_g_offset.error_RT = 205;
    
    set_val(&ctrl_regsb, &ctrl_regsc, param);

    for( i = 0 ; i < 10 ; i++ )
    {
        if( (ret = i2c_write(CTRL_REGSB,ctrl_regsb)) < 0 )
        {
            printk("%s: KXSD9_dev_init->i2c_write 1 failed, ret= %d, i= %d\n", __FUNCTION__, ret, i );
            mdelay(20);
        }
        else
        {
        	check++ ;
            break ;
        }
    }

    /*Delay required for the stablization of device after enabling*/
    mdelay(20);

    for( i = 0 ; i < 10 ; i++ )
    {
        if( (ret = i2c_write(CTRL_REGSC,ctrl_regsc)) < 0 )
        {
            printk("%s: KXSD9_dev_init->i2c_write 2 failed, ret= %d, i= %d\n", __FUNCTION__, ret, i );
            mdelay(20);
        }
        else
        {
            check++ ;
            break ;
        }
    }

    if( check == 2 ) 
    {
        KXSD9_dev.valid = eTRUE;
        printk("%s: KXSD9_dev_init, KXSD9_dev.valid == eTRUE, check= %d\n", __FUNCTION__, check );
    }
    else if( KXSD9_dev.valid != eTRUE ) 
    {
        KXSD9_dev.valid = eTRUE;
        printk("%s: KXSD9_dev_init, Error occured, but set eTrue forcely, ret= %d\n", __FUNCTION__, ret );
    }
    else
    {
        printk("%s: KXSD9_dev_init, unknown check flag, check= %d\n", __FUNCTION__, check);
    }

    mutex_unlock(&(KXSD9_dev.lock));   	 

    trace_out() ;
    return ret;
}

/*Helper function: sets the values for CTRL regs and KXSD9_dev struct.
  *Should be called only from init*/
static void set_val(unsigned char *ctrl_regsb, unsigned char *ctrl_regsc, 
                              KXSD9_module_param_t *param) 
{
    trace_in() ;
    switch(param->mwup)
    {
        case ENABLED:
             CTRL_REGSB_BITSET_mwup_enb(ctrl_regsb);	
             KXSD9_dev.mode.mwup = ENABLED;

             switch( param->mwup_rsp )
             {
                case  LATCHED:
                default:						
                      CTRL_REGSC_BITSET_mwup_rsp_latchd(ctrl_regsc);	
                      KXSD9_dev.mode.mwup_rsp = LATCHED;
                      break;
                case  UNLATCHED:
                      CTRL_REGSC_BITSET_mwup_rsp_unlatchd(ctrl_regsc);	
                      KXSD9_dev.mode.mwup_rsp = UNLATCHED;	
                      break;								  
             }
             break;

        case DISABLED:
        default:				
             CTRL_REGSB_BITSET_mwup_disb(ctrl_regsb);	
             KXSD9_dev.mode.mwup = DISABLED;
             KXSD9_dev.mode.mwup_rsp = DONT_CARE;
             break;	
    }			

    switch(param->BW)
    {
        case BW50:
        default:
             CTRL_REGSC_BITSET_BW50(ctrl_regsc);
             KXSD9_dev.settings.BW = BW50;
             break;

        case BW100:
             CTRL_REGSC_BITSET_BW100(ctrl_regsc);
             KXSD9_dev.settings.BW = BW100;
             break;

        case BW500:
             CTRL_REGSC_BITSET_BW500(ctrl_regsc);
             KXSD9_dev.settings.BW = BW500;	
             break;

        case BW1000:
             CTRL_REGSC_BITSET_BW1000(ctrl_regsc);
             KXSD9_dev.settings.BW = BW1000;	
             break;

        case BW2000:
             CTRL_REGSC_BITSET_BW2000(ctrl_regsc);
             KXSD9_dev.settings.BW = BW2000;	
             break;

        case BW_no_filter:
             CTRL_REGSC_BITSET_BW_no_filter(ctrl_regsc);
             KXSD9_dev.settings.BW = BW_no_filter;		
             break;
    }

    switch(param->R_T)
    {
        case R2g_T1g:
        default:
             CTRL_REGSC_BITSET_R2g_T1g(ctrl_regsc);
             KXSD9_dev.settings.R_T= R2g_T1g;	
             KXSD9_dev.settings.sensitivity.counts_per_g = 819;
             break;

        case R2g_T1p5g:
             CTRL_REGSC_BITSET_R2g_T1p5g(ctrl_regsc);
             KXSD9_dev.settings.R_T = R2g_T1p5g;	
             KXSD9_dev.settings.sensitivity.counts_per_g = 819;
             break;

        case R4g_T2g:
             CTRL_REGSC_BITSET_R4g_T2g(ctrl_regsc);
             KXSD9_dev.settings.R_T = R4g_T2g;
             KXSD9_dev.settings.sensitivity.counts_per_g = 410;
             break;

        case R4g_T3g:
             CTRL_REGSC_BITSET_R4g_T3g(ctrl_regsc);
             KXSD9_dev.settings.R_T = R4g_T3g;
             KXSD9_dev.settings.sensitivity.counts_per_g = 410;
             break;			

        case R6g_T3g:
             CTRL_REGSC_BITSET_R6g_T3g(ctrl_regsc);
             KXSD9_dev.settings.R_T = R6g_T3g;
             KXSD9_dev.settings.sensitivity.counts_per_g = 273;
             break;

        case R6g_T4p5g:
             CTRL_REGSC_BITSET_R6g_T4p5g(ctrl_regsc);
             KXSD9_dev.settings.R_T = R6g_T4p5g;
             KXSD9_dev.settings.sensitivity.counts_per_g = 273;
             break;

        case R8g_T4g:
             CTRL_REGSC_BITSET_R8g_T4g(ctrl_regsc);
             KXSD9_dev.settings.R_T = R8g_T4g;	
             KXSD9_dev.settings.sensitivity.counts_per_g = 205;
             break;

        case R8g_T6g:
             CTRL_REGSC_BITSET_R8g_T6g(ctrl_regsc);
             KXSD9_dev.settings.R_T = R8g_T6g;
             KXSD9_dev.settings.sensitivity.counts_per_g = 205;
             break;				
    }	    
    
    trace_out() ;
}

int KXSD9_dev_exit(void)
{
    int ret = 0;
    trace_in() ;

    mutex_lock(&(KXSD9_dev.lock));   

    KXSD9_dev.client = NULL;	

    KXSD9_dev.valid = eFALSE;

    mutex_unlock(&(KXSD9_dev.lock)); 

    trace_out() ;
    return ret;
}

void KXSD9_dev_mutex_init(void)
{
    trace_in() ;
    
    mutex_init(&(KXSD9_dev.lock));
    
    trace_out() ;
}

int KXSD9_dev_mwup_status(void)
{
    int ret;
    trace_in() ;
    
    mutex_lock(&(KXSD9_dev.lock)); 

    if( (KXSD9_dev.valid == eFALSE) ||( KXSD9_dev.mode.dev_state == DISABLED ) )
    {
        ret = N_OK;
    }
    else
    {
        ret = KXSD9_dev.mode.mwup;
    }
    
    mutex_unlock(&(KXSD9_dev.lock)); 

    trace_out() ;
    return ret;
}

int KXSD9_dev_enable(void)
{
    int ret = 0;
    unsigned char reg_data;	 
    trace_in() ;

    mutex_lock(&(KXSD9_dev.lock));   

    if( KXSD9_dev.valid == eFALSE )
    {
        printk("%s: KXSD9_dev_enable called when DS is invalid\n", __FUNCTION__ );
        ret = -1;
    }
    else if( (ret = i2c_read(CTRL_REGSB,&reg_data)) < 0 )
    {
        printk("%s: KXSD9_dev_enable->i2c_read failed\n", __FUNCTION__ );
    }
    else	 	
    {
        CTRL_REGSB_BITSET_KXSD9_nor_op(&reg_data);
        CTRL_REGSB_BITSET_clk_ctrl_on(&reg_data);		

        if( (ret = i2c_write(CTRL_REGSB, reg_data)) < 0 )
        {
            printk("%s: KXSD9_dev_enable->i2c_write failed\n", __FUNCTION__ );
        }
        else
        {
            /*Delay required for the stablization of device after enabling*/
            mdelay(16);
            
            KXSD9_dev.mode.dev_state = ENABLED;
        }
    } 

    mutex_unlock(&(KXSD9_dev.lock)); 

    set_bit(KXSD9_ON, &KXSD9_status.status);
    trace_out() ;
    return ret;
}

int KXSD9_dev_disable(void)
{
    int ret = 0;
    unsigned char reg_data;	 
    trace_in() ;

    mutex_lock(&(KXSD9_dev.lock));   

    if( KXSD9_dev.valid == eFALSE )
    {
        printk("%s: KXSD9_dev_disable called when DS is invalid\n", __FUNCTION__ );
        ret = -1;
    }
    else if( (ret = i2c_read(CTRL_REGSB,&reg_data)) < 0 )
    {
        printk("%s: KXSD9_dev_disable->i2c_read failed\n", __FUNCTION__ );
    }
    else
    {
        CTRL_REGSB_BITSET_KXSD9_standby(&reg_data);
        CTRL_REGSB_BITSET_clk_ctrl_off(&reg_data);

        if( (ret = i2c_write(CTRL_REGSB, reg_data)) < 0 )
        {
            printk("%s: KXSD9_dev_disable->i2c_write failed\n", __FUNCTION__ );
        }
        else
        {
            KXSD9_dev.mode.dev_state = DISABLED;
        }
    } 

    mutex_unlock(&(KXSD9_dev.lock)); 

    clear_bit(KXSD9_ON, &KXSD9_status.status);

    trace_out() ;
    return ret;
}

int KXSD9_dev_mwup_enb(unsigned short mwup_rsp)
{
    int ret = 0;
    unsigned char ctrl_regsb, ctrl_regsc, ctrl_regsb_bkup;
    unsigned short mwup_prev, mwup_rsp_prev;
    trace_in() ;

    mutex_lock(&(KXSD9_dev.lock));   

    mwup_prev = KXSD9_dev.mode.mwup;
    mwup_rsp_prev = KXSD9_dev.mode.mwup_rsp;

    if( KXSD9_dev.valid == eFALSE )
    {
        printk("%s: KXSD9_dev_mwup_enb called with dev DS invalid\n", __FUNCTION__ );
        ret = -1;
    }
    else if( KXSD9_dev.mode.dev_state == DISABLED )
    {
        printk("%s: KXSD9_dev_mwup_enb: dev is DISABLED\n", __FUNCTION__ );
        ret = -1;
    }
    else if( (ret = i2c_read(CTRL_REGSB, &ctrl_regsb)) < 0 )
    {
        printk("%s: KXSD9_dev_mwup_enb i2c_read1 failed\n", __FUNCTION__ );
    }
    else if( (ret = i2c_read(CTRL_REGSC, &ctrl_regsc)) < 0 )
    {
        printk("%s: KXSD9_dev_mwup_enb i2c_read2 failed\n", __FUNCTION__ );
    }
    else
    {
        ctrl_regsb_bkup = ctrl_regsb;
        
        CTRL_REGSB_BITSET_mwup_enb(&ctrl_regsb);	
        KXSD9_dev.mode.mwup = ENABLED;

        switch( mwup_rsp )
        {
            case  LATCHED:
                CTRL_REGSC_BITSET_mwup_rsp_latchd(&ctrl_regsc);	
                KXSD9_dev.mode.mwup_rsp = LATCHED;
                break;
 
            case  UNLATCHED:
                CTRL_REGSC_BITSET_mwup_rsp_unlatchd(&ctrl_regsc);	
                KXSD9_dev.mode.mwup_rsp = UNLATCHED;	
                break;

            /* [2009.10.12 / BSP / ACCEl ] joon.c.baek
             * when a set DONT_CARE
             * default unlatched */
            case DONT_CARE:
                CTRL_REGSC_BITSET_mwup_rsp_unlatchd(&ctrl_regsc);	
                KXSD9_dev.mode.mwup_rsp = UNLATCHED;	
                break;

             default:  
                ret = -EINVAL;
                printk("%s: KXSD9_dev_mwup_enb invalid argument- mwup_rsp: %d\n",
                __FUNCTION__, mwup_rsp );
                break;
         }

         if( ret == 0 )
         {
             if( (ret = i2c_write(CTRL_REGSB,ctrl_regsb)) < 0 )
             {
                 printk("%s: KXSD9_dev_mwup_enb->i2c_write 1 failed\n", __FUNCTION__ );
             }
             else if( (ret = i2c_write(CTRL_REGSC,ctrl_regsc)) < 0 )
             {
                 printk("%s: KXSD9_dev_mwup_enb->i2c_write 2 failed\n", __FUNCTION__ );
                 
                 /*In this case the motion wakeup will be enabled with the previous
                     rsp type (unpredictable state from user's point of view), so we 
                     will try to revert. If that fails we'll mark KXSD9_dev as invalid*/
                  if( (ret = i2c_write(CTRL_REGSB, ctrl_regsb_bkup)) < 0 )
                  {
                      printk("%s: FATAL: the device is marked invalid\n", __FUNCTION__ );
                      KXSD9_dev.valid = eFALSE;
                  }
             }
         }
    }

    if(ret < 0)
    {
        KXSD9_dev.mode.mwup = mwup_prev;
        KXSD9_dev.mode.mwup_rsp = mwup_rsp_prev;
    }

    debug("mwup = %s", (KXSD9_dev.mode.mwup == ENABLED) ? "ENABLED" : "DISABLED" );
    debug("mwup_rsp = %s", 
    (KXSD9_dev.mode.mwup_rsp == LATCHED) ? "LATCHED" : 
    ((KXSD9_dev.mode.mwup_rsp == UNLATCHED) ? "UNLATCHED": "DONT_CARE"));

    mutex_unlock(&(KXSD9_dev.lock)); 

    trace_out() ;
    return ret;         
}

int KXSD9_dev_mwup_disb(void)
{
    int ret = 0;
    unsigned char ctrl_regsb;
    unsigned short mwup_prev;
    trace_in() ;
    
    mutex_lock(&(KXSD9_dev.lock));  

    mwup_prev = KXSD9_dev.mode.mwup;

    if( KXSD9_dev.valid == eFALSE )
    {
        printk("%s: KXSD9_dev_mwup_disb called with dev DS invalid\n", __FUNCTION__ );
        ret = -1;
    }
    else if( KXSD9_dev.mode.dev_state == DISABLED )
    {
        printk("%s: KXSD9_dev_mwup_disb: dev is DISABLED\n", __FUNCTION__ );
        ret = -1;		
    }
    else if( KXSD9_dev.mode.mwup == DISABLED )
    {
        printk("%s: mwup already disabled\n", __FUNCTION__ );
    }
    else if( (ret = i2c_read(CTRL_REGSB, &ctrl_regsb)) < 0 )
    {
        printk("%s: KXSD9_dev_mwup_enb i2c_read failed\n", __FUNCTION__ );
    }
    else 
    {
        CTRL_REGSB_BITSET_mwup_disb(&ctrl_regsb);	
        KXSD9_dev.mode.mwup = DISABLED;    

        if( (ret = i2c_write(CTRL_REGSB,ctrl_regsb)) < 0 )
        {
             printk("%s: KXSD9_dev_mwup_disb->i2c_write failed\n", __FUNCTION__ );
        }        
    }

    if(ret < 0)
    {
        KXSD9_dev.mode.mwup = mwup_prev;
    }
    else
    {
        //KXSD9_waitq_wkup_processes();
    }
    
    mutex_unlock(&(KXSD9_dev.lock)); 

    trace_out() ;
    return ret;
}

int KXSD9_dev_get_sensitivity(KXSD9_sensitivity_t *sensitivity)
{
    int ret = 0;
    trace_in() ;

    mutex_lock(&(KXSD9_dev.lock));

    if( KXSD9_dev.valid == eFALSE )
    {
        printk("%s: KXSD9_dev_get_sensitivity called with dev DS invalid\n", __FUNCTION__ );
        ret = -1;
    }
    else 
    {
        sensitivity->counts_per_g = KXSD9_dev.settings.sensitivity.counts_per_g;
        sensitivity->error_RT = KXSD9_dev.settings.sensitivity.error_RT;
    }

    mutex_unlock(&(KXSD9_dev.lock));

    trace_out() ;
    return ret;
}

int KXSD9_dev_get_zero_g_offset(KXSD9_zero_g_offset_t *offset)
{
    int ret = 0;
    trace_in() ;

    mutex_lock(&(KXSD9_dev.lock));

    if( KXSD9_dev.valid == eFALSE )
    {
        printk("%s: KXSD9_dev_get_zero_g_offset called with dev DS invalid\n", __FUNCTION__ );
        ret = -1;
    }
    else 
    {
        offset->counts= KXSD9_dev.settings.zero_g_offset.counts;
        offset->error_RT = KXSD9_dev.settings.zero_g_offset.error_RT;
    }

    mutex_unlock(&(KXSD9_dev.lock));

    trace_out() ;
    return ret;
}


/* Add for AKMD2, joon.c.baek */
int KXSD9_dev_set_range( int param )
{
    int ret = 0 ; 
    KXSD9_module_param_t temp_param ;
    unsigned char ctrl_regsc=0, ctrl_regsb=0;

    trace_in() ;

    temp_param.mwup = KXSD9_dev.mode.mwup;
    temp_param.mwup_rsp = KXSD9_dev.mode.mwup_rsp;
    temp_param.R_T = KXSD9_dev.settings.R_T ;
    temp_param.BW = KXSD9_dev.settings.BW ;

    mutex_lock(&(KXSD9_dev.lock));

    if( KXSD9_dev.valid == eFALSE )
    {
        printk("%s: KXSD9_dev_set_range called with dev DS invalid\n",
        __FUNCTION__ );
        ret = -1;
    }

    else 
    {
        switch( param ) 
        {
            case R2g_T1g:
                temp_param.R_T = R2g_T1g ;
                break ;

            case R2g_T1p5g:
                temp_param.R_T = R2g_T1p5g ;
                break ;

            case R4g_T2g:
                temp_param.R_T = R4g_T2g ;
                break ;

            case R4g_T3g:
                temp_param.R_T = R4g_T3g ;
                break ;

            case R6g_T3g:
                temp_param.R_T = R6g_T3g ;
                break ;

            case R6g_T4p5g:
                temp_param.R_T = R6g_T4p5g ;
                break ;

            case R8g_T4g:
                temp_param.R_T = R8g_T4g ;
                break ;

            case R8g_T6g:
                temp_param.R_T = R8g_T6g ;
                break ;

            default:
                printk("%s: KXSD9_dev_set_range called invalid\n", __FUNCTION__);
                ret = -1 ;
                break ;

            set_val(&ctrl_regsb, &ctrl_regsc, &temp_param);
        }
    }
    mutex_unlock(&(KXSD9_dev.lock));
    
    trace_out() ;
    return ret ;
}

unsigned short KXSD9_dev_get_threshold( void )
{
    trace_in();
    trace_out();
    return KXSD9_dev.threshold;
}

void KXSD9_dev_set_threshold( unsigned short new_threshold )
{
    trace_in();
    KXSD9_dev.threshold = new_threshold;
    trace_out();
}

short KXSD9_dev_get_bandwidth( void )
{
    trace_in() ;

    trace_out() ;
    return KXSD9_dev.settings.BW ;
}

int KXSD9_dev_set_bandwidth( int param )
{
    int ret = 0 ; 
    KXSD9_module_param_t temp_param ;
    unsigned char ctrl_regsc=0, ctrl_regsb=0;
    
    trace_in() ;

    temp_param.mwup = KXSD9_dev.mode.mwup;
    temp_param.mwup_rsp = KXSD9_dev.mode.mwup_rsp;
    temp_param.R_T = KXSD9_dev.settings.R_T ;
    temp_param.BW = KXSD9_dev.settings.BW ;

    mutex_lock(&(KXSD9_dev.lock));

    if( KXSD9_dev.valid == eFALSE )
    {
        printk("%s: KXSD9_dev_set_bandwidth called with dev DS invalid\n",
        __FUNCTION__);
        ret = -1;
    }
    else 
    {
        switch( param ) 
        {
            case BW50:
                temp_param.BW = BW50 ;
                break ;

            case BW100:
                temp_param.BW = BW100 ;
                break ;

            case BW500:
                temp_param.R_T = BW500 ;
                break ;

            case BW1000:
                temp_param.R_T = BW1000 ;
                break ;

            case BW2000:
                temp_param.R_T = BW2000 ;
                break ;

            case BW_no_filter:
                temp_param.R_T = BW_no_filter ;
                break ;

            default:
                printk("%s: KXSD9_dev_set_bandwidth called invalid\n", __FUNCTION__);
                ret = -1 ;
                break ;

            set_val(&ctrl_regsb, &ctrl_regsc, &temp_param);
        }
    }
    mutex_unlock(&(KXSD9_dev.lock));
    
    trace_out() ;
    return ret ;
}

int KXSD9_dev_set_status( int module, int param ) 
{
    trace_in() ;

    if( param == STANDBY ) 
    {
        KXSD9_operation_mode = KXSD9_operation_mode & (~(1 << module)) ;
    }
    else if( param == NORMAL )
    {
    	KXSD9_operation_mode = KXSD9_operation_mode | ( 1 << module ) ;
    }
    else
    {
        printk( "KXSD9_dev_get_status() Fault argument!!! module= %d, param=%d\n", module, param  ) ;
    }

    trace_out() ;
    return KXSD9_operation_mode ;
}

int KXSD9_dev_get_status( void )
{
    trace_in() ;

    trace_out() ;
    return KXSD9_operation_mode ;
}

int KXSD9_dev_set_operation_mode( int module, int param )
{
    int status= 0 ;

    trace_in() ;

    status= KXSD9_dev_set_status( module, param ) ;

    switch( status )
    {
        case STANDBY:
            KXSD9_dev_disable() ;
            KXSD9_timer.saved_timer_state= TIMER_OFF ;
            KXSD9_timer_disable() ;
            break ;
        case ONLYACCEL:
        case ONLYCOMPASS:
        case ACCELCOMPASS:
            KXSD9_dev_enable() ;
            KXSD9_timer.saved_timer_state= TIMER_ON ;
            KXSD9_timer_enable( KXSD9_timer.polling_time ) ;
            break ;
        default:
            KXSD9_dev_disable() ;
            KXSD9_timer.saved_timer_state= TIMER_OFF ;
            KXSD9_timer_disable() ;
            printk("%s: KXSD9_dev_set_operation_mode Fault argument!!! Disable of all in force\n", __FUNCTION__ );
            break ;
    }

    switch( status )
    {
        case STANDBY:
            KXSD9_report_func= NULL ;
            printk("%s: KXSD9_dev_set_operation_mode STANDBY\n", __FUNCTION__ );
            break ;
        case ONLYACCEL:
            KXSD9_report_func= KXSD9_report_sensorhal ;
            printk("%s: KXSD9_dev_set_operation_mode ONLYACCEL\n", __FUNCTION__ );
            break ;
        case ONLYCOMPASS:
            KXSD9_report_func= KXSD9_report_akmd2 ;
            printk("%s: KXSD9_dev_set_operation_mode ONLYCOMPASS\n", __FUNCTION__ );
            break ;
        case ACCELCOMPASS:
            KXSD9_report_func= KXSD9_report_sensorhal_akmd2 ;
            printk("%s: KXSD9_dev_set_operation_mode ACCELCOMPASS\n", __FUNCTION__ );
            break ;
        default:
            KXSD9_report_func= NULL ;
            printk("%s: KXSD9_dev_set_operation_mode Fault argument!!! Disable of all in force\n", __FUNCTION__ );
            break ;
    }
    
    trace_out() ; 
    return status ;
}

extern KXSD9_acc_t cal_data;

int KXSD9_dev_get_acc(KXSD9_acc_t *acc)
{
    int ret = 0;
    trace_in() ;

    mutex_lock(&(KXSD9_dev.lock));

    if( KXSD9_dev.valid == eFALSE )
    {
        printk("%s: KXSD9_dev_get_acc called with dev DS invalid\n", __FUNCTION__);
        ret = -1;
    }
    else if( KXSD9_dev.mode.dev_state == DISABLED )
    {
        printk("%s: KXSD9_dev_get_acc: dev is DISABLED\n", __FUNCTION__);
        ret = -1;
    }
    else if( KXSD9_dev.mode.mwup == ENABLED )
    {
        printk("%s: KXSD9_dev_get_acc: trying to read the acc when the motion wakeup feature is enabled\n", __FUNCTION__);
        ret = -1;
    }
    else if( acc == NULL )
    {
        ret = -1;
    }
    else
    {
        if( (ret = get_X_acc(&(acc->X_acc)) ) < 0 )
        {
            printk("%s: get_X_acc failed\n", __FUNCTION__ );
        }
        else if( (ret = get_Y_acc(&(acc->Y_acc)) ) < 0 )
        {
            printk("%s: get_Y_acc failed\n", __FUNCTION__ );	 
        }
        else if( (ret = get_Z_acc(&(acc->Z_acc)) ) < 0 )
        {
            printk("%s: get_Z_acc failed\n", __FUNCTION__ );	 
        }
        acc->X_acc -= cal_data.X_acc;
        acc->Y_acc -= cal_data.Y_acc;
        acc->Z_acc -= cal_data.Z_acc;
    }

    mutex_unlock(&(KXSD9_dev.lock));

    trace_out() ;
    return ret;
}


int KXSD9_dev_get_default(KXSD9_default_t *acc)
{
    int ret = 0;
    trace_in() ;

    acc->x1 = DEFAULT_ACC_X1;
    acc->y1 = DEFAULT_ACC_Y1;
    acc->z1 = DEFAULT_ACC_Z1;
    acc->x2 = DEFAULT_ACC_X2;
    acc->y2 = DEFAULT_ACC_Y2;
    acc->z2 = DEFAULT_ACC_Z2;

    trace_out() ;
    return ret;
}


/*********************************************************/
static inline int get_X_acc(unsigned short *X_acc)
{
    trace_in() ;
    trace_out() ;
    return get_acc(XOUT_H, XOUT_L, X_acc);
}

static inline int get_Y_acc(unsigned short *Y_acc)
{
    trace_in() ;
    trace_out() ;
    return get_acc(YOUT_H, YOUT_L, Y_acc);
}

static inline int get_Z_acc(unsigned short *Z_acc)
{
    trace_in() ;
    trace_out() ;
    return get_acc(ZOUT_H, ZOUT_L, Z_acc);
}

static int get_acc(char reg_H, char reg_L, unsigned short *acc)
{
    int ret = 0;
    char datah,datal;
    unsigned short aux = 0;
    trace_in() ;

    if( (ret = i2c_read( reg_H, &datah)) < 0 )
    {
        printk("%s: i2c_read(_H) failed\n", __FUNCTION__ );
    }
    else
    {
        if( (ret = i2c_read( reg_L , &datal)) < 0 )
        {
            printk( "%s: i2c_read(_L) failed\n", __FUNCTION__ );
        }
        else
        {
            aux = datah;
            aux<<= 8;
            aux|= datal;
            aux>>= 4;
            aux&= 0x0FFF;
            *acc = aux;
        }
    }
    trace_out() ;
    return ret;
}

/*i2c read function*/
/*KXSD9_dev.client should be set before calling this function.
   If KXSD9_dev.valid = eTRUE then KXSD9_dev.client will b valid
   This function should be called from the functions in this file. The 
   callers should check if KXSD9_dev.valid = eTRUE before
   calling this function. If it is eFALSE then this function should not
   be called. Init function is the only exception*/
static int i2c_read(unsigned char reg, unsigned char *data)
{
    int ret = 0, i = 0;
    struct i2c_msg msg[1];
    unsigned char aux[1];
    
    trace_in() ;

    if(KXSD9_dev.client->addr == NULL)
    {
        printk("%s: can't search i2c client adapter\n", __FUNCTION__);
        trace_out();
        return -ENODEV;    
    }

    /* Write */
    msg[0].addr  = KXSD9_dev.client->addr;
    msg[0].flags = I2C_M_WR;
    msg[0].len   = 1;
    aux[0]       = reg;
    msg[0].buf   = aux;  

    for(i = 0; i < 10; i++)
    {        
        ret = i2c_transfer(KXSD9_dev.client->adapter, msg, 1) == 1 ? 0 : -EIO;
        if (ret == 0) 
        {
          break;
        }
        mdelay(1);
    }

    if(i == 10)
    {
        printk("%s: i2c_read->i2c_transfer failed, ret= %d\n", __FUNCTION__, ret );
        trace_out();
        return ret;
    }

    /* Read */
    msg[0].addr  = KXSD9_dev.client->addr;
    msg[0].flags = I2C_M_RD;
    msg[0].len   = 1;
    msg[0].buf   = data;

    for(i = 0; i < 10; i++)
    {
        ret = i2c_transfer(KXSD9_dev.client->adapter, msg, 1) == 1 ? 0 : -EIO;
        if (ret == 0)
        {
            trace_out();
            return 0;
        }   
        mdelay(1);
    }

    printk("%s: i2c_read->i2c_transfer failed, ret= %d\n", __FUNCTION__, ret );
    trace_out();
    return ret;
}

/*i2c write function*/
/*KXSD9_dev.client should be set before calling this function.
   If KXSD9_dev.valid = eTRUE then KXSD9_dev.client will b valid
   This function should be called from the functions in this file. The 
   callers should check if KXSD9_dev.valid = eTRUE before
   calling this function. If it is eFALSE then this function should not
   be called. Init function is the only exception*/
static int i2c_write( unsigned char reg, unsigned char data )
{
    int ret = 0, i = 0;
    struct i2c_msg msg[1];
    unsigned char buf[2];
    
    trace_in() ;

    if(KXSD9_dev.client->addr == NULL)
    {
        printk("%s: can't search i2c client adapter\n", __FUNCTION__);
        trace_out();
        return -ENODEV;    
    }

    /* Write */
    msg[0].addr  = KXSD9_dev.client->addr;
    msg[0].flags = I2C_M_WR;
    msg[0].len   = 2;

    buf[0]       = reg;
    buf[1]       = data;
    msg[0].buf   = buf;

    for (i = 0; i < 10; i++)
    {
        ret =  i2c_transfer(KXSD9_dev.client->adapter, msg, 1) == 1 ? 0 : -EIO;
        if(ret == 0)
        {
            trace_out();
            return 0;
        }
        mdelay(1);
    }

    printk("%s: i2c_write->i2c_transfer failed, ret= %d\n", __FUNCTION__, ret );
    trace_out();
    return ret;
}

