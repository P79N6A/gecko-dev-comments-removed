



























#ifdef __FreeBSD__
#include <sys/cdefs.h>
__FBSDID("$FreeBSD: head/sys/netinet/sctp_ss_functions.c 235828 2012-05-23 11:26:28Z tuexen $");
#endif

#include <netinet/sctp_pcb.h>
#if defined(__Userspace__)
#include <netinet/sctp_os_userspace.h>
#endif






static void
sctp_ss_default_add(struct sctp_tcb *, struct sctp_association *,
                    struct sctp_stream_out *,
                    struct sctp_stream_queue_pending *, int);

static void
sctp_ss_default_remove(struct sctp_tcb *, struct sctp_association *,
                       struct sctp_stream_out *,
                       struct sctp_stream_queue_pending *, int);

static void
sctp_ss_default_init(struct sctp_tcb *stcb, struct sctp_association *asoc,
                     int holds_lock)
{
	uint16_t i;

	TAILQ_INIT(&asoc->ss_data.out_wheel);
	





	for (i = 0; i < stcb->asoc.streamoutcnt; i++) {
		stcb->asoc.ss_functions.sctp_ss_add_to_stream(stcb, &stcb->asoc,
		                                              &stcb->asoc.strmout[i],
		                                              NULL, holds_lock);
	}
	return;
}

static void
sctp_ss_default_clear(struct sctp_tcb *stcb, struct sctp_association *asoc,
                      int clear_values SCTP_UNUSED, int holds_lock)
{
	if (holds_lock == 0) {
		SCTP_TCB_SEND_LOCK(stcb);
	}
	while (!TAILQ_EMPTY(&asoc->ss_data.out_wheel)) {
		struct sctp_stream_out *strq = TAILQ_FIRST(&asoc->ss_data.out_wheel);
		TAILQ_REMOVE(&asoc->ss_data.out_wheel, TAILQ_FIRST(&asoc->ss_data.out_wheel), ss_params.rr.next_spoke);
		strq->ss_params.rr.next_spoke.tqe_next = NULL;
		strq->ss_params.rr.next_spoke.tqe_prev = NULL;
	}
	asoc->last_out_stream = NULL;
	if (holds_lock == 0) {
		SCTP_TCB_SEND_UNLOCK(stcb);
	}
	return;
}

static void
sctp_ss_default_init_stream(struct sctp_stream_out *strq, struct sctp_stream_out *with_strq SCTP_UNUSED)
{
	strq->ss_params.rr.next_spoke.tqe_next = NULL;
	strq->ss_params.rr.next_spoke.tqe_prev = NULL;
	return;
}

static void
sctp_ss_default_add(struct sctp_tcb *stcb, struct sctp_association *asoc,
                    struct sctp_stream_out *strq,
                    struct sctp_stream_queue_pending *sp SCTP_UNUSED, int holds_lock)
{
	if (holds_lock == 0) {
		SCTP_TCB_SEND_LOCK(stcb);
	}
	
	if (!TAILQ_EMPTY(&strq->outqueue) &&
	    (strq->ss_params.rr.next_spoke.tqe_next == NULL) &&
	    (strq->ss_params.rr.next_spoke.tqe_prev == NULL)) {
		TAILQ_INSERT_TAIL(&asoc->ss_data.out_wheel,
		                  strq, ss_params.rr.next_spoke);
	}
	if (holds_lock == 0) {
		SCTP_TCB_SEND_UNLOCK(stcb);
	}
	return;
}

static int
sctp_ss_default_is_empty(struct sctp_tcb *stcb SCTP_UNUSED, struct sctp_association *asoc)
{
	if (TAILQ_EMPTY(&asoc->ss_data.out_wheel)) {
		return (1);
	} else {
		return (0);
	}
}

static void
sctp_ss_default_remove(struct sctp_tcb *stcb, struct sctp_association *asoc,
                       struct sctp_stream_out *strq,
                       struct sctp_stream_queue_pending *sp SCTP_UNUSED, int holds_lock)
{
	if (holds_lock == 0) {
		SCTP_TCB_SEND_LOCK(stcb);
	}
	
	if (TAILQ_EMPTY(&strq->outqueue) &&
	    (strq->ss_params.rr.next_spoke.tqe_next != NULL ||
	    strq->ss_params.rr.next_spoke.tqe_prev != NULL)) {
		if (asoc->last_out_stream == strq) {
			asoc->last_out_stream = TAILQ_PREV(asoc->last_out_stream,
			                                   sctpwheel_listhead,
			                                   ss_params.rr.next_spoke);
			if (asoc->last_out_stream == NULL) {
				asoc->last_out_stream = TAILQ_LAST(&asoc->ss_data.out_wheel,
				                                   sctpwheel_listhead);
			}
			if (asoc->last_out_stream == strq) {
				asoc->last_out_stream = NULL;
			}
		}
		TAILQ_REMOVE(&asoc->ss_data.out_wheel, strq, ss_params.rr.next_spoke);
		strq->ss_params.rr.next_spoke.tqe_next = NULL;
		strq->ss_params.rr.next_spoke.tqe_prev = NULL;
	}
	if (holds_lock == 0) {
		SCTP_TCB_SEND_UNLOCK(stcb);
	}
	return;
}


static struct sctp_stream_out *
sctp_ss_default_select(struct sctp_tcb *stcb SCTP_UNUSED, struct sctp_nets *net,
                       struct sctp_association *asoc)
{
	struct sctp_stream_out *strq, *strqt;

	strqt = asoc->last_out_stream;
default_again:
	
	if (strqt == NULL) {
		strq = TAILQ_FIRST(&asoc->ss_data.out_wheel);
	} else {
		strq = TAILQ_NEXT(strqt, ss_params.rr.next_spoke);
		if (strq == NULL) {
			strq = TAILQ_FIRST(&asoc->ss_data.out_wheel);
		}
	}

	










	if (net != NULL && strq != NULL &&
	    SCTP_BASE_SYSCTL(sctp_cmt_on_off) == 0) {
		if (TAILQ_FIRST(&strq->outqueue) &&
		    TAILQ_FIRST(&strq->outqueue)->net != NULL &&
		    TAILQ_FIRST(&strq->outqueue)->net != net) {
			if (strq == asoc->last_out_stream) {
				return (NULL);
			} else {
				strqt = strq;
				goto default_again;
			}
		}
	}
	return (strq);
}

static void
sctp_ss_default_scheduled(struct sctp_tcb *stcb SCTP_UNUSED, struct sctp_nets *net SCTP_UNUSED,
                          struct sctp_association *asoc SCTP_UNUSED,
                          struct sctp_stream_out *strq, int moved_how_much SCTP_UNUSED)
{
	asoc->last_out_stream = strq;
	return;
}

static void
sctp_ss_default_packet_done(struct sctp_tcb *stcb SCTP_UNUSED, struct sctp_nets *net SCTP_UNUSED,
                            struct sctp_association *asoc SCTP_UNUSED)
{
	
	return;
}

static int
sctp_ss_default_get_value(struct sctp_tcb *stcb SCTP_UNUSED, struct sctp_association *asoc SCTP_UNUSED,
                          struct sctp_stream_out *strq SCTP_UNUSED, uint16_t *value SCTP_UNUSED)
{
	
	return (-1);
}

static int
sctp_ss_default_set_value(struct sctp_tcb *stcb SCTP_UNUSED, struct sctp_association *asoc SCTP_UNUSED,
                          struct sctp_stream_out *strq SCTP_UNUSED, uint16_t value SCTP_UNUSED)
{
	
	return (-1);
}





static void
sctp_ss_rr_add(struct sctp_tcb *stcb, struct sctp_association *asoc,
               struct sctp_stream_out *strq,
               struct sctp_stream_queue_pending *sp SCTP_UNUSED, int holds_lock)
{
	struct sctp_stream_out *strqt;

	if (holds_lock == 0) {
		SCTP_TCB_SEND_LOCK(stcb);
	}
	if (!TAILQ_EMPTY(&strq->outqueue) &&
	    (strq->ss_params.rr.next_spoke.tqe_next == NULL) &&
	    (strq->ss_params.rr.next_spoke.tqe_prev == NULL)) {
		if (TAILQ_EMPTY(&asoc->ss_data.out_wheel)) {
			TAILQ_INSERT_HEAD(&asoc->ss_data.out_wheel, strq, ss_params.rr.next_spoke);
		} else {
			strqt = TAILQ_FIRST(&asoc->ss_data.out_wheel);
			while (strqt != NULL && (strqt->stream_no < strq->stream_no)) {
				strqt = TAILQ_NEXT(strqt, ss_params.rr.next_spoke);
			}
			if (strqt != NULL) {
				TAILQ_INSERT_BEFORE(strqt, strq, ss_params.rr.next_spoke);
			} else {
				TAILQ_INSERT_TAIL(&asoc->ss_data.out_wheel, strq, ss_params.rr.next_spoke);
			}
		}
	}
	if (holds_lock == 0) {
		SCTP_TCB_SEND_UNLOCK(stcb);
	}
	return;
}






static struct sctp_stream_out *
sctp_ss_rrp_select(struct sctp_tcb *stcb SCTP_UNUSED, struct sctp_nets *net SCTP_UNUSED,
                   struct sctp_association *asoc)
{
	return (asoc->last_out_stream);
}

static void
sctp_ss_rrp_packet_done(struct sctp_tcb *stcb SCTP_UNUSED, struct sctp_nets *net,
                        struct sctp_association *asoc)
{
	struct sctp_stream_out *strq, *strqt;

	strqt = asoc->last_out_stream;
rrp_again:
	
	if (strqt == NULL) {
		strq = TAILQ_FIRST(&asoc->ss_data.out_wheel);
	} else {
		strq = TAILQ_NEXT(strqt, ss_params.rr.next_spoke);
		if (strq == NULL) {
			strq = TAILQ_FIRST(&asoc->ss_data.out_wheel);
		}
	}

	










	if (net != NULL && strq != NULL &&
	    SCTP_BASE_SYSCTL(sctp_cmt_on_off) == 0) {
		if (TAILQ_FIRST(&strq->outqueue) &&
		    TAILQ_FIRST(&strq->outqueue)->net != NULL &&
		    TAILQ_FIRST(&strq->outqueue)->net != net) {
			if (strq == asoc->last_out_stream) {
				strq = NULL;
			} else {
				strqt = strq;
				goto rrp_again;
			}
		}
	}
	asoc->last_out_stream = strq;
	return;
}






static void
sctp_ss_prio_clear(struct sctp_tcb *stcb, struct sctp_association *asoc,
                   int clear_values, int holds_lock)
{
	if (holds_lock == 0) {
		SCTP_TCB_SEND_LOCK(stcb);
	}
	while (!TAILQ_EMPTY(&asoc->ss_data.out_wheel)) {
		struct sctp_stream_out *strq = TAILQ_FIRST(&asoc->ss_data.out_wheel);
		if (clear_values) {
			strq->ss_params.prio.priority = 0;
		}
		TAILQ_REMOVE(&asoc->ss_data.out_wheel, TAILQ_FIRST(&asoc->ss_data.out_wheel), ss_params.prio.next_spoke);
		strq->ss_params.prio.next_spoke.tqe_next = NULL;
		strq->ss_params.prio.next_spoke.tqe_prev = NULL;

	}
	asoc->last_out_stream = NULL;
	if (holds_lock == 0) {
		SCTP_TCB_SEND_UNLOCK(stcb);
	}
	return;
}

static void
sctp_ss_prio_init_stream(struct sctp_stream_out *strq, struct sctp_stream_out *with_strq)
{
	strq->ss_params.prio.next_spoke.tqe_next = NULL;
	strq->ss_params.prio.next_spoke.tqe_prev = NULL;
	if (with_strq != NULL) {
		strq->ss_params.prio.priority = with_strq->ss_params.prio.priority;
	} else {
		strq->ss_params.prio.priority = 0;
	}
	return;
}

static void
sctp_ss_prio_add(struct sctp_tcb *stcb, struct sctp_association *asoc,
                 struct sctp_stream_out *strq, struct sctp_stream_queue_pending *sp SCTP_UNUSED,
                 int holds_lock)
{
	struct sctp_stream_out *strqt;

	if (holds_lock == 0) {
		SCTP_TCB_SEND_LOCK(stcb);
	}
	
	if (!TAILQ_EMPTY(&strq->outqueue) &&
	    (strq->ss_params.prio.next_spoke.tqe_next == NULL) &&
	    (strq->ss_params.prio.next_spoke.tqe_prev == NULL)) {
		if (TAILQ_EMPTY(&asoc->ss_data.out_wheel)) {
			TAILQ_INSERT_HEAD(&asoc->ss_data.out_wheel, strq, ss_params.prio.next_spoke);
		} else {
			strqt = TAILQ_FIRST(&asoc->ss_data.out_wheel);
			while (strqt != NULL && strqt->ss_params.prio.priority < strq->ss_params.prio.priority) {
				strqt = TAILQ_NEXT(strqt, ss_params.prio.next_spoke);
			}
			if (strqt != NULL) {
				TAILQ_INSERT_BEFORE(strqt, strq, ss_params.prio.next_spoke);
			} else {
				TAILQ_INSERT_TAIL(&asoc->ss_data.out_wheel, strq, ss_params.prio.next_spoke);
			}
		}
	}
	if (holds_lock == 0) {
		SCTP_TCB_SEND_UNLOCK(stcb);
	}
	return;
}

static void
sctp_ss_prio_remove(struct sctp_tcb *stcb, struct sctp_association *asoc,
                    struct sctp_stream_out *strq, struct sctp_stream_queue_pending *sp SCTP_UNUSED,
                    int holds_lock)
{
	if (holds_lock == 0) {
		SCTP_TCB_SEND_LOCK(stcb);
	}
	
	if (TAILQ_EMPTY(&strq->outqueue) &&
	    (strq->ss_params.prio.next_spoke.tqe_next != NULL ||
	    strq->ss_params.prio.next_spoke.tqe_prev != NULL)) {
		if (asoc->last_out_stream == strq) {
			asoc->last_out_stream = TAILQ_PREV(asoc->last_out_stream, sctpwheel_listhead,
			                                   ss_params.prio.next_spoke);
			if (asoc->last_out_stream == NULL) {
				asoc->last_out_stream = TAILQ_LAST(&asoc->ss_data.out_wheel,
				                                   sctpwheel_listhead);
			}
			if (asoc->last_out_stream == strq) {
				asoc->last_out_stream = NULL;
			}
		}
		TAILQ_REMOVE(&asoc->ss_data.out_wheel, strq, ss_params.prio.next_spoke);
		strq->ss_params.prio.next_spoke.tqe_next = NULL;
		strq->ss_params.prio.next_spoke.tqe_prev = NULL;
	}
	if (holds_lock == 0) {
		SCTP_TCB_SEND_UNLOCK(stcb);
	}
	return;
}

static struct sctp_stream_out*
sctp_ss_prio_select(struct sctp_tcb *stcb SCTP_UNUSED, struct sctp_nets *net,
                    struct sctp_association *asoc)
{
	struct sctp_stream_out *strq, *strqt, *strqn;

	strqt = asoc->last_out_stream;
prio_again:
	
	if (strqt == NULL) {
		strq = TAILQ_FIRST(&asoc->ss_data.out_wheel);
	} else {
		strqn = TAILQ_NEXT(strqt, ss_params.prio.next_spoke);
		if (strqn != NULL &&
		    strqn->ss_params.prio.priority == strqt->ss_params.prio.priority) {
			strq = strqn;
		} else {
			strq = TAILQ_FIRST(&asoc->ss_data.out_wheel);
		}
	}

	










	if (net != NULL && strq != NULL &&
	    SCTP_BASE_SYSCTL(sctp_cmt_on_off) == 0) {
		if (TAILQ_FIRST(&strq->outqueue) &&
		    TAILQ_FIRST(&strq->outqueue)->net != NULL &&
		    TAILQ_FIRST(&strq->outqueue)->net != net) {
			if (strq == asoc->last_out_stream) {
				return (NULL);
			} else {
				strqt = strq;
				goto prio_again;
			}
		}
	}
	return (strq);
}

static int
sctp_ss_prio_get_value(struct sctp_tcb *stcb SCTP_UNUSED, struct sctp_association *asoc SCTP_UNUSED,
                       struct sctp_stream_out *strq, uint16_t *value)
{
	if (strq == NULL) {
		return (-1);
	}
	*value = strq->ss_params.prio.priority;
	return (1);
}

static int
sctp_ss_prio_set_value(struct sctp_tcb *stcb, struct sctp_association *asoc,
                       struct sctp_stream_out *strq, uint16_t value)
{
	if (strq == NULL) {
		return (-1);
	}
	strq->ss_params.prio.priority = value;
	sctp_ss_prio_remove(stcb, asoc, strq, NULL, 1);
	sctp_ss_prio_add(stcb, asoc, strq, NULL, 1);
	return (1);
}





static void
sctp_ss_fb_clear(struct sctp_tcb *stcb, struct sctp_association *asoc,
                   int clear_values, int holds_lock)
{
	if (holds_lock == 0) {
		SCTP_TCB_SEND_LOCK(stcb);
	}
	while (!TAILQ_EMPTY(&asoc->ss_data.out_wheel)) {
		struct sctp_stream_out *strq = TAILQ_FIRST(&asoc->ss_data.out_wheel);
		if (clear_values) {
			strq->ss_params.fb.rounds = -1;
		}
		TAILQ_REMOVE(&asoc->ss_data.out_wheel, TAILQ_FIRST(&asoc->ss_data.out_wheel), ss_params.fb.next_spoke);
		strq->ss_params.fb.next_spoke.tqe_next = NULL;
		strq->ss_params.fb.next_spoke.tqe_prev = NULL;
	}
	asoc->last_out_stream = NULL;
	if (holds_lock == 0) {
		SCTP_TCB_SEND_UNLOCK(stcb);
	}
	return;
}

static void
sctp_ss_fb_init_stream(struct sctp_stream_out *strq, struct sctp_stream_out *with_strq)
{
	strq->ss_params.fb.next_spoke.tqe_next = NULL;
	strq->ss_params.fb.next_spoke.tqe_prev = NULL;
	if (with_strq != NULL) {
		strq->ss_params.fb.rounds = with_strq->ss_params.fb.rounds;
	} else {
		strq->ss_params.fb.rounds = -1;
	}
	return;
}

static void
sctp_ss_fb_add(struct sctp_tcb *stcb, struct sctp_association *asoc,
               struct sctp_stream_out *strq, struct sctp_stream_queue_pending *sp SCTP_UNUSED,
               int holds_lock)
{
	if (holds_lock == 0) {
		SCTP_TCB_SEND_LOCK(stcb);
	}
	if (!TAILQ_EMPTY(&strq->outqueue) &&
	    (strq->ss_params.fb.next_spoke.tqe_next == NULL) &&
	    (strq->ss_params.fb.next_spoke.tqe_prev == NULL)) {
		if (strq->ss_params.fb.rounds < 0)
			strq->ss_params.fb.rounds = TAILQ_FIRST(&strq->outqueue)->length;
		TAILQ_INSERT_TAIL(&asoc->ss_data.out_wheel, strq, ss_params.fb.next_spoke);
	}
	if (holds_lock == 0) {
		SCTP_TCB_SEND_UNLOCK(stcb);
	}
	return;
}

static void
sctp_ss_fb_remove(struct sctp_tcb *stcb, struct sctp_association *asoc,
                  struct sctp_stream_out *strq, struct sctp_stream_queue_pending *sp SCTP_UNUSED,
                  int holds_lock)
{
	if (holds_lock == 0) {
		SCTP_TCB_SEND_LOCK(stcb);
	}
	
	if (TAILQ_EMPTY(&strq->outqueue) &&
	    (strq->ss_params.fb.next_spoke.tqe_next != NULL ||
	    strq->ss_params.fb.next_spoke.tqe_prev != NULL)) {
		if (asoc->last_out_stream == strq) {
			asoc->last_out_stream = TAILQ_PREV(asoc->last_out_stream, sctpwheel_listhead,
			                                   ss_params.fb.next_spoke);
			if (asoc->last_out_stream == NULL) {
				asoc->last_out_stream = TAILQ_LAST(&asoc->ss_data.out_wheel,
				                                   sctpwheel_listhead);
			}
			if (asoc->last_out_stream == strq) {
				asoc->last_out_stream = NULL;
			}
		}
		TAILQ_REMOVE(&asoc->ss_data.out_wheel, strq, ss_params.fb.next_spoke);
		strq->ss_params.fb.next_spoke.tqe_next = NULL;
		strq->ss_params.fb.next_spoke.tqe_prev = NULL;
	}
	if (holds_lock == 0) {
		SCTP_TCB_SEND_UNLOCK(stcb);
	}
	return;
}

static struct sctp_stream_out*
sctp_ss_fb_select(struct sctp_tcb *stcb SCTP_UNUSED, struct sctp_nets *net,
                  struct sctp_association *asoc)
{
	struct sctp_stream_out *strq = NULL, *strqt;

	if (asoc->last_out_stream == NULL ||
	    TAILQ_FIRST(&asoc->ss_data.out_wheel) == TAILQ_LAST(&asoc->ss_data.out_wheel, sctpwheel_listhead)) {
		strqt = TAILQ_FIRST(&asoc->ss_data.out_wheel);
	} else {
		strqt = TAILQ_NEXT(asoc->last_out_stream, ss_params.fb.next_spoke);
	}
	do {
		if ((strqt != NULL) &&
		    ((SCTP_BASE_SYSCTL(sctp_cmt_on_off) > 0) ||
		     (SCTP_BASE_SYSCTL(sctp_cmt_on_off) == 0 &&
		      (net == NULL || (TAILQ_FIRST(&strqt->outqueue) && TAILQ_FIRST(&strqt->outqueue)->net == NULL) ||
		       (net != NULL && TAILQ_FIRST(&strqt->outqueue) && TAILQ_FIRST(&strqt->outqueue)->net != NULL &&
		        TAILQ_FIRST(&strqt->outqueue)->net == net))))) {
			if ((strqt->ss_params.fb.rounds >= 0) && (strq == NULL ||
				strqt->ss_params.fb.rounds < strq->ss_params.fb.rounds)) {
				strq = strqt;
			}
		}
		if (strqt != NULL) {
			strqt = TAILQ_NEXT(strqt, ss_params.fb.next_spoke);
		} else {
			strqt = TAILQ_FIRST(&asoc->ss_data.out_wheel);
		}
	} while (strqt != strq);
	return (strq);
}

static void
sctp_ss_fb_scheduled(struct sctp_tcb *stcb SCTP_UNUSED, struct sctp_nets *net SCTP_UNUSED,
                     struct sctp_association *asoc, struct sctp_stream_out *strq,
                     int moved_how_much SCTP_UNUSED)
{
	struct sctp_stream_out *strqt;
	int subtract;

	subtract = strq->ss_params.fb.rounds;
	TAILQ_FOREACH(strqt, &asoc->ss_data.out_wheel, ss_params.fb.next_spoke) {
		strqt->ss_params.fb.rounds -= subtract;
		if (strqt->ss_params.fb.rounds < 0)
			strqt->ss_params.fb.rounds = 0;
	}
	if (TAILQ_FIRST(&strq->outqueue)) {
		strq->ss_params.fb.rounds = TAILQ_FIRST(&strq->outqueue)->length;
	} else {
		strq->ss_params.fb.rounds = -1;
	}
	asoc->last_out_stream = strq;
	return;
}





static void
sctp_ss_fcfs_add(struct sctp_tcb *stcb, struct sctp_association *asoc,
                 struct sctp_stream_out *strq, struct sctp_stream_queue_pending *sp,
                 int holds_lock);

static void
sctp_ss_fcfs_init(struct sctp_tcb *stcb, struct sctp_association *asoc,
                  int holds_lock)
{
	uint32_t x, n = 0, add_more = 1;
	struct sctp_stream_queue_pending *sp;
	uint16_t i;

	TAILQ_INIT(&asoc->ss_data.out_list);
	






	while (add_more) {
		add_more = 0;
		for (i = 0; i < stcb->asoc.streamoutcnt; i++) {
			sp = TAILQ_FIRST(&stcb->asoc.strmout[i].outqueue);
			x = 0;
			
			while (sp != NULL && x < n) {
				sp = TAILQ_NEXT(sp, next);
				x++;
			}
			if (sp != NULL) {
				sctp_ss_fcfs_add(stcb, &stcb->asoc, &stcb->asoc.strmout[i], sp, holds_lock);
				add_more = 1;
			}
		}
		n++;
	}
	return;
}

static void
sctp_ss_fcfs_clear(struct sctp_tcb *stcb, struct sctp_association *asoc,
                   int clear_values, int holds_lock)
{
	if (clear_values) {
		if (holds_lock == 0) {
			SCTP_TCB_SEND_LOCK(stcb);
		}
		while (!TAILQ_EMPTY(&asoc->ss_data.out_list)) {
			TAILQ_REMOVE(&asoc->ss_data.out_list, TAILQ_FIRST(&asoc->ss_data.out_list), ss_next);
		}
		if (holds_lock == 0) {
			SCTP_TCB_SEND_UNLOCK(stcb);
		}
	}
	return;
}

static void
sctp_ss_fcfs_init_stream(struct sctp_stream_out *strq SCTP_UNUSED, struct sctp_stream_out *with_strq SCTP_UNUSED)
{
	
	return;
}

static void
sctp_ss_fcfs_add(struct sctp_tcb *stcb, struct sctp_association *asoc,
                 struct sctp_stream_out *strq SCTP_UNUSED, struct sctp_stream_queue_pending *sp,
                 int holds_lock)
{
	if (holds_lock == 0) {
		SCTP_TCB_SEND_LOCK(stcb);
	}
	if (sp && (sp->ss_next.tqe_next == NULL) &&
	    (sp->ss_next.tqe_prev == NULL)) {
		TAILQ_INSERT_TAIL(&asoc->ss_data.out_list, sp, ss_next);
	}
	if (holds_lock == 0) {
		SCTP_TCB_SEND_UNLOCK(stcb);
	}
	return;
}

static int
sctp_ss_fcfs_is_empty(struct sctp_tcb *stcb SCTP_UNUSED, struct sctp_association *asoc)
{
	if (TAILQ_EMPTY(&asoc->ss_data.out_list)) {
		return (1);
	} else {
		return (0);
	}
}

static void
sctp_ss_fcfs_remove(struct sctp_tcb *stcb, struct sctp_association *asoc,
                    struct sctp_stream_out *strq SCTP_UNUSED, struct sctp_stream_queue_pending *sp,
                    int holds_lock)
{
	if (holds_lock == 0) {
		SCTP_TCB_SEND_LOCK(stcb);
	}
	if (sp &&
	    ((sp->ss_next.tqe_next != NULL) ||
	     (sp->ss_next.tqe_prev != NULL))) {
		TAILQ_REMOVE(&asoc->ss_data.out_list, sp, ss_next);
	}
	if (holds_lock == 0) {
		SCTP_TCB_SEND_UNLOCK(stcb);
	}
	return;
}


static struct sctp_stream_out *
sctp_ss_fcfs_select(struct sctp_tcb *stcb SCTP_UNUSED, struct sctp_nets *net,
                    struct sctp_association *asoc)
{
	struct sctp_stream_out *strq;
	struct sctp_stream_queue_pending *sp;

	sp = TAILQ_FIRST(&asoc->ss_data.out_list);
default_again:
	if (sp != NULL) {
		strq = &asoc->strmout[sp->stream];
	} else {
		strq = NULL;
	}

	











	if (net != NULL && strq != NULL &&
	    SCTP_BASE_SYSCTL(sctp_cmt_on_off) == 0) {
		if (TAILQ_FIRST(&strq->outqueue) &&
		    TAILQ_FIRST(&strq->outqueue)->net != NULL &&
		    TAILQ_FIRST(&strq->outqueue)->net != net) {
			sp = TAILQ_NEXT(sp, ss_next);
			goto default_again;
		}
	}
	return (strq);
}

struct sctp_ss_functions sctp_ss_functions[] = {

{
#if defined(__Windows__) || defined(__Userspace_os_Windows)
	sctp_ss_default_init,
	sctp_ss_default_clear,
	sctp_ss_default_init_stream,
	sctp_ss_default_add,
	sctp_ss_default_is_empty,
	sctp_ss_default_remove,
	sctp_ss_default_select,
	sctp_ss_default_scheduled,
	sctp_ss_default_packet_done,
	sctp_ss_default_get_value,
	sctp_ss_default_set_value
#else
	.sctp_ss_init = sctp_ss_default_init,
	.sctp_ss_clear = sctp_ss_default_clear,
	.sctp_ss_init_stream = sctp_ss_default_init_stream,
	.sctp_ss_add_to_stream = sctp_ss_default_add,
	.sctp_ss_is_empty = sctp_ss_default_is_empty,
	.sctp_ss_remove_from_stream = sctp_ss_default_remove,
	.sctp_ss_select_stream = sctp_ss_default_select,
	.sctp_ss_scheduled = sctp_ss_default_scheduled,
	.sctp_ss_packet_done = sctp_ss_default_packet_done,
	.sctp_ss_get_value = sctp_ss_default_get_value,
	.sctp_ss_set_value = sctp_ss_default_set_value
#endif
},

{
#if defined(__Windows__) || defined(__Userspace_os_Windows)
	sctp_ss_default_init,
	sctp_ss_default_clear,
	sctp_ss_default_init_stream,
	sctp_ss_rr_add,
	sctp_ss_default_is_empty,
	sctp_ss_default_remove,
	sctp_ss_default_select,
	sctp_ss_default_scheduled,
	sctp_ss_default_packet_done,
	sctp_ss_default_get_value,
	sctp_ss_default_set_value
#else
	.sctp_ss_init = sctp_ss_default_init,
	.sctp_ss_clear = sctp_ss_default_clear,
	.sctp_ss_init_stream = sctp_ss_default_init_stream,
	.sctp_ss_add_to_stream = sctp_ss_rr_add,
	.sctp_ss_is_empty = sctp_ss_default_is_empty,
	.sctp_ss_remove_from_stream = sctp_ss_default_remove,
	.sctp_ss_select_stream = sctp_ss_default_select,
	.sctp_ss_scheduled = sctp_ss_default_scheduled,
	.sctp_ss_packet_done = sctp_ss_default_packet_done,
	.sctp_ss_get_value = sctp_ss_default_get_value,
	.sctp_ss_set_value = sctp_ss_default_set_value
#endif
},

{
#if defined(__Windows__) || defined(__Userspace_os_Windows)
	sctp_ss_default_init,
	sctp_ss_default_clear,
	sctp_ss_default_init_stream,
	sctp_ss_rr_add,
	sctp_ss_default_is_empty,
	sctp_ss_default_remove,
	sctp_ss_rrp_select,
	sctp_ss_default_scheduled,
	sctp_ss_rrp_packet_done,
	sctp_ss_default_get_value,
	sctp_ss_default_set_value
#else
	.sctp_ss_init = sctp_ss_default_init,
	.sctp_ss_clear = sctp_ss_default_clear,
	.sctp_ss_init_stream = sctp_ss_default_init_stream,
	.sctp_ss_add_to_stream = sctp_ss_rr_add,
	.sctp_ss_is_empty = sctp_ss_default_is_empty,
	.sctp_ss_remove_from_stream = sctp_ss_default_remove,
	.sctp_ss_select_stream = sctp_ss_rrp_select,
	.sctp_ss_scheduled = sctp_ss_default_scheduled,
	.sctp_ss_packet_done = sctp_ss_rrp_packet_done,
	.sctp_ss_get_value = sctp_ss_default_get_value,
	.sctp_ss_set_value = sctp_ss_default_set_value
#endif
},

{
#if defined(__Windows__) || defined(__Userspace_os_Windows)
	sctp_ss_default_init,
	sctp_ss_prio_clear,
	sctp_ss_prio_init_stream,
	sctp_ss_prio_add,
	sctp_ss_default_is_empty,
	sctp_ss_prio_remove,
	sctp_ss_prio_select,
	sctp_ss_default_scheduled,
	sctp_ss_default_packet_done,
	sctp_ss_prio_get_value,
	sctp_ss_prio_set_value
#else
	.sctp_ss_init = sctp_ss_default_init,
	.sctp_ss_clear = sctp_ss_prio_clear,
	.sctp_ss_init_stream = sctp_ss_prio_init_stream,
	.sctp_ss_add_to_stream = sctp_ss_prio_add,
	.sctp_ss_is_empty = sctp_ss_default_is_empty,
	.sctp_ss_remove_from_stream = sctp_ss_prio_remove,
	.sctp_ss_select_stream = sctp_ss_prio_select,
	.sctp_ss_scheduled = sctp_ss_default_scheduled,
	.sctp_ss_packet_done = sctp_ss_default_packet_done,
	.sctp_ss_get_value = sctp_ss_prio_get_value,
	.sctp_ss_set_value = sctp_ss_prio_set_value
#endif
},

{
#if defined(__Windows__) || defined(__Userspace_os_Windows)
	sctp_ss_default_init,
	sctp_ss_fb_clear,
	sctp_ss_fb_init_stream,
	sctp_ss_fb_add,
	sctp_ss_default_is_empty,
	sctp_ss_fb_remove,
	sctp_ss_fb_select,
	sctp_ss_fb_scheduled,
	sctp_ss_default_packet_done,
	sctp_ss_default_get_value,
	sctp_ss_default_set_value
#else
	.sctp_ss_init = sctp_ss_default_init,
	.sctp_ss_clear = sctp_ss_fb_clear,
	.sctp_ss_init_stream = sctp_ss_fb_init_stream,
	.sctp_ss_add_to_stream = sctp_ss_fb_add,
	.sctp_ss_is_empty = sctp_ss_default_is_empty,
	.sctp_ss_remove_from_stream = sctp_ss_fb_remove,
	.sctp_ss_select_stream = sctp_ss_fb_select,
	.sctp_ss_scheduled = sctp_ss_fb_scheduled,
	.sctp_ss_packet_done = sctp_ss_default_packet_done,
	.sctp_ss_get_value = sctp_ss_default_get_value,
	.sctp_ss_set_value = sctp_ss_default_set_value
#endif
},

{
#if defined(__Windows__) || defined(__Userspace_os_Windows)
	sctp_ss_fcfs_init,
	sctp_ss_fcfs_clear,
	sctp_ss_fcfs_init_stream,
	sctp_ss_fcfs_add,
	sctp_ss_fcfs_is_empty,
	sctp_ss_fcfs_remove,
	sctp_ss_fcfs_select,
	sctp_ss_default_scheduled,
	sctp_ss_default_packet_done,
	sctp_ss_default_get_value,
	sctp_ss_default_set_value
#else
	.sctp_ss_init = sctp_ss_fcfs_init,
	.sctp_ss_clear = sctp_ss_fcfs_clear,
	.sctp_ss_init_stream = sctp_ss_fcfs_init_stream,
	.sctp_ss_add_to_stream = sctp_ss_fcfs_add,
	.sctp_ss_is_empty = sctp_ss_fcfs_is_empty,
	.sctp_ss_remove_from_stream = sctp_ss_fcfs_remove,
	.sctp_ss_select_stream = sctp_ss_fcfs_select,
	.sctp_ss_scheduled = sctp_ss_default_scheduled,
	.sctp_ss_packet_done = sctp_ss_default_packet_done,
	.sctp_ss_get_value = sctp_ss_default_get_value,
	.sctp_ss_set_value = sctp_ss_default_set_value
#endif
}
};
