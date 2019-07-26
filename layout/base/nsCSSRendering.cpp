







#include <ctime>

#include "mozilla/DebugOnly.h"
#include "mozilla/HashFunctions.h"
#include "mozilla/MathAlgorithms.h"
#include "mozilla/Types.h"

#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsIFrame.h"
#include "nsPoint.h"
#include "nsRect.h"
#include "nsViewManager.h"
#include "nsIPresShell.h"
#include "nsFrameManager.h"
#include "nsStyleContext.h"
#include "nsGkAtoms.h"
#include "nsCSSAnonBoxes.h"
#include "nsTransform2D.h"
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
#include "nsIServiceManager.h"
#include "nsLayoutUtils.h"
#include "nsINameSpaceManager.h"
#include "nsBlockFrame.h"
#include "gfxContext.h"
#include "nsRenderingContext.h"
#include "nsIInterfaceRequestorUtils.h"
#include "gfxPlatform.h"
#include "gfxImageSurface.h"
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
#include <algorithm>

using namespace mozilla;
using namespace mozilla::css;
using mozilla::image::ImageOps;

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
    mContinuationPoint = mLineContinuationPoint = mUnbrokenWidth = 0;
    mFrame = mBlockFrame = nullptr;
  }

  nsRect GetContinuousRect(nsIFrame* aFrame)
  {
    SetFrame(aFrame);

    nscoord x;
    if (mBidiEnabled) {
      x = mLineContinuationPoint;

      
      
      
      bool isRtlBlock = (mBlockFrame->StyleVisibility()->mDirection ==
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
  bool          mBidiEnabled;

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

    
    
    nsIFrame* inlineFrame = GetPrevContinuation(aFrame);

    while (inlineFrame) {
      nsRect rect = inlineFrame->GetRect();
      mContinuationPoint += rect.width;
      if (mBidiEnabled && !AreOnSameLine(aFrame, inlineFrame)) {
        mLineContinuationPoint += rect.width;
      }
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
  ColorStop(double aPosition, gfxRGBA aColor) :
    mPosition(aPosition), mColor(aColor) {}
  double mPosition; 
  gfxRGBA mColor;
};

struct GradientCacheKey : public PLDHashEntryHdr {
  typedef const GradientCacheKey& KeyType;
  typedef const GradientCacheKey* KeyTypePointer;
  enum { ALLOW_MEMMOVE = true };
  const nsTArray<gfx::GradientStop> mStops;
  const bool mRepeating;
  const gfx::BackendType mBackendType;

  GradientCacheKey(const nsTArray<gfx::GradientStop>& aStops, const bool aRepeating, const gfx::BackendType aBackendType)
    : mStops(aStops), mRepeating(aRepeating), mBackendType(aBackendType)
  { }

  GradientCacheKey(const GradientCacheKey* aOther)
    : mStops(aOther->mStops), mRepeating(aOther->mRepeating), mBackendType(aOther->mBackendType)
  { }

  union FloatUint32
  {
    float    f;
    uint32_t u;
  };

  static PLDHashNumber
  HashKey(const KeyTypePointer aKey)
  {
    PLDHashNumber hash = 0;
    FloatUint32 convert;
    hash = AddToHash(hash, aKey->mBackendType);
    hash = AddToHash(hash, aKey->mRepeating);
    for (uint32_t i = 0; i < aKey->mStops.Length(); i++) {
      hash = AddToHash(hash, aKey->mStops[i].color.ToABGR());
      
      convert.f = aKey->mStops[i].offset;
      hash = AddToHash(hash, convert.f ? convert.u : 0);
    }
    return hash;
  }

  bool KeyEquals(KeyTypePointer aKey) const
  {
    bool sameStops = true;
    if (aKey->mStops.Length() != mStops.Length()) {
      sameStops = false;
    } else {
      for (uint32_t i = 0; i < mStops.Length(); i++) {
        if (mStops[i].color.ToABGR() != aKey->mStops[i].color.ToABGR() ||
            mStops[i].offset != aKey->mStops[i].offset) {
          sameStops = false;
          break;
        }
      }
    }

    return sameStops &&
           (aKey->mBackendType == mBackendType) &&
           (aKey->mRepeating == mRepeating);
  }
  static KeyTypePointer KeyToPointer(KeyType aKey)
  {
    return &aKey;
  }
};





struct GradientCacheData {
  GradientCacheData(mozilla::gfx::GradientStops* aStops, const GradientCacheKey& aKey)
    : mStops(aStops),
      mKey(aKey)
  {}

  GradientCacheData(const GradientCacheData& aOther)
    : mStops(aOther.mStops),
      mKey(aOther.mKey)
  { }

  nsExpirationState *GetExpirationState() {
    return &mExpirationState;
  }

  nsExpirationState mExpirationState;
  const mozilla::RefPtr<mozilla::gfx::GradientStops> mStops;
  GradientCacheKey mKey;
};















class GradientCache MOZ_FINAL : public nsExpirationTracker<GradientCacheData,4>
{
  public:
    GradientCache()
      : nsExpirationTracker<GradientCacheData, 4>(MAX_GENERATION_MS)
    {
      mHashEntries.Init();
      srand(time(nullptr));
      mTimerPeriod = rand() % MAX_GENERATION_MS + 1;
      Telemetry::Accumulate(Telemetry::GRADIENT_RETENTION_TIME, mTimerPeriod);
    }

    virtual void NotifyExpired(GradientCacheData* aObject)
    {
      
      RemoveObject(aObject);
      mHashEntries.Remove(aObject->mKey);
    }

    GradientCacheData* Lookup(const nsTArray<gfx::GradientStop>& aStops, bool aRepeating, gfx::BackendType aBackendType)
    {
      GradientCacheData* gradient =
        mHashEntries.Get(GradientCacheKey(aStops, aRepeating, aBackendType));

      if (gradient) {
        MarkUsed(gradient);
      }

      return gradient;
    }

    
    
    bool RegisterEntry(GradientCacheData* aValue)
    {
      nsresult rv = AddObject(aValue);
      if (NS_FAILED(rv)) {
        
        
        
        
        
        return false;
      }
      mHashEntries.Put(aValue->mKey, aValue);
      return true;
    }

  protected:
    uint32_t mTimerPeriod;
    static const uint32_t MAX_GENERATION_MS = 10000;
    



    nsClassHashtable<GradientCacheKey, GradientCacheData> mHashEntries;
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
                                     uint8_t aHFill,
                                     uint8_t aVFill,
                                     const nsSize& aUnitSize,
                                     const nsStyleBorder& aStyleBorder,
                                     uint8_t aIndex);

static nscolor MakeBevelColor(mozilla::css::Side whichSide, uint8_t style,
                              nscolor aBackgroundColor,
                              nscolor aBorderColor);

static InlineBackgroundData* gInlineBGData = nullptr;
static GradientCache* gGradientCache = nullptr;


void nsCSSRendering::Init()
{
  NS_ASSERTION(!gInlineBGData, "Init called twice");
  gInlineBGData = new InlineBackgroundData();
  gGradientCache = new GradientCache();
  nsCSSBorderRenderer::Init();
}


void nsCSSRendering::Shutdown()
{
  delete gInlineBGData;
  gInlineBGData = nullptr;
  delete gGradientCache;
  gGradientCache = nullptr;
  nsCSSBorderRenderer::Shutdown();
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
                            int aSkipSides)
{
  PROFILER_LABEL("nsCSSRendering", "PaintBorder");
  nsStyleContext *styleIfVisited = aStyleContext->GetStyleIfVisited();
  const nsStyleBorder *styleBorder = aStyleContext->StyleBorder();
  
  
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
                                           int aSkipSides)
{
  nsMargin            border;
  nscoord             twipsRadii[8];
  nsCompatibility     compatMode = aPresContext->CompatibilityMode();

  SN("++ PaintBorder");

  
  
  
  const nsStyleDisplay* displayData = aStyleContext->StyleDisplay();
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

  
  const nsStyleColor* ourColor = aStyleContext->StyleColor();

  
  
  nsIFrame* bgFrame = nsCSSRendering::FindNonTransparentBackgroundFrame
    (aForFrame, compatMode == eCompatibility_NavQuirks ? true : false);
  nsStyleContext* bgContext = bgFrame->StyleContext();
  nscolor bgColor =
    bgContext->GetVisitedDependentColor(eCSSProperty_background_color);

  border = aStyleBorder.GetComputedBorder();
  if ((0 == border.left) && (0 == border.right) &&
      (0 == border.top) && (0 == border.bottom)) {
    
    return;
  }

  nsSize frameSize = aForFrame->GetSize();
  if (&aStyleBorder == aForFrame->StyleBorder() &&
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

  
  const nsStyleOutline* ourOutline = aStyleContext->StyleOutline();

  nscoord width;
  ourOutline->GetOutlineWidth(width);

  if (width == 0) {
    
    return;
  }

  nsIFrame* bgFrame = nsCSSRendering::FindNonTransparentBackgroundFrame
    (aForFrame, false);
  nsStyleContext* bgContext = bgFrame->StyleContext();
  nscolor bgColor =
    bgContext->GetVisitedDependentColor(eCSSProperty_background_color);

  
  
  
  
  
  
  nsIFrame *frameForArea = aForFrame;
  do {
    nsIAtom *pseudoType = frameForArea->StyleContext()->GetPseudo();
    if (pseudoType != nsCSSAnonBoxes::mozAnonymousBlock &&
        pseudoType != nsCSSAnonBoxes::mozAnonymousPositionedBlock)
      break;
    
    frameForArea = frameForArea->GetFirstPrincipalChild();
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

  uint8_t outlineStyle = ourOutline->GetOutlineStyle();
  uint8_t outlineStyles[4] = { outlineStyle,
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
                         nullptr, 0,
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

  uint8_t focusStyles[4] = { NS_STYLE_BORDER_STYLE_DOTTED,
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
                         nullptr, 0,
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
  if (!content || content->Tag() != nsGkAtoms::body)
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
                                    const nsRect& aDirtyRect)
{
  const nsStyleBorder* styleBorder = aForFrame->StyleBorder();
  nsCSSShadowArray* shadows = styleBorder->mBoxShadow;
  if (!shadows)
    return;
  nscoord twipsPerPixel = aPresContext->DevPixelsToAppUnits(1);

  bool hasBorderRadius;
  bool nativeTheme; 
  gfxCornerSizes borderRadii;

  
  const nsStyleDisplay* styleDisplay = aForFrame->StyleDisplay();
  nsITheme::Transparency transparency;
  if (aForFrame->IsThemed(styleDisplay, &transparency)) {
    
    hasBorderRadius = false;
    
    
    nativeTheme = transparency != nsITheme::eOpaque;
  } else {
    nativeTheme = false;
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

  for (uint32_t i = shadows->Length(); i > 0; --i) {
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
    nsContextBoxBlur blurringArea;

    
    
    
    
    gfxContext* shadowContext =
      blurringArea.Init(shadowRect, pixelSpreadRadius,
                        blurRadius, twipsPerPixel, renderContext, aDirtyRect,
                        useSkipGfxRect ? &skipGfxRect : nullptr,
                        nativeTheme ? nsContextBoxBlur::FORCE_MASK : 0);
    if (!shadowContext)
      continue;

    
    MOZ_ASSERT(shadowContext == renderContext ||
               shadowContext == blurringArea.GetContext());

    
    nscolor shadowColor;
    if (shadowItem->mHasColor)
      shadowColor = shadowItem->mColor;
    else
      shadowColor = aForFrame->StyleColor()->mColor;

    renderContext->Save();
    renderContext->SetColor(gfxRGBA(shadowColor));

    
    
    
    
    
    if (nativeTheme) {
      
      

      
      gfxContextMatrixAutoSaveRestore save(shadowContext);
      nsRefPtr<nsRenderingContext> wrapperCtx = new nsRenderingContext();
      wrapperCtx->Init(aPresContext->DeviceContext(), shadowContext);
      wrapperCtx->Translate(nsPoint(shadowItem->mXOffset,
                                    shadowItem->mYOffset));

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
        gfxFloat spreadDistance = shadowItem->mSpread / twipsPerPixel;

        gfxFloat borderSizes[4];

        borderSizes[NS_SIDE_LEFT] = spreadDistance;
        borderSizes[NS_SIDE_TOP] = spreadDistance;
        borderSizes[NS_SIDE_RIGHT] = spreadDistance;
        borderSizes[NS_SIDE_BOTTOM] = spreadDistance;

        nsCSSBorderRenderer::ComputeOuterRadii(borderRadii, borderSizes,
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
  const nsStyleBorder* styleBorder = aForFrame->StyleBorder();
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
  bool hasBorderRadius = aForFrame->GetBorderRadii(twipsRadii);
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
      skipGfxRect.Deflate(gfxMargin(
          std::max(clipRectRadii[C_TL].height, clipRectRadii[C_TR].height), 0,
          std::max(clipRectRadii[C_BL].height, clipRectRadii[C_BR].height), 0));
    }

    
    
    
    
    gfxContext* renderContext = aRenderingContext.ThebesContext();
    nsContextBoxBlur blurringArea;
    gfxContext* shadowContext =
      blurringArea.Init(shadowPaintRect, 0, blurRadius, twipsPerPixel,
                        renderContext, aDirtyRect, &skipGfxRect);
    if (!shadowContext)
      continue;

    
    MOZ_ASSERT(shadowContext == renderContext ||
               shadowContext == blurringArea.GetContext());

    
    nscolor shadowColor;
    if (shadowItem->mHasColor)
      shadowColor = shadowItem->mColor;
    else
      shadowColor = aForFrame->StyleColor()->mColor;

    renderContext->Save();
    renderContext->SetColor(gfxRGBA(shadowColor));

    
    
    
    gfxRect shadowGfxRect =
      nsLayoutUtils::RectToGfxRect(paddingRect, twipsPerPixel);
    shadowGfxRect.Round();
    renderContext->NewPath();
    if (hasBorderRadius)
      renderContext->RoundedRectangle(shadowGfxRect, innerRadii, false);
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
      shadowContext->RoundedRectangle(shadowClipGfxRect, clipRectRadii, false);
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
                                uint32_t aFlags,
                                nsRect* aBGClipRect,
                                int32_t aLayer)
{
  PROFILER_LABEL("nsCSSRendering", "PaintBackground");
  NS_PRECONDITION(aForFrame,
                  "Frame is expected to be provided to PaintBackground");

  nsStyleContext *sc;
  if (!FindBackground(aForFrame, &sc)) {
    
    
    
    
    
    if (!aForFrame->StyleDisplay()->mAppearance) {
      return;
    }

    nsIContent* content = aForFrame->GetContent();
    if (!content || content->GetParent()) {
      return;
    }

    sc = aForFrame->StyleContext();
  }

  PaintBackgroundWithSC(aPresContext, aRenderingContext, aForFrame,
                        aDirtyRect, aBorderArea, sc,
                        *aForFrame->StyleBorder(), aFlags,
                        aBGClipRect, aLayer);
}

void
nsCSSRendering::PaintBackgroundColor(nsPresContext* aPresContext,
                                     nsRenderingContext& aRenderingContext,
                                     nsIFrame* aForFrame,
                                     const nsRect& aDirtyRect,
                                     const nsRect& aBorderArea,
                                     uint32_t aFlags)
{
  PROFILER_LABEL("nsCSSRendering", "PaintBackgroundColor");
  NS_PRECONDITION(aForFrame,
                  "Frame is expected to be provided to PaintBackground");

  nsStyleContext *sc;
  if (!FindBackground(aForFrame, &sc)) {
    
    
    
    
    
    if (!aForFrame->StyleDisplay()->mAppearance) {
      return;
    }

    nsIContent* content = aForFrame->GetContent();
    if (!content || content->GetParent()) {
      return;
    }

    sc = aForFrame->StyleContext();
  }

  PaintBackgroundColorWithSC(aPresContext, aRenderingContext, aForFrame,
                             aDirtyRect, aBorderArea, sc,
                             *aForFrame->StyleBorder(), aFlags);
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

  
  
  
  
  if (aBorder.GetBorderImage())
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
  NS_ABORT_IF_FALSE(!aDirtyRect->IsEmpty() || aDirtyRectGfx->IsEmpty(),
                    "second should be empty if first is");
}

struct BackgroundClipState {
  nsRect mBGClipArea;
  nsRect mDirtyRect;
  gfxRect mDirtyRectGfx;

  gfxCornerSizes mClippedRadii;
  bool mRadiiAreOuter;

  
  
  bool mCustomClip;
};

static void
GetBackgroundClip(gfxContext *aCtx, uint8_t aBackgroundClip,
                  nsIFrame* aForFrame, const nsRect& aBorderArea,
                  const nsRect& aCallerDirtyRect, bool aHaveRoundedCorners,
                  const gfxCornerSizes& aBGRadii, nscoord aAppUnitsPerPixel,
                   BackgroundClipState* aClipState)
{
  aClipState->mBGClipArea = aBorderArea;
  aClipState->mCustomClip = false;
  aClipState->mRadiiAreOuter = true;
  aClipState->mClippedRadii = aBGRadii;
  if (aBackgroundClip != NS_STYLE_BG_CLIP_BORDER) {
    nsMargin border = aForFrame->GetUsedBorder();
    if (aBackgroundClip == NS_STYLE_BG_CLIP_MOZ_ALMOST_PADDING) {
      
      
      
      border.top = std::max(0, border.top - aAppUnitsPerPixel);
      border.right = std::max(0, border.right - aAppUnitsPerPixel);
      border.bottom = std::max(0, border.bottom - aAppUnitsPerPixel);
      border.left = std::max(0, border.left - aAppUnitsPerPixel);
    } else if (aBackgroundClip != NS_STYLE_BG_CLIP_PADDING) {
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
      aClipState->mRadiiAreOuter = false;
    }
  }

  SetupDirtyRects(aClipState->mBGClipArea, aCallerDirtyRect, aAppUnitsPerPixel,
                  &aClipState->mDirtyRect, &aClipState->mDirtyRectGfx);
}

static void
SetupBackgroundClip(BackgroundClipState& aClipState, gfxContext *aCtx,
                    bool aHaveRoundedCorners, nscoord aAppUnitsPerPixel,
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
                    bool aHaveRoundedCorners, nscoord aAppUnitsPerPixel)
{
  if (aClipState.mDirtyRectGfx.IsEmpty()) {
    
    
    return;
  }

  
  
  if (!aHaveRoundedCorners || aClipState.mCustomClip) {
    aCtx->NewPath();
    aCtx->Rectangle(aClipState.mDirtyRectGfx, true);
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
  aCtx->Rectangle(dirty, true);
  aCtx->Clip();

  aCtx->NewPath();
  aCtx->RoundedRectangle(bgAreaGfx, aClipState.mClippedRadii,
                         aClipState.mRadiiAreOuter);
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
      bg->BottomLayer().mImage.IsOpaque()) {
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
    NS_ABORT_IF_FALSE(false, "unknown radial gradient sizing method");
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

void
nsCSSRendering::PaintGradient(nsPresContext* aPresContext,
                              nsRenderingContext& aRenderingContext,
                              nsStyleGradient* aGradient,
                              const nsRect& aDirtyRect,
                              const nsRect& aOneCellArea,
                              const nsRect& aFillArea)
{
  PROFILER_LABEL("nsCSSRendering", "PaintGradient");
  Telemetry::AutoTimer<Telemetry::GRADIENT_DURATION, Telemetry::Microsecond> gradientTimer;
  if (aOneCellArea.IsEmpty())
    return;

  gfxContext *ctx = aRenderingContext.ThebesContext();
  nscoord appUnitsPerPixel = aPresContext->AppUnitsPerDevPixel();
  gfxRect oneCellArea =
    nsLayoutUtils::RectToGfxRect(aOneCellArea, appUnitsPerPixel);

  bool cellContainsFill = aOneCellArea.Contains(aFillArea);

  gfx::BackendType backendType = gfx::BACKEND_NONE;
  if (ctx->IsCairo()) {
    backendType = gfx::BACKEND_CAIRO;
  } else {
    gfx::DrawTarget* dt = ctx->GetDrawTarget();
    NS_ASSERTION(dt, "If we are not using Cairo, we should have a draw target.");
    backendType = dt->GetType();
  }

  
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
    case eStyleUnit_Calc:
      nsStyleCoord::Calc *calc;
      calc = stop.mLocation.GetCalcValue();
      position = calc->mPercent +
          ((lineLength < 1e-6) ? 0.0 :
          (NSAppUnitsToFloatPixels(calc->mLength, appUnitsPerPixel) / lineLength));
      break;
    default:
      NS_ABORT_IF_FALSE(false, "Unknown stop position type");
    }

    if (i > 0) {
      
      
      position = std::max(position, stops[i - 1].mPosition);
    }
    stops.AppendElement(ColorStop(position, stop.mColor));
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
    NS_ABORT_IF_FALSE(firstStop >= 0.0, "Failed to fix stop offsets");
  }

  if (aGradient->mShape != NS_STYLE_GRADIENT_SHAPE_LINEAR && !aGradient->mRepeating) {
    
    
    
    
    
    firstStop = 0;
  }

  double lastStop = stops[stops.Length() - 1].mPosition;
  
  
  
  double stopScale;
  double stopDelta = lastStop - firstStop;
  bool zeroRadius = aGradient->mShape != NS_STYLE_GRADIENT_SHAPE_LINEAR &&
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
  bool forceRepeatToCoverTiles = false;
  if (aGradient->mShape == NS_STYLE_GRADIENT_SHAPE_LINEAR) {
    
    
    gfxPoint gradientStart = lineStart + (lineEnd - lineStart)*firstStop;
    gfxPoint gradientEnd = lineStart + (lineEnd - lineStart)*lastStop;

    if (stopScale == 0.0) {
      
      
      
      
      
      
      gradientEnd = gradientStart + (lineEnd - lineStart);
    }

    gradientPattern = new gfxPattern(gradientStart.x, gradientStart.y,
                                      gradientEnd.x, gradientEnd.y);

    
    
    
    if (!cellContainsFill &&
        ((gradientStart.y == gradientEnd.y && gradientStart.x == 0 &&
          gradientEnd.x == oneCellArea.width) ||
          (gradientStart.x == gradientEnd.x && gradientStart.y == 0 &&
          gradientEnd.y == oneCellArea.height))) {
      forceRepeatToCoverTiles = true;
    }
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
    if (radiusX != radiusY) {
      
      
      
      
      
      gfxMatrix matrix;
      matrix.Translate(lineStart);
      matrix.Scale(1.0, radiusX/radiusY);
      matrix.Translate(-lineStart);
      gradientPattern->SetMatrix(matrix);
    }
  }
  if (gradientPattern->CairoStatus())
    return;

  if (stopScale == 0.0) {
    
    
    
    
    
    gfxRGBA firstColor(stops[0].mColor);
    gfxRGBA lastColor(stops.LastElement().mColor);
    stops.Clear();

    if (!aGradient->mRepeating && !zeroRadius) {
      stops.AppendElement(ColorStop(firstStop, firstColor));
    }
    stops.AppendElement(ColorStop(firstStop, lastColor));
  }

  bool isRepeat = aGradient->mRepeating || forceRepeatToCoverTiles;

  
  if (!ctx->IsCairo()) {
    
    
    
    
    
    
    nsTArray<gfx::GradientStop> rawStops(stops.Length());
    rawStops.SetLength(stops.Length());
    for(uint32_t i = 0; i < stops.Length(); i++) {
      rawStops[i].color = gfx::Color(stops[i].mColor.r, stops[i].mColor.g, stops[i].mColor.b, stops[i].mColor.a);
      rawStops[i].offset =  stopScale * (stops[i].mPosition - firstStop);
    }
    GradientCacheData* cached = gGradientCache->Lookup(rawStops, isRepeat, backendType);
    mozilla::RefPtr<mozilla::gfx::GradientStops> gs = cached ? cached->mStops : nullptr;
    if (!gs) {
      
      gs = ctx->GetDrawTarget()->CreateGradientStops(rawStops.Elements(), stops.Length(), isRepeat ? gfx::EXTEND_REPEAT : gfx::EXTEND_CLAMP);
      cached = new GradientCacheData(gs, GradientCacheKey(rawStops, isRepeat, backendType));
      if (!gGradientCache->RegisterEntry(cached)) {
        delete cached;
      }
    }
    gradientPattern->SetColorStops(gs);
  } else {
    for (uint32_t i = 0; i < stops.Length(); i++) {
      double pos = stopScale*(stops[i].mPosition - firstStop);
      gradientPattern->AddColorStop(pos, stops[i].mColor);
    }
    
    if (isRepeat) {
      gradientPattern->SetExtend(gfxPattern::EXTEND_REPEAT);
    }
  }

  
  
  
  
  nsRect dirty;
  if (!dirty.IntersectRect(aDirtyRect, aFillArea))
    return;

  gfxRect areaToFill =
    nsLayoutUtils::RectToGfxRect(aFillArea, appUnitsPerPixel);
  gfxMatrix ctm = ctx->CurrentMatrix();
  bool isCTMPreservingAxisAlignedRectangles = ctm.PreservesAxisAlignedRectangles();

  
  nscoord xStart = FindTileStart(dirty.x, aOneCellArea.x, aOneCellArea.width);
  nscoord yStart = FindTileStart(dirty.y, aOneCellArea.y, aOneCellArea.height);
  nscoord xEnd = forceRepeatToCoverTiles ? xStart + aOneCellArea.width : dirty.XMost();
  nscoord yEnd = forceRepeatToCoverTiles ? yStart + aOneCellArea.height : dirty.YMost();

  
  for (nscoord y = yStart; y < yEnd; y += aOneCellArea.height) {
    for (nscoord x = xStart; x < xEnd; x += aOneCellArea.width) {
      
      gfxRect tileRect = nsLayoutUtils::RectToGfxRect(
                      nsRect(x, y, aOneCellArea.width, aOneCellArea.height),
                      appUnitsPerPixel);
      
      
      gfxRect fillRect =
        forceRepeatToCoverTiles ? areaToFill : tileRect.Intersect(areaToFill);
      ctx->NewPath();
      
      
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
      ctx->Rectangle(fillRect);
      ctx->Translate(tileRect.TopLeft());
      ctx->SetPattern(gradientPattern);
      ctx->Fill();
      ctx->SetMatrix(ctm);
    }
  }
}

void
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
      return;
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
    return;

  
  gfxContext *ctx = aRenderingContext.ThebesContext();
  nscoord appUnitsPerPixel = aPresContext->AppUnitsPerDevPixel();

  
  gfxCornerSizes bgRadii;
  bool haveRoundedCorners;
  {
    nscoord radii[8];
    nsSize frameSize = aForFrame->GetSize();
    if (&aBorder == aForFrame->StyleBorder() &&
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

  
  
  
  
  
  
  BackgroundClipState clipState;
  uint8_t currentBackgroundClip;
  bool isSolidBorder;
  if (aBGClipRect) {
    clipState.mBGClipArea = *aBGClipRect;
    clipState.mCustomClip = true;
    SetupDirtyRects(clipState.mBGClipArea, aDirtyRect, appUnitsPerPixel,
                    &clipState.mDirtyRect, &clipState.mDirtyRectGfx);
  } else {
    
    
    
    
    
    
    
    
    currentBackgroundClip = bg->BottomLayer().mClip;
    isSolidBorder =
      (aFlags & PAINTBG_WILL_PAINT_BORDER) && IsOpaqueBorder(aBorder);
    if (isSolidBorder && currentBackgroundClip == NS_STYLE_BG_CLIP_BORDER) {
      
      
      
      currentBackgroundClip = haveRoundedCorners ?
        NS_STYLE_BG_CLIP_MOZ_ALMOST_PADDING : NS_STYLE_BG_CLIP_PADDING;
    }

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

  if (bg->mImageCount < 1) {
    
    
    return;
  }

  
  int32_t startLayer = aLayer;
  int32_t nLayers = 1;
  if (startLayer < 0) {
    startLayer = (int32_t)bg->mImageCount - 1;
    nLayers = bg->mImageCount;
  }

  
  
  
  if (aBackgroundSC != aForFrame->StyleContext()) {
    ImageLoader* loader = aPresContext->Document()->StyleImageLoader();

    NS_FOR_VISIBLE_BACKGROUND_LAYERS_BACK_TO_FRONT_WITH_RANGE(i, bg, startLayer, nLayers) {
      if (bg->mLayers[i].mImage.GetType() == eStyleImageType_Image) {
        imgIRequest *image = bg->mLayers[i].mImage.GetImageData();

        loader->AssociateRequestToFrame(image, aForFrame);
      }
    }
  }

  
  
  if (drawBackgroundColor && !isCanvasFrame) {
    DrawBackgroundColor(clipState, ctx, haveRoundedCorners, appUnitsPerPixel);
  }

  if (drawBackgroundImage) {
    bool clipSet = false;
    NS_FOR_VISIBLE_BACKGROUND_LAYERS_BACK_TO_FRONT_WITH_RANGE(i, bg, bg->mImageCount - 1,
                                                              nLayers + (bg->mImageCount -
                                                                         startLayer - 1)) {
      const nsStyleBackground::Layer &layer = bg->mLayers[i];
      if (!aBGClipRect) {
        uint8_t newBackgroundClip = layer.mClip;
        if (isSolidBorder && newBackgroundClip == NS_STYLE_BG_CLIP_BORDER) {
          newBackgroundClip = haveRoundedCorners ?
            NS_STYLE_BG_CLIP_MOZ_ALMOST_PADDING : NS_STYLE_BG_CLIP_PADDING;
        }
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
      if ((aLayer < 0 || i == (uint32_t)startLayer) &&
          !clipState.mDirtyRectGfx.IsEmpty()) {
        nsBackgroundLayerState state = PrepareBackgroundLayer(aPresContext, aForFrame,
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

void
nsCSSRendering::PaintBackgroundColorWithSC(nsPresContext* aPresContext,
                                           nsRenderingContext& aRenderingContext,
                                           nsIFrame* aForFrame,
                                           const nsRect& aDirtyRect,
                                           const nsRect& aBorderArea,
                                           nsStyleContext* aBackgroundSC,
                                           const nsStyleBorder& aBorder,
                                           uint32_t aFlags)
{
  NS_PRECONDITION(aForFrame,
                  "Frame is expected to be provided to PaintBackground");

  
  
  const nsStyleDisplay* displayData = aForFrame->StyleDisplay();
  if (displayData->mAppearance) {
    nsITheme *theme = aPresContext->GetTheme();
    if (theme && theme->ThemeSupportsWidget(aPresContext, aForFrame,
                                            displayData->mAppearance)) {
      NS_ERROR("Shouldn't be trying to paint a background color if we are themed!");
      return;
    }
  }

  NS_ASSERTION(!IsCanvasFrame(aForFrame), "Should not be trying to paint a background color for canvas frames!");

  
  
  bool drawBackgroundImage;
  bool drawBackgroundColor;

  nscolor bgColor = DetermineBackgroundColor(aPresContext,
                                             aBackgroundSC,
                                             aForFrame,
                                             drawBackgroundImage,
                                             drawBackgroundColor);

  NS_ASSERTION(drawBackgroundColor, "Should not be trying to paint a background color if we don't have one");

  
  gfxContext *ctx = aRenderingContext.ThebesContext();
  nscoord appUnitsPerPixel = aPresContext->AppUnitsPerDevPixel();

  
  gfxCornerSizes bgRadii;
  bool haveRoundedCorners;
  {
    nscoord radii[8];
    nsSize frameSize = aForFrame->GetSize();
    if (&aBorder == aForFrame->StyleBorder() &&
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

  
  
  
  
  
  
  
  
  const nsStyleBackground *bg = aBackgroundSC->StyleBackground();
  uint8_t currentBackgroundClip = bg->BottomLayer().mClip;
  bool isSolidBorder =
    (aFlags & PAINTBG_WILL_PAINT_BORDER) && IsOpaqueBorder(aBorder);
  if (isSolidBorder && currentBackgroundClip == NS_STYLE_BG_CLIP_BORDER) {
    
    
    
    currentBackgroundClip = haveRoundedCorners ?
      NS_STYLE_BG_CLIP_MOZ_ALMOST_PADDING : NS_STYLE_BG_CLIP_PADDING;
  }

  BackgroundClipState clipState;
  GetBackgroundClip(ctx, currentBackgroundClip, aForFrame, aBorderArea,
                    aDirtyRect, haveRoundedCorners, bgRadii, appUnitsPerPixel,
                    &clipState);

  ctx->SetColor(gfxRGBA(bgColor));

  gfxContextAutoSaveRestore autoSR;
  DrawBackgroundColor(clipState, ctx, haveRoundedCorners, appUnitsPerPixel);
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
                                                 const nsStyleBackground& aBackground,
                                                 const nsStyleBackground::Layer& aLayer,
                                                 nsIFrame** aAttachedToFrame)
{
  
  
  nsRect bgPositioningArea(0, 0, 0, 0);

  nsIAtom* frameType = aForFrame->GetType();
  nsIFrame* geometryFrame = aForFrame;
  if (frameType == nsGkAtoms::inlineFrame) {
    
    
    
    
    
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
    geometryFrame->ApplySkipSides(border);
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

nsBackgroundLayerState
nsCSSRendering::PrepareBackgroundLayer(nsPresContext* aPresContext,
                                       nsIFrame* aForFrame,
                                       uint32_t aFlags,
                                       const nsRect& aBorderArea,
                                       const nsRect& aBGClipRect,
                                       const nsStyleBackground& aBackground,
                                       const nsStyleBackground::Layer& aLayer)
{
  






















































  uint32_t irFlags = 0;
  if (aFlags & nsCSSRendering::PAINTBG_SYNC_DECODE_IMAGES) {
    irFlags |= nsImageRenderer::FLAG_SYNC_DECODE_IMAGES;
  }

  nsBackgroundLayerState state(aForFrame, &aLayer.mImage, irFlags);
  if (!state.mImageRenderer.PrepareImage()) {
    
    return state;
  }

  
  nsIFrame* attachedToFrame = aForFrame;
  
  
  nsRect bgPositioningArea =
    ComputeBackgroundPositioningArea(aPresContext, aForFrame, aBorderArea,
                                     aBackground, aLayer, &attachedToFrame);

  
  
  nsRect bgClipRect = aBGClipRect;

  
  
  
  
  nsPoint imageTopLeft;
  if (NS_STYLE_BG_ATTACHMENT_FIXED == aLayer.mAttachment) {
    if ((aFlags & nsCSSRendering::PAINTBG_TO_WINDOW) &&
        !IsTransformed(aForFrame, attachedToFrame)) {
      
      
      
      
      
      
      
      bgClipRect.IntersectRect(bgClipRect, bgPositioningArea + aBorderArea.TopLeft());
    }
  }

  
  
  
  nsSize imageSize = state.mImageRenderer.ComputeSize(aLayer.mSize, bgPositioningArea.Size());
  if (imageSize.width <= 0 || imageSize.height <= 0)
    return state;

  
  
  ComputeBackgroundAnchorPoint(aLayer, bgPositioningArea.Size(), imageSize,
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
  return state;
}

nsRect
nsCSSRendering::GetBackgroundLayerRect(nsPresContext* aPresContext,
                                       nsIFrame* aForFrame,
                                       const nsRect& aBorderArea,
                                       const nsRect& aClipRect,
                                       const nsStyleBackground& aBackground,
                                       const nsStyleBackground::Layer& aLayer)
{
  nsBackgroundLayerState state =
      PrepareBackgroundLayer(aPresContext, aForFrame, 0, aBorderArea,
                             aClipRect, aBackground, aLayer);
  return state.mFillArea;
}

static void
DrawBorderImage(nsPresContext*       aPresContext,
                nsRenderingContext&  aRenderingContext,
                nsIFrame*            aForFrame,
                const nsRect&        aBorderArea,
                const nsStyleBorder& aStyleBorder,
                const nsRect&        aDirtyRect)
{
  NS_PRECONDITION(aStyleBorder.IsBorderImageLoaded(),
                  "drawing border image that isn't successfully loaded");

  if (aDirtyRect.IsEmpty())
    return;

  
  
  
  
  
  
  imgIRequest *req = aStyleBorder.GetBorderImage();
  ImageLoader* loader = aPresContext->Document()->StyleImageLoader();

  
  loader->AssociateRequestToFrame(req, aForFrame);

  

  nsCOMPtr<imgIContainer> imgContainer;
  DebugOnly<nsresult> rv = req->GetImage(getter_AddRefs(imgContainer));
  NS_ASSERTION(NS_SUCCEEDED(rv) && imgContainer, "no image to draw");

  nsIntSize imageSize;
  if (NS_FAILED(imgContainer->GetWidth(&imageSize.width))) {
    imageSize.width =
      nsPresContext::AppUnitsToIntCSSPixels(aBorderArea.width);
  }
  if (NS_FAILED(imgContainer->GetHeight(&imageSize.height))) {
    imageSize.height =
      nsPresContext::AppUnitsToIntCSSPixels(aBorderArea.height);
  }

  
  
  nsRect borderImgArea(aBorderArea);
  borderImgArea.Inflate(aStyleBorder.GetImageOutset());

  
  
  nsIntMargin slice;
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
        value = coord.GetFactorValue();
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
    slice.Side(s) = NS_lround(value);

    nsMargin borderWidths(aStyleBorder.GetComputedBorder());
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
        value = nsPresContext::CSSPixelsToAppUnits(slice.Side(s));
        break;
      default:
        NS_NOTREACHED("unexpected CSS unit for border image area division");
        value = 0;
        break;
    }
    border.Side(s) = NS_lround(value);
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
      nsRect destArea(borderX[i], borderY[j], borderWidth[i], borderHeight[j]);
      nsIntRect subArea(sliceX[i], sliceY[j], sliceWidth[i], sliceHeight[j]);

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
          vFactor = nsPresContext::CSSPixelsToAppUnits(1);

        if (0 < border.top && 0 < slice.top)
          hFactor = gfxFloat(border.top)/slice.top;
        else if (0 < border.bottom && 0 < slice.bottom)
          hFactor = gfxFloat(border.bottom)/slice.bottom;
        else
          hFactor = nsPresContext::CSSPixelsToAppUnits(1);

        unitSize.width = sliceWidth[i]*hFactor;
        unitSize.height = sliceHeight[j]*vFactor;
        fillStyleH = aStyleBorder.mBorderImageRepeatH;
        fillStyleV = aStyleBorder.mBorderImageRepeatV;

      } else if (i == MIDDLE) { 
        
        
        gfxFloat factor;
        if (0 < borderHeight[j] && 0 < sliceHeight[j])
          factor = gfxFloat(borderHeight[j])/sliceHeight[j];
        else
          factor = nsPresContext::CSSPixelsToAppUnits(1);

        unitSize.width = sliceWidth[i]*factor;
        unitSize.height = borderHeight[j];
        fillStyleH = aStyleBorder.mBorderImageRepeatH;
        fillStyleV = NS_STYLE_BORDER_IMAGE_REPEAT_STRETCH;

      } else if (j == MIDDLE) { 
        gfxFloat factor;
        if (0 < borderWidth[i] && 0 < sliceWidth[i])
          factor = gfxFloat(borderWidth[i])/sliceWidth[i];
        else
          factor = nsPresContext::CSSPixelsToAppUnits(1);

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

      DrawBorderImageComponent(aRenderingContext, aForFrame,
                               imgContainer, aDirtyRect,
                               destArea, subArea,
                               fillStyleH, fillStyleV,
                               unitSize, aStyleBorder, i * (RIGHT + 1) + j);
    }
  }
}

static void
DrawBorderImageComponent(nsRenderingContext&  aRenderingContext,
                         nsIFrame*            aForFrame,
                         imgIContainer*       aImage,
                         const nsRect&        aDirtyRect,
                         const nsRect&        aFill,
                         const nsIntRect&     aSrc,
                         uint8_t              aHFill,
                         uint8_t              aVFill,
                         const nsSize&        aUnitSize,
                         const nsStyleBorder& aStyleBorder,
                         uint8_t              aIndex)
{
  if (aFill.IsEmpty() || aSrc.IsEmpty())
    return;

  nsCOMPtr<imgIContainer> subImage;
  if ((subImage = aStyleBorder.GetSubImage(aIndex)) == nullptr) {
    subImage = ImageOps::Clip(aImage, aSrc);
    aStyleBorder.SetSubImage(aIndex, subImage);
  }

  gfxPattern::GraphicsFilter graphicsFilter =
    nsLayoutUtils::GetGraphicsFilterForFrame(aForFrame);

  
  
  if ((aHFill == NS_STYLE_BORDER_IMAGE_REPEAT_STRETCH &&
       aVFill == NS_STYLE_BORDER_IMAGE_REPEAT_STRETCH) ||
      (aUnitSize.width == aFill.width &&
       aUnitSize.height == aFill.height)) {
    nsLayoutUtils::DrawSingleImage(&aRenderingContext, subImage,
                                   graphicsFilter, aFill, aDirtyRect,
                                   nullptr, imgIContainer::FLAG_NONE);
    return;
  }

  
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

  nsLayoutUtils::DrawImage(&aRenderingContext, subImage, graphicsFilter,
                           tile, aFill, tile.TopLeft(), aDirtyRect,
                           imgIContainer::FLAG_NONE);
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
                       uint8_t              aStartBevelSide = 0,
                       nscoord              aStartBevelOffset = 0,
                       uint8_t              aEndBevelSide = 0,
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
                                       int32_t                  aAppUnitsPerCSSPixel,
                                       uint8_t                  aStartBevelSide,
                                       nscoord                  aStartBevelOffset,
                                       uint8_t                  aEndBevelSide,
                                       nscoord                  aEndBevelOffset)
{
  aContext.SetColor (aBorderColor);

  bool horizontal = ((NS_SIDE_TOP == aStartBevelSide) || (NS_SIDE_BOTTOM == aStartBevelSide));
  nscoord twipsPerPixel = NSIntPixelsToAppUnits(1, aAppUnitsPerCSSPixel);
  uint8_t ridgeGroove = NS_STYLE_BORDER_STYLE_RIDGE;

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
      minDashLength = std::max(minDashLength, twipsPerPixel);
      nscoord numDashSpaces = 0;
      nscoord startDashLength = minDashLength;
      nscoord endDashLength   = minDashLength;
      if (horizontal) {
        GetDashInfo(aBorder.width, dashLength, twipsPerPixel, numDashSpaces, startDashLength, endDashLength);
        nsRect rect(aBorder.x, aBorder.y, startDashLength, aBorder.height);
        DrawSolidBorderSegment(aContext, rect, twipsPerPixel);
        for (int32_t spaceX = 0; spaceX < numDashSpaces; spaceX++) {
          rect.x += rect.width + dashLength;
          rect.width = (spaceX == (numDashSpaces - 1)) ? endDashLength : dashLength;
          DrawSolidBorderSegment(aContext, rect, twipsPerPixel);
        }
      }
      else {
        GetDashInfo(aBorder.height, dashLength, twipsPerPixel, numDashSpaces, startDashLength, endDashLength);
        nsRect rect(aBorder.x, aBorder.y, aBorder.width, startDashLength);
        DrawSolidBorderSegment(aContext, rect, twipsPerPixel);
        for (int32_t spaceY = 0; spaceY < numDashSpaces; spaceY++) {
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
                            ? RoundFloatToPixel(0.5f * (float)aStartBevelOffset, twipsPerPixel, true) : 0;
      nscoord endBevel =   (aEndBevelOffset > 0)
                            ? RoundFloatToPixel(0.5f * (float)aEndBevelOffset, twipsPerPixel, true) : 0;
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
    NS_ASSERTION(false, "inset, outset should have been converted to groove, ridge");
    break;
  case NS_STYLE_BORDER_STYLE_AUTO:
    NS_ASSERTION(false, "Unexpected 'auto' table border");
    break;
  }

  ctx->SetAntialiasMode(oldMode);
}



gfxRect
nsCSSRendering::ExpandPaintingRectForDecorationLine(nsIFrame* aFrame,
                                                    const uint8_t aStyle,
                                                    const gfxRect& aClippedRect,
                                                    const gfxFloat aXInFrame,
                                                    const gfxFloat aCycleLength)
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
  
  
  nscoord relativeX = 0;
  for (nsIFrame* f = aFrame; f; f = f->GetParent()) {
    block = do_QueryFrame(f);
    if (block) {
      break;
    }
    relativeX += f->GetRelativeOffset(f->StyleDisplay()).x;
  }

  NS_ENSURE_TRUE(block, aClippedRect);

  nscoord frameXInBlockAppUnits = aFrame->GetOffsetTo(block).x - relativeX;
  nsPresContext *pc = aFrame->PresContext();
  gfxFloat frameXInBlock = pc->AppUnitsToGfxUnits(frameXInBlockAppUnits);
  int32_t rectXInBlock = int32_t(NS_round(frameXInBlock + aXInFrame));
  int32_t extraLeft =
    rectXInBlock - (rectXInBlock / int32_t(aCycleLength) * aCycleLength);
  gfxRect rect(aClippedRect);
  rect.x -= extraLeft;
  rect.width += extraLeft;
  return rect;
}

void
nsCSSRendering::PaintDecorationLine(nsIFrame* aFrame,
                                    gfxContext* aGfxContext,
                                    const gfxRect& aDirtyRect,
                                    const nscolor aColor,
                                    const gfxPoint& aPt,
                                    const gfxFloat aXInFrame,
                                    const gfxSize& aLineSize,
                                    const gfxFloat aAscent,
                                    const gfxFloat aOffset,
                                    const uint8_t aDecoration,
                                    const uint8_t aStyle,
                                    const gfxFloat aDescentLimit)
{
  NS_ASSERTION(aStyle != NS_STYLE_TEXT_DECORATION_STYLE_NONE, "aStyle is none");

  gfxRect rect =
    GetTextDecorationRectInternal(aPt, aLineSize, aAscent, aOffset,
                                  aDecoration, aStyle, aDescentLimit);
  if (rect.IsEmpty() || !rect.Intersects(aDirtyRect)) {
    return;
  }

  if (aDecoration != NS_STYLE_TEXT_DECORATION_LINE_UNDERLINE &&
      aDecoration != NS_STYLE_TEXT_DECORATION_LINE_OVERLINE &&
      aDecoration != NS_STYLE_TEXT_DECORATION_LINE_LINE_THROUGH) {
    NS_ERROR("Invalid decoration value!");
    return;
  }

  gfxFloat lineHeight = std::max(NS_round(aLineSize.height), 1.0);
  bool contextIsSaved = false;

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
      contextIsSaved = true;
      aGfxContext->Clip(rect);
      gfxFloat dashWidth = lineHeight * DOT_LENGTH * DASH_LENGTH;
      gfxFloat dash[2] = { dashWidth, dashWidth };
      aGfxContext->SetLineCap(gfxContext::LINE_CAP_BUTT);
      aGfxContext->SetDash(dash, 2, 0.0);
      rect = ExpandPaintingRectForDecorationLine(aFrame, aStyle, rect,
                                                 aXInFrame, dashWidth * 2);
      
      rect.width += dashWidth;
      break;
    }
    case NS_STYLE_TEXT_DECORATION_STYLE_DOTTED: {
      aGfxContext->Save();
      contextIsSaved = true;
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
      rect = ExpandPaintingRectForDecorationLine(aFrame, aStyle, rect,
                                                 aXInFrame, dashWidth * 2);
      
      rect.width += dashWidth;
      break;
    }
    case NS_STYLE_TEXT_DECORATION_STYLE_WAVY:
      aGfxContext->Save();
      contextIsSaved = true;
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
      




























      gfxFloat adv = rect.Height() - lineHeight;
      gfxFloat flatLengthAtVertex = std::max((lineHeight - 1.0) * 2.0, 1.0);

      
      gfxFloat cycleLength = 2 * (adv + flatLengthAtVertex);
      rect = ExpandPaintingRectForDecorationLine(aFrame, aStyle, rect,
                                                 aXInFrame, cycleLength);
      
      
      
      int32_t skipCycles = floor((aDirtyRect.x - rect.x) / cycleLength);
      if (skipCycles > 0) {
        rect.x += skipCycles * cycleLength;
        rect.width -= skipCycles * cycleLength;
      }

      rect.x += lineHeight / 2.0;
      gfxPoint pt(rect.TopLeft());
      gfxFloat rightMost = pt.x + rect.Width() + lineHeight;

      skipCycles = floor((rightMost - aDirtyRect.XMost()) / cycleLength);
      if (skipCycles > 0) {
        rightMost -= skipCycles * cycleLength;
      }

      aGfxContext->NewPath();

      pt.x -= lineHeight;
      aGfxContext->MoveTo(pt); 

      pt.x = rect.X();
      aGfxContext->LineTo(pt); 

      bool goDown = true;
      uint32_t iter = 0;
      while (pt.x < rightMost) {
        if (++iter > 1000) {
          
          
          aGfxContext->Stroke();
          aGfxContext->NewPath();
          aGfxContext->MoveTo(pt);
          iter = 0;
        }
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

void
nsCSSRendering::DecorationLineToPath(nsIFrame* aFrame,
                                     gfxContext* aGfxContext,
                                     const gfxRect& aDirtyRect,
                                     const nscolor aColor,
                                     const gfxPoint& aPt,
                                     const gfxFloat aXInFrame,
                                     const gfxSize& aLineSize,
                                     const gfxFloat aAscent,
                                     const gfxFloat aOffset,
                                     const uint8_t aDecoration,
                                     const uint8_t aStyle,
                                     const gfxFloat aDescentLimit)
{
  NS_ASSERTION(aStyle != NS_STYLE_TEXT_DECORATION_STYLE_NONE, "aStyle is none");

  aGfxContext->NewPath();

  gfxRect rect =
    GetTextDecorationRectInternal(aPt, aLineSize, aAscent, aOffset,
                                  aDecoration, aStyle, aDescentLimit);
  if (rect.IsEmpty() || !rect.Intersects(aDirtyRect)) {
    return;
  }

  if (aDecoration != NS_STYLE_TEXT_DECORATION_LINE_UNDERLINE &&
      aDecoration != NS_STYLE_TEXT_DECORATION_LINE_OVERLINE &&
      aDecoration != NS_STYLE_TEXT_DECORATION_LINE_LINE_THROUGH) {
    NS_ERROR("Invalid decoration value!");
    return;
  }

  if (aStyle != NS_STYLE_TEXT_DECORATION_STYLE_SOLID) {
    
    return;
  }

  gfxFloat lineHeight = std::max(NS_round(aLineSize.height), 1.0);

  
  rect.y += lineHeight / 2;

  aGfxContext->Rectangle
    (gfxRect(gfxPoint(rect.TopLeft() - gfxPoint(0.0, lineHeight / 2)),
             gfxSize(rect.Width(), lineHeight)));
}

nsRect
nsCSSRendering::GetTextDecorationRect(nsPresContext* aPresContext,
                                      const gfxSize& aLineSize,
                                      const gfxFloat aAscent,
                                      const gfxFloat aOffset,
                                      const uint8_t aDecoration,
                                      const uint8_t aStyle,
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
                                              const uint8_t aDecoration,
                                              const uint8_t aStyle,
                                              const gfxFloat aDescentLimit)
{
  NS_ASSERTION(aStyle <= NS_STYLE_TEXT_DECORATION_STYLE_WAVY,
               "Invalid aStyle value");

  if (aStyle == NS_STYLE_TEXT_DECORATION_STYLE_NONE)
    return gfxRect(0, 0, 0, 0);

  bool canLiftUnderline = aDescentLimit >= 0.0;

  const gfxFloat left  = floor(aPt.x + 0.5),
                 right = floor(aPt.x + aLineSize.width + 0.5);
  gfxRect r(left, 0, right - left, 0);

  gfxFloat lineHeight = NS_round(aLineSize.height);
  lineHeight = std::max(lineHeight, 1.0);

  gfxFloat ascent = NS_round(aAscent);
  gfxFloat descentLimit = floor(aDescentLimit);

  gfxFloat suggestedMaxRectHeight = std::max(std::min(ascent, descentLimit), 1.0);
  r.height = lineHeight;
  if (aStyle == NS_STYLE_TEXT_DECORATION_STYLE_DOUBLE) {
    














    gfxFloat gap = NS_round(lineHeight / 2.0);
    gap = std::max(gap, 1.0);
    r.height = lineHeight * 2.0 + gap;
    if (canLiftUnderline) {
      if (r.Height() > suggestedMaxRectHeight) {
        
        
        r.height = std::max(suggestedMaxRectHeight, lineHeight * 2.0 + 1.0);
      }
    }
  } else if (aStyle == NS_STYLE_TEXT_DECORATION_STYLE_WAVY) {
    












    r.height = lineHeight > 2.0 ? lineHeight * 4.0 : lineHeight * 3.0;
    if (canLiftUnderline) {
      if (r.Height() > suggestedMaxRectHeight) {
        
        
        
        
        r.height = std::max(suggestedMaxRectHeight, lineHeight * 2.0);
      }
    }
  }

  gfxFloat baseline = floor(aPt.y + aAscent + 0.5);
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
      offset = aOffset - lineHeight + r.Height();
      break;
    case NS_STYLE_TEXT_DECORATION_LINE_LINE_THROUGH: {
      gfxFloat extra = floor(r.Height() / 2.0 + 0.5);
      extra = std::max(extra, lineHeight);
      offset = aOffset - lineHeight + extra;
      break;
    }
    default:
      NS_ERROR("Invalid decoration value!");
  }
  r.y = baseline - floor(offset + 0.5);
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
      NS_ABORT_IF_FALSE(NS_SUCCEEDED(rv) && srcImage,
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
          targetURI, mForFrame->GetFirstContinuation(),
          nsSVGEffects::BackgroundImageProperty());
      if (!property)
        return false;
      mPaintServerFrame = property->GetReferencedFrame();

      
      
      if (!mPaintServerFrame) {
        mImageElementSurface =
          nsLayoutUtils::SurfaceFromElement(property->GetReferencedElement());
        if (!mImageElementSurface.mSurface)
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

enum FitType { CONTAIN, COVER };

static nsSize
ComputeContainCoverSizeFromRatio(const nsSize& aBgPositioningArea,
                                 const nsSize& aRatio, FitType fitType)
{
  NS_ABORT_IF_FALSE(aRatio.width > 0, "width division by zero");
  NS_ABORT_IF_FALSE(aRatio.height > 0, "height division by zero");

  float scaleX = double(aBgPositioningArea.width) / aRatio.width;
  float scaleY = double(aBgPositioningArea.height) / aRatio.height;
  nsSize size;
  if ((fitType == CONTAIN) == (scaleX < scaleY)) {
    size.width = aBgPositioningArea.width;
    size.height = NSCoordSaturatingNonnegativeMultiply(aRatio.height, scaleX);
  } else {
    size.width = NSCoordSaturatingNonnegativeMultiply(aRatio.width, scaleY);
    size.height = aBgPositioningArea.height;
  }
  return size;
}

void
nsImageRenderer::ComputeUnscaledDimensions(const nsSize& aBgPositioningArea,
                                           nscoord& aUnscaledWidth, bool& aHaveWidth,
                                           nscoord& aUnscaledHeight, bool& aHaveHeight,
                                           nsSize& aRatio)
{
  NS_ASSERTION(mIsReady, "Ensure PrepareImage() has returned true "
                         "before calling me");

  switch (mType) {
    case eStyleImageType_Image:
    {
      nsIntSize imageIntSize;
      nsLayoutUtils::ComputeSizeForDrawing(mImageContainer, imageIntSize,
                                           aRatio, aHaveWidth, aHaveHeight);
      if (aHaveWidth) {
        aUnscaledWidth = nsPresContext::CSSPixelsToAppUnits(imageIntSize.width);
      }
      if (aHaveHeight) {
        aUnscaledHeight = nsPresContext::CSSPixelsToAppUnits(imageIntSize.height);
      }
      return;
    }
    case eStyleImageType_Gradient:
      
      
      aHaveWidth = aHaveHeight = false;
      aRatio = nsSize(0, 0);
      return;
    case eStyleImageType_Element:
    {
      
      
      
      
      
      
      aHaveWidth = aHaveHeight = true;
      nsSize size;
      if (mPaintServerFrame) {
        if (mPaintServerFrame->IsFrameOfType(nsIFrame::eSVG)) {
          size = aBgPositioningArea;
        } else {
          
          
          
          int32_t appUnitsPerDevPixel =
            mForFrame->PresContext()->AppUnitsPerDevPixel();
          size =
            nsSVGIntegrationUtils::GetContinuationUnionSize(mPaintServerFrame).
              ToNearestPixels(appUnitsPerDevPixel).
              ToAppUnits(appUnitsPerDevPixel);
        }
      } else {
        NS_ASSERTION(mImageElementSurface.mSurface, "Surface should be ready.");
        gfxIntSize surfaceSize = mImageElementSurface.mSize;
        size.width = nsPresContext::CSSPixelsToAppUnits(surfaceSize.width);
        size.height = nsPresContext::CSSPixelsToAppUnits(surfaceSize.height);
      }
      aRatio = size;
      aUnscaledWidth = size.width;
      aUnscaledHeight = size.height;
      return;
    }
    case eStyleImageType_Null:
    default:
      aHaveWidth = aHaveHeight = true;
      aUnscaledWidth = aUnscaledHeight = 0;
      aRatio = nsSize(0, 0);
      return;
  }
}

nsSize
nsImageRenderer::ComputeDrawnSize(const nsStyleBackground::Size& aLayerSize,
                                  const nsSize& aBgPositioningArea,
                                  nscoord aUnscaledWidth, bool aHaveWidth,
                                  nscoord aUnscaledHeight, bool aHaveHeight,
                                  const nsSize& aIntrinsicRatio)
{
  NS_ABORT_IF_FALSE(aIntrinsicRatio.width >= 0,
                    "image ratio with nonsense width");
  NS_ABORT_IF_FALSE(aIntrinsicRatio.height >= 0,
                    "image ratio with nonsense height");

  
  if ((aHaveWidth && aUnscaledWidth <= 0) ||
      (aHaveHeight && aUnscaledHeight <= 0)) {
    return nsSize(0, 0);
  }

  
  
  bool haveRatio = aIntrinsicRatio != nsSize(0, 0);
  if (haveRatio &&
      (aIntrinsicRatio.width == 0 || aIntrinsicRatio.height == 0)) {
    return nsSize(0, 0);
  }

  
  if (aLayerSize.mWidthType == nsStyleBackground::Size::eLengthPercentage &&
      aLayerSize.mHeightType == nsStyleBackground::Size::eLengthPercentage) {
    return nsSize(aLayerSize.ResolveWidthLengthPercentage(aBgPositioningArea),
                  aLayerSize.ResolveHeightLengthPercentage(aBgPositioningArea));
  }

  
  if (aLayerSize.mWidthType == nsStyleBackground::Size::eContain ||
      aLayerSize.mWidthType == nsStyleBackground::Size::eCover) {
    FitType fitType = aLayerSize.mWidthType == nsStyleBackground::Size::eCover
                    ? COVER
                    : CONTAIN;
    if (!haveRatio) {
      
      
      
      return aBgPositioningArea;
    }

    return ComputeContainCoverSizeFromRatio(aBgPositioningArea, aIntrinsicRatio,
                                            fitType);
  }

  
  if (aLayerSize.mWidthType == nsStyleBackground::Size::eAuto &&
      aLayerSize.mHeightType == nsStyleBackground::Size::eAuto) {
    
    if (aHaveWidth && aHaveHeight)
      return nsSize(aUnscaledWidth, aUnscaledHeight);

    
    if (!aHaveWidth && !aHaveHeight) {
      if (!haveRatio) {
        
        return aBgPositioningArea;
      }

      
      return ComputeContainCoverSizeFromRatio(aBgPositioningArea,
                                              aIntrinsicRatio, CONTAIN);
    }

    NS_ABORT_IF_FALSE(aHaveWidth != aHaveHeight, "logic error");

    if (haveRatio) {
      
      nsSize size;
      if (aHaveWidth) {
        size.width = aUnscaledWidth;
        size.height =
          NSCoordSaturatingNonnegativeMultiply(size.width,
                                               double(aIntrinsicRatio.height) /
                                               aIntrinsicRatio.width);
      } else {
        size.height = aUnscaledHeight;
        size.width =
          NSCoordSaturatingNonnegativeMultiply(size.height,
                                               double(aIntrinsicRatio.width) /
                                               aIntrinsicRatio.height);
      }

      return size;
    }

    
    
    return aHaveWidth ? nsSize(aUnscaledWidth, aBgPositioningArea.height)
                      : nsSize(aBgPositioningArea.width, aUnscaledHeight);
  }

  
  
  NS_ABORT_IF_FALSE((aLayerSize.mWidthType == nsStyleBackground::Size::eAuto) !=
                    (aLayerSize.mHeightType == nsStyleBackground::Size::eAuto),
                    "logic error");

  bool isAutoWidth = aLayerSize.mWidthType == nsStyleBackground::Size::eAuto;

  if (haveRatio) {
    
    NS_ABORT_IF_FALSE(aIntrinsicRatio.width > 0,
                      "ratio width out of sync with width?");
    NS_ABORT_IF_FALSE(aIntrinsicRatio.height > 0,
                      "ratio height out of sync with width?");
    nsSize size;
    if (isAutoWidth) {
      size.height = aLayerSize.ResolveHeightLengthPercentage(aBgPositioningArea);
      size.width =
        NSCoordSaturatingNonnegativeMultiply(size.height,
                                             double(aIntrinsicRatio.width) /
                                             aIntrinsicRatio.height);
    } else {
      size.width = aLayerSize.ResolveWidthLengthPercentage(aBgPositioningArea);
      size.height =
        NSCoordSaturatingNonnegativeMultiply(size.width,
                                             double(aIntrinsicRatio.height) /
                                             aIntrinsicRatio.width);
    }

    return size;
  }

  NS_ABORT_IF_FALSE(!(aHaveWidth && aHaveHeight),
                    "if we have width and height, we must have had a ratio");

  
  
  
  nsSize size;
  if (isAutoWidth) {
    size.width = aHaveWidth ? aUnscaledWidth : aBgPositioningArea.width;
    size.height = aLayerSize.ResolveHeightLengthPercentage(aBgPositioningArea);
  } else {
    size.width = aLayerSize.ResolveWidthLengthPercentage(aBgPositioningArea);
    size.height = aHaveHeight ? aUnscaledHeight : aBgPositioningArea.height;
  }

  return size;
}






















nsSize
nsImageRenderer::ComputeSize(const nsStyleBackground::Size& aLayerSize,
                             const nsSize& aBgPositioningArea)
{
  bool haveWidth, haveHeight;
  nsSize ratio;
  nscoord unscaledWidth, unscaledHeight;
  ComputeUnscaledDimensions(aBgPositioningArea,
                            unscaledWidth, haveWidth,
                            unscaledHeight, haveHeight,
                            ratio);
  nsSize drawnSize = ComputeDrawnSize(aLayerSize, aBgPositioningArea,
                                      unscaledWidth, haveWidth,
                                      unscaledHeight, haveHeight,
                                      ratio);
  mSize.width = haveWidth ? unscaledWidth : drawnSize.width;
  mSize.height = haveHeight ? unscaledHeight : drawnSize.height;
  return drawnSize;
}

void
nsImageRenderer::Draw(nsPresContext*       aPresContext,
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
      uint32_t drawFlags = (mFlags & FLAG_SYNC_DECODE_IMAGES)
                             ? (uint32_t) imgIContainer::FLAG_SYNC_DECODE
                             : (uint32_t) imgIContainer::FLAG_NONE;
      nsLayoutUtils::DrawBackgroundImage(&aRenderingContext, mImageContainer,
          nsIntSize(nsPresContext::AppUnitsToIntCSSPixels(mSize.width),
                    nsPresContext::AppUnitsToIntCSSPixels(mSize.height)),
          graphicsFilter,
          aDest, aFill, aAnchor, aDirty, drawFlags);
      break;
    }
    case eStyleImageType_Gradient:
      nsCSSRendering::PaintGradient(aPresContext, aRenderingContext,
          mGradientData, aDirty, aDest, aFill);
      break;
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
    case eStyleImageType_Null:
    default:
      break;
  }
}

bool
nsImageRenderer::IsRasterImage()
{
  if (mType != eStyleImageType_Image || !mImageContainer)
    return false;
  return mImageContainer->GetType() == imgIContainer::TYPE_RASTER;
}

already_AddRefed<mozilla::layers::ImageContainer>
nsImageRenderer::GetContainer(LayerManager* aManager)
{
  if (mType != eStyleImageType_Image || !mImageContainer)
    return nullptr;

  nsRefPtr<ImageContainer> container;
  nsresult rv = mImageContainer->GetImageContainer(aManager, getter_AddRefs(container));
  NS_ENSURE_SUCCESS(rv, nullptr);
  return container.forget();
}

#define MAX_BLUR_RADIUS 300
#define MAX_SPREAD_RADIUS 50

static inline gfxIntSize
ComputeBlurRadius(nscoord aBlurRadius, int32_t aAppUnitsPerDevPixel, gfxFloat aScaleX = 1.0, gfxFloat aScaleY = 1.0)
{
  
  
  gfxFloat blurStdDev = gfxFloat(aBlurRadius) / gfxFloat(aAppUnitsPerDevPixel);

  gfxPoint scaledBlurStdDev = gfxPoint(std::min((blurStdDev * aScaleX),
                                              gfxFloat(MAX_BLUR_RADIUS)) / 2.0,
                                       std::min((blurStdDev * aScaleY),
                                              gfxFloat(MAX_BLUR_RADIUS)) / 2.0);
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
  
  if (transform.HasNonAxisAlignedTransform() || transform.xx <= 0.0 || transform.yy <= 0.0) {
    transform = gfxMatrix();
  } else {
    scaleX = transform.xx;
    scaleY = transform.yy;
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
    mContext = blur.Init(rect, spreadRadius,
                         blurRadius, &dirtyRect, &skipRect);
  } else {
    mContext = blur.Init(rect, spreadRadius,
                         blurRadius, &dirtyRect, NULL);
  }

  if (mContext) {
    
    
    mContext->SetMatrix(transform);
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
    mDestinationCtx->IdentityMatrix();
  }

  blur.Paint(mDestinationCtx);
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
  result.top    = blurRadius.height * aAppUnitsPerDevPixel;
  result.right  = blurRadius.width  * aAppUnitsPerDevPixel;
  result.bottom = blurRadius.height * aAppUnitsPerDevPixel;
  result.left   = blurRadius.width  * aAppUnitsPerDevPixel;
  return result;
}
