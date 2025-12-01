/*
 * Copyright (c) 2025 Core Devices LLC
 * Copyright (c) 2025 SiFli Technologies(Nanjing) Co., Ltd
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_SIFLI_SF32_SF32LB52X_PINCTRL_SOC_H_
#define _SOC_SIFLI_SF32_SF32LB52X_PINCTRL_SOC_H_

#include <stdint.h>

#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/pinctrl/sf32lb-common-pinctrl.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Type for SF32LB pin.
 *
 * Bitmap:
 * - 0-10: Maps 1:1 to HPSYS_PINMUX:
 *   - 0-3: Function select
 *   - 4: Enable/disable pull
 *   - 5: Pull select (0=pulldown, 1=pullup)
 *   - 6: Input enable (0=disable, 1=enable)
 *   - 7: Input select (0=normal, 1=schmitt trigger)
 *   - 8: Slew rate (0=slow, 1=fast)
 *   - 9-10: Drive strength {DS0, DS1} (0-3), DS0 is high bit, default=2
 * - 11: Reserved
 * - 11-31: Location, port, pad, PINR register field and offset.
 */
typedef uint32_t pinctrl_soc_pin_t;

#define SF32LB_PE_MSK BIT(4U)
#define SF32LB_PS_MSK BIT(5U)
#define SF32LB_IE_MSK BIT(6U)
#define SF32LB_IS_MSK BIT(7U)
#define SF32LB_SR_MSK BIT(8U)
#define SF32LB_DS_MSK GENMASK(10U, 9U)

/* Analog function select value */
#define SF32LB_ANALOG_FSEL 15U

/*
 * Helper macro to handle analog mode.
 * When sifli,analog is set, override FSEL to analog (15) and clear PE/IE.
 * Otherwise, use the pinmux value from DT.
 */
#define Z_PINCTRL_ANALOG_FSEL(node_id, pinmux_val)                                                 \
	COND_CODE_1(DT_PROP(node_id, sifli_analog),                                                \
		    ((pinmux_val & ~SF32LB_FSEL_MSK) | SF32LB_ANALOG_FSEL),                        \
		    (pinmux_val))

#define Z_PINCTRL_ANALOG_PE(node_id) COND_CODE_1(DT_PROP(node_id, sifli_analog), (0), (1))

/*
 * Pin configuration mask for bits that should be modified.
 * SR (slew-rate) and IS (input-schmitt) are preserved from hardware defaults.
 */
#define SF32LB_PINMUX_CFG_MSK                                                                      \
	(SF32LB_FSEL_MSK | SF32LB_PE_MSK | SF32LB_PS_MSK | SF32LB_IE_MSK | SF32LB_DS_MSK)

#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)                                               \
	(Z_PINCTRL_ANALOG_FSEL(node_id, DT_PROP_BY_IDX(node_id, prop, idx)) |                      \
	 FIELD_PREP(SF32LB_PE_MSK,                                                                 \
		    Z_PINCTRL_ANALOG_PE(node_id) &                                                 \
			    (DT_PROP(node_id, bias_pull_up) | DT_PROP(node_id, bias_pull_down))) | \
	 FIELD_PREP(SF32LB_PS_MSK, DT_PROP(node_id, bias_pull_up)) |                               \
	 FIELD_PREP(SF32LB_IE_MSK,                                                                 \
		    Z_PINCTRL_ANALOG_PE(node_id) & DT_PROP(node_id, input_enable)) |               \
	 FIELD_PREP(SF32LB_DS_MSK, DT_PROP(node_id, drive_strength))),

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), DT_FOREACH_PROP_ELEM, pinmux,           \
				Z_PINCTRL_STATE_PIN_INIT)}

#ifdef __cplusplus
}
#endif

#endif /* _SOC_SIFLI_SF32_SF32LB52X_PINCTRL_SOC_H_ */
