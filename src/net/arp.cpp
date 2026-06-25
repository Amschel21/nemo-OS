#include "net.hpp"
#include "../drivers/rtl8139.hpp"
#include "../libk/memory.hpp"

#define ARP_CACHE_SIZE 8

static struct {
    uint8_t ip[4];
    uint8_t mac[ETH_ALEN];
    int     valid;
} arp_cache[ARP_CACHE_SIZE];

static int arp_cache_add(uint8_t* ip, uint8_t* mac)
{
    for(int i = 0; i < ARP_CACHE_SIZE; i++)
    {
        if(arp_cache[i].valid &&
           arp_cache[i].ip[0] == ip[0] &&
           arp_cache[i].ip[1] == ip[1] &&
           arp_cache[i].ip[2] == ip[2] &&
           arp_cache[i].ip[3] == ip[3])
        {
            for(int j = 0; j < ETH_ALEN; j++)
                arp_cache[i].mac[j] = mac[j];
            return 0;
        }
    }

    for(int i = 0; i < ARP_CACHE_SIZE; i++)
    {
        if(!arp_cache[i].valid)
        {
            for(int j = 0; j < 4; j++)
                arp_cache[i].ip[j] = ip[j];
            for(int j = 0; j < ETH_ALEN; j++)
                arp_cache[i].mac[j] = mac[j];
            arp_cache[i].valid = 1;
            return 0;
        }
    }

    return -1;
}

static int arp_cache_lookup(uint8_t* ip, uint8_t* mac)
{
    for(int i = 0; i < ARP_CACHE_SIZE; i++)
    {
        if(arp_cache[i].valid &&
           arp_cache[i].ip[0] == ip[0] &&
           arp_cache[i].ip[1] == ip[1] &&
           arp_cache[i].ip[2] == ip[2] &&
           arp_cache[i].ip[3] == ip[3])
        {
            for(int j = 0; j < ETH_ALEN; j++)
                mac[j] = arp_cache[i].mac[j];
            return 0;
        }
    }

    return -1;
}

void arp_send_request(uint8_t* ip)
{
    const rtl8139_dev* nic = rtl8139_get_device(0);
    if(!nic)
        return;

    uint8_t broadcast[ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    uint8_t buf[sizeof(eth_hdr) + sizeof(arp_pkt)];
    eth_hdr* eth = (eth_hdr*)buf;
    arp_pkt* arp = (arp_pkt*)(buf + sizeof(eth_hdr));

    for(int i = 0; i < ETH_ALEN; i++)
    {
        eth->dst[i] = broadcast[i];
        eth->src[i] = our_mac[i];
    }
    eth->type = 0x0608;

    arp->htype = 0x0100;
    arp->ptype = 0x0008;
    arp->hlen  = ETH_ALEN;
    arp->plen  = 4;
    arp->oper  = 0x0100;

    for(int i = 0; i < ETH_ALEN; i++)
        arp->sha[i] = our_mac[i];

    for(int i = 0; i < 4; i++)
        arp->spa[i] = our_ip[i];

    for(int i = 0; i < ETH_ALEN; i++)
        arp->tha[i] = 0;

    for(int i = 0; i < 4; i++)
        arp->tpa[i] = ip[i];

    rtl8139_send_packet(nic, buf, sizeof(buf));
}

int arp_resolve(uint8_t* ip, uint8_t* mac)
{
    if(arp_cache_lookup(ip, mac) == 0)
        return 0;

    return -1;
}

int arp_handle_packet(arp_pkt* arp, int len)
{
    (void)len;

    if(arp->htype != 0x0100 || arp->ptype != 0x0008)
        return -1;

    if(arp->oper == 0x0100)
    {
        if(arp->tpa[0] == our_ip[0] &&
           arp->tpa[1] == our_ip[1] &&
           arp->tpa[2] == our_ip[2] &&
           arp->tpa[3] == our_ip[3])
        {
            const rtl8139_dev* nic = rtl8139_get_device(0);
            if(!nic)
                return -1;

            uint8_t buf[sizeof(eth_hdr) + sizeof(arp_pkt)];
            eth_hdr* eth = (eth_hdr*)buf;
            arp_pkt* rep = (arp_pkt*)(buf + sizeof(eth_hdr));

            for(int i = 0; i < ETH_ALEN; i++)
            {
                eth->dst[i] = arp->sha[i];
                eth->src[i] = our_mac[i];
            }
            eth->type = 0x0608;

            rep->htype = 0x0100;
            rep->ptype = 0x0008;
            rep->hlen  = ETH_ALEN;
            rep->plen  = 4;
            rep->oper  = 0x0200;

            for(int i = 0; i < ETH_ALEN; i++)
                rep->sha[i] = our_mac[i];

            for(int i = 0; i < 4; i++)
                rep->spa[i] = our_ip[i];

            for(int i = 0; i < ETH_ALEN; i++)
                rep->tha[i] = arp->sha[i];

            for(int i = 0; i < 4; i++)
                rep->tpa[i] = arp->spa[i];

            rtl8139_send_packet(nic, buf, sizeof(buf));

            arp_cache_add(arp->spa, arp->sha);
        }
    }
    else if(arp->oper == 0x0200)
    {
        arp_cache_add(arp->spa, arp->sha);
    }

    return 0;
}
