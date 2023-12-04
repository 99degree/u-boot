// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (C) 2016 The Android Open Source Project
 */

#include <common.h>
#include <env.h>
#include <fastboot.h>
#include <fastboot-internal.h>
#include <fb_mmc.h>
#include <fb_nand.h>
#include <fs.h>
#include <part.h>
#include <version.h>
#include <linux/printk.h>

#include "fb_backend.h"

const struct fastboot_flash_backend *flash_backend;

static void getvar_version(char *var_parameter, char *response);
static void getvar_version_bootloader(char *var_parameter, char *response);
static void getvar_downloadsize(char *var_parameter, char *response);
static void getvar_serialno(char *var_parameter, char *response);
static void getvar_version_baseband(char *var_parameter, char *response);
static void getvar_product(char *var_parameter, char *response);
static void getvar_platform(char *var_parameter, char *response);
static void getvar_current_slot(char *var_parameter, char *response);
static void getvar_has_slot(char *var_parameter, char *response);
static void getvar_partition_type(char *part_name, char *response);
static void getvar_partition_size(char *part_name, char *response);
static void getvar_is_userspace(char *var_parameter, char *response);

static const struct {
	const char *variable;
	void (*dispatch)(char *var_parameter, char *response);
} getvar_dispatch[] = {
	{
		.variable = "version",
		.dispatch = getvar_version
	}, {
		.variable = "version-bootloader",
		.dispatch = getvar_version_bootloader
	}, {
		.variable = "downloadsize",
		.dispatch = getvar_downloadsize
	}, {
		.variable = "max-download-size",
		.dispatch = getvar_downloadsize
	}, {
		.variable = "serialno",
		.dispatch = getvar_serialno
	}, {
		.variable = "version-baseband",
		.dispatch = getvar_version_baseband
	}, {
		.variable = "product",
		.dispatch = getvar_product
	}, {
		.variable = "platform",
		.dispatch = getvar_platform
	}, {
		.variable = "current-slot",
		.dispatch = getvar_current_slot
#if IS_ENABLED(CONFIG_FASTBOOT_FLASH)
	}, {
		.variable = "has-slot",
		.dispatch = getvar_has_slot
#endif
#if !IS_ENABLED(CONFIG_FASTBOOT_FLASH_NAND)
	}, {
		.variable = "partition-type",
		.dispatch = getvar_partition_type
#endif
#if IS_ENABLED(CONFIG_FASTBOOT_FLASH)
	}, {
		.variable = "partition-size",
		.dispatch = getvar_partition_size
#endif
	}, {
		.variable = "is-userspace",
		.dispatch = getvar_is_userspace
	}
};

static void getvar_version(char *var_parameter, char *response)
{
	fastboot_okay(FASTBOOT_VERSION, response);
}

static void getvar_version_bootloader(char *var_parameter, char *response)
{
	fastboot_okay(U_BOOT_VERSION, response);
}

static void getvar_downloadsize(char *var_parameter, char *response)
{
	fastboot_response("OKAY", response, "0x%08x", fastboot_buf_size);
}

static void getvar_serialno(char *var_parameter, char *response)
{
	const char *tmp = env_get("serial#");

	if (tmp)
		fastboot_okay(tmp, response);
	else
		fastboot_fail("Value not set", response);
}

static void getvar_version_baseband(char *var_parameter, char *response)
{
	fastboot_okay("N/A", response);
}

static void getvar_product(char *var_parameter, char *response)
{
	const char *board = env_get("board");

	if (board)
		fastboot_okay(board, response);
	else
		fastboot_fail("Board not set", response);
}

static void getvar_platform(char *var_parameter, char *response)
{
	const char *p = env_get("platform");

	if (p)
		fastboot_okay(p, response);
	else
		fastboot_fail("platform not set", response);
}

static void getvar_current_slot(char *var_parameter, char *response)
{
	/* A/B not implemented, for now always return "a" */
	fastboot_okay("a", response);
}

static void __maybe_unused getvar_has_slot(char *part_name, char *response)
{
	char part_name_wslot[PART_NAME_LEN];
	size_t len;
	int r;

	if (!part_name || part_name[0] == '\0')
		goto fail;

	/* part_name_wslot = part_name + "_a" */
	len = strlcpy(part_name_wslot, part_name, PART_NAME_LEN - 3);
	if (len >= PART_NAME_LEN - 3)
		goto fail;
	strcat(part_name_wslot, "_a");

	r = flash_backend->get_part_size(part_name_wslot, NULL, response);
	if (r >= 0) {
		fastboot_okay("yes", response); /* part exists and slotted */
		return;
	}

	r = flash_backend->get_part_size(part_name, NULL, response);
	if (r >= 0)
		fastboot_okay("no", response); /* part exists but not slotted */

	/* At this point response is filled with okay or fail string */
	return;

fail:
	fastboot_fail("invalid partition name", response);
}

static void __maybe_unused getvar_partition_type(char *part_name, char *response)
{
	const char *part_type;

	if (!flash_backend->get_part_type) {
		fastboot_fail("not supported", response);
		return;
	}

	part_type = flash_backend->get_part_type(part_name, response);
	if (part_type)
		fastboot_okay(part_type, response);
	else
		fastboot_fail("failed to get partition type", response);
}

static void __maybe_unused getvar_partition_size(char *part_name, char *response)
{
	int r;
	size_t size;

	r = flash_backend->get_part_size(part_name, &size, response);
	if (r >= 0)
		fastboot_response("OKAY", response, "0x%016zx", size);
}

static void getvar_is_userspace(char *var_parameter, char *response)
{
	fastboot_okay("no", response);
}

/**
 * fastboot_getvar() - Writes variable indicated by cmd_parameter to response.
 *
 * @cmd_parameter: Pointer to command parameter
 * @response: Pointer to fastboot response buffer
 *
 * Look up cmd_parameter first as an environment variable of the form
 * fastboot.<cmd_parameter>, if that exists return use its value to set
 * response.
 *
 * Otherwise lookup the name of variable and execute the appropriate
 * function to return the requested value.
 */
void fastboot_getvar(char *cmd_parameter, char *response)
{
	if (!cmd_parameter) {
		fastboot_fail("missing var", response);
	} else {
#define FASTBOOT_ENV_PREFIX	"fastboot."
		int i;
		char *var_parameter = cmd_parameter;
		char envstr[FASTBOOT_RESPONSE_LEN];
		const char *s;

		snprintf(envstr, sizeof(envstr) - 1,
			 FASTBOOT_ENV_PREFIX "%s", cmd_parameter);
		s = env_get(envstr);
		if (s) {
			fastboot_response("OKAY", response, "%s", s);
			return;
		}

		strsep(&var_parameter, ":");
		for (i = 0; i < ARRAY_SIZE(getvar_dispatch); ++i) {
			if (!strcmp(getvar_dispatch[i].variable,
				    cmd_parameter)) {
				getvar_dispatch[i].dispatch(var_parameter,
							    response);
				return;
			}
		}
		pr_warn("WARNING: unknown variable: %s\n", cmd_parameter);
		fastboot_fail("Variable not implemented", response);
	}
}
