/* Copyright (c) 2009-2010, Code Aurora Forum. All rights reserved.
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


//FULL HD LCD (BSP_LCD_LCD Driver) ADD LCD Driver
#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_lgit_fhd.h"


static struct msm_panel_info pinfo;

#define DSI_BIT_CLK_900MHZ

static struct mipi_dsi_phy_ctrl dsi_video_mode_phy_db = {
	/* 768*1280, RGB888, 4 Lane 60 fps video mode */
#if	defined(DSI_BIT_CLK_900MHZ)

    /* regulator */
	{0x03, 0x0a, 0x04, 0x00, 0x20},
	/* timing */
	{0xEB, 0x35, 0x21, 0x00, 0x59, 0x63, 0x27, 0x39, 0x42, 0x03, 0x04},
    /* phy ctrl */
	{0x5f, 0x00, 0x00, 0x10},
    /* strength */
	{0xff, 0x00, 0x06, 0x00},
	/* pll control */
#if defined(CONFIG_MACH_APQ8064_GVDCM)
	{0x00, 0xC1, 0x01, 0x1A, 0x00, 0x50, 0x48, 0x63,
	 0x40, 0x07, 0x01, 0x00, 0x14, 0x03, 0x00, 0x02,
	 0x00, 0x20, 0x00, 0x01},
#elif defined(CONFIG_MACH_APQ8064_GKU) || defined(CONFIG_MACH_APQ8064_GKKT) || defined(CONFIG_MACH_APQ8064_GKSK)
	{0x00, 0xEA, 0x01, 0x1A, 0x00, 0x50, 0x48, 0x63,
	 0x40, 0x07, 0x01, 0x00, 0x14, 0x03, 0x00, 0x02,
	 0x00, 0x20, 0x00, 0x01},
#else
	{0x00, 0xB8, 0x01, 0x1A, 0x00, 0x50, 0x48, 0x63,
	 0x40, 0x07, 0x01, 0x00, 0x14, 0x03, 0x00, 0x02,
	 0x00, 0x20, 0x00, 0x01},
#endif
#endif
};

static int __init mipi_video_lgit_fhd_pt_init(void)
{
	int ret;

#ifdef CONFIG_FB_MSM_MIPI_PANEL_DETECT
	if (msm_fb_detect_client("mipi_video_lgit_fhd"))
		return 0;
#endif

	pinfo.xres = 1080;
	pinfo.yres = 1920;
	pinfo.type = MIPI_VIDEO_PANEL;
	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;
#if defined(CONFIG_MACH_APQ8064_GVDCM)
	pinfo.lcdc.h_back_porch = 100;
	pinfo.lcdc.h_front_porch = 110;
#elif defined(CONFIG_MACH_APQ8064_GKU) || defined(CONFIG_MACH_APQ8064_GKKT) || defined(CONFIG_MACH_APQ8064_GKSK)
	pinfo.lcdc.h_back_porch = 150;
	pinfo.lcdc.h_front_porch = 179;
#else
	pinfo.lcdc.h_back_porch = 85;     /* 106 */
	pinfo.lcdc.h_front_porch = 100;     /* 95 */
#endif
	pinfo.lcdc.h_pulse_width = 4;
	pinfo.lcdc.v_back_porch = 9;
	pinfo.lcdc.v_front_porch = 3;
	pinfo.lcdc.v_pulse_width = 1;
	pinfo.lcdc.border_clr = 0;         /* blk */
	pinfo.lcdc.underflow_clr = 0xff;   /* blue */
	pinfo.lcdc.hsync_skew = 0;
	pinfo.bl_max = 0xFF;
	pinfo.bl_min = 0;
	pinfo.fb_num = 2;

	pinfo.mipi.mode = DSI_VIDEO_MODE;
	pinfo.mipi.pulse_mode_hsa_he = FALSE;

/* jinho.jang 2011.03.22,  Modify code to apply IEF function */

	pinfo.mipi.hfp_power_stop = FALSE;//TRUE; // FALSE; //TRUE;
	pinfo.mipi.hbp_power_stop = FALSE;//TRUE; // FALSE; //TRUE;
	pinfo.mipi.hsa_power_stop = FALSE;//FALSE; //TRUE;

	pinfo.mipi.esc_byte_ratio = 6;
	pinfo.mipi.eof_bllp_power_stop = TRUE; //FALSE; //TRUE;
	pinfo.mipi.bllp_power_stop = TRUE;
	pinfo.mipi.traffic_mode = DSI_NON_BURST_SYNCH_EVENT;
 	pinfo.mipi.dst_format = DSI_VIDEO_DST_FORMAT_RGB888;
	pinfo.mipi.vc = 0;
	pinfo.mipi.rgb_swap = DSI_RGB_SWAP_RGB;
	pinfo.mipi.data_lane0 = TRUE;
	pinfo.mipi.data_lane1 = TRUE;
	pinfo.mipi.data_lane2 = TRUE;
	pinfo.mipi.data_lane3 = TRUE;
#if	defined(DSI_BIT_CLK_900MHZ)	
	pinfo.mipi.t_clk_post = 0x21;
	pinfo.mipi.t_clk_pre = 0x3C;
#if defined(CONFIG_MACH_APQ8064_GVDCM)
	pinfo.clk_rate = 900000000;
#elif defined(CONFIG_MACH_APQ8064_GKU) || defined(CONFIG_MACH_APQ8064_GKKT) || defined(CONFIG_MACH_APQ8064_GKSK)
	pinfo.clk_rate = 983000000;
#else
	pinfo.clk_rate = 884000000;
#endif
	pinfo.mipi.frame_rate = 60;	
#endif

	pinfo.mipi.stream = 0;		/* dma_p */
	pinfo.mipi.mdp_trigger = 0;//DSI_CMD_TRIGGER_SW;
	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.dsi_phy_db = &dsi_video_mode_phy_db;
	ret = mipi_lgit_device_register(&pinfo, MIPI_DSI_PRIM,
						MIPI_DSI_PANEL_FHD);
	if (ret)
		printk(KERN_ERR "%s: failed to register device!\n", __func__);

	return ret;
}

module_init(mipi_video_lgit_fhd_pt_init);

