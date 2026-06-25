#include "pci.hpp"
#include "../arch/x86/ports.hpp"

#define PCI_CONFIG_ADDR  0xCF8
#define PCI_CONFIG_DATA  0xCFC

#define MAX_PCI_DEVICES 64

static pci_device pci_devices[MAX_PCI_DEVICES];
static int pci_count = 0;

static uint32_t pci_config_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
    uint32_t address =
        (uint32_t)(((uint32_t)bus << 16) |
                   ((uint32_t)slot << 11) |
                   ((uint32_t)func << 8) |
                   (offset & 0xFC) |
                   0x80000000);

    outl(PCI_CONFIG_ADDR, address);
    return inl(PCI_CONFIG_DATA);
}

static void pci_config_write(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value)
{
    uint32_t address =
        (uint32_t)(((uint32_t)bus << 16) |
                   ((uint32_t)slot << 11) |
                   ((uint32_t)func << 8) |
                   (offset & 0xFC) |
                   0x80000000);

    outl(PCI_CONFIG_ADDR, address);
    outl(PCI_CONFIG_DATA, value);
}

static void pci_add_device(uint8_t bus, uint8_t slot, uint8_t func)
{
    if(pci_count >= MAX_PCI_DEVICES)
        return;

    uint32_t id = pci_config_read(bus, slot, func, PCI_VENDOR_ID);
    uint16_t vendor = id & 0xFFFF;
    uint16_t device = id >> 16;

    if(vendor == 0xFFFF)
        return;

    uint32_t class_rev = pci_config_read(bus, slot, func, PCI_CLASS);
    uint8_t class_code = class_rev >> 24;
    uint8_t subclass   = (class_rev >> 16) & 0xFF;
    uint8_t prog_if    = (class_rev >> 8) & 0xFF;

    uint32_t hdr = pci_config_read(bus, slot, func, PCI_HEADER_TYPE);
    uint8_t header_type = (hdr >> 16) & 0xFF;

    pci_device* d = &pci_devices[pci_count++];
    d->bus         = bus;
    d->slot        = slot;
    d->func        = func;
    d->vendor_id   = vendor;
    d->device_id   = device;
    d->class_code  = class_code;
    d->subclass    = subclass;
    d->prog_if     = prog_if;
    d->header_type = header_type;
}

int pci_init()
{
    pci_count = 0;

    for(int bus = 0; bus < 256; bus++)
    {
        for(int slot = 0; slot < 32; slot++)
        {
            uint32_t id = pci_config_read(bus, slot, 0, PCI_VENDOR_ID);
            uint16_t vendor = id & 0xFFFF;

            if(vendor == 0xFFFF)
                continue;

            pci_add_device(bus, slot, 0);

            uint32_t hdr = pci_config_read(bus, slot, 0, PCI_HEADER_TYPE);
            uint8_t header_type = (hdr >> 16) & 0xFF;

            if(header_type & 0x80)
            {
                for(int func = 1; func < 8; func++)
                {
                    id = pci_config_read(bus, slot, func, PCI_VENDOR_ID);
                    vendor = id & 0xFFFF;

                    if(vendor != 0xFFFF)
                        pci_add_device(bus, slot, func);
                }
            }
        }
    }

    return pci_count;
}

int pci_device_count()
{
    return pci_count;
}

const pci_device* pci_get_device(int index)
{
    if(index < 0 || index >= pci_count)
        return nullptr;
    return &pci_devices[index];
}

uint32_t pci_read_config(const pci_device* dev, uint8_t offset)
{
    return pci_config_read(dev->bus, dev->slot, dev->func, offset);
}

void pci_write_config(const pci_device* dev, uint8_t offset, uint32_t value)
{
    pci_config_write(dev->bus, dev->slot, dev->func, offset, value);
}

uint32_t pci_read_bar(const pci_device* dev, int bar_index)
{
    if(bar_index < 0 || bar_index > 5)
        return 0;

    return pci_read_config(dev, 0x10 + bar_index * 4);
}
