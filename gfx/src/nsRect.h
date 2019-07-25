





#ifndef NSRECT_H
#define NSRECT_H

#include <stdio.h>
#include "nsCoord.h"
#include "nsPoint.h"
#include "nsSize.h"
#include "nsMargin.h"
#include "gfxCore.h"
#include "nsTraceRefcnt.h"
#include "mozilla/gfx/BaseRect.h"

struct nsIntRect;

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

  
  
  
  void SaturatingInflate(const nsMargin& aMargin)
  {
#ifdef NS_COORD_IS_FLOAT
    Inflate(aMargin);
#else
    PRInt64 nx = PRInt64(x) - aMargin.left;
    PRInt64 w = PRInt64(width) + PRInt64(aMargin.left) + aMargin.right;
    if (NS_UNLIKELY(w > nscoord_MAX)) {
      NS_WARNING("Overflowed nscoord_MAX in conversion to nscoord width");
      PRInt64 xdiff = nx - nscoord_MIN / 2;
      if (xdiff < 0) {
        
        nx = nscoord_MIN / 2;
        w += xdiff;
      }
      if (NS_UNLIKELY(w > nscoord_MAX)) {
        w = nscoord_MAX;
      }
    }
    width = nscoord(w);
    if (NS_UNLIKELY(nx < nscoord_MIN)) {
      NS_WARNING("Underflowed nscoord_MIN in conversion to nscoord x");
      nx = nscoord_MIN;
    }
    x = nscoord(nx);

    PRInt64 ny = PRInt64(y) - aMargin.top;
    PRInt64 h = PRInt64(height) + PRInt64(aMargin.top) + aMargin.bottom;
    if (NS_UNLIKELY(h > nscoord_MAX)) {
      NS_WARNING("Overflowed nscoord_MAX in conversion to nscoord height");
      PRInt64 ydiff = ny - nscoord_MIN / 2;
      if (ydiff < 0) {
        
        ny = nscoord_MIN / 2;
        h += ydiff;
      }
      if (NS_UNLIKELY(h > nscoord_MAX)) {
        h = nscoord_MAX;
      }
    }
    height = nscoord(h);
    if (NS_UNLIKELY(ny < nscoord_MIN)) {
      NS_WARNING("Underflowed nscoord_MIN in conversion to nscoord y");
      ny = nscoord_MIN;
    }
    y = nscoord(ny);
#endif
  }

  
  
  

  nsRect SaturatingUnion(const nsRect& aRect) const
  {
    if (IsEmpty()) {
      return aRect;
    } else if (aRect.IsEmpty()) {
      return *static_cast<const nsRect*>(this);
    } else {
      return SaturatingUnionEdges(aRect);
    }
  }

  nsRect SaturatingUnionEdges(const nsRect& aRect) const
  {
#ifdef NS_COORD_IS_FLOAT
    return UnionEdges(aRect);
#else
    nsRect result;
    result.x = NS_MIN(aRect.x, x);
    PRInt64 w = NS_MAX(PRInt64(aRect.x) + aRect.width, PRInt64(x) + width) - result.x;
    if (NS_UNLIKELY(w > nscoord_MAX)) {
      NS_WARNING("Overflowed nscoord_MAX in conversion to nscoord width");
      
      result.x = NS_MAX(result.x, nscoord_MIN / 2);
      w = NS_MAX(PRInt64(aRect.x) + aRect.width, PRInt64(x) + width) - result.x;
      if (NS_UNLIKELY(w > nscoord_MAX)) {
        w = nscoord_MAX;
      }
    }
    result.width = nscoord(w);

    result.y = NS_MIN(aRect.y, y);
    PRInt64 h = NS_MAX(PRInt64(aRect.y) + aRect.height, PRInt64(y) + height) - result.y;
    if (NS_UNLIKELY(h > nscoord_MAX)) {
      NS_WARNING("Overflowed nscoord_MAX in conversion to nscoord height");
      
      result.y = NS_MAX(result.y, nscoord_MIN / 2);
      h = NS_MAX(PRInt64(aRect.y) + aRect.height, PRInt64(y) + height) - result.y;
      if (NS_UNLIKELY(h > nscoord_MAX)) {
        h = nscoord_MAX;
      }
    }
    result.height = nscoord(h);
    return result;
#endif
  }

#ifndef NS_COORD_IS_FLOAT
  
  nsRect UnionEdges(const nsRect& aRect) const
  {
    return SaturatingUnionEdges(aRect);
  }
  void UnionRectEdges(const nsRect& aRect1, const nsRect& aRect2)
  {
    *this = aRect1.UnionEdges(aRect2);
  }
  nsRect Union(const nsRect& aRect) const
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

  
  
  
  
  
  inline nsRect ConvertAppUnitsRoundOut(PRInt32 aFromAPP, PRInt32 aToAPP) const;
  inline nsRect ConvertAppUnitsRoundIn(PRInt32 aFromAPP, PRInt32 aToAPP) const;

  inline nsIntRect ScaleToNearestPixels(float aXScale, float aYScale,
                                        nscoord aAppUnitsPerPixel) const;
  inline nsIntRect ToNearestPixels(nscoord aAppUnitsPerPixel) const;
  
  inline nsIntRect ScaleToOutsidePixels(float aXScale, float aYScale,
                                        nscoord aAppUnitsPerPixel) const;
  
  inline nsIntRect ToOutsidePixels(nscoord aAppUnitsPerPixel) const;
  inline nsIntRect ScaleToInsidePixels(float aXScale, float aYScale,
                                       nscoord aAppUnitsPerPixel) const;
  inline nsIntRect ToInsidePixels(nscoord aAppUnitsPerPixel) const;
};

struct NS_GFX nsIntRect :
  public mozilla::gfx::BaseRect<PRInt32, nsIntRect, nsIntPoint, nsIntSize, nsIntMargin> {
  typedef mozilla::gfx::BaseRect<PRInt32, nsIntRect, nsIntPoint, nsIntSize, nsIntMargin> Super;

  
  nsIntRect() : Super()
  {
  }
  nsIntRect(const nsIntRect& aRect) : Super(aRect)
  {
  }
  nsIntRect(const nsIntPoint& aOrigin, const nsIntSize &aSize) : Super(aOrigin, aSize)
  {
  }
  nsIntRect(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight) :
      Super(aX, aY, aWidth, aHeight)
  {
  }

  inline nsRect ToAppUnits(nscoord aAppUnitsPerPixel) const;

  
  
  static const nsIntRect& GetMaxSizedIntRect() { return kMaxSizedIntRect; }

  
  bool operator==(const nsIntRect& aRect) const
  {
    return IsEqualEdges(aRect);
  }

protected:
  static const nsIntRect kMaxSizedIntRect;
};





inline nsRect
nsRect::ConvertAppUnitsRoundOut(PRInt32 aFromAPP, PRInt32 aToAPP) const
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
nsRect::ConvertAppUnitsRoundIn(PRInt32 aFromAPP, PRInt32 aToAPP) const
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
  rect.width  = NSToIntRoundUp(NSAppUnitsToDoublePixels(XMost(),
                               aAppUnitsPerPixel) * aXScale) - rect.x;
  rect.height = NSToIntRoundUp(NSAppUnitsToDoublePixels(YMost(),
                               aAppUnitsPerPixel) * aYScale) - rect.y;
  return rect;
}


inline nsIntRect
nsRect::ScaleToOutsidePixels(float aXScale, float aYScale,
                             nscoord aAppUnitsPerPixel) const
{
  nsIntRect rect;
  rect.x = NSToIntFloor(NSAppUnitsToFloatPixels(x, float(aAppUnitsPerPixel)) * aXScale);
  rect.y = NSToIntFloor(NSAppUnitsToFloatPixels(y, float(aAppUnitsPerPixel)) * aYScale);
  rect.width  = NSToIntCeil(NSAppUnitsToFloatPixels(XMost(),
                            float(aAppUnitsPerPixel)) * aXScale) - rect.x;
  rect.height = NSToIntCeil(NSAppUnitsToFloatPixels(YMost(),
                            float(aAppUnitsPerPixel)) * aYScale) - rect.y;
  return rect;
}


inline nsIntRect
nsRect::ScaleToInsidePixels(float aXScale, float aYScale,
                            nscoord aAppUnitsPerPixel) const
{
  nsIntRect rect;
  rect.x = NSToIntCeil(NSAppUnitsToFloatPixels(x, float(aAppUnitsPerPixel)) * aXScale);
  rect.y = NSToIntCeil(NSAppUnitsToFloatPixels(y, float(aAppUnitsPerPixel)) * aYScale);
  rect.width  = NSToIntFloor(NSAppUnitsToFloatPixels(XMost(),
                             float(aAppUnitsPerPixel)) * aXScale) - rect.x;
  rect.height = NSToIntFloor(NSAppUnitsToFloatPixels(YMost(),
                             float(aAppUnitsPerPixel)) * aYScale) - rect.y;
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


inline nsRect
nsIntRect::ToAppUnits(nscoord aAppUnitsPerPixel) const
{
  return nsRect(NSIntPixelsToAppUnits(x, aAppUnitsPerPixel),
                NSIntPixelsToAppUnits(y, aAppUnitsPerPixel),
                NSIntPixelsToAppUnits(width, aAppUnitsPerPixel),
                NSIntPixelsToAppUnits(height, aAppUnitsPerPixel));
}

#ifdef DEBUG

extern NS_GFX FILE* operator<<(FILE* out, const nsRect& rect);
#endif 

#endif 
