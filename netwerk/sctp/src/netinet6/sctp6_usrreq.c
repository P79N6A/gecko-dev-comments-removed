































#ifdef __FreeBSD__
#include <sys/cdefs.h>
__FBSDID("$FreeBSD: head/sys/netinet6/sctp6_usrreq.c 243186 2012-11-17 20:04:04Z tuexen $");
#endif

#include <netinet/sctp_os.h>
#ifdef INET6
#ifdef __FreeBSD__
#include <sys/proc.h>
#endif
#include <netinet/sctp_pcb.h>
#include <netinet/sctp_header.h>
#include <netinet/sctp_var.h>
#ifdef INET6
#include <netinet6/sctp6_var.h>
#endif
#include <netinet/sctp_sysctl.h>
#include <netinet/sctp_output.h>
#include <netinet/sctp_uio.h>
#include <netinet/sctp_asconf.h>
#include <netinet/sctputil.h>
#include <netinet/sctp_indata.h>
#include <netinet/sctp_timer.h>
#include <netinet/sctp_auth.h>
#include <netinet/sctp_input.h>
#include <netinet/sctp_output.h>
#include <netinet/sctp_bsd_addr.h>
#include <netinet/sctp_crc32.h>
#if !defined(__Userspace_os_Windows)
#include <netinet/udp.h>
#endif

#if defined(__APPLE__)
#define APPLE_FILE_NO 9
#endif
#ifdef IPSEC
#include <netipsec/ipsec.h>
#ifdef INET6
#include <netipsec/ipsec6.h>
#endif 
#endif 

#if !defined(__Userspace__)
extern struct protosw inetsw[];
#endif
#if defined(__Panda__) || defined(__Userspace__)
int ip6_v6only=0;
#endif
#if defined(__Userspace__)
void
in6_sin6_2_sin(struct sockaddr_in *sin, struct sockaddr_in6 *sin6)
{
#if defined(__Userspace_os_Windows)
	uint32_t temp;
#endif
	bzero(sin, sizeof(*sin));
#ifdef HAVE_SIN_LEN
	sin->sin_len = sizeof(struct sockaddr_in);
#endif
	sin->sin_family = AF_INET;
	sin->sin_port = sin6->sin6_port;
#if defined(__Userspace_os_Windows)
	temp = sin6->sin6_addr.s6_addr16[7];
	temp = temp << 16;
	temp = temp | sin6->sin6_addr.s6_addr16[6];
	sin->sin_addr.s_addr = temp;
	sctp_print_address((struct sockaddr*)sin);
#else
	sin->sin_addr.s_addr = sin6->sin6_addr.s6_addr32[3];
#endif
}

void
in6_sin6_2_sin_in_sock(struct sockaddr *nam)
{
	struct sockaddr_in *sin_p;
	struct sockaddr_in6 sin6;

	
	sin6 = *(struct sockaddr_in6 *)nam;
	sin_p = (struct sockaddr_in *)nam;
	in6_sin6_2_sin(sin_p, &sin6);
}
#endif

#if !defined(__Userspace__)
int
#if defined(__APPLE__) || defined(__FreeBSD__)
sctp6_input_with_port(struct mbuf **i_pak, int *offp, uint16_t port)
#elif defined( __Panda__)
sctp6_input(pakhandle_type *i_pak)
#else
sctp6_input(struct mbuf **i_pak, int *offp, int proto)
#endif
{
	struct mbuf *m;
	int iphlen;
	uint32_t vrf_id;
	uint8_t ecn_bits;
	struct sockaddr_in6 src, dst;
	struct ip6_hdr *ip6;
	struct sctphdr *sh;
	struct sctp_chunkhdr *ch;
	int length, offset;
#if !defined(SCTP_WITH_NO_CSUM)
	uint8_t compute_crc;
#endif
#if defined(__FreeBSD__)
	uint32_t mflowid;
	uint8_t use_mflowid;
#endif
#if !(defined(__APPLE__) || defined (__FreeBSD__))
	uint16_t port = 0;
#endif

#if defined(__Panda__)
	
	iphlen = sizeof(struct ip6_hdr);
#else
	iphlen = *offp;
#endif
	if (SCTP_GET_PKT_VRFID(*i_pak, vrf_id)) {
		SCTP_RELEASE_PKT(*i_pak);
		return (IPPROTO_DONE);
	}
	m = SCTP_HEADER_TO_CHAIN(*i_pak);
#ifdef __Panda__
	SCTP_DETACH_HEADER_FROM_CHAIN(*i_pak);
	(void)SCTP_RELEASE_HEADER(*i_pak);
#endif
#ifdef SCTP_MBUF_LOGGING
	
	if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_MBUF_LOGGING_ENABLE) {
		struct mbuf *mat;

		for (mat = m; mat; mat = SCTP_BUF_NEXT(mat)) {
			if (SCTP_BUF_IS_EXTENDED(mat)) {
				sctp_log_mb(mat, SCTP_MBUF_INPUT);
			}
		}
	}
#endif
#ifdef SCTP_PACKET_LOGGING
	if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_LAST_PACKET_TRACING) {
		sctp_packet_log(m);
	}
#endif
#if defined(__FreeBSD__)
#if __FreeBSD_version >= 800000
	SCTPDBG(SCTP_DEBUG_CRCOFFLOAD,
	        "sctp6_input(): Packet of length %d received on %s with csum_flags 0x%x.\n",
	        m->m_pkthdr.len,
	        if_name(m->m_pkthdr.rcvif),
	        m->m_pkthdr.csum_flags);
#else
	SCTPDBG(SCTP_DEBUG_CRCOFFLOAD,
	        "sctp6_input(): Packet of length %d received on %s with csum_flags 0x%x.\n",
	        m->m_pkthdr.len,
	        m->m_pkthdr.rcvif->if_xname,
	        m->m_pkthdr.csum_flags);
#endif
#endif
#if defined(__APPLE__)
	SCTPDBG(SCTP_DEBUG_CRCOFFLOAD,
	        "sctp6_input(): Packet of length %d received on %s%d with csum_flags 0x%x.\n",
	        m->m_pkthdr.len,
	        m->m_pkthdr.rcvif->if_name,
	        m->m_pkthdr.rcvif->if_unit,
	        m->m_pkthdr.csum_flags);
#endif
#if defined(__Windows__)
	SCTPDBG(SCTP_DEBUG_CRCOFFLOAD,
	        "sctp6_input(): Packet of length %d received on %s with csum_flags 0x%x.\n",
	        m->m_pkthdr.len,
	        m->m_pkthdr.rcvif->if_xname,
	        m->m_pkthdr.csum_flags);
#endif
#if defined(__FreeBSD__)
	if (m->m_flags & M_FLOWID) {
		mflowid = m->m_pkthdr.flowid;
		use_mflowid = 1;
	} else {
		mflowid = 0;
		use_mflowid = 0;
	}
#endif
	SCTP_STAT_INCR(sctps_recvpackets);
	SCTP_STAT_INCR_COUNTER64(sctps_inpackets);
	
	offset = iphlen + sizeof(struct sctphdr) + sizeof(struct sctp_chunkhdr);
	ip6 = mtod(m, struct ip6_hdr *);
	IP6_EXTHDR_GET(sh, struct sctphdr *, m, iphlen,
		       (int)(sizeof(struct sctphdr) + sizeof(struct sctp_chunkhdr)));
	if (sh == NULL) {
		SCTP_STAT_INCR(sctps_hdrops);
		return (IPPROTO_DONE);
	}
	ch = (struct sctp_chunkhdr *)((caddr_t)sh + sizeof(struct sctphdr));
	offset -= sizeof(struct sctp_chunkhdr);
	memset(&src, 0, sizeof(struct sockaddr_in6));
	src.sin6_family = AF_INET6;
#ifdef HAVE_SIN6_LEN
	src.sin6_len = sizeof(struct sockaddr_in6);
#endif
	src.sin6_port = sh->src_port;
	src.sin6_addr = ip6->ip6_src;
#if defined(__FreeBSD__)
#if defined(__APPLE__)
	
#endif
 	if (in6_setscope(&src.sin6_addr, m->m_pkthdr.rcvif, NULL) != 0) {
		goto out;
	}
#endif
	memset(&dst, 0, sizeof(struct sockaddr_in6));
	dst.sin6_family = AF_INET6;
#ifdef HAVE_SIN6_LEN
	dst.sin6_len = sizeof(struct sockaddr_in6);
#endif
	dst.sin6_port = sh->dest_port;
	dst.sin6_addr = ip6->ip6_dst;
#if defined(__FreeBSD__)
#if defined(__APPLE__)
	
#endif
 	if (in6_setscope(&dst.sin6_addr, m->m_pkthdr.rcvif, NULL) != 0) {
		goto out;
	}
#endif
#ifdef __FreeBSD__
	if (faithprefix_p != NULL && (*faithprefix_p) (&dst.sin6_addr)) {
		
		goto out;
	}
#endif
#if defined(__APPLE__)
#if defined(NFAITH) && 0 < NFAITH
	if (faithprefix(&dst.sin6_addr)) {
		goto out;
	}
#endif
#endif
	length = ntohs(ip6->ip6_plen) + iphlen;
	
	if (SCTP_HEADER_LEN(m) != length) {
		SCTPDBG(SCTP_DEBUG_INPUT1,
		        "sctp6_input() length:%d reported length:%d\n", length, SCTP_HEADER_LEN(m));
		SCTP_STAT_INCR(sctps_hdrops);
		goto out;
	}
	if (IN6_IS_ADDR_MULTICAST(&ip6->ip6_dst)) {
		goto out;
	}
	ecn_bits = ((ntohl(ip6->ip6_flow) >> 20) & 0x000000ff);
#if defined(SCTP_WITH_NO_CSUM)
	SCTP_STAT_INCR(sctps_recvnocrc);
#else
#if defined(__FreeBSD__) && __FreeBSD_version >= 800000
	if (m->m_pkthdr.csum_flags & CSUM_SCTP_VALID) {
		SCTP_STAT_INCR(sctps_recvhwcrc);
		compute_crc = 0;
	} else {
#else
	if (SCTP_BASE_SYSCTL(sctp_no_csum_on_loopback) &&
	    (IN6_ARE_ADDR_EQUAL(&src.sin6_addr, &dst.sin6_addr))) {
		SCTP_STAT_INCR(sctps_recvnocrc);
		compute_crc = 0;
	} else {
#endif
		SCTP_STAT_INCR(sctps_recvswcrc);
		compute_crc = 1;
	}
#endif
	sctp_common_input_processing(&m, iphlen, offset, length,
	                             (struct sockaddr *)&src,
	                             (struct sockaddr *)&dst,
	                             sh, ch,
#if !defined(SCTP_WITH_NO_CSUM)
	                             compute_crc,
#endif
	                             ecn_bits,
#if defined(__FreeBSD__)
	                             use_mflowid, mflowid,
#endif
	                             vrf_id, port);
 out:
	if (m) {
		sctp_m_freem(m);
	}
	return (IPPROTO_DONE);
}

#if defined(__APPLE__)
int
sctp6_input(struct mbuf **i_pak, int *offp)
{
	return (sctp6_input_with_port(i_pak, offp, 0));
}
#endif

#if defined(__FreeBSD__)
int
sctp6_input(struct mbuf **i_pak, int *offp, int proto SCTP_UNUSED)
{
	return (sctp6_input_with_port(i_pak, offp, 0));
}
#endif

#if defined(__Panda__)
void
#else
static void
#endif
sctp6_notify_mbuf(struct sctp_inpcb *inp, struct icmp6_hdr *icmp6,
		  struct sctphdr *sh, struct sctp_tcb *stcb, struct sctp_nets *net)
{
	uint32_t nxtsz;

	if ((inp == NULL) || (stcb == NULL) || (net == NULL) ||
	    (icmp6 == NULL) || (sh == NULL)) {
		goto out;
	}
	
	if (ntohl(sh->v_tag) != (stcb->asoc.peer_vtag))
		goto out;

	if (icmp6->icmp6_type != ICMP6_PACKET_TOO_BIG) {
		
		goto out;
	}
	





	nxtsz = ntohl(icmp6->icmp6_mtu);
	
	sctp_timer_stop(SCTP_TIMER_TYPE_PATHMTURAISE, inp, stcb, NULL, SCTP_FROM_SCTP6_USRREQ+SCTP_LOC_1);

	
	if (net->mtu > nxtsz) {
		net->mtu = nxtsz;
		if (net->port) {
			net->mtu -= sizeof(struct udphdr);
		}
	}
	
	if (stcb->asoc.smallest_mtu > nxtsz) {
		struct sctp_tmit_chunk *chk;

		
		stcb->asoc.smallest_mtu = nxtsz;
		

		TAILQ_FOREACH(chk, &stcb->asoc.send_queue, sctp_next) {
			if ((uint32_t) (chk->send_size + IP_HDR_SIZE) > nxtsz) {
				chk->flags |= CHUNK_FLAGS_FRAGMENT_OK;
			}
		}
		TAILQ_FOREACH(chk, &stcb->asoc.sent_queue, sctp_next) {
			if ((uint32_t) (chk->send_size + IP_HDR_SIZE) > nxtsz) {
				



				chk->flags |= CHUNK_FLAGS_FRAGMENT_OK;
				if (chk->sent != SCTP_DATAGRAM_RESEND)
					stcb->asoc.sent_queue_retran_cnt++;
				chk->sent = SCTP_DATAGRAM_RESEND;
				chk->rec.data.doing_fast_retransmit = 0;

				chk->sent = SCTP_DATAGRAM_RESEND;
				
				chk->sent_rcv_time.tv_sec = 0;
				chk->sent_rcv_time.tv_usec = 0;
				stcb->asoc.total_flight -= chk->send_size;
				net->flight_size -= chk->send_size;
			}
		}
	}
	sctp_timer_start(SCTP_TIMER_TYPE_PATHMTURAISE, inp, stcb, NULL);
out:
	if (stcb) {
		SCTP_TCB_UNLOCK(stcb);
	}
}
#endif


void
sctp6_notify(struct sctp_inpcb *inp,
    struct icmp6_hdr *icmph,
    struct sctphdr *sh,
    struct sockaddr *to,
    struct sctp_tcb *stcb,
    struct sctp_nets *net)
{
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
	struct socket *so;

#endif

	
	if ((inp == NULL) || (stcb == NULL) || (net == NULL) ||
	    (sh == NULL) || (to == NULL)) {
		if (stcb)
			SCTP_TCB_UNLOCK(stcb);
		return;
	}
	
	if (ntohl(sh->v_tag) != (stcb->asoc.peer_vtag)) {
		SCTP_TCB_UNLOCK(stcb);
		return;
	}
	if (icmph->icmp6_type != ICMP_UNREACH) {
		
		SCTP_TCB_UNLOCK(stcb);
		return;
	}
	if ((icmph->icmp6_code == ICMP_UNREACH_NET) ||
	    (icmph->icmp6_code == ICMP_UNREACH_HOST) ||
	    (icmph->icmp6_code == ICMP_UNREACH_NET_UNKNOWN) ||
	    (icmph->icmp6_code == ICMP_UNREACH_HOST_UNKNOWN) ||
	    (icmph->icmp6_code == ICMP_UNREACH_ISOLATED) ||
	    (icmph->icmp6_code == ICMP_UNREACH_NET_PROHIB) ||
	    (icmph->icmp6_code == ICMP_UNREACH_HOST_PROHIB) ||
#ifdef __Panda__
            (icmph->icmp6_code == ICMP_UNREACH_ADMIN)) {
#else
            (icmph->icmp6_code == ICMP_UNREACH_FILTER_PROHIB)) {
#endif







		if (net->dest_state & SCTP_ADDR_REACHABLE) {
			
			net->dest_state &= ~SCTP_ADDR_REACHABLE;
			net->dest_state &= ~SCTP_ADDR_PF;
			sctp_ulp_notify(SCTP_NOTIFY_INTERFACE_DOWN,
					stcb, 0, (void *)net, SCTP_SO_NOT_LOCKED);
		}
		SCTP_TCB_UNLOCK(stcb);
	} else  if ((icmph->icmp6_code == ICMP_UNREACH_PROTOCOL) ||
		    (icmph->icmp6_code == ICMP_UNREACH_PORT)) {
		







		sctp_abort_notification(stcb, 1, 0, NULL, SCTP_SO_NOT_LOCKED);
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
		so = SCTP_INP_SO(inp);
		atomic_add_int(&stcb->asoc.refcnt, 1);
		SCTP_TCB_UNLOCK(stcb);
		SCTP_SOCKET_LOCK(so, 1);
		SCTP_TCB_LOCK(stcb);
		atomic_subtract_int(&stcb->asoc.refcnt, 1);
#endif
		(void)sctp_free_assoc(inp, stcb, SCTP_NORMAL_PROC, SCTP_FROM_SCTP_USRREQ+SCTP_LOC_2);
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
		SCTP_SOCKET_UNLOCK(so, 1);
		
#endif
		
	} else {
		SCTP_TCB_UNLOCK(stcb);
	}
}



#if !defined(__Panda__) && !defined(__Userspace__)
void
sctp6_ctlinput(int cmd, struct sockaddr *pktdst, void *d)
{
	struct sctphdr sh;
	struct ip6ctlparam *ip6cp = NULL;
	uint32_t vrf_id;

#if defined(__FreeBSD__) || defined(__APPLE__) || defined(__Windows__)
	vrf_id = SCTP_DEFAULT_VRFID;
#endif

#ifdef HAVE_SA_LEN
	if (pktdst->sa_family != AF_INET6 ||
	    pktdst->sa_len != sizeof(struct sockaddr_in6))
#else
	if (pktdst->sa_family != AF_INET6)
#endif
		return;

	if ((unsigned)cmd >= PRC_NCMDS)
		return;
	if (PRC_IS_REDIRECT(cmd)) {
		d = NULL;
	} else if (inet6ctlerrmap[cmd] == 0) {
		return;
	}
	
	if (d != NULL) {
		ip6cp = (struct ip6ctlparam *)d;
	} else {
		ip6cp = (struct ip6ctlparam *)NULL;
	}

	if (ip6cp) {
		



		
		struct sctp_inpcb *inp = NULL;
		struct sctp_tcb *stcb = NULL;
		struct sctp_nets *net = NULL;
		struct sockaddr_in6 final;

		if (ip6cp->ip6c_m == NULL)
			return;

		bzero(&sh, sizeof(sh));
		bzero(&final, sizeof(final));
		inp = NULL;
		net = NULL;
		m_copydata(ip6cp->ip6c_m, ip6cp->ip6c_off, sizeof(sh),
		    (caddr_t)&sh);
		ip6cp->ip6c_src->sin6_port = sh.src_port;
#ifdef HAVE_SIN6_LEN
		final.sin6_len = sizeof(final);
#endif
		final.sin6_family = AF_INET6;
#if defined(__FreeBSD__) && __FreeBSD_cc_version < 440000
		final.sin6_addr = *ip6cp->ip6c_finaldst;
#else
		final.sin6_addr = ((struct sockaddr_in6 *)pktdst)->sin6_addr;
#endif				
		final.sin6_port = sh.dest_port;
		stcb = sctp_findassociation_addr_sa((struct sockaddr *)&final,
		    (struct sockaddr *)ip6cp->ip6c_src,
		    &inp, &net, 1, vrf_id);
		
		if (stcb != NULL && inp && (inp->sctp_socket != NULL)) {
			if (cmd == PRC_MSGSIZE) {
				sctp6_notify_mbuf(inp,
				    ip6cp->ip6c_icmp6,
				    &sh,
				    stcb,
				    net);
				
			} else {
				sctp6_notify(inp, ip6cp->ip6c_icmp6, &sh,
				    (struct sockaddr *)&final,
				    stcb, net);
				
			}
		} else {
#if !defined(__Windows__)
			if (PRC_IS_REDIRECT(cmd) && inp) {
				in6_rtchange((struct in6pcb *)inp,
				    inet6ctlerrmap[cmd]);
			}
#endif
			if (inp) {
				
				SCTP_INP_WLOCK(inp);
				SCTP_INP_DECR_REF(inp);
				SCTP_INP_WUNLOCK(inp);
			}
			if (stcb)
				SCTP_TCB_UNLOCK(stcb);
		}
	}
}
#endif





#ifdef __FreeBSD__
static int
sctp6_getcred(SYSCTL_HANDLER_ARGS)
{
	struct xucred xuc;
	struct sockaddr_in6 addrs[2];
	struct sctp_inpcb *inp;
	struct sctp_nets *net;
	struct sctp_tcb *stcb;
	int error;
	uint32_t vrf_id;

#if defined(__FreeBSD__) || defined(__APPLE__)
	vrf_id = SCTP_DEFAULT_VRFID;
#else
	vrf_id = panda_get_vrf_from_call(); 
#endif

#if defined(__FreeBSD__) && __FreeBSD_version > 602000
	error = priv_check(req->td, PRIV_NETINET_GETCRED);
#elif defined(__FreeBSD__) && __FreeBSD_version >= 500000
	error = suser(req->td);
#else
	error = suser(req->p);
#endif
	if (error)
		return (error);

	if (req->newlen != sizeof(addrs)) {
		SCTP_LTRACE_ERR_RET(NULL, NULL, NULL, SCTP_FROM_SCTP6_USRREQ, EINVAL);
		return (EINVAL);
	}
	if (req->oldlen != sizeof(struct ucred)) {
		SCTP_LTRACE_ERR_RET(NULL, NULL, NULL, SCTP_FROM_SCTP6_USRREQ, EINVAL);
		return (EINVAL);
	}
	error = SYSCTL_IN(req, addrs, sizeof(addrs));
	if (error)
		return (error);

	stcb = sctp_findassociation_addr_sa(sin6tosa(&addrs[1]),
	    sin6tosa(&addrs[0]),
	    &inp, &net, 1, vrf_id);
	if (stcb == NULL || inp == NULL || inp->sctp_socket == NULL) {
		if ((inp != NULL) && (stcb == NULL)) {
			
			SCTP_INP_WLOCK(inp);
			SCTP_INP_DECR_REF(inp);
			goto cred_can_cont;
		}
		SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP6_USRREQ, ENOENT);
		error = ENOENT;
		goto out;
	}
	SCTP_TCB_UNLOCK(stcb);
	






	SCTP_INP_WLOCK(inp);
 cred_can_cont:
	error = cr_canseesocket(req->td->td_ucred, inp->sctp_socket);
	if (error) {
		SCTP_INP_WUNLOCK(inp);
		goto out;
	}
	cru2x(inp->sctp_socket->so_cred, &xuc);
	SCTP_INP_WUNLOCK(inp);
	error = SYSCTL_OUT(req, &xuc, sizeof(struct xucred));
out:
	return (error);
}

SYSCTL_PROC(_net_inet6_sctp6, OID_AUTO, getcred, CTLTYPE_OPAQUE | CTLFLAG_RW,
    0, 0,
    sctp6_getcred, "S,ucred", "Get the ucred of a SCTP6 connection");

#endif


#if (defined(__FreeBSD__) && __FreeBSD_version > 690000) || defined(__Windows__)
static void
#elif defined(__Panda__) || defined(__Userspace__)
int
#else
static int
#endif
sctp6_abort(struct socket *so)
{
	struct sctp_inpcb *inp;
	uint32_t flags;

	inp = (struct sctp_inpcb *)so->so_pcb;
	if (inp == NULL) {
		SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP6_USRREQ, EINVAL);
#if (defined(__FreeBSD__) && __FreeBSD_version > 690000) || defined(__Windows__)
		return;
#else
		return (EINVAL);
#endif
	}
 sctp_must_try_again:
	flags = inp->sctp_flags;
#ifdef SCTP_LOG_CLOSING
	sctp_log_closing(inp, NULL, 17);
#endif
	if (((flags & SCTP_PCB_FLAGS_SOCKET_GONE) == 0) &&
	    (atomic_cmpset_int(&inp->sctp_flags, flags, (flags | SCTP_PCB_FLAGS_SOCKET_GONE | SCTP_PCB_FLAGS_CLOSE_IP)))) {
#ifdef SCTP_LOG_CLOSING
		sctp_log_closing(inp, NULL, 16);
#endif
		sctp_inpcb_free(inp, SCTP_FREE_SHOULD_USE_ABORT,
				SCTP_CALLED_AFTER_CMPSET_OFCLOSE);
		SOCK_LOCK(so);
		SCTP_SB_CLEAR(so->so_snd);
		


		SCTP_SB_CLEAR(so->so_rcv);
#if defined(__APPLE__)
		so->so_usecount--;
#else
		
		so->so_pcb = NULL;
#endif
		SOCK_UNLOCK(so);
	} else {
		flags = inp->sctp_flags;
		if ((flags & SCTP_PCB_FLAGS_SOCKET_GONE) == 0) {
			goto sctp_must_try_again;
		}
	}
#if (defined(__FreeBSD__) && __FreeBSD_version > 690000) || defined(__Windows__)
	return;
#else
	return (0);
#endif
}

#if defined(__FreeBSD__) && __FreeBSD_version >= 500000
static int
sctp6_attach(struct socket *so, int proto SCTP_UNUSED, struct thread *p SCTP_UNUSED)
#elif defined(__Panda__) || defined(__Userspace__)
int
sctp6_attach(struct socket *so, int proto SCTP_UNUSED, uint32_t vrf_id)
#elif defined(__Windows__)
static int
sctp6_attach(struct socket *so, int proto SCTP_UNUSED, PKTHREAD p SCTP_UNUSED)
#else
static int
sctp6_attach(struct socket *so, int proto SCTP_UNUSED, struct proc *p SCTP_UNUSED)
#endif
{
	struct in6pcb *inp6;
	int error;
	struct sctp_inpcb *inp;
#if !defined(__Panda__) && !defined(__Userspace__)
	uint32_t vrf_id = SCTP_DEFAULT_VRFID;
#endif

	inp = (struct sctp_inpcb *)so->so_pcb;
	if (inp != NULL) {
		SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP6_USRREQ, EINVAL);
		return (EINVAL);
	}

	if (so->so_snd.sb_hiwat == 0 || so->so_rcv.sb_hiwat == 0) {
		error = SCTP_SORESERVE(so, SCTP_BASE_SYSCTL(sctp_sendspace), SCTP_BASE_SYSCTL(sctp_recvspace));
		if (error)
			return (error);
	}
	error = sctp_inpcb_alloc(so, vrf_id);
	if (error)
		return (error);
	inp = (struct sctp_inpcb *)so->so_pcb;
	SCTP_INP_WLOCK(inp);
	inp->sctp_flags |= SCTP_PCB_FLAGS_BOUND_V6;	
	inp6 = (struct in6pcb *)inp;

#if defined(__FreeBSD__) || defined(__APPLE__) || defined(__Windows__) || defined(__Userspace__)
	inp6->inp_vflag |= INP_IPV6;
#else
	inp->inp_vflag |= INP_IPV6;
#endif
#if !defined(__Panda__)
	inp6->in6p_hops = -1;	
	inp6->in6p_cksum = -1;	
#endif
#ifdef INET
	




	inp6->inp_ip_ttl = MODULE_GLOBAL(ip_defttl);
#endif
	



	SCTP_INP_WUNLOCK(inp);
	return (0);
}

#if defined(__FreeBSD__) && __FreeBSD_version >= 500000
static int
sctp6_bind(struct socket *so, struct sockaddr *addr, struct thread *p)
{
#elif defined(__FreeBSD__) || defined(__APPLE__)
static int
sctp6_bind(struct socket *so, struct sockaddr *addr, struct proc *p)
{
#elif defined(__Panda__) || defined(__Userspace__)
int
sctp6_bind(struct socket *so, struct sockaddr *addr, void * p)
{
#elif defined(__Windows__)
static int
sctp6_bind(struct socket *so, struct sockaddr *addr, PKTHREAD p)
{
#else
static int
sctp6_bind(struct socket *so, struct mbuf *nam, struct proc *p)
{
	struct sockaddr *addr = nam ? mtod(nam, struct sockaddr *): NULL;

#endif
	struct sctp_inpcb *inp;
	struct in6pcb *inp6;
	int error;

	inp = (struct sctp_inpcb *)so->so_pcb;
	if (inp == NULL) {
		SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP6_USRREQ, EINVAL);
		return (EINVAL);
	}

#if !defined(__Windows__)
	if (addr) {
		switch (addr->sa_family) {
#ifdef INET
		case AF_INET:
#ifdef HAVE_SA_LEN
			if (addr->sa_len != sizeof(struct sockaddr_in)) {
				SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP6_USRREQ, EINVAL);
				return (EINVAL);
			}
#endif
			break;
#endif
#ifdef INET6
		case AF_INET6:
#ifdef HAVE_SA_LEN
			if (addr->sa_len != sizeof(struct sockaddr_in6)) {
				SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP6_USRREQ, EINVAL);
				return (EINVAL);
			}
#endif
			break;
#endif
		default:
			SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP6_USRREQ, EINVAL);
			return (EINVAL);
		}
	}
#endif
	inp6 = (struct in6pcb *)inp;
#if defined(__FreeBSD__) || defined(__APPLE__) || defined(__Windows__) || defined(__Userspace__)
	inp6->inp_vflag &= ~INP_IPV4;
	inp6->inp_vflag |= INP_IPV6;
#else
	inp->inp_vflag &= ~INP_IPV4;
	inp->inp_vflag |= INP_IPV6;
#endif
	if ((addr != NULL) && (SCTP_IPV6_V6ONLY(inp6) == 0)) {
		switch (addr->sa_family) {
#ifdef INET
		case AF_INET:
			
#if defined(__FreeBSD__) || defined(__APPLE__) || defined(__Windows__) || defined(__Userspace__)
			inp6->inp_vflag |= INP_IPV4;
			inp6->inp_vflag &= ~INP_IPV6;
#else
			inp->inp_vflag |= INP_IPV4;
			inp->inp_vflag &= ~INP_IPV6;
#endif
			break;
#endif
#ifdef INET6
		case AF_INET6:
		{
			struct sockaddr_in6 *sin6_p;

			sin6_p = (struct sockaddr_in6 *)addr;

			if (IN6_IS_ADDR_UNSPECIFIED(&sin6_p->sin6_addr)) {
#if defined(__FreeBSD__) || defined(__APPLE__) || defined(__Windows__) || defined(__Userspace__)
				inp6->inp_vflag |= INP_IPV4;
#else
				inp->inp_vflag |= INP_IPV4;
#endif
			}
#ifdef INET
			if (IN6_IS_ADDR_V4MAPPED(&sin6_p->sin6_addr)) {
				struct sockaddr_in sin;

				in6_sin6_2_sin(&sin, sin6_p);
#if defined(__FreeBSD__) || defined(__APPLE__) || defined(__Windows__) || defined(__Userspace__)
				inp6->inp_vflag |= INP_IPV4;
				inp6->inp_vflag &= ~INP_IPV6;
#else
				inp->inp_vflag |= INP_IPV4;
				inp->inp_vflag &= ~INP_IPV6;
#endif
				error = sctp_inpcb_bind(so, (struct sockaddr *)&sin, NULL, p);
				return (error);
			}
#endif
			break;
		}
#endif
		default:
			break;
		}
	} else if (addr != NULL) {
		struct sockaddr_in6 *sin6_p;

		
#ifdef INET
		if (addr->sa_family == AF_INET) {
			
			SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP6_USRREQ, EINVAL);
			return (EINVAL);
		}
#endif
		sin6_p = (struct sockaddr_in6 *)addr;

		if (IN6_IS_ADDR_V4MAPPED(&sin6_p->sin6_addr)) {
			
			
			SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP6_USRREQ, EINVAL);
			return (EINVAL);
		}
	}
	error = sctp_inpcb_bind(so, addr, NULL, p);
	return (error);
}


#if (defined(__FreeBSD__) && __FreeBSD_version > 690000) || defined(__Windows__) || defined(__Userspace__)
#if !defined(__Userspace__)
static void
#else
void
#endif
sctp6_close(struct socket *so)
{
	sctp_close(so);
}


#else

#if !defined(__Panda__)
static
#endif
int
sctp6_detach(struct socket *so)
{
#if defined(__Userspace__)
	sctp_close(so);
	return (0);
#else
	return (sctp_detach(so));
#endif
}

#endif

#if !defined(__Panda__) && !defined(__Userspace__)
static
#endif
int
sctp6_disconnect(struct socket *so)
{
	return (sctp_disconnect(so));
}


int
#if defined(__FreeBSD__) && __FreeBSD_version >= 500000
sctp_sendm(struct socket *so, int flags, struct mbuf *m, struct sockaddr *addr,
    struct mbuf *control, struct thread *p);

#else
sctp_sendm(struct socket *so, int flags, struct mbuf *m, struct sockaddr *addr,
    struct mbuf *control, struct proc *p);

#endif

#if !defined(__Panda__) && !defined(__Windows__) && !defined(__Userspace__)
#if defined(__FreeBSD__) && __FreeBSD_version >= 500000
static int
sctp6_send(struct socket *so, int flags, struct mbuf *m, struct sockaddr *addr,
    struct mbuf *control, struct thread *p)
{
#elif defined(__FreeBSD__) || defined(__APPLE__)
static int
sctp6_send(struct socket *so, int flags, struct mbuf *m, struct sockaddr *addr,
    struct mbuf *control, struct proc *p)
{
#else
static int
sctp6_send(struct socket *so, int flags, struct mbuf *m, struct mbuf *nam,
    struct mbuf *control, struct proc *p)
{
	struct sockaddr *addr = nam ? mtod(nam, struct sockaddr *): NULL;
#endif
	struct sctp_inpcb *inp;
	struct in6pcb *inp6;

#ifdef INET
	struct sockaddr_in6 *sin6;
#endif 
	

	inp = (struct sctp_inpcb *)so->so_pcb;
	if (inp == NULL) {
		if (control) {
			SCTP_RELEASE_PKT(control);
			control = NULL;
		}
		SCTP_RELEASE_PKT(m);
		SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP6_USRREQ, EINVAL);
		return (EINVAL);
	}
	inp6 = (struct in6pcb *)inp;
	



	if ((inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED) &&
	    (addr == NULL)) {
		goto connected_type;
	}
	if (addr == NULL) {
		SCTP_RELEASE_PKT(m);
		if (control) {
			SCTP_RELEASE_PKT(control);
			control = NULL;
		}
		SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP6_USRREQ, EDESTADDRREQ);
		return (EDESTADDRREQ);
	}
#ifdef INET
	sin6 = (struct sockaddr_in6 *)addr;
	if (SCTP_IPV6_V6ONLY(inp6)) {
		



		if (addr->sa_family == AF_INET) {
			SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP6_USRREQ, EINVAL);
			return (EINVAL);
		}
		if (IN6_IS_ADDR_V4MAPPED(&sin6->sin6_addr)) {
			SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP6_USRREQ, EINVAL);
			return (EINVAL);
		}
	}
	if (IN6_IS_ADDR_V4MAPPED(&sin6->sin6_addr)) {
		if (!MODULE_GLOBAL(ip6_v6only)) {
			struct sockaddr_in sin;

			
			in6_sin6_2_sin(&sin, sin6);
			return (sctp_sendm(so, flags, m, (struct sockaddr *)&sin,
			    control, p));
		} else {
			
			SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP6_USRREQ, EINVAL);
			return (EINVAL);
		}
	}
#endif				
connected_type:
	
	if (control) {
		if (inp->control) {
			SCTP_PRINTF("huh? control set?\n");
			SCTP_RELEASE_PKT(inp->control);
			inp->control = NULL;
		}
		inp->control = control;
	}
	
	if (inp->pkt) {
		SCTP_BUF_NEXT(inp->pkt_last) = m;
		inp->pkt_last = m;
	} else {
		inp->pkt_last = inp->pkt = m;
	}
	if (
#if defined(__FreeBSD__) || defined(__APPLE__)
	
	    ((flags & PRUS_MORETOCOME) == 0)
#else
	    1			

#endif
	    ) {
		






		int ret;

		ret = sctp_output(inp, inp->pkt, addr, inp->control, p, flags);
		inp->pkt = NULL;
		inp->control = NULL;
		return (ret);
	} else {
		return (0);
	}
}
#endif

#if defined(__FreeBSD__) && __FreeBSD_version >= 500000
static int
sctp6_connect(struct socket *so, struct sockaddr *addr, struct thread *p)
{
#elif defined(__FreeBSD__) || defined(__APPLE__)
static int
sctp6_connect(struct socket *so, struct sockaddr *addr, struct proc *p)
{
#elif defined(__Panda__)
int
sctp6_connect(struct socket *so, struct sockaddr *addr, void *p)
{
#elif defined(__Windows__)
static int
sctp6_connect(struct socket *so, struct sockaddr *addr, PKTHREAD p)
{
#elif defined(__Userspace__)
int
sctp6_connect(struct socket *so, struct sockaddr *addr)
{
	void *p = NULL;
#else
static int
sctp6_connect(struct socket *so, struct mbuf *nam, struct proc *p)
{
	struct sockaddr *addr = mtod(nam, struct sockaddr *);
#endif
	uint32_t vrf_id;
	int error = 0;
	struct sctp_inpcb *inp;
	struct in6pcb *inp6;
	struct sctp_tcb *stcb;
#ifdef INET
	struct sockaddr_in6 *sin6;
	struct sockaddr_storage ss;
#endif

	inp6 = (struct in6pcb *)so->so_pcb;
	inp = (struct sctp_inpcb *)so->so_pcb;
	if (inp == NULL) {
		SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP6_USRREQ, ECONNRESET);
		return (ECONNRESET);	

	}
	if (addr == NULL) {
		SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP6_USRREQ, EINVAL);
		return (EINVAL);
	}
#if !defined(__Windows__)
	switch (addr->sa_family) {
#ifdef INET
	case AF_INET:
#ifdef HAVE_SA_LEN
		if (addr->sa_len != sizeof(struct sockaddr_in)) {
			SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP6_USRREQ, EINVAL);
			return (EINVAL);
		}
#endif
		break;
#endif
#ifdef INET6
	case AF_INET6:
#ifdef HAVE_SA_LEN
		if (addr->sa_len != sizeof(struct sockaddr_in6)) {
			SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP6_USRREQ, EINVAL);
			return (EINVAL);
		}
#endif
		break;
#endif
	default:
		SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP6_USRREQ, EINVAL);
		return (EINVAL);
	}
#endif

	vrf_id = inp->def_vrf_id;
	SCTP_ASOC_CREATE_LOCK(inp);
	SCTP_INP_RLOCK(inp);
	if ((inp->sctp_flags & SCTP_PCB_FLAGS_UNBOUND) ==
	    SCTP_PCB_FLAGS_UNBOUND) {
		
		SCTP_INP_RUNLOCK(inp);
		error = sctp6_bind(so, NULL, p);
		if (error) {
			SCTP_ASOC_CREATE_UNLOCK(inp);

			return (error);
		}
		SCTP_INP_RLOCK(inp);
	}
	if ((inp->sctp_flags & SCTP_PCB_FLAGS_TCPTYPE) &&
	    (inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED)) {
		
		SCTP_INP_RUNLOCK(inp);
		SCTP_ASOC_CREATE_UNLOCK(inp);
		SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP6_USRREQ, EADDRINUSE);
		return (EADDRINUSE);
	}
#ifdef INET
	sin6 = (struct sockaddr_in6 *)addr;
	if (SCTP_IPV6_V6ONLY(inp6)) {
		



		if (addr->sa_family == AF_INET) {
			SCTP_INP_RUNLOCK(inp);
			SCTP_ASOC_CREATE_UNLOCK(inp);
			SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP6_USRREQ, EINVAL);
			return (EINVAL);
		}
		if (IN6_IS_ADDR_V4MAPPED(&sin6->sin6_addr)) {
			SCTP_INP_RUNLOCK(inp);
			SCTP_ASOC_CREATE_UNLOCK(inp);
			SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP6_USRREQ, EINVAL);
			return (EINVAL);
		}
	}
	if (IN6_IS_ADDR_V4MAPPED(&sin6->sin6_addr)) {
		if (!MODULE_GLOBAL(ip6_v6only)) {
			
			in6_sin6_2_sin((struct sockaddr_in *)&ss, sin6);
			addr = (struct sockaddr *)&ss;
		} else {
			
			SCTP_INP_RUNLOCK(inp);
			SCTP_ASOC_CREATE_UNLOCK(inp);
			SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP6_USRREQ, EINVAL);
			return (EINVAL);
		}
	}
#endif				
	
	if (inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED) {
		stcb = LIST_FIRST(&inp->sctp_asoc_list);
		if (stcb) {
			SCTP_TCB_UNLOCK(stcb);
		}
		SCTP_INP_RUNLOCK(inp);
	} else {
		SCTP_INP_RUNLOCK(inp);
		SCTP_INP_WLOCK(inp);
		SCTP_INP_INCR_REF(inp);
		SCTP_INP_WUNLOCK(inp);
		stcb = sctp_findassociation_ep_addr(&inp, addr, NULL, NULL, NULL);
		if (stcb == NULL) {
			SCTP_INP_WLOCK(inp);
			SCTP_INP_DECR_REF(inp);
			SCTP_INP_WUNLOCK(inp);
		}
	}

	if (stcb != NULL) {
		
		SCTP_ASOC_CREATE_UNLOCK(inp);
		SCTP_TCB_UNLOCK(stcb);
		SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP6_USRREQ, EALREADY);
		return (EALREADY);
	}
	
	stcb = sctp_aloc_assoc(inp, addr, &error, 0, vrf_id, p);
	SCTP_ASOC_CREATE_UNLOCK(inp);
	if (stcb == NULL) {
		
		return (error);
	}
	if (stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_TCPTYPE) {
		stcb->sctp_ep->sctp_flags |= SCTP_PCB_FLAGS_CONNECTED;
		
		soisconnecting(so);
	}
	stcb->asoc.state = SCTP_STATE_COOKIE_WAIT;
	(void)SCTP_GETTIME_TIMEVAL(&stcb->asoc.time_entered);

	
	sctp_initialize_auth_params(inp, stcb);

	sctp_send_initiate(inp, stcb, SCTP_SO_LOCKED);
	SCTP_TCB_UNLOCK(stcb);
	return (error);
}

static int
#if defined(__FreeBSD__) || defined(__APPLE__) || defined(__Windows__)
sctp6_getaddr(struct socket *so, struct sockaddr **addr)
{
	struct sockaddr_in6 *sin6;
#elif defined(__Panda__)
sctp6_getaddr(struct socket *so, struct sockaddr *addr)
{
	struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)addr;
#else
sctp6_getaddr(struct socket *so, struct mbuf *nam)
{
	struct sockaddr_in6 *sin6 = mtod(nam, struct sockaddr_in6 *);
#endif
	struct sctp_inpcb *inp;
	uint32_t vrf_id;
	struct sctp_ifa *sctp_ifa;

#ifdef SCTP_KAME
	int error;
#endif 

	


#if defined(__FreeBSD__) || defined(__APPLE__) || defined(__Windows__)
	SCTP_MALLOC_SONAME(sin6, struct sockaddr_in6 *, sizeof(*sin6));
	if (sin6 == NULL)
		return (ENOMEM);
#elif defined(__Panda__)
	bzero(sin6, sizeof(*sin6));
#else
	SCTP_BUF_LEN(nam) = sizeof(*sin6);
	bzero(sin6, sizeof(*sin6));
#endif
	sin6->sin6_family = AF_INET6;
#ifdef HAVE_SIN6_LEN
	sin6->sin6_len = sizeof(*sin6);
#endif

	inp = (struct sctp_inpcb *)so->so_pcb;
	if (inp == NULL) {
#if defined(__FreeBSD__) || defined(__APPLE__) || defined(__Windows__)
		SCTP_FREE_SONAME(sin6);
#endif
		SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP6_USRREQ, ECONNRESET);
		return (ECONNRESET);
	}
	SCTP_INP_RLOCK(inp);
	sin6->sin6_port = inp->sctp_lport;
	if (inp->sctp_flags & SCTP_PCB_FLAGS_BOUNDALL) {
		
		if (inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED) {
			struct sctp_tcb *stcb;
			struct sockaddr_in6 *sin_a6;
			struct sctp_nets *net;
			int fnd;
			stcb = LIST_FIRST(&inp->sctp_asoc_list);
			if (stcb == NULL) {
				goto notConn6;
			}
			fnd = 0;
			sin_a6 = NULL;
			TAILQ_FOREACH(net, &stcb->asoc.nets, sctp_next) {
				sin_a6 = (struct sockaddr_in6 *)&net->ro._l_addr;
				if (sin_a6 == NULL)
					
					continue;

				if (sin_a6->sin6_family == AF_INET6) {
					fnd = 1;
					break;
				}
			}
			if ((!fnd) || (sin_a6 == NULL)) {
				
				goto notConn6;
			}
			vrf_id = inp->def_vrf_id;
			sctp_ifa = sctp_source_address_selection(inp, stcb, (sctp_route_t *)&net->ro, net, 0, vrf_id);
			if (sctp_ifa) {
				sin6->sin6_addr = sctp_ifa->address.sin6.sin6_addr;
			}
		} else {
			
	notConn6:
			memset(&sin6->sin6_addr, 0, sizeof(sin6->sin6_addr));
		}
	} else {
		
		struct sctp_laddr *laddr;
		int fnd = 0;

		LIST_FOREACH(laddr, &inp->sctp_addr_list, sctp_nxt_addr) {
			if (laddr->ifa->address.sa.sa_family == AF_INET6) {
				struct sockaddr_in6 *sin_a;

				sin_a = (struct sockaddr_in6 *)&laddr->ifa->address.sin6;
				sin6->sin6_addr = sin_a->sin6_addr;
				fnd = 1;
				break;
			}
		}
		if (!fnd) {
#if defined(__FreeBSD__) || defined(__APPLE__) || defined(__Windows__)
			SCTP_FREE_SONAME(sin6);
#endif
			SCTP_INP_RUNLOCK(inp);
			SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP6_USRREQ, ENOENT);
			return (ENOENT);
		}
	}
	SCTP_INP_RUNLOCK(inp);
	
#ifdef SCTP_EMBEDDED_V6_SCOPE
#ifdef SCTP_KAME
	if ((error = sa6_recoverscope(sin6)) != 0) {
		SCTP_FREE_SONAME(sin6);
		return (error);
	}
#else
	if (IN6_IS_SCOPE_LINKLOCAL(&sin6->sin6_addr))
		
		in6_recoverscope(sin6, &sin6->sin6_addr, NULL);
	else
		sin6->sin6_scope_id = 0;	
#endif 
#endif 
#if defined(__FreeBSD__) || defined(__APPLE__) || defined(__Windows__)
	(*addr) = (struct sockaddr *)sin6;
#endif
	return (0);
}

static int
#if defined(__FreeBSD__) || defined(__APPLE__) || defined(__Windows__)
sctp6_peeraddr(struct socket *so, struct sockaddr **addr)
{
	struct sockaddr_in6 *sin6;
#elif defined(__Panda__)
sctp6_peeraddr(struct socket *so, struct sockaddr *addr)
{
	struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)addr;
#else
sctp6_peeraddr(struct socket *so, struct mbuf *nam)
{
	struct sockaddr_in6 *sin6 = mtod(nam, struct sockaddr_in6 *);
#endif
	int fnd;
	struct sockaddr_in6 *sin_a6;
	struct sctp_inpcb *inp;
	struct sctp_tcb *stcb;
	struct sctp_nets *net;
#ifdef SCTP_KAME
	int error;
#endif

	
#if defined(__FreeBSD__) || defined(__APPLE__) || defined(__Windows__)
	SCTP_MALLOC_SONAME(sin6, struct sockaddr_in6 *, sizeof *sin6);
	if (sin6 == NULL)
		return (ENOMEM);
#elif defined(__Panda__)
	memset(sin6, 0, sizeof(*sin6));
#else
	SCTP_BUF_LEN(nam) = sizeof(*sin6);
	memset(sin6, 0, sizeof(*sin6));
#endif
	sin6->sin6_family = AF_INET6;
#ifdef HAVE_SIN6_LEN
	sin6->sin6_len = sizeof(*sin6);
#endif

	inp = (struct sctp_inpcb *)so->so_pcb;
	if ((inp == NULL) ||
	    ((inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED) == 0)) {
		
#if defined(__FreeBSD__) || defined(__APPLE__) || defined(__Windows__)
		SCTP_FREE_SONAME(sin6);
#endif
		SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP6_USRREQ, ENOTCONN);
		return (ENOTCONN);
	}
	SCTP_INP_RLOCK(inp);
	stcb = LIST_FIRST(&inp->sctp_asoc_list);
	if (stcb) {
		SCTP_TCB_LOCK(stcb);
	}
	SCTP_INP_RUNLOCK(inp);
	if (stcb == NULL) {
#if defined(__FreeBSD__) || defined(__APPLE__) || defined(__Windows__)
		SCTP_FREE_SONAME(sin6);
#endif
		SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP6_USRREQ, ECONNRESET);
		return (ECONNRESET);
	}
	fnd = 0;
	TAILQ_FOREACH(net, &stcb->asoc.nets, sctp_next) {
		sin_a6 = (struct sockaddr_in6 *)&net->ro._l_addr;
		if (sin_a6->sin6_family == AF_INET6) {
			fnd = 1;
			sin6->sin6_port = stcb->rport;
			sin6->sin6_addr = sin_a6->sin6_addr;
			break;
		}
	}
	SCTP_TCB_UNLOCK(stcb);
	if (!fnd) {
		
#if defined(__FreeBSD__) || defined(__APPLE__) || defined(__Windows__)
		SCTP_FREE_SONAME(sin6);
#endif
		SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP6_USRREQ, ENOENT);
		return (ENOENT);
	}
#ifdef SCTP_EMBEDDED_V6_SCOPE
#ifdef SCTP_KAME
	if ((error = sa6_recoverscope(sin6)) != 0)
		return (error);
#else
	in6_recoverscope(sin6, &sin6->sin6_addr, NULL);
#endif 
#endif 
#if defined(__FreeBSD__) || defined(__APPLE__) || defined(__Windows__)
	*addr = (struct sockaddr *)sin6;
#endif
	return (0);
}

#if defined(__FreeBSD__) || defined(__APPLE__) || defined(__Windows__)
static int
sctp6_in6getaddr(struct socket *so, struct sockaddr **nam)
{
#ifdef INET
	struct sockaddr *addr;
#endif
#elif defined(__Panda__)
int
sctp6_in6getaddr(struct socket *so, struct sockaddr *nam, uint32_t *namelen)
{
	struct sockaddr *addr = nam;
#elif defined(__Userspace__)
int
sctp6_in6getaddr(struct socket *so, struct mbuf *nam)
{
#ifdef INET
	struct sockaddr *addr = mtod(nam, struct sockaddr *);
#endif
#else
static int
sctp6_in6getaddr(struct socket *so, struct mbuf *nam)
{
#ifdef INET
	struct sockaddr *addr = mtod(nam, struct sockaddr *);
#endif
#endif
	struct in6pcb *inp6 = sotoin6pcb(so);
	int error;

	if (inp6 == NULL) {
		SCTP_LTRACE_ERR_RET(NULL, NULL, NULL, SCTP_FROM_SCTP6_USRREQ, EINVAL);
		return (EINVAL);
	}

	
	error = sctp6_getaddr(so, nam);
#ifdef INET
	if (error) {
		
		error = sctp_ingetaddr(so, nam);
		if (error) {
			return (error);
		}
#if defined(__FreeBSD__) || defined(__APPLE__) || defined(__Windows__)
		addr = *nam;
#endif
		
		if (SCTP_IPV6_V6ONLY(inp6)) {
			struct sockaddr_in6 sin6;

			in6_sin_2_v4mapsin6((struct sockaddr_in *)addr, &sin6);
			memcpy(addr, &sin6, sizeof(struct sockaddr_in6));
		}
	}
#endif
#if defined(__Panda__)
	*namelen = nam->sa_len;
#endif
	return (error);
}


#if defined(__FreeBSD__) || defined(__APPLE__) || defined(__Windows__)
static int
sctp6_getpeeraddr(struct socket *so, struct sockaddr **nam)
{
#ifdef INET
	struct sockaddr *addr;
#endif
#elif defined(__Panda__)
int
sctp6_getpeeraddr(struct socket *so, struct sockaddr *nam, uint32_t *namelen)
{
	struct sockaddr *addr = (struct sockaddr *)nam;
#elif defined(__Userspace__)
int
sctp6_getpeeraddr(struct socket *so, struct mbuf *nam)
{
#ifdef INET
	struct sockaddr *addr = mtod(nam, struct sockaddr *);
#endif
#else
static
int
sctp6_getpeeraddr(struct socket *so, struct mbuf *nam)
{
#ifdef INET
	struct sockaddr *addr = mtod(nam, struct sockaddr *);
#endif

#endif
	struct in6pcb *inp6 = sotoin6pcb(so);
	int error;

	if (inp6 == NULL) {
		SCTP_LTRACE_ERR_RET(NULL, NULL, NULL, SCTP_FROM_SCTP6_USRREQ, EINVAL);
		return (EINVAL);
	}

	
	error = sctp6_peeraddr(so, nam);
#ifdef INET
	if (error) {
		
		error = sctp_peeraddr(so, nam);
		if (error) {
			return (error);
		}
#if defined(__FreeBSD__) || defined(__APPLE__) || defined(__Windows__)
		addr = *nam;
#endif
		
		if (SCTP_IPV6_V6ONLY(inp6)) {
			struct sockaddr_in6 sin6;

			in6_sin_2_v4mapsin6((struct sockaddr_in *)addr, &sin6);
			memcpy(addr, &sin6, sizeof(struct sockaddr_in6));
		}
	}
#endif
#if defined(__Panda__)
	*namelen = nam->sa_len;
#endif
	return (error);
}

#if defined(__FreeBSD__) || defined(__APPLE__) || defined(__Windows__)
struct pr_usrreqs sctp6_usrreqs = {
#if __FreeBSD_version >= 600000
	.pru_abort = sctp6_abort,
	.pru_accept = sctp_accept,
	.pru_attach = sctp6_attach,
	.pru_bind = sctp6_bind,
	.pru_connect = sctp6_connect,
	.pru_control = in6_control,
#if __FreeBSD_version >= 690000
	.pru_close = sctp6_close,
	.pru_detach = sctp6_close,
	.pru_sopoll = sopoll_generic,
	.pru_flush = sctp_flush,
#else
	.pru_detach = sctp6_detach,
	.pru_sopoll = sopoll,
#endif
	.pru_disconnect = sctp6_disconnect,
	.pru_listen = sctp_listen,
	.pru_peeraddr = sctp6_getpeeraddr,
	.pru_send = sctp6_send,
	.pru_shutdown = sctp_shutdown,
	.pru_sockaddr = sctp6_in6getaddr,
	.pru_sosend = sctp_sosend,
	.pru_soreceive = sctp_soreceive
#else
	sctp6_abort,
	sctp_accept,
	sctp6_attach,
	sctp6_bind,
	sctp6_connect,
	pru_connect2_notsupp,
#if defined(__Windows__)
	NULL,
	NULL,
#else
	in6_control,
	sctp6_detach,
#endif
	sctp6_disconnect,
	sctp_listen,
	sctp6_getpeeraddr,
	NULL,
	pru_rcvoob_notsupp,
#if defined(__Windows__)
	NULL,
#else
	sctp6_send,
#endif
	pru_sense_null,
	sctp_shutdown,
#if defined(__Windows__)
	sctp_flush,
#endif
	sctp6_in6getaddr,
	sctp_sosend,
	sctp_soreceive,
#if !defined(__Windows__)
 	sopoll
#else
	sopoll_generic,
	NULL,
	sctp6_close
#endif
#endif
};

#elif !defined(__Panda__) && !defined(__Userspace__)
int
sctp6_usrreq(so, req, m, nam, control, p)
	struct socket *so;
	int req;
	struct mbuf *m, *nam, *control;
	struct proc *p;
{
	int s;
	int error = 0;
	int family;
	uint32_t vrf_id;
	family = so->so_proto->pr_domain->dom_family;

	if (req == PRU_CONTROL) {
		switch (family) {
		case PF_INET:
			error = in_control(so, (long)m, (caddr_t)nam,
			    (struct ifnet *)control
			    );
#ifdef INET6
		case PF_INET6:
			error = in6_control(so, (long)m, (caddr_t)nam,
			    (struct ifnet *)control, p);
#endif
		default:
			SCTP_LTRACE_ERR_RET(NULL, NULL, NULL, SCTP_FROM_SCTP6_USRREQ, EAFNOSUPPORT);
			error = EAFNOSUPPORT;
		}
		return (error);
	}
	switch (req) {
	case PRU_ATTACH:
		error = sctp6_attach(so, family, p);
		break;
	case PRU_DETACH:
		error = sctp6_detach(so);
		break;
	case PRU_BIND:
		if (nam == NULL) {
			SCTP_LTRACE_ERR_RET(NULL, NULL, NULL, SCTP_FROM_SCTP6_USRREQ, EINVAL);
			return (EINVAL);
		}
		error = sctp6_bind(so, nam, p);
		break;
	case PRU_LISTEN:
		error = sctp_listen(so, p);
		break;
	case PRU_CONNECT:
		if (nam == NULL) {
			SCTP_LTRACE_ERR_RET(NULL, NULL, NULL, SCTP_FROM_SCTP6_USRREQ, EINVAL);
			return (EINVAL);
		}
		error = sctp6_connect(so, nam, p);
		break;
	case PRU_DISCONNECT:
		error = sctp6_disconnect(so);
		break;
	case PRU_ACCEPT:
		if (nam == NULL) {
			SCTP_LTRACE_ERR_RET(NULL, NULL, NULL, SCTP_FROM_SCTP6_USRREQ, EINVAL);
			return (EINVAL);
		}
		error = sctp_accept(so, nam);
		break;
	case PRU_SHUTDOWN:
		error = sctp_shutdown(so);
		break;

	case PRU_RCVD:
		




		error = sctp_usr_recvd(so, (int)((long)nam));
		break;

	case PRU_SEND:
		
		error = sctp6_send(so, 0, m, nam, control, p);
		break;
	case PRU_ABORT:
		error = sctp6_abort(so);
		break;

	case PRU_SENSE:
		error = 0;
		break;
	case PRU_RCVOOB:
		SCTP_LTRACE_ERR_RET(NULL, NULL, NULL, SCTP_FROM_SCTP6_USRREQ, EAFNOSUPPORT);
		error = EAFNOSUPPORT;
		break;
	case PRU_SENDOOB:
		SCTP_LTRACE_ERR_RET(NULL, NULL, NULL, SCTP_FROM_SCTP6_USRREQ, EAFNOSUPPORT);
		error = EAFNOSUPPORT;
		break;
	case PRU_PEERADDR:
		error = sctp6_getpeeraddr(so, nam);
		break;
	case PRU_SOCKADDR:
		error = sctp6_in6getaddr(so, nam);
		break;
	case PRU_SLOWTIMO:
		error = 0;
		break;
	default:
		break;
	}
	return (error);
}
#endif
#endif
