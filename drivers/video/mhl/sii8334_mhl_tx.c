#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <linux/gpio.h>
#include <linux/platform_data/mhl_device.h>
#include <linux/input.h>
#include <linux/hid.h>


#include "sii8334_mhl_tx.h"

/* LGE_CHANGE
 * do device driver initialization
 * using multithread during booting,
 * in order to reduce booting time.
 *
 * ported from G1-project
 * 2012-11-30, chaeuk.lee@lge.com
 */
#define LGE_MULTICORE_FASTBOOT
#if defined(CONFIG_MACH_LGE) && defined(LGE_MULTICORE_FASTBOOT)
#include <linux/kthread.h>
#endif

#ifdef CONFIG_LGE_MHL_REMOCON_TEST
#include <linux/proc_fs.h>
#endif

#ifdef CONFIG_MACH_LGE
#define LGE_ADOPTER_ID 612
#endif

#define SILICON_IMAGE_ADOPTER_ID 322
#define TRANSCODER_DEVICE_ID 0x8334

#define	POWER_STATE_D3				3
#define	POWER_STATE_D0_NO_MHL		2
#define	POWER_STATE_D0_MHL			0
#define	POWER_STATE_FIRST_INIT		0xFF

#define	TIMER_FOR_MONITORING				(TIMER_0)

#define TX_HW_RESET_PERIOD		10	// 10 ms.
#define TX_HW_RESET_DELAY			100

#define SII8334_I2C_DEVICE_NUMBER	5
static int sii8334_i2c_dev_index = 0;

#define MAX_I2C_MESSAGES	3		/* Maximum # of I2c message segments supported */
DEFINE_SEMAPHORE(sii8334_irq_sem);

struct i2c_client *sii8334_mhl_i2c_client[SII8334_I2C_DEVICE_NUMBER];

//
// To remember the current power state.
//
static	uint8_t	fwPowerState = POWER_STATE_FIRST_INIT;

//
// To serialize the RCP commands posted to the CBUS engine, this flag
// is maintained by the function SiiMhlTxDrvSendCbusCommand()
//
static	bool		mscCmdInProgress;	// false when it is okay to send a new command
//
// Preserve Downstream HPD status
//
static	uint8_t	dsHpdStatus = 0;

#ifdef CONFIG_MACH_LGE
static	bool		sii8334_mhl_suspended = false;
#endif

static struct mhl_platform_data *mhl_pdata[SII8334_I2C_DEVICE_NUMBER];

/*remove qup error*/
static uint8_t MHLSleepStatus = 0;

void SiiRegWrite(uint16_t virtualAddr, uint8_t value)
{
	struct i2c_client* client;
	uint16_t client_number;

	/*remove qup error*/
	if(MHLSleepStatus) return;

	client_number = (virtualAddr >> 8);

	if(client_number > SII8334_I2C_DEVICE_NUMBER)
    return;

	client = sii8334_mhl_i2c_client[client_number];
	i2c_smbus_write_byte_data(client, (virtualAddr & 0xff), value);
}

uint8_t SiiRegRead(uint16_t virtualAddr)
{
	int value;
	uint16_t client_number;
	struct i2c_client* client;

	/*remove qup error*/
	if(MHLSleepStatus) return 0xFF;

	client_number = (virtualAddr >> 8);

	if(client_number > SII8334_I2C_DEVICE_NUMBER)
		return 0xFF;

	client = sii8334_mhl_i2c_client[client_number];

	value = i2c_smbus_read_byte_data(client, (virtualAddr & 0xff));

	if (value < 0)
		return 0xFF;
	else
		return value;
}

void SiiRegModify(uint16_t virtualAddr, uint8_t mask, uint8_t value)
{
    uint8_t aByte;

    aByte = SiiRegRead( virtualAddr );
    aByte &= (~mask);                       // first clear all bits in mask
    aByte |= (mask & value);                // then set bits from value
    SiiRegWrite( virtualAddr, aByte );
}

#define WriteByteCBUS(offset,value)  SiiRegWrite(TX_PAGE_CBUS | (uint16_t)offset,value)
#define ReadByteCBUS(offset)         SiiRegRead( TX_PAGE_CBUS | (uint16_t)offset)
#define	SET_BIT(offset,bitnumber)		SiiRegModify(offset,(1<<bitnumber),(1<<bitnumber))
#define	CLR_BIT(offset,bitnumber)		SiiRegModify(offset,(1<<bitnumber),0x00)
//
//
#define	DISABLE_DISCOVERY				SiiRegModify(REG_DISC_CTRL1,BIT0,0);
#define	ENABLE_DISCOVERY				SiiRegModify(REG_DISC_CTRL1,BIT0,BIT0);

#define STROBE_POWER_ON					SiiRegModify(REG_DISC_CTRL1,BIT1,0);
/*
    Look for interrupts on INTR1
    6 - MDI_HPD  - downstream hotplug detect (DSHPD)
*/

#define INTR_1_DESIRED_MASK   (BIT6 | BIT5)
#define	UNMASK_INTR_1_INTERRUPTS		SiiRegWrite(REG_INTR1_MASK, INTR_1_DESIRED_MASK)
#define	MASK_INTR_1_INTERRUPTS			SiiRegWrite(REG_INTR1_MASK, 0x00)


//
//	Look for interrupts on INTR4 (Register 0x74)
//		7 = RSVD		(reserved)
//		6 = RGND Rdy	(interested)
//		5 = MHL DISCONNECT	(interested)
//		4 = CBUS LKOUT	(interested)
//		3 = USB EST		(interested)
//		2 = MHL EST		(interested)
//		1 = RPWR5V Change	(ignore)
//		0 = SCDT Change	(only if necessary)
//

#define	INTR_4_DESIRED_MASK				(BIT0 | BIT2 | BIT3 | BIT4 | BIT5 | BIT6)
#define	UNMASK_INTR_4_INTERRUPTS		SiiRegWrite(REG_INTR4_MASK, INTR_4_DESIRED_MASK)
#define	MASK_INTR_4_INTERRUPTS			SiiRegWrite(REG_INTR4_MASK, 0x00)

//	Look for interrupts on INTR_5 (Register ??)
//		4 = FIFO UNDERFLOW	(interested)
//		3 = FIFO OVERFLOW	(interested)

#define	INTR_5_DESIRED_MASK				(BIT3 | BIT4)
#define	UNMASK_INTR_5_INTERRUPTS		SiiRegWrite(REG_INTR5_MASK, INTR_5_DESIRED_MASK)
#define	MASK_INTR_5_INTERRUPTS			SiiRegWrite(REG_INTR5_MASK, 0x00)

//	Look for interrupts on CBUS:CBUS_INTR_STATUS [0xC8:0x08]
//		7 = RSVD			(reserved)
//		6 = MSC_RESP_ABORT	(interested)
//		5 = MSC_REQ_ABORT	(interested)
//		4 = MSC_REQ_DONE	(interested)
//		3 = MSC_MSG_RCVD	(interested)
//		2 = DDC_ABORT		(interested)
//		1 = RSVD			(reserved)
//		0 = rsvd			(reserved)
#define	INTR_CBUS1_DESIRED_MASK			(BIT2 | BIT3 | BIT4 | BIT5 | BIT6)
#define	UNMASK_CBUS1_INTERRUPTS			SiiRegWrite(TX_PAGE_CBUS | 0x0009, INTR_CBUS1_DESIRED_MASK)
#define	MASK_CBUS1_INTERRUPTS			SiiRegWrite(TX_PAGE_CBUS | 0x0009, 0x00)

#ifdef CONFIG_LG_MAGIC_MOTION_REMOCON
#define	INTR_CBUS2_DESIRED_MASK			(BIT0 | BIT2 | BIT3)
#else
#define	INTR_CBUS2_DESIRED_MASK			(BIT2 | BIT3)
#endif
#define	UNMASK_CBUS2_INTERRUPTS			SiiRegWrite(TX_PAGE_CBUS | 0x001F, INTR_CBUS2_DESIRED_MASK)
#define	MASK_CBUS2_INTERRUPTS			SiiRegWrite(TX_PAGE_CBUS | 0x001F, 0x00)

#define I2C_INACCESSIBLE -1
#define I2C_ACCESSIBLE 1

void	SiiMhlTxNotifyDsHpdChange( uint8_t dsHpdStatus );
void SiiMhlTxHwReset(uint16_t hwResetPeriod,uint16_t hwResetDelay);
void MhlTxProcessEvents(void);
void SiiMhlTxNotifyRgndMhl( void );
void	SiiMhlTxNotifyConnection( bool mhlConnected );
void	SiiMhlTxGotMhlMscMsg( uint8_t subCommand, uint8_t cmdData );
void	SiiMhlTxMscCommandDone( uint8_t data1 );
void	SiiMhlTxMscWriteBurstDone( uint8_t data1 );
void	SiiMhlTxGotMhlIntr( uint8_t intr_0, uint8_t intr_1 );
void	SiiMhlTxGotMhlStatus( uint8_t status_0, uint8_t status_1 );

static void Int1Isr (void);
static int  Int4Isr (void);
static void Int5Isr (void);
static void MhlCbusIsr (void);

static void CbusReset (void);
static void SwitchToD0 (void);
static void SwitchToD3 (void);
static void WriteInitialRegisterValues (void);
static void InitCBusRegs (void);
static void ForceUsbIdSwitchOpen (void);
static void ReleaseUsbIdSwitchOpen (void);
static void MhlTxDrvProcessConnection (void);
static void MhlTxDrvProcessDisconnection (void);
static void ProcessScdtStatusChange (void);

MhlTxNotifyEventsStatus_e  AppNotifyMhlEvent(uint8_t eventCode, uint8_t eventParam);
bool SiiMhlTxRcpeSend( uint8_t rcpeErrorCode );
bool SiiMhlTxReadDevcap( uint8_t offset );
bool SiiMhlTxSetPathEn(void );
bool SiiMhlTxClrPathEn( void );
bool SiiMhlTxRcpkSend( uint8_t rcpKeyCode );
void  AppNotifyMhlDownStreamHPDStatusChange(bool connected);

bool tmdsPoweredUp;
bool mhlConnected;
uint8_t g_chipRevId;


#ifdef CONFIG_MACH_LGE

struct mhl_common_type
{
    void  (*send_uevent)(char *buf);
    void (*hdmi_hpd_on)(int on);
};

struct mhl_common_type  *mhl_common_state;

extern void hdmi_common_send_uevent(char *buf);
extern void hdmi_common_set_hpd_on(int on);


#ifdef CONFIG_LG_MAGIC_MOTION_REMOCON
#define	MAGIC_CANVAS_X	1280
#define	MAGIC_CANVAS_Y	720
#define	MHD_SCRATCHPAD_SIZE	16
#define	MHD_MAX_BUFFER_SIZE	MHD_SCRATCHPAD_SIZE	// manually define highest number
static int	tvCtl_x = MAGIC_CANVAS_X;
static int	tvCtl_y = MAGIC_CANVAS_Y;

static unsigned short kbd_key_pressed[2];

void SiiMhlTxReadScratchpad(void);
void MhlControl(void);

#endif

#endif /* CONFIG_MACH_LGE */


///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxDrvAcquireUpstreamHPDControl
//
// Acquire the direct control of Upstream HPD.
//
void SiiMhlTxDrvAcquireUpstreamHPDControl (void)
{
	// set reg_hpd_out_ovr_en to first control the hpd
	SET_BIT(REG_INT_CTRL, 4);
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxDrvAcquireUpstreamHPDControlDriveLow
//
// Acquire the direct control of Upstream HPD.
//
void SiiMhlTxDrvAcquireUpstreamHPDControlDriveLow (void)
{
	// set reg_hpd_out_ovr_en to first control the hpd and clear reg_hpd_out_ovr_val
 	SiiRegModify(REG_INT_CTRL, BIT5 | BIT4, BIT4);	// Force upstream HPD to 0 when not in MHL mode.
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxDrvReleaseUpstreamHPDControl
//
// Release the direct control of Upstream HPD.
//
void SiiMhlTxDrvReleaseUpstreamHPDControl (void)
{
   	// Un-force HPD (it was kept low, now propagate to source
	// let HPD float by clearing reg_hpd_out_ovr_en
   	CLR_BIT(REG_INT_CTRL, 4);
}

static void Int1Isr(void)
{
	uint8_t regIntr1;
    regIntr1 = SiiRegRead(REG_INTR1);

    if (regIntr1)
    {
        // Clear all interrupts coming from this register.
        SiiRegWrite(REG_INTR1,regIntr1);

        if (BIT6 & regIntr1)
        {
        	uint8_t cbusStatus;
        	//
        	// Check if a SET_HPD came from the downstream device.
        	//
        	cbusStatus = SiiRegRead(TX_PAGE_CBUS | 0x000D);

        	// CBUS_HPD status bit
        	if(BIT6 & (dsHpdStatus ^ cbusStatus))
        	{
            	uint8_t status = cbusStatus & BIT6;

        		// Inform upper layer of change in Downstream HPD
        		SiiMhlTxNotifyDsHpdChange( status );

                if (status)
                {
                    SiiMhlTxDrvReleaseUpstreamHPDControl();  // this triggers an EDID read if control has not yet been released
                }

        		// Remember
        		dsHpdStatus = cbusStatus;
        	}
        }
    }
}


////////////////////////////////////////////////////////////////////
//
// E X T E R N A L L Y    E X P O S E D   A P I    F U N C T I O N S
//
////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxChipInitialize
//
// Chip specific initialization.
// This function is for SiI 8332/8336 Initialization: HW Reset, Interrupt enable.
//
//////////////////////////////////////////////////////////////////////////////
bool SiiMhlTxChipInitialize (void)
{
	tmdsPoweredUp = false;
	mhlConnected = false;
	mscCmdInProgress = false;	// false when it is okay to send a new command
	dsHpdStatus = 0;
	fwPowerState = POWER_STATE_D0_MHL;

    g_chipRevId = SiiRegRead(TX_PAGE_L0 | 0x04);
	/*LGE block msleep*/
	//msleep(TX_HW_RESET_PERIOD + TX_HW_RESET_DELAY);

	// setup device registers. Ensure RGND interrupt would happen.
	WriteInitialRegisterValues();

    // check of PlatformGPIOGet(pinAllowD3) is done inside SwitchToD3
	SwitchToD3();

	return true;
}


///////////////////////////////////////////////////////////////////////////////
// SiiMhlTxDeviceIsr
//
// This function must be called from a master interrupt handler or any polling
// loop in the host software if during initialization call the parameter
// interruptDriven was set to true. SiiMhlTxGetEvents will not look at these
// events assuming firmware is operating in interrupt driven mode. MhlTx component
// performs a check of all its internal status registers to see if a hardware event
// such as connection or disconnection has happened or an RCP message has been
// received from the connected device. Due to the interruptDriven being true,
// MhlTx code will ensure concurrency by asking the host software and hardware to
// disable interrupts and restore when completed. Device interrupts are cleared by
// the MhlTx component before returning back to the caller. Any handling of
// programmable interrupt controller logic if present in the host will have to
// be done by the caller after this function returns back.

// This function has no parameters and returns nothing.
//
// This is the master interrupt handler for 9244. It calls sub handlers
// of interest. Still couple of status would be required to be picked up
// in the monitoring routine (Sii9244TimerIsr)
//
// To react in least amount of time hook up this ISR to processor's
// interrupt mechanism.
//
// Just in case environment does not provide this, set a flag so we
// call this from our monitor (Sii9244TimerIsr) in periodic fashion.
//
// Device Interrupts we would look at
//		RGND		= to wake up from D3
//		MHL_EST 	= connection establishment
//		CBUS_LOCKOUT= Service USB switch
//		CBUS 		= responder to peer messages
//					  Especially for DCAP etc time based events
//
void SiiMhlTxDeviceIsr (void)
{
	uint8_t intMStatus, i; //master int status
	//
	// Look at discovery interrupts if not yet connected.
	//

	i=0;

	do
	{
    	if( POWER_STATE_D0_MHL != fwPowerState )
    	{
    		//
    		// Check important RGND, MHL_EST, CBUS_LOCKOUT and SCDT interrupts
    		// During D3 we only get RGND but same ISR can work for both states
    		//
    		if (I2C_INACCESSIBLE == Int4Isr())
    		{
                return; // don't do any more I2C traffic until the next interrupt.
    		}
    	}
    	else if( POWER_STATE_D0_MHL == fwPowerState )
    	{		
    		if (I2C_INACCESSIBLE == Int4Isr())
    		{
    			return; // don't do any more I2C traffic until the next interrupt.
    		}
    		// it is no longer necessary to check if the call to Int4Isr()
    		//  put us into D3 mode, since we now return immediately in that case
  			Int5Isr();

  			// Check for any peer messages for DCAP_CHG etc
  			// Dispatch to have the CBUS module working only once connected.
  			MhlCbusIsr();
  			Int1Isr();
    	}

    	if( POWER_STATE_D3 != fwPowerState )
    	{
    		// Call back into the MHL component to give it a chance to
    		// take care of any message processing caused by this interrupt.
    		MhlTxProcessEvents();
    	}

		intMStatus = SiiRegRead(TX_PAGE_L0 | 0x0070);	// read status
		if(0xFF == intMStatus)
		{
			intMStatus = 0;
		}
		i++;

		intMStatus &= 0x01; //RG mask bit 0
	} while (intMStatus);
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxDrvTmdsControl
//
// Control the TMDS output. MhlTx uses this to support RAP content on and off.
//
void SiiMhlTxDrvTmdsControl (bool enable)
{
	if( enable )
	{
		SET_BIT(REG_TMDS_CCTRL, 4);
        SiiMhlTxDrvReleaseUpstreamHPDControl();  // this triggers an EDID read
	}
	else
	{
		CLR_BIT(REG_TMDS_CCTRL, 4);
	}
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxDrvNotifyEdidChange
//
// MhlTx may need to inform upstream device of an EDID change. This can be
// achieved by toggling the HDMI HPD signal or by simply calling EDID read
// function.
//
void SiiMhlTxDrvNotifyEdidChange (void)
{
	//
	// Prepare to toggle HPD to upstream
	//
    SiiMhlTxDrvAcquireUpstreamHPDControl();

	// reg_hpd_out_ovr_val = LOW to force the HPD low
	CLR_BIT(REG_INT_CTRL, 5);

	// wait a bit
	msleep(110);

	// release HPD back to high by reg_hpd_out_ovr_val = HIGH
	SET_BIT(REG_INT_CTRL, 5);

    // release control to allow transcoder to modulate for CLR_HPD and SET_HPD
    SiiMhlTxDrvReleaseUpstreamHPDControl();
}
//------------------------------------------------------------------------------
// Function:    SiiMhlTxDrvSendCbusCommand
//
// Write the specified Sideband Channel command to the CBUS.
// Command can be a MSC_MSG command (RCP/RAP/RCPK/RCPE/RAPK), or another command
// such as READ_DEVCAP, SET_INT, WRITE_STAT, etc.
//
// Parameters:
//              pReq    - Pointer to a cbus_req_t structure containing the
//                        command to write
// Returns:     true    - successful write
//              false   - write failed
//------------------------------------------------------------------------------

bool SiiMhlTxDrvSendCbusCommand (cbus_req_t *pReq)
{
    bool  success = true;

    uint8_t i, startbit;

	//
	// If not connected, return with error
	//
	if( (POWER_STATE_D0_MHL != fwPowerState ) || (mscCmdInProgress))
	{
   		return false;
	}
	// Now we are getting busy
	mscCmdInProgress	= true;

    /****************************************************************************************/
    /* Setup for the command - write appropriate registers and determine the correct        */
    /*                         start bit.                                                   */
    /****************************************************************************************/

	// Set the offset and outgoing data byte right away
	SiiRegWrite(REG_CBUS_PRI_ADDR_CMD, pReq->offsetData); 	// set offset
	SiiRegWrite(REG_CBUS_PRI_WR_DATA_1ST, pReq->payload_u.msgData[0]);

    startbit = 0x00;
    switch ( pReq->command )
    {
		case MHL_SET_INT:	// Set one interrupt register = 0x60
			startbit = MSC_START_BIT_WRITE_REG;
			break;

        case MHL_WRITE_STAT:	// Write one status register = 0x60 | 0x80
            startbit = MSC_START_BIT_WRITE_REG;
            break;

        case MHL_READ_DEVCAP:	// Read one device capability register = 0x61
            startbit = MSC_START_BIT_READ_REG;
            break;

 		case MHL_GET_STATE:			// 0x62 -
		case MHL_GET_VENDOR_ID:		// 0x63 - for vendor id
		case MHL_SET_HPD:			// 0x64	- Set Hot Plug Detect in follower
		case MHL_CLR_HPD:			// 0x65	- Clear Hot Plug Detect in follower
		case MHL_GET_SC1_ERRORCODE:		// 0x69	- Get channel 1 command error code
		case MHL_GET_DDC_ERRORCODE:		// 0x6A	- Get DDC channel command error code.
		case MHL_GET_MSC_ERRORCODE:		// 0x6B	- Get MSC command error code.
		case MHL_GET_SC3_ERRORCODE:		// 0x6D	- Get channel 3 command error code.
			SiiRegWrite(REG_CBUS_PRI_ADDR_CMD, pReq->command );
            startbit = MSC_START_BIT_MSC_CMD;
            break;

        case MHL_MSC_MSG:
			SiiRegWrite(REG_CBUS_PRI_WR_DATA_2ND, pReq->payload_u.msgData[1]);
			SiiRegWrite(REG_CBUS_PRI_ADDR_CMD, pReq->command );
            startbit = MSC_START_BIT_VS_CMD;
            break;

        case MHL_WRITE_BURST:
            SiiRegWrite(REG_MSC_WRITE_BURST_LEN, pReq->length -1 );

            // Now copy all bytes from array to local scratchpad
            if (NULL == pReq->payload_u.pdatabytes)
            {
                success = false;
            }
            else
            {
	            uint8_t *pData = pReq->payload_u.pdatabytes;
                for ( i = 0; i < pReq->length; i++ )
                {
					SiiRegWrite(REG_CBUS_SCRATCHPAD_0 + i, *pData++ );
	            }
                startbit = MSC_START_BIT_WRITE_BURST;
			}
            break;

        default:
            success = false;
            break;
    }

    /****************************************************************************************/
    /* Trigger the CBUS command transfer using the determined start bit.                    */
    /****************************************************************************************/

    if ( success )
    {
        SiiRegWrite(REG_CBUS_PRI_START, startbit );
    }

    return( success );
}

bool SiiMhlTxDrvCBusBusy (void)
{
    return mscCmdInProgress ? true : false;
}

///////////////////////////////////////////////////////////////////////////
// WriteInitialRegisterValues
//
//
///////////////////////////////////////////////////////////////////////////
static void WriteInitialRegisterValues (void)
{
	// Power Up
	SiiRegWrite(TX_PAGE_L1 | 0x003D, 0x3F);			// Power up CVCC 1.2V core
	SiiRegWrite(TX_PAGE_2 | 0x0011, 0x01);			// Enable TxPLL Clock
	SiiRegWrite(TX_PAGE_2 | 0x0012, 0x11);			// Enable Tx Clock Path & Equalizer

	SiiRegWrite(REG_MHLTX_CTL1, 0x10); // TX Source termination ON
	SiiRegWrite(REG_MHLTX_CTL6, 0xBC); // Enable 1X MHL clock output
	SiiRegWrite(REG_MHLTX_CTL2, 0x3C); // TX Differential Driver Config
	SiiRegWrite(REG_MHLTX_CTL4, /*0xC8*/0xCA);  // for g TDR model : changed from 0xC8 to 0xCA

	SiiRegWrite(REG_MHLTX_CTL7, 0x03); // 2011-10-10
	SiiRegWrite(REG_MHLTX_CTL8, 0x0A); // PLL bias current, PLL BW Control

	// Analog PLL Control
	SiiRegWrite(REG_TMDS_CCTRL, 0x08);			// Enable Rx PLL clock 2011-10-10 - select BGR circuit for voltage references
	SiiRegWrite(REG_USB_CHARGE_PUMP, 0x8C);		// 2011-10-10 USB charge pump clock
       SiiRegWrite(TX_PAGE_L0 | 0x0085, 0x02);

	SiiRegWrite(TX_PAGE_2 | 0x0000, 0x00);
	SiiRegModify(REG_DVI_CTRL3, BIT5, 0);      // 2011-10-10
	SiiRegWrite(TX_PAGE_2 | 0x0013, 0x60);

	SiiRegWrite(TX_PAGE_2 | 0x0017, 0x03);			// PLL Calrefsel
	SiiRegWrite(TX_PAGE_2 | 0x001A, 0x20);			// VCO Cal
	SiiRegWrite(TX_PAGE_2 | 0x0022, 0xE0);			// Auto EQ
	SiiRegWrite(TX_PAGE_2 | 0x0023, 0xC0);			// Auto EQ
	SiiRegWrite(TX_PAGE_2 | 0x0024, 0xA0);			// Auto EQ
	SiiRegWrite(TX_PAGE_2 | 0x0025, 0x80);			// Auto EQ
	SiiRegWrite(TX_PAGE_2 | 0x0026, 0x60);			// Auto EQ
	SiiRegWrite(TX_PAGE_2 | 0x0027, 0x40);			// Auto EQ
	SiiRegWrite(TX_PAGE_2 | 0x0028, 0x20);			// Auto EQ
	SiiRegWrite(TX_PAGE_2 | 0x0029, 0x00);			// Auto EQ

	SiiRegWrite(TX_PAGE_2 | 0x0031, 0x0A);			// Rx PLL BW ~ 4MHz
	SiiRegWrite(TX_PAGE_2 | 0x0045, 0x06);			// Rx PLL BW value from I2C

	SiiRegWrite(TX_PAGE_2 | 0x004B, 0x06);
	SiiRegWrite(TX_PAGE_2 | 0x004C, 0x60);			// Manual zone control
	SiiRegWrite(TX_PAGE_2 | 0x004C, 0xE0);			// Manual zone control
	SiiRegWrite(TX_PAGE_2 | 0x004D, 0x00);			// PLL Mode Value

	SiiRegWrite(TX_PAGE_L0 | 0x0008, 0x35);			// bring out from power down (script moved this here from above)

	SiiRegWrite(REG_DISC_CTRL2, 0xAD);
	SiiRegWrite(REG_DISC_CTRL5, 0x57);				// 1.8V CBUS VTH 5K pullup for MHL state
	SiiRegWrite(REG_DISC_CTRL6, 0x11);				// RGND & single discovery attempt (RGND blocking)
	SiiRegWrite(REG_DISC_CTRL8, 0x82);				// Ignore VBUS

    SiiRegWrite(REG_DISC_CTRL9, 0x24);				// No OTG, Discovery pulse proceed, Wake pulse not bypassed

    // leave bit 3 reg_usb_en at its default value of 1
	SiiRegWrite(REG_DISC_CTRL4, 0x8C);				// Pull-up resistance off for IDLE state and 10K for discovery state.
	SiiRegWrite(REG_DISC_CTRL1, 0x27);				// Enable CBUS discovery
	SiiRegWrite(REG_DISC_CTRL7, 0x20);				// use 1K only setting
	SiiRegWrite(REG_DISC_CTRL3, 0x86);				// MHL CBUS discovery

    // Don't force HPD to 0 during wake-up from D3
	if (fwPowerState != TX_POWER_STATE_D3)
	{
		SiiRegModify(REG_INT_CTRL, BIT5 | BIT4, BIT4);	// Force HPD to 0 when not in MHL mode.
	}

	SiiRegModify(REG_INT_CTRL, BIT6, 0);   // push pull mode

	SiiRegWrite(REG_SRST, 0x84); 					// Enable Auto soft reset on SCDT = 0

	SiiRegWrite(TX_PAGE_L0 | 0x000D, 0x1C); 		// HDMI Transcode mode enable

	CbusReset();

	InitCBusRegs();
}

///////////////////////////////////////////////////////////////////////////
// InitCBusRegs
//
///////////////////////////////////////////////////////////////////////////
static void InitCBusRegs (void)
{
	uint8_t		regval;

	SiiRegWrite(TX_PAGE_CBUS | 0x0007, 0xF2); 			// Increase DDC translation layer timer
	SiiRegWrite(TX_PAGE_CBUS | 0x0036, 0x0B); 			// Drive High Time(changed from 0x0C to 0x03), it changed again from 0x03 to 0x0B for CBUS test of MHL certification
	SiiRegWrite(REG_CBUS_LINK_CONTROL_8, 0x30); 		// Use programmed timing.
	SiiRegWrite(TX_PAGE_CBUS | 0x0040, 0x03); 			// CBUS Drive Strength

#define DEVCAP_REG(x) (TX_PAGE_CBUS | 0x0080 | DEVCAP_OFFSET_##x)
	// Setup our devcap
	SiiRegWrite(DEVCAP_REG(DEV_STATE      ) ,DEVCAP_VAL_DEV_STATE       );
	SiiRegWrite(DEVCAP_REG(MHL_VERSION    ) ,DEVCAP_VAL_MHL_VERSION     );
	SiiRegWrite(DEVCAP_REG(DEV_CAT        ) ,DEVCAP_VAL_DEV_CAT         );
	SiiRegWrite(DEVCAP_REG(ADOPTER_ID_H   ) ,DEVCAP_VAL_ADOPTER_ID_H    );
	SiiRegWrite(DEVCAP_REG(ADOPTER_ID_L   ) ,DEVCAP_VAL_ADOPTER_ID_L    );
	SiiRegWrite(DEVCAP_REG(VID_LINK_MODE  ) ,DEVCAP_VAL_VID_LINK_MODE   );
	SiiRegWrite(DEVCAP_REG(AUD_LINK_MODE  ) ,DEVCAP_VAL_AUD_LINK_MODE   );
	SiiRegWrite(DEVCAP_REG(VIDEO_TYPE     ) ,DEVCAP_VAL_VIDEO_TYPE      );
	SiiRegWrite(DEVCAP_REG(LOG_DEV_MAP    ) ,DEVCAP_VAL_LOG_DEV_MAP     );
	SiiRegWrite(DEVCAP_REG(BANDWIDTH      ) ,DEVCAP_VAL_BANDWIDTH       );
	SiiRegWrite(DEVCAP_REG(FEATURE_FLAG   ) ,DEVCAP_VAL_FEATURE_FLAG    );
	SiiRegWrite(DEVCAP_REG(DEVICE_ID_H    ) ,DEVCAP_VAL_DEVICE_ID_H     );
	SiiRegWrite(DEVCAP_REG(DEVICE_ID_L    ) ,DEVCAP_VAL_DEVICE_ID_L     );
	SiiRegWrite(DEVCAP_REG(SCRATCHPAD_SIZE) ,DEVCAP_VAL_SCRATCHPAD_SIZE );
	SiiRegWrite(DEVCAP_REG(INT_STAT_SIZE  ) ,DEVCAP_VAL_INT_STAT_SIZE   );
	SiiRegWrite(DEVCAP_REG(RESERVED       ) ,DEVCAP_VAL_RESERVED        );

	// Make bits 2,3 (initiator timeout) to 1,1 for register CBUS_LINK_CONTROL_2
	regval = SiiRegRead(REG_CBUS_LINK_CONTROL_2);
	regval = (regval | 0x0C);
	SiiRegWrite(REG_CBUS_LINK_CONTROL_2, regval);

	 // Clear legacy bit on Wolverine TX. and set timeout to 0xF
    SiiRegWrite(REG_MSC_TIMEOUT_LIMIT, 0x0F);

	// Set NMax to 1
	SiiRegWrite(REG_CBUS_LINK_CONTROL_1, 0x01);
    SiiRegModify(TX_PAGE_CBUS | 0x002E, BIT4,BIT4);  // disallow vendor specific commands

}

///////////////////////////////////////////////////////////////////////////
//
// ForceUsbIdSwitchOpen
//
///////////////////////////////////////////////////////////////////////////
static void ForceUsbIdSwitchOpen (void)
{
	DISABLE_DISCOVERY
	SiiRegModify(REG_DISC_CTRL6, BIT6, BIT6);				// Force USB ID switch to open
	SiiRegWrite(REG_DISC_CTRL3, 0x86);
	SiiRegModify(REG_INT_CTRL, BIT5 | BIT4, BIT4);		// Force HPD to 0 when not in Mobile HD mode.
}
///////////////////////////////////////////////////////////////////////////
//
// ReleaseUsbIdSwitchOpen
//
///////////////////////////////////////////////////////////////////////////
static void ReleaseUsbIdSwitchOpen (void)
{
	msleep(50); // per spec
	SiiRegModify(REG_DISC_CTRL6, BIT6, 0x00);
	ENABLE_DISCOVERY
}
/*
	SiiMhlTxDrvProcessMhlConnection
		optionally called by the MHL Tx Component after giving the OEM layer the
		first crack at handling the event.
*/
void SiiMhlTxDrvProcessRgndMhl( void )
{
	SiiRegModify(REG_DISC_CTRL9, BIT0, BIT0);
}
///////////////////////////////////////////////////////////////////////////
// ProcessRgnd
//
// H/W has detected impedance change and interrupted.
// We look for appropriate impedance range to call it MHL and enable the
// hardware MHL discovery logic. If not, disable MHL discovery to allow
// USB to work appropriately.
//
// In current chip a firmware driven slow wake up pulses are sent to the
// sink to wake that and setup ourselves for full D0 operation.
///////////////////////////////////////////////////////////////////////////
void ProcessRgnd (void)
{
	uint8_t rgndImpedance;

	//
	// Impedance detection has completed - process interrupt
	//
	rgndImpedance = SiiRegRead(REG_DISC_STAT2) & 0x03;

	printk(KERN_INFO "[MHL]Drv: RGND = 0x%02X\n", (int)rgndImpedance);

	//
	// 00, 01 or 11 means USB.
	// 10 means 1K impedance (MHL)
	//
	// If 1K, then only proceed with wake up pulses
	if (0x02 == rgndImpedance)
	{
		printk(KERN_INFO "[MHL]MHL Device\n");
		SiiMhlTxNotifyRgndMhl(); // this will call the application and then optionally call
	}
	else
	{
		printk(KERN_INFO "[MHL]Non-MHL Device\n");
		SiiRegModify(REG_DISC_CTRL9, BIT3, BIT3);	// USB Established
	}
}

////////////////////////////////////////////////////////////////////
// SwitchToD0
// This function performs s/w as well as h/w state transitions.
//
// Chip comes up in D2. Firmware must first bring it to full operation
// mode in D0.
////////////////////////////////////////////////////////////////////
void SwitchToD0 (void)
{
	//
	// WriteInitialRegisterValues switches the chip to full power mode.
	//
	printk(KERN_INFO "[MHL]Switch to D0 mode\n");
	/*remove qup error*/
	MHLSleepStatus = 0;

	WriteInitialRegisterValues();

	// Force Power State to ON

	STROBE_POWER_ON // Force Power State to ON
	SiiRegModify(TPI_DEVICE_POWER_STATE_CTRL_REG, TX_POWER_STATE_MASK, 0x00);

	fwPowerState = POWER_STATE_D0_NO_MHL;  
}

////////////////////////////////////////////////////////////////////
// SwitchToD3
//
// This function performs s/w as well as h/w state transitions.
//
////////////////////////////////////////////////////////////////////
void SwitchToD3 (void)
{

	if(POWER_STATE_D3 != fwPowerState)
	{
#ifndef	__KERNEL__ //(
		pinM2uVbusCtrlM = 1;
		pinMhlConn = 1;
		pinUsbConn = 0;
#endif	//)

		printk(KERN_INFO "[MHL]Switch to D3 mode\n");

		// Force HPD to 0 when not in MHL mode.
        SiiMhlTxDrvAcquireUpstreamHPDControlDriveLow();

		// Change TMDS termination to high impedance on disconnection
		// Bits 1:0 set to 11


		SiiRegWrite(REG_MHLTX_CTL1, 0xD0);

		//
		// GPIO controlled from SiIMon can be utilized to disallow
		// low power mode, thereby allowing SiIMon to read/write register contents.
		// Otherwise SiIMon reads all registers as 0xFF
		//
/*	LGE_CHANGE :
	remove always operate false condition
	2011-10-25, jongyeol.yang@lge.com
		if(PlatformGPIOGet(pinAllowD3))
		{
			//
			// Change state to D3 by clearing bit 0 of 3D (SW_TPI, Page 1) register
			// ReadModifyWriteIndexedRegister(INDEXED_PAGE_1, 0x3D, BIT0, 0x00);
			//
*/
			CLR_BIT(TX_PAGE_L1 | 0x003D, 0);

			fwPowerState = POWER_STATE_D3;

			/*remove qup error*/
			MHLSleepStatus = 1;
/*
		}
		else
		{
			fwPowerState = POWER_STATE_D0_NO_MHL;
		}
*/
	}
}

////////////////////////////////////////////////////////////////////
// Int4Isr
//
//
//	Look for interrupts on INTR4 (Register 0x74)
//		7 = RSVD		(reserved)
//		6 = RGND Rdy	(interested)
//		5 = VBUS Low	(ignore)
//		4 = CBUS LKOUT	(interested)
//		3 = USB EST		(interested)
//		2 = MHL EST		(interested)
//		1 = RPWR5V Change	(ignore)
//		0 = SCDT Change	(interested during D0)
////////////////////////////////////////////////////////////////////
static int Int4Isr (void)
{
	uint8_t int4Status;

	int4Status = SiiRegRead(REG_INTR4);	// read status

	// When I2C is inoperational (D3) and a previous interrupt brought us here, do nothing.
	if(0xFF != int4Status)
	{
		if(int4Status)
		{
			printk(KERN_INFO "[MHL]Drv: INT4 Status = 0x%02X\n", (int) int4Status);
		}

		if(int4Status & BIT0) // SCDT Status Change
		{
            if (g_chipRevId < 1)
            {
			    ProcessScdtStatusChange();
            }
		}

		// process MHL_EST interrupt
		if(int4Status & BIT2) // MHL_EST_INT
		{
			MhlTxDrvProcessConnection();
		}

		// process USB_EST interrupt
		else if(int4Status & BIT3)
		{
			SiiRegWrite(REG_INTR4, int4Status); /*remove qup error*/
			SiiRegWrite(REG_DISC_STAT2, 0x80);	// Exit D3 via CBUS falling edge
			SwitchToD3();
			return I2C_INACCESSIBLE;
		}

		if (int4Status & BIT5)
		{
			MhlTxDrvProcessDisconnection();

			// Call back into the MHL component to give it a chance to
			// post process the disconnection event.
			MhlTxProcessEvents();
			return I2C_INACCESSIBLE;
		}

		if((POWER_STATE_D0_MHL != fwPowerState) && (int4Status & BIT6))
		{
			// Switch to full power mode.
			SwitchToD0();

			ProcessRgnd();
		}

    	// Can't succeed at these in D3
    	if(fwPowerState != POWER_STATE_D3)
    	{
    		// CBUS Lockout interrupt?
    		if (int4Status & BIT4)
    		{
    			ForceUsbIdSwitchOpen();
    			ReleaseUsbIdSwitchOpen();
    		}
        }
	}
    SiiRegWrite(REG_INTR4, int4Status); // clear all interrupts
	return I2C_ACCESSIBLE;
}

////////////////////////////////////////////////////////////////////
// Int5Isr
//
//
//	Look for interrupts on INTR5
//		7 =
//		6 =
//		5 =
//		4 =
//		3 =
//		2 =
//		1 =
//		0 =
////////////////////////////////////////////////////////////////////
static void Int5Isr (void)
{
	uint8_t int5Status;

	int5Status = SiiRegRead(REG_INTR5);

	SiiRegWrite(REG_INTR5, int5Status);	// clear all interrupts
}

///////////////////////////////////////////////////////////////////////////
//
// MhlTxDrvProcessConnection
//
///////////////////////////////////////////////////////////////////////////
static void MhlTxDrvProcessConnection (void)
{
	if( POWER_STATE_D0_MHL == fwPowerState )
	{
		return;
	}
#ifndef	__KERNEL__ //(
	// VBUS control gpio
	pinM2uVbusCtrlM = 0;
	pinMhlConn = 0;
	pinUsbConn = 1;
#endif	// )

	//
	// Discovery over-ride: reg_disc_ovride
	//
	SiiRegWrite(REG_MHLTX_CTL1, 0x10);

	fwPowerState = POWER_STATE_D0_MHL;

	//
	// Increase DDC translation layer timer (uint8_t mode)
	// Setting DDC Byte Mode
	//
	SiiRegWrite(TX_PAGE_CBUS | 0x0007, 0xF2);


	// Keep the discovery enabled. Need RGND interrupt
	ENABLE_DISCOVERY

	// Notify upper layer of cable connection
	SiiMhlTxNotifyConnection(mhlConnected = true);
}

///////////////////////////////////////////////////////////////////////////
//
// MhlTxDrvProcessDisconnection
//
///////////////////////////////////////////////////////////////////////////
static void MhlTxDrvProcessDisconnection (void)
{
	// clear all interrupts
	SiiRegWrite(REG_INTR4, SiiRegRead(REG_INTR4));

	SiiRegWrite(REG_MHLTX_CTL1, 0xD0);


	dsHpdStatus &= ~BIT6;  //cable disconnect implies downstream HPD low
	SiiRegWrite(TX_PAGE_CBUS | 0x000D, dsHpdStatus);
	SiiMhlTxNotifyDsHpdChange(0);

	if( POWER_STATE_D0_MHL == fwPowerState )
	{
		// Notify upper layer of cable removal
		SiiMhlTxNotifyConnection(false);
	}

	// Now put chip in sleep mode
	SwitchToD3();
}

///////////////////////////////////////////////////////////////////////////
//
// CbusReset
//
///////////////////////////////////////////////////////////////////////////
void CbusReset (void)
{
	uint8_t idx;
	SET_BIT(REG_SRST, 3);
	msleep(2);
	CLR_BIT(REG_SRST, 3);

	mscCmdInProgress = false;

	// Adjust interrupt mask everytime reset is performed.
    UNMASK_INTR_1_INTERRUPTS;
	UNMASK_INTR_4_INTERRUPTS;
    if (g_chipRevId < 1)
    {
		UNMASK_INTR_5_INTERRUPTS;
	}
	else
	{
		//RG disabled due to auto FIFO reset
	    MASK_INTR_5_INTERRUPTS;
	}

	UNMASK_CBUS1_INTERRUPTS;
	UNMASK_CBUS2_INTERRUPTS;

	for(idx=0; idx < 4; idx++)
	{
		// Enable WRITE_STAT interrupt for writes to all 4 MSC Status registers.
		WriteByteCBUS((0xE0 + idx), 0xFF);

		// Enable SET_INT interrupt for writes to all 4 MSC Interrupt registers.
		WriteByteCBUS((0xF0 + idx), 0xFF);
	}
}

///////////////////////////////////////////////////////////////////////////
//
// CBusProcessErrors
//
//
///////////////////////////////////////////////////////////////////////////
static uint8_t CBusProcessErrors (uint8_t intStatus)
{
    uint8_t result          = 0;
    uint8_t mscAbortReason  = 0;
	uint8_t ddcAbortReason  = 0;

    /* At this point, we only need to look at the abort interrupts. */

    intStatus &= (BIT_MSC_ABORT | BIT_MSC_XFR_ABORT | BIT_DDC_ABORT);

    if ( intStatus )
    {
//      result = ERROR_CBUS_ABORT;		// No Retry will help

        /* If transfer abort or MSC abort, clear the abort reason register. */
		if( intStatus & BIT_DDC_ABORT )
		{
			result = ddcAbortReason = SiiRegRead(TX_PAGE_CBUS | REG_DDC_ABORT_REASON);
		}

        if ( intStatus & BIT_MSC_XFR_ABORT )
        {
            result = mscAbortReason = SiiRegRead(TX_PAGE_CBUS | REG_PRI_XFR_ABORT_REASON);

            SiiRegWrite(TX_PAGE_CBUS | REG_PRI_XFR_ABORT_REASON, 0xFF);
        }
        if ( intStatus & BIT_MSC_ABORT )
        {
            SiiRegWrite(TX_PAGE_CBUS | REG_CBUS_PRI_FWR_ABORT_REASON, 0xFF);
        }
    }
    return( result );
}

void SiiMhlTxDrvGetScratchPad (uint8_t startReg,uint8_t *pData,uint8_t length)
{
int i;

    for (i = 0; i < length;++i,++startReg)
    {
        *pData++ = SiiRegRead(TX_PAGE_CBUS | (0xC0 + startReg));
    }
}

///////////////////////////////////////////////////////////////////////////
//
// MhlCbusIsr
//
// Only when MHL connection has been established. This is where we have the
// first looks on the CBUS incoming commands or returned data bytes for the
// previous outgoing command.
//
// It simply stores the event and allows application to pick up the event
// and respond at leisure.
//
// Look for interrupts on CBUS:CBUS_INTR_STATUS [0xC8:0x08]
//		7 = RSVD			(reserved)
//		6 = MSC_RESP_ABORT	(interested)
//		5 = MSC_REQ_ABORT	(interested)
//		4 = MSC_REQ_DONE	(interested)
//		3 = MSC_MSG_RCVD	(interested)
//		2 = DDC_ABORT		(interested)
//		1 = RSVD			(reserved)
//		0 = rsvd			(reserved)
///////////////////////////////////////////////////////////////////////////
static void MhlCbusIsr (void)
{
	uint8_t		cbusInt;
	uint8_t     gotData[4];	// Max four status and int registers.
	uint8_t		i;
	//
	// Main CBUS interrupts on CBUS_INTR_STATUS
	//
	cbusInt = SiiRegRead(TX_PAGE_CBUS | 0x0008);

	// When I2C is inoperational (D3) and a previous interrupt brought us here, do nothing.
	if (cbusInt == 0xFF)
	{
		return;
	}

	if (cbusInt)
	{
		//
		// Clear all interrupts that were raised even if we did not process
		//
		SiiRegWrite(TX_PAGE_CBUS | 0x0008, cbusInt);
	}

	// MSC_MSG (RCP/RAP)
	if ((cbusInt & BIT3))
	{
    	uint8_t mscMsg[2];
		//
		// Two bytes arrive at registers 0x18 and 0x19
		//
        mscMsg[0] = SiiRegRead(TX_PAGE_CBUS | 0x0018);
        mscMsg[1] = SiiRegRead(TX_PAGE_CBUS | 0x0019);

		SiiMhlTxGotMhlMscMsg( mscMsg[0], mscMsg[1] );
	}
	if (cbusInt & (BIT_MSC_ABORT | BIT_MSC_XFR_ABORT | BIT_DDC_ABORT))
	{
		gotData[0] = CBusProcessErrors(cbusInt);
	}
	// MSC_REQ_DONE received.
	if (cbusInt & BIT4)
	{
		mscCmdInProgress = false;

        // only do this after cBusInt interrupts are cleared above
		SiiMhlTxMscCommandDone( SiiRegRead(TX_PAGE_CBUS | 0x0016) );
	}

	//
	// Now look for interrupts on register 0x1E. CBUS_MSC_INT2
	// 7:4 = Reserved
	//   3 = msc_mr_write_state = We got a WRITE_STAT
	//   2 = msc_mr_set_int. We got a SET_INT
	//   1 = reserved
	//   0 = msc_mr_write_burst. We received WRITE_BURST
	//
	cbusInt = SiiRegRead(TX_PAGE_CBUS | 0x001E);
	if(cbusInt)
	{
		//
		// Clear all interrupts that were raised even if we did not process
		//
		SiiRegWrite(TX_PAGE_CBUS | 0x001E, cbusInt);

	}


    if ( BIT0 & cbusInt)
    {
        // WRITE_BURST complete
        SiiMhlTxMscWriteBurstDone( cbusInt );
    }

	if(cbusInt & BIT2)
	{
    uint8_t intr[4];
    uint16_t address;

   		for(i = 0,address=0x00A0; i < 4; ++i,++address)
		{
			// Clear all, recording as we go
            intr[i] = SiiRegRead(TX_PAGE_CBUS | address);
			SiiRegWrite( (TX_PAGE_CBUS | address) , intr[i] );
		}
		// We are interested only in first two bytes.
		SiiMhlTxGotMhlIntr( intr[0], intr[1] );
	}

	if (cbusInt & BIT3)
	{
    uint8_t status[4];
    uint16_t address;

		for(i = 0,address=0x00B0; i < 4; ++i,++address)
		{
			// Clear all, recording as we go
            status[i] = SiiRegRead(TX_PAGE_CBUS | address);
			SiiRegWrite( (TX_PAGE_CBUS | address), 0xFF /* future status[i]*/ );
		}
		SiiMhlTxGotMhlStatus( status[0], status[1] );
	}

}

static void ProcessScdtStatusChange(void)
{
	uint8_t scdtStatus;
	uint8_t mhlFifoStatus;

	scdtStatus = SiiRegRead(REG_TMDS_CSTAT);

	if (scdtStatus & 0x02)
	{
		 mhlFifoStatus = SiiRegRead(REG_INTR5);
		 if (mhlFifoStatus & 0x0C)
		 {
			SiiRegWrite(REG_INTR5, 0x0C);

			SiiRegWrite(REG_SRST, 0x94);
			SiiRegWrite(REG_SRST, 0x84);
		 }
	}
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxDrvPowBitChange
//
// Alert the driver that the peer's POW bit has changed so that it can take
// action if necessary.
//
void SiiMhlTxDrvPowBitChange (bool enable)
{
	// MHL peer device has it's own power
	if (enable)
	{
		SiiRegModify(REG_DISC_CTRL8, 0x04, 0x04);
	}
}

/*
queue implementation
*/
#define NUM_CBUS_EVENT_QUEUE_EVENTS 5
typedef struct _CBusQueue_t
{
    uint8_t head;   // queue empty condition head == tail
    uint8_t tail;
    cbus_req_t queue[NUM_CBUS_EVENT_QUEUE_EVENTS];
}CBusQueue_t,*PCBusQueue_t;


#define QUEUE_SIZE(x) (sizeof(x.queue)/sizeof(x.queue[0]))
#define MAX_QUEUE_DEPTH(x) (QUEUE_SIZE(x) -1)
#define QUEUE_DEPTH(x) ((x.head <= x.tail)?(x.tail-x.head):(QUEUE_SIZE(x)-x.head+x.tail))
#define QUEUE_FULL(x) (QUEUE_DEPTH(x) >= MAX_QUEUE_DEPTH(x))

#define ADVANCE_QUEUE_HEAD(x) { x.head = (x.head < MAX_QUEUE_DEPTH(x))?(x.head+1):0; }
#define ADVANCE_QUEUE_TAIL(x) { x.tail = (x.tail < MAX_QUEUE_DEPTH(x))?(x.tail+1):0; }

#define RETREAT_QUEUE_HEAD(x) { x.head = (x.head > 0)?(x.head-1):MAX_QUEUE_DEPTH(x); }

// Because the Linux driver can be opened multiple times it can't
// depend on one time structure initialization done by the compiler.
//CBusQueue_t CBusQueue={0,0,{0}};
CBusQueue_t CBusQueue;

cbus_req_t *GetNextCBusTransactionImpl(void)
{
    if (0==QUEUE_DEPTH(CBusQueue))
    {
        return NULL;
    }
    else
    {
    cbus_req_t *retVal;
        retVal = &CBusQueue.queue[CBusQueue.head];
        ADVANCE_QUEUE_HEAD(CBusQueue)
        return retVal;
    }
}

#define GetNextCBusTransaction(func) GetNextCBusTransactionImpl()

bool PutNextCBusTransactionImpl(cbus_req_t *pReq)
{
    if (QUEUE_FULL(CBusQueue))
    {
        //queue is full
        return false;
    }
    // at least one slot available
    CBusQueue.queue[CBusQueue.tail] = *pReq;
    ADVANCE_QUEUE_TAIL(CBusQueue)
    return true;
}

#define PutNextCBusTransaction(req) PutNextCBusTransactionImpl(req)

bool PutPriorityCBusTransactionImpl(cbus_req_t *pReq)
{
    if (QUEUE_FULL(CBusQueue))
    {
        //queue is full
        return false;
    }
    // at least one slot available
    RETREAT_QUEUE_HEAD(CBusQueue)
    CBusQueue.queue[CBusQueue.head] = *pReq;
    return true;
}
#define PutPriorityCBusTransaction(pReq) PutPriorityCBusTransactionImpl(pReq)

#define IncrementCBusReferenceCount(func) {mhlTxConfig.cbusReferenceCount++; }
#define DecrementCBusReferenceCount(func) {mhlTxConfig.cbusReferenceCount--; }

#define SetMiscFlag(func,x) { mhlTxConfig.miscFlags |=  (x); }
#define ClrMiscFlag(func,x) { mhlTxConfig.miscFlags &= ~(x); }
//
// Store global config info here. This is shared by the driver.
//
//
//
// structure to hold operating information of MhlTx component
//
static	mhlTx_config_t	mhlTxConfig={0};
//
// Functions used internally.
//
static bool SiiMhlTxSetDCapRdy( void );
/*static bool SiiMhlTxClrDCapRdy( void );*/
static	bool 		SiiMhlTxRapkSend( void );

static	void		MhlTxResetStates( void );
static	bool		MhlTxSendMscMsg ( uint8_t command, uint8_t cmdData );
static void SiiMhlTxRefreshPeerDevCapEntries(void);
static void MhlTxDriveStates( void );

#define	MHL_DEV_LD_DISPLAY					(0x01 << 0)
#define	MHL_DEV_LD_VIDEO					(0x01 << 1)
#define	MHL_DEV_LD_AUDIO					(0x01 << 2)
#define	MHL_DEV_LD_MEDIA					(0x01 << 3)
#define	MHL_DEV_LD_TUNER					(0x01 << 4)
#define	MHL_DEV_LD_RECORD					(0x01 << 5)
#define	MHL_DEV_LD_SPEAKER					(0x01 << 6)
#define	MHL_DEV_LD_GUI						(0x01 << 7)


#define	MHL_MAX_RCP_KEY_CODE	(0x7F + 1)	// inclusive

uint8_t rcpSupportTable [MHL_MAX_RCP_KEY_CODE] = {
	(MHL_DEV_LD_GUI),		// 0x00 = Select
	(MHL_DEV_LD_GUI),		// 0x01 = Up
	(MHL_DEV_LD_GUI),		// 0x02 = Down
	(MHL_DEV_LD_GUI),		// 0x03 = Left
	(MHL_DEV_LD_GUI),		// 0x04 = Right
	0, 0, 0, 0,				// 05-08 Reserved
	(MHL_DEV_LD_GUI),		// 0x09 = Root Menu
	0, 0, 0,				// 0A-0C Reserved
	(MHL_DEV_LD_GUI),		// 0x0D = Select
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// 0E-1F Reserved
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_TUNER),	// Numeric keys 0x20-0x29
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_TUNER),
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_TUNER),
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_TUNER),
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_TUNER),
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_TUNER),
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_TUNER),
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_TUNER),
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_TUNER),
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_TUNER),
	0,						// 0x2A = Dot
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_TUNER),	// Enter key = 0x2B
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_TUNER),	// Clear key = 0x2C
	0, 0, 0,				// 2D-2F Reserved
	(MHL_DEV_LD_TUNER),		// 0x30 = Channel Up
	(MHL_DEV_LD_TUNER),		// 0x31 = Channel Dn
	(MHL_DEV_LD_TUNER),		// 0x32 = Previous Channel
	(MHL_DEV_LD_AUDIO),		// 0x33 = Sound Select
	0,						// 0x34 = Input Select
	0,						// 0x35 = Show Information
	0,						// 0x36 = Help
	0,						// 0x37 = Page Up
	0,						// 0x38 = Page Down
	0, 0, 0, 0, 0, 0, 0,	// 0x39-0x3F Reserved
	0,						// 0x40 = Undefined

	(MHL_DEV_LD_SPEAKER),	// 0x41 = Volume Up
	(MHL_DEV_LD_SPEAKER),	// 0x42 = Volume Down
	(MHL_DEV_LD_SPEAKER),	// 0x43 = Mute
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO),	// 0x44 = Play
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_RECORD),	// 0x45 = Stop
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_RECORD),	// 0x46 = Pause
	(MHL_DEV_LD_RECORD),	// 0x47 = Record
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO),	// 0x48 = Rewind
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO),	// 0x49 = Fast Forward
	(MHL_DEV_LD_MEDIA),		// 0x4A = Eject
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA),	// 0x4B = Forward
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA),	// 0x4C = Backward
	0, 0, 0,				// 4D-4F Reserved
	0,						// 0x50 = Angle
	0,						// 0x51 = Subpicture
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 52-5F Reserved
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO),	// 0x60 = Play Function
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO |MHL_DEV_LD_RECORD),	// 0x61 = Pause the Play Function
	(MHL_DEV_LD_RECORD),	// 0x62 = Record Function
	(MHL_DEV_LD_RECORD),	// 0x63 = Pause the Record Function
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_RECORD),	// 0x64 = Stop Function

	(MHL_DEV_LD_SPEAKER),	// 0x65 = Mute Function
	(MHL_DEV_LD_SPEAKER),	// 0x66 = Restore Mute Function
	0, 0, 0, 0, 0, 0, 0, 0, 0, 	                        // 0x67-0x6F Undefined or reserved
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 		// 0x70-0x7F Undefined or reserved
};

bool MhlTxCBusBusy(void)
{
    return ((QUEUE_DEPTH(CBusQueue) > 0)||SiiMhlTxDrvCBusBusy() || mhlTxConfig.cbusReferenceCount)?true:false;
}
/*
	QualifyPathEnable
		Support MHL 1.0 sink devices.

	return value
		1 - consider PATH_EN received
		0 - consider PATH_EN NOT received
 */
static uint8_t QualifyPathEnable( void )
{
uint8_t retVal = 0;  // return fail by default
    if (MHL_STATUS_PATH_ENABLED & mhlTxConfig.status_1)
    {
    	retVal = 1;
    }
    else
    {
        if (0x10 == mhlTxConfig.aucDevCapCache[DEVCAP_OFFSET_MHL_VERSION])
        {
	        if (0x44 == mhlTxConfig.aucDevCapCache[DEVCAP_OFFSET_INT_STAT_SIZE])
        	{
        		retVal = 1;
        	}
        }
    }
    return retVal;
}
///////////////////////////////////////////////////////////////////////////////
// SiiMhlTxTmdsEnable
//
// Implements conditions on enabling TMDS output stated in MHL spec section 7.6.1
//
//
static void SiiMhlTxTmdsEnable(void)
{
    if (MHL_RSEN & mhlTxConfig.mhlHpdRSENflags)
    {
        if (MHL_HPD & mhlTxConfig.mhlHpdRSENflags)
        {
            if (QualifyPathEnable())
            {
            	if (RAP_CONTENT_ON & mhlTxConfig.rapFlags)
            	{
	                SiiMhlTxDrvTmdsControl( true );
	            }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxSetInt
//
// Set MHL defined INTERRUPT bits in peer's register set.
//
// This function returns true if operation was successfully performed.
//
//  regToWrite      Remote interrupt register to write
//
//  mask            the bits to write to that register
//
//  priority        0:  add to head of CBusQueue
//                  1:  add to tail of CBusQueue
//
static bool SiiMhlTxSetInt( uint8_t regToWrite,uint8_t  mask, uint8_t priorityLevel )
{
	cbus_req_t	req;
	bool retVal;

	// find the offset and bit position
	// and feed
    req.retryCount  = 2;
	req.command     = MHL_SET_INT;
	req.offsetData  = regToWrite;
	req.payload_u.msgData[0]  = mask;
    if (0 == priorityLevel)
    {
        retVal = PutPriorityCBusTransaction(&req);
    }
    else
    {
        retVal = PutNextCBusTransaction(&req);
    }
    return retVal;
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxDoWriteBurst
//
static bool SiiMhlTxDoWriteBurst( uint8_t startReg, uint8_t *pData,uint8_t length )
{
    if (FLAGS_WRITE_BURST_PENDING & mhlTxConfig.miscFlags)
    {
		cbus_req_t	req;
        bool retVal;

        req.retryCount  = 1;
    	req.command     = MHL_WRITE_BURST;
        req.length      = length;
    	req.offsetData  = startReg;
    	req.payload_u.pdatabytes  = pData;

        retVal = PutPriorityCBusTransaction(&req);
        ClrMiscFlag(SiiMhlTxDoWriteBurst, FLAGS_WRITE_BURST_PENDING)
        return retVal;
    }
    return false;
}

/////////////////////////////////////////////////////////////////////////
// SiiMhlTxRequestWriteBurst
//
ScratchPadStatus_e SiiMhlTxRequestWriteBurst(uint8_t startReg, uint8_t length, uint8_t *pData)
{
ScratchPadStatus_e retVal = SCRATCHPAD_BUSY;

	if (!(MHL_FEATURE_SP_SUPPORT & mhlTxConfig.mscFeatureFlag))
	{
		retVal= SCRATCHPAD_NOT_SUPPORTED;
	}
    else
    {
        if (
            (FLAGS_SCRATCHPAD_BUSY & mhlTxConfig.miscFlags)
            ||
            MhlTxCBusBusy()
           )
        {

        }
        else
        {
        bool temp;
        uint8_t i,reg;
        	for (i = 0,reg=startReg; (i < length) && (reg < SCRATCHPAD_SIZE); ++i,++reg)
        	{
        		mhlTxConfig.localScratchPad[reg]=pData[i];
        	}

            temp =  SiiMhlTxSetInt(MHL_RCHANGE_INT,MHL_INT_REQ_WRT, 1);
            retVal = temp ? SCRATCHPAD_SUCCESS: SCRATCHPAD_FAIL;
        }
    }
	return retVal;
}

///////////////////////////////////////////////////////////////////////////////
// SiiMhlTxInitialize
//
// Sets the transmitter component firmware up for operation, brings up chip
// into power on state first and then back to reduced-power mode D3 to conserve
// power until an MHL cable connection has been established. If the MHL port is
// used for USB operation, the chip and firmware continue to stay in D3 mode.
// Only a small circuit in the chip observes the impedance variations to see if
// processor should be interrupted to continue MHL discovery process or not.
//
// All device events will result in call to the function SiiMhlTxDeviceIsr()
// by host's hardware or software(a master interrupt handler in host software
// can call it directly). This implies that the MhlTx component shall make use
// of AppDisableInterrupts() and AppRestoreInterrupts() for any critical section
// work to prevent concurrency issues.
//
// Parameters
//
// pollIntervalMs		This number should be higher than 0 and lower than
//						51 milliseconds for effective operation of the firmware.
//						A higher number will only imply a slower response to an
//						event on MHL side which can lead to violation of a
//						connection disconnection related timing or a slower
//						response to RCP messages.
//
//
//
//
void SiiMhlTxInitialize(uint8_t pollIntervalMs )
{
	// Initialize queue of pending CBUS requests.
	CBusQueue.head = 0;
	CBusQueue.tail = 0;

	//
	// Remember mode of operation.
	//
	mhlTxConfig.pollIntervalMs  = pollIntervalMs;

	MhlTxResetStates( );

	SiiMhlTxChipInitialize ();
}



///////////////////////////////////////////////////////////////////////////////
//
// MhlTxProcessEvents
//
// This internal function is called at the end of interrupt processing.  It's
// purpose is to process events detected during the interrupt.  Some events are
// internally handled here but most are handled by a notification to the application
// layer.
//
void MhlTxProcessEvents(void)
{
	// Make sure any events detected during the interrupt are processed.
    MhlTxDriveStates();
	if( mhlTxConfig.mhlConnectionEvent )
	{
		// Consume the message
		mhlTxConfig.mhlConnectionEvent = false;

		//
		// Let app know about the change of the connection state.
		//
		AppNotifyMhlEvent(mhlTxConfig.mhlConnected, mhlTxConfig.mscFeatureFlag);

		// If connection has been lost, reset all state flags.
		if(MHL_TX_EVENT_DISCONNECTION == mhlTxConfig.mhlConnected)
		{
			MhlTxResetStates( );
		}
        else if (MHL_TX_EVENT_CONNECTION == mhlTxConfig.mhlConnected)
        {
            SiiMhlTxSetDCapRdy();
        }
	}
	else if( mhlTxConfig.mscMsgArrived )
	{
		// Consume the message
		mhlTxConfig.mscMsgArrived = false;

		//
		// Map sub-command to an event id
		//
		switch( mhlTxConfig.mscMsgSubCommand )
		{
			case	MHL_MSC_MSG_RAP:
				// RAP is fully handled here.
				//
				// Handle RAP sub-commands here itself
				//
				if( MHL_RAP_CONTENT_ON == mhlTxConfig.mscMsgData)
				{
					mhlTxConfig.rapFlags |= RAP_CONTENT_ON;
                    SiiMhlTxTmdsEnable();
				}
				else if( MHL_RAP_CONTENT_OFF == mhlTxConfig.mscMsgData)
				{
					mhlTxConfig.rapFlags &= ~RAP_CONTENT_ON;
					SiiMhlTxDrvTmdsControl( false );
				}
				// Always RAPK to the peer
				SiiMhlTxRapkSend( );
				break;

			case	MHL_MSC_MSG_RCP:
				// If we get a RCP key that we do NOT support, send back RCPE
				// Do not notify app layer.
				if(MHL_LOGICAL_DEVICE_MAP & rcpSupportTable [mhlTxConfig.mscMsgData & 0x7F] )
				{
					AppNotifyMhlEvent(MHL_TX_EVENT_RCP_RECEIVED, mhlTxConfig.mscMsgData);
				}
				else
				{
					// Save keycode to send a RCPK after RCPE.
					mhlTxConfig.mscSaveRcpKeyCode = mhlTxConfig.mscMsgData; // key code
					SiiMhlTxRcpeSend( RCPE_INEEFECTIVE_KEY_CODE );
				}
				break;

			case	MHL_MSC_MSG_RCPK:
				AppNotifyMhlEvent(MHL_TX_EVENT_RCPK_RECEIVED, mhlTxConfig.mscMsgData);
                DecrementCBusReferenceCount(MhlTxProcessEvents)
                mhlTxConfig.mscLastCommand = 0;
                mhlTxConfig.mscMsgLastCommand = 0;
				break;

			case	MHL_MSC_MSG_RCPE:
				AppNotifyMhlEvent(MHL_TX_EVENT_RCPE_RECEIVED, mhlTxConfig.mscMsgData);
				break;

			case	MHL_MSC_MSG_RAPK:
				// Do nothing if RAPK comes, except decrement the reference counter
                DecrementCBusReferenceCount(MhlTxProcessEvents)
                mhlTxConfig.mscLastCommand = 0;
                mhlTxConfig.mscMsgLastCommand = 0;
				break;

			default:
				// Any freak value here would continue with no event to app
				break;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
//
// MhlTxDriveStates
//
// This function is called by the interrupt handler in the driver layer.
// to move the MSC engine to do the next thing before allowing the application
// to run RCP APIs.
//
static void MhlTxDriveStates( void )
{
    // process queued CBus transactions
    if (QUEUE_DEPTH(CBusQueue) > 0)
    {
        if (!SiiMhlTxDrvCBusBusy())
        {
        int reQueueRequest = 0;
        cbus_req_t *pReq = GetNextCBusTransaction(MhlTxDriveStates);
            // coordinate write burst requests and grants.
            if (pReq == NULL)
				return;

            if (MHL_SET_INT == pReq->command)
            {
                if (MHL_RCHANGE_INT == pReq->offsetData)
                {
                    if (FLAGS_SCRATCHPAD_BUSY & mhlTxConfig.miscFlags)
                    {
                        if (MHL_INT_REQ_WRT == pReq->payload_u.msgData[0])
                        {
                            reQueueRequest= 1;
                        }
                        else if (MHL_INT_GRT_WRT == pReq->payload_u.msgData[0])
                        {
                            reQueueRequest= 0;
                        }
                    }
                    else
                    {
                        if (MHL_INT_REQ_WRT == pReq->payload_u.msgData[0])
                        {
                            IncrementCBusReferenceCount(MhlTxDriveStates)
                            SetMiscFlag(MhlTxDriveStates, FLAGS_SCRATCHPAD_BUSY)
                            SetMiscFlag(MhlTxDriveStates, FLAGS_WRITE_BURST_PENDING)
                        }
                        else if (MHL_INT_GRT_WRT == pReq->payload_u.msgData[0])
                        {
                            SetMiscFlag(MhlTxDriveStates, FLAGS_SCRATCHPAD_BUSY)
                        }
                    }
                }
            }
            if (reQueueRequest)
            {
                // send this one to the back of the line for later attempts
                if (pReq->retryCount-- > 0)
                {
                    PutNextCBusTransaction(pReq);
                }
            }
            else
            {
                if (MHL_MSC_MSG == pReq->command)
                {
                    mhlTxConfig.mscMsgLastCommand = pReq->payload_u.msgData[0];
                    mhlTxConfig.mscMsgLastData    = pReq->payload_u.msgData[1];
                }
                else
                {
                    mhlTxConfig.mscLastOffset  = pReq->offsetData;
                    mhlTxConfig.mscLastData    = pReq->payload_u.msgData[0];

                }
                mhlTxConfig.mscLastCommand = pReq->command;

                IncrementCBusReferenceCount(MhlTxDriveStates)
                SiiMhlTxDrvSendCbusCommand( pReq  );
            }
        }
    }
}


static void ExamineLocalAndPeerVidLinkMode( void )
{
	// set default values
	mhlTxConfig.linkMode &= ~MHL_STATUS_CLK_MODE_MASK;
	mhlTxConfig.linkMode |= MHL_STATUS_CLK_MODE_NORMAL;

	// when more modes than PPIXEL and normal are supported,
	//   this should become a table lookup.
	if (MHL_DEV_VID_LINK_SUPP_PPIXEL & mhlTxConfig.aucDevCapCache[DEVCAP_OFFSET_VID_LINK_MODE])
	{
		if (MHL_DEV_VID_LINK_SUPP_PPIXEL & DEVCAP_VAL_VID_LINK_MODE)
		{
			mhlTxConfig.linkMode &= ~MHL_STATUS_CLK_MODE_MASK;
	    	mhlTxConfig.linkMode |= mhlTxConfig.preferredClkMode;
	    }
	}
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxMscCommandDone
//
// This function is called by the driver to inform of completion of last command.
//
// It is called in interrupt context to meet some MHL specified timings, therefore,
// it should not have to call app layer and do negligible processing, no printfs.
//
#define FLAG_OR_NOT(x) (FLAGS_HAVE_##x & mhlTxConfig.miscFlags)?#x:""
#define SENT_OR_NOT(x) (FLAGS_SENT_##x & mhlTxConfig.miscFlags)?#x:""

void	SiiMhlTxMscCommandDone( uint8_t data1 )
{
    DecrementCBusReferenceCount(SiiMhlTxMscCommandDone)
    if ( MHL_READ_DEVCAP == mhlTxConfig.mscLastCommand )
    {
		if (mhlTxConfig.mscLastOffset < sizeof(mhlTxConfig.aucDevCapCache) )
		{
			// populate the cache
		    mhlTxConfig.aucDevCapCache[mhlTxConfig.mscLastOffset] = data1;

        	if(MHL_DEV_CATEGORY_OFFSET == mhlTxConfig.mscLastOffset)
    		{
    			uint8_t param;
                mhlTxConfig.miscFlags |= FLAGS_HAVE_DEV_CATEGORY;
                param  = data1 & MHL_DEV_CATEGORY_POW_BIT;

				// Give the OEM a chance at handling power for himself
				if (MHL_TX_EVENT_STATUS_PASSTHROUGH == AppNotifyMhlEvent(MHL_TX_EVENT_POW_BIT_CHG, param))
				{
	    			SiiMhlTxDrvPowBitChange((bool)param );
	    		}

    	    }
        	else if(MHL_DEV_FEATURE_FLAG_OFFSET == mhlTxConfig.mscLastOffset)
    		{
                mhlTxConfig.miscFlags |= FLAGS_HAVE_DEV_FEATURE_FLAGS;

    			// Remember features of the peer
    			mhlTxConfig.mscFeatureFlag	= data1;
        	}
        	else if (DEVCAP_OFFSET_VID_LINK_MODE == mhlTxConfig.mscLastOffset)
        	{
        		ExamineLocalAndPeerVidLinkMode();
        	}


			if ( ++mhlTxConfig.ucDevCapCacheIndex < sizeof(mhlTxConfig.aucDevCapCache) )
			{
            	// OK to call this here, since requests always get queued and processed in the "foreground"
				SiiMhlTxReadDevcap( mhlTxConfig.ucDevCapCacheIndex );
			}
			else
			{
				// this is necessary for both firmware and linux driver.
				AppNotifyMhlEvent(MHL_TX_EVENT_DCAP_CHG, 0);

    			// These variables are used to remember if we issued a READ_DEVCAP
    	   		//    or other MSC command
    			// Since we are done, reset them.
    			mhlTxConfig.mscLastCommand = 0;
    			mhlTxConfig.mscLastOffset  = 0;
			}
		}
	}
	else if(MHL_WRITE_STAT == mhlTxConfig.mscLastCommand)
	{
        if (MHL_STATUS_REG_CONNECTED_RDY == mhlTxConfig.mscLastOffset)
        {
            if (MHL_STATUS_DCAP_RDY & mhlTxConfig.mscLastData)
            {
                mhlTxConfig.miscFlags |= FLAGS_SENT_DCAP_RDY;
                SiiMhlTxSetInt(MHL_RCHANGE_INT,MHL_INT_DCAP_CHG, 0); // priority request
            }
		}
        else if (MHL_STATUS_REG_LINK_MODE == mhlTxConfig.mscLastOffset)
        {
            if ( MHL_STATUS_PATH_ENABLED & mhlTxConfig.mscLastData)
	        {
                mhlTxConfig.miscFlags |= FLAGS_SENT_PATH_EN;
   	    	}
		}

        mhlTxConfig.mscLastCommand = 0;
        mhlTxConfig.mscLastOffset  = 0;
	}
	else if (MHL_MSC_MSG == mhlTxConfig.mscLastCommand)
    {
    	if(MHL_MSC_MSG_RCPE == mhlTxConfig.mscMsgLastCommand)
		{
			//
			// RCPE is always followed by an RCPK with original key code that came.
			//
			if( SiiMhlTxRcpkSend( mhlTxConfig.mscSaveRcpKeyCode ) )
			{
    		}
    	}
        else
        {
        }
        mhlTxConfig.mscLastCommand = 0;
    }
    else if (MHL_WRITE_BURST == mhlTxConfig.mscLastCommand)
    {
        mhlTxConfig.mscLastCommand = 0;
        mhlTxConfig.mscLastOffset  = 0;
        mhlTxConfig.mscLastData    = 0;

        // all CBus request are queued, so this is OK to call here
        // use priority 0 so that other queued commands don't interfere
        SiiMhlTxSetInt( MHL_RCHANGE_INT,MHL_INT_DSCR_CHG,0 );
    }
    else if (MHL_SET_INT == mhlTxConfig.mscLastCommand)
    {
        if (MHL_RCHANGE_INT == mhlTxConfig.mscLastOffset)
        {
            if (MHL_INT_DSCR_CHG == mhlTxConfig.mscLastData)
            {
                DecrementCBusReferenceCount(SiiMhlTxMscCommandDone)  // this one is for the write burst request
                ClrMiscFlag(SiiMhlTxMscCommandDone, FLAGS_SCRATCHPAD_BUSY)
            }
        }
			// Once the command has been sent out successfully, forget this case.
        mhlTxConfig.mscLastCommand = 0;
        mhlTxConfig.mscLastOffset  = 0;
        mhlTxConfig.mscLastData    = 0;
    }
    else
    {
    }
    if (!(FLAGS_RCP_READY & mhlTxConfig.miscFlags))
    {
        if (FLAGS_HAVE_DEV_CATEGORY & mhlTxConfig.miscFlags)
        {
            if (FLAGS_HAVE_DEV_FEATURE_FLAGS& mhlTxConfig.miscFlags)
            {
                if (FLAGS_SENT_PATH_EN & mhlTxConfig.miscFlags)
                {
                    if (FLAGS_SENT_DCAP_RDY & mhlTxConfig.miscFlags)
                    {
						if (mhlTxConfig.ucDevCapCacheIndex >= sizeof(mhlTxConfig.aucDevCapCache))
						{
                            mhlTxConfig.miscFlags |= FLAGS_RCP_READY;
                    		// Now we can entertain App commands for RCP
                    		// Let app know this state
                    		mhlTxConfig.mhlConnectionEvent = true;
                    		mhlTxConfig.mhlConnected = MHL_TX_EVENT_RCP_READY;
						}
                    }
                }
            }
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxMscWriteBurstDone
//
// This function is called by the driver to inform of completion of a write burst.
//
// It is called in interrupt context to meet some MHL specified timings, therefore,
// it should not have to call app layer and do negligible processing, no printfs.
//
void	SiiMhlTxMscWriteBurstDone( uint8_t data1 )
{
#ifdef CONFIG_LG_MAGIC_MOTION_REMOCON
	SiiMhlTxReadScratchpad();
#else
#define WRITE_BURST_TEST_SIZE 16
uint8_t temp[WRITE_BURST_TEST_SIZE];
uint8_t i;
    data1=data1;  // make this compile for NON debug builds
    SiiMhlTxDrvGetScratchPad(0,temp,WRITE_BURST_TEST_SIZE);
    for (i = 0; i < WRITE_BURST_TEST_SIZE ; ++i)
    {
        if (temp[i]>=' ')
        {
        }
        else
        {
        }
    }
#endif
}


///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxGotMhlMscMsg
//
// This function is called by the driver to inform of arrival of a MHL MSC_MSG
// such as RCP, RCPK, RCPE. To quickly return back to interrupt, this function
// remembers the event (to be picked up by app later in task context).
//
// It is called in interrupt context to meet some MHL specified timings, therefore,
// it should not have to call app layer and do negligible processing of its own,
//
// No printfs.
//
// Application shall not call this function.
//
void	SiiMhlTxGotMhlMscMsg( uint8_t subCommand, uint8_t cmdData )
{
	// Remember the event.
	mhlTxConfig.mscMsgArrived		= true;
	mhlTxConfig.mscMsgSubCommand	= subCommand;
	mhlTxConfig.mscMsgData			= cmdData;
}
///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxGotMhlIntr
//
// This function is called by the driver to inform of arrival of a MHL INTERRUPT.
//
// It is called in interrupt context to meet some MHL specified timings, therefore,
// it should not have to call app layer and do negligible processing, no printfs.
//
void	SiiMhlTxGotMhlIntr( uint8_t intr_0, uint8_t intr_1 )
{
	//
	// Handle DCAP_CHG INTR here
	//
	if(MHL_INT_DCAP_CHG & intr_0)
	{
        if (MHL_STATUS_DCAP_RDY & mhlTxConfig.status_0)
        {
			SiiMhlTxRefreshPeerDevCapEntries();
		}
	}

	if( MHL_INT_DSCR_CHG & intr_0)
    {
        SiiMhlTxDrvGetScratchPad(0,mhlTxConfig.localScratchPad,sizeof(mhlTxConfig.localScratchPad));
        // remote WRITE_BURST is complete
        ClrMiscFlag(SiiMhlTxGotMhlIntr, FLAGS_SCRATCHPAD_BUSY)
        AppNotifyMhlEvent(MHL_TX_EVENT_DSCR_CHG,0);
    }
	if( MHL_INT_REQ_WRT  & intr_0)
    {

        // this is a request from the sink device.
        if (FLAGS_SCRATCHPAD_BUSY & mhlTxConfig.miscFlags)
        {
            // use priority 1 to defer sending grant until
            //  local traffic is done
            SiiMhlTxSetInt( MHL_RCHANGE_INT, MHL_INT_GRT_WRT,1);
        }
        else
        {
            SetMiscFlag(SiiMhlTxGotMhlIntr, FLAGS_SCRATCHPAD_BUSY)
            // OK to call this here, since all requests are queued
            // use priority 0 to respond immediately
            SiiMhlTxSetInt( MHL_RCHANGE_INT, MHL_INT_GRT_WRT,0);
        }
    }
	if( MHL_INT_GRT_WRT  & intr_0)
    {
    	uint8_t length =sizeof(mhlTxConfig.localScratchPad);
        SiiMhlTxDoWriteBurst(0x40, mhlTxConfig.localScratchPad, length);
    }

    // removed "else", since interrupts are not mutually exclusive of each other.
	if(MHL_INT_EDID_CHG & intr_1)
	{
		// force upstream source to read the EDID again.
		// Most likely by appropriate togggling of HDMI HPD
		SiiMhlTxDrvNotifyEdidChange ( );
	}
}
///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxGotMhlStatus
//
// This function is called by the driver to inform of arrival of a MHL STATUS.
//
// It is called in interrupt context to meet some MHL specified timings, therefore,
// it should not have to call app layer and do negligible processing, no printfs.
//
void	SiiMhlTxGotMhlStatus( uint8_t status_0, uint8_t status_1 )
{
	//
	// Handle DCAP_RDY STATUS here itself
	//
	uint8_t StatusChangeBitMask0,StatusChangeBitMask1;
    StatusChangeBitMask0 = status_0 ^ mhlTxConfig.status_0;
    StatusChangeBitMask1 = status_1 ^ mhlTxConfig.status_1;
	// Remember the event.   (other code checks the saved values, so save the values early, but not before the XOR operations above)
	mhlTxConfig.status_0 = status_0;
	mhlTxConfig.status_1 = status_1;

	if(MHL_STATUS_DCAP_RDY & StatusChangeBitMask0)
	{
        if (MHL_STATUS_DCAP_RDY & status_0)
        {
			SiiMhlTxRefreshPeerDevCapEntries();
        }
	}


    // did PATH_EN change?
	if(MHL_STATUS_PATH_ENABLED & StatusChangeBitMask1)
    {
        if(MHL_STATUS_PATH_ENABLED & status_1)
        {
            // OK to call this here since all requests are queued
            SiiMhlTxSetPathEn();
        }
        else
        {
            // OK to call this here since all requests are queued
            SiiMhlTxClrPathEn();
        }
    }

}
///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxRcpSend
//
// This function checks if the peer device supports RCP and sends rcpKeyCode. The
// function will return a value of true if it could successfully send the RCP
// subcommand and the key code. Otherwise false.
//
// The followings are not yet utilized.
//
// (MHL_FEATURE_RAP_SUPPORT & mhlTxConfig.mscFeatureFlag))
// (MHL_FEATURE_SP_SUPPORT & mhlTxConfig.mscFeatureFlag))
//
//
bool SiiMhlTxRcpSend( uint8_t rcpKeyCode )
{
	bool retVal;
	//
	// If peer does not support do not send RCP or RCPK/RCPE commands
	//

	if((0 == (MHL_FEATURE_RCP_SUPPORT & mhlTxConfig.mscFeatureFlag))
	    ||
        !(FLAGS_RCP_READY & mhlTxConfig.miscFlags)
		)
	{
		retVal=false;
	}

	retVal=MhlTxSendMscMsg ( MHL_MSC_MSG_RCP, rcpKeyCode );
    if(retVal)
    {
        IncrementCBusReferenceCount(SiiMhlTxRcpSend)
		MhlTxDriveStates();
	}
    return retVal;
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxRcpkSend
//
// This function sends RCPK to the peer device.
//
bool SiiMhlTxRcpkSend( uint8_t rcpKeyCode )
{
	bool	retVal;

	retVal = MhlTxSendMscMsg(MHL_MSC_MSG_RCPK, rcpKeyCode);
	if(retVal)
	{
		MhlTxDriveStates();
	}
	return retVal;
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxRapkSend
//
// This function sends RAPK to the peer device.
//
static	bool SiiMhlTxRapkSend( void )
{
	return	( MhlTxSendMscMsg ( MHL_MSC_MSG_RAPK, 0 ) );
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxRcpeSend
//
// The function will return a value of true if it could successfully send the RCPE
// subcommand. Otherwise false.
//
// When successful, MhlTx internally sends RCPK with original (last known)
// keycode.
//
bool SiiMhlTxRcpeSend( uint8_t rcpeErrorCode )
{
	bool	retVal;

	retVal = MhlTxSendMscMsg(MHL_MSC_MSG_RCPE, rcpeErrorCode);
	if(retVal)
	{
		MhlTxDriveStates();
	}
	return retVal;
}

static bool SiiMhlTxSetStatus( uint8_t regToWrite, uint8_t value )
{
	cbus_req_t	req;
    bool retVal;

	// find the offset and bit position
	// and feed
    req.retryCount  = 2;
	req.command     = MHL_WRITE_STAT;
	req.offsetData  = regToWrite;
	req.payload_u.msgData[0]  = value;

    retVal = PutNextCBusTransaction(&req);
    return retVal;
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxSetDCapRdy
//
static bool SiiMhlTxSetDCapRdy( void )
{
    mhlTxConfig.connectedReady |= MHL_STATUS_DCAP_RDY;   // update local copy
    return SiiMhlTxSetStatus( MHL_STATUS_REG_CONNECTED_RDY, mhlTxConfig.connectedReady);
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxClrDCapRdy
//
/*static bool SiiMhlTxClrDCapRdy( void )
{
    mhlTxConfig.connectedReady &= ~MHL_STATUS_DCAP_RDY;  // update local copy
    return SiiMhlTxSetStatus( MHL_STATUS_REG_CONNECTED_RDY, mhlTxConfig.connectedReady);
}
*/
///////////////////////////////////////////////////////////////////////////////
//
//  SiiMhlTxSendLinkMode
//
static bool SiiMhlTxSendLinkMode(void)
{
    return SiiMhlTxSetStatus( MHL_STATUS_REG_LINK_MODE, mhlTxConfig.linkMode);
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxSetPathEn
//
bool SiiMhlTxSetPathEn(void )
{
    SiiMhlTxTmdsEnable();
    mhlTxConfig.linkMode |= MHL_STATUS_PATH_ENABLED;     // update local copy
    return SiiMhlTxSetStatus( MHL_STATUS_REG_LINK_MODE, mhlTxConfig.linkMode);
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxClrPathEn
//
bool SiiMhlTxClrPathEn( void )
{
    SiiMhlTxDrvTmdsControl( false );
    mhlTxConfig.linkMode &= ~MHL_STATUS_PATH_ENABLED;    // update local copy
    return SiiMhlTxSetStatus( MHL_STATUS_REG_LINK_MODE, mhlTxConfig.linkMode);
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxReadDevcap
//
// This function sends a READ DEVCAP MHL command to the peer.
// It  returns true if successful in doing so.
//
// The value of devcap should be obtained by making a call to SiiMhlTxGetEvents()
//
// offset		Which byte in devcap register is required to be read. 0..0x0E
//
bool SiiMhlTxReadDevcap( uint8_t offset )
{
	cbus_req_t	req;
	//
	// Send MHL_READ_DEVCAP command
	//
    req.retryCount  = 2;
	req.command     = MHL_READ_DEVCAP;
	req.offsetData  = offset;
    req.payload_u.msgData[0]  = 0;  // do this to avoid confusion

    return PutNextCBusTransaction(&req);
}

/*
 * SiiMhlTxRefreshPeerDevCapEntries
 */

static void SiiMhlTxRefreshPeerDevCapEntries(void)
{
	// only issue if no existing refresh is in progress
	if (mhlTxConfig.ucDevCapCacheIndex >= sizeof(mhlTxConfig.aucDevCapCache))
	{
		mhlTxConfig.ucDevCapCacheIndex=0;
		SiiMhlTxReadDevcap( mhlTxConfig.ucDevCapCacheIndex );
	}
}

///////////////////////////////////////////////////////////////////////////////
//
// MhlTxSendMscMsg
//
// This function sends a MSC_MSG command to the peer.
// It  returns true if successful in doing so.
//
// The value of devcap should be obtained by making a call to SiiMhlTxGetEvents()
//
// offset		Which byte in devcap register is required to be read. 0..0x0E
//
static bool MhlTxSendMscMsg ( uint8_t command, uint8_t cmdData )
{
	cbus_req_t	req;
	uint8_t		ccode;

	//
	// Send MSC_MSG command
	//
	// Remember last MSC_MSG command (RCPE particularly)
	//
    req.retryCount  = 2;
	req.command     = MHL_MSC_MSG;
	req.payload_u.msgData[0]  = command;
	req.payload_u.msgData[1]  = cmdData;
    ccode = PutNextCBusTransaction(&req);
	return( (bool) ccode );
}
///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxNotifyConnection
//
//
void	SiiMhlTxNotifyConnection( bool mhlConnected )
{
	mhlTxConfig.mhlConnectionEvent = true;

	if(mhlConnected)
	{
        mhlTxConfig.rapFlags |= RAP_CONTENT_ON;
		mhlTxConfig.mhlConnected = MHL_TX_EVENT_CONNECTION;
        mhlTxConfig.mhlHpdRSENflags |= MHL_RSEN;
        SiiMhlTxTmdsEnable();
        SiiMhlTxSendLinkMode();
		MhlTxProcessEvents();
	}
	else
	{
        mhlTxConfig.rapFlags &= ~RAP_CONTENT_ON;
		mhlTxConfig.mhlConnected = MHL_TX_EVENT_DISCONNECTION;
        mhlTxConfig.mhlHpdRSENflags &= ~MHL_RSEN;
	}
}
///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxNotifyDsHpdChange
// Driver tells about arrival of SET_HPD or CLEAR_HPD by calling this function.
//
// Turn the content off or on based on what we got.
//
void	SiiMhlTxNotifyDsHpdChange( uint8_t dsHpdStatus )
{
	if( 0 == dsHpdStatus )
	{
		printk(KERN_INFO "%s: ~MHL_HPD\n", __func__);
        mhlTxConfig.mhlHpdRSENflags &= ~MHL_HPD;
		AppNotifyMhlDownStreamHPDStatusChange(dsHpdStatus);
		SiiMhlTxDrvTmdsControl( false );
	}
	else
	{
	    printk(KERN_INFO "%s: MHL_HPD\n", __func__);
        mhlTxConfig.mhlHpdRSENflags |= MHL_HPD;
		AppNotifyMhlDownStreamHPDStatusChange(dsHpdStatus);
        SiiMhlTxTmdsEnable();
	}
}
///////////////////////////////////////////////////////////////////////////////
//
// MhlTxResetStates
//
// Application picks up mhl connection and rcp events at periodic intervals.
// Interrupt handler feeds these variables. Reset them on disconnection.
//
static void	MhlTxResetStates( void )
{
	mhlTxConfig.mhlConnectionEvent	= false;
	mhlTxConfig.mhlConnected		= MHL_TX_EVENT_DISCONNECTION;
    mhlTxConfig.mhlHpdRSENflags     &= ~(MHL_RSEN | MHL_HPD);
    mhlTxConfig.rapFlags 			&= ~RAP_CONTENT_ON;
	mhlTxConfig.mscMsgArrived		= false;

    mhlTxConfig.status_0            = 0;
    mhlTxConfig.status_1            = 0;
    mhlTxConfig.connectedReady      = 0;
    mhlTxConfig.linkMode            = MHL_STATUS_CLK_MODE_NORMAL; // indicate normal (24-bit) mode
    mhlTxConfig.preferredClkMode	= MHL_STATUS_CLK_MODE_NORMAL;  // this can be overridden by the application calling SiiMhlTxSetPreferredPixelFormat()
    mhlTxConfig.cbusReferenceCount  = 0;
    mhlTxConfig.miscFlags           = 0;
    mhlTxConfig.mscLastCommand      = 0;
    mhlTxConfig.mscMsgLastCommand   = 0;
    mhlTxConfig.ucDevCapCacheIndex  = 1 + sizeof(mhlTxConfig.aucDevCapCache);
}

/*
    SiiTxReadConnectionStatus
    returns:
    0: if not fully connected
    1: if fully connected
*/
uint8_t    SiiTxReadConnectionStatus(void)
{
    return (mhlTxConfig.mhlConnected >= MHL_TX_EVENT_RCP_READY)?1:0;
}

/*
  SiiMhlTxSetPreferredPixelFormat

	clkMode - the preferred pixel format for the CLK_MODE status register

	Returns: 0 -- success
		     1 -- failure - bits were specified that are not within the mask
 */
uint8_t SiiMhlTxSetPreferredPixelFormat(uint8_t clkMode)
{
	if (~MHL_STATUS_CLK_MODE_MASK & clkMode)
	{
		return 1;
	}
	else
	{
     	mhlTxConfig.preferredClkMode = clkMode;

		// check to see if a refresh has happened since the last call
		//   to MhlTxResetStates()
     	if (mhlTxConfig.ucDevCapCacheIndex <= sizeof(mhlTxConfig.aucDevCapCache))
     	{
           	// check to see if DevCap cache update has already updated this yet.
      		if (mhlTxConfig.ucDevCapCacheIndex > DEVCAP_OFFSET_VID_LINK_MODE)
      		{
              	ExamineLocalAndPeerVidLinkMode();
      		}
     	}
		return 0;
	}
}

/*
	SiiTxGetPeerDevCapEntry
	index -- the devcap index to get
	*pData pointer to location to write data
	returns
		0 -- success
		1 -- busy.
 */
uint8_t SiiTxGetPeerDevCapEntry(uint8_t index,uint8_t *pData)
{
	if (mhlTxConfig.ucDevCapCacheIndex < sizeof(mhlTxConfig.aucDevCapCache))
	{
		// update is in progress
		return 1;
	}
	else
	{
		*pData = mhlTxConfig.aucDevCapCache[index];
		return 0;
	}
}

/*
	SiiGetScratchPadVector
	offset -- The beginning offset into the scratch pad from which to fetch entries.
	length -- The number of entries to fetch
	*pData -- A pointer to an array of bytes where the data should be placed.

	returns:
		ScratchPadStatus_e see si_mhl_tx_api.h for details

*/
ScratchPadStatus_e SiiGetScratchPadVector(uint8_t offset,uint8_t length, uint8_t *pData)
{
	if (!(MHL_FEATURE_SP_SUPPORT & mhlTxConfig.mscFeatureFlag))
	{
		return SCRATCHPAD_NOT_SUPPORTED;
	}
	else if (FLAGS_SCRATCHPAD_BUSY & mhlTxConfig.miscFlags)
	{
	    return SCRATCHPAD_BUSY;
	}
	else if ((offset >= sizeof(mhlTxConfig.localScratchPad)) ||
			 (length > (sizeof(mhlTxConfig.localScratchPad)- offset)) )
	{
		return SCRATCHPAD_BAD_PARAM;
	}
	else
	{
    uint8_t i,reg;
    	for (i = 0,reg=offset; (i < length) && (reg < sizeof(mhlTxConfig.localScratchPad));++i,++reg)
    	{
    	    pData[i] = mhlTxConfig.localScratchPad[reg];
    	}
		return SCRATCHPAD_SUCCESS;
	}
}

void SiiMhlTxNotifyRgndMhl( void )
{
	if (MHL_TX_EVENT_STATUS_PASSTHROUGH == AppNotifyMhlEvent(MHL_TX_EVENT_RGND_MHL, 0))
	{
		// Application layer did not claim this, so send it to the driver layer
		   SiiMhlTxDrvProcessRgndMhl(); 	
	}
}

/***** local variable declarations *******************************************/
static int32_t devMajor = 0;    /**< default device major number */
static struct cdev siiMhlCdev;
static struct class *siiMhlClass;

/***** global variable declarations *******************************************/

MHL_DRIVER_CONTEXT_T gDriverContext;

#if defined(DEBUG)
unsigned char DebugChannelMasks[SII_OSAL_DEBUG_NUM_CHANNELS/8+1]={0xFF,0xFF,0xFF,0xFF};
//ulong DebugChannelMask = 0xFFFFFFFF;
module_param_array(DebugChannelMasks, byte, NULL, S_IRUGO | S_IWUSR);

ushort DebugFormat = SII_OS_DEBUG_FORMAT_FILEINFO;
module_param(DebugFormat, ushort, S_IRUGO | S_IWUSR);
#endif


/***** local functions *******************************************************/

/**
 *  @brief Start the MHL transmitter device
 *
 *  This function is called during driver startup to initialize control of the
 *  MHL transmitter device by the driver.
 *
 *  @return     0 if successful, negative error code otherwise
 *
 *****************************************************************************/
int32_t StartMhlTxDevice(void)
{
    printk(KERN_INFO "[MHL]Starting %s\n", MHL_PART_NAME);

    /* Initialize the MHL Tx code a polling interval of 30ms. */
	if (down_interruptible(&sii8334_irq_sem) == 0) {
		SiiMhlTxInitialize(EVENT_POLL_INTERVAL_30_MS);
    	up(&sii8334_irq_sem);
	}

    return 0;
}

/***** public functions ******************************************************/

/**
 * @brief Handle read request to the connection_state attribute file.
 */
ssize_t ShowConnectionState(struct device *dev, struct device_attribute *attr,
							char *buf)
{
	if(gDriverContext.flags & MHL_STATE_FLAG_CONNECTED) {
		return scnprintf(buf, PAGE_SIZE, "connected %s_ready",
				gDriverContext.flags & MHL_STATE_FLAG_RCP_READY? "rcp" : "not_rcp");
	} else {
		return scnprintf(buf, PAGE_SIZE, "not connected");
	}
}


/**
 * @brief Handle read request to the rcp_keycode attribute file.
 */
ssize_t ShowRcp(struct device *dev, struct device_attribute *attr,
							char *buf)
{
	int		status = 0;

	if(down_interruptible(&sii8334_irq_sem) != 0)
		return -ERESTARTSYS;

	if(gDriverContext.flags &
		(MHL_STATE_FLAG_RCP_SENT | MHL_STATE_FLAG_RCP_RECEIVED))
	{
		status = scnprintf(buf, PAGE_SIZE, "0x%02x %s",
				gDriverContext.keyCode,
				gDriverContext.flags & MHL_STATE_FLAG_RCP_SENT? "sent" : "received");
	}

	up(&sii8334_irq_sem);
	return status;
}



/**
 * @brief Handle write request to the rcp_keycode attribute file.
 */
ssize_t SendRcp(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	unsigned long	keyCode;
	int				status = -EINVAL;

	if(down_interruptible(&sii8334_irq_sem) != 0)
		return -ERESTARTSYS;

	while(gDriverContext.flags & MHL_STATE_FLAG_RCP_READY) {

		if(strict_strtoul(buf, 0, &keyCode)) {
			break;
		}

		if(keyCode >= 0xFE) {
			break;
		}

		gDriverContext.flags &= ~(MHL_STATE_FLAG_RCP_RECEIVED |
								  MHL_STATE_FLAG_RCP_ACK |
								  MHL_STATE_FLAG_RCP_NAK);
		gDriverContext.flags |= MHL_STATE_FLAG_RCP_SENT;
		gDriverContext.keyCode = (uint8_t)keyCode;
		SiiMhlTxRcpSend((uint8_t)keyCode);
		status = count;
		break;
	}

	up(&sii8334_irq_sem);

	return status;
}


/**
 * @brief Handle write request to the rcp_ack attribute file.
 *
 * This file is used to send either an ACK or NAK for a received
 * Remote Control Protocol (RCP) key code.
 *
 * The format of the string in buf must be:
 * 	"keycode=<keyvalue> errorcode=<errorvalue>
 * 	where:	<keyvalue>		is replaced with value of the RCP to be ACK'd or NAK'd
 * 			<errorvalue>	0 if the RCP key code is to be ACK'd
 * 							non-zero error code if the RCP key code is to be NAK'd
 */
ssize_t SendRcpAck(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	unsigned long	keyCode = 0x100;	// initialize with invalid values
	unsigned long	errCode = 0x100;
	char			*pStr;
	int				status = -EINVAL;

	// Parse the input string and extract the RCP key code and error code
	do {
		pStr = strstr(buf, "keycode=");
		if(pStr != NULL) {
			if(strict_strtoul(pStr + 8, 0, &keyCode)) {
				break;
			}
		} else {
			break;
		}

		pStr = strstr(buf, "errorcode=");
		if(pStr != NULL) {
			if(strict_strtoul(pStr + 10, 0, &errCode)) {
				break;
			}
		} else
			break;

	} while(false);

	if((keyCode > 0xFF) || (errCode > 0xFF))
		return status;

	if(down_interruptible(&sii8334_irq_sem) != 0)
		return -ERESTARTSYS;

	while(gDriverContext.flags & MHL_STATE_FLAG_RCP_READY) {

		if((keyCode != gDriverContext.keyCode)
			|| !(gDriverContext.flags & MHL_STATE_FLAG_RCP_RECEIVED))
			break;

		if(errCode == 0) {
			SiiMhlTxRcpkSend((uint8_t)keyCode);
		} else {
			SiiMhlTxRcpeSend((uint8_t)errCode);
		}

		status = count;
		break;
	}

	up(&sii8334_irq_sem);

	return status;
}



/**
 * @brief Handle read request to the rcp_ack attribute file.
 *
 * Reads from this file return a string detailing the last RCP
 * ACK or NAK received by the driver.
 *
 * The format of the string returned in buf is:
 * 	"keycode=<keyvalue> errorcode=<errorvalue>
 * 	where:	<keyvalue>		is replaced with value of the RCP key code for which
 * 							an ACK or NAK has been received.
 * 			<errorvalue>	0 if the last RCP key code was ACK'd or
 * 							non-zero error code if the RCP key code was NAK'd
 */
ssize_t ShowRcpAck(struct device *dev, struct device_attribute *attr,
					char *buf)
{
	int				status = -EINVAL;

	if(down_interruptible(&sii8334_irq_sem) != 0)
		return -ERESTARTSYS;

	if(gDriverContext.flags & (MHL_STATE_FLAG_RCP_ACK | MHL_STATE_FLAG_RCP_NAK)) {

		status = scnprintf(buf, PAGE_SIZE, "keycode=0x%02x errorcode=0x%02x",
				gDriverContext.keyCode, gDriverContext.errCode);
	}

	up(&sii8334_irq_sem);

	return status;
}



/**
 * @brief Handle write request to the devcap attribute file.
 *
 * Writes to the devcap file are done to set the offset of a particular
 * Device Capabilities register to be returned by a subsequent read
 * from this file.
 *
 * All we need to do is validate the specified offset and if valid
 * save it for later use.
 */
ssize_t SelectDevCap(struct device *dev, struct device_attribute *attr,
					 const char *buf, size_t count)
{
	unsigned long	devCapOffset;
	int				status = -EINVAL;

	do {

		if (strict_strtoul(buf, 0, &devCapOffset))
			break;

		if (devCapOffset >= 0x0F)
			break;

		gDriverContext.devCapOffset = (uint8_t)devCapOffset;

		status = count;

	} while(false);

	return status;
}



/**
 * @brief Handle read request to the devcap attribute file.
 *
 * Reads from this file return the hexadecimal string value of the last
 * Device Capability register offset written to this file.
 *
 * The return value is the number characters written to buf, or EAGAIN
 * if the driver is busy and cannot service the read request immediately.
 * If EAGAIN is returned the caller should wait a little and retry the
 * read.
 *
 * The format of the string returned in buf is:
 * 	"offset:<offset>=<regvalue>
 * 	where:	<offset>	is the last Device Capability register offset
 * 						written to this file
 * 			<regvalue>	the currentl value of the Device Capability register
 * 						specified in offset
 */
ssize_t ReadDevCap(struct device *dev, struct device_attribute *attr,
					char *buf)
{
	uint8_t		regValue;
	int			status = -EINVAL;

	if(down_interruptible(&sii8334_irq_sem) != 0)
		return -ERESTARTSYS;

	do {
		if(gDriverContext.flags & MHL_STATE_FLAG_CONNECTED) {

			status = SiiTxGetPeerDevCapEntry(gDriverContext.devCapOffset,
											 &regValue);
			if(status != 0) {
				// Driver is busy and cannot provide the requested DEVCAP
				// register value right now so inform caller they need to
				// try again later.
				status = -EAGAIN;
				break;
			}
			status = scnprintf(buf, PAGE_SIZE, "offset:0x%02x=0x%02x",
								gDriverContext.devCapOffset, regValue);
		}
	} while(false);

	up(&sii8334_irq_sem);

	return status;
}



#define MAX_EVENT_STRING_LEN 40
/*****************************************************************************/
/**
 * @brief Handler for MHL hot plug detect status change notifications
 *  from the MhlTx layer.
 *
 *****************************************************************************/
void  AppNotifyMhlDownStreamHPDStatusChange(bool connected)
{
	char	event_string[MAX_EVENT_STRING_LEN];
	char	*envp[] = {event_string, NULL};

	snprintf(event_string, MAX_EVENT_STRING_LEN, "MHLEVENT=%s",
			connected? "HPD" : "NO_HPD");
	kobject_uevent_env(&gDriverContext.pDevice->kobj,
						KOBJ_CHANGE, envp);
}



/*****************************************************************************/
/**
 * @brief Handler for most of the event notifications from the MhlTx layer.
 *
 *****************************************************************************/
MhlTxNotifyEventsStatus_e  AppNotifyMhlEvent(uint8_t eventCode, uint8_t eventParam)
{
	char						event_string[MAX_EVENT_STRING_LEN];
	char						*envp[] = {event_string, NULL};
	MhlTxNotifyEventsStatus_e	rtnStatus = MHL_TX_EVENT_STATUS_PASSTHROUGH;
	char rcp_buf[MAX_EVENT_STRING_LEN];

	// Save the info on the most recent event.  This is done to support the
	// SII_MHL_GET_MHL_TX_EVENT IOCTL.  If at some point in the future the
	// driver's IOCTL interface is abandoned in favor of using sysfs attributes
	// this can be removed.
	gDriverContext.pendingEvent = eventCode;
	gDriverContext.pendingEventData = eventParam;

	pr_info("%s: eventCode[%d]\n",__func__,eventCode);

	switch(eventCode) {

		case MHL_TX_EVENT_CONNECTION:
			gDriverContext.flags |= MHL_STATE_FLAG_CONNECTED;
			strncpy(event_string, "MHLEVENT=connected", MAX_EVENT_STRING_LEN);
			kobject_uevent_env(&gDriverContext.pDevice->kobj,
								KOBJ_CHANGE, envp);
#ifdef CONFIG_MACH_LGE
                    /* HDMI HPD on when MHL is connected*/
                    pr_info("%s : HDMI HPD on when MHL is connected \n",__func__);
			if(mhl_common_state->hdmi_hpd_on)
				mhl_common_state->hdmi_hpd_on(1);
#endif
			break;

		case MHL_TX_EVENT_RCP_READY:
			gDriverContext.flags |= MHL_STATE_FLAG_RCP_READY;
			strncpy(event_string, "MHLEVENT=rcp_ready", MAX_EVENT_STRING_LEN);
			kobject_uevent_env(&gDriverContext.pDevice->kobj,
								KOBJ_CHANGE, envp);
			break;

		case MHL_TX_EVENT_DISCONNECTION:
			gDriverContext.flags = 0;
			gDriverContext.keyCode = 0;
			gDriverContext.errCode = 0;
			strncpy(event_string, "MHLEVENT=disconnected", MAX_EVENT_STRING_LEN);
			kobject_uevent_env(&gDriverContext.pDevice->kobj,
								KOBJ_CHANGE, envp);

#ifdef CONFIG_MACH_LGE
                    /* HDMI HPD off when MHL is disconnected */
                    pr_info("%s : HDMI HPD off when MHL is disconnected \n",__func__);
			if(mhl_common_state->hdmi_hpd_on)
				mhl_common_state->hdmi_hpd_on(0);
#endif
			break;

		case MHL_TX_EVENT_RCP_RECEIVED:
			gDriverContext.flags &= ~MHL_STATE_FLAG_RCP_SENT;
			gDriverContext.flags |= MHL_STATE_FLAG_RCP_RECEIVED;
			gDriverContext.keyCode = eventParam;
			snprintf(event_string, MAX_EVENT_STRING_LEN,
					"MHLEVENT=received_RCP key code=0x%02x", eventParam);
			sprintf(rcp_buf, "MHL_RCP=%d", eventParam);
			kobject_uevent_env(&gDriverContext.pDevice->kobj,
								KOBJ_CHANGE, envp);
#ifdef CONFIG_MACH_LGE
			pr_info("[MHL_RCP] %s\n",rcp_buf);
			if(mhl_common_state->send_uevent)
				mhl_common_state->send_uevent(rcp_buf);
#endif
			break;

		case MHL_TX_EVENT_RCPK_RECEIVED:
			if((gDriverContext.flags & MHL_STATE_FLAG_RCP_SENT)
				&& (gDriverContext.keyCode == eventParam)) {

				gDriverContext.flags |= MHL_STATE_FLAG_RCP_ACK;

				snprintf(event_string, MAX_EVENT_STRING_LEN,
						"MHLEVENT=received_RCPK key code=0x%02x", eventParam);
				sprintf(rcp_buf, "MHL_RCPK=%d", eventParam);
				kobject_uevent_env(&gDriverContext.pDevice->kobj,
									KOBJ_CHANGE, envp);
#ifdef CONFIG_MACH_LGE
			if(mhl_common_state->send_uevent)
				mhl_common_state->send_uevent(rcp_buf);
#endif
			}
			break;

		case MHL_TX_EVENT_RCPE_RECEIVED:
			if(gDriverContext.flags & MHL_STATE_FLAG_RCP_SENT) {

				gDriverContext.errCode = eventParam;
				gDriverContext.flags |= MHL_STATE_FLAG_RCP_NAK;

				snprintf(event_string, MAX_EVENT_STRING_LEN,
						"MHLEVENT=received_RCPE error code=0x%02x", eventParam);
				sprintf(rcp_buf, "MHL_RCPE=%d", eventParam);
				kobject_uevent_env(&gDriverContext.pDevice->kobj,
									KOBJ_CHANGE, envp);
#ifdef CONFIG_MACH_LGE
			if(mhl_common_state->send_uevent)
				mhl_common_state->send_uevent(rcp_buf);
#endif
			}
			break;

		case MHL_TX_EVENT_DCAP_CHG:
			snprintf(event_string, MAX_EVENT_STRING_LEN,
					"MHLEVENT=DEVCAP change");
			kobject_uevent_env(&gDriverContext.pDevice->kobj,
								KOBJ_CHANGE, envp);

			break;

		case MHL_TX_EVENT_DSCR_CHG:	// Scratch Pad registers have changed
			snprintf(event_string, MAX_EVENT_STRING_LEN,
					"MHLEVENT=SCRATCHPAD change");
			kobject_uevent_env(&gDriverContext.pDevice->kobj,
								KOBJ_CHANGE, envp);
			break;

		case MHL_TX_EVENT_POW_BIT_CHG:	// Peer's power capability has changed
			if(eventParam) {
				// Since downstream device is supplying VBUS power we should
				// turn off our VBUS power here.  If the platform application
				// can control VBUS power it should turn off it's VBUS power
				// now and return status of MHL_TX_EVENT_STATUS_HANDLED.  If
				// platform cannot control VBUS power it should return
				// MHL_TX_EVENT_STATUS_PASSTHROUGH to allow the MHL layer to
				// try to turn it off.

				// In this sample driver all that is done is to report an
				// event describing the requested state of VBUS power and
				// return MHL_TX_EVENT_STATUS_PASSTHROUGH to allow lower driver
				// layers to control VBUS power if possible.
				snprintf(event_string, MAX_EVENT_STRING_LEN,
						"MHLEVENT=MHL VBUS power OFF");

			} else {
				snprintf(event_string, MAX_EVENT_STRING_LEN,
						"MHLEVENT=MHL VBUS power ON");
			}

			kobject_uevent_env(&gDriverContext.pDevice->kobj,
								KOBJ_CHANGE, envp);
			break;

		case MHL_TX_EVENT_RGND_MHL:
			// RGND measurement has determine that the peer is an MHL device.
			// If platform application can determine that the attached device
			// is not supplying VBUS power it should turn on it's VBUS power
			// here and return MHL_TX_EVENT_STATUS_HANDLED to indicate to
			// indicate to the caller that it handled the notification.
			 rtnStatus = MHL_TX_EVENT_STATUS_HANDLED;
			 break;
			// In this sample driver all that is done is to report the event
			// and return MHL_TX_EVENT_STATUS_PASSTHROUGH to allow lower driver
			// layers to control VBUS power if possible.
			snprintf(event_string, MAX_EVENT_STRING_LEN,
					"MHLEVENT=MHL device detected");
			kobject_uevent_env(&gDriverContext.pDevice->kobj,
								KOBJ_CHANGE, envp);
			break;
	}
	return rtnStatus;
}

static irqreturn_t sii8334_mhl_tx_irq(int irq, void *handle)
{
	printk(KERN_INFO "%s\n",__func__);
	/*remove qup error*/
	MHLSleepStatus = 0;

	if (down_interruptible(&sii8334_irq_sem) == 0) {
		SiiMhlTxDeviceIsr();
		up(&sii8334_irq_sem);
	}

	return IRQ_HANDLED;
}

/* true if transmitter has been opened */
static bool	bTxOpen = false;
int32_t SiiMhlOpen(struct inode *pInode, struct file *pFile)
{

	if(bTxOpen)
		return -EBUSY;

	bTxOpen = true;		// flag driver has been opened
    return 0;
}

int32_t SiiMhlRelease(struct inode *pInode, struct file *pFile)
{
	bTxOpen = false;	// flag driver has been closed

	return 0;
}

long SiiMhlIoctl(struct file *pFile, unsigned int ioctlCode,
					unsigned long ioctlParam)
{
	mhlTxEventPacket_t		mhlTxEventPacket;
	mhlTxReadDevCap_t		mhlTxReadDevCap;
	mhlTxScratchPadAccess_t	mhlTxScratchPadAccess;
	ScratchPadStatus_e		scratchPadStatus;
	long					retStatus = 0;
	bool					status;

	if (down_interruptible(&sii8334_irq_sem) != 0)
		return -ERESTARTSYS;

	switch (ioctlCode)
	{
		case SII_MHL_GET_MHL_TX_EVENT:

			mhlTxEventPacket.event = gDriverContext.pendingEvent;
			mhlTxEventPacket.eventParam = gDriverContext.pendingEventData;

			if(copy_to_user((mhlTxEventPacket_t*)ioctlParam,
						 &mhlTxEventPacket, sizeof(mhlTxEventPacket_t))) {
				retStatus = -EFAULT;
				break;
			}

			gDriverContext.pendingEvent = MHL_TX_EVENT_NONE;
			gDriverContext.pendingEventData = 0;
			break;

		case SII_MHL_RCP_SEND:

			gDriverContext.flags &= ~(MHL_STATE_FLAG_RCP_RECEIVED |
									  MHL_STATE_FLAG_RCP_ACK |
									  MHL_STATE_FLAG_RCP_NAK);
			gDriverContext.flags |= MHL_STATE_FLAG_RCP_SENT;
			gDriverContext.keyCode = (uint8_t)ioctlParam;
			status = SiiMhlTxRcpSend((uint8_t)ioctlParam);
			break;

		case SII_MHL_RCP_SEND_ACK:

			status = SiiMhlTxRcpkSend((uint8_t)ioctlParam);
			break;

		case SII_MHL_RCP_SEND_ERR_ACK:

			status = SiiMhlTxRcpeSend((uint8_t)ioctlParam);
			break;

		case SII_MHL_GET_MHL_CONNECTION_STATUS:

			if(gDriverContext.flags & MHL_STATE_FLAG_RCP_READY)
				mhlTxEventPacket.event = MHL_TX_EVENT_RCP_READY;

			else if(gDriverContext.flags & MHL_STATE_FLAG_CONNECTED)
				mhlTxEventPacket.event = MHL_TX_EVENT_CONNECTION;

			else
				mhlTxEventPacket.event = MHL_TX_EVENT_DISCONNECTION;

			mhlTxEventPacket.eventParam = 0;

			if(copy_to_user((mhlTxEventPacket_t*)ioctlParam,
						 &mhlTxEventPacket, sizeof(mhlTxEventPacket_t))) {
				retStatus = -EFAULT;
			}
			break;

		case SII_MHL_GET_DEV_CAP_VALUE:

			if(copy_from_user(&mhlTxReadDevCap, (mhlTxReadDevCap_t*)ioctlParam,
						   	   sizeof(mhlTxReadDevCap_t))) {
				retStatus = -EFAULT;
				break;
			}

			// Make sure there is an MHL connection and that the requested
			// DEVCAP register number is valid.
			if(!(gDriverContext.flags & MHL_STATE_FLAG_RCP_READY) ||
				(mhlTxReadDevCap.regNum > 0x0F)) {
				retStatus = -EINVAL;
				break;
			}

			retStatus = SiiTxGetPeerDevCapEntry(mhlTxReadDevCap.regNum,
												&mhlTxReadDevCap.regValue);
			if(retStatus != 0) {
				// Driver is busy and cannot provide the requested DEVCAP
				// register value right now so inform caller they need to
				// try again later.
				retStatus = -EAGAIN;
				break;
			}

			if(copy_to_user((mhlTxReadDevCap_t*)ioctlParam,
							&mhlTxReadDevCap, sizeof(mhlTxReadDevCap_t))) {
				retStatus = -EFAULT;
			}
			break;

		case SII_MHL_WRITE_SCRATCH_PAD:

			if(copy_from_user(&mhlTxScratchPadAccess,
						   	  (mhlTxScratchPadAccess_t*)ioctlParam,
						   	  sizeof(mhlTxScratchPadAccess_t))) {
				retStatus = -EFAULT;
				break;
			}

			// Make sure there is an MHL connection and that the requested
			// data transfer parameters don't exceed the address space of
			// the scratch pad.  NOTE: The address space reserved for the
			// Scratch Pad registers is 64 bytes but sources and sink devices
			// are only required to implement the 1st 16 bytes.
			if(!(gDriverContext.flags & MHL_STATE_FLAG_RCP_READY) ||
				(mhlTxScratchPadAccess.length < ADOPTER_ID_SIZE) ||
				(mhlTxScratchPadAccess.length > MAX_SCRATCH_PAD_TRANSFER_SIZE) ||
				(mhlTxScratchPadAccess.offset >
					(MAX_SCRATCH_PAD_TRANSFER_SIZE - ADOPTER_ID_SIZE)) ||
				(mhlTxScratchPadAccess.offset + mhlTxScratchPadAccess.length >
					MAX_SCRATCH_PAD_TRANSFER_SIZE)) {
				retStatus = -EINVAL;
				break;
			}

			scratchPadStatus =  SiiMhlTxRequestWriteBurst(mhlTxScratchPadAccess.offset,
														  mhlTxScratchPadAccess.length,
														  mhlTxScratchPadAccess.data);
			switch(scratchPadStatus) {
				case SCRATCHPAD_SUCCESS:
					break;

				case SCRATCHPAD_BUSY:
					retStatus = -EAGAIN;
					break;

				default:
					retStatus = -EFAULT;
					break;
			}
			break;

		case SII_MHL_READ_SCRATCH_PAD:

			if(copy_from_user(&mhlTxScratchPadAccess,
							  (mhlTxScratchPadAccess_t*)ioctlParam,
							  sizeof(mhlTxScratchPadAccess_t))) {
				retStatus = -EFAULT;
				break;
			}

			// Make sure there is an MHL connection and that the requested
			// data transfer parameters don't exceed the address space of
			// the scratch pad.  NOTE: The address space reserved for the
			// Scratch Pad registers is 64 bytes but sources and sink devices
			// are only required to implement the 1st 16 bytes.
			if(!(gDriverContext.flags & MHL_STATE_FLAG_RCP_READY) ||
				(mhlTxScratchPadAccess.length > MAX_SCRATCH_PAD_TRANSFER_SIZE) ||
				(mhlTxScratchPadAccess.offset >= MAX_SCRATCH_PAD_TRANSFER_SIZE) ||
				(mhlTxScratchPadAccess.offset + mhlTxScratchPadAccess.length >
				 MAX_SCRATCH_PAD_TRANSFER_SIZE)) {
				retStatus = -EINVAL;
				break;
			}

			scratchPadStatus =  SiiGetScratchPadVector(mhlTxScratchPadAccess.offset,
													   mhlTxScratchPadAccess.length,
													   mhlTxScratchPadAccess.data);
			switch(scratchPadStatus) {
				case SCRATCHPAD_SUCCESS:
					break;

				case SCRATCHPAD_BUSY:
					retStatus = -EAGAIN;
					break;
				default:
					retStatus = -EFAULT;
					break;
			}
			break;

		default:
			retStatus = -ENOTTY;
	}

	up(&sii8334_irq_sem);

	return retStatus;
}

#ifdef CONFIG_MACH_LGE

#ifdef CONFIG_LG_MAGIC_MOTION_REMOCON

void SiiMhlTxReadScratchpad(void)
{
	int m_idx = 0;
	//pr_info("%s: magic is called\n",__func__);
	memset(mhlTxConfig.mscScratchpadData, 0x00, MHD_SCRATCHPAD_SIZE);

	for(m_idx=0; m_idx<(MHD_SCRATCHPAD_SIZE/2); m_idx++)
	{
		mhlTxConfig.mscScratchpadData[m_idx] = SiiRegRead( TX_PAGE_CBUS |( 0xC0+m_idx));
		//pr_info("[MAGIC] SiiMhlTxReadScratchpad, [%d] %x\n", m_idx, mhlTxConfig.mscScratchpadData[m_idx]);
	}

	MhlControl();
}

void SiiMhlTxWriteScratchpad(uint8_t *wdata)
{
	int i;

	SiiRegWrite(REG_CBUS_PRI_START,0xff);
	SiiRegWrite(REG_CBUS_PRI_ADDR_CMD, 0x40);

	SiiRegWrite(REG_MSC_WRITE_BURST_LEN, MHD_MAX_BUFFER_SIZE -1 );

	for ( i = 0; i < MHD_MAX_BUFFER_SIZE; i++ )
	{
		SiiRegWrite(REG_CBUS_SCRATCHPAD_0 + i,wdata[i] );
	}
	SiiRegWrite(REG_CBUS_PRI_START, MSC_START_BIT_WRITE_BURST );
}

int mhl_ms_ptr(signed short x, signed short y)	// mouse pointer, here because of speed
{
	return 0;
}
void mhl_ms_hide_cursor(void)	// in case mhl disconnection
{

}
int mhl_ms_btn(int action)		// mouse button...only left button, here becasue of speed
{
	return 0;
}
int mhl_kbd_key(unsigned int tKeyCode, int value)	// keyboard key input, here because of speed
{
	char mbuf[120];
	memset(mbuf, 0, sizeof(mbuf));
	sprintf(mbuf, "MHL_KEY press=%04d, keycode=%04d", value, tKeyCode);
	pr_info("[MAGIC] mhl_kbd_key %s\n",mbuf);

	 if(mhl_common_state->send_uevent)
		mhl_common_state->send_uevent(mbuf);

	return 0;
}
void mhl_writeburst_uevent(unsigned short mev)
{
	char env_buf[120];

	memset(env_buf, 0, sizeof(env_buf));

	switch(mev)
	{
		case 1:
			sprintf(env_buf, "hdmi_kbd_on");
			break;

		case 2:
			sprintf(env_buf, "hdmi_kbd_off");
			break;

		default:

			break;
	}

	if(mhl_common_state->send_uevent)
	      mhl_common_state->send_uevent(env_buf);

	pr_info("[MAGIC] mhl_writeburst_uevent %s\n",env_buf);
	return;
}


void MhlControl(void)
{
	uint8_t category = 0;
	uint8_t command = 0;
	uint8_t action = 0;
	uint8_t r_action = 0;
	int x_position = 0;
	int y_position = 0;
	unsigned short keyCode = 0;
	uint8_t plugResp[MHD_MAX_BUFFER_SIZE];
	signed char r_lval = 0;
	signed char r_uval = 0;
	int fdr = 3;
	int tmp_x = 0;
	int tmp_y = 0;
	char mbuf[120];

	static uint8_t pre_r_action = 0;
	static int pre_x_position = 0;
	static int pre_y_position = 0;

	category = mhlTxConfig.mscScratchpadData[0];

	if(category == 0x01)
	{
		command =  mhlTxConfig.mscScratchpadData[1];
		if(command == 0x01)			// Magic Motion Remote Controller -> absolute position, button
		{
			action = mhlTxConfig.mscScratchpadData[2];
			x_position = (int)((mhlTxConfig.mscScratchpadData[3] <<8) | mhlTxConfig.mscScratchpadData[4]);
			y_position = (int)((mhlTxConfig.mscScratchpadData[5] <<8) | mhlTxConfig.mscScratchpadData[6]);

			if((0 < tvCtl_x) && (0 < tvCtl_y))
			{
				x_position = x_position*MAGIC_CANVAS_X/tvCtl_x;
				y_position = y_position*MAGIC_CANVAS_Y/tvCtl_y;

				if(x_position>MAGIC_CANVAS_X) x_position = MAGIC_CANVAS_X;
				if(y_position>MAGIC_CANVAS_Y) y_position = MAGIC_CANVAS_Y;
			}

			if(action == 0x01)	r_action = 1;
			else if(action == 0x02) r_action = 0;
			else return;

			if(pre_r_action == r_action && pre_x_position == x_position && pre_y_position == y_position){
				pr_info("%s ->magic RC, double input, so remove this info. ",__func__);
				return;
			}
			else{
				pre_r_action = r_action;
				pre_x_position = x_position;
				pre_y_position = y_position;
			}

			memset(mbuf, 0, sizeof(mbuf));
			pr_info("%s ->magic RC, action:%d, (%d,%d)\n",__func__,r_action,x_position,y_position);
			sprintf(mbuf, "MHL_CTL action=%04d, x_pos=%04d, y_pos=%04d", r_action,x_position,y_position);
			if(mhl_common_state->send_uevent)
				mhl_common_state->send_uevent(mbuf);
		}
		else if(command == 0x02)		// mouse -> relative position + mouse button
		{
			action = mhlTxConfig.mscScratchpadData[2];

			r_lval = 0;
			r_uval = 0;
			r_lval = (signed char) mhlTxConfig.mscScratchpadData[4];
			r_uval = (signed char) mhlTxConfig.mscScratchpadData[3];
			x_position = (int)((r_uval<<8) | r_lval);

			r_lval = 0;
			r_uval = 0;
			r_lval = (signed char) mhlTxConfig.mscScratchpadData[6];
			r_uval = (signed char) mhlTxConfig.mscScratchpadData[5];
			y_position = (int)((r_uval<<8) | r_lval);

			pr_info("%s -> mouse, action:%d, (%d,%d) \n",__func__, action,x_position,y_position);

			if(((x_position> 100) || (x_position< -100)) ||((y_position> 100) || (y_position< -100)))
				fdr = 5;
			else if(((-30 < x_position) && (x_position < 30)) && ((-30 < y_position) && (y_position < 30)))
				fdr = 1;
			else
				fdr = 3;

			fdr = 7;
			x_position = fdr*x_position;
			y_position = fdr*y_position;

			if(action == 0x00)	// cursor
			{
				mhl_ms_ptr(x_position, y_position);
				return;
			}

			if((action == 0x03)) r_action = 1;
			else if((action == 0x02)) r_action = 0;
			else return;

			mhl_ms_ptr(x_position, y_position);
			mhl_ms_btn(r_action);
		}
		else if(command == 0x03)		// keyboard -> keycode includeing MENU/HOME/BACK
		{
			action = mhlTxConfig.mscScratchpadData[2];
			keyCode = ((mhlTxConfig.mscScratchpadData[3] <<8) | mhlTxConfig.mscScratchpadData[4]);

			pr_info("%s ->magic RC, action:%d, keyCode:%d \n",__func__,r_action,keyCode);

			if((keyCode == KEY_F11) || (keyCode == KEY_F12))	// F11:ZoomIn, F12:ZoomOut
			{
				char mbuf[120];

				if(action == 0x01)
				{
					if(keyCode == KEY_F11) r_action = 1;
					else r_action = 0;

					x_position = 9999;
					y_position = 9999;
					memset(mbuf, 0, sizeof(mbuf));
					pr_info("%s ->magic RC, action:%d, (%d,%d) \n",__func__,r_action,x_position,y_position);
					sprintf(mbuf, "MHL_CTL action=%04d, x_pos=%04d, y_pos=%04d", r_action,x_position,y_position);

				       if(mhl_common_state->send_uevent)
	      					mhl_common_state->send_uevent(mbuf);
				}
				return;
			}

			if(action == 0x01)
			{
				r_action = 1;
				if((kbd_key_pressed[0]>>8) == 0x00)
				{
					if(((kbd_key_pressed[1] >>8) == 0x01) && ((kbd_key_pressed[1]&0xFF) == keyCode))
					{
						pr_info("%s -> keyboard, duplicated pressed, %x \n",__func__, keyCode);
						return;
					}
					kbd_key_pressed[0] = (0x0100 | keyCode);
				}
				else
				{
					if((kbd_key_pressed[1] >>8) == 0x01)
					{
						pr_info("%s -> keyboard, duplicated pressed, %x \n",__func__, keyCode);
						return;
					}
					kbd_key_pressed[1] = (0x0100 | keyCode);
				}

/*				if(kbd_key_pressed == 0x01)
				{
					printk("%s -> keyboard, duplicated pressed, %d \n",__func__, keyCode);
					return;
				}
				else
				{
					kbd_key_pressed = 0x01;
				}
*/
			}
			else if(action == 0x02)
			{
				r_action = 0;

				if(((kbd_key_pressed[0]>>8) == 0x01) && ((kbd_key_pressed[0]&0xFF) == keyCode))
				{
					kbd_key_pressed[0] = 0x0000;
					mhl_kbd_key(keyCode,r_action);
					return;
				}
				if(((kbd_key_pressed[1]>>8) == 0x01) && ((kbd_key_pressed[1]&0xFF) == keyCode))
				{
					kbd_key_pressed[1] = 0x0000;
					mhl_kbd_key(keyCode,r_action);
					return;
				}

				pr_info("%s -> keyboard, duplicated released, %x \n",__func__, keyCode);
				return;

/*				if(kbd_key_pressed == 0x00)
				{
					printk("%s -> keyboard, duplicated released, %d \n",__func__, keyCode);
					return;
				}
				else
				{
					kbd_key_pressed = 0x00;
				}
*/
			}
			else return;

			mhl_kbd_key(keyCode,r_action);
		}
		else
		{
			pr_info("MhlControl : not defined message\n");
		}
	}
	else if(category == 0x02)
	{
		command =  mhlTxConfig.mscScratchpadData[1];
		action = mhlTxConfig.mscScratchpadData[2];

		if(command > 0x03)
		{
			pr_info("MhlControl : not defined message\n");
			return;
		}

		if(command == 0x01)	// TV magic motion remote controller
		{
			if(action == 0x01)
			{
				tmp_x = (int)((mhlTxConfig.mscScratchpadData[3] <<8) | mhlTxConfig.mscScratchpadData[4]);
				tmp_y = (int)((mhlTxConfig.mscScratchpadData[5] <<8) | mhlTxConfig.mscScratchpadData[6]);

				pr_info("%s -> TV control canvas (%d, %d) \n",__func__, tmp_x, tmp_y);
				if((((MAGIC_CANVAS_X/2) < tmp_x) && (tmp_x < 2*MAGIC_CANVAS_X))
					&& (((MAGIC_CANVAS_Y/2) < tmp_y) && (tmp_y < 2*MAGIC_CANVAS_Y)))
				{
					tvCtl_x = tmp_x;
					tvCtl_y = tmp_y;
				}
				else
				{
					tvCtl_x = MAGIC_CANVAS_X;
					tvCtl_y = MAGIC_CANVAS_Y;
				}
			}
		}
		else if(command == 0x02)	// mouse
		{
			if(action == 0x02)
			{
				mhl_ms_hide_cursor();
				pr_info("%s -> mouse UNPLUGGED.\n",__func__);
			}
			else;
		}
		else if(command == 0x03)	// keyboard
		{
			kbd_key_pressed[0] = 0x0000;
			kbd_key_pressed[1] = 0x0000;

			if(action == 0x01)
			{
				mhl_writeburst_uevent(1);
				pr_info("%s -> keyboard PLUGGED.\n",__func__);
			}
			else if(action == 0x02)
			{
				mhl_writeburst_uevent(2);
				pr_info("%s -> keyboard UNPLUGGED.\n",__func__);
			}
			else;
		}
		else;

		memcpy(plugResp, mhlTxConfig.mscScratchpadData, MHD_MAX_BUFFER_SIZE);
		plugResp[3] = 0x01;
		plugResp[4] = 0x00;
		plugResp[5] = 0x00;
		plugResp[6] = 0x00;
		SiiMhlTxWriteScratchpad(plugResp);
	}
	else
	{
		pr_info("MhlControl : not defined message\n");
		return;
	}
}

#endif /*CONFIG_LG_MAGIC_MOTION_REMOCON*/

#endif /* CONFIG_MACH_LGE*/

/**
 *  File operations supported by the MHL driver
 */
static const struct file_operations siiMhlFops = {
    .owner			= THIS_MODULE,
    .open			= SiiMhlOpen,
    .release		= SiiMhlRelease,
    .unlocked_ioctl	= SiiMhlIoctl
};

/*
 * Sysfs attribute files supported by this driver.
 */
struct device_attribute driver_attribs[] = {
	__ATTR(connection_state, 0444, ShowConnectionState, NULL),
	__ATTR(rcp_keycode, 0644, ShowRcp, SendRcp),
	__ATTR(rcp_ack, 0644, ShowRcpAck, SendRcpAck),
	__ATTR(devcap, 0644, ReadDevCap, SelectDevCap),
	__ATTR_NULL
};

int mhl_get_i2c_index(int addr)
{
	switch (addr) {
		case 0x39:
		case 0x72:
			return 0;
		case 0x3D:
		case 0x7A:
			return 1;
		case 0x49:
		case 0x92:
			return 2;
		case 0x4D:
		case 0x9A:
			return 3;
		case 0x64:
		case 0xC8:
			return 4;
    }

    return 0;
}

#if defined(CONFIG_MACH_LGE) && defined(LGE_MULTICORE_FASTBOOT)
static int sii8334_mhl_tx_probe_thread(void *arg)
{
    /* force power on - never power down */
	mhl_pdata[0]->power(1, 0);

	return StartMhlTxDevice();
}
#endif

static int32_t sii8334_mhl_tx_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
    int32_t	ret = 0;
    dev_t dev_no;

	printk(KERN_INFO "[MHL]%s\n",__func__);

	sii8334_mhl_i2c_client[mhl_get_i2c_index(client->addr)] = client;
	mhl_pdata[mhl_get_i2c_index(client->addr)] = client->dev.platform_data;

	++sii8334_i2c_dev_index;
	if (sii8334_i2c_dev_index < SII8334_I2C_DEVICE_NUMBER)
		goto error;

	/* If a major device number has already been selected use it,
	 * otherwise dynamically allocate one.
	 */
    if (devMajor) {
		dev_no = MKDEV(devMajor, 0);
		ret = register_chrdev_region(dev_no, MHL_DRIVER_MINOR_MAX,
			MHL_DRIVER_NAME);
    } else {
		ret = alloc_chrdev_region(&dev_no, 0, MHL_DRIVER_MINOR_MAX,
			MHL_DRIVER_NAME);
		devMajor = MAJOR(dev_no);
    }

    if (ret) {
		printk(KERN_ERR "register_chrdev %d, %s failed, error code: %d\n",
			devMajor, MHL_DRIVER_NAME, ret);
		goto error;
	}

	cdev_init(&siiMhlCdev, &siiMhlFops);
	siiMhlCdev.owner = THIS_MODULE;
	ret = cdev_add(&siiMhlCdev, dev_no, MHL_DRIVER_MINOR_MAX);
	if (ret) {
		printk(KERN_ERR "cdev_add %s failed %d\n", MHL_DRIVER_NAME, ret);
		goto free_chrdev;
	}

	siiMhlClass = class_create(THIS_MODULE, "mhl");
	if (IS_ERR(siiMhlClass)) {
		printk(KERN_ERR "class_create failed %d\n", ret);
		ret = PTR_ERR(siiMhlClass);
		goto free_cdev;
	}

	siiMhlClass->dev_attrs = driver_attribs;

	gDriverContext.pDevice = device_create(siiMhlClass, NULL,
		MKDEV(devMajor, 0), NULL, "%s", MHL_DEVICE_NAME);
	if (IS_ERR(gDriverContext.pDevice)) {
		printk(KERN_ERR "class_device_create failed %s %d\n", MHL_DEVICE_NAME, ret);
		ret = PTR_ERR(gDriverContext.pDevice);
		goto free_class;
	}

#ifdef CONFIG_MACH_LGE
	mhl_common_state =  (struct mhl_common_type *)kmalloc(sizeof(
		                              struct mhl_common_type), GFP_KERNEL);
	if(!mhl_common_state){
		pr_err("%s: memory allocation fail\n",__func__);
		ret = -ENOMEM;
		goto free_dev;
	}

	mhl_common_state->hdmi_hpd_on = hdmi_common_set_hpd_on;
	mhl_common_state->send_uevent = hdmi_common_send_uevent;

#endif	/* CONFIG_MACH_LGE */

	sema_init(&sii8334_irq_sem, 1);

	ret = request_threaded_irq(sii8334_mhl_i2c_client[0]->irq, NULL,
		sii8334_mhl_tx_irq, IRQF_TRIGGER_LOW | IRQF_ONESHOT, MHL_PART_NAME,
		NULL);

#if defined(CONFIG_MACH_LGE) && defined(LGE_MULTICORE_FASTBOOT)
	{
		struct task_struct *th;
		th = kthread_create(sii8334_mhl_tx_probe_thread, NULL, "sii8334_mhl_tx_probe");
		if (IS_ERR(th)) {
			ret = PTR_ERR(th);
			goto free_dev;
		}
		wake_up_process(th);
		return 0;
	}
#else
    /* force power on - never power down */
	mhl_pdata[0]->power(1, 0);

	ret = StartMhlTxDevice();
	if(ret) {
		pr_err("%s: StartMhlTxDevice fail\n", __func__);
		goto free_dev;
	}

	return 0;
#endif /* CONFIG_MACH_LGE && LGE_MULTICORE_FASTBOOT */

free_dev:
	device_destroy(siiMhlClass, MKDEV(devMajor, 0));

free_class:
	class_destroy(siiMhlClass);

free_cdev:
	cdev_del(&siiMhlCdev);

free_chrdev:
	unregister_chrdev_region(MKDEV(devMajor, 0), MHL_DRIVER_MINOR_MAX);

error:
	return ret;
}

static int32_t sii8334_mhl_tx_remove(struct i2c_client *client)
{
	sii8334_mhl_i2c_client[mhl_get_i2c_index(client->addr)] = NULL;

      device_destroy(siiMhlClass, MKDEV(devMajor, 0));
      class_destroy(siiMhlClass);
     unregister_chrdev_region(MKDEV(devMajor, 0), MHL_DRIVER_MINOR_MAX);

#ifdef CONFIG_MACH_LGE
    if(mhl_common_state){
        kfree(mhl_common_state);
        mhl_common_state = NULL;
     }
#endif

	return 0;
}

#ifdef CONFIG_MACH_LGE
static int32_t sii8334_mhl_tx_suspend(struct i2c_client *client, pm_message_t mesg)
{
    if (sii8334_mhl_suspended == true)
        return 0;
    sii8334_mhl_suspended = true;
    printk(KERN_INFO "[MHL]%s\n", __func__);
#if 0 /* keep power on */
    mhl_pdata[0]->power(0, 1);
#endif
    disable_irq(sii8334_mhl_i2c_client[0]->irq);

    return 0;
}

static int32_t sii8334_mhl_tx_resume(struct i2c_client *client)
{
#if 0 /* keep power on */
    int ret;
#endif
    if (sii8334_mhl_suspended == false)
        return 0;

    sii8334_mhl_suspended = false;
    printk(KERN_INFO "[MHL]%s\n", __func__);
#if 0 /* keep power on */
    ret = mhl_pdata[0]->power(1, 1);
    if (ret > -1)
        StartMhlTxDevice();
#endif
    enable_irq(sii8334_mhl_i2c_client[0]->irq);

    return 0;
}
#endif

/* LGE_CHANGE
 * generate MHL touch event using procfs.
 * USAGE : echo [action],[x],[y] > /proc/mhl_remocon
 *    ex) echo 1,500,400 > /proc/mhl_remocon
 * 2012-10-13, chaeuk.lee@lge.com
 */
#ifdef CONFIG_LGE_MHL_REMOCON_TEST
#define PROC_CMD_MAX_LEN 20
static int atoi(const char *name)
{
	int val = 0;

	for (;; name++) {
		switch (*name) {
			case '0' ... '9':
				val = 10*val+(*name-'0');
				break;
			default:
				return val;
		}
	}
}

static int lge_mhl_remocon_write_proc(struct file *file, const char __user *buffer,
		unsigned long count, void *data)
{
	char buf[PROC_CMD_MAX_LEN];
	char x_pos[5];
	char y_pos[5];
	int action, x, y;
	int ret;
	int i = 0;
	char *p;
	char mbuf[120];

	if (count >= PROC_CMD_MAX_LEN) {
		ret = -EINVAL;
		goto write_proc_failed;
	}

	if (copy_from_user(buf, buffer, PROC_CMD_MAX_LEN)) {
		ret = -EFAULT;
		goto write_proc_failed;
	}

	buf[count] = '\0';

	if (buf[0] == '1') action = 1;
	else if (buf[0] == '0') action = 0;
	else if (buf[0] == '2') action = 2;
	else action = -1;

	p = &buf[2];
	i = 0;
	while(*p != ',') {
		x_pos[i++] = *p++;
	}
	x_pos[i] = '\0';
	p++;

	i = 0;
	while(*p) {
		y_pos[i++] = *p++;
	}
	y_pos[i] = '\0';

	x = atoi(x_pos);
	y = atoi(y_pos);

	printk("%s : action[%d], x[%d], y[%d]\n", __func__, action, x, y);
	if (action == 2) {
		memset(mbuf, 0, sizeof(mbuf));
		pr_info("%s, MHL_REMOCON_TEST > action:%d, (%d,%d)\n", __func__, action, x, y);
		sprintf(mbuf, "MHL_CTL action=%04d, x_pos=%04d, y_pos=%04d", 1, x, y);
		if(mhl_common_state->send_uevent)
			mhl_common_state->send_uevent(mbuf);

		memset(mbuf, 0, sizeof(mbuf));
		pr_info("%s, MHL_REMOCON_TEST > action:%d, (%d,%d)\n", __func__, action, x, y);
		sprintf(mbuf, "MHL_CTL action=%04d, x_pos=%04d, y_pos=%04d", 0, x, y);
		if(mhl_common_state->send_uevent)
			mhl_common_state->send_uevent(mbuf);
	} else {
		memset(mbuf, 0, sizeof(mbuf));
		pr_info("%s, MHL_REMOCON_TEST > action:%d, (%d,%d)\n", __func__, action, x, y);
		sprintf(mbuf, "MHL_CTL action=%04d, x_pos=%04d, y_pos=%04d", action, x, y);
		if(mhl_common_state->send_uevent)
			mhl_common_state->send_uevent(mbuf);
	}

	return count;

write_proc_failed:
	return ret;
}
#endif

static const struct i2c_device_id sii8334_mhl_tx_id[] = {
	{ MHL_PART_NAME, 0 },
	{ }
};

static struct i2c_driver sii8334_mhl_tx_i2c_driver = {
	.driver = {
		.name = MHL_DRIVER_NAME,
	},
	.probe = sii8334_mhl_tx_probe,
	.remove = sii8334_mhl_tx_remove,
#ifdef CONFIG_MACH_LGE
	.suspend = sii8334_mhl_tx_suspend,
	.resume = sii8334_mhl_tx_resume,
#endif
	.id_table = sii8334_mhl_tx_id,
};

static int __init sii8334_mhl_tx_init(void)
{
#ifdef CONFIG_LGE_MHL_REMOCON_TEST
	struct proc_dir_entry *d_entry;

	d_entry = create_proc_entry("mhl_remocon",
			S_IWUSR | S_IWGRP, NULL);
	if (d_entry) {
		d_entry->read_proc = NULL;
		d_entry->write_proc = lge_mhl_remocon_write_proc;
		d_entry->data = NULL;
	}
#endif
	return i2c_add_driver(&sii8334_mhl_tx_i2c_driver);
}

static void __exit sii8334_mhl_tx_exit(void)
{
	i2c_del_driver(&sii8334_mhl_tx_i2c_driver);
}

module_init(sii8334_mhl_tx_init);
module_exit(sii8334_mhl_tx_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Silicon Image <http://www.siliconimage.com>");
MODULE_DESCRIPTION(MHL_DRIVER_DESC);
