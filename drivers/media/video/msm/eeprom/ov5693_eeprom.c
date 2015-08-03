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

DEFINE_MUTEX(ov5693_eeprom_mutex);
static struct msm_eeprom_ctrl_t ov5693_eeprom_t;

static const struct i2c_device_id ov5693_eeprom_i2c_id[] = {
	{"ov5693_eeprom", (kernel_ulong_t)&ov5693_eeprom_t},
	{ }
};

static struct i2c_driver ov5693_eeprom_i2c_driver = {
	.id_table = ov5693_eeprom_i2c_id,
	.probe  = msm_eeprom_i2c_probe,
	.remove = __exit_p(ov5693_eeprom_i2c_remove),
	.driver = {
		.name = "ov5693_eeprom",
	},
};

static int __init ov5693_eeprom_i2c_add_driver(void)
{
	int rc = 0;
	rc = i2c_add_driver(ov5693_eeprom_t.i2c_driver);
	return rc;
}

static struct v4l2_subdev_core_ops ov5693_eeprom_subdev_core_ops = {
	.ioctl = msm_eeprom_subdev_ioctl,
};

static struct v4l2_subdev_ops ov5693_eeprom_subdev_ops = {
	.core = &ov5693_eeprom_subdev_core_ops,
};

uint8_t ov5693_wbcalib_data[6];
struct msm_calib_wb ov5693_wb_data;
uint8_t ov5693_lsccalib_data[886]; //                                                                    
struct msm_calib_lsc ov5693_lsc_data;
uint8_t ov5693_afcalib_data[4];
struct msm_calib_af ov5693_af_data;

/*                                                                                                         */
uint8_t ov5693_idcalib_data;
struct msm_calib_id ov5693_id_data;
/*                                                                                                         */

static struct msm_camera_eeprom_info_t ov5693_calib_supp_info = {
	{ TRUE, 4, 2, 1}, // af
	{ TRUE, 6, 0, 1024},  //                                                                          
	{ TRUE, 886, 1, 255}, //                                                                            
	{ TRUE, 1, 3, 1}, //                                                                                                               
	{FALSE, 0, 0, 1},
	{FALSE, 0, 0, 1},
};

static struct msm_camera_eeprom_read_t ov5693_eeprom_read_tbl[] = {
	{0x3f, &ov5693_wbcalib_data[0], 6, 1},
	{0x4c, &ov5693_lsccalib_data[0], 886, 0}, //                                                                          
	{0x45, &ov5693_afcalib_data[0], 4, 1},
	{0x00, &ov5693_idcalib_data, 1, 0}, /*                                                                                                       */
};

static struct msm_camera_eeprom_data_t ov5693_eeprom_data_tbl[] = {
	{&ov5693_wb_data, sizeof(struct msm_calib_wb)},
	{&ov5693_lsc_data, sizeof(struct msm_calib_lsc)},
	{&ov5693_af_data, sizeof(struct msm_calib_af)},
	{&ov5693_id_data, sizeof(struct msm_calib_id)}, /*                                                                                                       */
};

/*                                                                                                         */
static void ov5693_format_iddata(void)
{
	ov5693_id_data.sensor_id= (uint16_t)ov5693_idcalib_data;
	//pr_err("ov5693_id_data.sensor_id = 0x%x \n", ov5693_id_data.sensor_id);
}
/*                                                                                                         */

//                                                                      
#define R_OVER_G_TYP 561 // 692 // 20140401 Andy moidified
#define B_OVER_G_TYP 575 // 671 // 20140401 Andy moidified
#define GR_OVER_GB_TYP 1024
//                                                                      
static void ov5693_format_wbdata(void)
{

#if 1
	/*                                                                                      */
	ov5693_wb_data.r_over_g = (uint16_t)(ov5693_wbcalib_data[0] << 8) |
		ov5693_wbcalib_data[1];
	ov5693_wb_data.b_over_g = (uint16_t)(ov5693_wbcalib_data[2] << 8) |
		ov5693_wbcalib_data[3];
	ov5693_wb_data.gr_over_gb = (uint16_t)(ov5693_wbcalib_data[4] << 8) |
		ov5693_wbcalib_data[5];
//                                                                      
    if (ov5693_wb_data.r_over_g > R_OVER_G_TYP + (R_OVER_G_TYP >> 2) || /* +- 25% */
		ov5693_wb_data.r_over_g < R_OVER_G_TYP - (R_OVER_G_TYP >> 2) ||
		ov5693_wb_data.b_over_g > B_OVER_G_TYP + (R_OVER_G_TYP >> 2) ||
		ov5693_wb_data.b_over_g < B_OVER_G_TYP - (R_OVER_G_TYP >> 2) ||
		ov5693_wb_data.gr_over_gb > GR_OVER_GB_TYP + (R_OVER_G_TYP >> 3) || /* +- 12.5% */
		ov5693_wb_data.gr_over_gb < GR_OVER_GB_TYP - (R_OVER_G_TYP >> 3)) {
	  pr_err("Cal data range over!! r_over_g=0x%x, b_over_g=0x%x, gr_over_gb=0x%x\n", 
			ov5693_wb_data.r_over_g, 
			ov5693_wb_data.b_over_g, 
			ov5693_wb_data.gr_over_gb);
	  ov5693_calib_supp_info.wb.is_supported = FALSE;
    }
//                                                                      
	/*                                                                                     */
#else
	ov5693_wb_data.r_over_g = (uint16_t)(ov5693_wbcalib_data[1] << 8) |
		ov5693_wbcalib_data[0];
	ov5693_wb_data.b_over_g = (uint16_t)(ov5693_wbcalib_data[3] << 8) |
		ov5693_wbcalib_data[2];
	ov5693_wb_data.gr_over_gb = (uint16_t)(ov5693_wbcalib_data[5] << 8) |
		ov5693_wbcalib_data[4];
#endif
        //pr_err(" r_over_g= 0x%x , b_over_g= 0x%x , gr_over_gb= 0x%x \n", ov5693_wb_data.r_over_g, ov5693_wb_data.b_over_g, ov5693_wb_data.gr_over_gb);

}

#define LSC_CHANNEL_OFFSET 221
static void ov5693_format_lscdata(void)
{
  int i;
//                                                                      
  uint16_t r_sum = 0, gr_sum = 0, gb_sum = 0, b_sum = 0;
  uint16_t total_check_sum = 0, total_check_sum_of_eeprom = 0;

  for (i = 0; i < LSC_CHANNEL_OFFSET; i++) {
	ov5693_lsc_data.r_gain[i]= ov5693_lsccalib_data[i];
	ov5693_lsc_data.gr_gain[i]= ov5693_lsccalib_data[i+LSC_CHANNEL_OFFSET];
	ov5693_lsc_data.gb_gain[i]= ov5693_lsccalib_data[i+LSC_CHANNEL_OFFSET*2];
	ov5693_lsc_data.b_gain[i]= ov5693_lsccalib_data[i+LSC_CHANNEL_OFFSET*3];

	r_sum += ov5693_lsc_data.r_gain[i];
	gr_sum += ov5693_lsc_data.gr_gain[i];
	gb_sum += ov5693_lsc_data.gb_gain[i];
	b_sum += ov5693_lsc_data.b_gain[i];
  }

  total_check_sum = r_sum + gr_sum + gb_sum + b_sum;
  total_check_sum_of_eeprom = (ov5693_lsccalib_data[LSC_CHANNEL_OFFSET*4] << 8) |
      (ov5693_lsccalib_data[LSC_CHANNEL_OFFSET*4+1]);

  if(total_check_sum != total_check_sum_of_eeprom) {
    pr_err("LSC Checksum Error!! r_sum= 0x%x , gr_sum= 0x%x , gb_sum= 0x%x, b_sum= 0x%x\n",
        r_sum, gr_sum, gb_sum, b_sum);

    pr_err("LSC Checksum Error!! total_check_sum = 0x%x, total_check_sum_of_eeprom = 0x%x\n",
        total_check_sum, total_check_sum_of_eeprom);

    ov5693_calib_supp_info.lsc.is_supported = FALSE;
  }
//                                                                      
}

static void ov5693_format_afdata(void)
{
	ov5693_af_data.start_dac = (uint16_t)(ov5693_afcalib_data[1] << 8) |
		ov5693_afcalib_data[0];
	ov5693_af_data.macro_dac = (uint16_t)(ov5693_afcalib_data[3] << 8) |
		ov5693_afcalib_data[2];
        //pr_err(" start= 0x%x , macro= 0x%x \n", ov5693_af_data.start_dac, ov5693_af_data.macro_dac);
}

void ov5693_format_calibrationdata(void)
{
	ov5693_format_iddata(); /*                                                                                                       */
	ov5693_format_wbdata();
	ov5693_format_lscdata();
	ov5693_format_afdata();
}
static struct msm_eeprom_ctrl_t ov5693_eeprom_t = {
	.i2c_driver = &ov5693_eeprom_i2c_driver,
	.i2c_addr = 0x28,
	.eeprom_v4l2_subdev_ops = &ov5693_eeprom_subdev_ops,

	.i2c_client = {
		.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
	},

	.eeprom_mutex = &ov5693_eeprom_mutex,

	.func_tbl = {
		.eeprom_init = NULL,
		.eeprom_release = NULL,
		.eeprom_get_info = msm_camera_eeprom_get_info,
		.eeprom_get_data = msm_camera_eeprom_get_data,
		.eeprom_set_dev_addr = NULL,
		.eeprom_format_data = ov5693_format_calibrationdata,
	},
	.info = &ov5693_calib_supp_info,
	.info_size = sizeof(struct msm_camera_eeprom_info_t),
	.read_tbl = ov5693_eeprom_read_tbl,
	.read_tbl_size = ARRAY_SIZE(ov5693_eeprom_read_tbl),
	.data_tbl = ov5693_eeprom_data_tbl,
	.data_tbl_size = ARRAY_SIZE(ov5693_eeprom_data_tbl),
};

subsys_initcall(ov5693_eeprom_i2c_add_driver);
MODULE_DESCRIPTION("OV5693 EEPROM");
MODULE_LICENSE("GPL v2");
