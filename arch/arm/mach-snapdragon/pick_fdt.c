// SPDX-License-Identifier: GPL-2.0+
/*
 * Multi-FDT FIT support for Qualcomm boards
 *
 * Copyright (c) 2024 Linaro Ltd.
 *   Author: Caleb Connolly <caleb.connolly@linaro.org>
 */

#include <asm/armv8/mmu.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <asm/psci.h>
#include <asm/system.h>
#include <dm/device.h>
#include <dm/pinctrl.h>
#include <dm/uclass-internal.h>
#include <dm/read.h>
#include <env.h>
#include <fdt_support.h>
#include <init.h>
#include <linux/arm-smccc.h>
#include <linux/bug.h>
#include <linux/psci.h>
#include <linux/sizes.h>
#include <lmb.h>
#include <malloc.h>
#include <fdt_support.h>
#include <usb.h>
#include <sort.h>

#include "qcom-priv.h"

/* We support booting U-Boot with an internal DT when running as a first-stage bootloader
 * or for supporting quirky devices where it's easier to leave the downstream DT in place
 * to improve ABL compatibility. Otherwise, we use the DT provided by ABL.
 */
void *board_fdt_blob_setup(int *err)
{
	struct pte_smem_detect_state smem_state = { 0 };
	struct fdt_header *fdt;
	bool internal_valid;

	qcom_smem_detect(&smem_state);

	printf("SMEM: 0x%llx - 0x%llx\n", smem_state.start, smem_state.start + smem_state.size);

	*err = 0;
	fdt = (struct fdt_header *)get_prev_bl_fdt_addr();
	internal_valid = !fdt_check_header(gd->fdt_blob);

	/*
	 * There is no point returning an error here, U-Boot can't do anything useful in this situation.
	 * Bail out while we can still print a useful error message.
	 */
	if (!internal_valid && (!fdt || fdt_check_header(fdt) != 0))
		panic("Internal FDT is invalid and no external FDT was provided! (fdt=%#llx)\n", (phys_addr_t)fdt);

	if (internal_valid) {
		debug("Using built in FDT\n");
		return (void *)gd->fdt_blob;
	} else {
		debug("Using external FDT\n");
		return (void *)fdt;
	}
}
