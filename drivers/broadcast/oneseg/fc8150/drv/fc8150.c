#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/poll.h>
#include <linux/vmalloc.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#include <mach/gpio.h>
#include <mach/board_lge.h> /* miseon.kim 2012.06.21 FC8150 Device Release */

#include "fc8150.h"
#include "bbm.h"
#include "fci_oal.h"
#include "fci_tun.h"
#include "fc8150_regs.h"
#include "fc8150_isr.h"
#include "fci_hal.h"
#include <linux/wakelock.h> 		/* wake_lock, unlock */

// eric0.kim@lge.com [2012.08.07]
#define ONESEG_KDDI_ANT_CTL


#ifdef ONESEG_KDDI_ANT_CTL
#include "../../../../../arch/arm/mach-msm/lge/j1/board-j1.h"
#define ONESEG_ANT_SEL_1       	    PM8921_GPIO_PM_TO_SYS(11)
#define ONESEG_ANT_SEL_2       	    PM8921_GPIO_PM_TO_SYS(12)
#endif

#define INT_THR_SIZE (128*1024)


ISDBT_INIT_INFO_T *hInit;

u32 totalTS=0;
u32 totalErrTS=0;
unsigned char ch_num = 0;
u32 remain_ts_len=0;
u8 remain_ts_buf[INT_THR_SIZE];

u8 scan_mode;
u8 dm_en;

ISDBT_MODE driver_mode = ISDBT_POWEROFF;

int isdbt_open (struct inode *inode, struct file *filp);
long isdbt_ioctl (struct file *filp, unsigned int cmd, unsigned long arg);
int isdbt_release (struct inode *inode, struct file *filp);
ssize_t isdbt_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);

struct wake_lock oneseg_wakelock;
static wait_queue_head_t isdbt_isr_wait;

#define RING_BUFFER_SIZE	(128 * 1024)

//GPIO(RESET & INTRRUPT) Setting
#define FC8150_NAME		"isdbt"

#define GPIO_ISDBT_IRQ 77
#define GPIO_ISDBT_PWR_EN 85
#define GPIO_ISDBT_RST 1

static DEFINE_MUTEX(ringbuffer_lock);


void isdbt_hw_setting(void)
{
	gpio_request(GPIO_ISDBT_PWR_EN, "ISDBT_EN");
	udelay(50);
	gpio_direction_output(GPIO_ISDBT_PWR_EN, 0);//Rev.C 에서 0->1

    /* miseon.kim 2012.06.21 FC8150 Device Release */
    if(lge_get_board_revno() < HW_REV_C) 
    	gpio_set_value(GPIO_ISDBT_PWR_EN, 1);//Rev.C 에서 1->0

	if(gpio_request(GPIO_ISDBT_IRQ, "ISDBT_IRQ_INT"))		
		PRINTF(0,"ISDBT_IRQ_INT Port request error!!!\n");

	gpio_direction_input(GPIO_ISDBT_IRQ);  
	
	gpio_request(GPIO_ISDBT_RST, "ISDBT_RST");
	udelay(50);
	gpio_direction_output(GPIO_ISDBT_RST, 1);
}

//POWER_ON & HW_RESET & INTERRUPT_CLEAR
void isdbt_hw_init(void)
{
	int i=0;
			
	while(driver_mode == ISDBT_DATAREAD)
	{
		msWait(100);
		if(i++>5)
			break;
	}

	//PRINTF(0, "isdbt_hw_init \n");
	gpio_set_value(GPIO_ISDBT_RST, 1);

    /* miseon.kim 2012.06.21 FC8150 Device Release */
    if(lge_get_board_revno() >= HW_REV_C)
    	gpio_set_value(GPIO_ISDBT_PWR_EN, 1); //Rev.C 에서 0->1
    else
    	gpio_set_value(GPIO_ISDBT_PWR_EN, 0); //Rev.C 에서 0->1
    
	msWait(10);
	gpio_set_value(GPIO_ISDBT_RST, 0);
	msWait(5);
	gpio_set_value(GPIO_ISDBT_RST, 1);

	driver_mode = ISDBT_POWERON;
	wake_lock(&oneseg_wakelock);

#ifdef ONESEG_KDDI_ANT_CTL
	if(lge_get_board_revno() < HW_REV_E) 
	{
		gpio_set_value(ONESEG_ANT_SEL_1, 0);
		gpio_set_value(ONESEG_ANT_SEL_2, 1);
	}
#endif

	
}

//POWER_OFF
void isdbt_hw_deinit(void)
{
	driver_mode = ISDBT_POWEROFF;
	//gpio_set_value(GPIO_ISDBT_RST, 0);
	//mdelay(1);

    /* miseon.kim 2012.06.21 FC8150 Device Release */
    if(lge_get_board_revno() >= HW_REV_C)
    	gpio_set_value(GPIO_ISDBT_PWR_EN, 0);//Rev.C 에서 1->0
    else
    	gpio_set_value(GPIO_ISDBT_PWR_EN, 1);//Rev.C 에서 1->0

	wake_unlock(&oneseg_wakelock);
}

#if 1
static u8 isdbt_isr_sig=0;
static struct task_struct *isdbt_kthread = NULL;

static irqreturn_t isdbt_irq(int irq, void *dev_id)
{
	isdbt_isr_sig++;
	wake_up(&isdbt_isr_wait);
	return IRQ_HANDLED;
}
#endif
int data_callback(u32 hDevice, u8 *data, int len)
{
	ISDBT_INIT_INFO_T *hInit;
	struct list_head *temp;
	int i;
	
	totalTS +=(len/188);

	for(i=0;i<len;i+=188)
	{
		if((data[i+1]&0x80)||data[i]!=0x47)
			totalErrTS++;
	}

	hInit = (ISDBT_INIT_INFO_T *)hDevice;

	list_for_each(temp, &(hInit->hHead))
	{
		ISDBT_OPEN_INFO_T *hOpen;

		hOpen = list_entry(temp, ISDBT_OPEN_INFO_T, hList);

		if(hOpen->isdbttype == TS_TYPE)
		{
			mutex_lock(&ringbuffer_lock);
			if(fci_ringbuffer_free(&hOpen->RingBuffer) < (len+2) ) 
			{
				mutex_unlock(&ringbuffer_lock);
				PRINTF(0, "fc8150 data_callback : ring buffer is full\n");
				return 0;
			}

			FCI_RINGBUFFER_WRITE_BYTE(&hOpen->RingBuffer, len >> 8);
			FCI_RINGBUFFER_WRITE_BYTE(&hOpen->RingBuffer, len & 0xff);

			fci_ringbuffer_write(&hOpen->RingBuffer, data, len);

			wake_up_interruptible(&(hOpen->RingBuffer.queue));
			
			mutex_unlock(&ringbuffer_lock);
		}
	}

	return 0;
}
#if 1
static int isdbt_thread(void *hDevice)
{
	ISDBT_INIT_INFO_T *hInit = (ISDBT_INIT_INFO_T *)hDevice;
	
	set_user_nice(current, -20);
	
	PRINTF(hInit, "isdbt_kthread enter\n");

	BBM_TS_CALLBACK_REGISTER((u32)hInit, data_callback);

	init_waitqueue_head(&isdbt_isr_wait);

	while(1)
	{
		wait_event_interruptible(isdbt_isr_wait, isdbt_isr_sig || kthread_should_stop());

		if(driver_mode == ISDBT_POWERON)
		{
			driver_mode = ISDBT_DATAREAD;
			BBM_ISR(hInit);
			driver_mode = ISDBT_POWERON;
		}

		if(isdbt_isr_sig>0)
		{
			isdbt_isr_sig--;
		}
	
		if (kthread_should_stop())
			break;
	}

	BBM_TS_CALLBACK_DEREGISTER();
	
	PRINTF(hInit, "isdbt_kthread exit\n");

	return 0;
}
#endif
static struct file_operations isdbt_fops = 
{
	.owner		= THIS_MODULE,
	.unlocked_ioctl	= isdbt_ioctl,
	.open		= isdbt_open,
	.read		= isdbt_read,
	.release	= isdbt_release,
};

static struct miscdevice fc8150_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = FC8150_NAME,
    .fops = &isdbt_fops,
};

int isdbt_open (struct inode *inode, struct file *filp)
{
	ISDBT_OPEN_INFO_T *hOpen;

	PRINTF(hInit, "isdbt open\n");

	hOpen = (ISDBT_OPEN_INFO_T *)kmalloc(sizeof(ISDBT_OPEN_INFO_T), GFP_KERNEL);

	hOpen->buf = (u8 *)kmalloc(RING_BUFFER_SIZE, GFP_KERNEL);
	hOpen->isdbttype = 0;

	list_add(&(hOpen->hList), &(hInit->hHead));

	hOpen->hInit = (HANDLE *)hInit;

	if(hOpen->buf == NULL)
	{
		PRINTF(hInit, "ring buffer malloc error\n");
		return -ENOMEM;
	}

	fci_ringbuffer_init(&hOpen->RingBuffer, hOpen->buf, RING_BUFFER_SIZE);

	filp->private_data = hOpen;

	return 0;
}

 ssize_t isdbt_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
    s32 avail;
    s32 non_blocking = filp->f_flags & O_NONBLOCK;
    ISDBT_OPEN_INFO_T *hOpen = (ISDBT_OPEN_INFO_T*)filp->private_data;
    struct fci_ringbuffer *cibuf = &hOpen->RingBuffer;
    ssize_t len, total_len = 0;
 
    if (!cibuf->data || !count)
    {
        //PRINTF(hInit, " return 0\n");
        return 0;
    }
    
    if (non_blocking && (fci_ringbuffer_empty(cibuf)))
    {
        //PRINTF(hInit, "return EWOULDBLOCK\n");
        return -EWOULDBLOCK;
    }

	mutex_lock(&ringbuffer_lock);
	
// add to fill user buffer fully - hyewon.eum@lge.com 20110825[Start]
	if((remain_ts_len != 0) && (remain_ts_len <= INT_THR_SIZE))
	{
		if(count >= remain_ts_len)
		{
			if(copy_to_user(buf, remain_ts_buf, remain_ts_len))
			{
				PRINTF(hInit, "write to user buffer falied\n");
				mutex_unlock(&ringbuffer_lock);
				return 0;
			}

			total_len += remain_ts_len;
			PRINTF(hInit, "remian data copied %d !!!\n",remain_ts_len);
		}

		remain_ts_len = 0;
	}
// add to fill user buffer fully - hyewon.eum@lge.com 20110825[End]

    while(1)
    {
        avail = fci_ringbuffer_avail(cibuf);
        
        if (avail < 4)
        {
            //PRINTF(hInit, "avail %d\n", avail);
            break;
        }
        
        len = FCI_RINGBUFFER_PEEK(cibuf, 0) << 8;
        len |= FCI_RINGBUFFER_PEEK(cibuf, 1);

		// add to fill user buffer fully - hyewon.eum@lge.com 20110825[Start]
		if ((avail < len + 2) || ((count-total_len) == 0))
		{
			PRINTF(hInit, "not enough data or user buffer avail %d, len %d, remain user buffer %d\n",avail, len, count-total_len);
			break;
		}

		if(count-total_len < len)
		{
			int read_size;

			read_size = count-total_len;
			remain_ts_len = len - read_size;			
			
			FCI_RINGBUFFER_SKIP(cibuf, 2); //skip 2 byte for length field
			
			total_len += fci_ringbuffer_read_user(cibuf, buf + total_len, read_size);
			fci_ringbuffer_read(cibuf, remain_ts_buf, remain_ts_len);
			PRINTF(hInit, "user buf not enough, read %d , write %d , remain %d, avail %d\n",len, read_size, remain_ts_len, avail);
			PRINTF(hInit, "total_len %d, count %d\n",total_len, count);
			break;
		}
		// add to fill user buffer fully - hyewon.eum@lge.com 20110825[End]
		
		FCI_RINGBUFFER_SKIP(cibuf, 2);

		total_len += fci_ringbuffer_read_user(cibuf, buf + total_len, len);

	}
	mutex_unlock(&ringbuffer_lock);
	
	return total_len;
}

static  ssize_t ioctl_isdbt_read(ISDBT_OPEN_INFO_T *hOpen  ,void __user *arg)
{
	struct broadcast_dmb_data_info __user* puserdata = (struct broadcast_dmb_data_info  __user*)arg;
	int ret = -ENODEV;
	size_t count;
	DMB_BB_HEADER_TYPE dmb_header;
	static int read_count = 0;
	char *buf;
	
       s32 avail;
       //s32 non_blocking = filp->f_flags & O_NONBLOCK;
       // ISDBT_OPEN_INFO_T *hOpen = (ISDBT_OPEN_INFO_T*)filp->private_data;
   	struct fci_ringbuffer *cibuf = &hOpen->RingBuffer;
    	ssize_t len, total_len = 0;
	
	buf = puserdata->data_buf + sizeof(DMB_BB_HEADER_TYPE);
	count = puserdata->data_buf_size - sizeof(DMB_BB_HEADER_TYPE);
	count = (count/188)*188;
 
    if (!cibuf->data || !count)
    {
        PRINTF(hInit, " ioctl_isdbt_read return 0\n");
        return 0;
    }
    
    // if (non_blocking && (fci_ringbuffer_empty(cibuf)))
    if ( fci_ringbuffer_empty(cibuf) )
    {
        //PRINTF(hInit, "return fci_ringbuffer_empty EWOULDBLOCK\n");
        return -EWOULDBLOCK;
    }

	mutex_lock(&ringbuffer_lock);
	
// add to fill user buffer fully - hyewon.eum@lge.com 20110825[Start]
	if((remain_ts_len != 0) && (remain_ts_len <= INT_THR_SIZE))
	{
		if(count >= remain_ts_len)
		{
			if(copy_to_user(buf, remain_ts_buf, remain_ts_len))
			{
				PRINTF(hInit, "write to user buffer falied\n");
				mutex_unlock(&ringbuffer_lock);
				return 0;
			}

			total_len += remain_ts_len;
			PRINTF(hInit, "remian data copied %d !!!\n",remain_ts_len);
		}

		remain_ts_len = 0;
	}
// add to fill user buffer fully - hyewon.eum@lge.com 20110825[End]

    while(1)
    {
        avail = fci_ringbuffer_avail(cibuf);
        
        if (avail < 4)
        {
            //PRINTF(hInit, "avail %d\n", avail);
            break;
        }
        
        len = FCI_RINGBUFFER_PEEK(cibuf, 0) << 8;
        len |= FCI_RINGBUFFER_PEEK(cibuf, 1);

		// add to fill user buffer fully - hyewon.eum@lge.com 20110825[Start]
		if ((avail < len + 2) || ((count-total_len) == 0))
		{
			PRINTF(hInit, "not enough data or user buffer avail %d, len %d, remain user buffer %d\n",avail, len, count-total_len);
			break;
		}

		if(count-total_len < len)
		{
			int read_size;

			read_size = count-total_len;
			remain_ts_len = len - read_size;			
			
			FCI_RINGBUFFER_SKIP(cibuf, 2); //skip 2 byte for length field
			
			total_len += fci_ringbuffer_read_user(cibuf, buf + total_len, read_size);
			fci_ringbuffer_read(cibuf, remain_ts_buf, remain_ts_len);
			PRINTF(hInit, "user buf not enough, read %d , write %d , remain %d, avail %d\n",len, read_size, remain_ts_len, avail);
			PRINTF(hInit, "total_len %d, count %d\n",total_len, count);
			break;
		}
		// add to fill user buffer fully - hyewon.eum@lge.com 20110825[End]
		
		FCI_RINGBUFFER_SKIP(cibuf, 2);

		total_len += fci_ringbuffer_read_user(cibuf, buf + total_len, len);

	}
	
	mutex_unlock(&ringbuffer_lock);

	dmb_header.data_type = DMB_BB_DATA_TS;
	dmb_header.size = (unsigned short)total_len;
	dmb_header.subch_id = ch_num;//0xFF;
	dmb_header.reserved = read_count++;
	
	ret = copy_to_user(puserdata->data_buf, &dmb_header, sizeof(DMB_BB_HEADER_TYPE));
	
	puserdata->copied_size = total_len + sizeof(DMB_BB_HEADER_TYPE);
	
	return ret;
}
 
int isdbt_release (struct inode *inode, struct file *filp)
{
	ISDBT_OPEN_INFO_T *hOpen;

	hOpen = filp->private_data;

	hOpen->isdbttype = 0;

	list_del(&(hOpen->hList));

	kfree(hOpen->buf);
	kfree(hOpen);

	return 0;
}

int fc8150_if_test(void)
{
	int res=0;
	int i;
	u16 wdata=0;
	u32 ldata=0;
	u8 data=0;
	u8 temp = 0;

	PRINTF(0, "fc8150_if_test Start!!!\n");
	for(i=0;i<100;i++) {
		BBM_BYTE_WRITE(0, 0xa4, i&0xff);
		BBM_BYTE_READ(0, 0xa4, &data);
		if((i&0xff) != data) {
			PRINTF(0, "fc8150_if_btest!   i=0x%x, data=0x%x\n", i&0xff, data);
			res=1;
		}
	}

	
	for(i = 0 ; i < 100 ; i++) {
		BBM_WORD_WRITE(0, 0xa4, i&0xffff);
		BBM_WORD_READ(0, 0xa4, &wdata);
		if((i & 0xffff) != wdata) {
			PRINTF(0, "fc8150_if_wtest!   i=0x%x, data=0x%x\n", i&0xffff, wdata);
			res = 1;
		}
	}

	for(i = 0 ; i < 100; i++) {
		BBM_LONG_WRITE(0, 0xa4, i&0xffffffff);
		BBM_LONG_READ(0, 0xa4, &ldata);
		if((i&0xffffffff) != ldata) {
			PRINTF(0, "fc8150_if_ltest!   i=0x%x, data=0x%x\n", i&0xffffffff, ldata);
			res=1;
		}
	}
	
	for(i=0 ; i < 100 ; i++) {
		temp = i & 0xff;
		BBM_TUNER_WRITE(NULL, 0x52, 0x01, &temp, 0x01);
		BBM_TUNER_READ(NULL, 0x52, 0x01, &data, 0x01);
		if((i & 0xff) != data)
			PRINTF(0, "FC8150 tuner test (0x%x,0x%x)\n", i & 0xff, data);
	}

	PRINTF(0, "fc8150_if_test End!!!\n");

	return res;
}

static int isdbt_sw_lock_check(HANDLE hDevice)
{
	int res = BBM_NOK;
	unsigned char lock_data;
	
	res = BBM_READ(hDevice, 0x5053, &lock_data);

	if(res)
		return res;

	if(lock_data & 0x01)
		res = BBM_OK;
	else
		res = BBM_NOK;

	return res;
}

void isdbt_ber_per_measure(HANDLE hDevice, u32 *ui32BER, u32 *ui32PER, u16 *CN)
{
	u32 main_err_bits = 0;
	u16 main_err_rsps = 0, main_rxd_rsps = 0;
	u32 dmp_err_bits = 0, dmp_rxd_bits = 0; // , demap_ber = 0;
	u8 cnval;

	BBM_WRITE(hDevice, BBM_REQ_BER, 0x0e);
	
	BBM_LONG_READ(hDevice, 0x5024, &main_err_bits);
	BBM_WORD_READ(hDevice, 0x5022, &main_err_rsps);
	BBM_WORD_READ(hDevice, 0x5020, &main_rxd_rsps);
	
	BBM_LONG_READ(hDevice, 0x5044, &dmp_err_bits);
	BBM_LONG_READ(hDevice, 0x5040, &dmp_rxd_bits);
	BBM_READ(hDevice, 0x4063, &cnval);

	*CN = cnval;
	
	if(dmp_rxd_bits>0)
	{
		if(dmp_err_bits<42949)
			*ui32BER = (dmp_err_bits * 100000 / dmp_rxd_bits) ;
		else
			*ui32BER = (dmp_err_bits * 100 / dmp_rxd_bits) * 1000 ;
		
		*ui32PER = ((main_err_rsps * 100000) / main_rxd_rsps) ;
	}
	else
	{
		*ui32BER = 100000;
		*ui32PER = 100000;
	}
	
	PRINTF(hDevice, "ISDBT MEASURE BER : %d, PER : %d\n", *ui32BER, *ui32PER);
}

u8 isdbt_dm_enable(HANDLE hDevice, u8 mode)
{
	if(mode) {
		if(!dm_en)
		{
			BBM_WRITE(hDevice, BBM_DM_AUTO_ENABLE, 2);
			dm_en = 1;
		}
	}
	else
	{
		if(dm_en)
		{
			BBM_WRITE(hDevice, BBM_DM_AUTO_ENABLE, 0);
			dm_en = 0;
		}
	}
	
	return BBM_OK;
}

void isdbt_get_signal_info(HANDLE hDevice,u16 *lock, u32 *ui32BER, u32 *ui32PER, s32 *rssi, u16 *cn)
{
	//u32 main_viterbi_ber, main_rs_per, inter_viterbi_ber, inter_rs_per, demap_ber;
	
	struct dm_st {
		u8  start;
		s8  rssi;
		u8  wscn;
		u8  reserved;
		u16 main_rxd_rsps;
		u16 main_err_rsps;
		u32 main_err_bits;
		u32 dmp_rxd_bits;
		u32 dmp_err_bits;
		u16 inter_rxd_rsps;
		u16 inter_err_rsps;
		u32 inter_err_bits;
		u8  lna_code;
		u8  rfvga;
		u8  k;
		u8  csf_gain;
		u8  pga_gain;
		u8  extlna;
		u8  high_current_mode_gain;
		u8  extlna_gain;
	} dm;

    if(isdbt_sw_lock_check(hDevice))
	{
        *lock = 0;
		*ui32BER = 10000;
		*ui32PER = 10000;
		*rssi = -100;
		*cn = 0;
		
		return;
	}
    else
    {
        *lock = 1;
    }
	
	isdbt_dm_enable(hDevice, 1);
	
	BBM_WRITE(hDevice, 0x5000, 0x0e);
	BBM_BULK_READ(hDevice, BBM_DM_DATA, (u8*) &dm + 1, sizeof(dm) - 1);

	//PRINTF(hDevice, "main_rxd_rsps: %d, dmp_rxd_bits: %d, inter_rxd_rsps: %d\n", dm.main_rxd_rsps, dm.dmp_rxd_bits, dm.inter_rxd_rsps);
		
	if(dm.inter_rxd_rsps)
		*ui32PER = ((dm.inter_err_rsps * 10000) / dm.inter_rxd_rsps);
	else
		*ui32PER = 10000;

	if(dm.dmp_rxd_bits)
		*ui32BER = ((dm.dmp_err_bits * 10000) / dm.dmp_rxd_bits);
	else
		*ui32BER = 10000;
	
	*rssi = dm.rssi;
	*cn = dm.wscn;
	
	//PRINTF(hDevice, "[FC8150]LOCK :%d, BER: %d, PER : %d, RSSI : %d, CN : %d\n",lock, *ui32BER, *ui32PER, *rssi, *cn);
}

void isdbt_set_scanmode(HANDLE hDevice, u8 scanmode)
{
	if(scanmode)
	{
		if(!scan_mode)
		{
			BBM_WRITE(hDevice, 0x3040, 0x00);
			BBM_WRITE(hDevice, 0x3004, 0x02);
			BBM_WRITE(hDevice, 0x3006, 0x02);
			BBM_WRITE(hDevice, 0x2020, 0x18);
			BBM_WRITE(hDevice, 0x2021, 0x14);
			BBM_WRITE(hDevice, 0x2022, 0xea);
			BBM_WRITE(hDevice, 0x2082, 0x70);
			BBM_WRITE(hDevice, 0x2083, 0x70);
			BBM_WRITE(hDevice, 0x2084, 0x70);
			BBM_WRITE(hDevice, 0x2085, 0x60);

			scan_mode=1;
			PRINTF(hDevice, "SCAN MODE ON\n");
		}
	}
	else
	{
		if(scan_mode)
		{
			BBM_WRITE(hDevice, 0x3040, 0x27);
			BBM_WRITE(hDevice, 0x3004, 0x04);
			BBM_WRITE(hDevice, 0x3006, 0x04);
			BBM_WRITE(hDevice, 0x2020, 0x10);
			BBM_WRITE(hDevice, 0x2021, 0x0e);
			BBM_WRITE(hDevice, 0x2022, 0x4a);
			BBM_WRITE(hDevice, 0x2082, 0x45);
			BBM_WRITE(hDevice, 0x2083, 0x5f);
			BBM_WRITE(hDevice, 0x2084, 0x37);
			BBM_WRITE(hDevice, 0x2085, 0x30);

			scan_mode=0;
			PRINTF(hDevice, "SCAN MODE OFF\n");
		}
	}

}

long isdbt_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{
	s32 res = BBM_NOK;
	
	void __user *argp = (void __user *)arg;
	
	s32 err = 0;
	s32 size = 0;
	int uData=0;
	ISDBT_OPEN_INFO_T *hOpen;
		
	IOCTL_ISDBT_SIGNAL_INFO isdbt_signal_info;
	
	if(_IOC_TYPE(cmd) != ISDBT_IOC_MAGIC)
	{
		return -EINVAL;
	}
	
	if(_IOC_NR(cmd) >= IOCTL_MAXNR)
	{
		return -EINVAL;
	}

	hOpen = filp->private_data;

	size = _IOC_SIZE(cmd);
	
	// PRINTF(0, "isdbt_ioctl  0x%x\n", cmd);	

	switch(cmd)
	{
		case IOCTL_ISDBT_POWER_ON:
		case LGE_BROADCAST_DMB_IOCTL_ON:			
			PRINTF(0, "IOCTL_ISDBT_POWER_ON \n");	
			
			isdbt_hw_init();
			res = BBM_I2C_INIT(hInit, FCI_I2C_TYPE);

			res |= BBM_PROBE(hInit);
			
			if(res) {
				PRINTF(hInit, "FC8150 Initialize Fail \n");
				break;
			}
        
			res |= BBM_INIT(hInit);
			res |= BBM_TUNER_SELECT(hInit, FC8150_TUNER, 0);
			scan_mode = 0;
			dm_en = 0;
            
			if(res)
			PRINTF(0, "IOCTL_ISDBT_POWER_ON FAIL \n");			
			else
			PRINTF(0, "IOCTL_ISDBT_POWER_OK \n");

			//fc8150_if_test();
			break;
		case IOCTL_ISDBT_POWER_OFF:
		case LGE_BROADCAST_DMB_IOCTL_OFF:
			
			PRINTF(0, "IOCTL_ISDBT_POWER_OFF \n");
			isdbt_hw_deinit();	
			res = BBM_OK;
			break;
		case IOCTL_ISDBT_SCAN_FREQ:
		{
			u32 f_rf;
			err = copy_from_user((void *)&uData, (void *)arg, size);

			f_rf = (uData- 13) * 6000 + 473143;
			//PRINTF(0, "IOCTL_ISDBT_SCAN_FREQ  f_rf : %d\n", f_rf);
			if(lge_get_board_revno() >= HW_REV_C)
				isdbt_dm_enable(hInit, 0);
			
			isdbt_set_scanmode(hInit, 1);
            
			res = BBM_TUNER_SET_FREQ(hInit, f_rf);
			res |= BBM_SCAN_STATUS(hInit);
		}
			break;
		case IOCTL_ISDBT_SET_FREQ:
		{
			u32 f_rf;
			totalTS=0;
			totalErrTS=0;
			remain_ts_len=0;

			err = copy_from_user((void *)&uData, (void *)arg, size);
			mutex_lock(&ringbuffer_lock);
			fci_ringbuffer_flush(&hOpen->RingBuffer);
			mutex_unlock(&ringbuffer_lock);
			f_rf = (uData- 13) * 6000 + 473143;
			//PRINTF(0, "IOCTL_ISDBT_SET_FREQ chNum : %d, f_rf : %d\n", uData, f_rf);

			if(lge_get_board_revno() >= HW_REV_C)
				isdbt_dm_enable(hInit, 0);

			isdbt_set_scanmode(hInit, 0);	
			res = BBM_TUNER_SET_FREQ(hInit, f_rf);
			res |= BBM_SCAN_STATUS(hInit);
		}
			break;
		case IOCTL_ISDBT_GET_LOCK_STATUS:
			PRINTF(0, "IOCTL_ISDBT_GET_LOCK_STATUS \n");	
			res = isdbt_sw_lock_check(hInit);
			if(res)
				uData=0;
			else
				uData=1;
			err |= copy_to_user((void *)arg, (void *)&uData, size);
			res = BBM_OK;
			break;
		case IOCTL_ISDBT_GET_SIGNAL_INFO:

			if(lge_get_board_revno() >= HW_REV_C)
				isdbt_get_signal_info(hInit, &isdbt_signal_info.lock, &isdbt_signal_info.ber, &isdbt_signal_info.per, &isdbt_signal_info.rssi, &isdbt_signal_info.cn);
			else
			{
				if(isdbt_sw_lock_check(hInit))
					isdbt_signal_info.lock=0;
				else 
					isdbt_signal_info.lock=1;

				isdbt_ber_per_measure(hInit, &isdbt_signal_info.ber, &isdbt_signal_info.per, &isdbt_signal_info.cn);
				BBM_TUNER_GET_RSSI(hInit, &isdbt_signal_info.rssi);
			}
				
			isdbt_signal_info.ErrTSP = totalErrTS;
			isdbt_signal_info.TotalTSP = totalTS;
			
			totalTS=totalErrTS=0;

			err |= copy_to_user((void *)arg, (void *)&isdbt_signal_info, size);
			
			res = BBM_OK;
			
			break;
		case IOCTL_ISDBT_START_TS:
			hOpen->isdbttype = TS_TYPE;
			res = BBM_OK;
			break;
		case IOCTL_ISDBT_STOP_TS:
		case LGE_BROADCAST_DMB_IOCTL_USER_STOP:			
			hOpen->isdbttype = 0;
			res = BBM_OK;
			break;

		case LGE_BROADCAST_DMB_IOCTL_SET_CH:
			{
				struct broadcast_dmb_set_ch_info udata;
				u32 f_rf;
				//PRINTF(0, "LGE_BROADCAST_DMB_IOCTL_SET_CH \n");	
				
				if(lge_get_board_revno() >= HW_REV_C)
					isdbt_dm_enable(hInit, 0);
				
				if(copy_from_user(&udata, argp, sizeof(struct broadcast_dmb_set_ch_info)))
				{
					PRINTF(0,"broadcast_dmb_set_ch fail!!! \n");
					res = -1;
				}
				else
				{
					f_rf = (udata.ch_num- 13) * 6000 + 473143;
					//PRINTF(0, "IOCTL_ISDBT_SET_FREQ freq:%d, RF:%d\n",udata.ch_num,f_rf);
					if(udata.mode == LGE_BROADCAST_OPMODE_ENSQUERY)
						isdbt_set_scanmode(hInit, 1);
                    else
                        isdbt_set_scanmode(hInit, 0);
					
					res = BBM_TUNER_SET_FREQ(hInit, f_rf);

					if(udata.mode == LGE_BROADCAST_OPMODE_ENSQUERY)
					{
						res |= BBM_SCAN_STATUS(hInit);
						if(res != BBM_OK)
						{
							PRINTF(0, " BBM_SCAN_STATUS  Unlock \n");
							break;
						}
						PRINTF(0, " BBM_SCAN_STATUS : Lock \n");
					}

					// PRINTF(0, "IOCTL_ISDBT_SET_FREQ \n");	
					totalTS=0;
					totalErrTS=0;
					remain_ts_len=0;
					ch_num = udata.ch_num;
					mutex_lock(&ringbuffer_lock);
					fci_ringbuffer_flush(&hOpen->RingBuffer);
					mutex_unlock(&ringbuffer_lock);
					//f_rf = (udata.ch_num- 13) * 6000  + 473143;
					//res = BBM_TUNER_SET_FREQ(hInit, f_rf);
					hOpen->isdbttype = TS_TYPE;
				}
			}
			break;
		case LGE_BROADCAST_DMB_IOCTL_GET_SIG_INFO:
			{
				struct broadcast_dmb_sig_info udata;				
				//PRINTF(0, "LGE_BROADCAST_DMB_IOCTL_GET_SIG_INFO \n");		
				
				if(lge_get_board_revno() >= HW_REV_C)
					isdbt_get_signal_info(hInit, &isdbt_signal_info.lock, &isdbt_signal_info.ber, &isdbt_signal_info.per, &isdbt_signal_info.rssi, &isdbt_signal_info.cn);
				else
				{
					if(isdbt_sw_lock_check(hInit))
						isdbt_signal_info.lock=0;
					else 
						isdbt_signal_info.lock=1;
					
					isdbt_ber_per_measure(hInit, &isdbt_signal_info.ber, &isdbt_signal_info.per, &isdbt_signal_info.cn);
					BBM_TUNER_GET_RSSI(hInit, &isdbt_signal_info.rssi);
				}
				
				isdbt_signal_info.ErrTSP = totalErrTS;
				isdbt_signal_info.TotalTSP = totalTS;
				
				totalTS=totalErrTS=0;

				udata.info.oneseg_info.lock = (int)isdbt_signal_info.lock;
				udata.info.oneseg_info.ErrTSP = (int)isdbt_signal_info.ErrTSP;
				udata.info.oneseg_info.TotalTSP = (int)isdbt_signal_info.TotalTSP;

				udata.info.oneseg_info.ber = (int)isdbt_signal_info.ber;
				udata.info.oneseg_info.per = (int)isdbt_signal_info.per;
				udata.info.oneseg_info.rssi = (int)isdbt_signal_info.rssi;
				udata.info.oneseg_info.cn = (int)isdbt_signal_info.cn;
                udata.info.oneseg_info.antenna_level = 0;

				if(copy_to_user((void *)argp, &udata, sizeof(struct broadcast_dmb_sig_info)))
				{
					PRINTF(0,"broadcast_dmb_get_sig_info copy_to_user error!!! \n");
					res = BBM_NOK;
				}
				else
				{
                    PRINTF(0, "LOCK :%d, BER: %d, PER : %d, RSSI : %d, CN : %d\n",
                        udata.info.oneseg_info.lock, 
                        udata.info.oneseg_info.ber, 
                        udata.info.oneseg_info.per, 
                        udata.info.oneseg_info.rssi, 
                        udata.info.oneseg_info.cn);
                    
					res = BBM_OK;
				}
			}
			break;

		case LGE_BROADCAST_DMB_IOCTL_GET_DMB_DATA:
			//PRINTF(0, "LGE_BROADCAST_DMB_IOCTL_GET_DMB_DATA \n");
			res = ioctl_isdbt_read(hOpen,argp);
			break;
		case LGE_BROADCAST_DMB_IOCTL_OPEN:
		case LGE_BROADCAST_DMB_IOCTL_CLOSE:
		case LGE_BROADCAST_DMB_IOCTL_RESYNC:
		case LGE_BROADCAST_DMB_IOCTL_DETECT_SYNC:
		case LGE_BROADCAST_DMB_IOCTL_GET_CH_INFO:
		case LGE_BROADCAST_DMB_IOCTL_RESET_CH:
		case LGE_BROADCAST_DMB_IOCTL_SELECT_ANTENNA:
			PRINTF(0, "LGE_BROADCAST_DMB_IOCTL_SKIP \n");	
            res = BBM_OK;
			break;
		default:
			PRINTF(hInit, "isdbt ioctl error!\n");
			res = BBM_NOK;
			break;
	}
	
	if(err < 0)
	{
		PRINTF(hInit, "copy to/from user fail : %d", err);
		res = BBM_NOK;
	}
	return res; 
}

int isdbt_init(void)
{
	s32 res;

	PRINTF(hInit, "isdbt_init\n");

	res = misc_register(&fc8150_misc_device);

	if(res < 0)
	{
		PRINTF(hInit, "isdbt init fail : %d\n", res);
		return res;
	}
	wake_lock_init(&oneseg_wakelock, WAKE_LOCK_SUSPEND, fc8150_misc_device.name);

	isdbt_hw_setting();

	isdbt_hw_init();

	hInit = (ISDBT_INIT_INFO_T *)kmalloc(sizeof(ISDBT_INIT_INFO_T), GFP_KERNEL);

	res = BBM_HOSTIF_SELECT(hInit, BBM_SPI);
	
	if(res)
	{
		PRINTF(hInit, "isdbt host interface select fail!\n");
	}

	isdbt_hw_deinit();

	if (!isdbt_kthread)
	{
		
		PRINTF(hInit, "kthread run\n");
		isdbt_kthread = kthread_run(isdbt_thread, (void*)hInit, "isdbt_thread");
	}

    /* miseon.kim 2012.06.21 FC8150 Device Release */
	res = request_irq(gpio_to_irq(GPIO_ISDBT_IRQ), isdbt_irq, IRQF_DISABLED | IRQF_TRIGGER_FALLING, FC8150_NAME, NULL);
    
	if(res) 
	{
		PRINTF(hInit, "dmb rquest irq fail : %d\n", res);
	}

	INIT_LIST_HEAD(&(hInit->hHead));

	return 0;
}

void isdbt_exit(void)
{
	PRINTF(hInit, "isdbt isdbt_exit \n");

	free_irq(GPIO_ISDBT_IRQ, NULL);
#if 1	
	kthread_stop(isdbt_kthread);
	isdbt_kthread = NULL;
#endif
	BBM_HOSTIF_DESELECT(hInit);

	isdbt_hw_deinit();
	
	misc_deregister(&fc8150_misc_device);

	kfree(hInit);
	wake_lock_destroy(&oneseg_wakelock);
}

module_init(isdbt_init);
module_exit(isdbt_exit);

MODULE_LICENSE("Dual BSD/GPL");
