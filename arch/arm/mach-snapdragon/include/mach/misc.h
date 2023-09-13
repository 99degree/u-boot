/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Snapdragon DRAM
 * Copyright (C) 2018 Ramon Fried <ramon.fried@gmail.com>
 */

#ifndef MISC_H
#define MISC_H

u32 msm_board_serial(void);
void msm_generate_mac_addr(u8 *mac);
void build_mem_map(void);

#endif
