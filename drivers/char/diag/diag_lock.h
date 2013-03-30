#ifndef __DIAG_LOCK_H__
#define __DIAG_LOCK_H__

#include <linux/types.h>

#define DIAG_ENABLE     1
#define DIAG_DISABLE    0

#define diag_lock()     set_diag_state(DIAG_DISABLE)
#define diag_unlock()   set_diag_state(DIAG_ENABLE)
#define diag_state()    get_diag_state()

int get_diag_state(void);
void set_diag_state(int enable);

#endif /* __DIAG_LOCK_H__ */
