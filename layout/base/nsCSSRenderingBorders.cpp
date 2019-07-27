





#include "nsStyleConsts.h"
#include "nsCSSColorUtils.h"
#include "GeckoProfiler.h"
#include "nsExpirationTracker.h"
#include "RoundedRect.h"
#include "nsClassHashtable.h"
#include "nsStyleStruct.h"
#include "gfxContext.h"
#include "nsCSSRenderingBorders.h"
#include "mozilla/gfx/2D.h"
#include "gfx2DGlue.h"
#include "gfxGradientCache.h"
#include <algorithm>

using namespace mozilla;
using namespace mozilla::gfx;

























static void ComputeBorderCornerDimensions(const gfxRect& aOuterRect,
                                          const gfxRect& aInnerRect,
                                          const gfxCornerSizes& aRadii,
                                          gfxCornerSizes *aDimsResult);


#define NEXT_SIDE(_s) mozilla::css::Side(((_s) + 1) & 3)
#define PREV_SIDE(_s) mozilla::css::Side(((_s) + 3) & 3)



static gfxRGBA MakeBorderColor(const gfxRGBA& aColor,
                               const gfxRGBA& aBackgroundColor,
                               BorderColorStyle aBorderColorStyle);





static gfxRGBA ComputeColorForLine(uint32_t aLineIndex,
                                   const BorderColorStyle* aBorderColorStyle,
                                   uint32_t aBorderColorStyleCount,
                                   nscolor aBorderColor,
                                   nscolor aBackgroundColor);

static gfxRGBA ComputeCompositeColorForLine(uint32_t aLineIndex,
                                            const nsBorderColors* aBorderColors);



static bool
CheckFourFloatsEqual(const gfxFloat *vals, gfxFloat k)
{
  return (vals[0] == k &&
          vals[1] == k &&
          vals[2] == k &&
          vals[3] == k);
}

static bool
IsZeroSize(const gfxSize& sz) {
  return sz.width == 0.0 || sz.height == 0.0;
}

static bool
AllCornersZeroSize(const gfxCornerSizes& corners) {
  return IsZeroSize(corners[NS_CORNER_TOP_LEFT]) &&
    IsZeroSize(corners[NS_CORNER_TOP_RIGHT]) &&
    IsZeroSize(corners[NS_CORNER_BOTTOM_RIGHT]) &&
    IsZeroSize(corners[NS_CORNER_BOTTOM_LEFT]);
}

typedef enum {
  
  
  
  CORNER_NORMAL,

  
  
  CORNER_SOLID,

  
  
  CORNER_DOT
} CornerStyle;

nsCSSBorderRenderer::nsCSSBorderRenderer(int32_t aAppUnitsPerPixel,
                                         gfxContext* aDestContext,
                                         gfxRect& aOuterRect,
                                         const uint8_t* aBorderStyles,
                                         const gfxFloat* aBorderWidths,
                                         gfxCornerSizes& aBorderRadii,
                                         const nscolor* aBorderColors,
                                         nsBorderColors* const* aCompositeColors,
                                         nscolor aBackgroundColor)
  : mContext(aDestContext),
    mOuterRect(aOuterRect),
    mBorderStyles(aBorderStyles),
    mBorderWidths(aBorderWidths),
    mBorderRadii(aBorderRadii),
    mBorderColors(aBorderColors),
    mCompositeColors(aCompositeColors),
    mAUPP(aAppUnitsPerPixel),
    mBackgroundColor(aBackgroundColor)
{
  if (!mCompositeColors) {
    static nsBorderColors * const noColors[4] = { nullptr };
    mCompositeColors = &noColors[0];
  }

  mInnerRect = mOuterRect;
  mInnerRect.Deflate(
      gfxMargin(mBorderStyles[0] != NS_STYLE_BORDER_STYLE_NONE ? mBorderWidths[0] : 0,
                mBorderStyles[1] != NS_STYLE_BORDER_STYLE_NONE ? mBorderWidths[1] : 0,
                mBorderStyles[2] != NS_STYLE_BORDER_STYLE_NONE ? mBorderWidths[2] : 0,
                mBorderStyles[3] != NS_STYLE_BORDER_STYLE_NONE ? mBorderWidths[3] : 0));

  ComputeBorderCornerDimensions(mOuterRect, mInnerRect, mBorderRadii, &mBorderCornerDimensions);

  mOneUnitBorder = CheckFourFloatsEqual(mBorderWidths, 1.0);
  mNoBorderRadius = AllCornersZeroSize(mBorderRadii);
  mAvoidStroke = false;
}

 void
nsCSSBorderRenderer::ComputeInnerRadii(const gfxCornerSizes& aRadii,
                                       const gfxFloat *aBorderSizes,
                                       gfxCornerSizes *aInnerRadiiRet)
{
  gfxCornerSizes& iRadii = *aInnerRadiiRet;

  iRadii[C_TL].width = std::max(0.0, aRadii[C_TL].width - aBorderSizes[NS_SIDE_LEFT]);
  iRadii[C_TL].height = std::max(0.0, aRadii[C_TL].height - aBorderSizes[NS_SIDE_TOP]);

  iRadii[C_TR].width = std::max(0.0, aRadii[C_TR].width - aBorderSizes[NS_SIDE_RIGHT]);
  iRadii[C_TR].height = std::max(0.0, aRadii[C_TR].height - aBorderSizes[NS_SIDE_TOP]);

  iRadii[C_BR].width = std::max(0.0, aRadii[C_BR].width - aBorderSizes[NS_SIDE_RIGHT]);
  iRadii[C_BR].height = std::max(0.0, aRadii[C_BR].height - aBorderSizes[NS_SIDE_BOTTOM]);

  iRadii[C_BL].width = std::max(0.0, aRadii[C_BL].width - aBorderSizes[NS_SIDE_LEFT]);
  iRadii[C_BL].height = std::max(0.0, aRadii[C_BL].height - aBorderSizes[NS_SIDE_BOTTOM]);
}

 void
nsCSSBorderRenderer::ComputeOuterRadii(const gfxCornerSizes& aRadii,
                                       const gfxFloat *aBorderSizes,
                                       gfxCornerSizes *aOuterRadiiRet)
{
  gfxCornerSizes& oRadii = *aOuterRadiiRet;

  
  oRadii = gfxCornerSizes(0.0);

  
  if (aRadii[C_TL].width > 0.0 && aRadii[C_TL].height > 0.0) {
    oRadii[C_TL].width = std::max(0.0, aRadii[C_TL].width + aBorderSizes[NS_SIDE_LEFT]);
    oRadii[C_TL].height = std::max(0.0, aRadii[C_TL].height + aBorderSizes[NS_SIDE_TOP]);
  }

  if (aRadii[C_TR].width > 0.0 && aRadii[C_TR].height > 0.0) {
    oRadii[C_TR].width = std::max(0.0, aRadii[C_TR].width + aBorderSizes[NS_SIDE_RIGHT]);
    oRadii[C_TR].height = std::max(0.0, aRadii[C_TR].height + aBorderSizes[NS_SIDE_TOP]);
  }

  if (aRadii[C_BR].width > 0.0 && aRadii[C_BR].height > 0.0) {
    oRadii[C_BR].width = std::max(0.0, aRadii[C_BR].width + aBorderSizes[NS_SIDE_RIGHT]);
    oRadii[C_BR].height = std::max(0.0, aRadii[C_BR].height + aBorderSizes[NS_SIDE_BOTTOM]);
  }

  if (aRadii[C_BL].width > 0.0 && aRadii[C_BL].height > 0.0) {
    oRadii[C_BL].width = std::max(0.0, aRadii[C_BL].width + aBorderSizes[NS_SIDE_LEFT]);
    oRadii[C_BL].height = std::max(0.0, aRadii[C_BL].height + aBorderSizes[NS_SIDE_BOTTOM]);
  }
}

 void
ComputeBorderCornerDimensions(const gfxRect& aOuterRect,
                              const gfxRect& aInnerRect,
                              const gfxCornerSizes& aRadii,
                              gfxCornerSizes *aDimsRet)
{
  gfxFloat leftWidth = aInnerRect.X() - aOuterRect.X();
  gfxFloat topWidth = aInnerRect.Y() - aOuterRect.Y();
  gfxFloat rightWidth = aOuterRect.Width() - aInnerRect.Width() - leftWidth;
  gfxFloat bottomWidth = aOuterRect.Height() - aInnerRect.Height() - topWidth;

  if (AllCornersZeroSize(aRadii)) {
    
    (*aDimsRet)[C_TL] = gfxSize(leftWidth, topWidth);
    (*aDimsRet)[C_TR] = gfxSize(rightWidth, topWidth);
    (*aDimsRet)[C_BR] = gfxSize(rightWidth, bottomWidth);
    (*aDimsRet)[C_BL] = gfxSize(leftWidth, bottomWidth);
  } else {
    
    
    
    (*aDimsRet)[C_TL] = gfxSize(ceil(std::max(leftWidth, aRadii[C_TL].width)),
                                ceil(std::max(topWidth, aRadii[C_TL].height)));
    (*aDimsRet)[C_TR] = gfxSize(ceil(std::max(rightWidth, aRadii[C_TR].width)),
                                ceil(std::max(topWidth, aRadii[C_TR].height)));
    (*aDimsRet)[C_BR] = gfxSize(ceil(std::max(rightWidth, aRadii[C_BR].width)),
                                ceil(std::max(bottomWidth, aRadii[C_BR].height)));
    (*aDimsRet)[C_BL] = gfxSize(ceil(std::max(leftWidth, aRadii[C_BL].width)),
                                ceil(std::max(bottomWidth, aRadii[C_BL].height)));
  }
}

bool
nsCSSBorderRenderer::AreBorderSideFinalStylesSame(uint8_t aSides)
{
  NS_ASSERTION(aSides != 0 && (aSides & ~SIDE_BITS_ALL) == 0,
               "AreBorderSidesSame: invalid whichSides!");

  
  int firstStyle = 0;
  NS_FOR_CSS_SIDES (i) {
    if (firstStyle == i) {
      if (((1 << i) & aSides) == 0)
        firstStyle++;
      continue;
    }

    if (((1 << i) & aSides) == 0) {
      continue;
    }

    if (mBorderStyles[firstStyle] != mBorderStyles[i] ||
        mBorderColors[firstStyle] != mBorderColors[i] ||
        !nsBorderColors::Equal(mCompositeColors[firstStyle],
                               mCompositeColors[i]))
      return false;
  }

  

  switch (mBorderStyles[firstStyle]) {
    case NS_STYLE_BORDER_STYLE_GROOVE:
    case NS_STYLE_BORDER_STYLE_RIDGE:
    case NS_STYLE_BORDER_STYLE_INSET:
    case NS_STYLE_BORDER_STYLE_OUTSET:
      return ((aSides & ~(SIDE_BIT_TOP | SIDE_BIT_LEFT)) == 0 ||
              (aSides & ~(SIDE_BIT_BOTTOM | SIDE_BIT_RIGHT)) == 0);
  }

  return true;
}

bool
nsCSSBorderRenderer::IsSolidCornerStyle(uint8_t aStyle, mozilla::css::Corner aCorner)
{
  switch (aStyle) {
    case NS_STYLE_BORDER_STYLE_DOTTED:
    case NS_STYLE_BORDER_STYLE_DASHED:
    case NS_STYLE_BORDER_STYLE_SOLID:
      return true;

    case NS_STYLE_BORDER_STYLE_INSET:
    case NS_STYLE_BORDER_STYLE_OUTSET:
      return (aCorner == NS_CORNER_TOP_LEFT || aCorner == NS_CORNER_BOTTOM_RIGHT);

    case NS_STYLE_BORDER_STYLE_GROOVE:
    case NS_STYLE_BORDER_STYLE_RIDGE:
      return mOneUnitBorder && (aCorner == NS_CORNER_TOP_LEFT || aCorner == NS_CORNER_BOTTOM_RIGHT);

    case NS_STYLE_BORDER_STYLE_DOUBLE:
      return mOneUnitBorder;

    default:
      return false;
  }
}

BorderColorStyle
nsCSSBorderRenderer::BorderColorStyleForSolidCorner(uint8_t aStyle, mozilla::css::Corner aCorner)
{
  
  
  switch (aStyle) {
    case NS_STYLE_BORDER_STYLE_DOTTED:
    case NS_STYLE_BORDER_STYLE_DASHED:
    case NS_STYLE_BORDER_STYLE_SOLID:
    case NS_STYLE_BORDER_STYLE_DOUBLE:
      return BorderColorStyleSolid;

    case NS_STYLE_BORDER_STYLE_INSET:
    case NS_STYLE_BORDER_STYLE_GROOVE:
      if (aCorner == NS_CORNER_TOP_LEFT)
        return BorderColorStyleDark;
      else if (aCorner == NS_CORNER_BOTTOM_RIGHT)
        return BorderColorStyleLight;
      break;

    case NS_STYLE_BORDER_STYLE_OUTSET:
    case NS_STYLE_BORDER_STYLE_RIDGE:
      if (aCorner == NS_CORNER_TOP_LEFT)
        return BorderColorStyleLight;
      else if (aCorner == NS_CORNER_BOTTOM_RIGHT)
        return BorderColorStyleDark;
      break;
  }

  return BorderColorStyleNone;
}

void
nsCSSBorderRenderer::DoCornerSubPath(mozilla::css::Corner aCorner)
{
  gfxPoint offset(0.0, 0.0);

  if (aCorner == C_TR || aCorner == C_BR)
    offset.x = mOuterRect.Width() - mBorderCornerDimensions[aCorner].width;
  if (aCorner == C_BR || aCorner == C_BL)
    offset.y = mOuterRect.Height() - mBorderCornerDimensions[aCorner].height;

  mContext->Rectangle(gfxRect(mOuterRect.TopLeft() + offset,
                              mBorderCornerDimensions[aCorner]));
}

void
nsCSSBorderRenderer::DoSideClipWithoutCornersSubPath(mozilla::css::Side aSide)
{
  gfxPoint offset(0.0, 0.0);

  
  
  
  
  
  if (aSide == NS_SIDE_TOP) {
    offset.x = mBorderCornerDimensions[C_TL].width;
  } else if (aSide == NS_SIDE_RIGHT) {
    offset.x = mOuterRect.Width() - mBorderWidths[NS_SIDE_RIGHT];
    offset.y = mBorderCornerDimensions[C_TR].height;
  } else if (aSide == NS_SIDE_BOTTOM) {
    offset.x = mBorderCornerDimensions[C_BL].width;
    offset.y = mOuterRect.Height() - mBorderWidths[NS_SIDE_BOTTOM];
  } else if (aSide == NS_SIDE_LEFT) {
    offset.y = mBorderCornerDimensions[C_TL].height;
  }

  
  
  
  
  gfxSize sideCornerSum = mBorderCornerDimensions[mozilla::css::Corner(aSide)]
                        + mBorderCornerDimensions[mozilla::css::Corner(NEXT_SIDE(aSide))];
  gfxRect rect(mOuterRect.TopLeft() + offset,
               mOuterRect.Size() - sideCornerSum);

  if (aSide == NS_SIDE_TOP || aSide == NS_SIDE_BOTTOM)
    rect.height = mBorderWidths[aSide];
  else
    rect.width = mBorderWidths[aSide];

  mContext->Rectangle(rect);
}





typedef enum {
  
  
  SIDE_CLIP_TRAPEZOID,

  
  
  
  
  
  
  
  
  SIDE_CLIP_TRAPEZOID_FULL,

  
  
  
  SIDE_CLIP_RECTANGLE
} SideClipType;








static void
MaybeMoveToMidPoint(gfxPoint& aP0, gfxPoint& aP1, const gfxPoint& aMidPoint)
{
  gfxPoint ps = aP1 - aP0;

  if (ps.x == 0.0) {
    if (ps.y == 0.0) {
      aP1 = aMidPoint;
    } else {
      aP1.y = aMidPoint.y;
    }
  } else {
    if (ps.y == 0.0) {
      aP1.x = aMidPoint.x;
    } else {
      gfxFloat k = std::min((aMidPoint.x - aP0.x) / ps.x,
                          (aMidPoint.y - aP0.y) / ps.y);
      aP1 = aP0 + ps * k;
    }
  }
}

void
nsCSSBorderRenderer::DoSideClipSubPath(mozilla::css::Side aSide)
{
  
  
  
  
  
  
  
  
  

  gfxPoint start[2];
  gfxPoint end[2];

#define IS_DASHED_OR_DOTTED(_s)  ((_s) == NS_STYLE_BORDER_STYLE_DASHED || (_s) == NS_STYLE_BORDER_STYLE_DOTTED)
  bool isDashed      = IS_DASHED_OR_DOTTED(mBorderStyles[aSide]);
  bool startIsDashed = IS_DASHED_OR_DOTTED(mBorderStyles[PREV_SIDE(aSide)]);
  bool endIsDashed   = IS_DASHED_OR_DOTTED(mBorderStyles[NEXT_SIDE(aSide)]);
#undef IS_DASHED_OR_DOTTED

  SideClipType startType = SIDE_CLIP_TRAPEZOID;
  SideClipType endType = SIDE_CLIP_TRAPEZOID;

  if (!IsZeroSize(mBorderRadii[mozilla::css::Corner(aSide)]))
    startType = SIDE_CLIP_TRAPEZOID_FULL;
  else if (startIsDashed && isDashed)
    startType = SIDE_CLIP_RECTANGLE;

  if (!IsZeroSize(mBorderRadii[mozilla::css::Corner(NEXT_SIDE(aSide))]))
    endType = SIDE_CLIP_TRAPEZOID_FULL;
  else if (endIsDashed && isDashed)
    endType = SIDE_CLIP_RECTANGLE;

  gfxPoint midPoint = mInnerRect.Center();

  start[0] = mOuterRect.CCWCorner(aSide);
  start[1] = mInnerRect.CCWCorner(aSide);

  end[0] = mOuterRect.CWCorner(aSide);
  end[1] = mInnerRect.CWCorner(aSide);

  if (startType == SIDE_CLIP_TRAPEZOID_FULL) {
    MaybeMoveToMidPoint(start[0], start[1], midPoint);
  } else if (startType == SIDE_CLIP_RECTANGLE) {
    if (aSide == NS_SIDE_TOP || aSide == NS_SIDE_BOTTOM)
      start[1] = gfxPoint(mOuterRect.CCWCorner(aSide).x, mInnerRect.CCWCorner(aSide).y);
    else
      start[1] = gfxPoint(mInnerRect.CCWCorner(aSide).x, mOuterRect.CCWCorner(aSide).y);
  }

  if (endType == SIDE_CLIP_TRAPEZOID_FULL) {
    MaybeMoveToMidPoint(end[0], end[1], midPoint);
  } else if (endType == SIDE_CLIP_RECTANGLE) {
    if (aSide == NS_SIDE_TOP || aSide == NS_SIDE_BOTTOM)
      end[0] = gfxPoint(mInnerRect.CWCorner(aSide).x, mOuterRect.CWCorner(aSide).y);
    else
      end[0] = gfxPoint(mOuterRect.CWCorner(aSide).x, mInnerRect.CWCorner(aSide).y);
  }

  mContext->MoveTo(start[0]);
  mContext->LineTo(end[0]);
  mContext->LineTo(end[1]);
  mContext->LineTo(start[1]);
  mContext->ClosePath();
}

void
nsCSSBorderRenderer::FillSolidBorder(const gfxRect& aOuterRect,
                                     const gfxRect& aInnerRect,
                                     const gfxCornerSizes& aBorderRadii,
                                     const gfxFloat *aBorderSizes,
                                     int aSides,
                                     const gfxRGBA& aColor)
{
  mContext->SetColor(aColor);
  
  

  
  
  if (!AllCornersZeroSize(aBorderRadii)) {
    gfxCornerSizes innerRadii;
    ComputeInnerRadii(aBorderRadii, aBorderSizes, &innerRadii);

    mContext->NewPath();

    
    mContext->RoundedRectangle(aOuterRect, aBorderRadii, true);

    
    mContext->RoundedRectangle(aInnerRect, innerRadii, false);

    mContext->Fill();

    return;
  }

  
  
  
  
  
  if (aSides == SIDE_BITS_ALL &&
      CheckFourFloatsEqual(aBorderSizes, aBorderSizes[0]) &&
      !mAvoidStroke)
  {
    gfxRect r(aOuterRect);
    r.Deflate(aBorderSizes[0] / 2.0);
    mContext->SetLineWidth(aBorderSizes[0]);

    mContext->NewPath();
    mContext->Rectangle(r);
    mContext->Stroke();

    return;
  }

  
  
  

  gfxRect r[4];

  
  if (aSides & SIDE_BIT_TOP) {
    r[NS_SIDE_TOP] =
        gfxRect(aOuterRect.X(), aOuterRect.Y(),
                aOuterRect.Width(), aBorderSizes[NS_SIDE_TOP]);
  }

  if (aSides & SIDE_BIT_BOTTOM) {
    r[NS_SIDE_BOTTOM] =
        gfxRect(aOuterRect.X(), aOuterRect.YMost() - aBorderSizes[NS_SIDE_BOTTOM],
                aOuterRect.Width(), aBorderSizes[NS_SIDE_BOTTOM]);
  }

  if (aSides & SIDE_BIT_LEFT) {
    r[NS_SIDE_LEFT] =
        gfxRect(aOuterRect.X(), aOuterRect.Y(),
                aBorderSizes[NS_SIDE_LEFT], aOuterRect.Height());
  }

  if (aSides & SIDE_BIT_RIGHT) {
    r[NS_SIDE_RIGHT] =
        gfxRect(aOuterRect.XMost() - aBorderSizes[NS_SIDE_RIGHT], aOuterRect.Y(),
                aBorderSizes[NS_SIDE_RIGHT], aOuterRect.Height());
  }

  
  
  
  

  if ((aSides & (SIDE_BIT_TOP | SIDE_BIT_LEFT)) == (SIDE_BIT_TOP | SIDE_BIT_LEFT)) {
    
    r[NS_SIDE_LEFT].y += aBorderSizes[NS_SIDE_TOP];
    r[NS_SIDE_LEFT].height -= aBorderSizes[NS_SIDE_TOP];
  }

  if ((aSides & (SIDE_BIT_TOP | SIDE_BIT_RIGHT)) == (SIDE_BIT_TOP | SIDE_BIT_RIGHT)) {
    
    r[NS_SIDE_TOP].width -= aBorderSizes[NS_SIDE_RIGHT];
  }

  if ((aSides & (SIDE_BIT_BOTTOM | SIDE_BIT_RIGHT)) == (SIDE_BIT_BOTTOM | SIDE_BIT_RIGHT)) {
    
    r[NS_SIDE_RIGHT].height -= aBorderSizes[NS_SIDE_BOTTOM];
  }

  if ((aSides & (SIDE_BIT_BOTTOM | SIDE_BIT_LEFT)) == (SIDE_BIT_BOTTOM | SIDE_BIT_LEFT)) {
    
    r[NS_SIDE_BOTTOM].x += aBorderSizes[NS_SIDE_LEFT];
    r[NS_SIDE_BOTTOM].width -= aBorderSizes[NS_SIDE_LEFT];
  }

  
  for (uint32_t i = 0; i < 4; i++) {
    if (aSides & (1 << i)) {
      mContext->NewPath();
      mContext->Rectangle(r[i], true);
      mContext->Fill();
    }
  }
}

gfxRGBA
MakeBorderColor(const gfxRGBA& aColor, const gfxRGBA& aBackgroundColor, BorderColorStyle aBorderColorStyle)
{
  nscolor colors[2];
  int k = 0;

  switch (aBorderColorStyle) {
    case BorderColorStyleNone:
      return gfxRGBA(0.0, 0.0, 0.0, 0.0);

    case BorderColorStyleLight:
      k = 1;
      
    case BorderColorStyleDark:
      NS_GetSpecial3DColors(colors, aBackgroundColor.Packed(), aColor.Packed());
      return gfxRGBA(colors[k]);

    case BorderColorStyleSolid:
    default:
      return aColor;
  }
}

gfxRGBA
ComputeColorForLine(uint32_t aLineIndex,
                    const BorderColorStyle* aBorderColorStyle,
                    uint32_t aBorderColorStyleCount,
                    nscolor aBorderColor,
                    nscolor aBackgroundColor)
{
  NS_ASSERTION(aLineIndex < aBorderColorStyleCount, "Invalid lineIndex given");

  return MakeBorderColor(gfxRGBA(aBorderColor), gfxRGBA(aBackgroundColor), aBorderColorStyle[aLineIndex]);
}

gfxRGBA
ComputeCompositeColorForLine(uint32_t aLineIndex,
                             const nsBorderColors* aBorderColors)
{
  while (aLineIndex-- && aBorderColors->mNext)
    aBorderColors = aBorderColors->mNext;

  return gfxRGBA(aBorderColors->mColor);
}

void
nsCSSBorderRenderer::DrawBorderSidesCompositeColors(int aSides, const nsBorderColors *aCompositeColors)
{
  gfxCornerSizes radii = mBorderRadii;

  
  gfxRect soRect = mOuterRect;
  gfxFloat maxBorderWidth = 0;
  NS_FOR_CSS_SIDES (i) {
    maxBorderWidth = std::max(maxBorderWidth, mBorderWidths[i]);
  }

  gfxFloat fakeBorderSizes[4];

  gfxPoint itl = mInnerRect.TopLeft();
  gfxPoint ibr = mInnerRect.BottomRight();

  for (uint32_t i = 0; i < uint32_t(maxBorderWidth); i++) {
    gfxRGBA lineColor = ComputeCompositeColorForLine(i, aCompositeColors);

    gfxRect siRect = soRect;
    siRect.Deflate(1.0);

    
    gfxPoint tl = siRect.TopLeft();
    gfxPoint br = siRect.BottomRight();

    tl.x = std::min(tl.x, itl.x);
    tl.y = std::min(tl.y, itl.y);

    br.x = std::max(br.x, ibr.x);
    br.y = std::max(br.y, ibr.y);

    siRect = gfxRect(tl.x, tl.y, br.x - tl.x , br.y - tl.y);

    fakeBorderSizes[NS_SIDE_TOP] = siRect.TopLeft().y - soRect.TopLeft().y;
    fakeBorderSizes[NS_SIDE_RIGHT] = soRect.TopRight().x - siRect.TopRight().x;
    fakeBorderSizes[NS_SIDE_BOTTOM] = soRect.BottomRight().y - siRect.BottomRight().y;
    fakeBorderSizes[NS_SIDE_LEFT] = siRect.BottomLeft().x - soRect.BottomLeft().x;

    FillSolidBorder(soRect, siRect, radii, fakeBorderSizes, aSides, lineColor);

    soRect = siRect;

    ComputeInnerRadii(radii, fakeBorderSizes, &radii);
  }
}

void
nsCSSBorderRenderer::DrawBorderSides(int aSides)
{
  if (aSides == 0 || (aSides & ~SIDE_BITS_ALL) != 0) {
    NS_WARNING("DrawBorderSides: invalid sides!");
    return;
  }

  uint8_t borderRenderStyle = NS_STYLE_BORDER_STYLE_NONE;
  nscolor borderRenderColor;
  const nsBorderColors *compositeColors = nullptr;

  uint32_t borderColorStyleCount = 0;
  BorderColorStyle borderColorStyleTopLeft[3], borderColorStyleBottomRight[3];
  BorderColorStyle *borderColorStyle = nullptr;

  NS_FOR_CSS_SIDES (i) {
    if ((aSides & (1 << i)) == 0)
      continue;
    borderRenderStyle = mBorderStyles[i];
    borderRenderColor = mBorderColors[i];
    compositeColors = mCompositeColors[i];
    break;
  }

  if (borderRenderStyle == NS_STYLE_BORDER_STYLE_NONE ||
      borderRenderStyle == NS_STYLE_BORDER_STYLE_HIDDEN)
    return;

  
  
  
  
  if (compositeColors) {
    DrawBorderSidesCompositeColors(aSides, compositeColors);
    return;
  }

  
  
  
  
  
  
  
  if (mOneUnitBorder &&
      (borderRenderStyle == NS_STYLE_BORDER_STYLE_RIDGE ||
       borderRenderStyle == NS_STYLE_BORDER_STYLE_GROOVE ||
       borderRenderStyle == NS_STYLE_BORDER_STYLE_DOUBLE))
    borderRenderStyle = NS_STYLE_BORDER_STYLE_SOLID;

  switch (borderRenderStyle) {
    case NS_STYLE_BORDER_STYLE_SOLID:
    case NS_STYLE_BORDER_STYLE_DASHED:
    case NS_STYLE_BORDER_STYLE_DOTTED:
      borderColorStyleTopLeft[0] = BorderColorStyleSolid;

      borderColorStyleBottomRight[0] = BorderColorStyleSolid;

      borderColorStyleCount = 1;
      break;

    case NS_STYLE_BORDER_STYLE_GROOVE:
      borderColorStyleTopLeft[0] = BorderColorStyleDark;
      borderColorStyleTopLeft[1] = BorderColorStyleLight;

      borderColorStyleBottomRight[0] = BorderColorStyleLight;
      borderColorStyleBottomRight[1] = BorderColorStyleDark;

      borderColorStyleCount = 2;
      break;

    case NS_STYLE_BORDER_STYLE_RIDGE:
      borderColorStyleTopLeft[0] = BorderColorStyleLight;
      borderColorStyleTopLeft[1] = BorderColorStyleDark;

      borderColorStyleBottomRight[0] = BorderColorStyleDark;
      borderColorStyleBottomRight[1] = BorderColorStyleLight;

      borderColorStyleCount = 2;
      break;

    case NS_STYLE_BORDER_STYLE_DOUBLE:
      borderColorStyleTopLeft[0] = BorderColorStyleSolid;
      borderColorStyleTopLeft[1] = BorderColorStyleNone;
      borderColorStyleTopLeft[2] = BorderColorStyleSolid;

      borderColorStyleBottomRight[0] = BorderColorStyleSolid;
      borderColorStyleBottomRight[1] = BorderColorStyleNone;
      borderColorStyleBottomRight[2] = BorderColorStyleSolid;

      borderColorStyleCount = 3;
      break;

    case NS_STYLE_BORDER_STYLE_INSET:
      borderColorStyleTopLeft[0] = BorderColorStyleDark;
      borderColorStyleBottomRight[0] = BorderColorStyleLight;

      borderColorStyleCount = 1;
      break;

    case NS_STYLE_BORDER_STYLE_OUTSET:
      borderColorStyleTopLeft[0] = BorderColorStyleLight;
      borderColorStyleBottomRight[0] = BorderColorStyleDark;

      borderColorStyleCount = 1;
      break;

    default:
      NS_NOTREACHED("Unhandled border style!!");
      break;
  }

  
  
  
  NS_ASSERTION(borderColorStyleCount > 0 && borderColorStyleCount < 4,
               "Non-border-colors case with borderColorStyleCount < 1 or > 3; what happened?");

  
  
  
  if (aSides & (SIDE_BIT_BOTTOM | SIDE_BIT_RIGHT))
    borderColorStyle = borderColorStyleBottomRight;
  else
    borderColorStyle = borderColorStyleTopLeft;

  
  gfxFloat borderWidths[3][4];

  if (borderColorStyleCount == 1) {
    NS_FOR_CSS_SIDES (i) {
      borderWidths[0][i] = mBorderWidths[i];
    }
  } else if (borderColorStyleCount == 2) {
    
    NS_FOR_CSS_SIDES (i) {
      borderWidths[0][i] = int32_t(mBorderWidths[i]) / 2 + int32_t(mBorderWidths[i]) % 2;
      borderWidths[1][i] = int32_t(mBorderWidths[i]) / 2;
    }
  } else if (borderColorStyleCount == 3) {
    
    
    NS_FOR_CSS_SIDES (i) {
      if (mBorderWidths[i] == 1.0) {
        borderWidths[0][i] = 1.0;
        borderWidths[1][i] = borderWidths[2][i] = 0.0;
      } else {
        int32_t rest = int32_t(mBorderWidths[i]) % 3;
        borderWidths[0][i] = borderWidths[2][i] = borderWidths[1][i] = (int32_t(mBorderWidths[i]) - rest) / 3;

        if (rest == 1) {
          borderWidths[1][i] += 1.0;
        } else if (rest == 2) {
          borderWidths[0][i] += 1.0;
          borderWidths[2][i] += 1.0;
        }
      }
    }
  }

  
  gfxCornerSizes radii = mBorderRadii;

  gfxRect soRect(mOuterRect);
  gfxRect siRect(mOuterRect);

  for (unsigned int i = 0; i < borderColorStyleCount; i++) {
    
    
    siRect.Deflate(gfxMargin(borderWidths[i][0], borderWidths[i][1],
                             borderWidths[i][2], borderWidths[i][3]));

    if (borderColorStyle[i] != BorderColorStyleNone) {
      gfxRGBA color = ComputeColorForLine(i,
                                          borderColorStyle, borderColorStyleCount,
                                          borderRenderColor, mBackgroundColor);

      FillSolidBorder(soRect, siRect, radii, borderWidths[i], aSides, color);
    }

    ComputeInnerRadii(radii, borderWidths[i], &radii);

    
    soRect = siRect;
  }
}

void
nsCSSBorderRenderer::DrawDashedSide(mozilla::css::Side aSide)
{
  gfxFloat dashWidth;
  gfxFloat dash[2];

  uint8_t style = mBorderStyles[aSide];
  gfxFloat borderWidth = mBorderWidths[aSide];
  nscolor borderColor = mBorderColors[aSide];

  if (borderWidth == 0.0)
    return;

  if (style == NS_STYLE_BORDER_STYLE_NONE ||
      style == NS_STYLE_BORDER_STYLE_HIDDEN)
    return;

  if (style == NS_STYLE_BORDER_STYLE_DASHED) {
    dashWidth = gfxFloat(borderWidth * DOT_LENGTH * DASH_LENGTH);

    dash[0] = dashWidth;
    dash[1] = dashWidth;

    mContext->SetLineCap(gfxContext::LINE_CAP_BUTT);
  } else if (style == NS_STYLE_BORDER_STYLE_DOTTED) {
    dashWidth = gfxFloat(borderWidth * DOT_LENGTH);

    if (borderWidth > 2.0) {
      dash[0] = 0.0;
      dash[1] = dashWidth * 2.0;

      mContext->SetLineCap(gfxContext::LINE_CAP_ROUND);
    } else {
      dash[0] = dashWidth;
      dash[1] = dashWidth;
    }
  } else {
    PrintAsFormatString("DrawDashedSide: style: %d!!\n", style);
    NS_ERROR("DrawDashedSide called with style other than DASHED or DOTTED; someone's not playing nice");
    return;
  }

  PrintAsFormatString("dash: %f %f\n", dash[0], dash[1]);

  mContext->SetDash(dash, 2, 0.0);

  gfxPoint start = mOuterRect.CCWCorner(aSide);
  gfxPoint end = mOuterRect.CWCorner(aSide);

  if (aSide == NS_SIDE_TOP) {
    start.x += mBorderCornerDimensions[C_TL].width;
    end.x -= mBorderCornerDimensions[C_TR].width;

    start.y += borderWidth / 2.0;
    end.y += borderWidth / 2.0;
  } else if (aSide == NS_SIDE_RIGHT) {
    start.x -= borderWidth / 2.0;
    end.x -= borderWidth / 2.0;

    start.y += mBorderCornerDimensions[C_TR].height;
    end.y -= mBorderCornerDimensions[C_BR].height;
  } else if (aSide == NS_SIDE_BOTTOM) {
    start.x -= mBorderCornerDimensions[C_BR].width;
    end.x += mBorderCornerDimensions[C_BL].width;

    start.y -= borderWidth / 2.0;
    end.y -= borderWidth / 2.0;
  } else if (aSide == NS_SIDE_LEFT) {
    start.x += borderWidth / 2.0;
    end.x += borderWidth / 2.0;

    start.y -= mBorderCornerDimensions[C_BL].height;
    end.y += mBorderCornerDimensions[C_TL].height;
  }

  mContext->NewPath();
  mContext->MoveTo(start);
  mContext->LineTo(end);
  mContext->SetLineWidth(borderWidth);
  mContext->SetColor(gfxRGBA(borderColor));
  
  mContext->Stroke();
}

void
nsCSSBorderRenderer::SetupStrokeStyle(mozilla::css::Side aSide)
{
  mContext->SetColor(gfxRGBA(mBorderColors[aSide]));
  mContext->SetLineWidth(mBorderWidths[aSide]);
}

bool
nsCSSBorderRenderer::AllBordersSameWidth()
{
  if (mBorderWidths[0] == mBorderWidths[1] &&
      mBorderWidths[0] == mBorderWidths[2] &&
      mBorderWidths[0] == mBorderWidths[3])
  {
    return true;
  }

  return false;
}

bool
nsCSSBorderRenderer::AllBordersSolid(bool *aHasCompositeColors)
{
  *aHasCompositeColors = false;
  NS_FOR_CSS_SIDES(i) {
    if (mCompositeColors[i] != nullptr) {
      *aHasCompositeColors = true;
    }
    if (mBorderStyles[i] == NS_STYLE_BORDER_STYLE_SOLID ||
        mBorderStyles[i] == NS_STYLE_BORDER_STYLE_NONE ||
        mBorderStyles[i] == NS_STYLE_BORDER_STYLE_HIDDEN)
    {
      continue;
    }
    return false;
  }

  return true;
}

bool IsVisible(int aStyle)
{
  if (aStyle != NS_STYLE_BORDER_STYLE_NONE &&
      aStyle != NS_STYLE_BORDER_STYLE_HIDDEN) {
        return true;
  }
  return false;
}

already_AddRefed<gfxPattern>
nsCSSBorderRenderer::CreateCornerGradient(mozilla::css::Corner aCorner,
                                          const gfxRGBA &aFirstColor,
                                          const gfxRGBA &aSecondColor)
{
  typedef struct { gfxFloat a, b; } twoFloats;

  const twoFloats gradientCoeff[4] = { { -1, +1 },
                                       { -1, -1 },
                                       { +1, -1 },
                                       { +1, +1 } };

  
  
  const int cornerWidth[4] = { 3, 1, 1, 3 };
  const int cornerHeight[4] = { 0, 0, 2, 2 };

  gfxPoint cornerOrigin = mOuterRect.AtCorner(aCorner);

  gfxPoint pat1, pat2;
  pat1.x = cornerOrigin.x +
    mBorderWidths[cornerHeight[aCorner]] * gradientCoeff[aCorner].a;
  pat1.y = cornerOrigin.y +
    mBorderWidths[cornerWidth[aCorner]]  * gradientCoeff[aCorner].b;
  pat2.x = cornerOrigin.x -
    mBorderWidths[cornerHeight[aCorner]] * gradientCoeff[aCorner].a;
  pat2.y = cornerOrigin.y -
    mBorderWidths[cornerWidth[aCorner]]  * gradientCoeff[aCorner].b;

  float gradientOffset =
    0.25 / sqrt(pow(mBorderWidths[cornerHeight[aCorner]], 2) +
                pow(mBorderWidths[cornerHeight[aCorner]], 2));

  nsRefPtr<gfxPattern> pattern = new gfxPattern(pat1.x, pat1.y, pat2.x, pat2.y);
  pattern->AddColorStop(0.5 - gradientOffset, gfxRGBA(aFirstColor));
  pattern->AddColorStop(0.5 + gradientOffset, gfxRGBA(aSecondColor));

  return pattern.forget();
}

TemporaryRef<GradientStops>
nsCSSBorderRenderer::CreateCornerGradient(mozilla::css::Corner aCorner,
                                          const gfxRGBA &aFirstColor,
                                          const gfxRGBA &aSecondColor,
                                          DrawTarget *aDT,
                                          Point &aPoint1,
                                          Point &aPoint2)
{
  typedef struct { gfxFloat a, b; } twoFloats;

  const twoFloats gradientCoeff[4] = { { -1, +1 },
                                       { -1, -1 },
                                       { +1, -1 },
                                       { +1, +1 } };

  
  
  const int cornerWidth[4] = { 3, 1, 1, 3 };
  const int cornerHeight[4] = { 0, 0, 2, 2 };

  gfxPoint cornerOrigin = mOuterRect.AtCorner(aCorner);

  gfxPoint pat1, pat2;
  pat1.x = cornerOrigin.x +
    mBorderWidths[cornerHeight[aCorner]] * gradientCoeff[aCorner].a;
  pat1.y = cornerOrigin.y +
    mBorderWidths[cornerWidth[aCorner]]  * gradientCoeff[aCorner].b;
  pat2.x = cornerOrigin.x -
    mBorderWidths[cornerHeight[aCorner]] * gradientCoeff[aCorner].a;
  pat2.y = cornerOrigin.y -
    mBorderWidths[cornerWidth[aCorner]]  * gradientCoeff[aCorner].b;

  aPoint1 = Point(pat1.x, pat1.y);
  aPoint2 = Point(pat2.x, pat2.y);

  Color firstColor = ToColor(aFirstColor);
  Color secondColor = ToColor(aSecondColor);

  nsTArray<gfx::GradientStop> rawStops(2);
  rawStops.SetLength(2);
  
  
  
  
  rawStops[0].color = firstColor;
  rawStops[0].offset = 0.5;
  rawStops[1].color = secondColor;
  rawStops[1].offset = 0.5;
  RefPtr<GradientStops> gs =
    gfxGradientCache::GetGradientStops(aDT, rawStops, ExtendMode::CLAMP);
  if (!gs) {
    
    
    rawStops[0].color = secondColor;
    rawStops[1].color = firstColor;
    Point tmp = aPoint1;
    aPoint1 = aPoint2;
    aPoint2 = tmp;
    gs = gfxGradientCache::GetOrCreateGradientStops(aDT, rawStops, ExtendMode::CLAMP);
  }
  return gs;
}

typedef struct { gfxFloat a, b; } twoFloats;

void
nsCSSBorderRenderer::DrawSingleWidthSolidBorder()
{
  
  mContext->SetLineWidth(1);
  gfxRect rect = mOuterRect;
  rect.Deflate(0.5);

  const twoFloats cornerAdjusts[4] = { { +0.5,  0   },
                                       {    0, +0.5 },
                                       { -0.5,  0   },
                                       {    0, -0.5 } };


  NS_FOR_CSS_SIDES(side) {
    gfxPoint firstCorner = rect.CCWCorner(side);
    firstCorner.x += cornerAdjusts[side].a;
    firstCorner.y += cornerAdjusts[side].b;
    gfxPoint secondCorner = rect.CWCorner(side);
    secondCorner.x += cornerAdjusts[side].a;
    secondCorner.y += cornerAdjusts[side].b;

    mContext->SetColor(gfxRGBA(mBorderColors[side]));
    mContext->NewPath();
    mContext->MoveTo(firstCorner);
    mContext->LineTo(secondCorner);
    mContext->Stroke();
  }
}

void
nsCSSBorderRenderer::DrawNoCompositeColorSolidBorder()
{
  DrawTarget *dt = mContext->GetDrawTarget();

  const gfxFloat alpha = 0.55191497064665766025;

  const twoFloats cornerMults[4] = { { -1,  0 },
                                     {  0, -1 },
                                     { +1,  0 },
                                     {  0, +1 } };

  const twoFloats centerAdjusts[4] = { { 0, +0.5 },
                                       { -0.5, 0 },
                                       { 0, -0.5 },
                                       { +0.5, 0 } };

  Point pc, pci, p0, p1, p2, p3, pd, p3i;

  gfxCornerSizes innerRadii;
  ComputeInnerRadii(mBorderRadii, mBorderWidths, &innerRadii);

  gfxRect strokeRect = mOuterRect;
  strokeRect.Deflate(gfxMargin(mBorderWidths[0] / 2.0, mBorderWidths[1] / 2.0,
                               mBorderWidths[2] / 2.0, mBorderWidths[3] / 2.0));

  ColorPattern colorPat(Color(0, 0, 0, 0));
  LinearGradientPattern gradPat(Point(), Point(), nullptr);

  NS_FOR_CSS_CORNERS(i) {
      
    mozilla::css::Corner c = mozilla::css::Corner((i+1) % 4);
    mozilla::css::Corner prevCorner = mozilla::css::Corner(i);

    
    
    
    int i1 = (i+1) % 4;
    int i2 = (i+2) % 4;
    int i3 = (i+3) % 4;

    pc = ToPoint(mOuterRect.AtCorner(c));
    pci = ToPoint(mInnerRect.AtCorner(c));

    nscolor firstColor, secondColor;
    if (IsVisible(mBorderStyles[i]) && IsVisible(mBorderStyles[i1])) {
      firstColor = mBorderColors[i];
      secondColor = mBorderColors[i1];
    } else if (IsVisible(mBorderStyles[i])) {
      firstColor = mBorderColors[i];
      secondColor = mBorderColors[i];
    } else {
      firstColor = mBorderColors[i1];
      secondColor = mBorderColors[i1];
    }

    RefPtr<PathBuilder> builder = dt->CreatePathBuilder();

    Point strokeStart, strokeEnd;

    strokeStart.x = mOuterRect.AtCorner(prevCorner).x +
      mBorderCornerDimensions[prevCorner].width * cornerMults[i2].a;
    strokeStart.y = mOuterRect.AtCorner(prevCorner).y +
      mBorderCornerDimensions[prevCorner].height * cornerMults[i2].b;

    strokeEnd.x = pc.x + mBorderCornerDimensions[c].width * cornerMults[i].a;
    strokeEnd.y = pc.y + mBorderCornerDimensions[c].height * cornerMults[i].b;

    strokeStart.x += centerAdjusts[i].a * mBorderWidths[i];
    strokeStart.y += centerAdjusts[i].b * mBorderWidths[i];
    strokeEnd.x += centerAdjusts[i].a * mBorderWidths[i];
    strokeEnd.y += centerAdjusts[i].b * mBorderWidths[i];

    builder->MoveTo(strokeStart);
    builder->LineTo(strokeEnd);
    RefPtr<Path> path = builder->Finish();
    dt->Stroke(path, ColorPattern(Color::FromABGR(mBorderColors[i])), StrokeOptions(mBorderWidths[i]));
    builder = nullptr;
    path = nullptr;

    Pattern *pattern;

    if (firstColor != secondColor) {
      gradPat.mStops = CreateCornerGradient(c, firstColor, secondColor, dt, gradPat.mBegin, gradPat.mEnd);
      pattern = &gradPat;
    } else {
      colorPat.mColor = Color::FromABGR(firstColor);
      pattern = &colorPat;
    }

    builder = dt->CreatePathBuilder();

    if (mBorderRadii[c].width > 0 && mBorderRadii[c].height > 0) {
      p0.x = pc.x + cornerMults[i].a * mBorderRadii[c].width;
      p0.y = pc.y + cornerMults[i].b * mBorderRadii[c].height;

      p3.x = pc.x + cornerMults[i3].a * mBorderRadii[c].width;
      p3.y = pc.y + cornerMults[i3].b * mBorderRadii[c].height;

      p1.x = p0.x + alpha * cornerMults[i2].a * mBorderRadii[c].width;
      p1.y = p0.y + alpha * cornerMults[i2].b * mBorderRadii[c].height;

      p2.x = p3.x - alpha * cornerMults[i3].a * mBorderRadii[c].width;
      p2.y = p3.y - alpha * cornerMults[i3].b * mBorderRadii[c].height;

      Point cornerStart;
      cornerStart.x = pc.x + cornerMults[i].a * mBorderCornerDimensions[c].width;
      cornerStart.y = pc.y + cornerMults[i].b * mBorderCornerDimensions[c].height;

      builder->MoveTo(cornerStart);
      builder->LineTo(p0);

      builder->BezierTo(p1, p2, p3);

      Point outerCornerEnd;
      outerCornerEnd.x = pc.x + cornerMults[i3].a * mBorderCornerDimensions[c].width;
      outerCornerEnd.y = pc.y + cornerMults[i3].b * mBorderCornerDimensions[c].height;

      builder->LineTo(outerCornerEnd);

      p0.x = pci.x + cornerMults[i].a * innerRadii[c].width;
      p0.y = pci.y + cornerMults[i].b * innerRadii[c].height;

      p3i.x = pci.x + cornerMults[i3].a * innerRadii[c].width;
      p3i.y = pci.y + cornerMults[i3].b * innerRadii[c].height;

      p1.x = p0.x + alpha * cornerMults[i2].a * innerRadii[c].width;
      p1.y = p0.y + alpha * cornerMults[i2].b * innerRadii[c].height;

      p2.x = p3i.x - alpha * cornerMults[i3].a * innerRadii[c].width;
      p2.y = p3i.y - alpha * cornerMults[i3].b * innerRadii[c].height;
      builder->LineTo(p3i);
      builder->BezierTo(p2, p1, p0);
      builder->Close();
      path = builder->Finish();
      dt->Fill(path, *pattern);
    } else {
      Point c1, c2, c3, c4;

      c1.x = pc.x + cornerMults[i].a * mBorderCornerDimensions[c].width;
      c1.y = pc.y + cornerMults[i].b * mBorderCornerDimensions[c].height;
      c2 = pc;
      c3.x = pc.x + cornerMults[i3].a * mBorderCornerDimensions[c].width;
      c3.y = pc.y + cornerMults[i3].b * mBorderCornerDimensions[c].height;

      builder->MoveTo(c1);
      builder->LineTo(c2);
      builder->LineTo(c3);
      builder->LineTo(pci);
      builder->Close();

      path = builder->Finish();

      dt->Fill(path, *pattern);
    }
  }
}

void
nsCSSBorderRenderer::DrawRectangularCompositeColors()
{
  nsBorderColors *currentColors[4];
  mContext->SetLineWidth(1);
  memcpy(currentColors, mCompositeColors, sizeof(nsBorderColors*) * 4);
  gfxRect rect = mOuterRect;
  rect.Deflate(0.5);

  const twoFloats cornerAdjusts[4] = { { +0.5,  0   },
                                        {    0, +0.5 },
                                        { -0.5,  0   },
                                        {    0, -0.5 } };

  for (int i = 0; i < mBorderWidths[0]; i++) {
    NS_FOR_CSS_SIDES(side) {
      int sideNext = (side + 1) % 4;

      gfxPoint firstCorner = rect.CCWCorner(side);
      firstCorner.x += cornerAdjusts[side].a;
      firstCorner.y += cornerAdjusts[side].b;
      gfxPoint secondCorner = rect.CWCorner(side);
      secondCorner.x -= cornerAdjusts[side].a;
      secondCorner.y -= cornerAdjusts[side].b;

      gfxRGBA currentColor =
        currentColors[side] ? gfxRGBA(currentColors[side]->mColor)
                            : gfxRGBA(mBorderColors[side]);

      mContext->SetColor(currentColor);
      mContext->NewPath();
      mContext->MoveTo(firstCorner);
      mContext->LineTo(secondCorner);
      mContext->Stroke();

      mContext->NewPath();
      gfxPoint cornerTopLeft = rect.CWCorner(side);
      cornerTopLeft.x -= 0.5;
      cornerTopLeft.y -= 0.5;
      mContext->Rectangle(gfxRect(cornerTopLeft, gfxSize(1, 1)));
      gfxRGBA nextColor =
        currentColors[sideNext] ? gfxRGBA(currentColors[sideNext]->mColor)
                                : gfxRGBA(mBorderColors[sideNext]);

      gfxRGBA cornerColor((currentColor.r + nextColor.r) / 2.0,
                          (currentColor.g + nextColor.g) / 2.0,
                          (currentColor.b + nextColor.b) / 2.0,
                          (currentColor.a + nextColor.a) / 2.0);
      mContext->SetColor(cornerColor);
      mContext->Fill();

      if (side != 0) {
        
        if (currentColors[side] && currentColors[side]->mNext) {
          currentColors[side] = currentColors[side]->mNext;
        }
      }
    }
    
    if (currentColors[0] && currentColors[0]->mNext) {
      currentColors[0] = currentColors[0]->mNext;
    }
    rect.Deflate(1);
  }
}

void
nsCSSBorderRenderer::DrawBorders()
{
  bool forceSeparateCorners = false;

  
  
  bool tlBordersSame = AreBorderSideFinalStylesSame(SIDE_BIT_TOP | SIDE_BIT_LEFT);
  bool brBordersSame = AreBorderSideFinalStylesSame(SIDE_BIT_BOTTOM | SIDE_BIT_RIGHT);
  bool allBordersSame = AreBorderSideFinalStylesSame(SIDE_BITS_ALL);
  if (allBordersSame &&
      ((mCompositeColors[0] == nullptr &&
       (mBorderStyles[0] == NS_STYLE_BORDER_STYLE_NONE ||
        mBorderStyles[0] == NS_STYLE_BORDER_STYLE_HIDDEN ||
        mBorderColors[0] == NS_RGBA(0,0,0,0))) ||
       (mCompositeColors[0] &&
        (mCompositeColors[0]->mColor == NS_RGBA(0,0,0,0) &&
         !mCompositeColors[0]->mNext))))
  {
    
    
    
    
    
    return;
  }

  gfxMatrix mat = mContext->CurrentMatrix();

  
  
  
  
  if (mat.HasNonTranslation()) {
    if (!mat.HasNonAxisAlignedTransform()) {
      
      
      mAvoidStroke = true;
    }
  } else {
    mat._31 = floor(mat._31 + 0.5);
    mat._32 = floor(mat._32 + 0.5);
    mContext->SetMatrix(mat);

    
    
    
    
    mOuterRect.Round();
    mInnerRect.Round();
  }

  bool allBordersSameWidth = AllBordersSameWidth();

  if (allBordersSameWidth && mBorderWidths[0] == 0.0) {
    
    
    return;
  }

  bool allBordersSolid;

  
  
  
  if (allBordersSame &&
      mCompositeColors[0] == nullptr &&
      allBordersSameWidth &&
      mBorderStyles[0] == NS_STYLE_BORDER_STYLE_SOLID &&
      mNoBorderRadius &&
      !mAvoidStroke)
  {
    
    SetupStrokeStyle(NS_SIDE_TOP);
    gfxRect rect = mOuterRect;
    rect.Deflate(mBorderWidths[0] / 2.0);
    mContext->NewPath();
    mContext->Rectangle(rect);
    mContext->Stroke();
    return;
  }

  if (allBordersSame &&
      mCompositeColors[0] == nullptr &&
      allBordersSameWidth &&
      mBorderStyles[0] == NS_STYLE_BORDER_STYLE_DOTTED &&
      mBorderWidths[0] < 3 &&
      mNoBorderRadius &&
      !mAvoidStroke)
  {
    
    
    SetupStrokeStyle(NS_SIDE_TOP);

    gfxFloat dash = mBorderWidths[0];
    mContext->SetDash(&dash, 1, 0.5);
    mContext->SetAntialiasMode(AntialiasMode::NONE);
    gfxRect rect = mOuterRect;
    rect.Deflate(mBorderWidths[0] / 2.0);
    mContext->NewPath();
    mContext->Rectangle(rect);
    mContext->Stroke();
    return;
  }


  if (allBordersSame &&
      mCompositeColors[0] == nullptr &&
      mBorderStyles[0] == NS_STYLE_BORDER_STYLE_SOLID &&
      !mAvoidStroke &&
      !mNoBorderRadius)
  {
    
    SetupStrokeStyle(NS_SIDE_TOP);

    RoundedRect borderInnerRect(mOuterRect, mBorderRadii);
    borderInnerRect.Deflate(mBorderWidths[NS_SIDE_TOP],
                      mBorderWidths[NS_SIDE_BOTTOM],
                      mBorderWidths[NS_SIDE_LEFT],
                      mBorderWidths[NS_SIDE_RIGHT]);

    
    
    
    
    
    
    
    
    
    mContext->NewPath();
    mContext->RoundedRectangle(mOuterRect, mBorderRadii, true);
    mContext->RoundedRectangle(borderInnerRect.rect, borderInnerRect.corners, false);
    mContext->Fill();
    return;
  }

  bool hasCompositeColors;

  allBordersSolid = AllBordersSolid(&hasCompositeColors);
  
  
  if (allBordersSolid &&
      allBordersSameWidth &&
      mCompositeColors[0] == nullptr &&
      mBorderWidths[0] == 1 &&
      mNoBorderRadius &&
      !mAvoidStroke)
  {
    DrawSingleWidthSolidBorder();
    return;
  }

  if (allBordersSolid && !hasCompositeColors &&
      !mAvoidStroke)
  {
    DrawNoCompositeColorSolidBorder();
    return;
  }

  if (allBordersSolid &&
      allBordersSameWidth &&
      mNoBorderRadius &&
      !mAvoidStroke)
  {
    
    DrawRectangularCompositeColors();
    return;
  }

  
  
  
  if (allBordersSame && mCompositeColors[0] != nullptr && !mNoBorderRadius)
    forceSeparateCorners = true;

  PrintAsString(" mOuterRect: "), PrintAsString(mOuterRect), PrintAsStringNewline();
  PrintAsString(" mInnerRect: "), PrintAsString(mInnerRect), PrintAsStringNewline();
  PrintAsFormatString(" mBorderColors: 0x%08x 0x%08x 0x%08x 0x%08x\n", mBorderColors[0], mBorderColors[1], mBorderColors[2], mBorderColors[3]);

  
  
  mOuterRect.Condition();
  if (mOuterRect.IsEmpty())
    return;

  mInnerRect.Condition();
  int dashedSides = 0;

  NS_FOR_CSS_SIDES(i) {
    uint8_t style = mBorderStyles[i];
    if (style == NS_STYLE_BORDER_STYLE_DASHED ||
        style == NS_STYLE_BORDER_STYLE_DOTTED)
    {
      
      
      allBordersSame = false;
      dashedSides |= (1 << i);
    }
  }

  PrintAsFormatString(" allBordersSame: %d dashedSides: 0x%02x\n", allBordersSame, dashedSides);

  if (allBordersSame && !forceSeparateCorners) {
    
    DrawBorderSides(SIDE_BITS_ALL);
    PrintAsStringNewline("---------------- (1)");
  } else {
    PROFILER_LABEL("nsCSSBorderRenderer", "DrawBorders::multipass",
      js::ProfileEntry::Category::GRAPHICS);

    

    








    NS_FOR_CSS_CORNERS(corner) {
      const mozilla::css::Side sides[2] = { mozilla::css::Side(corner), PREV_SIDE(corner) };

      if (!IsZeroSize(mBorderRadii[corner]))
        continue;

      if (mBorderWidths[sides[0]] == 1.0 && mBorderWidths[sides[1]] == 1.0) {
        if (corner == NS_CORNER_TOP_LEFT || corner == NS_CORNER_TOP_RIGHT)
          mBorderCornerDimensions[corner].width = 0.0;
        else
          mBorderCornerDimensions[corner].height = 0.0;
      }
    }

    
    NS_FOR_CSS_CORNERS(corner) {
      
      if (IsZeroSize(mBorderCornerDimensions[corner]))
        continue;

      const int sides[2] = { corner, PREV_SIDE(corner) };
      int sideBits = (1 << sides[0]) | (1 << sides[1]);

      bool simpleCornerStyle = mCompositeColors[sides[0]] == nullptr &&
                                 mCompositeColors[sides[1]] == nullptr &&
                                 AreBorderSideFinalStylesSame(sideBits);

      
      
      
      if (simpleCornerStyle &&
          IsZeroSize(mBorderRadii[corner]) &&
          IsSolidCornerStyle(mBorderStyles[sides[0]], corner))
      {
        mContext->NewPath();
        DoCornerSubPath(corner);
        mContext->SetColor(MakeBorderColor(mBorderColors[sides[0]],
                                           mBackgroundColor,
                                           BorderColorStyleForSolidCorner(mBorderStyles[sides[0]], corner)));
        mContext->Fill();
        continue;
      }

      mContext->Save();

      
      mContext->NewPath();
      DoCornerSubPath(corner);
      mContext->Clip();

      if (simpleCornerStyle) {
        
        
        DrawBorderSides(sideBits);
      } else {
        
        
        
        
        
        
        
        
        
        

        for (int cornerSide = 0; cornerSide < 2; cornerSide++) {
          mozilla::css::Side side = mozilla::css::Side(sides[cornerSide]);
          uint8_t style = mBorderStyles[side];

          PrintAsFormatString("corner: %d cornerSide: %d side: %d style: %d\n", corner, cornerSide, side, style);

          mContext->Save();

          mContext->NewPath();
          DoSideClipSubPath(side);
          mContext->Clip();

          DrawBorderSides(1 << side);

          mContext->Restore();
        }
      }

      mContext->Restore();

      PrintAsStringNewline();
    }

    
    
    
    
    
    
    
    int alreadyDrawnSides = 0;
    if (mOneUnitBorder &&
        mNoBorderRadius &&
        (dashedSides & (SIDE_BIT_TOP | SIDE_BIT_LEFT)) == 0)
    {
      if (tlBordersSame) {
        DrawBorderSides(SIDE_BIT_TOP | SIDE_BIT_LEFT);
        alreadyDrawnSides |= (SIDE_BIT_TOP | SIDE_BIT_LEFT);
      }

      if (brBordersSame && (dashedSides & (SIDE_BIT_BOTTOM | SIDE_BIT_RIGHT)) == 0) {
        DrawBorderSides(SIDE_BIT_BOTTOM | SIDE_BIT_RIGHT);
        alreadyDrawnSides |= (SIDE_BIT_BOTTOM | SIDE_BIT_RIGHT);
      }
    }

    
    NS_FOR_CSS_SIDES (side) {
      
      if (alreadyDrawnSides & (1 << side))
        continue;

      
      if (mBorderWidths[side] == 0.0 ||
          mBorderStyles[side] == NS_STYLE_BORDER_STYLE_HIDDEN ||
          mBorderStyles[side] == NS_STYLE_BORDER_STYLE_NONE)
        continue;


      if (dashedSides & (1 << side)) {
        
        
        DrawDashedSide (side);

        PrintAsStringNewline("---------------- (d)");
        continue;
      }

      
      
      
      
      
      
      
      
      mContext->Save();
      mContext->NewPath();
      DoSideClipWithoutCornersSubPath(side);
      mContext->Clip();

      DrawBorderSides(1 << side);

      mContext->Restore();

      PrintAsStringNewline("---------------- (*)");
    }
  }
}
