/* Copyright (c) 2012, LGE Inc.
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/memory.h>
#include <linux/persistent_ram.h>

#include <asm/setup.h>
#include <asm/sizes.h>
#include <asm/system_info.h>
#include <asm/memory.h>
#ifdef CONFIG_LGE_PM
#include <linux/mfd/pm8xxx/pm8921.h>
#include <linux/delay.h>
#endif

#include <mach/board_lge.h>

#include <linux/platform_data/ram_console.h>

#ifdef CONFIG_LGE_PM
#include CONFIG_BOARD_HEADER_FILE
#endif

#ifdef CONFIG_LGE_BOOT_TIME_CHECK
#include "lge_boot_time_checker.h"
#endif

#ifdef CONFIG_KEXEC_HARDBOOT
#include <linux/memblock.h>
#endif

/* setting whether uart console is enalbed or disabled */
static int uart_console_mode = 0;

int __init lge_get_uart_mode(void)
{
	return uart_console_mode;
}

static int __init lge_uart_mode(char *uart_mode)
{
	if (!strncmp("enable", uart_mode, 5)) {
		printk(KERN_INFO"UART CONSOLE : enable\n");
		uart_console_mode = 1;
	}
	else
		printk(KERN_INFO"UART CONSOLE : disable\n");

	return 1;
}
__setup("uart_console=", lge_uart_mode);

#ifdef CONFIG_LGE_PM_LOW_BATT_CHG
int chargerlogo_state = 0;
int lge_set_charger_logo_state(int val)
{
	chargerlogo_state = val;
	printk(KERN_INFO"Chargerlogo state : %d\n", chargerlogo_state);
	return 0;
}
int lge_get_charger_logo_state(void)
{
	//printk(KERN_INFO"Chargerlogo state : %d\n", chargerlogo_state);
	return chargerlogo_state;
}
#endif

#ifdef CONFIG_LGE_PM
/* Implement cable detection */
struct chg_cable_info_table {
	int threshhold;
	acc_cable_type type;
	unsigned ta_ma;
	unsigned usb_ma;
};

/* This table is only for J1 */
static struct chg_cable_info_table pm8921_acc_cable_type_data[]={
	{ADC_NO_INIT_CABLE, NO_INIT_CABLE,  C_NO_INIT_TA_MA,    C_NO_INIT_USB_MA},
	{ADC_CABLE_MHL_1K,  CABLE_MHL_1K,   C_MHL_1K_TA_MA,     C_MHL_1K_USB_MA},
	{ADC_CABLE_U_28P7K, CABLE_U_28P7K,  C_U_28P7K_TA_MA,    C_U_28P7K_USB_MA},
	{ADC_CABLE_28P7K,   CABLE_28P7K,    C_28P7K_TA_MA,      C_28P7K_USB_MA},
	{ADC_CABLE_56K,     CABLE_56K,      C_56K_TA_MA,        C_56K_USB_MA},
	{ADC_CABLE_100K,    CABLE_100K,     C_100K_TA_MA,       C_100K_USB_MA},
	{ADC_CABLE_130K,    CABLE_130K,     C_130K_TA_MA,       C_130K_USB_MA},
	{ADC_CABLE_180K,    CABLE_180K,     C_180K_TA_MA,       C_180K_USB_MA},
	{ADC_CABLE_200K,    CABLE_200K,     C_200K_TA_MA,       C_200K_USB_MA},
	{ADC_CABLE_220K,    CABLE_220K,     C_220K_TA_MA,       C_220K_USB_MA},
	{ADC_CABLE_270K,    CABLE_270K,     C_270K_TA_MA,       C_270K_USB_MA},
	{ADC_CABLE_330K,    CABLE_330K,     C_330K_TA_MA,       C_330K_USB_MA},
	{ADC_CABLE_620K,    CABLE_620K,     C_620K_TA_MA,       C_620K_USB_MA},
	{ADC_CABLE_910K,    CABLE_910K,     C_910K_TA_MA,       C_910K_USB_MA},
	{ADC_CABLE_NONE,    CABLE_NONE,     C_NONE_TA_MA,       C_NONE_USB_MA},
};
#endif

/* for board revision */
static hw_rev_type lge_bd_rev = HW_REV_EVB1;

static int __init board_revno_setup(char *rev_info)
{
	/* CAUTION: These strings are come from LK. */
	char *rev_str[] = {"evb1", "evb2", "rev_a", "rev_b", "rev_c", "rev_d",
		"rev_e", "rev_f", "rev_g", "rev_h", "rev_10", "rev_11", "rev_12",
		"reserved"};
	int i;

	printk(KERN_INFO "BOARD : LGE input %s \n", rev_info);
	for (i=0; i< HW_REV_MAX; i++) {
		if( !strncmp(rev_info, rev_str[i], 6)) {
			lge_bd_rev = (hw_rev_type) i;
			system_rev = lge_bd_rev;
			break;
		}
	}

	printk(KERN_INFO "BOARD : LGE matched %s \n", rev_str[lge_bd_rev]);
	return 1;
}
__setup("lge.rev=", board_revno_setup);

hw_rev_type lge_get_board_revno(void)
{
    return lge_bd_rev;
}

#ifdef CONFIG_MACH_APQ8064_ALTEV
unsigned lge_get_board_revno_adc(void)
{
	struct pm8xxx_adc_chan_result result;
	
	pm8xxx_adc_mpp_config_read(PM8XXX_AMUX_MPP_3,
			ADC_MPP_1_AMUX6, &result);

	return result.physical;
}
#endif
#ifdef CONFIG_LGE_PM
int lge_pm_get_cable_info(struct chg_cable_info *cable_info)
{
	char *type_str[] = {"NOT INIT", "MHL 1K", "U_28P7K", "28P7K", "56K",
		"100K", "130K", "180K", "200K", "220K", "270K", "330K", "620K", "910K",
		"OPEN"};

	struct pm8xxx_adc_chan_result result;
	struct chg_cable_info *info = cable_info;
	struct chg_cable_info_table *table;
	int table_size = ARRAY_SIZE(pm8921_acc_cable_type_data);
	int acc_read_value = 0;
	int acc_read_value_data[5] = {0};
	int i, j, rc;
	int count = 5;

	if (!info) {
		pr_err("lge_pm_get_cable_info: invalid info parameters\n");
		return -1;
	}

	for (i = 0; i < count; i++) {
		rc = pm8xxx_adc_mpp_config_read(PM8XXX_AMUX_MPP_12,
				ADC_MPP_1_AMUX6, &result);

		if (rc < 0) {
			if (rc == -ETIMEDOUT) {
				/* reason: adc read timeout, assume it is open cable */
				info->cable_type = CABLE_NONE;
				info->ta_ma = C_NONE_TA_MA;
				info->usb_ma = C_NONE_USB_MA;
				pr_err("[DEBUG] lge_pm_get_cable_info : adc read timeout \n");
			} else {
	    			pr_err("lge_pm_get_cable_info: adc read error - %d\n", rc);
			}
			return rc;
		}

		acc_read_value_data[i] = (int)result.physical;
		pr_info("%s: acc_read_value - %d\n", __func__, (int)result.physical);

		for(j = 0; j < i; j++)
		{
			if(abs(acc_read_value_data[i] - acc_read_value_data[i-j-1]) > 100000)
			{
				count = 0;
				acc_read_value = 1800000;
				pr_info("%s: abnormal acc_read_value\n", __func__);
				break;
			}
			else
				acc_read_value = (int)result.physical;

		}
		mdelay(10);
	}

	info->cable_type = NO_INIT_CABLE;
	info->ta_ma = C_NO_INIT_TA_MA;
	info->usb_ma = C_NO_INIT_USB_MA;

	/* assume: adc value must be existed in ascending order */
	for (i = 0; i < table_size; i++) {
			table = &pm8921_acc_cable_type_data[i];

		if (acc_read_value <= table->threshhold) {
			info->cable_type = table->type;
			info->ta_ma = table->ta_ma;
			info->usb_ma = table->usb_ma;
			break;
		}
	}

	pr_info("\n\n[PM]Cable detected: %d(%s)(%d, %d)\n\n",
			acc_read_value, type_str[info->cable_type],
			info->ta_ma, info->usb_ma);

	return 0;
}

/* Belows are for using in interrupt context */
//static struct chg_cable_info lge_cable_info;
struct chg_cable_info lge_cable_info;

acc_cable_type lge_pm_get_cable_type(void)
{
	return lge_cable_info.cable_type;
}

#ifdef CONFIG_MACH_APQ8064_ALTEV
unsigned lge_pm_get_cable_type_adc(void)
{
	struct pm8xxx_adc_chan_result result;
	
	pm8xxx_adc_mpp_config_read(PM8XXX_AMUX_MPP_12,
			ADC_MPP_1_AMUX6, &result);

	return result.physical;
}
#endif
unsigned lge_pm_get_ta_current(void)
{
	return lge_cable_info.ta_ma;
}

unsigned lge_pm_get_usb_current(void)
{
	return lge_cable_info.usb_ma;
}

/* This must be invoked in process context */
void lge_pm_read_cable_info(void)
{
	lge_cable_info.cable_type = NO_INIT_CABLE;
	lge_cable_info.ta_ma = C_NO_INIT_TA_MA;
	lge_cable_info.usb_ma = C_NO_INIT_USB_MA;

	lge_pm_get_cable_info(&lge_cable_info);
}
#endif

#ifdef CONFIG_LGE_PM_BATTERY_ID_CHECKER
int lge_battery_info = BATT_ID_UNKNOWN;

bool is_lge_battery(void)
{
	if (lge_battery_info == BATT_ID_DS2704_N ||
	    lge_battery_info == BATT_ID_DS2704_L ||
	    lge_battery_info == BATT_ID_DS2704_C ||
	    lge_battery_info == BATT_ID_ISL6296_N ||
	    lge_battery_info == BATT_ID_ISL6296_L ||
	    lge_battery_info == BATT_ID_ISL6296_C)
		return true;
	return false;
}
EXPORT_SYMBOL(is_lge_battery);

static int __init battery_information_setup(char *batt_info)
{
        if(!strcmp(batt_info, "ds2704_n"))
                lge_battery_info = BATT_ID_DS2704_N;
        else if(!strcmp(batt_info, "ds2704_l"))
                lge_battery_info = BATT_ID_DS2704_L;
        else if(!strcmp(batt_info, "isl6296_n"))
                lge_battery_info = BATT_ID_ISL6296_N;
        else if(!strcmp(batt_info, "isl6296_l"))
                lge_battery_info = BATT_ID_ISL6296_L;
		else if(!strcmp(batt_info, "isl6296_c"))
				lge_battery_info = BATT_ID_ISL6296_C;
		else if(!strcmp(batt_info, "ds2704_c"))
				lge_battery_info = BATT_ID_DS2704_C;
        else
                lge_battery_info = BATT_ID_UNKNOWN;

        printk(KERN_INFO "Battery : %s %d\n", batt_info, lge_battery_info);

        return 1;
}
__setup("lge.batt_info=", battery_information_setup);
#endif

#ifdef CONFIG_LGE_KCAL
#if defined (CONFIG_MACH_APQ8064_GKU) || defined (CONFIG_MACH_APQ8064_GKKT) || defined (CONFIG_MACH_APQ8064_GKSK) || defined(CONFIG_MACH_APQ8064_GVKT) || defined(CONFIG_MACH_APQ8064_GKATT) || defined(CONFIG_MACH_APQ8064_GKGLOBAL) || defined(CONFIG_MACH_APQ8064_OMEGAR) || defined(CONFIG_MACH_APQ8064_OMEGA)
int g_kcal_r = 248;
int g_kcal_g = 248;
int g_kcal_b = 255;
#elif defined (CONFIG_MACH_APQ8064_GVDCM)
int g_kcal_r = 250;
int g_kcal_g = 250;
int g_kcal_b = 255;
#else
int g_kcal_r = 255;
int g_kcal_g = 255;
int g_kcal_b = 255;
#endif
static int __init display_kcal_setup(char *kcal)
{
		char vaild_k = 0;
        sscanf(kcal, "%d|%d|%d|%c", &g_kcal_r, &g_kcal_g, &g_kcal_b, &vaild_k );
        printk(KERN_INFO "kcal is %d|%d|%d|%c\n",
                                        g_kcal_r, g_kcal_g, g_kcal_b, vaild_k);

#if defined (CONFIG_MACH_APQ8064_GKU) || defined (CONFIG_MACH_APQ8064_GKKT) || defined (CONFIG_MACH_APQ8064_GKSK) || defined (CONFIG_MACH_APQ8064_GKATT) || defined(CONFIG_MACH_APQ8064_GKGLOBAL) || defined(CONFIG_MACH_APQ8064_OMEGAR) || defined(CONFIG_MACH_APQ8064_OMEGA) || defined(CONFIG_MACH_APQ8064_GVKT)
       g_kcal_r = g_kcal_r < 248 ? 248 : g_kcal_r;
       g_kcal_g = g_kcal_g < 248 ? 248 : g_kcal_g;
       g_kcal_b = g_kcal_b < 253 ? 253 : g_kcal_b;
#endif

        if(vaild_k != 'K') {
                printk(KERN_INFO "kcal not calibrated yet : %d\n", vaild_k);
                g_kcal_r = g_kcal_g = g_kcal_b = 255;
                printk(KERN_INFO "set to default : %d\n", g_kcal_r);
        }
        return 1;
}
__setup("lge.kcal=", display_kcal_setup);
#endif

/* get boot mode information from cmdline.
 * If any boot mode is not specified,
 * boot mode is normal type.
 */
static enum lge_boot_mode_type lge_boot_mode = LGE_BOOT_MODE_NORMAL;
int __init lge_boot_mode_init(char *s)
{
	if (!strcmp(s, "charger"))
		lge_boot_mode = LGE_BOOT_MODE_CHARGER;
	else if (!strcmp(s, "chargerlogo"))
		lge_boot_mode = LGE_BOOT_MODE_CHARGERLOGO;
	else if (!strcmp(s, "factory"))
		lge_boot_mode = LGE_BOOT_MODE_FACTORY;
	else if (!strcmp(s, "factory2"))
		lge_boot_mode = LGE_BOOT_MODE_FACTORY2;
	else if (!strcmp(s, "pifboot"))
		lge_boot_mode = LGE_BOOT_MODE_PIFBOOT;
	else if (!strcmp(s, "pifboot2"))
		lge_boot_mode = LGE_BOOT_MODE_PIFBOOT2;

#ifdef CONFIG_LGE_PM_LOW_BATT_CHG
	if(lge_boot_mode == LGE_BOOT_MODE_CHARGERLOGO)
		lge_set_charger_logo_state(1);
	else
		lge_set_charger_logo_state(0);
#endif

	return 1;
}
__setup("androidboot.mode=", lge_boot_mode_init);

enum lge_boot_mode_type lge_get_boot_mode(void)
{
	return lge_boot_mode;
}

static enum lge_boot_cable_type lge_boot_cable = LGE_BOOT_NO_INIT_CABLE;
int __init lge_boot_cable_type_init(char *s)
{
    lge_boot_cable = 0;

    for (;; s++) {
        switch (*s) {
            case '0' ... '9':
                lge_boot_cable = 10*lge_boot_cable+(*s-'0');
                break;
            default:
                return 1;
        }
    }
}
__setup("cable_type=", lge_boot_cable_type_init);

enum lge_boot_cable_type lge_get_boot_cable_type(void)
{
    return lge_boot_cable;
}

int lge_get_factory_boot(void)
{
	int res;

	/* if boot mode is factory,
	 * cable must be factory cable.
	 */
	switch (lge_boot_mode) {
		case LGE_BOOT_MODE_FACTORY:
		case LGE_BOOT_MODE_FACTORY2:
		case LGE_BOOT_MODE_PIFBOOT:
		case LGE_BOOT_MODE_PIFBOOT2:
			res = 1;
			break;
		default:
			res = 0;
			break;
	}

	return res;
}

#ifdef CONFIG_ANDROID_PERSISTENT_RAM
static struct persistent_ram_descriptor pram_descs[] = {
#ifdef CONFIG_ANDROID_RAM_CONSOLE
	{
		.name = "ram_console",
		.size = LGE_RAM_CONSOLE_SIZE,
	},
#endif
#ifdef CONFIG_LGE_CRASH_HANDLER
	{
		.name = "panic-handler",
		.size = LGE_CRASH_LOG_SIZE,
	},
#endif
};

static struct persistent_ram lge_persistent_ram = {
	.size = LGE_PERSISTENT_RAM_SIZE,
	.num_descs = ARRAY_SIZE(pram_descs),
	.descs = pram_descs,
};

void __init lge_add_persistent_ram(void)
{
	struct persistent_ram *pram = &lge_persistent_ram;
	struct membank* bank = &meminfo.bank[0];

	pram->start = bank->start + bank->size - LGE_PERSISTENT_RAM_SIZE;

	persistent_ram_early_init(pram);
}
#endif

void __init lge_reserve(void)
{
#ifdef CONFIG_KEXEC_HARDBOOT
	// Reserve space for hardboot page, just before the ram_console
	struct membank* bank = &meminfo.bank[0];
	phys_addr_t start = bank->start + bank->size - SZ_1M - LGE_PERSISTENT_RAM_SIZE;
	int ret = memblock_remove(start, SZ_1M);
	if(!ret)
		pr_info("Hardboot page reserved at 0x%X\n", start);
	else
		pr_err("Failed to reserve space for hardboot page at 0x%X!\n", start);
#endif

	lge_add_persistent_ram();
}

#ifdef CONFIG_ANDROID_RAM_CONSOLE
static char bootreason[128] = {0,};
int __init lge_boot_reason(char *s)
{
	int n;

	if (*s == '=')
		s++;
	n = snprintf(bootreason, sizeof(bootreason),
		 "Boot info:\n"
		 "Last boot reason: %s\n", s);
	bootreason[n] = '\0';
	return 1;
}
__setup("bootreason", lge_boot_reason);

struct ram_console_platform_data ram_console_pdata = {
	.bootinfo = bootreason,
};

static struct platform_device ram_console_device = {
	.name = "ram_console",
	.id = -1,
	.dev = {
		.platform_data = &ram_console_pdata,
	}
};

void __init lge_add_ramconsole_devices(void)
{
	platform_device_register(&ram_console_device);
}
#endif /* CONFIG_ANDROID_RAM_CONSOLE */

#ifdef CONFIG_LGE_CRASH_HANDLER
static struct platform_device panic_handler_device = {
	.name = "panic-handler",
	.id = -1,
};

void __init lge_add_panic_handler_devices(void)
{
	platform_device_register(&panic_handler_device);
}
#endif /*                          */

#ifdef CONFIG_LGE_ECO_MODE
static struct platform_device lge_kernel_device = {
	.name = "lge_kernel_driver",
	.id = -1,
};

void __init lge_add_lge_kernel_devices(void)
{
	platform_device_register(&lge_kernel_device);
}
#endif

#ifdef CONFIG_LGE_QFPROM_INTERFACE
static struct platform_device qfprom_device = {
	.name = "lge-apq8064-qfprom",
	.id = -1,
};
void __init lge_add_qfprom_devices(void)
{
	platform_device_register(&qfprom_device);
}
#endif

#ifdef CONFIG_LGE_BOOT_TIME_CHECK
static int lge_time_stamp_mode = 0;

int  lge_get_time_stamp_mode(void)
{
	return lge_time_stamp_mode;
}

static int __init lge_time_stamp(char *mode)
{
	if (!strncmp("enable", mode, 5))
		lge_time_stamp_mode = 1;

	return 1;
}
__setup("boot_time_stamp=", lge_time_stamp);

static struct platform_device boot_time_device = {
.name = "boot_time",
	.id = -1,
	.dev = {
		.platform_data = NULL,
	},
};
void __init lge_add_boot_time_checker(void)
{
	if(lge_get_time_stamp_mode())
		platform_device_register(&boot_time_device);
}
#endif
