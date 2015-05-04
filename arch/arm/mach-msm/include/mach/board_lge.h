/* arch/arm/mach-msm/include/mach/board_lge.h
 *
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2008-2012, Code Aurora Forum. All rights reserved.
 * Copyright (c) 2012, LGE Inc.
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

#ifndef __ASM_ARCH_MSM_BOARD_LGE_H
#define __ASM_ARCH_MSM_BOARD_LGE_H

#ifdef CONFIG_ANDROID_RAM_CONSOLE
#define LGE_RAM_CONSOLE_SIZE	(124*SZ_1K * 2)
#endif

#ifdef CONFIG_LGE_HANDLE_PANIC
#define LGE_CRASH_LOG_SIZE	(4*SZ_1K)
#endif

typedef enum {
	HW_REV_EVB1 = 0,
	HW_REV_EVB2,
	HW_REV_A,
	HW_REV_B,
	HW_REV_C,
	HW_REV_D,
	HW_REV_E,
	HW_REV_F,
	HW_REV_G,
	HW_REV_H,
	HW_REV_1_0,
	HW_REV_1_1,
	HW_REV_1_2,
	HW_REV_MAX
} hw_rev_type;

hw_rev_type lge_get_board_revno(void);

#ifdef CONFIG_LGE_PM
/*Classified the ADC value for cable detection */
typedef enum {
	NO_INIT_CABLE = 0,
	CABLE_MHL_1K,
	CABLE_U_28P7K,
	CABLE_28P7K,
	CABLE_56K,
	CABLE_100K,
	CABLE_130K,
	CABLE_180K,
	CABLE_200K,
	CABLE_220K,
	CABLE_270K,
	CABLE_330K,
	CABLE_620K,
	CABLE_910K,
	CABLE_NONE
} acc_cable_type;

struct chg_cable_info {
	acc_cable_type cable_type;
	unsigned ta_ma;
	unsigned usb_ma;
};

int lge_pm_get_cable_info(struct chg_cable_info *);
void lge_pm_read_cable_info(void);
acc_cable_type lge_pm_get_cable_type(void);
unsigned lge_pm_get_ta_current(void);
unsigned lge_pm_get_usb_current(void);
#endif

#ifdef CONFIG_LGE_PM_BATTERY_ID_CHECKER
bool is_lge_battery(void);
enum {
	BATT_ID_UNKNOWN,
	BATT_ID_DS2704_N = 17,
	BATT_ID_DS2704_L = 32,
	BATT_ID_ISL6296_N = 73,
	BATT_ID_ISL6296_L = 94,
	BATT_ID_DS2704_C = 48,
	BATT_ID_ISL6296_C =105,
};

#else
static inline bool is_lge_battery(void)
{
	return true;
}
#endif

#ifdef CONFIG_LGE_KCAL
struct kcal_platform_data {
	int (*set_values) (int r, int g, int b);
	int (*get_values) (int *r, int *g, int *b);
	int (*refresh_display) (void);
};
#endif

#ifdef CONFIG_LGE_PM
/* LGE_S kwangjae1.lee@lge.com 2012-06-11 Add bms debugger */
struct bms_batt_info_type{
	int mode;
};
/* LGE_E kwangjae1.lee@lge.com 2012-06-11 Add bms debugger */
struct pseudo_batt_info_type {
	int mode;
	int id;
	int therm;
	int temp;
	int volt;
	int capacity;
	int charging;
};
#endif
int __init lge_get_uart_mode(void);

#if defined(CONFIG_LGE_NFC)
void __init lge_add_nfc_devices(void);
#endif
/* from androidboot.mode */
enum lge_boot_mode_type {
	LGE_BOOT_MODE_NORMAL = 0,
	LGE_BOOT_MODE_CHARGER,
	LGE_BOOT_MODE_CHARGERLOGO,
	LGE_BOOT_MODE_FACTORY,
	LGE_BOOT_MODE_FACTORY2,
	LGE_BOOT_MODE_PIFBOOT,
	LGE_BOOT_MODE_PIFBOOT2,
};

/* from cable_type */
enum lge_boot_cable_type {
    LGE_BOOT_LT_CABLE_56K = 6,
    LGE_BOOT_LT_CABLE_130K,
    LGE_BOOT_USB_CABLE_400MA,
    LGE_BOOT_USB_CABLE_DTC_500MA,
    LGE_BOOT_ABNORMAL_USB_CABLE_400MA,
    LGE_BOOT_LT_CABLE_910K,
    LGE_BOOT_NO_INIT_CABLE,
};

#ifdef CONFIG_ANDROID_RAM_CONSOLE
void __init lge_add_ramconsole_devices(void);
#endif

#ifdef CONFIG_LGE_HANDLE_PANIC
void __init lge_add_panic_handler_devices(void);
int lge_get_magic_for_subsystem(void);
void lge_set_magic_for_subsystem(const char *subsys_name);
#endif

#ifdef CONFIG_LGE_QFPROM_INTERFACE
void __init lge_add_qfprom_devices(void);
#endif

#ifdef CONFIG_LGE_PM_LOW_BATT_CHG
int lge_set_charger_logo_state(int val);
int lge_get_charger_logo_state(void);
#endif

enum lge_boot_mode_type lge_get_boot_mode(void);
enum lge_boot_cable_type lge_get_boot_cable_type(void);
int lge_get_factory_boot(void);

#ifdef CONFIG_LGE_BOOT_TIME_CHECK
void __init lge_add_boot_time_checker(void);
#endif

#ifdef CONFIG_LGE_ECO_MODE
void __init lge_add_lge_kernel_devices(void);
#endif
#endif // __ASM_ARCH_MSM_BOARD_LGE_H
