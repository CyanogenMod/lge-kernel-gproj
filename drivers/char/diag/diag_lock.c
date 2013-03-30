#include "diag_lock.h"

#include <linux/syscalls.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

/* refer from android/vendor/lge/bsp/atd_demigod/lgftmitem/ftmitem.h */
#define LGFTM_PORTLOCK_STATE        26
#define LGFTM_PORTLOCK_STATE_SIZE   1
#define FTM_BLOCK_SIZE              2048

static const char *ftmdev = "/dev/block/platform/msm_sdcc.1/by-name/misc";

static int get_ftm_portlock_item(void)
{
    int fd;
    mm_segment_t old_fs;
    int offset = (LGFTM_PORTLOCK_STATE + 1) * FTM_BLOCK_SIZE;
    int enable = DIAG_DISABLE;
    char c;

    old_fs = get_fs();
    set_fs(get_ds());

    fd = sys_open(ftmdev, O_RDWR, 0);
    if (fd < 0)
    {
        pr_err("%s: sys_open error(%d)\n", __func__, fd);
    }
    else
    {
        sys_lseek(fd, offset, 0);
        if (sys_read(fd, &c, 1) != 1)
            pr_err("%s: sys_read error\n", __func__);

        enable = c - '0';

        switch (enable)
        {
            case DIAG_ENABLE:
            case DIAG_DISABLE:
                break;

            default:
                pr_info("%s: portlock is not set\n", __func__);

#ifdef CONFIG_LGE_USB_DIAG_DISABLE_ONLY_MDM
                c = '0';
#else
                c = '1';
#endif
                sys_lseek(fd, offset, 0);
                if (sys_write(fd, &c, 1) != 1)
                    pr_err("%s: sys_write error\n", __func__);

#ifdef CONFIG_LGE_USB_DIAG_DISABLE_ONLY_MDM
                enable = DIAG_DISABLE;
#else
                enable = DIAG_ENABLE;
#endif
                break;
        }

        sys_close(fd);
    }

    set_fs(old_fs);

    pr_info("%s: diag %s\n", __func__, enable ? "enabled" : "disabled");

    return enable;
}

static void set_ftm_portlock_item(int enable)
{
    int fd;
    mm_segment_t old_fs;
    int offset = (LGFTM_PORTLOCK_STATE + 1) * FTM_BLOCK_SIZE;
    char c = enable + '0';

    old_fs = get_fs();
    set_fs(get_ds());

    fd = sys_open(ftmdev, O_WRONLY, 0);
    if (fd < 0)
    {
        pr_err("%s: sys_open error(%d)\n", __func__, fd);
    }
    else
    {
        sys_lseek(fd, offset, 0);
        if (sys_write(fd, &c, 1) != 1)
            pr_err("%s: sys_write error\n", __func__);
        else
            pr_info("%s: diag %s\n", __func__, enable ? "enabled" : "disabled");

        sys_close(fd);
    }

    set_fs(old_fs);

}

int get_diag_state(void)
{
    return get_ftm_portlock_item();
}

void set_diag_state(int enable)
{
    set_ftm_portlock_item(enable);
}
