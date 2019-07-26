
























#ifndef _EVENT2_BUFFER_H_
#define _EVENT2_BUFFER_H_















































#ifdef __cplusplus
extern "C" {
#endif

#include <event2/event-config.h>
#include <stdarg.h>
#ifdef _EVENT_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef _EVENT_HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
#include <event2/util.h>







struct evbuffer
#ifdef _EVENT_IN_DOXYGEN
{}
#endif
;









struct evbuffer_ptr {
	ev_ssize_t pos;

	
	struct {
		void *chain;
		size_t pos_in_chain;
	} _internal;
};






#ifdef _EVENT_HAVE_SYS_UIO_H
#define evbuffer_iovec iovec

#define _EVBUFFER_IOVEC_IS_NATIVE
#else
struct evbuffer_iovec {
	
	void *iov_base;
	
	size_t iov_len;
};
#endif







struct evbuffer *evbuffer_new(void);





void evbuffer_free(struct evbuffer *buf);













int evbuffer_enable_locking(struct evbuffer *buf, void *lock);





void evbuffer_lock(struct evbuffer *buf);





void evbuffer_unlock(struct evbuffer *buf);

















#define EVBUFFER_FLAG_DRAINS_TO_FD 1








int evbuffer_set_flags(struct evbuffer *buf, ev_uint64_t flags);







int evbuffer_clear_flags(struct evbuffer *buf, ev_uint64_t flags);







size_t evbuffer_get_length(const struct evbuffer *buf);













size_t evbuffer_get_contiguous_space(const struct evbuffer *buf);











int evbuffer_expand(struct evbuffer *buf, size_t datlen);


































int
evbuffer_reserve_space(struct evbuffer *buf, ev_ssize_t size,
    struct evbuffer_iovec *vec, int n_vec);






















int evbuffer_commit_space(struct evbuffer *buf,
    struct evbuffer_iovec *vec, int n_vecs);









int evbuffer_add(struct evbuffer *buf, const void *data, size_t datlen);













int evbuffer_remove(struct evbuffer *buf, void *data, size_t datlen);












ev_ssize_t evbuffer_copyout(struct evbuffer *buf, void *data_out, size_t datlen);














int evbuffer_remove_buffer(struct evbuffer *src, struct evbuffer *dst,
    size_t datlen);



enum evbuffer_eol_style {
	








	EVBUFFER_EOL_ANY,
	

	EVBUFFER_EOL_CRLF,
	
	EVBUFFER_EOL_CRLF_STRICT,
	
	EVBUFFER_EOL_LF
};















char *evbuffer_readln(struct evbuffer *buffer, size_t *n_read_out,
    enum evbuffer_eol_style eol_style);













int evbuffer_add_buffer(struct evbuffer *outbuf, struct evbuffer *inbuf);







typedef void (*evbuffer_ref_cleanup_cb)(const void *data,
    size_t datalen, void *extra);
















int evbuffer_add_reference(struct evbuffer *outbuf,
    const void *data, size_t datlen,
    evbuffer_ref_cleanup_cb cleanupfn, void *cleanupfn_arg);






















int evbuffer_add_file(struct evbuffer *outbuf, int fd, ev_off_t offset,
    ev_off_t length);













int evbuffer_add_printf(struct evbuffer *buf, const char *fmt, ...)
#ifdef __GNUC__
  __attribute__((format(printf, 2, 3)))
#endif
;









int evbuffer_add_vprintf(struct evbuffer *buf, const char *fmt, va_list ap)
#ifdef __GNUC__
	__attribute__((format(printf, 2, 0)))
#endif
;









int evbuffer_drain(struct evbuffer *buf, size_t len);












int evbuffer_write(struct evbuffer *buffer, evutil_socket_t fd);













int evbuffer_write_atmost(struct evbuffer *buffer, evutil_socket_t fd,
						  ev_ssize_t howmuch);










int evbuffer_read(struct evbuffer *buffer, evutil_socket_t fd, int howmuch);












struct evbuffer_ptr evbuffer_search(struct evbuffer *buffer, const char *what, size_t len, const struct evbuffer_ptr *start);















struct evbuffer_ptr evbuffer_search_range(struct evbuffer *buffer, const char *what, size_t len, const struct evbuffer_ptr *start, const struct evbuffer_ptr *end);





enum evbuffer_ptr_how {
	

	EVBUFFER_PTR_SET,
	
	EVBUFFER_PTR_ADD
};













int
evbuffer_ptr_set(struct evbuffer *buffer, struct evbuffer_ptr *ptr,
    size_t position, enum evbuffer_ptr_how how);















struct evbuffer_ptr evbuffer_search_eol(struct evbuffer *buffer,
    struct evbuffer_ptr *start, size_t *eol_len_out,
    enum evbuffer_eol_style eol_style);





























int evbuffer_peek(struct evbuffer *buffer, ev_ssize_t len,
    struct evbuffer_ptr *start_at,
    struct evbuffer_iovec *vec_out, int n_vec);






struct evbuffer_cb_info {
	

	size_t orig_size;
	
	size_t n_added;
	
	size_t n_deleted;
};




















typedef void (*evbuffer_cb_func)(struct evbuffer *buffer, const struct evbuffer_cb_info *info, void *arg);

struct evbuffer_cb_entry;











struct evbuffer_cb_entry *evbuffer_add_cb(struct evbuffer *buffer, evbuffer_cb_func cb, void *cbarg);









int evbuffer_remove_cb_entry(struct evbuffer *buffer,
			     struct evbuffer_cb_entry *ent);







int evbuffer_remove_cb(struct evbuffer *buffer, evbuffer_cb_func cb, void *cbarg);






#define EVBUFFER_CB_ENABLED 1








int evbuffer_cb_set_flags(struct evbuffer *buffer,
			  struct evbuffer_cb_entry *cb, ev_uint32_t flags);








int evbuffer_cb_clear_flags(struct evbuffer *buffer,
			  struct evbuffer_cb_entry *cb, ev_uint32_t flags);

#if 0









void evbuffer_cb_suspend(struct evbuffer *buffer, struct evbuffer_cb_entry *cb);








void evbuffer_cb_unsuspend(struct evbuffer *buffer, struct evbuffer_cb_entry *cb);
#endif










unsigned char *evbuffer_pullup(struct evbuffer *buf, ev_ssize_t size);










int evbuffer_prepend(struct evbuffer *buf, const void *data, size_t size);









int evbuffer_prepend_buffer(struct evbuffer *dst, struct evbuffer* src);















int evbuffer_freeze(struct evbuffer *buf, int at_front);








int evbuffer_unfreeze(struct evbuffer *buf, int at_front);

struct event_base;







int evbuffer_defer_callbacks(struct evbuffer *buffer, struct event_base *base);

#ifdef __cplusplus
}
#endif

#endif
