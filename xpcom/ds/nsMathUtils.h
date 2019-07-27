





#ifndef nsMathUtils_h__
#define nsMathUtils_h__

#define _USE_MATH_DEFINES

#include "nscore.h"
#include <cmath>
#include <float.h>

#ifdef SOLARIS
#include <ieeefp.h>
#endif




inline double
NS_round(double aNum)
{
  return aNum >= 0.0 ? floor(aNum + 0.5) : ceil(aNum - 0.5);
}
inline float
NS_roundf(float aNum)
{
  return aNum >= 0.0f ? floorf(aNum + 0.5f) : ceilf(aNum - 0.5f);
}
inline int32_t
NS_lround(double aNum)
{
  return aNum >= 0.0 ? int32_t(aNum + 0.5) : int32_t(aNum - 0.5);
}




#if defined(XP_WIN32) && defined(_M_IX86) && !defined(__GNUC__) && !defined(__clang__)
inline int32_t NS_lroundup30(float x)
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

inline int32_t
NS_lroundf(float aNum)
{
  return aNum >= 0.0f ? int32_t(aNum + 0.5f) : int32_t(aNum - 0.5f);
}






inline double
NS_hypot(double aNum1, double aNum2)
{
#ifdef __GNUC__
  return __builtin_hypot(aNum1, aNum2);
#elif defined _WIN32
  return _hypot(aNum1, aNum2);
#else
  return hypot(aNum1, aNum2);
#endif
}





inline bool
NS_finite(double aNum)
{
#ifdef WIN32
  
  return !!_finite(aNum);
#elif defined(XP_DARWIN)
  
  
  return std::isfinite(aNum);
#else
  return finite(aNum);
#endif
}







inline double
NS_floorModulo(double aNum1, double aNum2)
{
  return (aNum1 - aNum2 * floor(aNum1 / aNum2));
}

#endif
