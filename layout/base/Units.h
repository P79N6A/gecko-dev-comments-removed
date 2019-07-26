




#ifndef MOZ_UNITS_H_
#define MOZ_UNITS_H_

#include "mozilla/gfx/Point.h"
#include "nsDeviceContext.h"

namespace mozilla {




struct CSSPixel {
  static gfx::IntPointTyped<CSSPixel> RoundToInt(const gfx::PointTyped<CSSPixel>& aPoint) {
    return gfx::IntPointTyped<CSSPixel>(NS_lround(aPoint.x),
                                        NS_lround(aPoint.y));
  }

  static gfx::PointTyped<CSSPixel> FromAppUnits(const nsPoint& aPoint) {
    return gfx::PointTyped<CSSPixel>(NSAppUnitsToFloatPixels(aPoint.x, float(nsDeviceContext::AppUnitsPerCSSPixel())),
                                     NSAppUnitsToFloatPixels(aPoint.y, float(nsDeviceContext::AppUnitsPerCSSPixel())));
  }

  static nsPoint ToAppUnits(const gfx::PointTyped<CSSPixel>& aPoint) {
    return nsPoint(NSFloatPixelsToAppUnits(aPoint.x, float(nsDeviceContext::AppUnitsPerCSSPixel())),
                   NSFloatPixelsToAppUnits(aPoint.y, float(nsDeviceContext::AppUnitsPerCSSPixel())));
  }

  static gfx::RectTyped<CSSPixel> FromAppUnits(const nsRect& aRect) {
    return gfx::RectTyped<CSSPixel>(NSAppUnitsToFloatPixels(aRect.x, float(nsDeviceContext::AppUnitsPerCSSPixel())),
                                    NSAppUnitsToFloatPixels(aRect.y, float(nsDeviceContext::AppUnitsPerCSSPixel())),
                                    NSAppUnitsToFloatPixels(aRect.width, float(nsDeviceContext::AppUnitsPerCSSPixel())),
                                    NSAppUnitsToFloatPixels(aRect.height, float(nsDeviceContext::AppUnitsPerCSSPixel())));
  }

  static nsRect ToAppUnits(const gfx::RectTyped<CSSPixel>& aRect) {
    return nsRect(NSFloatPixelsToAppUnits(aRect.x, float(nsDeviceContext::AppUnitsPerCSSPixel())),
                  NSFloatPixelsToAppUnits(aRect.y, float(nsDeviceContext::AppUnitsPerCSSPixel())),
                  NSFloatPixelsToAppUnits(aRect.width, float(nsDeviceContext::AppUnitsPerCSSPixel())),
                  NSFloatPixelsToAppUnits(aRect.height, float(nsDeviceContext::AppUnitsPerCSSPixel())));
  }
};

typedef gfx::PointTyped<CSSPixel> CSSPoint;
typedef gfx::IntPointTyped<CSSPixel> CSSIntPoint;
typedef gfx::SizeTyped<CSSPixel> CSSSize;
typedef gfx::RectTyped<CSSPixel> CSSRect;
typedef gfx::IntRectTyped<CSSPixel> CSSIntRect;









struct LayerPixel {
  static gfx::IntPointTyped<LayerPixel> FromCSSPointRounded(const CSSPoint& aPoint, float aResolutionX, float aResolutionY) {
    return gfx::IntPointTyped<LayerPixel>(NS_lround(aPoint.x * aResolutionX),
                                          NS_lround(aPoint.y * aResolutionY));
  }

  static gfx::IntRectTyped<LayerPixel> RoundToInt(const gfx::RectTyped<LayerPixel>& aRect) {
    return gfx::IntRectTyped<LayerPixel>(NS_lround(aRect.x),
                                         NS_lround(aRect.y),
                                         NS_lround(aRect.width),
                                         NS_lround(aRect.height));
  }

  static gfx::RectTyped<LayerPixel> FromCSSRect(const CSSRect& aRect, float aResolutionX, float aResolutionY) {
    return gfx::RectTyped<LayerPixel>(aRect.x * aResolutionX,
                                      aRect.y * aResolutionY,
                                      aRect.width * aResolutionX,
                                      aRect.height * aResolutionY);
  }

  static gfx::IntRectTyped<LayerPixel> FromCSSRectRounded(const CSSRect& aRect, float aResolutionX, float aResolutionY) {
    return RoundToInt(FromCSSRect(aRect, aResolutionX, aResolutionY));
  }

  static CSSIntRect ToCSSIntRectRoundIn(const gfx::IntRectTyped<LayerPixel>& aRect, float aResolutionX, float aResolutionY) {
    gfx::IntRectTyped<CSSPixel> ret(aRect.x, aRect.y, aRect.width, aRect.height);
    ret.ScaleInverseRoundIn(aResolutionX, aResolutionY);
    return ret;
  }
};

typedef gfx::PointTyped<LayerPixel> LayerPoint;
typedef gfx::IntPointTyped<LayerPixel> LayerIntPoint;
typedef gfx::IntSizeTyped<LayerPixel> LayerIntSize;
typedef gfx::RectTyped<LayerPixel> LayerRect;
typedef gfx::IntRectTyped<LayerPixel> LayerIntRect;










struct ScreenPixel {
};

typedef gfx::PointTyped<ScreenPixel> ScreenPoint;

};

#endif
