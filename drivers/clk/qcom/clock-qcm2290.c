// SPDX-License-Identifier: BSD-3-Clause
/*
 * Clock drivers for Qualcomm qcm2290
 *
 * (C) Copyright 2017 Jorge Ramirez Ortiz <jorge.ramirez-ortiz@linaro.org>
 * (C) Copyright 2021 Dzmitry Sankouski <dsankouski@gmail.com>
 *
 * Based on Little Kernel driver, simplified
 */

#include <common.h>
#include <clk-uclass.h>
#include <dm.h>
#include <linux/delay.h>
#include <errno.h>
#include <asm/io.h>
#include <linux/bug.h>
#include <linux/bitops.h>

#include <clk/qcom.h>
#include <dt-bindings/clock/qcom,gcc-qcm2290.h>

#define F(f, s, h, m, n) { (f), (s), (2 * (h) - 1), (m), (n) }

struct freq_tbl {
	uint freq;
	uint src;
	u8 pre_div;
	u16 m;
	u16 n;
};

static const struct freq_tbl ftbl_gcc_qupv3_wrap0_s0_clk_src[] = {
	F(7372800, CFG_CLK_SRC_GPLL0_AUX2, 1, 384, 15625),
	F(14745600, CFG_CLK_SRC_GPLL0_AUX2, 1, 768, 15625),
	F(19200000, CFG_CLK_SRC_CXO, 1, 0, 0),
	F(29491200, CFG_CLK_SRC_GPLL0_AUX2, 1, 1536, 15625),
	F(32000000, CFG_CLK_SRC_GPLL0_AUX2, 1, 8, 75),
	F(48000000, CFG_CLK_SRC_GPLL0_AUX2, 1, 4, 25),
	F(64000000, CFG_CLK_SRC_GPLL0_AUX2, 1, 16, 75),
	F(75000000, CFG_CLK_SRC_GPLL0_AUX2, 4, 0, 0),
	F(80000000, CFG_CLK_SRC_GPLL0_AUX2, 1, 4, 15),
	F(96000000, CFG_CLK_SRC_GPLL0_AUX2, 1, 8, 25),
	F(100000000, CFG_CLK_SRC_GPLL0_AUX2, 3, 0, 0),
	F(102400000, CFG_CLK_SRC_GPLL0_AUX2, 1, 128, 375),
	F(112000000, CFG_CLK_SRC_GPLL0_AUX2, 1, 28, 75),
	F(117964800, CFG_CLK_SRC_GPLL0_AUX2, 1, 6144, 15625),
	F(120000000, CFG_CLK_SRC_GPLL0_AUX2, 2.5, 0, 0),
	F(128000000, CFG_CLK_SRC_GPLL6, 3, 0, 0),
	{ }
};

static const struct bcr_regs uart4_regs = {
	.cmd_rcgr = 0x1f608,
	.cfg_rcgr = 0x1f608 + RCG_CFG_REG,
	.M = 0x1f608 + RCG_M_REG,
	.N = 0x1f608 + RCG_N_REG,
	.D = 0x1f608 + RCG_D_REG,
};

static const struct freq_tbl ftbl_gcc_sdcc2_apps_clk_src[] = {
	F(400000, CFG_CLK_SRC_CXO, 12, 1, 4),
	F(19200000, CFG_CLK_SRC_CXO, 1, 0, 0),
	F(25000000, CFG_CLK_SRC_GPLL0_AUX2, 12, 0, 0),
	F(50000000, CFG_CLK_SRC_GPLL0_AUX2, 6, 0, 0),
	F(100000000, CFG_CLK_SRC_GPLL0_AUX2, 3, 0, 0),
	F(202000000, CFG_CLK_SRC_GPLL7, 4, 0, 0), // 6.5, 1, 4
	{ }
};

static const struct bcr_regs sdcc2_apps_clk_src = {
	.cmd_rcgr = 0x1e00c,
	.cfg_rcgr = 0x1e00c + RCG_CFG_REG,
	.M = 0x1e00c + RCG_M_REG,
	.N = 0x1e00c + RCG_N_REG,
	.D = 0x1e00c + RCG_D_REG,
};

static const struct pll_vote_clk gpll7_clk = {
	.status = 0x7000,
	.status_bit = BIT(31),
	.ena_vote = 0x79000,
	.vote_bit = BIT(7),
};

static const struct freq_tbl *qcom_find_freq(const struct freq_tbl *f, uint rate)
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

static ulong qcm2290_set_rate(struct clk *clk, ulong rate)
{
	struct qcom_cc_priv *priv = dev_get_priv(clk->dev);
	const struct freq_tbl *freq;

	switch (clk->id) {
	case GCC_QUPV3_WRAP0_S4_CLK: /*UART2*/
		freq = qcom_find_freq(ftbl_gcc_qupv3_wrap0_s0_clk_src, rate);
		clk_rcg_set_rate_mnd(priv->base, &uart4_regs,
						freq->pre_div, freq->m, freq->n, freq->src, 16);
		return 0;
	case GCC_SDCC2_APPS_CLK:
		/* Enable GPLL7 so we can point SDCC2_APPS_CLK_SRC RCG at it */
		clk_enable_gpll0(priv->base, &gpll7_clk);
		freq = qcom_find_freq(ftbl_gcc_sdcc2_apps_clk_src, rate);
		printf("%s: got freq %u\n", __func__, freq->freq);
		WARN(freq->src != CFG_CLK_SRC_GPLL7, "SDCC2_APPS_CLK_SRC not set to GPLL7, requested rate %lu\n", rate);
		clk_rcg_set_rate_mnd(priv->base, &sdcc2_apps_clk_src,
						freq->pre_div, freq->m, freq->n, freq->src, 8);
		return freq->freq;
	default:
		return 0;
	}
}

/* RCG clocks */
#define CMD_REG		0x0
#define CFG_REG		0x4
#define CMD_UPDATE	BIT(0)
#define CMD_ROOT_EN	BIT(1)
#define CMD_DIRTY_CFG	BIT(4)
#define CMD_DIRTY_N	BIT(5)
#define CMD_DIRTY_M	BIT(6)
#define CMD_DIRTY_D	BIT(7)
#define CMD_ROOT_OFF	BIT(31)

static int clk_rcg2_is_enabled(phys_addr_t cmd_rcgr)
{
	u32 cmd = readl(cmd_rcgr + CMD_REG);

	return (cmd & CMD_ROOT_OFF) == 0;
}

/* Hardcoded RCG2 clock registers */
static void init_rcg2_clk(phys_addr_t base, u32 cfg) {
	int count = 0;
	printf("%s: base = %#llx\n", __func__, base);
	setbits_le32(base + CFG_REG, cfg); 
	/* Leave M/N/D all 0 */
	/* Enable clock! */
	setbits_le32(base + CMD_REG, CMD_UPDATE);
	/* Force enable?? */
	setbits_le32(base + CMD_REG, CMD_ROOT_EN);

	//printf("Enabled usb30_sec_master_clk_src\n");

	for (count = 500; count > 0; count--) {
		if (clk_rcg2_is_enabled(base))
			return;

		udelay(1);
	}
}

static const struct simple_clk qcm2290_clks[] = {
	[GCC_AHB2PHY_CSI_CLK]			= SIMPLE_CLK(0x1d004, 0x00000001, "GCC_AHB2PHY_CSI_CLK"),
	[GCC_AHB2PHY_USB_CLK]			= SIMPLE_CLK(0x1d008, 0x00000001, "GCC_AHB2PHY_USB_CLK"),
	[GCC_BIMC_GPU_AXI_CLK]			= SIMPLE_CLK(0x71154, 0x00000001, "GCC_BIMC_GPU_AXI_CLK"),
	[GCC_BOOT_ROM_AHB_CLK]			= SIMPLE_CLK(0x79004, 0x00000400, "GCC_BOOT_ROM_AHB_CLK"),
	[GCC_CAM_THROTTLE_NRT_CLK]		= SIMPLE_CLK(0x79004, 0x08000000, "GCC_CAM_THROTTLE_NRT_CLK"),
	[GCC_CAM_THROTTLE_RT_CLK]		= SIMPLE_CLK(0x79004, 0x04000000, "GCC_CAM_THROTTLE_RT_CLK"),
	[GCC_CAMERA_AHB_CLK]			= SIMPLE_CLK(0x17008, 0x00000001, "GCC_CAMERA_AHB_CLK"),
	[GCC_CAMERA_XO_CLK]			= SIMPLE_CLK(0x17028, 0x00000001, "GCC_CAMERA_XO_CLK"),
	[GCC_CAMSS_AXI_CLK]			= SIMPLE_CLK(0x58044, 0x00000001, "GCC_CAMSS_AXI_CLK"),
	[GCC_CAMSS_CAMNOC_ATB_CLK]		= SIMPLE_CLK(0x5804c, 0x00000001, "GCC_CAMSS_CAMNOC_ATB_CLK"),
	[GCC_CAMSS_CAMNOC_NTS_XO_CLK]		= SIMPLE_CLK(0x58050, 0x00000001, "GCC_CAMSS_CAMNOC_NTS_XO_CLK"),
	[GCC_CAMSS_CCI_0_CLK]			= SIMPLE_CLK(0x56018, 0x00000001, "GCC_CAMSS_CCI_0_CLK"),
	[GCC_CAMSS_CPHY_0_CLK]			= SIMPLE_CLK(0x52088, 0x00000001, "GCC_CAMSS_CPHY_0_CLK"),
	[GCC_CAMSS_CPHY_1_CLK]			= SIMPLE_CLK(0x5208c, 0x00000001, "GCC_CAMSS_CPHY_1_CLK"),
	[GCC_CAMSS_CSI0PHYTIMER_CLK]		= SIMPLE_CLK(0x45018, 0x00000001, "GCC_CAMSS_CSI0PHYTIMER_CLK"),
	[GCC_CAMSS_CSI1PHYTIMER_CLK]		= SIMPLE_CLK(0x45034, 0x00000001, "GCC_CAMSS_CSI1PHYTIMER_CLK"),
	[GCC_CAMSS_MCLK0_CLK]			= SIMPLE_CLK(0x51018, 0x00000001, "GCC_CAMSS_MCLK0_CLK"),
	[GCC_CAMSS_MCLK1_CLK]			= SIMPLE_CLK(0x51034, 0x00000001, "GCC_CAMSS_MCLK1_CLK"),
	[GCC_CAMSS_MCLK2_CLK]			= SIMPLE_CLK(0x51050, 0x00000001, "GCC_CAMSS_MCLK2_CLK"),
	[GCC_CAMSS_MCLK3_CLK]			= SIMPLE_CLK(0x5106c, 0x00000001, "GCC_CAMSS_MCLK3_CLK"),
	[GCC_CAMSS_NRT_AXI_CLK]			= SIMPLE_CLK(0x58054, 0x00000001, "GCC_CAMSS_NRT_AXI_CLK"),
	[GCC_CAMSS_OPE_AHB_CLK]			= SIMPLE_CLK(0x5503c, 0x00000001, "GCC_CAMSS_OPE_AHB_CLK"),
	[GCC_CAMSS_OPE_CLK]			= SIMPLE_CLK(0x5501c, 0x00000001, "GCC_CAMSS_OPE_CLK"),
	[GCC_CAMSS_RT_AXI_CLK]			= SIMPLE_CLK(0x5805c, 0x00000001, "GCC_CAMSS_RT_AXI_CLK"),
	[GCC_CAMSS_TFE_0_CLK]			= SIMPLE_CLK(0x5201c, 0x00000001, "GCC_CAMSS_TFE_0_CLK"),
	[GCC_CAMSS_TFE_0_CPHY_RX_CLK]		= SIMPLE_CLK(0x5207c, 0x00000001, "GCC_CAMSS_TFE_0_CPHY_RX_CLK"),
	[GCC_CAMSS_TFE_0_CSID_CLK]		= SIMPLE_CLK(0x520ac, 0x00000001, "GCC_CAMSS_TFE_0_CSID_CLK"),
	[GCC_CAMSS_TFE_1_CLK]			= SIMPLE_CLK(0x5203c, 0x00000001, "GCC_CAMSS_TFE_1_CLK"),
	[GCC_CAMSS_TFE_1_CPHY_RX_CLK]		= SIMPLE_CLK(0x52080, 0x00000001, "GCC_CAMSS_TFE_1_CPHY_RX_CLK"),
	[GCC_CAMSS_TFE_1_CSID_CLK]		= SIMPLE_CLK(0x520cc, 0x00000001, "GCC_CAMSS_TFE_1_CSID_CLK"),
	[GCC_CAMSS_TOP_AHB_CLK]			= SIMPLE_CLK(0x58028, 0x00000001, "GCC_CAMSS_TOP_AHB_CLK"),
	[GCC_CFG_NOC_USB3_PRIM_AXI_CLK]		= SIMPLE_CLK(0x1a084, 0x00000001, "GCC_CFG_NOC_USB3_PRIM_AXI_CLK"),
	[GCC_DISP_AHB_CLK]			= SIMPLE_CLK(0x1700c, 0x00000001, "GCC_DISP_AHB_CLK"),
	[GCC_DISP_HF_AXI_CLK]			= SIMPLE_CLK(0x17020, 0x00000001, "GCC_DISP_HF_AXI_CLK"),
	[GCC_DISP_THROTTLE_CORE_CLK]		= SIMPLE_CLK(0x7900c, 0x00000020, "GCC_DISP_THROTTLE_CORE_CLK"),
	[GCC_DISP_XO_CLK]			= SIMPLE_CLK(0x1702c, 0x00000001, "GCC_DISP_XO_CLK"),
	[GCC_GP1_CLK]				= SIMPLE_CLK(0x4d000, 0x00000001, "GCC_GP1_CLK"),
	[GCC_GP2_CLK]				= SIMPLE_CLK(0x4e000, 0x00000001, "GCC_GP2_CLK"),
	[GCC_GP3_CLK]				= SIMPLE_CLK(0x4f000, 0x00000001, "GCC_GP3_CLK"),
	[GCC_GPU_CFG_AHB_CLK]			= SIMPLE_CLK(0x36004, 0x00000001, "GCC_GPU_CFG_AHB_CLK"),
	[GCC_GPU_IREF_CLK]			= SIMPLE_CLK(0x36100, 0x00000001, "GCC_GPU_IREF_CLK"),
	[GCC_GPU_MEMNOC_GFX_CLK]		= SIMPLE_CLK(0x3600c, 0x00000001, "GCC_GPU_MEMNOC_GFX_CLK"),
	[GCC_GPU_SNOC_DVM_GFX_CLK]		= SIMPLE_CLK(0x36018, 0x00000001, "GCC_GPU_SNOC_DVM_GFX_CLK"),
	[GCC_GPU_THROTTLE_CORE_CLK]		= SIMPLE_CLK(0x79004, 0x80000000, "GCC_GPU_THROTTLE_CORE_CLK"),
	[GCC_PDM2_CLK]				= SIMPLE_CLK(0x2000c, 0x00000001, "GCC_PDM2_CLK"),
	[GCC_PDM_AHB_CLK]			= SIMPLE_CLK(0x20004, 0x00000001, "GCC_PDM_AHB_CLK"),
	[GCC_PDM_XO4_CLK]			= SIMPLE_CLK(0x20008, 0x00000001, "GCC_PDM_XO4_CLK"),
	[GCC_PWM0_XO512_CLK]			= SIMPLE_CLK(0x2002c, 0x00000001, "GCC_PWM0_XO512_CLK"),
	[GCC_QMIP_CAMERA_NRT_AHB_CLK]		= SIMPLE_CLK(0x7900c, 0x00000001, "GCC_QMIP_CAMERA_NRT_AHB_CLK"),
	[GCC_QMIP_CAMERA_RT_AHB_CLK]		= SIMPLE_CLK(0x7900c, 0x00000004, "GCC_QMIP_CAMERA_RT_AHB_CLK"),
	[GCC_QMIP_DISP_AHB_CLK]			= SIMPLE_CLK(0x7900c, 0x00000002, "GCC_QMIP_DISP_AHB_CLK"),
	[GCC_QMIP_GPU_CFG_AHB_CLK]		= SIMPLE_CLK(0x7900c, 0x00000010, "GCC_QMIP_GPU_CFG_AHB_CLK"),
	[GCC_QMIP_VIDEO_VCODEC_AHB_CLK]		= SIMPLE_CLK(0x79004, 0x02000000, "GCC_QMIP_VIDEO_VCODEC_AHB_CLK"),
	[GCC_QUPV3_WRAP0_CORE_2X_CLK]		= SIMPLE_CLK(0x7900c, 0x00000200, "GCC_QUPV3_WRAP0_CORE_2X_CLK"),
	[GCC_QUPV3_WRAP0_CORE_CLK]		= SIMPLE_CLK(0x7900c, 0x00000100, "GCC_QUPV3_WRAP0_CORE_CLK"),
	[GCC_QUPV3_WRAP0_S0_CLK]		= SIMPLE_CLK(0x7900c, 0x00000400, "GCC_QUPV3_WRAP0_S0_CLK"),
	[GCC_QUPV3_WRAP0_S1_CLK]		= SIMPLE_CLK(0x7900c, 0x00000800, "GCC_QUPV3_WRAP0_S1_CLK"),
	[GCC_QUPV3_WRAP0_S2_CLK]		= SIMPLE_CLK(0x7900c, 0x00001000, "GCC_QUPV3_WRAP0_S2_CLK"),
	[GCC_QUPV3_WRAP0_S3_CLK]		= SIMPLE_CLK(0x7900c, 0x00002000, "GCC_QUPV3_WRAP0_S3_CLK"),
	[GCC_QUPV3_WRAP0_S4_CLK]		= SIMPLE_CLK(0x7900c, 0x00004000, "GCC_QUPV3_WRAP0_S4_CLK"),
	[GCC_QUPV3_WRAP0_S5_CLK]		= SIMPLE_CLK(0x7900c, 0x00008000, "GCC_QUPV3_WRAP0_S5_CLK"),
	[GCC_QUPV3_WRAP_0_M_AHB_CLK]		= SIMPLE_CLK(0x7900c, 0x00000040, "GCC_QUPV3_WRAP_0_M_AHB_CLK"),
	[GCC_QUPV3_WRAP_0_S_AHB_CLK]		= SIMPLE_CLK(0x7900c, 0x00000080, "GCC_QUPV3_WRAP_0_S_AHB_CLK"),
	[GCC_SDCC1_AHB_CLK]			= SIMPLE_CLK(0x38008, 0x00000001, "GCC_SDCC1_AHB_CLK"),
	[GCC_SDCC1_APPS_CLK]			= SIMPLE_CLK(0x38004, 0x00000001, "GCC_SDCC1_APPS_CLK"),
	[GCC_SDCC1_ICE_CORE_CLK]		= SIMPLE_CLK(0x3800c, 0x00000001, "GCC_SDCC1_ICE_CORE_CLK"),
	[GCC_SDCC2_AHB_CLK]			= SIMPLE_CLK(0x1e008, 0x00000001, "GCC_SDCC2_AHB_CLK"),
	[GCC_SDCC2_APPS_CLK]			= SIMPLE_CLK(0x1e004, 0x00000001, "GCC_SDCC2_APPS_CLK"),
	[GCC_SYS_NOC_CPUSS_AHB_CLK]		= SIMPLE_CLK(0x79004, 0x00000001, "GCC_SYS_NOC_CPUSS_AHB_CLK"),
	[GCC_SYS_NOC_USB3_PRIM_AXI_CLK]		= SIMPLE_CLK(0x1a080, 0x00000001, "GCC_SYS_NOC_USB3_PRIM_AXI_CLK"),
	[GCC_USB30_PRIM_MASTER_CLK]		= SIMPLE_CLK(0x1a010, 0x00000001, "GCC_USB30_PRIM_MASTER_CLK"),
	[GCC_USB30_PRIM_MOCK_UTMI_CLK]		= SIMPLE_CLK(0x1a018, 0x00000001, "GCC_USB30_PRIM_MOCK_UTMI_CLK"),
	[GCC_USB30_PRIM_SLEEP_CLK]		= SIMPLE_CLK(0x1a014, 0x00000001, "GCC_USB30_PRIM_SLEEP_CLK"),
	[GCC_USB3_PRIM_CLKREF_CLK]		= SIMPLE_CLK(0x9f000, 0x00000001, "GCC_USB3_PRIM_CLKREF_CLK"),
	[GCC_USB3_PRIM_PHY_COM_AUX_CLK]		= SIMPLE_CLK(0x1a054, 0x00000001, "GCC_USB3_PRIM_PHY_COM_AUX_CLK"),
	[GCC_USB3_PRIM_PHY_PIPE_CLK]		= SIMPLE_CLK(0x1a058, 0x00000001, "GCC_USB3_PRIM_PHY_PIPE_CLK"),
	[GCC_VCODEC0_AXI_CLK]			= SIMPLE_CLK(0x6e008, 0x00000001, "GCC_VCODEC0_AXI_CLK"),
	[GCC_VENUS_AHB_CLK]			= SIMPLE_CLK(0x6e010, 0x00000001, "GCC_VENUS_AHB_CLK"),
	[GCC_VENUS_CTL_AXI_CLK]			= SIMPLE_CLK(0x6e004, 0x00000001, "GCC_VENUS_CTL_AXI_CLK"),
	[GCC_VIDEO_AHB_CLK]			= SIMPLE_CLK(0x17004, 0x00000001, "GCC_VIDEO_AHB_CLK"),
	[GCC_VIDEO_AXI0_CLK]			= SIMPLE_CLK(0x1701c, 0x00000001, "GCC_VIDEO_AXI0_CLK"),
	[GCC_VIDEO_THROTTLE_CORE_CLK]		= SIMPLE_CLK(0x79004, 0x10000000, "GCC_VIDEO_THROTTLE_CORE_CLK"),
	[GCC_VIDEO_VCODEC0_SYS_CLK]		= SIMPLE_CLK(0x580a4, 0x00000001, "GCC_VIDEO_VCODEC0_SYS_CLK"),
	[GCC_VIDEO_VENUS_CTL_CLK]		= SIMPLE_CLK(0x5808c, 0x00000001, "GCC_VIDEO_VENUS_CTL_CLK"),
	[GCC_VIDEO_XO_CLK]			= SIMPLE_CLK(0x17024, 0x00000001, "GCC_VIDEO_XO_CLK"),
};

#define USB30_PRIM_GDSCR 0x1a004

static int qcm2290_enable(struct clk *clk)
{
	struct qcom_cc_priv *priv = dev_get_priv(clk->dev);

	if (priv->data->num_clks < clk->id) {
		debug("%s: unknown clk id %lu\n", __func__, clk->id);
		return 0;
	}

	debug("%s: clk %s\n", __func__, qcm2290_clks[clk->id].name);

	switch (clk->id) {
	case GCC_USB30_PRIM_MASTER_CLK:
		gdsc_enable(priv->base + USB30_PRIM_GDSCR);
		init_rcg2_clk(priv->base + 0x1a060, 0x105); // gcc_usb3_prim_phy_aux_clk_src /* SRC 0x100 (CFG_CLK_SRC_GPLL0) + DIV 5 */
		clk_enable_simple(priv, GCC_USB3_PRIM_PHY_COM_AUX_CLK);

		clk_enable_simple(priv, GCC_USB3_PRIM_CLKREF_CLK);

		init_rcg2_clk(priv->base + 0x1a034, 1);
		break;
	}

	clk_enable_simple(priv, clk->id);

	return 0;
}

static const struct qcom_reset_map qcm2290_gcc_resets[] = {
	[GCC_CAMSS_OPE_BCR] = { 0x55000, "GCC_CAMSS_OPE_BCR" },
	[GCC_CAMSS_TFE_BCR] = { 0x52000, "GCC_CAMSS_TFE_BCR" },
	[GCC_CAMSS_TOP_BCR] = { 0x58000, "GCC_CAMSS_TOP_BCR" },
	[GCC_GPU_BCR] = { 0x36000, "GCC_GPU_BCR" },
	[GCC_MMSS_BCR] = { 0x17000, "GCC_MMSS_BCR" },
	[GCC_PDM_BCR] = { 0x20000, "GCC_PDM_BCR" },
	[GCC_QUPV3_WRAPPER_0_BCR] = { 0x1f000, "GCC_QUPV3_WRAPPER_0_BCR" },
	[GCC_QUSB2PHY_PRIM_BCR] = { 0x1c000, "GCC_QUSB2PHY_PRIM_BCR" },
	[GCC_SDCC1_BCR] = { 0x38000, "GCC_SDCC1_BCR" },
	[GCC_SDCC2_BCR] = { 0x1e000, "GCC_SDCC2_BCR" },
	[GCC_USB30_PRIM_BCR] = { 0x1a000, "GCC_USB30_PRIM_BCR" },
	[GCC_USB3_PHY_PRIM_SP0_BCR] = { 0x1b000, "GCC_USB3_PHY_PRIM_SP0_BCR" },
	[GCC_USB3PHY_PHY_PRIM_SP0_BCR] = { 0x1b008, "GCC_USB3PHY_PHY_PRIM_SP0_BCR" },
	[GCC_USB_PHY_CFG_AHB2PHY_BCR] = { 0x1d000, "GCC_USB_PHY_CFG_AHB2PHY_BCR" },
	[GCC_VCODEC0_BCR] = { 0x58094, "GCC_VCODEC0_BCR" },
	[GCC_VENUS_BCR] = { 0x58078, "GCC_VENUS_BCR" },
	[GCC_VIDEO_INTERFACE_BCR] = { 0x6e000, "GCC_VIDEO_INTERFACE_BCR" },
};

static struct qcom_cc_data qcm2290_gcc_data = {
	.resets = qcm2290_gcc_resets,
	.num_resets = ARRAY_SIZE(qcm2290_gcc_resets),
	.clks = qcm2290_clks,
	.num_clks = ARRAY_SIZE(qcm2290_clks),

	.enable = qcm2290_enable,
	.set_rate = qcm2290_set_rate,
};


static const struct udevice_id gcc_qcm2290_of_match[] = {
	{
		.compatible = "qcom,gcc-qcm2290",
		.data = (ulong)&qcm2290_gcc_data,
	},
	{ }
};

U_BOOT_DRIVER(gcc_qcm2290) = {
	.name		= "gcc_qcm2290",
	.id		= UCLASS_NOP,
	.of_match	= gcc_qcm2290_of_match,
	.bind		= qcom_cc_bind,
	.flags		= DM_FLAG_PRE_RELOC,
};
