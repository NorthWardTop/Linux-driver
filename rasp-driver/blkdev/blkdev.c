


/**
 * 块设备的I/O操作方式与字符设备的存在较大的不同，因而引入了request_queue、request、bio等一系列数据结构。在整个块设备的I/O操作中，贯穿始终的就是“请求”，字符设备的I/O操作则是直接进行不绕弯，块设备的I/O操作会排队和整合。

驱动的任务是处理请求，对请求的排队和整合由I/O调度算法解决，因此，块设备驱动的核心就是请求处理函数或“制造请求”函数。

尽管在块设备驱动中仍然存在block_device_operations结构体及其成员函数，但不再包含读写类的成员函数，而只是包含打开、释放及I/O控制等与具体读写无关的函数。

块设备驱动的结构相对复杂，但幸运的是，块设备不像字符设备那样包罗万象，它通常就是存储设备，而且驱动的主体已经由Linux内核提供，针对一个特定的硬件系统，驱动工程师所涉及的工作往往只是编写极其少量的与硬件平台相关的代码。

*/




#include  <linux/spinlock_types.h>
#include  <linux/blkdev.h>
#include  <linux/module.h>
#include  <linux/kernel.h>
#include  <linux/fs.h>
#include  <linux/genhd.h>
#include  <linux/init.h>


#define DISK_MAJOR		255




static int xxx_open(struct block_device *bdev, fmode_t mode)
{
	struct xxx_dev *dev = bdev->bd_disk->private_data;

	return 0;
}


static void xxx_release(struct gendisk *disk, fmode_t mode)
{
	struct xxx_dev *dev = disk->private_data;
}


static int mmc_blk_ioctl(struct block_device *bdev, fmode_t mode,
	unsigned int cmd, unsigned long arg)
{
	int ret = -EINVAL;
	if (cmd == MMC_IOC_CMD)
		ret = mmc_blk_ioctl_cmd(bdev, (struct mmc_ioc_cmd __user*)arg);
	return ret;
}


static int disk_init(void)
{
	int err = 0;
	struct request_queue *disk_queue;
	
	// 注册设备
	err = register_blkdev(DISK_MAJOR, "disk");
	if (err < 0) {
		err = -EIO;
		goto out;
	}

	// 请求队列初始化
	disk_queue = blk_init_queue(disk_queue, disk_lock);
	if (!disk_queue)
		goto out_queue;
	blk_queue_max_hw_sectors(disk_queue, 255);
	blk_queue_logical_block_size(disk_queue, 512);
	
	// gendisk初始化
	disk_disks->major = DISK_MAJOR;
	disk_disks->first_minor = 0;
	disk_dosks->fops = &xxx_op;
	disk_disks->queue = disk_queue;
	sprintf(disk_disks->disk_name, "disk%d", i);
	set_capacity(disk_disks, size * 2);
	add_disk(disk_disks); // 注册对象
	return 0;

out_queue:
	unregister_blkdev(DISK_MAJOR, "xxx");
out:
	put_disk(disk_disks);
	blk_cleanup_queue(disk_queue);

	return -ENOMEM;

}


static void disk_exit(void)
{
	blk_cleanup_queue(disk_queue); // 清理请求队列
	put_disk(disk_disks); //删除对disk的引用
	unregister_blkdev(DISK_MAJOR, "xxx"); //注销块设备
}


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