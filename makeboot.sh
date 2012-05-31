#!/bin/bash

#offest 0x50000
uboot_max=327680

EXPECTED_ARGS=2
E_BADARGS=65

echo "...make boot.bin"
rm -f boot.bin
#dd if=/dev/zero of=boot.bin count=12224 bs=16
dd if=/dev/zero of=boot.bin count=$uboot_max bs=1
dd if=$BOOTDIR/u-boot.bin of=boot.bin conv=notrunc
#cat /home/mephisto/workspace/i8320/kernel/arch/arm/boot/uImage >> boot.bin
cat arch/arm/boot/uImage >> boot.bin
echo "...done"
