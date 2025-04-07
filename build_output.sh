#!/bin/bash

build_dir=$(basename $(pwd))
echo $build_dir

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
	ln -sf /tmp/${build_dir}/ ./.output
fi

echo /tmp/${build_dir}/
rm -rf /tmp/${build_dir}/

make ARCH=arm CROSS_COMPILE=aarch64-linux-gnu- O=/tmp/${build_dir}/ qcom_defconfig qcom-phone.config qcom-sm71xx.config qcom-multi-dtb.config misc.config
make ARCH=arm CROSS_COMPILE=aarch64-linux-gnu- O=/tmp/${build_dir}/ -j$(nproc)

gzip /tmp/${build_dir}/u-boot-nodtb.bin -c > /tmp/${build_dir}/u-boot-nodtb.bin.gz
cat /tmp/${build_dir}/u-boot-nodtb.bin.gz /tmp/${build_dir}/dts/upstream/src/arm64/qcom/qcom-multi-dummy.dtb > /tmp/${build_dir}/uboot-dtb
./mkbootimg/mkbootimg --base '0x0' --kernel_offset '0x00008000' --pagesize '4096' --kernel /tmp/${build_dir}/uboot-dtb --ramdisk /tmp/${build_dir}/fit-dtb.blob -o /tmp/${build_dir}/u-boot.img
