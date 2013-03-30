/* G-USB: hansun.lee@lge.com
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

#ifndef __LINUX_HSIC_TTY_XPORT_H__
#define __LINUX_HSIC_TTY_XPORT_H__

enum transport_type {
	HSIC_TTY_XPORT_UNDEF,
	HSIC_TTY_XPORT_TTY,
	HSIC_TTY_XPORT_SDIO,
	HSIC_TTY_XPORT_SMD,
	HSIC_TTY_XPORT_BAM,
	HSIC_TTY_XPORT_BAM2BAM,
	HSIC_TTY_XPORT_HSIC,
	HSIC_TTY_XPORT_HSUART,
	HSIC_TTY_XPORT_NONE,
};

#define XPORT_STR_LEN	10

//static char *xport_to_str(enum transport_type t)
//{
//	switch (t) {
//	case HSIC_TTY_XPORT_TTY:
//		return "TTY";
//	case HSIC_TTY_XPORT_SDIO:
//		return "SDIO";
//	case HSIC_TTY_XPORT_SMD:
//		return "SMD";
//	case HSIC_TTY_XPORT_BAM:
//		return "BAM";
//	case HSIC_TTY_XPORT_BAM2BAM:
//		return "BAM2BAM";
//	case HSIC_TTY_XPORT_HSIC:
//		return "HSIC";
//	case HSIC_TTY_XPORT_HSUART:
//		return "HSUART";
//	case HSIC_TTY_XPORT_NONE:
//		return "NONE";
//	default:
//		return "UNDEFINED";
//	}
//}

//static enum transport_type str_to_xport(const char *name)
//{
//	if (!strncasecmp("TTY", name, XPORT_STR_LEN))
//		return HSIC_TTY_XPORT_TTY;
//	if (!strncasecmp("SDIO", name, XPORT_STR_LEN))
//		return HSIC_TTY_XPORT_SDIO;
//	if (!strncasecmp("SMD", name, XPORT_STR_LEN))
//		return HSIC_TTY_XPORT_SMD;
//	if (!strncasecmp("BAM", name, XPORT_STR_LEN))
//		return HSIC_TTY_XPORT_BAM;
//	if (!strncasecmp("BAM2BAM", name, XPORT_STR_LEN))
//		return HSIC_TTY_XPORT_BAM2BAM;
//	if (!strncasecmp("HSIC", name, XPORT_STR_LEN))
//		return HSIC_TTY_XPORT_HSIC;
//	if (!strncasecmp("HSUART", name, XPORT_STR_LEN))
//		return HSIC_TTY_XPORT_HSUART;
//	if (!strncasecmp("", name, XPORT_STR_LEN))
//		return HSIC_TTY_XPORT_NONE;
//
//	return HSIC_TTY_XPORT_UNDEF;
//}

enum tty_type {
	HSIC_TTY_SERIAL,
	HSIC_TTY_RMNET,
};

#define NUM_RMNET_HSIC_PORTS 1
#define NUM_DUN_HSIC_PORTS 1
#define NUM_PORTS (NUM_RMNET_HSIC_PORTS \
	+ NUM_DUN_HSIC_PORTS)

int hsic_tty_ctrl_connect(void *, int);
void hsic_tty_ctrl_disconnect(void *, int);
int hsic_tty_ctrl_setup(unsigned int, enum tty_type);
int hsic_tty_data_connect(void *, int);
void hsic_tty_data_disconnect(void *, int);
int hsic_tty_data_setup(unsigned int, enum tty_type);

#endif
