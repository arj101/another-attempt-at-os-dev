#!/bin/sh

echo $(zig build-obj kernel.c -target i386-freestanding)
echo $(nasm boot_sect.asm -f bin -o boot_sect.bin)
echo $(nasm kernel_entry.asm -f elf32 -o kernel_entry.o)
echo $(ld -o kernel.bin -Ttext 0x9000 kernel_entry.o kernel.o --oformat binary -m elf_i386)

cat boot_sect.bin kernel.bin > os-image
qemu-system-x86_64 os-image
