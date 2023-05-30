#pragma once
#include "./typedefs.h"
#include "./consts.h"
#include "./print_utils.h"


static inline void outb(u16 port, u8 val) {
    asm volatile("outb %0, %1": : "a" (val), "Nd" (port));
}

static inline u8 inb(u16 port) {
    u8 ret;
    asm volatile ("inb %1, %0": "=a" (ret) : "Nd" (port));
    return ret;
}

static inline void io_wait() { outb(0x80, 0); }

static inline void outl(u16 port, u32 val) {
    asm volatile("outl %0, %1": : "a" (val), "Nd" (port));
}

static inline u32 inl(u16 port) {
    u32 ret;
    asm volatile ("inl %1, %0": "=a" (ret) : "Nd" (port));
    return ret;
}

void enable_cursor(u8 cursor_start, u8 cursor_end);
void disable_cursor();
void update_cursor(u32 x, u32 y);
