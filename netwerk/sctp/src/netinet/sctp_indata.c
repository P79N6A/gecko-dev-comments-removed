































#ifdef __FreeBSD__
#include <sys/cdefs.h>
__FBSDID("$FreeBSD: head/sys/netinet/sctp_indata.c 246674 2013-02-11 13:57:03Z tuexen $");
#endif

#include <netinet/sctp_os.h>
#include <netinet/sctp_var.h>
#include <netinet/sctp_sysctl.h>
#include <netinet/sctp_pcb.h>
#include <netinet/sctp_header.h>
#include <netinet/sctputil.h>
#include <netinet/sctp_output.h>
#include <netinet/sctp_input.h>
#include <netinet/sctp_indata.h>
#include <netinet/sctp_uio.h>
#include <netinet/sctp_timer.h>












void
sctp_set_rwnd(struct sctp_tcb *stcb, struct sctp_association *asoc)
{
	asoc->my_rwnd = sctp_calc_rwnd(stcb, asoc);
}


uint32_t
sctp_calc_rwnd(struct sctp_tcb *stcb, struct sctp_association *asoc)
{
	uint32_t calc = 0;

	





	if (stcb->sctp_socket == NULL)
		return (calc);

	if (stcb->asoc.sb_cc == 0 &&
	    asoc->size_on_reasm_queue == 0 &&
	    asoc->size_on_all_streams == 0) {
		
		calc = max(SCTP_SB_LIMIT_RCV(stcb->sctp_socket), SCTP_MINIMAL_RWND);
		return (calc);
	}
	
	calc = (uint32_t) sctp_sbspace(&stcb->asoc, &stcb->sctp_socket->so_rcv);

	



	calc = sctp_sbspace_sub(calc, (uint32_t)(asoc->size_on_reasm_queue +
	                                         asoc->cnt_on_reasm_queue * MSIZE));
	calc = sctp_sbspace_sub(calc, (uint32_t)(asoc->size_on_all_streams +
	                                         asoc->cnt_on_all_streams * MSIZE));

	if (calc == 0) {
		
		return (calc);
	}

	
	calc = sctp_sbspace_sub(calc, stcb->asoc.my_rwnd_control_len);
	


	if (calc < stcb->asoc.my_rwnd_control_len) {
		calc = 1;
	}
	return (calc);
}






struct sctp_queued_to_read *
sctp_build_readq_entry(struct sctp_tcb *stcb,
    struct sctp_nets *net,
    uint32_t tsn, uint32_t ppid,
    uint32_t context, uint16_t stream_no,
    uint16_t stream_seq, uint8_t flags,
    struct mbuf *dm)
{
	struct sctp_queued_to_read *read_queue_e = NULL;

	sctp_alloc_a_readq(stcb, read_queue_e);
	if (read_queue_e == NULL) {
		goto failed_build;
	}
	read_queue_e->sinfo_stream = stream_no;
	read_queue_e->sinfo_ssn = stream_seq;
	read_queue_e->sinfo_flags = (flags << 8);
	read_queue_e->sinfo_ppid = ppid;
	read_queue_e->sinfo_context = context;
	read_queue_e->sinfo_timetolive = 0;
	read_queue_e->sinfo_tsn = tsn;
	read_queue_e->sinfo_cumtsn = tsn;
	read_queue_e->sinfo_assoc_id = sctp_get_associd(stcb);
	read_queue_e->whoFrom = net;
	read_queue_e->length = 0;
	atomic_add_int(&net->ref_count, 1);
	read_queue_e->data = dm;
	read_queue_e->spec_flags = 0;
	read_queue_e->tail_mbuf = NULL;
	read_queue_e->aux_data = NULL;
	read_queue_e->stcb = stcb;
	read_queue_e->port_from = stcb->rport;
	read_queue_e->do_not_ref_stcb = 0;
	read_queue_e->end_added = 0;
	read_queue_e->some_taken = 0;
	read_queue_e->pdapi_aborted = 0;
failed_build:
	return (read_queue_e);
}





static struct sctp_queued_to_read *
sctp_build_readq_entry_chk(struct sctp_tcb *stcb,
    struct sctp_tmit_chunk *chk)
{
	struct sctp_queued_to_read *read_queue_e = NULL;

	sctp_alloc_a_readq(stcb, read_queue_e);
	if (read_queue_e == NULL) {
		goto failed_build;
	}
	read_queue_e->sinfo_stream = chk->rec.data.stream_number;
	read_queue_e->sinfo_ssn = chk->rec.data.stream_seq;
	read_queue_e->sinfo_flags = (chk->rec.data.rcv_flags << 8);
	read_queue_e->sinfo_ppid = chk->rec.data.payloadtype;
	read_queue_e->sinfo_context = stcb->asoc.context;
	read_queue_e->sinfo_timetolive = 0;
	read_queue_e->sinfo_tsn = chk->rec.data.TSN_seq;
	read_queue_e->sinfo_cumtsn = chk->rec.data.TSN_seq;
	read_queue_e->sinfo_assoc_id = sctp_get_associd(stcb);
	read_queue_e->whoFrom = chk->whoTo;
	read_queue_e->aux_data = NULL;
	read_queue_e->length = 0;
	atomic_add_int(&chk->whoTo->ref_count, 1);
	read_queue_e->data = chk->data;
	read_queue_e->tail_mbuf = NULL;
	read_queue_e->stcb = stcb;
	read_queue_e->port_from = stcb->rport;
	read_queue_e->spec_flags = 0;
	read_queue_e->do_not_ref_stcb = 0;
	read_queue_e->end_added = 0;
	read_queue_e->some_taken = 0;
	read_queue_e->pdapi_aborted = 0;
failed_build:
	return (read_queue_e);
}


struct mbuf *
sctp_build_ctl_nchunk(struct sctp_inpcb *inp, struct sctp_sndrcvinfo *sinfo)
{
	struct sctp_extrcvinfo *seinfo;
	struct sctp_sndrcvinfo *outinfo;
	struct sctp_rcvinfo *rcvinfo;
	struct sctp_nxtinfo *nxtinfo;
#if defined(__Userspace_os_Windows)
	WSACMSGHDR *cmh;
#else
	struct cmsghdr *cmh;
#endif
	struct mbuf *ret;
	int len;
	int use_extended;
	int provide_nxt;

	if (sctp_is_feature_off(inp, SCTP_PCB_FLAGS_RECVDATAIOEVNT) &&
	    sctp_is_feature_off(inp, SCTP_PCB_FLAGS_RECVRCVINFO) &&
	    sctp_is_feature_off(inp, SCTP_PCB_FLAGS_RECVNXTINFO)) {
		
		return (NULL);
	}

	len = 0;
	if (sctp_is_feature_on(inp, SCTP_PCB_FLAGS_RECVRCVINFO)) {
		len += CMSG_SPACE(sizeof(struct sctp_rcvinfo));
	}
	seinfo = (struct sctp_extrcvinfo *)sinfo;
	if (sctp_is_feature_on(inp, SCTP_PCB_FLAGS_RECVNXTINFO) &&
	    (seinfo->sreinfo_next_flags & SCTP_NEXT_MSG_AVAIL)) {
		provide_nxt = 1;
		len += CMSG_SPACE(sizeof(struct sctp_rcvinfo));
	} else {
		provide_nxt = 0;
	}
	if (sctp_is_feature_on(inp, SCTP_PCB_FLAGS_RECVDATAIOEVNT)) {
		if (sctp_is_feature_on(inp, SCTP_PCB_FLAGS_EXT_RCVINFO)) {
			use_extended = 1;
			len += CMSG_SPACE(sizeof(struct sctp_extrcvinfo));
		} else {
			use_extended = 0;
			len += CMSG_SPACE(sizeof(struct sctp_sndrcvinfo));
		}
	} else {
		use_extended = 0;
	}

	ret = sctp_get_mbuf_for_msg(len, 0, M_NOWAIT, 1, MT_DATA);
	if (ret == NULL) {
		
		return (ret);
	}
	SCTP_BUF_LEN(ret) = 0;

	
#if defined(__Userspace_os_Windows)
	cmh = mtod(ret, WSACMSGHDR *);
#else
	cmh = mtod(ret, struct cmsghdr *);
#endif
	if (sctp_is_feature_on(inp, SCTP_PCB_FLAGS_RECVRCVINFO)) {
		cmh->cmsg_level = IPPROTO_SCTP;
		cmh->cmsg_len = CMSG_LEN(sizeof(struct sctp_rcvinfo));
		cmh->cmsg_type = SCTP_RCVINFO;
		rcvinfo = (struct sctp_rcvinfo *)CMSG_DATA(cmh);
		rcvinfo->rcv_sid = sinfo->sinfo_stream;
		rcvinfo->rcv_ssn = sinfo->sinfo_ssn;
		rcvinfo->rcv_flags = sinfo->sinfo_flags;
		rcvinfo->rcv_ppid = sinfo->sinfo_ppid;
		rcvinfo->rcv_tsn = sinfo->sinfo_tsn;
		rcvinfo->rcv_cumtsn = sinfo->sinfo_cumtsn;
		rcvinfo->rcv_context = sinfo->sinfo_context;
		rcvinfo->rcv_assoc_id = sinfo->sinfo_assoc_id;
#if defined(__Userspace_os_Windows)
		cmh = (WSACMSGHDR *)((caddr_t)cmh + CMSG_SPACE(sizeof(struct sctp_rcvinfo)));
#else
		cmh = (struct cmsghdr *)((caddr_t)cmh + CMSG_SPACE(sizeof(struct sctp_rcvinfo)));
#endif
		SCTP_BUF_LEN(ret) += CMSG_SPACE(sizeof(struct sctp_rcvinfo));
	}
	if (provide_nxt) {
		cmh->cmsg_level = IPPROTO_SCTP;
		cmh->cmsg_len = CMSG_LEN(sizeof(struct sctp_nxtinfo));
		cmh->cmsg_type = SCTP_NXTINFO;
		nxtinfo = (struct sctp_nxtinfo *)CMSG_DATA(cmh);
		nxtinfo->nxt_sid = seinfo->sreinfo_next_stream;
		nxtinfo->nxt_flags = 0;
		if (seinfo->sreinfo_next_flags & SCTP_NEXT_MSG_IS_UNORDERED) {
			nxtinfo->nxt_flags |= SCTP_UNORDERED;
		}
		if (seinfo->sreinfo_next_flags & SCTP_NEXT_MSG_IS_NOTIFICATION) {
			nxtinfo->nxt_flags |= SCTP_NOTIFICATION;
		}
		if (seinfo->sreinfo_next_flags & SCTP_NEXT_MSG_ISCOMPLETE) {
			nxtinfo->nxt_flags |= SCTP_COMPLETE;
		}
		nxtinfo->nxt_ppid = seinfo->sreinfo_next_ppid;
		nxtinfo->nxt_length = seinfo->sreinfo_next_length;
		nxtinfo->nxt_assoc_id = seinfo->sreinfo_next_aid;
#if defined(__Userspace_os_Windows)
		cmh = (WSACMSGHDR *)((caddr_t)cmh + CMSG_SPACE(sizeof(struct sctp_nxtinfo)));
#else
		cmh = (struct cmsghdr *)((caddr_t)cmh + CMSG_SPACE(sizeof(struct sctp_nxtinfo)));
#endif
		SCTP_BUF_LEN(ret) += CMSG_SPACE(sizeof(struct sctp_nxtinfo));
	}
	if (sctp_is_feature_on(inp, SCTP_PCB_FLAGS_RECVDATAIOEVNT)) {
		cmh->cmsg_level = IPPROTO_SCTP;
		outinfo = (struct sctp_sndrcvinfo *)CMSG_DATA(cmh);
		if (use_extended) {
			cmh->cmsg_len = CMSG_LEN(sizeof(struct sctp_extrcvinfo));
			cmh->cmsg_type = SCTP_EXTRCV;
			memcpy(outinfo, sinfo, sizeof(struct sctp_extrcvinfo));
			SCTP_BUF_LEN(ret) += CMSG_SPACE(sizeof(struct sctp_extrcvinfo));
		} else {
			cmh->cmsg_len = CMSG_LEN(sizeof(struct sctp_sndrcvinfo));
			cmh->cmsg_type = SCTP_SNDRCV;
			*outinfo = *sinfo;
			SCTP_BUF_LEN(ret) += CMSG_SPACE(sizeof(struct sctp_sndrcvinfo));
		}
	}
	return (ret);
}


static void
sctp_mark_non_revokable(struct sctp_association *asoc, uint32_t tsn)
{
	uint32_t gap, i, cumackp1;
	int fnd = 0;

	if (SCTP_BASE_SYSCTL(sctp_do_drain) == 0) {
		return;
	}
	cumackp1 = asoc->cumulative_tsn + 1;
	if (SCTP_TSN_GT(cumackp1, tsn)) {
		


		return;
	}
	SCTP_CALC_TSN_TO_GAP(gap, tsn, asoc->mapping_array_base_tsn);
	if (!SCTP_IS_TSN_PRESENT(asoc->mapping_array, gap)) {
		SCTP_PRINTF("gap:%x tsn:%x\n", gap, tsn);
		sctp_print_mapping_array(asoc);
#ifdef INVARIANTS
		panic("Things are really messed up now!!");
#endif
	}
	SCTP_SET_TSN_PRESENT(asoc->nr_mapping_array, gap);
	SCTP_UNSET_TSN_PRESENT(asoc->mapping_array, gap);
	if (SCTP_TSN_GT(tsn, asoc->highest_tsn_inside_nr_map)) {
		asoc->highest_tsn_inside_nr_map = tsn;
	}
	if (tsn == asoc->highest_tsn_inside_map) {
		
		for (i = tsn - 1; SCTP_TSN_GE(i, asoc->mapping_array_base_tsn); i--) {
			SCTP_CALC_TSN_TO_GAP(gap, i, asoc->mapping_array_base_tsn);
			if (SCTP_IS_TSN_PRESENT(asoc->mapping_array, gap)) {
				asoc->highest_tsn_inside_map = i;
				fnd = 1;
				break;
			}
		}
		if (!fnd) {
			asoc->highest_tsn_inside_map = asoc->mapping_array_base_tsn - 1;
		}
	}
}







static void
sctp_service_reassembly(struct sctp_tcb *stcb, struct sctp_association *asoc)
{
	struct sctp_tmit_chunk *chk, *nchk;
	uint16_t nxt_todel;
	uint16_t stream_no;
	int end = 0;
	int cntDel;
	struct sctp_queued_to_read *control, *ctl, *nctl;

	if (stcb == NULL)
		return;

	cntDel = stream_no = 0;
	if ((stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_SOCKET_GONE) ||
	     (stcb->asoc.state & SCTP_STATE_ABOUT_TO_BE_FREED) ||
	     (stcb->asoc.state & SCTP_STATE_CLOSED_SOCKET)) {
		
	abandon:
		asoc->fragmented_delivery_inprogress = 0;
		TAILQ_FOREACH_SAFE(chk, &asoc->reasmqueue, sctp_next, nchk) {
			TAILQ_REMOVE(&asoc->reasmqueue, chk, sctp_next);
			asoc->size_on_reasm_queue -= chk->send_size;
			sctp_ucount_decr(asoc->cnt_on_reasm_queue);
			



			if (chk->data) {
				sctp_m_freem(chk->data);
				chk->data = NULL;
			}
			
			sctp_free_a_chunk(stcb, chk, SCTP_SO_NOT_LOCKED);
        		
		}
		return;
	}
	SCTP_TCB_LOCK_ASSERT(stcb);
	TAILQ_FOREACH_SAFE(chk, &asoc->reasmqueue, sctp_next, nchk) {
		if (chk->rec.data.TSN_seq != (asoc->tsn_last_delivered + 1)) {
			
			return;
		}
		stream_no = chk->rec.data.stream_number;
		nxt_todel = asoc->strmin[stream_no].last_sequence_delivered + 1;
		if (nxt_todel != chk->rec.data.stream_seq &&
		    (chk->rec.data.rcv_flags & SCTP_DATA_UNORDERED) == 0) {
			



			return;
		}
		if (chk->rec.data.rcv_flags & SCTP_DATA_FIRST_FRAG) {

			control = sctp_build_readq_entry_chk(stcb, chk);
			if (control == NULL) {
				
				return;
			}
			
			stcb->asoc.control_pdapi = control;
			if (chk->rec.data.rcv_flags & SCTP_DATA_LAST_FRAG)
				end = 1;
			else
				end = 0;
			sctp_mark_non_revokable(asoc, chk->rec.data.TSN_seq);
			sctp_add_to_readq(stcb->sctp_ep,
			                  stcb, control, &stcb->sctp_socket->so_rcv, end,
			                  SCTP_READ_LOCK_NOT_HELD, SCTP_SO_NOT_LOCKED);
			cntDel++;
		} else {
			if (chk->rec.data.rcv_flags & SCTP_DATA_LAST_FRAG)
				end = 1;
			else
				end = 0;
			sctp_mark_non_revokable(asoc, chk->rec.data.TSN_seq);
			if (sctp_append_to_readq(stcb->sctp_ep, stcb,
			    stcb->asoc.control_pdapi,
			    chk->data, end, chk->rec.data.TSN_seq,
			    &stcb->sctp_socket->so_rcv)) {
				





				if (stcb->asoc.state & SCTP_STATE_ABOUT_TO_BE_FREED) {
					goto abandon;
				} else {
#ifdef INVARIANTS
					if ((stcb->asoc.control_pdapi == NULL)  || (stcb->asoc.control_pdapi->tail_mbuf == NULL)) {
						panic("This should not happen control_pdapi NULL?");
					}
					
					panic("Bad chunking ??");
#else
					if ((stcb->asoc.control_pdapi == NULL)  || (stcb->asoc.control_pdapi->tail_mbuf == NULL)) {
					  SCTP_PRINTF("This should not happen control_pdapi NULL?\n");
					}
					SCTP_PRINTF("Bad chunking ??\n");
					SCTP_PRINTF("Dumping re-assembly queue this will probably hose the association\n");

#endif
					goto abandon;
				}
			}
			cntDel++;
		}
		
		TAILQ_REMOVE(&asoc->reasmqueue, chk, sctp_next);
		if (chk->rec.data.rcv_flags & SCTP_DATA_LAST_FRAG) {
			asoc->fragmented_delivery_inprogress = 0;
			if ((chk->rec.data.rcv_flags & SCTP_DATA_UNORDERED) == 0) {
				asoc->strmin[stream_no].last_sequence_delivered++;
			}
			if ((chk->rec.data.rcv_flags & SCTP_DATA_FIRST_FRAG) == 0) {
				SCTP_STAT_INCR_COUNTER64(sctps_reasmusrmsgs);
			}
		} else if (chk->rec.data.rcv_flags & SCTP_DATA_FIRST_FRAG) {
			



			asoc->fragmented_delivery_inprogress = 1;
		}
		asoc->tsn_of_pdapi_last_delivered = chk->rec.data.TSN_seq;
		asoc->last_flags_delivered = chk->rec.data.rcv_flags;
		asoc->last_strm_seq_delivered = chk->rec.data.stream_seq;
		asoc->last_strm_no_delivered = chk->rec.data.stream_number;

		asoc->tsn_last_delivered = chk->rec.data.TSN_seq;
		asoc->size_on_reasm_queue -= chk->send_size;
		sctp_ucount_decr(asoc->cnt_on_reasm_queue);
		
		chk->data = NULL;
		sctp_free_a_chunk(stcb, chk, SCTP_SO_NOT_LOCKED);

		if (asoc->fragmented_delivery_inprogress == 0) {
			



			struct sctp_stream_in *strm;

			strm = &asoc->strmin[stream_no];
			nxt_todel = strm->last_sequence_delivered + 1;
			TAILQ_FOREACH_SAFE(ctl, &strm->inqueue, next, nctl) {
				
				if (nxt_todel == ctl->sinfo_ssn) {
					TAILQ_REMOVE(&strm->inqueue, ctl, next);
					asoc->size_on_all_streams -= ctl->length;
					sctp_ucount_decr(asoc->cnt_on_all_streams);
					strm->last_sequence_delivered++;
					sctp_mark_non_revokable(asoc, ctl->sinfo_tsn);
					sctp_add_to_readq(stcb->sctp_ep, stcb,
					                  ctl,
					                  &stcb->sctp_socket->so_rcv, 1,
					                  SCTP_READ_LOCK_NOT_HELD, SCTP_SO_NOT_LOCKED);
				} else {
					break;
				}
				nxt_todel = strm->last_sequence_delivered + 1;
			}
			break;
		}
	}
}







static void
sctp_queue_data_to_stream(struct sctp_tcb *stcb, struct sctp_association *asoc,
    struct sctp_queued_to_read *control, int *abort_flag)
{
	

















	struct sctp_stream_in *strm;
	struct sctp_queued_to_read *at;
	int queue_needed;
	uint16_t nxt_todel;
	struct mbuf *oper;

	queue_needed = 1;
	asoc->size_on_all_streams += control->length;
	sctp_ucount_incr(asoc->cnt_on_all_streams);
	strm = &asoc->strmin[control->sinfo_stream];
	nxt_todel = strm->last_sequence_delivered + 1;
	if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_STR_LOGGING_ENABLE) {
		sctp_log_strm_del(control, NULL, SCTP_STR_LOG_FROM_INTO_STRD);
	}
	SCTPDBG(SCTP_DEBUG_INDATA1,
		"queue to stream called for ssn:%u lastdel:%u nxt:%u\n",
		(uint32_t) control->sinfo_stream,
		(uint32_t) strm->last_sequence_delivered,
		(uint32_t) nxt_todel);
	if (SCTP_SSN_GE(strm->last_sequence_delivered, control->sinfo_ssn)) {
		
		SCTPDBG(SCTP_DEBUG_INDATA1, "Duplicate S-SEQ:%d delivered:%d from peer, Abort  association\n",
			control->sinfo_ssn, strm->last_sequence_delivered);
	protocol_error:
		



		TAILQ_INSERT_HEAD(&strm->inqueue, control, next);
		oper = sctp_get_mbuf_for_msg((sizeof(struct sctp_paramhdr) + 3 * sizeof(uint32_t)),
					     0, M_NOWAIT, 1, MT_DATA);
		if (oper) {
			struct sctp_paramhdr *ph;
			uint32_t *ippp;
			SCTP_BUF_LEN(oper) = sizeof(struct sctp_paramhdr) +
			    (sizeof(uint32_t) * 3);
			ph = mtod(oper, struct sctp_paramhdr *);
			ph->param_type = htons(SCTP_CAUSE_PROTOCOL_VIOLATION);
			ph->param_length = htons(SCTP_BUF_LEN(oper));
			ippp = (uint32_t *) (ph + 1);
			*ippp = htonl(SCTP_FROM_SCTP_INDATA+SCTP_LOC_1);
			ippp++;
			*ippp = control->sinfo_tsn;
			ippp++;
			*ippp = ((control->sinfo_stream << 16) | control->sinfo_ssn);
		}
		stcb->sctp_ep->last_abort_code = SCTP_FROM_SCTP_INDATA+SCTP_LOC_1;
		sctp_abort_an_association(stcb->sctp_ep, stcb, oper, SCTP_SO_NOT_LOCKED);
		*abort_flag = 1;
		return;

	}
	if (nxt_todel == control->sinfo_ssn) {
		
		if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_STR_LOGGING_ENABLE) {
			sctp_log_strm_del(control, NULL, SCTP_STR_LOG_FROM_IMMED_DEL);
		}
		
		queue_needed = 0;
		asoc->size_on_all_streams -= control->length;
		sctp_ucount_decr(asoc->cnt_on_all_streams);
		strm->last_sequence_delivered++;

		sctp_mark_non_revokable(asoc, control->sinfo_tsn);
		sctp_add_to_readq(stcb->sctp_ep, stcb,
		                  control,
		                  &stcb->sctp_socket->so_rcv, 1,
		                  SCTP_READ_LOCK_NOT_HELD, SCTP_SO_NOT_LOCKED);
		TAILQ_FOREACH_SAFE(control, &strm->inqueue, next, at) {
			
			nxt_todel = strm->last_sequence_delivered + 1;
			if (nxt_todel == control->sinfo_ssn) {
				TAILQ_REMOVE(&strm->inqueue, control, next);
				asoc->size_on_all_streams -= control->length;
				sctp_ucount_decr(asoc->cnt_on_all_streams);
				strm->last_sequence_delivered++;
				





				if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_STR_LOGGING_ENABLE) {
					sctp_log_strm_del(control, NULL,
							  SCTP_STR_LOG_FROM_IMMED_DEL);
				}
				sctp_mark_non_revokable(asoc, control->sinfo_tsn);
				sctp_add_to_readq(stcb->sctp_ep, stcb,
				                  control,
				                  &stcb->sctp_socket->so_rcv, 1,
				                  SCTP_READ_LOCK_NOT_HELD,
				                  SCTP_SO_NOT_LOCKED);
				continue;
			}
			break;
		}
	}
	if (queue_needed) {
		



  	        if (SCTP_TSN_GE(asoc->cumulative_tsn, control->sinfo_tsn)) {
		        goto protocol_error;
		}
		if (TAILQ_EMPTY(&strm->inqueue)) {
			
			if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_STR_LOGGING_ENABLE) {
				sctp_log_strm_del(control, NULL, SCTP_STR_LOG_FROM_INSERT_HD);
			}
			TAILQ_INSERT_HEAD(&strm->inqueue, control, next);
		} else {
			TAILQ_FOREACH(at, &strm->inqueue, next) {
				if (SCTP_SSN_GT(at->sinfo_ssn, control->sinfo_ssn)) {
					



					if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_STR_LOGGING_ENABLE) {
						sctp_log_strm_del(control, at,
								  SCTP_STR_LOG_FROM_INSERT_MD);
					}
					TAILQ_INSERT_BEFORE(at, control, next);
					break;
				} else if (at->sinfo_ssn == control->sinfo_ssn) {
					



					









					if (control->data)
						sctp_m_freem(control->data);
					control->data = NULL;
					asoc->size_on_all_streams -= control->length;
					sctp_ucount_decr(asoc->cnt_on_all_streams);
					if (control->whoFrom) {
						sctp_free_remote_addr(control->whoFrom);
						control->whoFrom = NULL;
					}
					sctp_free_a_readq(stcb, control);
					return;
				} else {
					if (TAILQ_NEXT(at, next) == NULL) {
						



						if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_STR_LOGGING_ENABLE) {
							sctp_log_strm_del(control, at,
									  SCTP_STR_LOG_FROM_INSERT_TL);
						}
						TAILQ_INSERT_AFTER(&strm->inqueue,
						    at, control, next);
						break;
					}
				}
			}
		}
	}
}






static int
sctp_is_all_msg_on_reasm(struct sctp_association *asoc, uint32_t *t_size)
{
	struct sctp_tmit_chunk *chk;
	uint32_t tsn;

	*t_size = 0;
	chk = TAILQ_FIRST(&asoc->reasmqueue);
	if (chk == NULL) {
		
		return (0);
	}
	if ((chk->rec.data.rcv_flags & SCTP_DATA_FIRST_FRAG) == 0) {
		
		return (0);
	}
	tsn = chk->rec.data.TSN_seq;
	TAILQ_FOREACH(chk, &asoc->reasmqueue, sctp_next) {
		if (tsn != chk->rec.data.TSN_seq) {
			return (0);
		}
		*t_size += chk->send_size;
		if (chk->rec.data.rcv_flags & SCTP_DATA_LAST_FRAG) {
			return (1);
		}
		tsn++;
	}
	return (0);
}

static void
sctp_deliver_reasm_check(struct sctp_tcb *stcb, struct sctp_association *asoc)
{
	struct sctp_tmit_chunk *chk;
	uint16_t nxt_todel;
	uint32_t tsize, pd_point;

 doit_again:
	chk = TAILQ_FIRST(&asoc->reasmqueue);
	if (chk == NULL) {
		
		asoc->size_on_reasm_queue = 0;
		asoc->cnt_on_reasm_queue = 0;
		return;
	}
	if (asoc->fragmented_delivery_inprogress == 0) {
		nxt_todel =
		    asoc->strmin[chk->rec.data.stream_number].last_sequence_delivered + 1;
		if ((chk->rec.data.rcv_flags & SCTP_DATA_FIRST_FRAG) &&
		    (nxt_todel == chk->rec.data.stream_seq ||
		    (chk->rec.data.rcv_flags & SCTP_DATA_UNORDERED))) {
			



			if (stcb->sctp_socket) {
				pd_point = min(SCTP_SB_LIMIT_RCV(stcb->sctp_socket),
				               stcb->sctp_ep->partial_delivery_point);
			} else {
				pd_point = stcb->sctp_ep->partial_delivery_point;
			}
			if (sctp_is_all_msg_on_reasm(asoc, &tsize) || (tsize >= pd_point)) {

				




				asoc->fragmented_delivery_inprogress = 1;
				asoc->tsn_last_delivered =
				    chk->rec.data.TSN_seq - 1;
				asoc->str_of_pdapi =
				    chk->rec.data.stream_number;
				asoc->ssn_of_pdapi = chk->rec.data.stream_seq;
				asoc->pdapi_ppid = chk->rec.data.payloadtype;
				asoc->fragment_flags = chk->rec.data.rcv_flags;
				sctp_service_reassembly(stcb, asoc);
			}
		}
	} else {
		




		sctp_service_reassembly(stcb, asoc);
		if (asoc->fragmented_delivery_inprogress == 0) {
			


			goto doit_again;
		}
	}
}







static void
sctp_queue_data_for_reasm(struct sctp_tcb *stcb, struct sctp_association *asoc,
    struct sctp_tmit_chunk *chk, int *abort_flag)
{
	struct mbuf *oper;
	uint32_t cum_ackp1, prev_tsn, post_tsn;
	struct sctp_tmit_chunk *at, *prev, *next;

	prev = next = NULL;
	cum_ackp1 = asoc->tsn_last_delivered + 1;
	if (TAILQ_EMPTY(&asoc->reasmqueue)) {
		
		TAILQ_INSERT_HEAD(&asoc->reasmqueue, chk, sctp_next);
		



		asoc->size_on_reasm_queue = chk->send_size;
		sctp_ucount_incr(asoc->cnt_on_reasm_queue);
		if (chk->rec.data.TSN_seq == cum_ackp1) {
			if (asoc->fragmented_delivery_inprogress == 0 &&
			    (chk->rec.data.rcv_flags & SCTP_DATA_FIRST_FRAG) !=
			    SCTP_DATA_FIRST_FRAG) {
				




				SCTPDBG(SCTP_DEBUG_INDATA1, "Gak, Evil plot, its not first, no fragmented delivery in progress\n");
				oper = sctp_get_mbuf_for_msg((sizeof(struct sctp_paramhdr) + 3 * sizeof(uint32_t)),
							       0, M_NOWAIT, 1, MT_DATA);

				if (oper) {
					struct sctp_paramhdr *ph;
					uint32_t *ippp;

					SCTP_BUF_LEN(oper) =
					    sizeof(struct sctp_paramhdr) +
					    (sizeof(uint32_t) * 3);
					ph = mtod(oper, struct sctp_paramhdr *);
					ph->param_type =
					    htons(SCTP_CAUSE_PROTOCOL_VIOLATION);
					ph->param_length = htons(SCTP_BUF_LEN(oper));
					ippp = (uint32_t *) (ph + 1);
					*ippp = htonl(SCTP_FROM_SCTP_INDATA+SCTP_LOC_2);
					ippp++;
					*ippp = chk->rec.data.TSN_seq;
					ippp++;
					*ippp = ((chk->rec.data.stream_number << 16) | chk->rec.data.stream_seq);

				}
				stcb->sctp_ep->last_abort_code = SCTP_FROM_SCTP_INDATA+SCTP_LOC_2;
				sctp_abort_an_association(stcb->sctp_ep, stcb, oper, SCTP_SO_NOT_LOCKED);
				*abort_flag = 1;
			} else if (asoc->fragmented_delivery_inprogress &&
			    (chk->rec.data.rcv_flags & SCTP_DATA_FIRST_FRAG) == SCTP_DATA_FIRST_FRAG) {
				




				SCTPDBG(SCTP_DEBUG_INDATA1, "Gak, Evil plot, it IS a first and fragmented delivery in progress\n");
				oper = sctp_get_mbuf_for_msg((sizeof(struct sctp_paramhdr) + 3 * sizeof(uint32_t)),
							     0, M_NOWAIT, 1, MT_DATA);
				if (oper) {
					struct sctp_paramhdr *ph;
					uint32_t *ippp;

					SCTP_BUF_LEN(oper) =
					    sizeof(struct sctp_paramhdr) +
					    (3 *sizeof(uint32_t));
					ph = mtod(oper, struct sctp_paramhdr *);
					ph->param_type =
					    htons(SCTP_CAUSE_PROTOCOL_VIOLATION);
					ph->param_length = htons(SCTP_BUF_LEN(oper));
					ippp = (uint32_t *) (ph + 1);
					*ippp = htonl(SCTP_FROM_SCTP_INDATA+SCTP_LOC_3);
					ippp++;
					*ippp = chk->rec.data.TSN_seq;
					ippp++;
					*ippp = ((chk->rec.data.stream_number << 16) | chk->rec.data.stream_seq);
				}
				stcb->sctp_ep->last_abort_code = SCTP_FROM_SCTP_INDATA+SCTP_LOC_3;
				sctp_abort_an_association(stcb->sctp_ep, stcb, oper, SCTP_SO_NOT_LOCKED);
				*abort_flag = 1;
			} else if (asoc->fragmented_delivery_inprogress) {
				



				if (chk->rec.data.stream_number !=
				    asoc->str_of_pdapi) {
					
					SCTPDBG(SCTP_DEBUG_INDATA1, "Gak, Evil plot, it IS not same stream number %d vs %d\n",
						chk->rec.data.stream_number,
						asoc->str_of_pdapi);
					oper = sctp_get_mbuf_for_msg((sizeof(struct sctp_paramhdr) + 3 * sizeof(uint32_t)),
								     0, M_NOWAIT, 1, MT_DATA);
					if (oper) {
						struct sctp_paramhdr *ph;
						uint32_t *ippp;

						SCTP_BUF_LEN(oper) =
						    sizeof(struct sctp_paramhdr) +
						    (sizeof(uint32_t) * 3);
						ph = mtod(oper,
						    struct sctp_paramhdr *);
						ph->param_type =
						    htons(SCTP_CAUSE_PROTOCOL_VIOLATION);
						ph->param_length =
						    htons(SCTP_BUF_LEN(oper));
						ippp = (uint32_t *) (ph + 1);
						*ippp = htonl(SCTP_FROM_SCTP_INDATA+SCTP_LOC_4);
						ippp++;
						*ippp = chk->rec.data.TSN_seq;
						ippp++;
						*ippp = ((chk->rec.data.stream_number << 16) | chk->rec.data.stream_seq);
					}
					stcb->sctp_ep->last_abort_code = SCTP_FROM_SCTP_INDATA+SCTP_LOC_4;
					sctp_abort_an_association(stcb->sctp_ep, stcb, oper, SCTP_SO_NOT_LOCKED);
					*abort_flag = 1;
				} else if ((asoc->fragment_flags & SCTP_DATA_UNORDERED) !=
					    SCTP_DATA_UNORDERED &&
					    chk->rec.data.stream_seq != asoc->ssn_of_pdapi) {
					
					SCTPDBG(SCTP_DEBUG_INDATA1, "Gak, Evil plot, it IS not same stream seq %d vs %d\n",
						chk->rec.data.stream_seq,
						asoc->ssn_of_pdapi);
					oper = sctp_get_mbuf_for_msg((sizeof(struct sctp_paramhdr) + 3 * sizeof(uint32_t)),
								     0, M_NOWAIT, 1, MT_DATA);
					if (oper) {
						struct sctp_paramhdr *ph;
						uint32_t *ippp;

						SCTP_BUF_LEN(oper) =
						    sizeof(struct sctp_paramhdr) +
						    (3 * sizeof(uint32_t));
						ph = mtod(oper,
						    struct sctp_paramhdr *);
						ph->param_type =
						    htons(SCTP_CAUSE_PROTOCOL_VIOLATION);
						ph->param_length =
						    htons(SCTP_BUF_LEN(oper));
						ippp = (uint32_t *) (ph + 1);
						*ippp = htonl(SCTP_FROM_SCTP_INDATA+SCTP_LOC_5);
						ippp++;
						*ippp = chk->rec.data.TSN_seq;
						ippp++;
						*ippp = ((chk->rec.data.stream_number << 16) | chk->rec.data.stream_seq);

					}
					stcb->sctp_ep->last_abort_code = SCTP_FROM_SCTP_INDATA+SCTP_LOC_5;
					sctp_abort_an_association(stcb->sctp_ep, stcb, oper, SCTP_SO_NOT_LOCKED);
					*abort_flag = 1;
				}
			}
		}
		return;
	}
	
	TAILQ_FOREACH(at, &asoc->reasmqueue, sctp_next) {
		if (SCTP_TSN_GT(at->rec.data.TSN_seq, chk->rec.data.TSN_seq)) {
			



			
			asoc->size_on_reasm_queue += chk->send_size;
			sctp_ucount_incr(asoc->cnt_on_reasm_queue);
			next = at;
			TAILQ_INSERT_BEFORE(at, chk, sctp_next);
			break;
		} else if (at->rec.data.TSN_seq == chk->rec.data.TSN_seq) {
			
			






			if (chk->data) {
				sctp_m_freem(chk->data);
				chk->data = NULL;
			}
			sctp_free_a_chunk(stcb, chk, SCTP_SO_NOT_LOCKED);
			return;
		} else {
			prev = at;
			if (TAILQ_NEXT(at, sctp_next) == NULL) {
				



				
				asoc->size_on_reasm_queue += chk->send_size;
				sctp_ucount_incr(asoc->cnt_on_reasm_queue);
				TAILQ_INSERT_AFTER(&asoc->reasmqueue, at, chk, sctp_next);
				break;
			}
		}
	}
	
	if (prev) {
		prev_tsn = chk->rec.data.TSN_seq - 1;
		if (prev_tsn == prev->rec.data.TSN_seq) {
			



			if ((prev->rec.data.rcv_flags & SCTP_DATA_FRAG_MASK) ==
			    SCTP_DATA_FIRST_FRAG ||
			    (prev->rec.data.rcv_flags & SCTP_DATA_FRAG_MASK) ==
			    SCTP_DATA_MIDDLE_FRAG) {
				



				if ((chk->rec.data.rcv_flags & SCTP_DATA_FRAG_MASK) ==
				    SCTP_DATA_FIRST_FRAG) {
					SCTPDBG(SCTP_DEBUG_INDATA1, "Prev check - It can be a midlle or last but not a first\n");
					SCTPDBG(SCTP_DEBUG_INDATA1, "Gak, Evil plot, it's a FIRST!\n");
					oper = sctp_get_mbuf_for_msg((sizeof(struct sctp_paramhdr) + 3 * sizeof(uint32_t)),
								     0, M_NOWAIT, 1, MT_DATA);
					if (oper) {
						struct sctp_paramhdr *ph;
						uint32_t *ippp;

						SCTP_BUF_LEN(oper) =
						    sizeof(struct sctp_paramhdr) +
						    (3 * sizeof(uint32_t));
						ph = mtod(oper,
						    struct sctp_paramhdr *);
						ph->param_type =
						    htons(SCTP_CAUSE_PROTOCOL_VIOLATION);
						ph->param_length =
						    htons(SCTP_BUF_LEN(oper));
						ippp = (uint32_t *) (ph + 1);
						*ippp = htonl(SCTP_FROM_SCTP_INDATA+SCTP_LOC_6);
						ippp++;
						*ippp = chk->rec.data.TSN_seq;
						ippp++;
						*ippp = ((chk->rec.data.stream_number << 16) | chk->rec.data.stream_seq);

					}
					stcb->sctp_ep->last_abort_code = SCTP_FROM_SCTP_INDATA+SCTP_LOC_6;
					sctp_abort_an_association(stcb->sctp_ep, stcb, oper, SCTP_SO_NOT_LOCKED);
					*abort_flag = 1;
					return;
				}
				if (chk->rec.data.stream_number !=
				    prev->rec.data.stream_number) {
					



					SCTP_PRINTF("Prev check - Gak, Evil plot, ssn:%d not the same as at:%d\n",
						    chk->rec.data.stream_number,
						    prev->rec.data.stream_number);
					oper = sctp_get_mbuf_for_msg((sizeof(struct sctp_paramhdr) + 3 * sizeof(uint32_t)),
								     0, M_NOWAIT, 1, MT_DATA);
					if (oper) {
						struct sctp_paramhdr *ph;
						uint32_t *ippp;

						SCTP_BUF_LEN(oper) =
						    sizeof(struct sctp_paramhdr) +
						    (3 * sizeof(uint32_t));
						ph = mtod(oper,
						    struct sctp_paramhdr *);
						ph->param_type =
						    htons(SCTP_CAUSE_PROTOCOL_VIOLATION);
						ph->param_length =
						    htons(SCTP_BUF_LEN(oper));
						ippp = (uint32_t *) (ph + 1);
						*ippp = htonl(SCTP_FROM_SCTP_INDATA+SCTP_LOC_7);
						ippp++;
						*ippp = chk->rec.data.TSN_seq;
						ippp++;
						*ippp = ((chk->rec.data.stream_number << 16) | chk->rec.data.stream_seq);
					}
					stcb->sctp_ep->last_abort_code = SCTP_FROM_SCTP_INDATA+SCTP_LOC_7;
					sctp_abort_an_association(stcb->sctp_ep, stcb, oper, SCTP_SO_NOT_LOCKED);
					*abort_flag = 1;
					return;
				}
				if ((prev->rec.data.rcv_flags & SCTP_DATA_UNORDERED) == 0 &&
				    chk->rec.data.stream_seq !=
				    prev->rec.data.stream_seq) {
					



					SCTPDBG(SCTP_DEBUG_INDATA1, "Prev check - Gak, Evil plot, sseq:%d not the same as at:%d\n",
						chk->rec.data.stream_seq,
						prev->rec.data.stream_seq);
					oper = sctp_get_mbuf_for_msg((sizeof(struct sctp_paramhdr) + 3 * sizeof(uint32_t)),
								     0, M_NOWAIT, 1, MT_DATA);
					if (oper) {
						struct sctp_paramhdr *ph;
						uint32_t *ippp;

						SCTP_BUF_LEN(oper) =
						    sizeof(struct sctp_paramhdr) +
						    (3 * sizeof(uint32_t));
						ph = mtod(oper,
						    struct sctp_paramhdr *);
						ph->param_type =
						    htons(SCTP_CAUSE_PROTOCOL_VIOLATION);
						ph->param_length =
						    htons(SCTP_BUF_LEN(oper));
						ippp = (uint32_t *) (ph + 1);
						*ippp = htonl(SCTP_FROM_SCTP_INDATA+SCTP_LOC_8);
						ippp++;
						*ippp = chk->rec.data.TSN_seq;
						ippp++;
						*ippp = ((chk->rec.data.stream_number << 16) | chk->rec.data.stream_seq);
					}
					stcb->sctp_ep->last_abort_code = SCTP_FROM_SCTP_INDATA+SCTP_LOC_8;
					sctp_abort_an_association(stcb->sctp_ep, stcb, oper, SCTP_SO_NOT_LOCKED);
					*abort_flag = 1;
					return;
				}
			} else if ((prev->rec.data.rcv_flags & SCTP_DATA_FRAG_MASK) ==
			    SCTP_DATA_LAST_FRAG) {
				
				if ((chk->rec.data.rcv_flags & SCTP_DATA_FRAG_MASK) !=
				    SCTP_DATA_FIRST_FRAG) {
					SCTPDBG(SCTP_DEBUG_INDATA1, "Prev check - Gak, evil plot, its not FIRST and it must be!\n");
					oper = sctp_get_mbuf_for_msg((sizeof(struct sctp_paramhdr) + 3 * sizeof(uint32_t)),
								     0, M_NOWAIT, 1, MT_DATA);
					if (oper) {
						struct sctp_paramhdr *ph;
						uint32_t *ippp;

						SCTP_BUF_LEN(oper) =
						    sizeof(struct sctp_paramhdr) +
						    (3 * sizeof(uint32_t));
						ph = mtod(oper,
						    struct sctp_paramhdr *);
						ph->param_type =
						    htons(SCTP_CAUSE_PROTOCOL_VIOLATION);
						ph->param_length =
						    htons(SCTP_BUF_LEN(oper));
						ippp = (uint32_t *) (ph + 1);
						*ippp = htonl(SCTP_FROM_SCTP_INDATA+SCTP_LOC_9);
						ippp++;
						*ippp = chk->rec.data.TSN_seq;
						ippp++;
						*ippp = ((chk->rec.data.stream_number << 16) | chk->rec.data.stream_seq);

					}
					stcb->sctp_ep->last_abort_code = SCTP_FROM_SCTP_INDATA+SCTP_LOC_9;
					sctp_abort_an_association(stcb->sctp_ep, stcb, oper, SCTP_SO_NOT_LOCKED);
					*abort_flag = 1;
					return;
				}
			}
		}
	}
	if (next) {
		post_tsn = chk->rec.data.TSN_seq + 1;
		if (post_tsn == next->rec.data.TSN_seq) {
			



			if (next->rec.data.rcv_flags & SCTP_DATA_FIRST_FRAG) {
				
				if ((chk->rec.data.rcv_flags & SCTP_DATA_FRAG_MASK)
				    != SCTP_DATA_LAST_FRAG) {
					SCTPDBG(SCTP_DEBUG_INDATA1, "Next chk - Next is FIRST, we must be LAST\n");
					SCTPDBG(SCTP_DEBUG_INDATA1, "Gak, Evil plot, its not a last!\n");
					oper = sctp_get_mbuf_for_msg((sizeof(struct sctp_paramhdr) + 3 * sizeof(uint32_t)),
								     0, M_NOWAIT, 1, MT_DATA);
					if (oper) {
						struct sctp_paramhdr *ph;
						uint32_t *ippp;

						SCTP_BUF_LEN(oper) =
						    sizeof(struct sctp_paramhdr) +
						    ( 3 * sizeof(uint32_t));
						ph = mtod(oper,
						    struct sctp_paramhdr *);
						ph->param_type =
						    htons(SCTP_CAUSE_PROTOCOL_VIOLATION);
						ph->param_length =
						    htons(SCTP_BUF_LEN(oper));
						ippp = (uint32_t *) (ph + 1);
						*ippp = htonl(SCTP_FROM_SCTP_INDATA+SCTP_LOC_10);
						ippp++;
						*ippp = chk->rec.data.TSN_seq;
						ippp++;
						*ippp = ((chk->rec.data.stream_number << 16) | chk->rec.data.stream_seq);
					}
					stcb->sctp_ep->last_abort_code = SCTP_FROM_SCTP_INDATA+SCTP_LOC_10;
					sctp_abort_an_association(stcb->sctp_ep, stcb, oper, SCTP_SO_NOT_LOCKED);
					*abort_flag = 1;
					return;
				}
			} else if ((next->rec.data.rcv_flags & SCTP_DATA_FRAG_MASK) ==
				    SCTP_DATA_MIDDLE_FRAG ||
				    (next->rec.data.rcv_flags & SCTP_DATA_FRAG_MASK) ==
			    SCTP_DATA_LAST_FRAG) {
				



				if ((chk->rec.data.rcv_flags & SCTP_DATA_FRAG_MASK) ==
				    SCTP_DATA_LAST_FRAG) {
					SCTPDBG(SCTP_DEBUG_INDATA1, "Next chk - Next is a MIDDLE/LAST\n");
					SCTPDBG(SCTP_DEBUG_INDATA1, "Gak, Evil plot, new prev chunk is a LAST\n");
					oper = sctp_get_mbuf_for_msg((sizeof(struct sctp_paramhdr) + 3 * sizeof(uint32_t)),
								     0, M_NOWAIT, 1, MT_DATA);
					if (oper) {
						struct sctp_paramhdr *ph;
						uint32_t *ippp;

						SCTP_BUF_LEN(oper) =
						    sizeof(struct sctp_paramhdr) +
						    (3 * sizeof(uint32_t));
						ph = mtod(oper,
						    struct sctp_paramhdr *);
						ph->param_type =
						    htons(SCTP_CAUSE_PROTOCOL_VIOLATION);
						ph->param_length =
						    htons(SCTP_BUF_LEN(oper));
						ippp = (uint32_t *) (ph + 1);
						*ippp = htonl(SCTP_FROM_SCTP_INDATA+SCTP_LOC_11);
						ippp++;
						*ippp = chk->rec.data.TSN_seq;
						ippp++;
						*ippp = ((chk->rec.data.stream_number << 16) | chk->rec.data.stream_seq);

					}
					stcb->sctp_ep->last_abort_code = SCTP_FROM_SCTP_INDATA+SCTP_LOC_11;
					sctp_abort_an_association(stcb->sctp_ep, stcb, oper, SCTP_SO_NOT_LOCKED);
					*abort_flag = 1;
					return;
				}
				if (chk->rec.data.stream_number !=
				    next->rec.data.stream_number) {
					



					SCTPDBG(SCTP_DEBUG_INDATA1, "Next chk - Gak, Evil plot, ssn:%d not the same as at:%d\n",
						chk->rec.data.stream_number,
						next->rec.data.stream_number);
					oper = sctp_get_mbuf_for_msg((sizeof(struct sctp_paramhdr) + 3 * sizeof(uint32_t)),
								     0, M_NOWAIT, 1, MT_DATA);
					if (oper) {
						struct sctp_paramhdr *ph;
						uint32_t *ippp;

						SCTP_BUF_LEN(oper) =
						    sizeof(struct sctp_paramhdr) +
						    (3 * sizeof(uint32_t));
						ph = mtod(oper,
						    struct sctp_paramhdr *);
						ph->param_type =
						    htons(SCTP_CAUSE_PROTOCOL_VIOLATION);
						ph->param_length =
						    htons(SCTP_BUF_LEN(oper));
						ippp = (uint32_t *) (ph + 1);
						*ippp = htonl(SCTP_FROM_SCTP_INDATA+SCTP_LOC_12);
						ippp++;
						*ippp = chk->rec.data.TSN_seq;
						ippp++;
						*ippp = ((chk->rec.data.stream_number << 16) | chk->rec.data.stream_seq);

					}
					stcb->sctp_ep->last_abort_code = SCTP_FROM_SCTP_INDATA+SCTP_LOC_12;
					sctp_abort_an_association(stcb->sctp_ep, stcb, oper, SCTP_SO_NOT_LOCKED);
					*abort_flag = 1;
					return;
				}
				if ((next->rec.data.rcv_flags & SCTP_DATA_UNORDERED) == 0 &&
				    chk->rec.data.stream_seq !=
				    next->rec.data.stream_seq) {
					



					SCTPDBG(SCTP_DEBUG_INDATA1, "Next chk - Gak, Evil plot, sseq:%d not the same as at:%d\n",
						chk->rec.data.stream_seq,
						next->rec.data.stream_seq);
					oper = sctp_get_mbuf_for_msg((sizeof(struct sctp_paramhdr) + 3 * sizeof(uint32_t)),
								     0, M_NOWAIT, 1, MT_DATA);
					if (oper) {
						struct sctp_paramhdr *ph;
						uint32_t *ippp;

						SCTP_BUF_LEN(oper) =
						    sizeof(struct sctp_paramhdr) +
						    (3 * sizeof(uint32_t));
						ph = mtod(oper,
						    struct sctp_paramhdr *);
						ph->param_type =
						    htons(SCTP_CAUSE_PROTOCOL_VIOLATION);
						ph->param_length =
						    htons(SCTP_BUF_LEN(oper));
						ippp = (uint32_t *) (ph + 1);
						*ippp = htonl(SCTP_FROM_SCTP_INDATA+SCTP_LOC_13);
						ippp++;
						*ippp = chk->rec.data.TSN_seq;
						ippp++;
						*ippp = ((chk->rec.data.stream_number << 16) | chk->rec.data.stream_seq);
					}
					stcb->sctp_ep->last_abort_code = SCTP_FROM_SCTP_INDATA+SCTP_LOC_13;
					sctp_abort_an_association(stcb->sctp_ep, stcb, oper, SCTP_SO_NOT_LOCKED);
					*abort_flag = 1;
					return;
				}
			}
		}
	}
	
	sctp_deliver_reasm_check(stcb, asoc);
}







static int
sctp_does_tsn_belong_to_reasm(struct sctp_association *asoc,
    uint32_t TSN_seq)
{
	struct sctp_tmit_chunk *at;
	uint32_t tsn_est;

	TAILQ_FOREACH(at, &asoc->reasmqueue, sctp_next) {
		if (SCTP_TSN_GT(TSN_seq, at->rec.data.TSN_seq)) {
			
			tsn_est = at->rec.data.TSN_seq + 1;
			if (tsn_est == TSN_seq) {
				
				if ((at->rec.data.rcv_flags & SCTP_DATA_FRAG_MASK) !=
				    SCTP_DATA_LAST_FRAG) {
					





					return (1);
				} else {
					




					return (0);
				}
			}
		} else if (TSN_seq == at->rec.data.TSN_seq) {
			
			return (1);
		} else {
			



			tsn_est = TSN_seq + 1;
			if (tsn_est == at->rec.data.TSN_seq) {
				
				if ((at->rec.data.rcv_flags & SCTP_DATA_FRAG_MASK) !=
				    SCTP_DATA_FIRST_FRAG) {
					return (1);
				} else {
					return (0);
				}
			}
		}
	}
	return (0);
}


static int
sctp_process_a_data_chunk(struct sctp_tcb *stcb, struct sctp_association *asoc,
    struct mbuf **m, int offset, struct sctp_data_chunk *ch, int chk_length,
    struct sctp_nets *net, uint32_t *high_tsn, int *abort_flag,
    int *break_flag, int last_chunk)
{
	
	
	struct sctp_tmit_chunk *chk;
	uint32_t tsn, gap;
	struct mbuf *dmbuf;
	int the_len;
	int need_reasm_check = 0;
	uint16_t strmno, strmseq;
	struct mbuf *oper;
	struct sctp_queued_to_read *control;
	int ordered;
	uint32_t protocol_id;
	uint8_t chunk_flags;
	struct sctp_stream_reset_list *liste;

	chk = NULL;
	tsn = ntohl(ch->dp.tsn);
	chunk_flags = ch->ch.chunk_flags;
	if ((chunk_flags & SCTP_DATA_SACK_IMMEDIATELY) == SCTP_DATA_SACK_IMMEDIATELY) {
		asoc->send_sack = 1;
	}
	protocol_id = ch->dp.protocol_id;
	ordered = ((chunk_flags & SCTP_DATA_UNORDERED) == 0);
	if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_MAP_LOGGING_ENABLE) {
		sctp_log_map(tsn, asoc->cumulative_tsn, asoc->highest_tsn_inside_map, SCTP_MAP_TSN_ENTERS);
	}
	if (stcb == NULL) {
		return (0);
	}
	SCTP_LTRACE_CHK(stcb->sctp_ep, stcb, ch->ch.chunk_type, tsn);
	if (SCTP_TSN_GE(asoc->cumulative_tsn, tsn)) {
		
		SCTP_STAT_INCR(sctps_recvdupdata);
		if (asoc->numduptsns < SCTP_MAX_DUP_TSNS) {
			
			asoc->dup_tsns[asoc->numduptsns] = tsn;
			asoc->numduptsns++;
		}
		asoc->send_sack = 1;
		return (0);
	}
	
	SCTP_CALC_TSN_TO_GAP(gap, tsn, asoc->mapping_array_base_tsn);
	if (gap >= (SCTP_MAPPING_ARRAY << 3)) {
		
		return (0);
	}
	if (gap >= (uint32_t) (asoc->mapping_array_size << 3)) {
		SCTP_TCB_LOCK_ASSERT(stcb);
		if (sctp_expand_mapping_array(asoc, gap)) {
			
			return (0);
		}
	}
	if (SCTP_TSN_GT(tsn, *high_tsn)) {
		*high_tsn = tsn;
	}
	
	if (SCTP_IS_TSN_PRESENT(asoc->mapping_array, gap) ||
	    SCTP_IS_TSN_PRESENT(asoc->nr_mapping_array, gap)) {
		SCTP_STAT_INCR(sctps_recvdupdata);
		if (asoc->numduptsns < SCTP_MAX_DUP_TSNS) {
			
			asoc->dup_tsns[asoc->numduptsns] = tsn;
			asoc->numduptsns++;
		}
		asoc->send_sack = 1;
		return (0);
	}
	



	if (((stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_SOCKET_GONE) ||
	     (stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_SOCKET_ALLGONE) ||
	     (stcb->asoc.state & SCTP_STATE_CLOSED_SOCKET))
		) {
		



		struct mbuf *op_err;

		op_err = sctp_generate_invmanparam(SCTP_CAUSE_OUT_OF_RESC);
		sctp_abort_an_association(stcb->sctp_ep, stcb, op_err, SCTP_SO_NOT_LOCKED);
		*abort_flag = 1;
		return (0);
	}
	





	
	if (((asoc->cnt_on_all_streams +
	      asoc->cnt_on_reasm_queue +
	      asoc->cnt_msg_on_sb) >= SCTP_BASE_SYSCTL(sctp_max_chunks_on_queue)) ||
	    (((int)asoc->my_rwnd) <= 0)) {
		



		if (stcb->sctp_socket->so_rcv.sb_cc) {
			
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
			struct socket *so;

			so = SCTP_INP_SO(stcb->sctp_ep);
			atomic_add_int(&stcb->asoc.refcnt, 1);
			SCTP_TCB_UNLOCK(stcb);
			SCTP_SOCKET_LOCK(so, 1);
			SCTP_TCB_LOCK(stcb);
			atomic_subtract_int(&stcb->asoc.refcnt, 1);
			if (stcb->asoc.state & SCTP_STATE_CLOSED_SOCKET) {
				
				SCTP_SOCKET_UNLOCK(so, 1);
				return (0);
			}
#endif
			sctp_sorwakeup(stcb->sctp_ep, stcb->sctp_socket);
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
			SCTP_SOCKET_UNLOCK(so, 1);
#endif
		}
		
		if (SCTP_TSN_GT(tsn, asoc->highest_tsn_inside_map) &&
		    SCTP_TSN_GT(tsn, asoc->highest_tsn_inside_nr_map)) {
			
			sctp_set_rwnd(stcb, asoc);
			if ((asoc->cnt_on_all_streams +
			     asoc->cnt_on_reasm_queue +
			     asoc->cnt_msg_on_sb) >= SCTP_BASE_SYSCTL(sctp_max_chunks_on_queue)) {
				SCTP_STAT_INCR(sctps_datadropchklmt);
			} else {
				SCTP_STAT_INCR(sctps_datadroprwnd);
			}
			*break_flag = 1;
			return (0);
		}
	}
	strmno = ntohs(ch->dp.stream_id);
	if (strmno >= asoc->streamincnt) {
		struct sctp_paramhdr *phdr;
		struct mbuf *mb;

		mb = sctp_get_mbuf_for_msg((sizeof(struct sctp_paramhdr) * 2),
					   0, M_NOWAIT, 1, MT_DATA);
		if (mb != NULL) {
			
			SCTP_BUF_RESV_UF(mb, sizeof(struct sctp_chunkhdr));
			phdr = mtod(mb, struct sctp_paramhdr *);
			




			SCTP_BUF_LEN(mb) = (sizeof(struct sctp_paramhdr) * 2);
			phdr->param_type = htons(SCTP_CAUSE_INVALID_STREAM);
			phdr->param_length =
			    htons(sizeof(struct sctp_paramhdr) * 2);
			phdr++;
			
			phdr->param_type = ch->dp.stream_id;
			
			phdr->param_length = 0;
			sctp_queue_op_err(stcb, mb);
		}
		SCTP_STAT_INCR(sctps_badsid);
		SCTP_TCB_LOCK_ASSERT(stcb);
		SCTP_SET_TSN_PRESENT(asoc->nr_mapping_array, gap);
		if (SCTP_TSN_GT(tsn, asoc->highest_tsn_inside_nr_map)) {
			asoc->highest_tsn_inside_nr_map = tsn;
		}
		if (tsn == (asoc->cumulative_tsn + 1)) {
			
			asoc->cumulative_tsn = tsn;
		}
		return (0);
	}
	






	strmseq = ntohs(ch->dp.stream_sequence);
#ifdef SCTP_ASOCLOG_OF_TSNS
	SCTP_TCB_LOCK_ASSERT(stcb);
	if (asoc->tsn_in_at >= SCTP_TSN_LOG_SIZE) {
		asoc->tsn_in_at = 0;
		asoc->tsn_in_wrapped = 1;
	}
	asoc->in_tsnlog[asoc->tsn_in_at].tsn = tsn;
	asoc->in_tsnlog[asoc->tsn_in_at].strm = strmno;
	asoc->in_tsnlog[asoc->tsn_in_at].seq = strmseq;
	asoc->in_tsnlog[asoc->tsn_in_at].sz = chk_length;
	asoc->in_tsnlog[asoc->tsn_in_at].flgs = chunk_flags;
	asoc->in_tsnlog[asoc->tsn_in_at].stcb = (void *)stcb;
	asoc->in_tsnlog[asoc->tsn_in_at].in_pos = asoc->tsn_in_at;
	asoc->in_tsnlog[asoc->tsn_in_at].in_out = 1;
	asoc->tsn_in_at++;
#endif
	if ((chunk_flags & SCTP_DATA_FIRST_FRAG) &&
	    (TAILQ_EMPTY(&asoc->resetHead)) &&
	    (chunk_flags & SCTP_DATA_UNORDERED) == 0 &&
	    SCTP_SSN_GE(asoc->strmin[strmno].last_sequence_delivered, strmseq)) {
		
		SCTPDBG(SCTP_DEBUG_INDATA1, "EVIL/Broken-Dup S-SEQ:%d delivered:%d from peer, Abort!\n",
			strmseq, asoc->strmin[strmno].last_sequence_delivered);
		oper = sctp_get_mbuf_for_msg((sizeof(struct sctp_paramhdr) + 3 * sizeof(uint32_t)),
					     0, M_NOWAIT, 1, MT_DATA);
		if (oper) {
			struct sctp_paramhdr *ph;
			uint32_t *ippp;

			SCTP_BUF_LEN(oper) = sizeof(struct sctp_paramhdr) +
			    (3 * sizeof(uint32_t));
			ph = mtod(oper, struct sctp_paramhdr *);
			ph->param_type = htons(SCTP_CAUSE_PROTOCOL_VIOLATION);
			ph->param_length = htons(SCTP_BUF_LEN(oper));
			ippp = (uint32_t *) (ph + 1);
			*ippp = htonl(SCTP_FROM_SCTP_INDATA+SCTP_LOC_14);
			ippp++;
			*ippp = tsn;
			ippp++;
			*ippp = ((strmno << 16) | strmseq);

		}
		stcb->sctp_ep->last_abort_code = SCTP_FROM_SCTP_INDATA+SCTP_LOC_14;
		sctp_abort_an_association(stcb->sctp_ep, stcb, oper, SCTP_SO_NOT_LOCKED);
		*abort_flag = 1;
		return (0);
	}
	




	the_len = (chk_length - sizeof(struct sctp_data_chunk));
	if (last_chunk == 0) {
		dmbuf = SCTP_M_COPYM(*m,
				     (offset + sizeof(struct sctp_data_chunk)),
				     the_len, M_NOWAIT);
#ifdef SCTP_MBUF_LOGGING
		if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_MBUF_LOGGING_ENABLE) {
			struct mbuf *mat;

			for (mat = dmbuf; mat; mat = SCTP_BUF_NEXT(mat)) {
				if (SCTP_BUF_IS_EXTENDED(mat)) {
					sctp_log_mb(mat, SCTP_MBUF_ICOPY);
				}
			}
		}
#endif
	} else {
		
		int l_len;
		dmbuf = *m;
		
		m_adj(dmbuf, (offset + sizeof(struct sctp_data_chunk)));
		if (SCTP_BUF_NEXT(dmbuf) == NULL) {
			l_len = SCTP_BUF_LEN(dmbuf);
		} else {
			


			struct mbuf *lat;

			l_len = 0;
			for (lat = dmbuf; lat; lat = SCTP_BUF_NEXT(lat)) {
				l_len += SCTP_BUF_LEN(lat);
			}
		}
		if (l_len > the_len) {
			
			m_adj(dmbuf, -(l_len - the_len));
		}
	}
	if (dmbuf == NULL) {
		SCTP_STAT_INCR(sctps_nomem);
		return (0);
	}
	if ((chunk_flags & SCTP_DATA_NOT_FRAG) == SCTP_DATA_NOT_FRAG &&
	    asoc->fragmented_delivery_inprogress == 0 &&
	    TAILQ_EMPTY(&asoc->resetHead) &&
	    ((ordered == 0) ||
	    ((uint16_t)(asoc->strmin[strmno].last_sequence_delivered + 1) == strmseq &&
	    TAILQ_EMPTY(&asoc->strmin[strmno].inqueue)))) {
		
		







		
		sctp_alloc_a_readq(stcb, control);
		sctp_build_readq_entry_mac(control, stcb, asoc->context, net, tsn,
					   protocol_id,
					   stcb->asoc.context,
					   strmno, strmseq,
					   chunk_flags,
					   dmbuf);
		if (control == NULL) {
			goto failed_express_del;
		}
		SCTP_SET_TSN_PRESENT(asoc->nr_mapping_array, gap);
		if (SCTP_TSN_GT(tsn, asoc->highest_tsn_inside_nr_map)) {
			asoc->highest_tsn_inside_nr_map = tsn;
		}
		sctp_add_to_readq(stcb->sctp_ep, stcb,
		                  control, &stcb->sctp_socket->so_rcv,
		                  1, SCTP_READ_LOCK_NOT_HELD, SCTP_SO_NOT_LOCKED);

		if ((chunk_flags & SCTP_DATA_UNORDERED) == 0) {
			
			asoc->strmin[strmno].last_sequence_delivered++;
		}
		SCTP_STAT_INCR(sctps_recvexpress);
		if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_STR_LOGGING_ENABLE) {
			sctp_log_strm_del_alt(stcb, tsn, strmseq, strmno,
					      SCTP_STR_LOG_FROM_EXPRS_DEL);
		}
		control = NULL;

		goto finish_express_del;
	}
failed_express_del:
	
	chk = NULL;
	control = NULL;
	
	if ((asoc->fragmented_delivery_inprogress) &&
	    (stcb->asoc.control_pdapi) &&
	    (asoc->str_of_pdapi == strmno) &&
	    (asoc->ssn_of_pdapi == strmseq)
		) {
		control = stcb->asoc.control_pdapi;
		if ((chunk_flags & SCTP_DATA_FIRST_FRAG) == SCTP_DATA_FIRST_FRAG) {
			
			goto failed_pdapi_express_del;
		}
		if (tsn == (control->sinfo_tsn + 1)) {
			
			int end = 0;

			if (chunk_flags & SCTP_DATA_LAST_FRAG) {
				end = 1;
			}
			if (sctp_append_to_readq(stcb->sctp_ep, stcb, control, dmbuf, end,
			                         tsn,
			                         &stcb->sctp_socket->so_rcv)) {
				SCTP_PRINTF("Append fails end:%d\n", end);
				goto failed_pdapi_express_del;
			}

			SCTP_SET_TSN_PRESENT(asoc->nr_mapping_array, gap);
			if (SCTP_TSN_GT(tsn, asoc->highest_tsn_inside_nr_map)) {
				asoc->highest_tsn_inside_nr_map = tsn;
			}
			SCTP_STAT_INCR(sctps_recvexpressm);
			control->sinfo_tsn = tsn;
			asoc->tsn_last_delivered = tsn;
			asoc->fragment_flags = chunk_flags;
			asoc->tsn_of_pdapi_last_delivered = tsn;
			asoc->last_flags_delivered = chunk_flags;
			asoc->last_strm_seq_delivered = strmseq;
			asoc->last_strm_no_delivered = strmno;
			if (end) {
				
				asoc->fragmented_delivery_inprogress = 0;
				if ((chunk_flags & SCTP_DATA_UNORDERED) == 0) {
					asoc->strmin[strmno].last_sequence_delivered++;
				}
				stcb->asoc.control_pdapi = NULL;
				if (TAILQ_EMPTY(&asoc->reasmqueue) == 0) {
					
					need_reasm_check = 1;
				}
			}
			control = NULL;
			goto finish_express_del;
		}
	}
 failed_pdapi_express_del:
	control = NULL;
	if (SCTP_BASE_SYSCTL(sctp_do_drain) == 0) {
		SCTP_SET_TSN_PRESENT(asoc->nr_mapping_array, gap);
		if (SCTP_TSN_GT(tsn, asoc->highest_tsn_inside_nr_map)) {
			asoc->highest_tsn_inside_nr_map = tsn;
		}
	} else {
		SCTP_SET_TSN_PRESENT(asoc->mapping_array, gap);
		if (SCTP_TSN_GT(tsn, asoc->highest_tsn_inside_map)) {
			asoc->highest_tsn_inside_map = tsn;
		}
	}
	if ((chunk_flags & SCTP_DATA_NOT_FRAG) != SCTP_DATA_NOT_FRAG) {
		sctp_alloc_a_chunk(stcb, chk);
		if (chk == NULL) {
			
			SCTP_STAT_INCR(sctps_nomem);
			if (last_chunk == 0) {
				
				sctp_m_freem(dmbuf);
			}
			return (0);
		}
		chk->rec.data.TSN_seq = tsn;
		chk->no_fr_allowed = 0;
		chk->rec.data.stream_seq = strmseq;
		chk->rec.data.stream_number = strmno;
		chk->rec.data.payloadtype = protocol_id;
		chk->rec.data.context = stcb->asoc.context;
		chk->rec.data.doing_fast_retransmit = 0;
		chk->rec.data.rcv_flags = chunk_flags;
		chk->asoc = asoc;
		chk->send_size = the_len;
		chk->whoTo = net;
		atomic_add_int(&net->ref_count, 1);
		chk->data = dmbuf;
	} else {
		sctp_alloc_a_readq(stcb, control);
		sctp_build_readq_entry_mac(control, stcb, asoc->context, net, tsn,
		    protocol_id,
		    stcb->asoc.context,
		    strmno, strmseq,
		    chunk_flags,
		    dmbuf);
		if (control == NULL) {
			
			SCTP_STAT_INCR(sctps_nomem);
			if (last_chunk == 0) {
				
				sctp_m_freem(dmbuf);
			}
			return (0);
		}
		control->length = the_len;
	}

	
	
	if (control != NULL) {
		
		if (asoc->fragmented_delivery_inprogress) {
			





			uint32_t estimate_tsn;

			estimate_tsn = asoc->tsn_last_delivered + 1;
			if (TAILQ_EMPTY(&asoc->reasmqueue) &&
			    (estimate_tsn == control->sinfo_tsn)) {
				
				sctp_m_freem(control->data);
				control->data = NULL;
				if (control->whoFrom) {
					sctp_free_remote_addr(control->whoFrom);
					control->whoFrom = NULL;
				}
				sctp_free_a_readq(stcb, control);
				oper = sctp_get_mbuf_for_msg((sizeof(struct sctp_paramhdr) + 3 * sizeof(uint32_t)),
							     0, M_NOWAIT, 1, MT_DATA);
				if (oper) {
					struct sctp_paramhdr *ph;
					uint32_t *ippp;

					SCTP_BUF_LEN(oper) =
					    sizeof(struct sctp_paramhdr) +
					    (3 * sizeof(uint32_t));
					ph = mtod(oper, struct sctp_paramhdr *);
					ph->param_type =
					    htons(SCTP_CAUSE_PROTOCOL_VIOLATION);
					ph->param_length = htons(SCTP_BUF_LEN(oper));
					ippp = (uint32_t *) (ph + 1);
					*ippp = htonl(SCTP_FROM_SCTP_INDATA+SCTP_LOC_15);
					ippp++;
					*ippp = tsn;
					ippp++;
					*ippp = ((strmno << 16) | strmseq);
				}
				stcb->sctp_ep->last_abort_code = SCTP_FROM_SCTP_INDATA+SCTP_LOC_15;
				sctp_abort_an_association(stcb->sctp_ep, stcb, oper, SCTP_SO_NOT_LOCKED);
				*abort_flag = 1;
				return (0);
			} else {
				if (sctp_does_tsn_belong_to_reasm(asoc, control->sinfo_tsn)) {
					sctp_m_freem(control->data);
					control->data = NULL;
					if (control->whoFrom) {
						sctp_free_remote_addr(control->whoFrom);
						control->whoFrom = NULL;
					}
					sctp_free_a_readq(stcb, control);

					oper = sctp_get_mbuf_for_msg((sizeof(struct sctp_paramhdr) + 3 * sizeof(uint32_t)),
								     0, M_NOWAIT, 1, MT_DATA);
					if (oper) {
						struct sctp_paramhdr *ph;
						uint32_t *ippp;

						SCTP_BUF_LEN(oper) =
						    sizeof(struct sctp_paramhdr) +
						    ( 3 * sizeof(uint32_t));
						ph = mtod(oper,
						    struct sctp_paramhdr *);
						ph->param_type =
						    htons(SCTP_CAUSE_PROTOCOL_VIOLATION);
						ph->param_length =
						    htons(SCTP_BUF_LEN(oper));
						ippp = (uint32_t *) (ph + 1);
						*ippp = htonl(SCTP_FROM_SCTP_INDATA+SCTP_LOC_16);
						ippp++;
						*ippp = tsn;
						ippp++;
						*ippp = ((strmno << 16) | strmseq);
					}
					stcb->sctp_ep->last_abort_code = SCTP_FROM_SCTP_INDATA+SCTP_LOC_16;
					sctp_abort_an_association(stcb->sctp_ep, stcb, oper, SCTP_SO_NOT_LOCKED);
					*abort_flag = 1;
					return (0);
				}
			}
		} else {
			
			if (!TAILQ_EMPTY(&asoc->reasmqueue)) {
				





				if (sctp_does_tsn_belong_to_reasm(asoc, control->sinfo_tsn)) {
					sctp_m_freem(control->data);
					control->data = NULL;
					if (control->whoFrom) {
						sctp_free_remote_addr(control->whoFrom);
						control->whoFrom = NULL;
					}
					sctp_free_a_readq(stcb, control);
					oper = sctp_get_mbuf_for_msg((sizeof(struct sctp_paramhdr) + 3 * sizeof(uint32_t)),
								     0, M_NOWAIT, 1, MT_DATA);
					if (oper) {
						struct sctp_paramhdr *ph;
						uint32_t *ippp;

						SCTP_BUF_LEN(oper) =
						    sizeof(struct sctp_paramhdr) +
						    (3 * sizeof(uint32_t));
						ph = mtod(oper,
						    struct sctp_paramhdr *);
						ph->param_type =
						    htons(SCTP_CAUSE_PROTOCOL_VIOLATION);
						ph->param_length =
						    htons(SCTP_BUF_LEN(oper));
						ippp = (uint32_t *) (ph + 1);
						*ippp = htonl(SCTP_FROM_SCTP_INDATA+SCTP_LOC_17);
						ippp++;
						*ippp = tsn;
						ippp++;
						*ippp = ((strmno << 16) | strmseq);
					}
					stcb->sctp_ep->last_abort_code = SCTP_FROM_SCTP_INDATA+SCTP_LOC_17;
					sctp_abort_an_association(stcb->sctp_ep, stcb, oper, SCTP_SO_NOT_LOCKED);
					*abort_flag = 1;
					return (0);
				}
			}
		}
		
		if (chunk_flags & SCTP_DATA_UNORDERED) {
			
			sctp_mark_non_revokable(asoc, control->sinfo_tsn);
			sctp_add_to_readq(stcb->sctp_ep, stcb,
			                  control,
			                  &stcb->sctp_socket->so_rcv, 1, SCTP_READ_LOCK_NOT_HELD, SCTP_SO_NOT_LOCKED);
		} else {
			














			if (((liste = TAILQ_FIRST(&asoc->resetHead)) != NULL) &&
			    SCTP_TSN_GT(tsn, liste->tsn)) {
				



				if (TAILQ_EMPTY(&asoc->pending_reply_queue)) {
					
					TAILQ_INSERT_TAIL(&asoc->pending_reply_queue, control, next);
				} else {
					struct sctp_queued_to_read *ctlOn, *nctlOn;
					unsigned char inserted = 0;

					TAILQ_FOREACH_SAFE(ctlOn, &asoc->pending_reply_queue, next, nctlOn) {
						if (SCTP_TSN_GT(control->sinfo_tsn, ctlOn->sinfo_tsn)) {
							continue;
						} else {
							
							TAILQ_INSERT_BEFORE(ctlOn, control, next);
							inserted = 1;
							break;
						}
					}
					if (inserted == 0) {
						




						TAILQ_INSERT_TAIL(&asoc->pending_reply_queue, control, next);
					}
				}
			} else {
				sctp_queue_data_to_stream(stcb, asoc, control, abort_flag);
				if (*abort_flag) {
					return (0);
				}
			}
		}
	} else {
		
		sctp_queue_data_for_reasm(stcb, asoc, chk, abort_flag);
		if (*abort_flag) {
			



			*m = NULL;
			return (0);
		}
	}
finish_express_del:
	if (tsn == (asoc->cumulative_tsn + 1)) {
		
		asoc->cumulative_tsn = tsn;
	}
	if (last_chunk) {
		*m = NULL;
	}
	if (ordered) {
		SCTP_STAT_INCR_COUNTER64(sctps_inorderchunks);
	} else {
		SCTP_STAT_INCR_COUNTER64(sctps_inunorderchunks);
	}
	SCTP_STAT_INCR(sctps_recvdata);
	
	if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_STR_LOGGING_ENABLE) {
		sctp_log_strm_del_alt(stcb, tsn, strmseq, strmno, SCTP_STR_LOG_FROM_MARK_TSN);
	}
	if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_MAP_LOGGING_ENABLE) {
		sctp_log_map(asoc->mapping_array_base_tsn, asoc->cumulative_tsn,
			     asoc->highest_tsn_inside_map, SCTP_MAP_PREPARE_SLIDE);
	}
	
	if (((liste = TAILQ_FIRST(&asoc->resetHead)) != NULL) &&
	    SCTP_TSN_GE(asoc->cumulative_tsn, liste->tsn)) {
		





		struct sctp_queued_to_read *ctl, *nctl;

		sctp_reset_in_stream(stcb, liste->number_entries, liste->list_of_streams);
		TAILQ_REMOVE(&asoc->resetHead, liste, next_resp);
		SCTP_FREE(liste, SCTP_M_STRESET);
		
		liste = TAILQ_FIRST(&asoc->resetHead);
		if (TAILQ_EMPTY(&asoc->resetHead)) {
			
			TAILQ_FOREACH_SAFE(ctl, &asoc->pending_reply_queue, next, nctl) {
				TAILQ_REMOVE(&asoc->pending_reply_queue, ctl, next);
				sctp_queue_data_to_stream(stcb, asoc, ctl, abort_flag);
				if (*abort_flag) {
					return (0);
				}
			}
		} else {
			TAILQ_FOREACH_SAFE(ctl, &asoc->pending_reply_queue, next, nctl) {
				if (SCTP_TSN_GT(ctl->sinfo_tsn, liste->tsn)) {
					break;
				}
				




				TAILQ_REMOVE(&asoc->pending_reply_queue, ctl, next);
				sctp_queue_data_to_stream(stcb, asoc, ctl, abort_flag);
				if (*abort_flag) {
					return (0);
				}
			}
		}
		



		sctp_deliver_reasm_check(stcb, asoc);
		need_reasm_check = 0;
	}

	if (need_reasm_check) {
		
		sctp_deliver_reasm_check(stcb, asoc);
	}
	return (1);
}

int8_t sctp_map_lookup_tab[256] = {
  0, 1, 0, 2, 0, 1, 0, 3,
  0, 1, 0, 2, 0, 1, 0, 4,
  0, 1, 0, 2, 0, 1, 0, 3,
  0, 1, 0, 2, 0, 1, 0, 5,
  0, 1, 0, 2, 0, 1, 0, 3,
  0, 1, 0, 2, 0, 1, 0, 4,
  0, 1, 0, 2, 0, 1, 0, 3,
  0, 1, 0, 2, 0, 1, 0, 6,
  0, 1, 0, 2, 0, 1, 0, 3,
  0, 1, 0, 2, 0, 1, 0, 4,
  0, 1, 0, 2, 0, 1, 0, 3,
  0, 1, 0, 2, 0, 1, 0, 5,
  0, 1, 0, 2, 0, 1, 0, 3,
  0, 1, 0, 2, 0, 1, 0, 4,
  0, 1, 0, 2, 0, 1, 0, 3,
  0, 1, 0, 2, 0, 1, 0, 7,
  0, 1, 0, 2, 0, 1, 0, 3,
  0, 1, 0, 2, 0, 1, 0, 4,
  0, 1, 0, 2, 0, 1, 0, 3,
  0, 1, 0, 2, 0, 1, 0, 5,
  0, 1, 0, 2, 0, 1, 0, 3,
  0, 1, 0, 2, 0, 1, 0, 4,
  0, 1, 0, 2, 0, 1, 0, 3,
  0, 1, 0, 2, 0, 1, 0, 6,
  0, 1, 0, 2, 0, 1, 0, 3,
  0, 1, 0, 2, 0, 1, 0, 4,
  0, 1, 0, 2, 0, 1, 0, 3,
  0, 1, 0, 2, 0, 1, 0, 5,
  0, 1, 0, 2, 0, 1, 0, 3,
  0, 1, 0, 2, 0, 1, 0, 4,
  0, 1, 0, 2, 0, 1, 0, 3,
  0, 1, 0, 2, 0, 1, 0, 8
};


void
sctp_slide_mapping_arrays(struct sctp_tcb *stcb)
{
	













	struct sctp_association *asoc;
	int at;
	uint8_t val;
	int slide_from, slide_end, lgap, distance;
	uint32_t old_cumack, old_base, old_highest, highest_tsn;

	asoc = &stcb->asoc;

	old_cumack = asoc->cumulative_tsn;
	old_base = asoc->mapping_array_base_tsn;
	old_highest = asoc->highest_tsn_inside_map;
	



	at = 0;
	for (slide_from = 0; slide_from < stcb->asoc.mapping_array_size; slide_from++) {
		val = asoc->nr_mapping_array[slide_from] | asoc->mapping_array[slide_from];
		if (val == 0xff) {
			at += 8;
		} else {
			
			at += sctp_map_lookup_tab[val];
			break;
		}
	}
	asoc->cumulative_tsn = asoc->mapping_array_base_tsn + (at-1);

	if (SCTP_TSN_GT(asoc->cumulative_tsn, asoc->highest_tsn_inside_map) &&
            SCTP_TSN_GT(asoc->cumulative_tsn, asoc->highest_tsn_inside_nr_map)) {
#ifdef INVARIANTS
		panic("huh, cumack 0x%x greater than high-tsn 0x%x in map",
		      asoc->cumulative_tsn, asoc->highest_tsn_inside_map);
#else
		SCTP_PRINTF("huh, cumack 0x%x greater than high-tsn 0x%x in map - should panic?\n",
			    asoc->cumulative_tsn, asoc->highest_tsn_inside_map);
		sctp_print_mapping_array(asoc);
		if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_MAP_LOGGING_ENABLE) {
			sctp_log_map(0, 6, asoc->highest_tsn_inside_map, SCTP_MAP_SLIDE_RESULT);
		}
		asoc->highest_tsn_inside_map = asoc->cumulative_tsn;
		asoc->highest_tsn_inside_nr_map = asoc->cumulative_tsn;
#endif
	}
	if (SCTP_TSN_GT(asoc->highest_tsn_inside_nr_map, asoc->highest_tsn_inside_map)) {
		highest_tsn = asoc->highest_tsn_inside_nr_map;
	} else {
		highest_tsn = asoc->highest_tsn_inside_map;
	}
	if ((asoc->cumulative_tsn == highest_tsn) && (at >= 8)) {
		
		
		int clr;
#ifdef INVARIANTS
		unsigned int i;
#endif

		
		clr = ((at+7) >> 3);
		if (clr > asoc->mapping_array_size) {
			clr = asoc->mapping_array_size;
		}
		memset(asoc->mapping_array, 0, clr);
		memset(asoc->nr_mapping_array, 0, clr);
#ifdef INVARIANTS
		for (i = 0; i < asoc->mapping_array_size; i++) {
			if ((asoc->mapping_array[i]) || (asoc->nr_mapping_array[i])) {
				SCTP_PRINTF("Error Mapping array's not clean at clear\n");
				sctp_print_mapping_array(asoc);
			}
		}
#endif
		asoc->mapping_array_base_tsn = asoc->cumulative_tsn + 1;
		asoc->highest_tsn_inside_nr_map = asoc->highest_tsn_inside_map = asoc->cumulative_tsn;
	} else if (at >= 8) {
		
		

		



		SCTP_CALC_TSN_TO_GAP(lgap, highest_tsn, asoc->mapping_array_base_tsn);
		slide_end = (lgap >> 3);
		if (slide_end < slide_from) {
			sctp_print_mapping_array(asoc);
#ifdef INVARIANTS
			panic("impossible slide");
#else
			SCTP_PRINTF("impossible slide lgap:%x slide_end:%x slide_from:%x? at:%d\n",
			            lgap, slide_end, slide_from, at);
			return;
#endif
		}
		if (slide_end > asoc->mapping_array_size) {
#ifdef INVARIANTS
			panic("would overrun buffer");
#else
			SCTP_PRINTF("Gak, would have overrun map end:%d slide_end:%d\n",
			            asoc->mapping_array_size, slide_end);
			slide_end = asoc->mapping_array_size;
#endif
		}
		distance = (slide_end - slide_from) + 1;
		if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_MAP_LOGGING_ENABLE) {
			sctp_log_map(old_base, old_cumack, old_highest,
				     SCTP_MAP_PREPARE_SLIDE);
			sctp_log_map((uint32_t) slide_from, (uint32_t) slide_end,
				     (uint32_t) lgap, SCTP_MAP_SLIDE_FROM);
		}
		if (distance + slide_from > asoc->mapping_array_size ||
		    distance < 0) {
			






			if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_MAP_LOGGING_ENABLE) {
				sctp_log_map((uint32_t) distance, (uint32_t) slide_from,
					     (uint32_t) asoc->mapping_array_size,
					     SCTP_MAP_SLIDE_NONE);
			}
		} else {
			int ii;

			for (ii = 0; ii < distance; ii++) {
				asoc->mapping_array[ii] = asoc->mapping_array[slide_from + ii];
				asoc->nr_mapping_array[ii] = asoc->nr_mapping_array[slide_from + ii];

			}
			for (ii = distance; ii < asoc->mapping_array_size; ii++) {
				asoc->mapping_array[ii] = 0;
				asoc->nr_mapping_array[ii] = 0;
			}
			if (asoc->highest_tsn_inside_map + 1 == asoc->mapping_array_base_tsn) {
				asoc->highest_tsn_inside_map += (slide_from << 3);
			}
			if (asoc->highest_tsn_inside_nr_map + 1 == asoc->mapping_array_base_tsn) {
				asoc->highest_tsn_inside_nr_map += (slide_from << 3);
			}
			asoc->mapping_array_base_tsn += (slide_from << 3);
			if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_MAP_LOGGING_ENABLE) {
				sctp_log_map(asoc->mapping_array_base_tsn,
					     asoc->cumulative_tsn, asoc->highest_tsn_inside_map,
					     SCTP_MAP_SLIDE_RESULT);
			}
		}
	}
}

void
sctp_sack_check(struct sctp_tcb *stcb, int was_a_gap)
{
	struct sctp_association *asoc;
	uint32_t highest_tsn;

	asoc = &stcb->asoc;
	if (SCTP_TSN_GT(asoc->highest_tsn_inside_nr_map, asoc->highest_tsn_inside_map)) {
		highest_tsn = asoc->highest_tsn_inside_nr_map;
	} else {
		highest_tsn = asoc->highest_tsn_inside_map;
	}

	



	if (SCTP_GET_STATE(asoc) == SCTP_STATE_SHUTDOWN_SENT) {
		




		if (SCTP_OS_TIMER_PENDING(&stcb->asoc.dack_timer.timer)) {
			sctp_timer_stop(SCTP_TIMER_TYPE_RECV,
			                stcb->sctp_ep, stcb, NULL, SCTP_FROM_SCTP_INDATA+SCTP_LOC_18);
		}
		sctp_send_shutdown(stcb,
				   ((stcb->asoc.alternate) ? stcb->asoc.alternate : stcb->asoc.primary_destination));
		sctp_send_sack(stcb, SCTP_SO_NOT_LOCKED);
	} else {
		int is_a_gap;

		
		is_a_gap = SCTP_TSN_GT(highest_tsn, stcb->asoc.cumulative_tsn);

		



		stcb->asoc.cmt_dac_pkts_rcvd++;

		if ((stcb->asoc.send_sack == 1) ||      
		    ((was_a_gap) && (is_a_gap == 0)) ||	

		    (stcb->asoc.numduptsns) ||          
		    (is_a_gap) ||                       
		    (stcb->asoc.delayed_ack == 0) ||    
		    (stcb->asoc.data_pkts_seen >= stcb->asoc.sack_freq)	
			) {

			if ((stcb->asoc.sctp_cmt_on_off > 0) &&
			    (SCTP_BASE_SYSCTL(sctp_cmt_use_dac)) &&
			    (stcb->asoc.send_sack == 0) &&
			    (stcb->asoc.numduptsns == 0) &&
			    (stcb->asoc.delayed_ack) &&
			    (!SCTP_OS_TIMER_PENDING(&stcb->asoc.dack_timer.timer))) {

				











				sctp_timer_start(SCTP_TIMER_TYPE_RECV,
				                 stcb->sctp_ep, stcb, NULL);
			} else {
				





				(void)SCTP_OS_TIMER_STOP(&stcb->asoc.dack_timer.timer);
				sctp_send_sack(stcb, SCTP_SO_NOT_LOCKED);
			}
		} else {
			if (!SCTP_OS_TIMER_PENDING(&stcb->asoc.dack_timer.timer)) {
				sctp_timer_start(SCTP_TIMER_TYPE_RECV,
				                 stcb->sctp_ep, stcb, NULL);
			}
		}
	}
}

void
sctp_service_queues(struct sctp_tcb *stcb, struct sctp_association *asoc)
{
	struct sctp_tmit_chunk *chk;
	uint32_t tsize, pd_point;
	uint16_t nxt_todel;

	if (asoc->fragmented_delivery_inprogress) {
		sctp_service_reassembly(stcb, asoc);
	}
	
	if (asoc->fragmented_delivery_inprogress) {
		
		return;
	}
	



 doit_again:
	chk = TAILQ_FIRST(&asoc->reasmqueue);
	if (chk == NULL) {
		asoc->size_on_reasm_queue = 0;
		asoc->cnt_on_reasm_queue = 0;
		return;
	}
	nxt_todel = asoc->strmin[chk->rec.data.stream_number].last_sequence_delivered + 1;
	if ((chk->rec.data.rcv_flags & SCTP_DATA_FIRST_FRAG) &&
	    ((nxt_todel == chk->rec.data.stream_seq) ||
	    (chk->rec.data.rcv_flags & SCTP_DATA_UNORDERED))) {
		




		




		if (stcb->sctp_socket) {
			pd_point = min(SCTP_SB_LIMIT_RCV(stcb->sctp_socket),
				       stcb->sctp_ep->partial_delivery_point);
		} else {
			pd_point = stcb->sctp_ep->partial_delivery_point;
		}
		if (sctp_is_all_msg_on_reasm(asoc, &tsize) || (tsize >= pd_point)) {
			asoc->fragmented_delivery_inprogress = 1;
			asoc->tsn_last_delivered = chk->rec.data.TSN_seq - 1;
			asoc->str_of_pdapi = chk->rec.data.stream_number;
			asoc->ssn_of_pdapi = chk->rec.data.stream_seq;
			asoc->pdapi_ppid = chk->rec.data.payloadtype;
			asoc->fragment_flags = chk->rec.data.rcv_flags;
			sctp_service_reassembly(stcb, asoc);
			if (asoc->fragmented_delivery_inprogress == 0) {
				goto doit_again;
			}
		}
	}
}

int
sctp_process_data(struct mbuf **mm, int iphlen, int *offset, int length,
                  struct sockaddr *src, struct sockaddr *dst,
                  struct sctphdr *sh, struct sctp_inpcb *inp,
                  struct sctp_tcb *stcb, struct sctp_nets *net, uint32_t *high_tsn,
#if defined(__FreeBSD__)
                  uint8_t use_mflowid, uint32_t mflowid,
#endif
		  uint32_t vrf_id, uint16_t port)
{
	struct sctp_data_chunk *ch, chunk_buf;
	struct sctp_association *asoc;
	int num_chunks = 0;	
	int stop_proc = 0;
	int chk_length, break_flag, last_chunk;
	int abort_flag = 0, was_a_gap;
	struct mbuf *m;
	uint32_t highest_tsn;

	
	sctp_set_rwnd(stcb, &stcb->asoc);

	m = *mm;
	SCTP_TCB_LOCK_ASSERT(stcb);
	asoc = &stcb->asoc;
	if (SCTP_TSN_GT(asoc->highest_tsn_inside_nr_map, asoc->highest_tsn_inside_map)) {
		highest_tsn = asoc->highest_tsn_inside_nr_map;
	} else {
		highest_tsn = asoc->highest_tsn_inside_map;
	}
	was_a_gap = SCTP_TSN_GT(highest_tsn, stcb->asoc.cumulative_tsn);
	




	asoc->last_data_chunk_from = net;

#ifndef __Panda__
	







	if (SCTP_BUF_LEN(m) < (long)MLEN && SCTP_BUF_NEXT(m) == NULL) {
		
		m = sctp_get_mbuf_for_msg(SCTP_BUF_LEN(m), 0, M_NOWAIT, 1, MT_DATA);
		if (m) {
			
			caddr_t *from, *to;
			
			to = mtod(m, caddr_t *);
			from = mtod((*mm), caddr_t *);
			memcpy(to, from, SCTP_BUF_LEN((*mm)));
			
			SCTP_BUF_LEN(m) = SCTP_BUF_LEN((*mm));
			sctp_m_freem(*mm);
			
			*mm = m;
		} else {
			
			m = *mm;
		}
	}
#endif
	
	ch = (struct sctp_data_chunk *)sctp_m_getptr(m, *offset,
						     sizeof(struct sctp_data_chunk), (uint8_t *) & chunk_buf);
	if (ch == NULL) {
		return (1);
	}
	


	*high_tsn = asoc->cumulative_tsn;
	break_flag = 0;
	asoc->data_pkts_seen++;
	while (stop_proc == 0) {
		
		chk_length = ntohs(ch->ch.chunk_length);
		if (length - *offset < chk_length) {
			
			stop_proc = 1;
			continue;
		}
		if (ch->ch.chunk_type == SCTP_DATA) {
			if ((size_t)chk_length < sizeof(struct sctp_data_chunk) + 1) {
				



				struct mbuf *op_err;

				op_err = sctp_get_mbuf_for_msg((sizeof(struct sctp_paramhdr) + 2 * sizeof(uint32_t)),
							       0, M_NOWAIT, 1, MT_DATA);

				if (op_err) {
					struct sctp_paramhdr *ph;
					uint32_t *ippp;

					SCTP_BUF_LEN(op_err) = sizeof(struct sctp_paramhdr) +
						(2 * sizeof(uint32_t));
					ph = mtod(op_err, struct sctp_paramhdr *);
					ph->param_type =
						htons(SCTP_CAUSE_PROTOCOL_VIOLATION);
					ph->param_length = htons(SCTP_BUF_LEN(op_err));
					ippp = (uint32_t *) (ph + 1);
					*ippp = htonl(SCTP_FROM_SCTP_INDATA+SCTP_LOC_19);
					ippp++;
					*ippp = asoc->cumulative_tsn;

				}
				stcb->sctp_ep->last_abort_code = SCTP_FROM_SCTP_INDATA+SCTP_LOC_19;
				sctp_abort_association(inp, stcb, m, iphlen,
				                       src, dst, sh, op_err,
#if defined(__FreeBSD__)
				                       use_mflowid, mflowid,
#endif
				                       vrf_id, port);
				return (2);
			}
#ifdef SCTP_AUDITING_ENABLED
			sctp_audit_log(0xB1, 0);
#endif
			if (SCTP_SIZE32(chk_length) == (length - *offset)) {
				last_chunk = 1;
			} else {
				last_chunk = 0;
			}
			if (sctp_process_a_data_chunk(stcb, asoc, mm, *offset, ch,
						      chk_length, net, high_tsn, &abort_flag, &break_flag,
						      last_chunk)) {
				num_chunks++;
			}
			if (abort_flag)
				return (2);

			if (break_flag) {
				



				stop_proc = 1;
				continue;
			}
		} else {
			
			switch (ch->ch.chunk_type) {
			case SCTP_INITIATION:
			case SCTP_INITIATION_ACK:
			case SCTP_SELECTIVE_ACK:
			case SCTP_NR_SELECTIVE_ACK:
			case SCTP_HEARTBEAT_REQUEST:
			case SCTP_HEARTBEAT_ACK:
			case SCTP_ABORT_ASSOCIATION:
			case SCTP_SHUTDOWN:
			case SCTP_SHUTDOWN_ACK:
			case SCTP_OPERATION_ERROR:
			case SCTP_COOKIE_ECHO:
			case SCTP_COOKIE_ACK:
			case SCTP_ECN_ECHO:
			case SCTP_ECN_CWR:
			case SCTP_SHUTDOWN_COMPLETE:
			case SCTP_AUTHENTICATION:
			case SCTP_ASCONF_ACK:
			case SCTP_PACKET_DROPPED:
			case SCTP_STREAM_RESET:
			case SCTP_FORWARD_CUM_TSN:
			case SCTP_ASCONF:
				








				if (SCTP_BASE_SYSCTL(sctp_strict_data_order)) {
					struct mbuf *op_err;

					op_err = sctp_generate_invmanparam(SCTP_CAUSE_PROTOCOL_VIOLATION);
					sctp_abort_association(inp, stcb,
					                       m, iphlen,
					                       src, dst,
					                       sh, op_err,
#if defined(__FreeBSD__)
					                       use_mflowid, mflowid,
#endif
					                       vrf_id, port);
					return (2);
				}
				break;
			default:
				
				if (ch->ch.chunk_type & 0x40) {
					
					struct mbuf *merr;
					struct sctp_paramhdr *phd;

					merr = sctp_get_mbuf_for_msg(sizeof(*phd), 0, M_NOWAIT, 1, MT_DATA);
					if (merr) {
						phd = mtod(merr, struct sctp_paramhdr *);
						







						phd->param_type =
							htons(SCTP_CAUSE_UNRECOG_CHUNK);
						phd->param_length =
							htons(chk_length + sizeof(*phd));
						SCTP_BUF_LEN(merr) = sizeof(*phd);
						SCTP_BUF_NEXT(merr) = SCTP_M_COPYM(m, *offset, chk_length, M_NOWAIT);
						if (SCTP_BUF_NEXT(merr)) {
							if (sctp_pad_lastmbuf(SCTP_BUF_NEXT(merr), SCTP_SIZE32(chk_length) - chk_length, NULL)) {
								sctp_m_freem(merr);
							} else {
								sctp_queue_op_err(stcb, merr);
							}
						} else {
							sctp_m_freem(merr);
						}
					}
				}
				if ((ch->ch.chunk_type & 0x80) == 0) {
					
					stop_proc = 1;
				}	

				break;
			}	
		}
		*offset += SCTP_SIZE32(chk_length);
		if ((*offset >= length) || stop_proc) {
			
			stop_proc = 1;
			continue;
		}
		ch = (struct sctp_data_chunk *)sctp_m_getptr(m, *offset,
							     sizeof(struct sctp_data_chunk), (uint8_t *) & chunk_buf);
		if (ch == NULL) {
			*offset = length;
			stop_proc = 1;
			continue;
		}
	}
	if (break_flag) {
		


		sctp_send_packet_dropped(stcb, net, *mm, length, iphlen, 0);
	}
	if (num_chunks) {
		



		SCTP_STAT_INCR(sctps_recvpktwithdata);
		if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_THRESHOLD_LOGGING) {
			sctp_misc_ints(SCTP_THRESHOLD_CLEAR,
				       stcb->asoc.overall_error_count,
				       0,
				       SCTP_FROM_SCTP_INDATA,
				       __LINE__);
		}
		stcb->asoc.overall_error_count = 0;
		(void)SCTP_GETTIME_TIMEVAL(&stcb->asoc.time_last_rcvd);
	}
	
	if (!(TAILQ_EMPTY(&asoc->reasmqueue)))
		sctp_service_queues(stcb, asoc);

	if (SCTP_GET_STATE(asoc) == SCTP_STATE_SHUTDOWN_SENT) {
		
		stcb->asoc.send_sack = 1;
	}
	
	sctp_sack_check(stcb, was_a_gap);
	return (0);
}

static int
sctp_process_segment_range(struct sctp_tcb *stcb, struct sctp_tmit_chunk **p_tp1, uint32_t last_tsn,
			   uint16_t frag_strt, uint16_t frag_end, int nr_sacking,
			   int *num_frs,
			   uint32_t *biggest_newly_acked_tsn,
			   uint32_t  *this_sack_lowest_newack,
			   int *rto_ok)
{
	struct sctp_tmit_chunk *tp1;
	unsigned int theTSN;
	int j, wake_him = 0, circled = 0;

	
	tp1 = *p_tp1;
	if (tp1 == NULL) {
		tp1 = TAILQ_FIRST(&stcb->asoc.sent_queue);
	}
	for (j = frag_strt; j <= frag_end; j++) {
		theTSN = j + last_tsn;
		while (tp1) {
			if (tp1->rec.data.doing_fast_retransmit)
				(*num_frs) += 1;

			







			if ((tp1->whoTo->find_pseudo_cumack == 1) && (tp1->sent < SCTP_DATAGRAM_RESEND) &&
			    (tp1->snd_count == 1)) {
				tp1->whoTo->pseudo_cumack = tp1->rec.data.TSN_seq;
				tp1->whoTo->find_pseudo_cumack = 0;
			}
			if ((tp1->whoTo->find_rtx_pseudo_cumack == 1) && (tp1->sent < SCTP_DATAGRAM_RESEND) &&
			    (tp1->snd_count > 1)) {
				tp1->whoTo->rtx_pseudo_cumack = tp1->rec.data.TSN_seq;
				tp1->whoTo->find_rtx_pseudo_cumack = 0;
			}
			if (tp1->rec.data.TSN_seq == theTSN) {
				if (tp1->sent != SCTP_DATAGRAM_UNSENT) {
					



					if (tp1->sent < SCTP_DATAGRAM_RESEND) {
						






						if (SCTP_TSN_GT(tp1->rec.data.TSN_seq,
						                *biggest_newly_acked_tsn)) {
							*biggest_newly_acked_tsn = tp1->rec.data.TSN_seq;
						}
						






						if (tp1->rec.data.chunk_was_revoked == 0)
							tp1->whoTo->saw_newack = 1;

						if (SCTP_TSN_GT(tp1->rec.data.TSN_seq,
						                tp1->whoTo->this_sack_highest_newack)) {
							tp1->whoTo->this_sack_highest_newack =
								tp1->rec.data.TSN_seq;
						}
						



						if (*this_sack_lowest_newack == 0) {
							if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_SACK_LOGGING_ENABLE) {
								sctp_log_sack(*this_sack_lowest_newack,
									      last_tsn,
									      tp1->rec.data.TSN_seq,
									      0,
									      0,
									      SCTP_LOG_TSN_ACKED);
							}
							*this_sack_lowest_newack = tp1->rec.data.TSN_seq;
						}
						







						if (tp1->rec.data.TSN_seq == tp1->whoTo->pseudo_cumack) {
							if (tp1->rec.data.chunk_was_revoked == 0) {
								tp1->whoTo->new_pseudo_cumack = 1;
							}
							tp1->whoTo->find_pseudo_cumack = 1;
						}
						if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_CWND_LOGGING_ENABLE) {
							sctp_log_cwnd(stcb, tp1->whoTo, tp1->rec.data.TSN_seq, SCTP_CWND_LOG_FROM_SACK);
						}
						if (tp1->rec.data.TSN_seq == tp1->whoTo->rtx_pseudo_cumack) {
							if (tp1->rec.data.chunk_was_revoked == 0) {
								tp1->whoTo->new_pseudo_cumack = 1;
							}
							tp1->whoTo->find_rtx_pseudo_cumack = 1;
						}
						if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_SACK_LOGGING_ENABLE) {
							sctp_log_sack(*biggest_newly_acked_tsn,
								      last_tsn,
								      tp1->rec.data.TSN_seq,
								      frag_strt,
								      frag_end,
								      SCTP_LOG_TSN_ACKED);
						}
						if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_FLIGHT_LOGGING_ENABLE) {
							sctp_misc_ints(SCTP_FLIGHT_LOG_DOWN_GAP,
								       tp1->whoTo->flight_size,
								       tp1->book_size,
								       (uintptr_t)tp1->whoTo,
								       tp1->rec.data.TSN_seq);
						}
						sctp_flight_size_decrease(tp1);
						if (stcb->asoc.cc_functions.sctp_cwnd_update_tsn_acknowledged) {
							(*stcb->asoc.cc_functions.sctp_cwnd_update_tsn_acknowledged)(tp1->whoTo,
														     tp1);
						}
						sctp_total_flight_decrease(stcb, tp1);

						tp1->whoTo->net_ack += tp1->send_size;
						if (tp1->snd_count < 2) {
							


							tp1->whoTo->net_ack2 += tp1->send_size;

							


							if (tp1->do_rtt) {
								if (*rto_ok) {
									tp1->whoTo->RTO =
										sctp_calculate_rto(stcb,
												   &stcb->asoc,
												   tp1->whoTo,
												   &tp1->sent_rcv_time,
												   sctp_align_safe_nocopy,
												   SCTP_RTT_FROM_DATA);
									*rto_ok = 0;
								}
								if (tp1->whoTo->rto_needed == 0) {
									tp1->whoTo->rto_needed = 1;
								}
								tp1->do_rtt = 0;
							}
						}

					}
					if (tp1->sent <= SCTP_DATAGRAM_RESEND) {
						if (SCTP_TSN_GT(tp1->rec.data.TSN_seq,
						                stcb->asoc.this_sack_highest_gap)) {
							stcb->asoc.this_sack_highest_gap =
								tp1->rec.data.TSN_seq;
						}
						if (tp1->sent == SCTP_DATAGRAM_RESEND) {
							sctp_ucount_decr(stcb->asoc.sent_queue_retran_cnt);
#ifdef SCTP_AUDITING_ENABLED
							sctp_audit_log(0xB2,
								       (stcb->asoc.sent_queue_retran_cnt & 0x000000ff));
#endif
						}
					}
					



					if ((tp1->sent != SCTP_FORWARD_TSN_SKIP) &&
					    (tp1->sent != SCTP_DATAGRAM_NR_ACKED)) {
						tp1->sent = SCTP_DATAGRAM_MARKED;
					}
					if (tp1->rec.data.chunk_was_revoked) {
						
						tp1->whoTo->cwnd -= tp1->book_size;
						tp1->rec.data.chunk_was_revoked = 0;
					}
					
					if (nr_sacking &&
					    (tp1->sent != SCTP_DATAGRAM_NR_ACKED)) {
						if (stcb->asoc.strmout[tp1->rec.data.stream_number].chunks_on_queues > 0) {
							stcb->asoc.strmout[tp1->rec.data.stream_number].chunks_on_queues--;
#ifdef INVARIANTS
						} else {
							panic("No chunks on the queues for sid %u.", tp1->rec.data.stream_number);
#endif
						}
						tp1->sent = SCTP_DATAGRAM_NR_ACKED;
						if (tp1->data) {
							
							sctp_free_bufspace(stcb, &stcb->asoc, tp1, 1);
							sctp_m_freem(tp1->data);
							tp1->data = NULL;
						}
						wake_him++;
					}
				}
				break;
			}	
			if (SCTP_TSN_GT(tp1->rec.data.TSN_seq, theTSN)) {
				break;
			}
			tp1 = TAILQ_NEXT(tp1, sctp_next);
			if ((tp1 == NULL) && (circled == 0)) {
				circled++;
				tp1 = TAILQ_FIRST(&stcb->asoc.sent_queue);
			}
		}	
		if (tp1 == NULL) {
			circled = 0;
			tp1 = TAILQ_FIRST(&stcb->asoc.sent_queue);
		}
		
	} 
	*p_tp1 = tp1;
	return (wake_him);	
}


static int
sctp_handle_segments(struct mbuf *m, int *offset, struct sctp_tcb *stcb, struct sctp_association *asoc,
		uint32_t last_tsn, uint32_t *biggest_tsn_acked,
		uint32_t *biggest_newly_acked_tsn, uint32_t *this_sack_lowest_newack,
		int num_seg, int num_nr_seg, int *rto_ok)
{
	struct sctp_gap_ack_block *frag, block;
	struct sctp_tmit_chunk *tp1;
	int i;
	int num_frs = 0;
	int chunk_freed;
	int non_revocable;
	uint16_t frag_strt, frag_end, prev_frag_end;

	tp1 = TAILQ_FIRST(&asoc->sent_queue);
	prev_frag_end = 0;
	chunk_freed = 0;

	for (i = 0; i < (num_seg + num_nr_seg); i++) {
		if (i == num_seg) {
			prev_frag_end = 0;
			tp1 = TAILQ_FIRST(&asoc->sent_queue);
		}
		frag = (struct sctp_gap_ack_block *)sctp_m_getptr(m, *offset,
		                                                  sizeof(struct sctp_gap_ack_block), (uint8_t *) &block);
		*offset += sizeof(block);
		if (frag == NULL) {
			return (chunk_freed);
		}
		frag_strt = ntohs(frag->start);
		frag_end = ntohs(frag->end);

		if (frag_strt > frag_end) {
			
			continue;
		}
		if (frag_strt <= prev_frag_end) {
			
			 tp1 = TAILQ_FIRST(&asoc->sent_queue);
		}
		if (SCTP_TSN_GT((last_tsn + frag_end), *biggest_tsn_acked)) {
			*biggest_tsn_acked = last_tsn + frag_end;
		}
		if (i < num_seg) {
			non_revocable = 0;
		} else {
			non_revocable = 1;
		}
		if (sctp_process_segment_range(stcb, &tp1, last_tsn, frag_strt, frag_end,
		                               non_revocable, &num_frs, biggest_newly_acked_tsn,
		                               this_sack_lowest_newack, rto_ok)) {
			chunk_freed = 1;
		}
		prev_frag_end = frag_end;
	}
	if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_FR_LOGGING_ENABLE) {
		if (num_frs)
			sctp_log_fr(*biggest_tsn_acked,
			            *biggest_newly_acked_tsn,
			            last_tsn, SCTP_FR_LOG_BIGGEST_TSNS);
	}
	return (chunk_freed);
}

static void
sctp_check_for_revoked(struct sctp_tcb *stcb,
		       struct sctp_association *asoc, uint32_t cumack,
		       uint32_t biggest_tsn_acked)
{
	struct sctp_tmit_chunk *tp1;

	TAILQ_FOREACH(tp1, &asoc->sent_queue, sctp_next) {
		if (SCTP_TSN_GT(tp1->rec.data.TSN_seq, cumack)) {
			





			if (SCTP_TSN_GT(tp1->rec.data.TSN_seq, biggest_tsn_acked)) {
				break;
			}
			if (tp1->sent == SCTP_DATAGRAM_ACKED) {
				
				tp1->sent = SCTP_DATAGRAM_SENT;
				tp1->rec.data.chunk_was_revoked = 1;
				


				if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_FLIGHT_LOGGING_ENABLE) {
					sctp_misc_ints(SCTP_FLIGHT_LOG_UP_REVOKE,
						       tp1->whoTo->flight_size,
						       tp1->book_size,
						       (uintptr_t)tp1->whoTo,
						       tp1->rec.data.TSN_seq);
				}
				sctp_flight_size_increase(tp1);
				sctp_total_flight_increase(stcb, tp1);
				


				tp1->whoTo->cwnd += tp1->book_size;
				if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_SACK_LOGGING_ENABLE) {
					sctp_log_sack(asoc->last_acked_seq,
						      cumack,
						      tp1->rec.data.TSN_seq,
						      0,
						      0,
						      SCTP_LOG_TSN_REVOKED);
				}
			} else if (tp1->sent == SCTP_DATAGRAM_MARKED) {
				
				tp1->sent = SCTP_DATAGRAM_ACKED;
			}
		}
		if (tp1->sent == SCTP_DATAGRAM_UNSENT)
			break;
	}
}


static void
sctp_strike_gap_ack_chunks(struct sctp_tcb *stcb, struct sctp_association *asoc,
			   uint32_t biggest_tsn_acked, uint32_t biggest_tsn_newly_acked, uint32_t this_sack_lowest_newack, int accum_moved)
{
	struct sctp_tmit_chunk *tp1;
	int strike_flag = 0;
	struct timeval now;
	int tot_retrans = 0;
	uint32_t sending_seq;
	struct sctp_nets *net;
	int num_dests_sacked = 0;

	



	tp1 = TAILQ_FIRST(&stcb->asoc.send_queue);
	if (tp1 == NULL) {
		sending_seq = asoc->sending_seq;
	} else {
		sending_seq = tp1->rec.data.TSN_seq;
	}

	
	if ((asoc->sctp_cmt_on_off > 0) &&
	    SCTP_BASE_SYSCTL(sctp_cmt_use_dac)) {
		TAILQ_FOREACH(net, &asoc->nets, sctp_next) {
			if (net->saw_newack)
				num_dests_sacked++;
		}
	}
	if (stcb->asoc.peer_supports_prsctp) {
		(void)SCTP_GETTIME_TIMEVAL(&now);
	}
	TAILQ_FOREACH(tp1, &asoc->sent_queue, sctp_next) {
		strike_flag = 0;
		if (tp1->no_fr_allowed) {
			
			continue;
		}
		if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_FR_LOGGING_ENABLE) {
			if (tp1->sent < SCTP_DATAGRAM_RESEND)
				sctp_log_fr(biggest_tsn_newly_acked,
					    tp1->rec.data.TSN_seq,
					    tp1->sent,
					    SCTP_FR_LOG_CHECK_STRIKE);
		}
		if (SCTP_TSN_GT(tp1->rec.data.TSN_seq, biggest_tsn_acked) ||
		    tp1->sent == SCTP_DATAGRAM_UNSENT) {
			
			break;
		}
		if (stcb->asoc.peer_supports_prsctp) {
			if ((PR_SCTP_TTL_ENABLED(tp1->flags)) && tp1->sent < SCTP_DATAGRAM_ACKED) {
				
#ifndef __FreeBSD__
				if (timercmp(&now, &tp1->rec.data.timetodrop, >)) {
#else
				if (timevalcmp(&now, &tp1->rec.data.timetodrop, >)) {
#endif

					if (tp1->data != NULL) {
						(void)sctp_release_pr_sctp_chunk(stcb, tp1, 1,
										 SCTP_SO_NOT_LOCKED);
					}
					continue;
				}
			}

		}
		if (SCTP_TSN_GT(tp1->rec.data.TSN_seq, asoc->this_sack_highest_gap)) {
			
			break;
		}
		if (tp1->sent >= SCTP_DATAGRAM_RESEND) {
			
			
			if (tp1->sent == SCTP_FORWARD_TSN_SKIP) {
				
				tp1->rec.data.fwd_tsn_cnt++;
			}
			continue;
		}
		


		if (tp1->whoTo && tp1->whoTo->saw_newack == 0) {
			





			continue;
		} else if (tp1->whoTo && SCTP_TSN_GT(tp1->rec.data.TSN_seq,
		                                     tp1->whoTo->this_sack_highest_newack)) {
			







			continue;
		}
		





		



		if (accum_moved && asoc->fast_retran_loss_recovery) {
			



			if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_FR_LOGGING_ENABLE) {
				sctp_log_fr(biggest_tsn_newly_acked,
					    tp1->rec.data.TSN_seq,
					    tp1->sent,
					    SCTP_FR_LOG_STRIKE_CHUNK);
			}
			if (tp1->sent < SCTP_DATAGRAM_RESEND) {
				tp1->sent++;
			}
			if ((asoc->sctp_cmt_on_off > 0) &&
			    SCTP_BASE_SYSCTL(sctp_cmt_use_dac)) {
				










				if ((tp1->sent < SCTP_DATAGRAM_RESEND) && (num_dests_sacked == 1) &&
				    SCTP_TSN_GT(this_sack_lowest_newack, tp1->rec.data.TSN_seq)) {
					if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_FR_LOGGING_ENABLE) {
						sctp_log_fr(16 + num_dests_sacked,
							    tp1->rec.data.TSN_seq,
							    tp1->sent,
							    SCTP_FR_LOG_STRIKE_CHUNK);
					}
					tp1->sent++;
				}
			}
		} else if ((tp1->rec.data.doing_fast_retransmit) &&
		           (asoc->sctp_cmt_on_off == 0)) {
			





			if (
#ifdef SCTP_FR_TO_ALTERNATE
				





				(asoc->numnets < 2)
#else
				(1)
#endif
				) {

				if (SCTP_TSN_GE(biggest_tsn_newly_acked,
				                tp1->rec.data.fast_retran_tsn)) {
					




					if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_FR_LOGGING_ENABLE) {
						sctp_log_fr(biggest_tsn_newly_acked,
							    tp1->rec.data.TSN_seq,
							    tp1->sent,
							    SCTP_FR_LOG_STRIKE_CHUNK);
					}
					if (tp1->sent < SCTP_DATAGRAM_RESEND) {
						tp1->sent++;
					}
					strike_flag = 1;
					if ((asoc->sctp_cmt_on_off > 0) &&
					    SCTP_BASE_SYSCTL(sctp_cmt_use_dac)) {
						














						if ((tp1->sent < SCTP_DATAGRAM_RESEND) &&
						    (num_dests_sacked == 1) &&
						    SCTP_TSN_GT(this_sack_lowest_newack,
						                tp1->rec.data.TSN_seq)) {
							if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_FR_LOGGING_ENABLE) {
								sctp_log_fr(32 + num_dests_sacked,
									    tp1->rec.data.TSN_seq,
									    tp1->sent,
									    SCTP_FR_LOG_STRIKE_CHUNK);
							}
							if (tp1->sent < SCTP_DATAGRAM_RESEND) {
								tp1->sent++;
							}
						}
					}
				}
			}
			



		} else if (SCTP_TSN_GT(tp1->rec.data.TSN_seq,
		                       biggest_tsn_newly_acked)) {
			




			;
		} else {
			
			if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_FR_LOGGING_ENABLE) {
				sctp_log_fr(biggest_tsn_newly_acked,
					    tp1->rec.data.TSN_seq,
					    tp1->sent,
					    SCTP_FR_LOG_STRIKE_CHUNK);
			}
			if (tp1->sent < SCTP_DATAGRAM_RESEND) {
				tp1->sent++;
			}
			if ((asoc->sctp_cmt_on_off > 0) &&
			    SCTP_BASE_SYSCTL(sctp_cmt_use_dac)) {
				










				if ((tp1->sent < SCTP_DATAGRAM_RESEND) && (num_dests_sacked == 1) &&
				    SCTP_TSN_GT(this_sack_lowest_newack, tp1->rec.data.TSN_seq)) {
					if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_FR_LOGGING_ENABLE) {
						sctp_log_fr(48 + num_dests_sacked,
							    tp1->rec.data.TSN_seq,
							    tp1->sent,
							    SCTP_FR_LOG_STRIKE_CHUNK);
					}
					tp1->sent++;
				}
			}
		}
		if (tp1->sent == SCTP_DATAGRAM_RESEND) {
			struct sctp_nets *alt;

			
			if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_FLIGHT_LOGGING_ENABLE) {
				sctp_misc_ints(SCTP_FLIGHT_LOG_DOWN_RSND,
					       (tp1->whoTo ? (tp1->whoTo->flight_size) : 0),
					       tp1->book_size,
					       (uintptr_t)tp1->whoTo,
					       tp1->rec.data.TSN_seq);
			}
			if (tp1->whoTo) {
				tp1->whoTo->net_ack++;
				sctp_flight_size_decrease(tp1);
				if (stcb->asoc.cc_functions.sctp_cwnd_update_tsn_acknowledged) {
					(*stcb->asoc.cc_functions.sctp_cwnd_update_tsn_acknowledged)(tp1->whoTo,
												     tp1);
				}
			}

			if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_LOG_RWND_ENABLE) {
				sctp_log_rwnd(SCTP_INCREASE_PEER_RWND,
					      asoc->peers_rwnd, tp1->send_size, SCTP_BASE_SYSCTL(sctp_peer_chunk_oh));
			}
			
			asoc->peers_rwnd += (tp1->send_size + SCTP_BASE_SYSCTL(sctp_peer_chunk_oh));

			
			sctp_total_flight_decrease(stcb, tp1);

			if ((stcb->asoc.peer_supports_prsctp) &&
			    (PR_SCTP_RTX_ENABLED(tp1->flags))) {
				
				if (tp1->snd_count > tp1->rec.data.timetodrop.tv_sec) {
					
					if (tp1->data != NULL) {
						(void)sctp_release_pr_sctp_chunk(stcb, tp1, 1,
										 SCTP_SO_NOT_LOCKED);
					}
					
					tp1->whoTo->net_ack++;
					continue;
				}
			}
			
			if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_FR_LOGGING_ENABLE) {
				sctp_log_fr(tp1->rec.data.TSN_seq, tp1->snd_count,
					    0, SCTP_FR_MARKED);
			}
			if (strike_flag) {
				
				SCTP_STAT_INCR(sctps_sendmultfastretrans);
			}
			sctp_ucount_incr(stcb->asoc.sent_queue_retran_cnt);
			if (asoc->sctp_cmt_on_off > 0) {
				




				tp1->no_fr_allowed = 1;
				alt = tp1->whoTo;
				
				if (asoc->sctp_cmt_pf > 0) {
					
					alt = sctp_find_alternate_net(stcb, alt, 2);
				} else {
					
                                        
					alt = sctp_find_alternate_net(stcb, alt, 1);
				}
				if (alt == NULL) {
					alt = tp1->whoTo;
				}
				






				if (tp1->whoTo) {
					tp1->whoTo->find_pseudo_cumack = 1;
					tp1->whoTo->find_rtx_pseudo_cumack = 1;
				}

			} else {

#ifdef SCTP_FR_TO_ALTERNATE
				
				alt = sctp_find_alternate_net(stcb, tp1->whoTo, 0);
#else
				




				alt = tp1->whoTo;
#endif
			}

			tp1->rec.data.doing_fast_retransmit = 1;
			tot_retrans++;
			
			



			if (TAILQ_EMPTY(&asoc->send_queue)) {
				





				tp1->rec.data.fast_retran_tsn = sending_seq;
			} else {
				







				struct sctp_tmit_chunk *ttt;

				ttt = TAILQ_FIRST(&asoc->send_queue);
				tp1->rec.data.fast_retran_tsn =
					ttt->rec.data.TSN_seq;
			}

			if (tp1->do_rtt) {
				



				if ((tp1->whoTo != NULL) &&
				    (tp1->whoTo->rto_needed == 0)) {
					tp1->whoTo->rto_needed = 1;
				}
				tp1->do_rtt = 0;
			}
			if (alt != tp1->whoTo) {
				
				sctp_free_remote_addr(tp1->whoTo);
				
				tp1->whoTo = alt;
				atomic_add_int(&alt->ref_count, 1);
			}
		}
	}
}

struct sctp_tmit_chunk *
sctp_try_advance_peer_ack_point(struct sctp_tcb *stcb,
    struct sctp_association *asoc)
{
	struct sctp_tmit_chunk *tp1, *tp2, *a_adv = NULL;
	struct timeval now;
	int now_filled = 0;

	if (asoc->peer_supports_prsctp == 0) {
		return (NULL);
	}
	TAILQ_FOREACH_SAFE(tp1, &asoc->sent_queue, sctp_next, tp2) {
		if (tp1->sent != SCTP_FORWARD_TSN_SKIP &&
		    tp1->sent != SCTP_DATAGRAM_RESEND &&
		    tp1->sent != SCTP_DATAGRAM_NR_ACKED) {
			
			break;
		}
		if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_LOG_TRY_ADVANCE) {
			if ((tp1->sent == SCTP_FORWARD_TSN_SKIP) ||
			    (tp1->sent == SCTP_DATAGRAM_NR_ACKED)) {
				sctp_misc_ints(SCTP_FWD_TSN_CHECK,
					       asoc->advanced_peer_ack_point,
					       tp1->rec.data.TSN_seq, 0, 0);
			}
		}
		if (!PR_SCTP_ENABLED(tp1->flags)) {
			



			break;
		}
		if (!now_filled) {
			(void)SCTP_GETTIME_TIMEVAL(&now);
			now_filled = 1;
		}
		





		if (tp1->sent == SCTP_DATAGRAM_RESEND &&
		    (PR_SCTP_TTL_ENABLED(tp1->flags))) {
			



#ifndef __FreeBSD__
			if (timercmp(&now, &tp1->rec.data.timetodrop, >)) {
#else
			if (timevalcmp(&now, &tp1->rec.data.timetodrop, >)) {
#endif

				if (tp1->data) {
					(void)sctp_release_pr_sctp_chunk(stcb, tp1,
					    1, SCTP_SO_NOT_LOCKED);
				}
			} else {
				



				break;
			}
		}
		




		if ((tp1->sent == SCTP_FORWARD_TSN_SKIP) ||
		    (tp1->sent == SCTP_DATAGRAM_NR_ACKED)) {
			
			if (SCTP_TSN_GT(tp1->rec.data.TSN_seq, asoc->advanced_peer_ack_point)) {
				asoc->advanced_peer_ack_point = tp1->rec.data.TSN_seq;
				a_adv = tp1;
			} else if (tp1->rec.data.TSN_seq == asoc->advanced_peer_ack_point) {
				
				a_adv = tp1;
			}
		} else {
			



			break;
		}
	}
	return (a_adv);
}

static int
sctp_fs_audit(struct sctp_association *asoc)
{
	struct sctp_tmit_chunk *chk;
	int inflight = 0, resend = 0, inbetween = 0, acked = 0, above = 0;
	int entry_flight, entry_cnt, ret;
	entry_flight = asoc->total_flight;
	entry_cnt = asoc->total_flight_count;
	ret = 0;

	if (asoc->pr_sctp_cnt >= asoc->sent_queue_cnt)
		return (0);

	TAILQ_FOREACH(chk, &asoc->sent_queue, sctp_next) {
		if (chk->sent < SCTP_DATAGRAM_RESEND) {
			SCTP_PRINTF("Chk TSN:%u size:%d inflight cnt:%d\n",
			            chk->rec.data.TSN_seq,
			            chk->send_size,
			            chk->snd_count);
			inflight++;
		} else if (chk->sent == SCTP_DATAGRAM_RESEND) {
			resend++;
		} else if (chk->sent < SCTP_DATAGRAM_ACKED) {
			inbetween++;
		} else if (chk->sent > SCTP_DATAGRAM_ACKED) {
			above++;
		} else {
			acked++;
		}
	}

	if ((inflight > 0) || (inbetween > 0)) {
#ifdef INVARIANTS
		panic("Flight size-express incorrect? \n");
#else
		SCTP_PRINTF("asoc->total_flight:%d cnt:%d\n",
		            entry_flight, entry_cnt);

		SCTP_PRINTF("Flight size-express incorrect F:%d I:%d R:%d Ab:%d ACK:%d\n",
			    inflight, inbetween, resend, above, acked);
		ret = 1;
#endif
	}
	return (ret);
}


static void
sctp_window_probe_recovery(struct sctp_tcb *stcb,
	                   struct sctp_association *asoc,
			   struct sctp_tmit_chunk *tp1)
{
	tp1->window_probe = 0;
	if ((tp1->sent >= SCTP_DATAGRAM_ACKED) || (tp1->data == NULL)) {
		
		sctp_misc_ints(SCTP_FLIGHT_LOG_DWN_WP_FWD,
			       tp1->whoTo->flight_size,
			       tp1->book_size,
			       (uintptr_t)tp1->whoTo,
			       tp1->rec.data.TSN_seq);
		return;
	}
	
	if (stcb->asoc.cc_functions.sctp_cwnd_update_tsn_acknowledged) {
		(*stcb->asoc.cc_functions.sctp_cwnd_update_tsn_acknowledged)(tp1->whoTo,
									     tp1);
	}
	sctp_flight_size_decrease(tp1);
	sctp_total_flight_decrease(stcb, tp1);
	
	tp1->sent = SCTP_DATAGRAM_RESEND;
	sctp_ucount_incr(asoc->sent_queue_retran_cnt);

	if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_FLIGHT_LOGGING_ENABLE) {
		sctp_misc_ints(SCTP_FLIGHT_LOG_DOWN_WP,
			       tp1->whoTo->flight_size,
			       tp1->book_size,
			       (uintptr_t)tp1->whoTo,
			       tp1->rec.data.TSN_seq);
	}
}

void
sctp_express_handle_sack(struct sctp_tcb *stcb, uint32_t cumack,
                         uint32_t rwnd, int *abort_now, int ecne_seen)
{
	struct sctp_nets *net;
	struct sctp_association *asoc;
	struct sctp_tmit_chunk *tp1, *tp2;
	uint32_t old_rwnd;
	int win_probe_recovery = 0;
	int win_probe_recovered = 0;
	int j, done_once = 0;
	int rto_ok = 1;

	if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_LOG_SACK_ARRIVALS_ENABLE) {
		sctp_misc_ints(SCTP_SACK_LOG_EXPRESS, cumack,
		               rwnd, stcb->asoc.last_acked_seq, stcb->asoc.peers_rwnd);
	}
	SCTP_TCB_LOCK_ASSERT(stcb);
#ifdef SCTP_ASOCLOG_OF_TSNS
	stcb->asoc.cumack_log[stcb->asoc.cumack_log_at] = cumack;
	stcb->asoc.cumack_log_at++;
	if (stcb->asoc.cumack_log_at > SCTP_TSN_LOG_SIZE) {
		stcb->asoc.cumack_log_at = 0;
	}
#endif
	asoc = &stcb->asoc;
	old_rwnd = asoc->peers_rwnd;
	if (SCTP_TSN_GT(asoc->last_acked_seq, cumack)) {
		
		return;
	} else if (asoc->last_acked_seq == cumack) {
		
		asoc->peers_rwnd = sctp_sbspace_sub(rwnd,
						    (uint32_t) (asoc->total_flight + (asoc->total_flight_count * SCTP_BASE_SYSCTL(sctp_peer_chunk_oh))));
		if (asoc->peers_rwnd < stcb->sctp_ep->sctp_ep.sctp_sws_sender) {
			
			asoc->peers_rwnd = 0;
		}
		if (asoc->peers_rwnd > old_rwnd) {
			goto again;
		}
		return;
	}

	
	TAILQ_FOREACH(net, &asoc->nets, sctp_next) {
		if (SCTP_TSN_GT(cumack, net->cwr_window_tsn)) {
			
			net->cwr_window_tsn = cumack;
		}
		net->prev_cwnd = net->cwnd;
		net->net_ack = 0;
		net->net_ack2 = 0;

		



		net->new_pseudo_cumack = 0;
		net->will_exit_fast_recovery = 0;
		if (stcb->asoc.cc_functions.sctp_cwnd_prepare_net_for_sack) {
			(*stcb->asoc.cc_functions.sctp_cwnd_prepare_net_for_sack)(stcb, net);
		}
	}
	if (SCTP_BASE_SYSCTL(sctp_strict_sacks)) {
		uint32_t send_s;

		if (!TAILQ_EMPTY(&asoc->sent_queue)) {
			tp1 = TAILQ_LAST(&asoc->sent_queue,
					 sctpchunk_listhead);
			send_s = tp1->rec.data.TSN_seq + 1;
		} else {
			send_s = asoc->sending_seq;
		}
		if (SCTP_TSN_GE(cumack, send_s)) {
#ifndef INVARIANTS
			struct mbuf *oper;

#endif
#ifdef INVARIANTS
			panic("Impossible sack 1");
#else

			*abort_now = 1;
			
			oper = sctp_get_mbuf_for_msg((sizeof(struct sctp_paramhdr) + sizeof(uint32_t)),
						     0, M_NOWAIT, 1, MT_DATA);
			if (oper) {
				struct sctp_paramhdr *ph;
				uint32_t *ippp;

				SCTP_BUF_LEN(oper) = sizeof(struct sctp_paramhdr) +
					sizeof(uint32_t);
				ph = mtod(oper, struct sctp_paramhdr *);
				ph->param_type = htons(SCTP_CAUSE_PROTOCOL_VIOLATION);
				ph->param_length = htons(SCTP_BUF_LEN(oper));
				ippp = (uint32_t *) (ph + 1);
				*ippp = htonl(SCTP_FROM_SCTP_INDATA + SCTP_LOC_25);
			}
			stcb->sctp_ep->last_abort_code = SCTP_FROM_SCTP_INDATA + SCTP_LOC_25;
			sctp_abort_an_association(stcb->sctp_ep, stcb, oper, SCTP_SO_NOT_LOCKED);
			return;
#endif
		}
	}
	asoc->this_sack_highest_gap = cumack;
	if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_THRESHOLD_LOGGING) {
		sctp_misc_ints(SCTP_THRESHOLD_CLEAR,
			       stcb->asoc.overall_error_count,
			       0,
			       SCTP_FROM_SCTP_INDATA,
			       __LINE__);
	}
	stcb->asoc.overall_error_count = 0;
	if (SCTP_TSN_GT(cumack, asoc->last_acked_seq)) {
		
		TAILQ_FOREACH_SAFE(tp1, &asoc->sent_queue, sctp_next, tp2) {
			if (SCTP_TSN_GE(cumack, tp1->rec.data.TSN_seq)) {
				if (tp1->sent == SCTP_DATAGRAM_UNSENT) {
					SCTP_PRINTF("Warning, an unsent is now acked?\n");
				}
				if (tp1->sent < SCTP_DATAGRAM_ACKED) {
					




					if (tp1->sent < SCTP_DATAGRAM_RESEND) {
						if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_FLIGHT_LOGGING_ENABLE) {
							sctp_misc_ints(SCTP_FLIGHT_LOG_DOWN_CA,
								       tp1->whoTo->flight_size,
								       tp1->book_size,
								       (uintptr_t)tp1->whoTo,
								       tp1->rec.data.TSN_seq);
						}
						sctp_flight_size_decrease(tp1);
						if (stcb->asoc.cc_functions.sctp_cwnd_update_tsn_acknowledged) {
							(*stcb->asoc.cc_functions.sctp_cwnd_update_tsn_acknowledged)(tp1->whoTo,
														     tp1);
						}
						
						sctp_total_flight_decrease(stcb, tp1);
					}
					tp1->whoTo->net_ack += tp1->send_size;
					if (tp1->snd_count < 2) {
						



						tp1->whoTo->net_ack2 +=
							tp1->send_size;

						
						if (tp1->do_rtt) {
							if (rto_ok) {
								tp1->whoTo->RTO =
									



									sctp_calculate_rto(stcb,
											   asoc, tp1->whoTo,
											   &tp1->sent_rcv_time,
											   sctp_align_safe_nocopy,
											   SCTP_RTT_FROM_DATA);
								rto_ok = 0;
							}
							if (tp1->whoTo->rto_needed == 0) {
								tp1->whoTo->rto_needed = 1;
							}
							tp1->do_rtt = 0;
						}
					}
					











					tp1->whoTo->new_pseudo_cumack = 1;
					tp1->whoTo->find_pseudo_cumack = 1;
					tp1->whoTo->find_rtx_pseudo_cumack = 1;

					if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_CWND_LOGGING_ENABLE) {
						
						sctp_log_cwnd(stcb, tp1->whoTo, tp1->rec.data.TSN_seq, SCTP_CWND_LOG_FROM_SACK);
					}
				}
				if (tp1->sent == SCTP_DATAGRAM_RESEND) {
					sctp_ucount_decr(asoc->sent_queue_retran_cnt);
				}
				if (tp1->rec.data.chunk_was_revoked) {
					
					tp1->whoTo->cwnd -= tp1->book_size;
					tp1->rec.data.chunk_was_revoked = 0;
				}
				if (tp1->sent != SCTP_DATAGRAM_NR_ACKED) {
					if (asoc->strmout[tp1->rec.data.stream_number].chunks_on_queues > 0) {
						asoc->strmout[tp1->rec.data.stream_number].chunks_on_queues--;
#ifdef INVARIANTS
					} else {
						panic("No chunks on the queues for sid %u.", tp1->rec.data.stream_number);
#endif
					}
				}
				TAILQ_REMOVE(&asoc->sent_queue, tp1, sctp_next);
				if (tp1->data) {
					
					sctp_free_bufspace(stcb, asoc, tp1, 1);
					sctp_m_freem(tp1->data);
					tp1->data = NULL;
				}
				if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_SACK_LOGGING_ENABLE) {
					sctp_log_sack(asoc->last_acked_seq,
						      cumack,
						      tp1->rec.data.TSN_seq,
						      0,
						      0,
						      SCTP_LOG_FREE_SENT);
				}
				asoc->sent_queue_cnt--;
				sctp_free_a_chunk(stcb, tp1, SCTP_SO_NOT_LOCKED);
			} else {
				break;
			}
		}

	}
#if defined(__Userspace__)
	if (stcb->sctp_ep->recv_callback) {
		if (stcb->sctp_socket) {
			uint32_t inqueue_bytes, sb_free_now;
			struct sctp_inpcb *inp;

			inp = stcb->sctp_ep;
			inqueue_bytes = stcb->asoc.total_output_queue_size - (stcb->asoc.chunks_on_out_queue * sizeof(struct sctp_data_chunk));
			sb_free_now = SCTP_SB_LIMIT_SND(stcb->sctp_socket) - (inqueue_bytes + stcb->asoc.sb_send_resv);

			
			if (inp->send_callback &&
			    (((inp->send_sb_threshold > 0) &&
			      (sb_free_now >= inp->send_sb_threshold) &&
			      (stcb->asoc.chunks_on_out_queue <= SCTP_BASE_SYSCTL(sctp_max_chunks_on_queue))) ||
			     (inp->send_sb_threshold == 0))) {
				atomic_add_int(&stcb->asoc.refcnt, 1);
				SCTP_TCB_UNLOCK(stcb);
				inp->send_callback(stcb->sctp_socket, sb_free_now);
				SCTP_TCB_LOCK(stcb);
				atomic_subtract_int(&stcb->asoc.refcnt, 1);
			}
		}
	} else if (stcb->sctp_socket) {
#else

	if (stcb->sctp_socket) {
#endif
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
		struct socket *so;

#endif
		SOCKBUF_LOCK(&stcb->sctp_socket->so_snd);
		if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_WAKE_LOGGING_ENABLE) {
			
			sctp_wakeup_log(stcb, 1, SCTP_WAKESND_FROM_SACK);
		}
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
		sctp_sowwakeup_locked(stcb->sctp_ep, stcb->sctp_socket);
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
		SCTP_SOCKET_UNLOCK(so, 1);
#endif
	} else {
		if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_WAKE_LOGGING_ENABLE) {
			sctp_wakeup_log(stcb, 1, SCTP_NOWAKE_FROM_SACK);
		}
	}

	
	if ((asoc->last_acked_seq != cumack) && (ecne_seen == 0)) {
		TAILQ_FOREACH(net, &asoc->nets, sctp_next) {
			if (net->net_ack2 > 0) {
				



				net->error_count = 0;
				if (!(net->dest_state & SCTP_ADDR_REACHABLE)) {
					
					net->dest_state |= SCTP_ADDR_REACHABLE;
					sctp_ulp_notify(SCTP_NOTIFY_INTERFACE_UP, stcb,
					                0, (void *)net, SCTP_SO_NOT_LOCKED);
				}
				if (net == stcb->asoc.primary_destination) {
					if (stcb->asoc.alternate) {
						
						sctp_free_remote_addr(stcb->asoc.alternate);
						stcb->asoc.alternate = NULL;
					}
				}
				if (net->dest_state & SCTP_ADDR_PF) {
					net->dest_state &= ~SCTP_ADDR_PF;
					sctp_timer_stop(SCTP_TIMER_TYPE_HEARTBEAT, stcb->sctp_ep, stcb, net, SCTP_FROM_SCTP_INPUT + SCTP_LOC_3);
					sctp_timer_start(SCTP_TIMER_TYPE_HEARTBEAT, stcb->sctp_ep, stcb, net);
					asoc->cc_functions.sctp_cwnd_update_exit_pf(stcb, net);
					
					net->net_ack = 0;
				}
				
				net->RTO = (net->lastsa >> SCTP_RTT_SHIFT) + net->lastsv;
				if (net->RTO < stcb->asoc.minrto) {
					net->RTO = stcb->asoc.minrto;
				}
				if (net->RTO > stcb->asoc.maxrto) {
					net->RTO = stcb->asoc.maxrto;
				}
			}
		}
		asoc->cc_functions.sctp_cwnd_update_after_sack(stcb, asoc, 1, 0, 0);
	}
	asoc->last_acked_seq = cumack;

	if (TAILQ_EMPTY(&asoc->sent_queue)) {
		
		TAILQ_FOREACH(net, &asoc->nets, sctp_next) {
			net->flight_size = 0;
			net->partial_bytes_acked = 0;
		}
		asoc->total_flight = 0;
		asoc->total_flight_count = 0;
	}

	
	asoc->peers_rwnd = sctp_sbspace_sub(rwnd,
					    (uint32_t) (asoc->total_flight + (asoc->total_flight_count * SCTP_BASE_SYSCTL(sctp_peer_chunk_oh))));
	if (asoc->peers_rwnd < stcb->sctp_ep->sctp_ep.sctp_sws_sender) {
		
		asoc->peers_rwnd = 0;
	}
	if (asoc->peers_rwnd > old_rwnd) {
		win_probe_recovery = 1;
	}
	
again:
	j = 0;
	TAILQ_FOREACH(net, &asoc->nets, sctp_next) {
		int to_ticks;
		if (win_probe_recovery && (net->window_probe)) {
			win_probe_recovered = 1;
			



			
			TAILQ_FOREACH(tp1, &asoc->sent_queue, sctp_next) {
				if (tp1->window_probe) {
					
					sctp_window_probe_recovery(stcb, asoc, tp1);
					break;
				}
			}
		}
		if (net->RTO == 0) {
			to_ticks = MSEC_TO_TICKS(stcb->asoc.initial_rto);
		} else {
			to_ticks = MSEC_TO_TICKS(net->RTO);
		}
		if (net->flight_size) {
			j++;
			(void)SCTP_OS_TIMER_START(&net->rxt_timer.timer, to_ticks,
						  sctp_timeout_handler, &net->rxt_timer);
			if (net->window_probe) {
				net->window_probe = 0;
			}
		} else {
			if (net->window_probe) {
				
				net->window_probe = 0;
				if (!SCTP_OS_TIMER_PENDING(&net->rxt_timer.timer)) {
					SCTP_OS_TIMER_START(&net->rxt_timer.timer, to_ticks,
					                    sctp_timeout_handler, &net->rxt_timer);
				}
			} else if (SCTP_OS_TIMER_PENDING(&net->rxt_timer.timer)) {
				sctp_timer_stop(SCTP_TIMER_TYPE_SEND, stcb->sctp_ep,
				                stcb, net,
				                SCTP_FROM_SCTP_INDATA + SCTP_LOC_22);
			}
		}
	}
	if ((j == 0) &&
	    (!TAILQ_EMPTY(&asoc->sent_queue)) &&
	    (asoc->sent_queue_retran_cnt == 0) &&
	    (win_probe_recovered == 0) &&
	    (done_once == 0)) {
		


		if (sctp_fs_audit(asoc)) {
			TAILQ_FOREACH(net, &asoc->nets, sctp_next) {
				net->flight_size = 0;
			}
			asoc->total_flight = 0;
			asoc->total_flight_count = 0;
			asoc->sent_queue_retran_cnt = 0;
			TAILQ_FOREACH(tp1, &asoc->sent_queue, sctp_next) {
				if (tp1->sent < SCTP_DATAGRAM_RESEND) {
					sctp_flight_size_increase(tp1);
					sctp_total_flight_increase(stcb, tp1);
				} else if (tp1->sent == SCTP_DATAGRAM_RESEND) {
					sctp_ucount_incr(asoc->sent_queue_retran_cnt);
				}
			}
		}
		done_once = 1;
		goto again;
	}
	
	
	
	if (TAILQ_EMPTY(&asoc->send_queue) && TAILQ_EMPTY(&asoc->sent_queue)) {
		
		
		if ((asoc->stream_queue_cnt == 1) &&
		    ((asoc->state & SCTP_STATE_SHUTDOWN_PENDING) ||
		     (asoc->state & SCTP_STATE_SHUTDOWN_RECEIVED)) &&
		    (asoc->locked_on_sending)
			) {
			struct sctp_stream_queue_pending *sp;
			





			sp = TAILQ_LAST(&((asoc->locked_on_sending)->outqueue),
					sctp_streamhead);
			if ((sp) && (sp->length == 0)) {
				
				if (sp->msg_is_complete) {
					asoc->stream_queue_cnt--;
				} else {
					asoc->state |= SCTP_STATE_PARTIAL_MSG_LEFT;
					asoc->locked_on_sending = NULL;
					asoc->stream_queue_cnt--;
				}
			}
		}
		if ((asoc->state & SCTP_STATE_SHUTDOWN_PENDING) &&
		    (asoc->stream_queue_cnt == 0)) {
			if (asoc->state & SCTP_STATE_PARTIAL_MSG_LEFT) {
				
				struct mbuf *oper;

			abort_out_now:
				*abort_now = 1;
				
				oper = sctp_get_mbuf_for_msg(sizeof(struct sctp_paramhdr),
							     0, M_NOWAIT, 1, MT_DATA);
				if (oper) {
					struct sctp_paramhdr *ph;

					SCTP_BUF_LEN(oper) = sizeof(struct sctp_paramhdr);
					ph = mtod(oper, struct sctp_paramhdr *);
					ph->param_type = htons(SCTP_CAUSE_USER_INITIATED_ABT);
					ph->param_length = htons(SCTP_BUF_LEN(oper));
				}
				stcb->sctp_ep->last_abort_code = SCTP_FROM_SCTP_INDATA + SCTP_LOC_24;
				sctp_abort_an_association(stcb->sctp_ep, stcb, oper, SCTP_SO_NOT_LOCKED);
			} else {
				struct sctp_nets *netp;

				if ((SCTP_GET_STATE(asoc) == SCTP_STATE_OPEN) ||
				    (SCTP_GET_STATE(asoc) == SCTP_STATE_SHUTDOWN_RECEIVED)) {
					SCTP_STAT_DECR_GAUGE32(sctps_currestab);
				}
				SCTP_SET_STATE(asoc, SCTP_STATE_SHUTDOWN_SENT);
				SCTP_CLEAR_SUBSTATE(asoc, SCTP_STATE_SHUTDOWN_PENDING);
				sctp_stop_timers_for_shutdown(stcb);
				if (asoc->alternate) {
					netp = asoc->alternate;
				} else {
					netp = asoc->primary_destination;
				}
				sctp_send_shutdown(stcb, netp);
				sctp_timer_start(SCTP_TIMER_TYPE_SHUTDOWN,
						 stcb->sctp_ep, stcb, netp);
				sctp_timer_start(SCTP_TIMER_TYPE_SHUTDOWNGUARD,
						 stcb->sctp_ep, stcb, netp);
			}
		} else if ((SCTP_GET_STATE(asoc) == SCTP_STATE_SHUTDOWN_RECEIVED) &&
			   (asoc->stream_queue_cnt == 0)) {
			struct sctp_nets *netp;

			if (asoc->state & SCTP_STATE_PARTIAL_MSG_LEFT) {
				goto abort_out_now;
			}
			SCTP_STAT_DECR_GAUGE32(sctps_currestab);
			SCTP_SET_STATE(asoc, SCTP_STATE_SHUTDOWN_ACK_SENT);
			SCTP_CLEAR_SUBSTATE(asoc, SCTP_STATE_SHUTDOWN_PENDING);
			sctp_stop_timers_for_shutdown(stcb);
			if (asoc->alternate) {
				netp = asoc->alternate;
			} else {
				netp = asoc->primary_destination;
			}
			sctp_send_shutdown_ack(stcb, netp);
			sctp_timer_start(SCTP_TIMER_TYPE_SHUTDOWNACK,
					 stcb->sctp_ep, stcb, netp);
		}
	}
	
	
	
	
	
	if (SCTP_TSN_GT(cumack, asoc->advanced_peer_ack_point)) {
		asoc->advanced_peer_ack_point = cumack;
	}
	
	if ((asoc->peer_supports_prsctp) && (asoc->pr_sctp_cnt > 0)) {
		struct sctp_tmit_chunk *lchk;
		uint32_t old_adv_peer_ack_point;

		old_adv_peer_ack_point = asoc->advanced_peer_ack_point;
		lchk = sctp_try_advance_peer_ack_point(stcb, asoc);
		
		if (SCTP_TSN_GT(asoc->advanced_peer_ack_point, cumack)) {
			


			if (SCTP_TSN_GT(asoc->advanced_peer_ack_point, old_adv_peer_ack_point)) {
				send_forward_tsn(stcb, asoc);
			} else if (lchk) {
				
				if (lchk->rec.data.fwd_tsn_cnt >= 3) {
					send_forward_tsn(stcb, asoc);
				}
			}
		}
		if (lchk) {
			
			sctp_timer_start(SCTP_TIMER_TYPE_SEND,
					 stcb->sctp_ep, stcb, lchk->whoTo);
		}
	}
	if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_SACK_RWND_LOGGING_ENABLE) {
		sctp_misc_ints(SCTP_SACK_RWND_UPDATE,
			       rwnd,
			       stcb->asoc.peers_rwnd,
			       stcb->asoc.total_flight,
			       stcb->asoc.total_output_queue_size);
	}
}

void
sctp_handle_sack(struct mbuf *m, int offset_seg, int offset_dup,
                 struct sctp_tcb *stcb,
                 uint16_t num_seg, uint16_t num_nr_seg, uint16_t num_dup,
                 int *abort_now, uint8_t flags,
                 uint32_t cum_ack, uint32_t rwnd, int ecne_seen)
{
	struct sctp_association *asoc;
	struct sctp_tmit_chunk *tp1, *tp2;
	uint32_t last_tsn, biggest_tsn_acked, biggest_tsn_newly_acked, this_sack_lowest_newack;
	uint16_t wake_him = 0;
	uint32_t send_s = 0;
	long j;
	int accum_moved = 0;
	int will_exit_fast_recovery = 0;
	uint32_t a_rwnd, old_rwnd;
	int win_probe_recovery = 0;
	int win_probe_recovered = 0;
	struct sctp_nets *net = NULL;
	int done_once;
	int rto_ok = 1;
	uint8_t reneged_all = 0;
	uint8_t cmt_dac_flag;
	



	
















	SCTP_TCB_LOCK_ASSERT(stcb);
	
	this_sack_lowest_newack = 0;
	SCTP_STAT_INCR(sctps_slowpath_sack);
	last_tsn = cum_ack;
	cmt_dac_flag = flags & SCTP_SACK_CMT_DAC;
#ifdef SCTP_ASOCLOG_OF_TSNS
	stcb->asoc.cumack_log[stcb->asoc.cumack_log_at] = cum_ack;
	stcb->asoc.cumack_log_at++;
	if (stcb->asoc.cumack_log_at > SCTP_TSN_LOG_SIZE) {
		stcb->asoc.cumack_log_at = 0;
	}
#endif
	a_rwnd = rwnd;

	if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_LOG_SACK_ARRIVALS_ENABLE) {
		sctp_misc_ints(SCTP_SACK_LOG_NORMAL, cum_ack,
		               rwnd, stcb->asoc.last_acked_seq, stcb->asoc.peers_rwnd);
	}

	old_rwnd = stcb->asoc.peers_rwnd;
	if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_THRESHOLD_LOGGING) {
		sctp_misc_ints(SCTP_THRESHOLD_CLEAR,
		               stcb->asoc.overall_error_count,
		               0,
		               SCTP_FROM_SCTP_INDATA,
		               __LINE__);
	}
	stcb->asoc.overall_error_count = 0;
	asoc = &stcb->asoc;
	if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_SACK_LOGGING_ENABLE) {
		sctp_log_sack(asoc->last_acked_seq,
		              cum_ack,
		              0,
		              num_seg,
		              num_dup,
		              SCTP_LOG_NEW_SACK);
	}
	if ((num_dup) && (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_FR_LOGGING_ENABLE)) {
		uint16_t i;
		uint32_t *dupdata, dblock;

		for (i = 0; i < num_dup; i++) {
			dupdata = (uint32_t *)sctp_m_getptr(m, offset_dup + i * sizeof(uint32_t),
			                                    sizeof(uint32_t), (uint8_t *)&dblock);
			if (dupdata == NULL) {
				break;
			}
			sctp_log_fr(*dupdata, 0, 0, SCTP_FR_DUPED);
		}
	}
	if (SCTP_BASE_SYSCTL(sctp_strict_sacks)) {
		
		if (!TAILQ_EMPTY(&asoc->sent_queue)) {
			tp1 = TAILQ_LAST(&asoc->sent_queue,
			                 sctpchunk_listhead);
			send_s = tp1->rec.data.TSN_seq + 1;
		} else {
			tp1 = NULL;
			send_s = asoc->sending_seq;
		}
		if (SCTP_TSN_GE(cum_ack, send_s)) {
			struct mbuf *oper;
			



			SCTP_PRINTF("NEW cum_ack:%x send_s:%x is smaller or equal\n",
			            cum_ack, send_s);
			if (tp1) {
				SCTP_PRINTF("Got send_s from tsn:%x + 1 of tp1:%p\n",
				            tp1->rec.data.TSN_seq, (void *)tp1);
			}
		hopeless_peer:
			*abort_now = 1;
			
			oper = sctp_get_mbuf_for_msg((sizeof(struct sctp_paramhdr) + sizeof(uint32_t)),
			                             0, M_NOWAIT, 1, MT_DATA);
			if (oper) {
				struct sctp_paramhdr *ph;
				uint32_t *ippp;

				SCTP_BUF_LEN(oper) = sizeof(struct sctp_paramhdr) +
					sizeof(uint32_t);
				ph = mtod(oper, struct sctp_paramhdr *);
				ph->param_type = htons(SCTP_CAUSE_PROTOCOL_VIOLATION);
				ph->param_length = htons(SCTP_BUF_LEN(oper));
				ippp = (uint32_t *) (ph + 1);
				*ippp = htonl(SCTP_FROM_SCTP_INDATA + SCTP_LOC_25);
			}
			stcb->sctp_ep->last_abort_code = SCTP_FROM_SCTP_INDATA + SCTP_LOC_25;
			sctp_abort_an_association(stcb->sctp_ep, stcb, oper, SCTP_SO_NOT_LOCKED);
			return;
		}
	}
	
	
	
	if (SCTP_TSN_GT(asoc->last_acked_seq, last_tsn)) {
		
		return;
	}

	
	if (TAILQ_EMPTY(&asoc->sent_queue) &&
	    TAILQ_EMPTY(&asoc->send_queue) &&
	    (asoc->stream_queue_cnt == 0)) {
		
		if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_LOG_RWND_ENABLE) {
			sctp_log_rwnd_set(SCTP_SET_PEER_RWND_VIA_SACK,
			                  asoc->peers_rwnd, 0, 0, a_rwnd);
		}
		asoc->peers_rwnd = a_rwnd;
		if (asoc->sent_queue_retran_cnt) {
			asoc->sent_queue_retran_cnt = 0;
		}
		if (asoc->peers_rwnd < stcb->sctp_ep->sctp_ep.sctp_sws_sender) {
			
			asoc->peers_rwnd = 0;
		}
		
		TAILQ_FOREACH(net, &asoc->nets, sctp_next) {
			sctp_timer_stop(SCTP_TIMER_TYPE_SEND, stcb->sctp_ep,
			                stcb, net, SCTP_FROM_SCTP_INDATA + SCTP_LOC_26);
			net->partial_bytes_acked = 0;
			net->flight_size = 0;
		}
		asoc->total_flight = 0;
		asoc->total_flight_count = 0;
		return;
	}
	






	TAILQ_FOREACH(net, &asoc->nets, sctp_next) {
		if (SCTP_TSN_GT(cum_ack, net->cwr_window_tsn)) {
			
			net->cwr_window_tsn = cum_ack;
		}
		net->prev_cwnd = net->cwnd;
		net->net_ack = 0;
		net->net_ack2 = 0;

		



		net->new_pseudo_cumack = 0;
		net->will_exit_fast_recovery = 0;
		if (stcb->asoc.cc_functions.sctp_cwnd_prepare_net_for_sack) {
			(*stcb->asoc.cc_functions.sctp_cwnd_prepare_net_for_sack)(stcb, net);
		}
	}
	
	TAILQ_FOREACH(tp1, &asoc->sent_queue, sctp_next) {
		if (SCTP_TSN_GE(last_tsn, tp1->rec.data.TSN_seq)) {
			if (tp1->sent != SCTP_DATAGRAM_UNSENT) {
				accum_moved = 1;
				if (tp1->sent < SCTP_DATAGRAM_ACKED) {
					




					if ((tp1->whoTo->dest_state &
					     SCTP_ADDR_UNCONFIRMED) &&
					    (tp1->snd_count < 2)) {
						







						tp1->whoTo->dest_state &=
							~SCTP_ADDR_UNCONFIRMED;
					}
					if (tp1->sent < SCTP_DATAGRAM_RESEND) {
						if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_FLIGHT_LOGGING_ENABLE) {
							sctp_misc_ints(SCTP_FLIGHT_LOG_DOWN_CA,
							               tp1->whoTo->flight_size,
							               tp1->book_size,
							               (uintptr_t)tp1->whoTo,
							               tp1->rec.data.TSN_seq);
						}
						sctp_flight_size_decrease(tp1);
						sctp_total_flight_decrease(stcb, tp1);
						if (stcb->asoc.cc_functions.sctp_cwnd_update_tsn_acknowledged) {
							(*stcb->asoc.cc_functions.sctp_cwnd_update_tsn_acknowledged)(tp1->whoTo,
														     tp1);
						}
					}
					tp1->whoTo->net_ack += tp1->send_size;

					
					this_sack_lowest_newack = tp1->rec.data.TSN_seq;
					tp1->whoTo->saw_newack = 1;

					if (tp1->snd_count < 2) {
						



						tp1->whoTo->net_ack2 +=
							tp1->send_size;

						
						if (tp1->do_rtt) {
							if (rto_ok) {
								tp1->whoTo->RTO =
									sctp_calculate_rto(stcb,
											   asoc, tp1->whoTo,
											   &tp1->sent_rcv_time,
											   sctp_align_safe_nocopy,
											   SCTP_RTT_FROM_DATA);
								rto_ok = 0;
							}
							if (tp1->whoTo->rto_needed == 0) {
								tp1->whoTo->rto_needed = 1;
							}
							tp1->do_rtt = 0;
						}
					}
					











					tp1->whoTo->new_pseudo_cumack = 1;
					tp1->whoTo->find_pseudo_cumack = 1;
					tp1->whoTo->find_rtx_pseudo_cumack = 1;


					if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_SACK_LOGGING_ENABLE) {
						sctp_log_sack(asoc->last_acked_seq,
						              cum_ack,
						              tp1->rec.data.TSN_seq,
						              0,
						              0,
						              SCTP_LOG_TSN_ACKED);
					}
					if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_CWND_LOGGING_ENABLE) {
						sctp_log_cwnd(stcb, tp1->whoTo, tp1->rec.data.TSN_seq, SCTP_CWND_LOG_FROM_SACK);
					}
				}
				if (tp1->sent == SCTP_DATAGRAM_RESEND) {
					sctp_ucount_decr(asoc->sent_queue_retran_cnt);
#ifdef SCTP_AUDITING_ENABLED
					sctp_audit_log(0xB3,
					               (asoc->sent_queue_retran_cnt & 0x000000ff));
#endif
				}
				if (tp1->rec.data.chunk_was_revoked) {
					
					tp1->whoTo->cwnd -= tp1->book_size;
					tp1->rec.data.chunk_was_revoked = 0;
				}
				if (tp1->sent != SCTP_DATAGRAM_NR_ACKED) {
					tp1->sent = SCTP_DATAGRAM_ACKED;
				}
			}
		} else {
			break;
		}
	}
	biggest_tsn_newly_acked = biggest_tsn_acked = last_tsn;
	
	asoc->this_sack_highest_gap = last_tsn;

	if ((num_seg > 0) || (num_nr_seg > 0)) {

		




		TAILQ_FOREACH(net, &asoc->nets, sctp_next) {
			net->saw_newack = 0;
			net->this_sack_highest_newack = last_tsn;
		}

		





		if (sctp_handle_segments(m, &offset_seg, stcb, asoc, last_tsn, &biggest_tsn_acked,
			&biggest_tsn_newly_acked, &this_sack_lowest_newack,
			num_seg, num_nr_seg, &rto_ok)) {
			wake_him++;
		}
		if (SCTP_BASE_SYSCTL(sctp_strict_sacks)) {
			



			if (SCTP_TSN_GE(biggest_tsn_acked, send_s)) {
				



				SCTP_PRINTF("Hopeless peer! biggest_tsn_acked:%x largest seq:%x\n",
				            biggest_tsn_acked, send_s);
				goto hopeless_peer;
			}
		}
	}
	
	
	
	if (asoc->sctp_cmt_on_off > 0) {
		TAILQ_FOREACH(net, &asoc->nets, sctp_next) {
			if (net->new_pseudo_cumack)
				sctp_timer_stop(SCTP_TIMER_TYPE_SEND, stcb->sctp_ep,
				                stcb, net,
				                SCTP_FROM_SCTP_INDATA + SCTP_LOC_27);

		}
	} else {
		if (accum_moved) {
			TAILQ_FOREACH(net, &asoc->nets, sctp_next) {
				sctp_timer_stop(SCTP_TIMER_TYPE_SEND, stcb->sctp_ep,
				                stcb, net, SCTP_FROM_SCTP_INDATA + SCTP_LOC_28);
			}
		}
	}
	
	
	
	asoc->last_acked_seq = cum_ack;

	TAILQ_FOREACH_SAFE(tp1, &asoc->sent_queue, sctp_next, tp2) {
		if (SCTP_TSN_GT(tp1->rec.data.TSN_seq, cum_ack)) {
			break;
		}
		if (tp1->sent != SCTP_DATAGRAM_NR_ACKED) {
			if (asoc->strmout[tp1->rec.data.stream_number].chunks_on_queues > 0) {
				asoc->strmout[tp1->rec.data.stream_number].chunks_on_queues--;
#ifdef INVARIANTS
			} else {
				panic("No chunks on the queues for sid %u.", tp1->rec.data.stream_number);
#endif
			}
		}
		TAILQ_REMOVE(&asoc->sent_queue, tp1, sctp_next);
		if (tp1->pr_sctp_on) {
			if (asoc->pr_sctp_cnt != 0)
				asoc->pr_sctp_cnt--;
		}
		asoc->sent_queue_cnt--;
		if (tp1->data) {
			
			sctp_free_bufspace(stcb, asoc, tp1, 1);
			sctp_m_freem(tp1->data);
			tp1->data = NULL;
			if (asoc->peer_supports_prsctp && PR_SCTP_BUF_ENABLED(tp1->flags)) {
				asoc->sent_queue_cnt_removeable--;
			}
		}
		if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_SACK_LOGGING_ENABLE) {
			sctp_log_sack(asoc->last_acked_seq,
			              cum_ack,
			              tp1->rec.data.TSN_seq,
			              0,
			              0,
			              SCTP_LOG_FREE_SENT);
		}
		sctp_free_a_chunk(stcb, tp1, SCTP_SO_NOT_LOCKED);
		wake_him++;
	}
	if (TAILQ_EMPTY(&asoc->sent_queue) && (asoc->total_flight > 0)) {
#ifdef INVARIANTS
		panic("Warning flight size is postive and should be 0");
#else
		SCTP_PRINTF("Warning flight size incorrect should be 0 is %d\n",
		            asoc->total_flight);
#endif
		asoc->total_flight = 0;
	}

#if defined(__Userspace__)
	if (stcb->sctp_ep->recv_callback) {
		if (stcb->sctp_socket) {
			uint32_t inqueue_bytes, sb_free_now;
			struct sctp_inpcb *inp;

			inp = stcb->sctp_ep;
			inqueue_bytes = stcb->asoc.total_output_queue_size - (stcb->asoc.chunks_on_out_queue * sizeof(struct sctp_data_chunk));
			sb_free_now = SCTP_SB_LIMIT_SND(stcb->sctp_socket) - (inqueue_bytes + stcb->asoc.sb_send_resv);

			
			if (inp->send_callback &&
			   (((inp->send_sb_threshold > 0) && (sb_free_now >= inp->send_sb_threshold)) ||
			    (inp->send_sb_threshold == 0))) {
				atomic_add_int(&stcb->asoc.refcnt, 1);
				SCTP_TCB_UNLOCK(stcb);
				inp->send_callback(stcb->sctp_socket, sb_free_now);
				SCTP_TCB_LOCK(stcb);
				atomic_subtract_int(&stcb->asoc.refcnt, 1);
			}
		}
	} else if ((wake_him) && (stcb->sctp_socket)) {
#else

	if ((wake_him) && (stcb->sctp_socket)) {
#endif
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
		struct socket *so;

#endif
		SOCKBUF_LOCK(&stcb->sctp_socket->so_snd);
		if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_WAKE_LOGGING_ENABLE) {
			sctp_wakeup_log(stcb, wake_him, SCTP_WAKESND_FROM_SACK);
		}
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
		sctp_sowwakeup_locked(stcb->sctp_ep, stcb->sctp_socket);
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
		SCTP_SOCKET_UNLOCK(so, 1);
#endif
	} else {
		if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_WAKE_LOGGING_ENABLE) {
			sctp_wakeup_log(stcb, wake_him, SCTP_NOWAKE_FROM_SACK);
		}
	}

	if (asoc->fast_retran_loss_recovery && accum_moved) {
		if (SCTP_TSN_GE(asoc->last_acked_seq, asoc->fast_recovery_tsn)) {
			
			will_exit_fast_recovery = 1;
		}
	}
	









	if (num_seg) {
		sctp_check_for_revoked(stcb, asoc, cum_ack, biggest_tsn_acked);
		asoc->saw_sack_with_frags = 1;
	} else if (asoc->saw_sack_with_frags) {
		int cnt_revoked = 0;

		
		TAILQ_FOREACH(tp1, &asoc->sent_queue, sctp_next) {
			if (tp1->sent == SCTP_DATAGRAM_ACKED) {
				tp1->sent = SCTP_DATAGRAM_SENT;
				if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_FLIGHT_LOGGING_ENABLE) {
					sctp_misc_ints(SCTP_FLIGHT_LOG_UP_REVOKE,
					               tp1->whoTo->flight_size,
					               tp1->book_size,
					               (uintptr_t)tp1->whoTo,
					               tp1->rec.data.TSN_seq);
				}
				sctp_flight_size_increase(tp1);
				sctp_total_flight_increase(stcb, tp1);
				tp1->rec.data.chunk_was_revoked = 1;
				






				tp1->whoTo->cwnd += tp1->book_size;
				cnt_revoked++;
			}
		}
		if (cnt_revoked) {
			reneged_all = 1;
		}
		asoc->saw_sack_with_frags = 0;
	}
	if (num_nr_seg > 0)
		asoc->saw_sack_with_nr_frags = 1;
	else
		asoc->saw_sack_with_nr_frags = 0;

	
	if (ecne_seen == 0) {
		TAILQ_FOREACH(net, &asoc->nets, sctp_next) {
			if (net->net_ack2 > 0) {
				



				net->error_count = 0;
				if (!(net->dest_state & SCTP_ADDR_REACHABLE)) {
					
					net->dest_state |= SCTP_ADDR_REACHABLE;
					sctp_ulp_notify(SCTP_NOTIFY_INTERFACE_UP, stcb,
					                0, (void *)net, SCTP_SO_NOT_LOCKED);
				}

				if (net == stcb->asoc.primary_destination) {
					if (stcb->asoc.alternate) {
						
						sctp_free_remote_addr(stcb->asoc.alternate);
						stcb->asoc.alternate = NULL;
					}
				}

				if (net->dest_state & SCTP_ADDR_PF) {
					net->dest_state &= ~SCTP_ADDR_PF;
					sctp_timer_stop(SCTP_TIMER_TYPE_HEARTBEAT, stcb->sctp_ep, stcb, net, SCTP_FROM_SCTP_INPUT + SCTP_LOC_3);
					sctp_timer_start(SCTP_TIMER_TYPE_HEARTBEAT, stcb->sctp_ep, stcb, net);
					asoc->cc_functions.sctp_cwnd_update_exit_pf(stcb, net);
					
					net->net_ack = 0;
				}
				
				net->RTO = (net->lastsa >> SCTP_RTT_SHIFT) + net->lastsv;
				if (net->RTO < stcb->asoc.minrto) {
					net->RTO = stcb->asoc.minrto;
				}
				if (net->RTO > stcb->asoc.maxrto) {
					net->RTO = stcb->asoc.maxrto;
				}
			}
		}
		asoc->cc_functions.sctp_cwnd_update_after_sack(stcb, asoc, accum_moved, reneged_all, will_exit_fast_recovery);
	}

	if (TAILQ_EMPTY(&asoc->sent_queue)) {
		
		TAILQ_FOREACH(net, &asoc->nets, sctp_next) {
			
			sctp_timer_stop(SCTP_TIMER_TYPE_SEND, stcb->sctp_ep,
			                stcb, net, SCTP_FROM_SCTP_INDATA + SCTP_LOC_30);
			net->flight_size = 0;
			net->partial_bytes_acked = 0;
		}
		asoc->total_flight = 0;
		asoc->total_flight_count = 0;
	}

	
	
	
	if (TAILQ_EMPTY(&asoc->send_queue) && TAILQ_EMPTY(&asoc->sent_queue)) {
		
		if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_LOG_RWND_ENABLE) {
			sctp_log_rwnd_set(SCTP_SET_PEER_RWND_VIA_SACK,
			                  asoc->peers_rwnd, 0, 0, a_rwnd);
		}
		asoc->peers_rwnd = a_rwnd;
		if (asoc->peers_rwnd < stcb->sctp_ep->sctp_ep.sctp_sws_sender) {
			
			asoc->peers_rwnd = 0;
		}
		
		if ((asoc->stream_queue_cnt == 1) &&
		    ((asoc->state & SCTP_STATE_SHUTDOWN_PENDING) ||
		     (asoc->state & SCTP_STATE_SHUTDOWN_RECEIVED)) &&
		    (asoc->locked_on_sending)
			) {
			struct sctp_stream_queue_pending *sp;
			




			sp = TAILQ_LAST(&((asoc->locked_on_sending)->outqueue),
			                sctp_streamhead);
			if ((sp) && (sp->length == 0)) {
				asoc->locked_on_sending = NULL;
				if (sp->msg_is_complete) {
					asoc->stream_queue_cnt--;
				} else {
					asoc->state |= SCTP_STATE_PARTIAL_MSG_LEFT;
					asoc->stream_queue_cnt--;
				}
			}
		}
		if ((asoc->state & SCTP_STATE_SHUTDOWN_PENDING) &&
		    (asoc->stream_queue_cnt == 0)) {
			if (asoc->state & SCTP_STATE_PARTIAL_MSG_LEFT) {
				
				struct mbuf *oper;
			abort_out_now:
				*abort_now = 1;
				
				oper = sctp_get_mbuf_for_msg(sizeof(struct sctp_paramhdr),
				                             0, M_NOWAIT, 1, MT_DATA);
				if (oper) {
					struct sctp_paramhdr *ph;

					SCTP_BUF_LEN(oper) = sizeof(struct sctp_paramhdr);
					ph = mtod(oper, struct sctp_paramhdr *);
					ph->param_type = htons(SCTP_CAUSE_USER_INITIATED_ABT);
					ph->param_length = htons(SCTP_BUF_LEN(oper));
				}
				stcb->sctp_ep->last_abort_code = SCTP_FROM_SCTP_INDATA + SCTP_LOC_31;
				sctp_abort_an_association(stcb->sctp_ep, stcb, oper, SCTP_SO_NOT_LOCKED);
				return;
			} else {
				struct sctp_nets *netp;

				if ((SCTP_GET_STATE(asoc) == SCTP_STATE_OPEN) ||
				    (SCTP_GET_STATE(asoc) == SCTP_STATE_SHUTDOWN_RECEIVED)) {
					SCTP_STAT_DECR_GAUGE32(sctps_currestab);
				}
				SCTP_SET_STATE(asoc, SCTP_STATE_SHUTDOWN_SENT);
				SCTP_CLEAR_SUBSTATE(asoc, SCTP_STATE_SHUTDOWN_PENDING);
				sctp_stop_timers_for_shutdown(stcb);
				if (asoc->alternate) {
					netp = asoc->alternate;
				} else {
					netp = asoc->primary_destination;
				}
				sctp_send_shutdown(stcb, netp);
				sctp_timer_start(SCTP_TIMER_TYPE_SHUTDOWN,
				                 stcb->sctp_ep, stcb, netp);
				sctp_timer_start(SCTP_TIMER_TYPE_SHUTDOWNGUARD,
				                 stcb->sctp_ep, stcb, netp);
			}
			return;
		} else if ((SCTP_GET_STATE(asoc) == SCTP_STATE_SHUTDOWN_RECEIVED) &&
			   (asoc->stream_queue_cnt == 0)) {
			struct sctp_nets *netp;

			if (asoc->state & SCTP_STATE_PARTIAL_MSG_LEFT) {
				goto abort_out_now;
			}
			SCTP_STAT_DECR_GAUGE32(sctps_currestab);
			SCTP_SET_STATE(asoc, SCTP_STATE_SHUTDOWN_ACK_SENT);
			SCTP_CLEAR_SUBSTATE(asoc, SCTP_STATE_SHUTDOWN_PENDING);
			sctp_stop_timers_for_shutdown(stcb);
			if (asoc->alternate) {
				netp = asoc->alternate;
			} else {
				netp = asoc->primary_destination;
			}
			sctp_send_shutdown_ack(stcb, netp);
			sctp_timer_start(SCTP_TIMER_TYPE_SHUTDOWNACK,
			                 stcb->sctp_ep, stcb, netp);
			return;
		}
	}
	



	TAILQ_FOREACH(net, &asoc->nets, sctp_next) {
		net->net_ack = 0;
	}

	




	if ((asoc->sctp_cmt_on_off > 0) &&
	    SCTP_BASE_SYSCTL(sctp_cmt_use_dac) &&
	    (cmt_dac_flag == 0)) {
		this_sack_lowest_newack = cum_ack;
	}
	if ((num_seg > 0) || (num_nr_seg > 0)) {
		sctp_strike_gap_ack_chunks(stcb, asoc, biggest_tsn_acked,
		                           biggest_tsn_newly_acked, this_sack_lowest_newack, accum_moved);
	}
	
	asoc->cc_functions.sctp_cwnd_update_after_fr(stcb, asoc);

	
	if (will_exit_fast_recovery) {
		
		asoc->fast_retran_loss_recovery = 0;
	}
	if ((asoc->sat_t3_loss_recovery) &&
	    SCTP_TSN_GE(asoc->last_acked_seq, asoc->sat_t3_recovery_tsn)) {
		
		asoc->sat_t3_loss_recovery = 0;
	}
	


	TAILQ_FOREACH(net, &asoc->nets, sctp_next) {
		if (net->will_exit_fast_recovery) {
			
			net->fast_retran_loss_recovery = 0;
		}
	}

	
	if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_LOG_RWND_ENABLE) {
		sctp_log_rwnd_set(SCTP_SET_PEER_RWND_VIA_SACK,
		                  asoc->peers_rwnd, asoc->total_flight, (asoc->total_flight_count * SCTP_BASE_SYSCTL(sctp_peer_chunk_oh)), a_rwnd);
	}
	asoc->peers_rwnd = sctp_sbspace_sub(a_rwnd,
	                                    (uint32_t) (asoc->total_flight + (asoc->total_flight_count * SCTP_BASE_SYSCTL(sctp_peer_chunk_oh))));
	if (asoc->peers_rwnd < stcb->sctp_ep->sctp_ep.sctp_sws_sender) {
		
		asoc->peers_rwnd = 0;
	}
	if (asoc->peers_rwnd > old_rwnd) {
		win_probe_recovery = 1;
	}

	



	done_once = 0;
again:
	j = 0;
	TAILQ_FOREACH(net, &asoc->nets, sctp_next) {
		if (win_probe_recovery && (net->window_probe)) {
			win_probe_recovered = 1;
			





			TAILQ_FOREACH(tp1, &asoc->sent_queue, sctp_next) {
				if (tp1->window_probe) {
					sctp_window_probe_recovery(stcb, asoc, tp1);
					break;
				}
			}
		}
		if (net->flight_size) {
			j++;
			if (!SCTP_OS_TIMER_PENDING(&net->rxt_timer.timer)) {
				sctp_timer_start(SCTP_TIMER_TYPE_SEND,
				                 stcb->sctp_ep, stcb, net);
			}
			if (net->window_probe) {
				net->window_probe = 0;
			}
		} else {
			if (net->window_probe) {
				
				if (!SCTP_OS_TIMER_PENDING(&net->rxt_timer.timer)) {
					sctp_timer_start(SCTP_TIMER_TYPE_SEND,
					                 stcb->sctp_ep, stcb, net);

				}
			} else if (SCTP_OS_TIMER_PENDING(&net->rxt_timer.timer)) {
				sctp_timer_stop(SCTP_TIMER_TYPE_SEND, stcb->sctp_ep,
				                stcb, net,
				                SCTP_FROM_SCTP_INDATA + SCTP_LOC_22);
			}
		}
	}
	if ((j == 0) &&
	    (!TAILQ_EMPTY(&asoc->sent_queue)) &&
	    (asoc->sent_queue_retran_cnt == 0) &&
	    (win_probe_recovered == 0) &&
	    (done_once == 0)) {
		


		if (sctp_fs_audit(asoc)) {
			TAILQ_FOREACH(net, &asoc->nets, sctp_next) {
				net->flight_size = 0;
			}
			asoc->total_flight = 0;
			asoc->total_flight_count = 0;
			asoc->sent_queue_retran_cnt = 0;
			TAILQ_FOREACH(tp1, &asoc->sent_queue, sctp_next) {
				if (tp1->sent < SCTP_DATAGRAM_RESEND) {
					sctp_flight_size_increase(tp1);
					sctp_total_flight_increase(stcb, tp1);
				} else if (tp1->sent == SCTP_DATAGRAM_RESEND) {
					sctp_ucount_incr(asoc->sent_queue_retran_cnt);
				}
			}
		}
		done_once = 1;
		goto again;
	}
	
	
	
	
	
	if (SCTP_TSN_GT(cum_ack, asoc->advanced_peer_ack_point)) {
		asoc->advanced_peer_ack_point = cum_ack;
	}
	
	if ((asoc->peer_supports_prsctp) && (asoc->pr_sctp_cnt > 0)) {
		struct sctp_tmit_chunk *lchk;
		uint32_t old_adv_peer_ack_point;

		old_adv_peer_ack_point = asoc->advanced_peer_ack_point;
		lchk = sctp_try_advance_peer_ack_point(stcb, asoc);
		
		if (SCTP_TSN_GT(asoc->advanced_peer_ack_point, cum_ack)) {
			


			if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_LOG_TRY_ADVANCE) {
				sctp_misc_ints(SCTP_FWD_TSN_CHECK,
				               0xee, cum_ack, asoc->advanced_peer_ack_point,
				               old_adv_peer_ack_point);
			}
			if (SCTP_TSN_GT(asoc->advanced_peer_ack_point, old_adv_peer_ack_point)) {
				send_forward_tsn(stcb, asoc);
			} else if (lchk) {
				
				if (lchk->rec.data.fwd_tsn_cnt >= 3) {
					send_forward_tsn(stcb, asoc);
				}
			}
		}
		if (lchk) {
			
			sctp_timer_start(SCTP_TIMER_TYPE_SEND,
			                 stcb->sctp_ep, stcb, lchk->whoTo);
		}
	}
	if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_SACK_RWND_LOGGING_ENABLE) {
		sctp_misc_ints(SCTP_SACK_RWND_UPDATE,
		               a_rwnd,
		               stcb->asoc.peers_rwnd,
		               stcb->asoc.total_flight,
		               stcb->asoc.total_output_queue_size);
	}
}

void
sctp_update_acked(struct sctp_tcb *stcb, struct sctp_shutdown_chunk *cp, int *abort_flag)
{
	
	uint32_t cum_ack, a_rwnd;

	cum_ack = ntohl(cp->cumulative_tsn_ack);
	
	a_rwnd = stcb->asoc.peers_rwnd + stcb->asoc.total_flight;

	
	sctp_express_handle_sack(stcb, cum_ack, a_rwnd, abort_flag, 0);
}

static void
sctp_kick_prsctp_reorder_queue(struct sctp_tcb *stcb,
    struct sctp_stream_in *strmin)
{
	struct sctp_queued_to_read *ctl, *nctl;
	struct sctp_association *asoc;
	uint16_t tt;

	asoc = &stcb->asoc;
	tt = strmin->last_sequence_delivered;
	



	TAILQ_FOREACH_SAFE(ctl, &strmin->inqueue, next, nctl) {
		if (SCTP_SSN_GE(tt, ctl->sinfo_ssn)) {
			
			TAILQ_REMOVE(&strmin->inqueue, ctl, next);
			
			asoc->size_on_all_streams -= ctl->length;
			sctp_ucount_decr(asoc->cnt_on_all_streams);
			
			if (stcb->sctp_socket) {
				sctp_mark_non_revokable(asoc, ctl->sinfo_tsn);
				sctp_add_to_readq(stcb->sctp_ep, stcb,
						  ctl,
						  &stcb->sctp_socket->so_rcv, 1, SCTP_READ_LOCK_HELD, SCTP_SO_NOT_LOCKED);
			}
		} else {
			
			break;
		}
	}
	



	tt = strmin->last_sequence_delivered + 1;
	TAILQ_FOREACH_SAFE(ctl, &strmin->inqueue, next, nctl) {
		if (tt == ctl->sinfo_ssn) {
			
			TAILQ_REMOVE(&strmin->inqueue, ctl, next);
			
			asoc->size_on_all_streams -= ctl->length;
			sctp_ucount_decr(asoc->cnt_on_all_streams);
			
			strmin->last_sequence_delivered = ctl->sinfo_ssn;
			if (stcb->sctp_socket) {
				sctp_mark_non_revokable(asoc, ctl->sinfo_tsn);
				sctp_add_to_readq(stcb->sctp_ep, stcb,
						  ctl,
						  &stcb->sctp_socket->so_rcv, 1, SCTP_READ_LOCK_HELD, SCTP_SO_NOT_LOCKED);

			}
			tt = strmin->last_sequence_delivered + 1;
		} else {
			break;
		}
	}
}

static void
sctp_flush_reassm_for_str_seq(struct sctp_tcb *stcb,
	struct sctp_association *asoc,
	uint16_t stream, uint16_t seq)
{
	struct sctp_tmit_chunk *chk, *nchk;

	
	







	TAILQ_FOREACH_SAFE(chk, &asoc->reasmqueue, sctp_next, nchk) {
		



		if ((chk->rec.data.stream_number != stream) ||
		    ((chk->rec.data.rcv_flags & SCTP_DATA_UNORDERED) == SCTP_DATA_UNORDERED)) {
				continue;
		}
		if (chk->rec.data.stream_seq == seq) {
			
			TAILQ_REMOVE(&asoc->reasmqueue, chk, sctp_next);
			if (SCTP_TSN_GT(chk->rec.data.TSN_seq, asoc->tsn_last_delivered)) {
				asoc->tsn_last_delivered = chk->rec.data.TSN_seq;
				asoc->str_of_pdapi = chk->rec.data.stream_number;
				asoc->ssn_of_pdapi = chk->rec.data.stream_seq;
				asoc->fragment_flags = chk->rec.data.rcv_flags;
			}
			asoc->size_on_reasm_queue -= chk->send_size;
			sctp_ucount_decr(asoc->cnt_on_reasm_queue);

			
			if ((chk->rec.data.rcv_flags & SCTP_DATA_UNORDERED) != SCTP_DATA_UNORDERED &&
			    SCTP_SSN_GT(chk->rec.data.stream_seq, asoc->strmin[chk->rec.data.stream_number].last_sequence_delivered)) {
				
















				asoc->strmin[chk->rec.data.stream_number].last_sequence_delivered = chk->rec.data.stream_seq;
			}
			if (chk->data) {
				sctp_m_freem(chk->data);
				chk->data = NULL;
			}
			sctp_free_a_chunk(stcb, chk, SCTP_SO_NOT_LOCKED);
		} else if (SCTP_SSN_GT(chk->rec.data.stream_seq, seq)) {
			
			break;
		}
	}
}


void
sctp_handle_forward_tsn(struct sctp_tcb *stcb,
                        struct sctp_forward_tsn_chunk *fwd,
                        int *abort_flag, struct mbuf *m ,int offset)
{
	
	










	struct sctp_association *asoc;
	uint32_t new_cum_tsn, gap;
	unsigned int i, fwd_sz, m_size;
	uint32_t str_seq;
	struct sctp_stream_in *strm;
	struct sctp_tmit_chunk *chk, *nchk;
	struct sctp_queued_to_read *ctl, *sv;

	asoc = &stcb->asoc;
	if ((fwd_sz = ntohs(fwd->ch.chunk_length)) < sizeof(struct sctp_forward_tsn_chunk)) {
		SCTPDBG(SCTP_DEBUG_INDATA1,
			"Bad size too small/big fwd-tsn\n");
		return;
	}
	m_size = (stcb->asoc.mapping_array_size << 3);
	
	
	
	new_cum_tsn = ntohl(fwd->new_cumulative_tsn);

	if (SCTP_TSN_GE(asoc->cumulative_tsn, new_cum_tsn)) {
		
		return;
	}
	



	SCTP_CALC_TSN_TO_GAP(gap, new_cum_tsn, asoc->mapping_array_base_tsn);
	asoc->cumulative_tsn = new_cum_tsn;
	if (gap >= m_size) {
		if ((long)gap > sctp_sbspace(&stcb->asoc, &stcb->sctp_socket->so_rcv)) {
			struct mbuf *oper;
			



			*abort_flag = 1;
			oper = sctp_get_mbuf_for_msg((sizeof(struct sctp_paramhdr) + 3 * sizeof(uint32_t)),
					     0, M_NOWAIT, 1, MT_DATA);
			if (oper) {
				struct sctp_paramhdr *ph;
				uint32_t *ippp;
				SCTP_BUF_LEN(oper) = sizeof(struct sctp_paramhdr) +
					(sizeof(uint32_t) * 3);
				ph = mtod(oper, struct sctp_paramhdr *);
				ph->param_type = htons(SCTP_CAUSE_PROTOCOL_VIOLATION);
				ph->param_length = htons(SCTP_BUF_LEN(oper));
				ippp = (uint32_t *) (ph + 1);
				*ippp = htonl(SCTP_FROM_SCTP_INDATA+SCTP_LOC_33);
				ippp++;
				*ippp = asoc->highest_tsn_inside_map;
				ippp++;
				*ippp = new_cum_tsn;
			}
			stcb->sctp_ep->last_abort_code = SCTP_FROM_SCTP_INDATA+SCTP_LOC_33;
			sctp_abort_an_association(stcb->sctp_ep, stcb, oper, SCTP_SO_NOT_LOCKED);
			return;
		}
		SCTP_STAT_INCR(sctps_fwdtsn_map_over);

		memset(stcb->asoc.mapping_array, 0, stcb->asoc.mapping_array_size);
		asoc->mapping_array_base_tsn = new_cum_tsn + 1;
		asoc->highest_tsn_inside_map = new_cum_tsn;

		memset(stcb->asoc.nr_mapping_array, 0, stcb->asoc.mapping_array_size);
		asoc->highest_tsn_inside_nr_map = new_cum_tsn;

		if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_MAP_LOGGING_ENABLE) {
			sctp_log_map(0, 3, asoc->highest_tsn_inside_map, SCTP_MAP_SLIDE_RESULT);
		}
	} else {
		SCTP_TCB_LOCK_ASSERT(stcb);
		for (i = 0; i <= gap; i++) {
			if (!SCTP_IS_TSN_PRESENT(asoc->mapping_array, i) &&
			    !SCTP_IS_TSN_PRESENT(asoc->nr_mapping_array, i)) {
				SCTP_SET_TSN_PRESENT(asoc->nr_mapping_array, i);
				if (SCTP_TSN_GT(asoc->mapping_array_base_tsn + i, asoc->highest_tsn_inside_nr_map)) {
					asoc->highest_tsn_inside_nr_map = asoc->mapping_array_base_tsn + i;
				}
			}
		}
	}
	
	
	
	



	if (asoc->fragmented_delivery_inprogress) {
		sctp_service_reassembly(stcb, asoc);
	}
	
	







	TAILQ_FOREACH_SAFE(chk, &asoc->reasmqueue, sctp_next, nchk) {
		if (SCTP_TSN_GE(new_cum_tsn, chk->rec.data.TSN_seq)) {
			
			TAILQ_REMOVE(&asoc->reasmqueue, chk, sctp_next);
			if (SCTP_TSN_GT(chk->rec.data.TSN_seq, asoc->tsn_last_delivered)) {
				asoc->tsn_last_delivered = chk->rec.data.TSN_seq;
				asoc->str_of_pdapi = chk->rec.data.stream_number;
				asoc->ssn_of_pdapi = chk->rec.data.stream_seq;
				asoc->fragment_flags = chk->rec.data.rcv_flags;
			}
			asoc->size_on_reasm_queue -= chk->send_size;
			sctp_ucount_decr(asoc->cnt_on_reasm_queue);

			
			if ((chk->rec.data.rcv_flags & SCTP_DATA_UNORDERED) != SCTP_DATA_UNORDERED &&
			    SCTP_SSN_GT(chk->rec.data.stream_seq, asoc->strmin[chk->rec.data.stream_number].last_sequence_delivered)) {
				
















				asoc->strmin[chk->rec.data.stream_number].last_sequence_delivered = chk->rec.data.stream_seq;
			}
			if (chk->data) {
				sctp_m_freem(chk->data);
				chk->data = NULL;
			}
			sctp_free_a_chunk(stcb, chk, SCTP_SO_NOT_LOCKED);
		} else {
			



			break;
		}
	}
	
	
	
	
	fwd_sz -= sizeof(*fwd);
	if (m && fwd_sz) {
		
		unsigned int num_str;
		struct sctp_strseq *stseq, strseqbuf;
		offset += sizeof(*fwd);

		SCTP_INP_READ_LOCK(stcb->sctp_ep);
		num_str = fwd_sz / sizeof(struct sctp_strseq);
		for (i = 0; i < num_str; i++) {
			uint16_t st;
			stseq = (struct sctp_strseq *)sctp_m_getptr(m, offset,
								    sizeof(struct sctp_strseq),
								    (uint8_t *)&strseqbuf);
			offset += sizeof(struct sctp_strseq);
			if (stseq == NULL) {
				break;
			}
			
			st = ntohs(stseq->stream);
			stseq->stream = st;
			st = ntohs(stseq->sequence);
			stseq->sequence = st;

			

			




			if (stseq->stream >= asoc->streamincnt) {
				
				break;
			}
			if ((asoc->str_of_pdapi == stseq->stream) &&
			    (asoc->ssn_of_pdapi == stseq->sequence)) {
				



				asoc->fragmented_delivery_inprogress = 0;
			}
			sctp_flush_reassm_for_str_seq(stcb, asoc, stseq->stream, stseq->sequence);
			TAILQ_FOREACH(ctl, &stcb->sctp_ep->read_queue, next) {
				if ((ctl->sinfo_stream == stseq->stream) &&
				    (ctl->sinfo_ssn == stseq->sequence)) {
					str_seq = (stseq->stream << 16) | stseq->sequence;
					ctl->end_added = 1;
					ctl->pdapi_aborted = 1;
					sv = stcb->asoc.control_pdapi;
					stcb->asoc.control_pdapi = ctl;
					sctp_ulp_notify(SCTP_NOTIFY_PARTIAL_DELVIERY_INDICATION,
					                stcb,
					                SCTP_PARTIAL_DELIVERY_ABORTED,
					                (void *)&str_seq,
							SCTP_SO_NOT_LOCKED);
					stcb->asoc.control_pdapi = sv;
					break;
				} else if ((ctl->sinfo_stream == stseq->stream) &&
					   SCTP_SSN_GT(ctl->sinfo_ssn, stseq->sequence)) {
					
					break;
				}
			}
			strm = &asoc->strmin[stseq->stream];
			if (SCTP_SSN_GT(stseq->sequence, strm->last_sequence_delivered)) {
				
				strm->last_sequence_delivered = stseq->sequence;
			}
			
                        
			sctp_kick_prsctp_reorder_queue(stcb, strm);
		}
		SCTP_INP_READ_UNLOCK(stcb->sctp_ep);
	}
	


	sctp_slide_mapping_arrays(stcb);

	if (!TAILQ_EMPTY(&asoc->reasmqueue)) {
		
                
		sctp_deliver_reasm_check(stcb, &stcb->asoc);
	}
}
