







#include <ctime>

#include "gfx2DGlue.h"
#include "mozilla/ArrayUtils.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/gfx/Helpers.h"
#include "mozilla/gfx/PathHelpers.h"
#include "mozilla/HashFunctions.h"
#include "mozilla/MathAlgorithms.h"

#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsIFrame.h"
#include "nsPoint.h"
#include "nsRect.h"
#include "nsIPresShell.h"
#include "nsFrameManager.h"
#include "nsStyleContext.h"
#include "nsGkAtoms.h"
#include "nsCSSAnonBoxes.h"
#include "nsIContent.h"
#include "nsIDocumentInlines.h"
#include "nsIScrollableFrame.h"
#include "imgIRequest.h"
#include "imgIContainer.h"
#include "ImageOps.h"
#include "nsCSSRendering.h"
#include "nsCSSColorUtils.h"
#include "nsITheme.h"
#include "nsThemeConstants.h"
#include "nsLayoutUtils.h"
#include "nsBlockFrame.h"
#include "gfxContext.h"
#include "nsRenderingContext.h"
#include "nsStyleStructInlines.h"
#include "nsCSSFrameConstructor.h"
#include "nsCSSProps.h"
#include "nsContentUtils.h"
#include "nsSVGEffects.h"
#include "nsSVGIntegrationUtils.h"
#include "gfxDrawable.h"
#include "GeckoProfiler.h"
#include "nsCSSRenderingBorders.h"
#include "mozilla/css/ImageLoader.h"
#include "ImageContainer.h"
#include "mozilla/Telemetry.h"
#include "gfxUtils.h"
#include "gfxColor.h"
#include "gfxGradientCache.h"
#include "GraphicsFilter.h"
#include "nsInlineFrame.h"
#include <algorithm>

using namespace mozilla;
using namespace mozilla::css;
using namespace mozilla::gfx;
using namespace mozilla::image;
using mozilla::CSSSizeOrRatio;

static int gFrameTreeLockCount = 0;





struct InlineBackgroundData
{
  InlineBackgroundData()
      : mFrame(nullptr), mBlockFrame(nullptr)
  {
  }

  ~InlineBackgroundData()
  {
  }

  void Reset()
  {
    mBoundingBox.SetRect(0,0,0,0);
    mContinuationPoint = mLineContinuationPoint = mUnbrokenMeasure = 0;
    mFrame = mBlockFrame = nullptr;
    mPIStartBorderData.Reset();
  }

  




  nsRect GetContinuousRect(nsIFrame* aFrame)
  {
    MOZ_ASSERT(static_cast<nsInlineFrame*>(do_QueryFrame(aFrame)));

    SetFrame(aFrame);

    nscoord pos; 
                 
    if (mBidiEnabled) {
      pos = mLineContinuationPoint;

      
      
      
      bool isRtlBlock = (mBlockFrame->StyleVisibility()->mDirection ==
                           NS_STYLE_DIRECTION_RTL);
      nscoord curOffset = mVertical ? aFrame->GetOffsetTo(mBlockFrame).y
                                    : aFrame->GetOffsetTo(mBlockFrame).x;

      
      
      nsIFrame* inlineFrame = aFrame->GetPrevContinuation();
      while (inlineFrame && !inlineFrame->GetNextInFlow() &&
             AreOnSameLine(aFrame, inlineFrame)) {
        nscoord frameOffset = mVertical
          ? inlineFrame->GetOffsetTo(mBlockFrame).y
          : inlineFrame->GetOffsetTo(mBlockFrame).x;
        if (isRtlBlock == (frameOffset >= curOffset)) {
          pos += mVertical
               ? inlineFrame->GetSize().height
               : inlineFrame->GetSize().width;
        }
        inlineFrame = inlineFrame->GetPrevContinuation();
      }

      inlineFrame = aFrame->GetNextContinuation();
      while (inlineFrame && !inlineFrame->GetPrevInFlow() &&
             AreOnSameLine(aFrame, inlineFrame)) {
        nscoord frameOffset = mVertical
          ? inlineFrame->GetOffsetTo(mBlockFrame).y
          : inlineFrame->GetOffsetTo(mBlockFrame).x;
        if (isRtlBlock == (frameOffset >= curOffset)) {
          pos += mVertical
                 ? inlineFrame->GetSize().height
                 : inlineFrame->GetSize().width;
        }
        inlineFrame = inlineFrame->GetNextContinuation();
      }
      if (isRtlBlock) {
        
        pos += mVertical ? aFrame->GetSize().height : aFrame->GetSize().width;
        
        
        
        pos = mUnbrokenMeasure - pos;
      }
    } else {
      pos = mContinuationPoint;
    }

    
    
    
    return mVertical
      ? nsRect(0, -pos, mFrame->GetSize().width, mUnbrokenMeasure)
      : nsRect(-pos, 0, mUnbrokenMeasure, mFrame->GetSize().height);
  }

  







  nsRect GetBorderContinuousRect(nsIFrame* aFrame, nsRect aBorderArea)
  {
    
    
    PhysicalInlineStartBorderData saved(mPIStartBorderData);
    nsRect joinedBorderArea = GetContinuousRect(aFrame);
    if (!saved.mIsValid || saved.mFrame != mPIStartBorderData.mFrame) {
      if (aFrame == mPIStartBorderData.mFrame) {
        if (mVertical) {
          mPIStartBorderData.SetCoord(joinedBorderArea.y);
        } else {
          mPIStartBorderData.SetCoord(joinedBorderArea.x);
        }
      } else if (mPIStartBorderData.mFrame) {
        if (mVertical) {
          mPIStartBorderData.SetCoord(GetContinuousRect(mPIStartBorderData.mFrame).y);
        } else {
          mPIStartBorderData.SetCoord(GetContinuousRect(mPIStartBorderData.mFrame).x);
        }
      }
    } else {
      
      mPIStartBorderData.mCoord = saved.mCoord;
    }
    if (mVertical) {
      if (joinedBorderArea.y > mPIStartBorderData.mCoord) {
        joinedBorderArea.y =
          -(mUnbrokenMeasure + joinedBorderArea.y - aBorderArea.height);
      } else {
        joinedBorderArea.y -= mPIStartBorderData.mCoord;
      }
    } else {
      if (joinedBorderArea.x > mPIStartBorderData.mCoord) {
        joinedBorderArea.x =
          -(mUnbrokenMeasure + joinedBorderArea.x - aBorderArea.width);
      } else {
        joinedBorderArea.x -= mPIStartBorderData.mCoord;
      }
    }
    return joinedBorderArea;
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
  
  
  
  
  struct PhysicalInlineStartBorderData {
    nsIFrame* mFrame;   
    nscoord   mCoord;   
    bool      mIsValid; 
    void Reset() { mFrame = nullptr; mIsValid = false; }
    void SetCoord(nscoord aCoord) { mCoord = aCoord; mIsValid = true; }
  };

  nsIFrame*      mFrame;
  nsBlockFrame*  mBlockFrame;
  nsRect         mBoundingBox;
  nscoord        mContinuationPoint;
  nscoord        mUnbrokenMeasure;
  nscoord        mLineContinuationPoint;
  PhysicalInlineStartBorderData mPIStartBorderData;
  bool           mBidiEnabled;
  bool           mVertical;

  void SetFrame(nsIFrame* aFrame)
  {
    NS_PRECONDITION(aFrame, "Need a frame");
    NS_ASSERTION(gFrameTreeLockCount > 0,
                 "Can't call this when frame tree is not locked");

    if (aFrame == mFrame) {
      return;
    }

    nsIFrame *prevContinuation = GetPrevContinuation(aFrame);

    if (!prevContinuation || mFrame != prevContinuation) {
      
      Reset();
      Init(aFrame);
      return;
    }

    
    
    mContinuationPoint += mVertical ? mFrame->GetSize().height
                                    : mFrame->GetSize().width;

    
    if (mBidiEnabled &&
        (aFrame->GetPrevInFlow() || !AreOnSameLine(mFrame, aFrame))) {
       mLineContinuationPoint = mContinuationPoint;
    }

    mFrame = aFrame;
  }

  nsIFrame* GetPrevContinuation(nsIFrame* aFrame)
  {
    nsIFrame* prevCont = aFrame->GetPrevContinuation();
    if (!prevCont &&
        (aFrame->GetStateBits() & NS_FRAME_PART_OF_IBSPLIT)) {
      nsIFrame* block = static_cast<nsIFrame*>
        (aFrame->Properties().Get(nsIFrame::IBSplitPrevSibling()));
      if (block) {
        
        NS_ASSERTION(!block->GetPrevContinuation(),
                     "Incorrect value for IBSplitPrevSibling");
        prevCont = static_cast<nsIFrame*>
          (block->Properties().Get(nsIFrame::IBSplitPrevSibling()));
        NS_ASSERTION(prevCont, "How did that happen?");
      }
    }
    return prevCont;
  }

  nsIFrame* GetNextContinuation(nsIFrame* aFrame)
  {
    nsIFrame* nextCont = aFrame->GetNextContinuation();
    if (!nextCont &&
        (aFrame->GetStateBits() & NS_FRAME_PART_OF_IBSPLIT)) {
      
      aFrame = aFrame->FirstContinuation();
      nsIFrame* block = static_cast<nsIFrame*>
        (aFrame->Properties().Get(nsIFrame::IBSplitSibling()));
      if (block) {
        nextCont = static_cast<nsIFrame*>
          (block->Properties().Get(nsIFrame::IBSplitSibling()));
        NS_ASSERTION(nextCont, "How did that happen?");
      }
    }
    return nextCont;
  }

  void Init(nsIFrame* aFrame)
  {
    mPIStartBorderData.Reset();
    mBidiEnabled = aFrame->PresContext()->BidiEnabled();
    if (mBidiEnabled) {
      
      nsIFrame* frame = aFrame;
      do {
        frame = frame->GetParent();
        mBlockFrame = do_QueryFrame(frame);
      }
      while (frame && frame->IsFrameOfType(nsIFrame::eLineParticipant));

      NS_ASSERTION(mBlockFrame, "Cannot find containing block.");
    }

    mVertical = aFrame->GetWritingMode().IsVertical();

    
    
    nsIFrame* inlineFrame = GetPrevContinuation(aFrame);
    while (inlineFrame) {
      if (!mPIStartBorderData.mFrame &&
          !(mVertical ? inlineFrame->GetSkipSides().Top()
                      : inlineFrame->GetSkipSides().Left())) {
        mPIStartBorderData.mFrame = inlineFrame;
      }
      nsRect rect = inlineFrame->GetRect();
      mContinuationPoint += mVertical ? rect.height : rect.width;
      if (mBidiEnabled && !AreOnSameLine(aFrame, inlineFrame)) {
        mLineContinuationPoint += mVertical ? rect.height : rect.width;
      }
      mUnbrokenMeasure += mVertical ? rect.height : rect.width;
      mBoundingBox.UnionRect(mBoundingBox, rect);
      inlineFrame = GetPrevContinuation(inlineFrame);
    }

    
    
    inlineFrame = aFrame;
    while (inlineFrame) {
      if (!mPIStartBorderData.mFrame &&
          !(mVertical ? inlineFrame->GetSkipSides().Top()
                      : inlineFrame->GetSkipSides().Left())) {
        mPIStartBorderData.mFrame = inlineFrame;
      }
      nsRect rect = inlineFrame->GetRect();
      mUnbrokenMeasure += mVertical ? rect.height : rect.width;
      mBoundingBox.UnionRect(mBoundingBox, rect);
      inlineFrame = GetNextContinuation(inlineFrame);
    }

    mFrame = aFrame;
  }

  bool AreOnSameLine(nsIFrame* aFrame1, nsIFrame* aFrame2) {
    bool isValid1, isValid2;
    nsBlockInFlowLineIterator it1(mBlockFrame, aFrame1, &isValid1);
    nsBlockInFlowLineIterator it2(mBlockFrame, aFrame2, &isValid2);
    return isValid1 && isValid2 &&
      
      
      it1.GetContainer() == it2.GetContainer() &&
      
      it1.GetLine() == it2.GetLine();
  }
};



struct ColorStop {
  ColorStop(): mPosition(0), mIsMidpoint(false) {}
  ColorStop(double aPosition, bool aIsMidPoint, gfxRGBA aColor) :
    mPosition(aPosition), mIsMidpoint(aIsMidPoint), mColor(aColor) {}
  double mPosition; 
  bool mIsMidpoint;
  gfxRGBA mColor;
};


static void DrawBorderImage(nsPresContext* aPresContext,
                            nsRenderingContext& aRenderingContext,
                            nsIFrame* aForFrame,
                            const nsRect& aBorderArea,
                            const nsStyleBorder& aStyleBorder,
                            const nsRect& aDirtyRect,
                            Sides aSkipSides);

static nscolor MakeBevelColor(mozilla::css::Side whichSide, uint8_t style,
                              nscolor aBackgroundColor,
                              nscolor aBorderColor);

static InlineBackgroundData* gInlineBGData = nullptr;


void nsCSSRendering::Init()
{
  NS_ASSERTION(!gInlineBGData, "Init called twice");
  gInlineBGData = new InlineBackgroundData();
}


void nsCSSRendering::Shutdown()
{
  delete gInlineBGData;
  gInlineBGData = nullptr;
}




static nscolor
MakeBevelColor(mozilla::css::Side whichSide, uint8_t style,
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

static bool
GetRadii(nsIFrame* aForFrame, const nsStyleBorder& aBorder,
         const nsRect& aOrigBorderArea, const nsRect& aBorderArea,
         nscoord aRadii[8])
{
  bool haveRoundedCorners;
  nsSize sz = aBorderArea.Size();
  nsSize frameSize = aForFrame->GetSize();
  if (&aBorder == aForFrame->StyleBorder() &&
      frameSize == aOrigBorderArea.Size()) {
    haveRoundedCorners = aForFrame->GetBorderRadii(sz, sz, Sides(), aRadii);
   } else {
    haveRoundedCorners =
      nsIFrame::ComputeBorderRadii(aBorder.mBorderRadius, frameSize, sz, Sides(), aRadii);
  }

  return haveRoundedCorners;
}

static bool
GetRadii(nsIFrame* aForFrame, const nsStyleBorder& aBorder,
         const nsRect& aOrigBorderArea, const nsRect& aBorderArea,
         RectCornerRadii* aBgRadii)
{
  nscoord radii[8];
  bool haveRoundedCorners = GetRadii(aForFrame, aBorder, aOrigBorderArea, aBorderArea, radii);

  if (haveRoundedCorners) {
    auto d2a = aForFrame->PresContext()->AppUnitsPerDevPixel();
    nsCSSRendering::ComputePixelRadii(radii, d2a, aBgRadii);
  }
  return haveRoundedCorners;
}

static nsRect
JoinBoxesForVerticalSlice(nsIFrame* aFrame, const nsRect& aBorderArea)
{
  
  
  nsRect borderArea = aBorderArea;
  nscoord h = 0;
  nsIFrame* f = aFrame->GetNextContinuation();
  for (; f; f = f->GetNextContinuation()) {
    MOZ_ASSERT(!(f->GetStateBits() & NS_FRAME_PART_OF_IBSPLIT),
               "anonymous ib-split block shouldn't have border/background");
    h += f->GetRect().height;
  }
  borderArea.height += h;
  h = 0;
  f = aFrame->GetPrevContinuation();
  for (; f; f = f->GetPrevContinuation()) {
    MOZ_ASSERT(!(f->GetStateBits() & NS_FRAME_PART_OF_IBSPLIT),
               "anonymous ib-split block shouldn't have border/background");
    h += f->GetRect().height;
  }
  borderArea.y -= h;
  borderArea.height += h;
  return borderArea;
}







enum InlineBoxOrder { eForBorder, eForBackground };
static nsRect
JoinBoxesForSlice(nsIFrame* aFrame, const nsRect& aBorderArea,
                  InlineBoxOrder aOrder)
{
  if (static_cast<nsInlineFrame*>(do_QueryFrame(aFrame))) {
    return (aOrder == eForBorder
            ? gInlineBGData->GetBorderContinuousRect(aFrame, aBorderArea)
            : gInlineBGData->GetContinuousRect(aFrame)) +
      aBorderArea.TopLeft();
  }
  return JoinBoxesForVerticalSlice(aFrame, aBorderArea);
}

static bool
IsBoxDecorationSlice(const nsStyleBorder& aStyleBorder)
{
  return aStyleBorder.mBoxDecorationBreak ==
           NS_STYLE_BOX_DECORATION_BREAK_SLICE;
}

static nsRect
BoxDecorationRectForBorder(nsIFrame* aFrame, const nsRect& aBorderArea,
                           Sides aSkipSides,
                           const nsStyleBorder* aStyleBorder = nullptr)
{
  if (!aStyleBorder) {
    aStyleBorder = aFrame->StyleBorder();
  }
  
  
  return ::IsBoxDecorationSlice(*aStyleBorder) && !aSkipSides.IsEmpty()
           ? ::JoinBoxesForSlice(aFrame, aBorderArea, eForBorder)
           : aBorderArea;
}

static nsRect
BoxDecorationRectForBackground(nsIFrame* aFrame, const nsRect& aBorderArea,
                               Sides aSkipSides,
                               const nsStyleBorder* aStyleBorder = nullptr)
{
  if (!aStyleBorder) {
    aStyleBorder = aFrame->StyleBorder();
  }
  
  
  return ::IsBoxDecorationSlice(*aStyleBorder) && !aSkipSides.IsEmpty()
           ? ::JoinBoxesForSlice(aFrame, aBorderArea, eForBackground)
           : aBorderArea;
}








 void
nsCSSRendering::ComputePixelRadii(const nscoord *aAppUnitsRadii,
                                  nscoord aAppUnitsPerPixel,
                                  RectCornerRadii *oBorderRadii)
{
  Float radii[8];
  NS_FOR_CSS_HALF_CORNERS(corner)
    radii[corner] = Float(aAppUnitsRadii[corner]) / aAppUnitsPerPixel;

  (*oBorderRadii)[C_TL] = Size(radii[NS_CORNER_TOP_LEFT_X],
                               radii[NS_CORNER_TOP_LEFT_Y]);
  (*oBorderRadii)[C_TR] = Size(radii[NS_CORNER_TOP_RIGHT_X],
                               radii[NS_CORNER_TOP_RIGHT_Y]);
  (*oBorderRadii)[C_BR] = Size(radii[NS_CORNER_BOTTOM_RIGHT_X],
                               radii[NS_CORNER_BOTTOM_RIGHT_Y]);
  (*oBorderRadii)[C_BL] = Size(radii[NS_CORNER_BOTTOM_LEFT_X],
                               radii[NS_CORNER_BOTTOM_LEFT_Y]);
}

void
nsCSSRendering::PaintBorder(nsPresContext* aPresContext,
                            nsRenderingContext& aRenderingContext,
                            nsIFrame* aForFrame,
                            const nsRect& aDirtyRect,
                            const nsRect& aBorderArea,
                            nsStyleContext* aStyleContext,
                            Sides aSkipSides)
{
  PROFILER_LABEL("nsCSSRendering", "PaintBorder",
    js::ProfileEntry::Category::GRAPHICS);

  nsStyleContext *styleIfVisited = aStyleContext->GetStyleIfVisited();
  const nsStyleBorder *styleBorder = aStyleContext->StyleBorder();
  
  
  if (!styleIfVisited) {
    PaintBorderWithStyleBorder(aPresContext, aRenderingContext, aForFrame,
                               aDirtyRect, aBorderArea, *styleBorder,
                               aStyleContext, aSkipSides);
    return;
  }

  nsStyleBorder newStyleBorder(*styleBorder);
  
  
  
  newStyleBorder.TrackImage(aPresContext);

  NS_FOR_CSS_SIDES(side) {
    newStyleBorder.SetBorderColor(side,
      aStyleContext->GetVisitedDependentColor(
        nsCSSProps::SubpropertyEntryFor(eCSSProperty_border_color)[side]));
  }
  PaintBorderWithStyleBorder(aPresContext, aRenderingContext, aForFrame,
                             aDirtyRect, aBorderArea, newStyleBorder,
                             aStyleContext, aSkipSides);

  
  
  
  newStyleBorder.UntrackImage(aPresContext);
}

void
nsCSSRendering::PaintBorderWithStyleBorder(nsPresContext* aPresContext,
                                           nsRenderingContext& aRenderingContext,
                                           nsIFrame* aForFrame,
                                           const nsRect& aDirtyRect,
                                           const nsRect& aBorderArea,
                                           const nsStyleBorder& aStyleBorder,
                                           nsStyleContext* aStyleContext,
                                           Sides aSkipSides)
{
  DrawTarget& aDrawTarget = *aRenderingContext.GetDrawTarget();

  PrintAsStringNewline("++ PaintBorder");

  
  
  
  const nsStyleDisplay* displayData = aStyleContext->StyleDisplay();
  if (displayData->mAppearance) {
    nsITheme *theme = aPresContext->GetTheme();
    if (theme && theme->ThemeSupportsWidget(aPresContext, aForFrame, displayData->mAppearance))
      return; 
  }

  if (aStyleBorder.IsBorderImageLoaded()) {
    DrawBorderImage(aPresContext, aRenderingContext, aForFrame,
                    aBorderArea, aStyleBorder, aDirtyRect, aSkipSides);
    return;
  }

  
  const nsStyleColor* ourColor = aStyleContext->StyleColor();

  
  
  bool quirks = aPresContext->CompatibilityMode() == eCompatibility_NavQuirks;
  nsIFrame* bgFrame = FindNonTransparentBackgroundFrame(aForFrame, quirks);
  nsStyleContext* bgContext = bgFrame->StyleContext();
  nscolor bgColor =
    bgContext->GetVisitedDependentColor(eCSSProperty_background_color);

  nsMargin border = aStyleBorder.GetComputedBorder();
  if (0 == border.left && 0 == border.right &&
      0 == border.top  && 0 == border.bottom) {
    
    return;
  }

  
  
  nsRect joinedBorderArea =
    ::BoxDecorationRectForBorder(aForFrame, aBorderArea, aSkipSides, &aStyleBorder);
  RectCornerRadii bgRadii;
  ::GetRadii(aForFrame, aStyleBorder, aBorderArea, joinedBorderArea, &bgRadii);

  PrintAsFormatString(" joinedBorderArea: %d %d %d %d\n", joinedBorderArea.x, joinedBorderArea.y,
     joinedBorderArea.width, joinedBorderArea.height);

  
  gfxContext* ctx = aRenderingContext.ThebesContext();
  ctx->Save();

  if (::IsBoxDecorationSlice(aStyleBorder)) {
    if (joinedBorderArea.IsEqualEdges(aBorderArea)) {
      
      border.ApplySkipSides(aSkipSides);
    } else {
      
      
      aRenderingContext.ThebesContext()->
        Clip(NSRectToSnappedRect(aBorderArea,
                                 aForFrame->PresContext()->AppUnitsPerDevPixel(),
                                 aDrawTarget));
    }
  } else {
    MOZ_ASSERT(joinedBorderArea.IsEqualEdges(aBorderArea),
               "Should use aBorderArea for box-decoration-break:clone");
    MOZ_ASSERT(aForFrame->GetSkipSides().IsEmpty(),
               "Should not skip sides for box-decoration-break:clone except "
               "::first-letter/line continuations or other frame types that "
               "don't have borders but those shouldn't reach this point.");
  }

  
  nscoord twipsPerPixel = aPresContext->DevPixelsToAppUnits(1);
  Rect joinedBorderAreaPx = NSRectToRect(joinedBorderArea, twipsPerPixel);
  Float borderWidths[4] = { Float(border.top / twipsPerPixel),
                            Float(border.right / twipsPerPixel),
                            Float(border.bottom / twipsPerPixel),
                            Float(border.left / twipsPerPixel) };

  uint8_t borderStyles[4];
  nscolor borderColors[4];
  nsBorderColors *compositeColors[4];

  
  NS_FOR_CSS_SIDES (i) {
    bool foreground;
    borderStyles[i] = aStyleBorder.GetBorderStyle(i);
    aStyleBorder.GetBorderColor(i, borderColors[i], foreground);
    aStyleBorder.GetCompositeColors(i, &compositeColors[i]);

    if (foreground)
      borderColors[i] = ourColor->mColor;
  }

  PrintAsFormatString(" borderStyles: %d %d %d %d\n", borderStyles[0], borderStyles[1], borderStyles[2], borderStyles[3]);
  

#if 0
  
  ColorPattern color(ToDeviceColor(Color(1.f, 0.f, 0.f, 0.5f)));
  aDrawTarget.FillRect(joinedBorderAreaPx, color);
#endif

  nsCSSBorderRenderer br(&aDrawTarget,
                         joinedBorderAreaPx,
                         borderStyles,
                         borderWidths,
                         bgRadii,
                         borderColors,
                         compositeColors,
                         bgColor);
  br.DrawBorders();

  ctx->Restore();

  PrintAsStringNewline();
}

static nsRect
GetOutlineInnerRect(nsIFrame* aFrame)
{
  nsRect* savedOutlineInnerRect = static_cast<nsRect*>
    (aFrame->Properties().Get(nsIFrame::OutlineInnerRectProperty()));
  if (savedOutlineInnerRect)
    return *savedOutlineInnerRect;
  NS_NOTREACHED("we should have saved a frame property");
  return nsRect(nsPoint(0, 0), aFrame->GetSize());
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

  
  const nsStyleOutline* ourOutline = aStyleContext->StyleOutline();
  MOZ_ASSERT(ourOutline != NS_STYLE_BORDER_STYLE_NONE,
             "shouldn't have created nsDisplayOutline item");

  uint8_t outlineStyle = ourOutline->GetOutlineStyle();
  nscoord width;
  ourOutline->GetOutlineWidth(width);

  if (width == 0 && outlineStyle != NS_STYLE_BORDER_STYLE_AUTO) {
    
    return;
  }

  nsIFrame* bgFrame = nsCSSRendering::FindNonTransparentBackgroundFrame
    (aForFrame, false);
  nsStyleContext* bgContext = bgFrame->StyleContext();
  nscolor bgColor =
    bgContext->GetVisitedDependentColor(eCSSProperty_background_color);

  nsRect innerRect;
  if (
#ifdef MOZ_XUL
      aStyleContext->GetPseudoType() == nsCSSPseudoElements::ePseudo_XULTree
#else
      false
#endif
     ) {
    innerRect = aBorderArea;
  } else {
    innerRect = GetOutlineInnerRect(aForFrame) + aBorderArea.TopLeft();
  }
  nscoord offset = ourOutline->mOutlineOffset;
  innerRect.Inflate(offset, offset);
  
  
  
  
  
  if (innerRect.Contains(aDirtyRect))
    return;

  nsRect outerRect = innerRect;
  outerRect.Inflate(width, width);

  
  nsIFrame::ComputeBorderRadii(ourOutline->mOutlineRadius, aBorderArea.Size(),
                               outerRect.Size(), Sides(), twipsRadii);

  
  nscoord twipsPerPixel = aPresContext->DevPixelsToAppUnits(1);

  
  Rect oRect(NSRectToRect(outerRect, twipsPerPixel));

  
  nsMargin outlineMargin(width, width, width, width);
  RectCornerRadii outlineRadii;
  ComputePixelRadii(twipsRadii, twipsPerPixel, &outlineRadii);

  if (nsLayoutUtils::IsOutlineStyleAutoEnabled()) {
    if (outlineStyle == NS_STYLE_BORDER_STYLE_AUTO) {
      nsITheme* theme = aPresContext->GetTheme();
      if (theme && theme->ThemeSupportsWidget(aPresContext, aForFrame,
                                              NS_THEME_FOCUS_OUTLINE)) {
        theme->DrawWidgetBackground(&aRenderingContext, aForFrame,
                                    NS_THEME_FOCUS_OUTLINE, innerRect,
                                    aDirtyRect);
        return;
      } else if (width == 0) {
        return; 
      }
      
      
      outlineStyle = NS_STYLE_BORDER_STYLE_SOLID;
    }
  }

  uint8_t outlineStyles[4] = { outlineStyle, outlineStyle,
                               outlineStyle, outlineStyle };

  
  
  nscolor outlineColor =
    aStyleContext->GetVisitedDependentColor(eCSSProperty_outline_color);
  nscolor outlineColors[4] = { outlineColor,
                               outlineColor,
                               outlineColor,
                               outlineColor };

  
  Float outlineWidths[4] = { Float(width / twipsPerPixel),
                             Float(width / twipsPerPixel),
                             Float(width / twipsPerPixel),
                             Float(width / twipsPerPixel) };

  
  gfxContext *ctx = aRenderingContext.ThebesContext();

  nsCSSBorderRenderer br(ctx->GetDrawTarget(),
                         oRect,
                         outlineStyles,
                         outlineWidths,
                         outlineRadii,
                         outlineColors,
                         nullptr,
                         bgColor);
  br.DrawBorders();

  PrintAsStringNewline();
}

void
nsCSSRendering::PaintFocus(nsPresContext* aPresContext,
                           nsRenderingContext& aRenderingContext,
                           const nsRect& aFocusRect,
                           nscolor aColor)
{
  nscoord oneCSSPixel = nsPresContext::CSSPixelsToAppUnits(1);
  nscoord oneDevPixel = aPresContext->DevPixelsToAppUnits(1);

  Rect focusRect(NSRectToRect(aFocusRect, oneDevPixel));

  RectCornerRadii focusRadii;
  {
    nscoord twipsRadii[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    ComputePixelRadii(twipsRadii, oneDevPixel, &focusRadii);
  }
  Float focusWidths[4] = { Float(oneCSSPixel / oneDevPixel),
                           Float(oneCSSPixel / oneDevPixel),
                           Float(oneCSSPixel / oneDevPixel),
                           Float(oneCSSPixel / oneDevPixel) };

  uint8_t focusStyles[4] = { NS_STYLE_BORDER_STYLE_DOTTED,
                             NS_STYLE_BORDER_STYLE_DOTTED,
                             NS_STYLE_BORDER_STYLE_DOTTED,
                             NS_STYLE_BORDER_STYLE_DOTTED };
  nscolor focusColors[4] = { aColor, aColor, aColor, aColor };

  gfxContext *ctx = aRenderingContext.ThebesContext();

  
  
  
  
  
  
  nsCSSBorderRenderer br(ctx->GetDrawTarget(),
                         focusRect,
                         focusStyles,
                         focusWidths,
                         focusRadii,
                         focusColors,
                         nullptr,
                         NS_RGB(255, 0, 0));
  br.DrawBorders();

  PrintAsStringNewline();
}












typedef nsStyleBackground::Position::PositionCoord PositionCoord;
static void
ComputeObjectAnchorCoord(const PositionCoord& aCoord,
                         const nscoord aOriginBounds,
                         const nscoord aImageSize,
                         nscoord* aTopLeftCoord,
                         nscoord* aAnchorPointCoord)
{
  *aAnchorPointCoord = aCoord.mLength;
  *aTopLeftCoord = aCoord.mLength;

  if (aCoord.mHasPercent) {
    
    nscoord extraSpace = aOriginBounds - aImageSize;
    *aTopLeftCoord += NSToCoordRound(aCoord.mPercent * extraSpace);

    
    
    *aAnchorPointCoord += NSToCoordRound(aCoord.mPercent * aOriginBounds);
  }
}

void
nsImageRenderer::ComputeObjectAnchorPoint(
  const nsStyleBackground::Position& aPos,
  const nsSize& aOriginBounds,
  const nsSize& aImageSize,
  nsPoint* aTopLeft,
  nsPoint* aAnchorPoint)
{
  ComputeObjectAnchorCoord(aPos.mXPosition,
                           aOriginBounds.width, aImageSize.width,
                           &aTopLeft->x, &aAnchorPoint->x);

  ComputeObjectAnchorCoord(aPos.mYPosition,
                           aOriginBounds.height, aImageSize.height,
                           &aTopLeft->y, &aAnchorPoint->y);
}

nsIFrame*
nsCSSRendering::FindNonTransparentBackgroundFrame(nsIFrame* aFrame,
                                                  bool aStartAtParent )
{
  NS_ASSERTION(aFrame, "Cannot find NonTransparentBackgroundFrame in a null frame");

  nsIFrame* frame = nullptr;
  if (aStartAtParent) {
    frame = nsLayoutUtils::GetParentOrPlaceholderFor(aFrame);
  }
  if (!frame) {
    frame = aFrame;
  }

  while (frame) {
    
    
    if (NS_GET_A(frame->StyleBackground()->mBackgroundColor) > 0)
      break;

    if (frame->IsThemed())
      break;

    nsIFrame* parent = nsLayoutUtils::GetParentOrPlaceholderFor(frame);
    if (!parent)
      break;

    frame = parent;
  }
  return frame;
}





bool
nsCSSRendering::IsCanvasFrame(nsIFrame* aFrame)
{
  nsIAtom* frameType = aFrame->GetType();
  return frameType == nsGkAtoms::canvasFrame ||
         frameType == nsGkAtoms::rootFrame ||
         frameType == nsGkAtoms::pageContentFrame ||
         frameType == nsGkAtoms::viewportFrame;
}

nsIFrame*
nsCSSRendering::FindBackgroundStyleFrame(nsIFrame* aForFrame)
{
  const nsStyleBackground* result = aForFrame->StyleBackground();

  
  if (!result->IsTransparent()) {
    return aForFrame;
  }

  nsIContent* content = aForFrame->GetContent();
  
  
  
  if (!content) {
    return aForFrame;
  }

  nsIDocument* document = content->OwnerDoc();

  dom::Element* bodyContent = document->GetBodyElement();
  
  
  
  
  
  
  
  
  
  if (!bodyContent) {
    return aForFrame;
  }

  nsIFrame *bodyFrame = bodyContent->GetPrimaryFrame();
  if (!bodyFrame) {
    return aForFrame;
  }

  return nsLayoutUtils::GetStyleFrame(bodyFrame);
}




























nsStyleContext*
nsCSSRendering::FindRootFrameBackground(nsIFrame* aForFrame)
{
  return FindBackgroundStyleFrame(aForFrame)->StyleContext();
}

inline bool
FindElementBackground(nsIFrame* aForFrame, nsIFrame* aRootElementFrame,
                      nsStyleContext** aBackgroundSC)
{
  if (aForFrame == aRootElementFrame) {
    
    return false;
  }

  *aBackgroundSC = aForFrame->StyleContext();

  
  

  nsIContent* content = aForFrame->GetContent();
  if (!content || content->NodeInfo()->NameAtom() != nsGkAtoms::body)
    return true; 
  
  

  if (aForFrame->StyleContext()->GetPseudo())
    return true; 

  
  nsIDocument* document = content->OwnerDoc();

  dom::Element* bodyContent = document->GetBodyElement();
  if (bodyContent != content)
    return true; 

  
  
  
  if (!aRootElementFrame)
    return true;

  const nsStyleBackground* htmlBG = aRootElementFrame->StyleBackground();
  return !htmlBG->IsTransparent();
}

bool
nsCSSRendering::FindBackground(nsIFrame* aForFrame,
                               nsStyleContext** aBackgroundSC)
{
  nsIFrame* rootElementFrame =
    aForFrame->PresContext()->PresShell()->FrameConstructor()->GetRootElementStyleFrame();
  if (IsCanvasFrame(aForFrame)) {
    *aBackgroundSC = FindCanvasBackground(aForFrame, rootElementFrame);
    return true;
  } else {
    return FindElementBackground(aForFrame, rootElementFrame, aBackgroundSC);
  }
}

void
nsCSSRendering::BeginFrameTreesLocked()
{
  ++gFrameTreeLockCount;
}

void
nsCSSRendering::EndFrameTreesLocked()
{
  NS_ASSERTION(gFrameTreeLockCount > 0, "Unbalanced EndFrameTreeLocked");
  --gFrameTreeLockCount;
  if (gFrameTreeLockCount == 0) {
    gInlineBGData->Reset();
  }
}

void
nsCSSRendering::PaintBoxShadowOuter(nsPresContext* aPresContext,
                                    nsRenderingContext& aRenderingContext,
                                    nsIFrame* aForFrame,
                                    const nsRect& aFrameArea,
                                    const nsRect& aDirtyRect,
                                    float aOpacity)
{
  DrawTarget& aDrawTarget = *aRenderingContext.GetDrawTarget();
  const nsStyleBorder* styleBorder = aForFrame->StyleBorder();
  nsCSSShadowArray* shadows = styleBorder->mBoxShadow;
  if (!shadows)
    return;

  gfxContextAutoSaveRestore gfxStateRestorer;
  bool hasBorderRadius;
  bool nativeTheme; 
  const nsStyleDisplay* styleDisplay = aForFrame->StyleDisplay();
  nsITheme::Transparency transparency;
  if (aForFrame->IsThemed(styleDisplay, &transparency)) {
    
    hasBorderRadius = false;
    
    
    nativeTheme = transparency != nsITheme::eOpaque;
  } else {
    nativeTheme = false;
    hasBorderRadius = true; 
  }

  nsRect frameRect = nativeTheme ?
    aForFrame->GetVisualOverflowRectRelativeToSelf() + aFrameArea.TopLeft() :
    aFrameArea;
  Sides skipSides = aForFrame->GetSkipSides();
  frameRect = ::BoxDecorationRectForBorder(aForFrame, frameRect, skipSides);

  
  
  RectCornerRadii borderRadii;
  const nscoord twipsPerPixel = aPresContext->DevPixelsToAppUnits(1);
  if (hasBorderRadius) {
    nscoord twipsRadii[8];
    NS_ASSERTION(aFrameArea.Size() == aForFrame->VisualBorderRectRelativeToSelf().Size(),
                 "unexpected size");
    nsSize sz = frameRect.Size();
    hasBorderRadius = aForFrame->GetBorderRadii(sz, sz, Sides(), twipsRadii);
    if (hasBorderRadius) {
      ComputePixelRadii(twipsRadii, twipsPerPixel, &borderRadii);
    }
  }

  Rect frameGfxRect = NSRectToRect(frameRect, twipsPerPixel);
  frameGfxRect.Round();

  
  
  gfxRect skipGfxRect = ThebesRect(frameGfxRect);
  bool useSkipGfxRect = true;
  if (nativeTheme) {
    
    
    
    
    
    useSkipGfxRect = !aForFrame->IsLeaf();
    nsRect paddingRect =
      aForFrame->GetPaddingRect() - aForFrame->GetPosition() + aFrameArea.TopLeft();
    skipGfxRect = nsLayoutUtils::RectToGfxRect(paddingRect, twipsPerPixel);
  } else if (hasBorderRadius) {
    skipGfxRect.Deflate(gfxMargin(
        std::max(borderRadii[C_TL].height, borderRadii[C_TR].height), 0,
        std::max(borderRadii[C_BL].height, borderRadii[C_BR].height), 0));
  }

  gfxContext* renderContext = aRenderingContext.ThebesContext();

  for (uint32_t i = shadows->Length(); i > 0; --i) {
    nsCSSShadowItem* shadowItem = shadows->ShadowAt(i - 1);
    if (shadowItem->mInset)
      continue;

    nsRect shadowRect = frameRect;
    shadowRect.MoveBy(shadowItem->mXOffset, shadowItem->mYOffset);
    if (!nativeTheme) {
      shadowRect.Inflate(shadowItem->mSpread, shadowItem->mSpread);
    }

    
    
    nsRect shadowRectPlusBlur = shadowRect;
    nscoord blurRadius = shadowItem->mRadius;
    shadowRectPlusBlur.Inflate(
      nsContextBoxBlur::GetBlurRadiusMargin(blurRadius, twipsPerPixel));

    Rect shadowGfxRectPlusBlur =
      NSRectToRect(shadowRectPlusBlur, twipsPerPixel);
    shadowGfxRectPlusBlur.RoundOut();
    MaybeSnapToDevicePixels(shadowGfxRectPlusBlur, aDrawTarget, true);

    
    nscolor shadowColor;
    if (shadowItem->mHasColor)
      shadowColor = shadowItem->mColor;
    else
      shadowColor = aForFrame->StyleColor()->mColor;

    gfxRGBA gfxShadowColor(shadowColor);
    gfxShadowColor.a *= aOpacity;

    if (nativeTheme) {
      nsContextBoxBlur blurringArea;

      
      
      
      
      gfxContext* shadowContext =
        blurringArea.Init(shadowRect, shadowItem->mSpread,
                          blurRadius, twipsPerPixel, renderContext, aDirtyRect,
                          useSkipGfxRect ? &skipGfxRect : nullptr,
                          nsContextBoxBlur::FORCE_MASK);
      if (!shadowContext)
        continue;

      MOZ_ASSERT(shadowContext == blurringArea.GetContext());

      renderContext->Save();
      renderContext->SetColor(gfxShadowColor);

      
      
      
      
      

      
      

      
      gfxContextMatrixAutoSaveRestore save(shadowContext);
      gfxPoint devPixelOffset =
        nsLayoutUtils::PointToGfxPoint(nsPoint(shadowItem->mXOffset,
                                               shadowItem->mYOffset),
                                       aPresContext->AppUnitsPerDevPixel());
      shadowContext->SetMatrix(
        shadowContext->CurrentMatrix().Translate(devPixelOffset));

      nsRect nativeRect;
      nativeRect.IntersectRect(frameRect, aDirtyRect);
      nsRenderingContext wrapperCtx(shadowContext);
      aPresContext->GetTheme()->DrawWidgetBackground(&wrapperCtx, aForFrame,
          styleDisplay->mAppearance, aFrameArea, nativeRect);

      blurringArea.DoPaint();
      renderContext->Restore();
    } else {
      renderContext->Save();

      {
        
        
        RefPtr<PathBuilder> builder =
          aDrawTarget.CreatePathBuilder(FillRule::FILL_EVEN_ODD);
        AppendRectToPath(builder, shadowGfxRectPlusBlur);
        if (hasBorderRadius) {
          AppendRoundedRectToPath(builder, frameGfxRect, borderRadii);
        } else {
          AppendRectToPath(builder, frameGfxRect);
        }
        RefPtr<Path> path = builder->Finish();
        renderContext->Clip(path);
      }

      
      nsRect fragmentClip = shadowRectPlusBlur;
      if (!skipSides.IsEmpty()) {
        if (skipSides.Left()) {
          nscoord xmost = fragmentClip.XMost();
          fragmentClip.x = aFrameArea.x;
          fragmentClip.width = xmost - fragmentClip.x;
        }
        if (skipSides.Right()) {
          nscoord xmost = fragmentClip.XMost();
          nscoord overflow = xmost - aFrameArea.XMost();
          if (overflow > 0) {
            fragmentClip.width -= overflow;
          }
        }
        if (skipSides.Top()) {
          nscoord ymost = fragmentClip.YMost();
          fragmentClip.y = aFrameArea.y;
          fragmentClip.height = ymost - fragmentClip.y;
        }
        if (skipSides.Bottom()) {
          nscoord ymost = fragmentClip.YMost();
          nscoord overflow = ymost - aFrameArea.YMost();
          if (overflow > 0) {
            fragmentClip.height -= overflow;
          }
        }
      }
      renderContext->
        Clip(NSRectToSnappedRect(fragmentClip,
                                 aForFrame->PresContext()->AppUnitsPerDevPixel(),
                                 aDrawTarget));

      RectCornerRadii clipRectRadii;
      if (hasBorderRadius) {
        Float spreadDistance = shadowItem->mSpread / twipsPerPixel;

        Float borderSizes[4];

        borderSizes[NS_SIDE_LEFT] = spreadDistance;
        borderSizes[NS_SIDE_TOP] = spreadDistance;
        borderSizes[NS_SIDE_RIGHT] = spreadDistance;
        borderSizes[NS_SIDE_BOTTOM] = spreadDistance;

        nsCSSBorderRenderer::ComputeOuterRadii(borderRadii, borderSizes,
            &clipRectRadii);

      }
      nsContextBoxBlur::BlurRectangle(renderContext,
                                      shadowRect,
                                      twipsPerPixel,
                                      hasBorderRadius ? &clipRectRadii : nullptr,
                                      blurRadius,
                                      gfxShadowColor,
                                      aDirtyRect,
                                      skipGfxRect);
      renderContext->Restore();
    }

  }
}

void
nsCSSRendering::PaintBoxShadowInner(nsPresContext* aPresContext,
                                    nsRenderingContext& aRenderingContext,
                                    nsIFrame* aForFrame,
                                    const nsRect& aFrameArea,
                                    const nsRect& aDirtyRect)
{
  const nsStyleBorder* styleBorder = aForFrame->StyleBorder();
  nsCSSShadowArray* shadows = styleBorder->mBoxShadow;
  if (!shadows)
    return;
  if (aForFrame->IsThemed() && aForFrame->GetContent() &&
      !nsContentUtils::IsChromeDoc(aForFrame->GetContent()->GetCurrentDoc())) {
    
    
    
    
    return;
  }

  NS_ASSERTION(aForFrame->GetType() == nsGkAtoms::fieldSetFrame ||
               aFrameArea.Size() == aForFrame->GetSize(), "unexpected size");

  Sides skipSides = aForFrame->GetSkipSides();
  nsRect frameRect =
    ::BoxDecorationRectForBorder(aForFrame, aFrameArea, skipSides);
  nsRect paddingRect = frameRect;
  nsMargin border = aForFrame->GetUsedBorder();
  paddingRect.Deflate(border);

  
  
  nscoord twipsRadii[8];
  nsSize sz = frameRect.Size();
  bool hasBorderRadius = aForFrame->GetBorderRadii(sz, sz, Sides(), twipsRadii);
  const nscoord twipsPerPixel = aPresContext->DevPixelsToAppUnits(1);

  RectCornerRadii innerRadii;
  if (hasBorderRadius) {
    RectCornerRadii borderRadii;

    ComputePixelRadii(twipsRadii, twipsPerPixel, &borderRadii);
    Float borderSizes[4] = {
      Float(border.top / twipsPerPixel),
      Float(border.right / twipsPerPixel),
      Float(border.bottom / twipsPerPixel),
      Float(border.left / twipsPerPixel)
    };
    nsCSSBorderRenderer::ComputeInnerRadii(borderRadii, borderSizes,
                                           &innerRadii);
  }

  for (uint32_t i = shadows->Length(); i > 0; --i) {
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

    RectCornerRadii clipRectRadii;
    if (hasBorderRadius) {
      
      Float spreadDistance = shadowItem->mSpread / twipsPerPixel;
      Float borderSizes[4] = {0, 0, 0, 0};

      
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
      skipGfxRect.Deflate(gfxMargin(
          std::max(clipRectRadii[C_TL].height, clipRectRadii[C_TR].height), 0,
          std::max(clipRectRadii[C_BL].height, clipRectRadii[C_BR].height), 0));
    }

    
    
    
    
    gfxContext* renderContext = aRenderingContext.ThebesContext();
    DrawTarget* drawTarget = renderContext->GetDrawTarget();
    nsContextBoxBlur blurringArea;
    gfxContext* shadowContext =
      blurringArea.Init(shadowPaintRect, 0, blurRadius, twipsPerPixel,
                        renderContext, aDirtyRect, &skipGfxRect);
    if (!shadowContext)
      continue;
    DrawTarget* shadowDT = shadowContext->GetDrawTarget();

    
    MOZ_ASSERT(shadowContext == renderContext ||
               shadowContext == blurringArea.GetContext());

    
    Color shadowColor = Color::FromABGR(shadowItem->mHasColor ?
                                          shadowItem->mColor :
                                          aForFrame->StyleColor()->mColor);
    renderContext->Save();
    renderContext->SetColor(ThebesColor(shadowColor));

    
    
    
    Rect shadowGfxRect = NSRectToRect(paddingRect, twipsPerPixel);
    shadowGfxRect.Round();
    if (hasBorderRadius) {
      RefPtr<Path> roundedRect =
        MakePathForRoundedRect(*drawTarget, shadowGfxRect, innerRadii);
      renderContext->Clip(roundedRect);
    } else {
      renderContext->Clip(shadowGfxRect);
    }

    
    
    Rect shadowPaintGfxRect = NSRectToRect(shadowPaintRect, twipsPerPixel);
    shadowPaintGfxRect.RoundOut();
    Rect shadowClipGfxRect = NSRectToRect(shadowClipRect, twipsPerPixel);
    shadowClipGfxRect.Round();
    RefPtr<PathBuilder> builder =
      shadowDT->CreatePathBuilder(FillRule::FILL_EVEN_ODD);
    AppendRectToPath(builder, shadowPaintGfxRect, true);
    if (hasBorderRadius) {
      AppendRoundedRectToPath(builder, shadowClipGfxRect, clipRectRadii, false);
    } else {
      AppendRectToPath(builder, shadowClipGfxRect, false);
    }
    RefPtr<Path> path = builder->Finish();
    shadowContext->SetPath(path);
    shadowContext->Fill();
    shadowContext->NewPath();

    blurringArea.DoPaint();
    renderContext->Restore();
  }
}

DrawResult
nsCSSRendering::PaintBackground(nsPresContext* aPresContext,
                                nsRenderingContext& aRenderingContext,
                                nsIFrame* aForFrame,
                                const nsRect& aDirtyRect,
                                const nsRect& aBorderArea,
                                uint32_t aFlags,
                                nsRect* aBGClipRect,
                                int32_t aLayer)
{
  PROFILER_LABEL("nsCSSRendering", "PaintBackground",
    js::ProfileEntry::Category::GRAPHICS);

  NS_PRECONDITION(aForFrame,
                  "Frame is expected to be provided to PaintBackground");

  nsStyleContext *sc;
  if (!FindBackground(aForFrame, &sc)) {
    
    
    
    
    
    if (!aForFrame->StyleDisplay()->mAppearance) {
      return DrawResult::SUCCESS;
    }

    nsIContent* content = aForFrame->GetContent();
    if (!content || content->GetParent()) {
      return DrawResult::SUCCESS;
    }

    sc = aForFrame->StyleContext();
  }

  return PaintBackgroundWithSC(aPresContext, aRenderingContext, aForFrame,
                               aDirtyRect, aBorderArea, sc,
                               *aForFrame->StyleBorder(), aFlags,
                               aBGClipRect, aLayer);
}

static bool
IsOpaqueBorderEdge(const nsStyleBorder& aBorder, mozilla::css::Side aSide)
{
  if (aBorder.GetComputedBorder().Side(aSide) == 0)
    return true;
  switch (aBorder.GetBorderStyle(aSide)) {
  case NS_STYLE_BORDER_STYLE_SOLID:
  case NS_STYLE_BORDER_STYLE_GROOVE:
  case NS_STYLE_BORDER_STYLE_RIDGE:
  case NS_STYLE_BORDER_STYLE_INSET:
  case NS_STYLE_BORDER_STYLE_OUTSET:
    break;
  default:
    return false;
  }

  
  
  
  
  if (aBorder.mBorderImageSource.GetType() != eStyleImageType_Null)
    return false;

  nscolor color;
  bool isForeground;
  aBorder.GetBorderColor(aSide, color, isForeground);

  
  
  if (isForeground)
    return false;

  return NS_GET_A(color) == 255;
}




static bool
IsOpaqueBorder(const nsStyleBorder& aBorder)
{
  if (aBorder.mBorderColors)
    return false;
  NS_FOR_CSS_SIDES(i) {
    if (!IsOpaqueBorderEdge(aBorder, i))
      return false;
  }
  return true;
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
  MOZ_ASSERT(!aDirtyRect->IsEmpty() || aDirtyRectGfx->IsEmpty(),
             "second should be empty if first is");
}

 void
nsCSSRendering::GetBackgroundClip(const nsStyleBackground::Layer& aLayer,
                                  nsIFrame* aForFrame, const nsStyleBorder& aBorder,
                                  const nsRect& aBorderArea, const nsRect& aCallerDirtyRect,
                                  bool aWillPaintBorder, nscoord aAppUnitsPerPixel,
                                   BackgroundClipState* aClipState)
{
  
  
  Sides skipSides = aForFrame->GetSkipSides();
  nsRect clipBorderArea =
    ::BoxDecorationRectForBorder(aForFrame, aBorderArea, skipSides, &aBorder);

  bool haveRoundedCorners = GetRadii(aForFrame, aBorder, aBorderArea,
                                     clipBorderArea, aClipState->mRadii);

  uint8_t backgroundClip = aLayer.mClip;

  bool isSolidBorder =
      aWillPaintBorder && IsOpaqueBorder(aBorder);
  if (isSolidBorder && backgroundClip == NS_STYLE_BG_CLIP_BORDER) {
    
    
    
    backgroundClip = haveRoundedCorners ?
      NS_STYLE_BG_CLIP_MOZ_ALMOST_PADDING : NS_STYLE_BG_CLIP_PADDING;
  }

  aClipState->mBGClipArea = clipBorderArea;
  aClipState->mHasAdditionalBGClipArea = false;
  aClipState->mCustomClip = false;

  if (aForFrame->GetType() == nsGkAtoms::scrollFrame &&
      NS_STYLE_BG_ATTACHMENT_LOCAL == aLayer.mAttachment) {
    
    

    
    
    
    
    if (backgroundClip == NS_STYLE_BG_CLIP_CONTENT) {
      nsIScrollableFrame* scrollableFrame = do_QueryFrame(aForFrame);
      
      aClipState->mHasAdditionalBGClipArea = true;
      aClipState->mAdditionalBGClipArea = nsRect(
        aClipState->mBGClipArea.TopLeft()
          + scrollableFrame->GetScrolledFrame()->GetPosition()
          
          + scrollableFrame->GetScrollRange().TopLeft(),
        scrollableFrame->GetScrolledRect().Size());
      nsMargin padding = aForFrame->GetUsedPadding();
      
      
      padding.bottom = 0;
      padding.ApplySkipSides(skipSides);
      aClipState->mAdditionalBGClipArea.Deflate(padding);
    }

    
    
    backgroundClip = NS_STYLE_BG_CLIP_PADDING;
  }

  if (backgroundClip != NS_STYLE_BG_CLIP_BORDER) {
    nsMargin border = aForFrame->GetUsedBorder();
    if (backgroundClip == NS_STYLE_BG_CLIP_MOZ_ALMOST_PADDING) {
      
      
      
      border.top = std::max(0, border.top - aAppUnitsPerPixel);
      border.right = std::max(0, border.right - aAppUnitsPerPixel);
      border.bottom = std::max(0, border.bottom - aAppUnitsPerPixel);
      border.left = std::max(0, border.left - aAppUnitsPerPixel);
    } else if (backgroundClip != NS_STYLE_BG_CLIP_PADDING) {
      NS_ASSERTION(backgroundClip == NS_STYLE_BG_CLIP_CONTENT,
                   "unexpected background-clip");
      border += aForFrame->GetUsedPadding();
    }
    border.ApplySkipSides(skipSides);
    aClipState->mBGClipArea.Deflate(border);

    if (haveRoundedCorners) {
      nsIFrame::InsetBorderRadii(aClipState->mRadii, border);
    }
  }

  if (haveRoundedCorners) {
    auto d2a = aForFrame->PresContext()->AppUnitsPerDevPixel();
    nsCSSRendering::ComputePixelRadii(aClipState->mRadii, d2a, &aClipState->mClippedRadii);
    aClipState->mHasRoundedCorners = true;
  } else {
    aClipState->mHasRoundedCorners = false;
  }


  if (!haveRoundedCorners && aClipState->mHasAdditionalBGClipArea) {
    
    aClipState->mBGClipArea =
      aClipState->mBGClipArea.Intersect(aClipState->mAdditionalBGClipArea);
    aClipState->mHasAdditionalBGClipArea = false;
  }

  SetupDirtyRects(aClipState->mBGClipArea, aCallerDirtyRect, aAppUnitsPerPixel,
                  &aClipState->mDirtyRect, &aClipState->mDirtyRectGfx);
}

static void
SetupBackgroundClip(nsCSSRendering::BackgroundClipState& aClipState,
                    gfxContext *aCtx, nscoord aAppUnitsPerPixel,
                    gfxContextAutoSaveRestore* aAutoSR)
{
  if (aClipState.mDirtyRectGfx.IsEmpty()) {
    
    
    return;
  }

  if (aClipState.mCustomClip) {
    
    
    return;
  }

  DrawTarget* drawTarget = aCtx->GetDrawTarget();

  
  
  
  
  
  

  if (aClipState.mHasAdditionalBGClipArea) {
    gfxRect bgAreaGfx = nsLayoutUtils::RectToGfxRect(
      aClipState.mAdditionalBGClipArea, aAppUnitsPerPixel);
    bgAreaGfx.Round();
    bgAreaGfx.Condition();

    aAutoSR->EnsureSaved(aCtx);
    aCtx->NewPath();
    aCtx->Rectangle(bgAreaGfx, true);
    aCtx->Clip();
  }

  if (aClipState.mHasRoundedCorners) {
    Rect bgAreaGfx = NSRectToRect(aClipState.mBGClipArea, aAppUnitsPerPixel);
    bgAreaGfx.Round();

    if (bgAreaGfx.IsEmpty()) {
      
      
      NS_WARNING("converted background area should not be empty");
      
      aClipState.mDirtyRectGfx.SizeTo(gfxSize(0.0, 0.0));
      return;
    }

    aAutoSR->EnsureSaved(aCtx);

    RefPtr<Path> roundedRect =
      MakePathForRoundedRect(*drawTarget, bgAreaGfx, aClipState.mClippedRadii);
    aCtx->Clip(roundedRect);
  }
}

static void
DrawBackgroundColor(nsCSSRendering::BackgroundClipState& aClipState,
                    gfxContext *aCtx, nscoord aAppUnitsPerPixel)
{
  if (aClipState.mDirtyRectGfx.IsEmpty()) {
    
    
    return;
  }

  DrawTarget* drawTarget = aCtx->GetDrawTarget();

  
  
  if (!aClipState.mHasRoundedCorners || aClipState.mCustomClip) {
    aCtx->NewPath();
    aCtx->Rectangle(aClipState.mDirtyRectGfx, true);
    aCtx->Fill();
    return;
  }

  Rect bgAreaGfx = NSRectToRect(aClipState.mBGClipArea, aAppUnitsPerPixel);
  bgAreaGfx.Round();

  if (bgAreaGfx.IsEmpty()) {
    
    
    NS_WARNING("converted background area should not be empty");
    
    aClipState.mDirtyRectGfx.SizeTo(gfxSize(0.0, 0.0));
    return;
  }

  aCtx->Save();
  gfxRect dirty = ThebesRect(bgAreaGfx).Intersect(aClipState.mDirtyRectGfx);

  aCtx->NewPath();
  aCtx->Rectangle(dirty, true);
  aCtx->Clip();

  if (aClipState.mHasAdditionalBGClipArea) {
    gfxRect bgAdditionalAreaGfx = nsLayoutUtils::RectToGfxRect(
      aClipState.mAdditionalBGClipArea, aAppUnitsPerPixel);
    bgAdditionalAreaGfx.Round();
    bgAdditionalAreaGfx.Condition();
    aCtx->NewPath();
    aCtx->Rectangle(bgAdditionalAreaGfx, true);
    aCtx->Clip();
  }

  RefPtr<Path> roundedRect =
    MakePathForRoundedRect(*drawTarget, bgAreaGfx, aClipState.mClippedRadii);
  aCtx->SetPath(roundedRect);
  aCtx->Fill();
  aCtx->Restore();
}

nscolor
nsCSSRendering::DetermineBackgroundColor(nsPresContext* aPresContext,
                                         nsStyleContext* aStyleContext,
                                         nsIFrame* aFrame,
                                         bool& aDrawBackgroundImage,
                                         bool& aDrawBackgroundColor)
{
  aDrawBackgroundImage = true;
  aDrawBackgroundColor = true;

  if (aFrame->HonorPrintBackgroundSettings()) {
    aDrawBackgroundImage = aPresContext->GetBackgroundImageDraw();
    aDrawBackgroundColor = aPresContext->GetBackgroundColorDraw();
  }

  const nsStyleBackground *bg = aStyleContext->StyleBackground();
  nscolor bgColor;
  if (aDrawBackgroundColor) {
    bgColor =
      aStyleContext->GetVisitedDependentColor(eCSSProperty_background_color);
    if (NS_GET_A(bgColor) == 0) {
      aDrawBackgroundColor = false;
    }
  } else {
    
    
    
    
    bgColor = NS_RGB(255, 255, 255);
    if (aDrawBackgroundImage || !bg->IsTransparent()) {
      aDrawBackgroundColor = true;
    } else {
      bgColor = NS_RGBA(0,0,0,0);
    }
  }

  
  if (aDrawBackgroundColor &&
      bg->BottomLayer().mRepeat.mXRepeat == NS_STYLE_BG_REPEAT_REPEAT &&
      bg->BottomLayer().mRepeat.mYRepeat == NS_STYLE_BG_REPEAT_REPEAT &&
      bg->BottomLayer().mImage.IsOpaque() &&
      bg->BottomLayer().mBlendMode == NS_STYLE_BLEND_NORMAL) {
    aDrawBackgroundColor = false;
  }

  return bgColor;
}

static gfxFloat
ConvertGradientValueToPixels(const nsStyleCoord& aCoord,
                             gfxFloat aFillLength,
                             int32_t aAppUnitsPerPixel)
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
      if (!aGradient->mLegacySyntax) {
        angle = M_PI_2 - angle;
      }
    } else {
      angle = -M_PI_2; 
    }
    gfxPoint center(aBoxSize.width/2, aBoxSize.height/2);
    *aLineEnd = ComputeGradientLineEndFromAngle(center, angle, aBoxSize);
    *aLineStart = gfxPoint(aBoxSize.width, aBoxSize.height) - *aLineEnd;
  } else if (!aGradient->mLegacySyntax) {
    float xSign = aGradient->mBgPosX.GetPercentValue() * 2 - 1;
    float ySign = 1 - aGradient->mBgPosY.GetPercentValue() * 2;
    double angle = atan2(ySign * aBoxSize.width, xSign * aBoxSize.height);
    gfxPoint center(aBoxSize.width/2, aBoxSize.height/2);
    *aLineEnd = ComputeGradientLineEndFromAngle(center, angle, aBoxSize);
    *aLineStart = gfxPoint(aBoxSize.width, aBoxSize.height) - *aLineEnd;
  } else {
    int32_t appUnitsPerPixel = aPresContext->AppUnitsPerDevPixel();
    *aLineStart = gfxPoint(
      ConvertGradientValueToPixels(aGradient->mBgPosX, aBoxSize.width,
                                   appUnitsPerPixel),
      ConvertGradientValueToPixels(aGradient->mBgPosY, aBoxSize.height,
                                   appUnitsPerPixel));
    if (aGradient->mAngle.IsAngleValue()) {
      MOZ_ASSERT(aGradient->mLegacySyntax);
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
    int32_t appUnitsPerPixel = aPresContext->AppUnitsPerDevPixel();
    *aLineStart = gfxPoint(
      ConvertGradientValueToPixels(aGradient->mBgPosX, aBoxSize.width,
                                   appUnitsPerPixel),
      ConvertGradientValueToPixels(aGradient->mBgPosY, aBoxSize.height,
                                   appUnitsPerPixel));
  }

  
  double radiusX, radiusY;
  double leftDistance = Abs(aLineStart->x);
  double rightDistance = Abs(aBoxSize.width - aLineStart->x);
  double topDistance = Abs(aLineStart->y);
  double bottomDistance = Abs(aBoxSize.height - aLineStart->y);
  switch (aGradient->mSize) {
  case NS_STYLE_GRADIENT_SIZE_CLOSEST_SIDE:
    radiusX = std::min(leftDistance, rightDistance);
    radiusY = std::min(topDistance, bottomDistance);
    if (aGradient->mShape == NS_STYLE_GRADIENT_SHAPE_CIRCULAR) {
      radiusX = radiusY = std::min(radiusX, radiusY);
    }
    break;
  case NS_STYLE_GRADIENT_SIZE_CLOSEST_CORNER: {
    
    double offsetX = std::min(leftDistance, rightDistance);
    double offsetY = std::min(topDistance, bottomDistance);
    if (aGradient->mShape == NS_STYLE_GRADIENT_SHAPE_CIRCULAR) {
      radiusX = radiusY = NS_hypot(offsetX, offsetY);
    } else {
      
      radiusX = offsetX*M_SQRT2;
      radiusY = offsetY*M_SQRT2;
    }
    break;
  }
  case NS_STYLE_GRADIENT_SIZE_FARTHEST_SIDE:
    radiusX = std::max(leftDistance, rightDistance);
    radiusY = std::max(topDistance, bottomDistance);
    if (aGradient->mShape == NS_STYLE_GRADIENT_SHAPE_CIRCULAR) {
      radiusX = radiusY = std::max(radiusX, radiusY);
    }
    break;
  case NS_STYLE_GRADIENT_SIZE_FARTHEST_CORNER: {
    
    double offsetX = std::max(leftDistance, rightDistance);
    double offsetY = std::max(topDistance, bottomDistance);
    if (aGradient->mShape == NS_STYLE_GRADIENT_SHAPE_CIRCULAR) {
      radiusX = radiusY = NS_hypot(offsetX, offsetY);
    } else {
      
      radiusX = offsetX*M_SQRT2;
      radiusY = offsetY*M_SQRT2;
    }
    break;
  }
  case NS_STYLE_GRADIENT_SIZE_EXPLICIT_SIZE: {
    int32_t appUnitsPerPixel = aPresContext->AppUnitsPerDevPixel();
    radiusX = ConvertGradientValueToPixels(aGradient->mRadiusX,
                                           aBoxSize.width, appUnitsPerPixel);
    radiusY = ConvertGradientValueToPixels(aGradient->mRadiusY,
                                           aBoxSize.height, appUnitsPerPixel);
    break;
  }
  default:
    radiusX = radiusY = 0;
    MOZ_ASSERT(false, "unknown radial gradient sizing method");
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


static float Interpolate(float aF1, float aF2, float aFrac)
{
  return aF1 + aFrac * (aF2 - aF1);
}




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
  double multiples = floor(double(aDirtyCoord - aTilePos)/aTileDim);
  return NSToCoordRound(multiples*aTileDim + aTilePos);
}

static gfxFloat
LinearGradientStopPositionForPoint(const gfxPoint& aGradientStart,
                                   const gfxPoint& aGradientEnd,
                                   const gfxPoint& aPoint)
{
  gfxPoint d = aGradientEnd - aGradientStart;
  gfxPoint p = aPoint - aGradientStart;
  















  double numerator = d.x * p.x + d.y * p.y;
  double denominator = d.x * d.x + d.y * d.y;
  return numerator / denominator;
}

static bool
RectIsBeyondLinearGradientEdge(const gfxRect& aRect,
                               const gfxMatrix& aPatternMatrix,
                               const nsTArray<ColorStop>& aStops,
                               const gfxPoint& aGradientStart,
                               const gfxPoint& aGradientEnd,
                               gfxRGBA* aOutEdgeColor)
{
  gfxFloat topLeft = LinearGradientStopPositionForPoint(
    aGradientStart, aGradientEnd, aPatternMatrix.Transform(aRect.TopLeft()));
  gfxFloat topRight = LinearGradientStopPositionForPoint(
    aGradientStart, aGradientEnd, aPatternMatrix.Transform(aRect.TopRight()));
  gfxFloat bottomLeft = LinearGradientStopPositionForPoint(
    aGradientStart, aGradientEnd, aPatternMatrix.Transform(aRect.BottomLeft()));
  gfxFloat bottomRight = LinearGradientStopPositionForPoint(
    aGradientStart, aGradientEnd, aPatternMatrix.Transform(aRect.BottomRight()));

  const ColorStop& firstStop = aStops[0];
  if (topLeft < firstStop.mPosition && topRight < firstStop.mPosition &&
      bottomLeft < firstStop.mPosition && bottomRight < firstStop.mPosition) {
    *aOutEdgeColor = firstStop.mColor;
    return true;
  }

  const ColorStop& lastStop = aStops.LastElement();
  if (topLeft >= lastStop.mPosition && topRight >= lastStop.mPosition &&
      bottomLeft >= lastStop.mPosition && bottomRight >= lastStop.mPosition) {
    *aOutEdgeColor = lastStop.mColor;
    return true;
  }

  return false;
}

static void ResolveMidpoints(nsTArray<ColorStop>& stops)
{
  for (size_t x = 1; x < stops.Length() - 1;) {
    if (!stops[x].mIsMidpoint) {
      x++;
      continue;
    }

    gfxRGBA color1 = stops[x-1].mColor;
    gfxRGBA color2 = stops[x+1].mColor;
    float offset1 = stops[x-1].mPosition;
    float offset2 = stops[x+1].mPosition;
    float offset = stops[x].mPosition;
    
    if (offset - offset1 == offset2 - offset) {
      stops.RemoveElementAt(x);
      continue;
    }

    
    if (offset1 == offset) {
      
      
      stops[x].mColor = color2;
      stops[x].mIsMidpoint = false;
      continue;
    }

    
    if (offset2 == offset) {
      
      
      stops[x].mColor = color1;
      stops[x].mIsMidpoint = false;
      continue;
    }

    float midpoint = (offset - offset1) / (offset2 - offset1);
    ColorStop newStops[9];
    if (midpoint > .5f) {
      for (size_t y = 0; y < 7; y++) {
        newStops[y].mPosition = offset1 + (offset - offset1) * (7 + y) / 13;
      }

      newStops[7].mPosition = offset + (offset2 - offset) / 3;
      newStops[8].mPosition = offset + (offset2 - offset) * 2 / 3;
    } else {
      newStops[0].mPosition = offset1 + (offset - offset1) / 3;
      newStops[1].mPosition = offset1 + (offset - offset1) * 2 / 3;

      for (size_t y = 0; y < 7; y++) {
        newStops[y+2].mPosition = offset + (offset2 - offset) * y / 13;
      }
    }
    

    for (size_t y = 0; y < 9; y++) {
      
      
      
      
      
      float relativeOffset = (newStops[y].mPosition - offset1) / (offset2 - offset1);
      float multiplier = powf(relativeOffset, logf(.5f) / logf(midpoint));
      
      gfxFloat red = color1.r + multiplier * (color2.r - color1.r);
      gfxFloat green = color1.g + multiplier * (color2.g - color1.g);
      gfxFloat blue = color1.b + multiplier * (color2.b - color1.b);
      gfxFloat alpha = color1.a + multiplier * (color2.a - color1.a);

      newStops[y].mColor = gfxRGBA(red, green, blue, alpha);
    }

    stops.ReplaceElementsAt(x, 1, newStops, 9);
    x += 9;
  }
}

static gfxRGBA
Premultiply(const gfxRGBA& aColor)
{
  gfxFloat a = aColor.a;
  return gfxRGBA(aColor.r * a, aColor.g * a, aColor.b * a, a);
}

static gfxRGBA
Unpremultiply(const gfxRGBA& aColor)
{
  gfxFloat a = aColor.a;
  return (a > 0.0) ? gfxRGBA(aColor.r / a, aColor.g / a, aColor.b / a, a) : aColor;
}

static gfxRGBA
TransparentColor(gfxRGBA aColor) {
  aColor.a = 0;
  return aColor;
}




static const float kAlphaIncrementPerGradientStep = 0.1f;
static void
ResolvePremultipliedAlpha(nsTArray<ColorStop>& aStops)
{
  for (size_t x = 1; x < aStops.Length(); x++) {
    const ColorStop leftStop = aStops[x - 1];
    const ColorStop rightStop = aStops[x];

    
    
    if (leftStop.mColor.a == rightStop.mColor.a) {
      continue;
    }

    
    
    if (leftStop.mColor.a == 0) {
      aStops[x - 1].mColor = TransparentColor(rightStop.mColor);
      continue;
    }

    
    
    if (rightStop.mColor.a == 0) {
      ColorStop newStop = rightStop;
      newStop.mColor = TransparentColor(leftStop.mColor);
      aStops.InsertElementAt(x, newStop);
      x++;
      continue;
    }

    
    if (leftStop.mColor.a != 1.0f || rightStop.mColor.a != 1.0f) {
      gfxRGBA premulLeftColor = Premultiply(leftStop.mColor);
      gfxRGBA premulRightColor = Premultiply(rightStop.mColor);
      
      size_t stepCount = NSToIntFloor(fabs(leftStop.mColor.a - rightStop.mColor.a) / kAlphaIncrementPerGradientStep);
      for (size_t y = 1; y < stepCount; y++) {
        float frac = static_cast<float>(y) / stepCount;
        ColorStop newStop(Interpolate(leftStop.mPosition, rightStop.mPosition, frac), false,
                          Unpremultiply(InterpolateColor(premulLeftColor, premulRightColor, frac)));
        aStops.InsertElementAt(x, newStop);
        x++;
      }
    }
  }
}

void
nsCSSRendering::PaintGradient(nsPresContext* aPresContext,
                              nsRenderingContext& aRenderingContext,
                              nsStyleGradient* aGradient,
                              const nsRect& aDirtyRect,
                              const nsRect& aDest,
                              const nsRect& aFillArea,
                              const CSSIntRect& aSrc,
                              const nsSize& aIntrinsicSize)
{
  PROFILER_LABEL("nsCSSRendering", "PaintGradient",
    js::ProfileEntry::Category::GRAPHICS);

  Telemetry::AutoTimer<Telemetry::GRADIENT_DURATION, Telemetry::Microsecond> gradientTimer;
  if (aDest.IsEmpty() || aFillArea.IsEmpty()) {
    return;
  }

  gfxContext *ctx = aRenderingContext.ThebesContext();
  nscoord appUnitsPerDevPixel = aPresContext->AppUnitsPerDevPixel();
  gfxSize srcSize = gfxSize(gfxFloat(aIntrinsicSize.width)/appUnitsPerDevPixel,
                            gfxFloat(aIntrinsicSize.height)/appUnitsPerDevPixel);

  bool cellContainsFill = aDest.Contains(aFillArea);

  
  
  gfxPoint lineStart, lineEnd;
  double radiusX = 0, radiusY = 0; 
  if (aGradient->mShape == NS_STYLE_GRADIENT_SHAPE_LINEAR) {
    ComputeLinearGradientLine(aPresContext, aGradient, srcSize,
                              &lineStart, &lineEnd);
  } else {
    ComputeRadialGradientLine(aPresContext, aGradient, srcSize,
                              &lineStart, &lineEnd, &radiusX, &radiusY);
  }
  gfxFloat lineLength = NS_hypot(lineEnd.x - lineStart.x,
                                  lineEnd.y - lineStart.y);

  MOZ_ASSERT(aGradient->mStops.Length() >= 2,
             "The parser should reject gradients with less than two stops");

  
  nsTArray<ColorStop> stops;
  
  
  
  int32_t firstUnsetPosition = -1;
  for (uint32_t i = 0; i < aGradient->mStops.Length(); ++i) {
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
        stops.AppendElement(ColorStop(0, stop.mIsInterpolationHint, stop.mColor));
        continue;
      }
      break;
    case eStyleUnit_Percent:
      position = stop.mLocation.GetPercentValue();
      break;
    case eStyleUnit_Coord:
      position = lineLength < 1e-6 ? 0.0 :
          stop.mLocation.GetCoordValue() / appUnitsPerDevPixel / lineLength;
      break;
    case eStyleUnit_Calc:
      nsStyleCoord::Calc *calc;
      calc = stop.mLocation.GetCalcValue();
      position = calc->mPercent +
          ((lineLength < 1e-6) ? 0.0 :
          (NSAppUnitsToFloatPixels(calc->mLength, appUnitsPerDevPixel) / lineLength));
      break;
    default:
      MOZ_ASSERT(false, "Unknown stop position type");
    }

    if (i > 0) {
      
      
      position = std::max(position, stops[i - 1].mPosition);
    }
    stops.AppendElement(ColorStop(position, stop.mIsInterpolationHint, stop.mColor));
    if (firstUnsetPosition > 0) {
      
      double p = stops[firstUnsetPosition - 1].mPosition;
      double d = (stops[i].mPosition - p)/(i - firstUnsetPosition + 1);
      for (uint32_t j = firstUnsetPosition; j < i; ++j) {
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
        double instanceCount = ceil(-firstStop/stopDelta);
        
        
        double offset = instanceCount*stopDelta;
        for (uint32_t i = 0; i < stops.Length(); i++) {
          stops[i].mPosition += offset;
        }
      }
    } else {
      
      
      
      for (uint32_t i = 0; i < stops.Length(); i++) {
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
    MOZ_ASSERT(firstStop >= 0.0, "Failed to fix stop offsets");
  }

  if (aGradient->mShape != NS_STYLE_GRADIENT_SHAPE_LINEAR && !aGradient->mRepeating) {
    
    
    
    
    
    firstStop = 0;
  }

  double lastStop = stops[stops.Length() - 1].mPosition;
  
  
  
  double stopScale;
  double stopOrigin = firstStop;
  double stopEnd = lastStop;
  double stopDelta = lastStop - firstStop;
  bool zeroRadius = aGradient->mShape != NS_STYLE_GRADIENT_SHAPE_LINEAR &&
                      (radiusX < 1e-6 || radiusY < 1e-6);
  if (stopDelta < 1e-6 || lineLength < 1e-6 || zeroRadius) {
    
    
    
    
    if (aGradient->mRepeating || zeroRadius) {
      radiusX = radiusY = 0.0;
    }
    stopDelta = 0.0;
    lastStop = firstStop;
  }

  
  
  
  
  if (!aGradient->mRepeating || stopDelta == 0.0) {
    stopOrigin = std::min(stopOrigin, 0.0);
    stopEnd = std::max(stopEnd, 1.0);
  }
  stopScale = 1.0/(stopEnd - stopOrigin);

  
  nsRefPtr<gfxPattern> gradientPattern;
  bool forceRepeatToCoverTiles = false;
  gfxMatrix matrix;
  gfxPoint gradientStart;
  gfxPoint gradientEnd;
  if (aGradient->mShape == NS_STYLE_GRADIENT_SHAPE_LINEAR) {
    
    
    gradientStart = lineStart + (lineEnd - lineStart)*stopOrigin;
    gradientEnd = lineStart + (lineEnd - lineStart)*stopEnd;
    gfxPoint gradientStopStart = lineStart + (lineEnd - lineStart)*firstStop;
    gfxPoint gradientStopEnd = lineStart + (lineEnd - lineStart)*lastStop;

    if (stopDelta == 0.0) {
      
      
      
      
      
      
      gradientEnd = gradientStart + (lineEnd - lineStart);
      gradientStopEnd = gradientStopStart + (lineEnd - lineStart);
    }

    gradientPattern = new gfxPattern(gradientStart.x, gradientStart.y,
                                      gradientEnd.x, gradientEnd.y);

    
    
    
    if (!cellContainsFill &&
        ((gradientStopStart.y == gradientStopEnd.y && gradientStopStart.x == 0 &&
          gradientStopEnd.x == srcSize.width) ||
          (gradientStopStart.x == gradientStopEnd.x && gradientStopStart.y == 0 &&
          gradientStopEnd.y == srcSize.height))) {
      forceRepeatToCoverTiles = true;
    }
  } else {
    NS_ASSERTION(firstStop >= 0.0,
                  "Negative stops not allowed for radial gradients");

    
    
    double innerRadius = radiusX*stopOrigin;
    double outerRadius = radiusX*stopEnd;
    if (stopDelta == 0.0) {
      
      
      outerRadius = innerRadius + 1;
    }
    gradientPattern = new gfxPattern(lineStart.x, lineStart.y, innerRadius,
                                     lineStart.x, lineStart.y, outerRadius);
    if (radiusX != radiusY) {
      
      
      
      
      
      matrix.Translate(lineStart);
      matrix.Scale(1.0, radiusX/radiusY);
      matrix.Translate(-lineStart);
    }
  }
  
  matrix.Translate(gfxPoint(aPresContext->CSSPixelsToDevPixels(aSrc.x),
                            aPresContext->CSSPixelsToDevPixels(aSrc.y)));
  matrix.Scale(gfxFloat(aPresContext->CSSPixelsToAppUnits(aSrc.width))/aDest.width,
               gfxFloat(aPresContext->CSSPixelsToAppUnits(aSrc.height))/aDest.height);
  gradientPattern->SetMatrix(matrix);

  if (gradientPattern->CairoStatus())
    return;

  if (stopDelta == 0.0) {
    
    
    
    
    
    gfxRGBA firstColor(stops[0].mColor);
    gfxRGBA lastColor(stops.LastElement().mColor);
    stops.Clear();

    if (!aGradient->mRepeating && !zeroRadius) {
      stops.AppendElement(ColorStop(firstStop, false, firstColor));
    }
    stops.AppendElement(ColorStop(firstStop, false, lastColor));
  }

  ResolveMidpoints(stops);
  ResolvePremultipliedAlpha(stops);

  bool isRepeat = aGradient->mRepeating || forceRepeatToCoverTiles;

  
  
  
  
  
  
  
  nsTArray<gfx::GradientStop> rawStops(stops.Length());
  rawStops.SetLength(stops.Length());
  for(uint32_t i = 0; i < stops.Length(); i++) {
    rawStops[i].color = gfx::Color(stops[i].mColor.r, stops[i].mColor.g, stops[i].mColor.b, stops[i].mColor.a);
    rawStops[i].offset = stopScale * (stops[i].mPosition - stopOrigin);
  }
  mozilla::RefPtr<mozilla::gfx::GradientStops> gs =
    gfxGradientCache::GetOrCreateGradientStops(ctx->GetDrawTarget(),
                                               rawStops,
                                               isRepeat ? gfx::ExtendMode::REPEAT : gfx::ExtendMode::CLAMP);
  gradientPattern->SetColorStops(gs);

  
  
  
  
  nsRect dirty;
  if (!dirty.IntersectRect(aDirtyRect, aFillArea))
    return;

  gfxRect areaToFill =
    nsLayoutUtils::RectToGfxRect(aFillArea, appUnitsPerDevPixel);
  gfxRect dirtyAreaToFill = nsLayoutUtils::RectToGfxRect(dirty, appUnitsPerDevPixel);
  dirtyAreaToFill.RoundOut();

  gfxMatrix ctm = ctx->CurrentMatrix();
  bool isCTMPreservingAxisAlignedRectangles = ctm.PreservesAxisAlignedRectangles();

  
  nscoord xStart = FindTileStart(dirty.x, aDest.x, aDest.width);
  nscoord yStart = FindTileStart(dirty.y, aDest.y, aDest.height);
  nscoord xEnd = forceRepeatToCoverTiles ? xStart + aDest.width : dirty.XMost();
  nscoord yEnd = forceRepeatToCoverTiles ? yStart + aDest.height : dirty.YMost();

  
  for (nscoord y = yStart; y < yEnd; y += aDest.height) {
    for (nscoord x = xStart; x < xEnd; x += aDest.width) {
      
      gfxRect tileRect = nsLayoutUtils::RectToGfxRect(
                      nsRect(x, y, aDest.width, aDest.height),
                      appUnitsPerDevPixel);
      
      
      gfxRect fillRect =
        forceRepeatToCoverTiles ? areaToFill : tileRect.Intersect(areaToFill);
      
      
      gfxPoint snappedFillRectTopLeft = fillRect.TopLeft();
      gfxPoint snappedFillRectTopRight = fillRect.TopRight();
      gfxPoint snappedFillRectBottomRight = fillRect.BottomRight();
      
      
      if (isCTMPreservingAxisAlignedRectangles &&
          ctx->UserToDevicePixelSnapped(snappedFillRectTopLeft, true) &&
          ctx->UserToDevicePixelSnapped(snappedFillRectBottomRight, true) &&
          ctx->UserToDevicePixelSnapped(snappedFillRectTopRight, true)) {
        if (snappedFillRectTopLeft.x == snappedFillRectBottomRight.x ||
            snappedFillRectTopLeft.y == snappedFillRectBottomRight.y) {
          
          
          continue;
        }
        
        
        
        gfxMatrix transform = gfxUtils::TransformRectToRect(fillRect,
            snappedFillRectTopLeft, snappedFillRectTopRight,
            snappedFillRectBottomRight);
        ctx->SetMatrix(transform);
      }
      ctx->NewPath();
      ctx->Rectangle(fillRect);

      gfxRect dirtyFillRect = fillRect.Intersect(dirtyAreaToFill);
      gfxRect fillRectRelativeToTile = dirtyFillRect - tileRect.TopLeft();
      gfxRGBA edgeColor;
      if (aGradient->mShape == NS_STYLE_GRADIENT_SHAPE_LINEAR && !isRepeat &&
          RectIsBeyondLinearGradientEdge(fillRectRelativeToTile, matrix, stops,
                                         gradientStart, gradientEnd, &edgeColor)) {
        ctx->SetColor(edgeColor);
      } else {
        ctx->SetMatrix(
          ctx->CurrentMatrix().Copy().Translate(tileRect.TopLeft()));
        ctx->SetPattern(gradientPattern);
      }
      ctx->Fill();
      ctx->SetMatrix(ctm);
    }
  }
}

DrawResult
nsCSSRendering::PaintBackgroundWithSC(nsPresContext* aPresContext,
                                      nsRenderingContext& aRenderingContext,
                                      nsIFrame* aForFrame,
                                      const nsRect& aDirtyRect,
                                      const nsRect& aBorderArea,
                                      nsStyleContext* aBackgroundSC,
                                      const nsStyleBorder& aBorder,
                                      uint32_t aFlags,
                                      nsRect* aBGClipRect,
                                      int32_t aLayer)
{
  NS_PRECONDITION(aForFrame,
                  "Frame is expected to be provided to PaintBackground");

  
  
  
  DrawResult result = DrawResult::SUCCESS;

  
  
  
  const nsStyleDisplay* displayData = aForFrame->StyleDisplay();
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
      return DrawResult::SUCCESS;
    }
  }

  
  
  
  
  
  
  
  bool isCanvasFrame = IsCanvasFrame(aForFrame);

  
  
  bool drawBackgroundImage;
  bool drawBackgroundColor;

  nscolor bgColor = DetermineBackgroundColor(aPresContext,
                                             aBackgroundSC,
                                             aForFrame,
                                             drawBackgroundImage,
                                             drawBackgroundColor);

  
  
  const nsStyleBackground *bg = aBackgroundSC->StyleBackground();
  if (drawBackgroundColor && aLayer >= 0) {
    drawBackgroundColor = false;
  }

  
  
  
  if (!drawBackgroundImage && !drawBackgroundColor)
    return DrawResult::SUCCESS;

  
  
  Sides skipSides = aForFrame->GetSkipSides();
  nsRect paintBorderArea =
    ::BoxDecorationRectForBackground(aForFrame, aBorderArea, skipSides, &aBorder);
  nsRect clipBorderArea =
    ::BoxDecorationRectForBorder(aForFrame, aBorderArea, skipSides, &aBorder);

  
  
  
  
  
  
  gfxContext* ctx = aRenderingContext.ThebesContext();
  nscoord appUnitsPerPixel = aPresContext->AppUnitsPerDevPixel();
  BackgroundClipState clipState;
  if (aBGClipRect) {
    clipState.mBGClipArea = *aBGClipRect;
    clipState.mCustomClip = true;
    clipState.mHasRoundedCorners = false;
    SetupDirtyRects(clipState.mBGClipArea, aDirtyRect, appUnitsPerPixel,
                    &clipState.mDirtyRect, &clipState.mDirtyRectGfx);
  } else {
    GetBackgroundClip(bg->BottomLayer(),
                      aForFrame, aBorder, aBorderArea,
                      aDirtyRect, (aFlags & PAINTBG_WILL_PAINT_BORDER), appUnitsPerPixel,
                      &clipState);
  }

  
  if (drawBackgroundColor && !isCanvasFrame)
    ctx->SetColor(gfxRGBA(bgColor));

  
  
  gfxContextAutoSaveRestore autoSR;

  
  
  
  if (!drawBackgroundImage) {
    if (!isCanvasFrame) {
      DrawBackgroundColor(clipState, ctx, appUnitsPerPixel);
    }
    return DrawResult::SUCCESS;
  }

  if (bg->mImageCount < 1) {
    
    
    return DrawResult::SUCCESS;
  }

  
  int32_t startLayer = aLayer;
  int32_t nLayers = 1;
  if (startLayer < 0) {
    startLayer = (int32_t)bg->mImageCount - 1;
    nLayers = bg->mImageCount;
  }

  
  
  
  if (aBackgroundSC != aForFrame->StyleContext()) {
    NS_FOR_VISIBLE_BACKGROUND_LAYERS_BACK_TO_FRONT_WITH_RANGE(i, bg, startLayer, nLayers) {
      aForFrame->AssociateImage(bg->mLayers[i].mImage, aPresContext);
    }
  }

  
  
  if (drawBackgroundColor && !isCanvasFrame) {
    DrawBackgroundColor(clipState, ctx, appUnitsPerPixel);
  }

  if (drawBackgroundImage) {
    bool clipSet = false;
    uint8_t currentBackgroundClip = NS_STYLE_BG_CLIP_BORDER;
    NS_FOR_VISIBLE_BACKGROUND_LAYERS_BACK_TO_FRONT_WITH_RANGE(i, bg, bg->mImageCount - 1,
                                                              nLayers + (bg->mImageCount -
                                                                         startLayer - 1)) {
      const nsStyleBackground::Layer &layer = bg->mLayers[i];
      if (!aBGClipRect) {
        if (currentBackgroundClip != layer.mClip || !clipSet) {
          currentBackgroundClip = layer.mClip;
          
          
          
          if (clipSet) {
            autoSR.Restore(); 
            GetBackgroundClip(layer, aForFrame,
                              aBorder, aBorderArea, aDirtyRect, (aFlags & PAINTBG_WILL_PAINT_BORDER),
                              appUnitsPerPixel, &clipState);
          }
          SetupBackgroundClip(clipState, ctx, appUnitsPerPixel, &autoSR);
          clipSet = true;
          if (!clipBorderArea.IsEqualEdges(aBorderArea)) {
            
            
            gfxRect clip =
              nsLayoutUtils::RectToGfxRect(aBorderArea, appUnitsPerPixel);
            autoSR.EnsureSaved(ctx);
            ctx->NewPath();
            ctx->SnappedRectangle(clip);
            ctx->Clip();
          }
        }
      }
      if ((aLayer < 0 || i == (uint32_t)startLayer) &&
          !clipState.mDirtyRectGfx.IsEmpty()) {
        nsBackgroundLayerState state = PrepareBackgroundLayer(aPresContext, aForFrame,
            aFlags, paintBorderArea, clipState.mBGClipArea, layer);
        if (!state.mFillArea.IsEmpty()) {
          if (state.mCompositingOp != gfxContext::OPERATOR_OVER) {
            NS_ASSERTION(ctx->CurrentOperator() == gfxContext::OPERATOR_OVER,
                         "It is assumed the initial operator is OPERATOR_OVER, when it is restored later");
            ctx->SetOperator(state.mCompositingOp);
          }

          DrawResult resultForLayer =
            state.mImageRenderer.DrawBackground(aPresContext, aRenderingContext,
                                                state.mDestArea, state.mFillArea,
                                                state.mAnchor + paintBorderArea.TopLeft(),
                                                clipState.mDirtyRect);

          if (result == DrawResult::SUCCESS) {
            result = resultForLayer;
          }

          if (state.mCompositingOp != gfxContext::OPERATOR_OVER) {
            ctx->SetOperator(gfxContext::OPERATOR_OVER);
          }
        }
      }
    }
  }

  return result;
}

static inline bool
IsTransformed(nsIFrame* aForFrame, nsIFrame* aTopFrame)
{
  for (nsIFrame* f = aForFrame; f != aTopFrame; f = f->GetParent()) {
    if (f->IsTransformed()) {
      return true;
    }
  }
  return false;
}

nsRect
nsCSSRendering::ComputeBackgroundPositioningArea(nsPresContext* aPresContext,
                                                 nsIFrame* aForFrame,
                                                 const nsRect& aBorderArea,
                                                 const nsStyleBackground::Layer& aLayer,
                                                 nsIFrame** aAttachedToFrame)
{
  
  
  nsRect bgPositioningArea;

  nsIAtom* frameType = aForFrame->GetType();
  nsIFrame* geometryFrame = aForFrame;
  if (MOZ_UNLIKELY(frameType == nsGkAtoms::scrollFrame &&
                   NS_STYLE_BG_ATTACHMENT_LOCAL == aLayer.mAttachment)) {
    nsIScrollableFrame* scrollableFrame = do_QueryFrame(aForFrame);
    bgPositioningArea = nsRect(
      scrollableFrame->GetScrolledFrame()->GetPosition()
        
        + scrollableFrame->GetScrollRange().TopLeft(),
      scrollableFrame->GetScrolledRect().Size());
    
    
    
    if (aLayer.mOrigin == NS_STYLE_BG_ORIGIN_BORDER) {
      nsMargin border = geometryFrame->GetUsedBorder();
      border.ApplySkipSides(geometryFrame->GetSkipSides());
      bgPositioningArea.Inflate(border);
      bgPositioningArea.Inflate(scrollableFrame->GetActualScrollbarSizes());
    } else if (aLayer.mOrigin != NS_STYLE_BG_ORIGIN_PADDING) {
      nsMargin padding = geometryFrame->GetUsedPadding();
      padding.ApplySkipSides(geometryFrame->GetSkipSides());
      bgPositioningArea.Deflate(padding);
      NS_ASSERTION(aLayer.mOrigin == NS_STYLE_BG_ORIGIN_CONTENT,
                   "unknown background-origin value");
    }
    *aAttachedToFrame = aForFrame;
    return bgPositioningArea;
  }

  if (MOZ_UNLIKELY(frameType == nsGkAtoms::canvasFrame)) {
    geometryFrame = aForFrame->GetFirstPrincipalChild();
    
    
    
    
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
    bgPositioningArea.Deflate(border);
  }

  nsIFrame* attachedToFrame = aForFrame;
  if (NS_STYLE_BG_ATTACHMENT_FIXED == aLayer.mAttachment) {
    
    
    
    attachedToFrame = aPresContext->PresShell()->FrameManager()->GetRootFrame();
    NS_ASSERTION(attachedToFrame, "no root frame");
    nsIFrame* pageContentFrame = nullptr;
    if (aPresContext->IsPaginated()) {
      pageContentFrame =
        nsLayoutUtils::GetClosestFrameOfType(aForFrame, nsGkAtoms::pageContentFrame);
      if (pageContentFrame) {
        attachedToFrame = pageContentFrame;
      }
      
    }

    
    
    bgPositioningArea =
      nsRect(-aForFrame->GetOffsetTo(attachedToFrame), attachedToFrame->GetSize());

    if (!pageContentFrame) {
      
      nsIScrollableFrame* scrollableFrame =
        aPresContext->PresShell()->GetRootScrollFrameAsScrollable();
      if (scrollableFrame) {
        nsMargin scrollbars = scrollableFrame->GetActualScrollbarSizes();
        bgPositioningArea.Deflate(scrollbars);
      }
    }
  }
  *aAttachedToFrame = attachedToFrame;

  return bgPositioningArea;
}





static nsSize
ComputeDrawnSizeForBackground(const CSSSizeOrRatio& aIntrinsicSize,
                              const nsSize& aBgPositioningArea,
                              const nsStyleBackground::Size& aLayerSize)
{
  
  if (aLayerSize.mWidthType == nsStyleBackground::Size::eContain ||
      aLayerSize.mWidthType == nsStyleBackground::Size::eCover) {
    nsImageRenderer::FitType fitType =
      aLayerSize.mWidthType == nsStyleBackground::Size::eCover
        ? nsImageRenderer::COVER
        : nsImageRenderer::CONTAIN;
    return nsImageRenderer::ComputeConstrainedSize(aBgPositioningArea,
                                                   aIntrinsicSize.mRatio,
                                                   fitType);
  }

  
  CSSSizeOrRatio specifiedSize;
  if (aLayerSize.mWidthType == nsStyleBackground::Size::eLengthPercentage) {
    specifiedSize.SetWidth(
      aLayerSize.ResolveWidthLengthPercentage(aBgPositioningArea));
  }
  if (aLayerSize.mHeightType == nsStyleBackground::Size::eLengthPercentage) {
    specifiedSize.SetHeight(
      aLayerSize.ResolveHeightLengthPercentage(aBgPositioningArea));
  }

  return nsImageRenderer::ComputeConcreteSize(specifiedSize,
                                              aIntrinsicSize,
                                              aBgPositioningArea);
}

nsBackgroundLayerState
nsCSSRendering::PrepareBackgroundLayer(nsPresContext* aPresContext,
                                       nsIFrame* aForFrame,
                                       uint32_t aFlags,
                                       const nsRect& aBorderArea,
                                       const nsRect& aBGClipRect,
                                       const nsStyleBackground::Layer& aLayer)
{
  






















































  uint32_t irFlags = 0;
  if (aFlags & nsCSSRendering::PAINTBG_SYNC_DECODE_IMAGES) {
    irFlags |= nsImageRenderer::FLAG_SYNC_DECODE_IMAGES;
  }
  if (aFlags & nsCSSRendering::PAINTBG_TO_WINDOW) {
    irFlags |= nsImageRenderer::FLAG_PAINTING_TO_WINDOW;
  }

  nsBackgroundLayerState state(aForFrame, &aLayer.mImage, irFlags);
  if (!state.mImageRenderer.PrepareImage()) {
    
    return state;
  }

  
  nsIFrame* attachedToFrame = aForFrame;
  
  
  nsRect bgPositioningArea =
    ComputeBackgroundPositioningArea(aPresContext, aForFrame, aBorderArea,
                                     aLayer, &attachedToFrame);

  
  
  nsRect bgClipRect = aBGClipRect;

  
  
  
  
  nsPoint imageTopLeft;
  if (NS_STYLE_BG_ATTACHMENT_FIXED == aLayer.mAttachment) {
    if ((aFlags & nsCSSRendering::PAINTBG_TO_WINDOW) &&
        !IsTransformed(aForFrame, attachedToFrame)) {
      
      
      
      
      
      
      
      bgClipRect.IntersectRect(bgClipRect, bgPositioningArea + aBorderArea.TopLeft());
    }
  }

  
  
  
  CSSSizeOrRatio intrinsicSize = state.mImageRenderer.ComputeIntrinsicSize();
  nsSize bgPositionSize = bgPositioningArea.Size();
  nsSize imageSize = ComputeDrawnSizeForBackground(intrinsicSize,
                                                   bgPositionSize,
                                                   aLayer.mSize);
  if (imageSize.width <= 0 || imageSize.height <= 0)
    return state;

  state.mImageRenderer.SetPreferredSize(intrinsicSize,
                                        imageSize);

  
  
  nsImageRenderer::ComputeObjectAnchorPoint(aLayer.mPosition,
                                            bgPositionSize, imageSize,
                                            &imageTopLeft, &state.mAnchor);
  imageTopLeft += bgPositioningArea.TopLeft();
  state.mAnchor += bgPositioningArea.TopLeft();

  state.mDestArea = nsRect(imageTopLeft + aBorderArea.TopLeft(), imageSize);
  state.mFillArea = state.mDestArea;
  int repeatX = aLayer.mRepeat.mXRepeat;
  int repeatY = aLayer.mRepeat.mYRepeat;
  if (repeatX == NS_STYLE_BG_REPEAT_REPEAT) {
    state.mFillArea.x = bgClipRect.x;
    state.mFillArea.width = bgClipRect.width;
  }
  if (repeatY == NS_STYLE_BG_REPEAT_REPEAT) {
    state.mFillArea.y = bgClipRect.y;
    state.mFillArea.height = bgClipRect.height;
  }
  state.mFillArea.IntersectRect(state.mFillArea, bgClipRect);

  state.mCompositingOp = GetGFXBlendMode(aLayer.mBlendMode);

  return state;
}

nsRect
nsCSSRendering::GetBackgroundLayerRect(nsPresContext* aPresContext,
                                       nsIFrame* aForFrame,
                                       const nsRect& aBorderArea,
                                       const nsRect& aClipRect,
                                       const nsStyleBackground::Layer& aLayer,
                                       uint32_t aFlags)
{
  Sides skipSides = aForFrame->GetSkipSides();
  nsRect borderArea =
    ::BoxDecorationRectForBackground(aForFrame, aBorderArea, skipSides);
  nsBackgroundLayerState state =
      PrepareBackgroundLayer(aPresContext, aForFrame, aFlags, borderArea,
                             aClipRect, aLayer);
  return state.mFillArea;
}

static void
DrawBorderImage(nsPresContext*       aPresContext,
                nsRenderingContext&  aRenderingContext,
                nsIFrame*            aForFrame,
                const nsRect&        aBorderArea,
                const nsStyleBorder& aStyleBorder,
                const nsRect&        aDirtyRect,
                Sides                aSkipSides)
{
  NS_PRECONDITION(aStyleBorder.IsBorderImageLoaded(),
                  "drawing border image that isn't successfully loaded");

  if (aDirtyRect.IsEmpty())
    return;

  nsImageRenderer renderer(aForFrame, &aStyleBorder.mBorderImageSource, 0);

  
  
  
  
  
  
  aForFrame->AssociateImage(aStyleBorder.mBorderImageSource, aPresContext);

  if (!renderer.PrepareImage()) {
    return;
  }

  
  
  gfxContextAutoSaveRestore autoSR;

  
  
  
  
  nsRect borderImgArea;
  nsMargin borderWidths(aStyleBorder.GetComputedBorder());
  nsMargin imageOutset(aStyleBorder.GetImageOutset());
  if (::IsBoxDecorationSlice(aStyleBorder) && !aSkipSides.IsEmpty()) {
    borderImgArea = ::BoxDecorationRectForBorder(aForFrame, aBorderArea,
                                                 aSkipSides, &aStyleBorder);
    if (borderImgArea.IsEqualEdges(aBorderArea)) {
      
      borderWidths.ApplySkipSides(aSkipSides);
      imageOutset.ApplySkipSides(aSkipSides);
      borderImgArea.Inflate(imageOutset);
    } else {
      
      
      borderImgArea.Inflate(imageOutset);
      imageOutset.ApplySkipSides(aSkipSides);
      nsRect clip = aBorderArea;
      clip.Inflate(imageOutset);
      autoSR.EnsureSaved(aRenderingContext.ThebesContext());
      aRenderingContext.ThebesContext()->
        Clip(NSRectToSnappedRect(clip,
                                 aForFrame->PresContext()->AppUnitsPerDevPixel(),
                                 *aRenderingContext.GetDrawTarget()));
    }
  } else {
    borderImgArea = aBorderArea;
    borderImgArea.Inflate(imageOutset);
  }

  
  CSSSizeOrRatio intrinsicSize = renderer.ComputeIntrinsicSize();
  nsSize imageSize = nsImageRenderer::ComputeConcreteSize(CSSSizeOrRatio(),
                                                          intrinsicSize,
                                                          borderImgArea.Size());
  renderer.SetPreferredSize(intrinsicSize, imageSize);

  
  
  nsMargin slice;
  nsMargin border;
  NS_FOR_CSS_SIDES(s) {
    nsStyleCoord coord = aStyleBorder.mBorderImageSlice.Get(s);
    int32_t imgDimension = NS_SIDE_IS_VERTICAL(s)
                           ? imageSize.width : imageSize.height;
    nscoord borderDimension = NS_SIDE_IS_VERTICAL(s)
                           ? borderImgArea.width : borderImgArea.height;
    double value;
    switch (coord.GetUnit()) {
      case eStyleUnit_Percent:
        value = coord.GetPercentValue() * imgDimension;
        break;
      case eStyleUnit_Factor:
        value = nsPresContext::CSSPixelsToAppUnits(
          NS_lround(coord.GetFactorValue()));
        break;
      default:
        NS_NOTREACHED("unexpected CSS unit for image slice");
        value = 0;
        break;
    }
    if (value < 0)
      value = 0;
    if (value > imgDimension)
      value = imgDimension;
    slice.Side(s) = value;

    coord = aStyleBorder.mBorderImageWidth.Get(s);
    switch (coord.GetUnit()) {
      case eStyleUnit_Coord: 
        value = coord.GetCoordValue();
        break;
      case eStyleUnit_Percent:
        value = coord.GetPercentValue() * borderDimension;
        break;
      case eStyleUnit_Factor:
        value = coord.GetFactorValue() * borderWidths.Side(s);
        break;
      case eStyleUnit_Auto:  
        value = slice.Side(s);
        break;
      default:
        NS_NOTREACHED("unexpected CSS unit for border image area division");
        value = 0;
        break;
    }
    
    
    MOZ_ASSERT(value >= 0);
    border.Side(s) = NSToCoordRoundWithClamp(value);
    MOZ_ASSERT(border.Side(s) >= 0);
  }

  
  
  
  uint32_t combinedBorderWidth = uint32_t(border.left) +
                                 uint32_t(border.right);
  double scaleX = combinedBorderWidth > uint32_t(borderImgArea.width)
                  ? borderImgArea.width / double(combinedBorderWidth)
                  : 1.0;
  uint32_t combinedBorderHeight = uint32_t(border.top) +
                                  uint32_t(border.bottom);
  double scaleY = combinedBorderHeight > uint32_t(borderImgArea.height)
                  ? borderImgArea.height / double(combinedBorderHeight)
                  : 1.0;
  double scale = std::min(scaleX, scaleY);
  if (scale < 1.0) {
    border.left *= scale;
    border.right *= scale;
    border.top *= scale;
    border.bottom *= scale;
    NS_ASSERTION(border.left + border.right <= borderImgArea.width &&
                 border.top + border.bottom <= borderImgArea.height,
                 "rounding error in width reduction???");
  }

  
  
  
  
  enum {
    LEFT, MIDDLE, RIGHT,
    TOP = LEFT, BOTTOM = RIGHT
  };
  const nscoord borderX[3] = {
    borderImgArea.x + 0,
    borderImgArea.x + border.left,
    borderImgArea.x + borderImgArea.width - border.right,
  };
  const nscoord borderY[3] = {
    borderImgArea.y + 0,
    borderImgArea.y + border.top,
    borderImgArea.y + borderImgArea.height - border.bottom,
  };
  const nscoord borderWidth[3] = {
    border.left,
    borderImgArea.width - border.left - border.right,
    border.right,
  };
  const nscoord borderHeight[3] = {
    border.top,
    borderImgArea.height - border.top - border.bottom,
    border.bottom,
  };
  const int32_t sliceX[3] = {
    0,
    slice.left,
    imageSize.width - slice.right,
  };
  const int32_t sliceY[3] = {
    0,
    slice.top,
    imageSize.height - slice.bottom,
  };
  const int32_t sliceWidth[3] = {
    slice.left,
    std::max(imageSize.width - slice.left - slice.right, 0),
    slice.right,
  };
  const int32_t sliceHeight[3] = {
    slice.top,
    std::max(imageSize.height - slice.top - slice.bottom, 0),
    slice.bottom,
  };

  for (int i = LEFT; i <= RIGHT; i++) {
    for (int j = TOP; j <= BOTTOM; j++) {
      uint8_t fillStyleH, fillStyleV;
      nsSize unitSize;

      if (i == MIDDLE && j == MIDDLE) {
        
        if (NS_STYLE_BORDER_IMAGE_SLICE_NOFILL ==
            aStyleBorder.mBorderImageFill) {
          continue;
        }

        NS_ASSERTION(NS_STYLE_BORDER_IMAGE_SLICE_FILL ==
                     aStyleBorder.mBorderImageFill,
                     "Unexpected border image fill");

        
        
        
        
        
        
        
        
        
        gfxFloat hFactor, vFactor;

        if (0 < border.left && 0 < slice.left)
          vFactor = gfxFloat(border.left)/slice.left;
        else if (0 < border.right && 0 < slice.right)
          vFactor = gfxFloat(border.right)/slice.right;
        else
          vFactor = 1;

        if (0 < border.top && 0 < slice.top)
          hFactor = gfxFloat(border.top)/slice.top;
        else if (0 < border.bottom && 0 < slice.bottom)
          hFactor = gfxFloat(border.bottom)/slice.bottom;
        else
          hFactor = 1;

        unitSize.width = sliceWidth[i]*hFactor;
        unitSize.height = sliceHeight[j]*vFactor;
        fillStyleH = aStyleBorder.mBorderImageRepeatH;
        fillStyleV = aStyleBorder.mBorderImageRepeatV;

      } else if (i == MIDDLE) { 
        
        
        gfxFloat factor;
        if (0 < borderHeight[j] && 0 < sliceHeight[j])
          factor = gfxFloat(borderHeight[j])/sliceHeight[j];
        else
          factor = 1;

        unitSize.width = sliceWidth[i]*factor;
        unitSize.height = borderHeight[j];
        fillStyleH = aStyleBorder.mBorderImageRepeatH;
        fillStyleV = NS_STYLE_BORDER_IMAGE_REPEAT_STRETCH;

      } else if (j == MIDDLE) { 
        gfxFloat factor;
        if (0 < borderWidth[i] && 0 < sliceWidth[i])
          factor = gfxFloat(borderWidth[i])/sliceWidth[i];
        else
          factor = 1;

        unitSize.width = borderWidth[i];
        unitSize.height = sliceHeight[j]*factor;
        fillStyleH = NS_STYLE_BORDER_IMAGE_REPEAT_STRETCH;
        fillStyleV = aStyleBorder.mBorderImageRepeatV;

      } else {
        
        unitSize.width = borderWidth[i];
        unitSize.height = borderHeight[j];
        fillStyleH = NS_STYLE_BORDER_IMAGE_REPEAT_STRETCH;
        fillStyleV = NS_STYLE_BORDER_IMAGE_REPEAT_STRETCH;
      }

      nsRect destArea(borderX[i], borderY[j], borderWidth[i], borderHeight[j]);
      nsRect subArea(sliceX[i], sliceY[j], sliceWidth[i], sliceHeight[j]);
      nsIntRect intSubArea = subArea.ToOutsidePixels(nsPresContext::AppUnitsPerCSSPixel());

      renderer.DrawBorderImageComponent(aPresContext,
                                        aRenderingContext, aDirtyRect,
                                        destArea, CSSIntRect(intSubArea.x,
                                                             intSubArea.y,
                                                             intSubArea.width,
                                                             intSubArea.height),
                                        fillStyleH, fillStyleV,
                                        unitSize, j * (RIGHT + 1) + i);
    }
  }
}





static nscoord
RoundIntToPixel(nscoord aValue,
                nscoord aTwipsPerPixel,
                bool    aRoundDown = false)
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
                  bool    aRoundDown = false)
{
  return RoundIntToPixel(NSToCoordRound(aValue), aTwipsPerPixel, aRoundDown);
}

static void SetPoly(const Rect& aRect, Point* poly)
{
  poly[0].x = aRect.x;
  poly[0].y = aRect.y;
  poly[1].x = aRect.x + aRect.width;
  poly[1].y = aRect.y;
  poly[2].x = aRect.x + aRect.width;
  poly[2].y = aRect.y + aRect.height;
  poly[3].x = aRect.x;
  poly[3].y = aRect.y + aRect.height;
}

static void
DrawSolidBorderSegment(nsRenderingContext& aContext,
                       nsRect               aRect,
                       nscolor              aColor,
                       int32_t              aAppUnitsPerDevPixel,
                       nscoord              aTwipsPerPixel,
                       uint8_t              aStartBevelSide = 0,
                       nscoord              aStartBevelOffset = 0,
                       uint8_t              aEndBevelSide = 0,
                       nscoord              aEndBevelOffset = 0)
{
  DrawTarget* drawTarget = aContext.GetDrawTarget();

  ColorPattern color(ToDeviceColor(aColor));
  DrawOptions drawOptions(1.f, CompositionOp::OP_OVER, AntialiasMode::NONE);

  if ((aRect.width == aTwipsPerPixel) || (aRect.height == aTwipsPerPixel) ||
      ((0 == aStartBevelOffset) && (0 == aEndBevelOffset))) {
    
    if ((NS_SIDE_TOP == aStartBevelSide) || (NS_SIDE_BOTTOM == aStartBevelSide)) {
      if (1 == aRect.height)
        StrokeLineWithSnapping(aRect.TopLeft(), aRect.BottomLeft(),
                               aAppUnitsPerDevPixel, *drawTarget,
                               color, StrokeOptions(), drawOptions);
      else
        drawTarget->FillRect(NSRectToSnappedRect(aRect, aAppUnitsPerDevPixel,
                                                 *drawTarget),
                             color, drawOptions);
    }
    else {
      if (1 == aRect.width)
        StrokeLineWithSnapping(aRect.TopLeft(), aRect.TopRight(),
                               aAppUnitsPerDevPixel, *drawTarget,
                               color, StrokeOptions(), drawOptions);
      else
        drawTarget->FillRect(NSRectToSnappedRect(aRect, aAppUnitsPerDevPixel,
                                                 *drawTarget),
                             color, drawOptions);
    }
  }
  else {
    
    Point poly[4];
    SetPoly(NSRectToSnappedRect(aRect, aAppUnitsPerDevPixel, *drawTarget),
            poly);

    Float startBevelOffset =
      NSAppUnitsToFloatPixels(aStartBevelOffset, aAppUnitsPerDevPixel);
    switch(aStartBevelSide) {
    case NS_SIDE_TOP:
      poly[0].x += startBevelOffset;
      break;
    case NS_SIDE_BOTTOM:
      poly[3].x += startBevelOffset;
      break;
    case NS_SIDE_RIGHT:
      poly[1].y += startBevelOffset;
      break;
    case NS_SIDE_LEFT:
      poly[0].y += startBevelOffset;
    }

    Float endBevelOffset =
      NSAppUnitsToFloatPixels(aEndBevelOffset, aAppUnitsPerDevPixel);
    switch(aEndBevelSide) {
    case NS_SIDE_TOP:
      poly[1].x -= endBevelOffset;
      break;
    case NS_SIDE_BOTTOM:
      poly[2].x -= endBevelOffset;
      break;
    case NS_SIDE_RIGHT:
      poly[2].y -= endBevelOffset;
      break;
    case NS_SIDE_LEFT:
      poly[3].y -= endBevelOffset;
    }

    RefPtr<PathBuilder> builder = drawTarget->CreatePathBuilder();
    builder->MoveTo(poly[0]);
    builder->LineTo(poly[1]);
    builder->LineTo(poly[2]);
    builder->LineTo(poly[3]);
    builder->Close();
    RefPtr<Path> path = builder->Finish();
    drawTarget->Fill(path, color, drawOptions);
  }
}

static void
GetDashInfo(nscoord  aBorderLength,
            nscoord  aDashLength,
            nscoord  aTwipsPerPixel,
            int32_t& aNumDashSpaces,
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
                                       uint8_t                  aBorderStyle,
                                       nscolor                  aBorderColor,
                                       const nsStyleBackground* aBGColor,
                                       const nsRect&            aBorder,
                                       int32_t                  aAppUnitsPerDevPixel,
                                       int32_t                  aAppUnitsPerCSSPixel,
                                       uint8_t                  aStartBevelSide,
                                       nscoord                  aStartBevelOffset,
                                       uint8_t                  aEndBevelSide,
                                       nscoord                  aEndBevelOffset)
{
  bool horizontal = ((NS_SIDE_TOP == aStartBevelSide) || (NS_SIDE_BOTTOM == aStartBevelSide));
  nscoord twipsPerPixel = NSIntPixelsToAppUnits(1, aAppUnitsPerCSSPixel);
  uint8_t ridgeGroove = NS_STYLE_BORDER_STYLE_RIDGE;

  if ((twipsPerPixel >= aBorder.width) || (twipsPerPixel >= aBorder.height) ||
      (NS_STYLE_BORDER_STYLE_DASHED == aBorderStyle) || (NS_STYLE_BORDER_STYLE_DOTTED == aBorderStyle)) {
    
    aStartBevelOffset = 0;
    aEndBevelOffset = 0;
  }

  gfxContext *ctx = aContext.ThebesContext();
  AntialiasMode oldMode = ctx->CurrentAntialiasMode();
  ctx->SetAntialiasMode(AntialiasMode::NONE);

  ctx->SetColor(aBorderColor);

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
      minDashLength = std::max(minDashLength, twipsPerPixel);
      nscoord numDashSpaces = 0;
      nscoord startDashLength = minDashLength;
      nscoord endDashLength   = minDashLength;
      if (horizontal) {
        GetDashInfo(aBorder.width, dashLength, twipsPerPixel, numDashSpaces, startDashLength, endDashLength);
        nsRect rect(aBorder.x, aBorder.y, startDashLength, aBorder.height);
        DrawSolidBorderSegment(aContext, rect, aBorderColor, aAppUnitsPerDevPixel, twipsPerPixel);
        for (int32_t spaceX = 0; spaceX < numDashSpaces; spaceX++) {
          rect.x += rect.width + dashLength;
          rect.width = (spaceX == (numDashSpaces - 1)) ? endDashLength : dashLength;
          DrawSolidBorderSegment(aContext, rect, aBorderColor, aAppUnitsPerDevPixel, twipsPerPixel);
        }
      }
      else {
        GetDashInfo(aBorder.height, dashLength, twipsPerPixel, numDashSpaces, startDashLength, endDashLength);
        nsRect rect(aBorder.x, aBorder.y, aBorder.width, startDashLength);
        DrawSolidBorderSegment(aContext, rect, aBorderColor, aAppUnitsPerDevPixel, twipsPerPixel);
        for (int32_t spaceY = 0; spaceY < numDashSpaces; spaceY++) {
          rect.y += rect.height + dashLength;
          rect.height = (spaceY == (numDashSpaces - 1)) ? endDashLength : dashLength;
          DrawSolidBorderSegment(aContext, rect, aBorderColor, aAppUnitsPerDevPixel, twipsPerPixel);
        }
      }
    }
    break;
  case NS_STYLE_BORDER_STYLE_GROOVE:
    ridgeGroove = NS_STYLE_BORDER_STYLE_GROOVE; 
  case NS_STYLE_BORDER_STYLE_RIDGE:
    if ((horizontal && (twipsPerPixel >= aBorder.height)) ||
        (!horizontal && (twipsPerPixel >= aBorder.width))) {
      
      DrawSolidBorderSegment(aContext, aBorder, aBorderColor, aAppUnitsPerDevPixel, twipsPerPixel,
                             aStartBevelSide, aStartBevelOffset,
                             aEndBevelSide, aEndBevelOffset);
    }
    else {
      nscoord startBevel = (aStartBevelOffset > 0)
                            ? RoundFloatToPixel(0.5f * (float)aStartBevelOffset, twipsPerPixel, true) : 0;
      nscoord endBevel =   (aEndBevelOffset > 0)
                            ? RoundFloatToPixel(0.5f * (float)aEndBevelOffset, twipsPerPixel, true) : 0;
      mozilla::css::Side ridgeGrooveSide = (horizontal) ? NS_SIDE_TOP : NS_SIDE_LEFT;
      
      
      ctx->SetColor(
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
        DrawSolidBorderSegment(aContext, rect, aBorderColor, aAppUnitsPerDevPixel, twipsPerPixel, aStartBevelSide,
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
        DrawSolidBorderSegment(aContext, rect, aBorderColor, aAppUnitsPerDevPixel, twipsPerPixel, aStartBevelSide,
                               startBevel, aEndBevelSide, endBevel);
      }

      rect = aBorder;
      ridgeGrooveSide = (NS_SIDE_TOP == ridgeGrooveSide) ? NS_SIDE_BOTTOM : NS_SIDE_RIGHT;
      
      
      ctx->SetColor(
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
        DrawSolidBorderSegment(aContext, rect, aBorderColor, aAppUnitsPerDevPixel, twipsPerPixel, aStartBevelSide,
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
        DrawSolidBorderSegment(aContext, rect, aBorderColor, aAppUnitsPerDevPixel, twipsPerPixel, aStartBevelSide,
                               startBevel, aEndBevelSide, endBevel);
      }
    }
    break;
  case NS_STYLE_BORDER_STYLE_DOUBLE:
    
    
    
    if ((aBorder.width > 2*twipsPerPixel || horizontal) &&
        (aBorder.height > 2*twipsPerPixel || !horizontal)) {
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
        DrawSolidBorderSegment(aContext, topRect, aBorderColor, aAppUnitsPerDevPixel, twipsPerPixel, aStartBevelSide,
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
        DrawSolidBorderSegment(aContext, bottomRect, aBorderColor, aAppUnitsPerDevPixel, twipsPerPixel, aStartBevelSide,
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
        DrawSolidBorderSegment(aContext, leftRect, aBorderColor, aAppUnitsPerDevPixel, twipsPerPixel, aStartBevelSide,
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
        DrawSolidBorderSegment(aContext, rightRect, aBorderColor, aAppUnitsPerDevPixel, twipsPerPixel, aStartBevelSide,
                               startBevel, aEndBevelSide, endBevel);
      }
      break;
    }
    
  case NS_STYLE_BORDER_STYLE_SOLID:
    DrawSolidBorderSegment(aContext, aBorder, aBorderColor, aAppUnitsPerDevPixel, twipsPerPixel, aStartBevelSide,
                           aStartBevelOffset, aEndBevelSide, aEndBevelOffset);
    break;
  case NS_STYLE_BORDER_STYLE_OUTSET:
  case NS_STYLE_BORDER_STYLE_INSET:
    NS_ASSERTION(false, "inset, outset should have been converted to groove, ridge");
    break;
  case NS_STYLE_BORDER_STYLE_AUTO:
    NS_ASSERTION(false, "Unexpected 'auto' table border");
    break;
  }

  ctx->SetAntialiasMode(oldMode);
}



Rect
nsCSSRendering::ExpandPaintingRectForDecorationLine(
                  nsIFrame* aFrame,
                  const uint8_t aStyle,
                  const Rect& aClippedRect,
                  const Float aICoordInFrame,
                  const Float aCycleLength,
                  bool aVertical)
{
  switch (aStyle) {
    case NS_STYLE_TEXT_DECORATION_STYLE_DOTTED:
    case NS_STYLE_TEXT_DECORATION_STYLE_DASHED:
    case NS_STYLE_TEXT_DECORATION_STYLE_WAVY:
      break;
    default:
      NS_ERROR("Invalid style was specified");
      return aClippedRect;
  }

  nsBlockFrame* block = nullptr;
  
  
  nscoord framePosInBlockAppUnits = 0;
  for (nsIFrame* f = aFrame; f; f = f->GetParent()) {
    block = do_QueryFrame(f);
    if (block) {
      break;
    }
    framePosInBlockAppUnits += aVertical ?
      f->GetNormalPosition().y : f->GetNormalPosition().x;
  }

  NS_ENSURE_TRUE(block, aClippedRect);

  nsPresContext *pc = aFrame->PresContext();
  Float framePosInBlock = Float(pc->AppUnitsToGfxUnits(framePosInBlockAppUnits));
  int32_t rectPosInBlock =
    int32_t(NS_round(framePosInBlock + aICoordInFrame));
  int32_t extraStartEdge =
    rectPosInBlock - (rectPosInBlock / int32_t(aCycleLength) * aCycleLength);
  Rect rect(aClippedRect);
  if (aVertical) {
    rect.y -= extraStartEdge;
    rect.height += extraStartEdge;
  } else {
    rect.x -= extraStartEdge;
    rect.width += extraStartEdge;
  }
  return rect;
}

void
nsCSSRendering::PaintDecorationLine(nsIFrame* aFrame,
                                    DrawTarget& aDrawTarget,
                                    const Rect& aDirtyRect,
                                    const nscolor aColor,
                                    const gfxPoint& aPt,
                                    const Float aICoordInFrame,
                                    const gfxSize& aLineSize,
                                    const gfxFloat aAscent,
                                    const gfxFloat aOffset,
                                    const uint8_t aDecoration,
                                    const uint8_t aStyle,
                                    bool aVertical,
                                    const gfxFloat aDescentLimit)
{
  NS_ASSERTION(aStyle != NS_STYLE_TEXT_DECORATION_STYLE_NONE, "aStyle is none");

  Rect rect = ToRect(
    GetTextDecorationRectInternal(aPt, aLineSize, aAscent, aOffset,
                                  aDecoration, aStyle, aVertical,
                                  aDescentLimit));
  if (rect.IsEmpty() || !rect.Intersects(aDirtyRect)) {
    return;
  }

  if (aDecoration != NS_STYLE_TEXT_DECORATION_LINE_UNDERLINE &&
      aDecoration != NS_STYLE_TEXT_DECORATION_LINE_OVERLINE &&
      aDecoration != NS_STYLE_TEXT_DECORATION_LINE_LINE_THROUGH) {
    NS_ERROR("Invalid decoration value!");
    return;
  }

  Float lineThickness = std::max(NS_round(aLineSize.height), 1.0);

  ColorPattern color(ToDeviceColor(aColor));
  StrokeOptions strokeOptions(lineThickness);
  DrawOptions drawOptions;

  Float dash[2];

  AutoPopClips autoPopClips(&aDrawTarget);

  switch (aStyle) {
    case NS_STYLE_TEXT_DECORATION_STYLE_SOLID:
    case NS_STYLE_TEXT_DECORATION_STYLE_DOUBLE:
      break;
    case NS_STYLE_TEXT_DECORATION_STYLE_DASHED: {
      autoPopClips.PushClipRect(rect);
      Float dashWidth = lineThickness * DOT_LENGTH * DASH_LENGTH;
      dash[0] = dashWidth;
      dash[1] = dashWidth;
      strokeOptions.mDashPattern = dash;
      strokeOptions.mDashLength = MOZ_ARRAY_LENGTH(dash);
      strokeOptions.mLineCap = CapStyle::BUTT;
      rect = ExpandPaintingRectForDecorationLine(aFrame, aStyle, rect,
                                                 aICoordInFrame,
                                                 dashWidth * 2,
                                                 aVertical);
      
      rect.width += dashWidth;
      break;
    }
    case NS_STYLE_TEXT_DECORATION_STYLE_DOTTED: {
      autoPopClips.PushClipRect(rect);
      Float dashWidth = lineThickness * DOT_LENGTH;
      if (lineThickness > 2.0) {
        dash[0] = 0.f;
        dash[1] = dashWidth * 2.f;
        strokeOptions.mLineCap = CapStyle::ROUND;
      } else {
        dash[0] = dashWidth;
        dash[1] = dashWidth;
      }
      strokeOptions.mDashPattern = dash;
      strokeOptions.mDashLength = MOZ_ARRAY_LENGTH(dash);
      rect = ExpandPaintingRectForDecorationLine(aFrame, aStyle, rect,
                                                 aICoordInFrame,
                                                 dashWidth * 2,
                                                 aVertical);
      
      rect.width += dashWidth;
      break;
    }
    case NS_STYLE_TEXT_DECORATION_STYLE_WAVY:
      autoPopClips.PushClipRect(rect);
      if (lineThickness > 2.0) {
        drawOptions.mAntialiasMode = AntialiasMode::SUBPIXEL;
      } else {
        
        
        
        drawOptions.mAntialiasMode = AntialiasMode::NONE;
      }
      break;
    default:
      NS_ERROR("Invalid style value!");
      return;
  }

  
  if (aVertical) {
    rect.x += lineThickness / 2;
  } else {
    rect.y += lineThickness / 2;
  }

  switch (aStyle) {
    case NS_STYLE_TEXT_DECORATION_STYLE_SOLID:
    case NS_STYLE_TEXT_DECORATION_STYLE_DOTTED:
    case NS_STYLE_TEXT_DECORATION_STYLE_DASHED: {
      Point p1 = rect.TopLeft();
      Point p2 = aVertical ? rect.BottomLeft() : rect.TopRight();
      aDrawTarget.StrokeLine(p1, p2, color, strokeOptions, drawOptions);
      return;
    }
    case NS_STYLE_TEXT_DECORATION_STYLE_DOUBLE: {
      













      Point p1 = rect.TopLeft();
      Point p2 = aVertical ? rect.BottomLeft() : rect.TopRight();
      aDrawTarget.StrokeLine(p1, p2, color, strokeOptions, drawOptions);

      if (aVertical) {
        rect.width -= lineThickness;
      } else {
        rect.height -= lineThickness;
      }

      p1 = aVertical ? rect.TopRight() : rect.BottomLeft();
      p2 = rect.BottomRight();
      aDrawTarget.StrokeLine(p1, p2, color, strokeOptions, drawOptions);
      return;
    }
    case NS_STYLE_TEXT_DECORATION_STYLE_WAVY: {
      































      Float& rectICoord = aVertical ? rect.y : rect.x;
      Float& rectISize = aVertical ? rect.height : rect.width;
      const Float rectBSize = aVertical ? rect.width : rect.height;

      const Float adv = rectBSize - lineThickness;
      const Float flatLengthAtVertex =
        std::max((lineThickness - 1.0) * 2.0, 1.0);

      
      const Float cycleLength = 2 * (adv + flatLengthAtVertex);
      rect = ExpandPaintingRectForDecorationLine(aFrame, aStyle, rect,
                                                 aICoordInFrame, cycleLength,
                                                 aVertical);
      
      
      
      const Float dirtyRectICoord = aVertical ? aDirtyRect.y : aDirtyRect.x;
      int32_t skipCycles = floor((dirtyRectICoord - rectICoord) / cycleLength);
      if (skipCycles > 0) {
        rectICoord += skipCycles * cycleLength;
        rectISize -= skipCycles * cycleLength;
      }

      rectICoord += lineThickness / 2.0;
      Point pt(rect.TopLeft());
      Float& ptICoord = aVertical ? pt.y : pt.x;
      Float& ptBCoord = aVertical ? pt.x : pt.y;
      if (aVertical) {
        ptBCoord += adv + lineThickness / 2.0;
      }
      Float iCoordLimit = ptICoord + rectISize + lineThickness;

      const Float dirtyRectIMost = aVertical ?
        aDirtyRect.YMost() : aDirtyRect.XMost();
      skipCycles = floor((iCoordLimit - dirtyRectIMost) / cycleLength);
      if (skipCycles > 0) {
        iCoordLimit -= skipCycles * cycleLength;
      }

      RefPtr<PathBuilder> builder = aDrawTarget.CreatePathBuilder();
      RefPtr<Path> path;

      ptICoord -= lineThickness;
      builder->MoveTo(pt); 

      ptICoord = rectICoord;
      builder->LineTo(pt); 

      
      
      
      bool goDown = aVertical ? false : true;
      uint32_t iter = 0;
      while (ptICoord < iCoordLimit) {
        if (++iter > 1000) {
          
          
          path = builder->Finish();
          aDrawTarget.Stroke(path, color, strokeOptions, drawOptions);
          builder = aDrawTarget.CreatePathBuilder();
          builder->MoveTo(pt);
          iter = 0;
        }
        ptICoord += adv;
        ptBCoord += goDown ? adv : -adv;

        builder->LineTo(pt); 

        ptICoord += flatLengthAtVertex;
        builder->LineTo(pt); 

        goDown = !goDown;
      }
      path = builder->Finish();
      aDrawTarget.Stroke(path, color, strokeOptions, drawOptions);
      return;
    }
    default:
      NS_ERROR("Invalid style value!");
  }
}

Rect
nsCSSRendering::DecorationLineToPath(const Rect& aDirtyRect,
                                     const Point& aPt,
                                     const Size& aLineSize,
                                     const Float aAscent,
                                     const Float aOffset,
                                     const uint8_t aDecoration,
                                     const uint8_t aStyle,
                                     bool aVertical,
                                     const Float aDescentLimit)
{
  NS_ASSERTION(aStyle != NS_STYLE_TEXT_DECORATION_STYLE_NONE, "aStyle is none");

  Rect path; 

  Rect rect = ToRect(
    GetTextDecorationRectInternal(ThebesPoint(aPt), ThebesSize(aLineSize),
                                  aAscent, aOffset,
                                  aDecoration, aStyle, aVertical,
                                  aDescentLimit));
  if (rect.IsEmpty() || !rect.Intersects(aDirtyRect)) {
    return path;
  }

  if (aDecoration != NS_STYLE_TEXT_DECORATION_LINE_UNDERLINE &&
      aDecoration != NS_STYLE_TEXT_DECORATION_LINE_OVERLINE &&
      aDecoration != NS_STYLE_TEXT_DECORATION_LINE_LINE_THROUGH) {
    NS_ERROR("Invalid decoration value!");
    return path;
  }

  if (aStyle != NS_STYLE_TEXT_DECORATION_STYLE_SOLID) {
    
    return path;
  }

  Float lineThickness = std::max(NS_round(aLineSize.height), 1.0);

  
  if (aVertical) {
    rect.x += lineThickness / 2;
    path = Rect(rect.TopLeft() - Point(lineThickness / 2, 0.0),
                Size(lineThickness, rect.Height()));
  } else {
    rect.y += lineThickness / 2;
    path = Rect(rect.TopLeft() - Point(0.0, lineThickness / 2),
                Size(rect.Width(), lineThickness));
  }

  return path;
}

nsRect
nsCSSRendering::GetTextDecorationRect(nsPresContext* aPresContext,
                                      const gfxSize& aLineSize,
                                      const gfxFloat aAscent,
                                      const gfxFloat aOffset,
                                      const uint8_t aDecoration,
                                      const uint8_t aStyle,
                                      bool aVertical,
                                      const gfxFloat aDescentLimit)
{
  NS_ASSERTION(aPresContext, "aPresContext is null");
  NS_ASSERTION(aStyle != NS_STYLE_TEXT_DECORATION_STYLE_NONE, "aStyle is none");

  gfxRect rect =
    GetTextDecorationRectInternal(gfxPoint(0, 0), aLineSize, aAscent, aOffset,
                                  aDecoration, aStyle, aVertical,
                                  aDescentLimit);
  
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
                                              const uint8_t aDecoration,
                                              const uint8_t aStyle,
                                              bool aVertical,
                                              const gfxFloat aDescentLimit)
{
  NS_ASSERTION(aStyle <= NS_STYLE_TEXT_DECORATION_STYLE_WAVY,
               "Invalid aStyle value");

  if (aStyle == NS_STYLE_TEXT_DECORATION_STYLE_NONE)
    return gfxRect(0, 0, 0, 0);

  bool canLiftUnderline = aDescentLimit >= 0.0;

  gfxFloat iCoord = aVertical ? aPt.y : aPt.x;
  gfxFloat bCoord = aVertical ? aPt.x : aPt.y;

  
  
  
  
  const gfxFloat left  = floor(iCoord + 0.5),
                 right = floor(iCoord + aLineSize.width + 0.5);

  
  
  gfxRect r(left, 0, right - left, 0);

  gfxFloat lineThickness = NS_round(aLineSize.height);
  lineThickness = std::max(lineThickness, 1.0);

  gfxFloat ascent = NS_round(aAscent);
  gfxFloat descentLimit = floor(aDescentLimit);

  gfxFloat suggestedMaxRectHeight = std::max(std::min(ascent, descentLimit), 1.0);
  r.height = lineThickness;
  if (aStyle == NS_STYLE_TEXT_DECORATION_STYLE_DOUBLE) {
    














    gfxFloat gap = NS_round(lineThickness / 2.0);
    gap = std::max(gap, 1.0);
    r.height = lineThickness * 2.0 + gap;
    if (canLiftUnderline) {
      if (r.Height() > suggestedMaxRectHeight) {
        
        
        r.height = std::max(suggestedMaxRectHeight, lineThickness * 2.0 + 1.0);
      }
    }
  } else if (aStyle == NS_STYLE_TEXT_DECORATION_STYLE_WAVY) {
    












    r.height = lineThickness > 2.0 ? lineThickness * 4.0 : lineThickness * 3.0;
    if (canLiftUnderline) {
      if (r.Height() > suggestedMaxRectHeight) {
        
        
        
        
        r.height = std::max(suggestedMaxRectHeight, lineThickness * 2.0);
      }
    }
  }

  gfxFloat baseline = floor(bCoord + aAscent + 0.5);
  gfxFloat offset = 0.0;
  switch (aDecoration) {
    case NS_STYLE_TEXT_DECORATION_LINE_UNDERLINE:
      offset = aOffset;
      if (canLiftUnderline) {
        if (descentLimit < -offset + r.Height()) {
          
          
          
          
          gfxFloat offsetBottomAligned = -descentLimit + r.Height();
          gfxFloat offsetTopAligned = 0.0;
          offset = std::min(offsetBottomAligned, offsetTopAligned);
        }
      }
      break;
    case NS_STYLE_TEXT_DECORATION_LINE_OVERLINE:
      offset = aOffset - lineThickness + r.Height();
      break;
    case NS_STYLE_TEXT_DECORATION_LINE_LINE_THROUGH: {
      gfxFloat extra = floor(r.Height() / 2.0 + 0.5);
      extra = std::max(extra, lineThickness);
      offset = aOffset - lineThickness + extra;
      break;
    }
    default:
      NS_ERROR("Invalid decoration value!");
  }

  if (aVertical) {
    r.y = baseline + floor(aOffset + 0.5); 
                                           
    Swap(r.x, r.y);
    Swap(r.width, r.height);
  } else {
    r.y = baseline - floor(aOffset + 0.5);
  }

  return r;
}




nsImageRenderer::nsImageRenderer(nsIFrame* aForFrame,
                                 const nsStyleImage* aImage,
                                 uint32_t aFlags)
  : mForFrame(aForFrame)
  , mImage(aImage)
  , mType(aImage->GetType())
  , mImageContainer(nullptr)
  , mGradientData(nullptr)
  , mPaintServerFrame(nullptr)
  , mIsReady(false)
  , mSize(0, 0)
  , mFlags(aFlags)
{
}

nsImageRenderer::~nsImageRenderer()
{
}

bool
nsImageRenderer::PrepareImage()
{
  if (mImage->IsEmpty())
    return false;

  if (!mImage->IsComplete()) {
    
    mImage->StartDecoding();

    
    if (!mImage->IsComplete()) {
      
      
      
      
      nsCOMPtr<imgIContainer> img;
      if (!((mFlags & FLAG_SYNC_DECODE_IMAGES) &&
            (mType == eStyleImageType_Image) &&
            (NS_SUCCEEDED(mImage->GetImageData()->GetImage(getter_AddRefs(img))))))
        return false;
    }
  }

  switch (mType) {
    case eStyleImageType_Image:
    {
      nsCOMPtr<imgIContainer> srcImage;
      DebugOnly<nsresult> rv =
        mImage->GetImageData()->GetImage(getter_AddRefs(srcImage));
      MOZ_ASSERT(NS_SUCCEEDED(rv) && srcImage,
                 "If GetImage() is failing, mImage->IsComplete() "
                 "should have returned false");

      if (!mImage->GetCropRect()) {
        mImageContainer.swap(srcImage);
      } else {
        nsIntRect actualCropRect;
        bool isEntireImage;
        bool success =
          mImage->ComputeActualCropRect(actualCropRect, &isEntireImage);
        NS_ASSERTION(success, "ComputeActualCropRect() should not fail here");
        if (!success || actualCropRect.IsEmpty()) {
          
          return false;
        }
        if (isEntireImage) {
          
          mImageContainer.swap(srcImage);
        } else {
          nsCOMPtr<imgIContainer> subImage = ImageOps::Clip(srcImage, actualCropRect);
          mImageContainer.swap(subImage);
        }
      }
      mIsReady = true;
      break;
    }
    case eStyleImageType_Gradient:
      mGradientData = mImage->GetGradientData();
      mIsReady = true;
      break;
    case eStyleImageType_Element:
    {
      nsAutoString elementId =
        NS_LITERAL_STRING("#") + nsDependentString(mImage->GetElementId());
      nsCOMPtr<nsIURI> targetURI;
      nsCOMPtr<nsIURI> base = mForFrame->GetContent()->GetBaseURI();
      nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(targetURI), elementId,
                                                mForFrame->GetContent()->GetCurrentDoc(), base);
      nsSVGPaintingProperty* property = nsSVGEffects::GetPaintingPropertyForURI(
          targetURI, mForFrame->FirstContinuation(),
          nsSVGEffects::BackgroundImageProperty());
      if (!property)
        return false;
      mPaintServerFrame = property->GetReferencedFrame();

      
      
      if (!mPaintServerFrame) {
        mImageElementSurface =
          nsLayoutUtils::SurfaceFromElement(property->GetReferencedElement());
        if (!mImageElementSurface.mSourceSurface)
          return false;
      }
      mIsReady = true;
      break;
    }
    case eStyleImageType_Null:
    default:
      break;
  }

  return mIsReady;
}

nsSize
CSSSizeOrRatio::ComputeConcreteSize() const
{
  NS_ASSERTION(CanComputeConcreteSize(), "Cannot compute");
  if (mHasWidth && mHasHeight) {
    return nsSize(mWidth, mHeight);
  }
  if (mHasWidth) {
    nscoord height = NSCoordSaturatingNonnegativeMultiply(
      mWidth,
      double(mRatio.height) / mRatio.width);
    return nsSize(mWidth, height);
  }

  MOZ_ASSERT(mHasHeight);
  nscoord width = NSCoordSaturatingNonnegativeMultiply(
    mHeight,
    double(mRatio.width) / mRatio.height);
  return nsSize(width, mHeight);
}

CSSSizeOrRatio
nsImageRenderer::ComputeIntrinsicSize()
{
  NS_ASSERTION(mIsReady, "Ensure PrepareImage() has returned true "
                         "before calling me");

  CSSSizeOrRatio result;
  switch (mType) {
    case eStyleImageType_Image:
    {
      bool haveWidth, haveHeight;
      CSSIntSize imageIntSize;
      nsLayoutUtils::ComputeSizeForDrawing(mImageContainer, imageIntSize,
                                           result.mRatio, haveWidth, haveHeight);
      if (haveWidth) {
        result.SetWidth(nsPresContext::CSSPixelsToAppUnits(imageIntSize.width));
      }
      if (haveHeight) {
        result.SetHeight(nsPresContext::CSSPixelsToAppUnits(imageIntSize.height));
      }
      break;
    }
    case eStyleImageType_Element:
    {
      
      
      
      
      
      
      if (mPaintServerFrame) {
        
        if (!mPaintServerFrame->IsFrameOfType(nsIFrame::eSVG)) {
          
          
          
          int32_t appUnitsPerDevPixel =
            mForFrame->PresContext()->AppUnitsPerDevPixel();
          result.SetSize(
            IntSizeToAppUnits(
              nsSVGIntegrationUtils::GetContinuationUnionSize(mPaintServerFrame).
                ToNearestPixels(appUnitsPerDevPixel),
              appUnitsPerDevPixel));
        }
      } else {
        NS_ASSERTION(mImageElementSurface.mSourceSurface, "Surface should be ready.");
        gfxIntSize surfaceSize = mImageElementSurface.mSize;
        result.SetSize(
          nsSize(nsPresContext::CSSPixelsToAppUnits(surfaceSize.width),
                 nsPresContext::CSSPixelsToAppUnits(surfaceSize.height)));
      }
      break;
    }
    case eStyleImageType_Gradient:
      
      
    case eStyleImageType_Null:
    default:
      break;
  }

  return result;
}

 nsSize
nsImageRenderer::ComputeConcreteSize(const CSSSizeOrRatio& aSpecifiedSize,
                                     const CSSSizeOrRatio& aIntrinsicSize,
                                     const nsSize& aDefaultSize)
{
  
  if (aSpecifiedSize.IsConcrete()) {
    return aSpecifiedSize.ComputeConcreteSize();
  }

  MOZ_ASSERT(!aSpecifiedSize.mHasWidth || !aSpecifiedSize.mHasHeight);

  if (!aSpecifiedSize.mHasWidth && !aSpecifiedSize.mHasHeight) {
    
    if (aIntrinsicSize.CanComputeConcreteSize()) {
      return aIntrinsicSize.ComputeConcreteSize();
    }

    if (aIntrinsicSize.mHasWidth) {
      return nsSize(aIntrinsicSize.mWidth, aDefaultSize.height);
    }
    if (aIntrinsicSize.mHasHeight) {
      return nsSize(aDefaultSize.width, aIntrinsicSize.mHeight);
    }

    
    return ComputeConstrainedSize(aDefaultSize,
                                  aIntrinsicSize.mRatio,
                                  CONTAIN);
  }

  MOZ_ASSERT(aSpecifiedSize.mHasWidth || aSpecifiedSize.mHasHeight);

  
  if (aSpecifiedSize.mHasWidth) {
    nscoord height;
    if (aIntrinsicSize.HasRatio()) {
      height = NSCoordSaturatingNonnegativeMultiply(
        aSpecifiedSize.mWidth,
        double(aIntrinsicSize.mRatio.height) / aIntrinsicSize.mRatio.width);
    } else if (aIntrinsicSize.mHasHeight) {
      height = aIntrinsicSize.mHeight;
    } else {
      height = aDefaultSize.height;
    }
    return nsSize(aSpecifiedSize.mWidth, height);
  }

  MOZ_ASSERT(aSpecifiedSize.mHasHeight);
  nscoord width;
  if (aIntrinsicSize.HasRatio()) {
    width = NSCoordSaturatingNonnegativeMultiply(
      aSpecifiedSize.mHeight,
      double(aIntrinsicSize.mRatio.width) / aIntrinsicSize.mRatio.height);
  } else if (aIntrinsicSize.mHasWidth) {
    width = aIntrinsicSize.mWidth;
  } else {
    width = aDefaultSize.width;
  }
  return nsSize(width, aSpecifiedSize.mHeight);
}

 nsSize
nsImageRenderer::ComputeConstrainedSize(const nsSize& aConstrainingSize,
                                        const nsSize& aIntrinsicRatio,
                                        FitType aFitType)
{
  if (aIntrinsicRatio.width <= 0 && aIntrinsicRatio.height <= 0) {
    return aConstrainingSize;
  }

  float scaleX = double(aConstrainingSize.width) / aIntrinsicRatio.width;
  float scaleY = double(aConstrainingSize.height) / aIntrinsicRatio.height;
  nsSize size;
  if ((aFitType == CONTAIN) == (scaleX < scaleY)) {
    size.width = aConstrainingSize.width;
    size.height = NSCoordSaturatingNonnegativeMultiply(
                    aIntrinsicRatio.height, scaleX);
  } else {
    size.width = NSCoordSaturatingNonnegativeMultiply(
                   aIntrinsicRatio.width, scaleY);
    size.height = aConstrainingSize.height;
  }
  return size;
}




















void
nsImageRenderer::SetPreferredSize(const CSSSizeOrRatio& aIntrinsicSize,
                                  const nsSize& aDefaultSize)
{
  mSize.width = aIntrinsicSize.mHasWidth
                  ? aIntrinsicSize.mWidth
                  : aDefaultSize.width;
  mSize.height = aIntrinsicSize.mHasHeight
                  ? aIntrinsicSize.mHeight
                  : aDefaultSize.height;
}



static uint32_t
ConvertImageRendererToDrawFlags(uint32_t aImageRendererFlags)
{
  uint32_t drawFlags = imgIContainer::FLAG_NONE;
  if (aImageRendererFlags & nsImageRenderer::FLAG_SYNC_DECODE_IMAGES) {
    drawFlags |= imgIContainer::FLAG_SYNC_DECODE;
  }
  if (aImageRendererFlags & nsImageRenderer::FLAG_PAINTING_TO_WINDOW) {
    drawFlags |= imgIContainer::FLAG_HIGH_QUALITY_SCALING;
  }
  return drawFlags;
}

DrawResult
nsImageRenderer::Draw(nsPresContext*       aPresContext,
                      nsRenderingContext&  aRenderingContext,
                      const nsRect&        aDirtyRect,
                      const nsRect&        aDest,
                      const nsRect&        aFill,
                      const nsPoint&       aAnchor,
                      const CSSIntRect&    aSrc)
{
  if (!mIsReady) {
    NS_NOTREACHED("Ensure PrepareImage() has returned true before calling me");
    return DrawResult::TEMPORARY_ERROR;
  }
  if (aDest.IsEmpty() || aFill.IsEmpty() ||
      mSize.width <= 0 || mSize.height <= 0) {
    return DrawResult::SUCCESS;
  }

  GraphicsFilter filter = nsLayoutUtils::GetGraphicsFilterForFrame(mForFrame);

  switch (mType) {
    case eStyleImageType_Image:
    {
      CSSIntSize imageSize(nsPresContext::AppUnitsToIntCSSPixels(mSize.width),
                           nsPresContext::AppUnitsToIntCSSPixels(mSize.height));
      return
        nsLayoutUtils::DrawBackgroundImage(*aRenderingContext.ThebesContext(),
                                           aPresContext,
                                           mImageContainer, imageSize, filter,
                                           aDest, aFill, aAnchor, aDirtyRect,
                                           ConvertImageRendererToDrawFlags(mFlags));
    }
    case eStyleImageType_Gradient:
    {
      nsCSSRendering::PaintGradient(aPresContext, aRenderingContext,
                                    mGradientData, aDirtyRect,
                                    aDest, aFill, aSrc, mSize);
      return DrawResult::SUCCESS;
    }
    case eStyleImageType_Element:
    {
      nsRefPtr<gfxDrawable> drawable = DrawableForElement(aDest,
                                                          aRenderingContext);
      if (!drawable) {
        NS_WARNING("Could not create drawable for element");
        return DrawResult::TEMPORARY_ERROR;
      }

      gfxContext* ctx = aRenderingContext.ThebesContext();
      gfxContext::GraphicsOperator op = ctx->CurrentOperator();
      if (op != gfxContext::OPERATOR_OVER) {
        ctx->PushGroup(gfxContentType::COLOR_ALPHA);
        ctx->SetOperator(gfxContext::OPERATOR_OVER);
      }

      nsCOMPtr<imgIContainer> image(ImageOps::CreateFromDrawable(drawable));
      nsLayoutUtils::DrawImage(*aRenderingContext.ThebesContext(),
                               aPresContext, image,
                               filter, aDest, aFill, aAnchor, aDirtyRect,
                               ConvertImageRendererToDrawFlags(mFlags));

      if (op != gfxContext::OPERATOR_OVER) {
        ctx->PopGroupToSource();
        ctx->Paint();
      }

      return DrawResult::SUCCESS;
    }
    case eStyleImageType_Null:
    default:
      return DrawResult::SUCCESS;
  }
}

already_AddRefed<gfxDrawable>
nsImageRenderer::DrawableForElement(const nsRect& aImageRect,
                                    nsRenderingContext&  aRenderingContext)
{
  NS_ASSERTION(mType == eStyleImageType_Element,
               "DrawableForElement only makes sense if backed by an element");
  if (mPaintServerFrame) {
    int32_t appUnitsPerDevPixel = mForFrame->PresContext()->AppUnitsPerDevPixel();
    nsRect destRect = aImageRect - aImageRect.TopLeft();
    nsIntSize roundedOut = destRect.ToOutsidePixels(appUnitsPerDevPixel).Size();
    gfxIntSize imageSize(roundedOut.width, roundedOut.height);
    nsRefPtr<gfxDrawable> drawable =
      nsSVGIntegrationUtils::DrawableFromPaintServer(
        mPaintServerFrame, mForFrame, mSize, imageSize,
        aRenderingContext.GetDrawTarget(),
        aRenderingContext.ThebesContext()->CurrentMatrix(),
        mFlags & FLAG_SYNC_DECODE_IMAGES
          ? nsSVGIntegrationUtils::FLAG_SYNC_DECODE_IMAGES
          : 0);

    return drawable.forget();
  }
  NS_ASSERTION(mImageElementSurface.mSourceSurface, "Surface should be ready.");
  nsRefPtr<gfxDrawable> drawable = new gfxSurfaceDrawable(
                                mImageElementSurface.mSourceSurface,
                                mImageElementSurface.mSize);
  return drawable.forget();
}

DrawResult
nsImageRenderer::DrawBackground(nsPresContext*       aPresContext,
                                nsRenderingContext&  aRenderingContext,
                                const nsRect&        aDest,
                                const nsRect&        aFill,
                                const nsPoint&       aAnchor,
                                const nsRect&        aDirty)
{
  if (!mIsReady) {
    NS_NOTREACHED("Ensure PrepareImage() has returned true before calling me");
    return DrawResult::TEMPORARY_ERROR;
  }
  if (aDest.IsEmpty() || aFill.IsEmpty() ||
      mSize.width <= 0 || mSize.height <= 0) {
    return DrawResult::SUCCESS;
  }

  return Draw(aPresContext, aRenderingContext,
              aDirty, aDest, aFill, aAnchor,
              CSSIntRect(0, 0,
                         nsPresContext::AppUnitsToIntCSSPixels(mSize.width),
                         nsPresContext::AppUnitsToIntCSSPixels(mSize.height)));
}









static nsRect
ComputeTile(const nsRect&        aFill,
            uint8_t              aHFill,
            uint8_t              aVFill,
            const nsSize&        aUnitSize)
{
  nsRect tile;
  switch (aHFill) {
  case NS_STYLE_BORDER_IMAGE_REPEAT_STRETCH:
    tile.x = aFill.x;
    tile.width = aFill.width;
    break;
  case NS_STYLE_BORDER_IMAGE_REPEAT_REPEAT:
    tile.x = aFill.x + aFill.width/2 - aUnitSize.width/2;
    tile.width = aUnitSize.width;
    break;
  case NS_STYLE_BORDER_IMAGE_REPEAT_ROUND:
    tile.x = aFill.x;
    tile.width = aFill.width / ceil(gfxFloat(aFill.width)/aUnitSize.width);
    break;
  default:
    NS_NOTREACHED("unrecognized border-image fill style");
  }

  switch (aVFill) {
  case NS_STYLE_BORDER_IMAGE_REPEAT_STRETCH:
    tile.y = aFill.y;
    tile.height = aFill.height;
    break;
  case NS_STYLE_BORDER_IMAGE_REPEAT_REPEAT:
    tile.y = aFill.y + aFill.height/2 - aUnitSize.height/2;
    tile.height = aUnitSize.height;
    break;
  case NS_STYLE_BORDER_IMAGE_REPEAT_ROUND:
    tile.y = aFill.y;
    tile.height = aFill.height/ceil(gfxFloat(aFill.height)/aUnitSize.height);
    break;
  default:
    NS_NOTREACHED("unrecognized border-image fill style");
  }

  return tile;
}






static bool
RequiresScaling(const nsRect&        aFill,
                uint8_t              aHFill,
                uint8_t              aVFill,
                const nsSize&        aUnitSize)
{
  
  
  return (aHFill != NS_STYLE_BORDER_IMAGE_REPEAT_STRETCH ||
          aVFill != NS_STYLE_BORDER_IMAGE_REPEAT_STRETCH) &&
         (aUnitSize.width != aFill.width ||
          aUnitSize.height != aFill.height);
}

void
nsImageRenderer::DrawBorderImageComponent(nsPresContext*       aPresContext,
                                          nsRenderingContext&  aRenderingContext,
                                          const nsRect&        aDirtyRect,
                                          const nsRect&        aFill,
                                          const CSSIntRect&    aSrc,
                                          uint8_t              aHFill,
                                          uint8_t              aVFill,
                                          const nsSize&        aUnitSize,
                                          uint8_t              aIndex)
{
  if (!mIsReady) {
    NS_NOTREACHED("Ensure PrepareImage() has returned true before calling me");
    return;
  }
  if (aFill.IsEmpty() || aSrc.IsEmpty()) {
    return;
  }

  if (mType == eStyleImageType_Image || mType == eStyleImageType_Element) {
    nsCOMPtr<imgIContainer> subImage;

    
    nsIntRect srcRect(aSrc.x, aSrc.y, aSrc.width, aSrc.height);
    if (mType == eStyleImageType_Image) {
      if ((subImage = mImage->GetSubImage(aIndex)) == nullptr) {
        subImage = ImageOps::Clip(mImageContainer, srcRect);
        mImage->SetSubImage(aIndex, subImage);
      }
    } else {
      
      
      
      
      
      
      

      nsRefPtr<gfxDrawable> drawable = DrawableForElement(nsRect(nsPoint(), mSize),
                                                          aRenderingContext);
      if (!drawable) {
        NS_WARNING("Could not create drawable for element");
        return;
      }

      nsCOMPtr<imgIContainer> image(ImageOps::CreateFromDrawable(drawable));
      subImage = ImageOps::Clip(image, srcRect);
    }

    GraphicsFilter graphicsFilter =
      nsLayoutUtils::GetGraphicsFilterForFrame(mForFrame);

    if (!RequiresScaling(aFill, aHFill, aVFill, aUnitSize)) {
      nsLayoutUtils::DrawSingleImage(*aRenderingContext.ThebesContext(),
                                     aPresContext,
                                     subImage,
                                     graphicsFilter,
                                     aFill, aDirtyRect,
                                     nullptr,
                                     imgIContainer::FLAG_NONE);
      return;
    }

    nsRect tile = ComputeTile(aFill, aHFill, aVFill, aUnitSize);
    nsLayoutUtils::DrawImage(*aRenderingContext.ThebesContext(),
                             aPresContext,
                             subImage,
                             graphicsFilter,
                             tile, aFill, tile.TopLeft(), aDirtyRect,
                             imgIContainer::FLAG_NONE);
    return;
  }

  nsRect destTile = RequiresScaling(aFill, aHFill, aVFill, aUnitSize)
                  ? ComputeTile(aFill, aHFill, aVFill, aUnitSize)
                  : aFill;

  Draw(aPresContext, aRenderingContext, aDirtyRect, destTile,
       aFill, destTile.TopLeft(), aSrc);
}

bool
nsImageRenderer::IsRasterImage()
{
  if (mType != eStyleImageType_Image || !mImageContainer)
    return false;
  return mImageContainer->GetType() == imgIContainer::TYPE_RASTER;
}

bool
nsImageRenderer::IsAnimatedImage()
{
  if (mType != eStyleImageType_Image || !mImageContainer)
    return false;
  bool animated = false;
  if (NS_SUCCEEDED(mImageContainer->GetAnimated(&animated)) && animated)
    return true;

  return false;
}

already_AddRefed<mozilla::layers::ImageContainer>
nsImageRenderer::GetContainer(LayerManager* aManager)
{
  if (mType != eStyleImageType_Image || !mImageContainer) {
    return nullptr;
  }

  return mImageContainer->GetImageContainer(aManager, imgIContainer::FLAG_NONE);
}

#define MAX_BLUR_RADIUS 300
#define MAX_SPREAD_RADIUS 50

static inline gfxPoint ComputeBlurStdDev(nscoord aBlurRadius,
                                         int32_t aAppUnitsPerDevPixel,
                                         gfxFloat aScaleX,
                                         gfxFloat aScaleY)
{
  
  
  gfxFloat blurStdDev = gfxFloat(aBlurRadius) / gfxFloat(aAppUnitsPerDevPixel);

  return gfxPoint(std::min((blurStdDev * aScaleX),
                           gfxFloat(MAX_BLUR_RADIUS)) / 2.0,
                  std::min((blurStdDev * aScaleY),
                           gfxFloat(MAX_BLUR_RADIUS)) / 2.0);
}

static inline gfxIntSize
ComputeBlurRadius(nscoord aBlurRadius,
                  int32_t aAppUnitsPerDevPixel,
                  gfxFloat aScaleX = 1.0,
                  gfxFloat aScaleY = 1.0)
{
  gfxPoint scaledBlurStdDev = ComputeBlurStdDev(aBlurRadius, aAppUnitsPerDevPixel,
                                                aScaleX, aScaleY);
  return
    gfxAlphaBoxBlur::CalculateBlurRadius(scaledBlurStdDev);
}




gfxContext*
nsContextBoxBlur::Init(const nsRect& aRect, nscoord aSpreadRadius,
                       nscoord aBlurRadius,
                       int32_t aAppUnitsPerDevPixel,
                       gfxContext* aDestinationCtx,
                       const nsRect& aDirtyRect,
                       const gfxRect* aSkipRect,
                       uint32_t aFlags)
{
  if (aRect.IsEmpty()) {
    mContext = nullptr;
    return nullptr;
  }

  gfxFloat scaleX = 1;
  gfxFloat scaleY = 1;

  
  
  
  gfxMatrix transform = aDestinationCtx->CurrentMatrix();
  
  if (transform.HasNonAxisAlignedTransform() || transform._11 <= 0.0 || transform._22 <= 0.0) {
    transform = gfxMatrix();
  } else {
    scaleX = transform._11;
    scaleY = transform._22;
  }

  
  gfxIntSize blurRadius = ComputeBlurRadius(aBlurRadius, aAppUnitsPerDevPixel, scaleX, scaleY);
  gfxIntSize spreadRadius = gfxIntSize(std::min(int32_t(aSpreadRadius * scaleX / aAppUnitsPerDevPixel),
                                              int32_t(MAX_SPREAD_RADIUS)),
                                       std::min(int32_t(aSpreadRadius * scaleY / aAppUnitsPerDevPixel),
                                              int32_t(MAX_SPREAD_RADIUS)));
  mDestinationCtx = aDestinationCtx;

  
  if (blurRadius.width <= 0 && blurRadius.height <= 0 &&
      spreadRadius.width <= 0 && spreadRadius.height <= 0 &&
      !(aFlags & FORCE_MASK)) {
    mContext = aDestinationCtx;
    return mContext;
  }

  
  gfxRect rect = nsLayoutUtils::RectToGfxRect(aRect, aAppUnitsPerDevPixel);

  gfxRect dirtyRect =
    nsLayoutUtils::RectToGfxRect(aDirtyRect, aAppUnitsPerDevPixel);
  dirtyRect.RoundOut();

  rect = transform.TransformBounds(rect);

  mPreTransformed = !transform.IsIdentity();

  
  dirtyRect = transform.TransformBounds(dirtyRect);
  if (aSkipRect) {
    gfxRect skipRect = transform.TransformBounds(*aSkipRect);
    mContext = mAlphaBoxBlur.Init(rect, spreadRadius,
                                  blurRadius, &dirtyRect, &skipRect);
  } else {
    mContext = mAlphaBoxBlur.Init(rect, spreadRadius,
                                  blurRadius, &dirtyRect, nullptr);
  }

  if (mContext) {
    
    
    mContext->Multiply(transform);
  }
  return mContext;
}

void
nsContextBoxBlur::DoPaint()
{
  if (mContext == mDestinationCtx)
    return;

  gfxContextMatrixAutoSaveRestore saveMatrix(mDestinationCtx);

  if (mPreTransformed) {
    mDestinationCtx->SetMatrix(gfxMatrix());
  }

  mAlphaBoxBlur.Paint(mDestinationCtx);
}

gfxContext*
nsContextBoxBlur::GetContext()
{
  return mContext;
}

 nsMargin
nsContextBoxBlur::GetBlurRadiusMargin(nscoord aBlurRadius,
                                      int32_t aAppUnitsPerDevPixel)
{
  gfxIntSize blurRadius = ComputeBlurRadius(aBlurRadius, aAppUnitsPerDevPixel);

  nsMargin result;
  result.top = result.bottom = blurRadius.height * aAppUnitsPerDevPixel;
  result.left = result.right = blurRadius.width  * aAppUnitsPerDevPixel;
  return result;
}

 void
nsContextBoxBlur::BlurRectangle(gfxContext* aDestinationCtx,
                                const nsRect& aRect,
                                int32_t aAppUnitsPerDevPixel,
                                RectCornerRadii* aCornerRadii,
                                nscoord aBlurRadius,
                                const gfxRGBA& aShadowColor,
                                const nsRect& aDirtyRect,
                                const gfxRect& aSkipRect)
{
  DrawTarget& aDestDrawTarget = *aDestinationCtx->GetDrawTarget();

  if (aRect.IsEmpty()) {
    return;
  }

  Rect shadowGfxRect = NSRectToRect(aRect, aAppUnitsPerDevPixel);

  if (aBlurRadius <= 0) {
    ColorPattern color(ToDeviceColor(aShadowColor));
    if (aCornerRadii) {
      RefPtr<Path> roundedRect = MakePathForRoundedRect(aDestDrawTarget,
                                                        shadowGfxRect,
                                                        *aCornerRadii);
      aDestDrawTarget.Fill(roundedRect, color);
    } else {
      aDestDrawTarget.FillRect(shadowGfxRect, color);
    }
    return;
  }

  gfxFloat scaleX = 1;
  gfxFloat scaleY = 1;

  
  
  
  gfxMatrix transform = aDestinationCtx->CurrentMatrix();
  
  if (!transform.HasNonAxisAlignedTransform() && transform._11 > 0.0 && transform._22 > 0.0) {
    scaleX = transform._11;
    scaleY = transform._22;
    aDestinationCtx->SetMatrix(gfxMatrix());
  } else {
    transform = gfxMatrix();
  }

  gfxPoint blurStdDev = ComputeBlurStdDev(aBlurRadius, aAppUnitsPerDevPixel, scaleX, scaleY);

  gfxRect dirtyRect =
    nsLayoutUtils::RectToGfxRect(aDirtyRect, aAppUnitsPerDevPixel);
  dirtyRect.RoundOut();

  gfxRect shadowThebesRect = transform.TransformBounds(ThebesRect(shadowGfxRect));
  dirtyRect = transform.TransformBounds(dirtyRect);
  gfxRect skipRect = transform.TransformBounds(aSkipRect);

  if (aCornerRadii) {
    aCornerRadii->Scale(scaleX, scaleY);
  }

  gfxAlphaBoxBlur::BlurRectangle(aDestinationCtx,
                                 shadowThebesRect,
                                 aCornerRadii,
                                 blurStdDev,
                                 aShadowColor,
                                 dirtyRect,
                                 skipRect);
}
