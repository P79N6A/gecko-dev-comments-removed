


























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#endif

#ifdef HAVE_VASPRINTF

#define _GNU_SOURCE
#endif

#include <sys/types.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "event.h"
#include "config.h"
#include "evutil.h"

struct evbuffer *
evbuffer_new(void)
{
	struct evbuffer *buffer;
	
	buffer = calloc(1, sizeof(struct evbuffer));

	return (buffer);
}

void
evbuffer_free(struct evbuffer *buffer)
{
	if (buffer->orig_buffer != NULL)
		free(buffer->orig_buffer);
	free(buffer);
}






#define SWAP(x,y) do { \
	(x)->buffer = (y)->buffer; \
	(x)->orig_buffer = (y)->orig_buffer; \
	(x)->misalign = (y)->misalign; \
	(x)->totallen = (y)->totallen; \
	(x)->off = (y)->off; \
} while (0)

int
evbuffer_add_buffer(struct evbuffer *outbuf, struct evbuffer *inbuf)
{
	int res;

	
	if (outbuf->off == 0) {
		struct evbuffer tmp;
		size_t oldoff = inbuf->off;

		
		SWAP(&tmp, outbuf);
		SWAP(outbuf, inbuf);
		SWAP(inbuf, &tmp);

		




		if (inbuf->off != oldoff && inbuf->cb != NULL)
			(*inbuf->cb)(inbuf, oldoff, inbuf->off, inbuf->cbarg);
		if (oldoff && outbuf->cb != NULL)
			(*outbuf->cb)(outbuf, 0, oldoff, outbuf->cbarg);
		
		return (0);
	}

	res = evbuffer_add(outbuf, inbuf->buffer, inbuf->off);
	if (res == 0) {
		
		evbuffer_drain(inbuf, inbuf->off);
	}

	return (res);
}

int
evbuffer_add_vprintf(struct evbuffer *buf, const char *fmt, va_list ap)
{
	char *buffer;
	size_t space;
	size_t oldoff = buf->off;
	int sz;
	va_list aq;

	
	evbuffer_expand(buf, 64);
	for (;;) {
		size_t used = buf->misalign + buf->off;
		buffer = (char *)buf->buffer + buf->off;
		assert(buf->totallen >= used);
		space = buf->totallen - used;

#ifndef va_copy
#define	va_copy(dst, src)	memcpy(&(dst), &(src), sizeof(va_list))
#endif
		va_copy(aq, ap);

		sz = evutil_vsnprintf(buffer, space, fmt, aq);

		va_end(aq);

		if (sz < 0)
			return (-1);
		if (sz < space) {
			buf->off += sz;
			if (buf->cb != NULL)
				(*buf->cb)(buf, oldoff, buf->off, buf->cbarg);
			return (sz);
		}
		if (evbuffer_expand(buf, sz + 1) == -1)
			return (-1);

	}
	
}

int
evbuffer_add_printf(struct evbuffer *buf, const char *fmt, ...)
{
	int res = -1;
	va_list ap;

	va_start(ap, fmt);
	res = evbuffer_add_vprintf(buf, fmt, ap);
	va_end(ap);

	return (res);
}



int
evbuffer_remove(struct evbuffer *buf, void *data, size_t datlen)
{
	size_t nread = datlen;
	if (nread >= buf->off)
		nread = buf->off;

	memcpy(data, buf->buffer, nread);
	evbuffer_drain(buf, nread);
	
	return (nread);
}






char *
evbuffer_readline(struct evbuffer *buffer)
{
	u_char *data = EVBUFFER_DATA(buffer);
	size_t len = EVBUFFER_LENGTH(buffer);
	char *line;
	unsigned int i;

	for (i = 0; i < len; i++) {
		if (data[i] == '\r' || data[i] == '\n')
			break;
	}

	if (i == len)
		return (NULL);

	if ((line = malloc(i + 1)) == NULL) {
		fprintf(stderr, "%s: out of memory\n", __func__);
		evbuffer_drain(buffer, i);
		return (NULL);
	}

	memcpy(line, data, i);
	line[i] = '\0';

	



	if ( i < len - 1 ) {
		char fch = data[i], sch = data[i+1];

		
		if ( (sch == '\r' || sch == '\n') && sch != fch )
			i += 1;
	}

	evbuffer_drain(buffer, i + 1);

	return (line);
}



static void
evbuffer_align(struct evbuffer *buf)
{
	memmove(buf->orig_buffer, buf->buffer, buf->off);
	buf->buffer = buf->orig_buffer;
	buf->misalign = 0;
}



int
evbuffer_expand(struct evbuffer *buf, size_t datlen)
{
	size_t need = buf->misalign + buf->off + datlen;

	
	if (buf->totallen >= need)
		return (0);

	



	if (buf->misalign >= datlen) {
		evbuffer_align(buf);
	} else {
		void *newbuf;
		size_t length = buf->totallen;

		if (length < 256)
			length = 256;
		while (length < need)
			length <<= 1;

		if (buf->orig_buffer != buf->buffer)
			evbuffer_align(buf);
		if ((newbuf = realloc(buf->buffer, length)) == NULL)
			return (-1);

		buf->orig_buffer = buf->buffer = newbuf;
		buf->totallen = length;
	}

	return (0);
}

int
evbuffer_add(struct evbuffer *buf, const void *data, size_t datlen)
{
	size_t need = buf->misalign + buf->off + datlen;
	size_t oldoff = buf->off;

	if (buf->totallen < need) {
		if (evbuffer_expand(buf, datlen) == -1)
			return (-1);
	}

	memcpy(buf->buffer + buf->off, data, datlen);
	buf->off += datlen;

	if (datlen && buf->cb != NULL)
		(*buf->cb)(buf, oldoff, buf->off, buf->cbarg);

	return (0);
}

void
evbuffer_drain(struct evbuffer *buf, size_t len)
{
	size_t oldoff = buf->off;

	if (len >= buf->off) {
		buf->off = 0;
		buf->buffer = buf->orig_buffer;
		buf->misalign = 0;
		goto done;
	}

	buf->buffer += len;
	buf->misalign += len;

	buf->off -= len;

 done:
	
	if (buf->off != oldoff && buf->cb != NULL)
		(*buf->cb)(buf, oldoff, buf->off, buf->cbarg);

}





#define EVBUFFER_MAX_READ	4096

int
evbuffer_read(struct evbuffer *buf, int fd, int howmuch)
{
	u_char *p;
	size_t oldoff = buf->off;
	int n = EVBUFFER_MAX_READ;

#if defined(FIONREAD)
#ifdef WIN32
	long lng = n;
	if (ioctlsocket(fd, FIONREAD, &lng) == -1 || (n=lng) == 0) {
#else
	if (ioctl(fd, FIONREAD, &n) == -1 || n == 0) {
#endif
		n = EVBUFFER_MAX_READ;
	} else if (n > EVBUFFER_MAX_READ && n > howmuch) {
		






		if (n > buf->totallen << 2)
			n = buf->totallen << 2;
		if (n < EVBUFFER_MAX_READ)
			n = EVBUFFER_MAX_READ;
	}
#endif	
	if (howmuch < 0 || howmuch > n)
		howmuch = n;

	
	if (evbuffer_expand(buf, howmuch) == -1)
		return (-1);

	
	p = buf->buffer + buf->off;

#ifndef WIN32
	n = read(fd, p, howmuch);
#else
	n = recv(fd, p, howmuch, 0);
#endif
	if (n == -1)
		return (-1);
	if (n == 0)
		return (0);

	buf->off += n;

	
	if (buf->off != oldoff && buf->cb != NULL)
		(*buf->cb)(buf, oldoff, buf->off, buf->cbarg);

	return (n);
}

int
evbuffer_write(struct evbuffer *buffer, int fd)
{
	int n;

#ifndef WIN32
	n = write(fd, buffer->buffer, buffer->off);
#else
	n = send(fd, buffer->buffer, buffer->off, 0);
#endif
	if (n == -1)
		return (-1);
	if (n == 0)
		return (0);
	evbuffer_drain(buffer, n);

	return (n);
}

u_char *
evbuffer_find(struct evbuffer *buffer, const u_char *what, size_t len)
{
	u_char *search = buffer->buffer, *end = search + buffer->off;
	u_char *p;

	while (search < end &&
	    (p = memchr(search, *what, end - search)) != NULL) {
		if (p + len > end)
			break;
		if (memcmp(p, what, len) == 0)
			return (p);
		search = p + 1;
	}

	return (NULL);
}

void evbuffer_setcb(struct evbuffer *buffer,
    void (*cb)(struct evbuffer *, size_t, size_t, void *),
    void *cbarg)
{
	buffer->cb = cb;
	buffer->cbarg = cbarg;
}
