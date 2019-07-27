





#ifndef NS_CSS_RENDERING_BORDERS_H
#define NS_CSS_RENDERING_BORDERS_H

#include "nsColor.h"

class gfxContext;
class gfxPattern;
struct gfxRGBA;

namespace mozilla {
namespace gfx {
class GradientStops;
}
}


#undef DEBUG_NEW_BORDERS


#define DOT_LENGTH  1           //square
#define DASH_LENGTH 3           //3 times longer than dot


#define SIDE_BIT_TOP (1 << NS_SIDE_TOP)
#define SIDE_BIT_RIGHT (1 << NS_SIDE_RIGHT)
#define SIDE_BIT_BOTTOM (1 << NS_SIDE_BOTTOM)
#define SIDE_BIT_LEFT (1 << NS_SIDE_LEFT)
#define SIDE_BITS_ALL (SIDE_BIT_TOP|SIDE_BIT_RIGHT|SIDE_BIT_BOTTOM|SIDE_BIT_LEFT)

#define C_TL NS_CORNER_TOP_LEFT
#define C_TR NS_CORNER_TOP_RIGHT
#define C_BR NS_CORNER_BOTTOM_RIGHT
#define C_BL NS_CORNER_BOTTOM_LEFT


























typedef enum {
  BorderColorStyleNone,
  BorderColorStyleSolid,
  BorderColorStyleLight,
  BorderColorStyleDark
} BorderColorStyle;

struct nsCSSBorderRenderer {
  nsCSSBorderRenderer(int32_t aAppUnitsPerPixel,
                      gfxContext* aDestContext,
                      gfxRect& aOuterRect,
                      const uint8_t* aBorderStyles,
                      const gfxFloat* aBorderWidths,
                      gfxCornerSizes& aBorderRadii,
                      const nscolor* aBorderColors,
                      nsBorderColors* const* aCompositeColors,
                      nscolor aBackgroundColor);

  gfxCornerSizes mBorderCornerDimensions;

  
  gfxContext* mContext;

  
  gfxRect mOuterRect;
  gfxRect mInnerRect;

  
  const uint8_t* mBorderStyles;
  const gfxFloat* mBorderWidths;
  uint8_t* mSanitizedStyles;
  gfxFloat* mSanitizedWidths;
  gfxCornerSizes mBorderRadii;

  
  const nscolor* mBorderColors;
  nsBorderColors* const* mCompositeColors;

  
  int32_t mAUPP;

  
  nscolor mBackgroundColor;

  
  bool mOneUnitBorder;
  bool mNoBorderRadius;
  bool mAvoidStroke;

  
  
  bool AreBorderSideFinalStylesSame(uint8_t aSides);

  
  bool IsSolidCornerStyle(uint8_t aStyle, mozilla::css::Corner aCorner);

  
  BorderColorStyle BorderColorStyleForSolidCorner(uint8_t aStyle, mozilla::css::Corner aCorner);

  
  
  

  
  void DoCornerSubPath(mozilla::css::Corner aCorner);
  
  void DoSideClipWithoutCornersSubPath(mozilla::css::Side aSide);

  
  
  
  
  
  
  
  
  void DoSideClipSubPath(mozilla::css::Side aSide);

  
  
  
  
  
  
  
  
  
  
  
  
  void FillSolidBorder(const gfxRect& aOuterRect,
                       const gfxRect& aInnerRect,
                       const gfxCornerSizes& aBorderRadii,
                       const gfxFloat *aBorderSizes,
                       int aSides,
                       const gfxRGBA& aColor);

  
  
  

  
  
  void DrawBorderSides (int aSides);

  
  void DrawBorderSidesCompositeColors(int aSides, const nsBorderColors *compositeColors);

  
  void DrawDashedSide (mozilla::css::Side aSide);

  
  void SetupStrokeStyle(mozilla::css::Side aSize);

  
  bool AllBordersSameWidth();

  
  
  
  bool AllBordersSolid(bool *aHasCompositeColors);

  
  
  already_AddRefed<gfxPattern> CreateCornerGradient(mozilla::css::Corner aCorner,
                                                    const gfxRGBA &aFirstColor,
                                                    const gfxRGBA &aSecondColor);

  
  mozilla::TemporaryRef<mozilla::gfx::GradientStops>
  CreateCornerGradient(mozilla::css::Corner aCorner, const gfxRGBA &aFirstColor,
                       const gfxRGBA &aSecondColor, mozilla::gfx::DrawTarget *aDT,
                       mozilla::gfx::Point &aPoint1, mozilla::gfx::Point &aPoint2);

  
  void DrawSingleWidthSolidBorder();

  
  
  void DrawNoCompositeColorSolidBorder();

  
  
  void DrawRectangularCompositeColors();

  
  void DrawBorders ();

  
  static void ComputeInnerRadii(const gfxCornerSizes& aRadii,
                                const gfxFloat *aBorderSizes,
                                gfxCornerSizes *aInnerRadiiRet);

  
  
  
  
  static void ComputeOuterRadii(const gfxCornerSizes& aRadii,
                                const gfxFloat *aBorderSizes,
                                gfxCornerSizes *aOuterRadiiRet);
};

namespace mozilla {
#ifdef DEBUG_NEW_BORDERS
#include <stdarg.h>

static inline void PrintAsString(const gfxPoint& p) {
  fprintf (stderr, "[%f,%f]", p.x, p.y);
}

static inline void PrintAsString(const gfxSize& s) {
  fprintf (stderr, "[%f %f]", s.width, s.height);
}

static inline void PrintAsString(const gfxRect& r) {
  fprintf (stderr, "[%f %f %f %f]", r.X(), r.Y(), r.Width(), r.Height());
}

static inline void PrintAsString(const gfxFloat f) {
  fprintf (stderr, "%f", f);
}

static inline void PrintAsString(const char *s) {
  fprintf (stderr, "%s", s);
}

static inline void PrintAsStringNewline(const char *s = nullptr) {
  if (s)
    fprintf (stderr, "%s", s);
  fprintf (stderr, "\n");
  fflush (stderr);
}

static inline void PrintAsFormatString(const char *fmt, ...) {
  va_list vl;
  va_start(vl, fmt);
  vfprintf (stderr, fmt, vl);
  va_end(vl);
}

static inline void PrintGfxContext(gfxContext *ctx) {
  gfxPoint p = ctx->CurrentPoint();
  fprintf (stderr, "p: %f %f\n", p.x, p.y);
  return;
  ctx->MoveTo(p + gfxPoint(-2, -2)); ctx->LineTo(p + gfxPoint(2, 2));
  ctx->MoveTo(p + gfxPoint(-2, 2)); ctx->LineTo(p + gfxPoint(2, -2));
  ctx->MoveTo(p);
}


#else
static inline void PrintAsString(const gfxPoint& p) {}
static inline void PrintAsString(const gfxSize& s) {}
static inline void PrintAsString(const gfxRect& r) {}
static inline void PrintAsString(const gfxFloat f) {}
static inline void PrintAsString(const char *s) {}
static inline void PrintAsStringNewline(const char *s = nullptr) {}
static inline void PrintAsFormatString(const char *fmt, ...) {}
static inline void PrintGfxContext(gfxContext *ctx) {}
#endif

}

#endif 
