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

#define CFG_SYS_BAUDRATE_TABLE	{ 115200, 230400, 460800, 921600, 3000000 }

#define CFG_EXTRA_ENV_SETTINGS \
	"bootdelay=-1\0" \
	"bootmenu_delay=-1\0" \
	"stdin=serial,button-kbd\0"	\
	"stdout=vidconsole,serial\0"	\
	"stderr=vidconsole,serial\0" \
	"bootmenu_0=Boot first available device=bootflow scan -b\0" \
	"bootmenu_1=Enable USB mass storage=ums 0 scsi 0,1,2,3,4,5\0" \
	"bootmenu_2=Enable fastboot mode=fastboot usb 0\0" \
	"bootmenu_3=Reset device=reset\0" \
	"menucmd=bootmenu\0" \
	"bootcmd=bootflow scan -b\0" /* first entry is default */

#endif
