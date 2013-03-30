/* Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
 * Copyright (c) 2012, LGE Inc.
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

#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/bootmem.h>
#include <linux/mfd/pm8xxx/pm8921.h>
#include <linux/leds.h>
#include <linux/leds-pm8xxx.h>
#include <linux/mfd/pm8xxx/pm8xxx-adc.h>
#include <asm/mach-types.h>
#include <asm/mach/mmc.h>
#include <mach/msm_bus_board.h>
#include <mach/board.h>
#include <mach/gpio.h>
#include <mach/gpiomux.h>
#include <mach/restart.h>
#include <mach/socinfo.h>
#include <mach/board_lge.h>
#include <linux/i2c.h>
#include "devices.h"
#include "board-gk.h"
#ifdef CONFIG_BATTERY_MAX17043
#include <linux/max17043_fuelgauge.h>
#endif

// [[LGE_BSP_AUDIO, jeremy.pi@lge.com, Audience eS325 ALSA SoC Audio driver
#if defined(CONFIG_MACH_APQ8064_GKATT)
#include <sound/esxxx.h>
#endif /* CONFIG_MACH_APQ8064_GKATT */
// ]]LGE_BSP_AUDIO, jeremy.pi@lge.com, Audience eS325 ALSA SoC Audio driver

struct pm8xxx_gpio_init {
	unsigned			gpio;
	struct pm_gpio			config;
};

struct pm8xxx_mpp_init {
	unsigned			mpp;
	struct pm8xxx_mpp_config_data	config;
};

#define PM8921_GPIO_INIT(_gpio, _dir, _buf, _val, _pull, _vin, _out_strength, \
			_func, _inv, _disable) \
{ \
	.gpio	= PM8921_GPIO_PM_TO_SYS(_gpio), \
	.config	= { \
		.direction	= _dir, \
		.output_buffer	= _buf, \
		.output_value	= _val, \
		.pull		= _pull, \
		.vin_sel	= _vin, \
		.out_strength	= _out_strength, \
		.function	= _func, \
		.inv_int_pol	= _inv, \
		.disable_pin	= _disable, \
	} \
}

#define PM8921_MPP_INIT(_mpp, _type, _level, _control) \
{ \
	.mpp	= PM8921_MPP_PM_TO_SYS(_mpp), \
	.config	= { \
		.type		= PM8XXX_MPP_TYPE_##_type, \
		.level		= _level, \
		.control	= PM8XXX_MPP_##_control, \
	} \
}

#define PM8821_MPP_INIT(_mpp, _type, _level, _control) \
{ \
	.mpp	= PM8821_MPP_PM_TO_SYS(_mpp), \
	.config	= { \
		.type		= PM8XXX_MPP_TYPE_##_type, \
		.level		= _level, \
		.control	= PM8XXX_MPP_##_control, \
	} \
}

#define PM8921_GPIO_DISABLE(_gpio) \
	PM8921_GPIO_INIT(_gpio, PM_GPIO_DIR_IN, 0, 0, 0, PM_GPIO_VIN_S4, \
			 0, 0, 0, 1)

#define PM8921_GPIO_OUTPUT(_gpio, _val, _strength) \
	PM8921_GPIO_INIT(_gpio, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, _val, \
			PM_GPIO_PULL_NO, PM_GPIO_VIN_S4, \
			PM_GPIO_STRENGTH_##_strength, \
			PM_GPIO_FUNC_NORMAL, 0, 0)

#define PM8921_GPIO_OUTPUT_BUFCONF(_gpio, _val, _strength, _bufconf) \
	PM8921_GPIO_INIT(_gpio, PM_GPIO_DIR_OUT,\
			PM_GPIO_OUT_BUF_##_bufconf, _val, \
			PM_GPIO_PULL_NO, PM_GPIO_VIN_S4, \
			PM_GPIO_STRENGTH_##_strength, \
			PM_GPIO_FUNC_NORMAL, 0, 0)

#define PM8921_GPIO_INPUT(_gpio, _pull) \
	PM8921_GPIO_INIT(_gpio, PM_GPIO_DIR_IN, PM_GPIO_OUT_BUF_CMOS, 0, \
			_pull, PM_GPIO_VIN_S4, \
			PM_GPIO_STRENGTH_NO, \
			PM_GPIO_FUNC_NORMAL, 0, 0)

#define PM8921_GPIO_OUTPUT_FUNC(_gpio, _val, _func) \
	PM8921_GPIO_INIT(_gpio, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, _val, \
			PM_GPIO_PULL_NO, PM_GPIO_VIN_S4, \
			PM_GPIO_STRENGTH_HIGH, \
			_func, 0, 0)

#define PM8921_GPIO_OUTPUT_VIN(_gpio, _val, _vin) \
	PM8921_GPIO_INIT(_gpio, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, _val, \
			PM_GPIO_PULL_NO, _vin, \
			PM_GPIO_STRENGTH_HIGH, \
			PM_GPIO_FUNC_NORMAL, 0, 0)

/* Initial PM8921 GPIO configurations */
static struct pm8xxx_gpio_init pm8921_gpios[] __initdata = {

#ifdef CONFIG_SWITCH_FSA8008
	PM8921_GPIO_OUTPUT(31, 0, HIGH), /* PMIC - FSA8008_EAR_MIC_EN */
	PM8921_GPIO_INPUT(32, PM_GPIO_PULL_UP_1P5), /* PMIC - FSA8008_EARPOL_DETECT */
#endif
#ifdef CONFIG_SWITCH_MAX1462X
	PM8921_GPIO_OUTPUT(31, 0, HIGH), /* PMIC - MAX1462X_EAR_MIC_EN */
       PM8921_GPIO_OUTPUT(32, 0, LOW), /* PMIC - EXT_EAR_MIC_BIAS_EN */
#endif
#ifdef CONFIG_RADIO_IRIS
	PM8921_GPIO_OUTPUT(17, 1, LOW), /* PMIC - DMB_FM_ANT_SEL_P */
	PM8921_GPIO_OUTPUT(18, 0, LOW), /* PMIC - DMB_FM_ANT_SEL_N */
#endif
#ifdef CONFIG_SLIMPORT_ANX7808
	PM8921_GPIO_INPUT(14, PM_GPIO_PULL_DN),
	PM8921_GPIO_OUTPUT(15, 0, HIGH), /* ANX_P_DWN_CTL */
	PM8921_GPIO_OUTPUT(16, 0, HIGH), /* ANX_AVDD33_EN */
#else
	PM8921_GPIO_OUTPUT(14, 1, HIGH),	/* HDMI Mux Selector */
#endif
/* LGE_CHANGE_S, For GV/GK 13M & 2.4M camera driver, 2012.08.15, gayoung85.lee@lge.com */
	PM8921_GPIO_OUTPUT(13, 0, HIGH), /* ISP_STBY */
/* LGE_CHANGE_E, For GV/GK 13M & 2.4M camera driver, 2012.08.15, gayoung85.lee@lge.com */
	PM8921_GPIO_OUTPUT(23, 0, HIGH),	/* touchscreen power FET */
//	PM8921_GPIO_OUTPUT_BUFCONF(25, 0, LOW, CMOS), /* DISP_RESET_N */
//	PM8921_GPIO_OUTPUT_FUNC(26, 0, PM_GPIO_FUNC_2), /* Bl: Off, PWM mode */

// [S] LGE_BT: MOD/ilbeom.kim/'12-10-26 - [GK] BRCM INITIAL PORTING
#ifdef CONFIG_LGE_BLUESLEEP
	PM8921_GPIO_OUTPUT(30, 0, HIGH),
#else
// Original
	PM8921_GPIO_OUTPUT_VIN(30, 1, PM_GPIO_VIN_VPH), /* SMB349 susp line */
#endif
// [E] LGE_BT: MOD/ilbeom.kim/'12-10-26 - [GK] BRCM INITIAL PORTING
	PM8921_GPIO_OUTPUT_BUFCONF(36, 1, LOW, OPEN_DRAIN),
#ifdef CONFIG_BCMDHD
	PM8921_GPIO_OUTPUT_FUNC(43, 0, PM_GPIO_FUNC_1),
#else
	PM8921_GPIO_OUTPUT_FUNC(44, 0, PM_GPIO_FUNC_2),
#endif
	PM8921_GPIO_OUTPUT(33, 0, HIGH),
/* LGE_CHANGE_S, For GV/GK 2.4M front camera driver, 2012.07.20, gayoung85.lee@lge.com */
	PM8921_GPIO_OUTPUT(20, 0, HIGH), /* ISP_HOST_INT */
	PM8921_GPIO_OUTPUT(35, 0, HIGH), /* VT_PWR_EN */
//	PM8921_GPIO_INPUT(35, PM_GPIO_PULL_UP_30),
/* LGE_CHANGE_E, For GV/GK 2.4M front camera driver, 2012.07.20, gayoung85.lee@lge.com */

// [[LGE_BSP_AUDIO, jeremy.pi@lge.com, Audience eS325 ALSA SoC Audio driver
#if defined(CONFIG_SND_SOC_ES325_SLIM)
	PM8921_GPIO_INPUT(38, PM_GPIO_PULL_UP_30),
#else
	PM8921_GPIO_OUTPUT(38, 0, LOW),
#endif /* CONFIG_SND_SOC_ES325_SLIM */
// ]]LGE_BSP_AUDIO, jeremy.pi@lge.com, Audience eS325 ALSA SoC Audio driver
	/* TABLA CODEC RESET */
	PM8921_GPIO_OUTPUT(34, 1, HIGH),
	PM8921_GPIO_OUTPUT(13, 0, HIGH),               /* PCIE_CLK_PWR_EN */
/* LGE_CHANGE_S, delete gpio12 pullup setting for tdmb inner <-> ear antenna switching, 2012.10.18, wonhee.jeong@lge.com */
	//PM8921_GPIO_INPUT(12, PM_GPIO_PULL_UP_30),     /* PCIE_WAKE_N */
/* LGE_CHANGE_E, delete gpio12 pullup setting for tdmb inner <-> ear antenna switching, 2012.10.18, wonhee.jeong@lge.com */
#ifdef CONFIG_LGE_WIRELESS_CHARGER
	PM8921_GPIO_OUTPUT(26, 0, HIGH),		    /* CHG_STAT */
	PM8921_GPIO_INPUT(25, PM_GPIO_PULL_NO),//PM_GPIO_PULL_UP_30),	 /* WLC_ACTIVE */
#endif
#ifdef CONFIG_BCMDHD
	// WLAN_POWER_ON
	PM8921_GPIO_OUTPUT(29, 0, HIGH),
#endif
#ifdef CONFIG_FB_MSM_MIPI_LGIT_VIDEO_FHD_INVERSE_PT
    PM8921_GPIO_OUTPUT(42, 1, HIGH),
    PM8921_GPIO_OUTPUT(22, 1, HIGH),   
#endif

};

/* changduk.ryu@lge.com Disabled Unused GPIOs and MPPS*/
#if defined(CONFIG_MACH_LGE)
static struct pm8xxx_gpio_init pm8921_disabled_gpios[] __initdata= {
	PM8921_GPIO_DISABLE(3),
	PM8921_GPIO_DISABLE(4),
	PM8921_GPIO_DISABLE(5),
	PM8921_GPIO_DISABLE(6),
	PM8921_GPIO_DISABLE(8),
	PM8921_GPIO_DISABLE(11),
	PM8921_GPIO_DISABLE(12),
	PM8921_GPIO_DISABLE(17),
	PM8921_GPIO_DISABLE(18),
	PM8921_GPIO_DISABLE(36),
#if defined(CONFIG_MACH_APQ8064_GK_KR)
	PM8921_GPIO_DISABLE(38),
	PM8921_GPIO_DISABLE(40),
	PM8921_GPIO_DISABLE(41),
#endif
	PM8921_GPIO_DISABLE(44),
};
#endif		

static struct pm8xxx_gpio_init pm8921_mtp_kp_gpios[] __initdata = {
	PM8921_GPIO_INPUT(3, PM_GPIO_PULL_UP_30),
	PM8921_GPIO_INPUT(4, PM_GPIO_PULL_UP_30),
};

static struct pm8xxx_gpio_init pm8921_cdp_kp_gpios[] __initdata = {
	PM8921_GPIO_INPUT(27, PM_GPIO_PULL_UP_30),
	PM8921_GPIO_INPUT(42, PM_GPIO_PULL_UP_30),

};

static struct pm8xxx_gpio_init pm8921_mpq8064_hrd_gpios[] __initdata = {
	PM8921_GPIO_OUTPUT(37, 0, LOW),	/* MUX1_SEL */
};

/* Initial PM8917 GPIO configurations */
static struct pm8xxx_gpio_init pm8917_gpios[] __initdata = {
	PM8921_GPIO_OUTPUT(14, 1, HIGH),	/* HDMI Mux Selector */
	PM8921_GPIO_OUTPUT(23, 0, HIGH),	/* touchscreen power FET */
	PM8921_GPIO_OUTPUT_BUFCONF(25, 0, LOW, CMOS), /* DISP_RESET_N */
	PM8921_GPIO_OUTPUT(26, 1, HIGH), /* Backlight: on */
	PM8921_GPIO_OUTPUT_BUFCONF(36, 1, LOW, OPEN_DRAIN),
	PM8921_GPIO_OUTPUT_FUNC(38, 0, PM_GPIO_FUNC_2),
	PM8921_GPIO_OUTPUT(33, 0, HIGH),
	PM8921_GPIO_OUTPUT(20, 0, HIGH),
	PM8921_GPIO_INPUT(35, PM_GPIO_PULL_UP_30),
	PM8921_GPIO_INPUT(30, PM_GPIO_PULL_UP_30),
	/* TABLA CODEC RESET */
	PM8921_GPIO_OUTPUT(34, 1, MED),
	PM8921_GPIO_OUTPUT(13, 0, HIGH),               /* PCIE_CLK_PWR_EN */
	PM8921_GPIO_INPUT(12, PM_GPIO_PULL_UP_30),     /* PCIE_WAKE_N */
};

/* PM8921 GPIO 42 remaps to PM8917 GPIO 8 */
static struct pm8xxx_gpio_init pm8917_cdp_kp_gpios[] __initdata = {
	PM8921_GPIO_INPUT(27, PM_GPIO_PULL_UP_30),
	PM8921_GPIO_INPUT(8, PM_GPIO_PULL_UP_30),
	PM8921_GPIO_INPUT(17, PM_GPIO_PULL_UP_1P5),	/* SD_WP */
};

static struct pm8xxx_gpio_init pm8921_mpq_gpios[] __initdata = {
	PM8921_GPIO_INIT(27, PM_GPIO_DIR_IN, PM_GPIO_OUT_BUF_CMOS, 0,
			PM_GPIO_PULL_NO, PM_GPIO_VIN_VPH, PM_GPIO_STRENGTH_NO,
			PM_GPIO_FUNC_NORMAL, 0, 0),
};

/* changduk.ryu@lge.com Disabled Unused GPIOs and MPPS*/
#if defined(CONFIG_MACH_LGE)
static struct pm8xxx_mpp_init pm8xxx_disabled_mpps[] __initdata = {
		PM8921_MPP_INIT(2, SINK, PM8XXX_MPP_CS_OUT_5MA, CS_CTRL_DISABLE),
		PM8921_MPP_INIT(9, SINK, PM8XXX_MPP_CS_OUT_5MA, CS_CTRL_DISABLE),
		PM8921_MPP_INIT(10, SINK, PM8XXX_MPP_CS_OUT_5MA, CS_CTRL_DISABLE),
		PM8921_MPP_INIT(11, SINK, PM8XXX_MPP_CS_OUT_5MA, CS_CTRL_DISABLE),
};
#endif

/* Initial PM8XXX MPP configurations */
static struct pm8xxx_mpp_init pm8xxx_mpps[] __initdata = {
	PM8921_MPP_INIT(3, D_OUTPUT, PM8921_MPP_DIG_LEVEL_VPH, DOUT_CTRL_LOW),
	PM8921_MPP_INIT(8, D_OUTPUT, PM8921_MPP_DIG_LEVEL_S4, DOUT_CTRL_LOW),
	/*MPP9 is used to detect docking station connection/removal on Liquid*/
	/*MPP1 is used to read ADC for headset 3 Button read key*/
	PM8921_MPP_INIT(1, D_INPUT, PM8921_MPP_DIG_LEVEL_S4, DIN_TO_INT),
};
#if defined(CONFIG_LGE_WIRELESS_CHARGER)
struct pm_gpio pm_gpio_wlc_ts_ctrl = {
	.direction = PM_GPIO_DIR_IN,
	.output_buffer = PM_GPIO_OUT_BUF_CMOS,
	.output_value = 0,
	.pull = PM_GPIO_PULL_NO,
	.vin_sel = PM_GPIO_VIN_S4,
	.out_strength = PM_GPIO_STRENGTH_NO,
	.function = PM_GPIO_FUNC_NORMAL,
	.inv_int_pol = 0,
	.disable_pin = 0
};
#endif

/* LGE_CHANGE_S, For GK/GV Rev.E bring-up, 2012.10.26, gayoung85.lee[Start] */
#if defined (CONFIG_IMX132) || defined(CONFIG_CE1702)
struct pm_gpio pm_gpio_cam_rst_ctrl = {
	.direction = PM_GPIO_DIR_OUT,
	.output_buffer = PM_GPIO_OUT_BUF_CMOS,
	.output_value = 0,
	.pull = PM_GPIO_PULL_NO,
	.vin_sel = PM_GPIO_VIN_S4,
	.out_strength = PM_GPIO_STRENGTH_HIGH,
	.function = PM_GPIO_FUNC_NORMAL,
	.inv_int_pol = 0,
	.disable_pin = 0
};
#endif
/* LGE_CHANGE_S, For GK/GV Rev.E bring-up, 2012.10.26, gayoung85.lee[End] */

void __init apq8064_configure_gpios(struct pm8xxx_gpio_init *data, int len)
{
	int i, rc;
#if defined(CONFIG_LGE_WIRELESS_CHARGER)
	hw_rev_type lge_bd_rev = HW_REV_EVB1;
#endif

	for (i = 0; i < len; i++) {
		rc = pm8xxx_gpio_config(data[i].gpio, &data[i].config);
		if (rc)
			pr_err("%s: pm8xxx_gpio_config(%u) failed: rc=%d\n",
				__func__, data[i].gpio, rc);
	}

#if defined(CONFIG_LGE_WIRELESS_CHARGER)
	lge_bd_rev = lge_get_board_revno();
	if(lge_bd_rev == HW_REV_A || lge_bd_rev == HW_REV_B || lge_bd_rev == HW_REV_F) {
		rc = pm8xxx_gpio_config(PM8921_GPIO_PM_TO_SYS(44), &pm_gpio_wlc_ts_ctrl);
		if (rc)
			pr_err("%s: pm8xxx_gpio_config(%u) failed: rc=%d\n",
				__func__, PM8921_GPIO_PM_TO_SYS(44), rc);
	}
	else {
		rc = pm8xxx_gpio_config(PM8921_GPIO_PM_TO_SYS(7), &pm_gpio_wlc_ts_ctrl);
		if (rc)
			pr_err("%s: pm8xxx_gpio_config(%u) failed: rc=%d\n",
				__func__, PM8921_GPIO_PM_TO_SYS(7), rc);
	}
#endif
}

// [[LGE_BSP_AUDIO, jeremy.pi@lge.com, Audience eS325 ALSA SoC Audio driver
#if defined(CONFIG_MACH_APQ8064_GKATT)
int es325_gpio_init(void)
{
	struct pm_gpio es325_pm_gpio_param = {
		.direction		= PM_GPIO_DIR_OUT,
		.output_buffer	= PM_GPIO_OUT_BUF_CMOS,
		.output_value	= 0,
		.pull			= PM_GPIO_PULL_NO,
		.vin_sel		= PM_GPIO_VIN_S4,
		.out_strength	= PM_GPIO_STRENGTH_HIGH,
		.function		= PM_GPIO_FUNC_NORMAL,
		.inv_int_pol	= 0,
		.disable_pin	= 0
	};

	int rc;

	printk(KERN_INFO "%s\n", __func__);

	rc = gpio_request(ES325_RESET_GPIO, "es325_reset");
	if (rc < 0) {
		printk(KERN_INFO "%s(): es325_reset request failed", __func__);
		goto reset_gpio_request_error;
	}

	rc = pm8xxx_gpio_config(ES325_RESET_GPIO, &es325_pm_gpio_param);
	if (rc < 0) {
		printk(KERN_INFO "%s(): es325_reset config failed", __func__);
		goto reset_gpio_request_error;
	}

	rc = gpio_direction_output(ES325_RESET_GPIO, 0);
	if (rc < 0) {
		printk(KERN_INFO "%s(): es325_reset direction failed", __func__);
		goto reset_gpio_direction_error;
	}

	rc = gpio_request(ES325_WAKEUP_GPIO, "es325_wakeup");
	if (rc < 0) {
		printk(KERN_INFO "%s(): es325_wakeup request failed", __func__);
		goto wakeup_gpio_request_error;
	}

	rc = pm8xxx_gpio_config(ES325_WAKEUP_GPIO, &es325_pm_gpio_param);
	if (rc < 0) {
		printk(KERN_INFO "%s(): es325_wakeup config failed", __func__);
		goto wakeup_gpio_request_error;
	}

	rc = gpio_direction_output(ES325_WAKEUP_GPIO, 0);
	if (rc < 0) {
		printk(KERN_INFO "%s(): es325_wakeup direction failed",	__func__);
		goto wakeup_gpio_direction_error;
	}

	return rc;

wakeup_gpio_direction_error:
	gpio_free(ES325_WAKEUP_GPIO);
wakeup_gpio_request_error:
reset_gpio_direction_error:
	gpio_free(ES325_RESET_GPIO);
reset_gpio_request_error:
	printk(KERN_INFO "%s(): exit with error\n", __func__);
	return rc;
}
#endif /* CONFIG_MACH_APQ8064_GKATT */
// ]]LGE_BSP_AUDIO, jeremy.pi@lge.com, Audience eS325 ALSA SoC Audio driver

void __init apq8064_pm8xxx_gpio_mpp_init(void)
{
	int i, rc;
/* changduk.ryu@lge.com Disabled Unused GPIOs and MPPS*/
#if defined(CONFIG_MACH_LGE)
	hw_rev_type lge_bd_rev = HW_REV_EVB1;

	lge_bd_rev = lge_get_board_revno();
	if(lge_bd_rev >= HW_REV_C && lge_bd_rev != HW_REV_E ) {
		apq8064_configure_gpios(pm8921_disabled_gpios, ARRAY_SIZE(pm8921_disabled_gpios));

		for (i = 0; i < ARRAY_SIZE(pm8xxx_disabled_mpps); i++) {
			rc = pm8xxx_mpp_config(pm8xxx_disabled_mpps[i].mpp,
						&pm8xxx_disabled_mpps[i].config);
			if (rc) {
				pr_err("%s: pm8xxx_mpp_config: rc=%d\n", __func__, rc);
				break;
			}
		}	
	}
#endif

	if (socinfo_get_pmic_model() != PMIC_MODEL_PM8917)
		apq8064_configure_gpios(pm8921_gpios, ARRAY_SIZE(pm8921_gpios));
	else
		apq8064_configure_gpios(pm8917_gpios, ARRAY_SIZE(pm8917_gpios));

	if (machine_is_apq8064_cdp() || machine_is_apq8064_liquid()) {
		if (socinfo_get_pmic_model() != PMIC_MODEL_PM8917)
			apq8064_configure_gpios(pm8921_cdp_kp_gpios,
					ARRAY_SIZE(pm8921_cdp_kp_gpios));
		else
			apq8064_configure_gpios(pm8917_cdp_kp_gpios,
					ARRAY_SIZE(pm8917_cdp_kp_gpios));
	}

	if (machine_is_apq8064_mtp())
		apq8064_configure_gpios(pm8921_mtp_kp_gpios,
					ARRAY_SIZE(pm8921_mtp_kp_gpios));

	if (machine_is_mpq8064_cdp() || machine_is_mpq8064_hrd()
	    || machine_is_mpq8064_dtv())
		apq8064_configure_gpios(pm8921_mpq_gpios,
					ARRAY_SIZE(pm8921_mpq_gpios));

	if (machine_is_mpq8064_hrd())
		apq8064_configure_gpios(pm8921_mpq8064_hrd_gpios,
					ARRAY_SIZE(pm8921_mpq8064_hrd_gpios));

	for (i = 0; i < ARRAY_SIZE(pm8xxx_mpps); i++) {
		rc = pm8xxx_mpp_config(pm8xxx_mpps[i].mpp,
					&pm8xxx_mpps[i].config);
		if (rc) {
			pr_err("%s: pm8xxx_mpp_config: rc=%d\n", __func__, rc);
			break;
		}
	}

// [[LGE_BSP_AUDIO, jeremy.pi@lge.com, Audience eS325 ALSA SoC Audio driver
#if defined(CONFIG_MACH_APQ8064_GKATT)
	es325_gpio_init();
#endif /* CONFIG_MACH_APQ8064_GKATT */
// ]]LGE_BSP_AUDIO, jeremy.pi@lge.com, Audience eS325 ALSA SoC Audio driver
}

static struct pm8xxx_pwrkey_platform_data apq8064_pm8921_pwrkey_pdata = {
	.pull_up		= 1,
	.kpd_trigger_delay_us	= 15625,
	.wakeup			= 1,
};

static struct pm8xxx_misc_platform_data apq8064_pm8921_misc_pdata = {
	.priority		= 0,
};


#define PM8921_LC_LED_MAX_CURRENT	4	/* I = 4mA */
#if defined(CONFIG_MACH_APQ8064_GK_KR) || defined(CONFIG_MACH_APQ8064_GKATT)
#define PM8921_KEY_LED_MAX_CURRENT	16	/* I = 16mA */
#else
#define PM8921_KEY_LED_MAX_CURRENT	4	/* I = 4mA */
#endif
#define PM8921_LC_LED_LOW_CURRENT	1	/* I = 1mA */
#define PM8XXX_LED_PWM_PERIOD		1000
#define PM8XXX_LED_PWM_DUTY_MS		20
/**
 * PM8XXX_PWM_CHANNEL_NONE shall be used when LED shall not be
 * driven using PWM feature.
 */
#define PM8XXX_PWM_CHANNEL_NONE		-1

static struct led_info pm8921_led_info_rev_f[] = {
#if defined(CONFIG_LGE_PM)
			[0] = {
				.name			= "led:red",
			},
#else
			[0] = {
				.name			= "led:red",
				.default_trigger	= "ac-online",
			},
#endif

#if defined(CONFIG_MACH_LGE)	/* yoogyeong.lee@lge.com KEY_LED */
			[1] = {
				.name			= "button-backlight",
			},
 #if !defined(CONFIG_MACH_APQ8064_J1A)
			[2] = {
				.name			= "led:green",
			},
#endif
#endif

};

static struct led_platform_data pm8921_led_core_pdata_rev_f = {
	.num_leds = ARRAY_SIZE(pm8921_led_info_rev_f),
	.leds = pm8921_led_info_rev_f,
};


static struct led_info pm8921_led_info[] = {
			[0] = {
				.name			= "button-backlight",
			},
};

static struct led_platform_data pm8921_led_core_pdata = {
	.num_leds = ARRAY_SIZE(pm8921_led_info),
	.leds = pm8921_led_info,
};


#if defined(CONFIG_LGE_PM)
#else
static int pm8921_led0_pwm_duty_pcts[56] = {
	1, 4, 8, 12, 16, 20, 24, 28, 32, 36,
	40, 44, 46, 52, 56, 60, 64, 68, 72, 76,
	80, 84, 88, 92, 96, 100, 100, 100, 98, 95,
	92, 88, 84, 82, 78, 74, 70, 66, 62, 58,
	58, 54, 50, 48, 42, 38, 34, 30, 26, 22,
	14, 10, 6, 4, 1
};
#endif

/*
 * Note: There is a bug in LPG module that results in incorrect
 * behavior of pattern when LUT index 0 is used. So effectively
 * there are 63 usable LUT entries.
 */

#if defined(CONFIG_LGE_PM)
#else
static struct pm8xxx_pwm_duty_cycles pm8921_led0_pwm_duty_cycles = {
	.duty_pcts = (int *)&pm8921_led0_pwm_duty_pcts,
	.num_duty_pcts = ARRAY_SIZE(pm8921_led0_pwm_duty_pcts),
	.duty_ms = PM8XXX_LED_PWM_DUTY_MS,
	.start_idx = 1,
};
#endif

static struct pm8xxx_led_config pm8921_led_configs_rev_f[] = {

#if defined(CONFIG_LGE_PM)
			[0] = {
				.id = PM8XXX_ID_LED_0,
				.mode = PM8XXX_LED_MODE_MANUAL,
				.max_current = PM8921_LC_LED_MAX_CURRENT,
			},
#else
			[0] = {
				.id = PM8XXX_ID_LED_0,
				.mode = PM8XXX_LED_MODE_PWM2,
				.max_current = PM8921_LC_LED_MAX_CURRENT,
				.pwm_channel = 5,
				.pwm_period_us = PM8XXX_LED_PWM_PERIOD,
				.pwm_duty_cycles = &pm8921_led0_pwm_duty_cycles,
			},
#endif

#if defined(CONFIG_MACH_LGE)	/* yoogyeong.lee@lge.com KEY_LED */
			[1] = {
				.id = PM8XXX_ID_LED_1,
				.mode = PM8XXX_LED_MODE_MANUAL,
				.max_current = PM8921_KEY_LED_MAX_CURRENT,
			},
 #if !defined(CONFIG_MACH_APQ8064_J1A)
			[2] = {
				.id = PM8XXX_ID_LED_2,
				.mode = PM8XXX_LED_MODE_MANUAL,
				.max_current = PM8921_LC_LED_MAX_CURRENT,
			},
#endif
#endif

};

static struct pm8xxx_led_platform_data apq8064_pm8921_leds_pdata_rev_f = {
		.led_core = &pm8921_led_core_pdata_rev_f,
		.configs = pm8921_led_configs_rev_f,
		.num_configs = ARRAY_SIZE(pm8921_led_configs_rev_f),
};

static struct pm8xxx_led_config pm8921_led_configs[] = {
			[0] = {
				.id = PM8XXX_ID_LED_0,
				.mode = PM8XXX_LED_MODE_MANUAL,
				.max_current = PM8921_KEY_LED_MAX_CURRENT,
			},
};

static struct pm8xxx_led_platform_data apq8064_pm8921_leds_pdata = {
		.led_core = &pm8921_led_core_pdata,
		.configs = pm8921_led_configs,
		.num_configs = ARRAY_SIZE(pm8921_led_configs),
};


static struct pm8xxx_adc_amux apq8064_pm8921_adc_channels_data[] = {
	{"vcoin", CHANNEL_VCOIN, CHAN_PATH_SCALING2, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"vbat", CHANNEL_VBAT, CHAN_PATH_SCALING2, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"dcin", CHANNEL_DCIN, CHAN_PATH_SCALING4, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"ichg", CHANNEL_ICHG, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"vph_pwr", CHANNEL_VPH_PWR, CHAN_PATH_SCALING2, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"ibat", CHANNEL_IBAT, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"batt_therm", CHANNEL_BATT_THERM, CHAN_PATH_SCALING1, AMUX_RSV2,
		ADC_DECIMATION_TYPE2, ADC_SCALE_BATT_THERM},
	{"batt_id", CHANNEL_BATT_ID, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"usbin", CHANNEL_USBIN, CHAN_PATH_SCALING3, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"pmic_therm", CHANNEL_DIE_TEMP, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_PMIC_THERM},
	{"625mv", CHANNEL_625MV, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"125v", CHANNEL_125V, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"chg_temp", CHANNEL_CHG_TEMP, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"xo_therm", CHANNEL_MUXOFF, CHAN_PATH_SCALING1, AMUX_RSV0,
		ADC_DECIMATION_TYPE2, ADC_SCALE_XOTHERM},
	{"pa_therm0", ADC_MPP_1_AMUX3, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_APQ_THERM},
	{"usb_id_device", ADC_MPP_1_AMUX6, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
};

static struct pm8xxx_adc_properties apq8064_pm8921_adc_data = {
	.adc_vdd_reference	= 1800, /* milli-voltage for this adc */
	.bitresolution		= 15,
	.bipolar                = 0,
};

static struct pm8xxx_adc_platform_data apq8064_pm8921_adc_pdata = {
	.adc_channel		= apq8064_pm8921_adc_channels_data,
	.adc_num_board_channel	= ARRAY_SIZE(apq8064_pm8921_adc_channels_data),
	.adc_prop		= &apq8064_pm8921_adc_data,
	.adc_mpp_base		= PM8921_MPP_PM_TO_SYS(1),
};

static struct pm8xxx_mpp_platform_data
apq8064_pm8921_mpp_pdata __devinitdata = {
	.mpp_base	= PM8921_MPP_PM_TO_SYS(1),
};

static struct pm8xxx_gpio_platform_data
apq8064_pm8921_gpio_pdata __devinitdata = {
	.gpio_base	= PM8921_GPIO_PM_TO_SYS(1),
};

static struct pm8xxx_irq_platform_data
apq8064_pm8921_irq_pdata __devinitdata = {
	.irq_base		= PM8921_IRQ_BASE,
	.devirq			= MSM_GPIO_TO_INT(74),
	.irq_trigger_flag	= IRQF_TRIGGER_LOW,
	.dev_id			= 0,
};

static struct pm8xxx_rtc_platform_data
apq8064_pm8921_rtc_pdata = {
	.rtc_write_enable       = false,
	.rtc_alarm_powerup      = false,
};

static int apq8064_pm8921_therm_mitigation[] = {
	1100,
	700,
	600,
	325,
};

#ifdef CONFIG_LGE_PM
/*
 * J1 battery characteristic
 * Typ.1900mAh capacity, Li-Ion Polymer 3.8V
 * Battery/VDD voltage programmable range, 20mV steps.
 * it will be changed in future
 */
#define MAX_VOLTAGE_MV		4360
#define CHG_TERM_MA		100
static struct pm8921_charger_platform_data apq8064_pm8921_chg_pdata __devinitdata = {

	/* max charging time in minutes incl. fast and trkl. it will be changed in future  */
	.safety_time		= 512, /* 300 change max value for charging time */
	.update_time		= 60000,
	.max_voltage		= MAX_VOLTAGE_MV,

	/* the voltage (mV) where charging method switches from trickle to fast.
	 * This is also the minimum voltage the system operates at */
	.min_voltage		= 3200,
	/* the (mV) drop to wait for before resume charging after the battery has been fully charged */
	.resume_voltage_delta	= 100, //50,
	.resume_charge_percent	= 99,
	.term_current		= CHG_TERM_MA,

	/* the voltage of charger_gone_irq */
	.vin_min			= 4400,

#ifdef CONFIG_LGE_CHARGER_TEMP_SCENARIO
	/* Configuration of cool and warm thresholds (JEITA compliance only) */
	.cool_temp		= 0, /* 10 */	/* 10 degree celsius */
	.warm_temp		= 0, /* 40 */	/* 40 degree celsius */
	.cool_bat_chg_current	= 350,	/* 350 mA (max value = 2A) */
	.warm_bat_chg_current	= 350,
	.temp_level_1		= 550,
	.temp_level_2		= 450,
	.temp_level_3		= 420,
	.temp_level_4		= -50,
	.temp_level_5		= -100,
	/* Temperature Thresholds (JEITA compliance) (-10'C ~ 60'C) */
	.cold_thr		= 1,	/* 80% */
	.hot_thr		= 0,	/* 20% */
#else /* qualcomm original value */
	.cool_temp		= 10,
	.warm_temp		= 40,
	.cool_bat_chg_current	= 350,
	.warm_bat_chg_current	= 350,
#endif

	.temp_check_period	= 1,

#ifdef CONFIG_LGE_PM_LOW_BATT_CHG
	.weak_voltage = 3000,
	.trkl_voltage = 2800,	// trickle voltage 2800mV
	.trkl_current = 200,	// trickle current 200mA
#endif

	/*  Battery charge current programmable range 50mA steps */
	/*  max_bat_chg_current:
	 *    Max charge current of the battery in mA
	 *    Usually 70% of full charge capacity
	 */
	.max_bat_chg_current	= 1800,

	.cool_bat_voltage	= 4100,
	.warm_bat_voltage	= 4100,
	.thermal_mitigation	= apq8064_pm8921_therm_mitigation,
	.thermal_levels		= ARRAY_SIZE(apq8064_pm8921_therm_mitigation),
	/* for led on, off control */
	.led_src_config		= LED_SRC_MIN_VPH_5V,
	.rconn_mohm    = 18,
};
#else /* qualcomm original code */
#define MAX_VOLTAGE_MV          4200
#define CHG_TERM_MA		100
static struct pm8921_charger_platform_data
apq8064_pm8921_chg_pdata __devinitdata = {
	.safety_time		= 180,
	.update_time		= 60000,
	.max_voltage		= MAX_VOLTAGE_MV,
	.min_voltage		= 3200,
	.uvd_thresh_voltage	= 4050,
	.resume_voltage_delta	= 100,
	.term_current		= CHG_TERM_MA,
	.cool_temp		= 10,
	.warm_temp		= 40,
	.temp_check_period	= 1,
	.max_bat_chg_current	= 1100,
	.cool_bat_chg_current	= 350,
	.warm_bat_chg_current	= 350,
	.cool_bat_voltage	= 4100,
	.warm_bat_voltage	= 4100,
	.thermal_mitigation	= apq8064_pm8921_therm_mitigation,
	.thermal_levels		= ARRAY_SIZE(apq8064_pm8921_therm_mitigation),
	.rconn_mohm		= 18,
};
#endif

static struct pm8xxx_ccadc_platform_data
apq8064_pm8xxx_ccadc_pdata = {
	.r_sense		= 10,
	.calib_delay_ms		= 600000,
};

/* 1. Calculate the Rtotal.
* Rbatt : 206.78mohm  (Normal rbatt was 200mohm)
* ( = default_rbatt_mohm X cut off voltage level ) from Rbatt and OCV table
* ex : 98 X 211% (In 2100mAh table )
* (= 98mohm X In OCV table of 3.6V cut off near 3601, it was 5%. So, rbatt was 211)
* Rpwb : 20mohm
* Rlong : 60mohm
* Rtotal  = 286.78mohm
* calc I-test : 697mA
* cut off voltage = v_failure + I-test * Rtotal
*/
#define BMS_2100MAH_OFF3500
static struct pm8921_bms_platform_data
apq8064_pm8921_bms_pdata __devinitdata = {
	.battery_type	= BATT_2200_LGE,
#ifdef BMS_2100MAH_OFF3500
	.battery_type	= BATT_2100_LGE,
#endif
	.ignore_shutdown_soc = 0,
	.r_sense			= 10,
	.v_cutoff			= 3600, //3400,
	.max_voltage_uv			= MAX_VOLTAGE_MV * 1000,
	.rconn_mohm			= 44, //18,
	.shutdown_soc_valid_limit	= 20,
	.adjust_soc_low_threshold	= 25,
	.chg_term_ua   = CHG_TERM_MA * 1000,
};

static unsigned int keymap[] = {
	KEY(0, 0, KEY_VOLUMEUP),
	KEY(0, 1, KEY_VOLUMEDOWN),
	/* KEY_QUICK_CLIP, KEY_HOMEPAGE for GK_KR Rev. F  */
#if defined(CONFIG_MACH_APQ8064_GK_KR) || defined(CONFIG_MACH_APQ8064_GKATT)
	KEY(1, 0, KEY_QUICK_CLIP),
	KEY(1, 1, KEY_HOMEPAGE),
#endif
};

static struct matrix_keymap_data keymap_data = {
	.keymap_size    = ARRAY_SIZE(keymap),
	.keymap         = keymap,
};
#if defined(CONFIG_MACH_APQ8064_GK_KR) || defined(CONFIG_MACH_APQ8064_GKATT)
/* Keymap GK_KR except for Rev.F (EV board) */
static unsigned int keymap_gk_kr[] = {
	KEY(0, 0, KEY_VOLUMEDOWN),
	KEY(0, 1, KEY_VOLUMEUP),
	KEY(1, 0, KEY_QUICK_CLIP),
	KEY(1, 1, KEY_HOMEPAGE),
};

static struct matrix_keymap_data keymap_data_gk_kr = {
	.keymap_size    = ARRAY_SIZE(keymap_gk_kr),
	.keymap         = keymap_gk_kr,
};
#endif

static struct pm8xxx_keypad_platform_data keypad_data = {
	.input_name             = "gk-keypad-8064",
	.input_phys_device      = "gk-keypad-8064/input0",
#if defined(CONFIG_MACH_APQ8064_GK_KR) || defined(CONFIG_MACH_APQ8064_GKATT)
	.num_rows               = 2,
	.num_cols               = 5,
#else
	.num_rows               = 1,
	.num_cols               = 5,
#endif
	.rows_gpio_start	= PM8921_GPIO_PM_TO_SYS(9),
	.cols_gpio_start	= PM8921_GPIO_PM_TO_SYS(1),
	.debounce_ms            = 15,
	.scan_delay_ms          = 32,
	.row_hold_ns            = 91500,
	.wakeup                 = 1,
	.keymap_data            = &keymap_data,
};
static struct pm8921_platform_data
apq8064_pm8921_platform_data __devinitdata = {
	.regulator_pdatas	= msm8064_pm8921_regulator_pdata,
	.irq_pdata		= &apq8064_pm8921_irq_pdata,
	.gpio_pdata		= &apq8064_pm8921_gpio_pdata,
	.mpp_pdata		= &apq8064_pm8921_mpp_pdata,
	.rtc_pdata		= &apq8064_pm8921_rtc_pdata,
	.pwrkey_pdata		= &apq8064_pm8921_pwrkey_pdata,
	.keypad_pdata		= &keypad_data,
	.misc_pdata		= &apq8064_pm8921_misc_pdata,
	.leds_pdata		= &apq8064_pm8921_leds_pdata,
	.adc_pdata		= &apq8064_pm8921_adc_pdata,
	.charger_pdata		= &apq8064_pm8921_chg_pdata,
	.bms_pdata		= &apq8064_pm8921_bms_pdata,
	.ccadc_pdata		= &apq8064_pm8xxx_ccadc_pdata,
};

static struct pm8xxx_irq_platform_data
apq8064_pm8821_irq_pdata __devinitdata = {
	.irq_base		= PM8821_IRQ_BASE,
	.devirq			= PM8821_SEC_IRQ_N,
	.irq_trigger_flag	= IRQF_TRIGGER_HIGH,
	.dev_id			= 1,
};

static struct pm8xxx_mpp_platform_data
apq8064_pm8821_mpp_pdata __devinitdata = {
	.mpp_base	= PM8821_MPP_PM_TO_SYS(1),
};

static struct pm8821_platform_data
apq8064_pm8821_platform_data __devinitdata = {
	.irq_pdata	= &apq8064_pm8821_irq_pdata,
	.mpp_pdata	= &apq8064_pm8821_mpp_pdata,
};

static struct msm_ssbi_platform_data apq8064_ssbi_pm8921_pdata __devinitdata = {
	.controller_type = MSM_SBI_CTRL_PMIC_ARBITER,
	.slave	= {
		.name		= "pm8921-core",
		.platform_data	= &apq8064_pm8921_platform_data,
	},
};

static struct msm_ssbi_platform_data apq8064_ssbi_pm8821_pdata __devinitdata = {
	.controller_type = MSM_SBI_CTRL_PMIC_ARBITER,
	.slave	= {
		.name		= "pm8821-core",
		.platform_data	= &apq8064_pm8821_platform_data,
	},
};

#ifdef CONFIG_BATTERY_MAX17043
#define FUEL_GAUGE_INT_N	36

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

/* START: dukyong.kim@lge.com 2012-01-16 Implement Quickstart for Test Mode and SOC Accurency */
#if 0
static struct max17043_ocv_to_soc_data cal_data[] = {
	{3470,	 0}, {3610,	5}, {3660,	 5}, {3668,	 11},
	{3715,	16}, {3725,	24}, {3772,	 25}, {3781,	 31},
	{3794,	37}, {3853,	49}, {3915,	 59}, {4019,	 70},
	{4110,	81}, {4219,	92}, {4306,	 100}, {-1,	-1},

};
#endif
/* END: dukyong.kim@lge.com 2012-01-16 */

/* BEGIN: hiro.kwon@lge.com 2011-12-22 RCOMP update when the temperature of the cell changes */
static struct max17043_platform_data max17043_pdata = {
	.starting_rcomp	=	0x4C,
	.temp_co_hot	=	-75,
	.temp_co_cold	=	-585,
	//.soc_cal_data	=	cal_data,
};
/* END: hiro.kwon@lge.com 2011-12-22 */
static struct i2c_board_info max17043_i2c_info[] = {
	{
		I2C_BOARD_INFO("max17043", MAX17043_FUELGAUGE_I2C_ADDR),
		/* BEGIN: hiro.kwon@lge.com 2011-12-22 RCOMP update when the temperature of the cell changes */
		.platform_data = (void *)&max17043_pdata,
		/* END: hiro.kwon@lge.com 2011-12-22 */
		.irq = FUEL_GAUGE_INT_N,
	}
};

static struct i2c_registry gk_i2c_pm_subsystem __initdata = {
		I2C_SURF | I2C_FFA | I2C_LIQUID | I2C_RUMI,
		APQ_8064_GSBI1_QUP_I2C_BUS_ID,
		max17043_i2c_info,
		ARRAY_SIZE(max17043_i2c_info),
};

void __init lge_add_i2c_pm_subsystem_devices(void)
{
	/* LGE_CHANGE
	 * 2011-12-03, hyuncheol0.kim@lge.com
	 * Work-around code to support old H/W revision.
	 */

	/* Run the array and install devices as appropriate */
	i2c_register_board_info(gk_i2c_pm_subsystem.bus,
				gk_i2c_pm_subsystem.info,
				gk_i2c_pm_subsystem.len);
}

#endif // CONFIG_BATTERY_MAX17043

void __init apq8064_init_pmic(void)
{
	pmic_reset_irq = PM8921_IRQ_BASE + PM8921_RESOUT_IRQ;

	apq8064_device_ssbi_pmic1.dev.platform_data =
						&apq8064_ssbi_pm8921_pdata;
	apq8064_device_ssbi_pmic2.dev.platform_data =
				&apq8064_ssbi_pm8821_pdata;

	if (socinfo_get_pmic_model() != PMIC_MODEL_PM8917) {
		apq8064_pm8921_platform_data.regulator_pdatas
			= msm8064_pm8921_regulator_pdata;
		apq8064_pm8921_platform_data.num_regulators
			= msm8064_pm8921_regulator_pdata_len;
	} else {
		apq8064_pm8921_platform_data.regulator_pdatas
			= msm8064_pm8917_regulator_pdata;
		apq8064_pm8921_platform_data.num_regulators
			= msm8064_pm8917_regulator_pdata_len;
	}
	if (lge_get_board_revno() == HW_REV_F){
		printk("[%s] lge_get_board_revno() == HW_REV_F, LEDS PORT CHANGE\n", __func__);
		apq8064_pm8921_platform_data.leds_pdata
			= &apq8064_pm8921_leds_pdata_rev_f;
		}
#if defined(CONFIG_MACH_APQ8064_GK_KR) || defined(CONFIG_MACH_APQ8064_GKATT)
	if (!(lge_get_board_revno() == HW_REV_F)){
		keypad_data.keymap_data = &keymap_data_gk_kr;
	}
#endif

#ifndef CONFIG_MACH_LGE
/* LGE_S jungshik.park@lge.com 2012-04-18 for lge battery type */
	if (machine_is_apq8064_mtp()) {
		apq8064_pm8921_bms_pdata.battery_type = BATT_PALLADIUM;
	} else if (machine_is_apq8064_liquid()) {
		apq8064_pm8921_bms_pdata.battery_type = BATT_DESAY;
	} else if (machine_is_apq8064_cdp()) {
		apq8064_pm8921_chg_pdata.has_dc_supply = true;
	}
/* LGE_E jungshik.park@lge.com 2012-04-18 for lge battery type */
#endif

	apq8064_pm8921_adc_pdata.apq_therm = true;
}
