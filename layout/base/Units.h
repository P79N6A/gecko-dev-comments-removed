




#ifndef MOZ_UNITS_H_
#define MOZ_UNITS_H_

#include "mozilla/gfx/Point.h"
#include "nsDeviceContext.h"

namespace mozilla {


struct CSSPixel {
  static gfx::PointTyped<CSSPixel> FromAppUnits(const nsPoint &pt) {
    return gfx::PointTyped<CSSPixel>(NSAppUnitsToFloatPixels(pt.x, float(nsDeviceContext::AppUnitsPerCSSPixel())),
                                     NSAppUnitsToFloatPixels(pt.y, float(nsDeviceContext::AppUnitsPerCSSPixel())));
  }

  static nsPoint ToAppUnits(const gfx::PointTyped<CSSPixel> &pt) {
    return nsPoint(NSFloatPixelsToAppUnits(pt.x, float(nsDeviceContext::AppUnitsPerCSSPixel())),
                   NSFloatPixelsToAppUnits(pt.y, float(nsDeviceContext::AppUnitsPerCSSPixel())));
  }
};

typedef gfx::PointTyped<CSSPixel> CSSPoint;

};

#endif
