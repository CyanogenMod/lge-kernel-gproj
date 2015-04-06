/* Copyright (c) 2012, The Linux Foundation. All rights reserved.
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

DEFINE_MUTEX(imx091_eeprom_mutex);
static struct msm_eeprom_ctrl_t imx091_eeprom_t;

static const struct i2c_device_id imx091_eeprom_i2c_id[] = {
	{"imx091_eeprom", (kernel_ulong_t)&imx091_eeprom_t},
	{ }
};

static struct i2c_driver imx091_eeprom_i2c_driver = {
	.id_table = imx091_eeprom_i2c_id,
	.probe  = msm_eeprom_i2c_probe,
	.remove = __exit_p(imx091_eeprom_i2c_remove),
	.driver = {
		.name = "imx091_eeprom",
	},
};

static int __init imx091_eeprom_i2c_add_driver(void)
{
	int rc = 0;
	rc = i2c_add_driver(imx091_eeprom_t.i2c_driver);
	return rc;
}

static struct v4l2_subdev_core_ops imx091_eeprom_subdev_core_ops = {
	.ioctl = msm_eeprom_subdev_ioctl,
};

static struct v4l2_subdev_ops imx091_eeprom_subdev_ops = {
	.core = &imx091_eeprom_subdev_core_ops,
};

uint8_t imx091_wb50calib_data[6];
uint8_t imx091_wb30calib_data[6];
uint8_t imx091_afcalib_data[8]; //   
uint8_t imx091_lsc50calib_data[884];
uint8_t imx091_lsc40calib_data[884];
/*                                                                 */
uint8_t imx091_af_defocus_data[11];
/*                                                                 */
struct msm_calib_wb imx091_wb50_data;
struct msm_calib_wb imx091_wb30_data;
struct msm_calib_af imx091_af_data;
struct msm_calib_lsc imx091_lsc50_data;
struct msm_calib_lsc imx091_lsc40_data;

//                                                        
uint8_t imx091_Cal_ver; // rafal47 0813
struct msm_calib_ver imx091_Calib_Ver; // rafal47 0813
//                                                      

static struct msm_camera_eeprom_info_t imx091_calib_supp_info = {
	{TRUE, 8, 4, 1}, //                                          
	{TRUE, 6, 0, 1024},//                                          
	{TRUE, 6, 1, 1024},//                                            
//                                                             
	{TRUE, 884, 2, 255}, //                                           
	{TRUE, 884, 3, 255}, //                                           
	{FALSE, 0, 0, 1}, //       
	{TRUE, 1, 5, 1}, //                                                  
};

static struct msm_camera_eeprom_read_t imx091_eeprom_read_tbl[] = {
	{0x00, &imx091_wb50calib_data[0], 6, 1}, //                             
	{0x382, &imx091_wb30calib_data[0], 6, 1}, //                             
	{0x0C, &imx091_lsc50calib_data[0], 884, 0}, //LSC D50
	{0x38A, &imx091_lsc40calib_data[0], 884, 0}, //LSC A
	{0x703, &imx091_afcalib_data[0], 8, 1}, //                             
	{0x770, &imx091_Cal_ver, 1, 0}, //                                                  
/*                                                                 */
	{0x850, &imx091_af_defocus_data[0], 11, 0}, //                                             
/*                                                                 */
};

static struct msm_camera_eeprom_data_t imx091_eeprom_data_tbl[] = {
	{&imx091_wb50_data, sizeof(struct msm_calib_wb)},
	{&imx091_wb30_data, sizeof(struct msm_calib_wb)},
	{&imx091_lsc50_data, sizeof(struct msm_calib_lsc)},
	{&imx091_lsc40_data, sizeof(struct msm_calib_lsc)},
	{&imx091_af_data, sizeof(struct msm_calib_af)},
	{&imx091_Calib_Ver, sizeof(struct msm_calib_ver)}, //                                                  
};

//                                                        
static void imx091_format_Cal_ver(void)
{
		imx091_Calib_Ver.cal_ver = 0;
		imx091_Calib_Ver.cal_ver = (imx091_Cal_ver); // rafal47 0813
}
//                                                      

static void imx091_format_wbdata(void)
{
	imx091_wb50_data.r_over_g = (uint16_t)(imx091_wb50calib_data[1] << 8) |
		(imx091_wb50calib_data[0]);
	imx091_wb50_data.b_over_g = (uint16_t)(imx091_wb50calib_data[3] << 8) |
		(imx091_wb50calib_data[2]);
	imx091_wb50_data.gr_over_gb = (uint16_t)(imx091_wb50calib_data[5] << 8) |
		(imx091_wb50calib_data[4]);

	imx091_wb30_data.r_over_g = (uint16_t)(imx091_wb30calib_data[1] << 8) |
		(imx091_wb30calib_data[0]);
	imx091_wb30_data.b_over_g = (uint16_t)(imx091_wb30calib_data[3] << 8) |
		(imx091_wb30calib_data[2]);
	imx091_wb30_data.gr_over_gb = (uint16_t)(imx091_wb30calib_data[5] << 8) |
		(imx091_wb30calib_data[4]);
}

static void imx091_format_lscdata(void)
{
  int i;

  for (i = 0; i < 221; i++) {
	imx091_lsc50_data.r_gain[i]= imx091_lsc50calib_data[i];
	imx091_lsc50_data.gr_gain[i]= imx091_lsc50calib_data[i+221];
	imx091_lsc50_data.gb_gain[i]= imx091_lsc50calib_data[i+442];
	imx091_lsc50_data.b_gain[i]= imx091_lsc50calib_data[i+663];

	imx091_lsc40_data.r_gain[i]= imx091_lsc40calib_data[i];
	imx091_lsc40_data.gr_gain[i]= imx091_lsc40calib_data[i+221];
	imx091_lsc40_data.gb_gain[i]= imx091_lsc40calib_data[i+442];
	imx091_lsc40_data.b_gain[i]= imx091_lsc40calib_data[i+663];
	}
}

static void imx091_format_afdata(void)
{
	imx091_af_data.inf_dac = (uint16_t)(imx091_afcalib_data[1] << 8) |
		imx091_afcalib_data[0];
	imx091_af_data.macro_dac = (uint16_t)(imx091_afcalib_data[3] << 8) |
		imx091_afcalib_data[2];
	imx091_af_data.start_dac = (uint16_t)(imx091_afcalib_data[5] << 8) |
		imx091_afcalib_data[4];
}

void imx091_format_calibrationdata(void)
{
    imx091_format_Cal_ver(); //                                                  
	imx091_format_wbdata();
	imx091_format_afdata();
	imx091_format_lscdata();
}
static struct msm_eeprom_ctrl_t imx091_eeprom_t = {
	.i2c_driver = &imx091_eeprom_i2c_driver,
	.i2c_addr = 0xA6, //                                                      
	.eeprom_v4l2_subdev_ops = &imx091_eeprom_subdev_ops,

	.i2c_client = {
	//                                                                               
		.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
	//                                                                             
	},

	.eeprom_mutex = &imx091_eeprom_mutex,

	.func_tbl = {
		.eeprom_init = NULL,
		.eeprom_release = NULL,
		.eeprom_get_info = msm_camera_eeprom_get_info,
		.eeprom_get_data = msm_camera_eeprom_get_data,
		.eeprom_set_dev_addr = NULL,
		.eeprom_format_data = imx091_format_calibrationdata,
	},
	.info = &imx091_calib_supp_info,
	.info_size = sizeof(struct msm_camera_eeprom_info_t),
	.read_tbl = imx091_eeprom_read_tbl,
	.read_tbl_size = ARRAY_SIZE(imx091_eeprom_read_tbl),
	.data_tbl = imx091_eeprom_data_tbl,
	.data_tbl_size = ARRAY_SIZE(imx091_eeprom_data_tbl),
};

subsys_initcall(imx091_eeprom_i2c_add_driver);
MODULE_DESCRIPTION("imx091 EEPROM");
MODULE_LICENSE("GPL v2");
