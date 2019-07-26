































#ifdef __FreeBSD__
#include <sys/cdefs.h>
__FBSDID("$FreeBSD: head/sys/netinet6/sctp6_var.h 235828 2012-05-23 11:26:28Z tuexen $");
#endif

#ifndef _NETINET6_SCTP6_VAR_H_
#define _NETINET6_SCTP6_VAR_H_

#if defined(__Userspace__)
extern void in6_sin6_2_sin(struct sockaddr_in *, struct sockaddr_in6 *sin6);
#endif
#if defined(_KERNEL)

#if defined(__FreeBSD__) || (__APPLE__) || defined(__Windows__)
SYSCTL_DECL(_net_inet6_sctp6);
extern struct pr_usrreqs sctp6_usrreqs;

#else
int sctp6_usrreq
__P((struct socket *, int, struct mbuf *, struct mbuf *,
    struct mbuf *));

#endif				

#if defined(__APPLE__)
int sctp6_input __P((struct mbuf **, int *));
#elif defined(__Panda__)
int sctp6_input (pakhandle_type *);
#else
int sctp6_input __P((struct mbuf **, int *, int));
#endif
int sctp6_output
__P((struct sctp_inpcb *, struct mbuf *, struct sockaddr *,
     struct mbuf *, struct proc *));
void sctp6_ctlinput __P((int, struct sockaddr *, void *));

#if !(defined(__FreeBSD__) || defined(__APPLE__))
extern void in6_sin_2_v4mapsin6(struct sockaddr_in *sin,
				struct sockaddr_in6 *sin6);
extern void in6_sin6_2_sin(struct sockaddr_in *, struct sockaddr_in6 *sin6);
extern void in6_sin6_2_sin_in_sock(struct sockaddr *nam);
#endif

extern void sctp6_notify(struct sctp_inpcb *inp,
    struct icmp6_hdr *icmph,
    struct sctphdr *sh,
    struct sockaddr *to,
    struct sctp_tcb *stcb,
    struct sctp_nets *net);


#endif				
#endif
