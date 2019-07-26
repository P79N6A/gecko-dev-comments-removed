

























#ifndef _EVENT2_BUFFEREVENT_STRUCT_H_
#define _EVENT2_BUFFEREVENT_STRUCT_H_











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

#include <event2/event_struct.h>

struct event_watermark {
	size_t low;
	size_t high;
};









struct bufferevent {
	
	struct event_base *ev_base;
	

	const struct bufferevent_ops *be_ops;

	


	struct event ev_read;
	


	struct event ev_write;

	

	struct evbuffer *input;

	

	struct evbuffer *output;

	struct event_watermark wm_read;
	struct event_watermark wm_write;

	bufferevent_data_cb readcb;
	bufferevent_data_cb writecb;
	

	bufferevent_event_cb errorcb;
	void *cbarg;

	struct timeval timeout_read;
	struct timeval timeout_write;

	

	short enabled;
};

#ifdef __cplusplus
}
#endif

#endif
