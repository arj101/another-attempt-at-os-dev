#include "./headers/apic.h"

u8 lapic_present() {
  u32 cpuid_read;
  asm("mov $1, %%eax;"
      "cpuid;"
      : "=d"(cpuid_read));

  return (cpuid_read & LAPIC_BIT) >> 9;
}

u32 lapic_base() {
  u32 lo;
  asm("rdmsr" : "=a"(lo) : "c"(IA32_APIC_BASE));

  return lo & 0xfffff000;
}

u32 lapic_id(u32 apic_base) { return apic_read(apic_base, 0x020); }

void reload_lapic_timer(u32 lapic_base, u32 val) {
  apic_write(lapic_base, 0x380, val);
}

void set_lapic_timer_prescaler(u32 lapic_base, enum LAPICTimerPrescaler val) {
  apic_write(lapic_base, 0x3e0, val);
}

void set_lapic_timer_lvt(u32 lapic_base, u8 vector,
                         enum LapicTimerMode timer_mode) {
  u32 w_val = vector;
  w_val |= 1 << 16;  // masked by default
  w_val |= timer_mode << 17;

  apic_write(lapic_base, LVT_OFFSET_TIMER, w_val);
}

void set_lapic_int_mask(u32 lapic_base, u32 lvt_offset,
                        enum LapicIntMask mask) {
  u32 lvt_val = apic_read(lapic_base, lvt_offset);
  if (mask)
    lvt_val |= 1 << 16;
  else
    lvt_val &= ~(1 << 16);
  apic_write(lapic_base, lvt_offset, lvt_val);
}
