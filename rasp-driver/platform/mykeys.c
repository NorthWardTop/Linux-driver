/*
 * Driver for keys on GPIO lines capable of generating interrupts.
 *
 * Copyright 2005 Phil Blundell
 * Copyright 2010, 2011 David Jander <david@protonic.nl>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
// #include <linux/irqreturn.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/slab.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/gpio_keys.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/spinlock.h>



struct gpio_button_data {
	const struct gpio_keys_button *button;
	struct input_dev *input;

	struct timer_list release_timer;
	unsigned int release_delay;	/* in msecs, for IRQ-only buttons */

	struct delayed_work work;
	unsigned int software_debounce;	/* in msecs, for GPIO-driven buttons */

	unsigned int irq;
	spinlock_t lock;
	bool disabled;
	bool key_pressed;
};

struct gpio_keys_drvdata {
	const struct gpio_keys_platform_data *pdata;
	struct input_dev *input;
	struct mutex disable_lock;
	struct gpio_button_data data[0];
};


static int gpio_keys_probe(struct platform_device *pdev)
{
	// 先由plat获取设备
	struct device *dev = &pdev->dev;
	// 然后从设备获取驱动私有数据
	const struct gpio_keys_platform_data *pdata = dev_get_drvdata(dev); 
	struct gpio_keys_drvdata *ddata;
	struct input_dev *input;
	size_t size;
	int i, error;
	int wakeup = 0;

	// 若没有从dev获取到pdata, 则从设备树获取
	if (!pdata) {
		pdata = gpio_keys_get_devtree_pdata(dev);
		if (IS_ERR(pdata))
			return PTR_ERR(pdata);
	}
	// 申请dev data的大小, 包括按键驱动的数据和每个按键的数据*按键数目
	// 为dev申请一个被管理的内存
	size = sizeof(struct gpio_keys_drvdata) + 
		pdata->nbuttons * sizeof(struct gpio_button_data);
	ddata = devm_kzalloc(dev, size, GFP_KERNEL);
	if (!ddata) {
		dev_err(dev, "failed to allocate input device\n");
		return -ENOMEM;
	}
	// 分配一个被管理的输入设备
	input = devm_input_allocate_device(dev);
	if (!input) {
		dev_err(dev, "failed to allocate input device\n");
		return -ENOMEM;
	}

	// 申请成功后, 对对象进行设置
	ddata->pdata = pdata;
	ddata->input = input;
	mutex_init(&ddata->disable_lock);
	// 设置总线驱动数据和输入子系统驱动数据
	platform_set_drvdata(pdev, ddata);
	input_set_drvdata(input, ddata);

	input->name = pdata->name ? : pdev->name;
	input->phys = "gpio-keys/input0";
	input->dev.parent = &pdev->dev;
	input->open = gpio_keys_open;
	input->close = gpio_keys_close;

	input->id.bustype = BUS_HOST;
	input->id.vendor = 0x0001;
	input->id.product = 0x0001;
	input->id.version = 0x0100;

	// 启用Linux输入子系统的自动重复功能
	if (pdata->rep)
		__set_bit(EV_REP, input->evbit);
	
	// 初始化所有用到的gpio
	for (i = 0; i < pdata->nbuttons; i++) {
		const struct gpio_keys_button *button = &pdata->buttons[i];
		struct gpio_button_data *bdata = &ddata->data[i];

		error = gpio_keys_setup_key(pdev, input, bdata, button);
		if (error)
			return error;
		
		if (button->wakeup)
			wakeup = 1;
	}

	error = sysfs_create_group(&pdev->dev.kobj, &gpio_keys_attr_group);
	error = input_register_device(input); //注册输入设备
}


static irqretrun_t gpio_keys_irq_isr(int irq, void *dev_id)
{
	struct gpio_button_data *bdata = dev_id;
	const struct gpio_keys_button *button = bdata->button;
	struct input_dev *input = bdata->input;
	unsigned long flags;

	BUG_ON(irq != bdata->irq);
	
	// 自旋上锁
	spin_lock_irqsave(&bdata->lock, flags);
	if (!bdata->key_pressed) {
		// 如果开启了唤醒先唤醒父设备
		if (bdata->button->wakeup)
			pm_wakeup_event(bdata->input->dev.parent, 0);
		
		// 报告事件, 并同步
		input_event(input, EV_KEY, button->code, 1);
		input_sync(input);

		if (!bdata->release_delay) {
			input_event(input, EV_KEY, button->code, 0);
			goto out;
		}
		bdata->key_pressed = true;
	}

	if (bdata->release_delay)
		mod_timer(&bdata->release_timer, 
			jiffies + msecs_to_jiffies(bdata->release_delay));
	
out:
	spin_unlock_irqrestore(&bdata->lock, flags);
	return IRQ_HANDLED;
}
















