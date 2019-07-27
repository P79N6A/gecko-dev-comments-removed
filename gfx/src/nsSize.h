




#ifndef NSSIZE_H
#define NSSIZE_H

#include "nsCoord.h"
#include "mozilla/gfx/BaseSize.h"
#include "mozilla/gfx/Point.h"


#define NS_MAXSIZE nscoord_MAX

typedef mozilla::gfx::IntSize nsIntSize;
typedef nsIntSize gfxIntSize;

struct nsSize : public mozilla::gfx::BaseSize<nscoord, nsSize> {
  typedef mozilla::gfx::BaseSize<nscoord, nsSize> Super;

  nsSize() : Super() {}
  nsSize(nscoord aWidth, nscoord aHeight) : Super(aWidth, aHeight) {}

  inline nsIntSize ScaleToNearestPixels(float aXScale, float aYScale,
                                        nscoord aAppUnitsPerPixel) const;
  inline nsIntSize ToNearestPixels(nscoord aAppUnitsPerPixel) const;

  




  MOZ_WARN_UNUSED_RESULT inline nsSize
    ScaleToOtherAppUnits(int32_t aFromAPP, int32_t aToAPP) const;
};

inline nsIntSize
nsSize::ScaleToNearestPixels(float aXScale, float aYScale,
                             nscoord aAppUnitsPerPixel) const
{
  return nsIntSize(
      NSToIntRoundUp(NSAppUnitsToDoublePixels(width, aAppUnitsPerPixel) * aXScale),
      NSToIntRoundUp(NSAppUnitsToDoublePixels(height, aAppUnitsPerPixel) * aYScale));
}

inline nsIntSize
nsSize::ToNearestPixels(nscoord aAppUnitsPerPixel) const
{
  return ScaleToNearestPixels(1.0f, 1.0f, aAppUnitsPerPixel);
}

inline nsSize
nsSize::ScaleToOtherAppUnits(int32_t aFromAPP, int32_t aToAPP) const {
  if (aFromAPP != aToAPP) {
    nsSize size;
    size.width = NSToCoordRound(NSCoordScale(width, aFromAPP, aToAPP));
    size.height = NSToCoordRound(NSCoordScale(height, aFromAPP, aToAPP));
    return size;
  }
  return *this;
}

inline nsSize
IntSizeToAppUnits(mozilla::gfx::IntSize aSize, nscoord aAppUnitsPerPixel)
{
  return nsSize(NSIntPixelsToAppUnits(aSize.width, aAppUnitsPerPixel),
                NSIntPixelsToAppUnits(aSize.height, aAppUnitsPerPixel));
}

#endif 
