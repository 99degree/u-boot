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

/* Sets up the "board", "board_name", and "soc" environment variables as well as constructing
 * the devicetree path, with a few quirks to handle non-standard dtb filenames.
 */
static void configure_env(void) {
	const char *compat0, *compat1;
	char *tmp;
	char buf[32] = { 0 };
	char dt_path[64] = { 0 }; /* qcom/<soc>-[vendor]-<board>.dtb */
	int n_compat, ret;
	ofnode root;

	root = ofnode_root();
	n_compat = ofnode_read_string_count(root, "compatible");
	if (n_compat < 2) {
		log_warning("%s: only one root compatible bailing!\n", __func__);
		return;
	}

	ret = ofnode_read_string_index(root, "compatible", 0, &compat0);
	if (ret < 0) {
		log_warning("Can't read first compatible\n");
		return;
	}

	ret = ofnode_read_string_index(root, "compatible", 1, &compat1);
	if (ret < 0) {
		log_warning("Can't read second compatible\n");
		return;
	}

	strncpy(buf, compat1, sizeof(buf) - 1);
	tmp = buf;

	if (!strsep(&tmp, ",")) {
		log_warning("second compatible '%s' has no ','\n", buf);
		return;
	}

	env_set("soc", tmp);

	memset(buf, 0, sizeof(buf));
	strncpy(buf, compat0, sizeof(buf) - 1);
	tmp = buf;

	if (!strncmp("qcom", buf, strlen("qcom"))) {
		if (!strsep(&tmp, "-")) {
			log_warning("compatible '%s' has no '-'\n", buf);
			return;
		}
		env_set("board", tmp);
	} else {
		if (!strsep(&tmp, ",")) {
			log_warning("compatible '%s' has no ','\n", buf);
			return;
		}
		/* for thundercomm we just want the bit after the comma, for all other boards
		 * we replace the comma with a '-' and take both */
		if (!strncmp("thundercomm", buf, strlen("thundercomm"))) {
			env_set("board", tmp);
		} else {
			*(tmp-1) = '-';
			env_set("board", buf);
		}
	}

	/* We use env_get() to avoid more allocations */
	snprintf(dt_path, sizeof(dt_path), "qcom/%s-%s.dtb",
		env_get("soc"), env_get("board"));
	env_set("fdtfile", dt_path);
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
	status |= env_set_hex("kernel_comp_addr_r", _lmballoc(&lmb, KERNEL_COMP_SIZE));
	status |= env_set_hex("kernel_comp_size", KERNEL_COMP_SIZE);
	status |= env_set_hex("loadaddr", _lmballoc(&lmb, SZ_4M));
	status |= env_set_hex("fdt_addr_r", _lmballoc(&lmb, SZ_4M));
	status |= env_set_hex("ramdisk_addr_r", _lmballoc(&lmb, SZ_128M));
	status |= env_set_hex("kernel_addr_r", _lmballoc(&lmb, SZ_128M));
	status |= env_set_hex("scriptaddr", _lmballoc(&lmb, SZ_4M));
	status |= env_set_hex("pxefile_addr_r", _lmballoc(&lmb, SZ_4M));

	if (status)
		log_warning("%s: Failed to set run time variables\n", __func__);

	configure_env();

	return 0;
}

void enable_caches(void)
{
	build_mem_map();

	icache_enable();
	dcache_enable();
}
