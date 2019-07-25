




































#ifndef MOZILLA_GFX_HELPERSD2D_H_
#define MOZILLA_GFX_HELPERSD2D_H_

#include <d2d1.h>
#include "2D.h"

namespace mozilla {
namespace gfx {

static inline D2D1_POINT_2F D2DPoint(const Point &aPoint)
{
  return D2D1::Point2F(aPoint.x, aPoint.y);
}

static inline D2D1_SIZE_U D2DIntSize(const IntSize &aSize)
{
  return D2D1::SizeU(aSize.width, aSize.height);
}

static inline D2D1_RECT_F D2DRect(const Rect &aRect)
{
  return D2D1::RectF(aRect.x, aRect.y, aRect.XMost(), aRect.YMost());
}

static inline D2D1_EXTEND_MODE D2DExtend(ExtendMode aExtendMode)
{
  D2D1_EXTEND_MODE extend = D2D1_EXTEND_MODE_CLAMP;
  switch (aExtendMode) {
  case EXTEND_REPEAT:
    extend = D2D1_EXTEND_MODE_WRAP;
    break;
  case EXTEND_REFLECT:
    extend = D2D1_EXTEND_MODE_MIRROR;
    break;
  }

  return extend;
}

static inline D2D1_BITMAP_INTERPOLATION_MODE D2DFilter(const Filter &aFilter)
{
  switch (aFilter) {
  case FILTER_POINT:
    return D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR;
  }

  return D2D1_BITMAP_INTERPOLATION_MODE_LINEAR;
}

static inline D2D1_ANTIALIAS_MODE D2DAAMode(AntialiasMode aMode)
{
  switch (aMode) {
  case AA_NONE:
    return D2D1_ANTIALIAS_MODE_ALIASED;
  }

  return D2D1_ANTIALIAS_MODE_PER_PRIMITIVE;
}

static inline D2D1_MATRIX_3X2_F D2DMatrix(const Matrix &aTransform)
{
  return D2D1::Matrix3x2F(aTransform._11, aTransform._12,
                          aTransform._21, aTransform._22,
                          aTransform._31, aTransform._32);
}

static inline D2D1_COLOR_F D2DColor(const Color &aColor)
{
  return D2D1::ColorF(aColor.r, aColor.g, aColor.b, aColor.a);
}

static inline IntSize ToIntSize(const D2D1_SIZE_U &aSize)
{
  return IntSize(aSize.width, aSize.height);
}

static inline SurfaceFormat ToPixelFormat(const D2D1_PIXEL_FORMAT &aFormat)
{
  switch(aFormat.format) {
  case DXGI_FORMAT_A8_UNORM:
    return FORMAT_A8;
  case DXGI_FORMAT_B8G8R8A8_UNORM:
    if (aFormat.alphaMode == D2D1_ALPHA_MODE_IGNORE) {
      return FORMAT_B8G8R8X8;
    } else {
      return FORMAT_B8G8R8A8;
    }
  }

  return FORMAT_B8G8R8A8;
}

static inline Rect ToRect(const D2D1_RECT_F &aRect)
{
  return Rect(aRect.left, aRect.top, aRect.right - aRect.left, aRect.bottom - aRect.top);
}

static inline DXGI_FORMAT DXGIFormat(SurfaceFormat aFormat)
{
  switch (aFormat) {
  case FORMAT_B8G8R8A8:
    return DXGI_FORMAT_B8G8R8A8_UNORM;
  case FORMAT_B8G8R8X8:
    return DXGI_FORMAT_B8G8R8A8_UNORM;
  case FORMAT_A8:
    return DXGI_FORMAT_A8_UNORM;
  }

  return DXGI_FORMAT_UNKNOWN;
}

static inline D2D1_ALPHA_MODE AlphaMode(SurfaceFormat aFormat)
{
  switch (aFormat) {
  case FORMAT_B8G8R8X8:
    return D2D1_ALPHA_MODE_IGNORE;
  }

  return D2D1_ALPHA_MODE_PREMULTIPLIED;
}

static inline D2D1_PIXEL_FORMAT D2DPixelFormat(SurfaceFormat aFormat)
{
  return D2D1::PixelFormat(DXGIFormat(aFormat), AlphaMode(aFormat));
}

static inline int BytesPerPixel(SurfaceFormat aFormat)
{
  switch (aFormat) {
  case FORMAT_A8:
    return 1;
  default:
    return 4;
  }
}

static bool IsPatternSupportedByD2D(const Pattern &aPattern)
{
  if (aPattern.GetType() != PATTERN_RADIAL_GRADIENT) {
    return false;
  }

  const RadialGradientPattern *pat =
    static_cast<const RadialGradientPattern*>(&aPattern);
  
  if (pat->mRadius1 != 0) {
    return true;
  }

  Point diff = pat->mCenter2 - pat->mCenter1;

  if (sqrt(diff.x * diff.x + diff.y * diff.y) >= pat->mRadius2) {
    
    return true;
  }

  return false;
}








struct ShaderConstantRectD3D10
{
  float mX, mY, mWidth, mHeight;
  ShaderConstantRectD3D10(float aX, float aY, float aWidth, float aHeight)
    : mX(aX), mY(aY), mWidth(aWidth), mHeight(aHeight)
  { }

  
  operator float* () { return &mX; }
};

}
}

#endif 
