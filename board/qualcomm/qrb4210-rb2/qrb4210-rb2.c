// SPDX-License-Identifier: GPL-2.0+
/*
 * Board init file for QRB4210-RB2
 *
 * (C) Copyright 2023 Bhupesh Sharma <bhupesh.sharma@linaro.org>
 */

#include <common.h>
#include <cpu_func.h>
#include <dm.h>
#include <env.h>
#include <init.h>
#include <asm/cache.h>
#include <asm/gpio.h>
#include <asm/global_data.h>
#include <fdt_support.h>
#include <asm/psci.h>
#include <asm/arch/dram.h>

#include <linux/arm-smccc.h>
#include <linux/psci.h>

DECLARE_GLOBAL_DATA_PTR;

int dram_init(void)
{
	return fdtdec_setup_mem_size_base();
}

static void show_psci_version(void)
{
	struct arm_smccc_res res;

	arm_smccc_smc(ARM_PSCI_0_2_FN_PSCI_VERSION, 0, 0, 0, 0, 0, 0, 0, &res);

	printf("PSCI:  v%ld.%ld\n",
	       PSCI_VERSION_MAJOR(res.a0),
		PSCI_VERSION_MINOR(res.a0));
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
