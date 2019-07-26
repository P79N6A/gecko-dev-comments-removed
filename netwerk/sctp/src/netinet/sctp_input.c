































#ifdef __FreeBSD__
#include <sys/cdefs.h>
__FBSDID("$FreeBSD: head/sys/netinet/sctp_input.c 246595 2013-02-09 17:26:14Z tuexen $");
#endif

#include <netinet/sctp_os.h>
#include <netinet/sctp_var.h>
#include <netinet/sctp_sysctl.h>
#include <netinet/sctp_pcb.h>
#include <netinet/sctp_header.h>
#include <netinet/sctputil.h>
#include <netinet/sctp_output.h>
#include <netinet/sctp_input.h>
#include <netinet/sctp_auth.h>
#include <netinet/sctp_indata.h>
#include <netinet/sctp_asconf.h>
#include <netinet/sctp_bsd_addr.h>
#include <netinet/sctp_timer.h>
#include <netinet/sctp_crc32.h>
#if !defined(__Userspace_os_Windows)
#include <netinet/udp.h>
#endif
#if defined(__FreeBSD__)
#include <sys/smp.h>
#endif

#if defined(__APPLE__)
#define APPLE_FILE_NO 2
#endif


static void
sctp_stop_all_cookie_timers(struct sctp_tcb *stcb)
{
	struct sctp_nets *net;

	




	SCTP_TCB_LOCK_ASSERT(stcb);
	TAILQ_FOREACH(net, &stcb->asoc.nets, sctp_next) {
		if (net->rxt_timer.type == SCTP_TIMER_TYPE_COOKIE) {
			sctp_timer_stop(SCTP_TIMER_TYPE_COOKIE,
					stcb->sctp_ep,
					stcb,
					net, SCTP_FROM_SCTP_INPUT+SCTP_LOC_1);
		} else if (net->rxt_timer.type == SCTP_TIMER_TYPE_INIT) {
			sctp_timer_stop(SCTP_TIMER_TYPE_INIT,
					stcb->sctp_ep,
					stcb,
					net, SCTP_FROM_SCTP_INPUT+SCTP_LOC_2);
		}
	}
}


static void
sctp_handle_init(struct mbuf *m, int iphlen, int offset,
                 struct sockaddr *src, struct sockaddr *dst, struct sctphdr *sh,
                 struct sctp_init_chunk *cp, struct sctp_inpcb *inp,
                 struct sctp_tcb *stcb, int *abort_no_unlock,
#if defined(__FreeBSD__)
                 uint8_t use_mflowid, uint32_t mflowid,
#endif
                 uint32_t vrf_id, uint16_t port)
{
	struct sctp_init *init;
	struct mbuf *op_err;

	SCTPDBG(SCTP_DEBUG_INPUT2, "sctp_handle_init: handling INIT tcb:%p\n",
		(void *)stcb);
	if (stcb == NULL) {
		SCTP_INP_RLOCK(inp);
	}
	
	if (ntohs(cp->ch.chunk_length) < sizeof(struct sctp_init_chunk)) {
		op_err = sctp_generate_invmanparam(SCTP_CAUSE_INVALID_PARAM);
		sctp_abort_association(inp, stcb, m, iphlen, src, dst, sh, op_err,
#if defined(__FreeBSD__)
		                       use_mflowid, mflowid,
#endif
				       vrf_id, port);
		if (stcb)
			*abort_no_unlock = 1;
		goto outnow;
	}
	
	init = &cp->init;
	if (init->initiate_tag == 0) {
		
		op_err = sctp_generate_invmanparam(SCTP_CAUSE_INVALID_PARAM);
		sctp_abort_association(inp, stcb, m, iphlen, src, dst, sh, op_err,
#if defined(__FreeBSD__)
		                       use_mflowid, mflowid,
#endif
				       vrf_id, port);
		if (stcb)
			*abort_no_unlock = 1;
		goto outnow;
	}
	if (ntohl(init->a_rwnd) < SCTP_MIN_RWND) {
		
		op_err = sctp_generate_invmanparam(SCTP_CAUSE_INVALID_PARAM);
		sctp_abort_association(inp, stcb, m, iphlen, src, dst, sh, op_err,
#if defined(__FreeBSD__)
		                       use_mflowid, mflowid,
#endif
				       vrf_id, port);
		if (stcb)
			*abort_no_unlock = 1;
		goto outnow;
	}
	if (init->num_inbound_streams == 0) {
		
		op_err = sctp_generate_invmanparam(SCTP_CAUSE_INVALID_PARAM);
		sctp_abort_association(inp, stcb, m, iphlen, src, dst, sh, op_err,
#if defined(__FreeBSD__)
		                       use_mflowid, mflowid,
#endif
				       vrf_id, port);
		if (stcb)
			*abort_no_unlock = 1;
		goto outnow;
	}
	if (init->num_outbound_streams == 0) {
		
		op_err = sctp_generate_invmanparam(SCTP_CAUSE_INVALID_PARAM);
		sctp_abort_association(inp, stcb, m, iphlen, src, dst, sh, op_err,
#if defined(__FreeBSD__)
		                       use_mflowid, mflowid,
#endif
				       vrf_id, port);
		if (stcb)
			*abort_no_unlock = 1;
		goto outnow;
	}
	if (sctp_validate_init_auth_params(m, offset + sizeof(*cp),
					   offset + ntohs(cp->ch.chunk_length))) {
		
		sctp_abort_association(inp, stcb, m, iphlen, src, dst, sh, NULL,
#if defined(__FreeBSD__)
		                       use_mflowid, mflowid,
#endif
		                       vrf_id, port);
		if (stcb)
			*abort_no_unlock = 1;
		goto outnow;
	}
	
	if ((stcb == NULL) &&
	    ((inp->sctp_flags & SCTP_PCB_FLAGS_SOCKET_GONE) ||
	     (inp->sctp_flags & SCTP_PCB_FLAGS_SOCKET_ALLGONE) ||
	     (inp->sctp_socket == NULL) ||
	     (inp->sctp_socket->so_qlimit == 0))) {
		











		if (SCTP_BASE_SYSCTL(sctp_blackhole) == 0) {
			sctp_send_abort(m, iphlen, src, dst, sh, 0, NULL,
#if defined(__FreeBSD__)
			                use_mflowid, mflowid,
#endif
			                vrf_id, port);
		}
		goto outnow;
	}
	if ((stcb != NULL) &&
	    (SCTP_GET_STATE(&stcb->asoc) == SCTP_STATE_SHUTDOWN_ACK_SENT)) {
		SCTPDBG(SCTP_DEBUG_INPUT3, "sctp_handle_init: sending SHUTDOWN-ACK\n");
		sctp_send_shutdown_ack(stcb, NULL);
		sctp_chunk_output(inp, stcb, SCTP_OUTPUT_FROM_CONTROL_PROC, SCTP_SO_NOT_LOCKED);
	} else {
		SCTPDBG(SCTP_DEBUG_INPUT3, "sctp_handle_init: sending INIT-ACK\n");
		sctp_send_initiate_ack(inp, stcb, m, iphlen, offset, src, dst,
		                       sh, cp,
#if defined(__FreeBSD__)
		                       use_mflowid, mflowid,
#endif
		                       vrf_id, port,
		                       ((stcb == NULL) ? SCTP_HOLDS_LOCK : SCTP_NOT_LOCKED));
	}
 outnow:
	if (stcb == NULL) {
		SCTP_INP_RUNLOCK(inp);
	}
}





int
sctp_is_there_unsent_data(struct sctp_tcb *stcb, int so_locked
#if !defined(__APPLE__) && !defined(SCTP_SO_LOCK_TESTING)
	SCTP_UNUSED
#endif
)
{
	int unsent_data = 0;
	unsigned int i;
	struct sctp_stream_queue_pending *sp;
	struct sctp_association *asoc;

	




	asoc = &stcb->asoc;
	SCTP_TCB_SEND_LOCK(stcb);
	if (!stcb->asoc.ss_functions.sctp_ss_is_empty(stcb, asoc)) {
		
		for (i = 0; i < stcb->asoc.streamoutcnt; i++) {
			
			sp = TAILQ_FIRST(&stcb->asoc.strmout[i].outqueue);
			if (sp == NULL) {
				continue;
			}
			if ((sp->msg_is_complete) &&
			    (sp->length == 0)  &&
			    (sp->sender_all_done)) {
				



				if (sp->put_last_out == 0) {
					SCTP_PRINTF("Gak, put out entire msg with NO end!-1\n");
					SCTP_PRINTF("sender_done:%d len:%d msg_comp:%d put_last_out:%d\n",
					            sp->sender_all_done,
					            sp->length,
					            sp->msg_is_complete,
					            sp->put_last_out);
				}
				atomic_subtract_int(&stcb->asoc.stream_queue_cnt, 1);
				TAILQ_REMOVE(&stcb->asoc.strmout[i].outqueue, sp, next);
				if (sp->net) {
					sctp_free_remote_addr(sp->net);
					sp->net = NULL;
				}
				if (sp->data) {
					sctp_m_freem(sp->data);
					sp->data = NULL;
				}
				sctp_free_a_strmoq(stcb, sp, so_locked);
			} else {
				unsent_data++;
				break;
			}
		}
	}
	SCTP_TCB_SEND_UNLOCK(stcb);
	return (unsent_data);
}

static int
sctp_process_init(struct sctp_init_chunk *cp, struct sctp_tcb *stcb)
{
	struct sctp_init *init;
	struct sctp_association *asoc;
	struct sctp_nets *lnet;
	unsigned int i;

	init = &cp->init;
	asoc = &stcb->asoc;
	
	asoc->peer_vtag = ntohl(init->initiate_tag);
	asoc->peers_rwnd = ntohl(init->a_rwnd);
	
	asoc->highest_tsn_inside_map = asoc->asconf_seq_in = ntohl(init->initial_tsn) - 1;

	if (!TAILQ_EMPTY(&asoc->nets)) {
		
		TAILQ_FOREACH(lnet, &asoc->nets, sctp_next) {
			lnet->ssthresh = asoc->peers_rwnd;
			if (SCTP_BASE_SYSCTL(sctp_logging_level) & (SCTP_CWND_MONITOR_ENABLE|SCTP_CWND_LOGGING_ENABLE)) {
				sctp_log_cwnd(stcb, lnet, 0, SCTP_CWND_INITIALIZATION);
			}

		}
	}
	SCTP_TCB_SEND_LOCK(stcb);
	if (asoc->pre_open_streams > ntohs(init->num_inbound_streams)) {
		unsigned int newcnt;
		struct sctp_stream_out *outs;
		struct sctp_stream_queue_pending *sp, *nsp;
		struct sctp_tmit_chunk *chk, *nchk;

		
		newcnt = ntohs(init->num_inbound_streams);
		TAILQ_FOREACH_SAFE(chk, &asoc->send_queue, sctp_next, nchk) {
			if (chk->rec.data.stream_number >= newcnt) {
				TAILQ_REMOVE(&asoc->send_queue, chk, sctp_next);
				asoc->send_queue_cnt--;
				if (asoc->strmout[chk->rec.data.stream_number].chunks_on_queues > 0) {
					asoc->strmout[chk->rec.data.stream_number].chunks_on_queues--;
#ifdef INVARIANTS
				} else {
					panic("No chunks on the queues for sid %u.", chk->rec.data.stream_number);
#endif
				}
				if (chk->data != NULL) {
					sctp_free_bufspace(stcb, asoc, chk, 1);
					sctp_ulp_notify(SCTP_NOTIFY_UNSENT_DG_FAIL, stcb,
					                0, chk, SCTP_SO_NOT_LOCKED);
					if (chk->data) {
						sctp_m_freem(chk->data);
						chk->data = NULL;
					}
				}
				sctp_free_a_chunk(stcb, chk, SCTP_SO_NOT_LOCKED);
				
			}
		}
		if (asoc->strmout) {
			for (i = newcnt; i < asoc->pre_open_streams; i++) {
				outs = &asoc->strmout[i];
				TAILQ_FOREACH_SAFE(sp, &outs->outqueue, next, nsp) {
					TAILQ_REMOVE(&outs->outqueue, sp, next);
					asoc->stream_queue_cnt--;
					sctp_ulp_notify(SCTP_NOTIFY_SPECIAL_SP_FAIL,
					    stcb, 0, sp, SCTP_SO_NOT_LOCKED);
					if (sp->data) {
						sctp_m_freem(sp->data);
						sp->data = NULL;
					}
					if (sp->net) {
						sctp_free_remote_addr(sp->net);
						sp->net = NULL;
					}
					
					sctp_free_a_strmoq(stcb, sp, SCTP_SO_NOT_LOCKED);
					
				}
			}
		}
		
		asoc->pre_open_streams = newcnt;
	}
	SCTP_TCB_SEND_UNLOCK(stcb);
	asoc->strm_realoutsize = asoc->streamoutcnt = asoc->pre_open_streams;

	
	asoc->highest_tsn_inside_nr_map = asoc->highest_tsn_inside_map;
	if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_MAP_LOGGING_ENABLE) {
		sctp_log_map(0, 5, asoc->highest_tsn_inside_map, SCTP_MAP_SLIDE_RESULT);
	}
	
	asoc->str_reset_seq_in = asoc->asconf_seq_in + 1;

	asoc->mapping_array_base_tsn = ntohl(init->initial_tsn);
	asoc->tsn_last_delivered = asoc->cumulative_tsn = asoc->asconf_seq_in;

	asoc->advanced_peer_ack_point = asoc->last_acked_seq;
	

	if (asoc->strmin != NULL) {
		
		struct sctp_queued_to_read *ctl, *nctl;

		for (i = 0; i < asoc->streamincnt; i++) {
			TAILQ_FOREACH_SAFE(ctl, &asoc->strmin[i].inqueue, next, nctl) {
				TAILQ_REMOVE(&asoc->strmin[i].inqueue, ctl, next);
				sctp_free_remote_addr(ctl->whoFrom);
				ctl->whoFrom = NULL;
				sctp_m_freem(ctl->data);
				ctl->data = NULL;
				sctp_free_a_readq(stcb, ctl);
			}
		}
		SCTP_FREE(asoc->strmin, SCTP_M_STRMI);
	}
	asoc->streamincnt = ntohs(init->num_outbound_streams);
	if (asoc->streamincnt > MAX_SCTP_STREAMS) {
		asoc->streamincnt = MAX_SCTP_STREAMS;
	}
	SCTP_MALLOC(asoc->strmin, struct sctp_stream_in *, asoc->streamincnt *
		    sizeof(struct sctp_stream_in), SCTP_M_STRMI);
	if (asoc->strmin == NULL) {
		
		SCTPDBG(SCTP_DEBUG_INPUT2, "process_init: couldn't get memory for the streams!\n");
		return (-1);
	}
	for (i = 0; i < asoc->streamincnt; i++) {
		asoc->strmin[i].stream_no = i;
		asoc->strmin[i].last_sequence_delivered = 0xffff;
		




		TAILQ_INIT(&asoc->strmin[i].inqueue);
		asoc->strmin[i].delivery_started = 0;
	}
	








	
	return (0);
}




static int
sctp_process_init_ack(struct mbuf *m, int iphlen, int offset,
                      struct sockaddr *src, struct sockaddr *dst, struct sctphdr *sh,
                      struct sctp_init_ack_chunk *cp, struct sctp_tcb *stcb,
                      struct sctp_nets *net, int *abort_no_unlock,
#if defined(__FreeBSD__)
		      uint8_t use_mflowid, uint32_t mflowid,
#endif
                      uint32_t vrf_id)
{
	struct sctp_association *asoc;
	struct mbuf *op_err;
	int retval, abort_flag;
	uint32_t initack_limit;
	int nat_friendly = 0;

	
	abort_flag = 0;
	op_err = NULL;

	op_err = sctp_arethere_unrecognized_parameters(m,
						       (offset + sizeof(struct sctp_init_chunk)),
						       &abort_flag, (struct sctp_chunkhdr *)cp, &nat_friendly);
	if (abort_flag) {
		
		sctp_abort_an_association(stcb->sctp_ep, stcb, op_err, SCTP_SO_NOT_LOCKED);
		*abort_no_unlock = 1;
		return (-1);
	}
	asoc = &stcb->asoc;
	asoc->peer_supports_nat = (uint8_t)nat_friendly;
	
	retval = sctp_process_init((struct sctp_init_chunk *)cp, stcb);
	if (retval < 0) {
		return (retval);
	}
	initack_limit = offset + ntohs(cp->ch.chunk_length);
	
	if ((retval = sctp_load_addresses_from_init(stcb, m,
	    (offset + sizeof(struct sctp_init_chunk)), initack_limit,
	    src, dst, NULL))) {
		
		SCTPDBG(SCTP_DEBUG_INPUT1,
			"Load addresses from INIT causes an abort %d\n",
			retval);
		sctp_abort_association(stcb->sctp_ep, stcb, m, iphlen,
		                       src, dst, sh, NULL,
#if defined(__FreeBSD__)
		                       use_mflowid, mflowid,
#endif
		                       vrf_id, net->port);
		*abort_no_unlock = 1;
		return (-1);
	}
	
	if (asoc->peer_supports_asconf == 0) {
		struct sctp_asconf_addr *param, *nparam;

		TAILQ_FOREACH_SAFE(param, &asoc->asconf_queue, next, nparam) {
			TAILQ_REMOVE(&asoc->asconf_queue, param, next);
			SCTP_FREE(param, SCTP_M_ASC_ADDR);
		}
	}

	stcb->asoc.peer_hmac_id = sctp_negotiate_hmacid(stcb->asoc.peer_hmacs,
	    stcb->asoc.local_hmacs);
	if (op_err) {
		sctp_queue_op_err(stcb, op_err);
		
		op_err = NULL;
	}
	
	if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_THRESHOLD_LOGGING) {
		sctp_misc_ints(SCTP_THRESHOLD_CLEAR,
			       stcb->asoc.overall_error_count,
			       0,
			       SCTP_FROM_SCTP_INPUT,
			       __LINE__);
	}
	stcb->asoc.overall_error_count = 0;
	net->error_count = 0;

	





	sctp_timer_stop(SCTP_TIMER_TYPE_INIT, stcb->sctp_ep, stcb,
	    asoc->primary_destination, SCTP_FROM_SCTP_INPUT+SCTP_LOC_4);

	
	net->RTO = sctp_calculate_rto(stcb, asoc, net, &asoc->time_entered, sctp_align_safe_nocopy,
				      SCTP_RTT_FROM_NON_DATA);

	retval = sctp_send_cookie_echo(m, offset, stcb, net);
	if (retval < 0) {
		




		if (retval == -3) {
			
			op_err =
			    sctp_generate_invmanparam(SCTP_CAUSE_MISSING_PARAM);
			if (op_err) {
				



				struct sctp_inv_mandatory_param *mp;

				SCTP_BUF_LEN(op_err) =
				    sizeof(struct sctp_inv_mandatory_param);
				mp = mtod(op_err,
				    struct sctp_inv_mandatory_param *);
				
				mp->length =
				    htons(sizeof(struct sctp_inv_mandatory_param) - 2);
				mp->num_param = htonl(1);
				mp->param = htons(SCTP_STATE_COOKIE);
				mp->resv = 0;
			}
			sctp_abort_association(stcb->sctp_ep, stcb, m, iphlen,
			                       src, dst, sh, op_err,
#if defined(__FreeBSD__)
			                       use_mflowid, mflowid,
#endif
			                       vrf_id, net->port);
			*abort_no_unlock = 1;
		}
		return (retval);
	}

	return (0);
}

static void
sctp_handle_heartbeat_ack(struct sctp_heartbeat_chunk *cp,
    struct sctp_tcb *stcb, struct sctp_nets *net)
{
	struct sockaddr_storage store;
	struct sctp_nets *r_net, *f_net;
	struct timeval tv;
	int req_prim = 0;
	uint16_t old_error_counter;
#ifdef INET
	struct sockaddr_in *sin;
#endif
#ifdef INET6
	struct sockaddr_in6 *sin6;
#endif
#if defined(__Userspace__)
	struct sockaddr_conn *sconn;
#endif

	if (ntohs(cp->ch.chunk_length) != sizeof(struct sctp_heartbeat_chunk)) {
		
		return;
	}

	memset(&store, 0, sizeof(store));
	switch (cp->heartbeat.hb_info.addr_family) {
#ifdef INET
	case AF_INET:
		if (cp->heartbeat.hb_info.addr_len == sizeof(struct sockaddr_in)) {
			sin = (struct sockaddr_in *)&store;
			sin->sin_family = cp->heartbeat.hb_info.addr_family;
#ifdef HAVE_SIN_LEN
			sin->sin_len = cp->heartbeat.hb_info.addr_len;
#endif
			sin->sin_port = stcb->rport;
			memcpy(&sin->sin_addr, cp->heartbeat.hb_info.address,
			       sizeof(sin->sin_addr));
		} else {
			return;
		}
		break;
#endif
#ifdef INET6
	case AF_INET6:
		if (cp->heartbeat.hb_info.addr_len == sizeof(struct sockaddr_in6)) {
			sin6 = (struct sockaddr_in6 *)&store;
			sin6->sin6_family = cp->heartbeat.hb_info.addr_family;
#ifdef HAVE_SIN6_LEN
			sin6->sin6_len = cp->heartbeat.hb_info.addr_len;
#endif
			sin6->sin6_port = stcb->rport;
			memcpy(&sin6->sin6_addr, cp->heartbeat.hb_info.address,
			       sizeof(sin6->sin6_addr));
		} else {
			return;
		}
		break;
#endif
#if defined(__Userspace__)
	case AF_CONN:
		if (cp->heartbeat.hb_info.addr_len == sizeof(struct sockaddr_conn)) {
			sconn = (struct sockaddr_conn *)&store;
			sconn->sconn_family = cp->heartbeat.hb_info.addr_family;
#ifdef HAVE_SCONN_LEN
			sconn->sconn_len = cp->heartbeat.hb_info.addr_len;
#endif
			sconn->sconn_port = stcb->rport;
			memcpy(&sconn->sconn_addr, cp->heartbeat.hb_info.address,
			       sizeof(sconn->sconn_addr));
		} else {
			return;
		}
		break;
#endif
	default:
		return;
	}
	r_net = sctp_findnet(stcb, (struct sockaddr *)&store);
	if (r_net == NULL) {
		SCTPDBG(SCTP_DEBUG_INPUT1, "Huh? I can't find the address I sent it to, discard\n");
		return;
	}
	if ((r_net && (r_net->dest_state & SCTP_ADDR_UNCONFIRMED)) &&
	    (r_net->heartbeat_random1 == cp->heartbeat.hb_info.random_value1) &&
	    (r_net->heartbeat_random2 == cp->heartbeat.hb_info.random_value2)) {
		



		r_net->dest_state &= ~SCTP_ADDR_UNCONFIRMED;
		if (r_net->dest_state & SCTP_ADDR_REQ_PRIMARY) {
			stcb->asoc.primary_destination = r_net;
			r_net->dest_state &= ~SCTP_ADDR_REQ_PRIMARY;
			f_net = TAILQ_FIRST(&stcb->asoc.nets);
			if (f_net != r_net) {
				




				TAILQ_REMOVE(&stcb->asoc.nets, r_net, sctp_next);
				TAILQ_INSERT_HEAD(&stcb->asoc.nets, r_net, sctp_next);
			}
			req_prim = 1;
		}
		sctp_ulp_notify(SCTP_NOTIFY_INTERFACE_CONFIRMED,
		    stcb, 0, (void *)r_net, SCTP_SO_NOT_LOCKED);
		sctp_timer_stop(SCTP_TIMER_TYPE_HEARTBEAT, stcb->sctp_ep, stcb, r_net, SCTP_FROM_SCTP_INPUT + SCTP_LOC_3);
		sctp_timer_start(SCTP_TIMER_TYPE_HEARTBEAT, stcb->sctp_ep, stcb, r_net);
	}
	old_error_counter = r_net->error_count;
	r_net->error_count = 0;
	r_net->hb_responded = 1;
	tv.tv_sec = cp->heartbeat.hb_info.time_value_1;
	tv.tv_usec = cp->heartbeat.hb_info.time_value_2;
	
	r_net->RTO = sctp_calculate_rto(stcb, &stcb->asoc, r_net, &tv, sctp_align_safe_nocopy,
					SCTP_RTT_FROM_NON_DATA);
	if (!(r_net->dest_state & SCTP_ADDR_REACHABLE)) {
		r_net->dest_state |= SCTP_ADDR_REACHABLE;
		sctp_ulp_notify(SCTP_NOTIFY_INTERFACE_UP, stcb,
				0, (void *)r_net, SCTP_SO_NOT_LOCKED);
	}
	if (r_net->dest_state & SCTP_ADDR_PF) {
		r_net->dest_state &= ~SCTP_ADDR_PF;
		stcb->asoc.cc_functions.sctp_cwnd_update_exit_pf(stcb, net);
	}
	if (old_error_counter > 0) {
		sctp_timer_stop(SCTP_TIMER_TYPE_HEARTBEAT, stcb->sctp_ep, stcb, r_net, SCTP_FROM_SCTP_INPUT + SCTP_LOC_3);
		sctp_timer_start(SCTP_TIMER_TYPE_HEARTBEAT, stcb->sctp_ep, stcb, r_net);
	}
	if (r_net == stcb->asoc.primary_destination) {
		if (stcb->asoc.alternate) {
			
			sctp_free_remote_addr(stcb->asoc.alternate);
			stcb->asoc.alternate = NULL;
		}
	}
	
	if (req_prim) {
		if ((sctp_is_mobility_feature_on(stcb->sctp_ep,
				       	SCTP_MOBILITY_BASE) ||
	    	    sctp_is_mobility_feature_on(stcb->sctp_ep,
			    		SCTP_MOBILITY_FASTHANDOFF)) &&
	    	    sctp_is_mobility_feature_on(stcb->sctp_ep,
					SCTP_MOBILITY_PRIM_DELETED)) {

			sctp_timer_stop(SCTP_TIMER_TYPE_PRIM_DELETED, stcb->sctp_ep, stcb, NULL, SCTP_FROM_SCTP_TIMER+SCTP_LOC_7);
			if (sctp_is_mobility_feature_on(stcb->sctp_ep,
					SCTP_MOBILITY_FASTHANDOFF)) {
				sctp_assoc_immediate_retrans(stcb,
					stcb->asoc.primary_destination);
			}
			if (sctp_is_mobility_feature_on(stcb->sctp_ep,
					SCTP_MOBILITY_BASE)) {
				sctp_move_chunks_from_net(stcb,
					stcb->asoc.deleted_primary);
			}
			sctp_delete_prim_timer(stcb->sctp_ep, stcb,
					stcb->asoc.deleted_primary);
		}
	}
}

static int
sctp_handle_nat_colliding_state(struct sctp_tcb *stcb)
{
	


	struct sctpasochead *head;

	if (SCTP_GET_STATE(&stcb->asoc) == SCTP_STATE_COOKIE_WAIT) {
		
		LIST_REMOVE(stcb, sctp_asocs);
		stcb->asoc.my_vtag = sctp_select_a_tag(stcb->sctp_ep, stcb->sctp_ep->sctp_lport, stcb->rport, 1);
		head = &SCTP_BASE_INFO(sctp_asochash)[SCTP_PCBHASH_ASOC(stcb->asoc.my_vtag, SCTP_BASE_INFO(hashasocmark))];
		
		LIST_INSERT_HEAD(head, stcb, sctp_asocs);
		sctp_send_initiate(stcb->sctp_ep, stcb, SCTP_SO_NOT_LOCKED);
		return (1);
	}
	if (SCTP_GET_STATE(&stcb->asoc) == SCTP_STATE_COOKIE_ECHOED) {
		




		
		LIST_REMOVE(stcb, sctp_asocs);
		stcb->asoc.state &= ~SCTP_STATE_COOKIE_ECHOED;
		stcb->asoc.state |= SCTP_STATE_COOKIE_WAIT;
		sctp_stop_all_cookie_timers(stcb);
		sctp_toss_old_cookies(stcb, &stcb->asoc);
		stcb->asoc.my_vtag = sctp_select_a_tag(stcb->sctp_ep, stcb->sctp_ep->sctp_lport, stcb->rport,  1);
		head = &SCTP_BASE_INFO(sctp_asochash)[SCTP_PCBHASH_ASOC(stcb->asoc.my_vtag, SCTP_BASE_INFO(hashasocmark))];
		
		LIST_INSERT_HEAD(head, stcb, sctp_asocs);
		sctp_send_initiate(stcb->sctp_ep, stcb, SCTP_SO_NOT_LOCKED);
		return (1);
	}
	return (0);
}

static int
sctp_handle_nat_missing_state(struct sctp_tcb *stcb,
			      struct sctp_nets *net)

{
  


  if (stcb->asoc.peer_supports_auth == 0) {
    SCTPDBG(SCTP_DEBUG_INPUT2, "sctp_handle_nat_missing_state: Peer does not support AUTH, cannot send an asconf\n");
    return (0);
  }
  sctp_asconf_send_nat_state_update(stcb, net);
  return (1);
}


static void
sctp_handle_abort(struct sctp_abort_chunk *abort,
    struct sctp_tcb *stcb, struct sctp_nets *net)
{
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
	struct socket *so;
#endif
	uint16_t len;
	uint16_t error;

	SCTPDBG(SCTP_DEBUG_INPUT2, "sctp_handle_abort: handling ABORT\n");
	if (stcb == NULL)
		return;

	len = ntohs(abort->ch.chunk_length);
	if (len > sizeof (struct sctp_chunkhdr)) {
		



		struct sctp_missing_nat_state *natc;

		natc = (struct sctp_missing_nat_state *)(abort + 1);
		error = ntohs(natc->cause);
		if (error == SCTP_CAUSE_NAT_COLLIDING_STATE) {
			SCTPDBG(SCTP_DEBUG_INPUT2, "Received Colliding state abort flags:%x\n",
			                           abort->ch.chunk_flags);
			if (sctp_handle_nat_colliding_state(stcb)) {
				return;
			}
		} else if (error == SCTP_CAUSE_NAT_MISSING_STATE) {
			SCTPDBG(SCTP_DEBUG_INPUT2, "Received missing state abort flags:%x\n",
			                           abort->ch.chunk_flags);
			if (sctp_handle_nat_missing_state(stcb, net)) {
				return;
			}
		}
	} else {
		error = 0;
	}
	
	sctp_timer_stop(SCTP_TIMER_TYPE_RECV, stcb->sctp_ep, stcb, net, SCTP_FROM_SCTP_INPUT+SCTP_LOC_6);
	
	sctp_abort_notification(stcb, 1, error, abort, SCTP_SO_NOT_LOCKED);
	
	SCTP_STAT_INCR_COUNTER32(sctps_aborted);
	if ((SCTP_GET_STATE(&stcb->asoc) == SCTP_STATE_OPEN) ||
	    (SCTP_GET_STATE(&stcb->asoc) == SCTP_STATE_SHUTDOWN_RECEIVED)) {
		SCTP_STAT_DECR_GAUGE32(sctps_currestab);
	}
#ifdef SCTP_ASOCLOG_OF_TSNS
	sctp_print_out_track_log(stcb);
#endif
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
	so = SCTP_INP_SO(stcb->sctp_ep);
	atomic_add_int(&stcb->asoc.refcnt, 1);
	SCTP_TCB_UNLOCK(stcb);
	SCTP_SOCKET_LOCK(so, 1);
	SCTP_TCB_LOCK(stcb);
	atomic_subtract_int(&stcb->asoc.refcnt, 1);
#endif
	stcb->asoc.state |= SCTP_STATE_WAS_ABORTED;
	(void)sctp_free_assoc(stcb->sctp_ep, stcb, SCTP_NORMAL_PROC,
			      SCTP_FROM_SCTP_INPUT+SCTP_LOC_6);
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
	SCTP_SOCKET_UNLOCK(so, 1);
#endif
	SCTPDBG(SCTP_DEBUG_INPUT2, "sctp_handle_abort: finished\n");
}

static void
sctp_start_net_timers(struct sctp_tcb *stcb)
{
	uint32_t cnt_hb_sent;
	struct sctp_nets *net;

	cnt_hb_sent = 0;
	TAILQ_FOREACH(net, &stcb->asoc.nets, sctp_next) {
		






		sctp_timer_start(SCTP_TIMER_TYPE_PATHMTURAISE, stcb->sctp_ep, stcb, net);
		sctp_timer_start(SCTP_TIMER_TYPE_HEARTBEAT, stcb->sctp_ep, stcb, net);
		if ((net->dest_state & SCTP_ADDR_UNCONFIRMED) &&
		    (cnt_hb_sent < SCTP_BASE_SYSCTL(sctp_hb_maxburst))) {
			sctp_send_hb(stcb, net, SCTP_SO_NOT_LOCKED);
			cnt_hb_sent++;
		}
	}
	if (cnt_hb_sent) {
		sctp_chunk_output(stcb->sctp_ep, stcb,
				  SCTP_OUTPUT_FROM_COOKIE_ACK,
				  SCTP_SO_NOT_LOCKED);
	}
}


static void
sctp_handle_shutdown(struct sctp_shutdown_chunk *cp,
    struct sctp_tcb *stcb, struct sctp_nets *net, int *abort_flag)
{
	struct sctp_association *asoc;
	int some_on_streamwheel;
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
	struct socket *so;
#endif

	SCTPDBG(SCTP_DEBUG_INPUT2,
		"sctp_handle_shutdown: handling SHUTDOWN\n");
	if (stcb == NULL)
		return;
	asoc = &stcb->asoc;
	if ((SCTP_GET_STATE(asoc) == SCTP_STATE_COOKIE_WAIT) ||
	    (SCTP_GET_STATE(asoc) == SCTP_STATE_COOKIE_ECHOED)) {
		return;
	}
	if (ntohs(cp->ch.chunk_length) != sizeof(struct sctp_shutdown_chunk)) {
		
		return;
	} else {
		sctp_update_acked(stcb, cp, abort_flag);
		if (*abort_flag) {
			return;
		}
	}
	if (asoc->control_pdapi) {
		


		SCTP_INP_READ_LOCK(stcb->sctp_ep);
		asoc->control_pdapi->end_added = 1;
		asoc->control_pdapi->pdapi_aborted = 1;
		asoc->control_pdapi = NULL;
		SCTP_INP_READ_UNLOCK(stcb->sctp_ep);
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
		so = SCTP_INP_SO(stcb->sctp_ep);
		atomic_add_int(&stcb->asoc.refcnt, 1);
		SCTP_TCB_UNLOCK(stcb);
		SCTP_SOCKET_LOCK(so, 1);
		SCTP_TCB_LOCK(stcb);
		atomic_subtract_int(&stcb->asoc.refcnt, 1);
		if (stcb->asoc.state & SCTP_STATE_CLOSED_SOCKET) {
			
			SCTP_SOCKET_UNLOCK(so, 1);
			return;
		}
#endif
		sctp_sorwakeup(stcb->sctp_ep, stcb->sctp_socket);
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
		SCTP_SOCKET_UNLOCK(so, 1);
#endif
	}
	
	if (stcb->sctp_socket) {
		if ((SCTP_GET_STATE(asoc) != SCTP_STATE_SHUTDOWN_RECEIVED) &&
		    (SCTP_GET_STATE(asoc) != SCTP_STATE_SHUTDOWN_ACK_SENT) &&
		    (SCTP_GET_STATE(asoc) != SCTP_STATE_SHUTDOWN_SENT)) {
			SCTP_SET_STATE(asoc, SCTP_STATE_SHUTDOWN_RECEIVED);
			SCTP_CLEAR_SUBSTATE(asoc, SCTP_STATE_SHUTDOWN_PENDING);
			
			sctp_ulp_notify(SCTP_NOTIFY_PEER_SHUTDOWN, stcb, 0, NULL, SCTP_SO_NOT_LOCKED);

			
			(void)SCTP_GETTIME_TIMEVAL(&asoc->time_entered);
		}
	}
	if (SCTP_GET_STATE(asoc) == SCTP_STATE_SHUTDOWN_SENT) {
		



		sctp_timer_stop(SCTP_TIMER_TYPE_SHUTDOWN, stcb->sctp_ep, stcb, net, SCTP_FROM_SCTP_INPUT+SCTP_LOC_8);
	}
	
	some_on_streamwheel = sctp_is_there_unsent_data(stcb, SCTP_SO_NOT_LOCKED);

	if (!TAILQ_EMPTY(&asoc->send_queue) ||
	    !TAILQ_EMPTY(&asoc->sent_queue) ||
	    some_on_streamwheel) {
		
		return;
	} else {
		
		
		
		if ((SCTP_GET_STATE(asoc) == SCTP_STATE_OPEN) ||
		    (SCTP_GET_STATE(asoc) == SCTP_STATE_SHUTDOWN_RECEIVED)) {
			SCTP_STAT_DECR_GAUGE32(sctps_currestab);
		}
		SCTP_SET_STATE(asoc, SCTP_STATE_SHUTDOWN_ACK_SENT);
		SCTP_CLEAR_SUBSTATE(asoc, SCTP_STATE_SHUTDOWN_PENDING);
		sctp_stop_timers_for_shutdown(stcb);
		sctp_send_shutdown_ack(stcb, net);
		sctp_timer_start(SCTP_TIMER_TYPE_SHUTDOWNACK, stcb->sctp_ep,
				 stcb, net);
	}
}

static void
sctp_handle_shutdown_ack(struct sctp_shutdown_ack_chunk *cp SCTP_UNUSED,
                         struct sctp_tcb *stcb,
                         struct sctp_nets *net)
{
	struct sctp_association *asoc;
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
	struct socket *so;

	so = SCTP_INP_SO(stcb->sctp_ep);
#endif
	SCTPDBG(SCTP_DEBUG_INPUT2,
		"sctp_handle_shutdown_ack: handling SHUTDOWN ACK\n");
	if (stcb == NULL)
		return;

	asoc = &stcb->asoc;
	
	if ((SCTP_GET_STATE(asoc) == SCTP_STATE_COOKIE_WAIT) ||
	    (SCTP_GET_STATE(asoc) == SCTP_STATE_COOKIE_ECHOED)) {
		
		sctp_send_shutdown_complete(stcb, net, 1);
		SCTP_TCB_UNLOCK(stcb);
		return;
	}
	if ((SCTP_GET_STATE(asoc) != SCTP_STATE_SHUTDOWN_SENT) &&
	    (SCTP_GET_STATE(asoc) != SCTP_STATE_SHUTDOWN_ACK_SENT)) {
		
		SCTP_TCB_UNLOCK(stcb);
		return;
	}
	if (asoc->control_pdapi) {
		


		SCTP_INP_READ_LOCK(stcb->sctp_ep);
		asoc->control_pdapi->end_added = 1;
		asoc->control_pdapi->pdapi_aborted = 1;
		asoc->control_pdapi = NULL;
		SCTP_INP_READ_UNLOCK(stcb->sctp_ep);
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
		atomic_add_int(&stcb->asoc.refcnt, 1);
		SCTP_TCB_UNLOCK(stcb);
		SCTP_SOCKET_LOCK(so, 1);
		SCTP_TCB_LOCK(stcb);
		atomic_subtract_int(&stcb->asoc.refcnt, 1);
		if (stcb->asoc.state & SCTP_STATE_CLOSED_SOCKET) {
			
			SCTP_SOCKET_UNLOCK(so, 1);
			return;
		}
#endif
		sctp_sorwakeup(stcb->sctp_ep, stcb->sctp_socket);
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
		SCTP_SOCKET_UNLOCK(so, 1);
#endif
	}
	
	if (!TAILQ_EMPTY(&asoc->send_queue) ||
	    !TAILQ_EMPTY(&asoc->sent_queue) ||
	    !stcb->asoc.ss_functions.sctp_ss_is_empty(stcb, asoc)) {
		sctp_report_all_outbound(stcb, 0, 0, SCTP_SO_NOT_LOCKED);
	}
	
	sctp_timer_stop(SCTP_TIMER_TYPE_SHUTDOWN, stcb->sctp_ep, stcb, net, SCTP_FROM_SCTP_INPUT+SCTP_LOC_9);
	
	sctp_send_shutdown_complete(stcb, net, 0);
	
	if (stcb->sctp_socket) {
		if ((stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_TCPTYPE) ||
		    (stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_IN_TCPPOOL)) {
			stcb->sctp_socket->so_snd.sb_cc = 0;
		}
		sctp_ulp_notify(SCTP_NOTIFY_ASSOC_DOWN, stcb, 0, NULL, SCTP_SO_NOT_LOCKED);
	}
	SCTP_STAT_INCR_COUNTER32(sctps_shutdown);
	
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
	atomic_add_int(&stcb->asoc.refcnt, 1);
	SCTP_TCB_UNLOCK(stcb);
	SCTP_SOCKET_LOCK(so, 1);
	SCTP_TCB_LOCK(stcb);
	atomic_subtract_int(&stcb->asoc.refcnt, 1);
#endif
	(void)sctp_free_assoc(stcb->sctp_ep, stcb, SCTP_NORMAL_PROC,
			      SCTP_FROM_SCTP_INPUT+SCTP_LOC_10);
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
	SCTP_SOCKET_UNLOCK(so, 1);
#endif
}






static void
sctp_process_unrecog_chunk(struct sctp_tcb *stcb, struct sctp_paramhdr *phdr,
    struct sctp_nets *net)
{
	struct sctp_chunkhdr *chk;

	chk = (struct sctp_chunkhdr *)((caddr_t)phdr + sizeof(*phdr));
	switch (chk->chunk_type) {
	case SCTP_ASCONF_ACK:
	case SCTP_ASCONF:
		sctp_asconf_cleanup(stcb, net);
		break;
	case SCTP_FORWARD_CUM_TSN:
		stcb->asoc.peer_supports_prsctp = 0;
		break;
	default:
		SCTPDBG(SCTP_DEBUG_INPUT2,
			"Peer does not support chunk type %d(%x)??\n",
			chk->chunk_type, (uint32_t) chk->chunk_type);
		break;
	}
}






static void
sctp_process_unrecog_param(struct sctp_tcb *stcb, struct sctp_paramhdr *phdr)
{
	struct sctp_paramhdr *pbad;

	pbad = phdr + 1;
	switch (ntohs(pbad->param_type)) {
		
	case SCTP_PRSCTP_SUPPORTED:
		stcb->asoc.peer_supports_prsctp = 0;
		break;
	case SCTP_SUPPORTED_CHUNK_EXT:
		break;
		
	case SCTP_HAS_NAT_SUPPORT:
	        stcb->asoc.peer_supports_nat = 0;
	        break;
	case SCTP_ADD_IP_ADDRESS:
	case SCTP_DEL_IP_ADDRESS:
	case SCTP_SET_PRIM_ADDR:
		stcb->asoc.peer_supports_asconf = 0;
		break;
	case SCTP_SUCCESS_REPORT:
	case SCTP_ERROR_CAUSE_IND:
		SCTPDBG(SCTP_DEBUG_INPUT2, "Huh, the peer does not support success? or error cause?\n");
		SCTPDBG(SCTP_DEBUG_INPUT2,
			"Turning off ASCONF to this strange peer\n");
		stcb->asoc.peer_supports_asconf = 0;
		break;
	default:
		SCTPDBG(SCTP_DEBUG_INPUT2,
			"Peer does not support param type %d(%x)??\n",
			pbad->param_type, (uint32_t) pbad->param_type);
		break;
	}
}

static int
sctp_handle_error(struct sctp_chunkhdr *ch,
    struct sctp_tcb *stcb, struct sctp_nets *net)
{
	int chklen;
	struct sctp_paramhdr *phdr;
	uint16_t error, error_type;
	uint16_t error_len;
	struct sctp_association *asoc;
	int adjust;
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
	struct socket *so;
#endif

	
	asoc = &stcb->asoc;
	phdr = (struct sctp_paramhdr *)((caddr_t)ch +
	    sizeof(struct sctp_chunkhdr));
	chklen = ntohs(ch->chunk_length) - sizeof(struct sctp_chunkhdr);
	error = 0;
	while ((size_t)chklen >= sizeof(struct sctp_paramhdr)) {
		
		error_type = ntohs(phdr->param_type);
		error_len = ntohs(phdr->param_length);
		if ((error_len > chklen) || (error_len == 0)) {
			
			SCTPDBG(SCTP_DEBUG_INPUT1, "Bogus length in error param- chunk left:%d errorlen:%d\n",
				chklen, error_len);
			return (0);
		}
		if (error == 0) {
			
			error = error_type;
		}
		switch (error_type) {
		case SCTP_CAUSE_INVALID_STREAM:
		case SCTP_CAUSE_MISSING_PARAM:
		case SCTP_CAUSE_INVALID_PARAM:
		case SCTP_CAUSE_NO_USER_DATA:
			SCTPDBG(SCTP_DEBUG_INPUT1, "Software error we got a %d back? We have a bug :/ (or do they?)\n",
				error_type);
			break;
		case SCTP_CAUSE_NAT_COLLIDING_STATE:
		        SCTPDBG(SCTP_DEBUG_INPUT2, "Received Colliding state abort flags:%x\n",
				ch->chunk_flags);
			if (sctp_handle_nat_colliding_state(stcb)) {
			  return (0);
			}
			break;
		case SCTP_CAUSE_NAT_MISSING_STATE:
			SCTPDBG(SCTP_DEBUG_INPUT2, "Received missing state abort flags:%x\n",
			                           ch->chunk_flags);
			if (sctp_handle_nat_missing_state(stcb, net)) {
			  return (0);
			}
			break;
		case SCTP_CAUSE_STALE_COOKIE:
			



			if (SCTP_GET_STATE(asoc) == SCTP_STATE_COOKIE_ECHOED) {
				int *p;

				p = (int *)((caddr_t)phdr + sizeof(*phdr));
				
				asoc->cookie_preserve_req = ntohl(*p) << 1;
				asoc->stale_cookie_count++;
				if (asoc->stale_cookie_count >
				    asoc->max_init_times) {
					sctp_abort_notification(stcb, 0, 0, NULL, SCTP_SO_NOT_LOCKED);
					
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
					so = SCTP_INP_SO(stcb->sctp_ep);
					atomic_add_int(&stcb->asoc.refcnt, 1);
					SCTP_TCB_UNLOCK(stcb);
					SCTP_SOCKET_LOCK(so, 1);
					SCTP_TCB_LOCK(stcb);
					atomic_subtract_int(&stcb->asoc.refcnt, 1);
#endif
					(void)sctp_free_assoc(stcb->sctp_ep, stcb, SCTP_NORMAL_PROC,
							      SCTP_FROM_SCTP_INPUT+SCTP_LOC_11);
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
					SCTP_SOCKET_UNLOCK(so, 1);
#endif
					return (-1);
				}
				
				sctp_toss_old_cookies(stcb, &stcb->asoc);
				asoc->state &= ~SCTP_STATE_COOKIE_ECHOED;
				asoc->state |= SCTP_STATE_COOKIE_WAIT;
				sctp_stop_all_cookie_timers(stcb);
				sctp_send_initiate(stcb->sctp_ep, stcb, SCTP_SO_NOT_LOCKED);
			}
			break;
		case SCTP_CAUSE_UNRESOLVABLE_ADDR:
			










			break;
		case SCTP_CAUSE_UNRECOG_CHUNK:
			sctp_process_unrecog_chunk(stcb, phdr, net);
			break;
		case SCTP_CAUSE_UNRECOG_PARAM:
			sctp_process_unrecog_param(stcb, phdr);
			break;
		case SCTP_CAUSE_COOKIE_IN_SHUTDOWN:
			





			break;
		case SCTP_CAUSE_DELETING_LAST_ADDR:
		case SCTP_CAUSE_RESOURCE_SHORTAGE:
		case SCTP_CAUSE_DELETING_SRC_ADDR:
			



			SCTPDBG(SCTP_DEBUG_INPUT2, "Peer sends ASCONF errors in a Operational Error?<%d>?\n",
				error_type);
			break;
		case SCTP_CAUSE_OUT_OF_RESC:
			






			break;
		default:
			SCTPDBG(SCTP_DEBUG_INPUT1, "sctp_handle_error: unknown error type = 0x%xh\n",
				error_type);
			break;
		}
		adjust = SCTP_SIZE32(error_len);
		chklen -= adjust;
		phdr = (struct sctp_paramhdr *)((caddr_t)phdr + adjust);
	}
	sctp_ulp_notify(SCTP_NOTIFY_REMOTE_ERROR, stcb, error, ch, SCTP_SO_NOT_LOCKED);
	return (0);
}

static int
sctp_handle_init_ack(struct mbuf *m, int iphlen, int offset,
                     struct sockaddr *src, struct sockaddr *dst, struct sctphdr *sh,
                     struct sctp_init_ack_chunk *cp, struct sctp_tcb *stcb,
                     struct sctp_nets *net, int *abort_no_unlock,
#if defined(__FreeBSD__)
                     uint8_t use_mflowid, uint32_t mflowid,
#endif
                     uint32_t vrf_id)
{
	struct sctp_init_ack *init_ack;
	struct mbuf *op_err;

	SCTPDBG(SCTP_DEBUG_INPUT2,
		"sctp_handle_init_ack: handling INIT-ACK\n");

	if (stcb == NULL) {
		SCTPDBG(SCTP_DEBUG_INPUT2,
			"sctp_handle_init_ack: TCB is null\n");
		return (-1);
	}
	if (ntohs(cp->ch.chunk_length) < sizeof(struct sctp_init_ack_chunk)) {
		
		op_err = sctp_generate_invmanparam(SCTP_CAUSE_INVALID_PARAM);
		sctp_abort_association(stcb->sctp_ep, stcb, m, iphlen,
		                       src, dst, sh, op_err,
#if defined(__FreeBSD__)
		                       use_mflowid, mflowid,
#endif
		                       vrf_id, net->port);
		*abort_no_unlock = 1;
		return (-1);
	}
	init_ack = &cp->init;
	
	if (init_ack->initiate_tag == 0) {
		
		op_err = sctp_generate_invmanparam(SCTP_CAUSE_INVALID_PARAM);
		sctp_abort_association(stcb->sctp_ep, stcb, m, iphlen,
		                       src, dst, sh, op_err,
#if defined(__FreeBSD__)
		                       use_mflowid, mflowid,
#endif
		                       vrf_id, net->port);
		*abort_no_unlock = 1;
		return (-1);
	}
	if (ntohl(init_ack->a_rwnd) < SCTP_MIN_RWND) {
		
		op_err = sctp_generate_invmanparam(SCTP_CAUSE_INVALID_PARAM);
		sctp_abort_association(stcb->sctp_ep, stcb, m, iphlen,
		                       src, dst, sh, op_err,
#if defined(__FreeBSD__)
		                       use_mflowid, mflowid,
#endif
		                       vrf_id, net->port);
		*abort_no_unlock = 1;
		return (-1);
	}
	if (init_ack->num_inbound_streams == 0) {
		
		op_err = sctp_generate_invmanparam(SCTP_CAUSE_INVALID_PARAM);
		sctp_abort_association(stcb->sctp_ep, stcb, m, iphlen,
		                       src, dst, sh, op_err,
#if defined(__FreeBSD__)
		                       use_mflowid, mflowid,
#endif
		                       vrf_id, net->port);
		*abort_no_unlock = 1;
		return (-1);
	}
	if (init_ack->num_outbound_streams == 0) {
		
		op_err = sctp_generate_invmanparam(SCTP_CAUSE_INVALID_PARAM);
		sctp_abort_association(stcb->sctp_ep, stcb, m, iphlen,
		                       src, dst, sh, op_err,
#if defined(__FreeBSD__)
		                       use_mflowid, mflowid,
#endif
		                       vrf_id, net->port);
		*abort_no_unlock = 1;
		return (-1);
	}
	
	switch (stcb->asoc.state & SCTP_STATE_MASK) {
	case SCTP_STATE_COOKIE_WAIT:
		
		
		if (stcb->asoc.primary_destination->dest_state &
		    SCTP_ADDR_UNCONFIRMED) {
			





			stcb->asoc.primary_destination->dest_state &=
			    ~SCTP_ADDR_UNCONFIRMED;
			sctp_ulp_notify(SCTP_NOTIFY_INTERFACE_CONFIRMED,
			    stcb, 0, (void *)stcb->asoc.primary_destination, SCTP_SO_NOT_LOCKED);
		}
		if (sctp_process_init_ack(m, iphlen, offset, src, dst, sh, cp, stcb,
		                          net, abort_no_unlock,
#if defined(__FreeBSD__)
		                          use_mflowid, mflowid,
#endif
		                          vrf_id) < 0) {
			
			return (-1);
		}
		
		SCTPDBG(SCTP_DEBUG_INPUT2, "moving to COOKIE-ECHOED state\n");
		SCTP_SET_STATE(&stcb->asoc, SCTP_STATE_COOKIE_ECHOED);

		
		if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_THRESHOLD_LOGGING) {
			sctp_misc_ints(SCTP_THRESHOLD_CLEAR,
				       stcb->asoc.overall_error_count,
				       0,
				       SCTP_FROM_SCTP_INPUT,
				       __LINE__);
		}
		stcb->asoc.overall_error_count = 0;
		(void)SCTP_GETTIME_TIMEVAL(&stcb->asoc.time_entered);
		



		sctp_timer_start(SCTP_TIMER_TYPE_COOKIE, stcb->sctp_ep,
		    stcb, net);
		



		break;
	case SCTP_STATE_SHUTDOWN_SENT:
		
		break;
	case SCTP_STATE_COOKIE_ECHOED:
		
		break;
	case SCTP_STATE_OPEN:
		
		break;
	case SCTP_STATE_EMPTY:
	case SCTP_STATE_INUSE:
	default:
		
		return (-1);
		break;
	}
	SCTPDBG(SCTP_DEBUG_INPUT1, "Leaving handle-init-ack end\n");
	return (0);
}

static struct sctp_tcb *
sctp_process_cookie_new(struct mbuf *m, int iphlen, int offset,
    struct sockaddr *src, struct sockaddr *dst,
    struct sctphdr *sh, struct sctp_state_cookie *cookie, int cookie_len,
    struct sctp_inpcb *inp, struct sctp_nets **netp,
    struct sockaddr *init_src, int *notification,
    int auth_skipped, uint32_t auth_offset, uint32_t auth_len,
#if defined(__FreeBSD__)
    uint8_t use_mflowid, uint32_t mflowid,
#endif
    uint32_t vrf_id, uint16_t port);








static struct sctp_tcb *
sctp_process_cookie_existing(struct mbuf *m, int iphlen, int offset,
    struct sockaddr *src, struct sockaddr *dst,
    struct sctphdr *sh, struct sctp_state_cookie *cookie, int cookie_len,
    struct sctp_inpcb *inp, struct sctp_tcb *stcb, struct sctp_nets **netp,
    struct sockaddr *init_src, int *notification,
    int auth_skipped, uint32_t auth_offset, uint32_t auth_len,
#if defined(__FreeBSD__)
    uint8_t use_mflowid, uint32_t mflowid,
#endif
    uint32_t vrf_id, uint16_t port)
{
	struct sctp_association *asoc;
	struct sctp_init_chunk *init_cp, init_buf;
	struct sctp_init_ack_chunk *initack_cp, initack_buf;
	struct sctp_nets *net;
	struct mbuf *op_err;
	struct sctp_paramhdr *ph;
	int init_offset, initack_offset, i;
	int retval;
	int spec_flag = 0;
	uint32_t how_indx;

	net = *netp;
	
	asoc = &stcb->asoc;
	for (how_indx = 0; how_indx  < sizeof(asoc->cookie_how); how_indx++) {
		if (asoc->cookie_how[how_indx] == 0)
			break;
	}
	if (how_indx < sizeof(asoc->cookie_how)) {
		asoc->cookie_how[how_indx] = 1;
	}
	if (SCTP_GET_STATE(asoc) == SCTP_STATE_SHUTDOWN_ACK_SENT) {
		
		sctp_send_shutdown_ack(stcb, stcb->asoc.primary_destination);
		op_err = sctp_get_mbuf_for_msg(sizeof(struct sctp_paramhdr),
					       0, M_NOWAIT, 1, MT_DATA);
		if (op_err == NULL) {
			
			return (NULL);
		}
		
		SCTP_BUF_LEN(op_err) = sizeof(struct sctp_paramhdr);
		ph = mtod(op_err, struct sctp_paramhdr *);
		ph->param_type = htons(SCTP_CAUSE_COOKIE_IN_SHUTDOWN);
		ph->param_length = htons(sizeof(struct sctp_paramhdr));
		sctp_send_operr_to(src, dst, sh, cookie->peers_vtag, op_err,
#if defined(__FreeBSD__)
		                   use_mflowid, mflowid,
#endif
		                   vrf_id, net->port);
		if (how_indx < sizeof(asoc->cookie_how))
			asoc->cookie_how[how_indx] = 2;
		return (NULL);
	}
	




	init_offset = offset += sizeof(struct sctp_cookie_echo_chunk);

	init_cp = (struct sctp_init_chunk *)
		sctp_m_getptr(m, init_offset, sizeof(struct sctp_init_chunk),
			      (uint8_t *) & init_buf);
	if (init_cp == NULL) {
		
		return (NULL);
	}
	if (init_cp->ch.chunk_type != SCTP_INITIATION) {
		return (NULL);
	}
	



	initack_offset = init_offset + SCTP_SIZE32(ntohs(init_cp->ch.chunk_length));
	initack_cp = (struct sctp_init_ack_chunk *)
		sctp_m_getptr(m, initack_offset, sizeof(struct sctp_init_ack_chunk),
			      (uint8_t *) & initack_buf);
	if (initack_cp == NULL) {
		
		return (NULL);
	}
	if (initack_cp->ch.chunk_type != SCTP_INITIATION_ACK) {
		return (NULL);
	}
	if ((ntohl(initack_cp->init.initiate_tag) == asoc->my_vtag) &&
	    (ntohl(init_cp->init.initiate_tag) == asoc->peer_vtag)) {
		



		if (ntohl(initack_cp->init.initial_tsn) != asoc->init_seq_number) {
			













			if (how_indx < sizeof(asoc->cookie_how))
				asoc->cookie_how[how_indx] = 17;
			return (NULL);

		}
 		switch SCTP_GET_STATE(asoc) {
			case SCTP_STATE_COOKIE_WAIT:
			case SCTP_STATE_COOKIE_ECHOED:
				





				
				retval = sctp_process_init(init_cp, stcb);
				if (retval < 0) {
					if (how_indx < sizeof(asoc->cookie_how))
						asoc->cookie_how[how_indx] = 3;
					return (NULL);
				}
				
				sctp_timer_stop(SCTP_TIMER_TYPE_HEARTBEAT, inp, stcb,
						net, SCTP_FROM_SCTP_INPUT+SCTP_LOC_12);
				sctp_timer_stop(SCTP_TIMER_TYPE_INIT, inp, stcb, net, SCTP_FROM_SCTP_INPUT+SCTP_LOC_13);
				
				if (SCTP_GET_STATE(asoc) == SCTP_STATE_COOKIE_ECHOED)
					SCTP_STAT_INCR_COUNTER32(sctps_activeestab);
				else
					SCTP_STAT_INCR_COUNTER32(sctps_collisionestab);

				SCTP_SET_STATE(asoc, SCTP_STATE_OPEN);
				if (asoc->state & SCTP_STATE_SHUTDOWN_PENDING) {
					sctp_timer_start(SCTP_TIMER_TYPE_SHUTDOWNGUARD,
							 stcb->sctp_ep, stcb, asoc->primary_destination);
				}
				SCTP_STAT_INCR_GAUGE32(sctps_currestab);
				sctp_stop_all_cookie_timers(stcb);
				if (((stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_TCPTYPE) ||
				     (stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_IN_TCPPOOL)) &&
				    (inp->sctp_socket->so_qlimit == 0)
					) {
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
					struct socket *so;
#endif
					





					stcb->sctp_ep->sctp_flags |=
						SCTP_PCB_FLAGS_CONNECTED;
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
					so = SCTP_INP_SO(stcb->sctp_ep);
					atomic_add_int(&stcb->asoc.refcnt, 1);
					SCTP_TCB_UNLOCK(stcb);
					SCTP_SOCKET_LOCK(so, 1);
					SCTP_TCB_LOCK(stcb);
					atomic_add_int(&stcb->asoc.refcnt, -1);
					if (stcb->asoc.state & SCTP_STATE_CLOSED_SOCKET) {
						SCTP_SOCKET_UNLOCK(so, 1);
 						return (NULL);
					}
#endif
					soisconnected(stcb->sctp_socket);
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
					SCTP_SOCKET_UNLOCK(so, 1);
#endif
				}
				
				*notification = SCTP_NOTIFY_ASSOC_UP;
				



				net->hb_responded = 1;
				net->RTO = sctp_calculate_rto(stcb, asoc, net,
							      &cookie->time_entered,
							      sctp_align_unsafe_makecopy,
							      SCTP_RTT_FROM_NON_DATA);

				if (stcb->asoc.sctp_autoclose_ticks &&
				    (sctp_is_feature_on(inp, SCTP_PCB_FLAGS_AUTOCLOSE))) {
					sctp_timer_start(SCTP_TIMER_TYPE_AUTOCLOSE,
							 inp, stcb, NULL);
				}
				break;
			default:
				



				break;
		}	
		sctp_stop_all_cookie_timers(stcb);
		




		if (sctp_load_addresses_from_init(stcb, m,
						  init_offset + sizeof(struct sctp_init_chunk),
						  initack_offset, src, dst, init_src)) {
			if (how_indx < sizeof(asoc->cookie_how))
				asoc->cookie_how[how_indx] = 4;
			return (NULL);
		}
		
		sctp_toss_old_cookies(stcb, asoc);
		sctp_send_cookie_ack(stcb);
		if (how_indx < sizeof(asoc->cookie_how))
			asoc->cookie_how[how_indx] = 5;
		return (stcb);
	}

	if (ntohl(initack_cp->init.initiate_tag) != asoc->my_vtag &&
	    ntohl(init_cp->init.initiate_tag) == asoc->peer_vtag &&
	    cookie->tie_tag_my_vtag == 0 &&
	    cookie->tie_tag_peer_vtag == 0) {
		


		if (how_indx < sizeof(asoc->cookie_how))
			asoc->cookie_how[how_indx] = 6;
		return (NULL);
	}
	


	if ((SCTP_GET_STATE(asoc) == SCTP_STATE_OPEN)  &&
	    (asoc->peer_supports_nat) &&
	    ((ntohl(initack_cp->init.initiate_tag) == asoc->my_vtag) &&
	    ((ntohl(init_cp->init.initiate_tag) != asoc->peer_vtag) ||
	     (asoc->peer_vtag == 0)))) {
		








		op_err = sctp_get_mbuf_for_msg(sizeof(struct sctp_paramhdr),
		                               0, M_NOWAIT, 1, MT_DATA);
		if (op_err == NULL) {
			
			return (NULL);
		}
		
#ifdef INET6
		SCTP_BUF_RESV_UF(op_err, sizeof(struct ip6_hdr));
#else
		SCTP_BUF_RESV_UF(op_err, sizeof(struct ip));
#endif
		SCTP_BUF_RESV_UF(op_err, sizeof(struct sctphdr));
		SCTP_BUF_RESV_UF(op_err,  sizeof(struct sctp_chunkhdr));
		
		SCTP_BUF_LEN(op_err) = sizeof(struct sctp_paramhdr);
		ph = mtod(op_err, struct sctp_paramhdr *);
		ph->param_type = htons(SCTP_CAUSE_NAT_COLLIDING_STATE);
		ph->param_length = htons(sizeof(struct sctp_paramhdr));
		sctp_send_abort(m, iphlen,  src, dst, sh, 0, op_err,
#if defined(__FreeBSD__)
		                use_mflowid, mflowid,
#endif
		                vrf_id, port);
		return (NULL);
	}
	if ((ntohl(initack_cp->init.initiate_tag) == asoc->my_vtag) &&
	    ((ntohl(init_cp->init.initiate_tag) != asoc->peer_vtag) ||
	     (asoc->peer_vtag == 0))) {
		



		if (ntohl(initack_cp->init.initial_tsn) != asoc->init_seq_number) {
			















			if (how_indx < sizeof(asoc->cookie_how))
				asoc->cookie_how[how_indx] = 7;

			return (NULL);
		}
		if (how_indx < sizeof(asoc->cookie_how))
			asoc->cookie_how[how_indx] = 8;
		sctp_timer_stop(SCTP_TIMER_TYPE_HEARTBEAT, inp, stcb, net, SCTP_FROM_SCTP_INPUT+SCTP_LOC_14);
		sctp_stop_all_cookie_timers(stcb);
		



		net->hb_responded = 1;
		if (stcb->asoc.sctp_autoclose_ticks &&
		    sctp_is_feature_on(inp, SCTP_PCB_FLAGS_AUTOCLOSE)) {
			sctp_timer_start(SCTP_TIMER_TYPE_AUTOCLOSE, inp, stcb,
					 NULL);
		}
		asoc->my_rwnd = ntohl(initack_cp->init.a_rwnd);
		asoc->pre_open_streams = ntohs(initack_cp->init.num_outbound_streams);

		if (ntohl(init_cp->init.initiate_tag) != asoc->peer_vtag) {
			






			struct sctp_tmit_chunk *chk;
		        TAILQ_FOREACH(chk, &stcb->asoc.sent_queue, sctp_next) {
				if (chk->sent < SCTP_DATAGRAM_RESEND) {
					chk->sent = SCTP_DATAGRAM_RESEND;
					sctp_flight_size_decrease(chk);
					sctp_total_flight_decrease(stcb, chk);
					sctp_ucount_incr(stcb->asoc.sent_queue_retran_cnt);
					spec_flag++;
				}
			}

		}
		
		retval = sctp_process_init(init_cp, stcb);
		if (retval < 0) {
			if (how_indx < sizeof(asoc->cookie_how))
				asoc->cookie_how[how_indx] = 9;
			return (NULL);
		}
		if (sctp_load_addresses_from_init(stcb, m,
						  init_offset + sizeof(struct sctp_init_chunk),
						  initack_offset, src, dst, init_src)) {
			if (how_indx < sizeof(asoc->cookie_how))
				asoc->cookie_how[how_indx] = 10;
			return (NULL);
		}
		if ((asoc->state & SCTP_STATE_COOKIE_WAIT) ||
		    (asoc->state & SCTP_STATE_COOKIE_ECHOED)) {
			*notification = SCTP_NOTIFY_ASSOC_UP;

			if (((stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_TCPTYPE) ||
			     (stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_IN_TCPPOOL)) &&
			    (inp->sctp_socket->so_qlimit == 0)) {
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
				struct socket *so;
#endif
				stcb->sctp_ep->sctp_flags |=
					SCTP_PCB_FLAGS_CONNECTED;
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
				so = SCTP_INP_SO(stcb->sctp_ep);
				atomic_add_int(&stcb->asoc.refcnt, 1);
				SCTP_TCB_UNLOCK(stcb);
				SCTP_SOCKET_LOCK(so, 1);
				SCTP_TCB_LOCK(stcb);
				atomic_add_int(&stcb->asoc.refcnt, -1);
				if (stcb->asoc.state & SCTP_STATE_CLOSED_SOCKET) {
					SCTP_SOCKET_UNLOCK(so, 1);
					return (NULL);
				}
#endif
				soisconnected(stcb->sctp_socket);
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
				SCTP_SOCKET_UNLOCK(so, 1);
#endif
			}
			if (SCTP_GET_STATE(asoc) == SCTP_STATE_COOKIE_ECHOED)
				SCTP_STAT_INCR_COUNTER32(sctps_activeestab);
			else
				SCTP_STAT_INCR_COUNTER32(sctps_collisionestab);
			SCTP_STAT_INCR_GAUGE32(sctps_currestab);
		} else if (SCTP_GET_STATE(asoc) == SCTP_STATE_OPEN) {
			SCTP_STAT_INCR_COUNTER32(sctps_restartestab);
		} else {
			SCTP_STAT_INCR_COUNTER32(sctps_collisionestab);
		}
		SCTP_SET_STATE(asoc, SCTP_STATE_OPEN);
		if (asoc->state & SCTP_STATE_SHUTDOWN_PENDING) {
			sctp_timer_start(SCTP_TIMER_TYPE_SHUTDOWNGUARD,
					 stcb->sctp_ep, stcb, asoc->primary_destination);
		}
		sctp_stop_all_cookie_timers(stcb);
		sctp_toss_old_cookies(stcb, asoc);
		sctp_send_cookie_ack(stcb);
		if (spec_flag) {
			





			sctp_chunk_output(inp,stcb, SCTP_OUTPUT_FROM_COOKIE_ACK, SCTP_SO_NOT_LOCKED);
		}
		if (how_indx < sizeof(asoc->cookie_how))
			asoc->cookie_how[how_indx] = 11;

		return (stcb);
	}
	if ((ntohl(initack_cp->init.initiate_tag) != asoc->my_vtag &&
	     ntohl(init_cp->init.initiate_tag) != asoc->peer_vtag) &&
	    cookie->tie_tag_my_vtag == asoc->my_vtag_nonce &&
	    cookie->tie_tag_peer_vtag == asoc->peer_vtag_nonce &&
	    cookie->tie_tag_peer_vtag != 0) {
		struct sctpasochead *head;
		if (asoc->peer_supports_nat) {
		  




		  return (sctp_process_cookie_new(m, iphlen, offset, src, dst,
		                                  sh, cookie, cookie_len,
						  inp, netp, init_src,notification,
						  auth_skipped, auth_offset, auth_len,
#if defined(__FreeBSD__)
						  use_mflowid, mflowid,
#endif
						  vrf_id, port));
		}
		


		
		if (how_indx < sizeof(asoc->cookie_how))
			asoc->cookie_how[how_indx] = 12;
		sctp_timer_stop(SCTP_TIMER_TYPE_INIT, inp, stcb, net, SCTP_FROM_SCTP_INPUT+SCTP_LOC_15);
		sctp_timer_stop(SCTP_TIMER_TYPE_HEARTBEAT, inp, stcb, net, SCTP_FROM_SCTP_INPUT+SCTP_LOC_16);

		
		*notification = SCTP_NOTIFY_ASSOC_RESTART;
		atomic_add_int(&stcb->asoc.refcnt, 1);
		if ((SCTP_GET_STATE(asoc) != SCTP_STATE_OPEN) &&
		    (SCTP_GET_STATE(asoc) != SCTP_STATE_SHUTDOWN_RECEIVED) &&
		    (SCTP_GET_STATE(asoc) != SCTP_STATE_SHUTDOWN_SENT)) {
			SCTP_STAT_INCR_GAUGE32(sctps_currestab);
		}
		if (SCTP_GET_STATE(asoc) == SCTP_STATE_OPEN) {
			SCTP_STAT_INCR_GAUGE32(sctps_restartestab);
		} else if (SCTP_GET_STATE(asoc) != SCTP_STATE_SHUTDOWN_SENT) {
			SCTP_STAT_INCR_GAUGE32(sctps_collisionestab);
		}
		if (asoc->state & SCTP_STATE_SHUTDOWN_PENDING) {
			SCTP_SET_STATE(asoc, SCTP_STATE_OPEN);
			sctp_timer_start(SCTP_TIMER_TYPE_SHUTDOWNGUARD,
					 stcb->sctp_ep, stcb, asoc->primary_destination);

		} else if (!(asoc->state & SCTP_STATE_SHUTDOWN_SENT)) {
			
			SCTP_SET_STATE(asoc, SCTP_STATE_OPEN);
		}
		asoc->pre_open_streams =
			ntohs(initack_cp->init.num_outbound_streams);
		asoc->init_seq_number = ntohl(initack_cp->init.initial_tsn);
		asoc->sending_seq = asoc->asconf_seq_out = asoc->str_reset_seq_out = asoc->init_seq_number;
		asoc->asconf_seq_out_acked = asoc->asconf_seq_out - 1;

		asoc->asconf_seq_in = asoc->last_acked_seq = asoc->init_seq_number - 1;

		asoc->str_reset_seq_in = asoc->init_seq_number;

		asoc->advanced_peer_ack_point = asoc->last_acked_seq;
		if (asoc->mapping_array) {
			memset(asoc->mapping_array, 0,
			       asoc->mapping_array_size);
		}
		if (asoc->nr_mapping_array) {
			memset(asoc->nr_mapping_array, 0,
			    asoc->mapping_array_size);
		}
		SCTP_TCB_UNLOCK(stcb);
		SCTP_INP_INFO_WLOCK();
		SCTP_INP_WLOCK(stcb->sctp_ep);
		SCTP_TCB_LOCK(stcb);
		atomic_add_int(&stcb->asoc.refcnt, -1);
		
		SCTP_TCB_SEND_LOCK(stcb);

		sctp_report_all_outbound(stcb, 0, 1, SCTP_SO_NOT_LOCKED);
		for (i = 0; i < stcb->asoc.streamoutcnt; i++) {
			stcb->asoc.strmout[i].chunks_on_queues = 0;
			stcb->asoc.strmout[i].stream_no = i;
			stcb->asoc.strmout[i].next_sequence_send = 0;
			stcb->asoc.strmout[i].last_msg_incomplete = 0;
		}
		
		asoc->my_vtag = ntohl(initack_cp->init.initiate_tag);
		asoc->my_rwnd = ntohl(initack_cp->init.a_rwnd);

		
		LIST_REMOVE(stcb, sctp_asocs);
		
		head = &SCTP_BASE_INFO(sctp_asochash)[SCTP_PCBHASH_ASOC(stcb->asoc.my_vtag,
								    SCTP_BASE_INFO(hashasocmark))];
		



		LIST_INSERT_HEAD(head, stcb, sctp_asocs);

		
		SCTP_TCB_SEND_UNLOCK(stcb);
		SCTP_INP_WUNLOCK(stcb->sctp_ep);
		SCTP_INP_INFO_WUNLOCK();

		retval = sctp_process_init(init_cp, stcb);
		if (retval < 0) {
			if (how_indx < sizeof(asoc->cookie_how))
				asoc->cookie_how[how_indx] = 13;

			return (NULL);
		}
		



		net->hb_responded = 1;

		if (sctp_load_addresses_from_init(stcb, m,
						  init_offset + sizeof(struct sctp_init_chunk),
						  initack_offset, src, dst, init_src)) {
			if (how_indx < sizeof(asoc->cookie_how))
				asoc->cookie_how[how_indx] = 14;

			return (NULL);
		}
		
		sctp_stop_all_cookie_timers(stcb);
		sctp_toss_old_cookies(stcb, asoc);
		sctp_send_cookie_ack(stcb);
		if (how_indx < sizeof(asoc->cookie_how))
			asoc->cookie_how[how_indx] = 15;

		return (stcb);
	}
	if (how_indx < sizeof(asoc->cookie_how))
		asoc->cookie_how[how_indx] = 16;
	
	return (NULL);
}









static struct sctp_tcb *
sctp_process_cookie_new(struct mbuf *m, int iphlen, int offset,
    struct sockaddr *src, struct sockaddr *dst,
    struct sctphdr *sh, struct sctp_state_cookie *cookie, int cookie_len,
    struct sctp_inpcb *inp, struct sctp_nets **netp,
    struct sockaddr *init_src, int *notification,
    int auth_skipped, uint32_t auth_offset, uint32_t auth_len,
#if defined(__FreeBSD__)
    uint8_t use_mflowid, uint32_t mflowid,
#endif
    uint32_t vrf_id, uint16_t port)
{
	struct sctp_tcb *stcb;
	struct sctp_init_chunk *init_cp, init_buf;
	struct sctp_init_ack_chunk *initack_cp, initack_buf;
	struct sockaddr_storage sa_store;
	struct sockaddr *initack_src = (struct sockaddr *)&sa_store;
	struct sctp_association *asoc;
	int init_offset, initack_offset, initack_limit;
	int retval;
	int error = 0;
	uint8_t auth_chunk_buf[SCTP_PARAM_BUFFER_SIZE];
#ifdef INET
	struct sockaddr_in *sin;
#endif
#ifdef INET6
	struct sockaddr_in6 *sin6;
#endif
#if defined(__Userspace__)
	struct sockaddr_conn *sconn;
#endif
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
	struct socket *so;

	so = SCTP_INP_SO(inp);
#endif

	




	init_offset = offset + sizeof(struct sctp_cookie_echo_chunk);
	init_cp = (struct sctp_init_chunk *)
	    sctp_m_getptr(m, init_offset, sizeof(struct sctp_init_chunk),
	    (uint8_t *) & init_buf);
	if (init_cp == NULL) {
		
		SCTPDBG(SCTP_DEBUG_INPUT1,
			"process_cookie_new: could not pull INIT chunk hdr\n");
		return (NULL);
	}
	if (init_cp->ch.chunk_type != SCTP_INITIATION) {
		SCTPDBG(SCTP_DEBUG_INPUT1, "HUH? process_cookie_new: could not find INIT chunk!\n");
		return (NULL);
	}
	initack_offset = init_offset + SCTP_SIZE32(ntohs(init_cp->ch.chunk_length));
	



	initack_cp = (struct sctp_init_ack_chunk *)
	    sctp_m_getptr(m, initack_offset, sizeof(struct sctp_init_ack_chunk),
	    (uint8_t *) & initack_buf);
	if (initack_cp == NULL) {
		
		SCTPDBG(SCTP_DEBUG_INPUT1, "process_cookie_new: could not pull INIT-ACK chunk hdr\n");
		return (NULL);
	}
	if (initack_cp->ch.chunk_type != SCTP_INITIATION_ACK) {
		return (NULL);
	}
	





	initack_limit = offset + cookie_len;

	




        





	stcb = sctp_aloc_assoc(inp, init_src, &error,
			       ntohl(initack_cp->init.initiate_tag), vrf_id,
#if defined(__FreeBSD__) && __FreeBSD_version >= 500000
			       (struct thread *)NULL
#elif defined(__Windows__)
			       (PKTHREAD)NULL
#else
			       (struct proc *)NULL
#endif
			       );
	if (stcb == NULL) {
		struct mbuf *op_err;

		
		SCTPDBG(SCTP_DEBUG_INPUT1,
			"process_cookie_new: no room for another TCB!\n");
		op_err = sctp_generate_invmanparam(SCTP_CAUSE_OUT_OF_RESC);

		sctp_abort_association(inp, (struct sctp_tcb *)NULL, m, iphlen,
		                       src, dst, sh, op_err,
#if defined(__FreeBSD__)
		                       use_mflowid, mflowid,
#endif
		                       vrf_id, port);
		return (NULL);
	}
	
	if (netp)
		*netp = sctp_findnet(stcb, init_src);

	asoc = &stcb->asoc;
	
	asoc->scope.ipv4_local_scope = cookie->ipv4_scope;
	asoc->scope.site_scope = cookie->site_scope;
	asoc->scope.local_scope = cookie->local_scope;
	asoc->scope.loopback_scope = cookie->loopback_scope;

#if defined(__Userspace__)
	if ((asoc->scope.ipv4_addr_legal != cookie->ipv4_addr_legal) ||
	    (asoc->scope.ipv6_addr_legal != cookie->ipv6_addr_legal) ||
	    (asoc->scope.conn_addr_legal != cookie->conn_addr_legal)) {
#else
	if ((asoc->scope.ipv4_addr_legal != cookie->ipv4_addr_legal) ||
	    (asoc->scope.ipv6_addr_legal != cookie->ipv6_addr_legal)) {
#endif
		struct mbuf *op_err;

		




		atomic_add_int(&stcb->asoc.refcnt, 1);
		op_err = sctp_generate_invmanparam(SCTP_CAUSE_OUT_OF_RESC);
		sctp_abort_association(inp, (struct sctp_tcb *)NULL, m, iphlen,
				       src, dst, sh, op_err,
#if defined(__FreeBSD__)
		                       use_mflowid, mflowid,
#endif
		                       vrf_id, port);
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
		SCTP_TCB_UNLOCK(stcb);
		SCTP_SOCKET_LOCK(so, 1);
		SCTP_TCB_LOCK(stcb);
#endif
		(void)sctp_free_assoc(inp, stcb, SCTP_NORMAL_PROC,
				      SCTP_FROM_SCTP_INPUT+SCTP_LOC_16);
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
		SCTP_SOCKET_UNLOCK(so, 1);
#endif
		atomic_subtract_int(&stcb->asoc.refcnt, 1);
		return (NULL);
	}
	
	asoc->my_vtag = ntohl(initack_cp->init.initiate_tag);
	asoc->my_rwnd = ntohl(initack_cp->init.a_rwnd);
	asoc->pre_open_streams = ntohs(initack_cp->init.num_outbound_streams);
	asoc->init_seq_number = ntohl(initack_cp->init.initial_tsn);
	asoc->sending_seq = asoc->asconf_seq_out = asoc->str_reset_seq_out = asoc->init_seq_number;
	asoc->asconf_seq_out_acked = asoc->asconf_seq_out - 1;
	asoc->asconf_seq_in = asoc->last_acked_seq = asoc->init_seq_number - 1;
	asoc->str_reset_seq_in = asoc->init_seq_number;

	asoc->advanced_peer_ack_point = asoc->last_acked_seq;

	
	if (netp)
		retval = sctp_process_init(init_cp, stcb);
	else
		retval = 0;
	if (retval < 0) {
		atomic_add_int(&stcb->asoc.refcnt, 1);
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
		SCTP_TCB_UNLOCK(stcb);
		SCTP_SOCKET_LOCK(so, 1);
		SCTP_TCB_LOCK(stcb);
#endif
		(void)sctp_free_assoc(inp, stcb, SCTP_NORMAL_PROC, SCTP_FROM_SCTP_INPUT+SCTP_LOC_16);
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
		SCTP_SOCKET_UNLOCK(so, 1);
#endif
		atomic_subtract_int(&stcb->asoc.refcnt, 1);
		return (NULL);
	}
	
	if (sctp_load_addresses_from_init(stcb, m,
	    init_offset + sizeof(struct sctp_init_chunk), initack_offset,
	    src, dst, init_src)) {
		atomic_add_int(&stcb->asoc.refcnt, 1);
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
		SCTP_TCB_UNLOCK(stcb);
		SCTP_SOCKET_LOCK(so, 1);
		SCTP_TCB_LOCK(stcb);
#endif
		(void)sctp_free_assoc(inp, stcb, SCTP_NORMAL_PROC, SCTP_FROM_SCTP_INPUT+SCTP_LOC_17);
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
		SCTP_SOCKET_UNLOCK(so, 1);
#endif
		atomic_subtract_int(&stcb->asoc.refcnt, 1);
		return (NULL);
	}
	


	
	sctp_auth_get_cookie_params(stcb, m,
	    initack_offset + sizeof(struct sctp_init_ack_chunk),
	    initack_limit - (initack_offset + sizeof(struct sctp_init_ack_chunk)));
	if (auth_skipped) {
		struct sctp_auth_chunk *auth;

		auth = (struct sctp_auth_chunk *)
		    sctp_m_getptr(m, auth_offset, auth_len, auth_chunk_buf);
		if ((auth == NULL) || sctp_handle_auth(stcb, auth, m, auth_offset)) {
			
			SCTPDBG(SCTP_DEBUG_AUTH1,
				"COOKIE-ECHO: AUTH failed\n");
			atomic_add_int(&stcb->asoc.refcnt, 1);
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
			SCTP_TCB_UNLOCK(stcb);
			SCTP_SOCKET_LOCK(so, 1);
			SCTP_TCB_LOCK(stcb);
#endif
			(void)sctp_free_assoc(inp, stcb, SCTP_NORMAL_PROC, SCTP_FROM_SCTP_INPUT+SCTP_LOC_18);
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
			SCTP_SOCKET_UNLOCK(so, 1);
#endif
			atomic_subtract_int(&stcb->asoc.refcnt, 1);
			return (NULL);
		} else {
			
			stcb->asoc.authenticated = 1;
		}
	}
	
	SCTPDBG(SCTP_DEBUG_INPUT2, "moving to OPEN state\n");
	SCTP_SET_STATE(asoc, SCTP_STATE_OPEN);
	if (asoc->state & SCTP_STATE_SHUTDOWN_PENDING) {
		sctp_timer_start(SCTP_TIMER_TYPE_SHUTDOWNGUARD,
				 stcb->sctp_ep, stcb, asoc->primary_destination);
	}
	sctp_stop_all_cookie_timers(stcb);
	SCTP_STAT_INCR_COUNTER32(sctps_passiveestab);
	SCTP_STAT_INCR_GAUGE32(sctps_currestab);

	








	
	
	switch (cookie->laddr_type) {
#ifdef INET
	case SCTP_IPV4_ADDRESS:
		
		sin = (struct sockaddr_in *)initack_src;
		memset(sin, 0, sizeof(*sin));
		sin->sin_family = AF_INET;
#ifdef HAVE_SIN_LEN
		sin->sin_len = sizeof(struct sockaddr_in);
#endif
		sin->sin_addr.s_addr = cookie->laddress[0];
		break;
#endif
#ifdef INET6
	case SCTP_IPV6_ADDRESS:
		
		sin6 = (struct sockaddr_in6 *)initack_src;
		memset(sin6, 0, sizeof(*sin6));
		sin6->sin6_family = AF_INET6;
#ifdef HAVE_SIN6_LEN
		sin6->sin6_len = sizeof(struct sockaddr_in6);
#endif
		sin6->sin6_scope_id = cookie->scope_id;
		memcpy(&sin6->sin6_addr, cookie->laddress,
		    sizeof(sin6->sin6_addr));
		break;
#endif
#if defined(__Userspace__)
	case SCTP_CONN_ADDRESS:
		
		sconn = (struct sockaddr_conn *)initack_src;
		memset(sconn, 0, sizeof(struct sockaddr_conn));
		sconn->sconn_family = AF_CONN;
#ifdef HAVE_SCONN_LEN
		sconn->sconn_len = sizeof(struct sockaddr_conn);
#endif
		memcpy(&sconn->sconn_addr, cookie->laddress, sizeof(void *));
		break;
#endif
	default:
		atomic_add_int(&stcb->asoc.refcnt, 1);
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
		SCTP_TCB_UNLOCK(stcb);
		SCTP_SOCKET_LOCK(so, 1);
		SCTP_TCB_LOCK(stcb);
#endif
		(void)sctp_free_assoc(inp, stcb, SCTP_NORMAL_PROC, SCTP_FROM_SCTP_INPUT+SCTP_LOC_19);
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
		SCTP_SOCKET_UNLOCK(so, 1);
#endif
		atomic_subtract_int(&stcb->asoc.refcnt, 1);
		return (NULL);
	}

	
	*notification = SCTP_NOTIFY_ASSOC_UP;
	if (((stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_TCPTYPE) ||
	    (stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_IN_TCPPOOL)) &&
	    (inp->sctp_socket->so_qlimit == 0)) {
		







		stcb->sctp_ep->sctp_flags |= SCTP_PCB_FLAGS_CONNECTED;
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
		atomic_add_int(&stcb->asoc.refcnt, 1);
		SCTP_TCB_UNLOCK(stcb);
		SCTP_SOCKET_LOCK(so, 1);
		SCTP_TCB_LOCK(stcb);
		atomic_subtract_int(&stcb->asoc.refcnt, 1);
		if (stcb->asoc.state & SCTP_STATE_CLOSED_SOCKET) {
			SCTP_SOCKET_UNLOCK(so, 1);
			return (NULL);
		}
#endif
		soisconnected(stcb->sctp_socket);
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
		SCTP_SOCKET_UNLOCK(so, 1);
#endif
	} else if ((stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_TCPTYPE) &&
	    (inp->sctp_socket->so_qlimit)) {
		




		;
	}
	
	if ((netp) && (*netp))
		(*netp)->hb_responded = 1;

	if (stcb->asoc.sctp_autoclose_ticks &&
	    sctp_is_feature_on(inp, SCTP_PCB_FLAGS_AUTOCLOSE)) {
		sctp_timer_start(SCTP_TIMER_TYPE_AUTOCLOSE, inp, stcb, NULL);
	}
	
	(void)SCTP_GETTIME_TIMEVAL(&stcb->asoc.time_entered);
	if ((netp) && (*netp)) {
		(*netp)->RTO = sctp_calculate_rto(stcb, asoc, *netp,
						  &cookie->time_entered, sctp_align_unsafe_makecopy,
						  SCTP_RTT_FROM_NON_DATA);
	}
	
	sctp_send_cookie_ack(stcb);

	



	sctp_check_address_list(stcb, m,
	    initack_offset + sizeof(struct sctp_init_ack_chunk),
	    initack_limit - (initack_offset + sizeof(struct sctp_init_ack_chunk)),
	    initack_src, cookie->local_scope, cookie->site_scope,
	    cookie->ipv4_scope, cookie->loopback_scope);


	return (stcb);
}


















static struct mbuf *
sctp_handle_cookie_echo(struct mbuf *m, int iphlen, int offset,
    struct sockaddr *src, struct sockaddr *dst,
    struct sctphdr *sh, struct sctp_cookie_echo_chunk *cp,
    struct sctp_inpcb **inp_p, struct sctp_tcb **stcb, struct sctp_nets **netp,
    int auth_skipped, uint32_t auth_offset, uint32_t auth_len,
    struct sctp_tcb **locked_tcb,
#if defined(__FreeBSD__)
    uint8_t use_mflowid, uint32_t mflowid,
#endif
    uint32_t vrf_id, uint16_t port)
{
	struct sctp_state_cookie *cookie;
	struct sctp_tcb *l_stcb = *stcb;
	struct sctp_inpcb *l_inp;
	struct sockaddr *to;
	struct sctp_pcb *ep;
	struct mbuf *m_sig;
	uint8_t calc_sig[SCTP_SIGNATURE_SIZE], tmp_sig[SCTP_SIGNATURE_SIZE];
	uint8_t *sig;
	uint8_t cookie_ok = 0;
	unsigned int sig_offset, cookie_offset;
	unsigned int cookie_len;
	struct timeval now;
	struct timeval time_expires;
	int notification = 0;
	struct sctp_nets *netl;
	int had_a_existing_tcb = 0;
	int send_int_conf = 0;
#ifdef INET
	struct sockaddr_in sin;
#endif
#ifdef INET6
	struct sockaddr_in6 sin6;
#endif
#if defined(__Userspace__)
	struct sockaddr_conn sconn;
#endif

	SCTPDBG(SCTP_DEBUG_INPUT2,
		"sctp_handle_cookie: handling COOKIE-ECHO\n");

	if (inp_p == NULL) {
		return (NULL);
	}
	cookie = &cp->cookie;
	cookie_offset = offset + sizeof(struct sctp_chunkhdr);
	cookie_len = ntohs(cp->ch.chunk_length);

	if ((cookie->peerport != sh->src_port) &&
	    (cookie->myport != sh->dest_port) &&
	    (cookie->my_vtag != sh->v_tag)) {
		






		return (NULL);
	}
	if (cookie_len < sizeof(struct sctp_cookie_echo_chunk) +
	    sizeof(struct sctp_init_chunk) +
	    sizeof(struct sctp_init_ack_chunk) + SCTP_SIGNATURE_SIZE) {
		
		return (NULL);
	}
	



	sig_offset = offset + cookie_len - SCTP_SIGNATURE_SIZE;
	m_sig = m_split(m, sig_offset, M_NOWAIT);
	if (m_sig == NULL) {
		
		return (NULL);
	}
#ifdef SCTP_MBUF_LOGGING
	if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_MBUF_LOGGING_ENABLE) {
		struct mbuf *mat;

		for (mat = m_sig; mat; mat = SCTP_BUF_NEXT(mat)) {
			if (SCTP_BUF_IS_EXTENDED(mat)) {
				sctp_log_mb(mat, SCTP_MBUF_SPLIT);
			}
		}
	}
#endif

	


	ep = &(*inp_p)->sctp_ep;
	l_inp = *inp_p;
	if (l_stcb) {
		SCTP_TCB_UNLOCK(l_stcb);
	}
	SCTP_INP_RLOCK(l_inp);
	if (l_stcb) {
		SCTP_TCB_LOCK(l_stcb);
	}
	
	if ((cookie->time_entered.tv_sec < (long)ep->time_of_secret_change) &&
	    (ep->current_secret_number != ep->last_secret_number)) {
		
		(void)sctp_hmac_m(SCTP_HMAC,
		    (uint8_t *)ep->secret_key[(int)ep->last_secret_number],
		    SCTP_SECRET_SIZE, m, cookie_offset, calc_sig, 0);
	} else {
		
		(void)sctp_hmac_m(SCTP_HMAC,
		    (uint8_t *)ep->secret_key[(int)ep->current_secret_number],
		    SCTP_SECRET_SIZE, m, cookie_offset, calc_sig, 0);
	}
	
	SCTP_INP_RUNLOCK(l_inp);
	sig = (uint8_t *) sctp_m_getptr(m_sig, 0, SCTP_SIGNATURE_SIZE, (uint8_t *) & tmp_sig);
	if (sig == NULL) {
		
		sctp_m_freem(m_sig);
		return (NULL);
	}
	
	if (memcmp(calc_sig, sig, SCTP_SIGNATURE_SIZE) != 0) {
		
		if ((cookie->time_entered.tv_sec == (long)ep->time_of_secret_change) &&
		    (ep->current_secret_number != ep->last_secret_number)) {
			
			(void)sctp_hmac_m(SCTP_HMAC,
			    (uint8_t *)ep->secret_key[(int)ep->last_secret_number],
			    SCTP_SECRET_SIZE, m, cookie_offset, calc_sig, 0);
			
			if (memcmp(calc_sig, sig, SCTP_SIGNATURE_SIZE) == 0)
				cookie_ok = 1;
		}
	} else {
		cookie_ok = 1;
	}

	



	{
		struct mbuf *m_at;

		m_at = m;
		while (SCTP_BUF_NEXT(m_at) != NULL) {
			m_at = SCTP_BUF_NEXT(m_at);
		}
		SCTP_BUF_NEXT(m_at) = m_sig;
	}

	if (cookie_ok == 0) {
		SCTPDBG(SCTP_DEBUG_INPUT2, "handle_cookie_echo: cookie signature validation failed!\n");
		SCTPDBG(SCTP_DEBUG_INPUT2,
			"offset = %u, cookie_offset = %u, sig_offset = %u\n",
			(uint32_t) offset, cookie_offset, sig_offset);
		return (NULL);
	}

	


	(void)SCTP_GETTIME_TIMEVAL(&now);
	
	time_expires.tv_sec = cookie->time_entered.tv_sec + TICKS_TO_SEC(cookie->cookie_life);
	time_expires.tv_usec = cookie->time_entered.tv_usec;
        


#ifndef __FreeBSD__
	if (timercmp(&now, &time_expires, >))
#else
	if (timevalcmp(&now, &time_expires, >))
#endif
	{
		
		struct mbuf *op_err;
		struct sctp_stale_cookie_msg *scm;
		uint32_t tim;
		op_err = sctp_get_mbuf_for_msg(sizeof(struct sctp_stale_cookie_msg),
					       0, M_NOWAIT, 1, MT_DATA);
		if (op_err == NULL) {
			
			return (NULL);
		}
		
		SCTP_BUF_LEN(op_err) = sizeof(struct sctp_stale_cookie_msg);
		scm = mtod(op_err, struct sctp_stale_cookie_msg *);
		scm->ph.param_type = htons(SCTP_CAUSE_STALE_COOKIE);
		scm->ph.param_length = htons((sizeof(struct sctp_paramhdr) +
		    (sizeof(uint32_t))));
		
		tim = (now.tv_sec - time_expires.tv_sec) * 1000000;
		
		if (tim == 0)
			tim = now.tv_usec - cookie->time_entered.tv_usec;
		scm->time_usec = htonl(tim);
		sctp_send_operr_to(src, dst, sh, cookie->peers_vtag, op_err,
#if defined(__FreeBSD__)
		                   use_mflowid, mflowid,
#endif
		                   vrf_id, port);
		return (NULL);
	}
	








	to = NULL;
	switch (cookie->addr_type) {
#ifdef INET6
	case SCTP_IPV6_ADDRESS:
		memset(&sin6, 0, sizeof(sin6));
		sin6.sin6_family = AF_INET6;
#ifdef HAVE_SIN6_LEN
		sin6.sin6_len = sizeof(sin6);
#endif
		sin6.sin6_port = sh->src_port;
		sin6.sin6_scope_id = cookie->scope_id;
		memcpy(&sin6.sin6_addr.s6_addr, cookie->address,
		    sizeof(sin6.sin6_addr.s6_addr));
		to = (struct sockaddr *)&sin6;
		break;
#endif
#ifdef INET
	case SCTP_IPV4_ADDRESS:
		memset(&sin, 0, sizeof(sin));
		sin.sin_family = AF_INET;
#ifdef HAVE_SIN_LEN
		sin.sin_len = sizeof(sin);
#endif
		sin.sin_port = sh->src_port;
		sin.sin_addr.s_addr = cookie->address[0];
		to = (struct sockaddr *)&sin;
		break;
#endif
#if defined(__Userspace__)
	case SCTP_CONN_ADDRESS:
		memset(&sconn, 0, sizeof(struct sockaddr_conn));
		sconn.sconn_family = AF_CONN;
#ifdef HAVE_SCONN_LEN
		sconn.sconn_len = sizeof(struct sockaddr_conn);
#endif
		sconn.sconn_port = sh->src_port;
		memcpy(&sconn.sconn_addr, cookie->address, sizeof(void *));
		to = (struct sockaddr *)&sconn;
		break;
#endif
	default:
		
		return (NULL);
	}
	if ((*stcb == NULL) && to) {
		
		*stcb = sctp_findassociation_ep_addr(inp_p, to, netp, dst, NULL);
		if (*stcb == NULL) {
			




			if (l_inp != *inp_p) {
				SCTP_PRINTF("Bad problem find_ep got a diff inp then special_locate?\n");
			}
		} else {
			if (*locked_tcb == NULL) {
				





				*locked_tcb = *stcb;

				





				SCTP_INP_INCR_REF((*stcb)->sctp_ep);
				if ((*stcb)->sctp_ep != l_inp) {
					SCTP_PRINTF("Huh? ep:%p diff then l_inp:%p?\n",
						    (void *)(*stcb)->sctp_ep, (void *)l_inp);
				}
			}
		}
	}
	if (to == NULL) {
		return (NULL);
	}

	cookie_len -= SCTP_SIGNATURE_SIZE;
	if (*stcb == NULL) {
		
		*stcb = sctp_process_cookie_new(m, iphlen, offset, src, dst, sh,
		                                cookie, cookie_len, *inp_p,
		                                netp, to, &notification,
		                                auth_skipped, auth_offset, auth_len,
#if defined(__FreeBSD__)
		                                use_mflowid, mflowid,
#endif
		                                vrf_id, port);
	} else {
		
		had_a_existing_tcb = 1;
		*stcb = sctp_process_cookie_existing(m, iphlen, offset,
		                                     src, dst, sh,
						     cookie, cookie_len, *inp_p, *stcb, netp, to,
						     &notification, auth_skipped, auth_offset, auth_len,
#if defined(__FreeBSD__)
		                                     use_mflowid, mflowid,
#endif
		                                     vrf_id, port);
	}

	if (*stcb == NULL) {
		
		return (NULL);
	}
#if defined(__FreeBSD__)
	if ((*netp != NULL) && (use_mflowid != 0)) {
		(*netp)->flowid = mflowid;
#ifdef INVARIANTS
		(*netp)->flowidset = 1;
#endif
	}
#endif
	



	netl = sctp_findnet(*stcb, to);
	


	if (netl == NULL) {
		
		if (sctp_add_remote_addr(*stcb, to, NULL, SCTP_DONOT_SETSCOPE, SCTP_IN_COOKIE_PROC)) {
			return (NULL);
		}
		netl = sctp_findnet(*stcb, to);
	}
	if (netl) {
		if (netl->dest_state & SCTP_ADDR_UNCONFIRMED) {
			netl->dest_state &= ~SCTP_ADDR_UNCONFIRMED;
			(void)sctp_set_primary_addr((*stcb), (struct sockaddr *)NULL,
			    netl);
			send_int_conf = 1;
		}
	}
	sctp_start_net_timers(*stcb);
	if ((*inp_p)->sctp_flags & SCTP_PCB_FLAGS_TCPTYPE) {
		if (!had_a_existing_tcb ||
		    (((*inp_p)->sctp_flags & SCTP_PCB_FLAGS_CONNECTED) == 0)) {
			




			struct socket *so, *oso;
			struct sctp_inpcb *inp;

			if (notification == SCTP_NOTIFY_ASSOC_RESTART) {
				



				sctp_ulp_notify(notification, *stcb, 0, NULL, SCTP_SO_NOT_LOCKED);
				if (send_int_conf) {
					sctp_ulp_notify(SCTP_NOTIFY_INTERFACE_CONFIRMED,
			    		                (*stcb), 0, (void *)netl, SCTP_SO_NOT_LOCKED);
				}
				return (m);
			}
			oso = (*inp_p)->sctp_socket;
#if (defined(__FreeBSD__) && __FreeBSD_version < 700000)
			



			NET_LOCK_GIANT();
#endif
			atomic_add_int(&(*stcb)->asoc.refcnt, 1);
			SCTP_TCB_UNLOCK((*stcb));
#if defined(__FreeBSD__) && __FreeBSD_version >= 801000
			CURVNET_SET(oso->so_vnet);
#endif
#if defined(__APPLE__)
			SCTP_SOCKET_LOCK(oso, 1);
#endif
			so = sonewconn(oso, 0
#if defined(__APPLE__)
			    ,NULL
#endif
#ifdef __Panda__
			     ,NULL , (*inp_p)->def_vrf_id
#endif
			    );
#if (defined(__FreeBSD__) && __FreeBSD_version < 700000)
			NET_UNLOCK_GIANT();
#endif
#if defined(__APPLE__)
			SCTP_SOCKET_UNLOCK(oso, 1);
#endif
#if defined(__FreeBSD__) && __FreeBSD_version >= 801000
			CURVNET_RESTORE();
#endif
			SCTP_TCB_LOCK((*stcb));
			atomic_subtract_int(&(*stcb)->asoc.refcnt, 1);

			if (so == NULL) {
				struct mbuf *op_err;
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
				struct socket *pcb_so;
#endif
				
				SCTPDBG(SCTP_DEBUG_INPUT1, "process_cookie_new: no room for another socket!\n");
				op_err = sctp_generate_invmanparam(SCTP_CAUSE_OUT_OF_RESC);
				sctp_abort_association(*inp_p, NULL, m, iphlen,
						       src, dst, sh, op_err,
#if defined(__FreeBSD__)
				                       use_mflowid, mflowid,
#endif
				                       vrf_id, port);
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
				pcb_so = SCTP_INP_SO(*inp_p);
				atomic_add_int(&(*stcb)->asoc.refcnt, 1);
				SCTP_TCB_UNLOCK((*stcb));
				SCTP_SOCKET_LOCK(pcb_so, 1);
				SCTP_TCB_LOCK((*stcb));
				atomic_subtract_int(&(*stcb)->asoc.refcnt, 1);
#endif
				(void)sctp_free_assoc(*inp_p, *stcb, SCTP_NORMAL_PROC, SCTP_FROM_SCTP_INPUT+SCTP_LOC_20);
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
				SCTP_SOCKET_UNLOCK(pcb_so, 1);
#endif
				return (NULL);
			}
			inp = (struct sctp_inpcb *)so->so_pcb;
			SCTP_INP_INCR_REF(inp);
			




			inp->sctp_flags = (SCTP_PCB_FLAGS_TCPTYPE |
			    SCTP_PCB_FLAGS_CONNECTED |
			    SCTP_PCB_FLAGS_IN_TCPPOOL |
 			    SCTP_PCB_FLAGS_UNBOUND |
			    (SCTP_PCB_COPY_FLAGS & (*inp_p)->sctp_flags) |
			    SCTP_PCB_FLAGS_DONT_WAKE);
			inp->sctp_features = (*inp_p)->sctp_features;
			inp->sctp_mobility_features = (*inp_p)->sctp_mobility_features;
			inp->sctp_socket = so;
			inp->sctp_frag_point = (*inp_p)->sctp_frag_point;
			inp->sctp_cmt_on_off = (*inp_p)->sctp_cmt_on_off;
			inp->sctp_ecn_enable = (*inp_p)->sctp_ecn_enable;
			inp->partial_delivery_point = (*inp_p)->partial_delivery_point;
			inp->sctp_context = (*inp_p)->sctp_context;
			inp->local_strreset_support = (*inp_p)->local_strreset_support;
			inp->inp_starting_point_for_iterator = NULL;
#if defined(__Userspace__)
			inp->ulp_info = (*inp_p)->ulp_info;
			inp->recv_callback = (*inp_p)->recv_callback;
			inp->send_callback = (*inp_p)->send_callback;
			inp->send_sb_threshold = (*inp_p)->send_sb_threshold;
#endif
			



			if (inp->sctp_ep.local_hmacs)
				sctp_free_hmaclist(inp->sctp_ep.local_hmacs);
			inp->sctp_ep.local_hmacs =
			    sctp_copy_hmaclist((*inp_p)->sctp_ep.local_hmacs);
			if (inp->sctp_ep.local_auth_chunks)
				sctp_free_chunklist(inp->sctp_ep.local_auth_chunks);
			inp->sctp_ep.local_auth_chunks =
			    sctp_copy_chunklist((*inp_p)->sctp_ep.local_auth_chunks);

			




			


			if (*stcb) {
				(*stcb)->asoc.state |= SCTP_STATE_IN_ACCEPT_QUEUE;
			}
			sctp_move_pcb_and_assoc(*inp_p, inp, *stcb);

			atomic_add_int(&(*stcb)->asoc.refcnt, 1);
			SCTP_TCB_UNLOCK((*stcb));

#if defined(__FreeBSD__)
			sctp_pull_off_control_to_new_inp((*inp_p), inp, *stcb,
			    0);
#else
			sctp_pull_off_control_to_new_inp((*inp_p), inp, *stcb, M_NOWAIT);
#endif
			SCTP_TCB_LOCK((*stcb));
			atomic_subtract_int(&(*stcb)->asoc.refcnt, 1);


			


			if (inp->sctp_flags & SCTP_PCB_FLAGS_SOCKET_GONE) {
				




				SCTP_INP_DECR_REF(inp);
				return (NULL);
			}
			SCTP_INP_DECR_REF(inp);
			
			*inp_p = inp;
			sctp_ulp_notify(notification, *stcb, 0, NULL, SCTP_SO_NOT_LOCKED);
			if (send_int_conf) {
				sctp_ulp_notify(SCTP_NOTIFY_INTERFACE_CONFIRMED,
				                (*stcb), 0, (void *)netl, SCTP_SO_NOT_LOCKED);
			}

			
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
			atomic_add_int(&(*stcb)->asoc.refcnt, 1);
			SCTP_TCB_UNLOCK((*stcb));
			SCTP_SOCKET_LOCK(so, 1);
#endif
			soisconnected(so);
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
			SCTP_TCB_LOCK((*stcb));
			atomic_subtract_int(&(*stcb)->asoc.refcnt, 1);
			SCTP_SOCKET_UNLOCK(so, 1);
#endif
			return (m);
		}
	}
	if (notification) {
		sctp_ulp_notify(notification, *stcb, 0, NULL, SCTP_SO_NOT_LOCKED);
	}
	if (send_int_conf) {
		sctp_ulp_notify(SCTP_NOTIFY_INTERFACE_CONFIRMED,
		                (*stcb), 0, (void *)netl, SCTP_SO_NOT_LOCKED);
	}
	return (m);
}

static void
sctp_handle_cookie_ack(struct sctp_cookie_ack_chunk *cp SCTP_UNUSED,
    struct sctp_tcb *stcb, struct sctp_nets *net)
{
	
	struct sctp_association *asoc;

	SCTPDBG(SCTP_DEBUG_INPUT2,
		"sctp_handle_cookie_ack: handling COOKIE-ACK\n");
	if (stcb == NULL)
		return;

	asoc = &stcb->asoc;

	sctp_stop_all_cookie_timers(stcb);
	
	if (SCTP_GET_STATE(asoc) == SCTP_STATE_COOKIE_ECHOED) {
		
		SCTPDBG(SCTP_DEBUG_INPUT2, "moving to OPEN state\n");
		SCTP_SET_STATE(asoc, SCTP_STATE_OPEN);
		sctp_start_net_timers(stcb);
		if (asoc->state & SCTP_STATE_SHUTDOWN_PENDING) {
			sctp_timer_start(SCTP_TIMER_TYPE_SHUTDOWNGUARD,
					 stcb->sctp_ep, stcb, asoc->primary_destination);

		}
		
		SCTP_STAT_INCR_COUNTER32(sctps_activeestab);
		SCTP_STAT_INCR_GAUGE32(sctps_currestab);
		if (asoc->overall_error_count == 0) {
			net->RTO = sctp_calculate_rto(stcb, asoc, net,
					             &asoc->time_entered, sctp_align_safe_nocopy,
						      SCTP_RTT_FROM_NON_DATA);
		}
		(void)SCTP_GETTIME_TIMEVAL(&asoc->time_entered);
		sctp_ulp_notify(SCTP_NOTIFY_ASSOC_UP, stcb, 0, NULL, SCTP_SO_NOT_LOCKED);
		if ((stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_TCPTYPE) ||
		    (stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_IN_TCPPOOL)) {
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
			struct socket *so;

#endif
			stcb->sctp_ep->sctp_flags |= SCTP_PCB_FLAGS_CONNECTED;
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
			so = SCTP_INP_SO(stcb->sctp_ep);
			atomic_add_int(&stcb->asoc.refcnt, 1);
			SCTP_TCB_UNLOCK(stcb);
			SCTP_SOCKET_LOCK(so, 1);
			SCTP_TCB_LOCK(stcb);
			atomic_subtract_int(&stcb->asoc.refcnt, 1);
#endif
			if ((stcb->asoc.state & SCTP_STATE_CLOSED_SOCKET) == 0) {
				soisconnected(stcb->sctp_socket);
			}
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
			SCTP_SOCKET_UNLOCK(so, 1);
#endif
		}
		



		net->hb_responded = 1;

		if (stcb->asoc.state & SCTP_STATE_CLOSED_SOCKET) {
			


			goto closed_socket;
		}

		sctp_timer_start(SCTP_TIMER_TYPE_HEARTBEAT, stcb->sctp_ep,
		    stcb, net);


		if (stcb->asoc.sctp_autoclose_ticks &&
		    sctp_is_feature_on(stcb->sctp_ep, SCTP_PCB_FLAGS_AUTOCLOSE)) {
			sctp_timer_start(SCTP_TIMER_TYPE_AUTOCLOSE,
			    stcb->sctp_ep, stcb, NULL);
		}
		




		if ((sctp_is_feature_on(stcb->sctp_ep, SCTP_PCB_FLAGS_DO_ASCONF)) &&
		    (stcb->asoc.peer_supports_asconf) &&
		    (!TAILQ_EMPTY(&stcb->asoc.asconf_queue))) {
#ifdef SCTP_TIMER_BASED_ASCONF
			sctp_timer_start(SCTP_TIMER_TYPE_ASCONF,
					 stcb->sctp_ep, stcb,
					 stcb->asoc.primary_destination);
#else
			sctp_send_asconf(stcb, stcb->asoc.primary_destination,
					 SCTP_ADDR_NOT_LOCKED);
#endif
		}
	}
closed_socket:
	
	sctp_toss_old_cookies(stcb, asoc);
	if (!TAILQ_EMPTY(&asoc->sent_queue)) {
		
		struct sctp_tmit_chunk *chk;

		chk = TAILQ_FIRST(&asoc->sent_queue);
		sctp_timer_start(SCTP_TIMER_TYPE_SEND, stcb->sctp_ep, stcb, chk->whoTo);
	}
}

static void
sctp_handle_ecn_echo(struct sctp_ecne_chunk *cp,
		     struct sctp_tcb *stcb)
{
	struct sctp_nets *net;
	struct sctp_tmit_chunk *lchk;
	struct sctp_ecne_chunk bkup;
	uint8_t override_bit;
	uint32_t tsn, window_data_tsn;
	int len;
	unsigned int pkt_cnt;

	len = ntohs(cp->ch.chunk_length);
	if ((len != sizeof(struct sctp_ecne_chunk)) &&
	    (len != sizeof(struct old_sctp_ecne_chunk))) {
		return;
	}
	if (len == sizeof(struct old_sctp_ecne_chunk)) {
		
		memcpy(&bkup, cp, sizeof(struct old_sctp_ecne_chunk));
		bkup.num_pkts_since_cwr = htonl(1);
		cp = &bkup;
	}
	SCTP_STAT_INCR(sctps_recvecne);
	tsn = ntohl(cp->tsn);
	pkt_cnt = ntohl(cp->num_pkts_since_cwr);
	lchk = TAILQ_LAST(&stcb->asoc.send_queue, sctpchunk_listhead);
	if (lchk == NULL) {
		window_data_tsn = stcb->asoc.sending_seq - 1;
	} else {
		window_data_tsn = lchk->rec.data.TSN_seq;
	}

	
	net = NULL;
	TAILQ_FOREACH(lchk, &stcb->asoc.sent_queue, sctp_next) {
		if (lchk->rec.data.TSN_seq == tsn) {
			net = lchk->whoTo;
			net->ecn_prev_cwnd = lchk->rec.data.cwnd_at_send;
			break;
		}
		if (SCTP_TSN_GT(lchk->rec.data.TSN_seq, tsn)) {
			break;
		}
	}
	if (net == NULL) {
		




		TAILQ_FOREACH(net, &stcb->asoc.nets, sctp_next) {
			if (tsn == net->last_cwr_tsn) {
				
				break;
			}
		}
		if (net == NULL) {
			




			net = TAILQ_FIRST(&stcb->asoc.nets);
			if (net == NULL) {
				
				return;
			}
			override_bit = SCTP_CWR_REDUCE_OVERRIDE;
		} else {
			override_bit = 0;
		}
	} else {
		override_bit = 0;
	}
	if (SCTP_TSN_GT(tsn, net->cwr_window_tsn) &&
	    ((override_bit&SCTP_CWR_REDUCE_OVERRIDE) == 0)) {
		
		stcb->asoc.cc_functions.sctp_cwnd_update_after_ecn_echo(stcb, net, 0, pkt_cnt);
		



		net->cwr_window_tsn = window_data_tsn;
		net->ecn_ce_pkt_cnt += pkt_cnt;
		net->lost_cnt = pkt_cnt;
		net->last_cwr_tsn = tsn;
	} else {
		override_bit |= SCTP_CWR_IN_SAME_WINDOW;
		if (SCTP_TSN_GT(tsn, net->last_cwr_tsn) &&
		    ((override_bit&SCTP_CWR_REDUCE_OVERRIDE) == 0)) {
			



			int cnt = 1;
			if (pkt_cnt > net->lost_cnt) {
				
				cnt = (pkt_cnt - net->lost_cnt);
				net->ecn_ce_pkt_cnt += cnt;
			}
			net->lost_cnt = pkt_cnt;
			net->last_cwr_tsn = tsn;
			



			stcb->asoc.cc_functions.sctp_cwnd_update_after_ecn_echo(stcb, net, 1, cnt);
		}
	}
	





	sctp_send_cwr(stcb, net, net->last_cwr_tsn, override_bit);
}

static void
sctp_handle_ecn_cwr(struct sctp_cwr_chunk *cp, struct sctp_tcb *stcb, struct sctp_nets *net)
{
	




	struct sctp_tmit_chunk *chk;
	struct sctp_ecne_chunk *ecne;
	int override;
	uint32_t cwr_tsn;
	cwr_tsn = ntohl(cp->tsn);

	override = cp->ch.chunk_flags & SCTP_CWR_REDUCE_OVERRIDE;
	TAILQ_FOREACH(chk, &stcb->asoc.control_send_queue, sctp_next) {
		if (chk->rec.chunk_id.id != SCTP_ECN_ECHO) {
			continue;
		}
		if ((override == 0) && (chk->whoTo != net)) {
			
			continue;
		}
		ecne = mtod(chk->data, struct sctp_ecne_chunk *);
		if (SCTP_TSN_GE(cwr_tsn, ntohl(ecne->tsn))) {
			
			stcb->asoc.ecn_echo_cnt_onq--;
			TAILQ_REMOVE(&stcb->asoc.control_send_queue, chk,
			    sctp_next);
			if (chk->data) {
				sctp_m_freem(chk->data);
				chk->data = NULL;
			}
			stcb->asoc.ctrl_queue_cnt--;
			sctp_free_a_chunk(stcb, chk, SCTP_SO_NOT_LOCKED);
			if (override == 0) {
				break;
			}
		}
	}
}

static void
sctp_handle_shutdown_complete(struct sctp_shutdown_complete_chunk *cp SCTP_UNUSED,
    struct sctp_tcb *stcb, struct sctp_nets *net)
{
	struct sctp_association *asoc;
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
	struct socket *so;
#endif

	SCTPDBG(SCTP_DEBUG_INPUT2,
		"sctp_handle_shutdown_complete: handling SHUTDOWN-COMPLETE\n");
	if (stcb == NULL)
		return;

	asoc = &stcb->asoc;
	
	if (SCTP_GET_STATE(asoc) != SCTP_STATE_SHUTDOWN_ACK_SENT) {
		
		SCTPDBG(SCTP_DEBUG_INPUT2,
			"sctp_handle_shutdown_complete: not in SCTP_STATE_SHUTDOWN_ACK_SENT --- ignore\n");
		SCTP_TCB_UNLOCK(stcb);
		return;
	}
	
	if (stcb->sctp_socket) {
		sctp_ulp_notify(SCTP_NOTIFY_ASSOC_DOWN, stcb, 0, NULL, SCTP_SO_NOT_LOCKED);
		
		if (!TAILQ_EMPTY(&asoc->send_queue) ||
		    !TAILQ_EMPTY(&asoc->sent_queue) ||
		    !stcb->asoc.ss_functions.sctp_ss_is_empty(stcb, asoc)) {
			sctp_report_all_outbound(stcb, 0, 0, SCTP_SO_NOT_LOCKED);
		}
	}
	
	sctp_timer_stop(SCTP_TIMER_TYPE_SHUTDOWNACK, stcb->sctp_ep, stcb, net, SCTP_FROM_SCTP_INPUT+SCTP_LOC_22);
	SCTP_STAT_INCR_COUNTER32(sctps_shutdown);
	
	SCTPDBG(SCTP_DEBUG_INPUT2,
		"sctp_handle_shutdown_complete: calls free-asoc\n");
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
	so = SCTP_INP_SO(stcb->sctp_ep);
	atomic_add_int(&stcb->asoc.refcnt, 1);
	SCTP_TCB_UNLOCK(stcb);
	SCTP_SOCKET_LOCK(so, 1);
	SCTP_TCB_LOCK(stcb);
	atomic_subtract_int(&stcb->asoc.refcnt, 1);
#endif
	(void)sctp_free_assoc(stcb->sctp_ep, stcb, SCTP_NORMAL_PROC, SCTP_FROM_SCTP_INPUT+SCTP_LOC_23);
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
	SCTP_SOCKET_UNLOCK(so, 1);
#endif
	return;
}

static int
process_chunk_drop(struct sctp_tcb *stcb, struct sctp_chunk_desc *desc,
    struct sctp_nets *net, uint8_t flg)
{
	switch (desc->chunk_type) {
	case SCTP_DATA:
		
	{
		uint32_t tsn;
		struct sctp_tmit_chunk *tp1;

		tsn = ntohl(desc->tsn_ifany);
		TAILQ_FOREACH(tp1, &stcb->asoc.sent_queue, sctp_next) {
			if (tp1->rec.data.TSN_seq == tsn) {
				
				break;
			}
			if (SCTP_TSN_GT(tp1->rec.data.TSN_seq, tsn)) {
				
				tp1 = NULL;
				break;
			}
		}
		if (tp1 == NULL) {
			



			SCTP_STAT_INCR(sctps_pdrpdnfnd);
			TAILQ_FOREACH(tp1, &stcb->asoc.sent_queue, sctp_next) {
				if (tp1->rec.data.TSN_seq == tsn) {
					
					break;
				}
			}
		}
		if (tp1 == NULL) {
			SCTP_STAT_INCR(sctps_pdrptsnnf);
		}
		if ((tp1) && (tp1->sent < SCTP_DATAGRAM_ACKED)) {
			uint8_t *ddp;

			if (((flg & SCTP_BADCRC) == 0) &&
			    ((flg & SCTP_FROM_MIDDLE_BOX) == 0)) {
				return (0);
			}
			if ((stcb->asoc.peers_rwnd == 0) &&
			    ((flg & SCTP_FROM_MIDDLE_BOX) == 0)) {
				SCTP_STAT_INCR(sctps_pdrpdiwnp);
				return (0);
			}
			if (stcb->asoc.peers_rwnd == 0 &&
			    (flg & SCTP_FROM_MIDDLE_BOX)) {
				SCTP_STAT_INCR(sctps_pdrpdizrw);
				return (0);
			}
			ddp = (uint8_t *) (mtod(tp1->data, caddr_t) +
					   sizeof(struct sctp_data_chunk));
			{
				unsigned int iii;

				for (iii = 0; iii < sizeof(desc->data_bytes);
				     iii++) {
					if (ddp[iii] != desc->data_bytes[iii]) {
						SCTP_STAT_INCR(sctps_pdrpbadd);
						return (-1);
					}
				}
			}

			if (tp1->do_rtt) {
				



				if (tp1->whoTo->rto_needed == 0) {
					tp1->whoTo->rto_needed = 1;
				}
				tp1->do_rtt = 0;
			}
			SCTP_STAT_INCR(sctps_pdrpmark);
			if (tp1->sent != SCTP_DATAGRAM_RESEND)
				sctp_ucount_incr(stcb->asoc.sent_queue_retran_cnt);
			




			tp1->rec.data.doing_fast_retransmit = 1;
			



			if (TAILQ_EMPTY(&stcb->asoc.send_queue)) {
				tp1->rec.data.fast_retran_tsn = stcb->asoc.sending_seq;
			} else {
				tp1->rec.data.fast_retran_tsn = (TAILQ_FIRST(&stcb->asoc.send_queue))->rec.data.TSN_seq;
			}

			
			sctp_timer_stop(SCTP_TIMER_TYPE_SEND, stcb->sctp_ep,
					stcb, tp1->whoTo, SCTP_FROM_SCTP_INPUT+SCTP_LOC_24);
			sctp_timer_start(SCTP_TIMER_TYPE_SEND, stcb->sctp_ep,
					 stcb, tp1->whoTo);

			
			if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_FLIGHT_LOGGING_ENABLE) {
				sctp_misc_ints(SCTP_FLIGHT_LOG_DOWN_PDRP,
					       tp1->whoTo->flight_size,
					       tp1->book_size,
					       (uintptr_t)stcb,
					       tp1->rec.data.TSN_seq);
			}
			if (tp1->sent < SCTP_DATAGRAM_RESEND) {
				sctp_flight_size_decrease(tp1);
				sctp_total_flight_decrease(stcb, tp1);
			}
			tp1->sent = SCTP_DATAGRAM_RESEND;
		} {
			
			unsigned int audit;

			audit = 0;
			TAILQ_FOREACH(tp1, &stcb->asoc.sent_queue, sctp_next) {
				if (tp1->sent == SCTP_DATAGRAM_RESEND)
					audit++;
			}
			TAILQ_FOREACH(tp1, &stcb->asoc.control_send_queue,
				      sctp_next) {
				if (tp1->sent == SCTP_DATAGRAM_RESEND)
					audit++;
			}
			if (audit != stcb->asoc.sent_queue_retran_cnt) {
				SCTP_PRINTF("**Local Audit finds cnt:%d asoc cnt:%d\n",
					    audit, stcb->asoc.sent_queue_retran_cnt);
#ifndef SCTP_AUDITING_ENABLED
				stcb->asoc.sent_queue_retran_cnt = audit;
#endif
			}
		}
	}
	break;
	case SCTP_ASCONF:
	{
		struct sctp_tmit_chunk *asconf;

		TAILQ_FOREACH(asconf, &stcb->asoc.control_send_queue,
			      sctp_next) {
			if (asconf->rec.chunk_id.id == SCTP_ASCONF) {
				break;
			}
		}
		if (asconf) {
			if (asconf->sent != SCTP_DATAGRAM_RESEND)
				sctp_ucount_incr(stcb->asoc.sent_queue_retran_cnt);
			asconf->sent = SCTP_DATAGRAM_RESEND;
			asconf->snd_count--;
		}
	}
	break;
	case SCTP_INITIATION:
		
		stcb->asoc.dropped_special_cnt++;
		if (stcb->asoc.dropped_special_cnt < SCTP_RETRY_DROPPED_THRESH) {
			



			sctp_timer_stop(SCTP_TIMER_TYPE_INIT, stcb->sctp_ep,
					stcb, net, SCTP_FROM_SCTP_INPUT+SCTP_LOC_25);
			sctp_send_initiate(stcb->sctp_ep, stcb, SCTP_SO_NOT_LOCKED);
		}
		break;
	case SCTP_SELECTIVE_ACK:
	case SCTP_NR_SELECTIVE_ACK:
		
		sctp_send_sack(stcb, SCTP_SO_NOT_LOCKED);
		break;
	case SCTP_HEARTBEAT_REQUEST:
		
		if ((stcb->asoc.overall_error_count + 3) < stcb->asoc.max_send_times) {
			
			sctp_send_hb(stcb, net, SCTP_SO_NOT_LOCKED);
		}
		break;
	case SCTP_SHUTDOWN:
		sctp_send_shutdown(stcb, net);
		break;
	case SCTP_SHUTDOWN_ACK:
		sctp_send_shutdown_ack(stcb, net);
		break;
	case SCTP_COOKIE_ECHO:
	{
		struct sctp_tmit_chunk *cookie;

		cookie = NULL;
		TAILQ_FOREACH(cookie, &stcb->asoc.control_send_queue,
			      sctp_next) {
			if (cookie->rec.chunk_id.id == SCTP_COOKIE_ECHO) {
				break;
			}
		}
		if (cookie) {
			if (cookie->sent != SCTP_DATAGRAM_RESEND)
				sctp_ucount_incr(stcb->asoc.sent_queue_retran_cnt);
			cookie->sent = SCTP_DATAGRAM_RESEND;
			sctp_stop_all_cookie_timers(stcb);
		}
	}
	break;
	case SCTP_COOKIE_ACK:
		sctp_send_cookie_ack(stcb);
		break;
	case SCTP_ASCONF_ACK:
		
		sctp_send_asconf_ack(stcb);
		break;
	case SCTP_FORWARD_CUM_TSN:
		send_forward_tsn(stcb, &stcb->asoc);
		break;
		
	case SCTP_PACKET_DROPPED:
	case SCTP_INITIATION_ACK:	
	case SCTP_HEARTBEAT_ACK:
	case SCTP_ABORT_ASSOCIATION:
	case SCTP_OPERATION_ERROR:
	case SCTP_SHUTDOWN_COMPLETE:
	case SCTP_ECN_ECHO:
	case SCTP_ECN_CWR:
	default:
		break;
	}
	return (0);
}

void
sctp_reset_in_stream(struct sctp_tcb *stcb, uint32_t number_entries, uint16_t * list)
{
	uint32_t i;
	uint16_t temp;

	




	if (number_entries) {
		for (i = 0; i < number_entries; i++) {
			temp = ntohs(list[i]);
			if (temp >= stcb->asoc.streamincnt) {
				continue;
			}
			stcb->asoc.strmin[temp].last_sequence_delivered = 0xffff;
		}
	} else {
		list = NULL;
		for (i = 0; i < stcb->asoc.streamincnt; i++) {
			stcb->asoc.strmin[i].last_sequence_delivered = 0xffff;
		}
	}
	sctp_ulp_notify(SCTP_NOTIFY_STR_RESET_RECV, stcb, number_entries, (void *)list, SCTP_SO_NOT_LOCKED);
}

static void
sctp_reset_out_streams(struct sctp_tcb *stcb, int number_entries, uint16_t * list)
{
	int i;

	if (number_entries == 0) {
		for (i = 0; i < stcb->asoc.streamoutcnt; i++) {
			stcb->asoc.strmout[i].next_sequence_send = 0;
		}
	} else if (number_entries) {
		for (i = 0; i < number_entries; i++) {
			uint16_t temp;

			temp = ntohs(list[i]);
			if (temp >= stcb->asoc.streamoutcnt) {
				
				continue;
			}
			stcb->asoc.strmout[temp].next_sequence_send = 0;
		}
	}
	sctp_ulp_notify(SCTP_NOTIFY_STR_RESET_SEND, stcb, number_entries, (void *)list, SCTP_SO_NOT_LOCKED);
}


struct sctp_stream_reset_out_request *
sctp_find_stream_reset(struct sctp_tcb *stcb, uint32_t seq, struct sctp_tmit_chunk **bchk)
{
	struct sctp_association *asoc;
	struct sctp_chunkhdr *ch;
	struct sctp_stream_reset_out_request *r;
	struct sctp_tmit_chunk *chk;
	int len, clen;

	asoc = &stcb->asoc;
	if (TAILQ_EMPTY(&stcb->asoc.control_send_queue)) {
		asoc->stream_reset_outstanding = 0;
		return (NULL);
	}
	if (stcb->asoc.str_reset == NULL) {
		asoc->stream_reset_outstanding = 0;
		return (NULL);
	}
	chk = stcb->asoc.str_reset;
	if (chk->data == NULL) {
		return (NULL);
	}
	if (bchk) {
		
		*bchk = chk;
	}
	clen = chk->send_size;
	ch = mtod(chk->data, struct sctp_chunkhdr *);
	r = (struct sctp_stream_reset_out_request *)(ch + 1);
	if (ntohl(r->request_seq) == seq) {
		
		return (r);
	}
	len = SCTP_SIZE32(ntohs(r->ph.param_length));
	if (clen > (len + (int)sizeof(struct sctp_chunkhdr))) {
		
		r = (struct sctp_stream_reset_out_request *)((caddr_t)r + len);
		if (ntohl(r->request_seq) == seq) {
			return (r);
		}
	}
	
	return (NULL);
}

static void
sctp_clean_up_stream_reset(struct sctp_tcb *stcb)
{
	struct sctp_association *asoc;
	struct sctp_tmit_chunk *chk = stcb->asoc.str_reset;

	if (stcb->asoc.str_reset == NULL) {
		return;
	}
	asoc = &stcb->asoc;

	sctp_timer_stop(SCTP_TIMER_TYPE_STRRESET, stcb->sctp_ep, stcb, chk->whoTo, SCTP_FROM_SCTP_INPUT+SCTP_LOC_26);
	TAILQ_REMOVE(&asoc->control_send_queue,
	    chk,
	    sctp_next);
	if (chk->data) {
		sctp_m_freem(chk->data);
		chk->data = NULL;
	}
	asoc->ctrl_queue_cnt--;
	sctp_free_a_chunk(stcb, chk, SCTP_SO_NOT_LOCKED);
        
	stcb->asoc.str_reset = NULL;
}


static int
sctp_handle_stream_reset_response(struct sctp_tcb *stcb,
				  uint32_t seq, uint32_t action,
				  struct sctp_stream_reset_response *respin)
{
	uint16_t type;
	int lparm_len;
	struct sctp_association *asoc = &stcb->asoc;
	struct sctp_tmit_chunk *chk;
	struct sctp_stream_reset_out_request *srparam;
	int number_entries;

	if (asoc->stream_reset_outstanding == 0) {
		
		return (0);
	}
	if (seq == stcb->asoc.str_reset_seq_out) {
		srparam = sctp_find_stream_reset(stcb, seq, &chk);
		if (srparam) {
			stcb->asoc.str_reset_seq_out++;
			type = ntohs(srparam->ph.param_type);
			lparm_len = ntohs(srparam->ph.param_length);
			if (type == SCTP_STR_RESET_OUT_REQUEST) {
				number_entries = (lparm_len - sizeof(struct sctp_stream_reset_out_request)) / sizeof(uint16_t);
				asoc->stream_reset_out_is_outstanding = 0;
				if (asoc->stream_reset_outstanding)
					asoc->stream_reset_outstanding--;
				if (action == SCTP_STREAM_RESET_RESULT_PERFORMED) {
					
					sctp_reset_out_streams(stcb, number_entries, srparam->list_of_streams);
				} else if (action == SCTP_STREAM_RESET_RESULT_DENIED) {
					sctp_ulp_notify(SCTP_NOTIFY_STR_RESET_DENIED_OUT, stcb, number_entries, srparam->list_of_streams, SCTP_SO_NOT_LOCKED);
				} else {
					sctp_ulp_notify(SCTP_NOTIFY_STR_RESET_FAILED_OUT, stcb, number_entries, srparam->list_of_streams, SCTP_SO_NOT_LOCKED);
				}
			} else if (type == SCTP_STR_RESET_IN_REQUEST) {
				
				number_entries = (lparm_len - sizeof(struct sctp_stream_reset_in_request)) / sizeof(uint16_t);
				if (asoc->stream_reset_outstanding)
					asoc->stream_reset_outstanding--;
				if (action == SCTP_STREAM_RESET_RESULT_DENIED) {
					sctp_ulp_notify(SCTP_NOTIFY_STR_RESET_DENIED_IN, stcb,
							number_entries, srparam->list_of_streams, SCTP_SO_NOT_LOCKED);
				} else if (action != SCTP_STREAM_RESET_RESULT_PERFORMED) {
					sctp_ulp_notify(SCTP_NOTIFY_STR_RESET_FAILED_IN, stcb,
							number_entries, srparam->list_of_streams, SCTP_SO_NOT_LOCKED);
				}
			} else if (type == SCTP_STR_RESET_ADD_OUT_STREAMS) {
				
				int num_stream;

				num_stream = stcb->asoc.strm_pending_add_size;
				if (num_stream > (stcb->asoc.strm_realoutsize - stcb->asoc.streamoutcnt)) {
					
					num_stream = stcb->asoc.strm_realoutsize - stcb->asoc.streamoutcnt;
				}
				stcb->asoc.strm_pending_add_size = 0;
				if (asoc->stream_reset_outstanding)
					asoc->stream_reset_outstanding--;
				if (action == SCTP_STREAM_RESET_RESULT_PERFORMED) {
					
					stcb->asoc.streamoutcnt += num_stream;
					sctp_notify_stream_reset_add(stcb, stcb->asoc.streamincnt, stcb->asoc.streamoutcnt, 0);
				} else if (action == SCTP_STREAM_RESET_RESULT_DENIED) {
					sctp_notify_stream_reset_add(stcb, stcb->asoc.streamincnt, stcb->asoc.streamoutcnt,
								     SCTP_STREAM_CHANGE_DENIED);
				} else {
					sctp_notify_stream_reset_add(stcb, stcb->asoc.streamincnt, stcb->asoc.streamoutcnt,
								     SCTP_STREAM_CHANGE_FAILED);
				}
			} else if (type == SCTP_STR_RESET_ADD_IN_STREAMS) {
				if (asoc->stream_reset_outstanding)
					asoc->stream_reset_outstanding--;
				if (action == SCTP_STREAM_RESET_RESULT_DENIED) {
					sctp_notify_stream_reset_add(stcb, stcb->asoc.streamincnt, stcb->asoc.streamoutcnt,
								     SCTP_STREAM_CHANGE_DENIED);
				} else if (action != SCTP_STREAM_RESET_RESULT_PERFORMED) {
					sctp_notify_stream_reset_add(stcb, stcb->asoc.streamincnt, stcb->asoc.streamoutcnt,
								     SCTP_STREAM_CHANGE_FAILED);
				}
			} else if (type == SCTP_STR_RESET_TSN_REQUEST) {
				




				struct sctp_stream_reset_response_tsn *resp;
				struct sctp_forward_tsn_chunk fwdtsn;
				int abort_flag = 0;
				if (respin == NULL) {
					
					return (0);
				}
				if (action == SCTP_STREAM_RESET_RESULT_PERFORMED) {
					resp = (struct sctp_stream_reset_response_tsn *)respin;
					asoc->stream_reset_outstanding--;
					fwdtsn.ch.chunk_length = htons(sizeof(struct sctp_forward_tsn_chunk));
					fwdtsn.ch.chunk_type = SCTP_FORWARD_CUM_TSN;
					fwdtsn.new_cumulative_tsn = htonl(ntohl(resp->senders_next_tsn) - 1);
					sctp_handle_forward_tsn(stcb, &fwdtsn, &abort_flag, NULL, 0);
					if (abort_flag) {
						return (1);
					}
					stcb->asoc.highest_tsn_inside_map = (ntohl(resp->senders_next_tsn) - 1);
					if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_MAP_LOGGING_ENABLE) {
						sctp_log_map(0, 7, asoc->highest_tsn_inside_map, SCTP_MAP_SLIDE_RESULT);
					}

					stcb->asoc.tsn_last_delivered = stcb->asoc.cumulative_tsn = stcb->asoc.highest_tsn_inside_map;
					stcb->asoc.mapping_array_base_tsn = ntohl(resp->senders_next_tsn);
					memset(stcb->asoc.mapping_array, 0, stcb->asoc.mapping_array_size);

					stcb->asoc.highest_tsn_inside_nr_map = stcb->asoc.highest_tsn_inside_map;
					memset(stcb->asoc.nr_mapping_array, 0, stcb->asoc.mapping_array_size);

					stcb->asoc.sending_seq = ntohl(resp->receivers_next_tsn);
					stcb->asoc.last_acked_seq = stcb->asoc.cumulative_tsn;

					sctp_reset_out_streams(stcb, 0, (uint16_t *) NULL);
					sctp_reset_in_stream(stcb, 0, (uint16_t *) NULL);
					sctp_notify_stream_reset_tsn(stcb, stcb->asoc.sending_seq, (stcb->asoc.mapping_array_base_tsn + 1), 0);
				} else if (action == SCTP_STREAM_RESET_RESULT_DENIED) {
					sctp_notify_stream_reset_tsn(stcb, stcb->asoc.sending_seq, (stcb->asoc.mapping_array_base_tsn + 1),
								     SCTP_ASSOC_RESET_DENIED);
				} else {
					sctp_notify_stream_reset_tsn(stcb, stcb->asoc.sending_seq, (stcb->asoc.mapping_array_base_tsn + 1),
								     SCTP_ASSOC_RESET_FAILED);
				}
			}
			
			if (asoc->stream_reset_outstanding == 0) {
				sctp_clean_up_stream_reset(stcb);
			}
		}
	}
	return (0);
}

static void
sctp_handle_str_reset_request_in(struct sctp_tcb *stcb,
    struct sctp_tmit_chunk *chk,
    struct sctp_stream_reset_in_request *req, int trunc)
{
	uint32_t seq;
	int len, i;
	int number_entries;
	uint16_t temp;

	



	struct sctp_association *asoc = &stcb->asoc;

	seq = ntohl(req->request_seq);
	if (asoc->str_reset_seq_in == seq) {
		asoc->last_reset_action[1] = asoc->last_reset_action[0];
		if (!(asoc->local_strreset_support & SCTP_ENABLE_RESET_STREAM_REQ)) {
			asoc->last_reset_action[0] = SCTP_STREAM_RESET_RESULT_DENIED;
		} else if (trunc) {
			
			asoc->last_reset_action[0] = SCTP_STREAM_RESET_RESULT_DENIED;
		} else if (stcb->asoc.stream_reset_out_is_outstanding == 0) {
			len = ntohs(req->ph.param_length);
			number_entries = ((len - sizeof(struct sctp_stream_reset_in_request)) / sizeof(uint16_t));
			for (i = 0; i < number_entries; i++) {
				temp = ntohs(req->list_of_streams[i]);
				req->list_of_streams[i] = temp;
			}
			asoc->last_reset_action[0] = SCTP_STREAM_RESET_RESULT_PERFORMED;
			sctp_add_stream_reset_out(chk, number_entries, req->list_of_streams,
			    asoc->str_reset_seq_out,
			    seq, (asoc->sending_seq - 1));
			asoc->stream_reset_out_is_outstanding = 1;
			asoc->str_reset = chk;
			sctp_timer_start(SCTP_TIMER_TYPE_STRRESET, stcb->sctp_ep, stcb, chk->whoTo);
			stcb->asoc.stream_reset_outstanding++;
		} else {
			
			asoc->last_reset_action[0] = SCTP_STREAM_RESET_RESULT_ERR_IN_PROGRESS;
		}
		sctp_add_stream_reset_result(chk, seq, asoc->last_reset_action[0]);
		asoc->str_reset_seq_in++;
	} else if (asoc->str_reset_seq_in - 1 == seq) {
		sctp_add_stream_reset_result(chk, seq, asoc->last_reset_action[0]);
	} else if (asoc->str_reset_seq_in - 2 == seq) {
		sctp_add_stream_reset_result(chk, seq, asoc->last_reset_action[1]);
	} else {
		sctp_add_stream_reset_result(chk, seq, SCTP_STREAM_RESET_RESULT_ERR_BAD_SEQNO);
	}
}

static int
sctp_handle_str_reset_request_tsn(struct sctp_tcb *stcb,
    struct sctp_tmit_chunk *chk,
    struct sctp_stream_reset_tsn_request *req)
{
	
	




	struct sctp_forward_tsn_chunk fwdtsn;
	struct sctp_association *asoc = &stcb->asoc;
	int abort_flag = 0;
	uint32_t seq;

	seq = ntohl(req->request_seq);
	if (asoc->str_reset_seq_in == seq) {
		asoc->last_reset_action[1] = stcb->asoc.last_reset_action[0];
		if (!(asoc->local_strreset_support & SCTP_ENABLE_CHANGE_ASSOC_REQ)) {
			asoc->last_reset_action[0] = SCTP_STREAM_RESET_RESULT_DENIED;
		} else {
			fwdtsn.ch.chunk_length = htons(sizeof(struct sctp_forward_tsn_chunk));
			fwdtsn.ch.chunk_type = SCTP_FORWARD_CUM_TSN;
			fwdtsn.ch.chunk_flags = 0;
			fwdtsn.new_cumulative_tsn = htonl(stcb->asoc.highest_tsn_inside_map + 1);
			sctp_handle_forward_tsn(stcb, &fwdtsn, &abort_flag, NULL, 0);
			if (abort_flag) {
				return (1);
			}
			asoc->highest_tsn_inside_map += SCTP_STREAM_RESET_TSN_DELTA;
			if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_MAP_LOGGING_ENABLE) {
				sctp_log_map(0, 10, asoc->highest_tsn_inside_map, SCTP_MAP_SLIDE_RESULT);
			}
			asoc->tsn_last_delivered = asoc->cumulative_tsn = asoc->highest_tsn_inside_map;
			asoc->mapping_array_base_tsn = asoc->highest_tsn_inside_map + 1;
			memset(asoc->mapping_array, 0, asoc->mapping_array_size);
			asoc->highest_tsn_inside_nr_map = asoc->highest_tsn_inside_map;
			memset(asoc->nr_mapping_array, 0, asoc->mapping_array_size);
			atomic_add_int(&asoc->sending_seq, 1);
			
			asoc->last_sending_seq[1] = asoc->last_sending_seq[0];
			asoc->last_sending_seq[0] = asoc->sending_seq;
			asoc->last_base_tsnsent[1] = asoc->last_base_tsnsent[0];
			asoc->last_base_tsnsent[0] = asoc->mapping_array_base_tsn;
			sctp_reset_out_streams(stcb, 0, (uint16_t *) NULL);
			sctp_reset_in_stream(stcb, 0, (uint16_t *) NULL);
			asoc->last_reset_action[0] = SCTP_STREAM_RESET_RESULT_PERFORMED;
			sctp_notify_stream_reset_tsn(stcb, asoc->sending_seq, (asoc->mapping_array_base_tsn + 1), 0);
		}
		sctp_add_stream_reset_result_tsn(chk, seq, asoc->last_reset_action[0],
		                                 asoc->last_sending_seq[0], asoc->last_base_tsnsent[0]);
		asoc->str_reset_seq_in++;
	} else if (asoc->str_reset_seq_in - 1 == seq) {
		sctp_add_stream_reset_result_tsn(chk, seq, asoc->last_reset_action[0],
		                                 asoc->last_sending_seq[0], asoc->last_base_tsnsent[0]);
	} else if (asoc->str_reset_seq_in - 2 == seq) {
		sctp_add_stream_reset_result_tsn(chk, seq, asoc->last_reset_action[1],
		                                 asoc->last_sending_seq[1], asoc->last_base_tsnsent[1]);
	} else {
		sctp_add_stream_reset_result(chk, seq, SCTP_STREAM_RESET_RESULT_ERR_BAD_SEQNO);
	}
	return (0);
}

static void
sctp_handle_str_reset_request_out(struct sctp_tcb *stcb,
    struct sctp_tmit_chunk *chk,
    struct sctp_stream_reset_out_request *req, int trunc)
{
	uint32_t seq, tsn;
	int number_entries, len;
	struct sctp_association *asoc = &stcb->asoc;

	seq = ntohl(req->request_seq);

	
	if (asoc->str_reset_seq_in == seq) {
		len = ntohs(req->ph.param_length);
		number_entries = ((len - sizeof(struct sctp_stream_reset_out_request)) / sizeof(uint16_t));
		






		tsn = ntohl(req->send_reset_at_tsn);

		
		asoc->last_reset_action[1] = asoc->last_reset_action[0];
		if (!(asoc->local_strreset_support & SCTP_ENABLE_RESET_STREAM_REQ)) {
			asoc->last_reset_action[0] = SCTP_STREAM_RESET_RESULT_DENIED;
		} else if (trunc) {
			asoc->last_reset_action[0] = SCTP_STREAM_RESET_RESULT_DENIED;
		} else if (SCTP_TSN_GE(asoc->cumulative_tsn, tsn)) {
			
			sctp_reset_in_stream(stcb, number_entries, req->list_of_streams);
			asoc->last_reset_action[0] = SCTP_STREAM_RESET_RESULT_PERFORMED;
		} else {
			



			struct sctp_stream_reset_list *liste;
			int siz;

			siz = sizeof(struct sctp_stream_reset_list) + (number_entries * sizeof(uint16_t));
			SCTP_MALLOC(liste, struct sctp_stream_reset_list *,
				    siz, SCTP_M_STRESET);
			if (liste == NULL) {
				
				asoc->last_reset_action[0] = SCTP_STREAM_RESET_RESULT_DENIED;
				sctp_add_stream_reset_result(chk, seq, asoc->last_reset_action[0]);
				return;
			}
			liste->tsn = tsn;
			liste->number_entries = number_entries;
			memcpy(&liste->list_of_streams, req->list_of_streams, number_entries * sizeof(uint16_t));
			TAILQ_INSERT_TAIL(&asoc->resetHead, liste, next_resp);
			asoc->last_reset_action[0] = SCTP_STREAM_RESET_RESULT_PERFORMED;
		}
		sctp_add_stream_reset_result(chk, seq, asoc->last_reset_action[0]);
		asoc->str_reset_seq_in++;
	} else if ((asoc->str_reset_seq_in - 1) == seq) {
		



		sctp_add_stream_reset_result(chk, seq, asoc->last_reset_action[0]);
	} else if ((asoc->str_reset_seq_in - 2) == seq) {
		



		sctp_add_stream_reset_result(chk, seq, asoc->last_reset_action[1]);
	} else {
		sctp_add_stream_reset_result(chk, seq, SCTP_STREAM_RESET_RESULT_ERR_BAD_SEQNO);
	}
}

static void
sctp_handle_str_reset_add_strm(struct sctp_tcb *stcb, struct sctp_tmit_chunk *chk,
			       struct sctp_stream_reset_add_strm  *str_add)
{
	




	uint32_t num_stream, i;
	uint32_t seq;
	struct sctp_association *asoc = &stcb->asoc;
	struct sctp_queued_to_read *ctl, *nctl;

	
	seq = ntohl(str_add->request_seq);
	num_stream = ntohs(str_add->number_of_streams);
	
	if (asoc->str_reset_seq_in == seq) {
		num_stream += stcb->asoc.streamincnt;
		stcb->asoc.last_reset_action[1] = stcb->asoc.last_reset_action[0];
		if (!(asoc->local_strreset_support & SCTP_ENABLE_CHANGE_ASSOC_REQ)) {
			asoc->last_reset_action[0] = SCTP_STREAM_RESET_RESULT_DENIED;
		} else if ((num_stream > stcb->asoc.max_inbound_streams) ||
		    (num_stream > 0xffff)) {
			
  denied:
			stcb->asoc.last_reset_action[0] = SCTP_STREAM_RESET_RESULT_DENIED;
		} else {
			
			struct sctp_stream_in *oldstrm;

			
			oldstrm = stcb->asoc.strmin;
			SCTP_MALLOC(stcb->asoc.strmin, struct sctp_stream_in *,
			            (num_stream * sizeof(struct sctp_stream_in)),
			            SCTP_M_STRMI);
			if (stcb->asoc.strmin == NULL) {
				stcb->asoc.strmin = oldstrm;
				goto denied;
			}
			
			for (i = 0; i < stcb->asoc.streamincnt; i++) {
				TAILQ_INIT(&stcb->asoc.strmin[i].inqueue);
				stcb->asoc.strmin[i].stream_no = i;
				stcb->asoc.strmin[i].last_sequence_delivered = oldstrm[i].last_sequence_delivered;
				stcb->asoc.strmin[i].delivery_started = oldstrm[i].delivery_started;
				
				TAILQ_FOREACH_SAFE(ctl, &oldstrm[i].inqueue, next, nctl) {
					TAILQ_REMOVE(&oldstrm[i].inqueue, ctl, next);
					TAILQ_INSERT_TAIL(&stcb->asoc.strmin[i].inqueue, ctl, next);
				}
			}
			
			for (i = stcb->asoc.streamincnt; i < num_stream; i++) {
				TAILQ_INIT(&stcb->asoc.strmin[i].inqueue);
				stcb->asoc.strmin[i].stream_no = i;
				stcb->asoc.strmin[i].last_sequence_delivered = 0xffff;
				stcb->asoc.strmin[i].delivery_started = 0;
			}
			SCTP_FREE(oldstrm, SCTP_M_STRMI);
			
			stcb->asoc.streamincnt = num_stream;
			stcb->asoc.last_reset_action[0] = SCTP_STREAM_RESET_RESULT_PERFORMED;
			sctp_notify_stream_reset_add(stcb, stcb->asoc.streamincnt, stcb->asoc.streamoutcnt, 0);
		}
		sctp_add_stream_reset_result(chk, seq, asoc->last_reset_action[0]);
		asoc->str_reset_seq_in++;
	} else if ((asoc->str_reset_seq_in - 1) == seq) {
		



		sctp_add_stream_reset_result(chk, seq, asoc->last_reset_action[0]);
	} else if ((asoc->str_reset_seq_in - 2) == seq) {
		



		sctp_add_stream_reset_result(chk, seq, asoc->last_reset_action[1]);
	} else {
		sctp_add_stream_reset_result(chk, seq, SCTP_STREAM_RESET_RESULT_ERR_BAD_SEQNO);

	}
}

static void
sctp_handle_str_reset_add_out_strm(struct sctp_tcb *stcb, struct sctp_tmit_chunk *chk,
				   struct sctp_stream_reset_add_strm  *str_add)
{
	




	uint16_t num_stream;
	uint32_t seq;
	struct sctp_association *asoc = &stcb->asoc;

	
	seq = ntohl(str_add->request_seq);
	num_stream = ntohs(str_add->number_of_streams);
	
	if (asoc->str_reset_seq_in == seq) {
		stcb->asoc.last_reset_action[1] = stcb->asoc.last_reset_action[0];
		if (!(asoc->local_strreset_support & SCTP_ENABLE_CHANGE_ASSOC_REQ)) {
			asoc->last_reset_action[0] = SCTP_STREAM_RESET_RESULT_DENIED;
		} else if (stcb->asoc.stream_reset_outstanding) {
			
			stcb->asoc.last_reset_action[0] = SCTP_STREAM_RESET_RESULT_ERR_IN_PROGRESS;
		} else {
			
			int mychk;
			mychk = stcb->asoc.streamoutcnt;
			mychk += num_stream;
			if (mychk < 0x10000) {
				stcb->asoc.last_reset_action[0] = SCTP_STREAM_RESET_RESULT_PERFORMED;
				if (sctp_send_str_reset_req(stcb, 0, NULL, 0, 0, 0, 1, num_stream, 0, 1)) {
					stcb->asoc.last_reset_action[0] = SCTP_STREAM_RESET_RESULT_DENIED;
				}
			} else {
				stcb->asoc.last_reset_action[0] = SCTP_STREAM_RESET_RESULT_DENIED;
			}
		}
		sctp_add_stream_reset_result(chk, seq, stcb->asoc.last_reset_action[0]);
		asoc->str_reset_seq_in++;
	} else if ((asoc->str_reset_seq_in - 1) == seq) {
		



		sctp_add_stream_reset_result(chk, seq, asoc->last_reset_action[0]);
	} else if ((asoc->str_reset_seq_in - 2) == seq) {
		



		sctp_add_stream_reset_result(chk, seq, asoc->last_reset_action[1]);
	} else {
		sctp_add_stream_reset_result(chk, seq, SCTP_STREAM_RESET_RESULT_ERR_BAD_SEQNO);
	}
}

#if !defined(__Panda__)
#ifdef __GNUC__
__attribute__ ((noinline))
#endif
#endif
static int
sctp_handle_stream_reset(struct sctp_tcb *stcb, struct mbuf *m, int offset,
			 struct sctp_chunkhdr *ch_req)
{
	int chk_length, param_len, ptype;
	struct sctp_paramhdr pstore;
	uint8_t cstore[SCTP_CHUNK_BUFFER_SIZE];
	uint32_t seq = 0;
	int num_req = 0;
	int trunc = 0;
	struct sctp_tmit_chunk *chk;
	struct sctp_chunkhdr *ch;
	struct sctp_paramhdr *ph;
	int ret_code = 0;
	int num_param = 0;

	
	chk_length = ntohs(ch_req->chunk_length);

	
	sctp_alloc_a_chunk(stcb, chk);
	if (chk == NULL) {
		return (ret_code);
	}
	chk->rec.chunk_id.id = SCTP_STREAM_RESET;
	chk->rec.chunk_id.can_take_data = 0;
	chk->asoc = &stcb->asoc;
	chk->no_fr_allowed = 0;
	chk->book_size = chk->send_size = sizeof(struct sctp_chunkhdr);
	chk->book_size_scale = 0;
	chk->data = sctp_get_mbuf_for_msg(MCLBYTES, 0, M_NOWAIT, 1, MT_DATA);
	if (chk->data == NULL) {
	strres_nochunk:
		if (chk->data) {
			sctp_m_freem(chk->data);
			chk->data = NULL;
		}
		sctp_free_a_chunk(stcb, chk, SCTP_SO_NOT_LOCKED);
		return (ret_code);
	}
	SCTP_BUF_RESV_UF(chk->data, SCTP_MIN_OVERHEAD);

	
	chk->sent = SCTP_DATAGRAM_UNSENT;
	chk->snd_count = 0;
	chk->whoTo = NULL;

	ch = mtod(chk->data, struct sctp_chunkhdr *);
	ch->chunk_type = SCTP_STREAM_RESET;
	ch->chunk_flags = 0;
	ch->chunk_length = htons(chk->send_size);
	SCTP_BUF_LEN(chk->data) = SCTP_SIZE32(chk->send_size);
	offset += sizeof(struct sctp_chunkhdr);
	while ((size_t)chk_length >= sizeof(struct sctp_stream_reset_tsn_request)) {
		ph = (struct sctp_paramhdr *)sctp_m_getptr(m, offset, sizeof(pstore), (uint8_t *)&pstore);
		if (ph == NULL)
			break;
		param_len = ntohs(ph->param_length);
		if (param_len < (int)sizeof(struct sctp_stream_reset_tsn_request)) {
			
			break;
		}
		ph = (struct sctp_paramhdr *)sctp_m_getptr(m, offset, min(param_len, (int)sizeof(cstore)),
							   (uint8_t *)&cstore);
		ptype = ntohs(ph->param_type);
		num_param++;
		if (param_len > (int)sizeof(cstore)) {
			trunc = 1;
		} else {
			trunc = 0;
		}
		if (num_param > SCTP_MAX_RESET_PARAMS) {
			
			break;
		}
		if (ptype == SCTP_STR_RESET_OUT_REQUEST) {
			struct sctp_stream_reset_out_request *req_out;
			req_out = (struct sctp_stream_reset_out_request *)ph;
			num_req++;
			if (stcb->asoc.stream_reset_outstanding) {
				seq = ntohl(req_out->response_seq);
				if (seq == stcb->asoc.str_reset_seq_out) {
					
					(void)sctp_handle_stream_reset_response(stcb, seq, SCTP_STREAM_RESET_RESULT_PERFORMED, NULL);
				}
			}
			sctp_handle_str_reset_request_out(stcb, chk, req_out, trunc);
		} else if (ptype == SCTP_STR_RESET_ADD_OUT_STREAMS) {
			struct sctp_stream_reset_add_strm  *str_add;
			str_add = (struct sctp_stream_reset_add_strm  *)ph;
			num_req++;
			sctp_handle_str_reset_add_strm(stcb, chk, str_add);
		} else if (ptype == SCTP_STR_RESET_ADD_IN_STREAMS) {
			struct sctp_stream_reset_add_strm  *str_add;
			str_add = (struct sctp_stream_reset_add_strm  *)ph;
			num_req++;
			sctp_handle_str_reset_add_out_strm(stcb, chk, str_add);
		} else if (ptype == SCTP_STR_RESET_IN_REQUEST) {
			struct sctp_stream_reset_in_request *req_in;
			num_req++;
			req_in = (struct sctp_stream_reset_in_request *)ph;
			sctp_handle_str_reset_request_in(stcb, chk, req_in, trunc);
		} else if (ptype == SCTP_STR_RESET_TSN_REQUEST) {
			struct sctp_stream_reset_tsn_request *req_tsn;
			num_req++;
			req_tsn = (struct sctp_stream_reset_tsn_request *)ph;
			if (sctp_handle_str_reset_request_tsn(stcb, chk, req_tsn)) {
				ret_code = 1;
				goto strres_nochunk;
			}
			
			break;
		} else if (ptype == SCTP_STR_RESET_RESPONSE) {
			struct sctp_stream_reset_response *resp;
			uint32_t result;
			resp = (struct sctp_stream_reset_response *)ph;
			seq = ntohl(resp->response_seq);
			result = ntohl(resp->result);
			if (sctp_handle_stream_reset_response(stcb, seq, result, resp)) {
				ret_code = 1;
				goto strres_nochunk;
			}
		} else {
			break;
		}
		offset += SCTP_SIZE32(param_len);
		chk_length -= SCTP_SIZE32(param_len);
	}
	if (num_req == 0) {
		
		goto strres_nochunk;
	}
	
	TAILQ_INSERT_TAIL(&stcb->asoc.control_send_queue,
			  chk,
			  sctp_next);
	stcb->asoc.ctrl_queue_cnt++;
	return (ret_code);
}







static void
sctp_handle_packet_dropped(struct sctp_pktdrop_chunk *cp,
    struct sctp_tcb *stcb, struct sctp_nets *net, uint32_t limit)
{
	uint32_t bottle_bw, on_queue;
	uint16_t trunc_len;
	unsigned int chlen;
	unsigned int at;
	struct sctp_chunk_desc desc;
	struct sctp_chunkhdr *ch;

	chlen = ntohs(cp->ch.chunk_length);
	chlen -= sizeof(struct sctp_pktdrop_chunk);
	
	if (chlen == 0) {
		ch = NULL;
		if (cp->ch.chunk_flags & SCTP_FROM_MIDDLE_BOX)
			SCTP_STAT_INCR(sctps_pdrpbwrpt);
	} else {
		ch = (struct sctp_chunkhdr *)(cp->data + sizeof(struct sctphdr));
		chlen -= sizeof(struct sctphdr);
		
		memset(&desc, 0, sizeof(desc));
	}
	trunc_len = (uint16_t) ntohs(cp->trunc_len);
	if (trunc_len > limit) {
		trunc_len = limit;
	}

	
	while ((ch != NULL) && (chlen >= sizeof(struct sctp_chunkhdr))) {
		desc.chunk_type = ch->chunk_type;
		
		at = ntohs(ch->chunk_length);
		if (at < sizeof(struct sctp_chunkhdr)) {
			
			SCTP_STAT_INCR(sctps_pdrpcrupt);
			break;
		}
		if (trunc_len == 0) {
			
			if (at > chlen) {
				
				SCTP_STAT_INCR(sctps_pdrpcrupt);
				break;
			}
		} else {
			
			if (desc.chunk_type == SCTP_DATA) {
				if (chlen < (sizeof(struct sctp_data_chunk) +
				    sizeof(desc.data_bytes))) {
					break;
				}
			} else {
				if (chlen < sizeof(struct sctp_chunkhdr)) {
					break;
				}
			}
		}
		if (desc.chunk_type == SCTP_DATA) {
			
			if ((cp->ch.chunk_flags & SCTP_FROM_MIDDLE_BOX))
				SCTP_STAT_INCR(sctps_pdrpmbda);

			if (chlen >= (sizeof(struct sctp_data_chunk) + sizeof(uint32_t))) {
				
				struct sctp_data_chunk *dcp;
				uint8_t *ddp;
				unsigned int iii;

				dcp = (struct sctp_data_chunk *)ch;
				ddp = (uint8_t *) (dcp + 1);
				for (iii = 0; iii < sizeof(desc.data_bytes); iii++) {
					desc.data_bytes[iii] = ddp[iii];
				}
				desc.tsn_ifany = dcp->dp.tsn;
			} else {
				
				SCTP_STAT_INCR(sctps_pdrpnedat);
				break;
			}
		} else {
			if ((cp->ch.chunk_flags & SCTP_FROM_MIDDLE_BOX))
				SCTP_STAT_INCR(sctps_pdrpmbct);
		}

		if (process_chunk_drop(stcb, &desc, net, cp->ch.chunk_flags)) {
			SCTP_STAT_INCR(sctps_pdrppdbrk);
			break;
		}
		if (SCTP_SIZE32(at) > chlen) {
			break;
		}
		chlen -= SCTP_SIZE32(at);
		if (chlen < sizeof(struct sctp_chunkhdr)) {
			
			break;
		}
		ch = (struct sctp_chunkhdr *)((caddr_t)ch + SCTP_SIZE32(at));
	}
	
	if ((cp->ch.chunk_flags & SCTP_FROM_MIDDLE_BOX) == 0) {
		
		uint32_t a_rwnd;

		SCTP_STAT_INCR(sctps_pdrpfehos);

		bottle_bw = ntohl(cp->bottle_bw);
		on_queue = ntohl(cp->current_onq);
		if (bottle_bw && on_queue) {
			
			if (bottle_bw > on_queue)
				a_rwnd = bottle_bw - on_queue;
			else
				a_rwnd = 0;

			if (a_rwnd == 0)
				stcb->asoc.peers_rwnd = 0;
			else {
				if (a_rwnd > stcb->asoc.total_flight) {
					stcb->asoc.peers_rwnd =
					    a_rwnd - stcb->asoc.total_flight;
				} else {
					stcb->asoc.peers_rwnd = 0;
				}
				if (stcb->asoc.peers_rwnd <
				    stcb->sctp_ep->sctp_ep.sctp_sws_sender) {
					
					stcb->asoc.peers_rwnd = 0;
				}
			}
		}
	} else {
		SCTP_STAT_INCR(sctps_pdrpfmbox);
	}

	
	if ((cp->ch.chunk_flags & SCTP_FROM_MIDDLE_BOX) &&
	    (stcb->asoc.sat_t3_loss_recovery == 0) &&
	    (stcb->asoc.sat_network)) {
		




		stcb->asoc.cc_functions.sctp_cwnd_update_after_packet_dropped(stcb,
			net, cp, &bottle_bw, &on_queue);
	}
}










#if !defined(__Panda__)
#ifdef __GNUC__
__attribute__ ((noinline))
#endif
#endif
static struct sctp_tcb *
sctp_process_control(struct mbuf *m, int iphlen, int *offset, int length,
    struct sockaddr *src, struct sockaddr *dst,
    struct sctphdr *sh, struct sctp_chunkhdr *ch, struct sctp_inpcb *inp,
    struct sctp_tcb *stcb, struct sctp_nets **netp, int *fwd_tsn_seen,
#if defined(__FreeBSD__)
    uint8_t use_mflowid, uint32_t mflowid,
#endif
    uint32_t vrf_id, uint16_t port)
{
	struct sctp_association *asoc;
	uint32_t vtag_in;
	int num_chunks = 0;	
	uint32_t chk_length;
	int ret;
	int abort_no_unlock = 0;
	int ecne_seen = 0;
	




	uint8_t chunk_buf[SCTP_CHUNK_BUFFER_SIZE];
	struct sctp_tcb *locked_tcb = stcb;
	int got_auth = 0;
	uint32_t auth_offset = 0, auth_len = 0;
	int auth_skipped = 0;
	int asconf_cnt = 0;
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
	struct socket *so;
#endif

	SCTPDBG(SCTP_DEBUG_INPUT1, "sctp_process_control: iphlen=%u, offset=%u, length=%u stcb:%p\n",
		iphlen, *offset, length, (void *)stcb);

	
	if (ntohs(ch->chunk_length) < sizeof(*ch)) {
		SCTPDBG(SCTP_DEBUG_INPUT1, "Invalid header length %d\n",
			ntohs(ch->chunk_length));
		if (locked_tcb) {
			SCTP_TCB_UNLOCK(locked_tcb);
		}
		return (NULL);
	}
	


	vtag_in = ntohl(sh->v_tag);

	if (locked_tcb) {
		SCTP_TCB_LOCK_ASSERT(locked_tcb);
	}
	if (ch->chunk_type == SCTP_INITIATION) {
		SCTPDBG(SCTP_DEBUG_INPUT1, "Its an INIT of len:%d vtag:%x\n",
			ntohs(ch->chunk_length), vtag_in);
		if (vtag_in != 0) {
			
			SCTP_STAT_INCR(sctps_badvtag);
			if (locked_tcb) {
				SCTP_TCB_UNLOCK(locked_tcb);
			}
			return (NULL);
		}
	} else if (ch->chunk_type != SCTP_COOKIE_ECHO) {
		




		if ((ch->chunk_type == SCTP_AUTHENTICATION) &&
		    (stcb == NULL) &&
		    !SCTP_BASE_SYSCTL(sctp_auth_disable)) {
			
			auth_skipped = 1;
			auth_offset = *offset;
			auth_len = ntohs(ch->chunk_length);

			
			*offset += SCTP_SIZE32(auth_len);
			if (*offset >= length) {
				
				*offset = length;
				if (locked_tcb) {
					SCTP_TCB_UNLOCK(locked_tcb);
				}
				return (NULL);
			}
			ch = (struct sctp_chunkhdr *)sctp_m_getptr(m, *offset,
								   sizeof(struct sctp_chunkhdr), chunk_buf);
		}
		if (ch == NULL) {
			
			*offset = length;
			if (locked_tcb) {
				SCTP_TCB_UNLOCK(locked_tcb);
			}
			return (NULL);
		}
		if (ch->chunk_type == SCTP_COOKIE_ECHO) {
			goto process_control_chunks;
		}
		



		if (ch->chunk_type == SCTP_ASCONF && stcb == NULL) {
			struct sctp_chunkhdr *asconf_ch = ch;
			uint32_t asconf_offset = 0, asconf_len = 0;

			
			SCTP_INP_INCR_REF(inp);

			asconf_offset = *offset;
			do {
				asconf_len = ntohs(asconf_ch->chunk_length);
				if (asconf_len < sizeof(struct sctp_asconf_paramhdr))
					break;
				stcb = sctp_findassociation_ep_asconf(m,
				                                      *offset,
				                                      dst,
				                                      sh, &inp, netp, vrf_id);
				if (stcb != NULL)
					break;
				asconf_offset += SCTP_SIZE32(asconf_len);
				asconf_ch = (struct sctp_chunkhdr *)sctp_m_getptr(m, asconf_offset,
										  sizeof(struct sctp_chunkhdr), chunk_buf);
			} while (asconf_ch != NULL && asconf_ch->chunk_type == SCTP_ASCONF);
			if (stcb == NULL) {
				



				SCTP_INP_DECR_REF(inp);
			} else {
				locked_tcb = stcb;
			}

			
			if (auth_skipped && (stcb != NULL)) {
				struct sctp_auth_chunk *auth;

				auth = (struct sctp_auth_chunk *)
					sctp_m_getptr(m, auth_offset,
						      auth_len, chunk_buf);
				got_auth = 1;
				auth_skipped = 0;
				if ((auth == NULL) || sctp_handle_auth(stcb, auth, m,
								       auth_offset)) {
					
					*offset = length;
					if (locked_tcb) {
						SCTP_TCB_UNLOCK(locked_tcb);
					}
					return (NULL);
				} else {
					
					stcb->asoc.authenticated = 1;
				}
			}
		}
		if (stcb == NULL) {
			
			sctp_handle_ootb(m, iphlen, *offset, src, dst, sh, inp,
#if defined(__FreeBSD__)
			                 use_mflowid, mflowid,
#endif
					 vrf_id, port);
			*offset = length;
			if (locked_tcb) {
				SCTP_TCB_UNLOCK(locked_tcb);
			}
			return (NULL);
		}
		asoc = &stcb->asoc;
		
		if ((ch->chunk_type == SCTP_ABORT_ASSOCIATION) ||
		    (ch->chunk_type == SCTP_SHUTDOWN_COMPLETE) ||
		    (ch->chunk_type == SCTP_PACKET_DROPPED)) {
			if ((vtag_in == asoc->my_vtag) ||
			    ((ch->chunk_flags & SCTP_HAD_NO_TCB) &&
			     (vtag_in == asoc->peer_vtag))) {
				
			} else {
				
				SCTP_STAT_INCR(sctps_badvtag);
				if (locked_tcb) {
					SCTP_TCB_UNLOCK(locked_tcb);
				}
				return (NULL);
			}
		} else if (ch->chunk_type == SCTP_SHUTDOWN_ACK) {
			if (vtag_in != asoc->my_vtag) {
				






				if (locked_tcb) {
					SCTP_TCB_UNLOCK(locked_tcb);
				}
				sctp_handle_ootb(m, iphlen, *offset, src, dst,
				                 sh, inp,
#if defined(__FreeBSD__)
				                 use_mflowid, mflowid,
#endif
				                 vrf_id, port);
				return (NULL);
			}
		} else {
			
			if (vtag_in != asoc->my_vtag) {
				
				SCTPDBG(SCTP_DEBUG_INPUT3,
					"invalid vtag: %xh, expect %xh\n",
					vtag_in, asoc->my_vtag);
				SCTP_STAT_INCR(sctps_badvtag);
				if (locked_tcb) {
					SCTP_TCB_UNLOCK(locked_tcb);
				}
				*offset = length;
				return (NULL);
			}
		}
	}			
	


	if (((ch->chunk_type == SCTP_SELECTIVE_ACK) ||
	     (ch->chunk_type == SCTP_NR_SELECTIVE_ACK) ||
	     (ch->chunk_type == SCTP_HEARTBEAT_REQUEST)) &&
	    (SCTP_GET_STATE(&stcb->asoc) == SCTP_STATE_COOKIE_ECHOED)) {
		
		if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_THRESHOLD_LOGGING) {
			sctp_misc_ints(SCTP_THRESHOLD_CLEAR,
				       stcb->asoc.overall_error_count,
				       0,
				       SCTP_FROM_SCTP_INPUT,
				       __LINE__);
		}
		stcb->asoc.overall_error_count = 0;
		sctp_handle_cookie_ack((struct sctp_cookie_ack_chunk *)ch, stcb,
				       *netp);
	}

 process_control_chunks:
	while (IS_SCTP_CONTROL(ch)) {
		
		chk_length = ntohs(ch->chunk_length);
		SCTPDBG(SCTP_DEBUG_INPUT2, "sctp_process_control: processing a chunk type=%u, len=%u\n",
			ch->chunk_type, chk_length);
		SCTP_LTRACE_CHK(inp, stcb, ch->chunk_type, chk_length);
		if (chk_length < sizeof(*ch) ||
		    (*offset + (int)chk_length) > length) {
			*offset = length;
			if (locked_tcb) {
				SCTP_TCB_UNLOCK(locked_tcb);
			}
			return (NULL);
		}
		SCTP_STAT_INCR_COUNTER64(sctps_incontrolchunks);
		




		if ((ch->chunk_type == SCTP_INITIATION_ACK) ||
		    (ch->chunk_type == SCTP_INITIATION)) {
			
			ch = (struct sctp_chunkhdr *)sctp_m_getptr(m, *offset,
								   sizeof(struct sctp_init_ack_chunk), chunk_buf);
			if (ch == NULL) {
				*offset = length;
				if (locked_tcb) {
					SCTP_TCB_UNLOCK(locked_tcb);
				}
				return (NULL);
			}
		} else {
			
			if (chk_length > sizeof(chunk_buf)) {
				









				ch = (struct sctp_chunkhdr *)sctp_m_getptr(m, *offset,
									   (sizeof(chunk_buf) - 4),
									   chunk_buf);
				if (ch == NULL) {
					*offset = length;
					if (locked_tcb) {
						SCTP_TCB_UNLOCK(locked_tcb);
					}
					return (NULL);
				}
			} else {
				
				ch = (struct sctp_chunkhdr *)sctp_m_getptr(m, *offset,
								   chk_length, chunk_buf);
				if (ch == NULL) {
					SCTP_PRINTF("sctp_process_control: Can't get the all data....\n");
					*offset = length;
					if (locked_tcb) {
						SCTP_TCB_UNLOCK(locked_tcb);
					}
					return (NULL);
				}
			}
		}
		num_chunks++;
		
		if (stcb != NULL) {
			if (((netp != NULL) && (*netp != NULL)) || (ch->chunk_type == SCTP_ASCONF)) {
				




				if ((netp != NULL) && (*netp != NULL))
					stcb->asoc.last_control_chunk_from = *netp;
			}
		}
#ifdef SCTP_AUDITING_ENABLED
		sctp_audit_log(0xB0, ch->chunk_type);
#endif

		
		if ((stcb != NULL) &&
		    !SCTP_BASE_SYSCTL(sctp_auth_disable) &&
		    sctp_auth_is_required_chunk(ch->chunk_type, stcb->asoc.local_auth_chunks) &&
		    !stcb->asoc.authenticated) {
			
			SCTP_STAT_INCR(sctps_recvauthmissing);
			goto next_chunk;
		}
		switch (ch->chunk_type) {
		case SCTP_INITIATION:
			SCTPDBG(SCTP_DEBUG_INPUT3, "SCTP_INIT\n");
			
			if ((num_chunks > 1) ||
			    (length - *offset > (int)SCTP_SIZE32(chk_length))) {
				sctp_abort_association(inp, stcb, m, iphlen,
				                       src, dst, sh, NULL,
#if defined(__FreeBSD__)
				                       use_mflowid, mflowid,
#endif
				                       vrf_id, port);
				*offset = length;
				return (NULL);
			}
			
			if (chk_length > SCTP_LARGEST_INIT_ACCEPTED) {
				struct mbuf *op_err;

				op_err = sctp_generate_invmanparam(SCTP_CAUSE_OUT_OF_RESC);
				sctp_abort_association(inp, stcb, m, iphlen,
						       src, dst, sh, op_err,
#if defined(__FreeBSD__)
				                       use_mflowid, mflowid,
#endif
				                       vrf_id, port);
				*offset = length;
				return (NULL);
			}
			sctp_handle_init(m, iphlen, *offset, src, dst, sh,
			                 (struct sctp_init_chunk *)ch, inp,
			                 stcb, &abort_no_unlock,
#if defined(__FreeBSD__)
			                 use_mflowid, mflowid,
#endif
			                 vrf_id, port);
			*offset = length;
			if ((!abort_no_unlock) && (locked_tcb)) {
				SCTP_TCB_UNLOCK(locked_tcb);
			}
			return (NULL);
			break;
		case SCTP_PAD_CHUNK:
			break;
		case SCTP_INITIATION_ACK:
			SCTPDBG(SCTP_DEBUG_INPUT3, "SCTP_INIT-ACK\n");
			if (inp->sctp_flags & SCTP_PCB_FLAGS_SOCKET_GONE) {
				
 				if ((stcb) && (stcb->asoc.total_output_queue_size)) {
					;
				} else {
					if (locked_tcb != stcb) {
						
						SCTP_TCB_UNLOCK(locked_tcb);
					}
					*offset = length;
					if (stcb) {
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
						so = SCTP_INP_SO(inp);
						atomic_add_int(&stcb->asoc.refcnt, 1);
						SCTP_TCB_UNLOCK(stcb);
						SCTP_SOCKET_LOCK(so, 1);
						SCTP_TCB_LOCK(stcb);
						atomic_subtract_int(&stcb->asoc.refcnt, 1);
#endif
						(void)sctp_free_assoc(inp, stcb, SCTP_NORMAL_PROC, SCTP_FROM_SCTP_INPUT+SCTP_LOC_27);
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
						SCTP_SOCKET_UNLOCK(so, 1);
#endif
					}
					return (NULL);
				}
			}
			
			if ((num_chunks > 1) ||
			    (length - *offset > (int)SCTP_SIZE32(chk_length))) {
				*offset = length;
				if (locked_tcb) {
					SCTP_TCB_UNLOCK(locked_tcb);
				}
				return (NULL);
			}
			if ((netp) && (*netp)) {
				ret = sctp_handle_init_ack(m, iphlen, *offset,
				                           src, dst, sh,
				                           (struct sctp_init_ack_chunk *)ch,
				                           stcb, *netp,
				                           &abort_no_unlock,
#if defined(__FreeBSD__)
				                           use_mflowid, mflowid,
#endif
				                           vrf_id);
			} else {
				ret = -1;
			}
			*offset = length;
			if (abort_no_unlock) {
				return (NULL);
			}
			



			if ((stcb != NULL) && (ret == 0)) {
				sctp_chunk_output(stcb->sctp_ep, stcb, SCTP_OUTPUT_FROM_CONTROL_PROC, SCTP_SO_NOT_LOCKED);
			}
			if (locked_tcb) {
				SCTP_TCB_UNLOCK(locked_tcb);
			}
			return (NULL);
			break;
		case SCTP_SELECTIVE_ACK:
			{
				struct sctp_sack_chunk *sack;
				int abort_now = 0;
				uint32_t a_rwnd, cum_ack;
				uint16_t num_seg, num_dup;
				uint8_t flags;
				int offset_seg, offset_dup;

				SCTPDBG(SCTP_DEBUG_INPUT3, "SCTP_SACK\n");
				SCTP_STAT_INCR(sctps_recvsacks);
				if (stcb == NULL) {
					SCTPDBG(SCTP_DEBUG_INDATA1, "No stcb when processing SACK chunk\n");
					break;
				}
				if (chk_length < sizeof(struct sctp_sack_chunk)) {
					SCTPDBG(SCTP_DEBUG_INDATA1, "Bad size on SACK chunk, too small\n");
					break;
				}
				if (SCTP_GET_STATE(&stcb->asoc) == SCTP_STATE_SHUTDOWN_ACK_SENT) {
					




					break;
				}
				sack = (struct sctp_sack_chunk *)ch;
				flags = ch->chunk_flags;
				cum_ack = ntohl(sack->sack.cum_tsn_ack);
				num_seg = ntohs(sack->sack.num_gap_ack_blks);
				num_dup = ntohs(sack->sack.num_dup_tsns);
				a_rwnd = (uint32_t) ntohl(sack->sack.a_rwnd);
				if (sizeof(struct sctp_sack_chunk) +
				    num_seg * sizeof(struct sctp_gap_ack_block) +
				    num_dup * sizeof(uint32_t) != chk_length) {
					SCTPDBG(SCTP_DEBUG_INDATA1, "Bad size of SACK chunk\n");
					break;
				}
				offset_seg = *offset + sizeof(struct sctp_sack_chunk);
				offset_dup = offset_seg + num_seg * sizeof(struct sctp_gap_ack_block);
				SCTPDBG(SCTP_DEBUG_INPUT3, "SCTP_SACK process cum_ack:%x num_seg:%d a_rwnd:%d\n",
				        cum_ack, num_seg, a_rwnd);
				stcb->asoc.seen_a_sack_this_pkt = 1;
				if ((stcb->asoc.pr_sctp_cnt == 0) &&
				    (num_seg == 0) &&
				    SCTP_TSN_GE(cum_ack, stcb->asoc.last_acked_seq) &&
				    (stcb->asoc.saw_sack_with_frags == 0) &&
				    (stcb->asoc.saw_sack_with_nr_frags == 0) &&
				    (!TAILQ_EMPTY(&stcb->asoc.sent_queue))
					) {
					




					sctp_express_handle_sack(stcb, cum_ack, a_rwnd, &abort_now, ecne_seen);
				} else {
					if (netp && *netp)
						sctp_handle_sack(m, offset_seg, offset_dup, stcb,
								 num_seg, 0, num_dup, &abort_now, flags,
								 cum_ack, a_rwnd, ecne_seen);
				}
				if (abort_now) {
					
					*offset = length;
					return (NULL);
				}
				if (TAILQ_EMPTY(&stcb->asoc.send_queue) &&
				    TAILQ_EMPTY(&stcb->asoc.sent_queue) &&
				    (stcb->asoc.stream_queue_cnt == 0)) {
					sctp_ulp_notify(SCTP_NOTIFY_SENDER_DRY, stcb,  0, NULL, SCTP_SO_NOT_LOCKED);
				}
			}
			break;
		
		case SCTP_NR_SELECTIVE_ACK:
			{
				struct sctp_nr_sack_chunk *nr_sack;
				int abort_now = 0;
				uint32_t a_rwnd, cum_ack;
				uint16_t num_seg, num_nr_seg, num_dup;
				uint8_t flags;
				int offset_seg, offset_dup;

				SCTPDBG(SCTP_DEBUG_INPUT3, "SCTP_NR_SACK\n");
				SCTP_STAT_INCR(sctps_recvsacks);
				if (stcb == NULL) {
					SCTPDBG(SCTP_DEBUG_INDATA1, "No stcb when processing NR-SACK chunk\n");
					break;
				}
				if ((stcb->asoc.sctp_nr_sack_on_off == 0) ||
				    (stcb->asoc.peer_supports_nr_sack == 0)) {
					goto unknown_chunk;
				}
				if (chk_length < sizeof(struct sctp_nr_sack_chunk)) {
					SCTPDBG(SCTP_DEBUG_INDATA1, "Bad size on NR-SACK chunk, too small\n");
					break;
				}
				if (SCTP_GET_STATE(&stcb->asoc) == SCTP_STATE_SHUTDOWN_ACK_SENT) {
					




					break;
				}
				nr_sack = (struct sctp_nr_sack_chunk *)ch;
				flags = ch->chunk_flags;
				cum_ack = ntohl(nr_sack->nr_sack.cum_tsn_ack);
				num_seg = ntohs(nr_sack->nr_sack.num_gap_ack_blks);
				num_nr_seg = ntohs(nr_sack->nr_sack.num_nr_gap_ack_blks);
				num_dup = ntohs(nr_sack->nr_sack.num_dup_tsns);
				a_rwnd = (uint32_t) ntohl(nr_sack->nr_sack.a_rwnd);
				if (sizeof(struct sctp_nr_sack_chunk) +
				    (num_seg + num_nr_seg) * sizeof(struct sctp_gap_ack_block) +
				    num_dup * sizeof(uint32_t) != chk_length) {
					SCTPDBG(SCTP_DEBUG_INDATA1, "Bad size of NR_SACK chunk\n");
					break;
				}
				offset_seg = *offset + sizeof(struct sctp_nr_sack_chunk);
				offset_dup = offset_seg + num_seg * sizeof(struct sctp_gap_ack_block);
				SCTPDBG(SCTP_DEBUG_INPUT3, "SCTP_NR_SACK process cum_ack:%x num_seg:%d a_rwnd:%d\n",
				        cum_ack, num_seg, a_rwnd);
				stcb->asoc.seen_a_sack_this_pkt = 1;
				if ((stcb->asoc.pr_sctp_cnt == 0) &&
				    (num_seg == 0) && (num_nr_seg == 0) &&
				    SCTP_TSN_GE(cum_ack, stcb->asoc.last_acked_seq) &&
				    (stcb->asoc.saw_sack_with_frags == 0) &&
				    (stcb->asoc.saw_sack_with_nr_frags == 0) &&
				    (!TAILQ_EMPTY(&stcb->asoc.sent_queue))) {
					








					sctp_express_handle_sack(stcb, cum_ack, a_rwnd,
					                         &abort_now, ecne_seen);
				} else {
					if (netp && *netp)
						sctp_handle_sack(m, offset_seg, offset_dup, stcb,
						                 num_seg, num_nr_seg, num_dup, &abort_now, flags,
						                 cum_ack, a_rwnd, ecne_seen);
				}
				if (abort_now) {
					
					*offset = length;
					return (NULL);
				}
				if (TAILQ_EMPTY(&stcb->asoc.send_queue) &&
				    TAILQ_EMPTY(&stcb->asoc.sent_queue) &&
				    (stcb->asoc.stream_queue_cnt == 0)) {
					sctp_ulp_notify(SCTP_NOTIFY_SENDER_DRY, stcb,  0, NULL, SCTP_SO_NOT_LOCKED);
				}
			}
			break;

		case SCTP_HEARTBEAT_REQUEST:
			SCTPDBG(SCTP_DEBUG_INPUT3, "SCTP_HEARTBEAT\n");
			if ((stcb) && netp && *netp) {
				SCTP_STAT_INCR(sctps_recvheartbeat);
				sctp_send_heartbeat_ack(stcb, m, *offset,
							chk_length, *netp);

				
				if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_THRESHOLD_LOGGING) {
					sctp_misc_ints(SCTP_THRESHOLD_CLEAR,
						       stcb->asoc.overall_error_count,
						       0,
						       SCTP_FROM_SCTP_INPUT,
						       __LINE__);
				}
				stcb->asoc.overall_error_count = 0;
			}
			break;
		case SCTP_HEARTBEAT_ACK:
			SCTPDBG(SCTP_DEBUG_INPUT3, "SCTP_HEARTBEAT-ACK\n");
			if ((stcb == NULL) || (chk_length != sizeof(struct sctp_heartbeat_chunk))) {
				
				*offset = length;
				if (locked_tcb) {
					SCTP_TCB_UNLOCK(locked_tcb);
				}
				return (NULL);
			}
			
			if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_THRESHOLD_LOGGING) {
				sctp_misc_ints(SCTP_THRESHOLD_CLEAR,
					       stcb->asoc.overall_error_count,
					       0,
					       SCTP_FROM_SCTP_INPUT,
					       __LINE__);
			}
			stcb->asoc.overall_error_count = 0;
			SCTP_STAT_INCR(sctps_recvheartbeatack);
			if (netp && *netp)
				sctp_handle_heartbeat_ack((struct sctp_heartbeat_chunk *)ch,
							  stcb, *netp);
			break;
		case SCTP_ABORT_ASSOCIATION:
			SCTPDBG(SCTP_DEBUG_INPUT3, "SCTP_ABORT, stcb %p\n",
				(void *)stcb);
			if ((stcb) && netp && *netp)
				sctp_handle_abort((struct sctp_abort_chunk *)ch,
						  stcb, *netp);
			*offset = length;
			return (NULL);
			break;
		case SCTP_SHUTDOWN:
			SCTPDBG(SCTP_DEBUG_INPUT3, "SCTP_SHUTDOWN, stcb %p\n",
				(void *)stcb);
			if ((stcb == NULL) || (chk_length != sizeof(struct sctp_shutdown_chunk))) {
				*offset = length;
				if (locked_tcb) {
					SCTP_TCB_UNLOCK(locked_tcb);
				}
				return (NULL);
			}
			if (netp && *netp) {
				int abort_flag = 0;

				sctp_handle_shutdown((struct sctp_shutdown_chunk *)ch,
						     stcb, *netp, &abort_flag);
				if (abort_flag) {
					*offset = length;
					return (NULL);
				}
			}
			break;
		case SCTP_SHUTDOWN_ACK:
			SCTPDBG(SCTP_DEBUG_INPUT3, "SCTP_SHUTDOWN-ACK, stcb %p\n", (void *)stcb);
			if ((stcb) && (netp) && (*netp))
				sctp_handle_shutdown_ack((struct sctp_shutdown_ack_chunk *)ch, stcb, *netp);
			*offset = length;
			return (NULL);
			break;

		case SCTP_OPERATION_ERROR:
			SCTPDBG(SCTP_DEBUG_INPUT3, "SCTP_OP-ERR\n");
			if ((stcb) && netp && *netp && sctp_handle_error(ch, stcb, *netp) < 0) {
				*offset = length;
				return (NULL);
			}
			break;
		case SCTP_COOKIE_ECHO:
			SCTPDBG(SCTP_DEBUG_INPUT3,
				"SCTP_COOKIE-ECHO, stcb %p\n", (void *)stcb);
			if ((stcb) && (stcb->asoc.total_output_queue_size)) {
				;
			} else {
				if (inp->sctp_flags & SCTP_PCB_FLAGS_SOCKET_GONE) {
					
				abend:
					if (stcb) {
						SCTP_TCB_UNLOCK(stcb);
					}
					*offset = length;
					return (NULL);
				}
			}
			







			if ((stcb == NULL) && (inp->sctp_socket->so_qlen >= inp->sctp_socket->so_qlimit)) {
				if ((inp->sctp_flags & SCTP_PCB_FLAGS_TCPTYPE) &&
				    (SCTP_BASE_SYSCTL(sctp_abort_if_one_2_one_hits_limit))) {
					struct mbuf *op_err;

					op_err = sctp_generate_invmanparam(SCTP_CAUSE_OUT_OF_RESC);
					sctp_abort_association(inp, stcb, m, iphlen,
					                       src, dst, sh, op_err,
#if defined(__FreeBSD__)
					                       use_mflowid, mflowid,
#endif
					                       vrf_id, port);
				}
				*offset = length;
				return (NULL);
			} else {
				struct mbuf *ret_buf;
				struct sctp_inpcb *linp;
				if (stcb) {
					linp = NULL;
				} else {
					linp = inp;
				}

				if (linp) {
					SCTP_ASOC_CREATE_LOCK(linp);
					if ((inp->sctp_flags & SCTP_PCB_FLAGS_SOCKET_GONE) ||
					    (inp->sctp_flags & SCTP_PCB_FLAGS_SOCKET_ALLGONE)) {
						SCTP_ASOC_CREATE_UNLOCK(linp);
						goto abend;
					}
				}

				if (netp) {
					ret_buf =
						sctp_handle_cookie_echo(m, iphlen,
						                        *offset,
						                        src, dst,
						                        sh,
						                        (struct sctp_cookie_echo_chunk *)ch,
						                        &inp, &stcb, netp,
						                        auth_skipped,
						                        auth_offset,
						                        auth_len,
						                        &locked_tcb,
#if defined(__FreeBSD__)
						                        use_mflowid,
						                        mflowid,
#endif
						                        vrf_id,
						                        port);
				} else {
					ret_buf = NULL;
				}
				if (linp) {
					SCTP_ASOC_CREATE_UNLOCK(linp);
				}
				if (ret_buf == NULL) {
					if (locked_tcb) {
						SCTP_TCB_UNLOCK(locked_tcb);
					}
					SCTPDBG(SCTP_DEBUG_INPUT3,
						"GAK, null buffer\n");
					*offset = length;
					return (NULL);
				}
				
				if (auth_skipped) {
					got_auth = 1;
					auth_skipped = 0;
				}
				if (!TAILQ_EMPTY(&stcb->asoc.sent_queue)) {
					



					struct sctp_tmit_chunk *chk;

					chk = TAILQ_FIRST(&stcb->asoc.sent_queue);
					sctp_timer_start(SCTP_TIMER_TYPE_SEND, stcb->sctp_ep, stcb, chk->whoTo);
				}
			}
			break;
		case SCTP_COOKIE_ACK:
			SCTPDBG(SCTP_DEBUG_INPUT3, "SCTP_COOKIE-ACK, stcb %p\n", (void *)stcb);
			if ((stcb == NULL) || chk_length != sizeof(struct sctp_cookie_ack_chunk)) {
				if (locked_tcb) {
					SCTP_TCB_UNLOCK(locked_tcb);
				}
				return (NULL);
			}
			if (inp->sctp_flags & SCTP_PCB_FLAGS_SOCKET_GONE) {
				
				if ((stcb) && (stcb->asoc.total_output_queue_size)) {
					;
				} else if (stcb) {
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
					so = SCTP_INP_SO(inp);
					atomic_add_int(&stcb->asoc.refcnt, 1);
					SCTP_TCB_UNLOCK(stcb);
					SCTP_SOCKET_LOCK(so, 1);
					SCTP_TCB_LOCK(stcb);
					atomic_subtract_int(&stcb->asoc.refcnt, 1);
#endif
					(void)sctp_free_assoc(inp, stcb, SCTP_NORMAL_PROC, SCTP_FROM_SCTP_INPUT+SCTP_LOC_27);
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
					SCTP_SOCKET_UNLOCK(so, 1);
#endif
					*offset = length;
					return (NULL);
				}
			}
			
			if ((stcb) && netp && *netp) {
				if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_THRESHOLD_LOGGING) {
					sctp_misc_ints(SCTP_THRESHOLD_CLEAR,
						       stcb->asoc.overall_error_count,
						       0,
						       SCTP_FROM_SCTP_INPUT,
						       __LINE__);
				}
				stcb->asoc.overall_error_count = 0;
				sctp_handle_cookie_ack((struct sctp_cookie_ack_chunk *)ch,stcb, *netp);
			}
			break;
		case SCTP_ECN_ECHO:
			SCTPDBG(SCTP_DEBUG_INPUT3, "SCTP_ECN-ECHO\n");
			
			if ((stcb == NULL) || (chk_length != sizeof(struct sctp_ecne_chunk))) {
				
				if (locked_tcb) {
					SCTP_TCB_UNLOCK(locked_tcb);
				}
				*offset = length;
				return (NULL);
			}
			if (stcb) {
				if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_THRESHOLD_LOGGING) {
					sctp_misc_ints(SCTP_THRESHOLD_CLEAR,
						       stcb->asoc.overall_error_count,
						       0,
						       SCTP_FROM_SCTP_INPUT,
						       __LINE__);
				}
				stcb->asoc.overall_error_count = 0;
				sctp_handle_ecn_echo((struct sctp_ecne_chunk *)ch,
						     stcb);
				ecne_seen = 1;
			}
			break;
		case SCTP_ECN_CWR:
			SCTPDBG(SCTP_DEBUG_INPUT3, "SCTP_ECN-CWR\n");
			
			if ((stcb == NULL) || (chk_length != sizeof(struct sctp_cwr_chunk))) {
				
				if (locked_tcb) {
					SCTP_TCB_UNLOCK(locked_tcb);
				}
				*offset = length;
				return (NULL);
			}
			if (stcb) {
				if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_THRESHOLD_LOGGING) {
					sctp_misc_ints(SCTP_THRESHOLD_CLEAR,
						       stcb->asoc.overall_error_count,
						       0,
						       SCTP_FROM_SCTP_INPUT,
						       __LINE__);
				}
				stcb->asoc.overall_error_count = 0;
				sctp_handle_ecn_cwr((struct sctp_cwr_chunk *)ch, stcb, *netp);
			}
			break;
		case SCTP_SHUTDOWN_COMPLETE:
			SCTPDBG(SCTP_DEBUG_INPUT3, "SCTP_SHUTDOWN-COMPLETE, stcb %p\n", (void *)stcb);
			
			if ((num_chunks > 1) ||
			    (length - *offset > (int)SCTP_SIZE32(chk_length))) {
				*offset = length;
				if (locked_tcb) {
					SCTP_TCB_UNLOCK(locked_tcb);
				}
				return (NULL);
			}
			if ((stcb) && netp && *netp) {
				sctp_handle_shutdown_complete((struct sctp_shutdown_complete_chunk *)ch,
							      stcb, *netp);
			}
			*offset = length;
			return (NULL);
			break;
		case SCTP_ASCONF:
			SCTPDBG(SCTP_DEBUG_INPUT3, "SCTP_ASCONF\n");
			
			if (stcb) {
				if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_THRESHOLD_LOGGING) {
					sctp_misc_ints(SCTP_THRESHOLD_CLEAR,
						       stcb->asoc.overall_error_count,
						       0,
						       SCTP_FROM_SCTP_INPUT,
						       __LINE__);
				}
				stcb->asoc.overall_error_count = 0;
				sctp_handle_asconf(m, *offset, src,
						   (struct sctp_asconf_chunk *)ch, stcb, asconf_cnt == 0);
				asconf_cnt++;
			}
			break;
		case SCTP_ASCONF_ACK:
			SCTPDBG(SCTP_DEBUG_INPUT3, "SCTP_ASCONF-ACK\n");
			if (chk_length < sizeof(struct sctp_asconf_ack_chunk)) {
				
				if (locked_tcb) {
					SCTP_TCB_UNLOCK(locked_tcb);
				}
				*offset = length;
				return (NULL);
			}
			if ((stcb) && netp && *netp) {
				
				if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_THRESHOLD_LOGGING) {
					sctp_misc_ints(SCTP_THRESHOLD_CLEAR,
						       stcb->asoc.overall_error_count,
						       0,
						       SCTP_FROM_SCTP_INPUT,
						       __LINE__);
				}
				stcb->asoc.overall_error_count = 0;
				sctp_handle_asconf_ack(m, *offset,
						       (struct sctp_asconf_ack_chunk *)ch, stcb, *netp, &abort_no_unlock);
				if (abort_no_unlock)
					return (NULL);
			}
			break;
		case SCTP_FORWARD_CUM_TSN:
			SCTPDBG(SCTP_DEBUG_INPUT3, "SCTP_FWD-TSN\n");
			if (chk_length < sizeof(struct sctp_forward_tsn_chunk)) {
				
				if (locked_tcb) {
					SCTP_TCB_UNLOCK(locked_tcb);
				}
				*offset = length;
				return (NULL);
			}

			
			if (stcb) {
				int abort_flag = 0;
				stcb->asoc.overall_error_count = 0;
				if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_THRESHOLD_LOGGING) {
					sctp_misc_ints(SCTP_THRESHOLD_CLEAR,
						       stcb->asoc.overall_error_count,
						       0,
						       SCTP_FROM_SCTP_INPUT,
						       __LINE__);
				}
				*fwd_tsn_seen = 1;
				if (inp->sctp_flags & SCTP_PCB_FLAGS_SOCKET_GONE) {
					
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
					so = SCTP_INP_SO(inp);
					atomic_add_int(&stcb->asoc.refcnt, 1);
					SCTP_TCB_UNLOCK(stcb);
					SCTP_SOCKET_LOCK(so, 1);
					SCTP_TCB_LOCK(stcb);
					atomic_subtract_int(&stcb->asoc.refcnt, 1);
#endif
					(void)sctp_free_assoc(inp, stcb, SCTP_NORMAL_PROC, SCTP_FROM_SCTP_INPUT+SCTP_LOC_29);
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
					SCTP_SOCKET_UNLOCK(so, 1);
#endif
					*offset = length;
					return (NULL);
				}
				sctp_handle_forward_tsn(stcb,
							(struct sctp_forward_tsn_chunk *)ch, &abort_flag, m, *offset);
				if (abort_flag) {
					*offset = length;
					return (NULL);
				} else {
					if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_THRESHOLD_LOGGING) {
						sctp_misc_ints(SCTP_THRESHOLD_CLEAR,
							       stcb->asoc.overall_error_count,
							       0,
							       SCTP_FROM_SCTP_INPUT,
							       __LINE__);
					}
					stcb->asoc.overall_error_count = 0;
				}

			}
			break;
		case SCTP_STREAM_RESET:
			SCTPDBG(SCTP_DEBUG_INPUT3, "SCTP_STREAM_RESET\n");
			if (((stcb == NULL) || (ch == NULL) || (chk_length < sizeof(struct sctp_stream_reset_tsn_req)))) {
				
				if (locked_tcb) {
					SCTP_TCB_UNLOCK(locked_tcb);
				}
				*offset = length;
				return (NULL);
			}
			if (stcb->asoc.peer_supports_strreset == 0) {
				




				stcb->asoc.peer_supports_strreset = 1;
			}
			if (sctp_handle_stream_reset(stcb, m, *offset, ch)) {
				
				*offset = length;
				return (NULL);
			}
			break;
		case SCTP_PACKET_DROPPED:
			SCTPDBG(SCTP_DEBUG_INPUT3, "SCTP_PACKET_DROPPED\n");
			
			if (chk_length < sizeof(struct sctp_pktdrop_chunk)) {
				
				if (locked_tcb) {
					SCTP_TCB_UNLOCK(locked_tcb);
				}
				*offset = length;
				return (NULL);
			}


			if (ch && (stcb) && netp && (*netp)) {
				sctp_handle_packet_dropped((struct sctp_pktdrop_chunk *)ch,
							   stcb, *netp,
							   min(chk_length, (sizeof(chunk_buf) - 4)));

			}

			break;

		case SCTP_AUTHENTICATION:
			SCTPDBG(SCTP_DEBUG_INPUT3, "SCTP_AUTHENTICATION\n");
			if (SCTP_BASE_SYSCTL(sctp_auth_disable))
				goto unknown_chunk;

			if (stcb == NULL) {
				
				if (auth_skipped == 0) {
					auth_offset = *offset;
					auth_len = chk_length;
					auth_skipped = 1;
				}
				
				goto next_chunk;
			}
			if ((chk_length < (sizeof(struct sctp_auth_chunk))) ||
			    (chk_length > (sizeof(struct sctp_auth_chunk) +
					   SCTP_AUTH_DIGEST_LEN_MAX))) {
				
				if (locked_tcb) {
					SCTP_TCB_UNLOCK(locked_tcb);
				}
				*offset = length;
				return (NULL);
			}
			if (got_auth == 1) {
				
				goto next_chunk;
			}
			got_auth = 1;
			if ((ch == NULL) || sctp_handle_auth(stcb, (struct sctp_auth_chunk *)ch,
							     m, *offset)) {
				
				*offset = length;
				return (stcb);
			} else {
				
				stcb->asoc.authenticated = 1;
			}
			break;

		default:
		unknown_chunk:
			
			if ((ch->chunk_type & 0x40) && (stcb != NULL)) {
				struct mbuf *mm;
				struct sctp_paramhdr *phd;

				mm = sctp_get_mbuf_for_msg(sizeof(struct sctp_paramhdr),
							   0, M_NOWAIT, 1, MT_DATA);
				if (mm) {
					phd = mtod(mm, struct sctp_paramhdr *);
					






					phd->param_type =  htons(SCTP_CAUSE_UNRECOG_CHUNK);
					phd->param_length = htons(chk_length + sizeof(*phd));
					SCTP_BUF_LEN(mm) = sizeof(*phd);
					SCTP_BUF_NEXT(mm) = SCTP_M_COPYM(m, *offset, chk_length, M_NOWAIT);
					if (SCTP_BUF_NEXT(mm)) {
						if (sctp_pad_lastmbuf(SCTP_BUF_NEXT(mm), SCTP_SIZE32(chk_length) - chk_length, NULL)) {
							sctp_m_freem(mm);
						} else {
#ifdef SCTP_MBUF_LOGGING
							if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_MBUF_LOGGING_ENABLE) {
								struct mbuf *mat;

								for (mat = SCTP_BUF_NEXT(mm); mat; mat = SCTP_BUF_NEXT(mat)) {
									if (SCTP_BUF_IS_EXTENDED(mat)) {
										sctp_log_mb(mat, SCTP_MBUF_ICOPY);
									}
								}
							}
#endif
							sctp_queue_op_err(stcb, mm);
						}
					} else {
						sctp_m_freem(mm);
					}
				}
			}
			if ((ch->chunk_type & 0x80) == 0) {
				
				*offset = length;
				return (stcb);
			}	
			break;
		}		


	next_chunk:
		
		*offset += SCTP_SIZE32(chk_length);
		if (*offset >= length) {
			
			break;
		}
		ch = (struct sctp_chunkhdr *)sctp_m_getptr(m, *offset,
							   sizeof(struct sctp_chunkhdr), chunk_buf);
		if (ch == NULL) {
			if (locked_tcb) {
				SCTP_TCB_UNLOCK(locked_tcb);
			}
			*offset = length;
			return (NULL);
		}
	}			

	if (asconf_cnt > 0 && stcb != NULL) {
		sctp_send_asconf_ack(stcb);
	}
	return (stcb);
}


#ifdef INVARIANTS
#ifdef __GNUC__
__attribute__((noinline))
#endif
void
sctp_validate_no_locks(struct sctp_inpcb *inp)
{
#ifndef __APPLE__
	struct sctp_tcb *lstcb;
	LIST_FOREACH(lstcb, &inp->sctp_asoc_list, sctp_tcblist) {
		if (mtx_owned(&lstcb->tcb_mtx)) {
			panic("Own lock on stcb at return from input");
		}
	}
	if (mtx_owned(&inp->inp_create_mtx)) {
		panic("Own create lock on inp");
	}
	if (mtx_owned(&inp->inp_mtx)) {
		panic("Own inp lock on inp");
	}
#endif
}
#endif




void
sctp_common_input_processing(struct mbuf **mm, int iphlen, int offset, int length,
                             struct sockaddr *src, struct sockaddr *dst,
                             struct sctphdr *sh, struct sctp_chunkhdr *ch,
#if !defined(SCTP_WITH_NO_CSUM)
                             uint8_t compute_crc,
#endif
                             uint8_t ecn_bits,
#if defined(__FreeBSD__)
                             uint8_t use_mflowid, uint32_t mflowid,
#endif
                             uint32_t vrf_id, uint16_t port)
{
	uint32_t high_tsn;
	int fwd_tsn_seen = 0, data_processed = 0;
	struct mbuf *m = *mm;
	int un_sent;
	int cnt_ctrl_ready = 0;
	struct sctp_inpcb *inp = NULL, *inp_decr = NULL;
	struct sctp_tcb *stcb = NULL;
	struct sctp_nets *net = NULL;

	SCTP_STAT_INCR(sctps_recvdatagrams);
#ifdef SCTP_AUDITING_ENABLED
	sctp_audit_log(0xE0, 1);
	sctp_auditing(0, inp, stcb, net);
#endif
#if !defined(SCTP_WITH_NO_CSUM)
	if (compute_crc != 0) {
		uint32_t check, calc_check;

		check = sh->checksum;
		sh->checksum = 0;
		calc_check = sctp_calculate_cksum(m, iphlen);
		sh->checksum = check;
		if (calc_check != check) {
			SCTPDBG(SCTP_DEBUG_INPUT1, "Bad CSUM on SCTP packet calc_check:%x check:%x  m:%p mlen:%d iphlen:%d\n",
			        calc_check, check, (void *)m, length, iphlen);
			stcb = sctp_findassociation_addr(m, offset, src, dst,
			                                 sh, ch, &inp, &net, vrf_id);
			if ((net != NULL) && (port != 0)) {
				if (net->port == 0) {
					sctp_pathmtu_adjustment(stcb, net->mtu - sizeof(struct udphdr));
				}
				net->port = port;
			}
#if defined(__FreeBSD__)
			if ((net != NULL) && (use_mflowid != 0)) {
				net->flowid = mflowid;
#ifdef INVARIANTS
				net->flowidset = 1;
#endif
			}
#endif
			if ((inp != NULL) && (stcb != NULL)) {
				sctp_send_packet_dropped(stcb, net, m, length, iphlen, 1);
				sctp_chunk_output(inp, stcb, SCTP_OUTPUT_FROM_INPUT_ERROR, SCTP_SO_NOT_LOCKED);
			} else if ((inp != NULL) && (stcb == NULL)) {
				inp_decr = inp;
			}
			SCTP_STAT_INCR(sctps_badsum);
			SCTP_STAT_INCR_COUNTER32(sctps_checksumerrors);
			goto out;
		}
	}
#endif
	
	if (sh->dest_port == 0) {
		SCTP_STAT_INCR(sctps_hdrops);
		goto out;
	}
	stcb = sctp_findassociation_addr(m, offset, src, dst,
	                                 sh, ch, &inp, &net, vrf_id);
	if ((net != NULL) && (port != 0)) {
		if (net->port == 0) {
			sctp_pathmtu_adjustment(stcb, net->mtu - sizeof(struct udphdr));
		}
		net->port = port;
	}
#if defined(__FreeBSD__)
	if ((net != NULL) && (use_mflowid != 0)) {
		net->flowid = mflowid;
#ifdef INVARIANTS
		net->flowidset = 1;
#endif
	}
#endif
	if (inp == NULL) {
		SCTP_STAT_INCR(sctps_noport);
#if defined(__FreeBSD__) && (((__FreeBSD_version < 900000) && (__FreeBSD_version >= 804000)) || (__FreeBSD_version > 900000))
		if (badport_bandlim(BANDLIM_SCTP_OOTB) < 0) {
			goto out;
		}
#endif
		if (ch->chunk_type == SCTP_SHUTDOWN_ACK) {
			sctp_send_shutdown_complete2(src, dst, sh,
#if defined(__FreeBSD__)
			                             use_mflowid, mflowid,
#endif
			                             vrf_id, port);
			goto out;
		}
		if (ch->chunk_type == SCTP_SHUTDOWN_COMPLETE) {
			goto out;
		}
		if (ch->chunk_type != SCTP_ABORT_ASSOCIATION) {
			if ((SCTP_BASE_SYSCTL(sctp_blackhole) == 0) ||
			    ((SCTP_BASE_SYSCTL(sctp_blackhole) == 1) &&
			     (ch->chunk_type != SCTP_INIT))) {
				sctp_send_abort(m, iphlen, src, dst,
				                sh, 0, NULL,
#if defined(__FreeBSD__)
				                use_mflowid, mflowid,
#endif
				                vrf_id, port);
			}
		}
		goto out;
	} else if (stcb == NULL) {
		inp_decr = inp;
	}
#ifdef IPSEC
	



	if (inp != NULL) {
		switch (dst->sa_family) {
#ifdef INET
		case AF_INET:
			if (ipsec4_in_reject(m, &inp->ip_inp.inp)) {
				MODULE_GLOBAL(ipsec4stat).in_polvio++;
				SCTP_STAT_INCR(sctps_hdrops);
				goto out;
			}
			break;
#endif
#ifdef INET6
		case AF_INET6:
			if (ipsec6_in_reject(m, &inp->ip_inp.inp)) {
				MODULE_GLOBAL(ipsec6stat).in_polvio++;
				SCTP_STAT_INCR(sctps_hdrops);
				goto out;
			}
			break;
#endif
		default:
			break;
		}
	}
#endif
	SCTPDBG(SCTP_DEBUG_INPUT1, "Ok, Common input processing called, m:%p iphlen:%d offset:%d length:%d stcb:%p\n",
		(void *)m, iphlen, offset, length, (void *)stcb);
	if (stcb) {
		
		stcb->asoc.authenticated = 0;
		stcb->asoc.seen_a_sack_this_pkt = 0;
		SCTPDBG(SCTP_DEBUG_INPUT1, "stcb:%p state:%x\n",
			(void *)stcb, stcb->asoc.state);

		if ((stcb->asoc.state & SCTP_STATE_WAS_ABORTED) ||
		    (stcb->asoc.state & SCTP_STATE_ABOUT_TO_BE_FREED)) {
			





 			SCTP_TCB_UNLOCK(stcb);
			stcb = NULL;
			sctp_handle_ootb(m, iphlen, offset, src, dst, sh, inp,
#if defined(__FreeBSD__)
			                 use_mflowid, mflowid,
#endif
					 vrf_id, port);
			goto out;
		}

	}
	if (IS_SCTP_CONTROL(ch)) {
		
		
		stcb = sctp_process_control(m, iphlen, &offset, length,
		                            src, dst, sh, ch,
		                            inp, stcb, &net, &fwd_tsn_seen,
#if defined(__FreeBSD__)
					    use_mflowid, mflowid,
#endif
		                            vrf_id, port);
		if (stcb) {
			


 			inp = stcb->sctp_ep;
			if ((net) && (port)) {
				if (net->port == 0) {
					sctp_pathmtu_adjustment(stcb, net->mtu - sizeof(struct udphdr));
				}
				net->port = port;
			}
		}
	} else {
		




		




		if ((stcb != NULL) &&
		    !SCTP_BASE_SYSCTL(sctp_auth_disable) &&
		    sctp_auth_is_required_chunk(SCTP_DATA, stcb->asoc.local_auth_chunks)) {
			
			SCTP_STAT_INCR(sctps_recvauthmissing);
			goto out;
		}
		if (stcb == NULL) {
			
			sctp_handle_ootb(m, iphlen, offset, src, dst, sh, inp,
#if defined(__FreeBSD__)
			                 use_mflowid, mflowid,
#endif
					 vrf_id, port);
			goto out;
		}
		if (stcb->asoc.my_vtag != ntohl(sh->v_tag)) {
			
			SCTP_STAT_INCR(sctps_badvtag);
			goto out;
		}
	}

	if (stcb == NULL) {
		




		goto out;
	}

	


	

	



	if ((length > offset) &&
	    (stcb != NULL) &&
	    !SCTP_BASE_SYSCTL(sctp_auth_disable) &&
	    sctp_auth_is_required_chunk(SCTP_DATA, stcb->asoc.local_auth_chunks) &&
	    !stcb->asoc.authenticated) {
		
		SCTP_STAT_INCR(sctps_recvauthmissing);
		SCTPDBG(SCTP_DEBUG_AUTH1,
			"Data chunk requires AUTH, skipped\n");
		goto trigger_send;
	}
	if (length > offset) {
		int retval;

		




		switch (SCTP_GET_STATE(&stcb->asoc)) {
		case SCTP_STATE_COOKIE_ECHOED:
			




			if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_THRESHOLD_LOGGING) {
				sctp_misc_ints(SCTP_THRESHOLD_CLEAR,
					       stcb->asoc.overall_error_count,
					       0,
					       SCTP_FROM_SCTP_INPUT,
					       __LINE__);
			}
			stcb->asoc.overall_error_count = 0;
			sctp_handle_cookie_ack((struct sctp_cookie_ack_chunk *)ch, stcb, net);
			break;
		case SCTP_STATE_COOKIE_WAIT:
			


			sctp_handle_ootb(m, iphlen, offset, src, dst, sh, inp,
#if defined(__FreeBSD__)
			                 use_mflowid, mflowid,
#endif
					 vrf_id, port);
			goto out;
			
			break;
		case SCTP_STATE_EMPTY:	
		case SCTP_STATE_INUSE:	
		case SCTP_STATE_SHUTDOWN_RECEIVED:	
		case SCTP_STATE_SHUTDOWN_ACK_SENT:
		default:
			goto out;
			
			break;
		case SCTP_STATE_OPEN:
		case SCTP_STATE_SHUTDOWN_SENT:
			break;
		}
		
		retval = sctp_process_data(mm, iphlen, &offset, length,
		                           src, dst, sh,
		                           inp, stcb, net, &high_tsn,
#if defined(__FreeBSD__)
		                           use_mflowid, mflowid,
#endif
		                           vrf_id, port);
		if (retval == 2) {
			



			stcb = NULL;
			goto out;
		}
		data_processed = 1;
		



	}

	
	if ((data_processed == 1) &&
	    (stcb->asoc.ecn_allowed == 1) &&
	    ((ecn_bits & SCTP_CE_BITS) == SCTP_CE_BITS)) {
		
		sctp_send_ecn_echo(stcb, net, high_tsn);
	}

	if ((data_processed == 0) && (fwd_tsn_seen)) {
		int was_a_gap;
		uint32_t highest_tsn;

		if (SCTP_TSN_GT(stcb->asoc.highest_tsn_inside_nr_map, stcb->asoc.highest_tsn_inside_map)) {
			highest_tsn = stcb->asoc.highest_tsn_inside_nr_map;
		} else {
			highest_tsn = stcb->asoc.highest_tsn_inside_map;
		}
		was_a_gap = SCTP_TSN_GT(highest_tsn, stcb->asoc.cumulative_tsn);
		stcb->asoc.send_sack = 1;
		sctp_sack_check(stcb, was_a_gap);
	} else if (fwd_tsn_seen) {
		stcb->asoc.send_sack = 1;
	}
	
trigger_send:
#ifdef SCTP_AUDITING_ENABLED
	sctp_audit_log(0xE0, 2);
	sctp_auditing(1, inp, stcb, net);
#endif
	SCTPDBG(SCTP_DEBUG_INPUT1,
		"Check for chunk output prw:%d tqe:%d tf=%d\n",
		stcb->asoc.peers_rwnd,
		TAILQ_EMPTY(&stcb->asoc.control_send_queue),
		stcb->asoc.total_flight);
	un_sent = (stcb->asoc.total_output_queue_size - stcb->asoc.total_flight);
	if (!TAILQ_EMPTY(&stcb->asoc.control_send_queue)) {
		cnt_ctrl_ready = stcb->asoc.ctrl_queue_cnt - stcb->asoc.ecn_echo_cnt_onq;
	}
	if (cnt_ctrl_ready ||
	    ((un_sent) &&
	     (stcb->asoc.peers_rwnd > 0 ||
	      (stcb->asoc.peers_rwnd <= 0 && stcb->asoc.total_flight == 0)))) {
		SCTPDBG(SCTP_DEBUG_INPUT3, "Calling chunk OUTPUT\n");
		sctp_chunk_output(inp, stcb, SCTP_OUTPUT_FROM_CONTROL_PROC, SCTP_SO_NOT_LOCKED);
		SCTPDBG(SCTP_DEBUG_INPUT3, "chunk OUTPUT returns\n");
	}
#ifdef SCTP_AUDITING_ENABLED
	sctp_audit_log(0xE0, 3);
	sctp_auditing(2, inp, stcb, net);
#endif
 out:
	if (stcb != NULL) {
		SCTP_TCB_UNLOCK(stcb);
	}
	if (inp_decr != NULL) {
		
		SCTP_INP_WLOCK(inp_decr);
		SCTP_INP_DECR_REF(inp_decr);
		SCTP_INP_WUNLOCK(inp_decr);
	}
#ifdef INVARIANTS
	if (inp != NULL) {
		sctp_validate_no_locks(inp);
	}
#endif
	return;
}

#if 0
static void
sctp_print_mbuf_chain(struct mbuf *m)
{
	for (; m; m = SCTP_BUF_NEXT(m)) {
		SCTP_PRINTF("%p: m_len = %ld\n", (void *)m, SCTP_BUF_LEN(m));
		if (SCTP_BUF_IS_EXTENDED(m))
			SCTP_PRINTF("%p: extend_size = %d\n", (void *)m, SCTP_BUF_EXTEND_SIZE(m));
	}
}
#endif

#ifdef INET
#if !defined(__Userspace__)
#if defined(__FreeBSD__) || defined(__APPLE__) || defined(__Windows__)
void
sctp_input_with_port(struct mbuf *i_pak, int off, uint16_t port)
#elif defined(__Panda__)
void
sctp_input(pakhandle_type i_pak)
#else
void
#if __STDC__
sctp_input(struct mbuf *i_pak,...)
#else
sctp_input(i_pak, va_alist)
	struct mbuf *i_pak;
#endif
#endif
{
	struct mbuf *m;
	int iphlen;
	uint32_t vrf_id = 0;
	uint8_t ecn_bits;
	struct sockaddr_in src, dst;
	struct ip *ip;
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
#if !(defined(__FreeBSD__) || defined(__APPLE__) || defined(__Windows__))
	uint16_t port = 0;
#endif

#if defined(__Panda__)
	
	iphlen = sizeof(struct ip);
#else
	iphlen = off;
#endif
	if (SCTP_GET_PKT_VRFID(i_pak, vrf_id)) {
		SCTP_RELEASE_PKT(i_pak);
		return;
	}
	m = SCTP_HEADER_TO_CHAIN(i_pak);
#ifdef __Panda__
	SCTP_DETACH_HEADER_FROM_CHAIN(i_pak);
	(void)SCTP_RELEASE_HEADER(i_pak);
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
	        "sctp_input(): Packet of length %d received on %s with csum_flags 0x%x.\n",
	        m->m_pkthdr.len,
	        if_name(m->m_pkthdr.rcvif),
	        m->m_pkthdr.csum_flags);
#else
	SCTPDBG(SCTP_DEBUG_CRCOFFLOAD,
	        "sctp_input(): Packet of length %d received on %s with csum_flags 0x%x.\n",
	        m->m_pkthdr.len,
	        m->m_pkthdr.rcvif->if_xname,
	        m->m_pkthdr.csum_flags);
#endif
#endif
#if defined(__APPLE__)
	SCTPDBG(SCTP_DEBUG_CRCOFFLOAD,
	        "sctp_input(): Packet of length %d received on %s%d with csum_flags 0x%x.\n",
	        m->m_pkthdr.len,
	        m->m_pkthdr.rcvif->if_name,
	        m->m_pkthdr.rcvif->if_unit,
	        m->m_pkthdr.csum_flags);
#endif
#if defined(__Windows__)
	SCTPDBG(SCTP_DEBUG_CRCOFFLOAD,
	        "sctp_input(): Packet of length %d received on %s with csum_flags 0x%x.\n",
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
	if (SCTP_BUF_LEN(m) < offset) {
		if ((m = m_pullup(m, offset)) == NULL) {
			SCTP_STAT_INCR(sctps_hdrops);
			return;
		}
	}
	ip = mtod(m, struct ip *);
	sh = (struct sctphdr *)((caddr_t)ip + iphlen);
	ch = (struct sctp_chunkhdr *)((caddr_t)sh + sizeof(struct sctphdr));
	offset -= sizeof(struct sctp_chunkhdr);
	memset(&src, 0, sizeof(struct sockaddr_in));
	src.sin_family = AF_INET;
#ifdef HAVE_SIN_LEN
	src.sin_len = sizeof(struct sockaddr_in);
#endif
	src.sin_port = sh->src_port;
	src.sin_addr = ip->ip_src;
	memset(&dst, 0, sizeof(struct sockaddr_in));
	dst.sin_family = AF_INET;
#ifdef HAVE_SIN_LEN
	dst.sin_len = sizeof(struct sockaddr_in);
#endif
	dst.sin_port = sh->dest_port;
	dst.sin_addr = ip->ip_dst;
#if defined(__Windows__)
	NTOHS(ip->ip_len);
#endif
#if defined(__Userspace_os_Linux) || defined(__Userspace_os_Windows)
	ip->ip_len = ntohs(ip->ip_len);
#endif
#if defined(__FreeBSD__)
#if __FreeBSD_version >= 1000000
	length = ntohs(ip->ip_len);
#else
	length = ip->ip_len + iphlen;
#endif
#elif defined(__APPLE__)
	length = ip->ip_len + iphlen;
#elif defined(__Userspace__)
#if defined(__Userspace_os_Linux) || defined(__Userspace_os_Windows)
	length = ip->ip_len;
#else
	length = ip->ip_len + iphlen;
#endif
#else
	length = ip->ip_len;
#endif
	
	if (SCTP_HEADER_LEN(m) != length) {
		SCTPDBG(SCTP_DEBUG_INPUT1,
		        "sctp_input() length:%d reported length:%d\n", length, SCTP_HEADER_LEN(m));
		SCTP_STAT_INCR(sctps_hdrops);
		goto out;
	}
	
	if (IN_MULTICAST(ntohl(dst.sin_addr.s_addr))) {
		goto out;
	}
	if (SCTP_IS_IT_BROADCAST(dst.sin_addr, m)) {
		goto out;
	}
	ecn_bits = ip->ip_tos;
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
	    ((src.sin_addr.s_addr == dst.sin_addr.s_addr) ||
	     (SCTP_IS_IT_LOOPBACK(m)))) {
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
	return;
}

#if defined(__FreeBSD__) && defined(SCTP_MCORE_INPUT) && defined(SMP)
extern int *sctp_cpuarry;
#endif

void
sctp_input(struct mbuf *m, int off)
{
#if defined(__FreeBSD__) && defined(SCTP_MCORE_INPUT) && defined(SMP)
	struct ip *ip;
	struct sctphdr *sh;
	int offset;
	int cpu_to_use;
	uint32_t flowid, tag;

	if (mp_ncpus > 1) {
		if (m->m_flags & M_FLOWID) {
			flowid = m->m_pkthdr.flowid;
		} else {
			


			offset = off + sizeof(struct sctphdr);
			if (SCTP_BUF_LEN(m) < offset) {
				if ((m = m_pullup(m, offset)) == NULL) {
					SCTP_STAT_INCR(sctps_hdrops);
					return;
				}
			}
			ip = mtod(m, struct ip *);
			sh = (struct sctphdr *)((caddr_t)ip + off);
			tag = htonl(sh->v_tag);
			flowid = tag ^ ntohs(sh->dest_port) ^ ntohs(sh->src_port);
			m->m_pkthdr.flowid = flowid;
			m->m_flags |= M_FLOWID;
		}
		cpu_to_use = sctp_cpuarry[flowid % mp_ncpus];
		sctp_queue_to_mcore(m, off, cpu_to_use);
		return;
	}
#endif
	sctp_input_with_port(m, off, 0);
}
#endif
#endif
