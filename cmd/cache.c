// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

/*
 * Cache support: switch on or off, get status
 */
#include <command.h>
#include <cpu_func.h>
#include <dm.h>
#include <dm/uclass.h>
#include <iommu.h>
#include <linux/compiler.h>
#include <asm/armv8/mmu.h>

DECLARE_GLOBAL_DATA_PTR;

static int parse_argv(const char *);

static int do_icache(struct cmd_tbl *cmdtp, int flag, int argc,
		     char *const argv[])
{
	switch (argc) {
	case 2:			/* on / off / flush */
		switch (parse_argv(argv[1])) {
		case 0:
			icache_disable();
			break;
		case 1:
			icache_enable();
			break;
		case 2:
			invalidate_icache_all();
			break;
		default:
			return CMD_RET_USAGE;
		}
		break;
	case 1:			/* get status */
		printf("Instruction Cache is %s\n",
			icache_status() ? "ON" : "OFF");
		return 0;
	default:
		return CMD_RET_USAGE;
	}
	return 0;
}

static int do_dcache(struct cmd_tbl *cmdtp, int flag, int argc,
		     char *const argv[])
{
	switch (argc) {
	case 2:			/* on / off / flush */
		switch (parse_argv(argv[1])) {
		case 0:
			dcache_disable();
			break;
		case 1:
			dcache_enable();
#ifdef CONFIG_SYS_NONCACHED_MEMORY
			noncached_set_region();
#endif
			break;
		case 2:
			flush_dcache_all();
			break;
		default:
			return CMD_RET_USAGE;
		}
		break;
	case 1:			/* get status */
		printf("Data (writethrough) Cache is %s\n",
			dcache_status() ? "ON" : "OFF");
		return 0;
	default:
		return CMD_RET_USAGE;
	}
	return 0;
}

static int do_smmu(struct cmd_tbl *cmdtp, int flag, int argc,
		  char *const argv[])
{
	struct udevice *dev;
	struct uclass *uc;
	const struct iommu_ops *ops;

	uclass_id_foreach_dev(UCLASS_IOMMU, dev, uc) {
		if (!device_active(dev))
			continue;
		printf("IOMMU device %s\n", dev->name);
		ops = device_get_ops(dev);
		if (ops && ops->dump)
			ops->dump(dev);
	}

	dump_pagetable(gd->arch.tlb_addr, get_tcr(NULL, NULL));

	return 0;
}

static int parse_argv(const char *s)
{
	if (strcmp(s, "flush") == 0)
		return 2;
	else if (strcmp(s, "on") == 0)
		return 1;
	else if (strcmp(s, "off") == 0)
		return 0;

	return -1;
}

U_BOOT_CMD(
	icache,   2,   1,     do_icache,
	"enable or disable instruction cache",
	"[on, off, flush]\n"
	"    - enable, disable, or flush instruction cache"
);

U_BOOT_CMD(
	dcache,   2,   1,     do_dcache,
	"enable or disable data cache",
	"[on, off, flush]\n"
	"    - enable, disable, or flush data (writethrough) cache"
);

U_BOOT_CMD(
	iommu,   1,   1,     do_smmu,
	"dump SMMU context banks",
	""
);
