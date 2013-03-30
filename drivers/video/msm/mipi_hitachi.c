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

#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_hitachi.h"
#include <linux/gpio.h>

#if defined (CONFIG_LGE_BACKLIGHT_CABC) &&\
		defined (CONFIG_LGE_BACKLIGHT_CABC_DEBUG)
extern void set_hitachi_cabc(int i);
extern int get_hitachi_cabc(void);
#endif
static struct msm_panel_common_pdata *mipi_hitachi_pdata;

static struct dsi_buf hitachi_tx_buf;
static struct dsi_buf hitachi_rx_buf;

struct msm_fb_data_type *local_mfd=NULL;

#ifdef CONFIG_LGE_ESD_CHECK
static char reg_adr[2] = {0x0C, 0x00};
int reg_size = 1;

static char macp_off[2] = {0xB0, 0x04};
static char macp_on[2] = {0xB0, 0x03};

static struct dsi_cmd_desc cmds_test =
	{DTYPE_GEN_READ1, 1, 0, 0, 1, sizeof(reg_adr), reg_adr};
static struct dsi_cmd_desc cmds_macp_off =
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(macp_off), macp_off};
static struct dsi_cmd_desc cmds_macp_on =
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(macp_on), macp_on};
#endif /* CONFIG_LGE_ESD_CHECK */

/* [LGE_CHANGE] Remove unnecessary codes.
 * Remove mutex_lock, mutex_unlock and mipi_dsi_op_mode_config
 * in mipi_hitachi_lcd_on and mipi_hitachi_lcd_off.
 * minjong.gong@lge.com, 2011-07-19
 */
int mipi_hitachi_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	int cnt = 0;

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	printk(KERN_INFO "%s: mipi hitachi lcd on started \n", __func__);
	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x10000000);//HS mode
	cnt = mipi_dsi_cmds_tx(&hitachi_tx_buf, mipi_hitachi_pdata->power_on_set_1,
			mipi_hitachi_pdata->power_on_set_size_1);
	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x14000000);//LP mode
	
	if(local_mfd==NULL)
		local_mfd = mfd;
	printk(KERN_INFO "%s: mipi hitachi lcd on end \n", __func__);

	if (cnt > 0)
		cnt = 0;

	return cnt;
}

int mipi_hitachi_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	int cnt = 0;

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	printk(KERN_INFO "%s: mipi hitachi lcd off started \n", __func__);
	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x10000000);//HS mode
	cnt = mipi_dsi_cmds_tx(&hitachi_tx_buf,
			mipi_hitachi_pdata->power_off_set_1,
			mipi_hitachi_pdata->power_off_set_size_1);
	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x14000000);//LP mode
	printk(KERN_INFO "%s: mipi hitachi lcd off end \n", __func__);

	if (cnt > 0)
		cnt = 0;

	return cnt;
}
int mipi_hitachi_lcd_off_for_shutdown(void)
{
	int cnt = 0;

	printk(KERN_INFO "%s: mipi hitachi lcd off started \n", __func__);
	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x10000000);//HS mode
	cnt = mipi_dsi_cmds_tx(&hitachi_tx_buf,
		mipi_hitachi_pdata->power_off_set_1,
		mipi_hitachi_pdata->power_off_set_size_1);
	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x14000000);//LP mode

	return 0;
}

static void mipi_hitachi_set_backlight_board(struct msm_fb_data_type *mfd)
{
	int level;

	level = (int)mfd->bl_level;
	mipi_hitachi_pdata->backlight_level(level, 0, 0);
}

#ifdef CONFIG_LGE_ESD_CHECK
ssize_t write_reg_adr(struct device *dev, struct device_attribute *attr,
						const char *buf, size_t count)
{
	unsigned int tmp;
	ssize_t ret = strnlen(buf, PAGE_SIZE);
	printk(KERN_INFO "%s  \n", __func__);
	sscanf(buf,"%x",&tmp);
	reg_adr[0] = (char) tmp;
	return ret;
}


ssize_t write_cmd(struct device *dev, struct device_attribute *attr,
						const char *buf, size_t count)
{
	ssize_t ret = strnlen(buf, PAGE_SIZE);
	unsigned int cmds;
	printk(KERN_INFO "%s  \n", __func__);
	sscanf(buf,"%x",&cmds);
	cmds_test.dtype =(char) cmds;
	return ret;
}

ssize_t write_size(struct device *dev, struct device_attribute *attr,
						const char *buf, size_t count)
{
	ssize_t ret = strnlen(buf, PAGE_SIZE);
	printk(KERN_INFO "%s  \n", __func__);
	sscanf(buf,"%d",&reg_size);
	return ret;
}

ssize_t read_reg(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint32 dsi_ctrl, ctrl;
	int video_mode;
	char *lp;
	int len, i, idx, str_len, r;

	printk(KERN_INFO "%s :cmd_types %x  reg_address %x,  cmd_size %d\n",
								__func__,
								cmds_test.dtype,
								cmds_test.payload[0],
								reg_size);
	if(local_mfd->panel_power_on){
		len = reg_size;
		if(len > 8){
			len -= 4;
			len >>= 3;	/* divided by 8 */
			if((reg_size-4-len*8) > 0)
					len++;
			len++;
			len <<= 3;
		}
		else
			len = 8;
		str_len = 0;
		idx = 0;

		lp = kzalloc(sizeof(int32)*7, GFP_USER);	/* maximum 28 parameters */
		if(lp == NULL){
			printk(KERN_ERR "fail alloc for buffer\n");
			return 0;
		}
		for(i=8; i<=len;i+=8){
			// turn cmd mode
			mutex_lock(&local_mfd->dma->ov_mutex);
			MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x10000000);
			if(cmds_test.dtype == DTYPE_GEN_READ1)
				mipi_dsi_cmds_tx(&hitachi_tx_buf, &cmds_macp_off,1);
			dsi_ctrl = MIPI_INP(MIPI_DSI_BASE + 0x0000);
			video_mode = dsi_ctrl & 0x02; /* VIDEO_MODE_EN */
			if (video_mode) {
				ctrl = dsi_ctrl | 0x04; /* CMD_MODE_EN */
				MIPI_OUTP(MIPI_DSI_BASE + 0x0000, ctrl);
			}

			if(cmds_test.dtype == DTYPE_DCS_READ)	/* DTYPE_DCS_READ */
			{
				cmds_test.wait = reg_size;
				mipi_dsi_cmds_rx(local_mfd, &hitachi_tx_buf,&hitachi_rx_buf,
					&cmds_test, reg_size);
			}
			else	/* DTYPE_GEN_READ */
				mipi_dsi_cmds_rx(local_mfd, &hitachi_tx_buf,&hitachi_rx_buf,
					&cmds_test, i);
			if (video_mode)
				MIPI_OUTP(MIPI_DSI_BASE + 0x0000, dsi_ctrl); /* restore */
			if(cmds_test.dtype == DTYPE_GEN_READ1)
				mipi_dsi_cmds_tx(&hitachi_tx_buf, &cmds_macp_on,1);
			MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x14000000);
			mutex_unlock(&local_mfd->dma->ov_mutex);
			if(reg_size <= 8)
				str_len = reg_size;
			else if(idx == 0)
				str_len = 4;
			else if(idx > reg_size)
				str_len = idx - reg_size;
			else
				str_len = 8;
			memcpy(lp+idx, hitachi_rx_buf.data, str_len);
			idx +=	str_len;
		}
		r= snprintf(buf, PAGE_SIZE, "0x%02X : %02X %02X %02X %02X %02X %02X %02X %02X\
			\n	%02X %02X %02X %02X %02X %02X %02X %02X\
			\n	%02X %02X %02X %02X %02X %02X %02X %02X\
			\n	%02X %02X %02X %02X\n",
			cmds_test.payload[0],
			lp[0], lp[1], lp[2], lp[3], lp[4], lp[5], lp[6], lp[7], lp[8], lp[9], lp[10],
			lp[11], lp[12], lp[13], lp[14], lp[15], lp[16], lp[17], lp[18], lp[19], lp[20],
			lp[21], lp[22], lp[23], lp[24], lp[25], lp[26], lp[27]);
		kfree(lp);
		return r;
	}
	return 0;
}

DEVICE_ATTR(show_reg_value, 0644, read_reg, write_reg_adr);
DEVICE_ATTR(write_cmd_type, 0644, NULL, write_cmd);
DEVICE_ATTR(write_cmd_size, 0644, NULL, write_size);
#endif /* CONFIG_LGE_ESD_CHECK */
static int mipi_hitachi_lcd_probe(struct platform_device *pdev)
{
	struct msm_fb_panel_data *pdata;
#ifdef CONFIG_LGE_ESD_CHECK
	int ret;
#endif /* CONFIG_LGE_ESD_CHECK */
	if (pdev->id == 0) {
		mipi_hitachi_pdata = pdev->dev.platform_data;
		return 0;
	}

	printk(KERN_INFO "%s: mipi hitachi lcd probe start\n", __func__);
	pdata = pdev->dev.platform_data;
	if (!pdata)
		return 0;

	//pdata->panel_info.bl_max = mipi_hitachi_pdata->max_backlight_level;

	msm_fb_add_device(pdev);

#ifdef CONFIG_LGE_ESD_CHECK
	ret = device_create_file(&pdev->dev, &dev_attr_show_reg_value);
	ret = device_create_file(&pdev->dev, &dev_attr_write_cmd_type);
	ret = device_create_file(&pdev->dev, &dev_attr_write_cmd_size);
	if (ret) {
		printk(KERN_ERR " %s : fail to create sysfs\n", __func__);
	}
#endif /* CONFIG_LGE_ESD_CHECK */
	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_hitachi_lcd_probe,
	.driver = {
		.name   = "mipi_hitachi",
	},
};

static struct msm_fb_panel_data hitachi_panel_data = {
	.on		= mipi_hitachi_lcd_on,
	.off		= mipi_hitachi_lcd_off,
	.set_backlight = mipi_hitachi_set_backlight_board,
};

static int ch_used[3];

#if defined (CONFIG_LGE_BACKLIGHT_CABC) &&\
		defined (CONFIG_LGE_BACKLIGHT_CABC_DEBUG)
static ssize_t hitachi_cabc_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	get_hitachi_cabc();
	return sprintf(buf, "%d\n", get_hitachi_cabc());
}


static ssize_t hitachi_cabc_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int cabc_index;

	if (!count)
		return -EINVAL;

	cabc_index = simple_strtoul(buf, NULL, 10);

	set_hitachi_cabc(cabc_index);

	return count;

}
DEVICE_ATTR(hitachi_cabc, 0664, hitachi_cabc_show, hitachi_cabc_store);
#endif


int mipi_hitachi_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_hitachi", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	hitachi_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &hitachi_panel_data,
		sizeof(hitachi_panel_data));
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}

#if defined (CONFIG_LGE_BACKLIGHT_CABC) &&\
		defined (CONFIG_LGE_BACKLIGHT_CABC_DEBUG)
	ret = device_create_file(&pdev->dev, &dev_attr_hitachi_cabc);

	if (ret) {
		printk(KERN_ERR
		  "%s: device_create_file failed!\n", __func__);
		goto err_device_put;
	}
#endif
	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}

static int __init mipi_hitachi_lcd_init(void)
{
	mipi_dsi_buf_alloc(&hitachi_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&hitachi_rx_buf, DSI_BUF_SIZE);

	return platform_driver_register(&this_driver);
}

module_init(mipi_hitachi_lcd_init);
