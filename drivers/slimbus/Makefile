#
# Makefile for kernel slimbus framework.
#
#// [[LGE_BSP_AUDIO, jeremy.pi@lge.com, Audience eS325 ALSA SoC Audio driver
ifeq ($(CONFIG_SND_SOC_ES325_SLIM),y)
obj-$(CONFIG_SLIMBUS)			+= slimbus_es325.o
obj-$(CONFIG_SLIMBUS_MSM_CTRL)		+= slim-msm-ctrl_es325.o
else
obj-$(CONFIG_SLIMBUS)			+= slimbus.o
obj-$(CONFIG_SLIMBUS_MSM_CTRL)		+= slim-msm-ctrl.o
endif
#// ]]LGE_BSP_AUDIO, jeremy.pi@lge.com, Audience eS325 ALSA SoC Audio driver
