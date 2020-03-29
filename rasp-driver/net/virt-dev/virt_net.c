/**
 * 虚拟网卡驱动
 */

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/module.h>
#include <linux/scatterlist.h>
#include <linux/if_vlan.h>
#include <linux/slab.h>
#include <linux/cpu.h>
#include <linux/average.h>
#include <net/busy_poll.h>
#include <linux/types.h>
#include <linux/virtio_ids.h>
#include <linux/virtio_config.h>
#include <linux/virtio_types.h>
#include <linux/if_ether.h>
#include <linux/kernel.h>
#include <linux/init.h>


#define LKM_NAME	"vnet"

struct virt_net_dev
{
	unsigned char *dev_name;
	unsigned long count;
	spinlock_t lock;
	struct mutex mutex;
	struct net_device *net_dev;
};
static struct virt_net_dev *pvnet_dev;


//设置net_device对象
// void net_setup(struct net_device *dev)
// {
	
// }


static int vnetdev_setup(struct virt_net_dev *dev, struct net_device *netdev, int index)
{
	dev->dev_name = kzalloc(sizeof(LKM_NAME), GFP_KERNEL);
	dev->net_dev = netdev;
	dev->count = 0;
	spin_lock_init(&dev->lock);
	mutex_init(&dev->mutex);
	
	return 0;
}

static int vnetdev_unsetup(struct virt_net_dev *dev)
{
	kfree(pvnet_dev->dev_name);
	kfree(pvnet_dev);
	return 0;
}


static netdev_tx_t vnetdev_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct virt_net_dev *vnet = netdev_priv(dev);
	vnet->count++;
	printk("Current send packet count: %ld", vnet->count);


	return NETDEV_TX_OK;
}


static const struct net_device_ops netdev_ops = {
	.ndo_start_xmit = vnetdev_xmit,
};


static __init int lkm_vnet_init(void)
{
	int ret;
	struct net_device *net_dev = NULL;
	//分配对象
	net_dev = alloc_netdev(sizeof(struct virt_net_dev), "vnet%d", NET_NAME_UNKNOWN, ether_setup);
	net_dev->netdev_ops = &netdev_ops;
	//注册对象
	ret = register_netdev(net_dev);

	//分配全局对象
	pvnet_dev = kzalloc(sizeof(struct virt_net_dev), GFP_KERNEL);
	//综合全局设备设置
	ret = vnetdev_setup(pvnet_dev, net_dev, 0);
	return ret;
}



static __exit void lkm_vnet_exit(void)
{
	unregister_netdev(pvnet_dev->net_dev); //取消注册netdev设备
	free_netdev(pvnet_dev->net_dev); //释放netdev
	// 取消设备对象的设置
	vnetdev_unsetup(pvnet_dev);



	printk("module removed\n");
}




module_init(lkm_vnet_init);
module_exit(lkm_vnet_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("yonghuilee.cn@gmail.com");
MODULE_DESCRIPTION("virt_net first process");
MODULE_VERSION("1.0");


