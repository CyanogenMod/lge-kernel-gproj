/* Copyright(c) 2012, LGE Inc.
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
#include <mach/board_lge.h>
#include <linux/mfd/pm8xxx/pm8921.h>
#include <linux/regulator/consumer.h>
#include "devices.h"

#include "board-gk.h"
#include "board-8064.h"

#if defined(CONFIG_LGE_NFC)
#include <linux/nfc/pn544_lge.h>

#define I2C_SURF 1
#define I2C_FFA  (1 << 1)
#define I2C_RUMI (1 << 2)
#define I2C_SIM  (1 << 3)
#define I2C_LIQUID (1 << 4)


#define NFC_GPIO_VEN_REV_E  62	//for Broadcom 


struct i2c_registry {
	u8                     machs;
	int                    bus;
	struct i2c_board_info *info;
	int                    len;
};

typedef void (gpio_i2c_init_func_t)(int bus_num);

static struct pn544_i2c_platform_data pn544_i2c_platform_data[] = {
	{
		.ven_gpio = NFC_GPIO_VEN,
		.irq_gpio = NFC_GPIO_IRQ,
		.firm_gpio = NFC_GPIO_FIRM,
	},
};

static struct i2c_board_info msm_i2c_nxp_nfc_info[] = {
	{
		I2C_BOARD_INFO("pn544", NFC_I2C_SLAVE_ADDR),
		.platform_data = &pn544_i2c_platform_data,
		.irq = MSM_GPIO_TO_INT(NFC_GPIO_IRQ),
	}
};

static struct  i2c_registry apq8064_i2c_devices __initdata = {
	I2C_SURF | I2C_FFA | I2C_RUMI | I2C_SIM | I2C_LIQUID,
	APQ_8064_GSBI1_QUP_I2C_BUS_ID,
	msm_i2c_nxp_nfc_info,
	ARRAY_SIZE(msm_i2c_nxp_nfc_info),
};

//rev.E
static struct pn544_i2c_platform_data pn544_i2c_platform_data_rev_e[] = { 
           {
                     .ven_gpio = NFC_GPIO_VEN_REV_E,
                     .irq_gpio = NFC_GPIO_IRQ,
                     .firm_gpio = NFC_GPIO_FIRM,
           },
};

static struct i2c_board_info msm_i2c_nxp_nfc_info_rev_e[] = {
           {
                     I2C_BOARD_INFO("pn544", NFC_I2C_SLAVE_ADDR),
                     .platform_data = &pn544_i2c_platform_data_rev_e,
                     .irq = MSM_GPIO_TO_INT(NFC_GPIO_IRQ),
           }
};

static struct  i2c_registry apq8064_i2c_devices_rev_e __initdata = {
           I2C_SURF | I2C_FFA | I2C_RUMI | I2C_SIM | I2C_LIQUID,
           APQ_8064_GSBI1_QUP_I2C_BUS_ID,
           msm_i2c_nxp_nfc_info_rev_e,
           ARRAY_SIZE(msm_i2c_nxp_nfc_info_rev_e),
};

//rev.e
static void __init lge_add_i2c_nfc_devices(void)
{
	
           hw_rev_type hw_revision = HW_REV_MAX;
           hw_revision = lge_get_board_revno();
           printk("hw_revision : %d--------------------------pn544_nfc\n",(int)hw_revision);
           if((int)hw_revision == HW_REV_E || (int)hw_revision == HW_REV_C || (int)hw_revision == HW_REV_D || (int)hw_revision == HW_REV_1_0)	//HW_REV_E and HW_REV_C and HW_REV_D are for broadcom
           {
                     i2c_register_board_info(apq8064_i2c_devices_rev_e.bus,
                                       apq8064_i2c_devices_rev_e.info,
                                       apq8064_i2c_devices_rev_e.len);
                     printk("here here-upper rev_e--[ven : %d]---------pn544_nfc\n", (int)pn544_i2c_platform_data_rev_e[0].ven_gpio);
           }else{
                     i2c_register_board_info(apq8064_i2c_devices.bus,
                                       apq8064_i2c_devices.info,
                                       apq8064_i2c_devices.len);
                     printk("here here-under--[ven : %d]---------pn544_nfc\n", (int)pn544_i2c_platform_data[0].ven_gpio);
           }
          
}

void __init lge_add_nfc_devices(void)
{
	lge_add_i2c_nfc_devices();
}
#endif
