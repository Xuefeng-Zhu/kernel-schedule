#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/kthread.h>

#include "mp2_given.h"
#include "mp2.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group_09");
MODULE_DESCRIPTION("CS-423 MP2");

#define DEBUG 1
#define GLOBAL_RW_PERM 0666
#define DIR_NAME "mp2"
#define FILE_NAME "status"

/* The proc directory entry */
static struct proc_dir_entry *pdir_mp2;

/* A list holding the RMS parameters for each task */
static struct pid_sched_list pid_sched_list;

/* The currently running task */
static struct task_struct *mp2_current_task;

/* Used for creating and traversing the linked list */
static struct list_head *head, *next;
static struct pid_sched_list *tmp;

/* Spinlock to protect the linked list */
static spinlock_t list_lock;

/* Cache for mp2_task_struct slab allocation */
static struct kmem_cache *task_cache;

/* Dispatch thread */
static struct task_struct *dispatch_thread;

/* Available file operations for mp2/status */
struct file_operations proc_fops = {
   read: read_proc,
   write: write_proc
};

/* Helper function to delete the linked list */
void delete_pid_sched_list(void) {
   list_for_each_safe(head, next, &pid_sched_list.list) {
      tmp = list_entry(head, struct pid_sched_list, list);
      list_del(head);
      kfree(tmp);
   }   
}

/* Helper function to create the directory entries for /proc */
void create_mp2_proc_files(void) {
   pdir_mp2 = proc_mkdir(DIR_NAME, NULL);
   proc_create(FILE_NAME, GLOBAL_RW_PERM, pdir_mp2, &proc_fops);
}

/* Helper function to delete the directory entries for /proc */
void delete_mp2_proc_files(void) {
   remove_proc_entry(FILE_NAME, pdir_mp2);
   remove_proc_entry(DIR_NAME, NULL);   
}

/* Occurs when a user runs cat /proc/mp2/status
   Needs to return a list of PID and their period and compute time.
*/
ssize_t read_proc(struct file *filp, char *user, size_t count, loff_t *offset)
{	 
   int pos = 0;
   int len;
   char *buf = (char *)kmalloc(count, GFP_KERNEL);
   
   #ifdef DEBUG
   printk("/proc/mp2/status read\n");
   #endif

   if((int)*offset > 0) {
      kfree((void *)buf);
      return 0;
   }
   
   list_for_each(head, &pid_sched_list.list) {
      tmp = list_entry(head, struct pid_sched_list, list);
      len = sprintf(buf + pos, "PID: %lu, Period: %lu, Processing Time: %lu\n", tmp->pid, tmp->period, tmp->computation);
      pos += len;
   }

   copy_to_user(user, buf, pos);
   kfree((void *)buf);

   *offset +=pos;
   return pos;
}

/* Occurs when a user runs ./process > /proc/mp2/status 
   Input format should either be: "R PID PERIOD COMPUTATION"
                                  "Y PID"
                                  "D PID"
*/
ssize_t write_proc(struct file *filp, const char *user, size_t count, loff_t *offset)
{
   char *buf = (char *)kmalloc(count, GFP_KERNEL);
   char type;
   unsigned long pid;

   copy_from_user(buf, user, count);
   type = buf[0];

   switch(type) 
   {
      case 'R':
         tmp = kmem_cache_alloc(task_cache, GFP_KERNEL);
         sscanf(&buf[1], "%lu %lu %lu", &tmp->pid, &tmp->period, &tmp->computation);         
         tmp->linux_task = find_task_by_pid(tmp->pid);
         setup_timer(&tmp->wakeup_timer, ready_task, (unsigned long)tmp);
         tmp->state = SLEEPING; 

         if(task_admissible(tmp->period, tmp->computation)) {
            /* Add a task entry */   	 
            spin_lock(&list_lock);
            list_add(&(tmp->list), &(pid_sched_list.list));
            spin_unlock(&list_lock);   
            #ifdef DEBUG
   	    printk("PROCESS REGISTERED: %lu %lu %lu\n", tmp->pid, tmp->period, tmp->computation);
   	    #endif  
         } else {
            #ifdef DEBUG
   	    printk("PROCESS NOT SCHEDUABLE: %lu %lu %lu\n", tmp->pid, tmp->period, tmp->computation);
   	    #endif  
         }
         break;
      case 'Y':
         /* Switch context */
         sscanf(&buf[1], "%lu", &pid);
         context_switch(NULL);
         #ifdef DEBUG
   	 printk("PROCESS YIELDED: %lu\n", pid);
   	 #endif  
         break;
      case 'D':
         sscanf(&buf[1], "%lu", &pid);
         
         list_for_each_safe(head, next, &pid_sched_list.list) {
            tmp = list_entry(head, struct pid_sched_list, list);
            if(tmp->pid == pid) {
               list_del(head);
               kmem_cache_free(task_cache, tmp);
               
               #ifdef DEBUG
               printk("PROCESS DEREGISTERED: %lu\n", pid);
               #endif
            }
         }
         break;
   }

   kfree((void *)buf);
   return count;
}

/* Sets task to ready (callback for when wakeup timer expires)  */
void ready_task(unsigned long data) 
{
   struct pid_sched_list *exp_task = (struct pid_sched_list *)data;
   exp_task->state = READY;
   
   wake_up_process(dispatch_thread);

}

/* Executes a context switch.  Occurs either when 
   wakeup timer expires or YIELD is called 
   TODO: Set task states to READY, SLEEPING OR RUNNING
*/
int context_switch(void *data) 
{
   struct sched_param sparam;
   struct task_struct *new_task;

   #ifdef DEBUG
   printk("Context Switching...\n");
   #endif

   // Preempt the old task
   sparam.sched_priority=0;
   sched_setscheduler(mp2_current_task, SCHED_FIFO, &sparam);

   // Schedule the new task
   new_task = get_next_task();
   wake_up_process(new_task);
   sparam.sched_priority=99;
   sched_setscheduler(new_task, SCHED_FIFO, &sparam);
   
   // Update the current task
   mp2_current_task = new_task;

   // Sleep the dispatch thread
   set_current_state(TASK_INTERRUPTIBLE);
   schedule();

   return 0;
}

int task_admissible(unsigned long period, unsigned long computation) {
   unsigned long util = 0;

   list_for_each(head, &pid_sched_list.list) {
      tmp = list_entry(head, struct pid_sched_list, list);
      util += (10000 * tmp->computation) / tmp->period;   
   }

   return util <= 6930;
}

/* Finds the highest priority ready task in the list */
struct task_struct *get_next_task(void) {
   struct task_struct *task = NULL;
   unsigned long min_period = ULONG_MAX;

   list_for_each(head, &pid_sched_list.list) {
      tmp = list_entry(head, struct pid_sched_list, list);
      if((tmp->state == READY) && (tmp->period < min_period)) {
         task = tmp->linux_task;
      }
   }

   return task;
}

/* Called when module is loaded */
int __init mp2_init(void)
{
   #ifdef DEBUG
   printk(KERN_ALERT "MP2 MODULE LOADING\n");
   #endif

   INIT_LIST_HEAD(&pid_sched_list.list);   
   create_mp2_proc_files();
   task_cache = kmem_cache_create("mp2_task_cache",
                                  sizeof(struct pid_sched_list),
                                  0, SLAB_HWCACHE_ALIGN,
                                  NULL);

   dispatch_thread = kthread_create(context_switch, NULL, "mp2_dispatch");

   spin_lock_init(&list_lock);

   printk(KERN_ALERT "MP2 MODULE LOADED\n");
   return 0;   
}

/* Called when module is unloaded */
void __exit mp2_exit(void)
{
   #ifdef DEBUG
   printk(KERN_ALERT "MP2 MODULE UNLOADING\n");
   #endif
   
   // Cleans up the file entries in /proc and the data structures
   kthread_stop(dispatch_thread);
   kmem_cache_destroy(task_cache);
   delete_mp2_proc_files();
   delete_pid_sched_list();

   printk(KERN_ALERT "MP2 MODULE UNLOADED\n");
}


// Register init and exit funtions
module_init(mp2_init);
module_exit(mp2_exit);
