



/**
 * 虚拟总线: platform总线, 对应platform_device设备, 对应的驱动platform_driver
 */
struct platform_device {
	const char *name;
	int id;
	bool id_auto;
	struct device dev;
	u32 num_resource *resource;

	const struct platform_device_id *id_entry;
	char *driver_override; //驱动名称强制匹配

	struct mfd_cell *mfd_cell;

	struct pdev_archdata;
};

// xxx_driver中xxx是总线名称, 如SPI IIC等
struct platform_driver {
	int (*probe)(struct platform_device*);
	int (*remove)(struct platform_device*);
	void (*shutdown)(struct platform_device*);
	// 以下两个函数指针无需填充, 需要在device_driver的dev_pm_ops实现
	int (*suspend)(struct platform_device*, pm_message_t state);
	int (*resume)(struct platform_device*);
	struct device_driver driver;
	const struct platform_device_id *id_table;
	bool prevent_deferred_probe;
};


struct device_driver {
	const char *name;
	struct bus_type *bus;
	struct module *owner;
	const char              *mod_name;  /* used for built-in modules */
	bool suppress_bind_attrs;           /* disables bind/unbind via sysfs */
	const struct of_device_id           *of_match_table;
	const struct acpi_device_id         *acpi_match_table;
	int (*probe) (struct device *dev);
	int (*remove) (struct device *dev);
	void (*shutdown) (struct device *dev);
	int (*suspend) (struct device *dev, pm_message_t state);
	int (*resume) (struct device *dev);
	const struct attribute_group **groups;

	const struct dev_pm_ops *pm;
	struct driver_private *p;
}



/**
 * match函数决定device和driver如何匹配
 * 
 * 从代码清单12.5可以看出，匹配platform_device和platform_driver有4种可能性，
 * 一是基于设备树风格的匹配；
 * 二是基于ACPI风格的匹配；
 * 三是匹配ID表（即platform_device设备名是否出现在platform_driver的ID表内）；
 * 第四种是匹配platform_device设备名和驱动的名字。
 */
static int platform_match(struct device *dev, struct device_driver *drv)
{
	struct platform_device *pdev = to_platfrom_device(dev);
	struct platform_driver *pdrv = to_platfrom_driver(drv);

	// 首先尝试OF风格的匹配
	if (of_driver_match_device(dev, drv))
		return 1;

	// 其次尝试ACPI风格
	if (acpi_driver_match_device(dev, drv))
		return 1;
	
	// 针对id表进行匹配
	if (pdrv->id_table)
		return platform_match_id(pdrv->id_table, pdev) != NULL;

	// 返回使用到驱动名称匹配
	return strcmp(pdev->name, drv->name) == 0;
}


// platform总线的bus_type实例platform_bus_type;
struct bus_type platform_bus_type = {
	.name           = "platform",
	.dev_groups     = platform_dev_groups,
	.match          = platform_match,
	.uevent         = platform_uevent,
	.pm             = &platform_dev_pm_ops,
};




