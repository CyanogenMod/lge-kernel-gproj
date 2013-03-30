#ifndef _SII8334_MHL_TX_H
#define _SII8334_MHL_TX_H

enum        // Index into pageConfig_t array (shifted left by 8)
{
    TX_PAGE_TPI         = 0x0000,   // TPI
    TX_PAGE_L0          = 0x0000,   // TX Legacy page 0
    TX_PAGE_L1          = 0x0100,   // TX Legacy page 1
    TX_PAGE_2           = 0x0200,   // TX page 2
    TX_PAGE_3           = 0x0300,   // TX page 3
    TX_PAGE_CBUS        = 0x0400,   // CBUS
};

#define REG_SRST		(TX_PAGE_3 | 0x0000)		/* Was 0x0005 */

#define REG_INTR1       (TX_PAGE_L0| 0x0071)
#define REG_INTR1_MASK  (TX_PAGE_L0| 0x0075)
#define REG_INTR2       (TX_PAGE_L0| 0x0072)
#define     INTR2_PSTABLE 0x02
#define REG_TMDS_CCTRL  (TX_PAGE_L0| 0x0080)
#define REG_USB_CHARGE_PUMP (TX_PAGE_L0 | 0x00F8)
#define REG_DVI_CTRL3	(TX_PAGE_2 | 0x05)

#define REG_DISC_CTRL1	(TX_PAGE_3 | 0x0010)		/* Was 0x0090 */
#define REG_DISC_CTRL2	(TX_PAGE_3 | 0x0011)		/* Was 0x0091 */
#define REG_DISC_CTRL3	(TX_PAGE_3 | 0x0012)		/* Was 0x0092 */
#define REG_DISC_CTRL4	(TX_PAGE_3 | 0x0013)		/* Was 0x0093 */
#define REG_DISC_CTRL5	(TX_PAGE_3 | 0x0014)		/* Was 0x0094 */
#define REG_DISC_CTRL6	(TX_PAGE_3 | 0x0015)		/* Was 0x0095 */
#define REG_DISC_CTRL7	(TX_PAGE_3 | 0x0016)		/* Was 0x0096 */
#define REG_DISC_CTRL8	(TX_PAGE_3 | 0x0017)		/* Was 0x0097 */
#define REG_DISC_CTRL9	(TX_PAGE_3 | 0x0018)
#define REG_DISC_CTRL10	(TX_PAGE_3 | 0x0019)
#define REG_DISC_CTRL11	(TX_PAGE_3 | 0x001A)
#define REG_DISC_STAT	(TX_PAGE_3 | 0x001B)		/* Was 0x0098 */
#define REG_DISC_STAT2	(TX_PAGE_3 | 0x001C)		/* Was 0x0099 */

#define REG_INT_CTRL	(TX_PAGE_3 | 0x0020)		/* Was 0x0079 */
#define REG_INTR4		(TX_PAGE_3 | 0x0021)		/* Was 0x0074 */
#define     INTR4_SCDT_CHANGE 0x01
#define REG_INTR4_MASK	(TX_PAGE_3 | 0x0022)		/* Was 0x0078 */
#define REG_INTR5		(TX_PAGE_3 | 0x0023)
#define REG_INTR5_MASK	(TX_PAGE_3 | 0x0024)

#define REG_MHLTX_CTL1	(TX_PAGE_3 | 0x0030)		/* Was 0x00A0 */
#define REG_MHLTX_CTL2	(TX_PAGE_3 | 0x0031)		/* Was 0x00A1 */
#define REG_MHLTX_CTL3	(TX_PAGE_3 | 0x0032)		/* Was 0x00A2 */
#define REG_MHLTX_CTL4	(TX_PAGE_3 | 0x0033)		/* Was 0x00A3 */
#define REG_MHLTX_CTL5	(TX_PAGE_3 | 0x0034)		/* Was 0x00A4 */
#define REG_MHLTX_CTL6	(TX_PAGE_3 | 0x0035)		/* Was 0x00A5 */
#define REG_MHLTX_CTL7	(TX_PAGE_3 | 0x0036)		/* Was 0x00A6 */
#define REG_MHLTX_CTL8	(TX_PAGE_3 | 0x0037)

#define REG_TMDS_CSTAT	(TX_PAGE_3 | 0x0040)
#define     TMDS_CSTAT_SCDT 0x02

#define REG_MIPI_DSI_CTRL		(TX_PAGE_3 | 0x0092)
#define REG_MIPI_DSI_FORMAT		(TX_PAGE_3 | 0x0093)
#define REG_MIPI_DSI_PXL_FORMAT	(TX_PAGE_3 | 0x0094)

// ===================================================== //

#define TPI_SYSTEM_CONTROL_DATA_REG			(TX_PAGE_TPI | 0x001A)

#define LINK_INTEGRITY_MODE_MASK			(BIT6)
#define LINK_INTEGRITY_STATIC				(0x00)
#define LINK_INTEGRITY_DYNAMIC				(0x40)

#define TMDS_OUTPUT_CONTROL_MASK			(BIT4)
#define TMDS_OUTPUT_CONTROL_ACTIVE			(0x00)
#define TMDS_OUTPUT_CONTROL_POWER_DOWN		(0x10)

#define AV_MUTE_MASK						(BIT3)
#define AV_MUTE_NORMAL						(0x00)
#define AV_MUTE_MUTED						(0x08)

#define DDC_BUS_REQUEST_MASK				(BIT2)
#define DDC_BUS_REQUEST_NOT_USING			(0x00)
#define DDC_BUS_REQUEST_REQUESTED			(0x04)

#define DDC_BUS_GRANT_MASK					(BIT1)
#define DDC_BUS_GRANT_NOT_AVAILABLE			(0x00)
#define DDC_BUS_GRANT_GRANTED				(0x02)

#define OUTPUT_MODE_MASK					(BIT0)
#define OUTPUT_MODE_DVI						(0x00)
#define OUTPUT_MODE_HDMI					(0x01)

// ===================================================== //

#define TPI_DEVICE_POWER_STATE_CTRL_REG		(TX_PAGE_TPI | 0x001E)

#define CTRL_PIN_CONTROL_MASK				(BIT4)
#define CTRL_PIN_TRISTATE					(0x00)
#define CTRL_PIN_DRIVEN_TX_BRIDGE			(0x10)

#define TX_POWER_STATE_MASK					(BIT1 | BIT0)
#define TX_POWER_STATE_D0					(0x00)
#define TX_POWER_STATE_D2					(0x02)
#define TX_POWER_STATE_D3					(0x03)

/*\
| | HDCP Implementation
| |
| | HDCP link security logic is implemented in certain transmitters; unique
| |   keys are embedded in each chip as part of the solution. The security
| |   scheme is fully automatic and handled completely by the hardware.
\*/

/// HDCP Query Data Register ============================================== ///

#define TPI_HDCP_QUERY_DATA_REG				(TX_PAGE_TPI | 0x0029)

#define EXTENDED_LINK_PROTECTION_MASK		(BIT7)
#define EXTENDED_LINK_PROTECTION_NONE		(0x00)
#define EXTENDED_LINK_PROTECTION_SECURE		(0x80)

#define LOCAL_LINK_PROTECTION_MASK			(BIT6)
#define LOCAL_LINK_PROTECTION_NONE			(0x00)
#define LOCAL_LINK_PROTECTION_SECURE		(0x40)

#define LINK_STATUS_MASK					(BIT5 | BIT4)
#define LINK_STATUS_NORMAL					(0x00)
#define LINK_STATUS_LINK_LOST				(0x10)
#define LINK_STATUS_RENEGOTIATION_REQ		(0x20)
#define LINK_STATUS_LINK_SUSPENDED			(0x30)

#define HDCP_REPEATER_MASK					(BIT3)
#define HDCP_REPEATER_NO					(0x00)
#define HDCP_REPEATER_YES					(0x08)

#define CONNECTOR_TYPE_MASK					(BIT2 | BIT0)
#define CONNECTOR_TYPE_DVI					(0x00)
#define CONNECTOR_TYPE_RSVD					(0x01)
#define CONNECTOR_TYPE_HDMI					(0x04)
#define CONNECTOR_TYPE_FUTURE				(0x05)

#define PROTECTION_TYPE_MASK				(BIT1)
#define PROTECTION_TYPE_NONE				(0x00)
#define PROTECTION_TYPE_HDCP				(0x02)

/// HDCP Control Data Register ============================================ ///

#define TPI_HDCP_CONTROL_DATA_REG			(TX_PAGE_TPI | 0x002A)

#define PROTECTION_LEVEL_MASK				(BIT0)
#define PROTECTION_LEVEL_MIN				(0x00)
#define PROTECTION_LEVEL_MAX				(0x01)

#define KSV_FORWARD_MASK					(BIT4)
#define KSV_FORWARD_ENABLE					(0x10)
#define KSV_FORWARD_DISABLE					(0x00)

/// HDCP BKSV Registers =================================================== ///

#define TPI_BKSV_1_REG						(TX_PAGE_TPI | 0x002B)
#define TPI_BKSV_2_REG						(TX_PAGE_TPI | 0x002C)
#define TPI_BKSV_3_REG						(TX_PAGE_TPI | 0x002D)
#define TPI_BKSV_4_REG						(TX_PAGE_TPI | 0x002E)
#define TPI_BKSV_5_REG						(TX_PAGE_TPI | 0x002F)

/// HDCP Revision Data Register =========================================== ///

#define TPI_HDCP_REVISION_DATA_REG			(TX_PAGE_TPI | 0x0030)

#define HDCP_MAJOR_REVISION_MASK			(BIT7 | BIT6 | BIT5 | BIT4)
#define HDCP_MAJOR_REVISION_VALUE			(0x10)

#define HDCP_MINOR_REVISION_MASK			(BIT3 | BIT2 | BIT1 | BIT0)
#define HDCP_MINOR_REVISION_VALUE			(0x02)

/// HDCP KSV and V' Value Data Register =================================== ///

#define TPI_V_PRIME_SELECTOR_REG			(TX_PAGE_TPI | 0x0031)

/// V' Value Readback Registers =========================================== ///

#define TPI_V_PRIME_7_0_REG					(TX_PAGE_TPI | 0x0032)
#define TPI_V_PRIME_15_9_REG				(TX_PAGE_TPI | 0x0033)
#define TPI_V_PRIME_23_16_REG				(TX_PAGE_TPI | 0x0034)
#define TPI_V_PRIME_31_24_REG				(TX_PAGE_TPI | 0x0035)

/// HDCP AKSV Registers =================================================== ///

#define TPI_AKSV_1_REG						(TX_PAGE_TPI | 0x0036)
#define TPI_AKSV_2_REG						(TX_PAGE_TPI | 0x0037)
#define TPI_AKSV_3_REG						(TX_PAGE_TPI | 0x0038)
#define TPI_AKSV_4_REG						(TX_PAGE_TPI | 0x0039)
#define TPI_AKSV_5_REG						(TX_PAGE_TPI | 0x003A)

/// Interrupt Status Register ============================================= ///

#define TPI_INTERRUPT_STATUS_REG			(TX_PAGE_TPI | 0x003D)

#define HDCP_AUTH_STATUS_CHANGE_EVENT_MASK	(BIT7)
#define HDCP_AUTH_STATUS_CHANGE_EVENT_NO	(0x00)
#define HDCP_AUTH_STATUS_CHANGE_EVENT_YES	(0x80)

#define HDCP_VPRIME_VALUE_READY_EVENT_MASK	(BIT6)
#define HDCP_VPRIME_VALUE_READY_EVENT_NO	(0x00)
#define HDCP_VPRIME_VALUE_READY_EVENT_YES	(0x40)

#define HDCP_SECURITY_CHANGE_EVENT_MASK		(BIT5)
#define HDCP_SECURITY_CHANGE_EVENT_NO		(0x00)
#define HDCP_SECURITY_CHANGE_EVENT_YES		(0x20)

#define AUDIO_ERROR_EVENT_MASK				(BIT4)
#define AUDIO_ERROR_EVENT_NO				(0x00)
#define AUDIO_ERROR_EVENT_YES				(0x10)

#define CPI_EVENT_MASK						(BIT3)
#define CPI_EVENT_NO						(0x00)
#define CPI_EVENT_YES						(0x08)
#define RX_SENSE_MASK						(BIT3)		/* This bit is dual purpose depending on the value of 0x3C[3] */
#define RX_SENSE_NOT_ATTACHED				(0x00)
#define RX_SENSE_ATTACHED					(0x08)

#define HOT_PLUG_PIN_STATE_MASK				(BIT2)
#define HOT_PLUG_PIN_STATE_LOW				(0x00)
#define HOT_PLUG_PIN_STATE_HIGH				(0x04)

#define RECEIVER_SENSE_EVENT_MASK			(BIT1)
#define RECEIVER_SENSE_EVENT_NO				(0x00)
#define RECEIVER_SENSE_EVENT_YES			(0x02)

#define HOT_PLUG_EVENT_MASK					(BIT0)
#define HOT_PLUG_EVENT_NO					(0x00)
#define HOT_PLUG_EVENT_YES					(0x01)

///////////////////////////////////////////////////////////////////////////////
//
// CBUS register defintions
//
#define REG_CBUS_INTR_STATUS            (TX_PAGE_CBUS | 0x0008)
#define BIT_DDC_ABORT                   (BIT2)    /* Responder aborted DDC command at translation layer */
#define BIT_MSC_MSG_RCV                 (BIT3)    /* Responder sent a VS_MSG packet (response data or command.) */
#define BIT_MSC_XFR_DONE                (BIT4)    /* Responder sent ACK packet (not VS_MSG) */
#define BIT_MSC_XFR_ABORT               (BIT5)    /* Command send aborted on TX side */
#define BIT_MSC_ABORT                   (BIT6)    /* Responder aborted MSC command at translation layer */

#define REG_CBUS_INTR_ENABLE            (TX_PAGE_CBUS | 0x0009)

#define REG_DDC_ABORT_REASON        	(TX_PAGE_CBUS | 0x000B)
#define REG_CBUS_BUS_STATUS             (TX_PAGE_CBUS | 0x000A)
#define BIT_BUS_CONNECTED                   0x01
#define BIT_LA_VAL_CHG                      0x02

#define REG_PRI_XFR_ABORT_REASON        (TX_PAGE_CBUS | 0x000D)

#define REG_CBUS_PRI_FWR_ABORT_REASON   (TX_PAGE_CBUS | 0x000E)
#define	CBUSABORT_BIT_REQ_MAXFAIL			(0x01 << 0)
#define	CBUSABORT_BIT_PROTOCOL_ERROR		(0x01 << 1)
#define	CBUSABORT_BIT_REQ_TIMEOUT			(0x01 << 2)
#define	CBUSABORT_BIT_UNDEFINED_OPCODE		(0x01 << 3)
#define	CBUSSTATUS_BIT_CONNECTED			(0x01 << 6)
#define	CBUSABORT_BIT_PEER_ABORTED			(0x01 << 7)

#define REG_CBUS_PRI_START              (TX_PAGE_CBUS | 0x0012)
#define BIT_TRANSFER_PVT_CMD                0x01
#define BIT_SEND_MSC_MSG                    0x02
#define	MSC_START_BIT_MSC_CMD		        (0x01 << 0)
#define	MSC_START_BIT_VS_CMD		        (0x01 << 1)
#define	MSC_START_BIT_READ_REG		        (0x01 << 2)
#define	MSC_START_BIT_WRITE_REG		        (0x01 << 3)
#define	MSC_START_BIT_WRITE_BURST	        (0x01 << 4)

#define REG_CBUS_PRI_ADDR_CMD           (TX_PAGE_CBUS | 0x0013)
#define REG_CBUS_PRI_WR_DATA_1ST        (TX_PAGE_CBUS | 0x0014)
#define REG_CBUS_PRI_WR_DATA_2ND        (TX_PAGE_CBUS | 0x0015)
#define REG_CBUS_PRI_RD_DATA_1ST        (TX_PAGE_CBUS | 0x0016)
#define REG_CBUS_PRI_RD_DATA_2ND        (TX_PAGE_CBUS | 0x0017)

#define REG_CBUS_PRI_VS_CMD             (TX_PAGE_CBUS | 0x0018)
#define REG_CBUS_PRI_VS_DATA            (TX_PAGE_CBUS | 0x0019)

#define	MSC_REQUESTOR_DONE_NACK         	(0x01 << 6)

#define	REG_CBUS_MSC_RETRY_INTERVAL		(TX_PAGE_CBUS | 0x001A)		/* default is 16 */
#define	REG_CBUS_DDC_FAIL_LIMIT			(TX_PAGE_CBUS | 0x001C)		/* default is 5  */
#define	REG_CBUS_MSC_FAIL_LIMIT			(TX_PAGE_CBUS | 0x001D)		/* default is 5  */
#define	REG_CBUS_MSC_INT2_STATUS        (TX_PAGE_CBUS | 0x001E)
#define REG_CBUS_MSC_INT2_ENABLE        (TX_PAGE_CBUS | 0x001F)
#define	MSC_INT2_REQ_WRITE_MSC              (0x01 << 0)	/* Write REG data written. */
#define	MSC_INT2_HEARTBEAT_MAXFAIL          (0x01 << 1)	/* Retry threshold exceeded for sending the Heartbeat */

#define	REG_MSC_WRITE_BURST_LEN         (TX_PAGE_CBUS | 0x0020)       /* only for WRITE_BURST */

#define	REG_MSC_HEARTBEAT_CONTROL       (TX_PAGE_CBUS | 0x0021)       /* Periodic heart beat. TX sends GET_REV_ID MSC command */
#define	MSC_HEARTBEAT_PERIOD_MASK		    0x0F	/**/ bits 3..0
#define	MSC_HEARTBEAT_FAIL_LIMIT_MASK	    0x70	/**/ bits 6..4
#define	MSC_HEARTBEAT_ENABLE			    0x80	/**/ bit 7

#define REG_MSC_TIMEOUT_LIMIT           (TX_PAGE_CBUS | 0x0022)
#define	MSC_TIMEOUT_LIMIT_MSB_MASK	        (0x0F)	        /* default is 1           */
#define	MSC_LEGACY_BIT					    (0x01 << 7)	    /* This should be cleared.*/

#define	REG_CBUS_LINK_CONTROL_1			(TX_PAGE_CBUS | 0x0030)
#define	REG_CBUS_LINK_CONTROL_2			(TX_PAGE_CBUS | 0x0031)
#define	REG_CBUS_LINK_CONTROL_3			(TX_PAGE_CBUS | 0x0032)
#define	REG_CBUS_LINK_CONTROL_4			(TX_PAGE_CBUS | 0x0033)
#define	REG_CBUS_LINK_CONTROL_5			(TX_PAGE_CBUS | 0x0034)
#define	REG_CBUS_LINK_CONTROL_6			(TX_PAGE_CBUS | 0x0035)
#define	REG_CBUS_LINK_CONTROL_7			(TX_PAGE_CBUS | 0x0036)
#define REG_CBUS_LINK_STATUS_1          (TX_PAGE_CBUS | 0x0037)
#define REG_CBUS_LINK_STATUS_2          (TX_PAGE_CBUS | 0x0038)
#define	REG_CBUS_LINK_CONTROL_8			(TX_PAGE_CBUS | 0x0039)
#define	REG_CBUS_LINK_CONTROL_9			(TX_PAGE_CBUS | 0x003A)
#define	REG_CBUS_LINK_CONTROL_10		(TX_PAGE_CBUS | 0x003B)
#define	REG_CBUS_LINK_CONTROL_11		(TX_PAGE_CBUS | 0x003C)
#define	REG_CBUS_LINK_CONTROL_12		(TX_PAGE_CBUS | 0x003D)


#define REG_CBUS_LINK_CTRL9_0			(TX_PAGE_CBUS | 0x003A)
#define REG_CBUS_LINK_CTRL9_1           (TX_PAGE_CBUS | 0x00BA)

#define	REG_CBUS_DRV_STRENGTH_0			(TX_PAGE_CBUS | 0x0040)
#define	REG_CBUS_DRV_STRENGTH_1			(TX_PAGE_CBUS | 0x0041)
#define	REG_CBUS_ACK_CONTROL			(TX_PAGE_CBUS | 0x0042)
#define	REG_CBUS_CAL_CONTROL			(TX_PAGE_CBUS | 0x0043)	/* Calibration */

#define REG_CBUS_SCRATCHPAD_0           (TX_PAGE_CBUS | 0x00C0)
#define REG_CBUS_DEVICE_CAP_0           (TX_PAGE_CBUS | 0x0080)
#define REG_CBUS_DEVICE_CAP_1           (TX_PAGE_CBUS | 0x0081)
#define REG_CBUS_DEVICE_CAP_2           (TX_PAGE_CBUS | 0x0082)
#define REG_CBUS_DEVICE_CAP_3           (TX_PAGE_CBUS | 0x0083)
#define REG_CBUS_DEVICE_CAP_4           (TX_PAGE_CBUS | 0x0084)
#define REG_CBUS_DEVICE_CAP_5           (TX_PAGE_CBUS | 0x0085)
#define REG_CBUS_DEVICE_CAP_6           (TX_PAGE_CBUS | 0x0086)
#define REG_CBUS_DEVICE_CAP_7           (TX_PAGE_CBUS | 0x0087)
#define REG_CBUS_DEVICE_CAP_8           (TX_PAGE_CBUS | 0x0088)
#define REG_CBUS_DEVICE_CAP_9           (TX_PAGE_CBUS | 0x0089)
#define REG_CBUS_DEVICE_CAP_A           (TX_PAGE_CBUS | 0x008A)
#define REG_CBUS_DEVICE_CAP_B           (TX_PAGE_CBUS | 0x008B)
#define REG_CBUS_DEVICE_CAP_C           (TX_PAGE_CBUS | 0x008C)
#define REG_CBUS_DEVICE_CAP_D           (TX_PAGE_CBUS | 0x008D)
#define REG_CBUS_DEVICE_CAP_E           (TX_PAGE_CBUS | 0x008E)
#define REG_CBUS_DEVICE_CAP_F           (TX_PAGE_CBUS | 0x008F)
#define REG_CBUS_SET_INT_0				(TX_PAGE_CBUS | 0x00A0)
#define REG_CBUS_SET_INT_1				(TX_PAGE_CBUS | 0x00A1)
#define REG_CBUS_SET_INT_2				(TX_PAGE_CBUS | 0x00A2)
#define REG_CBUS_SET_INT_3				(TX_PAGE_CBUS | 0x00A3)
#define REG_CBUS_WRITE_STAT_0        	(TX_PAGE_CBUS | 0x00B0)
#define REG_CBUS_WRITE_STAT_1        	(TX_PAGE_CBUS | 0x00B1)
#define REG_CBUS_WRITE_STAT_2        	(TX_PAGE_CBUS | 0x00B2)
#define REG_CBUS_WRITE_STAT_3        	(TX_PAGE_CBUS | 0x00B3)

#define	MHL_LOGICAL_DEVICE_MAP		(MHL_DEV_LD_AUDIO | MHL_DEV_LD_VIDEO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_GUI )

#define BIT0                    0x01
#define BIT1                    0x02
#define BIT2                    0x04
#define BIT3                    0x08
#define BIT4                    0x10
#define BIT5                    0x20
#define BIT6                    0x40
#define BIT7                    0x80

#define LOW                     0
#define HIGH                    1

typedef enum
{
     DEVCAP_OFFSET_DEV_STATE 		= 0x00
    ,DEVCAP_OFFSET_MHL_VERSION		= 0x01
    ,DEVCAP_OFFSET_DEV_CAT          = 0x02
    ,DEVCAP_OFFSET_ADOPTER_ID_H     = 0x03
    ,DEVCAP_OFFSET_ADOPTER_ID_L     = 0x04
    ,DEVCAP_OFFSET_VID_LINK_MODE    = 0x05
    ,DEVCAP_OFFSET_AUD_LINK_MODE    = 0x06
    ,DEVCAP_OFFSET_VIDEO_TYPE       = 0x07
    ,DEVCAP_OFFSET_LOG_DEV_MAP      = 0x08
    ,DEVCAP_OFFSET_BANDWIDTH        = 0x09
    ,DEVCAP_OFFSET_FEATURE_FLAG     = 0x0A
    ,DEVCAP_OFFSET_DEVICE_ID_H      = 0x0B
    ,DEVCAP_OFFSET_DEVICE_ID_L      = 0x0C
    ,DEVCAP_OFFSET_SCRATCHPAD_SIZE  = 0x0D
    ,DEVCAP_OFFSET_INT_STAT_SIZE    = 0x0E
    ,DEVCAP_OFFSET_RESERVED         = 0x0F
    // this one must be last
    ,DEVCAP_SIZE
}DevCapOffset_e;

// Device Power State
#define MHL_DEV_UNPOWERED		0x00
#define MHL_DEV_INACTIVE		0x01
#define MHL_DEV_QUIET			0x03
#define MHL_DEV_ACTIVE			0x04

// Version that this chip supports
#define	MHL_VER_MAJOR		(0x01 << 4)	// bits 4..7
#define	MHL_VER_MINOR		0x01		// bits 0..3
#define MHL_VERSION						(MHL_VER_MAJOR | MHL_VER_MINOR)

//Device Category
#define	MHL_DEV_CATEGORY_OFFSET				DEVCAP_OFFSET_DEV_CAT
#define	MHL_DEV_CATEGORY_POW_BIT			(BIT4)

#define	MHL_DEV_CAT_SINK					0x01
#define	MHL_DEV_CAT_SOURCE					0x02
#define	MHL_DEV_CAT_DONGLE					0x03
#define	MHL_DEV_CAT_SELF_POWERED_DONGLE		0x13

//Video Link Mode
#define	MHL_DEV_VID_LINK_SUPPRGB444			0x01
#define	MHL_DEV_VID_LINK_SUPPYCBCR444		0x02
#define	MHL_DEV_VID_LINK_SUPPYCBCR422		0x04
#define	MHL_DEV_VID_LINK_SUPP_PPIXEL		0x08
#define	MHL_DEV_VID_LINK_SUPP_ISLANDS		0x10

//Audio Link Mode Support
#define	MHL_DEV_AUD_LINK_2CH				0x01
#define	MHL_DEV_AUD_LINK_8CH				0x02


//Feature Flag in the devcap
#define	MHL_DEV_FEATURE_FLAG_OFFSET			DEVCAP_OFFSET_FEATURE_FLAG
#define	MHL_FEATURE_RCP_SUPPORT				BIT0	// Dongles have freedom to not support RCP
#define	MHL_FEATURE_RAP_SUPPORT				BIT1	// Dongles have freedom to not support RAP
#define	MHL_FEATURE_SP_SUPPORT				BIT2	// Dongles have freedom to not support SCRATCHPAD

/*
#define		MHL_POWER_SUPPLY_CAPACITY		16		// 160 mA current
#define		MHL_POWER_SUPPLY_PROVIDED		16		// 160mA 0r 0 for Wolverine.
#define		MHL_HDCP_STATUS					0		// Bits set dynamically
*/

// VIDEO TYPES
#define		MHL_VT_GRAPHICS					0x00
#define		MHL_VT_PHOTO					0x02
#define		MHL_VT_CINEMA					0x04
#define		MHL_VT_GAMES					0x08
#define		MHL_SUPP_VT						0x80

//Logical Dev Map
#define	MHL_DEV_LD_DISPLAY					(0x01 << 0)
#define	MHL_DEV_LD_VIDEO					(0x01 << 1)
#define	MHL_DEV_LD_AUDIO					(0x01 << 2)
#define	MHL_DEV_LD_MEDIA					(0x01 << 3)
#define	MHL_DEV_LD_TUNER					(0x01 << 4)
#define	MHL_DEV_LD_RECORD					(0x01 << 5)
#define	MHL_DEV_LD_SPEAKER					(0x01 << 6)
#define	MHL_DEV_LD_GUI						(0x01 << 7)

//Bandwidth
#define	MHL_BANDWIDTH_LIMIT					22		// 225 MHz


#define MHL_STATUS_REG_CONNECTED_RDY        0x30
#define MHL_STATUS_REG_LINK_MODE            0x31

#define	MHL_STATUS_DCAP_RDY					BIT0

#define MHL_STATUS_CLK_MODE_MASK            0x07
#define MHL_STATUS_CLK_MODE_PACKED_PIXEL    0x02
#define MHL_STATUS_CLK_MODE_NORMAL          0x03
#define MHL_STATUS_PATH_EN_MASK             0x08
#define MHL_STATUS_PATH_ENABLED             0x08
#define MHL_STATUS_PATH_DISABLED            0x00
#define MHL_STATUS_MUTED_MASK               0x10

#define MHL_RCHANGE_INT                     0x20
#define MHL_DCHANGE_INT                     0x21

#define	MHL_INT_DCAP_CHG					BIT0
#define MHL_INT_DSCR_CHG                    BIT1
#define MHL_INT_REQ_WRT                     BIT2
#define MHL_INT_GRT_WRT                     BIT3

// On INTR_1 the EDID_CHG is located at BIT 0
#define	MHL_INT_EDID_CHG					BIT1

#define		MHL_INT_AND_STATUS_SIZE			0x33		// This contains one nibble each - max offset
#define		MHL_SCRATCHPAD_SIZE				16
#define		MHL_MAX_BUFFER_SIZE				MHL_SCRATCHPAD_SIZE	// manually define highest number

enum
{
    MHL_MSC_MSG_RCP             = 0x10,     // RCP sub-command
    MHL_MSC_MSG_RCPK            = 0x11,     // RCP Acknowledge sub-command
    MHL_MSC_MSG_RCPE            = 0x12,     // RCP Error sub-command
    MHL_MSC_MSG_RAP             = 0x20,     // Mode Change Warning sub-command
    MHL_MSC_MSG_RAPK            = 0x21,     // MCW Acknowledge sub-command
};

#define	RCPE_NO_ERROR				0x00
#define	RCPE_INEEFECTIVE_KEY_CODE	0x01
#define	RCPE_BUSY					0x02
//
// MHL spec related defines
//
enum
{
	MHL_ACK						= 0x33,	// Command or Data byte acknowledge
	MHL_NACK					= 0x34,	// Command or Data byte not acknowledge
	MHL_ABORT					= 0x35,	// Transaction abort
	MHL_WRITE_STAT				= 0x60 | 0x80,	// 0xE0 - Write one status register strip top bit
	MHL_SET_INT					= 0x60,	// Write one interrupt register
	MHL_READ_DEVCAP				= 0x61,	// Read one register
	MHL_GET_STATE				= 0x62,	// Read CBUS revision level from follower
	MHL_GET_VENDOR_ID			= 0x63,	// Read vendor ID value from follower.
	MHL_SET_HPD					= 0x64,	// Set Hot Plug Detect in follower
	MHL_CLR_HPD					= 0x65,	// Clear Hot Plug Detect in follower
	MHL_SET_CAP_ID				= 0x66,	// Set Capture ID for downstream device.
	MHL_GET_CAP_ID				= 0x67,	// Get Capture ID from downstream device.
	MHL_MSC_MSG					= 0x68,	// VS command to send RCP sub-commands
	MHL_GET_SC1_ERRORCODE		= 0x69,	// Get Vendor-Specific command error code.
	MHL_GET_DDC_ERRORCODE		= 0x6A,	// Get DDC channel command error code.
	MHL_GET_MSC_ERRORCODE		= 0x6B,	// Get MSC command error code.
	MHL_WRITE_BURST				= 0x6C,	// Write 1-16 bytes to responder's scratchpad.
	MHL_GET_SC3_ERRORCODE		= 0x6D,	// Get channel 3 command error code.
};

#define	MHL_RAP_CONTENT_ON		0x10	// Turn content streaming ON.
#define	MHL_RAP_CONTENT_OFF		0x11	// Turn content streaming OFF.

///////////////////////////////////////////////////////////////////////////////
//
// MHL Timings applicable to this driver.
//
//
#define	T_SRC_VBUS_CBUS_TO_STABLE	(200)	// 100 - 1000 milliseconds. Per MHL 1.0 Specs
#define	T_SRC_WAKE_PULSE_WIDTH_1	(20)	// 20 milliseconds. Per MHL 1.0 Specs
#define	T_SRC_WAKE_PULSE_WIDTH_2	(60)	// 60 milliseconds. Per MHL 1.0 Specs

#define	T_SRC_WAKE_TO_DISCOVER		(500)	// 100 - 1000 milliseconds. Per MHL 1.0 Specs

#define T_SRC_VBUS_CBUS_T0_STABLE 	(500)

// Allow RSEN to stay low this much before reacting.
// Per specs between 100 to 200 ms
#define	T_SRC_RSEN_DEGLITCH			(100)	// (150)

// Wait this much after connection before reacting to RSEN (300-500ms)
// Per specs between 300 to 500 ms
#define	T_SRC_RXSENSE_CHK			(400)


typedef struct
{
    uint8_t reqStatus;       // CBUS_IDLE, CBUS_PENDING
    uint8_t retryCount;
    uint8_t command;         // VS_CMD or RCP opcode
    uint8_t offsetData;      // Offset of register on CBUS or RCP data
    uint8_t length;          // Only applicable to write burst. ignored otherwise.
    union
    {
    uint8_t msgData[ 16 ];   // Pointer to message data area.
	unsigned char	*pdatabytes;			// pointer for write burst or read many bytes
    }payload_u;

} cbus_req_t;

#define	MHL_TX_EVENT_NONE				0x00	/* No event worth reporting.  */
#define	MHL_TX_EVENT_DISCONNECTION		0x01	/* MHL connection has been lost */
#define	MHL_TX_EVENT_CONNECTION			0x02	/* MHL connection has been established */
#define	MHL_TX_EVENT_RCP_READY			0x03	/* MHL connection is ready for RCP */
#define	MHL_TX_EVENT_RCP_RECEIVED		0x04	/* Received an RCP. Key Code in "eventParameter" */
#define	MHL_TX_EVENT_RCPK_RECEIVED		0x05	/* Received an RCPK message */
#define	MHL_TX_EVENT_RCPE_RECEIVED		0x06	/* Received an RCPE message .*/
#define	MHL_TX_EVENT_DCAP_CHG			0x07	/* Received DCAP_CHG interrupt */
#define	MHL_TX_EVENT_DSCR_CHG			0x08	/* Received DSCR_CHG interrupt */
#define	MHL_TX_EVENT_POW_BIT_CHG		0x09	/* Peer's power capability has changed */
#define	MHL_TX_EVENT_RGND_MHL			0x0A	/* RGND measurement has determine that the peer is an MHL device */

typedef enum
{
	MHL_TX_EVENT_STATUS_HANDLED = 0
	,MHL_TX_EVENT_STATUS_PASSTHROUGH
}MhlTxNotifyEventsStatus_e;


typedef enum
{
	SCRATCHPAD_FAIL= -4
	,SCRATCHPAD_BAD_PARAM = -3
	,SCRATCHPAD_NOT_SUPPORTED = -2
	,SCRATCHPAD_BUSY = -1
	,SCRATCHPAD_SUCCESS = 0
}ScratchPadStatus_e;

typedef struct
{
    uint8_t		pollIntervalMs;		// Remember what app set the polling frequency as.

	uint8_t		status_0;			// Received status from peer is stored here
	uint8_t		status_1;			// Received status from peer is stored here

    uint8_t     connectedReady;     // local MHL CONNECTED_RDY register value
    uint8_t     linkMode;           // local MHL LINK_MODE register value
    uint8_t     mhlHpdStatus;       // keep track of SET_HPD/CLR_HPD
    uint8_t     mhlRequestWritePending;

	bool		mhlConnectionEvent;
	uint8_t		mhlConnected;

    uint8_t     mhlHpdRSENflags;       // keep track of SET_HPD/CLR_HPD

	// mscMsgArrived == true when a MSC MSG arrives, false when it has been picked up
	bool		mscMsgArrived;
	uint8_t		mscMsgSubCommand;
	uint8_t		mscMsgData;

	// Remember FEATURE FLAG of the peer to reject app commands if unsupported
	uint8_t		mscFeatureFlag;

    uint8_t     cbusReferenceCount;  // keep track of CBUS requests
	// Remember last command, offset that was sent.
	// Mostly for READ_DEVCAP command and other non-MSC_MSG commands
	uint8_t		mscLastCommand;
	uint8_t		mscLastOffset;
	uint8_t		mscLastData;

	// Remember last MSC_MSG command (RCPE particularly)
	uint8_t		mscMsgLastCommand;
	uint8_t		mscMsgLastData;
	uint8_t		mscSaveRcpKeyCode;

#define SCRATCHPAD_SIZE 16
    //  support WRITE_BURST
    uint8_t     localScratchPad[SCRATCHPAD_SIZE];
    uint8_t     miscFlags;          // such as SCRATCHPAD_BUSY
//  uint8_t 	mscData[ 16 ]; 		// What we got back as message data

	uint8_t		ucDevCapCacheIndex;
	uint8_t		aucDevCapCache[16];

	uint8_t		rapFlags;		// CONTENT ON/OFF
	uint8_t		preferredClkMode;
#ifdef CONFIG_LG_MAGIC_MOTION_REMOCON
	uint8_t 	mscScratchpadData[16];
#endif
} mhlTx_config_t;

// bits for mhlHpdRSENflags:
typedef enum
{
     MHL_HPD            = 0x01
   , MHL_RSEN           = 0x02
}MhlHpdRSEN_e;

typedef enum
{
      FLAGS_SCRATCHPAD_BUSY         = 0x01
    , FLAGS_REQ_WRT_PENDING         = 0x02
    , FLAGS_WRITE_BURST_PENDING     = 0x04
    , FLAGS_RCP_READY               = 0x08
    , FLAGS_HAVE_DEV_CATEGORY       = 0x10
    , FLAGS_HAVE_DEV_FEATURE_FLAGS  = 0x20
    , FLAGS_SENT_DCAP_RDY           = 0x40
    , FLAGS_SENT_PATH_EN            = 0x80
}MiscFlags_e;
typedef enum
{
	RAP_CONTENT_ON = 0x01
}rapFlags_e;

#define DEVCAP_VAL_DEV_STATE       0
#define DEVCAP_VAL_MHL_VERSION     MHL_VERSION
#ifdef CONFIG_MACH_LGE
#define DEVCAP_VAL_DEV_CAT         MHL_DEV_CAT_SOURCE
#define DEVCAP_VAL_ADOPTER_ID_H    (uint8_t)(LGE_ADOPTER_ID >>   8)
#define DEVCAP_VAL_ADOPTER_ID_L    (uint8_t)(LGE_ADOPTER_ID & 0xFF)
#define DEVCAP_VAL_BANDWIDTH       0xF
#else
#define DEVCAP_VAL_DEV_CAT         (MHL_DEV_CAT_SOURCE | MHL_DEV_CATEGORY_POW_BIT)
#define DEVCAP_VAL_ADOPTER_ID_H    (uint8_t)(SILICON_IMAGE_ADOPTER_ID >>   8)
#define DEVCAP_VAL_ADOPTER_ID_L    (uint8_t)(SILICON_IMAGE_ADOPTER_ID & 0xFF)
#define DEVCAP_VAL_BANDWIDTH       0
#endif
#define DEVCAP_VAL_VID_LINK_MODE   MHL_DEV_VID_LINK_SUPPRGB444
#define DEVCAP_VAL_AUD_LINK_MODE   MHL_DEV_AUD_LINK_2CH
#define DEVCAP_VAL_VIDEO_TYPE      0
#define DEVCAP_VAL_LOG_DEV_MAP     MHL_LOGICAL_DEVICE_MAP
#define DEVCAP_VAL_FEATURE_FLAG    (MHL_FEATURE_RCP_SUPPORT | MHL_FEATURE_RAP_SUPPORT |MHL_FEATURE_SP_SUPPORT)
#define DEVCAP_VAL_DEVICE_ID_H     (uint8_t)(TRANSCODER_DEVICE_ID>>   8)
#define DEVCAP_VAL_DEVICE_ID_L     (uint8_t)(TRANSCODER_DEVICE_ID& 0xFF)
#define DEVCAP_VAL_SCRATCHPAD_SIZE MHL_SCRATCHPAD_SIZE
#define DEVCAP_VAL_INT_STAT_SIZE   MHL_INT_AND_STATUS_SIZE
#define DEVCAP_VAL_RESERVED        0

// Standard result codes are in the range of 0 - 4095
typedef enum _SiiResultCodes_t
{
    SII_SUCCESS      = 0,           // Success.
    SII_ERR_FAIL,                   // General failure.
    SII_ERR_INVALID_PARAMETER,      //
    SII_ERR_IN_USE,                 // Module already initialized.
    SII_ERR_NOT_AVAIL,              // Allocation of resources failed.
} SiiResultCodes_t;

typedef enum
{
    SiiSYSTEM_NONE      = 0,
    SiiSYSTEM_SINK,
    SiiSYSTEM_SWITCH,
    SiiSYSTEM_SOURCE,
    SiiSYSTEM_REPEATER,
} SiiSystemTypes_t;

#define YES                         1
#define NO                          0

#define MHL_PART_NAME "SiI-833x"	// WARNING! Must match I2C_BOARD_INFO() setting in board-omap3beagle.c
#define MHL_DRIVER_NAME "simg8334"
#define MHL_DRIVER_DESC "Sil-8334 MHL Tx Driver"
#define MHL_DEVICE_NAME "siI-8334"

#define MHL_DRIVER_MINOR_MAX   1
#define EVENT_POLL_INTERVAL_30_MS	30

/***** public type definitions ***********************************************/

#define MHL_STATE_FLAG_CONNECTED	0x01	// MHL connection is estabished
#define MHL_STATE_FLAG_RCP_READY	0x02	// connection ready for RCP requests
#define MHL_STATE_FLAG_RCP_SENT		0x04	// last RCP event was a key send
#define MHL_STATE_FLAG_RCP_RECEIVED	0x08	// last RCP event was a key code receive
#define MHL_STATE_FLAG_RCP_ACK		0x10	// last RCP key code sent was ACK'd
#define MHL_STATE_FLAG_RCP_NAK		0x20	// last RCP key code sent was NAK'd

typedef struct {
	struct task_struct	*pTaskStruct;
	struct device		*pDevice;			// pointer to the driver's device object
	uint8_t				flags;				// various state flags
	uint8_t				keyCode;			// last RCP key code sent or received.
	uint8_t				errCode;			// last RCP NAK error code received
	uint8_t				devCapOffset;		// last Device Capability register
											// offset requested
	uint8_t				pendingEvent;		// event data wait for retrieval
	uint8_t				pendingEventData;	// by user mode application

} MHL_DRIVER_CONTEXT_T, *PMHL_DRIVER_CONTEXT_T;


/***** global variables ********************************************/

extern MHL_DRIVER_CONTEXT_T gDriverContext;

/**
 * \addtogroup driver_public_api
 * @{
 */


/**
 * @struct mhlTxEventPacket_t
 *
 * @brief Contains information about a driver event
 *
 * This structure is used with the SII_MHL_GET_MHL_TX_EVENT IOCTL to retrieve
 * event information from the driver.
 */
typedef struct {
	uint8_t	event;				/**< Event code */
	uint8_t	eventParam;			/**< Event code specific additional event parameter */
} mhlTxEventPacket_t;


/**
 * @struct mhlTxReadDevCap_t
 *
 * @brief Structure used to read a Device Capabilities register
 *
 * This structure is used with the SII_MHL_GET_DEV_CAP_VALUE IOCTL to retrieve
 * the value of one of the Device Capability registers of the device attached
 * to the MHL transmitter.
 */
typedef struct {
	uint8_t regNum;				/**< Input specifying Dev Cap register to read */
	uint8_t	regValue;			/**< On return contains the value of Dev Cap register regNum */
} mhlTxReadDevCap_t;


/**
 * @struct mhlTxScratchPadAccess_t
 *
 * @brief Structure used to read or write Scratch Pad registers
 *
 * This structure is used with the SII_MHL_WRITE_SCRATCH_PAD and SII_MHL_READ_SCRATCH_PAD
 * IOCTLs to write or read respectively the scratch pad registers of the device
 * attached to the MHL transmitter.
  */
#define ADOPTER_ID_SIZE					2
#define MAX_SCRATCH_PAD_TRANSFER_SIZE	64
typedef struct {
	uint8_t	offset;				/**< Offset within the Scratch Pad register space to begin
									 writing to, or reading from */
	uint8_t length;				/**< Number of Scratch Pad registers to write or read */
	uint8_t	data[MAX_SCRATCH_PAD_TRANSFER_SIZE];	/**< Buffer containing write or read data */
} mhlTxScratchPadAccess_t;


/**
 * Event codes returned by IOCTL SII_MHL_GET_MHL_TX_EVENT
 *
 */
#define		MHL_TX_EVENT_NONE			0x00	/**< No event worth reporting */
#define		MHL_TX_EVENT_DISCONNECTION	0x01	/**< MHL connection has been lost */
#define		MHL_TX_EVENT_CONNECTION		0x02	/**< MHL connection has been established */
#define		MHL_TX_EVENT_RCP_READY		0x03	/**< MHL connection is ready for RCP */
#define		MHL_TX_EVENT_RCP_RECEIVED	0x04	/**< Received an RCP. Key Code in "eventParameter" */
#define		MHL_TX_EVENT_RCPK_RECEIVED	0x05	/**< Received an RCPK message */
#define		MHL_TX_EVENT_RCPE_RECEIVED	0x06	/**< Received an RCPE message */
#define		MHL_TX_EVENT_DCAP_CHG		0x07	/**< Received Device Capabilities change intr. */
#define		MHL_TX_EVENT_DSCR_CHG		0x08	/**< Received Scratch Pad register change intr. */

/**
 * Error status codes for RCPE messages
 */
#define	MHL_RCPE_STATUS_NO_ERROR				0x00	/**< No error.  (Not allowed in
															RCPE messages) */
#define	MHL_RCPE_STATUS_INEEFECTIVE_KEY_CODE	0x01	/**< Unsupported/unrecognized key code */
#define	MHL_RCPE_STATUS_BUSY					0x02	/**< Responder busy.  Initiator may
															 retry message */

/* Define type field for Silicon Image MHL I/O control codes */
#define IOC_SII_MHL_TYPE ('S')


/*****************************************************************************/
/**
 * @brief IOCTL to poll the driver for events
 *
 * An application needs to send this IOCTL periodically to poll the driver
 * for MHL connection related events.  Events reported by this IOCTL fall
 * generally into two categories, MHL physical link related (connection /
 * disconnection), or Remote Control Protocol (RCP).
 *
 * With regard to RCP events the MHL specification states that RCP events
 * that are not responded to within the TRCP_WAIT period (1000ms. max) may
 * be regarded as failed transmissions by the sender.  Therefore applications
 * should call this function at intervals less than TRCP_WAIT to avoid RCP
 * message timeouts.
 *
 * The parameter passed with this IOCTL is a pointer to a structure of
 * type mhlTxEventPacket_t.  Upon return the structure will contain the
 * information about the next event (if any) detected by the driver.
 *
 *****************************************************************************/
#define SII_MHL_GET_MHL_TX_EVENT \
    _IOR(IOC_SII_MHL_TYPE, 0x01, mhlTxEventPacket_t *)


/*****************************************************************************/
/**
 * @brief IOCTL to send a remote control code
 *
 * An application uses this IOCTL code to send an MHL defined remote control
 * code to the downstream display device.
 *
 * The parameter passed with this IOCTL is the remote control key code to be
 * sent to the downstream display device.
 *
 *****************************************************************************/
#define SII_MHL_RCP_SEND \
    _IOW(IOC_SII_MHL_TYPE, 0x02, uint8_t)


/*****************************************************************************/
/**
 * @brief IOCTL to send a remote control code acknowledgment
 *
 * An application uses this IOCTL code to send a positive acknowledgment for
 * an RCP code received from the downstream display device and reported to the
 * application via the SII_MHL_GET_MHL_TX_EVENT IOCTL.
 *
 * The parameter passed with this IOCTL is the remote control key code that
 * is being acknowledged.
 *
 *****************************************************************************/
#define SII_MHL_RCP_SEND_ACK \
    _IOW(IOC_SII_MHL_TYPE, 0x03, uint8_t)


/*****************************************************************************/
/**
 * @brief IOCTL to send a remote control code error acknowledgment
 *
 * An application uses this IOCTL code to send a negative acknowledgment for
 * an RCP code received from the downstream display device and reported to the
 * application via the SII_MHL_GET_MHL_TX_EVENT IOCTL.
 *
 * The parameter passed with this IOCTL is the remote control key code to be
 * negatively acknowledged.
 *
 *****************************************************************************/
#define SII_MHL_RCP_SEND_ERR_ACK \
    _IOW(IOC_SII_MHL_TYPE, 0x04, uint8_t)


/*****************************************************************************/
/**
 * @brief IOCTL to retrieve status of MHL connection
 *
 * An application can use this IOCTL to determine the current state of the
 * transmitter's MHL port.
 *
 * The parameter passed with this IOCTL is a pointer to a structure of type
 * mhlTxEventPacket_t.  Upon return the event field of this structure will
 * contain the most recent event code pertaining to the status of the MHL
 * connection.
 *
 *****************************************************************************/
#define SII_MHL_GET_MHL_CONNECTION_STATUS \
    _IOR(IOC_SII_MHL_TYPE, 0x05, mhlTxEventPacket_t *)


/*****************************************************************************/
/**
 * @brief IOCTL to retrieve the value of one of the Device Capability registers
 *
 * An application can use this IOCTL to read the value of one of the Device
 * Capability registers from the device attached to the MHL transmitter.
 *
 * The parameter passed with this IOCTL is a pointer to a structure of type
 * mhlTxReadDevCap_t.  The regNum field of this structure must be filled in by
 * the caller with the index of the Device Capabilities register to be read.
 * Upon return the regValue field of this structure will contain the most recent
 * value read from the specified Device Capabilities register.
 *
 * This IOCTL will return EINVAL if an MHL connection is not established at the
 * time of the call or if the index value passed is invalid.  The driver will
 * return EAGAIN if it is currently in a state where it cannot immediately satisfy
 * the request.  If EAGAIN is returned the caller should wait a short amount of
 * time and retry the request.
 *
 *****************************************************************************/
#define SII_MHL_GET_DEV_CAP_VALUE \
    _IOWR(IOC_SII_MHL_TYPE, 0x06, mhlTxReadDevCap_t *)


/*****************************************************************************/
/**
 * @brief IOCTL to write the Scratch Pad registers of an attached MHL device.
 *
 * An application can use this IOCTL to write up to 16 data bytes within the
 * scratch pad register space of an attached MHL device.
 *
 * The parameter passed with this IOCTL is a pointer to a structure of type
 * mhlTxScratchPadAccess_t.  The offset field of this structure must be filled
 * in by the caller with the starting offset within the Scratch Pad register
 * address space of the 1st register to write.  The length field specifies the
 * total number of bytes passed in the data field.  The data field contains the
 * data bytes to be written.  Per the MHL specification, the write data must be
 * prepended with the Adopter ID of the receiving device.  The value passed in
 * the length field must include the Adopter ID bytes.
 *
 * This IOCTL will return EINVAL if an MHL connection is not established at the
 * time of the call or if the offset or length fields contain out of range values.
 * The driver will return EAGAIN if it is currently in a state where it cannot
 * immediately satisfy the request.  If EAGAIN is returned the caller should wait
 * a short amount of time and retry the request.
 *
 *****************************************************************************/
#define SII_MHL_WRITE_SCRATCH_PAD \
    _IOWR(IOC_SII_MHL_TYPE, 0x07, mhlTxScratchPadAccess_t *)


/*****************************************************************************/
/**
 * @brief IOCTL to read the Scratch Pad registers of an attached MHL device.
 *
 * An application can use this IOCTL to read up to 16 data bytes within the
 * scratch pad register space of an attached MHL device.
 *
 * The parameter passed with this IOCTL is a pointer to a structure of type
 * mhlTxScratchPadAccess_t.  The offset field of this structure must be filled
 * in by the caller with the starting offset within the Scratch Pad register
 * address space of the 1st register to read.  The length field specifies the
 * total number of bytes to read starting at offset.  Upon successful completion
 * the data field will contain the data bytes read.
 *
 * This IOCTL will return EINVAL if an MHL connection is not established at the
 * time of the call or if the offset or length fields contain out of range values.
 * The driver will return EAGAIN if it is currently in a state where it cannot
 * immediately satisfy the request.  If EAGAIN is returned the caller should wait
 * a short amount of time and retry the request.
 *
 *****************************************************************************/
#define SII_MHL_READ_SCRATCH_PAD \
    _IOWR(IOC_SII_MHL_TYPE, 0x08, mhlTxScratchPadAccess_t *)

#endif
