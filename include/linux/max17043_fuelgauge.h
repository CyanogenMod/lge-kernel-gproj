/*
 *  Copyright (C) 2009 LG Electronics
 *  Dajin Kim <dajin.kim@lge.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __MAX17043_FUELGAUGE_H_
#define __MAX17043_FUELGAUGE_H_

typedef enum {
	MAX17403_UNKNOWN,
	MAX17043_RESET,
	MAX17043_QUICKSTART,
	MAX17043_WORKING,
	MAX17043_STATE_MAX
} max17043_status;

struct max17043g_platform_data {
	unsigned int test_data;
	unsigned int (*test_func)(void);
};
/* 20111222, hiro.kwon@lge.com, fuel gauge not working without batt  [START] */
enum {
	DISABLE_MAX17043_WORK,
	ENABLE_MAX17043_WORK,
};
/* 20111222, hiro.kwon@lge.com, fuel gauge not working without batt [END] */

/* BEGIN: mansu.lee@lge.com 2012-01-16 Implement Quickstart for Test Mode and SOC Accurency */
struct max17043_ocv_to_soc_data {
	int voltage;
	int soc;
};
/* END: mansu.lee@lge.com 2012-01-16 */

/* BEGIN: hiro.kwon@lge.com 2011-12-22 RCOMP update when the temperature of the cell changes */
struct max17043_platform_data {
	u8			starting_rcomp;
	int			temp_co_hot;
	int			temp_co_cold;
	/* START: mansu.lee@lge.com 2011-01-16 Implement Quickstart for Test Mode and SOC Accurency */
	struct max17043_ocv_to_soc_data		*soc_cal_data;
	/* END: mansu.lee@lge.com 2011-01-16 */
};
/* END: hiro.kwon@lge.com 2011-12-22 */
extern int __max17043_get_capacity(void);
extern int __max17043_get_voltage(void);
extern int max17043_do_calibrate(void);
extern int max17043_set_rcomp_by_temperature(void);
extern int max17043_set_alert_level(int alert_level);

#endif
