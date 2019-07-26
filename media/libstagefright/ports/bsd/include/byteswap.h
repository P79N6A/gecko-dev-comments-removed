


#ifndef BYTESWAP_H_
#define BYTESWAP_H_

#include <sys/endian.h>

#ifdef __OpenBSD__
#define bswap_16(x)	swap16(x)
#else
#define bswap_16(x)	bswap16(x)
#endif

#endif
