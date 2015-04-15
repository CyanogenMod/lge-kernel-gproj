#!/bin/bash

echo Автосборщик Nitrogen Kernel
mkdir ../builded
mkdir ../builded/GEEHRC
mkdir ../builded/GEEHRC/tmp
mkdir ../builded/GEEHRC/tmp/anykernel
mkdir ../builded/GEEHRC/system
mkdir ../builded/GEEHRC/system/lib
mkdir ../builded/GEEHRC/system/modules
mkdir ../builded/GEEB
mkdir ../builded/GEEB/tmp
mkdir ../builded/GEEB/tmp/anykernel
mkdir ../builded/GEEB/system
mkdir ../builded/GEEB/system/lib
mkdir ../builded/GEEB/system/modules
echo Выбор конфига E975
make e975_defconfig
echo Настройка ядра
make menuconfig
echo Сборка ядра
make -j8 -o4
echo Копирование скомпилированного ядра
cp arch/arm/boot/zImage ../builded/GEEHRC/tmp/anykernel/zImage
cp crypto/ansi_cprng.ko ../builded/GEEHRC/system/lib/modules/ansi_cprng.ko
cp drivers/bluetooth/bluetooth-power.ko ../builded/GEEHRC/system/lib/modules/bluetooth-power.ko
cp arch/arm/mach-msm/msm-buspm-dev.ko ../builded/GEEHRC/system/lib/modules/msm-buspm-dev.ko
cp drivers/crypto/msm/qce40.ko ../builded/GEEHRC/system/lib/modules/qce40.ko
cp drivers/crypto/msm/qcedev.ko ../builded/GEEHRC/system/lib/modules/qcedev.ko
cp drivers/crypto/msm/qcrypto.ko ../builded/GEEHRC/system/lib/modules/qcrypto.ko
cp drivers/media/radio/radio-iris-transport.ko ../builded/GEEHRC/system/lib/modules/radio-iris-transport.ko
cp drivers/scsi/scsi_wait_scan.ko ../builded/GEEHRC/system/lib/modules/scsi_wait_scan.ko
echo Выбор конфига E970
make e970_defconfig
echo Настройка ядра
make menuconfig
echo Сборка ядра
make -j8 -o4
echo Копирование скомпилированного ядра
cp arch/arm/boot/zImage ../builded/GEEB/tmp/anykernel/zImage
cp crypto/ansi_cprng.ko ../builded/GEEB/system/lib/modules/ansi_cprng.ko
cp drivers/bluetooth/bluetooth-power.ko ../builded/GEEB/system/lib/modules/bluetooth-power.ko
cp arch/arm/mach-msm/msm-buspm-dev.ko ../builded/GEEB/system/lib/modules/msm-buspm-dev.ko
cp drivers/crypto/msm/qce40.ko ../builded/GEEB/system/lib/modules/qce40.ko
cp drivers/crypto/msm/qcedev.ko ../builded/GEEB/system/lib/modules/qcedev.ko
cp drivers/crypto/msm/qcrypto.ko ../builded/GEEB/system/lib/modules/qcrypto.ko
cp drivers/media/radio/radio-iris-transport.ko ../builded/GEEB/system/lib/modules/radio-iris-transport.ko
cp drivers/scsi/scsi_wait_scan.ko ../builded/GEEB/system/lib/modules/scsi_wait_scan.ko
echo Сборка завершена! Kernel находится по адресу:
echo ../builded
echo ../builded/GEEHRC
echo ../builded/GEEB
