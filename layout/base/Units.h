





#ifndef MOZ_UNITS_H_
#define MOZ_UNITS_H_

#include "mozilla/gfx/Point.h"
#include "mozilla/gfx/Rect.h"
#include "mozilla/gfx/ScaleFactor.h"
#include "nsDeviceContext.h"

namespace mozilla {

struct CSSPixel;
struct LayerPixel;
struct ScreenPixel;

typedef gfx::PointTyped<CSSPixel> CSSPoint;
typedef gfx::IntPointTyped<CSSPixel> CSSIntPoint;
typedef gfx::SizeTyped<CSSPixel> CSSSize;
typedef gfx::IntSizeTyped<CSSPixel> CSSIntSize;
typedef gfx::RectTyped<CSSPixel> CSSRect;
typedef gfx::IntRectTyped<CSSPixel> CSSIntRect;

typedef gfx::PointTyped<LayerPixel> LayerPoint;
typedef gfx::IntPointTyped<LayerPixel> LayerIntPoint;
typedef gfx::SizeTyped<LayerPixel> LayerSize;
typedef gfx::IntSizeTyped<LayerPixel> LayerIntSize;
typedef gfx::RectTyped<LayerPixel> LayerRect;
typedef gfx::IntRectTyped<LayerPixel> LayerIntRect;

typedef gfx::PointTyped<ScreenPixel> ScreenPoint;
typedef gfx::IntPointTyped<ScreenPixel> ScreenIntPoint;
typedef gfx::SizeTyped<ScreenPixel> ScreenSize;
typedef gfx::IntSizeTyped<ScreenPixel> ScreenIntSize;
typedef gfx::RectTyped<ScreenPixel> ScreenRect;
typedef gfx::IntRectTyped<ScreenPixel> ScreenIntRect;

typedef gfx::ScaleFactor<CSSPixel, LayerPixel> CSSToLayerScale;
typedef gfx::ScaleFactor<LayerPixel, CSSPixel> LayerToCSSScale;
typedef gfx::ScaleFactor<CSSPixel, ScreenPixel> CSSToScreenScale;
typedef gfx::ScaleFactor<ScreenPixel, CSSPixel> ScreenToCSSScale;
typedef gfx::ScaleFactor<LayerPixel, ScreenPixel> LayerToScreenScale;
typedef gfx::ScaleFactor<ScreenPixel, LayerPixel> ScreenToLayerScale;




struct CSSPixel {

  

  static CSSPoint FromAppUnits(const nsPoint& aPoint) {
    return CSSPoint(NSAppUnitsToFloatPixels(aPoint.x, float(nsDeviceContext::AppUnitsPerCSSPixel())),
                    NSAppUnitsToFloatPixels(aPoint.y, float(nsDeviceContext::AppUnitsPerCSSPixel())));
  }

  static CSSRect FromAppUnits(const nsRect& aRect) {
    return CSSRect(NSAppUnitsToFloatPixels(aRect.x, float(nsDeviceContext::AppUnitsPerCSSPixel())),
                   NSAppUnitsToFloatPixels(aRect.y, float(nsDeviceContext::AppUnitsPerCSSPixel())),
                   NSAppUnitsToFloatPixels(aRect.width, float(nsDeviceContext::AppUnitsPerCSSPixel())),
                   NSAppUnitsToFloatPixels(aRect.height, float(nsDeviceContext::AppUnitsPerCSSPixel())));
  }

  static CSSIntPoint FromAppUnitsRounded(const nsPoint& aPoint) {
    return CSSIntPoint(NSAppUnitsToIntPixels(aPoint.x, float(nsDeviceContext::AppUnitsPerCSSPixel())),
                       NSAppUnitsToIntPixels(aPoint.y, float(nsDeviceContext::AppUnitsPerCSSPixel())));
  }

  static CSSIntRect FromAppUnitsRounded(const nsRect& aRect) {
    return CSSIntRect(NSAppUnitsToIntPixels(aRect.x, float(nsDeviceContext::AppUnitsPerCSSPixel())),
                      NSAppUnitsToIntPixels(aRect.y, float(nsDeviceContext::AppUnitsPerCSSPixel())),
                      NSAppUnitsToIntPixels(aRect.width, float(nsDeviceContext::AppUnitsPerCSSPixel())),
                      NSAppUnitsToIntPixels(aRect.height, float(nsDeviceContext::AppUnitsPerCSSPixel())));
  }

  

  static nsPoint ToAppUnits(const CSSPoint& aPoint) {
    return nsPoint(NSFloatPixelsToAppUnits(aPoint.x, float(nsDeviceContext::AppUnitsPerCSSPixel())),
                   NSFloatPixelsToAppUnits(aPoint.y, float(nsDeviceContext::AppUnitsPerCSSPixel())));
  }

  static nsPoint ToAppUnits(const CSSIntPoint& aPoint) {
    return nsPoint(NSIntPixelsToAppUnits(aPoint.x, nsDeviceContext::AppUnitsPerCSSPixel()),
                   NSIntPixelsToAppUnits(aPoint.y, nsDeviceContext::AppUnitsPerCSSPixel()));
  }

  static nsRect ToAppUnits(const CSSRect& aRect) {
    return nsRect(NSFloatPixelsToAppUnits(aRect.x, float(nsDeviceContext::AppUnitsPerCSSPixel())),
                  NSFloatPixelsToAppUnits(aRect.y, float(nsDeviceContext::AppUnitsPerCSSPixel())),
                  NSFloatPixelsToAppUnits(aRect.width, float(nsDeviceContext::AppUnitsPerCSSPixel())),
                  NSFloatPixelsToAppUnits(aRect.height, float(nsDeviceContext::AppUnitsPerCSSPixel())));
  }
};









struct LayerPixel {

  

  static LayerIntPoint FromCSSPointRounded(const CSSPoint& aPoint, float aResolutionX, float aResolutionY) {
    return LayerIntPoint(NS_lround(aPoint.x * aResolutionX),
                         NS_lround(aPoint.y * aResolutionY));
  }

  static LayerRect FromCSSRect(const CSSRect& aRect, float aResolutionX, float aResolutionY) {
    return LayerRect(aRect.x * aResolutionX,
                     aRect.y * aResolutionY,
                     aRect.width * aResolutionX,
                     aRect.height * aResolutionY);
  }

  static LayerIntRect FromCSSRectRounded(const CSSRect& aRect, float aResolutionX, float aResolutionY) {
    return gfx::RoundedToInt(FromCSSRect(aRect, aResolutionX, aResolutionY));
  }
};










struct ScreenPixel {

  

  static CSSIntRect ToCSSIntRectRoundIn(const ScreenIntRect& aRect, float aResolutionX, float aResolutionY) {
    CSSIntRect ret(aRect.x, aRect.y, aRect.width, aRect.height);
    ret.ScaleInverseRoundIn(aResolutionX, aResolutionY);
    return ret;
  }
};



template<class src, class dst>
gfx::PointTyped<dst> operator*(const gfx::PointTyped<src>& aPoint, const gfx::ScaleFactor<src, dst>& aScale) {
  return gfx::PointTyped<dst>(aPoint.x * aScale.scale,
                              aPoint.y * aScale.scale);
}

template<class src, class dst>
gfx::PointTyped<dst> operator/(const gfx::PointTyped<src>& aPoint, const gfx::ScaleFactor<dst, src>& aScale) {
  return gfx::PointTyped<dst>(aPoint.x / aScale.scale,
                              aPoint.y / aScale.scale);
}

};

#endif
