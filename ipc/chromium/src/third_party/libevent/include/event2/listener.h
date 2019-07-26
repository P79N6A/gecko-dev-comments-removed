

























#ifndef _EVENT2_LISTENER_H_
#define _EVENT2_LISTENER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <event2/event.h>

struct sockaddr;
struct evconnlistener;










typedef void (*evconnlistener_cb)(struct evconnlistener *, evutil_socket_t, struct sockaddr *, int socklen, void *);







typedef void (*evconnlistener_errorcb)(struct evconnlistener *, void *);



#define LEV_OPT_LEAVE_SOCKETS_BLOCKING	(1u<<0)


#define LEV_OPT_CLOSE_ON_FREE		(1u<<1)

#define LEV_OPT_CLOSE_ON_EXEC		(1u<<2)


#define LEV_OPT_REUSEABLE		(1u<<3)


#define LEV_OPT_THREADSAFE		(1u<<4)


















struct evconnlistener *evconnlistener_new(struct event_base *base,
    evconnlistener_cb cb, void *ptr, unsigned flags, int backlog,
    evutil_socket_t fd);















struct evconnlistener *evconnlistener_new_bind(struct event_base *base,
    evconnlistener_cb cb, void *ptr, unsigned flags, int backlog,
    const struct sockaddr *sa, int socklen);



void evconnlistener_free(struct evconnlistener *lev);



int evconnlistener_enable(struct evconnlistener *lev);



int evconnlistener_disable(struct evconnlistener *lev);


struct event_base *evconnlistener_get_base(struct evconnlistener *lev);


evutil_socket_t evconnlistener_get_fd(struct evconnlistener *lev);



void evconnlistener_set_cb(struct evconnlistener *lev,
    evconnlistener_cb cb, void *arg);


void evconnlistener_set_error_cb(struct evconnlistener *lev,
    evconnlistener_errorcb errorcb);

#ifdef __cplusplus
}
#endif

#endif
