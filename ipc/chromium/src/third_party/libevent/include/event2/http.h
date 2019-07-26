

























#ifndef _EVENT2_HTTP_H_
#define _EVENT2_HTTP_H_


#include <event2/util.h>

#ifdef __cplusplus
extern "C" {
#endif


struct evbuffer;
struct event_base;













#define HTTP_OK			200	/**< request completed ok */
#define HTTP_NOCONTENT		204	/**< request does not have content */
#define HTTP_MOVEPERM		301	/**< the uri moved permanently */
#define HTTP_MOVETEMP		302	/**< the uri moved temporarily */
#define HTTP_NOTMODIFIED	304	/**< page was not modified from last */
#define HTTP_BADREQUEST		400	/**< invalid http request was made */
#define HTTP_NOTFOUND		404	/**< could not find content for uri */
#define HTTP_BADMETHOD		405 	/**< method not allowed for this uri */
#define HTTP_ENTITYTOOLARGE	413	/**<  */
#define HTTP_EXPECTATIONFAILED	417	/**< we can't handle this expectation */
#define HTTP_INTERNAL           500     /**< internal error */
#define HTTP_NOTIMPLEMENTED     501     /**< not implemented */
#define HTTP_SERVUNAVAIL	503	/**< the server is not available */

struct evhttp;
struct evhttp_request;
struct evkeyvalq;
struct evhttp_bound_socket;
struct evconnlistener;








struct evhttp *evhttp_new(struct event_base *base);













int evhttp_bind_socket(struct evhttp *http, const char *address, ev_uint16_t port);












struct evhttp_bound_socket *evhttp_bind_socket_with_handle(struct evhttp *http, const char *address, ev_uint16_t port);

















int evhttp_accept_socket(struct evhttp *http, evutil_socket_t fd);











struct evhttp_bound_socket *evhttp_accept_socket_with_handle(struct evhttp *http, evutil_socket_t fd);






struct evhttp_bound_socket *evhttp_bind_listener(struct evhttp *http, struct evconnlistener *listener);




struct evconnlistener *evhttp_bound_socket_get_listener(struct evhttp_bound_socket *bound);


















void evhttp_del_accept_socket(struct evhttp *http, struct evhttp_bound_socket *bound_socket);








evutil_socket_t evhttp_bound_socket_get_fd(struct evhttp_bound_socket *bound_socket);









void evhttp_free(struct evhttp* http);


void evhttp_set_max_headers_size(struct evhttp* http, ev_ssize_t max_headers_size);

void evhttp_set_max_body_size(struct evhttp* http, ev_ssize_t max_body_size);












void evhttp_set_allowed_methods(struct evhttp* http, ev_uint16_t methods);










int evhttp_set_cb(struct evhttp *http, const char *path,
    void (*cb)(struct evhttp_request *, void *), void *cb_arg);


int evhttp_del_cb(struct evhttp *, const char *);












void evhttp_set_gencb(struct evhttp *http,
    void (*cb)(struct evhttp_request *, void *), void *arg);























int evhttp_add_virtual_host(struct evhttp* http, const char *pattern,
    struct evhttp* vhost);









int evhttp_remove_virtual_host(struct evhttp* http, struct evhttp* vhost);









int evhttp_add_server_alias(struct evhttp *http, const char *alias);








int evhttp_remove_server_alias(struct evhttp *http, const char *alias);







void evhttp_set_timeout(struct evhttp *http, int timeout_in_secs);











void evhttp_send_error(struct evhttp_request *req, int error,
    const char *reason);














void evhttp_send_reply(struct evhttp_request *req, int code,
    const char *reason, struct evbuffer *databuf);

















void evhttp_send_reply_start(struct evhttp_request *req, int code,
    const char *reason);












void evhttp_send_reply_chunk(struct evhttp_request *req,
    struct evbuffer *databuf);





void evhttp_send_reply_end(struct evhttp_request *req);












enum evhttp_cmd_type {
	EVHTTP_REQ_GET     = 1 << 0,
	EVHTTP_REQ_POST    = 1 << 1,
	EVHTTP_REQ_HEAD    = 1 << 2,
	EVHTTP_REQ_PUT     = 1 << 3,
	EVHTTP_REQ_DELETE  = 1 << 4,
	EVHTTP_REQ_OPTIONS = 1 << 5,
	EVHTTP_REQ_TRACE   = 1 << 6,
	EVHTTP_REQ_CONNECT = 1 << 7,
	EVHTTP_REQ_PATCH   = 1 << 8
};


enum evhttp_request_kind { EVHTTP_REQUEST, EVHTTP_RESPONSE };






struct evhttp_request *evhttp_request_new(
	void (*cb)(struct evhttp_request *, void *), void *arg);








void evhttp_request_set_chunked_cb(struct evhttp_request *,
    void (*cb)(struct evhttp_request *, void *));


void evhttp_request_free(struct evhttp_request *req);

struct evdns_base;













struct evhttp_connection *evhttp_connection_base_new(
	struct event_base *base, struct evdns_base *dnsbase,
	const char *address, unsigned short port);




struct bufferevent *evhttp_connection_get_bufferevent(
	struct evhttp_connection *evcon);






void evhttp_request_own(struct evhttp_request *req);


int evhttp_request_is_owned(struct evhttp_request *req);







struct evhttp_connection *evhttp_request_get_connection(struct evhttp_request *req);




struct event_base *evhttp_connection_get_base(struct evhttp_connection *req);

void evhttp_connection_set_max_headers_size(struct evhttp_connection *evcon,
    ev_ssize_t new_max_headers_size);

void evhttp_connection_set_max_body_size(struct evhttp_connection* evcon,
    ev_ssize_t new_max_body_size);


void evhttp_connection_free(struct evhttp_connection *evcon);


void evhttp_connection_set_local_address(struct evhttp_connection *evcon,
    const char *address);


void evhttp_connection_set_local_port(struct evhttp_connection *evcon,
    ev_uint16_t port);


void evhttp_connection_set_timeout(struct evhttp_connection *evcon,
    int timeout_in_secs);


void evhttp_connection_set_retries(struct evhttp_connection *evcon,
    int retry_max);


void evhttp_connection_set_closecb(struct evhttp_connection *evcon,
    void (*)(struct evhttp_connection *, void *), void *);


void evhttp_connection_get_peer(struct evhttp_connection *evcon,
    char **address, ev_uint16_t *port);














int evhttp_make_request(struct evhttp_connection *evcon,
    struct evhttp_request *req,
    enum evhttp_cmd_type type, const char *uri);














void evhttp_cancel_request(struct evhttp_request *req);




struct evhttp_uri;


const char *evhttp_request_get_uri(const struct evhttp_request *req);

const struct evhttp_uri *evhttp_request_get_evhttp_uri(const struct evhttp_request *req);

enum evhttp_cmd_type evhttp_request_get_command(const struct evhttp_request *req);

int evhttp_request_get_response_code(const struct evhttp_request *req);


struct evkeyvalq *evhttp_request_get_input_headers(struct evhttp_request *req);

struct evkeyvalq *evhttp_request_get_output_headers(struct evhttp_request *req);

struct evbuffer *evhttp_request_get_input_buffer(struct evhttp_request *req);

struct evbuffer *evhttp_request_get_output_buffer(struct evhttp_request *req);




const char *evhttp_request_get_host(struct evhttp_request *req);












const char *evhttp_find_header(const struct evkeyvalq *headers,
    const char *key);









int evhttp_remove_header(struct evkeyvalq *headers, const char *key);










int evhttp_add_header(struct evkeyvalq *headers, const char *key, const char *value);






void evhttp_clear_headers(struct evkeyvalq *headers);















char *evhttp_encode_uri(const char *str);















char *evhttp_uriencode(const char *str, ev_ssize_t size, int space_to_plus);















char *evhttp_decode_uri(const char *uri);
















char *evhttp_uridecode(const char *uri, int decode_plus,
    size_t *size_out);




















int evhttp_parse_query(const char *uri, struct evkeyvalq *headers);


















int evhttp_parse_query_str(const char *uri, struct evkeyvalq *headers);












char *evhttp_htmlescape(const char *html);




struct evhttp_uri *evhttp_uri_new(void);





void evhttp_uri_set_flags(struct evhttp_uri *uri, unsigned flags);



const char *evhttp_uri_get_scheme(const struct evhttp_uri *uri);




const char *evhttp_uri_get_userinfo(const struct evhttp_uri *uri);












const char *evhttp_uri_get_host(const struct evhttp_uri *uri);

int evhttp_uri_get_port(const struct evhttp_uri *uri);

const char *evhttp_uri_get_path(const struct evhttp_uri *uri);


const char *evhttp_uri_get_query(const struct evhttp_uri *uri);


const char *evhttp_uri_get_fragment(const struct evhttp_uri *uri);



int evhttp_uri_set_scheme(struct evhttp_uri *uri, const char *scheme);


int evhttp_uri_set_userinfo(struct evhttp_uri *uri, const char *userinfo);


int evhttp_uri_set_host(struct evhttp_uri *uri, const char *host);


int evhttp_uri_set_port(struct evhttp_uri *uri, int port);


int evhttp_uri_set_path(struct evhttp_uri *uri, const char *path);



int evhttp_uri_set_query(struct evhttp_uri *uri, const char *query);



int evhttp_uri_set_fragment(struct evhttp_uri *uri, const char *fragment);



































struct evhttp_uri *evhttp_uri_parse_with_flags(const char *source_uri,
    unsigned flags);













#define EVHTTP_URI_NONCONFORMANT 0x01


struct evhttp_uri *evhttp_uri_parse(const char *source_uri);








void evhttp_uri_free(struct evhttp_uri *uri);














char *evhttp_uri_join(struct evhttp_uri *uri, char *buf, size_t limit);

#ifdef __cplusplus
}
#endif

#endif
