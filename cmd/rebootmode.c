// SPDX-License-Identifier: GPL-2.0+
#include <command.h>
#include <dm.h>
#include <reboot-mode/reboot-mode.h>
#include <linux/errno.h>

static int do_rebootmode(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
    struct udevice *dev;
    u32 mode_id;
    int ret;

    ret = uclass_get_device(UCLASS_REBOOT_MODE, 0, &dev);
    if (ret) {
        printf("No reboot-mode device found (err=%d)\n", ret);
        return CMD_RET_FAILURE;
    }

    const struct reboot_mode_ops *ops = reboot_mode_get_ops(dev);

    if (argc == 1) {
        if (ops && ops->get) {
            ret = ops->get(dev, &mode_id);
            if (!ret)
                printf("Current reboot-mode: %u\n", mode_id);
            else
                printf("Failed to read reboot-mode (err=%d)\n", ret);
        } else {
            printf("Driver does not support reading current mode\n");
        }
        return CMD_RET_SUCCESS;
    }

    if (argc == 2) {
        mode_id = simple_strtoul(argv[1], NULL, 0);
        ret = dm_reboot_mode_set(dev, mode_id);
        if (ret) {
            printf("Failed to set reboot-mode (err=%d)\n", ret);
            return CMD_RET_FAILURE;
        }
        printf("Reboot-mode set to %u\n", mode_id);
        return CMD_RET_SUCCESS;
    }

    return CMD_RET_USAGE;
}

U_BOOT_CMD(
    rebootmode, 2, 0, do_rebootmode,
    "Read or write reboot-mode value",
    "\n"
    "    rebootmode          - read current reboot-mode (if supported)\n"
    "    rebootmode <value>  - set reboot-mode by numeric value\n"
);
