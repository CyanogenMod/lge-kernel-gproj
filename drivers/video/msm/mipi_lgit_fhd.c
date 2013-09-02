/* Copyright (c) 2008-2010, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */
#include <linux/gpio.h>

#include <mach/board_lge.h>

#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_lgit_fhd.h"

#include "mdp4.h"

static struct msm_panel_common_pdata *mipi_lgit_pdata;

static struct dsi_buf lgit_tx_buf;
static struct dsi_buf lgit_rx_buf;
static struct dsi_buf lgit_camera_tx_buf;
static struct dsi_buf lgit_shutdown_tx_buf;

#if defined(CONFIG_MACH_APQ8064_GK_KR) || defined(CONFIG_MACH_APQ8064_GKATT) || defined(CONFIG_MACH_APQ8064_GV_KR) || defined(CONFIG_MACH_APQ8064_GKGLOBAL)
//#define LGD_PANEL_WORKAROUND       /* workaround for lcd defect(lcd flickering)   20130119 */ 
//#define LGD_PANEL_WORKAROUND_TIME  /* check the time inverval  from LCD ON to REBOOT */
#endif

#ifdef LGD_PANEL_WORKAROUND
#include <linux/syscalls.h>
#include <linux/rtc.h>


#if defined(CONFIG_MACH_APQ8064_GKATT) || defined(CONFIG_MACH_APQ8064_GKGLOBAL)
#define INVERSION_NORMAL_REG_1 0x30   // 1 column inversion
#define INVERSION_NORMAL_REG_2 0xF7   
#else
#define INVERSION_NORMAL_REG_1 0x32   // 3 column inversion
#define INVERSION_NORMAL_REG_2 0xF7   
#endif
#define INVERSION_RECOVERY_REG_1 0x30   // 2 dot inversion
#define INVERSION_RECOVERY_REG_2 0x17   // 2 dot inversion

#define INVERSION_NORMAL 0
#define INVERSION_RECOVERY 1

/* refer   non_HLOS\boot_images\modem\api\rfa\pm_pwron.h */
#define ONLY_POWERKEY_BOOT 0x0001

#define LCD_STATUS_OFF 0x00  // last lcd status was off
#define LCD_STATUS_ON 0x01   // last lcd status was on
#define LCD_STATUS_ALWAYS_INVERSION_NORMAL  0x02// request for normal inversion always
#define LCD_STATUS_ALWAYS_INVERSION_RECOVERY 0x03 // request for recovery inversion always

#define INVERSION_MODE_DEFAULT 0
#define INVERSION_MODE_ALWAYS_NORMAL 1
#define INVERSION_MODE_ALWAYS_RECOVERY 2

#if defined(CONFIG_MACH_APQ8064_GKU) || defined(CONFIG_MACH_APQ8064_GKATT) || defined(CONFIG_MACH_APQ8064_GKGLOBAL)
/* created lcd log partition by file system Team : http://lap.lge.com:8145/lap/219040 */
#define LCD_MMC_DEVICENAME "/dev/block/mmcblk0p36"
#elif defined(CONFIG_MACH_APQ8064_GKKT) || defined(CONFIG_MACH_APQ8064_GKSK) || defined(CONFIG_MACH_APQ8064_GVKT)
#define LCD_MMC_DEVICENAME "/dev/block/mmcblk0p37"
#endif
#define PTN_LCD_STATUS_POSITION_IN_PARTITION 0

#ifdef LGD_PANEL_WORKAROUND_TIME
#define PTN_LCD_ON_TIME_POSITION_IN_PARTITION 1
#define LCD_MAX_TIME_INTERVAL 4*60
#endif
#define LCD_RTC_HCTOSYS_DEVICE "rtc0"

#define CHANGE_INVERSION_INTERVAL 60*60 // sec

static bool first_boot_flag=false;
static int lcd_current_inversion =INVERSION_NORMAL;
static int lcd_inversion_mode=INVERSION_NORMAL;
static int lcd_default_inversion=INVERSION_NORMAL;
struct delayed_work lcd_worker;
struct delayed_work lcd_set_value_worker;
unsigned long change_inversion_interval = (CHANGE_INVERSION_INTERVAL*HZ);
unsigned long lcd_rtc_time_initial=0;
struct msm_fb_data_type *local_mfd1 = NULL;

extern uint16_t power_on_status_info_get(void);

#endif


#ifdef LGD_PANEL_WORKAROUND

int set_lcd_status(char val);

static int set_inversion_mode(int mode)
{
	lcd_inversion_mode=mode;
	return 0;

}

static int get_inversion_mode(void)
{
	return lcd_inversion_mode;

}

static int set_current_inversion(int inversion)
{
	lcd_current_inversion=inversion;
	return 0;
}

static int get_current_inversion(void)
{
	return lcd_current_inversion;

}

static int set_default_inversion(int inversion)
{
	lcd_default_inversion=inversion;
	return 0;
}

static int get_default_inversion(void)
{
	return lcd_default_inversion;

}

static int set_lcd_rtc_time_initial(unsigned long time)
{
	lcd_rtc_time_initial=time;
	return 0;
}

static int get_lcd_rtc_time_initial(void)
{
	return lcd_rtc_time_initial;
}

static unsigned long get_lcd_time_rtc(void)
{
	struct rtc_time tm;
	struct rtc_device *rtc;
	unsigned long now_tm_sec = 0;
	int rc = 0;

	rtc = rtc_class_open(LCD_RTC_HCTOSYS_DEVICE);
	if (rtc == NULL) {
		printk("%s: unable to open rtc device (%s)\n",
			__FILE__, CONFIG_RTC_HCTOSYS_DEVICE);
		return 0;
	}

	rc = rtc_read_time(rtc, &tm);
	if (rc) {
		printk("Error reading rtc device (%s) : %d\n",
			LCD_RTC_HCTOSYS_DEVICE, rc);

		now_tm_sec=0;
		goto err_read;
	}

	rc = rtc_valid_tm(&tm);
	if (rc) {
		printk("Invalid RTC time (%s): %d\n",
			LCD_RTC_HCTOSYS_DEVICE, rc);

		now_tm_sec=0;
		goto err_invalid;
	}

	rtc_tm_to_time(&tm, &now_tm_sec);

	printk("[LCD][DEBUG] secs = %lu, h:m:s == %d:%d:%d, d/m/y = %d/%d/%d\n",
			now_tm_sec, tm.tm_hour, tm.tm_min, tm.tm_sec,
			tm.tm_mday, tm.tm_mon, tm.tm_year);

err_invalid:
err_read:
	rtc_class_close(rtc);

	return now_tm_sec;
}

static int mipi_lgit_lcd_change_inversion(int req_inversion)
{

	struct dsi_cmd_desc *reg;
	int current_inversion=get_current_inversion();
	
	if( current_inversion==req_inversion){
		printk("[LCD][DEBUG] %s pre status: %d    req status:%d \n",__func__, current_inversion, req_inversion);
		return 0;
	}

	reg=mipi_lgit_pdata->power_on_set_1;

	if(req_inversion==INVERSION_RECOVERY){
		//recovery mode : 1 Dot inversion
		// set display_setting_2 reg
		reg[6].payload[1]=INVERSION_RECOVERY_REG_1;
		reg[6].payload[2]=INVERSION_RECOVERY_REG_2;

	}else{
		//normal mode : column inversion
		// set display_setting_2 reg
		reg[6].payload[1]=INVERSION_NORMAL_REG_1;
		reg[6].payload[2]=INVERSION_NORMAL_REG_2;

	}
	
	if(local_mfd1!=NULL && local_mfd1->panel_power_on )  {
		printk("[LCD][DEBUG] %s start mipi command : change the inversion\n",__func__);
        mutex_lock(&local_mfd1->dma->ov_mutex);
		MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x10000000);//HS mode

		mipi_dsi_cmds_tx(&lgit_tx_buf,&reg[6],1);
		
		MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x14000000);//LP mode
        mutex_unlock(&local_mfd1->dma->ov_mutex);
        printk("[LCD][DEBUG] %s end  mipi command : change the inversion\n",__func__);
	}

	printk("[LCD][DEBUG] %s pre status: %d	 req status:%d \n",__func__, current_inversion, req_inversion);

	set_current_inversion(req_inversion);
	return 0;
	
}

static void lcd_workqueue_handler(struct work_struct *work)
{


	// change inversion
	if(get_inversion_mode()==INVERSION_MODE_DEFAULT){
		mipi_lgit_lcd_change_inversion(INVERSION_NORMAL);
		set_default_inversion(INVERSION_NORMAL);
	}

	printk("[LCD][DEBUG] %s complete : change to column inversion\n",__func__);
	
}

static void lcd_set_value_workqueue_handler(struct work_struct *work)
{


#ifdef LGD_PANEL_WORKAROUND_TIME
	unsigned long sec;
	sec=get_lcd_time_rtc();
	set_lcd_on_time(sec);
#endif
	set_lcd_status(LCD_STATUS_ON);
	
}

static int write_lcd_value(int offset,const char *val, size_t count)
{
	int h_file = 0;
	int ret = 0;


	mm_segment_t old_fs = get_fs();
	set_fs(KERNEL_DS);
	h_file = sys_open(LCD_MMC_DEVICENAME, O_RDWR | O_SYNC,0);

	if(h_file >= 0)
	{
		sys_lseek( h_file, offset, 0 );

		ret = sys_write( h_file, val, count);

		if( ret != count )
		{
			printk("[LCD][DEBUG] Can't write in  partition.  ret:%d \n", ret);

		}

		sys_close(h_file);
	}
	else
	{
		printk("[LCD][DEBUG] Can't open  partition handle = %d.\n",h_file);

	}
	set_fs(old_fs);

	return ret;

	
}


static int read_lcd_value(int offset,  char *buf, size_t count)
{
	int h_file = 0;
	int ret = 0;


	mm_segment_t old_fs = get_fs();
	set_fs(KERNEL_DS);
	h_file = sys_open(LCD_MMC_DEVICENAME, O_RDWR | O_SYNC,0);

	if(h_file >= 0)
	{
		sys_lseek( h_file, offset, 0 );

		ret = sys_read( h_file, buf, count);

		if( ret != count )
		{
			printk("[LCD][DEBUG] Can't read  partition.\n");
		}
		sys_close(h_file);
	}
	else
	{
		printk("[LCD][DEBUG] Can't open  partition handle = %d.\n",h_file);
	}
	set_fs(old_fs);

	return ret;
}

int set_lcd_status(char val)
{	
	int ret;
	char stats;
	int offset = (int)PTN_LCD_STATUS_POSITION_IN_PARTITION;

	stats = val;

	printk("[LCD][DEBUG] set_lcd_status :%d\n",stats);

	ret = write_lcd_value(offset,&stats,1);
	if (ret != 1) {
		printk("[LCD][DEBUG] %s : write val has failed!!\n", __func__);
		}

	return 0;
		
}
EXPORT_SYMBOL(set_lcd_status);


int get_lcd_status(void)
{	
	int ret;
	char buf;
	int offset = (int)PTN_LCD_STATUS_POSITION_IN_PARTITION;
	

	ret = read_lcd_value(offset,&buf,1);
	printk("[LCD][DEBUG] get_lcd_status : %d \n",buf);

	return (int)buf;
		
}
EXPORT_SYMBOL(get_lcd_status);


#ifdef LGD_PANEL_WORKAROUND_TIME
static int set_lcd_on_time(unsigned long sec)
{	
	int ret;
	char value[4];
	int offset = (int)PTN_LCD_ON_TIME_POSITION_IN_PARTITION;

	
	value[0]=(char)(sec & 0xFF);
	value[1]=(char)((sec >> 8) & 0xFF);
	value[2]=(char)((sec >> 16) & 0xFF);
	value[3]=(char)((sec >> 24) & 0xFF);
	
	printk("[LCD][DEBUG] set_lcd_on_time : sec %lu   \n",sec);

	ret = write_lcd_value(offset,value,4);
	if (ret != 4) {
		printk("[LCD][DEBUG] %s : write val has failed!!\n", __func__);
		}

	return 0;
		
}

static unsigned long get_lcd_on_time(void)
{	
	int ret;
	char value[sizeof(unsigned long)];
	int offset = (int)PTN_LCD_ON_TIME_POSITION_IN_PARTITION;
	unsigned long sec;
	

	ret = read_lcd_value(offset,value,sizeof(unsigned long));

	/* if Can't read the time, set the time to zero */
	if(ret!=sizeof(unsigned long))
		return 0;
	
	sec=value[0] | (value[1] << 8) | (value[2] << 16) | (value[3] << 24);
	
	printk("[LCD][DEBUG] get_lcd_on_time : sec%lu   value[0]:%x value[1]:%x value[2]:%x value[3]:%x\n",sec,value[0],value[1],value[2],value[3] );

	return sec;
		
}

static bool calculate_lcd_time_interval(void)
{
	bool ret;
	unsigned long time_interval=0;
	unsigned long current_time;
	unsigned long last_lcd_on_time;

	last_lcd_on_time=get_lcd_on_time();
	current_time=get_lcd_time_rtc();
	
	time_interval=current_time-last_lcd_on_time;

	printk("[LCD][DEBUG] %s  current : %lu    last_lcd_on : %lu \n", __func__, current_time, last_lcd_on_time);
	if(time_interval< 0 || time_interval>LCD_MAX_TIME_INTERVAL )
		ret=false;
	else
		ret=true;

	return ret;
}

#endif //LGD_PANEL_WORKAROUND_TIME

#endif

static int __init mipi_lgit_lcd_init(void);

#if defined(CONFIG_LGIT_COLOR_ENGINE_SWITCH)
static int is_color_engine_on = 1;
struct msm_fb_data_type *local_mfd0 = NULL;
int mipi_lgit_lcd_color_engine_off(void)
{
       if(local_mfd0!=NULL && local_mfd0->panel_power_on && is_color_engine_on) {
		printk("Color Engine_OFF Starts with Camera\n");
              mutex_lock(&local_mfd0->dma->ov_mutex);

		MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x10000000);//HS mode
		mipi_dsi_cmds_tx(&lgit_camera_tx_buf,
              mipi_lgit_pdata->color_engine_off, 
              mipi_lgit_pdata->color_engine_off_size);
		printk("%s, %d\n", __func__,is_color_engine_on);
		MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x14000000);//LP mode

              mutex_unlock(&local_mfd0->dma->ov_mutex);
              printk("Color Engine_OFF Ends with Camera\n");
	}
	is_color_engine_on = 0;
	return 0;
}

int mipi_lgit_lcd_color_engine_on(void)
{
       if(local_mfd0!=NULL && local_mfd0->panel_power_on && !is_color_engine_on) {
		printk("Color Engine_ON Starts with Camera\n");
	       mutex_lock(&local_mfd0->dma->ov_mutex);

		MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x10000000);//HS mode
		mipi_dsi_cmds_tx(&lgit_camera_tx_buf,
              mipi_lgit_pdata->color_engine_on,
              mipi_lgit_pdata->color_engine_on_size);
		printk("%s, %d\n", __func__,is_color_engine_on);
		MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x14000000); //LP mode

		mutex_unlock(&local_mfd0->dma->ov_mutex);
		printk("Color Engine_ON Ends with Camera\n");
	}
       is_color_engine_on = 1;
	return 0;  
}
#endif //CONFIG_LGIT_COLOR_ENGINE_SWITCH

static int check_stable_lcd_on = 1;

static int mipi_stable_lcd_on(struct platform_device *pdev)
{
       int ret = 0;
       int retry_cnt = 0;

       do {
              printk("[LCD][DEBUG] %s, retry_cnt=%d\n", __func__, retry_cnt);
              ret = mipi_lgit_lcd_off(pdev);

              if (ret < 0) {
                     msleep(3);
                     retry_cnt++;
              }
              else {
                     check_stable_lcd_on = 0;
                     break;
              }
       } while(retry_cnt < 10);
       mdelay(10);

       return ret;
}

int mipi_lgit_lcd_on(struct platform_device *pdev)
{
       int cnt = 0;
	struct msm_fb_data_type *mfd;

#ifdef LGD_PANEL_WORKAROUND
	uint16_t boot_cause=0;
	bool powerkey_boot=false;
	char lcd_status=0;
	bool last_lcd_on=false;
	int inversion_mode=INVERSION_MODE_DEFAULT;
	int default_inversion=INVERSION_NORMAL;
	int current_inversion=INVERSION_NORMAL;

#ifdef LGD_PANEL_WORKAROUND_TIME
	bool valid_time_interval=false;
#endif	
	struct dsi_cmd_desc *reg;
	
	if(first_boot_flag==false)
	{
		//first boot time
		boot_cause=power_on_status_info_get();
		if(boot_cause==ONLY_POWERKEY_BOOT )
			powerkey_boot=true;

		lcd_status=get_lcd_status();
		if(lcd_status==LCD_STATUS_ON)
			last_lcd_on=true;

#ifdef LGD_PANEL_WORKAROUND_TIME
		valid_time_interval=calculate_lcd_time_interval();
		printk("[LCD][DEBUG] %s first boot status   boot_cause:%x    lcd_status:%x  valid_time_interval %d \n",__func__, boot_cause, lcd_status, valid_time_interval);
		if(powerkey_boot==true && last_lcd_on==true && valid_time_interval==true)
#else
		printk("[LCD][DEBUG] %s first boot status   boot_cause:%x    lcd_status:%x \n",__func__, boot_cause, lcd_status);
		if(powerkey_boot==true && last_lcd_on==true )
#endif
		{
			//change to recovery inversion
			// set display_setting_2 reg
			reg=mipi_lgit_pdata->power_on_set_1;
			reg[6].payload[1]=INVERSION_RECOVERY_REG_1;
			reg[6].payload[2]=INVERSION_RECOVERY_REG_2;

			//que the workque to change the inversion
			schedule_delayed_work(&lcd_worker, change_inversion_interval);

			// get_lcd_time_rtc
			set_lcd_rtc_time_initial(get_lcd_time_rtc());

			current_inversion =INVERSION_RECOVERY;
			default_inversion=INVERSION_RECOVERY;
			printk("[LCD][DEBUG] %s change inversion : 1 Dot inversion \n",__func__);

		}else{
		
			if(lcd_status==LCD_STATUS_ALWAYS_INVERSION_RECOVERY)
			{
				//change to recovery inversion
				// set display_setting_2 reg
				reg=mipi_lgit_pdata->power_on_set_1;
				reg[6].payload[1]=INVERSION_RECOVERY_REG_1;
				reg[6].payload[2]=INVERSION_RECOVERY_REG_2;
				inversion_mode=INVERSION_MODE_ALWAYS_RECOVERY;

				current_inversion =INVERSION_RECOVERY;
				default_inversion=INVERSION_RECOVERY;
		
			}else if(lcd_status==LCD_STATUS_ALWAYS_INVERSION_NORMAL) {
				inversion_mode=INVERSION_MODE_ALWAYS_NORMAL;
			}
		}
		/* else normal inversion */
		

		set_inversion_mode(inversion_mode);
		set_default_inversion(default_inversion);
		set_current_inversion(current_inversion);
		first_boot_flag=true;
	}else{

		if((get_inversion_mode()==INVERSION_MODE_DEFAULT) && (get_current_inversion()!=INVERSION_NORMAL)){
				unsigned long rtc_time_current=0;
				unsigned long rtc_time_initial=0;
				unsigned long rtc_time_interval=0;
				unsigned long rtc_time_remaining=0;
				
				rtc_time_current=get_lcd_time_rtc();
				rtc_time_initial=get_lcd_rtc_time_initial();
				rtc_time_interval=rtc_time_current - rtc_time_initial;
				rtc_time_remaining = CHANGE_INVERSION_INTERVAL - rtc_time_interval;
				printk("[LCD][DEBUG] %s rtc_interval:%lu  rtc_remaining:%lu  rtc_time_current:%lu  rtc_time_initial:%lu \n",__func__, rtc_time_interval, rtc_time_remaining, rtc_time_current, rtc_time_initial);

				cancel_delayed_work(&lcd_worker);
				if( rtc_time_interval<=0  || rtc_time_interval >=CHANGE_INVERSION_INTERVAL ){
					//change to normal inversion
					// set display_setting_2 reg
					reg=mipi_lgit_pdata->power_on_set_1;
					reg[6].payload[1]=INVERSION_NORMAL_REG_1;
					reg[6].payload[2]=INVERSION_NORMAL_REG_2;
					set_default_inversion(INVERSION_NORMAL);
					set_current_inversion(INVERSION_NORMAL);
					printk("[LCD][DEBUG] %s   change to normal inversion \n", __func__);
				}else{  // 0 < rtc_time_interval < CHANGE_INVERSION_INTERVAL

					schedule_delayed_work(&lcd_worker,rtc_time_remaining*HZ);

				}

			}
		}

#endif

       if (check_stable_lcd_on)
              mipi_stable_lcd_on(pdev);

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;
#if defined(CONFIG_LGIT_COLOR_ENGINE_SWITCH)
       if(local_mfd0 == NULL)
              local_mfd0 = mfd;
#endif
#if defined(LGD_PANEL_WORKAROUND)
		   if(local_mfd1 == NULL)
				  local_mfd1 = mfd;
#endif

	printk(KERN_INFO "[LCD][DEBUG] %s is started \n", __func__);

//LGE_UPDATE_S hj.eum@lge.com : adding change mipi mode to write register setting of LCD IC
	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x10000000);     //HS mode

#if defined (CONFIG_MACH_APQ8064_GVDCM)
       if(lge_get_board_revno() <= HW_REV_C){
		printk("[LCD][DEBUG] initial code of LCD for GVDCM\n");
              cnt = mipi_dsi_cmds_tx(&lgit_tx_buf,
		    mipi_lgit_pdata->power_on_set_1_old,
		    mipi_lgit_pdata->power_on_set_size_1_old);
              if (cnt < 0)
                     return cnt;
       }else{
		printk("[LCD][DEBUG] initial code of LCD for GVDCM Rev.D\n");
              cnt = mipi_dsi_cmds_tx(&lgit_tx_buf,
		    mipi_lgit_pdata->power_on_set_1,
		    mipi_lgit_pdata->power_on_set_size_1);
              if (cnt < 0)
                     return cnt;
       }
#else
       cnt = mipi_dsi_cmds_tx(&lgit_tx_buf,
              mipi_lgit_pdata->power_on_set_1,
		mipi_lgit_pdata->power_on_set_size_1);
       if (cnt < 0)
              return cnt;
#endif
	cnt = mipi_dsi_cmds_tx(&lgit_tx_buf,
		mipi_lgit_pdata->power_on_set_2, 
		mipi_lgit_pdata->power_on_set_size_2);
       if (cnt < 0)
              return cnt;

	mipi_dsi_op_mode_config(DSI_VIDEO_MODE);
       mdp4_overlay_dsi_video_start();
	mdelay(120);
#if defined(CONFIG_LGE_R63311_BACKLIGHT_CABC)
       cnt = mipi_dsi_cmds_tx(&lgit_tx_buf,
		mipi_lgit_pdata->power_on_set_3,
		mipi_lgit_pdata->power_on_set_size_3);
       if (cnt < 0)
              return cnt;
#endif
//LGE_UPDATE_E hj.eum@lge.com : adding change mipi mode to write register setting of LCD IC

#ifdef LGD_PANEL_WORKAROUND

		//set lcd_on_flag
		if(get_inversion_mode()==INVERSION_MODE_DEFAULT){
			
			schedule_delayed_work(&lcd_set_value_worker, 5);

		}
		
#endif

	printk(KERN_INFO "[LCD][DEBUG] %s is ended \n", __func__);

       return cnt;
}

int mipi_lgit_lcd_off(struct platform_device *pdev)
{
       int cnt = 0;
	struct msm_fb_data_type *mfd;
	
       printk(KERN_INFO "[LCD][DEBUG] %s is started \n", __func__);

	mfd =  platform_get_drvdata(pdev);
	
	if (!mfd)
		return -ENODEV;
	
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x10000000);//HS mode
//	mipi_dsi_cmds_tx(mfd, &lgit_tx_buf, 
       cnt = mipi_dsi_cmds_tx(&lgit_tx_buf,
		mipi_lgit_pdata->power_off_set_1,	
		mipi_lgit_pdata->power_off_set_size_1);
       if (cnt < 0)
              return cnt;

	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x14000000);//LP mode
	printk(KERN_INFO "[LCD][DEBUG] %s is ended \n", __func__);

	
#ifdef LGD_PANEL_WORKAROUND
		//unset lcd_on_flag
		if(get_inversion_mode()==INVERSION_MODE_DEFAULT)
			set_lcd_status(LCD_STATUS_OFF);
#endif
	
       return cnt;
}

#ifdef CONFIG_FB_MSM_MIPI_LGIT_VIDEO_FHD_INVERSE_PT
int mipi_lgit_lcd_off_for_shutdown(void)
{
       if(!local_mfd0 || !local_mfd0->panel_power_on)
              return -1;

       printk("[LCD][DEBUG] Backlight off!\n");

       mipi_lgit_pdata->backlight_level(0,0,0);
       
       printk("%s is started \n", __func__);

       MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x10000000);//HS mode
//	mipi_dsi_cmds_tx(mfd, &lgit_tx_buf, 
       mipi_dsi_cmds_tx(&lgit_shutdown_tx_buf,
	      mipi_lgit_pdata->power_off_set_2,
	      mipi_lgit_pdata->power_off_set_size_2);
       MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x14000000);//LP mode

       printk("%s is ended \n", __func__);

#ifdef LGD_PANEL_WORKAROUND
	//unset lcd_on_flag
	//set_lcd_status(LCD_STATUS_OFF);
#endif

       return 0;
}
#endif

static void mipi_lgit_set_backlight_board(struct msm_fb_data_type *mfd)
{
	int level;

	level = (int)mfd->bl_level;
	mipi_lgit_pdata->backlight_level(level, 0, 0);
}

#if defined(CONFIG_LGE_R63311_COLOR_ENGINE)
static int ce_ctrl_show(struct device * dev, struct device_attribute * attr, char * buf)
{
   int  i,r,total_len = 0;
   char str[150];
   char temp[5];	
   struct dsi_cmd_desc *reg;
    
   reg=mipi_lgit_pdata->power_on_set_1;

   for(i=0; i<33; i++){
       sprintf(temp,"%x ", reg[18].payload[i]);
	   if(i == 0)
		 strcpy(str, temp);
	   else
       	 strcat(str + total_len,temp);
	   total_len+=strlen(temp);
   }

   r = snprintf(buf, PAGE_SIZE, "color enhancement: %s\n", str);
  
   return r;
}

static int ce_ctrl_store(struct device * dev, struct device_attribute * attr, const char * buf, size_t count)
{
   char *bp;
   char **token;
   char *data = NULL;
   struct dsi_cmd_desc *reg;
   int i=0;

   reg=mipi_lgit_pdata->power_on_set_1;

   bp=(char *)buf;

   while(true){
        if(strstr(bp, ", ")){
            bp=strstr(bp, "0x");
            token=&bp;
            data=strsep(token,", ");
            if(data == NULL)
                return -1;
            reg[18].payload[i]=simple_strtol(data,NULL,16);            
            token=NULL;
        }else{
            bp=strstr(bp, "0x");
            if(bp == NULL)
                return -1;
            reg[18].payload[i]=simple_strtol(bp,NULL,16);            
            break;
        }
        i++;
    }
    return count;
}
DEVICE_ATTR(ce_ctrl, 0774, ce_ctrl_show, ce_ctrl_store);

static int aco_ctrl_show(struct device * dev, struct device_attribute * attr, char * buf)
{
    int r;
    struct dsi_cmd_desc *reg;
    
    reg=mipi_lgit_pdata->power_on_set_1;

   	r = snprintf(buf, PAGE_SIZE, "Auto Contrast Optimization contorl is : %x %x %x %x %x %x %x\n", reg[19].payload[0],reg[19].payload[1], reg[19].payload[2],
                reg[19].payload[3],reg[19].payload[4],reg[19].payload[5],reg[19].payload[6]);

    return r;
}

static int aco_ctrl_store(struct device * dev, struct device_attribute * attr, const char * buf, size_t count)
{
   char *bp;
   char **token;
   char *data = NULL;
   struct dsi_cmd_desc *reg;
   int i=0;

   reg=mipi_lgit_pdata->power_on_set_1;
   
   bp=(char *)buf;

   while(true){
        if(strstr(bp, ", ")){
            bp=strstr(bp, "0x");
            token=&bp;
            data=strsep(token,", ");
            if(data == NULL)
                return -1;
            reg[19].payload[i]=simple_strtol(data,NULL,16);         
            token=NULL;
        }else{
            bp=strstr(bp, "0x");
            if(bp == NULL)
                return -1;
            reg[19].payload[i]=simple_strtol(bp,NULL,16);            
            break;
        }
        i++;
    }
    return count;
}
DEVICE_ATTR(aco_ctrl, 0774, aco_ctrl_show, aco_ctrl_store);

static int sharp_ctrl_show(struct device * dev, struct device_attribute * attr, char * buf)
{
    int r;
    struct dsi_cmd_desc *reg;
    
    reg=mipi_lgit_pdata->power_on_set_1;

   	r = snprintf(buf, PAGE_SIZE, "Outline sharpning contorl is : %x %x %x\n", reg[20].payload[0],reg[20].payload[1], reg[20].payload[2]);

    return r;
}

static int sharp_ctrl_store(struct device * dev, struct device_attribute * attr, const char * buf, size_t count)
{
   char *bp;
   char **token;
   char *data = NULL;
   struct dsi_cmd_desc *reg;
   int i=0;

   reg=mipi_lgit_pdata->power_on_set_1;
   
   bp=(char *)buf;

   while(true){
        if(strstr(bp, ", ")){
            bp=strstr(bp, "0x");
            token=&bp;
            data=strsep(token,", ");
            if(data == NULL)
                return -1;
            reg[20].payload[i]=simple_strtol(data,NULL,16);            
            token=NULL;
        }else{
            bp=strstr(bp, "0x");
            if(bp == NULL)
                return -1;
            reg[20].payload[i]=simple_strtol(bp,NULL,16);            
            break;
        }
        i++;
    }
    return count;
}
DEVICE_ATTR(sharp_ctrl, 0774, sharp_ctrl_show, sharp_ctrl_store);
#endif

static int lte_tuning_part1_show(struct device * dev, struct device_attribute * attr, char * buf)
{
    int r =0;
#if 0
    struct dsi_cmd_desc *reg;
   
    reg=mipi_lgit_pdata->power_on_set_1;

    r = snprintf(buf, PAGE_SIZE, "HBP:%d, HFP:%d, HPW:%d\nVBP:%d, VFP:%d, VPW:%d\nPLL_1:0x%x, PLL_2:0x%x, PLL_3:0x%x\nMIPI_clk:%d\n", HBP_tuning, HFP_tuning, HPW_tuning, VBP_tuning, VFP_tuning, VPW_tuning, PLL_tuning_1, PLL_tuning_2, PLL_tuning_3, MIPI_CLK_tuning);
#endif
    return r;
}

static int lte_tuning_part1_store(struct device * dev, struct device_attribute * attr, const char * buf, size_t count)
{
#if 0
   char *bp;
   har **token;
   char *data = NULL;
   nt input[7]={0};

   struct dsi_cmd_desc *reg;
   int i=0;

   eg=mipi_lgit_pdata->power_on_set_1;
   
   p=(char *)buf;

   or(i=0; i<7; i++)
   {
        f(strstr(bp, ", ")){
            bp=strstr(bp, "d");
            oken=&bp;
            data=strsep(token,", ");
            f(data == NULL)
                return -1;
            nput[i]=simple_strtol(data,NULL,16);            
            token=NULL;
        else{
            bp=strstr(bp, "d");
            f(bp == NULL)
                return -1;
            input[i]=simple_strtol(bp,NULL,16);            
            break;
        }
   

   BP_tuning=input[0];
   HFP_tuning=input[1];
   PW_tuning=input[2];
   VBP_tuning=input[3];
   FP_tuning=input[4];
   VPW_tuning=input[5];
   IPI_CLK_tuning=input[6];
#endif

   return count;
}
DEVICE_ATTR(lte_tuning_part1, 0774, lte_tuning_part1_show, lte_tuning_part1_store);

static int lte_tuning_show(struct device * dev, struct device_attribute * attr, char * buf)
{
    int r;
    struct dsi_cmd_desc *reg;
    
    reg=mipi_lgit_pdata->power_on_set_1;

    r = snprintf(buf, PAGE_SIZE, "Inversion_1:0x%x, Inversion_2:0x%x\nDCDC_1:0x%x, DCDC_2:0x%x, DCDC_3:0x%x\nVsync:0x%x\nBias:0x%x\n", reg[6].payload[1], reg[6].payload[2], reg[14].payload[6], reg[14].payload[7], reg[14].payload[9], reg[7].payload[1], reg[15].payload[18]);

    return r;
}

static int lte_tuning_store(struct device * dev, struct device_attribute * attr, const char * buf, size_t count)
{
   char *bp;
   char **token;
   char *data = NULL;
   char input[7]={0};

   struct dsi_cmd_desc *reg;
   int i=0;

   reg=mipi_lgit_pdata->power_on_set_1;
   
   bp=(char *)buf;

   for(i=0; i<7; i++)
   { 
        if(strstr(bp, ", ")){
            bp=strstr(bp, "0x");
            token=&bp;
            data=strsep(token,", ");
            if(data == NULL)
                return -1;
            input[i]=simple_strtol(data,NULL,16);            
            token=NULL;
        }else{
            bp=strstr(bp, "0x");
            if(bp == NULL)
                return -1;
            input[i]=simple_strtol(bp,NULL,16);            
            break;
        }
   } 

   reg[6].payload[1] = input[0];
   reg[6].payload[2] = input[1];
   reg[14].payload[6] = input[2];
   reg[14].payload[7] = input[3];
   reg[14].payload[9] = input[4];
   reg[7].payload[1] = input[5];
   reg[15].payload[18] = input[6];

   return count;
}
DEVICE_ATTR(lte_tuning, 0774, lte_tuning_show, lte_tuning_store);

#if defined(CONFIG_LGIT_COLOR_ENGINE_SWITCH)
static ssize_t color_engine_show_on_off(struct device *dev, struct device_attribute *attr, char *buf)
{
	int r = 0;

	pr_info("%s received (prev color_engine_on: %s)\n", __func__, is_color_engine_on? "ON" : "OFF");

	return r;
}

static ssize_t color_engine_store_on_off(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int on_off;

	if (!count)
		return -EINVAL;

	pr_info("%s received (prev color_engine_on: %s)\n", __func__, is_color_engine_on ? "ON" : "OFF");

	on_off = simple_strtoul(buf, NULL, 10);

	printk(KERN_ERR "color_engine_on : %d", on_off);

	if (on_off == 1) {
	    mipi_lgit_lcd_color_engine_on();
	} else if (on_off == 0)
	    mipi_lgit_lcd_color_engine_off();

	return count;
}
DEVICE_ATTR(color_engine_on_off, 0644, color_engine_show_on_off, color_engine_store_on_off);
#endif //CONFIG_LGIT_COLOR_ENGINE_SWITCH

#if defined(LGD_PANEL_WORKAROUND)
static ssize_t lcd_change_inversion_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int r = 0;

	int current_inversion=get_current_inversion();
	
	printk(KERN_ERR "[LCD][DEBUG] %s  : inversion mode ==> %d  \n",__func__, current_inversion );

	 r = snprintf(buf, PAGE_SIZE, "%d",current_inversion );

	return r;
}

static ssize_t lcd_change_inversion_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int mode;
	int inversion=INVERSION_NORMAL;
	int request_inversion_mode=INVERSION_MODE_DEFAULT;
	
	if (!count)
		return -EINVAL;

	mode = simple_strtoul(buf, NULL, 10);

	/* 0 : normal inversion  1: recovery inversion    2: always normal  3: always revovery  4: always default*/
	if(mode<0 || mode>5){
		printk(KERN_ERR "[LCD][DEBUG] %s  : invalid  mode : %d, \n",__func__, mode );
		return -EINVAL;
	}else
		printk(KERN_ERR "[LCD][DEBUG] %s  : request mode : %d, \n",__func__, mode );

	
	switch( mode )
	{
		case 0:  //normal inversion
			inversion=INVERSION_NORMAL;
			request_inversion_mode=INVERSION_MODE_DEFAULT;
			break;
			

		case 1: // recovery inversion
			inversion=INVERSION_RECOVERY;
			request_inversion_mode=INVERSION_MODE_DEFAULT;
                     break;

		case 2: // always normal inversion
			inversion=INVERSION_NORMAL;			
			request_inversion_mode=INVERSION_MODE_ALWAYS_NORMAL;
			set_lcd_status(LCD_STATUS_ALWAYS_INVERSION_NORMAL);
			break;
			
		case 3: // always recovery inversion
			inversion=INVERSION_RECOVERY;
			request_inversion_mode=INVERSION_MODE_ALWAYS_RECOVERY;
			set_lcd_status(LCD_STATUS_ALWAYS_INVERSION_RECOVERY);
			break;

		case 4:
			inversion=get_default_inversion();
			request_inversion_mode=INVERSION_MODE_DEFAULT;

			if(local_mfd1!=NULL && local_mfd1->panel_power_on)
				set_lcd_status(LCD_STATUS_ON); /* if lcd is on */
			else
				set_lcd_status(LCD_STATUS_OFF); /* if lcd is off */

			break;

		case 5: //if shutdown requested  , set lcd status OFF
			if(get_inversion_mode()==INVERSION_MODE_DEFAULT)
				set_lcd_status(LCD_STATUS_OFF);
			
		default : // always default
			break;
		
	}

	
	
	// change inversion
	mipi_lgit_lcd_change_inversion(inversion);
	set_inversion_mode(request_inversion_mode);
	
	printk(KERN_ERR "[LCD][DEBUG] %s  mode:%d  inversion : %s \n",__func__, mode, inversion? "Recovery inversion" : "Normal inversion" );

	return count;
}
DEVICE_ATTR(lcd_change_inversion, 0644, lcd_change_inversion_show, lcd_change_inversion_store);
#endif //LGD_PANEL_WORKAROUND


static int mipi_lgit_lcd_probe(struct platform_device *pdev)
{
#if defined(CONFIG_LGE_R63311_COLOR_ENGINE)
    int err;
#endif
	if (pdev->id == 0) {
		mipi_lgit_pdata = pdev->dev.platform_data;
		return 0;
	}

	printk(KERN_INFO "[LCD][DEBUG] %s: mipi lgit lcd probe start\n", __func__);

	msm_fb_add_device(pdev);


	
#ifdef LGD_PANEL_WORKAROUND
		
	//Initialize delayed work to change the inversion 
	INIT_DELAYED_WORK(&lcd_worker, lcd_workqueue_handler);

	
	INIT_DELAYED_WORK(&lcd_set_value_worker, lcd_set_value_workqueue_handler);
		
#endif


#if defined(CONFIG_LGE_R63311_COLOR_ENGINE)
    err = device_create_file(&pdev->dev, &dev_attr_sharp_ctrl);
    if(err < 0)
        printk("[LCD][DEBUG] %s : Cannot create the sysfs\n" , __func__);
    
    err = device_create_file(&pdev->dev, &dev_attr_aco_ctrl);
    if(err < 0)
        printk("[LCD][DEBUG] %s : Cannot create the sysfs\n" , __func__);

    err = device_create_file(&pdev->dev, &dev_attr_ce_ctrl);
    if(err < 0)
        printk("[LCD][DEBUG] %s : Cannot create the sysfs\n" , __func__);
#if defined(CONFIG_LGIT_COLOR_ENGINE_SWITCH)
    err = device_create_file(&pdev->dev, &dev_attr_color_engine_on_off);
    if(err < 0)
        printk("[LCD][DEBUG] %s : Cannot create the sysfs\n" , __func__);
#endif //CONFIG_LGIT_COLOR_ENGINE_SWITCH
#if defined(LGD_PANEL_WORKAROUND)
	err = device_create_file(&pdev->dev, &dev_attr_lcd_change_inversion);
	if(err < 0)
		printk("[LCD][DEBUG] %s : Cannot create the sysfs\n" , __func__);
#endif //LGD_PANEL_WORKAROUND
    err = device_create_file(&pdev->dev, &dev_attr_lte_tuning_part1);
    if(err < 0)
         printk("[LCD][DEBUG] %s : Cannot create the sysfs\n" , __func__);

    err = device_create_file(&pdev->dev, &dev_attr_lte_tuning);
    if(err < 0)
         printk("[LCD][DEBUG] %s : Cannot create the sysfs\n" , __func__);
#endif

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_lgit_lcd_probe,
	.driver = {
		.name   = "mipi_lgit",
	},
};

static struct msm_fb_panel_data lgit_panel_data = {
	.on		= mipi_lgit_lcd_on,
	.off		= mipi_lgit_lcd_off,
	.set_backlight = mipi_lgit_set_backlight_board,
};

static int ch_used[3];

int mipi_lgit_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_lgit", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	lgit_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &lgit_panel_data,
		sizeof(lgit_panel_data));
	if (ret) {
		printk(KERN_ERR
		  "[LCD][DEBUG] %s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		printk(KERN_ERR
		  "[LCD][DEBUG] %s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}

	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}

static int __init mipi_lgit_lcd_init(void)
{
	mipi_dsi_buf_alloc(&lgit_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&lgit_rx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&lgit_camera_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&lgit_shutdown_tx_buf, DSI_BUF_SIZE);

	return platform_driver_register(&this_driver);
}

module_init(mipi_lgit_lcd_init);
