/*

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 as
   published by the Free Software Foundation.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
   for more details.


   Copyright (C) 2006-2007 - Motorola
   Copyright (c) 2008-2010, Code Aurora Forum. All rights reserved.

   Date         Author           Comment
   -----------  --------------   --------------------------------
   2006-Apr-28	Motorola	 The kernel module for running the Bluetooth(R)
				 Sleep-Mode Protocol from the Host side
   2006-Sep-08  Motorola         Added workqueue for handling sleep work.
   2007-Jan-24  Motorola         Added mbm_handle_ioi() call to ISR.

*/

#define DEBUG


#include <linux/module.h>	/* kernel module definitions */
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/notifier.h>
#include <linux/proc_fs.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>

#include <linux/irq.h>
#include <linux/param.h>
#include <linux/bitops.h>
#include <linux/termios.h>
#include <linux/wakelock.h>
#include <mach/gpio.h>
#include <mach/msm_serial_hs.h>

#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h> /* event notifications */
#include "hci_uart.h"

#include <linux/gpio.h>

//BEGIN: 0019639 chanha.park@lge.com 2012-06-16
//ADD: 0019639: [F200][BT] Support Bluetooth low power mode
#ifdef CONFIG_LGE_BLUESLEEP
#ifndef FEATURE_USE_BTLA
// Attention : The current polarity value is 0.
#define FEATURE_USE_BTLA
#endif/*FEATURE_USE_BTLA*/

#ifdef FEATURE_USE_BTLA
#define BT_PORT_NUM	0
#endif/*FEATURE_USE_BTLA*/

#define BTA_NOT_USE_ROOT_PERM

#ifdef BTA_NOT_USE_ROOT_PERM
#define AID_BLUETOOTH       1002
#endif  //BTA_NOT_USE_ROOT_PERM
#endif/*CONFIG_LGE_BLUESLEEP*/
//END: 0019639 chanha.park@lge.com 2012-06-16


#define BT_SLEEP_DBG
#ifndef BT_SLEEP_DBG
#define BT_DBG(fmt, arg...)
#endif
/*
 * Defines
 */

#define VERSION		"1.1"
#define PROC_DIR	"bluetooth/sleep"

struct bluesleep_info {
	unsigned host_wake;
	unsigned ext_wake;
	unsigned host_wake_irq;
	struct uart_port *uport;
//BEGIN: 0019639 chanha.park@lge.com 2012-06-16
//ADD: 0019639: [F200][BT] Support Bluetooth low power mode
#ifdef FEATURE_USE_BTLA
	struct wake_lock wake_lock;
#endif/*FEATURE_USE_BTLA*/
//END: 0019639 chanha.park@lge.com 2012-06-16
};

/* work function */
static void bluesleep_sleep_work(struct work_struct *work);

/* work queue */
DECLARE_DELAYED_WORK(sleep_workqueue, bluesleep_sleep_work);

/* Macros for handling sleep work */
#define bluesleep_rx_busy()     schedule_delayed_work(&sleep_workqueue, 0)
#define bluesleep_tx_busy()     schedule_delayed_work(&sleep_workqueue, 0)
#define bluesleep_rx_idle()     schedule_delayed_work(&sleep_workqueue, 0)
#define bluesleep_tx_idle()     schedule_delayed_work(&sleep_workqueue, 0)

#ifdef FEATURE_USE_BTLA
//BEGIN: 0019639 chanha.park@lge.com 2012-06-16
//MOD: 0019639: [F200][BT] Support Bluetooth low power mode
// Fixed power consumtion problem when connected with Samsung stereo headset.
/* 5 second timeout */
#define TX_TIMER_INTERVAL	5 // 1 -> 5
#define TX_TIMER_SLEEP_INTERVAL	TX_TIMER_INTERVAL//Add
// END : 0005259 chanha.park@lge.com 2010-03-21
#else
#define TX_TIMER_INTERVAL	1
#endif //FEATURE_USE_BTLA
/* state variable names and bit positions */
#define BT_PROTO	0x01
#define BT_TXDATA	0x02
#define BT_ASLEEP	0x04

/* global pointer to a single hci device. */
static struct hci_dev *bluesleep_hdev;

static struct bluesleep_info *bsi;

/* module usage */
static atomic_t open_count = ATOMIC_INIT(1);

//BEGIN: 0019639 chanha.park@lge.com 2012-06-16
//DEL: 0019639: [F200][BT] Support Bluetooth low power mode
/*
 * Local function prototypes
 */
#ifndef FEATURE_USE_BTLA
static int bluesleep_hci_event(struct notifier_block *this,
			    unsigned long event, void *data);
#endif/*FEATURE_USE_BTLA*/
//END: 0019639 chanha.park@lge.com 2012-06-16

/*
 * Global variables
 */

/** Global state flags */
static unsigned long flags;

/** Tasklet to respond to change in hostwake line */
static struct tasklet_struct hostwake_task;

/** Transmission timer */
static struct timer_list tx_timer;

/** Lock for state transitions */
static spinlock_t rw_lock;

//BEGIN: 0019639 chanha.park@lge.com 2012-06-16
//DEL: 0019639: [F200][BT] Support Bluetooth low power mode
#ifndef FEATURE_USE_BTLA
/** Notifier block for HCI events */
struct notifier_block hci_event_nblock = {
	.notifier_call = bluesleep_hci_event,
};

#endif/*FEATURE_USE_BTLA*/
//END: 0019639 chanha.park@lge.com 2012-06-16

struct proc_dir_entry *bluetooth_dir, *sleep_dir;

/*
 * Local functions
 */

static void hsuart_power(int on)
{
	if (on) {
		BT_DBG("hsuart_power : 1");
		msm_hs_request_clock_on(bsi->uport);
		msm_hs_set_mctrl(bsi->uport, TIOCM_RTS);
	} else {
		BT_DBG("hsuart_power : 0");
		msm_hs_set_mctrl(bsi->uport, 0);
		msm_hs_request_clock_off(bsi->uport);
	}
}


/**
 * @return 1 if the Host can go to sleep, 0 otherwise.
 */
static inline int bluesleep_can_sleep(void)
{
	/* check if MSM_WAKE_BT_GPIO and BT_WAKE_MSM_GPIO are both deasserted */
	return gpio_get_value(bsi->ext_wake) &&
		gpio_get_value(bsi->host_wake) &&
		(bsi->uport != NULL);
}

void bluesleep_sleep_wakeup(void)
{
	if (test_bit(BT_ASLEEP, &flags)) {
		BT_DBG("waking up...");
//BEGIN: 0019639 chanha.park@lge.com 2012-06-16
//MOD: 0019639: [F200][BT] Support Bluetooth low power mode
#ifdef FEATURE_USE_BTLA
		wake_lock(&bsi->wake_lock);
#else/*FEATURE_USE_BTLA*/
		/* Start the timer */
		mod_timer(&tx_timer, jiffies + (TX_TIMER_INTERVAL * HZ));
		gpio_set_value(bsi->ext_wake, 0);
#endif/*FEATURE_USE_BTLA*/
//END: 0019639 chanha.park@lge.com 2012-06-16

		clear_bit(BT_ASLEEP, &flags);
		/*Activating UART */
		hsuart_power(1);
	}
//WORKAROUND
#ifdef CONFIG_LGE_BLUESLEEP
	else
	{
		BT_DBG("Workaround for uart runtime suspend : Need to UART On");

		if(bsi->uport != NULL && msm_hs_get_bt_uport_clock_state(bsi->uport) == CLOCK_REQUEST_AVAILABLE)
		{
			BT_DBG("UART Clock On");
			hsuart_power(1);
		}
	}
#endif
}

/**
 * @brief@  main sleep work handling function which update the flags
 * and activate and deactivate UART ,check FIFO.
 */
static void bluesleep_sleep_work(struct work_struct *work)
{
	BT_DBG("+++ bluesleep_sleep_work");

	if (bluesleep_can_sleep()) {
		/* already asleep, this is an error case */
		BT_DBG("bluesleep_can_sleep is true");
		if (test_bit(BT_ASLEEP, &flags)) {
			BT_DBG("already asleep");
			return;
		}

		if (msm_hs_tx_empty(bsi->uport)) {
			BT_DBG("going to sleep...");
			set_bit(BT_ASLEEP, &flags);
			/*Deactivating UART */
			hsuart_power(0);

//BEGIN: 0019639 chanha.park@lge.com 2012-06-16
//ADD: 0019639: [F200][BT] Support Bluetooth low power mode
#ifdef FEATURE_USE_BTLA
			wake_lock_timeout(&bsi->wake_lock, HZ / 2);
#endif/*FEATURE_USE_BTLA*/
//END: 0019639 chanha.park@lge.com 2012-06-16
		}
//BEGIN: 0019639 chanha.park@lge.com 2012-06-16
//DEL: 0019639: [F200][BT] Support Bluetooth low power mode
#ifndef FEATURE_USE_BTLA
		else {
			//BT_DBG("Do not anything modtimer...");
		  	mod_timer(&tx_timer, jiffies + (TX_TIMER_INTERVAL * HZ));
			return;
		}
#endif/*FEATURE_USE_BTLA*/
//END: 0019639 chanha.park@lge.com 2012-06-16
	} else {
		BT_DBG("Call BT Wake Up");
		bluesleep_sleep_wakeup();
	}

	BT_DBG("--- bluesleep_sleep_work");
}

/**
 * A tasklet function that runs in tasklet context and reads the value
 * of the HOST_WAKE GPIO pin and further defer the work.
 * @param data Not used.
 */
static void bluesleep_hostwake_task(unsigned long data)
{
	BT_DBG("hostwake line change");

	spin_lock(&rw_lock);

//BEGIN: 0019639 chanha.park@lge.com 2012-06-16
//MOD: 0019639: [F200][BT] Support Bluetooth low power mode
#ifdef FEATURE_USE_BTLA
	if(gpio_get_value(bsi->host_wake) == 0)
	{
		BT_DBG("hostwake GPIO Low");
		// Do not need to check GPIO
		bluesleep_rx_busy();
		//Fixed power consumtion problem when connected with Samsung stereo headset.
		mod_timer(&tx_timer, jiffies + (TX_TIMER_INTERVAL * HZ));//CHPARK_TEST
	}
	else
	{
		BT_DBG("hostwake GPIO High");
	}
#else/*FEATURE_USE_BTLA*/
	if (gpio_get_value(bsi->host_wake))
		bluesleep_rx_busy();
	else
		bluesleep_rx_idle();
#endif/*defined(CONFIG_LGE_BRCM_H4_LPM_SUPPORT_PATCH)*/
//END: 0019639 chanha.park@lge.com 2012-06-16

	spin_unlock(&rw_lock);
}

//BEGIN: 0019639 chanha.park@lge.com 2012-06-16
//DEL: 0019639: [F200][BT] Support Bluetooth low power mode
#ifndef FEATURE_USE_BTLA
/**
 * Handles proper timer action when outgoing data is delivered to the
 * HCI line discipline. Sets BT_TXDATA.
 */
static void bluesleep_outgoing_data(void)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&rw_lock, irq_flags);

	/* log data passing by */
	set_bit(BT_TXDATA, &flags);

	/* if the tx side is sleeping... */
	if (gpio_get_value(bsi->ext_wake)) {

		BT_DBG("tx was sleeping");
		bluesleep_sleep_wakeup();
	}

	spin_unlock_irqrestore(&rw_lock, irq_flags);
}

/**
 * Handles HCI device events.
 * @param this Not used.
 * @param event The event that occurred.
 * @param data The HCI device associated with the event.
 * @return <code>NOTIFY_DONE</code>.
 */
static int bluesleep_hci_event(struct notifier_block *this,
				unsigned long event, void *data)
{
	struct hci_dev *hdev = (struct hci_dev *) data;
	struct hci_uart *hu;
	struct uart_state *state;

	if (!hdev)
		return NOTIFY_DONE;

	switch (event) {
	case HCI_DEV_REG:
		if (!bluesleep_hdev) {
			bluesleep_hdev = hdev;
			hu  = (struct hci_uart *) hdev->driver_data;
			state = (struct uart_state *) hu->tty->driver_data;
			bsi->uport = state->uart_port;
		}
		break;
	case HCI_DEV_UNREG:
		bluesleep_hdev = NULL;
		bsi->uport = NULL;
		break;
	case HCI_DEV_WRITE:
		bluesleep_outgoing_data();
		break;
	}

	return NOTIFY_DONE;
}
#endif/*FEATURE_USE_BTLA*/
//END: 0019639 chanha.park@lge.com 2012-06-16

/**
 * Handles transmission timer expiration.
 * @param data Not used.
 */
static void bluesleep_tx_timer_expire(unsigned long data)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&rw_lock, irq_flags);

	BT_DBG("Tx timer expired");

//BEGIN: 0019639 chanha.park@lge.com 2012-06-16
//MOD: 0019639: [F200][BT] Support Bluetooth low power mode
#ifdef FEATURE_USE_BTLA
	/* already asleep, this is an error case */
	if (test_bit(BT_ASLEEP, &flags)) {
		//BT_DBG("already asleep");
		spin_unlock_irqrestore(&rw_lock, irq_flags);
		return;
	}

	bluesleep_tx_idle();
#else/*FEATURE_USE_BTLA*/
	/* were we silent during the last timeout? */
	if (!test_bit(BT_TXDATA, &flags)) {
		BT_DBG("Tx has been idle");
		gpio_set_value(bsi->ext_wake, 1);
		bluesleep_tx_idle();
	} else {
		BT_DBG("Tx data during last period");
		mod_timer(&tx_timer, jiffies + (TX_TIMER_INTERVAL*HZ));
	}

	/* clear the incoming data flag */
	clear_bit(BT_TXDATA, &flags);
#endif/*FEATURE_USE_BTLA*/
//END: 0019639 chanha.park@lge.com 2012-06-16

	spin_unlock_irqrestore(&rw_lock, irq_flags);
}

/**
 * Schedules a tasklet to run when receiving an interrupt on the
 * <code>HOST_WAKE</code> GPIO pin.
 * @param irq Not used.
 * @param dev_id Not used.
 */
static irqreturn_t bluesleep_hostwake_isr(int irq, void *dev_id)
{
//BEGIN: 0019639 chanha.park@lge.com 2012-09-09
//MOD: 0019639: [F200][BT] Support Bluetooth low power mode
#ifdef FEATURE_USE_BTLA
	/* schedule a tasklet to handle the change in the host wake line */
	int wake, host_wake;
	wake = gpio_get_value(bsi->ext_wake);
	host_wake = gpio_get_value(bsi->host_wake);
	printk(KERN_DEBUG "bluesleep_hostwake_isr %d, %d", wake, host_wake);

	irq_set_irq_type(irq, host_wake ? IRQF_TRIGGER_LOW : IRQF_TRIGGER_HIGH);

	if(host_wake == 0)
	{
		BT_DBG("bluesleep_hostwake_isr : Registration Tasklet");
		tasklet_schedule(&hostwake_task);
	}
#else/*FEATURE_USE_BTLA*/
	tasklet_schedule(&hostwake_task);
#endif/*FEATURE_USE_BTLA*/
//END: 0019639 chanha.park@lge.com 2012-09-09

	return IRQ_HANDLED;
}

/**
 * Starts the Sleep-Mode Protocol on the Host.
 * @return On success, 0. On error, -1, and <code>errno</code> is set
 * appropriately.
 */
static int bluesleep_start(void)
{
	int retval;
	unsigned long irq_flags;

	BT_DBG("bluesleep_start");

	spin_lock_irqsave(&rw_lock, irq_flags);

	if (test_bit(BT_PROTO, &flags)) {
		spin_unlock_irqrestore(&rw_lock, irq_flags);
		return 0;
	}

	spin_unlock_irqrestore(&rw_lock, irq_flags);

	if (!atomic_dec_and_test(&open_count)) {
		atomic_inc(&open_count);
		return -EBUSY;
	}

//BEGIN: 0019639 chanha.park@lge.com 2012-06-16
//DEL: 0019639: [F200][BT] Support Bluetooth low power mode
#ifndef FEATURE_USE_BTLA
	/* start the timer */

	mod_timer(&tx_timer, jiffies + (TX_TIMER_INTERVAL*HZ));
	/* assert BT_WAKE */
	gpio_set_value(bsi->ext_wake, 0);
#endif/*FEATURE_USE_BTLA*/
//END: 0019639 chanha.park@lge.com 2012-06-16


//BEGIN: 0019639 chanha.park@lge.com 2012-06-16
//ADD: 0019639: [F200][BT] Support Bluetooth low power mode
#ifdef FEATURE_USE_BTLA
	BT_DBG("bluesleep_start");
	hsuart_power(1);
#endif/*FEATURE_USE_BTLA*/
//END: 0019639 chanha.park@lge.com 2012-06-16

//BEGIN: 0019639 chanha.park@lge.com 2012-09-09
//MOD: 0019639: [F200][BT] Support Bluetooth low power mode
#ifdef FEATURE_USE_BTLA
	retval = request_irq(bsi->host_wake_irq, bluesleep_hostwake_isr,
				IRQF_DISABLED | IRQF_TRIGGER_LOW,
				"bluetooth hostwake", NULL);
#else/*FEATURE_USE_BTLA*/
	retval = request_irq(bsi->host_wake_irq, bluesleep_hostwake_isr,
				IRQF_DISABLED | IRQF_TRIGGER_FALLING,
				"bluetooth hostwake", NULL);
#endif/*FEATURE_USE_BTLA*/
//END: 0019639 chanha.park@lge.com 2012-09-09

	if (retval  < 0) {
		BT_ERR("Couldn't acquire BT_HOST_WAKE IRQ");
		goto fail;
	}

	retval = enable_irq_wake(bsi->host_wake_irq);
	if (retval < 0) {
		BT_ERR("Couldn't enable BT_HOST_WAKE as wakeup interrupt");
		free_irq(bsi->host_wake_irq, NULL);
		goto fail;
	}

	set_bit(BT_PROTO, &flags);
//BEGIN: 0019639 chanha.park@lge.com 2012-06-16
//ADD: 0019639: [F200][BT] Support Bluetooth low power mode
#ifdef FEATURE_USE_BTLA
	wake_lock(&bsi->wake_lock);
#endif/*FEATURE_USE_BTLA*/
//END: 0019639 chanha.park@lge.com 2012-06-16
	return 0;
fail:
//BEGIN: 0019639 chanha.park@lge.com 2012-06-16
//DEL: 0019639: [F200][BT] Support Bluetooth low power mode
#ifndef FEATURE_USE_BTLA
	del_timer(&tx_timer);
#endif/*FEATURE_USE_BTLA*/
//END: 0019639 chanha.park@lge.com 2012-06-16
	atomic_inc(&open_count);

	return retval;
}

/**
 * Stops the Sleep-Mode Protocol on the Host.
 */
static void bluesleep_stop(void)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&rw_lock, irq_flags);

	if (!test_bit(BT_PROTO, &flags)) {
		spin_unlock_irqrestore(&rw_lock, irq_flags);
		return;
	}

	BT_DBG("bluesleep_stop");


//BEGIN: 0019639 chanha.park@lge.com 2012-06-16
//MOD: 0019639: [F200][BT] Support Bluetooth low power mode
#ifndef FEATURE_USE_BTLA
	/* assert BT_WAKE */
	gpio_set_value(bsi->ext_wake, 0);
	del_timer(&tx_timer);
#endif/*FEATURE_USE_BTLA*/
//END: 0019639 chanha.park@lge.com 2012-06-16

	clear_bit(BT_PROTO, &flags);

	if (test_bit(BT_ASLEEP, &flags)) {
		clear_bit(BT_ASLEEP, &flags);
//BEGIN: 0019639 chanha.park@lge.com 2012-06-16
//ADD: 0019639: [F200][BT] Support Bluetooth low power mode
#ifndef FEATURE_USE_BTLA
		hsuart_power(1);
#endif/*FEATURE_USE_BTLA*/
//END: 0019639 chanha.park@lge.com 2012-06-16
	}
//BEGIN: 0019639 chanha.park@lge.com 2012-06-16
//ADD: 0019639: [F200][BT] Support Bluetooth low power mode
#ifdef FEATURE_USE_BTLA
	else
	{
		//set_bit(BT_ASLEEP, &flags);
		hsuart_power(0);
	}
#endif/*FEATURE_USE_BTLA*/
//END: 0019639 chanha.park@lge.com 2012-06-16

	atomic_inc(&open_count);

	spin_unlock_irqrestore(&rw_lock, irq_flags);
	if (disable_irq_wake(bsi->host_wake_irq))
		BT_ERR("Couldn't disable hostwake IRQ wakeup mode\n");
	free_irq(bsi->host_wake_irq, NULL);
//BEGIN: 0019639 chanha.park@lge.com 2012-06-16
//ADD: 0019639: [F200][BT] Support Bluetooth low power mode
#ifdef FEATURE_USE_BTLA
	wake_lock_timeout(&bsi->wake_lock, HZ / 2);
#endif/*FEATURE_USE_BTLA*/
//END: 0019639 chanha.park@lge.com 2012-06-16
}
/**
 * Read the <code>BT_WAKE</code> GPIO pin value via the proc interface.
 * When this function returns, <code>page</code> will contain a 1 if the
 * pin is high, 0 otherwise.
 * @param page Buffer for writing data.
 * @param start Not used.
 * @param offset Not used.
 * @param count Not used.
 * @param eof Whether or not there is more data to be read.
 * @param data Not used.
 * @return The number of bytes written.
 */
static int bluepower_read_proc_btwake(char *page, char **start, off_t offset,
					int count, int *eof, void *data)
{
	*eof = 1;
	return sprintf(page, "btwake:%u\n", gpio_get_value(bsi->ext_wake));
}

/**
 * Write the <code>BT_WAKE</code> GPIO pin value via the proc interface.
 * @param file Not used.
 * @param buffer The buffer to read from.
 * @param count The number of bytes to be written.
 * @param data Not used.
 * @return On success, the number of bytes written. On error, -1, and
 * <code>errno</code> is set appropriately.
 */
static int bluepower_write_proc_btwake(struct file *file, const char *buffer,
					unsigned long count, void *data)
{
	char *buf;

	if (count < 1)
		return -EINVAL;

	buf = kmalloc(count, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	if (copy_from_user(buf, buffer, count)) {
		kfree(buf);
		return -EFAULT;
	}

	if (buf[0] == '0') {
		BT_DBG("BT WAKE Set to Wake");

		gpio_set_value(bsi->ext_wake, 0);
//BEGIN: 0019639 chanha.park@lge.com 2012-06-16
//ADD: 0019639: [F200][BT] Support Bluetooth low power mode
#ifdef FEATURE_USE_BTLA
		//set_bit(BT_TXDATA, &flags);
		bluesleep_sleep_wakeup();
#endif/*FEATURE_USE_BTLA*/
//END: 0019639 chanha.park@lge.com 2012-06-16
	} else if (buf[0] == '1') {
		BT_DBG("BT WAKE Set to Sleep");
		gpio_set_value(bsi->ext_wake, 1);
//BEGIN: 0019639 chanha.park@lge.com 2012-06-16
//ADD: 0019639: [F200][BT] Support Bluetooth low power mode
#ifdef FEATURE_USE_BTLA
		bluesleep_tx_idle();
#endif/*FEATURE_USE_BTLA*/
//END: 0019639 chanha.park@lge.com 2012-06-16
	} else {
		kfree(buf);
		return -EINVAL;
	}

	kfree(buf);
	return count;
}

/**
 * Read the <code>BT_HOST_WAKE</code> GPIO pin value via the proc interface.
 * When this function returns, <code>page</code> will contain a 1 if the pin
 * is high, 0 otherwise.
 * @param page Buffer for writing data.
 * @param start Not used.
 * @param offset Not used.
 * @param count Not used.
 * @param eof Whether or not there is more data to be read.
 * @param data Not used.
 * @return The number of bytes written.
 */
static int bluepower_read_proc_hostwake(char *page, char **start, off_t offset,
					int count, int *eof, void *data)
{
	*eof = 1;
	return sprintf(page, "hostwake: %u \n", gpio_get_value(bsi->host_wake));
}


/**
 * Read the low-power status of the Host via the proc interface.
 * When this function returns, <code>page</code> contains a 1 if the Host
 * is asleep, 0 otherwise.
 * @param page Buffer for writing data.
 * @param start Not used.
 * @param offset Not used.
 * @param count Not used.
 * @param eof Whether or not there is more data to be read.
 * @param data Not used.
 * @return The number of bytes written.
 */
static int bluesleep_read_proc_asleep(char *page, char **start, off_t offset,
					int count, int *eof, void *data)
{
	unsigned int asleep;

	asleep = test_bit(BT_ASLEEP, &flags) ? 1 : 0;
	*eof = 1;
	return sprintf(page, "asleep: %u\n", asleep);
}

/**
 * Read the low-power protocol being used by the Host via the proc interface.
 * When this function returns, <code>page</code> will contain a 1 if the Host
 * is using the Sleep Mode Protocol, 0 otherwise.
 * @param page Buffer for writing data.
 * @param start Not used.
 * @param offset Not used.
 * @param count Not used.
 * @param eof Whether or not there is more data to be read.
 * @param data Not used.
 * @return The number of bytes written.
 */
static int bluesleep_read_proc_proto(char *page, char **start, off_t offset,
					int count, int *eof, void *data)
{
	unsigned int proto;

	proto = test_bit(BT_PROTO, &flags) ? 1 : 0;
	*eof = 1;
	return sprintf(page, "proto: %u\n", proto);
}

/**
 * Modify the low-power protocol used by the Host via the proc interface.
 * @param file Not used.
 * @param buffer The buffer to read from.
 * @param count The number of bytes to be written.
 * @param data Not used.
 * @return On success, the number of bytes written. On error, -1, and
 * <code>errno</code> is set appropriately.
 */
static int bluesleep_write_proc_proto(struct file *file, const char *buffer,
					unsigned long count, void *data)
{
	char proto;

	if (count < 1)
		return -EINVAL;

	if (copy_from_user(&proto, buffer, 1))
		return -EFAULT;

	if (proto == '0')
		bluesleep_stop();
	else
		bluesleep_start();

	/* claim that we wrote everything */
	return count;
}

static int __init bluesleep_probe(struct platform_device *pdev)
{
	int ret;
	struct resource *res;

	bsi = kzalloc(sizeof(struct bluesleep_info), GFP_KERNEL);
	if (!bsi)
		return -ENOMEM;

	res = platform_get_resource_byname(pdev, IORESOURCE_IO,
				"gpio_host_wake");
	if (!res) {
		BT_ERR("couldn't find host_wake gpio\n");
		ret = -ENODEV;
		goto free_bsi;
	}
	bsi->host_wake = res->start;

	ret = gpio_request(bsi->host_wake, "bt_host_wake");
	if (ret)
		goto free_bsi;
	ret = gpio_direction_input(bsi->host_wake);
	if (ret)
		goto free_bt_host_wake;

	res = platform_get_resource_byname(pdev, IORESOURCE_IO,
				"gpio_ext_wake");
	if (!res) {
		BT_ERR("couldn't find ext_wake gpio\n");
		ret = -ENODEV;
		goto free_bt_host_wake;
	}
	bsi->ext_wake = res->start;

	ret = gpio_request(bsi->ext_wake, "bt_ext_wake");
	if (ret)
		goto free_bt_host_wake;
	/* assert bt wake */
	ret = gpio_direction_output(bsi->ext_wake, 0);
	if (ret)
		goto free_bt_ext_wake;

	bsi->host_wake_irq = platform_get_irq_byname(pdev, "host_wake");
	if (bsi->host_wake_irq < 0) {
		BT_ERR("couldn't find host_wake irq\n");
		ret = -ENODEV;
		goto free_bt_ext_wake;
	}

//BEGIN: 0019639 chanha.park@lge.com 2012-06-16
//ADD: 0019639: [F200][BT] Support Bluetooth low power mode
#ifdef CONFIG_LGE_BLUESLEEP
	bsi->uport= msm_hs_get_bt_uport(BT_PORT_NUM);
#endif/*defined(CONFIG_LGE_BLUESLEEP)*/
//END: 0019639 chanha.park@lge.com 2012-06-16

//BEGIN: 0019639 chanha.park@lge.com 2012-06-16
//ADD: 0019639: [F200][BT] Support Bluetooth low power mode
#ifdef FEATURE_USE_BTLA
	wake_lock_init(&bsi->wake_lock, WAKE_LOCK_SUSPEND, "bluesleep");
#endif/*FEATURE_USE_BTLA*/
//END: 0019639 chanha.park@lge.com 2012-06-16

	return 0;

free_bt_ext_wake:
	gpio_free(bsi->ext_wake);
free_bt_host_wake:
	gpio_free(bsi->host_wake);
free_bsi:
	kfree(bsi);
	return ret;
}

static int bluesleep_remove(struct platform_device *pdev)
{
	/* assert bt wake */
	gpio_set_value(bsi->ext_wake, 0);
	if (test_bit(BT_PROTO, &flags)) {
		if (disable_irq_wake(bsi->host_wake_irq))
			BT_ERR("Couldn't disable hostwake IRQ wakeup mode \n");
		free_irq(bsi->host_wake_irq, NULL);
		del_timer(&tx_timer);
		if (test_bit(BT_ASLEEP, &flags))
			hsuart_power(1);
	}

	gpio_free(bsi->host_wake);
	gpio_free(bsi->ext_wake);
//BEGIN: 0019639 chanha.park@lge.com 2012-06-16
//ADD: 0019639: [F200][BT] Support Bluetooth low power mode
#ifdef FEATURE_USE_BTLA
	wake_lock_destroy(&bsi->wake_lock);
#endif/*FEATURE_USE_BTLA*/
//END: 0019639 chanha.park@lge.com 2012-06-16
	kfree(bsi);
	return 0;
}

static struct platform_driver bluesleep_driver = {
	.remove = bluesleep_remove,
	.driver = {
		.name = "bluesleep",
		.owner = THIS_MODULE,
	},
};
/**
 * Initializes the module.
 * @return On success, 0. On error, -1, and <code>errno</code> is set
 * appropriately.
 */
static int __init bluesleep_init(void)
{
	int retval;
	struct proc_dir_entry *ent;

	BT_INFO("MSM Sleep Mode Driver Ver %s", VERSION);

	retval = platform_driver_probe(&bluesleep_driver, bluesleep_probe);
	if (retval)
		return retval;

	bluesleep_hdev = NULL;

	bluetooth_dir = proc_mkdir("bluetooth", NULL);
	if (bluetooth_dir == NULL) {
		BT_ERR("Unable to create /proc/bluetooth directory");
		return -ENOMEM;
	}

	sleep_dir = proc_mkdir("sleep", bluetooth_dir);
	if (sleep_dir == NULL) {
		BT_ERR("Unable to create /proc/%s directory", PROC_DIR);
		return -ENOMEM;
	}

	/* Creating read/write "btwake" entry */
	ent = create_proc_entry("btwake", 0, sleep_dir);
	if (ent == NULL) {
		BT_ERR("Unable to create /proc/%s/btwake entry", PROC_DIR);
		retval = -ENOMEM;
		goto fail;
	}
	ent->read_proc = bluepower_read_proc_btwake;
	ent->write_proc = bluepower_write_proc_btwake;
//BEGIN: 0019639 chanha.park@lge.com 2012-06-16
//ADD: 0019639: [F200][BT] Support Bluetooth low power mode
#ifdef BTA_NOT_USE_ROOT_PERM
    ent->uid = AID_BLUETOOTH;
    ent->gid = AID_BLUETOOTH;
#endif  //BTA_NOT_USE_ROOT_PERM
//END: 0019639 chanha.park@lge.com 2012-06-16

	/* read only proc entries */
	if (create_proc_read_entry("hostwake", 0, sleep_dir,
				bluepower_read_proc_hostwake, NULL) == NULL) {
		BT_ERR("Unable to create /proc/%s/hostwake entry", PROC_DIR);
		retval = -ENOMEM;
		goto fail;
	}

	/* read/write proc entries */
	ent = create_proc_entry("proto", 0, sleep_dir);
	if (ent == NULL) {
		BT_ERR("Unable to create /proc/%s/proto entry", PROC_DIR);
		retval = -ENOMEM;
		goto fail;
	}
	ent->read_proc = bluesleep_read_proc_proto;
	ent->write_proc = bluesleep_write_proc_proto;
//BEGIN: 0019639 chanha.park@lge.com 2012-06-16
//ADD: 0019639: [F200][BT] Support Bluetooth low power mode
#ifdef BTA_NOT_USE_ROOT_PERM
    ent->uid = AID_BLUETOOTH;
    ent->gid = AID_BLUETOOTH;
#endif  //BTA_NOT_USE_ROOT_PERM
//END: 0019639 chanha.park@lge.com 2012-06-16

	/* read only proc entries */
	if (create_proc_read_entry("asleep", 0,
			sleep_dir, bluesleep_read_proc_asleep, NULL) == NULL) {
		BT_ERR("Unable to create /proc/%s/asleep entry", PROC_DIR);
		retval = -ENOMEM;
		goto fail;
	}

	flags = 0; /* clear all status bits */

	/* Initialize spinlock. */
	spin_lock_init(&rw_lock);

	/* Initialize timer */
	init_timer(&tx_timer);
	tx_timer.function = bluesleep_tx_timer_expire;
	tx_timer.data = 0;

	/* initialize host wake tasklet */
	tasklet_init(&hostwake_task, bluesleep_hostwake_task, 0);

//BEGIN: 0019639 chanha.park@lge.com 2012-06-16
//ADD: 0019639: [F200][BT] Support Bluetooth low power mode
#ifndef FEATURE_USE_BTLA
	hci_register_notifier(&hci_event_nblock);
#endif/*FEATURE_USE_BTLA*/
//END: 0019639 chanha.park@lge.com 2012-06-16

	return 0;

fail:
	remove_proc_entry("asleep", sleep_dir);
	remove_proc_entry("proto", sleep_dir);
	remove_proc_entry("hostwake", sleep_dir);
	remove_proc_entry("btwake", sleep_dir);
	remove_proc_entry("sleep", bluetooth_dir);
	remove_proc_entry("bluetooth", 0);
	return retval;
}

/**
 * Cleans up the module.
 */
static void __exit bluesleep_exit(void)
{
//BEGIN: 0019639 chanha.park@lge.com 2012-06-16
//ADD: 0019639: [F200][BT] Support Bluetooth low power mode
#ifndef FEATURE_USE_BTLA
	hci_unregister_notifier(&hci_event_nblock);
#endif/*FEATURE_USE_BTLA*/
//END: 0019639 chanha.park@lge.com 2012-06-16
	platform_driver_unregister(&bluesleep_driver);

	remove_proc_entry("asleep", sleep_dir);
	remove_proc_entry("proto", sleep_dir);
	remove_proc_entry("hostwake", sleep_dir);
	remove_proc_entry("btwake", sleep_dir);
	remove_proc_entry("sleep", bluetooth_dir);
	remove_proc_entry("bluetooth", 0);
}

module_init(bluesleep_init);
module_exit(bluesleep_exit);

MODULE_DESCRIPTION("Bluetooth Sleep Mode Driver ver %s " VERSION);
#ifdef MODULE_LICENSE
MODULE_LICENSE("GPL");
#endif
