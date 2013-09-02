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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/msm_tsens.h>
#include <linux/workqueue.h>
#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/msm_tsens.h>
#include <linux/msm_thermal.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <mach/cpufreq.h>
#ifdef CONFIG_LGE_PM
#include <mach/board_lge.h>
#endif

#ifdef CONFIG_LGE_PM
#define DEF_ALLOWED_MAX_FREQ 	918000
#define DEF_ALLOWED_MAX_FREQ_1 1026000
#define DEF_ALLOWED_MAX_FREQ_2 1242000
#endif

static int enabled;
static struct msm_thermal_data msm_thermal_info;
static uint32_t limited_max_freq = MSM_CPUFREQ_NO_LIMIT;
static struct delayed_work check_temp_work;

static int limit_idx;
static int limit_idx_low;
static int limit_idx_high;
static struct cpufreq_frequency_table *table;
#if defined(CONFIG_MACH_APQ8064_GK_KR) || defined(CONFIG_MACH_APQ8064_GKATT)\
	|| defined(CONFIG_MACH_APQ8064_GVDCM) || defined(CONFIG_MACH_APQ8064_GV_KR) || defined(CONFIG_MACH_APQ8064_GKGLOBAL)
static struct timer_list limit_timer;
extern int limit_cpufreq;
#endif

static int msm_thermal_get_freq_table(void)
{
	int ret = 0;
	int i = 0;

	table = cpufreq_frequency_get_table(0);
	if (table == NULL) {
		pr_debug("%s: error reading cpufreq table\n", __func__);
		ret = -EINVAL;
		goto fail;
	}

	while (table[i].frequency != CPUFREQ_TABLE_END)
		i++;

	limit_idx_low = 0;
	limit_idx_high = limit_idx = i - 1;

	BUG_ON(limit_idx_high <= 0 || limit_idx_high <= limit_idx_low);
fail:
	return ret;
}

static int update_cpu_max_freq(int cpu, uint32_t max_freq)
{
	int ret = 0;

	ret = msm_cpufreq_set_freq_limits(cpu, MSM_CPUFREQ_NO_LIMIT, max_freq);
	if (ret)
		return ret;

	limited_max_freq = max_freq;
	if (max_freq != MSM_CPUFREQ_NO_LIMIT)
		pr_info("msm_thermal: Limiting cpu%d max frequency to %d\n",
				cpu, max_freq);
	else
		pr_info("msm_thermal: Max frequency reset for cpu%d\n", cpu);

	ret = cpufreq_update_policy(cpu);

	return ret;
}

static void check_temp(struct work_struct *work)
{
	static int limit_init;
	struct tsens_device tsens_dev;
	unsigned long temp = 0;
	uint32_t max_freq = limited_max_freq;
	int cpu = 0;
	int ret = 0;

	tsens_dev.sensor_num = msm_thermal_info.sensor_id;
	ret = tsens_get_temp(&tsens_dev, &temp);
	if (ret) {
		pr_debug("msm_thermal: Unable to read TSENS sensor %d\n",
				tsens_dev.sensor_num);
		goto reschedule;
	}

	if (!limit_init) {
		ret = msm_thermal_get_freq_table();
		if (ret)
			goto reschedule;
		else
			limit_init = 1;
	}

#if defined(CONFIG_MACH_APQ8064_GK_KR) || defined(CONFIG_MACH_APQ8064_GKATT)\
		|| defined(CONFIG_MACH_APQ8064_GVDCM) || defined(CONFIG_MACH_APQ8064_GV_KR) || defined(CONFIG_MACH_APQ8064_GKGLOBAL)
	if (lge_get_factory_boot())
		return;

	if (temp >= msm_thermal_info.limit_temp_degC
		|| temp <= msm_thermal_info.limit_temp_degC_low) {
		max_freq = DEF_ALLOWED_MAX_FREQ_1;
		pr_info("msm_thermal: tsens_temp %ld\n", temp); 
	} else if ( (temp < msm_thermal_info.limit_temp_degC -
		 msm_thermal_info.temp_hysteresis_degC)
		 && (temp > msm_thermal_info.limit_temp_degC_low)) {
		max_freq = DEF_ALLOWED_MAX_FREQ_2;
	} else {
		if(limited_max_freq == MSM_CPUFREQ_NO_LIMIT)
			max_freq = DEF_ALLOWED_MAX_FREQ_2;
	}
#else
	if (temp >= msm_thermal_info.limit_temp_degC) {
		if (limit_idx == limit_idx_low)
			goto reschedule;

		limit_idx -= msm_thermal_info.freq_step;
		if (limit_idx < limit_idx_low)
			limit_idx = limit_idx_low;
		max_freq = table[limit_idx].frequency;
#ifdef CONFIG_LGE_PM
		max_freq = DEF_ALLOWED_MAX_FREQ;
		pr_info("msm_thermal: tsens_temp %ld\n", temp); 
#endif
	} else if ( (temp < msm_thermal_info.limit_temp_degC -
		 msm_thermal_info.temp_hysteresis_degC)	) {
		if (limit_idx == limit_idx_high)
			goto reschedule;

		limit_idx += msm_thermal_info.freq_step;
		if (limit_idx >= limit_idx_high) {
			limit_idx = limit_idx_high;
			max_freq = MSM_CPUFREQ_NO_LIMIT;
		} else
			max_freq = table[limit_idx].frequency;
	}
#endif
	if (max_freq == limited_max_freq)
		goto reschedule;

	/* Update new limits */
	for_each_possible_cpu(cpu){
		ret = update_cpu_max_freq(cpu, max_freq);
		if (ret)
			pr_debug("Unable to limit cpu%d max freq to %d\n",
					cpu, max_freq);
	}

reschedule:
	if (enabled)
		schedule_delayed_work(&check_temp_work,
				msecs_to_jiffies(msm_thermal_info.poll_ms));
}

#if defined(CONFIG_MACH_APQ8064_GK_KR) || defined(CONFIG_MACH_APQ8064_GKATT)\
	|| defined(CONFIG_MACH_APQ8064_GVDCM) || defined(CONFIG_MACH_APQ8064_GV_KR) || defined(CONFIG_MACH_APQ8064_GKGLOBAL)
static void msm_thermal_limit_holding_timer(unsigned long data)
{
	int cpu = 0;

	pr_info("msm_thermal: Reset to limit cpu%d cur freq\n", cpu);
	limit_cpufreq = 0;
}
#endif

static void disable_msm_thermal(void)
{
	int cpu = 0;

	/* make sure check_temp is no longer running */
	cancel_delayed_work(&check_temp_work);
	flush_scheduled_work();

#if defined(CONFIG_MACH_APQ8064_GK_KR) || defined(CONFIG_MACH_APQ8064_GKATT)\
	|| defined(CONFIG_MACH_APQ8064_GVDCM) || defined(CONFIG_MACH_APQ8064_GV_KR) || defined(CONFIG_MACH_APQ8064_GKGLOBAL)
	if (lge_get_factory_boot())
		return;

	if (limited_max_freq == DEF_ALLOWED_MAX_FREQ_1) {
		pr_info("msm_thermal: Continue to limit cpu%d max freq to %d\n", 
			cpu, DEF_ALLOWED_MAX_FREQ_1);
		return;
	}

	if (limited_max_freq == DEF_ALLOWED_MAX_FREQ_2) {
		pr_info("msm_thermal: Continue to limit cpu%d cur freq to %d\n", 
			cpu, DEF_ALLOWED_MAX_FREQ_2);
		limit_cpufreq = 1;
		if( timer_pending(&limit_timer))
			del_timer(&limit_timer);
		init_timer(&limit_timer);
		limit_timer.function = msm_thermal_limit_holding_timer;
		limit_timer.expires = jiffies + msecs_to_jiffies(6000);
		add_timer(&limit_timer);
	}
#endif

	if (limited_max_freq == MSM_CPUFREQ_NO_LIMIT)
		return;

	for_each_possible_cpu(cpu){
		update_cpu_max_freq(cpu, MSM_CPUFREQ_NO_LIMIT);
	}
}

static int set_enabled(const char *val, const struct kernel_param *kp)
{
	int ret = 0;

	ret = param_set_bool(val, kp);
	if (!enabled)
		disable_msm_thermal();
	else
		pr_info("msm_thermal: no action for enabled = %d\n", enabled);

	pr_info("msm_thermal: enabled = %d\n", enabled);

	return ret;
}

static struct kernel_param_ops module_ops = {
	.set = set_enabled,
	.get = param_get_bool,
};

module_param_cb(enabled, &module_ops, &enabled, 0644);
MODULE_PARM_DESC(enabled, "enforce thermal limit on cpu");

int __devinit msm_thermal_init(struct msm_thermal_data *pdata)
{
	int ret = 0;

	BUG_ON(!pdata);
	BUG_ON(pdata->sensor_id >= TSENS_MAX_SENSORS);
	memcpy(&msm_thermal_info, pdata, sizeof(struct msm_thermal_data));
	
	enabled = 1;
	INIT_DELAYED_WORK(&check_temp_work, check_temp);
	schedule_delayed_work(&check_temp_work, 0);

	return ret;
}

static int __devinit msm_thermal_dev_probe(struct platform_device *pdev)
{
	int ret = 0;
	char *key = NULL;
	struct device_node *node = pdev->dev.of_node;
	struct msm_thermal_data data;

	memset(&data, 0, sizeof(struct msm_thermal_data));
	key = "qcom,sensor-id";
	ret = of_property_read_u32(node, key, &data.sensor_id);
	if (ret)
		goto fail;
	WARN_ON(data.sensor_id >= TSENS_MAX_SENSORS);

	key = "qcom,poll-ms";
	ret = of_property_read_u32(node, key, &data.poll_ms);
	if (ret)
		goto fail;

	key = "qcom,limit-temp";
	ret = of_property_read_u32(node, key, &data.limit_temp_degC);
	if (ret)
		goto fail;

	key = "qcom,temp-hysteresis";
	ret = of_property_read_u32(node, key, &data.temp_hysteresis_degC);
	if (ret)
		goto fail;

	key = "qcom,freq-step";
	ret = of_property_read_u32(node, key, &data.freq_step);

fail:
	if (ret)
		pr_err("%s: Failed reading node=%s, key=%s\n",
		       __func__, node->full_name, key);
	else
		ret = msm_thermal_init(&data);

	return ret;
}

static struct of_device_id msm_thermal_match_table[] = {
	{.compatible = "qcom,msm-thermal"},
	{},
};

static struct platform_driver msm_thermal_device_driver = {
	.probe = msm_thermal_dev_probe,
	.driver = {
		.name = "msm-thermal",
		.owner = THIS_MODULE,
		.of_match_table = msm_thermal_match_table,
	},
};

int __init msm_thermal_device_init(void)
{
	return platform_driver_register(&msm_thermal_device_driver);
}
