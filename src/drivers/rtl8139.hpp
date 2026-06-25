#pragma once

#include <stdint.h>

#define RTL8139_VENDOR 0x10EC
#define RTL8139_DEVICE 0x8139

struct rtl8139_dev
{
    uint16_t io_base;
    uint8_t  mac[6];
    int      initialized;
};

int  rtl8139_init_all();
int  rtl8139_count();
const rtl8139_dev* rtl8139_get_device(int index);
int  rtl8139_send_packet(const rtl8139_dev* dev, const void* data, int len);
