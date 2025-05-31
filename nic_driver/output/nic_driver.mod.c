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
	{ 0xc2e0098c, "alloc_etherdev_mqs" },
	{ 0x713adfb1, "platform_get_resource" },
	{ 0x41ed3709, "get_random_bytes" },
	{ 0xcf8ab1e3, "dev_addr_mod" },
	{ 0x5029ec4, "register_netdev" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x2cd5d0e1, "platform_driver_unregister" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0x9e07cb93, "__platform_driver_register" },
	{ 0x2f99106c, "unregister_netdev" },
	{ 0x122c3a7e, "_printk" },
	{ 0xb2b23fc2, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "1F88334B320F995529629DD");
