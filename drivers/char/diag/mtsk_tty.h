#ifndef MTSK_TTY_H_
#define MTSK_TTY_H_
/*
 * DIAG MTS for LGE MTS Kernel Driver
 *
 *  lg-msp TEAM <lg-msp@lge.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.    See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
#include <linux/list.h>
#include "stddef.h"
#include "diagchar_hdlc.h"
#include "diagmem.h"
#include "diagchar.h"
#include "diagfwd.h"
#include <linux/diagchar.h>
#ifdef CONFIG_DIAG_OVER_USB
#include <mach/usbdiag.h>
#endif
#ifdef CONFIG_DIAG_BRIDGE_CODE
#include "diagfwd_hsic.h"
#endif
#include <linux/kthread.h>
#include <linux/timer.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <asm/current.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/suspend.h>

#define MTS_TTY_MODULE_NAME		"MTS_TTY"
#define DIAG_MTS_RX_MAX_PACKET_SIZE	9000     /* max size = 9000B */
#define DIAG_MTS_TX_SIZE		8192
#define MAX_DIAG_MTS_DRV		1

/* Diag Packet */
#define DIAG_MSG_F			31
#define DIAG_EVENT_REPORT_F		96	/* 96 */
#define DIAG_EXT_MSG_F			0x79	/* 121 */
#define DIAG_QSR_EXT_MSG_TERSE_F	0x92	/* 146 */
#define DIAG_LOG_CONFIG_F		0x73	/* 115 */
#define DIAG_EXT_MSG_CONFIG_F		0x7D	/* 125 */
#define ASYNC_HDLC_ESC			0x7d

/* mts tty driver ioctl values */
#define MTSK_TTY_IOCTL_MAGIC	'S'
#define MTSK_TTY_MTS_START	_IOWR(MTSK_TTY_IOCTL_MAGIC, 0x01, int)
#define MTSK_TTY_MTS_STOP	_IOWR(MTSK_TTY_IOCTL_MAGIC, 0x02, int)
#define MTSK_TTY_MTS_READ	_IOWR(MTSK_TTY_IOCTL_MAGIC, 0x03, int)

struct mts_tty {
	wait_queue_head_t waitq;
	struct tty_driver *tty_drv;
	struct tty_struct *tty_struct;
	struct notifier_block pm_notify;
	int pm_notify_info;
	int run;
};

extern struct mts_tty *mtsk_tty;

int mtsk_tty_process(char *, int);
int mtsk_tty_send_mask(struct diag_request *diag_read_ptr);
#endif /* MTSK_TTY_H_ */

