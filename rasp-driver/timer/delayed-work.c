
// 对于周期性的任务，除了定时器以外，在Linux内核中还可以利用一套封装得很好的快捷机制，其本质是利用工作队列和定时器实现，这套快捷机制就是delayed_work，delayed_work结构体的定义如代码清单10.12所示。

// 代码清单10.12　delayed_work结构体

// 1struct delayed_work {
// 2        struct work_struct work;
// 3        struct timer_list timer;
// 4
// 5        /* target workqueue and CPU ->timer uses to queue ->work */
// 6        struct workqueue_struct *wq;
// 7        int cpu;
// 8};
// 我们可以通过如下函数调度一个delayed_work在指定的延时后执行：

// int schedule_delayed_work(struct delayed_work *work, unsigned long delay);
// 当指定的delay到来时，delayed_work结构体中的work成员work_func_t类型成员func（）会被执行。work_func_t类型定义为：

// typedef void (*work_func_t)(struct work_struct *work);
// 其中，delay参数的单位是jiffies，因此一种常见的用法如下：

// schedule_delayed_work(&work, msecs_to_jiffies(poll_interval));
// msecs_to_jiffies（）用于将毫秒转化为jiffies。

// 如果要周期性地执行任务，通常会在delayed_work的工作函数中再次调用schedule_delayed_work（），周而复始。

// 如下函数用来取消delayed_work：

// int cancel_delayed_work(struct delayed_work *work);
// int cancel_delayed_work_sync(struct delayed_work *work);

