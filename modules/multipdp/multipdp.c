#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/random.h>
#include <linux/if_arp.h>
#include <linux/proc_fs.h>
#include <linux/freezer.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/tty_flip.h>
#include <linux/poll.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/rtc.h>

typedef struct pdp_arg {
	unsigned char	id;
	char		ifname[16];
} __attribute__ ((packed)) pdp_arg_t;

#define IOC_MZ2_MAGIC		(0xC1)

#define HN_PDP_ACTIVATE		_IOWR(IOC_MZ2_MAGIC, 0xe0, pdp_arg_t)
#define HN_PDP_DEACTIVATE	_IOW (IOC_MZ2_MAGIC, 0xe1, pdp_arg_t)
#define HN_PDP_ADJUST		_IOW (IOC_MZ2_MAGIC, 0xe2, int)
#define HN_PDP_TXSTART		_IO(IOC_MZ2_MAGIC, 0xe3)
#define HN_PDP_TXSTOP		_IO(IOC_MZ2_MAGIC, 0xe4)
#define HN_PDP_CSDSTART		_IO(IOC_MZ2_MAGIC, 0xe5)
#define HN_PDP_CSDSTOP		_IO(IOC_MZ2_MAGIC, 0xe6)
#define HN_PDP_ATCMDSTART		_IO(IOC_MZ2_MAGIC, 0xe7)
#define HN_PDP_ATCMDSTOP		_IO(IOC_MZ2_MAGIC, 0xe8)

#include <mach/hardware.h>
#include <asm/uaccess.h>

#define MULTIPDP_ERROR
#undef USE_LOOPBACK_PING
#define MULTIPDP_HI_PRIORITY

#ifdef USE_LOOPBACK_PING
#include <linux/ip.h>
#include <linux/icmp.h>
#include <net/checksum.h>
#endif

#define MULTIPDP_ERROR
#ifdef MULTIPDP_ERROR
#define EPRINTK(X...) \
		do { \
			printk("%s(): ", __FUNCTION__); \
			printk(X); \
		} while (0)
#else
#define EPRINTK(X...)		do { } while (0)
#endif

#define CONFIG_MULTIPDP_DEBUG 1

#if (CONFIG_MULTIPDP_DEBUG > 0)
#define DPRINTK(N, X...) \
		do { \
			if (N <= CONFIG_MULTIPDP_DEBUG) { \
				printk("%s(): ", __FUNCTION__); \
				printk(X); \
			} \
		} while (0)
#else
#define DPRINTK(N, X...)	do { } while (0)
#endif

#define _MULTIPDP_DEBUG_HEXDUMP 1

#define MAX_PDP_CONTEXT			10

#define MAX_PDP_DATA_LEN		1500

#define MAX_PDP_PACKET_LEN		(MAX_PDP_DATA_LEN + 4 + 2)

#define VNET_PREFIX				"pdp"

#define APP_DEVNAME				"multipdp"

#define DPRAM_DEVNAME			"/dev/dpram1"

#define DEV_TYPE_NET			0
#define DEV_TYPE_SERIAL			1 

#define DEV_FLAG_STICKY			0x1

#define CSD_MAJOR_NUM			245
#define CSD_MINOR_NUM			0

struct pdp_hdr {
	u16	len;	
	u8	id;
	u8	control;
} __attribute__ ((packed));

struct pdp_info {
	u8		id;
	unsigned		type;
	unsigned		flags;

	u8		*tx_buf;

	union {
		struct {
			struct net_device	*net;
			struct net_device_stats	stats;
			struct work_struct	xmit_task;
		} vnet_u;

		struct {
			struct tty_driver	tty_driver[4];
			int			refcount;
			struct tty_struct	*tty_table[1];
			struct ktermios		*termios[1];
			struct ktermios		*termios_locked[1];
			char			tty_name[16];
			struct tty_struct	*tty;
			struct semaphore	write_lock;
		} vs_u;
	} dev_u;
#define vn_dev		dev_u.vnet_u
#define vs_dev		dev_u.vs_u
};

static struct pdp_info *pdp_table[MAX_PDP_CONTEXT];
static DECLARE_MUTEX(pdp_lock);
static DECLARE_MUTEX(netwrite_lock);

static struct task_struct *dpram_task;
static struct file *dpram_filp;
static DECLARE_COMPLETION(dpram_complete);
static struct task_struct *dpram_ctl_task;
static struct file *dpram_ctl_filp;
static DECLARE_COMPLETION(dpram_ctl_complete);

static int g_adjust = 0;
static unsigned long workqueue_data = 0;
static unsigned char pdp_rx_buf[MAX_PDP_DATA_LEN];

extern int pdp_tx_flag;
int pdp_csd_flag = 0;
int pdp_atcmd_flag = 1;

int fp_vsCSD = 0;
int fp_vsGPS = 0;
int fp_vsROUTER = 0;
int fp_vsEXGPS = 0;

static int pdp_mux(struct pdp_info *dev, const void *data, size_t len);
static int pdp_demux(void);
static inline struct pdp_info * pdp_get_serdev(const char *name);
static void vnet_defer_xmit(struct work_struct *data);

#ifdef _MULTIPDP_DEBUG_HEXDUMP
#define isprint(c)	((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
static void hexdump(const char *buf, int len)
{
	char str[80], octet[10];
	int ofs, i, l;

	for (ofs = 0; ofs < len; ofs += 16) {
		sprintf( str, "%03d: ", ofs );

		for (i = 0; i < 16; i++) {
			if ((i + ofs) < len)
				sprintf( octet, "%02x ", buf[ofs + i] );
			else
				strcpy( octet, "   " );

			strcat( str, octet );
		}
			strcat( str, "  " );
			l = strlen( str );

		for (i = 0; (i < 16) && ((i + ofs) < len); i++)
			str[l++] = isprint( buf[ofs + i] ) ? buf[ofs + i] : '.';

		str[l] = '\0';
		printk( "%s\n", str );
	}
}

static void print_debug_current_time(void)
{
	struct rtc_time tm;
	struct timeval time;

	do_gettimeofday(&time);

	rtc_time_to_tm(time.tv_sec, &tm);

	printk(KERN_INFO"Kernel Current Time info - %02d%02d%02d%02d%02d.%ld \n", tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, time.tv_usec);

}
#endif
static inline struct file *dpram_open(const char* name)
{
DPRINTK(1,"%d\n",__LINE__);
	int ret;
	struct file *filp;
	struct termios termios;
	mm_segment_t oldfs;
DPRINTK(1,"%d\n",__LINE__);
	filp = filp_open(DPRAM_DEVNAME, O_RDWR|O_NONBLOCK, 0);
	if (IS_ERR(filp)) {
		DPRINTK(1, "filp_open() failed~!: %ld\n", PTR_ERR(filp));
		return NULL;
	}
DPRINTK(1,"%d\n",__LINE__);
	oldfs = get_fs(); set_fs(get_ds());
	ret = filp->f_op->unlocked_ioctl(filp, 
				TCGETA, (unsigned long)&termios);
	set_fs(oldfs);
	if (ret < 0) {
		DPRINTK(1, "f_op->ioctl() failed: %d\n", ret);
		filp_close(filp, current->files);
		return NULL;
	}

	termios.c_cflag = CS8 | CREAD | HUPCL | CLOCAL | B115200;
	termios.c_iflag = IGNBRK | IGNPAR;
	termios.c_lflag = 0;
	termios.c_oflag = 0;
	termios.c_cc[VMIN] = 1;
	termios.c_cc[VTIME] = 1;

	oldfs = get_fs(); set_fs(get_ds());
	ret = filp->f_op->unlocked_ioctl(filp, 
				TCSETA, (unsigned long)&termios);
	set_fs(oldfs);
	if (ret < 0) {
		DPRINTK(1, "f_op->ioctl() failed: %d\n", ret);
		filp_close(filp, current->files);
		return NULL;
	}
DPRINTK(1,"%d\n",__LINE__);
	return filp;
}

static inline void phone_on(struct file *filp, int onoff)
{
#define IOC_MZ_MAGIC				('h')
#define HN_DPRAM_PHONE_ON			_IO(IOC_MZ_MAGIC, 0xd0)

	mm_segment_t oldfs;
	int ret;

	/* tunon phone */
	DPRINTK(1, "try to tunon phone\n");
	oldfs = get_fs(); set_fs(get_ds());
	ret = filp->f_op->ioctl(filp->f_dentry->d_inode, filp, 
				HN_DPRAM_PHONE_ON, (unsigned long)0);
	set_fs(oldfs);
	if (ret < 0) {
		DPRINTK(1, "f_op->ioctl() failed: %d\n", ret);
//		filp_close(filp, current->files);
		return NULL;
	}
	DPRINTK(1, "try to tunon phone OK...\n");
}

static inline void dpram_close(struct file *filp)
{
	filp_close(filp, current->files);
}

static inline int dpram_poll(struct file *filp)
{
	int ret;
	unsigned int mask;
	struct poll_wqueues wait_table;
	mm_segment_t oldfs;

DPRINTK(1,"%d\n",__LINE__);
	poll_initwait(&wait_table);
	for (;;) {
		set_current_state(TASK_INTERRUPTIBLE);

		oldfs = get_fs(); set_fs(get_ds());
		mask = filp->f_op->poll(filp, &wait_table.pt);
		set_fs(oldfs);
DPRINTK(1,"%d\n",__LINE__);
		if (mask & POLLIN) {
			ret = 0;
			break;
		}
DPRINTK(1,"%d\n",__LINE__);
		if (wait_table.error) {
			DPRINTK(1, "error in f_op->poll()\n");
			ret = wait_table.error;
			break;
		}
DPRINTK(1,"%d\n",__LINE__);
		if (signal_pending(current)) {
			ret = -ERESTARTSYS;
			break;
		}
DPRINTK(1,"%d\n",__LINE__);
		schedule();
	}
	set_current_state(TASK_RUNNING);
	poll_freewait(&wait_table);
DPRINTK(1,"%d\n",__LINE__);
	return ret;
}

static inline int dpram_write(struct file *filp, const void *buf, size_t count,
			      int nonblock)
{
	int ret, n = 0;
	mm_segment_t oldfs;
	
	while (count) {
		dpram_filp->f_flags |= O_NONBLOCK;
		oldfs = get_fs(); set_fs(get_ds());
		ret = filp->f_op->write(filp, buf + n, count, &filp->f_pos);
		set_fs(oldfs);
		dpram_filp->f_flags &= ~O_NONBLOCK;
		if (ret < 0) {
			if (ret == -EAGAIN && !nonblock) {
				continue;
			}
			DPRINTK(1, "f_op->write() failed: %d\n", ret);
			return ret;
		}
		n += ret;
		count -= ret;
	}
	
	DPRINTK(1,"%d\n",__LINE__);
	return n;
}

static inline int dpram_read(struct file *filp, void *buf, size_t count)
{
	int ret, n = 0;
	mm_segment_t oldfs;
	
	while (count) {
		dpram_filp->f_flags |= O_NONBLOCK;
		oldfs = get_fs(); set_fs(get_ds());
		ret = filp->f_op->read(filp, buf + n, count, &filp->f_pos);
		set_fs(oldfs);
		dpram_filp->f_flags &= ~O_NONBLOCK;
		if (ret < 0) {
			if (ret == -EAGAIN) continue;
			DPRINTK(1, "f_op->read() failed: %d\n", ret);
			return ret;
		}
		n += ret;
		count -= ret;
	}
	
	DPRINTK(1,"%d\n",__LINE__);
	return n;
}

static inline int dpram_flush_rx(struct file *filp, size_t count)
{
	int ret, n = 0;
	char *buf;
	mm_segment_t oldfs;

	buf = kmalloc(count, GFP_KERNEL);
	if (buf == NULL) return -ENOMEM;

	while (count) {
		dpram_filp->f_flags |= O_NONBLOCK;
		oldfs = get_fs(); set_fs(get_ds());
		ret = filp->f_op->read(filp, buf + n, count, &filp->f_pos);
		set_fs(oldfs);
		dpram_filp->f_flags &= ~O_NONBLOCK;
		if (ret < 0) {
			if (ret == -EAGAIN) continue;
			DPRINTK(1, "f_op->read() failed: %d\n", ret);
			kfree(buf);
			return ret;
		}
		n += ret;
		count -= ret;
	}
	kfree(buf);
	
	DPRINTK(1,"%d\n",__LINE__);
	return n;
}


static int dpram_thread(void *data)
{
	int ret = 0;
	struct file *filp;

	//unsigned int new_policy=SCHED_FIFO;
	//struct sched_param new_param = { .sched_priority = 1 };

	dpram_task = current;

	daemonize("dpram_thread");

	strcpy(current->comm, "multipdp");

//	struct sched_param schedpar;
//   schedpar.sched_priority = 1;
//   sched_setscheduler(current, SCHED_FIFO, &schedpar);
	/* set signals to accept */
	siginitsetinv(&current->blocked, sigmask(SIGUSR1));
	recalc_sigpending();


	filp = dpram_open(DPRAM_DEVNAME);
	if (filp == NULL) {
		goto out;
	}
	dpram_filp = filp;
DPRINTK(1,"%d\n",__LINE__);
	complete(&dpram_complete);
	
//	phone_on(filp, 1);

	while (1) {
		ret = dpram_poll(filp);

DPRINTK(1,"%d\n",__LINE__);
		if (ret == -ERESTARTSYS) {
			if (sigismember(&current->pending.signal, SIGUSR1)) {
				sigdelset(&current->pending.signal, SIGUSR1);
				recalc_sigpending();
				ret = 0;
				break;
			}
		}
		
		else if (ret < 0) {
			EPRINTK("dpram_poll() failed\n");
			break;
		}
		
		else {
			char ch;

			//sched_setscheduler_save(new_policy, new_param);
			
			//struct sched_param schedpar;
                        //schedpar.sched_priority = 1;
                        //sched_setscheduler(current, SCHED_FIFO, &schedpar);
        
DPRINTK(1,"%d\n",__LINE__);
			ret = dpram_read(dpram_filp, &ch, sizeof(ch));

			if(ret < 0) {
				return ret;
			}

			if (ch == 0x7f) {
				pdp_demux();
			}

			//sched_setscheduler_restore();

		}
		try_to_freeze();
	}

DPRINTK(1,"%d\n",__LINE__);
	dpram_close(filp);
	dpram_filp = NULL;

out:
	dpram_task = NULL;
	complete_and_exit(&dpram_complete, ret);
}


static int start_at_port(struct file *filp);
static int dpram_ctl_thread(void *data)
{
	int ret = 0;
	struct file *filp;

	//unsigned int new_policy=SCHED_FIFO;
	//struct sched_param new_param = { .sched_priority = 1 };

	dpram_ctl_task = current;

	daemonize("dpram_ctl_thread");

	strcpy(current->comm, "multipdp");

	siginitsetinv(&current->blocked, sigmask(SIGUSR1));
	recalc_sigpending();


	filp = dpram_open("/dev/dpram0");
	if (filp == NULL) {
		goto out;
	}
	dpram_ctl_filp = filp;

	complete(&dpram_ctl_complete);
	
//	phone_on(filp, 1);
	
	start_at_port(filp);

	while (1) {
		ret = dpram_poll(filp);

		if (ret == -ERESTARTSYS) {
			if (sigismember(&current->pending.signal, SIGUSR1)) {
				sigdelset(&current->pending.signal, SIGUSR1);
				recalc_sigpending();
				ret = 0;
				break;
			}
		}
		
		else if (ret < 0) {
			EPRINTK("dpram_poll() ctl failed\n");
			break;
		}
		
		else {
			char ch;

			//sched_setscheduler_save(new_policy, new_param);
                        
                       // struct sched_param schedpar;
                       //schedpar.sched_priority = 1;
                       //sched_setscheduler(current, SCHED_FIFO, &schedpar);
        
			ret = dpram_read(dpram_ctl_filp, &ch, sizeof(ch));

			if(ret < 0) {
				return ret;
			}

			if (ch == 0x7f) {
//				pdp_demux();
			}

//			sched_setscheduler_restore();

		}
		try_to_freeze();
	}

	dpram_close(filp);
	dpram_ctl_filp = NULL;

out:
	dpram_ctl_task = NULL;
	complete_and_exit(&dpram_ctl_complete, ret);
}

static int vnet_open(struct net_device *net)
{
DPRINTK(1,"%d\n",__LINE__);
	struct pdp_info *dev = (struct pdp_info *)net->ml_priv;
	INIT_WORK(&dev->vn_dev.xmit_task, NULL);
	netif_start_queue(net);

	return 0;
}

static int vnet_stop(struct net_device *net)
{
DPRINTK(1,"%d\n",__LINE__);
	struct pdp_info *dev = (struct pdp_info *)net->ml_priv;
	netif_stop_queue(net);
	cancel_work_sync(&dev->vn_dev.xmit_task);

	return 0;
}

static void vnet_defer_xmit(struct work_struct *data)
{

	int ret = 0;
	unsigned long flags;
	
	local_irq_save(flags);

	struct sk_buff *skb = (struct sk_buff *)workqueue_data;
	struct net_device *net = (struct net_device *)skb->dev;
	struct pdp_info *dev = (struct pdp_info *)net->ml_priv;

	ret = pdp_mux(dev, skb->data, skb->len);

	if (ret < 0) {
		dev->vn_dev.stats.tx_dropped++;
	}
	
	else {
		net->trans_start = jiffies;
		dev->vn_dev.stats.tx_bytes += skb->len;
		dev->vn_dev.stats.tx_packets++;
	}

	dev_kfree_skb_any(skb);
	netif_wake_queue(net);

	local_irq_restore(flags);

}

static int vnet_start_xmit(struct sk_buff *skb, struct net_device *net)
{
	struct pdp_info *dev = (struct pdp_info *)net->ml_priv;

	skb->dev = net;

	workqueue_data = (unsigned long)skb;
	PREPARE_WORK(&dev->vn_dev.xmit_task,vnet_defer_xmit);
	schedule_work(&dev->vn_dev.xmit_task);
	netif_stop_queue(net);
DPRINTK(1,"%d\n",__LINE__);
	return 0;
}

static int vnet_recv(struct pdp_info *dev, size_t len)
{
	struct sk_buff *skb;
	int ret;

	if (!dev) {
		return 0;
	}
DPRINTK(1,"%d\n",__LINE__);
	if (!netif_running(dev->vn_dev.net)) {
		DPRINTK(1, "%s(id: %u) is not running\n", 
			dev->vn_dev.net->name, dev->id);
		return -ENODEV;
	}

	skb = alloc_skb(len, GFP_KERNEL);

	if (skb == NULL) {
		DPRINTK(1, "alloc_skb() failed\n");
		return -ENOMEM;
	}

	ret = dpram_read(dpram_filp, skb->data, len);

	if (ret < 0) {
		DPRINTK(1, "dpram_read() failed: %d\n", ret);
		dev_kfree_skb_any(skb);
		return ret;
	}

	skb_put(skb, ret);

	skb->dev = dev->vn_dev.net;
	skb->protocol = __constant_htons(ETH_P_IP);
	
	if(in_interrupt())
		netif_rx(skb);
	else
		netif_rx_ni(skb);

	dev->vn_dev.stats.rx_packets++;
	dev->vn_dev.stats.rx_bytes += skb->len;
	return 0;
}

static struct net_device_stats *vnet_get_stats(struct net_device *net)
{
	struct pdp_info *dev = (struct pdp_info *)net->ml_priv;
	return &dev->vn_dev.stats;
}

static void vnet_tx_timeout(struct net_device *net)
{
	struct pdp_info *dev = (struct pdp_info *)net->ml_priv;

	net->trans_start = jiffies;
	dev->vn_dev.stats.tx_errors++;
	netif_wake_queue(net);
}


static const struct net_device_ops vnetOps =
{
        .ndo_open		= vnet_open,
	.ndo_stop		= vnet_stop,
	.ndo_start_xmit	= vnet_start_xmit,
	.ndo_get_stats		= vnet_get_stats,
	.ndo_tx_timeout		= vnet_tx_timeout,
};

void vnet_setup(struct net_device *dev)
{
        SET_ETHTOOL_OPS(dev, &vnetOps);
	return;

}


static struct net_device *vnet_add_dev(void *priv)
{
	int ret;
	struct net_device *net;

	net = kmalloc(sizeof(*net), GFP_KERNEL);
	memset(net, 0, sizeof(*net));

	net  = alloc_netdev(sizeof(*priv),VNET_PREFIX "%d", vnet_setup);
	#if 0
	net->open		= vnet_open;
	net->stop		= vnet_stop;
	net->hard_start_xmit	= vnet_start_xmit;
	net->get_stats		= vnet_get_stats;
	net->tx_timeout		= vnet_tx_timeout;
	#endif
	net->type		= ARPHRD_PPP;
	net->hard_header_len 	= 0;
	net->mtu		= MAX_PDP_DATA_LEN;
	net->addr_len		= 0;
	net->tx_queue_len	= 1000;
	net->flags		= IFF_POINTOPOINT | IFF_NOARP | IFF_MULTICAST;
	net->watchdog_timeo	= 15 * HZ;
	net->ml_priv		= priv;
	ret = register_netdev(net);

	if (ret != 0) {
		printk(KERN_ERR "register_netdevice failed: %d\n", ret);
		free_netdev(net);
		return NULL;
	}

	return net;
}

static void vnet_del_dev(struct net_device *net)
{
	unregister_netdev(net);
	kfree(net);
}

static int vs_open(struct tty_struct *tty, struct file *filp)
{
	struct pdp_info *dev;

	dev = pdp_get_serdev(tty->driver->name);

	if (dev == NULL) {
		return -ENODEV;
	}
	
	switch (dev->id) {
		case 1:
			fp_vsCSD = 1;
			break;

		case 25:
			fp_vsROUTER = 1;
			break;

		case 5:
			fp_vsGPS = 1;
			break;

		case 6:
			fp_vsEXGPS = 1;
			break;

		default:
			break;
	}


	tty->driver_data = (void *)dev;
	tty->low_latency = 1;
	dev->vs_dev.tty = tty;
	
	printk("[MULTIPDP] vs_open called. %s\n", tty->driver->name);

	return 0;
}

static void vs_close(struct tty_struct *tty, struct file *filp)
{
	struct pdp_info *dev = (struct pdp_info *)tty->driver_data;

	switch (dev->id) {
		case 1:
			fp_vsCSD = 0;
			break;

		case 25:
			fp_vsROUTER = 0;
			break;

		case 5:
			fp_vsGPS = 0;
			break;

		case 6:
			fp_vsEXGPS = 0;
			break;

		default:
			break;
	}

	printk("[MULTIPDP] vs_close called. %s\n", tty->driver->name);

}


static int vs_write(struct tty_struct *tty,
		const unsigned char *buf, int count)
{
	int ret;
	DPRINTK(1,"%d\n",__LINE__);
	struct pdp_info *dev = (struct pdp_info *)tty->driver_data;

#ifdef _MULTIPDP_DEBUG_HEXDUMP
	printk("[MULTIPDP] write :. %s\n", tty->driver->name);
	hexdump(buf, count);
#endif

	ret = pdp_mux(dev, buf, count);

	if (ret == 0) {
		ret = count;
	}
	DPRINTK(1,"%d\n",__LINE__);
	return ret;
}

static int vs_write_room(struct tty_struct *tty) 
{
	return 8192;//TTY_FLIPBUF_SIZE;
}

static int vs_chars_in_buffer(struct tty_struct *tty) 
{
	return 0;
}

static int vs_read(struct pdp_info *dev, size_t len)
{
	int ret = 0;
	
	if(!dev){
		return 0;
	}

	ret = dpram_read(dpram_filp, pdp_rx_buf, len);

#ifdef _MULTIPDP_DEBUG_HEXDUMP
	printk("[MULTIPDP] read : devid=%d\n", dev->id);
	hexdump(pdp_rx_buf, len);
#endif
DPRINTK(1,"%d\n",__LINE__);
	if (ret != len) {
		return ret;
	}
	if (ret > 0) {
		if((dev->id == 25 && !fp_vsROUTER) || (dev->id == 1 && !fp_vsCSD) || (dev->id == 5 && !fp_vsGPS)) {
			printk("[MULTIPDP] vs_read : %s, discard data.\n", dev->vs_dev.tty->name);
		}
		else {
			tty_insert_flip_string(dev->vs_dev.tty, pdp_rx_buf, ret);
			tty_flip_buffer_push(dev->vs_dev.tty);
		}
	}
DPRINTK(1,"%d\n",__LINE__);
	return 0;
}

static int vs_ioctl(struct tty_struct *tty, struct file *file, 
		    unsigned int cmd, unsigned long arg)
{
	return -ENOIOCTLCMD;
}

static void vs_break_ctl(struct tty_struct *tty, int break_state)
{
}

static struct tty_operations multipdp_tty_ops = {
	.open 		= vs_open,
	.close 		= vs_close,
	.write 		= vs_write,
	.write_room = vs_write_room,
	.ioctl 		= vs_ioctl,
	.chars_in_buffer = vs_chars_in_buffer,

	/* TODO: add more operations */
};

static int vs_add_dev(struct pdp_info *dev)
{
	struct tty_driver *tty_driver;

	switch (dev->id) {
		case 1:
			tty_driver = &dev->vs_dev.tty_driver[0];
			tty_driver->minor_start = CSD_MINOR_NUM;
			break;

		case 25:
			tty_driver = &dev->vs_dev.tty_driver[1];
			tty_driver->minor_start = 1;
			break;

		case 5:
			tty_driver = &dev->vs_dev.tty_driver[2];
			tty_driver->minor_start = 2;
			break;

		case 6:
			tty_driver = &dev->vs_dev.tty_driver[3];
			tty_driver->minor_start = 3;
			break;

		default:
			tty_driver = NULL;
	}

	if (!tty_driver) {
		printk("tty driver is NULL!\n");
		return -1;
	}

	tty_driver->owner = THIS_MODULE;
	tty_driver->magic	= TTY_DRIVER_MAGIC;
	tty_driver->driver_name	= "multipdp";
	tty_driver->name	= dev->vs_dev.tty_name;
	tty_driver->major	= CSD_MAJOR_NUM;
	tty_driver->num		= 1;
	tty_driver->type	= TTY_DRIVER_TYPE_SERIAL;
	tty_driver->subtype	= SERIAL_TYPE_NORMAL;
	tty_driver->flags	= TTY_DRIVER_REAL_RAW;
	//tty_driver->refcount	= dev->vs_dev.refcount;
	tty_driver->ttys	= dev->vs_dev.tty_table;
	tty_driver->termios	= dev->vs_dev.termios;
	tty_driver->termios_locked	= dev->vs_dev.termios_locked;
	
	tty_set_operations(tty_driver, &multipdp_tty_ops);
	
	#if 0
	tty_driver->open	= vs_open;
	tty_driver->close	= vs_close;
	tty_driver->write	= vs_write;
	tty_driver->write_room	= vs_write_room;
	tty_driver->chars_in_buffer	= vs_chars_in_buffer;
	tty_driver->ioctl	= vs_ioctl;
	tty_driver->break_ctl	= vs_break_ctl;
	#endif
	tty_driver->init_termios = tty_std_termios;
	tty_driver->init_termios.c_cflag = (B115200 | CS8 | CREAD | CLOCAL | HUPCL);

	return tty_register_driver(tty_driver);
}

static void vs_del_dev(struct pdp_info *dev)
{
	struct tty_driver *tty_driver = NULL;

	switch (dev->id) {
		case 1:
			tty_driver = &dev->vs_dev.tty_driver[0];
			break;

		case 25:
			tty_driver = &dev->vs_dev.tty_driver[1];
			break;

		case 5:
			tty_driver = &dev->vs_dev.tty_driver[2];
			break;

		case 6:
			tty_driver = &dev->vs_dev.tty_driver[3];
			break;
	}

		tty_unregister_driver(tty_driver);
}

static inline struct pdp_info * pdp_get_dev(u8 id)
{
	int slot;

	for (slot = 0; slot < MAX_PDP_CONTEXT; slot++) {
		if (pdp_table[slot] && pdp_table[slot]->id == id) {
			return pdp_table[slot];
		}
	}
	return NULL;
}

static inline struct pdp_info * pdp_get_serdev(const char *name)
{
	int slot;
	struct pdp_info *dev;

	for (slot = 0; slot < MAX_PDP_CONTEXT; slot++) {
		dev = pdp_table[slot];
		if (dev && dev->type == DEV_TYPE_SERIAL &&
		    strcmp(name, dev->vs_dev.tty_name) == 0) {
			return dev;
		}
	}
	return NULL;
}

static inline int pdp_add_dev(struct pdp_info *dev)
{
	int slot;

	if (pdp_get_dev(dev->id)) {
		return -EBUSY;
	}

	for (slot = 0; slot < MAX_PDP_CONTEXT; slot++) {
		if (pdp_table[slot] == NULL) {
			pdp_table[slot] = dev;
			return slot;
		}
	}
	return -ENOSPC;
}

static inline struct pdp_info * pdp_remove_dev(u8 id)
{
	int slot;
	struct pdp_info *dev;

	for (slot = 0; slot < MAX_PDP_CONTEXT; slot++) {
		if (pdp_table[slot] && pdp_table[slot]->id == id) {
			dev = pdp_table[slot];
			pdp_table[slot] = NULL;
			return dev;
		}
	}
	return NULL;
}

static inline struct pdp_info * pdp_remove_slot(int slot)
{
	struct pdp_info *dev;

	dev = pdp_table[slot];
	pdp_table[slot] = NULL;
	return dev;
}

static int pdp_mux(struct pdp_info *dev, const void *data, size_t len   )
{
	int ret;
	size_t nbytes;
	u8 *tx_buf;
	struct pdp_hdr *hdr;
	const u8 *buf;
DPRINTK(1,"%d\n",__LINE__);
#if 0
	unsigned int new_policy=SCHED_FIFO;
	struct sched_param new_param = { .sched_priority = 1 };
	sched_setscheduler_save(new_policy, new_param);
#endif	
//        struct sched_param schedpar;
//         schedpar.sched_priority = 1;
//        sched_setscheduler(current, SCHED_FIFO, &schedpar);
   


	tx_buf = dev->tx_buf;
	hdr = (struct pdp_hdr *)(tx_buf + 1);
	buf = data;

	hdr->id = dev->id;
	hdr->control = 0;

	if(pdp_tx_flag){
		if((hdr->id != 1) && (hdr->id != 5) && (hdr->id != 6)){
			if(!pdp_atcmd_flag)
				return -EAGAIN;
		}
	}

	while (len) {
		if (len > MAX_PDP_DATA_LEN) {
			nbytes = MAX_PDP_DATA_LEN;
		} else {
			nbytes = len;
		}
		hdr->len = nbytes + sizeof(struct pdp_hdr);

		tx_buf[0] = 0x7f;
		
		memcpy(tx_buf + 1 + sizeof(struct pdp_hdr), buf,  nbytes);
		
		tx_buf[1 + hdr->len] = 0x7e;
DPRINTK(1,"%d\n",__LINE__);
		ret = dpram_write(dpram_filp, tx_buf, hdr->len + 2, dev->type == DEV_TYPE_NET ? 1 : 0);

		if (ret < 0) {
			DPRINTK(1, "dpram_write() failed: %d\n", ret);
			return ret;
		}

		buf += nbytes;
		len -= nbytes;
	}

	//sched_setscheduler_restore();
DPRINTK(1,"%d\n",__LINE__);
	return 0;
}

static int pdp_demux(void)
{
	int ret;
	u8 ch;
	size_t len;
	struct pdp_info *dev = NULL;
	struct pdp_hdr hdr;

#if 0
	unsigned int new_policy=SCHED_FIFO;
	struct sched_param new_param = { .sched_priority = 1 };

	sched_setscheduler_save(new_policy, new_param);
#endif
//        struct sched_param schedpar;
//         schedpar.sched_priority = 1;
//        sched_setscheduler(current, SCHED_FIFO, &schedpar);
        DPRINTK(1,"%d\n",__LINE__);
	down(&pdp_lock);
	ret = dpram_read(dpram_filp, &hdr, sizeof(hdr));

	if (ret < 0) {
		up(&pdp_lock);
		return ret;
	}

	len = hdr.len - sizeof(struct pdp_hdr);

	dev = pdp_get_dev(hdr.id);

	if (dev == NULL) {
		printk("invalid id: %u, there is no existing device.\n", hdr.id);
		ret = -ENODEV;
		goto err;
	}
	up(&pdp_lock);

	switch (dev->type) {
		case DEV_TYPE_NET:
			ret = vnet_recv(dev, len);
			break;
		case DEV_TYPE_SERIAL:
			ret = vs_read(dev, len);
			break;
		default:
			ret = -1;
	}

	if (ret < 0) {
		goto err;
	}
DPRINTK(1,"%d\n",__LINE__);
	ret = dpram_read(dpram_filp, &ch, sizeof(ch));

	if (ret < 0 || ch != 0x7e) {
		return ret;
	}

	//sched_setscheduler_restore();
DPRINTK(1,"%d\n",__LINE__);
	return 0;
err:
	up(&pdp_lock);
	dpram_flush_rx(dpram_filp, len + 1);
	return ret;
}

static int pdp_activate(pdp_arg_t *pdp_arg, unsigned type, unsigned flags)
{
	int ret;
	struct pdp_info *dev;
	struct net_device *net;

	DPRINTK(2, "id: %d\n", pdp_arg->id);
DPRINTK(1,"%d\n",__LINE__);

	dev = kmalloc(sizeof(struct pdp_info) + MAX_PDP_PACKET_LEN, GFP_KERNEL);
	if (dev == NULL) {
		DPRINTK(1, "out of memory\n");
		return -ENOMEM;
	}
	memset(dev, 0, sizeof(struct pdp_info));

	if (type == DEV_TYPE_NET) {
		dev->id = pdp_arg->id + g_adjust;
	}
	else {
		dev->id = pdp_arg->id;
	}


	dev->type = type;
	dev->flags = flags;
	dev->tx_buf = (u8 *)(dev + 1);

	if (type == DEV_TYPE_NET) {
		net = vnet_add_dev((void *)dev);
		if (net == NULL) {
			kfree(dev);
			return -ENOMEM;
		}

		dev->vn_dev.net = net;
		strcpy(pdp_arg->ifname, net->name);

		down(&pdp_lock);
		ret = pdp_add_dev(dev);
		if (ret < 0) {
			DPRINTK(1, "pdp_add_dev() failed\n");
			up(&pdp_lock);
			vnet_del_dev(dev->vn_dev.net);
			kfree(dev);
			return ret;
		}
		up(&pdp_lock);

		DPRINTK(1, "%s(id: %u) network device created\n", 
			net->name, dev->id);
	} else if (type == DEV_TYPE_SERIAL) {
		init_MUTEX(&dev->vs_dev.write_lock);
		strcpy(dev->vs_dev.tty_name, pdp_arg->ifname);

		ret = vs_add_dev(dev);
		if (ret < 0) {
			kfree(dev);
			return ret;
		}

		down(&pdp_lock);
		ret = pdp_add_dev(dev);
		if (ret < 0) {
			DPRINTK(1, "pdp_add_dev() failed\n");
			up(&pdp_lock);
			vs_del_dev(dev);
			kfree(dev);
			return ret;
		}
		up(&pdp_lock);

		if (dev->id == 1) {
			DPRINTK(1, "%s(id: %u) serial device is created.\n",
					dev->vs_dev.tty_driver[0].name, dev->id);
		}

		else if (dev->id == 25) {
			DPRINTK(1, "%s(id: %u) serial device is created.\n",
					dev->vs_dev.tty_driver[1].name, dev->id);
		}

		else if (dev->id == 5) {
			DPRINTK(1, "%s(id: %u) serial device is created.\n",
					dev->vs_dev.tty_driver[2].name, dev->id);
		}

		else if (dev->id == 6) {
			DPRINTK(1, "%s(id: %u) serial device is created.\n",
					dev->vs_dev.tty_driver[3].name, dev->id);
		}
	}

	return 0;
}

static int pdp_deactivate(pdp_arg_t *pdp_arg, int force)
{
	struct pdp_info *dev = NULL;

	DPRINTK(1, "id: %d\n", pdp_arg->id);
DPRINTK(1,"%d\n",__LINE__);
	down(&pdp_lock);

	if (pdp_arg->id == 1) {
		DPRINTK(1, "Channel ID is 1, we will remove the network device (pdp) of channel ID: %d.\n",
				pdp_arg->id + g_adjust);
	}

	else {
		DPRINTK(1, "Channel ID: %d\n", pdp_arg->id);
	}

	pdp_arg->id = pdp_arg->id + g_adjust;

	DPRINTK(1, "ID is adjusted, new ID: %d\n", pdp_arg->id);

	dev = pdp_get_dev(pdp_arg->id);

	if (dev == NULL) {
		DPRINTK(1, "not found id: %u\n", pdp_arg->id);
		up(&pdp_lock);
		return -EINVAL;
	}
	if (!force && dev->flags & DEV_FLAG_STICKY) {
		DPRINTK(1, "sticky id: %u\n", pdp_arg->id);
		up(&pdp_lock);
		return -EACCES;
	}

	pdp_remove_dev(pdp_arg->id);
	up(&pdp_lock);

	if (dev->type == DEV_TYPE_NET) {
		DPRINTK(1, "%s(id: %u) network device removed\n", 
			dev->vn_dev.net->name, dev->id);
		vnet_del_dev(dev->vn_dev.net);
	} else if (dev->type == DEV_TYPE_SERIAL) {
		if (dev->id == 1) {
			DPRINTK(1, "%s(id: %u) serial device removed\n",
					dev->vs_dev.tty_driver[0].name, dev->id);
		}

		else if (dev->id == 25) {
			DPRINTK(1, "%s(id: %u) serial device removed\n",
					dev->vs_dev.tty_driver[1].name, dev->id);
		}

		else if (dev->id == 5) {
			DPRINTK(1, "%s(id: %u) serial device removed\n",
					dev->vs_dev.tty_driver[2].name, dev->id);
		}

		else if (dev->id == 6) {
			DPRINTK(1, "%s(id: %u) serial device removed\n",
					dev->vs_dev.tty_driver[3].name, dev->id);
		}

		vs_del_dev(dev);
	}

	kfree(dev);

	return 0;
}

static void __exit pdp_cleanup(void)
{
	int slot;
	struct pdp_info *dev;

	down(&pdp_lock);
	for (slot = 0; slot < MAX_PDP_CONTEXT; slot++) {
		dev = pdp_remove_slot(slot);
		if (dev) {
			if (dev->type == DEV_TYPE_NET) {
				DPRINTK(1, "%s(id: %u) network device removed\n", 
					dev->vn_dev.net->name, dev->id);
				vnet_del_dev(dev->vn_dev.net);
			} else if (dev->type == DEV_TYPE_SERIAL) {
				if (dev->id == 1) {
					DPRINTK(1, "%s(id: %u) serial device removed\n", 
							dev->vs_dev.tty_driver[0].name, dev->id);
				}

				else if (dev->id == 25) {
					DPRINTK(1, "%s(id: %u) serial device removed\n",
							dev->vs_dev.tty_driver[1].name, dev->id);
				}

				else if (dev->id == 5) {
					DPRINTK(1, "%s(id: %u) serial device removed\n",
							dev->vs_dev.tty_driver[2].name, dev->id);
				}

				else if (dev->id == 6) {
					DPRINTK(1, "%s(id: %u) serial device removed\n",
							dev->vs_dev.tty_driver[3].name, dev->id);
				}

				vs_del_dev(dev);
			}

			kfree(dev);
		}
	}
	up(&pdp_lock);
}

static int pdp_adjust(const int adjust)
{
DPRINTK(1,"%d\n",__LINE__);
	g_adjust = adjust;
	printk("adjusting value: %d\n", adjust);
	return 0;
}

static int multipdp_ioctl(struct inode *inode, struct file *file, 
			      unsigned int cmd, unsigned long arg)
{
	int ret, adjust;
	pdp_arg_t pdp_arg;
	
	printk("[MULTIPDP] multipdp_ioctl: cmd=%d.\n", cmd);

	switch (cmd) {
	case HN_PDP_ACTIVATE:
		if (copy_from_user(&pdp_arg, (void *)arg, sizeof(pdp_arg)))
			return -EFAULT;
		ret = pdp_activate(&pdp_arg, DEV_TYPE_NET, 0);
		DPRINTK(1,"%d\n",__LINE__);
		if (ret < 0) {
			return ret;
		}
		return copy_to_user((void *)arg, &pdp_arg, sizeof(pdp_arg));

	case HN_PDP_DEACTIVATE:
	DPRINTK(1,"%d\n",__LINE__);
		if (copy_from_user(&pdp_arg, (void *)arg, sizeof(pdp_arg)))
			return -EFAULT;
		return pdp_deactivate(&pdp_arg, 0);

	case HN_PDP_ADJUST:
	DPRINTK(1,"%d\n",__LINE__);
		if (copy_from_user(&adjust, (void *)arg, sizeof (int)))
			return -EFAULT;

		return pdp_adjust(adjust);
	case HN_PDP_TXSTART:
		pdp_tx_flag = 0;
		printk("[MULTIPDP] telephony clear the pdp_tx_flag. data is writable.\n");
		return 0;
			
	case HN_PDP_TXSTOP:
		pdp_tx_flag = 1;
		printk("[MULTIPDP] telephony set the pdp_tx_flag. data is not writable.\n");
		return 0;
	case HN_PDP_CSDSTART:
		pdp_csd_flag = 0;
		printk("[MULTIPDP] telephony clear the pdp_csd_flag. ttyCSD is available.\n");
		return 0;
			
	case HN_PDP_CSDSTOP:
		pdp_csd_flag = 1;
		printk("[MULTIPDP] telephony set the pdp_csd_flag. ttyCSD is not available.\n");
		return 0;
	case HN_PDP_ATCMDSTART:
		pdp_atcmd_flag = 1;
		printk("[MULTIPDP] DataRouter set the pdp_atcmd_flag. ATCMD is available.\n");
		return 0;
			
	case HN_PDP_ATCMDSTOP:
		pdp_atcmd_flag = 0;
		printk("[MULTIPDP] DataRouter clear the pdp_atcmd_flag. ATCMD is not available.\n");
		return 0;
	}

	return -EINVAL;
}

static struct file_operations multipdp_fops = {
	.owner =	THIS_MODULE,
	.ioctl =	multipdp_ioctl,
	.llseek =	no_llseek,
};

static struct miscdevice multipdp_dev = {
	.minor =	132,
	.name =		APP_DEVNAME,
	.fops =		&multipdp_fops,
};

#ifdef CONFIG_PROC_FS
static int multipdp_proc_read(char *page, char **start, off_t off,
			      int count, int *eof, void *data)
{

	char *p = page;
	int len;

	down(&pdp_lock);

	p += sprintf(p, "Protector Multipdp Driver - Device List\n");
	for (len = 0; len < MAX_PDP_CONTEXT; len++) {
		struct pdp_info *dev = pdp_table[len];
		if (!dev) continue;

		p += sprintf(p,
			     "name: %s\t, id: %-3u, type: %-7s, flags: 0x%04x\n",
			     dev->type == DEV_TYPE_NET ? 
			     dev->vn_dev.net->name : dev->vs_dev.tty_name,
			     dev->id, 
			     dev->type == DEV_TYPE_NET ? "network" : "serial",
			     dev->flags);
	}
	up(&pdp_lock);

	len = (p - page) - off;
	if (len < 0)
		len = 0;

	*eof = (len <= count) ? 1 : 0;
	*start = page + off;

	return len;
}
#endif

static int start_at_port(struct file *filp)
{
	int ret;
	char ch;
	int timeout=0;
	char data[] = {
    	0x7f, 0x0c, 0x00, 0x30, 0x09, 0x00, 0x0f, 0x00, 0x0d, 0x0f, 0x01, 0x04, 0x01, 0x7e
	};
	char data1[] = {
		0x7f, 0x0c, 0x00, 0x31, 0x09, 0x00, 0x10, 0x00, 0x0d, 0x0f, 0x01, 0x04, 0x01, 0x7e
	};
	
	ret = dpram_write(dpram_ctl_filp, data, sizeof(data),  0);

	if (ret < 0) {
		DPRINTK(1, "start_at_port() failed 1: %d\n", ret);
	}

	timeout=1000;
	do{
		ret = dpram_poll(filp);
		if(ret < 0 && timeout-- <= 0) break;
		timeout=1000;
		ret = dpram_read(dpram_ctl_filp, &ch, sizeof(ch));
	}while(ret == 1);
	
	ret = dpram_write(dpram_ctl_filp, data1, sizeof(data1),  0);

	if (ret < 0) {
		DPRINTK(1, "start_at_port() failed 2: %d\n", ret);
	}
	
	timeout=1000;
	do{
		ret = dpram_poll(filp);
		if(ret < 0 && timeout-- <= 0) break;
		timeout=1000;
		ret = dpram_read(dpram_ctl_filp, &ch, sizeof(ch));
	}while(ret == 1);
	
	return 0;    
}

static int __init multipdp_init(void)
{
	int ret;
	pdp_arg_t pdp_arg = { .id = 1, .ifname = "ttyCSD", };
	pdp_arg_t router_arg = { .id = 25, .ifname = "ttyROUTER", };
	pdp_arg_t gps_arg = { .id = 5, .ifname = "ttyGPS", };
	pdp_arg_t xgps_arg = { .id = 6, .ifname = "ttyXtraGPS", };

	ret = kernel_thread(dpram_thread, NULL, 0);
	if (ret < 0) {
		EPRINTK("kernel_thread() failed\n");
		return ret;
	}
	wait_for_completion(&dpram_complete);
	if (!dpram_task) {
		EPRINTK("DPRAM I/O thread error\n");
		return -EIO;
	}
#if 0
	ret = kernel_thread(dpram_ctl_thread, NULL, 0);
	if (ret < 0) {
		EPRINTK("kernel_thread() dpram_ctl_thread failed\n");
		return ret;
	}
	wait_for_completion(&dpram_ctl_complete);
	if (!dpram_ctl_task) {
		EPRINTK("DPRAM I/O thread error\n");
		return -EIO;
	}
#endif

	ret = pdp_activate(&pdp_arg, DEV_TYPE_SERIAL, DEV_FLAG_STICKY);
	if (ret < 0) {
		EPRINTK("failed to create a serial device for CSD\n");
		goto err0;
	}

	ret = pdp_activate(&router_arg, DEV_TYPE_SERIAL, DEV_FLAG_STICKY);
	if (ret < 0) {
		EPRINTK("failed to create a serial device for ROUTER\n");
		goto err1;
	}

	ret = pdp_activate(&gps_arg, DEV_TYPE_SERIAL, DEV_FLAG_STICKY);
	if (ret < 0) {
		EPRINTK("failed to create a serial device for GPS\n");
		goto err1;
	}

	ret = pdp_activate(&xgps_arg, DEV_TYPE_SERIAL, DEV_FLAG_STICKY);
	if (ret < 0) {
		EPRINTK("failed to create a serial device for Xtra GPS\n");
		goto err1;
	}

	ret = misc_register(&multipdp_dev);
	if (ret < 0) {
		EPRINTK("misc_register() failed\n");
		goto err1;
	}

#ifdef CONFIG_PROC_FS
	create_proc_read_entry(APP_DEVNAME, 0, 0, 
			       multipdp_proc_read, NULL);
#endif

	printk(KERN_INFO 
	       "[MULTIPDP] multipdp driver is loaded.\n");
	return 0;

err1:
	pdp_deactivate(&pdp_arg, 1);

err0:
	if (dpram_task) {
		send_sig(SIGUSR1, dpram_task, 1);
		wait_for_completion(&dpram_complete);
	}
	
	if (dpram_ctl_task) {
		send_sig(SIGUSR1, dpram_ctl_task, 1);
		wait_for_completion(&dpram_complete);
	}
	return ret;
}

static void __exit multipdp_exit(void)
{
#ifdef CONFIG_PROC_FS
	remove_proc_entry(APP_DEVNAME, 0);
#endif

	misc_deregister(&multipdp_dev);

	pdp_cleanup();

	if (dpram_task) {
		send_sig(SIGUSR1, dpram_task, 1);
		wait_for_completion(&dpram_complete);
	}
	
	if (dpram_ctl_task) {
		send_sig(SIGUSR1, dpram_ctl_task, 1);
		wait_for_completion(&dpram_ctl_complete);
	}
}

module_init(multipdp_init);
module_exit(multipdp_exit);

MODULE_AUTHOR("SAMSUNG ELECTRONICS CO., LTD");
MODULE_DESCRIPTION("Multiple PDP Muxer / Demuxer");
MODULE_LICENSE("GPL");

