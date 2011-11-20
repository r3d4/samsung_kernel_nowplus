/*
 * drivers/media/video/isp/omap_resizer.c
 *
 * Driver for Resizer module in TI's OMAP3430 ISP
 *
 * Copyright (C) 2008 Texas Instruments, Inc.
 *
 * Contributors:
 *      Sergio Aguirre <saaguirre@ti.com>
 *      Troy Laramy <t-laramy@ti.com>
 *      Atanas Filipov <afilipov@mm-sol.com>
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <media/v4l2-dev.h>
#include <asm/cacheflush.h>
#include <plat/omap-pm.h>

#include "isp.h"
#include "ispmmu.h"
#include "ispreg.h"
#include "ispresizer.h"

#include <linux/omap_resizer.h>

#define OMAP_RESIZER_NAME		"omap-resizer"

/* Defines and Constants*/
#define MAX_CHANNELS		16
#define MAX_IMAGE_WIDTH		2047
#define MAX_IMAGE_WIDTH_HIGH	2047
#define ALIGNMENT		16
#define PIXEL_EVEN		2
#define RATIO_MULTIPLIER	256
/* Bit position Macro */
/* macro for bit set and clear */
#define BITSET(variable, bit)	((variable) | (1 << bit))
#define BITRESET(variable, bit)	((variable) & ~(0x00000001 << (bit)))
#define SET_BIT_INPUTRAM	28
#define SET_BIT_CBLIN		29
#define SET_BIT_INPTYP		27
#define SET_BIT_YCPOS		26
#define INPUT_RAM		1
#define UP_RSZ_RATIO		64
#define DOWN_RSZ_RATIO		512
#define UP_RSZ_RATIO1		513
#define DOWN_RSZ_RATIO1		1024
#define RSZ_IN_SIZE_VERT_SHIFT	16
#define MAX_HORZ_PIXEL_8BIT	31
#define MAX_HORZ_PIXEL_16BIT	15
#define NUM_PHASES		8
#define NUM_TAPS		4
#define NUM_D2PH		4	/* for downsampling * 2+x ~ 4x,
					 * number of phases
					 */
#define NUM_D2TAPS		7 	/* for downsampling * 2+x ~ 4x,
					 * number of taps
					 */
#define ALIGN32			32
#define MAX_COEF_COUNTER	16
#define COEFF_ADDRESS_OFFSET	0x04
static DECLARE_MUTEX(rsz_hardware_mutex);

enum rsz_config {
	STATE_NOTDEFINED,	/* Resizer driver not configured */
	STATE_CONFIGURED	/* Resizer driver configured. */
};
static int multipass_active;
/* Register mapped structure which contains the every register
   information */
struct resizer_config {
	u32 rsz_pcr;				/* pcr register mapping
						 * variable.
						 */
	u32 rsz_in_start;			/* in_start register mapping
						 * variable.
						 */
	u32 rsz_in_size;			/* in_size register mapping
						 * variable.
						 */
	u32 rsz_out_size;			/* out_size register mapping
						 * variable.
						 */
	u32 rsz_cnt;				/* rsz_cnt register mapping
						 * variable.
						 */
	u32 rsz_sdr_inadd;			/* sdr_inadd register mapping
						 * variable.
						 */
	u32 rsz_sdr_inoff;			/* sdr_inoff register mapping
						 * variable.
						 */
	u32 rsz_sdr_outadd;			/* sdr_outadd register mapping
						 * variable.
						 */
	u32 rsz_sdr_outoff;			/* sdr_outbuff register
						 * mapping variable.
						 */
	u32 rsz_coeff_horz[16];			/* horizontal coefficients
						 * mapping array.
						 */
	u32 rsz_coeff_vert[16];			/* vertical coefficients
						 * mapping array.
						 */
	u32 rsz_yehn;				/* yehn(luma)register mapping
						 * variable.
						 */
};

struct rsz_mult {
	int in_hsize;				/* input frame horizontal
						 * size.
						 */
	int in_vsize;				/* input frame vertical size.
						 */
	int out_hsize;				/* output frame horizontal
						 * size.
						 */
	int out_vsize;				/* output frame vertical
						 * size.
						 */
	int in_pitch;				/* offset between two rows of
						 * input frame.
						 */
	int out_pitch;				/* offset between two rows of
						 * output frame.
						 */
	int end_hsize;
	int end_vsize;
	int num_htap;				/* 0 = 7tap; 1 = 4tap */
	int num_vtap;				/* 0 = 7tap; 1 = 4tap */
	int active;
	int inptyp;
	int vrsz;
	int hrsz;
	int hstph;				/* for specifying horizontal
						 * starting phase.
						 */
	int vstph;
	int pix_fmt;				/* # defined, UYVY or YUYV. */
	int cbilin;				/* # defined, filter with luma
						 * or bi-linear.
						 */
	u16 tap4filt_coeffs[32];		/* horizontal filter
						 * coefficients.
						 */
	u16 tap7filt_coeffs[32];		/* vertical filter
						 * coefficients.
						 */
};



enum rsz_status {
	CHANNEL_FREE,
	CHANNEL_BUSY
};

/* Channel specific structure contains information regarding
   the every channel */
struct channel_config {
	struct resizer_config register_config;	/* Instance of register set
						 * mapping structure
						 */
	int status;				/* Specifies whether the
						 * channel is busy or not
						 */
	struct mutex chanprotection_mutex;
	enum config_done config_state;
	u8 input_buf_index;
	u8 output_buf_index;

};
/* Global structure which contains information about number of channels
   and protection variables */
struct device_params {
	struct rsz_params *params;
	struct channel_config *config;		/* Pointer to channel */
	struct rsz_mult *multipass;		/* Multipass to support */
	unsigned char opened;			/* state of the device */
	struct completion isr_complete;		/* Completion for interrupt */
	struct mutex reszwrap_mutex;		/* Semaphore for array */
	struct videobuf_queue_ops vbq_ops;	/* videobuf queue operations */
	u32 *in_buf_virt_addr[32];
	u32 *out_buf_phy_addr[32];
	rsz_callback 	callback;		/* callback function which gets
						 * called when Resizer
						 * finishes resizing
						 */
	void *callback_arg;
	u32 *out_buf_virt_addr[32];
	u32 num_video_buffers;
	dma_addr_t tmp_buf;
	size_t tmp_buf_size;
	struct rsz_mult original_multipass;
	struct resizer_config original_rsz_conf_chan;
};

static struct device 		*rsz_device;
static struct class		*rsz_class;
static struct device_params	*rsz_params;
static int			 rsz_major = -1;

static struct isp_interface_config rsz_interface = {
	.ccdc_par_ser		= ISP_NONE,
	.dataline_shift		= 0,
	.hsvs_syncdetect	= ISPCTRL_SYNC_DETECT_VSRISE,
	.strobe			= 0,
	.prestrobe		= 0,
	.shutter		= 0,
	.wait_hs_vs		= 0,
};

/*
 * Filehandle data structure
 */
struct rsz_fhdl {
	struct isp_node pipe;
	int src_buff_index;
	u32 rsz_sdr_inadd;	/* Input address */
	int dst_buff_index;
	u32 rsz_sdr_outadd;	/* Output address */
	int rsz_buff_count;	/* used buffers for resizing */

	struct device_params	*device;
	struct device		*isp;
	struct isp_device	*isp_dev;

	struct videobuf_queue	src_vbq;
	spinlock_t		src_vbq_lock; /* spinlock for input queues. */
	struct videobuf_queue	dst_vbq;
	spinlock_t		dst_vbq_lock; /* spinlock for output queues. */

	enum rsz_status status;	/* Channel status: busy or not */
	enum rsz_config config;	/* Configuration state */
};

static struct device *rsz_device;

/*720p changes*/
static struct device *my_isp;
static struct isp_device *my_isp_dev;
static struct isp_res_device *my_isp_res_dev;

/* functions declaration */
static void rsz_hardware_setup(struct channel_config *rsz_conf_chan);
static int rsz_set_params(struct rsz_mult *multipass, struct rsz_params *,
						struct channel_config *);
static int rsz_get_params(struct rsz_params *, struct channel_config *);
static void rsz_copy_data(struct rsz_mult *multipass,
						struct rsz_params *params);
static void rsz_isr(unsigned long status, isp_vbq_callback_ptr arg1,
						void *arg2);
static void rsz_calculate_crop(struct channel_config *rsz_conf_chan,
					struct rsz_cropsize *cropsize);
static int rsz_set_multipass(struct rsz_mult *multipass,
					struct channel_config *rsz_conf_chan);
static int rsz_set_ratio(struct rsz_mult *multipass,
					struct channel_config *rsz_conf_chan);
static void rsz_config_ratio(struct rsz_mult *multipass,
					struct channel_config *rsz_conf_chan);
static void rsz_tmp_buf_free(void);
static u32 rsz_tmp_buf_alloc(size_t size);
static void rsz_save_multipass_context(void);
static void rsz_restore_multipass_context(void);
/*
 * rsz_isp_callback - Interrupt Service Routine for Resizer
 *
 *	@status: ISP IRQ0STATUS register value
 *	@arg1: Currently not used
 *	@arg2: Currently not used
 *
 */
static void rsz_isp_callback(unsigned long status, isp_vbq_callback_ptr arg1,
			     void *arg2)
{
	if ((status & RESZ_DONE) != RESZ_DONE)
		return;
	complete(&rsz_params->isr_complete);
}

static void rsz_hardware_setup(struct channel_config *rsz_conf_chan)
{
	int coeffcounter;
	int coeffoffset = 0;

	down(&rsz_hardware_mutex);
	isp_reg_writel(my_isp,rsz_conf_chan->register_config.rsz_cnt,
			OMAP3_ISP_IOMEM_RESZ, ISPRSZ_CNT);

	isp_reg_writel(my_isp, rsz_conf_chan->register_config.rsz_in_start,
			OMAP3_ISP_IOMEM_RESZ, ISPRSZ_IN_START);
	isp_reg_writel(my_isp, rsz_conf_chan->register_config.rsz_in_size,
			OMAP3_ISP_IOMEM_RESZ, ISPRSZ_IN_SIZE);

	isp_reg_writel(my_isp, rsz_conf_chan->register_config.rsz_out_size,
			OMAP3_ISP_IOMEM_RESZ, ISPRSZ_OUT_SIZE);
	isp_reg_writel(my_isp, rsz_conf_chan->register_config.rsz_sdr_inadd,
			OMAP3_ISP_IOMEM_RESZ, ISPRSZ_SDR_INADD);
	isp_reg_writel(my_isp, rsz_conf_chan->register_config.rsz_sdr_inoff,
			OMAP3_ISP_IOMEM_RESZ, ISPRSZ_SDR_INOFF);
	isp_reg_writel(my_isp, rsz_conf_chan->register_config.rsz_sdr_outadd,
			OMAP3_ISP_IOMEM_RESZ, ISPRSZ_SDR_OUTADD);
	isp_reg_writel(my_isp, rsz_conf_chan->register_config.rsz_sdr_outoff,
			OMAP3_ISP_IOMEM_RESZ, ISPRSZ_SDR_OUTOFF);
	isp_reg_writel(my_isp, rsz_conf_chan->register_config.rsz_yehn,
			OMAP3_ISP_IOMEM_RESZ, ISPRSZ_YENH);

	for (coeffcounter = 0; coeffcounter < MAX_COEF_COUNTER;
							coeffcounter++) {
		isp_reg_writel(my_isp, rsz_conf_chan->register_config.
						rsz_coeff_horz[coeffcounter],
						OMAP3_ISP_IOMEM_RESZ,
						ISPRSZ_HFILT10 + coeffoffset);

		isp_reg_writel(my_isp, rsz_conf_chan->register_config.
						rsz_coeff_vert[coeffcounter],
						OMAP3_ISP_IOMEM_RESZ,
						ISPRSZ_VFILT10 + coeffoffset);
		coeffoffset = coeffoffset + COEFF_ADDRESS_OFFSET;
	}
	up(&rsz_hardware_mutex);
}

static void rsz_save_multipass_context()
{
	struct channel_config *rsz_conf_chan = rsz_params->config;
	struct rsz_mult *multipass = rsz_params->multipass;

	struct resizer_config  *original_rsz_conf_chan
		= &rsz_params->original_rsz_conf_chan;
	struct rsz_mult *original_multipass
		= &rsz_params->original_multipass;

	memset(original_rsz_conf_chan,
		0, sizeof(struct  resizer_config));
	memcpy(original_rsz_conf_chan,
		(struct resizer_config *) &rsz_conf_chan->register_config,
		sizeof(struct  resizer_config));
	memset(original_multipass,
		0, sizeof(struct rsz_mult));
	memcpy(original_multipass,
		multipass, sizeof(struct rsz_mult));

	return;
}

static void rsz_restore_multipass_context()
{
	struct channel_config *rsz_conf_chan = rsz_params->config;
	struct rsz_mult *multipass = rsz_params->multipass;

	struct resizer_config  *original_rsz_conf_chan
		= &rsz_params->original_rsz_conf_chan;
	struct rsz_mult *original_multipass
		= &rsz_params->original_multipass;

	memcpy((struct resizer_config *) &rsz_conf_chan->register_config,
		original_rsz_conf_chan, sizeof(struct  resizer_config));

	memcpy(multipass, original_multipass, sizeof(struct rsz_mult));

	return;
}


/**
 * rsz_copy_data - Copy data
 * @params: Structure containing the Resizer Wrapper parameters
 *
 * Copy data
 **/
static void rsz_copy_data(struct rsz_mult *multipass, struct rsz_params *params)
{
	int i;
	multipass->in_hsize = params->in_hsize;
	multipass->in_vsize = params->in_vsize;
	multipass->out_hsize = params->out_hsize;
	multipass->out_vsize = params->out_vsize;
	multipass->end_hsize = params->out_hsize;
	multipass->end_vsize = params->out_vsize;
	multipass->in_pitch = params->in_pitch;
	multipass->out_pitch = params->out_pitch;
	multipass->hstph = params->hstph;
	multipass->vstph = params->vstph;
	multipass->inptyp = params->inptyp;
	multipass->pix_fmt = params->pix_fmt;
	multipass->cbilin = params->cbilin;

	for (i = 0; i < 32; i++) {
		multipass->tap4filt_coeffs[i] = params->tap4filt_coeffs[i];
		multipass->tap7filt_coeffs[i] = params->tap7filt_coeffs[i];
	}
}





/*
 * rsz_ioc_run_engine - Enables Resizer
 *
 *	@fhdl: Structure containing ISP resizer global information
 *
 *	Submits a resizing task specified by the structure.
 *	The call can either be blocked until the task is completed
 *	or returned immediately based on the value of the blocking argument.
 *	If it is blocking, the status of the task can be checked.
 *
 *	Returns 0 if successful, or -EINVAL otherwise.
 */
int rsz_ioc_run_engine(struct rsz_fhdl *fhdl)
{
	int ret;
	struct videobuf_queue *sq = &fhdl->src_vbq;
	struct videobuf_queue *dq = &fhdl->dst_vbq;
	struct isp_freq_devider *fdiv;

	if (fhdl->config != STATE_CONFIGURED) {
		dev_err(rsz_device, "State not configured \n");
		return -EINVAL;
	}

	fhdl->status = CHANNEL_BUSY;

	down(&rsz_hardware_mutex);

	if (ispresizer_s_pipeline(&fhdl->isp_dev->isp_res, &fhdl->pipe) != 0)
		return -EINVAL;
	if (ispresizer_set_inaddr(&fhdl->isp_dev->isp_res,
				  fhdl->rsz_sdr_inadd, &fhdl->pipe) != 0)
		return -EINVAL;
	if (ispresizer_set_outaddr(&fhdl->isp_dev->isp_res,
				   fhdl->rsz_sdr_outadd) != 0)
		return -EINVAL;

	/* Reduces memory bandwidth */
	fdiv = isp_get_upscale_ratio(fhdl->pipe.in.image.width,
				     fhdl->pipe.in.image.height,
				     fhdl->pipe.out.image.width,
				     fhdl->pipe.out.image.height);
	dev_dbg(rsz_device, "Set the REQ_EXP register = %d.\n",
		fdiv->resz_exp);
	isp_reg_and_or(fhdl->isp, OMAP3_ISP_IOMEM_SBL, ISPSBL_SDR_REQ_EXP,
		       ~ISPSBL_SDR_REQ_RSZ_EXP_MASK,
		       fdiv->resz_exp << ISPSBL_SDR_REQ_RSZ_EXP_SHIFT);

	init_completion(&rsz_params->isr_complete);

	if (isp_configure_interface(fhdl->isp, &rsz_interface) != 0) {
		dev_err(rsz_device, "Can not configure interface\n");
		return -EINVAL;
	}

	if (isp_set_callback(fhdl->isp, CBK_RESZ_DONE, rsz_isp_callback,
			     (void *) NULL, (void *)NULL)) {
		dev_err(rsz_device, "Can not set callback for resizer\n");
		return -EINVAL;
	}
	/*
	 * Through-put requirement:
	 * Set max OCP freq for 3630 is 200 MHz through-put
	 * is in KByte/s so 200000 KHz * 4 = 800000 KByte/s
	 */
	omap_pm_set_min_bus_tput(fhdl->isp, OCP_INITIATOR_AGENT, 800000);
	isp_start(fhdl->isp);
	ispresizer_enable(&fhdl->isp_dev->isp_res, 1);

	ret = wait_for_completion_interruptible_timeout(
			&rsz_params->isr_complete, msecs_to_jiffies(1000));
	if (ret == 0)
		dev_crit(rsz_device, "\nTimeout exit from"
				     " wait_for_completion\n");

	up(&rsz_hardware_mutex);

	fhdl->status = CHANNEL_FREE;
	fhdl->config = STATE_NOTDEFINED;

	sq->bufs[fhdl->src_buff_index]->state = VIDEOBUF_DONE;
	dq->bufs[fhdl->dst_buff_index]->state = VIDEOBUF_DONE;

	if (fhdl->rsz_sdr_inadd) {
		ispmmu_vunmap(fhdl->isp, fhdl->rsz_sdr_inadd);
		fhdl->rsz_sdr_inadd = 0;
	}

	if (fhdl->rsz_sdr_outadd) {
		ispmmu_vunmap(fhdl->isp, fhdl->rsz_sdr_outadd);
		fhdl->rsz_sdr_outadd = 0;
	}

	/* Unmap and free the memory allocated for buffers */
	if (sq->bufs[fhdl->src_buff_index] != NULL) {
		videobuf_dma_unmap(sq, videobuf_to_dma(
				   sq->bufs[fhdl->src_buff_index]));
		videobuf_dma_free(videobuf_to_dma(
				  sq->bufs[fhdl->src_buff_index]));
		dev_dbg(rsz_device, "Unmap source buffer.\n");
	}

	if (dq->bufs[fhdl->dst_buff_index] != NULL) {
		videobuf_dma_unmap(dq, videobuf_to_dma(
				   dq->bufs[fhdl->dst_buff_index]));
		videobuf_dma_free(videobuf_to_dma(
				  dq->bufs[fhdl->dst_buff_index]));
		dev_dbg(rsz_device, "Unmap destination buffer.\n");
	}
	isp_unset_callback(fhdl->isp, CBK_RESZ_DONE);

	/* Reset Through-put requirement */
	omap_pm_set_min_bus_tput(fhdl->isp, OCP_INITIATOR_AGENT, 0);

	/* This will flushes the queue */
	if (&fhdl->src_vbq)
		videobuf_queue_cancel(&fhdl->src_vbq);
	if (&fhdl->dst_vbq)
		videobuf_queue_cancel(&fhdl->dst_vbq);

	return 0;
}

/**
 * rsz_set_multipass - Set resizer multipass
 * @rsz_conf_chan: Structure containing channel configuration
 *
 * Returns always 0
 **/
static int rsz_set_multipass(struct rsz_mult *multipass,
			struct channel_config *rsz_conf_chan)
{
	multipass->in_hsize = multipass->out_hsize;
	multipass->in_vsize = multipass->out_vsize;
	multipass->out_hsize = multipass->end_hsize;
	multipass->out_vsize = multipass->end_vsize;

	multipass->out_pitch = (multipass->inptyp ? multipass->out_hsize
						: (multipass->out_hsize * 2));
	multipass->in_pitch = (multipass->inptyp ? multipass->in_hsize
						: (multipass->in_hsize * 2));
	rsz_conf_chan->register_config.rsz_sdr_inadd =rsz_conf_chan->register_config.rsz_sdr_outadd;
	rsz_set_ratio(multipass, rsz_conf_chan);
	rsz_config_ratio(multipass, rsz_conf_chan);
	rsz_hardware_setup(rsz_conf_chan);
	return 0;
}



/**
 * rsz_set_params - Set parameters for resizer wrapper
 * @params: Structure containing the Resizer Wrapper parameters
 * @rsz_conf_chan: Structure containing channel configuration
 *
 * Used to set the parameters of the Resizer hardware, including input and
 * output image size, horizontal and vertical poly-phase filter coefficients,
 * luma enchancement filter coefficients, etc.
 **/
static int rsz_set_params(struct rsz_mult *multipass, struct rsz_params *params,
					struct channel_config *rsz_conf_chan)
{
	int mul = 1;
	if ((params->yenh_params.type < 0) || (params->yenh_params.type > 2)) {
		dev_err(rsz_device, "rsz_set_params: Wrong yenh type\n");
		return -EINVAL;
	}
	if ((params->in_vsize <= 0) || (params->in_hsize <= 0) ||
			(params->out_vsize <= 0) || (params->out_hsize <= 0) ||
			(params->in_pitch <= 0) || (params->out_pitch <= 0)) {
		dev_err(rsz_device, "rsz_set_params: Invalid size params\n");
		return -EINVAL;
	}
	if ((params->inptyp != RSZ_INTYPE_YCBCR422_16BIT) &&
			(params->inptyp != RSZ_INTYPE_PLANAR_8BIT)) {
		dev_err(rsz_device, "rsz_set_params: Invalid input type\n");
		return -EINVAL;
	}
	if ((params->pix_fmt != RSZ_PIX_FMT_UYVY) &&
			(params->pix_fmt != RSZ_PIX_FMT_YUYV)) {
		dev_err(rsz_device, "rsz_set_params: Invalid pixel format\n");
		return -EINVAL;
	}
	if (params->inptyp == RSZ_INTYPE_YCBCR422_16BIT)
		mul = 2;
	else
		mul = 1;
	if (params->in_pitch < (params->in_hsize * mul)) {
		dev_err(rsz_device, "rsz_set_params: Pitch is incorrect\n");
		return -EINVAL;
	}
	if (params->out_pitch < (params->out_hsize * mul)) {
		dev_err(rsz_device, "rsz_set_params: Out pitch cannot be less"
					" than out hsize\n");
		return -EINVAL;
	}
	/* Output H size should be even */
	if ((params->out_hsize % PIXEL_EVEN) != 0) {
		dev_err(rsz_device, "rsz_set_params: Output H size should"
					" be even\n");
		return -EINVAL;
	}
	if (params->horz_starting_pixel < 0) {
		dev_err(rsz_device, "rsz_set_params: Horz start pixel cannot"
					" be less than zero\n");
		return -EINVAL;
	}

	rsz_copy_data(multipass, params);
	if (0 != rsz_set_ratio(multipass, rsz_conf_chan))
		goto err_einval;

	if (params->yenh_params.type) {
		if ((multipass->num_htap && multipass->out_hsize >
				1280) ||
				(!multipass->num_htap && multipass->out_hsize >
				640))
			goto err_einval;
	}

	if (INPUT_RAM)
		params->vert_starting_pixel = 0;

	rsz_conf_chan->register_config.rsz_in_start =
						(params->vert_starting_pixel
						<< ISPRSZ_IN_SIZE_VERT_SHIFT)
						& ISPRSZ_IN_SIZE_VERT_MASK;

	if (params->inptyp == RSZ_INTYPE_PLANAR_8BIT) {
		if (params->horz_starting_pixel > MAX_HORZ_PIXEL_8BIT)
			goto err_einval;
	}
	if (params->inptyp == RSZ_INTYPE_YCBCR422_16BIT) {
		if (params->horz_starting_pixel > MAX_HORZ_PIXEL_16BIT)
			goto err_einval;
	}

	rsz_conf_chan->register_config.rsz_in_start |=
						params->horz_starting_pixel
						& ISPRSZ_IN_START_HORZ_ST_MASK;

	rsz_conf_chan->register_config.rsz_yehn =
						(params->yenh_params.type
						<< ISPRSZ_YENH_ALGO_SHIFT)
						& ISPRSZ_YENH_ALGO_MASK;

	if (params->yenh_params.type) {
		rsz_conf_chan->register_config.rsz_yehn |=
						params->yenh_params.core
						& ISPRSZ_YENH_CORE_MASK;

		rsz_conf_chan->register_config.rsz_yehn |=
						(params->yenh_params.gain
						<< ISPRSZ_YENH_GAIN_SHIFT)
						& ISPRSZ_YENH_GAIN_MASK;

		rsz_conf_chan->register_config.rsz_yehn |=
						(params->yenh_params.slop
						<< ISPRSZ_YENH_SLOP_SHIFT)
						& ISPRSZ_YENH_SLOP_MASK;
	}

	rsz_config_ratio(multipass, rsz_conf_chan);

	rsz_conf_chan->config_state = STATE_CONFIGURED;

	return 0;
err_einval:
	return -EINVAL;
}



/*
 * rsz_ioc_set_params - Set parameters for resizer
 *
 *	@dst: Target registers map structure.
 *	@src: Structure containing all configuration parameters.
 *
 *	Returns 0 if successful, -1 otherwise.
 */
static int rsz_ioc_set_params(struct rsz_fhdl *fhdl)
{
	/* the source always is memory */
	if (fhdl->pipe.in.image.pixelformat == V4L2_PIX_FMT_YUYV ||
	    fhdl->pipe.in.image.pixelformat == V4L2_PIX_FMT_UYVY) {
		fhdl->pipe.in.path = RSZ_MEM_YUV;
	} else {
		fhdl->pipe.in.path = RSZ_MEM_COL8;
	}

	if (ispresizer_try_pipeline(&fhdl->isp_dev->isp_res, &fhdl->pipe) != 0)
		return -EINVAL;

	/* Ready to set hardware and start operation */
	fhdl->config = STATE_CONFIGURED;

	dev_dbg(rsz_device, "Setup parameters.\n");

	return 0;
}

/**
 * rsz_set_ratio - Set ratio
 * @rsz_conf_chan: Structure containing channel configuration
 *
 * Returns 0 if successful, -EINVAL if invalid output size, upscaling ratio is
 * being requested, or other ratio configuration value is out of bounds
 **/
static int rsz_set_ratio(struct rsz_mult *multipass,
				struct channel_config *rsz_conf_chan)
{
	int alignment = 0;

	rsz_conf_chan->register_config.rsz_cnt = 0;

	if ((multipass->out_hsize > MAX_IMAGE_WIDTH) ||
			(multipass->out_vsize > MAX_IMAGE_WIDTH)) {
		dev_err(rsz_device, "Invalid output size!");
		goto err_einval;
	}
	if (multipass->cbilin) {
		rsz_conf_chan->register_config.rsz_cnt =
				BITSET(rsz_conf_chan->register_config.rsz_cnt,
				SET_BIT_CBLIN);
	}
	if (INPUT_RAM) {
		rsz_conf_chan->register_config.rsz_cnt =
				BITSET(rsz_conf_chan->register_config.rsz_cnt,
				SET_BIT_INPUTRAM);
	}
	if (multipass->inptyp == RSZ_INTYPE_PLANAR_8BIT) {
		rsz_conf_chan->register_config.rsz_cnt =
				BITSET(rsz_conf_chan->register_config.rsz_cnt,
				SET_BIT_INPTYP);
	} else {
		rsz_conf_chan->register_config.rsz_cnt =
				BITRESET(rsz_conf_chan->register_config.
				rsz_cnt, SET_BIT_INPTYP);

		if (multipass->pix_fmt == RSZ_PIX_FMT_UYVY) {
			rsz_conf_chan->register_config.rsz_cnt =
				BITRESET(rsz_conf_chan->register_config.
				rsz_cnt, SET_BIT_YCPOS);
		} else if (multipass->pix_fmt == RSZ_PIX_FMT_YUYV) {
			rsz_conf_chan->register_config.rsz_cnt =
					BITSET(rsz_conf_chan->register_config.
					rsz_cnt, SET_BIT_YCPOS);
		}

	}
	multipass->vrsz =
		(multipass->in_vsize * RATIO_MULTIPLIER) / multipass->out_vsize;
	multipass->hrsz =
		(multipass->in_hsize * RATIO_MULTIPLIER) / multipass->out_hsize;
	if (UP_RSZ_RATIO > multipass->vrsz || UP_RSZ_RATIO > multipass->hrsz) {
		dev_err(rsz_device, "Upscaling ratio not supported!");
		goto err_einval;
	}
	multipass->vrsz = (multipass->in_vsize - NUM_D2TAPS) * RATIO_MULTIPLIER
						/ (multipass->out_vsize - 1);
	multipass->hrsz = ((multipass->in_hsize - NUM_D2TAPS)
						* RATIO_MULTIPLIER) /
						(multipass->out_hsize - 1);

	if (UP_RSZ_RATIO > multipass->vrsz || UP_RSZ_RATIO > multipass->hrsz) {
		dev_err(rsz_device, "Upscaling ratio not supported!");
		goto err_einval;
	}

	if (multipass->hrsz <= 512) {
		multipass->hrsz = (multipass->in_hsize - NUM_TAPS)
						* RATIO_MULTIPLIER
						/ (multipass->out_hsize - 1);
		if (multipass->hrsz < 64)
			multipass->hrsz = 64;
		if (multipass->hrsz > 512)
			multipass->hrsz = 512;
		if (multipass->hstph > NUM_PHASES)
			goto err_einval;
		multipass->num_htap = 1;
	} else if (multipass->hrsz >= 513 && multipass->hrsz <= 1024) {
		if (multipass->hstph > NUM_D2PH)
			goto err_einval;
		multipass->num_htap = 0;
	}

	if (multipass->vrsz <= 512) {
		multipass->vrsz = (multipass->in_vsize - NUM_TAPS)
						* RATIO_MULTIPLIER
						/ (multipass->out_vsize - 1);
		if (multipass->vrsz < 64)
			multipass->vrsz = 64;
		if (multipass->vrsz > 512)
			multipass->vrsz = 512;
		if (multipass->vstph > NUM_PHASES)
			goto err_einval;
		multipass->num_vtap = 1;
	} else if (multipass->vrsz >= 513 && multipass->vrsz <= 1024) {
		if (multipass->vstph > NUM_D2PH)
			goto err_einval;
		multipass->num_vtap = 0;
	}

	if ((multipass->in_pitch) % ALIGN32) {
		dev_err(rsz_device, "Invalid input pitch: %d \n",
							multipass->in_pitch);
		goto err_einval;
	}
	if ((multipass->out_pitch) % ALIGN32) {
		dev_err(rsz_device, "Invalid output pitch %d \n",
							multipass->out_pitch);
		goto err_einval;
	}

	if (multipass->vrsz < 256 &&
			(multipass->in_vsize < multipass->out_vsize)) {
		if (multipass->inptyp == RSZ_INTYPE_PLANAR_8BIT)
			alignment = ALIGNMENT;
		else if (multipass->inptyp == RSZ_INTYPE_YCBCR422_16BIT)
			alignment = (ALIGNMENT / 2);
		else
			dev_err(rsz_device, "Invalid input type\n");

		if (!(((multipass->out_hsize % PIXEL_EVEN) == 0)
				&& (multipass->out_hsize % alignment) == 0)) {
			dev_err(rsz_device, "wrong hsize\n");
			goto err_einval;
		}
	}
	if (multipass->hrsz >= 64 && multipass->hrsz <= 1024) {
		if (multipass->out_hsize > MAX_IMAGE_WIDTH) {
			dev_err(rsz_device, "wrong width\n");
			goto err_einval;
		}
		multipass->active = 0;

	} else if (multipass->hrsz > 1024) {
		if (multipass->out_hsize > MAX_IMAGE_WIDTH) {
			dev_err(rsz_device, "wrong width\n");
			goto err_einval;
		}
		if (multipass->hstph > NUM_D2PH)
			goto err_einval;
		multipass->num_htap = 0;
		multipass->out_hsize = multipass->in_hsize * 256 / 1024;
		if (multipass->out_hsize % ALIGN32) {
			multipass->out_hsize +=
				abs((multipass->out_hsize % ALIGN32) - ALIGN32);
		}
		multipass->out_pitch = ((multipass->inptyp) ?
						multipass->out_hsize :
						(multipass->out_hsize * 2));
		multipass->hrsz = ((multipass->in_hsize - NUM_D2TAPS)
						* RATIO_MULTIPLIER)
						/ (multipass->out_hsize - 1);
		multipass->active = 1;

	}

	if (multipass->vrsz > 1024) {
		if (multipass->out_vsize > MAX_IMAGE_WIDTH_HIGH) {
			dev_err(rsz_device, "wrong width\n");
			goto err_einval;
		}

		multipass->out_vsize = multipass->in_vsize * 256 / 1024;
		multipass->vrsz = ((multipass->in_vsize - NUM_D2TAPS)
						* RATIO_MULTIPLIER)
						/ (multipass->out_vsize - 1);
		multipass->active = 1;
		multipass->num_vtap = 0;

	}
	rsz_conf_chan->register_config.rsz_out_size =
						multipass->out_hsize
						& ISPRSZ_OUT_SIZE_HORZ_MASK;

	rsz_conf_chan->register_config.rsz_out_size |=
						(multipass->out_vsize
						<< ISPRSZ_OUT_SIZE_VERT_SHIFT)
						& ISPRSZ_OUT_SIZE_VERT_MASK;

	rsz_conf_chan->register_config.rsz_sdr_inoff =
						multipass->in_pitch
						& ISPRSZ_SDR_INOFF_OFFSET_MASK;

	rsz_conf_chan->register_config.rsz_sdr_outoff =
					multipass->out_pitch
					& ISPRSZ_SDR_OUTOFF_OFFSET_MASK;

	if (multipass->hrsz >= 64 && multipass->hrsz <= 512) {
		if (multipass->hstph > NUM_PHASES)
			goto err_einval;
	} else if (multipass->hrsz >= 64 && multipass->hrsz <= 512) {
		if (multipass->hstph > NUM_D2PH)
			goto err_einval;
	}

	rsz_conf_chan->register_config.rsz_cnt |=
						(multipass->hstph
						<< ISPRSZ_CNT_HSTPH_SHIFT)
						& ISPRSZ_CNT_HSTPH_MASK;

	if (multipass->vrsz >= 64 && multipass->hrsz <= 512) {
		if (multipass->vstph > NUM_PHASES)
			goto err_einval;
	} else if (multipass->vrsz >= 64 && multipass->vrsz <= 512) {
		if (multipass->vstph > NUM_D2PH)
			goto err_einval;
	}

	rsz_conf_chan->register_config.rsz_cnt |=
						(multipass->vstph
						<< ISPRSZ_CNT_VSTPH_SHIFT)
						& ISPRSZ_CNT_VSTPH_MASK;

	rsz_conf_chan->register_config.rsz_cnt |=
						(multipass->hrsz - 1)
						& ISPRSZ_CNT_HRSZ_MASK;

	rsz_conf_chan->register_config.rsz_cnt |=
						((multipass->vrsz - 1)
						<< ISPRSZ_CNT_VRSZ_SHIFT)
						& ISPRSZ_CNT_VRSZ_MASK;

	return 0;
err_einval:
	return -EINVAL;
}


/*
 * rsz_ioc_get_params - Gets the parameter values
 *
 *	@src: Structure containing all configuration parameters.
 *
 *	Returns 0 if successful, -1 otherwise.
 */
static int rsz_ioc_get_params(const struct rsz_fhdl *fhdl)
{
	if (fhdl->config != STATE_CONFIGURED) {
		dev_err(rsz_device, "state not configured\n");
		return -EINVAL;
	}
	return 0;
}

/**
 * rsz_config_ratio - Configure ratio
 * @rsz_conf_chan: Structure containing channel configuration
 *
 * Configure ratio
 **/
static void rsz_config_ratio(struct rsz_mult *multipass,
				struct channel_config *rsz_conf_chan)
{
	int hsize;
	int vsize;
	int coeffcounter;

	if (multipass->hrsz <= 512) {
		hsize = ((32 * multipass->hstph + (multipass->out_hsize - 1)
					* multipass->hrsz + 16) >> 8) + 7;
	} else {
		hsize = ((64 * multipass->hstph + (multipass->out_hsize - 1)
					* multipass->hrsz + 32) >> 8) + 7;
	}
	if (multipass->vrsz <= 512) {
		vsize = ((32 * multipass->vstph + (multipass->out_vsize - 1)
					* multipass->vrsz + 16) >> 8) + 4;
	} else {
		vsize = ((64 * multipass->vstph + (multipass->out_vsize - 1)
					* multipass->vrsz + 32) >> 8) + 7;
	}
	rsz_conf_chan->register_config.rsz_in_size = hsize;

	rsz_conf_chan->register_config.rsz_in_size |=
					((vsize << ISPRSZ_IN_SIZE_VERT_SHIFT)
					& ISPRSZ_IN_SIZE_VERT_MASK);

	for (coeffcounter = 0; coeffcounter < MAX_COEF_COUNTER;
							coeffcounter++) {
		if (multipass->num_htap) {
			rsz_conf_chan->register_config.
					rsz_coeff_horz[coeffcounter] =
					(multipass->tap4filt_coeffs[2
					* coeffcounter]
					& ISPRSZ_HFILT_COEF0_MASK);
			rsz_conf_chan->register_config.
					rsz_coeff_horz[coeffcounter] |=
					((multipass->tap4filt_coeffs[2
					* coeffcounter + 1]
					<< ISPRSZ_HFILT_COEF1_SHIFT)
					& ISPRSZ_HFILT10_COEF1_MASK);
		} else {
			rsz_conf_chan->register_config.
					rsz_coeff_horz[coeffcounter] =
					(multipass->tap7filt_coeffs[2
					* coeffcounter]
					& ISPRSZ_HFILT_COEF0_MASK);

			rsz_conf_chan->register_config.
					rsz_coeff_horz[coeffcounter] |=
					((multipass->tap7filt_coeffs[2
					* coeffcounter + 1]
					<< ISPRSZ_HFILT_COEF1_SHIFT)
					& ISPRSZ_HFILT10_COEF1_MASK);
		}

		if (multipass->num_vtap) {
			rsz_conf_chan->register_config.
					rsz_coeff_vert[coeffcounter] =
					(multipass->tap4filt_coeffs[2
					* coeffcounter]
					& ISPRSZ_VFILT_COEF0_MASK);

			rsz_conf_chan->register_config.
					rsz_coeff_vert[coeffcounter] |=
					((multipass->tap4filt_coeffs[2
					* coeffcounter + 1]
					<< ISPRSZ_VFILT_COEF1_SHIFT) &
					ISPRSZ_VFILT10_COEF1_MASK);
		} else {
			rsz_conf_chan->register_config.
					rsz_coeff_vert[coeffcounter] =
					(multipass->tap7filt_coeffs[2
					* coeffcounter]
					& ISPRSZ_VFILT_COEF0_MASK);
			rsz_conf_chan->register_config.
					rsz_coeff_vert[coeffcounter] |=
					((multipass->tap7filt_coeffs[2
					* coeffcounter + 1]
					<< ISPRSZ_VFILT_COEF1_SHIFT)
					& ISPRSZ_VFILT10_COEF1_MASK);
		}
	}
}


/*
 * rsz_vbq_setup - Sets up the videobuffer size and validates count.
 *
 *	@q: Structure containing the videobuffer queue file handle.
 *	@cnt: Number of buffers requested
 *	@size: Size in bytes of the buffer used for result
 *
 *	Returns 0 if successful, -1 otherwise.
 */
static int rsz_vbq_setup(struct videobuf_queue *q, unsigned int *cnt,
			 unsigned int *size)
{
	struct rsz_fhdl *fhdl = q->priv_data;

	if (*cnt <= 0)
		*cnt = 1;
	if (*cnt > VIDEO_MAX_FRAME)
		*cnt = VIDEO_MAX_FRAME;

	if (!fhdl->pipe.in.image.width || !fhdl->pipe.in.image.height ||
	    !fhdl->pipe.out.image.width || !fhdl->pipe.out.image.height) {
		dev_err(rsz_device, "%s: Can't setup buffers "
			"size\n", __func__);
		return -EINVAL;
	}

	if (q->type == V4L2_BUF_TYPE_VIDEO_CAPTURE)
		*size = fhdl->pipe.out.image.bytesperline *
			fhdl->pipe.out.image.height;
	else if (q->type == V4L2_BUF_TYPE_VIDEO_OUTPUT)
		*size = fhdl->pipe.in.image.bytesperline *
			fhdl->pipe.in.image.height;
	else
		return -EINVAL;

	dev_dbg(rsz_device, "Setup video buffer type=0x%X.\n", q->type);

	return 0;
}

/*
 * rsz_vbq_release - Videobuffer queue release
 *
 *	@q: Structure containing the videobuffer queue file handle.
 *	@vb: Structure containing the videobuffer used for resizer processing.
 */
static void rsz_vbq_release(struct videobuf_queue *q,
			    struct videobuf_buffer *vb)
{
	struct rsz_fhdl *fhdl = q->priv_data;

	if (q->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		ispmmu_vunmap(fhdl->isp, fhdl->rsz_sdr_outadd);
		fhdl->rsz_sdr_outadd = 0;
		spin_lock(&fhdl->dst_vbq_lock);
		vb->state = VIDEOBUF_NEEDS_INIT;
		spin_unlock(&fhdl->dst_vbq_lock);
	} else if (q->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		ispmmu_vunmap(fhdl->isp, fhdl->rsz_sdr_inadd);
		fhdl->rsz_sdr_inadd = 0;
		spin_lock(&fhdl->src_vbq_lock);
		vb->state = VIDEOBUF_NEEDS_INIT;
		spin_unlock(&fhdl->src_vbq_lock);
	}

	if (vb->memory != V4L2_MEMORY_MMAP) {
		videobuf_dma_unmap(q, videobuf_to_dma(vb));
		videobuf_dma_free(videobuf_to_dma(vb));
	}

	dev_dbg(rsz_device, "Release video buffer type=0x%X.\n", q->type);
}

/*
 * rsz_vbq_prepare - Videobuffer is prepared and mmapped.
 *
 *	@q: Structure containing the videobuffer queue file handle.
 *	@vb: Structure containing the videobuffer used for resizer processing.
 *	@field: Type of field to set in videobuffer device.
 *
 *	Returns 0 if successful, or negative otherwise.
 */
static int rsz_vbq_prepare(struct videobuf_queue *q, struct videobuf_buffer *vb,
			   enum v4l2_field field)
{
	struct rsz_fhdl *fhdl = q->priv_data;
	struct videobuf_dmabuf *dma  = videobuf_to_dma(vb);
	dma_addr_t isp_addr;
	int err = 0;

	if (!vb->baddr) {
		dev_err(rsz_device, "%s: No user buffer allocated\n", __func__);
		return -EINVAL;
	}

	if (q->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		spin_lock(&fhdl->dst_vbq_lock);
		vb->size = fhdl->pipe.out.image.bytesperline *
			   fhdl->pipe.out.image.height;
		vb->width = fhdl->pipe.out.image.width;
		vb->height = fhdl->pipe.out.image.height;
		vb->field = field;
		spin_unlock(&fhdl->dst_vbq_lock);
	} else if (q->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		spin_lock(&fhdl->src_vbq_lock);
		vb->size = fhdl->pipe.in.image.bytesperline *
			   fhdl->pipe.in.image.height;
		vb->width = fhdl->pipe.in.image.width;
		vb->height = fhdl->pipe.in.image.height;
		vb->field = field;
		spin_unlock(&fhdl->src_vbq_lock);
	} else {
		dev_dbg(rsz_device, "Wrong buffer type=0x%X.\n", q->type);
		return -EINVAL;
	}

	if (vb->state == VIDEOBUF_NEEDS_INIT) {
		dev_dbg(rsz_device, "baddr = %08x\n", (int)vb->baddr);
		err = videobuf_iolock(q, vb, NULL);
		if (!err) {
			isp_addr = ispmmu_vmap(fhdl->isp, dma->sglist,
					       dma->sglen);
			if (!isp_addr) {
				err = -EIO;
			} else {
				if (q->type == V4L2_BUF_TYPE_VIDEO_CAPTURE)
					fhdl->rsz_sdr_outadd = isp_addr;
				else if (q->type == V4L2_BUF_TYPE_VIDEO_OUTPUT)
					fhdl->rsz_sdr_inadd = isp_addr;
				else
					return -EINVAL;
			}
		}
	}

	if (!err) {
		if (q->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
			spin_lock(&fhdl->dst_vbq_lock);
			vb->state = VIDEOBUF_PREPARED;
			spin_unlock(&fhdl->dst_vbq_lock);
		} else if (q->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
			spin_lock(&fhdl->src_vbq_lock);
			vb->state = VIDEOBUF_PREPARED;
			spin_unlock(&fhdl->src_vbq_lock);
		}
	} else {
		rsz_vbq_release(q, vb);
	}

	dev_dbg(rsz_device, "Prepare video buffer type=0x%X.\n", q->type);

	return err;
}

static void rsz_vbq_queue(struct videobuf_queue *q, struct videobuf_buffer *vb)
{
	struct rsz_fhdl *fhdl = q->priv_data;

	if (q->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		spin_lock(&fhdl->dst_vbq_lock);
		vb->state = VIDEOBUF_QUEUED;
		spin_unlock(&fhdl->dst_vbq_lock);
	} else if (q->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		spin_lock(&fhdl->src_vbq_lock);
		vb->state = VIDEOBUF_QUEUED;
		spin_unlock(&fhdl->src_vbq_lock);
	}
	dev_dbg(rsz_device, "Queue video buffer type=0x%X.\n", q->type);
}

/*
 * rsz_open - Initializes and opens the Resizer
 *
 *	@inode: Inode structure associated with the Resizer
 *	@filp: File structure associated with the Resizer
 *
 *	Returns 0 if successful,
 *		-EBUSY	if its already opened or the ISP is not available.
 *		-ENOMEM if its unable to allocate the device in kernel memory.
 */
static int rsz_open(struct inode *inode, struct file *fp)
{
	int ret = 0;
	struct rsz_fhdl *fhdl;
	struct device_params *device = rsz_params;

	fhdl = kzalloc(sizeof(struct rsz_fhdl), GFP_KERNEL);
	if (NULL == fhdl)
		return -ENOMEM;

	fhdl->isp = isp_get();
	if (!fhdl->isp) {
		dev_err(rsz_device, "\nCan't get ISP device");
		ret = -EBUSY;
		goto err_isp;
	}
	fhdl->isp_dev = dev_get_drvdata(fhdl->isp);
	device->opened++;

	fhdl->config		= STATE_NOTDEFINED;
	fhdl->status		= CHANNEL_FREE;
	fhdl->src_vbq.type	= V4L2_BUF_TYPE_VIDEO_OUTPUT;
	fhdl->dst_vbq.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fhdl->device		= device;

	fp->private_data = fhdl;
	videobuf_queue_sg_init(&fhdl->src_vbq, &device->vbq_ops, NULL,
			       &fhdl->src_vbq_lock, fhdl->src_vbq.type,
			       V4L2_FIELD_NONE, sizeof(struct videobuf_buffer),
			       fhdl);
	spin_lock_init(&fhdl->src_vbq_lock);

	videobuf_queue_sg_init(&fhdl->dst_vbq, &device->vbq_ops, NULL,
			       &fhdl->dst_vbq_lock, fhdl->dst_vbq.type,
			       V4L2_FIELD_NONE, sizeof(struct videobuf_buffer),
			       fhdl);
	spin_lock_init(&fhdl->dst_vbq_lock);

	return 0;

err_isp:
	isp_put();

	kfree(fhdl);

	return ret;
}

/*
 * rsz_release - Releases Resizer and frees up allocated memory
 *
 *	@inode: Inode structure associated with the Resizer
 *	@filp: File structure associated with the Resizer
 *
 *	Returns 0 if successful, or -EBUSY if channel is being used.
 */
static int rsz_release(struct inode *inode, struct file *filp)
{
	u32 timeout = 0;
	struct rsz_fhdl *fhdl = filp->private_data;

	while ((fhdl->status != CHANNEL_FREE) && (timeout < 20)) {
		timeout++;
		schedule();
	}
	rsz_params->opened--;

	/* This will Free memory allocated to the buffers,
	 * and flushes the queue
	 */
	if (&fhdl->src_vbq)
		videobuf_queue_cancel(&fhdl->src_vbq);
	if (&fhdl->dst_vbq)
		videobuf_queue_cancel(&fhdl->dst_vbq);

	filp->private_data = NULL;

	isp_stop(fhdl->isp);

	isp_put();

	kfree(fhdl);

	return 0;
}

/*
 * rsz_mmap - Memory maps the Resizer module.
 *
 *	@file: File structure associated with the Resizer
 *	@vma: Virtual memory area structure.
 *
 *	Returns 0 if successful, or returned value by the map function.
 */
static int rsz_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct rsz_fhdl *fh = file->private_data;
	int rval = 0;

	rval = videobuf_mmap_mapper(&fh->src_vbq, vma);
	if (rval) {
		dev_dbg(rsz_device, "Memory map source fail!\n");
		return rval;
	}

	rval = videobuf_mmap_mapper(&fh->dst_vbq, vma);
	if (rval) {
		dev_dbg(rsz_device, "Memory map destination fail!\n");
		return rval;
	}

	return 0;
}

/*
 * rsz_ioctl - I/O control function for Resizer
 *
 *	@inode: Inode structure associated with the Resizer.
 *	@file: File structure associated with the Resizer.
 *	@cmd: Type of command to execute.
 *	@arg: Argument to send to requested command.
 *
 *	Returns 0 if successful,
 *		-EBUSY if channel is being used,
 *		-EFAULT if copy_from_user() or copy_to_user() fails.
 *		-EINVAL if parameter validation fails.
 *		-1 if bad command passed or access is denied.
 */
static long rsz_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long ret = 0;
	struct rsz_fhdl *fhdl = file->private_data;
	struct rsz_stat *status;

	if ((_IOC_TYPE(cmd) != RSZ_IOC_BASE) && (_IOC_TYPE(cmd) != 'V')) {
		dev_err(rsz_device, "Bad command value.\n");
		return -EFAULT;
	}

	if ((_IOC_TYPE(cmd) == RSZ_IOC_BASE) &&
	    (_IOC_NR(cmd) > RSZ_IOC_MAXNR)) {
		dev_err(rsz_device, "Command out of range.\n");
		return -EFAULT;
	}

	if (_IOC_DIR(cmd) & _IOC_READ)
		ret = !access_ok(VERIFY_WRITE, (void *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		ret = !access_ok(VERIFY_READ, (void *)arg, _IOC_SIZE(cmd));

	if (ret) {
		dev_err(rsz_device, "Access denied\n");
		return -EFAULT;
	}

	switch (cmd) {

	case RSZ_S_PARAM:
		if (copy_from_user(&fhdl->pipe, (void *)arg,
				   sizeof(fhdl->pipe)))
			return -EIO;
		ret = rsz_ioc_set_params(fhdl);
		if (ret)
			return ret;
		if (copy_to_user((void *)arg, &fhdl->pipe,
				 sizeof(fhdl->pipe)))
			return -EIO;
		break;

	case RSZ_G_PARAM:
		ret = rsz_ioc_get_params(fhdl);
		if (ret)
			return ret;
		if (copy_to_user((void *)arg, &fhdl->pipe,
				 sizeof(fhdl->pipe)))
			return -EIO;
		break;

	case RSZ_G_STATUS:
		status = (struct rsz_stat *)arg;
		status->ch_busy = fhdl->status;
		status->hw_busy = ispresizer_busy(&fhdl->isp_dev->isp_res);
		break;

	case RSZ_RESIZE:
		if (file->f_flags & O_NONBLOCK) {
			if (ispresizer_busy(&fhdl->isp_dev->isp_res))
				return -EBUSY;
		}
		ret = rsz_ioc_run_engine(fhdl);
		if (ret)
			return ret;
		break;

	case RSZ_REQBUF:
	{
		struct v4l2_requestbuffers v4l2_req;
		if (copy_from_user(&v4l2_req, (void *)arg,
				   sizeof(struct v4l2_requestbuffers)))
			return -EIO;
		if (v4l2_req.type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
			if (videobuf_reqbufs(&fhdl->src_vbq, &v4l2_req) < 0)
				return -EINVAL;
		} else if (v4l2_req.type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
			if (videobuf_reqbufs(&fhdl->dst_vbq, &v4l2_req) < 0)
				return -EINVAL;
		} else {
			dev_dbg(rsz_device, "Invalid buffer type.\n");
			return -EINVAL;
		}
		if (copy_to_user((void *)arg, &v4l2_req,
				 sizeof(struct v4l2_requestbuffers)))
			return -EIO;
		break;
	}
	case RSZ_QUERYBUF:
	{
		struct v4l2_buffer v4l2_buf;
		if (copy_from_user(&v4l2_buf, (void *)arg,
				   sizeof(struct v4l2_buffer)))
			return -EIO;
		if (v4l2_buf.type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
			if (videobuf_querybuf(&fhdl->src_vbq, &v4l2_buf) < 0)
				return -EINVAL;
		} else if (v4l2_buf.type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
			if (videobuf_querybuf(&fhdl->dst_vbq, &v4l2_buf) < 0)
				return -EINVAL;
		} else {
			dev_dbg(rsz_device, "Invalid buffer type.\n");
			return -EINVAL;
		}
		if (copy_to_user((void *)arg, &v4l2_buf,
				 sizeof(struct v4l2_buffer)))
			return -EIO;
		break;
	}
	case RSZ_QUEUEBUF:
	{
		struct v4l2_buffer v4l2_buf;
		if (copy_from_user(&v4l2_buf, (void *)arg,
				   sizeof(struct v4l2_buffer)))
			return -EIO;
		if (v4l2_buf.type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
			if (videobuf_qbuf(&fhdl->src_vbq, &v4l2_buf) < 0)
				return -EINVAL;
		} else if (v4l2_buf.type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
			if (videobuf_qbuf(&fhdl->dst_vbq, &v4l2_buf) < 0)
				return -EINVAL;
		} else {
			dev_dbg(rsz_device, "Invalid buffer type.\n");
				return -EINVAL;
		}
		if (copy_to_user((void *)arg, &v4l2_buf,
				 sizeof(struct v4l2_buffer)))
			return -EIO;
		break;
	}
	case VIDIOC_QUERYCAP:
	{
		struct v4l2_capability v4l2_cap;
		if (copy_from_user(&v4l2_cap, (void *)arg,
				   sizeof(struct v4l2_capability)))
			return -EIO;

		strcpy(v4l2_cap.driver, "omap3wrapper");
		strcpy(v4l2_cap.card, "omap3wrapper/resizer");
		v4l2_cap.version	= 1.0;;
		v4l2_cap.capabilities	= V4L2_CAP_VIDEO_CAPTURE |
					  V4L2_CAP_READWRITE;

		if (copy_to_user((void *)arg, &v4l2_cap,
				 sizeof(struct v4l2_capability)))
			return -EIO;
		dev_dbg(rsz_device, "Query cap done.\n");
		break;
	}
	default:
		dev_err(rsz_device, "IOC: Invalid Command Value!\n");
		ret = -EINVAL;
	}
	return ret;
}

static const struct file_operations rsz_fops = {
	.owner		= THIS_MODULE,
	.open		= rsz_open,
	.release	= rsz_release,
	.mmap		= rsz_mmap,
	.unlocked_ioctl	= rsz_ioctl,
};

/*
 * rsz_driver_probe - Checks for device presence
 *
 *	@device: Structure containing details of the current device.
 */
static int rsz_driver_probe(struct platform_device *device)
{
	return 0;
}

/**
 * rsz_get_resource - get the Resizer module from the kernel space.
 * This function will make sure that Resizer module is not used by any other
 * application. It is equivalent of open() call from user space.
 * It returns busy if the device is already opened by other application
 * or ENOMEM if it fails to allocate memory for structures
 * or 0 if the call is successful.
 **/

int rsz_get_resource(void)
{


	struct channel_config *rsz_conf_chan;
	struct device_params *device = rsz_params;
	struct rsz_params *params;
	struct rsz_mult *multipass;

	
	if (!device)
	{
		printk(KERN_EMERG "Device not found\n");
		return -ENOMEM;
	}

	if (device->opened) {
		dev_err(rsz_device, "rsz_get_resource: device is "
						"already opened\n");
		return -EBUSY;
	}

	rsz_conf_chan = kzalloc(sizeof(struct channel_config), GFP_KERNEL);

	if (rsz_conf_chan == NULL) {
		dev_err(rsz_device, "\n Can not allocate memory to config");
		return -ENOMEM;
	}

	params = kzalloc(sizeof(struct rsz_params), GFP_KERNEL);
	if (params == NULL) {
		dev_warn(rsz_device, "\n Can not allocate memory to params");
		if(rsz_conf_chan)kfree(rsz_conf_chan);
		return -ENOMEM;
	}

	multipass = kzalloc(sizeof(struct rsz_mult), GFP_KERNEL);
	if (multipass == NULL) {
		dev_err(rsz_device, "\n cannot allocate memory to multipass");
		if(rsz_conf_chan)kfree(rsz_conf_chan);
		if(params)kfree(params);
		return -ENOMEM;
	}

	device->params = params;
	device->config = rsz_conf_chan;
	device->multipass = multipass;
	device->opened = 1;

	rsz_conf_chan->config_state = STATE_NOT_CONFIGURED;

	init_completion(&device->isr_complete);
	mutex_init(&device->reszwrap_mutex);

 	my_isp = isp_get();
	my_isp_dev = dev_get_drvdata(my_isp);
	my_isp_res_dev = &my_isp_dev->isp_res;


	return 0;
}
EXPORT_SYMBOL(rsz_get_resource);

/*
 * rsz_driver_remove - Handles the removal of the driver
 *
 *	@omap_resizer_device: Structure containing details for device.
 */


/**
 * rsz_put_resource - release all the resource which were acquired during
 * rsz_get_resource() call.
 **/
void rsz_put_resource(void)
{
	struct device_params *device = rsz_params;
	struct channel_config *rsz_conf_chan = device->config;
	struct rsz_params *params = device->params;
	struct rsz_mult *multipass = device->multipass;
	int i = 0;

	if (device->opened != 1)
		return;

	/* unmap output buffers if allocated */
	for (i = 0; i < device->num_video_buffers; ++i) {
		if (device->out_buf_virt_addr[i]) {
			ispmmu_kunmap((dma_addr_t)device->out_buf_virt_addr[i]);
			rsz_params->out_buf_virt_addr[i] = NULL;
		}
		if (rsz_params->in_buf_virt_addr[i]) {
			ispmmu_kunmap((dma_addr_t)device->in_buf_virt_addr[i]);
			rsz_params->in_buf_virt_addr[i] = NULL;
		}
	}

	multipass_active = 0;
	rsz_tmp_buf_free();
	/* free all memory which was allocate during get() */
	if(rsz_conf_chan)kfree(rsz_conf_chan);
	if(params)kfree(params);
	if(multipass)kfree(multipass);

	/* make device available */
	device->opened = 0;
	device->params = NULL;
	device->config = NULL;
	device->multipass = NULL;

	/* release isp resource*/
	isp_put();

	return ;
}
EXPORT_SYMBOL(rsz_put_resource);



/**
  * rsz_configure - configure the Resizer parameters
  * @params: Structure containing the Resizer Wrapper parameters
  * @callback: callback function to call after resizing is over
  * @arg1: argument to callback function
  *
  * This function can be called from DSS to set the parameter of resizer
  * in case there is need to downsize the input image through resizer
  * before calling this function calling driver must already have called
  * rsz_get_resource() function so that necessory initialization is over.
  * Returns 0 if successful,
  **/
int rsz_configure(struct rsz_params *params, rsz_callback callback,
		  u32 num_video_buffers, void *arg1)
{
	int retval;
	struct channel_config *rsz_conf_chan = rsz_params->config;
	struct rsz_mult *multipass = rsz_params->multipass;
	size_t tmp_size;
	multipass_active = 0;
	retval = rsz_set_params(multipass, params, rsz_conf_chan);
	if (retval != 0)
		return retval;

	if (rsz_params->multipass->active) {
		multipass_active = 1;
		tmp_size = PAGE_ALIGN(multipass->out_hsize
				     * multipass->out_vsize
				     * (multipass->inptyp ? 1 : 2));
		rsz_tmp_buf_alloc(tmp_size);
		rsz_conf_chan->register_config.rsz_sdr_outadd =
			(u32)rsz_params->tmp_buf;
		rsz_save_multipass_context();
	}

	rsz_hardware_setup(rsz_conf_chan);
	rsz_params->callback = callback;
	rsz_params->callback_arg = arg1;
	rsz_params->num_video_buffers = num_video_buffers;

	return 0;
}
EXPORT_SYMBOL(rsz_configure);



static u32 rsz_tmp_buf_alloc(size_t size)
{
	rsz_tmp_buf_free();
	printk(KERN_INFO "%s: allocating %d bytes\n", __func__, size);

	rsz_params->tmp_buf = ispmmu_vmalloc(size);
	if (IS_ERR((void *)rsz_params->tmp_buf)) {
		printk(KERN_ERR "ispmmu_vmap mapping failed ");
		return -ENOMEM;
	}
	rsz_params->tmp_buf_size = size;

		return 0;
	/*else {
		dev_err(rsz_device, "Mapping of input buffer in ISP"
				"failed for buffer index %d", slot);
		return -1;
	}*/
}

static void rsz_tmp_buf_free(void)
{
	if (rsz_params->tmp_buf) {
		ispmmu_vfree(rsz_params->tmp_buf);
		rsz_params->tmp_buf = 0;
		rsz_params->tmp_buf_size = 0;
	}
}


/** rsz_begin - Function to be called by DSS when resizing of the input image/
  * buffer is needed
  * @slot: buffer index where the input image is stored
  * @output_buffer_index: output buffer index where output of resizer will
  * be stored
  * @out_off: The line size in bytes for output buffer. as most probably
  * this will be VRFB with YUV422 data, it should come 0x2000 as input
  * @out_phy_add: physical address of the start of output memory area for this
  *
  * rsz_begin()  takes the input buffer index and output buffer index
  * to start the process of resizing. after resizing is complete,
  * the callback function will be called with the argument.
  * Indexes of the input and output buffers are used so that it is faster
  * and easier to configure the input and output address for the ISP resizer.
  * As per the current implementation, DSS uses two VRFB contexts for rotation.
  * The input buffers are already mapped by rsz_map_input_dss_buffers() api, so
  * only index has been taken as the parameter, however for output buffers
  * index and physical address has been taken as argument. if this buffer is
  * not already mapped to ISP address space we use physical address to map it,
  * otherwise only the index is used.
  **/
int rsz_begin(u32 input_buffer_index, int output_buffer_index,
	u32 out_off, u32 out_phy_add, u32 in_phy_add, u32 in_off)
{
	unsigned int output_size;
	struct channel_config *rsz_conf_chan = rsz_params->config;
	int ret;
	if (output_buffer_index >= rsz_params->num_video_buffers) {
		dev_err(rsz_device,
		"ouput buffer index is out of range %d", output_buffer_index);
		return -EINVAL;
	}
//sasken:  enable resizer clock:
    if(ispresizer_request(my_isp_res_dev) < 0) {
        printk("ispresizer_request error\n");
		return -EBUSY;
	}

	if (multipass_active) {
		rsz_restore_multipass_context();
		rsz_hardware_setup(rsz_conf_chan);
	}

	/* If this output buffer has not been mapped till now then map it */
	if (rsz_params->out_buf_virt_addr[output_buffer_index] == NULL) {
		output_size =
			(rsz_conf_chan->register_config.rsz_out_size >>
			ISPRSZ_OUT_SIZE_VERT_SHIFT) * out_off;

		rsz_params->out_buf_virt_addr[output_buffer_index] =
			(u32 *)ispmmu_kmap(out_phy_add, output_size);
		if (!rsz_params->out_buf_virt_addr[output_buffer_index])
			dev_err(rsz_device, "Mapping of output buffer failed"
			"for index %d \n", output_buffer_index);
		}

	if (rsz_params->in_buf_virt_addr[input_buffer_index] == NULL) {
		rsz_params->in_buf_virt_addr[input_buffer_index] =
			(u32 *)ispmmu_kmap(in_phy_add, in_off);
		if (!rsz_params->in_buf_virt_addr[input_buffer_index])
			dev_err(rsz_device, "Mapping of input buffer in ISP"
			"failed for buffer index %d", input_buffer_index);

	}

	/* Configure the input and output address with output line size
	in resizer hardware */
	down(&rsz_hardware_mutex);
	rsz_conf_chan->register_config.rsz_sdr_inadd =
		(u32)rsz_params->in_buf_virt_addr[input_buffer_index];
	isp_reg_writel \
		(my_isp,rsz_conf_chan->register_config.rsz_sdr_inadd,
		OMAP3_ISP_IOMEM_RESZ, ISPRSZ_SDR_INADD);

	if (!multipass_active) {
	rsz_conf_chan->register_config.rsz_sdr_outoff = out_off;
		isp_reg_writel \
			(my_isp,rsz_conf_chan->register_config.rsz_sdr_outoff,
			OMAP3_ISP_IOMEM_RESZ, ISPRSZ_SDR_OUTOFF);

		rsz_conf_chan->register_config.rsz_sdr_outadd =
			(u32)rsz_params->\
			out_buf_virt_addr[output_buffer_index];
		isp_reg_writel \
		(my_isp,rsz_conf_chan->register_config.rsz_sdr_outadd,
			OMAP3_ISP_IOMEM_RESZ, ISPRSZ_SDR_OUTADD);
	}
	up(&rsz_hardware_mutex);

	/* Set ISP callback for the resizing complete even */
	if (isp_set_callback(my_isp,CBK_RESZ_DONE, rsz_isr, (void *) NULL,
							(void *)NULL)) {
		dev_err(rsz_device, "No callback for RSZR\n");
		return -1;
	}

	/* All settings are done.Enable the resizer */
   isp_set_hs_vs(my_isp,0);

mult:
	rsz_params->isr_complete.done = 0;

	/* All settings are done.Enable the resizer */
	ispresizer_enable(my_isp_res_dev,1);

	ret = 0;
	/* Wait for resizing complete event */
	// Added time out based call so that the execution never blocks here. This solves the quick lock-unlock preview freeze problem.
	ret = wait_for_completion_interruptible_timeout(&rsz_params->isr_complete,msecs_to_jiffies(100));      //,msecs_to_jiffies(4) //VIK_DBG RSZ_720
	if (ret == 0)
		{
		printk("Timeout waiting for resizing\n");
		}
	if (rsz_params->multipass->active) {
		multipass_active = 1;
		rsz_set_multipass(rsz_params->multipass, rsz_conf_chan);
		down(&rsz_hardware_mutex);
		if (!rsz_params->multipass->active) {
			rsz_conf_chan->register_config.rsz_sdr_outoff = out_off;
			isp_reg_writel \
				(my_isp,rsz_conf_chan->register_config.rsz_sdr_outoff,
				OMAP3_ISP_IOMEM_RESZ, ISPRSZ_SDR_OUTOFF);

			rsz_conf_chan->register_config.rsz_sdr_outadd =
				(u32)rsz_params->\
				out_buf_virt_addr[output_buffer_index];

			isp_reg_writel \
				(my_isp, rsz_conf_chan->register_config.rsz_sdr_outadd,
				OMAP3_ISP_IOMEM_RESZ, ISPRSZ_SDR_OUTADD);
		}
		up(&rsz_hardware_mutex);
		goto mult;
	}
//sasken : release resizer clock
    if(ispresizer_free(my_isp_res_dev) < 0)
        printk("ispresizer free error \n");

	/* Unset the ISP callback function */
	isp_unset_callback(my_isp,CBK_RESZ_DONE);

	/* Callback function for DSS driver */
	if (rsz_params->callback)
		rsz_params->callback(rsz_params->callback_arg);

	return 0;
}
EXPORT_SYMBOL(rsz_begin);

/* rsz_isr - Interrupt Service Routine for Resizer wrapper
 * @status: ISP IRQ0STATUS register value
 * @arg1: Currently not used
 * @arg2: Currently not used
 *
 * Interrupt Service Routine for Resizer wrapper
 **/
static void rsz_isr(unsigned long status, isp_vbq_callback_ptr arg1, void *arg2)
{

	if ((status & RESZ_DONE) != RESZ_DONE)
		return;

	complete(&(rsz_params->isr_complete));

}



static int rsz_driver_remove(struct platform_device *omap_resizer_device)
{
	return 0;
}

static struct platform_driver omap_resizer_driver = {
	.probe	= rsz_driver_probe,
	.remove	= rsz_driver_remove,
	.driver	= {
		.bus	= &platform_bus_type,
		.name	= OMAP_RESIZER_NAME,
	},
};

/*
 * rsz_device_release - Acts when Reference count is zero
 *
 *	@device: Structure containing ISP resizer global information
 *
 *	This is called when the reference count goes to zero.
 */
static void rsz_device_release(struct device *device)
{
}

static struct platform_device omap_resizer_device = {
	.name	= OMAP_RESIZER_NAME,
	.id	= 2,
	.dev	= {
		.release = rsz_device_release,
	}
};

/*
 * omap_resizer_module_init - Initialization of Resizer
 *
 *	Returns 0 if successful,
 *		-ENOMEM if could not allocate memory.
 *		-ENODEV if could not register the device
 */
static int __init omap_resizer_module_init(void)
{
	int ret = 0;
	struct device_params *device;

	device = kzalloc(sizeof(struct device_params), GFP_KERNEL);
	if (!device) {
		dev_crit(rsz_device, " could not allocate memory\n");
		return -ENOMEM;
	}
	rsz_major = register_chrdev(0, OMAP_RESIZER_NAME, &rsz_fops);

	/* register driver as a platform driver */
	ret = platform_driver_register(&omap_resizer_driver);
	if (ret) {
		dev_crit(rsz_device, "failed to register platform driver!\n");
		goto fail2;
	}

	/* Register the drive as a platform device */
	ret = platform_device_register(&omap_resizer_device);
	if (ret) {
		dev_crit(rsz_device, "failed to register platform device!\n");
		goto fail3;
	}

	rsz_class = class_create(THIS_MODULE, OMAP_RESIZER_NAME);
	if (!rsz_class) {
		dev_crit(rsz_device, "Failed to create class!\n");
		goto fail4;
	}

	/* make entry in the devfs */
	rsz_device = device_create(rsz_class, rsz_device, MKDEV(rsz_major, 0),
				   NULL, OMAP_RESIZER_NAME);

	device->opened = 0;

	device->vbq_ops.buf_setup	= rsz_vbq_setup;
	device->vbq_ops.buf_prepare	= rsz_vbq_prepare;
	device->vbq_ops.buf_release	= rsz_vbq_release;
	device->vbq_ops.buf_queue	= rsz_vbq_queue;

	init_completion(&device->isr_complete);

	rsz_params = device;

	return 0;

fail4:
	platform_device_unregister(&omap_resizer_device);
fail3:
	platform_driver_unregister(&omap_resizer_driver);
fail2:
	unregister_chrdev(rsz_major, OMAP_RESIZER_NAME);

	kfree(device);

	return ret;
}

/*
 * omap_resizer_module_exit - Close of Resizer
 */
void __exit omap_resizer_module_exit(void)
{
	device_destroy(rsz_class, MKDEV(rsz_major, 0));
	class_destroy(rsz_class);
	platform_device_unregister(&omap_resizer_device);
	platform_driver_unregister(&omap_resizer_driver);
	unregister_chrdev(rsz_major, OMAP_RESIZER_NAME);
	kfree(rsz_params);
	rsz_major = -1;
}

module_init(omap_resizer_module_init)
module_exit(omap_resizer_module_exit)

MODULE_AUTHOR("Texas Instruments");
MODULE_DESCRIPTION("OMAP ISP Resizer");
MODULE_LICENSE("GPL");
