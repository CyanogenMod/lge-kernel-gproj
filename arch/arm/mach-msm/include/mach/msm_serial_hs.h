/*
 * Copyright (C) 2008 Google, Inc.
 * Author: Nick Pelly <npelly@google.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __ASM_ARCH_MSM_SERIAL_HS_H
#define __ASM_ARCH_MSM_SERIAL_HS_H

#include<linux/serial_core.h>

/* Optional platform device data for msm_serial_hs driver.
 * Used to configure low power wakeup */
struct msm_serial_hs_platform_data {
	int wakeup_irq;  /* wakeup irq */
	/* bool: inject char into rx tty on wakeup */
	unsigned char inject_rx_on_wakeup;
	char rx_to_inject;
	int (*gpio_config)(int);
	int userid;
};

// [S] LGE_BT: ADD/ilbeom.kim/'12-10-24 - [GK] added BRCM solution
//BEGIN: 0019639 chanha.park@lge.com 2012-06-16
//ADD: 0019639: [F200][BT] Support Bluetooth low power mode
#ifdef CONFIG_LGE_BLUESLEEP
#define CLOCK_REQUEST_AVAILABLE 	0
#define CLOCK_REQUEST_UNAVAILABLE 	1
struct uart_port * msm_hs_get_bt_uport(unsigned int line);
int msm_hs_get_bt_uport_clock_state(struct uart_port *uport);
#endif/*CONFIG_LGE_BLUESLEEP*/
//END: 0019639 chanha.park@lge.com 2012-06-16
// [E] LGE_BT: ADD/ilbeom.kim/'12-10-24 - [GK] added BRCM solution

unsigned int msm_hs_tx_empty(struct uart_port *uport);
void msm_hs_request_clock_off(struct uart_port *uport);
void msm_hs_request_clock_on(struct uart_port *uport);
void msm_hs_set_mctrl(struct uart_port *uport,
				    unsigned int mctrl);
#endif
