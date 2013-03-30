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
 */

#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/termios.h>
#include <linux/debugfs.h>
#include <linux/bitops.h>
#include <linux/termios.h>
#include <mach/usb_bridge.h>
#include "hsic_tty_xport.h"

/* from cdc-acm.h */
#define ACM_CTRL_RTS		(1 << 1)	/* unused with full duplex */
#define ACM_CTRL_DTR		(1 << 0)	/* host is ready for data r/w */
#define ACM_CTRL_OVERRUN	(1 << 6)
#define ACM_CTRL_PARITY		(1 << 5)
#define ACM_CTRL_FRAMING	(1 << 4)
#define ACM_CTRL_RI			(1 << 3)
#define ACM_CTRL_BRK		(1 << 2)
#define ACM_CTRL_DSR		(1 << 1)
#define ACM_CTRL_DCD		(1 << 0)


static unsigned int	no_ctrl_ports;

static const char	*ctrl_bridge_names[] = {
    "dun_ctrl_hsic0",
    "rmnet_ctrl_hsic0"
};

#define CTRL_BRIDGE_NAME_MAX_LEN	20
#define READ_BUF_LEN			1024

#define CH_OPENED 0
#define CH_READY 1

struct tctrl_port {
    /* port */
    unsigned		port_num;

    /* tty */
    spinlock_t		port_lock;
    struct hsic_tty_info *info;

    /* work queue*/
    struct workqueue_struct	*wq;
    struct work_struct	connect_w;
    struct work_struct	disconnect_w;

    enum tty_type	ttype;

    /*ctrl pkt response cb*/
    int (*send_cpkt_response)(void *g, void *buf, size_t len);

    struct bridge		brdg;

    /* bridge status */
    unsigned long		bridge_sts;

    /* control bits */
    unsigned		cbits_tomodem;
    unsigned		cbits_tohost;

    /* counters */
    unsigned long		to_modem;
    unsigned long		to_host;
    unsigned long		drp_cpkt_cnt;
};

static struct {
    struct tctrl_port	*port;
    struct platform_driver	pdrv;
} tctrl_ports[NUM_PORTS];

static int hsic_tty_ctrl_receive(void *dev, void *buf, size_t actual)
{
    struct tctrl_port	*port = dev;
    int retval = 0;

    pr_debug("%s: read complete bytes read: %d\n",
            __func__, actual);

    /* send it to USB here */
    if (port && port->send_cpkt_response) {
        retval = port->send_cpkt_response(port->info, buf, actual);
        port->to_host++;
    }

    return retval;
}

//    static int
//hsic_tty_send_cpkt_tomodem(u8 portno, void *buf, size_t len)
//{
//    void			*cbuf;
//    struct tctrl_port	*port;
//
//    if (portno >= no_ctrl_ports) {
//        pr_err("%s: Invalid portno#%d\n", __func__, portno);
//        return -ENODEV;
//    }
//
//    port = tctrl_ports[portno].port;
//    if (!port) {
//        pr_err("%s: port is null\n", __func__);
//        return -ENODEV;
//    }
//
//    cbuf = kmalloc(len, GFP_ATOMIC);
//    if (!cbuf)
//        return -ENOMEM;
//
//    memcpy(cbuf, buf, len);
//
//    /* drop cpkt if ch is not open */
//    if (!test_bit(CH_OPENED, &port->bridge_sts)) {
//        port->drp_cpkt_cnt++;
//        kfree(cbuf);
//        return 0;
//    }
//
//    pr_debug("%s: ctrl_pkt:%d bytes\n", __func__, len);
//
//    ctrl_bridge_write(port->brdg.ch_id, cbuf, len);
//
//    port->to_modem++;
//
//    return 0;
//}

    static void
hsic_tty_send_cbits_tomodem(void *tptr, u8 portno, int cbits)
{
    struct tctrl_port	*port;

    if (portno >= no_ctrl_ports || !tptr) {
        pr_err("%s: Invalid portno#%d\n", __func__, portno);
        return;
    }

    port = tctrl_ports[portno].port;
    if (!port) {
        pr_err("%s: port is null\n", __func__);
        return;
    }

    if (cbits == port->cbits_tomodem)
        return;

    port->cbits_tomodem = cbits;

    if (!test_bit(CH_OPENED, &port->bridge_sts))
        return;

    pr_debug("%s: ctrl_tomodem:%d\n", __func__, cbits);

    ctrl_bridge_set_cbits(port->brdg.ch_id, cbits);
}

static void hsic_tty_ctrl_connect_w(struct work_struct *w)
{
    struct hsic_tty_info *info = NULL;
    struct tctrl_port	*port =
        container_of(w, struct tctrl_port, connect_w);
    unsigned long		flags;
    int			retval;
    unsigned		cbits;

    if (!port || !test_bit(CH_READY, &port->bridge_sts))
        return;

    pr_debug("%s: port:%p\n", __func__, port);

    retval = ctrl_bridge_open(&port->brdg);
    if (retval) {
        pr_err("%s: ctrl bridge open failed :%d\n", __func__, retval);
        return;
    }

    spin_lock_irqsave(&port->port_lock, flags);
    if (!port->info) {
        ctrl_bridge_close(port->brdg.ch_id);
        spin_unlock_irqrestore(&port->port_lock, flags);
        return;
    }
    set_bit(CH_OPENED, &port->bridge_sts);
    spin_unlock_irqrestore(&port->port_lock, flags);

    cbits = ctrl_bridge_get_cbits_tohost(port->brdg.ch_id);

    info = port->info;
    if ((port->ttype == HSIC_TTY_SERIAL && (cbits & ACM_CTRL_DCD)) ||
        port->ttype == HSIC_TTY_RMNET) {
        if (info && info->connect)
            info->connect(info);
        return;
    }
}

int hsic_tty_ctrl_connect(void *tptr, int port_num)
{
    struct tctrl_port	*port;
    struct hsic_tty_info *info;
    unsigned long		flags;

    pr_debug("%s: port#%d\n", __func__, port_num);

    if (port_num > no_ctrl_ports || !tptr) {
        pr_err("%s: invalid portno#%d\n", __func__, port_num);
        return -ENODEV;
    }

    port = tctrl_ports[port_num].port;
    if (!port) {
        pr_err("%s: port is null\n", __func__);
        return -ENODEV;
    }

    spin_lock_irqsave(&port->port_lock, flags);
    info = tptr;
    info->cport = port;
    port->info = info;
    if (port->ttype == HSIC_TTY_SERIAL) {
        info->notify_modem = hsic_tty_send_cbits_tomodem;
    }

    port->info = tptr;
    port->to_host = 0;
    port->to_modem = 0;
    port->drp_cpkt_cnt = 0;
    spin_unlock_irqrestore(&port->port_lock, flags);

    queue_work(port->wq, &port->connect_w);

    return 0;
}

static void tctrl_disconnect_w(struct work_struct *w)
{
    struct tctrl_port	*port =
        container_of(w, struct tctrl_port, disconnect_w);

    if (!test_bit(CH_OPENED, &port->bridge_sts))
        return;

    /* send the dtr zero */
    ctrl_bridge_close(port->brdg.ch_id);
    clear_bit(CH_OPENED, &port->bridge_sts);
}

void hsic_tty_ctrl_disconnect(void *tptr, int port_num)
{
    struct tctrl_port	*port;
    struct hsic_tty_info *info;
    unsigned long		flags;

    pr_debug("%s: port#%d\n", __func__, port_num);

    port = tctrl_ports[port_num].port;

    if (port_num > no_ctrl_ports) {
        pr_err("%s: invalid portno#%d\n", __func__, port_num);
        return;
    }

    if (!tptr || !port) {
        pr_err("%s: grmnet port is null\n", __func__);
        return;
    }

    info = port->info;
    spin_lock_irqsave(&port->port_lock, flags);
    if (info)
        info->notify_modem = 0;
    port->cbits_tomodem = 0;
    port->info = NULL;
    port->send_cpkt_response = 0;
    spin_unlock_irqrestore(&port->port_lock, flags);

    queue_work(port->wq, &port->disconnect_w);
}

static void hsic_tty_ctrl_status(void *ctxt, unsigned int ctrl_bits)
{
    struct tctrl_port	*port = ctxt;
    struct hsic_tty_info *info;

    pr_debug("%s - input control lines: dcd%c dsr%c break%c "
            "ring%c framing%c parity%c overrun%c\n", __func__,
            ctrl_bits & ACM_CTRL_DCD ? '+' : '-',
            ctrl_bits & ACM_CTRL_DSR ? '+' : '-',
            ctrl_bits & ACM_CTRL_BRK ? '+' : '-',
            ctrl_bits & ACM_CTRL_RI  ? '+' : '-',
            ctrl_bits & ACM_CTRL_FRAMING ? '+' : '-',
            ctrl_bits & ACM_CTRL_PARITY ? '+' : '-',
            ctrl_bits & ACM_CTRL_OVERRUN ? '+' : '-');

    port->cbits_tohost = ctrl_bits;
    info = port->info;
    if (info && info->send_modem_ctrl_bits)
        info->send_modem_ctrl_bits(info, ctrl_bits);
}

static int hsic_tty_ctrl_probe(struct platform_device *pdev)
{
    struct tctrl_port	*port;
    unsigned long		flags;

    pr_debug("%s: name:%s\n", __func__, pdev->name);

    if (pdev->id >= no_ctrl_ports) {
        pr_err("%s: invalid port: %d\n", __func__, pdev->id);
        return -EINVAL;
    }

    port = tctrl_ports[pdev->id].port;
    set_bit(CH_READY, &port->bridge_sts);

    /* if usb is online, start read */
    spin_lock_irqsave(&port->port_lock, flags);
    if (port->info)
        queue_work(port->wq, &port->connect_w);
    spin_unlock_irqrestore(&port->port_lock, flags);

    return 0;
}

static int hsic_tty_ctrl_remove(struct platform_device *pdev)
{
    struct tctrl_port	*port;
    struct hsic_tty_info *info = NULL;
    unsigned long		flags;

    pr_debug("%s: name:%s\n", __func__, pdev->name);

    if (pdev->id >= no_ctrl_ports) {
        pr_err("%s: invalid port: %d\n", __func__, pdev->id);
        return -EINVAL;
    }

    port = tctrl_ports[pdev->id].port;

    spin_lock_irqsave(&port->port_lock, flags);
    if (!port->info) {
        spin_unlock_irqrestore(&port->port_lock, flags);
        goto not_ready;
    }

    info = port->info;

    port->cbits_tohost = 0;
    spin_unlock_irqrestore(&port->port_lock, flags);

    if (info && info->disconnect)
        info->disconnect(info);

    ctrl_bridge_close(port->brdg.ch_id);

    clear_bit(CH_OPENED, &port->bridge_sts);
not_ready:
    clear_bit(CH_READY, &port->bridge_sts);

    return 0;
}

static void hsic_tty_ctrl_port_free(int portno)
{
    struct tctrl_port	*port = tctrl_ports[portno].port;
    struct platform_driver	*pdrv = &tctrl_ports[portno].pdrv;

    destroy_workqueue(port->wq);
    kfree(port);

    if (pdrv)
        platform_driver_unregister(pdrv);
}

static int tctrl_port_alloc(int portno, enum tty_type ttype)
{
    struct tctrl_port	*port;
    struct platform_driver	*pdrv;

    port = kzalloc(sizeof(struct tctrl_port), GFP_KERNEL);
    if (!port)
        return -ENOMEM;

    port->wq = create_singlethread_workqueue(ctrl_bridge_names[portno]);
    if (!port->wq) {
        pr_err("%s: Unable to create workqueue:%s\n",
                __func__, ctrl_bridge_names[portno]);
        return -ENOMEM;
    }

    port->port_num = portno;
    port->ttype = ttype;

    spin_lock_init(&port->port_lock);

    INIT_WORK(&port->connect_w, hsic_tty_ctrl_connect_w);
    INIT_WORK(&port->disconnect_w, tctrl_disconnect_w);

    port->brdg.ch_id = portno;
    port->brdg.ctx = port;
    port->brdg.ops.send_pkt = hsic_tty_ctrl_receive;
    if (port->ttype == HSIC_TTY_SERIAL)
        port->brdg.ops.send_cbits = hsic_tty_ctrl_status;
    tctrl_ports[portno].port = port;

    pdrv = &tctrl_ports[portno].pdrv;
    pdrv->probe = hsic_tty_ctrl_probe;
    pdrv->remove = hsic_tty_ctrl_remove;
    pdrv->driver.name = ctrl_bridge_names[portno];
    pdrv->driver.owner = THIS_MODULE;

    platform_driver_register(pdrv);

    pr_debug("%s: port:%p portno:%d\n", __func__, port, portno);

    return 0;
}

int hsic_tty_ctrl_setup(unsigned int num_ports, enum tty_type ttype)
{
    int		first_port_id = no_ctrl_ports;
    int		total_num_ports = num_ports + no_ctrl_ports;
    int		i;
    int		ret = 0;

    if (!num_ports || total_num_ports > NUM_PORTS) {
        pr_err("%s: Invalid num of ports count:%d\n",
                __func__, num_ports);
        return -EINVAL;
    }

    pr_debug("%s: requested ports:%d\n", __func__, num_ports);

    for (i = first_port_id; i < (first_port_id + num_ports); i++) {

        /*probe can be called while port_alloc,so update no_ctrl_ports*/
        no_ctrl_ports++;
        ret = tctrl_port_alloc(i, ttype);
        if (ret) {
            no_ctrl_ports--;
            pr_err("%s: Unable to alloc port:%d\n", __func__, i);
            goto free_ports;
        }
    }

    return first_port_id;

free_ports:
    for (i = first_port_id; i < no_ctrl_ports; i++)
        hsic_tty_ctrl_port_free(i);
    no_ctrl_ports = first_port_id;
    return ret;
}

#if defined(CONFIG_DEBUG_FS)
#define DEBUG_BUF_SIZE	1024
static ssize_t tctrl_read_stats(struct file *file, char __user *ubuf,
        size_t count, loff_t *ppos)
{
    struct tctrl_port	*port;
    struct platform_driver	*pdrv;
    char			*buf;
    unsigned long		flags;
    int			ret;
    int			i;
    int			temp = 0;

    buf = kzalloc(sizeof(char) * DEBUG_BUF_SIZE, GFP_KERNEL);
    if (!buf)
        return -ENOMEM;

    for (i = 0; i < no_ctrl_ports; i++) {
        port = tctrl_ports[i].port;
        if (!port)
            continue;
        pdrv = &tctrl_ports[i].pdrv;
        spin_lock_irqsave(&port->port_lock, flags);

        temp += scnprintf(buf + temp, DEBUG_BUF_SIZE - temp,
                "\nName:        %s\n"
                "#PORT:%d port: %p\n"
                "to_usbhost:    %lu\n"
                "to_modem:      %lu\n"
                "cpkt_drp_cnt:  %lu\n"
                "DTR:           %s\n"
                "ch_open:       %d\n"
                "ch_ready:      %d\n",
                pdrv->driver.name,
                i, port,
                port->to_host, port->to_modem,
                port->drp_cpkt_cnt,
                port->cbits_tomodem ? "HIGH" : "LOW",
                test_bit(CH_OPENED, &port->bridge_sts),
                test_bit(CH_READY, &port->bridge_sts));

        spin_unlock_irqrestore(&port->port_lock, flags);
    }

    ret = simple_read_from_buffer(ubuf, count, ppos, buf, temp);

    kfree(buf);

    return ret;
}

static ssize_t tctrl_reset_stats(struct file *file,
        const char __user *buf, size_t count, loff_t *ppos)
{
    struct tctrl_port	*port;
    int			i;
    unsigned long		flags;

    for (i = 0; i < no_ctrl_ports; i++) {
        port = tctrl_ports[i].port;
        if (!port)
            continue;

        spin_lock_irqsave(&port->port_lock, flags);
        port->to_host = 0;
        port->to_modem = 0;
        port->drp_cpkt_cnt = 0;
        spin_unlock_irqrestore(&port->port_lock, flags);
    }
    return count;
}

const struct file_operations tctrl_stats_ops = {
    .read = tctrl_read_stats,
    .write = tctrl_reset_stats,
};

struct dentry	*tctrl_dent;
struct dentry	*tctrl_dfile;
static void tctrl_debugfs_init(void)
{
    tctrl_dent = debugfs_create_dir("hsic_tty_ctrl_xport", 0);
    if (IS_ERR(tctrl_dent))
        return;

    tctrl_dfile =
        debugfs_create_file("status", 0444, tctrl_dent, 0,
                &tctrl_stats_ops);
    if (!tctrl_dfile || IS_ERR(tctrl_dfile))
        debugfs_remove(tctrl_dent);
}

static void tctrl_debugfs_exit(void)
{
    debugfs_remove(tctrl_dfile);
    debugfs_remove(tctrl_dent);
}

#else
static void tctrl_debugfs_init(void) { }
static void tctrl_debugfs_exit(void) { }
#endif

static int __init tctrl_init(void)
{
    tctrl_debugfs_init();

    return 0;
}
module_init(tctrl_init);

static void __exit tctrl_exit(void)
{
    tctrl_debugfs_exit();
}
module_exit(tctrl_exit);
MODULE_DESCRIPTION("hsic control xport driver");
MODULE_LICENSE("GPL v2");
