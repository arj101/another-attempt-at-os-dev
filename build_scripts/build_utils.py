import os
import sys
import time
from watchdog.observers import Observer
from watchdog.events import PatternMatchingEventHandler
from PyQt5 import QtCore
import subprocess
from subprocess import PIPE

had_error = False


def wait_for_process(subprocess):
    p = subprocess.poll()
    if p is None:
        while p is None:
            p = subprocess.poll()
            if p is not None:
                return p
    else:
        return p


def exit_on_error(exit_code, msg):
    global had_error
    if exit_code != 0:
        print(msg)
        had_error = True


def build():
    global had_error
    print(":: building...")
    zig_process = subprocess.Popen(
        "zig build-obj ./kernel/*.c -target i386-freestanding -I ./headers -femit-bin=./build/kernel.o", shell=True, )

    nasm_bootsect = subprocess.Popen(
        "nasm boot_sect.asm -f bin -o ./build/boot_sect.bin", shell=True,)

    nasm_kernelentry = subprocess.Popen(
        "nasm kernel_entry.asm -f elf32 -o ./build/kernel_entry.o", shell=True, )

    exit_code = zig_process.poll()
    if exit_code is None:
        print(":: waiting for 'zig build-obj'")
        exit_code = wait_for_process(zig_process)

    exit_on_error(exit_code, ":: kernel compilation failed.")

    exit_code = nasm_bootsect.poll()
    if exit_code is None:
        print(":: waiting for 'nasm boot_sect'")
        wait_for_process(nasm_bootsect)

    exit_on_error(exit_code, ":: boot sector compilation failed.")

    exit_code = nasm_kernelentry.poll()
    if exit_code is None:
        print(":: waiting for 'nasm kernel_entry'")
        wait_for_process(nasm_kernelentry)
    exit_on_error(exit_code, ":: kernel entry compilation failed.")
    if had_error:
        print(":: exiting due to previous error(s)")
        sys.exit(1)
    exit_code = os.system(
        "ld -o ./build/kernel.bin -Ttext 0x9000 ./build/kernel_entry.o ./build/kernel.o ./build/libgcc.a --oformat binary -m elf_i386")

    exit_on_error(exit_code, ":: linking failed.")

    if had_error:
        print(":: exiting due to previous error(s)")
        sys.exit(1)
    os.system("cat ./build/boot_sect.bin ./build/kernel.bin > ./build/os-image")


def run():
    print(":: running...")
    print(":: starting qemu...")
    # os.system("cp ./build/os-image ./build/os-image-copy"); #create a copy of the disk image to be used as SATA AHCI disk
    qemu = subprocess.Popen("exec qemu-system-x86_64 -drive file=./build/os-image,format=raw \
            -drive id=disk,file=./build/os-image-copy,if=none\
            -device ahci,id=ahci\
            -device ide-hd,drive=disk,bus=ahci.0 ", shell=True, )

    try:
        while True:
            if qemu.poll() is not None:
                break
    except KeyboardInterrupt:
        print(":: received interrupt")
    print(":: exiting...")
