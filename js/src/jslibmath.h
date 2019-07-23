







































#ifndef _LIBMATH_H
#define _LIBMATH_H

#include <math.h>
#include "jsversion.h"





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

#endif 

