






































#ifndef _CPR_WIN_STRING_H_
#define _CPR_WIN_STRING_H_

#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#define bcmp        memcmp
#define bzero(s, n) memset((s), 0, (n))
#define bcopy(src, dst, len) memcpy(dst, src, len); /* XXX Wrong */

#define cpr_strtok(a,b,c) strtok_s(a,b,c)

unsigned long sstrncpy (char *dst, const char *src, unsigned long max);
char *sstrncat (char *s1, const char *s2, unsigned long max);
void SafeStrCpy (char *dest, const char *src, int maxlen);
int strncasecmp (const char *s1, const char *s2, size_t length);
int strcasecmp (const char *s1, const char *s2);
char *strdup (const char *input_str);
char *strcasestr (const char *s1, const char *s2);
void upper_string (char *str);

#endif
