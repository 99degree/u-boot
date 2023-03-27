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

static int qcom_reset_assert(struct reset_ctl *rst)
{
	struct udevice *cdev = (struct udevice *)dev_get_driver_data(rst->dev);
	struct qcom_cc_priv *priv = dev_get_priv(cdev);
	const struct qcom_reset_map *map;
	u32 value;

	map = &priv->data->resets[rst->id];

	value = readl(priv->base + map->reg);
	value |= BIT(map->bit);
	writel(value, priv->base + map->reg);

	return 0;
}

static int qcom_reset_deassert(struct reset_ctl *rst)
{
	struct udevice *cdev = (struct udevice *)dev_get_driver_data(rst->dev);
	struct qcom_cc_priv *priv = dev_get_priv(cdev);
	const struct qcom_reset_map *map;
	u32 value;

	map = &priv->data->resets[rst->id];

	value = readl(priv->base + map->reg);
	value &= ~BIT(map->bit);
	writel(value, priv->base + map->reg);

	return 0;
}

static const struct reset_ops qcom_reset_ops = {
	.rst_assert = qcom_reset_assert,
	.rst_deassert = qcom_reset_deassert,
};

U_BOOT_DRIVER(qcom_reset) = {
	.name = "qcom_reset",
	.id = UCLASS_RESET,
	.ops = &qcom_reset_ops,
};
