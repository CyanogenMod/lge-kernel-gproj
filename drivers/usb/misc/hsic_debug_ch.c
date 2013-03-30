/*===========================================================================
 *	FILE:
 *	hsic_debug_ch.c
 *
 *	DESCRIPTION:
 *	This module was created for MDM debugging. 
 *   If EMS feature enabled on MDM9x15, MDM9x15 sends some debugging message when mdm crash occured.
 *   APQ8064 received this message by using this module and print out debugging message before kernel panic.
 *   Basically this module operates only SSR level 1, and use hsic sysmon channel.
 *   
 *	Edit History:
 *	YYYY-MM-DD		who 						why
 *	-----------------------------------------------------------------------------
 *	2012-07-25	 	wj1208.jo@lge.com	  	Make new file
 *
 *==========================================================================================
 */

#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/crc-ccitt.h>

#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/gpio.h>

#include <mach/hsic_debug_ch.h>
#include <linux/usb.h>
#include <mach/subsystem_restart.h>
#define LGE_FW_MDM_FATAL_NO_HIGH
#ifdef LGE_FW_MDM_FATAL_NO_HIGH
#define MDM2AP_ERRFATAL			19
#define EXTERNAL_MODEM "external_modem"
struct timer_list fatal_timer;
#endif

#define DRIVER_DESC	"USB host hsic debug channel driiver test"
#define DRIVER_VERSION	"1.0"

#define EMS_RD_BUF_SIZE	1024

#define HSIC_DEBUG_CH_INFO
#ifdef HSIC_DEBUG_CH_INFO
#define LTE_INFO(fmt,args...)printk("[%s] "fmt,__FUNCTION__, ## args)
#else
#define LTE_INFO(fmt, args...)
#endif

#ifdef EMS_RESET_SOC_ONLY_CHECK
static int ssr_level;
#endif

struct ems_data {
	char *read_buf;
	struct work_struct read_w;
	struct workqueue_struct *hsic_debug_wq;
	int hsic_suspend;
	int in_busy_hsic_read;
};
sEMS_RX_Data *gP_ems_rx = NULL;
sEMS_PRx_Data gP_ems_data = {0,}; //add for Display

const char *EMS_ARCH[] ={
				"NONE(0)",
				"A(1)",
				"M(2)" 
			};

struct ems_data emsData;
struct usb_device* hs_udev;
struct usb_interface* hs_ifc;
__u8 hs_in_epaddr;
struct usb_anchor	submitted;
int sysmon_hsic_in_busy = 0; //use in hsic_sysmon.c


/*******************************************************************************
 *
 * sysmon_hsic_debug_read_work
 * 
 *
 * DESC : This function is called by queue_work.
 *
 *
 ******************************************************************************/
static void sysmon_hsic_debug_read_work(struct work_struct *w)
{

#ifdef LG_FW_HSIC_EMS_DEBUG
		printk("[%s]++\n",__func__);
#endif

	if (emsData.read_buf != NULL)
		memset(emsData.read_buf, 0, EMS_RD_BUF_SIZE);

	if(!emsData.in_busy_hsic_read)
	{
		//trying to read
		int err_ret;
		emsData.in_busy_hsic_read = 1;
		err_ret = sysmon_hsic_debug_bridge_read(emsData.read_buf, EMS_RD_BUF_SIZE);
		if(err_ret){
			pr_err(" Error HSIC read from MDM , err : %d \n", err_ret);
			emsData.in_busy_hsic_read = 0;
		}
	}
	else {
#ifdef LG_FW_HSIC_EMS_DEBUG		
		printk("[%s] :  read failed (in_busy_hsic_read = 1 or dev is NULL)\n", __func__);
#endif
	}
	
	
#ifdef LG_FW_HSIC_EMS_DEBUG
				printk("[%s]--\n",__func__);
#endif
}


/*******************************************************************************
 *
 * sysmon_hsic_debug_disconnect
 * 
 *
 * DESC : This function is called when sysmon probe.
 *
 *
 ******************************************************************************/
void sysmon_hsic_debug_init(struct usb_device *udev_from_hs, struct usb_interface *ifc_from_hs, __u8 in_epaddr)
{
#ifdef LG_FW_HSIC_EMS_DEBUG
		printk("[%s]++\n",__func__);
#endif

	hs_udev = udev_from_hs;
	hs_ifc = ifc_from_hs;
	hs_in_epaddr = in_epaddr;	
	
	
	emsData.hsic_suspend = 0;
	emsData.in_busy_hsic_read = 0;

	emsData.read_buf = kmalloc(EMS_RD_BUF_SIZE, GFP_KERNEL);
	if(!emsData.read_buf)
		pr_err("[%s]kmalloc fail!\n", __func__);
	
	device_set_wakeup_enable(&hs_udev->dev,1);

	emsData.hsic_debug_wq = create_singlethread_workqueue("hsic_debug_wq");
	INIT_WORK(&emsData.read_w, sysmon_hsic_debug_read_work);
	
#ifdef EMS_RESET_SOC_ONLY_CHECK
	ssr_level = get_restart_level();
	if (ssr_level == 1) {
#endif

	if(!sysmon_hsic_in_busy)
		queue_work(emsData.hsic_debug_wq, &emsData.read_w);
	else
		pr_err("[%s]hsic_sysmon is busy!\n",__func__);
	
#ifdef EMS_RESET_SOC_ONLY_CHECK
		}
#endif		
	
#ifdef LG_FW_HSIC_EMS_DEBUG
		printk("[%s]--\n",__func__);
#endif

}
EXPORT_SYMBOL(sysmon_hsic_debug_init);



/*******************************************************************************
 *
 * sysmon_hsic_debug_disconnect
 * 
 *
 * DESC : This function is called when sysmon disconnect.
 *
 *
 ******************************************************************************/
void sysmon_hsic_debug_disconnect(void)
{
#ifdef LG_FW_HSIC_EMS_DEBUG
			printk("[%s]++\n",__func__);
#endif

	hs_udev = NULL;
	hs_ifc = NULL;
	hs_in_epaddr = 0;
	emsData.hsic_suspend = 0;
	emsData.in_busy_hsic_read = 0;

	if ( emsData.read_buf != NULL)
	{
		kfree(emsData.read_buf);
		emsData.read_buf = NULL;		
	}

	destroy_workqueue(emsData.hsic_debug_wq);

#ifdef LG_FW_HSIC_EMS_DEBUG
			printk("[%s]--\n",__func__);
#endif	
}
EXPORT_SYMBOL(sysmon_hsic_debug_disconnect);


/*******************************************************************************
 *
 * sysmon_hsic_debug_suspend
 * 
 *
 * DESC : Set hsic_suspend value of ems_dev and queue_work if sysmon resumed.
 *
 * @Param
 *   hsic_suspend : hsic_suspend value. (0 : resume, 1 : suspend)
 *
 ******************************************************************************/
void sysmon_hsic_debug_suspend(int hsic_suspend)
{

	emsData.hsic_suspend = hsic_suspend;

#ifdef LG_FW_HSIC_EMS_DEBUG
	if(hsic_suspend)
		printk("[%s] hsic_sysmon suspend called! = %d\n",__func__, hsic_suspend);
	else
		printk("[%s] hsic_sysmon resume called! = %d\n",__func__, hsic_suspend);
#endif
	
	if(!hsic_suspend){
	//when resume called

#ifdef EMS_RESET_SOC_ONLY_CHECK
	ssr_level = get_restart_level();
	if(ssr_level == 1) {
#endif
		//doing read work
		if(!sysmon_hsic_in_busy)
			queue_work(emsData.hsic_debug_wq, &emsData.read_w);
		else
			pr_err("[%s]hsic_sysmon is busy!\n",__func__);

#ifdef EMS_RESET_SOC_ONLY_CHECK
		}
#endif


#ifdef LG_FW_HSIC_EMS_DEBUG
		printk("[%s] queue_work done \n", __func__);
#endif

	}else{
	//when suspend called
	//usb_kill_anchored_urbs(&submitted);
	}
}
EXPORT_SYMBOL(sysmon_hsic_debug_suspend);


/*******************************************************************************
 *
 * sysmon_hsic_debug_in_busy
 * 
 *
 * DESC : Set in_busy_hsic_read value of ems_dev.
 *
 * @Param
 *   in_busy_hsic_read : in_busy_hsic_read value.
 *
 ******************************************************************************/
void sysmon_hsic_debug_in_busy(int in_busy_hsic_read)
{
#ifdef LG_FW_HSIC_EMS_DEBUG
	printk("[%s] in_busy_hsic_read = %d\n", __func__, in_busy_hsic_read);
#endif
	emsData.in_busy_hsic_read = in_busy_hsic_read;
}
EXPORT_SYMBOL(sysmon_hsic_debug_in_busy);



/*******************************************************************************
 *
 * sysmon_hsic_debug_bridge_read
 * 
 *
 * DESC : This function is callback function of sysmon_hsic_debug_read_complete
 *
 * @Param
 *   buf : Crash information from MDM.
 *   size  : length of data
 *   actual  : length of actual data
 *
 ******************************************************************************/
static void
sysmon_hsic_debug_read_complete_callback(void *buf, int size, int actual)
{
	emsData.in_busy_hsic_read = 0;

	if(actual > 0){
		if(!buf)
			pr_err("Out of memory \n");
		
		hsic_debug_display_log((void *)buf);
		
		if (emsData.read_buf)
			memset(emsData.read_buf, 0, EMS_RD_BUF_SIZE);
	}
	
	if(!emsData.hsic_suspend){
		//ems_dev still resume state, so trying to queue_work again
#ifdef LG_FW_HSIC_EMS_DEBUG
		printk("[%s] ems_dev still resume state, so trying to queue_work again! \n", __func__);
#endif
		if(!sysmon_hsic_in_busy)
			queue_work(emsData.hsic_debug_wq, &emsData.read_w);
		else
			pr_err("[%s]hsic_sysmon is busy!\n",__func__);
	}
}

/*******************************************************************************
 *
 * sysmon_hsic_debug_bridge_read_cb
 * 
 *
 * DESC : This function is callback function of sysmon_hsic_debug_bridge_read
 *
 *
 ******************************************************************************/
static void sysmon_hsic_debug_bridge_read_cb(struct urb *urb)
{
	dev_dbg(&hs_udev->dev, "%s: status:%d actual:%d\n", __func__,
			urb->status, urb->actual_length);

#ifdef LG_FW_HSIC_EMS_DEBUG
		printk("[%s] urb->actual_length = %d\n", __func__, urb->actual_length );
#endif

	if (urb->status == -EPROTO) {		
		return;
	}

	sysmon_hsic_debug_read_complete_callback(
		urb->transfer_buffer,
		urb->transfer_buffer_length,
		urb->status < 0 ? urb->status : urb->actual_length);

}

/*******************************************************************************
 *
 * sysmon_hsic_debug_bridge_read
 * 
 *
 * DESC : This function is called when received from MDM.
 *
 * @Param
 *   data : Crash information from MDM.
 *   size  : length of data
 *
 ******************************************************************************/
int sysmon_hsic_debug_bridge_read(char *data, int size)
{
	struct urb		*urb = NULL;
	unsigned int		pipe;
	int			ret;

	dev_dbg(&hs_udev->dev, "%s:\n", __func__);	

#ifdef LG_FW_HSIC_EMS_DEBUG
	printk("[%s]called! EMS_RD_BUF_SIZE buf size = %d\n",__func__,size);
#endif

	if (!size) {
		dev_err(&hs_udev->dev, "invalid size:%d\n", size);
		return -EINVAL;
	}

	if (!hs_ifc) {
		dev_err(&hs_udev->dev, "device is disconnected\n");
		return -ENODEV;
	}


	urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!urb) {
		dev_err(&hs_udev->dev, "unable to allocate urb\n");
		return -ENOMEM;
	}

	pipe = usb_rcvbulkpipe(hs_udev, hs_in_epaddr);
	usb_fill_bulk_urb(urb, hs_udev, pipe, data, size,
				sysmon_hsic_debug_bridge_read_cb, NULL);

	usb_anchor_urb(urb, &submitted);

	ret = usb_submit_urb(urb, GFP_KERNEL);
	if (ret) {
		dev_err(&hs_udev->dev, "submitting urb failed err:%d\n", ret);
		usb_unanchor_urb(urb);
		usb_free_urb(urb);
		return ret;
	}

	usb_free_urb(urb);

#ifdef LG_FW_HSIC_EMS_DEBUG
	printk("[%s]--\n",__func__);
#endif

	return 0;

}
EXPORT_SYMBOL(sysmon_hsic_debug_bridge_read);


#ifdef	LGE_FW_MDM_FATAL_NO_HIGH
static void hsic_debug_mdm_fatal_time_func(unsigned long arg)
{
	del_timer(&fatal_timer);
 	if (gpio_get_value(MDM2AP_ERRFATAL) != 1)
	{
		LTE_INFO("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");	
		LTE_INFO("EMS sent from MDM, But MDM error fatal gpio doesn't go to high within 10 seconds!!!\n");	
		subsystem_restart(EXTERNAL_MODEM);
 	}

}
#endif
/*******************************************************************************
 *
 * hsic_debug_display_log
 * 
 *
 * DESC : This function print out the EMS message to Kernel log.
 *
 * @Param
 *   buff : Crash information from MDM.
 *
 ******************************************************************************/
void hsic_debug_display_log(void *buff)
{
	gP_ems_rx = (sEMS_RX_Data *)buff;

	if (gP_ems_rx->header.mode == 1) // EMS mode
	{
		LTE_INFO("[EMS] ---------------------------------------------------------\n");	
		LTE_INFO("[EMS] [ Assert L : %s ]  File: %s, Line: %d\n", EMS_ARCH[gP_ems_rx->header.arch_type], gP_ems_rx->filename, gP_ems_rx->line);	
		LTE_INFO("[EMS] Error Message: %s \n", gP_ems_rx->msg);
		LTE_INFO("[EMS] SW Ver: %s \n", gP_ems_rx->sw_ver);
		LTE_INFO("[EMS] ---------------------------------------------------------\n");	
		gP_ems_data.rx_data = *((sEMS_RX_Data *)buff); //add for Display
		gP_ems_data.valid		= true; //add for Display
	}
	else
	{
		LTE_INFO("HSIC_DEBUG_CH : I received some data but not EMS log \n");
		return;
	}

#ifdef	LGE_FW_MDM_FATAL_NO_HIGH
//This feature should be enabled when subsystem restart level is RESET_SOC.
	if (get_restart_level() == RESET_SOC)
	{
		init_timer(&fatal_timer);
		fatal_timer.expires = 10*HZ + jiffies;  // 10 second 
		fatal_timer.data = 1;
		fatal_timer.function = &hsic_debug_mdm_fatal_time_func; /* timer handler */
		add_timer(&fatal_timer);
	}
#endif
}


static int __init hsic_debug_init(void)
{
	int ret = 0;
	pr_info("%s:\n", __func__);

	hs_udev = NULL;
	hs_ifc = NULL;
	hs_in_epaddr = 0;
	init_usb_anchor(&submitted);

	return ret;
}

static void __exit hsic_debug_exit(void)
{
	pr_info("%s:\n", __func__);

	hs_udev = NULL;
	hs_ifc = NULL;
	hs_in_epaddr = 0;
	usb_kill_anchored_urbs(&submitted);
}

module_init(hsic_debug_init);
module_exit(hsic_debug_exit);

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL v2");

