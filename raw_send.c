/* raw_send.c - program to send arbitrary ethernet packets.
 * See https://github.com/fordsfords/send_igmp */

/*
# This code and its documentation is Copyright 2024 Steven Ford, http://geeky-boy.com
# and licensed "public domain" style under Creative Commons "CC0": http://creativecommons.org/publicdomain/zero/1.0/
# To the extent possible under law, the contributors to this project have
# waived all copyright and related or neighboring rights to this work.
# In other words, you can use this code for any purpose without any
# restrictions.  This work is published from: United States.  The project home
# is https://github.com/fordsfords/send_igmp
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>

/* GNU-specific function. */
int asprintf(char **strp, const char *fmt, ...);


/* Simple error handler if -1. */
#define E(em1_sys_call_) do { \
  int em1_ = (em1_sys_call_); \
  if (em1_ == -1) { \
    int em1_errno_ = errno; \
    char *em1_errstr_; \
    asprintf(&em1_errstr_, "ERROR (%s:%d): %s returned -1", __FILE__, __LINE__, #em1_sys_call_); \
    errno = em1_errno_; \
    perror(em1_errstr_); \
    free(em1_errstr_); \
    exit(1); \
  } \
} while (0)  /* E */


#define IP_LEN 4  /* Bytes for an IP address in a packet. */
#define MAC_LEN 6  /* Bytes for a MAC address in a packet. */
#define ETH_MAX_SIZE 9000  /* Jumbo frames (not common). */
#define UDP_MAX_DATA (ETH_MAX_SIZE - (sizeof(struct eth_s) + sizeof(struct ip_s) + sizeof(struct udp_s)))

struct eth_s {
  uint8_t eth_dst[6];
  uint8_t eth_src[6];
  uint8_t eth_proto[2];
};

struct ip_s {
  uint8_t ip_version_ihl;  /* IHL is lower nibble in units of 4 bytes. */
  uint8_t ip_dscp_ecn;
  uint8_t ip_tot_len[2];
  uint8_t ip_id[2];
  uint8_t ip_flags_fragofs[2];
  uint8_t ip_ttl;
  uint8_t ip_protocol;
  uint8_t ip_hdr_chksum[2];
  uint8_t ip_src_ip[4];
  uint8_t ip_dst_ip[4];
};

/* IP option (used by IGMP). */
struct ipopt_s {
  uint8_t ipopt_type;
  uint8_t ipopt_len;
  uint8_t ipopt_routeralert[2];
};

struct igmp_s {
  uint8_t igmp_type;
  uint8_t igmp_reserved1;
  uint8_t igmp_chksum[2];
  uint8_t igmp_reserved2[2];
  uint8_t igmp_num_grp_recs[2];
  uint8_t igmp_grp_rec_type;
  uint8_t igmp_grp_rec_aux_len;
  uint8_t igmp_grp_rec_num_src[2];
  uint8_t igmp_grp_rec_mcast_addr[4];
};

/* ??? Will all C compilers on all CPU architectures pack these
 * structures without padding? */

struct igmp_full_pkt_s {
  struct eth_s e;
  struct ip_s i;
  struct ipopt_s o;
  struct igmp_s g;
};

struct udp_s {
  uint8_t udp_src_port[2];
  uint8_t udp_dst_port[2];
  uint8_t udp_len[2];
  uint8_t udp_chksum[2];
};

/* ??? Will all C compilers on all CPU architectures pack these
 * structures without padding? */

struct udp_full_pkt_s {
  struct eth_s e;
  struct ip_s i;
  struct udp_s u;
  uint8_t udp_data[1];
};


/* Globals for command-line parameters. */
int c_opt, e_opt, i_opt, g_opt, u_opt;  /* Options. */
char *intfc_str, *hex_data_str;  /* Required parameters. */

void usage()
{
  printf("Usage: raw_send [-h] [-e] [-c] [-g] [-i] [-u] interface 'hex_data'\n");
}  /* usage */

void help()
{
  usage();
  printf("Where:\n"
      "-h : help\n"
      "-c : overwrite IP checksum with calculated value.\n" 
      "-e : overwrite ethernet source address with interface MAC.\n"
      "-g : overwrite IGMP checksum with the calculated value.\n" 
      "-i : overwrite IP source address with interface address.\n" 
      "-u : overwrite UDP checksum with the calculated value.\n" 
      "interface : device name (e.g. 'eth0').\n"
      "hex_data : hex string with no spaces containing Ethernet headers, IP, etc.\n"
  );
}  /* help */


int hex2buffer(char *hex_str, uint8_t *buff, size_t buff_size)
{
  int len = strlen(hex_str);
  int i;
  uint8_t nibble_val;

  if ((len & 1) == 1) { printf("Error, hex string must have an even number of characters.\n"); exit(1); }
  if ((len/2) > buff_size) { printf("Error, hex string too large for buffer.\n"); exit(1); }

  for (i = 0; i < len; i++) {
    char c = hex_str[i];

    if (c >= '0' && c <= '9') { nibble_val = c - '0'; }
    else if (c >= 'A' && c <= 'F') { nibble_val = c - 'A' + 10; }
    else if (c >= 'a' && c <= 'f') { nibble_val = c - 'a' + 10; }
    else { printf("Error, hex string contains non-hex: %s\n", &hex_str[i]); exit(1); }

    if ((i & 1) == 0) {  /* Even character, most-significant nibble. */
      *buff = nibble_val << 4;
    } else {  /* Odd character, least-significant nibble. */
      *buff += nibble_val;
      buff++;
    }
  }  /* for */

  return len / 2;  /* Number of bytes. */
}  /* hex2buffer */


/* This calculates a partial checksum. It is intended for use when the data
 * to be checksumed is not a single contiguous byte array, but instead must
 * be calculated in chunks. To get the final checksum, you need to take
 * the result, invert it, and convert to network byte order. */
uint32_t running_checksum(uint8_t *data, int length, uint32_t cur_chksum) {
  int i;

  for (i = 0; i < length; i += 2) {
    uint16_t word = data[i] << 8;
    /* Handle case of IGMP which can have an odd number of bytes. */
    if (i + 1 < length) {
      word += data[i + 1];
    }
    cur_chksum += word;
    cur_chksum = (cur_chksum & 0xFFFF) + (cur_chksum >> 16);
  }

  return cur_chksum;
}  /* running_checksum */

/* Calculate checksum in simple case where data to be checksumed is a single
 * contiguous byte array. Return final checksum in network byte order. */
uint16_t net_checksum(uint8_t *data, int length) {
  uint32_t checksum;

  checksum = running_checksum(data, length, 0);

  return htons(~(uint16_t)checksum);
}  /* net_checksum */

/* Calculate checksum for a UDP frame.
 * Return final checksum in network byte order. */
uint16_t net_udp_checksum(struct udp_full_pkt_s *udp_full_pkt) {
  uint32_t checksum;
  uint8_t proto[2];
  int udp_len;
  uint16_t result;

  /* IPv4 UDP Pseudo header. */
  checksum = running_checksum((uint8_t *)udp_full_pkt->i.ip_src_ip, 4, 0);
  checksum = running_checksum((uint8_t *)udp_full_pkt->i.ip_dst_ip, 4, checksum);
  proto[0] = 0;  proto[1] = udp_full_pkt->i.ip_protocol;
  checksum = running_checksum(proto, 2, checksum);
  checksum = running_checksum(udp_full_pkt->u.udp_len, 2, checksum);

  /* Add in the actual UDP header and data. */
  udp_len = (udp_full_pkt->u.udp_len[0] << 8) + udp_full_pkt->u.udp_len[1];
  checksum = running_checksum((uint8_t *)&udp_full_pkt->u, udp_len, checksum);
  result = htons(~(uint16_t)checksum);
  /* If the final checksum would be zero, RFC 768 says to make it 0xffff. */
  if (result == 0) { result = 0xffff; }

  return htons(~(uint16_t)checksum);
}  /* net_udp_checksum */


void main_options(int argc, char **argv)
{
  int opt;

  c_opt = e_opt = g_opt = i_opt = u_opt = 0;

  while ((opt = getopt(argc, argv, "hcegiu")) != EOF) {
    switch (opt) {
      case 'h': { help();  exit(0);  break; }
      case 'c': { c_opt = 1;  break; }
      case 'e': { e_opt = 1;  break; }
      case 'i': { i_opt = 1;  break; }
      case 'g': { g_opt = 1;  break; }
      case 'u': { u_opt = 1;  break; }
      default: { usage();  exit(1); }
    }
  }  /* while */

  if (argc < (optind + 2)) {
    usage();  exit(1);
  }
  intfc_str = argv[optind];
  hex_data_str = argv[optind+1];
}  /* main_options */


int main(int argc, char **argv)
{
  int sockfd;
  struct ifreq ifr;
  struct ifreq if_idx;
  struct sockaddr_in *if_addr_in;
  int if_mtu;
  struct sockaddr_ll dest_sockaddr;
  struct in_addr src_ip;
  uint8_t src_mac[MAC_LEN];
  uint8_t *pkt_buffer;
  int pkt_buffer_size;
  int actual_size;
  struct udp_full_pkt_s *udp_full_pkt;
  struct igmp_full_pkt_s *igmp_full_pkt;
  uint16_t chksum;

  main_options(argc, argv);

  E(sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW));

  memset(&if_idx, 0, sizeof(if_idx));
  strncpy(if_idx.ifr_name, intfc_str, IFNAMSIZ-1);
  E(ioctl(sockfd, SIOCGIFINDEX, &if_idx));

  /* Get supplied interface's IP address. */
  ifr.ifr_addr.sa_family = AF_INET;
  strncpy(ifr.ifr_name, intfc_str, IFNAMSIZ-1);
  E(ioctl(sockfd, SIOCGIFADDR, &ifr));
  if_addr_in = (struct sockaddr_in *)(&ifr.ifr_addr);
  src_ip = if_addr_in->sin_addr;

  /* Get supplied interface's MAC address. */
  E(ioctl(sockfd, SIOCGIFHWADDR, &ifr));
  memcpy(src_mac, (uint8_t *)ifr.ifr_hwaddr.sa_data, MAC_LEN);

  /* Get MTU for interface. */
  ifr.ifr_addr.sa_family = AF_INET;
  strncpy(ifr.ifr_name, intfc_str, IFNAMSIZ-1);
  E(ioctl(sockfd, SIOCGIFMTU, &ifr));
  if_mtu = ifr.ifr_mtu;

  /* Create packet buffer. */
  pkt_buffer_size = if_mtu+14;  /* Include ethernet header. */
  pkt_buffer = malloc(pkt_buffer_size);
  udp_full_pkt = (struct udp_full_pkt_s *)pkt_buffer;
  igmp_full_pkt = (struct igmp_full_pkt_s *)pkt_buffer;

  /* Construct packet. */
  actual_size = hex2buffer(hex_data_str, pkt_buffer, pkt_buffer_size);

  if (e_opt) {
    memcpy((uint8_t *)&udp_full_pkt->e.eth_src, (uint8_t *)src_mac, 6);
  }

  if (i_opt) {
    memcpy((uint8_t *)&udp_full_pkt->i.ip_src_ip, (uint8_t *)&src_ip, 4);
  }

  if (c_opt) {
    memset(udp_full_pkt->i.ip_hdr_chksum, 0, 2);
    chksum = net_checksum((uint8_t *)&udp_full_pkt->i, (udp_full_pkt->i.ip_version_ihl & 0x0f) * 4);
    memcpy(udp_full_pkt->i.ip_hdr_chksum, (uint8_t *)&chksum, 2);
  }

  if (u_opt) {
    memset(udp_full_pkt->u.udp_chksum, 0, 2);
    chksum = net_udp_checksum(udp_full_pkt);
    memcpy(udp_full_pkt->u.udp_chksum, (uint8_t *)&chksum, 2);
  }

  if (g_opt) {
    memset(igmp_full_pkt->g.igmp_chksum, 0, 2);
    chksum = net_checksum((uint8_t *)&igmp_full_pkt->g, sizeof(igmp_full_pkt->g));
    memcpy(igmp_full_pkt->g.igmp_chksum, (uint8_t *)&chksum, 2);
  }

/*  { int i;
 *    for (i = 0; i < actual_size; i++) {
 *      if (i % 16 == 0) { printf("\n"); }
 *      printf("%02x ", pkt_buffer[i]);
 *    }
 *    printf("\n");
 *  }
 */

  /* Tell sendto which interface to send it over. */
  memset((uint8_t *)&dest_sockaddr, 0, sizeof(dest_sockaddr));
  dest_sockaddr.sll_family = AF_PACKET;
  dest_sockaddr.sll_ifindex = if_idx.ifr_ifindex;

  E(sendto(sockfd, pkt_buffer, actual_size, 0, (struct sockaddr *)&dest_sockaddr, sizeof(dest_sockaddr)));

  close(sockfd);
  return 0;
}  /* main */
