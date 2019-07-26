
































#ifndef __sctp_os_userspace_h__
#define __sctp_os_userspace_h__






#include <errno.h>

#if defined(__Userspace_os_Windows)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ws2ipdef.h>
#include <ws2def.h>
#include <iphlpapi.h>
#include <Mswsock.h>
#include <Windows.h>
#include "user_environment.h"
typedef CRITICAL_SECTION userland_mutex_t;
#if WINVER < 0x0600
enum {
	C_SIGNAL = 0,
	C_BROADCAST = 1,
	C_MAX_EVENTS = 2
};
typedef struct
{
	u_int waiters_count;
	CRITICAL_SECTION waiters_count_lock;
	HANDLE events_[C_MAX_EVENTS];
} userland_cond_t;
#define InitializeConditionVariable(cond) InitializeXPConditionVariable(cond)
#define DeleteConditionVariable(cond) DeleteXPConditionVariable(cond)
#define SleepConditionVariableCS(cond, mtx, time) SleepXPConditionVariable(cond, mtx)
#define WakeAllConditionVariable(cond) WakeAllXPConditionVariable(cond)
#else
#define DeleteConditionVariable(cond)
typedef CONDITION_VARIABLE userland_cond_t;
#endif
typedef HANDLE userland_thread_t;
#define ADDRESS_FAMILY	unsigned __int8
#define IPVERSION  4
#define MAXTTL     255
#define uint64_t   unsigned __int64
#define u_long     unsigned __int64
#define u_int      unsigned __int32
#define uint32_t   unsigned __int32
#define u_int32_t  unsigned __int32
#define int32_t	   __int32
#define int16_t	   __int16
#define uint16_t   unsigned __int16
#define u_int16_t  unsigned __int16
#define uint8_t    unsigned __int8
#define u_int8_t   unsigned __int8
#define int8_t     __int8
#define u_char     unsigned char
#define n_short    unsigned __int16
#define u_short    unsigned __int16
#define ssize_t    __int64
#define size_t     __int32
#define in_addr_t  unsigned __int32
#define in_port_t  unsigned __int16
#define n_time     unsigned __int32
#define sa_family_t unsigned __int8
#define IFNAMSIZ   64
#define __func__	__FUNCTION__

#ifndef EWOULDBLOCK
#define EWOULDBLOCK             WSAEWOULDBLOCK
#endif
#ifndef EINPROGRESS
#define EINPROGRESS             WSAEINPROGRESS
#endif
#ifndef EALREADY
#define EALREADY                WSAEALREADY
#endif
#ifndef ENOTSOCK
#define ENOTSOCK                WSAENOTSOCK
#endif
#ifndef EDESTADDRREQ
#define EDESTADDRREQ            WSAEDESTADDRREQ
#endif
#ifndef EMSGSIZE
#define EMSGSIZE                WSAEMSGSIZE
#endif
#ifndef EPROTOTYPE
#define EPROTOTYPE              WSAEPROTOTYPE
#endif
#ifndef ENOPROTOOPT
#define ENOPROTOOPT             WSAENOPROTOOPT
#endif
#ifndef EPROTONOSUPPORT
#define EPROTONOSUPPORT         WSAEPROTONOSUPPORT
#endif
#ifndef ESOCKTNOSUPPORT
#define ESOCKTNOSUPPORT         WSAESOCKTNOSUPPORT
#endif
#ifndef EOPNOTSUPP
#define EOPNOTSUPP              WSAEOPNOTSUPP
#endif
#ifndef ENOTSUP
#define ENOTSUP                 WSAEOPNOTSUPP
#endif
#ifndef EPFNOSUPPORT
#define EPFNOSUPPORT            WSAEPFNOSUPPORT
#endif
#ifndef EAFNOSUPPORT
#define EAFNOSUPPORT            WSAEAFNOSUPPORT
#endif
#ifndef EADDRINUSE
#define EADDRINUSE              WSAEADDRINUSE
#endif
#ifndef EADDRNOTAVAIL
#define EADDRNOTAVAIL           WSAEADDRNOTAVAIL
#endif
#ifndef ENETDOWN
#define ENETDOWN                WSAENETDOWN
#endif
#ifndef ENETUNREACH
#define ENETUNREACH             WSAENETUNREACH
#endif
#ifndef ENETRESET
#define ENETRESET               WSAENETRESET
#endif
#ifndef ECONNABORTED
#define ECONNABORTED            WSAECONNABORTED
#endif
#ifndef ECONNRESET
#define ECONNRESET              WSAECONNRESET
#endif
#ifndef ENOBUFS
#define ENOBUFS                 WSAENOBUFS
#endif
#ifndef EISCONN
#define EISCONN                 WSAEISCONN
#endif
#ifndef ENOTCONN
#define ENOTCONN                WSAENOTCONN
#endif
#ifndef ESHUTDOWN
#define ESHUTDOWN               WSAESHUTDOWN
#endif
#ifndef ETOOMANYREFS
#define ETOOMANYREFS            WSAETOOMANYREFS
#endif
#ifndef ETIMEDOUT
#define ETIMEDOUT               WSAETIMEDOUT
#endif
#ifndef ECONNREFUSED
#define ECONNREFUSED            WSAECONNREFUSED
#endif
#ifndef ELOOP
#define ELOOP                   WSAELOOP
#endif
#ifndef EHOSTDOWN
#define EHOSTDOWN               WSAEHOSTDOWN
#endif
#ifndef EHOSTUNREACH
#define EHOSTUNREACH            WSAEHOSTUNREACH
#endif
#ifndef EPROCLIM
#define EPROCLIM                WSAEPROCLIM
#endif
#ifndef EUSERS
#define EUSERS                  WSAEUSERS
#endif
#ifndef EDQUOT
#define EDQUOT                  WSAEDQUOT
#endif
#ifndef ESTALE
#define ESTALE                  WSAESTALE
#endif
#ifndef EREMOTE
#define EREMOTE                 WSAEREMOTE
#endif

typedef char* caddr_t;

#define getifaddrs(interfaces)  (int)Win_getifaddrs(interfaces)
#define if_nametoindex(x) (int)win_if_nametoindex(x)

#define bzero(buf, len) memset(buf, 0, len)
#define bcopy(srcKey, dstKey, len) memcpy(dstKey, srcKey, len)
#define snprintf(data, size, format, name) _snprintf_s(data, size, _TRUNCATE, format, name)
#define inline __inline
#define __inline__ __inline
#define random() rand()
#define srandom(s) srand(s)
#define	MSG_EOR		0x8		/* data completes record */
#define	MSG_DONTWAIT	0x80		/* this message should be nonblocking */

#ifdef CMSG_DATA
#undef CMSG_DATA
#endif
#define CMSG_DATA(x) WSA_CMSG_DATA(x)
#define CMSG_ALIGN(x) WSA_CMSGDATA_ALIGN(x)
#if WINVER < 0x0600
#define CMSG_FIRSTHDR(x) WSA_CMSG_FIRSTHDR(x)
#define CMSG_NXTHDR(x, y) WSA_CMSG_NXTHDR(x, y)
#define CMSG_SPACE(x) WSA_CMSG_SPACE(x)
#define CMSG_LEN(x) WSA_CMSG_LEN(x)
#endif


#define SCTP_IFN_IS_IFT_LOOP(ifn)	((ifn)->ifn_type == IFT_LOOP)
#define SCTP_ROUTE_IS_REAL_LOOP(ro) ((ro)->ro_rt && (ro)->ro_rt->rt_ifa && (ro)->ro_rt->rt_ifa->ifa_ifp && (ro)->ro_rt->rt_ifa->ifa_ifp->if_type == IFT_LOOP)





#define SCTP_GET_IFN_VOID_FROM_ROUTE(ro) \
	((ro)->ro_rt != NULL ? (ro)->ro_rt->rt_ifp : NULL)
#define SCTP_ROUTE_HAS_VALID_IFN(ro) \
	((ro)->ro_rt && (ro)->ro_rt->rt_ifp)


#define SCTP_GET_IF_INDEX_FROM_ROUTE(ro) 1 /* compiles...  TODO use routing socket to determine */

#define timeradd(tvp, uvp, vvp)   \
	do {                          \
	    (vvp)->tv_sec = (tvp)->tv_sec + (uvp)->tv_sec;  \
		(vvp)->tv_usec = (tvp)->tv_usec + (uvp)->tv_usec;  \
		if ((vvp)->tv_usec >= 1000000) {                   \
		    (vvp)->tv_sec++;                        \
			(vvp)->tv_usec -= 1000000;             \
		}                         \
	} while (0)

#define timersub(tvp, uvp, vvp)   \
	do {                          \
	    (vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec;  \
		(vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec;  \
		if ((vvp)->tv_usec < 0) {                   \
		    (vvp)->tv_sec--;                        \
			(vvp)->tv_usec += 1000000;             \
		}                       \
	} while (0)

#define BIG_ENDIAN 1
#define LITTLE_ENDIAN 0
#ifdef WORDS_BIGENDIAN
#define BYTE_ORDER BIG_ENDIAN
#else
#define BYTE_ORDER LITTLE_ENDIAN
#endif

struct iovec {
	ULONG len;
	CHAR FAR *buf;
};

#define iov_base buf
#define iov_len	len

struct ifa_msghdr {
	unsigned __int16 ifam_msglen;
	unsigned char    ifam_version;
	unsigned char    ifam_type;
	__int32          ifam_addrs;
	__int32          ifam_flags;
	unsigned __int16 ifam_index;
	__int32          ifam_metric;
};

struct ifdevmtu {
	int ifdm_current;
	int ifdm_min;
	int ifdm_max;
};

struct ifkpi {
	unsigned int  ifk_module_id;
	unsigned int  ifk_type;
	union {
		void *ifk_ptr;
		int ifk_value;
	} ifk_data;
};

struct ifreq {
	char    ifr_name[16];
	union {
		struct sockaddr ifru_addr;
		struct sockaddr ifru_dstaddr;
		struct sockaddr ifru_broadaddr;
		short  ifru_flags;
		int ifru_metric;
		int ifru_mtu;
		int ifru_phys;
		int ifru_media;
		int    ifru_intval;
		char*  ifru_data;
		struct ifdevmtu ifru_devmtu;
		struct ifkpi  ifru_kpi;
		unsigned __int32 ifru_wake_flags;
	} ifr_ifru;
#define ifr_addr        ifr_ifru.ifru_addr
#define ifr_dstaddr     ifr_ifru.ifru_dstaddr
#define ifr_broadaddr   ifr_ifru.ifru_broadaddr
#define ifr_flags       ifr_ifru.ifru_flags[0]
#define ifr_prevflags   ifr_ifru.ifru_flags[1]
#define ifr_metric      ifr_ifru.ifru_metric
#define ifr_mtu         ifr_ifru.ifru_mtu
#define ifr_phys        ifr_ifru.ifru_phys
#define ifr_media       ifr_ifru.ifru_media
#define ifr_data        ifr_ifru.ifru_data
#define ifr_devmtu      ifr_ifru.ifru_devmtu
#define ifr_intval      ifr_ifru.ifru_intval
#define ifr_kpi         ifr_ifru.ifru_kpi
#define ifr_wake_flags  ifr_ifru.ifru_wake_flags
};



struct ip {
	u_char    ip_hl:4, ip_v:4;
	u_char    ip_tos;
	u_short   ip_len;
	u_short   ip_id;
	u_short   ip_off;
#define IP_RP 0x8000
#define IP_DF 0x4000
#define IP_MF 0x2000
#define IP_OFFMASK 0x1fff
	u_char    ip_ttl;
	u_char    ip_p;
	u_short   ip_sum;
    struct in_addr ip_src, ip_dst;
};

struct ifaddrs {
	struct ifaddrs  *ifa_next;
	char		*ifa_name;
	unsigned int		 ifa_flags;
	struct sockaddr	*ifa_addr;
	struct sockaddr	*ifa_netmask;
	struct sockaddr	*ifa_dstaddr;
	void		*ifa_data;
};

struct udphdr {
	unsigned __int16 uh_sport;
	unsigned __int16 uh_dport;
	unsigned __int16 uh_ulen;
	unsigned __int16 uh_sum;
};

#else 
#include <sys/cdefs.h> 
#include <sys/socket.h>
#if defined(__Userspace_os_FreeBSD) || defined(__Userspace_os_OpenBSD)
#include <pthread.h>
#endif
typedef pthread_mutex_t userland_mutex_t;
typedef pthread_cond_t userland_cond_t;
typedef pthread_t userland_thread_t;
#endif

#define mtx_lock(arg1)
#define mtx_unlock(arg1)
#define mtx_assert(arg1,arg2)
#define MA_OWNED 7 /* sys/mutex.h typically on FreeBSD */
#if !defined(__Userspace_os_FreeBSD)
struct mtx {int dummy;};
struct selinfo {int dummy;};
struct sx {int dummy;};
#endif

#include <stdio.h>
#include <string.h>



#if defined(__Userspace_os_Windows)
#include <user_queue.h>
#else
#include <sys/queue.h>
#endif
#include <user_malloc.h>





#include "user_socketvar.h" 


#include <user_environment.h>
#include <user_atomic.h>
#include <user_mbuf.h>


#if defined(__FreeBSD__) && __FreeBSD_version > 602000
#include <sys/rwlock.h>
#endif

#if defined(__FreeBSD__) && __FreeBSD_version > 602000
#include <sys/priv.h>
#endif




#if defined(__Userspace_os_Darwin)

#include <net/if_var.h>
#endif
#if defined(__Userspace_os_FreeBSD)
#include <net/if_types.h>

#endif




#if !defined(__Userspace_os_Windows)
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#else
#include <user_ip_icmp.h>
#endif

#include <user_inpcb.h>


#include <sys/types.h>
#if !defined(__Userspace_os_Windows)
#include <ifaddrs.h>


#include <sys/ioctl.h>


#include <unistd.h>
#endif




#include <stddef.h>

#if defined(SCTP_PROCESS_LEVEL_LOCKS) && !defined(__Userspace_os_Windows)

#include <pthread.h>
#endif

#ifdef IPSEC
#include <netipsec/ipsec.h>
#include <netipsec/key.h>
#endif				

#ifdef INET6
#if defined(__Userspace_os_FreeBSD)
#include <sys/domain.h>
#endif
#ifdef IPSEC
#include <netipsec/ipsec6.h>
#endif
#if !defined(__Userspace_os_Windows)
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#endif
#if defined(__Userspace_os_Linux) || defined(__Userspace_os_Darwin) || defined(__Userspace_os_FreeBSD) || defined(__Userspace_os_OpenBSD) ||defined(__Userspace_os_Windows)
#include "user_ip6_var.h"
#else
#include <netinet6/ip6_var.h>
#endif
#if defined(__Userspace_os_FreeBSD)
#include <netinet6/in6_pcb.h>
#include <netinet6/ip6protosw.h>

#include <netinet6/scope6_var.h>
#endif
#endif 

#if defined(HAVE_SCTP_PEELOFF_SOCKOPT)
#include <sys/file.h>
#include <sys/filedesc.h>
#endif

#if __FreeBSD_version >= 700000
#include <netinet/ip_options.h>
#endif

#define SCTP_PRINTF(...)                                  \
	if (SCTP_BASE_VAR(debug_printf)) {                \
		SCTP_BASE_VAR(debug_printf)(__VA_ARGS__); \
	}

#if defined(__FreeBSD__)
#ifndef in6pcb
#define in6pcb		inpcb
#endif
#endif

MALLOC_DECLARE(SCTP_M_MAP);
MALLOC_DECLARE(SCTP_M_STRMI);
MALLOC_DECLARE(SCTP_M_STRMO);
MALLOC_DECLARE(SCTP_M_ASC_ADDR);
MALLOC_DECLARE(SCTP_M_ASC_IT);
MALLOC_DECLARE(SCTP_M_AUTH_CL);
MALLOC_DECLARE(SCTP_M_AUTH_KY);
MALLOC_DECLARE(SCTP_M_AUTH_HL);
MALLOC_DECLARE(SCTP_M_AUTH_IF);
MALLOC_DECLARE(SCTP_M_STRESET);
MALLOC_DECLARE(SCTP_M_CMSG);
MALLOC_DECLARE(SCTP_M_COPYAL);
MALLOC_DECLARE(SCTP_M_VRF);
MALLOC_DECLARE(SCTP_M_IFA);
MALLOC_DECLARE(SCTP_M_IFN);
MALLOC_DECLARE(SCTP_M_TIMW);
MALLOC_DECLARE(SCTP_M_MVRF);
MALLOC_DECLARE(SCTP_M_ITER);
MALLOC_DECLARE(SCTP_M_SOCKOPT);

#if defined(SCTP_LOCAL_TRACE_BUF)

#define SCTP_GET_CYCLECOUNT get_cyclecount()
#define SCTP_CTR6 sctp_log_trace

#else
#define SCTP_CTR6 CTR6
#endif


#define	CTR6(m, d, p1, p2, p3, p4, p5, p6)



#define SCTP_BASE_INFO(__m) system_base_info.sctppcbinfo.__m
#define SCTP_BASE_STATS system_base_info.sctpstat
#define SCTP_BASE_STAT(__m)     system_base_info.sctpstat.__m
#define SCTP_BASE_SYSCTL(__m) system_base_info.sctpsysctl.__m
#define SCTP_BASE_VAR(__m) system_base_info.__m




#if !defined(__Userspace_os_Darwin)
#define USER_ADDR_NULL	(NULL)		/* FIX ME: temp */
#endif

#if defined(SCTP_DEBUG)
#include <netinet/sctp_constants.h>
#define SCTPDBG(level, ...)  \
{                              \
    do {    \
	if (SCTP_BASE_SYSCTL(sctp_debug_on) & level) {  \
	    SCTP_PRINTF(__VA_ARGS__);           \
	}        \
	} while (0);     \
}
#define SCTPDBG_ADDR(level, addr)					\
{									\
    do {								\
	if (SCTP_BASE_SYSCTL(sctp_debug_on) & level ) {					\
	    sctp_print_address(addr);					\
	}								\
    } while (0);							\
}
#else
#define SCTPDBG(level, ...)
#define SCTPDBG_ADDR(level, addr)
#endif

#ifdef SCTP_LTRACE_CHUNKS
#define SCTP_LTRACE_CHK(a, b, c, d) if(sctp_logging_level & SCTP_LTRACE_CHUNK_ENABLE) CTR6(KTR_SUBSYS, "SCTP:%d[%d]:%x-%x-%x-%x", SCTP_LOG_CHUNK_PROC, 0, a, b, c, d)
#else
#define SCTP_LTRACE_CHK(a, b, c, d)
#endif

#ifdef SCTP_LTRACE_ERRORS
#define SCTP_LTRACE_ERR_RET_PKT(m, inp, stcb, net, file, err) \
	if (sctp_logging_level & SCTP_LTRACE_ERROR_ENABLE) \
		SCTP_PRINTF("mbuf:%p inp:%p stcb:%p net:%p file:%x line:%d error:%d\n", \
		            (void *)m, (void *)inp, (void *)stcb, (void *)net, file, __LINE__, err);
#define SCTP_LTRACE_ERR_RET(inp, stcb, net, file, err) \
	if (sctp_logging_level & SCTP_LTRACE_ERROR_ENABLE) \
		SCTP_PRINTF("inp:%p stcb:%p net:%p file:%x line:%d error:%d\n", \
		            (void *)inp, (void *)stcb, (void *)net, file, __LINE__, err);
#else
#define SCTP_LTRACE_ERR_RET_PKT(m, inp, stcb, net, file, err)
#define SCTP_LTRACE_ERR_RET(inp, stcb, net, file, err)
#endif





#define SCTP_MAX_VRF_ID		0
#define SCTP_SIZE_OF_VRF_HASH	3
#define SCTP_IFNAMSIZ		IFNAMSIZ
#define SCTP_DEFAULT_VRFID	0
#define SCTP_VRF_ADDR_HASH_SIZE	16
#define SCTP_VRF_IFN_HASH_SIZE	3
#define	SCTP_INIT_VRF_TABLEID(vrf)

#if !defined(__Userspace_os_Windows)
#define SCTP_IFN_IS_IFT_LOOP(ifn) (strncmp((ifn)->ifn_name, "lo", 2) == 0)



#define SCTP_ROUTE_IS_REAL_LOOP(ro) 0





#define SCTP_GET_IFN_VOID_FROM_ROUTE(ro) (void *)ro->ro_rt->rt_ifp
#define SCTP_GET_IF_INDEX_FROM_ROUTE(ro) 1 /* compiles...  TODO use routing socket to determine */
#define SCTP_ROUTE_HAS_VALID_IFN(ro) ((ro)->ro_rt && (ro)->ro_rt->rt_ifp)
#endif




#define SCTP_MALLOC(var, type, size, name) \
    do { \
	MALLOC(var, type, size, name, M_NOWAIT); \
    } while (0)

#define SCTP_FREE(var, type)	FREE(var, type)

#define SCTP_MALLOC_SONAME(var, type, size) \
    do { \
	MALLOC(var, type, size, M_SONAME, (M_WAITOK | M_ZERO)); \
    } while (0)

#define SCTP_FREE_SONAME(var)	FREE(var, M_SONAME)

#define SCTP_PROCESS_STRUCT struct proc *






#if defined(SCTP_SIMPLE_ALLOCATOR)

#define SCTP_ZONE_INIT(zone, name, size, number) { \
	zone = size; \
}


#define SCTP_ZONE_GET(zone, type)  \
        (type *)malloc(zone);



#define SCTP_ZONE_FREE(zone, element) { \
	free(element);  \
}

#define SCTP_ZONE_DESTROY(zone)
#else







#include "user_include/umem.h"









#define SCTP_ZONE_INIT(zone, name, size, number) { \
	zone = umem_cache_create(name, size, 0, NULL, NULL, NULL, NULL, NULL, 0); \
  }


#define SCTP_ZONE_GET(zone, type) \
        (type *)umem_cache_alloc(zone, UMEM_DEFAULT);



#define SCTP_ZONE_FREE(zone, element) \
	umem_cache_free(zone, element);



#define SCTP_ZONE_DESTROY(zone) \
	umem_cache_destroy(zone);
#endif






extern struct ifaddrs *g_interfaces;





void *sctp_hashinit_flags(int elements, struct malloc_type *type,
                    u_long *hashmask, int flags);
void
sctp_hashdestroy(void *vhashtbl, struct malloc_type *type, u_long hashmask);

void
sctp_hashfreedestroy(void *vhashtbl, struct malloc_type *type, u_long hashmask);


#define HASH_NOWAIT 0x00000001
#define HASH_WAITOK 0x00000002


#define SCTP_HASH_INIT(size, hashmark) sctp_hashinit_flags(size, M_PCB, hashmark, HASH_NOWAIT)

#define SCTP_HASH_FREE(table, hashmark) sctp_hashdestroy(table, M_PCB, hashmark)

#define SCTP_HASH_FREE_DESTROY(table, hashmark)  sctp_hashfreedestroy(table, M_PCB, hashmark)
#define SCTP_M_COPYM	m_copym









#include <netinet/sctp_callout.h>


#include <user_recv_thread.h>


#define KTR_SUBSYS 1

#define sctp_get_tick_count() (ticks)


#if !defined(__Userspace_os_Windows)
#define SCTP_PACKED __attribute__((packed))
#define SCTP_UNUSED __attribute__((unused))
#else
#define SCTP_PACKED
#define SCTP_UNUSED
#endif





#define SCTP_BUF_LEN(m) (m->m_len)
#define SCTP_BUF_NEXT(m) (m->m_next)
#define SCTP_BUF_NEXT_PKT(m) (m->m_nextpkt)
#define SCTP_BUF_RESV_UF(m, size) m->m_data += size
#define SCTP_BUF_AT(m, size) m->m_data + size
#define SCTP_BUF_IS_EXTENDED(m) (m->m_flags & M_EXT)
#define SCTP_BUF_EXTEND_SIZE(m) (m->m_ext.ext_size)
#define SCTP_BUF_TYPE(m) (m->m_type)
#define SCTP_BUF_RECVIF(m) (m->m_pkthdr.rcvif)
#define SCTP_BUF_PREPEND	M_PREPEND

#define SCTP_ALIGN_TO_END(m, len) if(m->m_flags & M_PKTHDR) { \
                                     MH_ALIGN(m, len); \
                                  } else if ((m->m_flags & M_EXT) == 0) { \
                                     M_ALIGN(m, len); \
                                  }







#define SCTP_PKTLOG_WRITERS_NEED_LOCK 3






typedef struct sctp_route	sctp_route_t;
typedef struct sctp_rtentry	sctp_rtentry_t;

static inline void sctp_userspace_rtalloc(sctp_route_t *ro)
{
	if (ro->ro_rt != NULL) {
		ro->ro_rt->rt_refcnt++;
		return;
	}

	ro->ro_rt = (sctp_rtentry_t *) malloc(sizeof(sctp_rtentry_t));
	if (ro->ro_rt == NULL)
		return;

	
	memset(ro->ro_rt, 0, sizeof(sctp_rtentry_t));
	ro->ro_rt->rt_refcnt = 1;

	
	
#if 0
	if (userspace_rawroute == -1) {
		userspace_rawroute = socket(AF_ROUTE, SOCK_RAW, 0);
		if (userspace_rawroute == -1)
			return;
	}
#endif
	ro->ro_rt->rt_rmx.rmx_mtu = 1500; 

	


}
#define SCTP_RTALLOC(ro, vrf_id) sctp_userspace_rtalloc((sctp_route_t *)ro)


static inline void sctp_userspace_rtfree(sctp_rtentry_t *rt)
{
	if(rt == NULL) {
		return;
	}
	if(--rt->rt_refcnt > 0) {
		return;
	}
	free(rt);
	rt = NULL;
}
#define rtfree(arg1) sctp_userspace_rtfree(arg1)





int sctp_userspace_get_mtu_from_ifn(uint32_t if_index, int af);

#define SCTP_GATHER_MTU_FROM_IFN_INFO(ifn, ifn_index, af) sctp_userspace_get_mtu_from_ifn(ifn_index, af)

#define SCTP_GATHER_MTU_FROM_ROUTE(sctp_ifa, sa, rt) ((rt != NULL) ? rt->rt_rmx.rmx_mtu : 0)

#define SCTP_GATHER_MTU_FROM_INTFC(sctp_ifn)  sctp_userspace_get_mtu_from_ifn(if_nametoindex(((struct ifaddrs *) (sctp_ifn))->ifa_name), AF_INET)

#define SCTP_SET_MTU_OF_ROUTE(sa, rt, mtu) do { \
                                              if (rt != NULL) \
                                                 rt->rt_rmx.rmx_mtu = mtu; \
                                           } while(0)


#define SCTP_REGISTER_INTERFACE(ifhandle, af)
#define SCTP_DEREGISTER_INTERFACE(ifhandle, af)






#define SCTP_BUF_EXTEND_BASE(m) (m->m_ext.ext_buf)
 
#define SCTP_BUF_EXTEND_REFCNT(m) (*m->m_ext.ref_cnt)



#define SCTP_BUF_GET_FLAGS(m) (m->m_flags)






#define SCTP_HEADER_TO_CHAIN(m) (m)
#define SCTP_DETACH_HEADER_FROM_CHAIN(m)
#define SCTP_HEADER_LEN(m) ((m)->m_pkthdr.len)
#define SCTP_GET_HEADER_FOR_OUTPUT(o_pak) 0
#define SCTP_RELEASE_HEADER(m)
#define SCTP_RELEASE_PKT(m)	sctp_m_freem(m)

#define SCTP_ENABLE_UDP_CSUM(m) m=m






#define SCTP_GET_PKT_VRFID(m, vrf_id)  ((vrf_id = SCTP_DEFAULT_VRFID) != SCTP_DEFAULT_VRFID)




#define SCTP_ATTACH_CHAIN(pak, m, packet_length) do { \
                                                  pak = m; \
                                                  pak->m_pkthdr.len = packet_length; \
                          } while(0)



#define SCTP_IS_IT_BROADCAST(dst, m) 0

#define SCTP_IS_IT_LOOPBACK(m) 0









#define SCTP_GET_HLIM(inp, ro) 128 /* As done for __Windows__ */
#define IPv6_HOP_LIMIT 128


#define SCTP_IPV6_V6ONLY(inp)	(((struct inpcb *)inp)->inp_flags & IN6P_IPV6_V6ONLY)

#define SCTP_SO_IS_NBIO(so)	((so)->so_state & SS_NBIO)
#define SCTP_SET_SO_NBIO(so)	((so)->so_state |= SS_NBIO)
#define SCTP_CLEAR_SO_NBIO(so)	((so)->so_state &= ~SS_NBIO)

#define SCTP_SO_TYPE(so)	((so)->so_type)


#define SCTP_SORESERVE(so, send, recv)	soreserve(so, send, recv)


#define SCTP_SOWAKEUP(so)	wakeup(&(so)->so_timeo, so)

#define SCTP_SB_CLEAR(sb)	\
	(sb).sb_cc = 0;		\
	(sb).sb_mb = NULL;	\
	(sb).sb_mbcnt = 0;

#define SCTP_SB_LIMIT_RCV(so) so->so_rcv.sb_hiwat
#define SCTP_SB_LIMIT_SND(so) so->so_snd.sb_hiwat


#define SCTP_ZERO_COPY_EVENT(inp, so)

#define SCTP_ZERO_COPY_SENDQ_EVENT(inp, so)














#define SCTP_READ_RANDOM(buf, len)	read_random(buf, len)


#ifdef USE_SCTP_SHA1
#include <netinet/sctp_sha1.h>
#else
#if 0 
#include <crypto/sha1.h>

#define SHA1_Init	SHA1Init
#define SHA1_Update	SHA1Update
#define SHA1_Final(x,y)	SHA1Final((caddr_t)x, y)
#endif
#endif

#if defined(HAVE_SHA2)
#include <crypto/sha2/sha2.h>
#endif
#if 0

#if 1 
#include <openssl/md5.h>
#include <openssl/sha.h>



typedef SHA_CTX SHA1_CTX;

#else 

#include <sys/md5.h>

#define MD5_Init	MD5Init
#define MD5_Update	MD5Update
#define MD5_Final	MD5Final
#endif
#endif






#define IFT_LOOP 0x18




#ifdef HAVE_SHA2
typedef int SHA256_CTX;
typedef int SHA384_CTX;
typedef int SHA512_CTX;
#endif

#if defined(__Userspace_os_Windows)
#define SHUT_RD 1
#define SHUT_WR 2
#define SHUT_RDWR 3
#endif
#define PRU_FLUSH_RD SHUT_RD
#define PRU_FLUSH_WR SHUT_WR
#define PRU_FLUSH_RDWR SHUT_RDWR


#define	IP_RAWOUTPUT		0x2




#define AF_CONN 123
struct sockaddr_conn {
#ifdef HAVE_SCONN_LEN
	uint8_t sconn_len;
#endif
	uint8_t sconn_family;
	uint16_t sconn_port;
	void *sconn_addr;
};










#define SCTP_IP_ID(inp) (ip_id)


#include <netinet/sctp.h>
extern void sctp_userspace_ip_output(int *result, struct mbuf *o_pak,
                                     sctp_route_t *ro, void *stcb,
                                     uint32_t vrf_id);

#define SCTP_IP_OUTPUT(result, o_pak, ro, stcb, vrf_id) sctp_userspace_ip_output(&result, o_pak, ro, stcb, vrf_id);

#if defined(INET6)
extern void sctp_userspace_ip6_output(int *result, struct mbuf *o_pak,
                                      struct route_in6 *ro, void *stcb,
                                      uint32_t vrf_id);
#define SCTP_IP6_OUTPUT(result, o_pak, ro, ifp, stcb, vrf_id) sctp_userspace_ip6_output(&result, o_pak, ro, stcb, vrf_id);
#endif



#if 0
#define SCTP_IP6_OUTPUT(result, o_pak, ro, ifp, stcb, vrf_id) \
{ \
	if (stcb && stcb->sctp_ep) \
		result = ip6_output(o_pak, \
				    ((struct in6pcb *)(stcb->sctp_ep))->in6p_outputopts, \
				    (ro), 0, 0, ifp, NULL); \
	else \
		result = ip6_output(o_pak, NULL, (ro), 0, 0, ifp, NULL); \
}
#endif

struct mbuf *
sctp_get_mbuf_for_msg(unsigned int space_needed, int want_header, int how, int allonebuf, int type);





#if defined(__Userspace_os_FreeBSD) || defined(__Userspace_os_OpenBSD)

#define CMSG_ALIGN(n)   _ALIGN(n)
#elif defined(__Userspace_os_Darwin)
#if !defined(__DARWIN_ALIGNBYTES)
#define	__DARWIN_ALIGNBYTES	(sizeof(__darwin_size_t) - 1)
#endif

#if !defined(__DARWIN_ALIGN)
#define	__DARWIN_ALIGN(p)	((__darwin_size_t)((char *)(uintptr_t)(p) + __DARWIN_ALIGNBYTES) &~ __DARWIN_ALIGNBYTES)
#endif

#if !defined(__DARWIN_ALIGNBYTES32)
#define __DARWIN_ALIGNBYTES32     (sizeof(__uint32_t) - 1)
#endif

#if !defined(__DARWIN_ALIGN32)
#define __DARWIN_ALIGN32(p)       ((__darwin_size_t)((char *)(uintptr_t)(p) + __DARWIN_ALIGNBYTES32) &~ __DARWIN_ALIGNBYTES32)
#endif
#define CMSG_ALIGN(n)   __DARWIN_ALIGN32(n)
#endif
#define I_AM_HERE \
                do { \
			SCTP_PRINTF("%s:%d at %s\n", __FILE__, __LINE__ , __FUNCTION__); \
		} while (0)

#ifndef timevalsub
#define timevalsub(tp1, tp2)                       \
	do {                                       \
		(tp1)->tv_sec -= (tp2)->tv_sec;    \
		(tp1)->tv_usec -= (tp2)->tv_usec;  \
		if ((tp1)->tv_usec < 0) {          \
			(tp1)->tv_sec--;           \
			(tp1)->tv_usec += 1000000; \
		}                                  \
	} while (0)
#endif

#if defined(__Userspace_os_Linux)
#define TAILQ_FOREACH_SAFE(var, head, field, tvar)             \
         for ((var) = ((head)->tqh_first);                     \
              (var) && ((tvar) = TAILQ_NEXT((var), field), 1); \
              (var) = (tvar))

#define LIST_FOREACH_SAFE(var, head, field, tvar)              \
         for ((var) = ((head)->lh_first);                      \
              (var) && ((tvar) = LIST_NEXT((var), field), 1);  \
              (var) = (tvar))
#endif
#endif
