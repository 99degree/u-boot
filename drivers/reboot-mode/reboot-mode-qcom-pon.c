// SPDX-License-Identifier: GPL-2.0+
#include <dm.h>
#include <dm/device-internal.h>  // for driver_find_by_name(), device_bind_with_driver_data()
#include <reboot-mode/reboot-mode.h>
#include <power/pmic.h>
#include <linux/bitops.h>
#include <linux/delay.h>

#define PON_SOFT_RB_SPARE         0x8F
#define GEN1_REASON_SHIFT         2
#define GEN2_REASON_SHIFT         1
#define NO_REASON_SHIFT           0

/*
 * Private data for reboot-mode driver
 */
struct qcom_pon_priv {
    struct udevice *pmic;
    u32 baseaddr;
    u32 reason_shift;
};

/*
 * Reboot-mode set: write encoded mode to PON_SOFT_RB_SPARE
 */
static int qcom_pon_set(struct udevice *dev, u32 mode_id)
{
    struct qcom_pon_priv *priv = dev_get_priv(dev);
    u32 val = (mode_id << priv->reason_shift) & GENMASK(7, priv->reason_shift);

//printf("mock writing to pon addr 0x%x 0x%x\n", priv->baseaddr + PON_SOFT_RB_SPARE, val);
    return pmic_reg_write(priv->pmic,
                          priv->baseaddr + PON_SOFT_RB_SPARE,
                          val);
}

/*
 * Reboot-mode get: read and decode mode from PON_SOFT_RB_SPARE
 */
static int qcom_pon_get(struct udevice *dev, u32 *mode_id)
{
    struct qcom_pon_priv *priv = dev_get_priv(dev);
    int val;

    val = pmic_reg_read(priv->pmic, priv->baseaddr + PON_SOFT_RB_SPARE);
    if (val < 0)
        return val;

    *mode_id = (val >> priv->reason_shift) & GENMASK(7 - priv->reason_shift, 0);
    return 0;
}

/*
 * Probe: initialize PMIC and base address
 */
static int qcom_pon_probe(struct udevice *dev)
{
    struct qcom_pon_priv *priv = dev_get_priv(dev);

    priv->pmic = dev->parent;
    priv->baseaddr = dev_read_u32_default(dev, "reg", 0x800);
    priv->reason_shift = (u32)dev_get_driver_data(dev);

    return 0;
}

static const struct reboot_mode_ops qcom_pon_ops = {
    .set = qcom_pon_set,
    .get = qcom_pon_get,
};

/*
 * Reboot-mode driver: manually bound child
 */
U_BOOT_DRIVER(qcom_pon_reboot_mode) = {
    .name = "qcom_pon_reboot_mode",
    .id = UCLASS_REBOOT_MODE,
    .ops = &qcom_pon_ops,
    .probe = qcom_pon_probe,
    .priv_auto = sizeof(struct qcom_pon_priv),
};
