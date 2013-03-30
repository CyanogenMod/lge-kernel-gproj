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

#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_hitachi.h"

static struct msm_panel_info pinfo;

#define DSI_BIT_CLK_470MHZ


static struct mipi_dsi_phy_ctrl dsi_video_mode_phy_db = {
	/* 720*1280, RGB888, 4 Lane 60 fps video mode */
#if defined(DSI_BIT_CLK_470MHZ)
    /* regulator */
	{0x03, 0x0a, 0x04, 0x00, 0x20},	/* Fixed values */
	/* timing */
	{0x66, 0x26, 0x1E, 0x00, 0x21, 0x96, 0x1E, 0x8F,
	0x21, 0x03, 0x04, 0xa0},
    /* phy ctrl */
	{0x5f, 0x00, 0x00, 0x10},	/* Fixed values */
    /* strength */
	{0xff, 0x00, 0x06, 0x00},	/* Fixed values */
	/* pll control */
	{0x00, 0xD5, 0x01, 0x1a, 0x00, 0x50, 0x48, 0x63,
	0x41, 0x0f, 0x03, 0x00, 0x14, 0x03, 0x00, 0x02,
	0x00, 0x20, 0x00, 0x01 },
#elif defined(DSI_BIT_CLK_482MHZ)
    /* regulator */
	{0x03, 0x0a, 0x04, 0x00, 0x20},	/* Fixed values */
	/* timing */
	{0x66, 0x26, 0x1F, 0x00, 0x22, 0x97, 0x1E, 0x8F,
	0x22, 0x03, 0x04},
    /* phy ctrl */
	{0x5f, 0x00, 0x00, 0x10},	/* Fixed values */
    /* strength */
	{0xff, 0x00, 0x06, 0x00},	/* Fixed values */
	/* pll control */
	{0x00, 0xe1, 0x01, 0x1a, 0x00, 0x50, 0x48, 0x63,
	0x41, 0x0f, 0x03, 0x00, 0x14, 0x03, 0x00, 0x02,
	0x00, 0x20, 0x00, 0x01 },
#endif


};

static int __init mipi_video_hitachi_hd_pt_init(void)
{
	int ret;

#ifdef CONFIG_FB_MSM_MIPI_PANEL_DETECT
	if (msm_fb_detect_client("mipi_video_hitachi_hd"))
		return 0;
#endif

	pinfo.xres = 720;
	pinfo.yres = 1280;
	pinfo.type = MIPI_VIDEO_PANEL;
	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;

	/* QCT Limitation :
	 * All proch values must be a multiple of 4. 2011.01.20 */
#if defined(DSI_BIT_CLK_470MHZ)
	pinfo.lcdc.h_back_porch = 189;
	pinfo.lcdc.h_front_porch = 100;
	pinfo.lcdc.h_pulse_width = 4;
	pinfo.lcdc.v_back_porch = 4;
	pinfo.lcdc.v_front_porch = 4;
	pinfo.lcdc.v_pulse_width = 1;
#elif defined(DSI_BIT_CLK_482MHZ)
	pinfo.lcdc.h_back_porch = 192;
	pinfo.lcdc.h_front_porch = 108;
	pinfo.lcdc.h_pulse_width = 4;
	pinfo.lcdc.v_back_porch = 4;
	pinfo.lcdc.v_front_porch = 23;
	pinfo.lcdc.v_pulse_width = 1;
#endif



	pinfo.lcdc.border_clr = 0;	/* blk */
	pinfo.lcdc.underflow_clr = 0;   /* black */
	pinfo.lcdc.hsync_skew = 0;
	pinfo.bl_max = 0xFF;
	pinfo.bl_min = 0;
	pinfo.fb_num = 2;

	pinfo.mipi.mode = DSI_VIDEO_MODE;
	pinfo.mipi.pulse_mode_hsa_he = FALSE;
	pinfo.mipi.hfp_power_stop = FALSE;//TRUE;
	pinfo.mipi.hbp_power_stop = FALSE;//TRUE;
	pinfo.mipi.hsa_power_stop = FALSE;//TRUE;
	pinfo.mipi.eof_bllp_power_stop = TRUE;
	pinfo.mipi.bllp_power_stop = TRUE;
	pinfo.mipi.traffic_mode = DSI_NON_BURST_SYNCH_PULSE;
	pinfo.mipi.dst_format = DSI_VIDEO_DST_FORMAT_RGB888;
	pinfo.mipi.vc = 0;
	pinfo.mipi.rgb_swap = DSI_RGB_SWAP_RGB;
	pinfo.mipi.data_lane0 = TRUE;
	pinfo.mipi.data_lane1 = TRUE;
	pinfo.mipi.data_lane2 = TRUE;
	pinfo.mipi.data_lane3 = TRUE;
#if defined(DSI_BIT_CLK_470MHZ)
	pinfo.mipi.t_clk_post = 0x22;
	pinfo.mipi.t_clk_pre = 0x36;
	pinfo.clk_rate = 470000000;
	pinfo.mipi.frame_rate = 60;
#elif defined(DSI_BIT_CLK_482MHZ)
	pinfo.mipi.t_clk_post = 0x22;
	pinfo.mipi.t_clk_pre = 0x36;
	pinfo.clk_rate = 482180000;
	pinfo.mipi.frame_rate = 60;
#endif

        pinfo.mipi.esc_byte_ratio = 5; /* if bllp_power_stop is true, esc_clk enabling is needed. */
	pinfo.mipi.stream = 0; /* dma_p */
	pinfo.mipi.mdp_trigger = 0;/* DSI_CMD_TRIGGER_SW; */
	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.dsi_phy_db = &dsi_video_mode_phy_db;
	ret = mipi_hitachi_device_register(&pinfo, MIPI_DSI_PRIM,
						MIPI_DSI_PANEL_720P_PT);
	if (ret)
		printk(KERN_ERR "%s: failed to register device!\n", __func__);

	return ret;
}

module_init(mipi_video_hitachi_hd_pt_init);
