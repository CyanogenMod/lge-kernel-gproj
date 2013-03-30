/*===========================================================================
 *	FILE:
 *	ddm_tty_forward.c
 *
 *	DESCRIPTION:

 *   
 *	Edit History:
 *	YYYY-MM-DD		who 						why
 *	-----------------------------------------------------------------------------
 *	2012-12-13	 	secheol.pyo@lge.com	  	Release
 *	2012-12-15		secheol.pyo@lge.com		added debugfs file.
 *==========================================================================================
*/

#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/crc-ccitt.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/time.h>
#include <linux/gpio.h>
#include <linux/debugfs.h>
#include <mach/ddm_bridge.h>
#include <mach/subsystem_restart.h>

#ifdef CONFIG_USB_LGE_DDM_TTY
#include "lg_ddm_tty.h"
#include <linux/sched.h>
#endif

#define DRIVER_DESC	"USB host ddm to tty forward driiver"
#define DRIVER_VERSION	"1.0"

#define RD_BUF_SIZE	4096
#define HSIC_DBG_CONNECTED	0
#define MAX_URB_BUF_SIZE 1536

/* For connecting ddm user process, 2012-12-20 */
//#define DDM_DRIRECT_CONN

struct ddm_dev *ddm_drv;
sddm_RX_Data *gP_ddm_rx = NULL;
sddm_PRx_Data gP_ddm_data = {0,};

sddm_TX_Data *gP_ddm_tx = NULL;
sddm_PTx_Data gP_ddm_Tx_data = {0,};

/* debugfs */
#if defined(CONFIG_DEBUG_FS)
#define TIMER_PERIODIC (5*HZ)
char tx_msg[MAX_URB_BUF_SIZE];
int tx_length = MAX_URB_BUF_SIZE;
struct timer_info{
	struct timer_list write_timer;
	unsigned long data;
};
struct timer_info *_debug_timer_info = NULL;

void ddm_write_test_timer_init(struct timer_info *timer, unsigned long expires);

void ddm_write_test_timer_cb(unsigned long arg){
	struct timer_info *ti = NULL;	
	pr_info("Queuing dummy data");
	queue_work(ddm_drv->ddm_wq, &ddm_drv->write_w);
	pr_info("Added Timer,  arg = %lu", arg);
	if(arg != 0){
		ti = (struct timer_info *)arg;
		ddm_write_test_timer_init(ti, TIMER_PERIODIC);
	}
}
void ddm_write_test_timer_stop(struct timer_info *timer){
	del_timer(&(_debug_timer_info->write_timer));
}
void ddm_write_test_timer_init(struct timer_info *timer, unsigned long expires){

	pr_info("Test Timer on");
	init_timer(&(timer->write_timer));
	timer->write_timer.expires = get_jiffies_64() + expires;
	timer->write_timer.data = (unsigned long)timer;
	timer->write_timer.function = ddm_write_test_timer_cb;
	add_timer(&(timer->write_timer));

	if(!ddm_drv->loopback_mode)
		ddm_drv->loopback_mode = 1;
}
#endif
/*******************************************************************************
 * * ddm_display_log
 *  * * DESC : This function print out the ddm message to Kernel log.
 *
 * @Param
 *   buff : Crash information from MDM.
 *
 ******************************************************************************/
#ifndef CONFIG_USB_LGE_DDM_TTY
static void 
ddm_process_read(void *buff, int size)
{
	gP_ddm_rx = (sddm_RX_Data *)buff;

	if (gP_ddm_rx->header.mode == 1)
	{
		gP_ddm_data.rx_data = *((sddm_RX_Data *)buff);
		gP_ddm_data.valid		= true;

		// To do Here.
	}
	else //test mode
	{
		pr_info("ddm bridge : 2012.12.20 I received some data....! \n");
		pr_info("ddm bridge : 2012.12.20 14:57  Data Size = %d , sizeof(buff) = %d, strlen = %d", size, sizeof(buff), strlen(buff));
#if 0
		gP_ddm_tx = (sddm_TX_Data *)buff;
		gP_ddm_Tx_data.tx_data = *((sddm_TX_Data *)buff);
		gP_ddm_Tx_data.valid		= true;

		return;
#endif

	}

}

#endif

/*******************************************************************************
 * ddm_read_complete_callback ( need to check )
 * 
 * DESC : This function is called when data transmission is completed through ddm channel.
 *
 * @Param
 ******************************************************************************/
static void
ddm_read_complete_callback(void *d, void *buf, int size, int actual)
{
	ddm_drv->in_busy_hsic_read = 0;

	if(actual > 0){
		if(!buf)
			pr_err("Out of memory \n");
		memcpy(lge_ddm_tty->read_buf, buf, actual);
		lge_ddm_tty->read_length = actual;

		lge_ddm_tty->set_logging = 1;
		wake_up_interruptible(&lge_ddm_tty->waitq);
	}
}


/*******************************************************************************
 * ddm_read_work
 * * DESC : This function print out the ddm message to Kernel log.
 *
 * @Param
 *   w : work queue struck.
 ******************************************************************************/
static void ddm_read_work(struct work_struct *w)
{
	struct ddm_dev *dev =
		container_of(w, struct ddm_dev, read_w);

	memset(dev->read_buf, 0, RD_BUF_SIZE);

	if(!ddm_drv->in_busy_hsic_read){
		int err_ret;
		ddm_drv->in_busy_hsic_read = 1;
		err_ret = ddm_bridge_read(dev->read_buf, RD_BUF_SIZE);
		if(err_ret){
			pr_err(" Error HSIC read from MDM , err : %d \n", err_ret);
		}
	}

}
static void ddm_write_work(struct work_struct *w)
{
#ifndef CONFIG_USB_LGE_DDM_TTY
	struct ddm_dev *dev = container_of(w, struct ddm_dev, write_w);

	memset(dev->write_buf, 0, RD_BUF_SIZE);

	pr_debug("2012.12.20 ddm_drv->loopback_mode = %d\n", ddm_drv->loopback_mode);
	if(ddm_drv->loopback_mode){
 		pr_debug("%s : tx_msg_length = %d", __func__, tx_length);
	}
	else {
		pr_debug("size gP_ddm_tx->msg = %d, len gP_ddm_tx->msg = %d", sizeof(gP_ddm_tx->msg), strlen(gP_ddm_tx->msg));
		memcpy(dev->write_buf, gP_ddm_tx->msg, sizeof(gP_ddm_tx->msg));
	}
	if(!ddm_drv->in_busy_hsic_write){
		int err_ret;
		pr_info("called ddm_bridge_write\n");		
		ddm_drv->in_busy_hsic_write = 1;
		err_ret = ddm_bridge_write(tx_msg, tx_length);
		if(err_ret){
			pr_err(" Error HSIC read from MDM , err : %d \n", err_ret);
		}
	}
#endif	
	pr_debug("called write queue work\n");
}
/*******************************************************************************
 * ddm_write_complete_callback ( need to check )
 * * DESC : This function is not called.
 *
 * @Param
 *   d : ddm device driver.
 ******************************************************************************/
static void
ddm_write_complete_callback(void *d, void *buf, int size, int actual)
{
	ddm_drv->in_busy_hsic_write = 0;
	if(actual > 0){
		if(!buf)
			pr_err("Out of memory \n");
	}
	pr_info("%s : is called write cb\n", __func__);
}

static int ddm_suspend(void *ctxt)
{
	if (ddm_drv->in_busy_hsic_write)
		return -EBUSY;

	ddm_drv->hsic_suspend = 1;

	return 0;
}

static void ddm_resume(void *ctxt)
{
	ddm_drv->hsic_suspend = 0;

	queue_work(ddm_drv->ddm_wq, &ddm_drv->read_w);
}

static int ddm_remove(struct platform_device *pdev)
{
	ddm_bridge_close();

	return 0;
}
/* operation from ddm channel to bridge */
static struct ddm_bridge_ops ddm_ops = {
	.ctxt = NULL,
	.read_complete_cb = ddm_read_complete_callback,
	.write_complete_cb = ddm_write_complete_callback,
	.suspend = ddm_suspend,
	.resume = ddm_resume,
};

#if defined(CONFIG_DEBUG_FS)
static ssize_t ddm_debugfs_write(struct file *file,
	const char __user *buf, size_t count, loff_t *ppos)
{
	struct timer_info *_debug_t_i;
	pr_info("Input = %d, Loopback Start. We will transmit dummy data to MDM", (int)*buf);

	_debug_t_i = _debug_timer_info;
	ddm_write_test_timer_init(_debug_t_i, TIMER_PERIODIC);

	return count;
}
#if 0 /* TBD */
static ssize_t ddm_debugfs_read(struct file *file,
	const char __user *buf, size_t count, loff_t *ppos)
{
	pr_info("TBD");
	return count;
}
#endif
const struct file_operations ddm_dbg_fops = {
#if 0
	.read = ddm_debugfs_read,
#endif
	.write = ddm_debugfs_write, 
};


static struct dentry *ddm_dbg_root;
static int ddm_debugfs_init(struct ddm_dev* debugfs_ddm_dev)
{
	struct dentry *ddm_dentry;
	struct timer_info *_debug_t_i;

	ddm_dbg_root = debugfs_create_dir("ddm_bridge", 0);
	if (!ddm_dbg_root || IS_ERR(ddm_dbg_root))
		return -ENODEV;

	ddm_dentry = debugfs_create_file("ddm_tx_test",
		0644,
		ddm_dbg_root, 0,
		&ddm_dbg_fops);

	if (!ddm_dentry) {
		debugfs_remove_recursive(ddm_dbg_root);
		return -ENODEV;
	}

	_debug_t_i = kmalloc(sizeof(struct timer_info), GFP_KERNEL);
	_debug_timer_info = _debug_t_i;
	return 0;
}

static void ddm_debugfs_exit(void)
{
	struct timer_info *_debug_t_i;

	debugfs_remove_recursive(ddm_dbg_root);
	_debug_t_i = _debug_timer_info;
	kfree(_debug_t_i);
}

#else
static void ddm_debugfs_init(void) { }
static void ddm_debugfs_exit(void) { }
#endif

#ifdef ddm_DRIRECT_CONN
/*******************************************************************************
 * ddm_read
 * 
 * DESC : This function read ddm Data and is called by system call from a user space process.
 *
 * @Param
 *   ??
 ******************************************************************************/
static int ddm_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{

	if(ddm_drv != NULL)
		queue_work(ddm_drv->ddm_wq, &ddm_drv->write_w);

	return 0;
}

/*******************************************************************************
 * ddm_read
 * 
 * DESC : This function read ddm Data and is called by system call from a user space process.
 *
 * @Param
 *   ??
 ******************************************************************************/
static int ddm_read(struct file *file, char __user *buf, size_t nbytes, loff_t *ppos)
{

	if(ddm_drv != NULL)
		queue_work(ddm_drv->ddm_wq, &ddm_drv->read_w);

	return 0;
}

/*******************************************************************************
 * ddm_open
 * 
 * DESC : This function opens ddm and is called by system call from a user space process.
 *
 * @Param
 *   ??
 ******************************************************************************/
static int ddm_open(struct inode *inode, struct file *file)
{
	int			ret = 0;

	ret = ddm_bridge_open(&ddm_ops);
	if (ret)
		pr_err("open failed: %d", ret);

	return ret;
}
#endif
/*******************************************************************************
 * ddm_probe
 * 
 * DESC : probe the ddm Channel
 *
 * @Param
 *   pdev : ddm platform device driver.
 ******************************************************************************/
static int ddm_probe(struct platform_device *pdev)
{
	struct ddm_dev	*dev = ddm_drv;
	int			ret = 0;
	
/* This function should work in SSR Level=3, Secheol.pyo@lge.com */
	ddm_drv->in_busy_hsic_read = 0;
	ddm_drv->loopback_mode = 0;
	
	ret = ddm_bridge_open(&ddm_ops);
	if (ret)
		pr_err("open failed: %d", ret);

	queue_work(ddm_drv->ddm_wq, &dev->read_w);

	ddm_debugfs_init(dev);	

	return ret;
}

#ifdef ddm_DRIRECT_CONN
static const struct file_operations ddm_fops = {
	.owner		= THIS_MODULE,
	.open		= ddm_open,
	.read		= ddm_read,
	.write		= ddm_write,
};
#endif

static struct platform_driver ddm = {
	.remove = ddm_remove,
	.probe	= ddm_probe,
	.driver = {
		.name = "ddm",
		.owner = THIS_MODULE,
	},
};

/*******************************************************************************
 * ddm_init
 * 
 * DESC : initialize the ddm Channel.
 *
 * @Param
 ******************************************************************************/
static int __init ddm_init(void)
{
	struct ddm_dev	*dev;
	int ret = 0;

	pr_info("%s\n", __func__);

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	ddm_drv = dev;
	
	dev->read_buf = kmalloc(RD_BUF_SIZE, GFP_KERNEL);
	dev->write_buf = kmalloc(RD_BUF_SIZE, GFP_KERNEL);
	
	if (!dev->read_buf) {
		pr_err("%s: unable to allocate read buffer\n", __func__);
		kfree(dev);
		return -ENOMEM;
	}
	if (!dev->write_buf) {
			pr_err("%s: unable to allocate write buffer\n", __func__);
		kfree(dev);
		return -ENOMEM;
	}

	dev->ops.ctxt = dev;
	ddm_drv->ddm_wq = create_singlethread_workqueue("ddm_wq");
#ifdef LG_FW_WAIT_QUEUE	
	init_waitqueue_head(&ddm_drv->read_wait_queue);
#endif
	INIT_WORK(&ddm_drv->read_w, ddm_read_work);
	INIT_WORK(&ddm_drv->write_w, ddm_write_work);

	ret = platform_driver_register(&ddm);
	if (ret)
		pr_err("%s: platform driver %s register failed %d\n",
				__func__, ddm.driver.name, ret);


	return ret;
}

static void __exit ddm_exit(void)
{
	struct ddm_dev *dev = ddm_drv;

	pr_info("%s:\n", __func__);

	if (test_bit(HSIC_DBG_CONNECTED, &dev->flags))
		ddm_bridge_close();

	ddm_debugfs_exit();

	kfree(dev->read_buf);
	kfree(dev->write_buf);
	kfree(dev);

}

module_init(ddm_init);
module_exit(ddm_exit);

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL v2");

