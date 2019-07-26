






































#ifndef _CPR_LINUX_STRING_H_
#define _CPR_LINUX_STRING_H_

#include <string.h>
#include <ctype.h>

#define cpr_strtok(a,b,c) strtok_r(a,b,c)















char * 
cpr_strdup(const char *str);



















char *
sstrncat(char *s1, const char *s2, unsigned long max);




















unsigned long
sstrncpy(char *dst, const char *src, unsigned long max);

#endif
