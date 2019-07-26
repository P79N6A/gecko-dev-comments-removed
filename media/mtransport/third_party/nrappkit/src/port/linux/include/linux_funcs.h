






































#ifndef _linux_funcs_h
#define _linux_funcs_h

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

#define ETHERTYPE_VLAN 0x8100

#define STDIO_BYTES_BUFFERED(fp) (fp->_IO_read_end - fp->_IO_read_ptr)

size_t strlcat(char *dst, const char *src, size_t siz);
#ifndef strlcpy
#define strlcpy(a,b,c) \
	(strncpy((a),(b),(c)), \
	 ((c)<= 0 ? 0 : ((a)[(c)-1]='\0')), \
	 strlen((b)))
#endif

#endif

