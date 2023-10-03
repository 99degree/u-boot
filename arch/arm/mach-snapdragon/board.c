// SPDX-License-Identifier: GPL-2.0+
/*
 * Common initialisation for Qualcomm Snapdragon boards.
 *
 * Copyright (c) 2023 Linaro Ltd.
 * Author: Caleb Connolly <caleb.connolly@linaro.org>
 */

#include <asm/armv8/mmu.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <asm/psci.h>
#include <asm/system.h>
#include <dm/device.h>
#include <env.h>
#include <init.h>
#include <linux/arm-smccc.h>
#include <linux/psci.h>
#include <linux/sizes.h>
#include <lmb.h>
#include <malloc.h>

DECLARE_GLOBAL_DATA_PTR;

static struct mm_region rbx_mem_map[CONFIG_NR_DRAM_BANKS + 2] = { { 0 } };

struct mm_region *mem_map = rbx_mem_map;

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

void *board_fdt_blob_setup(int *err)
{
	/* Return DTB pointer passed by ABL */
	*err = 0;
	return (void *)get_prev_bl_fdt_addr();
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

#define KERNEL_COMP_SIZE	SZ_64M

#define addr_alloc(lmb, size) lmb_alloc_base(lmb, size, SZ_2M, max_addr)

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
	status |= env_set_hex("kernel_addr_r", addr_alloc(&lmb, SZ_128M));
	status |= env_set_hex("loadaddr", addr_alloc(&lmb, SZ_64M));
	status |= env_set_hex("fdt_addr_r", addr_alloc(&lmb, SZ_2M));
	status |= env_set_hex("ramdisk_addr_r", addr_alloc(&lmb, SZ_128M));
	status |= env_set_hex("kernel_comp_addr_r", addr_alloc(&lmb, KERNEL_COMP_SIZE));
	status |= env_set_hex("kernel_comp_size", KERNEL_COMP_SIZE);
	status |= env_set_hex("scriptaddr", addr_alloc(&lmb, SZ_4M));
	status |= env_set_hex("pxefile_addr_r", addr_alloc(&lmb, SZ_4M));

	if (status)
		log_warning("%s: Failed to set run time variables\n", __func__);

	return 0;
}

static void build_mem_map(void)
{
	int i;

	/*
	 * Ensure the peripheral block is sized to correctly cover the address range
	 * up to the first memory bank.
	 * Don't map the first page to ensure that we actually trigger an abort on a
	 * null pointer access rather than just hanging.
	 * FIXME: we should probably split this into more precise regions
	 */
	mem_map[0].phys = 0x1000;
	mem_map[0].virt = mem_map[0].phys;
	mem_map[0].size = gd->bd->bi_dram[0].start - mem_map[0].phys;
	mem_map[0].attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN;

	debug("Configured memory map:\n");
	debug("  0x%016llx - 0x%016llx: Peripheral block\n",
	       mem_map[0].phys, mem_map[0].phys + mem_map[0].size);

	/*
	 * Now add memory map entries for each DRAM bank, ensuring we don't
	 * overwrite the list terminator
	 */
	for (i = 0; i < ARRAY_SIZE(rbx_mem_map)-2 && gd->bd->bi_dram[i].size; i++) {
		if (i == ARRAY_SIZE(rbx_mem_map)-1) {
			log_warning("Too many DRAM banks!\n");
			break;
		}
		mem_map[i+1].phys = gd->bd->bi_dram[i].start;
		mem_map[i+1].virt = mem_map[i+1].phys;
		mem_map[i+1].size = gd->bd->bi_dram[i].size;
		mem_map[i+1].attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
				     PTE_BLOCK_INNER_SHARE;

		debug("  0x%016llx - 0x%016llx: DDR bank %d\n",
		      mem_map[i+1].phys, mem_map[i+1].phys + mem_map[i+1].size, i);
	}
}

u64 get_page_table_size(void)
{
	return SZ_64K;
}

void enable_caches(void)
{
	build_mem_map();

	icache_enable();
	dcache_enable();
}
