// SPDX-License-Identifier: GPL-2.0+
/*
 * Qualcomm sc7280 pinctrl
 *
 * (C) Copyright 2024 Linaro Ltd.
 *
 */

#include <dm.h>

#include "pinctrl-qcom.h"

#define MAX_PIN_NAME_LEN 32
static char pin_name[MAX_PIN_NAME_LEN] __section(".data");

static const struct pinctrl_function msm_pinctrl_functions[] = {
	{"gpio", 0},
};

static const char *sc7280_get_function_name(struct udevice *dev,
						 unsigned int selector)
{
	return msm_pinctrl_functions[selector].name;
}

static const char *sc7280_get_pin_name(struct udevice *dev,
					unsigned int selector)
{
	snprintf(pin_name, MAX_PIN_NAME_LEN, "gpio%u", selector);

	return pin_name;
}

static unsigned int sc7280_get_function_mux(__maybe_unused unsigned int pin,
					    unsigned int selector)
{
	return msm_pinctrl_functions[selector].val;
}

static struct msm_pinctrl_data sc7280_data = {
	.pin_data = {
		.pin_count = 182,
		.special_pins_start = 175,
	},
	.functions_count = ARRAY_SIZE(msm_pinctrl_functions),
	.get_function_name = sc7280_get_function_name,
	.get_function_mux = sc7280_get_function_mux,
	.get_pin_name = sc7280_get_pin_name,
};

static const struct udevice_id msm_pinctrl_ids[] = {
	{ .compatible = "qcom,sc7280-pinctrl", .data = (ulong)&sc7280_data },
	{ /* Sentinel */ }
};

U_BOOT_DRIVER(pinctrl_sc7280) = {
	.name		= "pinctrl_sc7280",
	.id		= UCLASS_NOP,
	.of_match	= msm_pinctrl_ids,
	.ops		= &msm_pinctrl_ops,
	.bind		= msm_pinctrl_bind,
};

