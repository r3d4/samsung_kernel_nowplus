#ifndef _KXSD9_DEV_H
#define _KXSD9_DEV_H

#include <linux/i2c.h>
#include <linux/workqueue.h>
#include <mach/hardware.h>

#include "KXSD9_main.h"

/*dev_state and mwup*/
#define ENABLED    1
#define DISABLED   0

/*mwup_rsp*/
#define LATCHED    1
#define UNLATCHED  0
#define DONT_CARE  2

/*device range and thershold R_T*/
#define R2g_T1g       1
#define R2g_T1p5g     2
#define R4g_T2g       3
#define R4g_T3g       4
#define R6g_T3g       5
#define R6g_T4p5g     6
#define R8g_T4g       7
#define R8g_T6g       8

/*operational bandwidth*/
#define BW50          1
#define BW100         2 
#define BW500         3
#define BW1000        4
#define BW2000        5
#define BW_no_filter  6

/*operation mode, Add for AKMD2, joon.c.baek*/
#define ACCEL       0
#define COMPASS     1

#define STANDBY     0
#define NORMAL      1

#define STANDBY         0
#define ONLYACCEL       1
#define ONLYCOMPASS     2
#define ACCELCOMPASS    3

#define DEFAULT_ACC_X1		2048
#define DEFAULT_ACC_Y1		1229
#define DEFAULT_ACC_Z1		2048
#define DEFAULT_ACC_X2		2048
#define DEFAULT_ACC_Y2		2048
#define DEFAULT_ACC_Z2		2867

#define POLLING_INTERVAL    100

typedef struct 
{
    unsigned short x1;
    unsigned short y1;
    unsigned short z1;
    unsigned short x2;
    unsigned short y2;
    unsigned short z2;
}KXSD9_default_t;

typedef struct 
{
    unsigned short counts_per_g;
    /*error at room temperature*/
    unsigned short error_RT;
}KXSD9_sensitivity_t;

typedef struct
{
    unsigned short counts;
    /*error at room temperature*/
    unsigned short error_RT;
}KXSD9_zero_g_offset_t;

typedef struct
{
    unsigned short mwup_rsp;
}KXSD9_mwup_rsp_t;

/*Function prototypes*/
extern int KXSD9_dev_init(struct i2c_client *, KXSD9_module_param_t *); // dev.c
extern int KXSD9_dev_exit(void);	// dev.c

extern void KXSD9_dev_mutex_init(void); // dev.c

extern void KXSD9_dev_work_func(struct work_struct *);	 // main.c	

extern int KXSD9_dev_get_sensitivity(KXSD9_sensitivity_t *); // dev.c
extern int KXSD9_dev_get_zero_g_offset(KXSD9_zero_g_offset_t *); // dev.c
extern int KXSD9_dev_get_acc(KXSD9_acc_t *); // dev.c
extern int KXSD9_dev_get_default(KXSD9_default_t *); // dev.c

extern int KXSD9_dev_set_range( int ) ;
extern short KXSD9_dev_get_bandwidth( void ) ;
extern int KXSD9_dev_set_bandwidth( int ) ;
extern int KXSD9_dev_set_operation_mode( int, int ) ;

extern int KXSD9_dev_set_status( int, int ) ;
extern int KXSD9_dev_get_status( void ) ;

extern int KXSD9_dev_enable(void);	// dev.c
extern int KXSD9_dev_disable(void);	// dev.c

extern int KXSD9_dev_mwup_enb(unsigned short); // dev.c
extern int KXSD9_dev_mwup_disb(void);	// dev.c

extern int KXSD9_dev_mwup_status(void);

extern unsigned short KXSD9_dev_get_threshold(void);
extern void KXSD9_dev_set_threshold(unsigned short);

#endif

