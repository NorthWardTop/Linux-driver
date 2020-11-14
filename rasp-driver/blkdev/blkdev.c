


/**
 * 块设备的I/O操作方式与字符设备的存在较大的不同，因而引入了request_queue、request、bio等一系列数据结构。在整个块设备的I/O操作中，贯穿始终的就是“请求”，字符设备的I/O操作则是直接进行不绕弯，块设备的I/O操作会排队和整合。

驱动的任务是处理请求，对请求的排队和整合由I/O调度算法解决，因此，块设备驱动的核心就是请求处理函数或“制造请求”函数。

尽管在块设备驱动中仍然存在block_device_operations结构体及其成员函数，但不再包含读写类的成员函数，而只是包含打开、释放及I/O控制等与具体读写无关的函数。

块设备驱动的结构相对复杂，但幸运的是，块设备不像字符设备那样包罗万象，它通常就是存储设备，而且驱动的主体已经由Linux内核提供，针对一个特定的硬件系统，驱动工程师所涉及的工作往往只是编写极其少量的与硬件平台相关的代码。

*/



#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/slab.h>        
#include <linux/fs.h>
#include <linux/hdreg.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/bio.h>

/**
 * size = NSECTORS*HARDSECT_SIZE;
 */
#define HARDSECT_SIZE		512	/*  */
#define NSECTORS		1024	/* sectors number */
#define NDEVICES		4	/* device number(partition) */

#define VMEM_DISK_MINORS	16
#define KERNEL_SECTOR_SIZE	512

enum {
	VMEMD_QUEUE = 0, /* Use request_queue */
	VMEMD_NOQUEUE = 1, /* Use make_request */
};

static int request_mode = VMEMD_QUEUE;
static int vmem_disk_major;

module_param(request_mode, int, 0);
module_param(vmem_disk_major, int, 0); //加载时候可选major

struct vmem_disk_dev {
	int size;		/* Device size in sectors */
	u8 *data;		/* The data array */
	spinlock_t lock;	/* For mutual exclusion */
	struct request_queue *queue;	/* The device request queue */
	struct gendisk *gd;	/* The gendisk struct */
};

static struct vmem_disk_dev *devices;



static int vmem_disk_open(struct block_device *bdev, fmode_t mode)
{
	struct vmem_disk_dev *dev = bdev->bd_disk->private_data;

	return 0;
}


static void vmem_disk_release(struct gendisk *disk, fmode_t mode)
{
	struct vmem_disk_dev *dev = disk->private_data;
}


static void vmem_disk_transfer(struct vmem_disk_dev *dev, unsigned long secor, 
	unsigned long nsect, char *buffer, int direction)
{
	unsigned long offset = sector*KERNEL_SECTOR_SIZE;
	unsigned long nbytes = nsect*KERNEL_SECTOR_SIZE;

	if (offset + nbytes > dev->size) {
		printk(KERN_NOTICE "Beyond-end pos: %ld, len: %ld", offset, nbytes);
		return;
	}

	/**
	 * Complete the final hardware I/O operation here, per sector
	 * In this example is memcpy 
	 */
	if (direction == WRITE)
		memcpy(dev->data + offset, buffer, nbytes);
	else
		memcpy(buffer, dev->data + offset, nbytes);
}

static int vmem_disk_xfer_bio(struct vmem_disk_dev *dev, struct bio *bio)
{
	struct bio_vec bvec; /* vector */
	struct bvce_iter iter;	/* each one */
	sector_t sector = bio->bi_iter.bi_sector;

	/* iterate through each segment */
	bio_for_each_segment(bvec, bio, iter) {
		char *buffer = __bio_kmap_atomic(bio, iter);
		
		vmem_disk_transfer(dev, sector, bio_cur_bytes(bio) / HARDSECT_SIZE, buffer, bio_data_dir(bio));
		sector += bio_cur_bytes(bio) / HARDSECT_SIZE;
		__bio_kunmap_atomic(buffer);
	}

	return 0;
}

/**
 * queue mode - NOQUEUE
 * vmem_disk_make_request(q)->vmem_disk_xfer_bio(bio)->vmem_disk_transfer(sector)
 * Unit is bio
 */
static void vmem_disk_make_request(struct request_queue *q, struct bio *bio)
{
	struct vmem_disk_dev *dev = q->queuedata;
	int status;

	status = vmem_disk_xfer_bio(dev, bio);
	bio_endio(bio, status);
}

/**
 * queue mode - QUEUE 
 * vmem_disk_request(q)->vmem_disk_xfer_bio(bio)->vmem_disk_transfer(sector)
 * Unit is one by one request
 */
static void vmem_disk_request(struct request_queue *q)
{
	struct request *req;
	struct bio *bio;

	/* Take out requests in order */
	while ((req = blk_peek_request(q) != NULL)) {
		struct vmem_disk_dev *dev = req->rq_disk->private_data;
		if (req->cmd_tyte != REQ_TYPE_FS) {
			printk(KERN_NOTICE "Skip non-fs request\n");
			blk_start_request(req);
			__blk_end_request_all(req, -EIO);
			continue;
		}

		blk_start_request(req);
		__rq_for_each_bio(bio, req)
			vmem_disk_xfer_bio(dev, bio);
		__blk_end_request_all(req, 0);
	}
}



static int vmem_disk_getgeo(struct block_device *bdev, struct hd_geometry *geo)
{
	long size;
	struct vmem_disk_dev *dev = bdev->bd_disk->private_data;

	size = dev->size*(HARDSECT_SIZE / KERNEL_SECTOR_SIZE);
	geo->cylinders = (size & ~0x3f) >> 6; /* What's this */
	geo->heads = 4;
	geo->sectors = 16;
	geo->start = 4;

	return 0;
}


static struct block_device_operations vmem_disk_ops = {
	.open = vmem_disk_open,
	.release = vmem_disk_release,
	.getgeo		= vmem_disk_getgeo,
}


static void setup_device(struct vmem_disk_dev *dev, int which)
{
	memset(dev, 0, sizeof(struct vmem_disk_dev));
	dev->size = NSECTORS*HARDSECT_SIZE;
	dev->data = vmalloc(dev->size); /* alloc disk space */
	
	if (dev->data == NULL) {
		printk(KERN_NOTICE "malloc falied.\n");
		return;
	}

	spin_lock_init(&dev->lock);

	/**
	 * request_mode depening on whether we are using our own queue
	 */
	switch (request_mode) {
	case VMEMD_NOQUEUE:
		dev->queue = blk_alloc_queue(GFP_KERNEL);
		if (dev->queue == NULL)
			goto queue_falied;
		
		/* binding request_make and funtion */
		blk_queue_make_request(dev->queue, vmem_disk_make_request);
		break;

	case VMEMD_QUEUE:
		/* binding request handler function */
		dev->queue = blk_init_queue(vmem_disk_request, &dev->lock);
		if (dev->queue == NULL)
			goto queue_falied;
		break;
	default:
		printk(KERN_NOTICE "Unknow request mode: %d.\n", request_mode);
	}

	blk_queue_logical_block_size(dev->queue, HARDSECT_SIZE); /* 512 per sector */
	dev->queue->queuedata = dev;

	dev->gd = alloc_disk(VMEM_DISK_MINORS); /* alloc one general disk */
	if (!dev->gd) {
		printk(KERN_NOTICE "Alloc general disk failed");
		goto alloc_disk_falied;
	}

	dev->gd->major = vmem_disk_major;
	dev->gd->first_minor = which*VMEM_DISK_MINORS;
	dev->gd->queue = dev->queue;
	dev->gd->private_data = dev;
	snprintf(dev->gd->disk_namem, 32, "vmem_disk%c", which + 'a');
	set_capacity(dev->gd, NSECTORS * (HARDSECT_SIZE / KERNEL_SECTOR_SIZE));
	add_disk(dev->gd);

	return;

alloc_disk_falied:
queue_falied:
	if (dev->data)
		vfree(dev->data);
}


static int vmem_disk_init(void)
{
	int i;

	vmem_disk_major = register_blkdev(vmem_disk_major, "vmem_disk");
	if (vmem_disk_major <= 0) {
		printk(KERN_WARNING "vmem_disk: Unbale to get major number\n");
		return -EBUSY;
	}

	devices = kmalloc(NDEVICES*sizeof(struct vmem_disk_dev), GFP_KERNEL);
	if (!devices)
		goto malloc_filed;
	
	for (i = 0; i < NDEVICES; ++i)
		setup_device(devices + i, i);  /* One by one initial device */
	

	return 0;

malloc_filed:
	unregister_blkdev(vmem_disk_major, "vmem_disk");
	return -ENOMEM;
}

static void vmem_disk_exit(void)
{
	int i;
	
	for (i = 0; i < NDECICES; ++i) {
		struct vmem_disk_dev *dev = devices + i;

		if (dev->gd) {
			del_gendisk(dev->gd);
			put_disk(dev->gd);
		}

		if (dev->queue) {
			if (request_mode == VMEMD_NOQUEUE)
				kobject_put(&dev->queue->kobj);
			else
				blk_cleanup_queue(dev->queue);
		}

		if (dev->data)
			vfree(dev->data);
	}

	unregister_blkdev(vmem_disk_major, "vmem_disk");
	kfree(devices);
}


module_init(vmem_disk_init);
module_exit(vmem_disk_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("yonghuilee.cn@gmail.com");
MODULE_DESCRIPTION("disk first process");
MODULE_VERSION("1.0");

/////////////////////////////////////////////////////////////////////////////////
// gendisk: 通用磁盘
/**
 * 分配gendisk: struct gendisk *alloc_disk(int minors);
 * 	minors为分区数量, 创建后不能修改
 * 注册gendisk: void add_disk(struct gendisk *disk);
 * 	此函数必须在驱动程序初始化完成并能够响应磁盘请求之后调用
 * disk引用计数:
 * 	struct kobject *get_disk(struct gendisk *disk)
 * 	void put_disk(struct gendisk *disk)
 * 	内核自动调用, 驱动无需调用
 */

struct block_device_operations {
	int (*open) (struct block_device *, fmode_t);
	void (*release) (struct gendisk *, fmode_t);


	int (*rw_page)(struct block_device *, sector_t, struct page *, unsigned int);

	// ioctl系统调用的实现多数为标准请求, 多数直接调用linux通用块设备层处理, 
	// 64位系统32位进行调用的是compat_ioctl
	int (*ioctl) (struct block_device *, fmode_t, unsigned, unsigned long);
	int (*compat_ioctl) (struct block_device *, fmode_t, unsigned, unsigned long);

	// 检查是否有挂起事件, 有DISK_EVENT_MEDIA_CHANGE和有DISK_EVENT_EJECT_REQUEST就返回
	unsigned int (*check_events) (struct gendisk *disk,
				      unsigned int clearing);
	/* ->media_changed() is DEPRECATED, use ->check_events() instead */
	// 已过时, 旧kernel在user space轮询可移动磁盘存在否, 新kernel在kernel sapce轮询
	// 非可以动设备无需实现
	int (*media_changed) (struct gendisk *);
	void (*unlock_native_capacity) (struct gendisk *);

	// 用于响应一个介质被改变, 使新介质准备好
	int (*revalidate_disk) (struct gendisk *);

	// 获得驱动器信息, struct hd_geometry
	int (*getgeo)(struct block_device *, struct hd_geometry *);
	/* this callback is with swap_lock and sometimes page table lock held */
	void (*swap_slot_free_notify) (struct block_device *, unsigned long);
	int (*report_zones)(struct gendisk *, sector_t sector,
			    struct blk_zone *zones, unsigned int *nr_zones,
			    gfp_t gfp_mask);
	struct module *owner;
	const struct pr_ops *pr_ops;
};

/**
 * I/O调度算法将多个bio排序后组成一个request
 * 
 * 当bio提交给I/O调度器后, 调度器可能插入现存request, 也可能生成新的request
 * 
 * 每个块设备/分区都有自己的request_queue, 调度器整理后的request会被分发到设备级request_queue
 */

//描述一个bio
struct bio {
	struct bio *bi_next;
	struct bvec_iter bi_iter;

	// 多个内存片, 多个vec首地址, bi_iter.bi_idx为index
	struct bio_vec *bi_io_vec; // 描述bio请求对应所有的内存/片段
	struct bio_vec { // 描述一页内存中的某一部分用于bio
		struct page *bv_page;
		unsigned int bv_len;
		unsigned int bv_offset;
	};

	struct bio_vec bi_inline_vecs[0];
};

////////////////////////////////request and queue操作部分

// 初始化请求队列, 请求处理函数指针, 控制访问队列的自旋锁, 在块设备初始化时候调用, 检查返回值
struct request_queue *blk_init_queue(request_fn_proc *rfn, spinlock_t *lock);

// 清除请求队列, 在驱动卸载时候使用
void blk_cleanup_queue(struct request_queue *q);

// 分配请求队列 & 绑定queue和make_request_fn
struct request_queue *blk_alloc_queue(gfp_t gfp_mask);
void blk_queue_make_request(struct request_queue *q, make_request_fn *mfn); //绕过调度器

// 提取请求, 返回下一个要处理的请求, 由调度器决定返回哪个, 如果返回NULL则请求仍然存在
struct request *blk_peek_request(struct request_queue *q);

// 启动请求, 从队列移除请求
void blk_start_request(struct request *req);
// fetch = peek + start
struct request *blk_fetch_request(struct request_queue *q);

// 遍历
__rq_for_each_bio(_bio, rq); // 遍历一个请求的所有bio

__bio_for_each_segment(bvl, bio, iter, start); // 遍历一个bio的所有bio_vec
bio_for_each_segment(bvl, bio, iter);

rq_for_each_segment(bvl, _rq, _iter); // 遍历一个请求所有bio的所有bio_vec

// 报告请求完成否, error=0完成
void blk_end_request_all(struct request *rq, blk_status_t error);
void __blk_end_request_all(struct request *rq, blk_status_t error); // 在持有队列锁时候调用
void bio_endio(struct bio *bio); // 如果blk_queue_make_request绕过调度器则需这样通知结束
void bio_endio_error(bio); // IO 错误上报错误


/**
 * 调度器
 * 	Noop I/O // FIFO不排序只合并, 适合flash
 * 	Anticipatory I/O // 2010年弃用
 * 	Deadline I/O // 适合读取较多的环境如数据库
 * 	CFQ I/O // 所有任务平均分配均与IO带宽, 多媒体中保证音视频及时读取
 * 更改调度器:
 * 	启动参数: kernel elevatot=deadline
 * 	命令: echo deadline > /sys/block/sda/queue/scheduler
 */
