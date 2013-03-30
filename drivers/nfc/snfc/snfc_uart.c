/*
 *  felicauart.c
 *  
 */

 /*
  *    include header files
  */

#include <linux/syscalls.h>
#include <asm/termios.h>
  
#include "snfc_uart.h"

/*Internal defines*/
static struct file *snfc_uart_f = NULL;

/*
 * Description : open uart
 * Input : None
 * Output : success : 0 fail : others
 */
int snfc_uart_open(void)
{
    struct termios newtio;
    mm_segment_t old_fs = get_fs();

#ifdef FEATURE_DEBUG_LOW
    SNFC_DEBUG_MSG("[snfc_uart] open_hs_uart - start \n");
#endif

    if (snfc_uart_f != NULL)
    {
        SNFC_DEBUG_MSG("[snfc_uart] snfc_uart is already opened\n");
        return 0;
    }
    set_fs(KERNEL_DS);

    snfc_uart_f = filp_open("/dev/ttyHSL1", O_RDWR | O_NOCTTY | O_NONBLOCK, 0);


#ifdef FEATURE_DEBUG_LOW
    SNFC_DEBUG_MSG("[snfc_uart] open UART\n");
#endif

    if (snfc_uart_f == NULL)
    {
        SNFC_DEBUG_MSG("[snfc_uart] ERROR - can not sys_open \n");
        set_fs(old_fs);
        return -1;
    }

    memset(&newtio, 0, sizeof(newtio));
    newtio.c_cflag = B460800 | CS8 | CLOCAL | CREAD;
    newtio.c_cc[VMIN] = 1;
    newtio.c_cc[VTIME] = 5;
    do_vfs_ioctl(snfc_uart_f, -1, TCFLSH, TCIOFLUSH);
    do_vfs_ioctl(snfc_uart_f, -1, TCSETSF, (unsigned long)&newtio);

    set_fs(old_fs);

#ifdef FEATURE_DEBUG_LOW
    SNFC_DEBUG_MSG("[snfc_uart] open_hs_uart - end \n");
#endif

    return 0;
}

/*
 * Description : close uart
 * Input : None
 * Output : success : 0
 */
int snfc_uart_close(void)
{
    mm_segment_t old_fs = get_fs();

#ifdef FEATURE_DEBUG_LOW
    SNFC_DEBUG_MSG("[snfc_uart] close_hs_uart - start \n");
#endif

    if (snfc_uart_f == NULL)
    {
        SNFC_DEBUG_MSG("[snfc_uart] snfc_uart is not opened\n");
        return 0;
    }

    set_fs(KERNEL_DS);
    filp_close(snfc_uart_f, NULL);
    snfc_uart_f = NULL;
    set_fs(old_fs);

#ifdef FEATURE_DEBUG_LOW
    SNFC_DEBUG_MSG("[snfc_uart] close_hs_uart - end \n");
#endif

    return 0;
}


/*
 * Description : write data to uart
 * Input : buf : data count : data length
 * Output : success : data length fail : 0
 */
int snfc_uart_write(char *buf, size_t count)
{
    mm_segment_t old_fs = get_fs();
    int n;

#ifdef FEATURE_DEBUG_LOW
    SNFC_DEBUG_MSG("[snfc_uart] write_hs_uart - start \n");
#endif

    if (snfc_uart_f == NULL)
    {
        SNFC_DEBUG_MSG("[snfc_uart] snfc_uart is not opened\n");
        return 0;
    }

    set_fs(KERNEL_DS);
    
    n = vfs_write(snfc_uart_f, buf, count, &snfc_uart_f->f_pos);
    
#ifdef FEATURE_DEBUG_LOW
    SNFC_DEBUG_MSG("[snfc_uart] write_hs_uart - write (%d)\n", n);
#endif

    set_fs(old_fs);

#ifdef FEATURE_DEBUG_LOW
    SNFC_DEBUG_MSG("[snfc_uart] write_hs_uart - end \n");
#endif

    return n;
}


/*
 * Description : read data from uart
 * Input : buf : data count : data length
 * Output : success : data length fail : 0
 */
int snfc_uart_read(char *buf, size_t count)
{
    mm_segment_t old_fs = get_fs();
    int n;
    int retry = 5;

#ifdef FEATURE_DEBUG_LOW
    SNFC_DEBUG_MSG("[snfc_uart] read_hs_uart - start \n");
#endif

    if (snfc_uart_f == NULL)
    {
        SNFC_DEBUG_MSG("[snfc_uart] snfc_uart is not opened\n");
        return 0;
    }

    set_fs(KERNEL_DS);

    while ((n = vfs_read(snfc_uart_f, buf, count, &snfc_uart_f->f_pos)) == -EAGAIN && retry > 0)
    {
        mdelay(10);
#ifdef FEATURE_DEBUG_LOW
        SNFC_DEBUG_MSG("[snfc_uart] snfc_uart_read - delay : %d \n", retry);
#endif
        retry--;
    }


#ifdef FEATURE_DEBUG_LOW
    SNFC_DEBUG_MSG("[snfc_uart] read_hs_uart - count : %d numofreaddata : %d\n",count ,n);
#endif

    set_fs(old_fs);

#ifdef FEATURE_DEBUG_LOW
    SNFC_DEBUG_MSG("[snfc_uart] read_hs_uart - end \n");
#endif

    return n;
}

/*
 * Description : get size of remaing data
 * Input : none
 * Output : success : data length fail : 0
 */
int snfc_uart_ioctrl(int *count)
{
    mm_segment_t old_fs = get_fs();
    int n;

#ifdef FEATURE_DEBUG_LOW
    SNFC_DEBUG_MSG("[snfc_uart] snfc_uart_ioctrl - start \n");
#endif

    if (snfc_uart_f == NULL)
    {
        SNFC_DEBUG_MSG("[snfc_uart] snfc_uart is not opened\n");
        return 0;
    }

    set_fs(KERNEL_DS);
    n = do_vfs_ioctl(snfc_uart_f, -1, TIOCINQ, (unsigned long)count);
#ifdef FEATURE_DEBUG_LOW
    SNFC_DEBUG_MSG("[snfc_uart] do_vfs_ioctl return %d, count=%d\n", n, *count);
#endif
    set_fs(old_fs);

#ifdef FEATURE_DEBUG_LOW
    SNFC_DEBUG_MSG("[snfc_uart] snfc_uart_ioctrl - end \n");
#endif

    return n;
}

