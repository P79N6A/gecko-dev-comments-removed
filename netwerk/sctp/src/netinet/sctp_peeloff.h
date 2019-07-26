































#ifdef __FreeBSD__
#include <sys/cdefs.h>
__FBSDID("$FreeBSD: head/sys/netinet/sctp_peeloff.h 235828 2012-05-23 11:26:28Z tuexen $");
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
struct socket *sctp_get_peeloff(struct socket *, sctp_assoc_t, int *);

#if defined(HAVE_SCTP_PEELOFF_SOCKOPT)
int sctp_peeloff_option(struct proc *p, struct sctp_peeloff_opt *peeloff);

#endif				

#ifdef __APPLE__

struct sctp_peeloff_args {
	int s;
	caddr_t name;
};

#endif				

#endif				

#endif
