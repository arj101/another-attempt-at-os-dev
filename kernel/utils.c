#include "./headers/utils.h"
// following 3 functions shamelessly copied from OS dev. wiki
// I have no idea how they work
void enable_cursor(u8 cursor_start, u8 cursor_end) {
  outb(0x3D4, 0x0A);
  outb(0x3D5, (inb(0x3D5) & 0xC0) | cursor_start);

  outb(0x3D4, 0x0B);
  outb(0x3D5, (inb(0x3D5) & 0xE0) | cursor_end);
}

void disable_cursor() {
  outb(0x3D4, 0x0A);
  outb(0x3D5, 0x20);
}

void update_cursor(u32 x, u32 y) {
  u16 pos = y * VIDEO_ROW_LEN + x;

  outb(0x3D4, 0x0F);
  outb(0x3D5, (u8)(pos & 0xFF));
  outb(0x3D4, 0x0E);
  outb(0x3D5, (u8)((pos >> 8) & 0xFF));
}
