































#ifdef __FreeBSD__
#include <sys/cdefs.h>
__FBSDID("$FreeBSD: head/sys/netinet/sctp_os.h 235828 2012-05-23 11:26:28Z tuexen $");
#endif

#ifndef _NETINET_SCTP_OS_H_
#define _NETINET_SCTP_OS_H_

























#if defined(__FreeBSD__)
#include <netinet/sctp_os_bsd.h>
#else
#define MODULE_GLOBAL(_B) (_B)
#endif

#if defined(__Userspace__)
#include <netinet/sctp_os_userspace.h>
#endif

#if defined(__APPLE__)
#include <netinet/sctp_os_macosx.h>
#endif

#if defined(__Panda__)
#include <ip/sctp/sctp_os_iox.h>
#endif

#if defined(__Windows__)
#include <netinet/sctp_os_windows.h>
#endif





#define SCTP_DEFAULT_VRF 0
void sctp_init_vrf_list(int vrfid);

#endif
