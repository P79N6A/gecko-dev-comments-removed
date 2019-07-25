




























#define ANDROID_CHANGES 1
#define MOZILLA_NECKO_EXCLUDE_CODE 1

#include <sys/cdefs.h>
#ifndef lint
#ifdef notdef
static const char rcsid[] = "Id: ns_netint.c,v 1.1.206.1 2004/03/09 08:33:44 marka Exp";
#else
__RCSID("$NetBSD: ns_netint.c,v 1.2 2004/05/20 20:19:00 christos Exp $");
#endif
#endif



#include "arpa_nameser.h"



u_int16_t
ns_get16(const u_char *src) {
	u_int dst;

	NS_GET16(dst, src);
	return (dst);
}

u_int32_t
ns_get32(const u_char *src) {
	u_long dst;

	NS_GET32(dst, src);
	return (dst);
}

void
ns_put16(u_int16_t src, u_char *dst) {
	NS_PUT16(src, dst);
}

void
ns_put32(u_int32_t src, u_char *dst) {
	NS_PUT32(src, dst);
}
