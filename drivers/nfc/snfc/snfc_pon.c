/*
 *	snfc_pon.c
 *  
 */
 
/*
 *    Include header files
 */
#include "snfc_pon.h"
#include "snfc_gpio.h"

/*
 *  Defines
 */

/*
*    Internal definition
*/
  
/*
 *	  Internal variables
 */
static int isopen_snfcpon = 0; // 0 : No open 1 : Open

/*
 *    Function prototypes
 */

/*
 *    Function definitions
 */

/*
* Description :
* Input : 
* Output :
*/
static int snfc_pon_open (struct inode *inode, struct file *fp)
{
	int rc = 0;

	if(isopen_snfcpon == 1)
	{
		#ifdef FEATURE_DEBUG_LOW 
		SNFC_DEBUG_MSG("[snfc_pon] snfc_pon_open - already open \n");
		#endif
		return 0;
	}

	SNFC_DEBUG_MSG("[snfc_pon] snfc_pon_open - start \n");

	rc = snfc_gpio_open(GPIO_SNFC_PON, GPIO_DIRECTION_OUT, GPIO_LOW_VALUE);

	#ifdef FEATURE_DEBUG_LOW 
	SNFC_DEBUG_MSG("[snfc_pon] snfc_pon_open - end \n");
	#endif

	isopen_snfcpon = 1;

	return rc;
}


/*
* Description :
* Input : 
* Output :
*/
static ssize_t snfc_pon_write(struct file *pf, const char *pbuf, size_t size, loff_t *pos)
{
	int rc = 0;
	int SetValue = 0;

	#ifdef FEATURE_DEBUG_LOW 
	SNFC_DEBUG_MSG("[snfc_pon] snfc_pon_write - start \n");
	#endif

	/* Check parameters */
	if(pf == NULL || pbuf == NULL /*|| size == NULL*/ /*|| pos ==NULL*/)
	{
		SNFC_DEBUG_MSG("[snfc_pon] snfc_pon_write parameter error pf = %p, pbuf = %p, size =%d, pos =%p \n", pf, pbuf, (int)size, pos);
		return -1;    
	}
	/* Get value from userspace */
	rc = copy_from_user(&SetValue, (void*)pbuf, size);
	if(rc)
	{
		SNFC_DEBUG_MSG("[snfc_pon] ERROR - copy_from_user rc=%d \n", rc);
		return rc;
	}

	if((SetValue != GPIO_LOW_VALUE)&&(SetValue != GPIO_HIGH_VALUE))
	{
		SNFC_DEBUG_MSG("[snfc_pon] ERROR - SetValue is out of range \n");
		return -1;
	}

	if(SetValue)
		SNFC_DEBUG_MSG("[snfc_pon] snfc_pon is high \n");
	else
		SNFC_DEBUG_MSG("[snfc_pon] snfc_pon is low \n");

	snfc_gpio_write(GPIO_SNFC_PON, SetValue);

	mdelay(20);

	return 1;
}


/*
* Description : 
* Input : 
* Output : 
*/
static int snfc_pon_release (struct inode *inode, struct file *fp)
{
	if(isopen_snfcpon ==0)
	{
		#ifdef FEATURE_DEBUG_LOW 
		SNFC_DEBUG_MSG("[snfc_pon] snfc_pon_release - not open \n");
		#endif
		return -1;
	}

	SNFC_DEBUG_MSG("[snfc_pon] snfc_pon_release  <======== OFF \n");
	snfc_gpio_write(GPIO_SNFC_PON, GPIO_LOW_VALUE);

	#ifdef FEATURE_DEBUG_LOW 
	SNFC_DEBUG_MSG("[snfc_pon] snfc_pon_release - end \n");
	#endif

	isopen_snfcpon = 0;

	return 0;

}

static struct file_operations snfc_pon_fops = 
{
  .owner    = THIS_MODULE,
  .open      = snfc_pon_open,
  .write    = snfc_pon_write,
  .release  = snfc_pon_release,
};

static struct miscdevice snfc_pon_device = 
{
  .minor = 124,
  .name = "snfc_pon",
  .fops = &snfc_pon_fops,
};

static int snfc_pon_init(void)
{
	int rc = 0;

	#ifdef FEATURE_DEBUG_LOW 
	SNFC_DEBUG_MSG("[snfc_pon] snfc_pon_init - start \n");
	#endif

	/* Register the device file */
	rc = misc_register(&snfc_pon_device);
	if (rc)
	{
		SNFC_DEBUG_MSG("[snfc_pon] FAIL!! can not register snfc_pon \n");
		return rc;
	}

	#ifdef FEATURE_DEBUG_LOW 
	SNFC_DEBUG_MSG("[snfc_pon] snfc_pon_init - end \n");
	#endif

	return 0;
}

static void snfc_pon_exit(void)
{
	#ifdef FEATURE_DEBUG_LOW 
	SNFC_DEBUG_MSG("[snfc_pon] snfc_pon_exit - start \n");
	#endif

	/* Deregister the device file */
	misc_deregister(&snfc_pon_device);

	#ifdef FEATURE_DEBUG_LOW 
	SNFC_DEBUG_MSG("[snfc_pon] snfc_pon_exit - end \n");
	#endif
}

module_init(snfc_pon_init);
module_exit(snfc_pon_exit);

MODULE_LICENSE("Dual BSD/GPL");

