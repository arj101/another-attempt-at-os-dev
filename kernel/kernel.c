#include "./headers/apic.h"
#include "./headers/consts.h"
#include "./headers/gdt.h"
#include "./headers/idt.h"
#include "./headers/keyboard.h"
#include "./headers/pci.h"
#include "./headers/print_utils.h"
#include "./headers/typedefs.h"
#include "./headers/utils.h"
#include "stdalign.h"

u32 ioapic_base = 0xFEC00000;

__attribute__((interrupt)) void isr1(struct InterruptFrame *frame);
__attribute__((interrupt)) void exception_handler(struct InterruptFrame *frame);
__attribute__((interrupt)) void exception_handler__error_code(
    struct InterruptFrame_ErrorCode *frame);
__attribute__((interrupt)) void exception_gp(
    struct InterruptFrame_ErrorCode *frame);
void delay();

__attribute__((interrupt)) void exception_double_falt(
    struct InterruptFrame_ErrorCode *frame) {
  print_str("\nDouble Falt!\nError code:");
  print_unum(frame->eip);
  __asm__ __volatile__("cli; hlt;");
}

u8 initialise_fpu();
u8 initialise_sse();

volatile u32 apic_timer_int_count = 0;

struct __attribute((packed)) IOAPICRedirectionEntry {
  u32 low_bits;
  u32 high_bits;
};

u32 create_ioapic_rdentry_flags(u8 delivery_mode, u8 destination_mode,
                                u8 input_polarity, u8 trigger_mode, u8 mask) {
  u32 flag = delivery_mode;
  flag |= destination_mode << 3;
  flag |= input_polarity << 5;
  flag |= trigger_mode << 7;
  flag |= mask << 8;

  return flag;
}

struct IOAPICRedirectionEntry create_ioapic_rd_entry(u8 vector, u32 flags,
                                                     u8 destination) {
  struct IOAPICRedirectionEntry entry;
  entry.low_bits = vector;
  entry.low_bits |= (flags << 8);
  entry.high_bits = (destination << 24);

  return entry;
};

void write_ioapic_rdtable(u8 irq, struct IOAPICRedirectionEntry entry) {
#define RDTABLE_START 0x10
  u32 base_addr = irq * 2 + RDTABLE_START;
  ioapic_write(ioapic_base, base_addr, entry.low_bits);
  ioapic_write(ioapic_base, base_addr + 1, entry.high_bits);
};

__attribute((interrupt)) void apic_timer(struct InterruptFrame *frame) {
  apic_timer_int_count++;
  apic_write(apic_base, 0x0b0, 32);
}

u32 ioapic_keyboard_int_count = 0;
u32 set = 0;
u8 extended_byte = false;

volatile u32 pit_int_count = 0;
__attribute__((interrupt)) void pit_timer_interrupt(
    struct InterruptFrame *frame) {
  pit_int_count++;
  APIC_WRITE(0x0b0, 33);
}

__attribute__((interrupt)) void unhandled_interrupt(
    struct InterruptFrame *frame) {
  printf("\n Unhandled interrupt triggered :(");
  while (true) {}
  APIC_WRITE(0x0b0, 33);
}

__attribute__((interrupt)) void ioapic_keyboard_int_stub(struct InterruptFrame* frame) {
  u8 code = inb(PS2_DATAPORT);
  printf(" [ %x ] ", code);
  if (code == 0xEE || code == 0xFE || code == 0xFA) return;
  ioapic_keyboard_int_count++;
  ps2_keyboard_int(code);
  APIC_WRITE(0x0b0, 33);
}

#define EXCEPTION_HANDLER(vector)                               \
  __attribute__((interrupt)) void exception_##vector##_handler( \
      struct InterruptFrame *frame) {                           \
    printf("\nEXCEPTION! :( [vector ");                         \
    printf(#vector);                                            \
    printf("]  (at %x)", frame->eip);                           \
    asm volatile("cli;hlt");                                    \
  }

#define EXCEPTION_HANDLER__ERROR_CODE(vector)                           \
  __attribute__((interrupt)) void exception_##vector##_handler(         \
      struct InterruptFrame_ErrorCode *frame) {                         \
    printf("\nEXCEPTION! :( [vector ");                                 \
    printf(#vector);                                                    \
    printf("] (error code: %x, at %x)", frame->error_code, frame->eip); \
    asm volatile("cli;hlt");                                            \
  }

EXCEPTION_HANDLER(0);
EXCEPTION_HANDLER(1);
EXCEPTION_HANDLER(2);
EXCEPTION_HANDLER(3);
EXCEPTION_HANDLER(4);
EXCEPTION_HANDLER(5);
EXCEPTION_HANDLER(6);
EXCEPTION_HANDLER(7);
EXCEPTION_HANDLER(8);
EXCEPTION_HANDLER(9);
EXCEPTION_HANDLER__ERROR_CODE(10);
EXCEPTION_HANDLER(11);
EXCEPTION_HANDLER(12);
EXCEPTION_HANDLER(13);
EXCEPTION_HANDLER(14);
EXCEPTION_HANDLER(15);
EXCEPTION_HANDLER(16);
EXCEPTION_HANDLER(17);
EXCEPTION_HANDLER(18);
EXCEPTION_HANDLER(19);
EXCEPTION_HANDLER(20);
EXCEPTION_HANDLER(21);
EXCEPTION_HANDLER(22);
EXCEPTION_HANDLER(23);
EXCEPTION_HANDLER(24);
EXCEPTION_HANDLER(25);
EXCEPTION_HANDLER(26);
EXCEPTION_HANDLER(27);
EXCEPTION_HANDLER(28);
EXCEPTION_HANDLER(29);
EXCEPTION_HANDLER(30);
EXCEPTION_HANDLER(31);

#define AHCI_BASE 0x400000  // 4M

#define HBA_PxCMD_ST 0x0001
#define HBA_PxCMD_FRE 0x0010
#define HBA_PxCMD_FR 0x4000
#define HBA_PxCMD_CR 0x8000
void start_cmd(HBA_PORT *port) {
  // Wait until CR (bit15) is cleared
  while (port->cmd & HBA_PxCMD_CR)
    ;

  // Set FRE (bit4) and ST (bit0)
  port->cmd |= HBA_PxCMD_FRE;
  port->cmd |= HBA_PxCMD_ST;
}

// Stop command engine
void stop_cmd(HBA_PORT *port) {
  // Clear ST (bit0)
  port->cmd &= ~HBA_PxCMD_ST;

  // Clear FRE (bit4)
  port->cmd &= ~HBA_PxCMD_FRE;

  // Wait until FR (bit14), CR (bit15) are cleared
  while (1) {
    if (port->cmd & HBA_PxCMD_FR) continue;
    if (port->cmd & HBA_PxCMD_CR) continue;
    break;
  }
}

void memset(void *data, u32 start, u32 end) {
  for (u32 i = start; i < end; i++) ((u8 *)data)[i] = 0;
}

void port_rebase(HBA_PORT *port, int portno) {
  stop_cmd(port);  // Stop command engine

  // Command list offset: 1K*portno
  // Command list entry size = 32
  // Command list entry maxim count = 32
  // Command list maxim size = 32*32 = 1K per port
  port->clb = AHCI_BASE + (portno << 10);
  port->clbu = 0;
  memset((void *)(port->clb), 0, 1024);

  // FIS offset: 32K+256*portno
  // FIS entry size = 256 bytes per port
  port->fb = AHCI_BASE + (32 << 10) + (portno << 8);
  port->fbu = 0;
  memset((void *)(port->fb), 0, 256);

  // Command table offset: 40K + 8K*portno
  // Command table size = 256*32 = 8K per port
  HBA_CMD_HEADER *cmdheader = (HBA_CMD_HEADER *)(port->clb);
  for (int i = 0; i < 32; i++) {
    cmdheader[i].prdtl = 8;  // 8 prdt entries per command table
                             // 256 bytes per command table, 64+16+48+16*8
    // Command table offset: 40K + 8K*portno + cmdheader_index*256
    cmdheader[i].ctba = AHCI_BASE + (40 << 10) + (portno << 13) + (i << 8);
    cmdheader[i].ctbau = 0;
    memset((void *)cmdheader[i].ctba, 0, 256);
  }

  start_cmd(port);  // Start command engine
}

void _start() {
  clear_screen();
  u8 initialised_fpu = initialise_fpu();
  if (initialised_fpu == 0) print_str("Failed to initialise fpu :(\n");

  u8 _initialised_sse = initialise_sse();

  asm volatile("sgdt (gdt_desc)");

#define SET_EXCEPTION_DEFAULT_VECTOR(vector) \
  idt_set_descriptor(vector, &exception_##vector##_handler, 0x8E);

  // TODO: set all 256 descriptors
  //  for (u32 vector = 0;  vector < 32; vector++) {
  //      idt_set_descriptor(vector,&exception_handler, 0x8E);
  //  }
  //
  SET_EXCEPTION_DEFAULT_VECTOR(0);
  SET_EXCEPTION_DEFAULT_VECTOR(1);
  SET_EXCEPTION_DEFAULT_VECTOR(2);
  SET_EXCEPTION_DEFAULT_VECTOR(3);
  SET_EXCEPTION_DEFAULT_VECTOR(4);
  SET_EXCEPTION_DEFAULT_VECTOR(5);
  SET_EXCEPTION_DEFAULT_VECTOR(6);
  SET_EXCEPTION_DEFAULT_VECTOR(7);
  SET_EXCEPTION_DEFAULT_VECTOR(8);
  SET_EXCEPTION_DEFAULT_VECTOR(9);
  SET_EXCEPTION_DEFAULT_VECTOR(10);
  SET_EXCEPTION_DEFAULT_VECTOR(11);
  SET_EXCEPTION_DEFAULT_VECTOR(12);
  SET_EXCEPTION_DEFAULT_VECTOR(13);
  SET_EXCEPTION_DEFAULT_VECTOR(14);
  SET_EXCEPTION_DEFAULT_VECTOR(15);
  SET_EXCEPTION_DEFAULT_VECTOR(16);
  SET_EXCEPTION_DEFAULT_VECTOR(17);
  SET_EXCEPTION_DEFAULT_VECTOR(18);
  SET_EXCEPTION_DEFAULT_VECTOR(19);
  SET_EXCEPTION_DEFAULT_VECTOR(20);
  SET_EXCEPTION_DEFAULT_VECTOR(21);
  SET_EXCEPTION_DEFAULT_VECTOR(22);
  SET_EXCEPTION_DEFAULT_VECTOR(23);
  SET_EXCEPTION_DEFAULT_VECTOR(24);
  SET_EXCEPTION_DEFAULT_VECTOR(25);
  SET_EXCEPTION_DEFAULT_VECTOR(26);
  SET_EXCEPTION_DEFAULT_VECTOR(27);
  SET_EXCEPTION_DEFAULT_VECTOR(28);
  SET_EXCEPTION_DEFAULT_VECTOR(29);
  SET_EXCEPTION_DEFAULT_VECTOR(30);
  SET_EXCEPTION_DEFAULT_VECTOR(31);

  for (u32 vector = 32; vector < 256; vector++) {
    idt_set_descriptor(vector, &unhandled_interrupt, 0x8E);
  }


  idt_set_descriptor(8, &exception_double_falt, 0x8E);
  idt_set_descriptor(13, &exception_gp, 0x8E);
  idt_set_descriptor(32, &apic_timer, 0x8E);
  idt_set_descriptor(33, &ioapic_keyboard_int_stub, 0x8E);
  idt_set_descriptor(34, &pit_timer_interrupt, 0x8E);

  struct Keyboard* keyboard = init_ps2_keyboard();
  set_idt_desc();
  load_idt();
  // print_str("Loaded IDT and enabled interrupts.\n");
  printf("GDT offset: %x, ", gdt_desc.offset);

  printf("IDT offset: %x\n", get_idt_desc()->offset);
  //
  // outb(PS2_CMDREGISTER, 0xAD);
  // outb(PS2_CMDREGISTER, 0xA7);
  // outb(PS2_CMDREGISTER, 0xFF);
  // io_wait();
  outb(PS2_CMDREGISTER, 0xF0);
  // outb(PS2_CMDREGISTER, 0x0);
  io_wait();

  u32 i = 01;
  u32 j = 49;
  float a = 0.10;
  float b = 1.030;
  float c = 0.0;

  asm("flds %1;"
      "flds %2;"
      "fdiv;"
      "fstps %0"
      : "=g"(c)
      : "g"(a), "g"(b));

  printf("%f / %f = %f", b, a, c);

  // print vendor id
  {
    u32 chars1;
    u32 chars2;
    u32 chars3;
    asm("mov $0, %%eax;"
        "cpuid;"
        : "=b"(chars1), "=d"(chars2), "=c"(chars3));
    // printf("\nVendor ID: ");

    char *chars_c;
    u8 i;

#define PRINT_ID_PART(chars)  \
  chars_c = (char *)(&chars); \
  for (i = 0; i < 4; i++) {   \
    print_chr(chars_c[i]);    \
  }
    //
    // PRINT_ID_PART(chars1);
    // PRINT_ID_PART(chars2);
    // PRINT_ID_PART(chars3);
  }

  // APIC stuff
  u32 apic_id;
  if (!lapic_present()) {
    set_print_style((TEXT_BG_BRIGHT + TEXT_BG_RED) | TEXT_COLOR_BLACK);
    printf("\nLAPIC present: false");
    set_print_style(TEXT_COLOR_WHITE);
  }

  apic_base = lapic_base();
  printf("\nLAPIC base: %x", apic_base);

  apic_id = lapic_id(apic_base);
  printf("\nAPIC ID: %u", (apic_id >> 24) & 0xff);

  reload_lapic_timer(apic_base, 10000);
  set_lapic_timer_prescaler(apic_base, Div64);
  set_lapic_timer_lvt(apic_base, 32, Periodic);
  set_lapic_int_mask(apic_base, LVT_OFFSET_TIMER, Unmasked);

  // I/O APIC
  {
#define IOAPICID 0
#define IOAPICVER 1
#define IOAPICARB 2
#define IOREDTBL 3

    // printf("\nI/O APIC ID: %u", 0xf & (ioapic_read(ioapic_base, IOAPICID) >>
    // 24));
    u32 ioapic_off1_read = ioapic_read(ioapic_base, IOAPICVER);
    // printf("\nI/O APIC Version: %u", ioapic_off1_read & 0xff);
    // printf("\nI/O APIC Max IRQs: %u", (ioapic_off1_read >> 16) & 0xff);

    struct IOAPICRedirectionEntry entry =
        create_ioapic_rd_entry(33, 0b00000000, apic_id);
    write_ioapic_rdtable(1, entry);

    struct IOAPICRedirectionEntry pit_entry =
        create_ioapic_rd_entry(34, 0b0, apic_id);
    write_ioapic_rdtable(2, pit_entry);
  }

  // Programmable Interval Timer
  {
#define CH0_DATA_PORT 0x40
#define CMD_REGISTER_PORT 0x43

    outb(CMD_REGISTER_PORT, 0b00110100);
    outb(CH0_DATA_PORT, 0);  // low byte
    outb(CH0_DATA_PORT, 0);  // high byte
                             // runs at 18Hz. counter is set to 65536
  }

  u8 buf[512] = {':', '('};
  buf[15] = '\0';
  {
    set_print_style(TEXT_COLOR_CYAN);
    u8 found = 0;
    u32 pci_bus, pci_slot, pci_fn;

    for (u32 l = 0; l < 256; l++) {
      for (u32 i = 0; i < 256; i++) {
        for (u32 fn = 0; fn < 8; fn++) {
          u8 a = (u8)i;
          if (pci_device_exists(l, a) &&
              pci_config_read_u16(l, a, fn, 0x8 + 2) == 0x0106) {
            printf(
                "\nPCI SATA Controller found at bus %u, slot %u, function %u",
                l, a, fn);
            pci_bus = l;
            pci_slot = a;
            pci_fn = fn;
            found = 1;
            goto exit_pci_scan;
          }
        }
      }
    }
  exit_pci_scan:;

    if (found == 0) goto exit_pci_section;

    // printf("\n:: Prog. If. [15:8] | Rev. ID [8:0]: %X",
    // pci_config_read_u16(pci_bus, pci_slot, pci_fn, 0x8));

    u32 bar_val;
    for (u8 i = 0; i <= 5; i++) {
      bar_val = pci_config_read_u32(pci_bus, pci_slot, pci_fn, 0x10 + i * 4);
      if (bar_val == 0) continue;
      printf("\n:: BAR%u: %x %s", i, bar_val, i == 5 ? "(HBA memory)" : "");
    }

    bar_val = pci_config_read_u32(pci_bus, pci_slot, pci_fn, 0x24);
    u32 bar_addr = bar_val & 0xfffffff0;

    pci_config_write_u32(pci_bus, pci_slot, pci_fn, 0x24, 0xffffffff);
    printf("\n:: Address space: %u bytes",
           1 + (~(pci_config_read_u32(pci_bus, pci_slot, pci_fn, 0x24))));

    pci_config_write_u32(pci_bus, pci_slot, pci_fn, 0x24, bar_val);

    u16 cmd_reg = pci_config_read_u16(pci_bus, pci_slot, pci_fn, 0x4);
    cmd_reg &= ~(1 << 10);
    cmd_reg |= 1 << 2;

    pci_config_write_u16(pci_bus, pci_slot, pci_fn, 0x4, cmd_reg);

    printf("\n:: Command: %b %b", cmd_reg >> 8, cmd_reg);

    HBA_MEM *hba = (HBA_MEM *)bar_addr;
    port_rebase(&hba->port, 0);
    hba->ghc |= (u32)1 << 31;

    hba->ghc |= 1;
    while (hba->ghc & 0b1)
      ;
    // if((hba->cap & ((u32)1 << 18)) == 0)
    hba->ghc |= (u32)1 << 31;
    hba->ghc |= ((u32)1 << 31) + ((u32)0b10);  // enable interrupts and AHCI
    printf("\n:: ghc: %x", hba->ghc);

    // hba->ports[0].clbu = 0;
    // hba->port.clb = 0x400000 + (0 << 10);
    // memset((void*)(hba->ports[0].clb), 0, 1024);

    // hba->port.clbu = 0;
    // hba->port.clb = (u32)&cmdl;
    //
    // hba->port.fbu = 0;
    // hba->port.fb = (u32)&fis;

    printf("\n:: port.clbu, port.clb: %x %x", hba->port.clbu, hba->port.clb);
    printf("\n:: port.fbu, port.fb: %x, %x", hba->port.fbu, hba->port.fb);
    printf("\n:: Supports native command queuing: %B", 1 & (hba->cap >> 30));
    printf("\n:: Number of command slots: %u", 0b1111 & (hba->cap >> 8));
    printf("\n:: Number of ports: %u, ports implemented: %x", 0xf & (hba->cap),
           hba->pi);

    // (char*)buf = "WHHFHFKJJFS GKJJG";

    buf[10] = '\0';

    read_bytes(&hba->port, 1, 0, 1, &buf);
    // void* fn = (void*)0x100000;
    // ((void (*)(void ))fn)();
    buf[10] = '\0';


    set_print_style(TEXT_COLOR_WHITE);

  exit_pci_section:;
  }

  // while (true);

  u8 cx, cy;
  u8 first_loop = true;
  u8 tx, ty;
  u8 ptx, pty;
  u8 style_under_cursor = 0xf;
  u8 char_under_cursor = ' ';
  get_cursor_coord(&cx, &cy);
  enable_cursor(13, 15);
  while (true) {
    u8 x, y;
    u32 lo;
    u32 hi;
    asm("rdtsc" : "=d"(hi), "=a"(lo));
    u64 timestamp = ((u64)hi << 32) + (u64)lo;
    move_cursor_to(cx, cy);
    // printf("\ntimestamp: %a",timestamp);
    u32 count;
    APIC_READ(0x390, count);
    printf("\ntimer count: %u", count);
    printf("\ntimer interrupt count: %u", apic_timer_int_count);
    printf("\nkeyboard(IO APIC) interrupt count: %u",
           ioapic_keyboard_int_count);
    printf("\nPIT elapsed time: ");
    print_f32_fixed(((f32)pit_int_count * 1.0 / 18.2065), 2);
    printf("s, (%u interrupts)", pit_int_count);
    printf("\nstr: %s", (char *)(&buf[0]));

    print_newline();
    get_cursor_coord(&x, &y);
    clear_row(y - 1);
    set_print_style(TEXT_COLOR_LGRAY);
    if (ps2_keyboard.shift) printf("[SHIFT]");
    if (ps2_keyboard.caps_lock) printf("[CAPS_LOCK]");
    if (ps2_keyboard.num_lock) printf("[NUM_LOCK]");
    if (ps2_keyboard.ctrl) printf("[CTRL]");
    if (ps2_keyboard.scroll_lock) printf("[SCROLL_LOCK]");
    set_print_style(TEXT_COLOR_WHITE);
    print_newline();
    if (first_loop) {
      get_cursor_coord(&tx, &ty);
      ptx = tx;
      pty = ty;
      update_cursor(tx, ty);
    }
    move_cursor_to(tx, ty);
    char key;
    while (!key_queue_isempty(&keyboard->key_queue)) {
      key = pop_key_queue(&keyboard->key_queue);

      switch (key) {
        case 0x7:
          move_cursor_up();
          break;
        case 0x6:
          move_cursor_down();
          break;
        case 0x4:
          move_cursor_right();
          break;
        case 0x5:
          move_cursor_left();
          break;
        default:
          printf("%c", key);
      }
      // style_under_cursor = 0xf;
      // char_under_cursor = char_at_xy(cx, cy);
    }
    get_cursor_coord(&tx, &ty);
    if (ptx != tx || pty != ty) {
      ptx = tx;
      pty = ty;
      update_cursor(tx, ty);
    }
    first_loop = false;
  }
}

inline u8 initialise_fpu() {
  u8 a = 10;
  __asm__ __volatile__(
      "mov %%cr0, %%edx;"
      "and $(~(0b1100)), %%edx;"
      "mov %%edx, %%cr0;"
      "fninit;"
      "fnstsw %%ax;"
      : "=a"(a)
      : "a"(a));
  return a == 0;
}

inline u8 initialise_sse() {
  __asm__ __volatile__(
      "mov %%cr0, %%eax;"
      "and $(0xfffb), %%ax;"
      "or $(0x02), %%ax;"
      "mov %%eax, %%cr0;"
      "mov %%cr4, %%eax;"
      "or $(3 << 9), %%ax;"
      "mov %%eax, %%cr4;"
      :);
  return 1;  // FIXME: check if initialisation succeeded somehow
}

__attribute__((interrupt)) void exception_gp(
    struct InterruptFrame_ErrorCode *frame) {
  print_str("\n[vector 13] General Protection fault\n");
  print_str("Error code: ");
  print_unum(frame->error_code);
  print_str("\nat: ");
  print_hex_u32(frame->eip);
  __asm__ __volatile__("cli; hlt");
}

__attribute__((interrupt)) void exception_handler(
    struct InterruptFrame *frame) {
  print_str("\nException REEEEEEEEEEEEEEE [halting]\n");
  __asm__ __volatile__("cli; hlt");
}

void delay() {
  for (u32 i = 0; i < 4 * 10e4; i++)
    ;
}
