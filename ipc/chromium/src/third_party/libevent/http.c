


























#include "event2/event-config.h"

#ifdef _EVENT_HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef _EVENT_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef _EVENT_HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_IOCCOM_H
#include <sys/ioccom.h>
#endif

#ifndef WIN32
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include <sys/queue.h>

#ifdef _EVENT_HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef _EVENT_HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef _EVENT_HAVE_NETDB_H
#include <netdb.h>
#endif

#ifdef WIN32
#include <winsock2.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include <syslog.h>
#endif
#include <signal.h>
#include <time.h>
#ifdef _EVENT_HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef _EVENT_HAVE_FCNTL_H
#include <fcntl.h>
#endif

#undef timeout_pending
#undef timeout_initialized

#include "strlcpy-internal.h"
#include "event2/http.h"
#include "event2/event.h"
#include "event2/buffer.h"
#include "event2/bufferevent.h"
#include "event2/bufferevent_compat.h"
#include "event2/http_struct.h"
#include "event2/http_compat.h"
#include "event2/util.h"
#include "event2/listener.h"
#include "log-internal.h"
#include "util-internal.h"
#include "http-internal.h"
#include "mm-internal.h"
#include "bufferevent-internal.h"

#ifndef _EVENT_HAVE_GETNAMEINFO
#define NI_MAXSERV 32
#define NI_MAXHOST 1025

#ifndef NI_NUMERICHOST
#define NI_NUMERICHOST 1
#endif

#ifndef NI_NUMERICSERV
#define NI_NUMERICSERV 2
#endif

static int
fake_getnameinfo(const struct sockaddr *sa, size_t salen, char *host,
	size_t hostlen, char *serv, size_t servlen, int flags)
{
	struct sockaddr_in *sin = (struct sockaddr_in *)sa;

	if (serv != NULL) {
		char tmpserv[16];
		evutil_snprintf(tmpserv, sizeof(tmpserv),
		    "%d", ntohs(sin->sin_port));
		if (strlcpy(serv, tmpserv, servlen) >= servlen)
			return (-1);
	}

	if (host != NULL) {
		if (flags & NI_NUMERICHOST) {
			if (strlcpy(host, inet_ntoa(sin->sin_addr),
			    hostlen) >= hostlen)
				return (-1);
			else
				return (0);
		} else {
			struct hostent *hp;
			hp = gethostbyaddr((char *)&sin->sin_addr,
			    sizeof(struct in_addr), AF_INET);
			if (hp == NULL)
				return (-2);

			if (strlcpy(host, hp->h_name, hostlen) >= hostlen)
				return (-1);
			else
				return (0);
		}
	}
	return (0);
}

#endif

#define REQ_VERSION_BEFORE(req, major_v, minor_v)			\
	((req)->major < (major_v) ||					\
	    ((req)->major == (major_v) && (req)->minor < (minor_v)))

#define REQ_VERSION_ATLEAST(req, major_v, minor_v)			\
	((req)->major > (major_v) ||					\
	    ((req)->major == (major_v) && (req)->minor >= (minor_v)))

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

extern int debug;

static evutil_socket_t bind_socket_ai(struct evutil_addrinfo *, int reuse);
static evutil_socket_t bind_socket(const char *, ev_uint16_t, int reuse);
static void name_from_addr(struct sockaddr *, ev_socklen_t, char **, char **);
static int evhttp_associate_new_request_with_connection(
	struct evhttp_connection *evcon);
static void evhttp_connection_start_detectclose(
	struct evhttp_connection *evcon);
static void evhttp_connection_stop_detectclose(
	struct evhttp_connection *evcon);
static void evhttp_request_dispatch(struct evhttp_connection* evcon);
static void evhttp_read_firstline(struct evhttp_connection *evcon,
				  struct evhttp_request *req);
static void evhttp_read_header(struct evhttp_connection *evcon,
    struct evhttp_request *req);
static int evhttp_add_header_internal(struct evkeyvalq *headers,
    const char *key, const char *value);
static const char *evhttp_response_phrase_internal(int code);
static void evhttp_get_request(struct evhttp *, evutil_socket_t, struct sockaddr *, ev_socklen_t);
static void evhttp_write_buffer(struct evhttp_connection *,
    void (*)(struct evhttp_connection *, void *), void *);
static void evhttp_make_header(struct evhttp_connection *, struct evhttp_request *);


static void evhttp_read_cb(struct bufferevent *, void *);
static void evhttp_write_cb(struct bufferevent *, void *);
static void evhttp_error_cb(struct bufferevent *bufev, short what, void *arg);
static int evhttp_decode_uri_internal(const char *uri, size_t length,
    char *ret, int decode_plus);
static int evhttp_find_vhost(struct evhttp *http, struct evhttp **outhttp,
		  const char *hostname);

#ifndef _EVENT_HAVE_STRSEP


static char *
strsep(char **s, const char *del)
{
	char *d, *tok;
	EVUTIL_ASSERT(strlen(del) == 1);
	if (!s || !*s)
		return NULL;
	tok = *s;
	d = strstr(tok, del);
	if (d) {
		*d = '\0';
		*s = d + 1;
	} else
		*s = NULL;
	return tok;
}
#endif

static size_t
html_replace(const char ch, const char **escaped)
{
	switch (ch) {
	case '<':
		*escaped = "&lt;";
		return 4;
	case '>':
		*escaped = "&gt;";
		return 4;
	case '"':
		*escaped = "&quot;";
		return 6;
	case '\'':
		*escaped = "&#039;";
		return 6;
	case '&':
		*escaped = "&amp;";
		return 5;
	default:
		break;
	}

	return 1;
}








char *
evhttp_htmlescape(const char *html)
{
	size_t i;
	size_t new_size = 0, old_size = 0;
	char *escaped_html, *p;

	if (html == NULL)
		return (NULL);

	old_size = strlen(html);
	for (i = 0; i < old_size; ++i) {
		const char *replaced = NULL;
		const size_t replace_size = html_replace(html[i], &replaced);
		if (replace_size > EV_SIZE_MAX - new_size) {
			event_warn("%s: html_replace overflow", __func__);
			return (NULL);
		}
		new_size += replace_size;
	}

	if (new_size == EV_SIZE_MAX)
		return (NULL);
	p = escaped_html = mm_malloc(new_size + 1);
	if (escaped_html == NULL) {
		event_warn("%s: malloc(%lu)", __func__,
		           (unsigned long)(new_size + 1));
		return (NULL);
	}
	for (i = 0; i < old_size; ++i) {
		const char *replaced = &html[i];
		const size_t len = html_replace(html[i], &replaced);
		memcpy(p, replaced, len);
		p += len;
	}

	*p = '\0';

	return (escaped_html);
}




static const char *
evhttp_method(enum evhttp_cmd_type type)
{
	const char *method;

	switch (type) {
	case EVHTTP_REQ_GET:
		method = "GET";
		break;
	case EVHTTP_REQ_POST:
		method = "POST";
		break;
	case EVHTTP_REQ_HEAD:
		method = "HEAD";
		break;
	case EVHTTP_REQ_PUT:
		method = "PUT";
		break;
	case EVHTTP_REQ_DELETE:
		method = "DELETE";
		break;
	case EVHTTP_REQ_OPTIONS:
		method = "OPTIONS";
		break;
	case EVHTTP_REQ_TRACE:
		method = "TRACE";
		break;
	case EVHTTP_REQ_CONNECT:
		method = "CONNECT";
		break;
	case EVHTTP_REQ_PATCH:
		method = "PATCH";
		break;
	default:
		method = NULL;
		break;
	}

	return (method);
}







static int
evhttp_response_needs_body(struct evhttp_request *req)
{
	return (req->response_code != HTTP_NOCONTENT &&
		req->response_code != HTTP_NOTMODIFIED &&
		(req->response_code < 100 || req->response_code >= 200) &&
		req->type != EVHTTP_REQ_HEAD);
}




static int
evhttp_add_event(struct event *ev, int timeout, int default_timeout)
{
	if (timeout != 0) {
		struct timeval tv;

		evutil_timerclear(&tv);
		tv.tv_sec = timeout != -1 ? timeout : default_timeout;
		return event_add(ev, &tv);
	} else {
		return event_add(ev, NULL);
	}
}





static void
evhttp_write_buffer(struct evhttp_connection *evcon,
    void (*cb)(struct evhttp_connection *, void *), void *arg)
{
	event_debug(("%s: preparing to write buffer\n", __func__));

	
	evcon->cb = cb;
	evcon->cb_arg = arg;

	bufferevent_enable(evcon->bufev, EV_WRITE);

	


	bufferevent_setcb(evcon->bufev,
	    NULL, 
	    evhttp_write_cb,
	    evhttp_error_cb,
	    evcon);
}

static void
evhttp_send_continue_done(struct evhttp_connection *evcon, void *arg)
{
	bufferevent_disable(evcon->bufev, EV_WRITE);
}

static void
evhttp_send_continue(struct evhttp_connection *evcon,
			struct evhttp_request *req)
{
	bufferevent_enable(evcon->bufev, EV_WRITE);
	evbuffer_add_printf(bufferevent_get_output(evcon->bufev),
			"HTTP/%d.%d 100 Continue\r\n\r\n",
			req->major, req->minor);
	evcon->cb = evhttp_send_continue_done;
	evcon->cb_arg = NULL;
	bufferevent_setcb(evcon->bufev,
	    evhttp_read_cb,
	    evhttp_write_cb,
	    evhttp_error_cb,
	    evcon);
}


static int
evhttp_connected(struct evhttp_connection *evcon)
{
	switch (evcon->state) {
	case EVCON_DISCONNECTED:
	case EVCON_CONNECTING:
		return (0);
	case EVCON_IDLE:
	case EVCON_READING_FIRSTLINE:
	case EVCON_READING_HEADERS:
	case EVCON_READING_BODY:
	case EVCON_READING_TRAILER:
	case EVCON_WRITING:
	default:
		return (1);
	}
}





static void
evhttp_make_header_request(struct evhttp_connection *evcon,
    struct evhttp_request *req)
{
	const char *method;

	evhttp_remove_header(req->output_headers, "Proxy-Connection");

	
	method = evhttp_method(req->type);
	evbuffer_add_printf(bufferevent_get_output(evcon->bufev),
	    "%s %s HTTP/%d.%d\r\n",
	    method, req->uri, req->major, req->minor);

	
	if ((req->type == EVHTTP_REQ_POST || req->type == EVHTTP_REQ_PUT) &&
	    evhttp_find_header(req->output_headers, "Content-Length") == NULL){
		char size[22];
		evutil_snprintf(size, sizeof(size), EV_SIZE_FMT,
		    EV_SIZE_ARG(evbuffer_get_length(req->output_buffer)));
		evhttp_add_header(req->output_headers, "Content-Length", size);
	}
}




static int
evhttp_is_connection_close(int flags, struct evkeyvalq* headers)
{
	if (flags & EVHTTP_PROXY_REQUEST) {
		
		const char *connection = evhttp_find_header(headers, "Proxy-Connection");
		return (connection == NULL || evutil_ascii_strcasecmp(connection, "keep-alive") != 0);
	} else {
		const char *connection = evhttp_find_header(headers, "Connection");
		return (connection != NULL && evutil_ascii_strcasecmp(connection, "close") == 0);
	}
}


static int
evhttp_is_connection_keepalive(struct evkeyvalq* headers)
{
	const char *connection = evhttp_find_header(headers, "Connection");
	return (connection != NULL
	    && evutil_ascii_strncasecmp(connection, "keep-alive", 10) == 0);
}


static void
evhttp_maybe_add_date_header(struct evkeyvalq *headers)
{
	if (evhttp_find_header(headers, "Date") == NULL) {
		char date[50];
#ifndef WIN32
		struct tm cur;
#endif
		struct tm *cur_p;
		time_t t = time(NULL);
#ifdef WIN32
		cur_p = gmtime(&t);
#else
		gmtime_r(&t, &cur);
		cur_p = &cur;
#endif
		if (strftime(date, sizeof(date),
			"%a, %d %b %Y %H:%M:%S GMT", cur_p) != 0) {
			evhttp_add_header(headers, "Date", date);
		}
	}
}



static void
evhttp_maybe_add_content_length_header(struct evkeyvalq *headers,
    size_t content_length)
{
	if (evhttp_find_header(headers, "Transfer-Encoding") == NULL &&
	    evhttp_find_header(headers,	"Content-Length") == NULL) {
		char len[22];
		evutil_snprintf(len, sizeof(len), EV_SIZE_FMT,
		    EV_SIZE_ARG(content_length));
		evhttp_add_header(headers, "Content-Length", len);
	}
}





static void
evhttp_make_header_response(struct evhttp_connection *evcon,
    struct evhttp_request *req)
{
	int is_keepalive = evhttp_is_connection_keepalive(req->input_headers);
	evbuffer_add_printf(bufferevent_get_output(evcon->bufev),
	    "HTTP/%d.%d %d %s\r\n",
	    req->major, req->minor, req->response_code,
	    req->response_code_line);

	if (req->major == 1) {
		if (req->minor >= 1)
			evhttp_maybe_add_date_header(req->output_headers);

		



		if (req->minor == 0 && is_keepalive)
			evhttp_add_header(req->output_headers,
			    "Connection", "keep-alive");

		if ((req->minor >= 1 || is_keepalive) &&
		    evhttp_response_needs_body(req)) {
			




			evhttp_maybe_add_content_length_header(
				req->output_headers,
				evbuffer_get_length(req->output_buffer));
		}
	}

	
	if (evhttp_response_needs_body(req)) {
		if (evhttp_find_header(req->output_headers,
			"Content-Type") == NULL) {
			evhttp_add_header(req->output_headers,
			    "Content-Type", "text/html; charset=ISO-8859-1");
		}
	}

	
	if (evhttp_is_connection_close(req->flags, req->input_headers)) {
		evhttp_remove_header(req->output_headers, "Connection");
		if (!(req->flags & EVHTTP_PROXY_REQUEST))
		    evhttp_add_header(req->output_headers, "Connection", "close");
		evhttp_remove_header(req->output_headers, "Proxy-Connection");
	}
}




static void
evhttp_make_header(struct evhttp_connection *evcon, struct evhttp_request *req)
{
	struct evkeyval *header;
	struct evbuffer *output = bufferevent_get_output(evcon->bufev);

	



	if (req->kind == EVHTTP_REQUEST) {
		evhttp_make_header_request(evcon, req);
	} else {
		evhttp_make_header_response(evcon, req);
	}

	TAILQ_FOREACH(header, req->output_headers, next) {
		evbuffer_add_printf(output, "%s: %s\r\n",
		    header->key, header->value);
	}
	evbuffer_add(output, "\r\n", 2);

	if (evbuffer_get_length(req->output_buffer) > 0) {
		



		


		evbuffer_add_buffer(output, req->output_buffer);
	}
}

void
evhttp_connection_set_max_headers_size(struct evhttp_connection *evcon,
    ev_ssize_t new_max_headers_size)
{
	if (new_max_headers_size<0)
		evcon->max_headers_size = EV_SIZE_MAX;
	else
		evcon->max_headers_size = new_max_headers_size;
}
void
evhttp_connection_set_max_body_size(struct evhttp_connection* evcon,
    ev_ssize_t new_max_body_size)
{
	if (new_max_body_size<0)
		evcon->max_body_size = EV_UINT64_MAX;
	else
		evcon->max_body_size = new_max_body_size;
}

static int
evhttp_connection_incoming_fail(struct evhttp_request *req,
    enum evhttp_connection_error error)
{
	switch (error) {
	case EVCON_HTTP_TIMEOUT:
	case EVCON_HTTP_EOF:
		







		if (!req->userdone) {
			
			TAILQ_REMOVE(&req->evcon->requests, req, next);
			


			req->evcon = NULL;
		}
		return (-1);
	case EVCON_HTTP_INVALID_HEADER:
	case EVCON_HTTP_BUFFER_ERROR:
	case EVCON_HTTP_REQUEST_CANCEL:
	default:	
		
		if (req->uri) {
			mm_free(req->uri);
			req->uri = NULL;
		}
		if (req->uri_elems) {
			evhttp_uri_free(req->uri_elems);
			req->uri_elems = NULL;
		}

		



		(*req->cb)(req, req->cb_arg);
	}

	return (0);
}





void
evhttp_connection_fail(struct evhttp_connection *evcon,
    enum evhttp_connection_error error)
{
	struct evhttp_request* req = TAILQ_FIRST(&evcon->requests);
	void (*cb)(struct evhttp_request *, void *);
	void *cb_arg;
	EVUTIL_ASSERT(req != NULL);

	bufferevent_disable(evcon->bufev, EV_READ|EV_WRITE);

	if (evcon->flags & EVHTTP_CON_INCOMING) {
		







		if (evhttp_connection_incoming_fail(req, error) == -1)
			evhttp_connection_free(evcon);
		return;
	}

	
	if (error != EVCON_HTTP_REQUEST_CANCEL) {
		
		cb = req->cb;
		cb_arg = req->cb_arg;
	} else {
		cb = NULL;
		cb_arg = NULL;
	}

	



	TAILQ_REMOVE(&evcon->requests, req, next);
	evhttp_request_free(req);

	
	evhttp_connection_reset(evcon);

	
	if (TAILQ_FIRST(&evcon->requests) != NULL)
		evhttp_connection_connect(evcon);

	
	if (cb != NULL)
		(*cb)(NULL, cb_arg);
}



static void
evhttp_write_cb(struct bufferevent *bufev, void *arg)
{
	struct evhttp_connection *evcon = arg;

	
	if (evcon->cb != NULL)
		(*evcon->cb)(evcon, evcon->cb_arg);
}








static void
evhttp_connection_done(struct evhttp_connection *evcon)
{
	struct evhttp_request *req = TAILQ_FIRST(&evcon->requests);
	int con_outgoing = evcon->flags & EVHTTP_CON_OUTGOING;

	if (con_outgoing) {
		
		int need_close;
		TAILQ_REMOVE(&evcon->requests, req, next);
		req->evcon = NULL;

		evcon->state = EVCON_IDLE;

		need_close =
		    evhttp_is_connection_close(req->flags, req->input_headers)||
		    evhttp_is_connection_close(req->flags, req->output_headers);

		
		if (need_close)
			evhttp_connection_reset(evcon);

		if (TAILQ_FIRST(&evcon->requests) != NULL) {
			



			if (!evhttp_connected(evcon))
				evhttp_connection_connect(evcon);
			else
				evhttp_request_dispatch(evcon);
		} else if (!need_close) {
			



			evhttp_connection_start_detectclose(evcon);
		}
	} else {
		



		evcon->state = EVCON_WRITING;
	}

	
	(*req->cb)(req, req->cb_arg);

	


	if (con_outgoing && ((req->flags & EVHTTP_USER_OWNED) == 0)) {
		evhttp_request_free(req);
	}
}















static enum message_read_status
evhttp_handle_chunked_read(struct evhttp_request *req, struct evbuffer *buf)
{
	if (req == NULL || buf == NULL) {
	    return DATA_CORRUPTED;
	}

	while (1) {
		size_t buflen;

		if ((buflen = evbuffer_get_length(buf)) == 0) {
			break;
		}

		

		if (buflen > EV_SSIZE_MAX) {
			return DATA_CORRUPTED;
		}

		if (req->ntoread < 0) {
			
			ev_int64_t ntoread;
			char *p = evbuffer_readln(buf, NULL, EVBUFFER_EOL_CRLF);
			char *endp;
			int error;
			if (p == NULL)
				break;
			
			if (strlen(p) == 0) {
				mm_free(p);
				continue;
			}
			ntoread = evutil_strtoll(p, &endp, 16);
			error = (*p == '\0' ||
			    (*endp != '\0' && *endp != ' ') ||
			    ntoread < 0);
			mm_free(p);
			if (error) {
				
				return (DATA_CORRUPTED);
			}

			
			if ((ev_uint64_t)ntoread > EV_SIZE_MAX - req->body_size) {
			    return DATA_CORRUPTED;
			}

			if (req->body_size + (size_t)ntoread > req->evcon->max_body_size) {
				
				event_debug(("Request body is too long"));
				return (DATA_TOO_LONG);
			}

			req->body_size += (size_t)ntoread;
			req->ntoread = ntoread;
			if (req->ntoread == 0) {
				
				return (ALL_DATA_READ);
			}
			continue;
		}

		

		if (req->ntoread > EV_SSIZE_MAX) {
			return DATA_CORRUPTED;
		}

		
		if (req->ntoread > 0 && buflen < (ev_uint64_t)req->ntoread)
			return (MORE_DATA_EXPECTED);

		
		evbuffer_remove_buffer(buf, req->input_buffer, (size_t)req->ntoread);
		req->ntoread = -1;
		if (req->chunk_cb != NULL) {
			req->flags |= EVHTTP_REQ_DEFER_FREE;
			(*req->chunk_cb)(req, req->cb_arg);
			evbuffer_drain(req->input_buffer,
			    evbuffer_get_length(req->input_buffer));
			req->flags &= ~EVHTTP_REQ_DEFER_FREE;
			if ((req->flags & EVHTTP_REQ_NEEDS_FREE) != 0) {
				return (REQUEST_CANCELED);
			}
		}
	}

	return (MORE_DATA_EXPECTED);
}

static void
evhttp_read_trailer(struct evhttp_connection *evcon, struct evhttp_request *req)
{
	struct evbuffer *buf = bufferevent_get_input(evcon->bufev);

	switch (evhttp_parse_headers(req, buf)) {
	case DATA_CORRUPTED:
	case DATA_TOO_LONG:
		evhttp_connection_fail(evcon, EVCON_HTTP_INVALID_HEADER);
		break;
	case ALL_DATA_READ:
		bufferevent_disable(evcon->bufev, EV_READ);
		evhttp_connection_done(evcon);
		break;
	case MORE_DATA_EXPECTED:
	case REQUEST_CANCELED: 
	default:
		bufferevent_enable(evcon->bufev, EV_READ);
		break;
	}
}

static void
evhttp_read_body(struct evhttp_connection *evcon, struct evhttp_request *req)
{
	struct evbuffer *buf = bufferevent_get_input(evcon->bufev);

	if (req->chunked) {
		switch (evhttp_handle_chunked_read(req, buf)) {
		case ALL_DATA_READ:
			
			evcon->state = EVCON_READING_TRAILER;
			evhttp_read_trailer(evcon, req);
			return;
		case DATA_CORRUPTED:
		case DATA_TOO_LONG:
			
			evhttp_connection_fail(evcon,
			    EVCON_HTTP_INVALID_HEADER);
			return;
		case REQUEST_CANCELED:
			
			evhttp_request_free(req);
			return;
		case MORE_DATA_EXPECTED:
		default:
			break;
		}
	} else if (req->ntoread < 0) {
		
		if ((size_t)(req->body_size + evbuffer_get_length(buf)) < req->body_size) {
			evhttp_connection_fail(evcon, EVCON_HTTP_INVALID_HEADER);
			return;
		}

		req->body_size += evbuffer_get_length(buf);
		evbuffer_add_buffer(req->input_buffer, buf);
	} else if (req->chunk_cb != NULL || evbuffer_get_length(buf) >= (size_t)req->ntoread) {
		
		

		size_t n = evbuffer_get_length(buf);

		if (n > (size_t) req->ntoread)
			n = (size_t) req->ntoread;
		req->ntoread -= n;
		req->body_size += n;
		evbuffer_remove_buffer(buf, req->input_buffer, n);
	}

	if (req->body_size > req->evcon->max_body_size ||
	    (!req->chunked && req->ntoread >= 0 &&
		(size_t)req->ntoread > req->evcon->max_body_size)) {
		
		
		event_debug(("Request body is too long"));
		evhttp_connection_fail(evcon,
				       EVCON_HTTP_INVALID_HEADER);
		return;
	}

	if (evbuffer_get_length(req->input_buffer) > 0 && req->chunk_cb != NULL) {
		req->flags |= EVHTTP_REQ_DEFER_FREE;
		(*req->chunk_cb)(req, req->cb_arg);
		req->flags &= ~EVHTTP_REQ_DEFER_FREE;
		evbuffer_drain(req->input_buffer,
		    evbuffer_get_length(req->input_buffer));
		if ((req->flags & EVHTTP_REQ_NEEDS_FREE) != 0) {
			evhttp_request_free(req);
			return;
		}
	}

	if (req->ntoread == 0) {
		bufferevent_disable(evcon->bufev, EV_READ);
		
		evhttp_connection_done(evcon);
		return;
	}

	
	bufferevent_enable(evcon->bufev, EV_READ);
}

#define get_deferred_queue(evcon)		\
	(event_base_get_deferred_cb_queue((evcon)->base))





static void
evhttp_read_cb(struct bufferevent *bufev, void *arg)
{
	struct evhttp_connection *evcon = arg;
	struct evhttp_request *req = TAILQ_FIRST(&evcon->requests);

	
	event_deferred_cb_cancel(get_deferred_queue(evcon),
	    &evcon->read_more_deferred_cb);

	switch (evcon->state) {
	case EVCON_READING_FIRSTLINE:
		evhttp_read_firstline(evcon, req);
		

		break;
	case EVCON_READING_HEADERS:
		evhttp_read_header(evcon, req);
		

		break;
	case EVCON_READING_BODY:
		evhttp_read_body(evcon, req);
		

		break;
	case EVCON_READING_TRAILER:
		evhttp_read_trailer(evcon, req);
		break;
	case EVCON_IDLE:
		{
#ifdef USE_DEBUG
			struct evbuffer *input;
			size_t total_len;

			input = bufferevent_get_input(evcon->bufev);
			total_len = evbuffer_get_length(input);
			event_debug(("%s: read "EV_SIZE_FMT
				" bytes in EVCON_IDLE state,"
				" resetting connection",
				__func__, EV_SIZE_ARG(total_len)));
#endif

			evhttp_connection_reset(evcon);
		}
		break;
	case EVCON_DISCONNECTED:
	case EVCON_CONNECTING:
	case EVCON_WRITING:
	default:
		event_errx(1, "%s: illegal connection state %d",
			   __func__, evcon->state);
	}
}

static void
evhttp_deferred_read_cb(struct deferred_cb *cb, void *data)
{
	struct evhttp_connection *evcon = data;
	evhttp_read_cb(evcon->bufev, evcon);
}

static void
evhttp_write_connectioncb(struct evhttp_connection *evcon, void *arg)
{
	
	struct evhttp_request *req = TAILQ_FIRST(&evcon->requests);
	EVUTIL_ASSERT(req != NULL);

	EVUTIL_ASSERT(evcon->state == EVCON_WRITING);

	
	req->kind = EVHTTP_RESPONSE;

	evhttp_start_read(evcon);
}





void
evhttp_connection_free(struct evhttp_connection *evcon)
{
	struct evhttp_request *req;

	
	if (evcon->fd != -1) {
		if (evhttp_connected(evcon) && evcon->closecb != NULL)
			(*evcon->closecb)(evcon, evcon->closecb_arg);
	}

	




	while ((req = TAILQ_FIRST(&evcon->requests)) != NULL) {
		TAILQ_REMOVE(&evcon->requests, req, next);
		evhttp_request_free(req);
	}

	if (evcon->http_server != NULL) {
		struct evhttp *http = evcon->http_server;
		TAILQ_REMOVE(&http->connections, evcon, next);
	}

	if (event_initialized(&evcon->retry_ev)) {
		event_del(&evcon->retry_ev);
		event_debug_unassign(&evcon->retry_ev);
	}

	if (evcon->bufev != NULL)
		bufferevent_free(evcon->bufev);

	event_deferred_cb_cancel(get_deferred_queue(evcon),
	    &evcon->read_more_deferred_cb);

	if (evcon->fd != -1) {
		shutdown(evcon->fd, EVUTIL_SHUT_WR);
		evutil_closesocket(evcon->fd);
	}

	if (evcon->bind_address != NULL)
		mm_free(evcon->bind_address);

	if (evcon->address != NULL)
		mm_free(evcon->address);

	mm_free(evcon);
}

void
evhttp_connection_set_local_address(struct evhttp_connection *evcon,
    const char *address)
{
	EVUTIL_ASSERT(evcon->state == EVCON_DISCONNECTED);
	if (evcon->bind_address)
		mm_free(evcon->bind_address);
	if ((evcon->bind_address = mm_strdup(address)) == NULL)
		event_warn("%s: strdup", __func__);
}

void
evhttp_connection_set_local_port(struct evhttp_connection *evcon,
    ev_uint16_t port)
{
	EVUTIL_ASSERT(evcon->state == EVCON_DISCONNECTED);
	evcon->bind_port = port;
}

static void
evhttp_request_dispatch(struct evhttp_connection* evcon)
{
	struct evhttp_request *req = TAILQ_FIRST(&evcon->requests);

	
	if (req == NULL)
		return;

	
	evhttp_connection_stop_detectclose(evcon);

	
	EVUTIL_ASSERT(evcon->state == EVCON_IDLE);

	evcon->state = EVCON_WRITING;

	
	evhttp_make_header(evcon, req);

	evhttp_write_buffer(evcon, evhttp_write_connectioncb, NULL);
}



void
evhttp_connection_reset(struct evhttp_connection *evcon)
{
	struct evbuffer *tmp;

	










	bufferevent_disable_hard(evcon->bufev, EV_READ|EV_WRITE);

	if (evcon->fd != -1) {
		
		if (evhttp_connected(evcon) && evcon->closecb != NULL)
			(*evcon->closecb)(evcon, evcon->closecb_arg);

		shutdown(evcon->fd, EVUTIL_SHUT_WR);
		evutil_closesocket(evcon->fd);
		evcon->fd = -1;
	}

	
	tmp = bufferevent_get_output(evcon->bufev);
	evbuffer_drain(tmp, evbuffer_get_length(tmp));
	tmp = bufferevent_get_input(evcon->bufev);
	evbuffer_drain(tmp, evbuffer_get_length(tmp));

	evcon->state = EVCON_DISCONNECTED;
}

static void
evhttp_connection_start_detectclose(struct evhttp_connection *evcon)
{
	evcon->flags |= EVHTTP_CON_CLOSEDETECT;

	bufferevent_enable(evcon->bufev, EV_READ);
}

static void
evhttp_connection_stop_detectclose(struct evhttp_connection *evcon)
{
	evcon->flags &= ~EVHTTP_CON_CLOSEDETECT;

	bufferevent_disable(evcon->bufev, EV_READ);
}

static void
evhttp_connection_retry(evutil_socket_t fd, short what, void *arg)
{
	struct evhttp_connection *evcon = arg;

	evcon->state = EVCON_DISCONNECTED;
	evhttp_connection_connect(evcon);
}

static void
evhttp_connection_cb_cleanup(struct evhttp_connection *evcon)
{
	struct evcon_requestq requests;

	if (evcon->retry_max < 0 || evcon->retry_cnt < evcon->retry_max) {
		evtimer_assign(&evcon->retry_ev, evcon->base, evhttp_connection_retry, evcon);
		
		evhttp_add_event(&evcon->retry_ev,
		    MIN(3600, 2 << evcon->retry_cnt),
		    HTTP_CONNECT_TIMEOUT);
		evcon->retry_cnt++;
		return;
	}
	evhttp_connection_reset(evcon);

	





	TAILQ_INIT(&requests);
	while (TAILQ_FIRST(&evcon->requests) != NULL) {
		struct evhttp_request *request = TAILQ_FIRST(&evcon->requests);
		TAILQ_REMOVE(&evcon->requests, request, next);
		TAILQ_INSERT_TAIL(&requests, request, next);
	}

	
	while (TAILQ_FIRST(&requests) != NULL) {
		struct evhttp_request *request = TAILQ_FIRST(&requests);
		TAILQ_REMOVE(&requests, request, next);
		request->evcon = NULL;

		
		request->cb(request, request->cb_arg);
		evhttp_request_free(request);
	}
}

static void
evhttp_error_cb(struct bufferevent *bufev, short what, void *arg)
{
	struct evhttp_connection *evcon = arg;
	struct evhttp_request *req = TAILQ_FIRST(&evcon->requests);

	switch (evcon->state) {
	case EVCON_CONNECTING:
		if (what & BEV_EVENT_TIMEOUT) {
			event_debug(("%s: connection timeout for \"%s:%d\" on "
				EV_SOCK_FMT,
				__func__, evcon->address, evcon->port,
				EV_SOCK_ARG(evcon->fd)));
			evhttp_connection_cb_cleanup(evcon);
			return;
		}
		break;

	case EVCON_READING_BODY:
		if (!req->chunked && req->ntoread < 0
		    && what == (BEV_EVENT_READING|BEV_EVENT_EOF)) {
			
			evhttp_connection_done(evcon);
			return;
		}
		break;

	case EVCON_DISCONNECTED:
	case EVCON_IDLE:
	case EVCON_READING_FIRSTLINE:
	case EVCON_READING_HEADERS:
	case EVCON_READING_TRAILER:
	case EVCON_WRITING:
	default:
		break;
	}

	


	if (evcon->flags & EVHTTP_CON_CLOSEDETECT) {
		evcon->flags &= ~EVHTTP_CON_CLOSEDETECT;
		EVUTIL_ASSERT(evcon->http_server == NULL);
		



		EVUTIL_ASSERT(evcon->state == EVCON_IDLE);
		evhttp_connection_reset(evcon);
		return;
	}

	if (what & BEV_EVENT_TIMEOUT) {
		evhttp_connection_fail(evcon, EVCON_HTTP_TIMEOUT);
	} else if (what & (BEV_EVENT_EOF|BEV_EVENT_ERROR)) {
		evhttp_connection_fail(evcon, EVCON_HTTP_EOF);
	} else {
		evhttp_connection_fail(evcon, EVCON_HTTP_BUFFER_ERROR);
	}
}




static void
evhttp_connection_cb(struct bufferevent *bufev, short what, void *arg)
{
	struct evhttp_connection *evcon = arg;
	int error;
	ev_socklen_t errsz = sizeof(error);

	if (!(what & BEV_EVENT_CONNECTED)) {
		



#ifndef WIN32
		if (errno == ECONNREFUSED)
			goto cleanup;
#endif
		evhttp_error_cb(bufev, what, arg);
		return;
	}

	
	if (getsockopt(evcon->fd, SOL_SOCKET, SO_ERROR, (void*)&error,
		       &errsz) == -1) {
		event_debug(("%s: getsockopt for \"%s:%d\" on "EV_SOCK_FMT,
			__func__, evcon->address, evcon->port,
			EV_SOCK_ARG(evcon->fd)));
		goto cleanup;
	}

	if (error) {
		event_debug(("%s: connect failed for \"%s:%d\" on "
			EV_SOCK_FMT": %s",
			__func__, evcon->address, evcon->port,
			EV_SOCK_ARG(evcon->fd),
			evutil_socket_error_to_string(error)));
		goto cleanup;
	}

	
	event_debug(("%s: connected to \"%s:%d\" on "EV_SOCK_FMT"\n",
			__func__, evcon->address, evcon->port,
			EV_SOCK_ARG(evcon->fd)));

	
	evcon->retry_cnt = 0;
	evcon->state = EVCON_IDLE;

	
	bufferevent_setcb(evcon->bufev,
	    evhttp_read_cb,
	    evhttp_write_cb,
	    evhttp_error_cb,
	    evcon);

	if (evcon->timeout == -1)
		bufferevent_settimeout(evcon->bufev,
		    HTTP_READ_TIMEOUT, HTTP_WRITE_TIMEOUT);
	else {
		struct timeval tv;
		tv.tv_sec = evcon->timeout;
		tv.tv_usec = 0;
		bufferevent_set_timeouts(evcon->bufev, &tv, &tv);
	}

	
	evhttp_request_dispatch(evcon);
	return;

 cleanup:
	evhttp_connection_cb_cleanup(evcon);
}





static int
evhttp_valid_response_code(int code)
{
	if (code == 0)
		return (0);

	return (1);
}

static int
evhttp_parse_http_version(const char *version, struct evhttp_request *req)
{
	int major, minor;
	char ch;
	int n = sscanf(version, "HTTP/%d.%d%c", &major, &minor, &ch);
	if (n != 2 || major > 1) {
		event_debug(("%s: bad version %s on message %p from %s",
			__func__, version, req, req->remote_host));
		return (-1);
	}
	req->major = major;
	req->minor = minor;
	return (0);
}



static int
evhttp_parse_response_line(struct evhttp_request *req, char *line)
{
	char *protocol;
	char *number;
	const char *readable = "";

	protocol = strsep(&line, " ");
	if (line == NULL)
		return (-1);
	number = strsep(&line, " ");
	if (line != NULL)
		readable = line;

	if (evhttp_parse_http_version(protocol, req) < 0)
		return (-1);

	req->response_code = atoi(number);
	if (!evhttp_valid_response_code(req->response_code)) {
		event_debug(("%s: bad response code \"%s\"",
			__func__, number));
		return (-1);
	}

	if ((req->response_code_line = mm_strdup(readable)) == NULL) {
		event_warn("%s: strdup", __func__);
		return (-1);
	}

	return (0);
}



static int
evhttp_parse_request_line(struct evhttp_request *req, char *line)
{
	char *method;
	char *uri;
	char *version;
	const char *hostname;
	const char *scheme;

	
	method = strsep(&line, " ");
	if (line == NULL)
		return (-1);
	uri = strsep(&line, " ");
	if (line == NULL)
		return (-1);
	version = strsep(&line, " ");
	if (line != NULL)
		return (-1);

	
	if (strcmp(method, "GET") == 0) {
		req->type = EVHTTP_REQ_GET;
	} else if (strcmp(method, "POST") == 0) {
		req->type = EVHTTP_REQ_POST;
	} else if (strcmp(method, "HEAD") == 0) {
		req->type = EVHTTP_REQ_HEAD;
	} else if (strcmp(method, "PUT") == 0) {
		req->type = EVHTTP_REQ_PUT;
	} else if (strcmp(method, "DELETE") == 0) {
		req->type = EVHTTP_REQ_DELETE;
	} else if (strcmp(method, "OPTIONS") == 0) {
		req->type = EVHTTP_REQ_OPTIONS;
	} else if (strcmp(method, "TRACE") == 0) {
		req->type = EVHTTP_REQ_TRACE;
	} else if (strcmp(method, "PATCH") == 0) {
		req->type = EVHTTP_REQ_PATCH;
	} else {
		req->type = _EVHTTP_REQ_UNKNOWN;
		event_debug(("%s: bad method %s on request %p from %s",
			__func__, method, req, req->remote_host));
		

	}

	if (evhttp_parse_http_version(version, req) < 0)
		return (-1);

	if ((req->uri = mm_strdup(uri)) == NULL) {
		event_debug(("%s: mm_strdup", __func__));
		return (-1);
	}

	if ((req->uri_elems = evhttp_uri_parse_with_flags(req->uri,
		    EVHTTP_URI_NONCONFORMANT)) == NULL) {
		return -1;
	}

	


	scheme = evhttp_uri_get_scheme(req->uri_elems);
	hostname = evhttp_uri_get_host(req->uri_elems);
	if (scheme && (!evutil_ascii_strcasecmp(scheme, "http") ||
		       !evutil_ascii_strcasecmp(scheme, "https")) &&
	    hostname &&
	    !evhttp_find_vhost(req->evcon->http_server, NULL, hostname))
		req->flags |= EVHTTP_PROXY_REQUEST;

	return (0);
}

const char *
evhttp_find_header(const struct evkeyvalq *headers, const char *key)
{
	struct evkeyval *header;

	TAILQ_FOREACH(header, headers, next) {
		if (evutil_ascii_strcasecmp(header->key, key) == 0)
			return (header->value);
	}

	return (NULL);
}

void
evhttp_clear_headers(struct evkeyvalq *headers)
{
	struct evkeyval *header;

	for (header = TAILQ_FIRST(headers);
	    header != NULL;
	    header = TAILQ_FIRST(headers)) {
		TAILQ_REMOVE(headers, header, next);
		mm_free(header->key);
		mm_free(header->value);
		mm_free(header);
	}
}






int
evhttp_remove_header(struct evkeyvalq *headers, const char *key)
{
	struct evkeyval *header;

	TAILQ_FOREACH(header, headers, next) {
		if (evutil_ascii_strcasecmp(header->key, key) == 0)
			break;
	}

	if (header == NULL)
		return (-1);

	
	TAILQ_REMOVE(headers, header, next);
	mm_free(header->key);
	mm_free(header->value);
	mm_free(header);

	return (0);
}

static int
evhttp_header_is_valid_value(const char *value)
{
	const char *p = value;

	while ((p = strpbrk(p, "\r\n")) != NULL) {
		
		p += strspn(p, "\r\n");
		
		if (*p != ' ' && *p != '\t')
			return (0);
	}
	return (1);
}

int
evhttp_add_header(struct evkeyvalq *headers,
    const char *key, const char *value)
{
	event_debug(("%s: key: %s val: %s\n", __func__, key, value));

	if (strchr(key, '\r') != NULL || strchr(key, '\n') != NULL) {
		
		event_debug(("%s: dropping illegal header key\n", __func__));
		return (-1);
	}

	if (!evhttp_header_is_valid_value(value)) {
		event_debug(("%s: dropping illegal header value\n", __func__));
		return (-1);
	}

	return (evhttp_add_header_internal(headers, key, value));
}

static int
evhttp_add_header_internal(struct evkeyvalq *headers,
    const char *key, const char *value)
{
	struct evkeyval *header = mm_calloc(1, sizeof(struct evkeyval));
	if (header == NULL) {
		event_warn("%s: calloc", __func__);
		return (-1);
	}
	if ((header->key = mm_strdup(key)) == NULL) {
		mm_free(header);
		event_warn("%s: strdup", __func__);
		return (-1);
	}
	if ((header->value = mm_strdup(value)) == NULL) {
		mm_free(header->key);
		mm_free(header);
		event_warn("%s: strdup", __func__);
		return (-1);
	}

	TAILQ_INSERT_TAIL(headers, header, next);

	return (0);
}











enum message_read_status
evhttp_parse_firstline(struct evhttp_request *req, struct evbuffer *buffer)
{
	char *line;
	enum message_read_status status = ALL_DATA_READ;

	size_t line_length;
	
	line = evbuffer_readln(buffer, &line_length, EVBUFFER_EOL_CRLF);
	if (line == NULL) {
		if (req->evcon != NULL &&
		    evbuffer_get_length(buffer) > req->evcon->max_headers_size)
			return (DATA_TOO_LONG);
		else
			return (MORE_DATA_EXPECTED);
	}

	if (req->evcon != NULL &&
	    line_length > req->evcon->max_headers_size) {
		mm_free(line);
		return (DATA_TOO_LONG);
	}

	req->headers_size = line_length;

	switch (req->kind) {
	case EVHTTP_REQUEST:
		if (evhttp_parse_request_line(req, line) == -1)
			status = DATA_CORRUPTED;
		break;
	case EVHTTP_RESPONSE:
		if (evhttp_parse_response_line(req, line) == -1)
			status = DATA_CORRUPTED;
		break;
	default:
		status = DATA_CORRUPTED;
	}

	mm_free(line);
	return (status);
}

static int
evhttp_append_to_last_header(struct evkeyvalq *headers, const char *line)
{
	struct evkeyval *header = TAILQ_LAST(headers, evkeyvalq);
	char *newval;
	size_t old_len, line_len;

	if (header == NULL)
		return (-1);

	old_len = strlen(header->value);
	line_len = strlen(line);

	newval = mm_realloc(header->value, old_len + line_len + 1);
	if (newval == NULL)
		return (-1);

	memcpy(newval + old_len, line, line_len + 1);
	header->value = newval;

	return (0);
}

enum message_read_status
evhttp_parse_headers(struct evhttp_request *req, struct evbuffer* buffer)
{
	enum message_read_status errcode = DATA_CORRUPTED;
	char *line;
	enum message_read_status status = MORE_DATA_EXPECTED;

	struct evkeyvalq* headers = req->input_headers;
	size_t line_length;
	while ((line = evbuffer_readln(buffer, &line_length, EVBUFFER_EOL_CRLF))
	       != NULL) {
		char *skey, *svalue;

		req->headers_size += line_length;

		if (req->evcon != NULL &&
		    req->headers_size > req->evcon->max_headers_size) {
			errcode = DATA_TOO_LONG;
			goto error;
		}

		if (*line == '\0') { 
			status = ALL_DATA_READ;
			mm_free(line);
			break;
		}

		
		if (*line == ' ' || *line == '\t') {
			if (evhttp_append_to_last_header(headers, line) == -1)
				goto error;
			mm_free(line);
			continue;
		}

		
		svalue = line;
		skey = strsep(&svalue, ":");
		if (svalue == NULL)
			goto error;

		svalue += strspn(svalue, " ");

		if (evhttp_add_header(headers, skey, svalue) == -1)
			goto error;

		mm_free(line);
	}

	if (status == MORE_DATA_EXPECTED) {
		if (req->evcon != NULL &&
		req->headers_size + evbuffer_get_length(buffer) > req->evcon->max_headers_size)
			return (DATA_TOO_LONG);
	}

	return (status);

 error:
	mm_free(line);
	return (errcode);
}

static int
evhttp_get_body_length(struct evhttp_request *req)
{
	struct evkeyvalq *headers = req->input_headers;
	const char *content_length;
	const char *connection;

	content_length = evhttp_find_header(headers, "Content-Length");
	connection = evhttp_find_header(headers, "Connection");

	if (content_length == NULL && connection == NULL)
		req->ntoread = -1;
	else if (content_length == NULL &&
	    evutil_ascii_strcasecmp(connection, "Close") != 0) {
		
		event_warnx("%s: we got no content length, but the "
		    "server wants to keep the connection open: %s.",
		    __func__, connection);
		return (-1);
	} else if (content_length == NULL) {
		req->ntoread = -1;
	} else {
		char *endp;
		ev_int64_t ntoread = evutil_strtoll(content_length, &endp, 10);
		if (*content_length == '\0' || *endp != '\0' || ntoread < 0) {
			event_debug(("%s: illegal content length: %s",
				__func__, content_length));
			return (-1);
		}
		req->ntoread = ntoread;
	}

	event_debug(("%s: bytes to read: "EV_I64_FMT" (in buffer "EV_SIZE_FMT")\n",
		__func__, EV_I64_ARG(req->ntoread),
		EV_SIZE_ARG(evbuffer_get_length(bufferevent_get_input(req->evcon->bufev)))));

	return (0);
}

static int
evhttp_method_may_have_body(enum evhttp_cmd_type type)
{
	switch (type) {
	case EVHTTP_REQ_POST:
	case EVHTTP_REQ_PUT:
	case EVHTTP_REQ_PATCH:
		return 1;
	case EVHTTP_REQ_TRACE:
		return 0;
	
	case EVHTTP_REQ_GET:
	case EVHTTP_REQ_HEAD:
	case EVHTTP_REQ_DELETE:
	case EVHTTP_REQ_OPTIONS:
	case EVHTTP_REQ_CONNECT:
		return 0;
	default:
		return 0;
	}
}

static void
evhttp_get_body(struct evhttp_connection *evcon, struct evhttp_request *req)
{
	const char *xfer_enc;

	
	if (req->kind == EVHTTP_REQUEST &&
	    !evhttp_method_may_have_body(req->type)) {
		evhttp_connection_done(evcon);
		return;
	}
	evcon->state = EVCON_READING_BODY;
	xfer_enc = evhttp_find_header(req->input_headers, "Transfer-Encoding");
	if (xfer_enc != NULL && evutil_ascii_strcasecmp(xfer_enc, "chunked") == 0) {
		req->chunked = 1;
		req->ntoread = -1;
	} else {
		if (evhttp_get_body_length(req) == -1) {
			evhttp_connection_fail(evcon,
			    EVCON_HTTP_INVALID_HEADER);
			return;
		}
		if (req->kind == EVHTTP_REQUEST && req->ntoread < 1) {
			

			evhttp_connection_done(evcon);
			return;
		}
	}

	
	if (req->kind == EVHTTP_REQUEST && REQ_VERSION_ATLEAST(req, 1, 1)) {
		const char *expect;

		expect = evhttp_find_header(req->input_headers, "Expect");
		if (expect) {
			if (!evutil_ascii_strcasecmp(expect, "100-continue")) {
				





				if (req->ntoread > 0) {
					 
					if ((req->evcon->max_body_size <= EV_INT64_MAX) && (ev_uint64_t)req->ntoread > req->evcon->max_body_size) {
						evhttp_send_error(req, HTTP_ENTITYTOOLARGE, NULL);
						return;
					}
				}
				if (!evbuffer_get_length(bufferevent_get_input(evcon->bufev)))
					evhttp_send_continue(evcon, req);
			} else {
				evhttp_send_error(req, HTTP_EXPECTATIONFAILED,
					NULL);
				return;
			}
		}
	}

	evhttp_read_body(evcon, req);
	
}

static void
evhttp_read_firstline(struct evhttp_connection *evcon,
		      struct evhttp_request *req)
{
	enum message_read_status res;

	res = evhttp_parse_firstline(req, bufferevent_get_input(evcon->bufev));
	if (res == DATA_CORRUPTED || res == DATA_TOO_LONG) {
		
		event_debug(("%s: bad header lines on "EV_SOCK_FMT"\n",
			__func__, EV_SOCK_ARG(evcon->fd)));
		evhttp_connection_fail(evcon, EVCON_HTTP_INVALID_HEADER);
		return;
	} else if (res == MORE_DATA_EXPECTED) {
		
		return;
	}

	evcon->state = EVCON_READING_HEADERS;
	evhttp_read_header(evcon, req);
}

static void
evhttp_read_header(struct evhttp_connection *evcon,
		   struct evhttp_request *req)
{
	enum message_read_status res;
	evutil_socket_t fd = evcon->fd;

	res = evhttp_parse_headers(req, bufferevent_get_input(evcon->bufev));
	if (res == DATA_CORRUPTED || res == DATA_TOO_LONG) {
		
		event_debug(("%s: bad header lines on "EV_SOCK_FMT"\n",
			__func__, EV_SOCK_ARG(fd)));
		evhttp_connection_fail(evcon, EVCON_HTTP_INVALID_HEADER);
		return;
	} else if (res == MORE_DATA_EXPECTED) {
		
		return;
	}

	
	bufferevent_disable(evcon->bufev, EV_READ);

	
	switch (req->kind) {
	case EVHTTP_REQUEST:
		event_debug(("%s: checking for post data on "EV_SOCK_FMT"\n",
			__func__, EV_SOCK_ARG(fd)));
		evhttp_get_body(evcon, req);
		
		break;

	case EVHTTP_RESPONSE:
		
		if (req->response_code == 100) {
			evhttp_start_read(evcon);
			return;
		}
		if (!evhttp_response_needs_body(req)) {
			event_debug(("%s: skipping body for code %d\n",
					__func__, req->response_code));
			evhttp_connection_done(evcon);
		} else {
			event_debug(("%s: start of read body for %s on "
				EV_SOCK_FMT"\n",
				__func__, req->remote_host, EV_SOCK_ARG(fd)));
			evhttp_get_body(evcon, req);
			

		}
		break;

	default:
		event_warnx("%s: bad header on "EV_SOCK_FMT, __func__,
		    EV_SOCK_ARG(fd));
		evhttp_connection_fail(evcon, EVCON_HTTP_INVALID_HEADER);
		break;
	}
	
}











struct evhttp_connection *
evhttp_connection_new(const char *address, unsigned short port)
{
	return (evhttp_connection_base_new(NULL, NULL, address, port));
}

struct evhttp_connection *
evhttp_connection_base_new(struct event_base *base, struct evdns_base *dnsbase,
    const char *address, unsigned short port)
{
	struct evhttp_connection *evcon = NULL;

	event_debug(("Attempting connection to %s:%d\n", address, port));

	if ((evcon = mm_calloc(1, sizeof(struct evhttp_connection))) == NULL) {
		event_warn("%s: calloc failed", __func__);
		goto error;
	}

	evcon->fd = -1;
	evcon->port = port;

	evcon->max_headers_size = EV_SIZE_MAX;
	evcon->max_body_size = EV_SIZE_MAX;

	evcon->timeout = -1;
	evcon->retry_cnt = evcon->retry_max = 0;

	if ((evcon->address = mm_strdup(address)) == NULL) {
		event_warn("%s: strdup failed", __func__);
		goto error;
	}

	if ((evcon->bufev = bufferevent_new(-1,
		    evhttp_read_cb,
		    evhttp_write_cb,
		    evhttp_error_cb, evcon)) == NULL) {
		event_warn("%s: bufferevent_new failed", __func__);
		goto error;
	}

	evcon->state = EVCON_DISCONNECTED;
	TAILQ_INIT(&evcon->requests);

	if (base != NULL) {
		evcon->base = base;
		bufferevent_base_set(base, evcon->bufev);
	}


	event_deferred_cb_init(&evcon->read_more_deferred_cb,
	    evhttp_deferred_read_cb, evcon);

	evcon->dns_base = dnsbase;

	return (evcon);

 error:
	if (evcon != NULL)
		evhttp_connection_free(evcon);
	return (NULL);
}

struct bufferevent *
evhttp_connection_get_bufferevent(struct evhttp_connection *evcon)
{
	return evcon->bufev;
}

void
evhttp_connection_set_base(struct evhttp_connection *evcon,
    struct event_base *base)
{
	EVUTIL_ASSERT(evcon->base == NULL);
	EVUTIL_ASSERT(evcon->state == EVCON_DISCONNECTED);
	evcon->base = base;
	bufferevent_base_set(base, evcon->bufev);
}

void
evhttp_connection_set_timeout(struct evhttp_connection *evcon,
    int timeout_in_secs)
{
	evcon->timeout = timeout_in_secs;

	if (evcon->timeout == -1)
		bufferevent_settimeout(evcon->bufev,
		    HTTP_READ_TIMEOUT, HTTP_WRITE_TIMEOUT);
	else
		bufferevent_settimeout(evcon->bufev,
		    evcon->timeout, evcon->timeout);
}

void
evhttp_connection_set_retries(struct evhttp_connection *evcon,
    int retry_max)
{
	evcon->retry_max = retry_max;
}

void
evhttp_connection_set_closecb(struct evhttp_connection *evcon,
    void (*cb)(struct evhttp_connection *, void *), void *cbarg)
{
	evcon->closecb = cb;
	evcon->closecb_arg = cbarg;
}

void
evhttp_connection_get_peer(struct evhttp_connection *evcon,
    char **address, ev_uint16_t *port)
{
	*address = evcon->address;
	*port = evcon->port;
}

int
evhttp_connection_connect(struct evhttp_connection *evcon)
{
	if (evcon->state == EVCON_CONNECTING)
		return (0);

	evhttp_connection_reset(evcon);

	EVUTIL_ASSERT(!(evcon->flags & EVHTTP_CON_INCOMING));
	evcon->flags |= EVHTTP_CON_OUTGOING;

	evcon->fd = bind_socket(
		evcon->bind_address, evcon->bind_port, 0 );
	if (evcon->fd == -1) {
		event_debug(("%s: failed to bind to \"%s\"",
			__func__, evcon->bind_address));
		return (-1);
	}

	
	bufferevent_setfd(evcon->bufev, evcon->fd);
	bufferevent_setcb(evcon->bufev,
	    NULL ,
	    NULL ,
	    evhttp_connection_cb,
	    evcon);
	bufferevent_settimeout(evcon->bufev, 0,
	    evcon->timeout != -1 ? evcon->timeout : HTTP_CONNECT_TIMEOUT);
	
	bufferevent_enable(evcon->bufev, EV_WRITE);

	if (bufferevent_socket_connect_hostname(evcon->bufev, evcon->dns_base,
		AF_UNSPEC, evcon->address, evcon->port) < 0) {
		event_sock_warn(evcon->fd, "%s: connection to \"%s\" failed",
		    __func__, evcon->address);
		



		evhttp_connection_cb_cleanup(evcon);
		return (0);
	}

	evcon->state = EVCON_CONNECTING;

	return (0);
}







int
evhttp_make_request(struct evhttp_connection *evcon,
    struct evhttp_request *req,
    enum evhttp_cmd_type type, const char *uri)
{
	
	req->kind = EVHTTP_REQUEST;
	req->type = type;
	if (req->uri != NULL)
		mm_free(req->uri);
	if ((req->uri = mm_strdup(uri)) == NULL) {
		event_warn("%s: strdup", __func__);
		evhttp_request_free(req);
		return (-1);
	}

	
	if (!req->major && !req->minor) {
		req->major = 1;
		req->minor = 1;
	}

	EVUTIL_ASSERT(req->evcon == NULL);
	req->evcon = evcon;
	EVUTIL_ASSERT(!(req->flags & EVHTTP_REQ_OWN_CONNECTION));

       TAILQ_INSERT_TAIL(&evcon->requests, req, next);

	
	if (!evhttp_connected(evcon)) {
		int res = evhttp_connection_connect(evcon);
	       



	       if (res != 0)
		       TAILQ_REMOVE(&evcon->requests, req, next);

		return res;
	}

	




	if (TAILQ_FIRST(&evcon->requests) == req)
		evhttp_request_dispatch(evcon);

	return (0);
}

void
evhttp_cancel_request(struct evhttp_request *req)
{
	struct evhttp_connection *evcon = req->evcon;
	if (evcon != NULL) {
		
		if (TAILQ_FIRST(&evcon->requests) == req) {
			


			evhttp_connection_fail(evcon,
			    EVCON_HTTP_REQUEST_CANCEL);

			
			return;
		} else {
			


			TAILQ_REMOVE(&evcon->requests, req, next);
		}
	}

	evhttp_request_free(req);
}






void
evhttp_start_read(struct evhttp_connection *evcon)
{
	
	bufferevent_disable(evcon->bufev, EV_WRITE);
	bufferevent_enable(evcon->bufev, EV_READ);
	evcon->state = EVCON_READING_FIRSTLINE;
	
	bufferevent_setcb(evcon->bufev,
	    evhttp_read_cb,
	    evhttp_write_cb,
	    evhttp_error_cb,
	    evcon);

	

	if (evbuffer_get_length(bufferevent_get_input(evcon->bufev))) {
		event_deferred_cb_schedule(get_deferred_queue(evcon),
		    &evcon->read_more_deferred_cb);
	}
}

static void
evhttp_send_done(struct evhttp_connection *evcon, void *arg)
{
	int need_close;
	struct evhttp_request *req = TAILQ_FIRST(&evcon->requests);
	TAILQ_REMOVE(&evcon->requests, req, next);

	need_close =
	    (REQ_VERSION_BEFORE(req, 1, 1) &&
		!evhttp_is_connection_keepalive(req->input_headers))||
	    evhttp_is_connection_close(req->flags, req->input_headers) ||
	    evhttp_is_connection_close(req->flags, req->output_headers);

	EVUTIL_ASSERT(req->flags & EVHTTP_REQ_OWN_CONNECTION);
	evhttp_request_free(req);

	if (need_close) {
		evhttp_connection_free(evcon);
		return;
	}

	
	if (evhttp_associate_new_request_with_connection(evcon) == -1) {
		evhttp_connection_free(evcon);
	}
}





void
evhttp_send_error(struct evhttp_request *req, int error, const char *reason)
{

#define ERR_FORMAT "<HTML><HEAD>\n" \
	    "<TITLE>%d %s</TITLE>\n" \
	    "</HEAD><BODY>\n" \
	    "<H1>%s</H1>\n" \
	    "</BODY></HTML>\n"

	struct evbuffer *buf = evbuffer_new();
	if (buf == NULL) {
		
		evhttp_connection_free(req->evcon);
		return;
	}
	if (reason == NULL) {
		reason = evhttp_response_phrase_internal(error);
	}

	evhttp_response_code(req, error, reason);

	evbuffer_add_printf(buf, ERR_FORMAT, error, reason, reason);

	evhttp_send_page(req, buf);

	evbuffer_free(buf);
#undef ERR_FORMAT
}



static inline void
evhttp_send(struct evhttp_request *req, struct evbuffer *databuf)
{
	struct evhttp_connection *evcon = req->evcon;

	if (evcon == NULL) {
		evhttp_request_free(req);
		return;
	}

	EVUTIL_ASSERT(TAILQ_FIRST(&evcon->requests) == req);

	
	req->userdone = 1;

	
	if (databuf != NULL)
		evbuffer_add_buffer(req->output_buffer, databuf);

	
	evhttp_make_header(evcon, req);

	evhttp_write_buffer(evcon, evhttp_send_done, NULL);
}

void
evhttp_send_reply(struct evhttp_request *req, int code, const char *reason,
    struct evbuffer *databuf)
{
	evhttp_response_code(req, code, reason);

	evhttp_send(req, databuf);
}

void
evhttp_send_reply_start(struct evhttp_request *req, int code,
    const char *reason)
{
	evhttp_response_code(req, code, reason);
	if (evhttp_find_header(req->output_headers, "Content-Length") == NULL &&
	    REQ_VERSION_ATLEAST(req, 1, 1) &&
	    evhttp_response_needs_body(req)) {
		




		evhttp_add_header(req->output_headers, "Transfer-Encoding",
		    "chunked");
		req->chunked = 1;
	} else {
		req->chunked = 0;
	}
	evhttp_make_header(req->evcon, req);
	evhttp_write_buffer(req->evcon, NULL, NULL);
}

void
evhttp_send_reply_chunk(struct evhttp_request *req, struct evbuffer *databuf)
{
	struct evhttp_connection *evcon = req->evcon;
	struct evbuffer *output;

	if (evcon == NULL)
		return;

	output = bufferevent_get_output(evcon->bufev);

	if (evbuffer_get_length(databuf) == 0)
		return;
	if (!evhttp_response_needs_body(req))
		return;
	if (req->chunked) {
		evbuffer_add_printf(output, "%x\r\n",
				    (unsigned)evbuffer_get_length(databuf));
	}
	evbuffer_add_buffer(output, databuf);
	if (req->chunked) {
		evbuffer_add(output, "\r\n", 2);
	}
	evhttp_write_buffer(evcon, NULL, NULL);
}

void
evhttp_send_reply_end(struct evhttp_request *req)
{
	struct evhttp_connection *evcon = req->evcon;
	struct evbuffer *output;

	if (evcon == NULL) {
		evhttp_request_free(req);
		return;
	}

	output = bufferevent_get_output(evcon->bufev);

	
	req->userdone = 1;

	if (req->chunked) {
		evbuffer_add(output, "0\r\n\r\n", 5);
		evhttp_write_buffer(req->evcon, evhttp_send_done, NULL);
		req->chunked = 0;
	} else if (evbuffer_get_length(output) == 0) {
		
		evhttp_send_done(evcon, NULL);
	} else {
		
		evcon->cb = evhttp_send_done;
		evcon->cb_arg = NULL;
	}
}

static const char *informational_phrases[] = {
	 "Continue",
	 "Switching Protocols"
};

static const char *success_phrases[] = {
	 "OK",
	 "Created",
	 "Accepted",
	 "Non-Authoritative Information",
	 "No Content",
	 "Reset Content",
	 "Partial Content"
};

static const char *redirection_phrases[] = {
	 "Multiple Choices",
	 "Moved Permanently",
	 "Found",
	 "See Other",
	 "Not Modified",
	 "Use Proxy",
	 "Temporary Redirect"
};

static const char *client_error_phrases[] = {
	 "Bad Request",
	 "Unauthorized",
	 "Payment Required",
	 "Forbidden",
	 "Not Found",
	 "Method Not Allowed",
	 "Not Acceptable",
	 "Proxy Authentication Required",
	 "Request Time-out",
	 "Conflict",
	 "Gone",
	 "Length Required",
	 "Precondition Failed",
	 "Request Entity Too Large",
	 "Request-URI Too Large",
	 "Unsupported Media Type",
	 "Requested range not satisfiable",
	 "Expectation Failed"
};

static const char *server_error_phrases[] = {
	 "Internal Server Error",
	 "Not Implemented",
	 "Bad Gateway",
	 "Service Unavailable",
	 "Gateway Time-out",
	 "HTTP Version not supported"
};

struct response_class {
	const char *name;
	size_t num_responses;
	const char **responses;
};

#ifndef MEMBERSOF
#define MEMBERSOF(x) (sizeof(x)/sizeof(x[0]))
#endif

static const struct response_class response_classes[] = {
	 { "Informational", MEMBERSOF(informational_phrases), informational_phrases },
	 { "Success", MEMBERSOF(success_phrases), success_phrases },
	 { "Redirection", MEMBERSOF(redirection_phrases), redirection_phrases },
	 { "Client Error", MEMBERSOF(client_error_phrases), client_error_phrases },
	 { "Server Error", MEMBERSOF(server_error_phrases), server_error_phrases }
};

static const char *
evhttp_response_phrase_internal(int code)
{
	int klass = code / 100 - 1;
	int subcode = code % 100;

	
	if (klass < 0 || klass >= (int) MEMBERSOF(response_classes))
		return "Unknown Status Class";

	
	if (subcode >= (int) response_classes[klass].num_responses)
		return response_classes[klass].name;

	return response_classes[klass].responses[subcode];
}

void
evhttp_response_code(struct evhttp_request *req, int code, const char *reason)
{
	req->kind = EVHTTP_RESPONSE;
	req->response_code = code;
	if (req->response_code_line != NULL)
		mm_free(req->response_code_line);
	if (reason == NULL)
		reason = evhttp_response_phrase_internal(code);
	req->response_code_line = mm_strdup(reason);
	if (req->response_code_line == NULL) {
		event_warn("%s: strdup", __func__);
		
	}
}

void
evhttp_send_page(struct evhttp_request *req, struct evbuffer *databuf)
{
	if (!req->major || !req->minor) {
		req->major = 1;
		req->minor = 1;
	}

	if (req->kind != EVHTTP_RESPONSE)
		evhttp_response_code(req, 200, "OK");

	evhttp_clear_headers(req->output_headers);
	evhttp_add_header(req->output_headers, "Content-Type", "text/html");
	evhttp_add_header(req->output_headers, "Connection", "close");

	evhttp_send(req, databuf);
}

static const char uri_chars[256] = {
	
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 1, 1, 0,
	1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 0, 0, 0, 0, 0, 0,
	
	0, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 0, 0, 0, 0, 1,
	0, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 0, 0, 0, 1, 0,
	
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
};

#define CHAR_IS_UNRESERVED(c)			\
	(uri_chars[(unsigned char)(c)])





char *
evhttp_uriencode(const char *uri, ev_ssize_t len, int space_as_plus)
{
	struct evbuffer *buf = evbuffer_new();
	const char *p, *end;
	char *result;

	if (buf == NULL)
		return (NULL);

	if (len >= 0)
		end = uri+len;
	else
		end = uri+strlen(uri);

	for (p = uri; p < end; p++) {
		if (CHAR_IS_UNRESERVED(*p)) {
			evbuffer_add(buf, p, 1);
		} else if (*p == ' ' && space_as_plus) {
			evbuffer_add(buf, "+", 1);
		} else {
			evbuffer_add_printf(buf, "%%%02X", (unsigned char)(*p));
		}
	}
	evbuffer_add(buf, "", 1); 
	result = mm_malloc(evbuffer_get_length(buf));
	if (result)
		evbuffer_remove(buf, result, evbuffer_get_length(buf));
	evbuffer_free(buf);

	return (result);
}

char *
evhttp_encode_uri(const char *str)
{
	return evhttp_uriencode(str, -1, 0);
}







static int
evhttp_decode_uri_internal(
	const char *uri, size_t length, char *ret, int decode_plus_ctl)
{
	char c;
	int j;
	int decode_plus = (decode_plus_ctl == 1) ? 1: 0;
	unsigned i;

	for (i = j = 0; i < length; i++) {
		c = uri[i];
		if (c == '?') {
			if (decode_plus_ctl < 0)
				decode_plus = 1;
		} else if (c == '+' && decode_plus) {
			c = ' ';
		} else if (c == '%' && EVUTIL_ISXDIGIT(uri[i+1]) &&
		    EVUTIL_ISXDIGIT(uri[i+2])) {
			char tmp[3];
			tmp[0] = uri[i+1];
			tmp[1] = uri[i+2];
			tmp[2] = '\0';
			c = (char)strtol(tmp, NULL, 16);
			i += 2;
		}
		ret[j++] = c;
	}
	ret[j] = '\0';

	return (j);
}


char *
evhttp_decode_uri(const char *uri)
{
	char *ret;

	if ((ret = mm_malloc(strlen(uri) + 1)) == NULL) {
		event_warn("%s: malloc(%lu)", __func__,
			  (unsigned long)(strlen(uri) + 1));
		return (NULL);
	}

	evhttp_decode_uri_internal(uri, strlen(uri),
	    ret, -1 );

	return (ret);
}

char *
evhttp_uridecode(const char *uri, int decode_plus, size_t *size_out)
{
	char *ret;
	int n;

	if ((ret = mm_malloc(strlen(uri) + 1)) == NULL) {
		event_warn("%s: malloc(%lu)", __func__,
			  (unsigned long)(strlen(uri) + 1));
		return (NULL);
	}

	n = evhttp_decode_uri_internal(uri, strlen(uri),
	    ret, !!decode_plus);

	if (size_out) {
		EVUTIL_ASSERT(n >= 0);
		*size_out = (size_t)n;
	}

	return (ret);
}






static int
evhttp_parse_query_impl(const char *str, struct evkeyvalq *headers,
    int is_whole_uri)
{
	char *line=NULL;
	char *argument;
	char *p;
	const char *query_part;
	int result = -1;
	struct evhttp_uri *uri=NULL;

	TAILQ_INIT(headers);

	if (is_whole_uri) {
		uri = evhttp_uri_parse(str);
		if (!uri)
			goto error;
		query_part = evhttp_uri_get_query(uri);
	} else {
		query_part = str;
	}

	
	if (!query_part || !strlen(query_part)) {
		result = 0;
		goto done;
	}

	if ((line = mm_strdup(query_part)) == NULL) {
		event_warn("%s: strdup", __func__);
		goto error;
	}

	p = argument = line;
	while (p != NULL && *p != '\0') {
		char *key, *value, *decoded_value;
		argument = strsep(&p, "&");

		value = argument;
		key = strsep(&value, "=");
		if (value == NULL || *key == '\0') {
			goto error;
		}

		if ((decoded_value = mm_malloc(strlen(value) + 1)) == NULL) {
			event_warn("%s: mm_malloc", __func__);
			goto error;
		}
		evhttp_decode_uri_internal(value, strlen(value),
		    decoded_value, 1 );
		event_debug(("Query Param: %s -> %s\n", key, decoded_value));
		evhttp_add_header_internal(headers, key, decoded_value);
		mm_free(decoded_value);
	}

	result = 0;
	goto done;
error:
	evhttp_clear_headers(headers);
done:
	if (line)
		mm_free(line);
	if (uri)
		evhttp_uri_free(uri);
	return result;
}

int
evhttp_parse_query(const char *uri, struct evkeyvalq *headers)
{
	return evhttp_parse_query_impl(uri, headers, 1);
}
int
evhttp_parse_query_str(const char *uri, struct evkeyvalq *headers)
{
	return evhttp_parse_query_impl(uri, headers, 0);
}

static struct evhttp_cb *
evhttp_dispatch_callback(struct httpcbq *callbacks, struct evhttp_request *req)
{
	struct evhttp_cb *cb;
	size_t offset = 0;
	char *translated;
	const char *path;

	
	path = evhttp_uri_get_path(req->uri_elems);
	offset = strlen(path);
	if ((translated = mm_malloc(offset + 1)) == NULL)
		return (NULL);
	evhttp_decode_uri_internal(path, offset, translated,
	    0 );

	TAILQ_FOREACH(cb, callbacks, next) {
		if (!strcmp(cb->what, translated)) {
			mm_free(translated);
			return (cb);
		}
	}

	mm_free(translated);
	return (NULL);
}


static int
prefix_suffix_match(const char *pattern, const char *name, int ignorecase)
{
	char c;

	while (1) {
		switch (c = *pattern++) {
		case '\0':
			return *name == '\0';

		case '*':
			while (*name != '\0') {
				if (prefix_suffix_match(pattern, name,
					ignorecase))
					return (1);
				++name;
			}
			return (0);
		default:
			if (c != *name) {
				if (!ignorecase ||
				    EVUTIL_TOLOWER(c) != EVUTIL_TOLOWER(*name))
					return (0);
			}
			++name;
		}
	}
	
}







static int
evhttp_find_alias(struct evhttp *http, struct evhttp **outhttp,
		  const char *hostname)
{
	struct evhttp_server_alias *alias;
	struct evhttp *vhost;

	TAILQ_FOREACH(alias, &http->aliases, next) {
		
		if (!evutil_ascii_strcasecmp(alias->alias, hostname)) {
			if (outhttp)
				*outhttp = http;
			return 1;
		}
	}

	

	TAILQ_FOREACH(vhost, &http->virtualhosts, next_vhost) {
		if (evhttp_find_alias(vhost, outhttp, hostname))
			return 1;
	}

	return 0;
}












static int
evhttp_find_vhost(struct evhttp *http, struct evhttp **outhttp,
		  const char *hostname)
{
	struct evhttp *vhost;
	struct evhttp *oldhttp;
	int match_found = 0;

	if (evhttp_find_alias(http, outhttp, hostname))
		return 1;

	do {
		oldhttp = http;
		TAILQ_FOREACH(vhost, &http->virtualhosts, next_vhost) {
			if (prefix_suffix_match(vhost->vhost_pattern,
				hostname, 1 )) {
				http = vhost;
				match_found = 1;
				break;
			}
		}
	} while (oldhttp != http);

	if (outhttp)
		*outhttp = http;

	return match_found;
}

static void
evhttp_handle_request(struct evhttp_request *req, void *arg)
{
	struct evhttp *http = arg;
	struct evhttp_cb *cb = NULL;
	const char *hostname;

	
	req->userdone = 0;

	if (req->type == 0 || req->uri == NULL) {
		evhttp_send_error(req, HTTP_BADREQUEST, NULL);
		return;
	}

	if ((http->allowed_methods & req->type) == 0) {
		event_debug(("Rejecting disallowed method %x (allowed: %x)\n",
			(unsigned)req->type, (unsigned)http->allowed_methods));
		evhttp_send_error(req, HTTP_NOTIMPLEMENTED, NULL);
		return;
	}

	
	hostname = evhttp_request_get_host(req);
	if (hostname != NULL) {
		evhttp_find_vhost(http, &http, hostname);
	}

	if ((cb = evhttp_dispatch_callback(&http->callbacks, req)) != NULL) {
		(*cb->cb)(req, cb->cbarg);
		return;
	}

	
	if (http->gencb) {
		(*http->gencb)(req, http->gencbarg);
		return;
	} else {
		
#define ERR_FORMAT "<html><head>" \
		    "<title>404 Not Found</title>" \
		    "</head><body>" \
		    "<h1>Not Found</h1>" \
		    "<p>The requested URL %s was not found on this server.</p>"\
		    "</body></html>\n"

		char *escaped_html;
		struct evbuffer *buf;

		if ((escaped_html = evhttp_htmlescape(req->uri)) == NULL) {
			evhttp_connection_free(req->evcon);
			return;
		}

		if ((buf = evbuffer_new()) == NULL) {
			mm_free(escaped_html);
			evhttp_connection_free(req->evcon);
			return;
		}

		evhttp_response_code(req, HTTP_NOTFOUND, "Not Found");

		evbuffer_add_printf(buf, ERR_FORMAT, escaped_html);

		mm_free(escaped_html);

		evhttp_send_page(req, buf);

		evbuffer_free(buf);
#undef ERR_FORMAT
	}
}


static void
accept_socket_cb(struct evconnlistener *listener, evutil_socket_t nfd, struct sockaddr *peer_sa, int peer_socklen, void *arg)
{
	struct evhttp *http = arg;

	evhttp_get_request(http, nfd, peer_sa, peer_socklen);
}

int
evhttp_bind_socket(struct evhttp *http, const char *address, ev_uint16_t port)
{
	struct evhttp_bound_socket *bound =
		evhttp_bind_socket_with_handle(http, address, port);
	if (bound == NULL)
		return (-1);
	return (0);
}

struct evhttp_bound_socket *
evhttp_bind_socket_with_handle(struct evhttp *http, const char *address, ev_uint16_t port)
{
	evutil_socket_t fd;
	struct evhttp_bound_socket *bound;

	if ((fd = bind_socket(address, port, 1 )) == -1)
		return (NULL);

	if (listen(fd, 128) == -1) {
		event_sock_warn(fd, "%s: listen", __func__);
		evutil_closesocket(fd);
		return (NULL);
	}

	bound = evhttp_accept_socket_with_handle(http, fd);

	if (bound != NULL) {
		event_debug(("Bound to port %d - Awaiting connections ... ",
			port));
		return (bound);
	}

	return (NULL);
}

int
evhttp_accept_socket(struct evhttp *http, evutil_socket_t fd)
{
	struct evhttp_bound_socket *bound =
		evhttp_accept_socket_with_handle(http, fd);
	if (bound == NULL)
		return (-1);
	return (0);
}


struct evhttp_bound_socket *
evhttp_accept_socket_with_handle(struct evhttp *http, evutil_socket_t fd)
{
	struct evhttp_bound_socket *bound;
	struct evconnlistener *listener;
	const int flags =
	    LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_EXEC|LEV_OPT_CLOSE_ON_FREE;

	listener = evconnlistener_new(http->base, NULL, NULL,
	    flags,
	    0, 
	    fd);
	if (!listener)
		return (NULL);

	bound = evhttp_bind_listener(http, listener);
	if (!bound) {
		evconnlistener_free(listener);
		return (NULL);
	}
	return (bound);
}

struct evhttp_bound_socket *
evhttp_bind_listener(struct evhttp *http, struct evconnlistener *listener)
{
	struct evhttp_bound_socket *bound;

	bound = mm_malloc(sizeof(struct evhttp_bound_socket));
	if (bound == NULL)
		return (NULL);

	bound->listener = listener;
	TAILQ_INSERT_TAIL(&http->sockets, bound, next);

	evconnlistener_set_cb(listener, accept_socket_cb, http);
	return bound;
}

evutil_socket_t
evhttp_bound_socket_get_fd(struct evhttp_bound_socket *bound)
{
	return evconnlistener_get_fd(bound->listener);
}

struct evconnlistener *
evhttp_bound_socket_get_listener(struct evhttp_bound_socket *bound)
{
	return bound->listener;
}

void
evhttp_del_accept_socket(struct evhttp *http, struct evhttp_bound_socket *bound)
{
	TAILQ_REMOVE(&http->sockets, bound, next);
	evconnlistener_free(bound->listener);
	mm_free(bound);
}

static struct evhttp*
evhttp_new_object(void)
{
	struct evhttp *http = NULL;

	if ((http = mm_calloc(1, sizeof(struct evhttp))) == NULL) {
		event_warn("%s: calloc", __func__);
		return (NULL);
	}

	http->timeout = -1;
	evhttp_set_max_headers_size(http, EV_SIZE_MAX);
	evhttp_set_max_body_size(http, EV_SIZE_MAX);
	evhttp_set_allowed_methods(http,
	    EVHTTP_REQ_GET |
	    EVHTTP_REQ_POST |
	    EVHTTP_REQ_HEAD |
	    EVHTTP_REQ_PUT |
	    EVHTTP_REQ_DELETE);

	TAILQ_INIT(&http->sockets);
	TAILQ_INIT(&http->callbacks);
	TAILQ_INIT(&http->connections);
	TAILQ_INIT(&http->virtualhosts);
	TAILQ_INIT(&http->aliases);

	return (http);
}

struct evhttp *
evhttp_new(struct event_base *base)
{
	struct evhttp *http = NULL;

	http = evhttp_new_object();
	if (http == NULL)
		return (NULL);
	http->base = base;

	return (http);
}





struct evhttp *
evhttp_start(const char *address, unsigned short port)
{
	struct evhttp *http = NULL;

	http = evhttp_new_object();
	if (http == NULL)
		return (NULL);
	if (evhttp_bind_socket(http, address, port) == -1) {
		mm_free(http);
		return (NULL);
	}

	return (http);
}

void
evhttp_free(struct evhttp* http)
{
	struct evhttp_cb *http_cb;
	struct evhttp_connection *evcon;
	struct evhttp_bound_socket *bound;
	struct evhttp* vhost;
	struct evhttp_server_alias *alias;

	
	while ((bound = TAILQ_FIRST(&http->sockets)) != NULL) {
		TAILQ_REMOVE(&http->sockets, bound, next);

		evconnlistener_free(bound->listener);

		mm_free(bound);
	}

	while ((evcon = TAILQ_FIRST(&http->connections)) != NULL) {
		
		evhttp_connection_free(evcon);
	}

	while ((http_cb = TAILQ_FIRST(&http->callbacks)) != NULL) {
		TAILQ_REMOVE(&http->callbacks, http_cb, next);
		mm_free(http_cb->what);
		mm_free(http_cb);
	}

	while ((vhost = TAILQ_FIRST(&http->virtualhosts)) != NULL) {
		TAILQ_REMOVE(&http->virtualhosts, vhost, next_vhost);

		evhttp_free(vhost);
	}

	if (http->vhost_pattern != NULL)
		mm_free(http->vhost_pattern);

	while ((alias = TAILQ_FIRST(&http->aliases)) != NULL) {
		TAILQ_REMOVE(&http->aliases, alias, next);
		mm_free(alias->alias);
		mm_free(alias);
	}

	mm_free(http);
}

int
evhttp_add_virtual_host(struct evhttp* http, const char *pattern,
    struct evhttp* vhost)
{
	
	if (vhost->vhost_pattern != NULL ||
	    TAILQ_FIRST(&vhost->sockets) != NULL)
		return (-1);

	vhost->vhost_pattern = mm_strdup(pattern);
	if (vhost->vhost_pattern == NULL)
		return (-1);

	TAILQ_INSERT_TAIL(&http->virtualhosts, vhost, next_vhost);

	return (0);
}

int
evhttp_remove_virtual_host(struct evhttp* http, struct evhttp* vhost)
{
	if (vhost->vhost_pattern == NULL)
		return (-1);

	TAILQ_REMOVE(&http->virtualhosts, vhost, next_vhost);

	mm_free(vhost->vhost_pattern);
	vhost->vhost_pattern = NULL;

	return (0);
}

int
evhttp_add_server_alias(struct evhttp *http, const char *alias)
{
	struct evhttp_server_alias *evalias;

	evalias = mm_calloc(1, sizeof(*evalias));
	if (!evalias)
		return -1;

	evalias->alias = mm_strdup(alias);
	if (!evalias->alias) {
		mm_free(evalias);
		return -1;
	}

	TAILQ_INSERT_TAIL(&http->aliases, evalias, next);

	return 0;
}

int
evhttp_remove_server_alias(struct evhttp *http, const char *alias)
{
	struct evhttp_server_alias *evalias;

	TAILQ_FOREACH(evalias, &http->aliases, next) {
		if (evutil_ascii_strcasecmp(evalias->alias, alias) == 0) {
			TAILQ_REMOVE(&http->aliases, evalias, next);
			mm_free(evalias->alias);
			mm_free(evalias);
			return 0;
		}
	}

	return -1;
}

void
evhttp_set_timeout(struct evhttp* http, int timeout_in_secs)
{
	http->timeout = timeout_in_secs;
}

void
evhttp_set_max_headers_size(struct evhttp* http, ev_ssize_t max_headers_size)
{
	if (max_headers_size < 0)
		http->default_max_headers_size = EV_SIZE_MAX;
	else
		http->default_max_headers_size = max_headers_size;
}

void
evhttp_set_max_body_size(struct evhttp* http, ev_ssize_t max_body_size)
{
	if (max_body_size < 0)
		http->default_max_body_size = EV_UINT64_MAX;
	else
		http->default_max_body_size = max_body_size;
}

void
evhttp_set_allowed_methods(struct evhttp* http, ev_uint16_t methods)
{
	http->allowed_methods = methods;
}

int
evhttp_set_cb(struct evhttp *http, const char *uri,
    void (*cb)(struct evhttp_request *, void *), void *cbarg)
{
	struct evhttp_cb *http_cb;

	TAILQ_FOREACH(http_cb, &http->callbacks, next) {
		if (strcmp(http_cb->what, uri) == 0)
			return (-1);
	}

	if ((http_cb = mm_calloc(1, sizeof(struct evhttp_cb))) == NULL) {
		event_warn("%s: calloc", __func__);
		return (-2);
	}

	http_cb->what = mm_strdup(uri);
	if (http_cb->what == NULL) {
		event_warn("%s: strdup", __func__);
		mm_free(http_cb);
		return (-3);
	}
	http_cb->cb = cb;
	http_cb->cbarg = cbarg;

	TAILQ_INSERT_TAIL(&http->callbacks, http_cb, next);

	return (0);
}

int
evhttp_del_cb(struct evhttp *http, const char *uri)
{
	struct evhttp_cb *http_cb;

	TAILQ_FOREACH(http_cb, &http->callbacks, next) {
		if (strcmp(http_cb->what, uri) == 0)
			break;
	}
	if (http_cb == NULL)
		return (-1);

	TAILQ_REMOVE(&http->callbacks, http_cb, next);
	mm_free(http_cb->what);
	mm_free(http_cb);

	return (0);
}

void
evhttp_set_gencb(struct evhttp *http,
    void (*cb)(struct evhttp_request *, void *), void *cbarg)
{
	http->gencb = cb;
	http->gencbarg = cbarg;
}





struct evhttp_request *
evhttp_request_new(void (*cb)(struct evhttp_request *, void *), void *arg)
{
	struct evhttp_request *req = NULL;

	
	if ((req = mm_calloc(1, sizeof(struct evhttp_request))) == NULL) {
		event_warn("%s: calloc", __func__);
		goto error;
	}

	req->headers_size = 0;
	req->body_size = 0;

	req->kind = EVHTTP_RESPONSE;
	req->input_headers = mm_calloc(1, sizeof(struct evkeyvalq));
	if (req->input_headers == NULL) {
		event_warn("%s: calloc", __func__);
		goto error;
	}
	TAILQ_INIT(req->input_headers);

	req->output_headers = mm_calloc(1, sizeof(struct evkeyvalq));
	if (req->output_headers == NULL) {
		event_warn("%s: calloc", __func__);
		goto error;
	}
	TAILQ_INIT(req->output_headers);

	if ((req->input_buffer = evbuffer_new()) == NULL) {
		event_warn("%s: evbuffer_new", __func__);
		goto error;
	}

	if ((req->output_buffer = evbuffer_new()) == NULL) {
		event_warn("%s: evbuffer_new", __func__);
		goto error;
	}

	req->cb = cb;
	req->cb_arg = arg;

	return (req);

 error:
	if (req != NULL)
		evhttp_request_free(req);
	return (NULL);
}

void
evhttp_request_free(struct evhttp_request *req)
{
	if ((req->flags & EVHTTP_REQ_DEFER_FREE) != 0) {
		req->flags |= EVHTTP_REQ_NEEDS_FREE;
		return;
	}

	if (req->remote_host != NULL)
		mm_free(req->remote_host);
	if (req->uri != NULL)
		mm_free(req->uri);
	if (req->uri_elems != NULL)
		evhttp_uri_free(req->uri_elems);
	if (req->response_code_line != NULL)
		mm_free(req->response_code_line);
	if (req->host_cache != NULL)
		mm_free(req->host_cache);

	evhttp_clear_headers(req->input_headers);
	mm_free(req->input_headers);

	evhttp_clear_headers(req->output_headers);
	mm_free(req->output_headers);

	if (req->input_buffer != NULL)
		evbuffer_free(req->input_buffer);

	if (req->output_buffer != NULL)
		evbuffer_free(req->output_buffer);

	mm_free(req);
}

void
evhttp_request_own(struct evhttp_request *req)
{
	req->flags |= EVHTTP_USER_OWNED;
}

int
evhttp_request_is_owned(struct evhttp_request *req)
{
	return (req->flags & EVHTTP_USER_OWNED) != 0;
}

struct evhttp_connection *
evhttp_request_get_connection(struct evhttp_request *req)
{
	return req->evcon;
}

struct event_base *
evhttp_connection_get_base(struct evhttp_connection *conn)
{
	return conn->base;
}

void
evhttp_request_set_chunked_cb(struct evhttp_request *req,
    void (*cb)(struct evhttp_request *, void *))
{
	req->chunk_cb = cb;
}





const char *
evhttp_request_get_uri(const struct evhttp_request *req) {
	if (req->uri == NULL)
		event_debug(("%s: request %p has no uri\n", __func__, req));
	return (req->uri);
}

const struct evhttp_uri *
evhttp_request_get_evhttp_uri(const struct evhttp_request *req) {
	if (req->uri_elems == NULL)
		event_debug(("%s: request %p has no uri elems\n",
			    __func__, req));
	return (req->uri_elems);
}

const char *
evhttp_request_get_host(struct evhttp_request *req)
{
	const char *host = NULL;

	if (req->host_cache)
		return req->host_cache;

	if (req->uri_elems)
		host = evhttp_uri_get_host(req->uri_elems);
	if (!host && req->input_headers) {
		const char *p;
		size_t len;

		host = evhttp_find_header(req->input_headers, "Host");
		

		if (host) {
			p = host + strlen(host) - 1;
			while (p > host && EVUTIL_ISDIGIT(*p))
				--p;
			if (p > host && *p == ':') {
				len = p - host;
				req->host_cache = mm_malloc(len + 1);
				if (!req->host_cache) {
					event_warn("%s: malloc", __func__);
					return NULL;
				}
				memcpy(req->host_cache, host, len);
				req->host_cache[len] = '\0';
				host = req->host_cache;
			}
		}
	}

	return host;
}

enum evhttp_cmd_type
evhttp_request_get_command(const struct evhttp_request *req) {
	return (req->type);
}

int
evhttp_request_get_response_code(const struct evhttp_request *req)
{
	return req->response_code;
}


struct evkeyvalq *evhttp_request_get_input_headers(struct evhttp_request *req)
{
	return (req->input_headers);
}


struct evkeyvalq *evhttp_request_get_output_headers(struct evhttp_request *req)
{
	return (req->output_headers);
}


struct evbuffer *evhttp_request_get_input_buffer(struct evhttp_request *req)
{
	return (req->input_buffer);
}


struct evbuffer *evhttp_request_get_output_buffer(struct evhttp_request *req)
{
	return (req->output_buffer);
}







static struct evhttp_connection*
evhttp_get_request_connection(
	struct evhttp* http,
	evutil_socket_t fd, struct sockaddr *sa, ev_socklen_t salen)
{
	struct evhttp_connection *evcon;
	char *hostname = NULL, *portname = NULL;

	name_from_addr(sa, salen, &hostname, &portname);
	if (hostname == NULL || portname == NULL) {
		if (hostname) mm_free(hostname);
		if (portname) mm_free(portname);
		return (NULL);
	}

	event_debug(("%s: new request from %s:%s on "EV_SOCK_FMT"\n",
		__func__, hostname, portname, EV_SOCK_ARG(fd)));

	
	evcon = evhttp_connection_base_new(
		http->base, NULL, hostname, atoi(portname));
	mm_free(hostname);
	mm_free(portname);
	if (evcon == NULL)
		return (NULL);

	evcon->max_headers_size = http->default_max_headers_size;
	evcon->max_body_size = http->default_max_body_size;

	evcon->flags |= EVHTTP_CON_INCOMING;
	evcon->state = EVCON_READING_FIRSTLINE;

	evcon->fd = fd;

	bufferevent_setfd(evcon->bufev, fd);

	return (evcon);
}

static int
evhttp_associate_new_request_with_connection(struct evhttp_connection *evcon)
{
	struct evhttp *http = evcon->http_server;
	struct evhttp_request *req;
	if ((req = evhttp_request_new(evhttp_handle_request, http)) == NULL)
		return (-1);

	if ((req->remote_host = mm_strdup(evcon->address)) == NULL) {
		event_warn("%s: strdup", __func__);
		evhttp_request_free(req);
		return (-1);
	}
	req->remote_port = evcon->port;

	req->evcon = evcon;	
	req->flags |= EVHTTP_REQ_OWN_CONNECTION;

	




	req->userdone = 1;

	TAILQ_INSERT_TAIL(&evcon->requests, req, next);

	req->kind = EVHTTP_REQUEST;


	evhttp_start_read(evcon);

	return (0);
}

static void
evhttp_get_request(struct evhttp *http, evutil_socket_t fd,
    struct sockaddr *sa, ev_socklen_t salen)
{
	struct evhttp_connection *evcon;

	evcon = evhttp_get_request_connection(http, fd, sa, salen);
	if (evcon == NULL) {
		event_sock_warn(fd, "%s: cannot get connection on "EV_SOCK_FMT,
		    __func__, EV_SOCK_ARG(fd));
		evutil_closesocket(fd);
		return;
	}

	
	if (http->timeout != -1)
		evhttp_connection_set_timeout(evcon, http->timeout);

	



	evcon->http_server = http;
	TAILQ_INSERT_TAIL(&http->connections, evcon, next);

	if (evhttp_associate_new_request_with_connection(evcon) == -1)
		evhttp_connection_free(evcon);
}







static void
name_from_addr(struct sockaddr *sa, ev_socklen_t salen,
    char **phost, char **pport)
{
	char ntop[NI_MAXHOST];
	char strport[NI_MAXSERV];
	int ni_result;

#ifdef _EVENT_HAVE_GETNAMEINFO
	ni_result = getnameinfo(sa, salen,
		ntop, sizeof(ntop), strport, sizeof(strport),
		NI_NUMERICHOST|NI_NUMERICSERV);

	if (ni_result != 0) {
#ifdef EAI_SYSTEM
		
		if (ni_result == EAI_SYSTEM)
			event_err(1, "getnameinfo failed");
		else
#endif
			event_errx(1, "getnameinfo failed: %s", gai_strerror(ni_result));
		return;
	}
#else
	ni_result = fake_getnameinfo(sa, salen,
		ntop, sizeof(ntop), strport, sizeof(strport),
		NI_NUMERICHOST|NI_NUMERICSERV);
	if (ni_result != 0)
			return;
#endif

	*phost = mm_strdup(ntop);
	*pport = mm_strdup(strport);
}



static evutil_socket_t
bind_socket_ai(struct evutil_addrinfo *ai, int reuse)
{
	evutil_socket_t fd;

	int on = 1, r;
	int serrno;

	
	fd = socket(ai ? ai->ai_family : AF_INET, SOCK_STREAM, 0);
	if (fd == -1) {
			event_sock_warn(-1, "socket");
			return (-1);
	}

	if (evutil_make_socket_nonblocking(fd) < 0)
		goto out;
	if (evutil_make_socket_closeonexec(fd) < 0)
		goto out;

	if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&on, sizeof(on))<0)
		goto out;
	if (reuse) {
		if (evutil_make_listen_socket_reuseable(fd) < 0)
			goto out;
	}

	if (ai != NULL) {
		r = bind(fd, ai->ai_addr, (ev_socklen_t)ai->ai_addrlen);
		if (r == -1)
			goto out;
	}

	return (fd);

 out:
	serrno = EVUTIL_SOCKET_ERROR();
	evutil_closesocket(fd);
	EVUTIL_SET_SOCKET_ERROR(serrno);
	return (-1);
}

static struct evutil_addrinfo *
make_addrinfo(const char *address, ev_uint16_t port)
{
	struct evutil_addrinfo *ai = NULL;

	struct evutil_addrinfo hints;
	char strport[NI_MAXSERV];
	int ai_result;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	

	hints.ai_flags = EVUTIL_AI_PASSIVE|EVUTIL_AI_ADDRCONFIG;
	evutil_snprintf(strport, sizeof(strport), "%d", port);
	if ((ai_result = evutil_getaddrinfo(address, strport, &hints, &ai))
	    != 0) {
		if (ai_result == EVUTIL_EAI_SYSTEM)
			event_warn("getaddrinfo");
		else
			event_warnx("getaddrinfo: %s",
			    evutil_gai_strerror(ai_result));
		return (NULL);
	}

	return (ai);
}

static evutil_socket_t
bind_socket(const char *address, ev_uint16_t port, int reuse)
{
	evutil_socket_t fd;
	struct evutil_addrinfo *aitop = NULL;

	
	if (address == NULL && port == 0)
		return bind_socket_ai(NULL, 0);

	aitop = make_addrinfo(address, port);

	if (aitop == NULL)
		return (-1);

	fd = bind_socket_ai(aitop, reuse);

	evutil_freeaddrinfo(aitop);

	return (fd);
}

struct evhttp_uri {
	unsigned flags;
	char *scheme; 
	char *userinfo; 
	char *host; 
	int port; 
	char *path; 
	char *query; 
	char *fragment; 
};

struct evhttp_uri *
evhttp_uri_new(void)
{
	struct evhttp_uri *uri = mm_calloc(sizeof(struct evhttp_uri), 1);
	if (uri)
		uri->port = -1;
	return uri;
}

void
evhttp_uri_set_flags(struct evhttp_uri *uri, unsigned flags)
{
	uri->flags = flags;
}




static int
scheme_ok(const char *s, const char *eos)
{
	
	EVUTIL_ASSERT(eos >= s);
	if (s == eos)
		return 0;
	if (!EVUTIL_ISALPHA(*s))
		return 0;
	while (++s < eos) {
		if (! EVUTIL_ISALNUM(*s) &&
		    *s != '+' && *s != '-' && *s != '.')
			return 0;
	}
	return 1;
}

#define SUBDELIMS "!$&'()*+,;="


static int
userinfo_ok(const char *s, const char *eos)
{
	while (s < eos) {
		if (CHAR_IS_UNRESERVED(*s) ||
		    strchr(SUBDELIMS, *s) ||
		    *s == ':')
			++s;
		else if (*s == '%' && s+2 < eos &&
		    EVUTIL_ISXDIGIT(s[1]) &&
		    EVUTIL_ISXDIGIT(s[2]))
			s += 3;
		else
			return 0;
	}
	return 1;
}

static int
regname_ok(const char *s, const char *eos)
{
	while (s && s<eos) {
		if (CHAR_IS_UNRESERVED(*s) ||
		    strchr(SUBDELIMS, *s))
			++s;
		else if (*s == '%' &&
		    EVUTIL_ISXDIGIT(s[1]) &&
		    EVUTIL_ISXDIGIT(s[2]))
			s += 3;
		else
			return 0;
	}
	return 1;
}

static int
parse_port(const char *s, const char *eos)
{
	int portnum = 0;
	while (s < eos) {
		if (! EVUTIL_ISDIGIT(*s))
			return -1;
		portnum = (portnum * 10) + (*s - '0');
		if (portnum < 0)
			return -1;
		++s;
	}
	return portnum;
}


static int
bracket_addr_ok(const char *s, const char *eos)
{
	if (s + 3 > eos || *s != '[' || *(eos-1) != ']')
		return 0;
	if (s[1] == 'v') {
		


		s += 2; 
		--eos;
		if (!EVUTIL_ISXDIGIT(*s)) 
			return 0;
		while (s < eos && *s != '.') {
			if (EVUTIL_ISXDIGIT(*s))
				++s;
			else
				return 0;
		}
		if (*s != '.')
			return 0;
		++s;
		while (s < eos) {
			if (CHAR_IS_UNRESERVED(*s) ||
			    strchr(SUBDELIMS, *s) ||
			    *s == ':')
				++s;
			else
				return 0;
		}
		return 2;
	} else {
		
		char buf[64];
		ev_ssize_t n_chars = eos-s-2;
		struct in6_addr in6;
		if (n_chars >= 64) 
			return 0;
		memcpy(buf, s+1, n_chars);
		buf[n_chars]='\0';
		return (evutil_inet_pton(AF_INET6,buf,&in6)==1) ? 1 : 0;
	}
}

static int
parse_authority(struct evhttp_uri *uri, char *s, char *eos)
{
	char *cp, *port;
	EVUTIL_ASSERT(eos);
	if (eos == s) {
		uri->host = mm_strdup("");
		if (uri->host == NULL) {
			event_warn("%s: strdup", __func__);
			return -1;
		}
		return 0;
	}

	

	cp = strchr(s, '@');
	if (cp && cp < eos) {
		if (! userinfo_ok(s,cp))
			return -1;
		*cp++ = '\0';
		uri->userinfo = mm_strdup(s);
		if (uri->userinfo == NULL) {
			event_warn("%s: strdup", __func__);
			return -1;
		}
	} else {
		cp = s;
	}
	
	for (port=eos-1; port >= cp && EVUTIL_ISDIGIT(*port); --port)
		;
	if (port >= cp && *port == ':') {
		if (port+1 == eos) 

			uri->port = -1;
		else if ((uri->port = parse_port(port+1, eos))<0)
			return -1;
		eos = port;
	}
	

	EVUTIL_ASSERT(eos >= cp);
	if (*cp == '[' && eos >= cp+2 && *(eos-1) == ']') {
		
		if (! bracket_addr_ok(cp, eos))
			return -1;
	} else {
		
		if (! regname_ok(cp,eos)) 
			return -1;
	}
	uri->host = mm_malloc(eos-cp+1);
	if (uri->host == NULL) {
		event_warn("%s: malloc", __func__);
		return -1;
	}
	memcpy(uri->host, cp, eos-cp);
	uri->host[eos-cp] = '\0';
	return 0;

}

static char *
end_of_authority(char *cp)
{
	while (*cp) {
		if (*cp == '?' || *cp == '#' || *cp == '/')
			return cp;
		++cp;
	}
	return cp;
}

enum uri_part {
	PART_PATH,
	PART_QUERY,
	PART_FRAGMENT
};





static char *
end_of_path(char *cp, enum uri_part part, unsigned flags)
{
	if (flags & EVHTTP_URI_NONCONFORMANT) {
		




		switch (part) {
		case PART_PATH:
			while (*cp && *cp != '#' && *cp != '?')
				++cp;
			break;
		case PART_QUERY:
			while (*cp && *cp != '#')
				++cp;
			break;
		case PART_FRAGMENT:
			cp += strlen(cp);
			break;
		};
		return cp;
	}

	while (*cp) {
		if (CHAR_IS_UNRESERVED(*cp) ||
		    strchr(SUBDELIMS, *cp) ||
		    *cp == ':' || *cp == '@' || *cp == '/')
			++cp;
		else if (*cp == '%' && EVUTIL_ISXDIGIT(cp[1]) &&
		    EVUTIL_ISXDIGIT(cp[2]))
			cp += 3;
		else if (*cp == '?' && part != PART_PATH)
			++cp;
		else
			return cp;
	}
	return cp;
}

static int
path_matches_noscheme(const char *cp)
{
	while (*cp) {
		if (*cp == ':')
			return 0;
		else if (*cp == '/')
			return 1;
		++cp;
	}
	return 1;
}

struct evhttp_uri *
evhttp_uri_parse(const char *source_uri)
{
	return evhttp_uri_parse_with_flags(source_uri, 0);
}

struct evhttp_uri *
evhttp_uri_parse_with_flags(const char *source_uri, unsigned flags)
{
	char *readbuf = NULL, *readp = NULL, *token = NULL, *query = NULL;
	char *path = NULL, *fragment = NULL;
	int got_authority = 0;

	struct evhttp_uri *uri = mm_calloc(1, sizeof(struct evhttp_uri));
	if (uri == NULL) {
		event_warn("%s: calloc", __func__);
		goto err;
	}
	uri->port = -1;
	uri->flags = flags;

	readbuf = mm_strdup(source_uri);
	if (readbuf == NULL) {
		event_warn("%s: strdup", __func__);
		goto err;
	}

	readp = readbuf;
	token = NULL;

	







	
	token = strchr(readp, ':');
	if (token && scheme_ok(readp,token)) {
		*token = '\0';
		uri->scheme = mm_strdup(readp);
		if (uri->scheme == NULL) {
			event_warn("%s: strdup", __func__);
			goto err;
		}
		readp = token+1; 
	}

	
	if (readp[0]=='/' && readp[1] == '/') {
		char *authority;
		readp += 2;
		authority = readp;
		path = end_of_authority(readp);
		if (parse_authority(uri, authority, path) < 0)
			goto err;
		readp = path;
		got_authority = 1;
	}

	

	path = readp;
	readp = end_of_path(path, PART_PATH, flags);

	
	if (*readp == '?') {
		*readp = '\0';
		++readp;
		query = readp;
		readp = end_of_path(readp, PART_QUERY, flags);
	}
	
	if (*readp == '#') {
		*readp = '\0';
		++readp;
		fragment = readp;
		readp = end_of_path(readp, PART_FRAGMENT, flags);
	}
	if (*readp != '\0') {
		goto err;
	}

	

	
	if (!got_authority && path[0]=='/' && path[1]=='/')
		goto err;
	

	if (got_authority && path[0] != '/' && path[0] != '\0')
		goto err;
	

	

	if (! uri->scheme && !path_matches_noscheme(path))
		goto err;

	EVUTIL_ASSERT(path);
	uri->path = mm_strdup(path);
	if (uri->path == NULL) {
		event_warn("%s: strdup", __func__);
		goto err;
	}

	if (query) {
		uri->query = mm_strdup(query);
		if (uri->query == NULL) {
			event_warn("%s: strdup", __func__);
			goto err;
		}
	}
	if (fragment) {
		uri->fragment = mm_strdup(fragment);
		if (uri->fragment == NULL) {
			event_warn("%s: strdup", __func__);
			goto err;
		}
	}

	mm_free(readbuf);

	return uri;
err:
	if (uri)
		evhttp_uri_free(uri);
	if (readbuf)
		mm_free(readbuf);
	return NULL;
}

void
evhttp_uri_free(struct evhttp_uri *uri)
{
#define _URI_FREE_STR(f)		\
	if (uri->f) {			\
		mm_free(uri->f);		\
	}

	_URI_FREE_STR(scheme);
	_URI_FREE_STR(userinfo);
	_URI_FREE_STR(host);
	_URI_FREE_STR(path);
	_URI_FREE_STR(query);
	_URI_FREE_STR(fragment);

	mm_free(uri);
#undef _URI_FREE_STR
}

char *
evhttp_uri_join(struct evhttp_uri *uri, char *buf, size_t limit)
{
	struct evbuffer *tmp = 0;
	size_t joined_size = 0;
	char *output = NULL;

#define _URI_ADD(f)	evbuffer_add(tmp, uri->f, strlen(uri->f))

	if (!uri || !buf || !limit)
		return NULL;

	tmp = evbuffer_new();
	if (!tmp)
		return NULL;

	if (uri->scheme) {
		_URI_ADD(scheme);
		evbuffer_add(tmp, ":", 1);
	}
	if (uri->host) {
		evbuffer_add(tmp, "//", 2);
		if (uri->userinfo)
			evbuffer_add_printf(tmp,"%s@", uri->userinfo);
		_URI_ADD(host);
		if (uri->port >= 0)
			evbuffer_add_printf(tmp,":%d", uri->port);

		if (uri->path && uri->path[0] != '/' && uri->path[0] != '\0')
			goto err;
	}

	if (uri->path)
		_URI_ADD(path);

	if (uri->query) {
		evbuffer_add(tmp, "?", 1);
		_URI_ADD(query);
	}

	if (uri->fragment) {
		evbuffer_add(tmp, "#", 1);
		_URI_ADD(fragment);
	}

	evbuffer_add(tmp, "\0", 1); 

	joined_size = evbuffer_get_length(tmp);

	if (joined_size > limit) {
		
		evbuffer_free(tmp);
		return NULL;
	}
       	evbuffer_remove(tmp, buf, joined_size);

	output = buf;
err:
	evbuffer_free(tmp);

	return output;
#undef _URI_ADD
}

const char *
evhttp_uri_get_scheme(const struct evhttp_uri *uri)
{
	return uri->scheme;
}
const char *
evhttp_uri_get_userinfo(const struct evhttp_uri *uri)
{
	return uri->userinfo;
}
const char *
evhttp_uri_get_host(const struct evhttp_uri *uri)
{
	return uri->host;
}
int
evhttp_uri_get_port(const struct evhttp_uri *uri)
{
	return uri->port;
}
const char *
evhttp_uri_get_path(const struct evhttp_uri *uri)
{
	return uri->path;
}
const char *
evhttp_uri_get_query(const struct evhttp_uri *uri)
{
	return uri->query;
}
const char *
evhttp_uri_get_fragment(const struct evhttp_uri *uri)
{
	return uri->fragment;
}

#define _URI_SET_STR(f) do {					\
	if (uri->f)						\
		mm_free(uri->f);				\
	if (f) {						\
		if ((uri->f = mm_strdup(f)) == NULL) {		\
			event_warn("%s: strdup()", __func__);	\
			return -1;				\
		}						\
	} else {						\
		uri->f = NULL;					\
	}							\
	} while(0)

int
evhttp_uri_set_scheme(struct evhttp_uri *uri, const char *scheme)
{
	if (scheme && !scheme_ok(scheme, scheme+strlen(scheme)))
		return -1;

	_URI_SET_STR(scheme);
	return 0;
}
int
evhttp_uri_set_userinfo(struct evhttp_uri *uri, const char *userinfo)
{
	if (userinfo && !userinfo_ok(userinfo, userinfo+strlen(userinfo)))
		return -1;
	_URI_SET_STR(userinfo);
	return 0;
}
int
evhttp_uri_set_host(struct evhttp_uri *uri, const char *host)
{
	if (host) {
		if (host[0] == '[') {
			if (! bracket_addr_ok(host, host+strlen(host)))
				return -1;
		} else {
			if (! regname_ok(host, host+strlen(host)))
				return -1;
		}
	}

	_URI_SET_STR(host);
	return 0;
}
int
evhttp_uri_set_port(struct evhttp_uri *uri, int port)
{
	if (port < -1)
		return -1;
	uri->port = port;
	return 0;
}
#define end_of_cpath(cp,p,f) \
	((const char*)(end_of_path(((char*)(cp)), (p), (f))))

int
evhttp_uri_set_path(struct evhttp_uri *uri, const char *path)
{
	if (path && end_of_cpath(path, PART_PATH, uri->flags) != path+strlen(path))
		return -1;

	_URI_SET_STR(path);
	return 0;
}
int
evhttp_uri_set_query(struct evhttp_uri *uri, const char *query)
{
	if (query && end_of_cpath(query, PART_QUERY, uri->flags) != query+strlen(query))
		return -1;
	_URI_SET_STR(query);
	return 0;
}
int
evhttp_uri_set_fragment(struct evhttp_uri *uri, const char *fragment)
{
	if (fragment && end_of_cpath(fragment, PART_FRAGMENT, uri->flags) != fragment+strlen(fragment))
		return -1;
	_URI_SET_STR(fragment);
	return 0;
}
