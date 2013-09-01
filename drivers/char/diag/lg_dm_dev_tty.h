#ifndef TTY_LGE_DM_DEV_H_
#define TTY_LGE_DM_DEV_H_

#ifdef CONFIG_LGE_DM_DEV

#define NUM_MODEM_CHIP    2
#define DM_DEV_TTY_IOCTL_MAGIC		'K'
#define DM_DEV_TTY_MODEM_OPEN		_IOWR(DM_DEV_TTY_IOCTL_MAGIC, 0x01, short)
#define DM_DEV_TTY_MODEM_CLOSE		_IOWR(DM_DEV_TTY_IOCTL_MAGIC, 0x02, short)
#define DM_DEV_TTY_MODEM_STATUS 	_IOWR(DM_DEV_TTY_IOCTL_MAGIC, 0x03, short)
#define DM_DEV_TTY_DATA_TO_APP		_IOWR(DM_DEV_TTY_IOCTL_MAGIC, 0x04, short)
#define DM_DEV_TTY_DATA_TO_USB		_IOWR(DM_DEV_TTY_IOCTL_MAGIC, 0x05, short)
#define DM_DEV_TTY_ENABLE			_IOWR(DM_DEV_TTY_IOCTL_MAGIC, 0x06, short)

struct dm_dev_tty {
  wait_queue_head_t   waitq;
  struct task_struct *tty_ts;
  struct tty_driver *tty_drv;
  struct tty_struct *tty_str;
  int tty_state;
  int is_modem_open[NUM_MODEM_CHIP + 1];
  int proc_num;
  int read_length;
  int set_logging;
  struct workqueue_struct *dm_dev_wq;
  struct work_struct dm_dev_usb_work;
};

extern struct dm_dev_tty *lge_dm_dev_tty;

#endif /* CONFIG_LGE_DM_DEV */
#endif /*TTY_LGE_DM_H_ */

