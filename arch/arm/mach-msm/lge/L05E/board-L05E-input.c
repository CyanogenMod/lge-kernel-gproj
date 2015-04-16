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
#ifdef CONFIG_TOUCHSCREEN_S340010_SYNAPTICS_TK
#include <linux/input/synaptics_so340010_keytouch.h>
#endif
#include <mach/board_lge.h>
#include "board-L05E.h"

#if defined(CONFIG_RMI4_I2C)
#include <linux/rmi.h>
#endif
#if defined (CONFIG_I2C_GPIO)
#include <linux/i2c-gpio.h>
#endif

/* TOUCH GPIOS */
#define SYNAPTICS_TS_I2C_SDA                 	8
#define SYNAPTICS_TS_I2C_SCL                 	9
#define SYNAPTICS_TS_I2C_INT_GPIO            	72
#define TOUCH_RESET                             33
#define TOUCH_FW_VERSION                        1

#ifdef CONFIG_TOUCHSCREEN_S340010_SYNAPTICS_TK
#define SYNAPTICS_T1320_TS_PWR					PM8921_GPIO_PM_TO_SYS(21)
#endif

int synaptics_t1320_power_on(int on)
{
	int rc = -EINVAL;
	static struct regulator *vreg_l15 = NULL;
	static struct regulator *vreg_l22 = NULL;

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
		regulator_enable(vreg_l22);
	} else {
		printk("[Touch D]touch disable\n");
		regulator_disable(vreg_l15);
		regulator_disable(vreg_l22);
	}

	return rc;
}

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
	.number_of_button 			= 3,
	.button_name 				= {KEY_BACK,KEY_HOMEPAGE,KEY_MENU},
	.button_margin				= 0,	
	.is_width_supported 		= 1,
	.is_pressure_supported 		= 1,
	.is_id_supported			= 1,
	.max_width 					= 15,
	.max_pressure 				= 0xFF,
	.max_id						= 10,
	.lcd_x						= 720,
	.lcd_y						= 1280,
	.x_max						= 1100,
	.y_max						= 1900,
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
	.ghost_detection_enable = 1,
	.pen_enable		= 0,
#endif
};

#ifdef CONFIG_TOUCHSCREEN_S340010_SYNAPTICS_TK
int tk_set_vreg_a(int on, bool log_en);
int tk_set_vreg_c(int on, bool log_en);

static unsigned short key_touch_map[] = {
	KEY_RESERVED,
	KEY_HOMEPAGE,
	KEY_BACK,
	KEY_MENU,
};
#endif

static struct touch_platform_data j1_ts_data = {
	.int_pin	= SYNAPTICS_TS_I2C_INT_GPIO,
	.reset_pin	= TOUCH_RESET,
	.maker		= "Synaptics",
	.fw_version	= "",
	.caps		= &touch_caps,
	.role		= &touch_role,
	.pwr		= &touch_pwr,
};

#ifdef CONFIG_TOUCHSCREEN_S340010_SYNAPTICS_TK
/* L05E HW Rev.A, Rev.B */
static struct key_touch_platform_data tk_platform_data_a = {
	.tk_power	= tk_set_vreg_a,
	.irq   	= KEY_TOUCH_GPIO_INT,
	.scl  	= KEY_TOUCH_GPIO_I2C1_SCL,
	.sda  	= KEY_TOUCH_GPIO_I2C1_SDA,
//	.ldo    = SYNAPTICS_T1320_TS_PWR,
	.keycode = (unsigned char *)key_touch_map,
	.keycodemax = (ARRAY_SIZE(key_touch_map) * 2),
	.tk_power_flag = 0,
};

/* L05E HW Rev.C */
static struct key_touch_platform_data tk_platform_data_c = {
	.tk_power	= tk_set_vreg_c,
	.irq   	= KEY_TOUCH_GPIO_INT,
	.scl  	= KEY_TOUCH_GPIO_I2C2_SCL,
	.sda  	= KEY_TOUCH_GPIO_I2C2_SDA,
//	.ldo    = SYNAPTICS_T1320_TS_PWR,
	.keycode = (unsigned char *)key_touch_map,
	.keycodemax = (ARRAY_SIZE(key_touch_map) * 2),
	.tk_power_flag = 0,
};
#endif

static struct i2c_board_info synaptics_ts_info[] = {
	[0] = {
		I2C_BOARD_INFO(LGE_TOUCH_NAME, 0x20),
		.platform_data = &j1_ts_data,
		.irq = MSM_GPIO_TO_INT(SYNAPTICS_TS_I2C_INT_GPIO),
	},
};

#ifdef CONFIG_TOUCHSCREEN_S340010_SYNAPTICS_TK
static struct i2c_board_info key_touch_i2c_bdinfo[] = {
	/* L05E HW Rev.A, Rev.B */
	[0] = {
		I2C_BOARD_INFO(SYNAPTICS_KEYTOUCH_NAME, SYNAPTICS_KEYTOUCH_I2C_SLAVE_ADDR),
		.type = "so340010",
		.platform_data = &tk_platform_data_a,
	},
	/* L05E HW Rev.C ~ */
	[1] = {
		I2C_BOARD_INFO(SYNAPTICS_KEYTOUCH_NAME, SYNAPTICS_KEYTOUCH_I2C_SLAVE_ADDR),
				.type = "so340010",
				.platform_data = &tk_platform_data_c,
	},
};


static bool touch_dev_first_boot = false;
int tk_set_vreg_a(int on, bool log_en)
{

	if (on) {
		if (!tk_platform_data_a.tk_power_flag) {
			mdelay(20);
			touch_dev_first_boot = true;
			mdelay(45);
		} else {
			;
		}
		tk_platform_data_a.tk_power_flag = 1;
	} else {
		tk_platform_data_a.tk_power_flag = 0;
		if (!tk_platform_data_a.tk_power_flag) {
			;
		} else {
			;
		}
	}
	return 0;
}

int tk_set_vreg_c(int on, bool log_en)
{
	int rc = -EINVAL;
	static struct regulator *vreg_l21 = NULL;
	static int pm8921_gpio21;

	pm8921_gpio21 = SYNAPTICS_T1320_TS_PWR;

	vreg_l21 = regulator_get(NULL, "8921_l21");
	if (IS_ERR(vreg_l21)) {
		pr_err("could not get vreg_l21, rc = %ld\n", PTR_ERR(vreg_l21));
		return -ENODEV;
	}

	rc = regulator_set_voltage(vreg_l21, 1800000, 1800000);
	if (rc) {
		pr_err("set_voltage vreg_l21 failed, rc=%d\n", rc);
		return -EINVAL;
	}

	if (on) {
		gpio_set_value(pm8921_gpio21, 1);
		
		rc = regulator_set_optimum_mode(vreg_l21, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode vreg_l21 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_enable(vreg_l21);  
		if (rc) {
			pr_err("enable vreg_l21 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		pr_info("%s: tk power on\n", __func__);
		mdelay(3);
	}else {
		rc = regulator_disable(vreg_l21); 	
		if (rc) {
			pr_err("disable vreg_l21 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		gpio_set_value(pm8921_gpio21, 0);
		pr_info(" %s: tk power off\n", __func__);
	}
	return 0;
}
#endif

#if defined (CONFIG_I2C_GPIO)
int init_gpio_i2c_pin(struct i2c_gpio_platform_data *i2c_adap_pdata)
{
    gpio_tlmm_config(GPIO_CFG(i2c_adap_pdata->sda_pin, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
    gpio_tlmm_config(GPIO_CFG(i2c_adap_pdata->scl_pin, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
    gpio_set_value(i2c_adap_pdata->sda_pin, 1);
    gpio_set_value(i2c_adap_pdata->scl_pin, 1);

    return 0;
}

static struct i2c_gpio_platform_data touch_key_i2c_pdata = {
    .sda_pin = 84,
    .scl_pin = 85,
    .sda_is_open_drain  = 0,
    .scl_is_open_drain  = 0,
    .udelay             = 2,
};

static struct platform_device touch_key_i2c_device = {
    .id      = APQ_8064_GSBI7_GPIO_I2C_BUS_ID,
    .name    = "i2c-gpio",
    .dev.platform_data = &touch_key_i2c_pdata,
};
#endif

void __init apq8064_init_input(void)
{
		i2c_register_board_info(APQ_8064_GSBI3_QUP_I2C_BUS_ID,
				&synaptics_ts_info[0], 1);

#ifdef CONFIG_TOUCHSCREEN_S340010_SYNAPTICS_TK
		if (lge_get_board_revno() < HW_REV_C){
			i2c_register_board_info(APQ_8064_GSBI3_QUP_I2C_BUS_ID,
				&key_touch_i2c_bdinfo[0], 1);
		}else{
#if defined (CONFIG_I2C_GPIO)
			i2c_register_board_info(APQ_8064_GSBI7_GPIO_I2C_BUS_ID,
				&key_touch_i2c_bdinfo[1], 1);
#else
			i2c_register_board_info(APQ_8064_GSBI7_QUP_I2C_BUS_ID,
				&key_touch_i2c_bdinfo[1], 1);
#endif
		}
#if defined (CONFIG_I2C_GPIO)
	    init_gpio_i2c_pin(&touch_key_i2c_pdata);
		platform_device_register(&touch_key_i2c_device);
#endif
#endif
}
