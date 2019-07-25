







































#ifndef _LIBMATH_H
#define _LIBMATH_H

#include <math.h>
#include "jsnum.h"






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

#if defined(_M_X64) && defined(_MSC_VER) && _MSC_VER <= 1500

extern "C" double js_myfmod(double x, double y);
#define fmod js_myfmod
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

namespace js {

inline double
NumberDiv(double a, double b) {
    if (b == 0) {
        if (a == 0 || JSDOUBLE_IS_NaN(a) 
#ifdef XP_WIN
            || JSDOUBLE_IS_NaN(a) 
#endif
        )
            return js_NaN;    
        else if (JSDOUBLE_IS_NEG(a) != JSDOUBLE_IS_NEG(b))
            return js_NegativeInfinity;
        else
            return js_PositiveInfinity; 
    }

    return a / b;
}

}

#endif 

