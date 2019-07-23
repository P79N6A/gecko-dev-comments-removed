












































#ifndef _LIBMATH_H
#define _LIBMATH_H

#include <math.h>
#include "jsconfig.h"










#ifndef JS_USE_FDLIBM_MATH
#define JS_USE_FDLIBM_MATH 0
#endif

#if !JS_USE_FDLIBM_MATH





#define fd_acos acos
#define fd_asin asin
#define fd_atan atan
#define fd_atan2 atan2
#define fd_ceil ceil


#if __GNUC__ >= 4 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
#define fd_copysign __builtin_copysign
#elif defined WINCE
#define fd_copysign _copysign
#elif defined _WIN32
#if _MSC_VER < 1400

#define fd_copysign js_copysign
extern double js_copysign(double, double);
#else
#define fd_copysign _copysign
#endif
#else
#define fd_copysign copysign
#endif

#define fd_cos cos
#define fd_exp exp
#define fd_fabs fabs
#define fd_floor floor
#define fd_fmod fmod
#define fd_log log
#define fd_pow pow
#define fd_sin sin
#define fd_sqrt sqrt
#define fd_tan tan

#else





#undef __P
#ifdef __STDC__
#define __P(p)  p
#else
#define __P(p)  ()
#endif

#if (defined _WIN32 && !defined WINCE) || defined SUNOS4

#define fd_acos acos
#define fd_asin asin
#define fd_atan atan
#define fd_cos cos
#define fd_sin sin
#define fd_tan tan
#define fd_exp exp
#define fd_log log
#define fd_sqrt sqrt
#define fd_ceil ceil
#define fd_fabs fabs
#define fd_floor floor
#define fd_fmod fmod

extern double fd_atan2 __P((double, double));
extern double fd_copysign __P((double, double));
extern double fd_pow __P((double, double));

#elif defined IRIX

#define fd_acos acos
#define fd_asin asin
#define fd_atan atan
#define fd_exp exp
#define fd_log log
#define fd_log10 log10
#define fd_sqrt sqrt
#define fd_fabs fabs
#define fd_floor floor
#define fd_fmod fmod

extern double fd_cos __P((double));
extern double fd_sin __P((double));
extern double fd_tan __P((double));
extern double fd_atan2 __P((double, double));
extern double fd_pow __P((double, double));
extern double fd_ceil __P((double));
extern double fd_copysign __P((double, double));

#elif defined SOLARIS

#define fd_atan atan
#define fd_cos cos
#define fd_sin sin
#define fd_tan tan
#define fd_exp exp
#define fd_sqrt sqrt
#define fd_ceil ceil
#define fd_fabs fabs
#define fd_floor floor
#define fd_fmod fmod

extern double fd_acos __P((double));
extern double fd_asin __P((double));
extern double fd_log __P((double));
extern double fd_atan2 __P((double, double));
extern double fd_pow __P((double, double));
extern double fd_copysign __P((double, double));

#elif defined HPUX

#define fd_cos cos
#define fd_sin sin
#define fd_exp exp
#define fd_sqrt sqrt
#define fd_fabs fabs
#define fd_floor floor
#define fd_fmod fmod

extern double fd_ceil __P((double));
extern double fd_acos __P((double));
extern double fd_log __P((double));
extern double fd_atan2 __P((double, double));
extern double fd_tan __P((double));
extern double fd_pow __P((double, double));
extern double fd_asin __P((double));
extern double fd_atan __P((double));
extern double fd_copysign __P((double, double));

#elif defined(OSF1)

#define fd_acos acos
#define fd_asin asin
#define fd_atan atan
#define fd_copysign copysign
#define fd_cos cos
#define fd_exp exp
#define fd_fabs fabs
#define fd_fmod fmod
#define fd_sin sin
#define fd_sqrt sqrt
#define fd_tan tan

extern double fd_atan2 __P((double, double));
extern double fd_ceil __P((double));
extern double fd_floor __P((double));
extern double fd_log __P((double));
extern double fd_pow __P((double, double));

#elif defined(AIX)

#define fd_acos acos
#define fd_asin asin
#define fd_atan2 atan2
#define fd_copysign copysign
#define fd_cos cos
#define fd_exp exp
#define fd_fabs fabs
#define fd_floor floor
#define fd_fmod fmod
#define fd_log log
#define fd_sin sin
#define fd_sqrt sqrt

extern double fd_atan __P((double));
extern double fd_ceil __P((double));
extern double fd_pow __P((double,double));
extern double fd_tan __P((double));

#else 

extern double fd_acos __P((double));
extern double fd_asin __P((double));
extern double fd_atan __P((double));
extern double fd_cos __P((double));
extern double fd_sin __P((double));
extern double fd_tan __P((double));

extern double fd_exp __P((double));
extern double fd_log __P((double));
extern double fd_sqrt __P((double));

extern double fd_ceil __P((double));
extern double fd_fabs __P((double));
extern double fd_floor __P((double));
extern double fd_fmod __P((double, double));

extern double fd_atan2 __P((double, double));
extern double fd_pow __P((double, double));
extern double fd_copysign __P((double, double));

#endif

#endif 

#endif 

