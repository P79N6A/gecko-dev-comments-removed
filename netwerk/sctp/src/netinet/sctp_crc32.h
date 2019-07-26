































#ifdef __FreeBSD__
#include <sys/cdefs.h>
__FBSDID("$FreeBSD: head/sys/netinet/sctp_crc32.h 235828 2012-05-23 11:26:28Z tuexen $");
#endif

#ifndef _NETINET_SCTP_CRC32_H_
#define _NETINET_SCTP_CRC32_H_

#if defined(_KERNEL)
#if !defined(SCTP_WITH_NO_CSUM)
uint32_t sctp_calculate_cksum(struct mbuf *, uint32_t);
#endif
void sctp_delayed_cksum(struct mbuf *, uint32_t offset);
#endif				
#if defined(__Userspace__)
#if !defined(SCTP_WITH_NO_CSUM)
uint32_t sctp_calculate_cksum(struct mbuf *, uint32_t);
#endif
#endif
#endif				
