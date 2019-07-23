







































#ifndef _LIBMATH_H
#define _LIBMATH_H

#include <math.h>
#include "jsversion.h"






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

#endif 

