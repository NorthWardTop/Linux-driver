/**
 * 在中断到来时，会遍历执行共享此中断的所有中断处理程序，直到某一个函数返回IRQ_HANDLED。
 * 在中断处理程序顶半部中，应根据硬件寄存器中的信息比照传入的dev_id参数迅速地判断
 * 是否为本设备的中断，若不是，应迅速返回IRQ_NONE
 */


#include <linux/interrupt.h>

// 顶部
irqreturn_t xxx_interrupt(int irq, void *dev_id)
{
	int status = read_int_status();
	if (!is_myint(dev_id, status))
		return IRQ_NONE;
	
	//本设备中断, 进行处理
	pass;
	return IRQ_HANDLED;
}





static int __init xxx_init(void)
{
	//申请中断
	int result = request_irq(xxx_irq, xxx_interrupt, 
		IRQF_SHARED, "xxx", NULL);

	return IRQ_HANDLED;
}


static void __exit xxx_exit(void)
{
	free_irq(xxx_irq, xxx_interrupt);
}
