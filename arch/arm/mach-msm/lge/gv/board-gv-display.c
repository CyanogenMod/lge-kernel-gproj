/* Copyright (c) 2012, Code Aurora Forum. All rights reserved.
 * Copyright (c) 2012, LGE Inc.
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
#include <linux/ion.h>
#include <asm/mach-types.h>
#include <mach/msm_memtypes.h>
#include <mach/board.h>
#include <mach/gpio.h>
#include <mach/gpiomux.h>
#include <mach/ion.h>
#include <mach/msm_bus_board.h>
#include <mach/socinfo.h>

#include "devices.h"
#include "board-gv.h"

#include "../../../../drivers/video/msm/msm_fb.h"
#include "../../../../drivers/video/msm/msm_fb_def.h"
#include "../../../../drivers/video/msm/mipi_dsi.h"

#include <mach/board_lge.h>

#include <linux/i2c.h>
#include <linux/kernel.h>

#ifndef LGE_DSDR_SUPPORT
#define LGE_DSDR_SUPPORT
#endif

#ifdef CONFIG_LGE_KCAL
#ifdef CONFIG_LGE_QC_LCDC_LUT
extern int set_qlut_kcal_values(int kcal_r, int kcal_g, int kcal_b);
extern int refresh_qlut_display(void);
#else
#error only kcal by Qucalcomm LUT is supported now!!!
#endif
#endif

#ifdef CONFIG_FB_MSM_TRIPLE_BUFFER
/* prim = 1366 x 768 x 3(bpp) x 3(pages) */
#if defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_WXGA_PT)
#define MSM_FB_PRIM_BUF_SIZE roundup(768 * 1280 * 4 * 3, 0x10000)
#elif defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_FHD_PT) ||\
	defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_FHD_INVERSE_PT)
#define MSM_FB_PRIM_BUF_SIZE roundup(1088 * 1920 * 4 * 3, 0x10000)
#else
#define MSM_FB_PRIM_BUF_SIZE roundup(1920 * 1088 * 4 * 3, 0x10000)
#endif
#else
/* prim = 1366 x 768 x 3(bpp) x 2(pages) */
#if defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_WXGA_PT)
#define MSM_FB_PRIM_BUF_SIZE roundup(768 * 1280 * 4 * 2, 0x10000)
#elif defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_FHD_PT) ||\
	defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_FHD_INVERSE_PT)
#define MSM_FB_PRIM_BUF_SIZE roundup(1088 * 1920 * 4 * 2, 0x10000)
#else
#define MSM_FB_PRIM_BUF_SIZE roundup(1920 * 1088 * 4 * 2, 0x10000)
#endif
#endif /*CONFIG_FB_MSM_TRIPLE_BUFFER */

#ifdef LGE_DSDR_SUPPORT
#define MSM_FB_EXT_BUF_SIZE \
        (roundup((1920 * 1088 * 4), 4096) * 3) /* 4 bpp x 3 page */
#else  /* LGE_DSDR_SUPPORT */
#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
#define MSM_FB_EXT_BUF_SIZE \
		(roundup((1920 * 1088 * 2), 4096) * 1) /* 2 bpp x 1 page */
#elif defined(CONFIG_FB_MSM_TVOUT)
#define MSM_FB_EXT_BUF_SIZE \
		(roundup((720 * 576 * 2), 4096) * 2) /* 2 bpp x 2 pages */
#else
#define MSM_FB_EXT_BUF_SIZE	0
#endif /* CONFIG_FB_MSM_HDMI_MSM_PANEL */
#endif /* LGE_DSDR_SUPPORT */

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
#endif

#define MSM_FB_SIZE \
	roundup(MSM_FB_PRIM_BUF_SIZE + \
		MSM_FB_EXT_BUF_SIZE + MSM_FB_WFD_BUF_SIZE, 4096)

#ifdef CONFIG_FB_MSM_OVERLAY0_WRITEBACK
	#if defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_WXGA_PT)
	#define MSM_FB_OVERLAY0_WRITEBACK_SIZE roundup((768 * 1280 * 3 * 2), 4096)
	#elif defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_FHD_PT) ||\
	defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_FHD_INVERSE_PT)
	#define MSM_FB_OVERLAY0_WRITEBACK_SIZE roundup((1088 * 1920 * 3 * 2), 4096)
	#else
	#define MSM_FB_OVERLAY0_WRITEBACK_SIZE (0)
	#endif
#else
#define MSM_FB_OVERLAY0_WRITEBACK_SIZE (0)
#endif  /* CONFIG_FB_MSM_OVERLAY0_WRITEBACK */

#ifdef CONFIG_FB_MSM_OVERLAY1_WRITEBACK
#define MSM_FB_OVERLAY1_WRITEBACK_SIZE roundup((1920 * 1088 * 3 * 2), 4096)
#else
#define MSM_FB_OVERLAY1_WRITEBACK_SIZE (0)
#endif  /* CONFIG_FB_MSM_OVERLAY1_WRITEBACK */

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
#endif /* CONFIG_MACH_LGE */

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
#endif /* CONFIG_MACH_LGE */
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
		.ab = 577474560 * 2,//.ab = 2000000000,    // 602603520 * 2,
		.ib = 866211840 * 2,//.ib = 2000000000,    // 753254400 * 2,
	},
};

static struct msm_bus_vectors mdp_vga_vectors[] = {
	/* VGA and less video */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 605122560 * 2,//.ab = 2000000000,    // 602603520 * 2,
		.ib = 756403200 * 2,//.ib = 2000000000,    // 753254400 * 2,
	},
};

static struct msm_bus_vectors mdp_720p_vectors[] = {
	/* 720p and less video */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 660418560 * 2,//.ab = 2000000000,    // 602603520 * 2,
		.ib = 825523200 * 2,//.ib = 2000000000,    // 753254400 * 2,
	},
};

static struct msm_bus_vectors mdp_1080p_vectors[] = {
	/* 1080p and less video */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 764098560 * 2,//.ab = 2000000000,    // 602603520 * 2,
		.ib = 955123200 * 2,//.ib = 2000000000,    // 753254400 * 2,
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
	.mdp_bus_scale_table = &mdp_bus_scale_pdata,
	.mdp_rev = MDP_REV_44,
#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
	.mem_hid = BIT(ION_CP_MM_HEAP_ID),
#else
	.mem_hid = MEMTYPE_EBI1,
#endif
	/* for early backlight on for APQ8064 */
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
#endif /* CONFIG_LGE_KCAL */

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
#ifndef CONFIG_MACH_LGE
#define HDMI_CEC_VAR_GPIO	69
#endif /* CONFIG_MACH_LGE */
#define HDMI_DDC_CLK_GPIO	70
#define HDMI_DDC_DATA_GPIO	71
#define HDMI_HPD_GPIO		72

#define DSV_ONBST	57 // DSV_EN

static bool dsi_power_on = false;
static int mipi_dsi_panel_power(int on)
{
	static struct regulator *reg_l2, *reg_lvs6;
    static int lcd_reset_gpio;
    static int dsv_load_en_gpio;
    static int dsv_en_gpio;
	int rc;

	pr_debug("%s: state : %d\n", __func__, on);

    if(lge_get_board_revno() < HW_REV_C){
        dsv_load_en_gpio = PM8921_GPIO_PM_TO_SYS(22);
        dsv_en_gpio = DSV_ONBST;
    }else{
        dsv_load_en_gpio = PM8921_GPIO_PM_TO_SYS(23);
        dsv_en_gpio = PM8921_GPIO_PM_TO_SYS(22);
    }

	if (!dsi_power_on) /* LCD initial start (power side) */
       {
	       printk(KERN_INFO "[LCD][DEBUG] %s: mipi lcd power initial\n", __func__);
		reg_lvs6 = regulator_get(&msm_mipi_dsi1_device.dev, "dsi_iovcc");
		if (IS_ERR(reg_lvs6)) {
			pr_err("could not get 8921_lvs6, rc = %ld\n",
				 PTR_ERR(reg_lvs6));
			return -ENODEV;
		}

		reg_l2 = regulator_get(&msm_mipi_dsi1_device.dev, "dsi_vdda");
		if (IS_ERR(reg_l2)) {
			pr_err("could not get 8921_l2, rc = %ld\n",
				PTR_ERR(reg_l2));
			return -ENODEV;
		}

		rc = regulator_set_voltage(reg_l2, 1200000, 1200000);
		if (rc) {
			pr_err("set_voltage l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		/* LGE_CHANGE_S
		 * jamin.koo@lge.com, 2012.09.04
		 * Enable DSV_LOAD_EN
		 */
		rc = gpio_request(dsv_load_en_gpio, "dsv_load_en");
		if (rc) {
			pr_err("request gpio 22 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		/* LGE_CHANGE_E */

		rc = gpio_request(dsv_en_gpio, "DSV_ONBST");
		if (rc) {
			pr_err(KERN_INFO "%s: DSV_ONBST Request Fail, rc=%d\n", __func__, rc);
		}
        if(lge_get_board_revno() < HW_REV_C){
		gpio_tlmm_config(GPIO_CFG(DSV_ONBST, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
        }

        mdelay(50);

		lcd_reset_gpio= PM8921_GPIO_PM_TO_SYS(42);

		rc = gpio_request(lcd_reset_gpio, "disp_rst_n");
		if (rc) {
			pr_err("request gpio 42 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		dsi_power_on = true;

	}

	if (on) /* LCD on start (power side) */
       {
              printk(KERN_INFO "[LCD][DEBUG] %s: lcd power on status (status=%d)\n", __func__, on);

		/* LCD RESET LOW */
		gpio_direction_output(lcd_reset_gpio, 0);

		rc = regulator_set_optimum_mode(reg_l2, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}

		rc = regulator_enable(reg_lvs6); // IOVCC
		if (rc) {
			pr_err("enable lvs6 failed, rc=%d\n", rc);
			return -ENODEV;
		}
              gpio_set_value_cansleep(dsv_load_en_gpio, 1); // Turn on the DSV_LOAD_EN switch
              mdelay(20);

              gpio_set_value_cansleep(dsv_en_gpio,1);
		mdelay(20);   /* mdelay(3); */

		/* LCD RESET LOW */
		gpio_direction_output(lcd_reset_gpio, 0);
		mdelay(10);

		rc = regulator_enable(reg_l2);  // DSI
		if (rc) {
			pr_err("enable l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		mdelay(1);    /* udelay(100); */

		/* LCD RESET HIGH */
		gpio_direction_output(lcd_reset_gpio, 1);
		mdelay(10);

	}
	else /* LCD off start (power side) */
	{
              printk(KERN_INFO "[LCD][DEBUG] %s: lcd power off (status=%d)\n", __func__, on);

		rc = regulator_disable(reg_l2);	//DSI
		if (rc) {
			pr_err("disable reg_l2  failed, rc=%d\n", rc);
			return -ENODEV;
		}
		mdelay(1);   /* mdelay(100); */

		/* LCD RESET LOW */
		gpio_direction_output(lcd_reset_gpio, 0);
		mdelay(10);

		gpio_set_value_cansleep(dsv_en_gpio, 0);
		gpio_set_value_cansleep(dsv_load_en_gpio, 0); //Turn off the DSV_LOAD_EN switch
		mdelay(20);
		
		rc = regulator_disable(reg_lvs6);	// IOVCC
		if (rc) {
			pr_err("disable lvs6 failed, rc=%d\n", rc);
			return -ENODEV;
		}
              mdelay(10);


		rc = regulator_set_optimum_mode(reg_l2, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}
	}
	
	return 0;
}

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
	} else {
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

#endif  /* LGE Not Used */

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
#ifndef CONFIG_MACH_LGE
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
#else
	return 0;
#endif /* CONFIG_MACH_LGE */
}

#if defined (CONFIG_BACKLIGHT_LM3530)
extern void lm3530_lcd_backlight_set_level( int level);
#elif defined (CONFIG_BACKLIGHT_LM3533)
extern void lm3533_lcd_backlight_set_level( int level);
#elif defined (CONFIG_BACKLIGHT_LM3630)
extern void lm3630_lcd_backlight_set_level( int level);
#endif /* CONFIG_BACKLIGHT_LMXXXX */

static int mipi_lgit_backlight_level(int level, int max, int min)
{
#if defined (CONFIG_BACKLIGHT_LM3530)
	lm3530_lcd_backlight_set_level(level);
#elif defined (CONFIG_BACKLIGHT_LM3533)
	lm3533_lcd_backlight_set_level(level);
#elif defined (CONFIG_BACKLIGHT_LM3630)
	lm3630_lcd_backlight_set_level(level);
#endif
	return 0;
}

static char exit_sleep_mode             [2] = {0x11,0x00};

#if defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_FHD_INVERSE_PT)
static char set_address_mode            [2] = {0x36,0x40};
#endif /* CONFIG_FB_MSM_MIPI_LGIT_VIDEO_FHD_INVERSE_PT */

static char mcap_setting                [2] = {0xB0, 0x04};
		/* LGE_CHANGE_S
		 * jihee.moon@lge.com, 2012.09.10
		 * Change the initial set for Rev.A
		 */
static char nop_command                 [2] = {0x00, 0x00};

static char interface_setting           [7] = {0xB3, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00};

static char dsi_ctrl                    [3] = {0xB6, 0x3A, 0xD3};

static char display_setting_1          [35] = {
					0xC1,
					0x84, 0x60, 0x50, 0x00, 0x00,
					0x00, 0x00, 0x00, 0x00, 0x0C,
					0x01, 0x58, 0x73, 0xAE, 0x31,
					0x20, 0x06, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x00, 0x10, 0x10,
					0x10, 0x10, 0x00, 0x00, 0x00,
					0x22, 0x02, 0x02, 0x00
					};

static char display_setting_2_old           [8] = {
					0xC2,
					0x30, 0xF7, 0x80, 0x0A, 0x08,
					0x00, 0x00
					};
static char display_setting_2           [8] = {
					0xC2,
					0x31, 0xF7, 0x80, 0x0A, 0x08,
					0x00, 0x00
					};

static char source_timing_setting      [23] = {
					0xC4,
					0x70, 0x00, 0x00, 0x00, 0x07,
					0x05, 0x05, 0x09, 0x09, 0x0C,
					0x06, 0x00, 0x00, 0x00, 0x00,
					0x07, 0x05, 0x05, 0x09, 0x09,
					0x0C, 0x06
					};

static char ltps_timing_setting        [41] = {
					0xC6,
					0x00, 0x69, 0x00, 0x69, 0x00,
					0x69, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x69, 0x00, 0x69, 0x00,
					0x69, 0x10, 0x19, 0x07, 0x00,
					0x01, 0x00, 0x69, 0x00, 0x69,
					0x00, 0x69, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x69, 0x00, 0x69,
					0x00, 0x69, 0x10, 0x19, 0x07
					};

static char gamma_setting_a_old            [25] = {
					0xC7,
					0x00, 0x09, 0x14, 0x26, 0x32,
					0x49, 0x3B, 0x52, 0x5F, 0x67,
					0x6B, 0x70, 0x00, 0x09, 0x14,
					0x26, 0x32, 0x49, 0x3B, 0x52,
					0x5F, 0x67, 0x6B, 0x70
					};
static char gamma_setting_a            [25] = {
					0xC7,
					0x00, 0x09, 0x14, 0x26, 0x31,
					0x48, 0x3B, 0x52, 0x5F, 0x67,
					0x6B, 0x70, 0x00, 0x09, 0x14,
					0x26, 0x31, 0x48, 0x3B, 0x52,
					0x5F, 0x67, 0x6B, 0x70
					};

static char gamma_setting_b_old           [25] = {
					0xC8,
					0x00, 0x09, 0x14, 0x26, 0x32,
					0x49, 0x3B, 0x52, 0x5F, 0x67,
					0x6B, 0x70, 0x00, 0x09, 0x14,
					0x26, 0x32, 0x49, 0x3B, 0x52,
					0x5F, 0x67, 0x6B, 0x70
					};
static char gamma_setting_b           [25] = {
					0xC8,
					0x00, 0x09, 0x14, 0x26, 0x31,
					0x48, 0x3B, 0x52, 0x5F, 0x67,
					0x6B, 0x70, 0x00, 0x09, 0x14,
					0x26, 0x31, 0x48, 0x3B, 0x52,
					0x5F, 0x67, 0x6B, 0x70
					};

static char gamma_setting_c_old            [25] = {
					0xC9,
					0x00, 0x09, 0x14, 0x26, 0x32,
					0x49, 0x3B, 0x52, 0x5F, 0x67,
					0x6B, 0x70, 0x00, 0x09, 0x14,
					0x26, 0x32, 0x49, 0x3B, 0x52,
					0x5F, 0x67, 0x6B, 0x70
					};
static char gamma_setting_c            [25] = {
					0xC9,
					0x00, 0x09, 0x14, 0x26, 0x31,
					0x48, 0x3B, 0x52, 0x5F, 0x67,
					0x6B, 0x70, 0x00, 0x09, 0x14,
					0x26, 0x31, 0x48, 0x3B, 0x52,
					0x5F, 0x67, 0x6B, 0x70
					};

static char panel_interface_ctrl        [2] = {0xCC, 0x09};

static char pwr_setting_chg_pump       [15] = {
					0xD0,
					0x00, 0x00, 0x19, 0x18, 0x99,
					0x99, 0x19, 0x01, 0x89, 0x00,
					0x55, 0x19, 0x99, 0x01
					};

static char pwr_setting_internal_pwr   [27] = {
					0xD3,
					0x1B, 0x33, 0xBB, 0xCC, 0xC4,
					0x33, 0x33, 0x33, 0x00, 0x01,
					0x00, 0xA0, 0xD8, 0xA0, 0x0D,
					0x37, 0x33, 0x44, 0x22, 0x70,
					0x02, 0x37, 0x03, 0x3D, 0xBF,
					0x00
					};

static char vcom_setting                [8] = {
					0xD5,
					0x06, 0x00, 0x00, 0x01, 0x39,
					0x01, 0x39
					};

static char vcom_setting_old            [8] = {
					0xD5,
					0x06, 0x00, 0x00, 0x01, 0x4A,
					0x01, 0x4A
					};

static char vcom_setting_for_suspend    [8] = {
                    0xD5,
                    0x06, 0x00, 0x00, 0x00, 0x48,
                    0x00, 0x48
};

static char display_on                  [2] = {0x29,0x00};

static char display_off                 [2] = {0x28,0x00};

static char enter_sleep_mode            [2] = {0x10,0x00};

static char deep_standby_mode           [2] = {0xB1,0x01};

/*     static char vsync_setting               [4] = {0xC3, 0x01, 0x00, 0x00};      */

#if defined(CONFIG_LGE_R63311_COLOR_ENGINE)
static char color_enhancement          [33] = {
                                   0xCA,
                                   0x01, 0x70, 0x90, 0xA0, 0xB0,
                                   0x98, 0x90, 0x90, 0x3F, 0x3F,
                                   0x80, 0x78, 0x08, 0x38, 0x08,
                                   0x3F, 0x08, 0x90, 0x0C, 0x0C,
                                   0x0A, 0x06, 0x04, 0x04, 0x00,
                                   0xC8, 0x10, 0x10, 0x3F, 0x3F,
                                   0x3F, 0x3F
                                   };
static char color_enhancement_off      [33] = {
                                   0xCA,
                                   0x00, 0x80, 0xDC, 0xF0, 0xDC,
                                   0xBE, 0xA5, 0xDC, 0x14, 0x20,
                                   0x80, 0x8C, 0x0A, 0x4A, 0x37,
                                   0xA0, 0x55, 0xF8, 0x0C, 0x0C,
                                   0x20, 0x10, 0x20, 0x20, 0x18,
                                   0xE8, 0x10, 0x10, 0x3F, 0x3F,
                                   0x3F, 0x3F
                                   };

static char auto_contrast               [7] = {
                                   0xD8,
                                   0x00, 0x80, 0x80, 0x40, 0x42,
                                   0x55
                                   };

static char auto_contrast_off           [7] = {
                                   0xD8,
                                   0x00, 0x80, 0x80, 0x40, 0x42,
                                   0x55
                                   };

static char sharpening_control          [3] = {0xDD, 0x01, 0x95};

static char sharpening_control_off      [3] = {0xDD, 0x20, 0x45};
#endif // color engine apply
#if defined(CONFIG_LGE_R63311_BACKLIGHT_CABC)
static char pwm_set                     [8] = {0xCE, 0x00, 0x01, 0x00, 0xC1, 0x00, 0x00, 0x00};
static char write_display_brightness    [3] = {0x51, 0x00, 0xFF};
static char write_ctrl_display          [2] = {0x53, 0x24};
static char write_cabc_minimum_brightness [3] = {0x5E, 0x00, 0x00};
#if 0
static char write_cabc_ui_on         [2] = {0x55, 0x01};
#endif
#if 0
static char write_cabc_movie_on      [2] = {0x55, 0x03};
#endif
#if 1
static char write_cabc_still_on      [2] = {0x55, 0x02};
#endif
static char write_cabc_off           [2] = {0x55, 0x00};

static char backlight_ctrl_ui          [8] = {0xBA, 0x00, 0x3F, 0x04, 0x40, 0x9F, 0x1F, 0xD7};
static char backlight_ctrl_movie_still [8] = {0xB9, 0x00, 0x02, 0x18, 0x18, 0x9F, 0x1F, 0x80};
static char backlight_ctrl_common      [26]= {0xB8,
                                       0x18, 0x80, 0x18, 0x18,
                                       0xCF, 0x1F, 0x00, 0x0C,
                                       0x0C, 0x6C, 0x0C, 0x6C,
                                       0x0C, 0x0C, 0x12, 0xDA,
                                       0x6D, 0xFF, 0xFF, 0x10,
                                       0xB3, 0xFB, 0xFF, 0xFF,
                                       0xFF};
#endif // CABC apply

/* initialize device */
static struct dsi_cmd_desc lgit_power_on_set_1_old[] = {
	// Display Initial Set
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(mcap_setting), mcap_setting},
	{DTYPE_DCS_WRITE,  1, 0, 0, 0, sizeof(nop_command), nop_command},
	{DTYPE_DCS_WRITE,  1, 0, 0, 0, sizeof(nop_command), nop_command},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(interface_setting), interface_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(dsi_ctrl), dsi_ctrl},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(display_setting_1), display_setting_1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(display_setting_2_old) ,display_setting_2_old},
/*     {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(vsync_setting), vsync_setting}, */
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(source_timing_setting), source_timing_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(ltps_timing_setting), ltps_timing_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(gamma_setting_a_old), gamma_setting_a_old},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(gamma_setting_b_old), gamma_setting_b_old},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(gamma_setting_c_old), gamma_setting_c_old},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(panel_interface_ctrl), panel_interface_ctrl},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(pwr_setting_chg_pump), pwr_setting_chg_pump},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(pwr_setting_internal_pwr), pwr_setting_internal_pwr},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(vcom_setting_old), vcom_setting_old},
       {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(vcom_setting_old), vcom_setting_old},
#if defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_FHD_INVERSE_PT)
	{DTYPE_DCS_WRITE1, 1, 0, 0, 20, sizeof(set_address_mode),set_address_mode},
#if defined(CONFIG_LGE_R63311_BACKLIGHT_CABC)
       {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(pwm_set), pwm_set},
       {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(write_ctrl_display), write_ctrl_display},
       {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(write_cabc_still_on), write_cabc_still_on},
       {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(write_cabc_minimum_brightness), write_cabc_minimum_brightness},
#endif // CABC apply
#endif /* CONFIG_FB_MSM_MIPI_LGIT_VIDEO_FHD_INVERSE_PT */
#if defined(CONFIG_LGE_R63311_COLOR_ENGINE)
#if defined(CONFIG_LGE_R63311_BACKLIGHT_CABC)
       {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(backlight_ctrl_movie_still), backlight_ctrl_movie_still},
       {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(backlight_ctrl_ui), backlight_ctrl_ui},
       {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(backlight_ctrl_common), backlight_ctrl_common},
#endif // CABC apply
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(color_enhancement), color_enhancement},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(auto_contrast), auto_contrast},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(sharpening_control), sharpening_control},
#endif // color engine apply
};
static struct dsi_cmd_desc lgit_power_on_set_1[] = {
	// Display Initial Set
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(mcap_setting), mcap_setting},
	{DTYPE_DCS_WRITE,  1, 0, 0, 0, sizeof(nop_command), nop_command},
	{DTYPE_DCS_WRITE,  1, 0, 0, 0, sizeof(nop_command), nop_command},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(interface_setting), interface_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(dsi_ctrl), dsi_ctrl},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(display_setting_1), display_setting_1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(display_setting_2) ,display_setting_2},
/*     {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(vsync_setting), vsync_setting}, */
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(source_timing_setting), source_timing_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(ltps_timing_setting), ltps_timing_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(gamma_setting_a), gamma_setting_a},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(gamma_setting_b), gamma_setting_b},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(gamma_setting_c), gamma_setting_c},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(panel_interface_ctrl), panel_interface_ctrl},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(pwr_setting_chg_pump), pwr_setting_chg_pump},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(pwr_setting_internal_pwr), pwr_setting_internal_pwr},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(vcom_setting), vcom_setting},
       {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(vcom_setting), vcom_setting},
#if defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_FHD_INVERSE_PT)
	{DTYPE_DCS_WRITE1, 1, 0, 0, 20, sizeof(set_address_mode),set_address_mode},
#if defined(CONFIG_LGE_R63311_BACKLIGHT_CABC)
       {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(pwm_set), pwm_set},
       {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(write_ctrl_display), write_ctrl_display},
       {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(write_cabc_still_on), write_cabc_still_on},
       {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(write_cabc_minimum_brightness), write_cabc_minimum_brightness},
#endif // CABC apply
#endif /* CONFIG_FB_MSM_MIPI_LGIT_VIDEO_FHD_INVERSE_PT */
#if defined(CONFIG_LGE_R63311_COLOR_ENGINE)
#if defined(CONFIG_LGE_R63311_BACKLIGHT_CABC)
       {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(backlight_ctrl_movie_still), backlight_ctrl_movie_still},
       {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(backlight_ctrl_ui), backlight_ctrl_ui},
       {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(backlight_ctrl_common), backlight_ctrl_common},
#endif // CABC apply
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(color_enhancement), color_enhancement},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(auto_contrast), auto_contrast},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(sharpening_control), sharpening_control},
#endif // color engine apply
};

static struct dsi_cmd_desc lgit_power_on_set_2[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(display_on), display_on},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(exit_sleep_mode), exit_sleep_mode},
};
#if defined(CONFIG_LGE_R63311_BACKLIGHT_CABC)
static struct dsi_cmd_desc lgit_power_on_set_3[] = {
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(write_display_brightness), write_display_brightness},
};
#endif //CONFIG_LGE_R63311_BACKLIGHT_CABC
#if defined(CONFIG_LGIT_COLOR_ENGINE_SWITCH)
static struct dsi_cmd_desc lgit_color_engine_on[3] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(color_enhancement), color_enhancement},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(auto_contrast), auto_contrast},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(sharpening_control), sharpening_control},
};

static struct dsi_cmd_desc lgit_color_engine_off[3] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(color_enhancement_off), color_enhancement_off},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(auto_contrast_off), auto_contrast_off},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(sharpening_control_off), sharpening_control_off},
};
#endif //CONFIG_LGIT_COLOR_ENGINE_SWITCH

static struct dsi_cmd_desc lgit_power_off_set[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(vcom_setting_for_suspend), vcom_setting_for_suspend},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 20, sizeof(vcom_setting_for_suspend), vcom_setting_for_suspend},
	{DTYPE_DCS_WRITE, 1, 0, 0, 20, sizeof(display_off), display_off},
#if defined(CONFIG_LGE_R63311_BACKLIGHT_CABC)
       {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(write_cabc_off), write_cabc_off},
#endif // CABC apply
	{DTYPE_DCS_WRITE, 1, 0, 0, 100, sizeof(enter_sleep_mode), enter_sleep_mode},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 40, sizeof(deep_standby_mode), deep_standby_mode}
};
/* LGE_CHANGE_E */

static struct dsi_cmd_desc lgit_shutdown_set[] = {
       {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(vcom_setting_for_suspend), vcom_setting_for_suspend},
       {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(vcom_setting_for_suspend), vcom_setting_for_suspend},
       {DTYPE_DCS_WRITE, 1, 0, 0, 0,  sizeof(display_off), display_off},
};

static struct msm_panel_common_pdata mipi_lgit_pdata = {
	.backlight_level = mipi_lgit_backlight_level,
       .power_on_set_1_old = lgit_power_on_set_1_old,
       .power_on_set_size_1_old = ARRAY_SIZE(lgit_power_on_set_1_old),
	.power_on_set_1 = lgit_power_on_set_1,
	.power_on_set_size_1 = ARRAY_SIZE(lgit_power_on_set_1),
	.power_on_set_2 = lgit_power_on_set_2,
	.power_on_set_size_2 = ARRAY_SIZE(lgit_power_on_set_2),
#if defined(CONFIG_LGE_R63311_BACKLIGHT_CABC)
	.power_on_set_3 = lgit_power_on_set_3,
	.power_on_set_size_3 = ARRAY_SIZE(lgit_power_on_set_3),
#endif //CABC apply
#if defined(CONFIG_LGIT_COLOR_ENGINE_SWITCH)
	.color_engine_on = lgit_color_engine_on,
	.color_engine_on_size = ARRAY_SIZE(lgit_color_engine_on),
	.color_engine_off = lgit_color_engine_off,
	.color_engine_off_size = ARRAY_SIZE(lgit_color_engine_off),
#endif //CONFIG_LGIT_COLOR_ENGINE_SWITCH
       .power_off_set_1 = lgit_power_off_set,
       .power_off_set_size_1 = ARRAY_SIZE(lgit_power_off_set),
       .power_off_set_2 = lgit_shutdown_set,
       .power_off_set_size_2 = ARRAY_SIZE(lgit_shutdown_set),
};

static struct platform_device mipi_dsi_lgit_panel_device = {
	.name = "mipi_lgit",
	.id = 0,
	.dev = {
		.platform_data = &mipi_lgit_pdata,
	}
};

static struct platform_device *gv_panel_devices[] __initdata = {
#if defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_WXGA_PT) || defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_FHD_PT) ||\
	defined(CONFIG_FB_MSM_MIPI_LGIT_VIDEO_FHD_INVERSE_PT)
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
#endif /* CONFIG_MACH_LGE*/

#ifdef CONFIG_FB_MSM_WRITEBACK_MSM_PANEL
	platform_device_register(&wfd_panel_device);
	platform_device_register(&wfd_device);
#endif /* CONFIG_FB_MSM_WRITEBACK_MSM_PANEL */
	platform_add_devices(gv_panel_devices,
			ARRAY_SIZE(gv_panel_devices));

#ifndef CONFIG_MACH_LGE
	if (machine_is_apq8064_liquid())
		platform_device_register(&mipi_dsi2lvds_bridge_device);
	if (machine_is_apq8064_mtp())
		platform_device_register(&mipi_dsi_toshiba_panel_device);
	if (machine_is_mpq8064_dtv())
		platform_device_register(&lvds_frc_panel_device);
#endif /* CONFIG_MACH_LGE */

	msm_fb_register_device("mdp", &mdp_pdata);
#ifndef CONFIG_MACH_LGE
	msm_fb_register_device("lvds", &lvds_pdata);
#endif /* CONFIG_MACH_LGE */
	msm_fb_register_device("mipi_dsi", &mipi_dsi_pdata);
	platform_device_register(&hdmi_msm_device);
	msm_fb_register_device("dtv", &dtv_pdata);
}

#define I2C_SURF 1
#define I2C_FFA  (1 << 1)
#define I2C_RUMI (1 << 2)
#define I2C_SIM  (1 << 3)
#define I2C_LIQUID (1 << 4)

struct i2c_registry {
	u8                     machs;
	int                    bus;
	struct i2c_board_info *info;
	int                    len;
};

#if defined(CONFIG_LGE_BACKLIGHT_CABC)
#define PWM_SIMPLE_EN 0xA0
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
#if defined(CONFIG_LGE_BACKLIGHT_CABC)
	.max_current = 0x17 | PWM_BRIGHTNESS,
#else
	.max_current = 0x17,
#endif
	.min_brightness = 0x01,
	.max_brightness = 0x71,
	
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
	.factory_brightness = 0x78,
};
#elif defined(CONFIG_BACKLIGHT_LM3630)
static struct backlight_platform_data lm3630_data = {
	.gpio = PM8921_GPIO_PM_TO_SYS(24),
#if defined(CONFIG_LGE_BACKLIGHT_CABC)
	.max_current = 0x17 | PWM_SIMPLE_EN,
#else
	.max_current = 0x17,
#endif
	.min_brightness = 0x05,
	.max_brightness = 0xFF,
	.default_brightness = 0x9C,
	.factory_brightness = 0x78,
};
#endif
static struct i2c_board_info msm_i2c_backlight_info[] = {
	{

#if defined(CONFIG_BACKLIGHT_LM3530)
		I2C_BOARD_INFO("lm3530", 0x38),
		.platform_data = &lm3530_data,
#elif defined(CONFIG_BACKLIGHT_LM3533)
		I2C_BOARD_INFO("lm3533", 0x38),
		.platform_data = &lm3533_data,
#elif defined(CONFIG_BACKLIGHT_LM3630)
		I2C_BOARD_INFO("lm3630", 0x38),
		.platform_data = &lm3630_data,
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
	else if (machine_is_apq8064_gk())
		mach_mask = I2C_FFA;
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
/**
 * Set MDP clocks to high frequency to avoid DSI underflow
 * when using high resolution 1200x1920 WUXGA panels
 */
 #ifndef CONFIG_MACH_LGE
static void set_mdp_clocks_for_wuxga(void)
{
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
#endif /* CONFIG_MACH_LGE */
