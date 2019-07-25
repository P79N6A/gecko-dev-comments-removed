







































#ifndef _LIBMATH_H
#define _LIBMATH_H

#include <math.h>
#ifdef XP_WIN
# include "jsnum.h"
#endif






#if __GNUC__ >= 4 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
#define js_copysign __builtin_copysign
#elif defined _WIN32
#if _MSC_VER < 1400

#define js_copysign js_copysign
extern double js_copysign(double, double);
#else
#define js_copysign _copysign
#endif
#else
#define js_copysign copysign
#endif


static inline double
js_fmod(double d, double d2)
{
#ifdef XP_WIN
    



    if ((JSDOUBLE_IS_FINITE(d) && JSDOUBLE_IS_INFINITE(d2)) ||
        (d == 0 && JSDOUBLE_IS_FINITE(d2))) {
        return d;
    }
#endif
    return fmod(d, d2);
}

#endif 

