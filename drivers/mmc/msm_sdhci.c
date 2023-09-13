// SPDX-License-Identifier: GPL-2.0+
/*
 * Qualcomm SDHCI driver - SD/eMMC controller
 *
 * (C) Copyright 2015 Mateusz Kulikowski <mateusz.kulikowski@gmail.com>
 *
 * Based on Linux driver
 */

#include <common.h>
#include <clk.h>
#include <dm.h>
#include <malloc.h>
#include <sdhci.h>
#include <wait_bit.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <linux/bitops.h>
#include <reset.h>

/* Non-standard registers needed for SDHCI startup */
#define SDCC_MCI_POWER   0x0
#define SDCC_MCI_POWER_SW_RST BIT(7)

/* This is undocumented register */
#define SDCC_MCI_VERSION		0x50
#define SDCC_V5_VERSION			0x318

#define SDCC_VERSION_MAJOR_SHIFT	28
#define SDCC_VERSION_MAJOR_MASK		(0xf << SDCC_VERSION_MAJOR_SHIFT)
#define SDCC_VERSION_MINOR_MASK		0xff

#define SDCC_MCI_STATUS2 0x6C
#define SDCC_MCI_STATUS2_MCI_ACT 0x1
#define SDCC_MCI_HC_MODE 0x78

#define CORE_VENDOR_SPEC_POR_VAL 0xa9c
#define CORE_CLK_PWRSAVE	BIT(1)
#define CORE_HC_MCLK_SEL_DFLT	(2 << 8)
#define CORE_HC_MCLK_SEL_HS400	(3 << 8)
#define CORE_HC_MCLK_SEL_MASK	(3 << 8)
#define CORE_IO_PAD_PWR_SWITCH_EN	BIT(15)
#define CORE_IO_PAD_PWR_SWITCH	BIT(16)
#define CORE_HC_SELECT_IN_EN	BIT(18)
#define CORE_HC_SELECT_IN_HS400	(6 << 19)
#define CORE_HC_SELECT_IN_MASK	(7 << 19)

/* Non standard (?) SDHCI register */
//#define SDHCI_VENDOR_SPEC_CAPABILITIES0  0x11c

struct msm_sdhc_plat {
	struct mmc_config cfg;
	struct mmc mmc;
};

struct msm_sdhc {
	struct sdhci_host host;
	void *base;
	struct reset_ctl reset;
};

struct sdhci_msm_offset {
	u32 core_hc_mode;
	u32 core_mci_data_cnt;
	u32 core_mci_status;
	u32 core_mci_fifo_cnt;
	u32 core_mci_version;
	u32 core_generics;
	u32 core_testbus_config;
	u32 core_testbus_sel2_bit;
	u32 core_testbus_ena;
	u32 core_testbus_sel2;
	u32 core_pwrctl_status;
	u32 core_pwrctl_mask;
	u32 core_pwrctl_clear;
	u32 core_pwrctl_ctl;
	u32 core_sdcc_debug_reg;
	u32 core_dll_config;
	u32 core_dll_status;
	u32 core_vendor_spec;
	u32 core_vendor_spec_adma_err_addr0;
	u32 core_vendor_spec_adma_err_addr1;
	u32 core_vendor_spec_func2;
	u32 core_vendor_spec_capabilities0;
	u32 core_ddr_200_cfg;
	u32 core_vendor_spec3;
	u32 core_dll_config_2;
	u32 core_dll_config_3;
	u32 core_ddr_config_old; /* Applicable to sdcc minor ver < 0x49 */
	u32 core_ddr_config;
	u32 core_dll_usr_ctl; /* Present on SDCC5.1 onwards */
};

static const struct sdhci_msm_offset sdhci_msm_v5_offset = {
	.core_mci_data_cnt = 0x35c,
	.core_mci_status = 0x324,
	.core_mci_fifo_cnt = 0x308,
	.core_mci_version = 0x318,
	.core_generics = 0x320,
	.core_testbus_config = 0x32c,
	.core_testbus_sel2_bit = 3,
	.core_testbus_ena = (1 << 31),
	.core_testbus_sel2 = (1 << 3),
	.core_pwrctl_status = 0x240,
	.core_pwrctl_mask = 0x244,
	.core_pwrctl_clear = 0x248,
	.core_pwrctl_ctl = 0x24c,
	.core_sdcc_debug_reg = 0x358,
	.core_dll_config = 0x200,
	.core_dll_status = 0x208,
	.core_vendor_spec = 0x20c,
	.core_vendor_spec_adma_err_addr0 = 0x214,
	.core_vendor_spec_adma_err_addr1 = 0x218,
	.core_vendor_spec_func2 = 0x210,
	.core_vendor_spec_capabilities0 = 0x21c,
	.core_ddr_200_cfg = 0x224,
	.core_vendor_spec3 = 0x250,
	.core_dll_config_2 = 0x254,
	.core_dll_config_3 = 0x258,
	.core_ddr_config = 0x25c,
	.core_dll_usr_ctl = 0x388,
};

static const struct sdhci_msm_offset sdhci_msm_mci_offset = {
	.core_hc_mode = 0x78,
	.core_mci_data_cnt = 0x30,
	.core_mci_status = 0x34,
	.core_mci_fifo_cnt = 0x44,
	.core_mci_version = 0x050,
	.core_generics = 0x70,
	.core_testbus_config = 0x0cc,
	.core_testbus_sel2_bit = 4,
	.core_testbus_ena = (1 << 3),
	.core_testbus_sel2 = (1 << 4),
	.core_pwrctl_status = 0xdc,
	.core_pwrctl_mask = 0xe0,
	.core_pwrctl_clear = 0xe4,
	.core_pwrctl_ctl = 0xe8,
	.core_sdcc_debug_reg = 0x124,
	.core_dll_config = 0x100,
	.core_dll_status = 0x108,
	.core_vendor_spec = 0x10c,
	.core_vendor_spec_adma_err_addr0 = 0x114,
	.core_vendor_spec_adma_err_addr1 = 0x118,
	.core_vendor_spec_func2 = 0x110,
	.core_vendor_spec_capabilities0 = 0x11c,
	.core_ddr_200_cfg = 0x184,
	.core_vendor_spec3 = 0x1b0,
	.core_dll_config_2 = 0x1b4,
	.core_ddr_config_old = 0x1b8,
	.core_ddr_config = 0x1bc,
};

/*
 * From V5, register spaces have changed. Wrap this info in a structure
 * and choose the data_structure based on version info mentioned in DT.
 */
struct msm_sdhc_variant_info {
	bool mci_removed;
	const struct sdhci_msm_offset *offset;
};

DECLARE_GLOBAL_DATA_PTR;

static int msm_sdc_clk_init(struct udevice *dev)
{
	int node = dev_of_offset(dev);
	uint clk_rate = fdtdec_get_uint(gd->fdt_blob, node, "clock-frequency",
					400000);
	uint clkd[2]; /* clk_id and clk_no */
	int clk_offset;
	struct udevice *clk_dev;
	struct clk clk;
	int ret;

	ret = fdtdec_get_int_array(gd->fdt_blob, node, "clock", clkd, 2);
	if (ret)
		return ret;

	clk_offset = fdt_node_offset_by_phandle(gd->fdt_blob, clkd[0]);
	if (clk_offset < 0)
		return clk_offset;

	ret = uclass_get_device_by_of_offset(UCLASS_CLK, clk_offset, &clk_dev);
	if (ret)
		return ret;

	clk.id = clkd[1];
	ret = clk_request(clk_dev, &clk);
	if (ret < 0)
		return ret;

	ret = clk_set_rate(&clk, clk_rate);
	clk_free(&clk);
	if (ret < 0)
		return ret;

	return 0;
}

static int msm_sdc_mci_init(struct msm_sdhc *prv)
{
	/* Reset the core and Enable SDHC mode */
	writel(readl(prv->base + SDCC_MCI_POWER) | SDCC_MCI_POWER_SW_RST,
	       prv->base + SDCC_MCI_POWER);


	/* Wait for reset to be written to register */
	if (wait_for_bit_le32(prv->base + SDCC_MCI_STATUS2,
			      SDCC_MCI_STATUS2_MCI_ACT, false, 10, false)) {
		printf("msm_sdhci: reset request failed\n");
		return -EIO;
	}

	/* SW reset can take upto 10HCLK + 15MCLK cycles. (min 40us) */
	if (wait_for_bit_le32(prv->base + SDCC_MCI_POWER,
			      SDCC_MCI_POWER_SW_RST, false, 2, false)) {
		printf("msm_sdhci: stuck in reset\n");
		return -ETIMEDOUT;
	}

	/* Enable host-controller mode */
	writel(1, prv->base + SDCC_MCI_HC_MODE);

	return 0;
}

static int msm_sdc_probe(struct udevice *dev)
{
	struct mmc_uclass_priv *upriv = dev_get_uclass_priv(dev);
	struct msm_sdhc_plat *plat = dev_get_plat(dev);
	struct msm_sdhc *prv = dev_get_priv(dev);
	const struct msm_sdhc_variant_info *var_info;
	struct sdhci_host *host = &prv->host;
	u32 host_version, core_version, core_minor, core_major;
	u32 caps;
	int ret;
	u32 val;

	printf("%s: Entering func..\n", __func__);

	ret = reset_get_by_index(dev, 0, &prv->reset);
	if (ret && ret != -ENOENT) {
		printf("%s: Couldn't get reset %d\n", __func__, ret);
		return ret;
	}

	if (prv->reset.dev) {
		printf("%s: Entering func.. has_reset is true..\n", __func__);
		reset_assert(&prv->reset);
		udelay(500);
		reset_deassert(&prv->reset);
		udelay(500);
	}

	host->quirks = SDHCI_QUIRK_WAIT_SEND_CMD | SDHCI_QUIRK_BROKEN_R1B;

	host->max_clk = 0;

	/* Init clocks */
	ret = msm_sdc_clk_init(dev);
	if (ret)
		return ret;

	var_info = (void *)dev_get_driver_data(dev);
	/* Reset the vendor spec register to power on reset state */
	writel(CORE_VENDOR_SPEC_POR_VAL,
			host->ioaddr + var_info->offset->core_vendor_spec);
	if (!var_info->mci_removed) {
		ret = msm_sdc_mci_init(prv);
		if (ret)
			return ret;
	}

	host_version = readw(host->ioaddr + SDHCI_HOST_VERSION);

	if (!var_info->mci_removed)
		core_version = readl(prv->base + SDCC_MCI_VERSION);
	else
		core_version = readl(host->ioaddr + SDCC_V5_VERSION);

	core_major = (core_version & SDCC_VERSION_MAJOR_MASK);
	core_major >>= SDCC_VERSION_MAJOR_SHIFT;

	core_minor = core_version & SDCC_VERSION_MINOR_MASK;

	printf("%s: Host Version: 0x%x Vendor Version 0x%x\n",
		__func__, host_version,
		((host_version & SDHCI_VENDOR_VER_MASK) >> SDHCI_VENDOR_VER_SHIFT));
	printf("%s: MCI Version: 0x%08x, major: 0x%04x, minor: 0x%02x\n",
		__func__, core_version, core_major, core_minor);

	/*
	 * Support for some capabilities is not advertised by newer
	 * controller versions and must be explicitly enabled.
	 */
	if (core_major >= 1 && core_minor != 0x11 && core_minor != 0x12) {
		caps = readl(host->ioaddr + SDHCI_CAPABILITIES);
		caps |= SDHCI_CAN_VDD_300 | SDHCI_CAN_DO_8BIT;
		writel(caps, host->ioaddr + var_info->offset->core_vendor_spec_capabilities0);
	}

	ret = mmc_of_parse(dev, &plat->cfg);
	if (ret)
		return ret;

	host->mmc = &plat->mmc;
	host->mmc->dev = dev;
	ret = sdhci_setup_cfg(&plat->cfg, host, 0, 0);
	if (ret)
		return ret;
	host->mmc->priv = &prv->host;
	upriv->mmc = host->mmc;

	return sdhci_probe(dev);
}

static int msm_sdc_remove(struct udevice *dev)
{
	struct msm_sdhc *priv = dev_get_priv(dev);
	const struct msm_sdhc_variant_info *var_info;

	var_info = (void *)dev_get_driver_data(dev);

	/* Disable host-controller mode */
	if (!var_info->mci_removed)
		writel(0, priv->base + SDCC_MCI_HC_MODE);

	return 0;
}

static int msm_of_to_plat(struct udevice *dev)
{
	struct udevice *parent = dev->parent;
	struct msm_sdhc *priv = dev_get_priv(dev);
	struct sdhci_host *host = &priv->host;
	int node = dev_of_offset(dev);

	host->name = strdup(dev->name);
	host->ioaddr = dev_read_addr_ptr(dev);
	host->bus_width = fdtdec_get_int(gd->fdt_blob, node, "bus-width", 4);
	host->index = fdtdec_get_uint(gd->fdt_blob, node, "index", 0);
	priv->base = (void *)fdtdec_get_addr_size_auto_parent(gd->fdt_blob,
			dev_of_offset(parent), node, "reg", 1, NULL, false);

	if (host->ioaddr == (void *)FDT_ADDR_T_NONE)
		return -EINVAL;

	return 0;
}

static int msm_sdc_bind(struct udevice *dev)
{
	struct msm_sdhc_plat *plat = dev_get_plat(dev);

	return sdhci_bind(dev, &plat->mmc, &plat->cfg);
}

static const struct msm_sdhc_variant_info msm_sdhc_mci_var = {
	.mci_removed = false,
	.offset = &sdhci_msm_mci_offset,
};

static const struct msm_sdhc_variant_info msm_sdhc_v5_var = {
	.mci_removed = true,
	.offset = &sdhci_msm_v5_offset,
};

static const struct udevice_id msm_mmc_ids[] = {
	{ .compatible = "qcom,sdhci-msm-v4", .data = (ulong)&msm_sdhc_mci_var },
	{ .compatible = "qcom,sdhci-msm-v5", .data = (ulong)&msm_sdhc_v5_var },
	{ }
};

U_BOOT_DRIVER(msm_sdc_drv) = {
	.name		= "msm_sdc",
	.id		= UCLASS_MMC,
	.of_match	= msm_mmc_ids,
	.of_to_plat = msm_of_to_plat,
	.ops		= &sdhci_ops,
	.bind		= msm_sdc_bind,
	.probe		= msm_sdc_probe,
	.remove		= msm_sdc_remove,
	.priv_auto	= sizeof(struct msm_sdhc),
	.plat_auto	= sizeof(struct msm_sdhc_plat),
};
