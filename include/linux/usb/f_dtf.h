/*
 * Gadget Driver for MCPC DTF
 *
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

#include <linux/usb.h>
#include <linux/usb/android_composite.h>
#include <linux/usb/f_dtf_if.h>

#ifndef __F_DTF_H__
#define __F_DTF_H__

struct sUsb_model_spcfc_func_desc {
	__u8    bLength;
	__u8    bDescriptorType;
	__u8    bDescriptorSubType;
	__u8    bType;
	__u8    bMode[3];
	int     mMode_num;
} __attribute__ ((packed));;

struct sDtf_pg {
    unsigned mCtrl_id;
    unsigned mData_id;

    struct usb_ep *ep_intr;
    struct usb_ep *ep_in;
    struct usb_ep *ep_out;

    struct usb_request *mReq_intr;
    struct usb_request *mReq_in;
    struct usb_request *mReq_out;
};

struct dtf_dev {
	struct usb_function function;
	struct usb_composite_dev *cdev;
	spinlock_t lock;
    struct sDtf_pg pg;
    int    mCtrl_ep_enbl;
    int    mData_ep_enbl;
};

int dtf_if_in_intr_in(unsigned size, const char *data);
int dtf_if_in_bulk_in(unsigned size, const char *data);
void dtf_if_in_set_halt_intr_in(void);
void dtf_if_in_set_halt_bulk_in(void);
void dtf_if_in_set_halt_out(void);
void dtf_if_in_clear_halt_intr_in(void);
void dtf_if_in_clear_halt_bulk_in(void);
void dtf_if_in_clear_halt_out(void);
void dtf_if_in_ctrl_in(int length, const char *data);
void dtf_if_in_ctrl_out(int length);

#endif /* __F_DTF_H__ */
