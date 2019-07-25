




#ifndef nsMathUtils_h__
#define nsMathUtils_h__

#define _USE_MATH_DEFINES

#include "nscore.h"
#include <cmath>
#include <float.h>

#ifdef SOLARIS
#include <ieeefp.h>
#endif




inline NS_HIDDEN_(double) NS_round(double x)
{
    return x >= 0.0 ? floor(x + 0.5) : ceil(x - 0.5);
}
inline NS_HIDDEN_(float) NS_roundf(float x)
{
    return x >= 0.0f ? floorf(x + 0.5f) : ceilf(x - 0.5f);
}
inline NS_HIDDEN_(int32_t) NS_lround(double x)
{
    return x >= 0.0 ? int32_t(x + 0.5) : int32_t(x - 0.5);
}




#if defined(XP_WIN32) && defined(_M_IX86) && !defined(__GNUC__)
inline NS_HIDDEN_(int32_t) NS_lroundup30(float x)
{
    
    

    
    
    
    
    
    
    
    
    

    
    
    

    static const double round_to_nearest = 0.5f;
    int i;

    __asm {
      fld     x                   ; load fp argument
      fadd    st, st(0)           ; double it
      fadd    round_to_nearest    ; add the rounding factor
      fistp   dword ptr i         ; convert the result to int
    }
    return i >> 1;                
}
#endif 

inline NS_HIDDEN_(int32_t) NS_lroundf(float x)
{
    return x >= 0.0f ? int32_t(x + 0.5f) : int32_t(x - 0.5f);
}






inline NS_HIDDEN_(double) NS_hypot(double x, double y)
{
#if __GNUC__ >= 4
    return __builtin_hypot(x, y);
#elif defined _WIN32
    return _hypot(x, y);
#else
    return hypot(x, y);
#endif
}





inline NS_HIDDEN_(bool) NS_finite(double d)
{
#ifdef WIN32
    
    return !!_finite(d);
#elif defined(XP_DARWIN)
    
    
    return std::isfinite(d);
#else
    return finite(d);
#endif
}







inline NS_HIDDEN_(double) NS_floorModulo(double x, double y)
{
  return (x - y * floor(x / y));
}

#endif
