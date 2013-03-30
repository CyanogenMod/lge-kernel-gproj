#ifndef MHL_DEVICE
#define MHL_DEVICE

struct mhl_platform_data
{
    int (*power)(bool on, bool pm_ctrl);
};

#endif /* MHL_DEVICE */
