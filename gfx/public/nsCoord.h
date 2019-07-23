




































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
