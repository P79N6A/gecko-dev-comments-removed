































#ifdef __FreeBSD__
#include <sys/cdefs.h>
__FBSDID("$FreeBSD: head/sys/netinet/sctp_asconf.h 237715 2012-06-28 16:01:08Z tuexen $");
#endif

#ifndef _NETINET_SCTP_ASCONF_H_
#define _NETINET_SCTP_ASCONF_H_

#if defined(_KERNEL) || defined(__Userspace__)




extern void sctp_asconf_cleanup(struct sctp_tcb *, struct sctp_nets *);

extern struct mbuf *sctp_compose_asconf(struct sctp_tcb *, int *, int);

extern void
sctp_handle_asconf(struct mbuf *, unsigned int, struct sockaddr *,
                   struct sctp_asconf_chunk *, struct sctp_tcb *, int);

extern void
sctp_handle_asconf_ack(struct mbuf *, int, struct sctp_asconf_ack_chunk *,
     struct sctp_tcb *, struct sctp_nets *, int *);

extern uint32_t
sctp_addr_mgmt_ep_sa(struct sctp_inpcb *, struct sockaddr *,
		     uint32_t, uint32_t, struct sctp_ifa *);


extern int sctp_asconf_iterator_ep(struct sctp_inpcb *inp, void *ptr,
				   uint32_t val);
extern void sctp_asconf_iterator_stcb(struct sctp_inpcb *inp,
				      struct sctp_tcb *stcb,
				      void *ptr, uint32_t type);
extern void sctp_asconf_iterator_end(void *ptr, uint32_t val);


extern int32_t
sctp_set_primary_ip_address_sa(struct sctp_tcb *,
    struct sockaddr *);

extern void
sctp_set_primary_ip_address(struct sctp_ifa *ifa);

extern void
sctp_check_address_list(struct sctp_tcb *, struct mbuf *, int, int,
    struct sockaddr *, uint16_t, uint16_t, uint16_t, uint16_t);

extern void
sctp_assoc_immediate_retrans(struct sctp_tcb *, struct sctp_nets *);
#if defined(__FreeBSD__) || defined(__APPLE__) || defined(__Userspace__)
extern void
sctp_net_immediate_retrans(struct sctp_tcb *, struct sctp_nets *);
#endif

extern void
sctp_asconf_send_nat_state_update(struct sctp_tcb *stcb,
				  struct sctp_nets *net);

extern int
sctp_is_addr_pending(struct sctp_tcb *, struct sctp_ifa *);
#endif				

#endif				
