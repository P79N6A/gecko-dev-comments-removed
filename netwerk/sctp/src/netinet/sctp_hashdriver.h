































#ifdef __FreeBSD__
#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");
#endif

#ifndef _NETINET_SCTP_HASHDRIVER_H_
#define _NETINET_SCTP_HASHDRIVER_H_


void sctp_hash_digest(char *, int, char *, int, unsigned char *);

void sctp_hash_digest_m(char *, int, struct mbuf *, int, unsigned char *);

#endif
