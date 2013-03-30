/*****************************************************************************
 Copyright(c) 2012 FCI Inc. All Rights Reserved
 
 File name : fc8150_tun.c (BGA & QFN)
 
 Description : fc8150 tuner driver
 
 History : 
 ----------------------------------------------------------------------
 2012/01/20 	initial 0.1 version
 2012/01/25 	initial 0.3 version
 2012/01/27 	initial 0.5 version
 2012/01/31 	initial 1.0 version
 2012/01/31 	initial 1.1 version
 2012/02/06		initial 1.2 version
 2012/02/09		initial 1.3 Version		
 2012/02/15		initial 1.4 Version
 2012/02/15		initial 2.0 Version
 2012/02/24		initial 2.01 Version
 2012/03/30		initial 3.0 Version
 2012/06/07	pre SLR Version
 2012/06/11     pre SLR Version
 2012/06/15	
 2012/06/17	SLR 0.3 version
 2012/06/19	SLR 0.4 version
 2012/06/20
 2012/07/04
 2012/07/09
 2012/07/10
 2012/07/15
*******************************************************************************/
#include <linux/init.h> /* miseon.kim 2012.06.21 FC8150 Device Release */
#include <mach/board_lge.h> /* miseon.kim 2012.06.21 FC8150 Device Release */

#include "fci_types.h"
#include "fci_oal.h"
#include "fci_tun.h"
#include "fc8150_regs.h"
#include "fci_hal.h"

#define  FC8150_FREQ_XTAL  BBM_XTAL_FREQ  //32MHZ

static int fc8150_write(HANDLE hDevice, u8 addr, u8 data)
{
	int res;
	u8 tmp;

	tmp = data;
	res = tuner_i2c_write(hDevice, addr, 1,&tmp, 1);

	return res;
}

static int fc8150_read(HANDLE hDevice, u8 addr, u8 *data)
{
	int res;

	res = tuner_i2c_read(hDevice, addr, 1,data, 1);

	return res;
}

static int fc8150_bb_read(HANDLE hDevice, u16 addr, u8 *data)
{
	int res;

	res = bbm_read(hDevice, addr, data);

	return res;
}

#if 0
static int fc8150_bb_write(HANDLE hDevice, u16 addr, u8 data)
{
	int res;

	res = bbm_write(hDevice, addr, data);

	return res;
}
#endif

static int fc8150_set_filter(HANDLE hDevice)
{
	int i;
	u8 cal_mon = 0;

#if (FC8150_FREQ_XTAL == 16000)
	fc8150_write(hDevice, 0x3B, 0x01);
	fc8150_write(hDevice, 0x3D, 0x20);
	fc8150_write(hDevice, 0x3B, 0x00);
#elif (FC8150_FREQ_XTAL == 16384)
	fc8150_write(hDevice, 0x3B, 0x01);
	fc8150_write(hDevice, 0x3D, 0x21);
	fc8150_write(hDevice, 0x3B, 0x00);
#elif (FC8150_FREQ_XTAL == 18000)
	fc8150_write(hDevice, 0x3B, 0x01);
	fc8150_write(hDevice, 0x3D, 0x24);
	fc8150_write(hDevice, 0x3B, 0x00);
#elif (FC8150_FREQ_XTAL == 19200)
	fc8150_write(hDevice, 0x3B, 0x01);
	fc8150_write(hDevice, 0x3D, 0x26);
	fc8150_write(hDevice, 0x3B, 0x00);	
#elif (FC8150_FREQ_XTAL == 24000)
	fc8150_write(hDevice, 0x3B, 0x01);
	fc8150_write(hDevice, 0x3D, 0x30);
	fc8150_write(hDevice, 0x3B, 0x00);
#elif (FC8150_FREQ_XTAL == 24576)
	fc8150_write(hDevice, 0x3B, 0x01);
	fc8150_write(hDevice, 0x3D, 0x31);
	fc8150_write(hDevice, 0x3B, 0x00);
#elif (FC8150_FREQ_XTAL == 26000)
	fc8150_write(hDevice, 0x3B, 0x01);
	fc8150_write(hDevice, 0x3D, 0x34);
	fc8150_write(hDevice, 0x3B, 0x00);
#elif (FC8150_FREQ_XTAL == 27000)
	fc8150_write(hDevice, 0x3B, 0x01);
	fc8150_write(hDevice, 0x3D, 0x36);
	fc8150_write(hDevice, 0x3B, 0x00);	
#elif (FC8150_FREQ_XTAL == 27120)
	fc8150_write(hDevice, 0x3B, 0x01);
	fc8150_write(hDevice, 0x3D, 0x36);
	fc8150_write(hDevice, 0x3B, 0x00);	
#elif (FC8150_FREQ_XTAL == 32000)
	fc8150_write(hDevice, 0x3B, 0x01);
	fc8150_write(hDevice, 0x3D, 0x40);
	fc8150_write(hDevice, 0x3B, 0x00);
#elif (FC8150_FREQ_XTAL == 37400)
	fc8150_write(hDevice, 0x3B, 0x01);
	fc8150_write(hDevice, 0x3D, 0x4B);
	fc8150_write(hDevice, 0x3B, 0x00);
#elif (FC8150_FREQ_XTAL == 38400)
	fc8150_write(hDevice, 0x3B, 0x01);
	fc8150_write(hDevice, 0x3D, 0x4D);
	fc8150_write(hDevice, 0x3B, 0x00);
#else
	return BBM_NOK;
#endif

	for(i=0; i<10; i++) {
		msWait(5);
		fc8150_read(hDevice, 0x33, &cal_mon);
		if( (cal_mon & 0xC0) == 0xC0) 
			break;
		fc8150_write(hDevice, 0x32, 0x01);
		fc8150_write(hDevice, 0x32, 0x09);
	}

	fc8150_write(hDevice, 0x32, 0x01);

	return BBM_OK;
}

int fc8150_tuner_init(HANDLE hDevice, u32 band)
{
	PRINTF(hDevice, "fc8150_init\n");
    /* miseon.kim 2012.06.21 FC8150 Device Release */
    if(lge_get_board_revno() >= HW_REV_C)
    {
	 	int i;
		int n_RFAGC_PD1_AVG,n_RFAGC_PD2_AVG;
		u8  RFPD_REF;
		u8  RFAGC_PD2[6],RFAGC_PD2_AVG,RFAGC_PD2_MAX,RFAGC_PD2_MIN;
		u8  RFAGC_PD1[6],RFAGC_PD1_AVG,RFAGC_PD1_MAX,RFAGC_PD1_MIN;
		
		int res = BBM_OK;
				
		fc8150_write(hDevice, 0x00, 0x00);
		fc8150_write(hDevice, 0x02, 0x81);
	
		fc8150_write(hDevice, 0x15, 0x02);  
		fc8150_write(hDevice, 0x20, 0x33);
		fc8150_write(hDevice, 0x28, 0x62);  
		fc8150_write(hDevice, 0x35, 0xAA);  
		fc8150_write(hDevice, 0x38, 0x28);  
	
		fc8150_write(hDevice, 0x3B, 0x01);
		
		fc8150_set_filter(hDevice);
		
		fc8150_write(hDevice, 0x3B, 0x00);
	
		fc8150_write(hDevice, 0x56, 0x01);
		fc8150_write(hDevice, 0x57, 0x86);
		fc8150_write(hDevice, 0x58, 0xA7);
		fc8150_write(hDevice, 0x59, 0x4D);
	
		fc8150_write(hDevice, 0x80, 0x17);
		fc8150_write(hDevice, 0xAB, 0x48);
	
		fc8150_write(hDevice, 0xA0, 0xC0);
		fc8150_write(hDevice, 0xD0, 0x00);
	
		fc8150_write(hDevice, 0xA5, 0x65); 
		
		RFAGC_PD1[0]	=	0;
		RFAGC_PD1[1]	=	0;
		RFAGC_PD1[2]	=	0;
		RFAGC_PD1[3]	=	0;
		RFAGC_PD1[4]	=	0;
		RFAGC_PD1[5]	=	0;
		RFAGC_PD1_MAX	=	0;
		RFAGC_PD1_MIN	=	255;
		
		for (i = 0 ; i<6 ; i++){
			fc8150_read(hDevice, 0xD8 , &RFAGC_PD1[i] );
			
			if( RFAGC_PD1[i] >= RFAGC_PD1_MAX)	RFAGC_PD1_MAX = RFAGC_PD1[i];
			if( RFAGC_PD1[i] <= RFAGC_PD1_MIN)	RFAGC_PD1_MIN = RFAGC_PD1[i];
		}
		n_RFAGC_PD1_AVG	= (RFAGC_PD1[0] + RFAGC_PD1[1] + RFAGC_PD1[2] + RFAGC_PD1[3] + RFAGC_PD1[4] + RFAGC_PD1[5] - RFAGC_PD1_MAX - RFAGC_PD1_MIN) /4;
		RFAGC_PD1_AVG =	(unsigned char) n_RFAGC_PD1_AVG;
		
		fc8150_write(hDevice, 0x7F , RFAGC_PD1_AVG);	
	
		RFAGC_PD2[0]	=	0;
		RFAGC_PD2[1]	=	0;
		RFAGC_PD2[2]	=	0;
		RFAGC_PD2[3]	=	0;
		RFAGC_PD2[4]	=	0;
		RFAGC_PD2[5]	=	0;
		
		RFAGC_PD2_MAX	=	0;
		RFAGC_PD2_MIN	=	255;
		
		for (i = 0 ; i<6 ; i++){
			fc8150_read(hDevice, 0xD6 , &RFAGC_PD2[i] );
			
			if( RFAGC_PD2[i] >= RFAGC_PD2_MAX)	RFAGC_PD2_MAX = RFAGC_PD2[i];
			if( RFAGC_PD2[i] <= RFAGC_PD2_MIN)	RFAGC_PD2_MIN = RFAGC_PD2[i];
		}
		n_RFAGC_PD2_AVG	= (RFAGC_PD2[0] + RFAGC_PD2[1] + RFAGC_PD2[2] + RFAGC_PD2[3] + RFAGC_PD2[4] + RFAGC_PD2[5] - RFAGC_PD2_MAX - RFAGC_PD2_MIN) /4;
		RFAGC_PD2_AVG =	(unsigned char) n_RFAGC_PD2_AVG;
		
		fc8150_write(hDevice, 0x7E , RFAGC_PD2_AVG);	
	
		res = fc8150_read(hDevice, 0xD6, &RFPD_REF);
		
		if(0x86<=RFPD_REF)	fc8150_write(hDevice, 0x7B, 0x8F);
		else if (RFPD_REF<0x86)	fc8150_write(hDevice, 0x7B, 0x88);
		
		fc8150_write(hDevice, 0x79, 0x32);
		fc8150_write(hDevice, 0x7A, 0x2C);
		fc8150_write(hDevice, 0x7C, 0x10);
		fc8150_write(hDevice, 0x7D, 0x0C);
		fc8150_write(hDevice, 0x81, 0x0A);
		fc8150_write(hDevice, 0x84, 0x00);
		
		fc8150_write(hDevice, 0x02, 0x81);
    }
    else
    {
    	u8  RFPD_REF, MIXPD_REF;

    	fc8150_write(hDevice, 0x00, 0x00);
    	fc8150_write(hDevice, 0x02, 0x81);
    	
    	fc8150_write(hDevice, 0x13, 0xF4);
    	fc8150_write(hDevice, 0x30, 0x0A);
    	fc8150_write(hDevice, 0x3B, 0x01);
    	
    	fc8150_set_filter(hDevice);
    	
    	fc8150_write(hDevice, 0x3B, 0x00);
    	
    	fc8150_write(hDevice, 0x34, 0x68);
    	fc8150_write(hDevice, 0x36, 0xFF);
    	fc8150_write(hDevice, 0x37, 0xFF);
    	fc8150_write(hDevice, 0x39, 0x11);
    	fc8150_write(hDevice, 0x3A, 0x00);
    	
    	fc8150_write(hDevice, 0x52, 0x20);
    	fc8150_write(hDevice, 0x53, 0x5F);
    	fc8150_write(hDevice, 0x54, 0x00);
    	fc8150_write(hDevice, 0x5E, 0x00);
    	fc8150_write(hDevice, 0x63, 0x30);
    	
    	
    	fc8150_write(hDevice, 0x56, 0x0F);
    	fc8150_write(hDevice, 0x57, 0x1F);
    	fc8150_write(hDevice, 0x58, 0x09);
    	fc8150_write(hDevice, 0x59, 0x5E);
    	
    	fc8150_write(hDevice, 0x29, 0x00);
    	
    	fc8150_write(hDevice, 0x94, 0x00);
    	fc8150_write(hDevice, 0x95, 0x01);
    	fc8150_write(hDevice, 0x96, 0x11);
    	fc8150_write(hDevice, 0x97, 0x21);
    	fc8150_write(hDevice, 0x98, 0x31);
    	fc8150_write(hDevice, 0x99, 0x32);
    	fc8150_write(hDevice, 0x9A, 0x42);
    	fc8150_write(hDevice, 0x9B, 0x52);
    	fc8150_write(hDevice, 0x9C, 0x53);
    	fc8150_write(hDevice, 0x9D, 0x63);
    	fc8150_write(hDevice, 0x9E, 0x63);
    	fc8150_write(hDevice, 0x9F, 0x63);
    	
    	fc8150_write(hDevice, 0x79, 0x2A);
    	fc8150_write(hDevice, 0x7A, 0x24);
    	fc8150_write(hDevice, 0x7B, 0xFF);
    	fc8150_write(hDevice, 0x7C, 0x1B);
    	fc8150_write(hDevice, 0x7D, 0x17);
    	fc8150_write(hDevice, 0x84, 0x00);
    	fc8150_write(hDevice, 0x85, 0x08);
    	fc8150_write(hDevice, 0x86, 0x00);
    	fc8150_write(hDevice, 0x87, 0x08);
    	fc8150_write(hDevice, 0x88, 0x00);
    	fc8150_write(hDevice, 0x89, 0x08);
    	fc8150_write(hDevice, 0x8A, 0x00);
    	fc8150_write(hDevice, 0x8B, 0x08);
    	fc8150_write(hDevice, 0x8C, 0x00);
    	fc8150_write(hDevice, 0x8D, 0x1D);
    	fc8150_write(hDevice, 0x8E, 0x13);
    	fc8150_write(hDevice, 0x8F, 0x1D);
    	fc8150_write(hDevice, 0x90, 0x13);
    	fc8150_write(hDevice, 0x91, 0x1D);
    	fc8150_write(hDevice, 0x92, 0x13);
    	fc8150_write(hDevice, 0x93, 0x1D);
    	fc8150_write(hDevice, 0x80, 0x1F);
    	fc8150_write(hDevice, 0x81, 0x0A);
    	fc8150_write(hDevice, 0x82, 0x40);
    	fc8150_write(hDevice, 0x83, 0x0A);

    	fc8150_write(hDevice, 0xA0, 0xC0);
    	fc8150_write(hDevice, 0x7E, 0x7F);
    	fc8150_write(hDevice, 0x7F, 0x7F);
    	fc8150_write(hDevice, 0xD0, 0x0A);
    	fc8150_write(hDevice, 0xD2, 0x28);
    	fc8150_write(hDevice, 0xD4, 0x28);

    	fc8150_write(hDevice, 0xA0, 0x17);
    	fc8150_write(hDevice, 0xD0, 0x00);
    	fc8150_write(hDevice, 0xA1, 0x1D);
    	
    	msWait(100);
    	
    	fc8150_read(hDevice, 0xD6, &RFPD_REF);
    	fc8150_read(hDevice, 0xD8, &MIXPD_REF);

    	fc8150_write(hDevice, 0xA0, 0xD7);
    	fc8150_write(hDevice, 0xD0, 0x0A);

    	fc8150_write(hDevice, 0x7E, RFPD_REF);
    	fc8150_write(hDevice, 0x7F, MIXPD_REF);

    	fc8150_write(hDevice, 0xA0, 0xC0);
    	fc8150_write(hDevice, 0xA1, 0x00);
    }
	return BBM_OK;
}


int fc8150_set_freq(HANDLE hDevice, band_type band, u32 rf_kHz)
{
	int n_captune = 0;
	unsigned long f_diff, f_diff_shifted, n_val, k_val;
	unsigned long f_vco, f_comp;
	unsigned char r_val, data_0x56;
	unsigned char pre_shift_bits = 4;
	
	f_vco = (rf_kHz) << 2;
	if(f_vco < FC8150_FREQ_XTAL*40)
		r_val = 2;
	else
		r_val = 1;

	f_comp = FC8150_FREQ_XTAL / r_val;

	n_val =	f_vco / f_comp;
	f_diff = f_vco - f_comp * n_val;

	f_diff_shifted = f_diff << (20 - pre_shift_bits);

    /* miseon.kim 2012.06.21 FC8150 Device Release */
    if(lge_get_board_revno() >= HW_REV_C)
    {
    	k_val = (f_diff_shifted ) / (f_comp >> pre_shift_bits);
		k_val = (k_val | 1);
	
		if(470000<rf_kHz && rf_kHz<=473143){
			fc8150_write(hDevice, 0x1E, 0x04);
			fc8150_write(hDevice, 0x1F, 0x36);
			fc8150_write(hDevice, 0x14, 0x84);
		} else if(473143<rf_kHz && rf_kHz<=485143){
			fc8150_write(hDevice, 0x1E, 0x03);
			fc8150_write(hDevice, 0x1F, 0x3E);
			fc8150_write(hDevice, 0x14, 0x84);
		} else if(485143<rf_kHz && rf_kHz<=551143){
			fc8150_write(hDevice, 0x1E, 0x04);
			fc8150_write(hDevice, 0x1F, 0x36);
			fc8150_write(hDevice, 0x14, 0x84);
		} else if(551143<rf_kHz && rf_kHz<=563143){
			fc8150_write(hDevice, 0x1E, 0x03);
			fc8150_write(hDevice, 0x1F, 0x3E);
			fc8150_write(hDevice, 0x14, 0xC4);
		} else if(563143<rf_kHz && rf_kHz<=593143){
			fc8150_write(hDevice, 0x1E, 0x02);
			fc8150_write(hDevice, 0x1F, 0x3E);
			fc8150_write(hDevice, 0x14, 0xC4);
		} else if(593143<rf_kHz && rf_kHz<=659143){  
			fc8150_write(hDevice, 0x1E, 0x02);
			fc8150_write(hDevice, 0x1F, 0x36);
			fc8150_write(hDevice, 0x14, 0x84);
		} else if(659143<rf_kHz && rf_kHz<=767143){  
			fc8150_write(hDevice, 0x1E, 0x01);
			fc8150_write(hDevice, 0x1F, 0x36);
			fc8150_write(hDevice, 0x14, 0x84);
		} else if(767143<rf_kHz){
			fc8150_write(hDevice, 0x1E, 0x00);
			fc8150_write(hDevice, 0x1F, 0x36);
			fc8150_write(hDevice, 0x14, 0x84);
		} else{
			fc8150_write(hDevice, 0x1E, 0x05);
			fc8150_write(hDevice, 0x1F, 0x36);
			fc8150_write(hDevice, 0x14, 0x84);
		}
	
		data_0x56 = ((r_val==1)? 0 : 0x10) + (unsigned char)(k_val>>16);
		fc8150_write(hDevice, 0x56, data_0x56);
		fc8150_write(hDevice, 0x57, (unsigned char)((k_val>>8)&0xFF));
		fc8150_write(hDevice, 0x58, (unsigned char)(((k_val)&0xFF)));
		fc8150_write(hDevice, 0x59, (unsigned char) n_val);
		
		if(rf_kHz<525000){
			fc8150_write(hDevice, 0x55, 0x0E);
		} else if (525000<=rf_kHz && rf_kHz<600000){
			fc8150_write(hDevice, 0x55, 0x0C);
		} else if (600000<=rf_kHz && rf_kHz<700000){
			fc8150_write(hDevice, 0x55, 0x08);
		} else if (700000<rf_kHz){
			fc8150_write(hDevice, 0x55, 0x06);
		}
		
		if(rf_kHz<=491143){
			fc8150_write(hDevice, 0x79, 0x28);
			fc8150_write(hDevice, 0x7A, 0x24);
		} else if (491143<rf_kHz && rf_kHz<=659143){
			fc8150_write(hDevice, 0x79, 0x2A);
			fc8150_write(hDevice, 0x7A, 0x26);
		} else if (659143<rf_kHz && rf_kHz<=773143){
			fc8150_write(hDevice, 0x79, 0x2C);
			fc8150_write(hDevice, 0x7A, 0x28);
		} else if(773143<rf_kHz){
			fc8150_write(hDevice, 0x79, 0x2F);
			fc8150_write(hDevice, 0x7A, 0x2B);
		}
		
		if(rf_kHz<=707143){
			fc8150_write(hDevice, 0x54, 0x00);
			fc8150_write(hDevice, 0x53, 0x5F);
		} else if (707143<rf_kHz){
			fc8150_write(hDevice, 0x54, 0x04);
			fc8150_write(hDevice, 0x53, 0x9F);
		}
    }
    else
    {
    	k_val = (f_diff_shifted + (f_comp >> (pre_shift_bits+1))) / (f_comp >> pre_shift_bits);
    	k_val = (k_val | 1);
    	
    	
    	if(470000<rf_kHz && rf_kHz<=505000){
    		n_captune	=	4;
    	
    		
    	}
    	else if(505000<rf_kHz && rf_kHz<=545000){
    		n_captune	=	3;
    	
    		
    	}
    	else if(545000<rf_kHz && rf_kHz<=610000){
    		n_captune	=	2;
    	
    		
    	}
    	else if(610000<rf_kHz && rf_kHz<=695000){
    		n_captune	=	1;
    	
    		
    	}
    	else if(695000<rf_kHz){
    		n_captune	=	0;
    		
    	}	
    	fc8150_write(hDevice, 0x1E, (unsigned char)n_captune);
    	
    	

    	data_0x56 = ((r_val==1)? 0 : 0x10) + (unsigned char)(k_val>>16);
    	fc8150_write(hDevice, 0x56, data_0x56);
    	fc8150_write(hDevice, 0x57, (unsigned char)((k_val>>8)&0xFF));
    	fc8150_write(hDevice, 0x58, (unsigned char)(((k_val)&0xFF)));
    	fc8150_write(hDevice, 0x59, (unsigned char) n_val);
    	
    	if(rf_kHz<=600000){
    		fc8150_write(hDevice, 0x55, 0x07);
    	}
    	else {
    		fc8150_write(hDevice, 0x55, 0x05);
    	}

    	if((490000<rf_kHz)&& (560000>=rf_kHz)){
    		fc8150_write(hDevice, 0x1F, 0x0E);
    	}
    	else {
    		fc8150_write(hDevice, 0x1F, 0x06);
    	}
    }
	return BBM_OK;
}

int fc8150_get_rssi(HANDLE hDevice, int *rssi)
{
	int res = BBM_OK;
	u8  LNA, RFVGA, CSF,PREAMP_PGA = 0x00;
	int K = -101;
	int PGA = 0;

	res |= fc8150_read(hDevice, 0xA3, &LNA);
	res |= fc8150_read(hDevice, 0xA4, &RFVGA);
	res |= fc8150_read(hDevice, 0xA8, &CSF);
	res |= fc8150_bb_read(hDevice, 0x106E, &PREAMP_PGA);

	if(res != BBM_OK)
		return res;

	if (127<PREAMP_PGA) 		PGA = -1*((256-PREAMP_PGA)+1);
	else if(PREAMP_PGA<=127) 	PGA = PREAMP_PGA;

	//*rssi = (LNA & 0x07) * 6 + (RFVGA & 0x1F) + ((CSF & 0x03)+((CSF & 0x70) >> 4) )*6 - PGA * 0.25f + K ;
	*rssi = (LNA & 0x07) * 6 + (RFVGA & 0x1F) + ((CSF & 0x03)+((CSF & 0x70) >> 4) )*6 - PGA / 4 + K;

	return BBM_OK;
}

int fc8150_tuner_deinit(HANDLE hDevice)
{
	return BBM_OK;
}

