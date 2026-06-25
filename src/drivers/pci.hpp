#pragma once

#include <stdint.h>

#define PCI_VENDOR_ID   0x00
#define PCI_DEVICE_ID   0x02
#define PCI_COMMAND     0x04
#define PCI_STATUS      0x06
#define PCI_REVISION_ID 0x08
#define PCI_PROG_IF     0x09
#define PCI_SUBCLASS    0x0A
#define PCI_CLASS       0x0B
#define PCI_HEADER_TYPE 0x0E

#define PCI_CLASS_MASS_STORAGE    0x01
#define PCI_CLASS_NETWORK         0x02
#define PCI_CLASS_DISPLAY         0x03
#define PCI_CLASS_MULTIMEDIA      0x04
#define PCI_CLASS_BRIDGE          0x06
#define PCI_CLASS_BASE_PERIPHERAL 0x08
#define PCI_CLASS_INPUT_DEVICE    0x09

#define PCI_SUBCLASS_IDE       0x01
#define PCI_SUBCLASS_SATA      0x06
#define PCI_SUBCLASS_AHCI      0x06

struct pci_device
{
    uint8_t  bus;
    uint8_t  slot;
    uint8_t  func;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t  class_code;
    uint8_t  subclass;
    uint8_t  prog_if;
    uint8_t  header_type;
};

int  pci_init();
int  pci_device_count();
const pci_device* pci_get_device(int index);
uint32_t pci_read_config(const pci_device* dev, uint8_t offset);
void pci_write_config(const pci_device* dev, uint8_t offset, uint32_t value);
uint32_t pci_read_bar(const pci_device* dev, int bar_index);
