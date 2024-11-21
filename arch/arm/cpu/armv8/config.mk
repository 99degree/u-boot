# SPDX-License-Identifier: GPL-2.0+
#
# (C) Copyright 2002
# Gary Jennejohn, DENX Software Engineering, <garyj@denx.de>
PLATFORM_RELFLAGS += $(call cc-option,-mbranch-protection=none)

PF_NO_UNALIGNED := $(call cc-option, -mstrict-align)
PLATFORM_CPPFLAGS += $(PF_NO_UNALIGNED)

EFI_LDS := elf_aarch64_efi.lds
EFI_CRT0 := crt0_aarch64_efi.o
EFI_RELOC := reloc_aarch64_efi.o

LDSCRIPT_EFI := $(srctree)/arch/arm/lib/elf_aarch64_efi.lds
EFISTUB := crt0_aarch64_efi.o reloc_aarch64_efi.o
OBJCOPYFLAGS_EFI += --target=pei-aarch64-little
EFIPAYLOAD_BFDTARGET := pei-aarch64-little
EFIPAYLOAD_BFDARCH := aarch64
LDFLAGS_EFI_PAYLOAD := -Bsymbolic -Bsymbolic-functions -shared --no-undefined \
		       -s -zexecstack

CPPFLAGS_REMOVE_crt0-efi-aarch64.o += $(CFLAGS_NON_EFI)
CPPFLAGS_crt0-efi-aarch64.o += $(CFLAGS_EFI)
