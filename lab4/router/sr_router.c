/**********************************************************************
 * file:  sr_router.c
 * date:  Mon Feb 18 12:50:42 PST 2002
 * Contact: casado@stanford.edu
 *
 * Description:
 *
 * This file contains all the functions that interact directly
 * with the routing table, as well as the main entry method
 * for routing.
 *
 **********************************************************************/

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "sr_if.h"
#include "sr_rt.h"
#include "sr_router.h"
#include "sr_protocol.h"
#include "sr_arpcache.h"
#include "sr_utils.h"
#include "vnscommand.h"


void sr_handle_ip(struct sr_instance* sr, uint8_t * buf, unsigned int len,char* interface);
struct sr_rt *prefix_match(struct sr_instance * sr, uint32_t addr);
void icmp_unreachable(struct sr_instance * sr, uint8_t code, sr_ip_hdr_t * ip, char* interface);
void handle_icmp(struct sr_instance* sr, uint8_t * buf, unsigned int len, char* interface);
void sr_icmp_send_message(struct sr_instance* sr, uint8_t type, uint8_t code, sr_ip_hdr_t * ip, char* interface);
bool is_own_ip(struct sr_instance* sr, sr_ip_hdr_t* current);
void sr_handle_arp(struct sr_instance* sr, uint8_t * buf, unsigned int len, char* interface);
void send_arp_rep(struct sr_instance* sr, struct sr_if* iface, sr_arp_hdr_t* arp);


/*---------------------------------------------------------------------
 * Method: sr_init(void)
 * Scope:  Global
 *
 * Initialize the routing subsystem
 *
 *---------------------------------------------------------------------*/

void sr_init(struct sr_instance* sr)
{
    /* REQUIRES */
    assert(sr);

    /* Initialize cache and cache cleanup thread */
    sr_arpcache_init(&(sr->cache));

    pthread_attr_init(&(sr->attr));
    pthread_attr_setdetachstate(&(sr->attr), PTHREAD_CREATE_JOINABLE);
    pthread_attr_setscope(&(sr->attr), PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setscope(&(sr->attr), PTHREAD_SCOPE_SYSTEM);
    pthread_t arp_thread;

    pthread_create(&arp_thread, &(sr->attr), sr_arpcache_timeout, sr);
    
    srand(time(NULL));
    pthread_mutexattr_init(&(sr->rt_lock_attr));
    pthread_mutexattr_settype(&(sr->rt_lock_attr), PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&(sr->rt_lock), &(sr->rt_lock_attr));

    pthread_attr_init(&(sr->rt_attr));
    pthread_attr_setdetachstate(&(sr->rt_attr), PTHREAD_CREATE_JOINABLE);
    pthread_attr_setscope(&(sr->rt_attr), PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setscope(&(sr->rt_attr), PTHREAD_SCOPE_SYSTEM);
    pthread_t rt_thread;
    pthread_create(&rt_thread, &(sr->rt_attr), sr_rip_timeout, sr);
    
    /* Add initialization code here! */

} /* -- sr_init -- */

/*---------------------------------------------------------------------
 * Method: sr_handlepacket(uint8_t* p,char* interface)
 * Scope:  Global
 *
 * This method is called each time the router receives a packet on the
 * interface.  The packet buffer, the packet length and the receiving
 * interface are passed in as parameters. The packet is complete with
 * ethernet headers.
 *
 * Note: Both the packet buffer and the character's memory are handled
 * by sr_vns_comm.c that means do NOT delete either.  Make a copy of the
 * packet instead if you intend to keep it around beyond the scope of
 * the method call.
 *
 *---------------------------------------------------------------------*/

void sr_handlepacket(struct sr_instance* sr,
        uint8_t * packet/* lent */,
        unsigned int len,
        char* interface/* lent */)
{
  /* REQUIRES */
  assert(sr);
  assert(packet);
  assert(interface);

  printf("*** -> Received packet of length %d \n",len);

  /* Lab4: Fill your code here */
  uint16_t ethtype = ethertype(packet);

  switch(ethtype) {
 		case ethertype_arp:
      /* case1 */
      printf("handle_arp\n");
			sr_handle_arp(sr, packet+sizeof(sr_ethernet_hdr_t), len-sizeof(sr_ethernet_hdr_t), interface);
      break;
    case ethertype_ip:
      /* case2 */
      printf("handle_ip\n");
      sr_handle_ip(sr, packet+sizeof(sr_ethernet_hdr_t), len-sizeof(sr_ethernet_hdr_t), interface);
      break;
  }
}/* end sr_ForwardPacket */

void sr_handle_ip(struct sr_instance* sr, uint8_t * buf, unsigned int len,char* interface){
  /*2a Check whether the checksum in the IP header is correct. 
  If the checksum is not correct, just ignore this packet and return. 
  Recall the Internet checksum algorithm returns zero if there is no bit error*/
  sr_ip_hdr_t* ip = (sr_ip_hdr_t*)buf;
  uint16_t received = ip->ip_sum;
  ip->ip_sum = 0;
  uint16_t calc = cksum(ip,sizeof(sr_ip_hdr_t));
  if(calc!=received){
    printf("something went wrong with checksum");
    return;
  }
  ip->ip_sum = calc;
  /*if(cksum(ip,sizeof(sr_ip_hdr_t))!=0){
    printf("something went wrong with checksum");
    return;
  }*/
  printf("checksum fine\n");
  /*2b If the destination IP of this packet is router’s own IP */
  if(is_own_ip(sr,ip)){
    printf("is own ip\n");
    uint8_t ip_proto = ip_protocol(buf);
    /*2b1 */
    if (ip_proto == ip_protocol_icmp) {
      printf("isicmp\n");
      handle_icmp(sr, buf , len , interface);
    }
    /*2b2 */
    else{
      printf("is unreachable\n");
      icmp_unreachable(sr, Unreachable_port_code, ip, interface);
    }

  }
  /*2c*/
  else{
    /*2c1 Check whether the TTL in the IP header equals 1. 
    If TTL=1, your router should reply an ICMP Time Exceeded message back to the Sender*/
    if(ip->ip_ttl == 1){
      sr_icmp_send_message(sr, TimeExceededType, TimeExceededCode, (sr_ip_hdr_t *)ip, interface);
    }
    else{
      /*2c2 Otherwise, check whether the destination IP address is in your routing table.*/
      /*If you can not find this destination IP in your routing table, 
      you should send an ICMP DEST_NET_UNREACHABLE message back to the Sender. 
      You should implement a Longest Prefix Matching here.*/
      struct sr_rt * match = prefix_match(sr,ip->ip_dst);
      if(match==NULL){
        icmp_unreachable(sr, Unreachable_net_code, ip, interface);
      }
      else{
        /*2c3*/

      }
    }
  }
}



struct sr_rt *prefix_match(struct sr_instance * sr, uint32_t addr){
  struct sr_rt * table = sr->routing_table;
  int max_len = -1;
	struct sr_rt * ans = NULL;

  while (table != NULL) {
		in_addr_t left = (table->mask.s_addr & addr);
		in_addr_t right = (table->dest.s_addr & table->mask.s_addr);
		if (left == right) {
      uint8_t size = 0;
      uint32_t checker = 1 << 31;
      while ((checker != 0) && ((checker & table->mask.s_addr) != 0)) {
        size++;
        checker = checker >> 1;
      }
      if (size > max_len) {
				max_len = size;
				ans = table;
			}
		}
		table = table->next;
	}
  return ans;
}

void icmp_unreachable(struct sr_instance * sr, uint8_t code, sr_ip_hdr_t * ip, char* interface){
  /*malloc*/
  sr_ethernet_hdr_t * block = (sr_ethernet_hdr_t *) malloc(sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_hdr_t));
  /*ethernet header*/
  uint8_t * ether_shost = malloc(sizeof(unsigned char) * ETHER_ADDR_LEN);
  struct sr_if * iface = sr_get_interface(sr, interface);
  memcpy((void*) ether_shost, iface->addr, sizeof(unsigned char) * ETHER_ADDR_LEN);

  uint8_t * ether_dhost = malloc(sizeof(unsigned char) * ETHER_ADDR_LEN);
  struct sr_arpentry * entry = sr_arpcache_lookup( &(sr->cache), ip->ip_src);
  memcpy(ether_dhost, entry->mac, sizeof(unsigned char) * ETHER_ADDR_LEN);

  memcpy((void *) block->ether_dhost, (void *) ether_dhost, sizeof(uint8_t) * ETHER_ADDR_LEN);
  memcpy((void *) block->ether_shost, (void *) ether_shost, sizeof(uint8_t) * ETHER_ADDR_LEN);
  block->ether_type = htons(ethertype_ip);

  
  /*ip header*/
  uint32_t ip_src = ntohl(ip->ip_dst);
  uint32_t ip_dst= ntohl(ip->ip_src);
  sr_ip_hdr_t* pkt = (sr_ip_hdr_t *)(block + sizeof(sr_ethernet_hdr_t));
  pkt->ip_hl = 0x5;
  pkt->ip_v  = 4;
  pkt->ip_tos = iptos;
  pkt->ip_len = htons((uint16_t) (sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_hdr_t)));
  pkt->ip_id = htons(ipid);
  pkt->ip_off = htons(ipoff);
  pkt->ip_ttl = ipttl;
  pkt->ip_p = ip_protocol_icmp;
  pkt->ip_sum = 0;
  pkt->ip_src = htonl(ip_src);
  pkt->ip_dst = htonl(ip_dst);
  pkt->ip_sum = cksum(((void *) pkt), sizeof(sr_ip_hdr_t));

  /*icmp header*/
  sr_icmp_t3_hdr_t* icmp_t3_hdr = (sr_icmp_t3_hdr_t*)(block + sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t));
  icmp_t3_hdr->icmp_type = htons(t3_type);
  icmp_t3_hdr->icmp_code = htons(code);
  icmp_t3_hdr->next_mtu = Next_mtu;
  icmp_t3_hdr->icmp_sum = 0;
  icmp_t3_hdr->unused = Unused;

  memcpy((icmp_t3_hdr->data), (uint8_t *) ip, sizeof(uint8_t) * ICMP_DATA_SIZE);
  icmp_t3_hdr->icmp_sum = cksum(icmp_t3_hdr, sizeof(sr_icmp_t3_hdr_t));


  unsigned int packet_len = sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_hdr_t);
  sr_send_packet(sr, (uint8_t*) block, packet_len, interface );
  free(block);
  free(ether_shost);
  free(ether_dhost);
  
}

/*2b1*/
void handle_icmp(struct sr_instance* sr, uint8_t * buf, unsigned int len, char* interface){
  sr_ip_hdr_t * ip_hdr = (sr_ip_hdr_t *)(buf);
  sr_icmp_hdr_t * icmp_hdr = (sr_icmp_hdr_t *) (((void *) buf)+ sizeof(sr_ip_hdr_t));
  uint8_t type = icmp_hdr->icmp_type;
  if(type==Echorequest){
    /*2b12*/
    printf("is echo request\n");
    sr_icmp_send_message(sr, Echoreply, Echoreply, ip_hdr, interface);
  }
  /*2b11 If this is not an ICMP ECHO packet, your router can ignore this packet*/
  else{
    printf("ignoring packet\n");
  }
}

/*void fillin(struct sr_instance* sr,sr_ip_hdr_t * ip, char* interface,sr_ethernet_hdr_t * block)*/

void sr_icmp_send_message(struct sr_instance* sr, uint8_t type, uint8_t code, sr_ip_hdr_t * ip, char* interface){
  printf("sending icmp\n");
  /*2b12a Malloc a space to store ethernet header and IP header and ICMP header*/
  sr_ethernet_hdr_t * block = (sr_ethernet_hdr_t *) malloc(sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_hdr_t));
  printf("filling in\n");
  /*fillin( sr,ip, interface,block);*/
  /*2b12d,e Fill the Source MAC Address, Destination MAC Address, Ethernet Type in ethernet header*/
  uint8_t * ether_shost = malloc(sizeof(unsigned char) * ETHER_ADDR_LEN);
  struct sr_if * iface = sr_get_interface(sr, interface);
  memcpy((void*) ether_shost, iface->addr, sizeof(unsigned char) * ETHER_ADDR_LEN);
  printf("shost finished\n");

  uint8_t * ether_dhost = malloc(sizeof(unsigned char) * ETHER_ADDR_LEN);
  printf("print1\n");
  struct sr_arpentry * entry = sr_arpcache_lookup( &(sr->cache), ip->ip_src);
  printf("print2\n");
  memcpy(ether_dhost, entry->mac, sizeof(unsigned char) * ETHER_ADDR_LEN); /*something goes wrong here*/
  printf("dhost finished\n");

  memcpy((void *) block->ether_dhost, (void *) ether_dhost, sizeof(uint8_t) * ETHER_ADDR_LEN);
  memcpy((void *) block->ether_shost, (void *) ether_shost, sizeof(uint8_t) * ETHER_ADDR_LEN);
  block->ether_type = htons(ethertype_ip);

  /*struct sr_if * iface = sr_get_interface(sr, interface);
  struct sr_arpentry * entry = sr_arpcache_lookup( &(sr->cache), ip->ip_src);
  memcpy(block->ether_dhost, entry->mac, ETHER_ADDR_LEN);
  memcpy(block->ether_shost, iface->addr, ETHER_ADDR_LEN);
  block->ether_type = htons(ethertype_ip);
  printf("ip\n");*/
  /*2b12cFill the source IP address, destination IP address, ttl, protocol, length, checksum in IP header*/
  uint32_t ip_src = ntohl(ip->ip_dst);
  uint32_t ip_dst= ntohl(ip->ip_src);
  sr_ip_hdr_t* pkt = (sr_ip_hdr_t *)(block + sizeof(sr_ethernet_hdr_t));
  printf("%i, ipid\n", ip->ip_id);
  pkt->ip_hl = 0x5;
  pkt->ip_v  = 4;
  pkt->ip_tos = iptos;
  pkt->ip_len = htons((uint16_t) (sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_hdr_t)));
  pkt->ip_id = htons(ip->ip_id);
  pkt->ip_off = htons(ipoff);
  pkt->ip_ttl = ipttl;
  pkt->ip_p = ip_protocol_icmp;
  pkt->ip_sum = 0;
  pkt->ip_src = htonl(ip_src);
  pkt->ip_dst = htonl(ip_dst);
  pkt->ip_sum = cksum((void *) pkt, sizeof(sr_ip_hdr_t));

  /*2b12b Fill the ICMP code, type in ICMP header*/
  sr_icmp_hdr_t* icmp_hdr = (sr_icmp_hdr_t*)(block + sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t));
  icmp_hdr->icmp_type = type;
  icmp_hdr->icmp_code = code;
  icmp_hdr->icmp_sum  = 0;
  icmp_hdr->icmp_sum = cksum((void *)icmp_hdr, sizeof(sr_icmp_hdr_t));

  unsigned int packet_len = sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_hdr_t);
  /*2b13 Send this ICMP Reply packet back to the Sender*/

  print_hdr_eth((uint8_t *)block);
  print_hdr_ip((uint8_t *)(block + sizeof(sr_ethernet_hdr_t) ));
  print_hdr_icmp((uint8_t *)(block + sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t)));
  sr_send_packet(sr, (uint8_t*) block, packet_len, interface );
  free(block);
  free(ether_shost);
  free(ether_dhost);
}


bool is_own_ip(struct sr_instance* sr, sr_ip_hdr_t* current) {
  printf("is_own_ip\n");
	struct sr_if * iface = sr->if_list;
  printf("iface established\n");
	while (iface != NULL) {
    printf("not null\n");
		if (current->ip_dst == iface->ip) {
			return true;
		}
		iface = iface->next;
	}
	return false;
}

void sr_handle_arp(struct sr_instance* sr, uint8_t * buf, unsigned int len, char* interface) {
	sr_arp_hdr_t* arp = (sr_arp_hdr_t*) buf;
	enum sr_arp_opcode op = (enum sr_arp_opcode)ntohs(arp->ar_op);
	struct sr_if* iface = sr_get_interface(sr, interface);
	switch(op) {
		case arp_op_request : 
      /* case 1a */
      sr_arpcache_insert(&sr->cache, arp->ar_sha, arp->ar_sip); /*1a1 Insert the Sender MAC in this packet to your ARP cache*/
      /* TODO: 1a2: optimization? */
			send_arp_rep(sr, iface, arp); /*1a3,4*/
			break;
		case arp_op_reply :
      /* case 1b */
			sr_arpcache_insert(&sr->cache, arp->ar_tha, arp->ar_tip); /* 1b1 Insert the Target MAC to your ARP cache*/
      /* TODO: 1b2 */
			break;
	}
}


void send_arp_rep(struct sr_instance* sr, struct sr_if* iface, sr_arp_hdr_t* arp){

  /* 1 Malloc a space to store an Ethernet header and ARP header */
  uint8_t* block = malloc(sizeof(sr_arp_hdr_t)+sizeof(sr_ethernet_hdr_t));
	sr_arp_hdr_t* arphdr = (sr_arp_hdr_t*)(block+sizeof(sr_ethernet_hdr_t));
  sr_ethernet_hdr_t* ethhdr = (sr_ethernet_hdr_t*)(block);

  /* 2 ill the ARP opcode, Sender IP, Sender MAC, Target IP, Target MAC in ARP header*/
  arphdr->ar_hrd = htons(arp_hrd_ethernet);
  arphdr->ar_pro = htons(0x0800);
	arphdr->ar_hln = ETHER_ADDR_LEN;
	arphdr->ar_pln = sizeof(uint32_t);
	arphdr->ar_op  = htons(arp_op_reply);
	memcpy(arphdr->ar_sha, iface->addr, ETHER_ADDR_LEN);
	arphdr->ar_sip = iface->ip;
	memcpy(arphdr->ar_tha, arp->ar_sha, ETHER_ADDR_LEN);
	arphdr->ar_tip = arp->ar_sip;

  /* 3 Fill the Source MAC Address, Destination MAC Address, Ethernet Type in the Ethernet header */
  memcpy(ethhdr->ether_dhost, arp->ar_sha, ETHER_ADDR_LEN);
  memcpy(ethhdr->ether_shost, iface->addr, ETHER_ADDR_LEN);
  ethhdr->ether_type = htons(ethertype_arp);

  /* Send this ARP response back to the Sender */
	sr_send_packet(sr, block, sizeof(sr_arp_hdr_t)+sizeof(sr_ethernet_hdr_t), iface->name);
	free(block);
	return;
}