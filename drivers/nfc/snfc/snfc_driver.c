/*
 *  snfc.c
 *  
 */
 
/*
* Includes header files
*
*/

#include <linux/proc_fs.h>

#include "snfc_driver.h"


/*
* Defines
*
*/
#define SNFC_RXTX_LOG_ENABLE

/* Definition for transmit and receive buffer */
#define RECEIVE_BUFFER_MAX_SIZE 4096 /* SNFC Buffer size to be fixed */
#define TRANSMIT_BUFFER_MAX_SIZE 4096 

char snfc_receive_buf[RECEIVE_BUFFER_MAX_SIZE + 4];
char snfc_transmit_buf[TRANSMIT_BUFFER_MAX_SIZE + 4];

/*
*	External variables
*/

/*
*	Internal variables
*/
static int isopen = 0; // 0 : No open 1 : Open

static DEFINE_MUTEX(snfc_mutex);

/*
*	Function prototype
*/

/*
*	Function definition
*/
/*
* Description :
* Input : 
* Output :
*/
static int snfc_open (struct inode *inode, struct file *fp)
{
  int rc = 0;

#ifdef FEATURE_DEBUG_LOW 
  SNFC_DEBUG_MSG("[snfc] snfc_open() is called \n");
#endif

  /* Check input parameters */
  if(NULL == fp)
  {
	SNFC_DEBUG_MSG("[snfc] ERROR - fp \n");
	return -1;	
  }

  /* FileInputStream and FileOutPutStream open snfc
	 Only one case has to be excuted */
  if(1 == isopen)
  {
#ifdef FEATURE_DEBUG_LOW
	SNFC_DEBUG_MSG("[snfc] /dev/snfc is already openned. \n");
#endif

	return 0;
  }
  else
  {
#ifdef FEATURE_DEBUG_LOW
	SNFC_DEBUG_MSG("[snfc] snfc_open - start \n");
#endif

	isopen = 1;
  }
  
  rc = snfc_uart_open();

  //mdelay(100);

  if(rc)
  {
	SNFC_DEBUG_MSG("[snfc] ERROR - open_hs_uart \n");
	return rc;
  }

#ifdef FEATURE_DEBUG_LOW
  SNFC_DEBUG_MSG("[snfc] snfc_open - end \n");
#endif

#ifdef FELICA_FN_DEVICE_TEST
  SNFC_DEBUG_MSG("[snfc] snfc_open - result_open(%d) \n",result_open_uart);
  return result_open_uart;
#else
  return 0;
#endif
}


/*
* Description :
* Input : 
* Output :
*/

static ssize_t snfc_read(struct file *fp, char *buf, size_t count, loff_t *pos)
{
  int rc = 0;
  int readcount = 0;

  #ifdef FEATURE_DEBUG_LOW
  SNFC_DEBUG_MSG("[snfc] snfc_read - start \n");
  #endif

  /* Check input parameters */
  if(NULL == fp)
  {
    SNFC_DEBUG_MSG("[snfc] ERROR - fp \n");
    return -1;    
  }
  
  if(NULL == buf)
  {
    SNFC_DEBUG_MSG("[snfc] ERROR - buf \n");
    return -1;    
  }

  if(count > RECEIVE_BUFFER_MAX_SIZE)
  {
    SNFC_DEBUG_MSG("[snfc] ERROR - count \n");
    return -1;    
  }

  if(NULL == pos)
  {
    SNFC_DEBUG_MSG("[snfc] ERROR - pos \n");
    return -1;    
  }

  memset(snfc_receive_buf, 0, sizeof(snfc_receive_buf));

  /* Copy UART receive data to receive buffer */
  mutex_lock(&snfc_mutex);
  readcount = snfc_uart_read(snfc_receive_buf,count);
  mutex_unlock(&snfc_mutex);

  if(0 >= readcount)
  {
    SNFC_DEBUG_MSG("[snfc] ERROR - No data in data buffer \n");
    return 0;
  }

  /* Copy receive buffer to user memory */
  rc = copy_to_user(buf, snfc_receive_buf, count);

  if (rc) {
    SNFC_DEBUG_MSG("[snfc] ERROR - copy_to_user \n");
    return rc;
  }

  #ifdef FEATURE_DEBUG_LOW
  SNFC_DEBUG_MSG("[snfc] snfc_read - end \n");
  #endif

  return readcount;
}


/*
* Description :
* Input : 
* Output :
*/
static ssize_t snfc_write(struct file *fp, const char *buf, size_t count, loff_t *f_pos)
{
  int rc = 0;
  int writecount = 0;

  #ifdef FEATURE_DEBUG_LOW
  SNFC_DEBUG_MSG("[snfc] snfc_write - start \n");
  #endif
 
  /* Check input parameters */
  if(NULL == fp)
  {
    SNFC_DEBUG_MSG("[snfc] ERROR - fp \n");
    return -1;  
  }

  if(NULL == buf)
  {
    SNFC_DEBUG_MSG("[snfc] ERROR - buf \n");
    return -1;  
  }

  if(count > TRANSMIT_BUFFER_MAX_SIZE)
  {
    SNFC_DEBUG_MSG("[snfc] ERROR - count \n");
    return -1;  
  }
  
  /* Clear transmit buffer before using */
  memset(snfc_transmit_buf, 0, sizeof(snfc_transmit_buf));

  /* Copy user memory to kernel memory */
  rc = copy_from_user(snfc_transmit_buf, buf, count);
  if (rc) {
    SNFC_DEBUG_MSG("[snfc] ERROR - copy_to_user \n");
    return rc;
  }

  /* Send transmit data to UART transmit buffer */
  mutex_lock(&snfc_mutex);
  writecount = snfc_uart_write(snfc_transmit_buf,count);
  mutex_unlock(&snfc_mutex);

  #ifdef FEATURE_DEBUG_LOW
  SNFC_DEBUG_MSG("[snfc] writecount : %d \n",writecount);
  #endif

  return writecount;
}
/*
* Description :
* Input : 
* Output :
*/
static int snfc_release (struct inode *inode, struct file *fp)
{
  int rc = 0;

  /* Check input parameters */
  if(NULL == fp)
  {
    SNFC_DEBUG_MSG("[snfc] ERROR - fp \n");
    return -1;  
  }
  /* FileInputStream and FileOutPutStream close snfc
     Only one case has to be excuted */
  if(0 == isopen)
  {
    return 0;
  }
  else
  {
    isopen = 0;
  }

  #ifdef FEATURE_DEBUG_LOW
  SNFC_DEBUG_MSG("[snfc] snfc_release - start \n");
  #endif

  rc = snfc_uart_close();
  if(rc)
  {
    SNFC_DEBUG_MSG("[snfc] ERROR - open_hs_uart \n");
    return rc;
  }

  #ifdef FEATURE_DEBUG_LOW
  SNFC_DEBUG_MSG("[snfc] snfc_release - end \n");
  #endif

  return 0;
}

/*
* Description : 
* Input : None
* Output : 
*/
static long snfc_ioctl (struct file *fp, unsigned int cmd, unsigned long arg)
{
  int numofreceiveddata = 0;
  int rc = 0;
  int *uarg = (int *)arg;

  #ifdef FEATURE_DEBUG_LOW
  SNFC_DEBUG_MSG("[snfc] snfc_ioctl - start \n");
  #endif

  /* Check input parameters */
  if(NULL == fp)
  {
    SNFC_DEBUG_MSG("[snfc] ERROR - fp \n");
    return -1;  
  }
/*
  if(IOCTL_FELICA_MAGIC != _IOC_TYPE(cmd)) 
  {
    SNFC_DEBUG_MSG("[snfc] ERROR - IO cmd type \n");
    return -1;
  }

  if(IOCTL_FELICA_CMD_AVAILABLE != _IOC_NR(cmd)) 
  {
    SNFC_DEBUG_MSG("[snfc] ERROR - IO cmd number \n");
    return -1;
  }

  if(0 != _IOC_SIZE(cmd)) 
  {
    SNFC_DEBUG_MSG("[snfc] ERROR - IO cmd size \n");
    return -1;
  }
*/
  /* temp */
  if('T' == _IOC_TYPE(cmd)) 
  	SNFC_DEBUG_MSG("[snfc] _IOC_TYPE(cmd) = 'T'");
  if(0x1B == _IOC_NR(cmd))
  	SNFC_DEBUG_MSG("[snfc] _IOC_NR(cmd) = 0x1B");
  SNFC_DEBUG_MSG("[snfc] _IOC_SIZE(cmd) = %d",_IOC_SIZE(cmd));
  
  mutex_lock(&snfc_mutex);
  rc = snfc_uart_ioctrl(&numofreceiveddata);
  mutex_unlock(&snfc_mutex);

  if (rc) {
    SNFC_DEBUG_MSG("[snfc] ERROR - snfc_uart_ioctrl \n");
    return rc;
  }

  rc = copy_to_user(uarg, &numofreceiveddata, sizeof(int));
  if(rc)
  {
    SNFC_DEBUG_MSG("[snfc] ERROR - open_hs_uart \n");
    return rc;
  }

  #ifdef FEATURE_DEBUG_LOW
  SNFC_DEBUG_MSG("[snfc] felica_ioctl - end \n");
  #endif

  return rc;
}

static struct file_operations snfc_fops = {
	.owner			= THIS_MODULE,
	.open			= snfc_open,
	.read			= snfc_read,
	.write			= snfc_write,
	.unlocked_ioctl	= snfc_ioctl,
	//.fsync    = NULL,
	.release		= snfc_release,
};

static struct miscdevice snfc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "SNFC",
	.fops = &snfc_fops,
};


/*
* Description :
* Input : 
* Output :
*/
static int snfc_init(void)
{
	int rc = 0;
		//struct proc_dir_entry *d_entry;

	#ifdef FEATURE_DEBUG_LOW
	SNFC_DEBUG_MSG("[snfc] snfc_init - start \n");
	#endif

	/* register the device file */
	rc = misc_register(&snfc_device);
	if(rc)
	{
		SNFC_DEBUG_MSG("[snfc] snfc init : error : can not register : %d\n", rc);
		return rc;
	}

	#ifdef FEATURE_DEBUG_LOW
	SNFC_DEBUG_MSG("[snfc] snfc_init - end \n");
	#endif
	/*
	d_entry = create_proc_entry("msm_snfc_uart",
			S_IRUGO | S_IWUSR | S_IWGRP, NULL);
	if (d_entry) {
		d_entry->read_proc = NULL;
		d_entry->write_proc = msm_snfc_uart_write_proc;
		d_entry->data = NULL;
	}
	*/
	return 0;
}

/*
* Description :
* Input : 
* Output :
*/
static void snfc_exit(void)
{
	#ifdef FEATURE_DEBUG_LOW
	SNFC_DEBUG_MSG("[snfc] snfc_exit - start \n");
	#endif

	/* deregister the device file */
	misc_deregister(&snfc_device);

	#ifdef FEATURE_DEBUG_LOW
	SNFC_DEBUG_MSG("[snfc] snfc_exit - end \n");
	#endif
}

module_init(snfc_init);
module_exit(snfc_exit);

MODULE_LICENSE("Dual BSD/GPL");

