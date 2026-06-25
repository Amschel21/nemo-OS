#include "net.hpp"
#include "../drivers/rtl8139.hpp"
#include "../libk/memory.hpp"

uint8_t our_mac[ETH_ALEN] = {0};
uint8_t our_ip[4] = {10, 0, 2, 15};

static uint16_t ip_id = 0;

static inline uint16_t htons(uint16_t v)
{
    return ((v << 8) & 0xFF00) | ((v >> 8) & 0x00FF);
}

static uint16_t net_checksum(void* data, int len)
{
    uint32_t sum = 0;
    uint16_t* p = (uint16_t*)data;

    for(int i = 0; i < len / 2; i++)
        sum += p[i];

    if(len & 1)
        sum += ((uint8_t*)data)[len - 1];

    while(sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    return ~(uint16_t)sum;
}

void ip_init()
{
    const rtl8139_dev* nic = rtl8139_get_device(0);

    if(nic)
    {
        for(int i = 0; i < ETH_ALEN; i++)
            our_mac[i] = nic->mac[i];
    }
}

int ip_send(uint8_t* dst_ip, uint8_t proto, void* data, int len)
{
    const rtl8139_dev* nic = rtl8139_get_device(0);
    if(!nic)
        return -1;

    uint8_t mac[ETH_ALEN];
    int ret = arp_resolve(dst_ip, mac);

    if(ret != 0)
    {
        arp_send_request(dst_ip);
        return -2;
    }

    int total = sizeof(eth_hdr) + sizeof(ip_hdr) + len;
    uint8_t buf[1600];

    if(total > (int)sizeof(buf))
        return -1;

    eth_hdr* eth = (eth_hdr*)buf;
    ip_hdr*  ip  = (ip_hdr*)(buf + sizeof(eth_hdr));

    for(int i = 0; i < ETH_ALEN; i++)
    {
        eth->dst[i] = mac[i];
        eth->src[i] = our_mac[i];
    }
    eth->type = htons(ETHERTYPE_IP);

    ip->ver_ihl    = 0x45;
    ip->tos        = 0;
    ip->total_len  = htons(sizeof(ip_hdr) + len);
    ip->id         = htons(ip_id++);
    ip->flags_frag = 0;
    ip->ttl        = 64;
    ip->proto      = proto;
    ip->checksum   = 0;

    for(int i = 0; i < 4; i++)
    {
        ip->src_ip[i] = our_ip[i];
        ip->dst_ip[i] = dst_ip[i];
    }

    for(int i = 0; i < len; i++)
        ((uint8_t*)(ip + 1))[i] = ((uint8_t*)data)[i];

    ip->checksum = net_checksum(ip, sizeof(ip_hdr));

    return rtl8139_send_packet(nic, buf, total);
}
