/*
 * ARM64 purgatory.
 */
#include <asm-offsets.h>
#include <config.h>
#include <linux/linkage.h>
#include <asm/macro.h>

.macro  size, sym:req
        .size \sym, . - \sym
.endm

.pushsection .text.purgatory_start, "ax"
ENTRY(purgatory_start)
purgatory_start:

//	adr	x19, .Lstack
//	mov	sp, x19

//	bl	purgatory

	/* Start new image. */
	ldr	x17, kernel_entry
	ldr	x0, dtb_addr
	mov	x1, xzr
	mov	x2, xzr
	mov	x3, xzr
	br	x17
ENDPROC(purgatory_start)
.popsection
.align 3

.globl kernel_entry
kernel_entry:
        .quad   0
size kernel_entry

.globl dtb_addr
dtb_addr:
        .quad   0
size dtb_addr
.end
