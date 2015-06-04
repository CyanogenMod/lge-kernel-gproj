#!/bin/bash

# Architecture
export ARCH=arm
# Subarchitecture
export SUBARCH=arm
# The kernel configuration by default
export KERNEL_DEFCONFIG=nitrogen_e975_defconfig
# The path to the compiler
export PATH=$PATH:../Toolchain/Linaro-4.9.3/bin
# The path to the cross compiler
export CROSS_COMPILE=../Toolchain/Linaro-4.9.3/bin/arm-cortex_a15-linux-gnueabihf-
export KBUILD_OUTPUT=../nkbuild

echo Автосборщик Nitrogen Kernel
mkdir ../nkbuild
mkdir ../builded
mkdir ../builded/GEEHRC
mkdir ../builded/GEEHRC/tmp
mkdir ../builded/GEEHRC/tmp/anykernel
mkdir ../builded/GEEHRC/system
mkdir ../builded/GEEHRC/system/lib
mkdir ../builded/GEEHRC/system/lib/modules
mkdir ../builded/GEEB
mkdir ../builded/GEEB/tmp
mkdir ../builded/GEEB/tmp/anykernel
mkdir ../builded/GEEB/system
mkdir ../builded/GEEB/system/lib
mkdir ../builded/GEEB/system/lib/modules
echo Выбор конфига E975
make nitrogen_e975_defconfig
echo Сборка ядра
make -j8 -o4
echo Копирование скомпилированного ядра
cp ../nkbuild/arch/arm/boot/zImage ../builded/GEEHRC/tmp/anykernel/zImage
cp ../nkbuild/crypto/ansi_cprng.ko ../builded/GEEHRC/system/lib/modules/ansi_cprng.ko
cp ../nkbuild/drivers/bluetooth/bluetooth-power.ko ../builded/GEEHRC/system/lib/modules/bluetooth-power.ko
cp ../nkbuild/arch/arm/mach-msm/msm-buspm-dev.ko ../builded/GEEHRC/system/lib/modules/msm-buspm-dev.ko
cp ../nkbuild/drivers/crypto/msm/qce40.ko ../builded/GEEHRC/system/lib/modules/qce40.ko
cp ../nkbuild/drivers/crypto/msm/qcedev.ko ../builded/GEEHRC/system/lib/modules/qcedev.ko
cp ../nkbuild/drivers/crypto/msm/qcrypto.ko ../builded/GEEHRC/system/lib/modules/qcrypto.ko
cp ../nkbuild/drivers/media/radio/radio-iris-transport.ko ../builded/GEEHRC/system/lib/modules/radio-iris-transport.ko
cp ../nkbuild/drivers/scsi/scsi_wait_scan.ko ../builded/GEEHRC/system/lib/modules/scsi_wait_scan.ko
echo Выбор конфига E970
make nitrogen_e970_defconfig
echo Сборка ядра
make -j8 -o4
echo Копирование скомпилированного ядра
cp ../nkbuild/arch/arm/boot/zImage ../builded/GEEB/tmp/anykernel/zImage
cp ../nkbuild/crypto/ansi_cprng.ko ../builded/GEEB/system/lib/modules/ansi_cprng.ko
cp ../nkbuild/drivers/bluetooth/bluetooth-power.ko ../builded/GEEB/system/lib/modules/bluetooth-power.ko
cp ../nkbuild/arch/arm/mach-msm/msm-buspm-dev.ko ../builded/GEEB/system/lib/modules/msm-buspm-dev.ko
cp ../nkbuild/drivers/crypto/msm/qce40.ko ../builded/GEEB/system/lib/modules/qce40.ko
cp ../nkbuild/drivers/crypto/msm/qcedev.ko ../builded/GEEB/system/lib/modules/qcedev.ko
cp ../nkbuild/drivers/crypto/msm/qcrypto.ko ../builded/GEEB/system/lib/modules/qcrypto.ko
cp ../nkbuild/drivers/media/radio/radio-iris-transport.ko ../builded/GEEB/system/lib/modules/radio-iris-transport.ko
cp ../nkbuild/drivers/scsi/scsi_wait_scan.ko ../builded/GEEB/system/lib/modules/scsi_wait_scan.ko
echo Сборка завершена! Kernel находится по адресу:
echo ../builded
echo ../builded/GEEHRC
echo ../builded/GEEB
