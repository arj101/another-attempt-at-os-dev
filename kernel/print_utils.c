#pragma once
#include "./headers/print_utils.h"

#include <stdarg.h>

#include "./headers/typedefs.h"

#define PI 3.14159265359
#define PI_F2 (3.14159265359 / 2.0)

volatile struct VGATextScreen screen = {0, 0, 0x0f};

ATTRS void clear_screen() {
  for (u32 i = 0; i < VIDEO_ROW_LEN * VIDEO_COL_LEN * 2; i += 2) {
    VIDEO_MEM[i] = ' ';
    VIDEO_MEM[i + 1] = screen.style;
  }
  screen.row_offset = 0;
  screen.col_offset = 0;
}

ATTRS char char_at_xy(u8 x, u8 y) {
  return VIDEO_MEM[x * 2 + y * VIDEO_ROW_LEN * 2];
}

ATTRS char style_at_xy(u8 x, u8 y) {
  return VIDEO_MEM[x * 2 + y * VIDEO_ROW_LEN * 2 + 1];
}

ATTRS void scroll_up() {
  for (u32 i = 0; i < VIDEO_ROW_LEN * (VIDEO_COL_LEN - 1) * 2; i += 2) {
    VIDEO_MEM[i] = VIDEO_MEM[i + VIDEO_ROW_LEN * 2];
    VIDEO_MEM[i + 1] = VIDEO_MEM[i + 1 + VIDEO_ROW_LEN * 2];
  }
  for (u32 i = VIDEO_ROW_LEN * (VIDEO_COL_LEN - 1) * 2;
       i < VIDEO_ROW_LEN * VIDEO_COL_LEN * 2; i += 2) {
    VIDEO_MEM[i] = ' ';
    VIDEO_MEM[i + 1] = screen.style;
  }
  screen.row_offset = 0;
  screen.col_offset = VIDEO_ROW_LEN * (VIDEO_COL_LEN - 1) * 2;
}
__attribute__((no_caller_saved_registers)) void print_newline() {
  screen.row_offset = 0;
  screen.col_offset += VIDEO_ROW_LEN * 2;
  if (screen.col_offset >= VIDEO_ROW_LEN * VIDEO_COL_LEN * 2) scroll_up();
}

ATTRS inline void print_chr(char c) {
  if (c == '\n') {
    print_newline();
    return;
  }
  if (c == 0x08) {
    print_backspace();
    return;
  }

  VIDEO_MEM[screen.col_offset + screen.row_offset] = c;
  VIDEO_MEM[screen.col_offset + screen.row_offset + 1] = screen.style;
  screen.row_offset += 2;
  if (screen.row_offset >= VIDEO_ROW_LEN * 2) {
    screen.row_offset = 0;
    screen.col_offset += VIDEO_ROW_LEN * 2;
  }
  if (screen.col_offset >= VIDEO_ROW_LEN * VIDEO_COL_LEN * 2) scroll_up();
}

ATTRS void move_cursor_right() {
  if (screen.row_offset >= VIDEO_ROW_LEN * 2 - 2) {
    screen.row_offset = 0;
    move_cursor_down();
    return;
  }

  screen.row_offset += 2;
}

ATTRS void move_cursor_left() {
  if (screen.row_offset <= 0) {
    screen.row_offset = VIDEO_ROW_LEN * 2 - 2;
    move_cursor_up();
    return;
  }
  screen.row_offset -= 2;
}

ATTRS void move_cursor_up() {
  if (screen.col_offset < VIDEO_ROW_LEN * 2) {
    screen.col_offset = VIDEO_ROW_LEN * (VIDEO_COL_LEN - 1) * 2;
    return;
  }

  screen.col_offset -= VIDEO_ROW_LEN * 2;
}

ATTRS void move_cursor_down() {
  if (screen.col_offset >= VIDEO_ROW_LEN * (VIDEO_COL_LEN - 1) * 2) {
    screen.col_offset = 0;
    return;
  }
  screen.col_offset += VIDEO_ROW_LEN * 2;
}

ATTRS void print_backspace() {
  if (screen.col_offset <= 0 && screen.row_offset <= 0) return;
  if (screen.row_offset <= 0) {
    screen.col_offset -= VIDEO_ROW_LEN * 2;
    screen.row_offset = VIDEO_ROW_LEN * 2 - 2;
  } else {
    screen.row_offset -= 2;
  }

  VIDEO_MEM[screen.col_offset + screen.row_offset] = ' ';
  VIDEO_MEM[screen.col_offset + screen.row_offset + 1] = screen.style;
}

ATTRS void print_str(char *s) {
  for (u32 i = 0; s[i] != '\0'; i++) {
    print_chr(s[i]);
  }
}

ATTRS void print_unum(u32 num) {
  u32 start = screen.row_offset + screen.col_offset;
  u32 len = 0;
  u32 num_cpy = num;
  for (; num_cpy > 0; num_cpy /= 10, len += 2)
    ;
  u32 i = 0;
  if (num != 0) len -= 2;  // FIXME: why is this necessary?
  while (num > 0 || i == 0) {
    u8 digit = num % 10;
    VIDEO_MEM[start + len - i] = '0' + digit;
    VIDEO_MEM[start + len - i + 1] = screen.style;

    num /= 10;
    i += 2;
    screen.row_offset += 2;
    if (screen.row_offset >= VIDEO_ROW_LEN * 2) {
      screen.row_offset = 0;
      screen.col_offset += VIDEO_ROW_LEN * 2;
    }
    if (screen.col_offset >= VIDEO_ROW_LEN * VIDEO_COL_LEN * 2) {
      scroll_up();
      start -= VIDEO_ROW_LEN * 2;
    }
  }
}

ATTRS void print_u64(u64 num) {
  u32 start = screen.row_offset + screen.col_offset;
  u32 len = 0;
  u64 num_cpy = num;
  for (; num_cpy > 0; num_cpy /= 10, len += 2)
    ;
  u64 i = 0;
  if (num != 0) len -= 2;  // FIXME: why is this necessary?
  while (num > 0 || i == 0) {
    u8 digit = num % 10;
    VIDEO_MEM[start + len - i] = '0' + digit;
    VIDEO_MEM[start + len - i + 1] = screen.style;

    num /= 10;
    i += 2;
    screen.row_offset += 2;
    if (screen.row_offset >= VIDEO_ROW_LEN * 2) {
      screen.row_offset = 0;
      screen.col_offset += VIDEO_ROW_LEN * 2;
    }
    if (screen.col_offset >= VIDEO_ROW_LEN * VIDEO_COL_LEN * 2) {
      scroll_up();
      start -= VIDEO_ROW_LEN * 2;
    }
  }
}

ATTRS u32 pow(u32 num, u32 pow) {
  u32 x = num;
  for (u32 i = 0; i < pow; i++) {
    x *= num;
  }
  return x;
}

ATTRS void print_f32_fixed(f32 num, u32 dec_count) {
  i32 num_nondec = (i32)num;
  f32 dec = (f32)num_nondec - num;
  if (dec < 0) dec *= -1.0;
  print_unum(num_nondec);
  print_chr('.');
  //
  for (u32 i = 0; i < dec_count; i++) {
    dec *= 10.0;
    u32 decN = (u32)dec;
    decN %= 10;
    print_unum(decN);
  }
}

ATTRS void print_f32(f32 num) {
  i32 num_nondec = (i32)num;
  f32 dec = (f32)num_nondec - num;
  if (dec < 0) dec *= -1.0;

  print_unum(num_nondec);
  print_chr('.');

  char buf[F32_PRECISION + 1];
  u32 len = F32_PRECISION + 1;
  u32 decN;
  for (u32 i = 0; i < F32_PRECISION; i++) {
    dec *= 10.0;
    decN = (u32)dec;
    decN %= 10;
    buf[i] = '0' + (char)decN;
  }
  buf[F32_PRECISION] = '\0';
  for (u32 i = F32_PRECISION; i > 0; i--) {
    if (buf[i] == '0') buf[i] = '\0';
  }
  print_str(buf);
}

ATTRS void print_f64(f64 num) {
  i64 num_nondec = (i64)num;
  f64 dec = (f64)num_nondec - num;
  if (dec < 0) dec *= -1.0;

  print_unum(num_nondec);
  print_chr('.');

  char buf[F64_PRECISION + 1];
  u32 len = F64_PRECISION + 1;
  u32 decN;
  for (u32 i = 0; i < F64_PRECISION; i++) {
    dec *= 10.0;
    decN = (u32)dec;
    decN %= 10;
    buf[i] = '0' + (char)decN;
  }
  buf[F64_PRECISION] = '\0';
  for (u32 i = F64_PRECISION; i > 0; i--) {
    if (buf[i] == '0') buf[i] = '\0';
  }
  print_str(buf);
}

ATTRS void print_hex_u64(u64 num) {
  print_str("0x");
  u8 part = 0;
  char s = '0';
  for (i32 i = 15; i >= 0; i--) {
    part = (num & ((u64)0xf << (4 * i))) >> (4 * i);
    // TODO: declare a mask variable and right shift on each iteration
    //  instead of this mess
    if (part <= 9) {
      s = '0' + part;
    } else {
      s = 'a' + part - 10;
    }
    print_chr(s);
  }
}

ATTRS void print_hex_u32(u32 num) {
  print_str("0x");
  u8 part = 0;
  char s = '0';
  for (i32 i = 7; i >= 0; i--) {
    // TODO: declare a mask variable and right shift on each iteration
    //  instead of this mess
    part = (num & ((u64)0xf << (4 * i))) >> (4 * i);
    if (part <= 9) {
      s = '0' + part;
    } else {
      s = 'a' + part - 10;
    }
    print_chr(s);
  }
}

ATTRS void print_hex_u16(u16 num) {
  print_str("0x");
  u8 part = 0;
  char s = '0';
  for (i32 i = 3; i >= 0; i--) {
    // TODO: declare a mask variable and right shift on each iteration
    //  instead of this mess
    part = (num & ((u64)0xf << (4 * i))) >> (4 * i);
    if (part <= 9) {
      s = '0' + part;
    } else {
      s = 'a' + part - 10;
    }
    print_chr(s);
  }
}

ATTRS void print_hex_byte(u8 num) {
  print_str("0x");
  u8 part = 0;
  char s = '0';
  for (i32 i = 1; i >= 0; i--) {
    part = (num & ((u64)0xf << (4 * i))) >> (4 * i);
    // TODO: declare a mask variable and right shift on each iteration
    //  instead of this mess
    if (part <= 9) {
      s = '0' + part;
    } else {
      s = 'a' + part - 10;
    }
    print_chr(s);
  }
}

ATTRS void print_bin_byte(u8 num) {
  print_str("0b");
  u8 part = 0;
  u8 mask = 0b1 << 7;
  for (i32 i = 7; i >= 0; i--, mask >>= 1) {
    part = num & mask;
    if (part == 0)
      print_str("0\0");  // FIXME:weird bug: displays "c" without \0
    else
      print_chr('1');
  }
}

ATTRS void set_print_style(u8 style) { screen.style = style; }

ATTRS void print_bool(u8 bool) {
  if (bool)
    print_str("true");
  else
    print_str("false");
}

ATTRS void print_inum(i32 num) {
  if (num >= 0) {
    print_unum((u32)num);
  } else {
    VIDEO_MEM[screen.row_offset + screen.col_offset] = '-';
    VIDEO_MEM[screen.row_offset + screen.col_offset + 1] = screen.style;
    screen.row_offset += 2;
    if (screen.row_offset >= VIDEO_ROW_LEN * 2) {
      screen.row_offset = 0;
      screen.col_offset += VIDEO_ROW_LEN * 2;
    }
    if (screen.col_offset >= VIDEO_ROW_LEN * VIDEO_COL_LEN * 2) scroll_up();

    print_unum((u32)(-1 * num));
  }
}

ATTRS void move_cursor_to(u8 x, u8 y) {
  screen.col_offset = VIDEO_ROW_LEN * 2 * y;
  screen.row_offset = x * 2;
}

ATTRS void get_cursor_coord(u8 *x, u8 *y) {
  *x = screen.row_offset / 2;
  *y = screen.col_offset / (VIDEO_ROW_LEN * 2);
}

ATTRS void clear_row(u8 y) {
  for (u32 i = VIDEO_ROW_LEN * 2 * (y + 1); i < VIDEO_ROW_LEN * 2 * (y + 2);
       i += 2) {
    VIDEO_MEM[i] = ' ';
    VIDEO_MEM[i + 1] = screen.style;
  }
  screen.row_offset = 0;
}

i8 char_to_num(char c) {
  c = (i8)c;

  if (c - '0' >= 0 && c - '0' <= 9) {
    return c - '0';
  }

  return -1;
}

/// %<n>f -> print_float(f, n)
/// %n    -> print_inum(n)
/// %u    -> print_unum(u)
/// %<y>x -> print_hex_<y>(x)
/// %b    -> print_bin_byte(b)
/// %s    -> print_str(s)
/// %c    -> print_chr(c)
ATTRS void printf(const char *s, ...) {
  va_list args;
  va_start(args, s);

  for (i32 i = 0; s[i] != '\0'; i++) {
    if (s[i] != '%') {
      print_chr(s[i]);
      continue;
    } else if (s[i + 1] != '\0') {
      switch (s[i + 1]) {
        case 'c':
          print_chr((char)va_arg(args, i32));
          break;
        case 's':
          print_str(va_arg(args, char *));
          break;
        case 'b':
          print_bin_byte((u8)va_arg(args, int));
          break;
        case 'n':
          print_inum(va_arg(args, i32));
          break;
        case 'u':
          print_unum(va_arg(args, u32));
          break;
        case 'f':
          print_f32((f32)va_arg(args, f64));
          break;
        // case 'd': print_f32_fixed((float)va_arg(args, double), (u32) (n > -1
        // ? n : 7)); break;
        case 'd':
          print_f64(va_arg(args, f64));
          break;
        case 'x':
          print_hex_u32(va_arg(args, u32));
          break;
        case 'a':
          print_u64(va_arg(args, u64));
          break;
        case 'B':
          print_bool((u8)va_arg(args, u32));
          break;
        case 'X':
          print_hex_u16((u16)va_arg(args, u32));
          break;
        default:
          print_chr('%');
          print_chr(s[i + 1]);
      }
      i++;
      continue;
    }
    print_chr('%');
  }
  va_end(args);
}
