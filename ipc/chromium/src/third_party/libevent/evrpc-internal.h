

























#ifndef _EVRPC_INTERNAL_H_
#define _EVRPC_INTERNAL_H_

#include "http-internal.h"

struct evrpc;

#define EVRPC_URI_PREFIX "/.rpc."

struct evrpc_hook {
	TAILQ_ENTRY(evrpc_hook) (next);

	
	int (*process)(struct evhttp_request *, struct evbuffer *, void *);
	void *process_arg;
};

TAILQ_HEAD(evrpc_hook_list, evrpc_hook);






struct _evrpc_hooks {
	
	struct evrpc_hook_list in_hooks;
	struct evrpc_hook_list out_hooks;
};

#define input_hooks common.in_hooks
#define output_hooks common.out_hooks

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

	TAILQ_HEAD(evrpc_requestq, evrpc_request_wrapper) requests;
};


#endif 
