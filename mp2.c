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
static unsigned long lock_flag;

/* Cache for mp2_task_struct slab allocation */
static struct kmem_cache *task_cache;

/* Dispatch thread */
static struct task_struct *dispatch_thread;
static int thread_flag;

/* Available file operations for mp2/status */
struct file_operations proc_fops = {
   read: read_proc,
   write: write_proc
};

/* Helper function to delete the linked list */
void delete_pid_sched_list(void) {
   list_for_each_safe(head, next, &pid_sched_list.list) {
      tmp = list_entry(head, struct pid_sched_list, list);
      del_timer(&tmp->wakeup_timer);
      list_del(head);
      kmem_cache_free(task_cache, tmp);
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

   copy_from_user(buf, user, count);
   type = buf[0];

   switch(type)
   {
      case 'R':
         register_handler(buf);
         break;
      case 'Y':
         yield_handler(buf);
         break;
      case 'D':
         deregister_handler(buf);
         break;
   }

   kfree((void *)buf);
   return count;
}


void register_handler(char *buf){
   struct pid_sched_list *temp_task;
   temp_task = kmem_cache_alloc(task_cache, GFP_KERNEL);
   sscanf(&buf[1], "%lu %lu %lu", &temp_task->pid, &temp_task->period, &temp_task->computation);
   temp_task->linux_task = find_task_by_pid(temp_task->pid);
   setup_timer(&temp_task->wakeup_timer, ready_task, (unsigned long)temp_task);
   temp_task->state = SLEEPING;
   temp_task->period_start = 0;

   if(task_admissible(temp_task->period, temp_task->computation)) {
      /* Add a task entry */
      spin_lock_irqsave(&list_lock, lock_flag);
      list_add(&(temp_task->list), &(pid_sched_list.list));
      spin_unlock_irqrestore(&list_lock, lock_flag);
      #ifdef DEBUG
      printk("PROCESS REGISTERED: %lu %lu %lu\n", temp_task->pid, temp_task->period, temp_task->computation);
      #endif
   } else {
      del_timer(&temp_task->wakeup_timer);
      kmem_cache_free(task_cache, temp_task);
      #ifdef DEBUG
      printk("PROCESS NOT SCHEDUABLE: %lu %lu %lu\n", temp_task->pid, temp_task->period, temp_task->computation);
      #endif
   }
}

/* Helper function to yield a task */
void yield_handler(char *buf){
   unsigned long pid;
   unsigned long running_duration, sleep_duration;
   struct pid_sched_list *sched_task;

   sscanf(&buf[1], "%lu", &pid);
   sched_task = get_pcb_from_pid(pid);

   // calculate release time
   if (sched_task->period_start == 0){
       running_duration = 0;
   }
   else{
       running_duration = (jiffies_to_msecs(jiffies) - sched_task->period_start);
   }

   sleep_duration = sched_task->period - running_duration;
   if (sleep_duration > 0){
      // Set the task to Sleep
      mod_timer(&(sched_task->wakeup_timer), jiffies + msecs_to_jiffies(sleep_duration));

      spin_lock_irqsave(&list_lock, lock_flag);
      sched_task->state = SLEEPING;
      spin_unlock_irqrestore(&list_lock, lock_flag);
      set_task_state(sched_task->linux_task, TASK_UNINTERRUPTIBLE);
      schedule();
      if (sched_task->pid == mp2_current_task->pid){
         mp2_current_task = NULL;
      }
   }

   wake_up_process(dispatch_thread);
   
   #ifdef DEBUG
   printk("PROCESS YIELDED: %lu\n", pid);
   printk("Sleep duration: %lu\n",sleep_duration);
   #endif
}

/* Helper function to deregister a task */
void deregister_handler(char *buf){
   unsigned long pid;

   sscanf(&buf[1], "%lu", &pid);

   spin_lock_irqsave(&list_lock, lock_flag);
   list_for_each_safe(head, next, &pid_sched_list.list) {
      tmp = list_entry(head, struct pid_sched_list, list);
      if(tmp->pid == pid) {
         del_timer(&tmp->wakeup_timer);
         if (tmp->pid == mp2_current_task->pid){
            mp2_current_task = NULL;
         }
         list_del(head);
         kmem_cache_free(task_cache, tmp);
         break;
      }
   }
   spin_unlock_irqrestore(&list_lock, lock_flag);
   #ifdef DEBUG
   printk("PROCESS DEREGISTERED: %lu\n", pid);
   #endif
}


/* Sets task to ready (callback for when wakeup timer expires)  */
void ready_task(unsigned long data)
{
   struct pid_sched_list *exp_task = (struct pid_sched_list *)data;
   spin_lock_irqsave(&list_lock, lock_flag);
   exp_task->state = READY;
   exp_task->period_start = jiffies_to_msecs(jiffies);
   spin_unlock_irqrestore(&list_lock, lock_flag);

   #ifdef DEBUG
   printk("Timer\n" );
   #endif
   wake_up_process(dispatch_thread);

}

/* Executes a context switch.  Occurs either when
   wakeup timer expires or YIELD is called
   TODO: Set task states to READY, SLEEPING OR RUNNING
*/
int context_switch(void *data)
{
   struct sched_param sparam;

   struct pid_sched_list *new_task;
   struct pid_sched_list *old_task;

   while(thread_flag){

      #ifdef DEBUG
      printk("Context Switching...\n");
      #endif   

      // Preempt the old task
      new_task = get_next_task();
      if (mp2_current_task != NULL){
         old_task = get_pcb_from_pid(mp2_current_task->pid);
         sparam.sched_priority=0;
         sched_setscheduler(mp2_current_task, SCHED_NORMAL, &sparam);
      }
      else{
         old_task = NULL;
      }  
   

      if (new_task != NULL){
         if(old_task != NULL && new_task->period < old_task->period
          && old_task->state == RUNNING) {
            spin_lock_irqsave(&list_lock, lock_flag);
            old_task->state = READY;
            spin_unlock_irqrestore(&list_lock, lock_flag);
            
         }  

         if (old_task == NULL || new_task->period < old_task->period){
               spin_lock_irqsave(&list_lock, lock_flag);
               new_task->state = RUNNING;
               spin_unlock_irqrestore(&list_lock, lock_flag);
               
               wake_up_process(new_task->linux_task);
               sparam.sched_priority=99;
               sched_setscheduler(new_task->linux_task, SCHED_FIFO, &sparam);
               mp2_current_task = new_task->linux_task;
         }
      }  

      #ifdef DEBUG
      printk("Finish Context Switching...\n");
      #endif
      // Sleep the dispatch thread
      set_current_state(TASK_INTERRUPTIBLE);
      schedule();
   }
   return 0;
}

struct pid_sched_list* get_pcb_from_pid(unsigned int pid) {
   list_for_each(head, &pid_sched_list.list) {
      tmp = list_entry(head, struct pid_sched_list, list);
      if(tmp->pid == pid) {
         return tmp;
      }
   }
   return NULL;
}


int task_admissible(unsigned long period, unsigned long computation) {
   unsigned long util = 0;

   list_for_each(head, &pid_sched_list.list) {
      tmp = list_entry(head, struct pid_sched_list, list);
      util += (10000 * tmp->computation) / tmp->period;
   }

   util += (10000 * computation) / period;

   return util <= 6930;
}

/* Finds the highest priority ready task in the list */
struct pid_sched_list *get_next_task(void) {
   struct pid_sched_list *sched_task = NULL;

   list_for_each(head, &pid_sched_list.list) {
      tmp = list_entry(head, struct pid_sched_list, list);
      if((tmp->state == READY) && (sched_task == NULL || tmp->period < sched_task->period)) {
         sched_task = tmp;
      }
   }

   return sched_task;
}

/* Called when module is loaded */
int __init mp2_init(void)
{
   #ifdef DEBUG
   printk(KERN_ALERT "MP2 MODULE LOADING\n");
   #endif

   mp2_current_task = NULL;

   INIT_LIST_HEAD(&pid_sched_list.list);
   create_mp2_proc_files();
   task_cache = kmem_cache_create("mp2_task_cache",
                                  sizeof(struct pid_sched_list),
                                  0, SLAB_HWCACHE_ALIGN,
                                  NULL);

   dispatch_thread = kthread_create(context_switch, NULL, "mp2_dispatch");
   thread_flag = 1;
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

   thread_flag = 0;
   wake_up_process(dispatch_thread);
   kthread_stop(dispatch_thread);
   kmem_cache_destroy(task_cache);
   delete_mp2_proc_files();
   delete_pid_sched_list();

   printk(KERN_ALERT "MP2 MODULE UNLOADED\n");
}


// Register init and exit funtions
module_init(mp2_init);
module_exit(mp2_exit);
