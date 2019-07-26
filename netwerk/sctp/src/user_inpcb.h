































#ifndef _USER_INPCB_H_
#define _USER_INPCB_H_

#include <user_route.h> 

#define	in6pcb		inpcb	/* for KAME src sync over BSD*'s */
#define	in6p_sp		inp_sp	/* for KAME src sync over BSD*'s */
struct inpcbpolicy;









LIST_HEAD(inpcbhead, inpcb);
LIST_HEAD(inpcbporthead, inpcbport);
typedef	u_quad_t	inp_gen_t;






struct in_addr_4in6 {
	u_int32_t	ia46_pad32[3];
	struct	in_addr	ia46_addr4;
};





struct in_endpoints {
	u_int16_t	ie_fport;		
	u_int16_t	ie_lport;		
	
	union {
		
		struct	in_addr_4in6 ie46_foreign;
		struct	in6_addr ie6_foreign;
	} ie_dependfaddr;
	union {
		
		struct	in_addr_4in6 ie46_local;
		struct	in6_addr ie6_local;
	} ie_dependladdr;
#define	ie_faddr	ie_dependfaddr.ie46_foreign.ia46_addr4
#define	ie_laddr	ie_dependladdr.ie46_local.ia46_addr4
#define	ie6_faddr	ie_dependfaddr.ie6_foreign
#define	ie6_laddr	ie_dependladdr.ie6_local
};





struct in_conninfo {
	u_int8_t	inc_flags;
	u_int8_t	inc_len;
	u_int16_t	inc_pad;	
	
	struct	in_endpoints inc_ie;
};
#define inc_isipv6	inc_flags	/* temp compatability */
#define	inc_fport	inc_ie.ie_fport
#define	inc_lport	inc_ie.ie_lport
#define	inc_faddr	inc_ie.ie_faddr
#define	inc_laddr	inc_ie.ie_laddr
#define	inc6_faddr	inc_ie.ie6_faddr
#define	inc6_laddr	inc_ie.ie6_laddr

struct	icmp6_filter;

struct inpcb {
	LIST_ENTRY(inpcb) inp_hash;	
	LIST_ENTRY(inpcb) inp_list;	
	void	*inp_ppcb;		
	struct	inpcbinfo *inp_pcbinfo;	
	struct	socket *inp_socket;	

	u_int32_t	inp_flow;
	int	inp_flags;		

	u_char	inp_vflag;		
#define	INP_IPV4	0x1
#define	INP_IPV6	0x2
#define	INP_IPV6PROTO	0x4		/* opened under IPv6 protocol */
#define	INP_TIMEWAIT	0x8		/* .. probably doesn't go here */
#define	INP_ONESBCAST	0x10		/* send all-ones broadcast */
#define	INP_DROPPED	0x20		/* protocol drop flag */
#define	INP_SOCKREF	0x40		/* strong socket reference */
#define INP_CONN        0x80
	u_char	inp_ip_ttl;		
	u_char	inp_ip_p;		
	u_char	inp_ip_minttl;		
	uint32_t inp_ispare1;		
	void	*inp_pspare[2];		

	
	struct	in_conninfo inp_inc;

					
	struct	label *inp_label;	
	struct	inpcbpolicy *inp_sp;    

	
	struct {
		u_char	inp4_ip_tos;		
		struct	mbuf *inp4_options;	
		struct	ip_moptions *inp4_moptions; 
	} inp_depend4;
#define	inp_fport	inp_inc.inc_fport
#define	inp_lport	inp_inc.inc_lport
#define	inp_faddr	inp_inc.inc_faddr
#define	inp_laddr	inp_inc.inc_laddr
#define	inp_ip_tos	inp_depend4.inp4_ip_tos
#define	inp_options	inp_depend4.inp4_options
#define	inp_moptions	inp_depend4.inp4_moptions
	struct {
		
		struct	mbuf *inp6_options;
		
		struct	ip6_pktopts *inp6_outputopts;
		
#if 0
		struct	ip6_moptions *inp6_moptions;
#endif
		
		struct	icmp6_filter *inp6_icmp6filt;
		
		int	inp6_cksum;
		short	inp6_hops;
	} inp_depend6;
	LIST_ENTRY(inpcb) inp_portlist;
	struct	inpcbport *inp_phd;	
#define inp_zero_size offsetof(struct inpcb, inp_gencnt)
	inp_gen_t	inp_gencnt;	
	struct mtx	inp_mtx;

#define	in6p_faddr	inp_inc.inc6_faddr
#define	in6p_laddr	inp_inc.inc6_laddr
#define	in6p_hops	inp_depend6.inp6_hops	/* default hop limit */
#define	in6p_ip6_nxt	inp_ip_p
#define	in6p_flowinfo	inp_flow
#define	in6p_vflag	inp_vflag
#define	in6p_options	inp_depend6.inp6_options
#define	in6p_outputopts	inp_depend6.inp6_outputopts
#if 0
#define	in6p_moptions	inp_depend6.inp6_moptions
#endif
#define	in6p_icmp6filt	inp_depend6.inp6_icmp6filt
#define	in6p_cksum	inp_depend6.inp6_cksum
#define	in6p_flags	inp_flags  /* for KAME src sync over BSD*'s */
#define	in6p_socket	inp_socket  /* for KAME src sync over BSD*'s */
#define	in6p_lport	inp_lport  /* for KAME src sync over BSD*'s */
#define	in6p_fport	inp_fport  /* for KAME src sync over BSD*'s */
#define	in6p_ppcb	inp_ppcb  /* for KAME src sync over BSD*'s */
};











#ifdef _SYS_SOCKETVAR_H_
struct	xinpcb {
	size_t	xi_len;		
	struct	inpcb xi_inp;
	struct	xsocket xi_socket;
	u_quad_t	xi_alignment_hack;
};

struct	xinpgen {
	size_t	xig_len;	
	u_int	xig_count;	
	inp_gen_t xig_gen;	
	so_gen_t xig_sogen;	
};
#endif 

struct inpcbport {
	LIST_ENTRY(inpcbport) phd_hash;
	struct inpcbhead phd_pcblist;
	u_short phd_port;
};





struct inpcbinfo {
	


	struct inpcbhead	*ipi_listhead;
	u_int			 ipi_count;

	



	struct inpcbhead	*ipi_hashbase;
	u_long			 ipi_hashmask;

	


	struct inpcbporthead	*ipi_porthashbase;
	u_long			 ipi_porthashmask;

	


	u_short			 ipi_lastport;
	u_short			 ipi_lastlow;
	u_short			 ipi_lasthi;

	


	struct	uma_zone	*ipi_zone;

	



	u_quad_t		 ipi_gencnt;
	struct mtx		 ipi_mtx;

	



	void 			*ipi_pspare[2];
};

#define INP_LOCK_INIT(inp, d, t) \
	mtx_init(&(inp)->inp_mtx, (d), (t), MTX_DEF | MTX_RECURSE | MTX_DUPOK)
#define INP_LOCK_DESTROY(inp)	mtx_destroy(&(inp)->inp_mtx)
#define INP_LOCK(inp)		mtx_lock(&(inp)->inp_mtx)
#define INP_UNLOCK(inp)		mtx_unlock(&(inp)->inp_mtx)
#define INP_LOCK_ASSERT(inp)	mtx_assert(&(inp)->inp_mtx, MA_OWNED)
#define	INP_UNLOCK_ASSERT(inp)	mtx_assert(&(inp)->inp_mtx, MA_NOTOWNED)

#define INP_INFO_LOCK_INIT(ipi, d) \
	mtx_init(&(ipi)->ipi_mtx, (d), NULL, MTX_DEF | MTX_RECURSE)
#define INP_INFO_LOCK_DESTROY(ipi)  mtx_destroy(&(ipi)->ipi_mtx)
#define INP_INFO_RLOCK(ipi)	mtx_lock(&(ipi)->ipi_mtx)
#define INP_INFO_WLOCK(ipi)	mtx_lock(&(ipi)->ipi_mtx)
#define INP_INFO_RUNLOCK(ipi)	mtx_unlock(&(ipi)->ipi_mtx)
#define INP_INFO_WUNLOCK(ipi)	mtx_unlock(&(ipi)->ipi_mtx)
#define INP_INFO_RLOCK_ASSERT(ipi)	mtx_assert(&(ipi)->ipi_mtx, MA_OWNED)
#define INP_INFO_WLOCK_ASSERT(ipi)	mtx_assert(&(ipi)->ipi_mtx, MA_OWNED)
#define INP_INFO_UNLOCK_ASSERT(ipi)	mtx_assert(&(ipi)->ipi_mtx, MA_NOTOWNED)

#define INP_PCBHASH(faddr, lport, fport, mask) \
	(((faddr) ^ ((faddr) >> 16) ^ ntohs((lport) ^ (fport))) & (mask))
#define INP_PCBPORTHASH(lport, mask) \
	(ntohs((lport)) & (mask))


#define	INP_RECVOPTS		0x01	/* receive incoming IP options */
#define	INP_RECVRETOPTS		0x02	/* receive IP options for reply */
#define	INP_RECVDSTADDR		0x04	/* receive IP dst address */
#define	INP_HDRINCL		0x08	/* user supplies entire IP header */
#define	INP_HIGHPORT		0x10	/* user wants "high" port binding */
#define	INP_LOWPORT		0x20	/* user wants "low" port binding */
#define	INP_ANONPORT		0x40	/* port chosen for user */
#define	INP_RECVIF		0x80	/* receive incoming interface */
#define	INP_MTUDISC		0x100	/* user can do MTU discovery */
#define	INP_FAITH		0x200	/* accept FAITH'ed connections */
#define	INP_RECVTTL		0x400	/* receive incoming IP TTL */
#define	INP_DONTFRAG		0x800	/* don't fragment packet */

#define IN6P_IPV6_V6ONLY	0x008000 /* restrict AF_INET6 socket for v6 */

#define	IN6P_PKTINFO		0x010000 /* receive IP6 dst and I/F */
#define	IN6P_HOPLIMIT		0x020000 /* receive hoplimit */
#define	IN6P_HOPOPTS		0x040000 /* receive hop-by-hop options */
#define	IN6P_DSTOPTS		0x080000 /* receive dst options after rthdr */
#define	IN6P_RTHDR		0x100000 /* receive routing header */
#define	IN6P_RTHDRDSTOPTS	0x200000 /* receive dstoptions before rthdr */
#define	IN6P_TCLASS		0x400000 /* receive traffic class value */
#define	IN6P_AUTOFLOWLABEL	0x800000 /* attach flowlabel automatically */
#define	IN6P_RFC2292		0x40000000 /* used RFC2292 API on the socket */
#define	IN6P_MTU		0x80000000 /* receive path MTU */

#define	INP_CONTROLOPTS		(INP_RECVOPTS|INP_RECVRETOPTS|INP_RECVDSTADDR|\
				 INP_RECVIF|INP_RECVTTL|\
				 IN6P_PKTINFO|IN6P_HOPLIMIT|IN6P_HOPOPTS|\
				 IN6P_DSTOPTS|IN6P_RTHDR|IN6P_RTHDRDSTOPTS|\
				 IN6P_TCLASS|IN6P_AUTOFLOWLABEL|IN6P_RFC2292|\
				 IN6P_MTU)
#define	INP_UNMAPPABLEOPTS	(IN6P_HOPOPTS|IN6P_DSTOPTS|IN6P_RTHDR|\
				 IN6P_TCLASS|IN6P_AUTOFLOWLABEL)

 
#define	IN6P_HIGHPORT		INP_HIGHPORT
#define	IN6P_LOWPORT		INP_LOWPORT
#define	IN6P_ANONPORT		INP_ANONPORT
#define	IN6P_RECVIF		INP_RECVIF
#define	IN6P_MTUDISC		INP_MTUDISC
#define	IN6P_FAITH		INP_FAITH
#define	IN6P_CONTROLOPTS INP_CONTROLOPTS
	




#define	INPLOOKUP_WILDCARD	1
#define	sotoinpcb(so)	((struct inpcb *)(so)->so_pcb)
#define	sotoin6pcb(so)	sotoinpcb(so) /* for KAME src sync over BSD*'s */

#define	INP_SOCKAF(so) so->so_proto->pr_domain->dom_family

#define	INP_CHECK_SOCKAF(so, af)	(INP_SOCKAF(so) == af)


extern int	ipport_reservedhigh;
extern int	ipport_reservedlow;
extern int	ipport_lowfirstauto;
extern int	ipport_lowlastauto;
extern int	ipport_firstauto;
extern int	ipport_lastauto;
extern int	ipport_hifirstauto;
extern int	ipport_hilastauto;
extern struct callout ipport_tick_callout;

void	in_pcbpurgeif0(struct inpcbinfo *, struct ifnet *);
int	in_pcballoc(struct socket *, struct inpcbinfo *);
int	in_pcbbind(struct inpcb *, struct sockaddr *, struct ucred *);
int	in_pcbbind_setup(struct inpcb *, struct sockaddr *, in_addr_t *,
	    u_short *, struct ucred *);
int	in_pcbconnect(struct inpcb *, struct sockaddr *, struct ucred *);
int	in_pcbconnect_setup(struct inpcb *, struct sockaddr *, in_addr_t *,
	    u_short *, in_addr_t *, u_short *, struct inpcb **,
	    struct ucred *);
void	in_pcbdetach(struct inpcb *);
void	in_pcbdisconnect(struct inpcb *);
void	in_pcbdrop(struct inpcb *);
void	in_pcbfree(struct inpcb *);
int	in_pcbinshash(struct inpcb *);
struct inpcb *
	in_pcblookup_local(struct inpcbinfo *,
	    struct in_addr, u_int, int);
struct inpcb *
	in_pcblookup_hash(struct inpcbinfo *, struct in_addr, u_int,
	    struct in_addr, u_int, int, struct ifnet *);
void	in_pcbnotifyall(struct inpcbinfo *pcbinfo, struct in_addr,
	    int, struct inpcb *(*)(struct inpcb *, int));
void	in_pcbrehash(struct inpcb *);
void	in_pcbsetsolabel(struct socket *so);
int	in_getpeeraddr(struct socket *so, struct sockaddr **nam);
int	in_getsockaddr(struct socket *so, struct sockaddr **nam);
struct sockaddr *
	in_sockaddr(in_port_t port, struct in_addr *addr);
void	in_pcbsosetlabel(struct socket *so);
void	in_pcbremlists(struct inpcb *inp);
void	ipport_tick(void *xtp);




void	db_print_inpcb(struct inpcb *inp, const char *name, int indent);



#endif 
