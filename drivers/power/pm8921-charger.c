/* Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
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
#define pr_fmt(fmt)	"%s: " fmt, __func__

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/mfd/pm8xxx/pm8921-charger.h>
#include <linux/mfd/pm8xxx/pm8921-bms.h>
#include <linux/mfd/pm8xxx/pm8xxx-adc.h>
#include <linux/mfd/pm8xxx/ccadc.h>
#include <linux/mfd/pm8xxx/core.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/bitops.h>
#include <linux/workqueue.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/ratelimit.h>

#include <mach/msm_xo.h>
#include <mach/msm_hsusb.h>

#ifdef CONFIG_BATTERY_MAX17047
#include <linux/max17047_battery.h>
#endif

#ifdef CONFIG_BATTERY_MAX17043
#include <linux/max17043_fuelgauge.h>
#endif

#ifdef CONFIG_LGE_PM
#include <mach/board_lge.h>
#endif

#ifdef CONFIG_LGE_WIRELESS_CHARGER
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/time.h>
#endif
#ifdef CONFIG_LGE_FTT_CHARGER
#include <linux/mfd/pm8xxx/ftt-charger.h>
#endif

#ifdef CONFIG_MACH_LGE
#include <linux/gpio.h>
#endif

#ifdef CONFIG_LGE_PM_REMOVE_BATT
#include <linux/reboot.h>
#endif

#define ChgLog(x, ...) //printk(x, ##__VA_ARGS__)

#ifdef CONFIG_LGE_PM_ADP_CHG
/* Adapive USB draw current limit */
enum {
	IUSB_MAX_NONE,
	IUSB_MAX_INCREASE,
	IUSB_MAX_DECREASE,
};

#if defined(CONFIG_MACH_APQ8064_GKGLOBAL)&& !defined(CONFIG_MACH_APQ8064_GKTCLMX)
#define ADAPTIVE_USB_CURRENT_CHECK_PERIOD_MS 	(1000)
#define ADAPTIVE_USB_CURRENT_DROP_CHECK_MS	(100)
#define CHARGING_COLLAPSE_VOLTAGE		4580

#define ADAPTIVE_NUM_OF_TABLE                   (4)
static int ADAPTIVE_MA_TABLE[ADAPTIVE_NUM_OF_TABLE] = { 500, 700, 900, 1100};
static int ADAPTIVE_MA_TABLE_CHARGERLOGO[] = { 500, 700, 900, 1100};

#elif defined(CONFIG_MACH_APQ8064_GK_KR) || defined(CONFIG_MACH_APQ8064_GKATT) || defined(CONFIG_MACH_APQ8064_GVDCM) || defined(CONFIG_MACH_APQ8064_GV_KR) || defined(CONFIG_MACH_APQ8064_GKTCLMX)
#define ADAPTIVE_USB_CURRENT_CHECK_PERIOD_MS 	(1000)
#define ADAPTIVE_USB_CURRENT_DROP_CHECK_MS	(100)
#define CHARGING_COLLAPSE_VOLTAGE		4580
#define ADAPTIVE_NUM_OF_TABLE                   (5)
static int ADAPTIVE_MA_TABLE[ADAPTIVE_NUM_OF_TABLE] = { 500, 700, 900, 1100, 1500 };
static int ADAPTIVE_MA_TABLE_CHARGERLOGO[] = { 500, 700, 900, 1100, 1500 };
#elif defined (CONFIG_MACH_APQ8064_J1D)
#define ADAPTIVE_USB_CURRENT_CHECK_PERIOD_MS	(5000)
#define ADAPTIVE_USB_CURRENT_DROP_CHECK_MS		(500)
#define CHARGING_COLLAPSE_VOLTAGE				(4500)

#define ADAPTIVE_NUM_OF_TABLE					(2)
static int ADAPTIVE_MA_TABLE[ADAPTIVE_NUM_OF_TABLE] = { 900, 1500 };
static int ADAPTIVE_MA_TABLE_CHARGERLOGO[] = { 900, 1500 };
#else
#define ADAPTIVE_USB_CURRENT_CHECK_PERIOD_MS    (1000)
#define ADAPTIVE_USB_CURRENT_DROP_CHECK_MS      (100)
#define CHARGING_COLLAPSE_VOLTAGE               (the_chip->max_voltage_mv)
#define ADAPTIVE_NUM_OF_TABLE                   (4)
static int ADAPTIVE_MA_TABLE[ADAPTIVE_NUM_OF_TABLE] = { 500, 700, 850, 900 };
static int ADAPTIVE_MA_TABLE_CHARGERLOGO[] = { 500, 700, 850, 900, 1100 };
#endif

static int search_iusb_max_status;
#endif /* End of CONFIG_LGE_PM_ADP_CHG */

#define CHG_BUCK_CLOCK_CTRL	0x14

#define PBL_ACCESS1		0x04
#define PBL_ACCESS2		0x05
#define SYS_CONFIG_1		0x06
#define SYS_CONFIG_2		0x07
#define CHG_CNTRL		0x204
#define CHG_IBAT_MAX		0x205
#define CHG_TEST		0x206
#define CHG_BUCK_CTRL_TEST1	0x207
#define CHG_BUCK_CTRL_TEST2	0x208
#define CHG_BUCK_CTRL_TEST3	0x209
#define COMPARATOR_OVERRIDE	0x20A
#define PSI_TXRX_SAMPLE_DATA_0	0x20B
#define PSI_TXRX_SAMPLE_DATA_1	0x20C
#define PSI_TXRX_SAMPLE_DATA_2	0x20D
#define PSI_TXRX_SAMPLE_DATA_3	0x20E
#define PSI_CONFIG_STATUS	0x20F
#define CHG_IBAT_SAFE		0x210
#define CHG_ITRICKLE		0x211
#define CHG_CNTRL_2		0x212
#define CHG_VBAT_DET		0x213
#define CHG_VTRICKLE		0x214
#define CHG_ITERM		0x215
#define CHG_CNTRL_3		0x216
#define CHG_VIN_MIN		0x217
#define CHG_TWDOG		0x218
#define CHG_TTRKL_MAX		0x219
#define CHG_TEMP_THRESH		0x21A
#define CHG_TCHG_MAX		0x21B
#define USB_OVP_CONTROL		0x21C
#define DC_OVP_CONTROL		0x21D
#define USB_OVP_TEST		0x21E
#define DC_OVP_TEST		0x21F
#define CHG_VDD_MAX		0x220
#define CHG_VDD_SAFE		0x221
#define CHG_VBAT_BOOT_THRESH	0x222
#define USB_OVP_TRIM		0x355
#define BUCK_CONTROL_TRIM1	0x356
#define BUCK_CONTROL_TRIM2	0x357
#define BUCK_CONTROL_TRIM3	0x358
#define BUCK_CONTROL_TRIM4	0x359
#define CHG_DEFAULTS_TRIM	0x35A
#define CHG_ITRIM		0x35B
#define CHG_TTRIM		0x35C
#define CHG_COMP_OVR		0x20A
#define IUSB_FINE_RES		0x2B6
#define OVP_USB_UVD		0x2B7

/* check EOC every 10 seconds */
#define EOC_CHECK_PERIOD_MS	10000
/* check for USB unplug every 200 msecs */
#define UNPLUG_CHECK_WAIT_PERIOD_MS 200

enum chg_fsm_state {
	FSM_STATE_OFF_0 = 0,
	FSM_STATE_BATFETDET_START_12 = 12,
	FSM_STATE_BATFETDET_END_16 = 16,
	FSM_STATE_ON_CHG_HIGHI_1 = 1,
	FSM_STATE_ATC_2A = 2,
	FSM_STATE_ATC_2B = 18,
	FSM_STATE_ON_BAT_3 = 3,
	FSM_STATE_ATC_FAIL_4 = 4 ,
	FSM_STATE_DELAY_5 = 5,
	FSM_STATE_ON_CHG_AND_BAT_6 = 6,
	FSM_STATE_FAST_CHG_7 = 7,
	FSM_STATE_TRKL_CHG_8 = 8,
	FSM_STATE_CHG_FAIL_9 = 9,
	FSM_STATE_EOC_10 = 10,
	FSM_STATE_ON_CHG_VREGOK_11 = 11,
	FSM_STATE_ATC_PAUSE_13 = 13,
	FSM_STATE_FAST_CHG_PAUSE_14 = 14,
	FSM_STATE_TRKL_CHG_PAUSE_15 = 15,
	FSM_STATE_START_BOOT = 20,
	FSM_STATE_FLCB_VREGOK = 21,
	FSM_STATE_FLCB = 22,
};

struct fsm_state_to_batt_status {
	enum chg_fsm_state	fsm_state;
	int			batt_state;
};

static struct fsm_state_to_batt_status map[] = {
	{FSM_STATE_OFF_0, POWER_SUPPLY_STATUS_UNKNOWN},
	{FSM_STATE_BATFETDET_START_12, POWER_SUPPLY_STATUS_UNKNOWN},
	{FSM_STATE_BATFETDET_END_16, POWER_SUPPLY_STATUS_UNKNOWN},
	/*
	 * for CHG_HIGHI_1 report NOT_CHARGING if battery missing,
	 * too hot/cold, charger too hot
	 */
	{FSM_STATE_ON_CHG_HIGHI_1, POWER_SUPPLY_STATUS_FULL},
	{FSM_STATE_ATC_2A, POWER_SUPPLY_STATUS_CHARGING},
	{FSM_STATE_ATC_2B, POWER_SUPPLY_STATUS_CHARGING},
	{FSM_STATE_ON_BAT_3, POWER_SUPPLY_STATUS_DISCHARGING},
	{FSM_STATE_ATC_FAIL_4, POWER_SUPPLY_STATUS_DISCHARGING},
	{FSM_STATE_DELAY_5, POWER_SUPPLY_STATUS_UNKNOWN },
	{FSM_STATE_ON_CHG_AND_BAT_6, POWER_SUPPLY_STATUS_CHARGING},
	{FSM_STATE_FAST_CHG_7, POWER_SUPPLY_STATUS_CHARGING},
	{FSM_STATE_TRKL_CHG_8, POWER_SUPPLY_STATUS_CHARGING},
	{FSM_STATE_CHG_FAIL_9, POWER_SUPPLY_STATUS_DISCHARGING},
	{FSM_STATE_EOC_10, POWER_SUPPLY_STATUS_FULL},
	{FSM_STATE_ON_CHG_VREGOK_11, POWER_SUPPLY_STATUS_NOT_CHARGING},
	{FSM_STATE_ATC_PAUSE_13, POWER_SUPPLY_STATUS_NOT_CHARGING},
	{FSM_STATE_FAST_CHG_PAUSE_14, POWER_SUPPLY_STATUS_NOT_CHARGING},
	{FSM_STATE_TRKL_CHG_PAUSE_15, POWER_SUPPLY_STATUS_NOT_CHARGING},
	{FSM_STATE_START_BOOT, POWER_SUPPLY_STATUS_NOT_CHARGING},
	{FSM_STATE_FLCB_VREGOK, POWER_SUPPLY_STATUS_NOT_CHARGING},
	{FSM_STATE_FLCB, POWER_SUPPLY_STATUS_NOT_CHARGING},
};

enum chg_regulation_loop {
	VDD_LOOP = BIT(3),
	BAT_CURRENT_LOOP = BIT(2),
	INPUT_CURRENT_LOOP = BIT(1),
	INPUT_VOLTAGE_LOOP = BIT(0),
	CHG_ALL_LOOPS = VDD_LOOP | BAT_CURRENT_LOOP
			| INPUT_CURRENT_LOOP | INPUT_VOLTAGE_LOOP,
};

enum pmic_chg_interrupts {
	USBIN_VALID_IRQ = 0,
	USBIN_OV_IRQ,
	BATT_INSERTED_IRQ,
	VBATDET_LOW_IRQ,
	USBIN_UV_IRQ,
	VBAT_OV_IRQ,
	CHGWDOG_IRQ,
	VCP_IRQ,
	ATCDONE_IRQ,
	ATCFAIL_IRQ,
	CHGDONE_IRQ,
	CHGFAIL_IRQ,
	CHGSTATE_IRQ,
	LOOP_CHANGE_IRQ,
	FASTCHG_IRQ,
	TRKLCHG_IRQ,
	BATT_REMOVED_IRQ,
	BATTTEMP_HOT_IRQ,
	CHGHOT_IRQ,
	BATTTEMP_COLD_IRQ,
	CHG_GONE_IRQ,
	BAT_TEMP_OK_IRQ,
	COARSE_DET_LOW_IRQ,
	VDD_LOOP_IRQ,
	VREG_OV_IRQ,
	VBATDET_IRQ,
	BATFET_IRQ,
	PSI_IRQ,
	DCIN_VALID_IRQ,
	DCIN_OV_IRQ,
	DCIN_UV_IRQ,
	PM_CHG_MAX_INTS,
};

struct bms_notify {
	int			is_battery_full;
	int			is_charging;
#ifdef CONFIG_LGE_PM
	int			is_charge_done_hit;
#endif
	struct	work_struct	work;
};

/**
 * struct pm8921_chg_chip -device information
 * @dev:			device pointer to access the parent
 * @usb_present:		present status of usb
 * @dc_present:			present status of dc
 * @usb_charger_current:	usb current to charge the battery with used when
 *				the usb path is enabled or charging is resumed
 * @safety_time:		max time for which charging will happen
 * @update_time:		how frequently the userland needs to be updated
 * @max_voltage_mv:		the max volts the batt should be charged up to
 * @min_voltage_mv:		the min battery voltage before turning the FETon
 * @uvd_voltage_mv:		(PM8917 only) the falling UVD threshold voltage
 * @cool_temp_dc:		the cool temp threshold in deciCelcius
 * @warm_temp_dc:		the warm temp threshold in deciCelcius
 * @resume_voltage_delta:	the voltage delta from vdd max at which the
 *				battery should resume charging
 * @term_current:		The charging based term current
 *
 */
struct pm8921_chg_chip {
	struct device			*dev;
	unsigned int			usb_present;
	unsigned int			dc_present;
	unsigned int			usb_charger_current;
	unsigned int			max_bat_chg_current;
	unsigned int			pmic_chg_irq[PM_CHG_MAX_INTS];
	unsigned int			safety_time;
	unsigned int			ttrkl_time;
	unsigned int			update_time;
	unsigned int			max_voltage_mv;
	unsigned int			min_voltage_mv;
	unsigned int			uvd_voltage_mv;
	int				cool_temp_dc;
	int				warm_temp_dc;
	unsigned int			temp_check_period;
	unsigned int			cool_bat_chg_current;
	unsigned int			warm_bat_chg_current;
	unsigned int			cool_bat_voltage;
	unsigned int			warm_bat_voltage;
	unsigned int			is_bat_cool;
	unsigned int			is_bat_warm;
	unsigned int			resume_voltage_delta;
	int				resume_charge_percent;
	unsigned int			term_current;
	unsigned int			vbat_channel;
	unsigned int			batt_temp_channel;
	unsigned int			batt_id_channel;
#ifdef CONFIG_LGE_WIRELESS_CHARGER
	struct power_supply		wireless_psy;
	struct delayed_work		wireless_inform_work;
	struct delayed_work		wireless_chip_control_work;
	struct delayed_work		wireless_unplug_check_work;
	struct delayed_work		wireless_source_control_work;
	struct delayed_work		wireless_polling_dc_work;
	struct work_struct		dcin_valid_irq_work;
	struct work_struct		wireless_interrupt_work;
	struct wake_lock		wireless_unplug_wake_lock;
	struct wake_lock		wireless_chip_wake_lock;
	struct wake_lock		wireless_source_wake_lock;
#endif
	struct power_supply		usb_psy;
	struct power_supply		dc_psy;
	struct power_supply		*ext_psy;
	struct power_supply		batt_psy;
	struct dentry			*dent;
	struct bms_notify		bms_notify;
	bool				keep_btm_on_suspend;
	bool				ext_charging;
	bool				ext_charge_done;
	bool				iusb_fine_res;
	bool				dc_unplug_check;
	DECLARE_BITMAP(enabled_irqs, PM_CHG_MAX_INTS);
	struct work_struct		battery_id_valid_work;
	int64_t				batt_id_min;
	int64_t				batt_id_max;
	int				trkl_voltage;
	int				weak_voltage;
	int				trkl_current;
	int				weak_current;
	int				vin_min;
	unsigned int			*thermal_mitigation;
	int				thermal_levels;
	struct delayed_work		update_heartbeat_work;
	struct delayed_work		eoc_work;
	struct delayed_work		unplug_check_work;
	struct delayed_work		vin_collapse_check_work;
#ifdef CONFIG_LGE_PM_ADP_CHG
	struct delayed_work      adaptive_usb_current_work;
#endif
	struct wake_lock		eoc_wake_lock;
	enum pm8921_chg_cold_thr	cold_thr;
	enum pm8921_chg_hot_thr		hot_thr;
	int				rconn_mohm;
	enum pm8921_chg_led_src_config	led_src_config;
	bool				has_dc_supply;
	int				recent_reported_soc;
	bool			host_mode;
	u8				active_path;
#ifdef CONFIG_LGE_PM_ADP_CHG
	struct delayed_work		unplug_wrkarnd_restore_work;
	struct wake_lock		unplug_wrkarnd_restore_wake_lock;
#endif
#ifdef CONFIG_LGE_CHARGER_TEMP_SCENARIO
	/* Create work for temp scerario kwangjae1.lee@lge.com */
	struct delayed_work		monitor_batt_temp_work;
	struct wake_lock        monitor_batt_temp_wake_lock;
	int				temp_level_1;
#if defined(CONFIG_MACH_APQ8064_J1SP)
	/* Add temp for charing scenario on SPRINT */
	int				temp_level_1_1;
#endif
	int				temp_level_2;
	int				temp_level_3;
	int				temp_level_4;
	int				temp_level_5;
#endif
#ifdef CONFIG_LGE_PM_REMOVE_BATT
	/* Restart the machine when Battery Remove/Insert - seonghun1.kim */
	struct work_struct		kernel_restart_work;
#endif
#ifdef CONFIG_LGE_PM
	/* Wake lock for deliver Uevent before suspend */
	struct wake_lock		deliver_uevent_wake_lock;
#endif
#ifdef CONFIG_LGE_WIRELESS_CHARGER
	int				wlc_ts_ctrl;
#endif
#ifdef CONFIG_LGE_PM
/* MAKO patch for BMS */
	int			eoc_check_soc;
#endif
};

#ifdef CONFIG_LGE_CHARGER_TEMP_SCENARIO
#define BATT_TEMP_LEVEL_POWER_OFF	(60)	/* UI code force power off if above this level. */
#define BATT_TEMP_LEVEL_OVERHEAT	(55)	/* chargerlogo consider OVERHEAT if above this level. */
#define BATT_TEMP_LEVEL_COLD		(-10)	/* chargerlogo consider COLD if below this level. */
#define BATT_TEMP_LEVEL_RESUME_OVERHEAT	(42)	/* Added for New scenario 1.6 */
#define BATT_TEMP_LEVEL_RESUME_COLD	(-5)	/* Added for New scenario 1.6 */
enum {
	THERM_M10,
	THERM_M5,
	THERM_42,
	THERM_45,
	THERM_55,
	THERM_57,
	THERM_60,
	THERM_65,
	THERM_LAST,
};

enum {
	DISCHG_BATT_TEMP_OVER_60,
	DISCHG_BATT_TEMP_57_60,
	DISCHG_BATT_TEMP_UNDER_57,
	CHG_BATT_TEMP_LEVEL_1, // OVER_55
	CHG_BATT_TEMP_LEVEL_2, // 46_55
	CHG_BATT_TEMP_LEVEL_3, // 42_45
	CHG_BATT_TEMP_LEVEL_4, // M4_41
	CHG_BATT_TEMP_LEVEL_5, // M10_M5
	CHG_BATT_TEMP_LEVEL_6, // UNDER_M10
#if defined(CONFIG_MACH_APQ8064_J1SP)
	/* Add temp for charing scenario on SPRINT */
	CHG_BATT_TEMP_LEVEL_1_1,
#endif
};

enum {
	DISCHG_BATT_NORMAL_STATE,
	DISCHG_BATT_WARNING_STATE,
	DISCHG_BATT_POWEROFF_STATE,
	CHG_BATT_NORMAL_STATE,
	CHG_BATT_DC_CURRENT_STATE,
	CHG_BATT_WARNING_STATE,
	CHG_BATT_STOP_CHARGING_STATE,
};
#endif

/* user space parameter to limit usb current */
static unsigned int usb_max_current;
/*
 * usb_target_ma is used for wall charger
 * adaptive input current limiting only. Use
 * pm_iusbmax_get() to get current maximum usb current setting.
 */
static int usb_target_ma;
static int charging_disabled;
static int thermal_mitigation;

static struct pm8921_chg_chip *the_chip;

static struct pm8xxx_adc_arb_btm_param btm_config;

#ifdef CONFIG_LGE_WIRELESS_CHARGER
#undef	WIRELESS_CAYMAN
#ifdef	WIRELESS_CAYMAN
#define	WIRELESS_MAXIM_SOC			95
#define	WIRELESS_BMS_SOC			95
#else
#define	WIRELESS_MAXIM_SOC			99
#define	WIRELESS_BMS_SOC			99
#endif
#define	PM8921_GPIO_BASE			NR_GPIO_IRQS
#define	PM8921_GPIO_PM_TO_SYS(pm_gpio)		(pm_gpio - 1 + PM8921_GPIO_BASE)
#define	CHG_STAT				PM8921_GPIO_PM_TO_SYS(26)
#define	ACTIVE_N				PM8921_GPIO_PM_TO_SYS(25)
#define WLC_TS_CTRL_WCN				44
#define WLC_TS_CTRL_BCM				7
#define	WIRELESS_ON				0
#define	WIRELESS_OFF				1
#define	WIRELESS_INFORM_NORMAL_TIME		20000
#define	WIRELESS_INFORM_RAPID_TIME		5000
#define	WIRELESS_CHIP_CONTROL_TIME		3000
#define	WIRELESS_EARLY_EOC_WORK_TIME		2500
#define	WIRELESS_EARLY_SOURCE_TIME		1000
//#if defined(CONFIG_MACH_MSM8960_D1LU) || defined(CONFIG_MACH_MSM8960_D1LSK) || defined(CONFIG_MACH_MSM8960_D1LKT)
#define	WIRELESS_VIN_MIN			4500
//#elif defined(CONFIG_MACH_MSM8960_D1LV)
//#define	WIRELESS_VIN_MIN			4600
//#else
//#define	WIRELESS_VIN_MIN			4600
//#endif
#define	WIRELESS_IBAT_MAX			625
#define	WIRELESS_DECREASE_IBAT			325
#define	WIRELESS_RESUME_TOLERANCE 		(100)
#define	WIRELESS_ITERM				150
#define WIRELESS_INT_TO_UNPLUG_CHECK		2000
#define	WIRELESS_POLLING_DC_TIME		1000
#define	WIRELESS_UNPLUG_CHECK_TIME		1000
#define	WIRELESS_SRC_COUNT_LIMIT		5
#define WIRELESS_BATFET_BIT			BIT(3)
#define	WIRELESS_VDD_MAX			4350
#define	WIRELESS_VDET_LOW			4350

static	bool	wireless_charging;
static	bool	wireless_charge_done;
static	bool	wireless_source_toggle;
static	int	wireless_source_control_count;
static	int	wireless_inform_no_dc;
//static	bool	wireless_interrupt_enable=true;
//static	long	wireless_dc_time;
//static	long	wireless_on_time;

extern	int	wireless_backlight_state(void);
static	void	wireless_polling_dc_worker(struct work_struct *work);
static	bool	wireless_get_backlight_on(void);
//static	int	wireless_batfet_on(struct pm8921_chg_chip *chip, int enable);
static	int	wireless_pm_power_get_property(struct power_supply *psy);
static	void	wireless_reset_hw(int usb , struct pm8921_chg_chip *chip);
static	void	wireless_set(struct pm8921_chg_chip *chip);
static	void	wireless_reset(struct pm8921_chg_chip *chip);
static	void	wireless_current_set(struct pm8921_chg_chip *chip);
static	void	wireless_vinmin_set(struct pm8921_chg_chip *chip);
static	int	wireless_hw_init(struct pm8921_chg_chip *chip);
static	void	wireless_chip_control_worker(struct work_struct *work);
static	void	wireless_information(struct work_struct *work);
static	int	wireless_batt_status(void);
static	int	wireless_soc(struct pm8921_chg_chip * chip);
static	bool	wireless_soc_check(struct pm8921_chg_chip * chip);
static	bool	wireless_is_plugged(void);
static	void	wireless_unplug_ovp_fet_open(struct pm8921_chg_chip *chip);
static	void	wireless_unplug_check_worker(struct work_struct *work);
static	void	wireless_interrupt_probe(struct pm8921_chg_chip *chip);
static	int	wireless_is_charging_finished(struct pm8921_chg_chip *chip);
static	void	wireless_interrupt_worker(struct work_struct *work);
static	void	wireless_source_control_worker(struct work_struct *work);
static	void	turn_off_dc_ovp_fet(struct pm8921_chg_chip *chip);
static	void	turn_on_dc_ovp_fet(struct pm8921_chg_chip *chip);
static	bool	pm_is_chg_charge_dis_bit_set(struct pm8921_chg_chip *chip);
static 	void	dcin_valid_irq_worker(struct work_struct *work);
static	irqreturn_t	wireless_interrupt_handler(int irq, void *data);
static	irqreturn_t	dcin_valid_irq_handler(int irq, void *data);
#endif

#ifdef CONFIG_LGE_CHARGER_TEMP_SCENARIO
static unsigned int last_stop_charging;
static unsigned int chg_batt_temp_state;
/*
 * kiwone.seo@lge.com 2011-0609
 * for show charging ani.although charging is stopped : charging scenario
 */
static int pseudo_ui_charging;
static int last_usb_chg_current;
/* 120712 cs.kim@lge.com Implements Thermal Mitigation iUSB setting */
static int pre_mitigation;
static int soc_limit;
#endif
#ifdef CONFIG_LGE_WIRELESS_CHARGER
static int wlc_charging_status;
#endif

#ifdef CONFIG_LGE_PM
/* add for thermister test kwangjae1.lee@lge.com */
struct pseudo_batt_info_type pseudo_batt_info = {
        .mode = 0,
};
/* LGE_S kwangjae1.lee@lge.com 2012-06-11 Add bms debugger */
struct bms_batt_info_type bms_batt_info = {
        .mode = 0,
};
/* LGE_E kwangjae1.lee@lge.com 2012-06-11 Add bms debugger */
static int block_charging_state = 1; /* 1 : charing, 0 : block charging */
#endif

#ifdef CONFIG_LGE_PM_LOW_BATT_CHG
/* this is indicator that chargerlogo app is running. */
extern int chargerlogo_state;
#endif
#if defined (CONFIG_BATTERY_MAX17047) || defined (CONFIG_BATTERY_MAX17043)
int lge_power_test_flag = 0;
#endif
#if defined (CONFIG_BATTERY_MAX17047)
static int g_batt_soc = 0;
static int g_batt_vol = 0;
#define MAX17047_DELAY_WORK	(10 * HZ)
#define MAX17047_RESUME_TIME 		(1 * HZ)
#define FIRST_CHANGING_MODE_UPDATE_TIME  3000
#define CHG_COMPLETE_VOL 	4350
#define CHG_COMPLETE_TOLERANCE 	10

#endif
#if defined (CONFIG_BATTERY_MAX17047)
/*doosan.baek@lge.com 20121108 Add battery condition */
static int g_batt_age = 0;
static int pseudo_batt_age_mode = 0;
#endif
#ifdef CONFIG_LGE_PM_BATTERY_ID_CHECKER
extern int lge_battery_info;
#endif

#ifdef CONFIG_LGE_PM
struct wake_lock uevent_wake_lock;
#endif

static enum lge_boot_mode_type boot_mode;

#ifdef CONFIG_BATTERY_MAX17047
int max17047_get_soc(void)
{
	if( g_batt_soc == 0)
		g_batt_soc = max17047_get_battery_capacity_percent();

	return g_batt_soc;
}
int max17047_get_batt_vol(void)
{
	if( g_batt_vol == 0)
		g_batt_vol = max17047_get_battery_mvolts();

	return g_batt_vol;
}
#endif

#ifdef CONFIG_BATTERY_MAX17047
/*doosan.baek@lge.com 20121108 Add battery condition */
void lge_pm_battery_age_update(void)
{
	if (pseudo_batt_age_mode)
		return;

#if defined(CONFIG_MACH_APQ8064_GVDCM)
    if(lge_get_board_revno() > HW_REV_A)
    {
        g_batt_age = max17047_get_battery_age();
    }
    else
    {
        g_batt_age = 100;
    }
#else
    g_batt_age = 100;
#endif
    ChgLog("ChgLog> lge_pm_battery_age_update() : batt_age = %d \n", g_batt_age);
}
int lge_pm_get_battery_age(void)
{
    if ( g_batt_age == 0)
        lge_pm_battery_age_update();

    return g_batt_age;
}
int lge_pm_get_battery_condition(void)
{
    int batt_age = lge_pm_get_battery_age();
    int batt_condition = 0;

    if ( batt_age == 999)
	batt_condition = 0; //Error or Uncalculated Battery age.
    else if (batt_age >= 80)
        batt_condition = 1; //Very Good Condition
    else if (batt_age >= 50)
        batt_condition = 2; //Good Condition
    else if (batt_age >= 0)
        batt_condition = 3; //Bad Condition
    else
        batt_condition = 0; //Uncalculated Battery age.i

    ChgLog("ChgLog> lge_pm_get_battery_condition() :  batt_condition = %d, batt_age = %d \n",batt_condition, batt_age);

    return batt_condition;
}

static int get_bat_age(void *data, u64 * val)
{
	*val = g_batt_age;
	return 0;
}

static int set_bat_age(void *data, u64 val)
{
	int bat_age;

	bat_age = (int) val;
	if (bat_age == -1)
	{
		pseudo_batt_age_mode = 0;
		lge_pm_battery_age_update();
	}
	else
	{
		pseudo_batt_age_mode = 1;
		g_batt_age = bat_age;
	}

	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(bat_age_fops, get_bat_age, set_bat_age, "%llu\n");
#endif
static int pm_chg_masked_write(struct pm8921_chg_chip *chip, u16 addr,
							u8 mask, u8 val)
{
	int rc;
	u8 reg;

	rc = pm8xxx_readb(chip->dev->parent, addr, &reg);
	if (rc) {
		pr_err("pm8xxx_readb failed: addr=%03X, rc=%d\n", addr, rc);
		return rc;
	}
	reg &= ~mask;
	reg |= val & mask;
	rc = pm8xxx_writeb(chip->dev->parent, addr, reg);
	if (rc) {
		pr_err("pm8xxx_writeb failed: addr=%03X, rc=%d\n", addr, rc);
		return rc;
	}
	return 0;
}

#ifdef CONFIG_LGE_PM
static int get_prop_batt_temp(struct pm8921_chg_chip *chip);
#endif
static int pm_chg_get_rt_status(struct pm8921_chg_chip *chip, int irq_id)
{
#ifdef CONFIG_LGE_PM
	/* fake battery */
	if ((irq_id == BATTTEMP_COLD_IRQ) && (get_prop_batt_temp(chip) == -30))
		return 0;
#endif

	return pm8xxx_read_irq_stat(chip->dev->parent,
					chip->pmic_chg_irq[irq_id]);
}

/* Treat OverVoltage/UnderVoltage as source missing */
static int is_usb_chg_plugged_in(struct pm8921_chg_chip *chip)
{
	return pm_chg_get_rt_status(chip, USBIN_VALID_IRQ);
}

/* Treat OverVoltage/UnderVoltage as source missing */
static int is_dc_chg_plugged_in(struct pm8921_chg_chip *chip)
{
	return pm_chg_get_rt_status(chip, DCIN_VALID_IRQ);
}

static int is_batfet_closed(struct pm8921_chg_chip *chip)
{
	return pm_chg_get_rt_status(chip, BATFET_IRQ);
}

#ifdef CONFIG_LGE_PM
static bool is_factory_cable(void)
{
	acc_cable_type cable_type = lge_pm_get_cable_type();

	if (cable_type == CABLE_56K ||
		cable_type == CABLE_130K ||
		cable_type == CABLE_910K)
		return 1;
	else
		return 0;
}

/* jungwoo.yun@lge.com 2012-07-21 asked by mass production SW*/
static bool is_real_battery_or_factory_cable(struct pm8921_chg_chip *chip)
{
	if (pm_chg_get_rt_status(chip, BATT_INSERTED_IRQ))
		return 0;

	if (is_factory_cable())
		return 1;
	else
		return 0;
}
#endif

#define CAPTURE_FSM_STATE_CMD	0xC2
#define READ_BANK_7		0x70
#define READ_BANK_4		0x40
static int pm_chg_get_fsm_state(struct pm8921_chg_chip *chip)
{
	u8 temp;
	int err, ret = 0;

	temp = CAPTURE_FSM_STATE_CMD;
	err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
	if (err) {
		pr_err("Error %d writing %d to addr %d\n", err, temp, CHG_TEST);
		return err;
	}

	temp = READ_BANK_7;
	err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
	if (err) {
		pr_err("Error %d writing %d to addr %d\n", err, temp, CHG_TEST);
		return err;
	}

	err = pm8xxx_readb(chip->dev->parent, CHG_TEST, &temp);
	if (err) {
		pr_err("pm8xxx_readb fail: addr=%03X, rc=%d\n", CHG_TEST, err);
		return err;
	}
	/* get the lower 4 bits */
	ret = temp & 0xF;

	temp = READ_BANK_4;
	err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
	if (err) {
		pr_err("Error %d writing %d to addr %d\n", err, temp, CHG_TEST);
		return err;
	}

	err = pm8xxx_readb(chip->dev->parent, CHG_TEST, &temp);
	if (err) {
		pr_err("pm8xxx_readb fail: addr=%03X, rc=%d\n", CHG_TEST, err);
		return err;
	}
	/* get the upper 1 bit */
	ret |= (temp & 0x1) << 4;
	return  ret;
}

#define READ_BANK_6		0x60
static int pm_chg_get_regulation_loop(struct pm8921_chg_chip *chip)
{
	u8 temp;
	int err;

	temp = READ_BANK_6;
	err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
	if (err) {
		pr_err("Error %d writing %d to addr %d\n", err, temp, CHG_TEST);
		return err;
	}

	err = pm8xxx_readb(chip->dev->parent, CHG_TEST, &temp);
	if (err) {
		pr_err("pm8xxx_readb fail: addr=%03X, rc=%d\n", CHG_TEST, err);
		return err;
	}

	/* return the lower 4 bits */
	return temp & CHG_ALL_LOOPS;
}

#define CHG_USB_SUSPEND_BIT  BIT(2)
static int pm_chg_usb_suspend_enable(struct pm8921_chg_chip *chip, int enable)
{
	return pm_chg_masked_write(chip, CHG_CNTRL_3, CHG_USB_SUSPEND_BIT,
			enable ? CHG_USB_SUSPEND_BIT : 0);
}

#define CHG_EN_BIT	BIT(7)
static int pm_chg_auto_enable(struct pm8921_chg_chip *chip, int enable)
{
	ChgLog("ChgLog> %s : %d\n",__func__,enable);
	return pm_chg_masked_write(chip, CHG_CNTRL_3, CHG_EN_BIT,
				enable ? CHG_EN_BIT : 0);
}

#define CHG_FAILED_CLEAR	BIT(0)
#define ATC_FAILED_CLEAR	BIT(1)
static int pm_chg_failed_clear(struct pm8921_chg_chip *chip, int clear)
{
	int rc;

	rc = pm_chg_masked_write(chip, CHG_CNTRL_3, ATC_FAILED_CLEAR,
				clear ? ATC_FAILED_CLEAR : 0);
	rc |= pm_chg_masked_write(chip, CHG_CNTRL_3, CHG_FAILED_CLEAR,
				clear ? CHG_FAILED_CLEAR : 0);
	return rc;
}

#define CHG_CHARGE_DIS_BIT	BIT(1)
static int pm_chg_charge_dis(struct pm8921_chg_chip *chip, int disable)
{
	ChgLog("ChgLog> %s : %d\n",__func__,disable);
	return pm_chg_masked_write(chip, CHG_CNTRL, CHG_CHARGE_DIS_BIT,
				disable ? CHG_CHARGE_DIS_BIT : 0);
}

static int pm_is_chg_charge_dis(struct pm8921_chg_chip *chip)
{
	u8 temp;

	pm8xxx_readb(chip->dev->parent, CHG_CNTRL, &temp);
	return  temp & CHG_CHARGE_DIS_BIT;
}
#define PM8921_CHG_V_MIN_MV	3240
#define PM8921_CHG_V_STEP_MV	20
#define PM8921_CHG_V_STEP_10MV_OFFSET_BIT	BIT(7)
#define PM8921_CHG_VDDMAX_MAX	4500
#define PM8921_CHG_VDDMAX_MIN	3400
#define PM8921_CHG_V_MASK	0x7F
static int __pm_chg_vddmax_set(struct pm8921_chg_chip *chip, int voltage)
{
	int remainder;
	u8 temp = 0;

	if (voltage < PM8921_CHG_VDDMAX_MIN
			|| voltage > PM8921_CHG_VDDMAX_MAX) {
		pr_err("bad mV=%d asked to set\n", voltage);
		return -EINVAL;
	}

	temp = (voltage - PM8921_CHG_V_MIN_MV) / PM8921_CHG_V_STEP_MV;

	remainder = voltage % 20;
	if (remainder >= 10) {
		temp |= PM8921_CHG_V_STEP_10MV_OFFSET_BIT;
	}

	pr_debug("voltage=%d setting %02x\n", voltage, temp);
	return pm8xxx_writeb(chip->dev->parent, CHG_VDD_MAX, temp);
}

static int pm_chg_vddmax_get(struct pm8921_chg_chip *chip, int *voltage)
{
	u8 temp;
	int rc;

	rc = pm8xxx_readb(chip->dev->parent, CHG_VDD_MAX, &temp);
	if (rc) {
		pr_err("rc = %d while reading vdd max\n", rc);
		*voltage = 0;
		return rc;
	}
	*voltage = (int)(temp & PM8921_CHG_V_MASK) * PM8921_CHG_V_STEP_MV
							+ PM8921_CHG_V_MIN_MV;
	if (temp & PM8921_CHG_V_STEP_10MV_OFFSET_BIT)
		*voltage =  *voltage + 10;
	return 0;
}

static int pm_chg_vddmax_set(struct pm8921_chg_chip *chip, int voltage)
{
	int current_mv, ret, steps, i;
	bool increase;

	ret = 0;

	ChgLog("ChgLog> %s : %d\n",__func__,voltage);
	if (voltage < PM8921_CHG_VDDMAX_MIN
		|| voltage > PM8921_CHG_VDDMAX_MAX) {
		pr_err("bad mV=%d asked to set\n", voltage);
		return -EINVAL;
	}

	ret = pm_chg_vddmax_get(chip, &current_mv);
	if (ret) {
		pr_err("Failed to read vddmax rc=%d\n", ret);
		return -EINVAL;
	}
	if (current_mv == voltage)
		return 0;

	/* Only change in increments when USB is present */
#ifdef CONFIG_LGE_WIRELESS_CHARGER
	if (is_usb_chg_plugged_in(chip)||wireless_is_plugged()) {
#else
	if (is_usb_chg_plugged_in(chip)) {
#endif
		if (current_mv < voltage) {
			steps = (voltage - current_mv) / PM8921_CHG_V_STEP_MV;
			increase = true;
		} else {
			steps = (current_mv - voltage) / PM8921_CHG_V_STEP_MV;
			increase = false;
		}
		for (i = 0; i < steps; i++) {
			if (increase)
				current_mv += PM8921_CHG_V_STEP_MV;
			else
				current_mv -= PM8921_CHG_V_STEP_MV;
			ret |= __pm_chg_vddmax_set(chip, current_mv);
		}
	}
	ret |= __pm_chg_vddmax_set(chip, voltage);
	return ret;
}

#define PM8921_CHG_VDDSAFE_MIN	3400
#define PM8921_CHG_VDDSAFE_MAX	4500
static int pm_chg_vddsafe_set(struct pm8921_chg_chip *chip, int voltage)
{
	u8 temp;

	if (voltage < PM8921_CHG_VDDSAFE_MIN
			|| voltage > PM8921_CHG_VDDSAFE_MAX) {
		pr_err("bad mV=%d asked to set\n", voltage);
		return -EINVAL;
	}
	temp = (voltage - PM8921_CHG_V_MIN_MV) / PM8921_CHG_V_STEP_MV;
	pr_debug("voltage=%d setting %02x\n", voltage, temp);
	return pm_chg_masked_write(chip, CHG_VDD_SAFE, PM8921_CHG_V_MASK, temp);
}

#define PM8921_CHG_VBATDET_MIN	3240
#define PM8921_CHG_VBATDET_MAX	5780
static int pm_chg_vbatdet_set(struct pm8921_chg_chip *chip, int voltage)
{
	u8 temp;

	ChgLog("ChgLog> %s : %d\n",__func__,voltage);
	if (voltage < PM8921_CHG_VBATDET_MIN
			|| voltage > PM8921_CHG_VBATDET_MAX) {
		pr_err("bad mV=%d asked to set\n", voltage);
		return -EINVAL;
	}
	temp = (voltage - PM8921_CHG_V_MIN_MV) / PM8921_CHG_V_STEP_MV;
	pr_debug("voltage=%d setting %02x\n", voltage, temp);
	return pm_chg_masked_write(chip, CHG_VBAT_DET, PM8921_CHG_V_MASK, temp);
}

#define PM8921_CHG_VINMIN_MIN_MV	3800
#define PM8921_CHG_VINMIN_STEP_MV	100
#define PM8921_CHG_VINMIN_USABLE_MAX	6500
#define PM8921_CHG_VINMIN_USABLE_MIN	4300
#define PM8921_CHG_VINMIN_MASK		0x1F
static int pm_chg_vinmin_set(struct pm8921_chg_chip *chip, int voltage)
{
	u8 temp;

	if (voltage < PM8921_CHG_VINMIN_USABLE_MIN
			|| voltage > PM8921_CHG_VINMIN_USABLE_MAX) {
		pr_err("bad mV=%d asked to set\n", voltage);
		return -EINVAL;
	}
	temp = (voltage - PM8921_CHG_VINMIN_MIN_MV) / PM8921_CHG_VINMIN_STEP_MV;
	pr_debug("voltage=%d setting %02x\n", voltage, temp);
	return pm_chg_masked_write(chip, CHG_VIN_MIN, PM8921_CHG_VINMIN_MASK,
									temp);
}

static int pm_chg_vinmin_get(struct pm8921_chg_chip *chip)
{
	u8 temp;
	int rc, voltage_mv;

	rc = pm8xxx_readb(chip->dev->parent, CHG_VIN_MIN, &temp);
	temp &= PM8921_CHG_VINMIN_MASK;

	voltage_mv = PM8921_CHG_VINMIN_MIN_MV +
			(int)temp * PM8921_CHG_VINMIN_STEP_MV;

	return voltage_mv;
}

#define PM8917_USB_UVD_MIN_MV	3850
#define PM8917_USB_UVD_MAX_MV	4350
#define PM8917_USB_UVD_STEP_MV	100
#define PM8917_USB_UVD_MASK	0x7
static int pm_chg_uvd_threshold_set(struct pm8921_chg_chip *chip, int thresh_mv)
{
	u8 temp;

	if (thresh_mv < PM8917_USB_UVD_MIN_MV
			|| thresh_mv > PM8917_USB_UVD_MAX_MV) {
		pr_err("bad mV=%d asked to set\n", thresh_mv);
		return -EINVAL;
	}
	temp = (thresh_mv - PM8917_USB_UVD_MIN_MV) / PM8917_USB_UVD_STEP_MV;
	return pm_chg_masked_write(chip, OVP_USB_UVD,
				PM8917_USB_UVD_MASK, temp);
}

#define PM8921_CHG_IBATMAX_MIN	325
#define PM8921_CHG_IBATMAX_MAX	2000
#define PM8921_CHG_I_MIN_MA	225
#define PM8921_CHG_I_STEP_MA	50
#define PM8921_CHG_I_MASK	0x3F
static int pm_chg_ibatmax_set(struct pm8921_chg_chip *chip, int chg_current)
{
	u8 temp;

	ChgLog("ChgLog> %s : %d\n",__func__,chg_current);
	if (chg_current < PM8921_CHG_IBATMAX_MIN
			|| chg_current > PM8921_CHG_IBATMAX_MAX) {
		pr_err("bad mA=%d asked to set\n", chg_current);
		return -EINVAL;
	}
	temp = (chg_current - PM8921_CHG_I_MIN_MA) / PM8921_CHG_I_STEP_MA;
	return pm_chg_masked_write(chip, CHG_IBAT_MAX, PM8921_CHG_I_MASK, temp);
}

#define PM8921_CHG_IBATSAFE_MIN	225
#define PM8921_CHG_IBATSAFE_MAX	3375
static int pm_chg_ibatsafe_set(struct pm8921_chg_chip *chip, int chg_current)
{
	u8 temp;

	ChgLog("ChgLog> %s : %d\n",__func__,chg_current);

	if (chg_current < PM8921_CHG_IBATSAFE_MIN
			|| chg_current > PM8921_CHG_IBATSAFE_MAX) {
		pr_err("bad mA=%d asked to set\n", chg_current);
		return -EINVAL;
	}
	temp = (chg_current - PM8921_CHG_I_MIN_MA) / PM8921_CHG_I_STEP_MA;
	return pm_chg_masked_write(chip, CHG_IBAT_SAFE,
						PM8921_CHG_I_MASK, temp);
}

#define PM8921_CHG_ITERM_MIN_MA		50
#define PM8921_CHG_ITERM_MAX_MA		200
#define PM8921_CHG_ITERM_STEP_MA	10
#define PM8921_CHG_ITERM_MASK		0xF
static int pm_chg_iterm_set(struct pm8921_chg_chip *chip, int chg_current)
{
	u8 temp;

	ChgLog("ChgLog> %s : %d\n",__func__,chg_current);

	if (chg_current < PM8921_CHG_ITERM_MIN_MA
			|| chg_current > PM8921_CHG_ITERM_MAX_MA) {
		pr_err("bad mA=%d asked to set\n", chg_current);
		return -EINVAL;
	}

	temp = (chg_current - PM8921_CHG_ITERM_MIN_MA)
				/ PM8921_CHG_ITERM_STEP_MA;
	return pm_chg_masked_write(chip, CHG_ITERM, PM8921_CHG_ITERM_MASK,
					 temp);
}

static int pm_chg_iterm_get(struct pm8921_chg_chip *chip, int *chg_current)
{
	u8 temp;
	int rc;

	rc = pm8xxx_readb(chip->dev->parent, CHG_ITERM, &temp);
	if (rc) {
		pr_err("err=%d reading CHG_ITEM\n", rc);
		*chg_current = 0;
		return rc;
	}
	temp &= PM8921_CHG_ITERM_MASK;
	*chg_current = (int)temp * PM8921_CHG_ITERM_STEP_MA
					+ PM8921_CHG_ITERM_MIN_MA;
	return 0;
}

struct usb_ma_limit_entry {
	int	usb_ma;
	u8	value;
};

static struct usb_ma_limit_entry usb_ma_table[] = {
	{100, 0x0},
	{200, 0x1},
	{500, 0x2},
	{600, 0x3},
	{700, 0x4},
	{800, 0x5},
	{850, 0x6},
	{900, 0x8},
	{950, 0x7},
	{1000, 0x9},
	{1100, 0xA},
	{1200, 0xB},
	{1300, 0xC},
	{1400, 0xD},
	{1500, 0xE},
	{1600, 0xF},
};

#define PM8921_CHG_IUSB_MASK 0x1C
#define PM8921_CHG_IUSB_SHIFT 2
#define PM8921_CHG_IUSB_MAX  7
#define PM8921_CHG_IUSB_MIN  0
#define PM8917_IUSB_FINE_RES BIT(0)

#ifdef CONFIG_LGE_PM
#define PM8921_CHG_IBATT_FACTORY_CABLE 600
#endif

static int pm_chg_iusbmax_set(struct pm8921_chg_chip *chip, int reg_val)
{
	u8 temp, fineres;
	int rc;

	ChgLog("ChgLog> %s : reg_val: %d, iusb : %d mA\n",__func__,reg_val,usb_ma_table[reg_val].usb_ma);

	fineres = PM8917_IUSB_FINE_RES & usb_ma_table[reg_val].value;
	reg_val = usb_ma_table[reg_val].value >> 1;

	if (reg_val < PM8921_CHG_IUSB_MIN || reg_val > PM8921_CHG_IUSB_MAX) {
		pr_err("bad mA=%d asked to set\n", reg_val);
		return -EINVAL;
	}
	temp = reg_val << PM8921_CHG_IUSB_SHIFT;

	/* IUSB_FINE_RES */
	if (chip->iusb_fine_res) {
		/* Clear IUSB_FINE_RES bit to avoid overshoot */
		rc = pm_chg_masked_write(chip, IUSB_FINE_RES,
			PM8917_IUSB_FINE_RES, 0);

		rc |= pm_chg_masked_write(chip, PBL_ACCESS2,
			PM8921_CHG_IUSB_MASK, temp);

		if (rc) {
			pr_err("Failed to write PBL_ACCESS2 rc=%d\n", rc);
			return rc;
		}

		if (fineres) {
			rc = pm_chg_masked_write(chip, IUSB_FINE_RES,
				PM8917_IUSB_FINE_RES, fineres);
			if (rc)
				pr_err("Failed to write ISUB_FINE_RES rc=%d\n",
					rc);
		}
	} else {
		rc = pm_chg_masked_write(chip, PBL_ACCESS2,
			PM8921_CHG_IUSB_MASK, temp);
		if (rc)
			pr_err("Failed to write PBL_ACCESS2 rc=%d\n", rc);
	}

	return rc;
}

static int pm_chg_iusbmax_get(struct pm8921_chg_chip *chip, int *mA)
{
	u8 temp, fineres;
	int rc, i;

	fineres = 0;
	*mA = 0;
	rc = pm8xxx_readb(chip->dev->parent, PBL_ACCESS2, &temp);
	if (rc) {
		pr_err("err=%d reading PBL_ACCESS2\n", rc);
		return rc;
	}

	if (chip->iusb_fine_res) {
		rc = pm8xxx_readb(chip->dev->parent, IUSB_FINE_RES, &fineres);
		if (rc) {
			pr_err("err=%d reading IUSB_FINE_RES\n", rc);
			return rc;
		}
	}
	temp &= PM8921_CHG_IUSB_MASK;
	temp = temp >> PM8921_CHG_IUSB_SHIFT;

	temp = (temp << 1) | (fineres & PM8917_IUSB_FINE_RES);
	for (i = ARRAY_SIZE(usb_ma_table) - 1; i >= 0; i--) {
		if (usb_ma_table[i].value == temp)
			break;
	}

	*mA = usb_ma_table[i].usb_ma;

	return rc;
}

#define PM8921_CHG_WD_MASK 0x1F
static int pm_chg_disable_wd(struct pm8921_chg_chip *chip)
{
	/* writing 0 to the wd timer disables it */
	return pm_chg_masked_write(chip, CHG_TWDOG, PM8921_CHG_WD_MASK, 0);
}

#define PM8921_CHG_TCHG_MASK	0x7F
#define PM8921_CHG_TCHG_MIN	4
#define PM8921_CHG_TCHG_MAX	512
#define PM8921_CHG_TCHG_STEP	4
static int pm_chg_tchg_max_set(struct pm8921_chg_chip *chip, int minutes)
{
	u8 temp;

	ChgLog("ChgLog> %s : %d\n",__func__,minutes);

	if (minutes < PM8921_CHG_TCHG_MIN || minutes > PM8921_CHG_TCHG_MAX) {
		pr_err("bad max minutes =%d asked to set\n", minutes);
		return -EINVAL;
	}

	temp = (minutes - 1)/PM8921_CHG_TCHG_STEP;
	return pm_chg_masked_write(chip, CHG_TCHG_MAX, PM8921_CHG_TCHG_MASK,
					 temp);
}

#define PM8921_CHG_TTRKL_MASK	0x3F
#define PM8921_CHG_TTRKL_MIN	1
#define PM8921_CHG_TTRKL_MAX	64
static int pm_chg_ttrkl_max_set(struct pm8921_chg_chip *chip, int minutes)
{
	u8 temp;

	ChgLog("ChgLog> %s : %d\n",__func__,minutes);

	if (minutes < PM8921_CHG_TTRKL_MIN || minutes > PM8921_CHG_TTRKL_MAX) {
		pr_err("bad max minutes =%d asked to set\n", minutes);
		return -EINVAL;
	}

	temp = minutes - 1;
	return pm_chg_masked_write(chip, CHG_TTRKL_MAX, PM8921_CHG_TTRKL_MASK,
					 temp);
}

#define PM8921_CHG_VTRKL_MIN_MV		2050
#define PM8921_CHG_VTRKL_MAX_MV		2800
#define PM8921_CHG_VTRKL_STEP_MV	50
#define PM8921_CHG_VTRKL_SHIFT		4
#define PM8921_CHG_VTRKL_MASK		0xF0
static int pm_chg_vtrkl_low_set(struct pm8921_chg_chip *chip, int millivolts)
{
	u8 temp;

	ChgLog("ChgLog> %s : %d\n",__func__,millivolts);

	if (millivolts < PM8921_CHG_VTRKL_MIN_MV
			|| millivolts > PM8921_CHG_VTRKL_MAX_MV) {
		pr_err("bad voltage = %dmV asked to set\n", millivolts);
		return -EINVAL;
	}

	temp = (millivolts - PM8921_CHG_VTRKL_MIN_MV)/PM8921_CHG_VTRKL_STEP_MV;
	temp = temp << PM8921_CHG_VTRKL_SHIFT;
	return pm_chg_masked_write(chip, CHG_VTRICKLE, PM8921_CHG_VTRKL_MASK,
					 temp);
}

#define PM8921_CHG_VWEAK_MIN_MV		2100
#define PM8921_CHG_VWEAK_MAX_MV		3600
#define PM8921_CHG_VWEAK_STEP_MV	100
#define PM8921_CHG_VWEAK_MASK		0x0F
static int pm_chg_vweak_set(struct pm8921_chg_chip *chip, int millivolts)
{
	u8 temp;

	ChgLog("ChgLog> %s : %d\n",__func__,millivolts);

	if (millivolts < PM8921_CHG_VWEAK_MIN_MV
			|| millivolts > PM8921_CHG_VWEAK_MAX_MV) {
		pr_err("bad voltage = %dmV asked to set\n", millivolts);
		return -EINVAL;
	}

	temp = (millivolts - PM8921_CHG_VWEAK_MIN_MV)/PM8921_CHG_VWEAK_STEP_MV;
	return pm_chg_masked_write(chip, CHG_VTRICKLE, PM8921_CHG_VWEAK_MASK,
					 temp);
}

#define PM8921_CHG_ITRKL_MIN_MA		50
#define PM8921_CHG_ITRKL_MAX_MA		200
#define PM8921_CHG_ITRKL_MASK		0x0F
#define PM8921_CHG_ITRKL_STEP_MA	10
static int pm_chg_itrkl_set(struct pm8921_chg_chip *chip, int milliamps)
{
	u8 temp;

	ChgLog("ChgLog> %s : %d\n",__func__,milliamps);

	if (milliamps < PM8921_CHG_ITRKL_MIN_MA
		|| milliamps > PM8921_CHG_ITRKL_MAX_MA) {
		pr_err("bad current = %dmA asked to set\n", milliamps);
		return -EINVAL;
	}

	temp = (milliamps - PM8921_CHG_ITRKL_MIN_MA)/PM8921_CHG_ITRKL_STEP_MA;

	return pm_chg_masked_write(chip, CHG_ITRICKLE, PM8921_CHG_ITRKL_MASK,
					 temp);
}

#define PM8921_CHG_IWEAK_MIN_MA		325
#define PM8921_CHG_IWEAK_MAX_MA		525
#define PM8921_CHG_IWEAK_SHIFT		7
#define PM8921_CHG_IWEAK_MASK		0x80
static int pm_chg_iweak_set(struct pm8921_chg_chip *chip, int milliamps)
{
	u8 temp;

	ChgLog("ChgLog> %s : %d\n",__func__,milliamps);

	if (milliamps < PM8921_CHG_IWEAK_MIN_MA
		|| milliamps > PM8921_CHG_IWEAK_MAX_MA) {
		pr_err("bad current = %dmA asked to set\n", milliamps);
		return -EINVAL;
	}

	if (milliamps < PM8921_CHG_IWEAK_MAX_MA)
		temp = 0;
	else
		temp = 1;

	temp = temp << PM8921_CHG_IWEAK_SHIFT;
	return pm_chg_masked_write(chip, CHG_ITRICKLE, PM8921_CHG_IWEAK_MASK,
					 temp);
}

#define PM8921_CHG_BATT_TEMP_THR_COLD	BIT(1)
#define PM8921_CHG_BATT_TEMP_THR_COLD_SHIFT	1
static int pm_chg_batt_cold_temp_config(struct pm8921_chg_chip *chip,
					enum pm8921_chg_cold_thr cold_thr)
{
	u8 temp;

	temp = cold_thr << PM8921_CHG_BATT_TEMP_THR_COLD_SHIFT;
	temp = temp & PM8921_CHG_BATT_TEMP_THR_COLD;
	return pm_chg_masked_write(chip, CHG_CNTRL_2,
					PM8921_CHG_BATT_TEMP_THR_COLD,
					 temp);
}

#define PM8921_CHG_BATT_TEMP_THR_HOT		BIT(0)
#define PM8921_CHG_BATT_TEMP_THR_HOT_SHIFT	0
static int pm_chg_batt_hot_temp_config(struct pm8921_chg_chip *chip,
					enum pm8921_chg_hot_thr hot_thr)
{
	u8 temp;

	temp = hot_thr << PM8921_CHG_BATT_TEMP_THR_HOT_SHIFT;
	temp = temp & PM8921_CHG_BATT_TEMP_THR_HOT;
	return pm_chg_masked_write(chip, CHG_CNTRL_2,
					PM8921_CHG_BATT_TEMP_THR_HOT,
					 temp);
}

#define PM8921_CHG_LED_SRC_CONFIG_SHIFT	4
#define PM8921_CHG_LED_SRC_CONFIG_MASK	0x30
static int pm_chg_led_src_config(struct pm8921_chg_chip *chip,
				enum pm8921_chg_led_src_config led_src_config)
{
	u8 temp;

	if (led_src_config < LED_SRC_GND ||
			led_src_config > LED_SRC_BYPASS)
		return -EINVAL;

	if (led_src_config == LED_SRC_BYPASS)
		return 0;

	temp = led_src_config << PM8921_CHG_LED_SRC_CONFIG_SHIFT;

	return pm_chg_masked_write(chip, CHG_CNTRL_3,
					PM8921_CHG_LED_SRC_CONFIG_MASK, temp);
}

#ifdef CONFIG_LGE_PM
#define CHG_BATFET_ON  BIT(3)
/* TRUE : forcing CLOSE, FALSE : depend on PMIC FSM */
static int pm_chg_batfet_set(struct pm8921_chg_chip *chip, int on)
{
	int rc;

	pr_info("set CHG_BATFET_ON=%d\n", on);
	rc = pm_chg_masked_write(chip, CHG_CNTRL, CHG_BATFET_ON, on?CHG_BATFET_ON:0);
	return rc;
}
static int pm_chg_batfet_get(struct pm8921_chg_chip *chip)
{
	int rc;
	u8 temp;

	rc = pm8xxx_readb(chip->dev->parent, CHG_CNTRL, &temp);
	if (rc) {
		pr_err("pm8xxx_readb to %x value =%d errored = %d\n", CHG_CNTRL, temp, rc);
		return -EAGAIN;
	}
	pr_info("get CHG_BATFET_ON=%d\n", !!(temp & CHG_BATFET_ON));
	return !!(temp & CHG_BATFET_ON);
}
int pm8921_chg_batfet_set_ext(int on)
{
	if(the_chip==NULL) return -EAGAIN;
	return pm_chg_batfet_set(the_chip, on);
}
int pm8921_chg_batfet_get_ext(void)
{
	if(the_chip==NULL) return -EAGAIN;
	return pm_chg_batfet_get(the_chip);
}
#endif

#ifndef CONFIG_LGE_PM_ADP_CHG
/* Revert USB unplug detection to 1043 - seonghun1.kim */
static void disable_input_voltage_regulation(struct pm8921_chg_chip *chip)
{
	u8 temp;
	int rc;

	rc = pm8xxx_writeb(chip->dev->parent, CHG_BUCK_CTRL_TEST3, 0x70);
	if (rc) {
		pr_err("Failed to write 0x70 to CTRL_TEST3 rc = %d\n", rc);
		return;
	}
	rc = pm8xxx_readb(chip->dev->parent, CHG_BUCK_CTRL_TEST3, &temp);
	if (rc) {
		pr_err("Failed to read CTRL_TEST3 rc = %d\n", rc);
		return;
	}
	/* set the input voltage disable bit and the write bit */
	temp |= 0x81;
	rc = pm8xxx_writeb(chip->dev->parent, CHG_BUCK_CTRL_TEST3, temp);
	if (rc) {
		pr_err("Failed to write 0x%x to CTRL_TEST3 rc=%d\n", temp, rc);
		return;
	}
}

static void enable_input_voltage_regulation(struct pm8921_chg_chip *chip)
{
	u8 temp;
	int rc;

	rc = pm8xxx_writeb(chip->dev->parent, CHG_BUCK_CTRL_TEST3, 0x70);
	if (rc) {
		pr_err("Failed to write 0x70 to CTRL_TEST3 rc = %d\n", rc);
		return;
	}
	rc = pm8xxx_readb(chip->dev->parent, CHG_BUCK_CTRL_TEST3, &temp);
	if (rc) {
		pr_err("Failed to read CTRL_TEST3 rc = %d\n", rc);
		return;
	}
	/* unset the input voltage disable bit */
	temp &= 0xFE;
	/* set the write bit */
	temp |= 0x80;
	rc = pm8xxx_writeb(chip->dev->parent, CHG_BUCK_CTRL_TEST3, temp);
	if (rc) {
		pr_err("Failed to write 0x%x to CTRL_TEST3 rc=%d\n", temp, rc);
		return;
	}
}
#endif

#ifndef CONFIG_LGE_PM_BATTERY_ID_CHECKER
static int64_t read_battery_id(struct pm8921_chg_chip *chip)
{
	int rc;
	struct pm8xxx_adc_chan_result result;

	rc = pm8xxx_adc_read(chip->batt_id_channel, &result);
	if (rc) {
		pr_err("error reading batt id channel = %d, rc = %d\n",
					chip->vbat_channel, rc);
		return rc;
	}
	pr_debug("batt_id phy = %lld meas = 0x%llx\n", result.physical,
						result.measurement);
	return result.physical;
}
#endif

static int is_battery_valid(struct pm8921_chg_chip *chip)
{
#ifndef CONFIG_LGE_PM_BATTERY_ID_CHECKER
	int64_t rc;
#endif

#ifdef CONFIG_LGE_PM
	if(pseudo_batt_info.mode == 1)
		return 1;

	boot_mode = lge_get_boot_mode();
	if( (boot_mode == LGE_BOOT_MODE_FACTORY)    ||
	    (boot_mode == LGE_BOOT_MODE_FACTORY2)   ||
	    (boot_mode == LGE_BOOT_MODE_PIFBOOT)    ||
	    (boot_mode == LGE_BOOT_MODE_PIFBOOT2) ) 
		return 1;
#endif

#ifdef CONFIG_LGE_PM_BATTERY_ID_CHECKER
	if (lge_battery_info == BATT_ID_DS2704_N || lge_battery_info == BATT_ID_DS2704_L ||
		lge_battery_info == BATT_ID_ISL6296_N || lge_battery_info == BATT_ID_ISL6296_L ||
		lge_battery_info == BATT_ID_DS2704_C || lge_battery_info == BATT_ID_ISL6296_C)
		return 1;
	else
		return 0;
#else
	if (chip->batt_id_min == 0 && chip->batt_id_max == 0)
		return 1;

	rc = read_battery_id(chip);
	if (rc < 0) {
		pr_err("error reading batt id channel = %d, rc = %lld\n",
					chip->vbat_channel, rc);
		/* assume battery id is valid when adc error happens */
		return 1;
	}

	if (rc < chip->batt_id_min || rc > chip->batt_id_max) {
		pr_err("batt_id phy =%lld is not valid\n", rc);
		return 0;
	}
	return 1;
#endif
}

static void check_battery_valid(struct pm8921_chg_chip *chip)
{
	if (is_battery_valid(chip) == 0) {
		pr_err("batt_id not valid, disbling charging\n");
		pm_chg_auto_enable(chip, 0);
	} else {
		pm_chg_auto_enable(chip, !charging_disabled);
	}
}

static void battery_id_valid(struct work_struct *work)
{
	struct pm8921_chg_chip *chip = container_of(work,
				struct pm8921_chg_chip, battery_id_valid_work);

	check_battery_valid(chip);
}

static void pm8921_chg_enable_irq(struct pm8921_chg_chip *chip, int interrupt)
{
	if (!__test_and_set_bit(interrupt, chip->enabled_irqs)) {
		dev_dbg(chip->dev, "%d\n", chip->pmic_chg_irq[interrupt]);
		enable_irq(chip->pmic_chg_irq[interrupt]);
	}
}

static void pm8921_chg_disable_irq(struct pm8921_chg_chip *chip, int interrupt)
{
	if (__test_and_clear_bit(interrupt, chip->enabled_irqs)) {
		dev_dbg(chip->dev, "%d\n", chip->pmic_chg_irq[interrupt]);
		disable_irq_nosync(chip->pmic_chg_irq[interrupt]);
	}
}

static int pm8921_chg_is_enabled(struct pm8921_chg_chip *chip, int interrupt)
{
	return test_bit(interrupt, chip->enabled_irqs);
}

static bool is_ext_charging(struct pm8921_chg_chip *chip)
{
	union power_supply_propval ret = {0,};

	if (!chip->ext_psy)
		return false;
	if (chip->ext_psy->get_property(chip->ext_psy,
					POWER_SUPPLY_PROP_CHARGE_TYPE, &ret))
		return false;
	if (ret.intval > POWER_SUPPLY_CHARGE_TYPE_NONE)
		return ret.intval;

	return false;
}

static bool is_ext_trickle_charging(struct pm8921_chg_chip *chip)
{
	union power_supply_propval ret = {0,};

	if (!chip->ext_psy)
		return false;
	if (chip->ext_psy->get_property(chip->ext_psy,
					POWER_SUPPLY_PROP_CHARGE_TYPE, &ret))
		return false;
	if (ret.intval == POWER_SUPPLY_CHARGE_TYPE_TRICKLE)
		return true;

	return false;
}

static int is_battery_charging(int fsm_state)
{
	if (is_ext_charging(the_chip))
		return 1;

	switch (fsm_state) {
	case FSM_STATE_ATC_2A:
	case FSM_STATE_ATC_2B:
	case FSM_STATE_ON_CHG_AND_BAT_6:
	case FSM_STATE_FAST_CHG_7:
	case FSM_STATE_TRKL_CHG_8:
		return 1;
	}
	return 0;
}

static void bms_notify(struct work_struct *work)
{
	struct bms_notify *n = container_of(work, struct bms_notify, work);

	if (n->is_charging) {
		pm8921_bms_charging_began();
	} else {
		pm8921_bms_charging_end(n->is_battery_full);
		n->is_battery_full = 0;
	}
}

static void bms_notify_check(struct pm8921_chg_chip *chip)
{
	int fsm_state, new_is_charging;

	fsm_state = pm_chg_get_fsm_state(chip);
	new_is_charging = is_battery_charging(fsm_state);

	if (chip->bms_notify.is_charging ^ new_is_charging) {
		chip->bms_notify.is_charging = new_is_charging;
		schedule_work(&(chip->bms_notify.work));
	}
}

static enum power_supply_property pm_power_props_usb[] = {
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_CURRENT_MAX,
	POWER_SUPPLY_PROP_SCOPE,
};

static enum power_supply_property pm_power_props_mains[] = {
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
};

#ifdef CONFIG_LGE_WIRELESS_CHARGER
static enum power_supply_property pm_power_props_wireless[] = {
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
#ifdef CONFIG_LGE_FTT_CHARGER
	POWER_SUPPLY_PROP_FTT_ANNTENA_LEVEL,
#endif
};
#endif

static char *pm_power_supplied_to[] = {
	"battery",
};

#ifdef CONFIG_LGE_WIRELESS_CHARGER
static int pm_power_get_property_wireless(struct power_supply *psy,enum power_supply_property psp,union power_supply_propval *val)
{
	/* Check if called before init */
	if (!the_chip)
		return -EINVAL;

	switch (psp) {
	case POWER_SUPPLY_PROP_PRESENT:
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = 0;
		if (charging_disabled)
			return 0;

		val->intval = wireless_pm_power_get_property(psy);

		break;
#ifdef CONFIG_LGE_FTT_CHARGER
	case POWER_SUPPLY_PROP_FTT_ANNTENA_LEVEL:
		val->intval = -1;
		if (charging_disabled)
			return 0;

		val->intval = get_ftt_ant_level();
		break;
#endif
	default:
		return -EINVAL;
	}
	return 0;
}
#endif

#ifdef CONFIG_LGE_PM
#if defined(CONFIG_MACH_APQ8064_GKGLOBAL) && !defined(CONFIG_MACH_APQ8064_GKTCLMX)
#define USB_WALL_THRESHOLD_MA	1100
#else
#define USB_WALL_THRESHOLD_MA	1500
#endif
#else
#define USB_WALL_THRESHOLD_MA	500
#endif
static int pm_power_get_property_mains(struct power_supply *psy,
				  enum power_supply_property psp,
				  union power_supply_propval *val)
{
	/* Check if called before init */
	if (!the_chip)
		return -EINVAL;

	switch (psp) {
	case POWER_SUPPLY_PROP_PRESENT:
#ifdef CONFIG_LGE_PM
		/* make separated case, 2012.7.24, hayoung.lee@ */
		val->intval = 0;
		if ( the_chip->usb_psy.type == POWER_SUPPLY_TYPE_MAINS ||
			 the_chip->usb_psy.type == POWER_SUPPLY_TYPE_USB_DCP )
			val->intval = is_usb_chg_plugged_in(the_chip);
		return 0;
#endif

	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = 0;

		if (the_chip->has_dc_supply) {
			val->intval = 1;
			return 0;
		}

		if (charging_disabled)
			return 0;

		/* check external charger first before the dc path */
		if (is_ext_charging(the_chip)) {
			val->intval = 1;
			return 0;
		}

		if (pm_is_chg_charge_dis(the_chip)) {
			val->intval = 0;
			return 0;
		}

#ifndef CONFIG_LGE_WIRELESS_CHARGER
		if (the_chip->dc_present) {
			val->intval = 1;
			return 0;
		}
#endif

#ifdef CONFIG_LGE_PM
		/* Decision AC(TA) charger */
		if ( the_chip->usb_psy.type == POWER_SUPPLY_TYPE_MAINS ||
		     the_chip->usb_psy.type == POWER_SUPPLY_TYPE_USB_DCP )
#else
		/* USB with max current greater than 500 mA connected */
		if (usb_target_ma > USB_WALL_THRESHOLD_MA)
#endif
			val->intval = is_usb_chg_plugged_in(the_chip);
			return 0;

		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int switch_usb_to_charge_mode(struct pm8921_chg_chip *chip)
{
	int rc;

	if (!chip->host_mode)
		return 0;

	/* enable usbin valid comparator and remove force usb ovp fet off */
	rc = pm8xxx_writeb(chip->dev->parent, USB_OVP_TEST, 0xB2);
	if (rc < 0) {
		pr_err("Failed to write 0xB2 to USB_OVP_TEST rc = %d\n", rc);
		return rc;
	}

	chip->host_mode = 0;

	return 0;
}

static int switch_usb_to_host_mode(struct pm8921_chg_chip *chip)
{
	int rc;

	if (chip->host_mode)
		return 0;

	/* disable usbin valid comparator and force usb ovp fet off */
	rc = pm8xxx_writeb(chip->dev->parent, USB_OVP_TEST, 0xB3);
	if (rc < 0) {
		pr_err("Failed to write 0xB3 to USB_OVP_TEST rc = %d\n", rc);
		return rc;
	}

	chip->host_mode = 1;

	return 0;
}

static int pm_power_set_property_usb(struct power_supply *psy,
				  enum power_supply_property psp,
				  const union power_supply_propval *val)
{
	/* Check if called before init */
	if (!the_chip)
		return -EINVAL;

	switch (psp) {
	case POWER_SUPPLY_PROP_SCOPE:
		if (val->intval == POWER_SUPPLY_SCOPE_SYSTEM)
			return switch_usb_to_host_mode(the_chip);
		if (val->intval == POWER_SUPPLY_SCOPE_DEVICE)
			return switch_usb_to_charge_mode(the_chip);
		else
			return -EINVAL;
		break;
	case POWER_SUPPLY_PROP_TYPE:
		return pm8921_set_usb_power_supply_type(val->intval);
	default:
		return -EINVAL;
	}
	return 0;
}

static int pm_power_get_property_usb(struct power_supply *psy,
				  enum power_supply_property psp,
				  union power_supply_propval *val)
{
	int current_max;

	/* Check if called before init */
	if (!the_chip)
		return -EINVAL;

	switch (psp) {
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		if (pm_is_chg_charge_dis(the_chip)) {
			val->intval = 0;
		} else {
			pm_chg_iusbmax_get(the_chip, &current_max);
			val->intval = current_max;
		}
		break;
	case POWER_SUPPLY_PROP_PRESENT:
#ifdef CONFIG_LGE_PM
		/* make separated case, 2012.7.24, hayoung.lee@ */
		val->intval = 0;
		if ( the_chip->usb_psy.type == POWER_SUPPLY_TYPE_USB ||
			 the_chip->usb_psy.type == POWER_SUPPLY_TYPE_USB_CDP ||
			 the_chip->usb_psy.type == POWER_SUPPLY_TYPE_USB_ACA )
			val->intval = is_usb_chg_plugged_in(the_chip);
		return 0;
#endif
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = 0;
		if (charging_disabled)
			return 0;

		/*
		 * if drawing any current from usb is disabled behave
		 * as if no usb cable is connected
		 */
		if (pm_is_chg_charge_dis(the_chip))
			return 0;

#ifdef CONFIG_LGE_PM
		/* Decision USB charger */
		if ( the_chip->usb_psy.type == POWER_SUPPLY_TYPE_USB ||
		     the_chip->usb_psy.type == POWER_SUPPLY_TYPE_USB_CDP ||
		     the_chip->usb_psy.type == POWER_SUPPLY_TYPE_USB_ACA )
#else
		/* USB charging */
		if (usb_target_ma < USB_WALL_THRESHOLD_MA)
#endif
			val->intval = is_usb_chg_plugged_in(the_chip);
		else
		    return 0;
		break;

	case POWER_SUPPLY_PROP_SCOPE:
		if (the_chip->host_mode)
			val->intval = POWER_SUPPLY_SCOPE_SYSTEM;
		else
			val->intval = POWER_SUPPLY_SCOPE_DEVICE;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static enum power_supply_property msm_batt_power_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_CURRENT_MAX,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_ENERGY_FULL,
#ifdef CONFIG_LGE_PM
	POWER_SUPPLY_PROP_PSEUDO_BATT,
	POWER_SUPPLY_PROP_BLOCK_CHARGING,
	POWER_SUPPLY_PROP_EXT_PWR_CHECK,
	/* kwangjae1.lee@lge.com 2012-06-11 Add bms debugger */
	POWER_SUPPLY_PROP_BMS_BATT,
	/* Add battery present check in the testmode */
	POWER_SUPPLY_PROP_REAL_BATT_PRESENT,
#ifdef CONFIG_BATTERY_MAX17047
/*doosan.baek@lge.com 20121108 Add battery condition */
        POWER_SUPPLY_PROP_BATTERY_CONDITION,
        POWER_SUPPLY_PROP_BATTERY_AGE,
#endif
#endif
#ifdef CONFIG_LGE_PM_BATTERY_ID_CHECKER
	POWER_SUPPLY_PROP_BATTERY_ID_CHECK,
#endif
};

#ifdef CONFIG_BATTERY_MAX17043
void pm8921_charger_force_update_batt_psy(void)
{
	struct pm8921_chg_chip *chip;

	if (!the_chip) {
		pr_err("called before init\n");
		return;
	}

	chip = the_chip;

	power_supply_changed(&chip->batt_psy);
}
EXPORT_SYMBOL_GPL(pm8921_charger_force_update_batt_psy);

static int max17043_get_capacity(struct pm8921_chg_chip *chip)
{
	int percent_soc;

	percent_soc = __max17043_get_capacity();

	if (percent_soc <= 10)
		pr_warn("low battery charge = %d%%\n", percent_soc);

#ifdef CONFIG_LGE_PM
	if (is_real_battery_or_factory_cable(chip)) {
		pr_debug("is_real_battery_or_factory_cable=enable percent_soc = 75, real_percent_soc = %d\n",percent_soc);
		percent_soc = 75;
	}
#endif

	chip->recent_reported_soc = percent_soc;
	return percent_soc;
}

static int max17043_get_voltage(struct pm8921_chg_chip *chip)
{
#ifdef CONFIG_LGE_PM
	if (is_real_battery_or_factory_cable(chip))
	{
		pr_debug("is_real_battery_or_factory_cable=enable fake_volt=3990\n");
		return 3990*1000;
	}
#endif

	return __max17043_get_voltage();
}
#endif

static int get_prop_battery_uvolts(struct pm8921_chg_chip *chip)
{
	int rc;
	struct pm8xxx_adc_chan_result result;

	rc = pm8xxx_adc_read(chip->vbat_channel, &result);
	if (rc) {
		pr_err("error reading adc channel = %d, rc = %d\n",
					chip->vbat_channel, rc);
		return rc;
	}

#ifdef CONFIG_LGE_PM
	if (pseudo_batt_info.mode == 1) {
		pr_debug("pseudo_batt_mode=enable fake_volt= 3990 real mvolts phy = %lld real meas = 0x%llx\n",
			result.physical, result.measurement);
		return 3990*1000;
	}
	else if (is_real_battery_or_factory_cable(chip))
	{
		pr_debug("is_real_battery_or_factory_cable=enable fake_volt=3990 real mvolts phy = %lld real meas = 0x%llx\n",
			 result.physical, result.measurement);
		return 3990*1000;
	}
	else {
		pr_debug("mvolts phy = %lld meas = 0x%llx\n", result.physical,
						result.measurement);
	return (int)result.physical;
}
#else
	pr_debug("mvolts phy = %lld meas = 0x%llx\n", result.physical,
					result.measurement);
	return (int)result.physical;
#endif
}

#ifndef CONFIG_BATTERY_MAX17043
static unsigned int voltage_based_capacity(struct pm8921_chg_chip *chip)
{
	unsigned int current_voltage_uv = get_prop_battery_uvolts(chip);
	unsigned int current_voltage_mv = current_voltage_uv / 1000;
	unsigned int low_voltage = chip->min_voltage_mv;
	unsigned int high_voltage = chip->max_voltage_mv;

	if (current_voltage_mv <= low_voltage)
		return 0;
	else if (current_voltage_mv >= high_voltage)
		return 100;
	else
		return (current_voltage_mv - low_voltage) * 100
		    / (high_voltage - low_voltage);
}
#endif
static int get_prop_batt_present(struct pm8921_chg_chip *chip)
{
#ifdef CONFIG_LGE_PM
	if(pseudo_batt_info.mode == 1)
		return 1;
	else if (is_factory_cable())
		return 1;
#endif

   return pm_chg_get_rt_status(chip, BATT_INSERTED_IRQ);
}
#ifndef CONFIG_BATTERY_MAX17043
static int get_prop_batt_capacity(struct pm8921_chg_chip *chip)
{
	int percent_soc;
#ifdef CONFIG_LGE_PM
	int is_fastchg;

	is_fastchg = pm_chg_get_rt_status(chip, FASTCHG_IRQ);
#endif

	if (!get_prop_batt_present(chip))
		percent_soc = voltage_based_capacity(chip);
	else
		percent_soc = pm8921_bms_get_percent_charge();

	if (percent_soc == -ENXIO)
		percent_soc = voltage_based_capacity(chip);

	if (percent_soc <= 10)
		pr_warn_ratelimited("low battery charge = %d%%\n",
						percent_soc);

#ifdef CONFIG_LGE_PM
	if ( !is_fastchg &&
		 chip->recent_reported_soc == (chip->resume_charge_percent + 1) &&
		 percent_soc <= chip->resume_charge_percent)
#else
	if (chip->recent_reported_soc == (chip->resume_charge_percent + 1)
			&& percent_soc == chip->resume_charge_percent)
#endif
	{
		pr_info("soc fell below %d. charging enabled.\n",
						chip->resume_charge_percent);
		if (chip->is_bat_warm)
			pr_warn_ratelimited("battery is warm = %d, do not resume charging at %d%%.\n",
					chip->is_bat_warm,
					chip->resume_charge_percent);
		else if (chip->is_bat_cool)
			pr_warn_ratelimited("battery is cool = %d, do not resume charging at %d%%.\n",
					chip->is_bat_cool,
					chip->resume_charge_percent);
		else
			pm_chg_vbatdet_set(the_chip, PM8921_CHG_VBATDET_MAX);
	}

#ifdef CONFIG_LGE_PM
	if (pseudo_batt_info.mode == 1) {
		pr_debug("pseudo_batt_mode=enable percent_soc = 75, real_percent_soc = %d\n",percent_soc);
		if( percent_soc > 1 )/* Order from MST test */
			percent_soc = 75;
	}
	else if (is_real_battery_or_factory_cable(chip)) {
		pr_debug("is_real_battery_or_factory_cable=enable percent_soc = 75, real_percent_soc = %d\n",percent_soc);
		percent_soc = 75;
	}
#endif

	chip->recent_reported_soc = percent_soc;
	return percent_soc;
}
#endif

static int get_prop_batt_current_max(struct pm8921_chg_chip *chip)
{
	int rbatt, ibatt_ua, vbatt_uv, ocv_uv;
	int imax_ma;
	int rc;

	rbatt = pm8921_bms_get_rbatt();

	if (rbatt < 0) {
		rc = -ENXIO;
		return rc;
	}

	rc =  pm8921_bms_get_simultaneous_battery_voltage_and_current
			(&ibatt_ua, &vbatt_uv);

	if (rc)
		return rc;

	ocv_uv = vbatt_uv + ibatt_ua*rbatt/1000;

	imax_ma = (ocv_uv - chip->min_voltage_mv*1000)/rbatt;

	if (imax_ma < 0)
		imax_ma = 0;

	return imax_ma*1000;

}

static int get_prop_batt_current(struct pm8921_chg_chip *chip)
{
	int result_ua, rc;

	rc = pm8921_bms_get_battery_current(&result_ua);
	if (rc == -ENXIO) {
		rc = pm8xxx_ccadc_get_battery_current(&result_ua);
	}

	if (rc) {
		pr_err("unable to get batt current rc = %d\n", rc);
		return rc;
	} else {
		return result_ua;
	}
}

static int get_prop_batt_fcc(struct pm8921_chg_chip *chip)
{
	int rc;

	rc = pm8921_bms_get_fcc();
	if (rc < 0)
		pr_err("unable to get batt fcc rc = %d\n", rc);
	return rc;
}

static int get_prop_batt_health(struct pm8921_chg_chip *chip)
{
	int temp;

#ifdef CONFIG_LGE_PM
	/* LGE does not PMIC temp IRQ, so directly read batt-ADC. */
	temp = get_prop_batt_temp(chip);
	if (temp > BATT_TEMP_LEVEL_OVERHEAT*10)
		return POWER_SUPPLY_HEALTH_OVERHEAT;
	else if (temp < BATT_TEMP_LEVEL_COLD*10)
		return POWER_SUPPLY_HEALTH_COLD;
	else if ( chg_batt_temp_state == CHG_BATT_STOP_CHARGING_STATE &&
		(temp >= BATT_TEMP_LEVEL_RESUME_OVERHEAT*10) )
		return POWER_SUPPLY_HEALTH_OVERHEAT;
	else if ( chg_batt_temp_state == CHG_BATT_STOP_CHARGING_STATE &&
		(temp <= BATT_TEMP_LEVEL_RESUME_COLD*10) )
		return POWER_SUPPLY_HEALTH_COLD;
#else
	temp = pm_chg_get_rt_status(chip, BATTTEMP_HOT_IRQ);
	if (temp)
		return POWER_SUPPLY_HEALTH_OVERHEAT;

	temp = pm_chg_get_rt_status(chip, BATTTEMP_COLD_IRQ);
	if (temp)
		return POWER_SUPPLY_HEALTH_COLD;
#endif

	return POWER_SUPPLY_HEALTH_GOOD;
}

static int get_prop_charge_type(struct pm8921_chg_chip *chip)
{
	int temp;

	if (!get_prop_batt_present(chip))
		return POWER_SUPPLY_CHARGE_TYPE_NONE;

	if (is_ext_trickle_charging(chip))
		return POWER_SUPPLY_CHARGE_TYPE_TRICKLE;

	if (is_ext_charging(chip))
		return POWER_SUPPLY_CHARGE_TYPE_FAST;

	temp = pm_chg_get_rt_status(chip, TRKLCHG_IRQ);
	if (temp)
		return POWER_SUPPLY_CHARGE_TYPE_TRICKLE;

	temp = pm_chg_get_rt_status(chip, FASTCHG_IRQ);
	if (temp)
		return POWER_SUPPLY_CHARGE_TYPE_FAST;

	return POWER_SUPPLY_CHARGE_TYPE_NONE;
}

static int get_prop_batt_status(struct pm8921_chg_chip *chip)
{
	int batt_state = POWER_SUPPLY_STATUS_DISCHARGING;
	int fsm_state = pm_chg_get_fsm_state(chip);
	int i;
#ifdef CONFIG_LGE_PM
	/* jooyeong.lee@lge.com 2012-01-19 Change battery status sequence when start charging */
	int batt_capacity;
#endif

	if (chip->ext_psy) {
		if (chip->ext_charge_done)
			return POWER_SUPPLY_STATUS_FULL;
		if (chip->ext_charging)
			return POWER_SUPPLY_STATUS_CHARGING;
	}

#ifdef CONFIG_LGE_WIRELESS_CHARGER
	if(wireless_is_plugged()) { //if(is_dc_chg_plugged_in(chip)) {
		batt_state = wireless_batt_status();
		return batt_state;
	}
#endif

	for (i = 0; i < ARRAY_SIZE(map); i++)
		if (map[i].fsm_state == fsm_state)
			batt_state = map[i].batt_state;

#ifdef CONFIG_LGE_PM
	/* jooyeong.lee@lge.com 2012-01-19 Change battery status sequence when start charging */
	if (batt_state == POWER_SUPPLY_STATUS_FULL) {
#ifdef CONFIG_BATTERY_MAX17047
#if defined(CONFIG_MACH_APQ8064_GVDCM)
	if(lge_get_board_revno() > HW_REV_A)
		batt_capacity = max17047_get_soc();
	else
		batt_capacity = get_prop_batt_capacity(chip);
#else
	batt_capacity = max17047_get_soc();
#endif
#elif defined (CONFIG_BATTERY_MAX17043)
		batt_capacity = max17043_get_capacity(chip);
#else
		batt_capacity = get_prop_batt_capacity(chip);
#endif
		if (pm_chg_get_rt_status(chip, BATT_INSERTED_IRQ)
			&& is_battery_valid(chip)
			&& pm_chg_get_rt_status(chip, BAT_TEMP_OK_IRQ)
			&& batt_capacity >= 100 ) {
			return batt_state;
		}

		if (pm_chg_get_rt_status(chip, BATT_INSERTED_IRQ)
			&& is_battery_valid(chip)
			&& pm_chg_get_rt_status(chip, BAT_TEMP_OK_IRQ)
			&& batt_capacity >= 0 && batt_capacity < 100) {
			if ( pseudo_ui_charging == 1 )
				batt_state = POWER_SUPPLY_STATUS_NOT_CHARGING;
			else
				batt_state = POWER_SUPPLY_STATUS_CHARGING;
			return batt_state;
		}
	}
#endif

	if (fsm_state == FSM_STATE_ON_CHG_HIGHI_1) {
#ifdef CONFIG_LGE_CHARGER_TEMP_SCENARIO
		if (!pm_chg_get_rt_status(chip, BATT_INSERTED_IRQ)
			|| !pm_chg_get_rt_status(chip, BAT_TEMP_OK_IRQ)
			|| !pm_chg_get_rt_status(chip, CHGHOT_IRQ)
			|| ((pseudo_ui_charging == 0)
			    && (chg_batt_temp_state == CHG_BATT_STOP_CHARGING_STATE)))

			batt_state = POWER_SUPPLY_STATUS_NOT_CHARGING;
#else
		if (!pm_chg_get_rt_status(chip, BATT_INSERTED_IRQ)
			|| !pm_chg_get_rt_status(chip, BAT_TEMP_OK_IRQ)
			|| pm_chg_get_rt_status(chip, CHGHOT_IRQ)
			|| pm_chg_get_rt_status(chip, VBATDET_LOW_IRQ))

			batt_state = POWER_SUPPLY_STATUS_NOT_CHARGING;
#endif
	}
	
#if defined(CONFIG_MACH_APQ8064_GK_KR) || defined(CONFIG_MACH_APQ8064_GV_KR)
#ifdef CONFIG_LGE_CHARGER_TEMP_SCENARIO
		if(((chg_batt_temp_state == CHG_BATT_STOP_CHARGING_STATE) && (pseudo_ui_charging == 1))
			&& (is_usb_chg_plugged_in(chip))
			&& max17043_get_capacity(chip) < 100){
	
			batt_state = POWER_SUPPLY_STATUS_NOT_CHARGING;
			return batt_state;
		}
#endif
#endif	
	return batt_state;
}

#define MAX_TOLERABLE_BATT_TEMP_DDC	680
static int get_prop_batt_temp(struct pm8921_chg_chip *chip)
{
	int rc;
	struct pm8xxx_adc_chan_result result;

	rc = pm8xxx_adc_read(chip->batt_temp_channel, &result);
	if (rc) {
		pr_err("error reading adc channel = %d, rc = %d\n",
					chip->vbat_channel, rc);
		return rc;
	}
	pr_debug("batt_temp phy = %lld meas = 0x%llx\n", result.physical,
						result.measurement);
#ifdef CONFIG_LGE_PM
#if defined(CONFIG_MACH_APQ8064_J1SP)
        /* Temperatue compenation -2 degree for Sprint */
        result.physical -= 20;
#endif
	/* add for thermister test kwangjae1.lee@lge.com */
	if (pseudo_batt_info.mode)
		rc = (pseudo_batt_info.temp * 10);
	else
		rc = (int)result.physical ;

	pr_debug("batt_temp phy = %lld, result = %d\n",
						result.physical ,rc);

	return rc;
#else
	if (result.physical > MAX_TOLERABLE_BATT_TEMP_DDC)
		pr_err("BATT_TEMP= %d > 68degC, device will be shutdown\n",
							(int) result.physical);

	return (int)result.physical;
#endif
}

static int pm_batt_power_get_property(struct power_supply *psy,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	struct pm8921_chg_chip *chip = container_of(psy, struct pm8921_chg_chip,
								batt_psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
#ifdef CONFIG_LGE_PM
		if(block_charging_state == 0)
			val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
		else
			val->intval = get_prop_batt_status(chip);
#else
		val->intval = get_prop_batt_status(chip);
#endif
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		val->intval = get_prop_charge_type(chip);
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = get_prop_batt_health(chip);
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = get_prop_batt_present(chip);
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
		val->intval = chip->max_voltage_mv * 1000;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
		val->intval = chip->min_voltage_mv * 1000;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:

#ifdef CONFIG_BATTERY_MAX17047
#if defined(CONFIG_MACH_APQ8064_GVDCM)
		if(lge_get_board_revno() > HW_REV_A)
			val->intval = max17047_get_batt_vol();
		else
			val->intval = get_prop_battery_uvolts(chip);
#else
		val->intval = max17047_get_batt_vol();
#endif
#elif defined (CONFIG_BATTERY_MAX17043)
		val->intval = max17043_get_voltage(chip);
#else
		val->intval = get_prop_battery_uvolts(chip);
#endif
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
#ifdef CONFIG_BATTERY_MAX17047
#if defined(CONFIG_MACH_APQ8064_GVDCM)
		if(lge_get_board_revno() > HW_REV_A)
			val->intval = max17047_get_soc();
		else
			val->intval = get_prop_batt_capacity(chip);
#else
		val->intval = max17047_get_soc();
#endif
#elif defined (CONFIG_BATTERY_MAX17043)
		val->intval = max17043_get_capacity(chip);
#else
		val->intval = get_prop_batt_capacity(chip);
#endif
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = get_prop_batt_current(chip);
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		val->intval = get_prop_batt_current_max(chip);
		break;
	case POWER_SUPPLY_PROP_TEMP:
#ifdef CONFIG_LGE_PM
		if (is_real_battery_or_factory_cable(chip))
			val->intval = 25 * 10;
		else
			val->intval = get_prop_batt_temp(chip);
#else
		val->intval = get_prop_batt_temp(chip);
#endif
		break;
	case POWER_SUPPLY_PROP_ENERGY_FULL:
		val->intval = get_prop_batt_fcc(chip) * 1000;
		break;
#ifdef CONFIG_LGE_PM
	case POWER_SUPPLY_PROP_PSEUDO_BATT:
		val->intval = pseudo_batt_info.mode;
		break;
	case POWER_SUPPLY_PROP_BLOCK_CHARGING:
		val->intval = block_charging_state;
		break;
	case POWER_SUPPLY_PROP_EXT_PWR_CHECK:
		val->intval = lge_pm_get_cable_type();
		break;
	/* kwangjae1.lee@lge.com 2012-06-11 Add bms debugger */
	case POWER_SUPPLY_PROP_BMS_BATT:
		val->intval = bms_batt_info.mode;
		break;
	/* 2012-07-11 Add battery present check in the testmode */
	case POWER_SUPPLY_PROP_REAL_BATT_PRESENT:
		val->intval = pm_chg_get_rt_status(chip, BATT_INSERTED_IRQ);
		break;
#ifdef CONFIG_BATTERY_MAX17047
/*doosan.baek@lge.com 20121108 Add battery condition */
	case POWER_SUPPLY_PROP_BATTERY_CONDITION:
		val->intval = lge_pm_get_battery_condition();
		break;
	case POWER_SUPPLY_PROP_BATTERY_AGE:
		val->intval = lge_pm_get_battery_age();
		break;
#endif
#endif
#ifdef CONFIG_LGE_PM_BATTERY_ID_CHECKER
	case POWER_SUPPLY_PROP_BATTERY_ID_CHECK:
		val->intval = is_battery_valid(chip);
		break;
#endif
	default:
		return -EINVAL;
	}

	return 0;
}

/* [START] sungsookim */
#ifdef CONFIG_LGE_PM
/* LGE_S kwangjae1.lee@lge.com 2012-06-11 Add bms debugger */
int bms_batt_set(struct bms_batt_info_type* bms_info)
{
	bms_batt_info.mode = bms_info->mode;
	pr_debug("%s value =%d\n",__func__,bms_batt_info.mode);
	return 0;
}
EXPORT_SYMBOL(bms_batt_set);
/* LGE_E kwangjae1.lee@lge.com 2012-06-11 Add bms debugger */
#define PSEUDO_BATT_MAX 800
int pseudo_batt_set(struct pseudo_batt_info_type* info)
{
	struct pm8921_chg_chip *chip = the_chip;
	static int mA = 0;

	pseudo_batt_info.mode = info->mode;
	pseudo_batt_info.id = info->id;
	pseudo_batt_info.therm = info->therm;
	pseudo_batt_info.temp = info->temp;
	pseudo_batt_info.volt = info->volt;
	pseudo_batt_info.capacity = info->capacity;
	pseudo_batt_info.charging = info->charging;

	power_supply_changed(&chip->batt_psy);

	/* If pseudo batt enabled, Set iusb max to PSEUDO_BATT_MAX(800mA) - seonghun1.kim */
	pm_chg_iusbmax_get(chip, &mA);
	pr_debug("%s: iusb(%d) \n", __func__, mA);

	if ( pseudo_batt_info.mode == 1 ) {
		if ( mA <= PSEUDO_BATT_MAX )
			pm8921_charger_vbus_draw(PSEUDO_BATT_MAX);
	}
	else {
		if( lge_pm_get_cable_type() == CABLE_NONE && the_chip->usb_psy.type == POWER_SUPPLY_TYPE_USB_DCP)
			search_iusb_max_status = IUSB_MAX_INCREASE;
		else if(the_chip->usb_psy.type == POWER_SUPPLY_TYPE_USB_DCP)
			pm8921_charger_vbus_draw(lge_pm_get_ta_current());
		else if(the_chip->usb_psy.type == POWER_SUPPLY_TYPE_USB)
			pm8921_charger_vbus_draw(lge_pm_get_usb_current());
		else if(the_chip->usb_psy.type == POWER_SUPPLY_TYPE_BATTERY)
			pm8921_charger_vbus_draw(100);
		else
			pm8921_charger_vbus_draw(mA);

	}

	return 0;
}
EXPORT_SYMBOL(pseudo_batt_set);

void block_charging_set(int block)
{
	if(block)
	{
		//pm_chg_auto_disable(0);
		//pm_chg_charge_dis(the_chip, 0);
		pm8921_charger_enable(true);
	}
	else
	{
		//pm_chg_auto_disable(1);
		//pm_chg_auto_enable(the_chip, 1);
		pm8921_charger_enable(false);
	}
}
void batt_block_charging_set(int block)
{
	struct pm8921_chg_chip *chip = the_chip;

	block_charging_state = block;
	block_charging_set(block);

	power_supply_changed(&chip->batt_psy);
}
EXPORT_SYMBOL(batt_block_charging_set);
#endif
/* [END] */

static void (*notify_vbus_state_func_ptr)(int);
static int usb_chg_current;
static DEFINE_SPINLOCK(vbus_lock);

int pm8921_charger_register_vbus_sn(void (*callback)(int))
{
	pr_debug("%p\n", callback);
	notify_vbus_state_func_ptr = callback;
	return 0;
}
EXPORT_SYMBOL_GPL(pm8921_charger_register_vbus_sn);

/* this is passed to the hsusb via platform_data msm_otg_pdata */
void pm8921_charger_unregister_vbus_sn(void (*callback)(int))
{
	pr_debug("%p\n", callback);
	notify_vbus_state_func_ptr = NULL;
}
EXPORT_SYMBOL_GPL(pm8921_charger_unregister_vbus_sn);

static void notify_usb_of_the_plugin_event(int plugin)
{
	plugin = !!plugin;
	if (notify_vbus_state_func_ptr) {
		pr_debug("notifying plugin\n");
		(*notify_vbus_state_func_ptr) (plugin);
	} else {
		pr_debug("unable to notify plugin\n");
	}
}

/* assumes vbus_lock is held */
static void __pm8921_charger_vbus_draw(unsigned int mA)
{
	int i, rc;
	if (!the_chip) {
		pr_err("called before init\n");
		return;
	}

#ifdef CONFIG_LGE_PM
	/* If pseudo batt enabled, Set iusb max to PSEUDO_BATT_MAX(800mA) - seonghun1.kim */
	if ((pseudo_batt_info.mode == 1) && (mA <= PSEUDO_BATT_MAX))
		mA = PSEUDO_BATT_MAX;
#endif

	if (mA >= 0 && mA <= 2) {
		usb_chg_current = 0;
		rc = pm_chg_iusbmax_set(the_chip, 0);
		if (rc) {
			pr_err("unable to set iusb to %d rc = %d\n", 0, rc);
		}
		rc = pm_chg_usb_suspend_enable(the_chip, 1);
		if (rc)
			pr_err("fail to set suspend bit rc=%d\n", rc);
	} else {
		rc = pm_chg_usb_suspend_enable(the_chip, 0);
		if (rc)
			pr_err("fail to reset suspend bit rc=%d\n", rc);
		for (i = ARRAY_SIZE(usb_ma_table) - 1; i >= 0; i--) {
			if (usb_ma_table[i].usb_ma <= mA)
				break;
		}

		/* Check if IUSB_FINE_RES is available */
		while ((usb_ma_table[i].value & PM8917_IUSB_FINE_RES)
				&& !the_chip->iusb_fine_res)
			i--;
		if (i < 0)
			i = 0;
		rc = pm_chg_iusbmax_set(the_chip, i);
		if (rc) {
			pr_err("unable to set iusb to %d rc = %d\n", i, rc);
		}
	}
}

/* USB calls these to tell us how much max usb current the system can draw */
void pm8921_charger_vbus_draw(unsigned int mA)
{
	unsigned long flags;

	pr_debug("Enter charge=%d\n", mA);

	if (!the_chip) {
		pr_err("chip not yet initalized\n");
		return;
	}

	/*
	 * Reject VBUS requests if USB connection is the only available
	 * power source. This makes sure that if booting without
	 * battery the iusb_max value is not decreased avoiding potential
	 * brown_outs.
	 *
	 * This would also apply when the battery has been
	 * removed from the running system.
	 */
	if (!get_prop_batt_present(the_chip)
		&& !is_dc_chg_plugged_in(the_chip)) {
		if (!the_chip->has_dc_supply) {
			pr_err("rejected: no other power source connected\n");
			return;
		}
	}

	if (usb_max_current && mA > usb_max_current) {
		pr_warn("restricting usb current to %d instead of %d\n",
					usb_max_current, mA);
		mA = usb_max_current;
	}
	if (usb_target_ma == 0 && mA > USB_WALL_THRESHOLD_MA)
		usb_target_ma = mA;

	spin_lock_irqsave(&vbus_lock, flags);
	if (the_chip) {
		if (mA > USB_WALL_THRESHOLD_MA)
			__pm8921_charger_vbus_draw(USB_WALL_THRESHOLD_MA);
		else
			__pm8921_charger_vbus_draw(mA);
	} else {
		/*
		 * called before pmic initialized,
		 * save this value and use it at probe
		 */
		if (mA > USB_WALL_THRESHOLD_MA)
			usb_chg_current = USB_WALL_THRESHOLD_MA;
		else
			usb_chg_current = mA;
	}
#ifdef CONFIG_LGE_CHARGER_TEMP_SCENARIO
	last_usb_chg_current = mA;
#endif
	spin_unlock_irqrestore(&vbus_lock, flags);
}
EXPORT_SYMBOL_GPL(pm8921_charger_vbus_draw);

int pm8921_charger_enable(bool enable)
{
	int rc;

	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}
	enable = !!enable;
	rc = pm_chg_auto_enable(the_chip, enable);
	if (rc)
		pr_err("Failed rc=%d\n", rc);
	return rc;
}
EXPORT_SYMBOL(pm8921_charger_enable);
#ifdef CONFIG_LGE_PM
   /* MAKO patch for BMS */
int pm8921_force_start_charging(void)
{
	int rc;

	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}

	if (the_chip->eoc_check_soc) {
		rc = pm_chg_vbatdet_set(the_chip,
				the_chip->max_voltage_mv);
		if (rc) {
			pr_err("failed to set vbatdet\n");
			return rc;
		}
	}

	rc = pm_chg_auto_enable(the_chip, 1);
	if (rc)
		pr_err("Failed rc=%d\n", rc);

	return rc;
}
EXPORT_SYMBOL(pm8921_force_start_charging);
#endif

int pm8921_is_usb_chg_plugged_in(void)
{
	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}
	return is_usb_chg_plugged_in(the_chip);
}
EXPORT_SYMBOL(pm8921_is_usb_chg_plugged_in);

int pm8921_is_dc_chg_plugged_in(void)
{
	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}
	return is_dc_chg_plugged_in(the_chip);
}
EXPORT_SYMBOL(pm8921_is_dc_chg_plugged_in);

int pm8921_is_battery_present(void)
{
	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}
	return get_prop_batt_present(the_chip);
}
EXPORT_SYMBOL(pm8921_is_battery_present);

int pm8921_is_batfet_closed(void)
{
	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}
	return is_batfet_closed(the_chip);
}
EXPORT_SYMBOL(pm8921_is_batfet_closed);

#ifdef CONFIG_LGE_PM
/* jungwoo.yun@lge.com 2012-08-07 check battery preset regardless of factory cable*/
int pm8921_is_real_battery_present(void)
{
	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}
	return pm_chg_get_rt_status(the_chip, BATT_INSERTED_IRQ);
}
EXPORT_SYMBOL(pm8921_is_real_battery_present);


int pm8921_chg_get_fsm_state(void)
{
	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}
	return pm_chg_get_fsm_state(the_chip);
}
EXPORT_SYMBOL(pm8921_chg_get_fsm_state);

#endif

/*
 * Disabling the charge current limit causes current
 * current limits to have no monitoring. An adequate charger
 * capable of supplying high current while sustaining VIN_MIN
 * is required if the limiting is disabled.
 */
int pm8921_disable_input_current_limit(bool disable)
{
	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}
	if (disable) {
		pr_warn("Disabling input current limit!\n");

		return pm8xxx_writeb(the_chip->dev->parent,
			 CHG_BUCK_CTRL_TEST3, 0xF2);
	}
	return 0;
}
EXPORT_SYMBOL(pm8921_disable_input_current_limit);

int pm8917_set_under_voltage_detection_threshold(int mv)
{
	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}
	return pm_chg_uvd_threshold_set(the_chip, mv);
}
EXPORT_SYMBOL(pm8917_set_under_voltage_detection_threshold);

int pm8921_set_max_battery_charge_current(int ma)
{
	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}
	return pm_chg_ibatmax_set(the_chip, ma);
}
EXPORT_SYMBOL(pm8921_set_max_battery_charge_current);

int pm8921_disable_source_current(bool disable)
{
	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}
	if (disable)
		pr_warn("current drawn from chg=0, battery provides current\n");

	pm_chg_usb_suspend_enable(the_chip, disable);

	return pm_chg_charge_dis(the_chip, disable);
}
EXPORT_SYMBOL(pm8921_disable_source_current);

int pm8921_regulate_input_voltage(int voltage)
{
	int rc;

	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}
	rc = pm_chg_vinmin_set(the_chip, voltage);

	if (rc == 0)
		the_chip->vin_min = voltage;

	return rc;
}

#define USB_OV_THRESHOLD_MASK  0x60
#define USB_OV_THRESHOLD_SHIFT  5
int pm8921_usb_ovp_set_threshold(enum pm8921_usb_ov_threshold ov)
{
	u8 temp;

	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}

	if (ov > PM_USB_OV_7V) {
		pr_err("limiting to over voltage threshold to 7volts\n");
		ov = PM_USB_OV_7V;
	}

	temp = USB_OV_THRESHOLD_MASK & (ov << USB_OV_THRESHOLD_SHIFT);

	return pm_chg_masked_write(the_chip, USB_OVP_CONTROL,
				USB_OV_THRESHOLD_MASK, temp);
}
EXPORT_SYMBOL(pm8921_usb_ovp_set_threshold);

#define USB_DEBOUNCE_TIME_MASK	0x06
#define USB_DEBOUNCE_TIME_SHIFT 1
int pm8921_usb_ovp_set_hystersis(enum pm8921_usb_debounce_time ms)
{
	u8 temp;

	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}

	if (ms > PM_USB_DEBOUNCE_80P5MS) {
		pr_err("limiting debounce to 80.5ms\n");
		ms = PM_USB_DEBOUNCE_80P5MS;
	}

	temp = USB_DEBOUNCE_TIME_MASK & (ms << USB_DEBOUNCE_TIME_SHIFT);

	return pm_chg_masked_write(the_chip, USB_OVP_CONTROL,
				USB_DEBOUNCE_TIME_MASK, temp);
}
EXPORT_SYMBOL(pm8921_usb_ovp_set_hystersis);

#define USB_OVP_DISABLE_MASK	0x80
int pm8921_usb_ovp_disable(int disable)
{
	u8 temp = 0;

	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}

	if (disable)
		temp = USB_OVP_DISABLE_MASK;

	return pm_chg_masked_write(the_chip, USB_OVP_CONTROL,
				USB_OVP_DISABLE_MASK, temp);
}

bool pm8921_is_battery_charging(int *source)
{
	int fsm_state, is_charging, dc_present, usb_present;

	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}
	fsm_state = pm_chg_get_fsm_state(the_chip);
	is_charging = is_battery_charging(fsm_state);
	if (is_charging == 0) {
		*source = PM8921_CHG_SRC_NONE;
		return is_charging;
	}

	if (source == NULL)
		return is_charging;

	/* the battery is charging, the source is requested, find it */
	dc_present = is_dc_chg_plugged_in(the_chip);
	usb_present = is_usb_chg_plugged_in(the_chip);

	if (dc_present && !usb_present)
		*source = PM8921_CHG_SRC_DC;

	if (usb_present && !dc_present)
		*source = PM8921_CHG_SRC_USB;

	if (usb_present && dc_present)
		/*
		 * The system always chooses dc for charging since it has
		 * higher priority.
		 */
		*source = PM8921_CHG_SRC_DC;

	return is_charging;
}
EXPORT_SYMBOL(pm8921_is_battery_charging);

int pm8921_set_usb_power_supply_type(enum power_supply_type type)
{
	ChgLog("ChgLog> %s : %d\n", __func__,type);
	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}

#ifdef CONFIG_LGE_PM
	/* Wake lock for delever Uevnet before suspend */
	wake_lock_timeout(&the_chip->deliver_uevent_wake_lock, round_jiffies_relative(msecs_to_jiffies(1000)));
	power_supply_changed(&the_chip->batt_psy);
#endif

	if (type < POWER_SUPPLY_TYPE_USB)
		return 0; // -EINVAL;		 //Prevent otg error log

	the_chip->usb_psy.type = type;
	power_supply_changed(&the_chip->usb_psy);
	power_supply_changed(&the_chip->dc_psy);
#ifdef CONFIG_LGE_PM_ADP_CHG
	/* Adapive USB draw current limit */
	if (type == POWER_SUPPLY_TYPE_USB_DCP)
		search_iusb_max_status = IUSB_MAX_INCREASE;
	else
		search_iusb_max_status = IUSB_MAX_NONE;

#ifdef CONFIG_LGE_WIRELESS_CHARGER
	if (is_usb_chg_plugged_in(the_chip) && !is_dc_chg_plugged_in(the_chip) && !delayed_work_pending(&the_chip->adaptive_usb_current_work))
#else
	if(!delayed_work_pending(&the_chip->adaptive_usb_current_work))
#endif
	{
		printk("queueing adaptive_usb_current_work\n");
		/* max 2-sec delay time needed until fastchg-irq */
		schedule_delayed_work(&the_chip->adaptive_usb_current_work, 
			  round_jiffies_relative(msecs_to_jiffies(ADAPTIVE_USB_CURRENT_CHECK_PERIOD_MS*2)));
		
	}
#endif

	return 0;
}
EXPORT_SYMBOL_GPL(pm8921_set_usb_power_supply_type);

#ifdef CONFIG_LGE_PM
   /* MAKO patch for BMS */
int pm8921_get_batt_state(void)
{
	int batt_state = POWER_SUPPLY_STATUS_DISCHARGING;
	int fsm_state;
	int i;

	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}

	fsm_state = pm_chg_get_fsm_state(the_chip);

	for (i = 0; i < ARRAY_SIZE(map); i++)
		if (map[i].fsm_state == fsm_state)
			batt_state = map[i].batt_state;

	pr_debug("batt_state = %d fsm_state = %d \n",batt_state, fsm_state);
	return batt_state;
}
EXPORT_SYMBOL(pm8921_get_batt_state);

int pm8921_get_batt_health(void)
{
	int batt_health;

	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}

	batt_health = get_prop_batt_health(the_chip);

	pr_debug("batt health = %d\n", batt_health);
	return batt_health;
}
EXPORT_SYMBOL(pm8921_get_batt_health);
#endif

int pm8921_batt_temperature(void)
{
	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}
	return get_prop_batt_temp(the_chip);
}

static void handle_usb_insertion_removal(struct pm8921_chg_chip *chip)
{
	int usb_present;

#ifdef CONFIG_LGE_PM
	wake_lock_timeout(&uevent_wake_lock, HZ*2);
#endif

	pm_chg_failed_clear(chip, 1);
	usb_present = is_usb_chg_plugged_in(chip);
	if (chip->usb_present ^ usb_present) {
		notify_usb_of_the_plugin_event(usb_present);
		chip->usb_present = usb_present;
#ifdef CONFIG_LGE_PM
		/*
		 * Do not report power event here.
		 * Power event should reported after USB enumeration. - In pm8921_set_usb_power_supply_type()
		 * Because we don't know USB or TA before finished enumeration.
		 */
		//the_chip->usb_psy.type = 0;
#else
		power_supply_changed(&chip->usb_psy);
		power_supply_changed(&chip->batt_psy);
#endif
		pm8921_bms_calibrate_hkadc();
	}
#ifdef CONFIG_LGE_WIRELESS_CHARGER
	if (chg_batt_temp_state == CHG_BATT_NORMAL_STATE)
		wireless_reset_hw(usb_present,chip);
#endif
#ifdef CONFIG_LGE_CHARGER_TEMP_SCENARIO
	if (usb_present) {
		pseudo_ui_charging = 0;
		last_usb_chg_current = 0;
		/* 120712 cs.kim@lge.com Implements Thermal Mitigation iUSB setting */
		pre_mitigation = 0;
	}
#endif

#ifdef CONFIG_LGE_PM_ADP_CHG
	if (usb_present) {
		schedule_delayed_work(&chip->unplug_check_work,
			round_jiffies_relative(msecs_to_jiffies
				(UNPLUG_CHECK_WAIT_PERIOD_MS)));
	}
#else
	/* Revert USB unplug detection to 1043 - seonghun1.kim */
	if (usb_present) {
		schedule_delayed_work(&chip->unplug_check_work,
			round_jiffies_relative(msecs_to_jiffies
				(UNPLUG_CHECK_WAIT_PERIOD_MS)));
		pm8921_chg_enable_irq(chip, CHG_GONE_IRQ);
	} else {
		/* USB unplugged reset target current */
		usb_target_ma = 0;
		pm8921_chg_disable_irq(chip, CHG_GONE_IRQ);
	}
	enable_input_voltage_regulation(chip);
#endif

#ifdef CONFIG_LGE_PM
	chip->bms_notify.is_charge_done_hit = 0;
#endif
	bms_notify_check(chip);
}

static void handle_stop_ext_chg(struct pm8921_chg_chip *chip)
{
	if (!chip->ext_psy) {
		pr_debug("external charger not registered.\n");
		return;
	}

	if (!chip->ext_charging) {
		pr_debug("already not charging.\n");
		return;
	}

	power_supply_set_charge_type(chip->ext_psy,
					POWER_SUPPLY_CHARGE_TYPE_NONE);
	pm8921_disable_source_current(false); /* release BATFET */
	power_supply_changed(&chip->dc_psy);
	chip->ext_charging = false;
	chip->ext_charge_done = false;
	bms_notify_check(chip);
	/* Update battery charging LEDs and user space battery info */
	power_supply_changed(&chip->batt_psy);
}

static void handle_start_ext_chg(struct pm8921_chg_chip *chip)
{
	int dc_present;
	int batt_present;
	int batt_temp_ok;
	int vbat_ov;
	unsigned long delay =
		round_jiffies_relative(msecs_to_jiffies(EOC_CHECK_PERIOD_MS));

	if (!chip->ext_psy) {
		pr_debug("external charger not registered.\n");
		return;
	}

	if (chip->ext_charging) {
		pr_debug("already charging.\n");
		return;
	}

	dc_present = is_dc_chg_plugged_in(the_chip);
	batt_present = pm_chg_get_rt_status(chip, BATT_INSERTED_IRQ);
	batt_temp_ok = pm_chg_get_rt_status(chip, BAT_TEMP_OK_IRQ);

	if (!dc_present) {
		pr_warn("%s. dc not present.\n", __func__);
		return;
	}
	if (!batt_present) {
		pr_warn("%s. battery not present.\n", __func__);
		return;
	}
	if (!batt_temp_ok) {
		pr_warn("%s. battery temperature not ok.\n", __func__);
		return;
	}
	pm8921_disable_source_current(true); /* Force BATFET=ON */
	vbat_ov = pm_chg_get_rt_status(chip, VBAT_OV_IRQ);
	if (vbat_ov) {
		pr_warn("%s. battery over voltage.\n", __func__);
		return;
	}

	power_supply_set_online(chip->ext_psy, dc_present);
	power_supply_set_charge_type(chip->ext_psy,
					POWER_SUPPLY_CHARGE_TYPE_FAST);
	power_supply_changed(&chip->dc_psy);
	chip->ext_charging = true;
	chip->ext_charge_done = false;
	bms_notify_check(chip);
	/* Start BMS */
	schedule_delayed_work(&chip->eoc_work, delay);
	wake_lock(&chip->eoc_wake_lock);
	/* Update battery charging LEDs and user space battery info */
	power_supply_changed(&chip->batt_psy);
}

static void turn_off_ovp_fet(struct pm8921_chg_chip *chip, u16 ovptestreg)
{
	u8 temp;
	int rc;

	rc = pm8xxx_writeb(chip->dev->parent, ovptestreg, 0x30);
	if (rc) {
		pr_err("Failed to write 0x30 to OVP_TEST rc = %d\n", rc);
		return;
	}
	rc = pm8xxx_readb(chip->dev->parent, ovptestreg, &temp);
	if (rc) {
		pr_err("Failed to read from OVP_TEST rc = %d\n", rc);
		return;
	}
	/* set ovp fet disable bit and the write bit */
	temp |= 0x81;
	rc = pm8xxx_writeb(chip->dev->parent, ovptestreg, temp);
	if (rc) {
		pr_err("Failed to write 0x%x OVP_TEST rc=%d\n", temp, rc);
		return;
	}
}

static void turn_on_ovp_fet(struct pm8921_chg_chip *chip, u16 ovptestreg)
{
	u8 temp;
	int rc;

	rc = pm8xxx_writeb(chip->dev->parent, ovptestreg, 0x30);
	if (rc) {
		pr_err("Failed to write 0x30 to OVP_TEST rc = %d\n", rc);
		return;
	}
	rc = pm8xxx_readb(chip->dev->parent, ovptestreg, &temp);
	if (rc) {
		pr_err("Failed to read from OVP_TEST rc = %d\n", rc);
		return;
	}
	/* unset ovp fet disable bit and set the write bit */
	temp &= 0xFE;
	temp |= 0x80;
	rc = pm8xxx_writeb(chip->dev->parent, ovptestreg, temp);
	if (rc) {
		pr_err("Failed to write 0x%x to OVP_TEST rc = %d\n",
								temp, rc);
		return;
	}
}

static int param_open_ovp_counter = 10;
module_param(param_open_ovp_counter, int, 0644);

#define USB_ACTIVE_BIT BIT(5)
#define DC_ACTIVE_BIT BIT(6)
static int is_active_chg_plugged_in(struct pm8921_chg_chip *chip,
						u8 active_chg_mask)
{
	if (active_chg_mask & USB_ACTIVE_BIT)
		return pm_chg_get_rt_status(chip, USBIN_VALID_IRQ);
	else if (active_chg_mask & DC_ACTIVE_BIT)
		return pm_chg_get_rt_status(chip, DCIN_VALID_IRQ);
	else
		return 0;
}

#define WRITE_BANK_4		0xC0
#ifdef CONFIG_LGE_PM_ADP_CHG
static void unplug_wrkarnd_restore_worker(struct work_struct *work)
{
	u8 temp;
	int rc;
	struct delayed_work *dwork = to_delayed_work(work);
	struct pm8921_chg_chip *chip = container_of(dwork,
				struct pm8921_chg_chip,
				unplug_wrkarnd_restore_work);

	pr_debug("restoring vin_min to %d mV\n", chip->vin_min);
	rc = pm_chg_vinmin_set(the_chip, chip->vin_min);

	temp = WRITE_BANK_4 | 0xA;
	rc = pm8xxx_writeb(chip->dev->parent, CHG_BUCK_CTRL_TEST3, temp);
	if (rc) {
		pr_err("Error %d writing %d to addr %d\n", rc,
					temp, CHG_BUCK_CTRL_TEST3);
	}
	wake_unlock(&chip->unplug_wrkarnd_restore_wake_lock);
}
#endif

#define OVP_DEBOUNCE_TIME 0x06
static void unplug_ovp_fet_open(struct pm8921_chg_chip *chip)
{
	int chg_gone = 0, active_chg_plugged_in = 0;
	int count = 0;
	u8 active_mask = 0;
	u16 ovpreg, ovptestreg;

	if (is_usb_chg_plugged_in(chip) &&
		(chip->active_path & USB_ACTIVE_BIT)) {
		ovpreg = USB_OVP_CONTROL;
		ovptestreg = USB_OVP_TEST;
		active_mask = USB_ACTIVE_BIT;
	} else if (is_dc_chg_plugged_in(chip) &&
		(chip->active_path & DC_ACTIVE_BIT)) {
		ovpreg = DC_OVP_CONTROL;
		ovptestreg = DC_OVP_TEST;
		active_mask = DC_ACTIVE_BIT;
	} else {
		return;
	}

	while (count++ < param_open_ovp_counter) {
		pm_chg_masked_write(chip, ovpreg, OVP_DEBOUNCE_TIME, 0x0);
		usleep(10);
		active_chg_plugged_in
			= is_active_chg_plugged_in(chip, active_mask);
		chg_gone = pm_chg_get_rt_status(chip, CHG_GONE_IRQ);
		pr_debug("OVP FET count = %d chg_gone=%d, active_valid = %d\n",
					count, chg_gone, active_chg_plugged_in);

		/* note usb_chg_plugged_in=0 => chg_gone=1 */
		if (chg_gone == 1 && active_chg_plugged_in == 1) {
			pr_debug("since chg_gone = 1 dis ovp_fet for 20msec\n");
			turn_off_ovp_fet(chip, ovptestreg);

			msleep(20);

			turn_on_ovp_fet(chip, ovptestreg);
		} else {
			break;
		}
	}
	pm_chg_masked_write(chip, ovpreg, OVP_DEBOUNCE_TIME, 0x2);
	pr_info("Exit count=%d chg_gone=%d, active_valid=%d\n",
		count, chg_gone, active_chg_plugged_in);
	return;
}

#ifdef CONFIG_LGE_PM_ADP_CHG
/* Adapive USB draw current limit */
int lge_get_next_iusb_max(int direction)
{
	int i, usb_mA;
	int size, *table;

	if(chargerlogo_state)
	{
		table = ADAPTIVE_MA_TABLE_CHARGERLOGO;
		size = ARRAY_SIZE(ADAPTIVE_MA_TABLE_CHARGERLOGO);
	}
	else
	{
		table = ADAPTIVE_MA_TABLE;
		size = ARRAY_SIZE(ADAPTIVE_MA_TABLE);
	}

	pm_chg_iusbmax_get(the_chip,&usb_mA);

	/* Increase */
	if ( direction ) {
		for ( i = 0 ; i < size ; i ++ )
			if ( table[i] > usb_mA ) break;
	}
	/* Decrease */
	else {
		for ( i = size-1 ; i ; i -- )
			if ( table[i] < usb_mA ) break;
	}

	if ( i < size )
		return table[i];
	else
		return table[size-1];
}

static void adaptive_usb_current_worker(struct work_struct *work)
{
	static int next_ma = 0, prev_ma = 0;
	struct pm8xxx_adc_chan_result Vchg;

	/* Do nothing if not in charging mode */
	if ( !the_chip->bms_notify.is_charging ) {
		prev_ma = 0;
		return;
	}
	/* Increase USB draw current
		Conditiosn : TA && open type cable */
	if ( lge_pm_get_cable_type() == CABLE_NONE &&
		the_chip->usb_psy.type == POWER_SUPPLY_TYPE_USB_DCP &&
		search_iusb_max_status == IUSB_MAX_INCREASE &&
		!thermal_mitigation ) /* 120712 cs.kim@lge.com Implements Thermal Mitigation iUSB setting */
	{
		next_ma = lge_get_next_iusb_max(1);
		if (next_ma != prev_ma) {
			printk("ChgLog> IUSB_MAX_INCREASE  %d mA\n", next_ma);
			pm8921_charger_vbus_draw(next_ma);
			prev_ma = next_ma;
			search_iusb_max_status = IUSB_MAX_INCREASE;

			/* Wait for a while to monitor Vusb-in collapse */
			msleep(ADAPTIVE_USB_CURRENT_DROP_CHECK_MS);
		}
		else
			search_iusb_max_status = IUSB_MAX_DECREASE;
	}

	/* Decrease USB draw current
		Condition : Vin < Vbat_max(4.36V) */
	pm8xxx_adc_read(CHANNEL_USBIN, &Vchg);
	if ( Vchg.physical < CHARGING_COLLAPSE_VOLTAGE*1000 &&
		 search_iusb_max_status != IUSB_MAX_NONE )
	{
		next_ma = lge_get_next_iusb_max(0);
		if ( prev_ma != next_ma ) {
			printk("ChgLog> IUSB_MAX_DECREASE Vchg %d mA	%lld uV\n", next_ma, Vchg.physical);
			pm8921_charger_vbus_draw(next_ma);
			prev_ma = next_ma;
		}
		search_iusb_max_status = IUSB_MAX_DECREASE;
	}

	/* Arm next */
	if ( search_iusb_max_status != IUSB_MAX_NONE )
		schedule_delayed_work(&the_chip->adaptive_usb_current_work,
				  round_jiffies_relative(msecs_to_jiffies(ADAPTIVE_USB_CURRENT_CHECK_PERIOD_MS)));
}
#else
/* Remove QCT's Adaptive current limiting */
static int find_usb_ma_value(int value)
{
	int i;

	for (i = ARRAY_SIZE(usb_ma_table) - 1; i >= 0; i--) {
		if (value >= usb_ma_table[i].usb_ma)
			break;
	}

	return i;
}

static void decrease_usb_ma_value(int *value)
{
	int i;

	if (value) {
		i = find_usb_ma_value(*value);
		if (i > 0)
			i--;
		while (!the_chip->iusb_fine_res && i > 0
			&& (usb_ma_table[i].value & PM8917_IUSB_FINE_RES))
			i--;
		*value = usb_ma_table[i].usb_ma;
	}
}

static void increase_usb_ma_value(int *value)
{
	int i;

	if (value) {
		i = find_usb_ma_value(*value);

		if (i < (ARRAY_SIZE(usb_ma_table) - 1))
			i++;
		/* Get next correct entry if IUSB_FINE_RES is not available */
		while (!the_chip->iusb_fine_res
			&& (usb_ma_table[i].value & PM8917_IUSB_FINE_RES)
			&& i < (ARRAY_SIZE(usb_ma_table) - 1))
			i++;

		*value = usb_ma_table[i].usb_ma;
	}
}

static void vin_collapse_check_worker(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct pm8921_chg_chip *chip = container_of(dwork,
			struct pm8921_chg_chip, vin_collapse_check_work);

	/* AICL only for wall-chargers */
	if (is_usb_chg_plugged_in(chip) &&
		usb_target_ma > USB_WALL_THRESHOLD_MA) {
		/* decrease usb_target_ma */
		decrease_usb_ma_value(&usb_target_ma);
		/* reset here, increase in unplug_check_worker */
		__pm8921_charger_vbus_draw(USB_WALL_THRESHOLD_MA);
		pr_debug("usb_now=%d, usb_target = %d\n",
				USB_WALL_THRESHOLD_MA, usb_target_ma);
	} else {
		handle_usb_insertion_removal(chip);
	}
}
#endif /* End of CONFIG_LGE_PM_ADP_CHG */

#ifdef CONFIG_LGE_PM_REMOVE_BATT
/* Restart the machine when Battery Remove/Insert - seonghun1.kim */
static void kernel_restart_worker(struct work_struct *work)
{
	kernel_restart("battery");
}
#endif

#define VIN_MIN_COLLAPSE_CHECK_MS	50
static irqreturn_t usbin_valid_irq_handler(int irq, void *data)
{
	struct pm8921_chg_chip *chip = data;
	int high_transition;

	high_transition = pm_chg_get_rt_status(chip, USBIN_VALID_IRQ);
	ChgLog("ChgLog> %s : %d\n", __func__,high_transition);

#ifdef CONFIG_LGE_PM_ADP_CHG
	handle_usb_insertion_removal(data);
#else
	/* Remove QCT's Adaptive current limiting */
	if (usb_target_ma)
		schedule_delayed_work(&the_chip->vin_collapse_check_work,
				      round_jiffies_relative(msecs_to_jiffies
						(VIN_MIN_COLLAPSE_CHECK_MS)));
	else
		handle_usb_insertion_removal(data);
#endif

	return IRQ_HANDLED;
}

static irqreturn_t batt_inserted_irq_handler(int irq, void *data)
{
	struct pm8921_chg_chip *chip = data;
	int status;

	ChgLog("ChgLog> %s\n", __func__);
#ifdef CONFIG_LGE_PM_REMOVE_BATT
	/* Restart the machine when Battery Remove/Insert - seonghun1.kim */
	if(lge_pm_get_cable_type() > NO_INIT_CABLE && lge_pm_get_cable_type() <= CABLE_NONE
		&& !is_factory_cable() && !pseudo_batt_info.mode) {
		pr_info("%s: Battery inserted but cable exists. Now reset as scenario!\n", __func__);
		schedule_work(&chip->kernel_restart_work);
	}
#endif

	status = pm_chg_get_rt_status(chip, BATT_INSERTED_IRQ);

#ifdef CONFIG_LGE_PM
	/* return control to PMIC FSM */
	if (status > 0 && pm_chg_batfet_get(chip) > 0) {
		pm_chg_batfet_set(chip, 0);
	}
#endif

	schedule_work(&chip->battery_id_valid_work);
	handle_start_ext_chg(chip);
	pr_debug("battery present=%d", status);
	power_supply_changed(&chip->batt_psy);
	return IRQ_HANDLED;
}

/*
 * this interrupt used to restart charging a battery.
 *
 * Note: When DC-inserted the VBAT can't go low.
 * VPH_PWR is provided by the ext-charger.
 * After End-Of-Charging from DC, charging can be resumed only
 * if DC is removed and then inserted after the battery was in use.
 * Therefore the handle_start_ext_chg() is not called.
 */
static irqreturn_t vbatdet_low_irq_handler(int irq, void *data)
{
	struct pm8921_chg_chip *chip = data;
	int high_transition;

	high_transition = pm_chg_get_rt_status(chip, VBATDET_LOW_IRQ);
	ChgLog("ChgLog> %s : %d temp_state : %d\n", __func__,high_transition,chg_batt_temp_state);

#ifdef CONFIG_LGE_PM
	if (chg_batt_temp_state == CHG_BATT_STOP_CHARGING_STATE)
		return IRQ_HANDLED;//Batt is too cold or hot to restart charging.
#endif

	if (high_transition) {
		/* enable auto charging */
		pm_chg_auto_enable(chip, !charging_disabled);
		pr_debug("batt fell below resume voltage %s\n",
			charging_disabled ? "" : "charger enabled");
	}
	pr_debug("fsm_state=%d\n", pm_chg_get_fsm_state(data));

	power_supply_changed(&chip->batt_psy);
	power_supply_changed(&chip->usb_psy);
	power_supply_changed(&chip->dc_psy);

	return IRQ_HANDLED;
}

static irqreturn_t vbat_ov_irq_handler(int irq, void *data)
{
	ChgLog("ChgLog> %s\n", __func__);
	pr_debug("fsm_state=%d\n", pm_chg_get_fsm_state(data));
	return IRQ_HANDLED;
}

static irqreturn_t chgwdog_irq_handler(int irq, void *data)
{
	ChgLog("ChgLog> %s\n", __func__);
	pr_debug("fsm_state=%d\n", pm_chg_get_fsm_state(data));
	return IRQ_HANDLED;
}

static irqreturn_t vcp_irq_handler(int irq, void *data)
{
	ChgLog("ChgLog> %s\n", __func__);
	pr_debug("fsm_state=%d\n", pm_chg_get_fsm_state(data));
	return IRQ_HANDLED;
}

static irqreturn_t atcdone_irq_handler(int irq, void *data)
{
	ChgLog("ChgLog> %s\n", __func__);
	pr_debug("fsm_state=%d\n", pm_chg_get_fsm_state(data));
	return IRQ_HANDLED;
}

static irqreturn_t atcfail_irq_handler(int irq, void *data)
{
	ChgLog("ChgLog> %s\n", __func__);
	pr_debug("fsm_state=%d\n", pm_chg_get_fsm_state(data));
	return IRQ_HANDLED;
}

static irqreturn_t chgdone_irq_handler(int irq, void *data)
{
	struct pm8921_chg_chip *chip = data;

	ChgLog("ChgLog> %s\n", __func__);
	pr_debug("state_changed_to=%d\n", pm_chg_get_fsm_state(data));

	handle_stop_ext_chg(chip);

	power_supply_changed(&chip->batt_psy);
	power_supply_changed(&chip->usb_psy);
	power_supply_changed(&chip->dc_psy);

#ifdef CONFIG_LGE_PM
	chip->bms_notify.is_charge_done_hit = 1;
#endif
	bms_notify_check(chip);

	return IRQ_HANDLED;
}

static irqreturn_t chgfail_irq_handler(int irq, void *data)
{
	struct pm8921_chg_chip *chip = data;
	int ret;

	ChgLog("ChgLog> %s\n", __func__);
	ret = pm_chg_failed_clear(chip, 1);
	if (ret)
		pr_err("Failed to write CHG_FAILED_CLEAR bit\n");

	pr_err("batt_present = %d, batt_temp_ok = %d, state_changed_to=%d\n",
			get_prop_batt_present(chip),
			pm_chg_get_rt_status(chip, BAT_TEMP_OK_IRQ),
			pm_chg_get_fsm_state(data));

	power_supply_changed(&chip->batt_psy);
	power_supply_changed(&chip->usb_psy);
	power_supply_changed(&chip->dc_psy);
	return IRQ_HANDLED;
}

static irqreturn_t chgstate_irq_handler(int irq, void *data)
{
	struct pm8921_chg_chip *chip = data;
#ifdef CONFIG_LGE_WIRELESS_CHARGER
	printk(KERN_INFO "[WIRELESS] CHGSTATE FSM=%d\n", pm_chg_get_fsm_state(chip));
#else
	ChgLog("ChgLog> %s\n", __func__);
	pr_debug("state_changed_to=%d\n", pm_chg_get_fsm_state(data));
	power_supply_changed(&chip->batt_psy);
	power_supply_changed(&chip->usb_psy);
	power_supply_changed(&chip->dc_psy);

	bms_notify_check(chip);
#endif
	return IRQ_HANDLED;
}

static int param_vin_disable_counter = 5;
module_param(param_vin_disable_counter, int, 0644);

#ifndef CONFIG_LGE_PM_ADP_CHG
/* Revert USB unplug detection to 1043 - seonghun1.kim */
static void attempt_reverse_boost_fix(struct pm8921_chg_chip *chip,
							int count, int usb_ma)
{
	if (usb_ma)
		__pm8921_charger_vbus_draw(500);
	pr_debug("count = %d iusb=500mA\n", count);
	disable_input_voltage_regulation(chip);
	pr_debug("count = %d disable_input_regulation\n", count);

	msleep(20);

	pr_debug("count = %d end sleep 20ms chg_gone=%d, usb_valid = %d\n",
				count,
				pm_chg_get_rt_status(chip, CHG_GONE_IRQ),
				is_usb_chg_plugged_in(chip));
	pr_debug("count = %d restoring input regulation and usb_ma = %d\n",
		 count, usb_ma);
	enable_input_voltage_regulation(chip);
	if (usb_ma)
		__pm8921_charger_vbus_draw(usb_ma);
}
#endif

#define VIN_ACTIVE_BIT BIT(0)
#define UNPLUG_WRKARND_RESTORE_WAIT_PERIOD_US	200
#define VIN_MIN_INCREASE_MV	100
#ifdef CONFIG_LGE_PM_ADP_CHG
static void unplug_check_worker(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct pm8921_chg_chip *chip = container_of(dwork,
				struct pm8921_chg_chip, unplug_check_work);
	u8 reg_loop, active_path;
	int rc, ibat, usb_chg_plugged_in;

	rc = pm8xxx_readb(chip->dev->parent, PBL_ACCESS1, &active_path);
	if (rc) {
		pr_err("Failed to read PBL_ACCESS1 rc=%d\n", rc);
		return;
	}
	chip->active_path = active_path;

	usb_chg_plugged_in = is_usb_chg_plugged_in(chip);
	if (!usb_chg_plugged_in) {
		pr_debug("Stopping Unplug Check Worker since USB is removed"
			"reg_loop = %d, fsm = %d ibat = %d\n",
			pm_chg_get_regulation_loop(chip),
			pm_chg_get_fsm_state(chip),
			get_prop_batt_current(chip)
			);
		return;
	}
	reg_loop = pm_chg_get_regulation_loop(chip);
	pr_debug("reg_loop=0x%x\n", reg_loop);

	if (reg_loop & VIN_ACTIVE_BIT) {
		ibat = get_prop_batt_current(chip);

		pr_debug("ibat = %d fsm = %d reg_loop = 0x%x\n",
				ibat, pm_chg_get_fsm_state(chip), reg_loop);
		if (ibat > 0) {
			int err;
			u8 temp;

			temp = WRITE_BANK_4 | 0xE;
			err = pm8xxx_writeb(chip->dev->parent,
						CHG_BUCK_CTRL_TEST3, temp);
			if (err) {
				pr_err("Error %d writing %d to addr %d\n", err,
						temp, CHG_BUCK_CTRL_TEST3);
			}

			pm_chg_vinmin_set(chip,
					chip->vin_min + VIN_MIN_INCREASE_MV);

			wake_lock(&chip->unplug_wrkarnd_restore_wake_lock);
			schedule_delayed_work(
				&chip->unplug_wrkarnd_restore_work,
				round_jiffies_relative(usecs_to_jiffies
				(UNPLUG_WRKARND_RESTORE_WAIT_PERIOD_US)));
		}
	}

	// USB Unplug detect using OVP_FET on/off
	if ( pm_chg_get_rt_status(chip, CHG_GONE_IRQ) &&
		( is_battery_charging(pm_chg_get_fsm_state(chip)) || chargerlogo_state ) )
	{
		int USB_Vin;
		struct pm8xxx_adc_chan_result Vchg;

		pm8xxx_adc_read(CHANNEL_USBIN, &Vchg);
		USB_Vin = Vchg.physical;
		pr_info(" USB Vin : %d\n",USB_Vin);

		/*  Turn off / on the OVP_FET for detect USB unplug if the USB Vin under Vbat_max */
		if ( USB_Vin/1000 <= chip->max_voltage_mv)
		{
			unplug_ovp_fet_open(chip);
		}
	}

	schedule_delayed_work(&chip->unplug_check_work,
		      round_jiffies_relative(msecs_to_jiffies
				(UNPLUG_CHECK_WAIT_PERIOD_MS)));
}
#else
/* Revert USB unplug detection to 1043 - seonghun1.kim */
static void unplug_check_worker(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct pm8921_chg_chip *chip = container_of(dwork,
				struct pm8921_chg_chip, unplug_check_work);
	u8 reg_loop, active_path;
	int rc, ibat, active_chg_plugged_in, usb_ma;
	int chg_gone = 0;

	reg_loop = 0;

	rc = pm8xxx_readb(chip->dev->parent, PBL_ACCESS1, &active_path);
	if (rc) {
		pr_err("Failed to read PBL_ACCESS1 rc=%d\n", rc);
		return;
	}
	chip->active_path = active_path;

	active_chg_plugged_in = is_active_chg_plugged_in(chip, active_path);
	pr_debug("active_path = 0x%x, active_chg_plugged_in = %d\n",
			active_path, active_chg_plugged_in);
	if (active_path & USB_ACTIVE_BIT) {
		pr_debug("USB charger active\n");

		pm_chg_iusbmax_get(chip, &usb_ma);
		if (usb_ma == 500 && !usb_target_ma) {
			pr_debug("Stopping Unplug Check Worker USB == 500mA\n");
			disable_input_voltage_regulation(chip);
			return;
		}

		if (usb_ma <= 100) {
			pr_debug(
				"Unenumerated or suspended usb_ma = %d skip\n",
				usb_ma);
			goto check_again_later;
		}
	} else if (active_path & DC_ACTIVE_BIT) {
		pr_debug("DC charger active\n");
		/* Some board designs are not prone to reverse boost on DC
		 * charging path */
		if (!chip->dc_unplug_check)
			return;
	} else {
		/* No charger active */
		if (!(is_usb_chg_plugged_in(chip)
				&& !(is_dc_chg_plugged_in(chip)))) {
			pr_debug(
			"Stop: chg removed reg_loop = %d, fsm = %d ibat = %d\n",
				pm_chg_get_regulation_loop(chip),
				pm_chg_get_fsm_state(chip),
				get_prop_batt_current(chip)
				);
			return;
		} else {
			goto check_again_later;
		}
	}

	if (active_path & USB_ACTIVE_BIT) {
		reg_loop = pm_chg_get_regulation_loop(chip);
		pr_debug("reg_loop=0x%x usb_ma = %d\n", reg_loop, usb_ma);
		if ((reg_loop & VIN_ACTIVE_BIT) &&
			(usb_ma > USB_WALL_THRESHOLD_MA)
			&& !charging_disabled) {
			decrease_usb_ma_value(&usb_ma);
			usb_target_ma = usb_ma;
			/* end AICL here */
			__pm8921_charger_vbus_draw(usb_ma);
			pr_debug("usb_now=%d, usb_target = %d\n",
				usb_ma, usb_target_ma);
		}
	}

	reg_loop = pm_chg_get_regulation_loop(chip);
	pr_debug("reg_loop=0x%x usb_ma = %d\n", reg_loop, usb_ma);

	ibat = get_prop_batt_current(chip);
	if (reg_loop & VIN_ACTIVE_BIT) {
		pr_debug("ibat = %d fsm = %d reg_loop = 0x%x\n",
				ibat, pm_chg_get_fsm_state(chip), reg_loop);
		if (ibat > 0) {
			int count = 0;

			while (count++ < param_vin_disable_counter
					&& active_chg_plugged_in == 1) {
				if (active_path & USB_ACTIVE_BIT)
					attempt_reverse_boost_fix(chip,
								count, usb_ma);
				else
					attempt_reverse_boost_fix(chip,
								count, 0);
				/* after reverse boost fix check if the active
				 * charger was detected as removed */
				active_chg_plugged_in
					= is_active_chg_plugged_in(chip,
						active_path);
				pr_debug("active_chg_plugged_in = %d\n",
						active_chg_plugged_in);
			}
		}
	}

	active_chg_plugged_in = is_active_chg_plugged_in(chip, active_path);
	pr_debug("active_path = 0x%x, active_chg = %d\n",
			active_path, active_chg_plugged_in);
	chg_gone = pm_chg_get_rt_status(chip, CHG_GONE_IRQ);

	if (chg_gone == 1  && active_chg_plugged_in == 1) {
		pr_debug("chg_gone=%d, active_chg_plugged_in = %d\n",
					chg_gone, active_chg_plugged_in);
		unplug_ovp_fet_open(chip);
	}

	if (!(reg_loop & VIN_ACTIVE_BIT) && (active_path & USB_ACTIVE_BIT)
		&& !charging_disabled) {
		/* only increase iusb_max if vin loop not active */
		if (usb_ma < usb_target_ma) {
			increase_usb_ma_value(&usb_ma);
			__pm8921_charger_vbus_draw(usb_ma);
			pr_debug("usb_now=%d, usb_target = %d\n",
					usb_ma, usb_target_ma);
		} else {
			usb_target_ma = usb_ma;
		}
	}
check_again_later:
	/* schedule to check again later */
	schedule_delayed_work(&chip->unplug_check_work,
		      round_jiffies_relative(msecs_to_jiffies
				(UNPLUG_CHECK_WAIT_PERIOD_MS)));
}
#endif

static irqreturn_t loop_change_irq_handler(int irq, void *data)
{
	struct pm8921_chg_chip *chip = data;

	ChgLog("ChgLog> %s\n", __func__);
	pr_debug("fsm_state=%d reg_loop=0x%x\n",
		pm_chg_get_fsm_state(data),
		pm_chg_get_regulation_loop(data));
	schedule_work(&chip->unplug_check_work.work);
	return IRQ_HANDLED;
}

static irqreturn_t fastchg_irq_handler(int irq, void *data)
{
	struct pm8921_chg_chip *chip = data;
	int high_transition;

	high_transition = pm_chg_get_rt_status(chip, FASTCHG_IRQ);
	ChgLog("ChgLog> %s : %d ", __func__,high_transition);
#ifdef CONFIG_LGE_WIRELESS_CHARGER
	if (!is_usb_chg_plugged_in(chip) && is_dc_chg_plugged_in(chip) && high_transition && !delayed_work_pending(&chip->eoc_work)) {
		wake_lock(&chip->eoc_wake_lock);
		schedule_delayed_work(&chip->eoc_work,round_jiffies_relative(msecs_to_jiffies(WIRELESS_EARLY_EOC_WORK_TIME)));
		printk(KERN_INFO "[WIRELESS] WLC fastchg_irq : eoc_work start ");
	}
#endif
	if (high_transition && !delayed_work_pending(&chip->eoc_work)) {
		wake_lock(&chip->eoc_wake_lock);
		ChgLog(" EOC Start");
		schedule_delayed_work(&chip->eoc_work,
				      round_jiffies_relative(msecs_to_jiffies
						     (EOC_CHECK_PERIOD_MS)));
	}
	ChgLog(" \n");
#ifdef CONFIG_LGE_PM
	/* Reset the resume charging voltage */
	if (high_transition) pm_chg_vbatdet_set(chip, chip->max_voltage_mv - chip->resume_voltage_delta);
#endif

#ifdef CONFIG_LGE_PM_ADP_CHG
#ifdef CONFIG_LGE_WIRELESS_CHARGER
	if (is_usb_chg_plugged_in(chip) && !is_dc_chg_plugged_in(chip) && high_transition && !delayed_work_pending(&chip->adaptive_usb_current_work))
#else
	/* Adapive USB draw current limit */
	if ( high_transition && !delayed_work_pending(&chip->adaptive_usb_current_work) )
#endif
	{
		printk("queueing adaptive_usb_current_work\n");
		schedule_delayed_work(&chip->adaptive_usb_current_work,
					round_jiffies_relative(msecs_to_jiffies(ADAPTIVE_USB_CURRENT_CHECK_PERIOD_MS*2)));
	}
#endif

	power_supply_changed(&chip->batt_psy);
	bms_notify_check(chip);

	return IRQ_HANDLED;
}

static irqreturn_t trklchg_irq_handler(int irq, void *data)
{
	struct pm8921_chg_chip *chip = data;

	ChgLog("ChgLog> %s\n", __func__);
	power_supply_changed(&chip->batt_psy);
	return IRQ_HANDLED;
}

static irqreturn_t batt_removed_irq_handler(int irq, void *data)
{
	struct pm8921_chg_chip *chip = data;
	int status;

	ChgLog("ChgLog> %s\n", __func__);
#ifdef CONFIG_LGE_PM_REMOVE_BATT
	/* Restart the machine when Battery Remove/Insert - seonghun1.kim */
	if(lge_pm_get_cable_type() > NO_INIT_CABLE && lge_pm_get_cable_type() <= CABLE_NONE
		&& !is_factory_cable() && !pseudo_batt_info.mode) {
		pr_debug("%s: Battery removed but cable exists. Now reset as scenario!\n", __func__);
		schedule_work(&chip->kernel_restart_work);
	}
#endif

	status = pm_chg_get_rt_status(chip, BATT_REMOVED_IRQ);
	pr_debug("battery present=%d state=%d", !status,
					 pm_chg_get_fsm_state(data));
	handle_stop_ext_chg(chip);
	power_supply_changed(&chip->batt_psy);
	return IRQ_HANDLED;
}

static irqreturn_t batttemp_hot_irq_handler(int irq, void *data)
{
	struct pm8921_chg_chip *chip = data;

	ChgLog("ChgLog> %s\n", __func__);
	handle_stop_ext_chg(chip);
	power_supply_changed(&chip->batt_psy);
	return IRQ_HANDLED;
}

static irqreturn_t chghot_irq_handler(int irq, void *data)
{
	struct pm8921_chg_chip *chip = data;

	ChgLog("ChgLog> %s\n", __func__);
	pr_debug("Chg hot fsm_state=%d\n", pm_chg_get_fsm_state(data));
	power_supply_changed(&chip->batt_psy);
	power_supply_changed(&chip->usb_psy);
	handle_stop_ext_chg(chip);
	return IRQ_HANDLED;
}

static irqreturn_t batttemp_cold_irq_handler(int irq, void *data)
{
	struct pm8921_chg_chip *chip = data;

	ChgLog("ChgLog> %s\n", __func__);
	pr_debug("Batt cold fsm_state=%d\n", pm_chg_get_fsm_state(data));
	handle_stop_ext_chg(chip);

	power_supply_changed(&chip->batt_psy);
	power_supply_changed(&chip->usb_psy);
	return IRQ_HANDLED;
}

static irqreturn_t chg_gone_irq_handler(int irq, void *data)
{
#ifndef CONFIG_LGE_PM_ADP_CHG
	/* Revert USB unplug detection to 1043 - seonghun1.kim */
	struct pm8921_chg_chip *chip = data;
	int chg_gone, usb_chg_plugged_in;

	usb_chg_plugged_in = is_usb_chg_plugged_in(chip);
	chg_gone = pm_chg_get_rt_status(chip, CHG_GONE_IRQ);

	pr_debug("chg_gone=%d, usb_valid = %d\n", chg_gone, usb_chg_plugged_in);
	pr_debug("Chg gone fsm_state=%d\n", pm_chg_get_fsm_state(data));

	power_supply_changed(&chip->batt_psy);
	power_supply_changed(&chip->usb_psy);
#endif
	ChgLog("ChgLog> %s\n", __func__);
	return IRQ_HANDLED;
}
/*
 *
 * bat_temp_ok_irq_handler - is edge triggered, hence it will
 * fire for two cases:
 *
 * If the interrupt line switches to high temperature is okay
 * and thus charging begins.
 * If bat_temp_ok is low this means the temperature is now
 * too hot or cold, so charging is stopped.
 *
 */
static irqreturn_t bat_temp_ok_irq_handler(int irq, void *data)
{
	int bat_temp_ok;
	struct pm8921_chg_chip *chip = data;

	ChgLog("ChgLog> %s\n", __func__);
	bat_temp_ok = pm_chg_get_rt_status(chip, BAT_TEMP_OK_IRQ);

	pr_debug("batt_temp_ok = %d fsm_state%d\n",
			 bat_temp_ok, pm_chg_get_fsm_state(data));

	if (bat_temp_ok)
		handle_start_ext_chg(chip);
	else
		handle_stop_ext_chg(chip);

	power_supply_changed(&chip->batt_psy);
	power_supply_changed(&chip->usb_psy);
	bms_notify_check(chip);
	return IRQ_HANDLED;
}

static irqreturn_t coarse_det_low_irq_handler(int irq, void *data)
{
	ChgLog("ChgLog> %s fsm:%d\n", __func__,pm_chg_get_fsm_state(data));
	pr_debug("fsm_state=%d\n", pm_chg_get_fsm_state(data));
	return IRQ_HANDLED;
}

static irqreturn_t vdd_loop_irq_handler(int irq, void *data)
{
	ChgLog("ChgLog> %s fsm:%d\n", __func__,pm_chg_get_fsm_state(data));
	pr_debug("fsm_state=%d\n", pm_chg_get_fsm_state(data));
	return IRQ_HANDLED;
}

static irqreturn_t vreg_ov_irq_handler(int irq, void *data)
{
	ChgLog("ChgLog> %s fsm:%d\n", __func__,pm_chg_get_fsm_state(data));
	pr_debug("fsm_state=%d\n", pm_chg_get_fsm_state(data));
	return IRQ_HANDLED;
}

static irqreturn_t vbatdet_irq_handler(int irq, void *data)
{
	ChgLog("ChgLog> %s fsm:%d\n", __func__,pm_chg_get_fsm_state(data));
	pr_debug("fsm_state=%d\n", pm_chg_get_fsm_state(data));
	return IRQ_HANDLED;
}

static irqreturn_t batfet_irq_handler(int irq, void *data)
{
	struct pm8921_chg_chip *chip = data;
#ifdef CONFIG_LGE_WIRELESS_CHARGER
	printk(KERN_INFO "[WIRELESS] BATFET RT=%d\n",pm_chg_get_rt_status(chip,BATFET_IRQ));
#else
	ChgLog("ChgLog> %s\n", __func__);
	pr_debug("vreg ov\n");
	power_supply_changed(&chip->batt_psy);
#endif
	return IRQ_HANDLED;
}

#ifndef CONFIG_LGE_WIRELESS_CHARGER
static irqreturn_t dcin_valid_irq_handler(int irq, void *data)
{
	struct pm8921_chg_chip *chip = data;
	int dc_present;

	ChgLog("ChgLog> %s\n", __func__);
	dc_present = pm_chg_get_rt_status(chip, DCIN_VALID_IRQ);
	if (chip->ext_psy)
		power_supply_set_online(chip->ext_psy, dc_present);
	chip->dc_present = dc_present;
	if (dc_present)
		handle_start_ext_chg(chip);
	else
		handle_stop_ext_chg(chip);
#ifdef CONFIG_LGE_CHARGER_TEMP_SCENARIO
	if (dc_present) {
		pseudo_ui_charging = 0;
		last_usb_chg_current = 0;
		/* 120712 cs.kim@lge.com Implements Thermal Mitigation iUSB setting */
		pre_mitigation = 0;
	}
#endif
	return IRQ_HANDLED;
}
#endif

static irqreturn_t dcin_ov_irq_handler(int irq, void *data)
{
	struct pm8921_chg_chip *chip = data;

	ChgLog("ChgLog> %s\n", __func__);
	handle_stop_ext_chg(chip);
	return IRQ_HANDLED;
}

static irqreturn_t dcin_uv_irq_handler(int irq, void *data)
{
	struct pm8921_chg_chip *chip = data;

	ChgLog("ChgLog> %s\n", __func__);
	handle_stop_ext_chg(chip);

	return IRQ_HANDLED;
}

#ifdef CONFIG_LGE_CHARGER_TEMP_SCENARIO
static int chg_is_battery_too_hot_or_too_cold(void *data,int batt_temp, int batt_level)
{
	int batt_temp_level;
	int rtnValue = 0;
	struct pm8921_chg_chip *chip = data;

#if !defined(CONFIG_MACH_APQ8064_GVKT)
	pseudo_ui_charging = 0;
#endif

	if (batt_temp > chip->temp_level_1)
		batt_temp_level = CHG_BATT_TEMP_LEVEL_1;/*  Over 55  */
	else if (batt_temp >= chip->temp_level_2)
		batt_temp_level = CHG_BATT_TEMP_LEVEL_2;/*  46~55   */
	else if (batt_temp >= chip->temp_level_3)
		batt_temp_level = CHG_BATT_TEMP_LEVEL_3;/*  42~45   */
	else if (batt_temp > chip->temp_level_4)
		batt_temp_level = CHG_BATT_TEMP_LEVEL_4;/*  M4~41   */
	else if (batt_temp >= chip->temp_level_5)
		batt_temp_level = CHG_BATT_TEMP_LEVEL_5;/*  M10~M5   */
	else
		batt_temp_level = CHG_BATT_TEMP_LEVEL_6;/*  Under M10   */

#if defined(CONFIG_MACH_APQ8064_J1SP)
/* Add temp for charing scenario on SPRINT */
	if( chg_batt_temp_state == CHG_BATT_STOP_CHARGING_STATE)
	{
		if (batt_temp <= chip->temp_level_1_1 && batt_temp >= chip->temp_level_4)
			batt_temp_level = CHG_BATT_TEMP_LEVEL_1_1; /*	M5 ~ 50	*/
	}
#endif

#ifdef CONFIG_LGE_WIRELESS_CHARGER
	if((batt_temp_level == CHG_BATT_TEMP_LEVEL_5) || (batt_temp_level == CHG_BATT_TEMP_LEVEL_6))
		wlc_charging_status = 1;
	else
		wlc_charging_status = 0;
#endif

	pr_debug("%s state = %d bat_level=%d \n",
		__func__ ,chg_batt_temp_state, batt_temp_level);
	switch(chg_batt_temp_state)
	{
	case CHG_BATT_NORMAL_STATE:
		if (batt_temp_level == CHG_BATT_TEMP_LEVEL_1 ||
			batt_temp_level == CHG_BATT_TEMP_LEVEL_6 ||
			(batt_temp_level == CHG_BATT_TEMP_LEVEL_2 && batt_level > 4000)) {

			chg_batt_temp_state = CHG_BATT_STOP_CHARGING_STATE;
			/*
			 * kiwone.seo@lge.com 2011-0609
			 * for show charging ani.although charging is stopped : charging scenario
			 */
/* CONFIG_PM_S submit ATnT temp scenario kwangjae1.lee */
			/* Change termal scenario to LGE ver 1.6 - charge Icon under -10 */
#if defined(CONFIG_MACH_APQ8064_GVDCM) || defined(CONFIG_MACH_APQ8064_J1D)
			if (batt_temp_level == CHG_BATT_TEMP_LEVEL_1 ||
				batt_temp_level == CHG_BATT_TEMP_LEVEL_6)
#else
			if (batt_temp_level == CHG_BATT_TEMP_LEVEL_1)
#endif
			{
				pseudo_ui_charging = 1;
			}
#if !defined(CONFIG_MACH_APQ8064_GVKT)
			else if(batt_temp_level == CHG_BATT_TEMP_LEVEL_2 && batt_level > 4000) {
				/* we must show charging image although charging is stopped. */
				pseudo_ui_charging = 0;
			}
#endif
/* CONFIG_PM_E submit ATnT temp scenario kwangjae1.lee */
			pr_debug("%s: BATT TEMP OUT OF SPEC (STATE: %d) (thm: %d) (volt: %d)!.\n",
				__func__, CHG_BATT_NORMAL_STATE, batt_temp, batt_level);

			rtnValue = 1;
		} else if (batt_temp_level == CHG_BATT_TEMP_LEVEL_2 && batt_level <= 4000) {
			chg_batt_temp_state = CHG_BATT_DC_CURRENT_STATE;

			pr_debug("%s: BATT TEMP 46 ~ 55 (STATE: %d) (thm: %d) (volt: %d)!.\n",
				__func__, CHG_BATT_NORMAL_STATE, batt_temp, batt_level);

			rtnValue = 0;

			if (the_chip)
#ifdef CONFIG_LGE_PM
/* changed set current on Battery input kwangjae1.lee@lge.com */
				pm_chg_ibatmax_set(chip, PM8921_CHG_IBATMAX_MIN);
#else
				pm8921_charger_vbus_draw_internal(500);
#endif
#ifdef CONFIG_LGE_WIRELESS_CHARGER
			if(is_dc_chg_plugged_in(chip)) {
				wireless_current_set(chip);
			}
#endif
		} else {
			chg_batt_temp_state = CHG_BATT_NORMAL_STATE;

			pr_debug("%s: BATT TEMP NORMAL (STATE: %d) (thm: %d) (volt: %d)!.\n",
				__func__, CHG_BATT_NORMAL_STATE, batt_temp, batt_level);

			rtnValue = 0;
		}
		break;

	case CHG_BATT_DC_CURRENT_STATE:
		if (batt_temp_level == CHG_BATT_TEMP_LEVEL_1 ||
			batt_temp_level == CHG_BATT_TEMP_LEVEL_6 ||
			(batt_temp_level == CHG_BATT_TEMP_LEVEL_2 && batt_level > 4000)) {

			chg_batt_temp_state = CHG_BATT_STOP_CHARGING_STATE;
			/*
			 * kiwone.seo@lge.com 2011-0609
			 * for show charging ani.although charging is stopped : charging scenario
			 */
/* CONFIG_PM_S submit ATnT temp scenario kwangjae1.lee */
			/* Change termal scenario to LGE ver 1.6 - charge Icon under -10 */
#if defined(CONFIG_MACH_APQ8064_GVDCM) || defined(CONFIG_MACH_APQ8064_J1D)
			if (batt_temp_level == CHG_BATT_TEMP_LEVEL_1 ||
				batt_temp_level == CHG_BATT_TEMP_LEVEL_6)
#else
			if (batt_temp_level == CHG_BATT_TEMP_LEVEL_1)
#endif
			{
				pseudo_ui_charging = 1;
			}
#if !defined(CONFIG_MACH_APQ8064_GVKT)
			else if(batt_temp_level == CHG_BATT_TEMP_LEVEL_2 && batt_level > 4000) {
				/* we must show charging image although charging is stopped. */
				pseudo_ui_charging = 0;
			}
#endif
/* CONFIG_PM_E submit ATnT temp scenario kwangjae1.lee */
			pr_debug("%s: BATT TEMP OUT OF SPEC (STATE: %d) (thm: %d) (volt: %d)!.\n",
				__func__, CHG_BATT_DC_CURRENT_STATE, batt_temp, batt_level);

			rtnValue = 1;

		} else if ((batt_temp_level == CHG_BATT_TEMP_LEVEL_2 ||
					batt_temp_level == CHG_BATT_TEMP_LEVEL_3) && batt_level <= 4000) {

			chg_batt_temp_state = CHG_BATT_DC_CURRENT_STATE;

			pr_debug("%s: BATT TEMP 46 ~ 55 (STATE: %d) (thm: %d) (volt: %d)!.\n",
				__func__, CHG_BATT_DC_CURRENT_STATE, batt_temp, batt_level);

			rtnValue = 0;

		} else if (batt_temp_level == CHG_BATT_TEMP_LEVEL_4) {

			chg_batt_temp_state = CHG_BATT_NORMAL_STATE;

			pr_debug("%s: BATT TEMP NORMAL (STATE: %d) (thm: %d) (volt: %d)!.\n",
				__func__, CHG_BATT_DC_CURRENT_STATE, batt_temp, batt_level);

			rtnValue = 0;

			if (the_chip){
#ifdef CONFIG_LGE_PM
/* changed set current on Battery input kwangjae1.lee@lge.com */
				pm_chg_ibatmax_set(chip, chip->max_bat_chg_current);
#else
				pm8921_charger_vbus_draw(last_usb_chg_current);
#endif
#ifdef CONFIG_LGE_WIRELESS_CHARGER
			if(is_dc_chg_plugged_in(chip)) {
				wireless_current_set(chip);
			}
#endif

				pr_err("%s: BATT TEMP NORMAL last_usb_chg_current : %d)!.\n",
					__func__, last_usb_chg_current);
			}
		} else {
			chg_batt_temp_state = CHG_BATT_DC_CURRENT_STATE;

			pr_debug("%s: BATT TEMP UNREAL (STATE: %d) (thm: %d) (volt: %d)!.\n",
				__func__, CHG_BATT_DC_CURRENT_STATE, batt_temp, batt_level);

			rtnValue = 0;
		}
		break;

	case CHG_BATT_STOP_CHARGING_STATE:
#if defined(CONFIG_MACH_APQ8064_J1SP)
		/* Add temp for charing scenario on SPRINT */
		if (batt_temp_level == CHG_BATT_TEMP_LEVEL_1_1)
#else
		if (batt_temp_level == CHG_BATT_TEMP_LEVEL_4)
#endif
		{
			chg_batt_temp_state = CHG_BATT_NORMAL_STATE;

			pr_err("%s: BATT TEMP NORMAL (STATE: %d) (thm: %d) (volt: %d)!.\n",
				__func__, CHG_BATT_STOP_CHARGING_STATE, batt_temp, batt_level);

			rtnValue = 0;

			if (the_chip){
#ifdef CONFIG_LGE_PM
/* changed set current on Battery input kwangjae1.lee@lge.com */
				pm_chg_ibatmax_set(chip, chip->max_bat_chg_current);
#else
				pm8921_charger_vbus_draw(last_usb_chg_current);
#endif
#ifdef CONFIG_LGE_WIRELESS_CHARGER
			if(is_dc_chg_plugged_in(chip)) {
				wireless_current_set(chip);
			}
#endif

				pr_err("%s: BATT TEMP NORMAL last_usb_chg_current : %d)!.\n",
					__func__, last_usb_chg_current);
			}
		}
		/*
		 * kiwone.seo@lge.com 2011-05-12, charging scenario. do not anything.
		 */
#if 0
		else if ((batt_temp_level == CHG_BATT_TEMP_46_55 && batt_level <= 4000) ||
					batt_temp_level == CHG_BATT_TEMP_42_45) {
			chg_batt_temp_state = CHG_BATT_DC_CURRENT_STATE;

			pr_debug("%s: BATT TEMP NORMAL (STATE: %d) (thm: %d) (volt: %d)!.",
				__func__,CHG_BATT_STOP_CHARGING_STATE, batt_temp,batt_level);

			rtnValue = 0;

			if (the_chip)
				pm8921_charger_vbus_draw_internal(500);
		}
#endif

#ifdef CONFIG_LGE_WIRELESS_CHARGER
		else if(batt_temp_level == CHG_BATT_TEMP_LEVEL_3 && is_dc_chg_plugged_in(chip)) {
			chg_batt_temp_state = CHG_BATT_DC_CURRENT_STATE;
			printk(KERN_INFO "[PM][WIRELESS] BATT TEMP NORMAL : STOP ---> DECREASE , TEMP=%d , VOLT=%d\n",batt_temp,batt_level);
			rtnValue = 0;
			wireless_current_set(chip);
			#if defined(CONFIG_MACH_MSM8960_D1LU) || defined(CONFIG_MACH_MSM8960_D1LSK) || defined(CONFIG_MACH_MSM8960_D1LKT) || defined(CONFIG_MACH_MSM8960_L_DCM)
			printk(KERN_INFO "[WIRELESS] batt_health -> POWER_SUPPLY_HEALTH_GOOD\n");
			batt_health=POWER_SUPPLY_HEALTH_GOOD;
			#endif
		}
#endif
		else {
			chg_batt_temp_state = CHG_BATT_STOP_CHARGING_STATE;
			/*
			 * kiwone.seo@lge.com 2011-0621
			 * 1. stop charging
			 * 2. remove charger
			 * 3 insert charger while stop charging */
/* CONFIG_PM_S submit ATnT temp scenario kwangjae1.lee */
			/* Change termal scenario to LGE ver 1.6 - charge Icon under -10 */
#if defined(CONFIG_MACH_APQ8064_GVDCM) || defined(CONFIG_MACH_APQ8064_J1D)
			if (batt_temp_level == CHG_BATT_TEMP_LEVEL_1 ||
				batt_temp_level == CHG_BATT_TEMP_LEVEL_6)
#else
			if (batt_temp_level == CHG_BATT_TEMP_LEVEL_1)
#endif
			{
				pseudo_ui_charging = 1;
			}
#if !defined(CONFIG_MACH_APQ8064_GVKT)
			else if(batt_temp_level == CHG_BATT_TEMP_LEVEL_2 && batt_level > 4000) {
				 /* we must show charging image although charging is stopped. */
				pseudo_ui_charging = 0;
			}
#endif
/* CONFIG_PM_E submit ATnT temp scenario kwangjae1.lee */
			pr_debug("%s: BATT TEMP OUT OF SPEC (STATE: %d) (thm: %d) (volt: %d)!.\n",
				__func__,CHG_BATT_STOP_CHARGING_STATE, batt_temp,batt_level);

			rtnValue = 1;
		}
		break;
	}

	return rtnValue;
}
#endif

#ifndef CONFIG_LGE_WIRELESS_CHARGER
static int __pm_batt_external_power_changed_work(struct device *dev, void *data)
{
	struct power_supply *psy = &the_chip->batt_psy;
	struct power_supply *epsy = dev_get_drvdata(dev);
	int i, dcin_irq;

	/* Only search for external supply if none is registered */
	if (!the_chip->ext_psy) {
		dcin_irq = the_chip->pmic_chg_irq[DCIN_VALID_IRQ];
		for (i = 0; i < epsy->num_supplicants; i++) {
			if (!strncmp(epsy->supplied_to[i], psy->name, 7)) {
				if (!strncmp(epsy->name, "dc", 2)) {
					the_chip->ext_psy = epsy;
					dcin_valid_irq_handler(dcin_irq,
							the_chip);
				}
			}
		}
	}
	return 0;
}

static void pm_batt_external_power_changed(struct power_supply *psy)
{
	/* Only look for an external supply if it hasn't been registered */
	if (!the_chip->ext_psy)
		class_for_each_device(power_supply_class, NULL, psy,
					 __pm_batt_external_power_changed_work);
}
#endif

/**
 * update_heartbeat - internal function to update userspace
 *		per update_time minutes
 *
 */
#ifdef CONFIG_LGE_CHARGER_TEMP_SCENARIO
/* battery of therm H/W register level reading kwangjae1.lee@lge.com */
static int get_rt_get_temp(struct pm8921_chg_chip *chip)
{
	int rc;
	struct pm8xxx_adc_chan_result result;

	rc = pm8xxx_adc_read(chip->batt_temp_channel, &result);
	if (rc) {
		pr_err("error reading adc channel = %d, rc = %d\n",
					chip->vbat_channel, rc);
		return rc;
	}
	pr_debug("batt_temp phy = %lld  meas = 0x%llx\n", result.physical,
		 result.measurement);

	return (int)result.adc_value;
}
#endif
#ifdef CONFIG_BATTERY_MAX17047
static void fuel_gauage_update_data(void)
{
#if defined(CONFIG_MACH_APQ8064_GVDCM)
	if(lge_get_board_revno() > HW_REV_A)
	{
		g_batt_vol = max17047_get_battery_mvolts();
		g_batt_soc = max17047_get_battery_capacity_percent();
		/* battery age update add */
		pr_info(" fuel_gauage_update_data: g_batt_soc(%d), g_batt_vol(%d)\n", g_batt_soc,  g_batt_vol);

	}
#else
	{
		g_batt_vol = max17047_get_battery_mvolts();
		g_batt_soc = max17047_get_battery_capacity_percent();
		/* battery age update add */
		pr_info("fuel_gauage_update_data: g_batt_soc(%d), g_batt_vol(%d)\n", g_batt_soc,  g_batt_vol);
	}
#endif
/*doosan.baek@lge.com 20121108 Add battery condition */
        lge_pm_battery_age_update();

}
#endif

#define LOW_SOC_HEARTBEAT_MS	20000
static void update_heartbeat(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct pm8921_chg_chip *chip = container_of(dwork,
				struct pm8921_chg_chip, update_heartbeat_work);

	pm_chg_failed_clear(chip, 1);
#ifdef CONFIG_BATTERY_MAX17047
	fuel_gauage_update_data();
#endif
	power_supply_changed(&chip->batt_psy);
#ifdef CONFIG_BATTERY_MAX17047
	if (g_batt_soc <= 20)
#else
	if (chip->recent_reported_soc <= 20)
#endif
		schedule_delayed_work(&chip->update_heartbeat_work,
			      round_jiffies_relative(msecs_to_jiffies
						     (LOW_SOC_HEARTBEAT_MS)));
	else
		schedule_delayed_work(&chip->update_heartbeat_work,
			      round_jiffies_relative(msecs_to_jiffies
						     (chip->update_time)));
}
#define VDD_LOOP_ACTIVE_BIT	BIT(3)
#define VDD_MAX_INCREASE_MV	400
static int vdd_max_increase_mv = VDD_MAX_INCREASE_MV;
module_param(vdd_max_increase_mv, int, 0644);

static int ichg_threshold_ua = -400000;
module_param(ichg_threshold_ua, int, 0644);

#define PM8921_CHG_VDDMAX_RES_MV	10
static void adjust_vdd_max_for_fastchg(struct pm8921_chg_chip *chip,
						int vbat_batt_terminal_uv)
{
	int adj_vdd_max_mv, programmed_vdd_max;
	int vbat_batt_terminal_mv;
	int reg_loop;
	int delta_mv = 0;

	if (chip->rconn_mohm == 0) {
		pr_debug("Exiting as rconn_mohm is 0\n");
		return;
	}
	/* adjust vdd_max only in normal temperature zone */
	if (chip->is_bat_cool || chip->is_bat_warm) {
		pr_debug("Exiting is_bat_cool = %d is_batt_warm = %d\n",
				chip->is_bat_cool, chip->is_bat_warm);
		return;
	}

	reg_loop = pm_chg_get_regulation_loop(chip);
	if (!(reg_loop & VDD_LOOP_ACTIVE_BIT)) {
		pr_debug("Exiting Vdd loop is not active reg loop=0x%x\n",
			reg_loop);
		return;
	}
	vbat_batt_terminal_mv = vbat_batt_terminal_uv/1000;
	pm_chg_vddmax_get(the_chip, &programmed_vdd_max);

	delta_mv =  chip->max_voltage_mv - vbat_batt_terminal_mv;

	adj_vdd_max_mv = programmed_vdd_max + delta_mv;
	pr_debug("vdd_max needs to be changed by %d mv from %d to %d\n",
			delta_mv,
			programmed_vdd_max,
			adj_vdd_max_mv);

	if (adj_vdd_max_mv < chip->max_voltage_mv) {
		pr_debug("adj vdd_max lower than default max voltage\n");
		return;
	}

	if (adj_vdd_max_mv > (chip->max_voltage_mv + vdd_max_increase_mv))
		adj_vdd_max_mv = chip->max_voltage_mv + vdd_max_increase_mv;

	pr_debug("adjusting vdd_max_mv to %d to make "
		"vbat_batt_termial_uv = %d to %d\n",
		adj_vdd_max_mv, vbat_batt_terminal_uv, chip->max_voltage_mv);
	pm_chg_vddmax_set(chip, adj_vdd_max_mv);
}

enum {
	CHG_IN_PROGRESS,
	CHG_NOT_IN_PROGRESS,
	CHG_FINISHED,
};

#define VBAT_TOLERANCE_MV	70
#define CHG_DISABLE_MSLEEP	100
static int is_charging_finished(struct pm8921_chg_chip *chip,
			int vbat_batt_terminal_uv, int ichg_meas_ma)
{
	int vbat_programmed, iterm_programmed, vbat_intended, vbatdet_low;
	int regulation_loop, fast_chg, vcp;
	int rc;
	static int last_vbat_programmed = -EINVAL;

	if (!is_ext_charging(chip)) {
		/* return if the battery is not being fastcharged */
		fast_chg = pm_chg_get_rt_status(chip, FASTCHG_IRQ);
		pr_debug("fast_chg = %d\n", fast_chg);
		if (fast_chg == 0)
			return CHG_NOT_IN_PROGRESS;

		vcp = pm_chg_get_rt_status(chip, VCP_IRQ);
		pr_debug("vcp = %d\n", vcp);
		if (vcp == 1)
			return CHG_IN_PROGRESS;

		vbatdet_low = pm_chg_get_rt_status(chip, VBATDET_LOW_IRQ);
		pr_debug("vbatdet_low = %d\n", vbatdet_low);
		if (vbatdet_low == 1)
			return CHG_IN_PROGRESS;

		/* reset count if battery is hot/cold */
		rc = pm_chg_get_rt_status(chip, BAT_TEMP_OK_IRQ);
		pr_debug("batt_temp_ok = %d\n", rc);
		if (rc == 0)
			return CHG_IN_PROGRESS;

		rc = pm_chg_vddmax_get(chip, &vbat_programmed);
		if (rc) {
			pr_err("couldnt read vddmax rc = %d\n", rc);
			return CHG_IN_PROGRESS;
		}
		pr_debug("vddmax = %d vbat_batt_terminal_uv=%d\n",
			 vbat_programmed, vbat_batt_terminal_uv);

#if defined (CONFIG_BATTERY_MAX17047)
#if defined(CONFIG_MACH_APQ8064_GVDCM)
	if(lge_get_board_revno() > HW_REV_A) 
	{
		g_batt_vol = max17047_get_battery_mvolts();//must read vbat_vol here

		if(( g_batt_vol < (CHG_COMPLETE_VOL - CHG_COMPLETE_TOLERANCE))
			||( g_batt_soc < 100 ) )
		{
			pr_info("g_batt_vol = %d g_batt_soc=%d\n",
				 g_batt_vol, g_batt_soc);
			return CHG_IN_PROGRESS;
		}
		/* QCT code not used */
		vbat_intended = chip->max_voltage_mv;
		last_vbat_programmed = vbat_programmed;

	}
	else
	{
		if (last_vbat_programmed == -EINVAL)
			last_vbat_programmed = vbat_programmed;
		if (last_vbat_programmed !=  vbat_programmed) {
			/* vddmax changed, reset and check again */
			pr_info("vddmax = %d last_vdd_max=%d\n",
				 vbat_programmed, last_vbat_programmed);
			last_vbat_programmed = vbat_programmed;
			return CHG_IN_PROGRESS;
		}
	

		if (chip->is_bat_cool)
			vbat_intended = chip->cool_bat_voltage;
		else if (chip->is_bat_warm)
			vbat_intended = chip->warm_bat_voltage;
		else
			vbat_intended = chip->max_voltage_mv;
	}

#else
		g_batt_vol = max17047_get_battery_mvolts();//must read vbat_vol here

		if(( g_batt_vol < (CHG_COMPLETE_VOL - CHG_COMPLETE_TOLERANCE))
			||( g_batt_soc < 100 ) )
		{
			pr_info("g_batt_vol = %d g_batt_soc=%d\n",
				 g_batt_vol, g_batt_soc);
			return CHG_IN_PROGRESS;
		}

		/* QCT code not used */
		vbat_intended = chip->max_voltage_mv;
		last_vbat_programmed = vbat_programmed;
#endif		
#else
		if (last_vbat_programmed == -EINVAL)
			last_vbat_programmed = vbat_programmed;
		if (last_vbat_programmed !=  vbat_programmed) {
			/* vddmax changed, reset and check again */
			pr_debug("vddmax = %d last_vdd_max=%d\n",
				 vbat_programmed, last_vbat_programmed);
			last_vbat_programmed = vbat_programmed;
			return CHG_IN_PROGRESS;
		}

		if (chip->is_bat_cool)
			vbat_intended = chip->cool_bat_voltage;
		else if (chip->is_bat_warm)
			vbat_intended = chip->warm_bat_voltage;
		else
			vbat_intended = chip->max_voltage_mv;
#endif

#ifndef CONFIG_LGE_PM
 /* This is Qualcomm code, but LGE G-model does not want this. */
 		if (vbat_batt_terminal_uv / 1000 < vbat_intended) {
			pr_debug("terminal_uv:%d < vbat_intended:%d.\n",
							vbat_batt_terminal_uv,
							vbat_intended);
			return CHG_IN_PROGRESS;
		}
#endif

		regulation_loop = pm_chg_get_regulation_loop(chip);
		if (regulation_loop < 0) {
			pr_err("couldnt read the regulation loop err=%d\n",
				regulation_loop);
			return CHG_IN_PROGRESS;
		}
		pr_debug("regulation_loop=%d\n", regulation_loop);

		if (regulation_loop != 0 && regulation_loop != VDD_LOOP)
			return CHG_IN_PROGRESS;
	} /* !is_ext_charging */

	/* reset count if battery chg current is more than iterm */
	rc = pm_chg_iterm_get(chip, &iterm_programmed);
	if (rc) {
		pr_err("couldnt read iterm rc = %d\n", rc);
		return CHG_IN_PROGRESS;
	}

	pr_info("iterm_programmed = %d ichg_meas_ma=%d\n",
				iterm_programmed, ichg_meas_ma);
	/*
	 * ichg_meas_ma < 0 means battery is drawing current
	 * ichg_meas_ma > 0 means battery is providing current
	 */
	if (ichg_meas_ma > 0)
		return CHG_IN_PROGRESS;

	if (ichg_meas_ma * -1 > iterm_programmed)
		return CHG_IN_PROGRESS;

	return CHG_FINISHED;
}

/**
 * eoc_worker - internal function to check if battery EOC
 *		has happened
 *
 * If all conditions favouring, if the charge current is
 * less than the term current for three consecutive times
 * an EOC has happened.
 * The wakelock is released if there is no need to reshedule
 * - this happens when the battery is removed or EOC has
 * happened
 */
#define CONSECUTIVE_COUNT	3
static void eoc_worker(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct pm8921_chg_chip *chip = container_of(dwork,
				struct pm8921_chg_chip, eoc_work);
	static int count;
	int end;
	int vbat_meas_uv, vbat_meas_mv;
	int ichg_meas_ua, ichg_meas_ma;
	int vbat_batt_terminal_uv;

	pm_chg_failed_clear(chip, 1);
#ifdef CONFIG_LGE_WIRELESS_CHARGER
        pm8921_bms_get_simultaneous_battery_voltage_and_current(
                                        &ichg_meas_ua,  &vbat_meas_uv);
        vbat_meas_mv = vbat_meas_uv / 1000;
        /* rconn_mohm is in milliOhms */
        ichg_meas_ma = ichg_meas_ua / 1000;
        vbat_batt_terminal_uv = vbat_meas_uv
                                        + ichg_meas_ma
                                        * the_chip->rconn_mohm;

	if (is_usb_chg_plugged_in(chip)) {
		printk(KERN_INFO "eoc_worker\n");
		//end = is_charging_finished(chip);
		end = is_charging_finished(chip, vbat_batt_terminal_uv, ichg_meas_ma);
	}
	else if(is_dc_chg_plugged_in(chip)) {
		printk(KERN_INFO "[WIRELESS] eoc_worker : WLC ====== \n");
		end = wireless_is_charging_finished(chip);
	}
	else {
		printk(KERN_INFO "eoc_worker : No Charger\n");
		end = CHG_NOT_IN_PROGRESS;
	}
#else
#ifdef CONFIG_BATTERY_MAX17047
#if defined(CONFIG_MACH_APQ8064_GVDCM)
	if(lge_get_board_revno() > HW_REV_A) 
	{
		vbat_meas_mv = max17047_get_batt_vol();
		vbat_meas_uv = vbat_meas_mv * 1000;
		ichg_meas_ma = -1 * max17047_get_battery_current();
		ichg_meas_ua = 1000 * ichg_meas_ma;

		pr_info("max17047 : vbat_meas_mv = %d ichg_meas_ma=%d\n",
			vbat_meas_mv, ichg_meas_ma);
	}
	else
	{
		pm8921_bms_get_simultaneous_battery_voltage_and_current(
		                &ichg_meas_ua,  &vbat_meas_uv);
		vbat_meas_mv = vbat_meas_uv / 1000;
		/* rconn_mohm is in milliOhms */
		ichg_meas_ma = ichg_meas_ua / 1000;
	}
#else
	vbat_meas_mv = max17047_get_batt_vol();
	vbat_meas_uv = vbat_meas_mv * 1000;
	ichg_meas_ma = -1 * max17047_get_battery_current();
	ichg_meas_ua = 1000 * ichg_meas_ma;

	pr_info("max17047 : vbat_meas_mv = %d ichg_meas_ma=%d\n",
		vbat_meas_mv, ichg_meas_ma);
#endif
#else
        pm8921_bms_get_simultaneous_battery_voltage_and_current(
                                        &ichg_meas_ua,  &vbat_meas_uv);
        vbat_meas_mv = vbat_meas_uv / 1000;
        /* rconn_mohm is in milliOhms */
        ichg_meas_ma = ichg_meas_ua / 1000;
#endif
		
        vbat_batt_terminal_uv = vbat_meas_uv
                                        + ichg_meas_ma
                                        * the_chip->rconn_mohm;

        end = is_charging_finished(chip, vbat_batt_terminal_uv, ichg_meas_ma);
#endif

	if (end == CHG_NOT_IN_PROGRESS) {
		count = 0;
		wake_unlock(&chip->eoc_wake_lock);
		return;
	}

	if (end == CHG_FINISHED) {
		count++;
	} else {
		count = 0;
	}

	if (count == CONSECUTIVE_COUNT) {
		count = 0;
		pr_info("End of Charging\n");
		/* set the vbatdet back, in case it was changed
		 * to trigger charging */
		if (chip->is_bat_cool) {
			pm_chg_vbatdet_set(the_chip,
				the_chip->cool_bat_voltage
				- the_chip->resume_voltage_delta);
		} else if (chip->is_bat_warm) {
			pm_chg_vbatdet_set(the_chip,
				the_chip->warm_bat_voltage
				- the_chip->resume_voltage_delta);
		} else {
			pm_chg_vbatdet_set(the_chip,
				the_chip->max_voltage_mv
				- the_chip->resume_voltage_delta);
		}

#ifdef CONFIG_LGE_WIRELESS_CHARGER
		if(is_dc_chg_plugged_in(chip)) {
			wireless_chip_control_worker(&chip->wireless_chip_control_work.work);
			wireless_charge_done = true;
		}
#endif
		pm_chg_auto_enable(chip, 0);

		if (is_ext_charging(chip))
			chip->ext_charge_done = true;

		if (chip->is_bat_warm || chip->is_bat_cool)
			chip->bms_notify.is_battery_full = 0;
		else
			chip->bms_notify.is_battery_full = 1;
		/* declare end of charging by invoking chgdone interrupt */
		chgdone_irq_handler(chip->pmic_chg_irq[CHGDONE_IRQ], chip);
#ifdef CONFIG_LGE_PM
		/* Run EOC Worker once more befor enter sleep mode to give a chance handling the uevent */
		schedule_delayed_work(&chip->eoc_work,
				  round_jiffies_relative(msecs_to_jiffies
							 (EOC_CHECK_PERIOD_MS)));
#else
		wake_unlock(&chip->eoc_wake_lock);
#endif
	} else {
                adjust_vdd_max_for_fastchg(chip, vbat_batt_terminal_uv);
		pr_debug("EOC count = %d\n", count);
		schedule_delayed_work(&chip->eoc_work,
			      round_jiffies_relative(msecs_to_jiffies
						     (EOC_CHECK_PERIOD_MS)));
	}
}

static void btm_configure_work(struct work_struct *work)
{
	int rc;

	rc = pm8xxx_adc_btm_configure(&btm_config);
	if (rc)
		pr_err("failed to configure btm rc=%d", rc);
}

DECLARE_WORK(btm_config_work, btm_configure_work);

static void set_appropriate_battery_current(struct pm8921_chg_chip *chip)
{
	unsigned int chg_current = chip->max_bat_chg_current;

	if (chip->is_bat_cool)
		chg_current = min(chg_current, chip->cool_bat_chg_current);

	if (chip->is_bat_warm)
		chg_current = min(chg_current, chip->warm_bat_chg_current);

#ifndef CONFIG_LGE_PM
	/*
	 * do nothing, we just wanna get "thermal_mitigation" values.
	 * we will use this value to set charging current setting.
	 * and it should not operate origin code.
	 */
	if (thermal_mitigation != 0 && chip->thermal_mitigation)
		chg_current = min(chg_current,
				chip->thermal_mitigation[thermal_mitigation]);
#endif
	pm_chg_ibatmax_set(the_chip, chg_current);
}

#define TEMP_HYSTERISIS_DEGC 2
static void battery_cool(bool enter)
{
	pr_debug("enter = %d\n", enter);
	ChgLog("ChgLog> %s : %d\n", __func__,enter);
	if (enter == the_chip->is_bat_cool)
		return;
	the_chip->is_bat_cool = enter;
	if (enter) {
		btm_config.low_thr_temp =
			the_chip->cool_temp_dc + TEMP_HYSTERISIS_DEGC;
		set_appropriate_battery_current(the_chip);
		pm_chg_vddmax_set(the_chip, the_chip->cool_bat_voltage);
		pm_chg_vbatdet_set(the_chip,
			the_chip->cool_bat_voltage
			- the_chip->resume_voltage_delta);
	} else {
		btm_config.low_thr_temp = the_chip->cool_temp_dc;
		set_appropriate_battery_current(the_chip);
		pm_chg_vddmax_set(the_chip, the_chip->max_voltage_mv);
		pm_chg_vbatdet_set(the_chip,
			the_chip->max_voltage_mv
			- the_chip->resume_voltage_delta);
	}
	schedule_work(&btm_config_work);
}

static void battery_warm(bool enter)
{
	pr_debug("enter = %d\n", enter);
	ChgLog("ChgLog> %s : %d\n", __func__,enter);
	if (enter == the_chip->is_bat_warm)
		return;
	the_chip->is_bat_warm = enter;
	if (enter) {
		btm_config.high_thr_temp =
			the_chip->warm_temp_dc - TEMP_HYSTERISIS_DEGC;
		set_appropriate_battery_current(the_chip);
		pm_chg_vddmax_set(the_chip, the_chip->warm_bat_voltage);
		pm_chg_vbatdet_set(the_chip,
			the_chip->warm_bat_voltage
			- the_chip->resume_voltage_delta);
	} else {
		btm_config.high_thr_temp = the_chip->warm_temp_dc;
		set_appropriate_battery_current(the_chip);
		pm_chg_vddmax_set(the_chip, the_chip->max_voltage_mv);
		pm_chg_vbatdet_set(the_chip,
			the_chip->max_voltage_mv
			- the_chip->resume_voltage_delta);
	}
	schedule_work(&btm_config_work);
}

static int configure_btm(struct pm8921_chg_chip *chip)
{
	int rc;

	if (chip->warm_temp_dc != INT_MIN)
		btm_config.btm_warm_fn = battery_warm;
	else
		btm_config.btm_warm_fn = NULL;

	if (chip->cool_temp_dc != INT_MIN)
		btm_config.btm_cool_fn = battery_cool;
	else
		btm_config.btm_cool_fn = NULL;

	btm_config.low_thr_temp = chip->cool_temp_dc;
	btm_config.high_thr_temp = chip->warm_temp_dc;
	btm_config.interval = chip->temp_check_period;
	rc = pm8xxx_adc_btm_configure(&btm_config);
	if (rc)
		pr_err("failed to configure btm rc = %d\n", rc);
	rc = pm8xxx_adc_btm_start();
	if (rc)
		pr_err("failed to start btm rc = %d\n", rc);

	return rc;
}

/**
 * set_disable_status_param -
 *
 * Internal function to disable battery charging and also disable drawing
 * any current from the source. The device is forced to run on a battery
 * after this.
 */
static int set_disable_status_param(const char *val, struct kernel_param *kp)
{
	int ret;
	struct pm8921_chg_chip *chip = the_chip;

	ret = param_set_int(val, kp);
	if (ret) {
		pr_err("error setting value %d\n", ret);
		return ret;
	}
	pr_info("factory set disable param to %d\n", charging_disabled);
	if (chip) {
		pm_chg_auto_enable(chip, !charging_disabled);
		pm_chg_charge_dis(chip, charging_disabled);
	}
	return 0;
}
module_param_call(disabled, set_disable_status_param, param_get_uint,
					&charging_disabled, 0644);

static int rconn_mohm;
static int set_rconn_mohm(const char *val, struct kernel_param *kp)
{
	int ret;
	struct pm8921_chg_chip *chip = the_chip;

	ret = param_set_int(val, kp);
	if (ret) {
		pr_err("error setting value %d\n", ret);
		return ret;
	}
	if (chip)
		chip->rconn_mohm = rconn_mohm;
	return 0;
}
module_param_call(rconn_mohm, set_rconn_mohm, param_get_uint,
					&rconn_mohm, 0644);
/**
 * set_thermal_mitigation_level -
 *
 * Internal function to control battery charging current to reduce
 * temperature
 */
static int set_therm_mitigation_level(const char *val, struct kernel_param *kp)
{
	int ret;
	struct pm8921_chg_chip *chip = the_chip;

	ret = param_set_int(val, kp);
	if (ret) {
		pr_err("error setting value %d\n", ret);
		return ret;
	}

	if (!chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}

	if (!chip->thermal_mitigation) {
		pr_err("no thermal mitigation\n");
		return -EINVAL;
	}

#ifndef CONFIG_LGE_PM
	/*
	 * do nothing, we just wanna get "thermal_mitigation" values.
	 * we will use this value to set charging current setting.
	 * and it should not operate origin code.
	 */
	if (thermal_mitigation < 0
		|| thermal_mitigation >= chip->thermal_levels) {
		pr_err("out of bound level selected\n");
		return -EINVAL;
	}
	set_appropriate_battery_current(chip);
#endif

	return ret;
}
module_param_call(thermal_mitigation, set_therm_mitigation_level,
					param_get_uint,
					&thermal_mitigation, 0644);

static int set_usb_max_current(const char *val, struct kernel_param *kp)
{
	int ret, mA;
	struct pm8921_chg_chip *chip = the_chip;

	ret = param_set_int(val, kp);
	if (ret) {
		pr_err("error setting value %d\n", ret);
		return ret;
	}
	if (chip) {
		pr_warn("setting current max to %d\n", usb_max_current);
		pm_chg_iusbmax_get(chip, &mA);
		if (mA > usb_max_current)
			pm8921_charger_vbus_draw(usb_max_current);
		return 0;
	}
	return -EINVAL;
}
module_param_call(usb_max_current, set_usb_max_current,
	param_get_uint, &usb_max_current, 0644);

#ifdef CONFIG_LGE_PM_LOW_BATT_CHG
/* this is indicator that chargerlogo app is running. */
/* sysfs : /sys/module/pm8921_charger/parameters/chargerlogo_state */
static int param_set_chargerlogo_state(const char *val, struct kernel_param *kp)
{
	int ret;

	ret = param_set_int(val, kp);
	if (ret) {
		pr_err("error setting value %d\n", ret);
		return ret;
	}

	pr_info("set chargerlogo_state to %d\n", chargerlogo_state);
	return 0;
}
module_param_call(chargerlogo_state, param_set_chargerlogo_state,
	param_get_uint, &chargerlogo_state, 0644);
#endif

static void free_irqs(struct pm8921_chg_chip *chip)
{
	int i;

	for (i = 0; i < PM_CHG_MAX_INTS; i++)
		if (chip->pmic_chg_irq[i]) {
			free_irq(chip->pmic_chg_irq[i], chip);
			chip->pmic_chg_irq[i] = 0;
		}
}

/* determines the initial present states */
static void __devinit determine_initial_state(struct pm8921_chg_chip *chip)
{
	unsigned long flags;
	int fsm_state;
	int is_fast_chg;

#ifndef CONFIG_LGE_WIRELESS_CHARGER
	chip->dc_present = !!is_dc_chg_plugged_in(chip);
#endif
	chip->usb_present = !!is_usb_chg_plugged_in(chip);

	notify_usb_of_the_plugin_event(chip->usb_present);
	if (chip->usb_present) {
		schedule_delayed_work(&chip->unplug_check_work,
			round_jiffies_relative(msecs_to_jiffies
				(UNPLUG_CHECK_WAIT_PERIOD_MS)));
#ifndef CONFIG_LGE_PM
		pm8921_chg_enable_irq(chip, CHG_GONE_IRQ); //Removed - seonghun1.kim
#endif
	}

	pm8921_chg_enable_irq(chip, DCIN_VALID_IRQ);
	pm8921_chg_enable_irq(chip, USBIN_VALID_IRQ);
	pm8921_chg_enable_irq(chip, BATT_REMOVED_IRQ);
	pm8921_chg_enable_irq(chip, BATT_INSERTED_IRQ);
	pm8921_chg_enable_irq(chip, DCIN_OV_IRQ);
	pm8921_chg_enable_irq(chip, DCIN_UV_IRQ);
	pm8921_chg_enable_irq(chip, CHGFAIL_IRQ);
	pm8921_chg_enable_irq(chip, FASTCHG_IRQ);
	pm8921_chg_enable_irq(chip, VBATDET_LOW_IRQ);
	pm8921_chg_enable_irq(chip, BAT_TEMP_OK_IRQ);

#ifdef CONFIG_LGE_PM
	/* return control to PMIC FSM */
	pr_info("boot battery present=%d\n", pm_chg_get_rt_status(chip, BATT_INSERTED_IRQ));
	if( pm_chg_batfet_get(chip) > 0 && pm_chg_get_rt_status(chip, BATT_INSERTED_IRQ) > 0 )
	{
		pm_chg_batfet_set(chip, 0);
	}
#endif

	spin_lock_irqsave(&vbus_lock, flags);
#ifdef CONFIG_LGE_PM
	/* Fix for EOC check at Power on - for Charger Logo */
	if (usb_chg_current || pm_chg_get_rt_status(chip, USBIN_VALID_IRQ)) {
		if (usb_chg_current)
			__pm8921_charger_vbus_draw(usb_chg_current);
		fastchg_irq_handler(chip->pmic_chg_irq[FASTCHG_IRQ], chip);
	}
#else
	if (usb_chg_current) {
		/* reissue a vbus draw call */
		__pm8921_charger_vbus_draw(usb_chg_current);
	}
#endif
	spin_unlock_irqrestore(&vbus_lock, flags);
	/*
	 * The bootloader could have started charging, a fastchg interrupt
	 * might not happen. Check the real time status and if it is fast
	 * charging invoke the handler so that the eoc worker could be
	 * started
	 */
	is_fast_chg = pm_chg_get_rt_status(chip, FASTCHG_IRQ);
	if (is_fast_chg)
		fastchg_irq_handler(chip->pmic_chg_irq[FASTCHG_IRQ], chip);

#ifdef CONFIG_LGE_WIRELESS_CHARGER
	if(pm_chg_get_rt_status(chip,DCIN_VALID_IRQ)) {
		printk(KERN_INFO "[WIRELESS] Boot with WLC_charger, DC_IRQ=%d\n",pm_chg_get_rt_status(chip, DCIN_VALID_IRQ));
		wireless_interrupt_handler(gpio_to_irq(ACTIVE_N),chip);
		dcin_valid_irq_handler(chip->pmic_chg_irq[DCIN_VALID_IRQ], chip);
	}
#endif

	fsm_state = pm_chg_get_fsm_state(chip);
	if (is_battery_charging(fsm_state)) {
		chip->bms_notify.is_charging = 1;
		pm8921_bms_charging_began();
	}

	check_battery_valid(chip);

	pr_debug("usb = %d, dc = %d  batt = %d state=%d\n",
			chip->usb_present,
			chip->dc_present,
			get_prop_batt_present(chip),
			fsm_state);
}

struct pm_chg_irq_init_data {
	unsigned int	irq_id;
	char		*name;
	unsigned long	flags;
	irqreturn_t	(*handler)(int, void *);
};

#define CHG_IRQ(_id, _flags, _handler) \
{ \
	.irq_id		= _id, \
	.name		= #_id, \
	.flags		= _flags, \
	.handler	= _handler, \
}
struct pm_chg_irq_init_data chg_irq_data[] = {
	CHG_IRQ(USBIN_VALID_IRQ, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
						usbin_valid_irq_handler),
	CHG_IRQ(BATT_INSERTED_IRQ, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
						batt_inserted_irq_handler),
	CHG_IRQ(VBATDET_LOW_IRQ, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
						vbatdet_low_irq_handler),
	CHG_IRQ(VBAT_OV_IRQ, IRQF_TRIGGER_RISING, vbat_ov_irq_handler),
	CHG_IRQ(CHGWDOG_IRQ, IRQF_TRIGGER_RISING, chgwdog_irq_handler),
	CHG_IRQ(VCP_IRQ, IRQF_TRIGGER_RISING, vcp_irq_handler),
	CHG_IRQ(ATCDONE_IRQ, IRQF_TRIGGER_RISING, atcdone_irq_handler),
	CHG_IRQ(ATCFAIL_IRQ, IRQF_TRIGGER_RISING, atcfail_irq_handler),
	CHG_IRQ(CHGDONE_IRQ, IRQF_TRIGGER_RISING, chgdone_irq_handler),
	CHG_IRQ(CHGFAIL_IRQ, IRQF_TRIGGER_RISING, chgfail_irq_handler),
	CHG_IRQ(CHGSTATE_IRQ, IRQF_TRIGGER_RISING, chgstate_irq_handler),
	CHG_IRQ(LOOP_CHANGE_IRQ, IRQF_TRIGGER_RISING, loop_change_irq_handler),
	CHG_IRQ(FASTCHG_IRQ, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
						fastchg_irq_handler),
	CHG_IRQ(TRKLCHG_IRQ, IRQF_TRIGGER_RISING, trklchg_irq_handler),
	CHG_IRQ(BATT_REMOVED_IRQ, IRQF_TRIGGER_RISING,
						batt_removed_irq_handler),
	CHG_IRQ(BATTTEMP_HOT_IRQ, IRQF_TRIGGER_RISING,
						batttemp_hot_irq_handler),
	CHG_IRQ(CHGHOT_IRQ, IRQF_TRIGGER_RISING, chghot_irq_handler),
	CHG_IRQ(BATTTEMP_COLD_IRQ, IRQF_TRIGGER_RISING,
						batttemp_cold_irq_handler),
	CHG_IRQ(CHG_GONE_IRQ, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
						chg_gone_irq_handler),
	CHG_IRQ(BAT_TEMP_OK_IRQ, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
						bat_temp_ok_irq_handler),
	CHG_IRQ(COARSE_DET_LOW_IRQ, IRQF_TRIGGER_RISING,
						coarse_det_low_irq_handler),
	CHG_IRQ(VDD_LOOP_IRQ, IRQF_TRIGGER_RISING, vdd_loop_irq_handler),
	CHG_IRQ(VREG_OV_IRQ, IRQF_TRIGGER_RISING, vreg_ov_irq_handler),
	CHG_IRQ(VBATDET_IRQ, IRQF_TRIGGER_RISING, vbatdet_irq_handler),
	CHG_IRQ(BATFET_IRQ, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
						batfet_irq_handler),
	CHG_IRQ(DCIN_VALID_IRQ, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
						dcin_valid_irq_handler),
	CHG_IRQ(DCIN_OV_IRQ, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
						dcin_ov_irq_handler),
	CHG_IRQ(DCIN_UV_IRQ, IRQF_TRIGGER_RISING, dcin_uv_irq_handler),
};

static int __devinit request_irqs(struct pm8921_chg_chip *chip,
					struct platform_device *pdev)
{
	struct resource *res;
	int ret, i;

	ret = 0;
	bitmap_fill(chip->enabled_irqs, PM_CHG_MAX_INTS);

	for (i = 0; i < ARRAY_SIZE(chg_irq_data); i++) {
		res = platform_get_resource_byname(pdev, IORESOURCE_IRQ,
				chg_irq_data[i].name);
		if (res == NULL) {
			pr_err("couldn't find %s\n", chg_irq_data[i].name);
			goto err_out;
		}
		chip->pmic_chg_irq[chg_irq_data[i].irq_id] = res->start;
		ret = request_irq(res->start, chg_irq_data[i].handler,
			chg_irq_data[i].flags,
			chg_irq_data[i].name, chip);
		if (ret < 0) {
			pr_err("couldn't request %d (%s) %d\n", res->start,
					chg_irq_data[i].name, ret);
			chip->pmic_chg_irq[chg_irq_data[i].irq_id] = 0;
			goto err_out;
		}
		pm8921_chg_disable_irq(chip, chg_irq_data[i].irq_id);
	}
	return 0;

err_out:
	free_irqs(chip);
	return -EINVAL;
}

static void pm8921_chg_force_19p2mhz_clk(struct pm8921_chg_chip *chip)
{
	int err;
	u8 temp;

	temp  = 0xD1;
	err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
	if (err) {
		pr_err("Error %d writing %d to addr %d\n", err, temp, CHG_TEST);
		return;
	}

	temp  = 0xD3;
	err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
	if (err) {
		pr_err("Error %d writing %d to addr %d\n", err, temp, CHG_TEST);
		return;
	}

	temp  = 0xD1;
	err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
	if (err) {
		pr_err("Error %d writing %d to addr %d\n", err, temp, CHG_TEST);
		return;
	}

	temp  = 0xD5;
	err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
	if (err) {
		pr_err("Error %d writing %d to addr %d\n", err, temp, CHG_TEST);
		return;
	}

	udelay(183);

	temp  = 0xD1;
	err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
	if (err) {
		pr_err("Error %d writing %d to addr %d\n", err, temp, CHG_TEST);
		return;
	}

	temp  = 0xD0;
	err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
	if (err) {
		pr_err("Error %d writing %d to addr %d\n", err, temp, CHG_TEST);
		return;
	}
	udelay(32);

	temp  = 0xD1;
	err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
	if (err) {
		pr_err("Error %d writing %d to addr %d\n", err, temp, CHG_TEST);
		return;
	}

	temp  = 0xD3;
	err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
	if (err) {
		pr_err("Error %d writing %d to addr %d\n", err, temp, CHG_TEST);
		return;
	}
}

static void pm8921_chg_set_hw_clk_switching(struct pm8921_chg_chip *chip)
{
	int err;
	u8 temp;

	temp  = 0xD1;
	err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
	if (err) {
		pr_err("Error %d writing %d to addr %d\n", err, temp, CHG_TEST);
		return;
	}

	temp  = 0xD0;
	err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
	if (err) {
		pr_err("Error %d writing %d to addr %d\n", err, temp, CHG_TEST);
		return;
	}
}

#define VREF_BATT_THERM_FORCE_ON	BIT(7)
static void detect_battery_removal(struct pm8921_chg_chip *chip)
{
	u8 temp;

	pm8xxx_readb(chip->dev->parent, CHG_CNTRL, &temp);
	pr_debug("upon restart CHG_CNTRL = 0x%x\n",  temp);

	if (!(temp & VREF_BATT_THERM_FORCE_ON))
		/*
		 * batt therm force on bit is battery backed and is default 0
		 * The charger sets this bit at init time. If this bit is found
		 * 0 that means the battery was removed. Tell the bms about it
		 */
		pm8921_bms_invalidate_shutdown_soc();
}

#ifndef QCT_CLK_KICK_START
static void pm8921_turn_on_19p2mhz_clk(void)
{
	int err;
	u8 temp;
	struct pm8921_chg_chip *chip = the_chip;

	if (!the_chip) {
		pr_err("%s called before init\n", __func__);
		return;
	}

	pr_info("Kick Start\n");

	temp  = 0xD1;
	err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
	if (err) {
		pr_err("Error %d writing %d to addr %d\n", err, temp, CHG_TEST);
		return;
	}

	temp  = 0xD3;
	err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
	if (err) {
		pr_err("Error %d writing %d to addr %d\n", err, temp, CHG_TEST);
		return;
	}
}

void pm8921_turn_on_19p2mhz_clk_ext(void)
{
	pm8921_turn_on_19p2mhz_clk();
}
#endif

#define ENUM_TIMER_STOP_BIT	BIT(1)
#define BOOT_DONE_BIT		BIT(6)
#define BOOT_TIMER_EN_BIT	BIT(1)
#define BOOT_DONE_MASK		(BOOT_DONE_BIT | BOOT_TIMER_EN_BIT)
#define CHG_BATFET_ON_BIT	BIT(3)
#define CHG_VCP_EN		BIT(0)
#define CHG_BAT_TEMP_DIS_BIT	BIT(2)
#if defined(CONFIG_MACH_APQ8064_GK_KR) || defined(CONFIG_MACH_APQ8064_GKATT) || defined(CONFIG_MACH_APQ8064_GVDCM) || defined(CONFIG_MACH_APQ8064_GKGLOBAL)
#define SAFE_CURRENT_MA		3000
#else
#define SAFE_CURRENT_MA		1500
#endif
#define PM_SUB_REV             0x001
static int __devinit pm8921_chg_hw_init(struct pm8921_chg_chip *chip)
{
	int rc;
	int vdd_safe;
	u8 subrev;
#ifdef CONFIG_LGE_PM
	int m;
	int mask;
#endif

	/* forcing 19p2mhz before accessing any charger registers */
	pm8921_chg_force_19p2mhz_clk(chip);

	detect_battery_removal(chip);

	rc = pm_chg_masked_write(chip, SYS_CONFIG_2,
					BOOT_DONE_MASK, BOOT_DONE_MASK);
	if (rc) {
		pr_err("Failed to set BOOT_DONE_BIT rc=%d\n", rc);
		return rc;
	}

	vdd_safe = chip->max_voltage_mv + VDD_MAX_INCREASE_MV;

	if (vdd_safe > PM8921_CHG_VDDSAFE_MAX)
		vdd_safe = PM8921_CHG_VDDSAFE_MAX;

	rc = pm_chg_vddsafe_set(chip, vdd_safe);

	if (rc) {
		pr_err("Failed to set safe voltage to %d rc=%d\n",
						chip->max_voltage_mv, rc);
		return rc;
	}

#ifdef CONFIG_LGE_PM
	/* Force enable charger at boot time for High Voltage battery */
	rc = pm_chg_vbatdet_set(chip, chip->max_voltage_mv);
#else
	rc = pm_chg_vbatdet_set(chip,
				chip->max_voltage_mv
				- chip->resume_voltage_delta);
#endif
	if (rc) {
		pr_err("Failed to set vbatdet comprator voltage to %d rc=%d\n",
			chip->max_voltage_mv - chip->resume_voltage_delta, rc);
		return rc;
	}

	rc = pm_chg_vddmax_set(chip, chip->max_voltage_mv);
	if (rc) {
		pr_err("Failed to set max voltage to %d rc=%d\n",
						chip->max_voltage_mv, rc);
		return rc;
	}
	rc = pm_chg_ibatsafe_set(chip, SAFE_CURRENT_MA);
	if (rc) {
		pr_err("Failed to set max voltage to %d rc=%d\n",
						SAFE_CURRENT_MA, rc);
		return rc;
	}
#ifdef CONFIG_LGE_PM
	if(lge_get_factory_boot())
	{
		rc = pm_chg_ibatmax_set(chip, PM8921_CHG_IBATT_FACTORY_CABLE);
		pr_info("factory setting ibatmax=%d mA\n",PM8921_CHG_IBATT_FACTORY_CABLE);
	}
	else
	{
		rc = pm_chg_ibatmax_set(chip, chip->max_bat_chg_current);
	}
#else
	rc = pm_chg_ibatmax_set(chip, chip->max_bat_chg_current);
#endif
	if (rc) {
		pr_err("Failed to set max current to 400 rc=%d\n", rc);
		return rc;
	}

	rc = pm_chg_iterm_set(chip, chip->term_current);
	if (rc) {
		pr_err("Failed to set term current to %d rc=%d\n",
						chip->term_current, rc);
		return rc;
	}

	/* Disable the ENUM TIMER */
	rc = pm_chg_masked_write(chip, PBL_ACCESS2, ENUM_TIMER_STOP_BIT,
			ENUM_TIMER_STOP_BIT);
	if (rc) {
		pr_err("Failed to set enum timer stop rc=%d\n", rc);
		return rc;
	}

#ifdef CONFIG_LGE_PM
	/* Set initial charging current based on USB max power */
	pm_chg_iusbmax_get(chip, &mask);

	for (m = 0; m < ARRAY_SIZE(usb_ma_table); m++) {
		if (usb_ma_table[m].usb_ma == usb_chg_current)
			break;
	}

	if (m >= ARRAY_SIZE(usb_ma_table)) {
		m = 2;	// default 500mA
	}

	/* Set limit current masking when factory boot mode */
	if (lge_get_factory_boot())
	{
		/*LGE_S jungwoo.yun@lge.com 2012-08-06 iusbmax set to 1100mA in 56K/910K cable*/
		if(lge_get_boot_cable_type() == LGE_BOOT_LT_CABLE_56K || lge_get_boot_cable_type() == LGE_BOOT_LT_CABLE_910K)
		{
			m = 0x0A;
#if defined(CONFIG_MACH_APQ8064_GVDCM) || defined(CONFIG_MACH_APQ8064_GKSK) || defined(CONFIG_MACH_APQ8064_GKKT) || defined(CONFIG_MACH_APQ8064_GKU) || defined(CONFIG_MACH_APQ8064_GKATT) || defined(CONFIG_MACH_APQ8064_GVKT) || defined(CONFIG_MACH_APQ8064_GKGLOBAL)
			pm_chg_batfet_set(chip, 1);
#endif
		}
#if defined(CONFIG_MACH_APQ8064_GVDCM) || defined(CONFIG_MACH_APQ8064_GKSK) || defined(CONFIG_MACH_APQ8064_GKKT) || defined(CONFIG_MACH_APQ8064_GKU) || defined(CONFIG_MACH_APQ8064_GKATT) || defined(CONFIG_MACH_APQ8064_GVKT) || defined(CONFIG_MACH_APQ8064_GKGLOBAL)
		else if (lge_get_boot_cable_type() == LGE_BOOT_LT_CABLE_130K)
		{
			m = 0x00;
		}
#endif
        else
		/*LGE_E jungwoo.yun@lge.com 2012-08-06 iusbmax set to 1100mA in 910K cable*/
		{
			m = ARRAY_SIZE(usb_ma_table) - 1;
		}
	}

	pr_debug("sbl=%d mA, kernel=%d mA\n",
		mask, usb_ma_table[m].usb_ma);

	rc = pm_chg_iusbmax_set(chip, m);
	if (rc) {
		pr_err("Failed to set usb max to %d rc=%d\n", 0, rc);
		return rc;
	}
#endif

	if (chip->safety_time != 0) {
		rc = pm_chg_tchg_max_set(chip, chip->safety_time);
		if (rc) {
			pr_err("Failed to set max time to %d minutes rc=%d\n",
							chip->safety_time, rc);
			return rc;
		}
	}

	if (chip->ttrkl_time != 0) {
		rc = pm_chg_ttrkl_max_set(chip, chip->ttrkl_time);
		if (rc) {
			pr_err("Failed to set trkl time to %d minutes rc=%d\n",
							chip->safety_time, rc);
			return rc;
		}
	}

	if (chip->vin_min != 0) {
		rc = pm_chg_vinmin_set(chip, chip->vin_min);
		if (rc) {
			pr_err("Failed to set vin min to %d mV rc=%d\n",
							chip->vin_min, rc);
			return rc;
		}
	} else {
		chip->vin_min = pm_chg_vinmin_get(chip);
	}

	rc = pm_chg_disable_wd(chip);
	if (rc) {
		pr_err("Failed to disable wd rc=%d\n", rc);
		return rc;
	}

#ifdef CONFIG_LGE_PM
	/* Disable Battery temperature Check Function */
	rc = pm_chg_masked_write(chip, CHG_CNTRL_2, CHG_BAT_TEMP_DIS_BIT, CHG_BAT_TEMP_DIS_BIT);
#else
	rc = pm_chg_masked_write(chip, CHG_CNTRL_2,
				CHG_BAT_TEMP_DIS_BIT, 0);
#endif
	if (rc) {
		pr_err("Failed to enable temp control chg rc=%d\n", rc);
		return rc;
	}
	/* switch to a 3.2Mhz for the buck */
	rc = pm8xxx_writeb(chip->dev->parent, CHG_BUCK_CLOCK_CTRL, 0x15);
	if (rc) {
		pr_err("Failed to switch buck clk rc=%d\n", rc);
		return rc;
	}

	if (chip->trkl_voltage != 0) {
		rc = pm_chg_vtrkl_low_set(chip, chip->trkl_voltage);
		if (rc) {
			pr_err("Failed to set trkl voltage to %dmv  rc=%d\n",
							chip->trkl_voltage, rc);
			return rc;
		}
	}

	if (chip->weak_voltage != 0) {
		rc = pm_chg_vweak_set(chip, chip->weak_voltage);
		if (rc) {
			pr_err("Failed to set weak voltage to %dmv  rc=%d\n",
							chip->weak_voltage, rc);
			return rc;
		}
	}

	if (chip->trkl_current != 0) {
		rc = pm_chg_itrkl_set(chip, chip->trkl_current);
		if (rc) {
			pr_err("Failed to set trkl current to %dmA  rc=%d\n",
							chip->trkl_voltage, rc);
			return rc;
		}
	}

	if (chip->weak_current != 0) {
		rc = pm_chg_iweak_set(chip, chip->weak_current);
		if (rc) {
			pr_err("Failed to set weak current to %dmA  rc=%d\n",
							chip->weak_current, rc);
			return rc;
		}
	}

	rc = pm_chg_batt_cold_temp_config(chip, chip->cold_thr);
	if (rc) {
		pr_err("Failed to set cold config %d  rc=%d\n",
						chip->cold_thr, rc);
	}

	rc = pm_chg_batt_hot_temp_config(chip, chip->hot_thr);
	if (rc) {
		pr_err("Failed to set hot config %d  rc=%d\n",
						chip->hot_thr, rc);
	}

	rc = pm_chg_led_src_config(chip, chip->led_src_config);
	if (rc) {
		pr_err("Failed to set charger LED src config %d  rc=%d\n",
						chip->led_src_config, rc);
	}

	/* Workarounds for die 3.0 */
	if (pm8xxx_get_revision(chip->dev->parent) == PM8XXX_REVISION_8921_3p0
	/* CR 403150 for PMIC 3.0.1*/
        && pm8xxx_get_version(chip->dev->parent) == PM8XXX_VERSION_8921) {
                rc = pm8xxx_readb(chip->dev->parent, PM_SUB_REV, &subrev);
                if (rc) {
                        pr_err("read failed: addr=%03X, rc=%d\n",
                                PM_SUB_REV, rc);
                        return rc;
                }
                /* Check if die 3.0.1 is present */
                if (subrev & 0x1)
                        pm8xxx_writeb(chip->dev->parent,
                                CHG_BUCK_CTRL_TEST3, 0xA4);
                else
                        pm8xxx_writeb(chip->dev->parent,
                                CHG_BUCK_CTRL_TEST3, 0xAC);
       }

	/* Enable isub_fine resolution AICL for PM8917 */
	if (pm8xxx_get_version(chip->dev->parent) == PM8XXX_VERSION_8917) {
		chip->iusb_fine_res = true;
		if (chip->uvd_voltage_mv)
			rc = pm_chg_uvd_threshold_set(chip,
					chip->uvd_voltage_mv);
			if (rc) {
				pr_err("Failed to set UVD threshold %drc=%d\n",
						chip->uvd_voltage_mv, rc);
			return rc;
		}
	}

	pm8xxx_writeb(chip->dev->parent, CHG_BUCK_CTRL_TEST3, 0xD9);

	/* Disable EOC FSM processing */
	pm8xxx_writeb(chip->dev->parent, CHG_BUCK_CTRL_TEST3, 0x91);

	rc = pm_chg_masked_write(chip, CHG_CNTRL, VREF_BATT_THERM_FORCE_ON,
						VREF_BATT_THERM_FORCE_ON);
	if (rc)
		pr_err("Failed to Force Vref therm rc=%d\n", rc);
#if defined(CONFIG_MACH_APQ8064_GVDCM) || defined(CONFIG_MACH_APQ8064_GKSK) || defined(CONFIG_MACH_APQ8064_GKKT) || defined(CONFIG_MACH_APQ8064_GKU) || defined(CONFIG_MACH_APQ8064_GKATT) || defined(CONFIG_MACH_APQ8064_GVKT) || defined(CONFIG_MACH_APQ8064_GKGLOBAL)
	if (lge_get_boot_cable_type() == LGE_BOOT_LT_CABLE_130K){
		charging_disabled = 1;
	}
#endif
	rc = pm_chg_charge_dis(chip, charging_disabled);
	if (rc) {
		pr_err("Failed to disable CHG_CHARGE_DIS bit rc=%d\n", rc);
		return rc;
	}

	rc = pm_chg_auto_enable(chip, !charging_disabled);
	if (rc) {
		pr_err("Failed to enable charging rc=%d\n", rc);
		return rc;
	}

#ifdef CONFIG_LGE_WIRELESS_CHARGER
	rc = wireless_hw_init(chip);
	if (rc) {
		printk(KERN_INFO "[WIRELESS] Failed to wireless_hw_init rc=%d\n", rc);
		return rc;
	}
#endif

	return 0;
}

static int get_rt_status(void *data, u64 * val)
{
	int i = (int)data;
	int ret;

	/* global irq number is passed in via data */
	ret = pm_chg_get_rt_status(the_chip, i);
	*val = ret;
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(rt_fops, get_rt_status, NULL, "%llu\n");

static int get_fsm_status(void *data, u64 * val)
{
	u8 temp;

	temp = pm_chg_get_fsm_state(the_chip);
	*val = temp;
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(fsm_fops, get_fsm_status, NULL, "%llu\n");

static int get_reg_loop(void *data, u64 * val)
{
	u8 temp;

	if (!the_chip) {
		pr_err("%s called before init\n", __func__);
		return -EINVAL;
	}
	temp = pm_chg_get_regulation_loop(the_chip);
	*val = temp;
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(reg_loop_fops, get_reg_loop, NULL, "0x%02llx\n");

static int get_reg(void *data, u64 * val)
{
	int addr = (int)data;
	int ret;
	u8 temp;

	ret = pm8xxx_readb(the_chip->dev->parent, addr, &temp);
	if (ret) {
		pr_err("pm8xxx_readb to %x value =%d errored = %d\n",
			addr, temp, ret);
		return -EAGAIN;
	}
	*val = temp;
	return 0;
}

static int set_reg(void *data, u64 val)
{
	int addr = (int)data;
	int ret;
	u8 temp;

	temp = (u8) val;
	ret = pm8xxx_writeb(the_chip->dev->parent, addr, temp);
	if (ret) {
		pr_err("pm8xxx_writeb to %x value =%d errored = %d\n",
			addr, temp, ret);
		return -EAGAIN;
	}
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(reg_fops, get_reg, set_reg, "0x%02llx\n");

enum {
	BAT_WARM_ZONE,
	BAT_COOL_ZONE,
};
static int get_warm_cool(void *data, u64 * val)
{
	if (!the_chip) {
		pr_err("%s called before init\n", __func__);
		return -EINVAL;
	}
	if ((int)data == BAT_WARM_ZONE)
		*val = the_chip->is_bat_warm;
	if ((int)data == BAT_COOL_ZONE)
		*val = the_chip->is_bat_cool;
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(warm_cool_fops, get_warm_cool, NULL, "0x%lld\n");

static void create_debugfs_entries(struct pm8921_chg_chip *chip)
{
	int i;

	chip->dent = debugfs_create_dir("pm8921_chg", NULL);

	if (IS_ERR(chip->dent)) {
		pr_err("pmic charger couldnt create debugfs dir\n");
		return;
	}

	debugfs_create_file("CHG_CNTRL", 0644, chip->dent,
			    (void *)CHG_CNTRL, &reg_fops);
	debugfs_create_file("CHG_CNTRL_2", 0644, chip->dent,
			    (void *)CHG_CNTRL_2, &reg_fops);
	debugfs_create_file("CHG_CNTRL_3", 0644, chip->dent,
			    (void *)CHG_CNTRL_3, &reg_fops);
	debugfs_create_file("PBL_ACCESS1", 0644, chip->dent,
			    (void *)PBL_ACCESS1, &reg_fops);
	debugfs_create_file("PBL_ACCESS2", 0644, chip->dent,
			    (void *)PBL_ACCESS2, &reg_fops);
	debugfs_create_file("SYS_CONFIG_1", 0644, chip->dent,
			    (void *)SYS_CONFIG_1, &reg_fops);
	debugfs_create_file("SYS_CONFIG_2", 0644, chip->dent,
			    (void *)SYS_CONFIG_2, &reg_fops);
	debugfs_create_file("CHG_VDD_MAX", 0644, chip->dent,
			    (void *)CHG_VDD_MAX, &reg_fops);
	debugfs_create_file("CHG_VDD_SAFE", 0644, chip->dent,
			    (void *)CHG_VDD_SAFE, &reg_fops);
	debugfs_create_file("CHG_VBAT_DET", 0644, chip->dent,
			    (void *)CHG_VBAT_DET, &reg_fops);
	debugfs_create_file("CHG_IBAT_MAX", 0644, chip->dent,
			    (void *)CHG_IBAT_MAX, &reg_fops);
	debugfs_create_file("CHG_IBAT_SAFE", 0644, chip->dent,
			    (void *)CHG_IBAT_SAFE, &reg_fops);
	debugfs_create_file("CHG_VIN_MIN", 0644, chip->dent,
			    (void *)CHG_VIN_MIN, &reg_fops);
	debugfs_create_file("CHG_VTRICKLE", 0644, chip->dent,
			    (void *)CHG_VTRICKLE, &reg_fops);
	debugfs_create_file("CHG_ITRICKLE", 0644, chip->dent,
			    (void *)CHG_ITRICKLE, &reg_fops);
	debugfs_create_file("CHG_ITERM", 0644, chip->dent,
			    (void *)CHG_ITERM, &reg_fops);
	debugfs_create_file("CHG_TCHG_MAX", 0644, chip->dent,
			    (void *)CHG_TCHG_MAX, &reg_fops);
	debugfs_create_file("CHG_TWDOG", 0644, chip->dent,
			    (void *)CHG_TWDOG, &reg_fops);
	debugfs_create_file("CHG_TEMP_THRESH", 0644, chip->dent,
			    (void *)CHG_TEMP_THRESH, &reg_fops);
	debugfs_create_file("CHG_COMP_OVR", 0644, chip->dent,
			    (void *)CHG_COMP_OVR, &reg_fops);
	debugfs_create_file("CHG_BUCK_CTRL_TEST1", 0644, chip->dent,
			    (void *)CHG_BUCK_CTRL_TEST1, &reg_fops);
	debugfs_create_file("CHG_BUCK_CTRL_TEST2", 0644, chip->dent,
			    (void *)CHG_BUCK_CTRL_TEST2, &reg_fops);
	debugfs_create_file("CHG_BUCK_CTRL_TEST3", 0644, chip->dent,
			    (void *)CHG_BUCK_CTRL_TEST3, &reg_fops);
	debugfs_create_file("CHG_TEST", 0644, chip->dent,
			    (void *)CHG_TEST, &reg_fops);

	debugfs_create_file("FSM_STATE", 0644, chip->dent, NULL,
			    &fsm_fops);

	debugfs_create_file("REGULATION_LOOP_CONTROL", 0644, chip->dent, NULL,
			    &reg_loop_fops);

	debugfs_create_file("BAT_WARM_ZONE", 0644, chip->dent,
				(void *)BAT_WARM_ZONE, &warm_cool_fops);
	debugfs_create_file("BAT_COOL_ZONE", 0644, chip->dent,
				(void *)BAT_COOL_ZONE, &warm_cool_fops);
#ifdef CONFIG_BATTERY_MAX17047
	/*doosan.baek@lge.com 20121108 Add battery condition */
	debugfs_create_file("BAT_AGE", 0644, chip->dent,
			NULL, &bat_age_fops);
#endif

	for (i = 0; i < ARRAY_SIZE(chg_irq_data); i++) {
		if (chip->pmic_chg_irq[chg_irq_data[i].irq_id])
			debugfs_create_file(chg_irq_data[i].name, 0444,
				chip->dent,
				(void *)chg_irq_data[i].irq_id,
				&rt_fops);
	}
}

/* LGE_CHANGE
* for ATCMD
* 2011-11-04, janghyun.baek@lge.com
*/

#ifdef CONFIG_MACH_LGE
extern void machine_restart(char *cmd);

int pm8921_stop_charging_for_ATCMD(void)
{

	struct pm8921_chg_chip *chip = the_chip;
#if defined(CONFIG_MACH_APQ8064_GVDCM) || defined(CONFIG_MACH_APQ8064_GKSK) || defined(CONFIG_MACH_APQ8064_GKKT) || defined(CONFIG_MACH_APQ8064_GKU) || defined(CONFIG_MACH_APQ8064_GKATT) || defined(CONFIG_MACH_APQ8064_GVKT) || defined(CONFIG_MACH_APQ8064_GKGLOBAL)
    pm_chg_failed_clear(chip, 1);
	pm_chg_iusbmax_set(chip,usb_ma_table[0x0].value);

	charging_disabled = 1;
#endif
	pm8921_chg_disable_irq(chip, ATCFAIL_IRQ);
	pm8921_chg_disable_irq(chip, CHGHOT_IRQ);
	pm8921_chg_disable_irq(chip, ATCDONE_IRQ);
	pm8921_chg_disable_irq(chip, FASTCHG_IRQ);
	pm8921_chg_disable_irq(chip, CHGDONE_IRQ);
	pm8921_chg_disable_irq(chip, VBATDET_IRQ);
	pm8921_chg_disable_irq(chip, VBATDET_LOW_IRQ);

	return 1;
}
int pm8921_start_charging_for_ATCMD(void)
{

	struct pm8921_chg_chip *chip = the_chip;
#if defined(CONFIG_MACH_APQ8064_GVDCM) || defined(CONFIG_MACH_APQ8064_GKSK) || defined(CONFIG_MACH_APQ8064_GKKT) || defined(CONFIG_MACH_APQ8064_GKU) || defined(CONFIG_MACH_APQ8064_GKATT) || defined(CONFIG_MACH_APQ8064_GVKT) || defined(CONFIG_MACH_APQ8064_GKGLOBAL)
	pm_chg_failed_clear(chip, 1);
	pm_chg_iusbmax_set(chip,usb_ma_table[0xA].value);

	charging_disabled = 0;
#endif
	pm8921_chg_enable_irq(chip, ATCFAIL_IRQ);
	pm8921_chg_enable_irq(chip, CHGHOT_IRQ);
	pm8921_chg_enable_irq(chip, ATCDONE_IRQ);
	pm8921_chg_enable_irq(chip, FASTCHG_IRQ);
	pm8921_chg_enable_irq(chip, CHGDONE_IRQ);
	pm8921_chg_enable_irq(chip, VBATDET_IRQ);
	pm8921_chg_enable_irq(chip, VBATDET_LOW_IRQ);

	return 1;
}


static ssize_t at_chg_status_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int fsm_state, is_charging, r;
	bool b_chg_ok = false;

	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}
	fsm_state = pm_chg_get_fsm_state(the_chip);
	is_charging = is_battery_charging(fsm_state);

	if (is_charging) {
		b_chg_ok = true;
		r = sprintf(buf, "%d\n", b_chg_ok);
		printk(KERN_INFO "at_chg_status_show , true ! buf = %s, is_charging = %d\n",
							buf, is_charging);
	} else {
		b_chg_ok = false;
		r = sprintf(buf, "%d\n", b_chg_ok);
		printk(KERN_INFO "at_chg_status_show , false ! buf = %s, is_charging = %d\n",
							buf, is_charging);
	}

	return r;
}

static ssize_t at_chg_status_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int ret = 0, batt_status = 0;
	struct pm8921_chg_chip *chip = the_chip;

#if defined (CONFIG_BATTERY_MAX17047) || defined (CONFIG_BATTERY_MAX17043)
	lge_power_test_flag = 1;
#endif

	if (!count)
		return -EINVAL;

	batt_status = get_prop_batt_status(chip);

	if (strncmp(buf, "0", 1) == 0) {
		/* stop charging */
		printk(KERN_INFO "at_chg_status_store : stop charging start\n");
		if (batt_status == POWER_SUPPLY_STATUS_CHARGING) {
			ret = pm8921_stop_charging_for_ATCMD();
#if defined(CONFIG_MACH_APQ8064_GVDCM) || defined(CONFIG_MACH_APQ8064_GKSK) || defined(CONFIG_MACH_APQ8064_GKKT) || defined(CONFIG_MACH_APQ8064_GKU) || defined(CONFIG_MACH_APQ8064_GKATT) || defined(CONFIG_MACH_APQ8064_GVKT) || defined(CONFIG_MACH_APQ8064_GKGLOBAL)
			pm_chg_auto_enable(chip, !charging_disabled);
			pm_chg_charge_dis(chip,charging_disabled);
#else
            pm_chg_auto_enable(chip, 0);
            pm_chg_charge_dis(chip,1);
#endif
            printk(KERN_INFO "at_chg_status_store : stop charging end\n");
		}
	} else if (strncmp(buf, "1", 1) == 0) {
		/* start charging */
		printk(KERN_INFO "at_chg_status_store : start charging start\n");
		if (batt_status != POWER_SUPPLY_STATUS_CHARGING) {
			ret = pm8921_start_charging_for_ATCMD();
#if defined(CONFIG_MACH_APQ8064_GVDCM) || defined(CONFIG_MACH_APQ8064_GKSK) || defined(CONFIG_MACH_APQ8064_GKKT) || defined(CONFIG_MACH_APQ8064_GKU) || defined(CONFIG_MACH_APQ8064_GKATT) || defined(CONFIG_MACH_APQ8064_GVKT) || defined(CONFIG_MACH_APQ8064_GKGLOBAL)
			pm_chg_auto_enable(chip, !charging_disabled);
			pm_chg_charge_dis(chip,charging_disabled);
#else
            pm_chg_auto_enable(chip, 1);
            pm_chg_charge_dis(chip,0);
#endif
            printk(KERN_INFO "at_chg_status_store : start charging end\n");
		}
	}

	if(ret == 0)
		return -EINVAL;

	return ret;
}

static ssize_t at_chg_complete_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int guage_level = 0, r = 0;
        /*[start] jungwoo.yun@leg.com*/
	//guage_level = 90; //temp
#ifdef CONFIG_BATTERY_MAX17047
#if defined(CONFIG_MACH_APQ8064_GVDCM)
	if(lge_get_board_revno() > HW_REV_A) 
		guage_level = max17047_get_soc_for_charging_complete_at_cmd();
	else
		guage_level = get_prop_batt_capacity(the_chip);
#else
	guage_level = max17047_get_soc_for_charging_complete_at_cmd();
#endif
#elif defined (CONFIG_BATTERY_MAX17043)
	guage_level = max17043_get_capacity(the_chip);
#else
	guage_level = get_prop_batt_capacity(the_chip);
#endif

        /*[end] jungwoo.yun@leg.com*/
	if (guage_level == 100) {
		r = sprintf(buf, "%d\n", 0);
	} else {
		r = sprintf(buf, "%d\n", 1);
	}
	return r;

}

static ssize_t at_chg_complete_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int ret = 0, batt_status = 0;
	struct pm8921_chg_chip *chip = the_chip;

	if (!count)
		return -EINVAL;

	batt_status = get_prop_batt_status(chip);

	if (strncmp(buf, "0", 1) == 0) {
		/* charging not complete */
		printk(KERN_INFO "at_chg_complete_store : charging not complete start\n");
		if (batt_status != POWER_SUPPLY_STATUS_CHARGING) {
			pm8921_chg_enable_irq(chip, FASTCHG_IRQ);
			pm_chg_auto_enable(chip, 1);
			printk(KERN_INFO "at_chg_complete_store : charging not complete end\n");
		}
	} else if (strncmp(buf, "1", 1) == 0) {
		/* charging complete */
		printk(KERN_INFO "at_chg_complete_store : charging complete start\n");
		if (batt_status != POWER_SUPPLY_STATUS_FULL) {
			ret = pm8921_stop_charging_for_ATCMD();
			pm_chg_auto_enable(chip, 0);
			printk(KERN_INFO "at_chg_complete_store : charging complete end\n");
		}
	}

	return ret;
}

static ssize_t at_pmic_reset_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int r = 0;
	bool pm_reset = true;

	msleep(3000); /* for waiting return values of testmode */

	machine_restart(NULL);

	r = sprintf(buf, "%d\n", pm_reset);

	return r;
}

/* test code by sungsookim */
static ssize_t at_current_limit_show(struct device *dev,
					struct device_attribute *attr, char *buf)
{
	int r = 0;
	bool current_limit = true;

	printk(KERN_INFO "Current limit is successfully called\n");

	__pm8921_charger_vbus_draw(1500);
	r = sprintf(buf, "%d\n", current_limit);

	return r;
}

static ssize_t at_vcoin_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int r;
	struct pm8xxx_adc_chan_result result = { 0 };

	r = pm8xxx_adc_read(CHANNEL_VCOIN, &result);

	r = sprintf(buf, "%lld\n", result.physical);

	return r;
}

DEVICE_ATTR(at_current, 0644, at_current_limit_show, NULL);
DEVICE_ATTR(at_charge, 0664, at_chg_status_show, at_chg_status_store);
DEVICE_ATTR(at_chcomp, 0664, at_chg_complete_show, at_chg_complete_store);
DEVICE_ATTR(at_pmrst, 0644, at_pmic_reset_show, NULL);
DEVICE_ATTR(at_vcoin, 0644, at_vcoin_show, NULL);
#endif

#ifdef CONFIG_LGE_CHARGER_TEMP_SCENARIO
static ssize_t pm8921_batt_therm_adc_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int rc;
	struct pm8xxx_adc_chan_result result;

	rc = pm8xxx_adc_read(CHANNEL_BATT_THERM, &result);
	if (rc) {
		pr_err("error reading adc channel = %d, rc = %d\n",
					CHANNEL_BATT_THERM, rc);
		return rc;
	}
	pr_debug("batt_temp phy = %lld  meas = 0x%llx adc_code = %d\n",
				result.physical, result.measurement, result.adc_code);
	rc = sprintf(buf, "%d\n", result.adc_code);
	return rc;
}
DEVICE_ATTR(batt_temp_adc, 0644, pm8921_batt_therm_adc_show, NULL);
#endif

#if defined(CONFIG_MACH_APQ8064_GK_KR) || defined(CONFIG_MACH_APQ8064_GV_KR)
#include <linux/mfd/pm8xxx/misc.h>
extern int pmic_reset_irq;
int hard_reset_disable = 0;
static ssize_t hardreset_status_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int  r;

	if (hard_reset_disable) {
		r = sprintf(buf, "%d\n", 1);
	} else {
		r = sprintf(buf, "%d\n", 0);
	}
	printk("%s hard_reset_disable=%d\n", __func__, hard_reset_disable);

	return r;
}

static ssize_t hardreset_status_store(struct device *dev,struct device_attribute *attr,
				const char *buf, size_t count)
{
	int r = 1;

	if (!count)
		return -EINVAL;

	printk("%s  reset_irq(%d) param(%s) curr_status(%d)\n", __func__,
		pmic_reset_irq, buf, hard_reset_disable);

	if (strncmp(buf, "1", 1) == 0) {
		/* disable hard-reset */
		if (pmic_reset_irq != 0 && !hard_reset_disable) {
			hard_reset_disable = 1;
			disable_irq_nosync(pmic_reset_irq);
			pm8xxx_hard_reset_config(PM8XXX_DISABLE_HARD_RESET);
			printk("PM8XXX HARD RESET DISABLE!\n");
		}
	} else if (strncmp(buf, "0", 1) == 0) {
		/* enable hard-reset */
		if (pmic_reset_irq != 0 && hard_reset_disable) {
			hard_reset_disable = 0;
			enable_irq(pmic_reset_irq);
			pm8xxx_hard_reset_config(PM8XXX_RESTART_ON_HARD_RESET);
			printk("PM8XXX HARD RESET ENABLE!\n");
		}
	}

	return r;
}
DEVICE_ATTR(hardreset_dis, 0644, hardreset_status_show, hardreset_status_store);
#endif

static int pm8921_charger_suspend_noirq(struct device *dev)
{
	int rc;
	struct pm8921_chg_chip *chip = dev_get_drvdata(dev);

	rc = pm_chg_masked_write(chip, CHG_CNTRL, VREF_BATT_THERM_FORCE_ON, 0);
	if (rc)
		pr_err("Failed to Force Vref therm off rc=%d\n", rc);
        pm8921_chg_set_hw_clk_switching(chip);
	return 0;
}

static int pm8921_charger_resume_noirq(struct device *dev)
{
	int rc;
	struct pm8921_chg_chip *chip = dev_get_drvdata(dev);

	pm8921_chg_force_19p2mhz_clk(chip);

	rc = pm_chg_masked_write(chip, CHG_CNTRL, VREF_BATT_THERM_FORCE_ON,
						VREF_BATT_THERM_FORCE_ON);
	if (rc)
		pr_err("Failed to Force Vref therm on rc=%d\n", rc);
	return 0;
}


#ifdef CONFIG_LGE_WIRELESS_CHARGER
static int wireless_inform_enable = 0;
module_param(wireless_inform_enable, int, 0644);


static int wireless_ibat_max_param = WIRELESS_IBAT_MAX;
static int set_wireless_ibat_max_param(const char *val, struct kernel_param *kp)
{
	int ret;
	ret = param_set_int(val, kp);
	if (ret) {
		pr_info("error setting value %d\n", ret);
		return ret;
	}

	printk(KERN_INFO "[WIRELESS] your input : wireless_ibat_max=%d\n",wireless_ibat_max_param);
	if(!the_chip) {
		printk(KERN_INFO "[WIRELESS] Failed to IBAT_MAX\n");
	}
	else {
		pm_chg_ibatmax_set(the_chip,wireless_ibat_max_param);
	}
	return 0;
}
module_param_call(wireless_ibat_max_param,set_wireless_ibat_max_param, param_get_uint,&wireless_ibat_max_param, 0644);


static int wireless_vin_min_param = WIRELESS_VIN_MIN;
static int set_wireless_vin_min_param(const char *val, struct kernel_param *kp)
{
	int ret;
	ret = param_set_int(val, kp);
	if (ret) {
		pr_info("error setting value %d\n", ret);
		return ret;
	}

	printk(KERN_INFO "[WIRELESS] your input : wireless_vin_min_param=%d\n",wireless_vin_min_param);
	if(!the_chip) {
		printk(KERN_INFO "[WIRELESS] Failed to VIN_MIN\n");
	}
	else {
		pm_chg_vinmin_set(the_chip,wireless_vin_min_param);
	}
	return 0;
}
module_param_call(wireless_vin_min_param,set_wireless_vin_min_param, param_get_uint,&wireless_vin_min_param, 0644);


static int wireless_rx_off_param = 0;
static int wireless_rx_off_param_call(const char *val, struct kernel_param *kp)
{
	int ret;
	ret = param_set_int(val, kp);
	if (ret) {
		pr_info("error setting value %d\n", ret);
		return ret;
	}

	printk(KERN_INFO "[WIRELESS] your input : wireless_rx_off_param=%d\n",wireless_rx_off_param);
	if(!the_chip) {
		printk(KERN_INFO "[WIRELESS] Failed to wireless_rx_off_param\n");
	}
	else {
		wireless_chip_control_worker(&the_chip->wireless_chip_control_work.work);
	}
	return 0;
}
module_param_call(wireless_rx_off_param,wireless_rx_off_param_call, param_get_uint,&wireless_rx_off_param, 0644);


static int wireless_dis_param = 0;
static int wireless_dis_param_call(const char *val, struct kernel_param *kp)
{
	int ret;
	ret = param_set_int(val, kp);
	if (ret) {
		pr_info("error setting value %d\n", ret);
		return ret;
	}

	printk(KERN_INFO "[WIRELESS] your input : wireless_dis_param=%d\n",wireless_dis_param);
	if(!the_chip) {
		printk(KERN_INFO "[WIRELESS] Failed to wireless_dis_param\n");
		return 0;
	}
	else {
		pm_chg_charge_dis(the_chip,wireless_dis_param);
	}
	return 0;
}
module_param_call(wireless_dis_param,wireless_dis_param_call, param_get_uint,&wireless_dis_param, 0644);


static int wireless_auto_param = 0;
static int wireless_auto_param_call(const char *val, struct kernel_param *kp)
{
	int ret;
	ret = param_set_int(val, kp);
	if (ret) {
		pr_info("error setting value %d\n", ret);
		return ret;
	}

	printk(KERN_INFO "[WIRELESS] your input : wireless_auto_param=%d\n",wireless_auto_param);
	if(!the_chip) {
		printk(KERN_INFO "[WIRELESS] Failed to wireless_dis_param\n");
		return 0;
	}
	else {
		pm_chg_auto_enable(the_chip,wireless_auto_param);
	}
	return 0;
}
module_param_call(wireless_auto_param,wireless_auto_param_call, param_get_uint,&wireless_auto_param, 0644);


static bool pm_is_chg_charge_dis_bit_set(struct pm8921_chg_chip *chip)
{
	u8 temp = 0;

	pm8xxx_readb(chip->dev->parent, CHG_CNTRL, &temp);
	return !!(temp & CHG_CHARGE_DIS_BIT);
}


static bool wireless_get_backlight_on(void)
{
#if 1//def CONFIG_LGE_FTT_CHARGER
	return false;
#else
	if(wireless_backlight_state()) {
		printk(KERN_INFO "[WIRELESS] ON-BackLight\n");
		return true;
	}
	else {
		printk(KERN_INFO "[WIRELESS] OFF-BackLight\n");
		return false;
	}
#endif
}

static bool wireless_is_plugged(void)
{
	int ret;
	ret = !gpio_get_value_cansleep(ACTIVE_N);
	return ret;
}


static int wireless_pm_power_get_property(struct power_supply *psy)
{
	#if 0
	int ret;

	if(wireless_charging) {
		if(the_chip)
			ret = is_dc_chg_plugged_in(the_chip);
		else
			ret = wireless_is_plugged();
	}
	return ret;
	#endif
	if(!is_dc_chg_plugged_in(the_chip) || !wireless_is_plugged()) {
		return 0;
	}
	else {
		return wireless_charging;
	}
}


static int wireless_batt_status(void)
{
	if (wireless_charge_done)
		return POWER_SUPPLY_STATUS_FULL;
	if (wireless_charging) {
#ifdef CONFIG_LGE_CHARGER_TEMP_SCENARIO
#if defined(CONFIG_MACH_MSM8960_D1LU) || defined(CONFIG_MACH_MSM8960_D1LSK) || defined(CONFIG_MACH_MSM8960_D1LKT)
		if((chg_batt_temp_state == CHG_BATT_STOP_CHARGING_STATE) && wlc_charging_status)// && !pseudo_ui_charging)
			return POWER_SUPPLY_STATUS_CHARGING;
		else if(chg_batt_temp_state == CHG_BATT_STOP_CHARGING_STATE)
			return POWER_SUPPLY_STATUS_NOT_CHARGING;
#else
		if(chg_batt_temp_state == CHG_BATT_STOP_CHARGING_STATE)// && !pseudo_ui_charging)
			return POWER_SUPPLY_STATUS_NOT_CHARGING;
#endif
		else
#endif
			return POWER_SUPPLY_STATUS_CHARGING;
	}
	return  POWER_SUPPLY_STATUS_DISCHARGING;
}


static int wireless_soc(struct pm8921_chg_chip *chip)
{
	int soc;
#ifdef CONFIG_BATTERY_MAX17047
	soc = max17047_get_soc();
#elif defined (CONFIG_BATTERY_MAX17043)
	soc = max17043_get_capacity(chip);
#else
	soc = get_prop_batt_capacity(chip);
#endif
	return soc;
}


static bool wireless_soc_check(struct pm8921_chg_chip *chip)
{
	int soc;
	soc = wireless_soc(chip);
#if defined (CONFIG_BATTERY_MAX17047) || defined (CONFIG_BATTERY_MAX17043)
	if(soc > WIRELESS_MAXIM_SOC)
#else
	if(soc > WIRELESS_BMS_SOC)
#endif
		return false;
	else
		return true;
}


static void turn_on_dc_ovp_fet(struct pm8921_chg_chip *chip)
{
	u8 temp;
	int rc;

	rc = pm8xxx_writeb(chip->dev->parent, DC_OVP_TEST, 0x30);
	if (rc) {
		printk(KERN_INFO "[WIRELESS] Failed to write 0x30 to DC_OVP_TEST rc = %d\n", rc);
		return;
	}
	rc = pm8xxx_readb(chip->dev->parent, DC_OVP_TEST, &temp);
	if (rc) {
		printk(KERN_INFO "[WIRELESS] Failed to read from DC_OVP_TEST rc = %d\n", rc);
		return;
	}
	/* unset ovp fet disable bit and set the write bit */
	temp &= 0xFE;
	temp |= 0x80;
	rc = pm8xxx_writeb(chip->dev->parent, DC_OVP_TEST, temp);
	if (rc) {
		printk(KERN_INFO "[WIRELESS] Failed to write 0x%x to DC_OVP_TEST rc = %d\n",temp, rc);
		return;
	}
}


static void turn_off_dc_ovp_fet(struct pm8921_chg_chip *chip)
{
	u8 temp;
	int rc;

	rc = pm8xxx_writeb(chip->dev->parent, DC_OVP_TEST, 0x30);
	if (rc) {
		printk(KERN_INFO "[WIRELESS] Failed to write 0x30 to DC_OVP_TEST rc = %d\n", rc);
		return;
	}
	rc = pm8xxx_readb(chip->dev->parent, DC_OVP_TEST, &temp);
	if (rc) {
		printk(KERN_INFO "[WIRELESS] Failed to read from DC_OVP_TEST rc = %d\n", rc);
		return;
	}
	/* set ovp fet disable bit and the write bit */
	temp |= 0x81;
	rc = pm8xxx_writeb(chip->dev->parent, DC_OVP_TEST, temp);
	if (rc) {
		printk(KERN_INFO "[WIRELESS] Failed to write 0x%x DC_OVP_TEST rc=%d\n", temp, rc);
		return;
	}
}


#define DC_OVP_DEBOUNCE_TIME 0x06
static void wireless_unplug_ovp_fet_open(struct pm8921_chg_chip *chip)
{
	int chg,dc,count = 0;

	while (count++ < param_open_ovp_counter) {
		pm_chg_masked_write(chip,DC_OVP_CONTROL,DC_OVP_DEBOUNCE_TIME,0x0);
		usleep(10);
		dc = is_dc_chg_plugged_in(chip);
		chg = wireless_is_plugged();
		printk(KERN_INFO "[WIRELESS] OVP FET count=%d , DC_Physical=%d , DC_IRQ=%d\n",count,chg,dc);
		if (!chg && dc) {
			turn_off_dc_ovp_fet(chip);
			msleep(20);
			turn_on_dc_ovp_fet(chip);
		}
		else {
			pm_chg_masked_write(chip,DC_OVP_CONTROL,DC_OVP_DEBOUNCE_TIME,0x1);
			printk(KERN_INFO "[WIRELESS] Exit count=%d , DC_Physical=%d , DC_IRQ=%d\n",count,chg,dc);
			return;
		}
	}
}


static void wireless_current_set(struct pm8921_chg_chip *chip)
{
#ifdef CONFIG_LGE_CHARGER_TEMP_SCENARIO
	if(chg_batt_temp_state == CHG_BATT_DC_CURRENT_STATE /*|| pre_mitigation*/) {
		pm_chg_ibatmax_set(chip,WIRELESS_DECREASE_IBAT);
		printk(KERN_INFO "[WIRELESS] SET : Decrease IBAT %d\n",WIRELESS_DECREASE_IBAT);
		return;
	}
#endif

	if(wireless_get_backlight_on()) {
		pm_chg_ibatmax_set(chip,WIRELESS_DECREASE_IBAT);
		printk(KERN_INFO "[WIRELESS] SET : Backlight ON, Decrease IBAT %d\n",WIRELESS_DECREASE_IBAT);
		return;
	}

	if(wireless_ibat_max_param != WIRELESS_IBAT_MAX) {
		pm_chg_ibatmax_set(chip,wireless_ibat_max_param);
		printk(KERN_INFO "[WIRELESS] SET : Param IBAT %d\n",wireless_ibat_max_param);
		return;
	}
	else {
		pm_chg_ibatmax_set(chip,WIRELESS_IBAT_MAX);
		printk(KERN_INFO "[WIRELESS] SET : Default IBAT %d\n",WIRELESS_IBAT_MAX);
		return;
	}
}


static void wireless_vinmin_set(struct pm8921_chg_chip *chip)
{
	if(wireless_vin_min_param != WIRELESS_VIN_MIN) {
		pm_chg_vinmin_set(chip,wireless_vin_min_param);
		printk(KERN_INFO "[WIRELESS] SET : Param VINMIN %d\n",wireless_vin_min_param);
	}
	else {
		pm_chg_vinmin_set(chip,WIRELESS_VIN_MIN);
		printk(KERN_INFO "[WIRELESS] SET : Default VINMIN %d\n",WIRELESS_VIN_MIN);
	}
}


/*static int wireless_batfet_on(struct pm8921_chg_chip *chip, int enable)
{
	return pm_chg_masked_write(chip, CHG_CNTRL,WIRELESS_BATFET_BIT,enable ? WIRELESS_BATFET_BIT : 0);
}*/


static void wireless_set(struct pm8921_chg_chip *chip)
{
	printk(KERN_INFO "[WIRELESS] SET START\n");
	//pm_chg_charge_dis(chip,1);
	pm_chg_failed_clear(chip, 1);

#ifdef WIRELESS_CAYMAN
	printk(KERN_INFO "[WIRELESS] CAYMAN scenario enabled!!\n");
	if(!wireless_soc_check(chip)) {
		printk(KERN_INFO "[WIRELESS] SOC check false\n");
		wireless_chip_control_worker(&chip->wireless_chip_control_work.work);
		return;
	}
#endif

#ifdef CONFIG_LGE_CHARGER_TEMP_SCENARIO
	last_stop_charging = 0;
	pseudo_ui_charging = 0;
	pre_mitigation = 0;
	if(delayed_work_pending(&chip->monitor_batt_temp_work)) {
		printk(KERN_INFO "[WIRELESS] SET : Go monitor batt temp\n");
		cancel_delayed_work_sync(&chip->monitor_batt_temp_work);
		schedule_delayed_work(&chip->monitor_batt_temp_work,round_jiffies_relative(msecs_to_jiffies(5000)));
	}
#endif

	//pm_chg_vddmax_set(chip,WIRELESS_VDD_MAX);
	pm_chg_vbatdet_set(chip,WIRELESS_VDET_LOW);
	wireless_vinmin_set(chip);
	wireless_current_set(chip);
	pm_chg_iterm_set(chip,WIRELESS_ITERM);
	//printk(KERN_INFO "[WIRELESS] SET : ITERM %d\n",WIRELESS_ITERM);
	//printk(KERN_INFO "[WIRELESS] SET : VBATDET %d\n",WIRELESS_VDET_LOW);
	//printk(KERN_INFO "[WIRELESS] SET : VDDMAX  %d\n",WIRELESS_VDD_MAX);
	pm_chg_auto_enable(chip,1);
	pm_chg_charge_dis(chip,0);

	chip->dc_present = 1;
	wireless_charging = true;
	wireless_charge_done = false;
	wireless_source_control_count = 0;
	bms_notify_check(chip);
	power_supply_changed(&chip->batt_psy);
	power_supply_changed(&chip->wireless_psy);
	printk(KERN_INFO "[WIRELESS] SET END\n");
	return;
}


static void wireless_reset(struct pm8921_chg_chip *chip)
{
	if(chip->dc_present) {
		printk(KERN_INFO "[WIRELESS] RESET START\n");
	        pm_chg_failed_clear(chip, 1);
		//pm_chg_vddmax_set(chip,chip->max_voltage_mv);
		pm_chg_vbatdet_set(chip,chip->max_voltage_mv-chip->resume_voltage_delta);
		pm_chg_vinmin_set(chip,chip->vin_min);
		pm_chg_ibatmax_set(chip,chip->max_bat_chg_current);
		pm_chg_iterm_set(chip,chip->term_current);
		//wireless_batfet_on(chip,0);
		pm_chg_charge_dis(chip,0);
		pm_chg_auto_enable(chip,1);
		wireless_charging = false;
		wireless_charge_done = false;
		wireless_source_control_count = 0;
#ifdef CONFIG_LGE_CHARGER_TEMP_SCENARIO
		last_stop_charging = 0;
		pseudo_ui_charging = 0;
		pre_mitigation = 0;
#endif
		chip->dc_present = 0;
		bms_notify_check(chip);
		power_supply_changed(&chip->batt_psy);
		power_supply_changed(&chip->wireless_psy);
		printk(KERN_INFO "[WIRELESS] RESET END\n");
	}
}


static void wireless_reset_hw(int usb , struct pm8921_chg_chip *chip)
{
	if(usb) {
		//pm_chg_vddmax_set(chip,chip->max_voltage_mv);
		pm_chg_vbatdet_set(chip,chip->max_voltage_mv-chip->resume_voltage_delta);
		pm_chg_vinmin_set(chip,chip->vin_min);
		pm_chg_ibatmax_set(chip,chip->max_bat_chg_current);
		pm_chg_iterm_set(chip,chip->term_current);
		//wireless_batfet_on(chip,0);
		pm_chg_charge_dis(chip,0);
		pm_chg_auto_enable(chip,1);
		printk(KERN_INFO "[WIRELESS] USB Insert - VINMIN: %dmV , IBATMAX: %dmA , VDDMAX: %dmV , VBATDET: %dmV\n",
		chip->vin_min,chip->max_bat_chg_current,chip->max_voltage_mv,chip->max_voltage_mv-chip->resume_voltage_delta);
	}
}


static void wireless_unplug_check_worker(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct pm8921_chg_chip *chip = container_of(dwork,struct pm8921_chg_chip,wireless_unplug_check_work);

	if (!is_dc_chg_plugged_in(chip)) {
		printk(KERN_INFO "[WIRELESS] DC is removed,Stopping. reg_loop=%d , fsm=%d , ibat=%d\n",pm_chg_get_regulation_loop(chip),pm_chg_get_fsm_state(chip),get_prop_batt_current(chip));
		wake_unlock(&chip->wireless_unplug_wake_lock);
		return;
	}

	if(wireless_is_plugged()) {
		printk(KERN_INFO "[WIRELESS] DC_Physical=1 , Unlock wireless_unplug_check_worker\n");
		wireless_set(chip);
		wake_unlock(&chip->wireless_unplug_wake_lock);
		schedule_delayed_work(&chip->wireless_polling_dc_work,round_jiffies_relative(msecs_to_jiffies(WIRELESS_POLLING_DC_TIME)));
		return;
	}

	if (!wireless_is_plugged() && is_dc_chg_plugged_in(chip)) {
		wireless_unplug_ovp_fet_open(chip);
		schedule_delayed_work(&chip->wireless_unplug_check_work,round_jiffies_relative(msecs_to_jiffies(WIRELESS_UNPLUG_CHECK_TIME)));
	}
}


static void wireless_polling_dc_worker(struct work_struct *work)
{
	int dc,dc_gpio,ibat;
	struct delayed_work *dwork = to_delayed_work(work);
	struct pm8921_chg_chip *chip = container_of(dwork,struct pm8921_chg_chip,wireless_polling_dc_work);

	//printk(KERN_INFO "[WIRELESS] Polling\n");

	dc = is_dc_chg_plugged_in(chip);
	if(!dc) {
		printk(KERN_INFO "[WIRELESS] Polling: DC removed\n");
		return;
	}

	dc_gpio = wireless_is_plugged();
	ibat = get_prop_batt_current(chip);
	if((ibat>0) && !dc_gpio && !wake_lock_active(&chip->wireless_unplug_wake_lock)) {
		wake_lock(&chip->wireless_unplug_wake_lock);
		schedule_delayed_work(&chip->wireless_unplug_check_work,round_jiffies_relative(msecs_to_jiffies(WIRELESS_INT_TO_UNPLUG_CHECK)));
		printk(KERN_INFO "[WIRELESS] Polling: Go wireless_unplug_check_worker\n");
		disable_irq(gpio_to_irq(ACTIVE_N));
		enable_irq(gpio_to_irq(ACTIVE_N));
		return;
	}

	schedule_delayed_work(&chip->wireless_polling_dc_work,round_jiffies_relative(msecs_to_jiffies(WIRELESS_POLLING_DC_TIME)));
}


static void wireless_interrupt_worker(struct work_struct *work)
{
	struct pm8921_chg_chip *chip = container_of(work,struct pm8921_chg_chip,wireless_interrupt_work);

	printk(KERN_INFO "[WIRELESS] INTWORKER START\n");
	if(wireless_is_plugged())
		wireless_set(chip);
	else
		wireless_reset(chip);

	if(!wireless_is_plugged() && is_dc_chg_plugged_in(chip) && !delayed_work_pending(&chip->wireless_unplug_check_work)) {
		printk(KERN_INFO "[WIRELESS] INT: Do Schedule unplug check\n");
		wake_lock(&chip->wireless_unplug_wake_lock);
		schedule_delayed_work(&chip->wireless_unplug_check_work,round_jiffies_relative(msecs_to_jiffies(WIRELESS_INT_TO_UNPLUG_CHECK)));
	}
	if(wireless_is_plugged() && delayed_work_pending(&chip->wireless_unplug_check_work)) {
		if(wake_lock_active(&chip->wireless_unplug_wake_lock))
			wake_unlock(&chip->wireless_unplug_wake_lock);
		cancel_delayed_work_sync(&chip->wireless_unplug_check_work);
		printk(KERN_INFO "[WIRELESS] INT: Cancel schedule unplug check\n");
	}
	printk(KERN_INFO "[WIRELESS] INTWORKER END\n");
}


static irqreturn_t  wireless_interrupt_handler(int irq, void *data)
{
	int chg;
	struct pm8921_chg_chip *chip = data;

	printk(KERN_INFO "[WIRELESS] ACTIVE INT START===========================\n");
	chg = wireless_is_plugged();
	printk(KERN_INFO "[WIRELESS] INT DC_Physical = %d\n",chg);
	schedule_work(&chip->wireless_interrupt_work);
	printk(KERN_INFO "[WIRELESS] ACTIVE INT END=============================\n");
	return IRQ_HANDLED;
}


static void wireless_interrupt_probe(struct pm8921_chg_chip *chip)
{
	int ret;
	hw_rev_type lge_bd_rev = HW_REV_EVB1;

	pm8921_chg_enable_irq(chip, BATFET_IRQ);
	pm8921_chg_enable_irq(chip, CHGSTATE_IRQ);

	ret = gpio_request(ACTIVE_N,"pm8921_wireless_int");
	if (ret < 0) {
		printk(KERN_INFO "[WIRELESS] gpio_request failed\n");
		return;
	}
	gpio_direction_input(ACTIVE_N);
	ret = request_irq(gpio_to_irq(ACTIVE_N),wireless_interrupt_handler,IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING,"WLC_ACTIVE_N",chip);
	if (ret < 0) {
		printk(KERN_INFO "[WIRELESS] request_irq failed\n");
		return;
	}
	enable_irq_wake(gpio_to_irq(ACTIVE_N));

	lge_bd_rev = lge_get_board_revno();
	if(lge_bd_rev != HW_REV_A && lge_bd_rev != HW_REV_B && lge_bd_rev != HW_REV_F)
		chip->wlc_ts_ctrl = PM8921_GPIO_PM_TO_SYS(WLC_TS_CTRL_BCM);
	else
		chip->wlc_ts_ctrl = PM8921_GPIO_PM_TO_SYS(WLC_TS_CTRL_WCN);
	ret = gpio_request(chip->wlc_ts_ctrl,"wlc_ts_ctrl");
	if (ret < 0) {
		printk(KERN_INFO "[WIRELESS] wlc_ts_ctrl gpio_request failed\n");
		return;
	}

	return;
}


static void dcin_valid_irq_worker(struct work_struct *work)
{
	struct pm8921_chg_chip *chip = container_of(work,struct pm8921_chg_chip,dcin_valid_irq_work);

	/*if(!wireless_interrupt_enable) {
		printk(KERN_INFO "[WIRELESS] Enable Active_N IRQ\n");
		enable_irq(gpio_to_irq(ACTIVE_N));
		disable_irq(gpio_to_irq(ACTIVE_N));
		enable_irq(gpio_to_irq(ACTIVE_N));
		wireless_interrupt_enable = true;
	}*/

	if(is_dc_chg_plugged_in(chip)&&!is_usb_chg_plugged_in(chip)) {
		if(delayed_work_pending(&chip->eoc_work)) {
			if(wake_lock_active(&chip->eoc_wake_lock))
				wake_unlock(&chip->eoc_wake_lock);
			cancel_delayed_work_sync(&chip->eoc_work);
			printk(KERN_INFO "[WIRELESS] DC-Cancel previous eoc_work\n");
		}
		if(!wake_lock_active(&chip->eoc_wake_lock))
			wake_lock(&chip->eoc_wake_lock);
		schedule_delayed_work(&chip->eoc_work,round_jiffies_relative(msecs_to_jiffies(WIRELESS_EARLY_EOC_WORK_TIME)));
		printk(KERN_INFO "[WIRELESS] Go eoc_worker\n");
	}

	if(delayed_work_pending(&chip->wireless_unplug_check_work)) {
		if(wake_lock_active(&chip->wireless_unplug_wake_lock))
			wake_unlock(&chip->wireless_unplug_wake_lock);
		cancel_delayed_work_sync(&chip->wireless_unplug_check_work);
		printk(KERN_INFO "[WIRELESS] Cancel wireless_unplug_work\n");
	}
	else {
		printk(KERN_INFO "[WIRELESS] Nothing to cancel wireless_unplug_work\n");
	}

	if(is_dc_chg_plugged_in(chip)) {
		printk(KERN_INFO "[WIRELESS] dc worker - SET\n");
		wireless_set(chip);
	}
	else{
		printk(KERN_INFO "[WIRELESS] dc worker - RESET\n");
		wireless_reset(chip);
	}
}


static irqreturn_t dcin_valid_irq_handler(int irq, void *data)
{
	struct pm8921_chg_chip *chip = data;
	int dc_present;
	/*struct timeval time;
	long gap;*/
	int dc_gpio;

	printk(KERN_INFO "[WIRELESS] DC IRQ START\n");
	dc_present = pm_chg_get_rt_status(chip, DCIN_VALID_IRQ);
	if(dc_present) {
		printk(KERN_INFO "[WIRELESS] Schedule DC Polling\n");
		schedule_delayed_work(&chip->wireless_polling_dc_work,round_jiffies_relative(msecs_to_jiffies(WIRELESS_POLLING_DC_TIME)));
	}
	/*if(dc_present && !wireless_dc_time) {
		printk(KERN_INFO "[WIRELESS] wireless_interrupt_enable\n");
		wireless_interrupt_handler(gpio_to_irq(ACTIVE_N),data);
		do_gettimeofday(&time);
		wireless_dc_time = time.tv_sec;
	}*/
	dc_gpio = wireless_is_plugged();
	printk(KERN_INFO "[WIRELESS] SAVED_DC=%d , NOW_DC=%d , GPIO=%d\n",chip->dc_present,dc_present,dc_gpio);
	schedule_work(&chip->dcin_valid_irq_work);
	/*gap = wireless_dc_time-wireless_on_time;
	if(gap > 0 && gap < 6)
		printk(KERN_INFO "[WIRELESS] ON ~ DC Abnormal triger !!!\n");*/
	printk(KERN_INFO "[WIRELESS] DC IRQ END\n");
	return IRQ_HANDLED;
}


static void wireless_source_control_worker(struct work_struct *work)
{
	struct delayed_work	*dwork = to_delayed_work(work);
	struct pm8921_chg_chip	*chip = container_of(dwork,struct pm8921_chg_chip,wireless_source_control_work);

	if(is_dc_chg_plugged_in(chip) && !is_usb_chg_plugged_in(chip)) {
		if(!wireless_source_toggle) {
			wake_lock(&chip->wireless_source_wake_lock);
			pm_chg_charge_dis(chip,1);
			//pm_chg_vddmax_set(chip,chip->max_voltage_mv);
			pm_chg_vbatdet_set(chip,chip->max_voltage_mv-chip->resume_voltage_delta);
			schedule_delayed_work(&chip->wireless_source_control_work,round_jiffies_relative(msecs_to_jiffies(WIRELESS_EARLY_SOURCE_TIME)));
			wireless_source_toggle = true;
			printk(KERN_INFO "[WIRELESS] Source Disabled. FSM=%d,SOC=%d\n",pm_chg_get_fsm_state(chip),wireless_soc(chip));

		}
		else {
			//pm_chg_vddmax_set(chip,WIRELESS_VDD_MAX);
			pm_chg_vbatdet_set(chip,WIRELESS_VDET_LOW);
			pm_chg_charge_dis(chip,0);
			if(delayed_work_pending(&chip->eoc_work)) {
				if(wake_lock_active(&chip->eoc_wake_lock))
					wake_unlock(&chip->eoc_wake_lock);
				cancel_delayed_work_sync(&chip->eoc_work);
				printk(KERN_INFO "[WIRELESS] Source-Cancel previous eoc_work\n");
			}
			if(!wake_lock_active(&chip->eoc_wake_lock))
				wake_lock(&chip->eoc_wake_lock);
			schedule_delayed_work(&chip->eoc_work,round_jiffies_relative(msecs_to_jiffies(WIRELESS_EARLY_EOC_WORK_TIME)));
			wireless_source_toggle = false;
			wireless_source_control_count++;
			printk(KERN_INFO "[WIRELESS] Source Enabled , go eoc_worker\n");
			wake_unlock(&chip->wireless_source_wake_lock);
		}
	}
	else {
		printk(KERN_INFO "[WIRELESS] Source-Do Nothing , USB=%d\n",is_usb_chg_plugged_in(chip));
	}
}


static void wireless_chip_control_worker(struct work_struct *work)
{
	struct delayed_work	*dwork = to_delayed_work(work);
	struct pm8921_chg_chip	*chip = container_of(dwork,struct pm8921_chg_chip,wireless_chip_control_work);
	//struct timeval time;

	if(wireless_is_plugged()) {
		wake_lock(&chip->wireless_chip_wake_lock);
		printk(KERN_INFO "[WIRELESS] Turn Off RX-Chip.  FSM=%d,SOC=%d\n",pm_chg_get_fsm_state(chip),wireless_soc(chip));
		pm_chg_charge_dis(chip,1);
		gpio_set_value_cansleep(CHG_STAT,WIRELESS_OFF);
		schedule_delayed_work(&chip->wireless_chip_control_work,round_jiffies_relative(msecs_to_jiffies(WIRELESS_CHIP_CONTROL_TIME)));
		wireless_source_control_count=0;
	}
	else{
		printk(KERN_INFO "[WIRELESS] Turn On RX-Chip\n");
		/*do_gettimeofday(&time);
		wireless_dc_time = 0;
		wireless_on_time = time.tv_sec;
		wireless_interrupt_enable = false;
		disable_irq(gpio_to_irq(ACTIVE_N));*/
		gpio_set_value_cansleep(CHG_STAT,WIRELESS_ON);
		wake_unlock(&chip->wireless_chip_wake_lock);
	}
}


static int wireless_is_charging_finished(struct pm8921_chg_chip *chip)
{
	int vbatdet_low;
	int ichg_meas_ma,iterm_programmed;
	int fast_chg, vcp;
	int soc;
	int rc;

	wireless_current_set(chip);

	if(!last_stop_charging) {
		fast_chg = pm_chg_get_rt_status(chip, FASTCHG_IRQ);
		printk(KERN_INFO "[WIRELESS] fast_chg = %d\n", fast_chg);
		if (fast_chg == 0) {
			if(pm_chg_get_fsm_state(chip) == FSM_STATE_TRKL_CHG_8) {
				printk(KERN_INFO "[WIRELESS] TRICKLE\n");
				return CHG_IN_PROGRESS;
			}

			if(wireless_soc_check(chip) && wireless_source_control_count < WIRELESS_SRC_COUNT_LIMIT) {
				printk(KERN_INFO "[WIRELESS] NOT FSM 7 !!!  Source Control. Count=%d\n",wireless_source_control_count);
				wireless_source_control_worker(&chip->wireless_source_control_work.work);

			}
			else {
				if(!wireless_soc_check(chip) && chargerlogo_state) // SOC   100
					return CHG_NOT_IN_PROGRESS;

				printk(KERN_INFO "[WIRELESS] NOT FSM 7 !!!  Reset TI-CHIP. Count=%d\n",wireless_source_control_count);
				wireless_chip_control_worker(&chip->wireless_chip_control_work.work);
			}
			return CHG_NOT_IN_PROGRESS;
		}
	}
	else {
		printk(KERN_INFO "[WIRELESS] Skip check fast_chg, last_stop_charing=%d\n",last_stop_charging);
		return CHG_NOT_IN_PROGRESS;
	}

	wireless_source_control_count = 0;
	vcp = pm_chg_get_rt_status(chip, VCP_IRQ);
	printk(KERN_INFO "[WIRELESS] vcp = %d\n", vcp);
	if (vcp == 1)
		return CHG_IN_PROGRESS;
	vbatdet_low = pm_chg_get_rt_status(chip, VBATDET_LOW_IRQ);
	printk(KERN_INFO "[WIRELESS] vbatdet_low = %d\n", vbatdet_low);
	//if (vbatdet_low == 1)
	//	return CHG_IN_PROGRESS;
	rc = pm_chg_get_rt_status(chip, BAT_TEMP_OK_IRQ);
	printk(KERN_INFO "[WIRELESS] batt_temp_ok = %d\n", rc);
	if (rc == 0)
		return CHG_IN_PROGRESS;
#ifdef CONFIG_BATTERY_MAX17047
	soc = max17047_get_soc();
#elif defined (CONFIG_BATTERY_MAX17043)
	soc = max17043_get_capacity(chip);
#else
	soc = get_prop_batt_capacity(chip);
#endif
	if(soc < 100) {
		printk(KERN_INFO "[WIRELESS] SOC is %d\n",soc);
		return CHG_IN_PROGRESS;
	}
	rc = pm_chg_iterm_get(chip, &iterm_programmed);
	if (rc) {
		printk(KERN_INFO "[WIRELESS] couldnt read iterm rc = %d\n", rc);
		return CHG_IN_PROGRESS;
	}
	ichg_meas_ma = (get_prop_batt_current(chip)) / 1000;
	printk(KERN_INFO "[WIRELESS] iterm_programmed = %d ichg_meas_ma=%d\n",iterm_programmed, ichg_meas_ma);
	if (ichg_meas_ma > 0) {
		printk(KERN_INFO "[WIRELESS] IBAT is providing from BATT,still CHG_IN_PROGERESS\n");
		if(soc > 99) {
			printk(KERN_INFO "[WIRELESS] SOC is %d but IBAT is providing, CHG_FINISHED\n",soc);
			return CHG_FINISHED;
		}
		else{
			return CHG_IN_PROGRESS;
		}
	}
	if (ichg_meas_ma * -1 > iterm_programmed) {
		return CHG_IN_PROGRESS;
	}
	return CHG_FINISHED;
}


static int wireless_hw_init(struct pm8921_chg_chip *chip)
{
	int rc = 0;

	if(wireless_is_plugged()) {
		printk(KERN_INFO "[WIRELESS] Boot & Probing by Wireless Charger \n");
		rc = pm_chg_charge_dis(chip,1);
		if (rc) {
			printk(KERN_INFO "[WIRELESS] Failed to disable CHG_CHARGE_DIS bit rc=%d\n", rc);
			return rc;
		}

		rc = pm_chg_auto_enable(chip,0);
		if (rc) {
			printk(KERN_INFO "[WIRELESS] Failed to enable charging rc=%d\n", rc);
			return rc;
		}
	}
	return rc;
}


static void wireless_information(struct work_struct *work)
{
	int ret=0;		int rc=0;
	int regloop=0;		int vin_min=0;		int ibat=0;
	bool dis=0;		u8 ato = 0;		u8 ibat_get=0;
	int this_ibat=0;	int fast=0;
	int maxim_volt=0;	int vbatdet_low=0;	int dc=0;
	int dc_gpio=0;		int fsm=0;		int fet=0;
	int usb=0;		int bms_volt=0;		int vbat_max=0;
	u8 temp=0;		int vdet_mv=0;		int maxim_soc=0;
	int bms_soc=0;		int iterm=0;		u8 batfet=0;

	struct pm8xxx_adc_chan_result	result;
	struct delayed_work		*dwork = to_delayed_work(work);
	struct pm8921_chg_chip		*chip = container_of(dwork,struct pm8921_chg_chip,wireless_inform_work);

	if(the_chip) {
		if(is_dc_chg_plugged_in(the_chip) || wireless_inform_no_dc < 10 || wireless_inform_enable) {
			dc	= is_dc_chg_plugged_in(the_chip);
			if(!dc && !wireless_inform_enable)
				wireless_inform_no_dc++;
			else
				wireless_inform_no_dc = 0;

			usb	= is_usb_chg_plugged_in(the_chip);
			dc_gpio = wireless_is_plugged();
			regloop	= pm_chg_get_regulation_loop(the_chip);
			vin_min = pm_chg_vinmin_get(the_chip);
			ibat 	= get_prop_batt_current(the_chip);
			fsm 	= pm_chg_get_fsm_state(the_chip);
			fet 	= pm_chg_get_rt_status(the_chip, BATFET_IRQ);
			rc 	= pm8xxx_adc_read(CHANNEL_DCIN,&result);
			dis	= pm_is_chg_charge_dis_bit_set(the_chip);
			pm8xxx_readb(the_chip->dev->parent,CHG_CNTRL_3,&ato);
			ato 	= !!(ato & CHG_EN_BIT);
			pm8xxx_readb(the_chip->dev->parent,CHG_IBAT_MAX,&ibat_get);
			ibat_get = (ibat_get&PM8921_CHG_I_MASK);
			this_ibat = (ibat_get*PM8921_CHG_I_STEP_MA)+PM8921_CHG_I_MIN_MA;
			fast 	= pm_chg_get_rt_status(the_chip, FASTCHG_IRQ);
#ifdef CONFIG_BATTERY_MAX17047
			maxim_volt = max17047_get_batt_vol();
#endif
#ifdef CONFIG_BATTERY_MAX17043
			maxim_volt = max17043_get_voltage(the_chip);
#endif
			bms_volt = get_prop_battery_uvolts(the_chip) / 1000;
			vbatdet_low = pm_chg_get_rt_status(the_chip, VBATDET_LOW_IRQ);
			pm_chg_vddmax_get(the_chip,&vbat_max);
			pm8xxx_readb(the_chip->dev->parent, CHG_VBAT_DET, &temp);
			temp = temp&PM8921_CHG_V_MASK;
			vdet_mv = temp*PM8921_CHG_V_STEP_MV+PM8921_CHG_V_MIN_MV;
#ifdef CONFIG_BATTERY_MAX17047
			maxim_soc = max17047_get_soc();
#elif defined (CONFIG_BATTERY_MAX17043)
			maxim_soc = max17043_get_capacity(the_chip);
#else
			bms_soc = get_prop_batt_capacity(the_chip);
#endif
			pm_chg_iterm_get(the_chip,&iterm);
			pm8xxx_readb(the_chip->dev->parent,CHG_CNTRL,&batfet);
			batfet 	= !!(batfet & WIRELESS_BATFET_BIT);

			printk(KERN_INFO "[WIRELESS]-%d >>>\n",wireless_inform_no_dc);
			printk(KERN_INFO "| wireless_charging=%d\n",wireless_charging);
			printk(KERN_INFO "| DC_IRQ=%d(GPIO=%d) , USB_IRQ=%d\n",dc,dc_gpio,usb);
			printk(KERN_INFO "| VIN_MIN=%dmV , DC_IN=%dmV\n",vin_min,((int)result.physical/1000));
			printk(KERN_INFO "| VBAT_MAX=%dmV , VBAT_LOW=%dmV\n" , vbat_max,vdet_mv);
			printk(KERN_INFO "| Maxim_Volt=%dmV , BMS_Volt=%dmV\n",maxim_volt,bms_volt);
			printk(KERN_INFO "| Maxim_SOC=%d , BMS_SOC=%d\n",maxim_soc,bms_soc);
			printk(KERN_INFO "| IBAT_MAX=%dmA , IBAT=%dmA , ITERM=%dmA\n",this_ibat,(ibat/1000),iterm);
			printk(KERN_INFO "| FET_IRQ=%d , FET_FORCE=%d , FSM=%d , Dis_Bit=%d , Auto_Enable=%d , RegLoop=%d\n",fet,batfet,fsm,dis,ato,regloop);
			printk(KERN_INFO "| Low_IRQ=%d , FAST_IRQ=%d\n",vbatdet_low,fast);
			printk(KERN_INFO "| chg_batt_temp_state=%d , last_stop_charging=%d , pseudo_ui_charging=%d\n",chg_batt_temp_state,last_stop_charging,pseudo_ui_charging);
			printk(KERN_INFO "[WIRELESS]-%d <<<\n",wireless_inform_no_dc);
		}
	}

	if(wireless_inform_enable)
		ret = schedule_delayed_work(&chip->wireless_inform_work,round_jiffies_relative(msecs_to_jiffies(WIRELESS_INFORM_RAPID_TIME)));
	else
		ret = schedule_delayed_work(&chip->wireless_inform_work,round_jiffies_relative(msecs_to_jiffies(WIRELESS_INFORM_NORMAL_TIME)));
}


static void wireless_probe(struct pm8921_chg_chip *chip)
{
	printk(KERN_INFO "[WIRELESS] Probe\n");
	INIT_WORK(&chip->wireless_interrupt_work,wireless_interrupt_worker);
	INIT_WORK(&chip->dcin_valid_irq_work,dcin_valid_irq_worker);
	wake_lock_init(&chip->wireless_chip_wake_lock,WAKE_LOCK_SUSPEND,"pm8921_wireless_chip");
	wake_lock_init(&chip->wireless_source_wake_lock,WAKE_LOCK_SUSPEND,"pm8921_wireless_source");
	wake_lock_init(&chip->wireless_unplug_wake_lock,WAKE_LOCK_SUSPEND,"pm8921_wireless_unplug");
	INIT_DELAYED_WORK(&chip->wireless_unplug_check_work,wireless_unplug_check_worker);
	INIT_DELAYED_WORK(&chip->wireless_chip_control_work,wireless_chip_control_worker);
	INIT_DELAYED_WORK(&chip->wireless_source_control_work,wireless_source_control_worker);
	INIT_DELAYED_WORK(&chip->wireless_inform_work,wireless_information);
	INIT_DELAYED_WORK(&chip->wireless_polling_dc_work,wireless_polling_dc_worker);
	schedule_delayed_work(&chip->wireless_inform_work,round_jiffies_relative(msecs_to_jiffies(WIRELESS_INFORM_NORMAL_TIME)));
	wireless_interrupt_probe(chip);
}
#endif


static int pm8921_charger_resume(struct device *dev)
{
	int rc;
	struct pm8921_chg_chip *chip = dev_get_drvdata(dev);

	if(!chip) {
		pr_err("%s: failed", __func__);
		return -EINVAL;
	}

#ifdef CONFIG_LGE_CHARGER_TEMP_SCENARIO
	/* Create work for temp scerario kwangjae1.lee@lge.com */
	rc = schedule_delayed_work(&chip->monitor_batt_temp_work,
			  round_jiffies_relative(msecs_to_jiffies(1)));
#endif
#ifdef CONFIG_LGE_WIRELESS_CHARGER
	if(wireless_inform_enable)
		rc = schedule_delayed_work(&chip->wireless_inform_work,round_jiffies_relative(msecs_to_jiffies(WIRELESS_INFORM_RAPID_TIME)));
	else
		rc = schedule_delayed_work(&chip->wireless_inform_work,round_jiffies_relative(msecs_to_jiffies(WIRELESS_INFORM_NORMAL_TIME)));
#endif

#if defined (CONFIG_BATTERY_MAX17047) || defined (CONFIG_BATTERY_MAX17043)
	rc = schedule_delayed_work(&chip->update_heartbeat_work,
			  round_jiffies_relative(msecs_to_jiffies(500)));
#endif

#ifdef CONFIG_LGE_PM
/* Qulcomm charging scenario off kwangjae1.lee@lge.com */
	if (!(chip->cool_temp_dc == 0 && chip->warm_temp_dc == 0)
#else /* QCT_ORG */
	if (!(chip->cool_temp_dc == INT_MIN && chip->warm_temp_dc == INT_MIN)
#endif
		&& !(chip->keep_btm_on_suspend)) {
		rc = pm8xxx_adc_btm_configure(&btm_config);
		if (rc)
			pr_err("couldn't reconfigure btm rc=%d\n", rc);

		rc = pm8xxx_adc_btm_start();
		if (rc)
			pr_err("couldn't restart btm rc=%d\n", rc);
	}
	if (pm8921_chg_is_enabled(chip, LOOP_CHANGE_IRQ)) {
		disable_irq_wake(chip->pmic_chg_irq[LOOP_CHANGE_IRQ]);
		pm8921_chg_disable_irq(chip, LOOP_CHANGE_IRQ);
	}
	return 0;
}

static int pm8921_charger_suspend(struct device *dev)
{
	int rc;
	struct pm8921_chg_chip *chip = dev_get_drvdata(dev);

	if(!chip) {
		pr_err("%s: failed", __func__);
		return -EINVAL;
	}

#ifdef CONFIG_LGE_CHARGER_TEMP_SCENARIO
/* Create work for temp scerario kwangjae1.lee@lge.com */
	rc = cancel_delayed_work_sync(&chip->monitor_batt_temp_work);
#endif

#ifdef CONFIG_LGE_WIRELESS_CHARGER
	rc = cancel_delayed_work_sync(&chip->wireless_inform_work);
#endif

#if defined (CONFIG_BATTERY_MAX17047) || defined (CONFIG_BATTERY_MAX17043)
	rc = cancel_delayed_work_sync(&chip->update_heartbeat_work);
#endif


#ifdef CONFIG_LGE_PM
/* Qulcomm charging scenario off kwangjae1.lee@lge.com */
	if (!(chip->cool_temp_dc == 0 && chip->warm_temp_dc == 0)
#else /* QCT_ORG */
	if (!(chip->cool_temp_dc == INT_MIN && chip->warm_temp_dc == INT_MIN)
#endif
		&& !(chip->keep_btm_on_suspend)) {
		rc = pm8xxx_adc_btm_end();
		if (rc)
			pr_err("Failed to disable BTM on suspend rc=%d\n", rc);
	}

#ifdef CONFIG_LGE_WIRELESS_CHARGER
	if (is_usb_chg_plugged_in(chip)||is_dc_chg_plugged_in(chip))
#else
	if (is_usb_chg_plugged_in(chip))
#endif
	{
		pm8921_chg_enable_irq(chip, LOOP_CHANGE_IRQ);
		enable_irq_wake(chip->pmic_chg_irq[LOOP_CHANGE_IRQ]);
	}

	return 0;
}
#ifdef CONFIG_LGE_CHARGER_TEMP_SCENARIO
extern void kernel_power_off(void); //low battery protect code kwangjae1.lee
/* Create work for temp scerario kwangjae1.lee@lge.com */
static void monitor_batt_temp(struct work_struct *work)
{

	struct delayed_work *dwork = to_delayed_work(work);
	struct pm8921_chg_chip *chip = container_of(dwork,
				struct pm8921_chg_chip, monitor_batt_temp_work);
	int batt_temp;
	int batt_volt;
	int stop_charging = 0;
	int ret = 0;
	int bat_voltage;
	int iusb_pre = 0;
	static unsigned int prev_chg_batt_temp_state = 0;

	if (is_factory_cable())
		batt_temp = 25;
	else
		batt_temp = get_prop_batt_temp(chip);
#ifdef CONFIG_BATTERY_MAX17047
#if defined(CONFIG_MACH_APQ8064_GVDCM)
	if(lge_get_board_revno() > HW_REV_A)
	{
		batt_volt = max17047_get_batt_vol();
		
		max17047_write_battery_temp(batt_temp);
	}
	else
		batt_volt = get_prop_battery_uvolts(chip) / 1000;

#else
	batt_volt = max17047_get_batt_vol();

	max17047_write_battery_temp(batt_temp);

#endif
#elif defined (CONFIG_BATTERY_MAX17043)
	batt_volt = max17043_get_voltage(chip);
#else
	batt_volt = get_prop_battery_uvolts(chip) / 1000;
#endif
	if(pseudo_batt_info.mode == 1)
		batt_volt = (pseudo_batt_info.volt == 0) ? batt_volt : pseudo_batt_info.volt;

	bat_voltage = get_rt_get_temp(chip);
/* LGE_S kwangjae1.lee low battery protect code */
	if( batt_volt < 3400 && ((pseudo_batt_info.mode == 1) || (batt_temp/10) < -10)){
		mdelay(2000);
		if((get_prop_battery_uvolts(chip) / 1000) < 3400){
			pr_debug(" power off, couse voltage low (%dmV) !!!! \n",batt_volt);
			kernel_power_off();
		}
	}
/* LGE_E kwangjae1.lee low battery protect code */
	pr_debug(" psudo=%d, factory=%d temp=%d, volt=%d, temp_ADC=%d\n",
		pseudo_batt_info.mode,is_factory_cable(), batt_temp, batt_volt, bat_voltage);

	stop_charging = chg_is_battery_too_hot_or_too_cold(chip, batt_temp, batt_volt);

	if(wake_lock_active(&chip->monitor_batt_temp_wake_lock) && !is_usb_chg_plugged_in(chip)
		&& !is_dc_chg_plugged_in(chip)) {
		printk(KERN_INFO "[PM] monitor_batt_temp: Release wake lock , no charger\n");
		wake_unlock(&chip->monitor_batt_temp_wake_lock);
	}
	if (stop_charging == 1 && last_stop_charging == 0) {
		if(wake_lock_active(&chip->eoc_wake_lock)) {
			printk(KERN_INFO "[PM] monitor_batt_temp: Charging stop & wake lock by Temperature Scenario\n");
			wake_lock(&chip->monitor_batt_temp_wake_lock);
		}
		/* When stop charging, next step do not resume charing kwangjae1.lee@lge.com */
		pm_chg_auto_enable(chip, 0);
		last_stop_charging = stop_charging;
	}
	else if (stop_charging == 0 && last_stop_charging == 1) {
		/* When stop charging, next step do not resume charing kwangjae1.lee@lge.com*/
		pm_chg_auto_enable(chip, 1);
		last_stop_charging = stop_charging;
		pseudo_ui_charging = 0;
#ifdef CONFIG_LGE_WIRELESS_CHARGER
		if(is_dc_chg_plugged_in(chip)) {
			printk(KERN_INFO "[WIRELESS] Enabled by Temperature Scenario !\n");
			pm_chg_charge_dis(chip,0);
			if(!wake_lock_active(&chip->eoc_wake_lock))
				wake_lock(&chip->eoc_wake_lock);
			if(!delayed_work_pending(&chip->eoc_work))
				schedule_delayed_work(&chip->eoc_work,round_jiffies_relative(msecs_to_jiffies(WIRELESS_EARLY_EOC_WORK_TIME)));
		}
#endif

		if(wake_lock_active(&chip->monitor_batt_temp_wake_lock)) {
			printk(KERN_INFO "[PM] monitor_batt_temp: Release wake lock by Temperature Scenario\n");
			wake_unlock(&chip->monitor_batt_temp_wake_lock);
		}
	}
	pm_chg_failed_clear(chip, 1);

	/*
	 * chager can get thermal_mitigation parameter from thermal daemon.
	 * so, we decide to use this parameter for charging current mitigation.
	 * if thermal daemon triggered by special threshold and do "battery" action,
	 * "battery actioninfo" value is come to charger.
	 */

	/* below 10% soc, thermal mitigation can't decrease IUSB*/
#ifdef CONFIG_BATTERY_MAX17047
	if (g_batt_soc <= 10) {
		thermal_mitigation = 0;
		soc_limit = 1;
	}
	/* above 30%, soc limit is cleared */
	else if ((soc_limit == 1) && (g_batt_soc >= 15)) {
		soc_limit = 0;
		pre_mitigation = 0;
	}
#elif defined (CONFIG_BATTERY_MAX17043)
	if (max17043_get_capacity(chip) <= 10 && thermal_mitigation == 1) {
		thermal_mitigation = 0;
		soc_limit = 1;
	}
	/* above 15%, soc limit is cleared */
	else if ((soc_limit == 1) && (max17043_get_capacity(chip) >= 15)) {
		soc_limit = 0;
		pre_mitigation = 0;
	}
#else
#ifdef CONFIG_LGE_PM
	if (chip->recent_reported_soc <= 10)
#else
	if (pm8921_bms_get_percent_charge() <= 10)
#endif
	{
		thermal_mitigation = 0;
		soc_limit = 1;
	}
	/* above 30%, soc limit is cleared */
#ifdef CONFIG_LGE_PM
	else if ((soc_limit == 1) && (chip->recent_reported_soc >= 30))
#else
	else if ((soc_limit == 1) && (pm8921_bms_get_percent_charge() >= 30))
#endif
	{
		soc_limit = 0;
		pre_mitigation = 0;
	}
#endif

	if ((!last_stop_charging) && (chg_batt_temp_state != CHG_BATT_DC_CURRENT_STATE)
			&& (the_chip->usb_psy.type == POWER_SUPPLY_TYPE_USB_DCP)) {
		if ((thermal_mitigation == 1)&&(thermal_mitigation != pre_mitigation)&&(soc_limit == 0)) {
			pm_chg_iusbmax_get(chip, &iusb_pre);
#if defined(CONFIG_MACH_APQ8064_GK_KR) || defined(CONFIG_MACH_APQ8064_GKATT) || defined(CONFIG_MACH_APQ8064_GV_KR) || defined(CONFIG_MACH_APQ8064_GKGLOBAL)
			__pm8921_charger_vbus_draw(1100);
			ChgLog("ChgLog> resetting current iusb_pre = %d : mitigation current = %d\n",iusb_pre, 1100);
#elif defined (CONFIG_MACH_APQ8064_J1D)
			__pm8921_charger_vbus_draw(900);
			ChgLog("ChgLog> resetting current iusb_pre = %d : mitigation current = %d\n",iusb_pre, 900);
#else
			__pm8921_charger_vbus_draw(700); /* set 700mA*/
			ChgLog("ChgLog> resetting current iusb_pre = %d : mitigation current = %d\n",iusb_pre, 700);
#endif
			pre_mitigation = thermal_mitigation;
		}
		else if ((thermal_mitigation == 2)&&(thermal_mitigation != pre_mitigation)&&(soc_limit == 0)) {
			pm_chg_iusbmax_get(chip, &iusb_pre);
#if defined(CONFIG_MACH_APQ8064_GK_KR) || defined(CONFIG_MACH_APQ8064_GKATT) || defined(CONFIG_MACH_APQ8064_GV_KR) || defined(CONFIG_MACH_APQ8064_GKGLOBAL)
			__pm8921_charger_vbus_draw(700);
			ChgLog("ChgLog> resetting current iusb_pre = %d : mitigation current = %d\n",iusb_pre, 700);
#else
			__pm8921_charger_vbus_draw(500); /* set 500mA*/
			ChgLog("ChgLog> resetting current iusb_pre = %d : mitigation current = %d\n",iusb_pre, 500);
#endif
			pre_mitigation = thermal_mitigation;
		}
#if defined(CONFIG_MACH_APQ8064_GK_KR) || defined(CONFIG_MACH_APQ8064_GKATT) || defined(CONFIG_MACH_APQ8064_GV_KR) || defined(CONFIG_MACH_APQ8064_GKGLOBAL)  // test code
		else if ((thermal_mitigation == 3)&&(thermal_mitigation != pre_mitigation)&&(soc_limit == 0)) {
			pm_chg_iusbmax_get(chip, &iusb_pre);
			__pm8921_charger_vbus_draw(900);
			ChgLog("ChgLog> resetting current iusb_pre = %d : mitigation current = %d\n",iusb_pre, 900);
			pre_mitigation = thermal_mitigation;
		}
#endif
		else if ((thermal_mitigation == 0)&&(thermal_mitigation != pre_mitigation)) {
			pm_chg_iusbmax_get(chip, &iusb_pre);
			__pm8921_charger_vbus_draw(ADAPTIVE_MA_TABLE[ADAPTIVE_NUM_OF_TABLE-1]);
			ChgLog("ChgLog> resetting current iusb_pre = %d : retore current = %d\n", iusb_pre, ADAPTIVE_MA_TABLE[ADAPTIVE_NUM_OF_TABLE-1]);
			pre_mitigation = thermal_mitigation;
		}
	}
#if 0//defined(CONFIG_LGE_WIRELESS_CHARGER) && !defined(CONFIG_LGE_FTT_CHARGER)
	else {
		/* 120412 mansu.lee@lge.com Replace wireless ibat mitigation code */
		if(is_dc_chg_plugged_in(chip)) {
			pr_debug("[Thermal] Wireless charger is enabled.\n");
			if ((thermal_mitigation == 1) && (thermal_mitigation != pre_mitigation)){
				printk(KERN_INFO "[WIRELESS] Thermal Mitigation:Decrease IBAT MAX to %dmA\n",WIRELESS_DECREASE_IBAT);
				pre_mitigation = thermal_mitigation;
				wireless_current_set(chip);
			}else if ((thermal_mitigation == 0) && (thermal_mitigation != pre_mitigation)){
				printk(KERN_INFO "[WIRELESS] Thermal Mitigation:Normal IBAT MAX to %dmA\n",WIRELESS_IBAT_MAX);
				pre_mitigation = thermal_mitigation;
				wireless_current_set(chip);
			}
			else{
				printk(KERN_INFO "[WIRELESS] Thermal Mitigation:current mitigation = %d / pre= %d\n", thermal_mitigation, pre_mitigation);
			}
		}
	}
#endif

#if 1
	/* DO NOT REPORT BATTERY UEVENT PERIODICALLY HERE !!!
	    It is cause of heavy load for Framework - Periodic Report is doing in Heartbeat workqueue */
	if (prev_chg_batt_temp_state != chg_batt_temp_state) {
		prev_chg_batt_temp_state = chg_batt_temp_state;
		power_supply_changed(&chip->batt_psy);
	}
#else
    power_supply_changed(&chip->batt_psy);
#endif

	ret = schedule_delayed_work(&chip->monitor_batt_temp_work,
					  round_jiffies_relative(msecs_to_jiffies
								 (60000)));
/*Shawn-Dbg*/{
	#include <linux/msm_tsens.h>
	int Vbat, Ibat;
	struct pm8xxx_adc_chan_result Vchg;
	struct tsens_device tsens_dev;
	unsigned long temp = 0;

	tsens_dev.sensor_num = 7; // CPU0
	tsens_get_temp(&tsens_dev, &temp);

	pm8xxx_adc_read(CHANNEL_USBIN, &Vchg);
	pm8921_bms_get_simultaneous_battery_voltage_and_current(&Ibat, &Vbat);
	pm_chg_iusbmax_get(chip, &iusb_pre);
	ChgLog("ChgLog>, soc:%d, vbat:%d, ibat:%d, vchg:%lld, fsm:%d-%d, Ttsens:%ld, Tbatt:%d, iusb_pre:%d\n",
		chip->recent_reported_soc /* pm8921_bms_get_percent_charge() */, Vbat, 0-Ibat, Vchg.physical,
		is_battery_charging(pm_chg_get_fsm_state(chip)), pm_chg_get_fsm_state(chip), temp, batt_temp,iusb_pre);
/*Shawn-Dbg*/}
}
#endif

static int __devinit pm8921_charger_probe(struct platform_device *pdev)
{
	int rc = 0;
	struct pm8921_chg_chip *chip;
	const struct pm8921_charger_platform_data *pdata
				= pdev->dev.platform_data;

	if (!pdata) {
		pr_err("missing platform data\n");
		return -EINVAL;
	}

	chip = kzalloc(sizeof(struct pm8921_chg_chip),
					GFP_KERNEL);
	if (!chip) {
		pr_err("Cannot allocate pm_chg_chip\n");
		return -ENOMEM;
	}

	chip->dev = &pdev->dev;
	chip->safety_time = pdata->safety_time;
	chip->ttrkl_time = pdata->ttrkl_time;
	chip->update_time = pdata->update_time;
	chip->max_voltage_mv = pdata->max_voltage;
	chip->min_voltage_mv = pdata->min_voltage;
	chip->uvd_voltage_mv = pdata->uvd_thresh_voltage;
	chip->resume_voltage_delta = pdata->resume_voltage_delta;
	chip->resume_charge_percent = pdata->resume_charge_percent;
	chip->term_current = pdata->term_current;
	chip->vbat_channel = pdata->charger_cdata.vbat_channel;
	chip->batt_temp_channel = pdata->charger_cdata.batt_temp_channel;
	chip->batt_id_channel = pdata->charger_cdata.batt_id_channel;
	chip->batt_id_min = pdata->batt_id_min;
	chip->batt_id_max = pdata->batt_id_max;
	if (pdata->cool_temp != INT_MIN)
		chip->cool_temp_dc = pdata->cool_temp * 10;
	else
		chip->cool_temp_dc = INT_MIN;

	if (pdata->warm_temp != INT_MIN)
		chip->warm_temp_dc = pdata->warm_temp * 10;
	else
		chip->warm_temp_dc = INT_MIN;

	chip->temp_check_period = pdata->temp_check_period;
	chip->dc_unplug_check = pdata->dc_unplug_check;
	chip->max_bat_chg_current = pdata->max_bat_chg_current;
	chip->cool_bat_chg_current = pdata->cool_bat_chg_current;
	chip->warm_bat_chg_current = pdata->warm_bat_chg_current;
	chip->cool_bat_voltage = pdata->cool_bat_voltage;
	chip->warm_bat_voltage = pdata->warm_bat_voltage;
	chip->keep_btm_on_suspend = pdata->keep_btm_on_suspend;
	chip->trkl_voltage = pdata->trkl_voltage;
	chip->weak_voltage = pdata->weak_voltage;
	chip->trkl_current = pdata->trkl_current;
	chip->weak_current = pdata->weak_current;
	chip->vin_min = pdata->vin_min;
	chip->thermal_mitigation = pdata->thermal_mitigation;
	chip->thermal_levels = pdata->thermal_levels;

	chip->cold_thr = pdata->cold_thr;
	chip->hot_thr = pdata->hot_thr;
	chip->rconn_mohm = pdata->rconn_mohm;
	chip->led_src_config = pdata->led_src_config;
	chip->has_dc_supply = pdata->has_dc_supply;

#ifdef CONFIG_LGE_PM
	/* MAKO patch for BMS */
	chip->eoc_check_soc = pdata->eoc_check_soc;
#endif

#ifdef CONFIG_LGE_CHARGER_TEMP_SCENARIO
	chip->temp_level_1 = pdata->temp_level_1;
#if defined(CONFIG_MACH_APQ8064_J1SP)
	/* Add temp for charing scenario on SPRINT */
	chip->temp_level_1_1 = pdata->temp_level_1_1;
#endif
	chip->temp_level_2 = pdata->temp_level_2;
	chip->temp_level_3 = pdata->temp_level_3;
	chip->temp_level_4 = pdata->temp_level_4;
	chip->temp_level_5 = pdata->temp_level_5;
	last_stop_charging = 0;
	chg_batt_temp_state = CHG_BATT_NORMAL_STATE;
	pseudo_ui_charging = 0;
	last_usb_chg_current = 0;
	/* 120712 cs.kim@lge.com Implements Thermal Mitigation iUSB setting */
	pre_mitigation = 0;
	soc_limit = 0;
#endif

	rc = pm8921_chg_hw_init(chip);
	if (rc) {
		pr_err("couldn't init hardware rc=%d\n", rc);
		goto free_chip;
	}

	chip->usb_psy.name = "usb",
	chip->usb_psy.type = POWER_SUPPLY_TYPE_USB,
	chip->usb_psy.supplied_to = pm_power_supplied_to,
	chip->usb_psy.num_supplicants = ARRAY_SIZE(pm_power_supplied_to),
	chip->usb_psy.properties = pm_power_props_usb,
	chip->usb_psy.num_properties = ARRAY_SIZE(pm_power_props_usb),
	chip->usb_psy.get_property = pm_power_get_property_usb,
	chip->usb_psy.set_property = pm_power_set_property_usb,

#ifdef CONFIG_LGE_WIRELESS_CHARGER
	chip->wireless_psy.name = "wireless",
	chip->wireless_psy.type = POWER_SUPPLY_TYPE_WIRELESS,
	chip->wireless_psy.supplied_to = pm_power_supplied_to,
	chip->wireless_psy.num_supplicants = ARRAY_SIZE(pm_power_supplied_to),
	chip->wireless_psy.properties = pm_power_props_wireless,
	chip->wireless_psy.num_properties = ARRAY_SIZE(pm_power_props_wireless),
	chip->wireless_psy.get_property = pm_power_get_property_wireless,
#endif
	chip->dc_psy.name = "pm8921-dc",
	chip->dc_psy.type = POWER_SUPPLY_TYPE_MAINS,
	chip->dc_psy.supplied_to = pm_power_supplied_to,
	chip->dc_psy.num_supplicants = ARRAY_SIZE(pm_power_supplied_to),
	chip->dc_psy.properties = pm_power_props_mains,
	chip->dc_psy.num_properties = ARRAY_SIZE(pm_power_props_mains),
	chip->dc_psy.get_property = pm_power_get_property_mains,

	chip->batt_psy.name = "battery",
	chip->batt_psy.type = POWER_SUPPLY_TYPE_BATTERY,
	chip->batt_psy.properties = msm_batt_power_props,
	chip->batt_psy.num_properties = ARRAY_SIZE(msm_batt_power_props),
	chip->batt_psy.get_property = pm_batt_power_get_property,
#ifndef CONFIG_LGE_WIRELESS_CHARGER
	chip->batt_psy.external_power_changed = pm_batt_external_power_changed,
#else
	chip->batt_psy.external_power_changed = NULL,


	rc = power_supply_register(chip->dev, &chip->wireless_psy);
	if (rc < 0) {
		pr_err("power_supply_register wireless failed rc = %d\n", rc);
		goto free_chip;
	}
#endif

	rc = power_supply_register(chip->dev, &chip->usb_psy);
	if (rc < 0) {
		pr_err("power_supply_register usb failed rc = %d\n", rc);
#ifdef CONFIG_LGE_WIRELESS_CHARGER
		goto unregister_wireless;
#else
		goto free_chip;
#endif
	}

	rc = power_supply_register(chip->dev, &chip->dc_psy);
	if (rc < 0) {
		pr_err("power_supply_register dc failed rc = %d\n", rc);
		goto unregister_usb;
	}

	rc = power_supply_register(chip->dev, &chip->batt_psy);
	if (rc < 0) {
		pr_err("power_supply_register batt failed rc = %d\n", rc);
		goto unregister_dc;
	}

	platform_set_drvdata(pdev, chip);
	the_chip = chip;

#ifdef CONFIG_LGE_PM
	/* Initialize wake lock for deliver Uevent before suspend */
	/* Move it to before IRQ request because very very rarely it can be called by IRQ before initialized */
	wake_lock_init(&chip->deliver_uevent_wake_lock,WAKE_LOCK_SUSPEND, "deliver_uevent");
#endif

#ifdef CONFIG_LGE_PM
	wake_lock_init(&uevent_wake_lock, WAKE_LOCK_SUSPEND, "pm8921_chg_uevent");
#endif
	wake_lock_init(&chip->eoc_wake_lock, WAKE_LOCK_SUSPEND, "pm8921_eoc");
	INIT_DELAYED_WORK(&chip->eoc_work, eoc_worker);
#ifdef CONFIG_LGE_PM_ADP_CHG
	wake_lock_init(&chip->unplug_wrkarnd_restore_wake_lock,
			WAKE_LOCK_SUSPEND, "pm8921_unplug_wrkarnd");
	INIT_DELAYED_WORK(&chip->unplug_wrkarnd_restore_work,
					unplug_wrkarnd_restore_worker);
#else
	/* Revert USB unplug detection to 1043 - seonghun1.kim */
	INIT_DELAYED_WORK(&chip->vin_collapse_check_work,
						vin_collapse_check_worker);
#endif
	INIT_DELAYED_WORK(&chip->unplug_check_work, unplug_check_worker);

	INIT_WORK(&chip->bms_notify.work, bms_notify);
	INIT_WORK(&chip->battery_id_valid_work, battery_id_valid);

	INIT_DELAYED_WORK(&chip->update_heartbeat_work, update_heartbeat);

#ifdef CONFIG_LGE_PM_REMOVE_BATT
	/* Restart the machine when Battery Remove/Insert - seonghun1.kim */
	INIT_WORK(&chip->kernel_restart_work, kernel_restart_worker);
#endif
#ifdef CONFIG_LGE_PM_ADP_CHG
	/* Adapive USB draw current limit */
	INIT_DELAYED_WORK(&chip->adaptive_usb_current_work, adaptive_usb_current_worker);
#endif

	rc = request_irqs(chip, pdev);
	if (rc) {
		pr_err("couldn't register interrupts rc=%d\n", rc);
		goto unregister_batt;
	}

	enable_irq_wake(chip->pmic_chg_irq[USBIN_VALID_IRQ]);
	enable_irq_wake(chip->pmic_chg_irq[DCIN_VALID_IRQ]);
	enable_irq_wake(chip->pmic_chg_irq[VBATDET_LOW_IRQ]);
	enable_irq_wake(chip->pmic_chg_irq[FASTCHG_IRQ]);
#ifdef CONFIG_LGE_WIRELESS_CHARGER
	enable_irq_wake(chip->pmic_chg_irq[DCIN_VALID_IRQ]);
	enable_irq_wake(chip->pmic_chg_irq[DCIN_OV_IRQ]);
	enable_irq_wake(chip->pmic_chg_irq[DCIN_UV_IRQ]);
#endif
	/*
	 * if both the cool_temp_dc and warm_temp_dc are invalid device doesnt
	 * care for jeita compliance
	 */
#ifdef CONFIG_LGE_PM
/* Qulcomm charging scenario off kwangjae1.lee@lge.com */
	if (!(chip->cool_temp_dc == 0 && chip->warm_temp_dc == 0))
#else  /* QCT_ORG */
	if (!(chip->cool_temp_dc == INT_MIN && chip->warm_temp_dc == INT_MIN))
#endif
	{
		rc = configure_btm(chip);
		if (rc) {
			pr_err("couldn't register with btm rc=%d\n", rc);
			goto free_irq;
		}
	}

	create_debugfs_entries(chip);

#ifdef CONFIG_LGE_WIRELESS_CHARGER
	wireless_probe(chip);
#endif

	/* determine what state the charger is in */
	determine_initial_state(chip);

#ifdef CONFIG_MACH_LGE
	rc = device_create_file(&pdev->dev, &dev_attr_at_current);
	rc = device_create_file(&pdev->dev, &dev_attr_at_charge);
	rc = device_create_file(&pdev->dev, &dev_attr_at_chcomp);
	rc = device_create_file(&pdev->dev, &dev_attr_at_pmrst);
	rc = device_create_file(&pdev->dev, &dev_attr_at_vcoin);
#endif

#ifdef CONFIG_LGE_CHARGER_TEMP_SCENARIO
	rc = device_create_file(&pdev->dev, &dev_attr_batt_temp_adc);
#endif

#if defined(CONFIG_MACH_APQ8064_GK_KR) || defined(CONFIG_MACH_APQ8064_GV_KR)
	rc = device_create_file(&pdev->dev, &dev_attr_hardreset_dis);
#endif

	if (chip->update_time)
	{
#ifdef CONFIG_BATTERY_MAX17047
//doosan.baek@lge.com 20121207 Change time that is updating SOC and voltage of battery for exact value in start screen of the charging mode.
		if (lge_get_charger_logo_state())
			schedule_delayed_work(&chip->update_heartbeat_work,
						round_jiffies_relative(msecs_to_jiffies
						(FIRST_CHANGING_MODE_UPDATE_TIME)));
		else
#endif
			schedule_delayed_work(&chip->update_heartbeat_work,
						round_jiffies_relative(msecs_to_jiffies
						(chip->update_time)));
	}

#ifdef CONFIG_LGE_CHARGER_TEMP_SCENARIO
/* Create work for temp scerario kwangjae1.lee@lge.com */
	wake_lock_init(&chip->monitor_batt_temp_wake_lock,
					WAKE_LOCK_SUSPEND,"pm8921_monitor_batt_temp");

	INIT_DELAYED_WORK(&chip->monitor_batt_temp_work,
					monitor_batt_temp);

	rc = schedule_delayed_work(&chip->monitor_batt_temp_work,
				  round_jiffies_relative(msecs_to_jiffies
						(10000)));
	pr_debug("%s rc = %d\n",__func__, rc);
#endif

	return 0;

free_irq:
	free_irqs(chip);
unregister_batt:
	power_supply_unregister(&chip->batt_psy);
unregister_dc:
	power_supply_unregister(&chip->dc_psy);
unregister_usb:
	power_supply_unregister(&chip->usb_psy);
#ifdef CONFIG_LGE_WIRELESS_CHARGER
unregister_wireless:
	power_supply_unregister(&chip->wireless_psy);
#endif
free_chip:
	kfree(chip);
	return rc;
}

static int __devexit pm8921_charger_remove(struct platform_device *pdev)
{
	struct pm8921_chg_chip *chip = platform_get_drvdata(pdev);

#ifdef CONFIG_MACH_LGE
	device_remove_file(&pdev->dev, &dev_attr_at_current); /* sungsookim */
	device_remove_file(&pdev->dev, &dev_attr_at_charge);
	device_remove_file(&pdev->dev, &dev_attr_at_chcomp);
	device_remove_file(&pdev->dev, &dev_attr_at_pmrst);
	device_remove_file(&pdev->dev, &dev_attr_at_vcoin);
#endif
#ifdef CONFIG_LGE_CHARGER_TEMP_SCENARIO
	device_remove_file(&pdev->dev, &dev_attr_batt_temp_adc);
#endif
#if defined(CONFIG_MACH_APQ8064_GK_KR) || defined(CONFIG_MACH_APQ8064_GV_KR)
	device_remove_file(&pdev->dev, &dev_attr_hardreset_dis);
#endif

#ifdef CONFIG_LGE_PM
	wake_lock_destroy(&uevent_wake_lock);
#endif

	free_irqs(chip);
	platform_set_drvdata(pdev, NULL);
	the_chip = NULL;
	kfree(chip);
	return 0;
}
static const struct dev_pm_ops pm8921_pm_ops = {
	.suspend	= pm8921_charger_suspend,
	.suspend_noirq  = pm8921_charger_suspend_noirq,
	.resume_noirq   = pm8921_charger_resume_noirq,
	.resume		= pm8921_charger_resume,
};
static struct platform_driver pm8921_charger_driver = {
	.probe		= pm8921_charger_probe,
	.remove		= __devexit_p(pm8921_charger_remove),
	.driver		= {
			.name	= PM8921_CHARGER_DEV_NAME,
			.owner	= THIS_MODULE,
			.pm	= &pm8921_pm_ops,
	},
};

static int __init pm8921_charger_init(void)
{
	return platform_driver_register(&pm8921_charger_driver);
}

static void __exit pm8921_charger_exit(void)
{
	platform_driver_unregister(&pm8921_charger_driver);
}

late_initcall(pm8921_charger_init);
module_exit(pm8921_charger_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("PMIC8921 charger/battery driver");
MODULE_VERSION("1.0");
MODULE_ALIAS("platform:" PM8921_CHARGER_DEV_NAME);
