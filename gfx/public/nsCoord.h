




































#ifndef NSCOORD_H
#define NSCOORD_H

#include "nscore.h"
#include "nsMathUtils.h"
#include <math.h>
#include <float.h>

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




inline PRBool
NSCoordLessThan(nscoord a,nscoord b)
{
  NS_ASSERTION(a != nscoord_MAX, 
               "This coordinate should be constrained");
  return ((a < b) || (b == nscoord_MAX));
}





inline PRBool
NSCoordGreaterThan(nscoord a,nscoord b)
{
  NS_ASSERTION(a != nscoord_MAX, 
               "This coordinate should be constrained");
  return ((a > b) && (b != nscoord_MAX));
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




inline nscoord NSToCoordFloor(float aValue)
{
  return nscoord(NS_floorf(aValue));
}

inline nscoord NSToCoordCeil(float aValue)
{
  return nscoord(NS_ceilf(aValue));
}

inline nscoord NSToCoordRound(float aValue)
{
#if defined(XP_WIN32) && defined(_M_IX86) && !defined(__GNUC__)
  return NS_lroundup30(aValue);
#else
  return nscoord(NS_floorf(aValue + 0.5f));
#endif 
}




inline PRInt32 NSToIntFloor(float aValue)
{
  return PRInt32(NS_floorf(aValue));
}

inline PRInt32 NSToIntCeil(float aValue)
{
  return PRInt32(NS_ceilf(aValue));
}

inline PRInt32 NSToIntRound(float aValue)
{
  return NS_lroundf(aValue);
}




inline nscoord NSFloatPixelsToAppUnits(float aPixels, PRInt32 aAppUnitsPerPixel)
{
  float product = aPixels * aAppUnitsPerPixel;
  nscoord result;

#ifdef NS_COORD_IS_FLOAT
  
  result = NSToCoordRound(product);
#else
  
  if (product >= nscoord_MAX) {
    NS_WARNING("Overflowed nscoord_MAX in conversion to nscoord");
    result = nscoord_MAX;
  } else if (product <= nscoord_MIN) {
    NS_WARNING("Overflowed nscoord_MIN in conversion to nscoord");
    result = nscoord_MIN;
  } else {
    result = NSToCoordRound(product);
  }
#endif

  VERIFY_COORD(result);
  return result;
}

inline nscoord NSIntPixelsToAppUnits(PRInt32 aPixels, PRInt32 aAppUnitsPerPixel)
{
  
  
  nscoord r = aPixels * (nscoord)aAppUnitsPerPixel;
  VERIFY_COORD(r);
  return r;
}

inline float NSAppUnitsToFloatPixels(nscoord aAppUnits, PRInt32 aAppUnitsPerPixel)
{
  return (float(aAppUnits) / aAppUnitsPerPixel);
}

inline PRInt32 NSAppUnitsToIntPixels(nscoord aAppUnits, PRInt32 aAppUnitsPerPixel)
{
  return NSToIntRound(float(aAppUnits) / aAppUnitsPerPixel);
}


#define TWIPS_PER_POINT_INT           20
#define TWIPS_PER_POINT_FLOAT         20.0f




inline nscoord NSUnitsToTwips(float aValue, float aPointsPerUnit)
{
  return NSToCoordRound(aValue * aPointsPerUnit * TWIPS_PER_POINT_FLOAT);
}

inline float NSTwipsToUnits(nscoord aTwips, float aUnitsPerPoint)
{
  return (aTwips * (aUnitsPerPoint / TWIPS_PER_POINT_FLOAT));
}




#define NS_POINTS_TO_TWIPS(x)         NSUnitsToTwips((x), 1.0f)
#define NS_INCHES_TO_TWIPS(x)         NSUnitsToTwips((x), 72.0f)                      // 72 points per inch
#define NS_FEET_TO_TWIPS(x)           NSUnitsToTwips((x), (72.0f * 12.0f))            // 12 inches per foot
#define NS_MILES_TO_TWIPS(x)          NSUnitsToTwips((x), (72.0f * 12.0f * 5280.0f))  // 5280 feet per mile

#define NS_MILLIMETERS_TO_TWIPS(x)    NSUnitsToTwips((x), (72.0f * 0.03937f))
#define NS_CENTIMETERS_TO_TWIPS(x)    NSUnitsToTwips((x), (72.0f * 0.3937f))
#define NS_METERS_TO_TWIPS(x)         NSUnitsToTwips((x), (72.0f * 39.37f))
#define NS_KILOMETERS_TO_TWIPS(x)     NSUnitsToTwips((x), (72.0f * 39370.0f))

#define NS_PICAS_TO_TWIPS(x)          NSUnitsToTwips((x), 12.0f)                      // 12 points per pica
#define NS_DIDOTS_TO_TWIPS(x)         NSUnitsToTwips((x), (16.0f / 15.0f))            // 15 didots per 16 points
#define NS_CICEROS_TO_TWIPS(x)        NSUnitsToTwips((x), (12.0f * (16.0f / 15.0f)))  // 12 didots per cicero

#define NS_TWIPS_TO_POINTS(x)         NSTwipsToUnits((x), 1.0f)
#define NS_TWIPS_TO_INCHES(x)         NSTwipsToUnits((x), 1.0f / 72.0f)
#define NS_TWIPS_TO_FEET(x)           NSTwipsToUnits((x), 1.0f / (72.0f * 12.0f))
#define NS_TWIPS_TO_MILES(x)          NSTwipsToUnits((x), 1.0f / (72.0f * 12.0f * 5280.0f))

#define NS_TWIPS_TO_MILLIMETERS(x)    NSTwipsToUnits((x), 1.0f / (72.0f * 0.03937f))
#define NS_TWIPS_TO_CENTIMETERS(x)    NSTwipsToUnits((x), 1.0f / (72.0f * 0.3937f))
#define NS_TWIPS_TO_METERS(x)         NSTwipsToUnits((x), 1.0f / (72.0f * 39.37f))
#define NS_TWIPS_TO_KILOMETERS(x)     NSTwipsToUnits((x), 1.0f / (72.0f * 39370.0f))

#define NS_TWIPS_TO_PICAS(x)          NSTwipsToUnits((x), 1.0f / 12.0f)
#define NS_TWIPS_TO_DIDOTS(x)         NSTwipsToUnits((x), 1.0f / (16.0f / 15.0f))
#define NS_TWIPS_TO_CICEROS(x)        NSTwipsToUnits((x), 1.0f / (12.0f * (16.0f / 15.0f)))


#endif
