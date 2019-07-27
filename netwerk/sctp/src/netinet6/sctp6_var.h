































#ifdef __FreeBSD__
#include <sys/cdefs.h>
__FBSDID("$FreeBSD: head/sys/netinet6/sctp6_var.h 243186 2012-11-17 20:04:04Z tuexen $");
#endif

#ifndef _NETINET6_SCTP6_VAR_H_
#define _NETINET6_SCTP6_VAR_H_

#if defined(__Userspace__)
#ifdef INET
extern void in6_sin6_2_sin(struct sockaddr_in *, struct sockaddr_in6 *);
extern void in6_sin6_2_sin_in_sock(struct sockaddr *);
extern void in6_sin_2_v4mapsin6(struct sockaddr_in *, struct sockaddr_in6 *);
#endif
#endif
#if defined(_KERNEL)

#if defined(__FreeBSD__) || (__APPLE__) || defined(__Windows__)
SYSCTL_DECL(_net_inet6_sctp6);
extern struct pr_usrreqs sctp6_usrreqs;
#else
int sctp6_usrreq(struct socket *, int, struct mbuf *, struct mbuf *, struct mbuf *);
#endif

#if defined(__APPLE__)
int sctp6_input(struct mbuf **, int *);
int sctp6_input_with_port(struct mbuf **, int *, uint16_t);
#elif defined(__Panda__)
int sctp6_input (pakhandle_type *);
#elif defined(__FreeBSD__) && __FreeBSD_version < 902000
int sctp6_input __P((struct mbuf **, int *, int));
int sctp6_input_with_port __P((struct mbuf **, int *, uint16_t));
#else
int sctp6_input(struct mbuf **, int *, int);
int sctp6_input_with_port(struct mbuf **, int *, uint16_t);
#endif
#if defined(__FreeBSD__) && __FreeBSD_version < 902000
int sctp6_output
__P((struct sctp_inpcb *, struct mbuf *, struct sockaddr *,
     struct mbuf *, struct proc *));
void sctp6_ctlinput __P((int, struct sockaddr *, void *));
#else
int sctp6_output(struct sctp_inpcb *, struct mbuf *, struct sockaddr *,
                 struct mbuf *, struct proc *);
void sctp6_ctlinput(int, struct sockaddr *, void *);
#endif
#if !(defined(__FreeBSD__) || defined(__APPLE__))
extern void in6_sin_2_v4mapsin6(struct sockaddr_in *, struct sockaddr_in6 *);
extern void in6_sin6_2_sin(struct sockaddr_in *, struct sockaddr_in6 *);
extern void in6_sin6_2_sin_in_sock(struct sockaddr *);
#endif
extern void sctp6_notify(struct sctp_inpcb *, struct icmp6_hdr *,
                         struct sctphdr *, struct sockaddr *,
                         struct sctp_tcb *, struct sctp_nets *);
#endif
#endif
