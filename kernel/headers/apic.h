#pragma once
#include "./typedefs.h"

#define APIC_READ(addr, var) var = *(volatile u32*)(apic_base + addr);
#define APIC_WRITE(addr, val) *(volatile u32*)(apic_base + addr) = val;

#define LAPIC_BIT 0b1 << 9
#define IA32_APIC_BASE 0x1B

#define LVT_OFFSET_TIMER 0x320
#define LVT_OFFSET_CMCI  0x2F0
#define LVT_OFFSET_LINT0 0x350
#define LVT_OFFSET_LINT1 0x360
#define LVT_OFFSET_ERROR 0x370
#define LVT_OFFSET_PERFCOUNTERS 0x340
#define LVT_OFFSET_THERMAL 0x330

enum LAPICTimerPrescaler {
    Div2 = 0b0000,
    Div4 = 0b0001,
    Div8 = 0b0010,
    Div16 = 0b0011,
    Div32 = 0b1000,
    Div64 = 0b1001,
    Div128 = 0b1010,
    NoScale = 0b1011,
};

static inline void ioapic_write(u32 base, u8 offset, u32 val) {
    *(volatile u32*)base = offset;
    *(volatile u32*)(base + 0x10) = val;
}

static inline u32 ioapic_read(u32 base, u8 offset) {
    *(volatile u32*)base = offset;
    return *(volatile u32*)(base + 0x10);
}

static inline void apic_write(u32 base, u32 addr, u32 val) {
    *(volatile u32*)(base + addr) = val;
}

static inline u32 apic_read(u32 base, u32 addr) {
    return *(volatile u32*)(base + addr);
}

u32 lapic_base();
u8 lapic_present();
u32 lapic_id(u32 apic_base);

void reload_lapic_timer(u32 lapic_base, u32 val);

void set_lapic_timer_prescaler(u32 lapic_base, enum LAPICTimerPrescaler val);

enum LapicIntDeliverMode {
    Fixed = 0,
    SMI = 0b010,
    NMI = 0b100,
    INIT = 0b101,
    ExtINT = 0b1111,
};

enum LapicIntInputPolarity {
    ActiveHigh = 0,
    ActiveLow = 1,
};

enum LapicIntTriggerMode {
                       Edge = 0,
                       Level = 1,
                   };

enum LapicTimerMode {
                        OneShot = 0,
                        Periodic = 0b01,
                        TSCDeadline = 0b10,
                };

void set_lapic_timer_lvt(u32 lapic_base, u8 vector, enum LapicTimerMode timer_mode);
enum LapicIntMask { Unmasked = 0, Masked = 1 };
void set_lapic_int_mask(u32 lapic_base, u32 lvt_offset, enum LapicIntMask mask);


