

























#ifndef _EVENT2_HTTP_STRUCT_H_
#define _EVENT2_HTTP_STRUCT_H_








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






struct evhttp_request {
#if defined(TAILQ_ENTRY)
	TAILQ_ENTRY(evhttp_request) next;
#else
struct {
	struct evhttp_request *tqe_next;
	struct evhttp_request **tqe_prev;
}       next;
#endif

	
	struct evhttp_connection *evcon;
	int flags;

#define EVHTTP_REQ_OWN_CONNECTION	0x0001

#define EVHTTP_PROXY_REQUEST		0x0002

#define EVHTTP_USER_OWNED		0x0004

#define EVHTTP_REQ_DEFER_FREE		0x0008

#define EVHTTP_REQ_NEEDS_FREE		0x0010

	struct evkeyvalq *input_headers;
	struct evkeyvalq *output_headers;

	
	char *remote_host;
	ev_uint16_t remote_port;

	
	char *host_cache;

	enum evhttp_request_kind kind;
	enum evhttp_cmd_type type;

	size_t headers_size;
	size_t body_size;

	char *uri;			
	struct evhttp_uri *uri_elems;	

	char major;			
	char minor;			

	int response_code;		
	char *response_code_line;	

	struct evbuffer *input_buffer;	
	ev_int64_t ntoread;
	unsigned chunked:1,		
	    userdone:1;			

	struct evbuffer *output_buffer;	

	
	void (*cb)(struct evhttp_request *, void *);
	void *cb_arg;

	




	void (*chunk_cb)(struct evhttp_request *, void *);
};

#ifdef __cplusplus
}
#endif

#endif

