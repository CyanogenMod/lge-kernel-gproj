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

#include "msm_sensor.h"
#include "msm.h"
#include "msm_sensor_bayer.h"
/* LGE_CHANGE_S, Code refining for Jelly bean from G-MR ver. , 2012.11.03 youngwook.song */
// #include "imx091.h"
/* LGE_CHANGE_E, Code refining for Jelly bean from G-MR ver. , 2012.11.03 youngwook.song */

static struct i2c_driver *sensor_i2c_driver[] = {
	/* back camera */
/* LGE_CHANGE_S, Code refining for Jelly bean from G-MR ver. , 2012.11.03 youngwook.song */
//	&imx091_i2c_driver,
/* LGE_CHANGE_E, Code refining for Jelly bean from G-MR ver. , 2012.11.03 youngwook.song */
	/* front camera */
};

static int __init msm_sensor_init_module(void)
{
	int index = 0;
	for (index = 0; index < ARRAY_SIZE(sensor_i2c_driver); index++)
		i2c_add_driver(sensor_i2c_driver[index]);
	return 0;
}

module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("Sensor driver probe");
MODULE_LICENSE("GPL v2");
