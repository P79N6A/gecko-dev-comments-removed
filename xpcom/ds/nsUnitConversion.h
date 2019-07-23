



































#ifndef nsUnitConversion_h__
#define nsUnitConversion_h__

#include "nscore.h"
#include "nsCoord.h"
#include "nsMathUtils.h"
#include <math.h>
#include <float.h>


#define TWIPS_PER_POINT_INT           20
#define TWIPS_PER_POINT_FLOAT         20.0f




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
  return nscoord(NS_roundf(aValue));
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
  nscoord r = NSToCoordRound(aPixels * aAppUnitsPerPixel);
  VERIFY_COORD(r);
  return r;
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
