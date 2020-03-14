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
	{ 0xb7700415, "param_ops_int" },
	{ 0x37a0cba, "kfree" },
	{ 0xe825a41f, "cdev_del" },
	{ 0xd204ddc9, "kmalloc_caches" },
	{ 0xc586ff95, "cdev_add" },
	{ 0xb223b2ef, "cdev_init" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x3dd2ea23, "kmem_cache_alloc_trace" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0x3fd78f3b, "register_chrdev_region" },
	{ 0xc38c83b8, "mod_timer" },
	{ 0x526c3a6c, "jiffies" },
	{ 0x24d273d1, "add_timer" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0x7c32d0f0, "printk" },
	{ 0x2b68bd2f, "del_timer" },
	{ 0x2e5810c6, "__aeabi_unwind_cpp_pr1" },
	{ 0xbc10dd97, "__put_user_4" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "3556A8E66AF08B8AFC17E3B");
