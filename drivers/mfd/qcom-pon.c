// SPDX-License-Identifier: GPL-2.0+
#include <dm.h>
#include <dm/device-internal.h>    // device_bind_with_driver_data()
#include <dm/lists.h>              // lists_driver_lookup_name()
#include <linux/bitops.h>

#define GEN1_REASON_SHIFT 2
#define GEN2_REASON_SHIFT 1
#define NO_REASON_SHIFT   0

/*
 * This driver owns pon@800 via .of_match and spawns children:
 * - qcom_pon_reboot_mode (same ofnode, UCLASS_REBOOT_MODE)
 * - qcom_pwrkey for each button subnode (pwrkey/resin, UCLASS_BUTTON)
 */
static const struct udevice_id qcom_pon_mfd_ids[] = {
    { .compatible = "qcom,pm8916-pon",  .data = (ulong)GEN1_REASON_SHIFT },
    { .compatible = "qcom,pm8941-pon",  .data = (ulong)NO_REASON_SHIFT   },
    { .compatible = "qcom,pm8998-pon",  .data = (ulong)GEN2_REASON_SHIFT },
    { .compatible = "qcom,pmk8350-pon", .data = (ulong)GEN2_REASON_SHIFT },
    { .compatible = "qcom,pm6150-pon",  .data = (ulong)GEN2_REASON_SHIFT },
    { .compatible = "qcom,pm6150l-pon", .data = (ulong)GEN2_REASON_SHIFT },
    { }
};

static int qcom_pon_mfd_bind(struct udevice *dev)
{
    struct udevice *child;
    const struct driver *drv;
    ofnode node;

    /* Fetch .data from our of_match entry */
    u32 reason_shift = (u32)dev_get_driver_data(dev);

    /* Bind reboot-mode child (isolated uclass, same ofnode as parent) */
    drv = lists_driver_lookup_name("qcom_pon_reboot_mode");
    if (drv) {
        int ret = device_bind_with_driver_data(dev, drv, "qcom_pon_reboot_mode",
                                               reason_shift, dev_ofnode(dev), &child);
        if (ret)
            return ret;
    }

    /* Enumerate button subnodes (e.g., pwrkey/resin) and bind each */
    dev_for_each_subnode(node, dev) {
        if (!ofnode_is_enabled(node))
            continue;

        if (ofnode_device_is_compatible(node, "qcom,pm8941-pwrkey") ||
            ofnode_device_is_compatible(node, "qcom,pm8941-resin")  ||
            ofnode_device_is_compatible(node, "qcom,pm8998-pwrkey") ||
            ofnode_device_is_compatible(node, "qcom,pm8998-resin")  ||
            ofnode_device_is_compatible(node, "qcom,pmk8350-pwrkey")||
            ofnode_device_is_compatible(node, "qcom,pmk8350-resin")) {

            drv = lists_driver_lookup_name("qcom_pwrkey");
            if (!drv)
                continue;

            int ret = device_bind_with_driver_data(dev, drv,
                                                   ofnode_get_name(node),
                                                   0, node, &child);
            if (ret)
                return ret;
        }
    }

    return 0;
}

U_BOOT_DRIVER(qcom_pon_mfd) = {
    .name     = "qcom_pon_mfd",
    .id       = UCLASS_MISC,
    .of_match = qcom_pon_mfd_ids,
    .bind     = qcom_pon_mfd_bind,
};
