/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2023 Linaro Ltd.
 */

#ifndef _FB_BACKEND_H_
#define _FB_BACKEND_H_

#include <linux/types.h>

struct fastboot_cmd {
	const char *command;
	int command_num;
	void (*dispatch)(char *cmd_parameter, char *response);
};

struct fastboot_flash_backend {
	/* Which device to flash (e.g. MMC 0/1) */
	int flash_device;

	int (*get_part_size)(const char *part_name,
			     size_t *size, char *response);
	/* Optional */
	const char *(*get_part_type)(const char *part_name,
			     char *response);
	void (*flash_write)(const char *part_name,
			   void *buf, u32 count,
			   char *response);
	void (*flash_erase)(const char *part_name,
			   char *response);

	/* Backend specific commands */
	struct fastboot_cmd *cmds;
};

extern const struct fastboot_flash_backend *flash_backend;

#endif /* _FB_BACKEND_H_ */
