














































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
#include "nsCSSProps.h"
#include "nsContentUtils.h"
#ifdef MOZ_SVG
#include "nsSVGEffects.h"
#include "nsSVGIntegrationUtils.h"
#include "gfxDrawable.h"
#endif

#include "nsCSSRenderingBorders.h"









class ImageRenderer {
public:
  enum {
    FLAG_SYNC_DECODE_IMAGES = 0x01
  };
  ImageRenderer(nsIFrame* aForFrame, const nsStyleImage* aImage, PRUint32 aFlags);
  ~ImageRenderer();
  




  PRBool PrepareImage();
  



  nsSize ComputeSize(const nsSize& aDefault);
  



  void Draw(nsPresContext*       aPresContext,
            nsRenderingContext& aRenderingContext,
            const nsRect&        aDest,
            const nsRect&        aFill,
            const nsPoint&       aAnchor,
            const nsRect&        aDirty);

private:
  nsIFrame*                 mForFrame;
  const nsStyleImage*       mImage;
  nsStyleImageType          mType;
  nsCOMPtr<imgIContainer>   mImageContainer;
  nsRefPtr<nsStyleGradient> mGradientData;
#ifdef MOZ_SVG
  nsIFrame*                 mPaintServerFrame;
  nsLayoutUtils::SurfaceFromElementResult mImageElementSurface;
#endif
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
      nsIFrame* block = static_cast<nsIFrame*>
        (aFrame->Properties().Get(nsIFrame::IBSplitSpecialPrevSibling()));
      if (block) {
        
        NS_ASSERTION(!block->GetPrevContinuation(),
                     "Incorrect value for IBSplitSpecialPrevSibling");
        prevCont = static_cast<nsIFrame*>
          (block->Properties().Get(nsIFrame::IBSplitSpecialPrevSibling()));
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
      nsIFrame* block = static_cast<nsIFrame*>
        (aFrame->Properties().Get(nsIFrame::IBSplitSpecialSibling()));
      if (block) {
        nextCont = static_cast<nsIFrame*>
          (block->Properties().Get(nsIFrame::IBSplitSpecialSibling()));
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


static void DrawBorderImage(nsPresContext* aPresContext,
                            nsRenderingContext& aRenderingContext,
                            nsIFrame* aForFrame,
                            const nsRect& aBorderArea,
                            const nsStyleBorder& aStyleBorder,
                            const nsRect& aDirtyRect);

static void DrawBorderImageComponent(nsRenderingContext& aRenderingContext,
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

static nscolor MakeBevelColor(mozilla::css::Side whichSide, PRUint8 style,
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
MakeBevelColor(mozilla::css::Side whichSide, PRUint8 style,
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








 void
nsCSSRendering::ComputePixelRadii(const nscoord *aAppUnitsRadii,
                                  nscoord aAppUnitsPerPixel,
                                  gfxCornerSizes *oBorderRadii)
{
  gfxFloat radii[8];
  NS_FOR_CSS_HALF_CORNERS(corner)
    radii[corner] = gfxFloat(aAppUnitsRadii[corner]) / aAppUnitsPerPixel;

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
                            nsRenderingContext& aRenderingContext,
                            nsIFrame* aForFrame,
                            const nsRect& aDirtyRect,
                            const nsRect& aBorderArea,
                            nsStyleContext* aStyleContext,
                            PRIntn aSkipSides)
{
  nsStyleContext *styleIfVisited = aStyleContext->GetStyleIfVisited();
  const nsStyleBorder *styleBorder = aStyleContext->GetStyleBorder();
  
  
  if (!styleIfVisited) {
    PaintBorderWithStyleBorder(aPresContext, aRenderingContext, aForFrame,
                               aDirtyRect, aBorderArea, *styleBorder,
                               aStyleContext, aSkipSides);
    return;
  }

  nsStyleBorder newStyleBorder(*styleBorder);
  
  
#ifdef DEBUG
  newStyleBorder.mImageTracked = styleBorder->mImageTracked;
#endif

  NS_FOR_CSS_SIDES(side) {
    newStyleBorder.SetBorderColor(side,
      aStyleContext->GetVisitedDependentColor(
        nsCSSProps::SubpropertyEntryFor(eCSSProperty_border_color)[side]));
  }
  PaintBorderWithStyleBorder(aPresContext, aRenderingContext, aForFrame,
                             aDirtyRect, aBorderArea, newStyleBorder,
                             aStyleContext, aSkipSides);

#ifdef DEBUG
  newStyleBorder.mImageTracked = false;
#endif
}

void
nsCSSRendering::PaintBorderWithStyleBorder(nsPresContext* aPresContext,
                                           nsRenderingContext& aRenderingContext,
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

  
  
  nsIFrame* bgFrame = nsCSSRendering::FindNonTransparentBackgroundFrame
    (aForFrame, compatMode == eCompatibility_NavQuirks ? PR_TRUE : PR_FALSE);
  nsStyleContext* bgContext = bgFrame->GetStyleContext();
  nscolor bgColor =
    bgContext->GetVisitedDependentColor(eCSSProperty_background_color);

  border = aStyleBorder.GetComputedBorder();
  if ((0 == border.left) && (0 == border.right) &&
      (0 == border.top) && (0 == border.bottom)) {
    
    return;
  }

  nsSize frameSize = aForFrame->GetSize();
  if (&aStyleBorder == aForFrame->GetStyleBorder() &&
      frameSize == aBorderArea.Size()) {
    aForFrame->GetBorderRadii(twipsRadii);
  } else {
    nsIFrame::ComputeBorderRadii(aStyleBorder.mBorderRadius, frameSize,
                                 aBorderArea.Size(), aSkipSides, twipsRadii);
  }

  
  if (aSkipSides & SIDE_BIT_TOP) border.top = 0;
  if (aSkipSides & SIDE_BIT_RIGHT) border.right = 0;
  if (aSkipSides & SIDE_BIT_BOTTOM) border.bottom = 0;
  if (aSkipSides & SIDE_BIT_LEFT) border.left = 0;

  
  nsRect outerRect(aBorderArea);

  SF(" outerRect: %d %d %d %d\n", outerRect.x, outerRect.y, outerRect.width, outerRect.height);

  

  
  nscoord twipsPerPixel = aPresContext->DevPixelsToAppUnits(1);

  
  gfxRect oRect(nsLayoutUtils::RectToGfxRect(outerRect, twipsPerPixel));

  
  gfxFloat borderWidths[4] = { gfxFloat(border.top / twipsPerPixel),
                               gfxFloat(border.right / twipsPerPixel),
                               gfxFloat(border.bottom / twipsPerPixel),
                               gfxFloat(border.left / twipsPerPixel) };

  
  gfxCornerSizes borderRadii;
  ComputePixelRadii(twipsRadii, twipsPerPixel, &borderRadii);

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
                         bgColor);
  br.DrawBorders();

  ctx->Restore();

  SN();
}

static nsRect
GetOutlineInnerRect(nsIFrame* aFrame)
{
  nsRect* savedOutlineInnerRect = static_cast<nsRect*>
    (aFrame->Properties().Get(nsIFrame::OutlineInnerRectProperty()));
  if (savedOutlineInnerRect)
    return *savedOutlineInnerRect;
  
  
  
  return aFrame->GetVisualOverflowRect();
}

void
nsCSSRendering::PaintOutline(nsPresContext* aPresContext,
                             nsRenderingContext& aRenderingContext,
                             nsIFrame* aForFrame,
                             const nsRect& aDirtyRect,
                             const nsRect& aBorderArea,
                             nsStyleContext* aStyleContext)
{
  nscoord             twipsRadii[8];

  
  const nsStyleOutline* ourOutline = aStyleContext->GetStyleOutline();

  nscoord width;
  ourOutline->GetOutlineWidth(width);

  if (width == 0) {
    
    return;
  }

  nsIFrame* bgFrame = nsCSSRendering::FindNonTransparentBackgroundFrame
    (aForFrame, PR_FALSE);
  nsStyleContext* bgContext = bgFrame->GetStyleContext();
  nscolor bgColor =
    bgContext->GetVisitedDependentColor(eCSSProperty_background_color);

  
  
  
  
  
  
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
  nscoord offset = ourOutline->mOutlineOffset;
  innerRect.Inflate(offset, offset);
  
  
  
  
  
  if (innerRect.Contains(aDirtyRect))
    return;

  nsRect outerRect = innerRect;
  outerRect.Inflate(width, width);

  
  nsIFrame::ComputeBorderRadii(ourOutline->mOutlineRadius, aBorderArea.Size(),
                               outerRect.Size(), 0, twipsRadii);

  
  nscoord twipsPerPixel = aPresContext->DevPixelsToAppUnits(1);

  
  gfxRect oRect(nsLayoutUtils::RectToGfxRect(outerRect, twipsPerPixel));

  
  nsMargin outlineMargin(width, width, width, width);
  gfxCornerSizes outlineRadii;
  ComputePixelRadii(twipsRadii, twipsPerPixel, &outlineRadii);

  PRUint8 outlineStyle = ourOutline->GetOutlineStyle();
  PRUint8 outlineStyles[4] = { outlineStyle,
                               outlineStyle,
                               outlineStyle,
                               outlineStyle };

  
  
  nscolor outlineColor =
    aStyleContext->GetVisitedDependentColor(eCSSProperty_outline_color);
  nscolor outlineColors[4] = { outlineColor,
                               outlineColor,
                               outlineColor,
                               outlineColor };

  
  gfxFloat outlineWidths[4] = { gfxFloat(width / twipsPerPixel),
                                gfxFloat(width / twipsPerPixel),
                                gfxFloat(width / twipsPerPixel),
                                gfxFloat(width / twipsPerPixel) };

  
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
                         bgColor);
  br.DrawBorders();

  ctx->Restore();

  SN();
}

void
nsCSSRendering::PaintFocus(nsPresContext* aPresContext,
                           nsRenderingContext& aRenderingContext,
                           const nsRect& aFocusRect,
                           nscolor aColor)
{
  nscoord oneCSSPixel = nsPresContext::CSSPixelsToAppUnits(1);
  nscoord oneDevPixel = aPresContext->DevPixelsToAppUnits(1);

  gfxRect focusRect(nsLayoutUtils::RectToGfxRect(aFocusRect, oneDevPixel));

  gfxCornerSizes focusRadii;
  {
    nscoord twipsRadii[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    ComputePixelRadii(twipsRadii, oneDevPixel, &focusRadii);
  }
  gfxFloat focusWidths[4] = { gfxFloat(oneCSSPixel / oneDevPixel),
                              gfxFloat(oneCSSPixel / oneDevPixel),
                              gfxFloat(oneCSSPixel / oneDevPixel),
                              gfxFloat(oneCSSPixel / oneDevPixel) };

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
  double percentX = aLayer.mPosition.mXPosition.mPercent;
  nscoord lengthX = aLayer.mPosition.mXPosition.mLength;
  aAnchorPoint->x = lengthX + NSToCoordRound(percentX*aOriginBounds.width);
  aTopLeft->x = lengthX +
    NSToCoordRound(percentX*(aOriginBounds.width - aImageSize.width));

  double percentY = aLayer.mPosition.mYPosition.mPercent;
  nscoord lengthY = aLayer.mPosition.mYPosition.mLength;
  aAnchorPoint->y = lengthY + NSToCoordRound(percentY*aOriginBounds.height);
  aTopLeft->y = lengthY +
    NSToCoordRound(percentY*(aOriginBounds.height - aImageSize.height));
}

nsIFrame*
nsCSSRendering::FindNonTransparentBackgroundFrame(nsIFrame* aFrame,
                                                  PRBool aStartAtParent )
{
  NS_ASSERTION(aFrame, "Cannot find NonTransparentBackgroundFrame in a null frame");

  nsIFrame* frame = nsnull;
  if (aStartAtParent) {
    frame = nsLayoutUtils::GetParentOrPlaceholderFor(
              aFrame->PresContext()->FrameManager(), aFrame);
  }
  if (!frame) {
    frame = aFrame;
  }

  while (frame) {
    
    
    if (NS_GET_A(frame->GetStyleBackground()->mBackgroundColor) > 0)
      break;

    if (frame->IsThemed())
      break;

    nsIFrame* parent = nsLayoutUtils::GetParentOrPlaceholderFor(
                         frame->PresContext()->FrameManager(), frame);
    if (!parent)
      break;

    frame = parent;
  }
  return frame;
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
          nsIFrame *bodyFrame = bodyContent->GetPrimaryFrame();
          if (bodyFrame) {
            return nsLayoutUtils::GetStyleFrame(bodyFrame);
          }
        }
      }
    }
  }

  return aForFrame;
}




























nsStyleContext*
nsCSSRendering::FindRootFrameBackground(nsIFrame* aForFrame)
{
  return FindBackgroundStyleFrame(aForFrame)->GetStyleContext();
}

inline PRBool
FindElementBackground(nsIFrame* aForFrame, nsIFrame* aRootElementFrame,
                      nsStyleContext** aBackgroundSC)
{
  if (aForFrame == aRootElementFrame) {
    
    return PR_FALSE;
  }

  *aBackgroundSC = aForFrame->GetStyleContext();

  
  

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
                               nsStyleContext** aBackgroundSC)
{
  nsIFrame* rootElementFrame =
    aPresContext->PresShell()->FrameConstructor()->GetRootElementStyleFrame();
  if (IsCanvasFrame(aForFrame)) {
    *aBackgroundSC = FindCanvasBackground(aForFrame, rootElementFrame);
    return PR_TRUE;
  } else {
    return FindElementBackground(aForFrame, rootElementFrame, aBackgroundSC);
  }
}

void
nsCSSRendering::DidPaint()
{
  gInlineBGData->Reset();
}

void
nsCSSRendering::PaintBoxShadowOuter(nsPresContext* aPresContext,
                                    nsRenderingContext& aRenderingContext,
                                    nsIFrame* aForFrame,
                                    const nsRect& aFrameArea,
                                    const nsRect& aDirtyRect)
{
  const nsStyleBorder* styleBorder = aForFrame->GetStyleBorder();
  nsCSSShadowArray* shadows = styleBorder->mBoxShadow;
  if (!shadows)
    return;
  nscoord twipsPerPixel = aPresContext->DevPixelsToAppUnits(1);

  PRBool hasBorderRadius;
  PRBool nativeTheme; 
  gfxCornerSizes borderRadii;

  
  const nsStyleDisplay* styleDisplay = aForFrame->GetStyleDisplay();
  nsITheme::Transparency transparency;
  if (aForFrame->IsThemed(styleDisplay, &transparency)) {
    
    hasBorderRadius = PR_FALSE;
    
    
    nativeTheme = transparency != nsITheme::eOpaque;
  } else {
    nativeTheme = PR_FALSE;
    nscoord twipsRadii[8];
    NS_ASSERTION(aFrameArea.Size() == aForFrame->GetSize(), "unexpected size");
    hasBorderRadius = aForFrame->GetBorderRadii(twipsRadii);
    if (hasBorderRadius) {
      ComputePixelRadii(twipsRadii, twipsPerPixel, &borderRadii);
    }
  }

  nsRect frameRect =
    nativeTheme ? aForFrame->GetVisualOverflowRectRelativeToSelf() + aFrameArea.TopLeft() : aFrameArea;
  gfxRect frameGfxRect(nsLayoutUtils::RectToGfxRect(frameRect, twipsPerPixel));
  frameGfxRect.Round();

  
  
  gfxRect skipGfxRect = frameGfxRect;
  PRBool useSkipGfxRect = PR_TRUE;
  if (nativeTheme) {
    
    
    
    
    
    useSkipGfxRect = !aForFrame->IsLeaf();
    nsRect paddingRect =
      aForFrame->GetPaddingRect() - aForFrame->GetPosition() + aFrameArea.TopLeft();
    skipGfxRect = nsLayoutUtils::RectToGfxRect(paddingRect, twipsPerPixel);
  } else if (hasBorderRadius) {
    skipGfxRect.Inset(
        PR_MAX(borderRadii[C_TL].height, borderRadii[C_TR].height), 0,
        PR_MAX(borderRadii[C_BL].height, borderRadii[C_BR].height), 0);
  }

  for (PRUint32 i = shadows->Length(); i > 0; --i) {
    nsCSSShadowItem* shadowItem = shadows->ShadowAt(i - 1);
    if (shadowItem->mInset)
      continue;

    nsRect shadowRect = frameRect;
    shadowRect.MoveBy(shadowItem->mXOffset, shadowItem->mYOffset);
    nscoord pixelSpreadRadius;
    if (nativeTheme) {
      pixelSpreadRadius = shadowItem->mSpread;
    } else {
      shadowRect.Inflate(shadowItem->mSpread, shadowItem->mSpread);
      pixelSpreadRadius = 0;
    }

    
    
    nsRect shadowRectPlusBlur = shadowRect;
    nscoord blurRadius = shadowItem->mRadius;
    shadowRectPlusBlur.Inflate(
      nsContextBoxBlur::GetBlurRadiusMargin(blurRadius, twipsPerPixel));

    gfxRect shadowGfxRect =
      nsLayoutUtils::RectToGfxRect(shadowRect, twipsPerPixel);
    gfxRect shadowGfxRectPlusBlur =
      nsLayoutUtils::RectToGfxRect(shadowRectPlusBlur, twipsPerPixel);
    shadowGfxRect.Round();
    shadowGfxRectPlusBlur.RoundOut();

    gfxContext* renderContext = aRenderingContext.ThebesContext();
    nsRefPtr<gfxContext> shadowContext;
    nsContextBoxBlur blurringArea;

    
    
    
    
    shadowContext = 
      blurringArea.Init(shadowRect, pixelSpreadRadius,
                        blurRadius, twipsPerPixel, renderContext, aDirtyRect,
                        useSkipGfxRect ? &skipGfxRect : nsnull,
                        nativeTheme ? nsContextBoxBlur::FORCE_MASK : 0);
    if (!shadowContext)
      continue;

    
    nscolor shadowColor;
    if (shadowItem->mHasColor)
      shadowColor = shadowItem->mColor;
    else
      shadowColor = aForFrame->GetStyleColor()->mColor;

    renderContext->Save();
    renderContext->SetColor(gfxRGBA(shadowColor));

    
    
    
    
    
    if (nativeTheme) {
      
      

      
      gfxContextMatrixAutoSaveRestore save(shadowContext);
      nsIDeviceContext* devCtx = aPresContext->DeviceContext();
      nsRefPtr<nsRenderingContext> wrapperCtx;
      devCtx->CreateRenderingContextInstance(*getter_AddRefs(wrapperCtx));
      wrapperCtx->Init(devCtx, shadowContext);
      wrapperCtx->Translate(nsPoint(shadowItem->mXOffset, shadowItem->mYOffset));

      nsRect nativeRect;
      nativeRect.IntersectRect(frameRect, aDirtyRect);

      aPresContext->GetTheme()->DrawWidgetBackground(wrapperCtx, aForFrame,
          styleDisplay->mAppearance, aFrameArea, nativeRect);
    } else {
      
      
      renderContext->NewPath();
      renderContext->Rectangle(shadowGfxRectPlusBlur);
      if (hasBorderRadius) {
        renderContext->RoundedRectangle(frameGfxRect, borderRadii);
      } else {
        renderContext->Rectangle(frameGfxRect);
      }

      renderContext->SetFillRule(gfxContext::FILL_RULE_EVEN_ODD);
      renderContext->Clip();

      shadowContext->NewPath();
      if (hasBorderRadius) {
        gfxCornerSizes clipRectRadii;
        gfxFloat spreadDistance = -shadowItem->mSpread / twipsPerPixel;
        gfxFloat borderSizes[4] = { 0, 0, 0, 0 };

        
        
        
        
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
    }

    blurringArea.DoPaint();
    renderContext->Restore();
  }
}

void
nsCSSRendering::PaintBoxShadowInner(nsPresContext* aPresContext,
                                    nsRenderingContext& aRenderingContext,
                                    nsIFrame* aForFrame,
                                    const nsRect& aFrameArea,
                                    const nsRect& aDirtyRect)
{
  const nsStyleBorder* styleBorder = aForFrame->GetStyleBorder();
  nsCSSShadowArray* shadows = styleBorder->mBoxShadow;
  if (!shadows)
    return;
  if (aForFrame->IsThemed() && aForFrame->GetContent() &&
      !nsContentUtils::IsChromeDoc(aForFrame->GetContent()->GetCurrentDoc())) {
    
    
    
    
    return;
  }

  
  
  nscoord twipsRadii[8];
  NS_ASSERTION(aForFrame->GetType() == nsGkAtoms::fieldSetFrame ||
               aFrameArea.Size() == aForFrame->GetSize(), "unexpected size");
  PRBool hasBorderRadius = aForFrame->GetBorderRadii(twipsRadii);
  nscoord twipsPerPixel = aPresContext->DevPixelsToAppUnits(1);

  nsRect paddingRect = aFrameArea;
  nsMargin border = aForFrame->GetUsedBorder();
  aForFrame->ApplySkipSides(border);
  paddingRect.Deflate(border);

  gfxCornerSizes innerRadii;
  if (hasBorderRadius) {
    gfxCornerSizes borderRadii;

    ComputePixelRadii(twipsRadii, twipsPerPixel, &borderRadii);
    gfxFloat borderSizes[4] = {
      gfxFloat(border.top / twipsPerPixel),
      gfxFloat(border.right / twipsPerPixel),
      gfxFloat(border.bottom / twipsPerPixel),
      gfxFloat(border.left / twipsPerPixel)
    };
    nsCSSBorderRenderer::ComputeInnerRadii(borderRadii, borderSizes,
                                           &innerRadii);
  }

  for (PRUint32 i = shadows->Length(); i > 0; --i) {
    nsCSSShadowItem* shadowItem = shadows->ShadowAt(i - 1);
    if (!shadowItem->mInset)
      continue;

    






    nscoord blurRadius = shadowItem->mRadius;
    nsMargin blurMargin =
      nsContextBoxBlur::GetBlurRadiusMargin(blurRadius, twipsPerPixel);
    nsRect shadowPaintRect = paddingRect;
    shadowPaintRect.Inflate(blurMargin);

    nsRect shadowClipRect = paddingRect;
    shadowClipRect.MoveBy(shadowItem->mXOffset, shadowItem->mYOffset);
    shadowClipRect.Deflate(shadowItem->mSpread, shadowItem->mSpread);

    gfxCornerSizes clipRectRadii;
    if (hasBorderRadius) {
      
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
    }

    
    
    nsRect skipRect = shadowClipRect;
    skipRect.Deflate(blurMargin);
    gfxRect skipGfxRect = nsLayoutUtils::RectToGfxRect(skipRect, twipsPerPixel);
    if (hasBorderRadius) {
      skipGfxRect.Inset(PR_MAX(clipRectRadii[C_TL].height, clipRectRadii[C_TR].height), 0,
                        PR_MAX(clipRectRadii[C_BL].height, clipRectRadii[C_BR].height), 0);
    }

    
    
    
    
    gfxContext* renderContext = aRenderingContext.ThebesContext();
    nsRefPtr<gfxContext> shadowContext;
    nsContextBoxBlur blurringArea;
    shadowContext =
      blurringArea.Init(shadowPaintRect, 0, blurRadius, twipsPerPixel,
                        renderContext, aDirtyRect, &skipGfxRect);
    if (!shadowContext)
      continue;

    
    nscolor shadowColor;
    if (shadowItem->mHasColor)
      shadowColor = shadowItem->mColor;
    else
      shadowColor = aForFrame->GetStyleColor()->mColor;

    renderContext->Save();
    renderContext->SetColor(gfxRGBA(shadowColor));

    
    
    
    gfxRect shadowGfxRect =
      nsLayoutUtils::RectToGfxRect(paddingRect, twipsPerPixel);
    shadowGfxRect.Round();
    renderContext->NewPath();
    if (hasBorderRadius)
      renderContext->RoundedRectangle(shadowGfxRect, innerRadii, PR_FALSE);
    else
      renderContext->Rectangle(shadowGfxRect);
    renderContext->Clip();

    
    
    gfxRect shadowPaintGfxRect =
      nsLayoutUtils::RectToGfxRect(shadowPaintRect, twipsPerPixel);
    shadowPaintGfxRect.RoundOut();
    gfxRect shadowClipGfxRect =
      nsLayoutUtils::RectToGfxRect(shadowClipRect, twipsPerPixel);
    shadowClipGfxRect.Round();
    shadowContext->NewPath();
    shadowContext->Rectangle(shadowPaintGfxRect);
    if (hasBorderRadius)
      shadowContext->RoundedRectangle(shadowClipGfxRect, clipRectRadii, PR_FALSE);
    else
      shadowContext->Rectangle(shadowClipGfxRect);
    shadowContext->SetFillRule(gfxContext::FILL_RULE_EVEN_ODD);
    shadowContext->Fill();

    blurringArea.DoPaint();
    renderContext->Restore();
  }
}

void
nsCSSRendering::PaintBackground(nsPresContext* aPresContext,
                                nsRenderingContext& aRenderingContext,
                                nsIFrame* aForFrame,
                                const nsRect& aDirtyRect,
                                const nsRect& aBorderArea,
                                PRUint32 aFlags,
                                nsRect* aBGClipRect)
{
  NS_PRECONDITION(aForFrame,
                  "Frame is expected to be provided to PaintBackground");

  nsStyleContext *sc;
  if (!FindBackground(aPresContext, aForFrame, &sc)) {
    
    
    
    
    
    if (!aForFrame->GetStyleDisplay()->mAppearance) {
      return;
    }

    nsIContent* content = aForFrame->GetContent();
    if (!content || content->GetParent()) {
      return;
    }

    sc = aForFrame->GetStyleContext();
  }

  PaintBackgroundWithSC(aPresContext, aRenderingContext, aForFrame,
                        aDirtyRect, aBorderArea, sc,
                        *aForFrame->GetStyleBorder(), aFlags,
                        aBGClipRect);
}

static PRBool
IsOpaqueBorderEdge(const nsStyleBorder& aBorder, mozilla::css::Side aSide)
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
  NS_FOR_CSS_SIDES(i) {
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

  
  *aDirtyRectGfx = nsLayoutUtils::RectToGfxRect(*aDirtyRect, aAppUnitsPerPixel);
  NS_WARN_IF_FALSE(aDirtyRect->IsEmpty() || !aDirtyRectGfx->IsEmpty(),
                   "converted dirty rect should not be empty");
  NS_ABORT_IF_FALSE(!aDirtyRect->IsEmpty() || aDirtyRectGfx->IsEmpty(),
                    "second should be empty if first is");
}

struct BackgroundClipState {
  nsRect mBGClipArea;
  nsRect mDirtyRect;
  gfxRect mDirtyRectGfx;

  gfxCornerSizes mClippedRadii;
  PRPackedBool mRadiiAreOuter;

  
  
  PRPackedBool mCustomClip;
};

static void
GetBackgroundClip(gfxContext *aCtx, PRUint8 aBackgroundClip,
                  nsIFrame* aForFrame, const nsRect& aBorderArea,
                  const nsRect& aCallerDirtyRect, PRBool aHaveRoundedCorners,
                  const gfxCornerSizes& aBGRadii, nscoord aAppUnitsPerPixel,
                   BackgroundClipState* aClipState)
{
  aClipState->mBGClipArea = aBorderArea;
  aClipState->mCustomClip = PR_FALSE;
  aClipState->mRadiiAreOuter = PR_TRUE;
  aClipState->mClippedRadii = aBGRadii;
  if (aBackgroundClip != NS_STYLE_BG_CLIP_BORDER) {
    nsMargin border = aForFrame->GetUsedBorder();
    if (aBackgroundClip != NS_STYLE_BG_CLIP_PADDING) {
      NS_ASSERTION(aBackgroundClip == NS_STYLE_BG_CLIP_CONTENT,
                   "unexpected background-clip");
      border += aForFrame->GetUsedPadding();
    }
    aForFrame->ApplySkipSides(border);
    aClipState->mBGClipArea.Deflate(border);

    if (aHaveRoundedCorners) {
      gfxFloat borderSizes[4] = {
        gfxFloat(border.top / aAppUnitsPerPixel),
        gfxFloat(border.right / aAppUnitsPerPixel),
        gfxFloat(border.bottom / aAppUnitsPerPixel),
        gfxFloat(border.left / aAppUnitsPerPixel)
      };
      nsCSSBorderRenderer::ComputeInnerRadii(aBGRadii, borderSizes,
                                             &aClipState->mClippedRadii);
      aClipState->mRadiiAreOuter = PR_FALSE;
    }
  }

  SetupDirtyRects(aClipState->mBGClipArea, aCallerDirtyRect, aAppUnitsPerPixel,
                  &aClipState->mDirtyRect, &aClipState->mDirtyRectGfx);
}

static void
SetupBackgroundClip(BackgroundClipState& aClipState, gfxContext *aCtx,
                    PRBool aHaveRoundedCorners, nscoord aAppUnitsPerPixel,
                    gfxContextAutoSaveRestore* aAutoSR)
{
  if (aClipState.mDirtyRectGfx.IsEmpty()) {
    
    
    return;
  }

  if (aClipState.mCustomClip) {
    
    
    return;
  }

  
  
  
  
  
  

  if (aHaveRoundedCorners) {
    gfxRect bgAreaGfx =
      nsLayoutUtils::RectToGfxRect(aClipState.mBGClipArea, aAppUnitsPerPixel);
    bgAreaGfx.Round();
    bgAreaGfx.Condition();

    if (bgAreaGfx.IsEmpty()) {
      
      
      NS_WARNING("converted background area should not be empty");
      
      aClipState.mDirtyRectGfx.SizeTo(gfxSize(0.0, 0.0));
      return;
    }

    aAutoSR->Reset(aCtx);
    aCtx->NewPath();
    aCtx->RoundedRectangle(bgAreaGfx, aClipState.mClippedRadii, aClipState.mRadiiAreOuter);
    aCtx->Clip();
  }
}

static void
DrawBackgroundColor(BackgroundClipState& aClipState, gfxContext *aCtx,
                    PRBool aHaveRoundedCorners, nscoord aAppUnitsPerPixel)
{
  if (aClipState.mDirtyRectGfx.IsEmpty()) {
    
    
    return;
  }

  
  
  if (!aHaveRoundedCorners || aClipState.mCustomClip) {
    aCtx->NewPath();
    aCtx->Rectangle(aClipState.mDirtyRectGfx, PR_TRUE);
    aCtx->Fill();
    return;
  }

  gfxRect bgAreaGfx =
    nsLayoutUtils::RectToGfxRect(aClipState.mBGClipArea, aAppUnitsPerPixel);
  bgAreaGfx.Round();
  bgAreaGfx.Condition();

  if (bgAreaGfx.IsEmpty()) {
    
    
    NS_WARNING("converted background area should not be empty");
    
    aClipState.mDirtyRectGfx.SizeTo(gfxSize(0.0, 0.0));
    return;
  }

  aCtx->Save();
  gfxRect dirty = bgAreaGfx.Intersect(aClipState.mDirtyRectGfx);

  aCtx->NewPath();
  aCtx->Rectangle(dirty, PR_TRUE);
  aCtx->Clip();

  aCtx->NewPath();
  aCtx->RoundedRectangle(bgAreaGfx, aClipState.mClippedRadii,
                         aClipState.mRadiiAreOuter);
  aCtx->Fill();
  aCtx->Restore();
}

static nscolor
DetermineBackgroundColorInternal(nsPresContext* aPresContext,
                                 nsStyleContext* aStyleContext,
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
    bgColor =
      aStyleContext->GetVisitedDependentColor(eCSSProperty_background_color);
    if (NS_GET_A(bgColor) == 0)
      aDrawBackgroundColor = PR_FALSE;
  } else {
    
    
    
    
    bgColor = NS_RGB(255, 255, 255);
    if (aDrawBackgroundImage ||
        !aStyleContext->GetStyleBackground()->IsTransparent())
      aDrawBackgroundColor = PR_TRUE;
    else
      bgColor = NS_RGBA(0,0,0,0);
  }

  return bgColor;
}

nscolor
nsCSSRendering::DetermineBackgroundColor(nsPresContext* aPresContext,
                                         nsStyleContext* aStyleContext,
                                         nsIFrame* aFrame)
{
  PRBool drawBackgroundImage;
  PRBool drawBackgroundColor;
  return DetermineBackgroundColorInternal(aPresContext,
                                          aStyleContext,
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
    case eStyleUnit_Calc: {
      const nsStyleCoord::Calc *calc = aCoord.GetCalcValue();
      return calc->mPercent * aFillLength +
             NSAppUnitsToFloatPixels(calc->mLength, aAppUnitsPerPixel);
    }
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

static nscoord
FindTileStart(nscoord aDirtyCoord, nscoord aTilePos, nscoord aTileDim)
{
  NS_ASSERTION(aTileDim > 0, "Non-positive tile dimension");
  double multiples = NS_floor(double(aDirtyCoord - aTilePos)/aTileDim);
  return NSToCoordRound(multiples*aTileDim + aTilePos);
}

void
nsCSSRendering::PaintGradient(nsPresContext* aPresContext,
                              nsRenderingContext& aRenderingContext,
                              nsStyleGradient* aGradient,
                              const nsRect& aDirtyRect,
                              const nsRect& aOneCellArea,
                              const nsRect& aFillArea)
{
  if (aOneCellArea.IsEmpty())
    return;

  gfxContext *ctx = aRenderingContext.ThebesContext();
  nscoord appUnitsPerPixel = aPresContext->AppUnitsPerDevPixel();
  gfxRect oneCellArea =
    nsLayoutUtils::RectToGfxRect(aOneCellArea, appUnitsPerPixel);

  
  gfxPoint lineStart, lineEnd;
  double radiusX = 0, radiusY = 0; 
  if (aGradient->mShape == NS_STYLE_GRADIENT_SHAPE_LINEAR) {
    ComputeLinearGradientLine(aPresContext, aGradient, oneCellArea.Size(),
                              &lineStart, &lineEnd);
  } else {
    ComputeRadialGradientLine(aPresContext, aGradient, oneCellArea.Size(),
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
  PRBool zeroRadius = aGradient->mShape != NS_STYLE_GRADIENT_SHAPE_LINEAR &&
                      (radiusX < 1e-6 || radiusY < 1e-6);
  if (stopDelta < 1e-6 || lineLength < 1e-6 || zeroRadius) {
    
    
    
    
    stopScale = 0.0;
    if (aGradient->mRepeating || zeroRadius) {
      radiusX = radiusY = 0.0;
    }
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
    if (stopScale == 0.0) {
      
      
      outerRadius = innerRadius + 1;
    }
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
    
    
    
    
    if (!aGradient->mRepeating && !zeroRadius) {
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
  if (!dirty.IntersectRect(aDirtyRect, aFillArea))
    return;

  gfxRect areaToFill =
    nsLayoutUtils::RectToGfxRect(aFillArea, appUnitsPerPixel);
  gfxMatrix ctm = ctx->CurrentMatrix();

  
  nscoord xStart = FindTileStart(dirty.x, aOneCellArea.x, aOneCellArea.width);
  nscoord yStart = FindTileStart(dirty.y, aOneCellArea.y, aOneCellArea.height);
  nscoord xEnd = dirty.XMost();
  nscoord yEnd = dirty.YMost();
  
  for (nscoord y = yStart; y < yEnd; y += aOneCellArea.height) {
    for (nscoord x = xStart; x < xEnd; x += aOneCellArea.width) {
      
      gfxRect tileRect = nsLayoutUtils::RectToGfxRect(
                      nsRect(x, y, aOneCellArea.width, aOneCellArea.height),
                      appUnitsPerPixel);
      
      
      gfxRect fillRect = tileRect.Intersect(areaToFill);
      ctx->NewPath();
      ctx->Translate(tileRect.TopLeft());
      ctx->SetPattern(gradientPattern);
      ctx->Rectangle(fillRect - tileRect.TopLeft(), PR_TRUE);
      ctx->Fill();
      ctx->SetMatrix(ctm);
    }
  }
}






struct BackgroundLayerState {
  


  BackgroundLayerState(nsIFrame* aForFrame, const nsStyleImage* aImage, PRUint32 aFlags)
    : mImageRenderer(aForFrame, aImage, aFlags) {}

  


  ImageRenderer mImageRenderer;
  




  nsRect mDestArea;
  




  nsRect mFillArea;
  




  nsPoint mAnchor;
};

static BackgroundLayerState
PrepareBackgroundLayer(nsPresContext* aPresContext,
                       nsIFrame* aForFrame,
                       PRUint32 aFlags,
                       const nsRect& aBorderArea,
                       const nsRect& aBGClipRect,
                       const nsStyleBackground& aBackground,
                       const nsStyleBackground::Layer& aLayer);

void
nsCSSRendering::PaintBackgroundWithSC(nsPresContext* aPresContext,
                                      nsRenderingContext& aRenderingContext,
                                      nsIFrame* aForFrame,
                                      const nsRect& aDirtyRect,
                                      const nsRect& aBorderArea,
                                      nsStyleContext* aBackgroundSC,
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
      nsRect drawing(aBorderArea);
      theme->GetWidgetOverflow(aPresContext->DeviceContext(),
                               aForFrame, displayData->mAppearance, &drawing);
      drawing.IntersectRect(drawing, aDirtyRect);
      theme->DrawWidgetBackground(&aRenderingContext, aForFrame,
                                  displayData->mAppearance, aBorderArea,
                                  drawing);
      return;
    }
  }

  
  
  
  
  
  
  
  PRBool isCanvasFrame = IsCanvasFrame(aForFrame);

  
  
  PRBool drawBackgroundImage;
  PRBool drawBackgroundColor;

  nscolor bgColor = DetermineBackgroundColorInternal(aPresContext,
                                                     aBackgroundSC,
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
    nsSize frameSize = aForFrame->GetSize();
    if (&aBorder == aForFrame->GetStyleBorder() &&
        frameSize == aBorderArea.Size()) {
      haveRoundedCorners = aForFrame->GetBorderRadii(radii);
    } else {
      haveRoundedCorners = nsIFrame::ComputeBorderRadii(aBorder.mBorderRadius,
                                   frameSize, aBorderArea.Size(),
                                   aForFrame->GetSkipSides(), radii);
    }
    if (haveRoundedCorners)
      ComputePixelRadii(radii, appUnitsPerPixel, &bgRadii);
  }

  
  
  
  
  
  
  const nsStyleBackground *bg = aBackgroundSC->GetStyleBackground();
  BackgroundClipState clipState;
  PRUint8 currentBackgroundClip;
  PRBool isSolidBorder;
  if (aBGClipRect) {
    clipState.mBGClipArea = *aBGClipRect;
    clipState.mCustomClip = PR_TRUE;
    SetupDirtyRects(clipState.mBGClipArea, aDirtyRect, appUnitsPerPixel,
                    &clipState.mDirtyRect, &clipState.mDirtyRectGfx);
  } else {
    
    
    
    
    
    
    
    
    currentBackgroundClip = bg->BottomLayer().mClip;
    isSolidBorder =
      (aFlags & PAINTBG_WILL_PAINT_BORDER) && IsOpaqueBorder(aBorder);
    if (isSolidBorder && currentBackgroundClip == NS_STYLE_BG_CLIP_BORDER)
      currentBackgroundClip = NS_STYLE_BG_CLIP_PADDING;

    GetBackgroundClip(ctx, currentBackgroundClip, aForFrame, aBorderArea,
                      aDirtyRect, haveRoundedCorners, bgRadii, appUnitsPerPixel,
                      &clipState);
  }

  
  if (drawBackgroundColor && !isCanvasFrame)
    ctx->SetColor(gfxRGBA(bgColor));

  gfxContextAutoSaveRestore autoSR;

  
  
  
  if (!drawBackgroundImage) {
    if (!isCanvasFrame) {
      DrawBackgroundColor(clipState, ctx, haveRoundedCorners, appUnitsPerPixel);
    }
    return;
  }

  
  
  
  aPresContext->SetupBackgroundImageLoaders(aForFrame, bg);

  
  if (drawBackgroundColor &&
      bg->BottomLayer().mRepeat == NS_STYLE_BG_REPEAT_XY &&
      bg->BottomLayer().mImage.IsOpaque())
    drawBackgroundColor = PR_FALSE;

  
  
  if (drawBackgroundColor && !isCanvasFrame) {
    DrawBackgroundColor(clipState, ctx, haveRoundedCorners, appUnitsPerPixel);
  }

  if (drawBackgroundImage) {
    bool clipSet = false;
    NS_FOR_VISIBLE_BACKGROUND_LAYERS_BACK_TO_FRONT(i, bg) {
      const nsStyleBackground::Layer &layer = bg->mLayers[i];
      if (!aBGClipRect) {
        PRUint8 newBackgroundClip = layer.mClip;
        if (isSolidBorder && newBackgroundClip == NS_STYLE_BG_CLIP_BORDER)
          newBackgroundClip = NS_STYLE_BG_CLIP_PADDING;
        if (currentBackgroundClip != newBackgroundClip || !clipSet) {
          currentBackgroundClip = newBackgroundClip;
          
          
          
          if (clipSet) {
            GetBackgroundClip(ctx, currentBackgroundClip, aForFrame,
                              aBorderArea, aDirtyRect, haveRoundedCorners,
                              bgRadii, appUnitsPerPixel, &clipState);
          }
          SetupBackgroundClip(clipState, ctx, haveRoundedCorners,
                              appUnitsPerPixel, &autoSR);
          clipSet = true;
        }
      }
      if (!clipState.mDirtyRectGfx.IsEmpty()) {
        BackgroundLayerState state = PrepareBackgroundLayer(aPresContext, aForFrame,
            aFlags, aBorderArea, clipState.mBGClipArea, *bg, layer);
        if (!state.mFillArea.IsEmpty()) {
          state.mImageRenderer.Draw(aPresContext, aRenderingContext,
                                    state.mDestArea, state.mFillArea,
                                    state.mAnchor + aBorderArea.TopLeft(),
                                    clipState.mDirtyRect);
        }
      }
    }
  }
}

static inline float
ScaleDimension(const nsStyleBackground::Size::Dimension& aDimension,
               PRUint8 aType,
               nscoord aLength, nscoord aAvailLength)
{
  switch (aType) {
    case nsStyleBackground::Size::eLengthPercentage:
      
      return NS_MAX(double(aDimension.mPercent) * double(aAvailLength) +
                      double(aDimension.mLength),
                    0.0) /
             double(aLength);
    default:
      NS_ABORT_IF_FALSE(PR_FALSE, "bad aDimension.mType");
      return 1.0f;
    case nsStyleBackground::Size::eAuto:
      NS_ABORT_IF_FALSE(PR_FALSE, "aDimension.mType == eAuto isn't handled");
      return 1.0f;
  }
}

static inline PRBool
IsTransformed(nsIFrame* aForFrame, nsIFrame* aTopFrame)
{
  for (nsIFrame* f = aForFrame; f != aTopFrame; f = f->GetParent()) {
    if (f->IsTransformed()) {
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}

static BackgroundLayerState
PrepareBackgroundLayer(nsPresContext* aPresContext,
                       nsIFrame* aForFrame,
                       PRUint32 aFlags,
                       const nsRect& aBorderArea,
                       const nsRect& aBGClipRect,
                       const nsStyleBackground& aBackground,
                       const nsStyleBackground::Layer& aLayer)
{
  






















































  PRUint32 irFlags = 0;
  if (aFlags & nsCSSRendering::PAINTBG_SYNC_DECODE_IMAGES) {
    irFlags |= ImageRenderer::FLAG_SYNC_DECODE_IMAGES;
  }

  BackgroundLayerState state(aForFrame, &aLayer.mImage, irFlags);
  if (!state.mImageRenderer.PrepareImage()) {
    
    return state;
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
    if (aLayer.mOrigin != NS_STYLE_BG_ORIGIN_PADDING) {
      border += geometryFrame->GetUsedPadding();
      NS_ASSERTION(aLayer.mOrigin == NS_STYLE_BG_ORIGIN_CONTENT,
                   "unknown background-origin value");
    }
    geometryFrame->ApplySkipSides(border);
    bgPositioningArea.Deflate(border);
  }

  
  
  nsRect bgClipRect = aBGClipRect;

  
  
  
  
  nsPoint imageTopLeft;
  if (NS_STYLE_BG_ATTACHMENT_FIXED == aLayer.mAttachment) {
    aPresContext->SetHasFixedBackgroundFrame();

    
    
    
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

    
    
    bgPositioningArea = nsRect(-aForFrame->GetOffsetTo(topFrame), topFrame->GetSize());

    if (!pageContentFrame) {
      
      nsIScrollableFrame* scrollableFrame =
        aPresContext->PresShell()->GetRootScrollFrameAsScrollable();
      if (scrollableFrame) {
        nsMargin scrollbars = scrollableFrame->GetActualScrollbarSizes();
        bgPositioningArea.Deflate(scrollbars);
      }
    }

    if (aFlags & nsCSSRendering::PAINTBG_TO_WINDOW &&
        !IsTransformed(aForFrame, topFrame)) {
      
      
      
      
      
      
      
      bgClipRect.IntersectRect(bgClipRect, bgPositioningArea + aBorderArea.TopLeft());
    }
  }

  nsSize imageSize = state.mImageRenderer.ComputeSize(bgPositioningArea.Size());
  if (imageSize.width <= 0 || imageSize.height <= 0)
    return state;

  
  
  
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
                               &imageTopLeft, &state.mAnchor);
  imageTopLeft += bgPositioningArea.TopLeft();
  state.mAnchor += bgPositioningArea.TopLeft();

  state.mDestArea = nsRect(imageTopLeft + aBorderArea.TopLeft(), imageSize);
  state.mFillArea = state.mDestArea;
  PRIntn repeat = aLayer.mRepeat;
  PR_STATIC_ASSERT(NS_STYLE_BG_REPEAT_XY ==
                   (NS_STYLE_BG_REPEAT_X | NS_STYLE_BG_REPEAT_Y));
  if (repeat & NS_STYLE_BG_REPEAT_X) {
    state.mFillArea.x = bgClipRect.x;
    state.mFillArea.width = bgClipRect.width;
  }
  if (repeat & NS_STYLE_BG_REPEAT_Y) {
    state.mFillArea.y = bgClipRect.y;
    state.mFillArea.height = bgClipRect.height;
  }
  state.mFillArea.IntersectRect(state.mFillArea, bgClipRect);
  return state;
}

nsRect
nsCSSRendering::GetBackgroundLayerRect(nsPresContext* aPresContext,
                                       nsIFrame* aForFrame,
                                       const nsRect& aBorderArea,
                                       const nsStyleBackground& aBackground,
                                       const nsStyleBackground::Layer& aLayer)
{
  BackgroundLayerState state =
      PrepareBackgroundLayer(aPresContext, aForFrame, 0, aBorderArea,
                             aBorderArea, aBackground, aLayer);
  return state.mFillArea;
}

static void
DrawBorderImage(nsPresContext*       aPresContext,
                nsRenderingContext& aRenderingContext,
                nsIFrame*            aForFrame,
                const nsRect&        aBorderArea,
                const nsStyleBorder& aStyleBorder,
                const nsRect&        aDirtyRect)
{
  NS_PRECONDITION(aStyleBorder.IsBorderImageLoaded(),
                  "drawing border image that isn't successfully loaded");

  if (aDirtyRect.IsEmpty())
    return;

  
  
  
  
  
  
  aPresContext->SetupBorderImageLoaders(aForFrame, &aStyleBorder);

  imgIRequest *req = aStyleBorder.GetBorderImage();

  
  
  

  nsCOMPtr<imgIContainer> imgContainer;
  req->GetImage(getter_AddRefs(imgContainer));
  NS_ASSERTION(imgContainer, "no image to draw");

  nsIntSize imageSize;
  if (NS_FAILED(imgContainer->GetWidth(&imageSize.width))) {
    imageSize.width =
      nsPresContext::AppUnitsToIntCSSPixels(aBorderArea.width);
  }
  if (NS_FAILED(imgContainer->GetHeight(&imageSize.height))) {
    imageSize.height =
      nsPresContext::AppUnitsToIntCSSPixels(aBorderArea.height);
  }

  
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
DrawBorderImageComponent(nsRenderingContext& aRenderingContext,
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
DrawSolidBorderSegment(nsRenderingContext& aContext,
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
        aContext.DrawLine(aRect.TopLeft(), aRect.BottomLeft());
      else
        aContext.FillRect(aRect);
    }
    else {
      if (1 == aRect.width)
        aContext.DrawLine(aRect.TopLeft(), aRect.TopRight());
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
nsCSSRendering::DrawTableBorderSegment(nsRenderingContext&     aContext,
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
      mozilla::css::Side ridgeGrooveSide = (horizontal) ? NS_SIDE_TOP : NS_SIDE_LEFT;
      
      
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
  NS_ASSERTION(aStyle != NS_STYLE_TEXT_DECORATION_STYLE_NONE, "aStyle is none");

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
    case NS_STYLE_TEXT_DECORATION_STYLE_SOLID:
    case NS_STYLE_TEXT_DECORATION_STYLE_DOUBLE:
      oldLineWidth = aGfxContext->CurrentLineWidth();
      oldPattern = aGfxContext->GetPattern();
      break;
    case NS_STYLE_TEXT_DECORATION_STYLE_DASHED: {
      aGfxContext->Save();
      contextIsSaved = PR_TRUE;
      aGfxContext->Clip(rect);
      gfxFloat dashWidth = lineHeight * DOT_LENGTH * DASH_LENGTH;
      gfxFloat dash[2] = { dashWidth, dashWidth };
      aGfxContext->SetLineCap(gfxContext::LINE_CAP_BUTT);
      aGfxContext->SetDash(dash, 2, 0.0);
      
      rect.width += dashWidth;
      break;
    }
    case NS_STYLE_TEXT_DECORATION_STYLE_DOTTED: {
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
      
      rect.width += dashWidth;
      break;
    }
    case NS_STYLE_TEXT_DECORATION_STYLE_WAVY:
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

  
  rect.y += lineHeight / 2;

  aGfxContext->SetColor(gfxRGBA(aColor));
  aGfxContext->SetLineWidth(lineHeight);
  switch (aStyle) {
    case NS_STYLE_TEXT_DECORATION_STYLE_SOLID:
      aGfxContext->NewPath();
      aGfxContext->MoveTo(rect.TopLeft());
      aGfxContext->LineTo(rect.TopRight());
      aGfxContext->Stroke();
      break;
    case NS_STYLE_TEXT_DECORATION_STYLE_DOUBLE:
      













      aGfxContext->NewPath();
      aGfxContext->MoveTo(rect.TopLeft());
      aGfxContext->LineTo(rect.TopRight());
      rect.height -= lineHeight;
      aGfxContext->MoveTo(rect.BottomLeft());
      aGfxContext->LineTo(rect.BottomRight());
      aGfxContext->Stroke();
      break;
    case NS_STYLE_TEXT_DECORATION_STYLE_DOTTED:
    case NS_STYLE_TEXT_DECORATION_STYLE_DASHED:
      aGfxContext->NewPath();
      aGfxContext->MoveTo(rect.TopLeft());
      aGfxContext->LineTo(rect.TopRight());
      aGfxContext->Stroke();
      break;
    case NS_STYLE_TEXT_DECORATION_STYLE_WAVY: {
      




























      rect.x += lineHeight / 2.0;
      aGfxContext->NewPath();

      gfxPoint pt(rect.TopLeft());
      gfxFloat rightMost = pt.x + rect.Width() + lineHeight;
      gfxFloat adv = rect.Height() - lineHeight;
      gfxFloat flatLengthAtVertex = NS_MAX((lineHeight - 1.0) * 2.0, 1.0);

      pt.x -= lineHeight;
      aGfxContext->MoveTo(pt); 

      pt.x = rect.X();
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
  NS_ASSERTION(aStyle != NS_STYLE_TEXT_DECORATION_STYLE_NONE, "aStyle is none");

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
  NS_ASSERTION(aStyle <= NS_STYLE_TEXT_DECORATION_STYLE_WAVY,
               "Invalid aStyle value");

  if (aStyle == NS_STYLE_TEXT_DECORATION_STYLE_NONE)
    return gfxRect(0, 0, 0, 0);

  PRBool canLiftUnderline = aDescentLimit >= 0.0;

  gfxRect r(NS_floor(aPt.x + 0.5), 0, NS_round(aLineSize.width), 0);

  gfxFloat lineHeight = NS_round(aLineSize.height);
  lineHeight = NS_MAX(lineHeight, 1.0);

  gfxFloat ascent = NS_round(aAscent);
  gfxFloat descentLimit = NS_floor(aDescentLimit);

  gfxFloat suggestedMaxRectHeight = NS_MAX(NS_MIN(ascent, descentLimit), 1.0);
  r.height = lineHeight;
  if (aStyle == NS_STYLE_TEXT_DECORATION_STYLE_DOUBLE) {
    














    gfxFloat gap = NS_round(lineHeight / 2.0);
    gap = NS_MAX(gap, 1.0);
    r.height = lineHeight * 2.0 + gap;
    if (canLiftUnderline) {
      if (r.Height() > suggestedMaxRectHeight) {
        
        
        r.height = NS_MAX(suggestedMaxRectHeight, lineHeight * 2.0 + 1.0);
      }
    }
  } else if (aStyle == NS_STYLE_TEXT_DECORATION_STYLE_WAVY) {
    












    r.height = lineHeight > 2.0 ? lineHeight * 4.0 : lineHeight * 3.0;
    if (canLiftUnderline) {
      if (r.Height() > suggestedMaxRectHeight) {
        
        
        
        
        r.height = NS_MAX(suggestedMaxRectHeight, lineHeight * 2.0);
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
  r.y = baseline - NS_floor(offset + 0.5);
  return r;
}




ImageRenderer::ImageRenderer(nsIFrame* aForFrame,
                             const nsStyleImage* aImage,
                             PRUint32 aFlags)
  : mForFrame(aForFrame)
  , mImage(aImage)
  , mType(aImage->GetType())
  , mImageContainer(nsnull)
  , mGradientData(nsnull)
#ifdef MOZ_SVG
  , mPaintServerFrame(nsnull)
#endif
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
  if (mImage->IsEmpty() || !mImage->IsComplete()) {
    
    mImage->RequestDecode();

    
    
    
    
    nsCOMPtr<imgIContainer> img;
    if (!((mFlags & FLAG_SYNC_DECODE_IMAGES) &&
          (mType == eStyleImageType_Image) &&
          (NS_SUCCEEDED(mImage->GetImageData()->GetImage(getter_AddRefs(img))) && img)))
      return PR_FALSE;
  }

  switch (mType) {
    case eStyleImageType_Image:
    {
      nsCOMPtr<imgIContainer> srcImage;
      mImage->GetImageData()->GetImage(getter_AddRefs(srcImage));
      NS_ABORT_IF_FALSE(srcImage, "If srcImage is null, mImage->IsComplete() "
                                  "should have returned false");

      if (!mImage->GetCropRect()) {
        mImageContainer.swap(srcImage);
      } else {
        nsIntRect actualCropRect;
        PRBool isEntireImage;
        PRBool success =
          mImage->ComputeActualCropRect(actualCropRect, &isEntireImage);
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
      mGradientData = mImage->GetGradientData();
      mIsReady = PR_TRUE;
      break;
#ifdef MOZ_SVG
    case eStyleImageType_Element:
    {
      nsAutoString elementId =
        NS_LITERAL_STRING("#") + nsDependentString(mImage->GetElementId());
      nsCOMPtr<nsIURI> targetURI;
      nsCOMPtr<nsIURI> base = mForFrame->GetContent()->GetBaseURI();
      nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(targetURI), elementId,
                                                mForFrame->GetContent()->GetCurrentDoc(), base);
      nsSVGPaintingProperty* property = nsSVGEffects::GetPaintingPropertyForURI(
          targetURI, mForFrame->GetFirstContinuation(),
          nsSVGEffects::BackgroundImageProperty());
      if (!property)
        return PR_FALSE;
      mPaintServerFrame = property->GetReferencedFrame();

      
      
      if (!mPaintServerFrame) {
        nsCOMPtr<nsIDOMElement> imageElement =
          do_QueryInterface(property->GetReferencedElement());
        mImageElementSurface = nsLayoutUtils::SurfaceFromElement(imageElement);
        if (!mImageElementSurface.mSurface)
          return PR_FALSE;
      }
      mIsReady = PR_TRUE;
      break;
    }
#endif
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
      PRBool gotHeight, gotWidth;
      nsLayoutUtils::ComputeSizeForDrawing(mImageContainer, imageIntSize,
                                           gotWidth, gotHeight);

      mSize.width = gotWidth ?
        nsPresContext::CSSPixelsToAppUnits(imageIntSize.width) :
        aDefault.width;

      mSize.height = gotHeight ?
        nsPresContext::CSSPixelsToAppUnits(imageIntSize.height) :
        aDefault.height;

      break;
    }
    case eStyleImageType_Gradient:
      mSize = aDefault;
      break;
#ifdef MOZ_SVG
    case eStyleImageType_Element:
    {
      if (mPaintServerFrame) {
        if (mPaintServerFrame->IsFrameOfType(nsIFrame::eSVG)) {
          mSize = aDefault;
        } else {
          
          
          PRInt32 appUnitsPerDevPixel =
            mForFrame->PresContext()->AppUnitsPerDevPixel();
          nsRect rect =
            nsSVGIntegrationUtils::GetNonSVGUserSpace(mPaintServerFrame);
          nsRect size = rect - rect.TopLeft();
          nsIntRect rounded = size.ToNearestPixels(appUnitsPerDevPixel);
          mSize = rounded.ToAppUnits(appUnitsPerDevPixel).Size();
        }
      } else {
        NS_ASSERTION(mImageElementSurface.mSurface, "Surface should be ready.");
        gfxIntSize size = mImageElementSurface.mSize;
        mSize.width = nsPresContext::CSSPixelsToAppUnits(size.width);
        mSize.height = nsPresContext::CSSPixelsToAppUnits(size.height);
      }
      break;
    }
#endif
    case eStyleImageType_Null:
    default:
      mSize.SizeTo(0, 0);
      break;
  }

  return mSize;
}

void
ImageRenderer::Draw(nsPresContext*       aPresContext,
                         nsRenderingContext& aRenderingContext,
                         const nsRect&        aDest,
                         const nsRect&        aFill,
                         const nsPoint&       aAnchor,
                         const nsRect&        aDirty)
{
  if (!mIsReady) {
    NS_NOTREACHED("Ensure PrepareImage() has returned true before calling me");
    return;
  }

  if (aDest.IsEmpty() || aFill.IsEmpty() ||
      mSize.width <= 0 || mSize.height <= 0)
    return;

  gfxPattern::GraphicsFilter graphicsFilter =
    nsLayoutUtils::GetGraphicsFilterForFrame(mForFrame);

  switch (mType) {
    case eStyleImageType_Image:
    {
      PRUint32 drawFlags = (mFlags & FLAG_SYNC_DECODE_IMAGES)
                             ? (PRUint32) imgIContainer::FLAG_SYNC_DECODE
                             : (PRUint32) imgIContainer::FLAG_NONE;
      nsLayoutUtils::DrawImage(&aRenderingContext, mImageContainer,
          graphicsFilter,
          aDest, aFill, aAnchor, aDirty, drawFlags);
      break;
    }
    case eStyleImageType_Gradient:
      nsCSSRendering::PaintGradient(aPresContext, aRenderingContext,
          mGradientData, aDirty, aDest, aFill);
      break;
#ifdef MOZ_SVG
    case eStyleImageType_Element:
      if (mPaintServerFrame) {
        nsSVGIntegrationUtils::DrawPaintServer(
            &aRenderingContext, mForFrame, mPaintServerFrame, graphicsFilter,
            aDest, aFill, aAnchor, aDirty, mSize);
      } else {
        NS_ASSERTION(mImageElementSurface.mSurface, "Surface should be ready.");
        nsRefPtr<gfxDrawable> surfaceDrawable =
          new gfxSurfaceDrawable(mImageElementSurface.mSurface,
                                 mImageElementSurface.mSize);
        nsLayoutUtils::DrawPixelSnapped(
            &aRenderingContext, surfaceDrawable, graphicsFilter,
            aDest, aFill, aAnchor, aDirty);
      }
      break;
#endif
    case eStyleImageType_Null:
    default:
      break;
  }
}

#define MAX_BLUR_RADIUS 300
#define MAX_SPREAD_RADIUS 50

static inline gfxIntSize
ComputeBlurRadius(nscoord aBlurRadius, PRInt32 aAppUnitsPerDevPixel)
{
  
  
  gfxFloat blurStdDev =
    NS_MIN(gfxFloat(aBlurRadius) / gfxFloat(aAppUnitsPerDevPixel),
           gfxFloat(MAX_BLUR_RADIUS))
    / 2.0;
  return
    gfxAlphaBoxBlur::CalculateBlurRadius(gfxPoint(blurStdDev, blurStdDev));
}




gfxContext*
nsContextBoxBlur::Init(const nsRect& aRect, nscoord aSpreadRadius,
                       nscoord aBlurRadius,
                       PRInt32 aAppUnitsPerDevPixel,
                       gfxContext* aDestinationCtx,
                       const nsRect& aDirtyRect,
                       const gfxRect* aSkipRect,
                       PRUint32 aFlags)
{
  if (aRect.IsEmpty()) {
    mContext = nsnull;
    return nsnull;
  }

  gfxIntSize blurRadius = ComputeBlurRadius(aBlurRadius, aAppUnitsPerDevPixel);
  PRInt32 spreadRadius = NS_MIN(PRInt32(aSpreadRadius / aAppUnitsPerDevPixel),
                                PRInt32(MAX_SPREAD_RADIUS));
  mDestinationCtx = aDestinationCtx;

  
  if (blurRadius.width <= 0 && blurRadius.height <= 0 && spreadRadius <= 0 &&
      !(aFlags & FORCE_MASK)) {
    mContext = aDestinationCtx;
    return mContext;
  }

  
  gfxRect rect = nsLayoutUtils::RectToGfxRect(aRect, aAppUnitsPerDevPixel);

  gfxRect dirtyRect =
    nsLayoutUtils::RectToGfxRect(aDirtyRect, aAppUnitsPerDevPixel);
  dirtyRect.RoundOut();

  
  mContext = blur.Init(rect, gfxIntSize(spreadRadius, spreadRadius),
                       blurRadius, &dirtyRect, aSkipRect);
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

 nsMargin
nsContextBoxBlur::GetBlurRadiusMargin(nscoord aBlurRadius,
                                      PRInt32 aAppUnitsPerDevPixel)
{
  gfxIntSize blurRadius = ComputeBlurRadius(aBlurRadius, aAppUnitsPerDevPixel);

  nsMargin result;
  result.top    = blurRadius.height * aAppUnitsPerDevPixel;
  result.right  = blurRadius.width  * aAppUnitsPerDevPixel;
  result.bottom = blurRadius.height * aAppUnitsPerDevPixel;
  result.left   = blurRadius.width  * aAppUnitsPerDevPixel;
  return result;
}
