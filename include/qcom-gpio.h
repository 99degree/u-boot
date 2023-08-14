/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Qualcomm common pin control data. This is shared
 * between the pinctrl and GPIO drivers.
 * The pinctrl driver is responsible for instantiating
 * the gpio driver.
 *
 * (C) Copyright 2023 Linaro
 *
 */
#ifndef _QCOM_GPIO_H_
#define _QCOM_GPIO_H_

#include <asm/types.h>
#include <common.h>

struct qcom_gpio_priv {
	phys_addr_t base;
	const unsigned int *pin_offsets;
};

/* _DEV_PIN_OFFS must be defined by the user */
#define GPIO_CONFIG_OFFSET(dev, x)	({ \
	const unsigned int *offs = _DEV_PIN_OFFS(dev); \
	unsigned int out = (x * 0x1000); \
	if (offs) out += offs[x]; \
	out; \
})

#define TLMM_GPIO_PULL_MASK		GENMASK(1, 0)
#define TLMM_FUNC_SEL_MASK		GENMASK(5, 2)
#define TLMM_DRV_STRENGTH_MASK		GENMASK(8, 6)
#define TLMM_GPIO_DISABLE		BIT(9)

#define GPIO_IN_OUT_OFF(dev, x)		(GPIO_CONFIG_OFFSET(dev, x) + 0x4)
/* GPIO_IN_OUT register shifts. */
#define GPIO_IN				0
#define GPIO_OUT			1

struct udevice;

/* Fetch the array of pin-bank offsets, indexed by pin. */
const unsigned int* msm_pinctrl_get_offsets(struct udevice *dev, unsigned int *lenp);

/* Check if a pin lies in a reserved range (and should not be touched) */
bool msm_pinctrl_is_reserved(struct udevice *dev, unsigned int pin);

#endif /* _QCOM_GPIO_H_ */
