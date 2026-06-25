#include "net.hpp"
#include "../drivers/rtl8139.hpp"
#include "../terminal.hpp"
#include "../libk/itoa.hpp"
#include "../libk/memory.hpp"

static int ip_checksum_ok(ip_hdr* ip, int len)
{
    (void)len;

    uint32_t sum = 0;
    uint16_t* p = (uint16_t*)ip;

    unsigned int hdr_words = sizeof(ip_hdr) / 2;
    for(unsigned int i = 0; i < hdr_words; i++)
        sum += p[i];

    while(sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    return (uint16_t)sum == 0xFFFF;
}

void net_init()
{
    ip_init();
}

void net_poll()
{
    const rtl8139_dev* nic = rtl8139_get_device(0);

    if(!nic)
        return;

    uint8_t buf[1600];
    int len = sizeof(buf);

    while(rtl8139_poll(nic, buf, &len) > 0)
    {
        eth_hdr* eth = (eth_hdr*)buf;

        switch(eth->type)
        {
            case 0x0608:
                if(len >= (int)(sizeof(eth_hdr) + sizeof(arp_pkt)))
                    arp_handle_packet(
                        (arp_pkt*)(buf + sizeof(eth_hdr)),
                        len - sizeof(eth_hdr));
                break;

            case 0x0008:
            {
                ip_hdr* ip = (ip_hdr*)(buf + sizeof(eth_hdr));
                int ip_len = len - sizeof(eth_hdr);

                if(ip_len >= (int)sizeof(ip_hdr) && ip_checksum_ok(ip, ip_len))
                {
                    switch(ip->proto)
                    {
                        case IP_PROTO_ICMP:
                        {
                            int icmp_len = ip_len - sizeof(ip_hdr);
                            void* icmp = (uint8_t*)ip + sizeof(ip_hdr);

                            if(icmp_len >= 4)
                                icmp_handle_packet(ip, icmp, icmp_len);
                            break;
                        }
                    }
                }
                break;
            }
        }
    }
}
