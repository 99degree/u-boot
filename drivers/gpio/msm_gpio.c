// SPDX-License-Identifier: GPL-2.0+
/*
 * Qualcomm GPIO driver
 *
 * (C) Copyright 2015 Mateusz Kulikowski <mateusz.kulikowski@gmail.com>
 */

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <asm/global_data.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <qcom-gpio.h>

DECLARE_GLOBAL_DATA_PTR;

/* OE */
#define GPIO_OE_DISABLE  (0x0 << 9)
#define GPIO_OE_ENABLE   (0x1 << 9)
#define GPIO_OE_MASK     (0x1 << 9)

/* GPIO_IN_OUT register shifts. */
#define GPIO_IN          0
#define GPIO_OUT         1

#define _DEV_PIN_OFFS(dev) (((struct qcom_gpio_priv*)dev_get_priv(dev))->pin_offsets)

static int msm_gpio_direction_input(struct udevice *dev, unsigned int gpio)
{
	struct qcom_gpio_priv *priv = dev_get_priv(dev);
	phys_addr_t reg = priv->base + GPIO_CONFIG_OFFSET(dev, gpio);

	if (msm_pinctrl_is_reserved(dev_get_parent(dev), gpio))
		return 0;

	/* Disable OE bit */
	clrsetbits_le32(reg, GPIO_OE_MASK, GPIO_OE_DISABLE);

	return 0;
}

static int msm_gpio_set_value(struct udevice *dev, unsigned gpio, int value)
{
	struct qcom_gpio_priv *priv = dev_get_priv(dev);

	if (msm_pinctrl_is_reserved(dev_get_parent(dev), gpio))
		return 0;

	value = !!value;
	/* set value */
	writel(value << GPIO_OUT, priv->base + GPIO_IN_OUT_OFF(dev, gpio));

	return 0;
}

static int msm_gpio_direction_output(struct udevice *dev, unsigned gpio,
				     int value)
{
	struct qcom_gpio_priv *priv = dev_get_priv(dev);
	phys_addr_t reg = priv->base + GPIO_CONFIG_OFFSET(dev, gpio);

	if (msm_pinctrl_is_reserved(dev_get_parent(dev), gpio))
		return 0;

	value = !!value;
	/* set value */
	writel(value << GPIO_OUT, priv->base + GPIO_IN_OUT_OFF(dev, gpio));
	/* switch direction */
	clrsetbits_le32(reg, GPIO_OE_MASK, GPIO_OE_ENABLE);

	return 0;
}

static int msm_gpio_get_value(struct udevice *dev, unsigned gpio)
{
	struct qcom_gpio_priv *priv = dev_get_priv(dev);
	ulong addr = priv->base + GPIO_IN_OUT_OFF(dev, gpio);

	if (msm_pinctrl_is_reserved(dev_get_parent(dev), gpio))
		return 0;

	return !!(readl(addr) >> GPIO_IN);
}

static int msm_gpio_get_function(struct udevice *dev, unsigned gpio)
{
	struct qcom_gpio_priv *priv = dev_get_priv(dev);
	ulong addr = priv->base + GPIO_CONFIG_OFFSET(dev, gpio);

	if (msm_pinctrl_is_reserved(dev_get_parent(dev), gpio))
		return GPIOF_UNKNOWN;

	if (readl(addr) & GPIO_OE_ENABLE)
		return GPIOF_OUTPUT;

	return GPIOF_INPUT;
}

static const struct dm_gpio_ops gpio_msm_ops = {
	.direction_input	= msm_gpio_direction_input,
	.direction_output	= msm_gpio_direction_output,
	.get_value		= msm_gpio_get_value,
	.set_value		= msm_gpio_set_value,
	.get_function		= msm_gpio_get_function,
};

static int msm_gpio_probe(struct udevice *dev)
{
	struct qcom_gpio_priv *priv = dev_get_priv(dev);

	priv->base = dev_read_addr(dev);
	priv->pin_offsets = msm_pinctrl_get_offsets(dev_get_parent(dev), NULL);

	return priv->base == FDT_ADDR_T_NONE ? -EINVAL : 0;
}

static int msm_gpio_of_to_plat(struct udevice *dev)
{
	struct gpio_dev_priv *uc_priv = dev_get_uclass_priv(dev);

	/* Get the pin count from the pinctrl driver */
	msm_pinctrl_get_offsets(dev_get_parent(dev), &uc_priv->gpio_count);

	uc_priv->bank_name = fdt_getprop(gd->fdt_blob, dev_of_offset(dev),
					 "gpio-bank-name", NULL);
	if (uc_priv->bank_name == NULL)
		uc_priv->bank_name = "soc";

	return 0;
}

U_BOOT_DRIVER(gpio_msm) = {
	.name	= "gpio_msm",
	.id	= UCLASS_GPIO,
	.of_to_plat = msm_gpio_of_to_plat,
	.probe	= msm_gpio_probe,
	.ops	= &gpio_msm_ops,
	.flags	= DM_UC_FLAG_SEQ_ALIAS,
	.priv_auto	= sizeof(struct qcom_gpio_priv),
};
