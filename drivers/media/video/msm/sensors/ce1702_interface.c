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
//                                      
#include "ce1702_interface.h"

static int ce1702_spi_probe(struct spi_device *spi);
static int ce1702_spi_remove(struct spi_device *spi);
static int ce1702_spi_suspend(struct spi_device *spi, pm_message_t mesg);
static int ce1702_spi_resume(struct spi_device *spi);

static struct spi_device *ce1702_spi = NULL;

static struct wake_lock ce1702_wake_lock;
//static struct wake_lock ce1702_idle_lock;

static uint8_t ISP_BIN_DOWN_FLAG = 0; //0 = start, 1 = download done, 2 = download error

static struct spi_driver ce1702_spi_driver = {
	.probe = ce1702_spi_probe,
	.remove	= __devexit_p(ce1702_spi_remove),
	.suspend = ce1702_spi_suspend,
	.resume  = ce1702_spi_resume,
	.driver = {
		.name = SPI_DRIVER_NAME,
		.bus	= &spi_bus_type,
		.owner = THIS_MODULE,
	},
};

struct msm_sensor_ctrl_t* ce1702_s_interface_ctrl;

static DEFINE_MUTEX(lock);

static unsigned char tx_data[10];
static unsigned char tdata_buf[40] = {0};
static unsigned char rdata_buf[8196] = {0};
int dest_location_firmware = CE1702_NANDFLASH; //CE1702_SDCARD2;    // system , sdcard
int ce1702_irq_log;

//                                      

//inline void init_suspend(void)
void init_suspend(void)
{
    LDBGI("init_suspend \n");
	wake_lock_init(&ce1702_wake_lock, WAKE_LOCK_SUSPEND, "ce1702");
//	wake_lock_init(&ce1702_idle_lock, WAKE_LOCK_IDLE, "ce1702_idle");
}

//inline void deinit_suspend(void)
void deinit_suspend(void)
{
    LDBGI("deinit_suspend \n");
	wake_lock_destroy(&ce1702_wake_lock);
//	wake_lock_destroy(&ce1702_idle_lock);
}

//inline void prevent_suspend(void)
void prevent_suspend(void)
{
    LDBGI("prevent_suspend \n");
	wake_lock(&ce1702_wake_lock);
//	wake_lock(&ce1702_idle_lock);
}

//inline void allow_suspend(void)
void allow_suspend(void)
{
    LDBGI("allow_suspend \n");
	wake_unlock(&ce1702_wake_lock);
//	wake_unlock(&ce1702_idle_lock);
}

static int ce1702_firmware_show(struct device* dev, struct device_attribute* attr, char* buf)
{
#if 0
	return sprintf(buf,"Current version %d.%d\n",
			firmware_ver_major, firmware_ver_minor);
#else
    return sprintf(buf,"ce1702 firmware location is %s\n", dest_location_firmware == 0 ? "NANDFLASH" : "SDCARD");
#endif

}
static int ce1702_firmware_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t n)
{
	int val;
//	long rc;

	sscanf(buf,"%x",&val);

	switch (val)
	{
		case CE1702_NANDFLASH:
			dest_location_firmware = CE1702_NANDFLASH;
			break;
		case CE1702_SDCARD2:
			dest_location_firmware = CE1702_SDCARD2;
			break;
		default:
			LDBGE("ce1702: invalid input\n");
			return 0;
	}
    LDBGI("ce1702_firmware_store is %d !! \n", dest_location_firmware);
	return n;
}

static int ce1702_isp_event_show(struct device* dev, struct device_attribute* attr, char* buf)
{
    return sprintf(buf,"%s is not supported !! \n", __func__);
}

static int ce1702_isp_event_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t n)
{
	int val;
	struct timespec time;
	struct tm tm_result;
	uint8_t filename[128];

	time = __current_kernel_time();
	time_to_tm(time.tv_sec, sys_tz.tz_minuteswest * 60 * (-1), &tm_result);

	sprintf(filename, "/data/event_%02d%02d%02d%02d%02d.bin", tm_result.tm_mon + 1, tm_result.tm_mday, tm_result.tm_hour, tm_result.tm_min, tm_result.tm_sec);

	sscanf(buf,"%x", &val);
	if (val) {
		int32_t rc, num_read;
		uint8_t data[130];
		uint8_t packet_size[2] = {0, };

		rc = ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0xDB, NULL, 0, packet_size,  2);
		if (rc < 0)
		{
			LDBGE("%s: num_read reading error\n", __func__);
			return n;
		}
		num_read = packet_size[0] + (packet_size[1] << 8);
		LDBGE("%s: num_read : %d, [%x|%x]\n", __func__, num_read, packet_size[0], packet_size[1]);
		if (num_read > 0) {
			mm_segment_t old_fs;
			uint8_t *dump_data;
			int32_t dump_data_len, nloop, fd;

			dump_data_len = num_read * 128 * sizeof(uint8_t);
			dump_data = kmalloc(dump_data_len, GFP_KERNEL);
			for (nloop = 0; nloop < num_read; nloop++)
			{
				rc = ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0xDC, NULL, 0, data, 130);
				if (rc < 0) {
					LDBGE("%s: %d-th reading error\n", __func__, nloop);
					continue;
				}
				memcpy(&dump_data[nloop * 128], &data[2], sizeof(uint8_t) * 128);

			}
			old_fs = get_fs();
			set_fs(KERNEL_DS);
			fd = sys_open(filename, O_RDWR | O_CREAT , 0644);
			if (fd >= 0) {
				sys_write(fd, dump_data, dump_data_len);
				sys_close(fd);
				LDBGE("%s: isp eventlog (%s) successfully writed (%d)!! \n", __func__, filename, nloop);
			}
			set_fs(old_fs);
			kfree(dump_data);

		}
	}

	return n;
}

static int ce1702_fps_show(struct device* dev, struct device_attribute* attr, char* buf)
{
    return sprintf(buf,"%s is not supported yet!! \n", __func__);
}


static int ce1702_irq_log_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t n)
{
	int val;

	sscanf(buf,"%x",&val);
	ce1702_irq_log = val;

	return n;
}

static int ce1702_irq_log_show(struct device* dev, struct device_attribute* attr, char* buf)
{
    return sprintf(buf,"CE1702 irq log is %s !! \n", ce1702_irq_log == 0 ? "deactivated" : "activated");
}


static DEVICE_ATTR(firmware, S_IRUSR|S_IWUSR, ce1702_firmware_show, ce1702_firmware_store);
static DEVICE_ATTR(isp_event, S_IRUSR|S_IWUSR, ce1702_isp_event_show, ce1702_isp_event_store);
static DEVICE_ATTR(fps, S_IRUSR|S_IWUSR, ce1702_fps_show, NULL);
static DEVICE_ATTR(irq_log, S_IRUSR|S_IWUSR, ce1702_irq_log_show, ce1702_irq_log_store);

static struct attribute* ce1702_sysfs_attrs[] = {
	&dev_attr_firmware.attr,
	&dev_attr_isp_event.attr,
	&dev_attr_fps.attr,
	&dev_attr_irq_log.attr,
	NULL
};

void ce1702_sysfs_add(struct kobject* kobj)
{
	int i, n, ret;
	n = ARRAY_SIZE(ce1702_sysfs_attrs);
	for(i = 0; i < n; i++)
	{
		if(ce1702_sysfs_attrs[i])
		{
			ret = sysfs_create_file(kobj, ce1702_sysfs_attrs[i]);
			if(ret < 0)
				LDBGE("ce1702_sysfs_attrs sysfs is not created\n");
		}
	}
}

void ce1702_sysfs_rm(struct kobject* kobj)
{
	int i, n;

	n = ARRAY_SIZE(ce1702_sysfs_attrs);
	for(i = 0; i < n; i++)
	{
		if(ce1702_sysfs_attrs[i])
			sysfs_remove_file(kobj,ce1702_sysfs_attrs[i]);
	}
}

#if 0
static int ce1702_init_client(struct i2c_client *client)
{
	/* Initialize the MSM_CAMI2C Chip */
	init_waitqueue_head(&ce1702_wait_queue);
	return 0;
}
#endif

void ce1702_store_isp_eventlog(void)
{
	const char *buf = "1";

	ce1702_isp_event_store(NULL, NULL, buf, 1);
}

static int32_t ce1702_i2c_txdata(unsigned short saddr,
	unsigned char *txdata, int length)
{
	uint16_t ssaddr = saddr >> 1;
	struct i2c_msg msg[] = {
		{
			.addr = ssaddr,
//			.addr = saddr,
			.flags = 0,
			.len = length,
			.buf = txdata,
		},
	};

	if (i2c_transfer(ce1702_s_interface_ctrl->sensor_i2c_client->client->adapter, msg, 1) < 0) {
		LDBGE("ce1702_i2c_txdata failed\n");
		return -EIO;
	}

	return CE1702_OK;
}

int32_t ce1702_i2c_write(unsigned short saddr,
	unsigned short waddr, unsigned char *wdata, uint32_t length)
{
	int32_t rc = -EIO;
	uint8_t *buf, small_buf[16];
	uint32_t wlength = length + 1;

	if(wlength <= 16)
		buf = small_buf;
	else
		buf = kmalloc(wlength, GFP_KERNEL);

#if 0
	if(buf)
	{
		memset(buf, 0x00, wlength);
		buf[0] = waddr;
	}
	else
	{
		LDBGE("%s : mem alloc fail !! \n",__func__);
		kfree(buf);
		return -EIO;
	}
#endif
	buf[0] = (uint8_t)waddr;
	if(length) //size is not 0
	{
		memcpy(buf + 1, wdata, length);
#ifdef PRINT_CAM_REG
		for(i=0;i<wlength;i++)
		{
			LDBGE("i2c write buf[%d]=0x%x \n",i,buf[i]);
		}
#endif
	}

	rc = ce1702_i2c_txdata(saddr, buf, wlength);

	if (rc < 0)
		LDBGE("i2c_write failed, addr = 0x%x, val = 0x%x!\n",waddr, wdata != NULL ? wdata[0] : 0x00);
	if (wlength > 16)
		kfree(buf);
	return rc;
}

static int ce1702_i2c_rxdata(unsigned short saddr, unsigned char *cmd_data, int cmd_len,
	unsigned char *rxdata, int length)
{
	uint16_t ssaddr = saddr >> 1;
	struct i2c_msg msgs[] = {
	{
		.addr   = ssaddr,
//		.addr   = saddr,
		.flags = 0,
		.len   = cmd_len,
		.buf   = cmd_data,
	},
	{
		.addr   = ssaddr,
//		.addr   = saddr,
		.flags = I2C_M_RD,
		.len   = length,
		.buf   = rxdata,
	},
	};

	if (i2c_transfer(ce1702_s_interface_ctrl->sensor_i2c_client->client->adapter, msgs, 2) < 0) {
		LDBGE("ce1702_i2c_rxdata failed!\n");
		return -EIO;
	}

	return 0;
}

int32_t ce1702_i2c_read(unsigned short   saddr,
	unsigned char cmd_addr, unsigned char *cmd_data, int cmd_len, unsigned char *rdata, int length)
{
	int32_t rc = 0;
	uint8_t *cmd_buf;
	int cmd_buf_count;

	//LDBGI(">>>>ce1702_i2c_read : cmd_addr=0x%x, cmd_data=0x%x, cmd_len=%d \n",cmd_addr, cmd_data[0],cmd_len);

	cmd_buf_count = cmd_len+1;
	cmd_buf = kmalloc(cmd_buf_count, GFP_KERNEL);
	if(cmd_buf)
	{
		memset(cmd_buf, 0x00, cmd_buf_count);
		cmd_buf[0] = cmd_addr;
	}
	else
	{
		LDBGE("%s : mem alloc fail !! \n",__func__);
		kfree(cmd_buf);
		return -EIO;
	}

	if(cmd_len)
	{
		memcpy(cmd_buf+1,cmd_data,cmd_len);
	}

	rc = ce1702_i2c_rxdata(saddr, cmd_buf, cmd_buf_count, rdata, length);
	if(rc<0)
	{
		LDBGE("i2c_read failed, addr = 0x%x, val = 0x%x!\n",cmd_buf[0], rdata[0]);
	}

	kfree(cmd_buf);
	return rc;
}

/*===========================================================================

FUNCTION      CE_FwStart
DESCRIPTION
DEPENDENCIES
  None
RETURN VALUE
  None
SIDE EFFECTS
  None

===========================================================================*/
void CE_FwStart(void)
{
	uint8_t data[1], FW_cur_ver[5];
	int retry_cnt;

	LDBGI("%s: called[%lu]\n", __func__, jiffies);
	data[0] = 0x0;
	ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, 0xF0, data, 0);
	mdelay(280);	// FW start delay 1000
	//mdelay(500);	// FW start delay 1000

	// Check if there's enough deley
	retry_cnt = 0;
	do
	{
		if(ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x00, data, 1, FW_cur_ver,4)>=0) {
			LDBGI("%s: ISP FW is Started successfully! !!!!\n", __func__);
			break;
		}
		LDBGE("ISP FW is not ready yet !!!!!retry = %d\n", retry_cnt);
		retry_cnt++;
		mdelay(10);
		//mdelay(30);
	} while (retry_cnt <70);
	/* We might need 700ms to start FW. so 70 count times 10 is 700ms and it seems good enough
	     Comment by jungki.kim */
}

/*===========================================================================

FUNCTION      CE_FwUpload
DESCRIPTION
DEPENDENCIES
  None
RETURN VALUE
  None
SIDE EFFECTS
  None

===========================================================================*/
static long CE_FwUpload(uint8_t *ubFileName, uint8_t ubCmdID, uint8_t ubWait)
{
	int fd;
	int rc=0;
	uint32_t file_size;
	int index = 0;
	uint8_t *img_buf = NULL;
	uint8_t frame[129];
	uint8_t fs_name_buf[256];
//	int i;
//	uint8_t data;

	//Flash Burning using I2C
	mm_segment_t old_fs = get_fs();
	set_fs(KERNEL_DS);

	LDBGI("CE_FwUpload: burning firwmare from %d : %s....\n", dest_location_firmware, ubFileName);

	switch(dest_location_firmware)
	{
		case CE1702_NANDFLASH : //system
			sprintf(fs_name_buf,"/sbin/%s",ubFileName);
//			sprintf(fs_name_buf,"/system/media/camera/fw/%s",ubFileName);
			break;

		case CE1702_SDCARD2: //sdcard
			sprintf(fs_name_buf,"/sdcard/%s",ubFileName);
//			sprintf(fs_name_buf,"/storage/external_SD/%s",ubFileName);
			break;

		default :
			break;
	}

	fd = sys_open(fs_name_buf, O_RDONLY, 0);
	if (fd < 0) {
		LDBGE("%s: Can not open %s[%d]\n",__func__, fs_name_buf, fd);
		rc =  -ENOENT;
		set_fs(old_fs);
		goto fwupload_fail;
	}

	file_size = (unsigned)sys_lseek(fd, (off_t)0, 2);
	if (file_size == 0) {
		LDBGE("%s: File size is 0\n",__func__);
		rc = -EIO;
		sys_close(fd);
		set_fs(old_fs);
		goto fwupload_fail;
	}
	sys_lseek(fd, (off_t)0, 0);

	LDBGI("CE1702: firmware file size %d , %u\n", fd, file_size);

	img_buf = kmalloc(file_size, GFP_KERNEL);
	if (!img_buf) {
		LDBGE("%s: Can not alloc data\n", __func__);
		rc = -ENOMEM;
		sys_close(fd);
		set_fs(old_fs);
		goto fwupload_fail;
	}
	else
	{
		memset(img_buf, 0x00, file_size);
	}

	// XXX: MUST: file_size <= CE1702_FLASH_BIN00_FILE
	if ((unsigned)sys_read(fd, (char __user *)img_buf, file_size) != file_size)
	{
		rc = -ENOMEM;
		sys_close(fd);
		set_fs(old_fs);
		kfree(img_buf);

		goto fwupload_fail;
	}

	LDBGI("CE1702: success in reading firmware - img_buf=0x%x\n",img_buf[2]);

	sys_close(fd);
	set_fs(old_fs);

	if (ubCmdID == 0xF4)
	{
		while (file_size > 0)
		{
			int frame_size;

			if (file_size >= 129)
			{
				frame_size = 129;
			}
			else
			{
				frame_size = file_size;
				memset(frame, 0x00, 129);
				LDBGE(">>> not in here......\n");
			}

			memcpy(frame, &img_buf[index], frame_size);
			file_size -= frame_size;
			index += frame_size;

			rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, ubCmdID, frame, 129);
			if(rc < 0)
			{
				sys_close(fd);
				set_fs(old_fs);
				kfree(img_buf);
				goto fwupload_fail;
			}

		}
	}
	else
	{
		LDBGI(">>> I2C write 0xF2 -- %d\n ", file_size);
		rc = ce1702_i2c_write(ce1702_s_interface_ctrl->sensor_i2c_addr, ubCmdID, img_buf, file_size);
		if(rc<0)
		{
			sys_close(fd);
			set_fs(old_fs);
			kfree(img_buf);
			goto fwupload_fail;
		}

	}

	kfree(img_buf);

	return rc;

fwupload_fail :
	LDBGE("CE_FwUpload : fwupload fail !!\n");
	/*                                                                                        */
	dump_stack();
	BUG_ON(1);
	/*                                                                                      */
	return rc ;

}


/*===========================================================================

FUNCTION      ce1702_sony_check_flash_version
DESCRIPTION
DEPENDENCIES
  None
RETURN VALUE
  None
SIDE EFFECTS
  None

===========================================================================*/
long ce1702_check_flash_version(void)
{
	uint8_t *version_file;
	uint32_t file_size;
	int i, retry_cnt;
	int fd;

	int rc = 0;

	uint8_t cmd_data;
	uint8_t FW_cur_ver[4] ={0x00, 0x00, 0x00, 0x00};
	uint8_t AF_cur_ver[4] ={0x00, 0x00, 0x00, 0x00};
	uint8_t AWB_cur_ver[4] ={0x00, 0x00, 0x00, 0x00};
	uint8_t fs_name_buf[256];

	mm_segment_t old_fs = get_fs();
	set_fs(KERNEL_DS);

	LDBGI("ce1702_sony_check_flash_version ==== ISP and Bin Version Check !!\n");
	CE_FwStart();

	LDBGI("ce1702_sony_check_flash_version ==== ce1702_s_interface_ctrl->sensor_i2c_addr == 0x%x!!\n",ce1702_s_interface_ctrl->sensor_i2c_addr);

	cmd_data = 0x00;
	ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x00, &cmd_data, 1, FW_cur_ver,4);
	LDBGI(">>>>>>>>>>>> FW Version check 1 = 0x%x\n", FW_cur_ver[0]);
	LDBGI(">>>>>>>>>>>> FW Version check 2 = 0x%x\n", FW_cur_ver[1]);
	LDBGI(">>>>>>>>>>>> FW Version check 3 = 0x%x\n", FW_cur_ver[2]);
	LDBGI(">>>>>>>>>>>> FW Version check 4 = 0x%x\n", FW_cur_ver[3]);

	cmd_data = 0x05;
	ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x00, &cmd_data, 1, AF_cur_ver,4);
	LDBGI(">>>>>>>>>>>> AF Version check 1 = 0x%x\n", AF_cur_ver[0]);
	LDBGI(">>>>>>>>>>>> AF Version check 2 = 0x%x\n", AF_cur_ver[1]);
	LDBGI(">>>>>>>>>>>> AF Version check 3 = 0x%x\n", AF_cur_ver[2]);
	LDBGI(">>>>>>>>>>>> AF Version check 4 = 0x%x\n", AF_cur_ver[3]);

	cmd_data = 0x06;
	ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0x00, &cmd_data, 1, AWB_cur_ver,4);
	LDBGI(">>>>>>>>>>>> AWB Version check 1 = 0x%x\n", AWB_cur_ver[0]);
	LDBGI(">>>>>>>>>>>> AWB Version check 2 = 0x%x\n", AWB_cur_ver[1]);
	LDBGI(">>>>>>>>>>>> AWB Version check 3 = 0x%x\n", AWB_cur_ver[2]);
	LDBGI(">>>>>>>>>>>> AWB Version check 4 = 0x%x\n", AWB_cur_ver[3]);

	switch(dest_location_firmware)
	{
		case CE1702_NANDFLASH : //system
			sprintf(fs_name_buf,"/sbin/%s",CE1702_FLASH_BIN02_FILE);
//			sprintf(fs_name_buf,"/system/media/camera/fw/%s",CE1702_FLASH_BIN02_FILE);
			break;

		case CE1702_SDCARD2: //sdcard
			sprintf(fs_name_buf,"/sdcard/%s",CE1702_FLASH_BIN02_FILE);
//			sprintf(fs_name_buf,"/storage/external_SD/%s",CE1702_FLASH_BIN02_FILE);
			break;

		default :
			break;
	}

	retry_cnt = 0;
	do {
		retry_cnt++;
		LDBGI("ce1702_check_flash_version in a file open loop !!!!\n");

		fd = sys_open(fs_name_buf, O_RDONLY, 0);

		if( fd >= 0 ) break ;
		mdelay(3000);
	} while(retry_cnt < 20);

	if (fd < 0) {
		LDBGE("%s: Can not open %s[%d]\n",__func__, fs_name_buf, fd);
		set_fs(old_fs);
		return -ENOENT;
	}

	file_size = (unsigned)sys_lseek(fd, (off_t)0, SEEK_END);
	if (file_size == 0 || file_size < 8) {
		LDBGE("%s: %s file size[%d] mismatching error !!!\n", __func__, CE1702_FLASH_BIN02_FILE, file_size);
		rc = -EIO;
		sys_close(fd);
		goto version_fail;
	}
	sys_lseek(fd, (off_t)0, SEEK_SET);

	LDBGI("CE1702: CE1702_FLASH_BIN02_FILE file size %u\n", file_size);

	version_file = kmalloc(file_size, GFP_KERNEL);
	memset(version_file, 0x00, file_size);

	// XXX: MUST: file_size <= CE1702_FLASH_BIN00_FILE

	rc = sys_read(fd, version_file, file_size);
	if(rc <0)
	{
		sys_close(fd);
		kfree(version_file);
		goto version_fail;
	}

	LDBGI("CE1702: success in reading firmware\n");

	for( i =4; i<file_size;i++)
	{
		LDBGI(">>>>>>>>>>>> ISP FILE Version check[%d] = %x\n", i - 4, version_file[i]);
	}

	if ((FW_cur_ver[0] == version_file[4]) && (FW_cur_ver[1] == version_file[5]) && (FW_cur_ver[2] == version_file[6]) && (FW_cur_ver[3] == version_file[7]))
	{
		LDBGI("========== ISP Version Check OK(No need ISP update)!! !!==========\n");
		rc = 0;
	}
	else
	{
		LDBGE(">>>>>>>>>>>> Lower FW Version !!>>>>>>>>>>>>\n");
		rc = -EIO;
	}

	sys_close(fd);
	set_fs(old_fs);
	kfree(version_file);

	return rc;

version_fail :
	set_fs(old_fs);
	return rc;
}

/*===========================================================================

FUNCTION      ce142_sony_isp_fw_full_upload
DESCRIPTION
DEPENDENCIES
  None
RETURN VALUE
  None
SIDE EFFECTS
  None

===========================================================================*/
long ce1702_isp_fw_full_upload(void)
{
	long rc = 0;
	uint8_t rdata[2];
	uint8_t res;
	int from_chcek_cnt = 0;

	LDBGI("%s: called[%lu]\n", __func__, jiffies);

	//[2-1] ISP FW uploader
	if(CE_FwUpload(CE1702_FLASH_BIN00_FILE, 0xF2, 100)==0)
	{
		prevent_suspend(); //                                            

		mdelay(100);
		rc = CE_FwUpload(CE1702_FLASH_BIN01_FILE, 0xF4, 100);
		if(rc < 0)
			goto full_upload_fail;

		//[2-2] F5 check (ISP FW uploader)
		//camera_timed_wait(1000);
		mdelay(100);
		res = 0xFF;
		rdata[0] = 0x00;
		rc = ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0xF5, rdata, 0, &res, 1);
		if(rc < 0)
			goto full_upload_fail;
		LDBGI(">>> First F5 check = %x, res = %d,  rc=%ld\n", res, res, rc);

		//[2-3] NV update (check 1)
		if(res==0x5)
		{
			LDBGI(">>>>>>>	NV update check [1]:::: nv_data = %x \n", res);
		}else
		{
			LDBGE(">>>>>>>	NV update check [1] ::: ISP FW uploader upload fail \n");
			rc = -EIO;
			goto full_upload_fail;
		}

		//[3-1] ISP camera FW
		mdelay(100);
		rc = CE_FwUpload(CE1702_FLASH_BIN02_FILE, 0xF2, 100);
		if(rc < 0)
			goto full_upload_fail;
		mdelay(100);
		rc = CE_FwUpload(CE1702_FLASH_BIN03_FILE, 0xF4, 100);
		if(rc < 0)
			goto full_upload_fail;


		//[3-2]  F5 check (ISP camera FW)
		mdelay(100);
		res = 0xFF;
		rdata[0] = 0x00;
		rc = ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0xF5, rdata, 0, &res, 1);
		if(rc < 0)
			goto full_upload_fail;
		LDBGI(">>> Second F5 check start = %x \n", res);

		//[3-3] NV update (check 2)
		if(res==0x5)
		{
			LDBGI(">>>>>>>	NV update check [2]:::: nv_data = %x \n", res);
		}else
		{
			LDBGE(">>>>>>>	NV update check [2] ::: ISP camera FW upload fail \n");
			rc = -EIO;
			goto full_upload_fail;
		}
		mdelay(9 * 1000);	//FROM Write Time waiting

		//[4-1] FROM write check
		from_chcek_cnt = 0;
		do{
			mdelay(100);
			rdata[0] = 0x00;
			rc = ce1702_i2c_read(ce1702_s_interface_ctrl->sensor_i2c_addr, 0xF5, rdata, 0, &res, 1);
			if(rc < 0)
				goto full_upload_fail;
			LDBGI(">>> Second F5 check end [count %d x 100ms]  = %x \n", from_chcek_cnt, res);
			from_chcek_cnt++;
		}while((res != 0x06)&& (from_chcek_cnt < 500));

		//[4-2] NV update (check 3)
		if(res==0x06)
		{
			LDBGI(">>>>>>>	NV update check [3]:::: nv_data = %x \n", res);
		}else
		{
			LDBGE(">>>>>>>	NV update check [3] ::: ISP camera FROM write fail \n");
			rc = -EIO;
			goto full_upload_fail;
		}

		mdelay(50);

		ISP_BIN_DOWN_FLAG= 1;	//ISP bin download end without error
		allow_suspend();  //                                            
	}// SUB-PCB(ISP) Exist Test
	else
	{
		LDBGE("========== NO Sub-PCB or ISP Broken!!!========== \n");
		ISP_BIN_DOWN_FLAG= 2;	//ISP bin No download
		rc = -EIO;// IF ISP is not esist then FW update No needs.
	}

	return rc;

full_upload_fail :
	LDBGE("========== CE_FwUpload() fail !!! rc = %ld ========== \n", rc);
	allow_suspend();  //                                            
	ISP_BIN_DOWN_FLAG = 2;	//ISP bin download fail during download !!! should be tried again !!!
	return rc;

}

/*===========================================================================

FUNCTION      ce1702_wq_ISP_upload
DESCRIPTION
DEPENDENCIES
  None
RETURN VALUE
  None
SIDE EFFECTS
  None

===========================================================================*/
void ce1702_wq_ISP_upload(struct work_struct *work)
{
	long rc = 0;

	LDBGI("ce1702_wq_ISP_upload: Check ISP Version because ISP_BIN_DOWN_FLAG = %d !! \n", ISP_BIN_DOWN_FLAG);

	mutex_lock(ce1702_s_interface_ctrl->msm_sensor_mutex);
#if 0
	rc = ce1702_power_up(ce1702_s_interface_ctrl);

	rc = ce1702_isp_fw_full_upload();  // should implement version info check code.
	if (rc < 0)
	{
		LDBGI(">>>> ce1702_isp_fw_full_upload fail !! ISP_BIN_DOWN_FLAG = %d \n", ISP_BIN_DOWN_FLAG);
		ISP_BIN_DOWN_FLAG   = 2;
	}

	ce1702_power_down(ce1702_s_interface_ctrl);
#endif

#if 1
	rc = ce1702_power_up(ce1702_s_interface_ctrl);
	LDBGI("ce1702_wq_ISP_upload: ce1702_power_on called \n");

	if (rc < 0) {
		LDBGE("ce1702_wq_ISP_upload: Cam Power-on failed!\n");
		ISP_BIN_DOWN_FLAG   = 2;
	}
	else if(ce1702_check_flash_version() < 0)
	{
		LDBGI(">>>>ce1702_isp_fw_full_upload start !! \n");
		ce1702_power_down(ce1702_s_interface_ctrl);
		ce1702_power_up(ce1702_s_interface_ctrl);
// luvsong : semaphore to prevent to run camera while downloading
//		mdelay(20000);		// Semaphore test code
		rc = ce1702_isp_fw_full_upload();
		if (rc < 0)
		{
			LDBGE(">>>> ce1702_isp_fw_full_upload fail !! ISP_BIN_DOWN_FLAG = %d \n", ISP_BIN_DOWN_FLAG);
			ISP_BIN_DOWN_FLAG   = 2;
		}
		else
		{
			LDBGI(">>>> upload done !!! ISP_BIN_DOWN_FLAG = %d\n", ISP_BIN_DOWN_FLAG);
			ce1702_check_flash_version();
		}
	}
	else
	{
		ISP_BIN_DOWN_FLAG	= 1;
	}

	ce1702_power_down(ce1702_s_interface_ctrl);
#endif
// luvsong : semaphore to prevent to run camera while downloading
	mutex_unlock(ce1702_s_interface_ctrl->msm_sensor_mutex);

	LDBGI("%s: Download done !!! so release mutex !!![%lu]\n", __func__, jiffies);

}

int ce1702_spi_write_then_read(struct spi_device *spi, unsigned char *txbuf, unsigned short tx_length, unsigned char *rxbuf, unsigned short rx_length)
{
	int res;

	struct spi_message	message;
	struct spi_transfer	x;

	spi_message_init(&message);
	memset(&x, 0, sizeof x);

	spi_message_add_tail(&x, &message);

	memcpy(tdata_buf, txbuf, tx_length);

	x.tx_buf=tdata_buf;
	x.rx_buf=rdata_buf;
	x.len = tx_length + rx_length;

	res = spi_sync(spi, &message);

	memcpy(rxbuf, x.rx_buf + tx_length, rx_length);

	return res;
}

int ce1702_spi_write_then_read_burst(struct spi_device *spi, unsigned char *txbuf, unsigned short tx_length, unsigned char *rxbuf, unsigned short rx_length)
{
	int res;

	struct spi_message	message;
	struct spi_transfer	x;

	spi_message_init(&message);
	memset(&x, 0, sizeof x);

	spi_message_add_tail(&x, &message);

	x.tx_buf=txbuf;
	x.rx_buf=rxbuf;
	x.len = tx_length + rx_length;

	res = spi_sync(spi, &message);

	return res;
}

static int spi_bulkread(HANDLE hDevice, unsigned char addr, unsigned char *data, unsigned short length)
{
	int ret;

	tx_data[0] = SPI_CMD_BURST_READ;
	tx_data[1] = addr;

	ret = ce1702_spi_write_then_read(ce1702_spi, &tx_data[0], 2, &data[0], length);

	if(!ret)
	{
		printk("ce1702_spi_bulkread fail : %d\n", ret);
		return CE1702_NOK;
	}

	return CE1702_OK;
}

static int spi_bulkwrite(HANDLE hDevice, unsigned char addr, unsigned char* data, unsigned short length)
{
	int ret;
	int i;

	tx_data[0] = SPI_CMD_BURST_WRITE;
	tx_data[1] = addr;

	for(i=0;i<length;i++)
	{
		tx_data[2+i] = data[i];
	}

	ret =ce1702_spi_write_then_read(ce1702_spi, &tx_data[0], length+2, NULL, 0);

	if(!ret)
	{
		printk("ce1702_spi_bulkwrite fail : %d\n", ret);
		return CE1702_NOK;
	}

	return CE1702_OK;
}

static int spi_dataread(HANDLE hDevice, unsigned char addr, unsigned char* data, unsigned short length)
{
	int ret=0;

	tx_data[0] = SPI_CMD_BURST_READ;
	tx_data[1] = addr;

	if(length>384)
		ret = ce1702_spi_write_then_read_burst(ce1702_spi, &tx_data[0], 2, &data[0], length);
	else
		ret = ce1702_spi_write_then_read(ce1702_spi, &tx_data[0], 2, &data[0], length);
	//printk("spi_dataread  (0x%x,0x%x,0x%x,0x%x)\n", data[0], data[1], data[2], data[3]);
	if(!ret)
	{
		printk("ce1702_spi_dataread fail : %d\n", ret);
		return CE1702_NOK;
	}

	return CE1702_OK;
}

int ce1702_spi_byteread(HANDLE hDevice, unsigned short addr, unsigned char *data)
{
	int res;
	unsigned char command = HPIC_READ | HPIC_BMODE | HPIC_ENDIAN;

	mutex_lock(&lock);

	res  = spi_bulkwrite(hDevice, CE1702_COMMAND_REG, &command, 1);
	res |= spi_bulkwrite(hDevice, CE1702_ADDRESS_REG, (unsigned char*)&addr, 2);
	res |= spi_bulkread(hDevice, CE1702_DATA_REG, data, 1);

	mutex_unlock(&lock);

	return res;
}

int ce1702_spi_wordread(HANDLE hDevice, unsigned short addr, unsigned short *data)
{
	int res;
	unsigned char command = HPIC_READ | HPIC_AINC | HPIC_BMODE | HPIC_ENDIAN;

//	if(BBM_SCI_DATA <= addr && BBM_SCI_SYNCRX >= addr)
		command = HPIC_READ | HPIC_WMODE | HPIC_ENDIAN;

	mutex_lock(&lock);

	res  = spi_bulkwrite(hDevice, CE1702_COMMAND_REG, &command, 1);
	res |= spi_bulkwrite(hDevice, CE1702_ADDRESS_REG, (unsigned char*)&addr, 2);
	res |= spi_bulkread(hDevice, CE1702_DATA_REG, (unsigned char*)data, 2);

	mutex_unlock(&lock);

	return res;
}

int ce1702_spi_longread(HANDLE hDevice, unsigned short addr, unsigned int *data)
{
	int res;
	unsigned char command = HPIC_READ | HPIC_AINC | HPIC_BMODE | HPIC_ENDIAN;

	mutex_lock(&lock);

	res  = spi_bulkwrite(hDevice, CE1702_COMMAND_REG, &command, 1);
	res |= spi_bulkwrite(hDevice, CE1702_ADDRESS_REG, (unsigned char*)&addr, 2);
	res |= spi_bulkread(hDevice, CE1702_DATA_REG, (unsigned char*)data, 4);

	mutex_unlock(&lock);

	return res;
}

int ce1702_spi_bulkread(HANDLE hDevice, unsigned short addr, unsigned char *data, unsigned short length)
{
	int res;
	unsigned char command = HPIC_READ | HPIC_AINC | HPIC_BMODE | HPIC_ENDIAN;

	mutex_lock(&lock);

	res  = spi_bulkwrite(hDevice, CE1702_COMMAND_REG, &command, 1);
	res |= spi_bulkwrite(hDevice, CE1702_ADDRESS_REG, (unsigned char*)&addr, 2);
	res |= spi_bulkread(hDevice, CE1702_DATA_REG, data, length);

	mutex_unlock(&lock);

	return res;
}

int ce1702_spi_bytewrite(HANDLE hDevice, unsigned short addr, unsigned char data)
{
	int res;
	unsigned char command = HPIC_WRITE | HPIC_BMODE | HPIC_ENDIAN;

	mutex_lock(&lock);

	res  = spi_bulkwrite(hDevice, CE1702_COMMAND_REG, &command, 1);
	res |= spi_bulkwrite(hDevice, CE1702_ADDRESS_REG, (unsigned char*)&addr, 2);
	res |= spi_bulkwrite(hDevice, CE1702_DATA_REG, (unsigned char*)&data, 1);

	mutex_unlock(&lock);

	return res;
}

int ce1702_spi_wordwrite(HANDLE hDevice, unsigned short addr, unsigned short data)
{
	int res;
	unsigned char command = HPIC_WRITE | HPIC_AINC | HPIC_BMODE | HPIC_ENDIAN;

//	if(BBM_SCI_DATA <= addr && BBM_SCI_SYNCRX >= addr)
		command = HPIC_WRITE | HPIC_WMODE | HPIC_ENDIAN;

	mutex_lock(&lock);

	res  = spi_bulkwrite(hDevice, CE1702_COMMAND_REG, &command, 1);
	res |= spi_bulkwrite(hDevice, CE1702_ADDRESS_REG, (unsigned char*)&addr, 2);
	res |= spi_bulkwrite(hDevice, CE1702_DATA_REG, (unsigned char*)&data, 2);

	mutex_unlock(&lock);

	return res;
}

int ce1702_spi_longwrite(HANDLE hDevice, unsigned short addr, unsigned int data)
{
	int res;
	unsigned char command = HPIC_WRITE | HPIC_AINC | HPIC_BMODE | HPIC_ENDIAN;

	mutex_lock(&lock);

	res  = spi_bulkwrite(hDevice, CE1702_COMMAND_REG, &command, 1);
	res |= spi_bulkwrite(hDevice, CE1702_ADDRESS_REG, (unsigned char*)&addr, 2);
	res |= spi_bulkwrite(hDevice, CE1702_DATA_REG, (unsigned char*)&data, 4);

	mutex_unlock(&lock);

	return res;
}

int ce1702_spi_bulkwrite(HANDLE hDevice, unsigned short addr, unsigned char* data, unsigned short length)
{
	int res;
	unsigned char command = HPIC_WRITE | HPIC_AINC | HPIC_BMODE | HPIC_ENDIAN;

	mutex_lock(&lock);

	res  = spi_bulkwrite(hDevice, CE1702_COMMAND_REG, &command, 1);
	res |= spi_bulkwrite(hDevice, CE1702_ADDRESS_REG, (unsigned char*)&addr, 2);
	res |= spi_bulkwrite(hDevice, CE1702_DATA_REG, data, length);

	mutex_unlock(&lock);

	return res;
}

int ce1702_spi_dataread(HANDLE hDevice, unsigned short addr, unsigned char* data, unsigned short length)
{
	int res;
	unsigned char command = HPIC_READ | HPIC_BMODE | HPIC_ENDIAN;

	mutex_lock(&lock);

	res  = spi_bulkwrite(hDevice, CE1702_COMMAND_REG, &command, 1);
	res |= spi_bulkwrite(hDevice, CE1702_ADDRESS_REG, (unsigned char*)&addr, 2);
	res |= spi_dataread(hDevice, CE1702_DATA_REG, data, length);

	mutex_unlock(&lock);

	return res;
}

static int ce1702_spi_suspend(struct spi_device *spi, pm_message_t mesg)
{
	printk("ce1702_spi_suspend \n");
	return 0;
}

static int ce1702_spi_resume(struct spi_device *spi)
{
	printk("ce1702_spi_resume \n");
	return 0;
}

static int ce1702_spi_probe(struct spi_device *spi)
{
	int rc;

	printk("ce1702_spi_probe start");

	ce1702_spi 				= spi;
	ce1702_spi->mode 			= SPI_MODE_0;
	ce1702_spi->bits_per_word 	= 8;
	ce1702_spi->max_speed_hz 	= (24000*1000);
	rc = spi_setup(spi);

	printk("ce1702_spi_probe spi_setup %d\n", rc);

	return rc;
}

static int ce1702_spi_remove(struct spi_device *spi)
{
	spi_unregister_driver(&ce1702_spi_driver);
	return 0;
}


MODULE_AUTHOR("LGE.com ");
MODULE_DESCRIPTION("NEC CE1702 interface");
MODULE_LICENSE("GPL v2");
