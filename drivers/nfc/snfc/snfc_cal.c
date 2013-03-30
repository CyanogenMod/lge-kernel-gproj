/*
 *  snfc_cal.c
 *  
 */

 /*
  *    Include header files
  */
//#include <linux/module.h>
#include <linux/kernel.h>

#include "felica_cal.h"

/*
 *    Internal definition
 */

#define SNFC_I2C_SLAVE_ADDRESS  0x56
#define SNFC_I2C_REG_ADDRSS_01  0x01
#define SNFC_I2C_REG_ADDRSS_02  0x02

/*
 *    Function definition
 */

 /*
 * Description : 
 * Input :
 * Output :
 */
 static int snfc_cal_open (struct inode *inode, struct file *fp)
 {
  #ifdef FEATURE_DEBUG_LOW
   SNFC_DEBUG_MSG("[snfc_cal] snfc_cal_open\n");
  #endif
 
   return 0;
 }

 /*
 * Description : 
 * Input :
 * Output :
 */
 static int snfc_cal_release (struct inode *inode, struct file *fp)
 {
  #ifdef FEATURE_DEBUG_LOW
   SNFC_DEBUG_MSG("[snfc_cal] snfc_cal_release \n");
  #endif
 
   return 0;
 }
 
 /*
 * Description : 
 * Input :
 * Output :
 */
 static ssize_t snfc_cal_read(struct file *fp, char *buf, size_t count, loff_t *pos)
 {
   unsigned char read_buf = 0x00;
   int rc = -1;
 
  #ifdef FEATURE_DEBUG_LOW
   SNFC_DEBUG_MSG("[snfc_cal] felica_cal_read - start \n");
  #endif
 
 /* Check error */
   if(NULL == fp)
   {
	 SNFC_DEBUG_MSG("[snfc_cal] ERROR fp \n");
	 return -1;    
   }
 
   if(NULL == buf)
   {
	 SNFC_DEBUG_MSG("[snfc_cal] ERROR buf \n");
	 return -1;    
   }
   
   if(1 != count)
   {
	 SNFC_DEBUG_MSG("[snfc_cal] ERROR count \n");
	 return -1;    
   }
 
   if(NULL == pos)
   {
	 SNFC_DEBUG_MSG("[snfc_cal] ERROR file \n");
	 return -1;    
   }
 
 
 
   rc = snfc_i2c_read(SNFC_I2C_REG_ADDRSS_01, &read_buf, 1);
   if(rc)
   {
	 SNFC_DEBUG_MSG("[snfc_cal] felica_i2c_read : %d \n",rc);
	 return -1;
   }
 
  #ifdef FEATURE_DEBUG_LOW
   SNFC_DEBUG_MSG("[snfc_cal] felica_cal : 0x%02x \n",read_buf);
  #endif
 
   rc = copy_to_user(buf, &read_buf, count);
   if(rc)
   {
	 SNFC_DEBUG_MSG("[snfc_cal] ERROR - copy_from_user \n");
	 return -1;
   }
 
  #ifdef FEATURE_DEBUG_LOW
   SNFC_DEBUG_MSG("[snfc_cal] felica_cal_read - end \n");
  #endif
 
   return 1;
 }

 
 /*
 * Description : 
 * Input :
 * Output :
 */
 static ssize_t snfc_cal_write(struct file *fp, const char *buf, size_t count, loff_t *pos)
 {
   unsigned char write_buf = 0x00, read_buf = 0x00;
   int rc = -1;
   
  #ifdef FEATURE_DEBUG_LOW
   SNFC_DEBUG_MSG("[snfc_cal] felica_cal_write - start \n");
  #endif
 
 /* Check error */
   if(NULL == fp)
   {
	 SNFC_DEBUG_MSG("[snfc_cal] ERROR file \n");
	 return -1;    
   }
 
   if(NULL == buf)
   {
	 SNFC_DEBUG_MSG("[snfc_cal] ERROR buf \n");
	 return -1;    
   }
   
   if(1 != count)
   {
	 SNFC_DEBUG_MSG("[snfc_cal]ERROR count \n");
	 return -1;    
   }
 
   if(NULL == pos)
   {
	 SNFC_DEBUG_MSG("[snfc_cal] ERROR file \n");
	 return -1;    
   }
 
   /* copy from user data */
   rc = copy_from_user(&write_buf, buf, count);
   if(rc)
   {
	 SNFC_DEBUG_MSG("[snfc_cal] ERROR - copy_from_user \n");
	 return -1;
   }
 
  #ifdef FEATURE_DEBUG_LOW
   SNFC_DEBUG_MSG("[snfc_cal] write_buf : 0x%02x \n",write_buf);
  #endif
 
   /* read register value before writing new value */
   rc = snfc_i2c_read(SNFC_I2C_REG_ADDRSS_01, &read_buf, 1);
   udelay(50);
 
   /* write new value */
   write_buf = write_buf | 0x80;
   rc = snfc_i2c_write(SNFC_I2C_REG_ADDRSS_01, &write_buf, 1);
   mdelay(2);
 
   /* read register value after writing new value */
   rc = snfc_i2c_read(SNFC_I2C_REG_ADDRSS_01, &read_buf, 1);
   udelay(50);
 
  #ifdef FEATURE_DEBUG_LOW
   SNFC_DEBUG_MSG("[snfc_cal] felica_cal_write - end \n");
  #endif
   
   return 1;
 }

static struct file_operations snfc_cal_fops = 
{
  .owner    = THIS_MODULE,
  .open    = snfc_cal_open,
  .read    = snfc_cal_read,
  .write    = snfc_cal_write,
  .release  = snfc_cal_release,
};

static struct miscdevice snfc_cal_device = 
{
  .minor = MISC_DYNAMIC_MINOR,
  .name = "snfc_cal",
  .fops = &snfc_cal_fops
};

/*
* Description : 
* Input :
* Output :
*/
static int snfc_cal_init(void)
{
  int rc = -1;

  #ifdef FEATURE_DEBUG_LOW
  SNFC_DEBUG_MSG("[snfc_cal] snfc_cal_init - start \n");
  #endif

  /* register the device file */
  rc = misc_register(&snfc_cal_device);
  if (rc < 0)
  {
    SNFC_DEBUG_MSG("[snfc_cal] ERROR - can not register felica_cal \n");
    return rc;
  }
  
  #ifdef FEATURE_DEBUG_LOW
  SNFC_DEBUG_MSG("[snfc_cal] snfc_cal_init - end \n");
  #endif

  return 0;
}

/*
* Description : 
* Input :
* Output :
*/
static void snfc_cal_exit(void)
{
  #ifdef FEATURE_DEBUG_LOW
  SNFC_DEBUG_MSG("[snfc_cal] snfc_cal_exit - start \n");
  #endif

  /* deregister the device file */
  misc_deregister(&snfc_cal_device);

  #ifdef FEATURE_DEBUG_LOW
  SNFC_DEBUG_MSG("[snfc_cal] snfc_cal_exit - end \n");
  #endif
}

module_init(snfc_cal_init);
module_exit(snfc_cal_exit);

MODULE_LICENSE("Dual BSD/GPL");

