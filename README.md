# kernel-schedule

### Part 1: Implementation & Design Decisions
Given below are the steps we followed in order to implement a kernel module for Rate Monotonic Scheduling:**Step1**:​We Created a directory entry “/proc/mp2” within the Proc filesystem and created a file entry “/proc/mp2/status/” inside the directory

```/* Helper function to create the directory entries for /proc */void​create_mp2_proc_files(v​oid)​;
```**Step 2**:​Callback functions for read and write in the entry of the proc filesystem. The write function has a switch to separate each type of message (R: registration, Y: yield, D: deregistration). Here we implement three different functions to address these three cases,detailed will be showed in following steps.

```/* /proc file read opoccurs when a user runs cat /proc/mp1/statusreturns a list of PIDs and their period and computation time.*/ssize_t read_proc(s​truct​file *filp, c​har​*user, size_t count, loff_t *offset);/* /proc file write opoccurs when a user runs ./process &Input format should either be: “R PID PERIOD COMPUTATION”“Y PID” “D PID”*/ssize_t write_proc(s​truct​file *filp, c​onst​c​har​*user, size_t count, loff_t *offset);```
**Step 3**:​Declared and initialized a linked list “pid_sched_list” that contains our own PCB. Weadd additional state information of task to Linux PCB including the pid, period, computationtime, the current state of the task, a wake up timer for the task and the period start time.

```/* The linked list structure */struct​pid_sched_list {s​truct​list_head list; /​* Kernel's list structure */ s​truct t​ask_struct *linux_task;s​truct t​imer_list wakeup_timer; u​nsigned​l​ong​pid;u​nsigned​l​ong​period; u​nsigned​l​ong​computation; u​nsigned​l​ong​period_start;￼￼￼
￼task_state state; };/* Initialized the linked list */INIT_LIST_HEAD(&pid_sched_list.list);

```
**Step 4**:​Implement registration and admission control. The admission control check if the task to be admitted if the utilization of all tasks <=0.693. We initialize the task in Sleeping state and let it run until the application reaches the Yield message in real­time loop. We use the slab allocator to allocate a new instance of “pid_sched_list” in order to improve allocationperformance and reduce memory fragmentation.

```/* Helper function to register a task */void​register_handler(c​har​*buf);/* Helper function to check whether we can admit a task without missing any deadline. return 1 if successful, 0 otherwise.*/ int​task_admissible(u​nsigned​l​ong​period, u​nsigned​l​ong​computation)task_cache = kmem_cache_create("​mp2_task_cache",​s​izeof(​s​truct​pid_sched_list), 0​,​SLAB_HWCACHE_ALIGN, N​ULL)​; tmp = kmem_cache_alloc(task_cache, GFP_KERNEL);
```
**Step 5**:​Implement de­registration that remove the task from the list and free up memoryallocated during registration.

```/* Helper function to de­register a task */void​deregister_handler(c​har​*buf);
```
**Step 6a**:​Implement a kernel thread “dispatching thread” that is responsible for triggering the context switches when: 1. After receiving a Yield message; 2. After the wakeup timer of task expires. The dispatching thread will sleep the rest of the time.In the context switch, we implement the “get_next_task” function which is responsible to find the ready & highest priority task in the list. Also we use the scheduler API to handle context switches and the preemption.

```dispatch_thread = kthread_create(context_switch, N​ULL,​"​mp2_dispatch")​;/* Executes a context switch. Occurs either when wakeup timer expires or YIELD is called*/int​context_switch(v​oid​*data);/* Finds the highest priority ready task in the list */ struct​pid_sched_list *get_next_task(v​oid)​;// Sleep the dispatch threadset_current_state(TASK_UNINTERRUPTIBLE); schedule();
```
**Step 6b**: Implement the wake up timer handler which change the state of task to READY andshould wake up the dispatching thread.	/* Sets task to ready (callback for when wakeup timer expires) */	void​ready_task(u​nsigned​l​ong​data)

￼**Step 7**: Implement yield. In the function we put the task to sleep asTASK_UNINTERRUPTIBLE and set up the wake up timer.```/* Helper function to yield a task */void​yield_handler(c​har​*buf);sleep_duration = sched_task­>period ­ (running_duration % sched_task­>period); mod_timer(&(sched_task­>wakeup_timer), jiffies + msecs_to_jiffies(sleep_duration));
```
**Step 8**:​Implement the test application userapp.c that can take period and number of jobs of the application as command line parameter. Here we use fibonacci function since it takes longer to finish.We are launching multiple instances of our application through a shell script (LaunchMutipleApps.sh).**Step 9**: M​emory leak checks:​Our kernel module ensures that the resources that were allocated during its execution are freed which includes freeing allocated memory, stopping pending work function, destroying timers, slab allocator, dispatch thread and proc filesystem entry.

### Part 2 (1): Details of how to run the program
Given are the steps we followed to run our program: 

**Step 1:**

```run “make”sudo insmod mp2.ko­install the module```
**Step 2:**```sh LaunchMultipleApps.shdmesg­run the user space program “userapp” that registers itself into the module```￼