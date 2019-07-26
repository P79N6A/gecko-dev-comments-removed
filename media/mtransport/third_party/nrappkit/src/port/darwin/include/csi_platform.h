






































#ifndef _platform_h
#define _platform_h

#include <unistd.h>

#define STDIO_BYTES_BUFFERED(fp) (fp->_r)

#ifdef NR_SOCKET_IS_VOID_PTR
typedef void* NR_SOCKET;
#else
typedef int NR_SOCKET;
#define NR_SOCKET_READ(sock,buf,count)   read((sock),(buf),(count))
#define NR_SOCKET_WRITE(sock,buf,count)  write((sock),(buf),(count))
#define NR_SOCKET_CLOSE(sock)            close(sock)
#endif

#endif

