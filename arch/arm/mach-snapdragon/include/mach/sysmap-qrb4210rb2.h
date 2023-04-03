/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Qualcomm SM6115 sysmap
 *
 * (C) Copyright 2023 Bhupesh Sharma <bhupesh.sharma@linaro.org>
 */
#ifndef _MACH_SYSMAP_SM6115_H
#define _MACH_SYSMAP_SM6115_H

/* Clocks: (from CLK_CTL_BASE)  */
#define GPLL0_STATUS			(0x0)
#define APCS_GPLL_ENA_VOTE		(0x79000)
#define APCS_CLOCK_BRANCH_ENA_VOTE	(0x79004)

/* Root clock for QUPv3 WRAPPER0 */
#define QUPV3_WRAPPER_0_BCR	(0x1F000)

/* Uart clock control registers */
#define UART4_APPS_CBCR		(0x1F604)
#define UART4_APPS_CMD_RCGR	(0x1F608)
#define UART4_APPS_CFG_RCGR	(0x1F60c)
#define UART4_APPS_M		(0x1F610)
#define UART4_APPS_N		(0x1F614)
#define UART4_APPS_D		(0x1F618)

/* USB controller clock control registers */
#define USB30_PRIM_MASTER_CBCR		(0x1A010)
#define USB30_PRIM_SLEEP_CBCR		(0x1A014)
#define USB30_PRIM_MOCK_UTMI_CBCR	(0x1A018)
#define SYS_NOC_USB3_PRIM_AXI_CBCR	(0x1A080)
#define CFG_NOC_USB3_PRIM_AXI_CBCR	(0x1A084)
#define USB3_PRIM_CLKREF_EN		(0x9F000)
#define AHB2PHY_USB_CBCR		(0x1D008)

#define USB30_PRIM_MASTER_CMD_RCGR	(0x1A01C)
#define USB30_PRIM_MASTER_CFG_RCGR	(0x1A020)
#define USB30_PRIM_MASTER_M		(0x1A024)
#define USB30_PRIM_MASTER_N		(0x1A028)
#define USB30_PRIM_MASTER_D		(0x1A02C)

/* SDCC1 controller clock control registers */
#define SDCC1_BCR		(0x38000)
#define SDCC1_APPS_CBCR		(0x38004)
#define SDCC1_AHB_CBCR		(0x38008)
#define SDCC1_CMD_RCGR		(0x38028)
#define SDCC1_CFG_RCGR		(0x3802c)
#define SDCC1_M			(0x38030)
#define SDCC1_N			(0x38034)
#define SDCC1_D			(0x38038)

/* SDCC2 controller clock control registers */
#define SDCC2_BCR		(0x1E000)
#define SDCC2_APPS_CBCR		(0x1E004)
#define SDCC2_AHB_CBCR		(0x1E008)
#define SDCC2_CMD_RCGR		(0x1E00C)
#define SDCC2_CFG_RCGR		(0x1E010)
#define SDCC2_M			(0x1E014)
#define SDCC2_N			(0x1E018)
#define SDCC2_D			(0x1E01c)

#endif
