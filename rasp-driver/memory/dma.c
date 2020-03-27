
#include <linux/dma-buf.h>
#include <linux/types.h>


/**
 * 使cache无效
 */
__asm__ (
	"mov r0, #0\n"  
	"mcr p15, 0, r0, c7, c7, 0\n" // 使数据和指令cache无效
	"mcr p15, 0, r0, c7, c10, 4\n" // 放空写缓冲
	"mcr p15, 0, r0, c8, c7, 0\n"// 使TLB无效
)


/**
 * 当缓冲区是驱动申请的时候, 用一致性DMA自然方便, 
 * 但是许多情况下, 缓冲区来自内核较上层, 如网络报文等, 
 * 上层很可能使用普通的kmalloc, __get_free_pages等方法申请,
 * 此时缓冲区不具有cache一致性, 应该使用流式DMA映射
 * 
 * 一般步骤是:
 * 	1. 进行流式DMA映射
 * 	2. 执行DMA操作
 * 	3. 进行流式DMA去映射
 */

// 使用单个已分配的缓冲区, 流式映射为
dma_addr_t dma_map_single(struct device *dev, void *buffer, size_t size,
enum dma_data_direction direction);
// 取消映射
void dma_unmap_single(struct device *dev, dma_addr_t dma_addr, size_t size,
enum dma_data_direction direction);


// 对于支持SG模式的情况下, 申请多个较小的不连续DMA缓冲区, 
// 防止申请太大的连续物理空间
int dma_map_sg(struct device *dev, struct scatterlist *sg, int nents,
enum dma_data_direction direction);
//获取缓冲区长度
unsigned int sg_dma_len(struct scatterlist *sg);
//获取缓冲区总线地址
dma_addr_t sg_dma_address(struct scatterlist *sg);
//取消映射
void dma_unmap_sg(struct device *dev, struct scatterlist *list,
int nents, enum dma_data_direction direction);

/////////////////////////////////////////////////////////////////////////////////

/**
 * dmaengine标准API
 * Linux内核目前推荐使用dmaengine的驱动架构来编写DMA控制器的驱动，
 * 同时外设的驱动使用标准的dmaengine API进行DMA的准备、发起和完成时的回调工作。
 */

// 使用dma前, 先向dmaengine申请dma通道
struct dma_chan *dma_request_slave_channel(struct device *dev, const char *name);
struct dma_chan *__dma_request_channel(const dma_cap_mask_t *mask,
                                         dma_filter_fn fn, void *fn_param);
//释放
void dma_release_channel(struct dma_chan *chan);



/**
 * 利用dmaengine API进行DMA操作
 */
static void xxx_dma_fini_callback(void *data)
{
	struct completion *dma_complete = data;
	complete(dma_complete);
}


issue_xxx_dma(...)
{
	rx_desc = dmaengine_prep_slave_single(xxx->rx_chan,
		xxx->dst_start, t->len, DMA_DEV_TO_MEM,
		DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
		
	rx_desc->callback = xxx_dma_fini_callback;
	rx_desc->callback_param = &xxx->rx_done;

	dmaengine_submit(rx_desc);
	dma_async_issue_pending(xxx->rx_chan);
}

