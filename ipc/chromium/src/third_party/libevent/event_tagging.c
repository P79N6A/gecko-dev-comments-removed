


























#include "event2/event-config.h"

#ifdef _EVENT_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef _EVENT_HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#else
#include <sys/ioctl.h>
#endif

#include <sys/queue.h>
#ifdef _EVENT_HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include <syslog.h>
#endif
#ifdef _EVENT_HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <limits.h>

#include "event2/event.h"
#include "event2/tag.h"
#include "event2/buffer.h"
#include "log-internal.h"
#include "mm-internal.h"
#include "util-internal.h"

























int evtag_decode_int(ev_uint32_t *pnumber, struct evbuffer *evbuf);
int evtag_decode_int64(ev_uint64_t *pnumber, struct evbuffer *evbuf);
int evtag_encode_tag(struct evbuffer *evbuf, ev_uint32_t tag);
int evtag_decode_tag(ev_uint32_t *ptag, struct evbuffer *evbuf);

void
evtag_init(void)
{
}












#define ENCODE_INT_INTERNAL(data, number) do {				\
	int off = 1, nibbles = 0;					\
									\
	memset(data, 0, sizeof(number)+1);				\
	while (number) {						\
		if (off & 0x1)						\
			data[off/2] = (data[off/2] & 0xf0) | (number & 0x0f); \
		else							\
			data[off/2] = (data[off/2] & 0x0f) |		\
			    ((number & 0x0f) << 4);			\
		number >>= 4;						\
		off++;							\
	}								\
									\
	if (off > 2)							\
		nibbles = off - 2;					\
									\
	/* Off - 1 is the number of encoded nibbles */			\
	data[0] = (data[0] & 0x0f) | ((nibbles & 0x0f) << 4);		\
									\
	return ((off + 1) / 2);						\
} while (0)

static inline int
encode_int_internal(ev_uint8_t *data, ev_uint32_t number)
{
	ENCODE_INT_INTERNAL(data, number);
}

static inline int
encode_int64_internal(ev_uint8_t *data, ev_uint64_t number)
{
	ENCODE_INT_INTERNAL(data, number);
}

void
evtag_encode_int(struct evbuffer *evbuf, ev_uint32_t number)
{
	ev_uint8_t data[5];
	int len = encode_int_internal(data, number);
	evbuffer_add(evbuf, data, len);
}

void
evtag_encode_int64(struct evbuffer *evbuf, ev_uint64_t number)
{
	ev_uint8_t data[9];
	int len = encode_int64_internal(data, number);
	evbuffer_add(evbuf, data, len);
}






int
evtag_encode_tag(struct evbuffer *evbuf, ev_uint32_t tag)
{
	int bytes = 0;
	ev_uint8_t data[5];

	memset(data, 0, sizeof(data));
	do {
		ev_uint8_t lower = tag & 0x7f;
		tag >>= 7;

		if (tag)
			lower |= 0x80;

		data[bytes++] = lower;
	} while (tag);

	if (evbuf != NULL)
		evbuffer_add(evbuf, data, bytes);

	return (bytes);
}

static int
decode_tag_internal(ev_uint32_t *ptag, struct evbuffer *evbuf, int dodrain)
{
	ev_uint32_t number = 0;
	size_t len = evbuffer_get_length(evbuf);
	ev_uint8_t *data;
	size_t count = 0;
	int  shift = 0, done = 0;

	



	data = evbuffer_pullup(
		evbuf, len < sizeof(number) + 1 ? len : sizeof(number) + 1);

	while (count++ < len) {
		ev_uint8_t lower = *data++;
		number |= (lower & 0x7f) << shift;
		shift += 7;

		if (!(lower & 0x80)) {
			done = 1;
			break;
		}
	}

	if (!done)
		return (-1);

	if (dodrain)
		evbuffer_drain(evbuf, count);

	if (ptag != NULL)
		*ptag = number;

	return count > INT_MAX ? INT_MAX : (int)(count);
}

int
evtag_decode_tag(ev_uint32_t *ptag, struct evbuffer *evbuf)
{
	return (decode_tag_internal(ptag, evbuf, 1 ));
}







void
evtag_marshal(struct evbuffer *evbuf, ev_uint32_t tag,
    const void *data, ev_uint32_t len)
{
	evtag_encode_tag(evbuf, tag);
	evtag_encode_int(evbuf, len);
	evbuffer_add(evbuf, (void *)data, len);
}

void
evtag_marshal_buffer(struct evbuffer *evbuf, ev_uint32_t tag,
    struct evbuffer *data)
{
	evtag_encode_tag(evbuf, tag);
	
	evtag_encode_int(evbuf, (ev_uint32_t)evbuffer_get_length(data));
	evbuffer_add_buffer(evbuf, data);
}


void
evtag_marshal_int(struct evbuffer *evbuf, ev_uint32_t tag, ev_uint32_t integer)
{
	ev_uint8_t data[5];
	int len = encode_int_internal(data, integer);

	evtag_encode_tag(evbuf, tag);
	evtag_encode_int(evbuf, len);
	evbuffer_add(evbuf, data, len);
}

void
evtag_marshal_int64(struct evbuffer *evbuf, ev_uint32_t tag,
    ev_uint64_t integer)
{
	ev_uint8_t data[9];
	int len = encode_int64_internal(data, integer);

	evtag_encode_tag(evbuf, tag);
	evtag_encode_int(evbuf, len);
	evbuffer_add(evbuf, data, len);
}

void
evtag_marshal_string(struct evbuffer *buf, ev_uint32_t tag, const char *string)
{
	
	evtag_marshal(buf, tag, string, (ev_uint32_t)strlen(string));
}

void
evtag_marshal_timeval(struct evbuffer *evbuf, ev_uint32_t tag, struct timeval *tv)
{
	ev_uint8_t data[10];
	int len = encode_int_internal(data, tv->tv_sec);
	len += encode_int_internal(data + len, tv->tv_usec);
	evtag_marshal(evbuf, tag, data, len);
}

#define DECODE_INT_INTERNAL(number, maxnibbles, pnumber, evbuf, offset) \
do {									\
	ev_uint8_t *data;						\
	ev_ssize_t len = evbuffer_get_length(evbuf) - offset;		\
	int nibbles = 0;						\
									\
	if (len <= 0)							\
		return (-1);						\
									\
	/* XXX(niels): faster? */					\
	data = evbuffer_pullup(evbuf, offset + 1) + offset;		\
									\
	nibbles = ((data[0] & 0xf0) >> 4) + 1;				\
	if (nibbles > maxnibbles || (nibbles >> 1) + 1 > len)		\
		return (-1);						\
	len = (nibbles >> 1) + 1;					\
									\
	data = evbuffer_pullup(evbuf, offset + len) + offset;		\
									\
	while (nibbles > 0) {						\
		number <<= 4;						\
		if (nibbles & 0x1)					\
			number |= data[nibbles >> 1] & 0x0f;		\
		else							\
			number |= (data[nibbles >> 1] & 0xf0) >> 4;	\
		nibbles--;						\
	}								\
									\
	*pnumber = number;						\
									\
	return (int)(len);						\
} while (0)










static int
decode_int_internal(ev_uint32_t *pnumber, struct evbuffer *evbuf, int offset)
{
	ev_uint32_t number = 0;
	DECODE_INT_INTERNAL(number, 8, pnumber, evbuf, offset);
}

static int
decode_int64_internal(ev_uint64_t *pnumber, struct evbuffer *evbuf, int offset)
{
	ev_uint64_t number = 0;
	DECODE_INT_INTERNAL(number, 16, pnumber, evbuf, offset);
}

int
evtag_decode_int(ev_uint32_t *pnumber, struct evbuffer *evbuf)
{
	int res = decode_int_internal(pnumber, evbuf, 0);
	if (res != -1)
		evbuffer_drain(evbuf, res);

	return (res == -1 ? -1 : 0);
}

int
evtag_decode_int64(ev_uint64_t *pnumber, struct evbuffer *evbuf)
{
	int res = decode_int64_internal(pnumber, evbuf, 0);
	if (res != -1)
		evbuffer_drain(evbuf, res);

	return (res == -1 ? -1 : 0);
}

int
evtag_peek(struct evbuffer *evbuf, ev_uint32_t *ptag)
{
	return (decode_tag_internal(ptag, evbuf, 0 ));
}

int
evtag_peek_length(struct evbuffer *evbuf, ev_uint32_t *plength)
{
	int res, len;

	len = decode_tag_internal(NULL, evbuf, 0 );
	if (len == -1)
		return (-1);

	res = decode_int_internal(plength, evbuf, len);
	if (res == -1)
		return (-1);

	*plength += res + len;

	return (0);
}

int
evtag_payload_length(struct evbuffer *evbuf, ev_uint32_t *plength)
{
	int res, len;

	len = decode_tag_internal(NULL, evbuf, 0 );
	if (len == -1)
		return (-1);

	res = decode_int_internal(plength, evbuf, len);
	if (res == -1)
		return (-1);

	return (0);
}



int
evtag_unmarshal_header(struct evbuffer *evbuf, ev_uint32_t *ptag)
{
	ev_uint32_t len;

	if (decode_tag_internal(ptag, evbuf, 1 ) == -1)
		return (-1);
	if (evtag_decode_int(&len, evbuf) == -1)
		return (-1);

	if (evbuffer_get_length(evbuf) < len)
		return (-1);

	return (len);
}

int
evtag_consume(struct evbuffer *evbuf)
{
	int len;
	if ((len = evtag_unmarshal_header(evbuf, NULL)) == -1)
		return (-1);
	evbuffer_drain(evbuf, len);

	return (0);
}



int
evtag_unmarshal(struct evbuffer *src, ev_uint32_t *ptag, struct evbuffer *dst)
{
	int len;

	if ((len = evtag_unmarshal_header(src, ptag)) == -1)
		return (-1);

	if (evbuffer_add(dst, evbuffer_pullup(src, len), len) == -1)
		return (-1);

	evbuffer_drain(src, len);

	return (len);
}



int
evtag_unmarshal_int(struct evbuffer *evbuf, ev_uint32_t need_tag,
    ev_uint32_t *pinteger)
{
	ev_uint32_t tag;
	ev_uint32_t len;
	int result;

	if (decode_tag_internal(&tag, evbuf, 1 ) == -1)
		return (-1);
	if (need_tag != tag)
		return (-1);
	if (evtag_decode_int(&len, evbuf) == -1)
		return (-1);

	if (evbuffer_get_length(evbuf) < len)
		return (-1);

	result = decode_int_internal(pinteger, evbuf, 0);
	evbuffer_drain(evbuf, len);
	if (result < 0 || (size_t)result > len) 
		return (-1);
	else
		return result;
}

int
evtag_unmarshal_int64(struct evbuffer *evbuf, ev_uint32_t need_tag,
    ev_uint64_t *pinteger)
{
	ev_uint32_t tag;
	ev_uint32_t len;
	int result;

	if (decode_tag_internal(&tag, evbuf, 1 ) == -1)
		return (-1);
	if (need_tag != tag)
		return (-1);
	if (evtag_decode_int(&len, evbuf) == -1)
		return (-1);

	if (evbuffer_get_length(evbuf) < len)
		return (-1);

	result = decode_int64_internal(pinteger, evbuf, 0);
	evbuffer_drain(evbuf, len);
	if (result < 0 || (size_t)result > len) 
		return (-1);
	else
		return result;
}



int
evtag_unmarshal_fixed(struct evbuffer *src, ev_uint32_t need_tag, void *data,
    size_t len)
{
	ev_uint32_t tag;
	int tag_len;

	
	if ((tag_len = evtag_unmarshal_header(src, &tag)) < 0 ||
	    tag != need_tag)
		return (-1);

	if ((size_t)tag_len != len)
		return (-1);

	evbuffer_remove(src, data, len);
	return (0);
}

int
evtag_unmarshal_string(struct evbuffer *evbuf, ev_uint32_t need_tag,
    char **pstring)
{
	ev_uint32_t tag;
	int tag_len;

	if ((tag_len = evtag_unmarshal_header(evbuf, &tag)) == -1 ||
	    tag != need_tag)
		return (-1);

	*pstring = mm_malloc(tag_len + 1);
	if (*pstring == NULL) {
		event_warn("%s: malloc", __func__);
		return -1;
	}
	evbuffer_remove(evbuf, *pstring, tag_len);
	(*pstring)[tag_len] = '\0';

	return (0);
}

int
evtag_unmarshal_timeval(struct evbuffer *evbuf, ev_uint32_t need_tag,
    struct timeval *ptv)
{
	ev_uint32_t tag;
	ev_uint32_t integer;
	int len, offset, offset2;
	int result = -1;

	if ((len = evtag_unmarshal_header(evbuf, &tag)) == -1)
		return (-1);
	if (tag != need_tag)
		goto done;
	if ((offset = decode_int_internal(&integer, evbuf, 0)) == -1)
		goto done;
	ptv->tv_sec = integer;
	if ((offset2 = decode_int_internal(&integer, evbuf, offset)) == -1)
		goto done;
	ptv->tv_usec = integer;
	if (offset + offset2 > len) 
		goto done;

	result = 0;
 done:
	evbuffer_drain(evbuf, len);
	return result;
}
