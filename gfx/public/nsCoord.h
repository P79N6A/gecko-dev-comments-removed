




































#ifndef NSCOORD_H
#define NSCOORD_H

#include <math.h>

#include "nsDebug.h"



















inline float NS_IEEEPositiveInfinity() {
  union { PRUint32 mPRUint32; float mFloat; } pun;
  pun.mPRUint32 = 0x7F800000;
  return pun.mFloat;
}
inline PRBool NS_IEEEIsNan(float aF) {
  union { PRUint32 mBits; float mFloat; } pun;
  pun.mFloat = aF;
  return (pun.mBits & 0x7F800000) == 0x7F800000 &&
    (pun.mBits & 0x007FFFFF) != 0;
}

#ifdef NS_COORD_IS_FLOAT
typedef float nscoord;
#define nscoord_MAX NS_IEEEPositiveInfinity()
#else
typedef PRInt32 nscoord;
#define nscoord_MAX nscoord(1 << 30)
#endif

#define nscoord_MIN (-nscoord_MAX)

inline void VERIFY_COORD(nscoord aCoord) {
#ifdef NS_COORD_IS_FLOAT
  NS_ASSERTION(floorf(aCoord) == aCoord,
               "Coords cannot have fractions");
#endif
}

inline nscoord NSCoordMultiply(nscoord aCoord, float aVal) {
  VERIFY_COORD(aCoord);
#ifdef NS_COORD_IS_FLOAT
  return floorf(aCoord*aVal);
#else
  return (PRInt32)(aCoord*aVal);
#endif
}

inline nscoord NSCoordMultiply(nscoord aCoord, PRInt32 aVal) {
  VERIFY_COORD(aCoord);
  return aCoord*aVal;
}

inline nscoord NSCoordDivide(nscoord aCoord, float aVal) {
  VERIFY_COORD(aCoord);
#ifdef NS_COORD_IS_FLOAT
  return floorf(aCoord/aVal);
#else
  return (PRInt32)(aCoord/aVal);
#endif
}

inline nscoord NSCoordDivide(nscoord aCoord, PRInt32 aVal) {
  VERIFY_COORD(aCoord);
#ifdef NS_COORD_IS_FLOAT
  return floorf(aCoord/aVal);
#else
  return aCoord/aVal;
#endif
}










inline nscoord
NSCoordSaturatingAdd(nscoord a, nscoord b)
{
  VERIFY_COORD(a);
  VERIFY_COORD(b);
  NS_ASSERTION(a != nscoord_MIN && b != nscoord_MIN,
               "NSCoordSaturatingAdd got nscoord_MIN as argument");

#ifdef NS_COORD_IS_FLOAT
  
  return a + b;
#else
  if (a == nscoord_MAX || b == nscoord_MAX) {
    
    return nscoord_MAX;
  } else {
    
    NS_ASSERTION(a < nscoord_MAX && b < nscoord_MAX,
                 "Doing nscoord addition with values > nscoord_MAX");
    NS_ASSERTION((PRInt64)a + (PRInt64)b > (PRInt64)nscoord_MIN,
                 "nscoord addition will reach or pass nscoord_MIN");
    
    
    NS_WARN_IF_FALSE((PRInt64)a + (PRInt64)b < (PRInt64)nscoord_MAX,
                     "nscoord addition capped to nscoord_MAX");

    
    return PR_MIN(nscoord_MAX, a + b);
  }
#endif
}

















inline nscoord 
NSCoordSaturatingSubtract(nscoord a, nscoord b, 
                          nscoord infMinusInfResult)
{
  VERIFY_COORD(a);
  VERIFY_COORD(b);
  NS_ASSERTION(a != nscoord_MIN && b != nscoord_MIN,
               "NSCoordSaturatingSubtract got nscoord_MIN as argument");

  if (b == nscoord_MAX) {
    if (a == nscoord_MAX) {
      
      return infMinusInfResult;
    } else {
      
      NS_NOTREACHED("Attempted to subtract [n - nscoord_MAX]");
      return 0;
    }
  } else {
#ifdef NS_COORD_IS_FLOAT
    
    return a - b;
#else
    if (a == nscoord_MAX) {
      
      return nscoord_MAX;
    } else {
      
      NS_ASSERTION(a < nscoord_MAX && b < nscoord_MAX,
                   "Doing nscoord subtraction with values > nscoord_MAX");
      NS_ASSERTION((PRInt64)a - (PRInt64)b > (PRInt64)nscoord_MIN,
                   "nscoord subtraction will reach or pass nscoord_MIN");
      
      
      NS_WARN_IF_FALSE((PRInt64)a - (PRInt64)b < (PRInt64)nscoord_MAX,
                       "nscoord subtraction capped to nscoord_MAX");

      
      return PR_MIN(nscoord_MAX, a - b);
    }
  }
#endif
}






inline PRInt32 NSCoordToInt(nscoord aCoord) {
  VERIFY_COORD(aCoord);
#ifdef NS_COORD_IS_FLOAT
  NS_ASSERTION(!NS_IEEEIsNan(aCoord), "NaN encountered in int conversion");
  if (aCoord < -2147483648.0f) {
    
    
    return PR_INT32_MIN;
  } else if (aCoord > 2147483520.0f) {
    
    
    return PR_INT32_MAX;
  } else {
    return (PRInt32)aCoord;
  }
#else
  return aCoord;
#endif
}

inline float NSCoordToFloat(nscoord aCoord) {
  VERIFY_COORD(aCoord);
#ifdef NS_COORD_IS_FLOAT
  NS_ASSERTION(!NS_IEEEIsNan(aCoord), "NaN encountered in float conversion");
#endif
  return (float)aCoord;
}

#endif
