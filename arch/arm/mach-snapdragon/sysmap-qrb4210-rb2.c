// SPDX-License-Identifier: GPL-2.0+
/*
 * Qualcomm SM6115 memory map
 *
 * (C) Copyright 2023 Bhupesh Sharma <bhupesh.sharma@linaro.org>
 */

#include <common.h>
#include <asm/armv8/mmu.h>

static struct mm_region sm6115_mem_map[] = {
	{
		.virt = 0x0UL, /* Peripheral block */
		.phys = 0x0UL, /* Peripheral block */
		.size = 0xC100000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		.virt = 0xC100000UL, /* DDR */
		.phys = 0xC100000UL, /* DDR */
		.size = 0x002A000UL,  /* GiB - maximum allowed memory */
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_INNER_SHARE
	}, {
		.virt = 0xC200000UL, /* DDR */
		.phys = 0xC200000UL, /* DDR */
		.size = 0x017FFFFUL,  /* GiB - maximum allowed memory */
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_INNER_SHARE

	}, {
		.virt = 0xE000000UL, /* DDR */
		.phys = 0xE000000UL, /* DDR */
		.size = 0x1000000UL, /* GiB - maximum allowed memory */
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_INNER_SHARE
	}, {
		.virt = 0x10000000UL, /* DDR */
		.phys = 0x10000000UL, /* DDR */
		.size = 0x04000000UL, /* GiB - maximum allowed memory */
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_INNER_SHARE
	}, {
		.virt = 0x18000000UL, /* DDR */
		.phys = 0x18000000UL, /* DDR */
		.size = 0x200000000UL, /* GiB - maximum allowed memory */
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_INNER_SHARE

	}, {
		/* List terminator */
		0,
	}
};

struct mm_region *mem_map = sm6115_mem_map;
