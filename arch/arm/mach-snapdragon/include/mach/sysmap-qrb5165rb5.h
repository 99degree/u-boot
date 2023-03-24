/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Qualcomm SM8250 sysmap
 *
 * (C) Copyright 2023 Bhupesh Sharma <bhupesh.sharma@linaro.org>
 */
#ifndef _MACH_SYSMAP_SM8250_H
#define _MACH_SYSMAP_SM8250_H

/* Clocks: (from CLK_CTL_BASE)  */
#define GPLL0_STATUS			(0x0)
#define APCS_GPLL_ENA_VOTE		(0x79000)
#define APCS_CLOCK_BRANCH_ENA_VOTE	(0x79004)

/* Uart clock control registers */
#define DEBUG_UART_APPS_CBCR		(0x1726C)
#define DEBUG_UART_APPS_CMD_RCGR	(0x17270)
#define DEBUG_UART_APPS_CFG_RCGR	(0x17274)
#define DEBUG_UART_APPS_M		(0x17278)
#define DEBUG_UART_APPS_N		(0x1727C)
#define DEBUG_UART_APPS_D		(0x17280)

#endif
