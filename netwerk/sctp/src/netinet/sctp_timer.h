































#ifdef __FreeBSD__
#include <sys/cdefs.h>
__FBSDID("$FreeBSD: head/sys/netinet/sctp_timer.h 235828 2012-05-23 11:26:28Z tuexen $");
#endif

#ifndef _NETINET_SCTP_TIMER_H_
#define _NETINET_SCTP_TIMER_H_

#if defined(_KERNEL) || defined(__Userspace__)

#define SCTP_RTT_SHIFT 3
#define SCTP_RTT_VAR_SHIFT 2

struct sctp_nets *
sctp_find_alternate_net(struct sctp_tcb *,
    struct sctp_nets *, int mode);

int
sctp_threshold_management(struct sctp_inpcb *, struct sctp_tcb *,
    struct sctp_nets *, uint16_t);

int
sctp_t3rxt_timer(struct sctp_inpcb *, struct sctp_tcb *,
    struct sctp_nets *);
int
sctp_t1init_timer(struct sctp_inpcb *, struct sctp_tcb *,
    struct sctp_nets *);
int
sctp_shutdown_timer(struct sctp_inpcb *, struct sctp_tcb *,
    struct sctp_nets *);
int
sctp_heartbeat_timer(struct sctp_inpcb *, struct sctp_tcb *,
    struct sctp_nets *);

int
sctp_cookie_timer(struct sctp_inpcb *, struct sctp_tcb *,
    struct sctp_nets *);

void
sctp_pathmtu_timer(struct sctp_inpcb *, struct sctp_tcb *,
    struct sctp_nets *);

int
sctp_shutdownack_timer(struct sctp_inpcb *, struct sctp_tcb *,
    struct sctp_nets *);
int
sctp_strreset_timer(struct sctp_inpcb *inp, struct sctp_tcb *stcb,
    struct sctp_nets *net);

int
sctp_asconf_timer(struct sctp_inpcb *, struct sctp_tcb *,
    struct sctp_nets *);

void
sctp_delete_prim_timer(struct sctp_inpcb *, struct sctp_tcb *,
    struct sctp_nets *);

void
sctp_autoclose_timer(struct sctp_inpcb *, struct sctp_tcb *,
    struct sctp_nets *net);

void sctp_audit_retranmission_queue(struct sctp_association *);

void sctp_iterator_timer(struct sctp_iterator *it);

#if defined(__APPLE__)
void sctp_slowtimo();

#endif

#endif
#endif
