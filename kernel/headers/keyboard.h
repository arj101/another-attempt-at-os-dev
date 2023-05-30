#pragma once
#include "./idt.h"
#include "./typedefs.h"
#include "./idt.h"
#include "./utils.h"
#include "./typedefs.h"
#include "./print_utils.h"

#define APIC_READ(addr, var) var = *(volatile u32*)(apic_base + addr);
#define APIC_WRITE(addr, val) *(volatile u32*)(apic_base + addr) = val;


#define PS2_DATAPORT 0x60
#define PS2_STATUSREGISTER 0x64
#define PS2_CMDREGISTER 0x64

#define KEYQUEUE_SIZE 128

struct KeyQueue {
  char queue[KEYQUEUE_SIZE];
  u32 start;
  u32 length;
};

void clear_key_queue(struct KeyQueue* queue);
u8 key_queue_isfull(struct KeyQueue* queue);
u8 key_queue_isempty(struct KeyQueue* queue);
void push_key_queue(struct KeyQueue* queue, char value);
char pop_key_queue(struct KeyQueue* queue);

static u32 apic_base;
volatile struct Keyboard {
  u8 shift : 1;
  u8 ctrl : 1;
  u8 num_lock : 1;
  u8 scroll_lock : 1;
  u8 caps_lock : 1;
  u8 extended_mode : 1;
  struct KeyQueue key_queue;
};
struct Keyboard* init_ps2_keyboard();


static struct Keyboard ps2_keyboard;
char scan_code_to_char(int scan_code, u8 shift);

void ps2_keyboard_int(u8 code);


