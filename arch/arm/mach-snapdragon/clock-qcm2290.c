// SPDX-License-Identifier: BSD-3-Clause
/*
 * Clock drivers for Qualcomm QCM2290
 *
 * (C) Copyright 2023 Bhupesh Sharma <bhupesh.sharma@linaro.org>
 *
 * Based on Kernel driver, simplified
 */

#include <common.h>
#include <clk-uclass.h>
#include <dm.h>
#include <errno.h>
#include <asm/io.h>
#include <linux/bitops.h>
#include <linux/delay.h>

#include <clk/qcom.h>
#include <dt-bindings/clock/qcom,gcc-qcm2290.h>

/* GPLL clock control registers */
#define GPLL0_STATUS_ACTIVE	BIT(31)

#define GCC_GLOBAL_EN_BASE		(0x84000)
#define GCC_CLOBAL_PERIPHERALS_EN	BIT(5)

#define GCC_APCS_CLOCK_BRANCH_ENA_VOTE_1	(0x7900C)

#define F(f, s, h, m, n) { (f), (s), (2 * (h) - 1), (m), (n) }

struct freq_tbl {
	uint freq;
	uint src;
	u8 pre_div;
	u16 m;
	u16 n;
};

static const struct freq_tbl ftbl_gcc_qupv3_wrap0_s0_clk_src[] = {
	F(7372800, CFG_CLK_SRC_GPLL0_OUT_AUX2, 1, 384, 15625),
	F(14745600, CFG_CLK_SRC_GPLL0_OUT_AUX2, 1, 768, 15625),
	F(19200000, CFG_CLK_SRC_CXO, 1, 0, 0),
	F(29491200, CFG_CLK_SRC_GPLL0_OUT_AUX2, 1, 1536, 15625),
	F(32000000, CFG_CLK_SRC_GPLL0_OUT_AUX2, 1, 8, 75),
	F(48000000, CFG_CLK_SRC_GPLL0_OUT_AUX2, 1, 4, 25),
	F(64000000, CFG_CLK_SRC_GPLL0_OUT_AUX2, 1, 16, 75),
	F(75000000, CFG_CLK_SRC_GPLL0_OUT_AUX2, 4, 0, 0),
	F(80000000, CFG_CLK_SRC_GPLL0_OUT_AUX2, 1, 4, 15),
	F(96000000, CFG_CLK_SRC_GPLL0_OUT_AUX2, 1, 8, 25),
	F(100000000, CFG_CLK_SRC_GPLL0_OUT_AUX2, 3, 0, 0),
	F(102400000, CFG_CLK_SRC_GPLL0_OUT_AUX2, 1, 128, 375),
	F(112000000, CFG_CLK_SRC_GPLL0_OUT_AUX2, 1, 28, 75),
	F(117964800, CFG_CLK_SRC_GPLL0_OUT_AUX2, 1, 6144, 15625),
	F(120000000, CFG_CLK_SRC_GPLL0_OUT_AUX2, 2.5, 0, 0),
	F(128000000, CFG_CLK_SRC_GPLL6_OUT_MAIN, 3, 0, 0),
	{ }
};

static const struct bcr_regs sdcc2_regs = {
	.cfg_rcgr = SDCC2_AHB_CBCR,
	.cmd_rcgr = SDCC2_CMD_RCGR,
	.M = SDCC2_M,
	.N = SDCC2_N,
	.D = SDCC2_D,
};

static const struct bcr_regs uart4_regs = {
	.cfg_rcgr = UART4_APPS_CFG_RCGR,
	.cmd_rcgr = UART4_APPS_CMD_RCGR,
	.M = UART4_APPS_M,
	.N = UART4_APPS_N,
	.D = UART4_APPS_D,
};

static struct pll_vote_clk gpll0_vote_clk = {
	.status = GPLL0_STATUS,
	.status_bit = GPLL0_STATUS_ACTIVE,
	.ena_vote = APCS_GPLL_ENA_VOTE,
	.vote_bit = BIT(0),
};

static struct vote_clk uart4_vote_clk = {
	.cbcr_reg = UART4_APPS_CBCR,
	.ena_vote = GCC_APCS_CLOCK_BRANCH_ENA_VOTE_1,
	.vote_bit = BIT(14),
};

const struct freq_tbl *qcom_find_freq(const struct freq_tbl *f, uint rate)
{
	if (!f)
		return NULL;

	if (!f->freq)
		return f;

	for (; f->freq; f++)
		if (rate <= f->freq)
			return f;

	/* Default to our fastest rate */
	return f - 1;
}

static int clk_init_uart(struct qcom_cc_priv *priv, uint rate)
{
	const struct freq_tbl *freq = qcom_find_freq(ftbl_gcc_qupv3_wrap0_s0_clk_src, rate);

	clk_rcg_set_rate_mnd(priv->base, &uart4_regs,
						freq->pre_div, freq->m, freq->n, freq->src);

	return 0;
}

void clk_enable_vote_uart_clk(phys_addr_t base, const struct vote_clk *vclk)
{
	setbits_le32(base + vclk->ena_vote, vclk->vote_bit);

	udelay(100);
}

static int clk_init_geni_uart(struct qcom_cc_priv *priv, uint rate)
{
	clk_init_uart(priv, rate);

	/* Vote for gpll0 clock */
	clk_enable_gpll0(priv->base, &gpll0_vote_clk);

	/* Enable UART4 clk */
	setbits_le32(priv->base + 0x7900C, BIT(6));
	setbits_le32(priv->base + 0x7900C, BIT(7));
	setbits_le32(priv->base + 0x7900C, BIT(8));
	setbits_le32(priv->base + 0x7900C, BIT(9));
	clk_enable_vote_uart_clk(priv->base, &uart4_vote_clk);

	return 0;
}

ulong msm_set_rate(struct clk *clk, ulong rate)
{
	struct qcom_cc_priv *priv = dev_get_priv(clk->dev);

	switch (clk->id) {
	case GCC_QUPV3_WRAP0_S4_CLK: /* UART4 */
		return clk_init_geni_uart(priv, rate);
	default:
		return 0;
	}

	return 0;
}

int msm_enable(struct clk *clk)
{
	struct qcom_cc_priv *priv = dev_get_priv(clk->dev);

	switch (clk->id) {
	case GCC_SDCC2_APPS_CLK:
		/* SDCC2: 202MHz */
		clk_rcg_set_rate_mnd(priv->base, &sdcc2_regs, 4, 0, 0,
				     CFG_CLK_SRC_GPLL0);
		clk_enable_gpll0(priv->base, &gpll0_vote_clk);
		clk_enable_cbc(priv->base + SDCC2_APPS_CBCR);
		break;
	case GCC_SDCC2_AHB_CLK:
		clk_enable_cbc(priv->base + SDCC2_AHB_CBCR);
		break;
	default:
		return 0;
	}

	return 0;
}

static const struct qcom_reset_map qcm2290_gcc_resets[] = {
	[GCC_CAMSS_OPE_BCR] = { 0x55000 },
	[GCC_CAMSS_TFE_BCR] = { 0x52000 },
	[GCC_CAMSS_TOP_BCR] = { 0x58000 },
	[GCC_GPU_BCR] = { 0x36000 },
	[GCC_MMSS_BCR] = { 0x17000 },
	[GCC_PDM_BCR] = { 0x20000 },
	[GCC_QUPV3_WRAPPER_0_BCR] = { 0x1f000 },
	[GCC_QUSB2PHY_PRIM_BCR] = { 0x1c000 },
	[GCC_SDCC1_BCR] = { 0x38000 },
	[GCC_SDCC2_BCR] = { 0x1e000 },
	[GCC_USB30_PRIM_BCR] = { 0x1a000 },
	[GCC_USB3_PHY_PRIM_SP0_BCR] = { 0x1b000 },
	[GCC_USB3PHY_PHY_PRIM_SP0_BCR] = { 0x1b008 },
	[GCC_USB_PHY_CFG_AHB2PHY_BCR] = { 0x1d000 },
	[GCC_VCODEC0_BCR] = { 0x58094 },
	[GCC_VENUS_BCR] = { 0x58078 },
	[GCC_VIDEO_INTERFACE_BCR] = { 0x6e000 },
};

static const struct qcom_cc_data qcm2290_gcc_data = {
	.resets = qcm2290_gcc_resets,
	.num_resets = ARRAY_SIZE(qcm2290_gcc_resets),
};

static const struct udevice_id gcc_qcm2290_of_match[] = {
	{
		.compatible = "qcom,gcc-qcm2290",
		.data = (ulong)&qcm2290_gcc_data
	},
	{ }
};

U_BOOT_DRIVER(gcc_qcm2290) ={
	.name		= "gcc_qcm2290",
	.id		= UCLASS_NOP,
	.of_match	= gcc_qcm2290_of_match,
	.bind		= qcom_cc_bind,
	.flags		= DM_FLAG_PRE_RELOC,
};
