/* arch/arm/mach-msm/hsic_tty.c
 *
 * G-USB: hansun.lee@lge.com
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

//#define DEBUG
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/wakelock.h>
#include <linux/platform_device.h>
#include <linux/sched.h>

#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/tty_flip.h>
#include "hsic_tty_xport.h"

#define MAX_HSIC_TTYS 1
#define MAX_TTY_BUF_SIZE 2048

static DEFINE_MUTEX(hsic_tty_lock);

static uint hsic_tty_modem_wait = 60;
module_param_named(modem_wait, hsic_tty_modem_wait,
        uint, S_IRUGO | S_IWUSR | S_IWGRP);

struct hsic_tty_info {
    struct tty_struct *tty;
    struct tty_struct *ttys[MAX_HSIC_TTYS];
    struct wake_lock wake_lock;
    int open_counts;
    int open_count[MAX_HSIC_TTYS];
    struct tasklet_struct tty_tsklt;
    struct timer_list buf_req_timer;
    int in_reset;
    int in_reset_updated;
    int is_open;
    spinlock_t reset_lock;
    unsigned client_port_num;

    struct tdata_port *dport;
    struct tctrl_port *cport;

    spinlock_t          lock;

#ifdef CONFIG_MODEM_SUPPORT
    u8              pending;
    /* REVISIT avoid this CDC-ACM support harder ... */
//    struct usb_cdc_line_coding port_line_coding;    /* 9600-8-N-1 etc */
    /* SetControlLineState request */
    u16             port_handshake_bits;

    /* SerialState notification */
    u16             serial_state;

    /* control signal callbacks*/
    unsigned int (*get_dtr)(struct hsic_tty_info *p);
    unsigned int (*get_rts)(struct hsic_tty_info *p);

    /* notification callbacks */
    void (*connect)(struct hsic_tty_info *p);
    void (*disconnect)(struct hsic_tty_info *p);
    int (*send_break)(struct hsic_tty_info *p, int duration);
    unsigned int (*send_carrier_detect)(struct hsic_tty_info *p, unsigned int);
    unsigned int (*send_ring_indicator)(struct hsic_tty_info *p, unsigned int);
    int (*send_modem_ctrl_bits)(struct hsic_tty_info *p, int ctrl_bits);

    /* notification changes to modem */
    void (*notify_modem)(void *hsic_tty, u8 portno, int ctrl_bits);
#endif
};

static void buf_req_retry(unsigned long param);

#include "hsic_tty_data.c"
#include "hsic_tty_ctrl.c"

static struct hsic_tty_info hsic_tty;

#ifdef CONFIG_MODEM_SUPPORT
static int hsic_tty_tiocmset(struct tty_struct *tty, unsigned int set, unsigned int clear);

static int hsic_tty_notify_serial_state(struct hsic_tty_info *info)
{
    /*
    u16 serial_state = info->serial_state;

    pr_debug("%s - serial_state: dcd%c dsr%c break%c "
            "ring%c framing%c parity%c overrun%c\n", __func__,
            serial_state & ACM_CTRL_DCD ? '+' : '-',
            serial_state & ACM_CTRL_DSR ? '+' : '-',
            serial_state & ACM_CTRL_BRK ? '+' : '-',
            serial_state & ACM_CTRL_RI  ? '+' : '-',
            serial_state & ACM_CTRL_FRAMING ? '+' : '-',
            serial_state & ACM_CTRL_PARITY ? '+' : '-',
            serial_state & ACM_CTRL_OVERRUN ? '+' : '-');
    */

    return 1;
}

static void hsic_tty_connect(struct hsic_tty_info *info)
{
    pr_debug("%s\n", __func__);
    info->serial_state |= ACM_CTRL_DSR | ACM_CTRL_DCD;
    hsic_tty_notify_serial_state(info);
}

unsigned int hsic_tty_get_dtr(struct hsic_tty_info *info)
{
    pr_debug("%s\n", __func__);
    if (info->port_handshake_bits & ACM_CTRL_DTR)
        return 1;
    else
        return 0;
}

unsigned int hsic_tty_get_rts(struct hsic_tty_info *info)
{
    pr_debug("%s\n", __func__);
    if (info->port_handshake_bits & ACM_CTRL_RTS)
        return 1;
    else
        return 0;
}

unsigned int hsic_tty_send_carrier_detect(struct hsic_tty_info *info, unsigned int yes)
{
    u16         state;

    pr_debug("%s\n", __func__);
    state = info->serial_state;
    state &= ~ACM_CTRL_DCD;
    if (yes)
        state |= ACM_CTRL_DCD;

    info->serial_state = state;
    return hsic_tty_notify_serial_state(info);
}

unsigned int hsic_tty_send_ring_indicator(struct hsic_tty_info *info, unsigned int yes)
{
    u16         state;

    pr_debug("%s\n", __func__);
    state = info->serial_state;
    state &= ~ACM_CTRL_RI;
    if (yes)
        state |= ACM_CTRL_RI;

    info->serial_state = state;
    return hsic_tty_notify_serial_state(info);
}

static void hsic_tty_disconnect(struct hsic_tty_info *info)
{
    pr_debug("%s: dcd- dsr-\n", __func__);
    info->serial_state &= ~(ACM_CTRL_DSR | ACM_CTRL_DCD);
    hsic_tty_notify_serial_state(info);
}

static int hsic_tty_send_break(struct hsic_tty_info *info, int duration)
{
    u16         state;

    pr_debug("%s\n", __func__);
    state = info->serial_state;
    state &= ~ACM_CTRL_BRK;
    if (duration)
        state |= ACM_CTRL_BRK;

    info->serial_state = state;
    return hsic_tty_notify_serial_state(info);
}

static int hsic_tty_send_modem_ctrl_bits(struct hsic_tty_info *info, int ctrl_bits)
{
    pr_debug("%s: ctrl_bits(%04X)\n", __func__, ctrl_bits);
    info->serial_state = ctrl_bits;
    return hsic_tty_notify_serial_state(info);
}
#endif

static int is_in_reset(struct hsic_tty_info *info)
{
    return info->in_reset;
}

static void buf_req_retry(unsigned long param)
{
    struct hsic_tty_info *info = (struct hsic_tty_info *)param;
    struct tdata_port *port = info->dport;
    unsigned long flags;

    spin_lock_irqsave(&info->reset_lock, flags);
    if (info->is_open) {
        spin_unlock_irqrestore(&info->reset_lock, flags);
        queue_work(port->wq, &port->write_tohost_w);
        return;
    }
    spin_unlock_irqrestore(&info->reset_lock, flags);
}

static int hsic_tty_open(struct tty_struct *tty, struct file *f)
{
    int res = 0;
    unsigned int n = tty->index;
    struct hsic_tty_info *info = &hsic_tty;
    unsigned port_num;

    pr_debug("%s: #%d\n", __func__, n);

    if (n >= MAX_HSIC_TTYS)
        return -ENODEV;

    if (info->serial_state & ACM_CTRL_DSR & ACM_CTRL_DCD)
        return -ENODEV;

    port_num = info->client_port_num;

    mutex_lock(&hsic_tty_lock);
    tty->driver_data = info;

    if (info->open_counts++ == 0)
    {
        wake_lock_init(&info->wake_lock, WAKE_LOCK_SUSPEND, "hsic_tty");
    }

    if (info->open_count[n]++ == 0)
    {
        info->ttys[n] = tty;

        if (!info->tty || info->tty->index < n)
        {
            info->tty = tty;

#ifdef CONFIG_MODEM_SUPPORT
            if (n == 1)
            {
                if (info->port_handshake_bits & TIOCM_DTR)
                    hsic_tty_tiocmset(info->tty, 0, TIOCM_DTR);

                hsic_tty_tiocmset(info->tty, TIOCM_DTR, 0);
            }
#endif
        }
    }

    mutex_unlock(&hsic_tty_lock);

    return res;
}

static void hsic_tty_close(struct tty_struct *tty, struct file *f)
{
    struct hsic_tty_info *info = tty->driver_data;
    unsigned long flags;
    int n = tty->index;
    unsigned port_num;

    pr_debug("%s: #%d\n", __func__, n);

    if (info == 0)
        return;

    port_num = info->client_port_num;

    mutex_lock(&hsic_tty_lock);

    if (--info->open_count[n] == 0)
    {
#ifdef CONFIG_MODEM_SUPPORT
        if (n == 1)
            hsic_tty_tiocmset(info->tty, 0, TIOCM_DTR);
#endif

        info->ttys[n] = NULL;

        if (n-- > 0)
        {
            info->tty = info->ttys[n];
        }
    }

    if (--info->open_counts == 0)
    {
        spin_lock_irqsave(&info->reset_lock, flags);
        info->is_open = 0;
        spin_unlock_irqrestore(&info->reset_lock, flags);
        if (info->tty)
        {
            wake_lock_destroy(&info->wake_lock);
            info->tty = NULL;
        }
        tty->driver_data = 0;
        del_timer(&info->buf_req_timer);

#ifdef CONFIG_MODEM_SUPPORT
        info->port_handshake_bits = 0;
        info->serial_state = 0;
#endif
    }

    mutex_unlock(&hsic_tty_lock);
}

static int hsic_tty_write(struct tty_struct *tty, const unsigned char *buf, int len)
{
    struct hsic_tty_info *info = tty->driver_data;
    unsigned n = tty->index;
    struct tdata_port *port = info->dport;
    struct sk_buff		*skb;

    pr_debug("%s:#%d\n", __func__, n);

    /* if we're writing to a packet channel we will
     ** never be able to write more data than there
     ** is currently space for
     */
    if (is_in_reset(info))
    {
        pr_debug("%s: is in reset\n", __func__);
        return -ENETRESET;
    }

    if (info->tty->index != n)
    {
        pr_debug("%s: tty(%d) and info->tty(%d) dismatch, ignore it.\n",
                __func__, n, info->tty->index);
        return len;
    }

    skb = alloc_skb(len, GFP_ATOMIC);
	if(!skb)
	{
		pr_debug("%s: len alloc failed\n", __func__);
		return -ENOMEM;
	}
	skb->data = (unsigned char *)buf;
    skb->len = len;

    spin_lock(&port->rx_lock);
    __skb_queue_tail(&port->rx_skb_q, skb);
    if (info->port_handshake_bits & ACM_CTRL_DTR)
        queue_work(port->wq, &port->write_tomdm_w);
    spin_unlock(&port->rx_lock);

    return len;
}

static int hsic_tty_write_room(struct tty_struct *tty)
{
    struct hsic_tty_info *info = tty->driver_data;
    struct tdata_port *port = info->dport;
    return test_bit(CH_OPENED, &port->bridge_sts);
}

static int hsic_tty_chars_in_buffer(struct tty_struct *tty)
{
    struct hsic_tty_info *info = tty->driver_data;
    struct tdata_port *port = info->dport;
    return test_bit(CH_OPENED, &port->bridge_sts);
}

static void hsic_tty_unthrottle(struct tty_struct *tty)
{
    struct hsic_tty_info *info = tty->driver_data;
    struct tdata_port *port = info->dport;
    unsigned long flags;

    pr_debug("%s\n", __func__);

    spin_lock_irqsave(&info->reset_lock, flags);
    if (info->is_open) {
        spin_unlock_irqrestore(&info->reset_lock, flags);
        if (hsic_tty_data_fctrl_support &&
                port->tx_skb_q.qlen <= hsic_tty_data_fctrl_dis_thld &&
                test_and_clear_bit(RX_THROTTLED, &port->brdg.flags)) {
            port->rx_unthrottled_cnt++;
            port->unthrottled_pnd_skbs = port->tx_skb_q.qlen;
            pr_debug("%s: disable flow ctrl:"
                    " tx skbq len: %u\n",
                    __func__, port->tx_skb_q.qlen);
            data_bridge_unthrottle_rx(port->brdg.ch_id);
            queue_work(port->wq, &port->write_tohost_w);
        }
        return;
    }
    spin_unlock_irqrestore(&info->reset_lock, flags);
}

static int hsic_tty_tiocmget(struct tty_struct *tty)
{
    struct hsic_tty_info *info = tty->driver_data;
    return /* ((info->port_handshake_bits & ACM_CTRL_RTS) ? TIOCM_RTS : 0) |
        ((info->port_handshake_bits & ACM_CTRL_DTR) ? TIOCM_DTR: 0) |
        ((info->port_handshake_bits & ACM_CTRL_RTS) ? TIOCM_CTS : 0) | */
        ((info->serial_state & ACM_CTRL_DSR) ? TIOCM_DSR : 0) |
        ((info->serial_state & ACM_CTRL_DCD) ? TIOCM_CD: 0) |
        ((info->serial_state & ACM_CTRL_RI) ? TIOCM_RI: 0);
}

static int hsic_tty_tiocmset(struct tty_struct *tty, unsigned int set, unsigned int clear)
{
    struct hsic_tty_info *info = tty->driver_data;
    unsigned n = tty->index;
    struct tdata_port *port = info->dport;
    unsigned port_num;

    pr_debug("%s:#%d set(%04X), clear(%04X)\n", __func__, n, set, clear);
    if (is_in_reset(info))
    {
        pr_debug("%s: is in reset\n", __func__);
        return -ENETRESET;
    }

    if (info->tty->index != n)
    {
        pr_debug("%s: tty(%d) and info->tty(%d) dismatch, ignore it.\n",
                __func__, n, info->tty->index);
        return 0;
    }

    port_num = info->client_port_num;

    /* set */
    info->port_handshake_bits |= 
        ((set & TIOCM_DTR) ? ACM_CTRL_DTR : 0) | ((set & TIOCM_RTS) ? ACM_CTRL_RTS : 0);
    /* clear */
    info->port_handshake_bits &= 
        ~(((clear & TIOCM_DTR) ? ACM_CTRL_DTR : 0) | ((clear & TIOCM_RTS) ? ACM_CTRL_RTS : 0));

    info->notify_modem(info, port_num, info->port_handshake_bits);

    if ((info->port_handshake_bits & ACM_CTRL_DTR) &&
            !(info->port_handshake_bits & ACM_CTRL_RTS))
    {
        info->port_handshake_bits |= ACM_CTRL_RTS;
        info->notify_modem(info, port_num, info->port_handshake_bits);
    }

    if (!(info->port_handshake_bits & ACM_CTRL_DTR) &&
            (info->port_handshake_bits & ACM_CTRL_RTS))
    {
        info->port_handshake_bits &= ~ACM_CTRL_RTS;
        info->notify_modem(info, port_num, info->port_handshake_bits);
    }

    if (info->port_handshake_bits & (ACM_CTRL_DTR | ACM_CTRL_RTS))
    {
        pr_debug("%s: ACM_CTRL_DTR and ACM_CTRL_RTS is set. start write_tomdm\n", __func__);
        spin_lock(&port->rx_lock);
        queue_work(port->wq, &port->write_tomdm_w);
        spin_unlock(&port->rx_lock);
    }

    return 0;
}

static struct tty_operations hsic_tty_ops = {
    .open = hsic_tty_open,
    .close = hsic_tty_close,
    .write = hsic_tty_write,
    .write_room = hsic_tty_write_room,
    .chars_in_buffer = hsic_tty_chars_in_buffer,
    .unthrottle = hsic_tty_unthrottle,
    .tiocmget = hsic_tty_tiocmget,
    .tiocmset = hsic_tty_tiocmset,
};

static struct tty_driver *hsic_tty_driver;

static int __init hsic_tty_init(void)
{
    int ret;
    int n;
    unsigned port_num;

    pr_debug("%s\n", __func__);

    hsic_tty_driver = alloc_tty_driver(MAX_HSIC_TTYS);
    if (hsic_tty_driver == 0)
        return -ENOMEM;

    hsic_tty_driver->owner = THIS_MODULE;
    hsic_tty_driver->driver_name = "hsic_tty_driver";
    hsic_tty_driver->name = "hsic";
    hsic_tty_driver->major = 0;
    hsic_tty_driver->minor_start = 0;
    hsic_tty_driver->type = TTY_DRIVER_TYPE_SERIAL;
    hsic_tty_driver->subtype = SERIAL_TYPE_NORMAL;
    hsic_tty_driver->init_termios = tty_std_termios;
    hsic_tty_driver->init_termios.c_iflag = 0;
    hsic_tty_driver->init_termios.c_oflag = 0;
    hsic_tty_driver->init_termios.c_cflag = B38400 | CS8 | CREAD;
    hsic_tty_driver->init_termios.c_lflag = 0;
    hsic_tty_driver->flags = TTY_DRIVER_RESET_TERMIOS |
        TTY_DRIVER_REAL_RAW | TTY_DRIVER_DYNAMIC_DEV;
    tty_set_operations(hsic_tty_driver, &hsic_tty_ops);

    ret = tty_register_driver(hsic_tty_driver);
    if (ret) {
        put_tty_driver(hsic_tty_driver);
        pr_err("%s: driver registration failed %d\n", __func__, ret);
        return ret;
    }

    port_num = hsic_tty_data_setup(1, HSIC_TTY_SERIAL);
    if (port_num < 0)
    {
        pr_err("%s: hsic_tty_data_setup failed\n", __func__);
        goto out;
    }

    ret = hsic_tty_ctrl_setup(1, HSIC_TTY_SERIAL);
    if (ret < 0)
    {
        pr_err("%s: hsic_tty_ctrl_setup failed\n", __func__);
        goto out;
    }

    hsic_tty.client_port_num = port_num;
#ifdef CONFIG_MODEM_SUPPORT
    spin_lock_init(&hsic_tty.lock);
    spin_lock_init(&hsic_tty.reset_lock);
    hsic_tty.connect = hsic_tty_connect;
    hsic_tty.get_dtr = hsic_tty_get_dtr;
    hsic_tty.get_rts = hsic_tty_get_rts;
    hsic_tty.send_carrier_detect = hsic_tty_send_carrier_detect;
    hsic_tty.send_ring_indicator = hsic_tty_send_ring_indicator;
    hsic_tty.send_modem_ctrl_bits = hsic_tty_send_modem_ctrl_bits;
    hsic_tty.disconnect = hsic_tty_disconnect;
    hsic_tty.send_break = hsic_tty_send_break;;
#endif
    hsic_tty.tty = NULL;

    ret = hsic_tty_ctrl_connect(&hsic_tty, port_num);
    if (ret) {
        pr_err("%s: hsic_tty_ctrl_connect failed: err:%d\n",
                __func__, ret);
        goto out;
    }

    ret = hsic_tty_data_connect(&hsic_tty, port_num);
    if (ret) {
        pr_err("%s: hsic_tty_data_connect failed: err:%d\n",
                __func__, ret);
        hsic_tty_ctrl_disconnect(&hsic_tty, port_num);
        goto out;
    }

    for (n = 0; n < MAX_HSIC_TTYS; ++n)
    {
        pr_info("%s: %d\n", __func__, n);
        tty_register_device(hsic_tty_driver, n, 0);
    }
    return 0;

out:
    tty_unregister_driver(hsic_tty_driver);
    put_tty_driver(hsic_tty_driver);
    return ret;
}

module_init(hsic_tty_init);
