#include <linux/sched_count.h>
#include <linux/seq_file.h>

int proc_sched_count_show(struct seq_file *m, struct pid_namespace *ns,
             struct pid *pid, struct task_struct *tsk) {
  seq_printf(m, "%u\n", tsk->sched_count);
  return 0;
}

