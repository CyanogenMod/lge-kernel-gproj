#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/input/lge_touch_core.h>
#include <linux/input/touch_synaptics.h>

#define CFG_F54_TXCOUNT 16
#define CFG_F54_RXCOUNT 28

/*page 0x00*/
#define F01_CMD_BASE 0x89
#define F01_CTRL_BASE 0x50
#define F01_DATA_BASE 0x13

/*page 0x01*/
#define F54_COMMAND_BASE 0x7B
#define F54_DATA_BASE 0x00
#define F54_CONTROL_BASE 0x10
#define F54_DATA_LOWINDEX F54_DATA_BASE + 1
#define F54_DATA_HIGHINDEX F54_DATA_BASE + 2
#define F54_DATA_BUFFER F54_DATA_BASE + 3
#define F54_CBC_SETTINGS F54_CONTROL_BASE + 8
#define NOISE_MITIGATION 0x3A

signed ImageArray[CFG_F54_TXCOUNT][CFG_F54_RXCOUNT];
unsigned char ImageBuffer[CFG_F54_TXCOUNT*CFG_F54_RXCOUNT*2];
int F54_FullRawCap(struct i2c_client *client, int mode, char *buf)
{
	struct synaptics_ts_data* ts = (struct synaptics_ts_data*)get_touch_handle(client);

	int i, j, k, iRtn = 0;
	unsigned short length;
	unsigned char numberOfTx;
	unsigned char numberOfRx;

	unsigned char command;

	TOUCH_DEBUG_MSG("%s START!! disable_irq_nosync!! \n", __func__);
	disable_irq_nosync(ts->client->irq);

	numberOfTx = CFG_F54_TXCOUNT;
	numberOfRx = CFG_F54_RXCOUNT;

	TOUCH_DEBUG_MSG("numberOfTx : %d, numberOfRx : %d\n", numberOfTx, numberOfRx);
	length = numberOfTx * numberOfRx* 2;

	// Set report mode to to run the AutoScan
	if(mode >= 0 && mode < 4)	command = 0x03;
	if(mode == 4)				command = 0x02;

	touch_i2c_write_byte(ts->client, PAGE_SELECT_REG, 0x01);
	touch_i2c_write_byte(ts->client, F54_DATA_BASE, command);

	// Force update & Force cal
	touch_i2c_write_byte(ts->client, F54_COMMAND_BASE, 0x06);

	do {
		mdelay(1); //wait 1ms
		touch_i2c_read(ts->client, F54_COMMAND_BASE, sizeof(command), &command);
	} while (command != 0x00);

	// Enabling only the analog image reporting interrupt, and turn off the rest
	touch_i2c_write_byte(ts->client, PAGE_SELECT_REG, 0x00);
	touch_i2c_write_byte(ts->client, 0x14, 0x08);

	touch_i2c_write_byte(ts->client, PAGE_SELECT_REG, 0x01);
	command = 0x00;
	touch_i2c_write_byte(ts->client, F54_DATA_BASE + 1, 0x00);
	touch_i2c_write_byte(ts->client, F54_DATA_BASE + 2, 0x00);

	// Set the GetReport bit to run the AutoScan
	touch_i2c_write_byte(ts->client, F54_COMMAND_BASE, 0x01);

	// Wait until the command is completed
	do {
		mdelay(1); //wait 1ms
		touch_i2c_read(ts->client, F54_COMMAND_BASE, sizeof(command), &command);
	} while (command != 0x00);

	touch_i2c_read(ts->client, F54_DATA_BASE + 3, length, &ImageBuffer[0]);

	TOUCH_DEBUG_MSG("numberOfTx : %d, numberOfRx : %d\n", numberOfTx, numberOfRx);

	switch(mode)
	{
		case 3:
		case 4:
			k = 0;
			for (i = 0; i < numberOfTx; i++)
			{
				for (j = 0; j < numberOfRx; j++)
				{
					ImageArray[i][j] = (short)(ImageBuffer[k] | (ImageBuffer[k+1] << 8));
					k = k + 2;
				}
			}
			// Print Capacitance Imgae for getting log for Excel format
			printk("\n\t");
			/*if (buf != NULL) iRtn += sprintf(buf + iRtn, "RX : ");*/
			for (j = 0; j < numberOfRx; j++) {
				printk("RX%d\t", j);
				/*if (buf != NULL) iRtn += sprintf(buf + iRtn, "%d  ", j);*/
			}
			printk("\n");
			/*if (buf != NULL) iRtn += sprintf(buf + iRtn, "\n");*/

			for (i = 0; i < numberOfTx; i++)
			{
				printk("TX%d\t", i);
				/*if (buf != NULL) iRtn += sprintf(buf + iRtn, "TX%d : ", i);*/
				for (j = 0; j < numberOfRx; j++)
				{
					if (mode == 3) {
						printk("%d.%03d\t", ImageArray[i][j]/1000, ImageArray[i][j]%1000);
						/*if (buf != NULL) iRtn += sprintf(buf + iRtn, "%d.%01d ", ImageArray[i][j]/1000, (ImageArray[i][j]%1000)/100);*/
					} else if(mode == 4) {
						printk("%d\t", ImageArray[i][j]);
						/*if (buf != NULL) iRtn += sprintf(buf + iRtn, "%d ", ImageArray[i][j]);*/
					}
				}
				printk("\n");
				/*if (buf != NULL) iRtn += sprintf(buf + iRtn, "\n");*/
			}
			break;

		default:
			break;
	}

	//Reset
	touch_i2c_write_byte(ts->client, PAGE_SELECT_REG, 0x00);
	touch_i2c_write_byte(ts->client, F01_CMD_BASE, 0x01);
	mdelay(200);
	touch_i2c_read(ts->client, F01_DATA_BASE + 1, sizeof(command), &command);

	enable_irq(ts->client->irq);
	TOUCH_DEBUG_MSG("%s END!! enable_irq!!!\n", __func__);

	return iRtn;
}

unsigned char F54_TxToTxReport(struct i2c_client *client)
{
	struct synaptics_ts_data* ts = (struct synaptics_ts_data*)get_touch_handle(client);

	unsigned char ImageBuffer[CFG_F54_TXCOUNT];
	unsigned char ImageArray[CFG_F54_TXCOUNT];
	unsigned char result = 0;
	int i, k;
	int shift;
	unsigned char command;

	TOUCH_DEBUG_MSG("%s START!! disable_irq_nosync!! \n", __func__);
	disable_irq_nosync(ts->client->irq);

	printk("\nBin #: 5		Name: Transmitter To Transmitter Short Test\n");
	for (i=0; i<CFG_F54_TXCOUNT; i++) {
		ImageArray[i] = 1;
	}

	touch_i2c_write_byte(ts->client, PAGE_SELECT_REG, 0x01);
	touch_i2c_write_byte(ts->client, F54_DATA_BASE, 0x05);

	touch_i2c_write_byte(ts->client, F54_DATA_LOWINDEX, 0x00);
	touch_i2c_write_byte(ts->client, F54_DATA_HIGHINDEX, 0x00);

	touch_i2c_write_byte(ts->client, F54_COMMAND_BASE, 0x01);

	do {
		mdelay(1);
		touch_i2c_read(ts->client, F54_COMMAND_BASE, sizeof(command), &command);
	} while (command != 0x00);

	touch_i2c_read(ts->client, F54_DATA_BUFFER, 2, &ImageBuffer[0]);

	k = 0;
	for (i=0; i<CFG_F54_TXCOUNT; i++) {
		k = i / 8;
		shift = i % 8;
		if ( !(ImageBuffer[k] & (1 << shift)) ) ImageArray[i] = 0;
	}

	printk("Column:\t");
	for (i=0; i< CFG_F54_TXCOUNT; i++) {
		printk("Tx%d,\t", i);
	}
	printk("\n");

	printk("0:\t");
	for (i=0; i<CFG_F54_TXCOUNT; i++) {
		if (!ImageArray[i]) {
			result++;
			printk("%d,\t", ImageArray[i]);
		} else {
			printk("%d(*),\t", ImageArray[i]);
		}
	}
	printk("\n");

	touch_i2c_write_byte(ts->client, PAGE_SELECT_REG, 0x00);
	touch_i2c_write_byte(ts->client, F01_CMD_BASE, 0x01);
	mdelay(200);
	touch_i2c_read(ts->client, F01_DATA_BASE + 1, sizeof(command), &command);

	if (result == CFG_F54_TXCOUNT) {
		printk("Test Result : Pass\n");
	} else {
		printk("Test Result : Fail\n");
	}

	enable_irq(ts->client->irq);
	TOUCH_DEBUG_MSG("%s END!! enable_irq!!!\n", __func__);

	return result;
}

unsigned char F54_TxToGndReport(struct i2c_client *client)
{
	struct synaptics_ts_data* ts = (struct synaptics_ts_data*)get_touch_handle(client);

	unsigned char ImageBuffer[CFG_F54_TXCOUNT];
	unsigned char ImageArray[CFG_F54_TXCOUNT];
	unsigned char result = 0;
	int i, k;
	int shift;
	unsigned char command;

	TOUCH_DEBUG_MSG("%s START!! disable_irq_nosync!! \n", __func__);
	disable_irq_nosync(ts->client->irq);

	printk("\nBin #: 10		Name: Transmitter To Ground Short Test\n");
	for (i=0; i<CFG_F54_TXCOUNT; i++) {
		ImageArray[i] = 0;
	}

	touch_i2c_write_byte(ts->client, PAGE_SELECT_REG, 0x01);
	touch_i2c_write_byte(ts->client, F54_DATA_BASE, 0x10);

	touch_i2c_write_byte(ts->client, F54_DATA_LOWINDEX, 0x00);
	touch_i2c_write_byte(ts->client, F54_DATA_HIGHINDEX, 0x00);

	touch_i2c_write_byte(ts->client, F54_COMMAND_BASE, 0x01);

	do {
		mdelay(1);
		touch_i2c_read(ts->client, F54_COMMAND_BASE, sizeof(command), &command);
	} while (command != 0x00);

	touch_i2c_read(ts->client, F54_DATA_BUFFER, 2, &ImageBuffer[0]);

	k = 0;
	for (i=0; i<CFG_F54_TXCOUNT; i++) {
		k = i / 8;
		shift = i % 8;
		if ( ImageBuffer[k] & (1 << shift) ) ImageArray[i] = 1;
	}

	printk("Column:\t");
	for (i=0; i< CFG_F54_TXCOUNT; i++) {
		printk("Tx%d,\t", i);
	}
	printk("\n");

	printk("0:\t");
	for (i=0; i<CFG_F54_TXCOUNT; i++) {
		if (ImageArray[i]) {
			result++;
			printk("%d,\t", ImageArray[i]);
		} else {
			printk("%d(*),\t", ImageArray[i]);
		}
	}
	printk("\n");

	touch_i2c_write_byte(ts->client, PAGE_SELECT_REG, 0x00);
	touch_i2c_write_byte(ts->client, F01_CMD_BASE, 0x01);
	mdelay(200);
	touch_i2c_read(ts->client, F01_DATA_BASE + 1, sizeof(command), &command);

	if (result == CFG_F54_TXCOUNT) {
		printk("Test Result : Pass\n");
	} else {
		printk("Test Result : Fail\n");
	}

	enable_irq(ts->client->irq);
	TOUCH_DEBUG_MSG("%s END!! enable_irq!!!\n", __func__);

	return 0;
}

short ImageArray2[CFG_F54_RXCOUNT][CFG_F54_RXCOUNT];
unsigned char F54_RxToRxReport(struct i2c_client *client)
{
	struct synaptics_ts_data* ts = (struct synaptics_ts_data*)get_touch_handle(client);

	unsigned char result = 0;
	short DiagonalLowerLimit = 900;
	short DiagonalUpperLimit = 1100;
	short OthersLowerLimit = -100;
	short OthersUpperLimit = 100;
	int i, j, k;
	int length;
	unsigned char command;

	TOUCH_DEBUG_MSG("%s START!! disable_irq_nosync!! \n", __func__);
	disable_irq_nosync(ts->client->irq);

	printk("\nBin #: 7		Name: Receiver To Receiver Short Test\n");
	printk("\n\t");
	for (j = 0; j < CFG_F54_RXCOUNT; j++) printk("R%d\t", j);
	printk("\n");

	length = CFG_F54_RXCOUNT * CFG_F54_TXCOUNT * 2;

	touch_i2c_write_byte(ts->client, PAGE_SELECT_REG, 0x01);

	touch_i2c_write_byte(ts->client, F54_DATA_BASE, 0x07);
	touch_i2c_write_byte(ts->client, F54_CBC_SETTINGS, 0x00);
	touch_i2c_write_byte(ts->client, NOISE_MITIGATION, 0x01);
	touch_i2c_write_byte(ts->client, F54_COMMAND_BASE, 0x04);

	do {
		mdelay(1);
		touch_i2c_read(ts->client, F54_COMMAND_BASE, sizeof(command), &command);
	} while (command != 0x00);

	touch_i2c_write_byte(ts->client, F54_COMMAND_BASE, 0x02);

	do {
		mdelay(1);
		touch_i2c_read(ts->client, F54_COMMAND_BASE, sizeof(command), &command);
	} while (command != 0x00);

	touch_i2c_write_byte(ts->client, F54_DATA_LOWINDEX, 0x00);
	touch_i2c_write_byte(ts->client, F54_DATA_HIGHINDEX, 0x00);

	touch_i2c_write_byte(ts->client, F54_COMMAND_BASE, 0x01);

	do {
		mdelay(1);
		touch_i2c_read(ts->client, F54_COMMAND_BASE, sizeof(command), &command);
	} while (command != 0x00);

	touch_i2c_read(ts->client, F54_DATA_BUFFER, length, &ImageBuffer[0]);

	k = 0;
	for (i = 0; i < CFG_F54_TXCOUNT; i++)
	{
		for (j = 0; j < CFG_F54_RXCOUNT; j++)
		{
			ImageArray2[i][j] = (short)(ImageBuffer[k] | (ImageBuffer[k+1] << 8));
			k = k + 2;
		}
	}

	length = CFG_F54_RXCOUNT * (CFG_F54_RXCOUNT-CFG_F54_TXCOUNT) * 2;
	touch_i2c_write_byte(ts->client, F54_DATA_BASE, 0x11);
	touch_i2c_write_byte(ts->client, F54_DATA_LOWINDEX, 0x00);
	touch_i2c_write_byte(ts->client, F54_DATA_HIGHINDEX, 0x00);
	touch_i2c_write_byte(ts->client, F54_COMMAND_BASE, 0x01);

	do {
		mdelay(1);
		touch_i2c_read(ts->client, F54_COMMAND_BASE, sizeof(command), &command);
	} while (command != 0x00);

	touch_i2c_read(ts->client, F54_DATA_BUFFER, length, &ImageBuffer[0]);

	k = 0;
	for (i = 0; i < (CFG_F54_RXCOUNT-CFG_F54_TXCOUNT); i++)
	{
		for (j = 0; j < CFG_F54_RXCOUNT; j++)
		{
			ImageArray2[CFG_F54_TXCOUNT+i][j] = (short)(ImageBuffer[k] | (ImageBuffer[k+1] << 8));
			k = k + 2;
		}
	}

	for (i = 0; i < CFG_F54_RXCOUNT; i++)
	{
		printk("R%d\t", i);
		for (j = 0; j < CFG_F54_RXCOUNT; j++)
		{
			if (i == j)
			{
				if((ImageArray2[i][j] <= DiagonalUpperLimit) && (ImageArray2[i][j] >= DiagonalLowerLimit))
				{
					result++; //Pass
					printk("%d\t", ImageArray2[i][j]);
				}
				else
				{
					printk("%d(*)\t", ImageArray2[i][j]);
				}
			}
			else
			{
				if((ImageArray2[i][j] <= OthersUpperLimit) && (ImageArray2[i][j] >= OthersLowerLimit))
				{
					result++; //Pass
					printk("%d\t", ImageArray2[i][j]);
				}
				else
				{
					printk("%d(*)\t", ImageArray2[i][j]);
				}
			}
		}
		printk("\n");
	}

	touch_i2c_write_byte(ts->client, F54_COMMAND_BASE, 0x02);
	do {
		mdelay(1);
		touch_i2c_read(ts->client, F54_COMMAND_BASE, sizeof(command), &command);
	} while (command != 0x00);

	touch_i2c_write_byte(ts->client, PAGE_SELECT_REG, 0x00);
	touch_i2c_write_byte(ts->client, F01_CMD_BASE, 0x01);
	mdelay(200);
	touch_i2c_read(ts->client, F01_DATA_BASE + 1, sizeof(command), &command);

	if (result == CFG_F54_RXCOUNT* CFG_F54_RXCOUNT) {
		printk("Test Result : Pass\n");
	} else {
		printk("Test Result : Fail\n");
	}

	enable_irq(ts->client->irq);
	TOUCH_DEBUG_MSG("%s END!! enable_irq!!!\n", __func__);

	return 0;
}

unsigned char F54_HighResistance(struct i2c_client *client, char *buf)
{
	struct synaptics_ts_data* ts = (struct synaptics_ts_data*)get_touch_handle(client);

	unsigned char imageBuffer[6];
	short resistance[3];
	int i, result = 0, iRtn = 0;
	unsigned char command;
	int resistanceLimit[3][2] = {{-1000, 450}, {-1000, 450}, {-400, 20}};

	TOUCH_DEBUG_MSG("%s START!! disable_irq_nosync!! \n", __func__);
	disable_irq_nosync(ts->client->irq);

	touch_i2c_write_byte(ts->client, PAGE_SELECT_REG, 0x01);
	touch_i2c_write_byte(ts->client, F54_DATA_BASE, 0x04);
	touch_i2c_write_byte(ts->client, F54_COMMAND_BASE, 0x04);

	do {
		mdelay(1);
		touch_i2c_read(ts->client, F54_COMMAND_BASE, sizeof(command), &command);
	} while (command != 0x00);

	touch_i2c_write_byte(ts->client, F54_COMMAND_BASE, 0x02);

	do {
		mdelay(1);
		touch_i2c_read(ts->client, F54_COMMAND_BASE, sizeof(command), &command);
	} while (command != 0x00);

	touch_i2c_write_byte(ts->client, F54_DATA_LOWINDEX, 0x00);
	touch_i2c_write_byte(ts->client, F54_DATA_HIGHINDEX, 0x00);

	touch_i2c_write_byte(ts->client, F54_COMMAND_BASE, 0x01);

	do {
		mdelay(1);
		touch_i2c_read(ts->client, F54_COMMAND_BASE, sizeof(command), &command);
	} while (command != 0x00);

	touch_i2c_read(ts->client, F54_DATA_BUFFER, 6, &imageBuffer[0]);

	printk("Parameters:\t");
	if (buf != NULL) iRtn += sprintf(buf + iRtn, "Parameters:\t");
	for (i=0; i<3; i++) {
		resistance[i] = (short)((imageBuffer[i*2+1] << 8) | imageBuffer[i*2]);
		printk("%d.%03d,\t\t", resistance[i]/1000, resistance[i]%1000);
		if (buf != NULL) iRtn += sprintf(buf + iRtn, "%d.%03d,\t\t", resistance[i]/1000, resistance[i]%1000);
		if ((resistance[i] >= resistanceLimit[i][0]) && (resistance[i] <= resistanceLimit[i][1])) result ++;
	}
	printk("\n");
	if (buf != NULL) iRtn += sprintf(buf + iRtn, "\n");

	printk("Limits:\t\t");
	if (buf != NULL) iRtn += sprintf(buf + iRtn, "Limits:\t\t");
	for (i=0; i<3; i++) {
		printk("%d.%03d,%d.%03d\t", resistanceLimit[i][0]/1000, resistanceLimit[i][0]%1000, resistanceLimit[i][1]/1000, resistanceLimit[i][1]%1000);
		if (buf != NULL) iRtn += sprintf(buf + iRtn, "%d.%03d,%d.%03d\t", resistanceLimit[i][0]/1000, resistanceLimit[i][0]%1000, resistanceLimit[i][1]/1000, resistanceLimit[i][1]%1000);
	}
	printk("\n");
	if (buf != NULL) iRtn += sprintf(buf + iRtn, "\n");

	touch_i2c_write_byte(ts->client, F54_COMMAND_BASE, 0x02);
	do {
		mdelay(1);
		touch_i2c_read(ts->client, F54_COMMAND_BASE, sizeof(command), &command);
	} while (command != 0x00);

	touch_i2c_write_byte(ts->client, PAGE_SELECT_REG, 0x00);
	touch_i2c_write_byte(ts->client, F01_CMD_BASE, 0x01);
	mdelay(200);
	touch_i2c_read(ts->client, F01_DATA_BASE + 1, sizeof(command), &command);

	if (result == 3) {
		printk("Test Result : Pass\n");
		if (buf != NULL) iRtn += sprintf(buf + iRtn, "Test Result : Pass\n");
	} else {
		printk("Test Result : Fail\n");
		if (buf != NULL) iRtn += sprintf(buf + iRtn, "Test Result : Fail\n");
	}
	enable_irq(ts->client->irq);
	TOUCH_DEBUG_MSG("%s END!! enable_irq!!!\n", __func__);
	return iRtn;
}

unsigned char F54_RxOpenReport(struct i2c_client *client)
{
	struct synaptics_ts_data* ts = (struct synaptics_ts_data*)get_touch_handle(client);
	int result = 0;
	/*short DiagonalLowerLimit = 900;*/
	/*short DiagonalUpperLimit = 1100;*/
	short OthersLowerLimit = -100;
	short OthersUpperLimit = 100;

	int i, j, k;
	int length;
	unsigned char command;

	TOUCH_DEBUG_MSG("%s START!! disable_irq_nosync!! \n", __func__);
	disable_irq_nosync(ts->client->irq);

	for (j=0; j<CFG_F54_RXCOUNT; j++)
		printk("R%d\t", j);
	printk("\n");

	length = CFG_F54_RXCOUNT * CFG_F54_TXCOUNT * 2;

	touch_i2c_write_byte(ts->client, PAGE_SELECT_REG, 0x01);
	touch_i2c_write_byte(ts->client, F54_DATA_BASE, 0x0E);
	touch_i2c_write_byte(ts->client, F54_CBC_SETTINGS, 0x00);
	touch_i2c_write_byte(ts->client, NOISE_MITIGATION, 0x01);
	touch_i2c_write_byte(ts->client, F54_COMMAND_BASE, 0x04);

	do {
		mdelay(1);
		touch_i2c_read(ts->client, F54_COMMAND_BASE, sizeof(command), &command);
	} while (command != 0x00);
	touch_i2c_write_byte(ts->client, F54_COMMAND_BASE, 0x02);

	do {
		mdelay(1);
		touch_i2c_read(ts->client, F54_COMMAND_BASE, sizeof(command), &command);
	} while (command != 0x00);

	touch_i2c_write_byte(ts->client, F54_DATA_LOWINDEX, 0x00);
	touch_i2c_write_byte(ts->client, F54_DATA_HIGHINDEX, 0x00);

	touch_i2c_write_byte(ts->client, F54_COMMAND_BASE, 0x01);

	do {
		mdelay(1);
		touch_i2c_read(ts->client, F54_COMMAND_BASE, sizeof(command), &command);
	} while (command != 0x00);

	touch_i2c_read(ts->client, F54_DATA_BUFFER, length, &ImageBuffer[0]);
	k = 0;
	for (i = 0; i < CFG_F54_TXCOUNT; i++)
	{
		for (j = 0; j < CFG_F54_RXCOUNT; j++)
		{
			ImageArray2[i][j] = (short)(ImageBuffer[k] | (ImageBuffer[k+1] << 8));
			k = k + 2;
		}
	}

	length = CFG_F54_RXCOUNT * (CFG_F54_RXCOUNT-CFG_F54_TXCOUNT) * 2;
	touch_i2c_write_byte(ts->client, F54_DATA_BASE, 0x12);
	touch_i2c_write_byte(ts->client, F54_DATA_LOWINDEX, 0x00);
	touch_i2c_write_byte(ts->client, F54_DATA_HIGHINDEX, 0x00);
	touch_i2c_write_byte(ts->client, F54_COMMAND_BASE, 0x01);

	do {
		mdelay(1);
		touch_i2c_read(ts->client, F54_COMMAND_BASE, sizeof(command), &command);
	} while (command != 0x00);

	touch_i2c_read(ts->client, F54_DATA_BUFFER, length, &ImageBuffer[0]);

	k = 0;
	for (i = 0; i < (CFG_F54_RXCOUNT-CFG_F54_TXCOUNT); i++)
	{
		for (j = 0; j < CFG_F54_RXCOUNT; j++)
		{
			ImageArray2[CFG_F54_TXCOUNT+i][j] = (short)(ImageBuffer[k] | (ImageBuffer[k+1] << 8));
			k = k + 2;
		}
	}

#if 1
	for (i = 0; i < CFG_F54_RXCOUNT; i++)
	{
		printk("R%d\t", i);
		for (j = 0; j < CFG_F54_RXCOUNT; j++)
		{
			if((ImageArray2[i][j] <= OthersUpperLimit) && (ImageArray2[i][j] >= OthersLowerLimit)) {
				result ++;
				printk("%d\t", ImageArray2[i][j]);
			} else {
				printk("%d(*)\t", ImageArray2[i][j]);
			}
		}
		printk("\n");
	}
#else
	for (i = 0; i < CFG_F54_RXCOUNT; i++)
	{
		printk("R%d\t", i);
		for (j = 0; j < CFG_F54_RXCOUNT; j++)
		{
			if (i == j)
			{
				if((ImageArray[i][j] <= DiagonalUpperLimit) && (ImageArray[i][j] >= DiagonalLowerLimit))
				{
					result++; //Pass
					printk("%d\t", ImageArray[i][j]);
				}
				else
				{
					printk("%d(*)\t", ImageArray[i][j]);
				}
			}
			else
			{
				if((ImageArray[i][j] <= OthersUpperLimit) && (ImageArray[i][j] >= OthersLowerLimit))
				{
					result++; //Pass
					printk("%d\t", ImageArray[i][j]);
				}
				else
				{
					printk("%d(*)\t", ImageArray[i][j]);
				}
			}
		}
		printk("\n");
	}
#endif
	touch_i2c_write_byte(ts->client, F54_COMMAND_BASE, 0x01);
	do {
		mdelay(1);
		touch_i2c_read(ts->client, F54_COMMAND_BASE, sizeof(command), &command);
	} while (command != 0x00);

	touch_i2c_write_byte(ts->client, PAGE_SELECT_REG, 0x00);
	touch_i2c_write_byte(ts->client, F01_CMD_BASE, 0x01);
	mdelay(200);
	touch_i2c_read(ts->client, F01_DATA_BASE + 1, sizeof(command), &command);

	if (result == CFG_F54_RXCOUNT* CFG_F54_RXCOUNT) {
		printk("Test Result : Pass\n");
	} else {
		printk("Test Result : Fail\n");
	}

	enable_irq(ts->client->irq);
	TOUCH_DEBUG_MSG("%s END!! enable_irq!!!\n", __func__);

	return 0;
}

unsigned char F54_TxOpenReport(struct i2c_client *client)
{
	struct synaptics_ts_data* ts = (struct synaptics_ts_data*)get_touch_handle(client);

	unsigned char ImageBuffer[CFG_F54_TXCOUNT];
	unsigned char ImageArray[CFG_F54_TXCOUNT];
	unsigned char result = 0;
	int i, k;
	int shift;
	unsigned char command;

	TOUCH_DEBUG_MSG("%s START!! disable_irq_nosync!! \n", __func__);
	disable_irq_nosync(ts->client->irq);

	for (i=0; i<CFG_F54_TXCOUNT; i++) {
		ImageArray[i] = 0;
	}

	touch_i2c_write_byte(ts->client, PAGE_SELECT_REG, 0x01);
	touch_i2c_write_byte(ts->client, F54_DATA_BASE, 0x0F);

	touch_i2c_write_byte(ts->client, F54_DATA_LOWINDEX, 0x00);
	touch_i2c_write_byte(ts->client, F54_DATA_HIGHINDEX, 0x00);

	touch_i2c_write_byte(ts->client, F54_COMMAND_BASE, 0x01);

	do {
		mdelay(1);
		touch_i2c_read(ts->client, F54_COMMAND_BASE, sizeof(command), &command);
	} while (command != 0x00);

	touch_i2c_read(ts->client, F54_DATA_BUFFER, 2, &ImageBuffer[0]);

	k = 0;
	for (i=0; i<CFG_F54_TXCOUNT; i++) {
		k = i / 8;
		shift = i % 8;
		if ( ImageBuffer[k] & (1 << shift) ) ImageArray[i] = 0;
	}

	printk("Column:\t");
	for (i=0; i< CFG_F54_TXCOUNT; i++) {
		printk("Tx%d,\t", i);
	}
	printk("\n");

	printk("0:\t");
	for (i=0; i<CFG_F54_TXCOUNT; i++) {
		if (!ImageArray[i]) {
			result++;
			printk("%d,\t", ImageArray[i]);
		} else {
			printk("%d(*),\t", ImageArray[i]);
		}
	}
	printk("\n");

	touch_i2c_write_byte(ts->client, PAGE_SELECT_REG, 0x00);
	touch_i2c_write_byte(ts->client, F01_CMD_BASE, 0x01);
	mdelay(200);
	touch_i2c_read(ts->client, F01_DATA_BASE + 1, sizeof(command), &command);

	if (result == CFG_F54_TXCOUNT) {
		printk("Test Result : Pass\n");
	} else {
		printk("Test Result : Fail\n");
	}

	enable_irq(ts->client->irq);
	TOUCH_DEBUG_MSG("%s END!! enable_irq!!!\n", __func__);

	return 0;
}
