/* arch/arm/mach-msm/lge/lge_qfprom_access.c
 *
 * Copyright (C) 2010-2012 LGE, Inc
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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <asm/setup.h>
#include <mach/board_lge.h>
#include <mach/scm.h>

#define LGE_QFPROM_INTERFACE_NAME "lge-apq8064-qfprom"
/* service ID inside tzbsp */
#define QFPROM_SVC_ID           8
#define TZBSP_SVC_OEM           254
#define TZBSP_ADDITIONAL_CMD    0x1
#define QFPROM_WRITE_CMD        0x3
#define QFPROM_READ_CMD	        0x5
#define QFPROM_PRNG_CMD         0x7
#define QFPROM_OVERRIDE_CMD     0x8

/* tzbsp_boot_milestone_status type */
#define TZBSP_MILESTONE_TRUE    0x0132DD1B
#define TZBSP_MILESTONE_FALSE   0x013302A1
#define EMILE  0x25

/* qfprom read type */
#define QFPROM_ADDR_SPACE_RAW  0
#define QFPROM_ADDR_SPACE_CORR 1

#define QFPROM_CLOCK           (0x40*1000)

/* QFPROM address to blow */
#define QFPROM_SPARE_REGION_22  0x700410
#define QFPROM_SPARE_REGION_24  0x700428
#define QFPROM_SPARE_REGION_25  0x700430
#define QFPROM_SPARE_REGION_26  0x700438
#define QFPROM_SPARE_REGION_27  0x700440

#define QFPROM_HW_KEY_STATUS            0x702050
#define QFPROM_SECURE_BOOT_ENABLE       0x700310
#define QFPROM_OEM_CONFIG               0x700230
#define QFPROM_DEBUG_ENABLE             0x700220
#define QFPROM_SECONDARY_HW_KEY         0x700268
#define QFPROM_READ_PERMISSION          0x7000A8
#define QFPROM_WRITE_PERMISSION	        0x7000B0
#define QFPROM_OVERRIDE_REG             0x7060C0
#define QFPROM_CHECK_HW_KEY             0x123456
/* secondary hw key status flag */
#define SEC_HW_KEY_BLOWN  0x00000002
#define PRIM_HW_KEY_BLOWN 0x00000001
#define HW_KEYS_BLOCKED   0x00000003

#define HW_KEY_LSB_FEC_MASK 0xC1FF83FF
#define HW_KEY_MSB_FEC_MASK 0x007FE0FF
#define FUSING_COMPLETED_STATE 0x3F
#define RANDOM_BY_TZBSP 1

/* command buffer to write */
struct qfprom_write_cmd_buffer {
	u32 qfprom_addr;	/* qfprom address */
	u32 buf;		/* data to write qfprom */
	u32 qfprom_clk;		/* qfprom clock */
	u32 qfprom_status;	/* qfprom status */
};
/* command buffer to read */
struct qfprom_read_cmd_buffer {
	u32 qfprom_addr;
	u32 qfprom_addr_type;
	u32 read_buf;
	u32 qfprom_status;
};
/* blow data structure */
struct qfprom_blow_data {
	u32 qfprom_addr;
	u32 lsb_data;
	u32 msb_data;
};

static u32 fusing_flag=0;
static u32 qfprom_address = 0;
static u32 qfprom_lsb_value = 0;
static u32 qfprom_msb_value = 0;
static u32 enable = 0;

u32 qfprom_secondary_hwkey_status(void);
int qfprom_blow_secondary_hwkey_region(void);
int qfuse_write_single_row(u32 fuse_addr, u32 fuse_lsb, u32 fuse_msb);
int qfuse_read_single_row(u32 fuse_addr, u32 addr_type, u32 * r_buf);

static struct qfprom_blow_data blow_data[] = {
	/* addr                        LSB         MSB */
	{ QFPROM_OEM_CONFIG,         0x00000031, 0x00000000}, /* OEM ID        */
	{ QFPROM_SECURE_BOOT_ENABLE, 0x00000020, 0x00000000}, /* SECURE ENABLE */
	{ QFPROM_DEBUG_ENABLE,       0xC1300000, 0x0000006F}, /* JTAG DISABLE */
	{ QFPROM_CHECK_HW_KEY,       0x0,        0x0},
	{ QFPROM_READ_PERMISSION,    0x0C000000, 0x00000000}, /* READ PERMISSION */
	{ QFPROM_WRITE_PERMISSION,   0x54100000, 0x00000000} /* WRITE PERMISSION */
	//{QFPROM_SPARE_REGION_24, 0x0132DD1B, 0x013302A1},	/* Test Code */
	//{QFPROM_SPARE_REGION_25, 0x013302A1, 0x0132DD1B}	/* Test Code */
};

/* this api handle diag command(fusing check command) from ATD 
 * if fusing value 0 ==> is not fused
 * if fusing value 1 ==> fused (secure boot enable, jtag disable,
 * oem config, hw secondary key, RW permission)
 */
static ssize_t qfusing_show(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	int fusing = 0;
	int i, ret;
	u32 key_status = 0;
	u32 *p_buf = NULL;

	if(fusing_flag==0) {
////////////////////////////////////////
// workaround for apq8064 //
// hardcoding to understand easily.//

			p_buf = kmalloc(sizeof(u32) * 2, GFP_KERNEL);
			if (!p_buf) {
				printk("%s: memory alloc fail\n", __func__);
				goto err_mem;
			}

			memset(p_buf, 0x00, sizeof(u32) * 2);
			msleep(10);
			ret =
			    qfuse_read_single_row(QFPROM_DEBUG_ENABLE, 0,
						  p_buf);
			printk("%s: addr = 0x%x LSB = 0x%x MSB = 0x%x\n",
			       __func__, QFPROM_DEBUG_ENABLE, p_buf[0],
			       p_buf[1]);
			if (ret != 0) {
				printk("%s: qfprom read fail error=%d\n", __func__,ret);
				kfree(p_buf);
				goto err_mem;

			}

			if ((p_buf[0] == 0xC1000000) && (p_buf[1] == 0x0000006F)) {

					printk("old versionwill be passed unquestioningly for factory process");
					goto workaround_for_old_apq8064;

			} else if  ((p_buf[0] == 0xC1300000) && (p_buf[1]== 0x0000006F)) {

					printk("new version will be passed unquestioningly for factory process");
					goto workaround_for_new_apq8064;

			} else {

					printk("empty or strange will be passed unquestioningly for factory process");
					goto workaround_for_new_apq8064;

			}


////////////////////////////////////////

workaround_for_new_apq8064:

	key_status = qfprom_secondary_hwkey_status();
	if ((key_status & SEC_HW_KEY_BLOWN) != SEC_HW_KEY_BLOWN) {
			printk("%s: hw key is not blown\n",__func__);
		goto err;
	} else {
			msleep(10);
			printk("%s:secondary HW key check complete!!!!!\n",__func__);

workaround_for_old_apq8064:

		for (i = 0; i < ARRAY_SIZE(blow_data); i++) {
			if (blow_data[i].qfprom_addr == QFPROM_CHECK_HW_KEY)
				continue;

			memset(p_buf, 0x00, sizeof(u32) * 2);
			msleep(10);
			ret =
			    qfuse_read_single_row(blow_data[i].qfprom_addr, 0,
						  p_buf);
			printk("%s: addr = 0x%x LSB = 0x%x MSB = 0x%x\n",
			       __func__, blow_data[i].qfprom_addr, p_buf[0],
			       p_buf[1]);
			if (ret != 0) {
				printk("%s: qfprom read fail error=%d\n", __func__,ret);
				goto err;
			} else {

////////////////////////////////////////
// workaround for apq8064 //
// hardcoding to understand easily.//
if (blow_data[i].qfprom_addr == QFPROM_DEBUG_ENABLE) {
	if ((p_buf[0] == 0xC1000000) && (p_buf[1] == 0x0000006F)) {

		printk("old version will be passed unquestioningly for factory process");
		continue;
	}
}
///////////////////////////////////////



					if(((p_buf[0]&blow_data[i].lsb_data)==blow_data[i].lsb_data) &&
						((p_buf[1]&blow_data[i].msb_data)==blow_data[i].msb_data)) {
						printk("%s: 0x%x chekc complete\n",__func__,blow_data[i].qfprom_addr);
						continue;
					}
				else {
					printk
					    ("%s: fusing value is not match\n",
					     __func__);
					goto err;
				}
					msleep(10);
			}
		}
		fusing = 1;
	}
	} 
	else {
		if(fusing_flag==FUSING_COMPLETED_STATE)
			fusing=1;
	}
err:
	kfree(p_buf);
err_mem:
	return sprintf(buf, "%x\n", fusing);
}

/* this api handle diag command(fusing command) from ATD
 * this api fuse secure boot, jtag disable, oem config,
 * secondary hw key, R/W permission
 * this api check secondary hw key status before fusing R/W permission
 */
static ssize_t qfusing_store(struct device *dev, struct device_attribute *attr,
			     const char *buf, size_t count)
{
	int ret;
	int i=0;
	u32* p_buf=NULL;

	if (!sysfs_streq(buf, "fusing")) {
		printk("%s:argument fault\n", __func__);
		ret=-EINVAL;
		goto err;
	}

	p_buf = kmalloc(sizeof(u32)*2, GFP_KERNEL);
	if(!p_buf) {
		printk("%s: memory alloc fail\n",__func__);
		ret=-EINVAL;
		goto err;;

	}
	memset(p_buf, 0 , sizeof(u32)*2);

	for(i=0;i<ARRAY_SIZE(blow_data);i++){
		if(blow_data[i].qfprom_addr==QFPROM_CHECK_HW_KEY) {
			/* We dont check secondary hw key status 
			 * But qfprom_blow secondary_hwkey_region api does not create random if qfprom block was written
			 * The api create random if was not written only
			 * So HW key region to be written is not written by new random key
			 * The reason to not check hw key status reg is to check 7 hw key block to be written
			 */
			ret = qfprom_blow_secondary_hwkey_region();
			if (ret < 0) {
				printk("%s: hw key region blow error\n",
				       __func__);
					goto err_fuse;
			}
			fusing_flag |= (0x1<<i);
			printk("%s: HW secondary key region is blown successfully\n",__func__);
			continue;
		}

		msleep(10);
		ret = qfuse_read_single_row(blow_data[i].qfprom_addr,0,p_buf);
		if(ret!=0) {
			printk("%s: qfprom addr %x read fail, ret=%d\n",__func__,blow_data[i].qfprom_addr,ret);
			goto err_fuse;
		}
		printk("%s:read addr 0x%x, lsb 0x%x, msb 0x%x\n",__func__,blow_data[i].qfprom_addr,p_buf[0],p_buf[1]);
		/* Don't rewrite if value to read is same value to write */
		if(((p_buf[0]&blow_data[i].lsb_data)==blow_data[i].lsb_data) &&
			((p_buf[1]&blow_data[i].msb_data)==blow_data[i].msb_data)) {
			printk("%s: 0x%x was blown already\n",__func__,blow_data[i].qfprom_addr);
		}
		else {
			msleep(10);
			ret = qfuse_write_single_row(blow_data[i].qfprom_addr,blow_data[i].lsb_data,blow_data[i].msb_data);
			if(ret!=0){
				printk("%s: qfuse addr %x blow write error!!!\n",__func__,blow_data[i].qfprom_addr);
				ret = -EINVAL;
				goto err_fuse;
			} else {
				/* double check routine*/
				msleep(10);
				printk("%s: qfprom 0x%x addr write double check routine\n",__func__,blow_data[i].qfprom_addr);
				ret = qfuse_read_single_row(blow_data[i].qfprom_addr,0,p_buf);
				if(ret !=0 ){
					printk("%s: read fail when double check, ret=%d\n",__func__,ret);
					ret = -EINVAL;
					goto err_fuse;
				}
				if(((p_buf[0]&blow_data[i].lsb_data)==blow_data[i].lsb_data) &&
					((p_buf[1]&blow_data[i].msb_data)==blow_data[i].msb_data)) {
					printk("%s:write double check successfully",__func__);
				} else {
					printk("%s:qfprom write successful but error when double check\n",__func__);
					ret = -EINVAL;
					goto err_fuse;
				}

			}

		}
		fusing_flag |= (0x1<<i);
	}
	printk("%s: fusing flag = 0x%x\n",__func__,fusing_flag);
	printk("%s: fusing complete!!!!!!!!!!!!!!!!!!!!\n",__func__);
	kfree(p_buf);
	return count;

err_fuse:
	kfree(p_buf);
err:
	return ret;
}

static DEVICE_ATTR(qfusing, S_IWUSR | S_IRUGO, qfusing_show, qfusing_store);

static ssize_t qfprom_addr_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%x\n", qfprom_address);
}

static ssize_t qfprom_addr_store(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t count)
{
	unsigned long val;
	if (strict_strtoul(buf, 16, &val) < 0)
		return -EINVAL;
	qfprom_address = val;
	return count;
}

static DEVICE_ATTR(addr, S_IWUSR | S_IRUGO, qfprom_addr_show,
		   qfprom_addr_store);

static ssize_t qfprom_lsb_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%x\n", qfprom_lsb_value);
}

static ssize_t qfprom_lsb_store(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t count)
{
	unsigned long val;
	if (strict_strtoul(buf, 16, &val) < 0)
		return -EINVAL;
	qfprom_lsb_value = val;
	return count;
}

static DEVICE_ATTR(lsb, S_IWUSR | S_IRUGO, qfprom_lsb_show, qfprom_lsb_store);

static ssize_t qfprom_msb_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%x\n", qfprom_msb_value);
}

static ssize_t qfprom_msb_store(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t count)
{
	unsigned long val;
	if (strict_strtoul(buf, 16, &val) < 0)
		return -EINVAL;
	qfprom_msb_value = val;
	return count;
}

static DEVICE_ATTR(msb, S_IWUSR | S_IRUGO, qfprom_msb_show, qfprom_msb_store);

static ssize_t qfprom_enable_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%x\n", enable);
}

static ssize_t qfprom_enable_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	unsigned long val;
	if (strict_strtoul(buf, 16, &val) < 0)
		return -EINVAL;
	enable = val;
	return count;
}

static DEVICE_ATTR(enable, S_IWUSR | S_IRUGO, qfprom_enable_show,
		   qfprom_enable_store);

static ssize_t qfprom_write_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	int ret = 0;
	if (!enable) {
		printk("%s: qfprom write is not enabled\n", __func__);
		return -EINVAL;
	}
	if (!qfprom_address) {
		printk("%s: qfprom address is NULL\n", __func__);
		return -EINVAL;
	}
	ret = qfuse_write_single_row(qfprom_address, qfprom_lsb_value,
				   qfprom_msb_value);
	qfprom_address = 0;
	if (ret == 0)
		return count;
	else if (ret < 0)
		printk("%s: scm call fail error = %d\n", __func__, ret);
	else
		printk("%s: qfprom write status error = %d\n", __func__, ret);

	return -EINVAL;
}

static DEVICE_ATTR(write, S_IWUSR | S_IRUGO, NULL, qfprom_write_store);

static ssize_t qfprom_read_store(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t count)
{
	u32 *p_buf = NULL;
	int ret = 0;

	if (!qfprom_address) {
		printk("%s: qfprom address is NULL\n", __func__);
		return -EINVAL;
	}

	p_buf = kmalloc(sizeof(u32) * 2, GFP_KERNEL);
	if (!p_buf) {
		printk("%s : buffer memory alloc fail\n", __func__);
		ret = -ENOMEM;
	}
	memset(p_buf, 0, sizeof(u32) * 2);
	ret = qfuse_read_single_row(qfprom_address, 0, p_buf);
	qfprom_address = 0;
	if (ret == 0) {
		qfprom_lsb_value = p_buf[0];
		qfprom_msb_value = p_buf[1];
		kfree(p_buf);
		return count;
	} else if (ret < 0)
		printk("%s: scm call fail error = %d\n", __func__, ret);
	else
		printk("%s: qfprom write status error = %d\n", __func__, ret);

	kfree(p_buf);

	return -EINVAL;
}

static DEVICE_ATTR(read, S_IWUSR | S_IRUGO, NULL, qfprom_read_store);

static ssize_t qfprom_override_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	int val;
	void __iomem *r;

	r = ioremap(QFPROM_OVERRIDE_REG, 0x4);
	val = (uint32_t) readl(r);
	iounmap(r);
	return snprintf(buf, PAGE_SIZE, "%x\n", val);
}

static ssize_t qfprom_override_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	int ret;
	u32 on;

	if (!sysfs_streq(buf, "enable")) {
		printk("%s:argument fault\n", __func__);
		return -EINVAL;
	}

	on = 1;
	ret = scm_call(QFPROM_SVC_ID, QFPROM_OVERRIDE_CMD, &on,
			sizeof(on), NULL, 0);
	if (ret < 0) {
		printk("%s: scm call error\n", __func__);
		return -EINVAL;
	}
	return count;
}

static DEVICE_ATTR(override, S_IWUSR | S_IRUGO, qfprom_override_show,
		   qfprom_override_store);


static struct attribute *qfprom_attributes[] = {
	&dev_attr_qfusing.attr,
	&dev_attr_addr.attr,
	&dev_attr_lsb.attr,
	&dev_attr_msb.attr,
	&dev_attr_enable.attr,
	&dev_attr_write.attr,
	&dev_attr_read.attr,
	&dev_attr_override.attr,
	NULL
};

static const struct attribute_group qfprom_attribute_group = {
	.attrs = qfprom_attributes,
};

/* We cant access qfporm address range 0x70xxxxx using qfuse_single_read_row api
 * so we read the range using io read 
 */
u32 qfprom_secondary_hwkey_status(void)
{
	void __iomem *key_status_addr;
	u32 hw_key_status;

	key_status_addr = ioremap(QFPROM_HW_KEY_STATUS, sizeof(u32));
	hw_key_status = (u32) readl(key_status_addr);
	iounmap(key_status_addr);
	printk("%s: hw_key_status = 0x%x\n", __func__, hw_key_status);
	return hw_key_status;
}

#if RANDOM_BY_TZBSP
/* Create random key(8byte size) by tzbsp
 * if return value =0, error to create random key by tzbsp 
 * if return value >0, success to create random key by bzbsp
 */
int qfprom_create_random(u32 *value)
{
	int ret;
	u32 rand = 0;
	struct prng_data {
		u32 r;
		u32 s;
	} pdata;
	u8 *p_buf = NULL;

	p_buf = kmalloc(sizeof(u8) * 4, GFP_KERNEL);
	if (!p_buf) {
		printk("%s: memory alloc fail\n", __func__);
		ret = -EINVAL;
		goto err;
	}
	memset(p_buf , 0 , sizeof(u8) * 4 );

	pdata.r = virt_to_phys((void *)p_buf);
	pdata.s = 4;

	ret = scm_call(QFPROM_SVC_ID, QFPROM_PRNG_CMD, &pdata, sizeof(pdata),
		     NULL, 0);
	if (ret < 0) {
		printk("%s: scm call error for creating random\n", __func__);
		goto err;
	}
	rand =
	    (p_buf[0] << 24) | (p_buf[1] << 16) | (p_buf[2] << 8) | (p_buf[3]);
	printk("%s: random = 0x%x\n", __func__, rand);

	*value = rand;
	
err:
	if(p_buf != NULL )
		kfree(p_buf);
	return ret;

}
#endif

int qfprom_blow_secondary_hwkey_region(void)
{
	int ret;
	u32 addr, lsb, msb;
           u32 key_status = 0;
	int i;
	u32* p_buf=NULL;

	p_buf = kmalloc(sizeof(u32)*2, GFP_KERNEL);
	if( !p_buf ){
		printk("%s : buffer memory alloc fail\n",__func__);
		return -ENOMEM;
	}

	memset(p_buf, 0, sizeof(u32)*2);


           key_status = qfprom_secondary_hwkey_status();

           if ((key_status & SEC_HW_KEY_BLOWN) == SEC_HW_KEY_BLOWN) {
                                printk("%s: Already Blown\n",__func__);
                                ret = 0;
                     goto err;
           }

	addr = QFPROM_SECONDARY_HW_KEY;
	for(i=0;i<7;i++){
		/* we can read hw secondary key region because before read permission is set */
		/*
		ret = qfuse_read_single_row(addr, 0, p_buf);
		if(ret!=0){
			printk("%s: qfuse addr %x read fail, ret=%d\n",__func__,addr,ret);
			ret = -EINVAL;
			goto err;
		}
		printk("%s:Currently, secondary key addr=0x%x, lsb=0x%x, msb=0x%x\n",__func__,addr,p_buf[0],p_buf[1]);
		*/
		msleep(10);

		/* if you have not written ever hw key before, value to read will be zero
		 * so create random using tzbsp 
		 * LSB region */
		if(!p_buf[0]){
			
			if(qfprom_create_random((u32*)&lsb)) {
				ret = -EINVAL;
				goto err;
			}
			msleep(5);
		} else {
			/* dont create random value and rewrite value to read */
			printk("hw lsb key was blow already in 0x%x addr\n",addr);
			lsb = p_buf[0];
		}
		/* if you have not written ever hw key before, value to read will be zero
		 * so create random using tzbsp 
		 * MSB region */
		if(!p_buf[1]) {
			if(qfprom_create_random((u32*)&msb)) {
				ret=-EINVAL;
				goto err;
			}
			msleep(5);
		} else {

			printk("hw msb key was blow already in 0x%x addr\n",addr+4);
			msb = p_buf[1];
		}
		/* must mask FEC bit */
		lsb = lsb&HW_KEY_LSB_FEC_MASK;
		msb = msb&HW_KEY_MSB_FEC_MASK;
		printk("We start to writing secondary key !!!\n");
		printk("addr=0x%x, lsb=0x%x, msb=0x%x\n",addr,lsb,msb);
		msleep(10);
		ret = qfuse_write_single_row(addr,lsb,msb);
		if(ret!=0) {
			printk("%s:qfus addr 0x%x write error, ret=%d\n",__func__,addr,ret);
			ret=-EINVAL;
			goto err;
		}
		else {
			/* write double check routine */
			printk("hw secondary key write successful\n");
			msleep(10);
			/*
			ret = qfuse_read_single_row(addr,0,p_buf);
			if(ret!=0){
				printk("%s:read fail when double check routine, ret=%d\n",__func__,ret);
				goto err;
			}  
			if((p_buf[0]==lsb)&&(p_buf[1]==msb))
				printk("%s: hw key write double check successful!!!!!!!\n",__func__);
			else {
				printk("%s: hw key double check error, read_lsb=0x%x,read_msb=0x%x\n",__func__,p_buf[0],p_buf[1]);
				ret=-EINVAL;
				goto err;
			}
			*/
		}
		addr=addr+8;
		msleep(10);
	}

err:
	kfree(p_buf);
	return ret;
}

/* if return value == 0, success 
 * if return value < 0, scm call fail
 * if return value > 0, status error to write qfprom
 * This API can use in range 0x700XXX
 */
int qfuse_write_single_row(u32 fuse_addr, u32 fuse_lsb, u32 fuse_msb)
{

	struct qfprom_write_cmd_buffer request;
	u32 *p_buf = NULL;
	u32 *p_status = NULL;
	u32 scm_ret = 0;
	int ret = 0;
	u32 tzbsp_boot_milestone_status = TZBSP_MILESTONE_TRUE;

	p_buf = kmalloc(sizeof(u32) * 2, GFP_KERNEL);
	p_status = kmalloc(sizeof(u32), GFP_KERNEL);

	if (!p_buf) {
		printk("%s : buffer memory alloc fail\n", __func__);
		ret = -ENOMEM;
		goto error_p_status;
	}
	if (!p_status) {
		printk("%s : status memory alloc fail\n", __func__);
		ret = -ENOMEM;
		goto error_p_status;
	}


	memset(p_buf, 0, sizeof(u32)*2);
	memset(p_status, 0, sizeof(u32));

	p_buf[0] = fuse_lsb;
	p_buf[1] = fuse_msb;
	request.qfprom_addr = fuse_addr;
	request.buf = virt_to_phys((void *)p_buf);
	request.qfprom_status = virt_to_phys((void *)p_status);
	request.qfprom_clk = QFPROM_CLOCK;

	tzbsp_boot_milestone_status = TZBSP_MILESTONE_FALSE;
	if (TZBSP_MILESTONE_FALSE !=
	    scm_call(TZBSP_SVC_OEM, TZBSP_ADDITIONAL_CMD,
		     &tzbsp_boot_milestone_status,
		     sizeof(tzbsp_boot_milestone_status), &scm_ret,
		     sizeof(scm_ret))) {
		printk("change failed to %x ,  status = %x\n",
		       TZBSP_MILESTONE_FALSE, tzbsp_boot_milestone_status);
		ret = -EMILE;
		goto error_p_status;
		
	}else {
		printk("change succeeded to %x ,  status = %x\n", TZBSP_MILESTONE_FALSE,
		       tzbsp_boot_milestone_status);
	}
	
	msleep(10);
	ret = scm_call(QFPROM_SVC_ID, QFPROM_WRITE_CMD, &request,
			sizeof(request), &scm_ret, sizeof(scm_ret));
	if (ret < 0) {
		goto error_p_status;
	}
	ret = *((u32 *) phys_to_virt(request.qfprom_status));
	printk("%s: qfprom_status = 0x%x\n", __func__, ret);


	tzbsp_boot_milestone_status = TZBSP_MILESTONE_TRUE;
	msleep(10);
	if (TZBSP_MILESTONE_TRUE !=
	    scm_call(TZBSP_SVC_OEM, TZBSP_ADDITIONAL_CMD,
		     &tzbsp_boot_milestone_status,
		     sizeof(tzbsp_boot_milestone_status), &scm_ret,
		     sizeof(scm_ret))) {
		printk("change failed to %x ,  status = %x\n",
		       TZBSP_MILESTONE_TRUE, tzbsp_boot_milestone_status);
		ret = -EMILE;
		goto error_p_status;
	}else {
		printk("change succeeded to %x ,  status = %x\n", TZBSP_MILESTONE_TRUE,
		       tzbsp_boot_milestone_status);
	}

error_p_status:
	if(p_buf != NULL)
		kfree(p_buf);
	if(p_status != NULL)
		kfree(p_status);	
	return ret;
}

/* if return value == 0, success 
 * if return value < 0, scm call fail
 * if return value > 0, status error to read qfprom
 * This API can use in range 0x700XXX
 */
int qfuse_read_single_row(u32 fuse_addr, u32 addr_type, u32 * r_buf)
{
	struct qfprom_read_cmd_buffer request;
	u32 *p_status = NULL;
	u32 scm_ret = 0;
	int ret = 0;
	u32 tzbsp_boot_milestone_status = TZBSP_MILESTONE_TRUE;

	p_status = kmalloc(sizeof(u32), GFP_KERNEL);
	if (!p_status) {
		printk("%s : status memory alloc fail\n", __func__);
		ret = -ENOMEM;
		goto error_p_status;
	}

	memset(p_status, 0, sizeof(u32));
	request.qfprom_addr = fuse_addr;
	request.qfprom_addr_type = addr_type;
	request.read_buf = virt_to_phys((void *)r_buf);
	request.qfprom_status = virt_to_phys((void *)p_status);

	tzbsp_boot_milestone_status = TZBSP_MILESTONE_FALSE;
	if (TZBSP_MILESTONE_FALSE !=
	    scm_call(TZBSP_SVC_OEM, TZBSP_ADDITIONAL_CMD,
		     &tzbsp_boot_milestone_status,
		     sizeof(tzbsp_boot_milestone_status), &scm_ret,
		     sizeof(scm_ret))) {
		printk("change failed to %x ,  status = %x\n",
		       TZBSP_MILESTONE_FALSE, tzbsp_boot_milestone_status);
		ret = -EMILE;
		goto error_p_status;
	}else {
		printk("change succeeded to %x ,  status = %x\n", TZBSP_MILESTONE_FALSE,
	       		tzbsp_boot_milestone_status);
	}
	
	msleep(10);
	ret = scm_call(QFPROM_SVC_ID, QFPROM_READ_CMD, &request,
			sizeof(request), &scm_ret, sizeof(scm_ret));

	if (ret < 0) {
		printk("%s: scm call fail\n", __func__);
		goto error_p_status;
	}
	ret = *((u32 *) phys_to_virt(request.qfprom_status));
	printk("%s: qfprom_status = 0x%x\n", __func__, ret);

	tzbsp_boot_milestone_status = TZBSP_MILESTONE_TRUE;
	msleep(10);
	if (TZBSP_MILESTONE_TRUE !=
	    scm_call(TZBSP_SVC_OEM, TZBSP_ADDITIONAL_CMD,
		     &tzbsp_boot_milestone_status,
		     sizeof(tzbsp_boot_milestone_status), &scm_ret,
		     sizeof(scm_ret))) {
		printk("change failed to %x ,  status = %x\n",
		       TZBSP_MILESTONE_TRUE, tzbsp_boot_milestone_status);
		ret = -EMILE;
		goto error_p_status;
	}else {
		printk("change succeeded to %x ,  status = %x\n", TZBSP_MILESTONE_TRUE,
		       tzbsp_boot_milestone_status);
	}

error_p_status:
	if(p_status!=NULL)
		kfree(p_status);
	return ret;
}

static int __devexit lge_qfprom_interface_remove(struct platform_device *pdev)
{
	return 0;
}

static int __init lge_qfprom_probe(struct platform_device *pdev)
{
	int err;
	err = sysfs_create_group(&pdev->dev.kobj, &qfprom_attribute_group);
	if (err < 0)
		printk("%s: cant create attribute file\n", __func__);
	return err;
}

static struct platform_driver lge_qfprom_driver __refdata = {
	.probe = lge_qfprom_probe,
	.remove = __devexit_p(lge_qfprom_interface_remove),
	.driver = {
		.name = LGE_QFPROM_INTERFACE_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init lge_qfprom_interface_init(void)
{
	return platform_driver_register(&lge_qfprom_driver);
}

static void __exit lge_qfprom_interface_exit(void)
{
	platform_driver_unregister(&lge_qfprom_driver);
}

module_init(lge_qfprom_interface_init);
module_exit(lge_qfprom_interface_exit);

MODULE_DESCRIPTION("LGE QFPROM interface driver");
MODULE_AUTHOR("Taehung <taehung.kim@lge.com>");
MODULE_LICENSE("GPL");
