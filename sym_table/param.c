/*
 * @Author: northward
 * @Github: https://github.com/northwardtop
 * @LastEditors: northward
 * @Description: 将当前模块部分符号加入内核符号表
 * 使用
 * 
 * @Date: 2019-04-28 15:15:09
 * @LastEditTime: 2019-04-28 21:53:10
 */
#include <linux/module.h>
#include <linux/kernel.h>

int my_add(int a, int b)
{
	return a+b;
}

//导出到内核符号表
EXPORT_SYMBOL_GPL(my_add);

static int baud = 115200;
static int port[4] = {0,1,2,3};
static char* name="vcom";


module_param(baud, int, 0644);
module_param_array(port, int, NULL, 0644);
module_param(name, charp, 0644);


static int __init led_init(void)
{
	//while(1);
	printk("GEC6818 LED installed, (c) 2019 SLsiAP\n");
	printk("baud = %d\n", baud);
	printk("port = %d,%d,%d,%d\n", port[0],port[1],port[2],port[3]);
	printk("name = %s\n", name);
	return 0;
}

static void __exit led_exit(void)
{
	printk("GEC6818 LED removed, (c) 2019 SLsiAP\n");
}

module_init(led_init);
module_exit(led_exit);

MODULE_AUTHOR("yonghuilee <yonghuilee.cn@gmail.com>");
MODULE_DESCRIPTION("GEC6818 LED Device Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("v1.0");












