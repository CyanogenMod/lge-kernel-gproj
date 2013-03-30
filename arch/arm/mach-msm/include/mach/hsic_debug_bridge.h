/*
 * HSIC Debug Module for EMS, created by Secheol.Pyo.
 * 2012-3-17
 * 
 * Secheol Pyo <secheol.pyo@lge.com>
 *
 *
 *  HISTORY 
 * 2012-04-05, Release
 * 2012-04-12, suspend-resume
*/


#ifndef __LINUX_USB_HSIC_DEBUG_CH_H__
#define __LINUX_USB_HSIC_DEBUG_CH_H__

//#define LG_FW_EMS_CONTINUOSLY_RECV
//#define LG_FW_HSIC_EMS_DEBUG
//#define LG_FW_WAIT_QUEUE
#define LG_FW_EMS_PM
#define LG_FW_REMOTE_WAKEUP

#define EMS_LOG_FORMAT_LEN_FILE_NAME	(50)
#define EMS_LOG_FORMAT_LEN_LINE			(4)
#define EMS_LOG_FORMAT_LEN_ERR_MSG		(80)
#define EMS_LOG_FORMAT_LEN_SW_VER		(50)
#define HS_INTERVAL 7
#define FS_LS_INTERVAL 3

struct hsic_debug_bridge_ops {
	void *ctxt;
	int tmp; // temp
	void (*read_complete_cb)(void *ctxt,void *buf, 
			int buf_size, int actual);
	void (*write_complete_cb)(void *ctxt, void *buf,
			int buf_size, int actual);
	int (*suspend)(void *ctxt);
	void (*resume)(void *ctxt);
};

struct hsic_debug_dev {
	char *read_buf;
	struct work_struct read_w;
	struct workqueue_struct *hsic_debug_wq;
#ifdef LG_FW_WAIT_QUEUE	
	wait_queue_head_t	read_wait_queue;
#endif
	unsigned long	flags;

	int hsic_device_enabled;
	int hsic_suspend;
/* We currently do not support HSIC-write to mdm*/
	int in_busy_hsic_write;
	int in_busy_hsic_read;

	struct hsic_debug_bridge_ops	ops;
};

/* START - EMS packet header, data */

typedef struct
{
	u16 mode; // mode is always 1 ( 0:test mode, 1:EMS mode )
	u16 total_size;
	u32 arch_type;
} sEMS_RX_Header;

typedef struct
{
	sEMS_RX_Header header;
	char filename[EMS_LOG_FORMAT_LEN_FILE_NAME];
	int line;
	char msg[EMS_LOG_FORMAT_LEN_ERR_MSG];
	char sw_ver[EMS_LOG_FORMAT_LEN_SW_VER];
} sEMS_RX_Data;
typedef struct 
{
	bool				valid;		// 0: invalid, 1: valid
	sEMS_RX_Data		rx_data;	// contains debugging information
} sEMS_PRx_Data;
/* END - EMS packet header, data */

#ifdef LG_FW_EMS_CONTINUOSLY_RECV
#define TMODE_TOTAL_SIZE 100
typedef struct
{
	u16 mode; // mode is always 1 ( 0:test mode, 1:EMS mode )
	u16 total_size;
} sEMS_Tmode_RX_Header;


typedef struct
{
	sEMS_Tmode_RX_Header header;
	char msg[TMODE_TOTAL_SIZE];
} sEMS_Tmode_RX_Data;
#endif




extern int hsic_debug_bridge_read(char *data, int size);
extern int hsic_debug_bridge_write(char *data, int size);
extern int hsic_debug_bridge_open(struct hsic_debug_bridge_ops *ops);
extern void hsic_debug_bridge_close(void);

void hsic_debug_display_log(void *buff);


#endif

