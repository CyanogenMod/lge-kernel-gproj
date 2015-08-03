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

#include <linux/module.h>
#include "msm_camera_eeprom.h"
#include "msm_camera_i2c.h"

DEFINE_MUTEX(s5k4e5ya_eeprom_mutex);
static struct msm_eeprom_ctrl_t s5k4e5ya_eeprom_t;

static const struct i2c_device_id s5k4e5ya_eeprom_i2c_id[] = {
	{"s5k4e5ya_eeprom", (kernel_ulong_t)&s5k4e5ya_eeprom_t},
	{ }
};

static struct i2c_driver s5k4e5ya_eeprom_i2c_driver = {
	.id_table = s5k4e5ya_eeprom_i2c_id,
	.probe  = msm_eeprom_i2c_probe,
	.remove = __exit_p(s5k4e5ya_eeprom_i2c_remove),
	.driver = {
		.name = "s5k4e5ya_eeprom",
	},
};

static int __init s5k4e5ya_eeprom_i2c_add_driver(void)
{
	int rc = 0;
	rc = i2c_add_driver(s5k4e5ya_eeprom_t.i2c_driver);
	return rc;
}

static struct v4l2_subdev_core_ops s5k4e5ya_eeprom_subdev_core_ops = {
	.ioctl = msm_eeprom_subdev_ioctl,
};

static struct v4l2_subdev_ops s5k4e5ya_eeprom_subdev_ops = {
	.core = &s5k4e5ya_eeprom_subdev_core_ops,
};

uint8_t s5k4e5ya_wbcalib_data[6];
struct msm_calib_wb s5k4e5ya_wb_data;
uint8_t s5k4e5ya_lsccalib_data[896]; //                                                                    
struct msm_calib_lsc s5k4e5ya_lsc_data;
uint8_t s5k4e5ya_afcalib_data[4];
struct msm_calib_af s5k4e5ya_af_data;

static struct msm_camera_eeprom_info_t s5k4e5ya_calib_supp_info = {
	{ TRUE, 4, 2, 1}, // af
	{ TRUE, 6, 0, 1024},  //                                                                          
	{ TRUE, 896, 1, 255}, //                                                                            
	{FALSE, 0, 0, 1},
};

static struct msm_camera_eeprom_read_t s5k4e5ya_eeprom_read_tbl[] = {
	{0x60, &s5k4e5ya_wbcalib_data[0], 6, 1},
	{0x70, &s5k4e5ya_lsccalib_data[0], /* 884 */896, 0}, //                                                                          
	{0x66, &s5k4e5ya_afcalib_data[0], 4, 1},
};


static struct msm_camera_eeprom_data_t s5k4e5ya_eeprom_data_tbl[] = {
	{&s5k4e5ya_wb_data, sizeof(struct msm_calib_wb)},
	{&s5k4e5ya_lsc_data, sizeof(struct msm_calib_lsc)},
	{&s5k4e5ya_af_data, sizeof(struct msm_calib_af)},
};

//                                                                      
#define R_OVER_G_TYP 692
#define B_OVER_G_TYP 671
#define GR_OVER_GB_TYP 1024
//                                                                      
static void s5k4e5ya_format_wbdata(void)
{

#if 1
	/*                                                                                      */
	s5k4e5ya_wb_data.r_over_g = (uint16_t)(s5k4e5ya_wbcalib_data[0] << 8) |
		s5k4e5ya_wbcalib_data[1];
	s5k4e5ya_wb_data.b_over_g = (uint16_t)(s5k4e5ya_wbcalib_data[2] << 8) |
		s5k4e5ya_wbcalib_data[3];
	s5k4e5ya_wb_data.gr_over_gb = (uint16_t)(s5k4e5ya_wbcalib_data[4] << 8) |
		s5k4e5ya_wbcalib_data[5];
//                                                                      
    if (s5k4e5ya_wb_data.r_over_g > R_OVER_G_TYP + (R_OVER_G_TYP >> 2) || /* +- 25% */
		s5k4e5ya_wb_data.r_over_g < R_OVER_G_TYP - (R_OVER_G_TYP >> 2) ||
		s5k4e5ya_wb_data.b_over_g > B_OVER_G_TYP + (R_OVER_G_TYP >> 2) ||
		s5k4e5ya_wb_data.b_over_g < B_OVER_G_TYP - (R_OVER_G_TYP >> 2) ||
		s5k4e5ya_wb_data.gr_over_gb > GR_OVER_GB_TYP + (R_OVER_G_TYP >> 3) || /* +- 12.5% */
		s5k4e5ya_wb_data.gr_over_gb < GR_OVER_GB_TYP - (R_OVER_G_TYP >> 3)) {
	  pr_err("Cal data range over!! r_over_g=0x%x, b_over_g=0x%x, gr_over_gb=0x%x\n", 
			s5k4e5ya_wb_data.r_over_g, 
			s5k4e5ya_wb_data.b_over_g, 
			s5k4e5ya_wb_data.gr_over_gb);
	  s5k4e5ya_calib_supp_info.wb.is_supported = FALSE;
    }
//                                                                      
	/*                                                                                     */
#else
	s5k4e5ya_wb_data.r_over_g = (uint16_t)(s5k4e5ya_wbcalib_data[1] << 8) |
		s5k4e5ya_wbcalib_data[0];
	s5k4e5ya_wb_data.b_over_g = (uint16_t)(s5k4e5ya_wbcalib_data[3] << 8) |
		s5k4e5ya_wbcalib_data[2];
	s5k4e5ya_wb_data.gr_over_gb = (uint16_t)(s5k4e5ya_wbcalib_data[5] << 8) |
		s5k4e5ya_wbcalib_data[4];
#endif
        //pr_err(" r_over_g= 0x%x , b_over_g= 0x%x , gr_over_gb= 0x%x \n", s5k4e5ya_wb_data.r_over_g, s5k4e5ya_wb_data.b_over_g, s5k4e5ya_wb_data.gr_over_gb);

}

static void s5k4e5ya_format_lscdata(void)
{
  int i;
//                                                                      
  uint16_t r_sum = 0, gr_sum = 0, gb_sum = 0, b_sum = 0;
  
  for (i = 0; i < 221; i++) {
	s5k4e5ya_lsc_data.r_gain[i]= s5k4e5ya_lsccalib_data[i];
	s5k4e5ya_lsc_data.gr_gain[i]= s5k4e5ya_lsccalib_data[i+224];
	s5k4e5ya_lsc_data.gb_gain[i]= s5k4e5ya_lsccalib_data[i+448];
	s5k4e5ya_lsc_data.b_gain[i]= s5k4e5ya_lsccalib_data[i+672];

    r_sum += s5k4e5ya_lsc_data.r_gain[i];
	gr_sum += s5k4e5ya_lsc_data.gr_gain[i];
	gb_sum += s5k4e5ya_lsc_data.gb_gain[i];
	b_sum += s5k4e5ya_lsc_data.b_gain[i];

  }

  if (r_sum != (s5k4e5ya_lsccalib_data[i] | s5k4e5ya_lsccalib_data[i+1] << 8) ||
	  gr_sum != (s5k4e5ya_lsccalib_data[i+224] | s5k4e5ya_lsccalib_data[i+225] << 8) ||
	  gb_sum != (s5k4e5ya_lsccalib_data[i+448] | s5k4e5ya_lsccalib_data[i+449] << 8) ||
	  b_sum != (s5k4e5ya_lsccalib_data[i+672] | s5k4e5ya_lsccalib_data[i+673] << 8)) {
	  pr_err("Checksum Error!! r_sum= 0x%x , gr_sum= 0x%x , gb_sum= 0x%x, b_sum= 0x%x\n", 
		  r_sum, gr_sum, gb_sum, b_sum);
	  s5k4e5ya_calib_supp_info.lsc.is_supported = FALSE;
  }

//                                                                      
}

static void s5k4e5ya_format_afdata(void)
{
	s5k4e5ya_af_data.start_dac = (uint16_t)(s5k4e5ya_afcalib_data[1] << 8) |
		s5k4e5ya_afcalib_data[0];
	s5k4e5ya_af_data.macro_dac = (uint16_t)(s5k4e5ya_afcalib_data[3] << 8) |
		s5k4e5ya_afcalib_data[2];
        //pr_err(" start= 0x%x , macro= 0x%x \n", s5k4e5ya_af_data.start_dac, s5k4e5ya_af_data.macro_dac);
}

void s5k4e5ya_format_calibrationdata(void)
{
	s5k4e5ya_format_wbdata();
	s5k4e5ya_format_lscdata();
	s5k4e5ya_format_afdata();
}
static struct msm_eeprom_ctrl_t s5k4e5ya_eeprom_t = {
	.i2c_driver = &s5k4e5ya_eeprom_i2c_driver,
	.i2c_addr = 0x28,
	.eeprom_v4l2_subdev_ops = &s5k4e5ya_eeprom_subdev_ops,

	.i2c_client = {
		.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
	},

	.eeprom_mutex = &s5k4e5ya_eeprom_mutex,

	.func_tbl = {
		.eeprom_init = NULL,
		.eeprom_release = NULL,
		.eeprom_get_info = msm_camera_eeprom_get_info,
		.eeprom_get_data = msm_camera_eeprom_get_data,
		.eeprom_set_dev_addr = NULL,
		.eeprom_format_data = s5k4e5ya_format_calibrationdata,
	},
	.info = &s5k4e5ya_calib_supp_info,
	.info_size = sizeof(struct msm_camera_eeprom_info_t),
	.read_tbl = s5k4e5ya_eeprom_read_tbl,
	.read_tbl_size = ARRAY_SIZE(s5k4e5ya_eeprom_read_tbl),
	.data_tbl = s5k4e5ya_eeprom_data_tbl,
	.data_tbl_size = ARRAY_SIZE(s5k4e5ya_eeprom_data_tbl),
};

subsys_initcall(s5k4e5ya_eeprom_i2c_add_driver);
MODULE_DESCRIPTION("S5K4E5YA EEPROM");
MODULE_LICENSE("GPL v2");
