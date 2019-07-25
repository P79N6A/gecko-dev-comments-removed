




























#define ANDROID_CHANGES 1
#define MOZILLA_NECKO_EXCLUDE_CODE 1

#include <sys/cdefs.h>
#ifndef lint
#ifdef notdef
static const char rcsid[] = "Id: ns_samedomain.c,v 1.1.2.2.4.2 2004/03/16 12:34:17 marka Exp";
#else
__RCSID("$NetBSD: ns_samedomain.c,v 1.2 2004/05/20 20:35:05 christos Exp $");
#endif
#endif

#include <sys/types.h>
#include "arpa_nameser.h"
#include <errno.h>
#include <string.h>

#ifndef MOZILLA_NECKO_EXCLUDE_CODE
#ifndef _LIBC


















int
ns_samedomain(const char *a, const char *b) {
	size_t la, lb;
	int diff, i, escaped;
	const char *cp;

	la = strlen(a);
	lb = strlen(b);

	
	if (la != 0U && a[la - 1] == '.') {
		escaped = 0;
		
		for (i = la - 2; i >= 0; i--)
			if (a[i] == '\\') {
				if (escaped)
					escaped = 0;
				else
					escaped = 1;
			} else
				break;
		if (!escaped)
			la--;
	}

	
	if (lb != 0U && b[lb - 1] == '.') {
		escaped = 0;
		
		for (i = lb - 2; i >= 0; i--)
			if (b[i] == '\\') {
				if (escaped)
					escaped = 0;
				else
					escaped = 1;
			} else
				break;
		if (!escaped)
			lb--;
	}

	
	if (lb == 0U)
		return (1);

	
	if (lb > la)
		return (0);

	
	if (lb == la)
		return (strncasecmp(a, b, lb) == 0);

	

	diff = la - lb;

	




	if (diff < 2)
		return (0);

	




	if (a[diff - 1] != '.')
		return (0);

	



	escaped = 0;
	for (i = diff - 2; i >= 0; i--)
		if (a[i] == '\\') {
			if (escaped)
				escaped = 0;
			else
				escaped = 1;
		} else
			break;
	if (escaped)
		return (0);

	
	cp = a + diff;
	return (strncasecmp(cp, b, lb) == 0);
}






int
ns_subdomain(const char *a, const char *b) {
	return (ns_samename(a, b) != 1 && ns_samedomain(a, b));
}
#endif
#endif













int
ns_makecanon(const char *src, char *dst, size_t dstsize) {
	size_t n = strlen(src);

	if (n + sizeof "." > dstsize) {			
		errno = EMSGSIZE;
		return (-1);
	}
	strcpy(dst, src);
	while (n >= 1U && dst[n - 1] == '.')		
		if (n >= 2U && dst[n - 2] == '\\' &&	
		    (n < 3U || dst[n - 3] != '\\'))	
			break;
		else
			dst[--n] = '\0';
	dst[n++] = '.';
	dst[n] = '\0';
	return (0);
}











int
ns_samename(const char *a, const char *b) {
	char ta[NS_MAXDNAME], tb[NS_MAXDNAME];

	if (ns_makecanon(a, ta, sizeof ta) < 0 ||
	    ns_makecanon(b, tb, sizeof tb) < 0)
		return (-1);
	if (strcasecmp(ta, tb) == 0)
		return (1);
	else
		return (0);
}
