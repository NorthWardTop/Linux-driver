
/**
 * 无法分类的设备，杂项设备 msicdevice
 */
struct miscdevice {
	int minor;
	const char *name;
	const struct file_operations *fops;
	struct list_head list;
	
	struct device *parent;
	struct device *this_device;

	const char *nodename;
	umode_t mode;
}


////////////////misc驱动结构一般如下

struct xxx_device {
	unsigned int version;
	unsigned int size;
	spinlock_t lock;
	pass;
	struct miscdevice miscdev;
}


static long xxx_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct xxx_dev *xxx = container_of(file->private_data, struct xxx_dev, 
		miscdev);
	pass;
}


static const struct file_operations xxx_fops = {
	.unlocked_ioctl = xxx_ioctl,
	.mmap		= xxx_mmap,
};

static struct miscdevice xxx_miscdev = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "xxx",
	.fops	= &xxx_fops
};

static struct xxx_device xxx_dev = {
	.miscdev = xxx_miscdev
}

static int __init xxx_init(void)
{
	pr_info("ARC Hostlink driver mmap at 0x%p\n", __HOSTLINK__);
	return misc_register(&xxx_dev.xxx_miscdev);
}

