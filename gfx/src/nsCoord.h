




#ifndef NSCOORD_H
#define NSCOORD_H

#include "nsAlgorithm.h"
#include "nscore.h"
#include "nsMathUtils.h"
#include <math.h>
#include <float.h>

#include "nsDebug.h"
#include <algorithm>














inline float NS_IEEEPositiveInfinity() {
  union { uint32_t mPRUint32; float mFloat; } pun;
  pun.mPRUint32 = 0x7F800000;
  return pun.mFloat;
}
inline bool NS_IEEEIsNan(float aF) {
  union { uint32_t mBits; float mFloat; } pun;
  pun.mFloat = aF;
  return (pun.mBits & 0x7F800000) == 0x7F800000 &&
    (pun.mBits & 0x007FFFFF) != 0;
}

#ifdef NS_COORD_IS_FLOAT
typedef float nscoord;
#define nscoord_MAX NS_IEEEPositiveInfinity()
#else
typedef int32_t nscoord;
#define nscoord_MAX nscoord(1 << 30)
#endif

#define nscoord_MIN (-nscoord_MAX)

inline void VERIFY_COORD(nscoord aCoord) {
#ifdef NS_COORD_IS_FLOAT
  NS_ASSERTION(floorf(aCoord) == aCoord,
               "Coords cannot have fractions");
#endif
}

inline nscoord NSCoordMulDiv(nscoord aMult1, nscoord aMult2, nscoord aDiv) {
#ifdef NS_COORD_IS_FLOAT
  return (aMult1 * aMult2 / aDiv);
#else
  return (int64_t(aMult1) * int64_t(aMult2) / int64_t(aDiv));
#endif
}

inline nscoord NSToCoordRound(float aValue)
{
#if defined(XP_WIN32) && defined(_M_IX86) && !defined(__GNUC__) && !defined(__clang__)
  return NS_lroundup30(aValue);
#else
  return nscoord(floorf(aValue + 0.5f));
#endif 
}

inline nscoord NSToCoordRound(double aValue)
{
#if defined(XP_WIN32) && defined(_M_IX86) && !defined(__GNUC__) && !defined(__clang__)
  return NS_lroundup30((float)aValue);
#else
  return nscoord(floor(aValue + 0.5f));
#endif 
}

inline nscoord NSToCoordRoundWithClamp(float aValue)
{
#ifndef NS_COORD_IS_FLOAT
  
  if (aValue >= nscoord_MAX) {
    return nscoord_MAX;
  }
  if (aValue <= nscoord_MIN) {
    return nscoord_MIN;
  }
#endif
  return NSToCoordRound(aValue);
}







inline nscoord _nscoordSaturatingMultiply(nscoord aCoord, float aScale,
                                          bool requireNotNegative) {
  VERIFY_COORD(aCoord);
  if (requireNotNegative) {
    NS_ABORT_IF_FALSE(aScale >= 0.0f,
                      "negative scaling factors must be handled manually");
  }
#ifdef NS_COORD_IS_FLOAT
  return floorf(aCoord * aScale);
#else
  float product = aCoord * aScale;
  if (requireNotNegative ? aCoord > 0 : (aCoord > 0) == (aScale > 0))
    return NSToCoordRoundWithClamp(std::min<float>(nscoord_MAX, product));
  return NSToCoordRoundWithClamp(std::max<float>(nscoord_MIN, product));
#endif
}







inline nscoord NSCoordSaturatingNonnegativeMultiply(nscoord aCoord, float aScale) {
  return _nscoordSaturatingMultiply(aCoord, aScale, true);
}





inline nscoord NSCoordSaturatingMultiply(nscoord aCoord, float aScale) {
  return _nscoordSaturatingMultiply(aCoord, aScale, false);
}










inline nscoord
NSCoordSaturatingAdd(nscoord a, nscoord b)
{
  VERIFY_COORD(a);
  VERIFY_COORD(b);

#ifdef NS_COORD_IS_FLOAT
  
  return a + b;
#else
  if (a == nscoord_MAX || b == nscoord_MAX) {
    
    return nscoord_MAX;
  } else {
    
    
    return std::min(nscoord_MAX, a + b);
  }
#endif
}

















inline nscoord 
NSCoordSaturatingSubtract(nscoord a, nscoord b, 
                          nscoord infMinusInfResult)
{
  VERIFY_COORD(a);
  VERIFY_COORD(b);

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
      
      
      return std::min(nscoord_MAX, a - b);
    }
  }
#endif
}

inline float NSCoordToFloat(nscoord aCoord) {
  VERIFY_COORD(aCoord);
#ifdef NS_COORD_IS_FLOAT
  NS_ASSERTION(!NS_IEEEIsNan(aCoord), "NaN encountered in float conversion");
#endif
  return (float)aCoord;
}




inline nscoord NSToCoordFloor(float aValue)
{
  return nscoord(floorf(aValue));
}

inline nscoord NSToCoordFloor(double aValue)
{
  return nscoord(floor(aValue));
}

inline nscoord NSToCoordFloorClamped(float aValue)
{
#ifndef NS_COORD_IS_FLOAT
  
  if (aValue >= nscoord_MAX) {
    return nscoord_MAX;
  }
  if (aValue <= nscoord_MIN) {
    return nscoord_MIN;
  }
#endif
  return NSToCoordFloor(aValue);
}

inline nscoord NSToCoordCeil(float aValue)
{
  return nscoord(ceilf(aValue));
}

inline nscoord NSToCoordCeil(double aValue)
{
  return nscoord(ceil(aValue));
}

inline nscoord NSToCoordCeilClamped(double aValue)
{
#ifndef NS_COORD_IS_FLOAT
  
  if (aValue >= nscoord_MAX) {
    return nscoord_MAX;
  }
  if (aValue <= nscoord_MIN) {
    return nscoord_MIN;
  }
#endif
  return NSToCoordCeil(aValue);
}





inline nscoord NSToCoordTrunc(float aValue)
{
  
  
  return nscoord(aValue);
}

inline nscoord NSToCoordTrunc(double aValue)
{
  
  
  return nscoord(aValue);
}

inline nscoord NSToCoordTruncClamped(float aValue)
{
#ifndef NS_COORD_IS_FLOAT
  
  if (aValue >= nscoord_MAX) {
    return nscoord_MAX;
  }
  if (aValue <= nscoord_MIN) {
    return nscoord_MIN;
  }
#endif
  return NSToCoordTrunc(aValue);
}

inline nscoord NSToCoordTruncClamped(double aValue)
{
#ifndef NS_COORD_IS_FLOAT
  
  if (aValue >= nscoord_MAX) {
    return nscoord_MAX;
  }
  if (aValue <= nscoord_MIN) {
    return nscoord_MIN;
  }
#endif
  return NSToCoordTrunc(aValue);
}




inline int32_t NSToIntFloor(float aValue)
{
  return int32_t(floorf(aValue));
}

inline int32_t NSToIntCeil(float aValue)
{
  return int32_t(ceilf(aValue));
}

inline int32_t NSToIntRound(float aValue)
{
  return NS_lroundf(aValue);
}

inline int32_t NSToIntRound(double aValue)
{
  return NS_lround(aValue);
}

inline int32_t NSToIntRoundUp(double aValue)
{
  return int32_t(floor(aValue + 0.5));
}




inline nscoord NSFloatPixelsToAppUnits(float aPixels, float aAppUnitsPerPixel)
{
  return NSToCoordRoundWithClamp(aPixels * aAppUnitsPerPixel);
}

inline nscoord NSIntPixelsToAppUnits(int32_t aPixels, int32_t aAppUnitsPerPixel)
{
  
  
  nscoord r = aPixels * (nscoord)aAppUnitsPerPixel;
  VERIFY_COORD(r);
  return r;
}

inline float NSAppUnitsToFloatPixels(nscoord aAppUnits, float aAppUnitsPerPixel)
{
  return (float(aAppUnits) / aAppUnitsPerPixel);
}

inline double NSAppUnitsToDoublePixels(nscoord aAppUnits, double aAppUnitsPerPixel)
{
  return (double(aAppUnits) / aAppUnitsPerPixel);
}

inline int32_t NSAppUnitsToIntPixels(nscoord aAppUnits, float aAppUnitsPerPixel)
{
  return NSToIntRound(float(aAppUnits) / aAppUnitsPerPixel);
}

inline float NSCoordScale(nscoord aCoord, int32_t aFromAPP, int32_t aToAPP)
{
  return (NSCoordToFloat(aCoord) * aToAPP) / aFromAPP;
}


#define TWIPS_PER_POINT_INT           20
#define TWIPS_PER_POINT_FLOAT         20.0f
#define POINTS_PER_INCH_INT           72
#define POINTS_PER_INCH_FLOAT         72.0f
#define CM_PER_INCH_FLOAT             2.54f
#define MM_PER_INCH_FLOAT             25.4f




inline float NSUnitsToTwips(float aValue, float aPointsPerUnit)
{
  return aValue * aPointsPerUnit * TWIPS_PER_POINT_FLOAT;
}

inline float NSTwipsToUnits(float aTwips, float aUnitsPerPoint)
{
  return (aTwips * (aUnitsPerPoint / TWIPS_PER_POINT_FLOAT));
}



#define NS_POINTS_TO_TWIPS(x)         NSUnitsToTwips((x), 1.0f)
#define NS_INCHES_TO_TWIPS(x)         NSUnitsToTwips((x), POINTS_PER_INCH_FLOAT)                      // 72 points per inch

#define NS_MILLIMETERS_TO_TWIPS(x)    NSUnitsToTwips((x), (POINTS_PER_INCH_FLOAT * 0.03937f))

#define NS_POINTS_TO_INT_TWIPS(x)     NSToIntRound(NS_POINTS_TO_TWIPS(x))
#define NS_INCHES_TO_INT_TWIPS(x)     NSToIntRound(NS_INCHES_TO_TWIPS(x))

#define NS_TWIPS_TO_INCHES(x)         NSTwipsToUnits((x), 1.0f / POINTS_PER_INCH_FLOAT)

#define NS_TWIPS_TO_MILLIMETERS(x)    NSTwipsToUnits((x), 1.0f / (POINTS_PER_INCH_FLOAT * 0.03937f))


#endif
