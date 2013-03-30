subdir-ccflags-$(CONFIG_ARCH_MSM) += -Iarch/arm/mach-msm

-include $(src)/Makefile.board

obj-$(CONFIG_MACH_LGE) += devices_lge.o 
obj-$(CONFIG_LGE_HANDLE_PANIC) += lge_handle_panic.o
obj-$(CONFIG_LGE_BOOT_TIME_CHECK) += lge_boot_time_checker.o
obj-$(CONFIG_LGE_QFPROM_INTERFACE) += lge_qfprom_access.o
obj-$(CONFIG_LGE_ECO_MODE) += lge_kernel_driver.o
obj-$(CONFIG_LGE_QC_LCDC_LUT) += lge_qc_lcdc_luts.o
obj-$(CONFIG_LGE_KCAL) += lge_kcal_ctrl.o
obj-$(CONFIG_LGE_KCAL_QLUT) += kcal_qlut.o
#obj-$(CONFIG_LGE_CRASH_HANDLER) += lge_panic_handler.o
obj-$(CONFIG_LGE_SUSPEND_AUTOTEST) += lge_suspend_autotest.o
obj-$(CONFIG_MACH_LGE) += lge_blocking_monitor.o
