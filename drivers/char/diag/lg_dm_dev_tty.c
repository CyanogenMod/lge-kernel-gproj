/*
 * DM TTY Driver for LGE DM router
 *
 * Seongmook Yim <seongmook.yim@lge.com>
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

#ifdef CONFIG_LGE_DM_DEV
#include <linux/list.h>
#include "diagchar_hdlc.h"
#include "diagmem.h"
#include "diagchar.h"
#include "diagfwd.h"
#include <linux/diagchar.h>
#ifdef CONFIG_DIAG_SDIO_PIPE
#include "diagfwd_sdio.h"
#endif
#ifdef CONFIG_DIAG_BRIDGE_CODE
#include "diagfwd_hsic.h"
#include "diagfwd_smux.h"
#endif
#ifdef CONFIG_DIAG_OVER_USB
#include <mach/usbdiag.h>
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

#include "lg_dm_dev_tty.h"

#define DM_DEV_TTY_MODULE_NAME		"DM_DEV"
#define MAX_DM_DEV_TTY_DRV		1

#define TRUE 1
#define FALSE 0


#define DM_DEV_TTY_TX_MAX_PACKET_SIZE   9000            /* max size = 9000B */
#define DM_DEV_TTY_RX_MAX_PACKET_SIZE   40000           /* max size = 40000B */

struct dm_dev_tty   *lge_dm_dev_tty;

/* modem_request_packet */
char  *dm_dev_modem_response;
int    dm_dev_modem_response_length;

/* modem_response_packet */
char  *dm_dev_modem_request;
int    dm_dev_modem_request_length;


/* TTY Driver status  */
enum {
    DM_DEV_TTY_INITIAL = 0,
    DM_DEV_TTY_REGISTERED =1,
    DM_DEV_TTY_OPEN = 2,
    DM_DEV_TTY_CLOSED = 3,
};


void lge_dm_dev_usb_fn(struct work_struct *work)
{
		usb_diag_write(driver->legacy_ch, driver->write_ptr_svc);
}


/* modem_request_command */
static int lge_dm_dev_tty_modem_request(const unsigned char *buf, int count)
{
#ifdef CONFIG_DIAG_BRIDGE_CODE
	int err = 0;
#endif /*CONFIG_DIAG_BRIDGE_CODE*/
	
//pr_info("[mook:lge_dm_dev_tty_modem_request");
//pr_info("[mook] write mask in modem_request = %d %d %d",buf[0],buf[1],buf[2]);	

	
#ifdef CONFIG_DIAG_SDIO_PIPE
  /*send masks to 9k*/
	if (driver->sdio_ch) {
	wait_event_interruptible(driver->wait_q,
	    (sdio_write_avail(driver->sdio_ch) >= count));
	if (driver->sdio_ch && (count > 0)) {
	  sdio_write(driver->sdio_ch,
	      (void *)buf, count);
		}
	}
#endif /*CONFIG_DIAG_SDIO_PIPE*/

#ifdef CONFIG_DIAG_BRIDGE_CODE
    /* send masks to 9k too */
          if (driver->hsic_ch && (count > 0)) {

            /* wait sending mask updates if HSIC ch not ready */
            if (driver->in_busy_hsic_write)
            	{
              		wait_event_interruptible(driver->wait_q,
                  (driver->in_busy_hsic_write != 1));
            	}

            driver->in_busy_hsic_write = 1;
            driver->in_busy_hsic_read_on_device = 0;

//			pr_info("[mook] lge_dm_dev_tty_modem_request / count = %d\n",count);	
            err = diag_bridge_write((void *)buf , count);

            if (err) {
              pr_err("diag: err sending mask to MDM: %d\n", err);
               /*
                * If the error is recoverable, then clear
                * the write flag, so we will resubmit a
                * write on the next frame.  Otherwise, don't
                * resubmit a write on the next frame.
                */
                if ((-ESHUTDOWN) != err)
                  driver->in_busy_hsic_write = 0;
            }

          }
#endif /*CONFIG_DIAG_BRIDGE_CODE*/
          else
          	{
//          pr_info(DM_DEV_TTY_MODULE_NAME ": %s: lge_dm_dev_tty_write"
//                    "error count = %d\n",
//                    __func__, count);
          	}
          return count;
}

static int lge_dm_dev_tty_modem_response(struct dm_dev_tty *lge_dm_dev_tty_drv,
			const unsigned char *buf, int count)
{
	int num_push = 0;
	int total_push = 0;
//	struct timeval time;
//	int start_flag_length;
//	int end_flag_length;
	int left = 0;

	if (count == 0)
	{
//		pr_info("diag:[mook]lge_dm_dev_tty_modem_response / count = %d\n",count);
		return 0;
	}
//	if (lge_dm_dev_tty_drv->
//	is_modem_open[modem_number] == FALSE)
//	return 0;
//	pr_info("diag:[mook]lge_dm_dev_tty_modem_response / count = %d\n",count);
//	pr_info("diag:[mook]lge_dm_dev_tty_modem_response / tty_str->count = %d\n",lge_dm_dev_tty_drv->tty_str->count);

	left = count;

	do {
//	pr_info("[mook]lge_dm_dev_tty_modem_response -3-\n");
	num_push = tty_insert_flip_string(lge_dm_dev_tty_drv->tty_str,
		buf+total_push, left);
	total_push += num_push;
	left -= num_push;
//	pr_info("[mook]lge_dm_dev_tty_modem_response -4-\n");
	tty_flip_buffer_push(lge_dm_dev_tty_drv->tty_str);
//	pr_info("[mook]lge_dm_dev_tty_modem_response -5-\n");
	} while (left != 0);

//	pr_info("diag:[mook]lge_dm_dev_tty_modem_response -6-\n");

	return total_push;
}

static int lge_dm_dev_tty_read_thread(void *data)
{
	int i = 0;
	struct dm_dev_tty *lge_dm_dev_tty_drv = NULL;

#ifdef CONFIG_DIAG_BRIDGE_CODE
	unsigned long spin_lock_flags;
	struct diag_write_device hsic_buf_tbl[NUM_HSIC_BUF_TBL_ENTRIES];
#endif
	
//	pr_info("diag:[mook]lge_dm_dev_tty_read_thread / set_logging = %d / logging_mode = %d \n",lge_dm_dev_tty->set_logging,driver->logging_mode);

	lge_dm_dev_tty_drv = lge_dm_dev_tty;

//	pr_info("diag:[mook]lge_dm_dev_tty_read_thread\n");

	while(1){

		wait_event_interruptible(lge_dm_dev_tty->waitq,
			lge_dm_dev_tty->set_logging);

		mutex_lock(&driver->diagchar_mutex);
		if ((lge_dm_dev_tty->set_logging == 1)
				&& (driver->logging_mode == DM_DEV_MODE)) {
//				pr_info("diag:[mook]lge_dm_dev_tty_read_thread -2-\n");
#ifdef CONFIG_DIAG_SDIO_PIPE
				/* copy 9K data over SDIO */
				if (driver->in_busy_sdio == 1) {
//					pr_info("diag:[mook]lge_dm_dev_tty_read_thread -3-\n");
					lge_dm_dev_tty_modem_response(
					lge_dm_dev_tty_drv,
					driver->buf_in_sdio,
					driver->write_ptr_mdm->length);

					driver->in_busy_sdio = 0;
				}
#endif /*CONFIG_DIAG_SDIO_PIPE*/

#ifdef CONFIG_DIAG_BRIDGE_CODE
//			pr_info("diag:[mook]lge_dm_dev_tty_read_thread -4-\n");

			spin_lock_irqsave(&driver->hsic_spinlock,
			spin_lock_flags);
			for (i = 0; i < driver->poolsize_hsic_write; i++) {
//				pr_info("diag:[mook]lge_dm_dev_tty_read_thread -5-\n");
				hsic_buf_tbl[i].buf =
					driver->hsic_buf_tbl[i].buf;
				driver->hsic_buf_tbl[i].buf = 0;
				hsic_buf_tbl[i].length =
						driver->hsic_buf_tbl[i].length;
				driver->hsic_buf_tbl[i].length = 0;
			}
			driver->num_hsic_buf_tbl_entries = 0;
			spin_unlock_irqrestore(&driver->hsic_spinlock,
						spin_lock_flags);
//			pr_info("diag:[mook]lge_dm_dev_tty_read_thread -6-\n");

			for (i = 0; i < driver->poolsize_hsic_write; i++) {
//				pr_info("diag:[mook]lge_dm_dev_tty_read_thread -7-\n");
				if (hsic_buf_tbl[i].length > 0) {
					lge_dm_dev_tty_modem_response(
					lge_dm_dev_tty_drv,
					(void *)hsic_buf_tbl[i].buf,
					hsic_buf_tbl[i].length);
//					pr_info("diag:[mook]lge_dm_dev_tty_read_thread -8-\n");

					/* Return the buffer to the pool */
					diagmem_free(driver,
					(unsigned char *)(hsic_buf_tbl[i].buf),
						POOL_TYPE_HSIC);
				}
			}
#endif

			lge_dm_dev_tty->set_logging = 0;
//pr_info("diag:[mook]lge_dm_dev_tty_read_thread -9\n");

#ifdef CONFIG_DIAG_SDIO_PIPE
//pr_info("diag:[mook]lge_dm_dev_tty_read_thread -10-\n");

				if (driver->sdio_ch)
					queue_work(driver->diag_sdio_wq,
						&(driver->diag_read_sdio_work));
#endif /* CONFIG_DIAG_SDIO_PIPE */
#ifdef CONFIG_DIAG_BRIDGE_CODE
//pr_info("diag:[mook]lge_dm_dev_tty_read_thread -11-\n");

				/* Read data from the hsic */
				if (driver->hsic_ch)
					queue_work(driver->diag_bridge_wq,
					&driver->diag_read_hsic_work);

#endif /*CONFIG_DIAG_BRIDGE_CODE*/

		}
//pr_info("diag:[mook]lge_dm_dev_tty_read_thread -12-\n");

		mutex_unlock(&driver->diagchar_mutex);

		if (kthread_should_stop())
			break;
		mdelay(1);
//pr_info("diag:[mook]lge_dm_dev_tty_read_thread -13-\n");

	}
	return 0;
}

static void lge_dm_dev_tty_unthrottle(struct tty_struct *tty)
{
  return;
}

static int lge_dm_dev_tty_write_room(struct tty_struct *tty)
{
    return DM_DEV_TTY_TX_MAX_PACKET_SIZE;
}

static int lge_dm_dev_tty_write(struct tty_struct *tty, const unsigned char *buf, int count)
{
  int result;
  struct dm_dev_tty *lge_dm_dev_tty_drv = NULL;

  lge_dm_dev_tty_drv = lge_dm_dev_tty;
  tty->driver_data = lge_dm_dev_tty_drv;
  lge_dm_dev_tty_drv -> tty_str = tty;


//  pr_info("[mook] write mask in tty_write = %d %d %d",buf[0],buf[1],buf[2]);	

  /* check the packet size  */
  if(count > DM_DEV_TTY_TX_MAX_PACKET_SIZE)
  {
    pr_info(DM_DEV_TTY_MODULE_NAME "[mook_tty]:%s:"
        "lge_dm_dev_tty_write error count = %d\n",__func__, count);
    return -EPERM;
  }
  result = lge_dm_dev_tty_modem_request(buf,count);
  
  return result;
}

static int lge_dm_dev_tty_open(struct tty_struct *tty, struct file *file)
{
  struct dm_dev_tty *lge_dm_dev_tty_drv = NULL;

  printk("[mook] tty_open!!!\n");

  if(!tty)
  {
    pr_err(DM_DEV_TTY_MODULE_NAME "[mook]:%s: NULL tty", __func__);
    return -ENODEV;
  }

  lge_dm_dev_tty_drv = lge_dm_dev_tty;

  if(!lge_dm_dev_tty_drv)
  {
    pr_err(DM_DEV_TTY_MODULE_NAME "[mook]:%s:"
        "NULL lge_dm_dev_tty_drv", __func__);
    return -ENODEV;
  }

  tty->driver_data = lge_dm_dev_tty_drv;
  lge_dm_dev_tty_drv->tty_str = tty;

  if(lge_dm_dev_tty_drv->tty_state == DM_DEV_TTY_OPEN)
  {
    pr_err(DM_DEV_TTY_MODULE_NAME "[mook]:%s:"
        "tty is already open", __func__);
    return -EBUSY;
  }

  /* support max = 64KB */
  set_bit(TTY_NO_WRITE_SPLIT, &lge_dm_dev_tty_drv->tty_str->flags);
  
  lge_dm_dev_tty_drv->tty_ts = kthread_run(lge_dm_dev_tty_read_thread, NULL,"lge_dm_dev_tty_thread");
//  pr_info("[mook]:%s: TTY device open / kthread_run \n",__func__);

  lge_dm_dev_tty_drv->tty_state = DM_DEV_TTY_OPEN;

  lge_dm_dev_tty_drv->set_logging = 0;

  lge_dm_dev_tty_drv->dm_dev_wq = create_singlethread_workqueue("dm_dev_wq");
  INIT_WORK(&(lge_dm_dev_tty_drv->dm_dev_usb_work), lge_dm_dev_usb_fn);

  return 0;
}

static void lge_dm_dev_tty_close(struct tty_struct *tty, struct file *file)
{
	//seongmook.yim-0225
	int i;
	//seongmook.yim-0225
	struct dm_dev_tty *lge_dm_dev_tty_drv = NULL;

	lge_dm_dev_tty_drv = lge_dm_dev_tty;
	tty->driver_data = lge_dm_dev_tty_drv;
	lge_dm_dev_tty_drv->tty_str = tty;

	clear_bit(TTY_NO_WRITE_SPLIT, &lge_dm_dev_tty_drv->tty_str->flags);

	if (!tty) {
		pr_err(DM_DEV_TTY_MODULE_NAME "[mook] : %s: NULL tty", __func__);
		return;
	}

	lge_dm_dev_tty_drv = tty->driver_data;

	if (!lge_dm_dev_tty_drv) {
		pr_err(DM_DEV_TTY_MODULE_NAME "[mook] : %s: NULL sdio_tty_drv", __func__);
		return;
	}

	if (lge_dm_dev_tty_drv->tty_state != DM_DEV_TTY_OPEN) {
		pr_err(DM_DEV_TTY_MODULE_NAME "[mook] : %s: DEV_TTY device was not opened\n",
			__func__);
		return;
	}

	lge_dm_dev_tty->set_logging = 1;
	wake_up_interruptible(&lge_dm_dev_tty_drv->waitq);

	kthread_stop(lge_dm_dev_tty_drv->tty_ts);

	lge_dm_dev_tty_drv->tty_state = DM_DEV_TTY_CLOSED;

	pr_info(DM_DEV_TTY_MODULE_NAME ": %s: DEV_TTY device closed\n", __func__);

	cancel_work_sync(&(lge_dm_dev_tty_drv->dm_dev_usb_work));
	destroy_workqueue(lge_dm_dev_tty_drv->dm_dev_wq);

//seongmook.yim-0225
	pr_info( "[mook]DM_DEV_TTY_MODEM_CLOSE in tty_close\n");
		lge_dm_dev_tty->set_logging = 0;
		
		/* change path to USB driver */
		mutex_lock(&driver->diagchar_mutex);
		driver->logging_mode = USB_MODE;
		mutex_unlock(&driver->diagchar_mutex);
		
		if (driver->usb_connected == 0)
			diagfwd_disconnect();
		else
			diagfwd_connect();

#ifdef CONFIG_DIAG_BRIDGE_CODE
		driver->num_hsic_buf_tbl_entries = 0;
		for (i = 0; i < driver->poolsize_hsic_write; i++) {
			if (driver->hsic_buf_tbl[i].buf) {
				/* Return the buffer to the pool */
				diagmem_free(driver, (unsigned char *)
					(driver->hsic_buf_tbl[i].buf),
					POOL_TYPE_HSIC);
				driver->hsic_buf_tbl[i].buf = 0;
			}
			driver->hsic_buf_tbl[i].length = 0;
		}

		diagfwd_cancel_hsic();
		diagfwd_connect_bridge(0);
#endif

		pr_info(DM_DEV_TTY_MODULE_NAME ": %s : lge_dm_dev_tty_ioctl "
			"DM_DEV_TTY_IOCTL_MODEM_CLOSE\n", __func__);
//seongmook.yim-0225
	return;
}

static int lge_dm_dev_tty_ioctl(struct tty_struct *tty, unsigned int cmd, unsigned long arg)
{
	int i;
	#ifdef CONFIG_DIAG_BRIDGE_CODE
	unsigned long spin_lock_flags;
	#endif
	struct dm_dev_tty *lge_dm_dev_tty_drv = NULL;
	
	lge_dm_dev_tty_drv = lge_dm_dev_tty;
	tty->driver_data = lge_dm_dev_tty_drv;
	lge_dm_dev_tty_drv->tty_str = tty;

	if (_IOC_TYPE(cmd) != DM_DEV_TTY_IOCTL_MAGIC)
	return -EINVAL;

	switch (cmd){

	case DM_DEV_TTY_MODEM_OPEN:
		pr_info(DM_DEV_TTY_MODULE_NAME "[mook] DM_DEV_TTY_MODEM_OPEN\n");
#ifdef CONFIG_DIAG_BRIDGE_CODE
		if ((driver->usb_connected == 1) && (driver->count_hsic_pool == N_MDM_WRITE)) {
			spin_lock_irqsave(&driver->hsic_spinlock, spin_lock_flags);
			driver->count_hsic_pool = 0;
			spin_unlock_irqrestore(&driver->hsic_spinlock, spin_lock_flags);
		}

		driver->num_hsic_buf_tbl_entries = 0;
		for (i = 0; i < driver->poolsize_hsic_write; i++) {
			if (driver->hsic_buf_tbl[i].buf) {
				/* Return the buffer to the pool */
				diagmem_free(driver, (unsigned char *)
					(driver->hsic_buf_tbl[i].buf),
					POOL_TYPE_HSIC);
				driver->hsic_buf_tbl[i].buf = 0;
			}
			driver->hsic_buf_tbl[i].length = 0;
		}

		diagfwd_disconnect_bridge(1);

		diagfwd_cancel_hsic();
		diagfwd_connect_bridge(0);
#endif

		/* change path to DM DEV */
		mutex_lock(&driver->diagchar_mutex);
		driver->logging_mode = DM_DEV_MODE;
		mutex_unlock(&driver->diagchar_mutex);

#ifdef CONFIG_DIAG_SDIO_PIPE
		driver->in_busy_sdio = 0;
		/* Poll SDIO channel to check for data */
		if (driver->sdio_ch)
			queue_work(driver->diag_sdio_wq,
				&(driver->diag_read_sdio_work));
#endif
#ifdef CONFIG_DIAG_BRIDGE_CODE
		/* Read data from the hsic */
		if (driver->hsic_ch)
			queue_work(driver->diag_bridge_wq,
			&driver->diag_read_hsic_work);
#endif

		pr_info(DM_DEV_TTY_MODULE_NAME ": %s : [mook] DM_DEV_TTY_IOCTL_MODEM_OPEN "
			"logging mode = %d\n", __func__,driver->logging_mode);

		break;
		
	case DM_DEV_TTY_MODEM_CLOSE:
		pr_info(DM_DEV_TTY_MODULE_NAME "[mook]DM_DEV_TTY_MODEM_CLOSE\n");
		lge_dm_dev_tty->set_logging = 0;
		
		/* change path to USB driver */
		mutex_lock(&driver->diagchar_mutex);
		driver->logging_mode = USB_MODE;
		mutex_unlock(&driver->diagchar_mutex);
		
		if (driver->usb_connected == 0)
			diagfwd_disconnect();
		else
			diagfwd_connect();

#ifdef CONFIG_DIAG_BRIDGE_CODE
		driver->num_hsic_buf_tbl_entries = 0;
		for (i = 0; i < driver->poolsize_hsic_write; i++) {
			if (driver->hsic_buf_tbl[i].buf) {
				/* Return the buffer to the pool */
				diagmem_free(driver, (unsigned char *)
					(driver->hsic_buf_tbl[i].buf),
					POOL_TYPE_HSIC);
				driver->hsic_buf_tbl[i].buf = 0;
			}
			driver->hsic_buf_tbl[i].length = 0;
		}

		diagfwd_cancel_hsic();
		diagfwd_connect_bridge(0);
#endif

		pr_info(DM_DEV_TTY_MODULE_NAME ": %s : lge_dm_dev_tty_ioctl "
			"DM_DEV_TTY_IOCTL_MODEM_CLOSE\n", __func__);

		break;
		
	case DM_DEV_TTY_ENABLE:

		/* change path to DM DEV */
		mutex_lock(&driver->diagchar_mutex);
		pr_info(DM_DEV_TTY_MODULE_NAME "[mook_tty] : %s, Logging mode Change to DM_DEV_MODE\n",__func__);
		driver->logging_mode = DM_DEV_MODE;
		mutex_unlock(&driver->diagchar_mutex);
		break;

	default:
		pr_info(DM_DEV_TTY_MODULE_NAME ": %s:"
		"lge_dm_tty_ioctl error\n", __func__);
		break;
	}
	
	return 0;
}
static const struct tty_operations lge_dm_dev_tty_ops = {
  .open = lge_dm_dev_tty_open,
  .close = lge_dm_dev_tty_close,
  .write = lge_dm_dev_tty_write,
  .write_room = lge_dm_dev_tty_write_room,
  .unthrottle = lge_dm_dev_tty_unthrottle,
  .ioctl = lge_dm_dev_tty_ioctl,
 };

static int __init lge_dm_dev_tty_init(void)
{
    int ret = 0;
    struct device *tty_dev;
    struct dm_dev_tty *lge_dm_dev_tty_drv;

    pr_info(DM_DEV_TTY_MODULE_NAME "[mook_tty] : %s\n",__func__);
    
    lge_dm_dev_tty_drv = kzalloc(sizeof(struct dm_dev_tty), GFP_KERNEL);
    if(lge_dm_dev_tty_drv == NULL)
    {
      pr_info(DM_DEV_TTY_MODULE_NAME "[mook_tty]:%s:" 
                                     "failed to allocate lge_dm_dev_tty", __func__);
      return 0;
    }

    lge_dm_dev_tty = lge_dm_dev_tty_drv;
    lge_dm_dev_tty_drv -> tty_drv = alloc_tty_driver(MAX_DM_DEV_TTY_DRV);
    if(!lge_dm_dev_tty->tty_drv)
    {
      pr_info(DM_DEV_TTY_MODULE_NAME "[mook_tty] :%s - tty_drv is NULL", __func__);
      kfree(lge_dm_dev_tty_drv);
      return 0;
    }

    lge_dm_dev_tty_drv->tty_drv->name = "lge_dm_dev_tty";
    lge_dm_dev_tty_drv->tty_drv->owner = THIS_MODULE;
    lge_dm_dev_tty_drv->tty_drv->driver_name = "lge_dm_dev_tty";
    /* uses dynamically assigned dev_t values */
    lge_dm_dev_tty_drv->tty_drv->type = TTY_DRIVER_TYPE_SERIAL;
    lge_dm_dev_tty_drv->tty_drv->subtype = SERIAL_TYPE_NORMAL;
    lge_dm_dev_tty_drv->tty_drv->flags = TTY_DRIVER_REAL_RAW
        | TTY_DRIVER_DYNAMIC_DEV
        | TTY_DRIVER_RESET_TERMIOS;
    /* initializing the tty driver */
    lge_dm_dev_tty_drv->tty_drv->init_termios = tty_std_termios;
    lge_dm_dev_tty_drv->tty_drv->init_termios.c_iflag = IGNBRK | IGNPAR;
    lge_dm_dev_tty_drv->tty_drv->init_termios.c_oflag = 0;
    lge_dm_dev_tty_drv->tty_drv->init_termios.c_cflag =
        B9600 | CS8 | CREAD | HUPCL | CLOCAL;
    lge_dm_dev_tty_drv->tty_drv->init_termios.c_lflag = 0;
    
    tty_set_operations(lge_dm_dev_tty_drv->tty_drv, &lge_dm_dev_tty_ops);
    ret = tty_register_driver(lge_dm_dev_tty_drv->tty_drv);

    if (ret) 
    {
        put_tty_driver(lge_dm_dev_tty_drv->tty_drv);
        pr_info(DM_DEV_TTY_MODULE_NAME "[mook_tty]: %s:"
        "tty_register_driver() ""failed\n",
         __func__);

        lge_dm_dev_tty_drv->tty_drv = NULL;
        kfree(lge_dm_dev_tty_drv);
        return 0;
    }

    tty_dev = tty_register_device(lge_dm_dev_tty_drv -> tty_drv, 0, NULL);

    if(IS_ERR(tty_dev))
    {
       pr_info(DM_DEV_TTY_MODULE_NAME "[mook_tty]: %s:"
       "tty_register_driver() ""failed\n",
        __func__);
       tty_unregister_driver(lge_dm_dev_tty_drv -> tty_drv);
       put_tty_driver(lge_dm_dev_tty_drv -> tty_drv);
       kfree(lge_dm_dev_tty_drv);
       return 0;
    }
    
    init_waitqueue_head(&lge_dm_dev_tty_drv->waitq);

    lge_dm_dev_tty_drv -> tty_state = DM_DEV_TTY_REGISTERED;
  
    return 0;
}

static void __exit lge_dm_dev_tty_exit(void)
{
    int ret = 0;
    struct dm_dev_tty *lge_dm_dev_tty_drv = NULL;

    lge_dm_dev_tty_drv = lge_dm_dev_tty;

    if(!lge_dm_dev_tty_drv)
    {
      pr_err(DM_DEV_TTY_MODULE_NAME ":%s:"
          "NULL lge_dm_dev_tty_dev", __func__);
      return;
    }

    if(lge_dm_dev_tty_drv->tty_state != DM_DEV_TTY_INITIAL)
    {
      tty_unregister_device(lge_dm_dev_tty_drv->tty_drv, 0);
      ret = tty_unregister_driver(lge_dm_dev_tty_drv -> tty_drv);

      if(ret)
      {
        pr_err(DM_DEV_TTY_MODULE_NAME ": %s:"
            "tty_unregister_driver() failed\n", __func__);
      }

      put_tty_driver(lge_dm_dev_tty_drv -> tty_drv);
      lge_dm_dev_tty_drv -> tty_state = DM_DEV_TTY_INITIAL;
      lge_dm_dev_tty_drv -> tty_drv = NULL;
    }

    pr_info(DM_DEV_TTY_MODULE_NAME "[mook_tty]:%s: Freeing dm_dev_tty structure", __func__);
    kfree(lge_dm_dev_tty_drv);

    return;
}


module_init(lge_dm_dev_tty_init);
module_exit(lge_dm_dev_tty_exit);

MODULE_DESCRIPTION("LGE DM DEV TTY");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Seongmook Yim <seongmook.yim@lge.com>");
#endif /*CONFIG_LGE_DM_DEV*/
