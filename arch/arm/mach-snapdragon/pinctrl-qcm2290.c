// SPDX-License-Identifier: GPL-2.0+
/*
 * Qualcomm QCM2290 pinctrl
 *
 * (C) Copyright 2023 Bhupesh Sharma <bhupesh.sharma@linaro.org>
 *
 */

#include "pinctrl-snapdragon.h"
#include <common.h>

#define MAX_PIN_NAME_LEN 32
static char pin_name[MAX_PIN_NAME_LEN] __section(".data");

static const char * const msm_pinctrl_pins[] = {
	"SDC1_RCLK",
	"SDC1_CLK",
	"SDC1_CMD",
	"SDC1_DATA",
	"SDC2_CLK",
	"SDC2_CMD",
	"SDC2_DATA",
};

static const struct pinctrl_function msm_pinctrl_functions[] = {
	{"qup4", 1},
	{"gpio", 0},
};

static const char *qcm2290_get_function_name(struct udevice *dev,
					     unsigned int selector)
{
	return msm_pinctrl_functions[selector].name;
}

static const char *qcm2290_get_pin_name(struct udevice *dev,
					unsigned int selector)
{
	if (selector < 127) {
		snprintf(pin_name, MAX_PIN_NAME_LEN, "GPIO_%u", selector);
		return pin_name;
	} else {
		return msm_pinctrl_pins[selector - 127];
	}
}

static unsigned int qcm2290_get_function_mux(unsigned int selector)
{
	return msm_pinctrl_functions[selector].val;
}

struct msm_pinctrl_data qcm2290_data = {
	.pin_count = 134,
	.functions_count = ARRAY_SIZE(msm_pinctrl_functions),
	.get_function_name = qcm2290_get_function_name,
	.get_function_mux = qcm2290_get_function_mux,
	.get_pin_name = qcm2290_get_pin_name,
};
