/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c), Vaisala Oyj
 */

#ifndef REBOOT_MODE_REBOOT_MODE_H__
#define REBOOT_MODE_REBOOT_MODE_H__

#include <asm/types.h>
#include <dm/device.h>

struct reboot_mode_mode {
	const char *mode_name;
	u32 mode_id;
};

struct reboot_mode_uclass_platdata {
	struct reboot_mode_mode *modes;
	u8 count;
	const char *env_variable;
};

struct reboot_mode_ops {
	/**
	 * get() - get the current reboot mode value
	 *
	 * Returns the current value from the reboot mode backing store.
	 *
	 * @dev:	Device to read from
	 * @rebootmode:	Address to save the current reboot mode value
	 */
	int (*get)(struct udevice *dev, u32 *rebootmode);

	/**
	 * set() - set a reboot mode value
	 *
	 * Sets the value in the reboot mode backing store.
	 *
	 * @dev:	Device to read from
	 * @rebootmode:	New reboot mode value to store
	 */
	int (*set)(struct udevice *dev, u32 rebootmode);
};

/* Access the operations for a reboot mode device */
#define reboot_mode_get_ops(dev) ((struct reboot_mode_ops *)(dev)->driver->ops)

/**
 * dm_reboot_mode_update() - Update the reboot mode env variable.
 *
 * @dev:	Device to read from
 * Return: 0 if OK, -ve on error
 */
int dm_reboot_mode_update(struct udevice *dev);

/**
 * dm_reboot_mode_lookup() - Resolve a reboot mode name to its numeric ID
 * @dev: Reboot-mode device (UCLASS_REBOOT_MODE)
 * @name: Mode name string (e.g. "bootloader", "recovery")
 * @mode_id: Pointer to store the resolved numeric mode ID
 *
 * This function searches the parsed reboot-mode entries (from DTS `mode-*`
 * properties) and returns the corresponding numeric mode ID for the given
 * string name. It allows board code or command handlers to convert user input
 * or environment variables into a valid reboot mode ID.
 *
 * Returns:
 *   0 if the mode name was found and resolved
 *  -ENOENT if the name was not found
 *  -ENODATA if the mode list is missing or empty
 */
int dm_reboot_mode_lookup(struct udevice *dev, const char *name, u32 *mode_id);

/**
 * dm_reboot_mode_set() - Invoke the reboot-mode driver's set callback
 * @dev: Reboot-mode device (UCLASS_REBOOT_MODE)
 * @mode_id: Numeric reboot mode ID to set
 *
 * This function dispatches the reboot-mode update to the active driver by
 * calling its `.set()` callback. The mode ID should correspond to one of the
 * parsed `mode-*` values from the device tree, as resolved by
 * dm_reboot_mode_lookup().
 *
 * Returns:
 *   0 if the mode was successfully set
 *  -ENOSYS if the driver does not implement a set callback
 *  <0 if the driver-specific set operation fails
 */
int dm_reboot_mode_set(struct udevice *dev, u32 mode_id);

#endif /* REBOOT_MODE_REBOOT_MODE_H__ */
