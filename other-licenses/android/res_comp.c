

















































































#define ANDROID_CHANGES 1
#define MOZILLA_NECKO_EXCLUDE_CODE 1

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#ifdef notdef
static const char sccsid[] = "@(#)res_comp.c	8.1 (Berkeley) 6/4/93";
static const char rcsid[] = "Id: res_comp.c,v 1.1.2.1.4.1 2004/03/09 08:33:54 marka Exp";
#else
__RCSID("$NetBSD: res_comp.c,v 1.6 2004/05/22 23:47:09 christos Exp $");
#endif
#endif 

#include <sys/types.h>
#include <sys/param.h>
#include <netinet/in.h>
#include "arpa_nameser.h"
#include <ctype.h>
#ifdef ANDROID_CHANGES
#include "resolv_private.h"
#else
#include <resolv.h>
#endif
#include <stdio.h>
#include <string.h>
#include <unistd.h>








int
dn_expand(const u_char *msg, const u_char *eom, const u_char *src,
	  char *dst, int dstsiz)
{
	int n = ns_name_uncompress(msg, eom, src, dst, (size_t)dstsiz);

	if (n > 0 && dst[0] == '.')
		dst[0] = '\0';
	return (n);
}






int
dn_comp(const char *src, u_char *dst, int dstsiz,
	u_char **dnptrs, u_char **lastdnptr)
{
	return (ns_name_compress(src, dst, (size_t)dstsiz,
				 (const u_char **)dnptrs,
				 (const u_char **)lastdnptr));
}




int
dn_skipname(const u_char *ptr, const u_char *eom) {
	const u_char *saveptr = ptr;

	if (ns_name_skip(&ptr, eom) == -1)
		return (-1);
	return (ptr - saveptr);
}

















#define PERIOD 0x2e
#define	hyphenchar(c) ((c) == 0x2d)
#define bslashchar(c) ((c) == 0x5c)
#define periodchar(c) ((c) == PERIOD)
#define asterchar(c) ((c) == 0x2a)
#define alphachar(c) (((c) >= 0x41 && (c) <= 0x5a) \
		   || ((c) >= 0x61 && (c) <= 0x7a))
#define digitchar(c) ((c) >= 0x30 && (c) <= 0x39)
#define underscorechar(c)  ((c) == 0x5f)

#define borderchar(c) (alphachar(c) || digitchar(c))
#define middlechar(c) (borderchar(c) || hyphenchar(c) || underscorechar(c))
#define	domainchar(c) ((c) > 0x20 && (c) < 0x7f)

int
res_hnok(const char *dn) {
	int pch = PERIOD, ch = *dn++;

	while (ch != '\0') {
		int nch = *dn++;

		if (periodchar(ch)) {
			;
		} else if (periodchar(pch)) {
			if (!borderchar(ch))
				return (0);
		} else if (periodchar(nch) || nch == '\0') {
			if (!borderchar(ch))
				return (0);
		} else {
			if (!middlechar(ch))
				return (0);
		}
		pch = ch, ch = nch;
	}
	return (1);
}

#ifndef MOZILLA_NECKO_EXCLUDE_CODE




int
res_ownok(const char *dn) {
	if (asterchar(dn[0])) {
		if (periodchar(dn[1]))
			return (res_hnok(dn+2));
		if (dn[1] == '\0')
			return (1);
	}
	return (res_hnok(dn));
}





int
res_mailok(const char *dn) {
	int ch, escaped = 0;

	
	if (*dn == '\0')
		return (1);

	
	while ((ch = *dn++) != '\0') {
		if (!domainchar(ch))
			return (0);
		if (!escaped && periodchar(ch))
			break;
		if (escaped)
			escaped = 0;
		else if (bslashchar(ch))
			escaped = 1;
	}
	if (periodchar(ch))
		return (res_hnok(dn));
	return (0);
}





int
res_dnok(const char *dn) {
	int ch;

	while ((ch = *dn++) != '\0')
		if (!domainchar(ch))
			return (0);
	return (1);
}
#endif

#ifdef BIND_4_COMPAT








void __putlong(u_int32_t src, u_char *dst) { ns_put32(src, dst); }
void __putshort(u_int16_t src, u_char *dst) { ns_put16(src, dst); }
#ifndef __ultrix__
u_int32_t _getlong(const u_char *src) { return (ns_get32(src)); }
u_int16_t _getshort(const u_char *src) { return (ns_get16(src)); }
#endif 
#endif 
