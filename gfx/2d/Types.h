




#ifndef MOZILLA_GFX_TYPES_H_
#define MOZILLA_GFX_TYPES_H_

#include "mozilla/StandardInteger.h"
#include "mozilla/NullPtr.h"

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
  SURFACE_COREGRAPHICS_CGCONTEXT, 
  SURFACE_SKIA, 
  SURFACE_DUAL_DT, 
  SURFACE_RECORDING 
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
  BACKEND_NONE = 0,
  BACKEND_DIRECT2D,
  BACKEND_COREGRAPHICS,
  BACKEND_COREGRAPHICS_ACCELERATED,
  BACKEND_CAIRO,
  BACKEND_SKIA,
  BACKEND_RECORDING
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
  NATIVE_SURFACE_CGCONTEXT,
  NATIVE_SURFACE_CGCONTEXT_ACCELERATED
};

enum NativeFontType
{
  NATIVE_FONT_DWRITE_FONT_FACE,
  NATIVE_FONT_GDI_FONT_FACE,
  NATIVE_FONT_MAC_FONT_FACE,
  NATIVE_FONT_SKIA_FONT_FACE,
  NATIVE_FONT_CAIRO_FONT_FACE
};

enum FontStyle
{
  FONT_STYLE_NORMAL,
  FONT_STYLE_ITALIC,
  FONT_STYLE_BOLD,
  FONT_STYLE_BOLD_ITALIC
};

enum CompositionOp { OP_OVER, OP_ADD, OP_ATOP, OP_OUT, OP_IN, OP_SOURCE, OP_DEST_IN, OP_DEST_OUT, OP_DEST_OVER, OP_DEST_ATOP, OP_XOR, 
  OP_MULTIPLY, OP_SCREEN, OP_OVERLAY, OP_DARKEN, OP_LIGHTEN, OP_COLOR_DODGE, OP_COLOR_BURN, OP_HARD_LIGHT, OP_SOFT_LIGHT,  OP_DIFFERENCE, OP_EXCLUSION, OP_HUE, OP_SATURATION, OP_COLOR, OP_LUMINOSITY, OP_COUNT };
enum ExtendMode { EXTEND_CLAMP, EXTEND_REPEAT, EXTEND_REFLECT };
enum FillRule { FILL_WINDING, FILL_EVEN_ODD };
enum AntialiasMode { AA_NONE, AA_GRAY, AA_SUBPIXEL, AA_DEFAULT };
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

  uint32_t ToABGR() const
  {
    return uint32_t(r * 255.0f) | uint32_t(g * 255.0f) << 8 |
           uint32_t(b * 255.0f) << 16 | uint32_t(a * 255.0f) << 24;
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

#if defined(XP_WIN) && defined(MOZ_GFX)
#ifdef GFX2D_INTERNAL
#define GFX2D_API __declspec(dllexport)
#else
#define GFX2D_API __declspec(dllimport)
#endif
#else
#define GFX2D_API
#endif


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
