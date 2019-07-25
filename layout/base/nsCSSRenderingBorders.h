






































#ifndef NS_CSS_RENDERING_BORDERS_H
#define NS_CSS_RENDERING_BORDERS_H

#include "nsColor.h"
#include "nsStyleStruct.h"

#include "gfxContext.h"


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
  nsCSSBorderRenderer(PRInt32 aAppUnitsPerPixel,
                      gfxContext* aDestContext,
                      gfxRect& aOuterRect,
                      const PRUint8* aBorderStyles,
                      const gfxFloat* aBorderWidths,
                      gfxCornerSizes& aBorderRadii,
                      const nscolor* aBorderColors,
                      nsBorderColors* const* aCompositeColors,
                      PRIntn aSkipSides,
                      nscolor aBackgroundColor);

  gfxCornerSizes mBorderCornerDimensions;

  
  gfxContext* mContext;

  
  gfxRect mOuterRect;
  gfxRect mInnerRect;

  
  const PRUint8* mBorderStyles;
  const gfxFloat* mBorderWidths;
  PRUint8* mSanitizedStyles;
  gfxFloat* mSanitizedWidths;
  gfxCornerSizes mBorderRadii;

  
  const nscolor* mBorderColors;
  nsBorderColors* const* mCompositeColors;

  
  PRInt32 mAUPP;

  
  PRIntn mSkipSides;
  nscolor mBackgroundColor;

  
  PRPackedBool mOneUnitBorder;
  PRPackedBool mNoBorderRadius;
  PRPackedBool mAvoidStroke;

  
  
  PRBool AreBorderSideFinalStylesSame(PRUint8 aSides);

  
  PRBool IsSolidCornerStyle(PRUint8 aStyle, mozilla::css::Corner aCorner);

  
  BorderColorStyle BorderColorStyleForSolidCorner(PRUint8 aStyle, mozilla::css::Corner aCorner);

  
  
  

  
  void DoCornerSubPath(mozilla::css::Corner aCorner);
  
  void DoSideClipWithoutCornersSubPath(mozilla::css::Side aSide);

  
  
  
  
  
  
  
  
  void DoSideClipSubPath(mozilla::css::Side aSide);

  
  
  
  
  
  
  
  
  
  
  
  
  void FillSolidBorder(const gfxRect& aOuterRect,
                       const gfxRect& aInnerRect,
                       const gfxCornerSizes& aBorderRadii,
                       const gfxFloat *aBorderSizes,
                       PRIntn aSides,
                       const gfxRGBA& aColor);

  
  
  

  
  
  void DrawBorderSides (PRIntn aSides);

  
  void DrawBorderSidesCompositeColors(PRIntn aSides, const nsBorderColors *compositeColors);

  
  void DrawDashedSide (mozilla::css::Side aSide);
  
  
  void SetupStrokeStyle(mozilla::css::Side aSize);

  
  bool AllBordersSameWidth();

  
  
  
  bool AllBordersSolid(bool *aHasCompositeColors);

  
  
  already_AddRefed<gfxPattern> CreateCornerGradient(mozilla::css::Corner aCorner,
                                                    const gfxRGBA &aFirstColor,
                                                    const gfxRGBA &aSecondColor);

  
  void DrawSingleWidthSolidBorder();

  
  
  void DrawNoCompositeColorSolidBorder();

  
  
  void DrawRectangularCompositeColors();

  
  void DrawBorders ();

  
  static void ComputeInnerRadii(const gfxCornerSizes& aRadii,
                                const gfxFloat *aBorderSizes,
                                gfxCornerSizes *aInnerRadiiRet);
};

#ifdef DEBUG_NEW_BORDERS
#include <stdarg.h>

static inline void S(const gfxPoint& p) {
  fprintf (stderr, "[%f,%f]", p.x, p.y);
}

static inline void S(const gfxSize& s) {
  fprintf (stderr, "[%f %f]", s.width, s.height);
}

static inline void S(const gfxRect& r) {
  fprintf (stderr, "[%f %f %f %f]", r.pos.x, r.pos.y, r.size.width, r.size.height);
}

static inline void S(const gfxFloat f) {
  fprintf (stderr, "%f", f);
}

static inline void S(const char *s) {
  fprintf (stderr, "%s", s);
}

static inline void SN(const char *s = nsnull) {
  if (s)
    fprintf (stderr, "%s", s);
  fprintf (stderr, "\n");
  fflush (stderr);
}

static inline void SF(const char *fmt, ...) {
  va_list vl;
  va_start(vl, fmt);
  vfprintf (stderr, fmt, vl);
  va_end(vl);
}

static inline void SX(gfxContext *ctx) {
  gfxPoint p = ctx->CurrentPoint();
  fprintf (stderr, "p: %f %f\n", p.x, p.y);
  return;
  ctx->MoveTo(p + gfxPoint(-2, -2)); ctx->LineTo(p + gfxPoint(2, 2));
  ctx->MoveTo(p + gfxPoint(-2, 2)); ctx->LineTo(p + gfxPoint(2, -2));
  ctx->MoveTo(p);
}


#else
static inline void S(const gfxPoint& p) {}
static inline void S(const gfxSize& s) {}
static inline void S(const gfxRect& r) {}
static inline void S(const gfxFloat f) {}
static inline void S(const char *s) {}
static inline void SN(const char *s = nsnull) {}
static inline void SF(const char *fmt, ...) {}
static inline void SX(gfxContext *ctx) {}
#endif

#endif 
