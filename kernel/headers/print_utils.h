#pragma once
#include "./typedefs.h"

#define ATTRS __attribute__((no_caller_saved_registers))
#define NCSR ATTRS

#define VIDEO_MEM  ((char*)0xb8000)
#define VIDEO_ROW_LEN  80
#define VIDEO_COL_LEN  25

#define F32_PRECISION 9
#define F64_PRECISION 9
//FIXME: apparently this is the maximum it can go without triggering a #GP

#define TEXT_COLOR_BLACK    0
#define TEXT_COLOR_BLUE     1
#define TEXT_COLOR_GREEN    2
#define TEXT_COLOR_CYAN     3
#define TEXT_COLOR_RED      4
#define TEXT_COLOR_MAGENTA  5
#define TEXT_COLOR_BROWN    6
#define TEXT_COLOR_LGRAY    7

#define TEXT_COLOR_BRIGHT   8

#define TEXT_BG_BLACK       TEXT_COLOR_BLACK    << 4
#define TEXT_BG_BLUE        TEXT_COLOR_BLUE     << 4
#define TEXT_BG_GREEN       TEXT_COLOR_GREEN    << 4
#define TEXT_BG_CYAN        TEXT_COLOR_CYAN     << 4
#define TEXT_BG_RED         TEXT_COLOR_RED      << 4
#define TEXT_BG_MAGENTA     TEXT_COLOR_MAGENTA  << 4
#define TEXT_BG_BROWN       TEXT_COLOR_BROWN    << 4
#define TEXT_BG_LGRAY       TEXT_COLOR_LGRAY    << 4

#define TEXT_BG_BRIGHT      8

#define TEXT_COLOR_WHITE    TEXT_COLOR_LGRAY + TEXT_COLOR_BRIGHT
#define TEXT_BG_WHITE       TEXT_BG_LGRAY    + TEXT_BG_BRIGHT


struct VGATextScreen {
    u32 col_offset;
    u32 row_offset;
    u8 style;
};

NCSR void scroll_up();
NCSR void clear_screen();
NCSR void print_newline();
NCSR void print_str(char* s);
NCSR void print_unum(u32 num);
NCSR u32 pow();
NCSR void print_f32_fixed(f32 num, u32 dec_count);
NCSR void print_hex_u64(u64 num);
NCSR void print_hex_u32(u32 num);
NCSR void print_hex_u16(u16 num);
NCSR void print_hex_byte(u8 num);
NCSR void print_bin_byte(u8 num);
NCSR void set_print_style(u8 style);
NCSR void print_inum(i32 num);
NCSR void print_chr(char c);
NCSR void printf(const char* s,...);
NCSR void print_f32(f32 num);
NCSR void print_f64(f64 num);
NCSR void print_u64(u64 num);
NCSR void move_cursor_to(u8 x, u8 y);
NCSR void get_cursor_coord(u8* x, u8* y);
NCSR void clear_row(u8 y);
NCSR void print_backspace();
NCSR void move_cursor_up();
NCSR void move_cursor_down();
NCSR void move_cursor_right();//TODO
NCSR void move_cursor_left();//TODO
NCSR char char_at_xy(u8 x, u8 y);
NCSR char style_at_xy(u8 x, u8 y);
NCSR void print_bool(u8 bool);
