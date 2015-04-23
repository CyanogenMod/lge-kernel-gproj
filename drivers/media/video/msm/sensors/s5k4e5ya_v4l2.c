/* Copyright (c) 2012, Code Aurora Forum. All rights reserved.
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
#include "msm_sensor.h"
#define SENSOR_NAME "s5k4e5ya"
#define PLATFORM_DRIVER_NAME "msm_camera_s5k4e5ya"
#define s5k4e5ya_obj s5k4e5ya_##obj
#define MSB                             1
#define LSB                             0

DEFINE_MUTEX(s5k4e5ya_mut);
static struct msm_sensor_ctrl_t s5k4e5ya_s_ctrl;

static struct msm_camera_i2c_reg_conf s5k4e5ya_start_settings[] = {
	{0x0100, 0x01},
};

static struct msm_camera_i2c_reg_conf s5k4e5ya_stop_settings[] = {
	{0x0100, 0x00},
};

static struct msm_camera_i2c_reg_conf s5k4e5ya_groupon_settings[] = {
//                                                                                    
};

static struct msm_camera_i2c_reg_conf s5k4e5ya_groupoff_settings[] = {
//                                                                                    
};

#if 0
static struct msm_camera_i2c_reg_conf s5k4e5ya_prev_settings[] = {
	// Reset for operation ...
	
	//--> This registers are for FACTORY ONLY. If you change it without prior notification.
	// YOU are RESPONSIBLE for the FAILURE that will happen in the future.
	//+++++++++++++++++++++++++++++++//
	// Analog Setting
	//// CDS timing setting ...
	{0x3000, 0x05}, // ct_ld_start (default = 07h)
	{0x3001, 0x03}, // ct_sl_start (default = 05h)
	{0x3002, 0x08}, // ct_rx_start (default = 21h)
	{0x3003, 0x0A}, // ct_cds_start (default = 23h)
	{0x3004, 0x50}, // ct_smp_width (default = 60h)
	{0x3005, 0x0E}, // ct_az_width (default = 28h)
	{0x3006, 0x5E}, // ct_s1r_width (default = 88h)
	{0x3007, 0x00}, // ct_tx_start (default = 06h)
	{0x3008, 0x78}, // ct_tx_width 1.5us (default = 7Ch)
	{0x3009, 0x78}, // ct_stx_width 1.5us (default = 7Ch)
	{0x300A, 0x50}, // ct_dtx_width 1us (default = 3Eh)
	{0x300B, 0x08}, // ct_rmp_rst_start (default = 44h)
	{0x300C, 0x14}, // ct_rmp_sig_start (default = 48h)
	{0x300D, 0x00}, // ct_rmp_lat (default = 02h)
	//S300E28 // dshut_en / srx_en
	{0x300F, 0x40}, // [7]CDS limiter enable, [1]limiter off @ auto-zero
	{0x301B, 0x77}, // COMP bias [7:4]comp1, [3:0]comp2
	
	//s30ae0a	// High Speed TEST mode
			   
	//// CDS option setting ...
	{0x3010, 0x00}, // smp_en[2]=0(00) 1(04) row_id[1:0] = 00			   
	{0x3011, 0x3A}, // RST_MX (384), SIG_MX (1440)
	
	{0x3012, 0x30}, // SIG offset1	48 code 
	{0x3013, 0xA0}, // RST offset1	192 code
	{0x3014, 0x00}, // SIG offset2
	{0x3015, 0x00}, // RST offset2								  
	{0x3016, 0x52}, // ADC_SAT (450mV)							   
	{0x3017, 0x94}, // RMP_INIT[3:0](RMP_REG) 1.8V MS[6:4]=1	 
	{0x3018, 0x70}, // rmp option - ramp connect[MSB] +RMP INIT DAC MIN
	
	{0x301D, 0xD4}, // CLP level (default = 0Fh)
			
	{0x3021, 0x02}, // inrush ctrl[1] off							   
	{0x3022, 0x24}, // pump ring oscillator set [7:4]=CP, [3:0]=NCP 
	{0x3024, 0x40}, // pix voltage 3.06V   (default = 88h)		
	{0x3027, 0x08}, // ntg voltage (default = 04h)
	
	//// Pixel option setting ...	   
	{0x301C, 0x06}, // Pixel Bias [3:0] (default = 03h)
	{0x30D8, 0x3F}, // All tx off 2f, on 3f
	 
	//+++++++++++++++++++++++++++++++//
	// ADLC setting ...
	{0x3070, 0x5B}, // [6]L-ADLC BPR, [4]ch sel, [3]L-ADLC, [2]F-ADLC
	{0x3071, 0x00}, // F&L-adlc max 127 (default = 11h, max 255)
	{0x3080, 0x04}, // F-ADLC filter A (default = 10h)
	{0x3081, 0x38}, // F-ADLC filter B (default = 20h)
	// Internal VDD Regulator Setting 
	{0x302E, 0x0B},  // 0x08 -> 0x0B (For Internal regulator margin) 2010.08.05
	
	//+++++++++++++++++++++++++++++++//
	// MIPI setting
	{0x30BD, 0x00},//SEL_CCP[0]
	{0x3084, 0x15},//SYNC Mode
	{0x30BE, 0x1A},//M_PCLKDIV_AUTO[4], M_DIV_PCLK[3:0]
	{0x30C1, 0x01},//pack video enable [0]
	{0x30EE, 0x02},//DPHY enable [1]
	{0x3111, 0x86},//Embedded data off [5]
	
	//30E80F  // : 0F Continous mode(default) , : 07 Non-Continous mode   
	{0x30E3, 0x38},  //according to MCLK 24Mhz
	{0x30E4, 0x40},
	{0x3113, 0x70},
	{0x3114, 0x80},
	{0x3115, 0x7B},
	{0x3116, 0xC0},
	{0x30EE, 0x12},
	
	
	
		// PLL setting ...
	//// input clock 24MHz
	////// (3) MIPI 1-lane Serial(TST = 0000b or TST = 0010b), 15 fps
	{0x0305, 0x06}, //24/6 = 4Mhz
	{0x0306, 0x00},
	{0x0307, 0x6A}, //4Mhz x 100 x 2 = 848M
	{0x30B5, 0x00}, //848 / 2^0 = 848M per lane, total 1696Mbps
	{0x30E2, 0x02}, //num lanes[1:0] = 2
	{0x30F1, 0xD0}, //band control 0x30F1[7:4] 0x3 : 170M ~ 220Mb (per lane)
	{0x30BC, 0xB8}, // [7]bit : DBLR enable, [6]~[3]bit : DBLR Div
			// DBLR clock = Pll output/DBLR Div = 848/14=60.57 >= 60Mhz (lane �� ������ ����)
			//	/2 :  88h
			//	/4 :  90h
			//	/6 :  98h
			//	/8 :  A0h
			//	/10:  A8h
			//	/12:  B0h
			//	/14:  B8h
	
	{0x30BF, 0xAB},//outif_enable[7], data_type[5:0](2Bh = bayer 10bit)
	{0x30C0, 0x40},//video_offset[7:4] 1600%12
	{0x30C8, 0x06},//video_data_length 1600 = 1280 * 1.25
	{0x30C9, 0x40},
	
	{0x3112, 0x00}, //gain option sel off, shuter and gain setting for same time update
	{0x3030, 0x07}, //old shut mode  
	
	//--> This register are for user.
	//+++++++++++++++++++++++++++++++//
	// Integration setting ... 
	{0x0200, 0x03},    //fine integration time, WARNING: User should use fixed fine integration time.
	{0x0201, 0x5C},
	//{0x0203, 0x5C},
	
	
	{0x0202, 0x07},    // coarse integration time, Max Coarse integration time = frame lenth line -5   
	{0x0203, 0x02},
	
	{0x0204, 0x00},    // analog gain
	{0x0205, 0x20},
//                                                                   
#if 0	
	{0x0340, 0x07},   // Frame Length, Min frame lenth = Vsize + 12
	{0x0341, 0x07},   // 720 + 12 = 732 dec = 2DCh
					  //07B4 = 27fps, 0707h=30fps
#else
	{0x0340, 0x07},   // Frame Length, Min frame lenth = Vsize + 12
	{0x0341, 0x25},   // 720 + 12 = 732 dec = 2DCh
#endif
//                                                                   
	{0x0342, 0x0C},    // Line Length, MIPI Non-Continuous mode : B30h �̻�, Continous mode :AB2h �̻�.] 
	{0x0343, 0x6F},    // B30h �̻��̸� ��� �̻����.
	
	// MIPI Size Setting
	{0x30A9, 0x02},//Horizontal Binning On
	{0x300E, 0x29},//                                                                                   
	{0x302B, 0x00}, // [0]blst_en, [1]sl_off
	{0x3029, 0x74}, //04 RST_MX(ramp)(384) + ring oscillator
	
	{0x0380, 0x00},//x_even_inc 1
	{0x0381, 0x01},
	{0x0382, 0x00},//x_odd_inc 1
	{0x0383, 0x01},
	{0x0384, 0x00},//y_even_inc 1
	{0x0385, 0x01},
	{0x0386, 0x00},//y_odd_inc 3
	{0x0387, 0x03},
	
	{0x0344, 0x00},//x_addr_start
	{0x0345, 0x18},
	{0x0346, 0x00},//y_addr_start
	{0x0347, 0x14},
	{0x0348, 0x0A},//x_addr_end
	{0x0349, 0x17},
	{0x034A, 0x07},//y_addr_end
	{0x034B, 0x93},
	
	{0x034C, 0x05},//x_output_size 1304
	{0x034D, 0x00},
	{0x034E, 0x03},//y_output_size 980
	{0x034F, 0xC0},
};
#endif
static struct msm_camera_i2c_reg_conf s5k4e5ya_snap_settings[] = {
	//--> This registers are for FACTORY ONLY. If you change it without prior notification.
	// YOU are RESPONSIBLE for the FAILURE that will happen in the future.
	//+++++++++++++++++++++++++++++++//
	// Analog Setting
	//// CDS timing setting ...
	{0x3000, 0x05}, // ct_ld_start (default = 07h)
	{0x3001, 0x03}, // ct_sl_start (default = 05h)
	{0x3002, 0x08}, // ct_rx_start (default = 21h)
	{0x3003, 0x0A}, // ct_cds_start (default = 23h)
	{0x3004, 0x50}, // ct_smp_width (default = 60h)
	{0x3005, 0x0E}, // ct_az_width (default = 28h)
	{0x3006, 0x5E}, // ct_s1r_width (default = 88h)
	{0x3007, 0x00}, // ct_tx_start (default = 06h)
	{0x3008, 0x78}, // ct_tx_width 1.5us (default = 7Ch)
	{0x3009, 0x78}, // ct_stx_width 1.5us (default = 7Ch)
	{0x300A, 0x50}, // ct_dtx_width 1us (default = 3Eh)
	{0x300B, 0x08}, // ct_rmp_rst_start (default = 44h)
	{0x300C, 0x14}, // ct_rmp_sig_start (default = 48h)
	{0x300D, 0x00}, // ct_rmp_lat (default = 02h)
	//S300E28 // dshut_en / srx_en
	{0x300F, 0x40}, // [7]CDS limiter enable, [1]limiter off @ auto-zero
	{0x301B, 0x77}, // COMP bias [7:4]comp1, [3:0]comp2
	
	//s30ae0a	// High Speed TEST mode
			   
	//// CDS option setting ...
	{0x3010, 0x00}, // smp_en[2]=0(00) 1(04) row_id[1:0] = 00			   
	{0x3011, 0x3A}, // RST_MX (384), SIG_MX (1440)
	
	{0x3012, 0x30}, // SIG offset1	48 code 
	{0x3013, 0xA0}, // RST offset1	192 code
	{0x3014, 0x00}, // SIG offset2
	{0x3015, 0x00}, // RST offset2								  
	{0x3016, 0x52}, // ADC_SAT (450mV)							   
	{0x3017, 0x94}, // RMP_INIT[3:0](RMP_REG) 1.8V MS[6:4]=1	 
	{0x3018, 0x70}, // rmp option - ramp connect[MSB] +RMP INIT DAC MIN
	
	{0x301D, 0xD4}, // CLP level (default = 0Fh)
			
	{0x3021, 0x02}, // inrush ctrl[1] off							   
	{0x3022, 0x24}, // pump ring oscillator set [7:4]=CP, [3:0]=NCP 
	{0x3024, 0x40}, // pix voltage 3.06V   (default = 88h)		
	{0x3027, 0x08}, // ntg voltage (default = 04h)
	
	//// Pixel option setting ...	   
	{0x301C, 0x06}, // Pixel Bias [3:0] (default = 03h)
	{0x30D8, 0x3F}, // All tx off 2f, on 3f
	 
	//+++++++++++++++++++++++++++++++//
	// ADLC setting ...
	{0x3070, 0x5B}, // [6]L-ADLC BPR, [4]ch sel, [3]L-ADLC, [2]F-ADLC
	{0x3071, 0x00}, // F&L-adlc max 127 (default = 11h, max 255)
	{0x3080, 0x04}, // F-ADLC filter A (default = 10h)
	{0x3081, 0x38}, // F-ADLC filter B (default = 20h)
	// Internal VDD Regulator Setting 
	{0x302E, 0x0B},  // 0x08 -> 0x0B (For Internal regulator margin) 2010.08.05
	
	//+++++++++++++++++++++++++++++++//
	// MIPI setting
	{0x30BD, 0x00},//SEL_CCP[0]
	{0x3084, 0x15},//SYNC Mode
	{0x30BE, 0x1A},//M_PCLKDIV_AUTO[4], M_DIV_PCLK[3:0]
	{0x30C1, 0x01},//pack video enable [0]
	{0x30EE, 0x02},//DPHY enable [1]
	{0x3111, 0x86},//Embedded data off [5]
	
	//30E80F  // : 0F Continous mode(default) , : 07 Non-Continous mode   
	{0x30E3, 0x38},  //according to MCLK 24Mhz
	{0x30E4, 0x40},
	{0x3113, 0x70},
	{0x3114, 0x80},
	{0x3115, 0x7B},
	{0x3116, 0xC0},
	{0x30EE, 0x12},
	
	//+++++++++++++++++++++++++++++++//
	// PLL setting ...
	//// input clock 24MHz
	////// (3) MIPI 1-lane Serial(TST = 0000b or TST = 0010b), 15 fps
	{0x0305, 0x06}, //24/6 = 4Mhz
	{0x0306, 0x00},
 	{0x0307, 0x6B}, //4Mhz x 107 x 2 = 856M 
	{0x30B5, 0x00}, //848 / 2^0 = 848M per lane, total 1696Mbps
	{0x30E2, 0x02}, //num lanes[1:0] = 2
	{0x30F1, 0xD0}, //band control 0x30F1[7:4] 0x3 : 170M ~ 220Mb (per lane)
	{0x30BC, 0xB8}, // [7]bit : DBLR enable, [6]~[3]bit : DBLR Div
			// DBLR clock = Pll output/DBLR Div = 848/14=60.57 >= 60Mhz (lane �� ������ ����)
			//	/2 :  88h
			//	/4 :  90h
			//	/6 :  98h
			//	/8 :  A0h
			//	/10:  A8h
			//	/12:  B0h
			//	/14:  B8h
				
	{0x30BF, 0xAB},//outif_enable[7], data_type[5:0](2Bh = bayer 10bit)
	{0x30C0, 0x80},//video_offset[7:4] 3200%12
	{0x30C8, 0x0C},//video_data_length 3200 = 2560 * 1.25
	{0x30C9, 0xBC},
	
	{0x3112, 0x00}, //gain option sel off, shuter and gain setting for same time update
	{0x3030, 0x07}, //old shut mode  
	
	
	//--> This register are for user.
	//+++++++++++++++++++++++++++++++//
	// Integration setting ... 
	{0x0200, 0x03},    //fine integration time, WARNING: User should use fixed fine integration time.
	{0x0201, 0x5C},
	
	{0x0202, 0x08},    // coarse integration time, Max Coarse integration time = frame lenth line -5   
	{0x0203, 0x2B},
	
	{0x0204, 0x00},    // analog gain 
	{0x0205, 0x20},  
//                                                                   
#if 0	
	{0x0340, 0x07},   // Frame Length, Min frame lenth = Vsize + 12
	{0x0341, 0xB4},   //1960 + 12 = 1972 dec = 7B4h
					  //07B4 = 30fps,  0F6C = 15fps, 2E44 = 5fps
#else
	{0x0340, 0x08},   // Frame Length, Min frame lenth = Vsize + 12
	{0x0341, 0x30},   //1952 + 12 = 1972 dec = 7A8h
#endif
//                                                                   
	{0x0342, 0x0A},    // Line Length, MIPI Non-Continuous mode : B30h �̻�, Continous mode :AB2h �̻�.] 
	{0x0343, 0xB2},    // B30h �̻��̸� ��� �̻����. 
	// MIPI Size Setting
	{0x30A9, 0x03},//Horizontal Binning Off
	{0x300E, 0x28},//E8 Vertical Binning Off
	{0x302B, 0x01}, // [0]blst_en, [1]sl_off
	{0x3029, 0x34}, //04 RST_MX(ramp)(384) + ring oscillator
	
	{0x0380, 0x00},//x_even_inc 1
	{0x0381, 0x01},
	{0x0382, 0x00},//x_odd_inc 1
	{0x0383, 0x01},
	{0x0384, 0x00},//y_even_inc 1
	{0x0385, 0x01},
	{0x0386, 0x00},//y_odd_inc 3
	{0x0387, 0x01},
	
	{0x0344, 0x00},//x_addr_start
	{0x0345, 0x00},
	{0x0346, 0x00},//y_addr_start
	{0x0347, 0x00},
	{0x0348, 0x0A},//x_addr_end
	{0x0349, 0x2F},
	{0x034A, 0x07},//y_addr_end
	{0x034B, 0xA7},
	
	{0x034C, 0x0A},//x_output_size 2608
	{0x034D, 0x30},
	{0x034E, 0x07},//y_output_size 1952
	{0x034F, 0xA0},
};

static struct msm_camera_i2c_reg_conf s5k4e5ya_video_settings[] = {
	//--> This registers are for FACTORY ONLY. If you change it without prior notification.
	// YOU are RESPONSIBLE for the FAILURE that will happen in the future.
	//+++++++++++++++++++++++++++++++//
	// Analog Setting
	//// CDS timing setting ...
	{0x3000, 0x05}, // ct_ld_start (default = 07h)
	{0x3001, 0x03}, // ct_sl_start (default = 05h)
	{0x3002, 0x08}, // ct_rx_start (default = 21h)
	{0x3003, 0x0A}, // ct_cds_start (default = 23h)
	{0x3004, 0x50}, // ct_smp_width (default = 60h)
	{0x3005, 0x0E}, // ct_az_width (default = 28h)
	{0x3006, 0x5E}, // ct_s1r_width (default = 88h)
	{0x3007, 0x00}, // ct_tx_start (default = 06h)
	{0x3008, 0x78}, // ct_tx_width 1.5us (default = 7Ch)
	{0x3009, 0x78}, // ct_stx_width 1.5us (default = 7Ch)
	{0x300A, 0x50}, // ct_dtx_width 1us (default = 3Eh)
	{0x300B, 0x08}, // ct_rmp_rst_start (default = 44h)
	{0x300C, 0x14}, // ct_rmp_sig_start (default = 48h)
	{0x300D, 0x00}, // ct_rmp_lat (default = 02h)
	//S300E28 // dshut_en / srx_en
	{0x300F, 0x40}, // [7]CDS limiter enable, [1]limiter off @ auto-zero
	{0x301B, 0x77}, // COMP bias [7:4]comp1, [3:0]comp2
	
	//s30ae0a	// High Speed TEST mode
			   
	//// CDS option setting ...
	{0x3010, 0x00}, // smp_en[2]=0(00) 1(04) row_id[1:0] = 00			   
	{0x3011, 0x3A}, // RST_MX (384), SIG_MX (1440)
	
	{0x3012, 0x30}, // SIG offset1	48 code 
	{0x3013, 0xA0}, // RST offset1	192 code
	{0x3014, 0x00}, // SIG offset2
	{0x3015, 0x00}, // RST offset2								  
	{0x3016, 0x52}, // ADC_SAT (450mV)							   
	{0x3017, 0x94}, // RMP_INIT[3:0](RMP_REG) 1.8V MS[6:4]=1	 
	{0x3018, 0x70}, // rmp option - ramp connect[MSB] +RMP INIT DAC MIN
	
	{0x301D, 0xD4}, // CLP level (default = 0Fh)
			
	{0x3021, 0x02}, // inrush ctrl[1] off							   
	{0x3022, 0x24}, // pump ring oscillator set [7:4]=CP, [3:0]=NCP 
	{0x3024, 0x40}, // pix voltage 3.06V   (default = 88h)		
	{0x3027, 0x08}, // ntg voltage (default = 04h)
	
	//// Pixel option setting ...	   
	{0x301C, 0x06}, // Pixel Bias [3:0] (default = 03h)
	{0x30D8, 0x3F}, // All tx off 2f, on 3f
	
	//+++++++++++++++++++++++++++++++//
	// ADLC setting ...
	{0x3070, 0x5B}, // [6]L-ADLC BPR, [4]ch sel, [3]L-ADLC, [2]F-ADLC
	{0x3071, 0x00}, // F&L-adlc max 127 (default = 11h, max 255)
	{0x3080, 0x04}, // F-ADLC filter A (default = 10h)
	{0x3081, 0x38}, // F-ADLC filter B (default = 20h)
	// Internal VDD Regulator Setting 
	{0x302E, 0x0B},  // 0x08 -> 0x0B (For Internal regulator margin) 2010.08.05
	
	//+++++++++++++++++++++++++++++++//
	// MIPI setting
	{0x30BD, 0x00},//SEL_CCP[0]
	{0x3084, 0x15},//SYNC Mode
	{0x30BE, 0x1A},//M_PCLKDIV_AUTO[4], M_DIV_PCLK[3:0]
	{0x30C1, 0x01},//pack video enable [0]
	{0x30EE, 0x02},//DPHY enable [1]
	{0x3111, 0x86},//Embedded data off [5]
	
	//30E80F  // : 0F Continous mode(default) , : 07 Non-Continous mode   
	{0x30E3, 0x38},  //according to MCLK 24Mhz
	{0x30E4, 0x40},
	{0x3113, 0x70},
	{0x3114, 0x80},
	{0x3115, 0x7B},
	{0x3116, 0xC0},
	{0x30EE, 0x12},
	
	//+++++++++++++++++++++++++++++++//
	// PLL setting ...
	//// input clock 24MHz
	////// (3) MIPI 1-lane Serial(TST = 0000b or TST = 0010b), 15 fps
	{0x0305, 0x06}, //24/6 = 4Mhz
	{0x0306, 0x00},
 	{0x0307, 0x6B}, //4Mhz x 107 x 2 = 856M 
	{0x30B5, 0x00}, //848 / 2^0 = 848M per lane, total 1696Mbps
	{0x30E2, 0x02}, //num lanes[1:0] = 2
	{0x30F1, 0xD0}, //band control 0x30F1[7:4] 0x3 : 170M ~ 220Mb (per lane)
	{0x30BC, 0xB8}, // [7]bit : DBLR enable, [6]~[3]bit : DBLR Div
			// DBLR clock = Pll output/DBLR Div = 848/14=60.57 >= 60Mhz (lane �� ������ ����)
			//	/2 :  88h
			//	/4 :  90h
			//	/6 :  98h
			//	/8 :  A0h
			//	/10:  A8h
			//	/12:  B0h
			//	/14:  B8h
		   
	{0x30BF, 0xAB},//outif_enable[7], data_type[5:0](2Bh = bayer 10bit)
	{0x30C0, 0x80},//video_offset
	{0x30C8, 0x0C},//video_data_length 
	{0x30C9, 0x80},
	
	{0x3112, 0x00}, //gain option sel off, shuter and gain setting for same time update
	{0x3030, 0x07}, //old shut mode  
	
	
	//--> This register are for user.
	//+++++++++++++++++++++++++++++++//
	// Integration setting ... 
	{0x0200, 0x03},    //fine integration time, WARNING: User should use fixed fine integration time.
	{0x0201, 0x5C},
	
	{0x0202, 0x08},    // coarse integration time, Max Coarse integration time = frame lenth line -5   
	{0x0203, 0x1B},
	
	{0x0204, 0x00},    // analog gain
	{0x0205, 0x20},
//                                                                   
#if 0	
	{0x0340, 0x07},   // Frame Length, Min frame lenth = Vsize + 12
	{0x0341, 0xB4},   //1960 + 12 = 1972 dec = 7B4h
					  //07B4 = 30fps,  0F6C = 15fps, 2E44 = 5fps
#else
	{0x0340, 0x08},   
	{0x0341, 0x30},   //1960 + 12 = 1972 dec = 7B4h
#endif
//                                                                   
	{0x0342, 0x0A},    // Line Length, MIPI Non-Continuous mode : B30h �̻�, Continous mode :AB2h �̻�.] 
	{0x0343, 0xC0},    // B30h �̻��̸� ��� �̻����.
	
	// MIPI Size Setting
	{0x30A9, 0x03},//Horizontal Binning Off
	{0x300E, 0x28},//E8 Vertical Binning Off
	{0x302B, 0x01}, // [0]blst_en, [1]sl_off
	{0x3029, 0x34}, //04 RST_MX(ramp)(384) + ring oscillator
	
	{0x0380, 0x00},//x_even_inc 1
	{0x0381, 0x01},
	{0x0382, 0x00},//x_odd_inc 1
	{0x0383, 0x01},
	{0x0384, 0x00},//y_even_inc 1
	{0x0385, 0x01},
	{0x0386, 0x00},//y_odd_inc 3
	{0x0387, 0x01},
	
	{0x0344, 0x00},//x_addr_start
	{0x0345, 0x18}, 			   
	{0x0346, 0x00},//y_addr_start
	{0x0347, 0xF0},
	{0x0348, 0x0A},//x_addr_end,
	{0x0349, 0x18},
	{0x034A, 0x06},//y_addr_end
	{0x034B, 0x90},
	
	{0x034C, 0x0A},//x_output_size
	{0x034D, 0x00},
	{0x034E, 0x05},//y_output_size
	{0x034F, 0xA0},/*                                                                         */
};
static struct msm_camera_i2c_reg_conf s5k4e5ya_recommend_settings[] = {
#if 0
	// ******************* //
	// S5K4E5Y MIPI Setting
	// 
	// last update date : 2012. 10. 15
	//
	// Full size output (2608 x 1960)
	// This Setfile is optimized for 30 fps
	// 848Mbps (per lane), total 1696Mbps (2lane)
	// ******************* //
	//$MIPI[Width:2608,Height:1960,Format:Raw10,Lane:2,ErrorCheck:0,PolarityData:0,PolarityClock:0,Buffer:2, datarate:848]

	//+++++++++++++++++++++++++++++++//
	// Reset for operation ...
	//{0x0100, 0x00},  // stream off
	{0x0103, 0x01},  // software reset
	//--> This registers are for FACTORY ONLY. If you change it without prior notification.
	// YOU are RESPONSIBLE for the FAILURE that will happen in the future.
	//+++++++++++++++++++++++++++++++//
	// Analog Setting
	//// CDS timing setting ...
	{0x3000, 0x05},  	// ct_ld_start (default = 07h)
	{0x3001, 0x03},  	// ct_sl_start (default = 05h)
	{0x3002, 0x08},  	// ct_rx_start (default = 21h)
	{0x3003, 0x0A},  	// ct_cds_start (default = 23h)
	{0x3004, 0x50},  	// ct_smp_width (default = 60h)
	{0x3005, 0x0E},  	// ct_az_width (default = 28h)
	{0x3006, 0x5E},  	// ct_s1r_width (default = 88h)
	{0x3007, 0x00},  	// ct_tx_start (default = 06h)
	{0x3008, 0x78},  	// ct_tx_width 1.5us (default = 7Ch)
	{0x3009, 0x78},  	// ct_stx_width 1.5us (default = 7Ch)
	{0x300A, 0x50},  	// ct_dtx_width 1us (default = 3Eh)
	{0x300B, 0x08},  	// ct_rmp_rst_start (default = 44h)
	{0x300C, 0x14},  	// ct_rmp_sig_start (default = 48h)
	{0x300D, 0x00},  	// ct_rmp_lat (default = 02h)
	//{0x300E28 // dshut_en / srx_en
	{0x300F, 0x40},  	// [7]CDS limiter enable, [1]limiter off @ auto-zero
	{0x301B, 0x77},  	// COMP bias [7:4]comp1, [3:0]comp2

	//s30ae0a	// High Speed TEST mode
	           
	//// CDS option setting ...
	{0x3010, 0x00},  	// smp_en[2]=0(00) 1(04) row_id[1:0] = 00              
	{0x3011, 0x3A},  	// RST_MX (384), SIG_MX (1440)
	{0x3012, 0x30},  	// SIG offset1  48 code 
	{0x3013, 0xA0},  	// RST offset1  192 code
	{0x3014, 0x00},  	// SIG offset2
	{0x3015, 0x00},  	// RST offset2                                
	{0x3016, 0x52},  	// ADC_SAT (450mV)                             
	{0x3017, 0x94},  	// RMP_INIT[3:0](RMP_REG) 1.8V MS[6:4]=1     
	{0x3018, 0x70},  	// rmp option - ramp connect[MSB] +RMP INIT DAC MIN
	{0x301D, 0xD4},  	// CLP level (default = 0Fh)
	{0x3021, 0x02},  	// inrush ctrl[1] off                              
	{0x3022, 0x24},  	// pump ring oscillator set [7:4]=CP, [3:0]=NCP 
	{0x3024, 0x40},  	// pix voltage 3.06V   (default = 88h)      
	{0x3027, 0x08},  	// ntg voltage (default = 04h)

	//// Pixel option setting ...      
	{0x301C, 0x06},  	// Pixel Bias [3:0] (default = 03h)
	{0x30D8, 0x3F},  	// All tx off 2f, on 3f

	//+++++++++++++++++++++++++++++++//
	// ADLC setting ...
	{0x3070, 0x5B},  	// [6]L-ADLC BPR, [4]ch sel, [3]L-ADLC, [2]F-ADLC
	{0x3071, 0x00},  	// F&L-adlc max 127 (default = 11h, max 255)
	{0x3080, 0x04},  	// F-ADLC filter A (default = 10h)
	{0x3081, 0x38},  	// F-ADLC filter B (default = 20h)
	// Internal VDD Regulator Setting 
	{0x302E, 0x0B},    // 0x08 -> 0x0B (For Internal regulator margin) 2010.08.05

	//+++++++++++++++++++++++++++++++//
	// MIPI setting
	{0x30BD, 0x00}, //SEL_CCP[0]
	{0x3084, 0x15}, //SYNC Mode
	{0x30BE, 0x1A}, //M_PCLKDIV_AUTO[4], M_DIV_PCLK[3:0]
	{0x30C1, 0x01}, //pack video enable [0]
	{0x30EE, 0x02}, //DPHY enable [1]
	{0x3111, 0x86}, //Embedded data off [5]

	//30E80F  // : 0F Continous mode(default) , : 07 Non-Continous mode   
	{0x30E3, 0x38},   //according to MCLK 24Mhz
	{0x30E4, 0x40}, 
	{0x3113, 0x70}, 
	{0x3114, 0x80}, 
	{0x3115, 0x7B}, 
	{0x3116, 0xC0}, 
	{0x30EE, 0x12}, 

	//+++++++++++++++++++++++++++++++//
	// PLL setting ...
	//// input clock 24MHz
	////// (3) MIPI 1-lane Serial(TST = 0000b or TST = 0010b), 15 fps
	{0x0305, 0x06},  //24/6 = 4Mhz
	{0x0306, 0x00}, 
	{0x0307, 0x6A},  //4Mhz x 100 x 2 = 848M
	{0x30B5, 0x00},  //848 / 2^0 = 848M per lane, total 1696Mbps
	{0x30E2, 0x02},  //num lanes[1:0] = 2
	{0x30F1, 0xD0},  //band control 0x30F1[7:4] 0x3 : 170M ~ 220Mb (per lane)
	{0x30BC, 0xB8},  // [7]bit : DBLR enable, [6]~[3]bit : DBLR Div
	        // DBLR clock = Pll output/DBLR Div = 848/14=60.57 >= 60Mhz (lane �� ������ ����)
	        //  /2 :  88h
	        //  /4 :  90h
	        //  /6 :  98h
	        //  /8 :  A0h
	        //  /10:  A8h
	        //  /12:  B0h
	        //  /14:  B8h
	{0x30BF, 0xAB},  //outif_enable[7], data_type[5:0](2Bh = bayer 10bit)
	{0x30C0, 0x80},  //video_offset[7:4] 3260%12
	{0x30C8, 0x0C},  //video_data_length 3260 = 2608 * 1.25
	{0x30C9, 0xBC},  
	{0x3112, 0x00},  	//gain option sel off, shuter and gain setting for same time update
	{0x3030, 0x07},  	//old shut mode  

	//--> This register are for user.
	//+++++++++++++++++++++++++++++++//
	// Integration setting ... 
	{0x0200, 0x03},      //fine integration time, WARNING: User should use fixed fine integration time.
	{0x0201, 0x5C},  
	{0x0202, 0x07},      // coarse integration time, Max Coarse integration time = frame lenth line -5   
	{0x0203, 0xAA},      // = 7B4-5
	{0x0204, 0x00},      // analog gain
	{0x0205, 0x40},  
	{0x0340, 0x07},     //Frame Length, Min frame lenth = Vsize + 12
	{0x0341, 0xB4},     //1960 + 12 = 1972 dec = 7B4h
	                    //07B4 = 30fps,  0F6C = 15fps, 2E44 = 5fps
	{0x0342, 0x0B},      // Line Length, MIPI Non-Continuous mode : B30h �̻�, Continous mode :AB2h �̻�.] 
	{0x0343, 0x30},      // B30h �̻��̸� ��� �̻����.

	// MIPI Size Setting
	{0x30A9, 0x03},  //Horizontal Binning Off
	{0x300E, 0x28},  //E8 Vertical Binning Off
	{0x302B, 0x01},  	// [0]blst_en, [1]sl_off
	{0x3029, 0x34},  	//04 RST_MX(ramp)(384) + ring oscillator
	{0x0380, 0x00},  //x_even_inc 1
	{0x0381, 0x01},  
	{0x0382, 0x00},  //x_odd_inc 1
	{0x0383, 0x01},  
	{0x0384, 0x00},  //y_even_inc 1
	{0x0385, 0x01},  
	{0x0386, 0x00},  //y_odd_inc 3
	{0x0387, 0x01},  
	{0x0344, 0x00},  //x_addr_start
	{0x0345, 0x00},  
	{0x0346, 0x00},  //y_addr_start
	{0x0347, 0x00},  
	{0x0348, 0x0A},  //x_addr_end
	{0x0349, 0x2F},  
	{0x034A, 0x07},  //y_addr_end
	{0x034B, 0xA7},  
	{0x034C, 0x0A},  //x_output_size
	{0x034D, 0x30},  
	{0x034E, 0x07},  //y_output_size
	{0x034F, 0xA8},  
	//{0x0100, 0x01},  
#endif
};

static struct v4l2_subdev_info s5k4e5ya_subdev_info[] = {
	{
	.code   = V4L2_MBUS_FMT_SBGGR10_1X10,
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt    = 1,
	.order    = 0,
	},
	/* more can be supported, to be added later */
};

static struct msm_camera_i2c_conf_array s5k4e5ya_init_conf[] = {
	{&s5k4e5ya_recommend_settings[0],
	ARRAY_SIZE(s5k4e5ya_recommend_settings), 0, MSM_CAMERA_I2C_BYTE_DATA}
};

static struct msm_camera_i2c_conf_array s5k4e5ya_confs[] = {
	{&s5k4e5ya_snap_settings[0],
	ARRAY_SIZE(s5k4e5ya_snap_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
/*                                                                                               */
#if 0
	{&s5k4e5ya_prev_settings[0],
	ARRAY_SIZE(s5k4e5ya_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
#else
	{&s5k4e5ya_snap_settings[0],
	ARRAY_SIZE(s5k4e5ya_snap_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
#endif	
/*                                                                                               */
	{&s5k4e5ya_video_settings[0],
	ARRAY_SIZE(s5k4e5ya_video_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};


//                                                                                  
static struct msm_sensor_output_info_t s5k4e5ya_dimensions[] = {
	/*                                                                 */
	{
		.x_output = 0x0A30,
		.y_output = 0x07A0,
		.line_length_pclk = 0x0B30,
		.frame_length_lines = 0x7D5, //                                                             
		.vt_pixel_clk = 171200000,	//856Mhz/5=171.2Mhz		
		.op_pixel_clk = 266667000,	//848Mhz/5=169.6Mhz
		.binning_factor = 1,
	},
/*                                                                                               */
#if 0	
	/*                                                                             */
	{
		.x_output = 0x0500,		/*                                                                                              */
		.y_output = 0x03C0, 	/*                                                                                              */
		.line_length_pclk = 0x0C6F, 	/*                                                                                              */
		.frame_length_lines = 0x725, //                                                            
		.vt_pixel_clk = 169600000,	//848Mhz/5=169.6Mhz
		.op_pixel_clk = 169600000,	//848Mhz/5=169.6Mhz
		.binning_factor = 1,
	},
#else 
	{
		.x_output = 0x0A30,
		.y_output = 0x07A0,
		.line_length_pclk = 0x0B30,
		.frame_length_lines = 0x7D5, //                                                             
		.vt_pixel_clk = 171200000,	//856Mhz/5=171.2Mhz		
		.op_pixel_clk = 266667000,	//848Mhz/5=169.6Mhz
		.binning_factor = 1,
	},
#endif
/*                                                                                               */
	/*                                                                 */
	{
		.x_output = 0x0A00,
		.y_output = 0x05A0,
		.line_length_pclk = 0x0B30,
		.frame_length_lines = 0x7D5, //                                                             
		.vt_pixel_clk = 171200000,	//856Mhz/5=171.2Mhz		
		.op_pixel_clk = 266667000,	//848Mhz/5=169.6Mhz
		.binning_factor = 1,
	},
};

//                                                                                  
static struct msm_sensor_output_reg_addr_t s5k4e5ya_reg_addr = {
	.x_output = 0x034C,
	.y_output = 0x034E,
	.line_length_pclk = 0x0342,
	.frame_length_lines = 0x0340,
};

//                                            
static struct msm_sensor_id_info_t s5k4e5ya_id_info = {
	.sensor_id_reg_addr = 0x0000,
	.sensor_id = 0x4E50,
};

static struct msm_sensor_exp_gain_info_t s5k4e5ya_exp_gain_info = {
	.coarse_int_time_addr = 0x0202,
	.global_gain_addr = 0x0204,
	.vert_offset = 4,
};

static enum msm_camera_vreg_name_t s5k4e5ya_veg_seq[] = {
	CAM_VDIG,
	CAM_VIO,
	CAM_VANA,
//	CAM_VAF,
};

static inline uint8_t s5k4e5ya_byte(uint16_t word, uint8_t offset)
{
	return word >> (offset * BITS_PER_BYTE);
}

static int32_t s5k4e5ya_sensor_write_exp_gain1(struct msm_sensor_ctrl_t *s_ctrl,
						uint16_t gain, uint32_t line, int32_t luma_avg, uint16_t fgain)
{
	uint32_t fl_lines;
	uint8_t offset;
	fl_lines = s_ctrl->curr_frame_length_lines;
	fl_lines = (fl_lines * s_ctrl->fps_divider) / Q10;
	offset = s_ctrl->sensor_exp_gain_info->vert_offset;
	if (line > (fl_lines - offset))
		fl_lines = line + offset;

		s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_output_reg_addr->frame_length_lines, s5k4e5ya_byte(fl_lines, 1),
			MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_output_reg_addr->frame_length_lines + 1, s5k4e5ya_byte(fl_lines, 0),
			MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->coarse_int_time_addr, s5k4e5ya_byte(line, 1),
			MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->coarse_int_time_addr + 1, s5k4e5ya_byte(line, 0),
			MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->global_gain_addr, s5k4e5ya_byte(gain, 1),
			MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->global_gain_addr + 1, s5k4e5ya_byte(gain, 0),
			MSM_CAMERA_I2C_BYTE_DATA);
		s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
	return 0;
}

int32_t s5k4e5ya_sensor_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rc = 0;
	struct msm_camera_sensor_info *s_info;

	pr_err("s5k4e5ya_sensor_i2c_probe: enter");

	rc = msm_sensor_i2c_probe(client, id);

	s_info = client->dev.platform_data;
	if (s_info == NULL) {
		pr_err("%s %s NULL sensor data\n", __func__, client->name);
		return -EFAULT;
	}

	if (s_info->actuator_info->vcm_enable) {
		rc = gpio_request(s_info->actuator_info->vcm_pwd,
				"msm_actuator");
		if (rc < 0)
			pr_err("%s: gpio_request:msm_actuator %d failed\n",
				__func__, s_info->actuator_info->vcm_pwd);
		rc = gpio_direction_output(s_info->actuator_info->vcm_pwd, 0);
		if (rc < 0)
			pr_err("%s: gpio:msm_actuator %d direction can't be set\n",
				__func__, s_info->actuator_info->vcm_pwd);
		gpio_free(s_info->actuator_info->vcm_pwd);
	}

	return rc;
}

static const struct i2c_device_id s5k4e5ya_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&s5k4e5ya_s_ctrl},
	{ }
};

static struct i2c_driver s5k4e5ya_i2c_driver = {
	.id_table = s5k4e5ya_i2c_id,
	.probe  = s5k4e5ya_sensor_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client s5k4e5ya_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

static int __init msm_sensor_init_module(void)
{
	pr_err("s5k4e5ya module init !!!");
	return i2c_add_driver(&s5k4e5ya_i2c_driver);
}

static struct v4l2_subdev_core_ops s5k4e5ya_subdev_core_ops = {
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};

static struct v4l2_subdev_video_ops s5k4e5ya_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops s5k4e5ya_subdev_ops = {
	.core = &s5k4e5ya_subdev_core_ops,
	.video  = &s5k4e5ya_subdev_video_ops,
};

static struct msm_sensor_fn_t s5k4e5ya_func_tbl = {
	.sensor_start_stream = msm_sensor_start_stream,
	.sensor_stop_stream = msm_sensor_stop_stream,
	.sensor_group_hold_on = msm_sensor_group_hold_on,
	.sensor_group_hold_off = msm_sensor_group_hold_off,
	.sensor_set_fps = msm_sensor_set_fps,
	.sensor_write_exp_gain = s5k4e5ya_sensor_write_exp_gain1,
	.sensor_write_snapshot_exp_gain = s5k4e5ya_sensor_write_exp_gain1,
	.sensor_setting = msm_sensor_setting,
	.sensor_csi_setting = msm_sensor_setting1,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = msm_sensor_config,
	.sensor_power_up = msm_sensor_power_up,
	.sensor_power_down = msm_sensor_power_down,
	.sensor_adjust_frame_lines = msm_sensor_adjust_frame_lines1,
	.sensor_get_csi_params = msm_sensor_get_csi_params,
};

static struct msm_sensor_reg_t s5k4e5ya_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf = s5k4e5ya_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(s5k4e5ya_start_settings),
	.stop_stream_conf = s5k4e5ya_stop_settings,
	.stop_stream_conf_size = ARRAY_SIZE(s5k4e5ya_stop_settings),
	.group_hold_on_conf = s5k4e5ya_groupon_settings,
	.group_hold_on_conf_size = ARRAY_SIZE(s5k4e5ya_groupon_settings),
	.group_hold_off_conf = s5k4e5ya_groupoff_settings,
	.group_hold_off_conf_size =
		ARRAY_SIZE(s5k4e5ya_groupoff_settings),
	.init_settings = &s5k4e5ya_init_conf[0],
	.init_size = ARRAY_SIZE(s5k4e5ya_init_conf),
	.mode_settings = &s5k4e5ya_confs[0],
	.output_settings = &s5k4e5ya_dimensions[0],
	.num_conf = ARRAY_SIZE(s5k4e5ya_confs),
};

static struct msm_sensor_ctrl_t s5k4e5ya_s_ctrl = {
	.msm_sensor_reg = &s5k4e5ya_regs,
	.sensor_i2c_client = &s5k4e5ya_sensor_i2c_client,
	.sensor_i2c_addr = 0x20,
	.vreg_seq = s5k4e5ya_veg_seq,
	.num_vreg_seq = ARRAY_SIZE(s5k4e5ya_veg_seq),
	.sensor_output_reg_addr = &s5k4e5ya_reg_addr,
	.sensor_id_info = &s5k4e5ya_id_info,
	.sensor_exp_gain_info = &s5k4e5ya_exp_gain_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.msm_sensor_mutex = &s5k4e5ya_mut,
	.sensor_i2c_driver = &s5k4e5ya_i2c_driver,
	.sensor_v4l2_subdev_info = s5k4e5ya_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(s5k4e5ya_subdev_info),
	.sensor_v4l2_subdev_ops = &s5k4e5ya_subdev_ops,
	.func_tbl = &s5k4e5ya_func_tbl,
	.clk_rate = MSM_SENSOR_MCLK_24HZ,
};

module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("Samsung 5MP Bayer sensor driver");
MODULE_LICENSE("GPL v2");
