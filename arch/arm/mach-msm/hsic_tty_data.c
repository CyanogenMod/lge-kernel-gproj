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

//#define VERBOSE_DEBUG
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/termios.h>
#include <linux/netdevice.h>
#include <linux/debugfs.h>
#include <linux/bitops.h>
#include <linux/termios.h>
#include <mach/usb_bridge.h>
#include "hsic_tty_xport.h"

static unsigned int no_data_ports;

static const char *data_bridge_names[] = {
    "dun_data_hsic0",
    "rmnet_data_hsic0"
};

#define DATA_BRIDGE_NAME_MAX_LEN		20

#define HSIC_TTY_DATA_RMNET_RX_Q_SIZE		50
#define HSIC_TTY_DATA_RMNET_TX_Q_SIZE		300
#define HSIC_TTY_DATA_SERIAL_RX_Q_SIZE		10
#define HSIC_TTY_DATA_SERIAL_TX_Q_SIZE		20
#define HSIC_TTY_DATA_RX_REQ_SIZE			2048
#define HSIC_TTY_DATA_TX_INTR_THRESHOLD		20

static unsigned int hsic_tty_data_rmnet_tx_q_size = HSIC_TTY_DATA_RMNET_TX_Q_SIZE;
module_param(hsic_tty_data_rmnet_tx_q_size, uint, S_IRUGO | S_IWUSR);

static unsigned int hsic_tty_data_rmnet_rx_q_size = HSIC_TTY_DATA_RMNET_RX_Q_SIZE;
module_param(hsic_tty_data_rmnet_rx_q_size, uint, S_IRUGO | S_IWUSR);

static unsigned int hsic_tty_data_serial_tx_q_size = HSIC_TTY_DATA_SERIAL_TX_Q_SIZE;
module_param(hsic_tty_data_serial_tx_q_size, uint, S_IRUGO | S_IWUSR);

static unsigned int hsic_tty_data_serial_rx_q_size = HSIC_TTY_DATA_SERIAL_RX_Q_SIZE;
module_param(hsic_tty_data_serial_rx_q_size, uint, S_IRUGO | S_IWUSR);

static unsigned int hsic_tty_data_rx_req_size = HSIC_TTY_DATA_RX_REQ_SIZE;
module_param(hsic_tty_data_rx_req_size, uint, S_IRUGO | S_IWUSR);

unsigned int hsic_tty_data_tx_intr_thld = HSIC_TTY_DATA_TX_INTR_THRESHOLD;
module_param(hsic_tty_data_tx_intr_thld, uint, S_IRUGO | S_IWUSR);

/*flow ctrl*/
#define HSIC_TTY_DATA_FLOW_CTRL_EN_THRESHOLD	500
#define HSIC_TTY_DATA_FLOW_CTRL_DISABLE		300
#define HSIC_TTY_DATA_FLOW_CTRL_SUPPORT		1
#define HSIC_TTY_DATA_PENDLIMIT_WITH_BRIDGE	500

static unsigned int hsic_tty_data_fctrl_support = HSIC_TTY_DATA_FLOW_CTRL_SUPPORT;
module_param(hsic_tty_data_fctrl_support, uint, S_IRUGO | S_IWUSR);

static unsigned int hsic_tty_data_fctrl_en_thld =
HSIC_TTY_DATA_FLOW_CTRL_EN_THRESHOLD;
module_param(hsic_tty_data_fctrl_en_thld, uint, S_IRUGO | S_IWUSR);

static unsigned int hsic_tty_data_fctrl_dis_thld = HSIC_TTY_DATA_FLOW_CTRL_DISABLE;
module_param(hsic_tty_data_fctrl_dis_thld, uint, S_IRUGO | S_IWUSR);

static unsigned int hsic_tty_data_pend_limit_with_bridge =
HSIC_TTY_DATA_PENDLIMIT_WITH_BRIDGE;
module_param(hsic_tty_data_pend_limit_with_bridge, uint, S_IRUGO | S_IWUSR);

#define CH_OPENED 0
#define CH_READY 1

struct tdata_port {
    /* port */
    unsigned		port_num;

    /* tty */
    atomic_t		connected;
    struct hsic_tty_info *info;

    enum tty_type	ttype;

    /* data transfer queues */
    unsigned int		tx_q_size;
    struct list_head	tx_idle;
    struct sk_buff_head	tx_skb_q;
    spinlock_t		tx_lock;

    unsigned int		rx_q_size;
    struct list_head	rx_idle;
    struct sk_buff_head	rx_skb_q;
    spinlock_t		rx_lock;

    /* work */
    struct workqueue_struct	*wq;
    struct work_struct	connect_w;
    struct work_struct	disconnect_w;
    struct work_struct	write_tomdm_w;
    struct work_struct	write_tohost_w;

    struct bridge		brdg;

    /*bridge status*/
    unsigned long		bridge_sts;

    unsigned int		n_tx_req_queued;

    /*counters*/
    unsigned long		to_modem;
    unsigned long		to_host;
    unsigned int		rx_throttled_cnt;
    unsigned int		rx_unthrottled_cnt;
    unsigned int		tx_throttled_cnt;
    unsigned int		tx_unthrottled_cnt;
    unsigned int		tomodem_drp_cnt;
    unsigned int		unthrottled_pnd_skbs;
};

static struct {
    struct tdata_port	*port;
    struct platform_driver	pdrv;
} tdata_ports[NUM_PORTS];

static unsigned int get_timestamp(void);
//static void dbg_timestamp(char *, struct sk_buff *);
static void hsic_tty_data_start_rx(struct tdata_port *port);
static int is_in_reset(struct hsic_tty_info *info);

//static void hsic_tty_data_free_requests(struct usb_ep *ep, struct list_head *head)
//{
//    struct usb_request	*req;
//
//    while (!list_empty(head)) {
//        req = list_entry(head->next, struct usb_request, list);
//        list_del(&req->list);
//        usb_ep_free_request(ep, req);
//    }
//}

//static int hsic_tty_data_alloc_requests(struct usb_ep *ep, struct list_head *head,
//        int num,
//        void (*cb)(struct usb_ep *ep, struct usb_request *),
//        gfp_t flags)
//{
//    int			i;
//    struct usb_request	*req;
//
//    pr_debug("%s: ep:%s head:%p num:%d cb:%p", __func__,
//            ep->name, head, num, cb);
//
//    for (i = 0; i < num; i++) {
//        req = usb_ep_alloc_request(ep, flags);
//        if (!req) {
//            pr_debug("%s: req allocated:%d\n", __func__, i);
//            return list_empty(head) ? -ENOMEM : 0;
//        }
//        req->complete = cb;
//        list_add(&req->list, head);
//    }
//
//    return 0;
//}

static void hsic_tty_data_unthrottle_tx(void *ctx)
{
    struct tdata_port	*port = ctx;
    unsigned long		flags;

    if (!port || !atomic_read(&port->connected))
        return;

    spin_lock_irqsave(&port->rx_lock, flags);
    port->tx_unthrottled_cnt++;
    spin_unlock_irqrestore(&port->rx_lock, flags);

    queue_work(port->wq, &port->write_tomdm_w);
    pr_debug("%s: port num =%d unthrottled\n", __func__,
            port->port_num);
}

static void hsic_tty_data_write_tohost(struct work_struct *w)
{
    unsigned long		flags;
    struct sk_buff		*skb;
    int			ret;
    struct tdata_port	*port;
    unsigned char *ptr;
    struct hsic_tty_info *info;
    struct tty_struct *tty;

    pr_debug("%s\n", __func__);

    port = container_of(w, struct tdata_port, write_tohost_w);

    if (!port)
        return;

    mutex_lock(&hsic_tty_lock);
    info = port->info;
    tty = info->tty;
    pr_debug("%s: tty(%p)\n", __func__, tty);
    if (!tty)
    {
        mutex_unlock(&hsic_tty_lock);
        return;
    }

    spin_lock_irqsave(&port->tx_lock, flags);

    for (;;)
    {
        if (is_in_reset(info)) {
            /* signal TTY clients using TTY_BREAK */
            tty_insert_flip_char(tty, 0x00, TTY_BREAK);
            tty_flip_buffer_push(tty);
            break;
        }

        skb = __skb_dequeue(&port->tx_skb_q);
        if (!skb)
            break;

        port->n_tx_req_queued++;
        if (port->n_tx_req_queued == hsic_tty_data_tx_intr_thld) {
            port->n_tx_req_queued = 0;
        }

        spin_unlock_irqrestore(&port->tx_lock, flags);

#ifdef VERBOSE_DEBUG
        print_hex_dump(KERN_DEBUG, "tohost:", DUMP_PREFIX_OFFSET, 16, 1, skb->data, skb->len, 1);
#endif
        ret = tty_prepare_flip_string(tty, &ptr, skb->len);
        if (ret <= 0) {
            pr_err("%s: tty_prepare_flip_string failed\n", __func__);
            if (!timer_pending(&info->buf_req_timer)) {
                init_timer(&info->buf_req_timer);
                info->buf_req_timer.expires = jiffies +
                    ((30 * HZ)/1000);
                info->buf_req_timer.function = buf_req_retry;
                info->buf_req_timer.data = (unsigned long)info;
                add_timer(&info->buf_req_timer);
            }
            mutex_unlock(&hsic_tty_lock);
            return;
        }

        memcpy(ptr, skb->data, ret);
        dev_kfree_skb_any(skb);
        wake_lock_timeout(&info->wake_lock, HZ / 2);
        tty_flip_buffer_push(tty);

        spin_lock_irqsave(&port->tx_lock, flags);
        port->to_host++;
        if (hsic_tty_data_fctrl_support &&
                port->tx_skb_q.qlen <= hsic_tty_data_fctrl_dis_thld &&
                test_and_clear_bit(RX_THROTTLED, &port->brdg.flags)) {
            port->rx_unthrottled_cnt++;
            port->unthrottled_pnd_skbs = port->tx_skb_q.qlen;
            pr_debug("%s: disable flow ctrl:"
                    " tx skbq len: %u\n",
                    __func__, port->tx_skb_q.qlen);
            data_bridge_unthrottle_rx(port->brdg.ch_id);
        }
    }
    spin_unlock_irqrestore(&port->tx_lock, flags);

    /* XXX only when writable and necessary */
    tty_wakeup(tty);
    mutex_unlock(&hsic_tty_lock);
}

static int hsic_tty_data_receive(void *p, void *data, size_t len)
{
    struct tdata_port	*port = p;
    unsigned long		flags;
    struct sk_buff		*skb = data;

    pr_debug("%s\n", __func__);

    if (!port || !atomic_read(&port->connected)) {
        dev_kfree_skb_any(skb);
        return -ENOTCONN;
    }

    pr_debug("%s: p:%p#%d skb_len:%d\n", __func__,
            port, port->port_num, skb->len);

    spin_lock_irqsave(&port->tx_lock, flags);
    __skb_queue_tail(&port->tx_skb_q, skb);

    if (hsic_tty_data_fctrl_support &&
            port->tx_skb_q.qlen >= hsic_tty_data_fctrl_en_thld) {
        set_bit(RX_THROTTLED, &port->brdg.flags);
        port->rx_throttled_cnt++;
        pr_debug("%s: flow ctrl enabled: tx skbq len: %u\n",
                __func__, port->tx_skb_q.qlen);
        spin_unlock_irqrestore(&port->tx_lock, flags);
        queue_work(port->wq, &port->write_tohost_w);
        return -EBUSY;
    }

    spin_unlock_irqrestore(&port->tx_lock, flags);

    queue_work(port->wq, &port->write_tohost_w);

    return 0;
}

static void hsic_tty_data_write_tomdm(struct work_struct *w)
{
    struct tdata_port	*port;
    struct sk_buff		*skb;
    struct timestamp_info	*info;
    unsigned long		flags;
    int			ret;

    pr_debug("%s\n", __func__);

    port = container_of(w, struct tdata_port, write_tomdm_w);

    if (!port || !atomic_read(&port->connected))
    {
        pr_debug("%s: port(%p), connected(%d)\n", __func__, port, atomic_read(&port->connected));
        return;
    }

    spin_lock_irqsave(&port->rx_lock, flags);
    if (test_bit(TX_THROTTLED, &port->brdg.flags)) {
        pr_debug("%s: TX_THROTTLED\n", __func__);
        spin_unlock_irqrestore(&port->rx_lock, flags);
        goto start_rx;
    }

    while ((skb = __skb_dequeue(&port->rx_skb_q))) {
        pr_debug("%s: port:%p tom:%lu pno:%d\n", __func__,
                port, port->to_modem, port->port_num);

#ifdef VERBOSE_DEBUG
        print_hex_dump(KERN_DEBUG, "tomdm:", DUMP_PREFIX_OFFSET, 16, 1, skb->data, skb->len, 1);
#endif

        info = (struct timestamp_info *)skb->cb;
        info->rx_done_sent = get_timestamp();
        spin_unlock_irqrestore(&port->rx_lock, flags);
        ret = data_bridge_write(port->brdg.ch_id, skb);
        spin_lock_irqsave(&port->rx_lock, flags);
        if (ret < 0) {
            if (ret == -EBUSY) {
                /*flow control*/
                port->tx_throttled_cnt++;
                break;
            }
            pr_err("%s: write error:%d\n",
                    __func__, ret);
            port->tomodem_drp_cnt++;
            dev_kfree_skb_any(skb);
            break;
        }
        port->to_modem++;
    }
    spin_unlock_irqrestore(&port->rx_lock, flags);
start_rx:
    hsic_tty_data_start_rx(port);
}

//static void hsic_tty_data_epin_complete(struct usb_ep *ep, struct usb_request *req)
//{
//    struct tdata_port	*port = ep->driver_data;
//    struct sk_buff		*skb = req->context;
//    int			status = req->status;
//
//    switch (status) {
//        case 0:
//            /* successful completion */
//            dbg_timestamp("DL", skb);
//            break;
//        case -ECONNRESET:
//        case -ESHUTDOWN:
//            /* connection gone */
//            dev_kfree_skb_any(skb);
//            req->buf = 0;
//            usb_ep_free_request(ep, req);
//            return;
//        default:
//            pr_err("%s: data tx ep error %d\n", __func__, status);
//            break;
//    }
//
//    dev_kfree_skb_any(skb);
//
//    spin_lock(&port->tx_lock);
//    list_add_tail(&req->list, &port->tx_idle);
//    spin_unlock(&port->tx_lock);
//
//    queue_work(port->wq, &port->write_tohost_w);
//}

//    static void
//hsic_tty_data_epout_complete(struct usb_ep *ep, struct usb_request *req)
//{
//    struct tdata_port	*port = ep->driver_data;
//    struct sk_buff		*skb = req->context;
//    struct timestamp_info	*info = (struct timestamp_info *)skb->cb;
//    int			status = req->status;
//    int			queue = 0;
//
//    switch (status) {
//        case 0:
//            skb_put(skb, req->actual);
//            queue = 1;
//            break;
//        case -ECONNRESET:
//        case -ESHUTDOWN:
//            /* cable disconnection */
//            dev_kfree_skb_any(skb);
//            req->buf = 0;
//            usb_ep_free_request(ep, req);
//            return;
//        default:
//            pr_err("%s: %s response error %d, %d/%d\n",
//                    __func__, ep->name, status,
//                    req->actual, req->length);
//            dev_kfree_skb_any(skb);
//            break;
//    }
//
//    spin_lock(&port->rx_lock);
//    if (queue) {
//        info->rx_done = get_timestamp();
//        __skb_queue_tail(&port->rx_skb_q, skb);
//        list_add_tail(&req->list, &port->rx_idle);
//        queue_work(port->wq, &port->write_tomdm_w);
//    }
//    spin_unlock(&port->rx_lock);
//}

static void hsic_tty_data_start_rx(struct tdata_port *port)
{
//    struct usb_request	*req;
//    struct usb_ep		*ep;
    unsigned long		flags;
//    int			ret;
    struct sk_buff		*skb;
    struct timestamp_info	*info;
    unsigned int		created;

    pr_debug("%s: port:%p\n", __func__, port);
    if (!port)
        return;

    spin_lock_irqsave(&port->rx_lock, flags);
//    ep = port->out;
//    if (!ep) {
//        spin_unlock_irqrestore(&port->rx_lock, flags);
//        return;
//    }

    while (atomic_read(&port->connected) && !list_empty(&port->rx_idle)) {
        if (port->rx_skb_q.qlen > hsic_tty_data_pend_limit_with_bridge)
            break;

//        req = list_first_entry(&port->rx_idle,
//                struct usb_request, list);

        created = get_timestamp();
        skb = alloc_skb(hsic_tty_data_rx_req_size, GFP_ATOMIC);
        if (!skb)
            break;
        info = (struct timestamp_info *)skb->cb;
        info->created = created;
//        list_del(&req->list);
//        req->buf = skb->data;
//        req->length = hsic_tty_data_rx_req_size;
//        req->context = skb;

        info->rx_queued = get_timestamp();
//        spin_unlock_irqrestore(&port->rx_lock, flags);
//        ret = usb_ep_queue(ep, req, GFP_KERNEL);
//        spin_lock_irqsave(&port->rx_lock, flags);
//        if (ret) {
//            dev_kfree_skb_any(skb);
//
//            pr_err("%s: rx queue failed\n", __func__);
//
//            if (atomic_read(&port->connected))
//                list_add(&req->list, &port->rx_idle);
//            else
//                usb_ep_free_request(ep, req);
//            break;
//        }
    }
    spin_unlock_irqrestore(&port->rx_lock, flags);
}

static void hsic_tty_data_start_io(struct tdata_port *port)
{
//    unsigned long	flags;
//    struct usb_ep	*ep;
//    int		ret;

    pr_debug("%s: port:%p\n", __func__, port);

    if (!port)
        return;

//    spin_lock_irqsave(&port->rx_lock, flags);
//    ep = port->out;
//    if (!ep) {
//        spin_unlock_irqrestore(&port->rx_lock, flags);
//        return;
//    }

//    ret = hsic_tty_data_alloc_requests(ep, &port->rx_idle,
//            port->rx_q_size, hsic_tty_data_epout_complete, GFP_ATOMIC);
//    if (ret) {
//        pr_err("%s: rx req allocation failed\n", __func__);
//        spin_unlock_irqrestore(&port->rx_lock, flags);
//        return;
//    }
//    spin_unlock_irqrestore(&port->rx_lock, flags);

//    spin_lock_irqsave(&port->tx_lock, flags);
//    ep = port->in;
//    if (!ep) {
//        spin_unlock_irqrestore(&port->tx_lock, flags);
//        return;
//    }

//    ret = hsic_tty_data_alloc_requests(ep, &port->tx_idle,
//            port->tx_q_size, hsic_tty_data_epin_complete, GFP_ATOMIC);
//    if (ret) {
//        pr_err("%s: tx req allocation failed\n", __func__);
//        hsic_tty_data_free_requests(ep, &port->rx_idle);
//        spin_unlock_irqrestore(&port->tx_lock, flags);
//        return;
//    }
//    spin_unlock_irqrestore(&port->tx_lock, flags);

    /* queue out requests */
    hsic_tty_data_start_rx(port);
}

static void hsic_tty_data_connect_w(struct work_struct *w)
{
    struct tdata_port	*port =
        container_of(w, struct tdata_port, connect_w);
    int			ret;

    pr_debug("%s\n", __func__);

    if (!port || !atomic_read(&port->connected) ||
            !test_bit(CH_READY, &port->bridge_sts))
    {
        pr_debug("%s: port(%p), connected(%d), CH_READY(%d)\n", __func__,
                port, atomic_read(&port->connected), test_bit(CH_READY, &port->bridge_sts));
        return;
    }

    pr_debug("%s: port:%p\n", __func__, port);

    ret = data_bridge_open(&port->brdg);
    if (ret) {
        pr_err("%s: unable open bridge ch:%d err:%d\n",
                __func__, port->brdg.ch_id, ret);
        return;
    }

    set_bit(CH_OPENED, &port->bridge_sts);

    hsic_tty_data_start_io(port);
}

static void hsic_tty_data_disconnect_w(struct work_struct *w)
{
    struct tdata_port	*port =
        container_of(w, struct tdata_port, disconnect_w);

    if (!test_bit(CH_OPENED, &port->bridge_sts))
        return;

    data_bridge_close(port->brdg.ch_id);
    clear_bit(CH_OPENED, &port->bridge_sts);
}

static void hsic_tty_data_free_buffers(struct tdata_port *port)
{
    struct sk_buff	*skb;
    unsigned long	flags;

    if (!port)
        return;

    spin_lock_irqsave(&port->tx_lock, flags);
//    if (!port->in) {
//        spin_unlock_irqrestore(&port->tx_lock, flags);
//        return;
//    }

//    hsic_tty_data_free_requests(port->in, &port->tx_idle);

    while ((skb = __skb_dequeue(&port->tx_skb_q)))
        dev_kfree_skb_any(skb);
    spin_unlock_irqrestore(&port->tx_lock, flags);

    spin_lock_irqsave(&port->rx_lock, flags);
//    if (!port->out) {
//        spin_unlock_irqrestore(&port->rx_lock, flags);
//        return;
//    }
//
//    hsic_tty_data_free_requests(port->out, &port->rx_idle);

    while ((skb = __skb_dequeue(&port->rx_skb_q)))
        dev_kfree_skb_any(skb);
    spin_unlock_irqrestore(&port->rx_lock, flags);
}

static int hsic_tty_data_probe(struct platform_device *pdev)
{
    struct tdata_port *port;

    pr_debug("%s: name:%s no_data_ports= %d\n",
            __func__, pdev->name, no_data_ports);

    if (pdev->id >= no_data_ports) {
        pr_err("%s: invalid port: %d\n", __func__, pdev->id);
        return -EINVAL;
    }

    port = tdata_ports[pdev->id].port;
    set_bit(CH_READY, &port->bridge_sts);

    /* if usb is online, try opening bridge */
    if (atomic_read(&port->connected))
        queue_work(port->wq, &port->connect_w);

    return 0;
}

/* mdm disconnect */
static int hsic_tty_data_remove(struct platform_device *pdev)
{
    struct tdata_port *port;
//    struct usb_ep	*ep_in;
//    struct usb_ep	*ep_out;

    pr_debug("%s: name:%s\n", __func__, pdev->name);

    if (pdev->id >= no_data_ports) {
        pr_err("%s: invalid port: %d\n", __func__, pdev->id);
        return -EINVAL;
    }

    port = tdata_ports[pdev->id].port;

//    ep_in = port->in;
//    if (ep_in)
//        usb_ep_fifo_flush(ep_in);
//
//    ep_out = port->out;
//    if (ep_out)
//        usb_ep_fifo_flush(ep_out);

    hsic_tty_data_free_buffers(port);

    data_bridge_close(port->brdg.ch_id);

    clear_bit(CH_READY, &port->bridge_sts);
    clear_bit(CH_OPENED, &port->bridge_sts);

    return 0;
}

static void hsic_tty_data_port_free(int portno)
{
    struct tdata_port	*port = tdata_ports[portno].port;
    struct platform_driver	*pdrv = &tdata_ports[portno].pdrv;

    destroy_workqueue(port->wq);
    kfree(port);

    if (pdrv)
        platform_driver_unregister(pdrv);
}

static int hsic_tty_data_port_alloc(unsigned port_num, enum tty_type ttype)
{
    struct tdata_port	*port;
    struct platform_driver	*pdrv;

    port = kzalloc(sizeof(struct tdata_port), GFP_KERNEL);
    if (!port)
        return -ENOMEM;

    port->wq = create_singlethread_workqueue(data_bridge_names[port_num]);
    if (!port->wq) {
        pr_err("%s: Unable to create workqueue:%s\n",
                __func__, data_bridge_names[port_num]);
        kfree(port);
        return -ENOMEM;
    }
    port->port_num = port_num;

    /* port initialization */
    spin_lock_init(&port->rx_lock);
    spin_lock_init(&port->tx_lock);

    INIT_WORK(&port->connect_w, hsic_tty_data_connect_w);
    INIT_WORK(&port->disconnect_w, hsic_tty_data_disconnect_w);
    INIT_WORK(&port->write_tohost_w, hsic_tty_data_write_tohost);
    INIT_WORK(&port->write_tomdm_w, hsic_tty_data_write_tomdm);

    INIT_LIST_HEAD(&port->tx_idle);
    INIT_LIST_HEAD(&port->rx_idle);

    skb_queue_head_init(&port->tx_skb_q);
    skb_queue_head_init(&port->rx_skb_q);

    port->ttype = ttype;
    port->brdg.ch_id = port_num;
    port->brdg.ctx = port;
    port->brdg.ops.send_pkt = hsic_tty_data_receive;
    port->brdg.ops.unthrottle_tx = hsic_tty_data_unthrottle_tx;
    tdata_ports[port_num].port = port;

    pdrv = &tdata_ports[port_num].pdrv;
    pdrv->probe = hsic_tty_data_probe;
    pdrv->remove = hsic_tty_data_remove;
    pdrv->driver.name = data_bridge_names[port_num];
    pdrv->driver.owner = THIS_MODULE;

    platform_driver_register(pdrv);

    pr_debug("%s: port:%p portno:%d\n", __func__, port, port_num);

    return 0;
}

void hsic_tty_data_disconnect(void *tptr, int port_num)
{
    struct tdata_port	*port;
    unsigned long		flags;

    pr_debug("%s: port#%d\n", __func__, port_num);

    port = tdata_ports[port_num].port;

    if (port_num > no_data_ports) {
        pr_err("%s: invalid portno#%d\n", __func__, port_num);
        return;
    }

    if (!tptr || !port) {
        pr_err("%s: port is null\n", __func__);
        return;
    }

    hsic_tty_data_free_buffers(port);

    /* disable endpoints */
    atomic_set(&port->connected, 0);

    spin_lock_irqsave(&port->tx_lock, flags);
    port->n_tx_req_queued = 0;
    clear_bit(RX_THROTTLED, &port->brdg.flags);
    spin_unlock_irqrestore(&port->tx_lock, flags);

    spin_lock_irqsave(&port->rx_lock, flags);
    clear_bit(TX_THROTTLED, &port->brdg.flags);
    spin_unlock_irqrestore(&port->rx_lock, flags);

    queue_work(port->wq, &port->disconnect_w);
}

int hsic_tty_data_connect(void *tptr, int port_num)
{
    struct tdata_port *port;
    struct hsic_tty_info *info;
    unsigned long flags;
    int ret = 0;

    pr_debug("%s: port#%d\n", __func__, port_num);

    port = tdata_ports[port_num].port;

    if (port_num > no_data_ports) {
        pr_err("%s: invalid portno#%d\n", __func__, port_num);
        return -ENODEV;
    }

    if (!tptr || !port) {
        pr_err("%s: port is null\n", __func__);
        return -ENODEV;
    }

    info = (struct hsic_tty_info *)tptr;
    info->dport = port;
    port->info = info;

    if (port->ttype == HSIC_TTY_SERIAL) {
        port->tx_q_size = hsic_tty_data_serial_tx_q_size;
        port->rx_q_size = hsic_tty_data_serial_rx_q_size;
    } else {
        port->tx_q_size = hsic_tty_data_rmnet_tx_q_size;
        port->rx_q_size = hsic_tty_data_rmnet_rx_q_size;
    }

    atomic_set(&port->connected, 1);

    spin_lock_irqsave(&port->tx_lock, flags);
    port->to_host = 0;
    port->rx_throttled_cnt = 0;
    port->rx_unthrottled_cnt = 0;
    port->unthrottled_pnd_skbs = 0;
    spin_unlock_irqrestore(&port->tx_lock, flags);

    spin_lock_irqsave(&port->rx_lock, flags);
    port->to_modem = 0;
    port->tomodem_drp_cnt = 0;
    port->tx_throttled_cnt = 0;
    port->tx_unthrottled_cnt = 0;
    spin_unlock_irqrestore(&port->rx_lock, flags);

    queue_work(port->wq, &port->connect_w);

    return ret;
}

#if defined(CONFIG_DEBUG_FS)
#define DEBUG_BUF_SIZE 1024

static unsigned int	record_timestamp;
module_param(record_timestamp, uint, S_IRUGO | S_IWUSR);

static struct timestamp_buf dbg_data = {
    .idx = 0,
    .lck = __RW_LOCK_UNLOCKED(lck)
};

/*get_timestamp - returns time of day in us */
static unsigned int get_timestamp(void)
{
    struct timeval	tval;
    unsigned int	stamp;

    if (!record_timestamp)
        return 0;

    do_gettimeofday(&tval);
    /* 2^32 = 4294967296. Limit to 4096s. */
    stamp = tval.tv_sec & 0xFFF;
    stamp = stamp * 1000000 + tval.tv_usec;
    return stamp;
}

static void dbg_inc(unsigned *idx)
{
    *idx = (*idx + 1) & (DBG_DATA_MAX-1);
}

/**
 * dbg_timestamp - Stores timestamp values of a SKB life cycle
 *	to debug buffer
 * @event: "DL": Downlink Data
 * @skb: SKB used to store timestamp values to debug buffer
 */
/*
static void dbg_timestamp(char *event, struct sk_buff * skb)
{
    unsigned long		flags;
    struct timestamp_info	*info = (struct timestamp_info *)skb->cb;

    if (!record_timestamp)
        return;

    write_lock_irqsave(&dbg_data.lck, flags);

    scnprintf(dbg_data.buf[dbg_data.idx], DBG_DATA_MSG,
            "%p %u[%s] %u %u %u %u %u %u\n",
            skb, skb->len, event, info->created, info->rx_queued,
            info->rx_done, info->rx_done_sent, info->tx_queued,
            get_timestamp());

    dbg_inc(&dbg_data.idx);

    write_unlock_irqrestore(&dbg_data.lck, flags);
}
*/

/* show_timestamp: displays the timestamp buffer */
static ssize_t show_timestamp(struct file *file, char __user *ubuf,
        size_t count, loff_t *ppos)
{
    unsigned long	flags;
    unsigned	i;
    unsigned	j = 0;
    char		*buf;
    int		ret = 0;

    if (!record_timestamp)
        return 0;

    buf = kzalloc(sizeof(char) * 4 * DEBUG_BUF_SIZE, GFP_KERNEL);
    if (!buf)
        return -ENOMEM;

    read_lock_irqsave(&dbg_data.lck, flags);

    i = dbg_data.idx;
    for (dbg_inc(&i); i != dbg_data.idx; dbg_inc(&i)) {
        if (!strnlen(dbg_data.buf[i], DBG_DATA_MSG))
            continue;
        j += scnprintf(buf + j, (4 * DEBUG_BUF_SIZE) - j,
                "%s\n", dbg_data.buf[i]);
    }

    read_unlock_irqrestore(&dbg_data.lck, flags);

    ret = simple_read_from_buffer(ubuf, count, ppos, buf, j);

    kfree(buf);

    return ret;
}

const struct file_operations tdata_timestamp_ops = {
    .read = show_timestamp,
};

static ssize_t hsic_tty_data_read_stats(struct file *file,
        char __user *ubuf, size_t count, loff_t *ppos)
{
    struct tdata_port	*port;
    struct platform_driver	*pdrv;
    char			*buf;
    unsigned long		flags;
    int			ret;
    int			i;
    int			temp = 0;

    buf = kzalloc(sizeof(char) * DEBUG_BUF_SIZE, GFP_KERNEL);
    if (!buf)
        return -ENOMEM;

    for (i = 0; i < no_data_ports; i++) {
        port = tdata_ports[i].port;
        if (!port)
            continue;
        pdrv = &tdata_ports[i].pdrv;

        spin_lock_irqsave(&port->rx_lock, flags);
        temp += scnprintf(buf + temp, DEBUG_BUF_SIZE - temp,
                "\nName:           %s\n"
                "#PORT:%d port#:   %p\n"
                "data_ch_open:	   %d\n"
                "data_ch_ready:    %d\n"
                "\n******UL INFO*****\n\n"
                "dpkts_to_modem:   %lu\n"
                "tomodem_drp_cnt:  %u\n"
                "rx_buf_len:       %u\n"
                "tx thld cnt       %u\n"
                "tx unthld cnt     %u\n"
                "TX_THROTTLED      %d\n",
                pdrv->driver.name,
                i, port,
                test_bit(CH_OPENED, &port->bridge_sts),
                test_bit(CH_READY, &port->bridge_sts),
                port->to_modem,
                port->tomodem_drp_cnt,
                port->rx_skb_q.qlen,
                port->tx_throttled_cnt,
                port->tx_unthrottled_cnt,
                test_bit(TX_THROTTLED, &port->brdg.flags));
        spin_unlock_irqrestore(&port->rx_lock, flags);

        spin_lock_irqsave(&port->tx_lock, flags);
        temp += scnprintf(buf + temp, DEBUG_BUF_SIZE - temp,
                "\n******DL INFO******\n\n"
                "dpkts_to_usbhost: %lu\n"
                "tx_buf_len:	   %u\n"
                "rx thld cnt	   %u\n"
                "rx unthld cnt	   %u\n"
                "uthld pnd skbs    %u\n"
                "RX_THROTTLED	   %d\n",
                port->to_host,
                port->tx_skb_q.qlen,
                port->rx_throttled_cnt,
                port->rx_unthrottled_cnt,
                port->unthrottled_pnd_skbs,
                test_bit(RX_THROTTLED, &port->brdg.flags));
        spin_unlock_irqrestore(&port->tx_lock, flags);

    }

    ret = simple_read_from_buffer(ubuf, count, ppos, buf, temp);

    kfree(buf);

    return ret;
}

static ssize_t hsic_tty_data_reset_stats(struct file *file,
        const char __user *buf, size_t count, loff_t *ppos)
{
    struct tdata_port	*port;
    int			i;
    unsigned long		flags;

    for (i = 0; i < no_data_ports; i++) {
        port = tdata_ports[i].port;
        if (!port)
            continue;

        spin_lock_irqsave(&port->rx_lock, flags);
        port->to_modem = 0;
        port->tomodem_drp_cnt = 0;
        port->tx_throttled_cnt = 0;
        port->tx_unthrottled_cnt = 0;
        spin_unlock_irqrestore(&port->rx_lock, flags);

        spin_lock_irqsave(&port->tx_lock, flags);
        port->to_host = 0;
        port->rx_throttled_cnt = 0;
        port->rx_unthrottled_cnt = 0;
        port->unthrottled_pnd_skbs = 0;
        spin_unlock_irqrestore(&port->tx_lock, flags);
    }
    return count;
}

const struct file_operations hsic_tty_stats_ops = {
    .read = hsic_tty_data_read_stats,
    .write = hsic_tty_data_reset_stats,
};

static struct dentry	*tdata_dent;
static struct dentry	*tdata_dfile_stats;
static struct dentry	*tdata_dfile_tstamp;

static void hsic_tty_data_debugfs_init(void)
{
    tdata_dent = debugfs_create_dir("hsic_tty_data_xport", 0);
    if (IS_ERR(tdata_dent))
        return;

    tdata_dfile_stats = debugfs_create_file("status", 0444, tdata_dent, 0,
            &hsic_tty_stats_ops);
    if (!tdata_dfile_stats || IS_ERR(tdata_dfile_stats)) {
        debugfs_remove(tdata_dent);
        return;
    }

    tdata_dfile_tstamp = debugfs_create_file("timestamp", 0644, tdata_dent,
            0, &tdata_timestamp_ops);
    if (!tdata_dfile_tstamp || IS_ERR(tdata_dfile_tstamp))
        debugfs_remove(tdata_dent);
}

static void hsic_tty_data_debugfs_exit(void)
{
    debugfs_remove(tdata_dfile_stats);
    debugfs_remove(tdata_dfile_tstamp);
    debugfs_remove(tdata_dent);
}

#else
static void hsic_tty_data_debugfs_init(void) { }
static void hsic_tty_data_debugfs_exit(void) { }
//static void dbg_timestamp(char *event, struct sk_buff * skb)
//{
//    return;
//}
static unsigned int get_timestamp(void)
{
    return 0;
}

#endif

int hsic_tty_data_setup(unsigned num_ports, enum tty_type ttype)
{
    int		first_port_id = no_data_ports;
    int		total_num_ports = num_ports + no_data_ports;
    int		ret = 0;
    int		i;

    if (!num_ports || total_num_ports > NUM_PORTS) {
        pr_err("%s: Invalid num of ports count:%d\n",
                __func__, num_ports);
        return -EINVAL;
    }
    pr_debug("%s: count: %d\n", __func__, num_ports);

    for (i = first_port_id; i < (num_ports + first_port_id); i++) {

        /*probe can be called while port_alloc,so update no_data_ports*/
        no_data_ports++;
        ret = hsic_tty_data_port_alloc(i, ttype);
        if (ret) {
            no_data_ports--;
            pr_err("%s: Unable to alloc port:%d\n", __func__, i);
            goto free_ports;
        }
    }

    /*return the starting index*/
    return first_port_id;

free_ports:
    for (i = first_port_id; i < no_data_ports; i++)
        hsic_tty_data_port_free(i);
    no_data_ports = first_port_id;

    return ret;
}

static int __init hsic_tty_data_init(void)
{
    hsic_tty_data_debugfs_init();

    return 0;
}
module_init(hsic_tty_data_init);

static void __exit hsic_tty_data_exit(void)
{
    hsic_tty_data_debugfs_exit();
}
module_exit(hsic_tty_data_exit);
MODULE_DESCRIPTION("hsic data xport driver");
MODULE_LICENSE("GPL v2");
