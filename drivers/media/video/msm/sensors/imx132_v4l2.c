/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
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
#include "msm_sensor.h"
#include "msm_camera_i2c_mux.h"
#include <linux/mfd/pm8xxx/pm8921.h>
#include "../../../../../arch/arm/mach-msm/lge/gvar/board-gvar.h"
#include <mach/board_lge.h>
#define SENSOR_NAME "imx132"
#define PLATFORM_DRIVER_NAME "msm_camera_imx132"
#define imx132_obj imx132_##obj

#define MSM_VT_PWR_EN PM8921_GPIO_PM_TO_SYS(35)  //VT_PWR_EN
/*                                                                          */
#define MSM_CAM2_RST_EN PM8921_GPIO_PM_TO_SYS(28)	//VTCAM_RST_N
/*                                                                        */

/*                                                                */
#define CAMERA_DEBUG 1
#define LDBGE(fmt,args...) printk(KERN_EMERG "[CAM/E][ERR] "fmt,##args)
#if (CAMERA_DEBUG)
#define LDBGI(fmt,args...) printk(KERN_EMERG "[CAM/I] "fmt,##args)
#else
#define LDBGI(args...) do {} while(0)
#endif
/*                                                                */

/*                                                                         */
#ifdef CONFIG_MACH_LGE
#if defined (CONFIG_FB_MSM_MIPI_LGIT_VIDEO_FHD_INVERSE_PT) || defined (CONFIG_FB_MSM_MIPI_LGIT_VIDEO_FHD_INVERSE_PT_PANEL)
#define LGIT_COLOR_ENGINE_SWITCH

#ifdef LGIT_COLOR_ENGINE_SWITCH
extern int mipi_lgit_lcd_color_engine_on(void);
#endif
#endif
#endif
/*                                                                         */
    /*                                                                                             */
extern int sub_cam_id_for_keep_screen_on;
    /*                                                                                             */

int32_t imx132_sensor_power_up(struct msm_sensor_ctrl_t *s_ctrl);
int32_t imx132_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl);

static struct pm_gpio gpio35_param = {
		.direction      = PM_GPIO_DIR_OUT,
		.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
		.output_value   = 1,
		.pull           = PM_GPIO_PULL_NO,
		.vin_sel	= PM_GPIO_VIN_S4,
		.out_strength   = PM_GPIO_STRENGTH_MED,
		.function       = PM_GPIO_FUNC_NORMAL,
};
/*                                                                          */
static struct pm_gpio gpio28_param = {
		.direction      = PM_GPIO_DIR_OUT,
		.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
		.output_value   = 1,
		.pull           = PM_GPIO_PULL_NO,
		.vin_sel	= PM_GPIO_VIN_S4,
		.out_strength   = PM_GPIO_STRENGTH_MED,
		.function       = PM_GPIO_FUNC_NORMAL,
};
/*                                                                        */
static struct msm_cam_clk_info imx132_cam_clk_info[] = {
	{"cam_clk", MSM_SENSOR_MCLK_24HZ},
};

DEFINE_MUTEX(imx132_mut);
static struct msm_sensor_ctrl_t imx132_s_ctrl;

static struct msm_camera_i2c_reg_conf imx132_start_settings[] = {
	{0x0100, 0x01},
};

static struct msm_camera_i2c_reg_conf imx132_stop_settings[] = {
	{0x0100, 0x00},
};

static struct msm_camera_i2c_reg_conf imx132_groupon_settings[] = {
	{0x104, 0x01},
};

static struct msm_camera_i2c_reg_conf imx132_groupoff_settings[] = {
	{0x104, 0x00},
};


static struct msm_camera_i2c_reg_conf imx132_prev_settings[] = {
#if 1
//reg_D Preview Full resolution 2.38M, 1976x1200 - 30fps
	/* read out direction */
	{0x0101, 0x03},
	/* PLL setting */
	{0x0305, 0x04},
	{0x0307, 0x87},
	{0x30A4, 0x02},
	{0x303C, 0x4B},
	/* mode setting */
	{0x0340, 0x04},
	{0x0341, 0xD2},
//	{0x0341, 0xCA},
	{0x0342, 0x08},
	{0x0343, 0xC8},
	{0x0344, 0x00},
	{0x0345, 0x00},
	{0x0346, 0x00},
	{0x0347, 0x00},
	{0x0348, 0x07},
	{0x0349, 0xB7},
	{0x034A, 0x04},
	{0x034B, 0xAF},
	{0x034C, 0x07},
	{0x034D, 0xB8},
	{0x034E, 0x04},
	{0x034F, 0xB0},
	{0x0381, 0x01},
	{0x0383, 0x01},
	{0x0385, 0x01},
	{0x0387, 0x01},
	{0x303D, 0x10},
	{0x303E, 0x5A},
	{0x3041, 0x97},
	{0x3048, 0x00},
	{0x304C, 0x2F},
	{0x304D, 0x02},
	{0x306A, 0x10},
	{0x309B, 0x00},
	{0x309E, 0x41},
	{0x30A0, 0x10},
	{0x30A1, 0x0B},
	{0x30D5, 0x00},
	{0x30D6, 0x00},
	{0x30D7, 0x00},
	{0x30DE, 0x00},
	{0x3102, 0x0C},
	{0x3103, 0x33},
	{0x3104, 0x18},
	{0x3106, 0x65},
	{0x315C, 0x3D},
	{0x315D, 0x3C},
	{0x316E, 0x3E},
	{0x316F, 0x3D},
	{0x3318, 0x61},
	{0x3348, 0xE0},
#endif
};


static struct msm_camera_i2c_reg_conf imx132_recommend_settings[] = {
	/* global setting */
	{0x3087, 0x53},
	{0x308B, 0x5A},
	{0x3094, 0x11},
	{0x309D, 0xA4},
	{0x30AA, 0x01},
	{0x30C6, 0x00},
	{0x30C7, 0x00},
	{0x3118, 0x2F},
	{0x312A, 0x00},
	{0x312B, 0x0B},
	{0x312C, 0x0B},
	{0x312D, 0x13},
	/* black level setting */
	{0x3032, 0x40},
/*           
                                                       
                                  
 */
	{0x0101, 0x03},
};

static struct v4l2_subdev_info imx132_subdev_info[] = {
	{
	.code   = V4L2_MBUS_FMT_SBGGR10_1X10,
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt    = 1,
	.order    = 0,
	},
	/* more can be supported, to be added later */
};

static struct msm_camera_i2c_conf_array imx132_init_conf[] = {
	{&imx132_recommend_settings[0],
	ARRAY_SIZE(imx132_recommend_settings), 0, MSM_CAMERA_I2C_BYTE_DATA}
};

static struct msm_camera_i2c_conf_array imx132_confs[] = {
	{&imx132_prev_settings[0],
	ARRAY_SIZE(imx132_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_sensor_output_info_t imx132_dimensions[] = {
#if 1
	{
		.x_output = 0x7B8,
		.y_output = 0x4B0,
		.line_length_pclk = 0x8C8,
		.frame_length_lines = 0x4D2,
//		.frame_length_lines = 0x4CA,
		.vt_pixel_clk = 81000000,
		.op_pixel_clk = 81000000,
	},
#endif
};

static struct msm_sensor_output_reg_addr_t imx132_reg_addr = {
	.x_output = 0x34C,
	.y_output = 0x34E,
	.line_length_pclk = 0x342,
	.frame_length_lines = 0x340,
};

static enum msm_camera_vreg_name_t imx132_veg_seq[] = {
	CAM_VIO,
	CAM_VDIG,
};

static struct msm_sensor_id_info_t imx132_id_info = {
	.sensor_id_reg_addr = 0x0000,
	.sensor_id = 0x132,
};

static struct msm_sensor_exp_gain_info_t imx132_exp_gain_info = {
	.coarse_int_time_addr = 0x202,
	.global_gain_addr = 0x204,
	.vert_offset = 3,
};


static const struct i2c_device_id imx132_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&imx132_s_ctrl},
	{ }
};

static struct i2c_driver imx132_i2c_driver = {
	.id_table = imx132_i2c_id,
	.probe  = msm_sensor_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client imx132_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

static int __init msm_sensor_init_module(void)
{
	return i2c_add_driver(&imx132_i2c_driver);
}

static struct v4l2_subdev_core_ops imx132_subdev_core_ops = {
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};
static struct v4l2_subdev_video_ops imx132_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops imx132_subdev_ops = {
	.core = &imx132_subdev_core_ops,
	.video  = &imx132_subdev_video_ops,
};

static struct msm_sensor_fn_t imx132_func_tbl = {
	.sensor_start_stream = msm_sensor_start_stream,
	.sensor_stop_stream = msm_sensor_stop_stream,
	.sensor_group_hold_on = msm_sensor_group_hold_on,
	.sensor_group_hold_off = msm_sensor_group_hold_off,
	.sensor_set_fps = msm_sensor_set_fps,
	.sensor_write_exp_gain = msm_sensor_write_exp_gain1,
/*                                                                      */
	.sensor_write_snapshot_exp_gain = msm_sensor_write_exp_gain1,
/*                                                                      */
	.sensor_setting = msm_sensor_setting,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = msm_sensor_config,
	.sensor_power_up = imx132_sensor_power_up,
	.sensor_power_down = imx132_sensor_power_down,
//                                                        
	.sensor_get_csi_params = msm_sensor_get_csi_params,
//                                                       
};

static struct msm_sensor_reg_t imx132_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf = imx132_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(imx132_start_settings),
	.stop_stream_conf = imx132_stop_settings,
	.stop_stream_conf_size = ARRAY_SIZE(imx132_stop_settings),
	.group_hold_on_conf = imx132_groupon_settings,
	.group_hold_on_conf_size = ARRAY_SIZE(imx132_groupon_settings),
	.group_hold_off_conf = imx132_groupoff_settings,
	.group_hold_off_conf_size =
		ARRAY_SIZE(imx132_groupoff_settings),
	.init_settings = &imx132_init_conf[0],
	.init_size = ARRAY_SIZE(imx132_init_conf),
	.mode_settings = &imx132_confs[0],
	.output_settings = &imx132_dimensions[0],
	.num_conf = ARRAY_SIZE(imx132_confs),
};

static struct msm_sensor_ctrl_t imx132_s_ctrl = {
	.msm_sensor_reg = &imx132_regs,
	.sensor_i2c_client = &imx132_sensor_i2c_client,
	.sensor_i2c_addr = 0x6C,
	.vreg_seq = imx132_veg_seq,
	.num_vreg_seq = ARRAY_SIZE(imx132_veg_seq),
	.sensor_output_reg_addr = &imx132_reg_addr,
	.sensor_id_info = &imx132_id_info,
	.sensor_exp_gain_info = &imx132_exp_gain_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
//	.csi_params = &imx132_csi_params_array[0],
	.msm_sensor_mutex = &imx132_mut,
	.sensor_i2c_driver = &imx132_i2c_driver,
	.sensor_v4l2_subdev_info = imx132_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(imx132_subdev_info),
	.sensor_v4l2_subdev_ops = &imx132_subdev_ops,
	.func_tbl = &imx132_func_tbl,
};

int32_t imx132_msm_sensor_enable_i2c_mux(struct msm_camera_i2c_conf *i2c_conf)
{
	struct v4l2_subdev *i2c_mux_sd =
		dev_get_drvdata(&i2c_conf->mux_dev->dev);
	v4l2_subdev_call(i2c_mux_sd, core, ioctl,
		VIDIOC_MSM_I2C_MUX_INIT, NULL);
	v4l2_subdev_call(i2c_mux_sd, core, ioctl,
		VIDIOC_MSM_I2C_MUX_CFG, (void *)&i2c_conf->i2c_mux_mode);
	return 0;
}

int32_t imx132_msm_sensor_disable_i2c_mux(struct msm_camera_i2c_conf *i2c_conf)
{
	struct v4l2_subdev *i2c_mux_sd =
		dev_get_drvdata(&i2c_conf->mux_dev->dev);
	v4l2_subdev_call(i2c_mux_sd, core, ioctl,
				VIDIOC_MSM_I2C_MUX_RELEASE, NULL);
	return 0;
}

int32_t imx132_sensor_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	struct msm_camera_sensor_info *data = s_ctrl->sensordata;
	CDBG("%s: %d\n", __func__, __LINE__);

#ifdef LGIT_IEF_SWITCH
	if(system_state != SYSTEM_BOOTING) {
		mipi_lgit_lcd_ief_off();
	}
#endif

	s_ctrl->reg_ptr = kzalloc(sizeof(struct regulator *)
			* data->sensor_platform_info->num_vreg, GFP_KERNEL);
	if (!s_ctrl->reg_ptr) {
		pr_err("%s: could not allocate mem for regulators\n",
			__func__);
		return -ENOMEM;
	}

	pr_err("%s: before request gpio, sensor name : %s", __func__, s_ctrl->sensordata->sensor_name);
	rc = msm_camera_request_gpio_table(data, 1);
	if (rc < 0) {
		pr_err("%s: request gpio failed\n", __func__);
		goto request_gpio_failed;
	}

	rc = gpio_request(MSM_VT_PWR_EN, "VT_PWR_EN");

	rc = gpio_request(MSM_CAM2_RST_EN, "VTCAM_RST_EN");
	if (rc) {
		LDBGE("%s: PM request gpio failed\n", __func__);
	}

	rc = msm_camera_config_vreg(&s_ctrl->sensor_i2c_client->client->dev,
			s_ctrl->sensordata->sensor_platform_info->cam_vreg,
			s_ctrl->sensordata->sensor_platform_info->num_vreg,
			s_ctrl->vreg_seq,
			s_ctrl->num_vreg_seq,
			s_ctrl->reg_ptr, 1);
	if (rc < 0) {
		LDBGE("%s: regulator on failed\n", __func__);
		goto config_vreg_failed;
	}

	rc =pm8xxx_gpio_config(MSM_VT_PWR_EN, &gpio35_param);

	rc =pm8xxx_gpio_config(MSM_CAM2_RST_EN, &gpio28_param);
	if (rc) {
		LDBGE("%s: pm8xxx_gpio_config on failed\n", __func__);
	}

	rc = gpio_direction_output(MSM_VT_PWR_EN, 0);

	usleep(5);

	rc = gpio_direction_output(MSM_VT_PWR_EN, 1);
	if (rc) {
		LDBGE("%s: gpio_direction_output enable failed\n", __func__);
	}

	rc = msm_camera_enable_vreg(&s_ctrl->sensor_i2c_client->client->dev,
			s_ctrl->sensordata->sensor_platform_info->cam_vreg,
			s_ctrl->sensordata->sensor_platform_info->num_vreg,
			s_ctrl->vreg_seq,
			s_ctrl->num_vreg_seq,
			s_ctrl->reg_ptr, 1);
	if (rc < 0) {
		LDBGE("%s: enable regulator failed\n", __func__);
		goto enable_vreg_failed;
	}

	usleep(10);

	rc = gpio_direction_output(MSM_CAM2_RST_EN, 1);

	if (s_ctrl->clk_rate != 0)
		imx132_cam_clk_info->clk_rate = s_ctrl->clk_rate;

	rc = msm_cam_clk_enable(&s_ctrl->sensor_i2c_client->client->dev,
		imx132_cam_clk_info, s_ctrl->cam_clk, ARRAY_SIZE(imx132_cam_clk_info), 1);
	if (rc < 0) {
		LDBGE("%s: clk enable failed\n", __func__);
		goto enable_clk_failed;
	}

	usleep_range(1000, 2000);
	if (data->sensor_platform_info->ext_power_ctrl != NULL)
		data->sensor_platform_info->ext_power_ctrl(1);

	if (data->sensor_platform_info->i2c_conf &&
		data->sensor_platform_info->i2c_conf->use_i2c_mux)
		imx132_msm_sensor_enable_i2c_mux(data->sensor_platform_info->i2c_conf);

	return rc;

enable_clk_failed:
	rc = gpio_direction_output(MSM_CAM2_RST_EN, 0);

enable_vreg_failed:
	msm_camera_config_vreg(&s_ctrl->sensor_i2c_client->client->dev,
		s_ctrl->sensordata->sensor_platform_info->cam_vreg,
		s_ctrl->sensordata->sensor_platform_info->num_vreg,
		s_ctrl->vreg_seq,
		s_ctrl->num_vreg_seq,
		s_ctrl->reg_ptr, 0);
config_vreg_failed:
	msm_camera_request_gpio_table(data, 0);
request_gpio_failed:
	kfree(s_ctrl->reg_ptr);
	return rc;
}

int32_t imx132_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	struct msm_camera_sensor_info *data = s_ctrl->sensordata;
	CDBG("%s\n", __func__);
	pr_err("%s\n", __func__);

  /*                                                                                             */
sub_cam_id_for_keep_screen_on = -1;
  /*                                                                                             */

/*                                                                         */
#ifdef LGIT_COLOR_ENGINE_SWITCH
      if(system_state != SYSTEM_BOOTING) {
        mipi_lgit_lcd_color_engine_on();
      }
#endif
/*                                                                         */

	if (data->sensor_platform_info->i2c_conf &&
		data->sensor_platform_info->i2c_conf->use_i2c_mux)
		imx132_msm_sensor_disable_i2c_mux(
			data->sensor_platform_info->i2c_conf);

	if (data->sensor_platform_info->ext_power_ctrl != NULL)
		data->sensor_platform_info->ext_power_ctrl(0);

	msm_cam_clk_enable(&s_ctrl->sensor_i2c_client->client->dev,
		imx132_cam_clk_info, s_ctrl->cam_clk, ARRAY_SIZE(imx132_cam_clk_info), 0);


/*                                                                   */
    usleep(5);
/*                                                                   */

	rc = gpio_direction_output(MSM_CAM2_RST_EN, 0 );

/*                                                                 */
	if(s_ctrl->reg_ptr != NULL) {
	msm_camera_enable_vreg(&s_ctrl->sensor_i2c_client->client->dev,
		s_ctrl->sensordata->sensor_platform_info->cam_vreg,
		s_ctrl->sensordata->sensor_platform_info->num_vreg,
		s_ctrl->vreg_seq,
		s_ctrl->num_vreg_seq,
		s_ctrl->reg_ptr, 0);
	msm_camera_config_vreg(&s_ctrl->sensor_i2c_client->client->dev,
		s_ctrl->sensordata->sensor_platform_info->cam_vreg,
		s_ctrl->sensordata->sensor_platform_info->num_vreg,
		s_ctrl->vreg_seq,
		s_ctrl->num_vreg_seq,
		s_ctrl->reg_ptr, 0);
		kfree(s_ctrl->reg_ptr);
	}
	else {
		// NULL!
		LDBGE("%s: No Regulator Pointer!\n", __func__);
	}
/*                                                               */

	rc = gpio_direction_output(MSM_VT_PWR_EN, 0);
	if (rc) {
		pr_err("%s: gpio_direction_output enable failed\n", __func__);
	}

	msm_camera_request_gpio_table(data, 0);

	gpio_free(MSM_VT_PWR_EN);

	gpio_free(MSM_CAM2_RST_EN);

	return 0;
}

module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("Sony 2.4MP Bayer sensor driver");
MODULE_LICENSE("GPL v2");


