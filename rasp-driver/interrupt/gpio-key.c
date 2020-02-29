
#include <linux/gpio.h>
#include <linux/wait.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <stdbool.h>


struct gpio_button_data {
        const struct gpio_keys_button *button;
        struct input_dev *input;
        struct timer_list timer;
        struct work_struct work;
        unsigned int timer_debounce;    /* in msecs */
        unsigned int irq;
        spinlock_t lock;
        bool disabled;
        bool key_pressed;
};


// 中断申请
static int gpiokey_setup(struct platform_device *pdev,
			struct input_dev *input,
			struct gpio_button_data *bdata,
			const struct gpio_keys_button *button)
{
	//由gpio控制器设备 上级的中断判断使用request_irq还是request_threaded_irq
	int err = request_any_context_irq(bdata->irq, isr, irqflags, desc, bdata);
	if (err < 0) {
		dev_err(dev, "Unable to claim irq %d, error %d\n", 
		bdata->irq, err);
		goto fail;
	}
fail:
	return -1;
}


// 中断释放
static void gpiokey_remove(struct gpio_button_data *bdata)
{
	free_irq(bdata->irq, bdata);
	if (bdata->timer_debounce)
		del_timer_sync(&bdata->timer);
	cancel_work_sync(&bdata->work);
	if (gpio_is_valid(bdata->button->gpio))
		gpio_free(bdata->button->gpio);
}



// 顶部
static irqreturn_t gpiokey_isr(int irq, void *dev_id)
{
	struct gpio_button_data *bdata = dev_id;

	BUG_NO(irq != bdata->irq);

	if (bdata->button->wakeup)
		pm_stay_awake(bdata->input->dev.parent);
	
	if (bdata->timer_debounce)
		mod_timer(&bdata->timer, 
		jiffies + msecs_to_jiffies(bdata->timer_debounce));
	else 
		schedule_work(&bdata->work);
	return IRQ_HANDLED;
}


// 底部
static void gpiokey_work_func(struct work_struct *work)
{
	struct gpio_button_data *bdata = 
		container_of(work, struct gpio_button_data, work);

	gpio_keys_gpio_report_event(bdata);

	if (bdata->button->wakeup)
		pm_relax(bdata->input->dev.parent);
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
