/*
 *  Copyright (C) 2011-2012, LG Eletronics,Inc. All rights reserved.
 *      LGIT LCD device driver
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
#include <linux/gpio.h>


#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_lgit.h"

#include "mdp4.h"
static struct msm_panel_common_pdata *mipi_lgit_pdata;

static struct dsi_buf lgit_tx_buf;
static struct dsi_buf lgit_rx_buf;
static int __init mipi_lgit_lcd_init(void);

#define DSV_ONBST 57

// To prevent doubletap2wake 3 taps issue when suspended. - by jollaman999
// - jollaman999 -
#ifdef CONFIG_TOUCHSCREEN_DOUBLETAP2WAKE
extern bool scr_suspended;
#endif

//LGE_UPDATE_S hojin.ryu@lge.com 20120629 IEF On/Off function for camera preview
#define LGIT_IEF_SWITCH

#ifdef LGIT_IEF_SWITCH
struct msm_fb_data_type *local_mfd0 = NULL;
static int is_ief_on = 1;
#endif

#ifdef LGIT_IEF_SWITCH
int mipi_lgit_lcd_ief_off(void)
{
	if(local_mfd0->panel_power_on && is_ief_on) {	
		printk("IEF_OFF Starts with Camera\n");
		mutex_lock(&local_mfd0->dma->ov_mutex);
		MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x10000000);//HS mode
		mipi_dsi_cmds_tx(&lgit_tx_buf, mipi_lgit_pdata->power_off_set_ief, mipi_lgit_pdata->power_off_set_ief_size);
			
		printk("%s, %d\n", __func__,is_ief_on);
		MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x14000000);//LP mode
		mutex_unlock(&local_mfd0->dma->ov_mutex);
		printk("IEF_OFF Ends with Camera\n");
	}
	is_ief_on = 0;
                                                                                         
	return 0;
} 

int mipi_lgit_lcd_ief_on(void)
{	
	if(local_mfd0->panel_power_on && !is_ief_on) {
		printk("IEF_ON Starts with Camera\n");
		mutex_lock(&local_mfd0->dma->ov_mutex);
		MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x10000000);//HS mode
		mipi_dsi_cmds_tx(&lgit_tx_buf, mipi_lgit_pdata->power_on_set_ief, mipi_lgit_pdata->power_on_set_ief_size); 
							
		printk("%s, %d\n", __func__,is_ief_on);
		MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x14000000); //LP mode
		mutex_unlock(&local_mfd0->dma->ov_mutex);
		printk("IEF_ON Ends with Camera\n");
	}
        is_ief_on = 1;
                    
	return 0;                                                                             
} 
#endif
//LGE_UPDATE_E hojin.ryu@lge.com 20120629 IEF On/Off function for camera preview

 int mipi_lgit_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	int rc = 0;
	int cnt = 0;
	
	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

#ifdef LGIT_IEF_SWITCH
	if(local_mfd0 == NULL)
		local_mfd0 = mfd;
#endif

	printk(KERN_INFO "%s: mipi lgit lcd on started \n", __func__);
	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x10000000);
	cnt = mipi_dsi_cmds_tx(&lgit_tx_buf,
			mipi_lgit_pdata->power_on_set_1,
			mipi_lgit_pdata->power_on_set_size_1);
	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x14000000);
	if (cnt < 0)
		return cnt;

	mipi_dsi_op_mode_config(DSI_VIDEO_MODE);
	mdp4_overlay_dsi_video_start();

	mdelay(10);

	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x10000000);//HS mode
	cnt = mipi_dsi_cmds_tx(&lgit_tx_buf,
		mipi_lgit_pdata->power_on_set_2,
		mipi_lgit_pdata->power_on_set_size_2);
	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x14000000);//LP mode

	if (cnt < 0)
		return cnt;
	rc = gpio_request(DSV_ONBST,"DSV_ONBST_en");
	if (rc) {
		printk(KERN_INFO "%s: DSV_ONBST Request Fail \n", __func__);
	} else {
		rc = gpio_direction_output(DSV_ONBST, 1);
		if (rc) {
			printk(KERN_INFO "%s: DSV_ONBST Direction Set Fail \n"
					, __func__);
		} else {
			gpio_set_value(DSV_ONBST, 1);
		}
		gpio_free(DSV_ONBST);
	}
	mdelay(20);
	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x10000000);
	cnt = mipi_dsi_cmds_tx(&lgit_tx_buf,
			mipi_lgit_pdata->power_on_set_3,
			mipi_lgit_pdata->power_on_set_size_3);
	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x14000000);

	printk(KERN_INFO "%s: mipi lgit lcd on ended \n", __func__);
	// To prevent doubletap2wake 3 taps issue when suspended. - by jollaman999
	// - jollaman999 -
#ifdef CONFIG_TOUCHSCREEN_DOUBLETAP2WAKE
	scr_suspended = false;
#endif
	return cnt;
}

int mipi_lgit_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	int rc = 0;
	int cnt = 0;

	if (mipi_lgit_pdata->bl_pwm_disable)
		mipi_lgit_pdata->bl_pwm_disable();

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;

	if (mfd->key != MFD_KEY)
		return -EINVAL;

	printk(KERN_INFO"%s: mipi lgit lcd off started \n", __func__);
	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x10000000);
	cnt = mipi_dsi_cmds_tx(&lgit_tx_buf,
			mipi_lgit_pdata->power_off_set_1,
			mipi_lgit_pdata->power_off_set_size_1);
	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x14000000);
	if (cnt < 0)
		return cnt;
	rc = gpio_request(DSV_ONBST,"DSV_ONBST_en");

	if (rc) {
		printk(KERN_INFO "%s: DSV_ONBST Request Fail \n", __func__);
	} else {
		rc = gpio_direction_output(DSV_ONBST, 1);
		if (rc) {
			printk(KERN_INFO "%s: DSV_ONBST Direction Set Fail \n",
					__func__);
		} else {
			gpio_set_value(DSV_ONBST, 0);
		}
		gpio_free(DSV_ONBST);
	}

	mdelay(20);
	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x10000000);
	cnt = mipi_dsi_cmds_tx(&lgit_tx_buf,
			mipi_lgit_pdata->power_off_set_2,
			mipi_lgit_pdata->power_off_set_size_2);
	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x14000000);

	printk(KERN_INFO"%s: mipi lgit lcd off ended \n", __func__);
	// To prevent doubletap2wake 3 taps issue when suspended. - by jollaman999
	// - jollaman999 -
#ifdef CONFIG_TOUCHSCREEN_DOUBLETAP2WAKE
	scr_suspended = true;
#endif
	return cnt;
}

int mipi_lgit_lcd_off_for_shutdown(void)
{
    struct msm_fb_panel_data *pdata = NULL;

    if(!local_mfd0 || !local_mfd0->panel_power_on)
        return -1;

    pdata = local_mfd0->pdev->dev.platform_data;

    printk("%s: mipi_lgit_lcd_off_for_shutdown start.\n", __func__);

    pdata->off(local_mfd0->pdev);

    printk("%s: mipi_lgit_lcd_off_for_shutdown done.\n", __func__);

	return 0;
}

static int mipi_lgit_backlight_on_status(void)
{
	return (mipi_lgit_pdata->bl_on_status());
}

static void mipi_lgit_set_backlight_board(struct msm_fb_data_type *mfd)
{
	int level;

	level = (int)mfd->bl_level;
	mipi_lgit_pdata->backlight_level(level, 0, 0);
}

static int mipi_lgit_lcd_probe(struct platform_device *pdev)
{
	if (pdev->id == 0) {
		mipi_lgit_pdata = pdev->dev.platform_data;
		return 0;
	}

	printk(KERN_INFO "%s: mipi lgit lcd probe start\n", __func__);

	msm_fb_add_device(pdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe = mipi_lgit_lcd_probe,
	.driver = {
		.name = "mipi_lgit",
	},
};

static struct msm_fb_panel_data lgit_panel_data = {
	.on = mipi_lgit_lcd_on,
	.off = mipi_lgit_lcd_off,
	.set_backlight = mipi_lgit_set_backlight_board,
	.get_backlight_on_status = mipi_lgit_backlight_on_status,
};

static int ch_used[3];

int mipi_lgit_device_register(struct msm_panel_info *pinfo,
		u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_lgit", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	lgit_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &lgit_panel_data,
			sizeof(lgit_panel_data));
	if (ret) {
		printk(KERN_ERR "%s: platform_device_add_data failed!\n",
				__func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		printk(KERN_ERR "%s: platform_device_register failed!\n",
				__func__);
		goto err_device_put;
	}
	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}

static int __init mipi_lgit_lcd_init(void)
{
	mipi_dsi_buf_alloc(&lgit_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&lgit_rx_buf, DSI_BUF_SIZE);

	return platform_driver_register(&this_driver);
}

module_init(mipi_lgit_lcd_init);
