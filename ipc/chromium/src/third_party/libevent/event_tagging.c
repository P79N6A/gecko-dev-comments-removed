


























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_PARAM_H
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
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include <syslog.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "event.h"
#include "evutil.h"
#include "log.h"

int evtag_decode_int(ev_uint32_t *pnumber, struct evbuffer *evbuf);
int evtag_encode_tag(struct evbuffer *evbuf, ev_uint32_t tag);
int evtag_decode_tag(ev_uint32_t *ptag, struct evbuffer *evbuf);

static struct evbuffer *_buf;	

void
evtag_init(void)
{
	if (_buf != NULL)
		return;

	if ((_buf = evbuffer_new()) == NULL)
		event_err(1, "%s: malloc", __func__);
}







void
encode_int(struct evbuffer *evbuf, ev_uint32_t number)
{
	int off = 1, nibbles = 0;
	ev_uint8_t data[5];

	memset(data, 0, sizeof(ev_uint32_t)+1);
	while (number) {
		if (off & 0x1)
			data[off/2] = (data[off/2] & 0xf0) | (number & 0x0f);
		else
			data[off/2] = (data[off/2] & 0x0f) |
			    ((number & 0x0f) << 4);
		number >>= 4;
		off++;
	}

	if (off > 2)
		nibbles = off - 2;

	
	data[0] = (data[0] & 0x0f) | ((nibbles & 0x0f) << 4);

	evbuffer_add(evbuf, data, (off + 1) / 2);
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
	ev_uint8_t *data = EVBUFFER_DATA(evbuf);
	int len = EVBUFFER_LENGTH(evbuf);
	int count = 0, shift = 0, done = 0;

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

	return (count);
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
	encode_int(evbuf, len);
	evbuffer_add(evbuf, (void *)data, len);
}


void
evtag_marshal_int(struct evbuffer *evbuf, ev_uint32_t tag, ev_uint32_t integer)
{
	evbuffer_drain(_buf, EVBUFFER_LENGTH(_buf));
	encode_int(_buf, integer);

	evtag_encode_tag(evbuf, tag);
	encode_int(evbuf, EVBUFFER_LENGTH(_buf));
	evbuffer_add_buffer(evbuf, _buf);
}

void
evtag_marshal_string(struct evbuffer *buf, ev_uint32_t tag, const char *string)
{
	evtag_marshal(buf, tag, string, strlen(string));
}

void
evtag_marshal_timeval(struct evbuffer *evbuf, ev_uint32_t tag, struct timeval *tv)
{
	evbuffer_drain(_buf, EVBUFFER_LENGTH(_buf));

	encode_int(_buf, tv->tv_sec);
	encode_int(_buf, tv->tv_usec);

	evtag_marshal(evbuf, tag, EVBUFFER_DATA(_buf),
	    EVBUFFER_LENGTH(_buf));
}

static int
decode_int_internal(ev_uint32_t *pnumber, struct evbuffer *evbuf, int dodrain)
{
	ev_uint32_t number = 0;
	ev_uint8_t *data = EVBUFFER_DATA(evbuf);
	int len = EVBUFFER_LENGTH(evbuf);
	int nibbles = 0;

	if (!len)
		return (-1);

	nibbles = ((data[0] & 0xf0) >> 4) + 1;
	if (nibbles > 8 || (nibbles >> 1) + 1 > len)
		return (-1);
	len = (nibbles >> 1) + 1;

	while (nibbles > 0) {
		number <<= 4;
		if (nibbles & 0x1)
			number |= data[nibbles >> 1] & 0x0f;
		else
			number |= (data[nibbles >> 1] & 0xf0) >> 4;
		nibbles--;
	}

	if (dodrain)
		evbuffer_drain(evbuf, len);

	*pnumber = number;

	return (len);
}

int
evtag_decode_int(ev_uint32_t *pnumber, struct evbuffer *evbuf)
{
	return (decode_int_internal(pnumber, evbuf, 1) == -1 ? -1 : 0);
}

int
evtag_peek(struct evbuffer *evbuf, ev_uint32_t *ptag)
{
	return (decode_tag_internal(ptag, evbuf, 0 ));
}

int
evtag_peek_length(struct evbuffer *evbuf, ev_uint32_t *plength)
{
	struct evbuffer tmp;
	int res, len;

	len = decode_tag_internal(NULL, evbuf, 0 );
	if (len == -1)
		return (-1);

	tmp = *evbuf;
	tmp.buffer += len;
	tmp.off -= len;

	res = decode_int_internal(plength, &tmp, 0);
	if (res == -1)
		return (-1);

	*plength += res + len;

	return (0);
}

int
evtag_payload_length(struct evbuffer *evbuf, ev_uint32_t *plength)
{
	struct evbuffer tmp;
	int res, len;

	len = decode_tag_internal(NULL, evbuf, 0 );
	if (len == -1)
		return (-1);

	tmp = *evbuf;
	tmp.buffer += len;
	tmp.off -= len;

	res = decode_int_internal(plength, &tmp, 0);
	if (res == -1)
		return (-1);

	return (0);
}

int
evtag_consume(struct evbuffer *evbuf)
{
	ev_uint32_t len;
	if (decode_tag_internal(NULL, evbuf, 1 ) == -1)
		return (-1);
	if (evtag_decode_int(&len, evbuf) == -1)
		return (-1);
	evbuffer_drain(evbuf, len);

	return (0);
}



int
evtag_unmarshal(struct evbuffer *src, ev_uint32_t *ptag, struct evbuffer *dst)
{
	ev_uint32_t len;
	ev_uint32_t integer;

	if (decode_tag_internal(ptag, src, 1 ) == -1)
		return (-1);
	if (evtag_decode_int(&integer, src) == -1)
		return (-1);
	len = integer;

	if (EVBUFFER_LENGTH(src) < len)
		return (-1);

	if (evbuffer_add(dst, EVBUFFER_DATA(src), len) == -1)
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
	ev_uint32_t integer;

	if (decode_tag_internal(&tag, evbuf, 1 ) == -1)
		return (-1);
	if (need_tag != tag)
		return (-1);
	if (evtag_decode_int(&integer, evbuf) == -1)
		return (-1);
	len = integer;

	if (EVBUFFER_LENGTH(evbuf) < len)
		return (-1);
	
	evbuffer_drain(_buf, EVBUFFER_LENGTH(_buf));
	if (evbuffer_add(_buf, EVBUFFER_DATA(evbuf), len) == -1)
		return (-1);

	evbuffer_drain(evbuf, len);

	return (evtag_decode_int(pinteger, _buf));
}



int
evtag_unmarshal_fixed(struct evbuffer *src, ev_uint32_t need_tag, void *data,
    size_t len)
{
	ev_uint32_t tag;

	
	evbuffer_drain(_buf, EVBUFFER_LENGTH(_buf));

	
	if (evtag_unmarshal(src, &tag, _buf) == -1 || tag != need_tag)
		return (-1);

	if (EVBUFFER_LENGTH(_buf) != len)
		return (-1);

	memcpy(data, EVBUFFER_DATA(_buf), len);
	return (0);
}

int
evtag_unmarshal_string(struct evbuffer *evbuf, ev_uint32_t need_tag,
    char **pstring)
{
	ev_uint32_t tag;

	evbuffer_drain(_buf, EVBUFFER_LENGTH(_buf));

	if (evtag_unmarshal(evbuf, &tag, _buf) == -1 || tag != need_tag)
		return (-1);

	*pstring = calloc(EVBUFFER_LENGTH(_buf) + 1, 1);
	if (*pstring == NULL)
		event_err(1, "%s: calloc", __func__);
	evbuffer_remove(_buf, *pstring, EVBUFFER_LENGTH(_buf));

	return (0);
}

int
evtag_unmarshal_timeval(struct evbuffer *evbuf, ev_uint32_t need_tag,
    struct timeval *ptv)
{
	ev_uint32_t tag;
	ev_uint32_t integer;

	evbuffer_drain(_buf, EVBUFFER_LENGTH(_buf));
	if (evtag_unmarshal(evbuf, &tag, _buf) == -1 || tag != need_tag)
		return (-1);

	if (evtag_decode_int(&integer, _buf) == -1)
		return (-1);
	ptv->tv_sec = integer;
	if (evtag_decode_int(&integer, _buf) == -1)
		return (-1);
	ptv->tv_usec = integer;

	return (0);
}
