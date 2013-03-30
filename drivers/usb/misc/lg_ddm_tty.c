/*
 * DDM TTY Driver for DDM application
 *
 * Youngjin Park <jin.park@lge.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
*/

#ifdef CONFIG_USB_LGE_DDM_TTY
#include <linux/list.h>
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

#include "lg_ddm_tty.h"

#include <mach/ddm_bridge.h>

#define DDM_TTY_MODULE_NAME		"DDM_TTY"
#define MAX_DDM_TTY_DRV			1

#define TRUE 1
#define FALSE 0

/* TTY driver status */
enum {
	DDM_TTY_INITIAL 	= 0,
	DDM_TTY_REGISTERED 	= 1,
	DDM_TTY_OPEN 		= 2,
	DDM_TTY_CLOSED 		= 3,
};

struct ddm_tty *lge_ddm_tty;

#define DDM_TTY_TX_MAX_PACKET_SIZE		40000 	/*max size = 40000B */
#define DDM_TTY_RX_MAX_PACKET_SIZE		9000 	/* max size = 9000B */

static int lge_ddm_tty_read_thread(void *data)
{

	struct ddm_tty *lge_ddm_tty_drv = NULL;
	int num_push = 0;
	int left = 0;
	int total_push = 0;

	lge_ddm_tty_drv = lge_ddm_tty;

	while (1) {

		wait_event_interruptible(lge_ddm_tty->waitq,
			lge_ddm_tty->set_logging);

		if ((lge_ddm_tty->set_logging == 1) && 
			(lge_ddm_tty->read_length > 0)) {

#ifdef DDM_TTY_DEBUG
			pr_info(DDM_TTY_MODULE_NAME ": %s: lge_ddm_tty_read_thread "
				"read_length = %d\n",
					__func__, lge_ddm_tty->read_length);
#endif
				total_push = 0;
				left = lge_ddm_tty->read_length;
				
				do {
					num_push = tty_insert_flip_string(lge_ddm_tty_drv->tty_str,
						lge_ddm_tty->read_buf + total_push, left);
					total_push += num_push;
					left -= num_push;
					tty_flip_buffer_push(lge_ddm_tty_drv->tty_str);
				} while (left != 0);

			lge_ddm_tty->set_logging = 0;

			if (!ddm_drv->hsic_suspend)
				queue_work(ddm_drv->ddm_wq, &ddm_drv->read_w);

		}

		if (kthread_should_stop())
			break;
		mdelay(1);

	}

	return 0;

}

static void lge_ddm_tty_unthrottle(struct tty_struct *tty)
{
	return;
}

static int lge_ddm_tty_write_room(struct tty_struct *tty)
{
	return DDM_TTY_TX_MAX_PACKET_SIZE;
}

static int lge_ddm_tty_write(struct tty_struct *tty, const unsigned char *buf,
	int count)
{

	int result = 0;
	struct ddm_tty *lge_ddm_tty_drv = NULL;

	lge_ddm_tty_drv = lge_ddm_tty;
	tty->driver_data = lge_ddm_tty_drv;
	lge_ddm_tty_drv->tty_str = tty;

	/* check the packet size */
	if (count > DDM_TTY_TX_MAX_PACKET_SIZE) {
		pr_info(DDM_TTY_MODULE_NAME ": %s:"
		"lge_ddm_tty_write error count = %d\n",
			__func__, count);
		return -EPERM;
	}

	if (!ddm_drv->in_busy_hsic_write) {
		pr_info("called ddm_bridge_write\n");
		ddm_drv->in_busy_hsic_write = 1;
		result = ddm_bridge_write((char *)buf, count);
		if (result) {
			pr_err(" Error HSIC read from MDM , err : %d \n", result);
		}
	}

	return count;

}

static int lge_ddm_tty_open(struct tty_struct *tty, struct file *file)
{

	struct ddm_tty *lge_ddm_tty_drv = NULL;

	if (!tty) {
		pr_err(DDM_TTY_MODULE_NAME ": %s: NULL tty", __func__);
		return -ENODEV;
	}

	lge_ddm_tty_drv = lge_ddm_tty;

	if (!lge_ddm_tty_drv) {
		pr_err(DDM_TTY_MODULE_NAME ": %s:"
		"NULL lge_ddm_tty_drv", __func__);
		return -ENODEV;
	}

	tty->driver_data = lge_ddm_tty_drv;
	lge_ddm_tty_drv->tty_str = tty;

	if (lge_ddm_tty_drv->tty_state == DDM_TTY_OPEN) {
		pr_err(DDM_TTY_MODULE_NAME ": %s:"
		"tty is already open", __func__);
		return -EBUSY;
	}

	/* support max = 64KB */
	set_bit(TTY_NO_WRITE_SPLIT, &lge_ddm_tty_drv->tty_str->flags);

	lge_ddm_tty_drv->tty_ts = kthread_run(lge_ddm_tty_read_thread, NULL,
		"lge_ddm_tty_thread");

	lge_ddm_tty_drv->tty_state = DDM_TTY_OPEN;

	pr_info(DDM_TTY_MODULE_NAME ": %s: TTY device open\n", __func__);

	lge_ddm_tty_drv->set_logging = 0;

	return 0;

}

static void lge_ddm_tty_close(struct tty_struct *tty, struct file *file)
{

	struct ddm_tty *lge_ddm_tty_drv = NULL;

	lge_ddm_tty_drv = lge_ddm_tty;
	tty->driver_data = lge_ddm_tty_drv;
	lge_ddm_tty_drv->tty_str = tty;

	clear_bit(TTY_NO_WRITE_SPLIT, &lge_ddm_tty_drv->tty_str->flags);

	if (!tty) {
		pr_err(DDM_TTY_MODULE_NAME ": %s: NULL tty", __func__);
		return;
	}

	lge_ddm_tty_drv = tty->driver_data;

	if (!lge_ddm_tty_drv) {
		pr_err(DDM_TTY_MODULE_NAME ": %s: NULL sdio_tty_drv", __func__);
		return;
	}

	if (lge_ddm_tty_drv->tty_state != DDM_TTY_OPEN) {
		pr_err(DDM_TTY_MODULE_NAME ": %s: TTY device was not opened\n",
			__func__);
		return;
	}

	lge_ddm_tty_drv->set_logging = 1;
	wake_up_interruptible(&lge_ddm_tty_drv->waitq);

	kthread_stop(lge_ddm_tty_drv->tty_ts);

	lge_ddm_tty_drv->tty_state = DDM_TTY_CLOSED;

	pr_info(DDM_TTY_MODULE_NAME ": %s: TTY device closed\n", __func__);

	return;

}

static int lge_ddm_tty_ioctl(struct tty_struct *tty, unsigned int cmd,
	unsigned long arg)
{
	return 0;
}

static const struct tty_operations lge_ddm_tty_ops = {
	.open = lge_ddm_tty_open,
	.close = lge_ddm_tty_close,
	.write = lge_ddm_tty_write,
	.write_room = lge_ddm_tty_write_room,
	.unthrottle = lge_ddm_tty_unthrottle,
	.ioctl = lge_ddm_tty_ioctl,
};

static int __init lge_ddm_tty_init(void)
{

	int ret = 0;
	struct device *tty_dev;
	struct ddm_tty *lge_ddm_tty_drv;

	pr_info(DDM_TTY_MODULE_NAME ": %s\n", __func__);

	lge_ddm_tty_drv = kzalloc(sizeof(struct ddm_tty), GFP_KERNEL);
	if (lge_ddm_tty_drv == NULL) {
		pr_info(DDM_TTY_MODULE_NAME "%s:"
		"failed to allocate lge_ddm_tty", __func__);
		return 0;
	}

	lge_ddm_tty = lge_ddm_tty_drv;

	lge_ddm_tty_drv->tty_drv = alloc_tty_driver(MAX_DDM_TTY_DRV);

	if (!lge_ddm_tty_drv->tty_drv) {
		pr_info(DDM_TTY_MODULE_NAME ": %s - tty_drv is NULL", __func__);
		kfree(lge_ddm_tty_drv);
		return 0;
	}

	lge_ddm_tty_drv->tty_drv->name = "lge_ddm_tty";
	lge_ddm_tty_drv->tty_drv->owner = THIS_MODULE;
	lge_ddm_tty_drv->tty_drv->driver_name = "lge_ddm_tty";

	/* uses dynamically assigned dev_t values */
	lge_ddm_tty_drv->tty_drv->type = TTY_DRIVER_TYPE_SERIAL;
	lge_ddm_tty_drv->tty_drv->subtype = SERIAL_TYPE_NORMAL;
	lge_ddm_tty_drv->tty_drv->flags = TTY_DRIVER_REAL_RAW
		| TTY_DRIVER_DYNAMIC_DEV
		| TTY_DRIVER_RESET_TERMIOS;

	/* initializing the tty driver */
	lge_ddm_tty_drv->tty_drv->init_termios = tty_std_termios;
	lge_ddm_tty_drv->tty_drv->init_termios.c_iflag = IGNBRK | IGNPAR;
	lge_ddm_tty_drv->tty_drv->init_termios.c_oflag = 0;
	lge_ddm_tty_drv->tty_drv->init_termios.c_cflag =
		B9600 | CS8 | CREAD | HUPCL | CLOCAL;
	lge_ddm_tty_drv->tty_drv->init_termios.c_lflag = 0;

	tty_set_operations(lge_ddm_tty_drv->tty_drv, &lge_ddm_tty_ops);

	ret = tty_register_driver(lge_ddm_tty_drv->tty_drv);

	if (ret) {
		put_tty_driver(lge_ddm_tty_drv->tty_drv);
		pr_info(DDM_TTY_MODULE_NAME ": %s:"
		"tty_register_driver() ""failed\n",
			__func__);

		lge_ddm_tty_drv->tty_drv = NULL;
		kfree(lge_ddm_tty_drv);
		return 0;
	}

	tty_dev = tty_register_device(lge_ddm_tty_drv->tty_drv, 0, NULL);

	if (IS_ERR(tty_dev)) {
		pr_info(DDM_TTY_MODULE_NAME ": %s:"
		"tty_register_device() " "failed\n",
			__func__);
		tty_unregister_driver(lge_ddm_tty_drv->tty_drv);
		put_tty_driver(lge_ddm_tty_drv->tty_drv);
		kfree(lge_ddm_tty_drv);
		return 0;
	}

	init_waitqueue_head(&lge_ddm_tty_drv->waitq);

	lge_ddm_tty_drv->tty_state = DDM_TTY_REGISTERED;

	lge_ddm_tty_drv->read_buf = kzalloc(DDM_TTY_RX_MAX_PACKET_SIZE, GFP_KERNEL);
	if (lge_ddm_tty_drv->read_buf == NULL)
		pr_info(DDM_TTY_MODULE_NAME ": %s: read_buf ""failed\n",
			__func__);

	lge_ddm_tty_drv->read_length = 0;

	return 0;

}

static void __exit lge_ddm_tty_exit(void)
{
	int ret = 0;
	struct ddm_tty *lge_ddm_tty_drv = NULL;

	lge_ddm_tty_drv = lge_ddm_tty;

	if (!lge_ddm_tty_drv) {
		pr_err(DDM_TTY_MODULE_NAME ": %s:"
		"NULL lge_ddm_tty_drv", __func__);
		return;
	}

	if (lge_ddm_tty_drv->tty_state != DDM_TTY_INITIAL) {
		tty_unregister_device(lge_ddm_tty_drv->tty_drv, 0);

		ret = tty_unregister_driver(lge_ddm_tty_drv->tty_drv);

		if (ret) {
			pr_err(DDM_TTY_MODULE_NAME ": %s: "
			    "tty_unregister_driver() failed\n", __func__);
		}

		put_tty_driver(lge_ddm_tty_drv->tty_drv);
		lge_ddm_tty_drv->tty_state = DDM_TTY_INITIAL;
		lge_ddm_tty_drv->tty_drv = NULL;
	}

	pr_info(DDM_TTY_MODULE_NAME ": %s: Freeing ddm_tty structure", __func__);
	kfree(lge_ddm_tty_drv);

	return;

}

module_init(lge_ddm_tty_init);
module_exit(lge_ddm_tty_exit);

MODULE_DESCRIPTION("LGE DDM TTY");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Youngjin Park <jin.park@lge.com>");

#endif
