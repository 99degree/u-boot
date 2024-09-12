// SPDX-License-Identifier: GPL-2.0+
/*
 * Qualcomm QPS615 PCIe bridge driver
 *
 */

#define pr_fmt(fmt) "qps615: " fmt

#include <dm.h>
#include <dm/device.h>
#include <i2c.h>
#include <dm/of.h>
#include <linux/delay.h>
#include <power/regulator.h>
#include <asm-generic/gpio.h>

#define QPS615_GPIO_CONFIG		0x801208
#define QPS615_RESET_GPIO		0x801210

#define QPS615_BUS_CONTROL		0x801014

#define QPS615_PORT_L0S_DELAY		0x82496c
#define QPS615_PORT_L1_DELAY		0x824970

#define QPS615_EMBEDDED_ETH_DELAY	0x8200d8
#define QPS615_ETH_L1_DELAY_MASK	GENMASK(27, 18)
#define QPS615_ETH_L1_DELAY_VALUE(x)	FIELD_PREP(QPS615_ETH_L1_DELAY_MASK, x)
#define QPS615_ETH_L0S_DELAY_MASK	GENMASK(17, 13)
#define QPS615_ETH_L0S_DELAY_VALUE(x)	FIELD_PREP(QPS615_ETH_L0S_DELAY_MASK, x)

#define QPS615_NFTS_2_5_GT		0x824978
#define QPS615_NFTS_5_GT		0x82497c

#define QPS615_PORT_LANE_ACCESS_ENABLE	0x828000

#define QPS615_PHY_RATE_CHANGE_OVERRIDE	0x828040
#define QPS615_PHY_RATE_CHANGE		0x828050

#define QPS615_TX_MARGIN		0x828234

#define QPS615_DFE_ENABLE		0x828a04
#define QPS615_DFE_EQ0_MODE		0x828a08
#define QPS615_DFE_EQ1_MODE		0x828a0c
#define QPS615_DFE_EQ2_MODE		0x828a14
#define QPS615_DFE_PD_MASK		0x828254

#define QPS615_PORT_SELECT		0x82c02c
#define QPS615_PORT_ACCESS_ENABLE	0x82c030

#define QPS615_POWER_CONTROL		0x82b09c
#define QPS615_POWER_CONTROL_OVREN	0x82b2c8

#define QPS615_AXI_CLK_FREQ_MHZ		125

#define N_VREGS 6

static const char *vregs[] = {
	"vddc-supply",
	"vdd18-supply",
	"vdd09-supply",
	"vddio1-supply",
	"vddio2-supply",
	"vddio18-supply",
};

struct qps615 {
	struct udevice *i2c;
	struct udevice *vregs[N_VREGS];
	struct gpio_desc reset_gpio;
};

static int qps615_pwrctl_i2c_write(struct udevice *client,
				   u32 reg_addr, u32 reg_val)
{
	u8 msg_buf[7];

	/* Big Endian for reg addr */
	reg_addr = cpu_to_be32(reg_addr);

	msg_buf[0] = (u8)(reg_addr >> 8);
	msg_buf[1] = (u8)(reg_addr >> 16);
	msg_buf[2] = (u8)(reg_addr >> 24);

	/* Little Endian for reg val */
	reg_val = cpu_to_le32(reg_val);

	msg_buf[3] = (u8)(reg_val);
	msg_buf[4] = (u8)(reg_val >> 8);
	msg_buf[5] = (u8)(reg_val >> 16);
	msg_buf[6] = (u8)(reg_val >> 24);

	return dm_i2c_write(client, 0, msg_buf, 7);
}

static int qps615_pwrctl_i2c_read(struct udevice *client,
				  u32 reg_addr, u32 *reg_val)
{
	struct dm_i2c_chip *chip = dev_get_parent_plat(client);
	struct i2c_msg msg[2];
	u8 wr_data[3];
	u32 rd_data;

	msg[0].addr = chip->chip_addr;
	msg[0].len = 3;
	msg[0].flags = 0;

	/* Big Endian for reg addr */
	reg_addr = cpu_to_be32(reg_addr);

	wr_data[0] = (u8)(reg_addr >> 8);
	wr_data[1] = (u8)(reg_addr >> 16);
	wr_data[2] = (u8)(reg_addr >> 24);

	msg[0].buf = wr_data;

	msg[1].addr = chip->chip_addr;
	msg[1].len = 4;
	msg[1].flags = I2C_M_RD;

	msg[1].buf = (u8 *)&rd_data;

	return dm_i2c_xfer(client, msg, 2);
}

static int qps615_pwrctl_assert_deassert_reset(struct qps615 *priv, bool deassert)
{
	int ret, val = 0;

	if (deassert)
		val = 0xc;

	ret = qps615_pwrctl_i2c_write(priv->i2c, QPS615_GPIO_CONFIG, 0xfffffff3);
	if (ret)
		return ret;

	return qps615_pwrctl_i2c_write(priv->i2c, QPS615_RESET_GPIO, val);
}

static int qps615_power_up(struct qps615 *priv)
{
	int ret;

	for (int i = 0; i < N_VREGS; i++) {
		ret = regulator_set_enable(priv->vregs[i], true);
		if (ret) {
			log_err("Couldn't enable %s regulator: %d\n", vregs[i], ret);
			return ret;
		}
	}

	ret = dm_gpio_set_value(&priv->reset_gpio, 1);
	if (ret) {
		log_err("Couldn't set reset gpio: %d\n", ret);
		return ret;
	}

	udelay(1000);

	qps615_pwrctl_assert_deassert_reset(priv, true);

	udelay(1000);

	ret = qps615_pwrctl_assert_deassert_reset(priv, true);
	if (!ret)
		return 0;

	return 0;
}



static int qps615_bind(struct udevice *dev)
{
	printf("Binding %s\n", dev->name);
	return 0;
}

static int qps615_probe(struct udevice *dev)
{
	struct qps615 *priv = dev_get_priv(dev);
	ofnode i2c_node;
	u32 phandle;
	int ret;

	printf("%s priv %p\n", __func__, priv);

	ret = dev_read_u32(dev, "qcom,qps615-controller", &phandle);
	if (ret) {
		log_err("qcom,qps615-controller property not found\n");
		return ret;
	}

	i2c_node = ofnode_get_by_phandle(phandle);
	if (!ofnode_valid(i2c_node)) {
		log_err("Couldn't get i2c node\n");
		return -ENODEV;
	}

	printf("Found i2c node: %s\n", ofnode_get_name(i2c_node));

	ret = device_find_global_by_ofnode(i2c_node, &priv->i2c);
	if (!ret) {
		log_err("Couldn't find i2c device: %d\n", ret);
		return ret;
	}

	for (int i = 0; i < ARRAY_SIZE(vregs); i++) {
		ret = device_get_supply_regulator(dev, vregs[i], &priv->vregs[i]);
		if (ret) {
			log_err("Couldn't get %s regulator: %d\n", vregs[i], ret);
			return ret;
		}
	}

	ret = gpio_request_by_name(dev, "reset-gpios", 0, &priv->reset_gpio, GPIOD_IS_OUT);
	if (ret) {
		log_err("Couldn't get reset gpio: %d\n", ret);
		return ret;
	}

	ret = qps615_power_up(priv);
	if (ret) {
		log_err("Couldn't power up: %d\n", ret);
		return ret;
	}

	return 0;
}

static const struct udevice_id qps615_ids[] = {
	{ .compatible = "pci1179,0623" },
	{ }
};

U_BOOT_DRIVER(qcom_qps615) = {
	.name		= "qcom-qps615",
	.id		= UCLASS_MISC,
	.of_match 	= qps615_ids,
	.bind 		= qps615_bind,
	.probe 		= qps615_probe,
	.priv_auto	= sizeof(struct qps615),
	.flags		= DM_FLAG_PROBE_AFTER_BIND,
};

static const struct udevice_id qps615_i2c_ids[] = {
	{ .compatible = "qcom,qps615" },
	{ }
};

/* Stub i2c peripheral */
U_BOOT_DRIVER(qcom_qps615_i2c) = {
	.name	= "qcom-qps615-i2c",
	.id	= UCLASS_I2C,
	.of_match = qps615_i2c_ids,
};
