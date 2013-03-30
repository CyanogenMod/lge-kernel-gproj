extern int F54_FullRawCap(struct i2c_client *client, int mode, char *buf);
extern unsigned char F54_TxToTxReport(struct i2c_client *client);
extern unsigned char F54_TxToGndReport(struct i2c_client *client);
extern unsigned char F54_RxToRxReport(struct i2c_client *client);
extern unsigned char F54_HighResistance(struct i2c_client *client, char *buf);
extern unsigned char F54_RxOpenReport(struct i2c_client *client);
extern unsigned char F54_TxOpenReport(struct i2c_client *client);
