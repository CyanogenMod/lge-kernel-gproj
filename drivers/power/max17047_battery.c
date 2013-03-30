/*
 * Fuel gauge driver for Maxim 17047 / 8966 / 8997
 *  Note that Maxim 8966 and 8997 are mfd and this is its subdevice.
 *
 * Copyright (C) 2012 LG Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * This driver is based on max17040_battery.c
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mod_devicetable.h>
#include <linux/power_supply.h>
#include <linux/max17047_battery.h>
#include <linux/mfd/pm8xxx/pm8921-charger.h>

#include <linux/delay.h>

//#include <mach/board_lge.h>
#include <linux/module.h>
//#include "../../lge/include/lg_backup_items.h"
#include <mach/msm_smsm.h>


//#define MAX17047_FULL_DEBUG
//#define MAX17047_LIGHT_DEBUG
//#define MAX17047_CORE_PRINT

//#define MAX17047_FLASH_READ


#ifdef MAX17047_FULL_DEBUG
#define F_bat(fmt, args...) printk(KERN_INFO fmt, ##args)
#else
#define F_bat(fmt, args...) do {} while (0)
#endif

#ifdef MAX17047_LIGHT_DEBUG
#define L_bat(fmt, args...) printk(KERN_INFO fmt, ##args)
#else
#define L_bat(fmt, args...) do {} while (0)
#endif

#ifdef MAX17047_CORE_PRINT
#define C_bat(fmt, args...) printk(KERN_INFO fmt, ##args)
#else
#define C_bat(fmt, args...) do {} while (0)
#endif


static struct i2c_client *max17047_i2c_client;

u16 pre_soc=100;
u16 real_soc=100;

/** Extern variables **/
extern int lge_bd_rev;

struct max17047_chip {
	struct i2c_client *client;
	//struct power_supply battery;
	struct max17047_platform_data *pdata;
};

#ifdef MAX17047_FLASH_READ
#define BATTERY_INFO_DATA_SIZE 30
typedef struct MmcPartition MmcPartition;

struct MmcPartition {
    char *device_index;
    char *filesystem;
    char *name;
    unsigned dstatus;
    unsigned dtype ;
    unsigned dfirstsec;
    unsigned dsize;
};

extern int lge_mmc_scan_partitions(void);
extern int lge_erase_block(int secnum, size_t size);
extern int lge_write_block(int secnum, unsigned char *buf, size_t size);
extern int lge_read_block(int secnum, unsigned char *buf, size_t size);

extern int usb_cable_info;


extern const MmcPartition *lge_mmc_find_partition_by_name(const char *name);

//Access nand setting flag.
//1: POR state - need to restore battery info from nand.
//2: Non-POR state - need to save battery info from nand.
//3: Error state - need to execute once again.
//4: Fininsh work
//u8 access_nand_setting_flag = 0;

//u8 boot_check_count = 0;
//u8 battery_check_count = 0;

//u16 battery_cycle_count = 0;

//bool save_battery_info_flag = 0; //0 = Not yet. 1 = Already saved.

#endif
/* 121031 doosan.baek@lge.com Implement Power test SOC quickstart */
extern int lge_power_test_flag;
/* 121031 doosan.baek@lge.com Implement Power test SOC quickstart */

//static int max17047_access_control_of_flash(void);
//bool max17047_count_control(u8 select, u8 count_up);
//int max17047_save_bat_info_check(void);
bool max17047_write_temp(int battery_temp);
int max17047_read_battery_age(void);
bool max17047_battery_full_info_print(void);




#ifdef MAX17047_FLASH_READ
static int max17047_read_flash_all(void);
#endif


static int max17047_write_reg(struct i2c_client *client, u8 reg, u16 value)
{
	int ret = i2c_smbus_write_word_data(client, reg, value);

	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	return ret;
}

static int max17047_read_reg(struct i2c_client *client, u8 reg)
{
	int ret = i2c_smbus_read_word_data(client, reg);

	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	return ret;
}

static int max17047_multi_write_data(struct i2c_client *client, int reg, const u8 *values, int length)
{
		int ret;

		ret = i2c_smbus_write_i2c_block_data(client, reg, length, values);

		if (ret < 0)
				dev_err(&client->dev, "%s: err %d\n", __func__, ret);

		return ret;
}

static int max17047_multi_read_data(struct i2c_client *client, int reg, u8 *values, int length)
{
		int ret;

		ret = i2c_smbus_read_i2c_block_data(client, reg, length, values);

		if (ret < 0)
				dev_err(&client->dev, "%s: err %d\n", __func__, ret);

		return ret;
}


int max17047_get_mvolts(void)
{
	u16 read_reg;
	int vbatt_mv;

	//if (max17047_nobattery)
	//	return 3950;

	read_reg = max17047_read_reg(max17047_i2c_client, MAX17047_V_CELL);
	if (read_reg < 0)
		return 800;

	vbatt_mv = (read_reg >> 3);
	vbatt_mv = (vbatt_mv * 625) / 1000;

	printk("%s: vbatt = %d mV \n", __func__, vbatt_mv);

#if 0
	if (boot_check_count == 3){
		boot_check_count = 4; //Stop boot check count.
		max17047_access_control_of_flash();
		}
	else if (boot_check_count <3)
		boot_check_count++;


	if ( boot_check_count == 4){
	if (battery_check_count == 4){ //Check battery info save every 5 min.
		battery_check_count = 0; //Reset bat info save check count.
		max17047_save_bat_info_check();
		//max17047_battery_full_info_print();
		}
	else if (battery_check_count <4)
		battery_check_count++;
		}
#endif

	return vbatt_mv;

}

int max17047_suspend_get_mvolts(void)
{
	u16 read_reg;
	int vbatt_mv;

	//if (max17047_nobattery)
	//	return 3950;

	read_reg = max17047_read_reg(max17047_i2c_client, MAX17047_V_CELL);
	if (read_reg < 0)
		return 800;

	vbatt_mv = (read_reg >> 3);
	vbatt_mv = (vbatt_mv * 625) / 1000;

	printk("%s: vbatt = %d mV \n", __func__, vbatt_mv);

	return vbatt_mv;

}


int max17047_get_capacity_percent(void)
{
	int battery_soc = 0;
	int read_reg = 0;

	u8 upper_reg;
	u8 lower_reg;

	read_reg = max17047_read_reg(max17047_i2c_client, MAX17047_SOC_REP);

	if (read_reg < 0)
		return pre_soc;

	upper_reg = (read_reg & 0xFF00)>>8 ;
	lower_reg = (read_reg & 0xFF);

	F_bat("%s: read_reg = %X  upper_reg = %X lower_reg = %X \n", __func__, read_reg, upper_reg, lower_reg);

	/* cut off vol 3.55V : (soc - 2.76)*100/(95 - 2.76) */
	battery_soc = ((upper_reg *256)+lower_reg)*10/256;
	F_bat("%s: battery_soc  = %d\n", __func__, battery_soc);
		
	battery_soc = (battery_soc*100 - 2760) *100;
	battery_soc = (battery_soc  /9224) + 5 ; //round off 
	battery_soc /= 10 ; 
	printk("%s: battery_soc  = %d (upper_reg = %d lower_reg = %d )\n", __func__, battery_soc, upper_reg, lower_reg);
	
	real_soc = battery_soc;

	if (battery_soc >= 100) battery_soc = 100;

	if (battery_soc < 0) battery_soc = 0;


	pre_soc = battery_soc;


#ifdef MAX17047_FLASH_READ
	//if ( battery_check_count == 4)
		max17047_read_flash_all();
#endif

	return battery_soc;

}

int max17047_get_current(void)
{
	u16 read_reg;
	int ibatt_ma;
	int avg_ibatt_ma;
	u16 sign_bit;

	read_reg = max17047_read_reg(max17047_i2c_client, MAX17047_CURRENT);
	if (read_reg < 0)
		return 999; //Error Value return.

	sign_bit = (read_reg & 0x8000)>>15;

	if ( sign_bit == 1){
			ibatt_ma = ( 15625 * (read_reg  - 65536) )/100000;
		}
	else {
			ibatt_ma = (15625 * read_reg) /100000;
		}

	read_reg = max17047_read_reg(max17047_i2c_client, MAX17047_AVERAGE_CURRENT);
	if (read_reg < 0)
		return 999;//Error Value return.

	sign_bit = (read_reg & 0x8000)>>15;

	if ( sign_bit == 1){
			avg_ibatt_ma = ( 15625 * (read_reg  - 65536) )/100000;
		}
	else {
		avg_ibatt_ma = (15625 * read_reg ) /100000;
		}

	printk("%s: I_batt = %d mA avg_I_batt = %d mA \n", __func__, ibatt_ma, avg_ibatt_ma);

	return avg_ibatt_ma;

}

bool max17047_write_temp(int battery_temp)
{
	u16 ret;
	u16 write_temp;

	battery_temp = battery_temp/10;
	
	if(battery_temp < 0)
	{
		write_temp = (battery_temp + 256)<<8;
	}
	else
	{
		write_temp = 	(battery_temp)<<8;
	}

	//printk("max17047_write_temp write_temp = 0x%X ( %d C)", write_temp, battery_temp);
	printk("max17047_write_temp   - battery_temp (%d)\n", battery_temp);
	
	ret = max17047_write_reg(max17047_i2c_client, MAX17047_TEMPERATURE, write_temp);

	if (ret < 0){
		printk("max17047_write_temp error.");
		return 1;
		}

	return 0;
}

int max17047_read_battery_age(void)
{
	u16 read_reg;
	int battery_age;

	read_reg = max17047_read_reg(max17047_i2c_client, MAX17047_AGE);
	if (read_reg < 0)
		return 999; //Error Value return.

	battery_age = (read_reg >> 8);

	//Add the battery age report condition.
	//Do not report under 80% of age before the cycle count exceed 50 cycles.
	//if ((battery_cycle_count < 0x1F4) && (battery_age < 80))
	//	{
	//		printk("max17047_read_battery_age: battery age = %d cycles = %d \n", battery_age,  battery_cycle_count);
	//		battery_age = 80;
	//	}

	F_bat("%s: battery_age = %d\n", __func__, battery_age);

	return battery_age;
}

bool max17047_battery_full_info_print(void)
{
	u16 read_reg;
	int battery_age;
	int battery_remain_capacity;
	int battery_time_to_empty_sec;
	int battery_soc;
	int battery_voltage;
	int battery_temp;
	int battery_current;
	int battery_full_cap;

	battery_age = max17047_read_battery_age();

	read_reg = max17047_read_reg(max17047_i2c_client, MAX17047_REM_CAP_REP);

	battery_remain_capacity = (5 * read_reg)/10;

	read_reg = max17047_read_reg(max17047_i2c_client, MAX17047_FULL_CAP);

	battery_full_cap = (5 * read_reg)/10;

	read_reg = max17047_read_reg(max17047_i2c_client, MAX17047_TTE);

	battery_time_to_empty_sec = (5625 * read_reg )/1000;

	battery_soc = max17047_get_capacity_percent();

	battery_voltage = max17047_get_mvolts();

	read_reg = max17047_read_reg(max17047_i2c_client, MAX17047_TEMPERATURE);

	battery_temp = (read_reg >> 8);

	if ( battery_temp > 127)
		battery_temp = battery_temp - 256;
	else
		battery_temp = battery_temp;

	battery_current = max17047_get_current();

	printk("* -- max17047 battery full info print ---------- * \n");
	printk("battery age = %d %%    remain capacity = %d mAh  \n", battery_age, battery_remain_capacity);
	printk("battery current = %d mA     Time to Empty = %d min  \n", battery_current, battery_time_to_empty_sec/60);
	printk("battery SOC = %d %%    Voltage = %d mV  \n", battery_soc, battery_voltage);
	printk("battery TEMP = %d C   full capacity = %d mAh\n", battery_temp, battery_full_cap);
	printk("* ---------------------------------------------- * \n");

	return 0;
}


#if 0
static int max17047_get_property(struct power_supply *psy,
			    enum power_supply_property psp,
			    union power_supply_propval *val)
{
	struct max17047_chip *chip = container_of(psy,
				struct max17047_chip, battery);

	switch (psp) {
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = max17047_read_reg(chip->client,
				MAX17047_V_CELL) * 83; /* 1000 / 12 = 83 */
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_AVG:
		val->intval = max17047_read_reg(chip->client,
				MAX17047_AVERAGE_V_CELL) * 83;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = max17047_read_reg(chip->client,
				MAX17047_SOC_REP) / 256;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}
#endif

bool max17047_i2c_write_and_verify(u8 addr, u16 value)
{
	u16 read_reg;

	max17047_write_reg(max17047_i2c_client, addr, value);

	read_reg = max17047_read_reg(max17047_i2c_client, addr);

	if ( read_reg == value)
		{
			F_bat("[MAX17047] %s()  Addr = 0x%X, Value = 0x%X  Success \n", __func__, addr, value);
			return 1;
		}
	else
		{
			printk("[MAX17047] %s()  Addr = 0x%X, Value = 0x%X  Fail to write. Write once more. \n", __func__, addr, value);
			max17047_write_reg(max17047_i2c_client, addr, value);
			return 0;
		}

	return 1;

}


static int max17047_new_custom_model_write(void)
{
	//u16 ret;
	u16 read_reg;
	//u16 write_reg;
	u16 vfsoc;
	u16 full_cap_0;
	u16 rem_cap;
	u16 rep_cap;
	u16 dQ_acc;
	u16 capacity = 0x1802;

	u16 i;

	u8 custom_model_1[32] = {
		0x20, 0xAC, 0x40, 0xAD, 0xF0, 0xB1, 0xE0, 0xB8, 0x60, 0xBA, 0x90, 0xBB, 0x20, 0xBD, 0x00, 0xBE, 
		0xD0, 0xBE, 0xB0, 0xBF, 0x60, 0xC1, 0xD0, 0xC5, 0xE0, 0xCB, 0xC0, 0xD1, 0xC0, 0xD4, 0x20, 0xD7, 
		};

	u8 custom_model_2[32] = {
		0xB0, 0x00, 0x30, 0x06, 0x00, 0x04, 0x70, 0x18, 0x30, 0x0F, 0xB0, 0x17, 0x00, 0x18, 0x10, 0x22,
		0xA0, 0x0A, 0x90, 0x0D, 0xC0, 0x09, 0xF0, 0x07, 0xF0, 0x06, 0xA0, 0x09, 0xE0, 0x04, 0xE0, 0x04,
		};

	u8 custom_model_3[32] = {
		0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01,
		0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01,
		};

	u8 read_custom_model_1[32];
	u8 read_custom_model_2[32];
	u8 read_custom_model_3[32];

	F_bat("[MAX17047] %s()  Start \n", __func__);

	//0. Check for POR or Battery Insertion
	read_reg = max17047_read_reg(max17047_i2c_client, MAX17047_STATUS);
	F_bat("[MAX17047] %s()  MAX17047_STATUS = 0x%X \n", __func__, read_reg);
	if (read_reg == 0){
		printk("[MAX17047] IC Non-Power-On-Reset state. Go to max17047_save_bat_info_to_flash. \n");
		return 2; //Go to save the custom model.
		}
	else
		printk("[MAX17047] IC Power On Reset state. Start Custom Model Write. \n");

#if 0
	//Skip if the cable is Factory Mode.
	if ((usb_cable_info == 6) || (usb_cable_info == 7) || (usb_cable_info == 11))
	{
		printk("[MAX17047] IC Power-On-Reset state. But Skip Custom model in Factory Mode. \n");
		return 2;
	}
#endif
	//1. Delay 500mS
	msleep(500);

	//1.1 Version Check
	read_reg = max17047_read_reg(max17047_i2c_client, MAX17047_VERSION);
	F_bat("[MAX17047] %s()  MAX17047_VERSION = 0x%X \n", __func__, read_reg);
	if (read_reg != 0xAC)
		{
			printk("[MAX17047]  Version Check Error. \n");
			return 1; //Version Check Error
		}

	//2. Initialize Configuration
	max17047_write_reg(max17047_i2c_client, MAX17047_RELAX_CFG, 0x506B);
	max17047_write_reg(max17047_i2c_client, MAX17047_CONFIG, 0x2100);
	max17047_write_reg(max17047_i2c_client, MAX17047_FILTER_CFG, 0x87A4);
	max17047_write_reg(max17047_i2c_client, MAX17047_LEARN_CFG, 0x2607);
	max17047_write_reg(max17047_i2c_client, MAX17047_MISC_CFG, 0x0870);
	max17047_write_reg(max17047_i2c_client, MAX17047_FULL_SOC_THR, 0x5F00);

	//4. Unlock Model Access
	max17047_write_reg(max17047_i2c_client, 0x62, 0x0059);
	max17047_write_reg(max17047_i2c_client, 0x63, 0x00C4);

	//5. Write/Read/Verify the Custom Model
	max17047_multi_write_data(max17047_i2c_client, 0x80, &custom_model_1[0], 32);
	max17047_multi_write_data(max17047_i2c_client, 0x90, &custom_model_2[0], 32);
	max17047_multi_write_data(max17047_i2c_client, 0xA0, &custom_model_3[0], 32);

	//For test only. Read back written-custom model data.
	max17047_multi_read_data(max17047_i2c_client, 0x80, &read_custom_model_1[0], 32);
	max17047_multi_read_data(max17047_i2c_client, 0x90, &read_custom_model_2[0], 32);
	max17047_multi_read_data(max17047_i2c_client, 0xA0, &read_custom_model_3[0], 32);

	//Compare with original one.
	for ( i =0 ; i<32 ; i++)
		{
			if ( read_custom_model_1[i] != custom_model_1[i])
				printk("[MAX17047] Custom Model 1[%d] Write Error \n", i);
		}

	for ( i =0 ; i<32 ; i++)
		{
			if ( read_custom_model_2[i] != custom_model_2[i])
				printk("[MAX17047] Custom Model 2[%d] Write Error \n", i);
		}

	for ( i =0 ; i<32 ; i++)
		{
			if ( read_custom_model_3[i] != custom_model_3[i])
				printk("[MAX17047] Custom Model 3[%d] Write Error \n", i);
		}
	//For Test only end.

	//8. Lock Model Access
	max17047_write_reg(max17047_i2c_client, 0x62, 0x0000);
	max17047_write_reg(max17047_i2c_client, 0x63, 0x0000);

	//9. Verify the Model Access is locked.
	//Skip.

	//10. Write Custom Parameters
	max17047_i2c_write_and_verify(MAX17047_RCOMP_0, 0x0066);
	max17047_i2c_write_and_verify(MAX17047_TEMP_CO, 0x2436);
	max17047_i2c_write_and_verify(MAX17047_TEMP_NOM, 0x1400);
	max17047_write_reg(max17047_i2c_client, MAX17047_T_GAIN, 0xE932);
	max17047_write_reg(max17047_i2c_client, MAX17047_T_OFF, 0x2381);

	max17047_write_reg(max17047_i2c_client, MAX17047_I_CHG_TERM, 0x03C0);//Termination Current 150mA
	max17047_i2c_write_and_verify(MAX17047_V_EMPTY, 0xACDA);
	max17047_i2c_write_and_verify(MAX17047_Q_RESIDUAL_00, 0x8400);
	max17047_i2c_write_and_verify(MAX17047_Q_RESIDUAL_10, 0x5A84);
	max17047_i2c_write_and_verify(MAX17047_Q_RESIDUAL_20, 0x270A);
	max17047_i2c_write_and_verify(MAX17047_Q_RESIDUAL_30, 0x198A);

	//11. Update Full Capacity Parameters
	max17047_i2c_write_and_verify(MAX17047_FULL_CAP, capacity);
	max17047_write_reg(max17047_i2c_client, MAX17047_DESIGN_CAP, capacity);
	max17047_i2c_write_and_verify(MAX17047_FULL_CAP_NOM, capacity);

	//13. Delay at least 350mS
	msleep(360);

	//14. Write VFSOC value to VFSOC 0
	vfsoc = max17047_read_reg(max17047_i2c_client, MAX17047_SOC_VF);
	F_bat("[MAX17047] %s()  vfsoc = 0x%X \n", __func__, vfsoc);
	max17047_write_reg(max17047_i2c_client, 0x60, 0x0080);
	max17047_i2c_write_and_verify(0x48, vfsoc);
	max17047_write_reg(max17047_i2c_client, 0x60, 0x0000);

	//15. Write temperature (default 20 deg C)
	max17047_i2c_write_and_verify(MAX17047_TEMPERATURE, 0x1400);

	//16. Load New Capacity Parameters
	full_cap_0 =  max17047_read_reg(max17047_i2c_client, 0x35);
	F_bat("[MAX17047] %s()  full_cap_0 = %d  = 0x%X \n", __func__, full_cap_0, full_cap_0);
	rem_cap = (vfsoc * full_cap_0) / 25600;
	F_bat("[MAX17047] %s()  rem_cap = %d  = 0x%X \n", __func__, rem_cap, rem_cap);
	max17047_i2c_write_and_verify(MAX17047_REM_CAP_MIX, rem_cap);
	rep_cap = rem_cap;
	max17047_i2c_write_and_verify(MAX17047_REM_CAP_REP, rep_cap);
	dQ_acc =( capacity / 4);
	max17047_i2c_write_and_verify(MAX17047_D_QACC, dQ_acc);
	max17047_i2c_write_and_verify(MAX17047_D_PACC, 0x3200);
	max17047_i2c_write_and_verify(MAX17047_FULL_CAP, capacity);
	max17047_write_reg(max17047_i2c_client, MAX17047_DESIGN_CAP, capacity);
	max17047_i2c_write_and_verify(MAX17047_FULL_CAP_NOM, capacity);
	max17047_write_reg(max17047_i2c_client, MAX17047_SOC_REP, vfsoc);

	//17. Initialization Complete
	read_reg = max17047_read_reg(max17047_i2c_client, MAX17047_STATUS);
	max17047_i2c_write_and_verify(MAX17047_STATUS, (read_reg & 0xFFFD));

	//End of the Custom Model step 1.
	printk("[MAX17047] End of the max17047_new_custom_model_write. \n");

	F_bat("[MAX17047] %s()  End \n", __func__);
	return 0; //Success to write.

}

static int max17047_force_custom_model_write(void)
{
	//u16 ret;
	u16 read_reg;
	//u16 write_reg;
	u16 vfsoc;
	u16 full_cap_0;
	u16 rem_cap;
	u16 rep_cap;
	u16 dQ_acc;
	u16 capacity = 0x1770;

	u16 i;

	u8 custom_model_1[32] = {
		0x30, 0x7F, 0x10, 0xB6, 0xD0, 0xB7, 0xA0, 0xB9, 0xC0, 0xBB, 0xA0, 0xBC, 0xE0, 0xBC, 0x30, 0xBD,
		0x90, 0xBD, 0xD0, 0xBE, 0xE0, 0xC1, 0xF0, 0xC3, 0x10, 0xC6, 0x30, 0xC9, 0x60, 0xCC, 0x90, 0xCF,
		};

	u8 custom_model_2[32] = {
		0xE0, 0x00, 0x70, 0x0E, 0xF0, 0x0D, 0x00, 0x0F, 0x20, 0x15, 0xF0, 0x46, 0xC0, 0x38, 0x70, 0x19,
		0x00, 0x18, 0x60, 0x0C, 0x20, 0x0D, 0xC0, 0x0C, 0xE0, 0x07, 0xF0, 0x08, 0xF0, 0x08, 0xF0, 0x08,
		};

	u8 custom_model_3[32] = {
		0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01,
		0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01,
		};

	u8 read_custom_model_1[32];
	u8 read_custom_model_2[32];
	u8 read_custom_model_3[32];

	F_bat("[MAX17047] %s()  Start \n", __func__);

	//0. No Check for POR or Battery Insertion
	printk("[MAX17047]Start Force Custom Model Write. \n");

#if 0
	//Skip if the cable is Factory Mode.
	if ((usb_cable_info == 6) || (usb_cable_info == 7) || (usb_cable_info == 11))
	{
		printk("[MAX17047] Force Custom Write. But Skip Custom model in Factory Mode. \n");
		return 2;
	}
#endif
	//1. Delay 500mS
	msleep(500);

	//1.1 Version Check
	read_reg = max17047_read_reg(max17047_i2c_client, MAX17047_VERSION);
	F_bat("[MAX17047] %s()  MAX17047_VERSION = 0x%X \n", __func__, read_reg);
	if (read_reg != 0xAC)
		{
			printk("[MAX17047]  Version Check Error. \n");
			return 1; //Version Check Error
		}

	//2. Initialize Configuration
	max17047_write_reg(max17047_i2c_client, MAX17047_RELAX_CFG, 0x506B);
	max17047_write_reg(max17047_i2c_client, MAX17047_CONFIG, 0x2100);
	max17047_write_reg(max17047_i2c_client, MAX17047_FILTER_CFG, 0x87A4);
	max17047_write_reg(max17047_i2c_client, MAX17047_LEARN_CFG, 0x2607);
	max17047_write_reg(max17047_i2c_client, MAX17047_MISC_CFG, 0x0870);
	max17047_write_reg(max17047_i2c_client, MAX17047_FULL_SOC_THR, 0x5F00);

	//4. Unlock Model Access
	max17047_write_reg(max17047_i2c_client, 0x62, 0x0059);
	max17047_write_reg(max17047_i2c_client, 0x63, 0x00C4);

	//5. Write/Read/Verify the Custom Model
	max17047_multi_write_data(max17047_i2c_client, 0x80, &custom_model_1[0], 32);
	max17047_multi_write_data(max17047_i2c_client, 0x90, &custom_model_2[0], 32);
	max17047_multi_write_data(max17047_i2c_client, 0xA0, &custom_model_3[0], 32);

	//For test only. Read back written-custom model data.
	max17047_multi_read_data(max17047_i2c_client, 0x80, &read_custom_model_1[0], 32);
	max17047_multi_read_data(max17047_i2c_client, 0x90, &read_custom_model_2[0], 32);
	max17047_multi_read_data(max17047_i2c_client, 0xA0, &read_custom_model_3[0], 32);

	//Compare with original one.
	for ( i =0 ; i<32 ; i++)
		{
			if ( read_custom_model_1[i] != custom_model_1[i])
				printk("[MAX17047] Custom Model 1[%d] Write Error \n", i);
		}

	for ( i =0 ; i<32 ; i++)
		{
			if ( read_custom_model_2[i] != custom_model_2[i])
				printk("[MAX17047] Custom Model 2[%d] Write Error \n", i);
		}

	for ( i =0 ; i<32 ; i++)
		{
			if ( read_custom_model_3[i] != custom_model_3[i])
				printk("[MAX17047] Custom Model 3[%d] Write Error \n", i);
		}
	//For Test only end.

	//8. Lock Model Access
	max17047_write_reg(max17047_i2c_client, 0x62, 0x0000);
	max17047_write_reg(max17047_i2c_client, 0x63, 0x0000);

	//9. Verify the Model Access is locked.
	//Skip.

	//10. Write Custom Parameters
	max17047_i2c_write_and_verify(MAX17047_RCOMP_0, 0x0058);
	max17047_i2c_write_and_verify(MAX17047_TEMP_CO, 0x2230);
	max17047_i2c_write_and_verify(MAX17047_TEMP_NOM, 0x1400);
	max17047_write_reg(max17047_i2c_client, MAX17047_T_GAIN, 0xE932);
	max17047_write_reg(max17047_i2c_client, MAX17047_T_OFF, 0x2381);

	max17047_write_reg(max17047_i2c_client, MAX17047_I_CHG_TERM, 0x03C0);//Termination Current 150mA
	max17047_i2c_write_and_verify(MAX17047_V_EMPTY, 0xACDA);
	max17047_i2c_write_and_verify(MAX17047_Q_RESIDUAL_00, 0x9680);
	max17047_i2c_write_and_verify(MAX17047_Q_RESIDUAL_10, 0x3800);
	max17047_i2c_write_and_verify(MAX17047_Q_RESIDUAL_20, 0x3805);
	max17047_i2c_write_and_verify(MAX17047_Q_RESIDUAL_30, 0x2513);

	//11. Update Full Capacity Parameters
	max17047_i2c_write_and_verify(MAX17047_FULL_CAP, capacity);
	max17047_write_reg(max17047_i2c_client, MAX17047_DESIGN_CAP, capacity);
	max17047_i2c_write_and_verify(MAX17047_FULL_CAP_NOM, capacity);

	//13. Delay at least 350mS
	msleep(360);

	//14. Write VFSOC value to VFSOC 0
	vfsoc = max17047_read_reg(max17047_i2c_client, MAX17047_SOC_VF);
	F_bat("[MAX17047] %s()  vfsoc = 0x%X \n", __func__, vfsoc);
	max17047_write_reg(max17047_i2c_client, 0x60, 0x0080);
	max17047_i2c_write_and_verify(0x48, vfsoc);
	max17047_write_reg(max17047_i2c_client, 0x60, 0x0000);

	//15. Write temperature (default 20 deg C)
	max17047_i2c_write_and_verify(MAX17047_TEMPERATURE, 0x1400);

	//16. Load New Capacity Parameters
	full_cap_0 =  max17047_read_reg(max17047_i2c_client, 0x35);
	F_bat("[MAX17047] %s()  full_cap_0 = %d  = 0x%X \n", __func__, full_cap_0, full_cap_0);
	rem_cap = (vfsoc * full_cap_0) / 25600;
	F_bat("[MAX17047] %s()  rem_cap = %d  = 0x%X \n", __func__, rem_cap, rem_cap);
	max17047_i2c_write_and_verify(MAX17047_REM_CAP_MIX, rem_cap);
	rep_cap = rem_cap;
	max17047_i2c_write_and_verify(MAX17047_REM_CAP_REP, rep_cap);
	dQ_acc =( capacity / 4);
	max17047_i2c_write_and_verify(MAX17047_D_QACC, dQ_acc);
	max17047_i2c_write_and_verify(MAX17047_D_PACC, 0x3200);
	max17047_i2c_write_and_verify(MAX17047_FULL_CAP, capacity);
	max17047_write_reg(max17047_i2c_client, MAX17047_DESIGN_CAP, capacity);
	max17047_i2c_write_and_verify(MAX17047_FULL_CAP_NOM, capacity);
	max17047_write_reg(max17047_i2c_client, MAX17047_SOC_REP, vfsoc);

	//17. Initialization Complete
	read_reg = max17047_read_reg(max17047_i2c_client, MAX17047_STATUS);
	max17047_i2c_write_and_verify(MAX17047_STATUS, (read_reg & 0xFFFD));

	//End of the Custom Model step 1.
	printk("[MAX17047] End of the max17047_new_custom_model_write. \n");

	F_bat("[MAX17047] %s()  End \n", __func__);
	return 0; //Success to write.

}

#if 0

static int max17047_save_bat_info_to_flash(void)
{
	int ret;
	u16 read_reg;

	u8 i =0;

	//List of saved battery information.
	u16 saved_RCOMP0;
	u16 saved_TempCo;
	u16 saved_FullCap;
	u16 saved_cycles;
	u16 saved_FullCapNom;
	u16 saved_Iavg_empty;
	u16 QRTable00;
	u16 QRTable10;
	u16 QRTable20;
	u16 QRTable30;

	//Variables for Direct access to flash memory
	unsigned char pbuf[30]; //no need to have huge size, this is only for the flag
	const MmcPartition *pMisc_part;
	int mtd_op_result = 0;
	unsigned long bat_info_bytes_pos_in_emmc = 0;

	unsigned char *battery_info_save_string;

	battery_info_save_string = kmalloc(30, GFP_KERNEL);

	// allocation exception handling
	if(!battery_info_save_string)
	{
		printk(KERN_ERR "battery_info_save_string allocation failed, return\n");
		return 2; //Go to custom model write again.
	}

	ret = lge_mmc_scan_partitions();
	if ( ret < 0){
		printk(KERN_ERR "[MAX17047] lge_mmc_scan_partitions scan failed, return\n");
		return 2; //Go to custom model write again.
		}
	pMisc_part = lge_mmc_find_partition_by_name("misc");
	if ( pMisc_part == NULL){
		printk(KERN_ERR "[MAX17047] lge_mmc_find_partition_by_name failed, return\n");
		return 2; //Go to custom model write again.
		}

	bat_info_bytes_pos_in_emmc = (pMisc_part->dfirstsec*512)+PTN_BAT_INFO_PERSIST_POSITION_IN_MISC_PARTITION;

	printk("Saved Battery Info in mmc info sec : 0x%x, size : 0x%x type : 0x%x frst sec: 0x%lx\n", \
	        pMisc_part->dfirstsec, pMisc_part->dsize, pMisc_part->dtype, bat_info_bytes_pos_in_emmc);

	//Save the learned Parameters of the battery information from flash memory.

	//17.5 Check for MAX17047 Reset
	read_reg = max17047_read_reg(max17047_i2c_client, MAX17047_STATUS);
	F_bat("[MAX17047] %s()  MAX17047_STATUS = 0x%X \n", __func__, read_reg);
	if (read_reg == 1)
		return 2; //Go to custom model write again.

	printk("[MAX17047] Start max17047_save_bat_info_to_flash \n");

	// Read the data from IC register.
	saved_RCOMP0 = max17047_read_reg(max17047_i2c_client, MAX17047_RCOMP_0);
	saved_TempCo = max17047_read_reg(max17047_i2c_client, MAX17047_TEMP_CO);
	saved_FullCap = max17047_read_reg(max17047_i2c_client, MAX17047_FULL_CAP);
	saved_cycles = max17047_read_reg(max17047_i2c_client, MAX17047_CYCLES);
	if (saved_cycles > 0xFFFE){
		saved_cycles = 0x0; //Max cycles is 65535%. If the cycles is max, reset to 0.
		max17047_count_control(0,1);//Battery cycles count up
		}

	battery_cycle_count = saved_cycles;

	saved_FullCapNom = max17047_read_reg(max17047_i2c_client, MAX17047_FULL_CAP_NOM);
	saved_Iavg_empty = max17047_read_reg(max17047_i2c_client, MAX17047_I_AVG_EMPTY);
	QRTable00 = max17047_read_reg(max17047_i2c_client, MAX17047_Q_RESIDUAL_00);
	QRTable10 = max17047_read_reg(max17047_i2c_client, MAX17047_Q_RESIDUAL_10);
	QRTable20 = max17047_read_reg(max17047_i2c_client, MAX17047_Q_RESIDUAL_20);
	QRTable30 = max17047_read_reg(max17047_i2c_client, MAX17047_Q_RESIDUAL_30);

	printk("[MAX17047] %s()  read_battery_cycles = %d  \n", __func__, saved_cycles);

	pbuf[0] = (saved_RCOMP0 & 0xFF) ;
	pbuf[1] = (saved_RCOMP0 & 0xFF00)>>8 ;

	pbuf[2] = (saved_TempCo & 0xFF) ;
	pbuf[3] = (saved_TempCo & 0xFF00)>>8 ;

	pbuf[4] = (saved_FullCap & 0xFF) ;
	pbuf[5] = (saved_FullCap & 0xFF00)>>8 ;

	pbuf[6] = (saved_cycles & 0xFF) ;
	pbuf[7] = (saved_cycles & 0xFF00)>>8 ;

	pbuf[8] = (saved_FullCapNom & 0xFF) ;
	pbuf[9] = (saved_FullCapNom & 0xFF00)>>8 ;

	pbuf[10] = (saved_Iavg_empty & 0xFF) ;
	pbuf[11] = (saved_Iavg_empty & 0xFF00)>>8 ;

	pbuf[12] = (QRTable00 & 0xFF) ;
	pbuf[13] = (QRTable00 & 0xFF00)>>8 ;

	pbuf[14] = (QRTable10 & 0xFF) ;
	pbuf[15] = (QRTable10 & 0xFF00)>>8 ;

	pbuf[16] = (QRTable20 & 0xFF) ;
	pbuf[17] = (QRTable20 & 0xFF00)>>8 ;

	pbuf[18] = (QRTable30 & 0xFF) ;
	pbuf[19] = (QRTable30 & 0xFF00)>>8 ;

	for(i=0; i<20 ; i+=2)
		printk("[MAX17047] pbuf[%d] = %X  pbuf[%d] = %X \n", i, pbuf[i], i+1, pbuf[i+1]);

	battery_info_save_string = pbuf;

	// Save the data to emmc.
            mtd_op_result = lge_erase_block(bat_info_bytes_pos_in_emmc, BATTERY_INFO_DATA_SIZE);
            if(mtd_op_result != (BATTERY_INFO_DATA_SIZE))
            {
                printk(KERN_ERR "[MAX17047]lge_erase_block, error num = %d \n", mtd_op_result);
		  return 2; //Go to custom model write again.
            }
            else
            {
                mtd_op_result = lge_write_block(bat_info_bytes_pos_in_emmc, battery_info_save_string, BATTERY_INFO_DATA_SIZE);
                if(mtd_op_result!=(BATTERY_INFO_DATA_SIZE))
                {
                    printk(KERN_ERR "[MAX17047]lge_write_block, error num = %d \n", mtd_op_result);
		      return 2; //Go to custom model write again.
                }
		else
			printk("[MAX17047] SUCCESS - Battery Info Write to flash\n");	
            }

	return 0;

}


int max17047_save_bat_info_check(void)
{
	u16 read_cycles;
	u16 cycles_data;
	u8 ret = 0 ;

	read_cycles = max17047_read_reg(max17047_i2c_client, MAX17047_CYCLES);
	printk("[MAX17047] save_bat_info_check flag = %d cycles = %X \n", save_battery_info_flag, read_cycles);	
	cycles_data = (read_cycles & 0x7F); //Check under bit 7 of cycles reg bit.
	//Per 64%, save the battery info to flash memeory once.
	if (((cycles_data > 0x3F) && (cycles_data < 0x45)) || ((cycles_data > 0x0) && (cycles_data < 0x6))){ 
		if ( save_battery_info_flag == 1)
			return 0;

		ret = max17047_save_bat_info_to_flash(); //Non-POR state. Go to max17047_save_bat_info_to_flash.
		if ( ret == 0){
			save_battery_info_flag = 1;
			return ret;
		}
		else
			return ret;
	}
	else{
		if (save_battery_info_flag == 1)
			save_battery_info_flag = 0;
	}

	return 0;
}


static int max17047_restore_bat_info_from_flash(void)
{
	u16 read_reg;

	int ret;

	u8 i =0;

	//List of saved battery information.
	u16 saved_RCOMP0;
	u16 saved_TempCo;
	u16 saved_FullCap;
	u16 full_cap0;
	u16 rem_cap;
	u16 saved_cycles;
	u16 saved_FullCapNom;
	u16 saved_Iavg_empty;
	u16 QRTable00;
	u16 QRTable10;
	u16 QRTable20;
	u16 QRTable30;
	u16 dQ_acc;

	//Variables for Direct access to flash memory
	unsigned char pbuf[30]; //no need to have huge size, this is only for the flag
	const MmcPartition *pMisc_part;
	int mtd_op_result = 0;
	unsigned long bat_info_bytes_pos_in_emmc = 0;

	ret = lge_mmc_scan_partitions();
	if ( ret < 0){
		printk(KERN_ERR "[MAX17047] lge_mmc_scan_partitions scan failed, return\n");
		return 2; //Go to custom model write again.
	}
	pMisc_part = lge_mmc_find_partition_by_name("misc");
	if ( pMisc_part == NULL){
		printk(KERN_ERR "[MAX17047] lge_mmc_find_partition_by_name failed, return\n");
		return 2; //Go to custom model write again.
		}
	bat_info_bytes_pos_in_emmc = (pMisc_part->dfirstsec*512)+PTN_BAT_INFO_PERSIST_POSITION_IN_MISC_PARTITION;

	printk("Restore Battery Info from mmc info sec : 0x%x, size : 0x%x type : 0x%x frst sec: 0x%lx\n", \
	        pMisc_part->dfirstsec, pMisc_part->dsize, pMisc_part->dtype, bat_info_bytes_pos_in_emmc);

	//Restore the learned Parameters of the battery information from flash memory.

	//17.5 Check for MAX17047 Reset
	read_reg = max17047_read_reg(max17047_i2c_client, MAX17047_STATUS);
	F_bat("[MAX17047] %s()  MAX17047_STATUS = 0x%X \n", __func__, read_reg);
	if (read_reg == 1)
		return 2; //Go to custom model write again.

	printk("[MAX17047] Start max17047_restore_bat_info_from_flash\n");

	// Read the data from emmc.
	     memset((void*)pbuf, 0, BATTERY_INFO_DATA_SIZE);
            mtd_op_result = lge_read_block(bat_info_bytes_pos_in_emmc, pbuf, BATTERY_INFO_DATA_SIZE);
            if( mtd_op_result != (BATTERY_INFO_DATA_SIZE) )
            {
                 printk(KERN_ERR "[MAX17047]lge_read_block, error num = %d \n", mtd_op_result);
            }
            else
            {
			printk("[MAX17047] Battery Info Read from flash\n");
			for(i=0; i<20 ; i+=2)
				printk("[MAX17047]	pbuf[%d] = %X  pbuf[%d] = %X \n", i, pbuf[i], i+1, pbuf[i+1]);
            }

	saved_RCOMP0 = (pbuf[1]<<8) | pbuf[0];
	printk("[MAX17047]	saved_RCOMP0 = 0x%X  pbuf[0] = %X  pbuf[1] = %X\n", saved_RCOMP0,pbuf[0], pbuf[1]);

	saved_TempCo = (pbuf[3]<<8) | pbuf[2];
	saved_FullCap = (pbuf[5]<<8) | pbuf[4];
	saved_cycles = (pbuf[7]<<8) | pbuf[6];
	printk("[MAX17047]	saved_cycles = 0x%X  pbuf[6] = %X  pbuf[7] = %X\n", saved_cycles,pbuf[6], pbuf[7]);

	battery_cycle_count = saved_cycles;

	saved_FullCapNom = (pbuf[9]<<8) | pbuf[8];
	saved_Iavg_empty = (pbuf[11]<<8) | pbuf[10];
	QRTable00 = (pbuf[13]<<8) | pbuf[12];
	QRTable10 = (pbuf[15]<<8) | pbuf[14];
	QRTable20 = (pbuf[17]<<8) | pbuf[16];
	QRTable30 = (pbuf[19]<<8) | pbuf[18];

	//If there is no saved data, save battery info to nand first.
	if (((saved_RCOMP0 == 0xFFFF) && (saved_cycles == 0xFFFF)) ||( (saved_RCOMP0 == 0x0) && (saved_cycles == 0x0)))  {
		printk("[MAX17047] POR & No battery data in nand flash. Save battery info to nand first. \n");
		ret = max17047_save_bat_info_to_flash(); //Non-POR state. Go to max17047_save_bat_info_to_flash.
		if ( ret == 0){
			save_battery_info_flag = 1;
			return ret;
		}
		else
			return ret;
		}

	//21. Restore Learned Parameters.
	max17047_i2c_write_and_verify(MAX17047_RCOMP_0, saved_RCOMP0);
	max17047_i2c_write_and_verify(MAX17047_TEMP_CO, saved_TempCo);
	max17047_i2c_write_and_verify(MAX17047_I_AVG_EMPTY, saved_Iavg_empty);
	max17047_i2c_write_and_verify(MAX17047_FULL_CAP_NOM, saved_FullCapNom);
	max17047_i2c_write_and_verify(MAX17047_Q_RESIDUAL_00, QRTable00);
	max17047_i2c_write_and_verify(MAX17047_Q_RESIDUAL_10, QRTable10);
	max17047_i2c_write_and_verify(MAX17047_Q_RESIDUAL_20, QRTable20);
	max17047_i2c_write_and_verify(MAX17047_Q_RESIDUAL_30, QRTable30);

	//22. Wait 350 mS
	msleep(400);

	//23. Restore Full Capacity.
	full_cap0 = max17047_read_reg(max17047_i2c_client, 0x35);
	read_reg = max17047_read_reg(max17047_i2c_client, MAX17047_SOC_MIX);
	rem_cap = ( read_reg * full_cap0)/25600;
	max17047_i2c_write_and_verify(MAX17047_REM_CAP_MIX, rem_cap);
	max17047_i2c_write_and_verify(MAX17047_FULL_CAP, saved_FullCap);

	dQ_acc = (saved_FullCapNom / 4);
	max17047_i2c_write_and_verify(MAX17047_D_QACC, dQ_acc);
	max17047_i2c_write_and_verify(MAX17047_D_PACC, 0x3200);

	//24. Wait 350 mS
	msleep(360);

	//25. Restore Cycles.
	max17047_i2c_write_and_verify(MAX17047_CYCLES, saved_cycles);
	printk("[MAX17047] saved_battery_cycles = %d \n", saved_cycles);

	printk("[MAX17047] End max17047_restore_bat_info_from_flash\n");

	// 26. POR count up
	max17047_count_control(1,1);

	return 0;

}

#endif

#ifdef MAX17047_FLASH_READ
static int max17047_read_flash_all(void)
{
	u8 i =0;
	int ret;

	//Variables for Direct access to flash memory
	unsigned char pbuf[40]; //no need to have huge size, this is only for the flag
	const MmcPartition *pMisc_part;
	int mtd_op_result = 0;
	unsigned long bat_info_bytes_pos_in_emmc = 0;

	ret = lge_mmc_scan_partitions();
	if ( ret < 0){
		printk(KERN_ERR "[MAX17047] lge_mmc_scan_partitions scan failed, return\n");
		return 0;
	}
	pMisc_part = lge_mmc_find_partition_by_name("misc");
	if ( pMisc_part == NULL){
		printk(KERN_ERR "[MAX17047] lge_mmc_find_partition_by_name failed, return\n");
		return 0;
		}
	bat_info_bytes_pos_in_emmc = (pMisc_part->dfirstsec*512)+PTN_BAT_INFO_PERSIST_POSITION_IN_MISC_PARTITION;

	printk("Read Battery Info from mmc info sec : 0x%x, size : 0x%x type : 0x%x frst sec: 0x%lx\n", \
	        pMisc_part->dfirstsec, pMisc_part->dsize, pMisc_part->dtype, bat_info_bytes_pos_in_emmc);

	// Read the data from emmc.
	     memset((void*)pbuf, 0, BATTERY_INFO_DATA_SIZE+10);
            mtd_op_result = lge_read_block(bat_info_bytes_pos_in_emmc, pbuf, BATTERY_INFO_DATA_SIZE+10);
            if( mtd_op_result != (BATTERY_INFO_DATA_SIZE+10) )
            {
                 printk(KERN_ERR "[MAX17047]lge_read_block, error num = %d \n", mtd_op_result);
            }
            else
            {
			printk("[MAX17047] Battery Info Read from flash\n");
			for(i=0; i<40 ; i+=2)
				printk("[MAX17047]  pbuf[%d] = %X  pbuf[%d] = %X \n", i, pbuf[i], i+1, pbuf[i+1]);
            }
	return 0;

}
#endif

int max17047_battery_exchange_program(void)
{
	int ret = 0;

//	if(lge_bd_rev > LGE_REV_C) kwon temporary
	{
		//Call max17047_new_custom_model_write
		ret = max17047_force_custom_model_write();

		if ( ret == 2)
			printk("NON-POWER_ON reset. Proceed Booting. \n");
		else if ( ret == 1)
			max17047_force_custom_model_write(); //Error occurred. Write once more.
		else if ( ret == 0)
			printk("POWER_ON reset. Custom Model Success. \n");//New Custom model write End. Go to max17047_restore_bat_info_from_flash.
	}

return 0;

}

#if 0
//Battery Exchange Progrom
//Reset the stored battery information.
int max17047_battery_exchange_program(void)
{
	int ret;

	u8 i =0;

	//Variables for Direct access to flash memory
	unsigned char pbuf[20]; //no need to have huge size, this is only for the flag
	const MmcPartition *pMisc_part;
	int mtd_op_result = 0;
	unsigned long bat_info_bytes_pos_in_emmc = 0;

	unsigned char *battery_info_save_string;

	battery_info_save_string = kmalloc(30, GFP_KERNEL);

	// allocation exception handling
	if(!battery_info_save_string)
	{
		printk(KERN_ERR "battery_info_save_string allocation failed, return\n");
		return 2; //Go to custom model write again.
	}

	ret =lge_mmc_scan_partitions();
	if ( ret < 0){
		printk(KERN_ERR "[MAX17047] lge_mmc_scan_partitions scan failed, return\n");
		return 2; //Go to custom model write again.
		}
	pMisc_part = lge_mmc_find_partition_by_name("misc");
	if ( pMisc_part == NULL){
		printk(KERN_ERR "[MAX17047] lge_mmc_find_partition_by_name failed, return\n");
		return 2; //Go to custom model write again.
		}
	bat_info_bytes_pos_in_emmc = (pMisc_part->dfirstsec*512)+PTN_BAT_INFO_PERSIST_POSITION_IN_MISC_PARTITION;

	printk("[MAX17047] Start Battery Exchange program \n");

	pbuf[0] = 0x0 ;
	pbuf[1] = 0x0 ;

	pbuf[6] = 0x0 ;
	pbuf[7] = 0x0 ;

	for(i=0; i<20 ; i+=2)
		printk("[MAX17047] pbuf[%d] = %X  pbuf[%d] = %X \n", i, pbuf[i], i+1, pbuf[i+1]);

	battery_info_save_string = pbuf;

	// Save the data to emmc.
            mtd_op_result = lge_erase_block(bat_info_bytes_pos_in_emmc, BATTERY_INFO_DATA_SIZE-10);
            if(mtd_op_result != (BATTERY_INFO_DATA_SIZE-10))
            {
                printk(KERN_ERR "[MAX17047]lge_erase_block, error num = %d \n", mtd_op_result);
            }
            else
            {
                mtd_op_result = lge_write_block(bat_info_bytes_pos_in_emmc, battery_info_save_string, BATTERY_INFO_DATA_SIZE-10);
                if(mtd_op_result!=(BATTERY_INFO_DATA_SIZE-10))
                {
                    printk(KERN_ERR "[MAX17047]lge_write_block, error num = %d \n", mtd_op_result);
                }
		else
			printk("[MAX17047] SUCCESS - Battery Exchange program\n");	

		battery_check_count = 0; //Reset bat info save check count.
		save_battery_info_flag = 1;

		// Exchange count up
		max17047_count_control(2,1);

		#ifdef MAX17047_FLASH_READ
		max17047_read_flash_all(); //Test only
		#endif
            }

	return 0;
}


//Battery cycle & POR count control function.
// Select 0 : Battery Cycle up
// Select 1 : Battery POR count up
// Select 2 : Battrey Exchange Program call count up
// Select 3 : Read Cycle & POR count data from flash (Count_up = 0)
bool max17047_count_control(u8 select, u8 count_up)
{
	u8 i =0;
	int ret;

	//Variables for Direct access to flash memory
	unsigned char pbuf[10]; //no need to have huge size, this is only for the flag
	const MmcPartition *pMisc_part;
	int mtd_op_result = 0;
	unsigned long bat_info_bytes_pos_in_emmc = 0;

	unsigned char *battery_info_write_string;

	battery_info_write_string = kmalloc(10, GFP_KERNEL);

	// allocation exception handling
	if(!battery_info_write_string)
	{
		printk(KERN_ERR "battery_info_write_string allocation failed, return\n");
		return 1; //Return error report.
	}

	ret = lge_mmc_scan_partitions();
	if ( ret < 0){
		printk(KERN_ERR "[MAX17047] lge_mmc_scan_partitions scan failed, return\n");
		return 1; //Return error report.
		}
	pMisc_part = lge_mmc_find_partition_by_name("misc");
	if ( pMisc_part == NULL){
		printk(KERN_ERR "[MAX17047] lge_mmc_find_partition_by_name failed, return\n");
		return 1; //Return error report.
		}
	bat_info_bytes_pos_in_emmc = (pMisc_part->dfirstsec*512)+PTN_BAT_INFO_PERSIST_POSITION_IN_MISC_PARTITION;

	printk("Write Battery Count Info in mmc info sec : 0x%x, size : 0x%x type : 0x%x frst sec: 0x%lx\n", \
	        pMisc_part->dfirstsec, pMisc_part->dsize, pMisc_part->dtype, bat_info_bytes_pos_in_emmc);


	// Read the data from emmc first.
	memset((void*)pbuf, 0, 10);
	mtd_op_result = lge_read_block(bat_info_bytes_pos_in_emmc+30, pbuf, 10);
	if( mtd_op_result != (10) )
	{
		printk(KERN_ERR "[MAX17047]max17047_read_flash, error num = %d \n", mtd_op_result);
	}

	if (count_up == 0){
		for(i=0; i<4 ; i+=2)
			printk("[MAX17047] pbuf[%d] = %d pbuf[%d] = %d \n", i+30, pbuf[i], i+31, pbuf[i+1]);
		return 0;
		}

	//Write the count data of the battery into flash memory.
	if ( pbuf[0] == 0xFF)
		 pbuf[0] = 0; //Battery cycle count reset per 256.

	if ( pbuf[1] == 0xFF)
		 pbuf[1] = 0; //POR count reset per 256.

	 if ( pbuf[2] == 0xFF)
		 pbuf[2] = 0; //Battery exchange count reset per 256.

	pbuf[select] = pbuf[select]  + count_up;

	battery_info_write_string = pbuf;

	// Write the data to emmc.

       mtd_op_result = lge_write_block(bat_info_bytes_pos_in_emmc+30, battery_info_write_string, 10);
       if(mtd_op_result!=(10))
       {
             printk(KERN_ERR "[MAX17047]lge_write_block, error num = %d \n", mtd_op_result);
	return 1;//Return error report.
        }
	else
		printk("[MAX17047] Count info - cycle count = %d POR count = %d exchange = %d  \n",  pbuf[0], pbuf[1], pbuf[2]);	

	return 0;
}


static int max17047_access_control_of_flash(void)
{
	u8 ret = 0;

	printk("[MAX17047] access_nand_setting_flag = %d   Start flash access control\n", access_nand_setting_flag);

	if (access_nand_setting_flag == 2)
		ret = max17047_save_bat_info_check(); //Non-POR state. Go to max17047_save_bat_info_to_flash.
	else if (access_nand_setting_flag == 1)
		ret = max17047_restore_bat_info_from_flash(); //New Custom model write End. Go to max17047_restore_bat_info_from_flash.

	if (ret == 0){
		access_nand_setting_flag = 4;
		printk("[MAX17047] access_nand_setting_flag = %d   End flash access control\n", access_nand_setting_flag);
		return 0;
		}
	else if ( ret == 2){
		printk("[MAX17047] IC POR Check Error occurred. Custom model Write once more. \n");
		ret = max17047_new_custom_model_write(); //IC POR Check Error occurred. Custom model Write once more.

		if ( ret == 2)
			access_nand_setting_flag = 2; //Non-POR state. Go to max17047_save_bat_info_to_flash.
		else if ( ret == 0)
			access_nand_setting_flag = 1;//New Custom model write End. Go to max17047_restore_bat_info_from_flash.

		boot_check_count = 0; //Run max17047_access_control_of_flash again.
	}
	else
		return 0;

	return 0;
}

#endif

bool max17047_quick_start(void)
{
	u16 read_reg;
	u16 write_reg;
	u16 check_reg;
	u16 capacity = 0x1802;

	//1. Set the QuickStart and Verify bits
	read_reg = max17047_read_reg(max17047_i2c_client, MAX17047_MISC_CFG);
	write_reg = read_reg | 0x1400;
	max17047_write_reg(max17047_i2c_client, MAX17047_MISC_CFG, write_reg);

	//2. Verify no memory leaks during Quickstart writing
	read_reg = max17047_read_reg(max17047_i2c_client, MAX17047_MISC_CFG);
	check_reg = read_reg & 0x1000;
	if ( check_reg != 0x1000)
	{
	 	printk(" [MAX17047] quick_start error !!!! \n");
		return 1;
	}

	//3. Clean the Verify bit
	read_reg = max17047_read_reg(max17047_i2c_client, MAX17047_MISC_CFG);
	write_reg = read_reg & 0xefff;
	max17047_write_reg(max17047_i2c_client, MAX17047_MISC_CFG, write_reg);

	//4. Verify no memory leaks during Verify bit clearing
	read_reg = max17047_read_reg(max17047_i2c_client, MAX17047_MISC_CFG);
	check_reg = read_reg & 0x1000;
	if ( check_reg != 0x0000)
	{
		printk(" [MAX17047] quick_start error !!!! \n");
		return 1;
	}
	//5. Delay 500ms
	msleep(500);

	//6. Writing and Verify FullCAP Register Value
	max17047_i2c_write_and_verify(MAX17047_FULL_CAP, capacity);

	//7. Delay 500ms
	msleep(500);

	return 0;

}

int max17047_get_soc_for_charging_complete_at_cmd()
{

	int guage_level = 0;

	pm8921_charger_enable(0);
	pm8921_disable_source_current(1);

	printk( " [AT_CMD][at_fuel_guage_level_show] max17047_quick_start \n");
	max17047_quick_start();
	guage_level = max17047_get_capacity_percent();
	if(guage_level != 0){
		if(guage_level >= 80)
			guage_level = (real_soc * 962) /1000;
		else
			guage_level = (guage_level * 100) /100;
	}

	if (guage_level > 100)
		guage_level = 100;
	else if (guage_level < 0)
		guage_level = 0;

	pm8921_disable_source_current(0);
	pm8921_charger_enable(1);

	printk( " [AT_CMD][at_fuel_guage_soc_for_charging_complete] BATT guage_level = %d , real_soc = %d \n", guage_level, real_soc);

	return guage_level;
}
EXPORT_SYMBOL(max17047_get_soc_for_charging_complete_at_cmd);


int max17047_get_battery_mvolts(void)
{
	return max17047_get_mvolts();
}
EXPORT_SYMBOL(max17047_get_battery_mvolts);

u8 at_cmd_buf[2] = {0xff,0xff};
int max17047_get_battery_capacity_percent(void)
{
	if (at_cmd_buf[0] == 1)
		return at_cmd_buf[1];

	return max17047_get_capacity_percent();
}
EXPORT_SYMBOL(max17047_get_battery_capacity_percent);

int max17047_get_battery_current(void)
{
	return max17047_get_current();
}
EXPORT_SYMBOL(max17047_get_battery_current);

bool max17047_write_battery_temp(int battery_temp)
{
	return max17047_write_temp(battery_temp);
}
EXPORT_SYMBOL(max17047_write_battery_temp);


int max17047_get_battery_age(void)
{
	return max17047_read_battery_age();
}
EXPORT_SYMBOL(max17047_get_battery_age);

/* For max17047 AT cmd */
bool max17047_set_battery_atcmd(int flag, int value)
{
	bool ret;

	u16 soc_read;
	u16 vbat_mv;

	if (flag == 0) {
		//Call max17047 Quick Start function.
				ret = max17047_quick_start();

				if ( ret == 0){
				//Read and Verify Outputs
				soc_read = max17047_get_capacity_percent();
				vbat_mv = max17047_suspend_get_mvolts();
				printk("[MAX17047] max17047_quick_start end Reset_SOC = %d %% vbat = %d mV\n", soc_read, vbat_mv);

					if ((vbat_mv >= 4100) && (soc_read < 91)){
						at_cmd_buf[0] = 1;
						at_cmd_buf[1] = 100;
						printk("[MAX17047] max17047_quick_start error correction works. \n");
						return 1;
						}
					else
						at_cmd_buf[0] = 0;
					}
				else
					{
						at_cmd_buf[0] = 1;
						at_cmd_buf[1] = 100;
						printk("[MAX17047] max17047_quick_start error correction works. \n");
						return 1;
					}
	}
	else if (flag == 1) {
		at_cmd_buf[0] = 0;
	}

	return 0;
}

static ssize_t at_fuel_guage_reset_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int r = 0;
	bool ret = 0;
	ret = max17047_set_battery_atcmd(0, 100);  // Reset the fuel guage IC
	if ( ret == 1)
		printk("at_fuel_guage_reset_show error.\n");

 	r = sprintf(buf, "%d\n", true);
	//at_cmd_force_control = TRUE;

	msleep(100);

	return r;
}

static ssize_t at_fuel_guage_level_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int r = 0;
	int guage_level = 0;

	/* 121128 doosan.baek@lge.com Implement Power test SOC quickstart */
	if(lge_power_test_flag == 1){
		
		pm8921_charger_enable(0);
		pm8921_disable_source_current(1);
		
		printk( " [AT_CMD][at_fuel_guage_level_show] max17047_quick_start \n");
		max17047_quick_start();
		guage_level = max17047_get_capacity_percent();
		if(guage_level != 0){
			if(guage_level >= 80)
				guage_level = (real_soc * 962) /1000;
			else
				guage_level = (guage_level * 100) /100;
		}

		if (guage_level > 100)
			guage_level = 100;
		else if (guage_level < 0)
			guage_level = 0;

		printk( " [AT_CMD][at_fuel_guage_level_show] BATT guage_level = %d , real_soc = %d \n", guage_level, real_soc);
	
		pm8921_disable_source_current(0);
		pm8921_charger_enable(1);
		
		return sprintf(buf, "%d\n", guage_level);
	}
	/* 121128 doosan.baek@lge.com Implement Power test SOC quickstart */
	guage_level = max17047_get_capacity_percent();
	printk( " [AT_CMD][at_fuel_guage_level_show] not quick start BATT guage_level = %d\n", guage_level);
	r = sprintf(buf, "%d\n", guage_level);

	return r;
}

static ssize_t at_batt_level_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int r = 0;
	int battery_level = 0;


	/* 121128 doosan.baek@lge.com Implement Power test SOC quickstart */
	if(lge_power_test_flag == 1){
		pm8921_charger_enable(0);
		pm8921_disable_source_current(1);

		printk( " [AT_CMD][at_batt_level_show] max17047_quick_start \n");

		max17047_quick_start();
		battery_level =  max17047_get_battery_mvolts();

		pm8921_disable_source_current(0);
		pm8921_charger_enable(1);
		printk( " [AT_CMD][at_batt_level_show] BATT LVL = %d\n", battery_level);

		return snprintf(buf, PAGE_SIZE, "%d\n", battery_level);
	}
	/* 121128 doosan.baek@lge.com Implement Power test SOC quickstart */

	battery_level =  max17047_get_battery_mvolts();
	printk( " [AT_CMD][at_batt_level_show] not quick start BATT LVL = %d\n", battery_level);

	r = sprintf(buf, "%d\n", battery_level);

	return r;
}

DEVICE_ATTR(at_fuelrst, 0644, at_fuel_guage_reset_show, NULL);
DEVICE_ATTR(at_fuelval, 0644, at_fuel_guage_level_show, NULL);
DEVICE_ATTR(at_batl, 0644, at_batt_level_show, NULL);

static int  max17047_quickstart_check_chglogo_boot(u16 v_cell, u16 v_focv)
{
	u16 result;
	int ret;
	
	result = v_focv - v_cell;
	printk("[MAX17047]chglogoboot -result (%d),  v_focv (%d),  vcell(%d)\n", result, v_focv, v_cell);

	if(result >= 25 && result <= 100)
	{
		printk("[MAX17047] - skip quickstart\n");
	}
	else
	{
		printk("[MAX17047] - quick_start !!\n");
		ret = 1;	
	}
	
	return ret;


}
static int  max17047_quickstart_check_normalboot(u16 v_cell, u16 v_focv)
{
	u16 result;
	int ret;
	
	result = v_focv - v_cell;
	printk("[MAX17047]normalboot -result (%d),  v_focv (%d),  vcell(%d)\n", result, v_focv, v_cell);

	if(v_cell >= 3800)
	{
		if(result >= 67 && result <= 100)
		{
			printk("[MAX17047]- skip quickstart\n");
		}
		else
		{
			printk("[MAX17047]- quick_start !!\n");
			ret = 1;	
		}
	}
	else
	{
		if(result >= 72 && result <= 100)
		{
			printk("[MAX17047] - skip quickstart\n");
		}
		else
		{
			printk("[MAX17047] - quick_start !!\n");
			ret = 1;		
		}
	}

	return ret;


}


static void max17047_initial_quicstart_check(void)
{

	u16 read_reg;
	//u16 write_reg;
	u16 vfocv;
	u16 vcell;
	
	unsigned int* power_on_status = 0;
	unsigned int smem_size;

	read_reg = max17047_read_reg(max17047_i2c_client, MAX17047_V_CELL);
	vcell = (read_reg >> 3);
	vcell = (vcell * 625) / 1000;

	read_reg = max17047_read_reg(max17047_i2c_client, MAX17047_V_FOCV);
	vfocv = (read_reg >> 4);
	vfocv = (vfocv * 125) / 100;

	
	power_on_status = smem_get_entry(SMEM_POWER_ON_STATUS_INFO, &smem_size);
	
	if((0xff & *power_on_status) ==0x20)
	{
		if(max17047_quickstart_check_chglogo_boot(vcell, vfocv))
		{
			goto quick_start;
		}
	}
	else
	{
		if(max17047_quickstart_check_normalboot(vcell, vfocv))
		{
			goto quick_start;
		}

	}

	
quick_start:
	max17047_quick_start();

	return;
}

static int __devinit max17047_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct max17047_chip *chip;

	int ret = 0;

	//Test only
	//int i = 0;
	//u16 read_reg;

	F_bat("[MAX17047] %s()  Start \n", __func__);

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_WORD_DATA))
		return -EIO;

	chip = kzalloc(sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->client = client;
	chip->pdata = client->dev.platform_data;

	i2c_set_clientdata(client, chip);

	max17047_i2c_client = client;

	//Call max17047_new_custom_model_write
	ret = max17047_new_custom_model_write();

	if ( ret == 2)
		printk("NON-POWER_ON reset. Proceed Booting. \n");
	else if ( ret == 1)
		max17047_new_custom_model_write(); //Error occurred. Write once more.
	else if ( ret == 0)
		printk("POWER_ON reset. Custom Model Success. \n");//New Custom model write End. Go to max17047_restore_bat_info_from_flash.

	ret = device_create_file(&client->dev, &dev_attr_at_fuelrst);
	ret = device_create_file(&client->dev, &dev_attr_at_fuelval);
	ret = device_create_file(&client->dev, &dev_attr_at_batl);

	max17047_initial_quicstart_check();
	
	F_bat("[MAX17047] %s()  End \n", __func__);

	return 0;
}

static int __devexit max17047_remove(struct i2c_client *client)
{
	struct max17047_chip *chip = i2c_get_clientdata(client);

	//power_supply_unregister(&chip->battery);
	i2c_set_clientdata(client, NULL);
	kfree(chip);
	return 0;
}

static const struct i2c_device_id max17047_id[] = {
	{ "max17047", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, max17047_id);

static struct i2c_driver max17047_i2c_driver = {
	.driver	= {
		.name	= "max17047",
	},
	.probe		= max17047_probe,
	.remove		= __devexit_p(max17047_remove),
	.id_table	= max17047_id,
};

static int __init max17047_init(void)
{
	return i2c_add_driver(&max17047_i2c_driver);
}
module_init(max17047_init);

static void __exit max17047_exit(void)
{
	i2c_del_driver(&max17047_i2c_driver);
}
module_exit(max17047_exit);

MODULE_AUTHOR("LG Power <lge.com>");
MODULE_DESCRIPTION("MAX17047 Fuel Gauge");
MODULE_LICENSE("GPL");
