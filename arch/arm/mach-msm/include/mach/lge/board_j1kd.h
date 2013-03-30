/* arch/arm/mach-msm/include/mach/board_j1kd.h
 *
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2008-2012, Code Aurora Forum. All rights reserved.
 * Author: Brian Swetland <swetland@google.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __ASM_ARCH_MSM_BOARD_J1KD_H
#define __ASM_ARCH_MSM_BOARD_J1KD_H

#ifdef CONFIG_LGE_PM
#define ADC_CHANGE_REV	HW_REV_EVB1
#define IBAT_CURRENT		825

/* Ref resistance value = 665K */
#define ADC_NO_INIT_CABLE   0
#define ADC_CABLE_MHL_1K    5000
#define ADC_CABLE_U_28P7K   5500/* This value is obsolete */
#define ADC_CABLE_28P7K     6000/* min value of 56K is so low because of factory cable issue */
#define ADC_CABLE_56K       175000
#define ADC_CABLE_100K      260000
#define ADC_CABLE_130K      335000
#define ADC_CABLE_180K      395000
#define ADC_CABLE_200K	    425000
#define ADC_CABLE_220K      475000
#define ADC_CABLE_270K      550000
#define ADC_CABLE_330K      725000
#define ADC_CABLE_620K      945000
#define ADC_CABLE_910K      1130000
#define ADC_CABLE_NONE      1800000

/* Ref resistance value = 200K */
#define ADC_NO_INIT_CABLE2   0
#define ADC_CABLE_MHL_1K2    50000
#define ADC_CABLE_U_28P7K2   200000
#define ADC_CABLE_28P7K2     300000
#define ADC_CABLE_56K2       490000
#define ADC_CABLE_100K2      650000
#define ADC_CABLE_130K2      780000
#define ADC_CABLE_180K2      875000
#define ADC_CABLE_200K2      920000
#define ADC_CABLE_220K2      988000
#define ADC_CABLE_270K2      1077000
#define ADC_CABLE_330K2      1294000
#define ADC_CABLE_620K2      1418000
#define ADC_CABLE_910K2      1600000
#define ADC_CABLE_NONE2      1800000

#define C_NO_INIT_TA_MA     0
#define C_MHL_1K_TA_MA      500
#define C_U_28P7K_TA_MA     500
#define C_28P7K_TA_MA       500
#define C_56K_TA_MA         1500 /* it will be changed in future */
#define C_100K_TA_MA        500
#define C_130K_TA_MA        1500
#define C_180K_TA_MA        700
#define C_200K_TA_MA        700
#define C_220K_TA_MA        900
#define C_270K_TA_MA        800
#define C_330K_TA_MA        500
#define C_620K_TA_MA        500
#define C_910K_TA_MA        1500//[ORG]500
#define C_NONE_TA_MA        500 // DCM/KDDI Adaptive current 500mA to 1500mA

#define C_NO_INIT_USB_MA    0
#define C_MHL_1K_USB_MA     500
#define C_U_28P7K_USB_MA    500
#define C_28P7K_USB_MA      500
#define C_56K_USB_MA        1500 /* it will be changed in future */
#define C_100K_USB_MA       500
#define C_130K_USB_MA       1500
#define C_180K_USB_MA       500
#define C_200K_USB_MA       500
#define C_220K_USB_MA       500
#define C_270K_USB_MA       500
#define C_330K_USB_MA       500
#define C_620K_USB_MA       500
#define C_910K_USB_MA       1500//[ORG]500
#define C_NONE_USB_MA       500
#endif
	
#endif // __ASM_ARCH_MSM_BOARD_J1KD_H
