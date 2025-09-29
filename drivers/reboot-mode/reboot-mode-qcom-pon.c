// SPDX-License-Identifier: GPL-2.0+
#include <dm.h>
#include <reboot-mode/reboot-mode.h>
#include <spmi/spmi.h>
#include <linux/bitops.h>
#include <linux/delay.h>

#define PON_SOFT_RB_SPARE         0x8F
#define GEN1_REASON_SHIFT         2
#define GEN2_REASON_SHIFT         1
#define NO_REASON_SHIFT           0

struct qcom_pon_priv {
    struct udevice *spmi_dev;
    u32 baseaddr;
    u32 reason_shift;
};

static int qcom_pon_set(struct udevice *dev, u32 mode_id_)
{
    struct qcom_pon_priv *priv = dev_get_priv(dev);

    int mode_id = 1;
    int usid = dev_read_u32_default(priv->spmi_dev, "reg", 0);
    int pid = priv->baseaddr;
    int reg = PON_SOFT_RB_SPARE;
    int val = (mode_id << priv->reason_shift) & GENMASK(7, priv->reason_shift);

    printf("going to sleep soon, usid %d\n", usid);
    mdelay(2000);

    return spmi_reg_write(priv->spmi_dev, usid, pid, reg, val);
}

static int qcom_pon_probe(struct udevice *dev)
{
    struct qcom_pon_priv *priv = dev_get_priv(dev);

    priv->spmi_dev = dev->parent;
    priv->baseaddr = dev_read_u32_default(dev, "reg", 0x800);
    priv->reason_shift = (u32)dev_get_driver_data(dev);

    return 0;
}

static const struct reboot_mode_ops qcom_pon_ops = {
    .set = qcom_pon_set,
};

static const struct udevice_id qcom_pon_ids[] = {
    { .compatible = "qcom,pm8916-pon",   .data = GEN1_REASON_SHIFT },
    { .compatible = "qcom,pm8941-pon",   .data = NO_REASON_SHIFT },
    { .compatible = "qcom,pms405-pon",   .data = GEN1_REASON_SHIFT },
    { .compatible = "qcom,pm8998-pon",   .data = GEN2_REASON_SHIFT },
    { .compatible = "qcom,pmk8350-pon",  .data = GEN2_REASON_SHIFT },
    { .compatible = "qcom,pm6150-pon",   .data = GEN2_REASON_SHIFT },
    { .compatible = "qcom,pm6150l-pon",  .data = GEN2_REASON_SHIFT },
    { }
};

U_BOOT_DRIVER(qcom_pon_reboot_mode) = {
    .name = "qcom_pon_reboot_mode",
    .id = UCLASS_REBOOT_MODE,
    .of_match = qcom_pon_ids,
    .ops = &qcom_pon_ops,
    .probe = qcom_pon_probe,
    .priv_auto = sizeof(struct qcom_pon_priv),
};
