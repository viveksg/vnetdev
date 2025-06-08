#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

#ifdef CONFIG_UNWINDER_ORC
#include <asm/orc_header.h>
ORC_HEADER;
#endif

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
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
__used __section("__versions") = {
	{ 0xc6f46339, "init_timer_key" },
	{ 0xb2fcb56d, "queue_delayed_work_on" },
	{ 0x122c3a7e, "_printk" },
	{ 0x2cd5d0e1, "platform_driver_unregister" },
	{ 0x15ba50a6, "jiffies" },
	{ 0x9fa7184a, "cancel_delayed_work_sync" },
	{ 0x8c03d20c, "destroy_workqueue" },
	{ 0x7cd8d75e, "page_offset_base" },
	{ 0x69acdf38, "memcpy" },
	{ 0x4c9d28b0, "phys_base" },
	{ 0x9ed12e20, "kmalloc_large" },
	{ 0xfb578fc5, "memset" },
	{ 0xc2e0098c, "alloc_etherdev_mqs" },
	{ 0x713adfb1, "platform_get_resource" },
	{ 0x41ed3709, "get_random_bytes" },
	{ 0xcf8ab1e3, "dev_addr_mod" },
	{ 0x5029ec4, "register_netdev" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0xd2b00e8d, "__netdev_alloc_skb" },
	{ 0x435a3709, "skb_put" },
	{ 0x4b5f959d, "skb_push" },
	{ 0x855c4551, "netif_rx" },
	{ 0xcbd4898c, "fortify_panic" },
	{ 0x87a21cb3, "__ubsan_handle_out_of_bounds" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0x9e07cb93, "__platform_driver_register" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0x2f99106c, "unregister_netdev" },
	{ 0x49cd25ed, "alloc_workqueue" },
	{ 0xffeedf6a, "delayed_work_timer_fn" },
	{ 0xb2b23fc2, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "E5848EFF08AA2FCD0FAED0E");
