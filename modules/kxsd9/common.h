#ifndef _COMMON_H
#define _COMMON_H

#include <linux/kernel.h>

#ifdef DEBUG
    #define error(fmt,arg...) printk(KERN_CRIT "KXSD9 error msg: " fmt "\n",## arg)
    #define debug(fmt,arg...) printk(KERN_CRIT "KXSD9_Msg: " fmt "\n",## arg)
    #define trace_in()  debug( "%s +", __FUNCTION__ )
    #define trace_out() debug( "%s -", __FUNCTION__ )
#else
	#define error(fmt,arg...)
    #define debug(fmt,arg...)
    #define trace_in()
    #define trace_out()
#endif

#ifdef DEBUG_VALUE
    #define debug_value(fmt,arg...) printk(KERN_CRIT "KXSD9_Msg: " fmt "\n",## arg)
#endif


/*flag states*/
#define SET   1
#define RESET 0

/*dev status*/
#define OK  1
#define N_OK  -1

#define YES  1
#define NO  0

#define OMAP3430_GPIO_ACC_INT   115
#endif
