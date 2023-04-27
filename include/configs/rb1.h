/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) 2023, Linaro Limited
 */

#ifndef __CONFIGS_RB1_H
#define __CONFIGS_RB1_H

#include <linux/sizes.h>
#include <asm/arch/sysmap-qcm2290.h>

#define CFG_SYS_BAUDRATE_TABLE	{ 115200 }

#define CFG_EXTRA_ENV_SETTINGS \
	"bootm_size=0x5000000\0"	\
	"bootm_low=0x90008000\0"	\
	"bootcmd=mmc dev 1; "	\
	"setenv fdt_high 0xffffffffffffffff; "	\
	"setenv fdt_addr_r 0x80008000; "	\
	"ext4load mmc 1:0x2 ${fdt_addr_r} /boot/qrb2210-rb1.dtb; "	\
	"setenv kernel_addr_r 0x80100000; "	\
	"ext4load mmc 1:0x2 ${kernel_addr_r} /boot/Image.gz; "	\
	"setenv kernel_comp_size ${filesize}; "	\
	"setenv kernel_comp_addr_r 0x41000000; "	\
	"setenv initrd_high 0xffffffffffffffff; "	\
	"setenv ramdisk_addr_r 0x83000000; "	\
	"ext4load mmc 1:0x2 ${ramdisk_addr_r} /boot/initramfs-rbX.cpio.gz; "	\
	"setenv bootargs console=ttyMSM0,115200n8 root=/dev/mmcblk1p2 rootwait rw ignore_loglevel earlycon; "	\
	"booti ${kernel_addr_r} ${ramdisk_addr_r}:${filesize} ${fdt_addr_r}\0"

#endif
