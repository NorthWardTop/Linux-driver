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
	{ 0xe30a435, "class_destroy" },
	{ 0x3512c70c, "device_destroy" },
	{ 0xd204ddc9, "kmalloc_caches" },
	{ 0x37a0cba, "kfree" },
	{ 0x8dc40069, "device_create" },
	{ 0x6bc3fbc0, "__unregister_chrdev" },
	{ 0xd15f066d, "__class_create" },
	{ 0x5bbe49f4, "__init_waitqueue_head" },
	{ 0xe346f67a, "__mutex_init" },
	{ 0xeb5bc552, "__register_chrdev" },
	{ 0x3dd2ea23, "kmem_cache_alloc_trace" },
	{ 0x9d669763, "memcpy" },
	{ 0xf4fa543b, "arm_copy_to_user" },
	{ 0xaad8c7d6, "default_wake_function" },
	{ 0x5f754e5a, "memset" },
	{ 0x63d8cb45, "kill_fasync" },
	{ 0x3dcf1ffa, "__wake_up" },
	{ 0x28cc25db, "arm_copy_from_user" },
	{ 0x983ac031, "remove_wait_queue" },
	{ 0x1000e51, "schedule" },
	{ 0xa17bd3fc, "add_wait_queue" },
	{ 0x67ea780, "mutex_unlock" },
	{ 0xc271c3be, "mutex_lock" },
	{ 0x8b3b4588, "fasync_helper" },
	{ 0x2e5810c6, "__aeabi_unwind_cpp_pr1" },
	{ 0x7c32d0f0, "printk" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "54FF5DA323BD24744BD91FB");
