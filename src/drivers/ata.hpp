#pragma once
#include <stdint.h>

#define ATA_SECTOR_SIZE 512

struct ata_device
{
    uint16_t  base;
    uint16_t  ctrl;
    bool      present;
    bool      is_master;
    char      model[40];
    uint32_t  sectors;
};

int  ata_init();
int  ata_device_count();
const ata_device* ata_get_device(int index);
int  ata_read_sector(int dev, uint32_t lba, void* buffer);
int  ata_write_sector(int dev, uint32_t lba, const void* buffer);
