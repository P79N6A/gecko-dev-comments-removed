




































#ifndef NSSIZE_H
#define NSSIZE_H

#include "nsCoord.h"
#include "mozilla/gfx/BaseSize.h"


#define NS_MAXSIZE nscoord_MAX

struct nsIntSize;

struct nsSize : public mozilla::gfx::BaseSize<nscoord, nsSize> {
  typedef mozilla::gfx::BaseSize<nscoord, nsSize> Super;

  nsSize() : Super() {}
  nsSize(nscoord aWidth, nscoord aHeight) : Super(aWidth, aHeight) {}

  inline nsIntSize ScaleToNearestPixels(float aXScale, float aYScale,
                                        nscoord aAppUnitsPerPixel) const;

  
  inline nsSize ConvertAppUnits(PRInt32 aFromAPP, PRInt32 aToAPP) const;
};

struct nsIntSize : public mozilla::gfx::BaseSize<PRInt32, nsIntSize> {
  typedef mozilla::gfx::BaseSize<PRInt32, nsIntSize> Super;

  nsIntSize() : Super() {}
  nsIntSize(PRInt32 aWidth, PRInt32 aHeight) : Super(aWidth, aHeight) {}
};

inline nsIntSize
nsSize::ScaleToNearestPixels(float aXScale, float aYScale,
                             nscoord aAppUnitsPerPixel) const
{
  return nsIntSize(
      NSToIntRoundUp(NSAppUnitsToDoublePixels(width, aAppUnitsPerPixel) * aXScale),
      NSToIntRoundUp(NSAppUnitsToDoublePixels(height, aAppUnitsPerPixel) * aYScale));
}

inline nsSize
nsSize::ConvertAppUnits(PRInt32 aFromAPP, PRInt32 aToAPP) const {
  if (aFromAPP != aToAPP) {
    nsSize size;
    size.width = NSToCoordRound(NSCoordScale(width, aFromAPP, aToAPP));
    size.height = NSToCoordRound(NSCoordScale(height, aFromAPP, aToAPP));
    return size;
  }
  return *this;
}

#endif 
