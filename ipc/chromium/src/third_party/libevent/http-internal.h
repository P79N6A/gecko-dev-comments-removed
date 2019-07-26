








#ifndef _HTTP_INTERNAL_H_
#define _HTTP_INTERNAL_H_

#include "event2/event_struct.h"
#include "util-internal.h"
#include "defer-internal.h"

#define HTTP_CONNECT_TIMEOUT	45
#define HTTP_WRITE_TIMEOUT	50
#define HTTP_READ_TIMEOUT	50

#define HTTP_PREFIX		"http://"
#define HTTP_DEFAULTPORT	80

enum message_read_status {
	ALL_DATA_READ = 1,
	MORE_DATA_EXPECTED = 0,
	DATA_CORRUPTED = -1,
	REQUEST_CANCELED = -2,
	DATA_TOO_LONG = -3
};

enum evhttp_connection_error {
	EVCON_HTTP_TIMEOUT,
	EVCON_HTTP_EOF,
	EVCON_HTTP_INVALID_HEADER,
	EVCON_HTTP_BUFFER_ERROR,
	EVCON_HTTP_REQUEST_CANCEL
};

struct evbuffer;
struct addrinfo;
struct evhttp_request;


#define _EVHTTP_REQ_UNKNOWN (1<<15)

enum evhttp_connection_state {
	EVCON_DISCONNECTED,	
	EVCON_CONNECTING,	
	EVCON_IDLE,		
	EVCON_READING_FIRSTLINE,

	EVCON_READING_HEADERS,	
	EVCON_READING_BODY,	
	EVCON_READING_TRAILER,	
	EVCON_WRITING		
};

struct event_base;


struct evhttp_connection {
	

	TAILQ_ENTRY(evhttp_connection) next;

	evutil_socket_t fd;
	struct bufferevent *bufev;

	struct event retry_ev;		

	char *bind_address;		
	u_short bind_port;		

	char *address;			
	u_short port;

	size_t max_headers_size;
	ev_uint64_t max_body_size;

	int flags;
#define EVHTTP_CON_INCOMING	0x0001	/* only one request on it ever */
#define EVHTTP_CON_OUTGOING	0x0002  /* multiple requests possible */
#define EVHTTP_CON_CLOSEDETECT  0x0004  /* detecting if persistent close */

	int timeout;			
	int retry_cnt;			
	int retry_max;			

	enum evhttp_connection_state state;

	
	struct evhttp *http_server;

	TAILQ_HEAD(evcon_requestq, evhttp_request) requests;

	void (*cb)(struct evhttp_connection *, void *);
	void *cb_arg;

	void (*closecb)(struct evhttp_connection *, void *);
	void *closecb_arg;

	struct deferred_cb read_more_deferred_cb;

	struct event_base *base;
	struct evdns_base *dns_base;
};


struct evhttp_cb {
	TAILQ_ENTRY(evhttp_cb) next;

	char *what;

	void (*cb)(struct evhttp_request *req, void *);
	void *cbarg;
};


TAILQ_HEAD(evconq, evhttp_connection);


struct evhttp_bound_socket {
	TAILQ_ENTRY(evhttp_bound_socket) next;

	struct evconnlistener *listener;
};


struct evhttp_server_alias {
	TAILQ_ENTRY(evhttp_server_alias) next;

	char *alias; 
};

struct evhttp {
	
	TAILQ_ENTRY(evhttp) next_vhost;

	
	TAILQ_HEAD(boundq, evhttp_bound_socket) sockets;

	TAILQ_HEAD(httpcbq, evhttp_cb) callbacks;

	
	struct evconq connections;

	TAILQ_HEAD(vhostsq, evhttp) virtualhosts;

	TAILQ_HEAD(aliasq, evhttp_server_alias) aliases;

	
	char *vhost_pattern;

	int timeout;

	size_t default_max_headers_size;
	ev_uint64_t default_max_body_size;

	

	ev_uint16_t allowed_methods;

	

	void (*gencb)(struct evhttp_request *req, void *);
	void *gencbarg;

	struct event_base *base;
};




void evhttp_connection_reset(struct evhttp_connection *);


int evhttp_connection_connect(struct evhttp_connection *);


void evhttp_connection_fail(struct evhttp_connection *,
    enum evhttp_connection_error error);

enum message_read_status;

enum message_read_status evhttp_parse_firstline(struct evhttp_request *, struct evbuffer*);
enum message_read_status evhttp_parse_headers(struct evhttp_request *, struct evbuffer*);

void evhttp_start_read(struct evhttp_connection *);


void evhttp_response_code(struct evhttp_request *, int, const char *);
void evhttp_send_page(struct evhttp_request *, struct evbuffer *);

#endif 
