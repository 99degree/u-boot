// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2023 Linaro Ltd.
 */

#include <config.h>
#include <blk.h>
#include <usb.h>
#include <fastboot.h>
#include <fastboot-internal.h>
#include <image.h>
#include <part.h>

#include "fb_backend.h"

/* FIXME: write USB backend */

const struct fastboot_flash_backend usb_flash_backend = {
	// .get_part_size = fastboot_usb_get_part_size,
	// .get_part_type = fastboot_usb_get_part_type,
	// .flash_write = fastboot_usb_flash_write,
	// .flash_erase = fastboot_usb_erase,

	.flash_device = CONFIG_FASTBOOT_FLASH_USB_DEV,
};

const struct fastboot_flash_backend *flash_backend = &usb_flash_backend;
