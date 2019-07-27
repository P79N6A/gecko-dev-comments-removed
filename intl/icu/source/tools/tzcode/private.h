#ifndef PRIVATE_H

#define PRIVATE_H














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

#ifndef HAVE_LINK
#define HAVE_LINK		1
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

#ifndef HAVE_INTTYPES_H
# define HAVE_INTTYPES_H HAVE_STDINT_H
#endif
#if HAVE_INTTYPES_H
# include <inttypes.h>
#endif

#ifndef INT_FAST64_MAX

#if defined LLONG_MAX || defined __LONG_LONG_MAX__
typedef long long	int_fast64_t;
# ifdef LLONG_MAX
#  define INT_FAST64_MIN LLONG_MIN
#  define INT_FAST64_MAX LLONG_MAX
# else
#  define INT_FAST64_MIN __LONG_LONG_MIN__
#  define INT_FAST64_MAX __LONG_LONG_MAX__
# endif
# define SCNdFAST64 "lld"
#else 
#if (LONG_MAX >> 31) < 0xffffffff
Please use a compiler that supports a 64-bit integer type (or wider);
you may need to compile with "-DHAVE_STDINT_H".
#endif 
typedef long		int_fast64_t;
# define INT_FAST64_MIN LONG_MIN
# define INT_FAST64_MAX LONG_MAX
# define SCNdFAST64 "ld"
#endif 
#endif 

#ifndef INT_FAST32_MAX
# if INT_MAX >> 31 == 0
typedef long int_fast32_t;
# else
typedef int int_fast32_t;
# endif
#endif

#ifndef INTMAX_MAX
# if defined LLONG_MAX || defined __LONG_LONG_MAX__
typedef long long intmax_t;
#  define strtoimax strtoll
#  define PRIdMAX "lld"
#  ifdef LLONG_MAX
#   define INTMAX_MAX LLONG_MAX
#   define INTMAX_MIN LLONG_MIN
#  else
#   define INTMAX_MAX __LONG_LONG_MAX__
#   define INTMAX_MIN __LONG_LONG_MIN__
#  endif
# else
typedef long intmax_t;
#  define strtoimax strtol
#  define PRIdMAX "ld"
#  define INTMAX_MAX LONG_MAX
#  define INTMAX_MIN LONG_MIN
# endif
#endif

#ifndef UINTMAX_MAX
# if defined ULLONG_MAX || defined __LONG_LONG_MAX__
typedef unsigned long long uintmax_t;
#  define PRIuMAX "llu"
# else
typedef unsigned long uintmax_t;
#  define PRIuMAX "lu"
# endif
#endif

#ifndef INT32_MAX
#define INT32_MAX 0x7fffffff
#endif 
#ifndef INT32_MIN
#define INT32_MIN (-1 - INT32_MAX)
#endif 

#ifndef SIZE_MAX
#define SIZE_MAX ((size_t) -1)
#endif

#if 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
# define ATTRIBUTE_CONST __attribute__ ((const))
# define ATTRIBUTE_PURE __attribute__ ((__pure__))
# define ATTRIBUTE_FORMAT(spec) __attribute__ ((__format__ spec))
#else
# define ATTRIBUTE_CONST
# define ATTRIBUTE_PURE
# define ATTRIBUTE_FORMAT(spec)
#endif

#if !defined _Noreturn && __STDC_VERSION__ < 201112
# if 2 < __GNUC__ + (8 <= __GNUC_MINOR__)
#  define _Noreturn __attribute__ ((__noreturn__))
# else
#  define _Noreturn
# endif
#endif

#if __STDC_VERSION__ < 199901 && !defined restrict
# define restrict
#endif











#ifndef asctime_r
extern char *	asctime_r(struct tm const *, char *);
#endif








#ifdef time_tz
static time_t sys_time(time_t *x) { return time(x); }

# undef  ctime
# define ctime tz_ctime
# undef  ctime_r
# define ctime_r tz_ctime_r
# undef  difftime
# define difftime tz_difftime
# undef  gmtime
# define gmtime tz_gmtime
# undef  gmtime_r
# define gmtime_r tz_gmtime_r
# undef  localtime
# define localtime tz_localtime
# undef  localtime_r
# define localtime_r tz_localtime_r
# undef  mktime
# define mktime tz_mktime
# undef  time
# define time tz_time
# undef  time_t
# define time_t tz_time_t

typedef time_tz time_t;

char *ctime(time_t const *);
char *ctime_r(time_t const *, char *);
double difftime(time_t, time_t);
struct tm *gmtime(time_t const *);
struct tm *gmtime_r(time_t const *restrict, struct tm *restrict);
struct tm *localtime(time_t const *);
struct tm *localtime_r(time_t const *restrict, struct tm *restrict);
time_t mktime(struct tm *);

static time_t
time(time_t *p)
{
	time_t r = sys_time(0);
	if (p)
		*p = r;
	return r;
}
#endif





char *		icatalloc(char * old, const char * new);
char *		icpyalloc(const char * string);
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


static time_t const time_t_min =
  (TYPE_SIGNED(time_t)
   ? (time_t) -1 << (CHAR_BIT * sizeof (time_t) - 1)
   : 0);
static time_t const time_t_max =
  (TYPE_SIGNED(time_t)
   ? - (~ 0 < 0) - ((time_t) -1 << (CHAR_BIT * sizeof (time_t) - 1))
   : -1);

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
