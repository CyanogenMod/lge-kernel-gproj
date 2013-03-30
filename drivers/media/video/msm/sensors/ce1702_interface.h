/* Copyright (c) 2012, Code Aurora Forum. All rights reserved.
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
 */
//youngil.yun@lge.com 2012-07-09 - start 
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/syscalls.h>
#include <linux/workqueue.h>
#include <linux/wakelock.h>
#include <linux/spi/spi.h>
#include <linux/mfd/pm8xxx/pm8921.h>

#ifdef CONFIG_MACH_APQ8064_GVDCM
#include "../../../../../arch/arm/mach-msm/lge/gv/board-gv.h"
#else
#include "../../../../../arch/arm/mach-msm/lge/gk/board-gk.h"
#endif
#include "msm_camera_i2c_mux.h"
//youngil.yun@lge.com 2012-07-09 - end 

#if 1 // Start LGE_BSP_CAMERA::kyounghoon.noh@lge.com 2012-06-26
#include "msm.h"
#include "msm_ispif.h"
#endif // End LGE_BSP_CAMERA::kyounghoon.noh@lge.com 2012-06-26
#include "msm_sensor.h"
#include <mach/board_lge.h> 

/* LGE_CHANGE_S, Define Camera Log, 2012.10.18 jungki.kim@lge.com */
#define CAMERA_DEBUG 1
#define LDBGE(fmt,args...) printk(KERN_EMERG "[CAM/E][ERR] "fmt,##args)
#if (CAMERA_DEBUG)
#define LDBGI(fmt,args...) printk(KERN_EMERG "[CAM/I] "fmt,##args)
#else
#define LDBGI(args...) do {} while(0)
#endif
/* LGE_CHANGE_E, Define Camera Log, 2012.10.18 jungki.kim@lge.com */

#if 1 //def CE1702
#define CE1702_FLASH_BIN00_FILE "CE170F00.bin"
#define CE1702_FLASH_BIN01_FILE "CE170F01.bin"
#define CE1702_FLASH_BIN02_FILE "CE170F02.bin"
#define CE1702_FLASH_BIN03_FILE "CE170F03.bin"

#else
#define CE1702_FLASH_BIN00_FILE "/system/media/camera/fw/CE1702F00.bin"
#define CE1702_FLASH_BIN01_FILE "/system/media/camera/fw/CE1702F01.bin"
#define CE1702_FLASH_BIN02_FILE "/system/media/camera/fw/CE1702F02.bin"
#define CE1702_FLASH_BIN03_FILE "/system/media/camera/fw/CE1702F03.bin"

#define CE1702_FLASH_BIN00_FILE "/sdcard/CE14XF00.bin"
#define CE1702_FLASH_BIN01_FILE "/sdcard/CE14XF01.bin"
#define CE1702_FLASH_BIN02_FILE "/sdcard/CE14XF02.bin"
#define CE1702_FLASH_BIN03_FILE "/sdcard/CE14XF03.bin"
#endif

#define ISP_HOST_INT PM8921_GPIO_PM_TO_SYS(20)  
#define ISP_STBY PM8921_GPIO_PM_TO_SYS(13)  
/* LGE_CHANGE_S, For GK/GV Rev.E bring-up, 2012.10.26, gayoung85.lee[Start] */
#define ISP_RST PM8921_GPIO_PM_TO_SYS(27)
/* LGE_CHANGE_E, For GK/GV Rev.E bring-up, 2012.10.26, gayoung85.lee[End] */

#define CE1702_OK      0
#define CE1702_NOK     1

#define SPI_DRIVER_NAME "ce1702_spi"

/* LGE_CHANGE_S,  For isp bin location change, 2012.10.22, gayoung85.lee[Start] */
//#define ISP_BIN_LOCATION_SDCARD_FLAG
/* LGE_CHANGE_E,  For isp bin location change, 2012.10.22, gayoung85.lee[End] */

#define CHIPID 0
#if (CHIPID == 0)
#define SPI_CMD_WRITE                           0x0
#define SPI_CMD_READ                            0x1
#define SPI_CMD_BURST_WRITE                     0x2
#define SPI_CMD_BURST_READ                      0x3
#else
#define SPI_CMD_WRITE                           0x4
#define SPI_CMD_READ                            0x5
#define SPI_CMD_BURST_WRITE                     0x6
#define SPI_CMD_BURST_READ                      0x7
#endif 

#define HPIC_READ			0x01	// read command
#define HPIC_WRITE			0x02	// write command
#define HPIC_AINC			0x04	// address increment
#define HPIC_BMODE			0x00	// byte mode
#define HPIC_WMODE          0x10	// word mode
#define HPIC_LMODE          0x20	// long mode
#define HPIC_ENDIAN			0x00	// little endian
#define HPIC_CLEAR			0x80	// currently not used

// Host Access Common Register
#define CE1702_COMMAND_REG				0x0000
#define CE1702_ADDRESS_REG				0x0001
#define CE1702_DATA_REG				0x0002

typedef void        *HANDLE;

typedef enum 
{
  CE1702_NANDFLASH = 0, //internal NAND-FLASH
  CE1702_SDCARD2,
  CE1702_DIC,
  CE1702_ISP_MAX
}check_isp_bin;

/* LGE_CHANGE_S, Define For CE1702 output mode, 2012.11.10, elin.lee*/
typedef enum 
{
   CE1702_MODE_PREVIEW = 0, 
   CE1702_MODE_SINGLE_SHOT,
   CE1702_MODE_TMS_SHOT,
   CE1702_MODE_BURST_SHOT,
   CE1702_FRAME_MAX
}check_isp_mode;
/* LGE_CHANGE_E, Define For CE1702 output mode, 2012.11.10, elin.lee*/

struct ce1702_work {
	struct work_struct work;
};

/* LGE_CHANGE_S, Define For CE1702 Sensor To Understand Easier, 2012.10.22, jungki.kim[Start] */
#define AF_MODE_UNCHANGED	-1
#define AF_MODE_NORMAL		0
#define AF_MODE_MACRO			1
#define AF_MODE_AUTO			2
#define AF_MODE_CAF_PICTURE	3
#define AF_MODE_INFINITY		4
#define AF_MODE_CAF_VIDEO		5

#define SET_AREA_AF				0
#define SET_AREA_AE				1
#define SET_AREA_AF_OFF			2
#define SET_AREA_AE_OFF			3

#define FLASH_LED_OFF			0
#define FLASH_LED_AUTO			1
#define FLASH_LED_ON			2
#define FLASH_LED_TORCH		3

#define CAMERA_WB_OFF 			9

#define PREVIEW_MODE_CAMERA		0
#define PREVIEW_MODE_CAMCORDER	1

#define MAX_NUMBER_CE1702		128

#define CE1702_SIZE_13MP	0x31
#define CE1702_SIZE_W10MP	0x2C
#define CE1702_SIZE_8MP		0x24
#define CE1702_SIZE_6MP		0x31	// NOPE!!
#define CE1702_SIZE_W6MP	0x2B
#define CE1702_SIZE_W5MP	0x23	// NOPE!!
#define CE1702_SIZE_5MP		0x22
#define CE1702_SIZE_W3MP	0x29
#define CE1702_SIZE_3MP		0x21
#define CE1702_SIZE_2MP		0x1D
#define CE1702_SIZE_W1MP	0x28
#define CE1702_SIZE_HD		0x1A	//1280x720
#define CE1702_SIZE_1MP		0x1C	//1280x960
#define CE1702_SIZE_D1		0x0D
#define CE1702_SIZE_VGA		0x0B
#define CE1702_SIZE_CIF		0x09
#define CE1702_SIZE_QVGA	0x05
#define CE1702_SIZE_QCIF		0x02
#define CE1702_SIZE_FHD		0x1F
#define CE1702_SIZE_FHD1	0x1E
#define CE1702_SIZE_EIS_FHD	0x26
#define CE1702_SIZE_EIS_HD		0x25
/* LGE_CHANGE_E, Define For CE1702 Sensor To Understand Easier, 2012.10.22, jungki.kim[End] */

struct ce1702_size_type {
	uint16_t width;
	uint16_t height;
	uint16_t size_val;
};

/* LGE_CHANGE_S, Support ManualSceneMode for CE1702, 2012.10.28, gayoung85.lee[Start] */
typedef enum {
  CAMERA_SCENE_OFF = 0,
  CAMERA_SCENE_AUTO = 1,
  CAMERA_SCENE_LANDSCAPE = 2,
  CAMERA_SCENE_SNOW,
  CAMERA_SCENE_BEACH,
  CAMERA_SCENE_SUNSET,
  CAMERA_SCENE_NIGHT,
  CAMERA_SCENE_PORTRAIT,
  CAMERA_SCENE_BACKLIGHT,
  CAMERA_SCENE_SPORTS,
  CAMERA_SCENE_ANTISHAKE,
  CAMERA_SCENE_FLOWERS,
  CAMERA_SCENE_CANDLELIGHT,
  CAMERA_SCENE_FIREWORKS,
  CAMERA_SCENE_PARTY,
  CAMERA_SCENE_NIGHT_PORTRAIT,
  CAMERA_SCENE_THEATRE,
  CAMERA_SCENE_ACTION,
  CAMERA_SCENE_AR,
  CAMERA_SCENE_SMARTSHUTTER,  /* LGE_CHANGE Smart Shutter, 2012-01-11 sungmin.woo@lge.com */
  CAMERA_SCENE_MAX
} camera_scene_mode_type;
/* LGE_CHANGE_E, Support ManualSceneMode for CE1702, 2012.10.28, gayoung85.lee[End] */


extern int dest_location_firmware;


int32_t ce1702_power_up(struct msm_sensor_ctrl_t *s_ctrl);
int32_t ce1702_power_down(struct msm_sensor_ctrl_t *s_ctrl);
int32_t ce1702_match_id(struct msm_sensor_ctrl_t *s_ctrl);
extern int32_t ce1702_i2c_read(unsigned short   saddr,unsigned char cmd_addr, unsigned char *cmd_data, int cmd_len, unsigned char *rdata, int length);
extern int32_t ce1702_i2c_write(unsigned short saddr, unsigned short waddr, unsigned char *wdata, uint32_t length);
extern inline void init_suspend(void);
extern inline void deinit_suspend(void);
extern void ce1702_sysfs_add(struct kobject* kobj);
extern  void ce1702_sysfs_rm(struct kobject* kobj);
extern  void ce1702_wq_ISP_upload(struct work_struct *work);
extern long ce1702_check_flash_version(void);

/* LGE_CHANGE_S, Define to use every scope, 2012.10.24, jungki.kim[Start] */
extern long ce1702_isp_fw_full_upload(void);
int8_t ce1702_set_caf(int mode);
int8_t ce1702_stop_af(struct msm_sensor_ctrl_t *s_ctrl);
int8_t ce1702_set_focus_mode_setting(struct msm_sensor_ctrl_t *s_ctrl, int32_t afmode);
void ce1702_set_preview_assist_mode(struct msm_sensor_ctrl_t *s_ctrl, uint8_t mode);
int8_t ce1702_sensor_set_led_flash_mode_snapshot(int32_t led_mode);
int8_t ce1702_set_aec_awb_lock(struct msm_sensor_ctrl_t *s_ctrl, int32_t islock);
int8_t ce1702_set_model_name(void);
int8_t ce1702_set_ISP_mode(void);
int8_t ce1702_set_time(void);
int8_t ce1702_set_exif_rotation_to_isp(void);
int8_t ce1702_set_manual_focus_length(struct msm_sensor_ctrl_t *s_ctrl, int32_t focus_val);
int8_t ce1702_set_window(struct msm_sensor_ctrl_t *s_ctrl, int16_t *window, int16_t sw);
int8_t ce1702_set_VCM_default_position(struct msm_sensor_ctrl_t *s_ctrl);
/* LGE_CHANGE_E, Define to use every scope, 2012.10.24, jungki.kim[End] */
void CE_FwStart(void);

