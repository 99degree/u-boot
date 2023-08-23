/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Common configuration for Qualcomm boards.
 *
 * (C) Copyright 2023 Linaro
 * Author: Caleb Connolly <caleb.connolly@linaro.org>
 */

#ifndef __CONFIGS_QCOM_H
#define __CONFIGS_QCOM_H

#include <linux/sizes.h>

#define CFG_SYS_BAUDRATE_TABLE	{ 115200, 230400, 460800, 921600 }

#define CFG_EXTRA_ENV_SETTINGS \
	"bootdelay=3\0"		\
	"stdin=serial,button-kbd\0"	\
	"stdout=serial,vidconsole\0"	\
	"stderr=serial,vidconsole\0" \
	"bootcmd=usb start; load usb 0:1 ${kernel_comp_addr_r} /EFI/Boot/bootaa64.efi; " \
		"load usb 0:1 ${fdt_addr_r} /dtbs/${devicetree}; " \
		"bootefi $kernel_comp_addr_r $fdt_addr_r\0"

#endif
