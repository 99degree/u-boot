// SPDX-License-Identifier: GPL-2.0+
/*
 * Qualcomm SM6115 memory map
 *
 * Copyright (c) 2023, Linaro Limited
 */

#include <common.h>
#include <asm/armv8/mmu.h>

static struct mm_region qrb2210_mem_map[] = {
	/* Peripheral block */
	{
		.virt = 0x0UL,
		.phys = 0x0UL,
		.size = 0x40000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN |
			 PTE_BLOCK_UXN
	},

	{
		.virt = 0x40000000UL,
		.phys = 0x40000000UL,
		.size = 0x80000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_INNER_SHARE
	},

	/* Terminator */
	{ 0 }
};

struct mm_region *mem_map = qrb2210_mem_map;
