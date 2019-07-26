































#ifdef __FreeBSD__
#include <sys/cdefs.h>
__FBSDID("$FreeBSD: head/sys/netinet/sctp_bsd_addr.h 237540 2012-06-24 21:25:54Z tuexen $");
#endif

#ifndef _NETINET_SCTP_BSD_ADDR_H_
#define _NETINET_SCTP_BSD_ADDR_H_

#include <netinet/sctp_pcb.h>

#if defined(_KERNEL) || defined(__Userspace__)

extern struct iterator_control sctp_it_ctl;
void sctp_wakeup_iterator(void);

void sctp_startup_iterator(void);


#ifdef INET6
void sctp_gather_internal_ifa_flags(struct sctp_ifa *ifa);
#endif

#ifdef  SCTP_PACKET_LOGGING

void sctp_packet_log(struct mbuf *m);
int sctp_copy_out_packet_log(uint8_t *target, int length);

#endif

#if !defined(__Panda__)
void sctp_addr_change(struct ifaddr *ifa, int cmd);
#endif

void sctp_add_or_del_interfaces(int (*pred)(struct ifnet *), int add);

#endif
#endif
