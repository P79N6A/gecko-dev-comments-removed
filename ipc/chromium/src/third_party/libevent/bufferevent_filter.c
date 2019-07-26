



























#include <sys/types.h>

#include "event2/event-config.h"

#ifdef _EVENT_HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _EVENT_HAVE_STDARG_H
#include <stdarg.h>
#endif

#ifdef WIN32
#include <winsock2.h>
#endif

#include "event2/util.h"
#include "event2/bufferevent.h"
#include "event2/buffer.h"
#include "event2/bufferevent_struct.h"
#include "event2/event.h"
#include "log-internal.h"
#include "mm-internal.h"
#include "bufferevent-internal.h"
#include "util-internal.h"


static int be_filter_enable(struct bufferevent *, short);
static int be_filter_disable(struct bufferevent *, short);
static void be_filter_destruct(struct bufferevent *);

static void be_filter_readcb(struct bufferevent *, void *);
static void be_filter_writecb(struct bufferevent *, void *);
static void be_filter_eventcb(struct bufferevent *, short, void *);
static int be_filter_flush(struct bufferevent *bufev,
    short iotype, enum bufferevent_flush_mode mode);
static int be_filter_ctrl(struct bufferevent *, enum bufferevent_ctrl_op, union bufferevent_ctrl_data *);

static void bufferevent_filtered_outbuf_cb(struct evbuffer *buf,
    const struct evbuffer_cb_info *info, void *arg);

struct bufferevent_filtered {
	struct bufferevent_private bev;

	
	struct bufferevent *underlying;
	
	struct evbuffer_cb_entry *outbuf_cb;
	

	unsigned got_eof;

	
	void (*free_context)(void *);
	
	bufferevent_filter_cb process_in;
	
	bufferevent_filter_cb process_out;
	
	void *context;
};

const struct bufferevent_ops bufferevent_ops_filter = {
	"filter",
	evutil_offsetof(struct bufferevent_filtered, bev.bev),
	be_filter_enable,
	be_filter_disable,
	be_filter_destruct,
	_bufferevent_generic_adj_timeouts,
	be_filter_flush,
	be_filter_ctrl,
};



static inline struct bufferevent_filtered *
upcast(struct bufferevent *bev)
{
	struct bufferevent_filtered *bev_f;
	if (bev->be_ops != &bufferevent_ops_filter)
		return NULL;
	bev_f = (void*)( ((char*)bev) -
			 evutil_offsetof(struct bufferevent_filtered, bev.bev));
	EVUTIL_ASSERT(bev_f->bev.bev.be_ops == &bufferevent_ops_filter);
	return bev_f;
}

#define downcast(bev_f) (&(bev_f)->bev.bev)




static int
be_underlying_writebuf_full(struct bufferevent_filtered *bevf,
    enum bufferevent_flush_mode state)
{
	struct bufferevent *u = bevf->underlying;
	return state == BEV_NORMAL &&
	    u->wm_write.high &&
	    evbuffer_get_length(u->output) >= u->wm_write.high;
}



static int
be_readbuf_full(struct bufferevent_filtered *bevf,
    enum bufferevent_flush_mode state)
{
	struct bufferevent *bufev = downcast(bevf);
	return state == BEV_NORMAL &&
	    bufev->wm_read.high &&
	    evbuffer_get_length(bufev->input) >= bufev->wm_read.high;
}



static enum bufferevent_filter_result
be_null_filter(struct evbuffer *src, struct evbuffer *dst, ev_ssize_t lim,
	       enum bufferevent_flush_mode state, void *ctx)
{
	(void)state;
	if (evbuffer_remove_buffer(src, dst, lim) == 0)
		return BEV_OK;
	else
		return BEV_ERROR;
}

struct bufferevent *
bufferevent_filter_new(struct bufferevent *underlying,
		       bufferevent_filter_cb input_filter,
		       bufferevent_filter_cb output_filter,
		       int options,
		       void (*free_context)(void *),
		       void *ctx)
{
	struct bufferevent_filtered *bufev_f;
	int tmp_options = options & ~BEV_OPT_THREADSAFE;

	if (!underlying)
		return NULL;

	if (!input_filter)
		input_filter = be_null_filter;
	if (!output_filter)
		output_filter = be_null_filter;

	bufev_f = mm_calloc(1, sizeof(struct bufferevent_filtered));
	if (!bufev_f)
		return NULL;

	if (bufferevent_init_common(&bufev_f->bev, underlying->ev_base,
				    &bufferevent_ops_filter, tmp_options) < 0) {
		mm_free(bufev_f);
		return NULL;
	}
	if (options & BEV_OPT_THREADSAFE) {
		bufferevent_enable_locking(downcast(bufev_f), NULL);
	}

	bufev_f->underlying = underlying;

	bufev_f->process_in = input_filter;
	bufev_f->process_out = output_filter;
	bufev_f->free_context = free_context;
	bufev_f->context = ctx;

	bufferevent_setcb(bufev_f->underlying,
	    be_filter_readcb, be_filter_writecb, be_filter_eventcb, bufev_f);

	bufev_f->outbuf_cb = evbuffer_add_cb(downcast(bufev_f)->output,
	   bufferevent_filtered_outbuf_cb, bufev_f);

	_bufferevent_init_generic_timeout_cbs(downcast(bufev_f));
	bufferevent_incref(underlying);

	bufferevent_enable(underlying, EV_READ|EV_WRITE);
	bufferevent_suspend_read(underlying, BEV_SUSPEND_FILT_READ);

	return downcast(bufev_f);
}

static void
be_filter_destruct(struct bufferevent *bev)
{
	struct bufferevent_filtered *bevf = upcast(bev);
	EVUTIL_ASSERT(bevf);
	if (bevf->free_context)
		bevf->free_context(bevf->context);

	if (bevf->bev.options & BEV_OPT_CLOSE_ON_FREE) {
		




		if (BEV_UPCAST(bevf->underlying)->refcnt < 2) {
			event_warnx("BEV_OPT_CLOSE_ON_FREE set on an "
			    "bufferevent with too few references");
		} else {
			bufferevent_free(bevf->underlying);
		}
	} else {
		if (bevf->underlying) {
			if (bevf->underlying->errorcb == be_filter_eventcb)
				bufferevent_setcb(bevf->underlying,
				    NULL, NULL, NULL, NULL);
			bufferevent_unsuspend_read(bevf->underlying,
			    BEV_SUSPEND_FILT_READ);
		}
	}

	_bufferevent_del_generic_timeout_cbs(bev);
}

static int
be_filter_enable(struct bufferevent *bev, short event)
{
	struct bufferevent_filtered *bevf = upcast(bev);
	if (event & EV_WRITE)
		BEV_RESET_GENERIC_WRITE_TIMEOUT(bev);

	if (event & EV_READ) {
		BEV_RESET_GENERIC_READ_TIMEOUT(bev);
		bufferevent_unsuspend_read(bevf->underlying,
		    BEV_SUSPEND_FILT_READ);
	}
	return 0;
}

static int
be_filter_disable(struct bufferevent *bev, short event)
{
	struct bufferevent_filtered *bevf = upcast(bev);
	if (event & EV_WRITE)
		BEV_DEL_GENERIC_WRITE_TIMEOUT(bev);
	if (event & EV_READ) {
		BEV_DEL_GENERIC_READ_TIMEOUT(bev);
		bufferevent_suspend_read(bevf->underlying,
		    BEV_SUSPEND_FILT_READ);
	}
	return 0;
}

static enum bufferevent_filter_result
be_filter_process_input(struct bufferevent_filtered *bevf,
			enum bufferevent_flush_mode state,
			int *processed_out)
{
	enum bufferevent_filter_result res;
	struct bufferevent *bev = downcast(bevf);

	if (state == BEV_NORMAL) {
		

		if (!(bev->enabled & EV_READ) ||
		    be_readbuf_full(bevf, state))
			return BEV_OK;
	}

	do {
		ev_ssize_t limit = -1;
		if (state == BEV_NORMAL && bev->wm_read.high)
			limit = bev->wm_read.high -
			    evbuffer_get_length(bev->input);

		res = bevf->process_in(bevf->underlying->input,
		    bev->input, limit, state, bevf->context);

		if (res == BEV_OK)
			*processed_out = 1;
	} while (res == BEV_OK &&
		 (bev->enabled & EV_READ) &&
		 evbuffer_get_length(bevf->underlying->input) &&
		 !be_readbuf_full(bevf, state));

	if (*processed_out)
		BEV_RESET_GENERIC_READ_TIMEOUT(bev);

	return res;
}


static enum bufferevent_filter_result
be_filter_process_output(struct bufferevent_filtered *bevf,
			 enum bufferevent_flush_mode state,
			 int *processed_out)
{
	
	enum bufferevent_filter_result res = BEV_OK;
	struct bufferevent *bufev = downcast(bevf);
	int again = 0;

	if (state == BEV_NORMAL) {
		




		if (!(bufev->enabled & EV_WRITE) ||
		    be_underlying_writebuf_full(bevf, state) ||
		    !evbuffer_get_length(bufev->output))
			return BEV_OK;
	}

	

	evbuffer_cb_set_flags(bufev->output, bevf->outbuf_cb, 0);

	do {
		int processed = 0;
		again = 0;

		do {
			ev_ssize_t limit = -1;
			if (state == BEV_NORMAL &&
			    bevf->underlying->wm_write.high)
				limit = bevf->underlying->wm_write.high -
				    evbuffer_get_length(bevf->underlying->output);

			res = bevf->process_out(downcast(bevf)->output,
			    bevf->underlying->output,
			    limit,
			    state,
			    bevf->context);

			if (res == BEV_OK)
				processed = *processed_out = 1;
		} while (
			res == BEV_OK &&
			
			(bufev->enabled & EV_WRITE) &&
			

			evbuffer_get_length(bufev->output) &&
			
			!be_underlying_writebuf_full(bevf,state));

		if (processed &&
		    evbuffer_get_length(bufev->output) <= bufev->wm_write.low) {
			
			_bufferevent_run_writecb(bufev);

			if (res == BEV_OK &&
			    (bufev->enabled & EV_WRITE) &&
			    evbuffer_get_length(bufev->output) &&
			    !be_underlying_writebuf_full(bevf, state)) {
				again = 1;
			}
		}
	} while (again);

	
	evbuffer_cb_set_flags(bufev->output,bevf->outbuf_cb,
	    EVBUFFER_CB_ENABLED);

	if (*processed_out)
		BEV_RESET_GENERIC_WRITE_TIMEOUT(bufev);

	return res;
}


static void
bufferevent_filtered_outbuf_cb(struct evbuffer *buf,
    const struct evbuffer_cb_info *cbinfo, void *arg)
{
	struct bufferevent_filtered *bevf = arg;
	struct bufferevent *bev = downcast(bevf);

	if (cbinfo->n_added) {
		int processed_any = 0;
		

		_bufferevent_incref_and_lock(bev);
		be_filter_process_output(bevf, BEV_NORMAL, &processed_any);
		_bufferevent_decref_and_unlock(bev);
	}
}


static void
be_filter_readcb(struct bufferevent *underlying, void *_me)
{
	struct bufferevent_filtered *bevf = _me;
	enum bufferevent_filter_result res;
	enum bufferevent_flush_mode state;
	struct bufferevent *bufev = downcast(bevf);
	int processed_any = 0;

	_bufferevent_incref_and_lock(bufev);

	if (bevf->got_eof)
		state = BEV_FINISHED;
	else
		state = BEV_NORMAL;

	
	res = be_filter_process_input(bevf, state, &processed_any);
	(void)res;

	


	if (processed_any &&
	    evbuffer_get_length(bufev->input) >= bufev->wm_read.low)
		_bufferevent_run_readcb(bufev);

	_bufferevent_decref_and_unlock(bufev);
}



static void
be_filter_writecb(struct bufferevent *underlying, void *_me)
{
	struct bufferevent_filtered *bevf = _me;
	struct bufferevent *bev = downcast(bevf);
	int processed_any = 0;

	_bufferevent_incref_and_lock(bev);
	be_filter_process_output(bevf, BEV_NORMAL, &processed_any);
	_bufferevent_decref_and_unlock(bev);
}


static void
be_filter_eventcb(struct bufferevent *underlying, short what, void *_me)
{
	struct bufferevent_filtered *bevf = _me;
	struct bufferevent *bev = downcast(bevf);

	_bufferevent_incref_and_lock(bev);
	
	_bufferevent_run_eventcb(bev, what);
	_bufferevent_decref_and_unlock(bev);
}

static int
be_filter_flush(struct bufferevent *bufev,
    short iotype, enum bufferevent_flush_mode mode)
{
	struct bufferevent_filtered *bevf = upcast(bufev);
	int processed_any = 0;
	EVUTIL_ASSERT(bevf);

	_bufferevent_incref_and_lock(bufev);

	if (iotype & EV_READ) {
		be_filter_process_input(bevf, mode, &processed_any);
	}
	if (iotype & EV_WRITE) {
		be_filter_process_output(bevf, mode, &processed_any);
	}
	
	
	bufferevent_flush(bevf->underlying, iotype, mode);

	_bufferevent_decref_and_unlock(bufev);

	return processed_any;
}

static int
be_filter_ctrl(struct bufferevent *bev, enum bufferevent_ctrl_op op,
    union bufferevent_ctrl_data *data)
{
	struct bufferevent_filtered *bevf;
	switch (op) {
	case BEV_CTRL_GET_UNDERLYING:
		bevf = upcast(bev);
		data->ptr = bevf->underlying;
		return 0;
	case BEV_CTRL_GET_FD:
	case BEV_CTRL_SET_FD:
	case BEV_CTRL_CANCEL_ALL:
	default:
		return -1;
	}
}
