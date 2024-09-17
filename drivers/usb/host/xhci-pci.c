// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2015, Google, Inc
 * Written by Simon Glass <sjg@chromium.org>
 * All rights reserved.
 */

#define LOG_DEBUG

#include <dm.h>
#include <dm/device_compat.h>
#include <init.h>
#include <log.h>
#include <pci.h>
#include <reset.h>
#include <usb.h>
#include <usb/xhci.h>

#define RENESAS_FW_VERSION				0x6C
#define RENESAS_ROM_CONFIG				0xF0
#define RENESAS_FW_STATUS				0xF4
#define RENESAS_FW_STATUS_MSB				0xF5
#define RENESAS_ROM_STATUS				0xF6
#define RENESAS_ROM_STATUS_MSB				0xF7
#define RENESAS_DATA0					0xF8
#define RENESAS_DATA1					0xFC

#define RENESAS_ROM_STATUS_ACCESS			BIT(0)
#define RENESAS_ROM_STATUS_ERASE			BIT(1)
#define RENESAS_ROM_STATUS_RELOAD			BIT(2)
#define RENESAS_ROM_STATUS_RESULT			GENMASK(6, 4)
  #define RENESAS_ROM_STATUS_NO_RESULT			0
  #define RENESAS_ROM_STATUS_SUCCESS			BIT(4)
  #define RENESAS_ROM_STATUS_ERROR			BIT(5)
#define RENESAS_ROM_STATUS_SET_DATA0			BIT(8)
#define RENESAS_ROM_STATUS_SET_DATA1			BIT(9)
#define RENESAS_ROM_STATUS_ROM_EXISTS			BIT(15)

struct xhci_pci_plat {
	struct reset_ctl reset;
};

static int xhci_pci_init(struct udevice *dev, struct xhci_hccr **ret_hccr,
			 struct xhci_hcor **ret_hcor)
{
	struct xhci_hccr *hccr;
	struct xhci_hcor *hcor;
	u32 cmd;

	hccr = (struct xhci_hccr *)dm_pci_map_bar(dev,
			PCI_BASE_ADDRESS_0, 0, 0x10000000,
			PCI_REGION_TYPE, PCI_REGION_MEM);
	if (!hccr) {
		printf("xhci-pci init cannot map PCI mem bar\n");
		return -EIO;
	}

	hcor = (struct xhci_hcor *)((uintptr_t) hccr +
			HC_LENGTH(xhci_readl(&hccr->cr_capbase)));

	debug("XHCI-PCI init hccr %p and hcor %p hc_length %d\n",
	      hccr, hcor, (u32)HC_LENGTH(xhci_readl(&hccr->cr_capbase)));

	*ret_hccr = hccr;
	*ret_hcor = hcor;

	/* enable busmaster */
	dm_pci_read_config32(dev, PCI_COMMAND, &cmd);
	cmd |= PCI_COMMAND_MASTER;
	dm_pci_write_config32(dev, PCI_COMMAND, cmd);
	return 0;
}

static int xhci_pci_probe(struct udevice *dev)
{
	struct xhci_pci_plat *plat = dev_get_plat(dev);
	struct xhci_hccr *hccr;
	struct xhci_hcor *hcor;
	ofnode node;
	u16 rom_status;
	int ret;

	node = dev_ofnode(dev);

	printf("%s: %s, %s\n", __func__, dev->name, ofnode_valid(node) ? ofnode_get_name(node) : "NO node");

	ret = reset_get_by_index(dev, 0, &plat->reset);
	if (ret && ret != -ENOENT && ret != -ENOTSUPP) {
		dev_err(dev, "failed to get reset\n");
		return ret;
	}

	if (reset_valid(&plat->reset)) {
		ret = reset_assert(&plat->reset);
		if (ret)
			goto err_reset;

		ret = reset_deassert(&plat->reset);
		if (ret)
			goto err_reset;
	}

	ret = dm_pci_read_config16(dev, RENESAS_ROM_STATUS, &rom_status);
	if (ret) {
		dev_err(dev, "failed to read ROM status\n");
		return ret;
	}
	if (rom_status != 0x0000) {
		dev_err(dev, "ROM status is not 0x0000\n");
		return -ENODEV;
	}

	dev_info(dev, "ROM status: 0x%04x\n", rom_status);
	rom_status &= RENESAS_ROM_STATUS_ROM_EXISTS;
	if (rom_status) {
		dev_info(dev, "External ROM exists\n");
		return true; /* External ROM exists */
	}

	ret = xhci_pci_init(dev, &hccr, &hcor);
	if (ret)
		goto err_reset;

	ret = xhci_register(dev, hccr, hcor);
	if (ret)
		goto err_reset;

	return 0;

err_reset:
	if (reset_valid(&plat->reset))
		reset_free(&plat->reset);

	return ret;
}

static int xhci_pci_remove(struct udevice *dev)
{
	struct xhci_pci_plat *plat = dev_get_plat(dev);

	xhci_deregister(dev);
	if (reset_valid(&plat->reset))
		reset_free(&plat->reset);

	return 0;
}

static const struct udevice_id xhci_pci_ids[] = {
	{ .compatible = "xhci-pci" },
	{ }
};

U_BOOT_DRIVER(xhci_pci) = {
	.name	= "xhci_pci",
	.id	= UCLASS_USB,
	.probe = xhci_pci_probe,
	.remove	= xhci_pci_remove,
	.of_match = xhci_pci_ids,
	.ops	= &xhci_usb_ops,
	.plat_auto	= sizeof(struct xhci_pci_plat),
	.priv_auto	= sizeof(struct xhci_ctrl),
	.flags	= DM_FLAG_OS_PREPARE | DM_FLAG_ALLOC_PRIV_DMA,
};

static struct pci_device_id xhci_pci_supported[] = {
	{ PCI_DEVICE_CLASS(PCI_CLASS_SERIAL_USB_XHCI, ~0) },
	{},
};

U_BOOT_PCI_DEVICE(xhci_pci, xhci_pci_supported);
