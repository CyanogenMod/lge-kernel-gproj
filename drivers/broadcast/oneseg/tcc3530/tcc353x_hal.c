
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include "tcpal_os.h"
#include "tcc353x_hal.h"

#define INCLUDE_LGE_SRC_EAR_ANT_SEL
#ifdef INCLUDE_LGE_SRC_EAR_ANT_SEL
#include "../../../../arch/arm/mach-msm/lge/gv/board-gv.h"	/* PM8921_GPIO_PM_TO_SYS() */
#endif

#define ISDB_EN			85 	/* GPIO 85 */
#define ISDB_INT_N		77 	/* GPIO 77 */
#define ISDB_RESET_N  		1 	/* GPIO 1 */

#ifdef INCLUDE_LGE_SRC_EAR_ANT_SEL
#define ONESEG_EAR_ANT_SEL_P	PM8921_GPIO_PM_TO_SYS(11)	/* Internel/Ear antenna switch */
#endif

void TchalInit(void)
{
	gpio_request(ISDB_RESET_N, "ISDB_RESET");
	gpio_request(ISDB_EN, "ISDB_EN");
	gpio_request(ISDB_INT_N, "ISDB_INT");

#ifdef INCLUDE_LGE_SRC_EAR_ANT_SEL
	/* Internel antenna:OFF, Ear antenna: ON, GPIO11:LOW (Saving power)*/
	gpio_set_value_cansleep(ONESEG_EAR_ANT_SEL_P, 0); /* PMIC Extended GPIO */
#endif

	gpio_direction_output(ISDB_RESET_N, false); 	/* output low */
	gpio_direction_output(ISDB_EN, false); 		/* output low */
	gpio_direction_input(ISDB_INT_N); 		/* input */

	TcpalPrintStatus((I08S *)"[%s:%d]\n", __func__, __LINE__);
}

void TchalResetDevice(void)
{
	gpio_set_value(ISDB_RESET_N, 1);		/* high ISDB_RESET_N */
	TcpalmSleep(5);
	gpio_set_value(ISDB_RESET_N, 0);		/* low ISDB_RESET_N */
	TcpalmSleep(5);
	gpio_set_value(ISDB_RESET_N, 1);		/* high ISDB_RESET_N */
	TcpalmSleep(5);

	TcpalPrintStatus((I08S *)"[%s:%d]\n", __func__, __LINE__);
}

void TchalPowerOnDevice(void)
{

#ifdef INCLUDE_LGE_SRC_EAR_ANT_SEL
	/* Internel antenna:ON, Ear antenna: OFF, GPIO11: HIGH (Default: Use Internel Antenna )*/
	gpio_set_value_cansleep(ONESEG_EAR_ANT_SEL_P, 1); /* PMIC Extended GPIO */
#endif
	gpio_direction_output(ISDB_EN, false); 		/* output low */
	gpio_direction_output(ISDB_RESET_N, false); 	/* output low */

	gpio_set_value(ISDB_EN, 1);			/* high ISDB_EN */
	TcpalmSleep(10);
	TchalResetDevice();
	TchalIrqSetup();

	TcpalPrintStatus((I08S *)"[%s:%d]\n", __func__, __LINE__);
}

void TchalPowerDownDevice(void)
{
	gpio_set_value(ISDB_RESET_N, 0);		/* low ISDB_RESET_N */
	TcpalmSleep(5);
	gpio_set_value(ISDB_EN, 0);			/* low ISDB_EN */

#ifdef INCLUDE_LGE_SRC_EAR_ANT_SEL
	/* Internel antenna:OFF, Ear antenna: ON, GPIO11:LOW (Saving power)*/
	gpio_set_value_cansleep(ONESEG_EAR_ANT_SEL_P, 0); /* PMIC Extended GPIO */
#endif

	TcpalPrintStatus((I08S *)"[%s:%d]\n", __func__, __LINE__);
}

void TchalIrqSetup(void)
{
	gpio_direction_input(ISDB_INT_N);		/* input mode */
}

