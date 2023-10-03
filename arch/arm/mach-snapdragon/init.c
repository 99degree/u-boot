// SPDX-License-Identifier: GPL-2.0+
/*
 * Common initialisation for Qualcomm Snapdragon boards.
 *
 * Copyright (c) 2023 Linaro
 * Author: Caleb Connolly <caleb.connolly@linaro.org>
 */

#include <init.h>
#include <dm/device.h>
#include <env.h>
#include <common.h>
#include <asm/system.h>
#include <asm/gpio.h>
#include <asm/psci.h>
#include <linux/arm-smccc.h>
#include <linux/psci.h>
#include <lmb.h>
#include <asm/io.h>
#include <mach/misc.h>

DECLARE_GLOBAL_DATA_PTR;

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

static void set_devicetree(void) {
	int i;
	/* Pairs of compatible string -> DTB filename */
	static const char * const dtb_map[] = {
		"thundercomm,db845c",	"qcom/sdm845-db845c.dtb",
		"qcom,qrb2210-rb1",		"qcom/qrb2210-rb1.dtb",
		"qcom,qrb4210-rb2",		"qcom/qrb4210-rb2.dtb",
		"qcom,qrb5165-rb5",		"qcom/qrb5165-rb5.dtb",
		"shift,axolotl",		"qcom/sdm845-shift-axolotl.dtb",
		NULL,
	};

	for (i = 0; dtb_map[i]; i += 2) {
		if (of_machine_is_compatible(dtb_map[i])) {
			printf("board is %s, dtb path %s\n", dtb_map[i], dtb_map[i + 1]);
			env_set("devicetree", dtb_map[i + 1]);
			return;
		}
	}

	log_warning("%s: unknown board\n", __func__);
}

#define KERNEL_COMP_SIZE	SZ_64M

#define _lmballoc(lmb, size) lmb_alloc_base(lmb, size, SZ_2M, max_addr)

/* Stolen from arch/arm/mach-apple/board.c */
int board_late_init(void)
{
	struct lmb lmb;
	u32 status = 0;
	phys_addr_t max_addr = 0;

	lmb_init_and_reserve(&lmb, gd->bd, (void *)gd->fdt_blob);

	/* If we have more than 1 RAM bank, and the two RAM banks have a hole in the middle
	 * then stop LMB using the second bank
	 */
	if (gd->bd->bi_dram[1].size)
		max_addr = gd->bd->bi_dram[0].start + gd->bd->bi_dram[0].size;

	/* We need to be fairly conservative here as we support boards with just 1G of TOTAL RAM */
	status |= env_set_hex("kernel_addr_r", _lmballoc(&lmb, SZ_128M));
	status |= env_set_hex("loadaddr", _lmballoc(&lmb, SZ_64M));
	status |= env_set_hex("fdt_addr_r", _lmballoc(&lmb, SZ_2M));
	status |= env_set_hex("ramdisk_addr_r", _lmballoc(&lmb, SZ_128M));
	status |= env_set_hex("kernel_comp_addr_r", _lmballoc(&lmb, KERNEL_COMP_SIZE));
	status |= env_set_hex("kernel_comp_size", KERNEL_COMP_SIZE);
	status |= env_set_hex("scriptaddr", _lmballoc(&lmb, SZ_4M));
	status |= env_set_hex("pxefile_addr_r", _lmballoc(&lmb, SZ_4M));

	if (status)
		log_warning("%s: Failed to set run time variables\n", __func__);

	set_devicetree();

	return 0;
}

void enable_caches(void)
{
	build_mem_map();

	icache_enable();
	dcache_enable();
}
