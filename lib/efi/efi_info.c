// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2015 Google, Inc
 *
 * Access to the EFI information table
 */

#include <efi.h>
#include <efi_loader.h>
#include <efi_stub.h>
#include <errno.h>
#include <mapmem.h>
#include <asm/global_data.h>
#include <efi_api.h>
#include <dm/of.h>
#include <dm/ofnode.h>
#include <dm/of_access.h>
#include <log.h>

DECLARE_GLOBAL_DATA_PTR;

int efi_info_get(enum efi_entry_t type, void **datap, int *sizep)
{
	struct efi_entry_hdr *entry;
	struct efi_info_hdr *info;
	int ret;

	if (!gd->arch.table)
		return -ENODATA;

	info = map_sysmem(gd->arch.table, 0);
	if (info->version != EFI_TABLE_VERSION) {
		ret = -EPROTONOSUPPORT;
		goto err;
	}

	entry = (struct efi_entry_hdr *)((ulong)info + info->hdr_size);
	while (entry->type != EFIET_END) {
		if (entry->type == type) {
			if (entry->addr)
				*datap = map_sysmem(entry->addr, entry->size);
			else
				*datap = entry + 1;
			*sizep = entry->size;
			return 0;
		}
		entry = (struct efi_entry_hdr *)((ulong)entry + entry->link);
	}

	ret = -ENOENT;
err:
	unmap_sysmem(info);

	return -ENOSYS;
}

#ifdef CONFIG_OF_LIVE
static int of_populate_framebuffer(struct device_node *root)
{
	struct device_node *chosen, *fb;
	struct efi_entry_gopmode *mode;
	ofnode node;
	int ret, size;
	u64 reg[2];
	char fb_node_name[50] = { 0 };

	ret = efi_info_get(EFIET_GOP_MODE, (void **)&mode, &size);
	if (ret) {
		printf("EFI graphics output entry not found\n");
		return ret;
	}

	fb = of_find_node_opts_by_path(root, "/chosen/framebuffer", NULL);
	/* framebuffer already defined */
	if (fb)
		return 0;

	chosen = of_find_node_opts_by_path(root, "/chosen", NULL);
	if (!chosen) {
		ret = of_add_subnode(root, "chosen", -1, &chosen);
		if (ret) {
			debug("Failed to add chosen node\n");
			return ret;
		}
	}
	node = np_to_ofnode(chosen);
	ofnode_write_u32(node, "#address-cells", 2);
	ofnode_write_u32(node, "#size-cells", 2);
	/*
	 * In order for of_translate_one() to correctly detect an empty ranges property, the value
	 * pointer has to be non-null even though the length is 0.
	 */
	of_write_prop(chosen, "ranges", 0, (void *)FDT_ADDR_T_NONE);

	snprintf(fb_node_name, sizeof(fb_node_name), "framebuffer@%llx", mode->fb_base);
	ret = of_add_subnode(chosen, fb_node_name, -1, &fb);
	if (ret) {
		debug("Failed to add framebuffer node\n");
		return ret;
	}
	node = np_to_ofnode(fb);
	ofnode_write_string(node, "compatible", "simple-framebuffer");
	reg[0] = cpu_to_fdt64(mode->fb_base);
	reg[1] = cpu_to_fdt64(mode->fb_size);
	ofnode_write_prop(node, "reg", reg, sizeof(reg), true);
	ofnode_write_u32(node, "width", mode->info->width);
	ofnode_write_u32(node, "height", mode->info->height);
	ofnode_write_u32(node, "stride", mode->info->pixels_per_scanline * 4);
	ofnode_write_string(node, "format", "a8r8g8b8");

	return 0;
}
#endif

int of_populate_from_efi(struct device_node *root)
{
	int ret = 0;
	if (CONFIG_IS_ENABLED(VIDEO_SIMPLE) && CONFIG_IS_ENABLED(OF_LIVE))
		ret = of_populate_framebuffer(root);

	return ret;
}

static bool efi_mem_type_is_usable(u32 type)
{
	switch (type) {
	case EFI_CONVENTIONAL_MEMORY:
	case EFI_LOADER_DATA:
	case EFI_LOADER_CODE:
	case EFI_BOOT_SERVICES_CODE:
		return true;
	case EFI_RESERVED_MEMORY_TYPE:
	case EFI_UNUSABLE_MEMORY:
	case EFI_UNACCEPTED_MEMORY_TYPE:
	/* TBD: U-Boot doesn't use EFI runtime services yet(?). Maybe we can re-use this */
	case EFI_RUNTIME_SERVICES_DATA:
	case EFI_MMAP_IO:
	case EFI_MMAP_IO_PORT:
	case EFI_PERSISTENT_MEMORY_TYPE:
	default:
		return false;
	}
}

int dram_init_banksize_from_efi(void)
{
	struct efi_mem_desc *desc, *end;
	struct efi_entry_memmap *map;
	int ret, size, bank = 0;
	int num_banks;

	ret = efi_info_get(EFIET_MEMORY_MAP, (void **)&map, &size);
	if (ret) {
		/* We should have stopped in dram_init(), something is wrong */
		debug("%s: Missing memory map\n", __func__);
		return -ENXIO;
	}
	end = (struct efi_mem_desc *)((ulong)map + size);
	desc = map->desc;
	for (num_banks = 0;
	     desc < end && num_banks < CONFIG_NR_DRAM_BANKS;
	     desc = efi_get_next_mem_desc(desc, map->desc_size)) {
		/*
		 * We only use conventional memory and ignore
		 * anything less than 1MB.
		 */
		log_debug("EFI bank #%d: start %llx, size %llx type %u\n",
			bank, desc->physical_start,
			desc->num_pages << EFI_PAGE_SHIFT, desc->type);
		bank++;
		if (!efi_mem_type_is_usable(desc->type) || (desc->num_pages << EFI_PAGE_SHIFT) < 1 << 20)
			continue;
		gd->bd->bi_dram[num_banks].start = desc->physical_start;
		gd->bd->bi_dram[num_banks].size = desc->num_pages <<
			EFI_PAGE_SHIFT;
		num_banks++;
	}

	return 0;
}

/* Called by U-Boot's EFI subsystem to add known memory. In our case
 * we need to add some specific memory types from the original bootloaders
 * EFI memory map
 */
void efi_add_known_memory_from_efi(void)
{
	struct efi_mem_desc *desc, *end;
	struct efi_entry_memmap *map;
	int ret, size;

	EFI_PRINT("Adding known memory from previous stage EFI bootloader\n");

	ret = efi_info_get(EFIET_MEMORY_MAP, (void **)&map, &size);
	if (ret) {
		EFI_PRINT("%s: Missing memory map\n", __func__);
		return;
	}
	end = (struct efi_mem_desc *)((ulong)map + size);

	for (desc = map->desc; desc < end; desc = efi_get_next_mem_desc(desc, map->desc_size)) {
		switch (desc->type) {
		case EFI_RESERVED_MEMORY_TYPE:
			efi_add_memory_map_pg(desc->physical_start, desc->num_pages, desc->type, false);
			break; /* Breaks the switch/case not the for loop */
		default:
			continue;
		}
	}
}
