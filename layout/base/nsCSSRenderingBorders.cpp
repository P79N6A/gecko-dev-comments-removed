






































#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsIImage.h"
#include "nsIFrame.h"
#include "nsPoint.h"
#include "nsRect.h"
#include "nsIViewManager.h"
#include "nsIPresShell.h"
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
#include "gfxIImageFrame.h"
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

static void ComputeInnerRadii(const gfxCornerSizes& radii,
                              const gfxFloat *borderSizes,
                              gfxCornerSizes *innerRadii);


#define NEXT_SIDE(_s) (((_s) + 1) & 3)
#define PREV_SIDE(_s) (((_s) + 3) & 3)



typedef enum {
  BorderColorStyleNone,
  BorderColorStyleSolid,
  BorderColorStyleLight,
  BorderColorStyleDark
} BorderColorStyle;

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
  return IsZeroSize(corners[0]) &&
    IsZeroSize(corners[1]) &&
    IsZeroSize(corners[2]) &&
    IsZeroSize(corners[3]);
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
                                         nscolor aBackgroundColor,
                                         const gfxRect* aGapRect)
  : mAUPP(aAppUnitsPerPixel),
    mContext(aDestContext),
    mOuterRect(aOuterRect),
    mBorderStyles(aBorderStyles),
    mBorderWidths(aBorderWidths),
    mBorderRadii(aBorderRadii),
    mBorderColors(aBorderColors),
    mCompositeColors(aCompositeColors),
    mSkipSides(aSkipSides),
    mBackgroundColor(aBackgroundColor),
    mGapRect(aGapRect)
{
  if (!mCompositeColors) {
    static nsBorderColors * const noColors[4] = { NULL };
    mCompositeColors = &noColors[0];
  }

  mInnerRect = mOuterRect;
  mInnerRect.Inset(mBorderWidths[0], mBorderWidths[1], mBorderWidths[2], mBorderWidths[3]);

  ComputeBorderCornerDimensions(mOuterRect, mInnerRect, mBorderRadii, &mBorderCornerDimensions);
}

void
ComputeInnerRadii(const gfxCornerSizes& aRadii,
                  const gfxFloat *aBorderSizes,
                  gfxCornerSizes *aInnerRadiiRet)
{
  gfxCornerSizes& iRadii = *aInnerRadiiRet;

  iRadii[C_TL].width = PR_MAX(0.0, aRadii[C_TL].width - aBorderSizes[NS_SIDE_LEFT]);
  iRadii[C_TL].height = PR_MAX(0.0, aRadii[C_TL].height - aBorderSizes[NS_SIDE_TOP]);

  iRadii[C_TR].width = PR_MAX(0.0, aRadii[C_TR].width - aBorderSizes[NS_SIDE_RIGHT]);
  iRadii[C_TR].height = PR_MAX(0.0, aRadii[C_TR].height - aBorderSizes[NS_SIDE_TOP]);

  iRadii[C_BR].width = PR_MAX(0.0, aRadii[C_BR].width - aBorderSizes[NS_SIDE_RIGHT]);
  iRadii[C_BR].height = PR_MAX(0.0, aRadii[C_BR].height - aBorderSizes[NS_SIDE_BOTTOM]);

  iRadii[C_BL].width = PR_MAX(0.0, aRadii[C_BL].width - aBorderSizes[NS_SIDE_LEFT]);
  iRadii[C_BL].height = PR_MAX(0.0, aRadii[C_BL].height - aBorderSizes[NS_SIDE_BOTTOM]);
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
    
    
    
    (*aDimsRet)[C_TL] = gfxSize(ceil(PR_MAX(leftWidth, aRadii[C_TL].width)),
                                ceil(PR_MAX(topWidth, aRadii[C_TL].height)));
    (*aDimsRet)[C_TR] = gfxSize(ceil(PR_MAX(rightWidth, aRadii[C_TR].width)),
                                ceil(PR_MAX(topWidth, aRadii[C_TR].height)));
    (*aDimsRet)[C_BR] = gfxSize(ceil(PR_MAX(rightWidth, aRadii[C_BR].width)),
                                ceil(PR_MAX(bottomWidth, aRadii[C_BR].height)));
    (*aDimsRet)[C_BL] = gfxSize(ceil(PR_MAX(leftWidth, aRadii[C_BL].width)),
                                ceil(PR_MAX(bottomWidth, aRadii[C_BL].height)));
  }
}



static PRBool
AreCompositeColorsEqual(nsBorderColors *aA, nsBorderColors *aB)
{
  if (aA == aB)
    return PR_TRUE;

  if (!aA || !aB)
    return PR_FALSE;

  while (aA && aB) {
    if (aA->mTransparent != aB->mTransparent)
      return PR_FALSE;
    if (!aA->mTransparent && (aA->mColor != aB->mColor))
      return PR_FALSE;
    aA = aA->mNext;
    aB = aB->mNext;
  }

  
  
  return (aA == aB);
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

    if (mBorderStyles[firstStyle] != mBorderStyles[i] ||
        mBorderColors[firstStyle] != mBorderColors[i] ||
        !AreCompositeColorsEqual(mCompositeColors[firstStyle], mCompositeColors[i]))
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


void
nsCSSBorderRenderer::DoCornerClipSubPath(PRUint8 aCorner)
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
nsCSSBorderRenderer::DoSideClipWithoutCornersSubPath(PRUint8 aSide)
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

  
  
  
  
  gfxSize sideCornerSum = mBorderCornerDimensions[aSide] + mBorderCornerDimensions[NEXT_SIDE(aSide)];
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

  if (ps.x != 0.0 && ps.y != 0.0) {
    gfxFloat k = PR_MIN((aMidPoint.x - aP0.x) / ps.x,
                        (aMidPoint.y - aP1.y) / ps.y);
    aP1 = aP0 + ps * k;
  }
}

void
nsCSSBorderRenderer::DoSideClipSubPath(PRUint8 aSide)
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

  if (!IsZeroSize(mBorderRadii[aSide]))
    startType = SIDE_CLIP_TRAPEZOID_FULL;
  else if (startIsDashed && isDashed)
    startType = SIDE_CLIP_RECTANGLE;

  if (!IsZeroSize(mBorderRadii[NEXT_SIDE(aSide)]))
    endType = SIDE_CLIP_TRAPEZOID_FULL;
  else if (endIsDashed && isDashed)
    endType = SIDE_CLIP_RECTANGLE;

  gfxPoint midPoint = mInnerRect.pos + mInnerRect.size / 2.0;

  start[0] = mOuterRect.Corner(aSide);
  start[1] = mInnerRect.Corner(aSide);

  end[0] = mOuterRect.Corner(NEXT_SIDE(aSide));
  end[1] = mInnerRect.Corner(NEXT_SIDE(aSide));

  if (startType == SIDE_CLIP_TRAPEZOID_FULL) {
    MaybeMoveToMidPoint(start[0], start[1], midPoint);
  } else if (startType == SIDE_CLIP_RECTANGLE) {
    if (aSide == NS_SIDE_TOP || aSide == NS_SIDE_BOTTOM)
      start[1] = gfxPoint(mOuterRect.Corner(aSide).x, mInnerRect.Corner(aSide).y);
    else
      start[1] = gfxPoint(mInnerRect.Corner(aSide).x, mOuterRect.Corner(aSide).y);
  }

  if (endType == SIDE_CLIP_TRAPEZOID_FULL) {
    MaybeMoveToMidPoint(end[0], end[1], midPoint);
  } else if (endType == SIDE_CLIP_RECTANGLE) {
    if (aSide == NS_SIDE_TOP || aSide == NS_SIDE_BOTTOM)
      end[0] = gfxPoint(mInnerRect.Corner(NEXT_SIDE(aSide)).x, mOuterRect.Corner(NEXT_SIDE(aSide)).y);
    else
      end[0] = gfxPoint(mOuterRect.Corner(NEXT_SIDE(aSide)).x, mInnerRect.Corner(NEXT_SIDE(aSide)).y);
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

  mContext->NewPath();

  
  
  if (AllCornersZeroSize(aBorderRadii) &&
      CheckFourFloatsEqual(aBorderSizes, aBorderSizes[0]))
  {
    if (aSides == SIDE_BITS_ALL) {
      gfxRect r(aOuterRect);
      r.Inset(aBorderSizes[0] / 2.0);

      mContext->SetLineWidth(aBorderSizes[0]);

      mContext->Rectangle(r);
      mContext->Stroke();

      return;
    }

    if (aBorderSizes[0] == 1.0 && aColor.a == 1.0) {
      if (aSides == (SIDE_BIT_TOP | SIDE_BIT_LEFT)) {
          mContext->SetLineWidth(1.0);

          gfxRect r(aOuterRect);
          r.Inset(0.5, 0.0, 0.0, 0.5);

          mContext->MoveTo(r.BottomLeft());
          mContext->LineTo(r.TopLeft());
          mContext->LineTo(r.TopRight());
          mContext->Stroke();
          return;
        }

      if (aSides == (SIDE_BIT_BOTTOM | SIDE_BIT_RIGHT)) {
        mContext->SetLineWidth(1.0);

        gfxRect r(aOuterRect);
        r.Inset(0.0, 0.5, 0.5, 0.0);

        mContext->MoveTo(r.BottomLeft());
        mContext->LineTo(r.BottomRight());
        mContext->LineTo(r.TopRight());
        mContext->Stroke();
        return;
      }
    }
  }

  
  gfxCornerSizes innerRadii;
  ComputeInnerRadii(aBorderRadii, aBorderSizes, &innerRadii);

  
  mContext->RoundedRectangle(aOuterRect, aBorderRadii, PR_TRUE);

  
  mContext->RoundedRectangle(aInnerRect, innerRadii, PR_FALSE);

  mContext->Fill();
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

  if (aBorderColors->mTransparent)
    return gfxRGBA(0.0, 0.0, 0.0, 0.0);

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
    maxBorderWidth = PR_MAX(maxBorderWidth, mBorderWidths[i]);
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

    tl.x = PR_MIN(tl.x, itl.x);
    tl.y = PR_MIN(tl.y, itl.y);

    br.x = PR_MAX(br.x, ibr.x);
    br.y = PR_MAX(br.y, ibr.y);

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

  
  
  
  
  
  
  
  if (CheckFourFloatsEqual(mBorderWidths, 1.0)) {
    if (borderRenderStyle == NS_STYLE_BORDER_STYLE_RIDGE ||
        borderRenderStyle == NS_STYLE_BORDER_STYLE_GROOVE ||
        borderRenderStyle == NS_STYLE_BORDER_STYLE_DOUBLE)
      borderRenderStyle = NS_STYLE_BORDER_STYLE_SOLID;
  }

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
nsCSSBorderRenderer::DrawDashedSide(PRUint8 aSide)
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

  gfxPoint start = mOuterRect.Corner(aSide);
  gfxPoint end = mOuterRect.Corner(NEXT_SIDE(aSide));

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
nsCSSBorderRenderer::DrawBorders()
{
  PRBool forceSeparateCorners = PR_FALSE;

  
  
  PRBool allBordersSame = AreBorderSideFinalStylesSame(SIDE_BITS_ALL);
  if (allBordersSame &&
      mCompositeColors[0] == NULL &&
      (mBorderStyles[0] == NS_STYLE_BORDER_STYLE_NONE ||
       mBorderStyles[0] == NS_STYLE_BORDER_STYLE_HIDDEN ||
       mBorderColors[0] == NS_RGBA(0,0,0,0)))
  {
    
    
    
    
    return;
  }

  
  
  
  if (allBordersSame && mCompositeColors[0] != nsnull && !AllCornersZeroSize(mBorderRadii))
    forceSeparateCorners = PR_TRUE;

  
  
  
  mOuterRect.Round();
  mInnerRect.Round();

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

    
    
    if (style & NS_STYLE_BORDER_STYLE_RULES_MARKER)
      return;
  }

  SF(" allBordersSame: %d dashedSides: 0x%02x\n", allBordersSame, dashedSides);

  
  
  
  
  gfxMatrix mat = mContext->CurrentMatrix();
  if (!mat.HasNonTranslation()) {
    mat.x0 = floor(mat.x0 + 0.5);
    mat.y0 = floor(mat.y0 + 0.5);
    mContext->SetMatrix(mat);
  }

  
  mContext->NewPath();
  mContext->Rectangle(mOuterRect);

  if (mGapRect) {
    
    
    mContext->MoveTo(mGapRect->pos);
    mContext->LineTo(mGapRect->pos + gfxSize(0.0, mGapRect->size.height));
    mContext->LineTo(mGapRect->pos + mGapRect->size);
    mContext->LineTo(mGapRect->pos + gfxSize(mGapRect->size.width, 0.0));
    mContext->ClosePath();
  }

  mContext->Clip();

  if (allBordersSame && !forceSeparateCorners) {
    
    DrawBorderSides(SIDE_BITS_ALL);
    SN("---------------- (1)");
  } else {
    

    
    for (int corner = 0; corner < gfxCorner::NUM_CORNERS; corner++) {
      const PRIntn sides[2] = { corner, PREV_SIDE(corner) };
      PRIntn sideBits = (1 << sides[0]) | (1 << sides[1]);

      mContext->Save();

      
      mContext->NewPath();
      DoCornerClipSubPath(corner);
      mContext->Clip();

      if (dashedSides == 0 &&
          mCompositeColors[sides[0]] == NULL &&
          mCompositeColors[sides[1]] == NULL &&
          AreBorderSideFinalStylesSame(sideBits))
      {
        
        DrawBorderSides(sideBits);
      } else {
        
        
        
        
        
        
        
        
        

        mContext->PushGroup(gfxASurface::CONTENT_COLOR_ALPHA);
        mContext->SetOperator(gfxContext::OPERATOR_ADD);

        for (int cornerSide = 0; cornerSide < 2; cornerSide++) {
          PRUint8 side = sides[cornerSide];
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

    
    NS_FOR_CSS_SIDES (side) {
      mContext->Save();
      mContext->NewPath();
      DoSideClipWithoutCornersSubPath(side);
      mContext->Clip();

      if (dashedSides & (1 << side)) {
        DrawDashedSide (side);
        SN("---------------- (d)");
      } else {
        DrawBorderSides(1 << side);
        SN("---------------- (*)");
      }

      mContext->Restore();
    }
  }
}
