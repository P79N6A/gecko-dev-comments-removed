













































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
#include "nsIInterfaceRequestorUtils.h"
#include "gfxPlatform.h"
#include "gfxImageSurface.h"
#include "nsStyleStructInlines.h"
#include "nsCSSFrameConstructor.h"

#include "nsCSSRenderingBorders.h"





struct InlineBackgroundData
{
  InlineBackgroundData()
      : mFrame(nsnull), mBlockFrame(nsnull)
  {
  }

  ~InlineBackgroundData()
  {
  }

  void Reset()
  {
    mBoundingBox.SetRect(0,0,0,0);
    mContinuationPoint = mLineContinuationPoint = mUnbrokenWidth = 0;
    mFrame = mBlockFrame = nsnull;
  }

  nsRect GetContinuousRect(nsIFrame* aFrame)
  {
    SetFrame(aFrame);

    nscoord x;
    if (mBidiEnabled) {
      x = mLineContinuationPoint;

      
      
      
      PRBool isRtlBlock = (mBlockFrame->GetStyleVisibility()->mDirection ==
                           NS_STYLE_DIRECTION_RTL);      
      nscoord curOffset = aFrame->GetOffsetTo(mBlockFrame).x;

      nsIFrame* inlineFrame = aFrame->GetPrevContinuation();
      
      
      while (inlineFrame && !inlineFrame->GetNextInFlow() &&
             AreOnSameLine(aFrame, inlineFrame)) {
        nscoord frameXOffset = inlineFrame->GetOffsetTo(mBlockFrame).x;
        if(isRtlBlock == (frameXOffset >= curOffset)) {
          x += inlineFrame->GetSize().width;
        }
        inlineFrame = inlineFrame->GetPrevContinuation();
      }

      inlineFrame = aFrame->GetNextContinuation();
      while (inlineFrame && !inlineFrame->GetPrevInFlow() &&
             AreOnSameLine(aFrame, inlineFrame)) {
        nscoord frameXOffset = inlineFrame->GetOffsetTo(mBlockFrame).x;
        if(isRtlBlock == (frameXOffset >= curOffset)) {
          x += inlineFrame->GetSize().width;
        }
        inlineFrame = inlineFrame->GetNextContinuation();
      }
      if (isRtlBlock) {
        
        x += aFrame->GetSize().width;
        
        
        
        x = mUnbrokenWidth - x;
      }
    } else {
      x = mContinuationPoint;
    }

    
    
    
    return nsRect(-x, 0, mUnbrokenWidth, mFrame->GetSize().height);
  }

  nsRect GetBoundingRect(nsIFrame* aFrame)
  {
    SetFrame(aFrame);

    
    
    
    
    
    nsRect boundingBox(mBoundingBox);
    nsPoint point = mFrame->GetPosition();
    boundingBox.MoveBy(-point.x, -point.y);

    return boundingBox;
  }

protected:
  nsIFrame*     mFrame;
  nscoord       mContinuationPoint;
  nscoord       mUnbrokenWidth;
  nsRect        mBoundingBox;

  PRBool        mBidiEnabled;
  nsBlockFrame* mBlockFrame;
  nscoord       mLineContinuationPoint;
  
  void SetFrame(nsIFrame* aFrame)
  {
    NS_PRECONDITION(aFrame, "Need a frame");

    nsIFrame *prevContinuation = aFrame->GetPrevContinuation();

    if (!prevContinuation || mFrame != prevContinuation) {
      
      Reset();
      Init(aFrame);
      return;
    }

    
    
    mContinuationPoint += mFrame->GetSize().width;

    
    if (mBidiEnabled &&
        (aFrame->GetPrevInFlow() || !AreOnSameLine(mFrame, aFrame))) {
       mLineContinuationPoint = mContinuationPoint;
    }
    
    mFrame = aFrame;
  }

  void Init(nsIFrame* aFrame)
  {    
    
    
    nsIFrame* inlineFrame = aFrame->GetPrevContinuation();

    while (inlineFrame) {
      nsRect rect = inlineFrame->GetRect();
      mContinuationPoint += rect.width;
      mUnbrokenWidth += rect.width;
      mBoundingBox.UnionRect(mBoundingBox, rect);
      inlineFrame = inlineFrame->GetPrevContinuation();
    }

    
    
    inlineFrame = aFrame;
    while (inlineFrame) {
      nsRect rect = inlineFrame->GetRect();
      mUnbrokenWidth += rect.width;
      mBoundingBox.UnionRect(mBoundingBox, rect);
      inlineFrame = inlineFrame->GetNextContinuation();
    }

    mFrame = aFrame;

    mBidiEnabled = aFrame->PresContext()->BidiEnabled();
    if (mBidiEnabled) {
      
      nsIFrame* frame = aFrame;
      nsresult rv = NS_ERROR_FAILURE;
      while (frame &&
             frame->IsFrameOfType(nsIFrame::eLineParticipant) &&
             NS_FAILED(rv)) {
        frame = frame->GetParent();
        rv = frame->QueryInterface(kBlockFrameCID, (void**)&mBlockFrame);
      }
      NS_ASSERTION(NS_SUCCEEDED(rv) && mBlockFrame, "Cannot find containing block.");

      mLineContinuationPoint = mContinuationPoint;
    }
  }
  
  PRBool AreOnSameLine(nsIFrame* aFrame1, nsIFrame* aFrame2) {
    
    PRBool isValid1, isValid2;
    nsBlockInFlowLineIterator it1(mBlockFrame, aFrame1, &isValid1);
    nsBlockInFlowLineIterator it2(mBlockFrame, aFrame2, &isValid2);
    return isValid1 && isValid2 && it1.GetLine() == it2.GetLine();
  }
};


static void DrawBorderImage(nsPresContext* aPresContext,
                            nsIRenderingContext& aRenderingContext,
                            nsIFrame* aForFrame,
                            const nsRect& aBorderArea,
                            const nsStyleBorder& aBorderStyle);

static void DrawBorderImageSide(gfxContext *aThebesContext,
                                nsIDeviceContext* aDeviceContext,
                                imgIContainer* aImage,
                                gfxRect& aDestRect,
                                gfxSize aInterSize,
                                gfxRect& aSourceRect,
                                PRUint8 aHFillType,
                                PRUint8 aVFillType);

static void PaintBackgroundColor(nsPresContext* aPresContext,
                                 nsIRenderingContext& aRenderingContext,
                                 nsIFrame* aForFrame,
                                 const nsRect& aBgClipArea,
                                 const nsStyleBackground& aColor,
                                 const nsStyleBorder& aBorder,
                                 PRBool aCanPaintNonWhite);

static nscolor MakeBevelColor(PRIntn whichSide, PRUint8 style,
                              nscolor aBackgroundColor,
                              nscolor aBorderColor);

static gfxRect GetTextDecorationRectInternal(const gfxPoint& aPt,
                                             const gfxSize& aLineSize,
                                             const gfxFloat aAscent,
                                             const gfxFloat aOffset,
                                             const PRUint8 aDecoration,
                                             const PRUint8 aStyle);


static PRBool GetBorderRadiusTwips(const nsStyleCorners& aBorderRadius,
                                   const nscoord& aFrameWidth,
                                   nscoord aTwipsRadii[8]);

static InlineBackgroundData* gInlineBGData = nsnull;


nsresult nsCSSRendering::Init()
{  
  NS_ASSERTION(!gInlineBGData, "Init called twice");
  gInlineBGData = new InlineBackgroundData();
  if (!gInlineBGData)
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}


void nsCSSRendering::Shutdown()
{
  delete gInlineBGData;
  gInlineBGData = nsnull;
}




static nscolor
MakeBevelColor(PRIntn whichSide, PRUint8 style,
               nscolor aBackgroundColor, nscolor aBorderColor)
{

  nscolor colors[2];
  nscolor theColor;

  
  
  NS_GetSpecial3DColors(colors, aBackgroundColor, aBorderColor);
 
  if ((style == NS_STYLE_BORDER_STYLE_OUTSET) ||
      (style == NS_STYLE_BORDER_STYLE_RIDGE)) {
    
    switch (whichSide) {
    case NS_SIDE_BOTTOM: whichSide = NS_SIDE_TOP;    break;
    case NS_SIDE_RIGHT:  whichSide = NS_SIDE_LEFT;   break;
    case NS_SIDE_TOP:    whichSide = NS_SIDE_BOTTOM; break;
    case NS_SIDE_LEFT:   whichSide = NS_SIDE_RIGHT;  break;
    }
  }

  switch (whichSide) {
  case NS_SIDE_BOTTOM:
    theColor = colors[1];
    break;
  case NS_SIDE_RIGHT:
    theColor = colors[1];
    break;
  case NS_SIDE_TOP:
    theColor = colors[0];
    break;
  case NS_SIDE_LEFT:
  default:
    theColor = colors[0];
    break;
  }
  return theColor;
}

nscolor
nsCSSRendering::TransformColor(nscolor  aMapColor,PRBool aNoBackGround)
{
PRUint16  hue,sat,value;
nscolor   newcolor;

  newcolor = aMapColor;
  if (PR_TRUE == aNoBackGround){
    
    NS_RGB2HSV(newcolor,hue,sat,value);
    
    
    
    
    
    
    if (value > sat) {
      value = sat;
      
      NS_HSV2RGB(newcolor,hue,sat,value);
    }
  }
  return newcolor;
}





static gfxRect
RectToGfxRect(const nsRect& rect, nscoord twipsPerPixel)
{
  return gfxRect(gfxFloat(rect.x) / twipsPerPixel,
                 gfxFloat(rect.y) / twipsPerPixel,
                 gfxFloat(rect.width) / twipsPerPixel,
                 gfxFloat(rect.height) / twipsPerPixel);
}









static void
ComputePixelRadii(const nscoord *aTwipsRadii,
                  const nsRect& outerRect,
                  PRIntn skipSides,
                  nscoord twipsPerPixel,
                  gfxCornerSizes *oBorderRadii)
{
  nscoord twipsRadii[8];
  memcpy(twipsRadii, aTwipsRadii, sizeof twipsRadii);

  if (skipSides & SIDE_BIT_TOP) {
    twipsRadii[NS_CORNER_TOP_LEFT_X] = 0;
    twipsRadii[NS_CORNER_TOP_LEFT_Y] = 0;
    twipsRadii[NS_CORNER_TOP_RIGHT_X] = 0;
    twipsRadii[NS_CORNER_TOP_RIGHT_Y] = 0;
  }

  if (skipSides & SIDE_BIT_RIGHT) {
    twipsRadii[NS_CORNER_TOP_RIGHT_X] = 0;
    twipsRadii[NS_CORNER_TOP_RIGHT_Y] = 0;
    twipsRadii[NS_CORNER_BOTTOM_RIGHT_X] = 0;
    twipsRadii[NS_CORNER_BOTTOM_RIGHT_Y] = 0;
  }

  if (skipSides & SIDE_BIT_BOTTOM) {
    twipsRadii[NS_CORNER_BOTTOM_RIGHT_X] = 0;
    twipsRadii[NS_CORNER_BOTTOM_RIGHT_Y] = 0;
    twipsRadii[NS_CORNER_BOTTOM_LEFT_X] = 0;
    twipsRadii[NS_CORNER_BOTTOM_LEFT_Y] = 0;
  }

  if (skipSides & SIDE_BIT_LEFT) {
    twipsRadii[NS_CORNER_BOTTOM_LEFT_X] = 0;
    twipsRadii[NS_CORNER_BOTTOM_LEFT_Y] = 0;
    twipsRadii[NS_CORNER_TOP_LEFT_X] = 0;
    twipsRadii[NS_CORNER_TOP_LEFT_Y] = 0;
  }

  gfxFloat radii[8];
  NS_FOR_CSS_HALF_CORNERS(corner)
    radii[corner] = twipsRadii[corner] / twipsPerPixel;

  
  
  gfxFloat maxWidth = outerRect.width / twipsPerPixel;
  gfxFloat maxHeight = outerRect.height / twipsPerPixel;
  gfxFloat f = 1.0f;
  NS_FOR_CSS_SIDES(side) {
    PRUint32 hc1 = NS_SIDE_TO_HALF_CORNER(side, PR_FALSE, PR_TRUE);
    PRUint32 hc2 = NS_SIDE_TO_HALF_CORNER(side, PR_TRUE, PR_TRUE);
    gfxFloat length = NS_SIDE_IS_VERTICAL(side) ? maxHeight : maxWidth;
    gfxFloat sum = radii[hc1] + radii[hc2];
    
    if (length < sum)
      f = PR_MIN(f, length/sum);
  }
  if (f < 1.0) {
    NS_FOR_CSS_HALF_CORNERS(corner) {
      radii[corner] *= f;
    }
  }

  (*oBorderRadii)[C_TL] = gfxSize(radii[NS_CORNER_TOP_LEFT_X],
                                  radii[NS_CORNER_TOP_LEFT_Y]);
  (*oBorderRadii)[C_TR] = gfxSize(radii[NS_CORNER_TOP_RIGHT_X],
                                  radii[NS_CORNER_TOP_RIGHT_Y]);
  (*oBorderRadii)[C_BR] = gfxSize(radii[NS_CORNER_BOTTOM_RIGHT_X],
                                  radii[NS_CORNER_BOTTOM_RIGHT_Y]);
  (*oBorderRadii)[C_BL] = gfxSize(radii[NS_CORNER_BOTTOM_LEFT_X],
                                  radii[NS_CORNER_BOTTOM_LEFT_Y]);
}

void
nsCSSRendering::PaintBorder(nsPresContext* aPresContext,
                            nsIRenderingContext& aRenderingContext,
                            nsIFrame* aForFrame,
                            const nsRect& aDirtyRect,
                            const nsRect& aBorderArea,
                            const nsStyleBorder& aBorderStyle,
                            nsStyleContext* aStyleContext,
                            PRIntn aSkipSides)
{
  nsMargin            border;
  nscoord             twipsRadii[8];
  nsCompatibility     compatMode = aPresContext->CompatibilityMode();

  SN("++ PaintBorder");

  
  
  
  const nsStyleDisplay* displayData = aStyleContext->GetStyleDisplay();
  if (displayData->mAppearance) {
    nsITheme *theme = aPresContext->GetTheme();
    if (theme && theme->ThemeSupportsWidget(aPresContext, aForFrame, displayData->mAppearance))
      return; 
  }

  if (aBorderStyle.IsBorderImageLoaded()) {
    DrawBorderImage(aPresContext, aRenderingContext, aForFrame,
                    aBorderArea, aBorderStyle);
    return;
  }
  
  
  const nsStyleColor* ourColor = aStyleContext->GetStyleColor();

  
  
  const nsStyleBackground* bgColor = nsCSSRendering::FindNonTransparentBackground
    (aStyleContext, compatMode == eCompatibility_NavQuirks ? PR_TRUE : PR_FALSE);

  border = aBorderStyle.GetComputedBorder();
  if ((0 == border.left) && (0 == border.right) &&
      (0 == border.top) && (0 == border.bottom)) {
    
    return;
  }

  GetBorderRadiusTwips(aBorderStyle.mBorderRadius, aForFrame->GetSize().width,
                       twipsRadii);

  
  if (aSkipSides & SIDE_BIT_TOP) border.top = 0;
  if (aSkipSides & SIDE_BIT_RIGHT) border.right = 0;
  if (aSkipSides & SIDE_BIT_BOTTOM) border.bottom = 0;
  if (aSkipSides & SIDE_BIT_LEFT) border.left = 0;

  
  nsRect outerRect(aBorderArea);

  SF(" outerRect: %d %d %d %d\n", outerRect.x, outerRect.y, outerRect.width, outerRect.height);

  

  
  nscoord twipsPerPixel = aPresContext->DevPixelsToAppUnits(1);

  
  gfxRect oRect(RectToGfxRect(outerRect, twipsPerPixel));

  
  gfxFloat borderWidths[4] = { border.top / twipsPerPixel,
                               border.right / twipsPerPixel,
                               border.bottom / twipsPerPixel,
                               border.left / twipsPerPixel };

  
  gfxCornerSizes borderRadii;
  ComputePixelRadii(twipsRadii, outerRect, aSkipSides, twipsPerPixel,
                    &borderRadii);

  PRUint8 borderStyles[4];
  nscolor borderColors[4];
  nsBorderColors *compositeColors[4];

  
  NS_FOR_CSS_SIDES (i) {
    PRBool foreground;
    borderStyles[i] = aBorderStyle.GetBorderStyle(i);
    aBorderStyle.GetBorderColor(i, borderColors[i], foreground);
    aBorderStyle.GetCompositeColors(i, &compositeColors[i]);

    if (foreground)
      borderColors[i] = ourColor->mColor;
  }

  SF(" borderStyles: %d %d %d %d\n", borderStyles[0], borderStyles[1], borderStyles[2], borderStyles[3]);

  
  gfxContext *ctx = aRenderingContext.ThebesContext();

  ctx->Save();

#if 0
  
  ctx->Save();
  ctx->Rectangle(oRect);
  ctx->SetColor(gfxRGBA(1.0, 0.0, 0.0, 0.5));
  ctx->Fill();
  ctx->Restore();
#endif

  

  nsCSSBorderRenderer br(twipsPerPixel,
                         ctx,
                         oRect,
                         borderStyles,
                         borderWidths,
                         borderRadii,
                         borderColors,
                         compositeColors,
                         aSkipSides,
                         bgColor->mBackgroundColor);
  br.DrawBorders();

  ctx->Restore();

  SN();
}

static nsRect
GetOutlineInnerRect(nsIFrame* aFrame)
{
  nsRect* savedOutlineInnerRect = static_cast<nsRect*>
    (aFrame->GetProperty(nsGkAtoms::outlineInnerRectProperty));
  if (savedOutlineInnerRect)
    return *savedOutlineInnerRect;
  return aFrame->GetOverflowRect();
}

void
nsCSSRendering::PaintOutline(nsPresContext* aPresContext,
                             nsIRenderingContext& aRenderingContext,
                             nsIFrame* aForFrame,
                             const nsRect& aDirtyRect,
                             const nsRect& aBorderArea,
                             const nsStyleBorder& aBorderStyle,
                             const nsStyleOutline& aOutlineStyle,
                             nsStyleContext* aStyleContext)
{
  nscoord             twipsRadii[8];

  
  const nsStyleColor* ourColor = aStyleContext->GetStyleColor();

  nscoord width;
  aOutlineStyle.GetOutlineWidth(width);

  if (width == 0) {
    
    return;
  }

  const nsStyleBackground* bgColor = nsCSSRendering::FindNonTransparentBackground
    (aStyleContext, PR_FALSE);

  
  GetBorderRadiusTwips(aOutlineStyle.mOutlineRadius, aBorderArea.width,
                       twipsRadii);

  
  
  
  
  
  
  nsIFrame *frameForArea = aForFrame;
  do {
    nsIAtom *pseudoType = frameForArea->GetStyleContext()->GetPseudoType();
    if (pseudoType != nsCSSAnonBoxes::mozAnonymousBlock &&
        pseudoType != nsCSSAnonBoxes::mozAnonymousPositionedBlock)
      break;
    
    frameForArea = frameForArea->GetFirstChild(nsnull);
    NS_ASSERTION(frameForArea, "anonymous block with no children?");
  } while (frameForArea);
  nsRect innerRect; 
  if (frameForArea == aForFrame) {
    innerRect = GetOutlineInnerRect(aForFrame);
  } else {
    for (; frameForArea; frameForArea = frameForArea->GetNextSibling()) {
      
      
      
      
      nsRect r(GetOutlineInnerRect(frameForArea) +
               frameForArea->GetOffsetTo(aForFrame));
      innerRect.UnionRect(innerRect, r);
    }
  }

  innerRect += aBorderArea.TopLeft();
  nscoord offset = aOutlineStyle.mOutlineOffset;
  innerRect.Inflate(offset, offset);
  
  
  
  
  
  if (innerRect.Contains(aDirtyRect))
    return;

  nsRect outerRect = innerRect;
  outerRect.Inflate(width, width);

  
  nscoord twipsPerPixel = aPresContext->DevPixelsToAppUnits(1);

  
  gfxRect oRect(RectToGfxRect(outerRect, twipsPerPixel));

  
  nsMargin outlineMargin(width, width, width, width);
  gfxCornerSizes outlineRadii;
  ComputePixelRadii(twipsRadii, outerRect, 0, twipsPerPixel,
                    &outlineRadii);

  PRUint8 outlineStyle = aOutlineStyle.GetOutlineStyle();
  PRUint8 outlineStyles[4] = { outlineStyle,
                               outlineStyle,
                               outlineStyle,
                               outlineStyle };

  nscolor outlineColor;
  
  
  if (!aOutlineStyle.GetOutlineColor(outlineColor))
    outlineColor = ourColor->mColor;
  nscolor outlineColors[4] = { outlineColor,
                               outlineColor,
                               outlineColor,
                               outlineColor };

  
  gfxFloat outlineWidths[4] = { width / twipsPerPixel,
                                width / twipsPerPixel,
                                width / twipsPerPixel,
                                width / twipsPerPixel };

  
  gfxContext *ctx = aRenderingContext.ThebesContext();

  ctx->Save();

  nsCSSBorderRenderer br(twipsPerPixel,
                         ctx,
                         oRect,
                         outlineStyles,
                         outlineWidths,
                         outlineRadii,
                         outlineColors,
                         nsnull, 0,
                         bgColor->mBackgroundColor);
  br.DrawBorders();

  ctx->Restore();

  SN();
}

void
nsCSSRendering::PaintFocus(nsPresContext* aPresContext,
                           nsIRenderingContext& aRenderingContext,
                           const nsRect& aFocusRect,
                           nscolor aColor)
{
  nscoord oneCSSPixel = nsPresContext::CSSPixelsToAppUnits(1);
  nscoord oneDevPixel = aPresContext->DevPixelsToAppUnits(1);

  gfxRect focusRect(RectToGfxRect(aFocusRect, oneDevPixel));

  gfxCornerSizes focusRadii;
  {
    nscoord twipsRadii[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    ComputePixelRadii(twipsRadii, aFocusRect, 0, oneDevPixel, &focusRadii);
  }
  gfxFloat focusWidths[4] = { oneCSSPixel / oneDevPixel,
                              oneCSSPixel / oneDevPixel,
                              oneCSSPixel / oneDevPixel,
                              oneCSSPixel / oneDevPixel };

  PRUint8 focusStyles[4] = { NS_STYLE_BORDER_STYLE_DOTTED,
                             NS_STYLE_BORDER_STYLE_DOTTED,
                             NS_STYLE_BORDER_STYLE_DOTTED,
                             NS_STYLE_BORDER_STYLE_DOTTED };
  nscolor focusColors[4] = { aColor, aColor, aColor, aColor };

  gfxContext *ctx = aRenderingContext.ThebesContext();

  ctx->Save();

  
  
  
  
  
  
  nsCSSBorderRenderer br(oneDevPixel,
                         ctx,
                         focusRect,
                         focusStyles,
                         focusWidths,
                         focusRadii,
                         focusColors,
                         nsnull, 0,
                         NS_RGB(255, 0, 0));
  br.DrawBorders();

  ctx->Restore();

  SN();
}

























static void
ComputeBackgroundAnchorPoint(const nsStyleBackground& aColor,
                             const nsRect& aOriginBounds,
                             const nsRect& aClipBounds,
                             nscoord aTileWidth, nscoord aTileHeight,
                             nsPoint& aResult)
{
  nscoord x;
  if (NS_STYLE_BG_X_POSITION_LENGTH & aColor.mBackgroundFlags) {
    x = aColor.mBackgroundXPosition.mCoord;
  }
  else if (NS_STYLE_BG_X_POSITION_PERCENT & aColor.mBackgroundFlags) {
    PRFloat64 percent = PRFloat64(aColor.mBackgroundXPosition.mFloat);
    nscoord tilePos = nscoord(percent * PRFloat64(aTileWidth));
    nscoord boxPos = nscoord(percent * PRFloat64(aOriginBounds.width));
    x = boxPos - tilePos;
  }
  else {
    x = 0;
  }
  x += aOriginBounds.x - aClipBounds.x;
  if (NS_STYLE_BG_REPEAT_X & aColor.mBackgroundRepeat) {
    
    
    
    
    if (x < 0) {
      x = -x;
      if (x < 0) {
        
        x = 0;
      }
      x %= aTileWidth;
      x = -x;
    }
    else if (x != 0) {
      x %= aTileWidth;
      if (x > 0) {
        x = x - aTileWidth;
      }
    }

    NS_POSTCONDITION((x >= -(aTileWidth - 1)) && (x <= 0), "bad computed anchor value");
  }
  aResult.x = x;

  nscoord y;
  if (NS_STYLE_BG_Y_POSITION_LENGTH & aColor.mBackgroundFlags) {
    y = aColor.mBackgroundYPosition.mCoord;
  }
  else if (NS_STYLE_BG_Y_POSITION_PERCENT & aColor.mBackgroundFlags){
    PRFloat64 percent = PRFloat64(aColor.mBackgroundYPosition.mFloat);
    nscoord tilePos = nscoord(percent * PRFloat64(aTileHeight));
    nscoord boxPos = nscoord(percent * PRFloat64(aOriginBounds.height));
    y = boxPos - tilePos;
  }
  else {
    y = 0;
  }
  y += aOriginBounds.y - aClipBounds.y;
  if (NS_STYLE_BG_REPEAT_Y & aColor.mBackgroundRepeat) {
    
    
    
    
    if (y < 0) {
      y = -y;
      if (y < 0) {
        
        y = 0;
      }
      y %= aTileHeight;
      y = -y;
    }
    else if (y != 0) {
      y %= aTileHeight;
      if (y > 0) {
        y = y - aTileHeight;
      }
    }
    
    NS_POSTCONDITION((y >= -(aTileHeight - 1)) && (y <= 0), "bad computed anchor value");
  }
  aResult.y = y;
}

const nsStyleBackground*
nsCSSRendering::FindNonTransparentBackground(nsStyleContext* aContext,
                                             PRBool aStartAtParent )
{
  NS_ASSERTION(aContext, "Cannot find NonTransparentBackground in a null context" );
  
  const nsStyleBackground* result = nsnull;
  nsStyleContext* context = nsnull;
  if (aStartAtParent) {
    context = aContext->GetParent();
  }
  if (!context) {
    context = aContext;
  }
  
  while (context) {
    result = context->GetStyleBackground();
    if (NS_GET_A(result->mBackgroundColor) > 0)
      break;

    context = context->GetParent();
  }
  return result;
}





































inline PRBool
IsCanvasFrame(nsIFrame *aFrame)
{
  nsIAtom* frameType = aFrame->GetType();
  return frameType == nsGkAtoms::canvasFrame ||
         frameType == nsGkAtoms::rootFrame ||
         frameType == nsGkAtoms::pageFrame ||
         frameType == nsGkAtoms::pageContentFrame ||
         frameType == nsGkAtoms::viewportFrame;
}

inline PRBool
FindCanvasBackground(nsIFrame* aForFrame, nsIFrame* aRootElementFrame,
                     const nsStyleBackground** aBackground)
{
  if (aRootElementFrame) {
    const nsStyleBackground* result = aRootElementFrame->GetStyleBackground();

    
    if (result->IsTransparent()) {
      nsIContent* content = aRootElementFrame->GetContent();
      
      
      
      nsIDocument* document = content->GetOwnerDoc();
      nsCOMPtr<nsIHTMLDocument> htmlDoc = do_QueryInterface(document);
      if (htmlDoc) {
        nsIContent* bodyContent = htmlDoc->GetBodyContentExternal();
        
        
        
        
        
        
        
        
        
        if (bodyContent) {
          nsIFrame *bodyFrame = aForFrame->PresContext()->GetPresShell()->
            GetPrimaryFrameFor(bodyContent);
          if (bodyFrame)
            result = bodyFrame->GetStyleBackground();
        }
      }
    }

    *aBackground = result;
  } else {
    
    
    
    *aBackground = aForFrame->GetStyleBackground();
  }
  
  return PR_TRUE;
}

inline PRBool
FindElementBackground(nsIFrame* aForFrame, nsIFrame* aRootElementFrame,
                      const nsStyleBackground** aBackground)
{
  if (aForFrame == aRootElementFrame) {
    
    return PR_FALSE;
  }

  *aBackground = aForFrame->GetStyleBackground();

  
  

  nsIContent* content = aForFrame->GetContent();
  if (!content || content->Tag() != nsGkAtoms::body)
    return PR_TRUE; 
  
  

  if (aForFrame->GetStyleContext()->GetPseudoType())
    return PR_TRUE; 

  
  nsIDocument* document = content->GetOwnerDoc();
  nsCOMPtr<nsIHTMLDocument> htmlDoc = do_QueryInterface(document);
  if (!htmlDoc)
    return PR_TRUE;

  nsIContent* bodyContent = htmlDoc->GetBodyContentExternal();
  if (bodyContent != content)
    return PR_TRUE; 

  
  
  
  if (!aRootElementFrame)
    return PR_TRUE;

  const nsStyleBackground* htmlBG = aRootElementFrame->GetStyleBackground();
  return !htmlBG->IsTransparent();
}

PRBool
nsCSSRendering::FindBackground(nsPresContext* aPresContext,
                               nsIFrame* aForFrame,
                               const nsStyleBackground** aBackground,
                               PRBool* aIsCanvas)
{
  nsIFrame* rootElementFrame =
    aPresContext->PresShell()->FrameConstructor()->GetRootElementStyleFrame();
  PRBool isCanvasFrame = IsCanvasFrame(aForFrame);
  *aIsCanvas = isCanvasFrame;
  return isCanvasFrame
      ? FindCanvasBackground(aForFrame, rootElementFrame, aBackground)
      : FindElementBackground(aForFrame, rootElementFrame, aBackground);
}

void
nsCSSRendering::DidPaint()
{
  gInlineBGData->Reset();
}

static PRBool
GetBorderRadiusTwips(const nsStyleCorners& aBorderRadius,
                     const nscoord& aFrameWidth, nscoord aTwipsRadii[8])
{
  PRBool result = PR_FALSE;
  
  
  NS_FOR_CSS_HALF_CORNERS(i) {
    const nsStyleCoord c = aBorderRadius.Get(i);

    switch (c.GetUnit()) {
      case eStyleUnit_Percent:
        aTwipsRadii[i] = (nscoord)(c.GetPercentValue() * aFrameWidth);
        break;

      case eStyleUnit_Coord:
        aTwipsRadii[i] = c.GetCoordValue();
        break;

      default:
        NS_NOTREACHED("GetBorderRadiusTwips: bad unit");
        aTwipsRadii[i] = 0;
        break;
    }

    if (aTwipsRadii[i])
      result = PR_TRUE;
  }
  return result;
}

void
nsCSSRendering::PaintBoxShadow(nsPresContext* aPresContext,
                               nsIRenderingContext& aRenderingContext,
                               nsIFrame* aForFrame,
                               const nsPoint& aForFramePt)
{
  nsMargin      borderValues;
  PRIntn        sidesToSkip;
  nsRect        frameRect;

  const nsStyleBorder* styleBorder = aForFrame->GetStyleBorder();
  borderValues = styleBorder->GetActualBorder();
  sidesToSkip = aForFrame->GetSkipSides();
  frameRect = nsRect(aForFramePt, aForFrame->GetSize());

  
  nscoord twipsRadii[8];
  PRBool hasBorderRadius = GetBorderRadiusTwips(styleBorder->mBorderRadius,
                                                frameRect.width, twipsRadii);
  nscoord twipsPerPixel = aPresContext->DevPixelsToAppUnits(1);

  gfxCornerSizes borderRadii;
  ComputePixelRadii(twipsRadii, frameRect, sidesToSkip,
                    twipsPerPixel, &borderRadii);

  gfxRect frameGfxRect = RectToGfxRect(frameRect, twipsPerPixel);
  for (PRUint32 i = styleBorder->mBoxShadow->Length(); i > 0; --i) {
    nsCSSShadowItem* shadowItem = styleBorder->mBoxShadow->ShadowAt(i - 1);
    gfxRect shadowRect(frameRect.x, frameRect.y, frameRect.width, frameRect.height);
    shadowRect.MoveBy(gfxPoint(shadowItem->mXOffset, shadowItem->mYOffset));
    shadowRect.Outset(shadowItem->mSpread);

    gfxRect shadowRectPlusBlur = shadowRect;
    shadowRect.ScaleInverse(twipsPerPixel);
    shadowRect.RoundOut();

    
    
    nscoord blurRadius = shadowItem->mRadius;
    shadowRectPlusBlur.Outset(blurRadius);
    shadowRectPlusBlur.ScaleInverse(twipsPerPixel);
    shadowRectPlusBlur.RoundOut();

    gfxContext* renderContext = aRenderingContext.ThebesContext();
    nsRefPtr<gfxContext> shadowContext;
    nsContextBoxBlur blurringArea;

    
    blurRadius /= twipsPerPixel;
    shadowContext = blurringArea.Init(shadowRect, blurRadius, 1, renderContext);
    if (!shadowContext)
      continue;

    
    nscolor shadowColor;
    if (shadowItem->mHasColor)
      shadowColor = shadowItem->mColor;
    else
      shadowColor = aForFrame->GetStyleColor()->mColor;

    renderContext->Save();
    renderContext->SetColor(gfxRGBA(shadowColor));

    
    
    renderContext->NewPath();
    renderContext->Rectangle(shadowRectPlusBlur);
    if (hasBorderRadius)
      renderContext->RoundedRectangle(frameGfxRect, borderRadii);
    else
      renderContext->Rectangle(frameGfxRect);
    renderContext->SetFillRule(gfxContext::FILL_RULE_EVEN_ODD);
    renderContext->Clip();

    
    
    
    
    
    shadowContext->NewPath();
    if (hasBorderRadius)
      shadowContext->RoundedRectangle(shadowRect, borderRadii);
    else
      shadowContext->Rectangle(shadowRect);
    shadowContext->Fill();

    blurringArea.DoPaint();
    renderContext->Restore();
  }
}

void
nsCSSRendering::PaintBackground(nsPresContext* aPresContext,
                                nsIRenderingContext& aRenderingContext,
                                nsIFrame* aForFrame,
                                const nsRect& aDirtyRect,
                                const nsRect& aBorderArea,
                                PRBool aUsePrintSettings,
                                nsRect* aBGClipRect)
{
  NS_PRECONDITION(aForFrame,
                  "Frame is expected to be provided to PaintBackground");

  PRBool isCanvas;
  const nsStyleBackground *color;
  const nsStyleBorder* border = aForFrame->GetStyleBorder();

  if (!FindBackground(aPresContext, aForFrame, &color, &isCanvas)) {
    
    
    
    
    
    if (!aForFrame->GetStyleDisplay()->mAppearance) {
      return;
    }

    nsIContent* content = aForFrame->GetContent();
    if (!content || content->GetParent()) {
      return;
    }
        
    color = aForFrame->GetStyleBackground();
  }
  if (!isCanvas) {
    PaintBackgroundWithSC(aPresContext, aRenderingContext, aForFrame,
                          aDirtyRect, aBorderArea, *color, *border,
                          aUsePrintSettings, aBGClipRect);
    return;
  }

  nsStyleBackground canvasColor(*color);

  nsIViewManager* vm = aPresContext->GetViewManager();

  if (NS_GET_A(canvasColor.mBackgroundColor) < 255) {
    
    
    
    nsIView* rView;
    vm->GetRootView(rView);
    if (!rView->GetParent() &&
        (!rView->HasWidget() ||
         rView->GetWidget()->GetTransparencyMode() == eTransparencyOpaque)) {
      nscolor backColor = aPresContext->DefaultBackgroundColor();
      NS_ASSERTION(NS_GET_A(backColor) == 255,
                   "default background color is not opaque");

      canvasColor.mBackgroundColor =
        NS_ComposeColors(backColor, canvasColor.mBackgroundColor);
    }
  }

  vm->SetDefaultBackgroundColor(canvasColor.mBackgroundColor);

  PaintBackgroundWithSC(aPresContext, aRenderingContext, aForFrame,
                        aDirtyRect, aBorderArea, canvasColor,
                        *border, aUsePrintSettings, aBGClipRect);
}

inline nscoord IntDivFloor(nscoord aDividend, nscoord aDivisor)
{
  NS_PRECONDITION(aDivisor > 0,
                  "this function only works for positive divisors");
  
  
  
  
  return (aDividend < 0 ? (aDividend - aDivisor + 1) : aDividend) / aDivisor;
}

inline nscoord IntDivCeil(nscoord aDividend, nscoord aDivisor)
{
  NS_PRECONDITION(aDivisor > 0,
                  "this function only works for positive divisors");
  
  
  
  
  return (aDividend > 0 ? (aDividend + aDivisor - 1) : aDividend) / aDivisor;
}





static nscoord
FindTileStart(nscoord aDirtyStart, nscoord aTileOffset, nscoord aTileSize)
{
  
  return aTileOffset +
         IntDivFloor(aDirtyStart - aTileOffset, aTileSize) * aTileSize;
}





static nscoord
FindTileEnd(nscoord aDirtyEnd, nscoord aTileOffset, nscoord aTileSize)
{
  
  return aTileOffset +
         IntDivCeil(aDirtyEnd - aTileOffset, aTileSize) * aTileSize;
}

static void
PixelSnapRectangle(gfxContext* aContext, nsIDeviceContext *aDC, nsRect& aRect)
{
  gfxRect tmpRect;
  tmpRect.pos.x = aDC->AppUnitsToGfxUnits(aRect.x);
  tmpRect.pos.y = aDC->AppUnitsToGfxUnits(aRect.y);
  tmpRect.size.width = aDC->AppUnitsToGfxUnits(aRect.width);
  tmpRect.size.height = aDC->AppUnitsToGfxUnits(aRect.height);
  if (aContext->UserToDevicePixelSnapped(tmpRect)) {
    tmpRect = aContext->DeviceToUser(tmpRect);
    aRect.x = aDC->GfxUnitsToAppUnits(tmpRect.pos.x);
    aRect.y = aDC->GfxUnitsToAppUnits(tmpRect.pos.y);
    aRect.width = aDC->GfxUnitsToAppUnits(tmpRect.XMost()) - aRect.x;
    aRect.height = aDC->GfxUnitsToAppUnits(tmpRect.YMost()) - aRect.y;
  }
}

static void
PixelSnapPoint(gfxContext* aContext, nsIDeviceContext *aDC, nsPoint& aPoint)
{
  gfxRect tmpRect;
  tmpRect.pos.x = aDC->AppUnitsToGfxUnits(aPoint.x);
  tmpRect.pos.y = aDC->AppUnitsToGfxUnits(aPoint.y);
  tmpRect.size.width = 0;
  tmpRect.size.height = 0;
  if (aContext->UserToDevicePixelSnapped(tmpRect)) {
    tmpRect = aContext->DeviceToUser(tmpRect);
    aPoint.x = aDC->GfxUnitsToAppUnits(tmpRect.pos.x);
    aPoint.y = aDC->GfxUnitsToAppUnits(tmpRect.pos.y);
  }
}

static PRBool
IsSolidBorderEdge(const nsStyleBorder& aBorder, PRUint32 aSide)
{
  if (aBorder.GetActualBorder().side(aSide) == 0)
    return PR_TRUE;
  if (aBorder.GetBorderStyle(aSide) != NS_STYLE_BORDER_STYLE_SOLID)
    return PR_FALSE;

  nscolor color;
  PRBool isForeground;
  aBorder.GetBorderColor(aSide, color, isForeground);
  return NS_GET_A(color) == 255;
}




static PRBool
IsSolidBorder(const nsStyleBorder& aBorder)
{
  if (aBorder.mBorderColors ||
      nsLayoutUtils::HasNonZeroCorner(aBorder.mBorderRadius))
    return PR_FALSE;
  for (PRUint32 i = 0; i < 4; ++i) {
    if (!IsSolidBorderEdge(aBorder, i))
      return PR_FALSE;
  }
  return PR_TRUE;
}

void
nsCSSRendering::PaintBackgroundWithSC(nsPresContext* aPresContext,
                                      nsIRenderingContext& aRenderingContext,
                                      nsIFrame* aForFrame,
                                      const nsRect& aDirtyRect,
                                      const nsRect& aBorderArea,
                                      const nsStyleBackground& aColor,
                                      const nsStyleBorder& aBorder,
                                      PRBool aUsePrintSettings,
                                      nsRect* aBGClipRect)
{
  NS_PRECONDITION(aForFrame,
                  "Frame is expected to be provided to PaintBackground");

  PRBool canDrawBackgroundImage = PR_TRUE;
  PRBool canDrawBackgroundColor = PR_TRUE;

  if (aUsePrintSettings) {
    canDrawBackgroundImage = aPresContext->GetBackgroundImageDraw();
    canDrawBackgroundColor = aPresContext->GetBackgroundColorDraw();
  }

  
  
  const nsStyleDisplay* displayData = aForFrame->GetStyleDisplay();
  if (displayData->mAppearance) {
    nsITheme *theme = aPresContext->GetTheme();
    if (theme && theme->ThemeSupportsWidget(aPresContext, aForFrame, displayData->mAppearance)) {
      nsRect dirty;
      dirty.IntersectRect(aDirtyRect, aBorderArea);
      theme->DrawWidgetBackground(&aRenderingContext, aForFrame, 
                                  displayData->mAppearance, aBorderArea, dirty);
      return;
    }
  }

  nsRect bgClipArea;
  if (aBGClipRect) {
    bgClipArea = *aBGClipRect;
  }
  else {
    
    bgClipArea = aBorderArea;
    
    
    if (aColor.mBackgroundClip != NS_STYLE_BG_CLIP_BORDER ||
        IsSolidBorder(aBorder)) {
      nsMargin border = aForFrame->GetUsedBorder();
      aForFrame->ApplySkipSides(border);
      bgClipArea.Deflate(border);
    }
  }

  nsIDeviceContext *dc = aPresContext->DeviceContext();
  gfxContext *ctx = aRenderingContext.ThebesContext();

  
  
  
  PixelSnapRectangle(ctx, dc, bgClipArea);

  
  
  nsRect dirtyRect;
  if (!dirtyRect.IntersectRect(bgClipArea, aDirtyRect)) {
    
    return;
  }

  
  if (!aColor.mBackgroundImage || !canDrawBackgroundImage) {
    PaintBackgroundColor(aPresContext, aRenderingContext, aForFrame, bgClipArea,
                         aColor, aBorder, canDrawBackgroundColor);
    return;
  }

  

  
  imgIRequest *req = aPresContext->LoadImage(aColor.mBackgroundImage,
                                             aForFrame);

  PRUint32 status = imgIRequest::STATUS_ERROR;
  if (req)
    req->GetImageStatus(&status);

  if (!req || !(status & imgIRequest::STATUS_FRAME_COMPLETE) || !(status & imgIRequest::STATUS_SIZE_AVAILABLE)) {
    PaintBackgroundColor(aPresContext, aRenderingContext, aForFrame, bgClipArea,
                         aColor, aBorder, canDrawBackgroundColor);
    return;
  }

  nsCOMPtr<imgIContainer> image;
  req->GetImage(getter_AddRefs(image));

  nsSize imageSize;
  image->GetWidth(&imageSize.width);
  image->GetHeight(&imageSize.height);

  imageSize.width = nsPresContext::CSSPixelsToAppUnits(imageSize.width);
  imageSize.height = nsPresContext::CSSPixelsToAppUnits(imageSize.height);

  req = nsnull;

  nsRect bgOriginArea;

  nsIAtom* frameType = aForFrame->GetType();
  if (frameType == nsGkAtoms::inlineFrame ||
      frameType == nsGkAtoms::positionedInlineFrame) {
    switch (aColor.mBackgroundInlinePolicy) {
    case NS_STYLE_BG_INLINE_POLICY_EACH_BOX:
      bgOriginArea = aBorderArea;
      break;
    case NS_STYLE_BG_INLINE_POLICY_BOUNDING_BOX:
      bgOriginArea = gInlineBGData->GetBoundingRect(aForFrame) +
                     aBorderArea.TopLeft();
      break;
    default:
      NS_ERROR("Unknown background-inline-policy value!  "
               "Please, teach me what to do.");
    case NS_STYLE_BG_INLINE_POLICY_CONTINUOUS:
      bgOriginArea = gInlineBGData->GetContinuousRect(aForFrame) +
                     aBorderArea.TopLeft();
      break;
    }
  }
  else {
    bgOriginArea = aBorderArea;
  }

  
  
  if (aColor.mBackgroundOrigin != NS_STYLE_BG_ORIGIN_BORDER) {
    nsMargin border = aForFrame->GetUsedBorder();
    aForFrame->ApplySkipSides(border);
    bgOriginArea.Deflate(border);
    if (aColor.mBackgroundOrigin != NS_STYLE_BG_ORIGIN_PADDING) {
      nsMargin padding = aForFrame->GetUsedPadding();
      aForFrame->ApplySkipSides(padding);
      bgOriginArea.Deflate(padding);
      NS_ASSERTION(aColor.mBackgroundOrigin == NS_STYLE_BG_ORIGIN_CONTENT,
                   "unknown background-origin value");
    }
  }

  
  
  PixelSnapRectangle(ctx, dc, bgOriginArea);

  
  
  
  nscoord tileWidth = imageSize.width;
  nscoord tileHeight = imageSize.height;
  PRBool  needBackgroundColor = NS_GET_A(aColor.mBackgroundColor) > 0;
  PRIntn  repeat = aColor.mBackgroundRepeat;

  switch (repeat) {
    case NS_STYLE_BG_REPEAT_X:
      break;
    case NS_STYLE_BG_REPEAT_Y:
      break;
    case NS_STYLE_BG_REPEAT_XY:
      if (needBackgroundColor) {
        
        
        nsCOMPtr<gfxIImageFrame> gfxImgFrame;
        image->GetCurrentFrame(getter_AddRefs(gfxImgFrame));
        if (gfxImgFrame) {
          gfxImgFrame->GetNeedsBackground(&needBackgroundColor);

          
          nsSize iSize;
          image->GetWidth(&iSize.width);
          image->GetHeight(&iSize.height);
          nsRect iframeRect;
          gfxImgFrame->GetRect(iframeRect);
          if (iSize.width != iframeRect.width ||
              iSize.height != iframeRect.height) {
            needBackgroundColor = PR_TRUE;
          }
        }
      }
      break;
    case NS_STYLE_BG_REPEAT_OFF:
    default:
      NS_ASSERTION(repeat == NS_STYLE_BG_REPEAT_OFF, "unknown background-repeat value");
      break;
  }

  
  if (needBackgroundColor) {
    PaintBackgroundColor(aPresContext, aRenderingContext, aForFrame, bgClipArea,
                         aColor, aBorder, canDrawBackgroundColor);
  }

  if ((tileWidth == 0) || (tileHeight == 0) || dirtyRect.IsEmpty()) {
    
    return;
  }

  nsPoint borderAreaOriginSnapped = aBorderArea.TopLeft();
  PixelSnapPoint(ctx, dc, borderAreaOriginSnapped);

  
  
  
  

  
  nsPoint anchor;
  if (NS_STYLE_BG_ATTACHMENT_FIXED == aColor.mBackgroundAttachment) {
    
    
    

    
    aPresContext->SetRenderedPositionVaryingContent();

    nsIFrame* topFrame =
      aPresContext->PresShell()->FrameManager()->GetRootFrame();
    NS_ASSERTION(topFrame, "no root frame");
    nsIFrame* pageContentFrame = nsnull;
    if (aPresContext->IsPaginated()) {
      pageContentFrame =
        nsLayoutUtils::GetClosestFrameOfType(aForFrame, nsGkAtoms::pageContentFrame);
      if (pageContentFrame) {
        topFrame = pageContentFrame;
      }
      
    }

    nsRect viewportArea = topFrame->GetRect();

    if (!pageContentFrame) {
      
      nsIScrollableFrame* scrollableFrame =
        aPresContext->PresShell()->GetRootScrollFrameAsScrollable();
      if (scrollableFrame) {
        nsMargin scrollbars = scrollableFrame->GetActualScrollbarSizes();
        viewportArea.Deflate(scrollbars);
      }
    }
     
    
    ComputeBackgroundAnchorPoint(aColor, viewportArea, viewportArea, tileWidth, tileHeight, anchor);

    
    
    anchor -= aForFrame->GetOffsetTo(topFrame);
  } else {
    if (frameType == nsGkAtoms::canvasFrame) {
      
      
      nsRect firstRootElementFrameArea;
      nsIFrame* firstRootElementFrame = aForFrame->GetFirstChild(nsnull);
      NS_ASSERTION(firstRootElementFrame, "A canvas with a background "
        "image had no child frame, which is impossible according to CSS. "
        "Make sure there isn't a background image specified on the "
        "|:viewport| pseudo-element in |html.css|.");

      
      if (firstRootElementFrame) {
        firstRootElementFrameArea = firstRootElementFrame->GetRect();

        
        const nsStyleBorder* borderStyle = firstRootElementFrame->GetStyleBorder();
        firstRootElementFrameArea.Deflate(borderStyle->GetActualBorder());

        
        ComputeBackgroundAnchorPoint(aColor, firstRootElementFrameArea +
            aBorderArea.TopLeft(), bgClipArea, tileWidth, tileHeight, anchor);
      } else {
        ComputeBackgroundAnchorPoint(aColor, bgOriginArea, bgClipArea, tileWidth, tileHeight, anchor);
      }
    } else {
      
      
      ComputeBackgroundAnchorPoint(aColor, bgOriginArea, bgClipArea, tileWidth, tileHeight, anchor);
    }

    
    anchor.x += bgClipArea.x - borderAreaOriginSnapped.x;
    anchor.y += bgClipArea.y - borderAreaOriginSnapped.y;
  }

  
  
  
  
  anchor.x = -anchor.x; anchor.y = -anchor.y;
  PixelSnapPoint(ctx, dc, anchor);
  anchor.x = -anchor.x; anchor.y = -anchor.y;

  ctx->Save();

  nscoord appUnitsPerPixel = aPresContext->DevPixelsToAppUnits(1);

  ctx->NewPath();
  ctx->Rectangle(RectToGfxRect(dirtyRect, appUnitsPerPixel), PR_TRUE);
  ctx->Clip();

  nscoord borderRadii[8];
  PRBool haveRadius = GetBorderRadiusTwips(aBorder.mBorderRadius,
                                           aForFrame->GetSize().width,
                                           borderRadii);

  if (haveRadius) {
    gfxCornerSizes radii;
    ComputePixelRadii(borderRadii, bgClipArea,
                      aForFrame ? aForFrame->GetSkipSides() : 0,
                      appUnitsPerPixel, &radii);

    gfxRect oRect(RectToGfxRect(bgClipArea, appUnitsPerPixel));
    oRect.Round();
    oRect.Condition();

    ctx->NewPath();
    ctx->RoundedRectangle(oRect, radii);
    ctx->Clip();
  }      

  

  

































































































  
  
  
  nsRect tileRect(anchor, nsSize(tileWidth, tileHeight));
  
  
  
  
  PRBool useSingleImagePath =
    tileRect.Contains(bgClipArea - borderAreaOriginSnapped);

  if (repeat & NS_STYLE_BG_REPEAT_X) {
    
    
    
    nscoord x0 = FindTileStart(dirtyRect.x - borderAreaOriginSnapped.x, anchor.x, tileWidth);
    nscoord x1 = FindTileEnd(dirtyRect.XMost() - borderAreaOriginSnapped.x, anchor.x, tileWidth);
    tileRect.x = x0;
    tileRect.width = x1 - x0;
  }
  if (repeat & NS_STYLE_BG_REPEAT_Y) {
    
    
    
    nscoord y0 = FindTileStart(dirtyRect.y - borderAreaOriginSnapped.y, anchor.y, tileHeight);
    nscoord y1 = FindTileEnd(dirtyRect.YMost() - borderAreaOriginSnapped.y, anchor.y, tileHeight);
    tileRect.y = y0;
    tileRect.height = y1 - y0;
  }

  
  nsRect absTileRect = tileRect + borderAreaOriginSnapped;
  
  nsRect drawRect;
  if (drawRect.IntersectRect(absTileRect, dirtyRect)) {
    
    
    NS_ASSERTION(drawRect.x >= absTileRect.x && drawRect.y >= absTileRect.y,
                 "Bogus intersection");
    NS_ASSERTION(drawRect.x < absTileRect.x + tileWidth,
                 "Bogus x coord for draw rect");
    NS_ASSERTION(drawRect.y < absTileRect.y + tileHeight,
                 "Bogus y coord for draw rect");
    
    nsRect sourceRect = drawRect - absTileRect.TopLeft();
    
    
    
    nsRect destRect; 
    destRect.IntersectRect(absTileRect, bgClipArea);
    nsRect subimageRect = destRect - borderAreaOriginSnapped - tileRect.TopLeft();
    if (useSingleImagePath) {
      NS_ASSERTION(sourceRect.XMost() <= tileWidth && sourceRect.YMost() <= tileHeight,
                   "We shouldn't need to tile here");
      
      
      nsLayoutUtils::DrawImage(&aRenderingContext, image,
              destRect, drawRect, &subimageRect);
    } else {
      
      
      subimageRect.ScaleRoundOutInverse(nsIDeviceContext::AppUnitsPerCSSPixel());
      aRenderingContext.DrawTile(image, absTileRect.x, absTileRect.y,
              &drawRect, &subimageRect);
    }
  }

  ctx->Restore();

}

static void
DrawBorderImage(nsPresContext* aPresContext,
                nsIRenderingContext& aRenderingContext,
                nsIFrame* aForFrame, const nsRect& aBorderArea,
                const nsStyleBorder& aBorderStyle)
{
    float percent;
    nsStyleCoord borderImageSplit[4];
    PRInt32 borderImageSplitInt[4];
    nsMargin border;
    gfxFloat borderTop, borderRight, borderBottom, borderLeft;
    gfxFloat borderImageSplitGfx[4];

    border = aBorderStyle.GetActualBorder();
    if ((0 == border.left) && (0 == border.right) &&
        (0 == border.top) && (0 == border.bottom)) {
      
      return;
    }

    borderImageSplit[NS_SIDE_TOP] = aBorderStyle.mBorderImageSplit.GetTop();
    borderImageSplit[NS_SIDE_RIGHT] = aBorderStyle.mBorderImageSplit.GetRight();
    borderImageSplit[NS_SIDE_BOTTOM] = aBorderStyle.mBorderImageSplit.GetBottom();
    borderImageSplit[NS_SIDE_LEFT] = aBorderStyle.mBorderImageSplit.GetLeft();

    imgIRequest *req = aPresContext->LoadBorderImage(aBorderStyle.GetBorderImage(), aForFrame);

    nsCOMPtr<imgIContainer> image;
    req->GetImage(getter_AddRefs(image));

    nsSize imageSize;
    image->GetWidth(&imageSize.width);
    image->GetHeight(&imageSize.height);
    imageSize.width = nsPresContext::CSSPixelsToAppUnits(imageSize.width);
    imageSize.height = nsPresContext::CSSPixelsToAppUnits(imageSize.height);

    
    NS_FOR_CSS_SIDES(side) {
      borderImageSplitInt[side] = 0;
      switch (borderImageSplit[side].GetUnit()) {
        case eStyleUnit_Percent:
          percent = borderImageSplit[side].GetPercentValue();
          if (side == NS_SIDE_TOP || side == NS_SIDE_BOTTOM)
            borderImageSplitInt[side] = (nscoord)(percent * imageSize.height);
          else
            borderImageSplitInt[side] = (nscoord)(percent * imageSize.width);
          break;
        case eStyleUnit_Integer:
          borderImageSplitInt[side] = nsPresContext::CSSPixelsToAppUnits(borderImageSplit[side].
                                          GetIntValue());
          break;
        case eStyleUnit_Factor:
          borderImageSplitInt[side] = nsPresContext::CSSPixelsToAppUnits(borderImageSplit[side].GetFactorValue());
          break;
        default:
          break;
      }
    }

    gfxContext *thebesCtx = aRenderingContext.ThebesContext();
    nsCOMPtr<nsIDeviceContext> dc;
    aRenderingContext.GetDeviceContext(*getter_AddRefs(dc));

    NS_FOR_CSS_SIDES(side) {
      borderImageSplitGfx[side] = nsPresContext::AppUnitsToFloatCSSPixels(borderImageSplitInt[side]);
    }

    borderTop = dc->AppUnitsToGfxUnits(border.top);
    borderRight = dc->AppUnitsToGfxUnits(border.right);
    borderBottom = dc->AppUnitsToGfxUnits(border.bottom);
    borderLeft = dc->AppUnitsToGfxUnits(border.left);

    gfxSize gfxImageSize;
    gfxImageSize.width = nsPresContext::AppUnitsToFloatCSSPixels(imageSize.width);
    gfxImageSize.height = nsPresContext::AppUnitsToFloatCSSPixels(imageSize.height);

    nsRect outerRect(aBorderArea);
    gfxRect rectToDraw,
            rectToDrawSource;

    gfxRect clipRect;
    clipRect.pos.x = dc->AppUnitsToGfxUnits(outerRect.x);
    clipRect.pos.y = dc->AppUnitsToGfxUnits(outerRect.y);
    clipRect.size.width = dc->AppUnitsToGfxUnits(outerRect.width);
    clipRect.size.height = dc->AppUnitsToGfxUnits(outerRect.height);
    if (thebesCtx->UserToDevicePixelSnapped(clipRect))
      clipRect = thebesCtx->DeviceToUser(clipRect);

    thebesCtx->Save();
    thebesCtx->PushGroup(gfxASurface::CONTENT_COLOR_ALPHA);

    gfxSize middleSize(clipRect.size.width - (borderLeft + borderRight),
                       clipRect.size.height - (borderTop + borderBottom));

    
    gfxIntSize middleSizeSource(gfxImageSize.width - (borderImageSplitGfx[NS_SIDE_RIGHT] + borderImageSplitGfx[NS_SIDE_LEFT]), 
                                gfxImageSize.height - (borderImageSplitGfx[NS_SIDE_TOP] + borderImageSplitGfx[NS_SIDE_BOTTOM]));

    gfxSize interSizeTop, interSizeBottom, interSizeLeft, interSizeRight,
            interSizeMiddle;
    gfxFloat topScale = borderTop/borderImageSplitGfx[NS_SIDE_TOP];
    gfxFloat bottomScale = borderBottom/borderImageSplitGfx[NS_SIDE_BOTTOM];
    gfxFloat leftScale = borderLeft/borderImageSplitGfx[NS_SIDE_LEFT];
    gfxFloat rightScale = borderRight/borderImageSplitGfx[NS_SIDE_RIGHT];
    gfxFloat middleScaleH,
             middleScaleV;
    
    if (topScale != 0.0 && borderImageSplitGfx[NS_SIDE_TOP] != 0.0) {
      middleScaleH = topScale;
    } else if (bottomScale != 0.0 && borderImageSplitGfx[NS_SIDE_BOTTOM] != 0.0) {
      middleScaleH = bottomScale;
    } else {
      middleScaleH = 1.0;
    }

    if (leftScale != 0.0 && borderImageSplitGfx[NS_SIDE_LEFT] != 0.0) {
      middleScaleV = leftScale;
    } else if (rightScale != 0.0 && borderImageSplitGfx[NS_SIDE_RIGHT] != 0.0) {
      middleScaleV = rightScale;
    } else {
      middleScaleV = 1.0;
    }

    interSizeTop.height = borderTop;
    interSizeTop.width = middleSizeSource.width*topScale;

    interSizeBottom.height = borderBottom;
    interSizeBottom.width = middleSizeSource.width*bottomScale;

    interSizeLeft.width = borderLeft;
    interSizeLeft.height = middleSizeSource.height*leftScale;

    interSizeRight.width = borderRight;
    interSizeRight.height = middleSizeSource.height*rightScale;

    interSizeMiddle.width = middleSizeSource.width*middleScaleH;
    interSizeMiddle.height = middleSizeSource.height*middleScaleV;

    
    rectToDraw = clipRect;
    rectToDraw.size.width = borderLeft;
    rectToDraw.size.height = borderTop;
    rectToDrawSource.pos.x = 0;
    rectToDrawSource.pos.y = 0;
    rectToDrawSource.size.width = borderImageSplitGfx[NS_SIDE_LEFT];
    rectToDrawSource.size.height = borderImageSplitGfx[NS_SIDE_TOP];
    DrawBorderImageSide(thebesCtx, dc, image,
                        rectToDraw, rectToDraw.size, rectToDrawSource,
                        NS_STYLE_BORDER_IMAGE_STRETCH, NS_STYLE_BORDER_IMAGE_STRETCH);

    
    rectToDraw = clipRect;
    rectToDraw.pos.x += borderLeft;
    rectToDraw.size.width = middleSize.width;
    rectToDraw.size.height = borderTop;
    rectToDrawSource.pos.x = borderImageSplitGfx[NS_SIDE_LEFT];
    rectToDrawSource.pos.y = 0;
    rectToDrawSource.size.width = middleSizeSource.width;
    rectToDrawSource.size.height = borderImageSplitGfx[NS_SIDE_TOP];
    DrawBorderImageSide(thebesCtx, dc, image,
                        rectToDraw, interSizeTop, rectToDrawSource, 
                        aBorderStyle.mBorderImageHFill, NS_STYLE_BORDER_IMAGE_STRETCH);
    
    
    rectToDraw = clipRect;
    rectToDraw.pos.x += clipRect.size.width - borderRight;
    rectToDraw.size.width = borderRight;
    rectToDraw.size.height = borderTop;
    rectToDrawSource.pos.x = gfxImageSize.width - borderImageSplitGfx[NS_SIDE_RIGHT];
    rectToDrawSource.pos.y = 0;
    rectToDrawSource.size.width = borderImageSplitGfx[NS_SIDE_RIGHT];
    rectToDrawSource.size.height = borderImageSplitGfx[NS_SIDE_TOP];
    DrawBorderImageSide(thebesCtx, dc, image,
                        rectToDraw, rectToDraw.size, rectToDrawSource,
                        NS_STYLE_BORDER_IMAGE_STRETCH, NS_STYLE_BORDER_IMAGE_STRETCH);
    
    
    rectToDraw = clipRect;
    rectToDraw.pos.x += clipRect.size.width - borderRight;
    rectToDraw.pos.y += borderTop;
    rectToDraw.size.width = borderRight;
    rectToDraw.size.height = middleSize.height;
    rectToDrawSource.pos.x = gfxImageSize.width - borderImageSplitGfx[NS_SIDE_RIGHT];
    rectToDrawSource.pos.y = borderImageSplitGfx[NS_SIDE_TOP];
    rectToDrawSource.size.width = borderImageSplitGfx[NS_SIDE_RIGHT];
    rectToDrawSource.size.height = middleSizeSource.height;
    DrawBorderImageSide(thebesCtx, dc, image,
                        rectToDraw, interSizeRight, rectToDrawSource, 
                        NS_STYLE_BORDER_IMAGE_STRETCH, aBorderStyle.mBorderImageVFill);
    
    
    rectToDraw = clipRect;
    rectToDraw.pos.x += clipRect.size.width - borderRight;
    rectToDraw.pos.y += clipRect.size.height - borderBottom;
    rectToDraw.size.width = borderRight;
    rectToDraw.size.height = borderBottom;
    rectToDrawSource.pos.x = gfxImageSize.width - borderImageSplitGfx[NS_SIDE_RIGHT];
    rectToDrawSource.pos.y = gfxImageSize.height - borderImageSplitGfx[NS_SIDE_BOTTOM];
    rectToDrawSource.size.width = borderImageSplitGfx[NS_SIDE_RIGHT];
    rectToDrawSource.size.height = borderImageSplitGfx[NS_SIDE_BOTTOM];
    DrawBorderImageSide(thebesCtx, dc, image,
                        rectToDraw, rectToDraw.size, rectToDrawSource,
                        NS_STYLE_BORDER_IMAGE_STRETCH, NS_STYLE_BORDER_IMAGE_STRETCH);
    
    
    rectToDraw = clipRect;
    rectToDraw.pos.x += borderLeft;
    rectToDraw.pos.y += clipRect.size.height - borderBottom;
    rectToDraw.size.width = middleSize.width;
    rectToDraw.size.height = borderBottom;
    rectToDrawSource.pos.x = borderImageSplitGfx[NS_SIDE_LEFT];
    rectToDrawSource.pos.y = gfxImageSize.height - borderImageSplitGfx[NS_SIDE_BOTTOM];
    rectToDrawSource.size.width = middleSizeSource.width;
    rectToDrawSource.size.height = borderImageSplitGfx[NS_SIDE_BOTTOM];
    DrawBorderImageSide(thebesCtx, dc, image,
                        rectToDraw, interSizeBottom, rectToDrawSource, 
                        aBorderStyle.mBorderImageHFill, NS_STYLE_BORDER_IMAGE_STRETCH);
    
    
    rectToDraw = clipRect;
    rectToDraw.pos.y += clipRect.size.height - borderBottom;
    rectToDraw.size.width = borderLeft;
    rectToDraw.size.height = borderBottom;
    rectToDrawSource.pos.x = 0;
    rectToDrawSource.pos.y = gfxImageSize.height - borderImageSplitGfx[NS_SIDE_BOTTOM];
    rectToDrawSource.size.width = borderImageSplitGfx[NS_SIDE_LEFT];
    rectToDrawSource.size.height = borderImageSplitGfx[NS_SIDE_BOTTOM];
    DrawBorderImageSide(thebesCtx, dc, image,
                        rectToDraw, rectToDraw.size, rectToDrawSource,
                        NS_STYLE_BORDER_IMAGE_STRETCH, NS_STYLE_BORDER_IMAGE_STRETCH);
    
    
    rectToDraw = clipRect;
    rectToDraw.pos.y += borderTop;
    rectToDraw.size.width = borderLeft;
    rectToDraw.size.height = middleSize.height;
    rectToDrawSource.pos.x = 0;
    rectToDrawSource.pos.y = borderImageSplitGfx[NS_SIDE_TOP];
    rectToDrawSource.size.width = borderImageSplitGfx[NS_SIDE_LEFT];
    rectToDrawSource.size.height = middleSizeSource.height;
    DrawBorderImageSide(thebesCtx, dc, image,
                        rectToDraw, interSizeLeft, rectToDrawSource, 
                        NS_STYLE_BORDER_IMAGE_STRETCH, aBorderStyle.mBorderImageVFill);

    
    rectToDraw = clipRect;
    rectToDraw.pos.x += borderLeft;
    rectToDraw.pos.y += borderTop;
    rectToDraw.size.width = middleSize.width;
    rectToDraw.size.height = middleSize.height;
    rectToDrawSource.pos.x = borderImageSplitGfx[NS_SIDE_LEFT];
    rectToDrawSource.pos.y = borderImageSplitGfx[NS_SIDE_TOP];
    rectToDrawSource.size = middleSizeSource;
    DrawBorderImageSide(thebesCtx, dc, image,
                        rectToDraw, interSizeMiddle, rectToDrawSource,
                        aBorderStyle.mBorderImageHFill, aBorderStyle.mBorderImageVFill);

    thebesCtx->PopGroupToSource();
    thebesCtx->SetOperator(gfxContext::OPERATOR_OVER);
    thebesCtx->Paint();
    thebesCtx->Restore();
}

static void
DrawBorderImageSide(gfxContext *aThebesContext,
                    nsIDeviceContext* aDeviceContext,
                    imgIContainer* aImage,
                    gfxRect& aDestRect,
                    gfxSize aInterSize, 
                    gfxRect& aSourceRect,
                    PRUint8 aHFillType,
                    PRUint8 aVFillType)
{
  if (aDestRect.size.width < 1.0 || aDestRect.size.height < 1.0 ||
      aSourceRect.size.width < 1.0 || aSourceRect.size.height < 1.0) {
    return;
  }

  gfxIntSize gfxSourceSize((PRInt32)aSourceRect.size.width,
                           (PRInt32)aSourceRect.size.height);

  
  if (aThebesContext->UserToDevicePixelSnapped(aDestRect))
    aDestRect = aThebesContext->DeviceToUser(aDestRect);
  if (aThebesContext->UserToDevicePixelSnapped(aSourceRect))
    aSourceRect = aThebesContext->DeviceToUser(aSourceRect);

  if (aDestRect.size.height < 1.0 ||
     aDestRect.size.width < 1.0)
    return;

  if (aInterSize.width < 1.0 ||
     aInterSize.height < 1.0)
    return;

  
  
  nsRefPtr<gfxASurface> interSurface =
    gfxPlatform::GetPlatform()->CreateOffscreenSurface(
        gfxSourceSize, gfxASurface::ImageFormatARGB32);

  gfxMatrix srcMatrix;
  
  srcMatrix.Scale(aSourceRect.size.width/aInterSize.width,
                  aSourceRect.size.height/aInterSize.height);
  {
    nsCOMPtr<gfxIImageFrame> frame;
    nsresult rv = aImage->GetCurrentFrame(getter_AddRefs(frame));
    if(NS_FAILED(rv))
      return;
    nsCOMPtr<nsIImage> image;
    image = do_GetInterface(frame);
    if(!image)
      return;

    
    nsRefPtr<gfxPattern> imagePattern;
    rv = image->GetPattern(getter_AddRefs(imagePattern));
    if(NS_FAILED(rv) || !imagePattern)
      return;

    gfxMatrix mat;
    mat.Translate(aSourceRect.pos);
    imagePattern->SetMatrix(mat);

    
    nsRefPtr<gfxContext> srcCtx = new gfxContext(interSurface);
    srcCtx->SetPattern(imagePattern);
    srcCtx->SetOperator(gfxContext::OPERATOR_SOURCE);
    srcCtx->Paint();
    srcCtx = nsnull;

  }

  
  gfxPoint renderOffset(0, 0);
  gfxSize rectSize(aDestRect.size);

  aThebesContext->Save();
  aThebesContext->Clip(aDestRect);

  gfxFloat hScale(1.0), vScale(1.0);

  nsRefPtr<gfxPattern> pattern = new gfxPattern(interSurface);
  pattern->SetExtend(gfxPattern::EXTEND_PAD_EDGE);
  switch (aHFillType) {
    case NS_STYLE_BORDER_IMAGE_REPEAT:
      renderOffset.x = (rectSize.width - aInterSize.width*NS_ceil(rectSize.width/aInterSize.width))*-0.5;
      aDestRect.pos.x -= renderOffset.x;
      pattern->SetExtend(gfxPattern::EXTEND_REPEAT);
      break;
    case NS_STYLE_BORDER_IMAGE_ROUND:
      hScale = aInterSize.width*(NS_ceil(aDestRect.size.width/aInterSize.width)/aDestRect.size.width);
      pattern->SetExtend(gfxPattern::EXTEND_REPEAT);
      break;
    case NS_STYLE_BORDER_IMAGE_STRETCH:
    default:
      hScale = aInterSize.width/aDestRect.size.width;
      break;
  }

  switch (aVFillType) {
    case NS_STYLE_BORDER_IMAGE_REPEAT:
      renderOffset.y = (rectSize.height - aInterSize.height*NS_ceil(rectSize.height/aInterSize.height))*-0.5;
      aDestRect.pos.y -= renderOffset.y;
      pattern->SetExtend(gfxPattern::EXTEND_REPEAT);
      break;
    case NS_STYLE_BORDER_IMAGE_ROUND:
      vScale = aInterSize.height*(NS_ceil(aDestRect.size.height/aInterSize.height)/aDestRect.size.height);
      pattern->SetExtend(gfxPattern::EXTEND_REPEAT);
      break;
    case NS_STYLE_BORDER_IMAGE_STRETCH:
    default:
      vScale = aInterSize.height/aDestRect.size.height;
      break;
  }

  
  srcMatrix.Scale(hScale,vScale);
  pattern->SetMatrix(srcMatrix);

  
  aThebesContext->Translate(aDestRect.pos);
  aThebesContext->SetPattern(pattern);
  aThebesContext->NewPath();
  aThebesContext->Rectangle(gfxRect(renderOffset, rectSize));
  aThebesContext->SetOperator(gfxContext::OPERATOR_ADD);
  aThebesContext->Fill();
  aThebesContext->Restore();
}

static void
PaintBackgroundColor(nsPresContext* aPresContext,
                     nsIRenderingContext& aRenderingContext,
                     nsIFrame* aForFrame,
                     const nsRect& aBgClipArea,
                     const nsStyleBackground& aColor,
                     const nsStyleBorder& aBorder,
                     PRBool aCanPaintNonWhite)
{
  
  
  
  
  if (NS_GET_A(aColor.mBackgroundColor) == 0 &&
      (aCanPaintNonWhite || aColor.IsTransparent())) {
    
    return;
  }

  nscolor color = aColor.mBackgroundColor;
  if (!aCanPaintNonWhite) {
    color = NS_RGB(255, 255, 255);
  }
  aRenderingContext.SetColor(color);

  if (!nsLayoutUtils::HasNonZeroCorner(aBorder.mBorderRadius)) {
    aRenderingContext.FillRect(aBgClipArea);
    return;
  }

  gfxContext *ctx = aRenderingContext.ThebesContext();

  
  nscoord appUnitsPerPixel = aPresContext->AppUnitsPerDevPixel();

  nscoord borderRadii[8];
  GetBorderRadiusTwips(aBorder.mBorderRadius, aForFrame->GetSize().width,
                       borderRadii);

  
  gfxRect oRect(RectToGfxRect(aBgClipArea, appUnitsPerPixel));
  oRect.Round();
  oRect.Condition();
  if (oRect.IsEmpty())
    return;

  
  gfxCornerSizes radii;
  ComputePixelRadii(borderRadii, aBgClipArea,
                    aForFrame ? aForFrame->GetSkipSides() : 0,
                    appUnitsPerPixel, &radii);

  
  
  
  
  
  
  
  for (int i = 0; i < 4; i++) {
    if (radii[i].width > 0.0)
      radii[i].width += 1.0;
    if (radii[i].height > 0.0)
      radii[i].height += 1.0;
  }

  ctx->NewPath();
  ctx->RoundedRectangle(oRect, radii);
  ctx->Fill();
}






static nscoord
RoundIntToPixel(nscoord aValue, 
                nscoord aTwipsPerPixel,
                PRBool  aRoundDown = PR_FALSE)
{
  if (aTwipsPerPixel <= 0) 
    
    
    return aValue; 

  nscoord halfPixel = NSToCoordRound(aTwipsPerPixel / 2.0f);
  nscoord extra = aValue % aTwipsPerPixel;
  nscoord finalValue = (!aRoundDown && (extra >= halfPixel)) ? aValue + (aTwipsPerPixel - extra) : aValue - extra;
  return finalValue;
}

static nscoord
RoundFloatToPixel(float   aValue, 
                  nscoord aTwipsPerPixel,
                  PRBool  aRoundDown = PR_FALSE)
{
  return RoundIntToPixel(NSToCoordRound(aValue), aTwipsPerPixel, aRoundDown);
}

static void
SetPoly(const nsRect& aRect,
        nsPoint*      poly)
{
  poly[0].x = aRect.x;
  poly[0].y = aRect.y;
  poly[1].x = aRect.x + aRect.width;
  poly[1].y = aRect.y;
  poly[2].x = aRect.x + aRect.width;
  poly[2].y = aRect.y + aRect.height;
  poly[3].x = aRect.x;
  poly[3].y = aRect.y + aRect.height;
  poly[4].x = aRect.x;
  poly[4].y = aRect.y;
}
          
static void 
DrawSolidBorderSegment(nsIRenderingContext& aContext,
                       nsRect               aRect,
                       nscoord              aTwipsPerPixel,
                       PRUint8              aStartBevelSide = 0,
                       nscoord              aStartBevelOffset = 0,
                       PRUint8              aEndBevelSide = 0,
                       nscoord              aEndBevelOffset = 0)
{

  if ((aRect.width == aTwipsPerPixel) || (aRect.height == aTwipsPerPixel) ||
      ((0 == aStartBevelOffset) && (0 == aEndBevelOffset))) {
    
    if ((NS_SIDE_TOP == aStartBevelSide) || (NS_SIDE_BOTTOM == aStartBevelSide)) {
      if (1 == aRect.height) 
        aContext.DrawLine(aRect.x, aRect.y, aRect.x, aRect.y + aRect.height); 
      else 
        aContext.FillRect(aRect);
    }
    else {
      if (1 == aRect.width) 
        aContext.DrawLine(aRect.x, aRect.y, aRect.x + aRect.width, aRect.y); 
      else 
        aContext.FillRect(aRect);
    }
  }
  else {
    
    nsPoint poly[5];
    SetPoly(aRect, poly);
    switch(aStartBevelSide) {
    case NS_SIDE_TOP:
      poly[0].x += aStartBevelOffset;
      poly[4].x = poly[0].x;
      break;
    case NS_SIDE_BOTTOM:
      poly[3].x += aStartBevelOffset;
      break;
    case NS_SIDE_RIGHT:
      poly[1].y += aStartBevelOffset;
      break;
    case NS_SIDE_LEFT:
      poly[0].y += aStartBevelOffset;
      poly[4].y = poly[0].y;
    }

    switch(aEndBevelSide) {
    case NS_SIDE_TOP:
      poly[1].x -= aEndBevelOffset;
      break;
    case NS_SIDE_BOTTOM:
      poly[2].x -= aEndBevelOffset;
      break;
    case NS_SIDE_RIGHT:
      poly[2].y -= aEndBevelOffset;
      break;
    case NS_SIDE_LEFT:
      poly[3].y -= aEndBevelOffset;
    }

    aContext.FillPolygon(poly, 5);
  }


}

static void
GetDashInfo(nscoord  aBorderLength,
            nscoord  aDashLength,
            nscoord  aTwipsPerPixel,
            PRInt32& aNumDashSpaces,
            nscoord& aStartDashLength,
            nscoord& aEndDashLength)
{
  aNumDashSpaces = 0;
  if (aStartDashLength + aDashLength + aEndDashLength >= aBorderLength) {
    aStartDashLength = aBorderLength;
    aEndDashLength = 0;
  }
  else {
    aNumDashSpaces = aBorderLength / (2 * aDashLength); 
    nscoord extra = aBorderLength - aStartDashLength - aEndDashLength - (((2 * aNumDashSpaces) - 1) * aDashLength);
    if (extra > 0) {
      nscoord half = RoundIntToPixel(extra / 2, aTwipsPerPixel);
      aStartDashLength += half;
      aEndDashLength += (extra - half);
    }
  }
}

void 
nsCSSRendering::DrawTableBorderSegment(nsIRenderingContext&     aContext,
                                       PRUint8                  aBorderStyle,  
                                       nscolor                  aBorderColor,
                                       const nsStyleBackground* aBGColor,
                                       const nsRect&            aBorder,
                                       PRInt32                  aAppUnitsPerCSSPixel,
                                       PRUint8                  aStartBevelSide,
                                       nscoord                  aStartBevelOffset,
                                       PRUint8                  aEndBevelSide,
                                       nscoord                  aEndBevelOffset)
{
  aContext.SetColor (aBorderColor); 

  PRBool horizontal = ((NS_SIDE_TOP == aStartBevelSide) || (NS_SIDE_BOTTOM == aStartBevelSide));
  nscoord twipsPerPixel = NSIntPixelsToAppUnits(1, aAppUnitsPerCSSPixel);
  PRUint8 ridgeGroove = NS_STYLE_BORDER_STYLE_RIDGE;

  if ((twipsPerPixel >= aBorder.width) || (twipsPerPixel >= aBorder.height) ||
      (NS_STYLE_BORDER_STYLE_DASHED == aBorderStyle) || (NS_STYLE_BORDER_STYLE_DOTTED == aBorderStyle)) {
    
    aStartBevelOffset = 0;
    aEndBevelOffset = 0;
  }

  gfxContext *ctx = aContext.ThebesContext();
  gfxContext::AntialiasMode oldMode = ctx->CurrentAntialiasMode();
  ctx->SetAntialiasMode(gfxContext::MODE_ALIASED);

  switch (aBorderStyle) {
  case NS_STYLE_BORDER_STYLE_NONE:
  case NS_STYLE_BORDER_STYLE_HIDDEN:
    
    break;
  case NS_STYLE_BORDER_STYLE_DOTTED:
  case NS_STYLE_BORDER_STYLE_DASHED: 
    {
      nscoord dashLength = (NS_STYLE_BORDER_STYLE_DASHED == aBorderStyle) ? DASH_LENGTH : DOT_LENGTH;
      
      dashLength *= (horizontal) ? aBorder.height : aBorder.width;
      
      nscoord minDashLength = (NS_STYLE_BORDER_STYLE_DASHED == aBorderStyle) 
                              ? RoundFloatToPixel(((float)dashLength) / 2.0f, twipsPerPixel) : dashLength;
      minDashLength = PR_MAX(minDashLength, twipsPerPixel);
      nscoord numDashSpaces = 0;
      nscoord startDashLength = minDashLength;
      nscoord endDashLength   = minDashLength;
      if (horizontal) {
        GetDashInfo(aBorder.width, dashLength, twipsPerPixel, numDashSpaces, startDashLength, endDashLength);
        nsRect rect(aBorder.x, aBorder.y, startDashLength, aBorder.height);
        DrawSolidBorderSegment(aContext, rect, twipsPerPixel);
        for (PRInt32 spaceX = 0; spaceX < numDashSpaces; spaceX++) {
          rect.x += rect.width + dashLength;
          rect.width = (spaceX == (numDashSpaces - 1)) ? endDashLength : dashLength;
          DrawSolidBorderSegment(aContext, rect, twipsPerPixel);
        }
      }
      else {
        GetDashInfo(aBorder.height, dashLength, twipsPerPixel, numDashSpaces, startDashLength, endDashLength);
        nsRect rect(aBorder.x, aBorder.y, aBorder.width, startDashLength);
        DrawSolidBorderSegment(aContext, rect, twipsPerPixel);
        for (PRInt32 spaceY = 0; spaceY < numDashSpaces; spaceY++) {
          rect.y += rect.height + dashLength;
          rect.height = (spaceY == (numDashSpaces - 1)) ? endDashLength : dashLength;
          DrawSolidBorderSegment(aContext, rect, twipsPerPixel);
        }
      }
    }
    break;                                  
  case NS_STYLE_BORDER_STYLE_GROOVE:
    ridgeGroove = NS_STYLE_BORDER_STYLE_GROOVE; 
  case NS_STYLE_BORDER_STYLE_RIDGE:
    if ((horizontal && (twipsPerPixel >= aBorder.height)) ||
        (!horizontal && (twipsPerPixel >= aBorder.width))) {
      
      DrawSolidBorderSegment(aContext, aBorder, twipsPerPixel, aStartBevelSide, aStartBevelOffset,
                             aEndBevelSide, aEndBevelOffset);
    }
    else {
      nscoord startBevel = (aStartBevelOffset > 0) 
                            ? RoundFloatToPixel(0.5f * (float)aStartBevelOffset, twipsPerPixel, PR_TRUE) : 0;
      nscoord endBevel =   (aEndBevelOffset > 0) 
                            ? RoundFloatToPixel(0.5f * (float)aEndBevelOffset, twipsPerPixel, PR_TRUE) : 0;
      PRUint8 ridgeGrooveSide = (horizontal) ? NS_SIDE_TOP : NS_SIDE_LEFT;
      aContext.SetColor ( 
        MakeBevelColor(ridgeGrooveSide, ridgeGroove, aBGColor->mBackgroundColor, aBorderColor));
      nsRect rect(aBorder);
      nscoord half;
      if (horizontal) { 
        half = RoundFloatToPixel(0.5f * (float)aBorder.height, twipsPerPixel);
        rect.height = half;
        if (NS_SIDE_TOP == aStartBevelSide) {
          rect.x += startBevel;
          rect.width -= startBevel;
        }
        if (NS_SIDE_TOP == aEndBevelSide) {
          rect.width -= endBevel;
        }
        DrawSolidBorderSegment(aContext, rect, twipsPerPixel, aStartBevelSide, 
                               startBevel, aEndBevelSide, endBevel);
      }
      else { 
        half = RoundFloatToPixel(0.5f * (float)aBorder.width, twipsPerPixel);
        rect.width = half;
        if (NS_SIDE_LEFT == aStartBevelSide) {
          rect.y += startBevel;
          rect.height -= startBevel;
        }
        if (NS_SIDE_LEFT == aEndBevelSide) {
          rect.height -= endBevel;
        }
        DrawSolidBorderSegment(aContext, rect, twipsPerPixel, aStartBevelSide, 
                               startBevel, aEndBevelSide, endBevel);
      }

      rect = aBorder;
      ridgeGrooveSide = (NS_SIDE_TOP == ridgeGrooveSide) ? NS_SIDE_BOTTOM : NS_SIDE_RIGHT;
      aContext.SetColor ( 
        MakeBevelColor(ridgeGrooveSide, ridgeGroove, aBGColor->mBackgroundColor, aBorderColor));
      if (horizontal) {
        rect.y = rect.y + half;
        rect.height = aBorder.height - half;
        if (NS_SIDE_BOTTOM == aStartBevelSide) {
          rect.x += startBevel;
          rect.width -= startBevel;
        }
        if (NS_SIDE_BOTTOM == aEndBevelSide) {
          rect.width -= endBevel;
        }
        DrawSolidBorderSegment(aContext, rect, twipsPerPixel, aStartBevelSide, 
                               startBevel, aEndBevelSide, endBevel);
      }
      else {
        rect.x = rect.x + half;
        rect.width = aBorder.width - half;
        if (NS_SIDE_RIGHT == aStartBevelSide) {
          rect.y += aStartBevelOffset - startBevel;
          rect.height -= startBevel;
        }
        if (NS_SIDE_RIGHT == aEndBevelSide) {
          rect.height -= endBevel;
        }
        DrawSolidBorderSegment(aContext, rect, twipsPerPixel, aStartBevelSide, 
                               startBevel, aEndBevelSide, endBevel);
      }
    }
    break;
  case NS_STYLE_BORDER_STYLE_DOUBLE:
    if ((aBorder.width > 2) && (aBorder.height > 2)) {
      nscoord startBevel = (aStartBevelOffset > 0) 
                            ? RoundFloatToPixel(0.333333f * (float)aStartBevelOffset, twipsPerPixel) : 0;
      nscoord endBevel =   (aEndBevelOffset > 0) 
                            ? RoundFloatToPixel(0.333333f * (float)aEndBevelOffset, twipsPerPixel) : 0;
      if (horizontal) { 
        nscoord thirdHeight = RoundFloatToPixel(0.333333f * (float)aBorder.height, twipsPerPixel);

        
        nsRect topRect(aBorder.x, aBorder.y, aBorder.width, thirdHeight);
        if (NS_SIDE_TOP == aStartBevelSide) {
          topRect.x += aStartBevelOffset - startBevel;
          topRect.width -= aStartBevelOffset - startBevel;
        }
        if (NS_SIDE_TOP == aEndBevelSide) {
          topRect.width -= aEndBevelOffset - endBevel;
        }
        DrawSolidBorderSegment(aContext, topRect, twipsPerPixel, aStartBevelSide, 
                               startBevel, aEndBevelSide, endBevel);

        
        nscoord heightOffset = aBorder.height - thirdHeight; 
        nsRect bottomRect(aBorder.x, aBorder.y + heightOffset, aBorder.width, aBorder.height - heightOffset);
        if (NS_SIDE_BOTTOM == aStartBevelSide) {
          bottomRect.x += aStartBevelOffset - startBevel;
          bottomRect.width -= aStartBevelOffset - startBevel;
        }
        if (NS_SIDE_BOTTOM == aEndBevelSide) {
          bottomRect.width -= aEndBevelOffset - endBevel;
        }
        DrawSolidBorderSegment(aContext, bottomRect, twipsPerPixel, aStartBevelSide, 
                               startBevel, aEndBevelSide, endBevel);
      }
      else { 
        nscoord thirdWidth = RoundFloatToPixel(0.333333f * (float)aBorder.width, twipsPerPixel);

        nsRect leftRect(aBorder.x, aBorder.y, thirdWidth, aBorder.height); 
        if (NS_SIDE_LEFT == aStartBevelSide) {
          leftRect.y += aStartBevelOffset - startBevel;
          leftRect.height -= aStartBevelOffset - startBevel;
        }
        if (NS_SIDE_LEFT == aEndBevelSide) {
          leftRect.height -= aEndBevelOffset - endBevel;
        }
        DrawSolidBorderSegment(aContext, leftRect, twipsPerPixel, aStartBevelSide,
                               startBevel, aEndBevelSide, endBevel);

        nscoord widthOffset = aBorder.width - thirdWidth; 
        nsRect rightRect(aBorder.x + widthOffset, aBorder.y, aBorder.width - widthOffset, aBorder.height);
        if (NS_SIDE_RIGHT == aStartBevelSide) {
          rightRect.y += aStartBevelOffset - startBevel;
          rightRect.height -= aStartBevelOffset - startBevel;
        }
        if (NS_SIDE_RIGHT == aEndBevelSide) {
          rightRect.height -= aEndBevelOffset - endBevel;
        }
        DrawSolidBorderSegment(aContext, rightRect, twipsPerPixel, aStartBevelSide,
                               startBevel, aEndBevelSide, endBevel);
      }
      break;
    }
    
  case NS_STYLE_BORDER_STYLE_SOLID:
    DrawSolidBorderSegment(aContext, aBorder, twipsPerPixel, aStartBevelSide, 
                           aStartBevelOffset, aEndBevelSide, aEndBevelOffset);
    break;
  case NS_STYLE_BORDER_STYLE_OUTSET:
  case NS_STYLE_BORDER_STYLE_INSET:
    NS_ASSERTION(PR_FALSE, "inset, outset should have been converted to groove, ridge");
    break;
  case NS_STYLE_BORDER_STYLE_AUTO:
    NS_ASSERTION(PR_FALSE, "Unexpected 'auto' table border");
    break;
  }

  ctx->SetAntialiasMode(oldMode);
}



void
nsCSSRendering::PaintDecorationLine(gfxContext* aGfxContext,
                                    const nscolor aColor,
                                    const gfxPoint& aPt,
                                    const gfxSize& aLineSize,
                                    const gfxFloat aAscent,
                                    const gfxFloat aOffset,
                                    const PRUint8 aDecoration,
                                    const PRUint8 aStyle)
{
  gfxRect rect =
    GetTextDecorationRectInternal(aPt, aLineSize, aAscent, aOffset,
                                  aDecoration, aStyle);
  if (rect.IsEmpty())
    return;

  if (aDecoration != NS_STYLE_TEXT_DECORATION_UNDERLINE &&
      aDecoration != NS_STYLE_TEXT_DECORATION_OVERLINE &&
      aDecoration != NS_STYLE_TEXT_DECORATION_LINE_THROUGH)
  {
    NS_ERROR("Invalid decoration value!");
    return;
  }

  gfxFloat lineHeight = PR_MAX(NS_round(aLineSize.height), 1.0);
  PRBool contextIsSaved = PR_FALSE;

  gfxFloat oldLineWidth;
  nsRefPtr<gfxPattern> oldPattern;

  switch (aStyle) {
    case NS_STYLE_BORDER_STYLE_SOLID:
    case NS_STYLE_BORDER_STYLE_DOUBLE:
      oldLineWidth = aGfxContext->CurrentLineWidth();
      oldPattern = aGfxContext->GetPattern();
      break;
    case NS_STYLE_BORDER_STYLE_DASHED: {
      aGfxContext->Save();
      contextIsSaved = PR_TRUE;
      gfxFloat dashWidth = lineHeight * DOT_LENGTH * DASH_LENGTH;
      gfxFloat dash[2] = { dashWidth, dashWidth };
      aGfxContext->SetLineCap(gfxContext::LINE_CAP_BUTT);
      aGfxContext->SetDash(dash, 2, 0.0);
      break;
    }
    case NS_STYLE_BORDER_STYLE_DOTTED: {
      aGfxContext->Save();
      contextIsSaved = PR_TRUE;
      gfxFloat dashWidth = lineHeight * DOT_LENGTH;
      gfxFloat dash[2];
      if (lineHeight > 2.0) {
        dash[0] = 0.0;
        dash[1] = dashWidth * 2.0;
        aGfxContext->SetLineCap(gfxContext::LINE_CAP_ROUND);
      } else {
        dash[0] = dashWidth;
        dash[1] = dashWidth;
      }
      aGfxContext->SetDash(dash, 2, 0.0);
      break;
    }
    default:
      NS_ERROR("Invalid style value!");
      return;
  }

  
  rect.pos.y += lineHeight / 2;

  aGfxContext->SetColor(gfxRGBA(aColor));
  aGfxContext->SetLineWidth(lineHeight);
  switch (aStyle) {
    case NS_STYLE_BORDER_STYLE_SOLID:
      aGfxContext->NewPath();
      aGfxContext->MoveTo(rect.TopLeft());
      aGfxContext->LineTo(rect.TopRight());
      aGfxContext->Stroke();
      break;
    case NS_STYLE_BORDER_STYLE_DOUBLE:
      aGfxContext->NewPath();
      aGfxContext->MoveTo(rect.TopLeft());
      aGfxContext->LineTo(rect.TopRight());
      rect.size.height -= lineHeight;
      aGfxContext->MoveTo(rect.BottomLeft());
      aGfxContext->LineTo(rect.BottomRight());
      aGfxContext->Stroke();
      break;
    case NS_STYLE_BORDER_STYLE_DOTTED:
    case NS_STYLE_BORDER_STYLE_DASHED:
      aGfxContext->NewPath();
      aGfxContext->MoveTo(rect.TopLeft());
      aGfxContext->LineTo(rect.TopRight());
      aGfxContext->Stroke();
      break;
    default:
      NS_ERROR("Invalid style value!");
      break;
  }

  if (contextIsSaved) {
    aGfxContext->Restore();
  } else {
    aGfxContext->SetPattern(oldPattern);
    aGfxContext->SetLineWidth(oldLineWidth);
  }
}

nsRect
nsCSSRendering::GetTextDecorationRect(nsPresContext* aPresContext,
                                      const gfxSize& aLineSize,
                                      const gfxFloat aAscent,
                                      const gfxFloat aOffset,
                                      const PRUint8 aDecoration,
                                      const PRUint8 aStyle)
{
  NS_ASSERTION(aPresContext, "aPresContext is null");

  gfxRect rect =
    GetTextDecorationRectInternal(gfxPoint(0, 0), aLineSize, aAscent, aOffset,
                                  aDecoration, aStyle);
  
  nsRect r;
  r.x = aPresContext->GfxUnitsToAppUnits(rect.X());
  r.y = aPresContext->GfxUnitsToAppUnits(rect.Y());
  r.width = aPresContext->GfxUnitsToAppUnits(rect.Width());
  r.height = aPresContext->GfxUnitsToAppUnits(rect.Height());
  return r;
}

static gfxRect
GetTextDecorationRectInternal(const gfxPoint& aPt,
                              const gfxSize& aLineSize,
                              const gfxFloat aAscent,
                              const gfxFloat aOffset,
                              const PRUint8 aDecoration,
                              const PRUint8 aStyle)
{
  gfxRect r;
  r.pos.x = NS_floor(aPt.x + 0.5);
  r.size.width = NS_round(aLineSize.width);

  gfxFloat basesize = NS_round(aLineSize.height);
  basesize = PR_MAX(basesize, 1.0);
  r.size.height = basesize;
  if (aStyle == NS_STYLE_BORDER_STYLE_DOUBLE) {
    gfxFloat gap = NS_round(basesize / 2.0);
    gap = PR_MAX(gap, 1.0);
    r.size.height = basesize * 2.0 + gap;
  } else {
    r.size.height = basesize;
  }

  gfxFloat baseline = NS_floor(aPt.y + aAscent + 0.5);
  gfxFloat offset = 0;
  switch (aDecoration) {
    case NS_STYLE_TEXT_DECORATION_UNDERLINE:
      offset = aOffset;
      break;
    case NS_STYLE_TEXT_DECORATION_OVERLINE:
      offset = aOffset - basesize + r.Height();
      break;
    case NS_STYLE_TEXT_DECORATION_LINE_THROUGH: {
      gfxFloat extra = NS_floor(r.Height() / 2.0 + 0.5);
      extra = PR_MAX(extra, basesize);
      offset = aOffset - basesize + extra;
      break;
    }
    default:
      NS_ERROR("Invalid decoration value!");
  }
  r.pos.y = baseline - NS_floor(offset + 0.5);
  return r;
}




gfxContext*
nsContextBoxBlur::Init(const gfxRect& aRect, nscoord aBlurRadius,
                       PRInt32 aAppUnitsPerDevPixel,
                       gfxContext* aDestinationCtx)
{
  mDestinationCtx = aDestinationCtx;

  PRInt32 blurRadius = static_cast<PRInt32>(aBlurRadius / aAppUnitsPerDevPixel);

  
  if (blurRadius <= 0) {
    mContext = aDestinationCtx;
    return mContext;
  }

  
  gfxRect rect = aRect;
  rect.ScaleInverse(aAppUnitsPerDevPixel);

  if (rect.IsEmpty()) {
    mContext = aDestinationCtx;
    return mContext;
  }

  mDestinationCtx = aDestinationCtx;

  mContext = blur.Init(rect, gfxIntSize(blurRadius, blurRadius));
  return mContext;
}

void
nsContextBoxBlur::DoPaint()
{
  if (mContext == mDestinationCtx)
    return;

  blur.Paint(mDestinationCtx);
}

gfxContext*
nsContextBoxBlur::GetContext()
{
  return mContext;
}
