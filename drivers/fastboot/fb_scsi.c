// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2014 Broadcom Corporation.
 * Copyright 2023 Linaro Ltd.
 *
 * Based on fb_mmc.c
 */

#include <config.h>
#include <common.h>
#include <blk.h>
#include <env.h>
#include <fastboot.h>
#include <fastboot-internal.h>
#include <fb_mmc.h>
#include <image-sparse.h>
#include <image.h>
#include <log.h>
#include <part.h>
#include <mmc.h>
#include <div64.h>
#include <linux/compat.h>
#include <android_image.h>

#include "fb_backend.h"

#define BOOT_PARTITION_NAME "boot"

struct fb_scsi_sparse {
	struct blk_desc	*dev_desc;
};

static int raw_part_get_info_by_name(struct blk_desc *dev_desc,
				     const char *name,
				     struct disk_partition *info)
{
	/* strlen("fastboot_raw_partition_") + PART_NAME_LEN + 1 */
	char env_desc_name[23 + PART_NAME_LEN + 1];
	char *raw_part_desc;
	const char *argv[2];
	const char **parg = argv;

	/* check for raw partition descriptor */
	strcpy(env_desc_name, "fastboot_raw_partition_");
	strlcat(env_desc_name, name, sizeof(env_desc_name));
	raw_part_desc = strdup(env_get(env_desc_name));
	if (raw_part_desc == NULL)
		return -ENODEV;

	/*
	 * parse partition descriptor
	 *
	 * <lba_start> <lba_size> [mmcpart <num>]
	 */
	for (; parg < argv + sizeof(argv) / sizeof(*argv); ++parg) {
		*parg = strsep(&raw_part_desc, " ");
		if (*parg == NULL) {
			pr_err("Invalid number of arguments.\n");
			return -ENODEV;
		}
	}

	info->start = simple_strtoul(argv[0], NULL, 0);
	info->size = simple_strtoul(argv[1], NULL, 0);
	info->blksz = dev_desc->blksz;
	strlcpy((char *)info->name, name, PART_NAME_LEN);

	return 0;
}

static int do_get_part_info(struct blk_desc **dev_desc, const char *name,
			    struct disk_partition *info)
{
	int ret;

	/* Then try dev.hwpart:part */
	ret = part_get_info_by_dev_and_name_or_num("scsi", name, dev_desc,
						   info, true);
	return ret;
}

static int part_get_info_by_name_or_alias(struct blk_desc **dev_desc,
					  const char *name,
					  struct disk_partition *info)
{
	/* strlen("fastboot_partition_alias_") + PART_NAME_LEN + 1 */
	char env_alias_name[25 + PART_NAME_LEN + 1];
	char *aliased_part_name;

	/* check for alias */
	strlcpy(env_alias_name, "fastboot_partition_alias_", sizeof(env_alias_name));
	strlcat(env_alias_name, name, sizeof(env_alias_name));
	aliased_part_name = env_get(env_alias_name);
	if (aliased_part_name)
		name = aliased_part_name;

	return do_get_part_info(dev_desc, name, info);
}

/**
 * fb_scsi_blk_write() - Write/erase scsi in chunks of FASTBOOT_MAX_BLK_WRITE
 *
 * @block_dev: Pointer to block device
 * @start: First block to write/erase
 * @blkcnt: Count of blocks
 * @buffer: Pointer to data buffer for write or NULL for erase
 */
static lbaint_t fb_scsi_blk_write(struct blk_desc *block_dev, lbaint_t start,
				 lbaint_t blkcnt, const void *buffer)
{
	lbaint_t blk = start;
	lbaint_t blks_written;
	lbaint_t cur_blkcnt;
	lbaint_t blks = 0;
	int i;

	for (i = 0; i < blkcnt; i += FASTBOOT_MAX_BLK_WRITE) {
		cur_blkcnt = min((int)blkcnt - i, FASTBOOT_MAX_BLK_WRITE);
		if (buffer) {
			if (fastboot_progress_callback)
				fastboot_progress_callback("writing");
			blks_written = blk_dwrite(block_dev, blk, cur_blkcnt,
						  buffer + (i * block_dev->blksz));
		} else {
			if (fastboot_progress_callback)
				fastboot_progress_callback("erasing");
			blks_written = blk_derase(block_dev, blk, cur_blkcnt);
		}
		blk += blks_written;
		blks += blks_written;
	}
	return blks;
}

static lbaint_t fb_scsi_sparse_write(struct sparse_storage *info,
		lbaint_t blk, lbaint_t blkcnt, const void *buffer)
{
	struct fb_scsi_sparse *sparse = info->priv;
	struct blk_desc *dev_desc = sparse->dev_desc;

	return fb_scsi_blk_write(dev_desc, blk, blkcnt, buffer);
}

static lbaint_t fb_scsi_sparse_reserve(struct sparse_storage *info,
		lbaint_t blk, lbaint_t blkcnt)
{
	return blkcnt;
}

static void write_raw_image(struct blk_desc *dev_desc,
			    struct disk_partition *info, const char *part_name,
			    void *buffer, u32 download_bytes, char *response)
{
	lbaint_t blkcnt;
	lbaint_t blks;

	/* determine number of blocks to write */
	blkcnt = ((download_bytes + (info->blksz - 1)) & ~(info->blksz - 1));
	blkcnt = lldiv(blkcnt, info->blksz);

	if (blkcnt > info->size) {
		pr_err("too large for partition: '%s'\n", part_name);
		fastboot_fail("too large for partition", response);
		return;
	}

	puts("Flashing Raw Image\n");

	blks = fb_scsi_blk_write(dev_desc, info->start, blkcnt, buffer);

	if (blks != blkcnt) {
		pr_err("failed writing to device %d\n", dev_desc->devnum);
		fastboot_fail("failed writing to device", response);
		return;
	}

	printf("........ wrote " LBAFU " bytes to '%s'\n", blkcnt * info->blksz,
	       part_name);
	fastboot_okay(NULL, response);
}

/**
 * fastboot_scsi_get_part_info() - Lookup eMMC partion by name
 *
 * @part_name: Named partition to lookup
 * @dev_desc: Pointer to returned blk_desc pointer
 * @part_info: Pointer to returned struct disk_partition
 * @response: Pointer to fastboot response buffer
 */
int fastboot_scsi_get_part_info(const char *part_name,
			       struct blk_desc **dev_desc,
			       struct disk_partition *part_info, char *response)
{
	int ret;

	if (!part_name || !strcmp(part_name, "")) {
		fastboot_fail("partition not given", response);
		return -ENOENT;
	}

	ret = part_get_info_by_name_or_alias(dev_desc, part_name, part_info);
	if (ret < 0) {
		switch (ret) {
		case -ENOSYS:
		case -EINVAL:
			fastboot_fail("invalid partition or device", response);
			break;
		case -ENODEV:
			fastboot_fail("no such device", response);
			break;
		case -ENOENT:
			fastboot_fail("no such partition", response);
			break;
		case -EPROTONOSUPPORT:
			fastboot_fail("unknown partition table type", response);
			break;
		default:
			fastboot_fail("unanticipated error", response);
			break;
		}
	}

	return ret;
}

static struct blk_desc *fastboot_scsi_get_dev(int devnum, char *response)
{
	struct blk_desc *ret = blk_get_dev("scsi",
					   devnum);

	if (!ret || ret->type == DEV_TYPE_UNKNOWN) {
		pr_err("invalid scsi device\n");
		fastboot_fail("invalid scsi device", response);
		return NULL;
	}
	return ret;
}

static int parse_partnum(const char *cmd)
{
	const char *ns = cmd;
	char *endp;
	int num;

	/* strsep doesn't modify the string, just the pointer.
	 * it should take a char *const *s */
	strsep((char**)&ns, ":");
	if (!ns)
		return -EINVAL;

	num = simple_strtoul(ns, &endp, 10);
	if (!endp || endp == ns)
		return -EINVAL;

	return num;
}

/**
 * fastboot_scsi_flash_write() - Write image to eMMC for fastboot
 *
 * @cmd: Named partition to write image to
 * @download_buffer: Pointer to image data
 * @download_bytes: Size of image data
 * @response: Pointer to fastboot response buffer
 */
void fastboot_scsi_flash_write(const char *cmd, void *download_buffer,
			      u32 download_bytes, char *response)
{
	struct blk_desc *dev_desc;
	struct disk_partition info = {0};
	int devnum;

#if CONFIG_IS_ENABLED(EFI_PARTITION)
	if (strcmp(cmd, CONFIG_FASTBOOT_GPT_NAME) == 0) {
		devnum = parse_partnum(cmd);
		if (devnum < 0) {
			fastboot_fail("Couldn't parse partition number", response);
			return;
		}
		dev_desc = fastboot_scsi_get_dev(devnum, response);
		if (!dev_desc) {
			fastboot_fail("Partition not found", response);
			return;
		}

		printf("%s: updating MBR, Primary and Backup GPT(s)\n",
		       __func__);
		if (is_valid_gpt_buf(dev_desc, download_buffer)) {
			printf("%s: invalid GPT - refusing to write to flash\n",
			       __func__);
			fastboot_fail("invalid GPT partition", response);
			return;
		}
		if (write_mbr_and_gpt_partitions(dev_desc, download_buffer)) {
			printf("%s: writing GPT partitions failed\n", __func__);
			fastboot_fail("writing GPT partitions failed",
				      response);
			return;
		}
		part_init(dev_desc);
		printf("........ success\n");
		fastboot_okay(NULL, response);
		return;
	}
#endif

	if (!info.name[0] &&
	    fastboot_scsi_get_part_info(cmd, &dev_desc, &info, response) < 0)
		return;

	if (is_sparse_image(download_buffer)) {
		struct fb_scsi_sparse sparse_priv;
		struct sparse_storage sparse;
		int err;

		sparse_priv.dev_desc = dev_desc;

		sparse.blksz = info.blksz;
		sparse.start = info.start;
		sparse.size = info.size;
		sparse.write = fb_scsi_sparse_write;
		sparse.reserve = fb_scsi_sparse_reserve;
		sparse.mssg = fastboot_fail;

		printf("Flashing sparse image at offset " LBAFU "\n",
		       sparse.start);

		sparse.priv = &sparse_priv;
		err = write_sparse_image(&sparse, cmd, download_buffer,
					 response);
		if (!err)
			fastboot_okay(NULL, response);
	} else {
		write_raw_image(dev_desc, &info, cmd, download_buffer,
				download_bytes, response);
	}
}

/**
 * fastboot_scsi_flash_erase() - Erase eMMC for fastboot
 *
 * @cmd: Named partition to erase
 * @response: Pointer to fastboot response buffer
 */
void fastboot_scsi_erase(const char *cmd, char *response)
{
	struct blk_desc *dev_desc;
	struct disk_partition info;
	lbaint_t blks, blks_start, blks_size, grp_size;
	struct scsi *mmc = find_scsi_device(CONFIG_FASTBOOT_FLASH_scsi_DEV);

#ifdef CONFIG_FASTBOOT_scsi_BOOT_SUPPORT
	if (strcmp(cmd, CONFIG_FASTBOOT_scsi_BOOT1_NAME) == 0) {
		/* erase EMMC boot1 */
		dev_desc = fastboot_scsi_get_dev(response);
		if (dev_desc)
			fb_scsi_boot_ops(dev_desc, NULL, 1, 0, response);
		return;
	}
	if (strcmp(cmd, CONFIG_FASTBOOT_scsi_BOOT2_NAME) == 0) {
		/* erase EMMC boot2 */
		dev_desc = fastboot_scsi_get_dev(response);
		if (dev_desc)
			fb_scsi_boot_ops(dev_desc, NULL, 2, 0, response);
		return;
	}
#endif

#ifdef CONFIG_FASTBOOT_scsi_USER_SUPPORT
	if (strcmp(cmd, CONFIG_FASTBOOT_scsi_USER_NAME) == 0) {
		/* erase EMMC userdata */
		dev_desc = fastboot_scsi_get_dev(response);
		if (!dev_desc)
			return;

		if (fb_scsi_erase_scsi_hwpart(dev_desc))
			fastboot_fail("Failed to erase EMMC_USER", response);
		else
			fastboot_okay(NULL, response);
		return;
	}
#endif

	if (fastboot_scsi_get_part_info(cmd, &dev_desc, &info, response) < 0)
		return;

	/* Align blocks to erase group size to avoid erasing other partitions */
	grp_size = mmc->erase_grp_size;
	blks_start = (info.start + grp_size - 1) & ~(grp_size - 1);
	if (info.size >= grp_size)
		blks_size = (info.size - (blks_start - info.start)) &
				(~(grp_size - 1));
	else
		blks_size = 0;

	printf("Erasing blocks " LBAFU " to " LBAFU " due to alignment\n",
	       blks_start, blks_start + blks_size);

	blks = fb_scsi_blk_write(dev_desc, blks_start, blks_size, NULL);

	if (blks != blks_size) {
		pr_err("failed erasing from device %d\n", dev_desc->devnum);
		fastboot_fail("failed erasing from device", response);
		return;
	}

	printf("........ erased " LBAFU " bytes from '%s'\n",
	       blks_size * info.blksz, cmd);
	fastboot_okay(NULL, response);
}

const struct fastboot_flash_backend scsi_flash_backend = {
	// .get_part_size = fastboot_scsi_get_part_size,
	// .get_part_type = fastboot_scsi_get_part_type,
	.flash_write = fastboot_scsi_flash_write,
	.flash_erase = fastboot_scsi_erase,

	.flash_device = CONFIG_FASTBOOT_FLASH_SCSI_DEV,
};

const struct fastboot_flash_backend *flash_backend = &scsi_flash_backend;
