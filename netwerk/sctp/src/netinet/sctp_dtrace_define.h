






























#ifdef __FreeBSD__
#include <sys/cdefs.h>
__FBSDID("$FreeBSD: head/sys/netinet/sctp_dtrace_define.h 235828 2012-05-23 11:26:28Z tuexen $");
#endif

#ifndef _NETINET_SCTP_DTRACE_DEFINE_H_
#define _NETINET_SCTP_DTRACE_DEFINE_H_

#if defined(__FreeBSD__) && __FreeBSD_version >= 803000
#include "opt_kdtrace.h"
#include <sys/kernel.h>
#include <sys/sdt.h>

SDT_PROVIDER_DEFINE(sctp);





SDT_PROBE_DEFINE(sctp, cwnd, net, init, init);

SDT_PROBE_ARGTYPE(sctp, cwnd, net, init, 0, "uint32_t");



SDT_PROBE_ARGTYPE(sctp, cwnd, net, init, 1, "uint32_t");

SDT_PROBE_ARGTYPE(sctp, cwnd, net, init, 2, "uintptr_t");

SDT_PROBE_ARGTYPE(sctp, cwnd, net, init, 3, "int");

SDT_PROBE_ARGTYPE(sctp, cwnd, net, init, 4, "int");



SDT_PROBE_DEFINE(sctp, cwnd, net, ack, ack);

SDT_PROBE_ARGTYPE(sctp, cwnd, net, ack, 0, "uint32_t");



SDT_PROBE_ARGTYPE(sctp, cwnd, net, ack, 1, "uint32_t");

SDT_PROBE_ARGTYPE(sctp, cwnd, net, ack, 2, "uintptr_t");

SDT_PROBE_ARGTYPE(sctp, cwnd, net, ack, 3, "int");

SDT_PROBE_ARGTYPE(sctp, cwnd, net, ack, 4, "int");



SDT_PROBE_DEFINE(sctp, cwnd, net, rttvar, rttvar);

SDT_PROBE_ARGTYPE(sctp, cwnd, net, rttvar, 0, "uint64_t");

SDT_PROBE_ARGTYPE(sctp, cwnd, net, rttvar, 1, "uint64_t");

SDT_PROBE_ARGTYPE(sctp, cwnd, net, rttvar, 2, "uint64_t");

SDT_PROBE_ARGTYPE(sctp, cwnd, net, rttvar, 3, "uint64_t");

SDT_PROBE_ARGTYPE(sctp, cwnd, net, rttvar, 4, "uint64_t");


SDT_PROBE_DEFINE(sctp, cwnd, net, rttstep, rttstep);

SDT_PROBE_ARGTYPE(sctp, cwnd, net, rttstep, 0, "uint64_t");

SDT_PROBE_ARGTYPE(sctp, cwnd, net, rttstep, 1, "uint64_t");

SDT_PROBE_ARGTYPE(sctp, cwnd, net, rttstep, 2, "uint64_t");

SDT_PROBE_ARGTYPE(sctp, cwnd, net, rttstep, 3, "uint64_t");

SDT_PROBE_ARGTYPE(sctp, cwnd, net, rttstep, 4, "uint64_t");



SDT_PROBE_DEFINE(sctp, cwnd, net, fr, fr);

SDT_PROBE_ARGTYPE(sctp, cwnd, net, fr, 0, "uint32_t");



SDT_PROBE_ARGTYPE(sctp, cwnd, net, fr, 1, "uint32_t");

SDT_PROBE_ARGTYPE(sctp, cwnd, net, fr, 2, "uintptr_t");

SDT_PROBE_ARGTYPE(sctp, cwnd, net, fr, 3, "int");

SDT_PROBE_ARGTYPE(sctp, cwnd, net, fr, 4, "int");



SDT_PROBE_DEFINE(sctp, cwnd, net, to, to);

SDT_PROBE_ARGTYPE(sctp, cwnd, net, to, 0, "uint32_t");



SDT_PROBE_ARGTYPE(sctp, cwnd, net, to, 1, "uint32_t");

SDT_PROBE_ARGTYPE(sctp, cwnd, net, to, 2, "uintptr_t");

SDT_PROBE_ARGTYPE(sctp, cwnd, net, to, 3, "int");

SDT_PROBE_ARGTYPE(sctp, cwnd, net, to, 4, "int");



SDT_PROBE_DEFINE(sctp, cwnd, net, bl, bl);

SDT_PROBE_ARGTYPE(sctp, cwnd, net, bl, 0, "uint32_t");



SDT_PROBE_ARGTYPE(sctp, cwnd, net, bl, 1, "uint32_t");

SDT_PROBE_ARGTYPE(sctp, cwnd, net, bl, 2, "uintptr_t");

SDT_PROBE_ARGTYPE(sctp, cwnd, net, bl, 3, "int");

SDT_PROBE_ARGTYPE(sctp, cwnd, net, bl, 4, "int");



SDT_PROBE_DEFINE(sctp, cwnd, net, ecn, ecn);

SDT_PROBE_ARGTYPE(sctp, cwnd, net, ecn, 0, "uint32_t");



SDT_PROBE_ARGTYPE(sctp, cwnd, net, ecn, 1, "uint32_t");

SDT_PROBE_ARGTYPE(sctp, cwnd, net, ecn, 2, "uintptr_t");

SDT_PROBE_ARGTYPE(sctp, cwnd, net, ecn, 3, "int");

SDT_PROBE_ARGTYPE(sctp, cwnd, net, ecn, 4, "int");



SDT_PROBE_DEFINE(sctp, cwnd, net, pd, pd);

SDT_PROBE_ARGTYPE(sctp, cwnd, net, pd, 0, "uint32_t");



SDT_PROBE_ARGTYPE(sctp, cwnd, net, pd, 1, "uint32_t");

SDT_PROBE_ARGTYPE(sctp, cwnd, net, pd, 2, "uintptr_t");

SDT_PROBE_ARGTYPE(sctp, cwnd, net, pd, 3, "int");

SDT_PROBE_ARGTYPE(sctp, cwnd, net, pd, 4, "int");






SDT_PROBE_DEFINE(sctp, rwnd, assoc, val, val);

SDT_PROBE_ARGTYPE(sctp, rwnd, assoc, val, 0, "uint32_t");



SDT_PROBE_ARGTYPE(sctp, rwnd, assoc, val, 1, "uint32_t");

SDT_PROBE_ARGTYPE(sctp, rwnd, assoc, val, 2, "int");

SDT_PROBE_ARGTYPE(sctp, rwnd, assoc, val, 3, "int");




SDT_PROBE_DEFINE(sctp, flightsize, net, val, val);

SDT_PROBE_ARGTYPE(sctp, flightsize, net, val, 0, "uint32_t");



SDT_PROBE_ARGTYPE(sctp, flightsize, net, val, 1, "uint32_t");

SDT_PROBE_ARGTYPE(sctp, flightsize, net, val, 2, "uintptr_t");

SDT_PROBE_ARGTYPE(sctp, flightsize, net, val, 3, "int");

SDT_PROBE_ARGTYPE(sctp, flightsize, net, val, 4, "int");



SDT_PROBE_DEFINE(sctp, flightsize, assoc, val, val);

SDT_PROBE_ARGTYPE(sctp, flightsize, assoc, val, 0, "uint32_t");



SDT_PROBE_ARGTYPE(sctp, flightsize, assoc, val, 1, "uint32_t");

SDT_PROBE_ARGTYPE(sctp, flightsize, assoc, val, 2, "int");

SDT_PROBE_ARGTYPE(sctp, flightsize, assoc, val, 3, "int");
#else 

#ifndef SDT_PROBE
#define SDT_PROBE(a, b, c, d, e, f, g, h, i)
#endif

#endif

#endif
