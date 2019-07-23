














































#include "nsStyleConsts.h"
#include "nsPresContext.h"
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









class ImageRenderer {
public:
  enum {
    FLAG_SYNC_DECODE_IMAGES = 0x01
  };
  ImageRenderer(nsIFrame* aForFrame, const nsStyleImage& aImage, PRUint32 aFlags);
  ~ImageRenderer();
  




  PRBool PrepareImage();
  



  nsSize ComputeSize(const nsSize& aDefault);
  



  void Draw(nsPresContext*       aPresContext,
            nsIRenderingContext& aRenderingContext,
            const nsRect&        aDest,
            const nsRect&        aFill,
            const nsPoint&       aAnchor,
            const nsRect&        aDirty);

private:
  nsIFrame*                 mForFrame;
  nsStyleImage              mImage;
  nsStyleImageType          mType;
  nsCOMPtr<imgIContainer>   mImageContainer;
  nsRefPtr<nsStyleGradient> mGradientData;
  PRBool                    mIsReady;
  nsSize                    mSize;
  PRUint32                  mFlags;
};





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
  nsBlockFrame* mBlockFrame;
  nsRect        mBoundingBox;
  nscoord       mContinuationPoint;
  nscoord       mUnbrokenWidth;
  nscoord       mLineContinuationPoint;
  PRBool        mBidiEnabled;
  
  void SetFrame(nsIFrame* aFrame)
  {
    NS_PRECONDITION(aFrame, "Need a frame");

    nsIFrame *prevContinuation = GetPrevContinuation(aFrame);

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

  nsIFrame* GetPrevContinuation(nsIFrame* aFrame)
  {
    nsIFrame* prevCont = aFrame->GetPrevContinuation();
    if (!prevCont && (aFrame->GetStateBits() & NS_FRAME_IS_SPECIAL)) {
      nsIFrame* block =
        static_cast<nsIFrame*>
                   (aFrame->GetProperty(nsGkAtoms::IBSplitSpecialPrevSibling));
      if (block) {
        
        NS_ASSERTION(!block->GetPrevContinuation(),
                     "Incorrect value for IBSplitSpecialPrevSibling");
        prevCont =
          static_cast<nsIFrame*>
                     (block->GetProperty(nsGkAtoms::IBSplitSpecialPrevSibling));
        NS_ASSERTION(prevCont, "How did that happen?");
      }
    }
    return prevCont;
  }

  nsIFrame* GetNextContinuation(nsIFrame* aFrame)
  {
    nsIFrame* nextCont = aFrame->GetNextContinuation();
    if (!nextCont && (aFrame->GetStateBits() & NS_FRAME_IS_SPECIAL)) {
      
      aFrame = aFrame->GetFirstContinuation();
      nsIFrame* block =
        static_cast<nsIFrame*>
                   (aFrame->GetProperty(nsGkAtoms::IBSplitSpecialSibling));
      if (block) {
        nextCont =
          static_cast<nsIFrame*>
                     (block->GetProperty(nsGkAtoms::IBSplitSpecialSibling));
        NS_ASSERTION(nextCont, "How did that happen?");
      }
    }
    return nextCont;
  }

  void Init(nsIFrame* aFrame)
  {    
    
    
    nsIFrame* inlineFrame = GetPrevContinuation(aFrame);

    while (inlineFrame) {
      nsRect rect = inlineFrame->GetRect();
      mContinuationPoint += rect.width;
      mUnbrokenWidth += rect.width;
      mBoundingBox.UnionRect(mBoundingBox, rect);
      inlineFrame = GetPrevContinuation(inlineFrame);
    }

    
    
    inlineFrame = aFrame;
    while (inlineFrame) {
      nsRect rect = inlineFrame->GetRect();
      mUnbrokenWidth += rect.width;
      mBoundingBox.UnionRect(mBoundingBox, rect);
      inlineFrame = GetNextContinuation(inlineFrame);
    }

    mFrame = aFrame;

    mBidiEnabled = aFrame->PresContext()->BidiEnabled();
    if (mBidiEnabled) {
      
      nsIFrame* frame = aFrame;
      do {
        frame = frame->GetParent();
        mBlockFrame = do_QueryFrame(frame);
      }
      while (frame && frame->IsFrameOfType(nsIFrame::eLineParticipant));

      NS_ASSERTION(mBlockFrame, "Cannot find containing block.");

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


static void PaintBackgroundLayer(nsPresContext* aPresContext,
                                 nsIRenderingContext& aRenderingContext,
                                 nsIFrame* aForFrame,
                                 PRUint32 aFlags,
                                 const nsRect& aDirtyRect,
                                 const nsRect& aBorderArea,
                                 const nsRect& aBGClipRect,
                                 const nsStyleBackground& aBackground,
                                 const nsStyleBackground::Layer& aLayer);

static void DrawBorderImage(nsPresContext* aPresContext,
                            nsIRenderingContext& aRenderingContext,
                            nsIFrame* aForFrame,
                            const nsRect& aBorderArea,
                            const nsStyleBorder& aStyleBorder,
                            const nsRect& aDirtyRect);

static void DrawBorderImageComponent(nsIRenderingContext& aRenderingContext,
                                     nsIFrame* aForFrame,
                                     imgIContainer* aImage,
                                     const nsRect& aDirtyRect,
                                     const nsRect& aFill,
                                     const nsIntRect& aSrc,
                                     PRUint8 aHFill,
                                     PRUint8 aVFill,
                                     const nsSize& aUnitSize,
                                     const nsStyleBorder& aStyleBorder,
                                     PRUint8 aIndex);

static nscolor MakeBevelColor(PRIntn whichSide, PRUint8 style,
                              nscolor aBackgroundColor,
                              nscolor aBorderColor);

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
      f = NS_MIN(f, length/sum);
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
                            const nsStyleBorder& aStyleBorder,
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

  if (aStyleBorder.IsBorderImageLoaded()) {
    DrawBorderImage(aPresContext, aRenderingContext, aForFrame,
                    aBorderArea, aStyleBorder, aDirtyRect);
    return;
  }
  
  
  const nsStyleColor* ourColor = aStyleContext->GetStyleColor();

  
  
  nsStyleContext* bgContext = nsCSSRendering::FindNonTransparentBackground
    (aStyleContext, compatMode == eCompatibility_NavQuirks ? PR_TRUE : PR_FALSE);
  const nsStyleBackground* bgColor = bgContext->GetStyleBackground();

  border = aStyleBorder.GetComputedBorder();
  if ((0 == border.left) && (0 == border.right) &&
      (0 == border.top) && (0 == border.bottom)) {
    
    return;
  }

  GetBorderRadiusTwips(aStyleBorder.mBorderRadius, aForFrame->GetSize().width,
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
    borderStyles[i] = aStyleBorder.GetBorderStyle(i);
    aStyleBorder.GetBorderColor(i, borderColors[i], foreground);
    aStyleBorder.GetCompositeColors(i, &compositeColors[i]);

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
                             const nsStyleBorder& aStyleBorder,
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

  nsStyleContext* bgContext = nsCSSRendering::FindNonTransparentBackground
    (aStyleContext, PR_FALSE);
  const nsStyleBackground* bgColor = bgContext->GetStyleBackground();

  
  GetBorderRadiusTwips(aOutlineStyle.mOutlineRadius, aBorderArea.width,
                       twipsRadii);

  
  
  
  
  
  
  nsIFrame *frameForArea = aForFrame;
  do {
    nsIAtom *pseudoType = frameForArea->GetStyleContext()->GetPseudo();
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
ComputeBackgroundAnchorPoint(const nsStyleBackground::Layer& aLayer,
                             const nsSize& aOriginBounds,
                             const nsSize& aImageSize,
                             nsPoint* aTopLeft,
                             nsPoint* aAnchorPoint)
{
  if (!aLayer.mPosition.mXIsPercent) {
    aTopLeft->x = aAnchorPoint->x = aLayer.mPosition.mXPosition.mCoord;
  }
  else {
    double percent = aLayer.mPosition.mXPosition.mFloat;
    aAnchorPoint->x = NSToCoordRound(percent*aOriginBounds.width);
    aTopLeft->x = NSToCoordRound(percent*(aOriginBounds.width - aImageSize.width));
  }

  if (!aLayer.mPosition.mYIsPercent) {
    aTopLeft->y = aAnchorPoint->y = aLayer.mPosition.mYPosition.mCoord;
  }
  else {
    double percent = aLayer.mPosition.mYPosition.mFloat;
    aAnchorPoint->y = NSToCoordRound(percent*aOriginBounds.height);
    aTopLeft->y = NSToCoordRound(percent*(aOriginBounds.height - aImageSize.height));
  }
}

nsStyleContext*
nsCSSRendering::FindNonTransparentBackground(nsStyleContext* aContext,
                                             PRBool aStartAtParent )
{
  NS_ASSERTION(aContext, "Cannot find NonTransparentBackground in a null context" );
  
  nsStyleContext* context = nsnull;
  if (aStartAtParent) {
    context = aContext->GetParent();
  }
  if (!context) {
    context = aContext;
  }
  
  while (context) {
    const nsStyleBackground* bg = context->GetStyleBackground();
    if (NS_GET_A(bg->mBackgroundColor) > 0)
      break;

    const nsStyleDisplay* display = context->GetStyleDisplay();
    if (display->mAppearance)
      break;

    nsStyleContext* parent = context->GetParent();
    if (!parent)
      break;

    context = parent;
  }
  return context;
}





PRBool
nsCSSRendering::IsCanvasFrame(nsIFrame* aFrame)
{
  nsIAtom* frameType = aFrame->GetType();
  return frameType == nsGkAtoms::canvasFrame ||
         frameType == nsGkAtoms::rootFrame ||
         frameType == nsGkAtoms::pageFrame ||
         frameType == nsGkAtoms::pageContentFrame ||
         frameType == nsGkAtoms::viewportFrame;
}

nsIFrame*
nsCSSRendering::FindBackgroundStyleFrame(nsIFrame* aForFrame)
{
  const nsStyleBackground* result = aForFrame->GetStyleBackground();

  
  if (result->IsTransparent()) {
    nsIContent* content = aForFrame->GetContent();
    
    
    
    if (content) {
      nsIDocument* document = content->GetOwnerDoc();
      nsCOMPtr<nsIHTMLDocument> htmlDoc = do_QueryInterface(document);
      if (htmlDoc) {
        nsIContent* bodyContent = htmlDoc->GetBodyContentExternal();
        
        
        
        
        
        
        
        
        
        if (bodyContent) {
          nsIFrame *bodyFrame = aForFrame->PresContext()->GetPresShell()->
            GetPrimaryFrameFor(bodyContent);
          if (bodyFrame) {
            return nsLayoutUtils::GetStyleFrame(bodyFrame);
          }
        }
      }
    }
  }

  return aForFrame;
}




























const nsStyleBackground*
nsCSSRendering::FindRootFrameBackground(nsIFrame* aForFrame)
{
  return FindBackgroundStyleFrame(aForFrame)->GetStyleBackground();
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
  
  

  if (aForFrame->GetStyleContext()->GetPseudo())
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
                               const nsStyleBackground** aBackground)
{
  nsIFrame* rootElementFrame =
    aPresContext->PresShell()->FrameConstructor()->GetRootElementStyleFrame();
  if (IsCanvasFrame(aForFrame)) {
    *aBackground = FindCanvasBackground(aForFrame, rootElementFrame);
    return PR_TRUE;
  } else {
    return FindElementBackground(aForFrame, rootElementFrame, aBackground);
  }
}

void
nsCSSRendering::DidPaint()
{
  gInlineBGData->Reset();
}

PRBool
nsCSSRendering::GetBorderRadiusTwips(const nsStyleCorners& aBorderRadius,
                                     const nscoord& aFrameWidth,
                                     nscoord aRadii[8])
{
  PRBool result = PR_FALSE;
  
  
  NS_FOR_CSS_HALF_CORNERS(i) {
    const nsStyleCoord c = aBorderRadius.Get(i);

    switch (c.GetUnit()) {
      case eStyleUnit_Percent:
        aRadii[i] = (nscoord)(c.GetPercentValue() * aFrameWidth);
        break;

      case eStyleUnit_Coord:
        aRadii[i] = c.GetCoordValue();
        break;

      default:
        NS_NOTREACHED("GetBorderRadiusTwips: bad unit");
        aRadii[i] = 0;
        break;
    }

    if (aRadii[i])
      result = PR_TRUE;
  }
  return result;
}

void
nsCSSRendering::PaintBoxShadowOuter(nsPresContext* aPresContext,
                                    nsIRenderingContext& aRenderingContext,
                                    nsIFrame* aForFrame,
                                    const nsRect& aFrameArea,
                                    const nsRect& aDirtyRect)
{
  nsCSSShadowArray* shadows = aForFrame->GetEffectiveBoxShadows();
  if (!shadows)
    return;
  const nsStyleBorder* styleBorder = aForFrame->GetStyleBorder();
  PRIntn sidesToSkip = aForFrame->GetSkipSides();

  
  nscoord twipsRadii[8];
  PRBool hasBorderRadius = GetBorderRadiusTwips(styleBorder->mBorderRadius,
                                                aFrameArea.width, twipsRadii);
  nscoord twipsPerPixel = aPresContext->DevPixelsToAppUnits(1);

  gfxCornerSizes borderRadii;
  ComputePixelRadii(twipsRadii, aFrameArea, sidesToSkip,
                    twipsPerPixel, &borderRadii);

  gfxRect frameGfxRect = RectToGfxRect(aFrameArea, twipsPerPixel);
  frameGfxRect.Round();

  for (PRUint32 i = shadows->Length(); i > 0; --i) {
    nsCSSShadowItem* shadowItem = shadows->ShadowAt(i - 1);
    if (shadowItem->mInset)
      continue;

    nsRect shadowRect = aFrameArea;
    shadowRect.MoveBy(shadowItem->mXOffset, shadowItem->mYOffset);
    shadowRect.Inflate(shadowItem->mSpread, shadowItem->mSpread);

    
    
    nsRect shadowRectPlusBlur = shadowRect;
    nscoord blurRadius = shadowItem->mRadius;
    shadowRectPlusBlur.Inflate(blurRadius, blurRadius);

    gfxRect shadowGfxRect = RectToGfxRect(shadowRect, twipsPerPixel);
    gfxRect shadowGfxRectPlusBlur = RectToGfxRect(shadowRectPlusBlur, twipsPerPixel);
    shadowGfxRect.Round();
    shadowGfxRectPlusBlur.RoundOut();

    gfxContext* renderContext = aRenderingContext.ThebesContext();
    nsRefPtr<gfxContext> shadowContext;
    nsContextBoxBlur blurringArea;

    shadowContext = blurringArea.Init(shadowRect, blurRadius, twipsPerPixel, renderContext, aDirtyRect);
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
    renderContext->Rectangle(shadowGfxRectPlusBlur);
    if (hasBorderRadius)
      renderContext->RoundedRectangle(frameGfxRect, borderRadii);
    else
      renderContext->Rectangle(frameGfxRect);
    renderContext->SetFillRule(gfxContext::FILL_RULE_EVEN_ODD);
    renderContext->Clip();

    
    
    
    
    
    shadowContext->NewPath();
    if (hasBorderRadius) {
      gfxCornerSizes clipRectRadii;
      gfxFloat spreadDistance = -shadowItem->mSpread / twipsPerPixel;
      gfxFloat borderSizes[4] = {0, 0, 0, 0};

      
      
      
      
      if (borderRadii[C_TL].width > 0 || borderRadii[C_BL].width > 0) {
        borderSizes[NS_SIDE_LEFT] = spreadDistance;
      }

      if (borderRadii[C_TL].height > 0 || borderRadii[C_TR].height > 0) {
        borderSizes[NS_SIDE_TOP] = spreadDistance;
      }

      if (borderRadii[C_TR].width > 0 || borderRadii[C_BR].width > 0) {
        borderSizes[NS_SIDE_RIGHT] = spreadDistance;
      }

      if (borderRadii[C_BL].height > 0 || borderRadii[C_BR].height > 0) {
        borderSizes[NS_SIDE_BOTTOM] = spreadDistance;
      }

      nsCSSBorderRenderer::ComputeInnerRadii(borderRadii, borderSizes,
                                             &clipRectRadii);
      shadowContext->RoundedRectangle(shadowGfxRect, clipRectRadii);
    } else {
      shadowContext->Rectangle(shadowGfxRect);
    }
    shadowContext->Fill();

    blurringArea.DoPaint();
    renderContext->Restore();
  }
}

void
nsCSSRendering::PaintBoxShadowInner(nsPresContext* aPresContext,
                                    nsIRenderingContext& aRenderingContext,
                                    nsIFrame* aForFrame,
                                    const nsRect& aFrameArea,
                                    const nsRect& aDirtyRect)
{
  nsCSSShadowArray* shadows = aForFrame->GetEffectiveBoxShadows();
  if (!shadows)
    return;
  const nsStyleBorder* styleBorder = aForFrame->GetStyleBorder();

  
  nscoord twipsRadii[8];
  PRBool hasBorderRadius = GetBorderRadiusTwips(styleBorder->mBorderRadius,
                                                aFrameArea.width, twipsRadii);
  nscoord twipsPerPixel = aPresContext->DevPixelsToAppUnits(1);

  nsRect paddingRect = aFrameArea;
  nsMargin border = aForFrame->GetUsedBorder();
  aForFrame->ApplySkipSides(border);
  paddingRect.Deflate(border);

  gfxCornerSizes innerRadii;
  if (hasBorderRadius) {
    gfxCornerSizes borderRadii;
    PRIntn sidesToSkip = aForFrame->GetSkipSides();

    ComputePixelRadii(twipsRadii, aFrameArea, sidesToSkip,
                      twipsPerPixel, &borderRadii);
    gfxFloat borderSizes[4] = {
      border.top / twipsPerPixel, border.right / twipsPerPixel,
      border.bottom / twipsPerPixel, border.left / twipsPerPixel
    };
    nsCSSBorderRenderer::ComputeInnerRadii(borderRadii, borderSizes,
                                           &innerRadii);
  }

  for (PRUint32 i = shadows->Length(); i > 0; --i) {
    nsCSSShadowItem* shadowItem = shadows->ShadowAt(i - 1);
    if (!shadowItem->mInset)
      continue;

    






    nscoord blurRadius = shadowItem->mRadius;
    nsRect shadowPaintRect = paddingRect;
    shadowPaintRect.Inflate(blurRadius, blurRadius);

    nsRect shadowClipRect = paddingRect;
    shadowClipRect.MoveBy(shadowItem->mXOffset, shadowItem->mYOffset);
    shadowClipRect.Deflate(shadowItem->mSpread, shadowItem->mSpread);

    gfxContext* renderContext = aRenderingContext.ThebesContext();
    nsRefPtr<gfxContext> shadowContext;
    nsContextBoxBlur blurringArea;

    shadowContext = blurringArea.Init(shadowPaintRect, blurRadius, twipsPerPixel, renderContext, aDirtyRect);
    if (!shadowContext)
      continue;

    
    nscolor shadowColor;
    if (shadowItem->mHasColor)
      shadowColor = shadowItem->mColor;
    else
      shadowColor = aForFrame->GetStyleColor()->mColor;

    renderContext->Save();
    renderContext->SetColor(gfxRGBA(shadowColor));

    
    
    gfxRect shadowGfxRect = RectToGfxRect(paddingRect, twipsPerPixel);
    shadowGfxRect.Round();
    renderContext->NewPath();
    if (hasBorderRadius)
      renderContext->RoundedRectangle(shadowGfxRect, innerRadii, PR_FALSE);
    else
      renderContext->Rectangle(shadowGfxRect);
    renderContext->Clip();

    
    
    gfxRect shadowPaintGfxRect = RectToGfxRect(shadowPaintRect, twipsPerPixel);
    gfxRect shadowClipGfxRect = RectToGfxRect(shadowClipRect, twipsPerPixel);
    shadowPaintGfxRect.RoundOut();
    shadowClipGfxRect.Round();
    shadowContext->NewPath();
    shadowContext->Rectangle(shadowPaintGfxRect);
    if (hasBorderRadius) {
      
      gfxCornerSizes clipRectRadii;
      gfxFloat spreadDistance = shadowItem->mSpread / twipsPerPixel;
      gfxFloat borderSizes[4] = {0, 0, 0, 0};

      
      if (innerRadii[C_TL].width > 0 || innerRadii[C_BL].width > 0) {
        borderSizes[NS_SIDE_LEFT] = spreadDistance;
      }

      if (innerRadii[C_TL].height > 0 || innerRadii[C_TR].height > 0) {
        borderSizes[NS_SIDE_TOP] = spreadDistance;
      }

      if (innerRadii[C_TR].width > 0 || innerRadii[C_BR].width > 0) {
        borderSizes[NS_SIDE_RIGHT] = spreadDistance;
      }

      if (innerRadii[C_BL].height > 0 || innerRadii[C_BR].height > 0) {
        borderSizes[NS_SIDE_BOTTOM] = spreadDistance;
      }

      nsCSSBorderRenderer::ComputeInnerRadii(innerRadii, borderSizes,
                                             &clipRectRadii);
      shadowContext->RoundedRectangle(shadowClipGfxRect, clipRectRadii, PR_FALSE);
    } else {
      shadowContext->Rectangle(shadowClipGfxRect);
    }
    shadowContext->SetFillRule(gfxContext::FILL_RULE_EVEN_ODD);
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
                                PRUint32 aFlags,
                                nsRect* aBGClipRect)
{
  NS_PRECONDITION(aForFrame,
                  "Frame is expected to be provided to PaintBackground");

  const nsStyleBackground *background;
  if (!FindBackground(aPresContext, aForFrame, &background)) {
    
    
    
    
    
    if (!aForFrame->GetStyleDisplay()->mAppearance) {
      return;
    }

    nsIContent* content = aForFrame->GetContent();
    if (!content || content->GetParent()) {
      return;
    }

    background = aForFrame->GetStyleBackground();
  }

  PaintBackgroundWithSC(aPresContext, aRenderingContext, aForFrame,
                        aDirtyRect, aBorderArea, *background,
                        *aForFrame->GetStyleBorder(), aFlags,
                        aBGClipRect);
}

static PRBool
IsOpaqueBorderEdge(const nsStyleBorder& aBorder, PRUint32 aSide)
{
  if (aBorder.GetActualBorder().side(aSide) == 0)
    return PR_TRUE;
  switch (aBorder.GetBorderStyle(aSide)) {
  case NS_STYLE_BORDER_STYLE_SOLID:
  case NS_STYLE_BORDER_STYLE_GROOVE:
  case NS_STYLE_BORDER_STYLE_RIDGE:
  case NS_STYLE_BORDER_STYLE_INSET:
  case NS_STYLE_BORDER_STYLE_OUTSET:
    break;
  default:
    return PR_FALSE;
  }

  
  
  
  
  if (aBorder.GetBorderImage())
    return PR_FALSE;

  nscolor color;
  PRBool isForeground;
  aBorder.GetBorderColor(aSide, color, isForeground);

  
  
  if (isForeground)
    return PR_FALSE;

  return NS_GET_A(color) == 255;
}




static PRBool
IsOpaqueBorder(const nsStyleBorder& aBorder)
{
  if (aBorder.mBorderColors)
    return PR_FALSE;
  for (PRUint32 i = 0; i < 4; ++i) {
    if (!IsOpaqueBorderEdge(aBorder, i))
      return PR_FALSE;
  }
  return PR_TRUE;
}

static inline void
SetupDirtyRects(const nsRect& aBGClipArea, const nsRect& aCallerDirtyRect,
                nscoord aAppUnitsPerPixel,
                
                nsRect* aDirtyRect, gfxRect* aDirtyRectGfx)
{
  aDirtyRect->IntersectRect(aBGClipArea, aCallerDirtyRect);

  
  *aDirtyRectGfx = RectToGfxRect(*aDirtyRect, aAppUnitsPerPixel);
  NS_WARN_IF_FALSE(aDirtyRect->IsEmpty() || !aDirtyRectGfx->IsEmpty(),
                   "converted dirty rect should not be empty");
  NS_ABORT_IF_FALSE(!aDirtyRect->IsEmpty() || aDirtyRectGfx->IsEmpty(),
                    "second should be empty if first is");
}

static void
SetupBackgroundClip(gfxContext *aCtx, PRUint8 aBackgroundClip,
                    nsIFrame* aForFrame, const nsRect& aBorderArea,
                    const nsRect& aCallerDirtyRect, PRBool aHaveRoundedCorners,
                    const gfxCornerSizes& aBGRadii, nscoord aAppUnitsPerPixel,
                    gfxContextAutoSaveRestore* aAutoSR,
                    
                    nsRect* aBGClipArea, nsRect* aDirtyRect,
                    gfxRect* aDirtyRectGfx)
{
  *aBGClipArea = aBorderArea;
  PRBool radiiAreOuter = PR_TRUE;
  gfxCornerSizes clippedRadii = aBGRadii;
  if (aBackgroundClip != NS_STYLE_BG_CLIP_BORDER) {
    NS_ASSERTION(aBackgroundClip == NS_STYLE_BG_CLIP_PADDING,
                 "unexpected background-clip");
    nsMargin border = aForFrame->GetUsedBorder();
    aForFrame->ApplySkipSides(border);
    aBGClipArea->Deflate(border);

    if (aHaveRoundedCorners) {
      gfxFloat borderSizes[4] = {
        border.top / aAppUnitsPerPixel, border.right / aAppUnitsPerPixel,
        border.bottom / aAppUnitsPerPixel, border.left / aAppUnitsPerPixel
      };
      nsCSSBorderRenderer::ComputeInnerRadii(aBGRadii, borderSizes,
                                             &clippedRadii);
      radiiAreOuter = PR_FALSE;
    }
  }

  SetupDirtyRects(*aBGClipArea, aCallerDirtyRect, aAppUnitsPerPixel,
                  aDirtyRect, aDirtyRectGfx);

  if (aDirtyRectGfx->IsEmpty()) {
    
    
    return;
  }

  
  
  
  
  
  

  if (aHaveRoundedCorners) {
    gfxRect bgAreaGfx(RectToGfxRect(*aBGClipArea, aAppUnitsPerPixel));
    bgAreaGfx.Round();
    bgAreaGfx.Condition();

    if (bgAreaGfx.IsEmpty()) {
      
      
      NS_WARNING("converted background area should not be empty");
      
      aDirtyRectGfx->size.SizeTo(0.0, 0.0);
      return;
    }

    aAutoSR->Reset(aCtx);
    aCtx->NewPath();
    aCtx->RoundedRectangle(bgAreaGfx, clippedRadii, radiiAreOuter);
    aCtx->Clip();
  }
}

static nscolor
DetermineBackgroundColorInternal(nsPresContext* aPresContext,
                                 const nsStyleBackground& aBackground,
                                 nsIFrame* aFrame,
                                 PRBool& aDrawBackgroundImage,
                                 PRBool& aDrawBackgroundColor)
{
  aDrawBackgroundImage = PR_TRUE;
  aDrawBackgroundColor = PR_TRUE;

  if (aFrame->HonorPrintBackgroundSettings()) {
    aDrawBackgroundImage = aPresContext->GetBackgroundImageDraw();
    aDrawBackgroundColor = aPresContext->GetBackgroundColorDraw();
  }

  nscolor bgColor;
  if (aDrawBackgroundColor) {
    bgColor = aBackground.mBackgroundColor;
    if (NS_GET_A(bgColor) == 0)
      aDrawBackgroundColor = PR_FALSE;
  } else {
    
    
    
    
    bgColor = NS_RGB(255, 255, 255);
    if (aDrawBackgroundImage || !aBackground.IsTransparent())
      aDrawBackgroundColor = PR_TRUE;
    else
      bgColor = NS_RGBA(0,0,0,0);
  }

  return bgColor;
}

nscolor
nsCSSRendering::DetermineBackgroundColor(nsPresContext* aPresContext,
                                         const nsStyleBackground& aBackground,
                                         nsIFrame* aFrame)
{
  PRBool drawBackgroundImage;
  PRBool drawBackgroundColor;
  return DetermineBackgroundColorInternal(aPresContext,
                                          aBackground,
                                          aFrame,
                                          drawBackgroundImage,
                                          drawBackgroundColor);
}

static gfxFloat
ConvertGradientValueToPixels(const nsStyleCoord& aCoord,
                             gfxFloat aFillLength,
                             PRInt32 aAppUnitsPerPixel)
{
  switch (aCoord.GetUnit()) {
    case eStyleUnit_Percent:
      return aCoord.GetPercentValue() * aFillLength;
    case eStyleUnit_Coord:
      return NSAppUnitsToFloatPixels(aCoord.GetCoordValue(), aAppUnitsPerPixel);
    default:
      NS_WARNING("Unexpected coord unit");
      return 0;
  }
}






static gfxPoint
ComputeGradientLineEndFromAngle(const gfxPoint& aStart,
                                double aAngle,
                                const gfxSize& aBoxSize)
{
  double dx = cos(-aAngle);
  double dy = sin(-aAngle);
  gfxPoint farthestCorner(dx > 0 ? aBoxSize.width : 0,
                          dy > 0 ? aBoxSize.height : 0);
  gfxPoint delta = farthestCorner - aStart;
  double u = delta.x*dy - delta.y*dx;
  return farthestCorner + gfxPoint(-u*dy, u*dx);
}


static void
ComputeLinearGradientLine(nsPresContext* aPresContext,
                          nsStyleGradient* aGradient,
                          const gfxSize& aBoxSize,
                          gfxPoint* aLineStart,
                          gfxPoint* aLineEnd)
{
  if (aGradient->mBgPosX.GetUnit() == eStyleUnit_None) {
    double angle;
    if (aGradient->mAngle.IsAngleValue()) {
      angle = aGradient->mAngle.GetAngleValueInRadians();
    } else {
      angle = -M_PI_2; 
    }
    gfxPoint center(aBoxSize.width/2, aBoxSize.height/2);
    *aLineEnd = ComputeGradientLineEndFromAngle(center, angle, aBoxSize);
    *aLineStart = gfxPoint(aBoxSize.width, aBoxSize.height) - *aLineEnd;
  } else {
    PRInt32 appUnitsPerPixel = aPresContext->AppUnitsPerDevPixel();
    *aLineStart = gfxPoint(
      ConvertGradientValueToPixels(aGradient->mBgPosX, aBoxSize.width,
                                   appUnitsPerPixel),
      ConvertGradientValueToPixels(aGradient->mBgPosY, aBoxSize.height,
                                   appUnitsPerPixel));
    if (aGradient->mAngle.IsAngleValue()) {
      double angle = aGradient->mAngle.GetAngleValueInRadians();
      *aLineEnd = ComputeGradientLineEndFromAngle(*aLineStart, angle, aBoxSize);
    } else {
      
      
      *aLineEnd = gfxPoint(aBoxSize.width, aBoxSize.height) - *aLineStart;
    }
  }
}




static void
ComputeRadialGradientLine(nsPresContext* aPresContext,
                          nsStyleGradient* aGradient,
                          const gfxSize& aBoxSize,
                          gfxPoint* aLineStart,
                          gfxPoint* aLineEnd,
                          double* aRadiusX,
                          double* aRadiusY)
{
  if (aGradient->mBgPosX.GetUnit() == eStyleUnit_None) {
    
    *aLineStart = gfxPoint(aBoxSize.width/2, aBoxSize.height/2);
  } else {
    PRInt32 appUnitsPerPixel = aPresContext->AppUnitsPerDevPixel();
    *aLineStart = gfxPoint(
      ConvertGradientValueToPixels(aGradient->mBgPosX, aBoxSize.width,
                                   appUnitsPerPixel),
      ConvertGradientValueToPixels(aGradient->mBgPosY, aBoxSize.height,
                                   appUnitsPerPixel));
  }

  
  double radiusX, radiusY;
  double leftDistance = PR_ABS(aLineStart->x);
  double rightDistance = PR_ABS(aBoxSize.width - aLineStart->x);
  double topDistance = PR_ABS(aLineStart->y);
  double bottomDistance = PR_ABS(aBoxSize.height - aLineStart->y);
  switch (aGradient->mSize) {
  case NS_STYLE_GRADIENT_SIZE_CLOSEST_SIDE:
    radiusX = NS_MIN(leftDistance, rightDistance);
    radiusY = NS_MIN(topDistance, bottomDistance);
    if (aGradient->mShape == NS_STYLE_GRADIENT_SHAPE_CIRCULAR) {
      radiusX = radiusY = NS_MIN(radiusX, radiusY);
    }
    break;
  case NS_STYLE_GRADIENT_SIZE_CLOSEST_CORNER: {
    
    double offsetX = NS_MIN(leftDistance, rightDistance);
    double offsetY = NS_MIN(topDistance, bottomDistance);
    if (aGradient->mShape == NS_STYLE_GRADIENT_SHAPE_CIRCULAR) {
      radiusX = radiusY = NS_hypot(offsetX, offsetY);
    } else {
      
      radiusX = offsetX*M_SQRT2;
      radiusY = offsetY*M_SQRT2;
    }
    break;
  }
  case NS_STYLE_GRADIENT_SIZE_FARTHEST_SIDE:
    radiusX = NS_MAX(leftDistance, rightDistance);
    radiusY = NS_MAX(topDistance, bottomDistance);
    if (aGradient->mShape == NS_STYLE_GRADIENT_SHAPE_CIRCULAR) {
      radiusX = radiusY = NS_MAX(radiusX, radiusY);
    }
    break;
  case NS_STYLE_GRADIENT_SIZE_FARTHEST_CORNER: {
    
    double offsetX = NS_MAX(leftDistance, rightDistance);
    double offsetY = NS_MAX(topDistance, bottomDistance);
    if (aGradient->mShape == NS_STYLE_GRADIENT_SHAPE_CIRCULAR) {
      radiusX = radiusY = NS_hypot(offsetX, offsetY);
    } else {
      
      radiusX = offsetX*M_SQRT2;
      radiusY = offsetY*M_SQRT2;
    }
    break;
  }
  default:
    NS_ABORT_IF_FALSE(PR_FALSE, "unknown radial gradient sizing method");
  }
  *aRadiusX = radiusX;
  *aRadiusY = radiusY;

  double angle;
  if (aGradient->mAngle.IsAngleValue()) {
    angle = aGradient->mAngle.GetAngleValueInRadians();
  } else {
    
    angle = 0.0;
  }

  
  
  *aLineEnd = *aLineStart + gfxPoint(radiusX*cos(-angle), radiusY*sin(-angle));
}



struct ColorStop {
  ColorStop(double aPosition, nscolor aColor) :
    mPosition(aPosition), mColor(aColor) {}
  double mPosition; 
  gfxRGBA mColor;
};




static gfxRGBA
InterpolateColor(const gfxRGBA& aC1, const gfxRGBA& aC2, double aFrac)
{
  double other = 1 - aFrac;
  return gfxRGBA(aC2.r*aFrac + aC1.r*other,
                 aC2.g*aFrac + aC1.g*other,
                 aC2.b*aFrac + aC1.b*other,
                 aC2.a*aFrac + aC1.a*other);
}

void
nsCSSRendering::PaintGradient(nsPresContext* aPresContext,
                              nsIRenderingContext& aRenderingContext,
                              nsStyleGradient* aGradient,
                              const nsRect& aDirtyRect,
                              const nsRect& aOneCellArea,
                              const nsRect& aFillArea)
{
  if (aOneCellArea.IsEmpty())
    return;

  gfxContext *ctx = aRenderingContext.ThebesContext();
  nscoord appUnitsPerPixel = aPresContext->AppUnitsPerDevPixel();
  gfxRect oneCellArea = RectToGfxRect(aOneCellArea, appUnitsPerPixel);

  
  gfxPoint lineStart, lineEnd;
  double radiusX = 0, radiusY = 0; 
  if (aGradient->mShape == NS_STYLE_GRADIENT_SHAPE_LINEAR) {
    ComputeLinearGradientLine(aPresContext, aGradient, oneCellArea.size,
                              &lineStart, &lineEnd);
  } else {
    ComputeRadialGradientLine(aPresContext, aGradient, oneCellArea.size,
                              &lineStart, &lineEnd, &radiusX, &radiusY);
  }
  gfxFloat lineLength = NS_hypot(lineEnd.x - lineStart.x,
                                 lineEnd.y - lineStart.y);

  NS_ABORT_IF_FALSE(aGradient->mStops.Length() >= 2,
                    "The parser should reject gradients with less than two stops");

  
  nsTArray<ColorStop> stops;
  
  
  
  PRInt32 firstUnsetPosition = -1;
  for (PRUint32 i = 0; i < aGradient->mStops.Length(); ++i) {
    const nsStyleGradientStop& stop = aGradient->mStops[i];
    double position;
    switch (stop.mLocation.GetUnit()) {
    case eStyleUnit_None:
      if (i == 0) {
        
        position = 0.0;
      } else if (i == aGradient->mStops.Length() - 1) {
        
        position = 1.0;
      } else {
        
        
        
        
        if (firstUnsetPosition < 0) {
          firstUnsetPosition = i;
        }
        stops.AppendElement(ColorStop(0, stop.mColor));
        continue;
      }
      break;
    case eStyleUnit_Percent:
      position = stop.mLocation.GetPercentValue();
      break;
    case eStyleUnit_Coord:
      position = lineLength < 1e-6 ? 0.0 :
          stop.mLocation.GetCoordValue() / appUnitsPerPixel / lineLength;
      break;
    default:
      NS_ABORT_IF_FALSE(PR_FALSE, "Unknown stop position type");
    }

    if (i > 0) {
      
      
      position = NS_MAX(position, stops[i - 1].mPosition);
    }
    stops.AppendElement(ColorStop(position, stop.mColor));
    if (firstUnsetPosition > 0) {
      
      double p = stops[firstUnsetPosition - 1].mPosition;
      double d = (stops[i].mPosition - p)/(i - firstUnsetPosition + 1);
      for (PRUint32 j = firstUnsetPosition; j < i; ++j) {
        p += d;
        stops[j].mPosition = p;
      }
      firstUnsetPosition = -1;
    }
  }

  
  double firstStop = stops[0].mPosition;
  if (aGradient->mShape != NS_STYLE_GRADIENT_SHAPE_LINEAR && firstStop < 0.0) {
    if (aGradient->mRepeating) {
      
      
      double lastStop = stops[stops.Length() - 1].mPosition;
      double stopDelta = lastStop - firstStop;
      
      
      
      
      if (stopDelta >= 1e-6) {
        double instanceCount = NS_ceil(-firstStop/stopDelta);
        
        
        double offset = instanceCount*stopDelta;
        for (PRUint32 i = 0; i < stops.Length(); i++) {
          stops[i].mPosition += offset;
        }
      }
    } else {
      
      
      
      for (PRUint32 i = 0; i < stops.Length(); i++) {
        double pos = stops[i].mPosition;
        if (pos < 0.0) {
          stops[i].mPosition = 0.0;
          
          
          if (i < stops.Length() - 1) {
            double nextPos = stops[i + 1].mPosition;
            
            
            
            
            
            
            if (nextPos >= 0.0 && nextPos - pos >= 1e-6) {
              
              
              
              
              double frac = (0.0 - pos)/(nextPos - pos);
              stops[i].mColor =
                InterpolateColor(stops[i].mColor, stops[i + 1].mColor, frac);
            }
          }
        }
      }
    }
    firstStop = stops[0].mPosition;
    NS_ABORT_IF_FALSE(firstStop >= 0.0, "Failed to fix stop offsets");
  }

  double lastStop = stops[stops.Length() - 1].mPosition;
  
  
  
  double stopScale;
  double stopDelta = lastStop - firstStop;
  if (stopDelta < 1e-6 || lineLength < 1e-6 ||
      (aGradient->mShape != NS_STYLE_GRADIENT_SHAPE_LINEAR &&
       (radiusX < 1e-6 || radiusY < 1e-6))) {
    
    
    
    stopScale = 0.0;
    radiusX = radiusY = 0.0;
    lastStop = firstStop;
  } else {
    stopScale = 1.0/stopDelta;
  }

  
  nsRefPtr<gfxPattern> gradientPattern;
  if (aGradient->mShape == NS_STYLE_GRADIENT_SHAPE_LINEAR) {
    
    
    gfxPoint gradientStart = lineStart + (lineEnd - lineStart)*firstStop;
    gfxPoint gradientEnd = lineStart + (lineEnd - lineStart)*lastStop;

    if (stopScale == 0.0) {
      
      
      
      
      
      
      gradientEnd = gradientStart + (lineEnd - lineStart);
    }

    gradientPattern = new gfxPattern(gradientStart.x, gradientStart.y,
                                     gradientEnd.x, gradientEnd.y);
  } else {
    NS_ASSERTION(firstStop >= 0.0,
                 "Negative stops not allowed for radial gradients");

    
    
    double innerRadius = radiusX*firstStop;
    double outerRadius = radiusX*lastStop;
    gradientPattern = new gfxPattern(lineStart.x, lineStart.y, innerRadius,
                                     lineStart.x, lineStart.y, outerRadius);
    if (gradientPattern && radiusX != radiusY) {
      
      
      
      
      
      gfxMatrix matrix;
      matrix.Translate(lineStart);
      matrix.Scale(1.0, radiusX/radiusY);
      matrix.Translate(-lineStart);
      gradientPattern->SetMatrix(matrix);
    }
  }
  if (!gradientPattern || gradientPattern->CairoStatus())
    return;

  
  if (stopScale == 0.0) {
    
    
    
    
    if (!aGradient->mRepeating &&
        aGradient->mShape == NS_STYLE_GRADIENT_SHAPE_LINEAR) {
      gradientPattern->AddColorStop(0.0, stops[0].mColor);
    }
    gradientPattern->AddColorStop(0.0, stops[stops.Length() - 1].mColor);
  } else {
    
    for (PRUint32 i = 0; i < stops.Length(); i++) {
      double pos = stopScale*(stops[i].mPosition - firstStop);
      gradientPattern->AddColorStop(pos, stops[i].mColor);
    }
  }

  
  if (aGradient->mRepeating) {
    gradientPattern->SetExtend(gfxPattern::EXTEND_REPEAT);
  }

  
  
  
  
  nsRect dirty;
  dirty.IntersectRect(aDirtyRect, aFillArea);
  gfxRect areaToFill = RectToGfxRect(aFillArea, appUnitsPerPixel);
  gfxMatrix ctm = ctx->CurrentMatrix();

  
  PRInt32 firstTileX = (dirty.x - aOneCellArea.x)/aOneCellArea.width;
  PRInt32 firstTileY = (dirty.y - aOneCellArea.y)/aOneCellArea.height;
  
  nscoord xStart = firstTileX*aOneCellArea.width + aOneCellArea.x;
  nscoord yStart = firstTileY*aOneCellArea.height + aOneCellArea.y;
  nscoord xEnd = dirty.XMost();
  nscoord yEnd = dirty.YMost();
  
  for (nscoord y = yStart; y < yEnd; y += aOneCellArea.height) {
    for (nscoord x = xStart; x < xEnd; x += aOneCellArea.width) {
      
      gfxRect tileRect =
        RectToGfxRect(nsRect(x, y, aOneCellArea.width, aOneCellArea.height),
                      appUnitsPerPixel);
      
      
      gfxRect fillRect = tileRect.Intersect(areaToFill);
      ctx->NewPath();
      ctx->Translate(tileRect.pos);
      ctx->SetPattern(gradientPattern);
      ctx->Rectangle(fillRect - tileRect.pos, PR_TRUE);
      ctx->Fill();
      ctx->SetMatrix(ctm);
    }
  }
}

void
nsCSSRendering::PaintBackgroundWithSC(nsPresContext* aPresContext,
                                      nsIRenderingContext& aRenderingContext,
                                      nsIFrame* aForFrame,
                                      const nsRect& aDirtyRect,
                                      const nsRect& aBorderArea,
                                      const nsStyleBackground& aBackground,
                                      const nsStyleBorder& aBorder,
                                      PRUint32 aFlags,
                                      nsRect* aBGClipRect)
{
  NS_PRECONDITION(aForFrame,
                  "Frame is expected to be provided to PaintBackground");

  
  
  
  const nsStyleDisplay* displayData = aForFrame->GetStyleDisplay();
  if (displayData->mAppearance) {
    nsITheme *theme = aPresContext->GetTheme();
    if (theme && theme->ThemeSupportsWidget(aPresContext, aForFrame,
                                            displayData->mAppearance)) {
      nsRect dirty;
      dirty.IntersectRect(aDirtyRect, aBorderArea);
      theme->DrawWidgetBackground(&aRenderingContext, aForFrame, 
                                  displayData->mAppearance, aBorderArea, dirty);
      return;
    }
  }

  
  
  
  
  PRBool isCanvasFrame = IsCanvasFrame(aForFrame);

  
  
  PRBool drawBackgroundImage;
  PRBool drawBackgroundColor;

  nscolor bgColor = DetermineBackgroundColorInternal(aPresContext,
                                                     aBackground,
                                                     aForFrame,
                                                     drawBackgroundImage,
                                                     drawBackgroundColor);

  
  
  
  if (!drawBackgroundImage && !drawBackgroundColor)
    return;

  
  gfxContext *ctx = aRenderingContext.ThebesContext();
  nscoord appUnitsPerPixel = aPresContext->AppUnitsPerDevPixel();

  
  gfxCornerSizes bgRadii;
  PRBool haveRoundedCorners;
  {
    nscoord radii[8];
    haveRoundedCorners =
      GetBorderRadiusTwips(aBorder.mBorderRadius, aForFrame->GetSize().width,
                           radii);
    if (haveRoundedCorners)
      ComputePixelRadii(radii, aBorderArea, aForFrame->GetSkipSides(),
                        appUnitsPerPixel, &bgRadii);
  }
  
  
  
  
  
  
  
  nsRect bgClipArea, dirtyRect;
  gfxRect dirtyRectGfx;
  PRUint8 currentBackgroundClip;
  PRBool isSolidBorder;
  gfxContextAutoSaveRestore autoSR;
  if (aBGClipRect) {
    bgClipArea = *aBGClipRect;
    SetupDirtyRects(bgClipArea, aDirtyRect, appUnitsPerPixel,
                    &dirtyRect, &dirtyRectGfx);
  } else {
    
    
    
    
    
    
    
    
    currentBackgroundClip = aBackground.BottomLayer().mClip;
    isSolidBorder =
      (aFlags & PAINTBG_WILL_PAINT_BORDER) && IsOpaqueBorder(aBorder);
    if (isSolidBorder)
      currentBackgroundClip = NS_STYLE_BG_CLIP_PADDING;
    SetupBackgroundClip(ctx, currentBackgroundClip, aForFrame,
                        aBorderArea, aDirtyRect, haveRoundedCorners,
                        bgRadii, appUnitsPerPixel, &autoSR,
                        &bgClipArea, &dirtyRect, &dirtyRectGfx);
  }

  
  if (drawBackgroundColor && !isCanvasFrame)
    ctx->SetColor(gfxRGBA(bgColor));

  
  
  
  if (!drawBackgroundImage) {
    if (!dirtyRectGfx.IsEmpty() && !isCanvasFrame) {
      ctx->NewPath();
      ctx->Rectangle(dirtyRectGfx, PR_TRUE);
      ctx->Fill();
    }
    return;
  }

  
  
  
  aPresContext->SetupBackgroundImageLoaders(aForFrame, &aBackground);

  
  if (drawBackgroundColor &&
      aBackground.BottomLayer().mRepeat == NS_STYLE_BG_REPEAT_XY &&
      aBackground.BottomLayer().mImage.IsOpaque())
    drawBackgroundColor = PR_FALSE;

  
  
  if (drawBackgroundColor && !isCanvasFrame) {
    if (!dirtyRectGfx.IsEmpty()) {
      ctx->NewPath();
      ctx->Rectangle(dirtyRectGfx, PR_TRUE);
      ctx->Fill();
    }
  }

  if (drawBackgroundImage) {
    NS_FOR_VISIBLE_BACKGROUND_LAYERS_BACK_TO_FRONT(i, &aBackground) {
      const nsStyleBackground::Layer &layer = aBackground.mLayers[i];
      if (!aBGClipRect) {
        PRUint8 newBackgroundClip =
          isSolidBorder ? NS_STYLE_BG_CLIP_PADDING : layer.mClip;
        if (currentBackgroundClip != newBackgroundClip) {
          currentBackgroundClip = newBackgroundClip;
          SetupBackgroundClip(ctx, currentBackgroundClip, aForFrame,
                              aBorderArea, aDirtyRect, haveRoundedCorners,
                              bgRadii, appUnitsPerPixel, &autoSR,
                              &bgClipArea, &dirtyRect, &dirtyRectGfx);
        }
      }
      if (!dirtyRectGfx.IsEmpty()) {
        PaintBackgroundLayer(aPresContext, aRenderingContext, aForFrame, aFlags,
                             dirtyRect, aBorderArea, bgClipArea, aBackground,
                             layer);
      }
    }
  }
}

static inline float
ScaleDimension(nsStyleBackground::Size::Dimension aDimension,
               PRUint8 aType,
               nscoord aLength, nscoord aAvailLength)
{
  switch (aType) {
    case nsStyleBackground::Size::ePercentage:
      return double(aDimension.mFloat) * (double(aAvailLength) / double(aLength));
    case nsStyleBackground::Size::eLength:
      return double(aDimension.mCoord) / double(aLength);
    default:
      NS_ABORT_IF_FALSE(PR_FALSE, "bad aDimension.mType");
      return 1.0f;
    case nsStyleBackground::Size::eAuto:
      NS_ABORT_IF_FALSE(PR_FALSE, "aDimension.mType == eAuto isn't handled");
      return 1.0f;
  }
}

static void
PaintBackgroundLayer(nsPresContext* aPresContext,
                     nsIRenderingContext& aRenderingContext,
                     nsIFrame* aForFrame,
                     PRUint32 aFlags,
                     const nsRect& aDirtyRect, 
                     const nsRect& aBorderArea,
                     const nsRect& aBGClipRect,
                     const nsStyleBackground& aBackground,
                     const nsStyleBackground::Layer& aLayer)
{
  






















































  PRUint32 irFlags = 0;
  if (aFlags & nsCSSRendering::PAINTBG_SYNC_DECODE_IMAGES)
    irFlags |= ImageRenderer::FLAG_SYNC_DECODE_IMAGES;
  ImageRenderer imageRenderer(aForFrame, aLayer.mImage, irFlags);
  if (!imageRenderer.PrepareImage()) {
    
    return;
  }

  
  
  nsRect bgPositioningArea(0, 0, 0, 0);

  nsIAtom* frameType = aForFrame->GetType();
  nsIFrame* geometryFrame = aForFrame;
  if (frameType == nsGkAtoms::inlineFrame ||
      frameType == nsGkAtoms::positionedInlineFrame) {
    
    
    
    
    
    switch (aBackground.mBackgroundInlinePolicy) {
    case NS_STYLE_BG_INLINE_POLICY_EACH_BOX:
      bgPositioningArea = nsRect(nsPoint(0,0), aBorderArea.Size());
      break;
    case NS_STYLE_BG_INLINE_POLICY_BOUNDING_BOX:
      bgPositioningArea = gInlineBGData->GetBoundingRect(aForFrame);
      break;
    default:
      NS_ERROR("Unknown background-inline-policy value!  "
               "Please, teach me what to do.");
    case NS_STYLE_BG_INLINE_POLICY_CONTINUOUS:
      bgPositioningArea = gInlineBGData->GetContinuousRect(aForFrame);
      break;
    }
  } else if (frameType == nsGkAtoms::canvasFrame) {
    geometryFrame = aForFrame->GetFirstChild(nsnull);
    
    
    
    
    if (geometryFrame) {
      bgPositioningArea = geometryFrame->GetRect();
    }
  } else {
    bgPositioningArea = nsRect(nsPoint(0,0), aBorderArea.Size());
  }

  
  
  if (aLayer.mOrigin != NS_STYLE_BG_ORIGIN_BORDER && geometryFrame) {
    nsMargin border = geometryFrame->GetUsedBorder();
    geometryFrame->ApplySkipSides(border);
    bgPositioningArea.Deflate(border);
    if (aLayer.mOrigin != NS_STYLE_BG_ORIGIN_PADDING) {
      nsMargin padding = geometryFrame->GetUsedPadding();
      geometryFrame->ApplySkipSides(padding);
      bgPositioningArea.Deflate(padding);
      NS_ASSERTION(aLayer.mOrigin == NS_STYLE_BG_ORIGIN_CONTENT,
                   "unknown background-origin value");
    }
  }

  
  
  
  
  nsPoint imageTopLeft, anchor, offset;
  if (NS_STYLE_BG_ATTACHMENT_FIXED == aLayer.mAttachment) {
    
    
    
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

    
    bgPositioningArea.SetRect(nsPoint(0, 0), topFrame->GetSize());

    if (!pageContentFrame) {
      
      nsIScrollableFrame* scrollableFrame =
        aPresContext->PresShell()->GetRootScrollFrameAsScrollable();
      if (scrollableFrame) {
        nsMargin scrollbars = scrollableFrame->GetActualScrollbarSizes();
        bgPositioningArea.Deflate(scrollbars);
      }
    }

    offset = bgPositioningArea.TopLeft() - aForFrame->GetOffsetTo(topFrame);
  } else {
    offset = bgPositioningArea.TopLeft();
  }

  nsSize imageSize = imageRenderer.ComputeSize(bgPositioningArea.Size());
  if (imageSize.width <= 0 || imageSize.height <= 0)
    return;
     
  
  
  
  float scaleX, scaleY;
  switch (aLayer.mSize.mWidthType) {
    case nsStyleBackground::Size::eContain:
    case nsStyleBackground::Size::eCover: {
      float scaleFitX = double(bgPositioningArea.width) / imageSize.width;
      float scaleFitY = double(bgPositioningArea.height) / imageSize.height;
      if (aLayer.mSize.mWidthType == nsStyleBackground::Size::eCover) {
        scaleX = scaleY = NS_MAX(scaleFitX, scaleFitY);
      } else {
        scaleX = scaleY = NS_MIN(scaleFitX, scaleFitY);
      }
      break;
    }
    default: {
      if (aLayer.mSize.mWidthType == nsStyleBackground::Size::eAuto) {
        if (aLayer.mSize.mHeightType == nsStyleBackground::Size::eAuto) {
          scaleX = scaleY = 1.0f;
        } else {
          scaleX = scaleY =
            ScaleDimension(aLayer.mSize.mHeight, aLayer.mSize.mHeightType,
                           imageSize.height, bgPositioningArea.height);
        }
      } else {
        if (aLayer.mSize.mHeightType == nsStyleBackground::Size::eAuto) {
          scaleX = scaleY =
            ScaleDimension(aLayer.mSize.mWidth, aLayer.mSize.mWidthType,
                           imageSize.width, bgPositioningArea.width);
        } else {
          scaleX = ScaleDimension(aLayer.mSize.mWidth, aLayer.mSize.mWidthType,
                                  imageSize.width, bgPositioningArea.width);
          scaleY = ScaleDimension(aLayer.mSize.mHeight, aLayer.mSize.mHeightType,
                                  imageSize.height, bgPositioningArea.height);
        }
      }
      break;
    }
  }
  imageSize.width = NSCoordSaturatingNonnegativeMultiply(imageSize.width, scaleX);
  imageSize.height = NSCoordSaturatingNonnegativeMultiply(imageSize.height, scaleY);

  
  
  ComputeBackgroundAnchorPoint(aLayer, bgPositioningArea.Size(), imageSize,
                               &imageTopLeft, &anchor);
  imageTopLeft += offset;
  anchor += offset;

  nsRect destArea(imageTopLeft + aBorderArea.TopLeft(), imageSize);
  nsRect fillArea = destArea;
  PRIntn repeat = aLayer.mRepeat;
  PR_STATIC_ASSERT(NS_STYLE_BG_REPEAT_XY ==
                   (NS_STYLE_BG_REPEAT_X | NS_STYLE_BG_REPEAT_Y));
  if (repeat & NS_STYLE_BG_REPEAT_X) {
    fillArea.x = aBGClipRect.x;
    fillArea.width = aBGClipRect.width;
  }
  if (repeat & NS_STYLE_BG_REPEAT_Y) {
    fillArea.y = aBGClipRect.y;
    fillArea.height = aBGClipRect.height;
  }
  fillArea.IntersectRect(fillArea, aBGClipRect);

  imageRenderer.Draw(aPresContext, aRenderingContext, destArea, fillArea,
                     anchor + aBorderArea.TopLeft(), aDirtyRect);
}

static void
DrawBorderImage(nsPresContext*       aPresContext,
                nsIRenderingContext& aRenderingContext,
                nsIFrame*            aForFrame,
                const nsRect&        aBorderArea,
                const nsStyleBorder& aStyleBorder,
                const nsRect&        aDirtyRect)
{
  if (aDirtyRect.IsEmpty())
    return;

  
  
  
  
  
  
  aPresContext->SetupBorderImageLoaders(aForFrame, &aStyleBorder);

  imgIRequest *req = aStyleBorder.GetBorderImage();

#ifdef DEBUG
  {
    PRUint32 status = imgIRequest::STATUS_ERROR;
    if (req)
      req->GetImageStatus(&status);

    NS_ASSERTION(req && (status & imgIRequest::STATUS_LOAD_COMPLETE),
                 "no image to draw");
  }
#endif

  
  
  

  nsCOMPtr<imgIContainer> imgContainer;
  req->GetImage(getter_AddRefs(imgContainer));

  nsIntSize imageSize;
  imgContainer->GetWidth(&imageSize.width);
  imgContainer->GetHeight(&imageSize.height);

  
  nsIntMargin split;
  NS_FOR_CSS_SIDES(s) {
    nsStyleCoord coord = aStyleBorder.mBorderImageSplit.Get(s);
    PRInt32 imgDimension = ((s == NS_SIDE_TOP || s == NS_SIDE_BOTTOM)
                            ? imageSize.height
                            : imageSize.width);
    double value;
    switch (coord.GetUnit()) {
      case eStyleUnit_Percent:
        value = coord.GetPercentValue() * imgDimension;
        break;
      case eStyleUnit_Factor:
        value = coord.GetFactorValue();
        break;
      default:
        NS_ASSERTION(coord.GetUnit() == eStyleUnit_Null,
                     "unexpected CSS unit for image split");
        value = 0;
        break;
    }
    if (value < 0)
      value = 0;
    if (value > imgDimension)
      value = imgDimension;
    split.side(s) = NS_lround(value);
  }

  nsMargin border(aStyleBorder.GetActualBorder());

  
  
  
  
  enum {
    LEFT, MIDDLE, RIGHT,
    TOP = LEFT, BOTTOM = RIGHT
  };
  const nscoord borderX[3] = {
    aBorderArea.x + 0,
    aBorderArea.x + border.left,
    aBorderArea.x + aBorderArea.width - border.right,
  };
  const nscoord borderY[3] = {
    aBorderArea.y + 0,
    aBorderArea.y + border.top,
    aBorderArea.y + aBorderArea.height - border.bottom,
  };
  const nscoord borderWidth[3] = {
    border.left,
    aBorderArea.width - border.left - border.right,
    border.right,
  };
  const nscoord borderHeight[3] = {
    border.top,
    aBorderArea.height - border.top - border.bottom,
    border.bottom,
  };

  const PRInt32 splitX[3] = {
    0,
    split.left,
    imageSize.width - split.right,
  };
  const PRInt32 splitY[3] = {
    0,
    split.top,
    imageSize.height - split.bottom,
  };
  const PRInt32 splitWidth[3] = {
    split.left,
    imageSize.width - split.left - split.right,
    split.right,
  };
  const PRInt32 splitHeight[3] = {
    split.top,
    imageSize.height - split.top - split.bottom,
    split.bottom,
  };

  
  
  
  
  for (int i = LEFT; i <= RIGHT; i++) {
    for (int j = TOP; j <= BOTTOM; j++) {
      nsRect destArea(borderX[i], borderY[j], borderWidth[i], borderHeight[j]);
      nsIntRect subArea(splitX[i], splitY[j], splitWidth[i], splitHeight[j]);

      PRUint8 fillStyleH, fillStyleV;
      nsSize unitSize;

      if (i == MIDDLE && j == MIDDLE) {
        
        
        
        
        
        
        
        
        
        gfxFloat hFactor, vFactor;

        if (0 < border.left && 0 < split.left)
          vFactor = gfxFloat(border.left)/split.left;
        else if (0 < border.right && 0 < split.right)
          vFactor = gfxFloat(border.right)/split.right;
        else
          vFactor = nsPresContext::CSSPixelsToAppUnits(1);

        if (0 < border.top && 0 < split.top)
          hFactor = gfxFloat(border.top)/split.top;
        else if (0 < border.bottom && 0 < split.bottom)
          hFactor = gfxFloat(border.bottom)/split.bottom;
        else
          hFactor = nsPresContext::CSSPixelsToAppUnits(1);

        unitSize.width = splitWidth[i]*hFactor;
        unitSize.height = splitHeight[j]*vFactor;
        fillStyleH = aStyleBorder.mBorderImageHFill;
        fillStyleV = aStyleBorder.mBorderImageVFill;

      } else if (i == MIDDLE) { 
        
        
        gfxFloat factor;
        if (0 < borderHeight[j] && 0 < splitHeight[j])
          factor = gfxFloat(borderHeight[j])/splitHeight[j];
        else
          factor = nsPresContext::CSSPixelsToAppUnits(1);

        unitSize.width = splitWidth[i]*factor;
        unitSize.height = borderHeight[j];
        fillStyleH = aStyleBorder.mBorderImageHFill;
        fillStyleV = NS_STYLE_BORDER_IMAGE_STRETCH;

      } else if (j == MIDDLE) { 
        gfxFloat factor;
        if (0 < borderWidth[i] && 0 < splitWidth[i])
          factor = gfxFloat(borderWidth[i])/splitWidth[i];
        else
          factor = nsPresContext::CSSPixelsToAppUnits(1);

        unitSize.width = borderWidth[i];
        unitSize.height = splitHeight[j]*factor;
        fillStyleH = NS_STYLE_BORDER_IMAGE_STRETCH;
        fillStyleV = aStyleBorder.mBorderImageVFill;

      } else {
        
        unitSize.width = borderWidth[i];
        unitSize.height = borderHeight[j];
        fillStyleH = NS_STYLE_BORDER_IMAGE_STRETCH;
        fillStyleV = NS_STYLE_BORDER_IMAGE_STRETCH;
      }

      DrawBorderImageComponent(aRenderingContext, aForFrame,
                               imgContainer, aDirtyRect,
                               destArea, subArea,
                               fillStyleH, fillStyleV,
                               unitSize, aStyleBorder, i * (RIGHT + 1) + j);
    }
  }
}

static void
DrawBorderImageComponent(nsIRenderingContext& aRenderingContext,
                         nsIFrame*            aForFrame,
                         imgIContainer*       aImage,
                         const nsRect&        aDirtyRect,
                         const nsRect&        aFill,
                         const nsIntRect&     aSrc,
                         PRUint8              aHFill,
                         PRUint8              aVFill,
                         const nsSize&        aUnitSize,
                         const nsStyleBorder& aStyleBorder,
                         PRUint8              aIndex)
{
  if (aFill.IsEmpty() || aSrc.IsEmpty())
    return;

  
  
  PRBool animated = PR_TRUE;
  aImage->GetAnimated(&animated);

  nsCOMPtr<imgIContainer> subImage;
  if (animated || (subImage = aStyleBorder.GetSubImage(aIndex)) == 0) {
    if (NS_FAILED(aImage->ExtractFrame(imgIContainer::FRAME_CURRENT, aSrc,
                                       imgIContainer::FLAG_SYNC_DECODE,
                                       getter_AddRefs(subImage))))
      return;

    if (!animated)
      aStyleBorder.SetSubImage(aIndex, subImage);
  }

  gfxPattern::GraphicsFilter graphicsFilter =
    nsLayoutUtils::GetGraphicsFilterForFrame(aForFrame);

  
  
  if ((aHFill == NS_STYLE_BORDER_IMAGE_STRETCH &&
       aVFill == NS_STYLE_BORDER_IMAGE_STRETCH) ||
      (aUnitSize.width == aFill.width &&
       aUnitSize.height == aFill.height)) {
    nsLayoutUtils::DrawSingleImage(&aRenderingContext, subImage,
                                   graphicsFilter,
                                   aFill, aDirtyRect, imgIContainer::FLAG_NONE);
    return;
  }

  
  nsRect tile;
  switch (aHFill) {
  case NS_STYLE_BORDER_IMAGE_STRETCH:
    tile.x = aFill.x;
    tile.width = aFill.width;
    break;
  case NS_STYLE_BORDER_IMAGE_REPEAT:
    tile.x = aFill.x + aFill.width/2 - aUnitSize.width/2;
    tile.width = aUnitSize.width;
    break;

  case NS_STYLE_BORDER_IMAGE_ROUND:
    tile.x = aFill.x;
    tile.width = aFill.width / ceil(gfxFloat(aFill.width)/aUnitSize.width);
    break;

  default:
    NS_NOTREACHED("unrecognized border-image fill style");
  }

  switch (aVFill) {
  case NS_STYLE_BORDER_IMAGE_STRETCH:
    tile.y = aFill.y;
    tile.height = aFill.height;
    break;
  case NS_STYLE_BORDER_IMAGE_REPEAT:
    tile.y = aFill.y + aFill.height/2 - aUnitSize.height/2;
    tile.height = aUnitSize.height;
    break;

  case NS_STYLE_BORDER_IMAGE_ROUND:
    tile.y = aFill.y;
    tile.height = aFill.height/ceil(gfxFloat(aFill.height)/aUnitSize.height);
    break;

  default:
    NS_NOTREACHED("unrecognized border-image fill style");
  }

  nsLayoutUtils::DrawImage(&aRenderingContext, subImage, graphicsFilter,
                           tile, aFill, tile.TopLeft(), aDirtyRect,
                           imgIContainer::FLAG_NONE);
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
    aNumDashSpaces = (aBorderLength - aDashLength)/ (2 * aDashLength); 
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
      minDashLength = NS_MAX(minDashLength, twipsPerPixel);
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
                                    const PRUint8 aStyle,
                                    const gfxFloat aDescentLimit)
{
  NS_ASSERTION(aStyle != DECORATION_STYLE_NONE, "aStyle is none");

  gfxRect rect =
    GetTextDecorationRectInternal(aPt, aLineSize, aAscent, aOffset,
                                  aDecoration, aStyle, aDescentLimit);
  if (rect.IsEmpty())
    return;

  if (aDecoration != NS_STYLE_TEXT_DECORATION_UNDERLINE &&
      aDecoration != NS_STYLE_TEXT_DECORATION_OVERLINE &&
      aDecoration != NS_STYLE_TEXT_DECORATION_LINE_THROUGH)
  {
    NS_ERROR("Invalid decoration value!");
    return;
  }

  gfxFloat lineHeight = NS_MAX(NS_round(aLineSize.height), 1.0);
  PRBool contextIsSaved = PR_FALSE;

  gfxFloat oldLineWidth;
  nsRefPtr<gfxPattern> oldPattern;

  switch (aStyle) {
    case DECORATION_STYLE_SOLID:
    case DECORATION_STYLE_DOUBLE:
      oldLineWidth = aGfxContext->CurrentLineWidth();
      oldPattern = aGfxContext->GetPattern();
      break;
    case DECORATION_STYLE_DASHED: {
      aGfxContext->Save();
      contextIsSaved = PR_TRUE;
      aGfxContext->Clip(rect);
      gfxFloat dashWidth = lineHeight * DOT_LENGTH * DASH_LENGTH;
      gfxFloat dash[2] = { dashWidth, dashWidth };
      aGfxContext->SetLineCap(gfxContext::LINE_CAP_BUTT);
      aGfxContext->SetDash(dash, 2, 0.0);
      
      rect.size.width += dashWidth;
      break;
    }
    case DECORATION_STYLE_DOTTED: {
      aGfxContext->Save();
      contextIsSaved = PR_TRUE;
      aGfxContext->Clip(rect);
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
      
      rect.size.width += dashWidth;
      break;
    }
    case DECORATION_STYLE_WAVY:
      aGfxContext->Save();
      contextIsSaved = PR_TRUE;
      aGfxContext->Clip(rect);
      if (lineHeight > 2.0) {
        aGfxContext->SetAntialiasMode(gfxContext::MODE_COVERAGE);
      } else {
        
        
        
        aGfxContext->SetAntialiasMode(gfxContext::MODE_ALIASED);
      }
      break;
    default:
      NS_ERROR("Invalid style value!");
      return;
  }

  
  rect.pos.y += lineHeight / 2;

  aGfxContext->SetColor(gfxRGBA(aColor));
  aGfxContext->SetLineWidth(lineHeight);
  switch (aStyle) {
    case DECORATION_STYLE_SOLID:
      aGfxContext->NewPath();
      aGfxContext->MoveTo(rect.TopLeft());
      aGfxContext->LineTo(rect.TopRight());
      aGfxContext->Stroke();
      break;
    case DECORATION_STYLE_DOUBLE:
      













      aGfxContext->NewPath();
      aGfxContext->MoveTo(rect.TopLeft());
      aGfxContext->LineTo(rect.TopRight());
      rect.size.height -= lineHeight;
      aGfxContext->MoveTo(rect.BottomLeft());
      aGfxContext->LineTo(rect.BottomRight());
      aGfxContext->Stroke();
      break;
    case DECORATION_STYLE_DOTTED:
    case DECORATION_STYLE_DASHED:
      aGfxContext->NewPath();
      aGfxContext->MoveTo(rect.TopLeft());
      aGfxContext->LineTo(rect.TopRight());
      aGfxContext->Stroke();
      break;
    case DECORATION_STYLE_WAVY: {
      




























      rect.pos.x += lineHeight / 2.0;
      aGfxContext->NewPath();

      gfxPoint pt(rect.pos);
      gfxFloat rightMost = pt.x + rect.Width() + lineHeight;
      gfxFloat adv = rect.Height() - lineHeight;
      gfxFloat flatLengthAtVertex = NS_MAX((lineHeight - 1.0) * 2.0, 1.0);

      pt.x -= lineHeight;
      aGfxContext->MoveTo(pt); 

      pt.x = rect.pos.x;
      aGfxContext->LineTo(pt); 

      PRBool goDown = PR_TRUE;
      while (pt.x < rightMost) {
        pt.x += adv;
        pt.y += goDown ? adv : -adv;

        aGfxContext->LineTo(pt); 

        pt.x += flatLengthAtVertex;
        aGfxContext->LineTo(pt); 

        goDown = !goDown;
      }
      aGfxContext->Stroke();
      break;
    }
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
                                      const PRUint8 aStyle,
                                      const gfxFloat aDescentLimit)
{
  NS_ASSERTION(aPresContext, "aPresContext is null");
  NS_ASSERTION(aStyle != DECORATION_STYLE_NONE, "aStyle is none");

  gfxRect rect =
    GetTextDecorationRectInternal(gfxPoint(0, 0), aLineSize, aAscent, aOffset,
                                  aDecoration, aStyle, aDescentLimit);
  
  nsRect r;
  r.x = aPresContext->GfxUnitsToAppUnits(rect.X());
  r.y = aPresContext->GfxUnitsToAppUnits(rect.Y());
  r.width = aPresContext->GfxUnitsToAppUnits(rect.Width());
  r.height = aPresContext->GfxUnitsToAppUnits(rect.Height());
  return r;
}

gfxRect
nsCSSRendering::GetTextDecorationRectInternal(const gfxPoint& aPt,
                                              const gfxSize& aLineSize,
                                              const gfxFloat aAscent,
                                              const gfxFloat aOffset,
                                              const PRUint8 aDecoration,
                                              const PRUint8 aStyle,
                                              const gfxFloat aDescentLimit)
{
  NS_ASSERTION(aStyle <= DECORATION_STYLE_WAVY, "Invalid aStyle value");

  if (aStyle == DECORATION_STYLE_NONE)
    return gfxRect(0, 0, 0, 0);

  PRBool canLiftUnderline = aDescentLimit >= 0.0;

  gfxRect r;
  r.pos.x = NS_floor(aPt.x + 0.5);
  r.size.width = NS_round(aLineSize.width);

  gfxFloat lineHeight = NS_round(aLineSize.height);
  lineHeight = NS_MAX(lineHeight, 1.0);

  gfxFloat ascent = NS_round(aAscent);
  gfxFloat descentLimit = NS_floor(aDescentLimit);

  gfxFloat suggestedMaxRectHeight = NS_MAX(NS_MIN(ascent, descentLimit), 1.0);
  r.size.height = lineHeight;
  if (aStyle == DECORATION_STYLE_DOUBLE) {
    














    gfxFloat gap = NS_round(lineHeight / 2.0);
    gap = NS_MAX(gap, 1.0);
    r.size.height = lineHeight * 2.0 + gap;
    if (canLiftUnderline) {
      if (r.Height() > suggestedMaxRectHeight) {
        
        
        r.size.height = NS_MAX(suggestedMaxRectHeight, lineHeight * 2.0 + 1.0);
      }
    }
  } else if (aStyle == DECORATION_STYLE_WAVY) {
    












    r.size.height = lineHeight > 2.0 ? lineHeight * 4.0 : lineHeight * 3.0;
    if (canLiftUnderline) {
      if (r.Height() > suggestedMaxRectHeight) {
        
        
        
        
        r.size.height = NS_MAX(suggestedMaxRectHeight, lineHeight * 2.0);
      }
    }
  }

  gfxFloat baseline = NS_floor(aPt.y + aAscent + 0.5);
  gfxFloat offset = 0.0;
  switch (aDecoration) {
    case NS_STYLE_TEXT_DECORATION_UNDERLINE:
      offset = aOffset;
      if (canLiftUnderline) {
        if (descentLimit < -offset + r.Height()) {
          
          
          
          
          gfxFloat offsetBottomAligned = -descentLimit + r.Height();
          gfxFloat offsetTopAligned = 0.0;
          offset = NS_MIN(offsetBottomAligned, offsetTopAligned);
        }
      }
      break;
    case NS_STYLE_TEXT_DECORATION_OVERLINE:
      offset = aOffset - lineHeight + r.Height();
      break;
    case NS_STYLE_TEXT_DECORATION_LINE_THROUGH: {
      gfxFloat extra = NS_floor(r.Height() / 2.0 + 0.5);
      extra = NS_MAX(extra, lineHeight);
      offset = aOffset - lineHeight + extra;
      break;
    }
    default:
      NS_ERROR("Invalid decoration value!");
  }
  r.pos.y = baseline - NS_floor(offset + 0.5);
  return r;
}




ImageRenderer::ImageRenderer(nsIFrame* aForFrame,
                                       const nsStyleImage& aImage,
                                       PRUint32 aFlags)
  : mForFrame(aForFrame)
  , mImage(aImage)
  , mType(aImage.GetType())
  , mImageContainer(nsnull)
  , mGradientData(nsnull)
  , mIsReady(PR_FALSE)
  , mSize(0, 0)
  , mFlags(aFlags)
{
}

ImageRenderer::~ImageRenderer()
{
}

PRBool
ImageRenderer::PrepareImage()
{
  if (mImage.IsEmpty() || !mImage.IsComplete()) {
    
    mImage.RequestDecode();

    
    
    
    
    nsCOMPtr<imgIContainer> img;
    if (!((mFlags & FLAG_SYNC_DECODE_IMAGES) &&
          (mType == eStyleImageType_Image) &&
          (NS_SUCCEEDED(mImage.GetImageData()->GetImage(getter_AddRefs(img))) && img)))
    return PR_FALSE;
  }

  switch (mType) {
    case eStyleImageType_Image:
    {
      nsCOMPtr<imgIContainer> srcImage;
      mImage.GetImageData()->GetImage(getter_AddRefs(srcImage));
      NS_ABORT_IF_FALSE(srcImage, "If srcImage is null, mImage.IsComplete() "
                                  "should have returned false");

      if (!mImage.GetCropRect()) {
        mImageContainer.swap(srcImage);
      } else {
        nsIntRect actualCropRect;
        PRBool isEntireImage;
        PRBool success =
          mImage.ComputeActualCropRect(actualCropRect, &isEntireImage);
        NS_ASSERTION(success, "ComputeActualCropRect() should not fail here");
        if (!success || actualCropRect.IsEmpty()) {
          
          return PR_FALSE;
        }
        if (isEntireImage) {
          
          mImageContainer.swap(srcImage);
        } else {
          nsCOMPtr<imgIContainer> subImage;
          PRUint32 aExtractFlags = (mFlags & FLAG_SYNC_DECODE_IMAGES)
                                     ? (PRUint32) imgIContainer::FLAG_SYNC_DECODE
                                     : (PRUint32) imgIContainer::FLAG_NONE;
          nsresult rv = srcImage->ExtractFrame(imgIContainer::FRAME_CURRENT,
                                               actualCropRect, aExtractFlags,
                                               getter_AddRefs(subImage));
          if (NS_FAILED(rv)) {
            NS_WARNING("The cropped image contains no pixels to draw; "
                       "maybe the crop rect is outside the image frame rect");
            return PR_FALSE;
          }
          mImageContainer.swap(subImage);
        }
      }
      mIsReady = PR_TRUE;
      break;
    }
    case eStyleImageType_Gradient:
      mGradientData = mImage.GetGradientData();
      mIsReady = PR_TRUE;
      break;
    case eStyleImageType_Null:
    default:
      break;
  }

  return mIsReady;
}

nsSize
ImageRenderer::ComputeSize(const nsSize& aDefault)
{
  NS_ASSERTION(mIsReady, "Ensure PrepareImage() has returned true "
                         "before calling me");

  switch (mType) {
    case eStyleImageType_Image:
    {
      nsIntSize imageIntSize;
      mImageContainer->GetWidth(&imageIntSize.width);
      mImageContainer->GetHeight(&imageIntSize.height);

      mSize.width = nsPresContext::CSSPixelsToAppUnits(imageIntSize.width);
      mSize.height = nsPresContext::CSSPixelsToAppUnits(imageIntSize.height);
      break;
    }
    case eStyleImageType_Gradient:
      mSize = aDefault;
      break;
    case eStyleImageType_Null:
    default:
      mSize.SizeTo(0, 0);
      break;
  }

  return mSize;
}

void
ImageRenderer::Draw(nsPresContext*       aPresContext,
                         nsIRenderingContext& aRenderingContext,
                         const nsRect&        aDest,
                         const nsRect&        aFill,
                         const nsPoint&       aAnchor,
                         const nsRect&        aDirty)
{
  if (!mIsReady) {
    NS_NOTREACHED("Ensure PrepareImage() has returned true before calling me");
    return;
  }

  if (aDest.IsEmpty() || aFill.IsEmpty())
    return;

  switch (mType) {
    case eStyleImageType_Image:
    {
      PRUint32 drawFlags = (mFlags & FLAG_SYNC_DECODE_IMAGES)
                             ? (PRUint32) imgIContainer::FLAG_SYNC_DECODE
                             : (PRUint32) imgIContainer::FLAG_NONE;
      nsLayoutUtils::DrawImage(&aRenderingContext, mImageContainer,
          nsLayoutUtils::GetGraphicsFilterForFrame(mForFrame),
          aDest, aFill, aAnchor, aDirty, drawFlags);
      break;
    }
    case eStyleImageType_Gradient:
      nsCSSRendering::PaintGradient(aPresContext, aRenderingContext,
          mGradientData, aDirty, aDest, aFill);
      break;
    case eStyleImageType_Null:
    default:
      break;
  }
}




gfxContext*
nsContextBoxBlur::Init(const nsRect& aRect, nscoord aBlurRadius,
                       PRInt32 aAppUnitsPerDevPixel,
                       gfxContext* aDestinationCtx,
                       const nsRect& aDirtyRect)
{
  PRInt32 blurRadius = static_cast<PRInt32>(aBlurRadius / aAppUnitsPerDevPixel);
  mDestinationCtx = aDestinationCtx;

  
  if (blurRadius <= 0) {
    mContext = aDestinationCtx;
    return mContext;
  }

  
  gfxRect rect = RectToGfxRect(aRect, aAppUnitsPerDevPixel);
  rect.RoundOut();

  if (rect.IsEmpty()) {
    mContext = aDestinationCtx;
    return mContext;
  }

  gfxRect dirtyRect = RectToGfxRect(aDirtyRect, aAppUnitsPerDevPixel);
  dirtyRect.RoundOut();

  
  mContext = blur.Init(rect, gfxIntSize(blurRadius, blurRadius), &dirtyRect);
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

