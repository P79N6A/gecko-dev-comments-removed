


























#ifndef _EVENT2_BUFFEREVENT_COMPAT_H_
#define _EVENT2_BUFFEREVENT_COMPAT_H_

#define evbuffercb bufferevent_data_cb
#define everrorcb bufferevent_event_cb










































struct bufferevent *bufferevent_new(evutil_socket_t fd,
    evbuffercb readcb, evbuffercb writecb, everrorcb errorcb, void *cbarg);









void bufferevent_settimeout(struct bufferevent *bufev,
    int timeout_read, int timeout_write);

#define EVBUFFER_READ		BEV_EVENT_READING
#define EVBUFFER_WRITE		BEV_EVENT_WRITING
#define EVBUFFER_EOF		BEV_EVENT_EOF
#define EVBUFFER_ERROR		BEV_EVENT_ERROR
#define EVBUFFER_TIMEOUT	BEV_EVENT_TIMEOUT


#define EVBUFFER_INPUT(x)	bufferevent_get_input(x)

#define EVBUFFER_OUTPUT(x)	bufferevent_get_output(x)

#endif
