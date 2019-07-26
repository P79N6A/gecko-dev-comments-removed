































#ifdef __FreeBSD__
#include <sys/cdefs.h>
__FBSDID("$FreeBSD: head/sys/netinet/sctp_peeloff.h 243516 2012-11-25 14:25:08Z tuexen $");
#endif

#ifndef _NETINET_SCTP_PEELOFF_H_
#define _NETINET_SCTP_PEELOFF_H_
#if defined(HAVE_SCTP_PEELOFF_SOCKOPT)

struct sctp_peeloff_opt {
#if !defined(__Windows__)
	int s;
#else
	HANDLE s;
#endif
	sctp_assoc_t assoc_id;
#if !defined(__Windows__)
	int new_sd;
#else
	HANDLE new_sd;
#endif
};
#endif 
#if defined(_KERNEL)
int sctp_can_peel_off(struct socket *, sctp_assoc_t);
int sctp_do_peeloff(struct socket *, struct socket *, sctp_assoc_t);
#if defined(HAVE_SCTP_PEELOFF_SOCKOPT)
struct socket *sctp_get_peeloff(struct socket *, sctp_assoc_t, int *);
int sctp_peeloff_option(struct proc *p, struct sctp_peeloff_opt *peeloff);
#endif 
#endif 
#if defined(__Userspace__)
int sctp_can_peel_off(struct socket *, sctp_assoc_t);
int sctp_do_peeloff(struct socket *, struct socket *, sctp_assoc_t);
#endif 
#endif 
