// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 Sartura Ltd.
 * Copyright (c) 2022 Linaro Ltd.
 *
 * Author: Robert Marko <robert.marko@sartura.hr>
 *         Sumit Garg <sumit.garg@linaro.org>
 *
 * Based on Linux driver
 */

#include <asm/io.h>
#include <common.h>
#include <dm.h>
#include <reset-uclass.h>
#include <linux/bitops.h>
#include <malloc.h>
#include <clk/qcom.h>

static void __iomem *base;

static int qcom_reset_assert(struct reset_ctl *rst)
{
	struct qcom_cc_data *data = (struct qcom_cc_data *)dev_get_driver_data(rst->dev);
	const struct qcom_reset_map *map;
	u32 value;

	if (rst->id >= data->num_resets) {
		printf("Invalid reset id %lu\n", rst->id);
		return -EINVAL;
	}

	map = &data->resets[rst->id];

	if (map->name)
		debug("  ASSERT reset %s\n", map->name);
	else
		debug("  ASSERT reset %lu\n", rst->id);

	value = readl(base + map->reg);
	value |= BIT(map->bit);
	writel(value, base + map->reg);

	return 0;
}

static int qcom_reset_deassert(struct reset_ctl *rst)
{
	struct qcom_cc_data *data = (struct qcom_cc_data *)dev_get_driver_data(rst->dev);
	const struct qcom_reset_map *map;
	u32 value;

	map = &data->resets[rst->id];

	if (map->name)
		debug("DEASSERT reset %s\n", map->name);
	else
		debug("DEASSERT reset %lu\n", rst->id);

	value = readl(base + map->reg);
	value &= ~BIT(map->bit);
	writel(value, base + map->reg);

	return 0;
}

static int qcom_reset_of_to_plat(struct udevice *dev)
{
	base = (void __iomem*)dev_read_addr(dev);

	if ((fdt_addr_t)base == FDT_ADDR_T_NONE) {
		printf("%s: can't read base address\n", dev->name);
		return -EINVAL;
	}

	return 0;
}

static const struct reset_ops qcom_reset_ops = {
	.rst_assert = qcom_reset_assert,
	.rst_deassert = qcom_reset_deassert,
};

U_BOOT_DRIVER(qcom_reset) = {
	.name = "qcom_reset",
	.id = UCLASS_RESET,
	.of_to_plat = qcom_reset_of_to_plat,
	.ops = &qcom_reset_ops,
};
