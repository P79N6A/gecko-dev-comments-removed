




































#ifndef nsMathUtils_h__
#define nsMathUtils_h__

#define _USE_MATH_DEFINES

#include "nscore.h"
#include <math.h>
#include <float.h>








#ifndef M_E
# define M_E            2.7182818284590452354   /* e */
# define M_LOG2E        1.4426950408889634074   /* log_2 e */
# define M_LOG10E       0.43429448190325182765  /* log_10 e */
# define M_LN2          0.69314718055994530942  /* log_e 2 */
# define M_LN10         2.30258509299404568402  /* log_e 10 */
# define M_PI           3.14159265358979323846  /* pi */
# define M_PI_2         1.57079632679489661923  /* pi/2 */
# define M_PI_4         0.78539816339744830962  /* pi/4 */
# define M_1_PI         0.31830988618379067154  /* 1/pi */
# define M_2_PI         0.63661977236758134308  /* 2/pi */
# define M_2_SQRTPI     1.12837916709551257390  /* 2/sqrt(pi) */
# define M_SQRT2        1.41421356237309504880  /* sqrt(2) */
# define M_SQRT1_2      0.70710678118654752440  /* 1/sqrt(2) */
#endif




inline NS_HIDDEN_(double) NS_round(double x)
{
    return x >= 0.0 ? floor(x + 0.5) : ceil(x - 0.5);
}
inline NS_HIDDEN_(float) NS_roundf(float x)
{
    return x >= 0.0f ? floorf(x + 0.5f) : ceilf(x - 0.5f);
}
inline NS_HIDDEN_(PRInt32) NS_lround(double x)
{
    return x >= 0.0 ? PRInt32(x + 0.5) : PRInt32(x - 0.5);
}




#if defined(XP_WIN32) && defined(_M_IX86) && !defined(__GNUC__)
inline NS_HIDDEN_(PRInt32) NS_lroundup30(float x)
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

inline NS_HIDDEN_(PRInt32) NS_lroundf(float x)
{
    return x >= 0.0f ? PRInt32(x + 0.5f) : PRInt32(x - 0.5f);
}




inline NS_HIDDEN_(double) NS_ceil(double x)
{
    return ceil(x);
}
inline NS_HIDDEN_(float) NS_ceilf(float x)
{
    return ceilf(x);
}




inline NS_HIDDEN_(double) NS_floor(double x)
{
    return floor(x);
}
inline NS_HIDDEN_(float) NS_floorf(float x)
{
    return floorf(x);
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

#endif
