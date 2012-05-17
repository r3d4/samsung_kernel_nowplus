
/******************************************************
KXSD9 Test application
*******************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>


typedef struct
{
     unsigned short X_acc;
     unsigned short Y_acc;
    unsigned short Z_acc;
}KXSD9_acc_t;
 	       
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


#define KXSD9_IOC_MAGIC     0xF6
#define KXSD9_IOC_GET_ACC           _IOR( KXSD9_IOC_MAGIC, 0, KXSD9_acc_t ) 
#define KXSD9_IOC_GET_SENSITIVITY   _IOR( KXSD9_IOC_MAGIC, 4, \
KXSD9_sensitivity_t ) 
#define KXSD9_IOC_GET_ZERO_G_OFFSET _IOR( KXSD9_IOC_MAGIC, 5, \
KXSD9_zero_g_offset_t )
#define KXSD9_IOC_DISB_MWUP         _IO( KXSD9_IOC_MAGIC, 1 ) 
#define KXSD9_IOC_ENB_MWUP          _IOW( KXSD9_IOC_MAGIC, 2, unsigned short) 
#define KXSD9_IOC_MWUP_WAIT         _IO( KXSD9_IOC_MAGIC, 3 )  
#define KXSD9_IOC_GET_DEFAULT       _IOR( KXSD9_IOC_MAGIC, 6, KXSD9_acc_t )  
#define KXSD9_IOC_SET_MODE          _IOWR( KXSD9_IOC_MAGIC, 9, int )

#define XXX_DEVICE_FILE_NAME "/dev/kxsd9_accel"
  
static char buf[12];
   
int main()
{
    const char *trans_file_name;
    int fp, count, ret;
    ssize_t written = 0;
 	       
    ///////////////////////////////////////////////////////////////
    printf( "1) KXSD9 test device open  \n" );
    trans_file_name = XXX_DEVICE_FILE_NAME;
    fp = open( (char *)trans_file_name, O_RDWR | O_NONBLOCK );
    if( fp < 0 ) {
      	  printf( "device open error : %d   \n", fp );
   	            	    
   	      return 0;
    }
    printf( "open successful\n" ) ;
    usleep(10000) ;

    printf( "2) KXSD9 set mode   \n" );
    int i = 1;
    ret= ioctl( fp, KXSD9_IOC_SET_MODE, &i ) ;
    printf( "ret: %d", ret ) ;
    usleep(10000) ;
       	            	        
/*   	            	        
    printf( "2) KXSD9 test IOCTL   \n" );
    ret= ioctl( fp, KXSD9_IOC_GET_ACC, buf ) ;
    printf( "ret: %d", buf ) ;
    for( count= 0 ; count < 12 ; count++ )
    	printf( "%c\t", buf[count] ) ;
    printf( "\n" ) ;
    usleep(10000) ;

    ret= ioctl( fp, KXSD9_IOC_GET_SENSITIVITY, buf ) ;
    printf( "ret: %d", buf ) ;
    for( count= 0 ; count < 12 ; count++ )
    	printf( "%c\t", buf[count] ) ;
    printf( "\n" ) ;
    usleep(10000) ;

    ret= ioctl( fp, KXSD9_IOC_GET_ZERO_G_OFFSET, buf ) ;
    printf( "ret: %d", buf ) ;
    for( count= 0 ; count < 12 ; count++ )
    	printf( "%c\t", buf[count] ) ;
    printf( "\n" ) ;
    usleep(10000) ;
    
    ret= ioctl( fp, KXSD9_IOC_DISB_MWUP, buf ) ;
    printf( "ret: %d, Disable MWUP\n", ret ) ;
    usleep(10000) ;
    
    ret= ioctl( fp, KXSD9_IOC_ENB_MWUP, buf ) ;
    printf( "ret: %d, Enable MWUP\n", ret ) ;
    usleep(10000) ;
   
    ret= ioctl( fp, KXSD9_IOC_MWUP_WAIT, buf ) ;
    printf( "ret: %d, Wait MWUP\n", ret ) ;
    usleep(10000) ;
    
    ret= ioctl( fp, KXSD9_IOC_GET_DEFAULT, buf ) ;
    printf( "ret: %d", buf ) ;
    for( count= 0 ; count < 12 ; count++ )
    	printf( "%c\t", buf[count] ) ;
    printf( "\n" ) ;
    usleep(10000) ;

    ret= ioctl( fp, KXSD9_IOC_GET_ACC, buf ) ;
    printf( "ret: %d", buf ) ;
    //for( count= 0 ; count < 12 ; count++ )
    //	printf( "%c\t", buf[count] ) ;
    printf( "\n" ) ;
    usleep(10000) ;
*/    
    printf( "3) KXSD9 test Release   \n" );
    ret= close( fp ) ; 
    printf( "Return:\t%d\n", ret ) ;
    usleep(10000) ;

   return ret ;
}
