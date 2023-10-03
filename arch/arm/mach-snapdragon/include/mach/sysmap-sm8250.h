/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Qualcomm SDM845 sysmap
 *
 * (C) Copyright 2021 Dzmitry Sankouski <dsankouski@gmail.com>
 */
#ifndef _MACH_SYSMAP_SDM845_H
#define _MACH_SYSMAP_SDM845_H

#define GCC_SE12_UART_RCG_REG (0x184D0)
#define GCC_SDCC2_APPS_CLK_SRC_REG (0x1400c)

#define APCS_GPLL0_ENA_VOTE (0x79000)
#define APCS_GPLL9_STATUS (0x1c000)
#define APCS_GPLLX_ENA_REG (0x52018)

#define USB30_SEC_GDSCR (0x10004)

#endif /* _MACH_SYSMAP_SDM845_H */
