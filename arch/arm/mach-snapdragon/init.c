// SPDX-License-Identifier: GPL-2.0+
/*
 * Common initialisation for Qualcomm Snapdragon boards.
 *
 * Copyright (c) 2023 Linaro
 * Author: Caleb Connolly <caleb.connolly@linaro.org>
 */

#include <init.h>
#include <env.h>
#include <common.h>
#include <asm/system.h>
#include <asm/gpio.h>
#include <asm/psci.h>
#include <linux/arm-smccc.h>
#include <linux/psci.h>

int dram_init(void)
{
	return fdtdec_setup_mem_size_base();
}

int dram_init_banksize(void)
{
	return fdtdec_setup_memory_banksize();
}

static void show_psci_version(void)
{
	struct arm_smccc_res res;

	arm_smccc_smc(ARM_PSCI_0_2_FN_PSCI_VERSION, 0, 0, 0, 0, 0, 0, 0, &res);

	debug("PSCI:  v%ld.%ld\n",
	       PSCI_VERSION_MAJOR(res.a0),
		PSCI_VERSION_MINOR(res.a0));
}

void reset_cpu(void)
{
	psci_system_reset();
}

__weak int board_init(void)
{
	show_psci_version();
	return 0;
}
