/*
 * AT42QT602240/ATMXT224 Touchscreen driver
 *
 * Copyright (C) 2010 Samsung Electronics Co.Ltd
 * Author: Joonyoung Shim <jy0922.shim@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef __LINUX_QT602240_TS_H
#define __LINUX_QT602240_TS_H

/* Orient */
#define NORMAL			0x0
#define DIAGONAL		0x1
#define HORIZONTAL_FLIP	0x2
#define ROTATED_90_COUNTER	0x3
#define VERTICAL_FLIP		0x4
#define ROTATED_90		0x5
#define ROTATED_180		0x6
#define DIAGONAL_COUNTER	0x7

/* Version */
#define VER_16		16
#define VER_32		32

/* Slave addresses */
#define APP_LOW		0x4a
#define APP_HIGH		0x4b
#define BOOT_LOW		0x24
#define BOOT_HIGH		0x25

/* Firmware */
#define FW_NAME		"qt602240.fw"

/* Registers */
#define FAMILY_ID		0x00
#define VARIANT_ID		0x01
#define VERSION		0x02
#define BUILD			0x03
#define MATRIX_X_SIZE		0x04
#define MATRIX_Y_SIZE		0x05
#define OBJECT_NUM		0x06
#define OBJECT_START		0x07

#define OBJECT_SIZE		6

/* Object types */
#define GEN_MESSAGE		5
#define GEN_COMMAND		6
#define GEN_POWER			7
#define GEN_ACQUIRE		8
#define TOUCH_MULTI		9
#define TOUCH_KEYARRAY		15
#define TOUCH_PROXIMITY	23
#define PROCI_GRIPFACE		20
#define PROCG_NOISE		22
#define PROCI_ONETOUCH		24
#define PROCI_TWOTOUCH		27
#define SPT_COMMSCONFIG	18	/* firmware ver 21 over */
#define SPT_GPIOPWM		19
#define SPT_SELFTEST		25
#define SPT_CTECONFIG		28
#define DEBUG_DIAGNOSTIC	37
#define SPT_USERDATA		38	/* firmware ver 21 over */

/* GEN_COMMAND field */
#define COMMAND_RESET		0
#define COMMAND_BACKUPNV	1
#define COMMAND_CALIBRATE	2
#define COMMAND_REPORTALL	3
#define COMMAND_DIAGNOSTIC	5

/* GEN_POWER field */
#define POWER_IDLEACQINT	0
#define POWER_ACTVACQINT	1
#define POWER_ACTV2IDLETO	2

/* GEN_ACQUIRE field */
#define ACQUIRE_CHRGTIME	0
#define ACQUIRE_TCHDRIFT	2
#define ACQUIRE_DRIFTST	3
#define ACQUIRE_TCHAUTOCAL	4
#define ACQUIRE_SYNC		5
#define ACQUIRE_ATCHCALST	6
#define ACQUIRE_ATCHCALSTHR	7

/* TOUCH_MULTI field */
#define TOUCH_CTRL		0
#define TOUCH_XORIGIN		1
#define TOUCH_YORIGIN		2
#define TOUCH_XSIZE		3
#define TOUCH_YSIZE		4
#define TOUCH_BLEN			6
#define TOUCH_TCHTHR		7
#define TOUCH_TCHDI		8
#define TOUCH_ORIENT		9
#define TOUCH_MOVHYSTI		11
#define TOUCH_MOVHYSTN		12
#define TOUCH_NUMTOUCH		14
#define TOUCH_MRGHYST		15
#define TOUCH_MRGTHR		16
#define TOUCH_AMPHYST		17
#define TOUCH_XRANGE_LSB	18
#define TOUCH_XRANGE_MSB	19
#define TOUCH_YRANGE_LSB	20
#define TOUCH_YRANGE_MSB	21
#define TOUCH_XLOCLIP		22
#define TOUCH_XHICLIP		23
#define TOUCH_YLOCLIP		24
#define TOUCH_YHICLIP		25
#define TOUCH_XEDGECTRL	26
#define TOUCH_XEDGEDIST	27
#define TOUCH_YEDGECTRL	28
#define TOUCH_YEDGEDIST	29
#define TOUCH_JUMPLIMIT	30	/* firmware ver 22 over */

/* PROCI_GRIPFACE field */
#define GRIPFACE_CTRL		0
#define GRIPFACE_XLOGRIP	1
#define GRIPFACE_XHIGRIP	2
#define GRIPFACE_YLOGRIP	3
#define GRIPFACE_YHIGRIP	4
#define GRIPFACE_MAXTCHS	5
#define GRIPFACE_SZTHR1	7
#define GRIPFACE_SZTHR2	8
#define GRIPFACE_SHPTHR1	9
#define GRIPFACE_SHPTHR2	10
#define GRIPFACE_SUPEXTTO	11

/* PROCI_NOISE field */
#define NOISE_CTRL		0
#define NOISE_OUTFLEN		1
#define NOISE_GCAFUL_LSB	3
#define NOISE_GCAFUL_MSB	4
#define NOISE_GCAFLL_LSB	5
#define NOISE_GCAFLL_MSB	6
#define NOISE_ACTVGCAFVALID	7
#define NOISE_NOISETHR		8
#define NOISE_FREQHOPSCALE	10
#define NOISE_FREQ0		11
#define NOISE_FREQ1		12
#define NOISE_FREQ2		13
#define NOISE_FREQ3		14
#define NOISE_FREQ4		15
#define NOISE_IDLEGCAFVALID	16

/* SPT_COMMSCONFIG */
#define COMMS_CTRL		0
#define COMMS_CMD		1

/* SPT_CTECONFIG field */
#define CTE_CTRL		0
#define CTE_CMD		1
#define CTE_MODE		2
#define CTE_IDLEGCAFDEPTH	3
#define CTE_ACTVGCAFDEPTH	4
#define CTE_VOLTAGE		5	/* firmware ver 21 over */

#define VOLTAGE_DEFAULT	2700000
#define VOLTAGE_STEP		10000

/* Define for GEN_COMMAND */
#define BOOT_VALUE		0xa5
#define BACKUP_VALUE		0x55
#define BACKUP_TIME		25	/* msec */
#define RESET_TIME		65	/* msec */

#define FWRESET_TIME		175	/* msec */

/* Command to unlock bootloader */
#define UNLOCK_CMD_MSB		0xaa
#define UNLOCK_CMD_LSB		0xdc

/* Bootloader mode status */
#define WAITING_BOOTLOAD_CMD	0xc0	/* valid 7 6 bit only */
#define WAITING_FRAME_DATA	0x80	/* valid 7 6 bit only */
#define FRAME_CRC_CHECK	0x02
#define FRAME_CRC_FAIL		0x03
#define FRAME_CRC_PASS		0x04
#define APP_CRC_FAIL		0x40	/* valid 7 8 bit only */
#define BOOT_STATUS_MASK	0x3f

/* Touch status */
#define SUPPRESS		(1 << 1)
#define AMP			(1 << 2)
#define VECTOR			(1 << 3)
#define MOVE			(1 << 4)
#define RELEASE		(1 << 5)
#define PRESS			(1 << 6)
#define DETECT			(1 << 7)

#define MAX_XC			0x1E0
#define MAX_YC			0x320
#define MAX_AREA		0x1E
#define MAX_WIDTH		0x1E

#define MAX_FINGER		10

enum Firm_Status_ID {
	NO_FIRM_UP = 0,
	UPDATE_FIRM_UP,
	SUCCESS_FIRM_UP,
	FAIL_FIRM_UP,
};

struct qt602240_platform_data {
    unsigned int x_line;
    unsigned int y_line;
    unsigned int x_size;
    unsigned int y_size;
    unsigned int blen;
    unsigned int threshold;
    unsigned int voltage;
    unsigned char orient;
    unsigned int i2c_int_gpio;
    unsigned int gpio_sda;
    unsigned int gpio_scl;
    int (*power)(int on);   /* Only valid in first array entry */
    int (*power_enable)(int en, bool log_en);
};

static const u8 init_vals_ver_16[] = {
    /* [SPT_USERDATA_T38 INSTANCE 0] */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* [GEN_POWERCONFIG_T7 INSTANCE 0] */
    0x40, 0xFF, 0x32,
	/* [GEN_ACQUISITIONCONFIG_T8 INSTANCE 0] */
    0x1E, 0x00, 0x05, 0x05, 0x00, 0x00, 0x09, 0x05, 0x00, 0x00,
    /* [TOUCH_MULTITOUCHSCREEN_T9 INSTANCE 0] */
    0x8B, 0x00, 0x00, 0x13, 0x0B, 0x00, 0x10, 0x46, 0x02, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x0A, 0x0A, 0x0A, 0x0A, 0x63, 0x03,
    0xDF, 0x01, 0x0E, 0xFC, 0x16, 0x1C, 0x88, 0x28, 0x94, 0x5A,
    0x0A, 0x0A, 0x00, 0x00, 0x02,
    /* [TOUCH_KEYARRAY_T15 INSTANCE 0] */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00,
    /* [SPT_COMMSCONFIG_T18 INSTANCE 0] */
    0x00, 0x00,
    /* [SPT_GPIOPWM_T19 INSTANCE 0] */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* [TOUCH_PROXIMITY_T23 INSTANCE 0] */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    /* [SPT_SELFTEST_T25 INSTANCE 0] */
    0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    /* [PROCI_GRIPSUPPRESSION_T40 INSTANCE 0] */
    0x00, 0x00, 0x00, 0x00, 0x00,
    /* [PROCI_TOUCHSUPPRESSION_T42 INSTANCE 0] */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00,
    /* [SPT_CTECONFIG_T46 INSTANCE 0] */
    0x04, 0x03, 0x20, 0x3F, 0x00, 0x00, 0x01, 0x00, 0x00,
    /* [PROCI_STYLUS_T47 INSTANCE 0] */
    0x00, 0x14, 0x3C, 0x05, 0x02, 0x2D, 0x28, 0xB4, 0x00, 0x28,
    /* [PROCG_NOISESUPPRESSION_T48 INSTANCE 0] */
    0x03, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x06, 0x06, 0x00, 0x00, 0x64, 0x04, 0x40,
    0x0A, 0x00, 0x14, 0x00, 0x00, 0x26, 0x00, 0x28, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
};

#endif /* __LINUX_QT602240_TS_H */
