

























#ifndef _EVENT2_RPC_H_
#define _EVENT2_RPC_H_

#ifdef __cplusplus
extern "C" {
#endif











































#define EVTAG_HAS(msg, member) \
	((msg)->member##_set == 1)

#ifndef _EVENT2_RPC_COMPAT_H_








#define EVTAG_ASSIGN(msg, member, value) \
	(*(msg)->base->member##_assign)((msg), (value))








#define EVTAG_ASSIGN_WITH_LEN(msg, member, value, len)	\
	(*(msg)->base->member##_assign)((msg), (value), (len))








#define EVTAG_GET(msg, member, pvalue) \
	(*(msg)->base->member##_get)((msg), (pvalue))









#define EVTAG_GET_WITH_LEN(msg, member, pvalue, plen)	\
	(*(msg)->base->member##_get)((msg), (pvalue), (plen))

#endif  




#define EVTAG_ARRAY_ADD_VALUE(msg, member, value) \
	(*(msg)->base->member##_add)((msg), (value))



#define EVTAG_ARRAY_ADD(msg, member) \
	(*(msg)->base->member##_add)(msg)



#define EVTAG_ARRAY_GET(msg, member, offset, pvalue)	\
	(*(msg)->base->member##_get)((msg), (offset), (pvalue))



#define EVTAG_ARRAY_LEN(msg, member) ((msg)->member##_length)


struct evbuffer;
struct event_base;
struct evrpc_req_generic;
struct evrpc_request_wrapper;
struct evrpc;





#define EVRPC_STRUCT(rpcname) struct evrpc_req__##rpcname

struct evhttp_request;
struct evrpc_status;
struct evrpc_hook_meta;












#define EVRPC_HEADER(rpcname, reqstruct, rplystruct) \
EVRPC_STRUCT(rpcname) {	\
	struct evrpc_hook_meta *hook_meta; \
	struct reqstruct* request; \
	struct rplystruct* reply; \
	struct evrpc* rpc; \
	struct evhttp_request* http_req; \
	struct evbuffer* rpc_data; \
};								     \
int evrpc_send_request_##rpcname(struct evrpc_pool *, \
    struct reqstruct *, struct rplystruct *, \
    void (*)(struct evrpc_status *, \
	struct reqstruct *, struct rplystruct *, void *cbarg),	\
    void *);

struct evrpc_pool;


struct evrpc_request_wrapper *evrpc_make_request_ctx(
	struct evrpc_pool *pool, void *request, void *reply,
	const char *rpcname,
	void (*req_marshal)(struct evbuffer*, void *),
	void (*rpl_clear)(void *),
	int (*rpl_unmarshal)(void *, struct evbuffer *),
	void (*cb)(struct evrpc_status *, void *, void *, void *),
	void *cbarg);















#define EVRPC_MAKE_CTX(rpcname, reqstruct, rplystruct, \
    pool, request, reply, cb, cbarg)					\
	evrpc_make_request_ctx(pool, request, reply,			\
	    #rpcname,							\
	    (void (*)(struct evbuffer *, void *))reqstruct##_marshal,	\
	    (void (*)(void *))rplystruct##_clear,			\
	    (int (*)(void *, struct evbuffer *))rplystruct##_unmarshal, \
	    (void (*)(struct evrpc_status *, void *, void *, void *))cb, \
	    cbarg)











#define EVRPC_GENERATE(rpcname, reqstruct, rplystruct)			\
	int evrpc_send_request_##rpcname(struct evrpc_pool *pool,	\
	    struct reqstruct *request, struct rplystruct *reply,	\
	    void (*cb)(struct evrpc_status *,				\
		struct reqstruct *, struct rplystruct *, void *cbarg),	\
	    void *cbarg) {						\
	return evrpc_send_request_generic(pool, request, reply,	\
	    (void (*)(struct evrpc_status *, void *, void *, void *))cb, \
	    cbarg,							\
	    #rpcname,							\
	    (void (*)(struct evbuffer *, void *))reqstruct##_marshal,	\
	    (void (*)(void *))rplystruct##_clear,			\
	    (int (*)(void *, struct evbuffer *))rplystruct##_unmarshal); \
}










#define EVRPC_REQUEST_HTTP(rpc_req) (rpc_req)->http_req


void evrpc_request_done(struct evrpc_req_generic *req);


void *evrpc_get_request(struct evrpc_req_generic *req);
void *evrpc_get_reply(struct evrpc_req_generic *req);









#define EVRPC_REQUEST_DONE(rpc_req) do { \
  struct evrpc_req_generic *_req = (struct evrpc_req_generic *)(rpc_req); \
  evrpc_request_done(_req);					\
} while (0)


struct evrpc_base;
struct evhttp;









struct evrpc_base *evrpc_init(struct evhttp *server);









void evrpc_free(struct evrpc_base *base);

















#define EVRPC_REGISTER(base, name, request, reply, callback, cbarg)	\
	evrpc_register_generic(base, #name,				\
	    (void (*)(struct evrpc_req_generic *, void *))callback, cbarg, \
	    (void *(*)(void *))request##_new, NULL,			\
	    (void (*)(void *))request##_free,				\
	    (int (*)(void *, struct evbuffer *))request##_unmarshal,	\
	    (void *(*)(void *))reply##_new, NULL,			\
	    (void (*)(void *))reply##_free, \
	    (int (*)(void *))reply##_complete, \
	    (void (*)(struct evbuffer *, void *))reply##_marshal)








int evrpc_register_rpc(struct evrpc_base *, struct evrpc *,
    void (*)(struct evrpc_req_generic*, void *), void *);









#define EVRPC_UNREGISTER(base, name) evrpc_unregister_rpc((base), #name)

int evrpc_unregister_rpc(struct evrpc_base *base, const char *name);





struct evhttp_connection;
struct evrpc_status;
















#define EVRPC_MAKE_REQUEST(name, pool, request, reply, cb, cbarg)	\
	evrpc_send_request_##name((pool), (request), (reply), (cb), (cbarg))












int evrpc_make_request(struct evrpc_request_wrapper *ctx);











struct evrpc_pool *evrpc_pool_new(struct event_base *base);





void evrpc_pool_free(struct evrpc_pool *pool);









void evrpc_pool_add_connection(struct evrpc_pool *pool,
    struct evhttp_connection *evcon);









void evrpc_pool_remove_connection(struct evrpc_pool *pool,
    struct evhttp_connection *evcon);
















void evrpc_pool_set_timeout(struct evrpc_pool *pool, int timeout_in_secs);






enum EVRPC_HOOK_TYPE {
	EVRPC_INPUT,		
	EVRPC_OUTPUT		
};

#ifndef WIN32


#define INPUT EVRPC_INPUT


#define OUTPUT EVRPC_OUTPUT
#endif





enum EVRPC_HOOK_RESULT {
	EVRPC_TERMINATE = -1,	
	EVRPC_CONTINUE = 0,	
	EVRPC_PAUSE = 1		
};

















void *evrpc_add_hook(void *vbase,
    enum EVRPC_HOOK_TYPE hook_type,
    int (*cb)(void *, struct evhttp_request *, struct evbuffer *, void *),
    void *cb_arg);









int evrpc_remove_hook(void *vbase,
    enum EVRPC_HOOK_TYPE hook_type,
    void *handle);






int
evrpc_resume_request(void *vbase, void *ctx, enum EVRPC_HOOK_RESULT res);












void evrpc_hook_add_meta(void *ctx, const char *key,
    const void *data, size_t data_size);











int evrpc_hook_find_meta(void *ctx, const char *key,
    void **data, size_t *data_size);







struct evhttp_connection *evrpc_hook_get_connection(void *ctx);








int evrpc_send_request_generic(struct evrpc_pool *pool,
    void *request, void *reply,
    void (*cb)(struct evrpc_status *, void *, void *, void *),
    void *cb_arg,
    const char *rpcname,
    void (*req_marshal)(struct evbuffer *, void *),
    void (*rpl_clear)(void *),
    int (*rpl_unmarshal)(void *, struct evbuffer *));








int
evrpc_register_generic(struct evrpc_base *base, const char *name,
    void (*callback)(struct evrpc_req_generic *, void *), void *cbarg,
    void *(*req_new)(void *), void *req_new_arg, void (*req_free)(void *),
    int (*req_unmarshal)(void *, struct evbuffer *),
    void *(*rpl_new)(void *), void *rpl_new_arg, void (*rpl_free)(void *),
    int (*rpl_complete)(void *),
    void (*rpl_marshal)(struct evbuffer *, void *));


struct evrpc_pool* evrpc_request_get_pool(struct evrpc_request_wrapper *ctx);
void evrpc_request_set_pool(struct evrpc_request_wrapper *ctx,
    struct evrpc_pool *pool);
void evrpc_request_set_cb(struct evrpc_request_wrapper *ctx,
    void (*cb)(struct evrpc_status*, void *request, void *reply, void *arg),
    void *cb_arg);

#ifdef __cplusplus
}
#endif

#endif
