#include <linux/build-salt.h>
#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x57ec4d50, "module_layout" },
	{ 0x37a0cba, "kfree" },
	{ 0xdd61b099, "free_netdev" },
	{ 0x65fbd371, "unregister_netdev" },
	{ 0xd204ddc9, "kmalloc_caches" },
	{ 0xd2cba70a, "ether_setup" },
	{ 0xe346f67a, "__mutex_init" },
	{ 0x3dd2ea23, "kmem_cache_alloc_trace" },
	{ 0x6db9d150, "register_netdev" },
	{ 0x24f96ff7, "alloc_netdev_mqs" },
	{ 0x2e5810c6, "__aeabi_unwind_cpp_pr1" },
	{ 0x7c32d0f0, "printk" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "C20F394F950DE8A71166462");
