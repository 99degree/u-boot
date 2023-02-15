/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Configuration file for QRB4210-RB2 board
 *
 * (C) Copyright 2023 Bhupesh Sharma <bhupesh.sharma@linaro.org>
 */

#ifndef __CONFIGS_QRB4210RB2_H
#define __CONFIGS_QRB4210RB2_H

#include <linux/sizes.h>
#include <asm/arch/sysmap-qrb4210rb2.h>

#define CFG_SYS_BAUDRATE_TABLE	{ 115200 }

#define CFG_EXTRA_ENV_SETTINGS \
	"bootm_size=0x5000000\0"	\
	"bootm_low=0x80000000\0"	\
	"bootcmd=bootm $prevbl_initrd_start_addr\0"

#endif
