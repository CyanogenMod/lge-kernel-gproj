#include <linux/module.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/earlysuspend.h>
#include <linux/power_supply.h>
#include <linux/mfd/pm8xxx/ftt-charger.h>
#include <linux/mfd/pm8xxx/pm8921-charger.h>

#define FTT_CHAEGER_DEBUG	2

#if (FTT_CHAEGER_DEBUG == 1)
#define DPRINTK(x...) printk(KERN_INFO"FTT) " x)
#elif (FTT_CHAEGER_DEBUG == 2)
#define DPRINTK(x...) {\
if(ftt_is_debug == 1) printk(KERN_INFO"FTT) " x); \
}
#else
#define DPRINTK(x...)
#endif

#define FTT_BOARD_VERSION		2
#define FTT_CHARGER_SYSFS							1

#define FTT_CHARGER_STATUS_TIMER			1
#define FTT_SPIN_LOCK
#define FTT_MAGNETIC_SENSOR						0

#define DEVICE_NAME "ftt_charger"

#define FTT_DD_MAJOR_VERSION					0
#define FTT_DD_MINOR_VERSION_A					9
#define FTT_DD_MINOR_VERSION_B					2

#define FTT_FREQUENCY_COMPARE_COUNT					(5)
#define FTT_FREQUENCY_SAMPLE_COUNT						(500)

#define FTT_PING_POLLING_COUNT								(2)//(10)

#define FTT_CHARGER_STATUS_PING_DETECT_LOOP_COUNT										(4)
#define FTT_CHARGER_STATUS_CHARGING_LOOP_COUNT											(2)

#define FTT_BATT_CHARGING_LOW_LEVEL						(1)
#define FTT_BATT_CHARGING_WARRNING						(-1)
#define FTT_BATT_CHARGING_NO_CHARGING					(-2)
#define FTT_BATT_CHARGING_NO_DEFINED_TYPE			(-3)


#define FTT_1n_SEC						(1)
#define FTT_10n_SEC						(10)
#define FTT_100n_SEC					(100)
#define FTT_1u_SEC						(1000)
#define FTT_10u_SEC						(10000)
#define FTT_100u_SEC					(100000)
#define FTT_1m_SEC						(1000000)
#define FTT_10m_SEC					(10000000)
#define FTT_100m_SEC					(100000000)
#define FTT_1_SEC						(1000000000)
#define FTT_AVERIGE_TIME				FTT_1m_SEC
//#define FTT_BOOTING_TIME				30 //CIDT_jhpark_121120 [부팅시간 30초 내에는 ping type을 A1과 B1으로 한다]
//#define FTT_IS_BTYPE						110 //CIDT_jhpark_121123 [pad는 B1 타입인데 실제 측정되는 주파는 B1이 아닌 경우 A1으로 설정한다.]

//#define FTT_SYS_PERMISSION	(S_IRWXUGO)
#define FTT_SYS_PERMISSION	(S_IRWXU | S_IRGRP | S_IROTH)

#if FTT_CHARGER_STATUS_TIMER
static struct timer_list ftt_charger_status_timer;
#endif

struct ftt_charger_chip {
	struct device *dev;
	int en1;
	int detect;
	int ftt;
	unsigned int ftt_irq;
};
static int prev_ant_level = 0;


static struct power_supply *wireless_psy;

static struct ftt_charger_chip *gpdata;

static bool ftt_enable_flag=true;
static bool ftt_chip_enable_flag=false;

static char batt_chg_enable = 0;

struct pad_type {
	char pad_num;
	char pad_type_name[10];
	u32 ping_freq;
	u32 ping_freq_min;
	u32 ping_freq_max;
};

enum ftt_pad_type {
	FTT_PAD_A1_TYPE = 1,
	FTT_PAD_A2_TYPE,
	FTT_PAD_A3_TYPE,
	FTT_PAD_A4_TYPE,
	FTT_PAD_A5_TYPE,
	FTT_PAD_A6_TYPE,
	FTT_PAD_A7_TYPE,
	FTT_PAD_A8_TYPE,
	FTT_PAD_A9_TYPE,
	FTT_PAD_B1_TYPE,
	FTT_PAD_B2_TYPE,
	FTT_PAD_B3_TYPE,
};

static struct pad_type pad_type_table[] = {
		{FTT_PAD_A1_TYPE, "A1", 175000, 170000, 180000},
		{FTT_PAD_A2_TYPE, "A2", 138000, 135000, 141000},
		{FTT_PAD_A3_TYPE, "A3", 138000, 135000, 141000},
		{FTT_PAD_A4_TYPE, "A4", 130000, 127000, 133000},
		{FTT_PAD_A5_TYPE, "A5", 175000, 170000, 180000},
		{FTT_PAD_A6_TYPE, "A6", 175000, 170000, 180000},
		{FTT_PAD_A7_TYPE, "A7", 138000, 135000, 141000},
		{FTT_PAD_A8_TYPE, "A8", 130000, 127000, 133000},
		{FTT_PAD_A9_TYPE, "A9", 115000, 110000, 120000},
		{FTT_PAD_B1_TYPE, "B1", 105000, 102000, 108000},
		{FTT_PAD_B2_TYPE, "B2", 105000, 102000, 108000},
		{FTT_PAD_B3_TYPE, "B3", 105000, 102000, 108000},
};

#define FTT_A1_MAX_CHARGING_FREQ		210000
#define FTT_A1_MIN_CHARGING_FREQ		110000
struct ant_level_type {
	u32 ping_freq;
	char ant_level;
};

static struct ant_level_type ant_level_type_A1_A5_table[] = {
		{175, 5},
		{140, 4},
		{120, 3},
		{115, 2},
		{110, 1},
		{109, 0},
};

static struct ant_level_type ant_level_type_A9_table[] = {
		{110, 5},
		{107, 3},
		{106, 0},
};

static struct ant_level_type ant_level_type_A4_A8_table[] = {
		{150, 5},
		{145, 4},
		{135, 3},
		{125, 2},
		{120, 1},
		{115, 0},
};

static struct ant_level_type ant_level_type_A3_A7_table[] = {
		{130, 5},
		{120, 3},
		{110, 1},
		{105, 0},
};

static struct ant_level_type ant_level_type_A6_table[] = {
		{190, 5},
		{180, 4},
		{160, 3},
		{140, 2},
		{130, 1},
		{120, 0},
};

#if FTT_CHARGER_STATUS_TIMER
#define FTT_STATUS_DEFAULT_TIMER_MSEC							(1000)
#define FTT_STATUS_INIT_TIMER_MSEC								(5)
#define FTT_STATUS_INIT_STATE_TIMER_MSEC							(1000)
#define FTT_STATUS_START_TIMER_MSEC								(5000)
#define FTT_STATUS_STANDBY_TIMER_MSEC							(1000)
#define FTT_STATUS_PRE_DETECT_TIMER_MSEC						(100)
#define FTT_STATUS_RESET_TIMER_MSEC								(1000)			/* 1000ms에서 3000ms로 수정 : A2, A3 빨리 리셋하면 Ping이 틀리게 나옴 */
#define FTT_STATUS_PING_DETECT_TIMER_MSEC					(100)
#define FTT_STATUS_PRE_CHARGING_TIMER_MSEC					(1000)					/* 3000ms로 수정 : B1, B2, B3 는 빨리 주파수가 아오지 않는다. */
#define FTT_STATUS_NO_PRE_CHARGING_TIMER_MSEC			(100)
#define FTT_STATUS_CHARGING_TIMER_MSEC						(1000)
#define FTT_STATUS_EARLYSUSPEND_CHARGING_TIMER_MSEC		(2000)
#define FTT_STATUS_LOW_LEVEL_CHARGING_TIMER_MSEC		(1000)
#endif

/*=======================================================================================*/
//sysfs variable
/*=======================================================================================*/

static int freq_cnt_n =		FTT_FREQUENCY_COMPARE_COUNT;
static int freq_smp_n =		FTT_FREQUENCY_SAMPLE_COUNT;

static int ping_det_t =		FTT_STATUS_PING_DETECT_TIMER_MSEC;
static int charging_t =		FTT_STATUS_CHARGING_TIMER_MSEC;

/*=======================================================================================*/

enum ftt_charger_status {
	FTT_CHARGER_STATUS_NULL,
	FTT_CHARGER_STATUS_INIT,
	FTT_CHARGER_STATUS_STANDBY,
	FTT_CHARGER_STATUS_PRE_DETECT,
	FTT_CHARGER_STATUS_RESET,
	FTT_CHARGER_STATUS_PING_DETECT,
	FTT_CHARGER_STATUS_PRE_CHARGING,
	FTT_CHARGER_STATUS_LOW_LEVEL_CHARGING,
	FTT_CHARGER_STATUS_CHARGING,
};

static struct ftt_status_data {
	u32 next_timer;
	enum ftt_charger_status prev_ftt_status;
	enum ftt_charger_status ftt_status;
	enum ftt_charger_status ftt_next_status;
} ftt_status_d = {FTT_STATUS_STANDBY_TIMER_MSEC, FTT_CHARGER_STATUS_INIT, FTT_CHARGER_STATUS_INIT, FTT_CHARGER_STATUS_INIT};


static u32 pad_type_num = 0;

#ifdef FTT_SPIN_LOCK
spinlock_t ftt_frequency_lock;
unsigned long ftt_frequency_lock_flags;
#endif

#if (FTT_MAGNETIC_SENSOR == 1)
u32 ftt_magnetic_utesla = 0;
#endif

#if (FTT_CHAEGER_DEBUG == 2)
static u32 ftt_is_debug = 1;
#endif

static u32 ftt_ping_loop = 1;
static u32 ftt_charging_loop = 0;
static u32 ftt_pre_detect_flag = 0;

static u32 ftt_curr_frequency = 0;

static int ftt_enabled = 0;
#ifdef	CONFIG_HAS_EARLYSUSPEND
static u32 ftt_earlysuspend = false;
static struct early_suspend stftt_earlysuspend;
static void ftt_early_suspend(struct early_suspend *h);
static void ftt_early_resume(struct early_suspend *h);
#endif

static bool is_ftt_charging(void);
static u32 get_ftt_charger_next_time(void);
static bool is_ftt_charge_status(void);
static void ftt_charger_status_timer_handler(unsigned long data);

extern int pm8921_is_dc_chg_plugged_in(void);

static int set_ftt_enabled(const char *val, const struct kernel_param *kp)
{
	int ret = 0;

	ret = param_set_bool(val, kp);

	if(!gpdata) {
		pr_err("%s called before init\n", __func__);
		return -EINVAL;
	}

	if(ftt_enabled)
	{
		if(pm8921_is_dc_chg_plugged_in()) {
			pad_type_num = FTT_PAD_A1_TYPE;
			ftt_status_d.ftt_status = FTT_CHARGER_STATUS_CHARGING;
#if FTT_CHARGER_STATUS_TIMER
			if( timer_pending(&ftt_charger_status_timer))
				del_timer(&ftt_charger_status_timer);
			init_timer(&ftt_charger_status_timer);
			ftt_charger_status_timer.function = ftt_charger_status_timer_handler;
			ftt_charger_status_timer.data = (unsigned long)gpdata;
			ftt_charger_status_timer.expires = jiffies + msecs_to_jiffies(5);
			add_timer(&ftt_charger_status_timer);
#endif
		}
//		else {
//			enable_irq(gpdata->ftt_irq);
//		}
	}

	printk(KERN_INFO "%s ftt_enabled(%d) dc_plug(%d) \n",
		__func__, ftt_enabled, pm8921_is_dc_chg_plugged_in());

	return ret;
}

static struct kernel_param_ops module_ops = {
	.set = set_ftt_enabled,
	.get = param_get_bool,
};
module_param_cb(ftt_enabled, &module_ops, &ftt_enabled, 0644);


#ifdef	CONFIG_HAS_EARLYSUSPEND
static void ftt_early_suspend(struct early_suspend *h)
{
	ftt_earlysuspend = true;
}

static void ftt_early_resume(struct early_suspend *h)
{
	ftt_earlysuspend = false;
}
#endif

u32 get_ftt_frequency_poll(void)
{
	register u32 count_pulse = 0;
	u32 freq;
	register u8 prev, curr = 0;
	u64 s_time, e_time;
	u32 d_time;
	u32 count = 0;

#ifdef FTT_SPIN_LOCK
	spin_lock_irqsave(&ftt_frequency_lock, ftt_frequency_lock_flags);
#endif
	s_time = ktime_to_ns(ktime_get());
	for (count=0;count<freq_smp_n;count++) {
		prev = curr;
		curr = gpio_get_value(gpdata->ftt);
		if (curr == 0 && prev == 1) {
			count_pulse++;
		}
	}
	e_time = ktime_to_ns(ktime_get());
	d_time = e_time - s_time;

#ifdef FTT_SPIN_LOCK
	spin_unlock_irqrestore(&ftt_frequency_lock, ftt_frequency_lock_flags);
#endif

	freq = (u32)(FTT_1_SEC / d_time * count_pulse);

	//DPRINTK("FREQ_POLL count= %u delta=%u %uKHz\n",count_pulse, d_time/1000, freq/1000 );

	return freq;
}

static enum ftt_charger_status get_ftt_charger_current_status(void)
{
	return ftt_status_d.ftt_status;
}

static enum ftt_charger_status get_ftt_charger_prev_status(void)
{
	return ftt_status_d.prev_ftt_status;
}

static bool is_ftt_precharging(void)
{
	switch (pad_type_num) {
	case FTT_PAD_A4_TYPE :
	case FTT_PAD_B1_TYPE :
	case FTT_PAD_B2_TYPE :
	case FTT_PAD_B3_TYPE :
		return 1;
		break;
	default :
		return 0;
	}
	return 0;
}

u32 get_ftt_frequency_poll_ex(void)
{
	u32 max_freq = 0;
	u32 ret_freq;
	u32 i;

	for(i=0;i<freq_cnt_n;i++) {
		ret_freq = get_ftt_frequency_poll();
		if (max_freq < ret_freq) max_freq = ret_freq;
	}

	return max_freq;
}

u32 get_ftt_ping_frequency(void)
{
	u32 ftt_ping_frequency;
	u32 i;

	for(i=0;i<FTT_PING_POLLING_COUNT;i++) {
		ftt_ping_frequency = get_ftt_frequency_poll_ex();
		if (ftt_ping_frequency != 0) break;
	}
	DPRINTK("### PING FTT_FREQUANCY=%uHz  %u.%01ukHz\n", ftt_ping_frequency, ftt_ping_frequency/1000, ftt_ping_frequency%1000/100);

	return ftt_ping_frequency;
}

u32 get_ftt_pad_type_num(void)
{
	u32 ftt_ping_frequency;
	u32 i;
	u32 table_size;

	ftt_ping_frequency = get_ftt_ping_frequency();
	table_size = sizeof(pad_type_table)/sizeof(struct pad_type);
	for (i=0;i<table_size;i++) {
		if ( (ftt_ping_frequency >= pad_type_table[i].ping_freq_min) && (ftt_ping_frequency <= pad_type_table[i].ping_freq_max) )
			return pad_type_table[i].pad_num;
	}
	return 0;
}

char* get_ftt_pad_type_str(void)
{
	u32 ftt_ping_frequency;
	u32 i;
	u32 table_size;

	ftt_ping_frequency = get_ftt_ping_frequency();
	table_size = sizeof(pad_type_table)/sizeof(struct pad_type);
	for (i=0;i<table_size;i++) {
		if ( (ftt_ping_frequency >= pad_type_table[i].ping_freq_min) && (ftt_ping_frequency <= pad_type_table[i].ping_freq_max) )
			return pad_type_table[i].pad_type_name;
	}
	return NULL;
}

u32 get_ftt_saved_pad_type_num(void)
{
	switch (get_ftt_charger_current_status()) {
	case	FTT_CHARGER_STATUS_PRE_CHARGING :
	case	FTT_CHARGER_STATUS_LOW_LEVEL_CHARGING :
	case	FTT_CHARGER_STATUS_CHARGING :
		return pad_type_num;
		break;
	default :
		return 0;
	}
	return 0;
}

char* get_ftt_pad_type_num_to_str(u32 pad_num)
{
	u32 i;
	u32 table_size;

	table_size = sizeof(pad_type_table)/sizeof(struct pad_type);
	for (i=0;i<table_size;i++) {
		if ( pad_num == pad_type_table[i].pad_num ) {
			return pad_type_table[i].pad_type_name;
		}
	}
	return NULL;
}

char* get_ftt_saved_pad_type_str(void)
{
    char *get_pad_type;
	if (pad_type_num != 0) {
		switch (get_ftt_charger_current_status()) {
		case	FTT_CHARGER_STATUS_PRE_DETECT :
		case	FTT_CHARGER_STATUS_RESET :
		case	FTT_CHARGER_STATUS_PING_DETECT :
		case	FTT_CHARGER_STATUS_PRE_CHARGING :
		case	FTT_CHARGER_STATUS_LOW_LEVEL_CHARGING :
		case	FTT_CHARGER_STATUS_CHARGING :
		    get_pad_type = get_ftt_pad_type_num_to_str(pad_type_num);
		    if(get_pad_type != NULL)
		        return get_pad_type;
		    break;
		default :
			return "Unkown";
		}
	}
	return "Unkown";
}

/************
RX Chip Enable
*************/
u32 ftt_chip_enable(void)
{
	if( ftt_chip_enable_flag )
	{
		gpio_set_value_cansleep(gpdata->en1,0);
		ftt_chip_enable_flag = false;
	}

	return 0;
}

/************
RX Chip Disable
************/
u32 ftt_chip_disable(void)
{
	if( !ftt_chip_enable_flag )
	{
		gpio_set_value_cansleep(gpdata->en1,1);
		ftt_chip_enable_flag = true;
	}

	return 0;
}

bool ftt_get_detect(void)
{
	return !gpio_get_value_cansleep(gpdata->detect);
}

/******************************
ftt_irq enable
******************************/
void ftt_enable(void)
{
	if(!ftt_enabled)
		return;

	if (ftt_enable_flag == false) {
		enable_irq(gpdata->ftt_irq);
		ftt_enable_flag = true;
//		DPRINTK("%s\n", __FUNCTION__);
	}
}
EXPORT_SYMBOL_GPL(ftt_enable);

/******************************
ftt_irq disable
******************************/
void ftt_disable(void)
{
	if (ftt_enable_flag) {
		disable_irq_nosync(gpdata->ftt_irq);
		ftt_enable_flag = false;
//		DPRINTK("%s\n", __FUNCTION__);
	}
}

//CIDT_jhpark_121121 [io ctl로 Chip enable/Disable 하게 할 함수]
void ftt_charge_enable(void)
{
	#ifdef FTT_SPIN_LOCK
	spin_lock_irqsave(&ftt_frequency_lock, ftt_frequency_lock_flags);
	#endif

	ftt_status_d.ftt_status = FTT_CHARGER_STATUS_INIT;
	ftt_status_d.ftt_next_status = FTT_CHARGER_STATUS_INIT;
	ftt_status_d.prev_ftt_status = FTT_CHARGER_STATUS_INIT;

	if( timer_pending(&ftt_charger_status_timer))
		del_timer(&ftt_charger_status_timer);

	if (!batt_chg_enable) {
		batt_chg_enable = 1;
	}

	ftt_enable();

	#ifdef FTT_SPIN_LOCK
	spin_unlock_irqrestore(&ftt_frequency_lock, ftt_frequency_lock_flags);
	#endif
}

void ftt_charge_disable(void)
{
	int ant_level;

	#ifdef FTT_SPIN_LOCK
	spin_lock_irqsave(&ftt_frequency_lock, ftt_frequency_lock_flags);
	#endif

	ftt_status_d.ftt_status = FTT_CHARGER_STATUS_INIT;
	ftt_status_d.ftt_next_status = FTT_CHARGER_STATUS_INIT;
	ftt_status_d.prev_ftt_status = FTT_CHARGER_STATUS_INIT;

	if( timer_pending(&ftt_charger_status_timer))
		del_timer(&ftt_charger_status_timer);

	if (batt_chg_enable) {
		batt_chg_enable = 0;
		ftt_curr_frequency = 0;
	}

	ant_level = get_ftt_ant_level();
	if( prev_ant_level != ant_level) {
		#ifdef FTT_UEVENT //CIDT_jhpark_121106
		if (gftt_supply.name) power_supply_changed(&gftt_supply);
		#endif
		prev_ant_level = ant_level;
	}

	ftt_disable();

	#ifdef FTT_SPIN_LOCK
	spin_unlock_irqrestore(&ftt_frequency_lock, ftt_frequency_lock_flags);
	#endif
}

void set_ftt_batt_charge_enable(bool enable)
{
	if( enable )
	{
		ftt_charge_enable();
	}
	else
	{
		ftt_charge_disable();
	}
}

/***************************************************************************
Fuction : ftt_charger_isr
*****************************************************************************/
static irqreturn_t ftt_charger_isr(int irq, void *dev_id)
{
	struct ftt_charger_chip *chip = (struct ftt_charger_chip*)dev_id;
	unsigned long flags;

	DPRINTK("%s ftt_enabled(%d) detect(%d) dc_plug(%d)\n", 
		__func__, ftt_enabled, ftt_get_detect(), pm8921_is_dc_chg_plugged_in());

	if(!ftt_enabled) {
		disable_irq_nosync(gpdata->ftt_irq);
		return IRQ_HANDLED;
	}

	local_irq_save(flags);

#ifdef FTT_SPIN_LOCK
	spin_lock_irqsave(&ftt_frequency_lock, ftt_frequency_lock_flags);
#endif

	disable_irq_nosync(gpdata->ftt_irq);

	ftt_status_d.ftt_status = FTT_CHARGER_STATUS_PING_DETECT;

#if FTT_CHARGER_STATUS_TIMER
	if( timer_pending(&ftt_charger_status_timer))
		del_timer(&ftt_charger_status_timer);
	init_timer(&ftt_charger_status_timer);
	ftt_charger_status_timer.function = ftt_charger_status_timer_handler;
	ftt_charger_status_timer.data = (unsigned long)chip;
	ftt_charger_status_timer.expires = jiffies + msecs_to_jiffies(FTT_STATUS_INIT_TIMER_MSEC);
	add_timer(&ftt_charger_status_timer);
#endif

#ifdef FTT_SPIN_LOCK
	spin_unlock_irqrestore(&ftt_frequency_lock, ftt_frequency_lock_flags);
#endif

	local_irq_restore(flags);

	return IRQ_HANDLED;
}

/***************************************************************************
Fuction : set_ftt_charge_isr
*****************************************************************************/
void set_ftt_charge_isr(void* pdata)
{
	struct ftt_charger_chip* chip = (struct ftt_charger_chip*)pdata;
	int ant_level;

	if(chip == NULL) return;

#ifdef FTT_SPIN_LOCK
	spin_lock_irqsave(&ftt_frequency_lock, ftt_frequency_lock_flags);
#endif

	ftt_status_d.ftt_status = FTT_CHARGER_STATUS_INIT;
	ftt_status_d.ftt_next_status = FTT_CHARGER_STATUS_INIT;
	ftt_status_d.prev_ftt_status = FTT_CHARGER_STATUS_INIT;
	if( timer_pending(&ftt_charger_status_timer))
		del_timer(&ftt_charger_status_timer);

	if (batt_chg_enable) {
		batt_chg_enable = 0;
		ftt_curr_frequency = 0;
	}

	ant_level = get_ftt_ant_level();
	if( prev_ant_level != ant_level) {
		wireless_psy = power_supply_get_by_name("wireless");
		if (wireless_psy)
			power_supply_changed(wireless_psy);
		else
			pr_err("couldn't get wireless power supply\n");
		prev_ant_level = ant_level;
	}

	enable_irq(gpdata->ftt_irq);

	DPRINTK("++++++++++++++ FTT_CHARGER_STATUS_INTERRUP(%d) +++++++++++++++\n", ftt_get_detect());

#ifdef FTT_SPIN_LOCK
	spin_unlock_irqrestore(&ftt_frequency_lock, ftt_frequency_lock_flags);
#endif
}


#if FTT_CHARGER_STATUS_TIMER
static void set_ftt_charger_status(enum ftt_charger_status next_status)
{
	ftt_status_d.ftt_next_status = next_status;

	switch (ftt_status_d.ftt_next_status) {
	case FTT_CHARGER_STATUS_INIT :
		ftt_status_d.next_timer = FTT_STATUS_INIT_STATE_TIMER_MSEC;
		break;
	case FTT_CHARGER_STATUS_STANDBY :
		ftt_status_d.next_timer = FTT_STATUS_STANDBY_TIMER_MSEC;
		break;
	case FTT_CHARGER_STATUS_PRE_DETECT :
		ftt_status_d.next_timer = FTT_STATUS_PRE_DETECT_TIMER_MSEC;
		break;
	case FTT_CHARGER_STATUS_RESET :
		ftt_status_d.next_timer = FTT_STATUS_RESET_TIMER_MSEC;
		break;
	case FTT_CHARGER_STATUS_PING_DETECT :
		ftt_status_d.next_timer = ping_det_t;
		break;
	case FTT_CHARGER_STATUS_PRE_CHARGING :
		if (is_ftt_precharging()) {
			ftt_status_d.next_timer = FTT_STATUS_PRE_CHARGING_TIMER_MSEC;
		}
		else {
			ftt_status_d.next_timer = FTT_STATUS_NO_PRE_CHARGING_TIMER_MSEC;
		}
		break;
	case FTT_CHARGER_STATUS_CHARGING :
		if (ftt_earlysuspend) {
			ftt_status_d.next_timer = FTT_STATUS_EARLYSUSPEND_CHARGING_TIMER_MSEC;
		}
		else {
			ftt_status_d.next_timer = charging_t;
		}
		break;
	case FTT_CHARGER_STATUS_LOW_LEVEL_CHARGING :
		ftt_status_d.next_timer = FTT_STATUS_LOW_LEVEL_CHARGING_TIMER_MSEC;
		break;
	default :
		ftt_status_d.next_timer = FTT_STATUS_STANDBY_TIMER_MSEC;
		break;
	}
	return;
}

static void set_ftt_charger_save_status(void)
{
	ftt_status_d.prev_ftt_status = ftt_status_d.ftt_status;
	ftt_status_d.ftt_status = ftt_status_d.ftt_next_status;

	if( (ftt_status_d.ftt_status != FTT_CHARGER_STATUS_NULL) &&
		(ftt_status_d.ftt_status != FTT_CHARGER_STATUS_INIT))
		mod_timer(&ftt_charger_status_timer, jiffies + msecs_to_jiffies(get_ftt_charger_next_time()));
	return;
}

static bool is_ftt_charger_nochange_status(void)
{
	return ((ftt_status_d.prev_ftt_status == ftt_status_d.ftt_status) ? 1 : 0);
}

static u32 get_ftt_charger_next_time(void)
{
	return ftt_status_d.next_timer;
}

static bool is_ftt_charging(void)
{
	if (ftt_get_detect() == 1) {
		return 1;
	}
	return 0;
}

static bool is_ftt_low_level_charging(void)
{
	if ((ftt_get_detect() == 1) && (get_ftt_charger_current_status() == FTT_CHARGER_STATUS_LOW_LEVEL_CHARGING)) {
		return 1;
	}
	return 0;
}

static bool is_ftt_charge_status(void)
{
	if ((ftt_get_detect() == 1) && ((get_ftt_charger_current_status() == FTT_CHARGER_STATUS_CHARGING)
		|| (get_ftt_charger_current_status() == FTT_CHARGER_STATUS_PRE_CHARGING)
		|| (get_ftt_charger_current_status() == FTT_CHARGER_STATUS_LOW_LEVEL_CHARGING)
		)) {
			return 1;
	}
	return 0;
}

int get_ftt_ant_level(void)
{
	int level = FTT_BATT_CHARGING_WARRNING;
	u32 ftt_frequency_kb;
	u32 table_size;
	u32 i;
	u32 pad_type;

	if (is_ftt_charge_status()) {
		ftt_frequency_kb = get_ftt_frequency_poll_ex()/1000;
		pad_type = get_ftt_saved_pad_type_num();

#if 0
		//CIDT_jhpark_121123 [pad는 B1 타입인데 실제 측정되는 주파는 B1이 아닌 경우 A1으로 설정한다.]
		if( (pad_type >= FTT_PAD_B1_TYPE && pad_type <= FTT_PAD_B3_TYPE) &&
			ftt_frequency_kb > FTT_A1_MIN_CHARGING_FREQ/1000 )
		{
			pad_type = FTT_PAD_A1_TYPE;
			pad_type_num = pad_type;
		}
#endif

		switch (pad_type) {
		case FTT_PAD_A1_TYPE :
		case FTT_PAD_A5_TYPE :
			table_size = sizeof(ant_level_type_A1_A5_table)/sizeof(struct ant_level_type);
			for (i=0;i<table_size;i++) {
				if (ant_level_type_A1_A5_table[i].ping_freq <= ftt_frequency_kb) {
					level = ant_level_type_A1_A5_table[i].ant_level;
					break;
				}
				else {
					level = FTT_BATT_CHARGING_WARRNING;
				}
			}
			break;
		case FTT_PAD_A9_TYPE :
			table_size = sizeof(ant_level_type_A9_table)/sizeof(struct ant_level_type);
			for (i=0;i<table_size;i++) {
				if (ant_level_type_A9_table[i].ping_freq <= ftt_frequency_kb) {
					level = ant_level_type_A9_table[i].ant_level;
					break;
				}
				else {
					level = FTT_BATT_CHARGING_WARRNING;
				}
			}
			break;
		case FTT_PAD_A4_TYPE :
		case FTT_PAD_A8_TYPE :
			table_size = sizeof(ant_level_type_A4_A8_table)/sizeof(struct ant_level_type);
			for (i=0;i<table_size;i++) {
				if (ant_level_type_A4_A8_table[i].ping_freq <= ftt_frequency_kb) {
					level = ant_level_type_A4_A8_table[i].ant_level;
					break;
				}
				else {
					level = FTT_BATT_CHARGING_WARRNING;
				}
			}
			break;
		case FTT_PAD_B1_TYPE :
		case FTT_PAD_B2_TYPE :
		case FTT_PAD_B3_TYPE :
			level = 5;
			break;
		case  FTT_PAD_A2_TYPE :
			level = 5;
			break;
		case FTT_PAD_A3_TYPE :
		case FTT_PAD_A7_TYPE :
			table_size = sizeof(ant_level_type_A3_A7_table)/sizeof(struct ant_level_type);
			for (i=0;i<table_size;i++) {
				if (ant_level_type_A3_A7_table[i].ping_freq <= ftt_frequency_kb) {
					level = ant_level_type_A3_A7_table[i].ant_level;
					break;
				}
				else {
					level = FTT_BATT_CHARGING_WARRNING;
				}
			}
			break;
		case FTT_PAD_A6_TYPE :
			table_size = sizeof(ant_level_type_A6_table)/sizeof(struct ant_level_type);
			for (i=0;i<table_size;i++) {
				if (ant_level_type_A6_table[i].ping_freq <= ftt_frequency_kb) {
					level = ant_level_type_A6_table[i].ant_level;
					break;
				}
				else {
					level = FTT_BATT_CHARGING_WARRNING;
				}
			}
			break;
		default :
			level = FTT_BATT_CHARGING_NO_DEFINED_TYPE;
			break;
		}
		DPRINTK("FTT_FREQUANCY=%uHz  %u.%01ukHz PAD TYPE=%u LEVEL=%d\n", ftt_frequency_kb, ftt_frequency_kb/1000, ftt_frequency_kb%1000/100, pad_type, level);
		//DPRINTK("####### pad_type(%d) level(%d) #######\n",pad_type, level);
	}
	else {
		if (!ftt_pre_detect_flag || (get_ftt_charger_current_status() == FTT_CHARGER_STATUS_STANDBY)) {
			level = FTT_BATT_CHARGING_NO_CHARGING;
		}
		else {
			level = 5;
		}
	}	

	return level;
}
EXPORT_SYMBOL_GPL(get_ftt_ant_level);

static void ftt_charger_status_timer_handler(unsigned long data)
{
	int ant_level;
	static char curr_detect = 0;
	u32 local_pad_num = 0;
	struct ftt_charger_chip *chip = (struct ftt_charger_chip *)data;

	switch (get_ftt_charger_current_status())
	{
		case	FTT_CHARGER_STATUS_PING_DETECT :
			if (!is_ftt_charger_nochange_status())
			{
				DPRINTK("+++++++++++++ FTT_CHARGER_STATUS_PING_DETECT +++++++++++++\n");
			}
			local_pad_num = get_ftt_pad_type_num();
			DPRINTK("############ PAD TYPE = %s(%u) ############\n", get_ftt_pad_type_num_to_str(local_pad_num), local_pad_num);
			if (local_pad_num != 0)
			{
				/********************************************************
				검출된 TX Pad Type이 있으면 현재 상태를 PRE_CHARGING 상태로 변경
				********************************************************/
				pad_type_num = local_pad_num;
				set_ftt_charger_status(FTT_CHARGER_STATUS_PRE_CHARGING);
				ftt_ping_loop = 1;
			}
			else
			{
				if (ftt_ping_loop < FTT_CHARGER_STATUS_PING_DETECT_LOOP_COUNT)
				{
					/********************************************************
					패드 값을 읽지 못할 경우 바로 종료 하지 않고
					FTT_CHARGER_STATUS_PING_DETECT_LOOP_COUNT 만큼 Ping Detect 시킴
					********************************************************/
					set_ftt_charger_status(FTT_CHARGER_STATUS_PING_DETECT);
					ftt_ping_loop++;
				}
				else
				{
					/******************************
					패드값을 읽지 못할 경우 A1 Type 충전주파수 확인
					*******************************/
					ftt_curr_frequency = get_ftt_frequency_poll_ex();
					if ((ftt_curr_frequency >= FTT_A1_MIN_CHARGING_FREQ) &&
							(ftt_curr_frequency <= FTT_A1_MAX_CHARGING_FREQ))
					{
						pad_type_num = FTT_PAD_A1_TYPE;
						set_ftt_charger_status(FTT_CHARGER_STATUS_PRE_CHARGING);
						ftt_ping_loop = 1;
					}
					else if (ftt_curr_frequency == 0) {
						/******************************
						패드값을 읽지 못할 경우 FTT 루틴 종료
						*******************************/
						set_ftt_charge_isr(chip);
						ftt_ping_loop = 1;
					}
					else {
						set_ftt_charger_status(FTT_CHARGER_STATUS_PRE_CHARGING);
						ftt_ping_loop = 1;
					}
				}
			}
			break;
		case	FTT_CHARGER_STATUS_PRE_CHARGING :

			if (!is_ftt_charger_nochange_status())
			{
				DPRINTK("+++++++++++ FTT_CHARGER_STATUS_PRE_CHARGING +++++++++++\n");
			}
			set_ftt_charger_status(FTT_CHARGER_STATUS_CHARGING);
			break;
		case	FTT_CHARGER_STATUS_CHARGING :
			if (!is_ftt_charger_nochange_status())
			{
				DPRINTK("+++++++++++++ FTT_CHARGER_STATUS_CHARGING +++++++++++++\n");
			}
			curr_detect = ftt_get_detect();
			ftt_curr_frequency = get_ftt_frequency_poll_ex();
			ant_level = get_ftt_ant_level();
			if (ftt_curr_frequency)
			{
//----->
				local_pad_num = get_ftt_saved_pad_type_num();
				//pad는 B1 타입인데 실제 측정되는 주파는 B1이 아닌 경우 A1으로 설정한다.
				if( (local_pad_num >= FTT_PAD_B1_TYPE && local_pad_num <= FTT_PAD_B3_TYPE) &&
						ftt_curr_frequency > FTT_A1_MIN_CHARGING_FREQ )
				{
					pad_type_num = FTT_PAD_A1_TYPE;
					ant_level = get_ftt_ant_level();
				}
//-----<
				if( prev_ant_level != ant_level) {
					wireless_psy = power_supply_get_by_name("wireless");
					if (wireless_psy)
						power_supply_changed(wireless_psy);
					else
						pr_err("couldn't get wireless power supply\n");
				}
				if (ant_level <= FTT_BATT_CHARGING_WARRNING)
				{
					/**********************************
					패드에 따른 주파수 레벨이 검출 되지 않을때
					***********************************/
					if (is_ftt_precharging()) {
					/***************************************************************************
					 is_ftt_precharging 함수가 지정한는 TX PAD 타입은 Ping 이후 검출하는 주파가 늦게 출력되기 때문에
					주파가 검출 되지 않으면 바로 종료 시키지 않고 FTT_CHARGER_STATUS_CHARGING_LOOP_COUNT
					만큼 기다려 주파수를 검출한다.
					****************************************************************************/
						if (get_ftt_charger_prev_status() == FTT_CHARGER_STATUS_PRE_CHARGING)
						{
							if (ftt_charging_loop < FTT_CHARGER_STATUS_CHARGING_LOOP_COUNT)
							{
								set_ftt_charger_status(FTT_CHARGER_STATUS_PRE_CHARGING);
								ftt_charging_loop++;
							}
						}
						else
						{
							set_ftt_charger_status(FTT_CHARGER_STATUS_LOW_LEVEL_CHARGING);
							ftt_charging_loop = 0;
						}
					}
				}
				else
				{
					if (!batt_chg_enable)
					{
						batt_chg_enable = 1;
					}
					set_ftt_charger_status(FTT_CHARGER_STATUS_CHARGING);
				}
				/**************************************
				현재 배터리 level값을 이전 레벨값으로 변경
				***************************************/
				prev_ant_level = ant_level;
			}
			else
			{
				if (is_ftt_precharging())
				{
					/***************************************************************************
					 is_ftt_precharging 함수가 지정한는 TX PAD 타입은 Ping 이후 검출하는 주파가 늦게 출력되기 때문에
					주파가 검출 되지 않으면 바로 종료 시키지 않고 FTT_CHARGER_STATUS_CHARGING_LOOP_COUNT
					만큼 기다려 주파수를 검출한다.
					****************************************************************************/
					if (get_ftt_charger_prev_status() == FTT_CHARGER_STATUS_PRE_CHARGING)
					{
						if (ftt_charging_loop < FTT_CHARGER_STATUS_CHARGING_LOOP_COUNT)
						{
							set_ftt_charger_status(FTT_CHARGER_STATUS_PRE_CHARGING);
							ftt_charging_loop++;
						}
					}
					else
					{
					set_ftt_charge_isr(chip);
						ftt_charging_loop = 0;

					}
				}
				else
				{
					/******************************
					주파가 검출 되지 않을때
					*******************************/
				set_ftt_charge_isr(chip);
				ftt_charging_loop = 0;

				}
			}
			break;
		case	FTT_CHARGER_STATUS_LOW_LEVEL_CHARGING :
		/***********************************
		패드에 따른 주파수 레벨이 검출 되지 않을때
		***********************************/
			if (!is_ftt_charger_nochange_status())
			{
				DPRINTK("++++++++ FTT_CHARGER_STATUS_LOW_LEVEL_CHARGING ++++++++\n");
			}
			ftt_curr_frequency = get_ftt_frequency_poll_ex();
			curr_detect = ftt_get_detect();
			if (	ftt_curr_frequency && (curr_detect == 1))
			{
				ant_level = get_ftt_ant_level();
				DPRINTK("FTT_Frequency=%uHz(%u.%01ukHz) PAD_type=%s Level=%d\n", ftt_curr_frequency, ftt_curr_frequency/1000, ftt_curr_frequency%1000/100, get_ftt_pad_type_num_to_str(pad_type_num), ant_level);
				if (ant_level >= FTT_BATT_CHARGING_LOW_LEVEL)
				{
					/*****************************************************************
					패드에 따른 주파수 레벨이 검출 되면 FTT_CHARGER_STATUS_CHARGING 상태로 변경
					******************************************************************/
					set_ftt_charger_status(FTT_CHARGER_STATUS_CHARGING);
				}
				else
				{
				set_ftt_charge_isr(chip);
				}
			}
			else
			{
				/******************************
				주파가 검출 되지 않을때
				*******************************/
			set_ftt_charge_isr(chip);

			}
			break;
		case FTT_CHARGER_STATUS_INIT :
		case FTT_CHARGER_STATUS_STANDBY :
		default :
			set_ftt_charge_isr(chip);
			set_ftt_charger_status(FTT_CHARGER_STATUS_PING_DETECT);
			break;
		}

		set_ftt_charger_save_status();
		return;
}
#endif

#if FTT_CHARGER_SYSFS
/*****************************
/sys/devices/platform/ftt_charger/
******************************/
static	ssize_t show_ftt_total	(struct device *dev, struct device_attribute *attr, char *buf);
static	DEVICE_ATTR(ftt_total, FTT_SYS_PERMISSION, show_ftt_total, NULL);
static	ssize_t show_ftt_frequency	(struct device *dev, struct device_attribute *attr, char *buf);
static	DEVICE_ATTR(ftt_frequency, FTT_SYS_PERMISSION, show_ftt_frequency, NULL);
static	ssize_t show_ftt_is_charging	(struct device *dev, struct device_attribute *attr, char *buf);
static	DEVICE_ATTR(ftt_is_charging, FTT_SYS_PERMISSION, show_ftt_is_charging, NULL);
static	ssize_t show_ftt_ant_level	(struct device *dev, struct device_attribute *attr, char *buf);
static	DEVICE_ATTR(ftt_ant_level, FTT_SYS_PERMISSION, show_ftt_ant_level, NULL);
static	ssize_t show_ftt_pad_type	(struct device *dev, struct device_attribute *attr, char *buf);
static	DEVICE_ATTR(ftt_pad_type, FTT_SYS_PERMISSION, show_ftt_pad_type, NULL);
static	ssize_t show_ftt_is_low_level	(struct device *dev, struct device_attribute *attr, char *buf);
static	DEVICE_ATTR(ftt_is_low_level, FTT_SYS_PERMISSION, show_ftt_is_low_level, NULL);
static	ssize_t show_ftt_status(struct device *dev, struct device_attribute *attr, char *buf);
static	DEVICE_ATTR(ftt_status, FTT_SYS_PERMISSION, show_ftt_status, NULL);
#if (FTT_MAGNETIC_SENSOR == 1)
static	ssize_t show_ftt_magnetic_sensor_utesla(struct device *dev, struct device_attribute *attr, char *buf);
static 	ssize_t set_ftt_magnetic_sensor_utesla(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static	DEVICE_ATTR(ftt_magnetic_sensor_utesla, FTT_SYS_PERMISSION, show_ftt_magnetic_sensor_utesla, set_ftt_magnetic_sensor_utesla);
#endif
#if (FTT_CHAEGER_DEBUG == 2)
static	ssize_t show_ftt_debug(struct device *dev, struct device_attribute *attr, char *buf);
static 	ssize_t set_ftt_debug(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static	DEVICE_ATTR(ftt_debug, FTT_SYS_PERMISSION, show_ftt_debug, set_ftt_debug);
#endif
static	ssize_t show_ftt_ver(struct device *dev, struct device_attribute *attr, char *buf);
static	DEVICE_ATTR(ftt_ver, FTT_SYS_PERMISSION, show_ftt_ver, NULL);
static	ssize_t show_ftt_version(struct device *dev, struct device_attribute *attr, char *buf);
static	DEVICE_ATTR(ftt_version, FTT_SYS_PERMISSION, show_ftt_version, NULL);
static	ssize_t show_ftt_var_freq_cnt_n(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t set_ftt_var_freq_cnt_n(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static	DEVICE_ATTR(ftt_var_freq_cnt_n, FTT_SYS_PERMISSION, show_ftt_var_freq_cnt_n, set_ftt_var_freq_cnt_n);
static	ssize_t show_ftt_var_freq_smp_n(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t set_ftt_var_freq_smp_n(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static	DEVICE_ATTR(ftt_var_freq_smp_n, FTT_SYS_PERMISSION, show_ftt_var_freq_smp_n, set_ftt_var_freq_smp_n);
static	ssize_t show_ftt_var_ping_det_t(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t set_ftt_var_ping_det_t(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static	DEVICE_ATTR(ftt_var_ping_det_t, FTT_SYS_PERMISSION, show_ftt_var_ping_det_t, set_ftt_var_ping_det_t);
static	ssize_t show_ftt_var_charging_t(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t set_ftt_var_charging_t(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static	DEVICE_ATTR(ftt_var_charging_t, FTT_SYS_PERMISSION, show_ftt_var_charging_t, set_ftt_var_charging_t);
static ssize_t set_ftt_cmd(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static	DEVICE_ATTR(ftt_cmd, FTT_SYS_PERMISSION, NULL, set_ftt_cmd);

static struct attribute *ftt_frequency_sysfs_entries[] = {
	&dev_attr_ftt_total.attr,
	&dev_attr_ftt_frequency.attr,
	&dev_attr_ftt_is_charging.attr,
	&dev_attr_ftt_ant_level.attr,
	&dev_attr_ftt_pad_type.attr,
	&dev_attr_ftt_is_low_level.attr,
	&dev_attr_ftt_status.attr,
#if (FTT_MAGNETIC_SENSOR == 1)
	&dev_attr_ftt_magnetic_sensor_utesla.attr,
#endif
#if (FTT_CHAEGER_DEBUG == 2)
		&dev_attr_ftt_debug.attr,
#endif
	&dev_attr_ftt_ver.attr,
	&dev_attr_ftt_version.attr,
	&dev_attr_ftt_var_freq_cnt_n.attr,
	&dev_attr_ftt_var_freq_smp_n.attr,
	&dev_attr_ftt_var_ping_det_t.attr,
	&dev_attr_ftt_var_charging_t.attr,
	&dev_attr_ftt_cmd.attr,
	NULL
};

static struct attribute_group ftt_frequency_sysfs_attr_group = {
	.name   = NULL,
	.attrs  = ftt_frequency_sysfs_entries,
};

static	ssize_t show_ftt_total	(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf,
			"PAD TYPE : %s\n"
			"PAD NUMBER : %u\n"
			"FTT Frequency : %uHz\n"
			"LEVEL : %d\n"
#if (FTT_MAGNETIC_SENSOR == 1)
			"Megnetic uTesla : %u\n"
#endif
			,get_ftt_saved_pad_type_str()
			,get_ftt_saved_pad_type_num()
			,get_ftt_frequency_poll_ex()
			,get_ftt_ant_level()
#if (FTT_MAGNETIC_SENSOR == 1)
			,ftt_magnetic_utesla
#endif
			);
}

static 	ssize_t show_ftt_frequency(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", ftt_curr_frequency);
}

static	ssize_t show_ftt_is_charging(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", is_ftt_charging());
}

static	ssize_t show_ftt_ant_level(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", get_ftt_ant_level());
}

static	ssize_t show_ftt_pad_type(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", get_ftt_saved_pad_type_str());
}

static	ssize_t show_ftt_is_low_level(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", is_ftt_low_level_charging());
}

static	ssize_t show_ftt_status(struct device *dev, struct device_attribute *attr, char *buf)
{
	switch (get_ftt_charger_current_status()) {
	case	FTT_CHARGER_STATUS_STANDBY :
		return sprintf(buf, "%s\n", "STANDBY");
		break;
	case	FTT_CHARGER_STATUS_PRE_DETECT :
		return sprintf(buf, "%s\n", "PRE DETECT");
		break;
	case	FTT_CHARGER_STATUS_RESET :
		return sprintf(buf, "%s\n", "RESET");
		break;
	case	FTT_CHARGER_STATUS_PING_DETECT :
		return sprintf(buf, "%s\n", "DETECT");
		break;
	case	FTT_CHARGER_STATUS_PRE_CHARGING :
		return sprintf(buf, "%s\n", "PRE_CHARGING");
		break;
	case	FTT_CHARGER_STATUS_CHARGING :
		return sprintf(buf, "%s\n", "CHARGING");
		break;
	case	FTT_CHARGER_STATUS_LOW_LEVEL_CHARGING :
		return sprintf(buf, "%s\n", "LOW LEVEL CHARGING");
		break;
	default :
		return sprintf(buf, "%s\n", "NULL");
		break;
	}
}

#if (FTT_MAGNETIC_SENSOR == 1)
static	ssize_t show_ftt_magnetic_sensor_utesla(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", ftt_magnetic_utesla);
}

static 	ssize_t set_ftt_magnetic_sensor_utesla(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	sscanf(buf, "%u", &ftt_magnetic_utesla);
	return count;
}
#endif

#if (FTT_CHAEGER_DEBUG == 2)
static	ssize_t show_ftt_debug(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", ftt_is_debug);
}

static 	ssize_t set_ftt_debug(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	sscanf(buf, "%u", &ftt_is_debug);
	return count;
}
#endif

static	ssize_t show_ftt_ver(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "v%u.%u.%u\n", FTT_DD_MAJOR_VERSION, FTT_DD_MINOR_VERSION_A, FTT_DD_MINOR_VERSION_B);
}

static	ssize_t show_ftt_version(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s : v%u.%u.%u ,compile : %s, %s\n", DEVICE_NAME, FTT_DD_MAJOR_VERSION, FTT_DD_MINOR_VERSION_A, FTT_DD_MINOR_VERSION_B, __DATE__, __TIME__);
}
static	ssize_t show_ftt_var_freq_cnt_n(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", freq_cnt_n);
}
static ssize_t set_ftt_var_freq_cnt_n(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	sscanf(buf, "%u", &freq_cnt_n);
	return count;
}
static	ssize_t show_ftt_var_freq_smp_n(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", freq_smp_n);
}
static ssize_t set_ftt_var_freq_smp_n(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	sscanf(buf, "%u", &freq_smp_n);
	return count;
}
static	ssize_t show_ftt_var_ping_det_t(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", ping_det_t);
}
static ssize_t set_ftt_var_ping_det_t(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	sscanf(buf, "%u", &ping_det_t);
	return count;
}
static	ssize_t show_ftt_var_charging_t(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", charging_t);
}
static ssize_t set_ftt_var_charging_t(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	sscanf(buf, "%u", &charging_t);
	return count;
}

static ssize_t set_ftt_cmd(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	u32 d_time;

	sscanf(buf, "%u", &d_time);

	if (d_time == 0) {
		DPRINTK("***CHIP DISABLE***\n");
		ftt_chip_disable();
	}
	else if (d_time == 1) {
		DPRINTK("***CHIP ENABLE***\n");
		ftt_chip_enable();
	}
	else {
		DPRINTK("***CHIP DISABLE*** d_time=%d\n", d_time);
		ftt_chip_disable();
		msleep(d_time);
		DPRINTK("***CHIP ENABLE*** d_time=%d\n", d_time);
		ftt_chip_enable();
	}

	return count;
}
#endif

static int __init ftt_probe(struct platform_device *pdev)
{
	int ret;
	struct ftt_charger_chip *chip;
	struct ftt_charger_pdata *pdata = pdev->dev.platform_data;
	if (!pdata)
		return -EINVAL;

	chip = kzalloc(sizeof(struct ftt_charger_chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->dev = &pdev->dev;
	chip->detect = pdata->detect;
	chip->en1 = pdata->en1;
	chip->ftt = pdata->ftt;
	chip->ftt_irq = MSM_GPIO_TO_INT(pdata->ftt);

	printk(KERN_INFO "%s\n", __func__);

	ftt_curr_frequency = 0;
	platform_set_drvdata(pdev, chip);
	gpdata = chip;

	ret = gpio_request(chip->ftt, "ftt_frequency");
	if (ret < 0) {
		printk(KERN_ERR	"%s: ftt_frequency  %d request failed\n",
			__func__, chip->ftt);
		goto err_free_mem;
	}
	gpio_direction_input(chip->ftt);

	ret = request_irq(chip->ftt_irq, ftt_charger_isr,
		IRQF_TRIGGER_RISING, FTT_CHARGER_DEV_NAME, chip);
	if(ret < 0) {
		printk(KERN_ERR "%s: Can't allocate irq %d, ret %d\n",
			__func__, chip->ftt_irq, ret);
		goto err_free_gpio;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	stftt_earlysuspend.suspend = ftt_early_suspend;
	stftt_earlysuspend.resume = ftt_early_resume;
	register_early_suspend(&stftt_earlysuspend);
#endif

#if FTT_CHARGER_SYSFS
	ret = sysfs_create_group(&pdev->dev.kobj, &ftt_frequency_sysfs_attr_group);
	if (ret) {
		printk(KERN_ERR "%s: failed to sysfs_create_group\n",__func__);
		goto err_request_irq;
	}
#endif

	printk(KERN_INFO "%s Initialized.. \n", __func__);
	return 0;

err_request_irq:
	free_irq(chip->ftt_irq, chip);
err_free_gpio:
	gpio_free(chip->ftt);
err_free_mem:
	kfree(chip);
	return ret;
}

static int __exit ftt_remove(struct platform_device *pdev)
{
	struct ftt_charger_chip *chip = platform_get_drvdata(pdev);

	free_irq(chip->ftt_irq, chip);
	gpio_free(chip->ftt);
	platform_set_drvdata(pdev, NULL);
	kfree(chip);
	gpdata = NULL;
#if FTT_CHARGER_STATUS_TIMER
	del_timer(&ftt_charger_status_timer);
#endif
#if FTT_CHARGER_SYSFS
	sysfs_remove_group(&pdev->dev.kobj, &ftt_frequency_sysfs_attr_group);
#endif
	return 0;
}

static struct platform_driver ftt_charger_driver = {
	.driver = {
		.name = FTT_CHARGER_DEV_NAME,
		.owner = THIS_MODULE,
	},
	.remove = __exit_p(ftt_remove),
};

static int __init ftt_init(void)
{
	return platform_driver_probe(&ftt_charger_driver, ftt_probe);
}
module_init(ftt_init);

static void __exit ftt_exit(void)
{
	platform_driver_unregister(&ftt_charger_driver);
}
module_exit(ftt_exit);

MODULE_AUTHOR("Uhm choon ho");
MODULE_DESCRIPTION("Driver for FTT Charget module Ver 0.9.2");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:CIDT FTT");
