

























#ifndef _EVENT2_TAG_H_
#define _EVENT2_TAG_H_







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

struct evbuffer;







void evtag_init(void);








int evtag_unmarshal_header(struct evbuffer *evbuf, ev_uint32_t *ptag);

void evtag_marshal(struct evbuffer *evbuf, ev_uint32_t tag, const void *data,
    ev_uint32_t len);
void evtag_marshal_buffer(struct evbuffer *evbuf, ev_uint32_t tag,
    struct evbuffer *data);











void evtag_encode_int(struct evbuffer *evbuf, ev_uint32_t number);
void evtag_encode_int64(struct evbuffer *evbuf, ev_uint64_t number);

void evtag_marshal_int(struct evbuffer *evbuf, ev_uint32_t tag,
    ev_uint32_t integer);
void evtag_marshal_int64(struct evbuffer *evbuf, ev_uint32_t tag,
    ev_uint64_t integer);

void evtag_marshal_string(struct evbuffer *buf, ev_uint32_t tag,
    const char *string);

void evtag_marshal_timeval(struct evbuffer *evbuf, ev_uint32_t tag,
    struct timeval *tv);

int evtag_unmarshal(struct evbuffer *src, ev_uint32_t *ptag,
    struct evbuffer *dst);
int evtag_peek(struct evbuffer *evbuf, ev_uint32_t *ptag);
int evtag_peek_length(struct evbuffer *evbuf, ev_uint32_t *plength);
int evtag_payload_length(struct evbuffer *evbuf, ev_uint32_t *plength);
int evtag_consume(struct evbuffer *evbuf);

int evtag_unmarshal_int(struct evbuffer *evbuf, ev_uint32_t need_tag,
    ev_uint32_t *pinteger);
int evtag_unmarshal_int64(struct evbuffer *evbuf, ev_uint32_t need_tag,
    ev_uint64_t *pinteger);

int evtag_unmarshal_fixed(struct evbuffer *src, ev_uint32_t need_tag,
    void *data, size_t len);

int evtag_unmarshal_string(struct evbuffer *evbuf, ev_uint32_t need_tag,
    char **pstring);

int evtag_unmarshal_timeval(struct evbuffer *evbuf, ev_uint32_t need_tag,
    struct timeval *ptv);

#ifdef __cplusplus
}
#endif

#endif
