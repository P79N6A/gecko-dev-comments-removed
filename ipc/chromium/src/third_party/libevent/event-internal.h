

























#ifndef _EVENT_INTERNAL_H_
#define _EVENT_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "event2/event-config.h"
#include <time.h>
#include <sys/queue.h>
#include "event2/event_struct.h"
#include "minheap-internal.h"
#include "evsignal-internal.h"
#include "mm-internal.h"
#include "defer-internal.h"




#define ev_signal_next	_ev.ev_signal.ev_signal_next
#define ev_io_next	_ev.ev_io.ev_io_next
#define ev_io_timeout	_ev.ev_io.ev_timeout


#define ev_ncalls	_ev.ev_signal.ev_ncalls
#define ev_pncalls	_ev.ev_signal.ev_pncalls


#define EV_CLOSURE_NONE 0
#define EV_CLOSURE_SIGNAL 1
#define EV_CLOSURE_PERSIST 2


struct eventop {
	
	const char *name;
	




	void *(*init)(struct event_base *);
	







	int (*add)(struct event_base *, evutil_socket_t fd, short old, short events, void *fdinfo);
	
	int (*del)(struct event_base *, evutil_socket_t fd, short old, short events, void *fdinfo);
	




	int (*dispatch)(struct event_base *, struct timeval *);
	
	void (*dealloc)(struct event_base *);
	

	int need_reinit;
	

	enum event_method_feature features;
	




	size_t fdinfo_len;
};

#ifdef WIN32




#define EVMAP_USE_HT
#endif



#ifdef EVMAP_USE_HT
#include "ht-internal.h"
struct event_map_entry;
HT_HEAD(event_io_map, event_map_entry);
#else
#define event_io_map event_signal_map
#endif





struct event_signal_map {
	

	void **entries;
	
	int nentries;
};





struct common_timeout_list {
	
	struct event_list events;
	

	struct timeval duration;
	

	struct event timeout_event;
	
	struct event_base *base;
};


#define COMMON_TIMEOUT_MICROSECONDS_MASK       0x000fffff

struct event_change;



struct event_changelist {
	struct event_change *changes;
	int n_changes;
	int changes_size;
};

#ifndef _EVENT_DISABLE_DEBUG_MODE

extern int _event_debug_mode_on;
#define EVENT_DEBUG_MODE_IS_ON() (_event_debug_mode_on)
#else
#define EVENT_DEBUG_MODE_IS_ON() (0)
#endif

struct event_base {
	

	const struct eventop *evsel;
	
	void *evbase;

	

	struct event_changelist changelist;

	

	const struct eventop *evsigsel;
	
	struct evsig_info sig;

	
	int virtual_event_count;
	
	int event_count;
	
	int event_count_active;

	

	int event_gotterm;
	
	int event_break;
	
	int event_continue;

	
	int event_running_priority;

	

	int running_loop;

	
	



	struct event_list *activequeues;
	
	int nactivequeues;

	

	

	struct common_timeout_list **common_timeout_queues;
	
	int n_common_timeouts;
	
	int n_common_timeouts_allocated;

	

	struct deferred_cb_queue defer_queue;

	
	struct event_io_map io;

	
	struct event_signal_map sigmap;

	
	struct event_list eventqueue;

	
	struct timeval event_tv;

	
	struct min_heap timeheap;

	

	struct timeval tv_cache;

#if defined(_EVENT_HAVE_CLOCK_GETTIME) && defined(CLOCK_MONOTONIC)
	

	struct timeval tv_clock_diff;
	
	time_t last_updated_clock_diff;
#endif

#ifndef _EVENT_DISABLE_THREAD_SUPPORT
	
	
	unsigned long th_owner_id;
	
	void *th_base_lock;
	
	struct event *current_event;
	

	void *current_event_cond;
	
	int current_event_waiters;
#endif

#ifdef WIN32
	
	struct event_iocp_port *iocp;
#endif

	
	enum event_base_config_flag flags;

	
	

	int is_notify_pending;
	

	evutil_socket_t th_notify_fd[2];
	

	struct event th_notify;
	
	int (*th_notify_fn)(struct event_base *base);
};

struct event_config_entry {
	TAILQ_ENTRY(event_config_entry) next;

	const char *avoid_method;
};



struct event_config {
	TAILQ_HEAD(event_configq, event_config_entry) entries;

	int n_cpus_hint;
	enum event_method_feature require_features;
	enum event_base_config_flag flags;
};


#if defined(_EVENT_HAVE_SYS_QUEUE_H) && !defined(_EVENT_HAVE_TAILQFOREACH)
#ifndef TAILQ_FIRST
#define	TAILQ_FIRST(head)		((head)->tqh_first)
#endif
#ifndef TAILQ_END
#define	TAILQ_END(head)			NULL
#endif
#ifndef TAILQ_NEXT
#define	TAILQ_NEXT(elm, field)		((elm)->field.tqe_next)
#endif

#ifndef TAILQ_FOREACH
#define TAILQ_FOREACH(var, head, field)					\
	for ((var) = TAILQ_FIRST(head);					\
	     (var) != TAILQ_END(head);					\
	     (var) = TAILQ_NEXT(var, field))
#endif

#ifndef TAILQ_INSERT_BEFORE
#define	TAILQ_INSERT_BEFORE(listelm, elm, field) do {			\
	(elm)->field.tqe_prev = (listelm)->field.tqe_prev;		\
	(elm)->field.tqe_next = (listelm);				\
	*(listelm)->field.tqe_prev = (elm);				\
	(listelm)->field.tqe_prev = &(elm)->field.tqe_next;		\
} while (0)
#endif
#endif 

#define N_ACTIVE_CALLBACKS(base)					\
	((base)->event_count_active + (base)->defer_queue.active_count)

int _evsig_set_handler(struct event_base *base, int evsignal,
			  void (*fn)(int));
int _evsig_restore_handler(struct event_base *base, int evsignal);


void event_active_nolock(struct event *ev, int res, short count);


void event_base_add_virtual(struct event_base *base);
void event_base_del_virtual(struct event_base *base);







void event_base_assert_ok(struct event_base *base);

#ifdef __cplusplus
}
#endif

#endif

