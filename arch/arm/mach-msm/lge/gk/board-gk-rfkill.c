/*
 * Copyright (C) 2009 Google, Inc.
 * Copyright (C) 2009 HTC Corporation.
 * Copyright (C) 2011 Broadcom Corporation.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifdef CONFIG_LGE_BLUESLEEP

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/rfkill.h>
#include <asm/gpio.h>
#include <asm/mach-types.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/machine.h>

#include "board-gk.h"

#define LGPS3_GPIO_BT_RESET_N	PM8921_GPIO_PM_TO_SYS(30)

static struct rfkill *bt_rfk;
static const char bt_name[] = "brcm_Bluetooth_rfkill";

static int bluetooth_set_power(void *data, bool blocked)
{
       printk(KERN_ERR "bluetooth_set_power set blocked=%d",blocked);
	if (!blocked) {
	    gpio_direction_output(LGPS3_GPIO_BT_RESET_N, 0);
		msleep(30);
	    gpio_direction_output(LGPS3_GPIO_BT_RESET_N, 1);
	} else {
	    gpio_direction_output(LGPS3_GPIO_BT_RESET_N, 0);
	}
	return 0;
}

static struct rfkill_ops lgps3_rfkill_ops = {
	.set_block = bluetooth_set_power,
};

static int lgps3_rfkill_probe(struct platform_device *pdev)
{
	int rc = 0;
	bool default_state = true;  /* off */

      printk(KERN_ERR"lgps3_rfkill_probe\n");
	rc = gpio_request(LGPS3_GPIO_BT_RESET_N, "bt_reset");

	if (rc)
	{
	    printk(KERN_ERR "GPIO req error no=%d",rc);
	    gpio_free(LGPS3_GPIO_BT_RESET_N);
	    rc = gpio_request(LGPS3_GPIO_BT_RESET_N, "bt_reset");
	    if(rc)
	    {
	    printk(KERN_ERR "GPIO req error no=%d",rc);
		goto err_gpio_reset;
           }
	}
      gpio_direction_output(LGPS3_GPIO_BT_RESET_N, 0);

	bluetooth_set_power(NULL, default_state);

	bt_rfk = rfkill_alloc(bt_name, &pdev->dev, RFKILL_TYPE_BLUETOOTH,
				&lgps3_rfkill_ops, NULL);
	if (!bt_rfk) {
    	printk(KERN_ERR"rfkill alloc failed.\n");
		rc = -ENOMEM;
		goto err_rfkill_alloc;
	}

	rfkill_set_states(bt_rfk, default_state, false);

	/* userspace cannot take exclusive control */

	rc = rfkill_register(bt_rfk);
	if (rc)
		goto err_rfkill_reg;

	return 0;


err_rfkill_reg:
	rfkill_destroy(bt_rfk);
err_rfkill_alloc:
err_gpio_reset:
	gpio_free(LGPS3_GPIO_BT_RESET_N);
	printk(KERN_ERR"lgps3_rfkill_probe error!\n");
	return rc;
}

static int lgps3_rfkill_remove(struct platform_device *dev)
{
	rfkill_unregister(bt_rfk);
	rfkill_destroy(bt_rfk);
	gpio_free(LGPS3_GPIO_BT_RESET_N);

	return 0;
}
struct lgps3_rfkill_platform_data {
        int gpio_reset;
};

static struct lgps3_rfkill_platform_data lgps3_rfkill_platform;

static struct platform_device lgps3_rfkill_device = {
   .name = "lgps3_rfkill",
   .id   = -1,
   .dev = {
      .platform_data = &lgps3_rfkill_platform,
   },
};

static struct platform_driver lgps3_rfkill_driver = {
	.probe = lgps3_rfkill_probe,
	.remove = lgps3_rfkill_remove,
	.driver = {
		.name = "lgps3_rfkill",
		.owner = THIS_MODULE,
	},
};

static int __init lgps3_rfkill_init(void)
{
    int ret;
    printk(KERN_ERR"lgps3_rfkill_init\n");
	ret = platform_driver_register(&lgps3_rfkill_driver);
	if (ret) {
		printk(KERN_ERR"Fail to register rfkill platform driver\n");
	}
	printk(KERN_ERR"lgps3_rfkill_init done\n");

	return platform_device_register(&lgps3_rfkill_device);;


}

static void __exit lgps3_rfkill_exit(void)
{
    platform_device_unregister(&lgps3_rfkill_device);

	platform_driver_unregister(&lgps3_rfkill_driver);
}

//late_initcall_sync(lgps3_rfkill_init);
device_initcall(lgps3_rfkill_init);
module_exit(lgps3_rfkill_exit);

MODULE_DESCRIPTION("lgps3 rfkill");
MODULE_AUTHOR("Nick Pelly <npelly@google.com>");
MODULE_LICENSE("GPL");

#endif // CONFIG_LGE_BLUESLEEP