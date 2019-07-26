






























#ifdef __FreeBSD__
#include <sys/cdefs.h>
__FBSDID("$FreeBSD: head/sys/netinet/sctp_dtrace_declare.h 235828 2012-05-23 11:26:28Z tuexen $");
#endif

#ifndef _NETINET_SCTP_DTRACE_DECLARE_H_
#define _NETINET_SCTP_DTRACE_DECLARE_H_

#if defined(__FreeBSD__) && __FreeBSD_version >= 803000
#include "opt_kdtrace.h"
#include <sys/kernel.h>
#include <sys/sdt.h>


SDT_PROVIDER_DECLARE(sctp);





SDT_PROBE_DECLARE(sctp, cwnd, net, init);

SDT_PROBE_DECLARE(sctp, cwnd, net, ack);

SDT_PROBE_DECLARE(sctp, cwnd, net, fr);

SDT_PROBE_DECLARE(sctp, cwnd, net, to);

SDT_PROBE_DECLARE(sctp, cwnd, net, bl);

SDT_PROBE_DECLARE(sctp, cwnd, net, ecn);

SDT_PROBE_DECLARE(sctp, cwnd, net, pd);

SDT_PROBE_DECLARE(sctp, cwnd, net, rttvar);
SDT_PROBE_DECLARE(sctp, cwnd, net, rttstep);


SDT_PROBE_DECLARE(sctp, rwnd, assoc, val);


SDT_PROBE_DECLARE(sctp, flightsize, net, val);


SDT_PROBE_DECLARE(sctp, flightsize, assoc, val);






#else

#ifndef SDT_PROBE
#define SDT_PROBE(a, b, c, d, e, f, g, h, i)
#endif
#endif
#endif
