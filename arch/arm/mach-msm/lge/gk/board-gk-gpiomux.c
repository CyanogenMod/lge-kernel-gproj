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
#include <linux/platform_device.h>
#include <linux/bootmem.h>
#include <linux/gpio.h>
#include <asm/mach-types.h>
#include <asm/mach/mmc.h>
#include <mach/msm_bus_board.h>
#include <mach/board.h>
#include <mach/gpiomux.h>
#include <mach/socinfo.h>
#include "devices.h"
#include "board-gk.h"

#include <mach/board_lge.h>
#if !defined(CONFIG_MACH_LGE)
#if defined(CONFIG_KS8851) || defined(CONFIG_KS8851_MODULE)
static struct gpiomux_setting gpio_eth_config = {
	.pull = GPIOMUX_PULL_NONE,
	.drv = GPIOMUX_DRV_8MA,
	.func = GPIOMUX_FUNC_GPIO,
};

/* The SPI configurations apply to GSBI 5*/
static struct gpiomux_setting gpio_spi_config = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_12MA,
	.pull = GPIOMUX_PULL_NONE,
};

/* The SPI configurations apply to GSBI 5 chip select 2*/
static struct gpiomux_setting gpio_spi_cs2_config = {
	.func = GPIOMUX_FUNC_3,
	.drv = GPIOMUX_DRV_12MA,
	.pull = GPIOMUX_PULL_NONE,
};

/* Chip selects for SPI clients */
static struct gpiomux_setting gpio_spi_cs_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_12MA,
	.pull = GPIOMUX_PULL_UP,
};

/* Chip selects for EPM SPI clients */
static struct gpiomux_setting gpio_epm_spi_cs_config = {
	.func = GPIOMUX_FUNC_6,
	.drv = GPIOMUX_DRV_12MA,
	.pull = GPIOMUX_PULL_UP,
};

struct msm_gpiomux_config apq8064_ethernet_configs[] = {
	{
		.gpio = 43,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_eth_config,
			[GPIOMUX_ACTIVE] = &gpio_eth_config,
		}
	},
};
#endif
#endif /* CONFIG_MACH_LGE */

#if CONFIG_SWITCH_MAX1462X
static struct gpiomux_setting ear_key_int = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};
#endif
// [S] LGE_BT: ADD/ilbeom.kim/'12-10-24 - [GK] BRCM Solution bring-up
#ifdef CONFIG_LGE_BLUESLEEP
//BEGIN: 0019632 chanha.park@lge.com 2012-05-31
//ADD: 0019632: [F200][BT] Bluetooth board bring-up
/*static struct gpiomux_setting gsbi6 = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};
static struct gpiomux_setting gsbi6_suspend = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};
*/

static struct gpiomux_setting bt_pcm = {
    .func = GPIOMUX_FUNC_1,
    .drv = GPIOMUX_DRV_2MA,
    .pull = GPIOMUX_PULL_NONE,
};
static struct gpiomux_setting bt_pcm_suspend = {
    .func = GPIOMUX_FUNC_GPIO,
    .drv = GPIOMUX_DRV_2MA,
    .pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting bt_host_wakeup_active_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting bt_host_wakeup_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting bt_wakeup_active_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = /*GPIOMUX_PULL_UP,*/ GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting bt_wakeup_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = /*GPIOMUX_PULL_UP,*/ GPIOMUX_PULL_NONE,
};
#endif /* CONFIG_LGE_BLUESLEEP */
//END: 0019632 chanha.park@lge.com 2012-05-31
// [E] LGE_BT: ADD/ilbeom.kim/'12-10-24 - [GK] BRCM Solution bring-up

#if defined(CONFIG_LGE_BROADCAST_TDMB) 
static struct gpiomux_setting gsbi5_spi_config= {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gsbi5_spi_suspend_config= {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting dmb_ctrl_pin = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting dmb_int_pin = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};
#endif /* CONFIG_LGE_BROADCAST */


#ifdef CONFIG_MSM_VCAP
static struct gpiomux_setting gpio_vcap_config[] = {
	{
		.func = GPIOMUX_FUNC_GPIO,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
	},
	{
		.func = GPIOMUX_FUNC_1,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
	},
	{
		.func = GPIOMUX_FUNC_2,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
	},
	{
		.func = GPIOMUX_FUNC_3,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
	},
	{
		.func = GPIOMUX_FUNC_4,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
	},
	{
		.func = GPIOMUX_FUNC_5,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
	},
	{
		.func = GPIOMUX_FUNC_6,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
	},
	{
		.func = GPIOMUX_FUNC_7,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
	},
	{
		.func = GPIOMUX_FUNC_8,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
	},
	{
		.func = GPIOMUX_FUNC_9,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
	},
	{
		.func = GPIOMUX_FUNC_A,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
	},
};

struct msm_gpiomux_config vcap_configs[] = {
	{
		.gpio = 20,
		.settings = {
			[GPIOMUX_SUSPENDED] =	&gpio_vcap_config[7],
			[GPIOMUX_ACTIVE] =		&gpio_vcap_config[7],
		}
	},
	{
		.gpio = 25,
		.settings = {
			[GPIOMUX_SUSPENDED] =	&gpio_vcap_config[2],
			[GPIOMUX_ACTIVE] =		&gpio_vcap_config[2],
		}
	},
	{
		.gpio = 24,
		.settings = {
			[GPIOMUX_SUSPENDED] =	&gpio_vcap_config[1],
			[GPIOMUX_ACTIVE] =		&gpio_vcap_config[1],
		}
	},
	{
		.gpio = 23,
       #ifdef CONFIG_SWITCH_MAX1462X
		.settings = {
			[GPIOMUX_SUSPENDED] =	&ear_key_int,
			[GPIOMUX_ACTIVE] =		&ear_key_int,
		}
       #endif
       #ifdef CONFIG_SWITCH_FSA8008
              .settings = {
			[GPIOMUX_SUSPENDED] =	&gpio_vcap_config[2],
			[GPIOMUX_ACTIVE] =		&gpio_vcap_config[2],
		}
       #endif
	},
	{
		.gpio = 19,
		.settings = {
			[GPIOMUX_SUSPENDED] =	&gpio_vcap_config[8],
			[GPIOMUX_ACTIVE] =		&gpio_vcap_config[8],
		}
	},
	{
		.gpio = 22,
		.settings = {
			[GPIOMUX_SUSPENDED] =	&gpio_vcap_config[2],
			[GPIOMUX_ACTIVE] =		&gpio_vcap_config[2],
		}
	},
	{
		.gpio = 21,
		.settings = {
			[GPIOMUX_SUSPENDED] =	&gpio_vcap_config[7],
			[GPIOMUX_ACTIVE] =		&gpio_vcap_config[7],
		}
	},
	{
		.gpio = 12,
		.settings = {
			[GPIOMUX_SUSPENDED] =	&gpio_vcap_config[6],
			[GPIOMUX_ACTIVE] =		&gpio_vcap_config[6],
		}
	},
	{
		.gpio = 18,
		.settings = {
			[GPIOMUX_SUSPENDED] =	&gpio_vcap_config[9],
			[GPIOMUX_ACTIVE] =		&gpio_vcap_config[9],
		}
	},
	{
		.gpio = 11,
		.settings = {
			[GPIOMUX_SUSPENDED] =	&gpio_vcap_config[10],
			[GPIOMUX_ACTIVE] =		&gpio_vcap_config[10],
		}
	},
	{
		.gpio = 10,
		.settings = {
			[GPIOMUX_SUSPENDED] =	&gpio_vcap_config[9],
			[GPIOMUX_ACTIVE] =		&gpio_vcap_config[9],
		}
	},
	{
		.gpio = 9,
		.settings = {
			[GPIOMUX_SUSPENDED] =	&gpio_vcap_config[2],
			[GPIOMUX_ACTIVE] =		&gpio_vcap_config[2],
		}
	},
	{
		.gpio = 26,
		.settings = {
			[GPIOMUX_SUSPENDED] =	&gpio_vcap_config[1],
			[GPIOMUX_ACTIVE] =		&gpio_vcap_config[1],
		}
	},
	{
		.gpio = 8,
		.settings = {
			[GPIOMUX_SUSPENDED] =	&gpio_vcap_config[3],
			[GPIOMUX_ACTIVE] =		&gpio_vcap_config[3],
		}
	},
	{
		.gpio = 7,
		.settings = {
			[GPIOMUX_SUSPENDED] =	&gpio_vcap_config[7],
			[GPIOMUX_ACTIVE] =		&gpio_vcap_config[7],
		}
	},
	{
		.gpio = 6,
		.settings = {
			[GPIOMUX_SUSPENDED] =	&gpio_vcap_config[7],
			[GPIOMUX_ACTIVE] =		&gpio_vcap_config[7],
		}
	},
	{
		.gpio = 80,
		.settings = {
			[GPIOMUX_SUSPENDED] =	&gpio_vcap_config[2],
			[GPIOMUX_ACTIVE] =		&gpio_vcap_config[2],
		}
	},
	{
		.gpio = 86,
		.settings = {
			[GPIOMUX_SUSPENDED] =	&gpio_vcap_config[1],
			[GPIOMUX_ACTIVE] =		&gpio_vcap_config[1],
		}
	},
	{
		.gpio = 85,
		.settings = {
			[GPIOMUX_SUSPENDED] =	&gpio_vcap_config[4],
			[GPIOMUX_ACTIVE] =		&gpio_vcap_config[4],
		}
	},
	{
		.gpio = 84,
		.settings = {
			[GPIOMUX_SUSPENDED] =	&gpio_vcap_config[3],
			[GPIOMUX_ACTIVE] =		&gpio_vcap_config[3],
		}
	},
	{
		.gpio = 5,
		.settings = {
			[GPIOMUX_SUSPENDED] =	&gpio_vcap_config[2],
			[GPIOMUX_ACTIVE] =		&gpio_vcap_config[2],
		}
	},
	{
		.gpio = 4,
		.settings = {
			[GPIOMUX_SUSPENDED] =	&gpio_vcap_config[3],
			[GPIOMUX_ACTIVE] =		&gpio_vcap_config[3],
		}
	},
	{
		.gpio = 3,
		.settings = {
			[GPIOMUX_SUSPENDED] =	&gpio_vcap_config[6],
			[GPIOMUX_ACTIVE] =		&gpio_vcap_config[6],
		}
	},
	{
		.gpio = 2,
		.settings = {
			[GPIOMUX_SUSPENDED] =	&gpio_vcap_config[5],
			[GPIOMUX_ACTIVE] =		&gpio_vcap_config[5],
		}
	},
	{
		.gpio = 82,
		.settings = {
			[GPIOMUX_SUSPENDED] =	&gpio_vcap_config[4],
			[GPIOMUX_ACTIVE] =		&gpio_vcap_config[4],
		}
	},
	{
		.gpio = 83,
		.settings = {
			[GPIOMUX_SUSPENDED] =	&gpio_vcap_config[4],
			[GPIOMUX_ACTIVE] =		&gpio_vcap_config[4],
		}
	},
	{
		.gpio = 87,
		.settings = {
			[GPIOMUX_SUSPENDED] =	&gpio_vcap_config[2],
			[GPIOMUX_ACTIVE] =		&gpio_vcap_config[2],
		}
	},
	{
		.gpio = 13,
		.settings = {
			[GPIOMUX_SUSPENDED] =	&gpio_vcap_config[6],
			[GPIOMUX_ACTIVE] =		&gpio_vcap_config[6],
		}
	},
};
#endif

static struct gpiomux_setting gpio_i2c_config = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gpio_i2c_2ma_config = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};


static struct gpiomux_setting gpio_i2c_config_sus = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_KEEPER,
};

static struct gpiomux_setting mbhc_hs_detect = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting cdc_mclk = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

#ifndef CONFIG_MMC_MSM_SDC4_SUPPORT
static struct gpiomux_setting wcnss_5wire_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv  = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting wcnss_5wire_active_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv  = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_DOWN,
};
#endif

static struct gpiomux_setting slimbus = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_KEEPER,
};

static struct gpiomux_setting gsbi1_uart_config = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_16MA,
	.pull = GPIOMUX_PULL_NONE,
};


#if !defined(CONFIG_MACH_LGE)
static struct gpiomux_setting gsbi7_func1_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gsbi7_func2_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};
#endif /* CONFIG_MACH_LGE */

#if defined(CONFIG_LGE_IRRC)
static struct gpiomux_setting gsbi7_irrc_TXD = {
       .func = GPIOMUX_FUNC_2,
       .drv = GPIOMUX_DRV_8MA,
       .pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gsbi7_irrc_RXD = {
       .func = GPIOMUX_FUNC_1,
       .drv = GPIOMUX_DRV_8MA,
       .pull = GPIOMUX_PULL_NONE,
};
#endif

static struct gpiomux_setting gsbi3_suspended_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_KEEPER,
};

static struct gpiomux_setting gsbi3_active_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

//2012-10-30 soodong.kim@lge.com : set GPIO initial value to PULL_UP [START]
#if defined (CONFIG_SLIMPORT_ANX7808)
static struct gpiomux_setting slimport_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_IN,
};
#endif
//2012-10-30 soodong.kim@lge.com : set GPIO initial value to PULL_UP [END]

static struct gpiomux_setting hdmi_suspend_1_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting hdmi_suspend_2_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting hdmi_active_1_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting hdmi_active_2_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_16MA,
	.pull = GPIOMUX_PULL_DOWN,
};

#if !defined (CONFIG_MACH_LGE)
static struct gpiomux_setting hdmi_active_3_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting hdmi_active_4_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_OUT_HIGH,
};
#endif

static struct gpiomux_setting gsbi5_suspended_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_12MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gsbi5_active_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_12MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting sx150x_suspended_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting sx150x_active_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

#if defined(CONFIG_MACH_LGE)
static struct gpiomux_setting gsbi4_uart = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_DOWN,
};
static struct gpiomux_setting gsbi4_uart_active = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};
#endif

#ifdef CONFIG_USB_EHCI_MSM_HSIC
static struct gpiomux_setting cyts_sleep_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting cyts_sleep_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting cyts_int_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting cyts_int_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct msm_gpiomux_config cyts_gpio_configs[] __initdata = {
	{	/* TS INTERRUPT */
		.gpio = 6,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cyts_int_act_cfg,
			[GPIOMUX_SUSPENDED] = &cyts_int_sus_cfg,
		},
	},
	{	/* TS SLEEP */
		.gpio = 33,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cyts_sleep_act_cfg,
			[GPIOMUX_SUSPENDED] = &cyts_sleep_sus_cfg,
		},
	},
};
static struct msm_gpiomux_config cyts_gpio_alt_config[] __initdata = {
	{	/* TS INTERRUPT */
		.gpio = 6,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cyts_int_act_cfg,
			[GPIOMUX_SUSPENDED] = &cyts_int_sus_cfg,
		},
	},
	{	/* TS SLEEP */
		.gpio = 12,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cyts_sleep_act_cfg,
			[GPIOMUX_SUSPENDED] = &cyts_sleep_sus_cfg,
		},
	},
};

static struct gpiomux_setting hsic_act_cfg = {
	.func = GPIOMUX_FUNC_1,
#ifdef CONFIG_USB_G_LGE_ANDROID
	.drv = GPIOMUX_DRV_10MA,
#else
	.drv = GPIOMUX_DRV_8MA,
#endif
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting hsic_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_OUT_LOW,
};

static struct gpiomux_setting hsic_wakeup_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting hsic_wakeup_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};

static struct msm_gpiomux_config apq8064_hsic_configs[] = {
	{
		.gpio = 88,               /*HSIC_STROBE */
		.settings = {
			[GPIOMUX_ACTIVE] = &hsic_act_cfg,
			[GPIOMUX_SUSPENDED] = &hsic_sus_cfg,
		},
	},
	{
		.gpio = 89,               /* HSIC_DATA */
		.settings = {
			[GPIOMUX_ACTIVE] = &hsic_act_cfg,
			[GPIOMUX_SUSPENDED] = &hsic_sus_cfg,
		},
	},
	{
		.gpio = 47,              /* wake up */
		.settings = {
			[GPIOMUX_ACTIVE] = &hsic_wakeup_act_cfg,
			[GPIOMUX_SUSPENDED] = &hsic_wakeup_sus_cfg,
		},
	},
};
#endif

static struct gpiomux_setting mxt_reset_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting mxt_reset_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting mxt_int_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting mxt_int_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct msm_gpiomux_config apq8064_hdmi_configs[] __initdata = {
	{
		.gpio = 69,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_1_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_1_cfg,
		},
	},
	{
		.gpio = 70,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_1_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_1_cfg,
		},
	},
	{
		.gpio = 71,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_1_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_1_cfg,
		},
	},
	{
		.gpio = 72,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_2_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_2_cfg,
		},
	},
};

//2012-10-30 soodong.kim@lge.com : set GPIO initial value to PULL_UP [START]
//2012-11-22 soodong.kim@lge.com : HW revision check [START]
#if defined (CONFIG_SLIMPORT_ANX7808)
static struct msm_gpiomux_config apq8064_slimport_configs1[] __initdata = {
	{
		.gpio = 43,
		.settings = {
			[GPIOMUX_SUSPENDED] = &slimport_suspend_cfg,
		},
	},
};
static struct msm_gpiomux_config apq8064_slimport_configs2[] __initdata = {
	{
		.gpio = 32,
		.settings = {
			[GPIOMUX_SUSPENDED] = &slimport_suspend_cfg,
		},
	},
};

#endif
//2012-11-22 soodong.kim@lge.com : HW revision check [END]
//2012-10-30 soodong.kim@lge.com : set GPIO initial value to PULL_UP [END]

#if !defined (CONFIG_MACH_LGE)
static struct msm_gpiomux_config apq8064_mhl_configs[] __initdata = {
	{
		.gpio = 30,
		.settings = {
		[GPIOMUX_ACTIVE]    = &hdmi_active_3_cfg,
		[GPIOMUX_SUSPENDED] = &hdmi_suspend_1_cfg,
		},
	},
	{
		.gpio = 35,
		.settings = {
		[GPIOMUX_ACTIVE]    = &hdmi_active_4_cfg,
		[GPIOMUX_SUSPENDED] = &hdmi_suspend_1_cfg,
		},
	},
};
#endif

static struct msm_gpiomux_config apq8064_gsbi_configs[] __initdata = {
	{
		.gpio      = 8,			/* GSBI3 I2C QUP SDA */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi3_suspended_cfg,
			[GPIOMUX_ACTIVE] = &gsbi3_active_cfg,
		},
	},
	{
		.gpio      = 9,			/* GSBI3 I2C QUP SCL */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi3_suspended_cfg,
			[GPIOMUX_ACTIVE] = &gsbi3_active_cfg,
		},
	},
	
	{
		.gpio      = 18,		/* GSBI1 UART TX */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi1_uart_config,
		},
	},
	{
		.gpio      = 19,		/* GSBI1 UART RX */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi1_uart_config,
		},
	},
	
#if defined(CONFIG_MACH_LGE)
	{
		.gpio      = 10,		/* GSBI4 UART TX */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi4_uart,
			[GPIOMUX_ACTIVE] = &gsbi4_uart_active
		},
	},
	{
		.gpio      = 11,		/* GSBI4 UART RX */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi4_uart,
			[GPIOMUX_ACTIVE] = &gsbi4_uart_active
		},
	},
#endif /* CONFIG_MACH_LGE */
	
#if defined(CONFIG_LGE_BROADCAST_TDMB) 
	{
		.gpio	   = 51,		/* GSBI5 QUP DMB SPI_MOSI */
		.settings = {
			[GPIOMUX_ACTIVE] = &gsbi5_spi_config,
			[GPIOMUX_SUSPENDED]	= &gsbi5_spi_suspend_config,
		},
	},
	{
		.gpio	   = 52,		/* GSBI5 QUP DMB SPI_MISO */
		.settings = {
			[GPIOMUX_ACTIVE] = &gsbi5_spi_config,
			[GPIOMUX_SUSPENDED]	= &gsbi5_spi_suspend_config,
		},
	},
	{
		.gpio	   = 53,		/* GSBI5 QUP DMB SPI_CS */
		.settings = {
			[GPIOMUX_ACTIVE] = &gsbi5_spi_config,
			[GPIOMUX_SUSPENDED]	= &gsbi5_spi_suspend_config,
		},
	},
	{
		.gpio	   = 54,		/* GSBI5 QUP SPI_CLK */
		.settings = {
			[GPIOMUX_ACTIVE] = &gsbi5_spi_config,
			[GPIOMUX_SUSPENDED]	= &gsbi5_spi_suspend_config,
		},
	},
	{
		.gpio = 1,						 /* T-DMB_RESET && ONESEG_RESET*/
		.settings = {
			[GPIOMUX_SUSPENDED] = &dmb_ctrl_pin,
		},
	},
	{
		.gpio	   = 77,					 /* DMB_INT */
		.settings = {
			[GPIOMUX_SUSPENDED] = &dmb_int_pin,
		},
	},
	{
		.gpio = 85, 					   /* DMB_EN */
		.settings = {
			[GPIOMUX_SUSPENDED] = &dmb_ctrl_pin,
		},
	},	
#endif /* CONFIG_LGE_BROADCAST */
	
#if !defined(CONFIG_MACH_LGE)
#if defined(CONFIG_KS8851) || defined(CONFIG_KS8851_MODULE)
	{
		.gpio      = 51,		/* GSBI5 QUP SPI_DATA_MOSI */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_config,
		},
	},
	{
		.gpio      = 52,		/* GSBI5 QUP SPI_DATA_MISO */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_config,
		},
	},
	{
		.gpio      = 53,		/* Funny CS0 */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_config,
		},
	},
	{
		.gpio      = 31,		/* GSBI5 QUP SPI_CS2_N */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_cs2_config,
		},
	},
	{
		.gpio      = 54,		/* GSBI5 QUP SPI_CLK */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_config,
		},
	},
#endif
	{
		.gpio      = 30,		/* FP CS */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_cs_config,
		},
	},
	{
		.gpio      = 53,		/* NOR CS */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_cs_config,
		},
	},
	{
		.gpio      = 82,	/* GSBI7 UART2 TX */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi7_func2_cfg,
		},
	},
	{
		.gpio      = 83,	/* GSBI7 UART2 RX */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi7_func1_cfg,
		},
	},
#endif
};

#if !defined(CONFIG_MACH_LGE)
static struct msm_gpiomux_config apq8064_non_mi2s_gsbi_configs[] __initdata = {
	{
		.gpio      = 32,		/* EPM CS */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_epm_spi_cs_config,
		},
	},
};
#endif /* CONFIG_MACH_LGE */

static struct msm_gpiomux_config apq8064_gsbi1_i2c_2ma_configs[] __initdata = {
	{
		.gpio      = 21,		/* GSBI1 QUP I2C_CLK */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config_sus,
			[GPIOMUX_ACTIVE] = &gpio_i2c_2ma_config,
		},
	},
	{
		.gpio      = 20,		/* GSBI1 QUP I2C_DATA */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config_sus,
			[GPIOMUX_ACTIVE] = &gpio_i2c_2ma_config,
		},
	},
};

static struct msm_gpiomux_config apq8064_gsbi1_i2c_8ma_configs[] __initdata = {
        {
                .gpio      = 21,                /* GSBI1 QUP I2C_CLK */
                .settings = {
                        [GPIOMUX_SUSPENDED] = &gpio_i2c_config_sus,
                        [GPIOMUX_ACTIVE] = &gpio_i2c_config,
                },
        },
        {
                .gpio      = 20,                /* GSBI1 QUP I2C_DATA */
                .settings = {
                        [GPIOMUX_SUSPENDED] = &gpio_i2c_config_sus,
                        [GPIOMUX_ACTIVE] = &gpio_i2c_config,
                },
        },
};

static struct msm_gpiomux_config apq8064_slimbus_config[] __initdata = {
	{
		.gpio   = 40,           /* slimbus clk */
		.settings = {
			[GPIOMUX_SUSPENDED] = &slimbus,
		},
	},
	{
		.gpio   = 41,           /* slimbus data */
		.settings = {
			[GPIOMUX_SUSPENDED] = &slimbus,
		},
	},
};

#ifdef CONFIG_LGE_IRRC
static struct msm_gpiomux_config apq8064_irrc_configs[] __initdata = {
       {
              .gpio = 82,
              .settings ={
                     [GPIOMUX_ACTIVE] = &gsbi7_irrc_TXD,
                     [GPIOMUX_SUSPENDED] = &gsbi7_irrc_TXD,
              },
       },
       {
              .gpio = 83,
              .settings = {
                     [GPIOMUX_ACTIVE] = &gsbi7_irrc_RXD,
                     [GPIOMUX_SUSPENDED] = &gsbi7_irrc_RXD,
              },
       },
};
#endif

static struct gpiomux_setting spkr_i2s = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_KEEPER,
};

static struct msm_gpiomux_config mpq8064_spkr_i2s_config[] __initdata = {
	{
		.gpio   = 47,           /* spkr i2c sck */
		.settings = {
			[GPIOMUX_SUSPENDED] = &spkr_i2s,
		},
	},
	{
		.gpio   = 48,           /* spkr_i2s_ws */
		.settings = {
			[GPIOMUX_SUSPENDED] = &spkr_i2s,
		},
	},
	{
		.gpio   = 49,           /* spkr_i2s_dout */
		.settings = {
			[GPIOMUX_SUSPENDED] = &spkr_i2s,
		},
	},
	{
		.gpio   = 50,           /* spkr_i2s_mclk */
		.settings = {
			[GPIOMUX_SUSPENDED] = &spkr_i2s,
		},
	},
};

static struct msm_gpiomux_config apq8064_audio_codec_configs[] __initdata = {
	{
		.gpio = 38,
		.settings = {
			[GPIOMUX_SUSPENDED] = &mbhc_hs_detect,
		},
	},
	{
		.gpio = 39,
		.settings = {
			[GPIOMUX_SUSPENDED] = &cdc_mclk,
		},
	},
};


static struct gpiomux_setting ap2mdm_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting mdm2ap_status_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting mdm2ap_errfatal_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_16MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting mdm2ap_pblrdy = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_16MA,
	.pull = GPIOMUX_PULL_DOWN,
};


static struct gpiomux_setting ap2mdm_soft_reset_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting ap2mdm_wakeup = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_DOWN,
};

// LGE_START // featuring GPIO(MDM2AP_HSIC_READY) configuration for BCM4334
static struct msm_gpiomux_config mdm_configs_bcm[] __initdata = {
	/* AP2MDM_STATUS */
	{
		.gpio = 48,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_cfg,
		}
	},
	/* MDM2AP_STATUS */
	{
		.gpio = 49,
		.settings = {
			[GPIOMUX_ACTIVE] = &mdm2ap_status_cfg,
			[GPIOMUX_SUSPENDED] = &mdm2ap_status_cfg,
		}
	},
	/* MDM2AP_ERRFATAL */
	{
		.gpio = 19,
		.settings = {
			[GPIOMUX_SUSPENDED] = &mdm2ap_errfatal_cfg,
		}
	},
	/* AP2MDM_ERRFATAL */
	{
		.gpio = 18,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_cfg,
		}
	},
	/* AP2MDM_SOFT_RESET, aka AP2MDM_PON_RESET_N */
	{
		.gpio = 27,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_soft_reset_cfg,
		}
	},
	/* AP2MDM_WAKEUP */
	{
		.gpio = 35,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_wakeup,
		}
	},
	/* MDM2AP_PBL_READY*/
	{
		.gpio = 81,
		.settings = {
			[GPIOMUX_SUSPENDED] = &mdm2ap_pblrdy,
		}
	},
};
// LGE_END // featuring GPIO(MDM2AP_HSIC_READY) configuration for BCM4334


static struct msm_gpiomux_config mdm_configs[] __initdata = {
	/* AP2MDM_STATUS */
	{
		.gpio = 48,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_cfg,
		}
	},
	/* MDM2AP_STATUS */
	{
		.gpio = 49,
		.settings = {
			[GPIOMUX_ACTIVE] = &mdm2ap_status_cfg,
			[GPIOMUX_SUSPENDED] = &mdm2ap_status_cfg,
		}
	},
	/* MDM2AP_ERRFATAL */
	{
		.gpio = 19,
		.settings = {
			[GPIOMUX_SUSPENDED] = &mdm2ap_errfatal_cfg,
		}
	},
	/* AP2MDM_ERRFATAL */
	{
		.gpio = 18,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_cfg,
		}
	},
	/* AP2MDM_SOFT_RESET, aka AP2MDM_PON_RESET_N */
	{
		.gpio = 27,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_soft_reset_cfg,
		}
	},
	/* AP2MDM_WAKEUP */
	{
		.gpio = 35,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_wakeup,
		}
	},
	/* MDM2AP_PBL_READY*/
	{
		.gpio = 46,
		.settings = {
			[GPIOMUX_SUSPENDED] = &mdm2ap_pblrdy,
		}
	},
};

static struct msm_gpiomux_config mdm_i2s_configs[] __initdata = {
	/* AP2MDM_STATUS */
	{
		.gpio = 48,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_cfg,
		}
	},
	/* MDM2AP_STATUS */
	{
		.gpio = 49,
		.settings = {
			[GPIOMUX_SUSPENDED] = &mdm2ap_status_cfg,
		}
	},
	/* MDM2AP_ERRFATAL */
	{
		.gpio = 19,
		.settings = {
			[GPIOMUX_SUSPENDED] = &mdm2ap_errfatal_cfg,
		}
	},
	/* AP2MDM_ERRFATAL */
	{
		.gpio = 18,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_cfg,
		}
	},
	/* AP2MDM_SOFT_RESET, aka AP2MDM_PON_RESET_N */
	{
		.gpio = 0,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_soft_reset_cfg,
		}
	},
	/* AP2MDM_WAKEUP */
	{
		.gpio = 44,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_wakeup,
		}
	},
	/* MDM2AP_PBL_READY*/
	{
		.gpio = 81,
		.settings = {
			[GPIOMUX_SUSPENDED] = &mdm2ap_pblrdy,
		}
	},
};

static struct gpiomux_setting i2s_act_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting i2s_act_func_2_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting i2s_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct msm_gpiomux_config mpq8064_mi2s_configs[] __initdata = {
	{
		.gpio	= 27,		/* mi2s ws */
		.settings = {
			[GPIOMUX_ACTIVE]    = &i2s_act_cfg,
			[GPIOMUX_SUSPENDED] = &i2s_sus_cfg,
		},
	},
	{
		.gpio	= 28,		/* mi2s sclk */
		.settings = {
			[GPIOMUX_ACTIVE]    = &i2s_act_cfg,
			[GPIOMUX_SUSPENDED] = &i2s_sus_cfg,
		},
	},
	{
		.gpio	= 29,		/* mi2s dout3 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &i2s_act_cfg,
			[GPIOMUX_SUSPENDED] = &i2s_sus_cfg,
		},
	},
	{
		.gpio	= 30,		/* mi2s dout2 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &i2s_act_cfg,
			[GPIOMUX_SUSPENDED] = &i2s_sus_cfg,
		},
	},

	{
		.gpio	= 31,		/* mi2s dout1 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &i2s_act_cfg,
			[GPIOMUX_SUSPENDED] = &i2s_sus_cfg,
		},
	},
	{
		.gpio	= 32,		/* mi2s dout0 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &i2s_act_cfg,
			[GPIOMUX_SUSPENDED] = &i2s_sus_cfg,
		},
	},

	{
		.gpio	= 33,		/* mi2s mclk */
		.settings = {
			[GPIOMUX_ACTIVE]    = &i2s_act_cfg,
			[GPIOMUX_SUSPENDED] = &i2s_sus_cfg,
		},
	},
};

static struct msm_gpiomux_config apq8064_mi2s_configs[] __initdata = {
	{
		.gpio	= 27,		/* mi2s ws */
		.settings = {
			[GPIOMUX_ACTIVE]    = &i2s_act_cfg,
			[GPIOMUX_SUSPENDED] = &i2s_sus_cfg,
		},
	},
	{
		.gpio	= 28,		/* mi2s sclk */
		.settings = {
			[GPIOMUX_ACTIVE]    = &i2s_act_cfg,
			[GPIOMUX_SUSPENDED] = &i2s_sus_cfg,
		},
	},
	{
		.gpio	= 29,		/* mi2s dout3 - TX*/
		.settings = {
			[GPIOMUX_ACTIVE]    = &i2s_act_cfg,
			[GPIOMUX_SUSPENDED] = &i2s_sus_cfg,
		},
	},
	{
		.gpio	= 32,		/* mi2s dout0 - RX */
		.settings = {
			[GPIOMUX_ACTIVE]    = &i2s_act_cfg,
			[GPIOMUX_SUSPENDED] = &i2s_sus_cfg,
		},
	},

	{
		.gpio	= 33,		/* mi2s mclk */
		.settings = {
			[GPIOMUX_ACTIVE]    = &i2s_act_cfg,
			[GPIOMUX_SUSPENDED] = &i2s_sus_cfg,
		},
	},
};

// [S] LGE_BT: ADD/ilbeom.kim/'12-10-24 - [GK] BRCM Solution bring-up
//BEGIN: 0019632 chanha.park@lge.com 2012-05-31
//ADD: 0019632: [F200][BT] Bluetooth board bring-up
#ifdef CONFIG_LGE_BLUESLEEP
#if 0
static struct msm_gpiomux_config gsbi6_uart_configs[] __initdata = {
	{
		.gpio	   = 14,	/* BT_UART_TXD */
		.settings = {
			[GPIOMUX_ACTIVE]	= &gsbi6,
			[GPIOMUX_SUSPENDED] = &gsbi6_suspend,
		},
	},
	{
		.gpio	   = 15,	/* BT_UART_RXD */
		.settings = {
			[GPIOMUX_ACTIVE]	= &gsbi6,
			[GPIOMUX_SUSPENDED] = &gsbi6_suspend,
		},
	},
	{
		.gpio	   = 16,	/* BT_UART_CTS_N */
		.settings = {
			[GPIOMUX_ACTIVE]	= &gsbi6,
			[GPIOMUX_SUSPENDED] = &gsbi6_suspend,
		},
	},
	{
		.gpio	   = 17,	/* BT_UART_RTS_N */
		.settings = {
			[GPIOMUX_ACTIVE]	= &gsbi6,
			[GPIOMUX_SUSPENDED] = &gsbi6_suspend,
		},
	}
};
#endif

static struct msm_gpiomux_config bt_pcm_configs[] __initdata = {
	{
		.gpio	   = 43,	/* BT_PCM_DOUT */
		.settings = {
			[GPIOMUX_ACTIVE]	= &bt_pcm,
			[GPIOMUX_SUSPENDED] = &bt_pcm_suspend,
		},
	},
	{
		.gpio	   = 44,	/* BT_PCM_DIN */
		.settings = {
			[GPIOMUX_ACTIVE]	= &bt_pcm,
			[GPIOMUX_SUSPENDED] = &bt_pcm_suspend,
		},
	},
	{
		.gpio	   = 45,	/* BT_PCM_SYNC */
		.settings = {
			[GPIOMUX_ACTIVE]	= &bt_pcm,
			[GPIOMUX_SUSPENDED] = &bt_pcm_suspend,
		},
	},
	{
		.gpio	   = 46,	/* BT_PCM_CLK */
		.settings = {
			[GPIOMUX_ACTIVE]	= &bt_pcm,
			[GPIOMUX_SUSPENDED] = &bt_pcm_suspend,
		},
	}
};

//END: 0019632 chanha.park@lge.com 2012-05-31
#endif /* CONFIG_LGE_BLUESLEEP */
// [E] LGE_BT: ADD/ilbeom.kim/'12-10-24 - [GK] BRCM Solution bring-up

static struct msm_gpiomux_config apq8064_mic_i2s_configs[] __initdata = {
	{
		.gpio	= 35,		/* mic i2s sclk */
		.settings = {
			[GPIOMUX_ACTIVE]    = &i2s_act_cfg,
			[GPIOMUX_SUSPENDED] = &i2s_sus_cfg,
		},
	},
	{
		.gpio	= 36,		/* mic i2s ws */
		.settings = {
			[GPIOMUX_ACTIVE]    = &i2s_act_cfg,
			[GPIOMUX_SUSPENDED] = &i2s_sus_cfg,
		},
	},
	{
		.gpio	= 37,		/* mic i2s din0 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &i2s_act_cfg,
			[GPIOMUX_SUSPENDED] = &i2s_sus_cfg,
		},
	},
};


static struct msm_gpiomux_config apq8064_spkr_i2s_configs[] __initdata = {
	{
		.gpio	= 40,		/* spkr i2s sclk */
		.settings = {
			[GPIOMUX_ACTIVE]    = &i2s_act_func_2_cfg,
			[GPIOMUX_SUSPENDED] = &i2s_sus_cfg,
		},
	},
	{
		.gpio	= 41,		/* spkr i2s dout */
		.settings = {
			[GPIOMUX_ACTIVE]    = &i2s_act_func_2_cfg,
			[GPIOMUX_SUSPENDED] = &i2s_sus_cfg,
		},
	},
	{
		.gpio	= 42,		/* spkr i2s ws */
		.settings = {
			[GPIOMUX_ACTIVE]    = &i2s_act_cfg,
			[GPIOMUX_SUSPENDED] = &i2s_sus_cfg,
		},
	},
};


static struct msm_gpiomux_config apq8064_mxt_configs[] __initdata = {
	{	/* TS INTERRUPT */
		.gpio = 6,
		.settings = {
			[GPIOMUX_ACTIVE]    = &mxt_int_act_cfg,
			[GPIOMUX_SUSPENDED] = &mxt_int_sus_cfg,
		},
	},
	{	/* TS RESET */
		.gpio = 33,
		.settings = {
			[GPIOMUX_ACTIVE]    = &mxt_reset_act_cfg,
			[GPIOMUX_SUSPENDED] = &mxt_reset_sus_cfg,
		},
	},
};

#ifndef CONFIG_MMC_MSM_SDC4_SUPPORT
static struct msm_gpiomux_config wcnss_5wire_interface[] = {
	{
		.gpio = 64,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
	{
		.gpio = 65,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
	{
		.gpio = 66,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
	{
		.gpio = 67,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
	{
		.gpio = 68,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
};
#endif

// [S] LGE_BT: ADD/ilbeom.kim/'12-10-24 - [GK] BRCM Solution bring-up
//BEGIN: 0019632 chanha.park@lge.com 2012-05-31
//ADD: 0019632: [F200][BT] Bluetooth board bring-up
#ifdef CONFIG_LGE_BLUESLEEP
static struct msm_gpiomux_config msm8960_bt_host_wakeup_configs[] __initdata = {
	{
		.gpio = 55, // BT_HOST_WAKEUP
		.settings = {
			[GPIOMUX_ACTIVE]    = &bt_host_wakeup_active_cfg,
			[GPIOMUX_SUSPENDED] = &bt_host_wakeup_suspend_cfg,
		},
	},
};

static struct msm_gpiomux_config msm8960_bt_wakeup_configs[] __initdata = {
	{
		.gpio = 57, // BT_WAKEUP
		.settings = {
			[GPIOMUX_ACTIVE]    = &bt_wakeup_active_cfg,
			[GPIOMUX_SUSPENDED] = &bt_wakeup_suspend_cfg,
		},
	},
};
#endif // CONFIG_LGE_BLUESLEEP
//END: 0019632 chanha.park@lge.com 2012-05-31
// [E] LGE_BT: ADD/ilbeom.kim/'12-10-24 - [GK] BRCM Solution bring-up

static struct msm_gpiomux_config mpq8064_gsbi5_i2c_configs[] __initdata = {
	{
		.gpio      = 53,			/* GSBI5 I2C QUP SDA */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi5_suspended_cfg,
			[GPIOMUX_ACTIVE] = &gsbi5_active_cfg,
		},
	},
	{
		.gpio      = 54,			/* GSBI5 I2C QUP SCL */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi5_suspended_cfg,
			[GPIOMUX_ACTIVE] = &gsbi5_active_cfg,
		},
	},
};

static struct gpiomux_setting ir_suspended_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting ir_active_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct msm_gpiomux_config mpq8064_ir_configs[] __initdata = {
	{
		.gpio      = 88,			/* GPIO IR */
		.settings = {
			[GPIOMUX_SUSPENDED] = &ir_suspended_cfg,
			[GPIOMUX_ACTIVE] = &ir_active_cfg,
		},
	},
};

static struct msm_gpiomux_config sx150x_int_configs[] __initdata = {
	{
		.gpio      = 81,
		.settings = {
			[GPIOMUX_SUSPENDED] = &sx150x_suspended_cfg,
			[GPIOMUX_ACTIVE] = &sx150x_active_cfg,
		},
	},
};

#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
static struct gpiomux_setting sdc2_clk_active_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting sdc2_cmd_data_0_3_active_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting sdc2_suspended_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting sdc2_data_1_suspended_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct msm_gpiomux_config apq8064_sdc2_configs[] __initdata = {
	{
		.gpio      = 59,
		.settings = {
			[GPIOMUX_ACTIVE] = &sdc2_clk_active_cfg,
			[GPIOMUX_SUSPENDED] = &sdc2_suspended_cfg,
		},
	},
	{
		.gpio      = 62,
		.settings = {
			[GPIOMUX_ACTIVE] = &sdc2_cmd_data_0_3_active_cfg,
			[GPIOMUX_SUSPENDED] = &sdc2_suspended_cfg,
		},
	},
	{
		.gpio      = 61,
		.settings = {
			[GPIOMUX_ACTIVE] = &sdc2_cmd_data_0_3_active_cfg,
			[GPIOMUX_SUSPENDED] = &sdc2_data_1_suspended_cfg,
		},
	},
	{
		.gpio      = 60,
		.settings = {
			[GPIOMUX_ACTIVE] = &sdc2_cmd_data_0_3_active_cfg,
			[GPIOMUX_SUSPENDED] = &sdc2_suspended_cfg,
		},
	},
	{
		.gpio      = 58,
		.settings = {
			[GPIOMUX_ACTIVE] = &sdc2_cmd_data_0_3_active_cfg,
			[GPIOMUX_SUSPENDED] = &sdc2_suspended_cfg,
		},
	},
};
#else
static struct msm_gpiomux_config apq8064_sdc2_not_configs[] __initdata = {
	{
		.gpio      = 59,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_vcap_config[1],
			[GPIOMUX_SUSPENDED] = &gpio_vcap_config[1],
		},
	},
	{
		.gpio      = 61,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_vcap_config[1],
			[GPIOMUX_SUSPENDED] = &gpio_vcap_config[1],
		},
	},
	{
		.gpio      = 60,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_vcap_config[1],
			[GPIOMUX_SUSPENDED] = &gpio_vcap_config[1],
		},
	},
	{
		.gpio      = 58,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_vcap_config[1],
			[GPIOMUX_SUSPENDED] = &gpio_vcap_config[1],
		},
	},
};

// 20121106 changduk.ryu@lge.com [START] remove GPIO 62 pin configuration NFC VEN used the pin in rev E
static struct msm_gpiomux_config apq8064_sdc2_not_configs_rev_e[] __initdata = {
	{
		.gpio      = 59,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_vcap_config[1],
			[GPIOMUX_SUSPENDED] = &gpio_vcap_config[1],
		},
	},
	{
		.gpio      = 61,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_vcap_config[1],
			[GPIOMUX_SUSPENDED] = &gpio_vcap_config[1],
		},
	},
	{
		.gpio      = 60,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_vcap_config[1],
			[GPIOMUX_SUSPENDED] = &gpio_vcap_config[1],
		},
	},
	{
		.gpio      = 58,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_vcap_config[1],
			[GPIOMUX_SUSPENDED] = &gpio_vcap_config[1],
		},
	},	
};
// 20121106 changduk.ryu@lge.com [END]  remove GPIO 62 pin configuration NFC VEN used the pin in rev E


#endif


#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
static struct gpiomux_setting sdc4_clk_active_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting sdc4_cmd_data_0_3_active_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting sdc4_suspended_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting sdc4_data_1_suspended_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct msm_gpiomux_config apq8064_sdc4_configs[] __initdata = {
	{
		.gpio      = 68,
		.settings = {
			[GPIOMUX_ACTIVE] = &sdc4_clk_active_cfg,
			[GPIOMUX_SUSPENDED] = &sdc4_suspended_cfg,
		},
	},
	{
		.gpio      = 67,
		.settings = {
			[GPIOMUX_ACTIVE] = &sdc4_cmd_data_0_3_active_cfg,
			[GPIOMUX_SUSPENDED] = &sdc4_suspended_cfg,
		},

	},
	{
		.gpio      = 66,
		.settings = {
			[GPIOMUX_ACTIVE] = &sdc4_cmd_data_0_3_active_cfg,
			[GPIOMUX_SUSPENDED] = &sdc4_suspended_cfg,
		},
	},
	{
		.gpio      = 65,
		.settings = {
			[GPIOMUX_ACTIVE] = &sdc4_cmd_data_0_3_active_cfg,
			[GPIOMUX_SUSPENDED] = &sdc4_data_1_suspended_cfg,
		},
	},
	{
		.gpio      = 64,
		.settings = {
			[GPIOMUX_ACTIVE] = &sdc4_cmd_data_0_3_active_cfg,
			[GPIOMUX_SUSPENDED] = &sdc4_suspended_cfg,
		},
	},
	{
		.gpio      = 63,
		.settings = {
			[GPIOMUX_ACTIVE] = &sdc4_cmd_data_0_3_active_cfg,
			[GPIOMUX_SUSPENDED] = &sdc4_suspended_cfg,
		},
	},
};
#endif

static struct gpiomux_setting apq8064_sdc3_card_det_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct msm_gpiomux_config apq8064_sdc3_configs[] __initdata = {
	{
		.gpio      = 26,
		.settings = {
			[GPIOMUX_SUSPENDED] = &apq8064_sdc3_card_det_cfg,
			[GPIOMUX_ACTIVE] = &apq8064_sdc3_card_det_cfg,
		},
	},
};
//20120112 jungyub.jee@lge.com SD detect change
static struct gpiomux_setting apq8064_sdc3_card_det_cfg_chg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct msm_gpiomux_config apq8064_sdc3_configs_chg[] __initdata = {
	{
		.gpio      = 26,
		.settings = {
			[GPIOMUX_SUSPENDED] = &apq8064_sdc3_card_det_cfg_chg,
			[GPIOMUX_ACTIVE] = &apq8064_sdc3_card_det_cfg_chg,
		},
	},
};
//20120112 jungyub.jee@lge.com SD detect change


#ifdef CONFIG_LGE_WIRELESS_CHARGER
static struct gpiomux_setting wc_track_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};
static struct msm_gpiomux_config gpio_wc_track_configs[] = {
	{
		.gpio = 22,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wc_track_cfg,
			[GPIOMUX_SUSPENDED] = &wc_track_cfg,
		},
	},
};
#endif

/* GK Broadcom BCM4334 */
#if defined(CONFIG_LGE_BLUESLEEP)
static struct gpiomux_setting gsbi6_uartdm_active = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gsbi6_uartdm_suspended = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct msm_gpiomux_config mpq8064_uartdm_configs[] __initdata = {
	{ /* UARTDM_TX */
		.gpio      = 14,
		.settings = {
			[GPIOMUX_ACTIVE]    = &gsbi6_uartdm_active,
			[GPIOMUX_SUSPENDED] = &gsbi6_uartdm_suspended,
		},
	},
	{ /* UARTDM_RX */
		.gpio      = 15,
		.settings = {
			[GPIOMUX_ACTIVE]    = &gsbi6_uartdm_active,
			[GPIOMUX_SUSPENDED] = &gsbi6_uartdm_suspended,
		},
	},
	{ /* UARTDM_CTS */
		.gpio      = 16,
		.settings = {
			[GPIOMUX_ACTIVE]    = &gsbi6_uartdm_active,
			[GPIOMUX_SUSPENDED] = &gsbi6_uartdm_suspended,
		},
	},
	{ /* UARTDM_RFR */
		.gpio      = 17,
		.settings = {
			[GPIOMUX_ACTIVE]    = &gsbi6_uartdm_active,
			[GPIOMUX_SUSPENDED] = &gsbi6_uartdm_suspended,
		},
	},
};
#endif

/* ehee.lee@lge.com [START] for NFC */
#if defined(CONFIG_LGE_NFC)
static struct gpiomux_setting nfc_pn544_ven_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_LOW,
};

static struct gpiomux_setting nfc_pn544_irq_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting nfc_pn544_firm_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_LOW,
};
#endif
/* ehee.lee@lge.com [END] for NFC */ 



/* ehee.lee@lge.com [START] for NFC */
/* #ifndef CONFIG_LGE_FELICA_KDDI */
#if !defined(CONFIG_LGE_FELICA_KDDI) && !defined(CONFIG_LGE_FELICA_DCM)
#if defined(CONFIG_LGE_NFC)
static struct msm_gpiomux_config apq8064_nfc_configs[] __initdata = {
	{
		.gpio = 37,
		.settings = {
			[GPIOMUX_SUSPENDED] = &nfc_pn544_firm_cfg,
		},
	},
	{
		.gpio = 29,
		.settings = {
			[GPIOMUX_SUSPENDED] = &nfc_pn544_irq_cfg,
		},
	},
	{
		.gpio = 55,
		.settings = {
			[GPIOMUX_SUSPENDED] = &nfc_pn544_ven_cfg,
		},
	},
};

static struct msm_gpiomux_config apq8064_nfc_configs_rev_e[] __initdata = {
	{
		.gpio = 37,
		.settings = {
			[GPIOMUX_SUSPENDED] = &nfc_pn544_firm_cfg,
		},
	},
	{
		.gpio = 29,
		.settings = {
			[GPIOMUX_SUSPENDED] = &nfc_pn544_irq_cfg,
		},
	},
	{
		.gpio = 62,
		.settings = {
			[GPIOMUX_SUSPENDED] = &nfc_pn544_ven_cfg,
		},
	},
};
#endif
#endif//endif of CONFIG_LGE_FELICA_KDDI, CONFIG_LGE_FELICA_DCM
/* ehee.lee@lge.com [END] for NFC */ 

#ifdef CONFIG_BATTERY_MAX17043
static struct gpiomux_setting fuelgauge_max17048_cfg= {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_IN,
};
static struct msm_gpiomux_config apq8064_fuel_gauge_configs[] __initdata = {
	{
		.gpio = 36,
		.settings = {
			[GPIOMUX_ACTIVE] = &fuelgauge_max17048_cfg,
			[GPIOMUX_SUSPENDED] = &fuelgauge_max17048_cfg,
			},
	},
};
#endif


void __init apq8064_init_gpiomux(void)
{
	int rc;
	int platform_version = socinfo_get_platform_version();
	hw_rev_type lge_bd_rev = HW_REV_EVB1;//20120112 jungyub.jee@lge.com SD detect change

	rc = msm_gpiomux_init(NR_GPIO_IRQS);
	if (rc) {
		pr_err(KERN_ERR "msm_gpiomux_init failed %d\n", rc);
		return;
	}
#ifndef CONFIG_MMC_MSM_SDC4_SUPPORT
	msm_gpiomux_install(wcnss_5wire_interface,
			ARRAY_SIZE(wcnss_5wire_interface));
#endif
	if (machine_is_mpq8064_cdp() || machine_is_mpq8064_hrd() ||
		 machine_is_mpq8064_dtv()) {
		msm_gpiomux_install(mpq8064_gsbi5_i2c_configs,
				ARRAY_SIZE(mpq8064_gsbi5_i2c_configs));
#ifdef CONFIG_MSM_VCAP
		msm_gpiomux_install(vcap_configs,
				ARRAY_SIZE(vcap_configs));
#endif
		msm_gpiomux_install(sx150x_int_configs,
				ARRAY_SIZE(sx150x_int_configs));
	} else {
		#if defined(CONFIG_KS8851) || defined(CONFIG_KS8851_MODULE)
		msm_gpiomux_install(apq8064_ethernet_configs,
				ARRAY_SIZE(apq8064_ethernet_configs));
		#endif

		msm_gpiomux_install(apq8064_gsbi_configs,
				ARRAY_SIZE(apq8064_gsbi_configs));

#if !defined(CONFIG_MACH_LGE)
		if (!(machine_is_apq8064_mtp() &&
		(SOCINFO_VERSION_MINOR(platform_version) == 1)))
			msm_gpiomux_install(apq8064_non_mi2s_gsbi_configs,
				ARRAY_SIZE(apq8064_non_mi2s_gsbi_configs));
#endif
	}
	if (machine_is_apq8064_mtp() &&
	(SOCINFO_VERSION_MINOR(platform_version) == 1)) {
			msm_gpiomux_install(apq8064_mic_i2s_configs,
				ARRAY_SIZE(apq8064_mic_i2s_configs));
			msm_gpiomux_install(apq8064_spkr_i2s_configs,
				ARRAY_SIZE(apq8064_spkr_i2s_configs));
			msm_gpiomux_install(apq8064_mi2s_configs,
				ARRAY_SIZE(apq8064_mi2s_configs));
			msm_gpiomux_install(apq8064_gsbi1_i2c_2ma_configs,
				ARRAY_SIZE(apq8064_gsbi1_i2c_2ma_configs));
	} else {
		msm_gpiomux_install(apq8064_slimbus_config,
				ARRAY_SIZE(apq8064_slimbus_config));
		msm_gpiomux_install(apq8064_gsbi1_i2c_8ma_configs,
				ARRAY_SIZE(apq8064_gsbi1_i2c_8ma_configs));
	}

#ifdef CONFIG_LGE_IRRC
       msm_gpiomux_install(apq8064_irrc_configs,
                     ARRAY_SIZE(apq8064_irrc_configs));
       pr_debug("[IRRC] gpiomux_install");
#endif
	msm_gpiomux_install(apq8064_audio_codec_configs,
			ARRAY_SIZE(apq8064_audio_codec_configs));

	if (machine_is_mpq8064_cdp() || machine_is_mpq8064_hrd() ||
		machine_is_mpq8064_dtv()) {
		msm_gpiomux_install(mpq8064_spkr_i2s_config,
			ARRAY_SIZE(mpq8064_spkr_i2s_config));
	}

	pr_debug("%s(): audio-auxpcm: Include GPIO configs"
		" as audio is not the primary user"
		" for these GPIO Pins\n", __func__);

	if (machine_is_mpq8064_cdp() || machine_is_mpq8064_hrd() ||
		machine_is_mpq8064_dtv())
		msm_gpiomux_install(mpq8064_mi2s_configs,
			ARRAY_SIZE(mpq8064_mi2s_configs));



	if (machine_is_apq8064_mtp()) {
		if (SOCINFO_VERSION_MINOR(platform_version) == 1)
			msm_gpiomux_install(mdm_i2s_configs,
					ARRAY_SIZE(mdm_i2s_configs));
		// LGE_START // featuring GPIO(MDM2AP_HSIC_READY) configuration for BCM4334
		else{
			lge_bd_rev = lge_get_board_revno();
			if (lge_bd_rev == HW_REV_E || lge_bd_rev == HW_REV_C || lge_bd_rev == HW_REV_D)
			msm_gpiomux_install(mdm_configs_bcm,
					ARRAY_SIZE(mdm_configs_bcm));
			else
			msm_gpiomux_install(mdm_configs,
					ARRAY_SIZE(mdm_configs));
			}
		// LGE_END // featuring GPIO(MDM2AP_HSIC_READY) configuration for BCM4334
	}


	if (machine_is_apq8064_mtp()) {
		if (SOCINFO_VERSION_MINOR(platform_version) == 1) {
			msm_gpiomux_install(cyts_gpio_alt_config,
					ARRAY_SIZE(cyts_gpio_alt_config));
		} else {
			msm_gpiomux_install(cyts_gpio_configs,
					ARRAY_SIZE(cyts_gpio_configs));
		}
	}

#ifdef CONFIG_USB_EHCI_MSM_HSIC
	if (machine_is_apq8064_mtp())
		msm_gpiomux_install(apq8064_hsic_configs,
				ARRAY_SIZE(apq8064_hsic_configs));
#endif
/* ehee.lee@lge.com [START] for NFC */
#if defined(CONFIG_LGE_NFC)
	lge_bd_rev = lge_get_board_revno();
	if ((int)lge_bd_rev == HW_REV_E || (int)lge_bd_rev == HW_REV_C|| (int)lge_bd_rev == HW_REV_D)
	{
		msm_gpiomux_install(apq8064_nfc_configs_rev_e,
		ARRAY_SIZE(apq8064_nfc_configs_rev_e));
	}
	else
	{
		msm_gpiomux_install(apq8064_nfc_configs,
		ARRAY_SIZE(apq8064_nfc_configs));
	}
#endif
/* ehee.lee@lge.com [END] for NFC */ 

// [S] LGE_BT: ADD/ilbeom.kim/'12-10-24 - [GK] BRCM Solution bring-up
//BEGIN: 0019632 chanha.park@lge.com 2012-05-31
//ADD: 0019632: [F200][BT] Bluetooth board bring-up
#ifdef CONFIG_LGE_BLUESLEEP
	msm_gpiomux_install(bt_pcm_configs,
			ARRAY_SIZE(bt_pcm_configs));
	msm_gpiomux_install(mpq8064_uartdm_configs,
			ARRAY_SIZE(mpq8064_uartdm_configs));
/* 	msm_gpiomux_install(gsbi6_uart_configs,
		 	ARRAY_SIZE(gsbi6_uart_configs)); */

	msm_gpiomux_install(msm8960_bt_host_wakeup_configs,
			ARRAY_SIZE(msm8960_bt_host_wakeup_configs));

	msm_gpiomux_install(msm8960_bt_wakeup_configs,
			ARRAY_SIZE(msm8960_bt_wakeup_configs));
#endif
//END: 0019632 chanha.park@lge.com 2012-05-31
// [E] LGE_BT: ADD/ilbeom.kim/'12-10-24 - [GK] BRCM Solution bring-up
	if (machine_is_apq8064_cdp() || machine_is_apq8064_liquid())
		msm_gpiomux_install(apq8064_mxt_configs,
			ARRAY_SIZE(apq8064_mxt_configs));

	msm_gpiomux_install(apq8064_hdmi_configs,
			ARRAY_SIZE(apq8064_hdmi_configs));
//2012-10-30 soodong.kim@lge.com : set GPIO initial value to PULL_UP [START]
//2012-11-22 soodong.kim@lge.com : HW revision check [START]
#if defined (CONFIG_SLIMPORT_ANX7808)
    if( ( lge_get_board_revno() == HW_REV_F ||
          lge_get_board_revno() == HW_REV_B ||
          lge_get_board_revno() == HW_REV_A ) ) {
        msm_gpiomux_install(apq8064_slimport_configs1,
			ARRAY_SIZE(apq8064_slimport_configs1));
    } else {
        msm_gpiomux_install(apq8064_slimport_configs2,
			ARRAY_SIZE(apq8064_slimport_configs2));
    }
    pr_err("[Slimport] revision = %d gpiomux install complete!\n", lge_get_board_revno());
#endif
//2012-11-22 soodong.kim@lge.com : HW revision check [END]
//2012-10-30 soodong.kim@lge.com : set GPIO initial value to PULL_UP [END]

#if !defined (CONFIG_MACH_LGE)
	if (apq8064_mhl_display_enabled())
		msm_gpiomux_install(apq8064_mhl_configs,
				ARRAY_SIZE(apq8064_mhl_configs));
#endif

	 if (machine_is_mpq8064_cdp())
		msm_gpiomux_install(mpq8064_ir_configs,
				ARRAY_SIZE(mpq8064_ir_configs));

#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
	msm_gpiomux_install(apq8064_sdc2_configs,
			     ARRAY_SIZE(apq8064_sdc2_configs));
#else

//20121106 changduk.ryu@lge.com [Start] Rev E Setting
	lge_bd_rev = lge_get_board_revno();
	if ((int)lge_bd_rev == HW_REV_E)
		msm_gpiomux_install(apq8064_sdc2_not_configs_rev_e,
				     ARRAY_SIZE(apq8064_sdc2_not_configs_rev_e));
	else
		msm_gpiomux_install(apq8064_sdc2_not_configs,
				     ARRAY_SIZE(apq8064_sdc2_not_configs));
//20121106 changduk.ryu@lge.com [End] Rev E Setting
#endif

#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
	 msm_gpiomux_install(apq8064_sdc4_configs,
			     ARRAY_SIZE(apq8064_sdc4_configs));
#endif
//20120112 jungyub.jee@lge.com SD detect change
	lge_bd_rev = lge_get_board_revno();

	 if (lge_bd_rev == HW_REV_F) 
	 {
		msm_gpiomux_install(apq8064_sdc3_configs,
				ARRAY_SIZE(apq8064_sdc3_configs));
	 }
	 else
	 {
	 	msm_gpiomux_install(apq8064_sdc3_configs_chg,
				ARRAY_SIZE(apq8064_sdc3_configs_chg));
	 }
#ifdef CONFIG_LGE_WIRELESS_CHARGER
	 if (lge_bd_rev != HW_REV_F) 
	 {
	 	if(lge_bd_rev != HW_REV_A && lge_bd_rev != HW_REV_B)
			gpio_wc_track_configs[0].gpio = 84;
		 msm_gpiomux_install(gpio_wc_track_configs,
				 ARRAY_SIZE(gpio_wc_track_configs));
	 }
#endif

#ifdef CONFIG_BATTERY_MAX17043
		msm_gpiomux_install(apq8064_fuel_gauge_configs,
				ARRAY_SIZE(apq8064_fuel_gauge_configs));
#endif

//20120112 jungyub.jee@lge.com SD detect change

}
