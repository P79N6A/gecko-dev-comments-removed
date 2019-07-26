

























#ifndef _EVBUFFER_INTERNAL_H_
#define _EVBUFFER_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "event2/event-config.h"
#include "event2/util.h"
#include "util-internal.h"
#include "defer-internal.h"




#define EVBUFFER_CB_NODEFER 2

#ifdef WIN32
#include <winsock2.h>
#endif
#include <sys/queue.h>




#if _EVENT_SIZEOF_VOID_P < 8
#define MIN_BUFFER_SIZE	512
#else
#define MIN_BUFFER_SIZE	1024
#endif



struct evbuffer_cb_entry {
	
	TAILQ_ENTRY(evbuffer_cb_entry) next;
	


	union {
		evbuffer_cb_func cb_func;
		evbuffer_cb cb_obsolete;
	} cb;
	
	void *cbarg;
	
	ev_uint32_t flags;
};

struct bufferevent;
struct evbuffer_chain;
struct evbuffer {
	
	struct evbuffer_chain *first;
	
	struct evbuffer_chain *last;

	












	struct evbuffer_chain **last_with_datap;

	
	size_t total_len;

	

	size_t n_add_for_cb;
	

	size_t n_del_for_cb;

#ifndef _EVENT_DISABLE_THREAD_SUPPORT
	
	void *lock;
#endif
	

	unsigned own_lock : 1;
	

	unsigned freeze_start : 1;
	

	unsigned freeze_end : 1;
	




	unsigned deferred_cbs : 1;
#ifdef WIN32
	
	unsigned is_overlapped : 1;
#endif
	
	ev_uint32_t flags;

	
	struct deferred_cb_queue *cb_queue;

	



	int refcnt;

	

	struct deferred_cb deferred;

	
	TAILQ_HEAD(evbuffer_cb_queue, evbuffer_cb_entry) callbacks;

	

	struct bufferevent *parent;
};


struct evbuffer_chain {
	
	struct evbuffer_chain *next;

	
	size_t buffer_len;

	

	ev_off_t misalign;

	


	size_t off;

	
	unsigned flags;
#define EVBUFFER_MMAP		0x0001	/**< memory in buffer is mmaped */
#define EVBUFFER_SENDFILE	0x0002	/**< a chain used for sendfile */
#define EVBUFFER_REFERENCE	0x0004	/**< a chain with a mem reference */
#define EVBUFFER_IMMUTABLE	0x0008	/**< read-only chain */
	

#define EVBUFFER_MEM_PINNED_R	0x0010
#define EVBUFFER_MEM_PINNED_W	0x0020
#define EVBUFFER_MEM_PINNED_ANY (EVBUFFER_MEM_PINNED_R|EVBUFFER_MEM_PINNED_W)
	

#define EVBUFFER_DANGLING	0x0040

	





	unsigned char *buffer;
};





struct evbuffer_chain_fd {
	int fd;	
};



struct evbuffer_chain_reference {
	evbuffer_ref_cleanup_cb cleanupfn;
	void *extra;
};

#define EVBUFFER_CHAIN_SIZE sizeof(struct evbuffer_chain)

#define EVBUFFER_CHAIN_EXTRA(t, c) (t *)((struct evbuffer_chain *)(c) + 1)


#define ASSERT_EVBUFFER_LOCKED(buffer)			\
	EVLOCK_ASSERT_LOCKED((buffer)->lock)

#define EVBUFFER_LOCK(buffer)						\
	do {								\
		EVLOCK_LOCK((buffer)->lock, 0);				\
	} while (0)
#define EVBUFFER_UNLOCK(buffer)						\
	do {								\
		EVLOCK_UNLOCK((buffer)->lock, 0);			\
	} while (0)
#define EVBUFFER_LOCK2(buffer1, buffer2)				\
	do {								\
		EVLOCK_LOCK2((buffer1)->lock, (buffer2)->lock, 0, 0);	\
	} while (0)
#define EVBUFFER_UNLOCK2(buffer1, buffer2)				\
	do {								\
		EVLOCK_UNLOCK2((buffer1)->lock, (buffer2)->lock, 0, 0);	\
	} while (0)


void _evbuffer_incref(struct evbuffer *buf);

void _evbuffer_incref_and_lock(struct evbuffer *buf);


void _evbuffer_chain_pin(struct evbuffer_chain *chain, unsigned flag);

void _evbuffer_chain_unpin(struct evbuffer_chain *chain, unsigned flag);


void _evbuffer_decref_and_unlock(struct evbuffer *buffer);



int _evbuffer_expand_fast(struct evbuffer *, size_t, int);







int _evbuffer_read_setup_vecs(struct evbuffer *buf, ev_ssize_t howmuch,
    struct evbuffer_iovec *vecs, int n_vecs, struct evbuffer_chain ***chainp,
    int exact);


#define WSABUF_FROM_EVBUFFER_IOV(i,ei) do {		\
		(i)->buf = (ei)->iov_base;		\
		(i)->len = (unsigned long)(ei)->iov_len;	\
	} while (0)




void evbuffer_set_parent(struct evbuffer *buf, struct bufferevent *bev);

void evbuffer_invoke_callbacks(struct evbuffer *buf);

#ifdef __cplusplus
}
#endif

#endif
