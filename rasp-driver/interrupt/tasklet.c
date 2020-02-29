/**
 * tasklet
 */


#include <linux/interrupt.h>

// 定义tasklet作为底部处理函数
void xxx_do_tasklet(unsigned long);
DECLARE_TASKLET(xxx_tasklet, xxx_do_tasklet, 0);


//底部处理内容
void xxx_do_tasklet(unsigned long)
{
	return;
}


//顶部处理内容
irqreturn_t xxx_interrupt(int irq, void *dev_id)
{
	irqreturn_t ret = IRQ_NONE;

	//调用底部,在适当的时候将执行do_tasklet
	tasklet_schedule(&xxx_tasklet);

	ret = IRQ_HANDLED;
	return ret;
}



static int __init xxx_init(void)
{
	//申请中断
	int result = request_irq(xxx_irq, xxx_interrupt, 0, "xxx", NULL);
	return IRQ_HANDLED;
}


static void __exit xxx_exit(void)
{
	free_irq(xxx_irq, xxx_interrupt);
}
