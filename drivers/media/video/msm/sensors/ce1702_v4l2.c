/* Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
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
#include "ce1702_interface.h"

#define SENSOR_NAME "ce1702"
#define PLATFORM_DRIVER_NAME "msm_camera_ce1702"
#define ce1702_obj ce1702_##obj

#define CE1702_MULTI_IMAGE_PREVIEW_MODE

#define CE1702_STATUS_ON	1
#define CE1702_STATUS_OFF	0

/*                                                                         */
#ifdef CONFIG_MACH_LGE
#if defined (CONFIG_FB_MSM_MIPI_LGIT_VIDEO_FHD_INVERSE_PT) || defined (CONFIG_FB_MSM_MIPI_LGIT_VIDEO_FHD_INVERSE_PT_PANEL)
#define LGIT_COLOR_ENGINE_SWITCH

#ifdef LGIT_COLOR_ENGINE_SWITCH
extern int mipi_lgit_lcd_color_engine_off(void);
extern int mipi_lgit_lcd_color_engine_on(void);
#endif
#endif
#endif
/*                                                                         */

/*                                                                                            */
static bool ce1702_is_doing_touchaf = false;
static int ce1702_focus_mode = -1;
static int ce1702_manual_focus_val = -1;
static int ce1702_wb_mode = 0;
static int ce1702_brightness = 0;
static int ce1702_led_flash_mode = -1;
static int ce1702_iso_value = -1;
static int ce1702_scene_mode = CAMERA_SCENE_OFF;
static int ce1702_aec_awb_lock = 0;
static int ce1702_cam_preview = -1;
static int ce1702_rotation = -1;
static int ce1702_zoom_ratio = 255;
static int ce1702_size_info = 0x1c;
static int ce1702_deferred_af_mode;
static int ce1702_deferred_focus_length;
static int ce1702_deferred_af_start;
static int ce1702_need_thumbnail;
static int ce1702_asd_onoff = 0;
static bool ce1702_just_changed_to_preview = false;	/*                                                                    */
/*                                                                                          */

static int ce1702_i2c_probe(struct i2c_client *client,const struct i2c_device_id *id);
static int ce1702_i2c_remove(struct i2c_client *client);
static int8_t ce1702_set_zoom_ratio(struct msm_sensor_ctrl_t *s_ctrl, int32_t zoom);
static int8_t ce1702_set_iso(struct msm_sensor_ctrl_t *s_ctrl, int32_t iso);
uint8_t ce1702_check_af_status(bool ispolling);
int8_t ce1702_start_af(struct msm_sensor_ctrl_t *s_ctrl);
int32_t ce1702_stop_lens(struct msm_sensor_ctrl_t *s_ctrl);
int8_t ce1702_set_face_detect(bool asd_onoff);


DEFINE_MUTEX(ce1702_mut);
extern struct msm_sensor_ctrl_t* ce1702_s_interface_ctrl;
static struct msm_sensor_ctrl_t ce1702_s_ctrl;

static struct delayed_work      ce1702_ISP_down_delayed_wq ;
static struct workqueue_struct  *wq_ce1702_ISP_down_work_queue;

static int ce1702_frame_mode = CE1702_FRAME_MAX;
/*                                                                  */
static int isPreviewMode = 0;
static int isSingleCaptureMode = 0;
static int isTMSMode = 0;
static int isBurstMode = 0;
static int isHDRMode = 0;
static int isLowLightShotMode = 0;
/*                                                                  */

/*                                                                                          */
static struct dimen_t size_info;
/*                                                                                          */

static struct msm_camera_i2c_reg_conf ce1702_start_settings[] = {
#ifdef CE1702_YUV_IMAGE_PREVIEW_MODE
	{0x6B, 0x01},
#endif
#ifdef CE1702_MULTI_IMAGE_PREVIEW_MODE
	{0x63, 0x01},
#endif
};

static struct msm_camera_i2c_reg_conf ce1702_stop_settings[] = {
#ifdef CE1702_YUV_IMAGE_PREVIEW_MODE
		{0x6B, 0x00},
#endif
#ifdef CE1702_MULTI_IMAGE_PREVIEW_MODE
		{0x63, 0x00},
#endif
};

static struct msm_camera_i2c_reg_conf ce1702_prev_settings[] = {
	{0x54,	0x1C01, MSM_CAMERA_I2C_WORD_DATA},	//preview setting
	{0x11,		0x00},	  //AE/AWB unlock
};

static struct msm_camera_i2c_reg_conf ce1702_recommend_settings[] = {
	{0xF0,	0x00},		// FW start cmd
};

static struct v4l2_subdev_info ce1702_subdev_info[] = {
	{
	.code   = V4L2_MBUS_FMT_YUYV8_2X8,//V4L2_MBUS_FMT_SBGGR10_1X10,
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt    = 1,
	.order    = 0,
	},
	/* more can be supported, to be added later */
};

static struct msm_camera_i2c_conf_array ce1702_init_conf[] = {
	{&ce1702_recommend_settings[0],
	ARRAY_SIZE(ce1702_recommend_settings), 0, MSM_CAMERA_I2C_BYTE_DATA}
};

static struct msm_camera_i2c_conf_array ce1702_confs[] = {
	{&ce1702_prev_settings[0],
	ARRAY_SIZE(ce1702_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ce1702_prev_settings[0],
	ARRAY_SIZE(ce1702_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ce1702_prev_settings[0],
	ARRAY_SIZE(ce1702_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ce1702_prev_settings[0],
	ARRAY_SIZE(ce1702_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ce1702_prev_settings[0],
	ARRAY_SIZE(ce1702_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ce1702_prev_settings[0],
	ARRAY_SIZE(ce1702_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ce1702_prev_settings[0],
	ARRAY_SIZE(ce1702_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ce1702_prev_settings[0],
	ARRAY_SIZE(ce1702_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ce1702_prev_settings[0],
	ARRAY_SIZE(ce1702_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ce1702_prev_settings[0],
	ARRAY_SIZE(ce1702_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ce1702_prev_settings[0],
	ARRAY_SIZE(ce1702_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ce1702_prev_settings[0],
	ARRAY_SIZE(ce1702_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ce1702_prev_settings[0],
	ARRAY_SIZE(ce1702_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ce1702_prev_settings[0],
	ARRAY_SIZE(ce1702_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_sensor_output_info_t ce1702_dimensions[] = {
/*                                                                                   */
	{
		/* full size  15fps*/
		.x_output = 0x1040, /* 4160 */
		.y_output = 0x0C30, /* 3120 */
		.line_length_pclk = 0x1040,
		.frame_length_lines = 0x0C30,
		.vt_pixel_clk = 225600000,
		.op_pixel_clk = 225600000,
		.binning_factor = 1,
	},
	{
		/* preview  30fps*/
		.x_output = 0x0500, /* 1280 */
		.y_output = 0x03C0, /* 960 */
		.line_length_pclk = 0x0500,
		.frame_length_lines = 0x03C0,
		.vt_pixel_clk = 225600000,
		.op_pixel_clk = 225600000,
		.binning_factor = 1,
	},
	{
		/* Video 30fps*/
		.x_output = 0x0780, /* 1920 */
		.y_output = 0x0440, /* 1088 */
		.line_length_pclk = 0x0780,
		.frame_length_lines = 0x0440,
		.vt_pixel_clk = 225600000,
		.op_pixel_clk = 225600000,
		.binning_factor = 1,
	},
	{
		/* preview 30fps*/
		.x_output = 0x0500, /* 1280 */
		.y_output = 0x02D0, /* 720 */
		.line_length_pclk = 0x0500,
		.frame_length_lines = 0x02D0,
		.vt_pixel_clk = 225600000,
		.op_pixel_clk = 225600000,
		.binning_factor = 1,
	},
	{
		/* preview 30fps*/
		.x_output = 0x0780, /* 1920 */
		.y_output = 0x0438, /* 1080 */
		.line_length_pclk = 0x0780,
		.frame_length_lines = 0x0438,
		.vt_pixel_clk = 225600000,
		.op_pixel_clk = 225600000,
		.binning_factor = 1,
	},
	{
		/* preview 30fps*/
		.x_output = 0x02D0, /* 720 */
		.y_output = 0x01E0, /* 480 */
		.line_length_pclk = 0x02D0,
		.frame_length_lines = 0x01E0,
		.vt_pixel_clk = 225600000,
		.op_pixel_clk = 225600000,
		.binning_factor = 1,
	},
	{
		/* preview 30fps*/
		.x_output = 0x160, /* 352*/
		.y_output = 0x0120, /* 288 */
		.line_length_pclk = 0x160,
		.frame_length_lines = 0x0120,
		.vt_pixel_clk = 225600000,
		.op_pixel_clk = 225600000,
		.binning_factor = 1,
	},
	{
		/* EIS FHD size*/
		.x_output = 0x0960, /* 2400 */
		.y_output = 0x0546, /* 1350 */
		.line_length_pclk = 0x0960,
		.frame_length_lines = 0x0546,
		.vt_pixel_clk = 225600000,
		.op_pixel_clk = 225600000,
		.binning_factor = 1,
	},
	{
		/* EIS HD size*/
		.x_output = 0x0640, /* 1600 */
		.y_output = 0x0384, /* 900 */
		.line_length_pclk = 0x0640,
		.frame_length_lines = 0x0384,
		.vt_pixel_clk = 225600000,
		.op_pixel_clk = 225600000,
		.binning_factor = 1,
	},
	{
		/* preview 30fps*/
		.x_output = 0x280, /* 640*/
		.y_output = 0x01E0, /* 480 */
		.line_length_pclk = 0x280,
		.frame_length_lines = 0x01E0,
		.vt_pixel_clk = 225600000,
		.op_pixel_clk = 225600000,
		.binning_factor = 1,
	},
	{
		/* preview 30fps*/
		.x_output = 0x140, /* 320*/
		.y_output = 0x0F0, /* 240 */
		.line_length_pclk = 0x140,
		.frame_length_lines = 0x0F0,
		.vt_pixel_clk = 225600000,
		.op_pixel_clk = 225600000,
		.binning_factor = 1,
	},
/*                                                                        */
      {
		/* zsl*/
		.x_output = 0x0500, /* 1280 */
		.y_output = 0x03C0, /* 960 */
		.line_length_pclk = 0x0500,
		.frame_length_lines = 0x03C0,
		.vt_pixel_clk = 225600000,
		.op_pixel_clk = 225600000,
		.binning_factor = 1,
	},
	{
		/* burst shot*/
		.x_output = 0x0500, /* 1280 */
		.y_output = 0x03C0, /* 960 */
		.line_length_pclk = 0x0500,
		.frame_length_lines = 0x03C0,
		.vt_pixel_clk = 225600000,
		.op_pixel_clk = 225600000,
		.binning_factor = 1,
	},
	{
		/* HDR shot*/
		.x_output = 0x0500, /* 1280 */
		.y_output = 0x03C0, /* 960 */
		.line_length_pclk = 0x0500,
		.frame_length_lines = 0x03C0,
		.vt_pixel_clk = 225600000,
		.op_pixel_clk = 225600000,
		.binning_factor = 1,
	},
	{
		/* Low Light shot*/
		.x_output = 0x0500, /* 1280 */
		.y_output = 0x03C0, /* 960 */
		.line_length_pclk = 0x0500,
		.frame_length_lines = 0x03C0,
		.vt_pixel_clk = 225600000,
		.op_pixel_clk = 225600000,
		.binning_factor = 1,
	},
/*                                                                        */
/*                                                                                 */
};

static struct msm_sensor_output_reg_addr_t ce1702_reg_addr = {
	.x_output = 0x034C,
	.y_output = 0x034E,
	.line_length_pclk = 0x0342,
	.frame_length_lines = 0x0340,
};

static struct msm_sensor_id_info_t ce1702_id_info = {
	.sensor_id_reg_addr = 0x0000,
	.sensor_id = 0x0000,	//0x0091,
};

static struct msm_sensor_exp_gain_info_t ce1702_exp_gain_info = {
	.coarse_int_time_addr = 0x0202,
	.global_gain_addr = 0x0204,
	.vert_offset = 5,
};

static enum msm_camera_vreg_name_t ce1702_veg_seq[] = {
	CAM_VIO,
	CAM_VDIG,
	CAM_VANA,
	CAM_VAF,
	CAM_ISP_CORE,
	CAM_ISP_HOST,
	CAM_ISP_RAM,
	CAM_ISP_CAMIF,
	CAM_ISP_SYS,
};

static const struct i2c_device_id ce1702_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&ce1702_s_ctrl},
	{ }
};

static struct i2c_driver ce1702_i2c_driver = {
	.id_table = ce1702_i2c_id,
	.probe	= ce1702_i2c_probe,
	.remove = ce1702_i2c_remove,
	.driver = {
		.name = SENSOR_NAME,
	},
};

static struct pm_gpio gpio20_param = {
		.direction      = PM_GPIO_DIR_OUT,
		.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
		.output_value   = 1,
		.pull           = PM_GPIO_PULL_NO,
		.vin_sel	= PM_GPIO_VIN_S4,
		.out_strength   = PM_GPIO_STRENGTH_MED,
		.function       = PM_GPIO_FUNC_NORMAL,
};
static struct pm_gpio gpio13_param = {
		.direction      = PM_GPIO_DIR_OUT,
		.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
		.output_value   = 1,
		.pull           = PM_GPIO_PULL_NO,
		.vin_sel	= PM_GPIO_VIN_S4,
		.out_strength   = PM_GPIO_STRENGTH_MED,
		.function       = PM_GPIO_FUNC_NORMAL,
};
/*                                                                          */
static struct pm_gpio gpio27_param = {
		.direction      = PM_GPIO_DIR_OUT,
		.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
		.output_value   = 1,
		.pull           = PM_GPIO_PULL_NO,
		.vin_sel	= PM_GPIO_VIN_S4,
		.out_strength   = PM_GPIO_STRENGTH_MED,
		.function       = PM_GPIO_FUNC_NORMAL,
};
/*                                                                        */
static struct msm_cam_clk_info ce1702_cam_clk_info[] = {
	{"cam_clk", MSM_SENSOR_MCLK_24HZ},
};

static struct msm_camera_i2c_client ce1702_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
};

static int __init ce1702_sensor_init_module(void)
{
	LDBGI("%s: ENTER\n", __func__);
	return i2c_add_driver(&ce1702_i2c_driver);
}

static struct v4l2_subdev_core_ops ce1702_subdev_core_ops = {
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};

static struct v4l2_subdev_video_ops ce1702_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops ce1702_subdev_ops = {
	.core = &ce1702_subdev_core_ops,
	.video  = &ce1702_subdev_video_ops,
};

/*                                                                 */

static struct ce1702_size_type ce1702_picture_table[] = {
	{4208, 3120, CE1702_SIZE_13MP},
	{4160, 3120, CE1702_SIZE_13MP},
	{4160, 2340, CE1702_SIZE_W10MP},
	{3264, 2448, CE1702_SIZE_8MP},
	{3264, 1836, CE1702_SIZE_W6MP},
	{3200, 1920, CE1702_SIZE_W6MP},
	{2560, 1920, CE1702_SIZE_5MP},
	{2304, 1296, CE1702_SIZE_W3MP},
	{2240, 1344, CE1702_SIZE_W3MP},
	{2048, 1536, CE1702_SIZE_3MP},
	{1920, 1088, CE1702_SIZE_FHD},
	{1920, 1080, CE1702_SIZE_FHD1},
	{1600, 1200, CE1702_SIZE_2MP},
	{1536, 864, CE1702_SIZE_W1MP},
	{1280, 960, CE1702_SIZE_1MP},
	{1280, 768, CE1702_SIZE_W1MP},
	{1280, 720, CE1702_SIZE_HD},
	{720, 480, CE1702_SIZE_D1},
	{640, 480, CE1702_SIZE_VGA},
	{352, 288, CE1702_SIZE_CIF},
	{320, 240, CE1702_SIZE_QVGA},
	{176, 144, CE1702_SIZE_QCIF},
};

static struct ce1702_size_type ce1702_postview_table[] = {
	{1280, 960, CE1702_SIZE_1MP},
	{1280, 720, CE1702_SIZE_HD},
	{176, 144, CE1702_SIZE_QCIF},
};

static struct ce1702_size_type ce1702_preview_table[] = {
	{1920, 1088, CE1702_SIZE_FHD},
	{1920, 1080, CE1702_SIZE_FHD1},
	{1280, 960, CE1702_SIZE_1MP},
	{1280, 720, CE1702_SIZE_HD},
//	{720, 480, CE1702_SIZE_D1},
	{160, 120, CE1702_SIZE_QQCIF},
	{144, 176, CE1702_SIZE_CIF},
};

static struct ce1702_size_type ce1702_video_table[] = {
	{2400, 1350, CE1702_SIZE_EIS_FHD},
	{1920, 1088, CE1702_SIZE_FHD},
	{1600, 900, CE1702_SIZE_EIS_HD},
	{1280, 960, CE1702_SIZE_1MP},
	{1280, 720, CE1702_SIZE_HD},
	{720, 480, CE1702_SIZE_D1},
	{640, 480, CE1702_SIZE_VGA},
	{320, 240, CE1702_SIZE_QVGA},
	{352, 288, CE1702_SIZE_CIF},
	{176, 144, CE1702_SIZE_CIF},
	{144, 176, CE1702_SIZE_CIF},
//	{176, 144, CE1702_SIZE_QCIF},
};

static struct ce1702_size_type ce1702_thumbnail_table[] = {
	{480, 270, CE1702_SIZE_169},
	{400, 240, CE1702_SIZE_53},
	{320, 240, CE1702_SIZE_QVGA},
//	{240, 160, CE1702_SIZE_QVGA},	//requested. temporarily blocked TV size. (12/27)
	{176, 144, CE1702_SIZE_QCIF},
};

uint16_t ce1702_get_supported_size(uint16_t width, uint16_t height, struct ce1702_size_type *table, int tbl_size)
{
	int  i;
	uint16_t rc = 0;
	uint32_t calc=0, target=0;

	for(i=0;i<tbl_size;i++) {
		if( (table[i].width == width) && (table[i].height == height) ) {
			rc = table[i].size_val;
			LDBGI("%s: requested size = [%d x %d], supported size = [%d x %d] val=[0x%02x]\n", __func__,
					width, height, table[i].width, table[i].height, rc);
			break;
		}
	}

  if(rc == 0) {
       if(width == 160 && height == 120){
          rc = CE1702_SIZE_QQCIF;
			LDBGI("%s: Not find any size. use this! = [%d x %d](default) val=0x%02x\n", __func__,
						160, 120, rc);
          return rc;
       }
  }

	if(rc == 0) {
		if(height !=0)
			calc = (width*4096) / height;
		for(i=0;i<tbl_size;i++) {
			if (table[i].height !=0)
				target = (table[i].width*4096) / table[i].height;
			if(calc == target) {
				rc = table[i].size_val;
				LDBGI("%s: ## requested size = [%d x %d], supported size = [%d x %d] val=[0x%02x]\n", __func__,
						width, height, table[i].width, table[i].height, rc);
				break;
			}
		}
	}

	//Even though rc = 0,
	if(rc == 0) {
		if(table[0].width > 500) {	// Check whether thumbnail or not.
			rc = CE1702_SIZE_1MP;
			LDBGI("%s: Not find any size. use this! = [%d x %d](default) val=0x%02x\n", __func__,
						1280, 960, rc);
		} else {
			rc = CE1702_SIZE_QCIF;
			LDBGI("%s: Not find any size. use this! = [%d x %d](default) val=0x%02x\n", __func__,
						176, 144, rc);
		}
	}

	return rc;
}
/*                                                               */

//                                                        
int32_t ce1702_sensor_setting(struct msm_sensor_ctrl_t *s_ctrl,
			int update_type, int res)
{
	int32_t rc = 0;
	uint8_t wdata[20];
#ifndef STOP_LENS_WORKAROUND_1205
	//uint8_t rdata[2];
	uint8_t result =0;
	int cnt;
#endif

	LDBGI("%s: update_type = %d, i2c_add = %d, res=%d \n", __func__, update_type, ce1702_s_interface_ctrl->sensor_i2c_addr, res);
	if (update_type == MSM_SENSOR_REG_INIT) {
		//s_ctrl->func_tbl->sensor_stop_stream(s_ctrl);
		//msm_sensor_write_init_settings(s_ctrl);
		//mdelay(500);
		CE_FwStart();
		ce1702_set_model_name();
		ce1702_switching_exif_gps(false);
	} else if (update_type == MSM_SENSOR_UPDATE_PERIODIC) {
	//Start, Mode Change, Suspend/Resume
		if ((isPreviewMode | isSingleCaptureMode | isTMSMode | isBurstMode | isHDRMode | isLowLightShotMode) ==0){
			rc = ce1702_set_ISP_mode();
			if (rc < 0)
				LDBGE("%s: set ISP mode failed \n", __func__);
/*                                                                         */
#ifdef LGIT_COLOR_ENGINE_SWITCH
                        if(system_state != SYSTEM_BOOTING) {
                                mipi_lgit_lcd_color_engine_off();
                        }
#endif
/*                                                                         */
		}
		switch (res){
			case MSM_SENSOR_RES_QTR:
			case MSM_SENSOR_RES_2:
			case MSM_SENSOR_RES_3:
			case MSM_SENSOR_RES_4:
			case MSM_SENSOR_RES_5:
			case MSM_SENSOR_RES_6:
			case MSM_SENSOR_RES_7:
			case MSM_SENSOR_RES_8:
			case MSM_SENSOR_RES_9:
			case MSM_SENSOR_RES_10:
            case MSM_SENSOR_RES_YUV_PREVIEW:
				ce1702_frame_mode = CE1702_MODE_PREVIEW;
				v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
					NOTIFY_PCLK_CHANGE, &s_ctrl->msm_sensor_reg->
					output_settings[res].op_pixel_clk);
				return rc;
			case MSM_SENSOR_RES_FULL:
				ce1702_frame_mode = CE1702_MODE_SINGLE_SHOT;
				break;
			case MSM_SENSOR_RES_ZSL:
				LDBGE("%s: E, Time machine shot setting\n", __func__);
				ce1702_frame_mode = CE1702_MODE_TMS_SHOT;
				break;
			case MSM_SENSOR_RES_BURST:
				LDBGE("%s: E, Burst shot setting\n", __func__);
				ce1702_frame_mode = CE1702_MODE_BURST_SHOT;
				break;
          case MSM_SENSOR_RES_HDR:
              LDBGE("%s: E, HDR shot setting\n", __func__);
              ce1702_frame_mode = CE1702_MODE_HDR_SHOT;

              wdata[0] = 0x20;
              wdata[1] = 0x01;
              ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x07, wdata,  2);

              wdata[0] = 0x05;
              wdata[1] = 0x0A;
              ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x07, wdata,  2);

              cnt = sizeof(ce1702_postview_table) / sizeof(ce1702_postview_table[0]);
              wdata[0] = ce1702_get_supported_size(size_info.preview_width, size_info.preview_height, ce1702_postview_table, cnt);
              wdata[1] = 0x05;
              ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x55, wdata,  2);
              LDBGI("%s: postview wdata[0] = %d\n", __func__,wdata[0]);

              cnt = sizeof(ce1702_picture_table) / sizeof(ce1702_picture_table[0]);
              wdata[0] = ce1702_get_supported_size(size_info.picture_width, size_info.picture_height, ce1702_picture_table, cnt);
              ce1702_size_info = wdata[0];
              wdata[1] = 0x10;
              wdata[2] = 0x56;
              wdata[3] = 0x00;
              LDBGI("%s: picture wdata[0] = %d\n", __func__,wdata[0]);
              ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x73, wdata,  4);

              wdata[0] = 0x00;
              wdata[1] = 0x01;
              ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x71, wdata,  2);

             ce1702_set_zoom_ratio(s_ctrl, ce1702_zoom_ratio);

              break;
           case MSM_SENSOR_RES_LLS:
              LDBGE("%s: E, Low Light shot setting\n", __func__);
              ce1702_frame_mode = CE1702_MODE_LOW_LIGHT_SHOT;

              wdata[0] = 0x20;
              wdata[1] = 0x01;
              ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x07, wdata,  2);

              cnt = sizeof(ce1702_postview_table) / sizeof(ce1702_postview_table[0]);
              wdata[0] = ce1702_get_supported_size(size_info.preview_width, size_info.preview_height, ce1702_postview_table, cnt);
              wdata[1] = 0x05;
              ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x55, wdata,  2);
              LDBGI("%s: postview wdata[0] = %d\n", __func__,wdata[0]);

              cnt = sizeof(ce1702_picture_table) / sizeof(ce1702_picture_table[0]);
              wdata[0] = ce1702_get_supported_size(size_info.picture_width, size_info.picture_height, ce1702_picture_table, cnt);
              ce1702_size_info = wdata[0];
              wdata[1] = 0x10;
              wdata[2] = 0x14;
              wdata[3] = 0x00;
              LDBGI("%s: wdata[0] = %d\n", __func__,wdata[0]);
              ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x73, wdata,  4);

              wdata[0] = 0x00;
              wdata[1] = 0x01;
              ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x71, wdata,  2);

             ce1702_set_zoom_ratio(s_ctrl, ce1702_zoom_ratio);
              break;
			default:
				LDBGE("%s: res not exist.\n", __func__);
				break;
		}

#ifndef STOP_LENS_WORKAROUND_1205
		if(ce1702_focus_mode == AF_MODE_AUTO) {
			wdata[0] = 0x00;
			rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x35, wdata,  1);	// Stop VCM
			if (rc < 0)
				LDBGE("%s: ce1702_i2c_write failed \n", __func__);
			cnt = 0;
			LDBGI("%s: stop lens ce1702_i2c_read successed, result=%d\n", __func__, result);
			do {
				//rdata[0] = 0x00;
				rc = ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x24, NULL, 0, &result,  1);
				if (rc < 0)
					LDBGE("%s: ce1702_i2c_read failed \n", __func__);
				if ((result & 0x01) == 0){
					break;
				}
				cnt++;
				mdelay(5); //yt.jeon 1115 optimize delay time
			} while (cnt < 100); 	//lens running
		}
#endif
		LDBGI("%s: ce1702_frame_mode = %d, res=%d \n", __func__, ce1702_frame_mode, res);

		v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
			NOTIFY_PCLK_CHANGE, &s_ctrl->msm_sensor_reg->
			output_settings[res].op_pixel_clk);
	}
	return rc;
}

static int ce1702_i2c_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
	int rc = 0;
       struct msm_sensor_ctrl_t *s_ctrl;
	LDBGI("%s :%s_i2c_probe called\n", __func__, client->name);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		LDBGE("%s %s i2c_check_functionality failed\n",
			__func__, client->name);
		rc = -EFAULT;
		return rc;
	}

	s_ctrl = (struct msm_sensor_ctrl_t *)(id->driver_data);
	ce1702_s_interface_ctrl = s_ctrl;
	if (s_ctrl->sensor_i2c_client != NULL) {
		s_ctrl->sensor_i2c_client->client = client;
		if (s_ctrl->sensor_i2c_addr != 0)
			s_ctrl->sensor_i2c_client->client->addr =
				s_ctrl->sensor_i2c_addr;
	} else {
		LDBGE("%s %s sensor_i2c_client NULL\n",
			__func__, client->name);
		rc = -EFAULT;
		return rc;
	}

	s_ctrl->sensordata = client->dev.platform_data;
	if (s_ctrl->sensordata == NULL) {
		LDBGE("%s %s NULL sensor data\n", __func__, client->name);
		return -EFAULT;
	}

	init_suspend(); //                                            
	ce1702_sysfs_add(&client->dev.kobj);

	wq_ce1702_ISP_down_work_queue = create_singlethread_workqueue("wq_ce1702_ISP_down_work_queue");
	if (wq_ce1702_ISP_down_work_queue == NULL) {
		LDBGE("wq_ce1702_ISP_down_work_queue fail	\n");
		rc = -ENOMEM;
		goto probe_failure;
	}

	LDBGI("%s: Get Mutex to start delayed workqueue for ISP Bin Download !!!!![%lu]\n", __func__, jiffies);

	INIT_DELAYED_WORK(&ce1702_ISP_down_delayed_wq, ce1702_wq_ISP_upload);

    if(wq_ce1702_ISP_down_work_queue)
        queue_delayed_work(wq_ce1702_ISP_down_work_queue, &ce1702_ISP_down_delayed_wq, HZ );

	snprintf(s_ctrl->sensor_v4l2_subdev.name,
		sizeof(s_ctrl->sensor_v4l2_subdev.name), "%s", id->name);

	if (!s_ctrl->wait_num_frames)
			s_ctrl->wait_num_frames = 1 * Q10;

	LDBGE("%s %s probe succeeded\n", __func__, client->name);

	v4l2_i2c_subdev_init(&s_ctrl->sensor_v4l2_subdev, client,
		s_ctrl->sensor_v4l2_subdev_ops);
	s_ctrl->sensor_v4l2_subdev.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	media_entity_init(&s_ctrl->sensor_v4l2_subdev.entity, 0, NULL, 0);
	s_ctrl->sensor_v4l2_subdev.entity.type = MEDIA_ENT_T_V4L2_SUBDEV;
	s_ctrl->sensor_v4l2_subdev.entity.group_id = SENSOR_DEV;
	s_ctrl->sensor_v4l2_subdev.entity.name =
		s_ctrl->sensor_v4l2_subdev.name;
	msm_sensor_register(&s_ctrl->sensor_v4l2_subdev);
	s_ctrl->sensor_v4l2_subdev.entity.revision =
		s_ctrl->sensor_v4l2_subdev.devnode->num;

	LDBGI("ce1702_i2c_probe ends successfully\n");

	return 0;
probe_failure:
//	kfree(ce1702_sensorw);
//	ce1702_sensorw = NULL;
	LDBGE("ce1702_probe failed!\n");
	return rc;

}

static int ce1702_i2c_remove(struct i2c_client *client)
{
	/* TODO: this function is called twice. Handle It! */

	struct ce1702_work *sensorw = i2c_get_clientdata(client);

	LDBGI("%s: called\n", __func__);

	ce1702_sysfs_rm(&client->dev.kobj);
	free_irq(client->irq, sensorw);
	deinit_suspend();
	kfree(sensorw);
	return 0;
}


int32_t ce1702_msm_sensor_enable_i2c_mux(struct msm_camera_i2c_conf *i2c_conf)
{
	struct v4l2_subdev *i2c_mux_sd =
		dev_get_drvdata(&i2c_conf->mux_dev->dev);
	v4l2_subdev_call(i2c_mux_sd, core, ioctl,
		VIDIOC_MSM_I2C_MUX_INIT, NULL);
	v4l2_subdev_call(i2c_mux_sd, core, ioctl,
		VIDIOC_MSM_I2C_MUX_CFG, (void *)&i2c_conf->i2c_mux_mode);
	return 0;
}

int32_t ce1702_msm_sensor_disable_i2c_mux(struct msm_camera_i2c_conf *i2c_conf)
{
	struct v4l2_subdev *i2c_mux_sd =
		dev_get_drvdata(&i2c_conf->mux_dev->dev);
	v4l2_subdev_call(i2c_mux_sd, core, ioctl,
				VIDIOC_MSM_I2C_MUX_RELEASE, NULL);
	return 0;
}

//                                          
void ce1702_param_init(void)
{
	ce1702_is_doing_touchaf = false;
	ce1702_wb_mode = 0;
	ce1702_brightness = 0;
	ce1702_focus_mode = -1;
	ce1702_manual_focus_val = -1;
	ce1702_led_flash_mode = -1;
	ce1702_aec_awb_lock = 0;
	ce1702_cam_preview = -1;
	ce1702_iso_value = -1;
	ce1702_scene_mode = CAMERA_SCENE_OFF;
	ce1702_rotation = -1;
	ce1702_zoom_ratio = 255;
	ce1702_size_info = 0x1c;
	ce1702_deferred_af_mode = -1;
	ce1702_deferred_focus_length = -1;
	ce1702_deferred_af_start = 0;
	ce1702_need_thumbnail = true;
	ce1702_asd_onoff = 0;
	ce1702_just_changed_to_preview = false;	/*                                                                    */
}

int32_t ce1702_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	struct msm_camera_sensor_info *data = s_ctrl->sensordata;
	hw_rev_type lge_bd_rev = HW_REV_EVB1;
	lge_bd_rev = lge_get_board_revno();
	LDBGI("%s: %d\n", __func__, __LINE__);

	ce1702_param_init();	// init parameters by jungki.kim
	ce1702_frame_mode = CE1702_FRAME_MAX;
	if(isPreviewMode == 1 || isSingleCaptureMode == 1 || isTMSMode==1 || isBurstMode==1|| isHDRMode==1 || isLowLightShotMode==1 ){
		LDBGE("%s: Preview is still running!!! isPreviewMode = %d, isSingleCaptureMode = %d, isTMSMode=%d, isBurstMode=%d, isHDRMode=%d, isLowLightShotMode=%d\n",
      __func__, isPreviewMode, isSingleCaptureMode, isTMSMode, isBurstMode, isHDRMode, isLowLightShotMode);
		isPreviewMode = 0;
		isSingleCaptureMode = 0;
		isTMSMode = 0;
		isBurstMode = 0;
       isHDRMode = 0;
       isLowLightShotMode = 0;
	}

	s_ctrl->reg_ptr = kzalloc(sizeof(struct regulator *)
			* data->sensor_platform_info->num_vreg, GFP_KERNEL);
	if (!s_ctrl->reg_ptr) {
		LDBGE("%s: could not allocate mem for regulators\n",
			__func__);
		return -ENOMEM;
	}

	LDBGE("%s: before request gpio, sensor name : %s", __func__, s_ctrl->sensordata->sensor_name);
	rc = msm_camera_request_gpio_table(data, 1);
	if (rc < 0) {
		LDBGE("%s: request gpio failed\n", __func__);
		goto request_gpio_failed;
	}

	rc = gpio_request(ISP_HOST_INT, "ISP_HOST_INT");
	rc = gpio_request(ISP_STBY, "ISP_STBY");
/*                                                                          */
#if defined(CONFIG_MACH_APQ8064_GKKT) || defined(CONFIG_MACH_APQ8064_GKSK) || defined(CONFIG_MACH_APQ8064_GKU) || defined(CONFIG_MACH_APQ8064_GKATT) || defined(CONFIG_MACH_APQ8064_GVKT) || defined(CONFIG_MACH_APQ8064_GKGLOBAL)
	if(lge_bd_rev >= HW_REV_C ||lge_bd_rev == HW_REV_1_0){
		LDBGI("%s: Revision check! ISP_RST GPIO No.%d\n",__func__,ISP_RST );
		rc = gpio_request(ISP_RST, "ISP_RST");
	}
	if (rc) {
		LDBGE("%s: PM request gpio failed\n", __func__);
	}
#elif defined(CONFIG_MACH_APQ8064_GVDCM)
	if(lge_bd_rev >= HW_REV_C){
		LDBGI("%s: Revision check! ISP_RST GPIO No.%d\n",__func__,ISP_RST );
		rc = gpio_request(ISP_RST, "ISP_RST");
	}
	if (rc) {
		LDBGE("%s: PM request gpio failed\n", __func__);
	}
#elif defined(CONFIG_MACH_APQ8064_GVAR_CMCC) || defined(CONFIG_MACH_APQ8064_OMEGAR) || defined(CONFIG_MACH_APQ8064_OMEGA)
//	LDBGI("%s: Revision [%d] ISP_RST GPIO No.%d\n",__func__, lge_bd_rev, ISP_RST);
	rc = gpio_request(ISP_RST, "ISP_RST");
	if (rc) {
		LDBGE("%s: PM request gpio failed\n", __func__);
	}
#endif
/*                                                                        */

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

	mdelay(1);

	if (s_ctrl->clk_rate != 0)
		ce1702_cam_clk_info->clk_rate = s_ctrl->clk_rate;

	rc = msm_cam_clk_enable(&s_ctrl->sensor_i2c_client->client->dev,
		ce1702_cam_clk_info, s_ctrl->cam_clk, ARRAY_SIZE(ce1702_cam_clk_info), 1);
	if (rc < 0) {
		LDBGE("%s: clk enable failed\n", __func__);
		goto enable_clk_failed;
	}
	mdelay(5);

	rc =pm8xxx_gpio_config(ISP_HOST_INT, &gpio20_param);
	rc =pm8xxx_gpio_config(ISP_STBY, &gpio13_param);
/*                                                                          */
#if defined(CONFIG_MACH_APQ8064_GKKT) || defined(CONFIG_MACH_APQ8064_GKSK) || defined(CONFIG_MACH_APQ8064_GKU) || defined(CONFIG_MACH_APQ8064_GKATT) || defined(CONFIG_MACH_APQ8064_GVKT) || defined(CONFIG_MACH_APQ8064_GKGLOBAL)
	if(lge_bd_rev >= HW_REV_C ||lge_bd_rev == HW_REV_1_0){
		LDBGI("%s: Revision check! ISP_RST GPIO No.%d\n",__func__,ISP_RST );
		rc =pm8xxx_gpio_config(ISP_RST, &gpio27_param);
	}
	if (rc) {
		LDBGE("%s: pm8xxx_gpio_config on failed\n", __func__);
	}
#elif defined(CONFIG_MACH_APQ8064_GVDCM)
	if(lge_bd_rev >= HW_REV_C){
		LDBGI("%s: Revision check! ISP_RST GPIO No.%d\n",__func__,ISP_RST );
		rc =pm8xxx_gpio_config(ISP_RST, &gpio27_param);
	}
	if (rc) {
		LDBGE("%s: pm8xxx_gpio_config on failed\n", __func__);
	}
#elif defined(CONFIG_MACH_APQ8064_GVAR_CMCC)|| defined(CONFIG_MACH_APQ8064_OMEGAR) || defined(CONFIG_MACH_APQ8064_OMEGA)
	LDBGI("%s: Revision check! ISP_RST GPIO No.%d\n",__func__,ISP_RST );
	rc =pm8xxx_gpio_config(ISP_RST, &gpio27_param);
	if (rc) {
		LDBGE("%s: pm8xxx_gpio_config on failed\n", __func__);
	}
#endif
/*                                                                        */

	rc = gpio_direction_output(ISP_STBY, 1);

	mdelay(1);

/*                                                                          */
#if defined(CONFIG_MACH_APQ8064_GVAR_CMCC)|| defined(CONFIG_MACH_APQ8064_OMEGAR) || defined(CONFIG_MACH_APQ8064_OMEGA)
	rc = gpio_direction_output(ISP_RST, 1);
	if (rc < 0) {
		LDBGE("%s: config gpio failed\n", __func__);
		goto config_gpio_failed;
	}
#else
#if defined(CONFIG_MACH_APQ8064_GKKT) || defined(CONFIG_MACH_APQ8064_GKSK) || defined(CONFIG_MACH_APQ8064_GKU) || defined(CONFIG_MACH_APQ8064_GKATT) || defined(CONFIG_MACH_APQ8064_GVKT) || defined(CONFIG_MACH_APQ8064_GKGLOBAL)
	if(lge_bd_rev >= HW_REV_C ||lge_bd_rev == HW_REV_1_0)
	{
		LDBGI("%s: Revision check! ISP_RST GPIO No.%d\n",__func__,ISP_RST );
		rc = gpio_direction_output(ISP_RST, 1);
		if (rc < 0) {
			LDBGE("%s: config gpio failed\n", __func__);
			goto config_gpio_failed;
		}
	}else
#elif defined(CONFIG_MACH_APQ8064_GVDCM)
	if(lge_bd_rev >= HW_REV_C)
	{
		LDBGI("%s: Revision check! ISP_RST GPIO No.%d\n",__func__,ISP_RST );
		rc = gpio_direction_output(ISP_RST, 1);
		if (rc < 0) {
			LDBGE("%s: config gpio failed\n", __func__);
			goto config_gpio_failed;
		}
	}else
#endif

/*                                                                        */
	{
		rc = msm_camera_config_gpio_table(data, 1);
		if (rc < 0) {
			LDBGE("%s: config gpio failed\n", __func__);
			goto config_gpio_failed;
		}
	}
#endif

	usleep_range(1000, 2000);
	if (data->sensor_platform_info->ext_power_ctrl != NULL)
		data->sensor_platform_info->ext_power_ctrl(1);

	if (data->sensor_platform_info->i2c_conf &&
		data->sensor_platform_info->i2c_conf->use_i2c_mux)
		ce1702_msm_sensor_enable_i2c_mux(data->sensor_platform_info->i2c_conf);

	return rc;

enable_clk_failed:
/*                                                                          */
#if defined(CONFIG_MACH_APQ8064_GVAR_CMCC)|| defined(CONFIG_MACH_APQ8064_OMEGAR) || defined(CONFIG_MACH_APQ8064_OMEGA)
	rc = gpio_direction_output(ISP_RST, 0);
#else
#if defined(CONFIG_MACH_APQ8064_GKKT) || defined(CONFIG_MACH_APQ8064_GKSK) || defined(CONFIG_MACH_APQ8064_GKU) || defined(CONFIG_MACH_APQ8064_GKATT) || defined(CONFIG_MACH_APQ8064_GVKT) || defined(CONFIG_MACH_APQ8064_GKGLOBAL)
	if(lge_bd_rev >= HW_REV_C ||lge_bd_rev == HW_REV_1_0){
		LDBGI("%s: Revision check! ISP_RST GPIO No.%d\n",__func__,ISP_RST );
		rc = gpio_direction_output(ISP_RST, 0);
	}else
#elif defined(CONFIG_MACH_APQ8064_GVDCM)
	if(lge_bd_rev >= HW_REV_C){
		LDBGI("%s: Revision check! ISP_RST GPIO No.%d\n",__func__,ISP_RST );
		rc = gpio_direction_output(ISP_RST, 0);
	}else
#endif
/*                                                                        */
	{
		msm_camera_config_gpio_table(data, 0);
	}
#endif

config_gpio_failed:
	msm_camera_enable_vreg(&s_ctrl->sensor_i2c_client->client->dev,
			s_ctrl->sensordata->sensor_platform_info->cam_vreg,
			s_ctrl->sensordata->sensor_platform_info->num_vreg,
			s_ctrl->vreg_seq,
			s_ctrl->num_vreg_seq,
			s_ctrl->reg_ptr, 0);

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
	s_ctrl->reg_ptr = NULL;
	return rc;
}

int32_t ce1702_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	struct msm_camera_sensor_info *data = s_ctrl->sensordata;
	hw_rev_type lge_bd_rev = HW_REV_EVB1;
	lge_bd_rev = lge_get_board_revno();
	LDBGI("%s\n", __func__);

/*                                                                         */
#ifdef LGIT_COLOR_ENGINE_SWITCH
    if(system_state != SYSTEM_BOOTING) {
      mipi_lgit_lcd_color_engine_on();
    }
#endif
/*                                                                         */

	rc = ce1702_set_VCM_default_position(s_ctrl);//                                              

	if (data->sensor_platform_info->i2c_conf &&
		data->sensor_platform_info->i2c_conf->use_i2c_mux)
		ce1702_msm_sensor_disable_i2c_mux(
			data->sensor_platform_info->i2c_conf);
/*                                                                          */
#if defined(CONFIG_MACH_APQ8064_GVAR_CMCC)|| defined(CONFIG_MACH_APQ8064_OMEGAR) || defined(CONFIG_MACH_APQ8064_OMEGA)
	rc = gpio_direction_output(ISP_RST, 0);
#else
#if defined(CONFIG_MACH_APQ8064_GKKT) || defined(CONFIG_MACH_APQ8064_GKSK) || defined(CONFIG_MACH_APQ8064_GKU) || defined(CONFIG_MACH_APQ8064_GKATT) || defined(CONFIG_MACH_APQ8064_GVKT) || defined(CONFIG_MACH_APQ8064_GKGLOBAL)
	if(lge_bd_rev >= HW_REV_C ||lge_bd_rev == HW_REV_1_0){
		LDBGI("%s: Revision check! ISP_RST GPIO No.%d\n",__func__,ISP_RST );
		rc = gpio_direction_output(ISP_RST, 0);
 	}else
#elif defined(CONFIG_MACH_APQ8064_GVDCM)
	if(lge_bd_rev >= HW_REV_C){
		LDBGI("%s: Revision check! ISP_RST GPIO No.%d\n",__func__,ISP_RST );
		rc = gpio_direction_output(ISP_RST, 0);
	}else
#endif

/*                                                                        */
 	{
		msm_camera_config_gpio_table(data, 0);
 	}
#endif
	if (data->sensor_platform_info->ext_power_ctrl != NULL)
		data->sensor_platform_info->ext_power_ctrl(0);
	msm_cam_clk_enable(&s_ctrl->sensor_i2c_client->client->dev,
		ce1702_cam_clk_info, s_ctrl->cam_clk, ARRAY_SIZE(ce1702_cam_clk_info), 0);

	mdelay(1);

	rc = gpio_direction_output(ISP_HOST_INT, 0);
	rc = gpio_direction_output(ISP_STBY, 0);
	if (rc) {
		LDBGE("%s: gpio_direction_output enable failed\n", __func__);
	}

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
		msm_camera_request_gpio_table(data, 0);
		kfree(s_ctrl->reg_ptr);
	} else {
		// NULL!
		LDBGE("%s: No Regulator Pointer!\n", __func__);
	}
/*                                                               */

	gpio_free(ISP_HOST_INT);
	gpio_free(ISP_STBY);
/*                                                                          */
#if defined(CONFIG_MACH_APQ8064_GKKT) || defined(CONFIG_MACH_APQ8064_GKSK) || defined(CONFIG_MACH_APQ8064_GKU) || defined(CONFIG_MACH_APQ8064_GKATT) || defined(CONFIG_MACH_APQ8064_GVKT) || defined(CONFIG_MACH_APQ8064_GKGLOBAL)
		LDBGI("%s: Revision check! ISP_RST GPIO No.%d\n",__func__,ISP_RST );
		if(lge_bd_rev >= HW_REV_C ||lge_bd_rev == HW_REV_1_0){
		gpio_free(ISP_RST);
			}
#elif defined(CONFIG_MACH_APQ8064_GVDCM)
if(lge_bd_rev >= HW_REV_C){
	LDBGI("%s: Revision check! ISP_RST GPIO No.%d\n",__func__,ISP_RST );
	gpio_free(ISP_RST);
}
#elif defined(CONFIG_MACH_APQ8064_GVAR_CMCC)|| defined(CONFIG_MACH_APQ8064_OMEGAR) || defined(CONFIG_MACH_APQ8064_OMEGA)
	gpio_free(ISP_RST);
#endif
/*                                                                        */
	ce1702_frame_mode = CE1702_FRAME_MAX;
	if(isPreviewMode == 1 || isSingleCaptureMode == 1 || isTMSMode==1 || isBurstMode==1 || isHDRMode==1 || isLowLightShotMode==1){
		LDBGE("%s: Preview is still running!!! isPreviewMode = %d, isSingleCaptureMode = %d, isTMSMode=%d, isBurstMode=%d, isHDRMode=%d, isLowLightShotMode=%d\n",
      __func__, isPreviewMode, isSingleCaptureMode, isTMSMode, isBurstMode, isHDRMode, isLowLightShotMode);
		isPreviewMode = 0;
		isSingleCaptureMode = 0;
		isTMSMode = 0;
		isBurstMode = 0;
       isHDRMode = 0;
       isLowLightShotMode = 0;
	}
    ce1702_focus_mode = -1;

	return 0;
}

int32_t ce1702_match_id(struct msm_sensor_ctrl_t *s_ctrl)
{
	LDBGI("%s\n", __func__);

	//couldn't detect the match id
	if(dest_location_firmware == CE1702_SDCARD2){
		LDBGI("CE1702 firmware update !! \n");
		ce1702_isp_fw_full_upload();
		ce1702_check_flash_version();
		dest_location_firmware = CE1702_NANDFLASH; // only one-time firmware update...
	}
	return CE1702_OK ;
}
//                                    


//We need to excute these function lately because these have to try under previewing condition.
void ce1702_sensor_set_param_lately(struct msm_sensor_ctrl_t *s_ctrl)
{
	/*
		I found that focus mode does not work on the first time.
		Then I check whether it is previewing or not and it is not on previewing!!
		So I need it try later.
	*/
	ce1702_just_changed_to_preview = true;	/*                                                                    */

	LDBGI("%s: ce1702_aec_awb_lock=[%d] EXTRA WORK TO DO!\n", __func__, ce1702_aec_awb_lock);
	ce1702_set_aec_awb_lock(s_ctrl, ce1702_aec_awb_lock);

	if ( ((ce1702_frame_mode == CE1702_MODE_PREVIEW) && (isSingleCaptureMode == 0))
		|| ((ce1702_frame_mode == CE1702_MODE_TMS_SHOT) && (isTMSMode == 1)) ) {

		if (ce1702_deferred_af_mode >= 0) {
			LDBGI("%s: ce1702_focus_mode=[%d] EXTRA WORK TO DO!\n", __func__, ce1702_deferred_af_mode);
			ce1702_set_focus_mode_setting(s_ctrl, ce1702_deferred_af_mode);
			ce1702_deferred_af_mode = -1;
		}
		if(ce1702_deferred_focus_length >= 0) {
			LDBGI("%s: ce1702_manual_focus_val=[%d] EXTRA WORK TO DO!\n", __func__, ce1702_deferred_focus_length);
			ce1702_set_manual_focus_length(s_ctrl, ce1702_deferred_focus_length);
			ce1702_deferred_focus_length = -1;
		}
		if (ce1702_deferred_af_start > 0) {
			LDBGI("%s: ce1702_start_af=[%d] EXTRA WORK TO DO!\n", __func__, ce1702_deferred_af_start);
// Workaround, need to remove
			ce1702_set_aec_awb_lock(s_ctrl, 0);

			ce1702_start_af(s_ctrl);
			ce1702_deferred_af_start = -1;
		}
	}

	ce1702_just_changed_to_preview = false;	/*                                                                    */
}

void ce1702_sensor_start_stream(struct msm_sensor_ctrl_t *s_ctrl)
{
	uint8_t result = 0;
	int retry_cnt = 0;
	//uint8_t rdata[2];
	uint8_t wdata[7];
	int32_t rc = 0;
	int cnt = 0;
//	int dim_rate;

	LDBGI("%s: isPreviewMode=%d,  isSingleCaptureMode =%d, isTMSMode=%d, isBurstMode =%d, isHDRMode=%d, isLowLightShotMode=%d\n", __func__, isPreviewMode, isSingleCaptureMode, isTMSMode, isBurstMode, isHDRMode, isLowLightShotMode);

	switch(ce1702_frame_mode){
		case CE1702_MODE_PREVIEW:
			if(isPreviewMode == 0){
				rc = ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x6C, NULL, 0,  &result,  1);
				if (rc < 0)
					LDBGE("%s: ce1702_i2c_read failed \n", __func__);
				LDBGE("%s: preview status = %d \n", __func__,result);

				if (result == 0x00) {//if stopping
					LDBGE("%s: CE1702_MODE_PREVIEW initial start\n", __func__);
					if(ce1702_cam_preview == PREVIEW_MODE_CAMCORDER){
						LDBGI("ce1702_get_supported_size: ce1702_video_table\n");
						retry_cnt = sizeof(ce1702_video_table) / sizeof(ce1702_video_table[0]);
						wdata[0] = ce1702_get_supported_size(size_info.video_width, size_info.video_height, ce1702_video_table, retry_cnt);
						if(size_info.video_height*16 == size_info.video_width*9) {
							wdata[1] = 0x03;
							LDBGI("%s: 16:9 recording setting\n", __func__);
						} else {
							wdata[1] = 0x02;
							LDBGI("%s: 4:3 recording setting\n", __func__);
						}
					}
					else{
						LDBGI("ce1702_get_supported_size: ce1702_preview_table\n");
						retry_cnt = sizeof(ce1702_preview_table) / sizeof(ce1702_preview_table[0]);
						wdata[0] = ce1702_get_supported_size(size_info.preview_width, size_info.preview_height, ce1702_preview_table, retry_cnt);
						wdata[1] = 0x01;
					}
					rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x54, wdata,  2);
					if (rc < 0)
						LDBGE("%s: ce1702_i2c_write failed(1) \n", __func__);
					LDBGI("%s: video_width = [%d], video_height = [%d]\n", __func__, size_info.video_width,size_info.video_height);

					LDBGI("ce1702_get_supported_size: ce1702_postview_table\n");
					cnt = sizeof(ce1702_postview_table) / sizeof(ce1702_postview_table[0]);
					/*                                                                  */
					if(size_info.picture_width >= size_info.preview_width && size_info.picture_height >= size_info.preview_height)
				        	wdata[0] = ce1702_get_supported_size(size_info.preview_width, size_info.preview_height, ce1702_postview_table, cnt);
					else
						wdata[0] = ce1702_get_supported_size(size_info.picture_width, size_info.picture_height, ce1702_postview_table, cnt);
					wdata[1] = 0x05;
					rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x55, wdata,  2);
					if (rc < 0)
						LDBGE("%s: ce1702_i2c_write failed(2) \n", __func__);
					LDBGI("%s: Capture Size:: picture width = %d , picture height = %d\n", __func__,size_info.picture_width,size_info.picture_height);

					LDBGI("ce1702_get_supported_size: ce1702_picture_table\n");
					cnt = sizeof(ce1702_picture_table) / sizeof(ce1702_picture_table[0]);
					wdata[0] = ce1702_get_supported_size(size_info.picture_width, size_info.picture_height, ce1702_picture_table, cnt);
					ce1702_size_info = wdata[0];
					wdata[1] = 0x00;
					wdata[2] = 0x00;
					wdata[3] = 0x81;
					LDBGI("%s: wdata[0] = %d\n", __func__,wdata[0]);
					rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x73, wdata,  4);
					if (rc < 0)
						LDBGE("%s: ce1702_i2c_write failed(3) \n", __func__);

					wdata[0] = 0x00;
					wdata[1] = 0x80; //                                                    
					wdata[2] = 0x0C; //0x09;
					wdata[3] = 0x20;//0x320(800), 0xD0
					wdata[4] = 0x03; //0x07
					wdata[5] = 0x01;
					wdata[6] = 0x21; //0x20
					rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x90, wdata,  7);
					if (rc < 0)
						LDBGE("%s: ce1702_i2c_write failed(10) \n", __func__);

					LDBGI("ce1702_get_supported_size: ce1702_thumbnail_table\n");
					cnt = sizeof(ce1702_thumbnail_table) / sizeof(ce1702_thumbnail_table[0]);
					wdata[0] = 0x00;
					wdata[1] = 0x80; //                                                    
					wdata[2] = 0x0C; //0x09;
					wdata[3] = 0x20;//0x320(800), 0xD0
					wdata[4] = 0x03; //0x07
					wdata[5] = 0x01;
					wdata[6] = ce1702_get_supported_size(size_info.thumbnail_width, size_info.thumbnail_heigh, ce1702_thumbnail_table, cnt);
					rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x91, wdata,  7);
					if (rc < 0)
						LDBGE("%s: ce1702_i2c_write failed(11) \n", __func__);

					wdata[0] = 0x21;
					wdata[1] = 0x00;
					rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x07, wdata,  2);
					if (rc < 0)
						LDBGE("%s: ce1702_i2c_write failed(4) \n", __func__);

					ce1702_set_zoom_ratio(s_ctrl, ce1702_zoom_ratio);
					/*                                                                */

					wdata[0] = 0x14;
					wdata[1] = 0x01;
					rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x05, wdata,  2);
					if (rc < 0)
						LDBGE("%s: ce1702_i2c_write failed(5) \n", __func__);
					wdata[0] = 0x03;
					rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0xAE, wdata,  1);
					if (rc < 0)
						LDBGE("%s: ce1702_i2c_write failed(6) \n", __func__);

					wdata[0] = 0x00;
					rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x11, wdata,  1);
					if (rc < 0)
						LDBGE("%s: ce1702_i2c_write failed(7) \n", __func__);

					if(ce1702_cam_preview == PREVIEW_MODE_CAMCORDER)
						if((size_info.video_width==320 && size_info.video_height==240)||
							(size_info.video_width==176 && size_info.video_height==144)){
							LDBGE("%s: set 15fps\n", __func__);
							wdata[0] = 0x0F;		//set 15fps
						}
						else{
							LDBGE("%s: set 30fps camcorder\n", __func__);
							wdata[0] = 0x1E;		//set 30fps
						}
					else{
						LDBGE("%s: set 30fps camera\n", __func__);
						wdata[0] = 0x1E;		//set 30fps
					}
					wdata[1] = 0x00;
					rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x5A, wdata, 2);
					if (rc < 0)
						LDBGE("%s: ce1702_i2c_write failed(7) \n", __func__);
				}

				wdata[0] = 0x01;
				rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x6B, wdata,  1);
				if (rc < 0)
					LDBGE("%s: ce1702_i2c_write failed(8) \n", __func__);

				retry_cnt = 0;
				do{
					mdelay(10);
					//rdata[0] = 0x00;
					rc = ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x6C, NULL, 0,  &result,  1);
					if (rc < 0)
						LDBGE("%s: ce1702_i2c_read failed \n", __func__);
					retry_cnt++;
				} while ((result != 0x08) && (retry_cnt < 100));
				if(retry_cnt >= 100) {
					LDBGE("%s: %d: error to CE1702_MODE_PREVIEW \n", __func__, __LINE__);
					// event log
					//ce1702_store_isp_eventlog();
				}

				if(result == 0x08){
					isPreviewMode = 1;
					isSingleCaptureMode = 0;
					isTMSMode = 0;
					isBurstMode = 0;
					isHDRMode = 0;
					isLowLightShotMode = 0;
				}else{
					isPreviewMode = 0;
				}

				if (ce1702_cam_preview == PREVIEW_MODE_CAMCORDER) {
					LDBGI("%s: need for deferred AF!! \n", __func__);
					ce1702_deferred_af_start = 1;
				}

				LDBGI("%s: ce1702_i2c_read successed, CE1702_MODE_PREVIEW result=0x%02x, isPreviewMode=%d \n", __func__, result, isPreviewMode);
/*                                                                                */
					if(ce1702_asd_onoff){
						LDBGI("[G-young] %s : setting face detecting on \n",__func__);
						ce1702_set_face_detect(ce1702_asd_onoff);
					}
/*                                                                              */
			}
			break;

		case CE1702_MODE_SINGLE_SHOT:
			if(isSingleCaptureMode == 0){
				LDBGE("%s: CE1702_MODE_SINGLE_SHOT start\n", __func__);

				/*                                                                 */
				ce1702_sensor_set_led_flash_mode_snapshot(ce1702_led_flash_mode);
				/*                                                               */

				ce1702_set_exif_rotation_to_isp();

				if (ce1702_need_thumbnail == false) {
					LDBGE("%s: set remove exif data !!\n", __func__);
					ce1702_switching_exif_gps(false);
				}

				wdata[0] = 0x11;//aewb lock
				rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x11, wdata,  1);
				if (rc < 0)
					LDBGE("%s: ce1702_i2c_write failed(10) \n", __func__);

				wdata[0] = 0x00;	//Buffering Capture
				rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x74, wdata,  1);
				if (rc < 0)
					LDBGE("%s: ce1702_i2c_write failed(11) \n", __func__);

				retry_cnt = 0;
				do {
					mdelay(10);
					//rdata[0] = 0x00;
					rc = ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x6C, NULL, 0,  &result,  1);
					if (rc < 0)
						LDBGE("%s: ce1702_i2c_read failed \n", __func__);
					retry_cnt++;
				} while ((result != 0x01) && (retry_cnt < 100));
				if(retry_cnt >= 100) {
					LDBGE("%s: %d: error to CE1702_MODE_SINGLE_SHOT \n", __func__, __LINE__);
					// event log
					//ce1702_store_isp_eventlog();
					return;
				}

				if(result == 0x01){
					isPreviewMode = 0;
					isSingleCaptureMode = 1;
					isTMSMode = 0;
					isBurstMode = 0;
                 isHDRMode = 0;
                 isLowLightShotMode = 0;
				}else{
					isSingleCaptureMode = 0;
				}
				ce1702_deferred_af_start = 0;
				LDBGI("%s: ce1702_i2c_read successed, CE1702_MODE_SINGLE_SHOT result=%d, isSingleCaptureMode=%d \n", __func__, result, isSingleCaptureMode);
			}
			break;

		case CE1702_MODE_TMS_SHOT:
			if(isTMSMode ==0){
				LDBGI("%s: CE1702_MODE_TMS_SHOT start\n", __func__);
			        //                                                          
			        wdata[0] = 0x03;
			        rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0xAE, wdata,  1);
				if (rc < 0)
					LDBGE("%s: ce1702_i2c_write failed(12) \n", __func__);
			        //                                                          

			        cnt = sizeof(ce1702_postview_table) / sizeof(ce1702_postview_table[0]);
				/*                                                                  */
			        wdata[0] = ce1702_get_supported_size(size_info.picture_width, size_info.picture_height, ce1702_postview_table, cnt);
			        cnt = sizeof(ce1702_picture_table) / sizeof(ce1702_picture_table[0]);
				/*                                                                  */
			        wdata[1] = ce1702_get_supported_size(size_info.picture_width, size_info.picture_height, ce1702_picture_table, cnt);
			        wdata[2] = 0x06;
			        wdata[3] = 0x00;
			        wdata[4] = 0x00;
			        wdata[5] = 0x01;
			        wdata[6] = 0x00;

			        rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x62, wdata,  7);
				if (rc < 0)
					LDBGE("%s: ce1702_i2c_write failed(13) \n", __func__);

			        LDBGI("%s: TMC Size:: %d\n", __func__, wdata[0]);

			        // Multi-Image output setting
			        wdata[0] = 0x01;
				rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x63, wdata,  1);
				if (rc < 0)
					LDBGE("%s: ce1702_i2c_write failed(15) \n", __func__);

				do{
					mdelay(10);
					//rdata[0] = 0x00;
					rc = ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x6C, NULL, 0,  &result,  1);
					if (rc < 0)
						LDBGE("%s: ce1702_i2c_read failed \n", __func__);
					retry_cnt++;
				} while ((result != 0x18) && (retry_cnt < 100));
				if(retry_cnt >= 100) {
					LDBGE("%s: %d: error to CE1702_MODE_TMS_SHOT \n", __func__, __LINE__);
					// event log
					//ce1702_store_isp_eventlog();
				}

				if(result == 0x18){
					isPreviewMode = 0;
					isSingleCaptureMode = 0;
					isTMSMode = 1;
					isBurstMode = 0;
					isHDRMode = 0;
					isLowLightShotMode = 0;
				}else{
					isTMSMode = 0;
				}
				LDBGI("%s: ce1702_i2c_read successed, CE1702_MODE_TMS_SHOT result=%d, isTMSMode=%d \n", __func__, result, isTMSMode);
			}
			break;

		case CE1702_MODE_BURST_SHOT:
			if (isBurstMode ==0){
				LDBGI("%s: CE1702_MODE_BURST_SHOT start\n", __func__);
				wdata[0] = 0x20;
				wdata[1] = 0x00;
				ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x07, wdata,  2);

				cnt = sizeof(ce1702_postview_table) / sizeof(ce1702_postview_table[0]);
				wdata[0] = ce1702_get_supported_size(size_info.picture_width, size_info.picture_height, ce1702_postview_table, cnt);
				wdata[1] = 0x05;
				ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x55, wdata,  2);	// postview setting

				cnt = sizeof(ce1702_picture_table) / sizeof(ce1702_picture_table[0]);
				wdata[0] = ce1702_get_supported_size(size_info.picture_width, size_info.picture_height, ce1702_picture_table, cnt);
				ce1702_size_info = wdata[0];
				wdata[1] = 0x05;	// burstshot mode
				wdata[2] = 0x11;
				wdata[3] = 0x00;
				ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x73, wdata,  4);	// capture setting

				wdata[0] = 0x00;
				wdata[1] = 0x00;
				ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x71, wdata,  2);	// No of Capture Setting

				wdata[0] = 0x00;
				wdata[1] = 0xD0;
				wdata[2] = 0x0E;	//38%
				wdata[3] = 0x08;
				wdata[4] = 0x07;	//18%
				wdata[5] = 0x01;
				wdata[6] = 0x21;
				rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x90, wdata,  7);

				wdata[0] = 0x10;
				wdata[1] = 0x02;
				ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x96, wdata,  2);

				cnt = sizeof(ce1702_thumbnail_table) / sizeof(ce1702_thumbnail_table[0]);
				wdata[0] = 0x00;
				wdata[1] = 0x80;
				wdata[2] = 0x0C;
				wdata[3] = 0x8C;
				wdata[4] = 0x0A;
				wdata[5] = 0x01;
				wdata[6] = ce1702_get_supported_size(size_info.thumbnail_width, size_info.thumbnail_heigh, ce1702_thumbnail_table, cnt);
				rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x91, wdata,  7);

				ce1702_set_exif_rotation_to_isp();

				wdata[0] = 0x11;	//aewb lock
				ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x11, wdata,  1);

				wdata[0] = 0x00;	//Buffering Capture
				ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x74, wdata,  0);

				retry_cnt = 0;
				result = 0;
				do {
					rc = ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x6C, NULL, 0, &result,	1);
					if (rc < 0)
						LDBGE("%s: ce1702_i2c_read failed \n", __func__);
					if(result == 0x01)
						break;
					mdelay(10);
					retry_cnt++;
				} while (retry_cnt < 100);

				if(retry_cnt >= 100) {
					LDBGE("%s: %d: error to CE1702_MODE_BURST_SHOT \n", __func__, __LINE__);
					//event log
					//ce1702_store_isp_eventlog();
					return;
				}

				if(result == 0x01){
					isPreviewMode = 0;
					isSingleCaptureMode = 0;
					isTMSMode = 0;
					isBurstMode = 1;
                 isHDRMode = 0;
                 isLowLightShotMode = 0;
				}else{
					isBurstMode = 0;
				}
				ce1702_deferred_af_start = 0;
				LDBGI("%s: ce1702_i2c_read successed, CE1702_MODE_BURST_SHOT result=%d, isBurstMode=%d \n", __func__, result, isBurstMode);
			}
			break;
      case CE1702_MODE_HDR_SHOT:
			LDBGE("%s: CE1702_MODE_HDR_SHOT start\n", __func__);
			wdata[0] = 0x11;
			ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x11, wdata,  1);
			LDBGI("%s: test 11 cmd!!\n", __func__);

			wdata[0] = 0x00;  //Buffering Capture
			ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x74, wdata,  1);

			retry_cnt = 0;
			do {
				mdelay(10);
				//rdata[0] = 0x00;
				ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x6C, NULL, 0,  &result,  1);
				retry_cnt++;
			} while ((result != 0x01) && (retry_cnt < 100));

			if(retry_cnt >= 100) {
				LDBGE("%s: %d: error to capture with CE1702_MODE_HDR_SHOT\n", __func__, __LINE__);
				// event log
				//ce1702_store_isp_eventlog();
				return;
			}

			if(result == 0x01){
				isPreviewMode = 0;
				isSingleCaptureMode = 0;
				isTMSMode = 0;
				isBurstMode = 0;
				isHDRMode = 1;
				isLowLightShotMode = 0;
			}else{
				isHDRMode = 0;
			}
			//mdelay(40); //yt.jeon 1114 optimize delay time

			LDBGI("%s: ce1702_i2c_read successed, CE1702_MODE_HDR_SHOT result=%d, isHDRMode=%d \n", __func__, result, isHDRMode);
			break;
		case CE1702_MODE_LOW_LIGHT_SHOT:
			if (isLowLightShotMode ==0){
				LDBGE("%s: CE1702_MODE_LOW_LIGHT_SHOT start\n", __func__);
				wdata[0] = 0x11;
				ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x11, wdata,  1);
				LDBGI("%s: test 11 cmd!!\n", __func__);

				wdata[0] = 0x00;  //Buffering Capture
				ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x74, wdata,  1);

				retry_cnt = 0;
				do {
					mdelay(10);
					//rdata[0] = 0x00;
					ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x6C, NULL, 0,  &result,  1);
					retry_cnt++;
				} while ((result != 0x01) && (retry_cnt < 100));

				if(retry_cnt >= 100) {
					LDBGE("%s: %d: error to capture with CE1702_MODE_LOW_LIGHT_SHOT\n", __func__, __LINE__);
					// event log
					//ce1702_store_isp_eventlog();
					return;
				}

				if(result == 0x01){
					isPreviewMode = 0;
					isSingleCaptureMode = 0;
					isTMSMode = 0;
					isBurstMode = 0;
					isHDRMode = 0;
					isLowLightShotMode = 1;
				}else{
					isLowLightShotMode = 0;
				}
				//mdelay(40); //yt.jeon 1114 optimize delay time
				ce1702_deferred_af_start = 0;

				LDBGI("%s: ce1702_i2c_read successed, CE1702_MODE_LOW_LIGHT_SHOT result=%d, isLowLightShotMode=%d \n", __func__, result, isLowLightShotMode);
			}
			break;
		default:
			LDBGE("%s: ce1702_frame_mode is not exist\n", __func__);
			break;
	}
	/*
		Comment by jungki.kim
		We need to excute these function lately because these have to try under previewing condition.
	*/
	ce1702_sensor_set_param_lately(s_ctrl);
}

void ce1702_sensor_stop_stream(struct msm_sensor_ctrl_t *s_ctrl)
{
	uint8_t result =0;
	int retry_cnt = 0;
	uint8_t wdata=0;
	int32_t rc = 0;

	LDBGI("%s: isPreviewMode=%d,  isSingleCaptureMode =%d, isTMSMode=%d, isBurstMode =%d, isHDRMode=%d, isLowLightShotMode=%d\n", __func__, isPreviewMode, isSingleCaptureMode, isTMSMode, isBurstMode, isHDRMode, isLowLightShotMode);

	//stop Lens
	if(ce1702_frame_mode < CE1702_FRAME_MAX){
		rc = ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x24, NULL, 0, &result,  1);
		if (rc < 0)
			LDBGE("%s: ce1702_i2c_read failed \n", __func__);
		if ((result & 0x01) == 0x01) {
			wdata = 0x00;
			rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x35, &wdata,  1);	 // Stop VCM
			if (rc < 0)
				LDBGE("%s: ce1702_i2c_write failed(1) \n", __func__);
			mdelay(33); //yt.jeon 1115 optimize delay time

			retry_cnt = 0;
			do {
				rc = ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x24, NULL, 0, &result,	1);
				if (rc < 0)
					LDBGE("%s: ce1702_i2c_read failed \n", __func__);
				if ((result & 0x01) == 0){
					LDBGI("%s: stop lens ce1702_i2c_read successed, result=%d\n", __func__, result);
					break;
				}
				retry_cnt++;
				mdelay(5); //yt.jeon 1115 optimize delay time
			} while (retry_cnt < 100);  //lens running
		}
	}

	switch (ce1702_frame_mode) {
		case CE1702_MODE_PREVIEW :
			if (isPreviewMode == 1){
				LDBGI("%s: CE1702_MODE_PREVIEW stop\n", __func__);
				wdata = 0x00;
				rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x6B, &wdata,  1);
				if (rc < 0)
					LDBGE("%s: ce1702_i2c_write failed(2) \n", __func__);
				retry_cnt = 0;
				do
				{
					rc = ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x6C, NULL, 0,  &result,  1);
					if (rc < 0)
						LDBGE("%s: ce1702_i2c_read failed \n", __func__);
					if (result == 0x00)
						break;

					mdelay(10);
					retry_cnt++;
					} while (retry_cnt < 100);

				if(retry_cnt >= 100) {
					LDBGE("%s: %d: error to CE1702_MODE_PREVIEW \n", __func__, __LINE__);
					// event log
					//ce1702_store_isp_eventlog();
					return;
				}

				if(result == 0x00){
					isPreviewMode = 0;
				}
				LDBGI("%s: ce1702_i2c_read successed, result=%d, isPreviewMode=%d \n", __func__, result, isPreviewMode);
			}
			break;

		case CE1702_MODE_SINGLE_SHOT :
		case CE1702_MODE_BURST_SHOT :
		case CE1702_MODE_HDR_SHOT:
		case CE1702_MODE_LOW_LIGHT_SHOT:
			if ((isSingleCaptureMode == 1) ||(isBurstMode == 1) || (isHDRMode == 1) ||(isLowLightShotMode== 1)){
				LDBGI("%s: CE1702_MODE_SINGLE_SHOT(BURST_SHOT) stop\n", __func__);
				wdata = 0x00;
				rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x75, &wdata,  1);
				if (rc < 0)
					LDBGE("%s: ce1702_i2c_write failed(3) \n", __func__);
				retry_cnt = 0;
				do
				{
					rc = ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x6C, NULL, 0,  &result,  1);
					if (rc < 0)
						LDBGE("%s: ce1702_i2c_read failed \n", __func__);
					if (result == 0x00)
						break;

					mdelay(10);
					retry_cnt++;
				} while (retry_cnt < 100);
				if(retry_cnt >= 100) {
					LDBGE("%s: %d: error to CE1702_MODE_SINGLE_SHOT, CE1702_MODE_BURST_SHOT \n", __func__, __LINE__);
					// event log
					//ce1702_store_isp_eventlog();
					return;
				}

				if(result == 0x00){
					isSingleCaptureMode  = 0;
					isBurstMode = 0;
                 isHDRMode = 0;
                 isLowLightShotMode = 0;
				}
				LDBGE("%s: ce1702_i2c_read successed, result=%d, isSingleCaptureMode =%d, isHDRMode=%d, isLowLightShotMode=%d\n", __func__, result, isSingleCaptureMode, isHDRMode, isLowLightShotMode );
			}
			break;

		case CE1702_MODE_TMS_SHOT :
			if (isTMSMode == 1){
				LDBGI("%s: CE1702_MODE_TMS_SHOT stop\n", __func__);
				wdata = 0x00;
				rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x63, &wdata,  1);
				if (rc < 0)
					LDBGE("%s: ce1702_i2c_write failed(4) \n", __func__);
				retry_cnt = 0;
				do
				{
					rc = ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x6C, NULL, 0, &result,  1);
					if (rc < 0)
						LDBGE("%s: ce1702_i2c_read failed \n", __func__);
					if (result == 0x00)
						break;

					mdelay(10);
					retry_cnt++;
				} while (retry_cnt < 100);
				if(retry_cnt >= 100) {
					LDBGE("%s: %d: error to CE1702_MODE_TMS_SHOT \n", __func__, __LINE__);
					// event log
					//ce1702_store_isp_eventlog();
					return;
				}

				if(result == 0x00){
					isTMSMode= 0;
				}
				LDBGE("%s: ce1702_i2c_read successed, result=%d, isTMSMode =%d \n", __func__, result, isTMSMode );
			}
			break;
		default :
			LDBGE("%s:ce1702_frame_mode is not matched \n", __func__);
			break;
	}
	/*
		TODO: Need to know which preview mode is
		because 'Preview Assist Mode' command have to be excuted before previewing...
		Comment by jungki.kim

		Temporarily set to preview mode.
	*/
	ce1702_set_preview_assist_mode(s_ctrl, PREVIEW_MODE_CAMERA);
}

void ce1702_set_preview_assist_mode(struct msm_sensor_ctrl_t *s_ctrl, uint8_t mode)
{
	unsigned char data = 0;
	LDBGI("%s: mode = %d\n", __func__, mode);

	switch(mode) {
		case PREVIEW_MODE_CAMERA:
			data = 0x00;
			break;

		case PREVIEW_MODE_CAMCORDER:
			data = 0x01;
			break;

		default:
			data = 0x00;
			LDBGE("%s: Error. No preview mode was set! [%d]\n", __func__, mode);
			break;
	}
	if(ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x40, &data, 1)<0)
		LDBGE("%s: ce1702_i2c_write failed \n", __func__);
}

//                                   
// CMD : 0x3D
// DATA : 0x05
int32_t ce1702_set_special_effect(struct msm_sensor_ctrl_t *s_ctrl, uint8_t effect)
{
	int32_t rc = CE1702_OK;
	unsigned char data[2];
	//uint8_t rdata[2];
	uint8_t res;
	int cnt;
	LDBGI("%s: effect=%d\n", __func__, effect);

	data[0] = 0x05; // effect

	switch(effect) {
		case CAMERA_EFFECT_OFF :
			data[1] = 0x00;
			break;
		case CAMERA_EFFECT_MONO :
			data[1] = 0x01;
			break;
		case CAMERA_EFFECT_NEGATIVE :
			data[1] = 0x02;
			break;
		case CAMERA_EFFECT_SEPIA :
			data[1] = 0x03;
			break;
		default :
			data[1] = 0x00;
			break;
	}
	rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x3D, data, 2);
	if(rc < 0) {
		LDBGE("%s: ce1702_i2c_write failed(1)!! Fail to apply effect [%d]!!\n", __func__, effect);
		return FALSE;
	}
	data[0] = 0x00;
	rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x01, data, 0); // Apply!!
	if (rc < 0)
		LDBGE("%s: ce1702_i2c_write failed(2) \n", __func__);
	cnt = 0;
	do {
		mdelay(5); //yt.jeon 1115 optimize delay time
		//rdata[0] = 0x00;
		rc = ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x02, NULL, 0, &res, 1);
		if (rc < 0)
			LDBGE("%s: ce1702_i2c_read failed \n", __func__);
		LDBGI("%s: verifying... cnt=%d rc=%d\n", __func__, cnt, rc);
		cnt++;
	} while ( (res != 0) && (cnt < 100) );

	if(cnt >= 100) {
		LDBGE("%s: Fail to read isp status \n", __func__);
		rc = - EIO;
	}

	LDBGI("%s: SUCCESS! rc=%d\n", __func__, rc);
	return rc;
}

//                                 
// CMD : 0x04
// DATA : 0x02
//To-Do : very little change. Need to improve!
int32_t ce1702_set_exposure_compensation(struct msm_sensor_ctrl_t *s_ctrl, uint8_t exposure)
{
	int32_t rc = 0;
	unsigned char data[2];
	//uint8_t rdata[2];
	uint8_t res;
	int cnt;

	int8_t value = (int8_t)exposure;
	int8_t adj = (int8_t)(value/2)+6;

	LDBGI("%s: exposure = %d adj=[%d] rc=%d\n", __func__, value, adj, rc);

	if(ce1702_brightness == adj) {
		LDBGI("%s: just before value[%d] = requested value[%d] (SAME!)\n", __func__, ce1702_brightness, adj);
		return rc;
	}

	data[0] = 0x02; 	// brightness??
	data[1] = adj;	// value

	rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x04, data, 2);
	if(rc < 0) {
		LDBGE("%s: ce1702_i2c_write failed(1) !! Fail to apply brightness [%d]!!\n", __func__,adj);
		return FALSE;
	}
	data[0] = 0x00;
	rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x01, data, 0); // Apply!!
	if (rc < 0)
		LDBGE("%s: ce1702_i2c_write failed(2) \n", __func__);

	cnt = 0;
	do {
		mdelay(5); //yt.jeon 1115 optimize delay time
		//rdata[0] = 0x00;
		rc = ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x02, NULL, 0, &res, 1);
		if (rc < 0)
			LDBGE("%s: ce1702_i2c_read failed \n", __func__);
		LDBGI("%s: verifying... cnt=%d rc=%d\n", __func__, cnt, rc);
		cnt++;
	} while ( (res != 0) && (cnt < 100) );

	if(cnt >= 100) {
		LDBGE("%s: Fail to read isp status \n", __func__);
		rc = - EIO;
	}
	ce1702_brightness = adj;

	return rc;
}

// CAF Setting for CE1702 by jungki.kim
int8_t ce1702_set_caf(int mode)
{
	int8_t rc = 0, res;
	uint8_t data[4];
	int cnt;

	LDBGI("%s: Enter with [%d]\n",__func__, mode);

	switch(mode) {
		case AF_MODE_CAF_VIDEO:
			LDBGI("%s: start [ AF_MODE_CAF_VIDEO ]\n", __func__);

			// 1. Setting AF mode (CMD: 0x20)
			data[0] = 0x02; // AF-C
			rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x20, data, 1);
			LDBGI("%s: %d: check rc = [%d]\n",__func__, __LINE__, rc);

			// 2. Check AF Searching Status (CMD:0x24)
			ce1702_check_af_status(TRUE);

			// 3. Start CAF(CMD:0x23)
			rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x23, NULL, 0);
			break;

		case AF_MODE_CAF_PICTURE:
			LDBGI("%s: start [ AF_MODE_CAF_PICTURE ]\n", __func__);

			data[0] = 0x00; // AF-T
			rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x20, data, 1);
			// 2. Check AF Searching Status (CMD:0x24)
			ce1702_check_af_status(TRUE);

			ce1702_sensor_set_led_flash_mode_for_AF(FLASH_LED_OFF);
			data[0] = 0x01;
			rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x2C, data, 1);
			cnt = 0;
			do {
				rc = ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x2D, NULL, 0, &res, 1);
				if (res == 0x01) {
					break;
				}
				mdelay(5);
				cnt++;
			} while (cnt < 100);
			break;

		default:
			break;
	}

	return rc;
}

//Start AF for CE1702 by jungki.kim
int8_t ce1702_start_af(struct msm_sensor_ctrl_t *s_ctrl)
{
	int8_t rc = 0;
	int cnt = 0;
	uint8_t res = 0;
	uint8_t data = 0;

	if (ce1702_frame_mode == CE1702_FRAME_MAX) {
		ce1702_deferred_af_start = 1;
		LDBGI("%s: deferred af_start !!\n", __func__);
		return rc;
	}

	//ce1702_deferred_af_start = 0;
	if (ce1702_focus_mode == AF_MODE_CAF_PICTURE) {
		ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x2D, NULL, 0, &res, 1);
		if (res == 0x01) {
			LDBGI("%s: no need to start [ AF_MODE_CAF_PICTURE ]\n", __func__);
			return rc;
		}
	}

	// stop lens
	ce1702_stop_lens(s_ctrl);

	// check AREA
	if (ce1702_is_doing_touchaf) {
		LDBGI("%s: Start [TOUCH AF]\n", __func__);
		//data = 0x00;
		//ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x20, &data, 1);
		//ce1702_check_af_status(FALSE);
		if ((ce1702_focus_mode == AF_MODE_CAF_PICTURE) || (ce1702_focus_mode == AF_MODE_CAF_VIDEO)) {
			// check AREA
			ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x43, NULL, 0, &res, 1);
			if (res == 0x05) {
				int16_t window[4] = {0, };

				LDBGI("%s: Stop [TOUCH AF]\n", __func__);
				ce1702_set_window(NULL, window, SET_AREA_AF_OFF);
			}
			ce1702_is_doing_touchaf = false;
		}
		else {
			/*                                                                     */
			//ce1702_sensor_set_led_flash_mode_for_AF(ce1702_led_flash_mode);
			if(!ce1702_just_changed_to_preview)	ce1702_sensor_set_led_flash_mode_for_AF(ce1702_led_flash_mode);
			/*                                                                   */
			data = 0x05; // Area ON!
			rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x42, &data, 1); // control preview assist
			cnt = 0;
			do {
				rc = ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x43, NULL, 0, &res,  1);
				if(res == 0x05) break;	// check sync area
				mdelay(5); //yt.jeon 1115 optimize delay time
				cnt++;
			} while (cnt < 100);
		}

	}

	switch(ce1702_focus_mode) {
		case AF_MODE_MACRO:
		case AF_MODE_AUTO:
			LDBGI("%s: Start [AUTO, MACRO AF]\n", __func__);
			data = ce1702_focus_mode == AF_MODE_MACRO ? 0x01 : 0x00;
			ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x20, &data, 1);
			ce1702_check_af_status(TRUE);
			/*                                                                     */
			//ce1702_sensor_set_led_flash_mode_for_AF(ce1702_led_flash_mode);
			if(!ce1702_just_changed_to_preview)	ce1702_sensor_set_led_flash_mode_for_AF(ce1702_led_flash_mode);
			/*                                                                   */
			rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x23, NULL, 0);
			cnt = 0;
			do {
				rc = ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x24, NULL, 0, &res, 1);
				if ((res & 0x01))
					break;
				mdelay(10);
				cnt++;
			} while (cnt < 100);	//lens running
			break;
		case AF_MODE_CAF_VIDEO:
			LDBGI("%s: Start [CAF VIDEO, PICTURE AF]\n", __func__);
			ce1702_set_caf(ce1702_focus_mode);
			break;
		case AF_MODE_CAF_PICTURE:
			LDBGI("%s: Start [##CAF VIDEO, PICTURE AF]\n", __func__);
			ce1702_aec_awb_lock = 0;
			ce1702_set_aec_awb_lock(s_ctrl, 0);
			ce1702_set_caf(ce1702_focus_mode);
			break;
		case AF_MODE_INFINITY:
		default:
			break;
	}

	return rc;
}

//Stop AF for CE1702 by jungki.kim
int8_t ce1702_stop_af(struct msm_sensor_ctrl_t *s_ctrl)
{
	int8_t rc = 0;
	uint8_t data[4];
	uint8_t rdata[4] = {0, };
	int cnt;
	int16_t window[4] = {0, };

	// stop lens
	LDBGI("%s: Stop Lens [AF]\n", __func__);
	ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x35, NULL, 0); // stop
	cnt = 0;
	do {
		rc = ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x24, NULL, 0, rdata, 1);
		if ((rdata[0] & 0x01) == 0)
			break;
		mdelay(10);
		LDBGI("%s: lens stop check [count %d]	= %x \n", __func__, cnt, rdata[0]);
		cnt++;
	} while (cnt < 100);	//lens running

	// check AF-T
	cnt = 0;
	ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x2D, NULL, 0, rdata, 2);
	if (rdata[0] == 1) {
		data[0] = 0x00;
		ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x2C, data, 1); // stop AF-T
		do {
			rc = ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x2D, NULL, 0, rdata, 2);
			if (rdata[0] == 0)
				break;
			mdelay(10);
			LDBGI("%s: AF-T stop check [count %d]	= %x \n", __func__, cnt, rdata[0]);
			cnt++;
		} while (cnt < 100);	//lens running

	}

	// check AREA
	ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x43, NULL, 0, rdata, 1);
	if (rdata[0] == 0x05) {
		LDBGI("%s: Stop [TOUCH AF]\n", __func__);
		ce1702_set_window(s_ctrl, window, SET_AREA_AF_OFF);
	}
/*                                                                                */
	if(ce1702_asd_onoff){
		LDBGI("[G-young] %s : setting face detecting on \n",__func__);
		ce1702_set_face_detect(ce1702_asd_onoff);
	}
/*                                                                              */
	ce1702_is_doing_touchaf = false;
	return rc;
}

uint8_t ce1702_check_af_status(bool ispolling)
{
	uint8_t rdata = 0;
	int cnt;

	switch(ispolling) {
		case TRUE:
			cnt = 0;
			LDBGI("%s: Polling Start\n", __func__);
			do {
				ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x24, NULL, 0, &rdata,  1);
				if ((rdata & 0x01) == 0x00)
					break;
				mdelay(5);
				cnt++;
			} while (cnt < 100); 	//lens running
			LDBGI("%s: Polling End Count=[%d]\n", __func__, cnt);
			return TRUE;
			break;

		case FALSE:
			ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x24, NULL, 0, &rdata,  1);
			LDBGI("%s: stop check [single count] = 0x%02x \n", __func__, rdata);
			return rdata;
			break;
	}
	return 0;
}

int8_t ce1702_set_window(struct msm_sensor_ctrl_t *s_ctrl, int16_t *window, int16_t sw)
{
	int8_t rc = 0;
	unsigned char data[10];
	uint8_t rdata[2];
	uint8_t res=0;
	int cnt;

	LDBGI("%s: x=[%d] y=[%d] rx=[%d] ry=[%d] MODE=[%d]\n", __func__, window[0], window[1], window[2], window[3], sw);

	// Check if it is previewing or not.
	rdata[0] = 0x00; //NULL
	rc = ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x6C, rdata, 0, &res, 1);
	if( (res != 0x08) &&(res != 0x18) ) {
		LDBGI("%s: Sensor is not previewing... value=[%x] return\n", __func__, res);
		return -1;
	}

	if(ce1702_focus_mode != AF_MODE_CAF_VIDEO) {

		if(sw >= SET_AREA_AF_OFF) {
			data[0] = 0x00; // OFF
			rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x42, data, 1); // control preview assist
			cnt = 0;
			do {
				rc = ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x43, NULL, 0, &res,  1);
				if(res == 0x00) break;	// check
				mdelay(5); //yt.jeon 1115 optimize delay time
				cnt++;
			} while (cnt < 100);
			return rc;
		}

		switch(sw) {
			case SET_AREA_AF:
				LDBGI("%s: Mode Set= SET_AREA_AF\n",__func__);
				data[0] = 0x20;	// AF Area
				data[1] = 0xB3; 	// Enable Area
				break;
			case SET_AREA_AE:
				LDBGI("%s: Mode Set= SET_AREA_AE\n",__func__);
				data[0] = 0x10;	// AE Area
				data[1] = 0xB2; 	// Enable Area
				break;
			default:
				LDBGI("%s: Invalid parameter [%d] return\n",__func__, sw);
				return -1;
				break;
		}

		data[2] = window[0] & 0xFF; // x lower
		data[3] = window[0] >>0x08; // x upper
		data[4] = window[1] & 0xFF; // y lower
		data[5] = window[1] >>0x08; // y upper
		data[6] = window[2] & 0xFF; // x lower
		data[7] = window[2] >>0x08; // x upper
		data[8] = window[3] & 0xFF; // y lower
		data[9] = window[3] >>0x08; // y upper
		rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x41, data, 10); // preview assist
		if (rc < 0)
			LDBGE("%s: ce1702_i2c_write failed(1) \n", __func__);

		if( sw == SET_AREA_AF) {
			// Set Area for Exposure
			data[0] = 0x10;
			data[1] = 0xB3;
			rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x41, data, 10); // preview assist
			if (rc < 0)
				LDBGE("%s: ce1702_i2c_write failed(2) \n", __func__);
		}


	}

	LDBGI("%s: X\n", __func__);
	return rc;
}

//Set AE Window for CE1702 by jungki.kim
int8_t ce1702_set_ae_window(struct msm_sensor_ctrl_t *s_ctrl, int16_t *window)
{
	int8_t rc = 0;

	//LDBGI("%s: LT[%d,%d] RB[%d,%d]\n", __func__, window[0], window[1], window[2], window[3]);

	//rc = ce1702_set_window(s_ctrl, window, SET_AREA_AE);
	if(rc < 0) {
		LDBGE("%s: AE Area does NOT set...\n", __func__);
	}

	return rc;
}

//Set AF Window for CE1702 by jungki.kim
int8_t ce1702_set_af_window(struct msm_sensor_ctrl_t *s_ctrl, int16_t *window)
{
	int8_t rc = 0;

	ce1702_is_doing_touchaf = true;
	rc = ce1702_set_window(s_ctrl, window, SET_AREA_AF);
	if(rc < 0) {
		LDBGE("%s: AF Area does NOT set... [ SET_AREA_AF ]\n", __func__);
	}

	return rc;
}

int32_t ce1702_stop_lens(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	uint8_t data[4];
	uint8_t rdata[4] = {0, };
	int cnt;

	// stop lens
	LDBGI("%s: Stop Lens [AF]\n", __func__);
	ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x35, NULL, 0); // stop
	cnt = 0;
	do {
		rc = ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x24, NULL, 0, rdata, 1);
		if ((rdata[0] & 0x01) == 0)
			break;
		mdelay(10);
		cnt++;
	} while (cnt < 100);	//lens running
	LDBGE("%s: lens stop check [count %d]	= %x \n", __func__, cnt, rdata[0]);

	// check AF-T
	cnt = 0;
	ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x2D, NULL, 0, rdata, 2);
	if (rdata[0] == 1) {
		data[0] = 0x00;
		ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x2C, data, 1); // stop AF-T
		do {
			rc = ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x2D, NULL, 0, rdata, 2);
			if (rdata[0] == 0)
				break;
			mdelay(10);
			cnt++;
		} while (cnt < 100);	//lens running
	}

	ce1702_sensor_set_led_flash_mode_for_AF(FLASH_LED_OFF);
	LDBGE("%s: AF-T stop check [count %d]	= %x \n", __func__, cnt, rdata[0]);

	return 0;
}

//AF Mode Settings for CE1702 by jungki.kim
int8_t ce1702_set_focus_mode_setting(struct msm_sensor_ctrl_t *s_ctrl, int32_t afmode)
{
	int8_t rc = 0;

	LDBGI("%s: afmode: [%d] \n", __func__, afmode);

	if (ce1702_frame_mode == CE1702_FRAME_MAX) {
		ce1702_deferred_af_mode = afmode;
		LDBGI("%s: deferred afmode: [%d] !!\n", __func__, afmode);
		return rc;
	}

	if (afmode == AF_MODE_CAF_PICTURE) {
		LDBGI("%s: need for deferred AF!! \n", __func__);
		ce1702_deferred_af_start = 1;
	}

	ce1702_focus_mode = afmode;

	return rc;

}

//White Balance Settings for CE1702 by jungki.kim
int8_t ce1702_set_wb_setting(struct msm_sensor_ctrl_t *s_ctrl, uint8_t wb)
{
	int8_t rc = 0;
	unsigned char data[2];
	//uint8_t rdata[2];
	uint8_t res, wb_mode;
	int cnt;

	LDBGI("%s: cee1702_wb_mode:[%d] wb: [%d] \n", __func__, ce1702_wb_mode, wb);
	if(ce1702_wb_mode == wb) {
		LDBGE("%s: just before value[%d] = requested value[%d] (SAME!)\n", __func__, ce1702_wb_mode, wb);
		return rc;
	}

	switch (wb) {
		case CAMERA_WB_AUTO:
			LDBGI("%s: setting CAMERA_WB_AUTO\n", __func__);
			wb_mode = 0x00;
			data[0] = 0x11;
			data[1] = 0x00;
			break;
		case CAMERA_WB_INCANDESCENT:
			LDBGI("%s: setting CAMERA_WB_INCANDESCENT\n", __func__);
			wb_mode = 0x01;
			data[0] = 0x10;
			data[1] = 0x04;
			break;
		case CAMERA_WB_DAYLIGHT:
			LDBGI("%s: setting CAMERA_WB_DAYLIGHT\n", __func__);
			wb_mode = 0x01;
			data[0] = 0x10;
			data[1] = 0x01;
			break;
		case CAMERA_WB_FLUORESCENT:
			wb_mode = 0x01;
			data[0] = 0x10;
#if defined(CONFIG_MACH_APQ8064_GKKT) ||defined(CONFIG_MACH_APQ8064_GKSK)||defined(CONFIG_MACH_APQ8064_GKU) || defined(CONFIG_MACH_APQ8064_OMEGAR) || defined(CONFIG_MACH_APQ8064_OMEGA) || defined(CONFIG_MACH_APQ8064_GVKT)
			data[1] = 0x03;
			LDBGI("%s: setting CAMERA_WB_FLUORESCENT for F240 (TL84)  value = %d\n", __func__, data[1]);
#else
			data[1] = 0x05;
			LDBGI("%s: setting CAMERA_WB_FLUORESCENT for Global (Osram) value = %d\n", __func__, data[1]);
#endif
			break;
		case CAMERA_WB_CLOUDY_DAYLIGHT:
			LDBGI("%s: setting CAMERA_WB_CLOUDY_DAYLIGHT\n", __func__);
			wb_mode = 0x01;
			data[0] = 0x10;
			data[1] = 0x02;
			break;
		case CAMERA_WB_OFF:
			LDBGI("%s: setting CAMERA_WB_OFF (means WB auto!?)\n", __func__);
			wb_mode = 0x00;
			data[0] = 0x11;
			data[1] = 0x00;
			break;
		default:
			LDBGI("%s: setting default\n", __func__);
			wb_mode = 0x00;
			data[0] = 0x11;
			data[1] = 0x00;
			break;
	}

	rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x1A, &wb_mode, 1);	//WB Mode Setting
	if (rc < 0)
		LDBGE("%s: ce1702_i2c_write failed(1) \n", __func__);
	rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x04, data, 2);	//preset
	if (rc < 0)
		LDBGE("%s: ce1702_i2c_write failed(2) \n", __func__);
	data[0]=0x00; //NULL
	rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x01, data, 0);
	if (rc < 0)
		LDBGE("%s: ce1702_i2c_write failed(3) \n", __func__);
	cnt = 0;
	do {
		mdelay(5); //yt.jeon 1115 optimize delay time
		//rdata[0] = 0x00;
		rc = ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x02, NULL, 0, &res, 1);
		if (rc < 0)
			LDBGE("%s: ce1702_i2c_read failed \n", __func__);
		LDBGI("%s: WB setting check[%d] = %x \n", __func__, cnt, res);
		cnt++;
	} while((res != 0) && (cnt < 500));

	if (cnt >=500)
		rc = -EIO;

	ce1702_wb_mode = wb;
	return rc;
}

#define CAF_STOP_HERE 1
//Zoom Ratio Settings for CE1702 by jungki.kim
int8_t ce1702_set_zoom_ratio(struct msm_sensor_ctrl_t *s_ctrl, int32_t zoom)
{
	int8_t rc = 0;
	unsigned char data = 0;
	uint8_t rdata[2] = {0, 0};
	int cnt;
	if(isSingleCaptureMode == 1){
		return TRUE;
	}

	LDBGI("%s: zoom ratio=[%d] focus_mode=[%d]\n", __func__, zoom, ce1702_focus_mode);

#ifdef CAF_STOP_HERE
	if(isTMSMode) {
		LDBGI("%s: TMSMode Zoom\n", __func__);
		data = 0x03;
		rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x2C, &data, 1);
		if (rc < 0) LDBGE("%s: ce1702_i2c_write failed(1) \n", __func__);
	}
#endif

	data = zoom;
	rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0xB9, &data, 1);	// zoom ratio
	if(rc < 0) {
		LDBGE("%s: ce1702_i2c_write failed(2). Fail to apply zoom [%d]!!\n", __func__, zoom);
		return FALSE;
	}

	cnt = 0;
	LDBGI("%s: [%d]: now polling START!\n", __func__, cnt);
	do {
		//data[0] = 0x00;
		rc = ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0xBA, NULL, 0, rdata, 2);
		if (rc < 0)
			LDBGE("%s: ce1702_i2c_read failed \n", __func__);
		if (rdata[1] == 0)
			break;
		mdelay(5); // Do not reduce delay!
		//LDBGI("%s: verifying... cnt=%d rc=%d rdata[1]=[%d]\n", __func__, cnt, rc, rdata[1]);
		cnt++;
	} while ( cnt < 100 );
	LDBGI("%s: [%d]: now polling END!\n", __func__, cnt);

	if(cnt >= 100) {
		LDBGE("%s: Fail to read isp status \n", __func__);
		rc = - EIO;
	}

#ifdef CAF_STOP_HERE
	if(isTMSMode) {
		LDBGI("%s: TMSMode Zoom\n", __func__);
		data = 0x01;
		rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x2C, &data, 1);
		if (rc < 0) LDBGE("%s: ce1702_i2c_write failed(1) \n", __func__);
	}
#endif
	ce1702_zoom_ratio = zoom;
	return rc;
}

int8_t ce1702_set_manual_focus_length(struct msm_sensor_ctrl_t *s_ctrl, int32_t focus_val)
{
	int8_t rc = 0;
	int cnt;
	uint8_t rdata = 0;
	uint8_t data[10];

	LDBGI("%s: manual focus value=%d\n", __func__, focus_val);

	if (ce1702_frame_mode == CE1702_FRAME_MAX) {
		ce1702_deferred_focus_length = focus_val;
		LDBGI("%s: deferred focus length: [%d] !!\n", __func__, focus_val);
		return rc;
	}

	// check AF-T
	cnt = 0;
	ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x2D, NULL, 0, &rdata, 1);
	if (rdata== 1) {
		data[0] = 0x00;
		ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x2C, data, 1); // stop AF-T
		do {
			rc = ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x2D, NULL, 0, &rdata, 1);
			if (rdata == 0)
				break;
			mdelay(10);
			cnt++;
		} while (cnt < 100);	//lens running
	}
	LDBGE("%s: AF-T stop check [count %d]	= %x \n", __func__, cnt, rdata);

	focus_val = 610 - (10*focus_val);  //                                                                    
	data[0] = 0x01;	// absolut position
	data[1] = focus_val & 0xFF; // x lower
	data[2] = focus_val >> 0x08; // x upper
	mdelay(5);
	LDBGI("%s: CMD=0x%02X, 0x%02X, 0x%02X, 0x%02X\n", __func__, 0x33, data[0], data[1], data[2]);
	rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x33, data, 3);
	ce1702_check_af_status(TRUE);

	ce1702_manual_focus_val = focus_val;

	return rc;
}

/*                                                                               */
int get_pm8921_batt_temp(void)
{
	int rc = 0;
	struct pm8xxx_adc_chan_result result;

	rc = pm8xxx_adc_read(CHANNEL_BATT_THERM, &result);
	if (rc) {
		LDBGE("%s: Error reading battery temperature!\n", __func__);
		return rc;
	}
	rc = (int)((int)result.physical / 10);
	return rc;
}
/*                                                                             */

/*                                                                             */
int8_t ce1702_sensor_set_led_flash_mode(struct msm_sensor_ctrl_t *s_ctrl, int32_t led_mode)
{
	int8_t rc = 0;
	ce1702_led_flash_mode = led_mode;
	ce1702_sensor_set_led_flash_mode_snapshot(ce1702_led_flash_mode);

	return rc;
}

int8_t ce1702_sensor_set_led_flash_mode_for_AF(int32_t led_mode)
{
	int8_t rc = 0;
	unsigned char data[4];
	unsigned char led_power = 0x1A;
	int batt_temp = 0;

	LDBGI("%s: Enter [%d]\n", __func__, led_mode);

	batt_temp = get_pm8921_batt_temp();
	LDBGI("%s: led_mode = [ %d ] battery = [%d]\n", __func__, led_mode, batt_temp);
	if(batt_temp > -10) {
		led_power = 0x1A;	// 1237.5mA
	} else {
		led_power = 0x03;	// 225mA
	}

	switch(led_mode) {
		case FLASH_LED_OFF:
		case FLASH_LED_AUTO_NO_PRE_FLASH:
		case FLASH_LED_ON_NO_PRE_FLASH:
			data[0] = 0x01;
			data[1] = 0x00;
			rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0xB2, data, 2); // Set Strobe
			if (rc < 0)
				LDBGE("%s: ce1702_i2c_write failed(1) \n", __func__);
		break;

		case FLASH_LED_AUTO:	//auto for on AF
			data[0] = 0x01; 	//AF
			data[1] = 0x02; //auto
			rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0xB2, data, 2); // Set Strobe
			if (rc < 0)
				LDBGE("%s: ce1702_i2c_write failed(2) \n", __func__);
			data[0] = 0x01; 	//AF
			data[1] = 0x01;
			data[2] = led_power;
			data[3] = 0x00;
			rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0xB3, data, 4); // How Strong?
			if (rc < 0)
				LDBGE("%s: ce1702_i2c_write failed(3) \n", __func__);
			data[0] = 0x03;
			data[1] = 0x00;
			rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x07, data, 2);
			if (rc < 0)
				LDBGE("%s: ce1702_i2c_write failed(4) \n", __func__);
		break;

		case FLASH_LED_ON:	//on for on AF
			data[0] = 0x01; 	//AF
			data[1] = 0x01; //on
			rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0xB2, data, 2); // Set Strobe
			if (rc < 0)
				LDBGE("%s: ce1702_i2c_write failed(5) \n", __func__);
			data[0] = 0x01; 	//AF
			data[1] = 0x01;
			data[2] = led_power;
			data[3] = 0x00;
			rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0xB3, data, 4); // How Strong?
			if (rc < 0)
				LDBGE("%s: ce1702_i2c_write failed(6) \n", __func__);
			data[0] = 0x03;
			data[1] = 0x00;
			rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x07, data, 2);
			if (rc < 0)
				LDBGE("%s: ce1702_i2c_write failed(7) \n", __func__);
		break;

		case FLASH_LED_TORCH:	//torch
			data[0] = 0x00;
			data[1] = 0x00;
/*                                                                                   */
#ifdef CONFIG_MACH_APQ8064_GVDCM
			data[2] = 0x00;
#else
			data[2] = 0x03;	// 225mA
#endif
/*                                                                                 */

			data[3] = 0x00;
			rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0xB3, data, 4);
			if (rc < 0)
				LDBGE("%s: ce1702_i2c_write failed(8) \n", __func__);
			data[0] = 0x00;
			data[1] = 0x01; //ON
			rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x06, data, 2); // CMD:ONOFF!(Manually)
			if (rc < 0)
				LDBGE("%s: ce1702_i2c_write failed(9) \n", __func__);
		break;

		default:
			data[0] = 0x01;
			data[1] = 0x00;
			rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0xB2, data, 2); // Set Strobe
			if (rc < 0)
				LDBGE("%s: ce1702_i2c_write failed(10) \n", __func__);
			led_mode = 0;
		break;
	}

	return rc;
}
/*                                                                           */

/*                                                                              */
int8_t ce1702_sensor_set_led_flash_mode_snapshot(int32_t led_mode)
{
	int8_t rc = 0;
	unsigned char data[4];
	unsigned char led_power = 0x1A;
	int batt_temp = 0;

	batt_temp = get_pm8921_batt_temp();
	LDBGI("%s: led_mode = [ %d ] battery = [%d]\n", __func__, led_mode, batt_temp);
	if(batt_temp > -10) {
		led_power = 0x1A;	// 1237.5mA
	} else {
		led_power = 0x03;	// 225mA
	}

	LDBGI("%s: led_mode=%d\n", __func__, led_mode);

	switch(led_mode) {
		case FLASH_LED_OFF:
			data[0] = 0x01;
			data[1] = 0x00;
			rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0xB2, data, 2);
			if (rc < 0)
				LDBGE("%s: ce1702_i2c_write failed(1) \n", __func__);
			data[0] = 0x03;
			data[1] = 0x00;
			rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0xB2, data, 2);
			if (rc < 0)
				LDBGE("%s: ce1702_i2c_write failed(1) \n", __func__);
			data[0] = 0x00;
			data[1] = 0x00;
			rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x06, data, 2);
			if (rc < 0)
				LDBGE("%s: ce1702_i2c_write failed(9) \n", __func__);
		break;

		case FLASH_LED_AUTO:	//auto for snapshot
		case FLASH_LED_AUTO_NO_PRE_FLASH:
			data[0] = 0x03; 	//snapshot
			data[1] = 0x02;	//auto
			rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0xB2, data, 2);
			if (rc < 0)
				LDBGE("%s: ce1702_i2c_write failed(1) \n", __func__);
			data[0] = 0x03; 	//snapshot
			data[1] = 0x01;
			data[2] = led_power;
			data[3] = 0x00;
			rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0xB3, data, 4);
			if (rc < 0)
				LDBGE("%s: ce1702_i2c_write failed(2) \n", __func__);
			data[0] = 0x02;
			data[1] = 0x00;
			rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x07, data, 2);
			if (rc < 0)
				LDBGE("%s: ce1702_i2c_write failed(3) \n", __func__);
		break;

		case FLASH_LED_ON:	//on for snapshot
		case FLASH_LED_ON_NO_PRE_FLASH:
			data[0] = 0x03; 	//snapshot
			data[1] = 0x01;	//on
			rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0xB2, data, 2);
			if (rc < 0)
				LDBGE("%s: ce1702_i2c_write failed(4) \n", __func__);
			data[0] = 0x03; 	//snapshot
			data[1] = 0x01;
			data[2] = led_power;
			data[3] = 0x00;
			rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0xB3, data, 4);
			if (rc < 0)
				LDBGE("%s: ce1702_i2c_write failed(5) \n", __func__);
			data[0] = 0x02;
			data[1] = 0x00;
			rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x07, data, 2);
			if (rc < 0)
				LDBGE("%s: ce1702_i2c_write failed(6) \n", __func__);
		break;

		case FLASH_LED_TORCH:	// use for movie
			data[0] = 0x00;
			data[1] = 0x00;
/*                                                                                   */
#ifdef CONFIG_MACH_APQ8064_GVDCM
			data[2] = 0x00;
#else
			data[2] = 0x03;	// 225mA
#endif
/*                                                                                 */
			data[3] = 0x00;
			rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0xB3, data, 4);
			if (rc < 0)
				LDBGE("%s: ce1702_i2c_write failed(8) \n", __func__);
			data[0] = 0x00;
			data[1] = 0x01;
			rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x06, data, 2);
			if (rc < 0)
				LDBGE("%s: ce1702_i2c_write failed(9) \n", __func__);
		break;

		default:
			break;
	}

	return rc;
}
/*                                                                            */

//Set Antibanding for CE1702 by jungki.kim
int8_t ce1702_sensor_set_antibanding(struct msm_sensor_ctrl_t *s_ctrl, int32_t antibanding)
{
	int8_t rc = 0;
	unsigned char data = 0;

	if(ce1702_is_doing_touchaf == true) {
		LDBGI("%s: No need to set antibanding. ce1702_is_doing_touchaf = [%s]\n",__func__, (ce1702_is_doing_touchaf==true)?"true":"false");
		return rc;
	}

	LDBGI("%s: antibanding=%d\n", __func__, antibanding);

	switch(antibanding) {
		case CAMERA_ANTIBANDING_OFF:	//off
			data = 0x00;
			break;

		case CAMERA_ANTIBANDING_50HZ:	// 50hz
			data = 0x02;
			break;

		case CAMERA_ANTIBANDING_60HZ:	//60hz
			data = 0x03;
			break;

		case CAMERA_ANTIBANDING_AUTO:	// auto
			data = 0x01;
			break;
		default:
			data = 0x01;
			break;
	}

	rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x14, &data, 1);
	if (rc < 0)
		LDBGE("%s: ce1702_i2c_write failed(1) \n", __func__);

	return rc;
}

/*                                                                                             */
int32_t ce1702_object_tracking(struct msm_sensor_ctrl_t *s_ctrl, struct rec_t * rect_info)
{
	uint8_t wdata[10];
	uint16_t center_x, center_y;
	int32_t rc = 0;
	uint8_t res = 0;

	LDBGI("%s mode = %d\n", __func__,rect_info->mode);

	if(rect_info->mode) //on
	{
		center_x=(rect_info->x+(rect_info->dx/2));
		center_y=(rect_info->y+(rect_info->dy/2));

		LDBGI("%s x = %d, dx = %d, y = %d, dy = %d\n", __func__,rect_info->x,rect_info->dx,rect_info->y,rect_info->dy);
		LDBGI("%s center_x = %d, center_y = %d\n", __func__,center_x,center_y);

		rc = ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x43, NULL, 0, &res,  1);
		if(res == 0x04){
			int cnt= 0;
			LDBGI("%s object tracking is running = %d\n", __func__,res);
			wdata[0] = 0x00;
			rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x42, wdata,  1);	//Object tracking off
			do {
				rc = ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x43, NULL, 0, &res,  1);
				if(res == 0x00) break;	// check sync area
				mdelay(5); //yt.jeon 1115 optimize delay time
				cnt++;
			} while (cnt < 5);
		}

		wdata[0] = 0x04;
		wdata[1] = 0x00;
		wdata[2] = center_x&0xFF;  //lower
		wdata[3] = center_x>>0x8; //upper
		wdata[4] = center_y&0xFF; //lower
		wdata[5] = center_y>>0x8; //upper
		wdata[6] = 0x00;
		wdata[7] = 0x00;
		wdata[8] = 0x00;
		wdata[9] = 0x00;
		rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x41, wdata,  10);	//set Object tracking area
		if (rc < 0)
			LDBGE("%s: ce1702_i2c_write failed(1) \n", __func__);
		wdata[0] = 0x04;
		rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x42, wdata,  1);	//Object tracking on
		if (rc < 0)
			LDBGE("%s: ce1702_i2c_write failed(2) \n", __func__);
	}
	else //off
	{
		wdata[0] = 0x00;
		ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x42, wdata,  1);	//Object tracking off
		if (rc < 0)
			LDBGE("%s: ce1702_i2c_write failed(3) \n", __func__);
	}

	return CE1702_OK ;
}
/*                                                                                             */

/*                                                                              */
int8_t ce1702_set_aec_awb_lock(struct msm_sensor_ctrl_t *s_ctrl, int32_t islock)
{
	int8_t rc = 0;
	uint8_t data, res = 0;
       int8_t result =0;

	/* Is it previewing? */
	rc = ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x6C, NULL, 0, &res, 1);
	LDBGI("%s: %d: Preview state check=[0x%02x], ce1702_is_doing_touchaf=[%d]\n", __func__, __LINE__, res, ce1702_is_doing_touchaf);
	if( (res != 0x08) &&(res != 0x18)) {
		LDBGI("%s: Sensor is not previewing... value=[%x]\n", __func__, res);
		return rc;
	}

	if(ce1702_focus_mode==AF_MODE_CAF_VIDEO)
	{
		rc = ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x24, NULL, 0, &res, 1);	//check AF status
		LDBGE("%s: lens stop check [lens status = 0x%02x], [AF-C status = 0x%02x]  \n", __func__, res&0x01 , res>>6&0x3);
		result = res&0x01;
		if(result == 1) //running... status
		{
			int cnt= 0;
			uint8_t rdata;
			// stop lens
			LDBGI("%s: Stop Lens [AF]\n", __func__);
			ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x35, NULL, 0); // stop
			cnt = 0;
			do {
				rc = ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x24, NULL, 0, &rdata, 1);
				if ((rdata & 0x01) == 0)
					break;
				mdelay(10);
				cnt++;
			} while (cnt < 100);	//lens running
			LDBGE("%s: lens stop check [count %d]	= %x \n", __func__, cnt, rdata);
		}
	}

	data = ce1702_aec_awb_lock;
	switch (islock) {
		case 0 :
			data = data & 0xFE;
			break;
		case 1 :
			data = data | 0x01;
			break;
		case 2 :
			data = data & 0xEF;
			break;
		case 3 :
			data = data | 0x10;
			break;
	}
	ce1702_aec_awb_lock = data;

	if(!ce1702_is_doing_touchaf) {
		if( !((data == 0x02)||(data == 0x03)) ) {
			if(data == 0x11) data = 0x13;
			LDBGI("%s: data = [0x%02x]\n", __func__, data);
			rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x11, &data, 1);
		}
	}

	if(ce1702_focus_mode==AF_MODE_CAF_VIDEO && result == 1)
	{
		// 2. Start CAF(CMD:0x23)
		rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x23, NULL, 0);
	}
	return rc;
}
/*                                                                            */

/*                                                                          */
int8_t ce1702_get_cam_open_mode(struct msm_sensor_ctrl_t *s_ctrl, int32_t cam_op_mode)
{
	int8_t rc = 0;

	ce1702_cam_preview = cam_op_mode;

	LDBGI("%s: ### current opened preview for [%s]\n", __func__,
		(ce1702_cam_preview == PREVIEW_MODE_CAMERA)?"SNAPSHOT":"RECORDING");

	return rc;

}
/*                                                                        */

/*                                                                                          */
int32_t ce1702_dim_info(struct msm_sensor_ctrl_t *s_ctrl, struct dimen_t* dimension_info)
{
	size_info.preview_width = dimension_info->preview_width;
	size_info.preview_height = dimension_info->preview_height;
	size_info.picture_width = dimension_info->picture_width;
	size_info.picture_height = dimension_info->picture_height;
	size_info.video_width = dimension_info->video_width;
	size_info.video_height = dimension_info->video_height;
	size_info.thumbnail_width = dimension_info->thumbnail_width;
	size_info.thumbnail_heigh= dimension_info->thumbnail_heigh;

	LDBGI("%s: preview = %d x %d, picture = %d x %d, thumbnail = %d x %d\n", __func__,
			size_info.preview_width, size_info.preview_height,
			size_info.picture_width, size_info.picture_height,
			size_info.thumbnail_width, size_info.thumbnail_heigh);

	return CE1702_OK;
}
/*                                                                                          */
/*                                                                           */
int8_t ce1702_set_iso(struct msm_sensor_ctrl_t *s_ctrl, int32_t iso)
{
	uint8_t data[2];
	//uint8_t rdata[2];
	uint8_t res = 0;
	int retry_cnt = 0;
	int32_t rc = 0;
	LDBGI("%s\n", __func__);

	if(ce1702_cam_preview == PREVIEW_MODE_CAMCORDER)
	{
		iso = CAMERA_ISO_TYPE_DEFAULT;
		LDBGI("%s: set default camcorder mode EV preset [%d]\n", __func__, iso);
	}

	if(ce1702_iso_value == iso) {
		LDBGI("%s: just before value[%d] = requested value[%d] (SAME!)\n", __func__, ce1702_iso_value, iso);
		return rc;
	}

	data[0] = 0x01;

	switch (iso)
	{
	case CAMERA_ISO_TYPE_AUTO:
		data[1] = 0x00;
		break;

	case CAMERA_ISO_TYPE_100:
		data[1] = 0x02;
		break;

	case CAMERA_ISO_TYPE_200:
		data[1] = 0x03;
		break;

	case CAMERA_ISO_TYPE_400:
		data[1] =0x04;
		break;

	case CAMERA_ISO_TYPE_800:
		data[1] = 0x05;
		break;

	case CAMERA_ISO_TYPE_1600:
		data[1] = 0x06;
		break;

	case CAMERA_ISO_TYPE_DEFAULT:
		data[1] = 0x06;
		break;

	default:
		LDBGI("Not support iso setting \n");
		data[1] = 0x00;
		break;
	}

	LDBGI("ce1702_set_iso : %d	==> 0x04h value : %x, %x \n", iso, data[0], data[1]);

	rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x04, data, 2);
	if (rc < 0)
		LDBGE("%s: ce1702_i2c_write failed(1) \n", __func__);

	data[0]=0x00; //NULL
	rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x01, data, 0);
	if (rc < 0)
		LDBGE("%s: ce1702_i2c_write failed(2) \n", __func__);

	retry_cnt = 0;

	do{
		mdelay(5); //yt.jeon 1115 optimize delay time
		//rdata[0] = 0x00;
		rc = ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x02, NULL, 0, &res, 1);
		if (rc < 0)
			LDBGE("%s: ce1702_i2c_read failed \n", __func__);
		LDBGI(">>>> iso reflection check[%d] = %x \n", retry_cnt, res);
		retry_cnt++;
	} while((res != 0) && (retry_cnt < 500));

	if (retry_cnt >=500) rc = -EIO;

	ce1702_iso_value = iso;

	return rc;

}
/*                                                                         */

/*                                                                               */
int8_t ce1702_set_manual_scene_mode(struct msm_sensor_ctrl_t *s_ctrl, int32_t scene_mode)
{
	int8_t rc = CE1702_OK;
	unsigned char data[6];
	//uint8_t result =0;
	//int cnt;

	LDBGI("%s: scene_mode=%d, ce1702_scene_mode = %d\n", __func__, scene_mode, ce1702_scene_mode);

	data[0] = 0x02; // auto scene detection off
	data[2] = 0x00;
	data[3] = 0x00;
	data[4] = 0x00;
	data[5] = 0x00;

	if(ce1702_scene_mode != scene_mode) {
		switch(scene_mode) {
			case CAMERA_SCENE_OFF:
				data[0] = 0x00;
				data[1] = 0x00;
				LDBGI("%s: CAMERA_SCENE_OFF, CAMERA_SCENE_NIGHT=%d\n", __func__, scene_mode);
				break;
			case CAMERA_SCENE_LANDSCAPE :
				data[1] = 0x01;
				LDBGI("%s: CAMERA_SCENE_LANDSCAPE=%d\n", __func__, scene_mode);
				break;
			case CAMERA_SCENE_PORTRAIT :
				data[1] = 0x02;
				LDBGI("%s: CAMERA_SCENE_PORTRAIT=%d\n", __func__, scene_mode);
				break;
			case CAMERA_SCENE_NIGHT :
				data[1] = 0x03;
				LDBGI("%s: CAMERA_SCENE_NIGHT_PORTRAIT=%d\n", __func__, scene_mode);
				break;
			case CAMERA_SCENE_SPORTS:
				data[1] = 0x04;
				LDBGI("%s: CAMERA_SCENE_SPORTS=%d\n", __func__, scene_mode);
				break;
			case CAMERA_SCENE_SUNSET :
				data[1] = 0x05;
				LDBGI("%s: CAMERA_SCENE_SUNSET=%d\n", __func__, scene_mode);
				break;
			default :
				data[0] = 0x00;
				data[1] = 0x00;
				break;
		}

		if(ce1702_asd_onoff == 1){
			LDBGI("%s: ce1702_asd_onoff=%d  ce1702_scene_mode =%d\n", __func__, ce1702_asd_onoff, ce1702_scene_mode );
			ce1702_scene_mode = scene_mode;
			return rc;
		}

		rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x80, data, 6);
		if(rc < 0) {
			LDBGI("%s: ce1702_i2c_write failed(1). Fail to apply scene mode [%d]!!\n", __func__, scene_mode);
			return FALSE;
		}
		ce1702_scene_mode = scene_mode;
	}
	return rc;
}
/*                                                                             */

/*                                                                                       */
int8_t ce1702_set_model_name(void)
{
	int8_t rc = 0;
	char *maker_name = "LG Electronics";
	char *model_name;
	unsigned char data[MAX_NUMBER_CE1702+2];
	int i;
#ifdef CONFIG_MACH_APQ8064_GKKT
	model_name = "LG-F240K";
#endif
#ifdef CONFIG_MACH_APQ8064_GKSK
	model_name = "LG-F240S";
#endif
#if defined(CONFIG_MACH_APQ8064_GKU)
	model_name = "LG-F240L";
#endif
#if defined(CONFIG_MACH_APQ8064_OMEGAR)
	model_name = "LG-F310LR";
#endif
#if defined(CONFIG_MACH_APQ8064_OMEGA)
	model_name = "LG-F310L";
#endif
#ifdef CONFIG_MACH_APQ8064_GKATT
	model_name = "LG-E980";
#endif
#ifdef CONFIG_MACH_APQ8064_GKOPENHK
	model_name = "LG-E988"; //GK OPEN HK
#endif
#if defined(CONFIG_MACH_APQ8064_GKOPENTW) || defined(CONFIG_MACH_APQ8064_GKCHTTW)
	model_name = "LG-E988"; //GK OPEN TW
#endif
#if defined(CONFIG_MACH_APQ8064_GKSHBSG) || defined(CONFIG_MACH_APQ8064_GKOPENSG) || defined(CONFIG_MACH_APQ8064_GKMONSG) || defined(CONFIG_MACH_APQ8064_GKSTLSG)
	model_name = "LG-E988";
#endif
#ifdef CONFIG_MACH_APQ8064_GKOPENEU
	model_name = "LG-E986";  //GK OPEN EU
#endif
#ifdef CONFIG_MACH_APQ8064_GKOPENCIS
	model_name = "LG-E988";  //GK OPEN CIS
#endif
#ifdef CONFIG_MACH_APQ8064_GKTCLMX
	model_name = "LG-E980h";  //GK TCL MX
#endif
#ifdef CONFIG_MACH_APQ8064_GKOPENBR
	model_name = "LG-E989";  //GK OPEN BR
#endif
#ifdef CONFIG_MACH_APQ8064_GKVIVBR
	model_name = "LG-E989";  //GK VIV BR
#endif
#if defined(CONFIG_MACH_APQ8064_GKOPENESA) || defined(CONFIG_MACH_APQ8064_GKOPENAME) || defined(CONFIG_MACH_APQ8064_GKOPENMY) || defined(CONFIG_MACH_APQ8064_GKOPENZA) || defined(CONFIG_MACH_APQ8064_GKOPENID) || defined(CONFIG_MACH_APQ8064_GKORIIL)  || defined(CONFIG_MACH_APQ8064_GKCLCZA)
	model_name = "LG-E988";
#endif
#if defined(CONFIG_MACH_APQ8064_GKOPENIL) || defined(CONFIG_MACH_APQ8064_GKCCMIL) || defined(CONFIG_MACH_APQ8064_GKMIRIL) || defined(CONFIG_MACH_APQ8064_GKPCLIL) || defined(CONFIG_MACH_APQ8064_GKOPENTH)
	model_name = "LG-E989";
#endif
#ifdef CONFIG_MACH_APQ8064_GVDCM
	model_name = "L-04E";
#endif
#ifdef CONFIG_MACH_APQ8064_GVKT
	model_name = "LG-F220K";
#endif
#ifdef CONFIG_MACH_APQ8064_GVAR_CMCC
	model_name = "LG-E985T";
#endif

	memset(data, 0, sizeof(data));
	data[0] = 0x06;	// Model
	data[1] = strlen(model_name);
	for(i=0;i<strlen(model_name);i++)
		data[2+i] = model_name[i];
	data[2+i+1] ='\0';

	rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0xA1, data, MAX_NUMBER_CE1702+2);
	if (rc < 0)
		LDBGE("%s: ce1702_i2c_write failed(1) \n", __func__);
	LDBGI("%s: model = [%s] rc=[%d]\n", __func__, model_name, rc);

	memset(data, 0, sizeof(data));
	data[0] = 0x05;	// Maker
	data[1] = strlen(maker_name);
	for(i=0;i<strlen(maker_name);i++)
		data[2+i] = maker_name[i];
	data[2+i+1] ='\0';

	rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0xA1, data, MAX_NUMBER_CE1702+2);
	if (rc < 0)
		LDBGE("%s: ce1702_i2c_write failed(2) \n", __func__);
	LDBGI("%s: maker = [%s] rc=[%d]\n", __func__, maker_name, rc);

	ce1702_set_time();

	return rc;
}
/*                                                                                     */

int8_t ce1702_set_ISP_mode(void)
{
	int32_t rc = 0;
	uint8_t pdata[2];

	LDBGI("%s: Setting\n", __func__);
	//1. set ISP Mode
	if(ce1702_cam_preview == PREVIEW_MODE_CAMCORDER)
		pdata[0] = 0x0E; //660Mbps, 2lane
	else
		pdata[0] = 0x10; //768Mbps, 2lane
	rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x03, pdata, 1);
	if (rc < 0)
		LDBGE("%s: ce1702_i2c_write failed(1) \n", __func__);
	mdelay(10);

	//2. set System Preset
	pdata[0] = 0x00;
	pdata[1] = 0x04;
	rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x05, pdata, 2);
	if (rc < 0)
		LDBGE("%s: ce1702_i2c_write failed(2) \n", __func__);
	return rc;
}

int8_t ce1702_set_time(void)
{
	int8_t rc = 0;
	unsigned char cmd;
	unsigned char data[10];
	struct timespec time;
	struct tm tmresult;

	time = __current_kernel_time();
	time_to_tm(time.tv_sec,sys_tz.tz_minuteswest * 60* (-1),&tmresult);

	cmd = 0x0E;

	data[0] = (uint8_t)((tmresult.tm_year + 1900) & 0xFF);
	data[1] = (uint8_t)((tmresult.tm_year + 1900) >> 0x08);
	data[2] = tmresult.tm_mon + 1;
	data[3] = tmresult.tm_mday;
	data[4] = tmresult.tm_hour;
	data[5] = tmresult.tm_min;
	data[6] = tmresult.tm_sec+1;
	rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, cmd, data, 7);
	if (rc < 0)
		LDBGE("%s: ce1702_i2c_write failed(1) \n", __func__);

	//LDBGI("%s: %02x %02x-%02d-%02d %02d:%02d:%02d\n", __func__,
	//	data[0], data[1], data[2], data[3], data[4], data[5], data[6]);

	return rc;
}

/*                                                            */
int32_t ce1702_set_gyro_data(struct msm_sensor_ctrl_t *s_ctrl, uint8_t *data)
{
	uint8_t data2[32];
	int32_t rc = 0;

	//LDBGI("%s\n", __func__);

	if (copy_from_user(&data2, (void *)data, 32))
		return -EFAULT;

	if(data[0]){// gyro
		rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0xB1, &data[1], 11);
		if (rc < 0)
			LDBGE("%s: ce1702_i2c_write failed(1) \n", __func__);
	}

	if(data[12]){ // gravity
		rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0xB1, &data[13], 11);
		if (rc < 0)
			LDBGE("%s: ce1702_i2c_write failed(2) \n", __func__);
	}

	return CE1702_OK ;
}
/*                                                          */

/*                                                                             */
int8_t ce1702_set_WDR(struct msm_sensor_ctrl_t *s_ctrl, int32_t wdr_mode)
{
	int8_t rc = 0;
	uint8_t pdata[2];
	uint8_t res = 0;
//	uint8_t wdrdata[3];
	uint8_t uvdata[2];
	int cnt = 0;

	LDBGI("[G-young] %s: WDR = %d\n", __func__, wdr_mode);

	pdata[0] = 0x20;
	pdata[1] = 0x01;
	rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x04, pdata, 2);
	if (rc < 0)
		LDBGE("%s: ce1702_i2c_write failed(1) \n", __func__);

	//pdata[0] =0x00;
	rc =  ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x01, NULL, 0);
	if (rc < 0)
		LDBGE("%s: ce1702_i2c_write failed2() \n", __func__);

	do {
		mdelay(5); //yt.jeon 1115 optimize delay time
		//rdata[0] = 0x00;
		rc = ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x02, NULL, 0, &res, 1);
		if (rc < 0)
			LDBGE("%s: ce1702_i2c_read failed \n", __func__);
		LDBGI("%s: verifying... cnt=%d rc=%d\n", __func__, cnt, rc);
		cnt++;
	} while ( (res != 0) && (cnt < 100) );

	if(cnt >= 100) {
		LDBGI("%s: Fail to read isp status \n", __func__);
		rc = - EIO;
	}

	switch(wdr_mode) {
		case CE1702_STATUS_OFF:
			pdata[0] = 0x00;
			uvdata[0] = 0x03;
			uvdata[1] = 0x00;
			break;

		case CE1702_STATUS_ON:
			pdata[0] = 0x01;
			uvdata[0] = 0x03;
			uvdata[1] = 0x05;
			break;
		}

	// 0 : off, 1: on
	LDBGI("[G-young] %s : wdr = %d, uvdata = %d\n", __func__, wdr_mode, uvdata[1]);

	rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x88, pdata, 1);
	if(rc<0) {
		LDBGE("%s: ce1702_i2c_write failed(1).  Fail to apply WDR[%d]\n", __func__, wdr_mode);
		return FALSE;
	}

	rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x3D, uvdata, 2);
	if(rc<0) {
		LDBGE("%s: ce1702_i2c_write failed(1).  Fail to apply UV-MAP[%d]\n", __func__, uvdata[1]);
		return FALSE;
	}

	//pdata[0] =0x00;
	rc =  ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x01, NULL, 0);
	if (rc < 0)
		LDBGE("%s: ce1702_i2c_write failed2() \n", __func__);

	cnt = 0;
	do {
		mdelay(5); //yt.jeon 1115 optimize delay time
		//rdata[0] = 0x00;
		rc = ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x02, NULL, 0, &res, 1);
		if (rc < 0)
			LDBGE("%s: ce1702_i2c_read failed \n", __func__);
		LDBGI("%s: verifying... cnt=%d rc=%d\n", __func__, cnt, rc);
		cnt++;
	} while ( (res != 0) && (cnt < 100) );

#if 0
	//rdata[0] = 0x00;
	rc = ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x87, NULL, 0, wdrdata, 3);
	if (rc < 0)
		LDBGE("%s: ce1702_i2c_read failed(1) \n", __func__);
	LDBGI("%s : status = %d, hilight = %d, shadow = %d\n",__func__, wdrdata[0], wdrdata[1],wdrdata[2]);
#endif

	return rc;
}
/*                                                                           */


/*                                                                                  */
int8_t ce1702_set_exif_rotation(struct msm_sensor_ctrl_t *s_ctrl, int rotation)
{
	int8_t rc = 0;

	ce1702_rotation = rotation;
	LDBGI("%s: rotation=[%d]\n", __func__, ce1702_rotation);

	return rc;
}

int8_t ce1702_set_exif_rotation_to_isp(void)
{
	int8_t rc = 0;
	unsigned char cmd;
	unsigned char data[5];

	memset(data, 0, sizeof(data));

	if(ce1702_rotation != -1) {
		cmd = 0xA2;
		data[0] = 0x00;
		switch(ce1702_rotation) {
			case 0:
				data[1] = 0x01;
				break;
			case 90:
				data[1] = 0x06;
				break;
			case 180:
				data[1] = 0x03;
				break;
			case 270:
				data[1] = 0x08;
				break;
			default:
				data[1] = 0x01;
				LDBGE("%s: Bug!!\n",__func__);
				break;
		}
		rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, cmd, data, 5);
		if(rc != CE1702_OK)
			LDBGE("%s: ce1702_i2c_write failed(1) Send Command Error!\n", __func__);
	}

	return rc;
}
/*                                                                                */

/*                                                                  */
int8_t ce1702_switching_exif_gps(bool onoff)
{
	int8_t rc = 0;
	unsigned char data[2];

	data[0] = 0x11;

	if (ce1702_need_thumbnail) {
		data[1] = onoff ? 0x00 : 0x04;
	} else {
		data[1] = onoff ? 0x02 : 0x05;
	}
	LDBGE("%s: data[1]=%d ! \n", __func__, data[1]);

	rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x07, data, 2);

	return rc;
}
/*                                                                */

/*                                                                         */
int8_t ce1702_set_exif_gps(struct msm_sensor_ctrl_t *s_ctrl, struct k_exif_gps_t *gps_info)
{
	int8_t rc = 0;
	unsigned char cmd;
	unsigned char data[9];
	uint32_t flt, len;
	//int i;

	if (!gps_info->altitude && !gps_info->latitude[0] && !gps_info->latitude[1] && !gps_info->latitude[2] && !gps_info->longitude[0] && !gps_info->longitude[1] && !gps_info->longitude[2]) {
		LDBGE("%s: set remove gps data !!\n", __func__);
		ce1702_switching_exif_gps(false);
		return rc;
	}

	ce1702_switching_exif_gps(true);

	cmd = 0xA3;

	// Latitude
	data[0] = 0x80;
	if(gps_info->latRef == 'N')
					data[1] = 0x00;
	else
					data[1] = 0x01;

	data[2] = 0x02;
	data[3] = gps_info->latitude[0];
	data[4] = gps_info->latitude[1];

	data[5] = (uint32_t)(gps_info ->latitude[2] / 10000);
	flt = (gps_info->latitude[2]) % 10000;
	flt = flt * 10;

	data[6] = flt & 0xFF;
	flt = (uint32_t)(flt >> 0x08);
	data[7] = flt & 0xFF;
	flt = (uint32_t)(flt >> 0x08);
	data[8] = flt & 0xFF;

	//for(i=0;i<9;i++)
	//	LDBGI("%s: data[%d]=0x%02x\n", __func__, i, data[i]);
	rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, cmd, data, 9);
	if(rc != CE1702_OK)
		LDBGE("%s: ce1702_i2c_write failed(1) Error to insert GPS(latitude)!!\n", __func__);

	// Longitude
	data[0] = 0x81;
	if(gps_info->lonRef == 'E')
					data[1] = 0x00;
	else
					data[1] = 0x01;

	data[2] = 0x02;
	data[3] = gps_info->longitude[0];
	data[4] = gps_info->longitude[1];

	data[5] = (uint32_t)(gps_info ->longitude[2] / 10000);
	flt = (gps_info->longitude[2]) % 10000;
	flt = flt*10;

	data[6] = flt & 0xFF;
	flt = (uint32_t)(flt >> 0x08);
	data[7] = flt & 0xFF;
	flt = (uint32_t)(flt >> 0x08);
	data[8] = flt & 0xFF;

	//for(i=0;i<9;i++)
	//	LDBGI("%s: data[%d]=0x%02x\n", __func__, i, data[i]);
	rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, cmd, data, 9);
	if(rc != CE1702_OK)
		LDBGE("%s:ce1702_i2c_write failed(2) Error to insert GPS(longitude)!!\n", __func__);

	// Altitude
	data[0] = 0x02;
	if(gps_info->altiRef == 0)
					data[1] = 0x00;
	else
					data[1] = 0x01;

	flt = (gps_info->altitude) % 1000;
	gps_info->altitude = (uint32_t)(gps_info->altitude / 1000);

	data[2] = (gps_info->altitude) & 0xFF;
	gps_info->altitude = (uint32_t)((gps_info->altitude) >> 0x08);
	data[3] = (gps_info->altitude) & 0xFF;
	gps_info->altitude = (uint32_t)((gps_info->altitude) >> 0x08);
	data[4] = (gps_info->altitude) & 0xFF;
	gps_info->altitude = (uint32_t)((gps_info->altitude) >> 0x08);
	data[5] = (gps_info->altitude) & 0xFF;

	data[6] = (uint16_t)(flt / 10);

	data[7] = 0x00;
	data[8] = 0x00;

	//for(i=0;i<9;i++)
	//	LDBGI("%s: data[%d]=0x%02x\n", __func__, i, data[i]);
	rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, cmd, data, 9);
	if(rc != CE1702_OK)
		LDBGE("%s:ce1702_i2c_write failed(3) Error to insert GPS(altitude)!!\n", __func__);

	// Date And Time
	data[0] = 0x20;
	data[1] = (gps_info->gpsDateStamp[0]) & 0xFF; // x lower
	data[2] = (gps_info->gpsDateStamp[0]) >>0x08; // x upper
	data[3] = gps_info->gpsDateStamp[1];
	data[4] = gps_info->gpsDateStamp[2];
	data[5] = gps_info->gpsTimeStamp[0];
	data[6] = gps_info->gpsTimeStamp[1];
	data[7] = gps_info->gpsTimeStamp[2];
	data[8] = 0x00;

	//for(i=0;i<9;i++)
	//	LDBGI("%s: data[%d]=0x%02x\n", __func__, i, data[i]);
	rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, cmd, data, 9);
	if(rc != CE1702_OK)
		LDBGE("%s: ce1702_i2c_write failed(4) Error to insert GPS(date and time)!!\n", __func__);

	len = strlen(gps_info->gpsProcessingMethod);
	if (len) {
		uint8_t method[130] = {0, };

		method[0] = 0x0A;
		method[1] = (uint8_t)len + 1;
		strcpy(&method[2], gps_info->gpsProcessingMethod);

		rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0xA1, method, 130);
		if(rc != CE1702_OK)
			LDBGE("%s:ce1702_i2c_write failed(3) Error to insert GPS(ProcessingMethod)!!\n", __func__);
	}

	return rc;
}
/*                                                                       */

/*                                                        */
int8_t ce1702_asd_enable(struct msm_sensor_ctrl_t *s_ctrl, int32_t asd_onoff)
{
	unsigned char wdata[6];
	uint8_t data;
	int rc = CE1702_OK;

	LDBGI("[G-young] %s : value = %d [start]\n", __func__, asd_onoff);
	wdata[0] = asd_onoff;
	wdata[1] = 0x00;
	wdata[2] = asd_onoff;
	wdata[3] = 0x00;
	wdata[4] = 0x00;
	wdata[5] = 0x00;

	rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x80, wdata, 6 );
	if (rc < 0)
		LDBGE("%s: ce1702_i2c_write failed(5) \n", __func__);

	ce1702_asd_onoff = asd_onoff;

	data = 0x01;
	rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x14, &data, 1);
	if (rc < 0)
		LDBGE("%s: ce1702_i2c_write failed(4) \n", __func__);

	LDBGI("[G-young] %s : setting face detecting , value = %d \n",__func__, asd_onoff);
	ce1702_set_face_detect(asd_onoff);

	LDBGI("[G-young] %s : asd_onoff = %d [end]\n", __func__, asd_onoff);

	return CE1702_OK;

}
/*                                                      */

/*                                                                               */
int8_t ce1702_set_face_detect(bool asd_onoff)
{
	int cnt,  rc ;
	uint8_t res, data;
	unsigned char wdata[6];

	LDBGI("[G-young] %s : asd_onoff = %d [start]\n", __func__, asd_onoff);
	rc = 0;
	data = asd_onoff; // Face detect ON!

	if(asd_onoff){
		wdata[0] = 0x00;
		wdata[1] = 0x01;
		wdata[2] = 0x03;
		wdata[3] = 0x00;
		wdata[4] = 0x00;
		wdata[5] = 0x00;

		rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x41, wdata, 6);
		LDBGI("[G-young] %s : asd_onoff = %d  0x41 write\n", __func__, asd_onoff);
		if (rc < 0)
			LDBGE("%s: ce1702_i2c_write failed(1) \n", __func__);
	}

	res = ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x43, NULL, 0, &res,  1);
	LDBGI("[G-young] %s : preview assist mode status = %d \n", __func__, res);

	if((asd_onoff == 1 && res == 0x01) || (asd_onoff ==0 && res == 0x00)){
		LDBGI("[G-young] %s : asd_onoff = %d, preview assist mode = %d  / already setting! \n", __func__, asd_onoff, res);
		return 0;
	}

	LDBGI("[G-young] %s : 0x42 write [start]\n", __func__);

	rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x42, &data, 1); // control preview assist
	if (rc < 0)
		LDBGE("%s: ce1702_i2c_write failed(2) \n", __func__);

	cnt = 0;
		do {
			rc = ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x43, NULL, 0, &res,  1);
//			LDBGE("[G-young] %s : preview assist mode status = %d\n",__func__, res);
			if(res == data) break;	// check sync area
			mdelay(5); //yt.jeon 1115 optimize delay time
			cnt++;
		} while (cnt < 100);

	LDBGI("[G-young] %s : preview assist mode status = %d [end]\n", __func__, res);

	return 0;

}
/*                                                                             */

//                                                   
int8_t ce1702_set_manual_VCM_position(struct msm_sensor_ctrl_t *s_ctrl, int32_t focus_val)
{
	int8_t rc = 0;
	unsigned char data[3];

	LDBGI("%s: manual focus value=%d\n", __func__, focus_val);

		data[0] = 0x01;	// absolute position
		data[1] = (unsigned char)(focus_val & 0xFF); // x lower
		data[2] = (unsigned char)(focus_val >>0x08); // x upper
		LDBGI("%s: CMD=0x%02X, 0x%02X, 0x%02X, 0x%02X\n", __func__, 0x33, data[0], data[1], data[2]);
		rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x33, data, 3);
		if (rc < 0)
			LDBGE("%s: ce1702_i2c_write failed(1) \n", __func__);
		rc = ce1702_check_af_status(TRUE);

	return rc;
}

int8_t ce1702_set_VCM_default_position(struct msm_sensor_ctrl_t *s_ctrl)
{
	int8_t rc = 0, count = 0;
	//unsigned char data[3] = {0, };
	uint8_t rdata[2] = {0, };
	int32_t focus_val = 0;

	//data[0] = 0x00;
	rc = ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x34, NULL, 0, rdata, 2);
	if (rc < 0)
		LDBGE("%s: ce1702_i2c_read failed \n", __func__);

	focus_val = (int32_t)((0xFF00 &(rdata[1] <<0x08)) | rdata[0]);
	LDBGI("%s: focus value=%d\n", __func__, focus_val);

	//                                                                                         

	if (focus_val > 200)
	{
		rc = ce1702_set_manual_VCM_position(s_ctrl, 180);
		focus_val = 180;
	}

	if (focus_val < 100)
		rc = ce1702_set_manual_VCM_position(s_ctrl, 0);
	else
	{
		do{
			count++;
			focus_val = focus_val -70;
			rc = ce1702_set_manual_VCM_position(s_ctrl, focus_val);

		}while (focus_val > 80 && count <5);

		rc = ce1702_set_manual_VCM_position(s_ctrl, 0);
	}

	return rc;
}
//                                                 

/*                                                                  */
int32_t ce1702_set_exif_thumbnail_size(struct msm_sensor_ctrl_t *s_ctrl, struct dimen_t* dimension_info)
{
	int rc = 0;

	if (!dimension_info->thumbnail_width && !dimension_info->thumbnail_heigh) {
#if 0
		data[0] = 0x11;
		data[1] = 0x02;
		rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x07, data, 2);
#endif
		ce1702_need_thumbnail = false;
		LDBGE("%s: remove thumbnail !! \n", __func__);

	}
	else {
		ce1702_need_thumbnail = true;
	}

	LDBGI("[%s] thumbnail = %d x %d\n", __func__, dimension_info->thumbnail_width, dimension_info->thumbnail_heigh);
	return rc;
}
/*                                                                  */

static struct msm_sensor_fn_t ce1702_func_tbl = {
	.sensor_start_stream = ce1702_sensor_start_stream, //msm_sensor_start_stream,
	.sensor_stop_stream = ce1702_sensor_stop_stream,//msm_sensor_stop_stream,
	.sensor_group_hold_on = msm_sensor_group_hold_on,
	.sensor_group_hold_off = msm_sensor_group_hold_off,
	.sensor_set_fps = msm_sensor_set_fps,
	.sensor_write_exp_gain = msm_sensor_write_exp_gain1,
	.sensor_write_snapshot_exp_gain = msm_sensor_write_exp_gain1,
	.sensor_setting = ce1702_sensor_setting,
	.sensor_csi_setting = msm_sensor_setting1,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = msm_sensor_config,
	.sensor_power_up = ce1702_power_up,
	.sensor_power_down = ce1702_power_down,
	.sensor_adjust_frame_lines = msm_sensor_adjust_frame_lines1,
	.sensor_get_csi_params = msm_sensor_get_csi_params,
	.sensor_match_id = ce1702_match_id,
	.sensor_special_effect = ce1702_set_special_effect,						//                                   
	.sensor_exposure_compensation = ce1702_set_exposure_compensation,	//                                      
	.sensor_set_focus_mode_setting = ce1702_set_focus_mode_setting,		//                                                 
	.sensor_start_af = ce1702_start_af,									// Start AF for CE1702 by jungki.kim
	.sensor_stop_af = ce1702_stop_af,									// Stop AF for CE1702 by jungki.kim
	.sensor_set_af_window = ce1702_set_af_window,						//Set AF Window for CE1702 by jungki.kim
	.sensor_whitebalance_setting = ce1702_set_wb_setting,					//                                                       
	.sensor_set_zoom_ratio = ce1702_set_zoom_ratio,						//                                                    
	.sensor_set_manual_focus_length = ce1702_set_manual_focus_length,		//                                          
	.sensor_set_led_flash_mode = ce1702_sensor_set_led_flash_mode,		//Support LED Flash only for CE1702 by jungki.kim
	.sensor_set_antibanding_ce1702 = ce1702_sensor_set_antibanding,		//Set Antibanding for CE1702 by jungki.kim
	.sensor_set_ae_window = ce1702_set_ae_window,						//Set AE Window for CE1702 by jungki.kim
	.sensor_object_tracking = ce1702_object_tracking,						//                                                                             
	.sensor_set_aec_awb_lock = ce1702_set_aec_awb_lock,					//Support AEC/AWB Lock for CE1702 by jungki.kim
	.sensor_dim_info = ce1702_dim_info,									//                                                                              
	.sensor_get_cam_open_mode = ce1702_get_cam_open_mode,			//                                                 
	.sensor_set_iso = ce1702_set_iso,									//Support ISO setting for CE1702 by gayoung85.lee
	.sensor_set_manual_scene_mode = ce1702_set_manual_scene_mode,		//Support ManualSceneMode for CE1702 by gayoung85.lee
	.sensor_set_gyro_data = ce1702_set_gyro_data,						//                                    
	.sensor_set_wdr = ce1702_set_WDR,									//Set WDR mode for CE1702 by gayoung85.lee
	.sensor_set_exif_rotation = ce1702_set_exif_rotation,					//                                                         
	.sensor_set_exif_gps = ce1702_set_exif_gps,							//                                                 
	.sensor_set_asd_enable = ce1702_asd_enable,							//Support ASD for CE1702 by gayoung85.lee
	.sensor_set_exif_thumbnail_size = ce1702_set_exif_thumbnail_size,
};

static struct msm_sensor_reg_t ce1702_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf = ce1702_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(ce1702_start_settings),
	.stop_stream_conf = ce1702_stop_settings,
	.stop_stream_conf_size = ARRAY_SIZE(ce1702_stop_settings),
	.init_settings = &ce1702_init_conf[0],
	.init_size = ARRAY_SIZE(ce1702_init_conf),
	.mode_settings = &ce1702_confs[0],
	.output_settings = &ce1702_dimensions[0],
	.num_conf = ARRAY_SIZE(ce1702_confs),
};

static struct msm_sensor_ctrl_t ce1702_s_ctrl = {
	.msm_sensor_reg = &ce1702_regs,
	.sensor_i2c_client = &ce1702_sensor_i2c_client,
	.sensor_i2c_addr = 0x78,	//0x3c,
	.vreg_seq = ce1702_veg_seq,
	.num_vreg_seq = ARRAY_SIZE(ce1702_veg_seq),
	.sensor_output_reg_addr = &ce1702_reg_addr,
	.sensor_id_info = &ce1702_id_info,
	.sensor_exp_gain_info = &ce1702_exp_gain_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.msm_sensor_mutex = &ce1702_mut,
	.sensor_i2c_driver = &ce1702_i2c_driver,
	.sensor_v4l2_subdev_info = ce1702_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(ce1702_subdev_info),
	.sensor_v4l2_subdev_ops = &ce1702_subdev_ops,
	.func_tbl = &ce1702_func_tbl,
	.clk_rate = MSM_SENSOR_MCLK_24HZ,
};

module_init(ce1702_sensor_init_module);

MODULE_AUTHOR("LGE.com ");
MODULE_DESCRIPTION("NEC CE1702 Driver");
MODULE_LICENSE("GPL v2");
