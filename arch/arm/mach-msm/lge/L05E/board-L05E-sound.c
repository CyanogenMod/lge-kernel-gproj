/* Copyright (c) 2012, LGE Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/mfd/pm8xxx/pm8921.h>
#include <linux/regulator/consumer.h>
#include "devices.h"

#include "board-8064.h"
#ifdef CONFIG_SND_SOC_TPA2028D
#include <sound/tpa2028d.h>
#endif

#include "board-L05E.h"
#ifdef CONFIG_SWITCH_FSA8008
#include <linux/platform_data/hds_fsa8008.h>
#include <mach/board_lge.h>
#define DEVICE_NAME "fsa8008"
#elif defined(CONFIG_SWITCH_MAX1462X)
#include <linux/platform_data/hds_max1462x.h>
#include <mach/board_lge.h>
#define DEVICE_NAME "max1462x"
#endif

#define TPA2028D_ADDRESS (0xB0>>1)
#define MSM_AMP_EN (PM8921_GPIO_PM_TO_SYS(19))
#define AGC_COMPRESIION_RATE        0
#define AGC_OUTPUT_LIMITER_DISABLE  1
#define AGC_FIXED_GAIN              16

#ifdef CONFIG_SWITCH_FSA8008
#define GPIO_EAR_SENSE_N             82
#define GPIO_EAR_MIC_EN             PM8921_GPIO_PM_TO_SYS(31)
#define GPIO_EARPOL_DETECT          PM8921_GPIO_PM_TO_SYS(32)
#define GPIO_EAR_KEY_INT            83

#elif defined(CONFIG_SWITCH_MAX1462X)
#define GPIO_EAR_KEY_INT            PM8921_MPP_PM_TO_SYS(9)
#define GPIO_EAR_SENSE_N            PM8921_MPP_PM_TO_SYS(10)
#define GPIO_EAR_MIC_EN             PM8921_GPIO_PM_TO_SYS(31)
#endif

#define I2C_SURF 1
#define I2C_FFA  (1 << 1)
#define I2C_RUMI (1 << 2)
#define I2C_SIM  (1 << 3)
#define I2C_LIQUID (1 << 4)

struct i2c_registry {
	u8                     machs;
	int                    bus;
	struct i2c_board_info *info;
	int                    len;
};

#ifdef CONFIG_SND_SOC_TPA2028D
int amp_enable(int on_state)
{
	int err = 0;
	static int init_status = 0;
	struct pm_gpio param = {
		.direction      = PM_GPIO_DIR_OUT,
		.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
		.output_value   = 1,
		.pull           = PM_GPIO_PULL_NO,
		.vin_sel	= PM_GPIO_VIN_S4,
		.out_strength   = PM_GPIO_STRENGTH_MED,
		.function       = PM_GPIO_FUNC_NORMAL,
	};

	if (init_status == 0) {
		err = gpio_request(MSM_AMP_EN, "AMP_EN");
		if (err)
			pr_err("%s: Error requesting GPIO %d\n",
					__func__, MSM_AMP_EN);

		err = pm8xxx_gpio_config(MSM_AMP_EN, &param);
		if (err)
			pr_err("%s: Failed to configure gpio %d\n",
					__func__, MSM_AMP_EN);
		else
			init_status++;
	}

	switch (on_state) {
	case 0:
		err = gpio_direction_output(MSM_AMP_EN, 0);
		printk(KERN_INFO "%s: AMP_EN is set to 0\n", __func__);
		break;
	case 1:
		err = gpio_direction_output(MSM_AMP_EN, 1);
		printk(KERN_INFO "%s: AMP_EN is set to 1\n", __func__);
		break;
	case 2:
		printk(KERN_INFO "%s: amp enable bypass(%d)\n", __func__, on_state);
		err = 0;
		break;

	default:
		pr_err("amp enable fail\n");
		err = 1;
		break;
	}
	return err;
}

static struct audio_amp_platform_data amp_platform_data =  {
	.enable = amp_enable,
	.agc_compression_rate = AGC_COMPRESIION_RATE,
	.agc_output_limiter_disable = AGC_OUTPUT_LIMITER_DISABLE,
	.agc_fixed_gain = AGC_FIXED_GAIN,
};
#endif

static struct i2c_board_info msm_i2c_audiosubsystem_info[] = {
#ifdef CONFIG_SND_SOC_TPA2028D
	{
		I2C_BOARD_INFO("tpa2028d_amp", TPA2028D_ADDRESS),
		.platform_data = &amp_platform_data,
	}
#endif
};


static struct i2c_registry msm_i2c_audiosubsystem __initdata = {
	/* Add the I2C driver for Audio Amp, ehgrace.kim@lge.cim, 06/13/2011 */
		 I2C_SURF | I2C_FFA | I2C_RUMI | I2C_SIM | I2C_LIQUID,
		APQ_8064_GSBI1_QUP_I2C_BUS_ID,
		msm_i2c_audiosubsystem_info,
		ARRAY_SIZE(msm_i2c_audiosubsystem_info),
};

static void __init lge_add_i2c_tpa2028d_devices(void)
{
	/* Run the array and install devices as appropriate */
	i2c_register_board_info(msm_i2c_audiosubsystem.bus,
				msm_i2c_audiosubsystem.info,
				msm_i2c_audiosubsystem.len);
}

#ifdef CONFIG_SWITCH_FSA8008
static void enable_external_mic_bias(int status)
{
	static struct regulator *reg_mic_bias = NULL;
	static int prev_on;
	int rc = 0;

	if (status == prev_on)
		return;

	if (!reg_mic_bias) {
		reg_mic_bias = regulator_get(NULL, "mic_bias");
		if (IS_ERR(reg_mic_bias)) {
			pr_err("%s: could not regulator_get\n",
					__func__);
			reg_mic_bias = NULL;
			return;
		}
	}

	if (status) {
		rc = regulator_enable(reg_mic_bias);
		if (rc)
			pr_err("%s: regulator enable failed\n",
					__func__);
		pr_debug("%s: mic_bias is on\n", __func__);
	} else {
		rc = regulator_disable(reg_mic_bias);
		if (rc)
			pr_warn("%s: regulator disable failed\n",
					__func__);
		pr_debug("%s: mic_bias is off\n", __func__);
	}

	prev_on = status;

}
#endif

#ifdef CONFIG_SWITCH_FSA8008
static struct fsa8008_platform_data lge_hs_pdata = {
	.switch_name = "h2w",
	.keypad_name = "hs_detect",

	.key_code = KEY_MEDIA,

	.gpio_detect = GPIO_EAR_SENSE_N,
	.gpio_mic_en = GPIO_EAR_MIC_EN,
	.gpio_mic_bias_en = -1,
	.gpio_jpole  = GPIO_EARPOL_DETECT,
	.gpio_key    = GPIO_EAR_KEY_INT,

	.latency_for_detection = 75,
	.set_headset_mic_bias = enable_external_mic_bias,
};
#elif defined(CONFIG_SWITCH_MAX1462X)
static struct max1462x_platform_data lge_hs_pdata = {
    .switch_name = "h2w",               /* switch device name */
    .keypad_name = "hs_detect",         /* keypad device name */

    .key_code = 0,                      /* KEY_RESERVED(0), KEY_MEDIA(226), KEY_VOLUMEUP(115) or KEY_VOLUMEDOWN(114) */

    .gpio_mode  = GPIO_EAR_MIC_EN,      /* MODE : high, low, high-z */
    .gpio_det   = GPIO_EAR_SENSE_N,     /* DET : to detect jack inserted or not */
    .gpio_swd   = GPIO_EAR_KEY_INT,     /* SWD : to detect 3 pole or 4 pole | to detect among hook, volum up or down key */


    .latency_for_detection  = 300,      /* DETIN Debounce Time(300ms in Spec.) */
    .latency_for_key        = 30,       /* SEND/END Debounce Time(30ms in Spec.) */

    .adc_mpp_num = PM8XXX_AMUX_MPP_1,   /* PMIC adc mpp number to read adc level on MIC */
    .adc_channel = ADC_MPP_1_AMUX6,     /* PMIC adc channel to read adc level on MIC */

    .external_ldo_mic_bias  = 0,       /* GPIO for an external LDO control */
    .set_headset_mic_bias = NULL,       /* callback function for an external LDO control */
};
#else
static struct fsa8008_platform_data lge_hs_pdata;
#endif

static __init void L05E_fixed_audio(void)
{
//TODO
//HW modification
}

static struct platform_device lge_hsd_device = {
	.name = DEVICE_NAME,
	.id   = -1,
	.dev = {
		.platform_data = &lge_hs_pdata,
	},
};

static struct platform_device *sound_devices[] __initdata = {
	&lge_hsd_device,
};

void __init lge_add_sound_devices(void)
{
	L05E_fixed_audio();
	lge_add_i2c_tpa2028d_devices();
	platform_add_devices(sound_devices, ARRAY_SIZE(sound_devices));
}
