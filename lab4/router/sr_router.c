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

#include "sr_if.h"
#include "sr_rt.h"
#include "sr_router.h"
#include "sr_protocol.h"
#include "sr_arpcache.h"
#include "sr_utils.h"
#include "vnscommand.h"

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
			sr_handle_arp(sr, packet+sizeof(sr_ethernet_hdr_t), len-sizeof(sr_ethernet_hdr_t), interface);
      break;
    case ethertype_ip:
      break;
  }
}/* end sr_ForwardPacket */

void sr_handle_arp(struct sr_instance* sr, uint8_t * buf, unsigned int len, char* interface) {
	sr_arp_hdr_t* arp = (sr_arp_hdr_t*) buf;
	enum sr_arp_opcode op = (enum sr_arp_opcode)ntohs(arp->ar_op);
	struct sr_if* iface = sr_get_interface(sr, interface);
	switch(op) {
		case arp_op_request : 
			printf("Sending ARP reply\n");
			send_arp_rep(sr, iface, arp);
			break;
		case arp_op_reply :
			/* add mac and ip mapping */
			printf("Updating arp cache\n");
			sr_arpcache_insert(&sr->cache, arp->ar_sha, arp->ar_sip);
			/* sr_arpreq_destroy is handled in arpcache */
			break;
	}
}


int send_arp_rep(struct sr_instance* sr, struct sr_if* req_if, sr_arp_hdr_t* req){
    printf("sending arp_reply\n");
    uint8_t* block = malloc(sizeof(sr_arp_hdr_t)+sizeof(sr_ethernet_hdr_t));
	sr_arp_hdr_t* arp_hdr = (sr_arp_hdr_t*)(block+sizeof(sr_ethernet_hdr_t));
    sr_ethernet_hdr_t* eth_hdr = (sr_ethernet_hdr_t*)(block);

    /* modify/populate ARP header */
    arp_hdr->ar_hrd = htons(arp_hrd_ethernet);
    arp_hdr->ar_pro = htons(0x0800);
	arp_hdr->ar_hln = ETHER_ADDR_LEN;
	arp_hdr->ar_pln = sizeof(uint32_t);
	arp_hdr->ar_op  = htons(arp_op_reply);
	memcpy(arp_hdr->ar_sha, req_if->addr, ETHER_ADDR_LEN);
	arp_hdr->ar_sip = req_if->ip;
	memcpy(arp_hdr->ar_tha, req->ar_sha, ETHER_ADDR_LEN);
	arp_hdr->ar_tip = req->ar_sip;

    /* modify/populate MAC header */
    memcpy(eth_hdr->ether_dhost, req->ar_sha, ETHER_ADDR_LEN);
    memcpy(eth_hdr->ether_shost, req_if->addr, ETHER_ADDR_LEN);
    eth_hdr->ether_type = htons(ethertype_arp);

	int ret = sr_send_packet(sr, block, sizeof(sr_arp_hdr_t)+sizeof(sr_ethernet_hdr_t), req_if->name);
    if(ret != 0) { printf("ARP failed to send\n"); }
	free(block);
	return ret;
}