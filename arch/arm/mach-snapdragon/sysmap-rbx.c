// SPDX-License-Identifier: GPL-2.0+
/*
 * Qualcomm QCS404 memory map
 *
 * (C) Copyright 2022 Sumit Garg <sumit.garg@linaro.org>
 */

#include <common.h>
#include <dm.h>
#include <asm/armv8/mmu.h>
#include <mach/misc.h>
#include <asm/global_data.h>

DECLARE_GLOBAL_DATA_PTR;

static struct mm_region rbx_mem_map[] = {
	{
		.virt = 0x0UL, /* Peripheral block */
		.phys = 0x0UL, /* Peripheral block */
		.size = 0x80000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		/* First memory bank with defaults */
		.virt = 0x80000000UL, /* DDR */
		.phys = 0x80000000UL, /* DDR */
		.size = 0x200000000UL, /* Up to 8GiB RAM */
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_INNER_SHARE
	},{
		/* Spare for boards with two memory banks */
		0,
	}, {
		/* Spare for boards with three memory banks */
		0,
	},{
		/* List terminator */
		0,
	}
};

struct mm_region *mem_map = rbx_mem_map;

void build_mem_map(void)
{
	int i = 0;

	/* Ensure the peripheral block is sized to cover
	 * the entire address range before the first bank */
	mem_map[0].size = gd->bd->bi_dram[0].start;

	debug("Configured memory map:\n");
	debug("  0x%016llx - 0x%016llx: Peripheral block\n",
	       mem_map[0].phys, mem_map[0].phys + mem_map[0].size);

	/* There is one mm_region for IOMEM, and one list terminator */
	for (i = 0; i < ARRAY_SIZE(rbx_mem_map)-2 && gd->bd->bi_dram[i].size; i++) {
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
	return SZ_32K;
}
