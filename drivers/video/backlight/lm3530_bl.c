/* drivers/video/backlight/lm3530_bl.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/backlight.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <mach/board.h>
#include <mach/board_lge.h>
#include <linux/earlysuspend.h>

//LGE_UPDATE_S hojin.ryu@lge.com Exponential BL Setting applied 20120731
#define MAX_LEVEL		0xFF	//255 out of 255(android)
#define MIN_LEVEL 		0x6E	//110 out of 255(android)

#define DEFAULT_LEVEL	0x69	//105 out of 125(cal value)
#define max_brightness_lm3530	0x7B
#define min_brightness_lm3530	0x36

#define DEFAULT_FTM_BRIGHTNESS	0x36 
//LGE_UPDATE_E hojin.ryu@lge.com Exponential BL Setting applied 20120731

#define I2C_BL_NAME "lm3530"

#define BL_ON	1
#define BL_OFF	0

static struct i2c_client *lm3530_i2c_client;

struct backlight_platform_data {
	void (*platform_init)(void);
	int gpio;
	unsigned int mode;
	int max_current;
	int init_on_boot;
	int min_brightness;
	int max_brightness;
};

struct lm3530_device {
	struct i2c_client *client;
	struct backlight_device *bl_dev;
	int gpio;
	int max_current;
	int min_brightness;
	int max_brightness;
	int factory_brightness;
	struct mutex bl_mutex;
};

static const struct i2c_device_id lm3530_bl_id[] = {
	{ I2C_BL_NAME, 0 },
	{ },
};

static int lm3530_write_reg(struct i2c_client *client,
		unsigned char reg, unsigned char val);

static int cur_main_lcd_level;
static int saved_main_lcd_level;
static int backlight_status = BL_ON;

static struct lm3530_device *main_lm3530_dev;
#if 0
#ifdef CONFIG_HAS_EARLYSUSPEND
static struct early_suspend early_suspend;
#endif
#endif
static struct early_suspend * h;

static void lm3530_hw_reset(void)
{
	int gpio = main_lm3530_dev->gpio;
	if (gpio_is_valid(gpio)) {
		gpio_direction_output(gpio, 1);
		gpio_set_value_cansleep(gpio, 1);
		/* gpio is defined in board-lgp_s3-panel.c */
		mdelay(1);
	}
}

static int lm3530_write_reg(struct i2c_client *client,
		unsigned char reg, unsigned char val)
{
    int ret = 0;
	u8 buf[2];
	struct i2c_msg msg = {
		client->addr, 0, 2, buf
	};

	buf[0] = reg;
	buf[1] = val;

	if ((ret = i2c_transfer(client->adapter, &msg, 1)) < 0)
		dev_err(&client->dev, "i2c write error, ret = %d\n", ret);

	return ret;
}

 //EJJ_ORG 0911 Exp. based LUT 0911
/*
//LGE_UPDATE_S hojin.ryu@lge.com Exponential BL mapping table 20120828
static char mapped_value[146] = {
	54, 54, 54, 54, 54, 54, 55, 55, 55, 56, 56, 57, 57, 57, 58, 58, 59, 59,
	60, 61, 61, 62, 63, 63, 64, 65, 65, 66, 67, 68, 68, 69, 70, 70, 71, 72,
	72, 73, 74, 74, 75, 76, 76, 78, 79, 80, 80, 81, 82, 82, 83, 84, 84, 85,
	86, 86, 87, 88, 88, 88, 89, 90, 90, 91, 92, 92, 92, 93, 94, 94, 95, 95,
	96, 96, 97, 97, 97, 98, 98, 98, 99, 99, 99, 100, 100, 101, 102, 102, 103,
	103, 104, 104, 105, 105, 105, 105, 106, 106, 107, 108, 108, 108, 109, 109,
	109, 110, 110, 110, 111, 111, 111, 112, 112, 112, 113, 113, 114, 114, 114,
	115, 115, 115, 116, 116, 116, 117, 117, 118, 118, 118, 119, 119, 119, 119,
	120, 120, 120, 121, 121, 121, 121, 122, 122, 122, 123, 123};
//LGE_UPDATE_S hojin.ryu@lge.com Exponential BL mapping table 20120828
 //EJJ_ORG 0911 
 */
//EJJ_ADD 0921 Linear based Tune #3
	static char mapped_value[146] = {	
		1 	,1 	,1 	,1 	,1 	,1 	,1 	,1 	,1 	,1 	,1 	,1 	,2 	,2 	,2,
		2 	,2 	,2 	,2 	,2 	,2 	,2 	,2 	,3 	,3 	,3 	,3 	,3 	,3 	,3,
		4 	,4 	,4 	,4 	,4 	,4 	,5 	,5 	,5 	,6 	,6 	,6 	,7	,7 	,7,
		8 	,8 	,8 	,9 	,9 	,9 	,10	,10	,11	,11	,11	,12	,12	,13	,13,
		14 	,14 ,15	,15	,16	,16	,16	,17	,17	,18	,18	,19	,19	,19	,20,
		20 	,21 ,22	,22	,23	,24	,24	,25	,26	,26	,27	,28	,29	,30	,30 ,
		31 	,32 ,33	,34	,34	,35	,36	,37	,38	,39	,40	,41	,41	,42	,43,
		44 	,45 ,46	,47	,48	,49	,50	,51	,52	,53	,54	,56	,57	,58	,59,
		60 	,61 ,62	,63	,65	,66	,67	,69	,70	,71	,72	,73	,74	,76	,78,
		79 	,80 ,82	,83	,85	,87	,88	,90	,91	,92	,94 };
//EJJ_ADD 0921 Linear based Tune #3

//LGE_UPDATE_S hojin.ryu@lge.com Exponential BL level applied 20120731
static void lm3530_set_main_current_level(struct i2c_client *client, int level)
{
	struct lm3530_device *dev;
	int cal_value = 0;
	int max_brightness 		= main_lm3530_dev->max_brightness;

	dev = (struct lm3530_device *)i2c_get_clientdata(client);

	dev->bl_dev->props.brightness = level;

	mutex_lock(&main_lm3530_dev->bl_mutex);

	if (level != 0) {
		if (level > 0 && level < MIN_LEVEL)
			cal_value = 0;
		else if (level >= MIN_LEVEL && level <= MAX_LEVEL)
		{
			cal_value = mapped_value[level-MIN_LEVEL];
		}
		else if (level > MAX_LEVEL)
			cal_value = max_brightness;

		cur_main_lcd_level = cal_value;
		lm3530_write_reg(client, 0xA0, cal_value);
		 printk("%s() :level is : %d, cal_value is :* %d\n", __func__, level, cal_value); 
	} else
		lm3530_write_reg(client, 0x10, 0x00);

	msleep(1);

	mutex_unlock(&main_lm3530_dev->bl_mutex);
}
//LGE_UPDATE_S hojin.ryu@lge.com Exponential BL level applied 20120731

void lm3530_backlight_on(int level)
{

	if (backlight_status == BL_OFF) {
		msleep(17);
		printk("%s, ++ lm3530_backlight_on  \n",__func__);
		lm3530_hw_reset();

		lm3530_write_reg(main_lm3530_dev->client, 0xA0, 0x00);
		/* reset 0 brightness */
		lm3530_write_reg(main_lm3530_dev->client, 0x10,
				main_lm3530_dev->max_current);
		//EJJ_ORG lm3530_write_reg(main_lm3530_dev->client, 0x30, 0x00);
		lm3530_write_reg(main_lm3530_dev->client, 0x30, 0x03);	//EJJ_MOD ABS : Insensitive Brightness Transition, 0x03 Ramp rate : rising 32us/step, falling 16.384ms/step , Full scale 508ms
		/* fade in, out */

        backlight_status = BL_ON;
        /* msleep(100); */
	}

	/* printk("%s received (prev backlight_status: %s)\n",
	 * __func__, backlight_status?"ON":"OFF");*/
	lm3530_set_main_current_level(main_lm3530_dev->client, level);

	return;
}

void lm3530_backlight_off(struct early_suspend * h)
{
	int gpio = main_lm3530_dev->gpio;
    printk("%s, backlight_status : %d\n",__func__,backlight_status);
    if (backlight_status == BL_OFF)
		return;
	saved_main_lcd_level = cur_main_lcd_level;
	lm3530_set_main_current_level(main_lm3530_dev->client, 0);
	backlight_status = BL_OFF;

	gpio_tlmm_config(GPIO_CFG(gpio, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
				GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_direction_output(gpio, 0);
	msleep(6);
	return;
}

void lm3530_lcd_backlight_set_level(int level)
{
	if (level > MAX_LEVEL)
		level = MAX_LEVEL;

	if (lm3530_i2c_client != NULL) {
		if (level == 0)
			lm3530_backlight_off(h);
		else
			lm3530_backlight_on(level);

		/*printk("%s() : level is : %d\n", __func__, level);*/
	} else{
		printk(KERN_INFO "%s(): No client\n", __func__);
	}
}
EXPORT_SYMBOL(lm3530_lcd_backlight_set_level);

static int bl_set_intensity(struct backlight_device *bd)
{
	lm3530_lcd_backlight_set_level(bd->props.brightness);
	return 0;
}

static int bl_get_intensity(struct backlight_device *bd)
{
    return cur_main_lcd_level;
}

static ssize_t lcd_backlight_show_level(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int r;

	r = snprintf(buf, PAGE_SIZE, "LCD Backlight Level is : %d\n",
			cur_main_lcd_level);

	return r;
}

static ssize_t lcd_backlight_store_level(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int level;

	if (!count)
		return -EINVAL;

	level = simple_strtoul(buf, NULL, 10);
	lm3530_lcd_backlight_set_level(level);

	return count;
}

static int lm3530_bl_resume(struct i2c_client *client)
{
    lm3530_backlight_on(saved_main_lcd_level);

    return 0;
}

static int lm3530_bl_suspend(struct i2c_client *client, pm_message_t state)
{
    printk(KERN_INFO "%s: new state: %d\n", __func__, state.event);

    lm3530_backlight_off(h);

    return 0;
}

static ssize_t lcd_backlight_show_on_off(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	printk(KERN_INFO "%s received (prev backlight_status: %s)\n", __func__,
			backlight_status ? "ON" : "OFF");
	return 0;
}

static ssize_t lcd_backlight_store_on_off(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int on_off;
	struct i2c_client *client = to_i2c_client(dev);

	if (!count)
		return -EINVAL;

	printk(KERN_INFO "%s received (prev backlight_status: %s)\n", __func__,
			backlight_status ? "ON" : "OFF");

	on_off = simple_strtoul(buf, NULL, 10);

	printk(KERN_ERR "%d", on_off);

	if (on_off == 1)
		lm3530_bl_resume(client);
	else if (on_off == 0)
		lm3530_bl_suspend(client, PMSG_SUSPEND);

	return count;

}
DEVICE_ATTR(lm3530_level, 0644, lcd_backlight_show_level,
		lcd_backlight_store_level);
DEVICE_ATTR(lm3530_backlight_on_off, 0644, lcd_backlight_show_on_off,
		lcd_backlight_store_on_off);

static struct backlight_ops lm3530_bl_ops = {
	.update_status = bl_set_intensity,
	.get_brightness = bl_get_intensity,
};

static int __devinit lm3530_probe(struct i2c_client *i2c_dev,
		const struct i2c_device_id *id)
{
	struct backlight_platform_data *pdata;
	struct lm3530_device *dev;
	struct backlight_device *bl_dev;
	struct backlight_properties props;
	int err;

	printk("%s: i2c probe start\n", __func__);

	pdata = i2c_dev->dev.platform_data;
	lm3530_i2c_client = i2c_dev;

	dev = kzalloc(sizeof(struct lm3530_device), GFP_KERNEL);
	if (dev == NULL) {
		dev_err(&i2c_dev->dev, "fail alloc for lm3530_device\n");
		return 0;
	}

	main_lm3530_dev = dev;

	memset(&props, 0, sizeof(struct backlight_properties));
	props.type = BACKLIGHT_RAW;
	props.max_brightness = max_brightness_lm3530;
	
	bl_dev = backlight_device_register(I2C_BL_NAME, &i2c_dev->dev, NULL,
			&lm3530_bl_ops, &props);
	bl_dev->props.max_brightness = max_brightness_lm3530;
	bl_dev->props.brightness = DEFAULT_LEVEL;
	bl_dev->props.power = FB_BLANK_UNBLANK;

	dev->bl_dev = bl_dev;
	dev->client = i2c_dev;
	dev->gpio = pdata->gpio;
	dev->max_current = pdata->max_current;
	dev->min_brightness = pdata->min_brightness;
	dev->max_brightness = pdata->max_brightness;
	i2c_set_clientdata(i2c_dev, dev);

	dev->factory_brightness = DEFAULT_FTM_BRIGHTNESS; 

	if (dev->gpio && gpio_request(dev->gpio, "lm3530 reset") != 0)
		return -ENODEV;

	mutex_init(&dev->bl_mutex);

	err = device_create_file(&i2c_dev->dev, &dev_attr_lm3530_level);
	err = device_create_file(&i2c_dev->dev,
			&dev_attr_lm3530_backlight_on_off);

#if 0
#ifdef CONFIG_HAS_EARLYSUSPEND
       early_suspend.suspend = lm3530_backlight_off;
       register_early_suspend(&early_suspend);
#endif
#endif

	return 0;
}

static int __devexit lm3530_remove(struct i2c_client *i2c_dev)
{
	struct lm3530_device *dev;
	int gpio = main_lm3530_dev->gpio;

	device_remove_file(&i2c_dev->dev, &dev_attr_lm3530_level);
	device_remove_file(&i2c_dev->dev, &dev_attr_lm3530_backlight_on_off);
	dev = (struct lm3530_device *)i2c_get_clientdata(i2c_dev);
	backlight_device_unregister(dev->bl_dev);
	i2c_set_clientdata(i2c_dev, NULL);

	if (gpio_is_valid(gpio))
		gpio_free(gpio);
	return 0;
}

static struct i2c_driver main_lm3530_driver = {
	.probe = lm3530_probe,
	.remove = lm3530_remove,
	.suspend = NULL,
	.resume = NULL,
	.id_table = lm3530_bl_id,
	.driver = {
		.name = I2C_BL_NAME,
		.owner = THIS_MODULE,
	},
};


static int __init lcd_backlight_init(void)
{
	static int err;

	err = i2c_add_driver(&main_lm3530_driver);

	return err;
}

module_init(lcd_backlight_init);

MODULE_DESCRIPTION("LM3530 Backlight Control");
MODULE_AUTHOR("Jaeseong Gim <jaeseong.gim@lge.com>");
MODULE_LICENSE("GPL");
