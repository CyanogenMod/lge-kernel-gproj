/*
   +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   Copyright (c) 2011 Synaptics, Inc.
   
   Permission is hereby granted, free of charge, to any person obtaining a copy of 
   this software and associated documentation files (the "Software"), to deal in 
   the Software without restriction, including without limitation the rights to use, 
   copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the 
   Software, and to permit persons to whom the Software is furnished to do so, 
   subject to the following conditions:
   
   The above copyright notice and this permission notice shall be included in all 
   copies or substantial portions of the Software.
   
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
   SOFTWARE.
 */

#include "RefCode.h"
#include "RefCode_PDTScan.h"

#ifdef _F54_TEST_
#include "RefCode_F54_FullRawCapLimit.h"

#ifdef F54_Porting
static unsigned char ImageBuffer[CFG_F54_TXCOUNT*CFG_F54_RXCOUNT*2];
static signed ImageArray[CFG_F54_TXCOUNT] [CFG_F54_RXCOUNT];

static char buf[6000] = {0};
static unsigned char call_cnt = 0;
static int LimitBack[30][46*2];
#endif

unsigned char F54_FullRawCap(int mode)
// mode - 0:For sensor, 1:For FPC, 2:CheckTSPConnection, 3:Baseline, 4:Delta image
{
   signed short temp=0;
   int Result = 0;
#ifdef F54_Porting
	int ret = 0;
	unsigned char product_id[11];
#endif
    int TSPCheckLimit=700;
	short Flex_LowerLimit = -100;
	short Flex_UpperLimit = 500;

	int i, j, k;
	unsigned short length;
	
	unsigned char command;

	length = numberOfTx * numberOfRx* 2;

	//check product id to set basecap array
	readRMI(F01_Query_Base+11, &product_id[0], sizeof(product_id));
	
	if(!strncmp(product_id, "TM2369", 6)) {
		printk("set limit array to TPK value.\n");
		memcpy(Limit, Limit_TPK, sizeof(Limit_TPK));
	} else if(!strncmp(product_id, "PLG192", 6)) {
		printk("set limit array to PLG192 value.\n");
		memcpy(Limit, Limit_PLG192, sizeof(Limit_PLG192));
	} else if(!strncmp(product_id, "PLG193", 6)) {
		printk("set limit array to PLG193 value.\n");
		memcpy(Limit, Limit_PLG193, sizeof(Limit_PLG193));
	} else if(!strncmp(product_id, "PLG121", 6)) {
		printk("set limit array to PLG121 value.\n");
		memcpy(Limit, Limit_PLG121, sizeof(Limit_PLG121));
	} else {
		printk("set limit array to LGIT value.\n");
	}
	
	//set limit array
	if(call_cnt == 0){
		printk("Backup Limit to LimitBack array\n");
		memset(LimitBack, 0, sizeof(LimitBack));
		memcpy(LimitBack, Limit, sizeof(LimitBack));
	}
	if(get_limit(numberOfTx, numberOfRx) > 0) {
		printk("Get limit from file success!!Use Limit array from file data.\n");
		memcpy(Limit, LimitFile, sizeof(Limit));
	} else {
		printk("Get limit from file fail!!Use Limit array from image data\n");
		memcpy(Limit, LimitBack, sizeof(Limit));
	}
	call_cnt++;
	if(call_cnt > 0) call_cnt = 1;

	if(mode == 3 || mode == 4)
	{
/*		// Disable CBC
		command = 0x00;
		writeRMI(F54_CBCSettings, &command, 1);
		writeRMI(F54_CBCPolarity, &command, 1);
*/	}

	// No sleep
	command = 0x04;
	writeRMI(F01_Ctrl_Base, &command, 1);

	// Set report mode to to run the AutoScan
	if(mode >= 0 && mode < 4)	command = 0x03;
	if(mode == 4)				command = 0x02;
	writeRMI(F54_Data_Base, &command, 1);

	if(mode == 3 || mode == 4)
	{
/*		//NoCDM4
		command = 0x01;
		writeRMI(NoiseMitigation, &command, 1);
*/	}

	if(mode != 3 && mode != 4)
	{
		// Force update & Force cal
		command = 0x06;
		writeRMI(F54_Command_Base, &command, 1);

		do {
			delayMS(1); //wait 1ms
			readRMI(F54_Command_Base, &command, 1);
		} while (command != 0x00);
	}

	// Enabling only the analog image reporting interrupt, and turn off the rest
	command = 0x08;
	writeRMI(F01_Cmd_Base+1, &command, 1);

	command = 0x00;
	writeRMI(F54_Data_LowIndex, &command, 1);
	writeRMI(F54_Data_HighIndex, &command, 1);
 
	// Set the GetReport bit to run the AutoScan
	command = 0x01;
	writeRMI(F54_Command_Base, &command, 1);
 
	// Wait until the command is completed
	do {
	 delayMS(1); //wait 1ms
		readRMI(F54_Command_Base, &command, 1);
   } while (command != 0x00);
 
	readRMI(F54_Data_Buffer, &ImageBuffer[0], length);

	if( (numberOfTx > 29) || (numberOfRx > 45) ) {
		printk("Limit Index overflow. Test result: Fail\n");
		return 0;
	}

	switch(mode)
	{
		case 0:
		case 1:
#ifdef F54_Porting
			memset(buf, 0, sizeof(buf));
			ret = sprintf(buf, "#ofTx\t%d\n", numberOfTx);
			ret += sprintf(buf+ret, "#ofRx\t%d\n", numberOfRx);

			ret += sprintf(buf+ret, "\n#3.03	Full raw capacitance Test\n");
#else
			printk("#ofTx\t%d\n", numberOfTx);
			printk("#ofRx\t%d\n", numberOfRx);

			printk("\n#3.03	Full raw capacitance Test\n");
#endif
			k = 0;	   
			for (i = 0; i < numberOfTx; i++)
			{
#ifdef F54_Porting
				ret += sprintf(buf+ret, "%d\t", i);
#else
				printk("%d\t", i);
#endif
				for (j = 0; j < numberOfRx; j++)
				{
						temp = (short)(ImageBuffer[k] | (ImageBuffer[k+1] << 8));
						if(CheckButton[i][j] != 1)
						{
							if(mode==0)
							{
#ifdef F54_Porting
								if ((temp >= Limit[i][j*2]) && (temp <= Limit[i][j*2+1]))
#else
								if ((temp >= Limit[i][j*2]*1000) && (temp <= Limit[i][j*2+1]*1000))
#endif
								{
									Result++;
#ifdef F54_Porting									
									ret += sprintf(buf+ret, "%d\t", temp); //It's for getting log for limit set
#else
									printk("%d\t", temp); //It's for getting log for limit set
#endif
								}
								else {
#ifdef F54_Porting
									ret += sprintf(buf+ret, "%d\t", temp); //It's for getting log for limit set
#else
									printk("%d\t", temp); //It's for getting log for limit set
#endif
								}
							}
							else
							{
								if ((temp >= Flex_LowerLimit) && (temp <= Flex_UpperLimit))
								{
									Result++;
#ifdef F54_Porting
									ret += sprintf(buf+ret, "%d\t", temp); //It's for getting log for limit set
#else
									printk("%d\t", temp); //It's for getting log for limit set
#endif									
								}
								else {
#ifdef F54_Porting
									ret += sprintf(buf+ret, "%d\t", temp); //It's for getting log for limit set
#else
									printk("%d\t", temp); //It's for getting log for limit set
#endif
								}
							}
						}
						else
						{
							Result++;
#ifdef F54_Porting
							ret += sprintf(buf+ret, "%d\t", temp); //It's for getting log for limit set
#else
							printk("%d\t", temp); //It's for getting log for limit set
#endif							
						}
				  k = k + 2;
				}
#ifdef F54_Porting
				ret += sprintf(buf+ret, "\n");
#else
				printk("\n"); //It's for getting log for limit set
#endif				
			}

#ifdef F54_Porting
			ret += sprintf(buf+ret, "#3.04	Full raw capacitance Test END\n");
			ret += sprintf(buf+ret, "\n#3.01	Full raw capacitance Test Limit\n");

			// Print Capacitance Imgae for getting log for Excel format
			ret += sprintf(buf+ret, "\t");
#else
			printk("#3.04	Full raw capacitance Test END\n");

			printk("\n#3.01	Full raw capacitance Test Limit\n");
			// Print Capacitance Imgae for getting log for Excel format
			printk("\t");
#endif			
			for (j = 0; j < numberOfRx; j++) {
#ifdef F54_Porting
				ret += sprintf(buf+ret, "%d-Min\t%d-Max\t", j, j);
#else
				printk("%d-Min\t%d-Max\t", j, j);
#endif
			}

#ifdef F54_Porting
			ret += sprintf(buf+ret, "\n");
#else
			printk("\n");
#endif

			for (i = 0; i < numberOfTx; i++)
			{
#ifdef F54_Porting
				ret += sprintf(buf+ret, "%d\t", i);
#else
				printk("%d\t", i);
#endif
				for (j = 0; j < numberOfRx; j++)
				{
					if(mode==0) {
#ifdef F54_Porting
						//printk("%d\t%d\t", Limit[i][j*2], Limit[i][j*2+1]);
						ret += sprintf(buf+ret, "%d\t%d\t", Limit[i][j*2], Limit[i][j*2+1]);
#else
						printk("%d\t%d\t", Limit[i][j*2]*1000, Limit[i][j*2+1]*1000);
#endif
					} else {
#ifdef F54_Porting
						ret += sprintf(buf+ret, "%d\t%d\t", Flex_LowerLimit, Flex_UpperLimit);
#else
						printk("%d\t%d\t", Flex_LowerLimit, Flex_UpperLimit);
#endif
					}
				}
#ifdef F54_Porting
				ret += sprintf(buf+ret, "\n");
#else
				printk("\n");
#endif
			}
#ifdef F54_Porting
			ret += sprintf(buf+ret, "#3.02	Full raw cap Limit END\n");
#else
			printk("#3.02	Full raw cap Limit END\n");
#endif
			break;

		case 2:
			k = 0;	   
			for (i = 0; i < numberOfTx; i++)
			{
				for (j = 0; j < numberOfRx; j++)
				{
						temp = (short)(ImageBuffer[k] | (ImageBuffer[k+1] << 8));
						   if (temp > TSPCheckLimit)
						   Result++;
				  k = k + 2;
				}	   
			}
			break;

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
#ifdef F54_Porting
				 // Print Capacitance Imgae for getting log for Excel format
				 ret += sprintf(ret+buf, "\n\t");
#else
				 // Print Capacitance Imgae for getting log for Excel format
				 printk("\n\t");
#endif
				 for (j = 0; j < numberOfRx; j++) {
#ifdef F54_Porting
					ret += sprintf(ret+buf, "RX%d\t", RxChannelUsed[j]);
#else
				 	printk("RX%d\t", RxChannelUsed[j]);
#endif
				 }
				 
#ifdef F54_Porting
				 ret += sprintf(buf+ret, "\n");
#else
				 printk("\n");
#endif
				 
				 for (i = 0; i < numberOfTx; i++)
				 {
#ifdef F54_Porting
					 ret += sprintf(buf+ret, "TX%d\t", TxChannelUsed[i]);
#else
					 printk("TX%d\t", TxChannelUsed[i]);
#endif
					 for (j = 0; j < numberOfRx; j++)
					 {
#ifdef F54_Porting					 
						if(mode == 3)		ret += sprintf(buf+ret, "%d\t", (ImageArray[i][j]) / 1000);
						else if(mode == 4)	ret += sprintf(buf+ret, "%d\t", ImageArray[i][j]);
#else
						if(mode == 3)	   printk("%d\t", (ImageArray[i][j]) / 1000);
						else if(mode == 4) printk("%d\t", ImageArray[i][j]);
						//if(mode == 3)	   printk("%1.3f\t", (float)(ImageArray[i][j]) / 1000);
						//else if(mode == 4) printk("%d\t", ImageArray[i][j]);
#endif
						 
					  }
#ifdef F54_Porting
				  ret += sprintf(ret+buf, "\n");
#else
				  printk("\n");
#endif
				 }
			break;

		default:
			break;
	}

	if(mode != 3 && mode != 4)
	{
		//Reset
		command= 0x01;
		writeRMI(F01_Cmd_Base, &command, 1);
		delayMS(200);
		readRMI(F01_Data_Base+1, &command, 1); //Read Interrupt status register to Interrupt line goes to high
	}
	
	if(mode >= 0 && mode < 2)
	{
		if(Result == numberOfTx * numberOfRx)
		{
#ifdef F54_Porting
			ret += sprintf(buf+ret, "Test Result: Pass\n");
			write_log(buf);
#else
			printk("Test Result: Pass\n");
#endif
			return 1; //Pass
		}
		else
		{
#ifdef F54_Porting
			ret += sprintf(buf+ret, "Test Result: Fail\n");
			write_log(buf);
#else
			printk("Test Result: Fail\n");
#endif
			return 0; //Fail
		}
	}
	else if(mode == 2)
	{
		if(Result == 0) return 0; //TSP Not connected
		else return 1; //TSP connected 
	 }
	else
	{
#ifdef F54_Porting
		write_log(buf);
#endif
		return 0;
	}
 }
#endif

