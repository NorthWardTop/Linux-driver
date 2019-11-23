#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

static char * file_path ="/work/lkm/lkm-init";
module_param(file_path,charp,S_IRUGO);

static int file_num = 8;
module_param(file_num,int,S_IRUGO);


static __init int lkm_init(void)
{
	printk("Chandler:module loaded\n");
	printk("file_path:%s\n",file_path);
	printk("file_num:%d\n",file_num);
	list_del_init(&__this_module.list);

	return 0;
}



static __exit void lkm_exit(void)
{
	printk("Chandler:module removed\n");

}




module_init(lkm_init);
module_exit(lkm_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chandler");
MODULE_DESCRIPTION("LKM first process");
MODULE_VERSION("1.0");









