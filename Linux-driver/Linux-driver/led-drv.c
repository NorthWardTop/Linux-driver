/*
 * @Author: northward
 * @Github: https://github.com/northwardtop
 * @LastEditors: northward
 * @Description: insmod��rmmod�Զ������������豸�ڵ�
 * "/dev/LED", "/sys/class/LED"
 * @Date: 2019-04-29 19:35:08
 * @LastEditTime: 2019-04-29 22:04:54
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/device.h>



#define CLASS_NAME		"LED"
#define DEVICE_NAME		"LED0"


static struct cdev gec6818_led_cdev;
static struct class *led_class;
static struct device *led_device;
static dev_t led_num = 0;


/**
 * @description: ��Ӧ�õ���write����ʱ��, ��buf����д���ں˿ռ�
 * 		copy_from_user(kbuf,buf,len);
 * @param {type}
 * @return:
 */
static ssize_t gec6818_led_write(struct file *fl, const char __user *buf, size_t len, loff_t *off)
{
	int ret = -1;
	char kbuf[64] = {};
	printk("LED device writing...\n");
	if (len > sizeof kbuf) //����ں˿ռ�Сװ�����û��ռ�����򱨴�
		return -EINVAL;//����
	ret = copy_from_user(kbuf, buf, len);//����cp�ֽ���
	if (ret < 0)
	{
		printk("copy_from_user error\n");
		return -1;
	}
	printk("user say: %s\n", kbuf);

	return len - ret;
}

/**
 * @description: read����, ���ݴ��ں˵�Ӧ�ò�, copy_to_user();
 * @param {type}
 * @return:
 */
static ssize_t gec6818_led_read(struct file *fl, char __user *buf, size_t len, loff_t *offs)
{
	int ret = -1;
	char kbuf[64] = "I am from kernel data\n";
	printk("LED device reading...\n");
	if (len > sizeof kbuf) //����ں˿ռ�Сװ�����û��ռ�����򱨴�
		return -EINVAL;//����
	ret = copy_to_user(buf, kbuf, len);
	if (ret < 0)
	{
		printk("copy_to_user error\n");
		return -1;
	}

	return len - ret;
}

//��
static int gec6818_led_open(struct inode *i, struct file *f)
{
	printk("LED device opened!\n");
	return 0;
}


//�ر�
static int gec6818_led_release(struct inode *i, struct file *f)
{
	printk("LED closed\n");
	return 0;
}

static const struct file_operations gec6818_led_fops = {
	.owner = THIS_MODULE,
	.write = gec6818_led_write,
	.read = gec6818_led_read,
	.open = gec6818_led_open,
	.release = gec6818_led_release
};



static int __init gec6818_led_init(void)
{
	printk("GEC6818 LED installed, (c) 2019 SLsiAP\n");
	//led_num=MKDEV(239, 0); //20λ���豸��,12λ���豸��,�����豸��
	//int rt=register_chrdev_region(led_num, 1, DEVICE_NAME);//ע���豸��, ����, ����
	//��ֹͬ���豸��, ��̬����
	int ret = alloc_chrdev_region(&led_num, 0, 1, DEVICE_NAME);
	printk("led_major:%d,led_minor:%d\n", MAJOR(led_num), MINOR(led_num));

	if (ret < 0)
	{
		printk("register_chrdev_region fail\n");
		return ret;
	}

	//�ַ��豸��ʼ��
	cdev_init(&gec6818_led_cdev, &gec6818_led_fops);
	//�����ں�
	ret = cdev_add(&gec6818_led_cdev, led_num, 1);
	if (ret < 0)
	{
		printk("cdev_add failed!\n");
		goto cdev_add_failed;
	}

	//��sys/classĿ¼�����ļ���gec6818_leds
	//THIS_MODULE:class �������ߣ�Ĭ��д THIS_MODULE
	//class_led:�Զ��� class �����֣�����ʾ��/sys/class Ŀ¼����
	led_class = class_create(THIS_MODULE, CLASS_NAME);//������
	if (IS_ERR(led_class))
	{
		ret = PTR_ERR(led_class);
		printk("class_create failed\n");
		goto class_create_failed;
	}
	//�����豸 
	/*
		struct  device  *device_create(struct  class  *class,  struct  device  *parent,
		dev_t  devt,  void  *drvdata,  const  char  *fmt,  ...)
		class������ device �������ĸ���
		parent��Ĭ��Ϊ NULL
		devt���豸��,�豸�ű�����ȷ����Ϊ�����������/dev Ŀ¼�°������Զ������豸�ļ�
		drvdata��Ĭ��Ϊ NULL
		fmt���豸�����֣���������ɹ����Ϳ�����/dev Ŀ¼�������豸������ */
	led_device = device_create(led_class, NULL, led_num, NULL, DEVICE_NAME);
	if (IS_ERR(led_device))
	{
		ret = PTR_ERR(led_device);
		printk("device_create failed\n");
		goto device_create_failed;
	}
	printk("LED device initialization complete\n");

	return 0;

device_create_failed:
	class_destroy(led_class);

class_create_failed:
	cdev_del(&gec6818_led_cdev);

cdev_add_failed:
	unregister_chrdev_region(led_num, 1);

	return ret;

}

static void __exit gec6818_led_exit(void)
{
	//�����豸
	device_destroy(led_class, led_num);
	//������
	class_destroy(led_class);
	//�ں���ɾ���ַ��豸
	cdev_del(&gec6818_led_cdev);
	//ע���ַ��豸��
	unregister_chrdev_region(led_num, 1);
	printk("GEC6818 LED removed, (c) 2019 SLsiAP\n");
}

module_init(gec6818_led_init);
module_exit(gec6818_led_exit);

MODULE_AUTHOR("yonghuilee <yonghuilee.cn@gmail.com>");
MODULE_DESCRIPTION("GEC6818 LED Device Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("v1.0");

