#include "./headers/idt.h"

void idt_set_descriptor(u8 vector, void *isr, u8 flags) {
  struct InterruptDescriptor *desc = &idt.descriptors[vector];
  desc->_zeroes = 0;

  desc->offset_0 = (u32)isr & 0xffff;
  desc->offset_1 = (u32)isr >> 16;

  desc->segment_selector = GDT__CODESEG_OFFSET;
  desc->type_attributes = flags;
}

void load_idt() {
  asm volatile(
      "cli;"
      "lidt (idt_desc);"
      "sti;");
}

void set_idt_desc() {
  idt_desc.offset = (u32)&idt;
  idt_desc.size = 2047;
}

struct IDTDescriptor *get_idt_desc() { return &idt_desc; }

struct IDT *get_idt() { return &idt; }
