/*
 *  snfc_uart.h
 *  
 */

#ifndef __SNFC_UART_H__
#define __SNFC_UART_H__

/*
 *  INCLUDE FILES FOR MODULE
 */
#include "snfc_common.h"

/*
 *  DEFINE
 */
int snfc_uart_open(void);
int snfc_uart_close(void);
int snfc_uart_write(char *buf, size_t count);
int snfc_uart_read(char *buf, size_t count);
int snfc_uart_ioctrl(int *count);

#endif // __SNFC_UART_H__

