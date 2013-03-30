/* lge/com_device/input/touch_synaptics_fw_upgrade.c
 *
 * Copyright (C) 2011 LGE, Inc.
 *
 * Author: hyesung.shin@lge.com
 *
 * Notice: This is synaptic general touch FW upgrade driver for using RMI4.
 *		It not depends on specific regitser map and device spec.
 * 		If you want use this driver, you should completly understand
 * 		synaptics RMI4 register map structure.
 *
 *	F/W upgrade function Based on Synaptics RMIFuncs.cpp from charles.kim@synaptics.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */
#include <linux/delay.h>
#include <linux/slab.h>
#include <mach/gpio.h>
#include <linux/syscalls.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/i2c.h>

#include <linux/input/lge_touch_core.h>
#include <linux/input/touch_synaptics.h>

#define PAGE_SELECT_REG			0xFF
#define BLOCK_NUMBER_LOW_REG	0x00

struct RMI4FunctionDescriptor {
	u8 m_QueryBase;
	u8 m_CommandBase;
	u8 m_ControlBase;
	u8 m_DataBase;
	u8 m_IntSourceCount;
	u8 m_ID;
};

struct ConfigDataBlock {
	u16 m_Address;
	u8 m_Data;
};

struct synaptics_fw_data {
	struct RMI4FunctionDescriptor m_PdtF34Flash;
	struct RMI4FunctionDescriptor m_PdtF01Common;
	struct RMI4FunctionDescriptor m_BaseAddresses;
	struct RMI4FunctionDescriptor g_RMI4FunctionDescriptor;

	unsigned short m_uQuery_Base;

	struct ConfigDataBlock g_ConfigDataList[0x2a];
	struct ConfigDataBlock g_ConfigDataBlock;

	unsigned int g_ConfigDataCount;

	bool m_bAttenAsserted;
	bool m_bFlashProgOnStartup;
	bool m_bUnconfigured;

	unsigned short m_BootloadID;

	unsigned short m_bootloadImgID;
	unsigned char m_firmwareImgVersion;
	unsigned char *m_firmwareImgData;
	unsigned char *m_configImgData;
	unsigned short m_firmwareBlockSize;
	unsigned short m_firmwareBlockCount;
	unsigned short m_configBlockSize;
	unsigned short m_configBlockCount;

	/* Registers for configuration flash */
	unsigned char m_uPageData[0x200];
	unsigned char m_uStatus;
	unsigned long m_firmwareImgSize;
	unsigned long m_configImgSize;
	unsigned long m_fileSize;

	unsigned short m_uF01RMI_CommandBase;
	unsigned short m_uF01RMI_DataBase;
	unsigned short m_uF01RMI_QueryBase;
	unsigned short m_uF01RMI_IntStatus;

	unsigned short m_uF34Reflash_DataReg;
	unsigned short m_uF34Reflash_BlockNum;
	unsigned short m_uF34Reflash_BlockData;
	unsigned short m_uF34Reflash_FlashControl;
	unsigned short m_uF34ReflashQuery_BootID;
	unsigned short m_uF34ReflashQuery_FirmwareBlockSize;
	unsigned short m_uF34ReflashQuery_FirmwareBlockCount;
	unsigned short m_uF34ReflashQuery_ConfigBlockSize;
	unsigned short m_uF34ReflashQuery_ConfigBlockCount;
	unsigned short m_uF34ReflashQuery_FlashPropertyQuery;

	unsigned long m_FirmwareImgFile_checkSum;

	unsigned long total_sleep_count;

	unsigned char *image_bin;
	unsigned long image_size;
};

/*  Constants */
static const unsigned char s_uF34ReflashCmd_FirmwareCrc = 0x01;
static const unsigned char s_uF34ReflashCmd_FirmwareWrite = 0x02;
static const unsigned char s_uF34ReflashCmd_EraseAll = 0x03;
static const unsigned char s_uF34ReflashCmd_ConfigRead = 0x05;
static const unsigned char s_uF34ReflashCmd_ConfigWrite = 0x06;
static const unsigned char s_uF34ReflashCmd_ConfigErase = 0x07;
static const unsigned char s_uF34ReflashCmd_Enable = 0x0f;
/* Read the Flash control register. The result should be
$80: Flash Command = Idle ($0), Flash Status = Success ($0),
Program Enabled = Enabled (1). Any other value indicates an error. */
static const unsigned char s_uF34ReflashCmd_NormalResult = 0x80;

/* function proto type */
static int RMI4ReadConfigInfo(struct synaptics_fw_data *fw, struct synaptics_ts_data *ts);
static int RMI4ReadFirmwareInfo(struct synaptics_fw_data *fw, struct synaptics_ts_data *ts);
static int RMI4ReadPageDescriptionTable(struct synaptics_fw_data *fw, struct synaptics_ts_data *ts);
static int RMI4IssueFlashControlCommand(unsigned char * command,
		struct synaptics_fw_data *fw, struct synaptics_ts_data *ts);
static bool RMI4ValidateBootloadID(unsigned short bootloadID,
		struct synaptics_fw_data *fw, struct synaptics_ts_data *ts);
static int RMI4IssueEraseCommand(unsigned char * command,
		struct synaptics_fw_data *fw, struct synaptics_ts_data *ts);
static int RMI4setFlashAddrForDifFormat(struct synaptics_fw_data *fw, struct synaptics_ts_data *ts);

static void SynaSleep(unsigned long MilliSeconds, struct synaptics_fw_data *fw)
{
	msleep(MilliSeconds);

#ifdef LGE_TOUCH_TIME_DEBUG
	if (touch_time_debug_mask & DEBUG_TIME_FW_UPGRADE)
		fw->total_sleep_count += MilliSeconds * 1000;
#endif
}

static int SynaWaitForATTN(int dwMilliseconds,
		struct synaptics_fw_data *fw, struct synaptics_ts_data *ts)
{
	int trial_us=0;

	while ((gpio_get_value(ts->pdata->int_pin) != 0)
			&& (trial_us < (dwMilliseconds * 1000))) {
		//usleep(1);
		udelay(1);
		trial_us++;
	}

#ifdef LGE_TOUCH_TIME_DEBUG
	if (touch_time_debug_mask & DEBUG_TIME_FW_UPGRADE) {
		if(trial_us > 0)
			fw->total_sleep_count += trial_us;
	}
#endif

	if (gpio_get_value(ts->pdata->int_pin) != 0)
		return -EBUSY;
	else
		return 0;
}

static int SynaWriteRegister(struct i2c_client *client,
		u8 uRmiAddress, u8 * data, unsigned int length)
{
	return touch_i2c_write(client, uRmiAddress, length, data);
}

static int SynaReadRegister(struct i2c_client *client,
		u8 uRmiAddress, u8 * data, unsigned int length)
{
	return touch_i2c_read(client, uRmiAddress, length, data);
}

static void SpecialCopyEndianAgnostic(unsigned char *dest, unsigned short src)
{
	dest[0] = src % 0x100;	/* Endian agnostic method */
	dest[1] = src / 0x100;
}

static void RMI4FuncsConstructor(struct synaptics_fw_data *fw)
{
	if (touch_debug_mask & DEBUG_FW_UPGRADE)
		TOUCH_DEBUG_MSG("RMI4FuncsConstructor start\n");

	/* Initialize data members */
	fw->m_BaseAddresses.m_QueryBase = 0xff;
	fw->m_BaseAddresses.m_CommandBase = 0xff;
	fw->m_BaseAddresses.m_ControlBase = 0xff;
	fw->m_BaseAddresses.m_DataBase = 0xff;
	fw->m_BaseAddresses.m_IntSourceCount = 0xff;
	fw->m_BaseAddresses.m_ID = 0xff;
	fw->m_uQuery_Base = 0xffff;
	fw->m_bAttenAsserted = false;
	fw->m_bFlashProgOnStartup = false;
	fw->m_bUnconfigured = false;
	fw->g_ConfigDataList[0x29].m_Address = 0x0000;
	fw->g_ConfigDataList[0x29].m_Data = 0x00;
	fw->g_ConfigDataCount = sizeof(fw->g_ConfigDataList) / sizeof(struct ConfigDataBlock);
	fw->m_firmwareImgData = (unsigned char*)NULL;
	fw->m_configImgData = (unsigned char*)NULL;
}

static int RMI4Init(struct synaptics_fw_data *fw, struct synaptics_ts_data *ts)
{
	int ret;

	if (touch_debug_mask & DEBUG_FW_UPGRADE)
		TOUCH_DEBUG_MSG("RMI4Init start\n");

	/* Set up blockSize and blockCount for UI and config */
	ret = RMI4ReadConfigInfo(fw, ts);
	if (ret < 0) {
		TOUCH_ERR_MSG("Config info. read fail\n");
		return ret;
	}

	ret = RMI4ReadFirmwareInfo(fw, ts);
	if (ret < 0) {
		TOUCH_ERR_MSG("Config info. read fail\n");
		return ret;
	}

	return ret;
}

static unsigned short GetConfigSize(struct synaptics_fw_data *fw)
{
	return fw->m_configBlockSize * fw->m_configBlockCount;
}

static unsigned short GetFirmwareSize(struct synaptics_fw_data *fw)
{
	return fw->m_firmwareBlockSize * fw->m_firmwareBlockCount;
}

/* Read Bootloader ID from Block Data Registers as a 'key value' */
static int RMI4ReadBootloadID(struct synaptics_fw_data *fw, struct synaptics_ts_data *ts)
{
	int ret;
	char uData[2];

	ret = SynaReadRegister(ts->client,
			fw->m_uF34ReflashQuery_BootID, (unsigned char *)uData, 2);
	if (ret < 0)
		TOUCH_ERR_MSG("m_uF34ReflashQuery_BootID read fail\n");

	fw->m_BootloadID = (unsigned int)uData[0] + (unsigned int)uData[1] * 0x100;

	return ret;
}

static unsigned char RMI4IssueEnableFlashCommand(struct synaptics_fw_data *fw,
		struct synaptics_ts_data *ts)
{
	int ret;

	ret = SynaWriteRegister(ts->client,
			fw->m_uF34Reflash_FlashControl, (u8 *)&s_uF34ReflashCmd_Enable, 1);
	if (ret < 0)
		TOUCH_ERR_MSG("m_uF34Reflash_FlashControl write fail\n");

	return ret;
}

static int RMI4WriteBootloadID(struct synaptics_fw_data *fw, struct synaptics_ts_data *ts)
{
	int ret;
	unsigned char uData[2] = {0};

	SpecialCopyEndianAgnostic(uData, fw->m_BootloadID);

	ret = SynaWriteRegister(ts->client, fw->m_uF34Reflash_BlockData, &uData[0], 2);
	if (ret < 0)
		TOUCH_ERR_MSG("m_uF34Reflash_BlockData write fail\n");

	return ret;
}

/* Wait for ATTN assertion and see if it's idle and flash enabled */
static int RMI4WaitATTN(int errorCount,
	struct synaptics_fw_data *fw, struct synaptics_ts_data *ts)
{
	int ret;
	int uErrorCount = 0;

	/* To work around the physical address error from Control Bridge */
	ret = SynaWaitForATTN(1000, fw, ts);
	if (ret < 0) {
		TOUCH_ERR_MSG("SynaWaitForATTN 1000ms timeout error\n");
		return ret;
	}

	do {
		ret = SynaReadRegister(ts->client, fw->m_uF34Reflash_FlashControl, &fw->m_uPageData[0], 1);

		/* To work around the physical address error from control bridge
		  * The default check count value is 3. But the value is larger for erase condition
		  */
		if ((ret != 0) && uErrorCount < errorCount) {
			uErrorCount++;
			fw->m_uPageData[0] = 0;
			continue;
		}

		if (ret < 0) {
			TOUCH_ERR_MSG("m_uF34Reflash_FlashControl read fail %d times try\n", uErrorCount-1);
			return ret;
		}

		/* Clear the attention assertion by reading the interrupt status register */
		ret = SynaReadRegister(ts->client, fw->m_PdtF01Common.m_DataBase+1, &fw->m_uStatus, 1);
		if (ret < 0) {
			TOUCH_ERR_MSG("m_PdtF01Common.m_DataBase+1 read fail\n");
			return ret;
		}
	} while (fw->m_uPageData[0] != s_uF34ReflashCmd_NormalResult && uErrorCount < errorCount);

	return ret;
}

/* Enable Flashing programming */
static int RMI4EnableFlashing(struct synaptics_fw_data *fw, struct synaptics_ts_data *ts)
{
	int ret;
	u8 uData[2] = {0};
	int uErrorCount = 0;

	if (touch_debug_mask & DEBUG_FW_UPGRADE)
		TOUCH_DEBUG_MSG("RMI4EnableFlashing start\n");

	/* Read bootload ID */
	ret = RMI4ReadBootloadID(fw, ts);
	if (ret < 0) {
		TOUCH_ERR_MSG("Bootload ID read fail\n");
		return ret;
	}

	/* Write bootID to block data registers */
	ret = RMI4WriteBootloadID(fw, ts);
	if (ret < 0) {
		TOUCH_ERR_MSG("Bootload ID write fail\n");
		return ret;
	}

	do {
		ret = SynaReadRegister(ts->client, fw->m_uF34Reflash_FlashControl, &fw->m_uPageData[0], 1);

		/* To deal with ASIC physic address error from cdciapi lib when device is busy and not available for read */
		if ((ret != 0) && uErrorCount < 300) {
			uErrorCount++;
			fw->m_uPageData[0] = 0;
			continue;
		}

		if (ret < 0) {
			TOUCH_ERR_MSG("m_uF34Reflash_FlashControl read fail %d times try\n", uErrorCount);
			return ret;
		}

		/* Clear the attention assertion by reading the interrupt status register */
		ret = SynaReadRegister(ts->client, fw->m_PdtF01Common.m_DataBase+1, &fw->m_uStatus, 1);
		if (ret < 0) {
			TOUCH_ERR_MSG("m_PdtF01Common.m_DataBase+1 read fail\n");
			return ret;
		}
	} while ((( fw->m_uPageData[0] & 0x0f) != 0x00) && (uErrorCount <= 300));

	/* Issue Enable flash command */
	ret = RMI4IssueEnableFlashCommand(fw, ts);
	if (ret < 0) {
		TOUCH_ERR_MSG("Enable flash command write fail\n");
		return ret;
	}

	ret = RMI4ReadPageDescriptionTable(fw, ts);
	if (ret < 0) {
		TOUCH_ERR_MSG("Page description table read fail\n");
		return ret;
	}

	/* Wait for ATTN and check if flash command state is idle */
	ret = RMI4WaitATTN(300, fw, ts);
	if (ret < 0) {
		TOUCH_ERR_MSG("ATTN wait fail\n");
		return ret;
	}

	/* Read the data block 0 to determine the correctness of the image */
	ret = SynaReadRegister(ts->client, fw->m_uF34Reflash_FlashControl, &uData[0], 1);
	if (ret < 0) {
		TOUCH_ERR_MSG("m_uF34Reflash_FlashControl read fail\n");
		return ret;
	}

	if (uData[0] == 0x80) {
		if (touch_debug_mask & DEBUG_FW_UPGRADE)
			TOUCH_DEBUG_MSG("flash enabled\n");
	} else {	/* uData[0] == 0xff */
		TOUCH_ERR_MSG("flash failed: uData[0]=0x%x\n", uData[0]);
		return -EPERM;
	}

	return ret;
}

/* This function gets config block count and config block size */
static int RMI4ReadConfigInfo(struct synaptics_fw_data *fw, struct synaptics_ts_data *ts)
{
	int ret;
	u8 uData[2];

	ret = SynaReadRegister(ts->client, fw->m_uF34ReflashQuery_ConfigBlockSize, &uData[0], 2);
	if (ret < 0) {
		TOUCH_ERR_MSG("m_uF34ReflashQuery_ConfigBlockSize read fail\n");
		return ret;
	}

	fw->m_configBlockSize = uData[0] | (uData[1] << 8);

	ret = SynaReadRegister(ts->client, fw->m_uF34ReflashQuery_ConfigBlockCount, &uData[0], 2);
	if (ret < 0) {
		TOUCH_ERR_MSG("m_uF34ReflashQuery_ConfigBlockCount read fail\n");
		return ret;
	}

	fw->m_configBlockCount = uData[0] | (uData[1] << 8);
	fw->m_configImgSize = fw->m_configBlockSize * fw->m_configBlockCount;

	return ret;
}

/* This function write config data to device one block at a time */
static int RMI4ProgramConfiguration(struct synaptics_fw_data *fw, struct synaptics_ts_data *ts)
{
	int ret;
	unsigned char uData[2];
	unsigned char *puData = fw->m_configImgData;
	unsigned short blockNum = 0;

	if (touch_debug_mask & DEBUG_FW_UPGRADE)
		TOUCH_DEBUG_MSG("RMI4ProgramConfiguration start\n");

	for (blockNum = 0; blockNum < fw->m_configBlockCount; blockNum++) {
		SpecialCopyEndianAgnostic(&uData[0], blockNum);

		/* Write Configuration Block Number */
		ret = SynaWriteRegister(ts->client, fw->m_uF34Reflash_BlockNum, &uData[0], 2);
		if (ret < 0) {
			TOUCH_ERR_MSG("m_uF34Reflash_BlockNum write fail\n");
			return ret;
		}

		ret = SynaWriteRegister(ts->client,
				fw->m_uF34Reflash_BlockData, puData, fw->m_configBlockSize);
		if (ret < 0) {
			TOUCH_ERR_MSG("m_uF34Reflash_BlockData write fail\n");
			return ret;
		}

		puData += fw->m_configBlockSize;

		/* Issue Write Configuration Block command to flash command register */
		fw->m_bAttenAsserted = false;
		uData[0] = s_uF34ReflashCmd_ConfigWrite;

		ret = RMI4IssueFlashControlCommand(&uData[0], fw, ts);
		if (ret < 0) {
			TOUCH_ERR_MSG("Flash Control Command write fail\n");
			return ret;
		}

		/* Wait for ATTN */
		ret = RMI4WaitATTN(300, fw, ts);
		if (ret < 0) {
			TOUCH_ERR_MSG("ATTN wait fail\n");
			return ret;
		}
	}

	return ret;
}

static int RMI4IssueFlashControlCommand(unsigned char * command,
		struct synaptics_fw_data *fw, struct synaptics_ts_data *ts)
{
	int ret;

	ret = SynaWriteRegister(ts->client, fw->m_uF34Reflash_FlashControl, command, 1);
	if (ret < 0)
		TOUCH_ERR_MSG("m_uF34Reflash_FlashControl write fail\n");

	return ret;
}

/* Issue a reset ($01) command to the $F01 RMI command register
  * This tests firmware image and executes it if it's valid
  */
static int RMI4ResetDevice(struct synaptics_fw_data *fw, struct synaptics_ts_data *ts)
{
	int ret;
	unsigned short m_uF01DeviceControl_CommandReg = fw->m_PdtF01Common.m_CommandBase;
	unsigned char uData[1];

	uData[0] = 1;
	ret = SynaWriteRegister(ts->client, m_uF01DeviceControl_CommandReg, &uData[0], 1);
	if (ret < 0)
		TOUCH_ERR_MSG("m_uF01DeviceControl_CommandReg write fail\n");

	return ret;
}

static int RMI4DisableFlash(struct synaptics_fw_data *fw, struct synaptics_ts_data *ts)
{
	int ret = 0;
	unsigned char uData[2];
	unsigned int uErrorCount = 0;

	if (touch_debug_mask & DEBUG_FW_UPGRADE)
		TOUCH_DEBUG_MSG("RMI4DisableFlash start\n");

	/* Issue a reset command */
	ret = RMI4ResetDevice(fw, ts);
	if (ret < 0) {
		TOUCH_ERR_MSG("Reset device fail\n");
		return ret;
	}

	SynaSleep(200, fw);

	/* Wait for ATTN to be asserted to see if device is in idle state */
	ret = SynaWaitForATTN(300, fw, ts);
	if (ret < 0) {
		TOUCH_ERR_MSG("SynaWaitForATTN 300ms timeout error\n");
		return ret;
	}

	do {
		ret = SynaReadRegister(ts->client, fw->m_uF34Reflash_FlashControl, &fw->m_uPageData[0], 1);

		/* To work around the physical address error from control bridge */
		if ((ret != 0) && uErrorCount < 300) {
			uErrorCount++;
			fw->m_uPageData[0] = 0;
			continue;
		}
	} while (((fw->m_uPageData[0] & 0x0f) != 0x00) && (uErrorCount <= 300));

	if (ret < 0) {
		TOUCH_ERR_MSG("m_uF34Reflash_FlashControl read fail %d times try. m_uPageData = 0x%x\n",
				uErrorCount, (fw->m_uPageData[0] & 0x0f));
		return ret;
	} else {
		if (touch_debug_mask & DEBUG_FW_UPGRADE)
			TOUCH_DEBUG_MSG("RMI4WaitATTN after errorCount loop, uErrorCount=%d\n", uErrorCount);
	}

	/* Clear the attention assertion by reading the interrupt status register */
	ret = SynaReadRegister(ts->client, fw->m_PdtF01Common.m_DataBase+1, &fw->m_uStatus, 1);
	if (ret < 0) {
		TOUCH_ERR_MSG("m_PdtF01Common.m_DataBase+1 read fail\n");
		return ret;
	}

	/* Read F01 Status flash prog, ensure the 6th bit is '0' */
	do {
		ret = SynaReadRegister(ts->client, fw->m_uF01RMI_DataBase, &uData[0], 1);
		if (ret < 0)
			TOUCH_ERR_MSG("m_uF01RMI_DataBase read fail\n");

		if (touch_debug_mask & DEBUG_FW_UPGRADE)
			TOUCH_DEBUG_MSG("F01 data register bit 6: 0x%x\n", uData[0]);
	} while ((uData[0] & 0x40) != 0);

	/* With a new flash image the page description table could change */
	ret = RMI4ReadPageDescriptionTable(fw, ts);
	if (ret < 0) {
		TOUCH_ERR_MSG("Page description table read fail\n");
		return ret;
	}

	return ret;
}

static void RMI4CalculateChecksum(unsigned short * data, unsigned short len, unsigned long * dataBlock)
{
	unsigned long temp = *data++;
	unsigned long sum1;
	unsigned long sum2;

	*dataBlock = 0xffffffff;

	sum1 = *dataBlock & 0xFFFF;
	sum2 = *dataBlock >> 16;

	while (len--) {
		sum1 += temp;
		sum2 += sum1;
		sum1 = (sum1 & 0xffff) + (sum1 >> 16);
		sum2 = (sum2 & 0xffff) + (sum2 >> 16);
	}

	*dataBlock = sum2 << 16 | sum1;
}

static unsigned char RMI4FlashFirmwareWrite(struct synaptics_fw_data *fw, struct synaptics_ts_data *ts)
{
	int ret = 0;
	unsigned char *puFirmwareData = fw->m_firmwareImgData;
	unsigned char uData[2];
	u16 uBlockNum = 0;

	if (touch_debug_mask & DEBUG_FW_UPGRADE)
		TOUCH_DEBUG_MSG("Flash Firmware starts\n");

	if (touch_debug_mask & DEBUG_FW_UPGRADE)
		TOUCH_DEBUG_MSG("m_firmwareBlockCount=%d\n", fw->m_firmwareBlockCount);

	for (uBlockNum = 0; uBlockNum < fw->m_firmwareBlockCount; ++uBlockNum) {
		uData[0] = uBlockNum & 0xff;
		uData[1] = (uBlockNum & 0xff00) >> 8;

		/* Write Block Number */
		ret = SynaWriteRegister(ts->client, fw->m_uF34Reflash_BlockNum, &uData[0], 2);
		if (ret < 0) {
			TOUCH_ERR_MSG("m_uF34Reflash_BlockNum write fail\n");
			return ret;
		}

		/* Write Data Block */
		ret = SynaWriteRegister(ts->client, fw->m_uF34Reflash_BlockData, puFirmwareData, fw->m_firmwareBlockSize);
		if (ret < 0){
			TOUCH_ERR_MSG("m_uF34Reflash_BlockData write fail\n");
			return ret;
		}

		/* Move to next data block */
		puFirmwareData += fw->m_firmwareBlockSize;

		/* Issue Write Firmware Block command */
		fw->m_bAttenAsserted = false;
		uData[0] = 2;
		ret = SynaWriteRegister(ts->client, fw->m_uF34Reflash_FlashControl, &uData[0], 1);
		if (ret < 0) {
			TOUCH_ERR_MSG("m_uF34Reflash_FlashControl write fail\n");
			return ret;
		}

		if (touch_debug_mask & DEBUG_FW_UPGRADE)
			TOUCH_DEBUG_MSG("uBlockNum=%d, puFirmwareData=0x%p, m_firmwareBlockCount=%d\n",
						uBlockNum, puFirmwareData, fw->m_firmwareBlockCount);

		/* Wait ATTN. Read Flash Command register and check error */
		ret = RMI4WaitATTN(300, fw, ts);
		if (ret < 0) {
			TOUCH_ERR_MSG("ATTN wait fail\n");
			return ret;
		}
	}

	if (touch_debug_mask & DEBUG_FW_UPGRADE)
		TOUCH_DEBUG_MSG("Flash Firmware done\n");

	return ret;
}

static int RMI4ProgramFirmware(struct synaptics_fw_data *fw, struct synaptics_ts_data *ts)
{
	int ret;
	unsigned char uData[1];

	if (touch_debug_mask & DEBUG_FW_UPGRADE)
		TOUCH_DEBUG_MSG("RMI4ProgramFirmware start\n");

	if (!RMI4ValidateBootloadID(fw->m_bootloadImgID, fw, ts)) {
		TOUCH_ERR_MSG("Invalided Bootload ID\n");
		return -EPERM;
	}

	/* Write bootID to data block register */
	ret = RMI4WriteBootloadID(fw, ts);
	if (ret < 0) {
		TOUCH_ERR_MSG("Bootload ID write fail\n");
		return ret;
	}

	/* Issue the firmware and configuration erase command */
	uData[0] = 3;
	ret = RMI4IssueEraseCommand(&uData[0], fw, ts);
	if (ret < 0) {
		TOUCH_ERR_MSG("Erase Command fail\n");
		return ret;
	}

	ret = RMI4WaitATTN(1000, fw, ts);
	if (ret < 0) {
		TOUCH_ERR_MSG("ATTN wait fail\n");
		return ret;
	}

	/* Write firmware image */
	ret = RMI4FlashFirmwareWrite(fw, ts);
	if (ret < 0) {
		TOUCH_ERR_MSG("Firmware write fail\n");
		return ret;
	}

	if (touch_debug_mask & DEBUG_FW_UPGRADE)
		TOUCH_DEBUG_MSG("RMI4ProgramFirmware end\n");

	return ret;
}

static bool RMI4ValidateBootloadID(unsigned short bootloadID,
		struct synaptics_fw_data *fw, struct synaptics_ts_data *ts)
{
	int ret;

	ret = RMI4ReadBootloadID(fw, ts);
	if (ret < 0)
		TOUCH_ERR_MSG("Read bootload ID fail\n");

	if (touch_debug_mask & DEBUG_FW_UPGRADE)
		TOUCH_DEBUG_MSG("Bootload ID of device: %X, input bootID: %X\n",
			fw->m_BootloadID, bootloadID);

	/* check bootload ID against the value found in firmware--but only for image file format version 0 */
	return fw->m_firmwareImgVersion != 0 || bootloadID == fw->m_BootloadID;
}

static int RMI4IssueEraseCommand(unsigned char * command,
		struct synaptics_fw_data *fw, struct synaptics_ts_data *ts)
{
	int ret;

	/* command = 3 - erase all; command = 7 - erase config */
	ret = SynaWriteRegister(ts->client, fw->m_uF34Reflash_FlashControl, command, 1);

	return ret;
}

/* This function gets the firmware block size and block count */
static int RMI4ReadFirmwareInfo(struct synaptics_fw_data *fw, struct synaptics_ts_data *ts)
{
	int ret;
	unsigned char uData[2] = {0};

	ret = SynaReadRegister(ts->client, fw->m_uF34ReflashQuery_FirmwareBlockSize, &uData[0], 2);
	if (ret < 0) {
		TOUCH_ERR_MSG("m_uF34ReflashQuery_FirmwareBlockSize read fail\n");
		return ret;
	}

	fw->m_firmwareBlockSize = uData[0] | (uData[1] << 8);

	ret = SynaReadRegister(ts->client, fw->m_uF34ReflashQuery_FirmwareBlockCount, &uData[0], 2);
	if (ret < 0) {
		TOUCH_ERR_MSG("m_uF34ReflashQuery_FirmwareBlockCount read fail\n");
		return ret;
	}

	fw->m_firmwareBlockCount = uData[0] | (uData[1] << 8);
	fw->m_firmwareImgSize = fw->m_firmwareBlockCount * fw->m_firmwareBlockSize;

	if (touch_debug_mask & DEBUG_FW_UPGRADE)
		TOUCH_DEBUG_MSG("m_firmwareBlockSize=%d, m_firmwareBlockCount=%d\n",
			fw->m_firmwareBlockSize, fw->m_firmwareBlockCount);

	return ret;
}

static int RMI4ReadPageDescriptionTable(struct synaptics_fw_data *fw, struct synaptics_ts_data *ts)
{
	/* Read config data */
	int ret;
	struct RMI4FunctionDescriptor Buffer;
	unsigned short uAddress;

	memset(&Buffer, 0x0, sizeof(struct RMI4FunctionDescriptor));

	SynaSleep(20, fw);

	fw->m_PdtF01Common.m_ID = 0;
	fw->m_PdtF34Flash.m_ID = 0;
	fw->m_BaseAddresses.m_ID = 0xff;


	for (uAddress = DESCRIPTION_TABLE_START; uAddress > 10; uAddress -= sizeof(struct RMI4FunctionDescriptor)) {
		ret = SynaReadRegister(ts->client, uAddress, (unsigned char *)&Buffer, sizeof(Buffer));
		if (ret < 0) {
			TOUCH_ERR_MSG("RMI4FunctionDescriptor read fail\n");
			return ret;
		}

		if (fw->m_BaseAddresses.m_ID == 0xff)
			fw->m_BaseAddresses = Buffer;

		switch (Buffer.m_ID) {
		case 0x34:
			fw->m_PdtF34Flash = Buffer;
			break;
		case 0x01:
			fw->m_PdtF01Common = Buffer;
			break;
		}

		if (Buffer.m_ID == 0)
			break;
		else
		{
			if (touch_debug_mask & DEBUG_FW_UPGRADE)
				TOUCH_DEBUG_MSG("Function $%02x found\n", Buffer.m_ID);
		}
	}

	/* Initialize device related data members */
	fw->m_uF01RMI_DataBase = fw->m_PdtF01Common.m_DataBase;
	fw->m_uF01RMI_IntStatus = fw->m_PdtF01Common.m_DataBase+1;
	fw->m_uF01RMI_CommandBase = fw->m_PdtF01Common.m_CommandBase;
	fw->m_uF01RMI_QueryBase = fw->m_PdtF01Common.m_QueryBase;

	fw->m_uF34Reflash_DataReg = fw->m_PdtF34Flash.m_DataBase;
	fw->m_uF34Reflash_BlockNum = fw->m_PdtF34Flash.m_DataBase;
	fw->m_uF34Reflash_BlockData = fw->m_PdtF34Flash.m_DataBase+2;
	fw->m_uF34ReflashQuery_BootID = fw->m_PdtF34Flash.m_QueryBase;

	fw->m_uF34ReflashQuery_FlashPropertyQuery = fw->m_PdtF34Flash.m_QueryBase+2;
	fw->m_uF34ReflashQuery_FirmwareBlockSize = fw->m_PdtF34Flash.m_QueryBase+3;
	fw->m_uF34ReflashQuery_FirmwareBlockCount = fw->m_PdtF34Flash.m_QueryBase+5;
	fw->m_uF34ReflashQuery_ConfigBlockSize = fw->m_PdtF34Flash.m_QueryBase+3;
	fw->m_uF34ReflashQuery_ConfigBlockCount = fw->m_PdtF34Flash.m_QueryBase+7;

	ret = RMI4setFlashAddrForDifFormat(fw, ts);
	if (ret < 0) {
		TOUCH_ERR_MSG("Flash address setting fail\n");
		return ret;
	}

	return ret;
}

static int RMI4WritePage(struct synaptics_fw_data *fw, struct synaptics_ts_data *ts)
{
	int ret;
	unsigned char uPage = 0x00;
	unsigned char uF01_RMI_Data[2];
	unsigned char m_uStatus;

	if (touch_debug_mask & DEBUG_FW_UPGRADE)
		TOUCH_DEBUG_MSG("RMI4WritePage start\n");

	ret = SynaWriteRegister(ts->client, PAGE_SELECT_REG, (unsigned char *)&uPage, 1);
	if (ret < 0) {
		TOUCH_ERR_MSG("PAGE_SELECT_REG write fail\n");
		return ret;
	}

	if (touch_debug_mask & DEBUG_FW_UPGRADE) {
		uPage = 0;
		SynaReadRegister(ts->client, PAGE_SELECT_REG, (unsigned char *)&uPage, 1);
		TOUCH_DEBUG_MSG("PAGE_SELECT_REG = 0x%x\n", uPage);
	}

	do {
		ret = SynaReadRegister(ts->client, BLOCK_NUMBER_LOW_REG, (unsigned char *)&m_uStatus, 1);
		if (ret < 0) {
			TOUCH_ERR_MSG("BLOCK_NUMBER_LOW_REG read fail\n");
			return ret;
		}

		if (touch_debug_mask & DEBUG_FW_UPGRADE)
			TOUCH_DEBUG_MSG("BLOCK_NUMBER_LOW_REG = 0x%x\n", m_uStatus);

		if (m_uStatus & 0x40) {
			fw->m_bFlashProgOnStartup = true;
		}

		if (m_uStatus & 0x80) {
			fw->m_bUnconfigured = true;
			break;
		}

	} while (m_uStatus & 0x40);

	if (fw->m_bFlashProgOnStartup && !fw->m_bUnconfigured) {
		if (touch_debug_mask & DEBUG_FW_UPGRADE)
			TOUCH_DEBUG_MSG("Bootloader running\n");
	} else if (fw->m_bUnconfigured) {
		if (touch_debug_mask & DEBUG_FW_UPGRADE)
			TOUCH_DEBUG_MSG("UI running\n");
	}

	ret = RMI4ReadPageDescriptionTable(fw, ts);
	if (ret < 0) {
		TOUCH_ERR_MSG("Page description table read fail\n");
		return ret;
	}

	if (fw->m_PdtF34Flash.m_ID == 0) {
		TOUCH_ERR_MSG("Function $34(Flash Memory Management) is not supported\n");
		return -EPERM;
	}

	if (touch_debug_mask & DEBUG_FW_UPGRADE)
		TOUCH_DEBUG_MSG("Function $34 addresses Control base:$%02x Query base: $%02x\n",
					fw->m_PdtF34Flash.m_ControlBase, fw->m_PdtF34Flash.m_QueryBase);

	if (fw->m_PdtF01Common.m_ID == 0) {
		if (touch_debug_mask & DEBUG_FW_UPGRADE)
			TOUCH_DEBUG_MSG("Function $01 is not supported\n");
		fw->m_PdtF01Common.m_ID = 0x01;
		fw->m_PdtF01Common.m_DataBase = 0;
	}


	if (touch_debug_mask & DEBUG_FW_UPGRADE)
		TOUCH_DEBUG_MSG("Function $01 addresses Control base:$%02x Query base: $%02x\n",
					fw->m_PdtF01Common.m_ControlBase, fw->m_PdtF01Common.m_QueryBase);

	/* Get device status */
	ret = SynaReadRegister(ts->client,fw->m_PdtF01Common.m_DataBase,
			&uF01_RMI_Data[0], sizeof(uF01_RMI_Data));
	if (ret < 0) {
		TOUCH_ERR_MSG("BLOCK_NUMBER_LOW_REG read fail\n");
		return ret;
	} else {
		if (touch_debug_mask & DEBUG_FW_UPGRADE) {
			/* Check Device Status */
			TOUCH_DEBUG_MSG("Configured: %s\n", uF01_RMI_Data[0] & 0x80 ? "false" : "true");
			TOUCH_DEBUG_MSG("FlashProg:  %s\n", uF01_RMI_Data[0] & 0x40 ? "true" : "false");
			TOUCH_DEBUG_MSG("StatusCode: 0x%x \n", uF01_RMI_Data[0] & 0x0f);
		}
	}

	return ret;
}

static unsigned long ExtractLongFromHeader(const unsigned char* SynaImage)	/* Endian agnostic */
{
	return ((unsigned long)SynaImage[0] +
			(unsigned long)SynaImage[1] * 0x100 +
			(unsigned long)SynaImage[2] * 0x10000 +
			(unsigned long)SynaImage[3] * 0x1000000);
}

static int RMI4ReadFirmwareHeader(struct synaptics_fw_data *fw, struct synaptics_ts_data *ts)
{
	int ret = 0;
	unsigned long checkSumCode = 0;

	if (touch_debug_mask & DEBUG_FW_UPGRADE)
		TOUCH_DEBUG_MSG("RMI4ReadFirmwareHeader start\n");

	fw->m_fileSize = fw->image_size;

	checkSumCode = ExtractLongFromHeader(&(fw->image_bin[0]));
	fw->m_bootloadImgID = (unsigned int)fw->image_bin[4] + (unsigned int)fw->image_bin[5] * 0x100;
	fw->m_firmwareImgVersion = fw->image_bin[31];
	fw->m_firmwareImgSize = ExtractLongFromHeader(&(fw->image_bin[8]));
	fw->m_configImgSize = ExtractLongFromHeader(&(fw->image_bin[12]));

	if (touch_debug_mask & DEBUG_FW_UPGRADE) {
		TOUCH_DEBUG_MSG("Target = %s, ", &fw->image_bin[16]);
		TOUCH_DEBUG_MSG("Cksum = 0x%lX, Id = 0x%X, Ver = %d, FwSize = 0x%lX, ConfigSize = 0x%lX \n",
				checkSumCode, fw->m_bootloadImgID, fw->m_firmwareImgVersion, fw->m_firmwareImgSize, fw->m_configImgSize);
	}

#if !defined(TEST_WRONG_CHIPSET_FW_FORCE_UPGRADE)
	/* Check prpoer FW */
	if (strncmp(ts->fw_info->product_id , &fw->image_bin[16], 10)) {
		printk(KERN_INFO "[Touch E] IC = %s, Firmware Target = %s\n ", ts->fw_info->product_id, &fw->image_bin[16]);
		printk(KERN_INFO "[Touch E] WARNING - Firmware is mismatched with Touch IC\n");
		printk(KERN_INFO "[Touch E] Firmware upgrade stop\n");
		return -ENODEV;
	}
#endif

	ret = RMI4ReadFirmwareInfo(fw, ts);	/* Determine firmware organization - read firmware block size and firmware size */
	if (ret < 0) {
		TOUCH_ERR_MSG("Config info. read fail\n");
		return ret;
	}

	RMI4CalculateChecksum((unsigned short*)&(fw->image_bin[4]),
			(unsigned short)((fw->m_fileSize - 4) >> 1),
			(unsigned long *)&fw->m_FirmwareImgFile_checkSum);

	if (fw->m_fileSize != (0x100 + fw->m_firmwareImgSize + fw->m_configImgSize))
		TOUCH_ERR_MSG("SynaFirmware[] size = 0x%lX, expected 0x%lX\n",
				fw->m_fileSize, (0x100 + fw->m_firmwareImgSize + fw->m_configImgSize));

	if (fw->m_firmwareImgSize != GetFirmwareSize(fw))
		TOUCH_ERR_MSG("Firmware image size verfication failed, size in image 0x%lX did not match device size 0x%X\n",
				fw->m_firmwareImgSize, GetFirmwareSize(fw));

	if (fw->m_configImgSize != GetConfigSize(fw))
		TOUCH_ERR_MSG("Configuration size verfication failed, size in image 0x%lX did not match device size 0x%X\n",
				fw->m_configImgSize, GetConfigSize(fw));

	fw->m_firmwareImgData = (unsigned char *)((&fw->image_bin[0]) + 0x100);
	fw->m_configImgData = (unsigned char *)((&fw->image_bin[0]) + 0x100 + fw->m_firmwareImgSize);

	ret = SynaReadRegister(ts->client, fw->m_uF34Reflash_FlashControl, &fw->m_uPageData[0], 1);
	if (ret < 0) {
		TOUCH_ERR_MSG("m_uF34Reflash_FlashControl read fail\n");
		return ret;
	}

	return ret;
}

static int RMI4isExpectedRegFormat(struct synaptics_fw_data *fw, struct synaptics_ts_data *ts)
{
	/* Flash Properties query 1: registration map format version 1
	 * 0: registration map format version 0
	 */
	int ret;

	ret = SynaReadRegister(ts->client,
			fw->m_uF34ReflashQuery_FlashPropertyQuery, &fw->m_uPageData[0], 1);
	if (ret < 0) {
		TOUCH_ERR_MSG("RMI4isExpectedRegFormat read fail\n");
		return ret;
	} else {
		if (touch_debug_mask & DEBUG_FW_UPGRADE)
			TOUCH_DEBUG_MSG("FlashPropertyQuery = 0x%x\n", fw->m_uPageData[0]);
	}

	return (( fw->m_uPageData[0] & 0x01) == 0x01);
}

static int RMI4setFlashAddrForDifFormat(struct synaptics_fw_data *fw, struct synaptics_ts_data *ts)
{
	int ret;
	if (touch_debug_mask & DEBUG_FW_UPGRADE)
		TOUCH_DEBUG_MSG("RMI4setFlashAddrForDifFormat start\n");

	ret = RMI4isExpectedRegFormat(fw, ts);

	if (ret < 0) {
		return ret;
	} else if( ret == 1) {
		fw->m_uF34Reflash_FlashControl = fw->m_PdtF34Flash.m_DataBase + fw->m_firmwareBlockSize + 2;
		fw->m_uF34Reflash_BlockNum = fw->m_PdtF34Flash.m_DataBase;
		fw->m_uF34Reflash_BlockData = fw->m_PdtF34Flash.m_DataBase+2;
	} else {	/*ret == 0 */
		fw->m_uF34Reflash_FlashControl = fw->m_PdtF34Flash.m_DataBase;
		fw->m_uF34Reflash_BlockNum = fw->m_PdtF34Flash.m_DataBase+1;
		fw->m_uF34Reflash_BlockData = fw->m_PdtF34Flash.m_DataBase+3;
	}

	if (touch_debug_mask & DEBUG_FW_UPGRADE)
		TOUCH_DEBUG_MSG("Image format version %d\n", ret);

	return ret;
}

/* Scan Page Description Table (PDT) to find all RMI functions presented by this device.
 * The Table starts at $00EE. This and every sixth register (decrementing) is a function number
 * except when this "function number" is $00, meaning end of PDT.
 * In an actual use case this scan might be done only once on first run or before compile.
 */
int FirmwareUpgrade(struct synaptics_ts_data *ts, const char* fw_path)
{
	struct synaptics_fw_data *fw;
	int ret = 0;
	int fd = -1;
	struct stat fw_bin_stat;
	mm_segment_t old_fs = 0;
	unsigned long read_bytes;

	fw = kzalloc(sizeof(*fw), GFP_KERNEL);
	if (fw == NULL) {
		TOUCH_ERR_MSG("Can not allocate  memory\n");
		ret = -ENOMEM;
		goto exit_upgrade;
	}

	if(unlikely(fw_path[0] != 0)) {
		old_fs = get_fs();
		set_fs(get_ds());

		if ((fd = sys_open((const char __user *) fw_path, O_RDONLY, 0)) < 0) {
			TOUCH_ERR_MSG("Can not read FW binary from %s\n", fw_path);
			ret = -EEXIST;
			goto read_fail;
		}

		if ((ret = sys_newstat((char __user *) fw_path, (struct stat *)&fw_bin_stat)) < 0) {
			TOUCH_ERR_MSG("Can not read FW binary stat from %s\n", fw_path);
			goto fw_mem_alloc_fail;
		}

		fw->image_size = fw_bin_stat.st_size;
		fw->image_bin = kzalloc(sizeof(char) * (fw->image_size+1), GFP_KERNEL);
		if (fw->image_bin == NULL) {
			TOUCH_ERR_MSG("Can not allocate  memory\n");
			ret = -ENOMEM;
			goto fw_mem_alloc_fail;
		}

		//sys_lseek(fd, (off_t) pos, 0);
		read_bytes = sys_read(fd, (char __user *)fw->image_bin, fw->image_size);

		/* for checksum */
		*(fw->image_bin+fw->image_size) = 0xFF;

		TOUCH_INFO_MSG("Touch FW image read %ld bytes from %s\n", read_bytes, fw_path);
	}else {
		fw->image_size = ts->fw_info->fw_size-1;
		fw->image_bin = (unsigned char *)(&ts->fw_info->fw_start[0]);
	}

#ifdef LGE_TOUCH_TIME_DEBUG
	if (touch_time_debug_mask & DEBUG_TIME_FW_UPGRADE)
		fw->total_sleep_count = 0;
#endif

	RMI4FuncsConstructor(fw);

	ret = RMI4WritePage(fw, ts);
	if (ret < 0) {
		TOUCH_ERR_MSG("Can not write page\n");
		goto exit_upgrade;
	}

	ret = RMI4Init(fw, ts);
	if (ret < 0) {
		TOUCH_ERR_MSG("RMI4 init fail\n");
		goto exit_upgrade;
	}

	ret = RMI4setFlashAddrForDifFormat(fw, ts);
	if (ret < 0) {
		TOUCH_ERR_MSG("Flash address setting fail\n");
		goto exit_upgrade;
	}

	ret = RMI4EnableFlashing(fw, ts);
	if (ret < 0) {
		TOUCH_ERR_MSG("Flash address setting fail\n");
		goto exit_upgrade;
	}

	ret = RMI4ReadFirmwareHeader(fw, ts);
	if (ret < 0) {
		TOUCH_ERR_MSG("Firmware header read fail\n");
		goto exit_upgrade;
	}

	ret = RMI4ProgramFirmware(fw, ts);					/* issues the "eraseAll" so must call before any write */
	if (ret < 0) {
		TOUCH_ERR_MSG("Program Firmware fail\n");
		goto exit_upgrade;
	}

	ret = RMI4ProgramConfiguration(fw, ts);
	if (ret < 0) {
		TOUCH_ERR_MSG("Program Configuration fail\n");
		goto exit_upgrade;
	}

	ret = RMI4DisableFlash(fw, ts);
	if (ret < 0) {
		TOUCH_ERR_MSG("Disable Flash fail\n");
		goto exit_upgrade;
	}

#ifdef LGE_TOUCH_TIME_DEBUG
	if (touch_time_debug_mask & DEBUG_TIME_FW_UPGRADE)
		TOUCH_DEBUG_MSG("FW upgrade total waiting time count=%ld(us)\n", fw->total_sleep_count);
#endif

	if(unlikely(fw_path[0] != 0))
		kfree(fw->image_bin);
fw_mem_alloc_fail:
	sys_close(fd);
read_fail:
	set_fs(old_fs);
exit_upgrade:
	kfree(fw);
	return ret;
}
