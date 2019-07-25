







































#include "nsStyleConsts.h"
#include "nsIFrame.h"
#include "nsPoint.h"
#include "nsRect.h"
#include "nsIViewManager.h"
#include "nsFrameManager.h"
#include "nsStyleContext.h"
#include "nsGkAtoms.h"
#include "nsCSSAnonBoxes.h"
#include "nsTransform2D.h"
#include "nsIDeviceContext.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIScrollableFrame.h"
#include "imgIRequest.h"
#include "imgIContainer.h"
#include "nsCSSRendering.h"
#include "nsCSSColorUtils.h"
#include "nsITheme.h"
#include "nsThemeConstants.h"
#include "nsIServiceManager.h"
#include "nsIHTMLDocument.h"
#include "nsLayoutUtils.h"
#include "nsINameSpaceManager.h"
#include "nsBlockFrame.h"

#include "gfxContext.h"

#include "nsCSSRenderingBorders.h"



























static void ComputeBorderCornerDimensions(const gfxRect& aOuterRect,
                                          const gfxRect& aInnerRect,
                                          const gfxCornerSizes& aRadii,
                                          gfxCornerSizes *aDimsResult);


#define NEXT_SIDE(_s) mozilla::css::Side(((_s) + 1) & 3)
#define PREV_SIDE(_s) mozilla::css::Side(((_s) + 3) & 3)



static gfxRGBA MakeBorderColor(const gfxRGBA& aColor,
                               const gfxRGBA& aBackgroundColor,
                               BorderColorStyle aBorderColorStyle);





static gfxRGBA ComputeColorForLine(PRUint32 aLineIndex,
                                   const BorderColorStyle* aBorderColorStyle,
                                   PRUint32 aBorderColorStyleCount,
                                   nscolor aBorderColor,
                                   nscolor aBackgroundColor);

static gfxRGBA ComputeCompositeColorForLine(PRUint32 aLineIndex,
                                            const nsBorderColors* aBorderColors);



static PRBool
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

nsCSSBorderRenderer::nsCSSBorderRenderer(PRInt32 aAppUnitsPerPixel,
                                         gfxContext* aDestContext,
                                         gfxRect& aOuterRect,
                                         const PRUint8* aBorderStyles,
                                         const gfxFloat* aBorderWidths,
                                         gfxCornerSizes& aBorderRadii,
                                         const nscolor* aBorderColors,
                                         nsBorderColors* const* aCompositeColors,
                                         PRIntn aSkipSides,
                                         nscolor aBackgroundColor)
  : mContext(aDestContext),
    mOuterRect(aOuterRect),
    mBorderStyles(aBorderStyles),
    mBorderWidths(aBorderWidths),
    mBorderRadii(aBorderRadii),
    mBorderColors(aBorderColors),
    mCompositeColors(aCompositeColors),
    mAUPP(aAppUnitsPerPixel),
    mSkipSides(aSkipSides),
    mBackgroundColor(aBackgroundColor)
{
  if (!mCompositeColors) {
    static nsBorderColors * const noColors[4] = { NULL };
    mCompositeColors = &noColors[0];
  }

  mInnerRect = mOuterRect;
  mInnerRect.Inset(mBorderStyles[0] != NS_STYLE_BORDER_STYLE_NONE ? mBorderWidths[0] : 0,
                   mBorderStyles[1] != NS_STYLE_BORDER_STYLE_NONE ? mBorderWidths[1] : 0,
                   mBorderStyles[2] != NS_STYLE_BORDER_STYLE_NONE ? mBorderWidths[2] : 0,
                   mBorderStyles[3] != NS_STYLE_BORDER_STYLE_NONE ? mBorderWidths[3] : 0);

  ComputeBorderCornerDimensions(mOuterRect, mInnerRect, mBorderRadii, &mBorderCornerDimensions);

  mOneUnitBorder = CheckFourFloatsEqual(mBorderWidths, 1.0);
  mNoBorderRadius = AllCornersZeroSize(mBorderRadii);
}

 void
nsCSSBorderRenderer::ComputeInnerRadii(const gfxCornerSizes& aRadii,
                                       const gfxFloat *aBorderSizes,
                                       gfxCornerSizes *aInnerRadiiRet)
{
  gfxCornerSizes& iRadii = *aInnerRadiiRet;

  iRadii[C_TL].width = NS_MAX(0.0, aRadii[C_TL].width - aBorderSizes[NS_SIDE_LEFT]);
  iRadii[C_TL].height = NS_MAX(0.0, aRadii[C_TL].height - aBorderSizes[NS_SIDE_TOP]);

  iRadii[C_TR].width = NS_MAX(0.0, aRadii[C_TR].width - aBorderSizes[NS_SIDE_RIGHT]);
  iRadii[C_TR].height = NS_MAX(0.0, aRadii[C_TR].height - aBorderSizes[NS_SIDE_TOP]);

  iRadii[C_BR].width = NS_MAX(0.0, aRadii[C_BR].width - aBorderSizes[NS_SIDE_RIGHT]);
  iRadii[C_BR].height = NS_MAX(0.0, aRadii[C_BR].height - aBorderSizes[NS_SIDE_BOTTOM]);

  iRadii[C_BL].width = NS_MAX(0.0, aRadii[C_BL].width - aBorderSizes[NS_SIDE_LEFT]);
  iRadii[C_BL].height = NS_MAX(0.0, aRadii[C_BL].height - aBorderSizes[NS_SIDE_BOTTOM]);
}

 void
ComputeBorderCornerDimensions(const gfxRect& aOuterRect,
                              const gfxRect& aInnerRect,
                              const gfxCornerSizes& aRadii,
                              gfxCornerSizes *aDimsRet)
{
  gfxFloat topWidth = aInnerRect.pos.y - aOuterRect.pos.y;
  gfxFloat leftWidth = aInnerRect.pos.x - aOuterRect.pos.x;
  gfxFloat rightWidth = aOuterRect.size.width - aInnerRect.size.width - leftWidth;
  gfxFloat bottomWidth = aOuterRect.size.height - aInnerRect.size.height - topWidth;

  if (AllCornersZeroSize(aRadii)) {
    
    (*aDimsRet)[C_TL] = gfxSize(leftWidth, topWidth);
    (*aDimsRet)[C_TR] = gfxSize(rightWidth, topWidth);
    (*aDimsRet)[C_BR] = gfxSize(rightWidth, bottomWidth);
    (*aDimsRet)[C_BL] = gfxSize(leftWidth, bottomWidth);
  } else {
    
    
    
    (*aDimsRet)[C_TL] = gfxSize(ceil(NS_MAX(leftWidth, aRadii[C_TL].width)),
                                ceil(NS_MAX(topWidth, aRadii[C_TL].height)));
    (*aDimsRet)[C_TR] = gfxSize(ceil(NS_MAX(rightWidth, aRadii[C_TR].width)),
                                ceil(NS_MAX(topWidth, aRadii[C_TR].height)));
    (*aDimsRet)[C_BR] = gfxSize(ceil(NS_MAX(rightWidth, aRadii[C_BR].width)),
                                ceil(NS_MAX(bottomWidth, aRadii[C_BR].height)));
    (*aDimsRet)[C_BL] = gfxSize(ceil(NS_MAX(leftWidth, aRadii[C_BL].width)),
                                ceil(NS_MAX(bottomWidth, aRadii[C_BL].height)));
  }
}

PRBool
nsCSSBorderRenderer::AreBorderSideFinalStylesSame(PRUint8 aSides)
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
      return PR_FALSE;
  }

  

  switch (mBorderStyles[firstStyle]) {
    case NS_STYLE_BORDER_STYLE_GROOVE:
    case NS_STYLE_BORDER_STYLE_RIDGE:
    case NS_STYLE_BORDER_STYLE_INSET:
    case NS_STYLE_BORDER_STYLE_OUTSET:
      return ((aSides & ~(SIDE_BIT_TOP | SIDE_BIT_LEFT)) == 0 ||
              (aSides & ~(SIDE_BIT_BOTTOM | SIDE_BIT_RIGHT)) == 0);
  }

  return PR_TRUE;
}

PRBool
nsCSSBorderRenderer::IsSolidCornerStyle(PRUint8 aStyle, mozilla::css::Corner aCorner)
{
  switch (aStyle) {
    case NS_STYLE_BORDER_STYLE_DOTTED:
    case NS_STYLE_BORDER_STYLE_DASHED:
    case NS_STYLE_BORDER_STYLE_SOLID:
      return PR_TRUE;

    case NS_STYLE_BORDER_STYLE_INSET:
    case NS_STYLE_BORDER_STYLE_OUTSET:
      return (aCorner == NS_CORNER_TOP_LEFT || aCorner == NS_CORNER_BOTTOM_RIGHT);

    case NS_STYLE_BORDER_STYLE_GROOVE:
    case NS_STYLE_BORDER_STYLE_RIDGE:
      return mOneUnitBorder && (aCorner == NS_CORNER_TOP_LEFT || aCorner == NS_CORNER_BOTTOM_RIGHT);

    case NS_STYLE_BORDER_STYLE_DOUBLE:
      return mOneUnitBorder;

    default:
      return PR_FALSE;
  }
}

BorderColorStyle
nsCSSBorderRenderer::BorderColorStyleForSolidCorner(PRUint8 aStyle, mozilla::css::Corner aCorner)
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
    offset.x = mOuterRect.size.width - mBorderCornerDimensions[aCorner].width;
  if (aCorner == C_BR || aCorner == C_BL)
    offset.y = mOuterRect.size.height - mBorderCornerDimensions[aCorner].height;

  mContext->Rectangle(gfxRect(mOuterRect.pos + offset,
                              mBorderCornerDimensions[aCorner]));
}

void
nsCSSBorderRenderer::DoSideClipWithoutCornersSubPath(mozilla::css::Side aSide)
{
  gfxPoint offset(0.0, 0.0);

  
  
  
  
  
  if (aSide == NS_SIDE_TOP) {
    offset.x = mBorderCornerDimensions[C_TL].width;
  } else if (aSide == NS_SIDE_RIGHT) {
    offset.x = mOuterRect.size.width - mBorderWidths[NS_SIDE_RIGHT];
    offset.y = mBorderCornerDimensions[C_TR].height;
  } else if (aSide == NS_SIDE_BOTTOM) {
    offset.x = mBorderCornerDimensions[C_BL].width;
    offset.y = mOuterRect.size.height - mBorderWidths[NS_SIDE_BOTTOM];
  } else if (aSide == NS_SIDE_LEFT) {
    offset.y = mBorderCornerDimensions[C_TL].height;
  }

  
  
  
  
  gfxSize sideCornerSum = mBorderCornerDimensions[mozilla::css::Corner(aSide)]
                        + mBorderCornerDimensions[mozilla::css::Corner(NEXT_SIDE(aSide))];
  gfxRect rect(mOuterRect.pos + offset,
               mOuterRect.size - sideCornerSum);

  if (aSide == NS_SIDE_TOP || aSide == NS_SIDE_BOTTOM)
    rect.size.height = mBorderWidths[aSide];
  else
    rect.size.width = mBorderWidths[aSide];

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
      gfxFloat k = NS_MIN((aMidPoint.x - aP0.x) / ps.x,
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
  PRBool isDashed      = IS_DASHED_OR_DOTTED(mBorderStyles[aSide]);
  PRBool startIsDashed = IS_DASHED_OR_DOTTED(mBorderStyles[PREV_SIDE(aSide)]);
  PRBool endIsDashed   = IS_DASHED_OR_DOTTED(mBorderStyles[NEXT_SIDE(aSide)]);
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

  gfxPoint midPoint = mInnerRect.pos + mInnerRect.size / 2.0;

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
                                     PRIntn aSides,
                                     const gfxRGBA& aColor)
{
  mContext->SetColor(aColor);
  
  

  
  
  if (!AllCornersZeroSize(aBorderRadii)) {
    gfxCornerSizes innerRadii;
    ComputeInnerRadii(aBorderRadii, aBorderSizes, &innerRadii);

    mContext->NewPath();

    
    mContext->RoundedRectangle(aOuterRect, aBorderRadii, PR_TRUE);

    
    mContext->RoundedRectangle(aInnerRect, innerRadii, PR_FALSE);

    mContext->Fill();

    return;
  }

  
  
  
  
  
  if (aSides == SIDE_BITS_ALL &&
      CheckFourFloatsEqual(aBorderSizes, aBorderSizes[0]))
  {
    gfxRect r(aOuterRect);
    r.Inset(aBorderSizes[0] / 2.0);
    mContext->SetLineWidth(aBorderSizes[0]);

    mContext->NewPath();
    mContext->Rectangle(r);
    mContext->Stroke();

    return;
  }

  
  
  

  gfxRect r[4];

  
  if (aSides & SIDE_BIT_TOP) {
    r[NS_SIDE_TOP].pos = aOuterRect.TopLeft();
    r[NS_SIDE_TOP].size.width = aOuterRect.size.width;
    r[NS_SIDE_TOP].size.height = aBorderSizes[NS_SIDE_TOP];
  }

  if (aSides & SIDE_BIT_BOTTOM) {
    r[NS_SIDE_BOTTOM].pos = aOuterRect.BottomLeft();
    r[NS_SIDE_BOTTOM].pos.y -= aBorderSizes[NS_SIDE_BOTTOM];
    r[NS_SIDE_BOTTOM].size.width = aOuterRect.size.width;
    r[NS_SIDE_BOTTOM].size.height = aBorderSizes[NS_SIDE_BOTTOM];
  }

  if (aSides & SIDE_BIT_LEFT) {
    r[NS_SIDE_LEFT].pos = aOuterRect.TopLeft();
    r[NS_SIDE_LEFT].size.width = aBorderSizes[NS_SIDE_LEFT];
    r[NS_SIDE_LEFT].size.height = aOuterRect.size.height;
  }

  if (aSides & SIDE_BIT_RIGHT) {
    r[NS_SIDE_RIGHT].pos = aOuterRect.TopRight();
    r[NS_SIDE_RIGHT].pos.x -= aBorderSizes[NS_SIDE_RIGHT];
    r[NS_SIDE_RIGHT].size.width = aBorderSizes[NS_SIDE_RIGHT];
    r[NS_SIDE_RIGHT].size.height = aOuterRect.size.height;
  }

  
  
  
  

  if ((aSides & (SIDE_BIT_TOP | SIDE_BIT_LEFT)) == (SIDE_BIT_TOP | SIDE_BIT_LEFT)) {
    
    r[NS_SIDE_LEFT].pos.y += aBorderSizes[NS_SIDE_TOP];
    r[NS_SIDE_LEFT].size.height -= aBorderSizes[NS_SIDE_TOP];
  }

  if ((aSides & (SIDE_BIT_TOP | SIDE_BIT_RIGHT)) == (SIDE_BIT_TOP | SIDE_BIT_RIGHT)) {
    
    r[NS_SIDE_TOP].size.width -= aBorderSizes[NS_SIDE_RIGHT];
  }

  if ((aSides & (SIDE_BIT_BOTTOM | SIDE_BIT_RIGHT)) == (SIDE_BIT_BOTTOM | SIDE_BIT_RIGHT)) {
    
    r[NS_SIDE_RIGHT].size.height -= aBorderSizes[NS_SIDE_BOTTOM];
  }

  if ((aSides & (SIDE_BIT_BOTTOM | SIDE_BIT_LEFT)) == (SIDE_BIT_BOTTOM | SIDE_BIT_LEFT)) {
    
    r[NS_SIDE_BOTTOM].pos.x += aBorderSizes[NS_SIDE_LEFT];
    r[NS_SIDE_BOTTOM].size.width -= aBorderSizes[NS_SIDE_LEFT];
  }

  
  for (PRUint32 i = 0; i < 4; i++) {
    if (aSides & (1 << i)) {
      mContext->NewPath();
      mContext->Rectangle(r[i]);
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
ComputeColorForLine(PRUint32 aLineIndex,
                    const BorderColorStyle* aBorderColorStyle,
                    PRUint32 aBorderColorStyleCount,
                    nscolor aBorderColor,
                    nscolor aBackgroundColor)
{
  NS_ASSERTION(aLineIndex < aBorderColorStyleCount, "Invalid lineIndex given");

  return MakeBorderColor(gfxRGBA(aBorderColor), gfxRGBA(aBackgroundColor), aBorderColorStyle[aLineIndex]);
}

gfxRGBA
ComputeCompositeColorForLine(PRUint32 aLineIndex,
                             const nsBorderColors* aBorderColors)
{
  while (aLineIndex-- && aBorderColors->mNext)
    aBorderColors = aBorderColors->mNext;

  return gfxRGBA(aBorderColors->mColor);
}

void
nsCSSBorderRenderer::DrawBorderSidesCompositeColors(PRIntn aSides, const nsBorderColors *aCompositeColors)
{
  gfxCornerSizes radii = mBorderRadii;

  
  gfxRect soRect = mOuterRect;
  gfxRect siRect;
  gfxFloat maxBorderWidth = 0;
  NS_FOR_CSS_SIDES (i) {
    maxBorderWidth = NS_MAX(maxBorderWidth, mBorderWidths[i]);
  }

  gfxFloat fakeBorderSizes[4];

  gfxRGBA lineColor;
  gfxPoint tl, br;

  gfxPoint itl = mInnerRect.TopLeft();
  gfxPoint ibr = mInnerRect.BottomRight();

  for (PRUint32 i = 0; i < PRUint32(maxBorderWidth); i++) {
    lineColor = ComputeCompositeColorForLine(i, aCompositeColors);

    siRect = soRect;
    siRect.Inset(1.0, 1.0, 1.0, 1.0);

    
    tl = siRect.TopLeft();
    br = siRect.BottomRight();

    tl.x = NS_MIN(tl.x, itl.x);
    tl.y = NS_MIN(tl.y, itl.y);

    br.x = NS_MAX(br.x, ibr.x);
    br.y = NS_MAX(br.y, ibr.y);

    siRect.pos = tl;
    siRect.size.width = br.x - tl.x;
    siRect.size.height = br.y - tl.y;

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
nsCSSBorderRenderer::DrawBorderSides(PRIntn aSides)
{
  if (aSides == 0 || (aSides & ~SIDE_BITS_ALL) != 0) {
    NS_WARNING("DrawBorderSides: invalid sides!");
    return;
  }

  PRUint8 borderRenderStyle;
  nscolor borderRenderColor;
  const nsBorderColors *compositeColors = nsnull;

  PRUint32 borderColorStyleCount = 0;
  BorderColorStyle borderColorStyleTopLeft[3], borderColorStyleBottomRight[3];
  BorderColorStyle *borderColorStyle = nsnull;

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
      borderWidths[0][i] = PRInt32(mBorderWidths[i]) / 2 + PRInt32(mBorderWidths[i]) % 2;
      borderWidths[1][i] = PRInt32(mBorderWidths[i]) / 2;
    }
  } else if (borderColorStyleCount == 3) {
    
    
    NS_FOR_CSS_SIDES (i) {
      if (mBorderWidths[i] == 1.0) {
        borderWidths[0][i] = 1.0;
        borderWidths[1][i] = borderWidths[2][i] = 0.0;
      } else {
        PRInt32 rest = PRInt32(mBorderWidths[i]) % 3;
        borderWidths[0][i] = borderWidths[2][i] = borderWidths[1][i] = (PRInt32(mBorderWidths[i]) - rest) / 3;

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
    
    
    siRect.Inset(borderWidths[i]);

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

  PRUint8 style = mBorderStyles[aSide];
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
    SF("DrawDashedSide: style: %d!!\n", style);
    NS_ERROR("DrawDashedSide called with style other than DASHED or DOTTED; someone's not playing nice");
    return;
  }

  SF("dash: %f %f\n", dash[0], dash[1]);

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
    if (mCompositeColors[i] != nsnull) {
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

  float gradientOffset;
  
  if (mContext->OriginalSurface()->GetType() == gfxASurface::SurfaceTypeD2D ||
      mContext->OriginalSurface()->GetType() == gfxASurface::SurfaceTypeQuartz)
  {
    
    
    
    gradientOffset = 0;
  } else {
    
    gradientOffset = 0.25 / sqrt(pow(mBorderWidths[cornerHeight[aCorner]], 2) +
                                 pow(mBorderWidths[cornerHeight[aCorner]], 2));
  }

  nsRefPtr<gfxPattern> pattern = new gfxPattern(pat1.x, pat1.y, pat2.x, pat2.y);
  pattern->AddColorStop(0.5 - gradientOffset, gfxRGBA(aFirstColor));
  pattern->AddColorStop(0.5 + gradientOffset, gfxRGBA(aSecondColor));

  return pattern.forget();
}

typedef struct { gfxFloat a, b; } twoFloats;

void
nsCSSBorderRenderer::DrawSingleWidthSolidBorder()
{
  
  mContext->SetLineWidth(1);
  gfxRect rect = mOuterRect;
  rect.Inset(0.5);

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
  const gfxFloat alpha = 0.55191497064665766025;

  const twoFloats cornerMults[4] = { { -1,  0 },
                                      {  0, -1 },
                                      { +1,  0 },
                                      {  0, +1 } };

  const twoFloats centerAdjusts[4] = { { 0, +0.5 },
                                        { -0.5, 0 },
                                        { 0, -0.5 },
                                        { +0.5, 0 } };

  gfxPoint pc, pci, p0, p1, p2, p3, pd, p3i;

  gfxCornerSizes innerRadii;
  ComputeInnerRadii(mBorderRadii, mBorderWidths, &innerRadii);

  gfxRect strokeRect = mOuterRect;
  strokeRect.Inset(mBorderWidths[0] / 2.0, mBorderWidths[1] / 2.0,
                    mBorderWidths[2] / 2.0, mBorderWidths[3] / 2.0);

  NS_FOR_CSS_CORNERS(i) {
      
    mozilla::css::Corner c = mozilla::css::Corner((i+1) % 4);
    mozilla::css::Corner prevCorner = mozilla::css::Corner(i);

    
    
    
    int i1 = (i+1) % 4;
    int i2 = (i+2) % 4;
    int i3 = (i+3) % 4;

    pc = mOuterRect.AtCorner(c);
    pci = mInnerRect.AtCorner(c);
    mContext->SetLineWidth(mBorderWidths[i]);

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

    mContext->NewPath();

    gfxPoint strokeStart, strokeEnd;

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

    mContext->MoveTo(strokeStart);
    mContext->LineTo(strokeEnd);
    mContext->SetColor(gfxRGBA(mBorderColors[i]));
    mContext->Stroke();

    if (firstColor != secondColor) {
      nsRefPtr<gfxPattern> pattern =
        CreateCornerGradient(c, firstColor, secondColor);
      mContext->SetPattern(pattern);
    } else {
      mContext->SetColor(firstColor);
    }     
    
    if (mBorderRadii[c].width > 0 && mBorderRadii[c].height > 0) {
      p0.x = pc.x + cornerMults[i].a * mBorderRadii[c].width;
      p0.y = pc.y + cornerMults[i].b * mBorderRadii[c].height;

      p3.x = pc.x + cornerMults[i3].a * mBorderRadii[c].width;
      p3.y = pc.y + cornerMults[i3].b * mBorderRadii[c].height;

      p1.x = p0.x + alpha * cornerMults[i2].a * mBorderRadii[c].width;
      p1.y = p0.y + alpha * cornerMults[i2].b * mBorderRadii[c].height;

      p2.x = p3.x - alpha * cornerMults[i3].a * mBorderRadii[c].width;
      p2.y = p3.y - alpha * cornerMults[i3].b * mBorderRadii[c].height;

      mContext->NewPath();
            
      gfxPoint cornerStart;
      cornerStart.x = pc.x + cornerMults[i].a * mBorderCornerDimensions[c].width;
      cornerStart.y = pc.y + cornerMults[i].b * mBorderCornerDimensions[c].height;

      mContext->MoveTo(cornerStart);
      mContext->LineTo(p0);

      mContext->CurveTo(p1, p2, p3);
            
      gfxPoint outerCornerEnd;
      outerCornerEnd.x = pc.x + cornerMults[i3].a * mBorderCornerDimensions[c].width;
      outerCornerEnd.y = pc.y + cornerMults[i3].b * mBorderCornerDimensions[c].height;

      mContext->LineTo(outerCornerEnd);

      p0.x = pci.x + cornerMults[i].a * innerRadii[c].width;
      p0.y = pci.y + cornerMults[i].b * innerRadii[c].height;

      p3i.x = pci.x + cornerMults[i3].a * innerRadii[c].width;
      p3i.y = pci.y + cornerMults[i3].b * innerRadii[c].height;

      p1.x = p0.x + alpha * cornerMults[i2].a * innerRadii[c].width;
      p1.y = p0.y + alpha * cornerMults[i2].b * innerRadii[c].height;

      p2.x = p3i.x - alpha * cornerMults[i3].a * innerRadii[c].width;
      p2.y = p3i.y - alpha * cornerMults[i3].b * innerRadii[c].height;
      mContext->LineTo(p3i);
      mContext->CurveTo(p2, p1, p0);
      mContext->ClosePath();
      mContext->Fill();
    } else {
      gfxPoint c1, c2, c3, c4;

      c1.x = pc.x + cornerMults[i].a * mBorderCornerDimensions[c].width;
      c1.y = pc.y + cornerMults[i].b * mBorderCornerDimensions[c].height;
      c2 = pc;
      c3.x = pc.x + cornerMults[i3].a * mBorderCornerDimensions[c].width;
      c3.y = pc.y + cornerMults[i3].b * mBorderCornerDimensions[c].height;

      mContext->NewPath();
      mContext->MoveTo(c1);
      mContext->LineTo(c2);
      mContext->LineTo(c3);
      mContext->LineTo(pci);
      mContext->ClosePath();

      mContext->Fill();
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
  rect.Inset(0.5);

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
    rect.Inset(1);
  }
}

void
nsCSSBorderRenderer::DrawBorders()
{
  PRBool forceSeparateCorners = PR_FALSE;

  
  
  PRBool tlBordersSame = AreBorderSideFinalStylesSame(SIDE_BIT_TOP | SIDE_BIT_LEFT);
  PRBool brBordersSame = AreBorderSideFinalStylesSame(SIDE_BIT_BOTTOM | SIDE_BIT_RIGHT);
  PRBool allBordersSame = AreBorderSideFinalStylesSame(SIDE_BITS_ALL);
  if (allBordersSame &&
      ((mCompositeColors[0] == NULL &&
       (mBorderStyles[0] == NS_STYLE_BORDER_STYLE_NONE ||
        mBorderStyles[0] == NS_STYLE_BORDER_STYLE_HIDDEN ||
        mBorderColors[0] == NS_RGBA(0,0,0,0))) ||
       (mCompositeColors[0] &&
        (mCompositeColors[0]->mColor == NS_RGBA(0,0,0,0) &&
         !mCompositeColors[0]->mNext))))
  {
    
    
    
    
    
    return;
  }

  
  
  
  mOuterRect.Round();
  mInnerRect.Round();

  gfxMatrix mat = mContext->CurrentMatrix();

  
  
  
  
  if (!mat.HasNonTranslation()) {
    mat.x0 = floor(mat.x0 + 0.5);
    mat.y0 = floor(mat.y0 + 0.5);
    mContext->SetMatrix(mat);
  }

  PRBool allBordersSameWidth = AllBordersSameWidth();

  PRBool allBordersSolid;
  bool noCornerOutsideCenter = true;

  
  
  
  if (allBordersSame &&
      mCompositeColors[0] == NULL &&
      allBordersSameWidth &&
      mBorderStyles[0] == NS_STYLE_BORDER_STYLE_SOLID &&
      mNoBorderRadius)
  {
    
    SetupStrokeStyle(NS_SIDE_TOP);
    gfxRect rect = mOuterRect;
    rect.Inset(mBorderWidths[0] / 2.0);
    mContext->NewPath();
    mContext->Rectangle(rect);
    mContext->Stroke();
    return;
  }

  if (allBordersSame &&
      mCompositeColors[0] == NULL &&
      allBordersSameWidth &&
      mBorderStyles[0] == NS_STYLE_BORDER_STYLE_DOTTED &&
      mBorderWidths[0] < 3 &&
      mNoBorderRadius)
  {
    
    
    SetupStrokeStyle(NS_SIDE_TOP);
    
    gfxFloat dash = mBorderWidths[0];
    mContext->SetDash(&dash, 1, 0.5);
    mContext->SetAntialiasMode(gfxContext::MODE_ALIASED);
    gfxRect rect = mOuterRect;
    rect.Inset(mBorderWidths[0] / 2.0);
    mContext->NewPath();
    mContext->Rectangle(rect);
    mContext->Stroke();
    return;
  }

  
  if (allBordersSame &&
      allBordersSameWidth &&
      mCompositeColors[0] == NULL &&
      mBorderStyles[0] == NS_STYLE_BORDER_STYLE_SOLID)
  {
    NS_FOR_CSS_CORNERS(i) {
      if (mBorderRadii[i].width <= mBorderWidths[0]) {
        noCornerOutsideCenter = false;
      }
      if (mBorderRadii[i].height <= mBorderWidths[0]) {
        noCornerOutsideCenter = false;
      }
    }

    
    

    if (noCornerOutsideCenter) {
      
      SetupStrokeStyle(NS_SIDE_TOP);
      mOuterRect.Inset(mBorderWidths[0] / 2.0);
      NS_FOR_CSS_CORNERS(corner) {
        if (mBorderRadii.sizes[corner].height == 0 || mBorderRadii.sizes[corner].width == 0) {
          continue;
        }
        mBorderRadii.sizes[corner].width -= mBorderWidths[0] / 2;
        mBorderRadii.sizes[corner].height -= mBorderWidths[0] / 2;
      }

      mContext->NewPath();
      mContext->RoundedRectangle(mOuterRect, mBorderRadii);
      mContext->Stroke();
      return;
    }
  }

  bool hasCompositeColors;

  allBordersSolid = AllBordersSolid(&hasCompositeColors);
  
  
  if (allBordersSolid &&
      allBordersSameWidth &&
      mCompositeColors[0] == NULL &&
      mBorderWidths[0] == 1 &&
      mNoBorderRadius)
  {
    DrawSingleWidthSolidBorder();
    return;
  }

  if (allBordersSolid && !hasCompositeColors)
  {
    DrawNoCompositeColorSolidBorder();
    return;
  }

  if (allBordersSolid &&
      allBordersSameWidth &&
      mNoBorderRadius)
  {
    
    DrawRectangularCompositeColors();
    return;
  }

  
  
  
  if (allBordersSame && mCompositeColors[0] != nsnull && !mNoBorderRadius)
    forceSeparateCorners = PR_TRUE;

  S(" mOuterRect: "), S(mOuterRect), SN();
  S(" mInnerRect: "), S(mInnerRect), SN();
  SF(" mBorderColors: 0x%08x 0x%08x 0x%08x 0x%08x\n", mBorderColors[0], mBorderColors[1], mBorderColors[2], mBorderColors[3]);

  
  
  mOuterRect.Condition();
  if (mOuterRect.IsEmpty())
    return;

  mInnerRect.Condition();
  PRIntn dashedSides = 0;

  NS_FOR_CSS_SIDES(i) {
    PRUint8 style = mBorderStyles[i];
    if (style == NS_STYLE_BORDER_STYLE_DASHED ||
        style == NS_STYLE_BORDER_STYLE_DOTTED)
    {
      
      
      allBordersSame = PR_FALSE;
      dashedSides |= (1 << i);
    }
  }

  SF(" allBordersSame: %d dashedSides: 0x%02x\n", allBordersSame, dashedSides);

  if (allBordersSame && !forceSeparateCorners) {
    
    DrawBorderSides(SIDE_BITS_ALL);
    SN("---------------- (1)");
  } else {
    

    








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

      const PRIntn sides[2] = { corner, PREV_SIDE(corner) };
      PRIntn sideBits = (1 << sides[0]) | (1 << sides[1]);

      PRBool simpleCornerStyle = mCompositeColors[sides[0]] == NULL &&
                                 mCompositeColors[sides[1]] == NULL &&
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
        
        
        
        
        
        
        
        
        

        mContext->PushGroup(gfxASurface::CONTENT_COLOR_ALPHA);
        mContext->SetOperator(gfxContext::OPERATOR_ADD);

        for (int cornerSide = 0; cornerSide < 2; cornerSide++) {
          mozilla::css::Side side = mozilla::css::Side(sides[cornerSide]);
          PRUint8 style = mBorderStyles[side];

          SF("corner: %d cornerSide: %d side: %d style: %d\n", corner, cornerSide, side, style);

          mContext->Save();

          mContext->NewPath();
          DoSideClipSubPath(side);
          mContext->Clip();

          DrawBorderSides(1 << side);

          mContext->Restore();
        }

        mContext->PopGroupToSource();
        mContext->SetOperator(gfxContext::OPERATOR_OVER);
        mContext->Paint();
      }

      mContext->Restore();

      SN();
    }

    
    
    
    
    
    
    
    PRIntn alreadyDrawnSides = 0;
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

        SN("---------------- (d)");
        continue;
      }

      
      
      
      
      
      
      
      
      mContext->Save();
      mContext->NewPath();
      DoSideClipWithoutCornersSubPath(side);
      mContext->Clip();

      DrawBorderSides(1 << side);

      mContext->Restore();

      SN("---------------- (*)");
    }
  }
}
