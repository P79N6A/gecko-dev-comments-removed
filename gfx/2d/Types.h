




































#ifndef MOZILLA_GFX_TYPES_H_
#define MOZILLA_GFX_TYPES_H_

#include "mozilla/StdInt.h"

#include <stddef.h>

namespace mozilla {
namespace gfx {

typedef float Float;

enum SurfaceType
{
  SURFACE_DATA, 
  SURFACE_D2D1_BITMAP, 
  SURFACE_D2D1_DRAWTARGET, 
  SURFACE_CAIRO, 
  SURFACE_CAIRO_IMAGE, 
  SURFACE_COREGRAPHICS_IMAGE, 
  SURFACE_SKIA 
};

enum SurfaceFormat
{
  FORMAT_B8G8R8A8,
  FORMAT_B8G8R8X8,
  FORMAT_R5G6B5,
  FORMAT_A8
};

enum BackendType
{
  BACKEND_DIRECT2D,
  BACKEND_COREGRAPHICS,
  BACKEND_CAIRO,
  BACKEND_SKIA
};

enum FontType
{
  FONT_DWRITE,
  FONT_GDI,
  FONT_MAC,
  FONT_SKIA,
  FONT_CAIRO,
  FONT_COREGRAPHICS
};

enum NativeSurfaceType
{
  NATIVE_SURFACE_D3D10_TEXTURE,
  NATIVE_SURFACE_CAIRO_SURFACE,
  NATIVE_SURFACE_CGCONTEXT
};

enum NativeFontType
{
  NATIVE_FONT_DWRITE_FONT_FACE,
  NATIVE_FONT_GDI_FONT_FACE,
  NATIVE_FONT_MAC_FONT_FACE,
  NATIVE_FONT_SKIA_FONT_FACE,
  NATIVE_FONT_CAIRO_FONT_FACE
};

enum CompositionOp { OP_OVER, OP_ADD, OP_ATOP, OP_OUT, OP_IN, OP_SOURCE, OP_DEST_IN, OP_DEST_OUT, OP_DEST_OVER, OP_DEST_ATOP, OP_XOR, OP_COUNT };
enum ExtendMode { EXTEND_CLAMP, EXTEND_REPEAT, EXTEND_REFLECT };
enum FillRule { FILL_WINDING, FILL_EVEN_ODD };
enum AntialiasMode { AA_NONE, AA_GRAY, AA_SUBPIXEL };
enum Snapping { SNAP_NONE, SNAP_ALIGNED };
enum Filter { FILTER_LINEAR, FILTER_POINT };
enum PatternType { PATTERN_COLOR, PATTERN_SURFACE, PATTERN_LINEAR_GRADIENT, PATTERN_RADIAL_GRADIENT };
enum JoinStyle { JOIN_BEVEL, JOIN_ROUND, JOIN_MITER, JOIN_MITER_OR_BEVEL };
enum CapStyle { CAP_BUTT, CAP_ROUND, CAP_SQUARE };
enum SamplingBounds { SAMPLING_UNBOUNDED, SAMPLING_BOUNDED };


struct Color
{
public:
  Color()
    : r(0.0f), g(0.0f), b(0.0f), a(0.0f)
  {}
  Color(Float aR, Float aG, Float aB, Float aA)
    : r(aR), g(aG), b(aB), a(aA)
  {}
  Color(Float aR, Float aG, Float aB)
    : r(aR), g(aG), b(aB), a(1.0f)
  {}

  static Color FromABGR(uint32_t aColor)
  {
    Color newColor(((aColor >> 0) & 0xff) * (1.0f / 255.0f),
                   ((aColor >> 8) & 0xff) * (1.0f / 255.0f),
                   ((aColor >> 16) & 0xff) * (1.0f / 255.0f),
                   ((aColor >> 24) & 0xff) * (1.0f / 255.0f));

    return newColor;
  }

  Float r, g, b, a;
};

struct GradientStop
{
  bool operator<(const GradientStop& aOther) const {
    return offset < aOther.offset;
  }

  Float offset;
  Color color;
};

}
}


namespace mozilla {
  namespace css {
    enum Side {eSideTop, eSideRight, eSideBottom, eSideLeft};
  }
}


#define NS_SIDE_TOP     mozilla::css::eSideTop
#define NS_SIDE_RIGHT   mozilla::css::eSideRight
#define NS_SIDE_BOTTOM  mozilla::css::eSideBottom
#define NS_SIDE_LEFT    mozilla::css::eSideLeft

#endif 
