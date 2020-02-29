#include <linux/wait.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>


//定义工作队列和执行函数
struct work_struct xxx_wq;
void xxx_do_work(struct work_struct *work);

// 底部
void xxx_do_work(struct work_struct *work)
{
	//底部内容
	pass;
}

// 顶部
irqreturn_t xxx_interrupt(int irq, void *dev_id)
{
	//顶部内容
	pass;
	schedule_work(&xxx_wq);
	return IRQ_HANDLED;
}


static int __init xxx_init(void)
{
	//申请中断
	int result = request_irq(xxx_irq, xxx_interrupt, 0, "xxx", NULL);
	INIT_WORK(&xxx_wq, xxx_do_work);

	return IRQ_HANDLED;
}


static void __exit xxx_exit(void)
{
	free_irq(xxx_irq, xxx_interrupt);
}
