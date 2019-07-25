


















#ifndef _LIBS_CUTILS_UIO_H
#define _LIBS_CUTILS_UIO_H

#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#else

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

struct iovec {
    const void*  iov_base;
    size_t       iov_len;
};

extern int  readv( int  fd, struct iovec*  vecs, int  count );
extern int  writev( int  fd, const struct iovec*  vecs, int  count );

#ifdef __cplusplus
}
#endif

#endif

#endif

