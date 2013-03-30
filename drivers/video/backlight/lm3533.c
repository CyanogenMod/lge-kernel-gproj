/*
 *  Copyright (C) 2011-2012, LG Eletronics,Inc. All rights reserved.
 *      LM3533 backlight device driver
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
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

#define MAX_BRIGHTNESS_lm3533 			0xFF
#define MAX_BRIGHTNESS_lm3528 			0x7F
#define DEFAULT_BRIGHTNESS 				0x9C
#define MIN_BRIGHTNESS 					0x05
#define I2C_BL_NAME "lm3533"

#define DEFAULT_FTM_BRIGHTNESS			0x45

#define BL_ON                   1
#define BL_OFF                  0

static struct i2c_client *lm3533_i2c_client;

struct backlight_platform_data {
	void (*platform_init)(void);
	int gpio;
	unsigned int mode;
	int max_current;
	int init_on_boot;
	int min_brightness;
	int max_brightness;
	int default_brightness;
	int factory_brightness;
};

struct lm3533_device {
	struct i2c_client *client;
	struct backlight_device *bl_dev;
	int gpio;
	int max_current;
	int min_brightness;
	int max_brightness;
	int default_brightness;
	int factory_brightness;
	struct mutex bl_mutex;
};

static const struct i2c_device_id lm3533_bl_id[] = {
	{ I2C_BL_NAME, 0 },
	{ },
};

static int lm3533_write_reg(struct i2c_client *client,
		unsigned char reg, unsigned char val);

static int cur_main_lcd_level = DEFAULT_BRIGHTNESS;
static int saved_main_lcd_level = DEFAULT_BRIGHTNESS;

static int backlight_status = BL_ON;
static struct lm3533_device *main_lm3533_dev;
#ifdef CONFIG_HAS_EARLYSUSPEND
static struct early_suspend early_suspend;

#if defined(CONFIG_FB_MSM_MIPI_LGIT_CMD_WVGA_INVERSE_PT_PANEL) || \
	defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_WVGA_INVERSE_PT_PANEL)
static int is_early_suspended = false;
static int requested_in_early_suspend_lcd_level= 0;
#endif

#endif /* CONFIG_HAS_EARLYSUSPEND */

#if !defined(CONFIG_FB_MSM_MIPI_LGIT_CMD_WVGA_INVERSE_PT_PANEL) && \
	!defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_WVGA_INVERSE_PT_PANEL)
static struct early_suspend * h;
#endif

static void lm3533_hw_reset(void)
{
	int gpio = main_lm3533_dev->gpio;

	if (gpio_is_valid(gpio)) {
		gpio_direction_output(gpio, 1);
		gpio_set_value_cansleep(gpio, 1);
		mdelay(1);
	} else {
		pr_err("%s: gpio is not valid !!\n", __func__);
	}
}

static int lm3533_write_reg(struct i2c_client *client,
		unsigned char reg, unsigned char val)
{
	int err;
	u8 buf[2];
	struct i2c_msg msg = {
		client->addr, 0, 2, buf
	};

	buf[0] = reg;
	buf[1] = val;

	err = i2c_transfer(client->adapter, &msg, 1);
	if (err < 0)
		dev_err(&client->dev, "i2c write error\n");
	return 0;
}

static int exp_min_value = 150;
static int cal_value;
static char mapped_value[256] = {
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,113,113,113,113,114,114,114,114,114,115,115,115,115,
	116,117,117,118,118,119,120,121,121,122,123,125,126,127,129,130,131,132,
	132,133,134,135,136,137,138,139,140,141,142,142,143,144,146,147,148,150,
	151,152,154,154,155,156,157,158,159,160,161,162,164,165,166,167,168,169,
	170,171,172,173,174,174,175,175,176,177,178,179,180,181,182,183,183,184,
	185,185,186,187,188,189,190,191,191,192,193,194,195,196,197,198,199,200,
	200,201,201,202,203,204,204,205,206,207,207,208,208,209,210,211,211,212,
	212,213,214,214,215,215,216,217,217,218,218,219,219,220,220,221,221,222,
	223,224,224,225,226,226,226,227,227,228,228,229,229,230,230,231,231,231,
	232,232,233,234,234,235,236,237,237,237,238,238,239,239,240,240,240,241,
	241,241,242,242,243,243,244,244,245,246,246,246,247,247,248,248,249,249,
	250,251,251,251,252,252,253,253,253,254,254,255,255
};

static void lm3533_set_main_current_level(struct i2c_client *client, int level)
{
	struct lm3533_device *dev;
	dev = (struct lm3533_device *)i2c_get_clientdata(client);


	if (level == -1)
		level = dev->default_brightness;

	cur_main_lcd_level = level;
	dev->bl_dev->props.brightness = cur_main_lcd_level;

	mutex_lock(&main_lm3533_dev->bl_mutex);
	if (level != 0) {
		cal_value = mapped_value[level];
		lm3533_write_reg(main_lm3533_dev->client, 0x40, cal_value);
	} else {
		lm3533_write_reg(client, 0x27, 0x00);
	}
	mutex_unlock(&main_lm3533_dev->bl_mutex);
}

void lm3533_backlight_on(int level)
{

#if defined(CONFIG_HAS_EARLYSUSPEND) && \
	(defined(CONFIG_FB_MSM_MIPI_LGIT_CMD_WVGA_INVERSE_PT_PANEL) || \
	defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_WVGA_INVERSE_PT_PANEL))

	if (is_early_suspended) {
		requested_in_early_suspend_lcd_level = level;
		return;
	}
#endif /* CONFIG_HAS_EARLYSUSPEND */
	if (backlight_status == BL_OFF) {
	
	printk(" ### %s ### \n",__func__);
		lm3533_hw_reset();
		lm3533_write_reg(main_lm3533_dev->client, 0x10, 0x0);
#if defined(CONFIG_LGE_BACKLIGHT_CABC)
		lm3533_write_reg(main_lm3533_dev->client, 0x14, 0x1); 	/* PWM input is enabled */
#else
		lm3533_write_reg(main_lm3533_dev->client, 0x14, 0x0); 	/* PWM input is disabled */
#endif
		lm3533_write_reg(main_lm3533_dev->client, 0x1A, 0x00);	/* Linear & Control Bank A is configured for register Current control */
		lm3533_write_reg(main_lm3533_dev->client, 0x1F, 0x13); /* Full-Scale Current (20.2mA) */
		lm3533_write_reg(main_lm3533_dev->client, 0x27, 0x1); 	/* Control Bank A is enable */
#if defined(CONFIG_FB_MSM_MIPI_LGIT_CMD_WVGA_INVERSE_PT_PANEL) || \
	defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_WVGA_INVERSE_PT_PANEL)
		lm3533_write_reg(main_lm3533_dev->client, 0x2C, 0xC); /* Active High, OVP(32V), Boost Frequency(500khz) */
		lm3533_write_reg(main_lm3533_dev->client, 0x12, 0x9); /* START UP/SHUTDOWN TRANSITION */
		lm3533_write_reg(main_lm3533_dev->client, 0x13, 0x9); /* RUN-TIME TRANSITION */
#else
		lm3533_write_reg(main_lm3533_dev->client, 0x2C, 0xE); /*Active High, OVP(40V), Boost Frequency(500khz) */
#endif


	}

	lm3533_set_main_current_level(main_lm3533_dev->client, level);
	backlight_status = BL_ON;

	return;
}

#if defined(CONFIG_FB_MSM_MIPI_LGIT_CMD_WVGA_INVERSE_PT_PANEL) || \
	defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_WVGA_INVERSE_PT_PANEL) || \
	!defined(CONFIG_HAS_EARLYSUSPEND)
void lm3533_backlight_off(void)
#else
void lm3533_backlight_off(struct early_suspend * h)
#endif
{
	int gpio = main_lm3533_dev->gpio;

	if (backlight_status == BL_OFF)
		return;
	saved_main_lcd_level = cur_main_lcd_level;
	lm3533_set_main_current_level(main_lm3533_dev->client, 0);
	backlight_status = BL_OFF;

	gpio_direction_output(gpio, 0);
	msleep(6);

	return;
}

void lm3533_lcd_backlight_set_level(int level)
{
	if (level > MAX_BRIGHTNESS_lm3533)
		level = MAX_BRIGHTNESS_lm3533;

	if (lm3533_i2c_client != NULL) {
		if (level == 0) {
#if defined(CONFIG_FB_MSM_MIPI_LGIT_CMD_WVGA_INVERSE_PT_PANEL) || \
	defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_WVGA_INVERSE_PT_PANEL) || \
	!defined(CONFIG_HAS_EARLYSUSPEND)
			lm3533_backlight_off();
#else
			lm3533_backlight_off(h);
#endif
		} else {
			lm3533_backlight_on(level);
		}
	} else {
		pr_err("%s(): No client\n", __func__);
	}
}
EXPORT_SYMBOL(lm3533_lcd_backlight_set_level);

#if defined(CONFIG_HAS_EARLYSUSPEND) && \
	(defined(CONFIG_FB_MSM_MIPI_LGIT_CMD_WVGA_INVERSE_PT_PANEL) || \
	defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_WVGA_INVERSE_PT_PANEL))

void lm3533_early_suspend(struct early_suspend * h)
{
	pr_info("%s[Start] backlight_status: %d\n", __func__,
			backlight_status);
	if (backlight_status == BL_OFF)
		return;

	lm3533_lcd_backlight_set_level(0);
	is_early_suspended = true;
}

void lm3533_late_resume(struct early_suspend * h)
{
	is_early_suspended = false;

	pr_info("%s[Start] backlight_status: %d\n", __func__,
			backlight_status);
	if (backlight_status == BL_ON)
		return;

	lm3533_lcd_backlight_set_level(requested_in_early_suspend_lcd_level);
	return;
}
#endif /* CONFIG_HAS_EARLYSUSPEND */

static int bl_set_intensity(struct backlight_device *bd)
{
	struct i2c_client *client = to_i2c_client(bd->dev.parent);

	lm3533_set_main_current_level(client, bd->props.brightness);
	cur_main_lcd_level = bd->props.brightness;

	return 0;
}

static int bl_get_intensity(struct backlight_device *bd)
{
	unsigned char val = 0;
	val &= 0x1f;
	return (int)val;
}

static ssize_t lcd_backlight_show_level(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int r;
	r = snprintf(buf, PAGE_SIZE, "LCD Backlight Level is "
			": %d\n", cal_value);
	return r;
}

static ssize_t lcd_backlight_store_level(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int level;
	struct i2c_client *client = to_i2c_client(dev);

	if (!count)
		return -EINVAL;

	level = simple_strtoul(buf, NULL, 10);

	if (level > MAX_BRIGHTNESS_lm3533)
		level = MAX_BRIGHTNESS_lm3533;

	lm3533_set_main_current_level(client, level);
	cur_main_lcd_level = level;

	return count;
}

static int lm3533_bl_resume(struct i2c_client *client)
{
#if defined(CONFIG_FB_MSM_MIPI_LGIT_CMD_WVGA_INVERSE_PT_PANEL) || \
    defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_WVGA_INVERSE_PT_PANEL)
	lm3533_lcd_backlight_set_level(saved_main_lcd_level);
#else
	lm3533_backlight_on(saved_main_lcd_level);
#endif
	return 0;
}

static int lm3533_bl_suspend(struct i2c_client *client, pm_message_t state)
{
	printk(KERN_INFO"%s: new state: %d\n", __func__, state.event);

#if defined(CONFIG_FB_MSM_MIPI_LGIT_CMD_WVGA_INVERSE_PT_PANEL) || \
    defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_WVGA_INVERSE_PT_PANEL) || \
	!defined(CONFIG_HAS_EARLYSUSPEND)
	lm3533_lcd_backlight_set_level(saved_main_lcd_level);
#else
	lm3533_backlight_off(h);
#endif
	return 0;
}

static ssize_t lcd_backlight_show_on_off(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int r = 0;
	pr_info("%s received (prev backlight_status: %s)\n", __func__,
			backlight_status ? "ON" : "OFF");
	return r;
}

static ssize_t lcd_backlight_store_on_off(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int on_off;
	struct i2c_client *client = to_i2c_client(dev);

	if (!count)
		return -EINVAL;

	pr_info("%s received (prev backlight_status: %s)\n", __func__,
			backlight_status ? "ON" : "OFF");

	on_off = simple_strtoul(buf, NULL, 10);

	printk(KERN_ERR "%d", on_off);

	if (on_off == 1) {
		lm3533_bl_resume(client);
	} else if (on_off == 0)
		lm3533_bl_suspend(client, PMSG_SUSPEND);

	return count;
}
static ssize_t lcd_backlight_show_exp_min_value(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int r;
	r = snprintf(buf, PAGE_SIZE, "LCD Backlight : %d\n", exp_min_value);
	return r;
}

static ssize_t lcd_backlight_store_exp_min_value(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int value;

	if (!count)
		return -EINVAL;

	value = simple_strtoul(buf, NULL, 10);
	exp_min_value = value;

	return count;
}

DEVICE_ATTR(lm3533_level, 0644, lcd_backlight_show_level,
		lcd_backlight_store_level);
DEVICE_ATTR(lm3533_backlight_on_off, 0644, lcd_backlight_show_on_off,
		lcd_backlight_store_on_off);
DEVICE_ATTR(lm3533_exp_min_value, 0644, lcd_backlight_show_exp_min_value,
		lcd_backlight_store_exp_min_value);

static struct backlight_ops lm3533_bl_ops = {
	.update_status = bl_set_intensity,
	.get_brightness = bl_get_intensity,
};

static int lm3533_probe(struct i2c_client *i2c_dev,
		const struct i2c_device_id *id)
{
	struct backlight_platform_data *pdata;
	struct lm3533_device *dev;
	struct backlight_device *bl_dev;
	struct backlight_properties props;
	int err;

	pr_info("%s: i2c probe start\n", __func__);

	pdata = i2c_dev->dev.platform_data;
	lm3533_i2c_client = i2c_dev;

	dev = kzalloc(sizeof(struct lm3533_device), GFP_KERNEL);
	if (dev == NULL) {
		dev_err(&i2c_dev->dev, "fail alloc for lm3533_device\n");
		return 0;
	}

	pr_info("%s: gpio = %d\n", __func__,pdata->gpio);

	if (pdata->gpio && gpio_request(pdata->gpio, "lm3533 reset") != 0) {
		return -ENODEV;
	}

	main_lm3533_dev = dev;

	memset(&props, 0, sizeof(struct backlight_properties));
	props.type = BACKLIGHT_RAW;

	props.max_brightness = MAX_BRIGHTNESS_lm3533;
	bl_dev = backlight_device_register(I2C_BL_NAME, &i2c_dev->dev,
			NULL, &lm3533_bl_ops, &props);
	bl_dev->props.max_brightness = MAX_BRIGHTNESS_lm3533;
	bl_dev->props.brightness = DEFAULT_BRIGHTNESS;
	bl_dev->props.power = FB_BLANK_UNBLANK;

	dev->bl_dev = bl_dev;
	dev->client = i2c_dev;
	dev->gpio = pdata->gpio;
	dev->max_current = pdata->max_current;
	dev->min_brightness = pdata->min_brightness;
	dev->default_brightness = pdata->default_brightness;
	dev->max_brightness = pdata->max_brightness;
	i2c_set_clientdata(i2c_dev, dev);

	if (pdata->factory_brightness <= 0)
		dev->factory_brightness = DEFAULT_FTM_BRIGHTNESS;
	else
		dev->factory_brightness = pdata->factory_brightness;

	mutex_init(&dev->bl_mutex);

	err = device_create_file(&i2c_dev->dev,
			&dev_attr_lm3533_level);
	err = device_create_file(&i2c_dev->dev,
			&dev_attr_lm3533_backlight_on_off);
	err = device_create_file(&i2c_dev->dev,
			&dev_attr_lm3533_exp_min_value);

#ifdef CONFIG_HAS_EARLYSUSPEND
#if defined(CONFIG_FB_MSM_MIPI_LGIT_CMD_WVGA_INVERSE_PT_PANEL) || \
    defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_WVGA_INVERSE_PT_PANEL)
	early_suspend.suspend = lm3533_early_suspend;
	early_suspend.resume = lm3533_late_resume;
#else
	early_suspend.suspend = lm3533_backlight_off;
#endif
	register_early_suspend(&early_suspend);
#endif /* CONFIG_HAS_EARLYSUSPEND */
	return 0;
}

static int lm3533_remove(struct i2c_client *i2c_dev)
{
	struct lm3533_device *dev;
	int gpio = main_lm3533_dev->gpio;

	device_remove_file(&i2c_dev->dev, &dev_attr_lm3533_level);
	device_remove_file(&i2c_dev->dev, &dev_attr_lm3533_backlight_on_off);
	dev = (struct lm3533_device *)i2c_get_clientdata(i2c_dev);
	backlight_device_unregister(dev->bl_dev);
	i2c_set_clientdata(i2c_dev, NULL);

	if (gpio_is_valid(gpio))
		gpio_free(gpio);
	return 0;
}

static struct i2c_driver main_lm3533_driver = {
	.probe = lm3533_probe,
	.remove = lm3533_remove,
	.suspend = NULL,
	.resume = NULL,
	.id_table = lm3533_bl_id,
	.driver = {
		.name = I2C_BL_NAME,
		.owner = THIS_MODULE,
	},
};


static int __init lcd_backlight_init(void)
{
	static int err;

	err = i2c_add_driver(&main_lm3533_driver);

	return err;
}

module_init(lcd_backlight_init);

MODULE_DESCRIPTION("LM3533 Backlight Control");
MODULE_AUTHOR("Jaeseong Gim <jaeseong.gim@lge.com>");
MODULE_LICENSE("GPL");
