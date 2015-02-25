#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x9412fa01, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x397b7105, __VMLINUX_SYMBOL_STR(kmem_cache_destroy) },
	{ 0xee54fb05, __VMLINUX_SYMBOL_STR(kthread_stop) },
	{ 0x5be8aa36, __VMLINUX_SYMBOL_STR(kthread_create_on_node) },
	{ 0xb1092e41, __VMLINUX_SYMBOL_STR(kmem_cache_create) },
	{ 0xca42032b, __VMLINUX_SYMBOL_STR(kmem_cache_free) },
	{ 0xda3e43d1, __VMLINUX_SYMBOL_STR(_raw_spin_unlock) },
	{ 0xd52bf1ce, __VMLINUX_SYMBOL_STR(_raw_spin_lock) },
	{ 0x593a99b, __VMLINUX_SYMBOL_STR(init_timer_key) },
	{ 0x10a46f68, __VMLINUX_SYMBOL_STR(kmem_cache_alloc) },
	{ 0x20c55ae0, __VMLINUX_SYMBOL_STR(sscanf) },
	{ 0x4f6b400b, __VMLINUX_SYMBOL_STR(_copy_from_user) },
	{ 0x1000e51, __VMLINUX_SYMBOL_STR(schedule) },
	{ 0xe94c016a, __VMLINUX_SYMBOL_STR(current_task) },
	{ 0xb87bf447, __VMLINUX_SYMBOL_STR(sched_setscheduler) },
	{ 0x353c3b0c, __VMLINUX_SYMBOL_STR(remove_proc_entry) },
	{ 0xcc72f4ce, __VMLINUX_SYMBOL_STR(proc_create_data) },
	{ 0x45076c0d, __VMLINUX_SYMBOL_STR(proc_mkdir) },
	{ 0x357b0269, __VMLINUX_SYMBOL_STR(pid_task) },
	{ 0xa13f36ec, __VMLINUX_SYMBOL_STR(find_vpid) },
	{ 0x8603ad8d, __VMLINUX_SYMBOL_STR(wake_up_process) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0x4f8b5ddb, __VMLINUX_SYMBOL_STR(_copy_to_user) },
	{ 0x91715312, __VMLINUX_SYMBOL_STR(sprintf) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xd2b09ce5, __VMLINUX_SYMBOL_STR(__kmalloc) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "4BD2B1C12B1D65447DA8A76");
