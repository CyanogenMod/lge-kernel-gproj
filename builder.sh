#!/bin/bash

# Toolchain dir
dir_toolchain="Linaro-4.9.3/bin"
# Toolchain prefix
toolchain_prefix="arm-cortex_a15-linux-gnueabihf-"

dir_kbuild=nkbuild

dir_kbuilded=builded

dir_e975=e975

dir_e970=e970

cpu_cores="-j8"

# Architecture
export ARCH=arm
# Subarchitecture
export SUBARCH=arm
# The kernel configuration by default
export KERNEL_DEFCONFIG=nitrogen_e975_defconfig
# The path to the compiler
export PATH=$PATH:../$dir_toolchain
# The path to the cross compiler
export CROSS_COMPILE=../$dir_toolchain/$toolchain_prefix

export KBUILD_OUTPUT=../$dir_kbuild

rm -irf ../$dir_kbuild
mkdir ../$dir_kbuild
# E975
test -d ../$dir_kbuilded                              || mkdir ../$dir_kbuilded
test -d ../$dir_kbuilded/$dir_e975                    || mkdir ../$dir_kbuilded/$dir_e975
test -d ../$dir_kbuilded/$dir_e975/tmp                || mkdir ../$dir_kbuilded/$dir_e975/tmp
test -d ../$dir_kbuilded/$dir_e975/tmp/anykernel      || mkdir ../$dir_kbuilded/$dir_e975/tmp/anykernel
test -d ../$dir_kbuilded/$dir_e975/system             || mkdir ../$dir_kbuilded/$dir_e975/system
test -d ../$dir_kbuilded/$dir_e975/system/lib         || mkdir ../$dir_kbuilded/$dir_e975/system/lib
test -d ../$dir_kbuilded/$dir_e975/system/lib/modules || mkdir ../$dir_kbuilded/$dir_e975/system/lib/modules
# E970
test -d ../$dir_kbuilded/$dir_e970                    || mkdir ../$dir_kbuilded/$dir_e970
test -d ../$dir_kbuilded/$dir_e970/tmp                || mkdir ../$dir_kbuilded/$dir_e970/tmp
test -d ../$dir_kbuilded/$dir_e970/tmp/anykernel      || mkdir ../$dir_kbuilded/$dir_e970/tmp/anykernel
test -d ../$dir_kbuilded/$dir_e970/system             || mkdir ../$dir_kbuilded/$dir_e970/system
test -d ../$dir_kbuilded/$dir_e970/system/lib         || mkdir ../$dir_kbuilded/$dir_e970/system/lib
test -d ../$dir_kbuilded/$dir_e970/system/lib/modules || mkdir ../$dir_kbuilded/$dir_e970/system/lib/module
make nitrogen_e975_defconfig
make $cpu_cores -o4
test -e ../$dir_kbuild/arch/arm/boot/zImage                        || cp ../$dir_kbuild/arch/arm/boot/zImage ../$dir_kbuilded/$dir_e975/tmp/anykernel/zImage
test -e ../$dir_kbuild/crypto/ansi_cprng.ko                        || cp ../$dir_kbuild/crypto/ansi_cprng.ko ../$dir_kbuilded/$dir_e975/system/lib/modules/ansi_cprng.ko
test -e ../$dir_kbuild/drivers/bluetooth/bluetooth-power.ko        || cp ../$dir_kbuild/drivers/bluetooth/bluetooth-power.ko ../$dir_kbuilded/$dir_e975/system/lib/modules/bluetooth-power.ko
test -e ../$dir_kbuild/arch/arm/mach-msm/msm-buspm-dev.ko          || cp ../$dir_kbuild/arch/arm/mach-msm/msm-buspm-dev.ko ../$dir_kbuilded/$dir_e975/system/lib/modules/msm-buspm-dev.ko
test -e ../$dir_kbuild/drivers/crypto/msm/qce40.ko                 || cp ../$dir_kbuild/drivers/crypto/msm/qce40.ko ../$dir_kbuilded/$dir_e975/system/lib/modules/qce40.ko
test -e ../$dir_kbuild/drivers/crypto/msm/qcedev.ko                || cp ../$dir_kbuild/drivers/crypto/msm/qcedev.ko ../$dir_kbuilded/$dir_e975/system/lib/modules/qcedev.ko
test -e ../$dir_kbuild/drivers/crypto/msm/qcrypto.ko               || cp ../$dir_kbuild/drivers/crypto/msm/qcrypto.ko ../$dir_kbuilded/$dir_e975/system/lib/modules/qcrypto.ko
test -e ../$dir_kbuild/drivers/media/radio/radio-iris-transport.ko || cp ../$dir_kbuild/drivers/media/radio/radio-iris-transport.ko ../$dir_kbuilded/$dir_e975/system/lib/modules/radio-iris-transport.ko
test -e ../$dir_kbuild/drivers/scsi/scsi_wait_scan.ko              || cp ../$dir_kbuild/drivers/scsi/scsi_wait_scan.ko ../$dir_kbuilded/$dir_e975/system/lib/modules/scsi_wait_scan.ko
make nitrogen_e970_defconfig
make $cpu_cores -o4
test -e ../$dir_kbuild/arch/arm/boot/zImage                        || cp ../$dir_kbuild/arch/arm/boot/zImage ../$dir_kbuilded/$dir_e970/tmp/anykernel/zImage
test -e ../$dir_kbuild/crypto/ansi_cprng.ko                        || cp ../$dir_kbuild/crypto/ansi_cprng.ko ../$dir_kbuilded/$dir_e970/system/lib/modules/ansi_cprng.ko
test -e ../$dir_kbuild/drivers/bluetooth/bluetooth-power.ko        || cp ../$dir_kbuild/drivers/bluetooth/bluetooth-power.ko ../$dir_kbuilded/$dir_e970/system/lib/modules/bluetooth-power.ko
test -e ../$dir_kbuild/arch/arm/mach-msm/msm-buspm-dev.ko          || cp ../$dir_kbuild/arch/arm/mach-msm/msm-buspm-dev.ko ../$dir_kbuilded/$dir_e970/system/lib/modules/msm-buspm-dev.ko
test -e ../$dir_kbuild/drivers/crypto/msm/qce40.ko                 || cp ../$dir_kbuild/drivers/crypto/msm/qce40.ko ../$dir_kbuilded/$dir_e970/system/lib/modules/qce40.ko
test -e ../$dir_kbuild/drivers/crypto/msm/qcedev.ko                || cp ../$dir_kbuild/drivers/crypto/msm/qcedev.ko ../$dir_kbuilded/$dir_e970/system/lib/modules/qcedev.ko
test -e ../$dir_kbuild/drivers/crypto/msm/qcrypto.ko               || cp ../$dir_kbuild/drivers/crypto/msm/qcrypto.ko ../$dir_kbuilded/$dir_e970/system/lib/modules/qcrypto.ko
test -e ../$dir_kbuild/drivers/media/radio/radio-iris-transport.ko || cp ../$dir_kbuild/drivers/media/radio/radio-iris-transport.ko ../$dir_kbuilded/$dir_e970/system/lib/modules/radio-iris-transport.ko
test -e ../$dir_kbuild/drivers/scsi/scsi_wait_scan.ko              || cp ../$dir_kbuild/drivers/scsi/scsi_wait_scan.ko ../$dir_kbuilded/$dir_e970/system/lib/modules/scsi_wait_scan.ko
