# SPDX-License-Identifier: GPL-2.0+
#
# (C) Copyright 2015 Google, Inc

obj-$(CONFIG_EFI_APP) += efi_app.o efi.o efi_app_init.o
obj-$(CONFIG_EFI_STUB) += efi_info.o

ifeq ($(CONFIG_ARM64),y)
efi_stub.o := efi_stub_arm64.o
else
efi_stub.o := efi_stub_x86.o

ifeq ($(CONFIG_EFI_STUB_64BIT),y) # && !CONFIG_ARM64
CFLAGS_REMOVE_$(efi_stub.o) := -march=i386 -m32
CFLAGS_$(efi_stub.o) := -m64
CFLAGS_REMOVE_efi.o := -march=i386 -m32
CFLAGS_efi.o := -fpic -m64
endif
endif

CFLAGS_REMOVE_$(efi_stub.o) += -mregparm=3
CFLAGS_$(efi_stub.o) += -fpic -fshort-wchar
CFLAGS_REMOVE_efi.o += -mregparm=3
CFLAGS_efi.o += -fpic -fshort-wchar

$(info removing flags $(CFLAGS_REMOVE_$(efi_stub.o)))
extra-$(CONFIG_EFI_STUB) += $(efi_stub.o) efi.o
