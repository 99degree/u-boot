# SPDX-License-Identifier: GPL-2.0+
#
# (C) Copyright 2023 Linaro

obj-y += clock-snapdragon.o
obj-$(CONFIG_TARGET_QCOM_RBX) += clock-sdm845.o clock-qcm2290.o clock-sm6115.o clock-sm8250.o
obj-$(CONFIG_TARGET_DRAGONBOARD410C) += clock-apq8016.o
obj-$(CONFIG_TARGET_DRAGONBOARD820C) += clock-apq8096.o
obj-$(CONFIG_TARGET_QCS404EVB) += clock-qcs404.o

obj-$(CONFIG_CLK_QCOM_DEBUG) += debugcc/
