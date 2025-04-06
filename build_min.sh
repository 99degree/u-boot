#!/bin/bash
if [ -e board/qualcomm/misc.config ]
then
	echo ok found misc.config
else
	echo generate misc.config
	echo CONFIG_PANIC_HANG=n > board/qualcomm/misc.config
fi

if [ -e .output ]
then
	echo ok
else
	ln -sf /tmp/output ./.output
fi

#rm -rf /tmp/output

#make ARCH=arm CROSS_COMPILE=aarch64-linux-gnu- O=/tmp/output/ qcom_defconfig
make ARCH=arm CROSS_COMPILE=aarch64-linux-gnu- O=/tmp/output/ -j$(nproc)

gzip /tmp/output/u-boot-nodtb.bin -c > /tmp/output/u-boot-nodtb.bin.gz
cat /tmp/output/u-boot-nodtb.bin.gz /tmp/output/dts/upstream/src/arm64/qcom/qcom-multi-dummy.dtb > /tmp/output/uboot-dtb
./mkbootimg/mkbootimg --base '0x0' --kernel_offset '0x00008000' --pagesize '4096' --kernel /tmp/output/uboot-dtb --ramdisk /tmp/output/fit-dtb.blob -o /tmp/output/u-boot.img
