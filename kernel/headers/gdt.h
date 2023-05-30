#pragma once
#include "./typedefs.h"

//TODO: name each bits
struct __attribute__((packed)) SegmentDescriptor {
    u16 limit_0; //bits 0-15
    u16 base_0; //bits 0-15
    u8 base_1; //bits 16-23
    u8 flags_0; //1st flags, type flags
    u8 limit_1:4; //limti(bits 16-19)
    u8 flags_1:4; //2nd flags
    u8 base_2; //bits 24-31
};

struct __attribute__((packed)) GDT {
    u64 null_descriptor;
    struct SegmentDescriptor descriptors[2];
};

struct __attribute__((packed)) GDTDescriptor { 
    u16 size; //one less than the actual size
    u32 offset;
};

static struct GDTDescriptor gdt_desc;
