/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Helpers for U-Boot running as an EFI payload.
 *
 * Copyright (c) 2024 Linaro, Ltd
 */


#ifndef _EFI_STUB_H
#define _EFI_STUB_H

#include <linux/types.h>

enum efi_entry_t {
	EFIET_END,	/* Signals this is the last (empty) entry */
	EFIET_MEMORY_MAP,
	EFIET_GOP_MODE,
	EFIET_SYS_TABLE,

	/* Number of entries */
	EFIET_MEMORY_COUNT,
};

/**
 * efi_info_get() - get an entry from an EFI table
 *
 * This function is called from U-Boot proper to read information set up by the
 * EFI stub. It can only be used when running from the EFI stub, not when U-Boot
 * is running as an app.
 *
 * @type:	Entry type to search for
 * @datap:	Returns pointer to entry data
 * @sizep:	Returns entry size
 * Return: 0 if OK, -ENODATA if there is no table, -ENOENT if there is no entry
 * of the requested type, -EPROTONOSUPPORT if the table has the wrong version
 */
int efi_info_get(enum efi_entry_t type, void **datap, int *sizep);

struct device_node;

/**
 * of_populate_from_efi() - Populate the live tree from EFI tables
 *
 * @root: Root node of tree to populate
 *
 * This function populates the live tree with information from EFI tables
 * it is only applicable when running U-Boot as an EFI payload with
 * CONFIG_EFI_STUB enabled.
 */
int of_populate_from_efi(struct device_node *root);

/**
 * dram_init_banksize_from_efi() - Initialize the memory banks from EFI tables
 *
 * This function initializes the memory banks from the EFI memory map table we
 * stashed from the EFI stub. It is only applicable when running U-Boot as an
 * EFI payload with CONFIG_EFI_STUB enabled.
 */
int dram_init_banksize_from_efi(void);

#endif /* _EFI_STUB_H */
