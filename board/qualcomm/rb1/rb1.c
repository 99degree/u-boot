// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2023, Linaro Limited
 */

#include <common.h>
#include <cpu_func.h>
#include <dm.h>
#include <env.h>
#include <fdt_support.h>
#include <init.h>

#include <asm/arch/dram.h>
#include <asm/cache.h>
#include <asm/global_data.h>
#include <asm/gpio.h>
#include <asm/psci.h>

#include <linux/arm-smccc.h>
#include <linux/psci.h>

DECLARE_GLOBAL_DATA_PTR;

int dram_init(void)
{
	return fdtdec_setup_mem_size_base();
}

int dram_init_banksize(void)
{
	return fdtdec_setup_memory_banksize();
}

extern ulong fw_dtb_pointer;

void *board_fdt_blob_setup(int *err)
{
	*err = 0;
	return (void *)fw_dtb_pointer;
}

static void show_psci_version(void)
{
	struct arm_smccc_res res;

	arm_smccc_smc(ARM_PSCI_0_2_FN_PSCI_VERSION, 0, 0, 0, 0, 0, 0, 0, &res);

	printf("PSCI: v%ld.%ld\n", PSCI_VERSION_MAJOR(res.a0), PSCI_VERSION_MINOR(res.a0));
}

int board_init(void)
{
	show_psci_version();

	return 0;
}

void reset_cpu(void)
{
	psci_system_reset();
}
