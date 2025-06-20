// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2000-2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

/*
 * Misc boot support
 */
#include <command.h>
#include <net.h>
#include <vsprintf.h>
#include <dm/device-internal.h>
#include <dm/uclass.h>
#include <reboot-mode/reboot-mode.h>

#ifdef CONFIG_CMD_GO

/* Allow ports to override the default behavior */
__attribute__((weak))
unsigned long do_go_exec(ulong (*entry)(int, char * const []), int argc,
				 char *const argv[])
{
	return entry (argc, argv);
}

static int do_go(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	ulong	addr, rc;
	int     rcode = 0;

	if (argc < 2)
		return CMD_RET_USAGE;

	addr = hextoul(argv[1], NULL);

	printf ("## Starting application at 0x%08lX ...\n", addr);
	flush();

	/*
	 * pass address parameter as argv[0] (aka command name),
	 * and all remaining args
	 */
	rc = do_go_exec ((void *)addr, argc - 1, argv + 1);
	if (rc != 0) rcode = 1;

	printf ("## Application terminated, rc = 0x%lX\n", rc);
	return rcode;
}

static int do_bootmode(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
        int mode = 0;
        int ret;
        struct udevice *dev;

	if (argc >= 2)
	        mode = hextoul(argv[1], NULL);

	/* force to 2 atm */
	mode = 2;

	for (uclass_first_device(UCLASS_REBOOT_MODE, &dev);
	        dev;
	        uclass_next_device(&dev)) {
			printf("%s %d\n", __func__, __LINE__);

			if (!dev)
				break;

	                ret = dm_reboot_mode_set(dev, mode);
	                if (ret)
	                        break;
	}

	return 0;
}
/* -------------------------------------------------------------------- */

U_BOOT_CMD(
	go, CONFIG_SYS_MAXARGS, 1,	do_go,
	"start application at address 'addr'",
	"addr [arg ...]\n    - start application at address 'addr'\n"
	"      passing 'arg' as arguments"
);

#endif

U_BOOT_CMD(
	reset, 2, 0,	do_reset,
	"Perform RESET of the CPU",
	"- cold boot without level specifier\n"
	"reset -w - warm reset if implemented"
);

U_BOOT_CMD(
        bootmode, 2, 0,    do_bootmode,
        "Perform bootmode of the system\n",
	""
);

#ifdef CONFIG_CMD_POWEROFF
U_BOOT_CMD(
	poweroff, 1, 0,	do_poweroff,
	"Perform POWEROFF of the device",
	""
);
#endif
