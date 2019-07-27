





#ifndef NSRECT_H
#define NSRECT_H

#include <stdio.h>                      
#include <stdint.h>                     
#include <algorithm>                    
#include "nsDebug.h"                    
#include "gfxCore.h"                    
#include "mozilla/Likely.h"             
#include "mozilla/gfx/Rect.h"
#include "nsCoord.h"                    
#include "nsISupportsImpl.h"            
#include "nsPoint.h"                    
#include "nsSize.h"                     
#include "nscore.h"                     

struct nsMargin;
struct nsIntMargin;

typedef mozilla::gfx::IntRect nsIntRect;

struct NS_GFX nsRect :
  public mozilla::gfx::BaseRect<nscoord, nsRect, nsPoint, nsSize, nsMargin> {
  typedef mozilla::gfx::BaseRect<nscoord, nsRect, nsPoint, nsSize, nsMargin> Super;

  static void VERIFY_COORD(nscoord aValue) { ::VERIFY_COORD(aValue); }

  
  nsRect() : Super()
  {
    MOZ_COUNT_CTOR(nsRect);
  }
  nsRect(const nsRect& aRect) : Super(aRect)
  {
    MOZ_COUNT_CTOR(nsRect);
  }
  nsRect(const nsPoint& aOrigin, const nsSize &aSize) : Super(aOrigin, aSize)
  {
    MOZ_COUNT_CTOR(nsRect);
  }
  nsRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight) :
      Super(aX, aY, aWidth, aHeight)
  {
    MOZ_COUNT_CTOR(nsRect);
  }

#ifdef NS_BUILD_REFCNT_LOGGING
  ~nsRect() {
    MOZ_COUNT_DTOR(nsRect);
  }
#endif

  
  
  

  MOZ_WARN_UNUSED_RESULT nsRect SaturatingUnion(const nsRect& aRect) const
  {
    if (IsEmpty()) {
      return aRect;
    } else if (aRect.IsEmpty()) {
      return *static_cast<const nsRect*>(this);
    } else {
      return SaturatingUnionEdges(aRect);
    }
  }

  MOZ_WARN_UNUSED_RESULT nsRect SaturatingUnionEdges(const nsRect& aRect) const
  {
#ifdef NS_COORD_IS_FLOAT
    return UnionEdges(aRect);
#else
    nsRect result;
    result.x = std::min(aRect.x, x);
    int64_t w = std::max(int64_t(aRect.x) + aRect.width, int64_t(x) + width) - result.x;
    if (MOZ_UNLIKELY(w > nscoord_MAX)) {
      NS_WARNING("Overflowed nscoord_MAX in conversion to nscoord width");
      
      result.x = std::max(result.x, nscoord_MIN / 2);
      w = std::max(int64_t(aRect.x) + aRect.width, int64_t(x) + width) - result.x;
      if (MOZ_UNLIKELY(w > nscoord_MAX)) {
        w = nscoord_MAX;
      }
    }
    result.width = nscoord(w);

    result.y = std::min(aRect.y, y);
    int64_t h = std::max(int64_t(aRect.y) + aRect.height, int64_t(y) + height) - result.y;
    if (MOZ_UNLIKELY(h > nscoord_MAX)) {
      NS_WARNING("Overflowed nscoord_MAX in conversion to nscoord height");
      
      result.y = std::max(result.y, nscoord_MIN / 2);
      h = std::max(int64_t(aRect.y) + aRect.height, int64_t(y) + height) - result.y;
      if (MOZ_UNLIKELY(h > nscoord_MAX)) {
        h = nscoord_MAX;
      }
    }
    result.height = nscoord(h);
    return result;
#endif
  }

#ifndef NS_COORD_IS_FLOAT
  
  MOZ_WARN_UNUSED_RESULT nsRect UnionEdges(const nsRect& aRect) const
  {
    return SaturatingUnionEdges(aRect);
  }
  void UnionRectEdges(const nsRect& aRect1, const nsRect& aRect2)
  {
    *this = aRect1.UnionEdges(aRect2);
  }
  MOZ_WARN_UNUSED_RESULT nsRect Union(const nsRect& aRect) const
  {
    return SaturatingUnion(aRect);
  }
  void UnionRect(const nsRect& aRect1, const nsRect& aRect2)
  {
    *this = aRect1.Union(aRect2);
  }
#endif

  void SaturatingUnionRect(const nsRect& aRect1, const nsRect& aRect2)
  {
    *this = aRect1.SaturatingUnion(aRect2);
  }
  void SaturatingUnionRectEdges(const nsRect& aRect1, const nsRect& aRect2)
  {
    *this = aRect1.SaturatingUnionEdges(aRect2);
  }

  








  MOZ_WARN_UNUSED_RESULT inline nsRect
    ScaleToOtherAppUnitsRoundOut(int32_t aFromAPP, int32_t aToAPP) const;
  MOZ_WARN_UNUSED_RESULT inline nsRect
    ScaleToOtherAppUnitsRoundIn(int32_t aFromAPP, int32_t aToAPP) const;

  MOZ_WARN_UNUSED_RESULT inline nsIntRect
  ScaleToNearestPixels(float aXScale, float aYScale,
                       nscoord aAppUnitsPerPixel) const;

  MOZ_WARN_UNUSED_RESULT inline nsIntRect
  ToNearestPixels(nscoord aAppUnitsPerPixel) const;

  
  MOZ_WARN_UNUSED_RESULT inline nsIntRect
  ScaleToOutsidePixels(float aXScale, float aYScale,
                       nscoord aAppUnitsPerPixel) const;

  
  MOZ_WARN_UNUSED_RESULT inline nsIntRect
  ToOutsidePixels(nscoord aAppUnitsPerPixel) const;

  MOZ_WARN_UNUSED_RESULT inline nsIntRect
  ScaleToInsidePixels(float aXScale, float aYScale,
                      nscoord aAppUnitsPerPixel) const;

  MOZ_WARN_UNUSED_RESULT inline nsIntRect
  ToInsidePixels(nscoord aAppUnitsPerPixel) const;

  
  bool operator==(const nsRect& aRect) const
  {
    return IsEqualEdges(aRect);
  }
};





inline nsRect
nsRect::ScaleToOtherAppUnitsRoundOut(int32_t aFromAPP, int32_t aToAPP) const
{
  if (aFromAPP == aToAPP) {
    return *this;
  }

  nsRect rect;
  nscoord right = NSToCoordCeil(NSCoordScale(XMost(), aFromAPP, aToAPP));
  nscoord bottom = NSToCoordCeil(NSCoordScale(YMost(), aFromAPP, aToAPP));
  rect.x = NSToCoordFloor(NSCoordScale(x, aFromAPP, aToAPP));
  rect.y = NSToCoordFloor(NSCoordScale(y, aFromAPP, aToAPP));
  rect.width = (right - rect.x);
  rect.height = (bottom - rect.y);

  return rect;
}

inline nsRect
nsRect::ScaleToOtherAppUnitsRoundIn(int32_t aFromAPP, int32_t aToAPP) const
{
  if (aFromAPP == aToAPP) {
    return *this;
  }

  nsRect rect;
  nscoord right = NSToCoordFloor(NSCoordScale(XMost(), aFromAPP, aToAPP));
  nscoord bottom = NSToCoordFloor(NSCoordScale(YMost(), aFromAPP, aToAPP));
  rect.x = NSToCoordCeil(NSCoordScale(x, aFromAPP, aToAPP));
  rect.y = NSToCoordCeil(NSCoordScale(y, aFromAPP, aToAPP));
  rect.width = (right - rect.x);
  rect.height = (bottom - rect.y);

  return rect;
}


inline nsIntRect
nsRect::ScaleToNearestPixels(float aXScale, float aYScale,
                             nscoord aAppUnitsPerPixel) const
{
  nsIntRect rect;
  rect.x = NSToIntRoundUp(NSAppUnitsToDoublePixels(x, aAppUnitsPerPixel) * aXScale);
  rect.y = NSToIntRoundUp(NSAppUnitsToDoublePixels(y, aAppUnitsPerPixel) * aYScale);
  
  rect.width  = std::max(0, NSToIntRoundUp(NSAppUnitsToDoublePixels(XMost(),
                               aAppUnitsPerPixel) * aXScale) - rect.x);
  rect.height = std::max(0, NSToIntRoundUp(NSAppUnitsToDoublePixels(YMost(),
                               aAppUnitsPerPixel) * aYScale) - rect.y);
  return rect;
}


inline nsIntRect
nsRect::ScaleToOutsidePixels(float aXScale, float aYScale,
                             nscoord aAppUnitsPerPixel) const
{
  nsIntRect rect;
  rect.x = NSToIntFloor(NSAppUnitsToFloatPixels(x, float(aAppUnitsPerPixel)) * aXScale);
  rect.y = NSToIntFloor(NSAppUnitsToFloatPixels(y, float(aAppUnitsPerPixel)) * aYScale);
  
  rect.width  = std::max(0, NSToIntCeil(NSAppUnitsToFloatPixels(XMost(),
                            float(aAppUnitsPerPixel)) * aXScale) - rect.x);
  rect.height = std::max(0, NSToIntCeil(NSAppUnitsToFloatPixels(YMost(),
                            float(aAppUnitsPerPixel)) * aYScale) - rect.y);
  return rect;
}


inline nsIntRect
nsRect::ScaleToInsidePixels(float aXScale, float aYScale,
                            nscoord aAppUnitsPerPixel) const
{
  nsIntRect rect;
  rect.x = NSToIntCeil(NSAppUnitsToFloatPixels(x, float(aAppUnitsPerPixel)) * aXScale);
  rect.y = NSToIntCeil(NSAppUnitsToFloatPixels(y, float(aAppUnitsPerPixel)) * aYScale);
  
  rect.width  = std::max(0, NSToIntFloor(NSAppUnitsToFloatPixels(XMost(),
                             float(aAppUnitsPerPixel)) * aXScale) - rect.x);
  rect.height = std::max(0, NSToIntFloor(NSAppUnitsToFloatPixels(YMost(),
                             float(aAppUnitsPerPixel)) * aYScale) - rect.y);
  return rect;
}

inline nsIntRect
nsRect::ToNearestPixels(nscoord aAppUnitsPerPixel) const
{
  return ScaleToNearestPixels(1.0f, 1.0f, aAppUnitsPerPixel);
}

inline nsIntRect
nsRect::ToOutsidePixels(nscoord aAppUnitsPerPixel) const
{
  return ScaleToOutsidePixels(1.0f, 1.0f, aAppUnitsPerPixel);
}

inline nsIntRect
nsRect::ToInsidePixels(nscoord aAppUnitsPerPixel) const
{
  return ScaleToInsidePixels(1.0f, 1.0f, aAppUnitsPerPixel);
}

const nsIntRect& GetMaxSizedIntRect();


nsRect
ToAppUnits(const nsIntRect& aRect, nscoord aAppUnitsPerPixel);

#ifdef DEBUG

extern NS_GFX FILE* operator<<(FILE* out, const nsRect& rect);
#endif 

#endif 
