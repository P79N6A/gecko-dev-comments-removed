


























































#ifndef _USER_IP6_VAR_H_
#define _USER_IP6_VAR_H_

#if defined(__Userspace_os_Windows)
struct ip6_hdr {
	union {
		struct ip6_hdrctl {
			u_int32_t ip6_un1_flow;	
			u_int16_t ip6_un1_plen;	
			u_int8_t  ip6_un1_nxt;	
			u_int8_t  ip6_un1_hlim;	
		} ip6_un1;
		u_int8_t ip6_un2_vfc;	
	} ip6_ctlun;
	struct in6_addr ip6_src;	
	struct in6_addr ip6_dst;	
};
#define ip6_vfc		ip6_ctlun.ip6_un2_vfc
#define ip6_flow	ip6_ctlun.ip6_un1.ip6_un1_flow
#define ip6_plen	ip6_ctlun.ip6_un1.ip6_un1_plen
#define ip6_nxt		ip6_ctlun.ip6_un1.ip6_un1_nxt
#define ip6_hlim	ip6_ctlun.ip6_un1.ip6_un1_hlim
#define ip6_hops	ip6_ctlun.ip6_un1.ip6_un1_hlim

#define IPV6_VERSION		0x60
#endif

#if defined(__Userspace_os_Windows)
#define s6_addr16 u.Word
#endif
#if !defined(__Userspace_os_Windows)
#if !defined(__Userspace_os_Linux)
#define s6_addr8  __u6_addr.__u6_addr8
#define s6_addr16 __u6_addr.__u6_addr16
#define s6_addr32 __u6_addr.__u6_addr32
#endif
#endif

#if !defined(__Userspace_os_FreeBSD) && !defined(__Userspace_os_OpenBSD)
struct route_in6 {
	struct	rtentry *ro_rt;
	struct	llentry *ro_lle;
	struct	in6_addr *ro_ia6;
	int		ro_flags;
	struct	sockaddr_in6 ro_dst;
};
#endif
#define IP6_EXTHDR_GET(val, typ, m, off, len) \
do {									\
	struct mbuf *t;							\
	int tmp;							\
	if ((m)->m_len >= (off) + (len))				\
		(val) = (typ)(mtod((m), caddr_t) + (off));		\
	else {								\
		t = m_pulldown((m), (off), (len), &tmp);		\
		if (t) {						\
			KASSERT(t->m_len >= tmp + (len),		\
			        ("m_pulldown malfunction"));		\
			(val) = (typ)(mtod(t, caddr_t) + tmp);		\
		} else {						\
			(val) = (typ)NULL;				\
			(m) = NULL;					\
		}							\
	}								\
} while (0)

#endif 
