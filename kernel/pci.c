#include "./headers/pci.h"

#include "./headers/consts.h"

// PCI_CONFIG_ADDRESS Register
//  ---------------------------------------------------------------
//  |  31    |  30:24   |  23:16  |  15:11  |   10:8   |   7:0    |
//  |-------------------------------------------------------------|
//  | Enable | Reserved |  Bus    | Device  | Function | Register |
//  |        |          |  number | number  | number   |  offset  |
//  |________|__________|_________|_________|__________|__________|
//
//   Header fields (return type on writing out configuration as per
//   the above table
//
//   ______________________________________________________
//   | Register  | Register |               Bits          |
//   |           | offset   | 31:24 | 23:16 |  15:8| 7:0  |
//   ======================================================
//   | 0x0       |  0x0     | Device ID     | Vendor ID   |
//   |-----------------------------------------------------
//   | 0x1       |  0x4     | Status        | Command     |
//   |-----------------------------------------------------
//   | 0x2       | 0x8      | Class |Sub-   |Prog  | Rev  |
//   |           |          | code  | class |IF    | ID   |
//   |-----------------------------------------------------
//   | 0x3       | 0xC      |BIST   | Header|Laten-|Cache |
//   |           |          |       | type  |cy    |Line  |
//   |           |          |       |       |Timer |Size  |
//   ......................................................
//   . . . . . . . . . . . . . . . . . . . . . . . . . . . .
//
//

u16 pci_config_read_u16(u8 bus, u8 slot, u8 fn, u8 offset) {
  u32 addr = (u32)offset & 0xFC;  // PCI_CONFIG_ADDRESS register
  addr |= (u32)bus << 16;
  addr |= (u32)slot << 11;
  addr |= (u32)fn << 8;
  addr |= (u32)1 << 31;

  outl(PCI_CONFIG_ADDRESS_PORT, addr);

  u16 read_val = 0;
  read_val =
      (u16)((inl(PCI_CONFIG_DATA_PORT) >> ((offset & 0b10) * 8)) & 0xffff);

  return read_val;
}

void pci_config_write_u16(u8 bus, u8 slot, u8 fn, u8 offset, u16 val) {
  u32 reg_val = pci_config_read_u32(bus, slot, fn, offset);

  reg_val &= ~(
      (u32)0xffff << (8 * (offset &
                           0b10)));  // clear first 16 bits or the next 16 bits
  reg_val |=
      (u32)val << (8 * (offset & 0b10));  // write the value to the cleared bits

  u32 addr = (u32)offset & 0xFC;
  addr |= (u32)bus << 16;
  addr |= (u32)slot << 11;
  addr |= (u32)fn << 8;
  addr |= (u32)1 << 31;

  outl(PCI_CONFIG_ADDRESS_PORT, addr);
  outl(PCI_CONFIG_DATA_PORT, reg_val);
}

u32 pci_config_read_u32(u8 bus, u8 slot, u8 fn, u8 offset) {
  u32 addr = (u32)offset & 0xFC;  // PCI_CONFIG_ADDRESS register
  addr |= (u32)bus << 16;
  addr |= (u32)slot << 11;
  addr |= (u32)fn << 8;
  addr |= (u32)1 << 31;

  outl(PCI_CONFIG_ADDRESS_PORT, addr);

  return inl(PCI_CONFIG_DATA_PORT);
}

void pci_config_write_u32(u8 bus, u8 slot, u8 fn, u8 offset, u32 val) {
  u32 addr = (u32)offset & 0xFC;
  addr |= (u32)bus << 16;
  addr |= (u32)slot << 11;
  addr |= (u32)fn << 8;
  addr |= (u32)1 << 31;

  outl(PCI_CONFIG_ADDRESS_PORT, addr);
  outl(PCI_CONFIG_DATA_PORT, val);
}

u8 pci_device_exists(u8 bus, u8 slot) {
  return pci_config_read_u16(bus, slot, 0, 0) != 0xffff;
}

i32 find_cmdslot(HBA_PORT *port) {
  u32 slots = port->sact | port->ci;
  for (u32 i = 0; i < 16; i++, slots >>= 1) {
    if ((slots & 1) == 0) return (i32)i;
  }
  printf("\n:: cannot find command slot :(");
  return -1;
}

u8 read_bytes(HBA_PORT *port, u32 startl, u32 starth, u32 count, u16 *buffer) {
  //  port.clb, port.clbu
  //       |
  //       |
  //       |
  //       v
  // command list: CMD_HEADER[n+1]
  // |--------------|
  // |   slot 0     | ---> command header: CMD_HEADER
  // |--------------|     |-------------------------------|
  // |   slot 1     |     | 31   |15           |7        0|
  // ................     |------|-------------|----------|
  // |   slot n     |     | PRDTL| PMP R C B R |P W A CFL |
  // |--------------|     |-------------------------------|
  //                      | PRD Byte Count                |
  //                      |-------------------------------|
  //                      |  Cmd table addr(128b aligned) |--|
  //                      |-------------------------------|  |
  //                      |  Cmd table addr upper         |--|
  //                      .................................  |
  //                                                         |
  //                                                         |
  //                         /------------------------------/
  //                         |
  //                         v
  //                 command table: CMD_TABLE
  //            |-------------------------------| 0x0
  //            | Command FIS (max 64 bytes)    |
  //            |-------------------------------| 0x40
  //            | ATAPI Command (max 16 bytes)  |
  //            |-------------------------------| 0x50
  //            |~~~~~~~~~~~Reserved~~~~~~~~~~~~|
  //            |-------------------------------| 0x80
  //            |  Physical Region Descriptor   |
  //            |  Table (max 65535 entries)    |
  //            --------------------------------
  //
  // NOTE: port.fb and port.fbu stores the address of received FIS
  //

  port->is = 0xffff;  // I think this sets all bits to 1

  i32 _slot = find_cmdslot(port);
  if (_slot == -1) return false;
  u32 slot = (u32)_slot;

  HBA_CMD_HEADER *command_header = (HBA_CMD_HEADER *)port->clb;
  command_header += slot;  // offset to the actual position of the header
  command_header->cfl = sizeof(FIS_REG_H2D) / sizeof(u32);  // why?
  command_header->w = 0;                                    // read from device
  //
  command_header->prdtl = 1;  // FIXME
  // command_header->p = 1;
  // command_header->c = 1;
  //
  HBA_CMD_TBL *command_table = (HBA_CMD_TBL *)(command_header->ctba);

  for (u32 i = 0; i < sizeof(HBA_CMD_TBL); i++) {
    ((u8 *)command_table)[i] = (u8)0;
  }

  command_table->prdt_entry[0].dba = (u32)buffer;
  command_table->prdt_entry[0].dbc = count * 512 - 1;
  command_table->prdt_entry[0].i = 0;
  //
  FIS_REG_H2D *command_fis = (FIS_REG_H2D *)(&command_table->cfis);
  //
  command_fis->fis_type = FIS_TYPE_REG_H2D;
  command_fis->c = 1;
// command_fis->control = 0x08;
#define ATA_CMD_READ_DMA_EX 0x25
  command_fis->command = ATA_CMD_READ_DMA_EX;

  command_fis->lba0 = (u8)startl;
  command_fis->lba1 = (u8)(startl >> 8);
  command_fis->lba2 = (u8)(startl >> 16);
  command_fis->device = 1 << 6;  // LBA_MODE (?)
  //
  command_fis->lba3 = (u8)(startl >> 24);
  command_fis->lba4 = (u8)starth;
  command_fis->lba5 = (u8)(starth >> 8);

  command_fis->countl = count & 0xff;
  command_fis->counth = count >> 8;  // FIXME
//
#define ATA_DEV_BUSY 0x80
#define ATA_DEV_DRQ 0x08

  u32 spin = 0;
  port->cmd |= (1 << 4);
  port->cmd |= (1 << 0);
  while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000) spin++;

  if (spin == 1000000) {
    printf("\nPort is hung");
    return false;
  }

  port->ci = (u32)1 << slot;

  while (1) {
    // In some longer duration reads, it may be helpful to spin on the DPS bit
    // in the PxIS port field as well (1 << 5)
    if ((port->ci & ((u32)1 << slot)) == 0) break;
    if (port->is & ((u32)1 << 30))  // Task file error
    {
      printf("Read disk error\n");
      return false;
    }
  }
  // TODO:
  //
  // * while(DEVICE_IS_BUSY && SPIN_COUNT < MAX) { wait; }
  //   if (SPIN_COUNT >= MAX) { port_hung; return; }
  //
  // * issue command: port->ci = 1 << slot;
  //
  // * Wait for completion, if error: return
  //
  // * Check for error

  return true;
}
