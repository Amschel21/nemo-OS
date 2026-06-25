#pragma once

#include <stdint.h>

#define ETH_ALEN 6
#define ETHERTYPE_ARP  0x0806
#define ETHERTYPE_IP   0x0800

#pragma pack(push, 1)
struct eth_hdr
{
    uint8_t  dst[ETH_ALEN];
    uint8_t  src[ETH_ALEN];
    uint16_t type;
};
#pragma pack(pop)

#define ARP_HTYPE_ETH 1
#define ARP_PTYPE_IP  0x0800
#define ARP_HLEN 6
#define ARP_PLEN 4
#define ARP_OP_REQUEST 1
#define ARP_OP_REPLY   2

#pragma pack(push, 1)
struct arp_pkt
{
    uint16_t htype;
    uint16_t ptype;
    uint8_t  hlen;
    uint8_t  plen;
    uint16_t oper;
    uint8_t  sha[ETH_ALEN];
    uint8_t  spa[4];
    uint8_t  tha[ETH_ALEN];
    uint8_t  tpa[4];
};
#pragma pack(pop)

#define IP_PROTO_ICMP 1
#define IP_PROTO_UDP  17

#pragma pack(push, 1)
struct ip_hdr
{
    uint8_t  ver_ihl;
    uint8_t  tos;
    uint16_t total_len;
    uint16_t id;
    uint16_t flags_frag;
    uint8_t  ttl;
    uint8_t  proto;
    uint16_t checksum;
    uint8_t  src_ip[4];
    uint8_t  dst_ip[4];
};
#pragma pack(pop)

#define ICMP_TYPE_ECHO_REPLY   0
#define ICMP_TYPE_ECHO_REQUEST 8

#pragma pack(push, 1)
struct icmp_pkt
{
    uint8_t  type;
    uint8_t  code;
    uint16_t checksum;
    uint16_t id;
    uint16_t seq;
    uint8_t  data[];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct udp_hdr
{
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t len;
    uint16_t checksum;
};
#pragma pack(pop)

#define BROADCAST_MAC {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
#define OUR_IP {10, 0, 2, 15}
#define GATEWAY_IP {10, 0, 2, 1}

void net_init();
void net_poll();

int  arp_resolve(uint8_t* ip, uint8_t* mac);
void arp_send_request(uint8_t* ip);
int  arp_handle_packet(arp_pkt* arp, int len);

int  ip_send(uint8_t* dst_ip, uint8_t proto, void* data, int len);
int  ip_handle_packet(eth_hdr* eth, void* pkt, int len);

int  icmp_send_echo(uint8_t* dst_ip, uint16_t id, uint16_t seq);
int  icmp_handle_packet(ip_hdr* ip, void* pkt, int len);

void ip_init();

extern uint8_t our_mac[ETH_ALEN];
extern uint8_t our_ip[4];
