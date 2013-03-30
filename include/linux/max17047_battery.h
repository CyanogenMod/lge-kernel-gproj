/*
 *  Copyright (C) 2012 LG Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __MAX17047_BATTERY_H_
#define __MAX17047_BATTERY_H_

struct max17047_platform_data {
	int (*battery_online)(void);
	int (*charger_online)(void);
	int (*charger_enable)(void);
};

#define MAX17047_STATUS 					0x00
#define MAX17047_V_ALRT_THRESHOLD 	0x01
#define MAX17047_T_ALRT_THRESHOLD 	0x02
#define MAX17047_SOC_ALRT_THRESHOLD	0x03
#define MAX17047_AT_RATE					0x04
#define MAX17047_REM_CAP_REP			0x05
#define MAX17047_SOC_REP				0x06
#define MAX17047_AGE						0x07
#define MAX17047_TEMPERATURE			0x08
#define MAX17047_V_CELL					0x09
#define MAX17047_CURRENT				0x0A
#define MAX17047_AVERAGE_CURRENT		0x0B
#define MAX17047_SOC_MIX					0x0D
#define MAX17047_SOC_AV					0x0E
#define MAX17047_REM_CAP_MIX			0x0F
#define MAX17047_FULL_CAP				0x10
#define MAX17047_TTE						0x11
#define MAX17047_Q_RESIDUAL_00			0x12
#define MAX17047_FULL_SOC_THR			0x13
#define MAX17047_AVERAGE_TEMP			0x16
#define MAX17047_CYCLES					0x17
#define MAX17047_DESIGN_CAP				0x18
#define MAX17047_AVERAGE_V_CELL		0x19
#define MAX17047_MAX_MIN_TEMP			0x1A
#define MAX17047_MAX_MIN_VOLTAGE		0x1B
#define MAX17047_MAX_MIN_CURRENT		0x1C
#define MAX17047_CONFIG					0x1D
#define MAX17047_I_CHG_TERM				0x1E
#define MAX17047_REM_CAP_AV				0x1F
#define MAX17047_VERSION					0x21
#define MAX17047_Q_RESIDUAL_10			0x22
#define MAX17047_FULL_CAP_NOM			0x23
#define MAX17047_TEMP_NOM				0x24
#define MAX17047_TEMP_LIM				0x25
#define MAX17047_AIN						0x27
#define MAX17047_LEARN_CFG				0x28
#define MAX17047_FILTER_CFG				0x29
#define MAX17047_RELAX_CFG				0x2A
#define MAX17047_MISC_CFG				0x2B
#define MAX17047_T_GAIN					0x2C
#define MAX17047_T_OFF					0x2D
#define MAX17047_C_GAIN					0x2E
#define MAX17047_C_OFF					0x2F
#define MAX17047_Q_RESIDUAL_20			0x32
#define MAX17047_I_AVG_EMPTY			0x36
#define MAX17047_F_CTC					0x37
#define MAX17047_RCOMP_0				0x38
#define MAX17047_TEMP_CO				0x39
#define MAX17047_V_EMPTY					0x3A
#define MAX17047_F_STAT					0x3D
#define MAX17047_TIMER					0x3E
#define MAX17047_SHDN_TIMER				0x3F
#define MAX17047_Q_RESIDUAL_30			0x42
#define MAX17047_D_QACC					0x45
#define MAX17047_D_PACC					0x46
#define MAX17047_QH						0x4D
#define MAX17047_V_FOCV					0xFB
#define MAX17047_SOC_VF					0xFF


#ifdef CONFIG_LGE_PM
int max17047_get_battery_capacity_percent(void);
int max17047_get_battery_mvolts(void);
int max17047_suspend_get_mvolts(void);
int max17047_get_battery_age(void);
int max17047_get_battery_current(void);
int max17047_get_soc_for_charging_complete_at_cmd(void);
bool max17047_write_battery_temp(int battery_temp);
#endif

#endif
