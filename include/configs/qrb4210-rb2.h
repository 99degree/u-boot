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
	"bootm_low=0x90008000\0"	\
	"bootcmd=mmc dev 0; "	\
	"setenv fdt_high 0xffffffffffffffff; "	\
	"setenv fdt_addr_r 0x80008000; "	\
	"ext4load mmc 0:0xe ${fdt_addr_r} /boot/qrb4210-rb2.dtb; "	\
	"setenv kernel_addr_r 0x80100000; "	\
	"ext4load mmc 0:0xe ${kernel_addr_r} /boot/Image.gz; "	\
	"setenv kernel_comp_size ${filesize}; "	\
	"setenv kernel_comp_addr_r 0x41000000; "	\
	"setenv initrd_high 0xffffffffffffffff; "	\
	"setenv ramdisk_addr_r 0x83000000; "	\
	"ext4load mmc 0:0xe ${ramdisk_addr_r} /boot/initramfs-rbX.cpio.gz; "	\
	"setenv bootargs console=ttyMSM0,115200n8 root=/dev/mmcblk0p14 rootwait rw ignore_loglevel earlycon; "	\
	"booti ${kernel_addr_r} ${ramdisk_addr_r}:${filesize} ${fdt_addr_r}\0"

#endif
