/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2024 Linaro Ltd.
 *  author: Caleb Connolly <caleb.connolly@linaro.org>
 */

#include <asm-offsets.h>
#include <config.h>
#include <asm/ptrace.h>
#include <asm/macro.h>
#include <linux/linkage.h>

/* Save Lr and other registers to buf so we can jump back
 * here later... */
.globl _kgdb_setjmp
_kgdb_setjmp: /* x0 = buf */
	mov x29, lr
	str x29, [x0]
	stp x2,  x3, [x0, #8]
	str x1,  [x0, #24]

/* Restore state from buffer, put val in x4(??) and read lr */
.globl _kgdb_longjmp
_kgdb_longjmp: /* x0 = buf, x1 = val */
	mov x4 , x1
	ldr x1 , [x0, #24]
	ldp x2 , x3, [x0, #8]
	ldr x29, [x0]
	mov lr, x29
