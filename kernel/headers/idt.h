#pragma once
#include "./typedefs.h"
#include "./consts.h"

struct InterruptFrame {
  u32 eip;
  u32 cs;
  u32 eflags;
  u32 esp;
  u32 ss;
};

struct InterruptFrame_ErrorCode {
  u32 error_code;
  u32 eip;
  u32 cs;
  u32 eflags;
  u32 esp;
  u32 ss;
};


struct __attribute__((packed)) InterruptDescriptor {
    u16 offset_0;
    u16 segment_selector;
    u8 _zeroes; //reserved bits
    u8 type_attributes;
    u16 offset_1;
};

struct __attribute__((packed)) IDT {
    struct InterruptDescriptor descriptors[256];
};

struct __attribute__((packed)) IDTDescriptor {
    u16 size;
    u32 offset;
};

__attribute__((aligned(0x10)))
static struct IDT idt;
__attribute__((aligned(0x10)))
static struct IDTDescriptor idt_desc;

void idt_set_descriptor(u8 vector, void* isr, u8 flags);
void load_idt();
void set_idt_desc();

struct IDTDescriptor* get_idt_desc();
struct IDT* get_idt();
