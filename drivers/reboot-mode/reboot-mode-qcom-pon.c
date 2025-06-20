// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2022 Sean Anderson <sean.anderson@seco.com>
 */

#include <linux/delay.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <power/pmic.h>
#include <reboot-mode/reboot-mode.h>

#define PON_SOFT_RB_SPARE               0x8f

#define GEN1_REASON_SHIFT               2
#define GEN2_REASON_SHIFT               1
#define NO_REASON_SHIFT                 0

#define PON_REASON_MASK                 0x3

/**
 * struct pmic_reboot_mode_priv - Private data for the nvmem reboot mode device
 */
struct pmic_reboot_mode_priv {
        int base;
        size_t size;
        int reason_shift;
};

static int reboot_mode_get(struct udevice *dev, u32 *val)
{
        struct pmic_reboot_mode_priv *priv;
        int ret;

        if (!dev)
                return -EINVAL;

        priv = dev_get_priv(dev);
        if (!priv)
                return -EINVAL;

        ret = pmic_reg_read(dev->parent, priv->base + PON_SOFT_RB_SPARE);

        *val = ret >> priv->reason_shift;

        dev_warn(dev, "read 0x%x (0x%x >> %d) to 0x%x \n", *val, ret, priv->reason_shift,
		priv->base + PON_SOFT_RB_SPARE);

        return 0;
}

static int reboot_mode_set(struct udevice *dev, u32 val)
{
        struct pmic_reboot_mode_priv *priv;
        int ret;

        printf("%s %d\n", __func__, __LINE__);

        if (!dev)
                return -EINVAL;

        priv = dev_get_priv(dev);
        if (!priv)
                return -EINVAL;

        /* force to enter bootloader atm */
        val = 2;
        //priv->reason_shift = 1;

        dev_warn(dev, "write 0x%x (0x%x) to 0x%x \n", val,
			(val & PON_REASON_MASK) << priv->reason_shift,
			priv->base + PON_SOFT_RB_SPARE);

	udelay(0x100000);
	udelay(0x100000);

        ret = pmic_reg_write(dev->parent, priv->base + PON_SOFT_RB_SPARE,
                                (val & PON_REASON_MASK) << priv->reason_shift);
        if (ret < 0) {
                log_err("error setting PON_SOFT_RB_SPARE: %d\n", ret);
                return ret;
        }

	return 0;
}

static const struct reboot_mode_ops pmic_reboot_mode_ops = {
	.get = reboot_mode_get,
	.set = reboot_mode_set,
};

static int reboot_mode_probe(struct udevice *dev)
{
        struct pmic_reboot_mode_priv *priv = dev_get_priv(dev);
        if (!priv)
                return -EINVAL;

        priv->base =
                dev_read_addr_size_index(dev, 0, (fdt_size_t *)&priv->size);
        if (priv->base == FDT_ADDR_T_NONE) {
                dev_err(dev, "Invalid address\n");
                return -EINVAL;
        }

        if (priv->size > sizeof(u32)) {
                dev_err(dev, "Invalid reg size\n");
                return -EINVAL;
        }

        priv->reason_shift = dev_get_driver_data(dev);

	return 0;
}

static const struct udevice_id pmic_reboot_mode_ids[] = {
        { .compatible = "qcom,pm8916-pon", .data = GEN1_REASON_SHIFT },
        { .compatible = "qcom,pm8941-pon", .data = NO_REASON_SHIFT },
        { .compatible = "qcom,pms405-pon", .data = GEN1_REASON_SHIFT },
        { .compatible = "qcom,pm8998-pon", .data = GEN2_REASON_SHIFT },
        { .compatible = "qcom,pmk8350-pon", .data = GEN2_REASON_SHIFT },
	{ }
};

U_BOOT_DRIVER(pmic_reboot_mode) = {
	.name = "pmic-reboot-mode",
	.id = UCLASS_REBOOT_MODE,
	.of_match = pmic_reboot_mode_ids,
	.probe = reboot_mode_probe,
	.priv_auto = sizeof(struct pmic_reboot_mode_priv),
	.ops = &pmic_reboot_mode_ops,
};
