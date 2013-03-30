/* drivers/video/backlight/lm3630_bl.c
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
#include <linux/i2c.h>

#include <mach/board_lge.h>
#include <linux/earlysuspend.h>

#define I2C_BL_NAME                              "lm3630"
#define MAX_BRIGHTNESS_LM3630                    0xFF
#define MIN_BRIGHTNESS_LM3630                    0x0F
#define DEFAULT_BRIGHTNESS                       0xFF
#define DEFAULT_FTM_BRIGHTNESS                   0x0F

#define BL_ON        1
#define BL_OFF       0

static struct i2c_client *lm3630_i2c_client;

static int store_level_used = 0;
static int delay_for_shaking=50;

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

struct lm3630_device {
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

static const struct i2c_device_id lm3630_bl_id[] = {
	{ I2C_BL_NAME, 0 },
	{ },
};
#if defined(CONFIG_LGE_R63311_BACKLIGHT_CABC)
static int lm3630_read_reg(struct i2c_client *client, u8 reg, u8 *buf);
#endif // CABC apply

static int lm3630_write_reg(struct i2c_client *client, unsigned char reg, unsigned char val);

static int cur_main_lcd_level = DEFAULT_BRIGHTNESS;
static int saved_main_lcd_level = DEFAULT_BRIGHTNESS;

static int backlight_status = BL_ON;
static struct lm3630_device *main_lm3630_dev;

#ifdef CONFIG_HAS_EARLYSUSPEND
static struct early_suspend early_suspend;
static int is_early_suspended = false;
#endif /* CONFIG_HAS_EARLYSUSPEND */

static struct early_suspend * h;

#ifdef CONFIG_LGE_WIRELESS_CHARGER
int wireless_backlight_state(void)
{
	return backlight_status;
}
EXPORT_SYMBOL(wireless_backlight_state);
#endif

static void lm3630_hw_reset(void)
{
	int gpio = main_lm3630_dev->gpio;
	/* LGE_CHANGE
	  * Fix GPIO Setting Warning
	  * 2011. 12. 14, kyunghoo.ryu@lge.com
	  */

	if (gpio_is_valid(gpio)) {
		gpio_direction_output(gpio, 1);
		gpio_set_value_cansleep(gpio, 1);
		mdelay(10);
	}
	else
		pr_err("%s: gpio is not valid !!\n", __func__);
}
#if defined(CONFIG_LGE_R63311_BACKLIGHT_CABC)
static int lm3630_read_reg(struct i2c_client *client, u8 reg, u8 *buf)
{
    s32 ret;

    printk("[LCD][DEBUG] reg: %x\n", reg);

    ret = i2c_smbus_read_byte_data(client, reg);

    if(ret < 0)
           printk("[LCD][DEBUG] error\n");

    *buf = ret;

    return 0;
    
}
#endif // CABC apply
static int lm3630_write_reg(struct i2c_client *client, unsigned char reg, unsigned char val)
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
/* LGE_CHANGE
* This is a mapping table from android brightness bar value
* to backlilght driver value.
* 2012-02-28, baryun.hwang@lge.com
*/
#if defined(CONFIG_MACH_APQ8064_GVDCM) || defined(CONFIG_MACH_APQ8064_GVKDDI)
static char mapped_value[256] = {
	  3,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  4,  4,   // 14
	  4,  4,  4,  4,  4,  4,  4,  5,  5,  5,  5,  5,  5,  5,  5,   // 29
	  5,  5,  5,  5,  5,  6,  6,  6,  6,  6,  6,  6,  6,  7,  7,   // 44
	  7,  8,  8,  8,  9,  9,  9,  9,  9, 10, 10, 10, 11, 11, 11,   // 59
	 12, 12, 12, 12, 13, 13, 13, 13, 14, 14, 15, 15, 15, 16, 16,   // 74
	 17, 17, 17, 18, 18, 18, 19, 19, 20, 21, 22, 22, 23, 24, 24,   // 89
	 25, 25, 26, 26, 27, 28, 28, 29, 29, 30, 30, 31, 31, 32, 32,   // 104
	 33, 34, 35, 35, 36, 36, 37, 38, 39, 39, 40, 41, 41, 42, 43,   // 119
	 44, 45, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,   // 134
	 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72,   // 149
	 73, 74, 75, 76, 76, 77, 78, 80, 81, 82, 83, 85, 86, 87, 88,   // 164
	 89, 90, 91, 93, 95, 96, 97, 99,100,102,103,104,106,107,108,   // 199
	109,110,112,114,115,117,119,121,123,125,127,128,129,130,132,   // 204
	133,135,136,138,139,140,142,144,146,148,150,151,153,154,156,   // 219
	157,158,159,161,163,164,165,167,168,170,173,175,177,180,184,   // 224
	186,188,191,194,197,199,201,203,205,207,209,211,213,215,217,   // 239
	219,221,223,225,227,228,230,232,235,238,240,243,246,249,252,   // 244
	255
};
#elif defined(CONFIG_MACH_APQ8064_GKU) || defined(CONFIG_MACH_APQ8064_GKKT) \
       || defined(CONFIG_MACH_APQ8064_GKSK) || defined(CONFIG_MACH_APQ8064_GKATT)
static char mapped_value[256] = {
	  3,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  4,  4,   // 14
	  4,  4,  4,  4,  4,  4,  4,  5,  5,  5,  5,  5,  5,  5,  5,   // 29
	  5,  5,  5,  5,  5,  6,  6,  6,  6,  6,  6,  6,  6,  7,  7,   // 44
	  7,  8,  8,  8,  9,  9,  9,  9,  9, 10, 10, 10, 11, 11, 11,   // 59
	 12, 12, 12, 12, 13, 13, 13, 13, 14, 14, 15, 15, 15, 16, 16,   // 74
	 17, 17, 17, 18, 18, 18, 19, 19, 20, 21, 22, 22, 23, 24, 24,   // 89
	 25, 25, 26, 26, 27, 28, 28, 29, 29, 30, 30, 31, 31, 32, 32,   // 104
	 33, 34, 35, 35, 36, 36, 37, 38, 39, 39, 40, 41, 41, 42, 43,   // 119
	 44, 45, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,   // 134
	 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72,   // 149
	 73, 74, 75, 76, 76, 77, 78, 80, 81, 82, 83, 85, 86, 87, 88,   // 164
	 89, 90, 91, 93, 95, 96, 97, 99,100,102,103,104,106,107,108,   // 199
	109,110,112,114,115,117,119,121,123,125,127,128,129,130,132,   // 204
	133,135,136,138,139,140,142,144,146,148,150,151,153,154,156,   // 219
	157,158,159,161,163,164,165,167,168,170,173,175,177,180,184,   // 224
	186,188,191,194,197,199,201,203,205,207,209,211,213,215,217,   // 239
	219,221,223,225,227,228,230,232,235,238,240,243,246,249,252,   // 244
	255
};
#endif

static void lm3630_set_main_current_level(struct i2c_client *client, int level)
{
	struct lm3630_device *dev;
	dev = (struct lm3630_device *)i2c_get_clientdata(client);

	if (level == -1)
		level = dev->default_brightness;

	cur_main_lcd_level = level;
	dev->bl_dev->props.brightness = cur_main_lcd_level;

       store_level_used = 0;

	mutex_lock(&main_lm3630_dev->bl_mutex);
	if (level != 0) {
		cal_value = mapped_value[level];
		lm3630_write_reg(client, 0x03, cal_value);
	} else {
		lm3630_write_reg(client, 0x00, 0x00);
	}
	mutex_unlock(&main_lm3630_dev->bl_mutex);

       printk(KERN_INFO "[LCD][DEBUG] %s : backlight level=%d, cal_value=%d\n", __func__, level, cal_value);
}

static void lm3630_set_main_current_level_no_mapping(struct i2c_client *client, int level)
{
	struct lm3630_device *dev;
	dev = (struct lm3630_device *)i2c_get_clientdata(client);

	if (level > 255)
		level = 255; 
       else if (level < 0)
              level = 0;

	cur_main_lcd_level = level;
	dev->bl_dev->props.brightness = cur_main_lcd_level;

       store_level_used = 1;

	mutex_lock(&main_lm3630_dev->bl_mutex);
	if (level != 0) {
		lm3630_write_reg(client, 0x03, level);
	} else {
		lm3630_write_reg(client, 0x00, 0x00);
	}
	mutex_unlock(&main_lm3630_dev->bl_mutex);
}

void lm3630_backlight_on(int level)
{

#ifdef CONFIG_HAS_EARLYSUSPEND
	if (is_early_suspended)
              return;
#endif /* CONFIG_HAS_EARLYSUSPEND */
	if (backlight_status == BL_OFF) {
//              mdelay(delay_for_shaking);

		lm3630_hw_reset();

#if defined(CONFIG_MACH_APQ8064_GVDCM) || defined(CONFIG_MACH_APQ8064_GVKDDI)
		lm3630_write_reg(main_lm3630_dev->client, 0x02, 0x30);	/*  OVP(24V),OCP(1.0A) , Boost Frequency(500khz) */
#if defined(CONFIG_LGE_R63311_BACKLIGHT_CABC)
	    lm3630_write_reg(main_lm3630_dev->client, 0x01, 0x09);	/* eble Feedback , disable  PWM for BANK A,B */
#else
		lm3630_write_reg(main_lm3630_dev->client, 0x01, 0x08);	/* eble Feedback , disable  PWM for BANK A,B */
#endif // CABC apply
//		lm3630_write_reg(main_lm3630_dev->client, 0x03, 0xFF);	/* Brightness Code Setting Max on Bank A */
		lm3630_write_reg(main_lm3630_dev->client, 0x05, 0x14);	/* Full-Scale Current (20mA) of BANK A for GVDCM*/
		lm3630_write_reg(main_lm3630_dev->client, 0x00, 0x15);	/* Enable LED A to Exponential, LED2 is connected to BANK_A */
#elif defined(CONFIG_MACH_APQ8064_GKU) || defined(CONFIG_MACH_APQ8064_GKKT) \
       || defined(CONFIG_MACH_APQ8064_GKSK) || defined(CONFIG_MACH_APQ8064_GKATT)
		lm3630_write_reg(main_lm3630_dev->client, 0x02, 0x30);	/*  OVP(24V),OCP(1.0A) , Boost Frequency(500khz) */
#if defined(CONFIG_LGE_R63311_BACKLIGHT_CABC)
	    lm3630_write_reg(main_lm3630_dev->client, 0x01, 0x09);	/* eble Feedback , disable  PWM for BANK A,B */
#else
		lm3630_write_reg(main_lm3630_dev->client, 0x01, 0x08);	/* eble Feedback , disable  PWM for BANK A,B */
#endif // CABC apply
//		lm3630_write_reg(main_lm3630_dev->client, 0x03, 0xFF);	/* Brightness Code Setting Max on Bank A */
		lm3630_write_reg(main_lm3630_dev->client, 0x05, 0x15);  /* Full-Scale Current (20.75mA) of BANK A */
		lm3630_write_reg(main_lm3630_dev->client, 0x00, 0x15);	/* Enable LED A to Exponential, LED2 is connected to BANK_A */
#endif
	}
       mdelay(1);   /* udelay(100); */

	lm3630_set_main_current_level(main_lm3630_dev->client, level);
	backlight_status = BL_ON;

	return;
}

#if !defined(CONFIG_HAS_EARLYSUSPEND)
void lm3630_backlight_off(void)
#else
void lm3630_backlight_off(struct early_suspend * h)
#endif /* CONFIG_HAS_EARLYSUSPEND */
{
	int gpio = main_lm3630_dev->gpio;

	if (backlight_status == BL_OFF)
		return;

	saved_main_lcd_level = cur_main_lcd_level;
	lm3630_set_main_current_level(main_lm3630_dev->client, 0);
	backlight_status = BL_OFF;

	gpio_direction_output(gpio, 0);
	msleep(6);

	return;
}

void lm3630_lcd_backlight_set_level(int level)
{
	if (level > MAX_BRIGHTNESS_LM3630)
		level = MAX_BRIGHTNESS_LM3630;

//	printk("### %s level = (%d) \n ",__func__,level);
	if (lm3630_i2c_client != NULL) {
		if (level == 0) {
#if !defined(CONFIG_HAS_EARLYSUSPEND)
			lm3630_backlight_off();
#else
			lm3630_backlight_off(h);
#endif /* CONFIG_HAS_EARLYSUSPEND */
		} else {
			lm3630_backlight_on(level);
		}
	} else {
		pr_err("%s(): No client\n", __func__);
	}
}
EXPORT_SYMBOL(lm3630_lcd_backlight_set_level);   

#ifdef CONFIG_HAS_EARLYSUSPEND
void lm3630_early_suspend(struct early_suspend * h)
{
	pr_info("%s[Start] backlight_status: %d\n", __func__, backlight_status);
	if (backlight_status == BL_OFF)
		return;

	lm3630_lcd_backlight_set_level(0);
	is_early_suspended = true;
       return;
}

void lm3630_late_resume(struct early_suspend * h)
{
	pr_info("%s[Start] backlight_status: %d\n", __func__, backlight_status);
	if (backlight_status == BL_ON)
		return;

       lm3630_lcd_backlight_set_level(cur_main_lcd_level);
	is_early_suspended = false;
	return;
}
#endif /* CONFIG_HAS_EARLYSUSPEND */

static int bl_set_intensity(struct backlight_device *bd)
{
	struct i2c_client *client = to_i2c_client(bd->dev.parent);

	lm3630_set_main_current_level(client, bd->props.brightness);
	cur_main_lcd_level = bd->props.brightness;

	return 0;
}

static int bl_get_intensity(struct backlight_device *bd)
{
    unsigned char val = 0;
	 val &= 0x1f;

    return (int)val;
}

static ssize_t lcd_backlight_show_level(struct device *dev, struct device_attribute *attr, char *buf)
{
	int r = 0;

       if(store_level_used == 0)
              r = snprintf(buf, PAGE_SIZE, "LCD Backlight Level is : %d\n", cal_value);
       else if(store_level_used == 1)
              r = snprintf(buf, PAGE_SIZE, "LCD Backlight Level is : %d\n", cur_main_lcd_level);

	return r;
}

static ssize_t lcd_backlight_store_level(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int level;
	struct i2c_client *client = to_i2c_client(dev);

	if (!count)
		return -EINVAL;

	level = simple_strtoul(buf, NULL, 10);

       lm3630_set_main_current_level_no_mapping(client, level);
       printk(KERN_INFO "[LCD][DEBUG] write %d direct to backlight register\n", level);

	return count;
}

static int lm3630_bl_resume(struct i2c_client *client)
{
	lm3630_lcd_backlight_set_level(saved_main_lcd_level);
       return 0;
}

static int lm3630_bl_suspend(struct i2c_client *client, pm_message_t state)
{
       printk(KERN_INFO "[LCD][DEBUG] %s: new state: %d\n", __func__, state.event);

#if !defined(CONFIG_HAS_EARLYSUSPEND)
       lm3630_lcd_backlight_set_level(saved_main_lcd_level);
#else
	lm3630_backlight_off(h);
#endif /* CONFIG_HAS_EARLYSUSPEND */
    return 0;
}

static ssize_t lcd_backlight_show_on_off(struct device *dev, struct device_attribute *attr, char *buf)
{
	int r = 0;

	pr_info("%s received (prev backlight_status: %s)\n", __func__, backlight_status ? "ON" : "OFF");

	return r;
}

static ssize_t lcd_backlight_store_on_off(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int on_off;
	struct i2c_client *client = to_i2c_client(dev);

	if (!count)
		return -EINVAL;

	pr_info("%s received (prev backlight_status: %s)\n", __func__, backlight_status ? "ON" : "OFF");

	on_off = simple_strtoul(buf, NULL, 10);

	printk(KERN_ERR "[LCD][DEBUG] %d", on_off);

	if (on_off == 1) {
		lm3630_bl_resume(client);
	} else if (on_off == 0)
	    lm3630_bl_suspend(client, PMSG_SUSPEND);

	return count;

}
static ssize_t lcd_backlight_show_exp_min_value(struct device *dev, struct device_attribute *attr, char *buf)
{
	int r;

	r = snprintf(buf, PAGE_SIZE, "LCD Backlight  : %d\n", exp_min_value);

	return r;
}

static ssize_t lcd_backlight_store_exp_min_value(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int value;

	if (!count)
		return -EINVAL;

	value = simple_strtoul(buf, NULL, 10);
	exp_min_value = value;

	return count;
}

static ssize_t lcd_backlight_show_shaking_delay(struct device *dev, struct device_attribute *attr, char *buf)
{
	int r = 0;

	printk("%s received (shaking delay : %d)\n", __func__, delay_for_shaking);

	return r;
}

static ssize_t lcd_backlight_store_shaking_delay(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int delay;

	if (!count)
		return -EINVAL;

	delay = simple_strtoul(buf, NULL, 10);
	printk("%s received (you input : %d for shaking delay)\n", __func__, delay); 
       delay_for_shaking = delay;

	return count;

}
#if defined(CONFIG_LGE_R63311_BACKLIGHT_CABC)
static ssize_t lcd_backlight_show_pwm(struct device *dev, struct device_attribute *attr, char *buf)
{
    int r;
    u8 level,pwm_low,pwm_high,config;

    mutex_lock(&main_lm3630_dev->bl_mutex);
	lm3630_read_reg(main_lm3630_dev->client, 0x01, &config);
	mdelay(3);
    lm3630_read_reg(main_lm3630_dev->client, 0x03, &level);
    mdelay(3);
    lm3630_read_reg(main_lm3630_dev->client, 0x12, &pwm_low);
    mdelay(3);
    lm3630_read_reg(main_lm3630_dev->client, 0x13, &pwm_high);
    mdelay(3);
    mutex_unlock(&main_lm3630_dev->bl_mutex);

   	r = snprintf(buf, PAGE_SIZE, "Show PWM level: %d pwm_low: %d pwm_high: %d config: %d\n", level, pwm_low, pwm_high, config);

    return r;
}
static ssize_t lcd_backlight_store_pwm(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}
#endif // CABC apply

DEVICE_ATTR(lm3630_level, 0644, lcd_backlight_show_level, lcd_backlight_store_level);
DEVICE_ATTR(lm3630_backlight_on_off, 0644, lcd_backlight_show_on_off, lcd_backlight_store_on_off);
DEVICE_ATTR(lm3630_exp_min_value, 0644, lcd_backlight_show_exp_min_value, lcd_backlight_store_exp_min_value);
DEVICE_ATTR(lm3630_shaking_delay, 0644, lcd_backlight_show_shaking_delay, lcd_backlight_store_shaking_delay);
#if defined(CONFIG_LGE_R63311_BACKLIGHT_CABC)
DEVICE_ATTR(lm3630_pwm, 0644, lcd_backlight_show_pwm, lcd_backlight_store_pwm);
#endif // CABC apply

static struct backlight_ops lm3630_bl_ops = {
	.update_status = bl_set_intensity,
	.get_brightness = bl_get_intensity,
};

static int lm3630_probe(struct i2c_client *i2c_dev, const struct i2c_device_id *id)
{
	struct backlight_platform_data *pdata;
	struct lm3630_device *dev;
	struct backlight_device *bl_dev;
	struct backlight_properties props;
	int err;

	printk(KERN_INFO "[LCD][DEBUG] %s: i2c probe start\n", __func__);

	pdata = i2c_dev->dev.platform_data;
	lm3630_i2c_client = i2c_dev;

	dev = kzalloc(sizeof(struct lm3630_device), GFP_KERNEL);
	if (dev == NULL) {
		dev_err(&i2c_dev->dev, "fail alloc for lm3630_device\n");
		return 0;
	}

	printk(KERN_INFO "[LCD][DEBUG] %s: gpio = %d\n", __func__,pdata->gpio);

	if (pdata->gpio && gpio_request(pdata->gpio, "lm3630 reset") != 0) {
		return -ENODEV;
	}

	main_lm3630_dev = dev;

	memset(&props, 0, sizeof(struct backlight_properties));
	props.type = BACKLIGHT_RAW;

	props.max_brightness = MAX_BRIGHTNESS_LM3630;
	bl_dev = backlight_device_register(I2C_BL_NAME, &i2c_dev->dev, NULL, &lm3630_bl_ops, &props);
	bl_dev->props.max_brightness = MAX_BRIGHTNESS_LM3630;
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

	if(pdata->factory_brightness <= 0)
		dev->factory_brightness = DEFAULT_FTM_BRIGHTNESS;
	else
		dev->factory_brightness = pdata->factory_brightness;

	mutex_init(&dev->bl_mutex);

	err = device_create_file(&i2c_dev->dev, &dev_attr_lm3630_level);
	err = device_create_file(&i2c_dev->dev, &dev_attr_lm3630_backlight_on_off);
	err = device_create_file(&i2c_dev->dev, &dev_attr_lm3630_exp_min_value);
	err = device_create_file(&i2c_dev->dev, &dev_attr_lm3630_shaking_delay);
#if defined(CONFIG_LGE_R63311_BACKLIGHT_CABC)
    err = device_create_file(&i2c_dev->dev, &dev_attr_lm3630_pwm);
#endif // CABC apply

#ifdef CONFIG_HAS_EARLYSUSPEND
	early_suspend.suspend = lm3630_early_suspend;
	early_suspend.resume = lm3630_late_resume;
	register_early_suspend(&early_suspend);
#endif /* CONFIG_HAS_EARLYSUSPEND */
//       lm3630_backlight_on(250);
	return 0;
}

static int lm3630_remove(struct i2c_client *i2c_dev)
{
	struct lm3630_device *dev;
	int gpio = main_lm3630_dev->gpio;

	device_remove_file(&i2c_dev->dev, &dev_attr_lm3630_level);
	device_remove_file(&i2c_dev->dev, &dev_attr_lm3630_backlight_on_off);
	dev = (struct lm3630_device *)i2c_get_clientdata(i2c_dev);
	backlight_device_unregister(dev->bl_dev);
	i2c_set_clientdata(i2c_dev, NULL);

	if (gpio_is_valid(gpio))
		gpio_free(gpio);

	return 0;
}

static struct i2c_driver main_lm3630_driver = {
	.probe = lm3630_probe,
	.remove = lm3630_remove,
	.suspend = NULL,
	.resume = NULL,
	.id_table = lm3630_bl_id,
	.driver = {
		.name = I2C_BL_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init lcd_backlight_init(void)
{
	static int err;

	err = i2c_add_driver(&main_lm3630_driver);

	return err;
}

module_init(lcd_backlight_init);

MODULE_DESCRIPTION("LM3630 Backlight Control");
MODULE_AUTHOR("daewoo kwak");
MODULE_LICENSE("GPL");
