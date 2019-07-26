

























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
#ifdef _EVENT_HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef WIN32
#include <winsock2.h>
#endif

#include "event2/bufferevent.h"
#include "event2/bufferevent_struct.h"
#include "event2/bufferevent_ssl.h"
#include "event2/buffer.h"
#include "event2/event.h"

#include "mm-internal.h"
#include "bufferevent-internal.h"
#include "log-internal.h"

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
















#define BIO_TYPE_LIBEVENT 57



#if 0
static void
print_err(int val)
{
	int err;
	printf("Error was %d\n", val);

	while ((err = ERR_get_error())) {
		const char *msg = (const char*)ERR_reason_error_string(err);
		const char *lib = (const char*)ERR_lib_error_string(err);
		const char *func = (const char*)ERR_func_error_string(err);

		printf("%s in %s %s\n", msg, lib, func);
	}
}
#else
#define print_err(v) ((void)0)
#endif


static int
bio_bufferevent_new(BIO *b)
{
	b->init = 0;
	b->num = -1;
	b->ptr = NULL; 
	b->flags = 0;
	return 1;
}


static int
bio_bufferevent_free(BIO *b)
{
	if (!b)
		return 0;
	if (b->shutdown) {
		if (b->init && b->ptr)
			bufferevent_free(b->ptr);
		b->init = 0;
		b->flags = 0;
		b->ptr = NULL;
	}
	return 1;
}


static int
bio_bufferevent_read(BIO *b, char *out, int outlen)
{
	int r = 0;
	struct evbuffer *input;

	BIO_clear_retry_flags(b);

	if (!out)
		return 0;
	if (!b->ptr)
		return -1;

	input = bufferevent_get_input(b->ptr);
	if (evbuffer_get_length(input) == 0) {
		
		BIO_set_retry_read(b);
		return -1;
	} else {
		r = evbuffer_remove(input, out, outlen);
	}

	return r;
}


static int
bio_bufferevent_write(BIO *b, const char *in, int inlen)
{
	struct bufferevent *bufev = b->ptr;
	struct evbuffer *output;
	size_t outlen;

	BIO_clear_retry_flags(b);

	if (!b->ptr)
		return -1;

	output = bufferevent_get_output(bufev);
	outlen = evbuffer_get_length(output);

	

	if (bufev->wm_write.high && bufev->wm_write.high <= (outlen+inlen)) {
		if (bufev->wm_write.high <= outlen) {
			
			BIO_set_retry_write(b);
			return -1;
		}
		inlen = bufev->wm_write.high - outlen;
	}

	EVUTIL_ASSERT(inlen > 0);
	evbuffer_add(output, in, inlen);
	return inlen;
}


static long
bio_bufferevent_ctrl(BIO *b, int cmd, long num, void *ptr)
{
	struct bufferevent *bufev = b->ptr;
	long ret = 1;

	switch (cmd) {
	case BIO_CTRL_GET_CLOSE:
		ret = b->shutdown;
		break;
	case BIO_CTRL_SET_CLOSE:
		b->shutdown = (int)num;
		break;
	case BIO_CTRL_PENDING:
		ret = evbuffer_get_length(bufferevent_get_input(bufev)) != 0;
		break;
	case BIO_CTRL_WPENDING:
		ret = evbuffer_get_length(bufferevent_get_output(bufev)) != 0;
		break;
	

	case BIO_CTRL_DUP:
	case BIO_CTRL_FLUSH:
		ret = 1;
		break;
	default:
		ret = 0;
		break;
	}
	return ret;
}


static int
bio_bufferevent_puts(BIO *b, const char *s)
{
	return bio_bufferevent_write(b, s, strlen(s));
}


static BIO_METHOD methods_bufferevent = {
	BIO_TYPE_LIBEVENT, "bufferevent",
	bio_bufferevent_write,
	bio_bufferevent_read,
	bio_bufferevent_puts,
	NULL ,
	bio_bufferevent_ctrl,
	bio_bufferevent_new,
	bio_bufferevent_free,
	NULL ,
};


static BIO_METHOD *
BIO_s_bufferevent(void)
{
	return &methods_bufferevent;
}



static BIO *
BIO_new_bufferevent(struct bufferevent *bufferevent, int close_flag)
{
	BIO *result;
	if (!bufferevent)
		return NULL;
	if (!(result = BIO_new(BIO_s_bufferevent())))
		return NULL;
	result->init = 1;
	result->ptr = bufferevent;
	result->shutdown = close_flag ? 1 : 0;
	return result;
}














struct bio_data_counts {
	unsigned long n_written;
	unsigned long n_read;
};

struct bufferevent_openssl {
	




	struct bufferevent_private bev;
	

	struct bufferevent *underlying;
	
	SSL *ssl;

	

	struct evbuffer_cb_entry *outbuf_cb;

	

	struct bio_data_counts counts;

	

	ev_ssize_t last_write;

#define NUM_ERRORS 3
	ev_uint32_t errors[NUM_ERRORS];

	


	unsigned read_blocked_on_write : 1;
	
	unsigned write_blocked_on_read : 1;
	
	unsigned allow_dirty_shutdown : 1;
	
	unsigned fd_is_set : 1;
	
	unsigned n_errors : 2;

	
	unsigned state : 2;
};

static int be_openssl_enable(struct bufferevent *, short);
static int be_openssl_disable(struct bufferevent *, short);
static void be_openssl_destruct(struct bufferevent *);
static int be_openssl_adj_timeouts(struct bufferevent *);
static int be_openssl_flush(struct bufferevent *bufev,
    short iotype, enum bufferevent_flush_mode mode);
static int be_openssl_ctrl(struct bufferevent *, enum bufferevent_ctrl_op, union bufferevent_ctrl_data *);

const struct bufferevent_ops bufferevent_ops_openssl = {
	"ssl",
	evutil_offsetof(struct bufferevent_openssl, bev.bev),
	be_openssl_enable,
	be_openssl_disable,
	be_openssl_destruct,
	be_openssl_adj_timeouts,
	be_openssl_flush,
	be_openssl_ctrl,
};



static inline struct bufferevent_openssl *
upcast(struct bufferevent *bev)
{
	struct bufferevent_openssl *bev_o;
	if (bev->be_ops != &bufferevent_ops_openssl)
		return NULL;
	bev_o = (void*)( ((char*)bev) -
			 evutil_offsetof(struct bufferevent_openssl, bev.bev));
	EVUTIL_ASSERT(bev_o->bev.bev.be_ops == &bufferevent_ops_openssl);
	return bev_o;
}

static inline void
put_error(struct bufferevent_openssl *bev_ssl, unsigned long err)
{
	if (bev_ssl->n_errors == NUM_ERRORS)
		return;
	




	bev_ssl->errors[bev_ssl->n_errors++] = (ev_uint32_t) err;
}




static int
start_reading(struct bufferevent_openssl *bev_ssl)
{
	if (bev_ssl->underlying) {
		bufferevent_unsuspend_read(bev_ssl->underlying,
		    BEV_SUSPEND_FILT_READ);
		return 0;
	} else {
		struct bufferevent *bev = &bev_ssl->bev.bev;
		int r;
		r = _bufferevent_add_event(&bev->ev_read, &bev->timeout_read);
		if (r == 0 && bev_ssl->read_blocked_on_write)
			r = _bufferevent_add_event(&bev->ev_write,
			    &bev->timeout_write);
		return r;
	}
}




static int
start_writing(struct bufferevent_openssl *bev_ssl)
{
	int r = 0;
	if (bev_ssl->underlying) {
		;
	} else {
		struct bufferevent *bev = &bev_ssl->bev.bev;
		r = _bufferevent_add_event(&bev->ev_write, &bev->timeout_write);
		if (!r && bev_ssl->write_blocked_on_read)
			r = _bufferevent_add_event(&bev->ev_read,
			    &bev->timeout_read);
	}
	return r;
}

static void
stop_reading(struct bufferevent_openssl *bev_ssl)
{
	if (bev_ssl->write_blocked_on_read)
		return;
	if (bev_ssl->underlying) {
		bufferevent_suspend_read(bev_ssl->underlying,
		    BEV_SUSPEND_FILT_READ);
	} else {
		struct bufferevent *bev = &bev_ssl->bev.bev;
		event_del(&bev->ev_read);
	}
}

static void
stop_writing(struct bufferevent_openssl *bev_ssl)
{
	if (bev_ssl->read_blocked_on_write)
		return;
	if (bev_ssl->underlying) {
		;
	} else {
		struct bufferevent *bev = &bev_ssl->bev.bev;
		event_del(&bev->ev_write);
	}
}

static int
set_rbow(struct bufferevent_openssl *bev_ssl)
{
	if (!bev_ssl->underlying)
		stop_reading(bev_ssl);
	bev_ssl->read_blocked_on_write = 1;
	return start_writing(bev_ssl);
}

static int
set_wbor(struct bufferevent_openssl *bev_ssl)
{
	if (!bev_ssl->underlying)
		stop_writing(bev_ssl);
	bev_ssl->write_blocked_on_read = 1;
	return start_reading(bev_ssl);
}

static int
clear_rbow(struct bufferevent_openssl *bev_ssl)
{
	struct bufferevent *bev = &bev_ssl->bev.bev;
	int r = 0;
	bev_ssl->read_blocked_on_write = 0;
	if (!(bev->enabled & EV_WRITE))
		stop_writing(bev_ssl);
	if (bev->enabled & EV_READ)
		r = start_reading(bev_ssl);
	return r;
}


static int
clear_wbor(struct bufferevent_openssl *bev_ssl)
{
	struct bufferevent *bev = &bev_ssl->bev.bev;
	int r = 0;
	bev_ssl->write_blocked_on_read = 0;
	if (!(bev->enabled & EV_READ))
		stop_reading(bev_ssl);
	if (bev->enabled & EV_WRITE)
		r = start_writing(bev_ssl);
	return r;
}

static void
conn_closed(struct bufferevent_openssl *bev_ssl, int errcode, int ret)
{
	int event = BEV_EVENT_ERROR;
	int dirty_shutdown = 0;
	unsigned long err;

	switch (errcode) {
	case SSL_ERROR_ZERO_RETURN:
		
		if (SSL_get_shutdown(bev_ssl->ssl) & SSL_RECEIVED_SHUTDOWN)
			event = BEV_EVENT_EOF;
		else
			dirty_shutdown = 1;
		break;
	case SSL_ERROR_SYSCALL:
		
		if (ret == 0 && ERR_peek_error() == 0)
			dirty_shutdown = 1;
		break;
	case SSL_ERROR_SSL:
		
		break;
	case SSL_ERROR_WANT_X509_LOOKUP:
		
		break;
	case SSL_ERROR_NONE:
	case SSL_ERROR_WANT_READ:
	case SSL_ERROR_WANT_WRITE:
	case SSL_ERROR_WANT_CONNECT:
	case SSL_ERROR_WANT_ACCEPT:
	default:
		
		event_warnx("BUG: Unexpected OpenSSL error code %d", errcode);
		break;
	}

	while ((err = ERR_get_error())) {
		put_error(bev_ssl, err);
	}

	if (dirty_shutdown && bev_ssl->allow_dirty_shutdown)
		event = BEV_EVENT_EOF;

	stop_reading(bev_ssl);
	stop_writing(bev_ssl);

	_bufferevent_run_eventcb(&bev_ssl->bev.bev, event);
}

static void
init_bio_counts(struct bufferevent_openssl *bev_ssl)
{
	bev_ssl->counts.n_written =
	    BIO_number_written(SSL_get_wbio(bev_ssl->ssl));
	bev_ssl->counts.n_read =
	    BIO_number_read(SSL_get_rbio(bev_ssl->ssl));
}

static inline void
decrement_buckets(struct bufferevent_openssl *bev_ssl)
{
	unsigned long num_w = BIO_number_written(SSL_get_wbio(bev_ssl->ssl));
	unsigned long num_r = BIO_number_read(SSL_get_rbio(bev_ssl->ssl));
	
	unsigned long w = num_w - bev_ssl->counts.n_written;
	unsigned long r = num_r - bev_ssl->counts.n_read;
	if (w)
		_bufferevent_decrement_write_buckets(&bev_ssl->bev, w);
	if (r)
		_bufferevent_decrement_read_buckets(&bev_ssl->bev, r);
	bev_ssl->counts.n_written = num_w;
	bev_ssl->counts.n_read = num_r;
}

#define OP_MADE_PROGRESS 1
#define OP_BLOCKED 2
#define OP_ERR 4



static int
do_read(struct bufferevent_openssl *bev_ssl, int n_to_read) {
	
	struct bufferevent *bev = &bev_ssl->bev.bev;
	struct evbuffer *input = bev->input;
	int r, n, i, n_used = 0, atmost;
	struct evbuffer_iovec space[2];
	int result = 0;

	if (bev_ssl->bev.read_suspended)
		return 0;

	atmost = _bufferevent_get_read_max(&bev_ssl->bev);
	if (n_to_read > atmost)
		n_to_read = atmost;

	n = evbuffer_reserve_space(input, n_to_read, space, 2);
	if (n < 0)
		return OP_ERR;

	for (i=0; i<n; ++i) {
		if (bev_ssl->bev.read_suspended)
			break;
		r = SSL_read(bev_ssl->ssl, space[i].iov_base, space[i].iov_len);
		if (r>0) {
			result |= OP_MADE_PROGRESS;
			if (bev_ssl->read_blocked_on_write)
				if (clear_rbow(bev_ssl) < 0)
					return OP_ERR | result;
			++n_used;
			space[i].iov_len = r;
			decrement_buckets(bev_ssl);
		} else {
			int err = SSL_get_error(bev_ssl->ssl, r);
			print_err(err);
			switch (err) {
			case SSL_ERROR_WANT_READ:
				
				if (bev_ssl->read_blocked_on_write)
					if (clear_rbow(bev_ssl) < 0)
						return OP_ERR | result;
				break;
			case SSL_ERROR_WANT_WRITE:
				

				if (!bev_ssl->read_blocked_on_write)
					if (set_rbow(bev_ssl) < 0)
						return OP_ERR | result;
				break;
			default:
				conn_closed(bev_ssl, err, r);
				break;
			}
			result |= OP_BLOCKED;
			break; 
		}
	}

	if (n_used) {
		evbuffer_commit_space(input, space, n_used);
		if (bev_ssl->underlying)
			BEV_RESET_GENERIC_READ_TIMEOUT(bev);
	}

	return result;
}



static int
do_write(struct bufferevent_openssl *bev_ssl, int atmost)
{
	int i, r, n, n_written = 0;
	struct bufferevent *bev = &bev_ssl->bev.bev;
	struct evbuffer *output = bev->output;
	struct evbuffer_iovec space[8];
	int result = 0;

	if (bev_ssl->last_write > 0)
		atmost = bev_ssl->last_write;
	else
		atmost = _bufferevent_get_write_max(&bev_ssl->bev);

	n = evbuffer_peek(output, atmost, NULL, space, 8);
	if (n < 0)
		return OP_ERR | result;

	if (n > 8)
		n = 8;
	for (i=0; i < n; ++i) {
		if (bev_ssl->bev.write_suspended)
			break;

		


		if (space[i].iov_len == 0)
			continue;

		r = SSL_write(bev_ssl->ssl, space[i].iov_base,
		    space[i].iov_len);
		if (r > 0) {
			result |= OP_MADE_PROGRESS;
			if (bev_ssl->write_blocked_on_read)
				if (clear_wbor(bev_ssl) < 0)
					return OP_ERR | result;
			n_written += r;
			bev_ssl->last_write = -1;
			decrement_buckets(bev_ssl);
		} else {
			int err = SSL_get_error(bev_ssl->ssl, r);
			print_err(err);
			switch (err) {
			case SSL_ERROR_WANT_WRITE:
				
				if (bev_ssl->write_blocked_on_read)
					if (clear_wbor(bev_ssl) < 0)
						return OP_ERR | result;
				bev_ssl->last_write = space[i].iov_len;
				break;
			case SSL_ERROR_WANT_READ:
				

				if (!bev_ssl->write_blocked_on_read)
					if (set_wbor(bev_ssl) < 0)
						return OP_ERR | result;
				bev_ssl->last_write = space[i].iov_len;
				break;
			default:
				conn_closed(bev_ssl, err, r);
				bev_ssl->last_write = -1;
				break;
			}
			result |= OP_BLOCKED;
			break;
		}
	}
	if (n_written) {
		evbuffer_drain(output, n_written);
		if (bev_ssl->underlying)
			BEV_RESET_GENERIC_WRITE_TIMEOUT(bev);

		if (evbuffer_get_length(output) <= bev->wm_write.low)
			_bufferevent_run_writecb(bev);
	}
	return result;
}

#define WRITE_FRAME 15000

#define READ_DEFAULT 4096



static int
bytes_to_read(struct bufferevent_openssl *bev)
{
	struct evbuffer *input = bev->bev.bev.input;
	struct event_watermark *wm = &bev->bev.bev.wm_read;
	int result = READ_DEFAULT;
	ev_ssize_t limit;
	


	if (bev->write_blocked_on_read) {
		return 0;
	}

	if (! (bev->bev.bev.enabled & EV_READ)) {
		return 0;
	}

	if (bev->bev.read_suspended) {
		return 0;
	}

	if (wm->high) {
		if (evbuffer_get_length(input) >= wm->high) {
			return 0;
		}

		result = wm->high - evbuffer_get_length(input);
	} else {
		result = READ_DEFAULT;
	}

	
	limit = _bufferevent_get_read_max(&bev->bev);
	if (result > limit) {
		result = limit;
	}

	return result;
}






static void
consider_reading(struct bufferevent_openssl *bev_ssl)
{
	int r;
	int n_to_read;
	int all_result_flags = 0;

	while (bev_ssl->write_blocked_on_read) {
		r = do_write(bev_ssl, WRITE_FRAME);
		if (r & (OP_BLOCKED|OP_ERR))
			break;
	}
	if (bev_ssl->write_blocked_on_read)
		return;

	n_to_read = bytes_to_read(bev_ssl);

	while (n_to_read) {
		r = do_read(bev_ssl, n_to_read);
		all_result_flags |= r;

		if (r & (OP_BLOCKED|OP_ERR))
			break;

		if (bev_ssl->bev.read_suspended)
			break;
        
		







		n_to_read = SSL_pending(bev_ssl->ssl);

		













		if (!n_to_read && bev_ssl->underlying)
			n_to_read = bytes_to_read(bev_ssl);
	}

	if (all_result_flags & OP_MADE_PROGRESS) {
		struct bufferevent *bev = &bev_ssl->bev.bev;
		struct evbuffer *input = bev->input;

		if (evbuffer_get_length(input) >= bev->wm_read.low) {
			_bufferevent_run_readcb(bev);
		}
	}

	if (!bev_ssl->underlying) {
		
		if (bev_ssl->bev.read_suspended ||
		    !(bev_ssl->bev.bev.enabled & EV_READ)) {
			event_del(&bev_ssl->bev.bev.ev_read);
		}
	}
}

static void
consider_writing(struct bufferevent_openssl *bev_ssl)
{
	int r;
	struct evbuffer *output = bev_ssl->bev.bev.output;
	struct evbuffer *target = NULL;
	struct event_watermark *wm = NULL;

	while (bev_ssl->read_blocked_on_write) {
		r = do_read(bev_ssl, 1024); 
		if (r & OP_MADE_PROGRESS) {
			struct bufferevent *bev = &bev_ssl->bev.bev;
			struct evbuffer *input = bev->input;

			if (evbuffer_get_length(input) >= bev->wm_read.low) {
				_bufferevent_run_readcb(bev);
			}
		}
		if (r & (OP_ERR|OP_BLOCKED))
			break;
	}
	if (bev_ssl->read_blocked_on_write)
		return;
	if (bev_ssl->underlying) {
		target = bev_ssl->underlying->output;
		wm = &bev_ssl->underlying->wm_write;
	}
	while ((bev_ssl->bev.bev.enabled & EV_WRITE) &&
	    (! bev_ssl->bev.write_suspended) &&
	    evbuffer_get_length(output) &&
	    (!target || (! wm->high || evbuffer_get_length(target) < wm->high))) {
		int n_to_write;
		if (wm && wm->high)
			n_to_write = wm->high - evbuffer_get_length(target);
		else
			n_to_write = WRITE_FRAME;
		r = do_write(bev_ssl, n_to_write);
		if (r & (OP_BLOCKED|OP_ERR))
			break;
	}

	if (!bev_ssl->underlying) {
		if (evbuffer_get_length(output) == 0) {
			event_del(&bev_ssl->bev.bev.ev_write);
		} else if (bev_ssl->bev.write_suspended ||
		    !(bev_ssl->bev.bev.enabled & EV_WRITE)) {
			
			event_del(&bev_ssl->bev.bev.ev_write);
		}
	}
}

static void
be_openssl_readcb(struct bufferevent *bev_base, void *ctx)
{
	struct bufferevent_openssl *bev_ssl = ctx;
	consider_reading(bev_ssl);
}

static void
be_openssl_writecb(struct bufferevent *bev_base, void *ctx)
{
	struct bufferevent_openssl *bev_ssl = ctx;
	consider_writing(bev_ssl);
}

static void
be_openssl_eventcb(struct bufferevent *bev_base, short what, void *ctx)
{
	struct bufferevent_openssl *bev_ssl = ctx;
	int event = 0;

	if (what & BEV_EVENT_EOF) {
		if (bev_ssl->allow_dirty_shutdown)
			event = BEV_EVENT_EOF;
		else
			event = BEV_EVENT_ERROR;
	} else if (what & BEV_EVENT_TIMEOUT) {
		
		event = what;
	} else if (what & BEV_EVENT_ERROR) {
		
		event = what;
	} else if (what & BEV_EVENT_CONNECTED) {
		

	}
	if (event)
		_bufferevent_run_eventcb(&bev_ssl->bev.bev, event);
}

static void
be_openssl_readeventcb(evutil_socket_t fd, short what, void *ptr)
{
	struct bufferevent_openssl *bev_ssl = ptr;
	_bufferevent_incref_and_lock(&bev_ssl->bev.bev);
	if (what == EV_TIMEOUT) {
		_bufferevent_run_eventcb(&bev_ssl->bev.bev,
		    BEV_EVENT_TIMEOUT|BEV_EVENT_READING);
	} else {
		consider_reading(bev_ssl);
	}
	_bufferevent_decref_and_unlock(&bev_ssl->bev.bev);
}

static void
be_openssl_writeeventcb(evutil_socket_t fd, short what, void *ptr)
{
	struct bufferevent_openssl *bev_ssl = ptr;
	_bufferevent_incref_and_lock(&bev_ssl->bev.bev);
	if (what == EV_TIMEOUT) {
		_bufferevent_run_eventcb(&bev_ssl->bev.bev,
		    BEV_EVENT_TIMEOUT|BEV_EVENT_WRITING);
	} else {
		consider_writing(bev_ssl);
	}
	_bufferevent_decref_and_unlock(&bev_ssl->bev.bev);
}

static int
set_open_callbacks(struct bufferevent_openssl *bev_ssl, evutil_socket_t fd)
{
	if (bev_ssl->underlying) {
		bufferevent_setcb(bev_ssl->underlying,
		    be_openssl_readcb, be_openssl_writecb, be_openssl_eventcb,
		    bev_ssl);
		return 0;
	} else {
		struct bufferevent *bev = &bev_ssl->bev.bev;
		int rpending=0, wpending=0, r1=0, r2=0;
		if (fd < 0 && bev_ssl->fd_is_set)
			fd = event_get_fd(&bev->ev_read);
		if (bev_ssl->fd_is_set) {
			rpending = event_pending(&bev->ev_read, EV_READ, NULL);
			wpending = event_pending(&bev->ev_write, EV_WRITE, NULL);
			event_del(&bev->ev_read);
			event_del(&bev->ev_write);
		}
		event_assign(&bev->ev_read, bev->ev_base, fd,
		    EV_READ|EV_PERSIST, be_openssl_readeventcb, bev_ssl);
		event_assign(&bev->ev_write, bev->ev_base, fd,
		    EV_WRITE|EV_PERSIST, be_openssl_writeeventcb, bev_ssl);
		if (rpending)
			r1 = _bufferevent_add_event(&bev->ev_read, &bev->timeout_read);
		if (wpending)
			r2 = _bufferevent_add_event(&bev->ev_write, &bev->timeout_write);
		if (fd >= 0) {
			bev_ssl->fd_is_set = 1;
		}
		return (r1<0 || r2<0) ? -1 : 0;
	}
}

static int
do_handshake(struct bufferevent_openssl *bev_ssl)
{
	int r;

	switch (bev_ssl->state) {
	default:
	case BUFFEREVENT_SSL_OPEN:
		EVUTIL_ASSERT(0);
		return -1;
	case BUFFEREVENT_SSL_CONNECTING:
	case BUFFEREVENT_SSL_ACCEPTING:
		r = SSL_do_handshake(bev_ssl->ssl);
		break;
	}
	decrement_buckets(bev_ssl);

	if (r==1) {
		
		bev_ssl->state = BUFFEREVENT_SSL_OPEN;
		set_open_callbacks(bev_ssl, -1); 
		
		bufferevent_enable(&bev_ssl->bev.bev, bev_ssl->bev.bev.enabled);
		_bufferevent_run_eventcb(&bev_ssl->bev.bev,
		    BEV_EVENT_CONNECTED);
		return 1;
	} else {
		int err = SSL_get_error(bev_ssl->ssl, r);
		print_err(err);
		switch (err) {
		case SSL_ERROR_WANT_WRITE:
			if (!bev_ssl->underlying) {
				stop_reading(bev_ssl);
				return start_writing(bev_ssl);
			}
			return 0;
		case SSL_ERROR_WANT_READ:
			if (!bev_ssl->underlying) {
				stop_writing(bev_ssl);
				return start_reading(bev_ssl);
			}
			return 0;
		default:
			conn_closed(bev_ssl, err, r);
			return -1;
		}
	}
}

static void
be_openssl_handshakecb(struct bufferevent *bev_base, void *ctx)
{
	struct bufferevent_openssl *bev_ssl = ctx;
	do_handshake(bev_ssl);
}

static void
be_openssl_handshakeeventcb(evutil_socket_t fd, short what, void *ptr)
{
	struct bufferevent_openssl *bev_ssl = ptr;

	_bufferevent_incref_and_lock(&bev_ssl->bev.bev);
	if (what & EV_TIMEOUT) {
		_bufferevent_run_eventcb(&bev_ssl->bev.bev, BEV_EVENT_TIMEOUT);
	} else
		do_handshake(bev_ssl);
	_bufferevent_decref_and_unlock(&bev_ssl->bev.bev);
}

static int
set_handshake_callbacks(struct bufferevent_openssl *bev_ssl, evutil_socket_t fd)
{
	if (bev_ssl->underlying) {
		bufferevent_setcb(bev_ssl->underlying,
		    be_openssl_handshakecb, be_openssl_handshakecb,
		    be_openssl_eventcb,
		    bev_ssl);
		return do_handshake(bev_ssl);
	} else {
		struct bufferevent *bev = &bev_ssl->bev.bev;
		int r1=0, r2=0;
		if (fd < 0 && bev_ssl->fd_is_set)
			fd = event_get_fd(&bev->ev_read);
		if (bev_ssl->fd_is_set) {
			event_del(&bev->ev_read);
			event_del(&bev->ev_write);
		}
		event_assign(&bev->ev_read, bev->ev_base, fd,
		    EV_READ|EV_PERSIST, be_openssl_handshakeeventcb, bev_ssl);
		event_assign(&bev->ev_write, bev->ev_base, fd,
		    EV_WRITE|EV_PERSIST, be_openssl_handshakeeventcb, bev_ssl);
		if (fd >= 0) {
			r1 = _bufferevent_add_event(&bev->ev_read, &bev->timeout_read);
			r2 = _bufferevent_add_event(&bev->ev_write, &bev->timeout_write);
			bev_ssl->fd_is_set = 1;
		}
		return (r1<0 || r2<0) ? -1 : 0;
	}
}

int
bufferevent_ssl_renegotiate(struct bufferevent *bev)
{
	struct bufferevent_openssl *bev_ssl = upcast(bev);
	if (!bev_ssl)
		return -1;
	if (SSL_renegotiate(bev_ssl->ssl) < 0)
		return -1;
	bev_ssl->state = BUFFEREVENT_SSL_CONNECTING;
	if (set_handshake_callbacks(bev_ssl, -1) < 0)
		return -1;
	if (!bev_ssl->underlying)
		return do_handshake(bev_ssl);
	return 0;
}

static void
be_openssl_outbuf_cb(struct evbuffer *buf,
    const struct evbuffer_cb_info *cbinfo, void *arg)
{
	struct bufferevent_openssl *bev_ssl = arg;
	int r = 0;
	

	if (cbinfo->n_added && bev_ssl->state == BUFFEREVENT_SSL_OPEN) {
		if (cbinfo->orig_size == 0)
			r = _bufferevent_add_event(&bev_ssl->bev.bev.ev_write,
			    &bev_ssl->bev.bev.timeout_write);
		consider_writing(bev_ssl);
	}
	
        (void)r;
}


static int
be_openssl_enable(struct bufferevent *bev, short events)
{
	struct bufferevent_openssl *bev_ssl = upcast(bev);
	int r1 = 0, r2 = 0;

	if (bev_ssl->state != BUFFEREVENT_SSL_OPEN)
		return 0;

	if (events & EV_READ)
		r1 = start_reading(bev_ssl);
	if (events & EV_WRITE)
		r2 = start_writing(bev_ssl);

	if (bev_ssl->underlying) {
		if (events & EV_READ)
			BEV_RESET_GENERIC_READ_TIMEOUT(bev);
		if (events & EV_WRITE)
			BEV_RESET_GENERIC_WRITE_TIMEOUT(bev);

		if (events & EV_READ)
			consider_reading(bev_ssl);
		if (events & EV_WRITE)
			consider_writing(bev_ssl);
	}
	return (r1 < 0 || r2 < 0) ? -1 : 0;
}

static int
be_openssl_disable(struct bufferevent *bev, short events)
{
	struct bufferevent_openssl *bev_ssl = upcast(bev);
	if (bev_ssl->state != BUFFEREVENT_SSL_OPEN)
		return 0;

	if (events & EV_READ)
		stop_reading(bev_ssl);
	if (events & EV_WRITE)
		stop_writing(bev_ssl);

	if (bev_ssl->underlying) {
		if (events & EV_READ)
			BEV_DEL_GENERIC_READ_TIMEOUT(bev);
		if (events & EV_WRITE)
			BEV_DEL_GENERIC_WRITE_TIMEOUT(bev);
	}
	return 0;
}

static void
be_openssl_destruct(struct bufferevent *bev)
{
	struct bufferevent_openssl *bev_ssl = upcast(bev);

	if (bev_ssl->underlying) {
		_bufferevent_del_generic_timeout_cbs(bev);
	} else {
		event_del(&bev->ev_read);
		event_del(&bev->ev_write);
	}

	if (bev_ssl->bev.options & BEV_OPT_CLOSE_ON_FREE) {
		if (bev_ssl->underlying) {
			if (BEV_UPCAST(bev_ssl->underlying)->refcnt < 2) {
				event_warnx("BEV_OPT_CLOSE_ON_FREE set on an "
				    "bufferevent with too few references");
			} else {
				bufferevent_free(bev_ssl->underlying);
				bev_ssl->underlying = NULL;
			}
		} else {
			evutil_socket_t fd = -1;
			BIO *bio = SSL_get_wbio(bev_ssl->ssl);
			if (bio)
				fd = BIO_get_fd(bio, NULL);
			if (fd >= 0)
				evutil_closesocket(fd);
		}
		SSL_free(bev_ssl->ssl);
	} else {
		if (bev_ssl->underlying) {
			if (bev_ssl->underlying->errorcb == be_openssl_eventcb)
				bufferevent_setcb(bev_ssl->underlying,
				    NULL,NULL,NULL,NULL);
			bufferevent_unsuspend_read(bev_ssl->underlying,
			    BEV_SUSPEND_FILT_READ);
		}
	}
}

static int
be_openssl_adj_timeouts(struct bufferevent *bev)
{
	struct bufferevent_openssl *bev_ssl = upcast(bev);

	if (bev_ssl->underlying)
		return _bufferevent_generic_adj_timeouts(bev);
	else {
		int r1=0, r2=0;
		if (event_pending(&bev->ev_read, EV_READ, NULL))
			r1 = _bufferevent_add_event(&bev->ev_read, &bev->timeout_read);
		if (event_pending(&bev->ev_write, EV_WRITE, NULL))
			r2 = _bufferevent_add_event(&bev->ev_write, &bev->timeout_write);
		return (r1<0 || r2<0) ? -1 : 0;
	}
}

static int
be_openssl_flush(struct bufferevent *bufev,
    short iotype, enum bufferevent_flush_mode mode)
{
	
	return 0;
}

static int
be_openssl_ctrl(struct bufferevent *bev,
    enum bufferevent_ctrl_op op, union bufferevent_ctrl_data *data)
{
	struct bufferevent_openssl *bev_ssl = upcast(bev);
	switch (op) {
	case BEV_CTRL_SET_FD:
		if (bev_ssl->underlying)
			return -1;
		{
			BIO *bio;
			bio = BIO_new_socket(data->fd, 0);
			SSL_set_bio(bev_ssl->ssl, bio, bio);
			bev_ssl->fd_is_set = 1;
		}
		if (bev_ssl->state == BUFFEREVENT_SSL_OPEN)
			return set_open_callbacks(bev_ssl, data->fd);
		else {
			return set_handshake_callbacks(bev_ssl, data->fd);
		}
	case BEV_CTRL_GET_FD:
		if (bev_ssl->underlying)
			return -1;
		if (!bev_ssl->fd_is_set)
			return -1;
		data->fd = event_get_fd(&bev->ev_read);
		return 0;
	case BEV_CTRL_GET_UNDERLYING:
		if (!bev_ssl->underlying)
			return -1;
		data->ptr = bev_ssl->underlying;
		return 0;
	case BEV_CTRL_CANCEL_ALL:
	default:
		return -1;
	}
}

SSL *
bufferevent_openssl_get_ssl(struct bufferevent *bufev)
{
	struct bufferevent_openssl *bev_ssl = upcast(bufev);
	if (!bev_ssl)
		return NULL;
	return bev_ssl->ssl;
}

static struct bufferevent *
bufferevent_openssl_new_impl(struct event_base *base,
    struct bufferevent *underlying,
    evutil_socket_t fd,
    SSL *ssl,
    enum bufferevent_ssl_state state,
    int options)
{
	struct bufferevent_openssl *bev_ssl = NULL;
	struct bufferevent_private *bev_p = NULL;
	int tmp_options = options & ~BEV_OPT_THREADSAFE;

	if (underlying != NULL && fd >= 0)
		return NULL; 

	if (!(bev_ssl = mm_calloc(1, sizeof(struct bufferevent_openssl))))
		goto err;

	bev_p = &bev_ssl->bev;

	if (bufferevent_init_common(bev_p, base,
		&bufferevent_ops_openssl, tmp_options) < 0)
		goto err;

	

	SSL_set_mode(ssl, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

	bev_ssl->underlying = underlying;
	bev_ssl->ssl = ssl;

	bev_ssl->outbuf_cb = evbuffer_add_cb(bev_p->bev.output,
	    be_openssl_outbuf_cb, bev_ssl);

	if (options & BEV_OPT_THREADSAFE)
		bufferevent_enable_locking(&bev_ssl->bev.bev, NULL);

	if (underlying) {
		_bufferevent_init_generic_timeout_cbs(&bev_ssl->bev.bev);
		bufferevent_incref(underlying);
	}

	bev_ssl->state = state;
	bev_ssl->last_write = -1;

	init_bio_counts(bev_ssl);

	switch (state) {
	case BUFFEREVENT_SSL_ACCEPTING:
		SSL_set_accept_state(bev_ssl->ssl);
		if (set_handshake_callbacks(bev_ssl, fd) < 0)
			goto err;
		break;
	case BUFFEREVENT_SSL_CONNECTING:
		SSL_set_connect_state(bev_ssl->ssl);
		if (set_handshake_callbacks(bev_ssl, fd) < 0)
			goto err;
		break;
	case BUFFEREVENT_SSL_OPEN:
		if (set_open_callbacks(bev_ssl, fd) < 0)
			goto err;
		break;
	default:
		goto err;
	}

	if (underlying) {
		bufferevent_setwatermark(underlying, EV_READ, 0, 0);
		bufferevent_enable(underlying, EV_READ|EV_WRITE);
		if (state == BUFFEREVENT_SSL_OPEN)
			bufferevent_suspend_read(underlying,
			    BEV_SUSPEND_FILT_READ);
	} else {
		bev_ssl->bev.bev.enabled = EV_READ|EV_WRITE;
		if (bev_ssl->fd_is_set) {
			if (state != BUFFEREVENT_SSL_OPEN)
				if (event_add(&bev_ssl->bev.bev.ev_read, NULL) < 0)
					goto err;
			if (event_add(&bev_ssl->bev.bev.ev_write, NULL) < 0)
				goto err;
		}
	}

	return &bev_ssl->bev.bev;
err:
	if (bev_ssl)
		bufferevent_free(&bev_ssl->bev.bev);
	return NULL;
}

struct bufferevent *
bufferevent_openssl_filter_new(struct event_base *base,
    struct bufferevent *underlying,
    SSL *ssl,
    enum bufferevent_ssl_state state,
    int options)
{
	

	int close_flag = 0; 
	BIO *bio;
	if (!underlying)
		return NULL;
	if (!(bio = BIO_new_bufferevent(underlying, close_flag)))
		return NULL;

	SSL_set_bio(ssl, bio, bio);

	return bufferevent_openssl_new_impl(
		base, underlying, -1, ssl, state, options);
}

struct bufferevent *
bufferevent_openssl_socket_new(struct event_base *base,
    evutil_socket_t fd,
    SSL *ssl,
    enum bufferevent_ssl_state state,
    int options)
{
	
	BIO *bio = SSL_get_wbio(ssl);
	long have_fd = -1;

	if (bio)
		have_fd = BIO_get_fd(bio, NULL);

	if (have_fd >= 0) {
		
		if (fd < 0) {
			
			fd = (evutil_socket_t) have_fd;
		} else if (have_fd == (long)fd) {
			
		} else {
			

			return NULL;
		}
		(void) BIO_set_close(bio, 0);
	} else {
		
		if (fd >= 0) {
			
			bio = BIO_new_socket(fd, 0);
			SSL_set_bio(ssl, bio, bio);
		} else {
			
		}
	}

	return bufferevent_openssl_new_impl(
		base, NULL, fd, ssl, state, options);
}

unsigned long
bufferevent_get_openssl_error(struct bufferevent *bev)
{
	unsigned long err = 0;
	struct bufferevent_openssl *bev_ssl;
	BEV_LOCK(bev);
	bev_ssl = upcast(bev);
	if (bev_ssl && bev_ssl->n_errors) {
		err = bev_ssl->errors[--bev_ssl->n_errors];
	}
	BEV_UNLOCK(bev);
	return err;
}
