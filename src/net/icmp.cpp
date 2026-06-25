#include "net.hpp"
#include "../drivers/rtl8139.hpp"
#include "../libk/memory.hpp"

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

int icmp_send_echo(uint8_t* dst_ip, uint16_t id, uint16_t seq)
{
    uint8_t data[32];
    icmp_pkt* icmp = (icmp_pkt*)data;

    icmp->type = ICMP_TYPE_ECHO_REQUEST;
    icmp->code = 0;
    icmp->checksum = 0;
    icmp->id = ((id << 8) & 0xFF00) | ((id >> 8) & 0x00FF);
    icmp->seq = ((seq << 8) & 0xFF00) | ((seq >> 8) & 0x00FF);

    for(int i = 0; i < 24; i++)
        icmp->data[i] = i;

    icmp->checksum = net_checksum(data, sizeof(icmp_pkt) + 24);

    return ip_send(dst_ip, IP_PROTO_ICMP, data, sizeof(icmp_pkt) + 24);
}

int icmp_handle_packet(ip_hdr* ip, void* pkt, int len)
{
    (void)ip;
    (void)len;

    icmp_pkt* icmp = (icmp_pkt*)pkt;

    if(icmp->type == ICMP_TYPE_ECHO_REQUEST)
    {
        uint8_t reply_ip[4];

        for(int i = 0; i < 4; i++)
            reply_ip[i] = ip->src_ip[i];

        icmp->type = ICMP_TYPE_ECHO_REPLY;
        icmp->checksum = 0;
        icmp->checksum = net_checksum(pkt, len);

        ip_send(reply_ip, IP_PROTO_ICMP, pkt, len);
    }

    return 0;
}
