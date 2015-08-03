/* Copyright (c) 2011, The Linux Foundation. All rights reserved.
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
#if defined(CONFIG_MACH_APQ8064_AWIFI)
#include "../../../../../arch/arm/mach-msm/lge/awifi/board-awifi.h"
#else
#include "../../../../../arch/arm/mach-msm/lge/L05E/board-L05E.h"
#endif
#include <mach/board_lge.h>
#define SENSOR_NAME "imx119"
#define PLATFORM_DRIVER_NAME "msm_camera_imx119"
#define imx119_obj imx119_##obj

#define MSM_CAM2_RST_EN PM8921_GPIO_PM_TO_SYS(28)	//VTCAM_RST_N
#define CAMERA_DEBUG 1
#define LDBGE(fmt,args...) printk(KERN_EMERG "[CAM/E][ERR] "fmt,##args)
#if (CAMERA_DEBUG)
#define LDBGI(fmt,args...) printk(KERN_EMERG "[CAM/I] "fmt,##args)
#else
#define LDBGI(args...) do {} while(0)
#endif

int32_t imx119_sensor_power_up(struct msm_sensor_ctrl_t *s_ctrl);
int32_t imx119_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl);

static struct pm_gpio gpio28_param = {
		.direction      = PM_GPIO_DIR_OUT,
		.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
		.output_value   = 1,
		.pull           = PM_GPIO_PULL_NO,
		.vin_sel	= PM_GPIO_VIN_S4,
		.out_strength   = PM_GPIO_STRENGTH_MED,
		.function       = PM_GPIO_FUNC_NORMAL,
};
static struct msm_cam_clk_info imx119_cam_clk_info[] = {
	{"cam_clk", MSM_SENSOR_MCLK_24HZ},
};

DEFINE_MUTEX(imx119_mut);
static struct msm_sensor_ctrl_t imx119_s_ctrl;

static struct msm_camera_i2c_reg_conf imx119_start_settings[] = {
	{0x0100, 0x01},
};

static struct msm_camera_i2c_reg_conf imx119_stop_settings[] = {
	{0x0100, 0x00},
};

static struct msm_camera_i2c_reg_conf imx119_groupon_settings[] = {
	{0x104, 0x01},
};

static struct msm_camera_i2c_reg_conf imx119_groupoff_settings[] = {
	{0x104, 0x00},
};

static struct msm_camera_i2c_reg_conf imx119_prev_settings[] = {
//                                                                       
//                                                                         
#if defined(CONFIG_MACH_APQ8064_J1D) || defined(CONFIG_MACH_APQ8064_J1KD)
	{0x0101, 0x00}, /* read out direction */
#else
	{0x0101, 0x03}, /* read out direction */
#endif
//                                                                       
//                                                                       
	{0x0340, 0x04},
	{0x0341, 0x28},
	{0x0346, 0x00},
	{0x0347, 0x00},
	{0x034A, 0x04},
	{0x034B, 0x0F},
	{0x034C, 0x05},
	{0x034D, 0x10},
	{0x034E, 0x04},
	{0x034F, 0x10},
	{0x0381, 0x01},
	{0x0383, 0x01},
	{0x0385, 0x01},
	{0x0387, 0x01},
	{0x3001, 0x00},
	{0x3016, 0x02},
	{0x3060, 0x30},
	{0x30E8, 0x00},
	{0x3301, 0x05},
	{0x308A, 0x43},
	{0x3305, 0x03},
	{0x3309, 0x05},
	{0x330B, 0x03},
	{0x330D, 0x05},
};

static struct msm_camera_i2c_reg_conf imx119_recommend_settings[] = {
	{0x0305, 0x02},
	{0x0307, 0x26},
	{0x3025, 0x0A},
	{0x302B, 0x4B},
	{0x0112, 0x0A},
	{0x0113, 0x0A},
	{0x301C, 0x02},
	{0x302C, 0x85},
	{0x303A, 0xA4},
	{0x3108, 0x25},
	{0x310A, 0x27},
	{0x3122, 0x26},
	{0x3138, 0x26},
	{0x313A, 0x27},
	{0x316D, 0x0A},
	{0x308C, 0x00},
	{0x302E, 0x8C},
	{0x302F, 0x81},
/*           
                                                        
                                  
 */	
	{0x0101, 0x03}, 
};

static struct v4l2_subdev_info imx119_subdev_info[] = {
	{
	.code   = V4L2_MBUS_FMT_SBGGR10_1X10,
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt    = 1,
	.order    = 0,
	},
	/* more can be supported, to be added later */
};

static struct msm_camera_i2c_conf_array imx119_init_conf[] = {
	{&imx119_recommend_settings[0],
	ARRAY_SIZE(imx119_recommend_settings), 0, MSM_CAMERA_I2C_BYTE_DATA}
};

static struct msm_camera_i2c_conf_array imx119_confs[] = {
	{&imx119_prev_settings[0],
	ARRAY_SIZE(imx119_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_sensor_output_info_t imx119_dimensions[] = {
	{
		.x_output = 0x510,
		.y_output = 0x410,
		.line_length_pclk = 0x570,
		.frame_length_lines = 0x432,
		.vt_pixel_clk = 45600000,
		.op_pixel_clk = 45600000,
	},
};

static struct msm_sensor_output_reg_addr_t imx119_reg_addr = {
	.x_output = 0x34C,
	.y_output = 0x34E,
	.line_length_pclk = 0x342,
	.frame_length_lines = 0x340,
};

static enum msm_camera_vreg_name_t imx119_veg_seq[] = {
	CAM_VDIG,
	CAM_VIO,
	CAM_VANA,
};

static struct msm_sensor_id_info_t imx119_id_info = {
	.sensor_id_reg_addr = 0x0,
	.sensor_id = 0x119,
};

static struct msm_sensor_exp_gain_info_t imx119_exp_gain_info = {
	.coarse_int_time_addr = 0x202,
	.global_gain_addr = 0x204,
	.vert_offset = 3,
};


static const struct i2c_device_id imx119_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&imx119_s_ctrl},
	{ }
};

static struct i2c_driver imx119_i2c_driver = {
	.id_table = imx119_i2c_id,
	.probe  = msm_sensor_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client imx119_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

static int __init msm_sensor_init_module(void)
{
	return i2c_add_driver(&imx119_i2c_driver);
}

static struct v4l2_subdev_core_ops imx119_subdev_core_ops = {
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};
static struct v4l2_subdev_video_ops imx119_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops imx119_subdev_ops = {
	.core = &imx119_subdev_core_ops,
	.video  = &imx119_subdev_video_ops,
};

static struct msm_sensor_fn_t imx119_func_tbl = {
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
	.sensor_power_up = imx119_sensor_power_up,
	.sensor_power_down = imx119_sensor_power_down,
//                                                        
	.sensor_get_csi_params = msm_sensor_get_csi_params,
//                                                       
};

static struct msm_sensor_reg_t imx119_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf = imx119_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(imx119_start_settings),
	.stop_stream_conf = imx119_stop_settings,
	.stop_stream_conf_size = ARRAY_SIZE(imx119_stop_settings),
	.group_hold_on_conf = imx119_groupon_settings,
	.group_hold_on_conf_size = ARRAY_SIZE(imx119_groupon_settings),
	.group_hold_off_conf = imx119_groupoff_settings,
	.group_hold_off_conf_size =
		ARRAY_SIZE(imx119_groupoff_settings),
	.init_settings = &imx119_init_conf[0],
	.init_size = ARRAY_SIZE(imx119_init_conf),
	.mode_settings = &imx119_confs[0],
	.output_settings = &imx119_dimensions[0],
	.num_conf = ARRAY_SIZE(imx119_confs),
};

static struct msm_sensor_ctrl_t imx119_s_ctrl = {
	.msm_sensor_reg = &imx119_regs,
	.sensor_i2c_client = &imx119_sensor_i2c_client,
	.sensor_i2c_addr = 0x6E,
	.vreg_seq = imx119_veg_seq,
	.num_vreg_seq = ARRAY_SIZE(imx119_veg_seq),
	.sensor_output_reg_addr = &imx119_reg_addr,
	.sensor_id_info = &imx119_id_info,
	.sensor_exp_gain_info = &imx119_exp_gain_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
//	.csi_params = &imx119_csi_params_array[0],
	.msm_sensor_mutex = &imx119_mut,
	.sensor_i2c_driver = &imx119_i2c_driver,
	.sensor_v4l2_subdev_info = imx119_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(imx119_subdev_info),
	.sensor_v4l2_subdev_ops = &imx119_subdev_ops,
	.func_tbl = &imx119_func_tbl,
};

int32_t imx119_msm_sensor_enable_i2c_mux(struct msm_camera_i2c_conf *i2c_conf)
{
	struct v4l2_subdev *i2c_mux_sd =
		dev_get_drvdata(&i2c_conf->mux_dev->dev);
	v4l2_subdev_call(i2c_mux_sd, core, ioctl,
		VIDIOC_MSM_I2C_MUX_INIT, NULL);
	v4l2_subdev_call(i2c_mux_sd, core, ioctl,
		VIDIOC_MSM_I2C_MUX_CFG, (void *)&i2c_conf->i2c_mux_mode);
	return 0;
}

int32_t imx119_msm_sensor_disable_i2c_mux(struct msm_camera_i2c_conf *i2c_conf)
{
	struct v4l2_subdev *i2c_mux_sd =
		dev_get_drvdata(&i2c_conf->mux_dev->dev);
	v4l2_subdev_call(i2c_mux_sd, core, ioctl,
				VIDIOC_MSM_I2C_MUX_RELEASE, NULL);
	return 0;
}

int32_t imx119_sensor_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	struct msm_camera_sensor_info *data = s_ctrl->sensordata;
	struct device *dev = NULL;
	CDBG("%s: %d\n", __func__, __LINE__);
	if (s_ctrl->sensor_device_type == MSM_SENSOR_PLATFORM_DEVICE)
		dev = &s_ctrl->pdev->dev;
	else
		dev = &s_ctrl->sensor_i2c_client->client->dev;

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


	rc = msm_camera_config_vreg(dev,
			s_ctrl->sensordata->sensor_platform_info->cam_vreg,
			s_ctrl->sensordata->sensor_platform_info->num_vreg,
			s_ctrl->vreg_seq,
			s_ctrl->num_vreg_seq,
			s_ctrl->reg_ptr, 1);
	if (rc < 0) {
		LDBGE("%s: regulator on failed\n", __func__);
		goto config_vreg_failed;
	}

/*                                                                             */
	rc = gpio_request(MSM_CAM2_RST_EN, "VTCAM_RST_EN");
	if (rc) {
		LDBGE("%s: PM request gpio failed\n", __func__);
	}
/*                                                                           */
	rc = msm_camera_enable_vreg(dev,
			s_ctrl->sensordata->sensor_platform_info->cam_vreg,
			s_ctrl->sensordata->sensor_platform_info->num_vreg,
			s_ctrl->vreg_seq,
			s_ctrl->num_vreg_seq,
			s_ctrl->reg_ptr, 1);
	if (rc < 0) {
		LDBGE("%s: enable regulator failed\n", __func__);
		goto enable_vreg_failed;
	}
/*                                                                             */
	usleep(100);

	rc =pm8xxx_gpio_config(MSM_CAM2_RST_EN, &gpio28_param);
	if (rc) {
		LDBGE("%s: pm8xxx_gpio_config on failed\n", __func__);
	}
/*                                                                           */
	rc = gpio_direction_output(MSM_CAM2_RST_EN, 1);

	rc = msm_camera_config_gpio_table(data, 1);
	if (rc < 0) {
		LDBGE("%s: config gpio failed\n", __func__);
		goto config_gpio_failed;
	}

	if (s_ctrl->sensor_device_type == MSM_SENSOR_I2C_DEVICE) {
		if (s_ctrl->clk_rate != 0)
			imx119_cam_clk_info->clk_rate = s_ctrl->clk_rate;

		rc = msm_cam_clk_enable(dev, imx119_cam_clk_info,
			s_ctrl->cam_clk, ARRAY_SIZE(imx119_cam_clk_info), 1);
		if (rc < 0) {
			pr_err("%s: clk enable failed\n", __func__);
			goto enable_clk_failed;
		}
	}

	if (!s_ctrl->power_seq_delay)
		usleep_range(1000, 2000);
	else if (s_ctrl->power_seq_delay < 20)
		usleep_range((s_ctrl->power_seq_delay * 1000),
			((s_ctrl->power_seq_delay * 1000) + 1000));
	else
		msleep(s_ctrl->power_seq_delay);

	if (data->sensor_platform_info->ext_power_ctrl != NULL)
		data->sensor_platform_info->ext_power_ctrl(1);

	if (data->sensor_platform_info->i2c_conf &&
		data->sensor_platform_info->i2c_conf->use_i2c_mux)
		imx119_msm_sensor_enable_i2c_mux(data->sensor_platform_info->i2c_conf);

	if (s_ctrl->sensor_device_type == MSM_SENSOR_PLATFORM_DEVICE) {
		rc = msm_sensor_cci_util(s_ctrl->sensor_i2c_client,
			MSM_CCI_INIT);
		if (rc < 0) {
			pr_err("%s cci_init failed\n", __func__);
			goto cci_init_failed;
		}
	}
	s_ctrl->curr_res = MSM_SENSOR_INVALID_RES;
	pr_err( " %s : X sensor name is %s \n",__func__, s_ctrl->sensordata->sensor_name);
	return rc;

cci_init_failed:
	if (data->sensor_platform_info->i2c_conf &&
		data->sensor_platform_info->i2c_conf->use_i2c_mux)
		imx119_msm_sensor_disable_i2c_mux(
			data->sensor_platform_info->i2c_conf);
enable_clk_failed:
	rc = gpio_direction_output(MSM_CAM2_RST_EN, 0);
	msm_camera_config_gpio_table(data, 0);
config_gpio_failed:
	msm_camera_enable_vreg(dev,
			s_ctrl->sensordata->sensor_platform_info->cam_vreg,
			s_ctrl->sensordata->sensor_platform_info->num_vreg,
			s_ctrl->vreg_seq,
			s_ctrl->num_vreg_seq,
			s_ctrl->reg_ptr, 0);
enable_vreg_failed:
	msm_camera_config_vreg(dev,
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

int32_t imx119_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	struct msm_camera_sensor_info *data = s_ctrl->sensordata;
	struct device *dev = NULL;
	CDBG("%s\n", __func__);
	pr_err("%s\n", __func__);

	pr_err( " %s : E sensor name is %s \n",__func__, s_ctrl->sensordata->sensor_name);
	if (s_ctrl->sensor_device_type == MSM_SENSOR_PLATFORM_DEVICE)
		dev = &s_ctrl->pdev->dev;
	else
		dev = &s_ctrl->sensor_i2c_client->client->dev;
	if (s_ctrl->sensor_device_type == MSM_SENSOR_PLATFORM_DEVICE) {
		msm_sensor_cci_util(s_ctrl->sensor_i2c_client,
			MSM_CCI_RELEASE);
	}

	if (data->sensor_platform_info->i2c_conf &&
		data->sensor_platform_info->i2c_conf->use_i2c_mux)
		imx119_msm_sensor_disable_i2c_mux(
			data->sensor_platform_info->i2c_conf);

	if (data->sensor_platform_info->ext_power_ctrl != NULL)
		data->sensor_platform_info->ext_power_ctrl(0);

	msm_cam_clk_enable(dev, imx119_cam_clk_info, s_ctrl->cam_clk,
		ARRAY_SIZE(imx119_cam_clk_info), 0);

	    usleep(5);

	LDBGI("%s: Revision [%d] MSM_CAM2_RST_EN GPIO No.%d\n",__func__, lge_get_board_revno(), MSM_CAM2_RST_EN );
	rc = gpio_direction_output(MSM_CAM2_RST_EN, 0 );
	msm_camera_config_gpio_table(data, 0);

	msm_camera_enable_vreg(dev,
		s_ctrl->sensordata->sensor_platform_info->cam_vreg,
		s_ctrl->sensordata->sensor_platform_info->num_vreg,
		s_ctrl->vreg_seq,
		s_ctrl->num_vreg_seq,
		s_ctrl->reg_ptr, 0);
	msm_camera_config_vreg(dev,
		s_ctrl->sensordata->sensor_platform_info->cam_vreg,
		s_ctrl->sensordata->sensor_platform_info->num_vreg,
		s_ctrl->vreg_seq,
		s_ctrl->num_vreg_seq,
		s_ctrl->reg_ptr, 0);

	msm_camera_request_gpio_table(data, 0);
	kfree(s_ctrl->reg_ptr);
	s_ctrl->curr_res = MSM_SENSOR_INVALID_RES;
	pr_err( " %s : X sensor name is %s \n",__func__, s_ctrl->sensordata->sensor_name);
	gpio_free(MSM_CAM2_RST_EN);
	return 0;
}
module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("Sony 1.3MP Bayer sensor driver");
MODULE_LICENSE("GPL v2");


