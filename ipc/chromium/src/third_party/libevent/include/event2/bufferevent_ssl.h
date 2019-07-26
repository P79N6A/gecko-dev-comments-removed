
























#ifndef _EVENT2_BUFFEREVENT_SSL_H_
#define _EVENT2_BUFFEREVENT_SSL_H_






#include <event2/event-config.h>
#include <event2/bufferevent.h>
#include <event2/util.h>

#ifdef __cplusplus
extern "C" {
#endif


struct ssl_st;





enum bufferevent_ssl_state {
	BUFFEREVENT_SSL_OPEN = 0,
	BUFFEREVENT_SSL_CONNECTING = 1,
	BUFFEREVENT_SSL_ACCEPTING = 2
};

#if defined(_EVENT_HAVE_OPENSSL) || defined(_EVENT_IN_DOXYGEN)











struct bufferevent *
bufferevent_openssl_filter_new(struct event_base *base,
    struct bufferevent *underlying,
    struct ssl_st *ssl,
    enum bufferevent_ssl_state state,
    int options);











struct bufferevent *
bufferevent_openssl_socket_new(struct event_base *base,
    evutil_socket_t fd,
    struct ssl_st *ssl,
    enum bufferevent_ssl_state state,
    int options);


struct ssl_st *
bufferevent_openssl_get_ssl(struct bufferevent *bufev);


int bufferevent_ssl_renegotiate(struct bufferevent *bev);


unsigned long bufferevent_get_openssl_error(struct bufferevent *bev);

#endif

#ifdef __cplusplus
}
#endif

#endif
