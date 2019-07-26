#ifndef PRIVATE_H

#define PRIVATE_H


















#ifndef lint
#ifndef NOID
static char	privatehid[] = "@(#)private.h	8.6";
#endif 
#endif 

#define GRANDPARENTED	"Local time zone must be set--see zic manual page"






#ifndef HAVE_ADJTIME
#define HAVE_ADJTIME		1
#endif 

#ifndef HAVE_GETTEXT
#define HAVE_GETTEXT		0
#endif 

#ifndef HAVE_INCOMPATIBLE_CTIME_R
#define HAVE_INCOMPATIBLE_CTIME_R	0
#endif 

#ifndef HAVE_SETTIMEOFDAY
#define HAVE_SETTIMEOFDAY	3
#endif 

#ifndef HAVE_SYMLINK
#define HAVE_SYMLINK		1
#endif 

#ifndef HAVE_SYS_STAT_H
#define HAVE_SYS_STAT_H		1
#endif 

#ifndef HAVE_SYS_WAIT_H
#define HAVE_SYS_WAIT_H		1
#endif 

#ifndef HAVE_UNISTD_H
#define HAVE_UNISTD_H		1
#endif 

#ifndef HAVE_UTMPX_H
#define HAVE_UTMPX_H		0
#endif 

#ifndef LOCALE_HOME
#define LOCALE_HOME		"/usr/lib/locale"
#endif 

#if HAVE_INCOMPATIBLE_CTIME_R
#define asctime_r _incompatible_asctime_r
#define ctime_r _incompatible_ctime_r
#endif 





#include "sys/types.h"	
#include "stdio.h"
#include "errno.h"
#include "string.h"
#include "limits.h"	
#include "time.h"
#include "stdlib.h"

#if HAVE_GETTEXT
#include "libintl.h"
#endif 

#if HAVE_SYS_WAIT_H
#include <sys/wait.h>	
#endif 

#ifndef WIFEXITED
#define WIFEXITED(status)	(((status) & 0xff) == 0)
#endif 
#ifndef WEXITSTATUS
#define WEXITSTATUS(status)	(((status) >> 8) & 0xff)
#endif 

#if HAVE_UNISTD_H
#include "unistd.h"	
#endif 

#ifndef F_OK
#define F_OK	0
#endif 
#ifndef R_OK
#define R_OK	4
#endif 


#define is_digit(c) ((unsigned)(c) - '0' <= 9)







#ifndef HAVE_STDINT_H
#define HAVE_STDINT_H \
	(199901 <= __STDC_VERSION__ || \
	2 < (__GLIBC__ + (0 < __GLIBC_MINOR__)))
#endif 

#if HAVE_STDINT_H
#include "stdint.h"
#endif 

#ifndef INT_FAST64_MAX

#if defined LLONG_MAX || defined __LONG_LONG_MAX__
typedef long long	int_fast64_t;
#else 
#if (LONG_MAX >> 31) < 0xffffffff
Please use a compiler that supports a 64-bit integer type (or wider);
you may need to compile with "-DHAVE_STDINT_H".
#endif 
typedef long		int_fast64_t;
#endif 
#endif 

#ifndef INT32_MAX
#define INT32_MAX 0x7fffffff
#endif 
#ifndef INT32_MIN
#define INT32_MIN (-1 - INT32_MAX)
#endif 











#ifndef asctime_r
extern char *	asctime_r(struct tm const *, char *);
#endif





char *		icalloc(int nelem, int elsize);
char *		icatalloc(char * old, const char * new);
char *		icpyalloc(const char * string);
char *		imalloc(int n);
void *		irealloc(void * pointer, int size);
void		icfree(char * pointer);
void		ifree(char * pointer);
const char *	scheck(const char * string, const char * format);





#ifndef TRUE
#define TRUE	1
#endif 

#ifndef FALSE
#define FALSE	0
#endif 

#ifndef TYPE_BIT
#define TYPE_BIT(type)	(sizeof (type) * CHAR_BIT)
#endif 

#ifndef TYPE_SIGNED
#define TYPE_SIGNED(type) (((type) -1) < 0)
#endif 






#ifndef TYPE_INTEGRAL
#define TYPE_INTEGRAL(type) (((type) 0.5) != 0.5)
#endif 

#ifndef INT_STRLEN_MAXIMUM






#define INT_STRLEN_MAXIMUM(type) \
	((TYPE_BIT(type) - TYPE_SIGNED(type)) * 302 / 1000 + \
	1 + TYPE_SIGNED(type))
#endif 





#ifndef GNUC_or_lint
#ifdef lint
#define GNUC_or_lint
#endif 
#ifndef lint
#ifdef __GNUC__
#define GNUC_or_lint
#endif 
#endif 
#endif 

#ifndef INITIALIZE
#ifdef GNUC_or_lint
#define INITIALIZE(x)	((x) = 0)
#endif 
#ifndef GNUC_or_lint
#define INITIALIZE(x)
#endif 
#endif 







#ifndef _
#if HAVE_GETTEXT
#define _(msgid) gettext(msgid)
#else 
#define _(msgid) msgid
#endif 
#endif 

#ifndef TZ_DOMAIN
#define TZ_DOMAIN "tz"
#endif 

#if HAVE_INCOMPATIBLE_CTIME_R
#undef asctime_r
#undef ctime_r
char *asctime_r(struct tm const *, char *);
char *ctime_r(time_t const *, char *);
#endif 

#ifndef YEARSPERREPEAT
#define YEARSPERREPEAT		400	/* years before a Gregorian repeat */
#endif 





#ifndef AVGSECSPERYEAR
#define AVGSECSPERYEAR		31556952L
#endif 

#ifndef SECSPERREPEAT
#define SECSPERREPEAT		((int_fast64_t) YEARSPERREPEAT * (int_fast64_t) AVGSECSPERYEAR)
#endif 

#ifndef SECSPERREPEAT_BITS
#define SECSPERREPEAT_BITS	34	/* ceil(log2(SECSPERREPEAT)) */
#endif 





#endif 
