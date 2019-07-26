





#ifndef jslibmath_h
#define jslibmath_h

#include "mozilla/FloatingPoint.h"

#include <math.h>

#include "jsnum.h"






#ifdef __GNUC__
#define js_copysign __builtin_copysign
#elif defined _WIN32
#define js_copysign _copysign
#else
#define js_copysign copysign
#endif


static inline double
js_fmod(double d, double d2)
{
#ifdef XP_WIN
    



    if ((mozilla::IsFinite(d) && mozilla::IsInfinite(d2)) ||
        (d == 0 && mozilla::IsFinite(d2))) {
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
        if (a == 0 || mozilla::IsNaN(a)
#ifdef XP_WIN
            || mozilla::IsNaN(b) 
#endif
        )
            return JS::GenericNaN();

        if (mozilla::IsNegative(a) != mozilla::IsNegative(b))
            return mozilla::NegativeInfinity();
        return mozilla::PositiveInfinity();
    }

    return a / b;
}

inline double
NumberMod(double a, double b) {
    if (b == 0) 
        return JS::GenericNaN();
    return js_fmod(a, b);
}

}

#endif 
