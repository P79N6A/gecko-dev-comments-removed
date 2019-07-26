




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

  static gfx::RectTyped<CSSPixel> FromAppUnits(const nsRect &rect) {
    return gfx::RectTyped<CSSPixel>(NSAppUnitsToFloatPixels(rect.x, float(nsDeviceContext::AppUnitsPerCSSPixel())),
                                    NSAppUnitsToFloatPixels(rect.y, float(nsDeviceContext::AppUnitsPerCSSPixel())),
                                    NSAppUnitsToFloatPixels(rect.width, float(nsDeviceContext::AppUnitsPerCSSPixel())),
                                    NSAppUnitsToFloatPixels(rect.height, float(nsDeviceContext::AppUnitsPerCSSPixel())));
  }
};

typedef gfx::PointTyped<CSSPixel> CSSPoint;
typedef gfx::RectTyped<CSSPixel> CSSRect;
typedef gfx::IntRectTyped<CSSPixel> CSSIntRect;

struct LayerPixel {
  static gfx::IntPointTyped<LayerPixel> FromCSSPointRounded(const CSSPoint& pt, float resolutionX, float resolutionY) {
    return gfx::IntPointTyped<LayerPixel>(NS_lround(pt.x * resolutionX),
                                          NS_lround(pt.y * resolutionY));
  }

  static gfx::IntRectTyped<LayerPixel> FromCSSRectRounded(const CSSRect& rect, float resolutionX, float resolutionY) {
    return gfx::IntRectTyped<LayerPixel>(NS_lround(rect.x * resolutionX),
                                         NS_lround(rect.y * resolutionY),
                                         NS_lround(rect.width * resolutionX),
                                         NS_lround(rect.height * resolutionY));
  }

  static gfx::IntRectTyped<LayerPixel> FromCSSRectRoundOut(const CSSRect& rect, gfxFloat resolution) {
    gfx::RectTyped<LayerPixel> scaled(rect.x, rect.y, rect.width, rect.height);
    scaled.ScaleInverseRoundOut(resolution);
    return gfx::IntRectTyped<LayerPixel>(scaled.x, scaled.y, scaled.width, scaled.height);
  }

  static CSSIntRect ToCSSIntRectRoundIn(const gfx::IntRectTyped<LayerPixel>& rect, gfxFloat resolution) {
    gfx::IntRectTyped<CSSPixel> ret(rect.x, rect.y, rect.width, rect.height);
    ret.ScaleInverseRoundIn(resolution, resolution);
    return ret;
  }

  static CSSIntRect ToCSSIntRectRoundIn(const gfx::IntRectTyped<LayerPixel>& rect, gfxSize resolution) {
    gfx::IntRectTyped<CSSPixel> ret(rect.x, rect.y, rect.width, rect.height);
    ret.ScaleInverseRoundIn(resolution.width, resolution.height);
    return ret;
  }
};

typedef gfx::IntPointTyped<LayerPixel> LayerIntPoint;
typedef gfx::RectTyped<LayerPixel> LayerRect;
typedef gfx::IntRectTyped<LayerPixel> LayerIntRect;

struct ScreenPixel {
};

typedef gfx::PointTyped<ScreenPixel> ScreenPoint;

};

#endif
