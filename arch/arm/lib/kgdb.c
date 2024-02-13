#include <common.h>
#include <command.h>
#include <dm/uclass.h>
#include <kgdb.h>
#include <asm/ptrace.h>
#include <cpu_func.h>
#include <serial.h>

static struct udevice *kgdb_uart = NULL;

/* Stubbed in kgdb_stubs */
void kgdb_flush_cache_range(void *from, void *to)
{
	printf("%s(%#012llx, %#012llx)\n", __func__, (phys_addr_t)from, (phys_addr_t)to);
	flush_dcache_range((unsigned long)from, (unsigned long)to);
}

void kgdb_flush_cache_all(void)
{
	printf("%s()\n", __func__);
	flush_dcache_all();
}

void kgdb_interruptible(int yes)
{

	return;
}

void serial_dev_setbrg(struct udevice *dev, unsigned int baudrate);
void serial_dev_putc(struct udevice *dev, char ch);
int serial_dev_getc(struct udevice *dev);

void putDebugChar(int c)
{
	serial_dev_putc(kgdb_uart, c);
}

int getDebugChar(void)
{
	return serial_dev_getc(kgdb_uart);
}

static void find_uart(void)
{
	if (kgdb_uart)
		return;

	uclass_foreach_dev_probe(UCLASS_SERIAL, kgdb_uart) {
		if (kgdb_uart != gd->cur_serial_dev) {
			printf("Found second serial device for kgdb!!!\n");
			//serial_dev_setbrg(kgdb_uart, 3000000);
			return;
		}
	}

	printf("Only one serial port found, using debug port for KGDB\n");
	kgdb_uart = gd->cur_serial_dev;
}

/* defined in kgdb_asm.S */
int _kgdb_setjmp(long *buf);
int _kgdb_longjmp(long *buf, int val);

/* address of breakpoint instruction */
void breakinst(void);

int kgdb_setjmp(long *buf)
{
	_kgdb_setjmp(buf);

	printf("%s(%#012llx)\n", __func__, (phys_addr_t)buf);

	return 0;
}

void kgdb_longjmp(long *buf, int val)
{
	printf("%s(%#012llx, %d)\n", __func__, (phys_addr_t)buf, val);

	_kgdb_longjmp(buf, val);
}

void kgdb_enter(struct pt_regs *regs, kgdb_data *kdp)
{
	printf("%s(%#012llx, kdp)\n", __func__, (phys_addr_t)regs);
	//printf("breakinst @ %#012llx\n", (phys_addr_t)breakinst);
	
	find_uart();

	if (regs->elr == (phys_addr_t)breakinst)
		/* Skip over breakpoint instruction */
		regs->elr += 0x8;

	// switch (regs->esr & GENMASK(31, 26)) {
	// case 0b001101: /* Branch Target Exception */
	// 	kdp->sigval = SIGILL;
	// 	break;
	// case 0:
	// default:
	// 	kgp->sigval = SIGHUP;
	// 	break;
	// }

	// kdp->nregs = 1;

	// /* Uhh idek what, we actually get the registers in kgdb_getregs */
	// kdp->regs[0].num = 1;
	// kdp->regs[0].val = regs->regs[30]; /* LR or something */

	show_regs(regs);
}

void kgdb_exit(struct pt_regs *regs, kgdb_data *kdp)
{
	printf("STUB! %s(%#012llx, kdp)\n", __func__, (phys_addr_t)regs);

	show_regs(regs);
}

#define SPACE_REQUIRED	((48*16)+(1*8)+(1*4))

/* https://github.com/qemu/qemu/blob/master/gdb-xml/aarch64-core.xml */
int kgdb_getregs(struct pt_regs *regs, char *buf, int max)
{
	unsigned long *ptr = (unsigned long *)buf;
	int i;

	printf("%s(%#012llx, buf, max)\n", __func__, (phys_addr_t)regs);

	if (max < SPACE_REQUIRED)
		kgdb_error(KGDBERR_NOSPACE);

	if ((unsigned long)ptr & 3)
		kgdb_error(KGDBERR_ALIGNFAULT);

	/* x0-x30 */
	for (i = 0; i < 31; i++)
		*ptr++ = regs->regs[i];

	/* Stack pointer (aka exception link register(?) - efi_print_image_infos()) */
	*ptr++ = regs->regs[30];
	/* Program counter (DUP ?????) */
	*ptr++ = regs->elr;
	/* spsr */
	*ptr++ = regs->spsr;

	//printf("%d bytes\n", (int)((phys_addr_t)ptr - (phys_addr_t)buf));

	//show_regs(regs);

	return SPACE_REQUIRED;
}

void kgdb_putreg(struct pt_regs *regs, int regno, char *buf, int length)
{
	printf("STUB! %s(%#012llx, buf, max)\n", __func__, (phys_addr_t)regs);

	show_regs(regs);
}

void kgdb_putregs(struct pt_regs *regs, char *buf, int length)
{
	printf("STUB! %s(%#012llx, buf, max)\n", __func__, (phys_addr_t)regs);

	show_regs(regs);
}

int kgdb_trap(struct pt_regs *regs)
{
	//show_regs(regs);
	return regs->elr;
}

void kgdb_breakpoint(int argc, char *const argv[])
{
	asm volatile("	.globl breakinst\n\
		      breakinst: .inst	0xe7f000f0\n\
		      ");
}
