#include <linux/jiffies.h>
#include <linux/timer.h>
#include <linux/sched.h>


//////////////////////////////////////////////////////////////////////忙等待
void ndelay(unsigned long nsecs);
void udelay(unsigned long usecs);
void mdelay(unsigned long msecs);

// msleep（）和msleep_interruptible（）在本质上都是依靠包含了schedule_timeout（）
// 的schedule_timeout_uninterruptible（）和schedule_timeout_interruptible（）来实现的
void ssleep(unsigned int seconds);
void msleep(unsigned int millisecs);
unsigned long msleep_interruptible(unsigned int millisecs);


/**
 * 延迟100个 jiffies
 */
unsigned long delay = jiffies + 100;
while(time_before(jiffies, delay));


/**
 * 延迟2秒
 */
unsigned long delay2 = jiffies + 2*HZ;
while (time_before(jiffies, delay2));



/////////////////////////////////////////////////////////////////////睡着等待


signed long __sched schedule_timeout_interruptible(signed long timeout)
{
	set_current_state(TASK_INTERRUPTIBLE);
	return schedule_timeout(timeout);
}


signed long __sched schedule_timeout_uninterruptible(signed long timeout)
{
	set_current_state(TASK_UNINTERRUPTIBLE);
	return schedule_timeout(timeout);
}

// 等待队列上等待
sleep_on_timeout(wait_queue_head_t *q, unsigned long timeout);
interruptible_sleep_on_timeout(wait_queue_head_t*q, unsigned long timeout);


