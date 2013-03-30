/*
 * HSIC Debug Module for EMS, created by Secheol.Pyo.
 * 2012-3-17
 * 
 * Secheol Pyo <secheol.pyo@lge.com>
 *
 *
 *  HISTORY 
 * 2012-04-05, Release.
 * 2012-04-12, suspend-resume
*/


#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kref.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/usb.h>
#include <mach/hsic_debug_bridge.h>

/* Driver Description OK */
#define DRIVER_DESC	"USB host HSIC DEBUG CHANNEL"
#define DRIVER_VERSION	"1.0"

/* EMS bridge structure */
struct hsic_debug_bridge {
	struct usb_device	*udev;
	struct usb_interface	*ifc;
	struct usb_anchor	submitted;
	__u8			in_epAddr;
	__u8			out_epAddr;
	int			err;
	struct kref		kref;
	struct hsic_debug_bridge_ops	*ops;
	struct platform_device	*pdev;
};

static struct hsic_debug_bridge *ems_dev;


/*******************************************************************************
 *
 * hsic_debug_bridge_open
 * 
 *
 * DESC : This function open the EMS_brigde channel.
 *
 * @Param
 *   ops : callback from EMS channel
 *
 ******************************************************************************/
int hsic_debug_bridge_open(struct hsic_debug_bridge_ops *ops)
{
	struct hsic_debug_bridge	*dev = ems_dev;

	printk("[%s] ++\n",__func__);

	if (!dev) {
		err("dev is null");
		return -ENODEV;
	}

	dev->ops = ops;
	dev->err = 0;

	printk("[%s] --\n",__func__);

	return 0;
}
EXPORT_SYMBOL(hsic_debug_bridge_open);

void hsic_debug_bridge_close(void)
{
	struct hsic_debug_bridge	*dev = ems_dev;

	dev_dbg(&dev->udev->dev, "%s:\n", __func__);

	usb_kill_anchored_urbs(&dev->submitted);

	dev->ops = 0;
}
EXPORT_SYMBOL(hsic_debug_bridge_close);
/*******************************************************************************
 *
 * hsic_debug_bridge_read_cb
 * 
 *
 * DESC : This function is called when received from MDM.
 *
 ******************************************************************************/
static void hsic_debug_bridge_read_cb(struct urb *urb)
{
	struct hsic_debug_bridge	*dev = urb->context;
	struct hsic_debug_bridge_ops	*cbs = dev->ops;
	
	dev_dbg(&dev->udev->dev, "%s: status:%d actual:%d\n", __func__,
			urb->status, urb->actual_length);

	if (urb->status == -EPROTO) {
		/* save error so that subsequent read/write returns ESHUTDOWN */
		dev->err = urb->status;
		return;
	}
#ifdef LG_FW_HSIC_EMS_DEBUG
	printk("[%s] actual_length = %d\n", __func__, urb->actual_length );
#endif
	cbs->read_complete_cb(cbs->ctxt,
						urb->transfer_buffer,
						urb->transfer_buffer_length,
						urb->status < 0 ? urb->status : urb->actual_length);

}

/*******************************************************************************
 *
 * hsic_debug_bridge_read
 * 
 * DESC : This function is called when received from MDM.
 *
 * @Param
 *    data : data buffer from MDM 
 *    size : size to receive data.
 *
 ******************************************************************************/
int hsic_debug_bridge_read(char *data, int size)
{
	struct urb		*urb = NULL;
	unsigned int		pipe;
	struct hsic_debug_bridge	*dev = ems_dev;
	int			ret;

	dev_dbg(&dev->udev->dev, "%s:\n", __func__);

	if (!size) {
		dev_err(&dev->udev->dev, "invalid size:%d\n", size);
		return -EINVAL;
	}

	if (!dev->ifc) {
		dev_err(&dev->udev->dev, "device is disconnected\n");
		return -ENODEV;
	}

	/* if there was a previous unrecoverable error, just quit */
	if (dev->err)
		return -ESHUTDOWN;

	urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!urb) {
		dev_err(&dev->udev->dev, "unable to allocate urb\n");
		return -ENOMEM;
	}

	pipe = usb_rcvbulkpipe(dev->udev, dev->in_epAddr);
	usb_fill_bulk_urb(urb, dev->udev, pipe, data, size,
				hsic_debug_bridge_read_cb, dev);
	usb_anchor_urb(urb, &dev->submitted);

	ret = usb_submit_urb(urb, GFP_KERNEL);
	if (ret) {
		dev_err(&dev->udev->dev, "submitting urb failed err:%d\n", ret);
		usb_unanchor_urb(urb);
		usb_free_urb(urb);
		return ret;
	}

	usb_free_urb(urb);

	return 0;
}
EXPORT_SYMBOL(hsic_debug_bridge_read);

/*******************************************************************************
 *
 * hsic_debug_bridge_write_cb
 * 
 * DESC : This function is not used at all.
 *
 ******************************************************************************/
static void hsic_debug_bridge_write_cb(void)
{
	printk("do not support this function\n");
}

/*******************************************************************************
 *
 * hsic_debug_bridge_write
 * 
 * DESC : This function is not used at all.
 *
 ******************************************************************************/
int hsic_debug_bridge_write(char *data, int size)
{
	hsic_debug_bridge_write_cb();
	return 0;
}
EXPORT_SYMBOL(hsic_debug_bridge_write);

static void hsic_debug_bridge_delete(struct kref *kref)
{
	struct hsic_debug_bridge *dev =
		container_of(kref, struct hsic_debug_bridge, kref);

	usb_put_dev(dev->udev);
	ems_dev = 0;
	kfree(dev);
}

/*******************************************************************************
 *
 * hsic_debug_bridge_probe
 * 
 * DESC : This function is called after HSIC_DEBUG_CH is initiated.
 *
 ******************************************************************************/
static int
hsic_debug_bridge_probe(struct usb_interface *ifc, const struct usb_device_id *id)
{
	struct hsic_debug_bridge		*dev;
	struct usb_host_interface	*ifc_desc;
	struct usb_endpoint_descriptor	*ep_desc;
	int				i;
	int				ret = -ENOMEM;
	__u8				ifc_num;


	dbg("%s: id:%lu", __func__, id->driver_info);

	ifc_num = ifc->cur_altsetting->desc.bInterfaceNumber;

	/* is this interface supported ? */
	if (ifc_num != id->driver_info)
		return -ENODEV;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		pr_err("%s: unable to allocate dev\n", __func__);
		return -ENOMEM;
	}
	dev->pdev = platform_device_alloc("hsic_debug", -1);
	if (!dev->pdev) {
		pr_err("%s: unable to allocate platform device\n", __func__);
		kfree(dev);
		return -ENOMEM;
	}
	ems_dev = dev;

	dev->udev = usb_get_dev(interface_to_usbdev(ifc));
	dev->ifc = ifc;
#ifdef LG_FW_REMOTE_WAKEUP
	device_set_wakeup_enable(&dev->udev->dev, 1);
#else
	device_set_wakeup_enable(&dev->udev->dev, 0);
#endif	
	kref_init(&dev->kref);
	init_usb_anchor(&dev->submitted);

	ifc_desc = ifc->cur_altsetting;

	for (i = 0; i < ifc_desc->desc.bNumEndpoints; i++) {
#ifdef LG_FW_HSIC_EMS_DEBUG		
		printk("[%s] ifc_desc->desc.bNumEndpoints = %d\n", __func__,ifc_desc->desc.bNumEndpoints); // code for TEST
#endif
		ep_desc = &ifc_desc->endpoint[i].desc;
		if (!dev->in_epAddr && usb_endpoint_is_bulk_in(ep_desc))
			dev->in_epAddr = ep_desc->bEndpointAddress;
/*		
		do not use out endpoint, secheol.pyo. 

		if (!dev->out_epAddr && usb_endpoint_is_bulk_out(ep_desc))
			dev->out_epAddr = ep_desc->bEndpointAddress;
*/			
#ifdef LG_FW_HSIC_EMS_DEBUG
		printk("[%s] dev->in_epAddr = %d\n", __func__, dev->in_epAddr); // code for TEST
#endif		
	}

#if 0 /* do not use out endpoint, secheol.pyo. */
	if (!(dev->in_epAddr && dev->out_epAddr)) {
#else
	if (!(dev->in_epAddr)) {
#endif
		err("could not find bulk in endpoints");
		ret = -ENODEV;
		goto error;
	}

	usb_set_intfdata(ifc, dev);

	platform_device_add(dev->pdev);

	dev_dbg(&dev->udev->dev, "%s: complete\n", __func__);
	
	return 0;

error:
	if (dev)
		kref_put(&dev->kref, hsic_debug_bridge_delete);

	return ret;

}

static void hsic_debug_bridge_disconnect(struct usb_interface *ifc)
{
	struct hsic_debug_bridge	*dev = usb_get_intfdata(ifc);

	dev_dbg(&dev->udev->dev, "%s:\n", __func__);

#ifdef LG_FW_REMOTE_WAKEUP
	device_set_wakeup_enable(&dev->udev->dev, 0);
#endif
	platform_device_del(dev->pdev);
	kref_put(&dev->kref, hsic_debug_bridge_delete);
	usb_set_intfdata(ifc, NULL);
}
static int hsic_debug_bridge_suspend(struct usb_interface *ifc, pm_message_t message)
{
	struct hsic_debug_bridge	*dev = usb_get_intfdata(ifc);
	struct hsic_debug_bridge_ops	*cbs = dev->ops;
	int ret = 0;

	if (cbs && cbs->suspend) {
		ret = cbs->suspend(cbs->ctxt);
		if (ret) {
			dev_dbg(&dev->udev->dev,
				"%s: hsic_degug veto'd suspend\n", __func__);
			return ret;
		}

		usb_kill_anchored_urbs(&dev->submitted);
	}

	return ret;
}

static int hsic_debug_bridge_resume(struct usb_interface *ifc)
{
	struct hsic_debug_bridge	*dev = usb_get_intfdata(ifc);
	struct hsic_debug_bridge_ops	*cbs = dev->ops;

	if (cbs && cbs->resume){
		cbs->resume(cbs->ctxt);
	}

	return 0;
}

/* EMS id table need to check. We supported usb_id until 0x9048 */
#ifdef CONFIG_LGE_EMS_CH
#define HSIC_VALID_INTERFACE_NUM	3
#else
#define HSIC_VALID_INTERFACE_NUM	1
#endif
static const struct usb_device_id hsic_debug_bridge_ids[] = {
	{ USB_DEVICE(0x5c6, 0x9001),
	.driver_info = HSIC_VALID_INTERFACE_NUM, }, 
	{ USB_DEVICE(0x5c6, 0x9034),
	.driver_info = HSIC_VALID_INTERFACE_NUM, },
	{ USB_DEVICE(0x5c6, 0x9048),
	.driver_info = HSIC_VALID_INTERFACE_NUM, },
	{ USB_DEVICE(0x5c6, 0x904c),		/* Added for CSVT */
	.driver_info = HSIC_VALID_INTERFACE_NUM, },
	{} /* terminating entry */
};
MODULE_DEVICE_TABLE(usb, hsic_debug_bridge_ids);

/* EMS driver struct */

static struct usb_driver hsic_debug_bridge_driver = {
	.name =		"hsic_debug_bridge",
	.probe =	hsic_debug_bridge_probe,
	.disconnect =	hsic_debug_bridge_disconnect,
	.suspend = hsic_debug_bridge_suspend,
	.resume = hsic_debug_bridge_resume,
	.id_table =	hsic_debug_bridge_ids,
	.supports_autosuspend = 1,
};

/*******************************************************************************
 *
 * hsic_debug_bridge_init
 * 
 * DESC : 
 *
 ******************************************************************************/
static int __init hsic_debug_bridge_init(void)
{
	int ret;


	ret = usb_register(&hsic_debug_bridge_driver);
	if (ret) {
		err("%s: unable to register debug driver",
				__func__);
		return ret;
	}

	return 0;
}
/*******************************************************************************
 *
 * hsic_debug_bridge_exit
 * 
 * DESC : 
 *
 ******************************************************************************/
static void __exit hsic_debug_bridge_exit(void)
{
	usb_deregister(&hsic_debug_bridge_driver);
}

module_init(hsic_debug_bridge_init);
module_exit(hsic_debug_bridge_exit);

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL v2");

