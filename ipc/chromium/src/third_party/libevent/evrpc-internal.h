

























#ifndef _EVRPC_INTERNAL_H_
#define _EVRPC_INTERNAL_H_

#include "http-internal.h"

struct evrpc;
struct evrpc_request_wrapper;

#define EVRPC_URI_PREFIX "/.rpc."

struct evrpc_hook {
	TAILQ_ENTRY(evrpc_hook) next;

	


	int (*process)(void *, struct evhttp_request *,
	    struct evbuffer *, void *);
	void *process_arg;
};

TAILQ_HEAD(evrpc_hook_list, evrpc_hook);







struct evrpc_hook_ctx;
TAILQ_HEAD(evrpc_pause_list, evrpc_hook_ctx);

struct _evrpc_hooks {
	
	struct evrpc_hook_list in_hooks;
	struct evrpc_hook_list out_hooks;

	struct evrpc_pause_list pause_requests;
};

#define input_hooks common.in_hooks
#define output_hooks common.out_hooks
#define paused_requests common.pause_requests

struct evrpc_base {
	struct _evrpc_hooks common;

	
	struct evhttp* http_server;

	
	TAILQ_HEAD(evrpc_list, evrpc) registered_rpcs;
};

struct evrpc_req_generic;
void evrpc_reqstate_free(struct evrpc_req_generic* rpc_state);


struct evrpc_pool {
	struct _evrpc_hooks common;

	struct event_base *base;

	struct evconq connections;

	int timeout;

	TAILQ_HEAD(evrpc_requestq, evrpc_request_wrapper) (requests);
};

struct evrpc_hook_ctx {
	TAILQ_ENTRY(evrpc_hook_ctx) next;

	void *ctx;
	void (*cb)(void *, enum EVRPC_HOOK_RESULT);
};

struct evrpc_meta {
	TAILQ_ENTRY(evrpc_meta) next;
	char *key;

	void *data;
	size_t data_size;
};

TAILQ_HEAD(evrpc_meta_list, evrpc_meta);

struct evrpc_hook_meta {
	struct evrpc_meta_list meta_data;
	struct evhttp_connection *evcon;
};


static void evrpc_hook_associate_meta(struct evrpc_hook_meta **pctx,
    struct evhttp_connection *evcon);


static struct evrpc_hook_meta *evrpc_hook_meta_new(void);


static void evrpc_hook_context_free(struct evrpc_hook_meta *ctx);




struct evrpc_req_generic {
	



	struct evrpc_hook_meta *hook_meta;

	
	void *request;

	
	void *reply;

	



	struct evrpc *rpc;

	


	struct evhttp_request* http_req;

	


	struct evbuffer* rpc_data;
};


struct evrpc_request_wrapper {
	



	struct evrpc_hook_meta *hook_meta;

	TAILQ_ENTRY(evrpc_request_wrapper) next;

	
	struct evrpc_pool *pool;

	
	struct evhttp_connection *evcon;

	
	struct evhttp_request *req;

	
	struct event ev_timeout;

	
	char *name;

	
	void (*cb)(struct evrpc_status*, void *request, void *reply, void *arg);
	void *cb_arg;

	void *request;
	void *reply;

	
	void (*request_marshal)(struct evbuffer *, void *);

	
	void (*reply_clear)(void *);

	
	int (*reply_unmarshal)(void *, struct evbuffer*);
};

#endif 
