/* include/linux/wakelock.h
 *
 * Copyright (C) 2007-2008 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _LINUX_WAKELOCK_H
#define _LINUX_WAKELOCK_H

#include <linux/list.h>
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/kallsyms.h>

/* A wake_lock prevents the system from entering suspend or other low power
 * states when active. If the type is set to WAKE_LOCK_SUSPEND, the wake_lock
 * prevents a full system suspend. If the type is WAKE_LOCK_IDLE, low power
 * states that cause large interrupt latencies or that disable a set of
 * interrupts will not entered from idle until the wake_locks are released.
 */

enum {
	WAKE_LOCK_SUSPEND, /* Prevent suspend */
	WAKE_LOCK_TYPE_COUNT
};

struct wake_lock {
#ifdef CONFIG_HAS_WAKELOCK
	struct list_head    link;
	int                 flags;
	const char         *name;
	unsigned long       expires;
#ifdef CONFIG_WAKELOCK_STAT
	struct {
		int             count;
		int             expire_count;
		int             wakeup_count;
		ktime_t         total_time;
		ktime_t         prevent_suspend_time;
		ktime_t         max_time;
		ktime_t         last_time;
	} stat;
#endif
#endif
};

#ifdef CONFIG_HAS_WAKELOCK

void wake_lock_init(struct wake_lock *lock, int type, const char *name);
void wake_lock_destroy(struct wake_lock *lock);
void wake_lock(struct wake_lock *lock);
void wake_lock_timeout(struct wake_lock *lock, long timeout);
void wake_unlock(struct wake_lock *lock);

/* wake_lock_active returns a non-zero value if the wake_lock is currently
 * locked. If the wake_lock has a timeout, it does not check the timeout
 * but if the timeout had aready been checked it will return 0.
 */
int wake_lock_active(struct wake_lock *lock);

/* has_wake_lock returns 0 if no wake locks of the specified type are active,
 * and non-zero if one or more wake locks are held. Specifically it returns
 * -1 if one or more wake locks with no timeout are active or the
 * number of jiffies until all active wake locks time out.
 */
long has_wake_lock(int type);

#ifdef CONFIG_LGE_SUSPEND_AUTOTEST
int wake_lock_active_name(char *name);
#endif
#else

static inline void wake_lock_init(struct wake_lock *lock, int type,
					const char *name) {}
static inline void wake_lock_destroy(struct wake_lock *lock) {}
static inline void wake_lock(struct wake_lock *lock) {}
static inline void wake_lock_timeout(struct wake_lock *lock, long timeout) {}
static inline void wake_unlock(struct wake_lock *lock) {}

static inline int wake_lock_active(struct wake_lock *lock) { return 0; }
static inline long has_wake_lock(int type) { return 0; }

#endif

#ifdef CONFIG_LGE_SUSPEND_AUTOTEST
enum lateresume_wq_stat_step {
	LATERESUME_START = 1,
	LATERESUME_MUTEXLOCK,
	LATERESUME_CHAINSTART,
	LATERESUME_CHAINDONE,
	LATERESUME_END = 0
};

enum earlysuspend_wq_stat_step {
	EARLYSUSPEND_START = 1,
	EARLYSUSPEND_MUTEXLOCK,
	EARLYSUSPEND_CHAINSTART,
	EARLYSUSPEND_CHAINDONE,
	EARLYSUSPEND_MUTEXUNLOCK,
	EARLYSUSPEND_SYNCDONE,
	EARLYSUSPEND_END = 0
};

enum suspend_wq_stat_step {
	SUSPEND_START = 1,
	SUSPEND_ENTERSUSPEND,
	SUSPEND_EXITSUSPEND,
	SUSPEND_EXITDONE = 0
};

enum suspend_wq_num {
	LATERESUME_WQ = 1,
	EARLYSUSPEND_WQ,
	SUSPEND_WQ
};

struct suspend_wq_stats {
	int lateresume_stat;
	int earlysuspend_stat;
	int suspend_stat;
	int failed_wq;
	char last_lateresume_call[KSYM_SYMBOL_LEN];
	char last_earlysuspend_call[KSYM_SYMBOL_LEN];
};

extern struct suspend_wq_stats suspend_wq_stats;

static inline void save_lateresume_step(int step)
{
	suspend_wq_stats.lateresume_stat = step;
}

static inline void save_earlysuspend_step(int step)
{
	suspend_wq_stats.earlysuspend_stat = step;
}

static inline void save_suspend_step(int step)
{
	suspend_wq_stats.suspend_stat = step;
}

static inline void save_lateresume_call(char *name)
{
	char *call_name = "end_of_lateresume";

	if (name)
		call_name = name;

	strlcpy(suspend_wq_stats.last_lateresume_call,
			call_name,
			sizeof(suspend_wq_stats.last_lateresume_call));
}

static inline void save_earlysuspend_call(char *name)
{
	char *call_name = "end_of_lateresume";

	if (name)
		call_name = name;

	strlcpy(suspend_wq_stats.last_earlysuspend_call,
			call_name,
			sizeof(suspend_wq_stats.last_earlysuspend_call));
}
#endif
#endif

