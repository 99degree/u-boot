#!/bin/bash
if [ -e board/qualcomm/misc.config ]
then
	echo ok found misc.config
else
	echo generate misc.config
	echo CONFIG_PANIC_HANG=n > board/qualcomm/misc.config
fi

if [ -e output ]
then
	echo ok
else
	ln -sf /tmp/u-boot-next ./output
fi

#rm -rf /tmp/output

#make ARCH=arm CROSS_COMPILE=aarch64-linux-gnu- O=/tmp/u-boot-next/ qcom_defconfig
make ARCH=arm CROSS_COMPILE=aarch64-linux-gnu- O=/tmp/u-boot-next/ menuconfig

gzip /tmp/u-boot-next/u-boot-nodtb.bin -c > /tmp/u-boot-next/u-boot-nodtb.bin.gz
cat /tmp/u-boot-next/u-boot-nodtb.bin.gz /tmp/u-boot-next/dts/upstream/src/arm64/qcom/qcom-multi-dummy.dtb > /tmp/u-boot-next/uboot-dtb
./mkbootimg/mkbootimg --base '0x0' --kernel_offset '0x00008000' --pagesize '4096' --kernel /tmp/u-boot-next/uboot-dtb --ramdisk /tmp/u-boot-next/fit-dtb.blob -o /tmp/u-boot-next/u-boot.img
