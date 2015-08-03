/*
  * Copyright (C) 2011,2012 LGE, Inc.
  *
  * Author: Sungwoo Cho <sungwoo.cho@lge.com>
  *
  * This software is licensed under the terms of the GNU General
  * License version 2, as published by the Free Software Foundation,
  * may be copied, distributed, and modified under those terms.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
  * GNU General Public License for more details.
  */

#include <linux/types.h>
#include <linux/err.h>
#include <mach/msm_iomap.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <mach/gpiomux.h>

#include <mach/board_lge.h>

#include "devices.h"
#include <linux/android_vibrator.h>

#include <linux/i2c.h>

#ifdef CONFIG_LGE_ISA1200
#include <linux/lge_isa1200.h>
#endif

#ifdef CONFIG_SII8334_MHL_TX
#include <linux/platform_data/mhl_device.h>
#endif

#ifdef CONFIG_BU52031NVX
#include <linux/mfd/pm8xxx/cradle.h>
#endif

#include "board-L05E.h"
/* gpio and clock control for vibrator */

#define REG_WRITEL(value, reg)		writel(value, (MSM_CLK_CTL_BASE+reg))
#define REG_READL(reg)			readl((MSM_CLK_CTL_BASE+reg))

#define GPn_MD_REG(n)                           (0x2D00+32*(n))
#define GPn_NS_REG(n)                           (0x2D24+32*(n))

/* When use SM100 with GP_CLK
  170Hz motor : 22.4KHz - M=1, N=214 ,
  230Hz motor : 29.4KHZ - M=1, N=163 ,
  */

#if !defined(CONFIG_LGE_ISA1200) && !defined(CONFIG_ANDROID_VIBRATOR)
//tmporal code due to build error.. 
	#define GPIO_LIN_MOTOR_EN 0
	#define GPIO_LIN_MOTOR_PWM 0
#endif
/* Vibrator GPIOs */
#ifdef CONFIG_ANDROID_VIBRATOR
#define GPIO_LIN_MOTOR_EN                33
#define GPIO_LIN_MOTOR_PWR               47
#define GPIO_LIN_MOTOR_PWM               3

#define GP_CLK_ID                        0 /* gp clk 0 */
#define GP_CLK_M_DEFAULT                 1
#define GP_CLK_N_DEFAULT                 166
#define GP_CLK_D_MAX                     GP_CLK_N_DEFAULT
#define GP_CLK_D_HALF                    (GP_CLK_N_DEFAULT >> 1)

#define MOTOR_AMP                        120
#endif

#if defined(CONFIG_LGE_ISA1200) || defined(CONFIG_ANDROID_VIBRATOR)
static struct gpiomux_setting vibrator_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

/* gpio 3 */
static struct gpiomux_setting vibrator_active_cfg_gpio3 = {
	.func = GPIOMUX_FUNC_2, /*gp_mn:2 */
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct msm_gpiomux_config gpio2_vibrator_configs[] = {
	{
		.gpio = 3,
		.settings = {
			[GPIOMUX_ACTIVE]    = &vibrator_active_cfg_gpio3,
			[GPIOMUX_SUSPENDED] = &vibrator_suspend_cfg,
		},
	},
};

static int gpio_vibrator_en = 33;
static int gpio_vibrator_pwm = 3;
static int gp_clk_id = 0;

static int vibrator_gpio_init(void)
{
	gpio_vibrator_en = GPIO_LIN_MOTOR_EN;
	gpio_vibrator_pwm = GPIO_LIN_MOTOR_PWM;
/*
                                         
                         
                        
      
  
                       
                       */
	return 0;
}
#endif



#ifdef CONFIG_ANDROID_VIBRATOR
static struct regulator *vreg_l16 = NULL;
static bool snddev_reg_8921_l16_status = false;

static int vibrator_power_set(int enable)
{
	int rc = -EINVAL;
	if (NULL == vreg_l16) {

		vreg_l16 = regulator_get(NULL, "8921_l16");   //2.6 ~ 3V
		INFO_MSG("enable=%d\n", enable);

		if (IS_ERR(vreg_l16)) {
			pr_err("%s: regulator get of 8921_lvs6 failed (%ld)\n"
					, __func__, PTR_ERR(vreg_l16));
			printk("woosock ERROR\n");
			rc = PTR_ERR(vreg_l16);
			return rc;
		}
	}	
	//rc = regulator_set_voltage(vreg_l16, 3000000, 3000000);
	rc = regulator_set_voltage(vreg_l16, 2800000, 2800000);
	
	if(enable == snddev_reg_8921_l16_status) return 0;

	if (enable) {
		rc = regulator_set_voltage(vreg_l16, 2800000, 2800000);
		if (rc < 0)
			pr_err("LGE:  VIB %s: regulator_set_voltage(l1) failed (%d)\n",
			__func__, rc);
			
		rc = regulator_enable(vreg_l16);

		if (rc < 0)
			pr_err("LGE: VIB %s: regulator_enable(l1) failed (%d)\n", __func__, rc);
		snddev_reg_8921_l16_status = true;

	} else {
		rc = regulator_disable(vreg_l16);
		if (rc < 0)
			pr_err("%s: regulator_disable(l1) failed (%d)\n", __func__, rc);
		snddev_reg_8921_l16_status = false;
	}	

	return 0;
}

static int vibrator_pwm_set(int enable, int amp, int n_value)
{
	/* TODO: set clk for amp */
	uint M_VAL = GP_CLK_M_DEFAULT;
	uint D_VAL = GP_CLK_D_MAX;
	uint D_INV = 0;                 /* QCT support invert bit for msm8960 */
	uint clk_id = gp_clk_id;

	INFO_MSG("amp=%d, n_value=%d\n", amp, n_value);

	if (enable) {
		D_VAL = ((GP_CLK_D_MAX * amp) >> 7);
		if (D_VAL > GP_CLK_D_HALF) {
			if (D_VAL == GP_CLK_D_MAX) {      /* Max duty is 99% */
				D_VAL = 2;
			} else {
				D_VAL = GP_CLK_D_MAX - D_VAL;
			}
			D_INV = 1;
		}

		REG_WRITEL(
			(((M_VAL & 0xffU) << 16U) + /* M_VAL[23:16] */
			((~(D_VAL << 1)) & 0xffU)),  /* D_VAL[7:0] */
			GPn_MD_REG(clk_id));

		REG_WRITEL(
			((((~(n_value-M_VAL)) & 0xffU) << 16U) + /* N_VAL[23:16] */
			(1U << 11U) +  /* CLK_ROOT_ENA[11]  : Enable(1) */
			((D_INV & 0x01U) << 10U) +  /* CLK_INV[10]       : Disable(0) */
			(1U << 9U) +   /* CLK_BRANCH_ENA[9] : Enable(1) */
			(1U << 8U) +   /* NMCNTR_EN[8]      : Enable(1) */
			(0U << 7U) +   /* MNCNTR_RST[7]     : Not Active(0) */
			(2U << 5U) +   /* MNCNTR_MODE[6:5]  : Dual-edge mode(2) */
			(3U << 3U) +   /* PRE_DIV_SEL[4:3]  : Div-4 (3) */
			(5U << 0U)),   /* SRC_SEL[2:0]      : CXO (5)  */
			GPn_NS_REG(clk_id));
		INFO_MSG("GPIO_LIN_MOTOR_PWM is enable with M=%d N=%d D=%d\n", M_VAL, n_value, D_VAL);
	} else {
		REG_WRITEL(
			((((~(n_value-M_VAL)) & 0xffU) << 16U) + /* N_VAL[23:16] */
			(0U << 11U) +  /* CLK_ROOT_ENA[11]  : Disable(0) */
			(0U << 10U) +  /* CLK_INV[10]	    : Disable(0) */
			(0U << 9U) +	 /* CLK_BRANCH_ENA[9] : Disable(0) */
			(0U << 8U) +   /* NMCNTR_EN[8]      : Disable(0) */
			(0U << 7U) +   /* MNCNTR_RST[7]     : Not Active(0) */
			(2U << 5U) +   /* MNCNTR_MODE[6:5]  : Dual-edge mode(2) */
			(3U << 3U) +   /* PRE_DIV_SEL[4:3]  : Div-4 (3) */
			(5U << 0U)),   /* SRC_SEL[2:0]      : CXO (5)  */
			GPn_NS_REG(clk_id));
		INFO_MSG("GPIO_LIN_MOTOR_PWM is disalbe \n");
	}

	return 0;
}

static int vibrator_ic_enable_set(int enable)
{
	int gpio_lin_motor_en = 0;
	gpio_lin_motor_en = PM8921_GPIO_PM_TO_SYS(GPIO_LIN_MOTOR_EN);

	INFO_MSG("enable=%d\n", enable);

	//gpio_lin_motor_en = gpio_vibrator_en;

	if (enable)
		gpio_direction_output(gpio_lin_motor_en, 1);
	else
		gpio_direction_output(gpio_lin_motor_en, 0);

	return 0;
}

static int vibrator_init(void)
{
	int rc;
	int gpio_motor_en = 0;
	int gpio_motor_pwm = 0;

	gpio_motor_en = gpio_vibrator_en;
	gpio_motor_pwm = gpio_vibrator_pwm;

	/* GPIO function setting */
	msm_gpiomux_install(gpio2_vibrator_configs, ARRAY_SIZE(gpio2_vibrator_configs));

	/* GPIO setting for Motor EN in pmic8921 */
	gpio_motor_en = PM8921_GPIO_PM_TO_SYS(GPIO_LIN_MOTOR_EN);
	rc = gpio_request(gpio_motor_en, "lin_motor_en");
	if (rc) {
		ERR_MSG("GPIO_LIN_MOTOR_EN %d request failed\n", gpio_motor_en);
		return 0;
	}

	/* gpio init */
	rc = gpio_request(gpio_motor_pwm, "lin_motor_pwm");
	if (unlikely(rc < 0))
		ERR_MSG("not able to get gpio\n");

	vibrator_ic_enable_set(0);
	vibrator_pwm_set(0, 0, GP_CLK_N_DEFAULT);
	vibrator_power_set(0);

	return 0;
}



static struct android_vibrator_platform_data vibrator_data = {
	.enable_status = 0,
	.amp = MOTOR_AMP,
	.vibe_n_value = GP_CLK_N_DEFAULT,
	.power_set = vibrator_power_set,
	.pwm_set = vibrator_pwm_set,
	.ic_enable_set = vibrator_ic_enable_set,
	.vibrator_init = vibrator_init,
};

static struct platform_device android_vibrator_device = {
	.name   = "android-vibrator",
	.id = -1,
	.dev = {
		.platform_data = &vibrator_data,
	},
};
#endif /* CONFIG_ANDROID_VIBRATOR */

#ifdef CONFIG_LGE_ISA1200
static int lge_isa1200_clock(int enable)
{
	if (enable) {
		REG_WRITEL(
		 (((0 & 0xffU) << 16U) + /* N_VAL[23:16] */
		 (1U << 11U) +  /* CLK_ROOT_ENA[11]  : Enable(1) */
		 (0U << 10U) +  /* CLK_INV[10]	   : Disable(0) */
		 (1U << 9U) +    /* CLK_BRANCH_ENA[9] : Enable(1) */
		 (0U << 8U) +   /* NMCNTR_EN[8]	   : Enable(1) */
		 (0U << 7U) +   /* MNCNTR_RST[7]	   : Not Active(0) */
		 (0U << 5U) +   /* MNCNTR_MODE[6:5]  : Dual-edge mode(2) */
		 (0U << 3U) +   /* PRE_DIV_SEL[4:3]  : Div-4 (3) */
		 (0U << 0U)),   /* SRC_SEL[2:0]	   : pxo (0)  */
		 GPn_NS_REG(1));
		 /* printk("GPIO_LIN_MOTOR_PWM is enabled. pxo clock."); */
	} else {
		 REG_WRITEL(
		 (((0 & 0xffU) << 16U) + /* N_VAL[23:16] */
		 (0U << 11U) +  /* CLK_ROOT_ENA[11]  : Disable(0) */
		 (0U << 10U) +  /* CLK_INV[10]	  : Disable(0) */
		 (0U << 9U) +    /* CLK_BRANCH_ENA[9] : Disable(0) */
		 (0U << 8U) +   /* NMCNTR_EN[8]	   : Disable(0) */
		 (1U << 7U) +   /* MNCNTR_RST[7]	   : Not Active(0) */
		 (0U << 5U) +   /* MNCNTR_MODE[6:5]  : Dual-edge mode(2) */
		 (0U << 3U) +   /* PRE_DIV_SEL[4:3]  : Div-4 (3) */
		 (0U << 0U)),   /* SRC_SEL[2:0]	   : pxo (0)  */
		 GPn_NS_REG(1));
		 /* printk("GPIO_LIN_MOTOR_PWM is disabled."); */
	}

	return 0;
}

static struct isa1200_reg_cmd isa1200_init_seq[] = {

	{LGE_ISA1200_HCTRL2, 0x00},		/* bk : release sw reset */

	{LGE_ISA1200_SCTRL , 0x0F}, 		/* LDO:3V */

	{LGE_ISA1200_HCTRL0, 0x10},		/* [4:3]10 : PWM Generation Mode
						[1:0]00 : Divider 1/128 */
	{LGE_ISA1200_HCTRL1, 0xC0},		/* [7] 1 : Ext. Clock Selection, [5] 0:LRA, 1:ERM */
	{LGE_ISA1200_HCTRL3, 0x33},		/* [6:4] 1:PWM/SE Generation PLL Clock Divider */

	{LGE_ISA1200_HCTRL4, 0x81},		/* bk */
	{LGE_ISA1200_HCTRL5, 0x99},		/* [7:0] PWM High Duty(PWM Gen) 0-6B-D6 */ /* TODO make it platform data? */
	{LGE_ISA1200_HCTRL6, 0x9C},		/* [7:0] PWM Period(PWM Gen) */ /* TODO make it platform data? */

};

static struct isa1200_reg_seq isa1200_init = {
	.reg_cmd_list = isa1200_init_seq,
	.number_of_reg_cmd_list = ARRAY_SIZE(isa1200_init_seq),
};

static struct lge_isa1200_platform_data lge_isa1200_platform_data = {
	.vibrator_name = "vibrator",

	.gpio_hen = GPIO_MOTOR_EN,
	.gpio_len = GPIO_MOTOR_EN,

	.clock = lge_isa1200_clock,
	.max_timeout = 30000,
	.default_vib_strength = 255, /* max strength value is 255 */
	.init_seq = &isa1200_init,
};

static struct i2c_board_info lge_i2c_isa1200_board_info[] = {
	{
		I2C_BOARD_INFO("lge_isa1200", ISA1200_SLAVE_ADDR>>1),
		.platform_data = &lge_isa1200_platform_data
	},
};
#endif

#if defined(CONFIG_LGE_ISA1200) || defined(CONFIG_ANDROID_VIBRATOR)
static struct platform_device *misc_devices[] __initdata = {
#ifdef CONFIG_ANDROID_VIBRATOR
	&android_vibrator_device,
#endif
};
#endif

#ifdef CONFIG_SII8334_MHL_TX

#define GPIO_MHL_RESET_N 	31
#define GPIO_MHL_INT_N		43

static int sii8334_mhl_gpio_init(void)
{
	int rc;

	rc = gpio_request(GPIO_MHL_INT_N, "sii8334_mhl_int_n");
	if (rc < 0) {
		pr_err("failed to request sii8334_mhl_int_n gpio\n");
		goto error1;
	}
	gpio_export(GPIO_MHL_INT_N, 1);

	rc = gpio_request(GPIO_MHL_RESET_N, "sii8334_mhl_reset_n");
	if (rc < 0) {
		pr_err("failed to request sii8334_mhl_reset_n gpio\n");
		goto error2;
	}

	rc = gpio_direction_output(GPIO_MHL_RESET_N, 0);
	if (rc < 0) {
		pr_err("failed to set direction for sii8334_mhl_reset_n gpio\n");
		goto error3;
	}

error3:
	gpio_free(GPIO_MHL_RESET_N);
error2:
	gpio_free(GPIO_MHL_INT_N);
error1:

	return rc;
}

static struct regulator *vreg_l18_mhl;

static int sii8334_mhl_power_onoff(bool on, bool pm_ctrl)
{
	static bool power_state=0;
	int rc = 0;

	if (power_state == on) {
		pr_info("sii_power_state is already %s \n",
				power_state ? "on" : "off");
		return rc;
	}
	power_state = on;

	if (!vreg_l18_mhl)
		vreg_l18_mhl = regulator_get(NULL, "8921_l18");

	if (IS_ERR(vreg_l18_mhl)) {
		rc = PTR_ERR(vreg_l18_mhl);
		pr_err("%s: vreg_l18_mhl get failed (%d)\n", __func__, rc);
		return rc;
	}

	if (on) {
		gpio_set_value(GPIO_MHL_RESET_N, 0);
	
		rc = regulator_set_optimum_mode(vreg_l18_mhl, 100000);    
		if (rc < 0) {
			pr_err("%s : set optimum mode 100000,\
				vreg_l18_mhl failed (%d)\n",
				__func__, rc);
			return -EINVAL;
		}
	 
		rc = regulator_set_voltage(vreg_l18_mhl, 1200000, 1200000);
		if (rc < 0) {
			pr_err("%s : set voltage 1200000,\
				vreg_l18_mhl failed (%d)\n",
				__func__, rc);
			return -EINVAL;
		}

		rc = regulator_enable(vreg_l18_mhl);
		if (rc) {
			pr_err("%s : vreg_l18_mhl enable failed (%d)\n",
							__func__, rc);
			return rc;
		}

		msleep(100);
		gpio_set_value(GPIO_MHL_RESET_N, 1);

	}
	else {
		rc = regulator_set_optimum_mode(vreg_l18_mhl, 100);
		if (rc < 0) {
			pr_err("%s : set optimum mode 100,\
				vreg_l18_mhl failed (%d)\n",
				__func__, rc);
			return -EINVAL;
		}
		
		rc = regulator_disable(vreg_l18_mhl);
		if (rc) {
			pr_err("%s : vreg_l18_mhl disable failed (%d)\n",
							__func__, rc);
			return rc;
		}

		gpio_set_value(GPIO_MHL_RESET_N, 0);
	}

	return rc;
}

static struct mhl_platform_data sii8334_mhl_pdata = {
	.power = sii8334_mhl_power_onoff,
};


#define I2C_SURF 1
#define I2C_FFA  (1 << 1)
#define I2C_RUMI (1 << 2)
#define I2C_SIM  (1 << 3)
#define I2C_LIQUID (1 << 4)
/*                                           
                         */
#define I2C_J1V (1 << 5)
/*              */

#define MHL_I2C_DEVICE_TYPE "SiI-833x"

struct i2c_registry {
	u8                     machs;
	int                    bus;
	struct i2c_board_info *info;
	int                    len;
};

struct i2c_board_info i2c_mhl_info[] = {
	{
		I2C_BOARD_INFO(MHL_I2C_DEVICE_TYPE, 0x72 >> 1),  /* 0x39 */
		.irq = MSM_GPIO_TO_INT(GPIO_MHL_INT_N),
		.platform_data = &sii8334_mhl_pdata,
	},
	{
		I2C_BOARD_INFO(MHL_I2C_DEVICE_TYPE, 0x7A >> 1),  /* 0x3D */
	},
	{
		I2C_BOARD_INFO(MHL_I2C_DEVICE_TYPE, 0x92 >> 1), /* 0x49 */
	},
	{
		I2C_BOARD_INFO(MHL_I2C_DEVICE_TYPE, 0x9A >> 1), /* 0x4D */
	},
	{
		I2C_BOARD_INFO(MHL_I2C_DEVICE_TYPE, 0xC8 >> 1), /*  0x64 */
	},
};

static struct i2c_registry i2c_mhl_devices __initdata = {
	I2C_SURF | I2C_FFA | I2C_RUMI | I2C_SIM | I2C_LIQUID | I2C_J1V,
	APQ_8064_GSBI1_QUP_I2C_BUS_ID,
	i2c_mhl_info,
	ARRAY_SIZE(i2c_mhl_info),
};

static void __init lge_add_i2c_mhl_device(void)
{
	i2c_register_board_info(i2c_mhl_devices.bus,
							i2c_mhl_devices.info,
							i2c_mhl_devices.len);
}

#endif  /* CONFIG_SII8334_MHL_TX */

#ifdef CONFIG_BU52031NVX
#define GPIO_POUCH_INT		22
#define GPIO_CARKIT_INT		23

static unsigned hall_ic_int_gpio[] = {GPIO_POUCH_INT, GPIO_CARKIT_INT};

static unsigned hall_ic_gpio[] = {
	GPIO_CFG(22, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(23, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

static void __init hall_ic_init(void)
{
	int rc = 0;
	int i = 0;

	printk(KERN_INFO "%s, line: %d\n", __func__, __LINE__);

	rc = gpio_request(GPIO_POUCH_INT, "cradle_detect_s");	
	if (rc) {		
		printk(KERN_ERR	"%s: pouch_int  %d request failed\n",
			__func__, GPIO_POUCH_INT);
		return;
	}

	rc = gpio_request(GPIO_CARKIT_INT, "cradle_detect_n");	
	if (rc) {		
		printk(KERN_ERR	"%s: pouch_int  %d request failed\n",
			__func__, GPIO_CARKIT_INT);
		return;
	}

	for(i=0; i<ARRAY_SIZE(hall_ic_gpio); i++){
		rc = gpio_tlmm_config(hall_ic_gpio[i], GPIO_CFG_ENABLE);
		gpio_direction_input(hall_ic_int_gpio[i]);

		if(rc) {
			printk(KERN_ERR "%s: gpio_tlmm_config(%#x)=fg%d\n",
				__func__, hall_ic_gpio[i], rc);			
			return;			
		}
	}

	printk(KERN_INFO "yoogyeong.lee@%s_END\n", __func__);
}

static struct pm8xxx_cradle_platform_data cradle_data = {
#if defined(CONFIG_BU52031NVX_POUCHDETECT)
	.pouch_detect_pin = GPIO_POUCH_INT,
	.pouch_irq = MSM_GPIO_TO_INT(GPIO_POUCH_INT),
#endif
	.carkit_detect_pin = GPIO_CARKIT_INT,
	.carkit_irq = MSM_GPIO_TO_INT(GPIO_CARKIT_INT),
	.irq_flags = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
};

static struct platform_device cradle_device = {
	.name   = PM8XXX_CRADLE_DEV_NAME,
	.id = -1,
	.dev = {
		.platform_data = &cradle_data,
	},
};

#endif

void __init apq8064_init_misc(void)
{
#if defined(CONFIG_LGE_ISA1200) || defined(CONFIG_ANDROID_VIBRATOR)
	int vib_flag = 0;
	INFO_MSG("%s\n", __func__);

	vibrator_gpio_init();
#endif

#if defined(CONFIG_ANDROID_VIBRATOR)
#if defined(CONFIG_MACH_APQ8064_J1V) || defined(CONFIG_MACH_APQ8064_J1U) || defined(CONFIG_MACH_APQ8064_J1A) || defined(CONFIG_MACH_APQ8064_J1SP) || defined(CONFIG_MACH_APQ8064_J1D) || defined(CONFIG_MACH_APQ8064_J1SK) || defined(CONFIG_MACH_APQ8064_J1KT) || defined(CONFIG_MACH_APQ8064_J1KD) || defined(CONFIG_MACH_APQ8064_J1R) || defined(CONFIG_MACH_APQ8064_J1B)|| defined(CONFIG_MACH_APQ8064_J1VD)|| defined(CONFIG_MACH_APQ8064_J1X)|| defined(CONFIG_MACH_APQ8064_J1TL) || defined(CONFIG_MACH_APQ8064_J1TM) || defined(CONFIG_MACH_APQ8064_GKU) || defined(CONFIG_MACH_APQ8064_GKSK) || defined(CONFIG_MACH_APQ8064_GKKT) || defined(CONFIG_MACH_APQ8064_GKATT) || defined(CONFIG_MACH_APQ8064_L05E)
	if (vib_flag == 0) {
		platform_add_devices(misc_devices, ARRAY_SIZE(misc_devices));
	}
#endif
#endif

#ifdef CONFIG_SII8334_MHL_TX
	sii8334_mhl_gpio_init();
	lge_add_i2c_mhl_device();
#endif 

#ifdef CONFIG_BU52031NVX
/*	int rc = 0;

	rc = gpio_request(67, "CRADLE_DETECT_N");
	if (rc) {
		pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
			"CRADLE_DETECT_N", 67, rc);
	}

	rc = gpio_request(10, "CRADLE_DETECT_S");
	if (rc) {
		pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
			"CRADLE_DETECT_N", 10, rc);
	}
*/
	hall_ic_init();
	platform_device_register(&cradle_device);
#endif
}
