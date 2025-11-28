/*
 * Copyright (c) 2025 SiFli Technologies Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb_peri_ldo

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <register.h>

LOG_MODULE_REGISTER(regulator_sf32lb, CONFIG_REGULATOR_LOG_LEVEL);

/*
 * PMUC PERI_LDO register bit definitions from CMSIS headers.
 *
 * Register layout (PERI_LDO @ offset 0x60 in PMUC):
 * [0]     EN_LDO18        - Enable 1.8V LDO
 * [4:1]   LDO18_VREF_SEL  - Voltage reference selection
 * [5]     LDO18_PD        - Power down control
 * [8]     EN_VDD33_LDO2   - Enable 3.3V LDO2
 * [12:9]  VDD33_LDO2_SET_VOUT - Output voltage setting
 * [13]    VDD33_LDO2_PD   - Power down control
 * [16]    EN_VDD33_LDO3   - Enable 3.3V LDO3
 * [20:17] VDD33_LDO3_SET_VOUT - Output voltage setting
 * [21]    VDD33_LDO3_PD   - Power down control
 */

/* LDO rail indices - must match DTS reg values */
enum sf32lb_ldo_rail {
	SF32LB_LDO18 = 0,
	SF32LB_LDO2_3V3 = 1,
	SF32LB_LDO3_3V3 = 2,
	SF32LB_LDO_COUNT,
};

/* LDO descriptor */
struct sf32lb_ldo_desc {
	uint32_t enable_mask;   /* Enable bit mask */
	uint32_t pd_mask;       /* Power down bit mask */
	int32_t voltage_uv;     /* Fixed voltage in microvolts */
};

static const struct sf32lb_ldo_desc sf32lb_ldo_descs[SF32LB_LDO_COUNT] = {
	[SF32LB_LDO18] = {
		.enable_mask = PMUC_PERI_LDO_EN_LDO18_Msk,
		.pd_mask = PMUC_PERI_LDO_LDO18_PD_Msk,
		.voltage_uv = 1800000,
	},
	[SF32LB_LDO2_3V3] = {
		.enable_mask = PMUC_PERI_LDO_EN_VDD33_LDO2_Msk,
		.pd_mask = PMUC_PERI_LDO_VDD33_LDO2_PD_Msk,
		.voltage_uv = 3300000,
	},
	[SF32LB_LDO3_3V3] = {
		.enable_mask = PMUC_PERI_LDO_EN_VDD33_LDO3_Msk,
		.pd_mask = PMUC_PERI_LDO_VDD33_LDO3_PD_Msk,
		.voltage_uv = 3300000,
	},
};

struct regulator_sf32lb_config {
	struct regulator_common_config common;
	volatile uint32_t *reg;
	enum sf32lb_ldo_rail rail;
};

struct regulator_sf32lb_data {
	struct regulator_common_data common;
};

static int regulator_sf32lb_enable(const struct device *dev)
{
	const struct regulator_sf32lb_config *config = dev->config;
	const struct sf32lb_ldo_desc *desc = &sf32lb_ldo_descs[config->rail];
	uint32_t val;

	LOG_DBG("Enabling LDO rail %d", config->rail);

	val = *config->reg;
	/* Clear power down bit and set enable bit */
	val &= ~desc->pd_mask;
	val |= desc->enable_mask;
	*config->reg = val;

	return 0;
}

static int regulator_sf32lb_disable(const struct device *dev)
{
	const struct regulator_sf32lb_config *config = dev->config;
	const struct sf32lb_ldo_desc *desc = &sf32lb_ldo_descs[config->rail];
	uint32_t val;

	LOG_DBG("Disabling LDO rail %d", config->rail);

	val = *config->reg;
	/* Clear enable bit and set power down bit */
	val &= ~desc->enable_mask;
	val |= desc->pd_mask;
	*config->reg = val;

	return 0;
}

static unsigned int regulator_sf32lb_count_voltages(const struct device *dev)
{
	/* Fixed voltage regulators have exactly 1 voltage */
	return 1U;
}

static int regulator_sf32lb_list_voltage(const struct device *dev,
					 unsigned int idx,
					 int32_t *volt_uv)
{
	const struct regulator_sf32lb_config *config = dev->config;
	const struct sf32lb_ldo_desc *desc = &sf32lb_ldo_descs[config->rail];

	if (idx != 0) {
		return -EINVAL;
	}

	*volt_uv = desc->voltage_uv;
	return 0;
}

static int regulator_sf32lb_get_voltage(const struct device *dev,
					int32_t *volt_uv)
{
	return regulator_sf32lb_list_voltage(dev, 0, volt_uv);
}

static DEVICE_API(regulator, regulator_sf32lb_api) = {
	.enable = regulator_sf32lb_enable,
	.disable = regulator_sf32lb_disable,
	.count_voltages = regulator_sf32lb_count_voltages,
	.list_voltage = regulator_sf32lb_list_voltage,
	.get_voltage = regulator_sf32lb_get_voltage,
};

static int regulator_sf32lb_init(const struct device *dev)
{
	const struct regulator_sf32lb_config *config = dev->config;

	LOG_DBG("Initializing SF32LB LDO rail %d", config->rail);

	regulator_common_data_init(dev);

	return regulator_common_init(dev, false);
}

#define REGULATOR_SF32LB_DEFINE(node_id, parent_reg)                           \
	static struct regulator_sf32lb_data data_##node_id;                    \
                                                                               \
	static const struct regulator_sf32lb_config config_##node_id = {       \
		.common = REGULATOR_DT_COMMON_CONFIG_INIT(node_id),            \
		.reg = (volatile uint32_t *)parent_reg,                        \
		.rail = DT_REG_ADDR(node_id),                                  \
	};                                                                     \
                                                                               \
	DEVICE_DT_DEFINE(node_id, regulator_sf32lb_init, NULL,                 \
			 &data_##node_id, &config_##node_id,                   \
			 PRE_KERNEL_1, CONFIG_REGULATOR_SF32LB_INIT_PRIORITY,  \
			 &regulator_sf32lb_api);

#define REGULATOR_SF32LB_DEFINE_ALL(inst)                                      \
	DT_INST_FOREACH_CHILD_STATUS_OKAY_VARGS(                               \
		inst, REGULATOR_SF32LB_DEFINE, DT_INST_REG_ADDR(inst))

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_SF32LB_DEFINE_ALL)
