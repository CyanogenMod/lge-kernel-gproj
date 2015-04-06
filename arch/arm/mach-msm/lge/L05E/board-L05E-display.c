/* Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
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
 */

#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/bootmem.h>
#include <linux/msm_ion.h>
#include <asm/mach-types.h>
#include <mach/msm_memtypes.h>
#include <mach/board.h>
#include <mach/gpio.h>
#include <mach/gpiomux.h>
#include <mach/ion.h>
#include <mach/msm_bus_board.h>
#include <mach/socinfo.h>

#if defined(CONFIG_MACH_LGE)
#include "devices.h"
#include "board-L05E.h"
#else
#include "board-8064.h"
#endif

#if defined(CONFIG_MACH_LGE)
#include "../../../../drivers/video/msm/msm_fb.h"
#include "../../../../drivers/video/msm/msm_fb_def.h"
#include "../../../../drivers/video/msm/mipi_dsi.h"

#include <mach/board_lge.h>

#include <linux/i2c.h>
#include <linux/kernel.h>

#ifndef LGE_DSDR_KERNEL_SUPPORT
#define LGE_DSDR_KERNEL_SUPPORT
#endif

#ifdef CONFIG_LGE_KCAL
#ifdef CONFIG_LGE_QC_LCDC_LUT
extern int set_qlut_kcal_values(int kcal_r, int kcal_g, int kcal_b);
extern int refresh_qlut_display(void);
#else
#error only kcal by Qucalcomm LUT is supported now!!!
#endif //                      
#endif //               
#endif //                            

#ifdef CONFIG_FB_MSM_TRIPLE_BUFFER
/* prim = 1366 x 768 x 3(bpp) x 3(pages) */
#if defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_HD_PT)
#define MSM_FB_PRIM_BUF_SIZE roundup(736 * 1280 * 4 * 3, 0x10000)
#elif defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_FHD_PT) ||\
	defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_FHD_INVERSE_PT)
#define MSM_FB_PRIM_BUF_SIZE roundup(1088 * 1920 * 4 * 3, 0x10000)
#elif defined(CONFIG_FB_MSM_MIPI_HITACHI_VIDEO_HD_PT)
#define MSM_FB_PRIM_BUF_SIZE roundup(736 * 1280 * 4 * 3, 0x10000)
#else //Qualcomm original
#define MSM_FB_PRIM_BUF_SIZE roundup(1920 * 1088 * 4 * 3, 0x10000)
#endif
#else
/* prim = 1366 x 768 x 3(bpp) x 2(pages) */
#if defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_HD_PT)
#define MSM_FB_PRIM_BUF_SIZE roundup(736 * 1280 * 4 * 2, 0x10000)
#elif defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_FHD_PT) ||\
	defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_FHD_INVERSE_PT)
#define MSM_FB_PRIM_BUF_SIZE roundup(1088 * 1920 * 4 * 2, 0x10000)
#elif definded(CONFIG_FB_MSM_MIPI_HITACHI_VIDEO_HD_PT)
#define MSM_FB_PRIM_BUF_SIZE roundup(736 * 1280 * 4 * 2, 0x10000)
#else //Qualcomm original
#define MSM_FB_PRIM_BUF_SIZE roundup(1920 * 1088 * 4 * 2, 0x10000)
#endif
#endif /*CONFIG_FB_MSM_TRIPLE_BUFFER */

#if defined(CONFIG_MACH_LGE)
#ifdef LGE_DSDR_KERNEL_SUPPORT
#define MSM_FB_EXT_BUF_SIZE \
        (roundup((1920 * 1088 * 4), 4096) * 3) /* 4 bpp x 3 page */
#else  /*                         */
#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
#define MSM_FB_EXT_BUF_SIZE \
		(roundup((1920 * 1088 * 2), 4096) * 1) /* 2 bpp x 1 page */
#elif defined(CONFIG_FB_MSM_TVOUT)
#define MSM_FB_EXT_BUF_SIZE \
		(roundup((720 * 576 * 2), 4096) * 2) /* 2 bpp x 2 pages */
#else
#define MSM_FB_EXT_BUF_SIZE	0
#endif /* CONFIG_FB_MSM_HDMI_MSM_PANEL */
#endif /*                         */
#endif //                            

#if defined(CONFIG_MACH_LGE)
#ifdef CONFIG_FB_MSM_WRITEBACK_MSM_PANEL
#if defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_FHD_PT) ||\
	defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_FHD_INVERSE_PT)
#define MSM_FB_WFD_BUF_SIZE \
		(roundup((1920 * 1088 * 2), 4096) * 3) /* 2 bpp x 3 page */
#else
#define MSM_FB_WFD_BUF_SIZE \
		(roundup((1280 * 736 * 2), 4096) * 3) /* 2 bpp x 3 page */
#endif
#else
#define MSM_FB_WFD_BUF_SIZE     0
#endif //CONFIG_FB_MSM_WRITEBACK_MSM_PANEL
#endif //                            

#if defined(CONFIG_MACH_LGE)
#define MSM_FB_SIZE \
	roundup(MSM_FB_PRIM_BUF_SIZE + \
		MSM_FB_EXT_BUF_SIZE + MSM_FB_WFD_BUF_SIZE, 4096)
#else //Qualcomm original
#define MSM_FB_SIZE roundup(MSM_FB_PRIM_BUF_SIZE, 4096)
#endif //                            

#ifdef CONFIG_FB_MSM_OVERLAY0_WRITEBACK
#if defined(CONFIG_MACH_LGE)
	#if defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_HD_PT)
	#define MSM_FB_OVERLAY0_WRITEBACK_SIZE roundup((736 * 1280 * 3 * 2), 4096)
	#elif defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_FHD_PT) ||\
	defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_FHD_INVERSE_PT)
	#define MSM_FB_OVERLAY0_WRITEBACK_SIZE roundup((1088 * 1920 * 3 * 2), 4096)
	#elif defined(CONFIG_FB_MSM_MIPI_HITACHI_VIDEO_HD_PT)
	#define MSM_FB_OVERLAY0_WRITEBACK_SIZE roundup((736 * 1280 * 3 * 2), 4096)
	#else
	#define MSM_FB_OVERLAY0_WRITEBACK_SIZE (0)
	#endif
#else //Qualcomm original
#define MSM_FB_OVERLAY0_WRITEBACK_SIZE roundup((1376 * 768 * 3 * 2), 4096)
#endif //                            
#else
#define MSM_FB_OVERLAY0_WRITEBACK_SIZE (0)
#endif  /* CONFIG_FB_MSM_OVERLAY0_WRITEBACK */

#ifdef CONFIG_FB_MSM_OVERLAY1_WRITEBACK
#define MSM_FB_OVERLAY1_WRITEBACK_SIZE roundup((1920 * 1088 * 3 * 2), 4096)
#else
#define MSM_FB_OVERLAY1_WRITEBACK_SIZE (0)
#endif  /* CONFIG_FB_MSM_OVERLAY1_WRITEBACK */

#if defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_HD_PT)
#define LGIT_IEF
#endif
//                                                                             
#define LGIT_IEF_SWITCH
//                                        

static struct resource msm_fb_resources[] = {
	{
		.flags = IORESOURCE_DMA,
	}
};

#define LVDS_CHIMEI_PANEL_NAME "lvds_chimei_wxga"
#define LVDS_FRC_PANEL_NAME "lvds_frc_fhd"
#define MIPI_VIDEO_TOSHIBA_WSVGA_PANEL_NAME "mipi_video_toshiba_wsvga"
#define MIPI_VIDEO_CHIMEI_WXGA_PANEL_NAME "mipi_video_chimei_wxga"
#define HDMI_PANEL_NAME "hdmi_msm"
#define MHL_PANEL_NAME "hdmi_msm,mhl_8334"
#define TVOUT_PANEL_NAME "tvout_msm"

#define LVDS_PIXEL_MAP_PATTERN_1	1
#define LVDS_PIXEL_MAP_PATTERN_2	2
#ifndef CONFIG_MACH_LGE
#ifdef CONFIG_FB_MSM_HDMI_AS_PRIMARY
static unsigned char hdmi_is_primary = 1;
#else
static unsigned char hdmi_is_primary;
#endif /* CONFIG_FB_MSM_HDMI_AS_PRIMARY */

static unsigned char mhl_display_enabled;

unsigned char apq8064_hdmi_as_primary_selected(void)
{
	return hdmi_is_primary;
}

unsigned char apq8064_mhl_display_enabled(void)
{
        return mhl_display_enabled;
}

static void set_mdp_clocks_for_wuxga(void);
#endif /*                 */

static int msm_fb_detect_panel(const char *name)
{
#ifndef CONFIG_MACH_LGE
	u32 version;
	if (machine_is_apq8064_liquid()) {
		version = socinfo_get_platform_version();
		if ((SOCINFO_VERSION_MAJOR(version) == 1) &&
			(SOCINFO_VERSION_MINOR(version) == 1)) {
			if (!strncmp(name, MIPI_VIDEO_CHIMEI_WXGA_PANEL_NAME,
				strnlen(MIPI_VIDEO_CHIMEI_WXGA_PANEL_NAME,
					PANEL_NAME_MAX_LEN)))
				return 0;
		} else {
			if (!strncmp(name, LVDS_CHIMEI_PANEL_NAME,
				strnlen(LVDS_CHIMEI_PANEL_NAME,
					PANEL_NAME_MAX_LEN)))
				return 0;
		}
	} else if (machine_is_apq8064_mtp()) {
		if (!strncmp(name, MIPI_VIDEO_TOSHIBA_WSVGA_PANEL_NAME,
			strnlen(MIPI_VIDEO_TOSHIBA_WSVGA_PANEL_NAME,
				PANEL_NAME_MAX_LEN)))
			return 0;
	} else if (machine_is_apq8064_cdp()) {
		if (!strncmp(name, LVDS_CHIMEI_PANEL_NAME,
			strnlen(LVDS_CHIMEI_PANEL_NAME,
				PANEL_NAME_MAX_LEN)))
			return 0;
	} else if (machine_is_mpq8064_dtv()) {
		if (!strncmp(name, LVDS_FRC_PANEL_NAME,
			strnlen(LVDS_FRC_PANEL_NAME,
			PANEL_NAME_MAX_LEN))) {
			set_mdp_clocks_for_wuxga();
			return 0;
		}
	}

	if (!strncmp(name, HDMI_PANEL_NAME,
			strnlen(HDMI_PANEL_NAME,
				PANEL_NAME_MAX_LEN))) {
		if (apq8064_hdmi_as_primary_selected())
			set_mdp_clocks_for_wuxga();
		return 0;
	}
	return -ENODEV;

#else
	return 0;
#endif /*                 */
}

static struct msm_fb_platform_data msm_fb_pdata = {
	.detect_client = msm_fb_detect_panel,
};

static struct platform_device msm_fb_device = {
	.name              = "msm_fb",
	.id                = 0,
	.num_resources     = ARRAY_SIZE(msm_fb_resources),
	.resource          = msm_fb_resources,
	.dev.platform_data = &msm_fb_pdata,
};

void __init apq8064_allocate_fb_region(void)
{
	void *addr;
	unsigned long size;

	size = MSM_FB_SIZE;
	addr = alloc_bootmem_align(size, 0x1000);
	msm_fb_resources[0].start = __pa(addr);
	msm_fb_resources[0].end = msm_fb_resources[0].start + size - 1;
	pr_info("allocating %lu bytes at %p (%lx physical) for fb\n",
			size, addr, __pa(addr));
}

#define MDP_VSYNC_GPIO 0

static struct msm_bus_vectors mdp_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
};

static struct msm_bus_vectors mdp_ui_vectors[] = {
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 216000000 * 2,
		.ib = 270000000 * 2,
	},
};

static struct msm_bus_vectors mdp_vga_vectors[] = {
	/* VGA and less video */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 216000000 * 2,
		.ib = 270000000 * 2,
	},
};

static struct msm_bus_vectors mdp_720p_vectors[] = {
	/* 720p and less video */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 230400000 * 2,
		.ib = 288000000 * 2,
	},
};

static struct msm_bus_vectors mdp_1080p_vectors[] = {
	/* 1080p and less video */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 334080000 * 2,
		.ib = 417600000 * 2,
	},
};

static struct msm_bus_paths mdp_bus_scale_usecases[] = {
	{
		ARRAY_SIZE(mdp_init_vectors),
		mdp_init_vectors,
	},
	{
		ARRAY_SIZE(mdp_ui_vectors),
		mdp_ui_vectors,
	},
	{
		ARRAY_SIZE(mdp_ui_vectors),
		mdp_ui_vectors,
	},
	{
		ARRAY_SIZE(mdp_vga_vectors),
		mdp_vga_vectors,
	},
	{
		ARRAY_SIZE(mdp_720p_vectors),
		mdp_720p_vectors,
	},
	{
		ARRAY_SIZE(mdp_1080p_vectors),
		mdp_1080p_vectors,
	},
};

static struct msm_bus_scale_pdata mdp_bus_scale_pdata = {
	mdp_bus_scale_usecases,
	ARRAY_SIZE(mdp_bus_scale_usecases),
	.name = "mdp",
};

static struct msm_panel_common_pdata mdp_pdata = {
	.gpio = MDP_VSYNC_GPIO,
	.mdp_max_clk = 266667000,
	.mdp_max_bw = 2000000000,
	.mdp_bw_ab_factor = 200,
	.mdp_bw_ib_factor = 210,
	.mdp_bus_scale_table = &mdp_bus_scale_pdata,
	.mdp_rev = MDP_REV_44,
#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
	.mem_hid = BIT(ION_CP_MM_HEAP_ID),
#else
	.mem_hid = MEMTYPE_EBI1,
#endif
	.cont_splash_enabled = 0x01,
	.mdp_iommu_split_domain = 1,
};

void __init apq8064_mdp_writeback(struct memtype_reserve* reserve_table)
{
	mdp_pdata.ov0_wb_size = MSM_FB_OVERLAY0_WRITEBACK_SIZE;
	mdp_pdata.ov1_wb_size = MSM_FB_OVERLAY1_WRITEBACK_SIZE;
#if defined(CONFIG_ANDROID_PMEM) && !defined(CONFIG_MSM_MULTIMEDIA_USE_ION)
	reserve_table[mdp_pdata.mem_hid].size +=
		mdp_pdata.ov0_wb_size;
	reserve_table[mdp_pdata.mem_hid].size +=
		mdp_pdata.ov1_wb_size;

	pr_info("mem_map: mdp reserved with size 0x%lx in pool\n",
			mdp_pdata.ov0_wb_size + mdp_pdata.ov1_wb_size);
#endif
}

#ifdef CONFIG_LGE_KCAL
extern int set_kcal_values(int kcal_r, int kcal_g, int kcal_b);
extern int refresh_kcal_display(void);
extern int get_kcal_values(int *kcal_r, int *kcal_g, int *kcal_b);

static struct kcal_platform_data kcal_pdata = {
	.set_values = set_kcal_values,
	.get_values = get_kcal_values,
	.refresh_display = refresh_kcal_display
};

static struct platform_device kcal_platrom_device = {
	.name   = "kcal_ctrl",
	.dev = {
		.platform_data = &kcal_pdata,
	}
};
#endif /*                 */

static struct resource hdmi_msm_resources[] = {
	{
		.name  = "hdmi_msm_qfprom_addr",
		.start = 0x00700000,
		.end   = 0x007060FF,
		.flags = IORESOURCE_MEM,
	},
	{
		.name  = "hdmi_msm_hdmi_addr",
		.start = 0x04A00000,
		.end   = 0x04A00FFF,
		.flags = IORESOURCE_MEM,
	},
	{
		.name  = "hdmi_msm_irq",
		.start = HDMI_IRQ,
		.end   = HDMI_IRQ,
		.flags = IORESOURCE_IRQ,
	},
};

static int hdmi_enable_5v(int on);
static int hdmi_core_power(int on, int show);
static int hdmi_cec_power(int on);
static int hdmi_gpio_config(int on);
static int hdmi_panel_power(int on);

static struct msm_hdmi_platform_data hdmi_msm_data = {
	.irq = HDMI_IRQ,
	.enable_5v = hdmi_enable_5v,
	.core_power = hdmi_core_power,
	.cec_power = hdmi_cec_power,
	.panel_power = hdmi_panel_power,
	.gpio_config = hdmi_gpio_config,
};

static struct platform_device hdmi_msm_device = {
	.name = "hdmi_msm",
	.id = 0,
	.num_resources = ARRAY_SIZE(hdmi_msm_resources),
	.resource = hdmi_msm_resources,
	.dev.platform_data = &hdmi_msm_data,
};

static char wfd_check_mdp_iommu_split_domain(void)
{
	return mdp_pdata.mdp_iommu_split_domain;
}

#ifdef CONFIG_FB_MSM_WRITEBACK_MSM_PANEL
static struct msm_wfd_platform_data wfd_pdata = {
	.wfd_check_mdp_iommu_split = wfd_check_mdp_iommu_split_domain,
};

static struct platform_device wfd_panel_device = {
	.name = "wfd_panel",
	.id = 0,
	.dev.platform_data = NULL,
};

static struct platform_device wfd_device = {
	.name          = "msm_wfd",
	.id            = -1,
	.dev.platform_data = &wfd_pdata,
};
#endif

/* HDMI related GPIOs */
#define HDMI_CEC_VAR_GPIO	69
#define HDMI_DDC_CLK_GPIO	70
#define HDMI_DDC_DATA_GPIO	71
#define HDMI_HPD_GPIO		72


#if defined(CONFIG_MACH_LGE)
static bool dsi_power_on = false;
static int mipi_dsi_panel_power(int on)
{
	static struct regulator *reg_l8, *reg_l2, *reg_lvs6;
	static int gpio42;
	int rc;

	struct pm_gpio gpio42_param = {
		.direction = PM_GPIO_DIR_OUT,
		.output_buffer = PM_GPIO_OUT_BUF_CMOS,
		.output_value = 0,
		.pull = PM_GPIO_PULL_NO,
		.vin_sel = 2,
		.out_strength = PM_GPIO_STRENGTH_HIGH,
		.function = PM_GPIO_FUNC_PAIRED,
		.inv_int_pol = 0,
		.disable_pin = 0,
	};

	pr_debug("%s: onoff = %d\n", __func__, on);

	if (!dsi_power_on) {
#if defined(CONFIG_FB_MSM_MIPI_HITACHI_VIDEO_HD_PT)
		gpio42 = PM8921_GPIO_PM_TO_SYS(42);

		rc = gpio_request(gpio42, "disp_rst_n");
		if (rc) {
			pr_err("request gpio 42 failed, rc=%d\n", rc);
			return -ENODEV;
		}
#else
		/* RESET PIN(gpio42) Initial */
		gpio42 = PM8921_GPIO_PM_TO_SYS(42);

		rc = gpio_request(gpio42, "disp_rst_n");
		if (rc) {
			pr_err("request gpio 42 failed, rc=%d\n", rc);
			return -ENODEV;
		}
#endif
		/* L8:VCC */
		reg_l8 = regulator_get(&msm_mipi_dsi1_device.dev, "dsi_vci");
		if (IS_ERR(reg_l8)) {
			pr_err("could not get 8921_l8, rc = %ld\n",
				PTR_ERR(reg_l8));
			return -ENODEV;
		}
		/* LVS6:IOVCC */
		reg_lvs6 = regulator_get(&msm_mipi_dsi1_device.dev, "dsi_iovcc");
		if (IS_ERR(reg_lvs6)) {
			pr_err("could not get 8921_lvs6, rc = %ld\n",
				 PTR_ERR(reg_lvs6));
			return -ENODEV;
		}
		/* L2:VCI */
		reg_l2 = regulator_get(&msm_mipi_dsi1_device.dev, "dsi_vdda");
		if (IS_ERR(reg_l2)) {
			pr_err("could not get 8921_l2, rc = %ld\n",
				PTR_ERR(reg_l2));
			return -ENODEV;
		}

		rc = regulator_set_voltage(reg_l8, 3000000, 3000000);
		if (rc) {
			pr_err("set_voltage l8 failed, rc=%d\n", rc);
			return -EINVAL;
		}

		rc = regulator_set_voltage(reg_l2, 1200000, 1200000);
		if (rc) {
			pr_err("set_voltage l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		dsi_power_on = true;

	}
	if (on) {

		rc = regulator_set_optimum_mode(reg_l8, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l8 failed, rc=%d\n", rc);
			return -EINVAL;
		}

		rc = regulator_set_optimum_mode(reg_l2, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}

#if defined(CONFIG_FB_MSM_MIPI_HITACHI_VIDEO_HD_PT)
		rc = regulator_enable(reg_lvs6); // IOVCC
		if (rc) {
			pr_err("enable lvs6 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		udelay(100);

		rc = regulator_enable(reg_l8);	// dsi_vci
			if (rc) {
				pr_err("enable l8 failed, rc=%d\n", rc);
				return -ENODEV;
			}

		udelay(100);

		rc = regulator_enable(reg_l2);	// DSI
		if (rc) {
			pr_err("enable l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}
#elif defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_HD_PT)
		rc = regulator_enable(reg_l8);  // dsi_vci
		if (rc) {
			pr_err("enable l8 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		udelay(100);

		rc = regulator_enable(reg_lvs6); // IOVCC
		if (rc) {
			pr_err("enable lvs6 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		mdelay(20);

#endif
		rc = regulator_enable(reg_l2);  // DSI
		if (rc) {
			pr_err("enable l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		printk(KERN_INFO " %s : reset start.", __func__);
#if defined(CONFIG_FB_MSM_MIPI_HITACHI_VIDEO_HD_PT)
		mdelay(2);
		gpio42_param.output_value = 1;
		rc = pm8xxx_gpio_config(gpio42,&gpio42_param);
		if (rc) {
			pr_err("gpio_config 42 failed (3), rc=%d\n", rc);
			return -EINVAL;
		}
		mdelay(11);
#else
		/* LCD RESET HIGH */
		mdelay(2);
		gpio42_param.output_value = 1;
		rc = pm8xxx_gpio_config(gpio42,&gpio42_param);
				if (rc) {
			pr_err("gpio_config 42 failed (3), rc=%d\n", rc);
			return -EINVAL;
		}
		mdelay(5);
#endif

	} else {

#if defined(CONFIG_FB_MSM_MIPI_HITACHI_VIDEO_HD_PT)
		// DGMS : MC-C05717-2
		// LCD RESET LOW
		gpio42_param.output_value = 0;
		rc = pm8xxx_gpio_config(gpio42,&gpio42_param);
		if (rc) {
			pr_err("gpio_config 42 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		udelay(100);
#else
		// DGMS : MC-C05717-2
		// LCD RESET LOW
		gpio42_param.output_value = 0;
		rc = pm8xxx_gpio_config(gpio42,&gpio42_param);
		if (rc) {
			pr_err("gpio_config 42 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		udelay(100);
#endif

#if defined(CONFIG_FB_MSM_MIPI_HITACHI_VIDEO_HD_PT)
		rc = regulator_disable(reg_l8);		//VCI
		if (rc) {
			pr_err("disable reg_l8  failed, rc=%d\n", rc);
			return -ENODEV;
		}

		rc = regulator_disable(reg_lvs6);	// IOVCC
		if (rc) {
			pr_err("disable lvs6 failed, rc=%d\n", rc);
			return -ENODEV;
		}
#elif defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_HD_PT)
		rc = regulator_disable(reg_lvs6);	// IOVCC
		if (rc) {
			pr_err("disable lvs6 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		udelay(100);

		rc = regulator_disable(reg_l8);		//VCI
		if (rc) {
			pr_err("disable reg_l8  failed, rc=%d\n", rc);
			return -ENODEV;
		}
		udelay(100);
#endif
		rc = regulator_disable(reg_l2);		//DSI
		if (rc) {
			pr_err("disable reg_l2  failed, rc=%d\n", rc);
			return -ENODEV;
		}

		rc = regulator_set_optimum_mode(reg_l8, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l8 failed, rc=%d\n", rc);
			return -EINVAL;
		}

		rc = regulator_set_optimum_mode(reg_l2, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}
	}

	return 0;
}
#else //Qualcomm original
static bool dsi_power_on;
static int mipi_dsi_panel_power(int on)
{
	static struct regulator *reg_lvs7, *reg_l2, *reg_l11, *reg_ext_3p3v;
	static int gpio36, gpio25, gpio26, mpp3;
	int rc;

	pr_debug("%s: on=%d\n", __func__, on);

	if (!dsi_power_on) {
		reg_lvs7 = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi1_vddio");
		if (IS_ERR_OR_NULL(reg_lvs7)) {
			pr_err("could not get 8921_lvs7, rc = %ld\n",
				PTR_ERR(reg_lvs7));
			return -ENODEV;
		}

		reg_l2 = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi1_pll_vdda");
		if (IS_ERR_OR_NULL(reg_l2)) {
			pr_err("could not get 8921_l2, rc = %ld\n",
				PTR_ERR(reg_l2));
			return -ENODEV;
		}

		rc = regulator_set_voltage(reg_l2, 1200000, 1200000);
		if (rc) {
			pr_err("set_voltage l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		reg_l11 = regulator_get(&msm_mipi_dsi1_device.dev,
						"dsi1_avdd");
		if (IS_ERR(reg_l11)) {
				pr_err("could not get 8921_l11, rc = %ld\n",
						PTR_ERR(reg_l11));
				return -ENODEV;
		}
		rc = regulator_set_voltage(reg_l11, 3000000, 3000000);
		if (rc) {
				pr_err("set_voltage l11 failed, rc=%d\n", rc);
				return -EINVAL;
		}

		if (machine_is_apq8064_liquid()) {
			reg_ext_3p3v = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi1_vccs_3p3v");
			if (IS_ERR_OR_NULL(reg_ext_3p3v)) {
				pr_err("could not get reg_ext_3p3v, rc = %ld\n",
					PTR_ERR(reg_ext_3p3v));
				reg_ext_3p3v = NULL;
				return -ENODEV;
			}
			mpp3 = PM8921_MPP_PM_TO_SYS(3);
			rc = gpio_request(mpp3, "backlight_en");
			if (rc) {
				pr_err("request mpp3 failed, rc=%d\n", rc);
				return -ENODEV;
			}
		}

		gpio25 = PM8921_GPIO_PM_TO_SYS(25);
		rc = gpio_request(gpio25, "disp_rst_n");
		if (rc) {
			pr_err("request gpio 25 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		gpio26 = PM8921_GPIO_PM_TO_SYS(26);
		rc = gpio_request(gpio26, "pwm_backlight_ctrl");
		if (rc) {
			pr_err("request gpio 26 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		gpio36 = PM8921_GPIO_PM_TO_SYS(36); /* lcd1_pwr_en_n */
		rc = gpio_request(gpio36, "lcd1_pwr_en_n");
		if (rc) {
			pr_err("request gpio 36 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		dsi_power_on = true;
	}

	if (on) {
		rc = regulator_enable(reg_lvs7);
		if (rc) {
			pr_err("enable lvs7 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		rc = regulator_set_optimum_mode(reg_l11, 110000);
		if (rc < 0) {
			pr_err("set_optimum_mode l11 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_enable(reg_l11);
		if (rc) {
			pr_err("enable l11 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		rc = regulator_set_optimum_mode(reg_l2, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_enable(reg_l2);
		if (rc) {
			pr_err("enable l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		if (machine_is_apq8064_liquid()) {
			rc = regulator_enable(reg_ext_3p3v);
			if (rc) {
				pr_err("enable reg_ext_3p3v failed, rc=%d\n",
					rc);
				return -ENODEV;
			}
			gpio_set_value_cansleep(mpp3, 1);
		}

		gpio_set_value_cansleep(gpio36, 0);
		gpio_set_value_cansleep(gpio25, 1);
	} else {
		gpio_set_value_cansleep(gpio25, 0);
		gpio_set_value_cansleep(gpio36, 1);

		if (machine_is_apq8064_liquid()) {
			gpio_set_value_cansleep(mpp3, 0);

			rc = regulator_disable(reg_ext_3p3v);
			if (rc) {
				pr_err("disable reg_ext_3p3v failed, rc=%d\n",
					rc);
				return -ENODEV;
			}
		}

		rc = regulator_disable(reg_l11);
		if (rc) {
			pr_err("disable reg_l1 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		rc = regulator_disable(reg_lvs7);
		if (rc) {
			pr_err("disable reg_lvs7 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		rc = regulator_disable(reg_l2);
		if (rc) {
			pr_err("disable reg_l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}
	}

	return 0;
}
#endif //                            

static char mipi_dsi_splash_is_enabled(void)
{
       return mdp_pdata.cont_splash_enabled;
}

static struct mipi_dsi_platform_data mipi_dsi_pdata = {
	.dsi_power_save = mipi_dsi_panel_power,
	.splash_is_enabled = mipi_dsi_splash_is_enabled,
};

#if !defined(CONFIG_MACH_LGE)
static bool lvds_power_on;
static int lvds_panel_power(int on)
{
	static struct regulator *reg_lvs7, *reg_l2, *reg_ext_3p3v;
	static int gpio36, gpio26, mpp3;
	int rc;

	pr_debug("%s: on=%d\n", __func__, on);

	if (!lvds_power_on) {
		reg_lvs7 = regulator_get(&msm_lvds_device.dev,
				"lvds_vdda");
		if (IS_ERR_OR_NULL(reg_lvs7)) {
			pr_err("could not get 8921_lvs7, rc = %ld\n",
				PTR_ERR(reg_lvs7));
			return -ENODEV;
		}

		reg_l2 = regulator_get(&msm_lvds_device.dev,
				"lvds_pll_vdda");
		if (IS_ERR_OR_NULL(reg_l2)) {
			pr_err("could not get 8921_l2, rc = %ld\n",
				PTR_ERR(reg_l2));
			return -ENODEV;
		}

		rc = regulator_set_voltage(reg_l2, 1200000, 1200000);
		if (rc) {
			pr_err("set_voltage l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}

		reg_ext_3p3v = regulator_get(&msm_lvds_device.dev,
			"lvds_vccs_3p3v");
		if (IS_ERR_OR_NULL(reg_ext_3p3v)) {
			pr_err("could not get reg_ext_3p3v, rc = %ld\n",
			       PTR_ERR(reg_ext_3p3v));
		    return -ENODEV;
		}

		gpio26 = PM8921_GPIO_PM_TO_SYS(26);
		rc = gpio_request(gpio26, "pwm_backlight_ctrl");
		if (rc) {
			pr_err("request gpio 26 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		gpio36 = PM8921_GPIO_PM_TO_SYS(36); /* lcd1_pwr_en_n */
		rc = gpio_request(gpio36, "lcd1_pwr_en_n");
		if (rc) {
			pr_err("request gpio 36 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		mpp3 = PM8921_MPP_PM_TO_SYS(3);
		rc = gpio_request(mpp3, "backlight_en");
		if (rc) {
			pr_err("request mpp3 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		lvds_power_on = true;
	}

	if (on) {
		rc = regulator_enable(reg_lvs7);
		if (rc) {
			pr_err("enable lvs7 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		rc = regulator_set_optimum_mode(reg_l2, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_enable(reg_l2);
		if (rc) {
			pr_err("enable l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		rc = regulator_enable(reg_ext_3p3v);
		if (rc) {
			pr_err("enable reg_ext_3p3v failed, rc=%d\n", rc);
			return -ENODEV;
		}

		gpio_set_value_cansleep(gpio36, 0);
		gpio_set_value_cansleep(mpp3, 1);
		if (socinfo_get_pmic_model() == PMIC_MODEL_PM8917)
			gpio_set_value_cansleep(gpio26, 1);
	} else {
		if (socinfo_get_pmic_model() == PMIC_MODEL_PM8917)
			gpio_set_value_cansleep(gpio26, 0);
		gpio_set_value_cansleep(mpp3, 0);
		gpio_set_value_cansleep(gpio36, 1);

		rc = regulator_disable(reg_lvs7);
		if (rc) {
			pr_err("disable reg_lvs7 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_disable(reg_l2);
		if (rc) {
			pr_err("disable reg_l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_disable(reg_ext_3p3v);
		if (rc) {
			pr_err("disable reg_ext_3p3v failed, rc=%d\n", rc);
			return -ENODEV;
		}
	}

	return 0;
}

static int lvds_pixel_remap(void)
{
	u32 ver = socinfo_get_version();

	if (machine_is_apq8064_cdp() ||
	    machine_is_apq8064_liquid()) {
		if ((SOCINFO_VERSION_MAJOR(ver) == 1) &&
		    (SOCINFO_VERSION_MINOR(ver) == 0))
			return LVDS_PIXEL_MAP_PATTERN_1;
	} else if (machine_is_mpq8064_dtv()) {
		if ((SOCINFO_VERSION_MAJOR(ver) == 1) &&
		    (SOCINFO_VERSION_MINOR(ver) == 0))
			return LVDS_PIXEL_MAP_PATTERN_2;
	}
	return 0;
}

static struct lcdc_platform_data lvds_pdata = {
	.lcdc_power_save = lvds_panel_power,
	.lvds_pixel_remap = lvds_pixel_remap
};

#define LPM_CHANNEL 2
static int lvds_chimei_gpio[] = {LPM_CHANNEL};

static struct lvds_panel_platform_data lvds_chimei_pdata = {
	.gpio = lvds_chimei_gpio,
};

static struct platform_device lvds_chimei_panel_device = {
	.name = "lvds_chimei_wxga",
	.id = 0,
	.dev = {
		.platform_data = &lvds_chimei_pdata,
	}
};

#define FRC_GPIO_UPDATE	(SX150X_EXP4_GPIO_BASE + 8)
#define FRC_GPIO_RESET	(SX150X_EXP4_GPIO_BASE + 9)
#define FRC_GPIO_PWR	(SX150X_EXP4_GPIO_BASE + 10)

static int lvds_frc_gpio[] = {FRC_GPIO_UPDATE, FRC_GPIO_RESET, FRC_GPIO_PWR};
static struct lvds_panel_platform_data lvds_frc_pdata = {
	.gpio = lvds_frc_gpio,
};

static struct platform_device lvds_frc_panel_device = {
	.name = "lvds_frc_fhd",
	.id = 0,
	.dev = {
		.platform_data = &lvds_frc_pdata,
	}
};

static int dsi2lvds_gpio[2] = {
	LPM_CHANNEL,/* Backlight PWM-ID=0 for PMIC-GPIO#24 */
	0x1F08 /* DSI2LVDS Bridge GPIO Output, mask=0x1f, out=0x08 */
};
static struct msm_panel_common_pdata mipi_dsi2lvds_pdata = {
	.gpio_num = dsi2lvds_gpio,
};

static struct platform_device mipi_dsi2lvds_bridge_device = {
	.name = "mipi_tc358764",
	.id = 0,
	.dev.platform_data = &mipi_dsi2lvds_pdata,
};

static int toshiba_gpio[] = {LPM_CHANNEL};
static struct mipi_dsi_panel_platform_data toshiba_pdata = {
	.gpio = toshiba_gpio,
};

static struct platform_device mipi_dsi_toshiba_panel_device = {
	.name = "mipi_toshiba",
	.id = 0,
	.dev = {
			.platform_data = &toshiba_pdata,
	}
};

#endif  /*              */

static struct msm_bus_vectors dtv_bus_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
};

static struct msm_bus_vectors dtv_bus_def_vectors[] = {
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
#if defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_FHD_PT) ||\
	defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_FHD_INVERSE_PT)
		.ab = 2000000000,
		.ib = 2000000000,
#else
		.ab = 566092800 * 2,
		.ib = 707616000 * 2,
#endif
	},
};

static struct msm_bus_paths dtv_bus_scale_usecases[] = {
	{
		ARRAY_SIZE(dtv_bus_init_vectors),
		dtv_bus_init_vectors,
	},
	{
		ARRAY_SIZE(dtv_bus_def_vectors),
		dtv_bus_def_vectors,
	},
};
static struct msm_bus_scale_pdata dtv_bus_scale_pdata = {
	dtv_bus_scale_usecases,
	ARRAY_SIZE(dtv_bus_scale_usecases),
	.name = "dtv",
};

static struct lcdc_platform_data dtv_pdata = {
	.bus_scale_table = &dtv_bus_scale_pdata,
	.lcdc_power_save = hdmi_panel_power,
};

static int hdmi_panel_power(int on)
{
	int rc;

	pr_debug("%s: HDMI Core: %s\n", __func__, (on ? "ON" : "OFF"));
	rc = hdmi_core_power(on, 1);
	if (rc)
		rc = hdmi_cec_power(on);

	pr_debug("%s: HDMI Core: %s Success\n", __func__, (on ? "ON" : "OFF"));
	return rc;
}

static int hdmi_enable_5v(int on)
{
#ifdef CONFIG_HDMI_MVS
	/* TBD: PM8921 regulator instead of 8901 */
	static struct regulator *reg_8921_hdmi_mvs;	/* HDMI_5V */
	static int prev_on;
	int rc;

	if (on == prev_on)
		return 0;

	if (!reg_8921_hdmi_mvs) {
		reg_8921_hdmi_mvs = regulator_get(&hdmi_msm_device.dev,
			"hdmi_mvs");
		if (IS_ERR(reg_8921_hdmi_mvs)) {
			pr_err("could not get reg_8921_hdmi_mvs, rc = %ld\n",
				PTR_ERR(reg_8921_hdmi_mvs));
			reg_8921_hdmi_mvs = NULL;
			return -ENODEV;
		}
	}

	if (on) {
		rc = regulator_enable(reg_8921_hdmi_mvs);
		if (rc) {
			pr_err("'%s' regulator enable failed, rc=%d\n",
				"8921_hdmi_mvs", rc);
			return rc;
		}
		pr_debug("%s(on): success\n", __func__);
	} else {
		rc = regulator_disable(reg_8921_hdmi_mvs);
		if (rc)
			pr_warning("'%s' regulator disable failed, rc=%d\n",
				"8921_hdmi_mvs", rc);
		pr_debug("%s(off): success\n", __func__);
	}

	prev_on = on;
#endif

	return 0;
}

static int hdmi_core_power(int on, int show)
{
#ifdef CONFIG_HDMI_MVS
	static struct regulator *reg_8921_lvs7, *reg_8921_s4, *reg_ext_3p3v;
#else
	static struct regulator *reg_8921_lvs7;
#endif
	static int prev_on;
	int rc;

	if (on == prev_on)
		return 0;

#ifdef CONFIG_HDMI_MVS
	/* TBD: PM8921 regulator instead of 8901 */
	if (!reg_ext_3p3v) {
		reg_ext_3p3v = regulator_get(&hdmi_msm_device.dev,
					     "hdmi_mux_vdd");
		if (IS_ERR_OR_NULL(reg_ext_3p3v)) {
			pr_err("could not get reg_ext_3p3v, rc = %ld\n",
			       PTR_ERR(reg_ext_3p3v));
			reg_ext_3p3v = NULL;
			return -ENODEV;
		}
	}
#endif

	if (!reg_8921_lvs7) {
		reg_8921_lvs7 = regulator_get(&hdmi_msm_device.dev,
					      "hdmi_vdda");
		if (IS_ERR(reg_8921_lvs7)) {
			pr_err("could not get reg_8921_lvs7, rc = %ld\n",
				PTR_ERR(reg_8921_lvs7));
			reg_8921_lvs7 = NULL;
			return -ENODEV;
		}
	}
#ifdef CONFIG_HDMI_MVS
	if (!reg_8921_s4) {
		reg_8921_s4 = regulator_get(&hdmi_msm_device.dev,
					    "hdmi_lvl_tsl");
		if (IS_ERR(reg_8921_s4)) {
			pr_err("could not get reg_8921_s4, rc = %ld\n",
				PTR_ERR(reg_8921_s4));
			reg_8921_s4 = NULL;
			return -ENODEV;
		}
		rc = regulator_set_voltage(reg_8921_s4, 1800000, 1800000);
		if (rc) {
			pr_err("set_voltage failed for 8921_s4, rc=%d\n", rc);
			return -EINVAL;
		}
	}
#endif

	if (on) {
		/*
		 * Configure 3P3V_BOOST_EN as GPIO, 8mA drive strength,
		 * pull none, out-high
		 */
#ifdef CONFIG_HDMI_MVS
		rc = regulator_set_optimum_mode(reg_ext_3p3v, 290000);
		if (rc < 0) {
			pr_err("set_optimum_mode ext_3p3v failed, rc=%d\n", rc);
			return -EINVAL;
		}

		rc = regulator_enable(reg_ext_3p3v);
		if (rc) {
			pr_err("enable reg_ext_3p3v failed, rc=%d\n", rc);
			return rc;
		}
#endif
		rc = regulator_enable(reg_8921_lvs7);
		if (rc) {
			pr_err("'%s' regulator enable failed, rc=%d\n",
				"hdmi_vdda", rc);
			goto error1;
		}
#ifdef CONFIG_HDMI_MVS
		rc = regulator_enable(reg_8921_s4);
		if (rc) {
			pr_err("'%s' regulator enable failed, rc=%d\n",
				"hdmi_lvl_tsl", rc);
			goto error2;
		}
		pr_debug("%s(on): success\n", __func__);
#endif
	} else {
#ifdef CONFIG_HDMI_MVS
		rc = regulator_disable(reg_ext_3p3v);
		if (rc) {
			pr_err("disable reg_ext_3p3v failed, rc=%d\n", rc);
			return -ENODEV;
		}
#else
		rc = regulator_disable(reg_8921_lvs7);
		if (rc) {
			pr_err("disable reg_8921_l23 failed, rc=%d\n", rc);
			return -ENODEV;
		}
#endif

#ifdef CONFIG_HDMI_MVS
		rc = regulator_disable(reg_8921_s4);
		if (rc) {
			pr_err("disable reg_8921_s4 failed, rc=%d\n", rc);
			return -ENODEV;
		}
#endif
		pr_debug("%s(off): success\n", __func__);
	}

	prev_on = on;

	return 0;

#ifdef CONFIG_HDMI_MVS
error2:
	regulator_disable(reg_8921_lvs7);
error1:
	regulator_disable(reg_ext_3p3v);
	return rc;
#else
error1:
	return rc;
#endif
}

static int hdmi_gpio_config(int on)
{
	int rc = 0;
	static int prev_on;
#ifdef CONFIG_HDMI_MVS
	int pmic_gpio14 = PM8921_GPIO_PM_TO_SYS(14);
#endif

	if (on == prev_on)
		return 0;

	if (on) {
		rc = gpio_request(HDMI_DDC_CLK_GPIO, "HDMI_DDC_CLK");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"HDMI_DDC_CLK", HDMI_DDC_CLK_GPIO, rc);
			goto error1;
		}
		rc = gpio_request(HDMI_DDC_DATA_GPIO, "HDMI_DDC_DATA");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"HDMI_DDC_DATA", HDMI_DDC_DATA_GPIO, rc);
			goto error2;
		}
		rc = gpio_request(HDMI_HPD_GPIO, "HDMI_HPD");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"HDMI_HPD", HDMI_HPD_GPIO, rc);
			goto error3;
		}
#ifdef CONFIG_HDMI_MVS
		if (machine_is_apq8064_liquid()) {
			rc = gpio_request(pmic_gpio14, "PMIC_HDMI_MUX_SEL");
			if (rc) {
				pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
					"PMIC_HDMI_MUX_SEL", 14, rc);
				goto error4;
			}
			gpio_set_value_cansleep(pmic_gpio14, 0);
		}
		pr_debug("%s(on): success\n", __func__);
#endif
	} else {
		gpio_free(HDMI_DDC_CLK_GPIO);
		gpio_free(HDMI_DDC_DATA_GPIO);
		gpio_free(HDMI_HPD_GPIO);

#ifdef CONFIG_HDMI_MVS
		if (machine_is_apq8064_liquid()) {
			gpio_set_value_cansleep(pmic_gpio14, 1);
			gpio_free(pmic_gpio14);
		}
#endif
		pr_debug("%s(off): success\n", __func__);
	}

	prev_on = on;
	return 0;

#ifdef CONFIG_HDMI_MVS
error4:
	gpio_free(HDMI_HPD_GPIO);
#endif
error3:
	gpio_free(HDMI_DDC_DATA_GPIO);
error2:
	gpio_free(HDMI_DDC_CLK_GPIO);
error1:
	return rc;
}

static int hdmi_cec_power(int on)
{
	static int prev_on;
	int rc;

	if (on == prev_on)
		return 0;

	if (on) {
		rc = gpio_request(HDMI_CEC_VAR_GPIO, "HDMI_CEC_VAR");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"HDMI_CEC_VAR", HDMI_CEC_VAR_GPIO, rc);
			goto error;
		}
		pr_debug("%s(on): success\n", __func__);
	} else {
		gpio_free(HDMI_CEC_VAR_GPIO);
		pr_debug("%s(off): success\n", __func__);
	}

	prev_on = on;

	return 0;
error:
	return rc;
}

#if defined (CONFIG_BACKLIGHT_LM3530)
extern void lm3530_lcd_backlight_set_level( int level);
#elif defined (CONFIG_BACKLIGHT_LM3533)
extern void lm3533_lcd_backlight_set_level( int level);
#endif /* CONFIG_BACKLIGHT_LMXXXX */

/* For GJ JB+ Bring Up */
#if defined(CONFIG_FB_MSM_MIPI_HITACHI_VIDEO_HD_PT)
static int mipi_hitachi_backlight_level(int level, int max, int min)
{
	lm3533_lcd_backlight_set_level(level);
	return 0;
}

/* HITACHI 4.67" HD panel */
static char set_address_mode[2] = {0x36, 0x00};
static char set_pixel_format[2] = {0x3A, 0x70};

static char exit_sleep[2]  = {0x11, 0x00};
static char display_on[2]  = {0x29, 0x00};
static char enter_sleep[2] = {0x10, 0x00};
static char display_off[2] = {0x28, 0x00};

static char macp_off[2] = {0xB0, 0x04};
static char macp_on[2]  = {0xB0, 0x03};

#if defined(CONFIG_LGE_BACKLIGHT_CABC)
#define CABC_POWERON_OFFSET 4 /* offset from lcd display on cmds */

#define CABC_OFF 0
#define CABC_ON 1

#define CABC_10 1
#define CABC_20 2
#define CABC_30 3
#define CABC_40 4
#define CABC_50 5

#define CABC_DEFUALT CABC_10

#if defined (CONFIG_LGE_BACKLIGHT_CABC_DEBUG)
static int hitachi_cabc_index = CABC_DEFUALT;
#endif

static char backlight_ctrl1[2][6] = {

	/* off */
	{
		0xB8, 0x00, 0x00, 0x30,
		0x18, 0x18
	},
	/* on */
	{
		0xB8, 0x01, 0x00, 0x30,
		0x18, 0x18
	},
};

static char backlight_ctrl2[6][8] = {
	/* off */
	{
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00
	},
	/* 10% */
	{
		0xB9, 0x18, 0x00, 0x18,
		0x18, 0x9F, 0x1F, 0x0F
	},

	/* 20% */
	{
		0xB9, 0x18, 0x00, 0x18,
		0x18, 0x9F, 0x1F, 0x0F
	},

	/* 30% */
	{
		0xB9, 0x18, 0x00, 0x18,
		0x18, 0x9F, 0x1F, 0x0F
	},

	/* 40% */
	{
		0xB9, 0x18, 0x00, 0x18,
		0x18, 0x9F, 0x1F, 0x0F
	},
	/* 50% */
	{
		0xB9, 0x18, 0x00, 0x18,
		0x18, 0x9F, 0x1F, 0x0F
	}
};

static char backlight_ctrl3[6][25] = {
	/* off */
	{
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00
	},
	/* 10% */
	{
		0xBA, 0x00, 0x00, 0x0C,
		0x12, 0x6C, 0x11, 0x6C,
		0x12, 0x0C, 0x12, 0x00,
		0xDA, 0x6D, 0x03, 0xFF,
		0xFF, 0x10, 0x67, 0xA3,
		0xDB, 0xFB, 0xFF, 0x9F,
		0x00
	},
	/* 20% */
	{
		0xBA, 0x00, 0x00, 0x0C,
		0x0B, 0x6C, 0x0B, 0xAC,
		0x0B, 0x0C, 0x0B, 0x00,
		0xDA, 0x6D, 0x03, 0xFF,
		0xFF, 0x10, 0xB3, 0xC9,
		0xDC, 0xEE, 0xFF, 0x9F,
		0x00
	},
	/* 30% */
	{
		0xBA, 0x00, 0x00, 0x0C,
		0x0D, 0x6C, 0x0D, 0xAC,
		0x0D, 0x0C, 0x0D, 0x00,
		0xDA, 0x6D, 0x03, 0xFF,
		0xFF, 0x10, 0x8C, 0xAA,
		0xC7, 0xE3, 0xFF, 0x9F,
		0x00
	},
	/* 40% */
	{
		0xBA, 0x00, 0x00, 0x0C,
		0x13, 0xAC, 0x13, 0x6C,
		0x13, 0x0C, 0x13, 0x00,
		0xDA, 0x6D, 0x03, 0xFF,
		0xFF, 0x10, 0x67, 0x89,
		0xAF, 0xD6, 0xFF, 0x9F,
		0x00
	},
	/* 50% */
	{
		0xBA, 0x00, 0x00, 0x0C,
		0x14, 0xAC, 0x14, 0x6C,
		0x14, 0x0C, 0x14, 0x00,
		0xDA, 0x6D, 0x03, 0xFF,
		0xFF, 0x10, 0x37, 0x5A,
		0x87, 0xBD, 0xFF, 0x9F,
		0x00
	}
};
#endif

static struct dsi_cmd_desc hitachi_power_on_set[] = {
	/* Display initial set */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 20, sizeof(set_address_mode),
		set_address_mode},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(set_pixel_format),
		set_pixel_format},

	/* Sleep mode exit */
	{DTYPE_DCS_WRITE, 1, 0, 0, 70, sizeof(exit_sleep), exit_sleep},

	/* Manufacturer command protect off */
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(macp_off), macp_off},
#if defined(CONFIG_LGE_BACKLIGHT_CABC)
	/* Content adaptive backlight control */
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(backlight_ctrl1[1]),
		backlight_ctrl1[CABC_ON]},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(backlight_ctrl2[CABC_DEFUALT]),
		backlight_ctrl2[CABC_DEFUALT]},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(backlight_ctrl3[CABC_DEFUALT]),
		backlight_ctrl3[CABC_DEFUALT]},
#endif
	/* Manufacturer command protect on */
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(macp_on), macp_on},
	/* Display on */
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(display_on), display_on},
};

static struct dsi_cmd_desc hitachi_power_off_set[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(enter_sleep), enter_sleep}
};

#if defined (CONFIG_LGE_BACKLIGHT_CABC) &&\
		defined (CONFIG_LGE_BACKLIGHT_CABC_DEBUG)
void set_hitachi_cabc(int cabc_index)
{
	switch(cabc_index) {
	case 0: /* off */
	case 1: /* 10% */
	case 2: /* 20% */
	case 3: /* 30% */
	case 4: /* 40% */
	case 5: /* 50% */
		if (cabc_index == 0) { /* CABC OFF */
			hitachi_power_on_set[CABC_POWERON_OFFSET].payload =
						backlight_ctrl1[CABC_OFF];
		} else { /* CABC ON */
			hitachi_power_on_set[CABC_POWERON_OFFSET].payload =
						backlight_ctrl1[CABC_ON];
			hitachi_power_on_set[CABC_POWERON_OFFSET+1].payload =
						backlight_ctrl2[cabc_index];
			hitachi_power_on_set[CABC_POWERON_OFFSET+2].payload =
						backlight_ctrl3[cabc_index];
		}
		hitachi_cabc_index = cabc_index;
		break;
	default:
		printk("out of range cabc_index %d", cabc_index);
	}
	return;
}
EXPORT_SYMBOL(set_hitachi_cabc);

int get_hitachi_cabc(void)
{
	return hitachi_cabc_index;
}
EXPORT_SYMBOL(get_hitachi_cabc);

#endif
static struct msm_panel_common_pdata mipi_hitachi_pdata = {
	.backlight_level = mipi_hitachi_backlight_level,
	.power_on_set_1 = hitachi_power_on_set,
	.power_off_set_1 = hitachi_power_off_set,
	.power_on_set_size_1 = ARRAY_SIZE(hitachi_power_on_set),
	.power_off_set_size_1 = ARRAY_SIZE(hitachi_power_off_set),
};

static struct platform_device mipi_dsi_hitachi_panel_device = {
	.name = "mipi_hitachi",
	.id = 0,
	.dev = {
		.platform_data = &mipi_hitachi_pdata,
	}
};

/* L05E LCD Initial Set */
#elif defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_HD_PT)
static int mipi_lgit_backlight_level(int level, int max, int min)
{
	lm3530_lcd_backlight_set_level(level);
	return 0;
}

static char set_address[2]    = {0x36, 0x02};//Set Address Mode
static char dsi_config[6]     = {0xE0, 0x43, 0x00, 0x80, 0x00, 0x00};//MIPI DSI configuration
static char display_mode1[6]  = {0xB5, 0x59, 0x20, 0x40, 0x00, 0x20};//Display Control 1
static char display_mode2[6]  = {0xB6, 0x01, 0x40, 0x0F, 0x16, 0x13};//Display control 2

static char p_gamma_r_setting[10] = {0xD0, 0x60, 0x07, 0x62, 0x26, 0x01, 0x06, 0x70, 0x44, 0x04};//Gamma Curve for Red
static char n_gamma_r_setting[10] = {0xD1, 0x60, 0x07, 0x62, 0x26, 0x01, 0x06, 0x70, 0x44, 0x04};
static char p_gamma_g_setting[10] = {0xD2, 0x60, 0x07, 0x62, 0x26, 0x01, 0x06, 0x70, 0x44, 0x04};//Gamma Curve for Green
static char n_gamma_g_setting[10] = {0xD3, 0x60, 0x07, 0x62, 0x26, 0x01, 0x06, 0x70, 0x44, 0x04};
static char p_gamma_b_setting[10] = {0xD4, 0x60, 0x07, 0x62, 0x26, 0x01, 0x06, 0x70, 0x44, 0x04};//Gamma Curve for Blue
static char n_gamma_b_setting[10] = {0xD5, 0x60, 0x07, 0x62, 0x26, 0x01, 0x06, 0x70, 0x44, 0x04};

#if defined(LGIT_IEF)
static char ief_on_set0[2]  = {0x70, 0x03};
static char ief_on_set1[5]  = {0x71, 0x00, 0x00, 0x01, 0x01};
static char ief_on_set2[3]  = {0x72, 0x01, 0x05};
static char ief_on_set3[4]  = {0x73, 0x54, 0x54, 0x60};
static char ief_on_set4[4]  = {0x74, 0x02, 0x81, 0x01};
static char ief_on_set5[4]  = {0x75, 0x01, 0x82, 0x80};
static char ief_on_set6[4]  = {0x76, 0x07, 0x00, 0x05};
static char ief_on_set7[9]  = {0x77, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D};
static char ief_on_set8[9]  = {0x78, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C};
static char ief_on_set9[9]  = {0x79, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40};
#endif

#if defined(LGIT_IEF_SWITCH)
static char ief_off_set0[4] = {0x76, 0x00, 0x00, 0x05};
static char ief_off_set1[4] = {0x75, 0x00, 0x0F, 0x07};
static char ief_off_set2[4] = {0x74, 0x00, 0x01, 0x07};
static char ief_off_set3[2] = {0x70, 0x00};
#endif

static char osc_setting[3]     = {0xC0, 0x00, 0x04};//Internal Oscillator Setting
static char power_setting3[10] = {0xC3, 0x00, 0x09, 0x10, 0x15, 0x01, 0x12, 0x4D, 0x23, 0x02};//Power control3
static char power_setting4[6]  = {0xC4, 0x22, 0x24, 0x15, 0x15, 0x4A};//Power control4
static char power_setting5[4]  = {0xC7, 0x10, 0x00, 0x14};//Power control5
static char otp2_setting[2]    = {0xF9, 0x10};//Internal Oscillator Setting

#if defined(CONFIG_LGIT_VIDEO_HD_CABC)
static char cabc_set0[2] = {0x51, 0xFF};
static char cabc_set1[2] = {0x5E, 0xE8};
static char cabc_set2[2] = {0x53, 0x2C};
static char cabc_set3[2] = {0x55, 0x01};
static char cabc_set4[5] = {0xC8, 0x22, 0xE3, 0x01, 0x11};
#endif

static char power_ctrl_on_1[2]  = {0xC2, 0x06};
static char power_ctrl_on_2[2]  = {0xC2, 0x0E};
static char power_ctrl_on_3[2]  = {0xC1, 0x08};
static char exit_sleep[2]	= {0x11, 0x00};//Sleep out
static char display_on[2]	= {0x29, 0x00};//Display on

static char display_off[2] = {0x28, 0x00};
static char enter_sleep[2] = {0x10, 0x00};

static char vgl_off[2]      = {0xC2, 0x02};
static char vgh_off[2]      = {0xC2, 0x00};
static char deep_standby[2] = {0xC1, 0x02};

/* initialize device */
static struct dsi_cmd_desc lgit_power_on_set_1[] = {
	/* Display Initial Set */
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(set_address),   set_address},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(dsi_config),    dsi_config},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(display_mode1), display_mode1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(display_mode2), display_mode2},

	/* Gamma Set */
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(p_gamma_r_setting), p_gamma_r_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(n_gamma_r_setting), n_gamma_r_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(p_gamma_g_setting), p_gamma_g_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(n_gamma_g_setting), n_gamma_g_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(p_gamma_b_setting), p_gamma_b_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(n_gamma_b_setting), n_gamma_b_setting},

#if defined(LGIT_IEF)
	/* Image Enhancement Function Set */
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(ief_on_set0), ief_on_set0},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(ief_on_set1), ief_on_set1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(ief_on_set2), ief_on_set2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(ief_on_set3), ief_on_set3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(ief_on_set4), ief_on_set4},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(ief_on_set5), ief_on_set5},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(ief_on_set6), ief_on_set6},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(ief_on_set7), ief_on_set7},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(ief_on_set8), ief_on_set8},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(ief_on_set9), ief_on_set9},
#endif
	/* Power Supply Set */
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(osc_setting),    osc_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(power_setting3), power_setting3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(power_setting4), power_setting4},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(power_setting5), power_setting5},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(otp2_setting),   otp2_setting},

#if defined(CONFIG_LGIT_VIDEO_HD_CABC)
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cabc_set0),  cabc_set0},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cabc_set1),  cabc_set1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cabc_set2),  cabc_set2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cabc_set3),  cabc_set3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cabc_set4),  cabc_set4},
#endif
};

static struct dsi_cmd_desc lgit_power_on_set_2[] = {
	/* Power Supply Set */
	{DTYPE_GEN_LWRITE,  1, 0, 0, 10, sizeof(power_ctrl_on_1), power_ctrl_on_1},
	{DTYPE_GEN_LWRITE,  1, 0, 0, 1, sizeof(power_ctrl_on_2), power_ctrl_on_2},
};

static struct dsi_cmd_desc lgit_power_on_set_3[] = {
	/* Display On Set */
	{DTYPE_GEN_LWRITE, 1, 0, 0, 20, sizeof(power_ctrl_on_3), power_ctrl_on_3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0,  0, sizeof(display_on), display_on},
};

static struct dsi_cmd_desc lgit_power_off_set_1[] = {
	/* Display Off Set*/ /* wait 3frames */
	{DTYPE_DCS_WRITE, 1, 0, 0, 20, sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0,  5, sizeof(enter_sleep), enter_sleep},
	/* wait 7frames */
};

static struct dsi_cmd_desc lgit_power_off_set_2[] = {
	/* Sleep In & Deep Standby Set */
	{DTYPE_GEN_LWRITE, 1, 0, 0,  5, sizeof(vgl_off),      vgl_off},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 10, sizeof(vgh_off),      vgh_off},
	{DTYPE_GEN_LWRITE, 1, 0, 0,  5, sizeof(deep_standby), deep_standby},
};

#ifdef LGIT_IEF_SWITCH
static struct dsi_cmd_desc lgit_ief_off_set[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(ief_off_set0), ief_off_set0},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(ief_off_set1), ief_off_set1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(ief_off_set2), ief_off_set2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(ief_off_set3), ief_off_set3},
};

static struct dsi_cmd_desc lgit_ief_on_set[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(ief_on_set0), ief_on_set0},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(ief_on_set1), ief_on_set1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(ief_on_set2), ief_on_set2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(ief_on_set3), ief_on_set3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(ief_on_set4), ief_on_set4},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(ief_on_set5), ief_on_set5},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(ief_on_set6), ief_on_set6},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(ief_on_set7), ief_on_set7},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(ief_on_set8), ief_on_set8},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(ief_on_set9), ief_on_set9},
};
#endif

static struct msm_panel_common_pdata mipi_lgit_pdata = {
	.backlight_level = mipi_lgit_backlight_level,
	.power_on_set_1  = lgit_power_on_set_1,
	.power_on_set_2  = lgit_power_on_set_2,
	.power_on_set_3  = lgit_power_on_set_3,

	.power_on_set_ief  = lgit_ief_on_set,
	.power_off_set_ief = lgit_ief_off_set,

	.power_on_set_size_1 = ARRAY_SIZE(lgit_power_on_set_1),
	.power_on_set_size_2 = ARRAY_SIZE(lgit_power_on_set_2),
	.power_on_set_size_3 = ARRAY_SIZE(lgit_power_on_set_3),

	.power_on_set_ief_size  = ARRAY_SIZE(lgit_ief_on_set),
	.power_off_set_ief_size = ARRAY_SIZE(lgit_ief_off_set),

	.power_off_set_1 = lgit_power_off_set_1,
	.power_off_set_2 = lgit_power_off_set_2,
	.power_off_set_size_1 = ARRAY_SIZE(lgit_power_off_set_1),
	.power_off_set_size_2 = ARRAY_SIZE(lgit_power_off_set_2),
};
static struct platform_device mipi_dsi_lgit_panel_device = {
	.name = "mipi_lgit",
	.id = 0,
	.dev = {
		.platform_data = &mipi_lgit_pdata,
	}
};
#endif

static struct platform_device *L05E_panel_devices[] __initdata = {
#if defined(CONFIG_FB_MSM_MIPI_HITACHI_VIDEO_HD_PT)
	&mipi_dsi_hitachi_panel_device,
#elif defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_HD_PT)
	&mipi_dsi_lgit_panel_device,
#endif
#ifdef CONFIG_LGE_KCAL
	&kcal_platrom_device,
#endif
};

void __init apq8064_init_fb(void)
{
	platform_device_register(&msm_fb_device);

#ifndef CONFIG_MACH_LGE
	platform_device_register(&lvds_chimei_panel_device);
#endif /*                */

#ifdef CONFIG_FB_MSM_WRITEBACK_MSM_PANEL
	platform_device_register(&wfd_panel_device);
	platform_device_register(&wfd_device);
#endif /* CONFIG_FB_MSM_WRITEBACK_MSM_PANEL */

#if defined(CONFIG_MACH_LGE)
	platform_add_devices(L05E_panel_devices,
			ARRAY_SIZE(L05E_panel_devices));
#endif

#ifndef CONFIG_MACH_LGE
	if (machine_is_apq8064_liquid())
		platform_device_register(&mipi_dsi2lvds_bridge_device);
	if (machine_is_apq8064_mtp())
		platform_device_register(&mipi_dsi_toshiba_panel_device);
	if (machine_is_mpq8064_dtv())
		platform_device_register(&lvds_frc_panel_device);
#endif /*                 */

	msm_fb_register_device("mdp", &mdp_pdata);
#ifndef CONFIG_MACH_LGE
	msm_fb_register_device("lvds", &lvds_pdata);
#endif /*                 */
	msm_fb_register_device("mipi_dsi", &mipi_dsi_pdata);
	platform_device_register(&hdmi_msm_device);
	msm_fb_register_device("dtv", &dtv_pdata);
}

/**
 * Set MDP clocks to high frequency to avoid DSI underflow
 * when using high resolution 1200x1920 WUXGA panels
 */
 #ifndef CONFIG_MACH_LGE
static void set_mdp_clocks_for_wuxga(void)
{
	int i;

	mdp_ui_vectors[0].ab = 2000000000;
	mdp_ui_vectors[0].ib = 2000000000;
	mdp_vga_vectors[0].ab = 2000000000;
	mdp_vga_vectors[0].ib = 2000000000;
	mdp_720p_vectors[0].ab = 2000000000;
	mdp_720p_vectors[0].ib = 2000000000;
	mdp_1080p_vectors[0].ab = 2000000000;
	mdp_1080p_vectors[0].ib = 2000000000;

	if (apq8064_hdmi_as_primary_selected()) {
		dtv_bus_def_vectors[0].ab = 2000000000;
		dtv_bus_def_vectors[0].ib = 2000000000;
	}
}

void __init apq8064_set_display_params(char *prim_panel, char *ext_panel,
		unsigned char resolution)
{
	/*
	 * For certain MPQ boards, HDMI should be set as primary display
	 * by default, with the flexibility to specify any other panel
	 * as a primary panel through boot parameters.
	 */
	if (machine_is_mpq8064_hrd() || machine_is_mpq8064_cdp()) {
		pr_debug("HDMI is the primary display by default for MPQ\n");
		if (!strnlen(prim_panel, PANEL_NAME_MAX_LEN))
			strlcpy(msm_fb_pdata.prim_panel_name, HDMI_PANEL_NAME,
				PANEL_NAME_MAX_LEN);
	}

	if (strnlen(prim_panel, PANEL_NAME_MAX_LEN)) {
		strlcpy(msm_fb_pdata.prim_panel_name, prim_panel,
			PANEL_NAME_MAX_LEN);
		pr_debug("msm_fb_pdata.prim_panel_name %s\n",
			msm_fb_pdata.prim_panel_name);

		if (!strncmp((char *)msm_fb_pdata.prim_panel_name,
			HDMI_PANEL_NAME, strnlen(HDMI_PANEL_NAME,
				PANEL_NAME_MAX_LEN))) {
			pr_debug("HDMI is the primary display by"
				" boot parameter\n");
			hdmi_is_primary = 1;
			set_mdp_clocks_for_wuxga();
		}
	}
	if (strnlen(ext_panel, PANEL_NAME_MAX_LEN)) {
		strlcpy(msm_fb_pdata.ext_panel_name, ext_panel,
			PANEL_NAME_MAX_LEN);
		pr_debug("msm_fb_pdata.ext_panel_name %s\n",
			msm_fb_pdata.ext_panel_name);

                if (!strncmp((char *)msm_fb_pdata.ext_panel_name,
                        MHL_PANEL_NAME, strnlen(MHL_PANEL_NAME,
                                PANEL_NAME_MAX_LEN))) {
                        pr_debug("MHL is external display by boot parameter\n");
                        mhl_display_enabled = 1;
                }
	}

	msm_fb_pdata.ext_resolution = resolution;
        hdmi_msm_data.is_mhl_enabled = mhl_display_enabled;
}
#endif /*                 */

#define I2C_SURF    1
#define I2C_FFA    (1 << 1)
#define I2C_RUMI   (1 << 2)
#define I2C_SIM    (1 << 3)
#define I2C_LIQUID (1 << 4)

struct i2c_registry {
	u8                     machs;
	int                    bus;
	struct i2c_board_info *info;
	int                    len;
};

#if defined(CONFIG_LGIT_VIDEO_HD_CABC)
#define PWM_SIMPLE_EN  0xA0
#define PWM_BRIGHTNESS 0x20
#endif

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

#if defined (CONFIG_BACKLIGHT_LM3530)
static struct backlight_platform_data lm3530_data = {

	.gpio = PM8921_GPIO_PM_TO_SYS(24),
#if defined(CONFIG_LGIT_VIDEO_HD_CABC)
	.max_current = 0x17 | PWM_BRIGHTNESS,	//20121107 ej.jung ABS : Exp. -> Linear (0x15 -> 0x17)
#else
	.max_current = 0x17,			//20121107 ej.jung ABS : Exp. -> Linear (0x15 -> 0x17)
#endif
	.min_brightness = 0x00,
	.max_brightness = 0xFF,
	.default_brightness = 0x69,
	.factory_brightness = 0x36,
};
#elif defined(CONFIG_BACKLIGHT_LM3533)
static struct backlight_platform_data lm3533_data = {
	.gpio = PM8921_GPIO_PM_TO_SYS(24),
#if defined(CONFIG_LGE_BACKLIGHT_CABC)
	.max_current = 0x17 | PWM_SIMPLE_EN,
#else
	.max_current = 0x17,
#endif
	.min_brightness = 0x05,
	.max_brightness = 0xFF,
	.default_brightness = 0x9C,
	.factory_brightness = 0x45,
};
#endif
static struct i2c_board_info msm_i2c_backlight_info[] = {
	{

#if defined(CONFIG_BACKLIGHT_LM3530)
		I2C_BOARD_INFO("lm3530", 0x38),
		.platform_data = &lm3530_data,
#elif defined(CONFIG_BACKLIGHT_LM3533)
		I2C_BOARD_INFO("lm3533", 0x36),
		.platform_data = &lm3533_data,
#endif
	}
};
static struct i2c_registry apq8064_i2c_backlight_device[] __initdata = {

	{
	    I2C_SURF | I2C_FFA | I2C_RUMI | I2C_SIM | I2C_LIQUID,
		APQ_8064_GSBI1_QUP_I2C_BUS_ID,
		msm_i2c_backlight_info,
		ARRAY_SIZE(msm_i2c_backlight_info),
	},
};

void __init register_i2c_backlight_devices(void)
{
	u8 mach_mask = 0;
	int i;

	/* Build the matching 'supported_machs' bitmask */
	if (machine_is_apq8064_cdp())
		mach_mask = I2C_SURF;
	else if (machine_is_apq8064_mtp())
		mach_mask = I2C_FFA;
	else if (machine_is_apq8064_liquid())
		mach_mask = I2C_LIQUID;
	else if (machine_is_apq8064_rumi3())
		mach_mask = I2C_RUMI;
	else if (machine_is_apq8064_sim())
		mach_mask = I2C_SIM;
	else
		pr_err("unmatched machine ID in register_i2c_devices\n");

	/* Run the array and install devices as appropriate */
	for (i = 0; i < ARRAY_SIZE(apq8064_i2c_backlight_device); ++i) {
		if (apq8064_i2c_backlight_device[i].machs & mach_mask)
			i2c_register_board_info(apq8064_i2c_backlight_device[i].bus,
						apq8064_i2c_backlight_device[i].info,
						apq8064_i2c_backlight_device[i].len);
	}
}
