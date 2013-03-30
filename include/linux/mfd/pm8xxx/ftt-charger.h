#ifndef __FTT_CARGER_H__
#define __FTT_CARGER_H__ 	__FILE__



#endif //__FTT_CARGER_H__


/* include/linux/mfd/pm8xxx/cradle.h
 *
 * Copyright (c) 2011-2012, LG Electronics Inc, All rights reserved.
 * Author: 
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

#ifndef __PM8XXX_FTT_CHARGER_H__
#define __PM8XXX_FTT_CHARGER_H__

#define FTT_CHARGER_DEV_NAME "ftt_charger"

struct ftt_charger_pdata {
	int en1;
	int detect;
	int ftt;
};

int get_ftt_ant_level(void);

#endif /* __PM8XXX_FTT_CHARGER_H__ */

