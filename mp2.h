#ifndef __MP2__
#define __MP2__

/* Scheduling States */
typedef enum {
   READY,
   RUNNING,
   SLEEPING
} task_state;

/* The linked list structure */
struct pid_sched_list {
   struct list_head list; /* Kernel's list structure */
   struct task_struct* linux_task;
   struct timer_list wakeup_timer;
   unsigned long pid;
   unsigned long period;
   unsigned long computation;
   task_state state;
};

/* Helper function to get a pcb entry from a task */
struct pid_sched_list* get_pcb_from_task(struct task_struct *task);

/* Sets task to ready (callback for when wakeup timer expires)  */
void ready_task(unsigned long data);

/* Executes a context switch */
int context_switch(void *data);

/* Helper function to see if a task will not cause deadline misses */
int task_admissible(unsigned long period, unsigned long computation);

/* Helper function to find the next task the dispatch thread should execute */
struct task_struct *get_next_task(void);

/* Helper function to delete the pid RMS list */
void delete_pid_sched_list(void);

/* Helper function to create the directory entries for /proc */
void create_mp2_proc_files(void);

/* Helper function to delete the entries for /proc */
void delete_mp2_proc_files(void);

/* /proc file read op */
ssize_t read_proc(struct file *filp, char *user, size_t count, loff_t *offset);

/* /proc file write op */
ssize_t write_proc(struct file *filp, const char *user, size_t count, loff_t *offset);

/* Called when module is loaded */
int __init mp2_init(void);

/* Called when module is unloaded */
void __exit mp2_exit(void);



#endif
