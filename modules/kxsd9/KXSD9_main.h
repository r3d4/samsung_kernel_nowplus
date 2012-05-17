#ifndef _KXSD9_MAIN_H
#define _KXSD9_MAIN_H

#define TIMER_ON    1
#define TIMER_OFF   0

/*Structure to represent module parameters*/
typedef struct 
{
    /*motion wakeup feature*/
    unsigned short mwup;
    /*motion wakeupt response type*/	
    unsigned short mwup_rsp;
    /*Range and Threshold*/	
    unsigned short R_T;
    /*Operational Bandwidth*/	
    unsigned short BW;	
}KXSD9_module_param_t;

typedef struct
{
    struct timer_list*  polling_timer ;
    unsigned long       polling_time ;
    unsigned int        current_timer_state ;
    unsigned int        saved_timer_state ;
} KXSD9_timer_t ;

typedef struct
{
    unsigned short X_acc ;
    unsigned short Y_acc ;
    unsigned short Z_acc ;
} KXSD9_acc_t ;

typedef struct 
{
    short x ;
    short y ;
    short z ;
} KXSD9_cal_t ;

typedef struct 
{
	int x ;
	int y ;
	int z ;
} kxsd9_axis_t ;

typedef struct
{
//	unsigned char*  name ;
	int    zero_g_offset ;
	int    divide_unit ;
    kxsd9_axis_t       axis ;
    int     x_y_axis_changed ;
} kxsd9_convert_t ;

#define KXSD9_ON		0
#define KXSD9_THREAD		1
#define KXSD9_MOVING		2
#define KXSD9_NEWDATA		3
#define KXSD9_FIRST_TIME	4
typedef struct
{
    struct list_head list;
    volatile unsigned long status;
} KXSD9_status_t;

extern KXSD9_timer_t KXSD9_timer ;
extern kxsd9_convert_t kxsd9_convert ;
extern KXSD9_status_t KXSD9_status ;

extern int KXSD9_operation_mode;

extern KXSD9_module_param_t* KXSD9_main_getparam( void ) ;

extern void (*KXSD9_report_func)( KXSD9_acc_t* ) ; 

extern void KXSD9_timer_timeout( unsigned long ) ;

extern int  KXSD9_timer_init( void ) ;
extern int  KXSD9_timer_exit( void ) ;

extern int  KXSD9_timer_enable( unsigned long ) ;
extern int  KXSD9_timer_disable( void ) ;

extern unsigned long KXSD9_timer_get_polling_time( void ) ;
extern void KXSD9_timer_set_polling_time( int ) ;

extern void KXSD9_report_sensorhal( KXSD9_acc_t* ) ;
extern void KXSD9_report_akmd2( KXSD9_acc_t* ) ;
extern void KXSD9_report_sensorhal_akmd2( KXSD9_acc_t* ) ;
extern void KXSD9_calibrate(void);
extern void KXSD9_init_calibration(void);

extern void KXSD9_start_thread( void );
extern void KXSD9_stop_thread( void );

#endif

