

























#ifndef _EVENT2_EVENT_H_
#define _EVENT2_EVENT_H_


























































































































































#ifdef __cplusplus
extern "C" {
#endif

#include <event2/event-config.h>
#ifdef _EVENT_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef _EVENT_HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include <stdio.h>


#include <event2/util.h>














struct event_base
#ifdef _EVENT_IN_DOXYGEN
{}
#endif
;






















































struct event
#ifdef _EVENT_IN_DOXYGEN
{}
#endif
;














struct event_config
#ifdef _EVENT_IN_DOXYGEN
{}
#endif
;





















void event_enable_debug_mode(void);










void event_debug_unassign(struct event *);








struct event_base *event_base_new(void);











int event_reinit(struct event_base *base);














int event_base_dispatch(struct event_base *);







const char *event_base_get_method(const struct event_base *);













const char **event_get_supported_methods(void);











struct event_config *event_config_new(void);






void event_config_free(struct event_config *cfg);













int event_config_avoid_method(struct event_config *cfg, const char *method);










enum event_method_feature {
    
    EV_FEATURE_ET = 0x01,
    



    EV_FEATURE_O1 = 0x02,
    

    EV_FEATURE_FDS = 0x04
};









enum event_base_config_flag {
	

	EVENT_BASE_FLAG_NOLOCK = 0x01,
	

	EVENT_BASE_FLAG_IGNORE_ENV = 0x02,
	





	EVENT_BASE_FLAG_STARTUP_IOCP = 0x04,
	


	EVENT_BASE_FLAG_NO_CACHE_TIME = 0x08,

	













	EVENT_BASE_FLAG_EPOLL_USE_CHANGELIST = 0x10
};








int event_base_get_features(const struct event_base *base);























int event_config_require_features(struct event_config *cfg, int feature);







int event_config_set_flag(struct event_config *cfg, int flag);










int event_config_set_num_cpus_hint(struct event_config *cfg, int cpus);













struct event_base *event_base_new_with_config(const struct event_config *);









void event_base_free(struct event_base *);




#define EVENT_LOG_DEBUG 0
#define EVENT_LOG_MSG   1
#define EVENT_LOG_WARN  2
#define EVENT_LOG_ERR   3




#define _EVENT_LOG_DEBUG EVENT_LOG_DEBUG
#define _EVENT_LOG_MSG EVENT_LOG_MSG
#define _EVENT_LOG_WARN EVENT_LOG_WARN
#define _EVENT_LOG_ERR EVENT_LOG_ERR






typedef void (*event_log_cb)(int severity, const char *msg);










void event_set_log_callback(event_log_cb cb);






typedef void (*event_fatal_cb)(int err);













void event_set_fatal_callback(event_fatal_cb cb);










int event_base_set(struct event_base *, struct event *);








#define EVLOOP_ONCE	0x01


#define EVLOOP_NONBLOCK	0x02




















int event_base_loop(struct event_base *, int);
















int event_base_loopexit(struct event_base *, const struct timeval *);














int event_base_loopbreak(struct event_base *);













int event_base_got_exit(struct event_base *);













int event_base_got_break(struct event_base *);










#define EV_TIMEOUT	0x01

#define EV_READ		0x02

#define EV_WRITE	0x04

#define EV_SIGNAL	0x08






#define EV_PERSIST	0x10

#define EV_ET       0x20







#define evtimer_assign(ev, b, cb, arg) \
	event_assign((ev), (b), -1, 0, (cb), (arg))
#define evtimer_new(b, cb, arg)	       event_new((b), -1, 0, (cb), (arg))
#define evtimer_add(ev, tv)		event_add((ev), (tv))
#define evtimer_del(ev)			event_del(ev)
#define evtimer_pending(ev, tv)		event_pending((ev), EV_TIMEOUT, (tv))
#define evtimer_initialized(ev)		event_initialized(ev)








#define evsignal_add(ev, tv)		event_add((ev), (tv))
#define evsignal_assign(ev, b, x, cb, arg)			\
	event_assign((ev), (b), (x), EV_SIGNAL|EV_PERSIST, cb, (arg))
#define evsignal_new(b, x, cb, arg)				\
	event_new((b), (x), EV_SIGNAL|EV_PERSIST, (cb), (arg))
#define evsignal_del(ev)		event_del(ev)
#define evsignal_pending(ev, tv)	event_pending((ev), EV_SIGNAL, (tv))
#define evsignal_initialized(ev)	event_initialized(ev)













typedef void (*event_callback_fn)(evutil_socket_t, short, void *);

















































struct event *event_new(struct event_base *, evutil_socket_t, short, event_callback_fn, void *);








































int event_assign(struct event *, struct event_base *, evutil_socket_t, short, event_callback_fn, void *);







void event_free(struct event *);























int event_base_once(struct event_base *, evutil_socket_t, short, event_callback_fn, void *, const struct timeval *);























int event_add(struct event *ev, const struct timeval *timeout);












int event_del(struct event *);
















void event_active(struct event *ev, int res, short ncalls);














int event_pending(const struct event *ev, short events, struct timeval *tv);

















int event_initialized(const struct event *ev);




#define event_get_signal(ev) ((int)event_get_fd(ev))





evutil_socket_t event_get_fd(const struct event *ev);




struct event_base *event_get_base(const struct event *ev);




short event_get_events(const struct event *ev);




event_callback_fn event_get_callback(const struct event *ev);




void *event_get_callback_arg(const struct event *ev);








void event_get_assignment(const struct event *event,
    struct event_base **base_out, evutil_socket_t *fd_out, short *events_out,
    event_callback_fn *callback_out, void **arg_out);














size_t event_get_struct_event_size(void);










const char *event_get_version(void);












ev_uint32_t event_get_version_number(void);


#define LIBEVENT_VERSION _EVENT_VERSION


#define LIBEVENT_VERSION_NUMBER _EVENT_NUMERIC_VERSION


#define EVENT_MAX_PRIORITIES 256



























int	event_base_priority_init(struct event_base *, int);









int	event_priority_set(struct event *, int);




















const struct timeval *event_base_init_common_timeout(struct event_base *base,
    const struct timeval *duration);

#if !defined(_EVENT_DISABLE_MM_REPLACEMENT) || defined(_EVENT_IN_DOXYGEN)






















void event_set_mem_functions(
	void *(*malloc_fn)(size_t sz),
	void *(*realloc_fn)(void *ptr, size_t sz),
	void (*free_fn)(void *ptr));


#define EVENT_SET_MEM_FUNCTIONS_IMPLEMENTED
#endif

void event_base_dump_events(struct event_base *, FILE *);












int event_base_gettimeofday_cached(struct event_base *base,
    struct timeval *tv);

#ifdef __cplusplus
}
#endif

#endif
