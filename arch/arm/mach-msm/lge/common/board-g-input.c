/* Copyright (c) 2009-2010, Code Aurora Forum. All rights reserved.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/gpio_event.h>

#include <mach/vreg.h>
#include <mach/rpc_server_handset.h>
#include <mach/board.h>

/* keypad */
#include <linux/mfd/pm8xxx/pm8921.h>

/* i2c */
#include <linux/regulator/consumer.h>
#include <linux/i2c.h>

#ifdef CONFIG_TOUCHSCREEN_SYNAPTICS_I2C_RMI4
#include <linux/input/touch_synaptics_rmi4_i2c.h>
#include <linux/input/lge_touch_core.h>
#endif
#include <mach/board_lge.h>

#if defined(CONFIG_RMI4_I2C)
#include <linux/rmi.h>
#endif

//#include "board-j1.h"
#ifdef CONFIG_TOUCHSCREEN_ATMEL_MAXT224E
#include <linux/i2c/atmel_mxt_ts.h>
#endif

/* TOUCH GPIOS */
#define SYNAPTICS_TS_I2C_SDA                 	8
#define SYNAPTICS_TS_I2C_SCL                 	9
#define SYNAPTICS_TS_I2C_INT_GPIO            	6
#define SYNAPTICS_TS_I2C_INT_GPIO_GVDCM       	59
#define TOUCH_RESET                             33
#define TOUCH_POWER_EN                          62

#define TOUCH_FW_VERSION                        1

/* touch screen device */
#define APQ8064_GSBI3_QUP_I2C_BUS_ID            3

int synaptics_t1320_power_on(int on)
{
	int rc = -EINVAL;
	static struct regulator *vreg_l15 = NULL;
	static struct regulator *vreg_l22 = NULL;
#if defined(CONFIG_MACH_APQ8064_GVDCM)
	static struct regulator *vreg_l21 = NULL;
#endif
	/* 3.3V_TOUCH_VDD, VREG_L15: 2.75 ~ 3.3 */
	if (!vreg_l15) {
		vreg_l15 = regulator_get(NULL, "touch_vdd");
		if (IS_ERR(vreg_l15)) {
			pr_err("%s: regulator get of 8921_l15 failed (%ld)\n",
					__func__,
			       PTR_ERR(vreg_l15));
			rc = PTR_ERR(vreg_l15);
			vreg_l15 = NULL;
			return rc;
		}
	}
	/* 1.8V_TOUCH_IO, VREG_L22: 1.7 ~ 2.85 */
	if (!vreg_l22) {
		vreg_l22 = regulator_get(NULL, "touch_io");
		if (IS_ERR(vreg_l22)) {
			pr_err("%s: regulator get of 8921_l22 failed (%ld)\n",
					__func__,
			       PTR_ERR(vreg_l22));
			rc = PTR_ERR(vreg_l22);
			vreg_l22 = NULL;
			return rc;
		}
	}

#if defined(CONFIG_MACH_APQ8064_GVDCM)
        if (lge_get_board_revno() == HW_REV_A) {
		/* 1.8V_TOUCH_IO, VREG_L22: 1.7 ~ 2.85 */
		if (!vreg_l21) {
			vreg_l21 = regulator_get(NULL, "touch_io_temp");
			if (IS_ERR(vreg_l21)) {
				pr_err("%s: regulator get of 8921_l22 failed (%ld)\n",
						__func__,
				       PTR_ERR(vreg_l21));
				rc = PTR_ERR(vreg_l21);
				vreg_l21 = NULL;
				return rc;
			}
		}
	}
#endif
	rc = regulator_set_voltage(vreg_l15, 3300000, 3300000);
	rc |= regulator_set_voltage(vreg_l22, 1800000, 1800000);
#if defined(CONFIG_MACH_APQ8064_GVDCM)
        if (lge_get_board_revno() == HW_REV_A) 
	rc |= regulator_set_voltage(vreg_l21, 1800000, 1800000);
#endif	
	if (rc < 0) {
		printk(KERN_INFO "[Touch D] %s: cannot control regulator\n",
		       __func__);
		return rc;
	}

	if (on) {
		printk("[Touch D]touch enable\n");
		regulator_enable(vreg_l15);
		regulator_enable(vreg_l22);
#if defined(CONFIG_MACH_APQ8064_GVDCM)
     	if (lge_get_board_revno() == HW_REV_A)
		regulator_enable(vreg_l21);
#endif
	} else {
		printk("[Touch D]touch disable\n");
		regulator_disable(vreg_l15);
		regulator_disable(vreg_l22);
#if defined(CONFIG_MACH_APQ8064_GVDCM)
        if (lge_get_board_revno() == HW_REV_A)
		regulator_disable(vreg_l21);
#endif
	}

	return rc;
}

#ifdef CONFIG_TOUCHSCREEN_ATMEL_MAXT224E
int touch_init_hw(bool on)
{
	int rc = -EINVAL;

	if (on) {
		rc = gpio_request(TOUCH_POWER_EN, "touch_power_en");
		if (rc < 0)
			printk("%s not able to get gpio\n", __func__);
		gpio_tlmm_config(GPIO_CFG(TOUCH_POWER_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	} else {
		if (gpio_is_valid(TOUCH_POWER_EN))
			gpio_free(TOUCH_POWER_EN);
	}
	return rc;
}

int touch_power_on(bool on)
{
	int rc = -EINVAL;
	static struct regulator *vreg_l15 = NULL;
	static struct regulator *vreg_l22 = NULL;

#if 0
	rc = gpio_request(TOUCH_RESET, "touch_reset");
	if (rc < 0)
		printk("%s not able to get gpio\n", __func__);
	gpio_set_value(TOUCH_RESET, 0);
#endif


	/* 3.3V_TOUCH_VDD, VREG_L15: 2.75 ~ 3.3 */
	if (!vreg_l15) {
		vreg_l15 = regulator_get(NULL, "touch_vdd");
		if (IS_ERR(vreg_l15)) {
			pr_err("%s: regulator get of 8921_l15 failed (%ld)\n",
					__func__,
			       PTR_ERR(vreg_l15));
			rc = PTR_ERR(vreg_l15);
			vreg_l15 = NULL;
			return rc;
		}
	}
	/* 1.8V_TOUCH_IO, VREG_L22: 1.7 ~ 2.85 */
	if (!vreg_l22) {
		vreg_l22 = regulator_get(NULL, "touch_io");
		if (IS_ERR(vreg_l22)) {
			pr_err("%s: regulator get of 8921_l22 failed (%ld)\n",
					__func__,
			       PTR_ERR(vreg_l22));
			rc = PTR_ERR(vreg_l22);
			vreg_l22 = NULL;
			return rc;
		}
	}

	rc = regulator_set_voltage(vreg_l15, 3300000, 3300000);
	rc |= regulator_set_voltage(vreg_l22, 1800000, 1800000);
	if (rc < 0) {
		printk(KERN_INFO "[Touch D] %s: cannot control regulator\n",
		       __func__);
		return rc;
	}

	if (on) {
		printk("[Touch D]touch enable\n");
		regulator_enable(vreg_l15);
		gpio_set_value(TOUCH_POWER_EN, on);
		printk("%s = TOUCH_POWER_EN value = %d\n\n", __func__, gpio_get_value(TOUCH_POWER_EN));
		regulator_enable(vreg_l22);
	} else {
		printk("[Touch D]touch disable\n");
		regulator_disable(vreg_l15);
		gpio_set_value(TOUCH_POWER_EN, 0);
		printk("%s = TOUCH_POWER_EN value = %d\n\n", __func__, gpio_get_value(TOUCH_POWER_EN));
		regulator_disable(vreg_l22);
	}

	return rc;
}
#endif

static struct touch_power_module touch_pwr = {
	.use_regulator	= 0,
	.vdd			= "8921_l15",
	.vdd_voltage	= 3300000,
	.vio			= "8921_l22",
	.vio_voltage	= 1800000,
	.power			= synaptics_t1320_power_on,
};

static struct touch_device_caps touch_caps = {
	.button_support 			= 1,
	.y_button_boundary			= 0,
#if defined(CONFIG_MACH_APQ8064_GK_KR) || defined(CONFIG_MACH_APQ8064_GKATT) || defined(CONFIG_MACH_APQ8064_GVDCM)
	.number_of_button 			= 2,
	.button_name 				= {KEY_BACK,KEY_MENU},
#else
	.number_of_button 			= 3,
	.button_name 				= {KEY_BACK,KEY_HOMEPAGE,KEY_MENU},
#endif
	.button_margin				= 0,	
	.is_width_supported 		= 1,
	.is_pressure_supported 		= 1,
	.is_id_supported			= 1,
	.max_width 					= 15,
	.max_pressure 				= 0xFF,
	.max_id						= 10,

#if defined(CONFIG_MACH_APQ8064_GK_KR) || defined(CONFIG_MACH_APQ8064_GKATT) || defined(CONFIG_MACH_APQ8064_GVDCM)
	.lcd_x						= 1080,
	.lcd_y						= 1920,
#elif defined(CONFIG_MACH_APQ8064_J1D) || defined(CONFIG_MACH_APQ8064_J1KD)
	.lcd_x						= 720,
	.lcd_y						= 1280,
#else
	.lcd_x						= 768,
	.lcd_y						= 1280,
#endif

#if defined(CONFIG_MACH_APQ8064_J1D) || defined(CONFIG_MACH_APQ8064_J1KD)
	.x_max						= 1440,
#else
	.x_max						= 1536,
#endif
	.y_max						= 2560,
};

static struct touch_operation_role touch_role = {
	.operation_mode 		= INTERRUPT_MODE,
	.key_type				= TOUCH_HARD_KEY,
	.report_mode			= REDUCED_REPORT_MODE,
	.delta_pos_threshold 	= 1,
	.orientation 			= 0,
	.report_period			= 10000000,
	.booting_delay 			= 200,
	.reset_delay			= 5,
	.suspend_pwr			= POWER_OFF,
	.resume_pwr				= POWER_ON,
	.jitter_filter_enable	= 0,
	.jitter_curr_ratio		= 30,
	.accuracy_filter_enable = 1,
	.irqflags 				= IRQF_TRIGGER_FALLING,
#ifdef CUST_G_TOUCH
	.show_touches			= 0,
	.pointer_location		= 0,
	.ta_debouncing_count    = 2,
	.ta_debouncing_finger_num  = 2,
#ifdef CONFIG_MACH_APQ8064_GVDCM
	.ghost_detection_enable = 0,
#else
	.ghost_detection_enable = 1,
#endif
	.pen_enable		= 0,
#endif
};

#ifdef CONFIG_TOUCHSCREEN_ATMEL_MAXT224E
#if 1
static const u8 mxt_config_data[] = {
#if 0
	/*DEBUG_DIAGNOSTIC_T37*/
	17, 8,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
#endif
	/* dummy data start for T6*/
	0, 0, 0, 0, 0, 0,
	/* dummy data end */
	/*SPT_USERDATA_T38*/
	0, 0, 0, 0, 0, 0, 0, 0,
	/*GEN_POWERCONFIG_T7*/
	255, 255, 50, 3,
	/*GEN_ACQUISITIONCONFIG_T8*/
	80, 0, 5, 1, 0, 0, 255, 1, 255, 127,
	/*TOUCH_MULTITOUCHSCREEN_T9*/
	139, 0, 0, 28, 16, 1, 96, 50, 2, 5,
	10, 3, 1, 33, 10, 10, 10, 0, 128, 7,
	56, 4, 0, 0, 0, 0, 0, 0, 0, 0,
	15, 15, 44, 44, 0, 0,
	/*TOUCH_KEYARRAY_T15*/
	3, 26, 16, 2, 1, 4, 80, 40, 2, 0,
	0,
	/*SPT_COMMSCONFIG_T18*/
	0, 0,
	/*SPT_GPIOPWM_T19*/
	0, 0, 0, 0, 0, 0,
	/*TOUCH_PROXIMITY_T23*/
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	/*SPT_SELFTEST_T25*/
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	/*PROCI_GRIPSUPPRESSION_T40*/
	0, 0, 0, 0, 0,
	/*PROCI_TOUCHSUPPRESSION_T42*/
	3, 25, 20, 90, 45, 0, 0, 1, 5, 0,
	/*SPT_CTECONFIG_T46*/
	0, 0, 16, 24, 0, 0, 3, 0, 0, 1,
	/*PROCI_STYLUS_T47*/
	80, 20, 60, 12, 5, 30, 10, 140, 2, 24,
	0, 0, 3, 0, 0, 0, 0, 0, 0, 0,
	0, 0,
	/*PROCI_ADAPTIVETHRESHOLD_T55*/
	0, 0, 0, 0, 0, 0,
	/*PROCI_SHIELDLESS_T56*/
	3, 0, 1, 46, 14, 14, 14, 13, 14, 14,
	14, 14, 14, 14, 14, 14, 14, 14, 13, 14,
	14, 14, 14, 13, 14, 13, 14, 14, 14, 14,
	11, 13, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	/*PROCI_EXTRATOUCHSCREENDATA_T57*/
	0, 0, 0,
	/*SPT_TIMER_T61 (instance 0)*/
	0, 0, 0, 0, 0,
#if 0
	/*SPT_TIMER_T61 (instance 1)*/
	0, 0, 0, 0, 0,
#endif
	/*PROCG_NOISESUPPRESSION_T62*/
	1, 3, 8, 6, 0, 0, 0, 0, 20, 0,
	0, 0, 0, 0, 5, 0, 10, 5, 5, 96,
	25, 0, 18, 5, 63, 6, 6, 6, 64, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0,

};

static struct mxt_config_info mxt_config_array[] = {
	{
		.config		= mxt_config_data,
		.config_length = ARRAY_SIZE(mxt_config_data),
		.family_id	= 0x82,
		.variant_id = 0x1C,
		.version	= 0x09,
		.build		= 0xAA,
		.bootldr_id	= 0x0,
		.fw_name	= "mxt_fw_name",
	},
};
#endif

static int mxt_key_codes[] = {KEY_BACK, KEY_MENU,};

static struct mxt_platform_data atmel_ts_pdata = {
#if 1
	.config_array = mxt_config_array,
	.config_array_size = ARRAY_SIZE(mxt_config_array),
#endif

	.panel_minx = 0,
	.panel_maxx = 1080,
	.panel_miny = 0,
	.panel_maxy = 1920,

	.disp_minx = 0,
	.disp_maxx = 1080,
	.disp_miny = 0,
	.disp_maxy = 1920,

	.irqflags = IRQF_TRIGGER_FALLING,
	.reset_gpio = TOUCH_RESET,
	.irq_gpio = SYNAPTICS_TS_I2C_INT_GPIO,
	.init_hw = touch_init_hw,
	.power_on = touch_power_on,

	.key_codes = mxt_key_codes,
};
#endif

static struct touch_platform_data j1_ts_data = {
	.int_pin	= SYNAPTICS_TS_I2C_INT_GPIO,
	.reset_pin	= TOUCH_RESET,
	.maker		= "Synaptics",
	.fw_version	= "E129",
	.caps		= &touch_caps,
	.role		= &touch_role,
	.pwr		= &touch_pwr,
};

#ifdef CONFIG_TOUCHSCREEN_ATMEL_MAXT224E
static struct i2c_board_info atmel_touch_panel_i2c_bdinfo[] = {
	[0] = {
		I2C_BOARD_INFO("atmel_mxt_ts", 0x4A),
		.platform_data = &atmel_ts_pdata,
		.irq = MSM_GPIO_TO_INT(SYNAPTICS_TS_I2C_INT_GPIO),
	},
};
#endif

static struct i2c_board_info synaptics_ts_info[] = {
	[0] = {
		I2C_BOARD_INFO(LGE_TOUCH_NAME, 0x20),
		.platform_data = &j1_ts_data,
		.irq = MSM_GPIO_TO_INT(SYNAPTICS_TS_I2C_INT_GPIO),
	},
};

void __init apq8064_init_input(void)
{
#ifdef CONFIG_TOUCHSCREEN_ATMEL_MAXT224E
	if (lge_get_board_revno() == HW_REV_F) {
		printk("[%s] lge_get_board_revno() == HW_REV_F, ATMEL IC register\n", __func__);
		i2c_register_board_info(APQ8064_GSBI3_QUP_I2C_BUS_ID,
				&atmel_touch_panel_i2c_bdinfo[0], 1);
	} else {
#endif
		printk(KERN_INFO "[Touch D] %s: NOT DCM KDDI, reg synaptics driver \n",
				__func__);

#if defined(CONFIG_MACH_APQ8064_GVDCM)
        if(lge_get_board_revno() >= HW_REV_C) {
                synaptics_ts_info[0].irq = MSM_GPIO_TO_INT(SYNAPTICS_TS_I2C_INT_GPIO_GVDCM);
                j1_ts_data.int_pin = SYNAPTICS_TS_I2C_INT_GPIO_GVDCM;
        }
#endif
		i2c_register_board_info(APQ8064_GSBI3_QUP_I2C_BUS_ID,
				&synaptics_ts_info[0], 1);
#ifdef CONFIG_TOUCHSCREEN_ATMEL_MAXT224E
	}
#endif
}
