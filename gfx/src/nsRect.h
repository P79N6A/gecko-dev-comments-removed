





































#ifndef NSRECT_H
#define NSRECT_H

#include <stdio.h>
#include "nsCoord.h"
#include "nsPoint.h"
#include "nsSize.h"
#include "nsMargin.h"
#include "gfxCore.h"
#include "nsTraceRefcnt.h"
#include "mozilla/BaseRect.h"

struct nsIntRect;

struct NS_GFX nsRect :
  public mozilla::BaseRect<nscoord, nsRect, nsPoint, nsSize, nsMargin> {
  typedef mozilla::BaseRect<nscoord, nsRect, nsPoint, nsSize, nsMargin> Super;

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

  
  
  
  nsRect& ExtendForScaling(float aXMult, float aYMult);

  
  
  
  
  inline nsRect ConvertAppUnitsRoundOut(PRInt32 aFromAPP, PRInt32 aToAPP) const;
  inline nsRect ConvertAppUnitsRoundIn(PRInt32 aFromAPP, PRInt32 aToAPP) const;

  inline nsIntRect ToNearestPixels(nscoord aAppUnitsPerPixel) const;
  inline nsIntRect ToOutsidePixels(nscoord aAppUnitsPerPixel) const;
  inline nsIntRect ToInsidePixels(nscoord aAppUnitsPerPixel) const;
};

struct NS_GFX nsIntRect :
  public mozilla::BaseRect<PRInt32, nsIntRect, nsIntPoint, nsIntSize, nsIntMargin> {
  typedef mozilla::BaseRect<PRInt32, nsIntRect, nsIntPoint, nsIntSize, nsIntMargin> Super;

  
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
nsRect::ToNearestPixels(nscoord aAppUnitsPerPixel) const
{
  nsIntRect rect;
  rect.x = NSToIntRoundUp(NSAppUnitsToDoublePixels(x, aAppUnitsPerPixel));
  rect.y = NSToIntRoundUp(NSAppUnitsToDoublePixels(y, aAppUnitsPerPixel));
  rect.width  = NSToIntRoundUp(NSAppUnitsToDoublePixels(XMost(),
                               aAppUnitsPerPixel)) - rect.x;
  rect.height = NSToIntRoundUp(NSAppUnitsToDoublePixels(YMost(),
                               aAppUnitsPerPixel)) - rect.y;
  return rect;
}


inline nsIntRect
nsRect::ToOutsidePixels(nscoord aAppUnitsPerPixel) const
{
  nsIntRect rect;
  rect.x = NSToIntFloor(NSAppUnitsToFloatPixels(x, float(aAppUnitsPerPixel)));
  rect.y = NSToIntFloor(NSAppUnitsToFloatPixels(y, float(aAppUnitsPerPixel)));
  rect.width  = NSToIntCeil(NSAppUnitsToFloatPixels(XMost(),
                            float(aAppUnitsPerPixel))) - rect.x;
  rect.height = NSToIntCeil(NSAppUnitsToFloatPixels(YMost(),
                            float(aAppUnitsPerPixel))) - rect.y;
  return rect;
}


inline nsIntRect
nsRect::ToInsidePixels(nscoord aAppUnitsPerPixel) const
{
  nsIntRect rect;
  rect.x = NSToIntCeil(NSAppUnitsToFloatPixels(x, float(aAppUnitsPerPixel)));
  rect.y = NSToIntCeil(NSAppUnitsToFloatPixels(y, float(aAppUnitsPerPixel)));
  rect.width  = NSToIntFloor(NSAppUnitsToFloatPixels(XMost(),
                             float(aAppUnitsPerPixel))) - rect.x;
  rect.height = NSToIntFloor(NSAppUnitsToFloatPixels(YMost(),
                             float(aAppUnitsPerPixel))) - rect.y;
  return rect;
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
