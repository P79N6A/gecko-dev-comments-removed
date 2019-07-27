





#ifndef NS_CSS_RENDERING_BORDERS_H
#define NS_CSS_RENDERING_BORDERS_H

#include "gfxRect.h"
#include "mozilla/Attributes.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/gfx/PathHelpers.h"
#include "mozilla/RefPtr.h"
#include "nsColor.h"
#include "nsCOMPtr.h"
#include "nsStyleConsts.h"

struct nsBorderColors;

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

class nsCSSBorderRenderer final
{
  typedef mozilla::gfx::ColorPattern ColorPattern;
  typedef mozilla::gfx::DrawTarget DrawTarget;
  typedef mozilla::gfx::Float Float;
  typedef mozilla::gfx::Path Path;
  typedef mozilla::gfx::Rect Rect;
  typedef mozilla::gfx::RectCornerRadii RectCornerRadii;

public:

  nsCSSBorderRenderer(DrawTarget* aDrawTarget,
                      Rect& aOuterRect,
                      const uint8_t* aBorderStyles,
                      const Float* aBorderWidths,
                      RectCornerRadii& aBorderRadii,
                      const nscolor* aBorderColors,
                      nsBorderColors* const* aCompositeColors,
                      nscolor aBackgroundColor);

  
  void DrawBorders();

  
  static void ComputeInnerRadii(const RectCornerRadii& aRadii,
                                const Float* aBorderSizes,
                                RectCornerRadii* aInnerRadiiRet);

  
  
  
  
  static void ComputeOuterRadii(const RectCornerRadii& aRadii,
                                const Float* aBorderSizes,
                                RectCornerRadii* aOuterRadiiRet);

private:

  RectCornerRadii mBorderCornerDimensions;

  
  DrawTarget* mDrawTarget;

  
  Rect mOuterRect;
  Rect mInnerRect;

  
  const uint8_t* mBorderStyles;
  const Float* mBorderWidths;
  RectCornerRadii mBorderRadii;

  
  const nscolor* mBorderColors;
  nsBorderColors* const* mCompositeColors;

  
  nscolor mBackgroundColor;

  
  bool mOneUnitBorder;
  bool mNoBorderRadius;
  bool mAvoidStroke;

  
  
  bool AreBorderSideFinalStylesSame(uint8_t aSides);

  
  bool IsSolidCornerStyle(uint8_t aStyle, mozilla::css::Corner aCorner);

  
  BorderColorStyle BorderColorStyleForSolidCorner(uint8_t aStyle, mozilla::css::Corner aCorner);

  
  
  

  
  Rect GetCornerRect(mozilla::css::Corner aCorner);
  
  Rect GetSideClipWithoutCornersRect(mozilla::css::Side aSide);

  
  
  
  
  
  
  
  
  already_AddRefed<Path> GetSideClipSubPath(mozilla::css::Side aSide);

  
  
  
  
  
  
  
  
  
  
  
  
  void FillSolidBorder(const Rect& aOuterRect,
                       const Rect& aInnerRect,
                       const RectCornerRadii& aBorderRadii,
                       const Float* aBorderSizes,
                       int aSides,
                       const ColorPattern& aColor);

  
  
  

  
  
  void DrawBorderSides (int aSides);

  
  void DrawBorderSidesCompositeColors(int aSides, const nsBorderColors *compositeColors);

  
  void DrawDashedSide (mozilla::css::Side aSide);

  
  void SetupStrokeStyle(mozilla::css::Side aSize);

  
  bool AllBordersSameWidth();

  
  
  
  bool AllBordersSolid(bool *aHasCompositeColors);

  
  already_AddRefed<mozilla::gfx::GradientStops>
  CreateCornerGradient(mozilla::css::Corner aCorner, nscolor aFirstColor,
                       nscolor aSecondColor, mozilla::gfx::DrawTarget *aDT,
                       mozilla::gfx::Point &aPoint1, mozilla::gfx::Point &aPoint2);

  
  void DrawSingleWidthSolidBorder();

  
  
  void DrawNoCompositeColorSolidBorder();

  
  
  void DrawRectangularCompositeColors();
};

namespace mozilla {
#ifdef DEBUG_NEW_BORDERS
#include <stdarg.h>

static inline void PrintAsString(const mozilla::gfx::Point& p) {
  fprintf (stderr, "[%f,%f]", p.x, p.y);
}

static inline void PrintAsString(const mozilla::gfx::Size& s) {
  fprintf (stderr, "[%f %f]", s.width, s.height);
}

static inline void PrintAsString(const mozilla::gfx::Rect& r) {
  fprintf (stderr, "[%f %f %f %f]", r.X(), r.Y(), r.Width(), r.Height());
}

static inline void PrintAsString(const mozilla::gfx::Float f) {
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

#else
static inline void PrintAsString(const mozilla::gfx::Point& p) {}
static inline void PrintAsString(const mozilla::gfx::Size& s) {}
static inline void PrintAsString(const mozilla::gfx::Rect& r) {}
static inline void PrintAsString(const mozilla::gfx::Float f) {}
static inline void PrintAsString(const char *s) {}
static inline void PrintAsStringNewline(const char *s = nullptr) {}
static inline void PrintAsFormatString(const char *fmt, ...) {}
#endif

}

#endif 
