/* Copyright (c) 2011-2012, The Linux Foundation. All rights reserved.
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
#include <mach/board_lge.h>
#if defined(CONFIG_MACH_LGE)
#include "board-L05E.h"
#if defined(CONFIG_LGE_BROADCAST_ONESEG)
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

static struct gpiomux_setting oneseg_ctrl_pin = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting oneseg_int_pin = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};
#endif	 /*                             */
#else
#include "board-8064.h"
#endif

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
/* Chip selects for SPI clients */
static struct gpiomux_setting gpio_spi_cs_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_12MA,
	.pull = GPIOMUX_PULL_UP,
};
#ifdef CONFIG_SENSORS_EPM_ADC
/* Chip selects for EPM SPI clients */
static struct gpiomux_setting gpio_epm_spi_cs_config = {
	.func = GPIOMUX_FUNC_6,
	.drv = GPIOMUX_DRV_12MA,
	.pull = GPIOMUX_PULL_UP,
};
#endif
#endif /*                 */

//                                             
//ADD: 0019632: [F200][BT] Bluetooth board bring-up
#ifdef CONFIG_LGE_BLUESLEEP
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
#endif //                     
//                                           

#if defined(CONFIG_LGE_FELICA_NFC) || defined(CONFIG_LGE_NFC_SONY_CXD2235AGG)
static struct gpiomux_setting felica_pon_cfg = {
    .func = GPIOMUX_FUNC_GPIO,
    .drv = GPIOMUX_DRV_2MA,
    .pull = GPIOMUX_PULL_NONE,
    .dir = GPIOMUX_OUT_LOW,
};

static struct gpiomux_setting felica_int_cfg = {
    .func = GPIOMUX_FUNC_GPIO,
    .drv = GPIOMUX_DRV_2MA,
    .pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting felica_rfs_cfg = {
    .func = GPIOMUX_FUNC_GPIO,
    .drv = GPIOMUX_DRV_2MA,
    .pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting felica_lockcont_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
    .pull = GPIOMUX_PULL_NONE,
    .dir = GPIOMUX_OUT_LOW,
};

static struct gpiomux_setting nfc_hsel_cfg = {
    .func = GPIOMUX_FUNC_GPIO,
    .drv = GPIOMUX_DRV_2MA,
    .pull = GPIOMUX_PULL_NONE,
    .dir = GPIOMUX_OUT_LOW,
};
static struct gpiomux_setting snfc_intu_cfg = {
    .func = GPIOMUX_FUNC_GPIO,
    .drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,

};
static struct gpiomux_setting snfc_hvdd_cfg = {
    .func = GPIOMUX_FUNC_GPIO,
    .drv = GPIOMUX_DRV_2MA,
    .pull = GPIOMUX_PULL_NONE,
    .dir = GPIOMUX_OUT_LOW,
};
#endif
//                                             
//ADD: 0019632: [F200][BT] Bluetooth board bring-up
#ifdef CONFIG_LGE_BLUESLEEP
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
#endif //                     
//                                           

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
#if !defined(CONFIG_LGE_NFC_SONY_CXD2235AGG)
	{
		.gpio = 23,
		.settings = {
			[GPIOMUX_SUSPENDED] =	&gpio_vcap_config[2],
			[GPIOMUX_ACTIVE] =		&gpio_vcap_config[2],
		}
	},
#endif
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
#if !defined(CONFIG_LGE_NFC_SONY_CXD2235AGG)
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
#endif
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
#if !defined (CONFIG_MACH_LGE)
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
#endif	
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
#if !defined(CONFIG_LGE_NFC_SONY_CXD2235AGG)
	{
		.gpio = 82,
		.settings = {
			[GPIOMUX_SUSPENDED] =	&gpio_vcap_config[4],
			[GPIOMUX_ACTIVE] =		&gpio_vcap_config[4],
		}
	},
#endif
#if !defined(CONFIG_MACH_LGE)
	{
		.gpio = 83,
		.settings = {
			[GPIOMUX_SUSPENDED] =	&gpio_vcap_config[4],
			[GPIOMUX_ACTIVE] =		&gpio_vcap_config[4],
		}
	},
#endif	
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

#if !defined (CONFIG_MACH_LGE)
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
#endif

//                                                                                   
#if defined(CONFIG_WCD9310_MBHC) && !(defined(CONFIG_SWITCH_FSA8008)||defined(CONFIG_SWITCH_MAX1462X))
static struct gpiomux_setting mbhc_hs_detect = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};
#endif
//                                                                                 

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

#ifdef CONFIG_LGE_IRDA
static struct gpiomux_setting gsbi4_uart = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};
static struct gpiomux_setting gsbi4_uart_active = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};
#endif

#if defined(CONFIG_LGE_FELICA) || defined(CONFIG_LGE_NFC_SONY_CXD2235AGG)
static struct gpiomux_setting gsbi3_felica = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};
static struct gpiomux_setting gsbi3_felica_active = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};
#endif /*                   */

#if !defined(CONFIG_MACH_LGE)
static struct gpiomux_setting ext_regulator_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_LOW,
};
#endif

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
#endif /*              */

#if defined (CONFIG_MACH_LGE)
static struct gpiomux_setting gsbi1_suspended_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_KEEPER,
};

static struct gpiomux_setting gsbi1_active_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gsbi2_suspended_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_KEEPER,
};

static struct gpiomux_setting gsbi2_active_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};
#else //Qualcomm original
static struct gpiomux_setting gsbi1_uart_config = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_16MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting ext_regulator_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_LOW,
};

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
#endif //                            

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

#if defined (CONFIG_I2C_GPIO)
static struct gpiomux_setting gsbi7_suspended_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_KEEPER,
};

static struct gpiomux_setting gsbi7_active_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};
#else
static struct gpiomux_setting gsbi7_scl_suspended_cfg = {
	.func = GPIOMUX_FUNC_3, 
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_KEEPER,
};

static struct gpiomux_setting gsbi7_scl_active_cfg = {
	.func = GPIOMUX_FUNC_3, 
	.drv = GPIOMUX_DRV_8MA, 
	.pull = GPIOMUX_PULL_NONE, 
};

static struct gpiomux_setting gsbi7_sda_suspended_cfg = {
	.func = GPIOMUX_FUNC_2, 
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_KEEPER,
};

static struct gpiomux_setting gsbi7_sda_active_cfg = {
	.func = GPIOMUX_FUNC_2, 
	.drv = GPIOMUX_DRV_8MA, 
	.pull = GPIOMUX_PULL_NONE, 
};
#endif

#if !defined(CONFIG_MACH_APQ8064_L05E)
static struct gpiomux_setting hdmi_suspend_cfg = {
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
#endif

#if !defined(CONFIG_MACH_LGE)
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
#endif

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

#if defined (CONFIG_MACH_LGE)
static struct gpiomux_setting ts_int_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting ts_int_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct msm_gpiomux_config ts_gpio_configs[] __initdata = {
	{	/* TS INTERRUPT */
			.gpio = 72,
			.settings = {
				[GPIOMUX_ACTIVE]	= &ts_int_act_cfg,
				[GPIOMUX_SUSPENDED] = &ts_int_sus_cfg,
			},
	},
};

#ifdef CONFIG_TOUCHSCREEN_S340010_SYNAPTICS_TK
static struct gpiomux_setting tk_active_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting tk_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct msm_gpiomux_config tk_gpio_configs[] __initdata = {
	{	/* TK INTERRUPT */
		.gpio = 83,
		.settings = {
			[GPIOMUX_ACTIVE]    = &tk_active_cfg,
			[GPIOMUX_SUSPENDED] = &tk_suspend_cfg,
		},
	},	
};
#endif //CONFIG_TOUCHSCREEN_S340010_SYNAPTICS_TK
#endif //               

#ifdef CONFIG_USB_EHCI_MSM_HSIC
#if !defined(CONFIG_MACH_LGE)
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
#endif

#if !defined(CONFIG_MACH_LGE)
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
#endif

//                                             
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
#endif //                     
//                                           
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
#if !defined(CONFIG_LGE_NFC_SONY_CXD2235AGG)
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
#endif
#if !defined(CONFIG_MACH_APQ8064_L05E)
static struct msm_gpiomux_config apq8064_hdmi_configs[] __initdata = {
#if !defined(CONFIG_LGE_NFC_SONY_CXD2235AGG)
	{
		.gpio = 69,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_1_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
#endif
	{
		.gpio = 70,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_1_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
	{
		.gpio = 71,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_1_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
	{
		.gpio = 72,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_2_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
};
#endif

static struct msm_gpiomux_config apq8064_gsbi_configs[] __initdata = {
#if defined(CONFIG_MACH_LGE)
	{
		.gpio	   = 20, 		/* GSBI1 I2C QUP SDA */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi1_suspended_cfg,
			[GPIOMUX_ACTIVE] = &gsbi1_active_cfg,
		},
	},
	{
		.gpio	   = 21, 		/* GSBI1 I2C QUP SCL */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi1_suspended_cfg,
			[GPIOMUX_ACTIVE] = &gsbi1_active_cfg,
		},
	},
	{
		.gpio	   = 24, 		/* GSBI2 I2C QUP SDA */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi2_suspended_cfg,
			[GPIOMUX_ACTIVE] = &gsbi2_active_cfg,
		},
	},
	{
		.gpio	   = 25, 		/* GSBI2 I2C QUP SCL */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi2_suspended_cfg,
			[GPIOMUX_ACTIVE] = &gsbi2_active_cfg,
		},
	},
#endif
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
#if defined(CONFIG_MACH_LGE)
//                       
#if defined(CONFIG_LGE_BROADCAST_ONESEG)
	{
		.gpio	   = 51,		/* GSBI5 QUP 1SEG SPI_MOSI */
		.settings = {
			[GPIOMUX_ACTIVE] = &gsbi5_spi_config,
			[GPIOMUX_SUSPENDED]	= &gsbi5_spi_suspend_config,
		},
	},
	{
		.gpio	   = 52,		/* GSBI5 QUP 1SEG SPI_MISO */
		.settings = {
			[GPIOMUX_ACTIVE] = &gsbi5_spi_config,
			[GPIOMUX_SUSPENDED]	= &gsbi5_spi_suspend_config,
		},
	},
	{
		.gpio	   = 53,		/* GSBI5 QUP 1SEG SPI_CS */
		.settings = {
			[GPIOMUX_ACTIVE] = &gsbi5_spi_config,
			[GPIOMUX_SUSPENDED]	= &gsbi5_spi_suspend_config,
		},
	},
	{
		.gpio	   = 54,		/* GSBI5 QUP 1SEG SPI_CLK */
		.settings = {
			[GPIOMUX_ACTIVE] = &gsbi5_spi_config,
			[GPIOMUX_SUSPENDED]	= &gsbi5_spi_suspend_config,
		},
	},
	{
		.gpio = 1,                       /* 1SEG_RESET*/
		.settings = {
			[GPIOMUX_SUSPENDED] = &oneseg_ctrl_pin,
		},
	},
	{
		.gpio	   = 77,                     /* 1SEG_INT */
		.settings = {
			[GPIOMUX_SUSPENDED] = &oneseg_int_pin,
		},
	},
	#if 0 //DNI
	{
		.gpio = 85,                        /* 1SEG_LDO_EN */
		.settings = {
			[GPIOMUX_SUSPENDED] = &oneseg_ctrl_pin,
		},
	},
	#endif
#endif	 /*                             */
#endif /*                 */


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
#endif /*                 */


#if !defined(CONFIG_MACH_LGE)
	{
		.gpio      = 30,		/* FP CS */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_cs_config,
		},
	},
	{
		.gpio      = 32,		/* EPM CS */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_epm_spi_cs_config,
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
	{
		.gpio      = 21,		/* GSBI1 QUP I2C_CLK */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config_sus,
			[GPIOMUX_ACTIVE] = &gpio_i2c_config,
		},
	},
	{
		.gpio      = 20,		/* GSBI1 QUP I2C_DATA */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config_sus,
			[GPIOMUX_ACTIVE] = &gpio_i2c_config,
		},
	},
#endif /*              */

#if defined(CONFIG_LGE_FELICA_NFC)|| defined(CONFIG_LGE_NFC_SONY_CXD2235AGG)
    /* FELICA PON */
	{
		.gpio = 37,
		.settings = {
			[GPIOMUX_SUSPENDED] = &felica_pon_cfg,
		},
	},

    /* FELICA RFS */
	{
		.gpio = 29,
		.settings = {
			[GPIOMUX_SUSPENDED] = &felica_rfs_cfg,
		},
	},

    /* FELICA INT */
	{
		.gpio = 38,
		.settings = {
			[GPIOMUX_SUSPENDED] = &felica_int_cfg,
		},
	},

    /* LOCK CNT */
	{
		.gpio = 62,
		.settings = {
			[GPIOMUX_SUSPENDED] = &felica_lockcont_cfg,
		},
	},

    /* NFC HSEL */
	{
		.gpio = 69,
		.settings = {
			[GPIOMUX_SUSPENDED] = &nfc_hsel_cfg,
		},
	},
    /* SNFC INTU */
	{
		.gpio = 23,
		.settings = {
			[GPIOMUX_SUSPENDED] = &snfc_intu_cfg,
		},
	},
    /* NFC HVDD */
	{
		.gpio = 82,
		.settings = {
			[GPIOMUX_SUSPENDED] = &snfc_hvdd_cfg,
		},
	},
#endif
};

/* L05E HW Rev.C ~ */
#if defined (CONFIG_I2C_GPIO)
static struct msm_gpiomux_config apq8064_gsbi7_configs[] __initdata = {

	{
		.gpio      = 84,			/* GSBI7 I2C QUP SDA for Touch KEY */ 
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi7_suspended_cfg,
			[GPIOMUX_ACTIVE] = &gsbi7_active_cfg,
		},
	},
	{
		.gpio      = 85,			/* GSBI7 I2C QUP SCL for Touch KEY */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi7_suspended_cfg,
			[GPIOMUX_ACTIVE] = &gsbi7_active_cfg,			
		},
	},
};

#else
static struct msm_gpiomux_config apq8064_gsbi7_configs[] __initdata = {

	{
		.gpio      = 84,			/* GSBI7 I2C QUP SDA for Touch KEY */ 
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi7_sda_suspended_cfg,
			[GPIOMUX_ACTIVE] = &gsbi7_sda_active_cfg,			
		},
	},
	{
		.gpio      = 85,			/* GSBI7 I2C QUP SCL for Touch KEY */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi7_scl_suspended_cfg,
			[GPIOMUX_ACTIVE] = &gsbi7_scl_active_cfg,
		},
	},
};
#endif

#if defined(CONFIG_LGE_FELICA) || defined(CONFIG_LGE_NFC_SONY_CXD2235AGG)
static struct msm_gpiomux_config apq8064_felica_uart_gsbi3_configs[] __initdata = {
	{
		.gpio      = 6,		/* GSBI3 UART TX */
		.settings = {
			[GPIOMUX_ACTIVE] = &gsbi3_felica_active,
			[GPIOMUX_SUSPENDED] = &gsbi3_felica,
		},
	},
	{
		.gpio      = 7,		/* GSBI3 UART RX */
		.settings = {
			[GPIOMUX_ACTIVE] = &gsbi3_felica_active,
			[GPIOMUX_SUSPENDED] = &gsbi3_felica,
		},
	},
};
#endif
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

#if !defined(CONFIG_MACH_LGE)
static struct gpiomux_setting spkr_i2c = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_KEEPER,
};

static struct msm_gpiomux_config mpq8064_spkr_i2s_config[] __initdata = {
	{
		.gpio   = 47,           /* spkr i2c sck */
		.settings = {
			[GPIOMUX_SUSPENDED] = &spkr_i2c,
		},
	},
	{
		.gpio   = 48,           /* spkr_i2s_ws */
		.settings = {
			[GPIOMUX_SUSPENDED] = &spkr_i2c,
		},
	},
	{
		.gpio   = 49,           /* spkr_i2s_dout */
		.settings = {
			[GPIOMUX_SUSPENDED] = &spkr_i2c,
		},
	},
	{
		.gpio   = 50,           /* spkr_i2s_mclk */
		.settings = {
			[GPIOMUX_SUSPENDED] = &spkr_i2c,
		},
	},
};
#endif

#ifdef CONFIG_LGE_IRDA
static struct msm_gpiomux_config apq8064_irda_configs[] __initdata = {
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
};
#endif

//                                             
//ADD: 0019632: [F200][BT] Bluetooth board bring-up
#ifdef CONFIG_LGE_BLUESLEEP
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
#endif //                     
//                                           

static struct msm_gpiomux_config apq8064_audio_codec_configs[] __initdata = {
//                                                                                   
#if defined(CONFIG_WCD9310_MBHC) && !(defined(CONFIG_SWITCH_FSA8008)||defined(CONFIG_SWITCH_MAX1462X))
	{
		.gpio = 38,
		.settings = {
			[GPIOMUX_SUSPENDED] = &mbhc_hs_detect,
		},
	},
#endif
//                                                                                 
	{
		.gpio = 39,
		.settings = {
			[GPIOMUX_SUSPENDED] = &cdc_mclk,
		},
	},
};

#if !defined(CONFIG_MACH_LGE)
/* External 3.3 V regulator enable */
static struct msm_gpiomux_config apq8064_ext_regulator_configs[] __initdata = {
	{
		.gpio = APQ8064_EXT_3P3V_REG_EN_GPIO,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ext_regulator_config,
		},
	},
};
#endif

static struct gpiomux_setting ap2mdm_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_4MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting mdm2ap_status_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting mdm2ap_errfatal_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting mdm2ap_pblrdy = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};


static struct gpiomux_setting ap2mdm_soft_reset_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_4MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting ap2mdm_wakeup = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_4MA,
	.pull = GPIOMUX_PULL_DOWN,
};

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
	//                                                  
		.gpio = 81,
	//                                                  
		.settings = {
			[GPIOMUX_SUSPENDED] = &mdm2ap_pblrdy,
		}
	},
};
#if !defined(CONFIG_MACH_LGE)
static struct msm_gpiomux_config amdm_configs[] __initdata = {
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
		.gpio = 31,
		.settings = {
			[GPIOMUX_SUSPENDED] = &mdm2ap_pblrdy,
		}
	},
};

static struct msm_gpiomux_config bmdm_configs[] __initdata = {
	/* AP2MDM_STATUS */
	{
		.gpio = 56,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_cfg,
		}
	},
	/* MDM2AP_STATUS */
	{
		.gpio = 32,
		.settings = {
			[GPIOMUX_ACTIVE] = &mdm2ap_status_cfg,
			[GPIOMUX_SUSPENDED] = &mdm2ap_status_cfg,
		}
	},
	/* MDM2AP_ERRFATAL */
	{
		.gpio = 81,
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
		.gpio = 3,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_soft_reset_cfg,
		}
	},
	/* AP2MDM_WAKEUP */
	{
		.gpio = 29,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_wakeup,
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
#endif

#if !defined(CONFIG_MACH_LGE)
static struct gpiomux_setting mi2s_act_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting mi2s_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct msm_gpiomux_config mpq8064_mi2s_configs[] __initdata = {
	{
		.gpio	= 27,		/* mi2s ws */
		.settings = {
			[GPIOMUX_ACTIVE]    = &mi2s_act_cfg,
			[GPIOMUX_SUSPENDED] = &mi2s_sus_cfg,
		},
	},
	{
		.gpio	= 28,		/* mi2s sclk */
		.settings = {
			[GPIOMUX_ACTIVE]    = &mi2s_act_cfg,
			[GPIOMUX_SUSPENDED] = &mi2s_sus_cfg,
		},
	},
#if !defined(CONFIG_LGE_NFC_SONY_CXD2235AGG)
	{
		.gpio	= 29,		/* mi2s dout3 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &mi2s_act_cfg,
			[GPIOMUX_SUSPENDED] = &mi2s_sus_cfg,
		},
	},
#endif
	{
		.gpio	= 30,		/* mi2s dout2 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &mi2s_act_cfg,
			[GPIOMUX_SUSPENDED] = &mi2s_sus_cfg,
		},
	},

	{
		.gpio	= 31,		/* mi2s dout1 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &mi2s_act_cfg,
			[GPIOMUX_SUSPENDED] = &mi2s_sus_cfg,
		},
	},
	{
		.gpio	= 32,		/* mi2s dout0 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &mi2s_act_cfg,
			[GPIOMUX_SUSPENDED] = &mi2s_sus_cfg,
		},
	},

	{
		.gpio	= 33,		/* mi2s mclk */
		.settings = {
			[GPIOMUX_ACTIVE]    = &mi2s_act_cfg,
			[GPIOMUX_SUSPENDED] = &mi2s_sus_cfg,
		},
	},
};
#endif
static struct msm_gpiomux_config apq8064_mxt_configs[] __initdata = {
#if !defined(CONFIG_LGE_NFC_SONY_CXD2235AGG)
	{	/* TS INTERRUPT */
		.gpio = 6,
		.settings = {
			[GPIOMUX_ACTIVE]    = &mxt_int_act_cfg,
			[GPIOMUX_SUSPENDED] = &mxt_int_sus_cfg,
		},
	},
#endif
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

#if !defined(CONFIG_MACH_LGE)
/* 53, 54 gpios are used for SPI in KOR-TDMB and JR-1SEG G-Project */
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
#endif /*                 */

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
		.gpio      = 57,
		.settings = {
			[GPIOMUX_ACTIVE] = &sdc2_cmd_data_0_3_active_cfg,
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
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting sdc4_suspended_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting sdc4_data_1_suspended_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
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

#if !defined(CONFIG_MACH_LGE)
static struct msm_gpiomux_config sglte2_qsc_configs[] __initdata = {
	/* MDM2AP_STATUS */
	{
		.gpio = 51,
		.settings = {
			[GPIOMUX_SUSPENDED] = &mdm2ap_status_cfg,
		}
	},
	/* MDM2AP_ERRFATAL */
	{
		.gpio = 52,
		.settings = {
			[GPIOMUX_SUSPENDED] = &mdm2ap_errfatal_cfg,
		}
	},
};

static struct gpiomux_setting gsbi4_uartdm_active = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gsbi4_uartdm_suspended = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

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

static struct msm_gpiomux_config apq8064_uartdm_gsbi4_configs[] __initdata = {
	{
		.gpio      = 11,        /* GSBI4 UARTDM RX */
		.settings = {
			[GPIOMUX_ACTIVE] = &gsbi4_uartdm_active,
			[GPIOMUX_SUSPENDED] = &gsbi4_uartdm_suspended,
		},
	},
	{
		.gpio      = 10,       /* GSBI4 UARTDM TX */
		.settings = {
			[GPIOMUX_ACTIVE] = &gsbi4_uartdm_active,
			[GPIOMUX_SUSPENDED] = &gsbi4_uartdm_suspended,
		},
	},
	{
		.gpio      = 13,        /* GSBI4 UARTDM RFR */
		.settings = {
			[GPIOMUX_ACTIVE] = &gsbi4_uartdm_active,
			[GPIOMUX_SUSPENDED] = &gsbi4_uartdm_suspended,
		},
	},
	{
		.gpio      = 12,        /* GSBI4 UARTDM CTS */
		.settings = {
			[GPIOMUX_ACTIVE] = &gsbi4_uartdm_active,
			[GPIOMUX_SUSPENDED] = &gsbi4_uartdm_suspended,
		},
	},
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

#ifdef CONFIG_BATTERY_MAX17048
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
			[GPIOMUX_SUSPENDED] = &fuelgauge_max17048_cfg,
			},
	},
};
#endif

void __init apq8064_init_gpiomux(void)
{
	int rc;
#if !defined(CONFIG_MACH_LGE)
	int platform_version = socinfo_get_platform_version();
#endif

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
#if !defined(CONFIG_MACH_LGE)
		msm_gpiomux_install(mpq8064_gsbi5_i2c_configs,
				ARRAY_SIZE(mpq8064_gsbi5_i2c_configs));
#endif

#ifdef CONFIG_MSM_VCAP
		msm_gpiomux_install(vcap_configs,
				ARRAY_SIZE(vcap_configs));
#endif
		msm_gpiomux_install(sx150x_int_configs,
				ARRAY_SIZE(sx150x_int_configs));
	} else {
#if !defined(CONFIG_MACH_LGE)
		#if defined(CONFIG_KS8851) || defined(CONFIG_KS8851_MODULE)
		msm_gpiomux_install(apq8064_ethernet_configs,
				ARRAY_SIZE(apq8064_ethernet_configs));
		#endif
#endif
		msm_gpiomux_install(apq8064_gsbi_configs,
				ARRAY_SIZE(apq8064_gsbi_configs));
#if defined(CONFIG_MACH_LGE)
		if (lge_get_board_revno() >= HW_REV_C){
			msm_gpiomux_install(apq8064_gsbi7_configs,
							ARRAY_SIZE(apq8064_gsbi7_configs));
		}
#endif
	}
#if defined(CONFIG_LGE_FELICA) || defined(CONFIG_LGE_NFC_SONY_CXD2235AGG)
		msm_gpiomux_install(apq8064_felica_uart_gsbi3_configs,
				ARRAY_SIZE(apq8064_felica_uart_gsbi3_configs));
#endif
	msm_gpiomux_install(apq8064_slimbus_config,
			ARRAY_SIZE(apq8064_slimbus_config));

#ifdef CONFIG_BATTERY_MAX17048
	msm_gpiomux_install(apq8064_fuel_gauge_configs,
		ARRAY_SIZE(apq8064_fuel_gauge_configs));
	pr_debug("[MAX17048] gpiomux_install");
#endif
#ifdef CONFIG_LGE_IRDA
       msm_gpiomux_install(apq8064_irda_configs,
                     ARRAY_SIZE(apq8064_irda_configs));
       pr_debug("[IRDA] gpiomux_install");
#endif

	msm_gpiomux_install(apq8064_audio_codec_configs,
			ARRAY_SIZE(apq8064_audio_codec_configs));

#ifdef CONFIG_MACH_LGE
	msm_gpiomux_install(ts_gpio_configs,
			ARRAY_SIZE(ts_gpio_configs));
#ifdef CONFIG_TOUCHSCREEN_S340010_SYNAPTICS_TK
	msm_gpiomux_install(tk_gpio_configs,
			ARRAY_SIZE(tk_gpio_configs));
#endif	
	msm_gpiomux_install(mdm_configs,
			ARRAY_SIZE(mdm_configs));
#else
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

	msm_gpiomux_install(apq8064_ext_regulator_configs,
			ARRAY_SIZE(apq8064_ext_regulator_configs));

	if (machine_is_apq8064_mtp()) {
		if (socinfo_get_platform_subtype() == PLATFORM_SUBTYPE_DSDA2) {
			msm_gpiomux_install(amdm_configs,
					ARRAY_SIZE(amdm_configs));
			msm_gpiomux_install(bmdm_configs,
				ARRAY_SIZE(bmdm_configs));
		} else if (socinfo_get_platform_subtype() ==
					PLATFORM_SUBTYPE_SGLTE2) {
			msm_gpiomux_install(mdm_configs,
					ARRAY_SIZE(mdm_configs));
			msm_gpiomux_install(sglte2_qsc_configs,
					ARRAY_SIZE(sglte2_qsc_configs));

			/* GSBI4 UART GPIOs for Primary IPC */
			msm_gpiomux_install(apq8064_uartdm_gsbi4_configs,
				ARRAY_SIZE(apq8064_uartdm_gsbi4_configs));
		} else if (SOCINFO_VERSION_MINOR(platform_version) == 1) {
			msm_gpiomux_install(mdm_i2s_configs,
					ARRAY_SIZE(mdm_i2s_configs));
		} else {
			msm_gpiomux_install(mdm_configs,
					ARRAY_SIZE(mdm_configs));
		}
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
#endif

#ifdef CONFIG_USB_EHCI_MSM_HSIC
	if (machine_is_apq8064_mtp())
		msm_gpiomux_install(apq8064_hsic_configs,
				ARRAY_SIZE(apq8064_hsic_configs));
#endif

#if !defined(CONFIG_LGE_FELICA_NFC)
/*                                  */
#if defined(CONFIG_LGE_NFC)
	msm_gpiomux_install(apq8064_nfc_configs,
			ARRAY_SIZE(apq8064_nfc_configs));
#endif
#endif
/*                                */

	if (machine_is_apq8064_cdp() || machine_is_apq8064_liquid())
		msm_gpiomux_install(apq8064_mxt_configs,
			ARRAY_SIZE(apq8064_mxt_configs));

#if !defined(CONFIG_MACH_APQ8064_L05E)
	msm_gpiomux_install(apq8064_hdmi_configs,
			ARRAY_SIZE(apq8064_hdmi_configs));
#endif

	 if (machine_is_mpq8064_cdp())
		msm_gpiomux_install(mpq8064_ir_configs,
				ARRAY_SIZE(mpq8064_ir_configs));

#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
	 msm_gpiomux_install(apq8064_sdc2_configs,
			     ARRAY_SIZE(apq8064_sdc2_configs));
#endif

#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
	 msm_gpiomux_install(apq8064_sdc4_configs,
			     ARRAY_SIZE(apq8064_sdc4_configs));
#endif

	msm_gpiomux_install(apq8064_sdc3_configs,
			ARRAY_SIZE(apq8064_sdc3_configs));
//                                             
//ADD: 0019632: [F200][BT] Bluetooth board bring-up
#ifdef CONFIG_LGE_BLUESLEEP
	msm_gpiomux_install(bt_pcm_configs,
		    ARRAY_SIZE(bt_pcm_configs));
	msm_gpiomux_install(mpq8064_uartdm_configs,
			ARRAY_SIZE(mpq8064_uartdm_configs));
	msm_gpiomux_install(msm8960_bt_host_wakeup_configs,
			ARRAY_SIZE(msm8960_bt_host_wakeup_configs));
	msm_gpiomux_install(msm8960_bt_wakeup_configs,
			ARRAY_SIZE(msm8960_bt_wakeup_configs));
#endif //                     
//                                           
}
