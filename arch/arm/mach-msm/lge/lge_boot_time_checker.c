#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/module.h>
#include "lge_boot_time_checker.h"

#undef CONFIG_LGE_BOOT_TIME_CHECK_SBL1_SBL2

#ifdef CONFIG_LGE_BOOT_TIME_CHECK_SBL1_SBL2
static ssize_t
sbl1_log_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int len, i;
	char sbl_log;
	unsigned char *ioremap_address;

	len = snprintf(buf, PAGE_SIZE, "\n******** display sbl1 log! ********\n");
//LGE_UPDATE_S, inho.oh@lge.com, 2012-08-03 - Using ioremap cause Memory Leakage because there is no iounmap
	if (SBL1_LOG_SIZE < 4096)
	    ioremap_address = ioremap(SBL1_LOG_ADDR, SBL1_LOG_SIZE);
	else
	    return 0;

	for (i = 0 ; i < SBL1_LOG_SIZE ; i++)
	{
		sbl_log = readb(ioremap_address + i);
		len += snprintf(buf + len, PAGE_SIZE - len, "%c", sbl_log);
	}

	iounmap(ioremap_address);
#if 0
	for (i = 0 ; i < SBL1_LOG_SIZE ; i++) {
		sbl_log = readb(ioremap(SBL1_LOG_ADDR + i, 1));
		len += snprintf(buf + len, PAGE_SIZE - len, "%c", sbl_log);
	}
#endif
//LGE_UPDATE_E, inho.oh@lge.com, 2012-08-03 - Using ioremap cause Memory Leakage because there is no iounmap
	len += snprintf(buf + len, PAGE_SIZE - len, "\n");
	return len;
}

static ssize_t
sbl2_log_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int len, i;
	char sbl_log;
	unsigned char *ioremap_address;

	len = snprintf(buf, PAGE_SIZE, "\n******** display sbl2 log! ********\n");
//LGE_UPDATE_S, inho.oh@lge.com, 2012-08-03 - Using ioremap cause Memory Leakage because there is no iounmap
	if (SBL2_LOG_SIZE < 4096)
	    ioremap_address = ioremap(SBL2_LOG_ADDR, SBL2_LOG_SIZE);
	else
	    return 0;

	for (i = 0 ; i < SBL2_LOG_SIZE ; i++)
	{
	    sbl_log = readb(ioremap_address + i);
	    len += snprintf(buf + len, PAGE_SIZE - len, "%c", sbl_log);
	}

	iounmap(ioremap_address);
#if 0
	for (i = 0 ; i < SBL2_LOG_SIZE ; i++) {
		sbl_log = readb(ioremap(SBL2_LOG_ADDR + i, 1));
		len += snprintf(buf + len, PAGE_SIZE - len, "%c", sbl_log);
	}
#endif
//LGE_UPDATE_E, inho.oh@lge.com, 2012-08-03 - Using ioremap cause Memory Leakage because there is no iounmap
	len += snprintf(buf + len, PAGE_SIZE - len, "\n");
	return len;
}
#endif

static ssize_t
sbl3_log_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int len, i;
	char sbl_log;
	unsigned char *ioremap_address;

	len = snprintf(buf, PAGE_SIZE, "\n******** display sbl3 log! ********\n");
//LGE_UPDATE_S, inho.oh@lge.com, 2012-08-03 - Using ioremap cause Memory Leakage because there is no iounmap
	if (SBL3_LOG_SIZE < 4096)
		ioremap_address = ioremap(SBL3_LOG_ADDR, SBL3_LOG_SIZE);
	else
	    return 0;

	for (i = 0 ; i < SBL3_LOG_SIZE ; i++)
	{
		sbl_log = readb(ioremap_address + i);
		len += snprintf(buf + len, PAGE_SIZE - len, "%c", sbl_log);
	}

	iounmap(ioremap_address);

#if 0
	for (i = 0 ; i < SBL3_LOG_SIZE ; i++) {
		sbl_log = readb(ioremap(SBL3_LOG_ADDR + i, 1));
		len += snprintf(buf + len, PAGE_SIZE - len, "%c", sbl_log);
	}
#endif
//LGE_UPDATE_E, inho.oh@lge.com, 2012-08-03 - Using ioremap cause Memory Leakage because there is no iounmap
	len += snprintf(buf + len, PAGE_SIZE - len, "\n");
	return len;
}

static ssize_t
lk_time_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int len;
	unsigned start, end;

	start = readb(ioremap(LK_START_TIME_ADDR, 4));
	end = readl(ioremap(LK_END_TIME_ADDR, 4));

	len = snprintf(buf, PAGE_SIZE, "\n******** display lk time! ********\n");
	len += snprintf(buf + len, PAGE_SIZE - len, "=============================\n");
	len += snprintf(buf + len, PAGE_SIZE - len, "start : %d\n", start);
	len += snprintf(buf + len, PAGE_SIZE - len, "end   : %d\n", end);
	len += snprintf(buf + len, PAGE_SIZE - len, "delta : %d\n", end - start);

	return len;
}
#ifdef CONFIG_LGE_BOOT_TIME_CHECK_SBL1_SBL2
static ssize_t
total_time_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int len, i;
	char sbl_log;
	unsigned start, end;

	start = readb(ioremap(LK_START_TIME_ADDR, 4));
	end = readl(ioremap(LK_END_TIME_ADDR, 4));

	len = snprintf(buf, PAGE_SIZE, "\n******** display sbl1 to sbl3 log! ********\n\n");
	for (i = 0 ; i < SBL1_LOG_SIZE ; i++) {
		sbl_log = readb(ioremap(SBL1_LOG_ADDR + i, 1));
		len += snprintf(buf + len, PAGE_SIZE - len, "%c", sbl_log);
	}
	len += snprintf(buf + len, PAGE_SIZE - len, "\n");

	for (i = 0 ; i < SBL2_LOG_SIZE ; i++) {
		sbl_log = readb(ioremap(SBL2_LOG_ADDR + i, 1));
		len += snprintf(buf + len, PAGE_SIZE - len, "%c", sbl_log);
	}
	len += snprintf(buf + len, PAGE_SIZE - len, "\n");

	for (i = 0 ; i < SBL3_LOG_SIZE ; i++) {
		sbl_log = readb(ioremap(SBL3_LOG_ADDR + i, 1));
		len += snprintf(buf + len, PAGE_SIZE - len, "%c", sbl_log);
	}

	len += snprintf(buf + len, PAGE_SIZE - len, "\n");

	return len;
}
#endif

static struct device_attribute boot_time_device_attrs[] = {
#ifdef CONFIG_LGE_BOOT_TIME_CHECK_SBL1_SBL2
	__ATTR(sbl1_log,  S_IRUGO, sbl1_log_show, NULL),
	__ATTR(sbl2_log, S_IRUGO, sbl2_log_show, NULL),
#endif
	__ATTR(sbl3_log, S_IRUGO, sbl3_log_show, NULL),
	__ATTR(lk_time, S_IRUGO, lk_time_show, NULL),
#ifdef CONFIG_LGE_BOOT_TIME_CHECK_SBL1_SBL2
	__ATTR(total_time, S_IRUGO, total_time_show, NULL),
#endif
};

static int __init lge_boot_time_checker_probe(struct platform_device *pdev)
{
	int i, ret;

	printk("[BOOT TIME CHECKER]\n");

	for (i = 0; i < ARRAY_SIZE(boot_time_device_attrs); i++) {
		ret = device_create_file(&pdev->dev, &boot_time_device_attrs[i]);
		if (ret)
			return -1;
	}

	return 0;
}

static int __devexit lge_boot_time_checker_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver lge_boot_time_driver __refdata = {
	.probe = lge_boot_time_checker_probe,
	.remove = __devexit_p(lge_boot_time_checker_remove),
	.driver = {
		.name = "boot_time",
		.owner = THIS_MODULE,
	},
};

static int __init lge_boot_time_checker_init(void)
{
	return platform_driver_register(&lge_boot_time_driver);
}

static void __exit lge_boot_time_checker_exit(void)
{
	platform_driver_unregister(&lge_boot_time_driver);
}

module_init(lge_boot_time_checker_init);
module_exit(lge_boot_time_checker_exit);

MODULE_DESCRIPTION("LGE Boot Time Checker Driver");
MODULE_AUTHOR("Fred Cho <fred.cho@lge.com>");
MODULE_LICENSE("GPL");
