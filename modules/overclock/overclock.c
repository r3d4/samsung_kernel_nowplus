/*
	Motorola Milestone overclock module
	version 1.1-mapphone - 2011-03-30
	by Tiago Sousa <mirage@kaotik.org>, modified by nadlabak, Skrilax_CZ, tekahuna
	License: GNU GPLv2
	<http://www.gnu.org/licenses/old-licenses/gpl-2.0.html>

	Changelog:
 
	version 1.5-mapphone - 2011-03-30
	- port to Yokohama devices

	version 1.1-mapphone - 2011-03-30
	- simplified
	- added missing item to frequency table

	version 1.0-mapphone - 2010-11-19
	- automatic symbol detection
	- automatic values detection
	
	Description:

	The MPU (Microprocessor Unit) clock has 5 discrete pairs of possible
	rate frequencies and respective voltages, of which only 4 are passed
	down to cpufreq as you can see with a tool such as SetCPU.  The
	default frequencies are 125, 250, 500 and 550 MHz (and a hidden
	600).  By using this module, you are changing the highest pair in
	the tables of both cpufreq and MPU frequencies, so it becomes 125,
	250, 500 and, say, 800.  It's quite stable up to 1200; beyond
	that it quickly becomes unusable, specially over 1300, with lockups
	or spontaneous reboots.
*/

#define DRIVER_AUTHOR "Tiago Sousa <mirage@kaotik.org>, nadlabak, Skrilax_CZ, tekahuna"
#define DRIVER_DESCRIPTION "Motorola Milestone CPU overclocking"
#define DRIVER_VERSION "1.5-yokohama-beta-05"

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>

#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <asm/uaccess.h>

#include <linux/kallsyms.h>

#include <linux/notifier.h>
#include <linux/cpufreq.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/cpu.h>
#include <linux/completion.h>
#include <linux/mutex.h>

#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>

#include <mach/hardware.h>
#include <asm/system.h> 
  
#include <plat/omap-pm.h>

#include "opp_info.h"
#include "../symsearch/symsearch.h"

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESCRIPTION);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL");

//extern int cpufreq_stats_freq_update(unsigned int cpu, int index, unsigned int freq);

// opp.c
SYMSEARCH_DECLARE_FUNCTION_STATIC(int, 
			opp_get_opp_count_fp, struct device *dev);
SYMSEARCH_DECLARE_FUNCTION_STATIC(struct omap_opp *, 
			opp_find_freq_floor_fp, struct device *dev, unsigned long *freq);
SYMSEARCH_DECLARE_FUNCTION_STATIC(struct omap_opp * __deprecated,
			opp_find_by_opp_id_fp, struct device *dev, u8 opp_id);
// opp-max.c
SYMSEARCH_DECLARE_FUNCTION_STATIC(unsigned long, vsel_to_uv_fp, unsigned char vsel);
SYMSEARCH_DECLARE_FUNCTION_STATIC(unsigned char, uv_to_vsel_fp, unsigned long uv);

#ifdef OMAP4
// opp44xxdata.c
SYMSEARCH_DECLARE_FUNCTION_STATIC(struct device *, find_dev_ptr_fp, char *name);
#endif
// cpufreq_stats.c
/*SYMSEARCH_DECLARE_FUNCTION_STATIC(int, cpufreq_stats_free_table_fp, unsigned int cpu);
SYMSEARCH_DECLARE_FUNCTION_STATIC(int, cpufreq_stats_create_table_fp, 
			struct cpufreq_policy *policy, struct cpufreq_frequency_table *table); */

// voltage.c
SYMSEARCH_DECLARE_FUNCTION_STATIC(struct voltagedomain *, 
			omap_voltage_domain_get_fp, char *name);
SYMSEARCH_DECLARE_FUNCTION_STATIC(void, 
			omap_voltage_reset_fp, struct voltagedomain *voltdm);

// cpufreq.c
SYMSEARCH_DECLARE_FUNCTION_STATIC(struct cpufreq_governor *,
			__find_governor_fp, const char *str_governor);
SYMSEARCH_DECLARE_FUNCTION_STATIC(int, __cpufreq_set_policy_fp, 
			struct cpufreq_policy *data, struct cpufreq_policy *policy);

#ifdef OMAP4
#define MPU_CLK         "dpll_mpu_ck"
#define GPU_CLK         "gpu_fck"
#else
#define MPU_CLK         "dpll1_clk"
#endif

static int opp_count, enabled_mpu_opp_count, main_index, base_index, cpufreq_index;

static char orig_governor[16];
static char good_governor[16] = "userspace";

#ifdef OMAP4
static struct device *mpu_dev, *gpu_dev;
static struct omap_opp *mpu_opps, *gpu_opps;
static struct clk *mpu_clk, *gpu_clk;
#else
static struct device *mpu_dev;
static struct omap_opp *mpu_opps;
static struct clk *mpu_clk;
#endif
static struct voltagedomain *voltdm;
static struct omap_vdd_info *vdd;
static struct cpufreq_frequency_table *freq_table;
static struct cpufreq_policy *policy;


#define BUF_SIZE PAGE_SIZE
static char *buf;

static int set_governor(struct cpufreq_policy *policy, char str_governor[16])
{
	unsigned int ret = -EINVAL;
	struct cpufreq_policy *new_policy;
	struct cpufreq_governor *t;
	
	new_policy = policy;
	
	cpufreq_get_policy(new_policy, policy->cpu);;
		
	t = __find_governor_fp(str_governor);
	
	if (t != NULL) {
		new_policy->governor = t;
	} else {
		return ret;
	}

	ret = __cpufreq_set_policy_fp(policy, new_policy);
	
	policy->user_policy.policy = policy->policy;
	policy->user_policy.governor = policy->governor;
	
	return ret;
}

static int proc_info_read(char *buffer, char **buffer_location,
		off_t offset, int count, int *eof, void *data)
{
	int ret;

	if (offset > 0)
		ret = 0;
	else
#ifdef OMAP4
		ret = scnprintf(buffer, count, "cpumin=%u cpumax=%u min=%u max=%u usermin=%u usermax=%u\nmpu_clk_get_rate=%lu gpu_clk_get_rate=%lu\n",
				policy->cpuinfo.min_freq, policy->cpuinfo.max_freq, policy->min, policy->max, policy->user_policy.min, policy->user_policy.max, 
				clk_get_rate(mpu_clk) / 1000, clk_get_rate(gpu_clk) / 1000);
#else
		ret = scnprintf(buffer, count, "cpumin=%u cpumax=%u min=%u max=%u usermin=%u usermax=%u\nmpu_clk_get_rate=%lu\n",
					policy->cpuinfo.min_freq, policy->cpuinfo.max_freq, policy->min, policy->max, policy->user_policy.min, policy->user_policy.max, 
					clk_get_rate(mpu_clk) / 1000);
#endif

	return ret;
}


static int proc_freq_table_read(char *buffer, char **buffer_location,
		off_t offset, int count, int *eof, void *data)
{
	int i, ret = 0;

	if (offset > 0)
		ret = 0;
	else
		for(i = 0; freq_table[i].frequency != CPUFREQ_TABLE_END; i++) 
		{
			if(ret >= count)
				break;

			ret += scnprintf(buffer+ret, count-ret, "freq_table[%d] index=%u frequency=%u\n", i, freq_table[i].index, freq_table[i].frequency);
		}

	return ret;
}
                       
                        
static int proc_mpu_opps_read(char *buffer, char **buffer_location,
		off_t offset, int count, int *eof, void *data)
{
	int i, ret = 0;

	if (offset > 0)
		ret = 0;
	else
		// print out valid opp_id's
		for(i = main_index;i >= base_index; i--) 
		{
			mpu_opps = opp_find_by_opp_id_fp(mpu_dev, i);
			ret += scnprintf(buffer+ret, count-ret, "mpu_opps[%d] rate=%lu opp_id=%u vsel=%u u_volt=%lu\n", i, 
			mpu_opps->rate, mpu_opps->opp_id, uv_to_vsel_fp(mpu_opps->u_volt), mpu_opps->u_volt); 		
		}

	return ret;
}


static int proc_mpu_opps_write(struct file *filp, const char __user *buffer,
		unsigned long len, void *data)
{
	uint index, rate, vsel, volt, cpufreq_temp_min, cpufreq_temp_max;
	bool bad_gov_check = false;
	
	cpufreq_temp_min = policy->cpuinfo.min_freq;
	cpufreq_temp_max = policy->cpuinfo.max_freq;
	
	if(!len || len >= BUF_SIZE)
		return -ENOSPC;

	if(copy_from_user(buf, buffer, len))
		return -EFAULT;

	buf[len] = 0;
	
	if(sscanf(buf, "%d %d %d", &index, &rate, &vsel) == 3) 
	{
		//check to make sure valid opp_id is entered
		if (index < base_index || index > main_index) {
			printk(KERN_INFO "overclock: invalid parameters for mpu_opps opp_id\n");
			return len;
		}
		/* we need to lock frequency on to stop dvfs, and it can't be the
		   same opp_id we are writing to, sloppy fix goes here */
		if (index == main_index) {
			policy->min = policy->cpuinfo.min_freq =
			policy->user_policy.min =
			policy->max = policy->cpuinfo.max_freq =
			policy->user_policy.max = freq_table[cpufreq_index-1].frequency;
		}
		else {
			policy->min = policy->cpuinfo.min_freq =
			policy->user_policy.min = 
			policy->max = policy->cpuinfo.max_freq =
			policy->user_policy.max = cpufreq_temp_max;
		}	
		//we like making changes in userspace governor
		if (policy->governor->name != good_governor) {
			strcpy(orig_governor, policy->governor->name);
			set_governor(policy, good_governor);
			bad_gov_check = true;
		}
		//convert vsel to voltage in uV
		if (vsel < 100) {
			volt = vsel_to_uv_fp(vsel);
		} else {
		volt = vsel;
		}
		//lock up omap_vdd_info structure for mpu vdd & write  no
		mutex_lock(&vdd->scaling_mutex);
		//write nominal table
		vdd->volt_data[index].volt_nominal = volt;
		//write voltage dependancy table
		vdd->dep_vdd_info[0].dep_table[index].main_vdd_volt = volt;
		//unlock mpu vdd
		mutex_unlock(&vdd->scaling_mutex);
		//grab mpu_opps by opp_id
		mpu_opps = opp_find_by_opp_id_fp(mpu_dev, index);
		//write mpu_opps voltage
		mpu_opps->u_volt = volt;
		
		//undo the cpu_freq lock that stops dvfs transitioning
		if (index == main_index) {
			policy->min = policy->cpuinfo.min_freq =
			policy->user_policy.min = cpufreq_temp_min;
			policy->max = policy->cpuinfo.max_freq =
			policy->user_policy.max = rate / 1000;
		}
		else if (index == base_index) {
			policy->min = policy->cpuinfo.min_freq =
			policy->user_policy.min = rate / 1000;
			policy->max = policy->cpuinfo.max_freq =
			policy->user_policy.max = cpufreq_temp_max;
		}
		else {
			policy->min = policy->cpuinfo.min_freq =
			policy->user_policy.min = cpufreq_temp_min;
			policy->max = policy->cpuinfo.max_freq =
			policy->user_policy.max = cpufreq_temp_max;
		}
		//reset voltage to current values
		omap_voltage_reset_fp(voltdm);
		//update frequency table (index - lowest enabled opp_id)
		freq_table[index-(base_index)].frequency = rate / 1000;
		//write frequency for mpu_opps
		mpu_opps->rate = rate;
		//put us back in performance governor if we started there
		if (bad_gov_check == true) {
			set_governor(policy, orig_governor);
		}
//		cpufreq_stats_free_table_fp(0);
//		cpufreq_stats_create_table_fp(policy, freq_table);
//		cpufreq_stats_freq_update(0, cpufreq_index - index, rate / 1000);
	} 
	else
		printk(KERN_INFO "overclock: insufficient parameters for mpu_opps\n");

	return len;
}   

#ifdef OMAP4
static int proc_gpu_opps_read(char *buffer, char **buffer_location,
							  off_t offset, int count, int *eof, void *data)
{
	int ret = 0;
	unsigned long freq = ULONG_MAX;
	
	if (offset > 0)
		ret = 0;
	else {
		gpu_opps = opp_find_freq_floor_fp(gpu_dev, &freq);
		if (!gpu_opps || IS_ERR(gpu_opps)) {
			printk(KERN_INFO "overclock: could not find gpu_opps\n");
			return ret;
		}
		ret += scnprintf(buffer+ret, count-ret, "gpu_opps[%d] rate=%lu opp_id=%d u_volt=%lu\n", 
						 gpu_opps->opp_id, gpu_opps->rate, gpu_opps->opp_id, gpu_opps->u_volt);
	}
	
	return ret;
}


static int proc_gpu_opps_write(struct file *filp, const char __user *buffer,
							   unsigned long len, void *data)
{
	uint rate;
	unsigned long freq = ULONG_MAX;
		
	if(!len || len >= BUF_SIZE)
		return -ENOSPC;
	
	if(copy_from_user(buf, buffer, len))
		return -EFAULT;
	
	buf[len] = 0;
	
	if(sscanf(buf, "%d", &rate) == 1) 
	{
		gpu_opps = opp_find_freq_floor_fp(gpu_dev, &freq);
		gpu_opps->rate = rate;
	}
	else
		printk(KERN_INFO "overclock: insufficient parameters for gpu_opps\n");
	
	return len;
}
#endif

                            
static int proc_version_read(char *buffer, char **buffer_location,
		off_t offset, int count, int *eof, void *data)
{
	int ret;

	if (offset > 0)
		ret = 0;
	else
		ret = scnprintf(buffer, count, "%s\n", DRIVER_VERSION);

	return ret;
}


static int __init overclock_init(void)
{
	struct proc_dir_entry *proc_entry;
	printk(KERN_INFO "overclock: %s version %s\n", DRIVER_DESCRIPTION, DRIVER_VERSION);
	printk(KERN_INFO "overclock: by %s\n", DRIVER_AUTHOR);

	// opp.c
	SYMSEARCH_BIND_FUNCTION_TO(overclock, 
			opp_get_opp_count, opp_get_opp_count_fp);
	SYMSEARCH_BIND_FUNCTION_TO(overclock, 
			opp_find_freq_floor, opp_find_freq_floor_fp);
	SYMSEARCH_BIND_FUNCTION_TO(overclock,
			opp_find_by_opp_id, opp_find_by_opp_id_fp);
	// opp-max.c
#ifdef MAX8952
	SYMSEARCH_BIND_FUNCTION_TO(overclock,
			omap_max8952_vsel_to_uv,  vsel_to_uv_fp);
	SYMSEARCH_BIND_FUNCTION_TO(overclock,
			omap_max8952_uv_to_vsel,  uv_to_vsel_fp);
#else
	SYMSEARCH_BIND_FUNCTION_TO(overclock,
			omap_twl_vsel_to_uv,  vsel_to_uv_fp);
	SYMSEARCH_BIND_FUNCTION_TO(overclock,
			omap_twl_uv_to_vsel,  uv_to_vsel_fp);
#endif
#ifdef OMAP4
	SYMSEARCH_BIND_FUNCTION_TO(overclock,
			find_dev_ptr, find_dev_ptr_fp);
#endif
	// cpufreq_stats.c
/*	SYMSEARCH_BIND_FUNCTION_TO(overclock,
			cpufreq_stats_free_table, cpufreq_stats_free_table_fp);
	SYMSEARCH_BIND_FUNCTION_TO(overclock,
			cpufreq_stats_create_table, cpufreq_stats_create_table_fp); */
	
	// voltage.c
	SYMSEARCH_BIND_FUNCTION_TO(overclock, 
			omap_voltage_domain_get, omap_voltage_domain_get_fp);
	SYMSEARCH_BIND_FUNCTION_TO(overclock,
			omap_voltage_reset, omap_voltage_reset_fp);
	SYMSEARCH_BIND_FUNCTION_TO(overclock,
			__find_governor, __find_governor_fp);
	SYMSEARCH_BIND_FUNCTION_TO(overclock,
			__cpufreq_set_policy, __cpufreq_set_policy_fp);

	freq_table = cpufreq_frequency_get_table(0);
	
	policy = cpufreq_cpu_get(0);
	
	mpu_clk = clk_get(NULL, MPU_CLK);
	
	voltdm = omap_voltage_domain_get_fp("mpu");
	if (!voltdm || IS_ERR(voltdm)) {
		return -ENODEV;
	}
	vdd = container_of(voltdm, struct omap_vdd_info, voltdm);
	if (!vdd || IS_ERR(vdd)) {
		return -ENODEV;
	}
	opp_count = vdd->volt_data_count;
	
	mpu_dev = omap2_get_mpuss_device();
	if (!mpu_dev || IS_ERR(mpu_dev)) {
		return -ENODEV;
	}
	
#ifdef OMAP4
	gpu_clk = clk_get(NULL, GPU_CLK);
	
	gpu_dev = find_dev_ptr_fp("gpu");
	if (!gpu_dev || IS_ERR(gpu_dev)) {
		return -ENODEV;
	}
#endif
	enabled_mpu_opp_count = opp_get_opp_count_fp(mpu_dev);
	
	if (enabled_mpu_opp_count == opp_count) {
		main_index = cpufreq_index = (enabled_mpu_opp_count-1);
	} else {
		main_index = enabled_mpu_opp_count;
		cpufreq_index = (enabled_mpu_opp_count-1);
	}
	
	base_index = main_index-cpufreq_index;
		
	buf = (char *)vmalloc(BUF_SIZE);

	proc_mkdir("overclock", NULL);
	proc_entry = create_proc_read_entry("overclock/info", 0444, NULL, proc_info_read, NULL);
	proc_entry = create_proc_read_entry("overclock/freq_table", 0444, NULL, proc_freq_table_read, NULL);
	proc_entry = create_proc_read_entry("overclock/mpu_opps", 0644, NULL, proc_mpu_opps_read, NULL);
	proc_entry->write_proc = proc_mpu_opps_write;
#ifdef OMAP4
	proc_entry = create_proc_read_entry("overclock/gpu_opps", 0644, NULL, proc_gpu_opps_read, NULL);
	proc_entry->write_proc = proc_gpu_opps_write;
#endif
	proc_entry = create_proc_read_entry("overclock/version", 0444, NULL, proc_version_read, NULL);

	return 0;
}


static void __exit overclock_exit(void)
{
	remove_proc_entry("overclock/version", NULL);
#ifdef OMAP4
	remove_proc_entry("overclock/gpu_opps", NULL);
#endif
	remove_proc_entry("overclock/mpu_opps", NULL);
	remove_proc_entry("overclock/freq_table", NULL);
	remove_proc_entry("overclock/info", NULL);
	remove_proc_entry("overclock", NULL);

	vfree(buf);
	printk(KERN_INFO "overclock: removed overclocking and unloaded\n");
}


module_init(overclock_init);
module_exit(overclock_exit);
