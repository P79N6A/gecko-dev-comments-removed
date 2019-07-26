








































#ifndef _LIBMATH_H
#define _LIBMATH_H

#include "mozilla/FloatingPoint.h"

#include <math.h>
#include "jsnum.h"






#if __GNUC__ >= 4 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
#define js_copysign __builtin_copysign
#elif defined _WIN32
#define js_copysign _copysign
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
    



    if ((MOZ_DOUBLE_IS_FINITE(d) && MOZ_DOUBLE_IS_INFINITE(d2)) ||
        (d == 0 && MOZ_DOUBLE_IS_FINITE(d2))) {
        return d;
    }
#endif
    return fmod(d, d2);
}

namespace js {

inline double
NumberDiv(double a, double b)
{
    if (b == 0) {
        if (a == 0 || MOZ_DOUBLE_IS_NaN(a)
#ifdef XP_WIN
            || MOZ_DOUBLE_IS_NaN(b) 
#endif
        )
            return js_NaN;    

        if (MOZ_DOUBLE_IS_NEGATIVE(a) != MOZ_DOUBLE_IS_NEGATIVE(b))
            return js_NegativeInfinity;
        return js_PositiveInfinity; 
    }

    return a / b;
}

inline double
NumberMod(double a, double b) {
    if (b == 0) 
        return js_NaN;
    return js_fmod(a, b);
}

}

#endif 

