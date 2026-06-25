#include "ata.hpp"
#include "../arch/x86/io.hpp"
#include "../libk/memory.hpp"

#define ATA_PRIMARY_IO     0x1F0
#define ATA_PRIMARY_CTRL   0x3F6
#define ATA_SECONDARY_IO   0x170
#define ATA_SECONDARY_CTRL 0x376

#define ATA_REG_DATA       0
#define ATA_REG_ERROR      1
#define ATA_REG_SECTORS    2
#define ATA_REG_LBA_LOW    3
#define ATA_REG_LBA_MID    4
#define ATA_REG_LBA_HIGH   5
#define ATA_REG_DRIVE      6
#define ATA_REG_COMMAND    7
#define ATA_REG_STATUS     7

#define ATA_CMD_READ       0x20
#define ATA_CMD_IDENTIFY   0xEC
#define ATA_CMD_FLUSH      0xE7

#define ATA_STATUS_ERR     0x01
#define ATA_STATUS_DRQ     0x08
#define ATA_STATUS_SRV     0x10
#define ATA_STATUS_DF      0x20
#define ATA_STATUS_RDY     0x40
#define ATA_STATUS_BSY     0x80

#define ATA_TIMEOUT        10000000

static ata_device devices[4];
static int device_count = 0;

static void ata_poll()
{
    for(int i = 0; i < 4; i++)
        inb(ATA_PRIMARY_CTRL);
}

static int ata_wait_bsy(uint16_t base)
{
    uint32_t timeout = ATA_TIMEOUT;
    while((inb(base + ATA_REG_STATUS) & ATA_STATUS_BSY) && --timeout)
        asm volatile("pause");
    return timeout ? 0 : -1;
}

static int ata_wait_drq(uint16_t base)
{
    uint32_t timeout = ATA_TIMEOUT;
    while(!(inb(base + ATA_REG_STATUS) & ATA_STATUS_DRQ) && --timeout)
    {
        if(inb(base + ATA_REG_STATUS) & ATA_STATUS_ERR)
            return -1;
        asm volatile("pause");
    }
    return timeout ? 0 : -1;
}

static int ata_identify(uint16_t base, bool master, ata_device* dev)
{
    outb(base + ATA_REG_DRIVE, master ? 0xA0 : 0xB0);
    ata_poll();

    uint8_t status = inb(base + ATA_REG_STATUS);
    if(status == 0xFF)
        return -1;

    outb(base + ATA_REG_SECTORS, 0);
    outb(base + ATA_REG_LBA_LOW, 0);
    outb(base + ATA_REG_LBA_MID, 0);
    outb(base + ATA_REG_LBA_HIGH, 0);
    outb(base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

    status = inb(base + ATA_REG_STATUS);
    if(status == 0)
        return -1;

    if(ata_wait_bsy(base) < 0)
        return -1;

    status = inb(base + ATA_REG_STATUS);
    if(status & ATA_STATUS_ERR)
    {
        uint8_t mid = inb(base + ATA_REG_LBA_MID);
        uint8_t high = inb(base + ATA_REG_LBA_HIGH);
        if(mid == 0x14 && high == 0xEB)
            return -2;
        return -1;
    }

    if(ata_wait_drq(base) < 0)
        return -1;

    uint16_t buf[256];
    for(int i = 0; i < 256; i++)
        buf[i] = inw(base + ATA_REG_DATA);

    dev->base = base;
    dev->ctrl = (base == ATA_PRIMARY_IO) ? ATA_PRIMARY_CTRL : ATA_SECONDARY_CTRL;
    dev->present = true;
    dev->is_master = master;
    dev->sectors = ((uint32_t)buf[61] << 16) | buf[60];

    memset(dev->model, 0, sizeof(dev->model));
    for(int i = 0; i < 20; i++)
    {
        dev->model[i * 2]     = (buf[27 + i] >> 8) & 0xFF;
        dev->model[i * 2 + 1] =  buf[27 + i]       & 0xFF;
    }

    for(int i = 39; i >= 0; i--)
    {
        if(dev->model[i] != ' ')
            break;
        dev->model[i] = 0;
    }

    return 0;
}

int ata_init()
{
    device_count = 0;

    struct { uint16_t base; bool master; } probes[] = {
        { ATA_PRIMARY_IO,   true  },
        { ATA_PRIMARY_IO,   false },
        { ATA_SECONDARY_IO, true  },
        { ATA_SECONDARY_IO, false },
    };

    for(int i = 0; i < 4; i++)
    {
        ata_device dev;
        memset(&dev, 0, sizeof(dev));
        dev.present = false;

        int ret = ata_identify(probes[i].base, probes[i].master, &dev);
        if(ret == 0)
            devices[device_count++] = dev;
    }

    return device_count;
}

int ata_device_count()
{
    return device_count;
}

const ata_device* ata_get_device(int index)
{
    if(index < 0 || index >= device_count)
        return nullptr;
    return &devices[index];
}

int ata_read_sector(int dev, uint32_t lba, void* buffer)
{
    if(dev < 0 || dev >= device_count)
        return -1;

    const ata_device* d = &devices[dev];
    uint16_t base = d->base;

    if(ata_wait_bsy(base) < 0)
        return -1;

    outb(base + ATA_REG_DRIVE,
         0xE0 | (d->is_master ? 0 : 0x10) | ((lba >> 24) & 0x0F));
    outb(base + ATA_REG_SECTORS, 1);
    outb(base + ATA_REG_LBA_LOW,   lba & 0xFF);
    outb(base + ATA_REG_LBA_MID,  (lba >> 8) & 0xFF);
    outb(base + ATA_REG_LBA_HIGH, (lba >> 16) & 0xFF);
    outb(base + ATA_REG_COMMAND, ATA_CMD_READ);

    if(ata_wait_drq(base) < 0)
        return -1;

    for(int i = 0; i < 256; i++)
        ((uint16_t*)buffer)[i] = inw(base + ATA_REG_DATA);

    return 0;
}

int ata_write_sector(int dev, uint32_t lba, const void* buffer)
{
    if(dev < 0 || dev >= device_count)
        return -1;

    const ata_device* d = &devices[dev];
    uint16_t base = d->base;

    if(ata_wait_bsy(base) < 0)
        return -1;

    outb(base + ATA_REG_DRIVE,
         0xE0 | (d->is_master ? 0 : 0x10) | ((lba >> 24) & 0x0F));
    outb(base + ATA_REG_SECTORS, 1);
    outb(base + ATA_REG_LBA_LOW,   lba & 0xFF);
    outb(base + ATA_REG_LBA_MID,  (lba >> 8) & 0xFF);
    outb(base + ATA_REG_LBA_HIGH, (lba >> 16) & 0xFF);
    outb(base + ATA_REG_COMMAND, ATA_CMD_READ);
    outb(base + ATA_REG_COMMAND, 0x30);

    if(ata_wait_drq(base) < 0)
        return -1;

    for(int i = 0; i < 256; i++)
        outw(base + ATA_REG_DATA, ((uint16_t*)buffer)[i]);

    ata_wait_bsy(base);
    outb(base + ATA_REG_COMMAND, ATA_CMD_FLUSH);

    return 0;
}
