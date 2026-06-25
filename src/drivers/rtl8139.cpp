#include "rtl8139.hpp"
#include "pci.hpp"
#include "../arch/x86/ports.hpp"
#include "../memory/kmalloc.hpp"
#include "../libk/memory.hpp"

#define RTL8139_MAX_DEVICES 4

static rtl8139_dev rtl_devices[RTL8139_MAX_DEVICES];
static int rtl_count = 0;

#define RX_BUF_SIZE 8192
#define TX_BUF_SIZE 1536

struct rtl_priv
{
    uint8_t* rx_buffer;
    uint8_t* tx_buffer[4];
    int      tx_cur;
};

static rtl_priv* privs[RTL8139_MAX_DEVICES];

#define RTL_IDR0    0x00
#define RTL_TSD0    0x10
#define RTL_TSAD0   0x20
#define RTL_RBSTART 0x30
#define RTL_CR      0x37
#define RTL_CAPR    0x38
#define RTL_IMR     0x3C
#define RTL_ISR     0x3E
#define RTL_TCR     0x40
#define RTL_RCR     0x44
#define RTL_MSR     0x58

#define CR_RST  0x10
#define CR_RE   0x08
#define CR_TE   0x04
#define CR_BUFE 0x01

static void rtl_write8(const rtl8139_dev* dev, uint8_t reg, uint8_t val)
{
    outb(dev->io_base + reg, val);
}

static void rtl_write16(const rtl8139_dev* dev, uint8_t reg, uint16_t val)
{
    outw(dev->io_base + reg, val);
}

static void rtl_write32(const rtl8139_dev* dev, uint8_t reg, uint32_t val)
{
    outl(dev->io_base + reg, val);
}

static uint8_t rtl_read8(const rtl8139_dev* dev, uint8_t reg)
{
    return inb(dev->io_base + reg);
}

static uint16_t rtl_read16(const rtl8139_dev* dev, uint8_t reg)
{
    return inw(dev->io_base + reg);
}

static uint32_t rtl_read32(const rtl8139_dev* dev, uint8_t reg)
{
    return inl(dev->io_base + reg);
}

static int rtl8139_init_one(const pci_device* pci_dev, int index)
{
    rtl8139_dev* dev = &rtl_devices[index];
    dev->initialized = 0;

    uint32_t bar = pci_read_bar(pci_dev, 0);

    if(!bar)
        return -1;

    dev->io_base = bar & 0xFFFC;

    rtl_write8(dev, RTL_CR, CR_RST);

    int timeout = 0;
    while((rtl_read8(dev, RTL_CR) & CR_RST) && timeout < 1000)
    {
        for(volatile int i = 0; i < 1000; i++);
        timeout++;
    }

    if(timeout >= 1000)
        return -1;

    for(int i = 0; i < 6; i++)
        dev->mac[i] = rtl_read8(dev, RTL_IDR0 + i);

    rtl_priv* priv = (rtl_priv*)kmalloc(sizeof(rtl_priv));
    if(!priv)
        return -1;

    priv->rx_buffer = (uint8_t*)kmalloc(RX_BUF_SIZE + 16);
    if(!priv->rx_buffer)
    {
        kfree(priv);
        return -1;
    }

    memset(priv->rx_buffer, 0, RX_BUF_SIZE + 16);

    uint32_t rx_phys = (uint32_t)priv->rx_buffer;

    rtl_write32(dev, RTL_RBSTART, rx_phys);
    rtl_write16(dev, RTL_IMR, 0x0005);
    rtl_write32(dev, RTL_RCR, 0x00000F0F);
    rtl_write32(dev, RTL_TCR, 0x00000000);

    rtl_write8(dev, RTL_CR, CR_TE | CR_RE);

    for(int i = 0; i < 4; i++)
    {
        priv->tx_buffer[i] = (uint8_t*)kmalloc(TX_BUF_SIZE);
        if(!priv->tx_buffer[i])
        {
            for(int j = 0; j < i; j++)
                kfree(priv->tx_buffer[j]);
            kfree(priv->rx_buffer);
            kfree(priv);
            return -1;
        }
        memset(priv->tx_buffer[i], 0, TX_BUF_SIZE);
    }

    priv->tx_cur = 0;
    privs[index] = priv;

    dev->initialized = 1;
    return 0;
}

int rtl8139_init_all()
{
    rtl_count = 0;

    for(int i = 0; i < pci_device_count() && rtl_count < RTL8139_MAX_DEVICES; i++)
    {
        const pci_device* d = pci_get_device(i);

        if(d->vendor_id == RTL8139_VENDOR && d->device_id == RTL8139_DEVICE)
        {
            if(rtl8139_init_one(d, rtl_count) == 0)
                rtl_count++;
        }
    }

    return rtl_count;
}

int rtl8139_count()
{
    return rtl_count;
}

const rtl8139_dev* rtl8139_get_device(int index)
{
    if(index < 0 || index >= rtl_count)
        return nullptr;
    return &rtl_devices[index];
}

int rtl8139_send_packet(const rtl8139_dev* dev, const void* data, int len)
{
    if(!dev || !dev->initialized || !data || len <= 0 || len > TX_BUF_SIZE)
        return -1;

    int i_index = 0;
    for(int i = 0; i < rtl_count; i++)
    {
        if(&rtl_devices[i] == dev)
        {
            i_index = i;
            break;
        }
    }

    rtl_priv* priv = privs[i_index];

    memcpy(priv->tx_buffer[priv->tx_cur], data, len);

    uint32_t tx_addr = (uint32_t)priv->tx_buffer[priv->tx_cur];
    rtl_write32(dev, RTL_TSAD0 + priv->tx_cur * 4, tx_addr);
    rtl_write32(dev, RTL_TSD0 + priv->tx_cur * 4, len);

    priv->tx_cur = (priv->tx_cur + 1) % 4;

    return len;
}
