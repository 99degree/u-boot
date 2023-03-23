// SPDX-License-Identifier: BSD-3-Clause
/*
 * Clock drivers for Qualcomm SM6115
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
#include "clock-snapdragon.h"

#include <dt-bindings/clock/qcom,gcc-sm6115.h>

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

static const struct bcr_regs uart4_regs = {
	.cfg_rcgr = UART4_APPS_CFG_RCGR,
	.cmd_rcgr = UART4_APPS_CMD_RCGR,
	.M = UART4_APPS_M,
	.N = UART4_APPS_N,
	.D = UART4_APPS_D,
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

static int clk_init_uart(struct msm_clk_priv *priv, uint rate)
{
	const struct freq_tbl *freq = qcom_find_freq(ftbl_gcc_qupv3_wrap0_s0_clk_src, rate);

	clk_rcg_set_rate_mnd(priv->base, &uart4_regs,
						freq->pre_div, freq->m, freq->n, freq->src);

	return 0;
}

ulong msm_set_rate(struct clk *clk, ulong rate)
{
	struct msm_clk_priv *priv = dev_get_priv(clk->dev);

	switch (clk->id) {
	case GCC_QUPV3_WRAP0_S4_CLK: /* UART4 */
		return clk_init_uart(priv, rate);
	default:
		return 0;
	}

	return 0;
}

int msm_enable(struct clk *clk)
{
	return 0;
}
