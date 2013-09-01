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

#include "mtsk_tty.h"
#define HDLC_END 0x7E

struct mts_tty *mtsk_tty = NULL;

int mtsk_tty_process(char *buf, int left)
{
	int num_push = 0;
	int total_push = 0;

	struct mts_tty *mtsk_tty_drv = mtsk_tty;

	if (mtsk_tty_drv == NULL)  {
		return -1;
	}

	num_push = tty_insert_flip_string(mtsk_tty_drv->tty_struct,
			buf + total_push, left);
	total_push += num_push;
	left -= num_push;
	tty_flip_buffer_push(mtsk_tty_drv->tty_struct);

	return total_push;
}

static int mtsk_tty_open(struct tty_struct *tty, struct file *file)
{
	struct mts_tty *mtsk_tty_drv = NULL;

	if (!tty)
		return -ENODEV;

	mtsk_tty_drv = mtsk_tty;

	if (!mtsk_tty_drv)
		return -ENODEV;

	tty->driver_data = mtsk_tty_drv;
	mtsk_tty_drv->tty_struct = tty;

	set_bit(TTY_NO_WRITE_SPLIT, &mtsk_tty_drv->tty_struct->flags);

	diagfwd_connect_bridge(1);

	printk(KERN_INFO "mtsk_tty_open TTY device open %d,%d\n", 0, 0);
	return 0;
}

static void mtsk_tty_close(struct tty_struct *tty, struct file *file)
{
	struct mts_tty *mtsk_tty_drv = NULL;

	diagfwd_disconnect_bridge(1);

	if (!tty) {
		printk(KERN_INFO "mtsk_tty_close FAIL."
				 "tty is Null %d,%d\n", 0, 0);
		return;
	}

	mtsk_tty_drv = tty->driver_data;

	return;
}

static int mtsk_tty_ioctl(struct tty_struct *tty, unsigned int cmd,
			  unsigned long arg)
{
	int ret = 0;

	switch (cmd) {
	case MTSK_TTY_MTS_START:
		mtsk_tty->run = 1;
		printk("mtsk_tty->run: 1 (%s)\n", __func__);
		break;
	case MTSK_TTY_MTS_STOP:
		mtsk_tty->run = 0;
		wake_up_interruptible(&mtsk_tty->waitq);
		mtsk_tty->pm_notify_info = 2;
		printk("mtsk_tty->run: 0 (%s)\n", __func__);
		break;
	case MTSK_TTY_MTS_READ:
		wait_event_interruptible(mtsk_tty->waitq,
					mtsk_tty->pm_notify_info != 0);
		printk("MTSK_TTY_MTS_READ (%d)\n", mtsk_tty->pm_notify_info);

		ret = copy_to_user((void *)arg,
				(const void *)&mtsk_tty->pm_notify_info,
				sizeof(int));
		if (ret != 0)
			printk("err: MTSK_TTY_MTS_READ (%s)\n", __func__);
		mtsk_tty->pm_notify_info = 0;
		break;
	default:
		break;
	}
	return ret;
}

static void mtsk_tty_unthrottle(struct tty_struct *tty) {
	return;
}

static int mtsk_tty_write_room(struct tty_struct *tty) {
	return DIAG_MTS_TX_SIZE;
}

static int mtsk_tty_write(struct tty_struct *tty, const unsigned char *buf, int count)
{
	struct mts_tty *mtsk_tty_drv = NULL;

	mtsk_tty_drv = mtsk_tty;
	tty->driver_data = mtsk_tty_drv;
	mtsk_tty_drv->tty_struct = tty;

	/* check the packet size */
	if (count > DIAG_MTS_RX_MAX_PACKET_SIZE) {
		printk(KERN_INFO "mtsk_tty_write packet size  %d,%d\n",
				 count, DIAG_MTS_RX_MAX_PACKET_SIZE);
		return -EPERM;
	}

	driver->usb_read_mdm_ptr->actual = count - 4;

#ifdef DIAG_DEBUG
	print_hex_dump(KERN_DEBUG, "MASK DATA ", DUMP_PREFIX_OFFSET, 16, 1,
                             buf + 4, count - 4, 1);
#endif

	memcpy(driver->usb_buf_mdm_out, (char *)buf + 4,
	       driver->usb_read_mdm_ptr->actual);
	driver->usb_read_mdm_ptr->buf = driver->usb_buf_mdm_out;
	driver->usb_read_mdm_ptr->length = USB_MAX_OUT_BUF; //count - 4;

	mtsk_tty_send_mask(driver->usb_read_mdm_ptr);

        /* The write of the data to the HSIC bridge is not complete */
	while (driver->in_busy_hsic_write == 1) {
		msleep(25);
	}

	queue_work(driver->diag_bridge_wq, &driver->diag_read_mdm_work);

	return count;
}

static const struct tty_operations mtsk_tty_ops = {
	.open = mtsk_tty_open,
	.close = mtsk_tty_close,
	.write = mtsk_tty_write,
	.write_room = mtsk_tty_write_room,
	.unthrottle = mtsk_tty_unthrottle,
	.ioctl = mtsk_tty_ioctl,
};

static int mts_pm_notify(struct notifier_block *b, unsigned long event, void *p)
{
	wake_up_interruptible(&mtsk_tty->waitq);

	switch (event) {
        case PM_SUSPEND_PREPARE:
		mtsk_tty->pm_notify_info = 3;
		printk("mts_pm_notify: PM_SUSPEND_PREPARE\n");
                break;
        case PM_POST_SUSPEND:
		mtsk_tty->pm_notify_info = 4;
		printk("mts_pm_notify: PM_POST_SUSPEND\n");
                break;
	default:
		break;
        }

        return 0;
}

static int __init mtsk_tty_init(void)
{
	int ret = 0;
	struct device *tty_dev =  NULL;
	struct mts_tty *mtsk_tty_drv = NULL;

	pr_info(MTS_TTY_MODULE_NAME ": %s\n", __func__);
	mtsk_tty_drv = kzalloc(sizeof(struct mts_tty), GFP_KERNEL);

	if (mtsk_tty_drv == NULL) {
		printk(KERN_INFO "lge_diag_mts_init FAIL %d - %d\n", 0, 0);
		return 0;
	}

	mtsk_tty = mtsk_tty_drv;
	mtsk_tty_drv->tty_drv = alloc_tty_driver(MAX_DIAG_MTS_DRV);

	if (!mtsk_tty_drv->tty_drv) {
		printk(KERN_INFO "lge_diag_mts_init init FAIL %d - %d\n", 1, 0);
		kfree(mtsk_tty_drv);
		return 0;
	}

	mtsk_tty_drv->tty_drv->name = "mtsk_tty";
	mtsk_tty_drv->tty_drv->owner = THIS_MODULE;
	mtsk_tty_drv->tty_drv->driver_name = "mtsk_tty";

	/* uses dynamically assigned dev_t values */
	mtsk_tty_drv->tty_drv->type = TTY_DRIVER_TYPE_SERIAL;
	mtsk_tty_drv->tty_drv->subtype = SERIAL_TYPE_NORMAL;
	mtsk_tty_drv->tty_drv->flags = TTY_DRIVER_REAL_RAW |
				       TTY_DRIVER_DYNAMIC_DEV |
				       TTY_DRIVER_RESET_TERMIOS;

	/* initializing the mts driver */
	mtsk_tty_drv->tty_drv->init_termios = tty_std_termios;
	mtsk_tty_drv->tty_drv->init_termios.c_iflag = IGNBRK | IGNPAR;
	mtsk_tty_drv->tty_drv->init_termios.c_oflag = 0;
	mtsk_tty_drv->tty_drv->init_termios.c_cflag = B9600 |
						      CS8 |
						      CREAD |
						      HUPCL |
						      CLOCAL;
	mtsk_tty_drv->tty_drv->init_termios.c_lflag = 0;
	tty_set_operations(mtsk_tty_drv->tty_drv, &mtsk_tty_ops);
	ret = tty_register_driver(mtsk_tty_drv->tty_drv);

	if (ret) {
		put_tty_driver(mtsk_tty_drv->tty_drv);
		mtsk_tty_drv->tty_drv = NULL;
		kfree(mtsk_tty_drv);
		return 0;
	}

	tty_dev = tty_register_device(mtsk_tty_drv->tty_drv, 0, NULL);

	if (IS_ERR(tty_dev)) {
		tty_unregister_driver(mtsk_tty_drv->tty_drv);
		put_tty_driver(mtsk_tty_drv->tty_drv);
		kfree(mtsk_tty_drv);
		return 0;
	}
	printk(KERN_INFO "mtsk_tty_init  SUCESS MTS_TTY_REGISTERED \n");

	mtsk_tty->pm_notify.notifier_call = mts_pm_notify;
	register_pm_notifier(&mtsk_tty->pm_notify);
	init_waitqueue_head(&mtsk_tty->waitq);

	mtsk_tty->run = 0;
	mtsk_tty->pm_notify_info = 0;

	return 0;
}

static void __exit mtsk_tty_exit(void)
{
	int ret = 0;
	struct mts_tty *mtsk_tty_drv = NULL;

	mtsk_tty_drv = mtsk_tty;

	if (!mtsk_tty_drv) {
		pr_err(MTS_TTY_MODULE_NAME ": %s:" "NULL mtsk_tty_drv",
					   __func__);
		return;
	}
	mdelay(20);
	tty_unregister_device(mtsk_tty_drv->tty_drv, 0);
	ret = tty_unregister_driver(mtsk_tty_drv->tty_drv);
	put_tty_driver(mtsk_tty_drv->tty_drv);
	mtsk_tty_drv->tty_drv = NULL;
	kfree(mtsk_tty_drv);
	mtsk_tty = NULL;

	unregister_pm_notifier(&mtsk_tty->pm_notify);

	printk(KERN_INFO "mtsk_tty_exit  SUCESS %d - %d\n", 0, 0);
	return;
}

module_init(mtsk_tty_init);
module_exit(mtsk_tty_exit);

MODULE_DESCRIPTION("LGE MTS TTY");
MODULE_LICENSE("GPL");
MODULE_AUTHOR(" lg-msp TEAM <lg-msp@lge.com>");

