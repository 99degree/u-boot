// SPDX-License-Identifier: GPL-2.0+
/*
 * Qualcomm generic pmic gpio driver
 *
 * (C) Copyright 2015 Mateusz Kulikowski <mateusz.kulikowski@gmail.com>
 */

#include <button.h>
#include <common.h>
#include <dm.h>
#include <dm/lists.h>
#include <log.h>
#include <power/pmic.h>
#include <spmi/spmi.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <linux/bitops.h>

/* Register offset for each gpio */
#define REG_OFFSET(x)          ((x) * 0x100)

/* Register maps */

/* Type and subtype are shared for all PMIC peripherals */
#define REG_TYPE               0x4
#define REG_SUBTYPE            0x5

/* GPIO peripheral type and subtype out_values */
#define REG_TYPE_VAL		0x10
#define REG_SUBTYPE_GPIO_4CH	0x1
#define REG_SUBTYPE_GPIOC_4CH	0x5
#define REG_SUBTYPE_GPIO_8CH	0x9
#define REG_SUBTYPE_GPIOC_8CH	0xd
#define REG_SUBTYPE_GPIO_LV	0x10
#define REG_SUBTYPE_GPIO_MV	0x11

#define REG_STATUS             0x08
#define REG_STATUS_VAL_MASK    0x1

/* MODE_CTL */
#define REG_CTL		0x40
#define REG_CTL_MODE_MASK       0x70
#define REG_CTL_MODE_INPUT      0x00
#define REG_CTL_MODE_INOUT      0x20
#define REG_CTL_MODE_OUTPUT     0x10
#define REG_CTL_OUTPUT_MASK     0x0F
#define REG_CTL_LV_MV_MODE_MASK		0x3
#define REG_CTL_LV_MV_MODE_INPUT	0x0
#define REG_CTL_LV_MV_MODE_INOUT	0x2
#define REG_CTL_LV_MV_MODE_OUTPUT	0x1

#define REG_DIG_VIN_CTL        0x41
#define REG_DIG_VIN_VIN0       0

#define REG_DIG_PULL_CTL       0x42
#define REG_DIG_PULL_NO_PU     0x5

#define REG_LV_MV_OUTPUT_CTL	0x44
#define REG_LV_MV_OUTPUT_CTL_MASK	0x80
#define REG_LV_MV_OUTPUT_CTL_SHIFT	7

#define REG_DIG_OUT_CTL        0x45
#define REG_DIG_OUT_CTL_CMOS   (0x0 << 4)
#define REG_DIG_OUT_CTL_DRIVE_L 0x1

#define REG_EN_CTL             0x46
#define REG_EN_CTL_ENABLE      (1 << 7)

/**
 * pmic_gpio_match_data - platform specific configuration
 *
 * @PMIC_MATCH_NONE: no flags
 * @PMIC_MATCH_READONLY: treat all GPIOs as readonly, don't attempt to configure them
 */
enum pmic_gpio_match_data {
	PMIC_MATCH_NONE,
	PMIC_MATCH_READONLY = (1 << 0),
};

struct qcom_gpio_bank {
	uint32_t pid; /* Peripheral ID on SPMI bus */
	bool     lv_mv_type; /* If subtype is GPIO_LV(0x10) or GPIO_MV(0x11) */
};

static int qcom_gpio_set_direction(struct udevice *dev, unsigned offset,
				   bool input, int value)
{
	struct qcom_gpio_bank *priv = dev_get_priv(dev);
	uint32_t gpio_base = priv->pid + REG_OFFSET(offset);
	uint32_t reg_ctl_val;
	ulong match_flags = dev_get_driver_data(dev);
	int ret = 0;

	/* Some PMICs don't like their GPIOs being configured */
	if (match_flags & PMIC_MATCH_READONLY)
		return 0;

	/* Disable the GPIO */
	ret = pmic_clrsetbits(dev->parent, gpio_base + REG_EN_CTL,
			      REG_EN_CTL_ENABLE, 0);
	if (ret < 0)
		return ret;

	/* Select the mode and output */
	if (priv->lv_mv_type) {
		if (input)
			reg_ctl_val = REG_CTL_LV_MV_MODE_INPUT;
		else
			reg_ctl_val = REG_CTL_LV_MV_MODE_INOUT;
	} else {
		if (input)
			reg_ctl_val = REG_CTL_MODE_INPUT;
		else
			reg_ctl_val = REG_CTL_MODE_INOUT | !!value;
	}

	ret = pmic_reg_write(dev->parent, gpio_base + REG_CTL, reg_ctl_val);
	if (ret < 0)
		return ret;

	if (priv->lv_mv_type && !input) {
		ret = pmic_reg_write(dev->parent,
				     gpio_base + REG_LV_MV_OUTPUT_CTL,
				     !!value << REG_LV_MV_OUTPUT_CTL_SHIFT);
		if (ret < 0)
			return ret;
	}

	/* Set the right pull (no pull) */
	ret = pmic_reg_write(dev->parent, gpio_base + REG_DIG_PULL_CTL,
			     REG_DIG_PULL_NO_PU);
	if (ret < 0)
		return ret;

	/* Configure output pin drivers if needed */
	if (!input) {
		/* Select the VIN - VIN0, pin is input so it doesn't matter */
		ret = pmic_reg_write(dev->parent, gpio_base + REG_DIG_VIN_CTL,
				     REG_DIG_VIN_VIN0);
		if (ret < 0)
			return ret;

		/* Set the right dig out control */
		ret = pmic_reg_write(dev->parent, gpio_base + REG_DIG_OUT_CTL,
				     REG_DIG_OUT_CTL_CMOS |
				     REG_DIG_OUT_CTL_DRIVE_L);
		if (ret < 0)
			return ret;
	}

	/* Enable the GPIO */
	return pmic_clrsetbits(dev->parent, gpio_base + REG_EN_CTL, 0,
			       REG_EN_CTL_ENABLE);
}

static int qcom_gpio_direction_input(struct udevice *dev, unsigned offset)
{
	return qcom_gpio_set_direction(dev, offset, true, 0);
}

static int qcom_gpio_direction_output(struct udevice *dev, unsigned offset,
				      int value)
{
	return qcom_gpio_set_direction(dev, offset, false, value);
}

static int qcom_gpio_get_function(struct udevice *dev, unsigned offset)
{
	struct qcom_gpio_bank *priv = dev_get_priv(dev);
	uint32_t gpio_base = priv->pid + REG_OFFSET(offset);
	int reg;

	reg = pmic_reg_read(dev->parent, gpio_base + REG_CTL);
	if (reg < 0)
		return reg;

	if (priv->lv_mv_type) {
		switch (reg & REG_CTL_LV_MV_MODE_MASK) {
		case REG_CTL_LV_MV_MODE_INPUT:
			return GPIOF_INPUT;
		case REG_CTL_LV_MV_MODE_INOUT: /* Fallthrough */
		case REG_CTL_LV_MV_MODE_OUTPUT:
			return GPIOF_OUTPUT;
		default:
			return GPIOF_UNKNOWN;
		}
	} else {
		switch (reg & REG_CTL_MODE_MASK) {
		case REG_CTL_MODE_INPUT:
			return GPIOF_INPUT;
		case REG_CTL_MODE_INOUT: /* Fallthrough */
		case REG_CTL_MODE_OUTPUT:
			return GPIOF_OUTPUT;
		default:
			return GPIOF_UNKNOWN;
		}
	}
}

static int qcom_gpio_get_value(struct udevice *dev, unsigned offset)
{
	struct qcom_gpio_bank *priv = dev_get_priv(dev);
	uint32_t gpio_base = priv->pid + REG_OFFSET(offset);
	int reg;

	reg = pmic_reg_read(dev->parent, gpio_base + REG_STATUS);
	if (reg < 0)
		return reg;

	return !!(reg & REG_STATUS_VAL_MASK);
}

static int qcom_gpio_set_value(struct udevice *dev, unsigned offset,
			       int value)
{
	struct qcom_gpio_bank *priv = dev_get_priv(dev);
	uint32_t gpio_base = priv->pid + REG_OFFSET(offset);

	/* Set the output value of the gpio */
	if (priv->lv_mv_type)
		return pmic_clrsetbits(dev->parent,
				       gpio_base + REG_LV_MV_OUTPUT_CTL,
				       REG_LV_MV_OUTPUT_CTL_MASK,
				       !!value << REG_LV_MV_OUTPUT_CTL_SHIFT);
	else
		return pmic_clrsetbits(dev->parent, gpio_base + REG_CTL,
				       REG_CTL_OUTPUT_MASK, !!value);
}

static const struct dm_gpio_ops qcom_gpio_ops = {
	.direction_input	= qcom_gpio_direction_input,
	.direction_output	= qcom_gpio_direction_output,
	.get_value		= qcom_gpio_get_value,
	.set_value		= qcom_gpio_set_value,
	.get_function		= qcom_gpio_get_function,
};

static int qcom_gpio_probe(struct udevice *dev)
{
	struct qcom_gpio_bank *priv = dev_get_priv(dev);
	int reg;
	uint64_t pid;

	pid = dev_read_addr(dev);
	if (pid == FDT_ADDR_T_NONE)
		return log_msg_ret("bad address", -EINVAL);

	priv->pid = pid;

	/* Do a sanity check */
	reg = pmic_reg_read(dev->parent, priv->pid + REG_TYPE);
	if (reg != REG_TYPE_VAL)
		return log_msg_ret("bad type", -ENXIO);

	reg = pmic_reg_read(dev->parent, priv->pid + REG_SUBTYPE);
	if (reg != REG_SUBTYPE_GPIO_4CH && reg != REG_SUBTYPE_GPIOC_4CH &&
	    reg != REG_SUBTYPE_GPIO_LV && reg != REG_SUBTYPE_GPIO_MV)
		return log_msg_ret("bad subtype", -ENXIO);

	priv->lv_mv_type = reg == REG_SUBTYPE_GPIO_LV ||
			   reg == REG_SUBTYPE_GPIO_MV;

	return 0;
}

/*
 * Parse basic GPIO count specified via the gpio-ranges property
 * as specified in Linux devicetrees
 * Returns < 0 on error, otherwise gpio count
 */
static int qcom_gpio_of_parse_ranges(struct udevice *dev)
{
	int ret;
	struct ofnode_phandle_args args;

	ret = ofnode_parse_phandle_with_args(dev_ofnode(dev), "gpio-ranges", 
				     NULL, 3, 0, &args);
	if (ret) {
		return log_msg_ret("gpio-ranges", ret);
	}

	return args.args[2];
}

static int qcom_gpio_of_to_plat(struct udevice *dev)
{
	struct gpio_dev_priv *uc_priv = dev_get_uclass_priv(dev);
	int ret;

	uc_priv->gpio_count = dev_read_u32_default(dev, "gpio-count", 0);
	if (!uc_priv->gpio_count) {
		ret = qcom_gpio_of_parse_ranges(dev);
		if (ret > 0)
			uc_priv->gpio_count = ret;
		else
			printf("gpio-ranges error: %d\n", ret);
	}
	uc_priv->bank_name = dev_read_string(dev, "gpio-bank-name");
	if (uc_priv->bank_name == NULL)
		uc_priv->bank_name = "qcom_pmic";

	return 0;
}

static const struct udevice_id qcom_gpio_ids[] = {
	{ .compatible = "qcom,pm8916-gpio" },
	{ .compatible = "qcom,pm8994-gpio" },	/* 22 GPIO's */
	{ .compatible = "qcom,pm8998-gpio", .data = PMIC_MATCH_READONLY },
	{ .compatible = "qcom,pms405-gpio" },
	{ }
};

U_BOOT_DRIVER(qcom_pmic_gpio) = {
	.name	= "qcom_pmic_gpio",
	.id	= UCLASS_GPIO,
	.of_match = qcom_gpio_ids,
	.of_to_plat = qcom_gpio_of_to_plat,
	.probe	= qcom_gpio_probe,
	.ops	= &qcom_gpio_ops,
	.priv_auto	= sizeof(struct qcom_gpio_bank),
};

struct qcom_pmic_btn_priv {
	uint32_t base;
	uint32_t status_bit;
	int code;
	struct udevice *pmic;
};

/* Add pmic buttons as GPIO as well - there is no generic way for now */
#define PON_INT_RT_STS                        0x10
#define KPDPWR_ON_INT_BIT                     0
#define RESIN_ON_INT_BIT                      1

static enum button_state_t qcom_pwrkey_get_state(struct udevice *dev)
{
	struct qcom_pmic_btn_priv *priv = dev_get_priv(dev);

	int reg = pmic_reg_read(priv->pmic, priv->base + PON_INT_RT_STS);

	if (reg < 0)
		return 0;

	return (reg & BIT(priv->status_bit)) != 0;
}

static int qcom_pwrkey_get_code(struct udevice *dev)
{
	struct qcom_pmic_btn_priv *priv = dev_get_priv(dev);

	return priv->code;
}

static int qcom_pwrkey_probe(struct udevice *dev)
{
	struct button_uc_plat *uc_plat = dev_get_uclass_plat(dev);
	struct qcom_pmic_btn_priv *priv = dev_get_priv(dev);
	int ret;
	uint64_t base;

	/* Ignore the top-level button node */
	if (!uc_plat->label)
		return 0;

	/* the pwrkey and resin nodes are children of the "pon" node, get the
	 * PMIC device to use in pmic_reg_* calls.
	 */
	priv->pmic = dev->parent->parent;

	base = dev_read_addr(dev);
	if (!base || base == FDT_ADDR_T_NONE) {
		/* Linux devicetrees don't specify an address in the pwrkey node */
		base = dev_read_addr(dev->parent);
		if (base == FDT_ADDR_T_NONE) {
			printf("%s: Can't find address\n", dev->name);
			return -EINVAL;
		}
	}

	priv->base = base;

	/* Do a sanity check */
	ret = pmic_reg_read(priv->pmic, priv->base + REG_TYPE);
	if (ret != 0x1 && ret != 0xb) {
		printf("%s: unexpected PMIC function type %d\n", dev->name, ret);
		return -ENXIO;
	}

	ret = pmic_reg_read(priv->pmic, priv->base + REG_SUBTYPE);
	if ((ret & 0x7) == 0) {
		printf("%s: unexpected PMCI function subtype %d\n", dev->name, ret);
		//return -ENXIO;
	}

	/* Bit of a hack, we use the interrupt number to derive if this is the pwrkey or resin node, it just
	 * so happens to line up with the bit numbers in the interrupt status register
	 */
	ret = ofnode_read_u32_index(dev_ofnode(dev), "interrupts", 2, &priv->status_bit);
	if (ret < 0) {
		printf("%s: Couldn't read interrupts: %d\n", __func__, ret);
		return ret;
	}

	ret = ofnode_read_u32(dev_ofnode(dev), "linux,code", &priv->code);
	if (ret < 0) {
		printf("%s: Couldn't read interrupts: %d\n", __func__, ret);
		return ret;
	}

	return 0;
}

static int button_qcom_pmic_bind(struct udevice *parent)
{
	struct udevice *dev;
	ofnode node;
	int ret;

	dev_for_each_subnode(node, parent) {
		struct button_uc_plat *uc_plat;
		const char *label;

		if (!ofnode_is_enabled(node))
			continue;

		label = ofnode_read_string(node, "label");
		if (!label) {
			printf("%s: node %s has no label\n", __func__,
			      ofnode_get_name(node));
			/* Don't break booting, just print a warning and continue */
			continue;
		}
		printf("binding btn %s // %s\n", ofnode_get_name(node), label);
		/* We need the PMIC device to be the parent, so flatten it out here */
		ret = device_bind_driver_to_node(parent, "pwrkey_qcom",
						 ofnode_get_name(node),
						 node, &dev);
		if (ret) {
			printf("Failed to bind %s! %d\n", label, ret);
			return ret;
		}
		uc_plat = dev_get_uclass_plat(dev);
		uc_plat->label = label;
	}

	return 0;
}

static const struct button_ops button_qcom_pmic_ops = {
	.get_state	= qcom_pwrkey_get_state,
	.get_code	= qcom_pwrkey_get_code,
};


static const struct udevice_id qcom_pwrkey_ids[] = {
	{ .compatible = "qcom,pm8916-pwrkey" },
	{ .compatible = "qcom,pm8994-pwrkey" },
	{ .compatible = "qcom,pm8941-pwrkey" },
	{ .compatible = "qcom,pm8998-pon" },
	{ }
};

U_BOOT_DRIVER(pwrkey_qcom) = {
	.name	= "pwrkey_qcom",
	.id	= UCLASS_BUTTON,
	.of_match = qcom_pwrkey_ids,
	.bind = button_qcom_pmic_bind,
	.probe	= qcom_pwrkey_probe,
	.ops	= &button_qcom_pmic_ops,
	.priv_auto	= sizeof(struct qcom_pmic_btn_priv),
};
