

























#ifndef _EVENT2_HTTP_COMPAT_H_
#define _EVENT2_HTTP_COMPAT_H_








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


#include <event2/util.h>










struct evhttp *evhttp_start(const char *address, unsigned short port);








struct evhttp_connection *evhttp_connection_new(
	const char *address, unsigned short port);







void evhttp_connection_set_base(struct evhttp_connection *evcon,
    struct event_base *base);



#define evhttp_request_uri evhttp_request_get_uri

#ifdef __cplusplus
}
#endif

#endif
