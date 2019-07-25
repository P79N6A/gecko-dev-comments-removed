




























#define ANDROID_CHANGES 1
#define MOZILLA_NECKO_EXCLUDE_CODE 1

#include <sys/cdefs.h>
#ifndef lint
#ifdef notdef
static const char rcsid[] = "Id: ns_ttl.c,v 1.1.206.1 2004/03/09 08:33:45 marka Exp";
#else
__RCSID("$NetBSD: ns_ttl.c,v 1.2 2004/05/20 20:35:05 christos Exp $");
#endif
#endif



#include "arpa_nameser.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#ifdef SPRINTF_CHAR
# define SPRINTF(x) strlen(sprintf/**/x)
#else
# define SPRINTF(x) ((size_t)sprintf x)
#endif



static int	fmt1(int t, char s, char **buf, size_t *buflen);



#define T(x) do { if ((x) < 0) return (-1); } while(0)



int
ns_format_ttl(u_long src, char *dst, size_t dstlen) {
	char *odst = dst;
	int secs, mins, hours, days, weeks, x;
	char *p;

	secs = src % 60;   src /= 60;
	mins = src % 60;   src /= 60;
	hours = src % 24;  src /= 24;
	days = src % 7;    src /= 7;
	weeks = src;       src = 0;

	x = 0;
	if (weeks) {
		T(fmt1(weeks, 'W', &dst, &dstlen));
		x++;
	}
	if (days) {
		T(fmt1(days, 'D', &dst, &dstlen));
		x++;
	}
	if (hours) {
		T(fmt1(hours, 'H', &dst, &dstlen));
		x++;
	}
	if (mins) {
		T(fmt1(mins, 'M', &dst, &dstlen));
		x++;
	}
	if (secs || !(weeks || days || hours || mins)) {
		T(fmt1(secs, 'S', &dst, &dstlen));
		x++;
	}

	if (x > 1) {
		int ch;

		for (p = odst; (ch = *p) != '\0'; p++)
			if (isascii(ch) && isupper(ch))
				*p = tolower(ch);
	}

	return (dst - odst);
}

#ifndef MOZILLA_NECKO_EXCLUDE_CODE
#ifndef _LIBC
int
ns_parse_ttl(const char *src, u_long *dst) {
	u_long ttl, tmp;
	int ch, digits, dirty;

	ttl = 0;
	tmp = 0;
	digits = 0;
	dirty = 0;
	while ((ch = *src++) != '\0') {
		if (!isascii(ch) || !isprint(ch))
			goto einval;
		if (isdigit(ch)) {
			tmp *= 10;
			tmp += (ch - '0');
			digits++;
			continue;
		}
		if (digits == 0)
			goto einval;
		if (islower(ch))
			ch = toupper(ch);
		switch (ch) {
		case 'W':  tmp *= 7;	
		case 'D':  tmp *= 24;	
		case 'H':  tmp *= 60;	
		case 'M':  tmp *= 60;	
		case 'S':  break;
		default:   goto einval;
		}
		ttl += tmp;
		tmp = 0;
		digits = 0;
		dirty = 1;
	}
	if (digits > 0) {
		if (dirty)
			goto einval;
		else
			ttl += tmp;
	}
	*dst = ttl;
	return (0);

 einval:
	errno = EINVAL;
	return (-1);
}
#endif
#endif



static int
fmt1(int t, char s, char **buf, size_t *buflen) {
	char tmp[50];
	size_t len;

	len = SPRINTF((tmp, "%d%c", t, s));
	if (len + 1 > *buflen)
		return (-1);
	strcpy(*buf, tmp);
	*buf += len;
	*buflen -= len;
	return (0);
}
