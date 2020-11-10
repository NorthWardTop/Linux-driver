/**
 * spi driver
 */


#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/ioport.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/amba/bus.h>
#include <linux/amba/pl022.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/pm_runtime.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>

static inline int 
spi_write(struct spi_device *spi, const u8 *buf, size_t len)
{
	struct spi_transfer t = {
		// 设置发送缓冲区指针
		.tx_buf		= buf,
		.len		= len,
	};
	struct spi_message m;
	
	// 初始化一个message
	spi_message_init(&m);
	// 将一个transfer添加到尾部
	spi_message_add_tail(&t, &m);
	
	// 同步发送message
	return spi_sync(spi, &m);
}


static inline int 
spi_read(struct spi_device *spi, u8 *buf, size_t len)
{
	struct spi_transfer t = {
		// 设置接收缓冲区指针
		.rx_buf		= buf,
		.len		= len,
	};
	struct spi_message m;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	return spi_sync(spi, &m);
}



struct pl022 {
	struct amba_device		*adev;
	struct vendor_data		*vendor;
	resource_size_t			phybase;
	void __iomem			*virtbase;
	struct clk			*clk;
	struct spi_master		*master;
	struct pl022_ssp_controller	*master_info;
	/* Message per-transfer pump */
	struct tasklet_struct		pump_transfers;
	struct spi_message		*cur_msg;
	struct spi_transfer		*cur_transfer;
	struct chip_data		*cur_chip;
	bool				next_msg_cs_active;
	void				*tx;
	void				*tx_end;
	void				*rx;
	void				*rx_end;
	enum ssp_reading		read;
	enum ssp_writing		write;
	u32				exp_fifo_level;
	enum ssp_rx_level_trig		rx_lev_trig;
	enum ssp_tx_level_trig		tx_lev_trig;
	/* DMA settings */
#ifdef CONFIG_DMA_ENGINE
	struct dma_chan			*dma_rx_channel;
	struct dma_chan			*dma_tx_channel;
	struct sg_table			sgt_rx;
	struct sg_table			sgt_tx;
	char				*dummypage;
	bool				dma_running;
#endif
	int cur_cs;
	int *chipselects;
};



static int pl022_transfer_one_message(struct spi_master *master,
				struct spi_message *msg)
{
	struct pl022 *pl022 = spi_master_get_devdata(master);
	int 临时 = 0;
	
	// 初始化信息状态
	pl022->cur_msg = msg;
	msg->state = STATE_START;
	
	pl022->cur_transfer = list_entry(msg->transfers.next,
		struct spi_transfer, transfer_list);
	
	pl022->cur_chip = spi_get_ctldata(msg->spi);
	pl022->cur_cs = pl022->chipselects[msg->spi->chip_select];

	restore_state(pl022);
	flush(pl022);

	if (pl022->cur_chip->xfer_type == POLLING_TRANSFER)
		do_polling_transffer(pl022);
	else 
		do_interrupt_dma_transfer(pl022);
	
	return 0;
}


static int pl022_setup(struct spi_device *spi)
{
	if (spi->mode & SPI_CPOL)
		tmp = SSP_CLK_POL_IDLE_HIGH;
	else 
		tmp = SSP_CLK_POL_IDLE_LOW;
	SSP_WRITE_BITS(chip->cr0, tmp, SSP_CR0_MASK_SP0, 6);

	if (spi->mode & SPI_CPHA)
		tmp = SSP_CLK_SECOND_EDGE;
	else 
		tmp = SSP_CLK_FIRST_EDGE;
	SPP_WRITE_BITS(chip->cr0, tmp, SSP_CR0_MASK_SPH, 7);
}


static int pl022_probe(struct amba_device *adev, const struct amba_id *id)
{
	struct device *dev = &adev->dev;
	struct spi_master *master = spi_alloc_master(dev, sizeof(struct pl022));

	master->bus_num = platform_info->bus_id;
	master->num_chipselect = num_cs;
	master->cleanup = pl022_cleanup;
	master->setup = pl022_setup;
	master->auto_runtime_pm = true;
	master->transfer_one_message = pl022_transfer_one_message;
	master->unprepare_transfer_hardware = pl022_unprepare_transfer_hardware;
	master->rt = platform_info->rt;
	master->dev.of_node = dev->of_node;
}



