#include <linux/i2c.h>

#define SYNAPTICS_TS_I2C_INT_GPIO 6

extern int touch_i2c_read(struct i2c_client *client, u8 reg, int len, u8 *buf);
extern int touch_i2c_write(struct i2c_client *client, u8 reg, int len, u8 * buf);
extern struct i2c_client* ds4_i2c_client;

void device_I2C_read(unsigned char add, unsigned char *value, unsigned short len);
void device_I2C_write(unsigned char add, unsigned char *value, unsigned short len);
void InitPage(void);
void SetPage(unsigned char page);
void readRMI(unsigned short add, unsigned char *value, unsigned short len);
void writeRMI(unsigned short add, unsigned char *value, unsigned short len);
void delayMS(int val);
void cleanExit(int code);
int waitATTN(int code, int time);
void write_log(char *data);