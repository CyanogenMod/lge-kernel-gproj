/*
 * Gadget Driver for MCPC DTF
 *
 * This code borrows from f_adb.c, which is
 * Copyright (C) 2008 Google, Inc.
 * Author: Mike Lockwood <lockwood@android.com>
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

#include <linux/module.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/freezer.h>

#include <linux/types.h>
#include <linux/file.h>
#include <linux/device.h>
#include <linux/miscdevice.h>

#include <linux/usb.h>
#include <linux/usb_usual.h>
#include <linux/usb/ch9.h>
#include <linux/usb/cdc.h>
#include <linux/usb/android_composite.h>
#include <linux/usb/f_dtf.h>
#include <linux/usb/f_dtf_if.h>

#ifndef RH_DBG_MSG
#define _dbgmsg(fmt, ...)
#else
#define _dbgmsg(fmt, ...)		printk(KERN_INFO "[DTF]::[%s]::[%d]::"fmt, __func__,__LINE__, ##__VA_ARGS__)
#endif  /* RH_DBG_MSG */

#ifndef RH_DBG_MSG_SEQ
#define _dbgmsg_if(fmt, ...)
#define _dbgmsg_in(fmt, ...)
#define _dbgmsg_out(fmt, ...)
#define _dbgmsg_gadget(fmt, ...)
#else
#define _dbgmsg_if(fmt, ...)	printk(KERN_INFO "[DTFIF]::[%s]::[%d]::"fmt, __func__,__LINE__, ##__VA_ARGS__)
#define _dbgmsg_in(fmt, ...)	printk(KERN_INFO "[DTFIF(IN)]::[%s]::[%d]::"fmt, __func__,__LINE__, ##__VA_ARGS__)
#define _dbgmsg_out(fmt, ...)	printk(KERN_INFO "[DTFIF(OUT)]::[%s]::[%d]::"fmt, __func__,__LINE__, ##__VA_ARGS__)
#define _dbgmsg_gadget(fmt, ...)	printk(KERN_INFO "[DTFIF(GADGET)]::[%s]::[%d]::"fmt, __func__,__LINE__, ##__VA_ARGS__)
#endif  /* RH_DBG_MSG_SEQ */

/* PipeGroup 1 */
/* Interface Association Descriptor */

/* Standard interface Descriptor (communication) */
static struct usb_interface_descriptor vPg1_intf_comm_desc = {
	.bLength                = USB_DT_INTERFACE_SIZE,
	.bDescriptorType        = USB_DT_INTERFACE,
	.bInterfaceNumber       = 0,
	.bAlternateSetting      = 0,
	.bNumEndpoints          = 1,
	.bInterfaceClass        = USB_CLASS_COMM,
	.bInterfaceSubClass     = 0x88,
	.bInterfaceProtocol     = 1,
	.iInterface             = 0,
};

/* Header Functional Descriptor */
static struct usb_cdc_header_desc vPg1_cdc_header = {
	.bLength            = sizeof vPg1_cdc_header,
	.bDescriptorType    = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = USB_CDC_HEADER_TYPE,
	.bcdCDC             = __constant_cpu_to_le16(0x0110),
};

/* Control Management Functional Descriptor */
static struct usb_cdc_call_mgmt_descriptor vPg1_call_mng = {
	.bLength            =  sizeof vPg1_call_mng,
	.bDescriptorType    =  USB_DT_CS_INTERFACE,
	.bDescriptorSubType =  USB_CDC_CALL_MANAGEMENT_TYPE,
	.bmCapabilities     =  (USB_CDC_CALL_MGMT_CAP_CALL_MGMT | USB_CDC_CALL_MGMT_CAP_DATA_INTF),
	.bDataInterface     =  0x01,
};

/* Abstract Controll Magagement  Descriptor */
static struct usb_cdc_acm_descriptor vPg1_acm_desc = {
	.bLength            = sizeof vPg1_acm_desc,
	.bDescriptorType    = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = USB_CDC_ACM_TYPE,
	.bmCapabilities     = (USB_CDC_CAP_LINE|USB_CDC_CAP_BRK),
};

/* Union Function Descriptor */
static struct usb_cdc_union_desc vPg1_union_desc = {
	.bLength            = sizeof vPg1_union_desc,
	.bDescriptorType    = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = USB_CDC_UNION_TYPE,
	.bMasterInterface0  = 0,
	.bSlaveInterface0   = 1,
};

/* Mobile Abstract Control Medel Specific Function Descriptor */
static struct sUsb_model_spcfc_func_desc vPg1_model_desc = {
	.bLength            = 5,
	.bDescriptorType    = (USB_TYPE_VENDOR | USB_DT_INTERFACE),
	.bDescriptorSubType = 0x11,
	.bType              = 0x02,
	.bMode              = {0xC0},
	.mMode_num           = 1,
};

/* Standard endpoint Descriptor (interrupt) (full speed) */
static struct usb_endpoint_descriptor vPg1_epintr_desc = {
	.bLength            = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType    = USB_DT_ENDPOINT,
	.bEndpointAddress   = USB_DIR_IN | 0x01,
	.bmAttributes       = USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize     = __constant_cpu_to_le16(16),
	.bInterval          = 0x10,
};

/* Standard endpoint Descriptor (interrupt) (high speed) */
static struct usb_endpoint_descriptor vPg1_epintr_desc_hs = {
	.bLength            = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType    = USB_DT_ENDPOINT,
	.bEndpointAddress   = USB_DIR_IN | 0x01,
	.bmAttributes       = USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize     = __constant_cpu_to_le16(16),
	.bInterval          = 0x08,
};

/* Standard interface Descriptor */
static struct usb_interface_descriptor vPg1_intf_bulk_desc = {
	.bLength                = USB_DT_INTERFACE_SIZE,
	.bDescriptorType        = USB_DT_INTERFACE,
	.bInterfaceNumber       = 1,
	.bAlternateSetting      = 0,
	.bNumEndpoints          = 2,
	.bInterfaceClass        = USB_CLASS_CDC_DATA,
	.bInterfaceSubClass     = 0,
	.bInterfaceProtocol     = 0,
	.iInterface             = 0,
};

/* Standard endpoint Descriptor (bulk) (high speed) */
static struct usb_endpoint_descriptor vPg1_epin_desc_hs = {
	.bLength            = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType    = USB_DT_ENDPOINT,
	.bEndpointAddress   = USB_DIR_IN | 0x02,
	.bmAttributes       = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize     = __constant_cpu_to_le16(512),
	.bInterval          = 0x00,
};

/* Standard endpoint Descriptor (bulk) (high speed) */
static struct usb_endpoint_descriptor vPg1_epout_desc_hs = {
	.bLength            = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType    = USB_DT_ENDPOINT,
	.bEndpointAddress   = USB_DIR_OUT | 0x03,
	.bmAttributes       = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize     = __constant_cpu_to_le16(512),
	.bInterval          = 0x00,
};

/* Standard endpoint Descriptor (bulk) (full speed) */
static struct usb_endpoint_descriptor vPg1_epin_desc = {
	.bLength            = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType    = USB_DT_ENDPOINT,
	.bEndpointAddress   = USB_DIR_IN | 0x02,
	.bmAttributes       = USB_ENDPOINT_XFER_BULK,
	.bInterval          = 0x00,
};

/* Standard endpoint Descriptor (bulk) (full speed) */
static struct usb_endpoint_descriptor vPg1_epout_desc = {
	.bLength            = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType    = USB_DT_ENDPOINT,
	.bEndpointAddress   = USB_DIR_OUT | 0x03,
	.bmAttributes       = USB_ENDPOINT_XFER_BULK,
	.bInterval          = 0x00,
};

static struct usb_descriptor_header *vFs_dtf_descs[] = {
	/* PipeGroup 1 */
	(struct usb_descriptor_header *) &vPg1_intf_comm_desc,
	(struct usb_descriptor_header *) &vPg1_cdc_header,
	(struct usb_descriptor_header *) &vPg1_call_mng,
	(struct usb_descriptor_header *) &vPg1_acm_desc,
	(struct usb_descriptor_header *) &vPg1_union_desc,
	(struct usb_descriptor_header *) &vPg1_model_desc,
	(struct usb_descriptor_header *) &vPg1_epintr_desc,
	(struct usb_descriptor_header *) &vPg1_intf_bulk_desc,
	(struct usb_descriptor_header *) &vPg1_epin_desc,
	(struct usb_descriptor_header *) &vPg1_epout_desc,

	NULL,
};

static struct usb_descriptor_header *vHs_dtf_descs[] = {
	/* PipeGroup 1 */
	(struct usb_descriptor_header *) &vPg1_intf_comm_desc,
	(struct usb_descriptor_header *) &vPg1_cdc_header,
	(struct usb_descriptor_header *) &vPg1_call_mng,
	(struct usb_descriptor_header *) &vPg1_acm_desc,
	(struct usb_descriptor_header *) &vPg1_union_desc,
	(struct usb_descriptor_header *) &vPg1_model_desc,
	(struct usb_descriptor_header *) &vPg1_epintr_desc_hs,
	(struct usb_descriptor_header *) &vPg1_intf_bulk_desc,
	(struct usb_descriptor_header *) &vPg1_epin_desc_hs,
	(struct usb_descriptor_header *) &vPg1_epout_desc_hs,

	NULL,
};

static struct dtf_dev *_dtf_dev;

static void dtf_complete_intr(struct usb_ep *ep, struct usb_request *req);
static void dtf_complete_in(struct usb_ep *ep, struct usb_request *req);
static void dtf_complete_out(struct usb_ep *ep, struct usb_request *req);
static int dtf_function_bind(struct usb_configuration *c, struct usb_function *f);
static void dtf_function_unbind(struct usb_configuration *c, struct usb_function *f);
static int dtf_function_setup(struct usb_function *f, const struct usb_ctrlrequest *ctrl);
static int dtf_function_set_alt(struct usb_function *f, unsigned intf, unsigned alt);
static void dtf_function_disable(struct usb_function *f);
static void dtf_function_suspend(struct usb_function *f);
static void dtf_function_resume(struct usb_function *f);
static void dtf_request_free(struct usb_request *req, struct usb_ep *ep);
static inline struct dtf_dev *func_to_dtf(struct usb_function *f);
static struct usb_request *dtf_request_new(struct usb_ep *ep, int buffer_size);

static ssize_t dtf_if_read( struct file *pfile, char *pbuf, size_t count, loff_t *ppos );
static ssize_t dtf_if_write( struct file *pfile, const char *pbuf, size_t count, loff_t *ppos );
static unsigned int dtf_if_poll( struct file *pfile, poll_table *pwait );
static long dtf_if_ioctl( struct file *pfile, unsigned int cmd, unsigned long arg );
static int dtf_if_open( struct inode *pinode, struct file *pfile );
static int dtf_if_close( struct inode *pinode, struct file *pfile );

int dtf_if_init(void);
static void dtf_if_out_setup(const struct dtf_if_ctrlrequest *);
static void dtf_if_out_set_alt(int);
static void dtf_if_out_disable(int);
static void dtf_if_out_suspend(void);
static void dtf_if_out_resume(void);
static void dtf_if_out_complete_in(int);
static void dtf_if_out_complete_out(int, int, char *);
static void dtf_if_out_complete_intr(int);
static void dtf_if_out_ctrl_complete(int, int, char *);

static void dtf_if_init_read_data(void);
static void dtf_if_add_read_data(struct dtf_if_read_data *read_data);
static struct dtf_if_read_data *dtf_if_get_read_data(void);
static int dtf_if_get_read_data_num(void);

#define DTF_IF_READ_DATA_MAX	5

static const char dtf_shortname[] = "android_dtf";
spinlock_t 			lock_read_data;
wait_queue_head_t	poll_wait_read;
static bool dtf_if_active = false;
static int dtf_readable_count = 0;
struct dtf_if_read_data dtf_read_data[DTF_IF_READ_DATA_MAX];
static int dtf_if_readable_num = 0;
static int dtf_if_readable_head = 0;
static int dtf_if_readable_tail = 0;

/* dtf file_operation */
static struct file_operations dtf_fops =
{
	.read=				dtf_if_read,
	.write=				dtf_if_write,
	.poll=				dtf_if_poll,
	.unlocked_ioctl=	dtf_if_ioctl,
	.open=				dtf_if_open,
	.release=			dtf_if_close
};

#define D_DTF_MINOR_NUMBER  249		//LGE customize Global Minor Number

static struct miscdevice dtf_device = {
	.minor = D_DTF_MINOR_NUMBER,	//LGE customize Global Minor Number
	.name = dtf_shortname,
	.fops = &dtf_fops,
};

static int dtf_if_open( struct inode *pinode, struct file *pfile )
{
	_dbgmsg_if( "IN\n" );
	dtf_if_active = true;
	_dbgmsg_if( "OUT\n" );
	return 0;
}

static int dtf_if_close( struct inode *pinode, struct file *pfile )
{
	_dbgmsg_if( "IN\n" );
	dtf_if_active = false;
	_dbgmsg_if( "OUT\n" );
	return 0;
}

static ssize_t dtf_if_read( struct file *pfile, char *pbuf, size_t count, loff_t *ppos )
{
	struct dtf_if_read_data *read_data;
/* MSE-ADD-S for GV(Docomo) No.3-7 */
    unsigned long   flags;
/* MSE-ADD-E for GV(Docomo) No.3-7 */

	_dbgmsg_if( "IN\n" );

/* MSE-ADD-S for GV(Docomo) No.3-20, 3-7 */
    spin_lock_irqsave(&lock_read_data, flags);
/* MSE-ADD-E for GV(Docomo) No.3-20, 3-7 */
	read_data = dtf_if_get_read_data();
	if ( copy_to_user((void*)pbuf, read_data, sizeof(struct dtf_if_read_data)) )
	{
		_dbgmsg_if( "copy_to_user error\n" );
/* MSE-ADD-S for GV(Docomo) No.3-20, 3-7 */
        spin_unlock_irqrestore(&lock_read_data, flags);
/* MSE-ADD-E for GV(Docomo) No.3-20, 3-7 */
	    return -EFAULT;
	}
	_dbgmsg_if( "OUT\n" );
/* MSE-ADD-S for GV(Docomo) No.3-20, 3-7 */
    spin_unlock_irqrestore(&lock_read_data, flags);
/* MSE-ADD-E for GV(Docomo) No.3-20, 3-7 */
	return 0;
}
static ssize_t dtf_if_write( struct file *pfile, const char *pbuf, size_t count, loff_t *ppos )
{
	char bulk_in_buff[512];

	_dbgmsg_if( "IN\n" );
	
	if (count > 512)
	{
		_dbgmsg_if( "size over error\n" );
		return -EIO;
	}

	if (copy_from_user(bulk_in_buff, pbuf, count))
	{
		_dbgmsg_if( "copy_from_user error\n" );
		return -EFAULT;
	}
	dtf_if_in_bulk_in((unsigned)count, (const char *)bulk_in_buff);
	_dbgmsg_if( "OUT\n" );
	return 0;
}
static unsigned int dtf_if_poll( struct file *pfile, poll_table *pwait )
{
	unsigned int mask = 0;

	_dbgmsg_if( "IN\n" );
	poll_wait( pfile, &poll_wait_read, pwait );
	if (dtf_if_get_read_data_num() > 0)
	{
		_dbgmsg( "(POLLIN | POLLRDNORM)\n" );
		mask |= (POLLIN | POLLRDNORM);
	}
	_dbgmsg_if( "OUT(%d)\n", mask );
	return mask;
}
static long dtf_if_ioctl( struct file *pfile, unsigned int cmd, unsigned long arg )
{
	struct dtf_if_write_data write_data;

	_dbgmsg_if( "IN(%d)\n", cmd );
	
	switch (cmd) {
	case DTF_IF_EVENT_INTR_IN:
		_dbgmsg_if( "case DTF_IF_EVENT_INTR_IN\n" );

		if (copy_from_user(&write_data, (void *)arg, sizeof(write_data)))
		{
			_dbgmsg_if( "copy_from_user error\n" );
			return -EFAULT;
		}
		dtf_if_in_intr_in((unsigned)write_data.size, (const char *)write_data.data);

		break;
		
	case DTF_IF_EVENT_SET_HALT_INTR_IN:
		_dbgmsg_if( "case DTF_IF_EVENT_SET_HALT_INTR_IN\n" );
		dtf_if_in_set_halt_intr_in();
		break;
		
	case DTF_IF_EVENT_SET_HALT_BULK_IN:
		_dbgmsg_if( "case DTF_IF_EVENT_SET_HALT_BULK_IN\n" );
		dtf_if_in_set_halt_bulk_in();
		break;
		
	case DTF_IF_EVENT_SET_HALT_OUT:
		_dbgmsg_if( "case DTF_IF_EVENT_SET_HALT_OUT\n" );
		dtf_if_in_set_halt_out();
		break;

	case DTF_IF_EVENT_CLEAR_HALT_INTR_IN:
		_dbgmsg_if( "case DTF_IF_EVENT_CLEAR_HALT_INTR_IN\n" );
		dtf_if_in_clear_halt_intr_in();
		break;
		
	case DTF_IF_EVENT_CLEAR_HALT_BULK_IN:
		_dbgmsg_if( "case DTF_IF_EVENT_CLEAR_HALT_BULK_IN\n" );
		dtf_if_in_clear_halt_bulk_in();
		break;
		
	case DTF_IF_EVENT_CLEAR_HALT_OUT:
		_dbgmsg_if( "case DTF_IF_EVENT_CLEAR_HALT_OUT\n" );
		dtf_if_in_clear_halt_out();
		break;
		
	case DTF_IF_EVENT_CTRL_IN:
		_dbgmsg_if( "case DTF_IF_EVENT_CTRL_IN\n" );

		if (copy_from_user(&write_data, (void *)arg, sizeof(write_data)))
		{
			_dbgmsg_if( "copy_from_user error\n" );
			return -EFAULT;
		}
		dtf_if_in_ctrl_in(write_data.size, (const char *)write_data.data);

		break;

	case DTF_IF_EVENT_CTRL_OUT:
		_dbgmsg_if( "case DTF_IF_EVENT_CTRL_OUT\n" );

		if (copy_from_user(&write_data, (void *)arg, sizeof(write_data)))
		{
			_dbgmsg_if( "copy_from_user error\n" );
			return -EFAULT;
		}
		dtf_if_in_ctrl_out(write_data.size);

		break;

	default:
		_dbgmsg_if( "cmd error\n" );
		break;
	}
	
	
	
	_dbgmsg_if( "OUT\n" );
	return 0;
}


static int dtf_allocate_endpoints(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct dtf_dev *dev = func_to_dtf(f);
	struct usb_request *req;
	struct usb_ep *ep;

	_dbgmsg( "IN\n" );
	
	/* allocate endpoints: PipeGroup1 intrrupt */
	_dbgmsg_gadget( "usb_ep_autoconfig\n" );
	ep = usb_ep_autoconfig(cdev->gadget, &vPg1_epintr_desc);
	if( !ep ) {
		_dbgmsg( "usb_ep_autoconfig for PG1 ep_intr failed\n" );
		return -ENODEV;
	}
	_dbgmsg("usb_ep_autoconfig for PG1 ep_intr got %s\n", ep->name);

	ep->driver_data = dev;
	dev->pg.ep_intr = ep;

	/* allocate endpoints: PipeGroup1 bulk(in) */
	_dbgmsg_gadget( "usb_ep_autoconfig\n" );
	ep = usb_ep_autoconfig(cdev->gadget, &vPg1_epin_desc);
	if( !ep ) {
		_dbgmsg( "usb_ep_autoconfig for PG1 ep_in failed\n" );
		return -ENODEV;
	}
	_dbgmsg("usb_ep_autoconfig for PG1 ep_in got %s\n", ep->name);

	ep->driver_data = dev;
	dev->pg.ep_in = ep;

	/* allocate endpoints: PipeGroup1 bulk(out) */
	_dbgmsg_gadget( "usb_ep_autoconfig\n" );
	ep = usb_ep_autoconfig(cdev->gadget, &vPg1_epout_desc);
	if( !ep ) {
		_dbgmsg( "usb_ep_autoconfig for PG1 ep_out failed\n" );
		return -ENODEV;
	}
	_dbgmsg("usb_ep_autoconfig for PG1 ep_out got %s\n", ep->name);
	ep->driver_data = dev;
	dev->pg.ep_out = ep;

	/* support high speed hardware */
	if (gadget_is_dualspeed(cdev->gadget)) {
		vPg1_epintr_desc_hs.bEndpointAddress = vPg1_epintr_desc.bEndpointAddress;
		vPg1_epin_desc_hs.bEndpointAddress = vPg1_epin_desc.bEndpointAddress;
		vPg1_epout_desc_hs.bEndpointAddress = vPg1_epout_desc.bEndpointAddress;
	}

	_dbgmsg("%s speed %s: PG1[INTR/%s, IN/%s, OUT/%s]\n",
		gadget_is_dualspeed(cdev->gadget) ? "dual" : "full",
		f->name,
		dev->pg.ep_intr->name, dev->pg.ep_in->name, dev->pg.ep_out->name);

	/* allocate request for endpoints */
	req = dtf_request_new( dev->pg.ep_intr, 16 );
	if(!req) {
		_dbgmsg( "create request error\n" );
		return -ENODEV;
	}
	req->complete = dtf_complete_intr;
	dev->pg.mReq_intr = req;

	req = dtf_request_new( dev->pg.ep_in, 512 );
	if(!req) {
		_dbgmsg( "create request error\n" );
/* MSE-ADD-S for GV(Docomo) No.3-19 */
        dtf_request_free(dev->pg.mReq_intr, dev->pg.ep_intr);
/* MSE-ADD-E for GV(Docomo) No.3-19 */
		return -ENODEV;
	}
	req->complete = dtf_complete_in;
	dev->pg.mReq_in = req;

	req = dtf_request_new( dev->pg.ep_out, 512 );
	if(!req) {
		_dbgmsg( "create request error\n" );
/* MSE-ADD-S for GV(Docomo) No.3-19 */
        dtf_request_free(dev->pg.mReq_intr, dev->pg.ep_intr);
        dtf_request_free(dev->pg.mReq_in, dev->pg.ep_in);
/* MSE-ADD-E for GV(Docomo) No.3-19 */
		return -ENODEV;
	}
	req->complete = dtf_complete_out;
	dev->pg.mReq_out = req;

	_dbgmsg( "OUT\n" );
	return 0;
}

static int dtf_allocate_interface_ids( struct usb_configuration *c, struct usb_function *f )
{
	int id;
	struct dtf_dev *dev = func_to_dtf(f);

	_dbgmsg( "IN\n" );
	
	/* Allocate Interface ID: PipeGroup1 communication interface */
	_dbgmsg_gadget( "usb_interface_id\n" );
	id = usb_interface_id(c, f);
	_dbgmsg( "usb_interface_id() = %d\n", id );
	if( id < 0 )
		return id;

	dev->pg.mCtrl_id = id;
	id = 0;     /* fixed interface number */
	vPg1_intf_comm_desc.bInterfaceNumber = id;
	vPg1_union_desc.bMasterInterface0 = id;

	/* Allocate Interface ID: PipeGroup1 bulk interface */
	_dbgmsg_gadget( "usb_interface_id\n" );
	id = usb_interface_id(c, f);
	_dbgmsg( "usb_interface_id() = %d(%d)\n", id, vPg1_intf_comm_desc.bInterfaceNumber );
	if( id < 0 )
		return id;

	dev->pg.mData_id = id;
	id = 1;     /* fixed interface number */
	vPg1_intf_bulk_desc.bInterfaceNumber = id;
	vPg1_union_desc.bSlaveInterface0 = id;
	vPg1_call_mng.bDataInterface = id;
	_dbgmsg( "usb_interface_id() = %d(%d)\n", id, vPg1_intf_bulk_desc.bInterfaceNumber );

	_dbgmsg( "OUT\n" );
	return 0;
}

static int dtf_bind_config(struct usb_configuration *c)
{
	struct dtf_dev  *dev = _dtf_dev;
	int ret;

	_dbgmsg( "IN\n" );

	dev->cdev = c->cdev;
	dev->function.name = "dtf";
	dev->function.descriptors = vFs_dtf_descs;
	dev->function.hs_descriptors = vHs_dtf_descs;
	dev->function.bind = dtf_function_bind;
	dev->function.unbind = dtf_function_unbind;
	dev->function.setup = dtf_function_setup;
	dev->function.set_alt = dtf_function_set_alt;
	dev->function.disable = dtf_function_disable;
	dev->function.suspend = dtf_function_suspend;
	dev->function.resume = dtf_function_resume;

	dev->mCtrl_ep_enbl = 0;
	dev->mData_ep_enbl = 0;

	_dbgmsg_gadget( "usb_add_function\n" );
	ret = usb_add_function(c, &dev->function);
	_dbgmsg( "OUT(%d)\n", ret );

	return ret;
}

static int dtf_setup(void)
{
	struct dtf_dev  *dev;
	int ret;

	_dbgmsg( "IN\n" );

	dev = kzalloc( sizeof( *dev ), GFP_KERNEL );
	if( !dev ) {
		printk( KERN_ERR "dtf gadget driver failed to initialize nomem\n" );
		return -ENOMEM;
	}

	spin_lock_init(&dev->lock);

	_dtf_dev = dev;

	ret = dtf_if_init();
	if (ret)
		goto err;

	_dbgmsg( "OUT\n" );
	return 0;

err:
	kfree(dev);
	printk(KERN_ERR "dtf gadget driver failed to initialize\n");
	return ret;
}

static void dtf_complete_in(struct usb_ep *ep, struct usb_request *req)
{
	_dbgmsg( "IN\n" );
	dtf_if_out_complete_in(req->status);
	_dbgmsg( "OUT\n" );
}

static void dtf_complete_intr(struct usb_ep *ep, struct usb_request *req)
{
	_dbgmsg( "IN\n" );
	dtf_if_out_complete_intr(req->status);
	_dbgmsg( "OUT\n" );
}

static void dtf_complete_out(struct usb_ep *ep, struct usb_request *req)
{
	struct dtf_dev *dev = _dtf_dev;

	_dbgmsg( "IN\n" );

	dtf_if_out_complete_out(req->status, (int)req->actual, (char *)req->buf);

	if ( dev->mData_ep_enbl == 1 ) 
	{
		dev->pg.mReq_out->length = 512;
		
		_dbgmsg_gadget( "usb_ep_queue\n" );
		usb_ep_queue( dev->pg.ep_out, dev->pg.mReq_out, GFP_ATOMIC );
	}

	_dbgmsg( "OUT\n" );
}

static int dtf_function_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct dtf_dev *dev = func_to_dtf(f);
	int ret;

	_dbgmsg( "IN\n" );
	dev->cdev = cdev;
    
	/* allocate Interface IDs */
	ret = dtf_allocate_interface_ids( c, f );
	if( ret < 0 ) {
		printk( KERN_ERR "allocate interface IDs error\n" );
		return ret;
	}

	/* allocate endpoints */
	ret = dtf_allocate_endpoints( c, f );
	if( ret < 0 ) {
		printk( KERN_ERR "allocate endpoints error\n" );
		return ret;
	}

	_dbgmsg( "OUT\n" );
	return 0;
}

static void dtf_function_disable(struct usb_function *f)
{
	struct dtf_dev *dev = func_to_dtf(f);
	int speed_check;

	_dbgmsg( "IN\n" );

    if( dev->mCtrl_ep_enbl == 1 )
    {
        dev->mCtrl_ep_enbl = 0;

    	_dbgmsg_gadget( "usb_ep_disable(%s)\n", dev->pg.ep_intr->name );
		usb_ep_disable( dev->pg.ep_intr );
    }

    if( dev->mData_ep_enbl == 1 )
    {
    	dev->mData_ep_enbl = 0;

    	_dbgmsg_gadget( "usb_ep_dequeue\n" );
		usb_ep_dequeue( dev->pg.ep_out, dev->pg.mReq_out );
    	
		_dbgmsg_gadget( "usb_ep_disable(%s)\n", dev->pg.ep_intr->name );
		usb_ep_disable( dev->pg.ep_in );
    	
		_dbgmsg_gadget( "usb_ep_disable(%s)\n", dev->pg.ep_out->name );
		usb_ep_disable( dev->pg.ep_out );
    }
	
	speed_check = (_dtf_dev->cdev->gadget->speed == USB_SPEED_HIGH)?(1):(0);
	dtf_if_out_disable(speed_check);

	_dbgmsg( "OUT\n" );
}

static void dtf_function_resume(struct usb_function *f)
{
	_dbgmsg( "IN\n" );
	dtf_if_out_resume();
	_dbgmsg( "OUT\n" );
}

static int dtf_function_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
	struct dtf_dev *dev = func_to_dtf(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	int ret;
	int set_alt_end = 0;
	int speed_check = 0;

	_dbgmsg("dtf_function_set_alt(intf=%d,alt=%d)\n", intf, alt);

	if( dev->pg.mCtrl_id == intf ) {
		_dbgmsg_gadget( "usb_ep_enable(%s)\n", dev->pg.ep_intr->name );
/* MSE-MOD-S for GJ(Docomo) */
//		ret = usb_ep_enable( dev->pg.ep_intr,
//				     ep_choose( cdev->gadget, &vPg1_epintr_desc_hs, &vPg1_epintr_desc ) );
		ret = config_ep_by_speed( cdev->gadget, f, dev->pg.ep_intr );
		if( ret ) {
			_dbgmsg( "config_ep_by_speed failes for ep %s, result %d\n", dev->pg.ep_intr->name, ret );
			return ret;
		}
		ret = usb_ep_enable( dev->pg.ep_intr );
/* MSE-MOD-E for GJ(Docomo) */
		if( ret ) {
			_dbgmsg( "usb_ep_enable error pg1 ep_intr ret = %d\n", ret );
			return ret;
		}
        dev->mCtrl_ep_enbl = 1;
	} else if( dev->pg.mData_id == intf ) {
		_dbgmsg_gadget( "usb_ep_enable(%s)\n", dev->pg.ep_in->name );
/* MSE-MOD-S for GJ(Docomo) */
//		ret = usb_ep_enable( dev->pg.ep_in,
//				     ep_choose( cdev->gadget, &vPg1_epin_desc_hs, &vPg1_epin_desc ) );
		ret = config_ep_by_speed( cdev->gadget, f, dev->pg.ep_in );
		if( ret ) {
			_dbgmsg( "config_ep_by_speed failes for ep %s, result %d\n", dev->pg.ep_in->name, ret );
			return ret;
		}
		ret = usb_ep_enable( dev->pg.ep_in );
/* MSE-MOD-E for GJ(Docomo) */
		if( ret ) {
			_dbgmsg( "usb_ep_enable error pg1 ep_in ret = %d\n", ret );
			return ret;
		}
		_dbgmsg_gadget( "usb_ep_enable(%s)\n", dev->pg.ep_out->name );
/* MSE-MOD-S for GJ(Docomo) */
//		ret = usb_ep_enable( dev->pg.ep_out,
//				     ep_choose( cdev->gadget, &vPg1_epout_desc_hs, &vPg1_epout_desc ) );
		ret = config_ep_by_speed( cdev->gadget, f, dev->pg.ep_out );
		if( ret ) {
			_dbgmsg( "config_ep_by_speed failes for ep %s, result %d\n", dev->pg.ep_out->name, ret );
			return ret;
		}
		ret = usb_ep_enable( dev->pg.ep_out );
/* MSE-MOD-E for GJ(Docomo) */
		if( ret ) {
			_dbgmsg( "usb_ep_enable error pg1 ep_out ret = %d\n", ret );
			usb_ep_disable(dev->pg.ep_in);
			return ret;
		}

		dev->pg.mReq_out->length = 512;
		usb_ep_queue( dev->pg.ep_out, dev->pg.mReq_out, GFP_ATOMIC );
        dev->mData_ep_enbl = 1;

	} else {
		_dbgmsg( "unknown interface number\n" );
	}

	set_alt_end = (
		(dev->mCtrl_ep_enbl) & (dev->mData_ep_enbl) );
	speed_check = (dev->cdev->gadget->speed == USB_SPEED_HIGH)?(1):(0);

	if (set_alt_end == 1)
	{
		dtf_if_out_set_alt( speed_check );
	}

	return 0;
}

static int dtf_function_setup(struct usb_function *f, const struct usb_ctrlrequest *ctrl)
{
	struct dtf_if_ctrlrequest ctrlrequest;

	_dbgmsg( "IN\n" );

	ctrlrequest.bRequestType = ctrl->bRequestType;
	ctrlrequest.bRequest = ctrl->bRequest;
	ctrlrequest.wValue = ctrl->wValue;
	ctrlrequest.wIndex = ctrl->wIndex;
	ctrlrequest.wLength = ctrl->wLength;
		
	dtf_if_out_setup( &ctrlrequest );

	_dbgmsg( "OUT\n" );

	return 0;
}

static void dtf_function_suspend(struct usb_function *f)
{
	_dbgmsg( "IN\n" );
	dtf_if_out_suspend();
	_dbgmsg( "OUT\n" );
	
}

static void dtf_function_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct dtf_dev *dev = func_to_dtf(f);
/* MSE-ADD-S for GV(Docomo) No.3-7 */
    unsigned long   flags;
/* MSE-ADD-E for GV(Docomo) No.3-7 */
	_dbgmsg( "IN\n" );

/* MSE-MOD-S for GV(Docomo) No.3-7 */
//    spin_lock_irq(&dev->lock);
    spin_lock_irqsave(&dev->lock, flags);
/* MSE-MOD-E for GV(Docomo) No.3-7 */
	dtf_request_free( dev->pg.mReq_intr, dev->pg.ep_intr );
	dtf_request_free( dev->pg.mReq_in, dev->pg.ep_in );
	dtf_request_free( dev->pg.mReq_out, dev->pg.ep_out );
/* MSE-MOD-S for GV(Docomo) No.3-7 */
//    spin_unlock_irq(&dev->lock);
    spin_unlock_irqrestore(&dev->lock, flags);
/* MSE-MOD-E for GV(Docomo) No.3-7 */

	_dbgmsg( "OUT\n" );
}

static void dtf_cleanup(void)
{
	_dbgmsg( "IN\n" );

	kfree(_dtf_dev);
	_dtf_dev = NULL;

	_dbgmsg( "OUT\n" );
}

static void dtf_request_free(struct usb_request *req, struct usb_ep *ep)
{
	_dbgmsg( "IN\n" );
	if(req) {
		kfree( req->buf );
		_dbgmsg_gadget( "usb_ep_free_request\n" );
		usb_ep_free_request( ep, req );
	}
	_dbgmsg( "OUT\n" );
}

static struct usb_request *dtf_request_new(struct usb_ep *ep, int buffer_size)
{
	struct usb_request *req = usb_ep_alloc_request(ep, GFP_KERNEL);
	
	_dbgmsg_gadget( "_dbgmsg_gadget\n" );
	req = usb_ep_alloc_request(ep, GFP_KERNEL);
	if (!req) {
		_dbgmsg_gadget( "usb_ep_alloc_request error\n" );
		return NULL;
	}

	/* now allocate buffers for the requests */
	req->buf = kmalloc(buffer_size, GFP_KERNEL);
	if (!req->buf) {
		_dbgmsg_gadget( "usb_ep_free_request\n" );
		usb_ep_free_request(ep, req);
		return NULL;
	}

	return req;
}

static inline struct dtf_dev *func_to_dtf(struct usb_function *f)
{
	return container_of(f, struct dtf_dev, function);
}

/* DTF IF */
int dtf_if_in_intr_in(unsigned size, const char *data)
{
	struct usb_ep   *ep;
	struct usb_request *req = NULL;
	int ret;

	_dbgmsg_in( "IN\n" );
	ep = _dtf_dev->pg.ep_intr;
	req = _dtf_dev->pg.mReq_intr;
	
	req->length = size;
	memcpy( req->buf, data, size );
	
	_dbgmsg_gadget( "usb_ep_queue\n" );
	ret = usb_ep_queue( ep, req, GFP_KERNEL );
	
	_dbgmsg_in( "OUT ret: %d\n", ret );
	return ret;
}

int dtf_if_in_bulk_in(unsigned size, const char *data)
{
	struct usb_ep   *ep;
	struct usb_request *req = NULL;
	int ret;

	_dbgmsg_in( "IN\n" );

	ep = _dtf_dev->pg.ep_in;
	req = _dtf_dev->pg.mReq_in;
	
	req->length = size;
	memcpy( req->buf, data, size );

	_dbgmsg_gadget( "usb_ep_queue\n" );
	ret = usb_ep_queue( ep, req, GFP_KERNEL );

	_dbgmsg_in( "OUT\n");
	return ret;
}

void dtf_if_in_set_halt_intr_in(void)
{
	_dbgmsg_in( "IN\n" );
	_dbgmsg_gadget( "usb_ep_set_halt\n" );
	usb_ep_set_halt( _dtf_dev->pg.ep_intr );
	_dbgmsg_in( "OUT\n" );
}

void dtf_if_in_set_halt_bulk_in(void)
{
	_dbgmsg_in( "IN\n" );
	_dbgmsg_gadget( "usb_ep_set_halt\n" );
	usb_ep_set_halt( _dtf_dev->pg.ep_in );
	_dbgmsg_in( "OUT\n" );
}

void dtf_if_in_set_halt_out(void)
{
	_dbgmsg_in( "IN\n" );
	_dbgmsg_gadget( "usb_ep_set_halt\n" );
	usb_ep_set_halt( _dtf_dev->pg.ep_out );
	_dbgmsg_in( "OUT\n" );
}

void dtf_if_in_clear_halt_intr_in(void)
{
	_dbgmsg_in( "IN\n" );
	_dbgmsg_gadget( "usb_ep_clear_halt\n" );
	usb_ep_clear_halt( _dtf_dev->pg.ep_intr );
	_dbgmsg_in( "OUT\n" );
}

void dtf_if_in_clear_halt_bulk_in(void)
{
	_dbgmsg_in( "IN\n" );
	_dbgmsg_gadget( "usb_ep_clear_halt\n" );
	usb_ep_clear_halt( _dtf_dev->pg.ep_in );
	_dbgmsg_in( "OUT\n" );
}

void dtf_if_in_clear_halt_out(void)
{
	_dbgmsg_in( "IN\n" );
	_dbgmsg_gadget( "usb_ep_clear_halt\n" );
	usb_ep_clear_halt( _dtf_dev->pg.ep_out );
	_dbgmsg_in( "OUT\n" );
}

void dtf_if_in_ctrl_in(int length, const char *data)
{
	struct dtf_dev  *dev = _dtf_dev;
	struct usb_composite_dev *cdev = dev->cdev;
	struct usb_request  *req = cdev->req;
	int ret;

	_dbgmsg_in( "IN\n" );
	
	req->zero = 0;
	req->length = length;
	if ( length > 0 )
	{
		memcpy( req->buf, data, length );
	}
	_dbgmsg_gadget( "usb_ep_queue\n" );
	ret = usb_ep_queue(cdev->gadget->ep0, req, GFP_KERNEL);

	if (ret < 0)
	{
		_dbgmsg_in( "usb_ep_queue error %d\n", ret );
	}
	_dbgmsg_in( "OUT\n" );
}

static void dtf_ctrl_complete(struct usb_ep *ep, struct usb_request *req)
{
	_dbgmsg( "IN\n" );
	dtf_if_out_ctrl_complete(req->length, req->actual, (char *)req->buf);
	_dbgmsg( "OUT\n" );
}


void dtf_if_in_ctrl_out(int length)
{
	struct dtf_dev  *dev = _dtf_dev;
	struct usb_composite_dev *cdev = dev->cdev;
	struct usb_request  *req = cdev->req;
	int ret;
	
	_dbgmsg_in( "IN(length=%d)\n", length );
	cdev->gadget->ep0->driver_data = dev;
	req->complete = dtf_ctrl_complete;

	req->zero = 0;
	req->length = length;
	_dbgmsg_gadget( "usb_ep_queue\n" );
	ret = usb_ep_queue(cdev->gadget->ep0, req, GFP_KERNEL);

	if (ret < 0)
	{
		_dbgmsg_in( "usb_ep_queue error %d\n", ret );
	}
	_dbgmsg_in( "OUT\n" );
}

int dtf_if_init(void)
{
	int ret;
	
	_dbgmsg_if( "IN\n ");

	init_waitqueue_head(&poll_wait_read);
	dtf_readable_count = 0;

	dtf_if_init_read_data();
	
	ret = misc_register(&dtf_device);
	_dbgmsg_if( "OUT ret: %d\n", ret);
	return ret;
}

void dtf_if_out_setup(const struct dtf_if_ctrlrequest *ctrlrequest)
{
	struct dtf_if_read_data read_data;
	
	_dbgmsg_out( "IN\n ");
	if ( dtf_if_active == false )
	{
		_dbgmsg_out( "OUT dtf_if_active is false\n ");
		return ;
	}
	/* Event ID set */
	read_data.event_id = DTF_IF_EVENT_SETUP;
	/* Event Data set */
	read_data.ctrl.bRequestType = ctrlrequest->bRequestType;
	read_data.ctrl.bRequest = ctrlrequest->bRequest;
	read_data.ctrl.wValue = ctrlrequest->wValue;
	read_data.ctrl.wIndex = ctrlrequest->wIndex;
	read_data.ctrl.wLength = ctrlrequest->wLength;
	
	dtf_if_add_read_data(&read_data);
	
	if( waitqueue_active( &poll_wait_read ) ){
		_dbgmsg_out("waitqueue_active\n");
		wake_up_interruptible( &poll_wait_read );
	}
	_dbgmsg_out( "OUT\n ");
}

void dtf_if_out_set_alt(int speed_check)
{
	struct dtf_if_read_data read_data;
	
	_dbgmsg_out( "IN\n ");
	if ( dtf_if_active == false )
	{
		_dbgmsg_out( "OUT dtf_if_active is false\n ");
		return ;
	}
	/* Event ID set */
	read_data.event_id = DTF_IF_EVENT_SET_ALT;
	/* Event Data set */
	read_data.speed_check = speed_check;
	
	dtf_if_add_read_data(&read_data);
	
	if( waitqueue_active( &poll_wait_read ) ){
		_dbgmsg_out("waitqueue_active\n");
		wake_up_interruptible( &poll_wait_read );
	}
	_dbgmsg_out( "OUT\n ");
}

void dtf_if_out_disable(int speed_check)
{
	struct dtf_if_read_data read_data;
	
	_dbgmsg_out( "IN(speed_check=%d)\n", speed_check);
	if ( dtf_if_active == false )
	{
		_dbgmsg_out( "OUT dtf_if_active is false\n ");
		return ;
	}
	/* Event ID set */
	read_data.event_id = DTF_IF_EVENT_DISABLE;
	/* Event Data set */
	read_data.speed_check = speed_check;
	
	dtf_if_add_read_data(&read_data);
	
	if( waitqueue_active( &poll_wait_read ) ){
		_dbgmsg_out("waitqueue_active\n");
		wake_up_interruptible( &poll_wait_read );
	}
	_dbgmsg_out( "OUT\n ");
}

void dtf_if_out_suspend(void)
{
	struct dtf_if_read_data read_data;
	
	_dbgmsg_out( "IN\n ");
	if ( dtf_if_active == false )
	{
		_dbgmsg_out( "OUT dtf_if_active is false\n ");
		return ;
	}
	/* Event ID set */
	read_data.event_id = DTF_IF_EVENT_SUSPEND;
	
	dtf_if_add_read_data(&read_data);
	
	if( waitqueue_active( &poll_wait_read ) ){
		_dbgmsg_out("waitqueue_active\n");
		wake_up_interruptible( &poll_wait_read );
	}
	_dbgmsg_out( "OUT\n ");
}

void dtf_if_out_resume(void)
{
	struct dtf_if_read_data read_data;
	
	_dbgmsg_out( "IN\n ");
	if ( dtf_if_active == false )
	{
		_dbgmsg_out( "OUT dtf_if_active is false\n ");
		return ;
	}
	/* Event ID set */
	read_data.event_id = DTF_IF_EVENT_RESUME;
	
	dtf_if_add_read_data(&read_data);
	
	if( waitqueue_active( &poll_wait_read ) ){
		_dbgmsg_out("waitqueue_active\n");
		wake_up_interruptible( &poll_wait_read );
	}
	_dbgmsg_out( "OUT\n ");
}

void dtf_if_out_complete_in(int status)
{
	struct dtf_if_read_data read_data;
	
	_dbgmsg_out( "IN\n ");
	if ( dtf_if_active == false )
	{
		_dbgmsg_out( "OUT dtf_if_active is false\n ");
		return ;
	}
	/* Event ID set */
	read_data.event_id = DTF_IF_EVENT_COMPLETE_IN;
	
	/* Event Data set */
	read_data.status = status;

	dtf_if_add_read_data(&read_data);
	
	if( waitqueue_active( &poll_wait_read ) ){
		_dbgmsg_out("waitqueue_active\n");
		wake_up_interruptible( &poll_wait_read );
	}
	_dbgmsg_out( "OUT\n ");
}

void dtf_if_out_complete_out(int status, int actual, char *buf)
{
	struct dtf_if_read_data read_data;
	
	_dbgmsg_out( "IN\n ");
	if ( dtf_if_active == false )
	{
		_dbgmsg_out( "OUT dtf_if_active is false\n ");
		return ;
	}
	/* Event ID set */
	read_data.event_id = DTF_IF_EVENT_COMPLETE_OUT;
	
	/* Event Data set */
	read_data.status = status;
	read_data.actual = actual;
	memcpy(read_data.data, buf, actual);

	dtf_if_add_read_data(&read_data);
	
	if( waitqueue_active( &poll_wait_read ) ){
		_dbgmsg_out("waitqueue_active\n");
		wake_up_interruptible( &poll_wait_read );
	}
	_dbgmsg_out( "OUT\n ");
}

void dtf_if_out_complete_intr(int status)
{
	struct dtf_if_read_data read_data;
	
	_dbgmsg_out( "IN\n ");
	if ( dtf_if_active == false )
	{
		_dbgmsg_out( "OUT dtf_if_active is false\n ");
		return ;
	}
	/* Event ID set */
	read_data.event_id = DTF_IF_EVENT_COMPLETE_INTR;
	
	/* Event Data set */
	read_data.status = status;

	dtf_if_add_read_data(&read_data);
	
	if( waitqueue_active( &poll_wait_read ) ){
		_dbgmsg_out("waitqueue_active\n");
		wake_up_interruptible( &poll_wait_read );
	}
	_dbgmsg_out( "OUT\n ");
}

void dtf_if_out_ctrl_complete(int length, int actual, char *buf)
{
	struct dtf_if_read_data read_data;
	
	_dbgmsg_out( "IN\n ");
	if ( dtf_if_active == false )
	{
		_dbgmsg_out( "OUT dtf_if_active is false\n ");
		return ;
	}
	/* Event ID set */
	read_data.event_id = DTF_IF_EVENT_CTRL_COMPLETE;
	
	/* Event Data set */
	read_data.length = length;
	read_data.actual = actual;
	memcpy(read_data.data, buf, actual);

	dtf_if_add_read_data(&read_data);
	
	if( waitqueue_active( &poll_wait_read ) ){
		_dbgmsg_out("waitqueue_active\n");
		wake_up_interruptible( &poll_wait_read );
	}
	_dbgmsg_out( "OUT\n ");
}

static void dtf_if_init_read_data(void)
{
	_dbgmsg_if( "IN\n" );
	spin_lock_init(&lock_read_data);
	dtf_if_readable_num = 0;
	dtf_if_readable_head = 0;
	dtf_if_readable_tail = 0;
	_dbgmsg_if( "OUT\n" );
}

static void dtf_if_add_read_data(struct dtf_if_read_data *read_data)
{
/* MSE-ADD-S for GV(Docomo) No.3-7 */
    unsigned long   flags;
/* MSE-ADD-E for GV(Docomo) No.3-7 */
	_dbgmsg_if( "IN(%d)\n", read_data->event_id );

/* MSE-ADD-S for GV(Docomo) No.3-22, 3-7 */
    spin_lock_irqsave(&lock_read_data, flags);
/* MSE-ADD-E for GV(Docomo) No.3-22, 3-7 */
	if ( dtf_if_readable_num >= DTF_IF_READ_DATA_MAX )
	{
		_dbgmsg_if( "Buffer over flow\n" );
/* MSE-ADD-S for GV(Docomo) No.3-22, 3-7 */
        spin_unlock_irqrestore(&lock_read_data, flags);
/* MSE-ADD-E for GV(Docomo) No.3-22, 3-7 */
		return;
	}
/* MSE-DEL-S for GV(Docomo) No.3-22 */
//    spin_lock_irq(&lock_read_data);
/* MSE-DEL-E for GV(Docomo) No.3-22 */
	dtf_if_readable_num++;
	_dbgmsg_if( "dtf_if_readable_num: %d\n", dtf_if_readable_num );
	_dbgmsg_if( "event_id: %d\n", read_data->event_id );
	
	memcpy( &dtf_read_data[dtf_if_readable_tail], read_data, sizeof(struct dtf_if_read_data) );
	dtf_if_readable_tail++;
	if ( dtf_if_readable_tail >= DTF_IF_READ_DATA_MAX )
	{
		dtf_if_readable_tail = 0;
	}
/* MSE-MOD-S for GV(Docomo) No.3-7 */
//    spin_unlock_irq(&lock_read_data);
    spin_unlock_irqrestore(&lock_read_data, flags);
/* MSE-MOD-E for GV(Docomo) No. */
	_dbgmsg_if( "OUT\n" );
}

static struct dtf_if_read_data *dtf_if_get_read_data(void)
{
	struct dtf_if_read_data *read_data;

	_dbgmsg_if( "IN\n" );

/* MSE-DEL-S for GV(Docomo) No.3-20 */
//    spin_lock_irq(&lock_read_data);
/* MSE-DEL-E for GV(Docomo) No.3-20 */
	read_data = &dtf_read_data[dtf_if_readable_head];
	dtf_if_readable_head++;
	if ( dtf_if_readable_head >= DTF_IF_READ_DATA_MAX )
	{
		dtf_if_readable_head = 0;
	}
	dtf_if_readable_num--;
/* MSE-DEL-S for GV(Docomo) No.3-20 */
//    spin_unlock_irq(&lock_read_data);
/* MSE-DEL-E for GV(Docomo) No.3-20 */

	_dbgmsg_if( "dtf_if_readable_num: %d\n", dtf_if_readable_num );
	_dbgmsg_if( "event_id: %d\n", read_data->event_id );
	_dbgmsg_if( "OUT(%d)\n", read_data->event_id );
	return read_data;
}

static int dtf_if_get_read_data_num(void)
{
	_dbgmsg_if( "IN\n" );
	_dbgmsg_if( "OUT(%d)\n", dtf_if_readable_num);
	return dtf_if_readable_num;
}
