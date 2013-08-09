/*
 *  Sysfs interface for the universal power supply monitor class
 *
 *  Copyright © 2007  David Woodhouse <dwmw2@infradead.org>
 *  Copyright © 2007  Anton Vorontsov <cbou@mail.ru>
 *  Copyright © 2004  Szabolcs Gyurko
 *  Copyright © 2003  Ian Molton <spyro@f2s.com>
 *
 *  Modified: 2004, Oct     Szabolcs Gyurko
 *
 *  You may use this code as per GPL version 2
 */

#include <linux/ctype.h>
#include <linux/device.h>
#include <linux/power_supply.h>
#include <linux/slab.h>
#include <linux/stat.h>

#include "power_supply.h"

#include <mach/board_lge.h>
/*
 * This is because the name "current" breaks the device attr macro.
 * The "current" word resolves to "(get_current())" so instead of
 * "current" "(get_current())" appears in the sysfs.
 *
 * The source of this definition is the device.h which calls __ATTR
 * macro in sysfs.h which calls the __stringify macro.
 *
 * Only modification that the name is not tried to be resolved
 * (as a macro let's say).
 */
/* [START] sungsookim */
#ifdef CONFIG_LGE_PM
/* LGE_S kwangjae1.lee@lge.com 2012-06-11 Add bms debugger */
#define BMS_BATT_ATTR(_name)                                     \
{                                                                   \
	.attr = { .name = #_name, .mode = 0644 },                       \
	.show = bms_batt_show_property,                              \
	.store = bms_batt_store_property,                            \
}
/* LGE_E kwangjae1.lee@lge.com 2012-06-11 Add bms debugger */
#define PSEUDO_BATT_ATTR(_name)                                     \
{                                                                   \
	.attr = { .name = #_name, .mode = 0644 },                       \
	.show = pseudo_batt_show_property,                              \
	.store = pseudo_batt_store_property,                            \
}
#define BLOCK_CHARGING_ATTR(_name)                                  \
{                                                                   \
	.attr = { .name = #_name, .mode = 0644 },                       \
	.show = block_charging_show_property,                           \
	.store = block_charging_store_property,                         \
}
#endif
/* [END] */

#define POWER_SUPPLY_ATTR(_name)					\
{									\
	.attr = { .name = #_name },					\
	.show = power_supply_show_property,				\
	.store = power_supply_store_property,				\
}

static struct device_attribute power_supply_attrs[];

static ssize_t power_supply_show_property(struct device *dev,
					  struct device_attribute *attr,
					  char *buf) {
	static char *type_text[] = {
		"Unknown", "Battery", "UPS", "Mains", "USB",
		"USB_DCP", "USB_CDP", "USB_ACA", "BMS",
#ifdef CONFIG_LGE_WIRELESS_CHARGER
		"WIRELESS"
#endif
	};
	static char *status_text[] = {
		"Unknown", "Charging", "Discharging", "Not charging", "Full"
	};
	static char *charge_type[] = {
		"Unknown", "N/A", "Trickle", "Fast"
	};
	static char *health_text[] = {
		"Unknown", "Good", "Overheat", "Dead", "Over voltage",
		"Unspecified failure", "Cold",
	};
	static char *technology_text[] = {
		"Unknown", "NiMH", "Li-ion", "Li-poly", "LiFe", "NiCd",
		"LiMn"
	};
	static char *capacity_level_text[] = {
		"Unknown", "Critical", "Low", "Normal", "High", "Full"
	};
	static char *scope_text[] = {
		"Unknown", "System", "Device"
	};
	ssize_t ret = 0;
	struct power_supply *psy = dev_get_drvdata(dev);
	const ptrdiff_t off = attr - power_supply_attrs;
	union power_supply_propval value;

	if (off == POWER_SUPPLY_PROP_TYPE)
		value.intval = psy->type;
	else
		ret = psy->get_property(psy, off, &value);

	if (ret < 0) {
		if (ret == -ENODATA)
			dev_dbg(dev, "driver has no data for `%s' property\n",
				attr->attr.name);
		else if (ret != -ENODEV)
			dev_err(dev, "driver failed to report `%s' property: %zd\n",
				attr->attr.name, ret);
		return ret;
	}

	if (off == POWER_SUPPLY_PROP_STATUS)
		return sprintf(buf, "%s\n", status_text[value.intval]);
	else if (off == POWER_SUPPLY_PROP_CHARGE_TYPE)
		return sprintf(buf, "%s\n", charge_type[value.intval]);
	else if (off == POWER_SUPPLY_PROP_HEALTH)
		return sprintf(buf, "%s\n", health_text[value.intval]);
	else if (off == POWER_SUPPLY_PROP_TECHNOLOGY)
		return sprintf(buf, "%s\n", technology_text[value.intval]);
	else if (off == POWER_SUPPLY_PROP_CAPACITY_LEVEL)
		return sprintf(buf, "%s\n", capacity_level_text[value.intval]);
	else if (off == POWER_SUPPLY_PROP_TYPE)
		return sprintf(buf, "%s\n", type_text[value.intval]);
	else if (off == POWER_SUPPLY_PROP_SCOPE)
		return sprintf(buf, "%s\n", scope_text[value.intval]);
#ifdef CONFIG_LGE_PM
	else if (off >= POWER_SUPPLY_PROP_MODEL_NAME && off <= POWER_SUPPLY_PROP_SERIAL_NUMBER)
		return sprintf(buf, "%s\n", value.strval);
#else
	else if (off >= POWER_SUPPLY_PROP_MODEL_NAME)
		return sprintf(buf, "%s\n", value.strval);
#endif
	return sprintf(buf, "%d\n", value.intval);
}

static ssize_t power_supply_store_property(struct device *dev,
					   struct device_attribute *attr,
					   const char *buf, size_t count) {
	ssize_t ret;
	struct power_supply *psy = dev_get_drvdata(dev);
	const ptrdiff_t off = attr - power_supply_attrs;
	union power_supply_propval value;
	long long_val;

	/* TODO: support other types than int */
	ret = strict_strtol(buf, 10, &long_val);
	if (ret < 0)
		return ret;

	value.intval = long_val;

	ret = psy->set_property(psy, off, &value);
	if (ret < 0)
		return ret;

	return count;
}

/* [START] sungsookim */
#ifdef CONFIG_LGE_PM
/* LGE_S kwangjae1.lee@lge.com 2012-06-11 Add bms debugger */
static ssize_t bms_batt_show_property(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{

	ssize_t ret;
	struct power_supply *psy = dev_get_drvdata(dev);
	const ptrdiff_t off = attr - power_supply_attrs;
	union power_supply_propval value;
	static char *bms_mode[] = {
		"NORMAL", "BMSLOG",
	};

	ret = psy->get_property(psy, off, &value);

	if (ret < 0) {
		if (ret != -ENODEV)
			dev_err(dev, "driver failed to report `%s' property\n",
					attr->attr.name);
		return ret;
	}

	if (off == POWER_SUPPLY_PROP_BMS_BATT)
		return sprintf(buf, "[%s] \nusage: echo [mode] > bms_log\n", bms_mode[value.intval]);

	return 0;
}
extern int bms_batt_set(struct bms_batt_info_type*);
static ssize_t bms_batt_store_property(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct bms_batt_info_type bms_log;
	int ret = -EINVAL;

	if(sscanf(buf, "%d", &bms_log.mode) != 1)
	{
			if(bms_log.mode == 1) //bmslog mode
		{
			printk(KERN_ERR "usage : echo [mode] > bms_log");
			goto out;
		}
	}

	bms_batt_set(&bms_log);
	ret = count;

out:
	return ret;

}
/* LGE_E kwangjae1.lee@lge.com 2012-06-11 Add bms debugger */

static ssize_t pseudo_batt_show_property(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	ssize_t ret;
	struct power_supply *psy = dev_get_drvdata(dev);
	const ptrdiff_t off = attr - power_supply_attrs;
	union power_supply_propval value;

	static char *pseudo_batt[] = {
		"NORMAL", "PSEUDO",
	};

	ret = psy->get_property(psy, off, &value);

	if (ret < 0) {
		if (ret != -ENODEV)
			dev_err(dev, "driver failed to report `%s' property\n",
					attr->attr.name);
		return ret;
	}
	if (off == POWER_SUPPLY_PROP_PSEUDO_BATT)
		return sprintf(buf, "[%s] \nusage: echo [mode] [ID] [therm] [temp] [volt] [cap] [charging] > pseudo_batt\n", pseudo_batt[value.intval]);

	return 0;
}

extern int pseudo_batt_set(struct pseudo_batt_info_type*);

static ssize_t pseudo_batt_store_property(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int ret = -EINVAL;
	struct pseudo_batt_info_type info;

	if (sscanf(buf, "%d %d %d %d %d %d %d", &info.mode, &info.id, &info.therm,
				&info.temp, &info.volt, &info.capacity, &info.charging) != 7)
	{
		if(info.mode == 1) //pseudo mode
		{
			printk(KERN_ERR "usage : echo [mode] [ID] [therm] [temp] [volt] [cap] [charging] > pseudo_batt");
			goto out;
		}
	}
	pseudo_batt_set(&info);
	ret = count;
out:
	return ret;
}

extern void batt_block_charging_set(int);
static ssize_t block_charging_show_property(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	ssize_t ret;
	struct power_supply *psy = dev_get_drvdata(dev);
	const ptrdiff_t off = attr - power_supply_attrs;
	union power_supply_propval value;

	static char *block_charging_mode[] = {
		"BLOCK CHARGING", "NORMAL",
	};

	ret = psy->get_property(psy, off, &value);

	if (ret < 0) {
		if (ret != -ENODEV)
			dev_err(dev, "driver failed to report `%s' property\n",
					attr->attr.name);
		return ret;
	}
	if (off == POWER_SUPPLY_PROP_BLOCK_CHARGING)
		return sprintf(buf, "[%s] \n", block_charging_mode[value.intval]);

	return 0;
}

static ssize_t block_charging_store_property(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int ret = -EINVAL;
	int block;

	if(sscanf(buf, "%d", &block) != 1)
	{
		printk("%s:Too many argument\n",__func__);
		goto out;
	}
	printk("%s:block charging=%d\n",__func__,block);
	batt_block_charging_set(block);
	ret = count;
out:
	return ret;
}
#endif
/* [END] */
/* Must be in the same order as POWER_SUPPLY_PROP_* */
static struct device_attribute power_supply_attrs[] = {
	/* Properties of type `int' */
	POWER_SUPPLY_ATTR(status),
	POWER_SUPPLY_ATTR(charge_type),
	POWER_SUPPLY_ATTR(health),
	POWER_SUPPLY_ATTR(present),
	POWER_SUPPLY_ATTR(online),
	POWER_SUPPLY_ATTR(technology),
	POWER_SUPPLY_ATTR(cycle_count),
	POWER_SUPPLY_ATTR(voltage_max),
	POWER_SUPPLY_ATTR(voltage_min),
	POWER_SUPPLY_ATTR(voltage_max_design),
	POWER_SUPPLY_ATTR(voltage_min_design),
/* BEGIN: kidong0420.kim@lge.com 2011-10-17 display the battery voltage as mV unit */
	#ifdef CONFIG_MACH_LGE
	POWER_SUPPLY_ATTR(batt_vol),
	#else/*QCT_ORG*/
	POWER_SUPPLY_ATTR(voltage_now),
	#endif/*CONFIG_MACH_LGE */
/* END: kidong0420.kim@lge.com 2011-10-17 */
	POWER_SUPPLY_ATTR(voltage_avg),
	POWER_SUPPLY_ATTR(current_max),
	POWER_SUPPLY_ATTR(current_now),
	POWER_SUPPLY_ATTR(current_avg),
	POWER_SUPPLY_ATTR(power_now),
	POWER_SUPPLY_ATTR(power_avg),
	POWER_SUPPLY_ATTR(charge_full_design),
	POWER_SUPPLY_ATTR(charge_empty_design),
	POWER_SUPPLY_ATTR(charge_full),
	POWER_SUPPLY_ATTR(charge_empty),
	POWER_SUPPLY_ATTR(charge_now),
	POWER_SUPPLY_ATTR(charge_avg),
	POWER_SUPPLY_ATTR(charge_counter),
	POWER_SUPPLY_ATTR(energy_full_design),
	POWER_SUPPLY_ATTR(energy_empty_design),
	POWER_SUPPLY_ATTR(energy_full),
	POWER_SUPPLY_ATTR(energy_empty),
	POWER_SUPPLY_ATTR(energy_now),
	POWER_SUPPLY_ATTR(energy_avg),
	POWER_SUPPLY_ATTR(capacity),
	POWER_SUPPLY_ATTR(capacity_level),
	POWER_SUPPLY_ATTR(temp),
	POWER_SUPPLY_ATTR(temp_ambient),
	POWER_SUPPLY_ATTR(time_to_empty_now),
	POWER_SUPPLY_ATTR(time_to_empty_avg),
	POWER_SUPPLY_ATTR(time_to_full_now),
	POWER_SUPPLY_ATTR(time_to_full_avg),
	POWER_SUPPLY_ATTR(type),
	POWER_SUPPLY_ATTR(scope),
	/* Properties of type `const char *' */
	POWER_SUPPLY_ATTR(model_name),
	POWER_SUPPLY_ATTR(manufacturer),
	POWER_SUPPLY_ATTR(serial_number),
#ifdef CONFIG_LGE_PM_BATTERY_ID_CHECKER
	POWER_SUPPLY_ATTR(valid_batt_id),
#endif
/* [START] sungsookim */
#ifdef CONFIG_LGE_PM
	PSEUDO_BATT_ATTR(pseudo_batt),
	BLOCK_CHARGING_ATTR(block_charging),
	POWER_SUPPLY_ATTR(ext_pwr),
/* LGE_S kwangjae1.lee@lge.com 2012-06-11 Add bms debugger */
	BMS_BATT_ATTR(bms_log),
/* LGE_E kwangjae1.lee@lge.com 2012-06-11 Add bms debugger */

/*2012-07-11 Add battery present check in the testmode */
	POWER_SUPPLY_ATTR(real_present),
/*2012-07-11 Add battery present check in the testmode */
#ifdef CONFIG_BATTERY_MAX17047
/*doosan.baek@lge.com 20121108 Add battery condition */
	POWER_SUPPLY_ATTR(battery_condition),
	POWER_SUPPLY_ATTR(battery_age),
#endif
#endif
/* [END] */
#ifdef CONFIG_LGE_FTT_CHARGER
	POWER_SUPPLY_ATTR(ftt_anntena_level),
#endif

};

static struct attribute *
__power_supply_attrs[ARRAY_SIZE(power_supply_attrs) + 1];

static umode_t power_supply_attr_is_visible(struct kobject *kobj,
					   struct attribute *attr,
					   int attrno)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	struct power_supply *psy = dev_get_drvdata(dev);
	umode_t mode = S_IRUSR | S_IRGRP | S_IROTH;
	int i;

	if (attrno == POWER_SUPPLY_PROP_TYPE)
		return mode;

	for (i = 0; i < psy->num_properties; i++) {
		int property = psy->properties[i];

		if (property == attrno) {
			if (psy->property_is_writeable &&
			    psy->property_is_writeable(psy, property) > 0)
				mode |= S_IWUSR;

			return mode;
		}
	}

	return 0;
}

static struct attribute_group power_supply_attr_group = {
	.attrs = __power_supply_attrs,
	.is_visible = power_supply_attr_is_visible,
};

static const struct attribute_group *power_supply_attr_groups[] = {
	&power_supply_attr_group,
	NULL,
};

void power_supply_init_attrs(struct device_type *dev_type)
{
	int i;

	dev_type->groups = power_supply_attr_groups;

	for (i = 0; i < ARRAY_SIZE(power_supply_attrs); i++)
		__power_supply_attrs[i] = &power_supply_attrs[i].attr;
}

static char *kstruprdup(const char *str, gfp_t gfp)
{
	char *ret, *ustr;

	ustr = ret = kmalloc(strlen(str) + 1, gfp);

	if (!ret)
		return NULL;

	while (*str)
		*ustr++ = toupper(*str++);

	*ustr = 0;

	return ret;
}

int power_supply_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	int ret = 0, j;
	char *prop_buf;
	char *attrname;

	dev_dbg(dev, "uevent\n");

	if (!psy || !psy->dev) {
		dev_dbg(dev, "No power supply yet\n");
		return ret;
	}

	dev_dbg(dev, "POWER_SUPPLY_NAME=%s\n", psy->name);

	ret = add_uevent_var(env, "POWER_SUPPLY_NAME=%s", psy->name);
	if (ret)
		return ret;

	prop_buf = (char *)get_zeroed_page(GFP_KERNEL);
	if (!prop_buf)
		return -ENOMEM;

	for (j = 0; j < psy->num_properties; j++) {
		struct device_attribute *attr;
		char *line;

		attr = &power_supply_attrs[psy->properties[j]];

		ret = power_supply_show_property(dev, attr, prop_buf);
		if (ret == -ENODEV || ret == -ENODATA) {
			/* When a battery is absent, we expect -ENODEV. Don't abort;
			   send the uevent with at least the the PRESENT=0 property */
			ret = 0;
			continue;
		}

		if (ret < 0)
			goto out;

		line = strchr(prop_buf, '\n');
		if (line)
			*line = 0;

		attrname = kstruprdup(attr->attr.name, GFP_KERNEL);
		if (!attrname) {
			ret = -ENOMEM;
			goto out;
		}

		dev_dbg(dev, "prop %s=%s\n", attrname, prop_buf);

		ret = add_uevent_var(env, "POWER_SUPPLY_%s=%s", attrname, prop_buf);
		kfree(attrname);
		if (ret)
			goto out;
	}

out:
	free_page((unsigned long)prop_buf);

	return ret;
}
