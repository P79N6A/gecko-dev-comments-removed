































#ifdef __FreeBSD__
#include <sys/cdefs.h>
__FBSDID("$FreeBSD: head/sys/netinet/sctp_input.h 273168 2014-10-16 15:36:04Z tuexen $");
#endif

#ifndef _NETINET_SCTP_INPUT_H_
#define _NETINET_SCTP_INPUT_H_

#if defined(_KERNEL) || defined(__Userspace__)
void
sctp_common_input_processing(struct mbuf **, int, int, int,
                             struct sockaddr *, struct sockaddr *,
                             struct sctphdr *, struct sctp_chunkhdr *,
#if !defined(SCTP_WITH_NO_CSUM)
                             uint8_t,
#endif
                             uint8_t,
#if defined(__FreeBSD__)
                             uint8_t, uint32_t,
#endif
                             uint32_t, uint16_t);

struct sctp_stream_reset_request *
sctp_find_stream_reset(struct sctp_tcb *stcb, uint32_t seq,
    struct sctp_tmit_chunk **bchk);

void sctp_reset_in_stream(struct sctp_tcb *stcb, uint32_t number_entries,
    uint16_t *list);


int sctp_is_there_unsent_data(struct sctp_tcb *stcb, int so_locked);

#endif
#endif
