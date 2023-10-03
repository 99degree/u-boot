// SPDX-License-Identifier: GPL-2.0+
/*
 * TLMM driver for Qualcomm APQ8016, APQ8096
 *
 * (C) Copyright 2018 Ramon Fried <ramon.fried@gmail.com>
 *
 */

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <asm/io.h>
#include <dm/device_compat.h>
#include <dm/device-internal.h>
#include <dm/lists.h>
#include <asm/gpio.h>
#include <dm/pinctrl.h>
#include <linux/bitops.h>
#include <qcom-gpio.h>

#include "pinctrl-snapdragon.h"

#define MSM_PINCTRL_MAX_RESERVED_RANGES 32

struct msm_pinctrl_priv {
	phys_addr_t base;
	struct msm_pinctrl_data *data;
	u32 reserved_ranges[MSM_PINCTRL_MAX_RESERVED_RANGES * 2];
	int reserved_ranges_count;
};

unsigned long msm_pinctrl_gpio_config(unsigned int *pin_offsets, unsigned int selector);

#define _DEV_PIN_OFFS(x) (((struct msm_pinctrl_priv*)dev_get_priv(dev))->data->pin_offsets)

static const struct pinconf_param msm_conf_params[] = {
	{ "drive-strength", PIN_CONFIG_DRIVE_STRENGTH, 2 },
	{ "bias-disable", PIN_CONFIG_BIAS_DISABLE, 0 },
	{ "bias-pull-up", PIN_CONFIG_BIAS_PULL_UP, 3 },
};

static int msm_get_functions_count(struct udevice *dev)
{
	struct msm_pinctrl_priv *priv = dev_get_priv(dev);

	return priv->data->functions_count;
}

static int msm_get_pins_count(struct udevice *dev)
{
	struct msm_pinctrl_priv *priv = dev_get_priv(dev);

	return priv->data->pin_count;
}

static const char *msm_get_function_name(struct udevice *dev,
					 unsigned int selector)
{
	struct msm_pinctrl_priv *priv = dev_get_priv(dev);

	return priv->data->get_function_name(dev, selector);
}

static int msm_pinctrl_parse_ranges(struct udevice *dev)
{
	struct msm_pinctrl_priv *priv = dev_get_priv(dev);
	struct ofnode_phandle_args args;
	int count, ret;

	if (ofnode_read_prop(dev_ofnode(dev), "gpio-reserved-ranges",
			&count)) {
		if (count % 2 == 1) {
			dev_err(dev, "gpio-reserved-ranges must be a multiple of 2\n");
			return -EINVAL;
		}
		/* Size is in bytes, but we're indexing by ints */
		count /= 4;

		if (count > MSM_PINCTRL_MAX_RESERVED_RANGES) {
			dev_err(dev, "gpio-reserved-ranges must be less than %d (got %d)\n",
				MSM_PINCTRL_MAX_RESERVED_RANGES, count);
			return -EINVAL;
		}

		priv->reserved_ranges_count = count;
		for (count = 0; count < priv->reserved_ranges_count; count++) {
			if (ofnode_read_u32_index(dev_ofnode(dev), "gpio-reserved-ranges",
						count, &priv->reserved_ranges[count])) {
				dev_err(dev, "failed to read gpio-reserved-ranges[%d]\n", count);
				return -EINVAL;
			}
		}
	}

	ret = ofnode_parse_phandle_with_args(dev_ofnode(dev), "gpio-ranges", 
				     NULL, 3, 0, &args);
	if (ret) {
		dev_warn(dev, "Using deprecated gpio-count property\n");
		count = dev_read_u32_default(dev, "gpio-count", 0);
	} else {
		count = args.args[2];
	}

	/* Linux devicetrees have an additional special-case pin */
	if (count == priv->data->pin_count + 1)
		count--;

	if (count != priv->data->pin_count) {
		dev_err(dev, "gpio count %d != pin_count %d\n", count, priv->data->pin_count);
		return -EINVAL;
	}

	return 0;
}

static int msm_pinctrl_probe(struct udevice *dev)
{
	struct msm_pinctrl_priv *priv = dev_get_priv(dev);
	struct qcom_gpio_priv *gpio_priv;
	struct udevice *gpio_dev;

	priv->base = dev_read_addr(dev);
	priv->data = (struct msm_pinctrl_data *)dev_get_driver_data(dev);

	if (msm_pinctrl_parse_ranges(dev))
		return -EINVAL;

	if (device_get_child(dev, 0, &gpio_dev)) {
		dev_err(dev, "failed to get gpio child\n");
		return -ENODEV;
	}

	gpio_priv = dev_get_priv(gpio_dev);
	gpio_priv->pin_offsets = priv->data->pin_offsets;
	gpio_priv->base = dev_read_addr(dev);

	return priv->base == FDT_ADDR_T_NONE ? -EINVAL : 0;
}

static const char *msm_get_pin_name(struct udevice *dev, unsigned int selector)
{
	struct msm_pinctrl_priv *priv = dev_get_priv(dev);

	return priv->data->get_pin_name(dev, selector);
}

static int msm_pinmux_set(struct udevice *dev, unsigned int pin_selector,
			  unsigned int func_selector)
{
	struct msm_pinctrl_priv *priv = dev_get_priv(dev);

	if (msm_pinctrl_is_reserved(dev, pin_selector))
		return -EINVAL;

	debug("pin_selector: %d, func_selector: %d\n", pin_selector, func_selector);

	clrsetbits_le32(priv->base + GPIO_CONFIG_OFFSET(dev, pin_selector),
			TLMM_FUNC_SEL_MASK | TLMM_GPIO_DISABLE,
			priv->data->get_function_mux(func_selector) << 2);
	return 0;
}

static int msm_pinconf_set(struct udevice *dev, unsigned int pin_selector,
			   unsigned int param, unsigned int argument)
{
	struct msm_pinctrl_priv *priv = dev_get_priv(dev);

	if (msm_pinctrl_is_reserved(dev, pin_selector))
		return -EINVAL;

	debug("msm_pinconf_set: 0x%08llx pin_selector=%d, param=%d, argument=%d\n", priv->base + GPIO_CONFIG_OFFSET(dev, pin_selector),
			 pin_selector, param, argument);

	switch (param) {
	case PIN_CONFIG_DRIVE_STRENGTH:
		argument = (argument / 2) - 1;
		clrsetbits_le32(priv->base + GPIO_CONFIG_OFFSET(dev, pin_selector),
				TLMM_DRV_STRENGTH_MASK, argument << 6);
		break;
	case PIN_CONFIG_BIAS_DISABLE:
		clrbits_le32(priv->base + GPIO_CONFIG_OFFSET(dev, pin_selector),
			     TLMM_GPIO_PULL_MASK);
		break;
	case PIN_CONFIG_BIAS_PULL_UP:
		clrsetbits_le32(priv->base + GPIO_CONFIG_OFFSET(dev, pin_selector),
				TLMM_GPIO_PULL_MASK, argument);
		break;
	default:
		return 0;
	}

	return 0;
}

static struct pinctrl_ops msm_pinctrl_ops = {
	.get_pins_count = msm_get_pins_count,
	.get_pin_name = msm_get_pin_name,
	.set_state = pinctrl_generic_set_state,
	.pinmux_set = msm_pinmux_set,
	.pinconf_num_params = ARRAY_SIZE(msm_conf_params),
	.pinconf_params = msm_conf_params,
	.pinconf_set = msm_pinconf_set,
	.get_functions_count = msm_get_functions_count,
	.get_function_name = msm_get_function_name,
};

static int msm_pinctrl_bind(struct udevice *dev)
{
	ofnode node = dev_ofnode(dev);
	const char *name;
	struct driver *drv;
	int ret;

	ofnode_get_property(node, "gpio-controller", &ret);
	if (ret < 0)
		return 0;

	/* Get the name of gpio node */
	name = ofnode_get_name(node);
	if (!name)
		return -EINVAL;

	/* Bind gpio node */
	drv = lists_driver_lookup_name("gpio_msm");
	ret = device_bind_with_driver_data(dev, drv,
					 name, dev_get_driver_data(dev), node, NULL);
	if (ret)
		return ret;

	dev_dbg(dev, "bind %s\n", name);

	return 0;
}

static const struct udevice_id msm_pinctrl_ids[] = {
	{ .compatible = "qcom,msm8916-pinctrl", .data = (ulong)&apq8016_data },
	{ .compatible = "qcom,msm8996-pinctrl", .data = (ulong)&apq8096_data },
	{ .compatible = "qcom,sdm845-pinctrl", .data = (ulong)&sdm845_data },
	{ .compatible = "qcom,qcs404-pinctrl", .data = (ulong)&qcs404_data },
	{ .compatible = "qcom,qcm2290-tlmm", .data = (ulong)&qcm2290_data },
	{ .compatible = "qcom,sm6115-tlmm", .data = (ulong)&sm6115_data },
	{ }
};

U_BOOT_DRIVER(pinctrl_snapdraon) = {
	.name		= "pinctrl_msm",
	.id		= UCLASS_PINCTRL,
	.of_match	= msm_pinctrl_ids,
	.priv_auto	= sizeof(struct msm_pinctrl_priv),
	.ops		= &msm_pinctrl_ops,
	.probe		= msm_pinctrl_probe,
	.bind		= msm_pinctrl_bind,
	.flags		= DM_FLAG_PRE_RELOC,
};

const unsigned int* msm_pinctrl_get_offsets(struct udevice *dev, unsigned int *lenp)
{
	struct msm_pinctrl_data *data = (void*)dev_get_driver_data(dev);

	if (lenp)
		*lenp = data->pin_count;

	return data->pin_offsets;
}

bool msm_pinctrl_is_reserved(struct udevice *dev, unsigned int pin)
{
	struct msm_pinctrl_priv *priv = dev_get_priv(dev);
	unsigned int i, start;

	for (i = 0; i < priv->reserved_ranges_count; i+=2) {
		start = priv->reserved_ranges[i];
		if (pin >= start &&
		    pin < start + priv->reserved_ranges[i+1])
			return true;
	}

	return false;
}
