/*
 * io.h
 *
 * DSP-BIOS Bridge driver support functions for TI OMAP processors.
 *
 * The io module manages IO between CHNL and msg_ctrl.
 *
 * Copyright (C) 2005-2006 Texas Instruments, Inc.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef IO_
#define IO_

#include <dspbridge/cfgdefs.h>
#include <dspbridge/devdefs.h>

#include <dspbridge/iodefs.h>

/*
 *  ======== io_create ========
 *  Purpose:
 *      Create an IO manager object, responsible for managing IO between
 *      CHNL and msg_ctrl.
 *  Parameters:
 *      phChnlMgr:              Location to store a channel manager object on
 *                              output.
 *      hdev_obj:             Handle to a device object.
 *      pMgrAttrs:              IO manager attributes.
 *      pMgrAttrs->birq:        I/O IRQ number.
 *      pMgrAttrs->irq_shared:     TRUE if the IRQ is shareable.
 *      pMgrAttrs->word_size:   DSP Word size in equivalent PC bytes..
 *  Returns:
 *      DSP_SOK:                Success;
 *      -ENOMEM:            Insufficient memory for requested resources.
 *      CHNL_E_ISR:             Unable to plug channel ISR for configured IRQ.
 *      CHNL_E_INVALIDIRQ:      Invalid IRQ number. Must be 0 <= birq <= 15.
 *      CHNL_E_INVALIDWORDSIZE: Invalid DSP word size.  Must be > 0.
 *      CHNL_E_INVALIDMEMBASE:  Invalid base address for DSP communications.
 *  Requires:
 *      io_init(void) called.
 *      phIOMgr != NULL.
 *      pMgrAttrs != NULL.
 *  Ensures:
 */
extern dsp_status io_create(OUT struct io_mgr **phIOMgr,
			    struct dev_object *hdev_obj,
			    IN CONST struct io_attrs *pMgrAttrs);

/*
 *  ======== io_destroy ========
 *  Purpose:
 *      Destroy the IO manager.
 *  Parameters:
 *      hio_mgr:         IOmanager object.
 *  Returns:
 *      DSP_SOK:        Success.
 *      -EFAULT:    hio_mgr was invalid.
 *  Requires:
 *      io_init(void) called.
 *  Ensures:
 */
extern dsp_status io_destroy(struct io_mgr *hio_mgr);

/*
 *  ======== io_exit ========
 *  Purpose:
 *      Discontinue usage of the IO module.
 *  Parameters:
 *  Returns:
 *  Requires:
 *      io_init(void) previously called.
 *  Ensures:
 *      Resources, if any acquired in io_init(void), are freed when the last
 *      client of IO calls io_exit(void).
 */
extern void io_exit(void);

/*
 *  ======== io_init ========
 *  Purpose:
 *      Initialize the IO module's private state.
 *  Parameters:
 *  Returns:
 *      TRUE if initialized; FALSE if error occurred.
 *  Requires:
 *  Ensures:
 *      A requirement for each of the other public CHNL functions.
 */
extern bool io_init(void);

/*
 *  ======== io_on_loaded ========
 *  Purpose:
 *      Called when a program is loaded so IO manager can update its
 *      internal state.
 *  Parameters:
 *      hio_mgr:         IOmanager object.
 *  Returns:
 *      DSP_SOK:        Success.
 *      -EFAULT:    hio_mgr was invalid.
 *  Requires:
 *      io_init(void) called.
 *  Ensures:
 */
extern dsp_status io_on_loaded(struct io_mgr *hio_mgr);

#endif /* CHNL_ */
