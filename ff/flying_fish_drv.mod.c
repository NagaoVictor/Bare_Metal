#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

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



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x5353a139, "__platform_driver_register" },
	{ 0x61eb329b, "gpiod_set_value" },
	{ 0x92997ed8, "_printk" },
	{ 0x6168c53b, "devm_kmalloc" },
	{ 0x339745dc, "devm_gpiod_get" },
	{ 0x45696576, "_dev_err" },
	{ 0x4b0d7841, "gpiod_to_irq" },
	{ 0x364ba19e, "devm_request_threaded_irq" },
	{ 0xcfcce16f, "platform_driver_unregister" },
	{ 0x97f01b96, "gpiod_get_value" },
	{ 0x38022ea8, "module_layout" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("of:N*T*Cnagao,flying-fish");
MODULE_ALIAS("of:N*T*Cnagao,flying-fishC*");

MODULE_INFO(srcversion, "EFC2547948698546F1B1EBF");
