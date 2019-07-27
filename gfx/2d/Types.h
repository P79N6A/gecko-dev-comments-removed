




#ifndef MOZILLA_GFX_TYPES_H_
#define MOZILLA_GFX_TYPES_H_

#include <stddef.h>
#include <stdint.h>

namespace mozilla {
namespace gfx {

typedef float Float;

enum class SurfaceType : int8_t {
  DATA, 
  D2D1_BITMAP, 
  D2D1_DRAWTARGET, 
  CAIRO, 
  CAIRO_IMAGE, 
  COREGRAPHICS_IMAGE, 
  COREGRAPHICS_CGCONTEXT, 
  SKIA, 
  DUAL_DT, 
  D2D1_1_IMAGE, 
  RECORDING, 
  TILED 
};

enum class SurfaceFormat : int8_t {
  B8G8R8A8,
  B8G8R8X8,
  R8G8B8A8,
  R8G8B8X8,
  R5G6B5,
  A8,
  YUV,
  UNKNOWN
};

inline bool IsOpaque(SurfaceFormat aFormat)
{
  switch (aFormat) {
  case SurfaceFormat::B8G8R8X8:
  case SurfaceFormat::R8G8B8X8:
  case SurfaceFormat::R5G6B5:
  case SurfaceFormat::YUV:
    return true;
  default:
    return false;
  }
}

enum class FilterType : int8_t {
  BLEND = 0,
  TRANSFORM,
  MORPHOLOGY,
  COLOR_MATRIX,
  FLOOD,
  TILE,
  TABLE_TRANSFER,
  DISCRETE_TRANSFER,
  LINEAR_TRANSFER,
  GAMMA_TRANSFER,
  CONVOLVE_MATRIX,
  DISPLACEMENT_MAP,
  TURBULENCE,
  ARITHMETIC_COMBINE,
  COMPOSITE,
  DIRECTIONAL_BLUR,
  GAUSSIAN_BLUR,
  POINT_DIFFUSE,
  POINT_SPECULAR,
  SPOT_DIFFUSE,
  SPOT_SPECULAR,
  DISTANT_DIFFUSE,
  DISTANT_SPECULAR,
  CROP,
  PREMULTIPLY,
  UNPREMULTIPLY
};

enum class DrawTargetType : int8_t {
  SOFTWARE_RASTER = 0,
  HARDWARE_RASTER,
  VECTOR
};

enum class BackendType : int8_t {
  NONE = 0,
  DIRECT2D,
  COREGRAPHICS,
  COREGRAPHICS_ACCELERATED,
  CAIRO,
  SKIA,
  RECORDING,
  DIRECT2D1_1
};

enum class FontType : int8_t {
  DWRITE,
  GDI,
  MAC,
  SKIA,
  CAIRO,
  COREGRAPHICS
};

enum class NativeSurfaceType : int8_t {
  D3D10_TEXTURE,
  CAIRO_SURFACE,
  CAIRO_CONTEXT,
  CGCONTEXT,
  CGCONTEXT_ACCELERATED,
  OPENGL_TEXTURE
};

enum class NativeFontType : int8_t {
  DWRITE_FONT_FACE,
  GDI_FONT_FACE,
  MAC_FONT_FACE,
  SKIA_FONT_FACE,
  CAIRO_FONT_FACE
};

enum class FontStyle : int8_t {
  NORMAL,
  ITALIC,
  BOLD,
  BOLD_ITALIC
};

enum class FontHinting : int8_t {
  NONE,
  LIGHT,
  NORMAL,
  FULL
};

enum class CompositionOp : int8_t {
  OP_OVER,
  OP_ADD,
  OP_ATOP,
  OP_OUT,
  OP_IN,
  OP_SOURCE,
  OP_DEST_IN,
  OP_DEST_OUT,
  OP_DEST_OVER,
  OP_DEST_ATOP,
  OP_XOR,
  OP_MULTIPLY,
  OP_SCREEN,
  OP_OVERLAY,
  OP_DARKEN,
  OP_LIGHTEN,
  OP_COLOR_DODGE,
  OP_COLOR_BURN,
  OP_HARD_LIGHT,
  OP_SOFT_LIGHT,
  OP_DIFFERENCE,
  OP_EXCLUSION,
  OP_HUE,
  OP_SATURATION,
  OP_COLOR,
  OP_LUMINOSITY,
  OP_COUNT
};

enum class ExtendMode : int8_t {
  CLAMP,
  REPEAT,
  REFLECT
};

enum class FillRule : int8_t {
  FILL_WINDING,
  FILL_EVEN_ODD
};

enum class AntialiasMode : int8_t {
  NONE,
  GRAY,
  SUBPIXEL,
  DEFAULT
};

enum class Filter : int8_t {
  GOOD,
  LINEAR,
  POINT
};

enum class PatternType : int8_t {
  COLOR,
  SURFACE,
  LINEAR_GRADIENT,
  RADIAL_GRADIENT
};

enum class JoinStyle : int8_t {
  BEVEL,
  ROUND,
  MITER, 
         
         
  MITER_OR_BEVEL 
};

enum class CapStyle : int8_t {
  BUTT,
  ROUND,
  SQUARE
};

enum class SamplingBounds : int8_t {
  UNBOUNDED,
  BOUNDED
};


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

  static Color FromARGB(uint32_t aColor)
  {
    Color newColor(((aColor >> 16) & 0xff) * (1.0f / 255.0f),
                   ((aColor >> 8) & 0xff) * (1.0f / 255.0f),
                   ((aColor >> 0) & 0xff) * (1.0f / 255.0f),
                   ((aColor >> 24) & 0xff) * (1.0f / 255.0f));

    return newColor;
  }

  uint32_t ToABGR() const
  {
    return uint32_t(r * 255.0f) | uint32_t(g * 255.0f) << 8 |
           uint32_t(b * 255.0f) << 16 | uint32_t(a * 255.0f) << 24;
  }

  uint32_t ToARGB() const
  {
    return uint32_t(b * 255.0f) | uint32_t(g * 255.0f) << 8 |
           uint32_t(r * 255.0f) << 16 | uint32_t(a * 255.0f) << 24;
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




struct RectCorner {
  enum {
    
    
    TopLeft = 0,
    TopRight = 1,
    BottomRight = 2,
    BottomLeft = 3,
    Count = 4
  };
};


enum Side { eSideTop, eSideRight, eSideBottom, eSideLeft };

enum SideBits {
  eSideBitsNone   = 0,
  eSideBitsTop    = 1 << eSideTop,
  eSideBitsRight  = 1 << eSideRight,
  eSideBitsBottom = 1 << eSideBottom,
  eSideBitsLeft   = 1 << eSideLeft,
  eSideBitsTopBottom = eSideBitsTop  | eSideBitsBottom,
  eSideBitsLeftRight = eSideBitsLeft | eSideBitsRight,
  eSideBitsAll = eSideBitsTopBottom | eSideBitsLeftRight
};

} 

#define NS_SIDE_TOP    mozilla::eSideTop
#define NS_SIDE_RIGHT  mozilla::eSideRight
#define NS_SIDE_BOTTOM mozilla::eSideBottom
#define NS_SIDE_LEFT   mozilla::eSideLeft

#endif
