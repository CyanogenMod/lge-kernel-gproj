/*===========================================================================
 *	FILE:
 *	ddm_bridge.c
 *
 *	DESCRIPTION:

 *   
 *	Edit History:
 *	YYYY-MM-DD		who 						why
 *	-----------------------------------------------------------------------------
 *	2012-12-24	 	secheol.pyo@lge.com	  	Release
 *
 *==========================================================================================
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
#include <mach/ddm_bridge.h>
#include <linux/ratelimit.h>


/* Driver Description OK */
#define DRIVER_DESC	"USB host HSIC DDM BRIDGE"
#define DRIVER_VERSION	"1.0"

/* ddm bridge structure */
struct ddm_bridge {
	struct usb_device	*udev;
	struct usb_interface	*ifc;
	struct usb_anchor	submitted;
	struct kref		kref;
	struct ddm_bridge_ops	*ops;
	struct platform_device	*pdev;
	struct mutex		ddm_mutex;

	int 	pending_writes;
	__u8			in_epAddr;
	__u8			out_epAddr;
	int			err;
};

static struct ddm_bridge *ddm_bridge_dev;


/*******************************************************************************
 *
 * ddm_bridge_open
 * 
 *
 * DESC : This function open the ddm_brigde channel.
 *
 * @Param
 *   ops : callback from ddm channel
 *
 ******************************************************************************/
int ddm_bridge_open(struct ddm_bridge_ops *ops)
{
	struct ddm_bridge	*dev = ddm_bridge_dev;

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
EXPORT_SYMBOL(ddm_bridge_open);


/*******************************************************************************
 *
 * ddm_bridge_open_stub
 * 
 *
 * DESC : This function open the ddm_brigde fake channel.
 *
 * @Param
 *   ops : callback from ddm channel
 *
 ******************************************************************************/
int ddm_bridge_open_stub(struct ddm_bridge_ops *ops)
{
	struct ddm_bridge	*dev = ddm_bridge_dev;

	printk("[%s] ++\n",__func__);
	if (!dev) {
		err("dev is null");
		return -ENODEV;
	}
	printk("[%s] --\n",__func__);

	return 0;
}
EXPORT_SYMBOL(ddm_bridge_open_stub);

void ddm_bridge_close(void)
{
	struct ddm_bridge	*dev = ddm_bridge_dev;

	dev_dbg(&dev->udev->dev, "%s:\n", __func__);

	usb_kill_anchored_urbs(&dev->submitted);

	dev->ops = 0;
}
EXPORT_SYMBOL(ddm_bridge_close);
/*******************************************************************************
 *
 * ddm_bridge_read_cb
 * 
 *
 * DESC : This function is called when received from MDM.
 *
 ******************************************************************************/
static void ddm_bridge_read_cb(struct urb *urb)
{
	struct ddm_bridge	*dev = urb->context;
	struct ddm_bridge_ops	*cbs = dev->ops;
	
	dev_info(&dev->udev->dev, "%s: status:%d actual:%d\n", __func__,
			urb->status, urb->actual_length);

	if (urb->status == -EPROTO) {
		/* save error so that subsequent read/write returns ESHUTDOWN */
		dev->err = urb->status;
		return;
	}
#ifdef LG_FW_HSIC_DDM_DEBUG
	printk("[%s] 2012.12.13 actual_length = %d\n", __func__, urb->actual_length );
#endif
	cbs->read_complete_cb(cbs->ctxt,
						urb->transfer_buffer,
						urb->transfer_buffer_length,
						urb->status < 0 ? urb->status : urb->actual_length);

}

/*******************************************************************************
 *
 * ddm_bridge_read
 * 
 * DESC : This function is called when received from MDM.
 *
 * @Param
 *    data : data buffer from MDM 
 *    size : size to receive data.
 *
 ******************************************************************************/
int ddm_bridge_read(char *data, int size)
{
	struct urb		*urb = NULL;
	unsigned int		pipe;
	struct ddm_bridge	*dev = ddm_bridge_dev;
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
	mutex_lock(&dev->ddm_mutex);

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
				ddm_bridge_read_cb, dev);
	usb_anchor_urb(urb, &dev->submitted);

	ret = usb_submit_urb(urb, GFP_KERNEL);
	if (ret) {
		dev_err(&dev->udev->dev, "submitting urb failed err:%d\n", ret);
		usb_unanchor_urb(urb);
		usb_free_urb(urb);
		return ret;
	}

	usb_free_urb(urb);
	mutex_unlock(&dev->ddm_mutex);
	
	return 0;
}
EXPORT_SYMBOL(ddm_bridge_read);

/*******************************************************************************
 *
 * ddm_bridge_write_cb
 * 
 * DESC : This function is not used at all.
 *
 ******************************************************************************/
static void ddm_bridge_write_cb(struct urb *urb)
{
	struct ddm_bridge	*dev = urb->context;
	struct ddm_bridge_ops	*cbs = dev->ops;

#ifdef LG_FW_HSIC_DDM_DEBUG
	pr_info("urb->transfer_buffer_length = %d, urb->actual_length = %d bytes\n", urb->transfer_buffer_length , urb->actual_length);
#endif

	dev_dbg(&dev->ifc->dev, "%s:\n", __func__);
	usb_autopm_put_interface_async(dev->ifc);

	/* save error so that subsequent read/write returns ENODEV */
	if (urb->status == -EPROTO)
		dev->err = urb->status;

	if (cbs && cbs->write_complete_cb)
		cbs->write_complete_cb(cbs->ctxt,
			urb->transfer_buffer,
			urb->transfer_buffer_length,
			urb->status < 0 ? urb->status : urb->actual_length);

	//dev->bytes_to_mdm += urb->actual_length;
	dev->pending_writes--;
//	kref_put(&dev->kref, ddm_bridge_delete);
}

/*******************************************************************************
 *
 * ddm_bridge_write
 * 
 * DESC : This function is not used at all.
 *
 ******************************************************************************/
int ddm_bridge_write(char *data, int size)
{
	struct urb		*urb = NULL;
	unsigned int		pipe;
	struct ddm_bridge	*dev = ddm_bridge_dev;
	int			ret;

#ifdef LG_FW_HSIC_DDM_DEBUG
	pr_info("writing %d bytes", size);
#endif

	if (!dev) {
		pr_err("device is disconnected");
		return -ENODEV;
	}
	mutex_lock(&dev->ddm_mutex);
	if (!dev->ifc) {
		ret = -ENODEV;
		goto error;
	}

	if (!dev->ops) {
		pr_err("bridge is not open");
		ret = -ENODEV;
		goto error;
	}

	if (!size) {
		dev_err(&dev->ifc->dev, "invalid size:%d\n", size);
		ret = -EINVAL;
		goto error;
	}

	/* if there was a previous unrecoverable error, just quit */
	if (dev->err) {
		ret = -ENODEV;
		goto error;
	}

	kref_get(&dev->kref);

	urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!urb) {
		dev_err(&dev->ifc->dev, "unable to allocate urb\n");
		ret = -ENOMEM;
	}

	ret = usb_autopm_get_interface(dev->ifc);
	if (ret < 0 && ret != -EAGAIN && ret != -EACCES) {
		//pr_err_ratelimited("write: autopm_get failed:%d", ret);
	}

	pipe = usb_sndbulkpipe(dev->udev, dev->out_epAddr);
	usb_fill_bulk_urb(urb, dev->udev, pipe, data, size,
				ddm_bridge_write_cb, dev);
	urb->transfer_flags |= URB_ZERO_PACKET;
	usb_anchor_urb(urb, &dev->submitted);
	dev->pending_writes++;

	ret = usb_submit_urb(urb, GFP_KERNEL);
	if (ret) {
		pr_err_ratelimited("submitting urb failed err:%d", ret);
		dev->pending_writes--;
		usb_unanchor_urb(urb);
		usb_autopm_put_interface(dev->ifc);
		ret = -EBUSY;
	}
	usb_free_urb(urb);
//	if (ret) /* otherwise this is done in the completion handler */
error:
//		kref_put(&dev->kref, ddm_bridge_delete);
	mutex_unlock(&dev->ddm_mutex);

	return ret;
}
EXPORT_SYMBOL(ddm_bridge_write);

static void ddm_bridge_delete(struct kref *kref)
{
	struct ddm_bridge *dev =
		container_of(kref, struct ddm_bridge, kref);

	usb_put_dev(dev->udev);
	ddm_bridge_dev = 0;
	kfree(dev);
}

/*******************************************************************************
 *
 * ddm_bridge_probe
 * 
 * DESC : This function is called after ddm_CH is initiated.
 *
 ******************************************************************************/
static int
ddm_bridge_probe(struct usb_interface *ifc, const struct usb_device_id *id)
{
	struct ddm_bridge		*dev;
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
	dev->pdev = platform_device_alloc("ddm", -1);
	if (!dev->pdev) {
		pr_err("%s: unable to allocate platform device\n", __func__);
		kfree(dev);
		return -ENOMEM;
	}
	ddm_bridge_dev = dev;
	dev->udev = usb_get_dev(interface_to_usbdev(ifc));
	dev->ifc = ifc;

	device_set_wakeup_enable(&dev->udev->dev, 1);

	mutex_init(&dev->ddm_mutex);
	kref_init(&dev->kref);
	init_usb_anchor(&dev->submitted);
	
	ifc_desc = ifc->cur_altsetting;
	for (i = 0; i < ifc_desc->desc.bNumEndpoints; i++) {
		ep_desc = &ifc_desc->endpoint[i].desc;
		if (!dev->in_epAddr && usb_endpoint_is_bulk_in(ep_desc))
			dev->in_epAddr = ep_desc->bEndpointAddress;
		if (!dev->out_epAddr && usb_endpoint_is_bulk_out(ep_desc))
			dev->out_epAddr = ep_desc->bEndpointAddress;
	}
	if (!(dev->in_epAddr && dev->out_epAddr)) {
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
		kref_put(&dev->kref, ddm_bridge_delete);

	return ret;
}

static void ddm_bridge_disconnect(struct usb_interface *ifc)
{
	struct ddm_bridge	*dev = usb_get_intfdata(ifc);

	dev_dbg(&dev->udev->dev, "%s:\n", __func__);

#ifdef LG_FW_REMOTE_WAKEUP
	device_set_wakeup_enable(&dev->udev->dev, 0);
#endif
	platform_device_del(dev->pdev);
	kref_put(&dev->kref, ddm_bridge_delete);
	usb_set_intfdata(ifc, NULL);
}
static int ddm_bridge_suspend(struct usb_interface *ifc, pm_message_t message)
{
	struct ddm_bridge	*dev = usb_get_intfdata(ifc);
	struct ddm_bridge_ops	*cbs = dev->ops;
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

static int ddm_bridge_resume(struct usb_interface *ifc)
{
	struct ddm_bridge	*dev = usb_get_intfdata(ifc);
	struct ddm_bridge_ops	*cbs = dev->ops;

	if (cbs && cbs->resume){
		cbs->resume(cbs->ctxt);
	}

	return 0;
}

/* ddm id table need to check. We supported usb_id until 0x9048 */
#ifdef CONFIG_USB_LGE_DDM_BRIDGE
#define HSIC_VALID_INTERFACE_NUM	3
#else
#define HSIC_VALID_INTERFACE_NUM	1
#endif
static const struct usb_device_id ddm_bridge_ids[] = {
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
MODULE_DEVICE_TABLE(usb, ddm_bridge_ids);

/* ddm driver struct */

static struct usb_driver ddm_bridge_driver = {
	.name =		"DDM Bridge",
	.probe =	ddm_bridge_probe,
	.disconnect =	ddm_bridge_disconnect,
	.suspend = ddm_bridge_suspend,
	.resume = ddm_bridge_resume,
	.id_table =	ddm_bridge_ids,
	.supports_autosuspend = 1,
};

/*******************************************************************************
 *
 * ddm_bridge_init
 * 
 * DESC : 
 *
 ******************************************************************************/
static int __init ddm_bridge_init(void)
{
	int ret;


	ret = usb_register(&ddm_bridge_driver);
	if (ret) {
		err("%s: unable to register DDM bridge driver",
				__func__);
		return ret;
	}

	return 0;
}
/*******************************************************************************
 *
 * ddm_bridge_exit
 * 
 * DESC : 
 *
 ******************************************************************************/
static void __exit ddm_bridge_exit(void)
{
	usb_deregister(&ddm_bridge_driver);
}

module_init(ddm_bridge_init);
module_exit(ddm_bridge_exit);

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL v2");

