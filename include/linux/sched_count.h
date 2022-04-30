#ifndef _LINUX_SCHED_COUNT_H
#define _LINUX_SCHED_COUNT_H

#include <linux/fs.h>

int proc_sched_count_show(struct seq_file *m, struct pid_namespace *ns,
		     struct pid *pid, struct task_struct *tsk);


#endif /* _LINUX_SCHED_COUNT_H */
