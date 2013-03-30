/*
 *  lge/com_device/input/max1462x.c
 *
 *  LGE 3.5 PI Headset detection driver using max1462x.
 *
 * Copyright (C) 2008 Google, Inc.
 * Author: Mike Lockwood <lockwood@android.com>
 *
 * Copyright (C) 2009 ~ 2010 LGE, Inc.
 * Author: Lee SungYoung <lsy@lge.com>
 *
 * Copyright (C) 2010 LGE, Inc.
 * Author: Kim Eun Hye <ehgrace.kim@lge.com>
 *
 * Copyright (C) 2011 LGE, Inc.
 * Author: Yoon Gi Souk <gisouk.yoon@lge.com>
 *
 * Copyright (C) 2012 LGE, Inc.
 * Author: Park Gyu Hwa <gyuhwa.park@lge.com>
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

/* Interface is following;
 * source file is android/frameworks/base/services/java/com/android/server/HeadsetObserver.java
 * HEADSET_UEVENT_MATCH = "DEVPATH=/sys/devices/virtual/switch/h2w"
 * HEADSET_STATE_PATH = /sys/class/switch/h2w/state
 * HEADSET_NAME_PATH = /sys/class/switch/h2w/name
 */

#ifdef CONFIG_SWITCH_MAX1462X
#define CONFIG_MAX1462X_USE_LOCAL_WORK_QUEUE
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/switch.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/mutex.h>
#include <linux/hrtimer.h>
#include <linux/input.h>
#include <linux/debugfs.h>
#include <linux/wakelock.h>
#include <linux/platform_data/hds_max1462x.h>
#include <linux/jiffies.h>
#include <linux/mfd/pm8xxx/pm8921.h>	


#undef  LGE_HSD_DEBUG_PRINT /*TODO*/
#define LGE_HSD_DEBUG_PRINT /*TODO*/
#undef  LGE_HSD_ERROR_PRINT
#define LGE_HSD_ERROR_PRINT

#define HOOK_MIN			0
#define HOOK_MAX		       150000 
#define VUP_MIN			150000						 			
#define VUP_MAX			400000 
#define VDOWN_MIN		       400000
#define VDOWN_MAX		       600000

/* TODO */
/* 1. coding for additional excetion case in probe function */
/* 2. additional sleep related/excetional case  */

#if defined(LGE_HSD_DEBUG_PRINT)
#define HSD_DBG(fmt, args...) printk(KERN_INFO "HSD.max1462x[%-18s:%5d]" fmt, __func__, __LINE__, ## args)
#else
#define HSD_DBG(fmt, args...) do {} while (0)
#endif

#if defined(LGE_HSD_ERROR_PRINT)
#define HSD_ERR(fmt, args...) printk(KERN_ERR "HSD.max1462x[%-18s:%5d]" fmt, __func__, __LINE__, ## args)
#else
#define HSD_ERR(fmt, args...) do { } while (0)
#endif

#ifdef CONFIG_MAX1462X_USE_LOCAL_WORK_QUEUE
static struct workqueue_struct *local_max1462x_workqueue;
#endif

static struct wake_lock ear_hook_wake_lock;

struct ear_3button_info_table {
       unsigned int ADC_HEADSET_BUTTON;
       int PERMISS_REANGE_MAX;
       int PERMISS_REANGE_MIN;
       int PRESS_OR_NOT;
};

/* This table is only for J1 */
static struct ear_3button_info_table max1462x_ear_3button_type_data[]={
	{KEY_MEDIA, HOOK_MAX, HOOK_MIN, 0}, 
       {KEY_VOLUMEUP, VUP_MAX, VUP_MIN, 0},
	{KEY_VOLUMEDOWN, VDOWN_MAX, VDOWN_MIN, 0} 
}; 
	
	
struct hsd_info {
/* function devices provided by this driver */
	struct switch_dev sdev;
	struct input_dev *input;

/* mutex */
	struct mutex mutex_lock;

/* h/w configuration : initilized by platform data */
	unsigned int gpio_mode;		/* MODE : high, low, high-z */
	unsigned int gpio_swd;		/* SWD : to detect 3pole or 4pole | to detect among hook, volum up or down key */
	unsigned int gpio_det;		/* DET : to detect jack inserted or not */
       unsigned int external_ldo_mic_bias;  /* External LDO control */
       void (*set_headset_mic_bias)(int enable); /* callback function which is initialized while probing */
       void (*gpio_set_value_func)(unsigned gpio, int value);
       int (*gpio_get_value_func)(unsigned gpio);

	unsigned int latency_for_detection;
	unsigned int latency_for_key;

	unsigned int key_code;	// KEY_MEDIA, KEY_VOLUMEUP or KEY_VOLUMEDOWN

/* irqs */
	unsigned int irq_detect;	// det
	unsigned int irq_key;		// swd

/* internal states */
	atomic_t irq_key_enabled;
	atomic_t is_3_pole_or_not;
	atomic_t btn_state;

/* work for detect_work */
	struct work_struct work;
	struct delayed_work work_for_key_pressed;
	struct delayed_work work_for_key_released;

//	struct delayed_work work_for_polling_ldo;
	unsigned char *pdev_name;	// KEY_MEDIA, KEY_VOLUMEUP or KEY_VOLUMEDOWN
};

enum {
	NO_DEVICE   = 0,
	LGE_HEADSET = (1 << 0),
	LGE_HEADSET_NO_MIC = (1 << 1),
};

enum {
	FALSE = 0,
	TRUE = 1,
};

static ssize_t lge_hsd_print_name(struct switch_dev *sdev, char *buf)
{
	switch (switch_get_state(sdev)) {

	case NO_DEVICE:
		return sprintf(buf, "No Device");

	case LGE_HEADSET:
		return sprintf(buf, "Headset");

	case LGE_HEADSET_NO_MIC:
		return sprintf(buf, "Headset");

	}
	return -EINVAL;
}

static ssize_t lge_hsd_print_state(struct switch_dev *sdev, char *buf)
{
	return sprintf(buf, "%d\n", switch_get_state(sdev));
}

static void button_pressed(struct work_struct *work)
{
	struct delayed_work *dwork = container_of(work, struct delayed_work, work);
	struct hsd_info *hi = container_of(dwork, struct hsd_info, work_for_key_pressed);
	struct pm8xxx_adc_chan_result result;
	int acc_read_value = 0;
	int i, rc;
	int count = 3;
       struct ear_3button_info_table *table;
	int table_size = ARRAY_SIZE(max1462x_ear_3button_type_data);
//     if (gpio_get_value(hi->gpio_det) || (switch_get_state(&hi->sdev) != LGE_HEADSET)){
       if (hi->gpio_get_value_func(hi->gpio_det)){
		HSD_ERR("button_pressed but ear jack is plugged out already! just ignore the event.\n");
		return;
	}
	for (i = 0; i < count; i++) {
       rc = pm8xxx_adc_mpp_config_read(PM8XXX_AMUX_MPP_1,
				ADC_MPP_1_AMUX6, &result);
		
       if (rc < 0) {
			if (rc == -ETIMEDOUT) {
				pr_err("[DEBUG] button_pressed : adc read timeout \n");
			} else {
	    			pr_err("button_pressed: adc read error - %d\n", rc);
			}
		}

		acc_read_value = (int)result.physical;
		pr_info("%s: acc_read_value - %d\n", __func__, (int)result.physical);
	}
//	msleep(5);
       
       for (i = 0; i < table_size; i++) {
			table = &max1462x_ear_3button_type_data[i];
		if ((acc_read_value <= table->PERMISS_REANGE_MAX)&&(acc_read_value > table->PERMISS_REANGE_MIN)) {
			HSD_DBG("button_pressed \n");
              	atomic_set(&hi->btn_state, 1);
              	       switch(table->ADC_HEADSET_BUTTON){
              		case  KEY_MEDIA :
			              input_report_key(hi->input, KEY_MEDIA, 1);
                                   pr_info("%s: KEY_MEDIA \n", __func__);
			              break;
			       case KEY_VOLUMEUP :
			              input_report_key(hi->input, KEY_VOLUMEUP, 1);
                                   pr_info("%s: KEY_VOLUMEUP \n", __func__);
			              break;
			       case KEY_VOLUMEDOWN :
			              input_report_key(hi->input, KEY_VOLUMEDOWN, 1);
                                   pr_info("%s: KEY_VOLUMDOWN \n", __func__);
			              break;
		              }
                     table->PRESS_OR_NOT = 1;
			input_sync(hi->input);
       		break;
		}
	}
}

static void button_released(struct work_struct *work)
{
	struct delayed_work *dwork = container_of(work, struct delayed_work, work);
	struct hsd_info *hi = container_of(dwork, struct hsd_info, work_for_key_released);
       struct ear_3button_info_table *table;
	int table_size = ARRAY_SIZE(max1462x_ear_3button_type_data);
       int i;

//	if (gpio_get_value(hi->gpio_det) || (switch_get_state(&hi->sdev) != LGE_HEADSET)){
       if (hi->gpio_get_value_func(hi->gpio_det)){
		HSD_ERR("button_released but ear jack is plugged out already! just ignore the event.\n");
		return;
	}

	HSD_DBG("button_released \n");
       for (i = 0; i < table_size; i++) {
			table = &max1462x_ear_3button_type_data[i];
		if (table->PRESS_OR_NOT) {
              	atomic_set(&hi->btn_state, 0);
              	       switch(table->ADC_HEADSET_BUTTON){
              		case  KEY_MEDIA :
			              input_report_key(hi->input, KEY_MEDIA, 0);
			              break;
			       case KEY_VOLUMEUP :
			              input_report_key(hi->input, KEY_VOLUMEUP, 0);
			              break;
			       case KEY_VOLUMEDOWN :
			              input_report_key(hi->input, KEY_VOLUMEDOWN, 0);
			              break;
		              }
                     table->PRESS_OR_NOT = 0;
			input_sync(hi->input);
       		break;
		}
	}
}

static void insert_headset(struct hsd_info *hi)
{
	int earjack_type;

	HSD_DBG("insert_headset");

	if (hi->set_headset_mic_bias) {
		hi->set_headset_mic_bias(TRUE);
       }
       else {
              hi->gpio_set_value_func(hi->external_ldo_mic_bias, 1);
       }
	msleep(20);

	// check if 3-pole or 4-pole
	// 1. read gpio_swd
	// 2. check if 3-pole or 4-pole
	// 3-1. NOT regiter irq with gpio_swd if 3-pole. complete.
	// 3-2. regiter irq with gpio_swd if 4-pole
	// 4. read MPP1 and decide a pressed key when interrupt occurs

	// 1. read gpio_swd
	earjack_type = hi->gpio_get_value_func(hi->gpio_swd);

	// 2. check if 3-pole or 4-pole
	if ( earjack_type == 1 ) {	// high 		
		
		HSD_DBG("4 polarity earjack");		

		atomic_set(&hi->is_3_pole_or_not, 0);

		mutex_lock(&hi->mutex_lock);
		switch_set_state(&hi->sdev, LGE_HEADSET);
		mutex_unlock(&hi->mutex_lock);

		if (!atomic_read(&hi->irq_key_enabled)) {
			unsigned long irq_flags;
			HSD_DBG("irq_key_enabled = FALSE");

			local_irq_save(irq_flags);
			enable_irq(hi->irq_key);
			local_irq_restore(irq_flags);

			atomic_set(&hi->irq_key_enabled, TRUE);
		}
		input_report_switch(hi->input, SW_HEADPHONE_INSERT, 1);
		input_report_switch(hi->input, SW_MICROPHONE_INSERT, 1);
		input_sync(hi->input);

		
	} else {	// low 
		HSD_DBG("3 polarity earjack");

		atomic_set(&hi->is_3_pole_or_not, 1);

		mutex_lock(&hi->mutex_lock);
		switch_set_state(&hi->sdev, LGE_HEADSET_NO_MIC);
		mutex_unlock(&hi->mutex_lock);

	       irq_set_irq_wake(hi->irq_key, 0);

              if (hi->set_headset_mic_bias) {
			hi->set_headset_mic_bias(FALSE);
              }
              else {
                     hi->gpio_set_value_func(hi->external_ldo_mic_bias, 0);
              }
		input_report_switch(hi->input, SW_HEADPHONE_INSERT, 1);
		input_sync(hi->input);
	}	
}

static void remove_headset(struct hsd_info *hi)
{
	int has_mic = switch_get_state(&hi->sdev);
	
	HSD_DBG("remove_headset");

       atomic_set(&hi->is_3_pole_or_not, 1);
	mutex_lock(&hi->mutex_lock);
	switch_set_state(&hi->sdev, NO_DEVICE);
	mutex_unlock(&hi->mutex_lock);

	if (atomic_read(&hi->irq_key_enabled)) {
		unsigned long irq_flags;

		local_irq_save(irq_flags);
		disable_irq(hi->irq_key);
		local_irq_restore(irq_flags);
		atomic_set(&hi->irq_key_enabled, FALSE);
	}

	if (atomic_read(&hi->btn_state))
#ifdef	CONFIG_MAX1462X_USE_LOCAL_WORK_QUEUE
	queue_delayed_work(local_max1462x_workqueue, &(hi->work_for_key_released), hi->latency_for_key );
#else
	schedule_delayed_work(&(hi->work_for_key_released), hi->latency_for_key );
#endif
	input_report_switch(hi->input, SW_HEADPHONE_INSERT, 0);
	if (has_mic == LGE_HEADSET)
		input_report_switch(hi->input, SW_MICROPHONE_INSERT, 0);
	input_sync(hi->input);
}

static void detect_work(struct work_struct *work)
{
	int state;
       struct hsd_info *hi = container_of(work, struct hsd_info, work);

	HSD_DBG("detect_work");

	state = hi->gpio_get_value_func(hi->gpio_det);

	if (state == 1) {
		if (switch_get_state(&hi->sdev) != NO_DEVICE) {
			HSD_DBG("==== LGE headset removing\n");
			remove_headset(hi);
		} else {
			HSD_DBG("err_invalid_state state = %d\n", state);
		}
	} else {

		if (switch_get_state(&hi->sdev) == NO_DEVICE) {
			HSD_DBG("==== LGE headset inserting\n");
			insert_headset(hi);
		} else {
			HSD_DBG("err_invalid_state state = %d\n", state);
		}
	}
}

static irqreturn_t earjack_det_irq_handler(int irq, void *dev_id)
{
	struct hsd_info *hi = (struct hsd_info *) dev_id;

	wake_lock_timeout(&ear_hook_wake_lock, 2 * HZ);

       if (hi->set_headset_mic_bias) {
		hi->set_headset_mic_bias(FALSE);
       }
       else {
              hi->gpio_set_value_func(hi->external_ldo_mic_bias, 0);
       }
	HSD_DBG("earjack_det_irq_handler");

#ifdef CONFIG_MAX1462X_USE_LOCAL_WORK_QUEUE
	queue_work(local_max1462x_workqueue, &(hi->work));
#else
	schedule_work(&(hi->work));
#endif
	return IRQ_HANDLED;
}

static irqreturn_t button_irq_handler(int irq, void *dev_id)
{
	struct hsd_info *hi = (struct hsd_info *) dev_id;

	int value;

	wake_lock_timeout(&ear_hook_wake_lock, 2 * HZ); 

	HSD_DBG("button_irq_handler");
	printk(KERN_INFO "button_irq_handler\n");

	value = hi->gpio_get_value_func(hi->gpio_swd);	// low:pressed, high:released

	HSD_DBG("==========hi->gpio_get_value_func(hi->gpio_swd) : %d =======", value);

	if (value) 
		queue_delayed_work(local_max1462x_workqueue, &(hi->work_for_key_released), hi->latency_for_key );
	else
		queue_delayed_work(local_max1462x_workqueue, &(hi->work_for_key_pressed), hi->latency_for_key );

	return IRQ_HANDLED;
}

static int lge_hsd_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct max1462x_platform_data *pdata = pdev->dev.platform_data;
	

	struct hsd_info *hi;

	HSD_DBG("lge_hsd_probe");
	printk(KERN_INFO "lge_hsdprobe\n");


	hi = kzalloc(sizeof(struct hsd_info), GFP_KERNEL);

	if (NULL == hi) {
		HSD_ERR("Failed to allloate headset per device info\n");
		return -ENOMEM;
	}

	hi->key_code = pdata->key_code;

	platform_set_drvdata(pdev, hi);

	atomic_set(&hi->btn_state, 0);
	atomic_set(&hi->is_3_pole_or_not, 1);
	
	hi->gpio_mode = pdata->gpio_mode;
	hi->gpio_det = pdata->gpio_det;
	hi->gpio_swd = pdata->gpio_swd;
	hi->external_ldo_mic_bias = pdata->external_ldo_mic_bias;
	hi->set_headset_mic_bias = pdata->set_headset_mic_bias;
	hi->gpio_set_value_func = pdata->gpio_set_value_func;
	hi->gpio_get_value_func = pdata->gpio_get_value_func;

	hi->latency_for_detection = pdata->latency_for_detection;

	hi->latency_for_key = msecs_to_jiffies(50); /* convert milli to jiffies */
	mutex_init(&hi->mutex_lock);
	INIT_WORK(&hi->work, detect_work);
	INIT_DELAYED_WORK(&hi->work_for_key_pressed, button_pressed);
	INIT_DELAYED_WORK(&hi->work_for_key_released, button_released);

	// init gpio_mode	
	ret = gpio_request(hi->gpio_mode, "gpio_mode");
	if (ret < 0) {
		HSD_ERR("Failed to configure gpio%d (gpio_mode) gpio_request\n", hi->gpio_mode);
		goto error_01;
	}
	
	ret = gpio_direction_output(hi->gpio_mode, 1);
	if (ret < 0) {
		HSD_ERR("Failed to configure gpio%d (gpio_mode) gpio_direction_input\n", hi->gpio_mode);
		goto error_02;
	}
	HSD_DBG("gpio_get_value_cansleep(hi->gpio_mode) = %d \n", gpio_get_value_cansleep(hi->gpio_mode));

	// init external_ldo_mic_bias
	ret = gpio_request(hi->external_ldo_mic_bias, "external_ldo_mic_bias");
	if (ret < 0) {
		HSD_ERR("Failed to configure gpio%d (external_ldo_mic_bias) gpio_request\n", hi->external_ldo_mic_bias);
		goto error_05;
	}
	
	ret = gpio_direction_output(hi->external_ldo_mic_bias, 0);
	if (ret < 0) {
		HSD_ERR("Failed to configure gpio%d (external_ldo_mic_bias) gpio_direction_input\n", hi->external_ldo_mic_bias);
		goto error_05;
	}
	HSD_DBG("gpio_get_value_cansleep(hi->external_ldo_mic_bias) = %d \n", gpio_get_value_cansleep(hi->external_ldo_mic_bias));

	// init gpio_det	
	ret = gpio_request(hi->gpio_det, "gpio_det");
	if (ret < 0) {
		HSD_ERR("Failed to configure gpio%d (gpio_det) gpio_request\n", hi->gpio_det);
		goto error_03;
	}

	ret = gpio_direction_input(hi->gpio_det);
	if (ret < 0) {
		HSD_ERR("Failed to configure gpio%d (gpio_det) gpio_direction_input\n", hi->gpio_det);
		goto error_03;
	}

	// init gpio_swd	
	ret = gpio_request(hi->gpio_swd, "gpio_swd");
	if (ret < 0) {
		HSD_ERR("Failed to configure gpio%d (gpio_swd) gpio_request\n", hi->gpio_swd);
		goto error_04;
	}

	ret = gpio_direction_input(hi->gpio_swd);
	if (ret < 0) {
		HSD_ERR("Failed to configure gpio%d (gpio_swd) gpio_direction_input\n", hi->gpio_swd);
		goto error_04;
	}

	/* initialize irq of detection */
	hi->irq_detect = gpio_to_irq(hi->gpio_det);

	HSD_DBG("hi->irq_detect = %d\n", hi->irq_detect);

	if (hi->irq_detect < 0) {
		HSD_ERR("Failed to get interrupt number\n");
		ret = hi->irq_detect;
		goto error_06;
	}

	ret = request_threaded_irq(hi->irq_detect, NULL, earjack_det_irq_handler,
					IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING, pdev->name, hi);

	if (ret) {
		HSD_ERR("failed to request button irq");
		goto error_06;
	}

	ret = irq_set_irq_wake(hi->irq_detect, 1);
	if (ret < 0) {
		HSD_ERR("Failed to set irq_detect interrupt wake\n");
		goto error_06;
	}

	/* initialize irq of gpio_key */
	hi->irq_key = gpio_to_irq(hi->gpio_swd);

	HSD_DBG("hi->irq_key = %d\n", hi->irq_key);

	if (hi->irq_key < 0) {
		HSD_ERR("Failed to get interrupt number\n");
		ret = hi->irq_key;
		goto error_07;
	}
	ret = request_threaded_irq(hi->irq_key, NULL, button_irq_handler,
						IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING, pdev->name, hi);
	if (ret) {
		HSD_ERR("failed to request button irq");
		goto error_07;
	}

	disable_irq(hi->irq_key);

	ret = irq_set_irq_wake(hi->irq_key, 1);
	if (ret < 0) {
		HSD_ERR("Failed to set irq_key interrupt wake\n");
		goto error_07;
	}

	/* initialize switch device */
	hi->sdev.name = pdata->switch_name;
	hi->sdev.print_state = lge_hsd_print_state;
	hi->sdev.print_name = lge_hsd_print_name;

	ret = switch_dev_register(&hi->sdev);
	if (ret < 0) {
		HSD_ERR("Failed to register switch device\n");
		goto error_07;
	}

	/* initialize input device */
	hi->input = input_allocate_device();
	if (!hi->input) {
		HSD_ERR("Failed to allocate input device\n");
		ret = -ENOMEM;
		goto error_08;
	}

	hi->input->name = pdata->keypad_name;

	hi->input->id.vendor    = 0x0001;
	hi->input->id.product   = 1;
	hi->input->id.version   = 1;

// [START] // headset tx noise
{	
		struct pm8xxx_adc_chan_result result;
		int acc_read_value = 0;
		int i, rc;
		int count = 3;

		for (i = 0; i < count; i++)
		{
			rc = pm8xxx_adc_mpp_config_read(PM8XXX_AMUX_MPP_1,
				ADC_MPP_1_AMUX6, &result);

			if (rc < 0)
			{
				if (rc == -ETIMEDOUT) {
					pr_err("[DEBUG]adc read timeout \n");
				} else {
		    			pr_err("[DEBUG]adc read error - %d\n", rc);
				}
			}
			else
			{
				acc_read_value = (int)result.physical;
				pr_info("%s: acc_read_value - %d\n", __func__, (int)result.physical);
				break;
			}
		}
}
// [END]

	/*input_set_capability(hi->input, EV_SW, SW_HEADPHONE_INSERT);*/
	set_bit(EV_SYN, hi->input->evbit);
	set_bit(EV_KEY, hi->input->evbit);
	set_bit(EV_SW, hi->input->evbit);
	set_bit(hi->key_code, hi->input->keybit);
	set_bit(SW_HEADPHONE_INSERT, hi->input->swbit);
	set_bit(SW_MICROPHONE_INSERT, hi->input->swbit);
	input_set_capability(hi->input, EV_KEY, KEY_MEDIA);
	input_set_capability(hi->input, EV_KEY, KEY_VOLUMEUP);
	input_set_capability(hi->input, EV_KEY, KEY_VOLUMEDOWN);
	ret = input_register_device(hi->input);
	if (ret) {
		HSD_ERR("Failed to register input device\n");
		goto error_09;
	}

	if (!(hi->gpio_get_value_func(hi->gpio_det)))
#ifdef CONFIG_MAX1462X_USE_LOCAL_WORK_QUEUE
		queue_work(local_max1462x_workqueue, &(hi->work)); /* to detect in initialization with eacjack insertion */
#else
		schedule_work(&(hi->work)); /* to detect in initialization with eacjack insertion */
#endif

#ifdef AT_TEST_GPKD
	err = device_create_file(&pdev->dev, &dev_attr_hookkeylog);
#endif

	return ret;

error_09:
	input_free_device(hi->input);
error_08:
	switch_dev_unregister(&hi->sdev);
error_07:
	free_irq(hi->irq_key, 0);
error_06:
	free_irq(hi->irq_detect, 0);
error_05:
	gpio_free(hi->external_ldo_mic_bias);
error_04:
	gpio_free(hi->gpio_swd);
error_03:
	gpio_free(hi->gpio_det);
error_02:
	gpio_free(hi->gpio_mode);
error_01:
	mutex_destroy(&hi->mutex_lock);
	kfree(hi);

	return ret;
}

static int lge_hsd_remove(struct platform_device *pdev)
{
	struct hsd_info *hi = (struct hsd_info *)platform_get_drvdata(pdev);

	HSD_DBG("lge_hsd_remove");

	if (switch_get_state(&hi->sdev))
		remove_headset(hi);

	input_unregister_device(hi->input);
	switch_dev_unregister(&hi->sdev);

	free_irq(hi->irq_key, 0);
	free_irq(hi->irq_detect, 0);
	gpio_free(hi->gpio_det);
	gpio_free(hi->gpio_swd);
	gpio_free(hi->gpio_mode);
       gpio_free(hi->external_ldo_mic_bias);

	mutex_destroy(&hi->mutex_lock);

	kfree(hi);

	return 0;
}

static struct platform_driver lge_hsd_driver = {
	.probe          = lge_hsd_probe,
	.remove         = lge_hsd_remove,
	.driver         = {
	.name           = "max1462x", 
	.owner          = THIS_MODULE,
	},
};

static int __init lge_hsd_init(void)
{
	int ret;

	HSD_DBG("lge_hsd_init");

#ifdef CONFIG_MAX1462X_USE_LOCAL_WORK_QUEUE
	local_max1462x_workqueue = create_workqueue("max1462x") ;
	if(!local_max1462x_workqueue)
		return -ENOMEM;
#endif
       HSD_DBG("lge_hsd_init : wake_lock_init");
       wake_lock_init(&ear_hook_wake_lock, WAKE_LOCK_SUSPEND, "ear_hook");

	ret = platform_driver_register(&lge_hsd_driver);
	if (ret) {
		HSD_ERR("Fail to register platform driver\n");
	}

	return ret;
}

static void __exit lge_hsd_exit(void)
{
#ifdef CONFIG_MAX1462X_USE_LOCAL_WORK_QUEUE
	if(local_max1462x_workqueue)
		destroy_workqueue(local_max1462x_workqueue);
	local_max1462x_workqueue = NULL;
#endif

	platform_driver_unregister(&lge_hsd_driver);
	HSD_DBG("lge_hsd_exit");
	wake_lock_destroy(&ear_hook_wake_lock);
}

/* to make init after pmic8058-othc module */
/* module_init(lge_hsd_init); */
late_initcall_sync(lge_hsd_init);
module_exit(lge_hsd_exit);

MODULE_AUTHOR("Park Gyu Hwa <gyuhwa.park@lge.com>");
MODULE_DESCRIPTION("LGE Headset detection driver (max1462x)");
MODULE_LICENSE("GPL");
#endif
