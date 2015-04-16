GCC_VERSION      := $(shell $(CONFIG_SHELL) $(PWD)/scripts/gcc-version.sh $(CROSS_COMPILE)gcc)
EXTRA_CFLAGS += -Idrivers/media/video/msm
EXTRA_CFLAGS += -Idrivers/media/video/msm/io
EXTRA_CFLAGS += -Idrivers/media/video/msm/eeprom
EXTRA_CFLAGS += -Idrivers/media/video/msm/csi
obj-$(CONFIG_MSM_CAMERA_SENSOR) += msm_sensor_common.o msm_sensor.o msm_sensor_bayer.o msm_sensor_init.o
obj-$(CONFIG_S5K4E5YA) += s5k4e5ya_v4l2.o
obj-$(CONFIG_OV5693) += ov5693_v4l2.o
obj-$(CONFIG_IMX091) += imx091.o
obj-$(CONFIG_IMX111) += imx111.o

ifneq ($(or $(CONFIG_MACH_APQ8064_J1SK),$(CONFIG_MACH_APQ8064_J1KT),$(CONFIG_MACH_APQ8064_J1A),$(CONFIG_MACH_APQ8064_J1A),$(CONFIG_MACH_APQ8064_J1B),$(CONFIG_MACH_APQ8064_J1R),$(CONFIG_MACH_APQ8064_J1SP),$(CONFIG_MACH_APQ8064_J1TL),$(CONFIG_MACH_APQ8064_J1U)),)
 obj-$(CONFIG_IMX119) += imx119_v4l2_g.o
else
 obj-$(CONFIG_IMX119) += imx119_v4l2.o
endif

obj-$(CONFIG_CE1702) += ce1702_interface.o
obj-$(CONFIG_CE1702) += ce1702_v4l2.o
obj-$(CONFIG_IMX132) += imx132_v4l2.o
obj-$(CONFIG_IMX135) += imx135_v4l2.o
obj-$(CONFIG_OV9724) += ov9724_v4l2.o
