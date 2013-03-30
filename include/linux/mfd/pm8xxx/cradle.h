/* include/linux/mfd/pm8xxx/cradle.h
 *
 * Copyright (c) 2011-2012, LG Electronics Inc, All rights reserved.
 * Author: Fred Cho <fred.cho@lge.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __PM8XXX_CRADLE_H__
#define __PM8XXX_CRADLE_H__

//#define PM8XXX_CRADLE_DEV_NAME  "bu52014hfv"
#define HALL_IC_DEV_NAME "bu52031nvx"

#define CRADLE_NO_DEV         0
#define CRADLE_DESKDOCK     1
#define CRADLE_CARKIT         2

#define HALL_IC_GPIO       7           /*hsh test*/
#define CRADLE_SMARTCOVER       5
#define CRADLE_SMARTCOVER_NO_DEV    6

struct pm8xxx_cradle_platform_data {
    int carkit_detect_pin;
    int pouch_detect_pin;
    unsigned int carkit_irq;
    unsigned int pouch_irq;
    unsigned long irq_flags;
};

void cradle_set_deskdock(int state);
int cradle_get_deskdock(void);

#endif /* __CRADLE_H__ */
