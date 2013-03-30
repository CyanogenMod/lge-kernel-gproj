/* Copyright (c) 2008-2010, Code Aurora Forum. All rights reserved.
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

#include <mach/board_lge.h>

#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_lgit_fhd.h"

#include "mdp4.h"

static struct msm_panel_common_pdata *mipi_lgit_pdata;

static struct dsi_buf lgit_tx_buf;
static struct dsi_buf lgit_rx_buf;
static struct dsi_buf lgit_camera_tx_buf;
static struct dsi_buf lgit_shutdown_tx_buf;

static int __init mipi_lgit_lcd_init(void);

#if defined(CONFIG_LGIT_COLOR_ENGINE_SWITCH)
static int is_color_engine_on = 1;
struct msm_fb_data_type *local_mfd0 = NULL;
int mipi_lgit_lcd_color_engine_off(void)
{
       if(is_color_engine_on) {
		printk("Color Engine_OFF Starts with Camera\n");
              mutex_lock(&local_mfd0->dma->ov_mutex);

		MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x10000000);//HS mode
		mipi_dsi_cmds_tx(&lgit_camera_tx_buf,
              mipi_lgit_pdata->color_engine_off, 
              mipi_lgit_pdata->color_engine_off_size);
		printk("%s, %d\n", __func__,is_color_engine_on);
		MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x14000000);//LP mode

              mutex_unlock(&local_mfd0->dma->ov_mutex);
              printk("Color Engine_OFF Ends with Camera\n");
	}
	is_color_engine_on = 0;
	return 0;
}

int mipi_lgit_lcd_color_engine_on(void)
{
       if(!is_color_engine_on) {
		printk("Color Engine_ON Starts with Camera\n");
	       mutex_lock(&local_mfd0->dma->ov_mutex);

		MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x10000000);//HS mode
		mipi_dsi_cmds_tx(&lgit_camera_tx_buf,
              mipi_lgit_pdata->color_engine_on,
              mipi_lgit_pdata->color_engine_on_size);
		printk("%s, %d\n", __func__,is_color_engine_on);
		MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x14000000); //LP mode

		mutex_unlock(&local_mfd0->dma->ov_mutex);
		printk("Color Engine_ON Ends with Camera\n");
	}
       is_color_engine_on = 1;
	return 0;  
}
#endif //CONFIG_LGIT_COLOR_ENGINE_SWITCH

static int check_stable_lcd_on = 1;

static int mipi_stable_lcd_on(struct platform_device *pdev)
{
       int ret = 0;
       int retry_cnt = 0;

       do {
              printk("[LCD][DEBUG] %s, retry_cnt=%d\n", __func__, retry_cnt);
              ret = mipi_lgit_lcd_off(pdev);

              if (ret < 0) {
                     msleep(3);
                     retry_cnt++;
              }
              else {
                     check_stable_lcd_on = 0;
                     break;
              }
       } while(retry_cnt < 10);
       mdelay(10);

       return ret;
}

int mipi_lgit_lcd_on(struct platform_device *pdev)
{
       int cnt = 0;
	struct msm_fb_data_type *mfd;

       if (check_stable_lcd_on)
              mipi_stable_lcd_on(pdev);

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;
#if defined(CONFIG_LGIT_COLOR_ENGINE_SWITCH)
       if(local_mfd0 == NULL)
              local_mfd0 = mfd;
#endif
	printk(KERN_INFO "[LCD][DEBUG] %s is started \n", __func__);

//LGE_UPDATE_S hj.eum@lge.com : adding change mipi mode to write register setting of LCD IC
	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x10000000);     //HS mode

#if defined (CONFIG_MACH_APQ8064_GVDCM)
       if(lge_get_board_revno() != HW_REV_D){
		printk("[LCD][DEBUG] initial code of LCD for GVDCM\n");
              cnt = mipi_dsi_cmds_tx(&lgit_tx_buf,
		    mipi_lgit_pdata->power_on_set_1_old,
		    mipi_lgit_pdata->power_on_set_size_1_old);
              if (cnt < 0)
                     return cnt;
       }else{
		printk("[LCD][DEBUG] initial code of LCD for GVDCM Rev.D\n");
              cnt = mipi_dsi_cmds_tx(&lgit_tx_buf,
		    mipi_lgit_pdata->power_on_set_1,
		    mipi_lgit_pdata->power_on_set_size_1);
              if (cnt < 0)
                     return cnt;
       }
#else
       cnt = mipi_dsi_cmds_tx(&lgit_tx_buf,
              mipi_lgit_pdata->power_on_set_1,
		mipi_lgit_pdata->power_on_set_size_1);
       if (cnt < 0)
              return cnt;
#endif
	cnt = mipi_dsi_cmds_tx(&lgit_tx_buf,
		mipi_lgit_pdata->power_on_set_2, 
		mipi_lgit_pdata->power_on_set_size_2);
       if (cnt < 0)
              return cnt;

	mipi_dsi_op_mode_config(DSI_VIDEO_MODE);
       mdp4_overlay_dsi_video_start();
	mdelay(120);
#if defined(CONFIG_LGE_R63311_BACKLIGHT_CABC)
       cnt = mipi_dsi_cmds_tx(&lgit_tx_buf,
		mipi_lgit_pdata->power_on_set_3,
		mipi_lgit_pdata->power_on_set_size_3);
       if (cnt < 0)
              return cnt;
#endif
//LGE_UPDATE_E hj.eum@lge.com : adding change mipi mode to write register setting of LCD IC
	printk(KERN_INFO "[LCD][DEBUG] %s is ended \n", __func__);

       return cnt;
}

int mipi_lgit_lcd_off(struct platform_device *pdev)
{
       int cnt = 0;
	struct msm_fb_data_type *mfd;
	
       printk(KERN_INFO "[LCD][DEBUG] %s is started \n", __func__);

	mfd =  platform_get_drvdata(pdev);
	
	if (!mfd)
		return -ENODEV;
	
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x10000000);//HS mode
//	mipi_dsi_cmds_tx(mfd, &lgit_tx_buf, 
       cnt = mipi_dsi_cmds_tx(&lgit_tx_buf,
		mipi_lgit_pdata->power_off_set_1,	
		mipi_lgit_pdata->power_off_set_size_1);
       if (cnt < 0)
              return cnt;
#if defined(CONFIG_LGE_R63311_BACKLIGHT_CABC)
        cnt = mipi_dsi_cmds_tx(&lgit_tx_buf,
		mipi_lgit_pdata->cabc_off,	
		mipi_lgit_pdata->cabc_off_size);
       if (cnt < 0)
              return cnt;
#endif // CABC apply

	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x14000000);//LP mode
	printk(KERN_INFO "[LCD][DEBUG] %s is ended \n", __func__);
	
       return cnt;
}

#ifdef CONFIG_FB_MSM_MIPI_LGIT_VIDEO_FHD_INVERSE_PT
int mipi_lgit_lcd_off_for_shutdown(void)
{
   
    printk("%s is started \n", __func__);

    MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x10000000);//HS mode
//	mipi_dsi_cmds_tx(mfd, &lgit_tx_buf, 
        mipi_dsi_cmds_tx(&lgit_shutdown_tx_buf,
		mipi_lgit_pdata->power_off_set_1,	
		mipi_lgit_pdata->power_off_set_size_1);
	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x14000000);//LP mode

    printk("%s is ended \n", __func__);

    return 0;
}
#endif

static void mipi_lgit_set_backlight_board(struct msm_fb_data_type *mfd)
{
	int level;

	level = (int)mfd->bl_level;
	mipi_lgit_pdata->backlight_level(level, 0, 0);
}

#if defined(CONFIG_LGE_R63311_COLOR_ENGINE)
static int ce_ctrl_show(struct device * dev, struct device_attribute * attr, char * buf)
{
   int  i,r,total_len = 0;
   char str[150];
   char temp[5];	
   struct dsi_cmd_desc *reg;
    
   reg=mipi_lgit_pdata->power_on_set_1;

   for(i=0; i<33; i++){
       sprintf(temp,"%x ", reg[18].payload[i]);
	   if(i == 0)
		 strcpy(str, temp);
	   else
       	 strcat(str + total_len,temp);
	   total_len+=strlen(temp);
   }

   r = snprintf(buf, PAGE_SIZE, "color enhancement: %s\n", str);
  
   return r;
}

static int ce_ctrl_store(struct device * dev, struct device_attribute * attr, const char * buf, size_t count)
{
   char *bp;
   char **token;
   char *data = NULL;
   struct dsi_cmd_desc *reg;
   int i=0;

   reg=mipi_lgit_pdata->power_on_set_1;

   bp=(char *)buf;

   while(true){
        if(strstr(bp, ", ")){
            bp=strstr(bp, "0x");
            token=&bp;
            data=strsep(token,", ");
            if(data == NULL)
                return -1;
            reg[18].payload[i]=simple_strtol(data,NULL,16);            
            token=NULL;
        }else{
            bp=strstr(bp, "0x");
            if(bp == NULL)
                return -1;
            reg[18].payload[i]=simple_strtol(bp,NULL,16);            
            break;
        }
        i++;
    }
    return count;
}
DEVICE_ATTR(ce_ctrl, 0774, ce_ctrl_show, ce_ctrl_store);

static int aco_ctrl_show(struct device * dev, struct device_attribute * attr, char * buf)
{
    int r;
    struct dsi_cmd_desc *reg;
    
    reg=mipi_lgit_pdata->power_on_set_1;

   	r = snprintf(buf, PAGE_SIZE, "Auto Contrast Optimization contorl is : %x %x %x %x %x %x %x\n", reg[19].payload[0],reg[19].payload[1], reg[19].payload[2],
                reg[19].payload[3],reg[19].payload[4],reg[19].payload[5],reg[19].payload[6]);

    return r;
}

static int aco_ctrl_store(struct device * dev, struct device_attribute * attr, const char * buf, size_t count)
{
   char *bp;
   char **token;
   char *data = NULL;
   struct dsi_cmd_desc *reg;
   int i=0;

   reg=mipi_lgit_pdata->power_on_set_1;
   
   bp=(char *)buf;

   while(true){
        if(strstr(bp, ", ")){
            bp=strstr(bp, "0x");
            token=&bp;
            data=strsep(token,", ");
            if(data == NULL)
                return -1;
            reg[19].payload[i]=simple_strtol(data,NULL,16);         
            token=NULL;
        }else{
            bp=strstr(bp, "0x");
            if(bp == NULL)
                return -1;
            reg[19].payload[i]=simple_strtol(bp,NULL,16);            
            break;
        }
        i++;
    }
    return count;
}
DEVICE_ATTR(aco_ctrl, 0774, aco_ctrl_show, aco_ctrl_store);

static int sharp_ctrl_show(struct device * dev, struct device_attribute * attr, char * buf)
{
    int r;
    struct dsi_cmd_desc *reg;
    
    reg=mipi_lgit_pdata->power_on_set_1;

   	r = snprintf(buf, PAGE_SIZE, "Outline sharpning contorl is : %x %x %x\n", reg[20].payload[0],reg[20].payload[1], reg[20].payload[2]);

    return r;
}

static int sharp_ctrl_store(struct device * dev, struct device_attribute * attr, const char * buf, size_t count)
{
   char *bp;
   char **token;
   char *data = NULL;
   struct dsi_cmd_desc *reg;
   int i=0;

   reg=mipi_lgit_pdata->power_on_set_1;
   
   bp=(char *)buf;

   while(true){
        if(strstr(bp, ", ")){
            bp=strstr(bp, "0x");
            token=&bp;
            data=strsep(token,", ");
            if(data == NULL)
                return -1;
            reg[20].payload[i]=simple_strtol(data,NULL,16);            
            token=NULL;
        }else{
            bp=strstr(bp, "0x");
            if(bp == NULL)
                return -1;
            reg[20].payload[i]=simple_strtol(bp,NULL,16);            
            break;
        }
        i++;
    }
    return count;
}
DEVICE_ATTR(sharp_ctrl, 0774, sharp_ctrl_show, sharp_ctrl_store);
#endif

#if defined(CONFIG_LGIT_COLOR_ENGINE_SWITCH)
static ssize_t color_engine_show_on_off(struct device *dev, struct device_attribute *attr, char *buf)
{
	int r = 0;

	pr_info("%s received (prev color_engine_on: %s)\n", __func__, is_color_engine_on? "ON" : "OFF");

	return r;
}

static ssize_t color_engine_store_on_off(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int on_off;

	if (!count)
		return -EINVAL;

	pr_info("%s received (prev color_engine_on: %s)\n", __func__, is_color_engine_on ? "ON" : "OFF");

	on_off = simple_strtoul(buf, NULL, 10);

	printk(KERN_ERR "color_engine_on : %d", on_off);

	if (on_off == 1) {
	    mipi_lgit_lcd_color_engine_on();
	} else if (on_off == 0)
	    mipi_lgit_lcd_color_engine_off();

	return count;
}
DEVICE_ATTR(color_engine_on_off, 0644, color_engine_show_on_off, color_engine_store_on_off);
#endif //CONFIG_LGIT_COLOR_ENGINE_SWITCH

static int mipi_lgit_lcd_probe(struct platform_device *pdev)
{
#if defined(CONFIG_LGE_R63311_COLOR_ENGINE)
    int err;
#endif
	if (pdev->id == 0) {
		mipi_lgit_pdata = pdev->dev.platform_data;
		return 0;
	}

	printk(KERN_INFO "[LCD][DEBUG] %s: mipi lgit lcd probe start\n", __func__);

	msm_fb_add_device(pdev);
#if defined(CONFIG_LGE_R63311_COLOR_ENGINE)
    err = device_create_file(&pdev->dev, &dev_attr_sharp_ctrl);
    if(err < 0)
        printk("[LCD][DEBUG] %s : Cannot create the sysfs\n" , __func__);
    
    err = device_create_file(&pdev->dev, &dev_attr_aco_ctrl);
    if(err < 0)
        printk("[LCD][DEBUG] %s : Cannot create the sysfs\n" , __func__);

    err = device_create_file(&pdev->dev, &dev_attr_ce_ctrl);
    if(err < 0)
        printk("[LCD][DEBUG] %s : Cannot create the sysfs\n" , __func__);
#if defined(CONFIG_LGIT_COLOR_ENGINE_SWITCH)
    err = device_create_file(&pdev->dev, &dev_attr_color_engine_on_off);
    if(err < 0)
        printk("[LCD][DEBUG] %s : Cannot create the sysfs\n" , __func__);
#endif //CONFIG_LGIT_COLOR_ENGINE_SWITCH
#endif

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_lgit_lcd_probe,
	.driver = {
		.name   = "mipi_lgit",
	},
};

static struct msm_fb_panel_data lgit_panel_data = {
	.on		= mipi_lgit_lcd_on,
	.off		= mipi_lgit_lcd_off,
	.set_backlight = mipi_lgit_set_backlight_board,
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
		printk(KERN_ERR
		  "[LCD][DEBUG] %s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		printk(KERN_ERR
		  "[LCD][DEBUG] %s: platform_device_register failed!\n", __func__);
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
	mipi_dsi_buf_alloc(&lgit_camera_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&lgit_shutdown_tx_buf, DSI_BUF_SIZE);

	return platform_driver_register(&this_driver);
}

module_init(mipi_lgit_lcd_init);
