/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Configuration file for QRB2210 RB1 board
 *
 * Copyright (c) 2023, Linaro Limited
 */

#ifndef __CONFIGS_QRB2210RB1_H
#define __CONFIGS_QRB2210RB1_H

#include <linux/sizes.h>
#include <asm/arch/sysmap-qcm2290.h>

#define CFG_SYS_BAUDRATE_TABLE	{ 115200 }

#define CFG_EXTRA_ENV_SETTINGS \
	"bootm_size=0x5000000\0"	\
	"bootm_low=0x90008000\0"	\
	"bootcmd=bootm $prevbl_initrd_start_addr\0"

#endif
