/*
 * u_atcmd.c - AT Command Handler
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/device.h>

#include <asm/uaccess.h>
#include <asm/io.h>


/* for debug... */
//#define ATCMD_DBG

#ifdef ATCMD_DBG
#define ATCMD_DBG_DATA_ROW 16
#define ATCMD_DBG_DATA(data, len) \
    do { \
        char _atcmd_dbg_hex[3*ATCMD_DBG_DATA_ROW+1]; \
        char _atcmd_dbg_ascii[ATCMD_DBG_DATA_ROW+1]; \
        int idx = 0, row, i; \
        pr_info("%s: ATCMD_DBG_DATA = len(%d)\n", __func__, len); \
        while (idx < len) { \
            memset(_atcmd_dbg_ascii, ' ', ATCMD_DBG_DATA_ROW); \
            _atcmd_dbg_ascii[ATCMD_DBG_DATA_ROW] = '\0'; \
            row = len - idx; \
            row = (row > ATCMD_DBG_DATA_ROW) ? ATCMD_DBG_DATA_ROW: row; \
            for (i = 0; i < row; i++, idx++) { \
                sprintf(&_atcmd_dbg_hex[i*3], "%02X ", data[idx]); \
                _atcmd_dbg_ascii[i] = (data[idx] > 0x20 && data[idx] < 0x7f) ? data[idx] : '?'; \
            } \
            pr_info("%s : %s\n", _atcmd_dbg_ascii, _atcmd_dbg_hex); \
        } \
    } while(0)
#else
#define ATCMD_DBG_DATA(data, len) \
    do {} while(0)
#endif

static const char *atcmd_ap[] = {
    "+MTC","+CCLK","+CSDF","+CSO","+CSS","+CTSA","+VZWAPNE","%ACCEL","%ACS","%ALC","%AVR","%BATL",
    "%BATT","%BCPE","%BCPL","%BOOF","%BTAD","%BTSEARCH","%BTTM","%CALCK","%CALDT","%CAM","%CAMPREQ",
    "%CHARGE","%CHCOMP","%CHIPSET","%CODECRC","%COMPASS","%DBCHK","%DETACH","%DEVICETEST","%DIAGNV",
    "%DRMCERT","%DRMERASE","%DRMINDEX","%DRMTYPE","%ECALL","%EMMREJECT","%EMT","%FBOOT","%FILECRC",
    "%FKPD","%FLIGHT","%FOTAID","%FPRICRC","%FRST","%FRSTSTATUS","%FUELRST","%FUELVAL","%FUSG","%GKPD",
    "%GNSS","%GNSS1","%GYRO","%SURV","%VZWHM","%VZWIOTHM","%HWVER","%IDDE","%IMEI","%3WAYSYNC","%IMPL",
    "%IMSTESTMODE","%INFO","%INISIM","%ISSIM","%KEYLOCK","%LANG","%LCATT","%LCD","%BOFF","%LCDCAPTURE",
    "%LCDINFO","%LCIMSSETCFG","%LGANDROID","%LGATSERVICE","%LGPWD","%LOGSERVICE","%LTEAUTHFAIL",
    "%LTECALL","%LTERFMODE","%MAC","%MACCK","%MAXCT","%MIMOANTCHECK","%MLT","%MMCFACTORYFORMAT",
    "%MMCFORMAT","%MMCISFORMATTED","%MMCTOTALSIZE","%MMCUSEDSIZE","%DATST","%MOT","%MPT","%NCM","%NFC",
    "%NTCODE","%NTSWAP","%OSPPWDINIT","%OSVER","%PMRST","%PNUM","%PNUMBER","%PNUMBER","%PNUM","%PROXIMITY",
    "%PTNCLR","%REATTACH","%RESTART","%SATPC","%LOGSAVE","%SCHK","%WALLPAPER","%SERIALNO","%SIMID","%SIMOFF",
    "%SIMPWDINIT","%SLEN","%SLTYPE","%SPM","%SUFFIX","%SULC","%SWOV","%SWV","%TETHER","%TOTALCRC",
    "%TOUCHFWVER","%ULCV","%ULCW","%USB","%VCOIN","%VLC","%VLST","%VSLT","%WLAN","%WLANR","%WLANT",
    "%TSENS","%BATMP","%MDMPVS",
    "+CATLIST","+CTACT","+CCLGS","+CDUR","+CDVOL","+CEMAIL","+CWAP","+CDCONT","+CSYNC","+CBLTH","+CALRM",
    "+CTMRV","+CSMCT","+CWLNT","+CKSUM","+CNPAD","+CTASK","+CMSG","+CTBCPS","+CRST",
    "+OMADM","+PRL","+FUMO","$PRL?",

    NULL
    //don't use "%QEM","+CKPD"
};

enum {
    ATCMD_TO_AP,
    ATCMD_TO_CP
};

static struct {
    struct gdata_port *port;
    struct mutex lock;
} atcmd_info;

static ssize_t atcmd_write_tomdm(const char *buf, size_t count);

#define list_last_entry(ptr, type, member) \
    list_entry((ptr)->prev, type, member)

static int atcmd_major;
static DECLARE_WAIT_QUEUE_HEAD(atcmd_read_wait);
static int atcmd_modem = 0;

LIST_HEAD(atcmd_pool);
struct atcmd_request {
    char buf[2048];
    unsigned length;
    unsigned status;

    struct list_head list;
};

static struct class *atcmd_class;
static struct device *atcmd_dev;

static char atcmd_name[2048];
static char atcmd_state[2048];

static struct atcmd_request *atcmd_alloc_req(void)
{
    struct atcmd_request *req;

    req = kmalloc(sizeof(struct atcmd_request), GFP_KERNEL);
    if (req == NULL)
    {
        pr_err("%s: req alloc failed\n", __func__);
        return NULL;
    }

    req->length = 0;
    req->status = 0;

    return req;
}

static void atcmd_free_req(struct atcmd_request *req)
{
    kfree(req);
}

static int atcmd_to(const char *buf, size_t count)
{
    int i, len;
    char *p;

    strncpy(atcmd_name, buf, count);
    if ((p = strchr(atcmd_name, '=')) ||
        (p = strchr(atcmd_name, '?')) ||
        (p = strchr(atcmd_name, '\r')) )
    {
        *p = '\0';
    }

    for (i = 0; atcmd_ap[i] != NULL; i++)
    {
        len = strlen(atcmd_ap[i]);

        if (!strcasecmp(&atcmd_name[2], atcmd_ap[i]))
        {
            if ((p = strchr(buf, '=')) || (p = strchr(buf, '?')))
            {
                strncpy(atcmd_state, p, count);
                p = strchr(atcmd_state, '\r');
                if (p)
                    *p = '\0';
            }
            else
            {
                atcmd_state[0] = '\0';
            }

#ifdef ATCMD_DBG
            pr_info("%s: ATCMD_TO_AP: matching!!! %s, %s\n", __func__, atcmd_name, atcmd_state);
#endif
            return ATCMD_TO_AP;
        }
    }

#ifdef ATCMD_DBG
    pr_info("%s: ATCMD_TO_CP\n", __func__);
#endif
    return ATCMD_TO_CP;
}

int atcmd_queue(const char *buf, size_t count)
{
    struct atcmd_request *req;
    int ret = 1;

    if (count <= 0)
    {
        pr_debug("%s: count <= 0\n", __func__);
        return -1;
    }

    mutex_lock(&atcmd_info.lock);

    /* atcmd_pool is empty or new pool */
    if (list_empty(&atcmd_pool) ||
        (req = list_last_entry(&atcmd_pool, struct atcmd_request, list))->status == 1)
    {
        if (count >= 3 &&
            strncasecmp(buf, "at%", 3) &&
            strncasecmp(buf, "at+", 3) &&
            strncasecmp(buf, "at$", 3) &&
            strncasecmp(buf, "at*", 3))
        {
            ret = 0;
            goto out;
        }
        else if (count >= 2 && strncasecmp(buf, "at", 2))
        {
            ret = 0;
            goto out;
        }
        else if (buf[0] != 'a' && buf[0] != 'A')
        {
            ret = 0;
            goto out;
        }

        req = atcmd_alloc_req();
        if( req == NULL )
        {
            pr_err("can't alloc for req\n" ) ;
            ret = -ENOMEM ;
            goto out;
        }

        list_add_tail(&req->list, &atcmd_pool);
    }

    memcpy(&req->buf[req->length], buf, count);
    req->length += count;
    req->buf[req->length] = '\0';

#ifdef ATCMD_DBG
    ATCMD_DBG_DATA(req->buf, req->length);
#endif

    if (buf[count-1] == '\r' ||
        buf[count-1] == '\n' ||
        buf[count-1] == '\0')
    {
        if (atcmd_to(req->buf, req->length) == ATCMD_TO_AP)
        {
            if (atcmd_modem)
            {
#ifdef ATCMD_DBG
                pr_info("%s: wake_up! atcmd_read_wait\n", __func__);
#endif
                req->status = 1;
                wake_up_interruptible(&atcmd_read_wait);
            }
            else
            {
#ifdef ATCMD_DBG
                pr_info("%s: throw away! free_req\n", __func__);
#endif
                list_del(&req->list);
                atcmd_free_req(req);
            }
        }
        else
        {
            /* write to sdio */
            atcmd_write_tomdm(req->buf, req->length);
            list_del(&req->list);
            atcmd_free_req(req);
        }
    }

out:
    mutex_unlock(&atcmd_info.lock);
    return ret;
}

static int atcmd_write_tohost(const char *buf, size_t count)
{
    struct gdata_port *port = atcmd_info.port;
    unsigned long flags;
    struct sk_buff *skb;

    ATCMD_DBG_DATA(buf, count);

    skb = alloc_skb(count, GFP_KERNEL);
    if (!skb)
    {
        pr_err("%s: skb alloc failed\n", __func__);
        return -ENOMEM ;
    }
    memcpy(skb->data, buf, count);
    skb->len = count;

    spin_lock_irqsave(&port->tx_lock, flags);
    __skb_queue_tail(&port->tx_skb_q, skb);
    if (!(count & 0x7FF))
    {
        skb = alloc_skb(0, GFP_ATOMIC);
        if (!skb)
        {
            pr_err("%s: null_skb alloc failed\n", __func__);
            skb = __skb_dequeue(&port->tx_skb_q);
            dev_kfree_skb_any(skb);
            spin_unlock_irqrestore(&port->tx_lock, flags);
            return -ENOMEM ;
        }
        skb->data = NULL;
        skb->len = 0;
        __skb_queue_tail(&port->tx_skb_q, skb);
    }
    spin_unlock_irqrestore(&port->tx_lock, flags);
    queue_work(port->wq, &port->write_tohost_w);

    return count;
}

static ssize_t atcmd_write_tomdm(const char *buf, size_t count)
{
    struct gdata_port *port = atcmd_info.port;
    struct sk_buff *skb;
    int ret;

    ATCMD_DBG_DATA(buf, count);

    skb = alloc_skb(count, GFP_KERNEL);
    if (!skb )
    {
        pr_err("%s: skb alloc failed\n", __func__);
        return -ENOMEM ;
    }
    memcpy(skb->data, buf, count);
    skb->len = count;

    ret = data_bridge_write(port->brdg.ch_id, skb);
    if (ret < 0) {
        if (ret == -EBUSY) {
            /*flow control*/
            port->tx_throttled_cnt++;
            return ret;
        }
        pr_err("%s: write error:%d\n",
                __func__, ret);
        port->tomodem_drp_cnt++;
        dev_kfree_skb_any(skb);
        return ret;
    }
    port->to_modem++;

    return count;
}

static int atcmd_open(struct inode *inode, struct file *filp)
{
    atcmd_modem++;
    return 0;
}

static ssize_t atcmd_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
    struct atcmd_request *req;
    unsigned len=0;

    if (list_empty(&atcmd_pool))
    {
        if (filp->f_flags & O_NONBLOCK)
            return -EAGAIN;

#ifdef ATCMD_DBG
        pr_info("%s:1 sleeping?\n", __func__);
#endif
        interruptible_sleep_on(&atcmd_read_wait);
#ifdef ATCMD_DBG
        pr_info("%s:1 awake\n", __func__);
#endif

        if (signal_pending(current))
            return -ERESTARTSYS;

        mutex_lock(&atcmd_info.lock);
        req = list_first_entry(&atcmd_pool, struct atcmd_request, list);
    }
    else
    {
        mutex_lock(&atcmd_info.lock);
        req = list_first_entry(&atcmd_pool, struct atcmd_request, list);
        mutex_unlock(&atcmd_info.lock);

#ifdef ATCMD_DBG
        pr_info("%s:2 sleeping?\n", __func__);
#endif
        wait_event_interruptible(atcmd_read_wait, req->status);
#ifdef ATCMD_DBG
        pr_info("%s:2 awake\n", __func__);
#endif
        mutex_lock(&atcmd_info.lock);
    }

    list_del(&req->list);

    if (req->status != 1)
    {
        atcmd_free_req(req);
        goto out;
    }

    memcpy(buf, req->buf, req->length);

    len = req->length;

#ifdef ATCMD_DBG
    ATCMD_DBG_DATA(buf, len);
#endif

    atcmd_free_req(req);

out:
    mutex_unlock(&atcmd_info.lock);
    return len;
}

static ssize_t atcmd_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
    ssize_t size = 0;

    size = atcmd_write_tohost(buf, count);

    return size;
}

static int atcmd_release(struct inode *inodes, struct file *filp)
{
    atcmd_modem--;
    if (atcmd_modem < 0) atcmd_modem = 0;
    return 0;
}

void atcmd_connect(void *port)
{
    atcmd_info.port = (struct gdata_port *)port;
}

void atcmd_disconnect(void)
{
}

struct file_operations atcmd_fops =
{
    .owner      = THIS_MODULE,
    .read       = atcmd_read,
    .write      = atcmd_write,
    .open       = atcmd_open,
    .release    = atcmd_release,
};

static char *atcmd_dev_node(struct device *dev, mode_t *mode)
{
    *mode = 0666;
    return NULL;
}

static __init int atcmd_init(void)
{
    atcmd_major = register_chrdev(0, "ttyGS0", &atcmd_fops);
    if (atcmd_major < 0)
        return atcmd_major;

    pr_info("%s: ATCMD Handler /dev/ttyGS0 major=%d\n", __func__, atcmd_major);

    atcmd_class = class_create(THIS_MODULE, "ttyGS0");
    atcmd_class->devnode = atcmd_dev_node;
    atcmd_dev = device_create(atcmd_class, NULL, MKDEV(atcmd_major, 0), NULL, "ttyGS0");
    atcmd_dev->class = atcmd_class;

    mutex_init(&atcmd_info.lock);

    return 0;
}

static __exit void atcmd_exit(void)
{
    pr_info("%s\n", __func__);
    unregister_chrdev(atcmd_major, "ttyGS0");
}

module_init(atcmd_init);
module_exit(atcmd_exit);

MODULE_LICENSE("Dual BSD/GPL");
