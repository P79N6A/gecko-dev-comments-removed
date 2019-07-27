





#include "SVGTextFrame.h"


#include "DOMSVGPoint.h"
#include "gfx2DGlue.h"
#include "gfxFont.h"
#include "gfxSkipChars.h"
#include "gfxTypes.h"
#include "gfxUtils.h"
#include "LookAndFeel.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/gfx/PatternHelpers.h"
#include "mozilla/Likely.h"
#include "nsAlgorithm.h"
#include "nsBlockFrame.h"
#include "nsCaret.h"
#include "nsContentUtils.h"
#include "nsGkAtoms.h"
#include "nsIDOMSVGLength.h"
#include "nsISelection.h"
#include "nsQuickSort.h"
#include "nsRenderingContext.h"
#include "nsSVGEffects.h"
#include "nsSVGOuterSVGFrame.h"
#include "nsSVGPaintServerFrame.h"
#include "mozilla/dom/SVGRect.h"
#include "nsSVGIntegrationUtils.h"
#include "nsSVGUtils.h"
#include "nsTArray.h"
#include "nsTextFrame.h"
#include "nsTextNode.h"
#include "SVGAnimatedNumberList.h"
#include "SVGContentUtils.h"
#include "SVGLengthList.h"
#include "SVGNumberList.h"
#include "SVGPathElement.h"
#include "SVGTextPathElement.h"
#include "nsLayoutUtils.h"
#include "nsFrameSelection.h"
#include "nsStyleStructInlines.h"
#include <algorithm>
#include <cmath>
#include <limits>

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::gfx;














static void
ConvertOriginalToSkipped(gfxSkipCharsIterator& aIterator,
                         uint32_t aOriginalOffset, uint32_t aOriginalLength,
                         uint32_t& aSkippedOffset, uint32_t& aSkippedLength)
{
  aSkippedOffset = aIterator.ConvertOriginalToSkipped(aOriginalOffset);
  aIterator.AdvanceOriginal(aOriginalLength);
  aSkippedLength = aIterator.GetSkippedOffset() - aSkippedOffset;
}









static void
ConvertOriginalToSkipped(gfxSkipCharsIterator& aIterator,
                         uint32_t& aOffset, uint32_t& aLength)
{
  ConvertOriginalToSkipped(aIterator, aOffset, aLength, aOffset, aLength);
}





static gfxPoint
AppUnitsToGfxUnits(const nsPoint& aPoint, const nsPresContext* aContext)
{
  return gfxPoint(aContext->AppUnitsToGfxUnits(aPoint.x),
                  aContext->AppUnitsToGfxUnits(aPoint.y));
}





static gfxRect
AppUnitsToFloatCSSPixels(const gfxRect& aRect, const nsPresContext* aContext)
{
  return gfxRect(aContext->AppUnitsToFloatCSSPixels(aRect.x),
                 aContext->AppUnitsToFloatCSSPixels(aRect.y),
                 aContext->AppUnitsToFloatCSSPixels(aRect.width),
                 aContext->AppUnitsToFloatCSSPixels(aRect.height));
}








static void
ScaleAround(gfxRect& aRect, const gfxPoint& aPoint, double aScale)
{
  aRect.x = aPoint.x - aScale * (aPoint.x - aRect.x);
  aRect.y = aPoint.y - aScale * (aPoint.y - aRect.y);
  aRect.width *= aScale;
  aRect.height *= aScale;
}




static bool
Inside(const gfxRect& aRect, const gfxPoint& aPoint)
{
  return aPoint.x >= aRect.x &&
         aPoint.x < aRect.XMost() &&
         aPoint.y >= aRect.y &&
         aPoint.y < aRect.YMost();
}









static void
GetAscentAndDescentInAppUnits(nsTextFrame* aFrame,
                              gfxFloat& aAscent, gfxFloat& aDescent)
{
  gfxSkipCharsIterator it = aFrame->EnsureTextRun(nsTextFrame::eInflated);
  gfxTextRun* textRun = aFrame->GetTextRun(nsTextFrame::eInflated);

  uint32_t offset, length;
  ConvertOriginalToSkipped(it,
                           aFrame->GetContentOffset(),
                           aFrame->GetContentLength(),
                           offset, length);

  gfxTextRun::Metrics metrics =
    textRun->MeasureText(offset, length, gfxFont::LOOSE_INK_EXTENTS, nullptr,
                         nullptr);

  aAscent = metrics.mAscent;
  aDescent = metrics.mDescent;
}





static void
IntersectInterval(uint32_t& aStart, uint32_t& aLength,
                  uint32_t aStartOther, uint32_t aLengthOther)
{
  uint32_t aEnd = aStart + aLength;
  uint32_t aEndOther = aStartOther + aLengthOther;

  if (aStartOther >= aEnd || aStart >= aEndOther) {
    aLength = 0;
  } else {
    if (aStartOther >= aStart)
      aStart = aStartOther;
    aLength = std::min(aEnd, aEndOther) - aStart;
  }
}






static void
TrimOffsets(uint32_t& aStart, uint32_t& aLength,
            const nsTextFrame::TrimmedOffsets& aTrimmedOffsets)
{
  IntersectInterval(aStart, aLength,
                    aTrimmedOffsets.mStart, aTrimmedOffsets.mLength);
}





static nsIContent*
GetFirstNonAAncestor(nsIContent* aContent)
{
  while (aContent && aContent->IsSVGElement(nsGkAtoms::a)) {
    aContent = aContent->GetParent();
  }
  return aContent;
}






















static bool
IsTextContentElement(nsIContent* aContent)
{
  if (aContent->IsSVGElement(nsGkAtoms::text)) {
    nsIContent* parent = GetFirstNonAAncestor(aContent->GetParent());
    return !parent || !IsTextContentElement(parent);
  }

  if (aContent->IsSVGElement(nsGkAtoms::textPath)) {
    nsIContent* parent = GetFirstNonAAncestor(aContent->GetParent());
    return parent && parent->IsSVGElement(nsGkAtoms::text);
  }

  if (aContent->IsAnyOfSVGElements(nsGkAtoms::a,
                                   nsGkAtoms::tspan,
                                   nsGkAtoms::altGlyph)) {
    return true;
  }

  return false;
}





static bool
IsNonEmptyTextFrame(nsIFrame* aFrame)
{
  nsTextFrame* textFrame = do_QueryFrame(aFrame);
  if (!textFrame) {
    return false;
  }

  return textFrame->GetContentLength() != 0;
}










static bool
GetNonEmptyTextFrameAndNode(nsIFrame* aFrame,
                            nsTextFrame*& aTextFrame,
                            nsTextNode*& aTextNode)
{
  nsTextFrame* text = do_QueryFrame(aFrame);
  bool isNonEmptyTextFrame = text && text->GetContentLength() != 0;

  if (isNonEmptyTextFrame) {
    nsIContent* content = text->GetContent();
    NS_ASSERTION(content && content->IsNodeOfType(nsINode::eTEXT),
                 "unexpected content type for nsTextFrame");

    nsTextNode* node = static_cast<nsTextNode*>(content);
    MOZ_ASSERT(node->TextLength() != 0,
               "frame's GetContentLength() should be 0 if the text node "
               "has no content");

    aTextFrame = text;
    aTextNode = node;
  }

  MOZ_ASSERT(IsNonEmptyTextFrame(aFrame) == isNonEmptyTextFrame,
             "our logic should agree with IsNonEmptyTextFrame");
  return isNonEmptyTextFrame;
}






static bool
IsGlyphPositioningAttribute(nsIAtom* aAttribute)
{
  return aAttribute == nsGkAtoms::x ||
         aAttribute == nsGkAtoms::y ||
         aAttribute == nsGkAtoms::dx ||
         aAttribute == nsGkAtoms::dy ||
         aAttribute == nsGkAtoms::rotate;
}









static nscoord
GetBaselinePosition(nsTextFrame* aFrame,
                    gfxTextRun* aTextRun,
                    uint8_t aDominantBaseline,
                    float aFontSizeScaleFactor)
{
  WritingMode writingMode = aFrame->GetWritingMode();
  gfxTextRun::Metrics metrics =
    aTextRun->MeasureText(0, aTextRun->GetLength(), gfxFont::LOOSE_INK_EXTENTS,
                          nullptr, nullptr);

  switch (aDominantBaseline) {
    case NS_STYLE_DOMINANT_BASELINE_HANGING:
    case NS_STYLE_DOMINANT_BASELINE_TEXT_BEFORE_EDGE:
      return writingMode.IsVerticalRL()
             ? metrics.mAscent + metrics.mDescent : 0;

    case NS_STYLE_DOMINANT_BASELINE_USE_SCRIPT:
    case NS_STYLE_DOMINANT_BASELINE_NO_CHANGE:
    case NS_STYLE_DOMINANT_BASELINE_RESET_SIZE:
      
      
      
      

    case NS_STYLE_DOMINANT_BASELINE_AUTO:
    case NS_STYLE_DOMINANT_BASELINE_ALPHABETIC:
      return writingMode.IsVerticalRL()
             ? metrics.mAscent + metrics.mDescent -
               aFrame->GetLogicalBaseline(writingMode)
             : aFrame->GetLogicalBaseline(writingMode);

    case NS_STYLE_DOMINANT_BASELINE_MIDDLE:
      return aFrame->GetLogicalBaseline(writingMode) -
        SVGContentUtils::GetFontXHeight(aFrame) / 2.0 *
        aFrame->PresContext()->AppUnitsPerCSSPixel() * aFontSizeScaleFactor;

    case NS_STYLE_DOMINANT_BASELINE_TEXT_AFTER_EDGE:
    case NS_STYLE_DOMINANT_BASELINE_IDEOGRAPHIC:
      return writingMode.IsVerticalLR()
             ? 0 : metrics.mAscent + metrics.mDescent;

    case NS_STYLE_DOMINANT_BASELINE_CENTRAL:
    case NS_STYLE_DOMINANT_BASELINE_MATHEMATICAL:
      return (metrics.mAscent + metrics.mDescent) / 2.0;
  }

  NS_NOTREACHED("unexpected dominant-baseline value");
  return aFrame->GetLogicalBaseline(writingMode);
}











static uint32_t
ClusterLength(gfxTextRun* aTextRun, const gfxSkipCharsIterator& aIterator)
{
  uint32_t start = aIterator.GetSkippedOffset();
  uint32_t end = start + 1;
  while (end < aTextRun->GetLength() &&
         (!aTextRun->IsLigatureGroupStart(end) ||
          !aTextRun->IsClusterStart(end))) {
    end++;
  }
  return end - start;
}








template<typename T, typename U>
static void
TruncateTo(nsTArray<T>& aArrayToTruncate, const nsTArray<U>& aReferenceArray)
{
  uint32_t length = aReferenceArray.Length();
  if (aArrayToTruncate.Length() > length) {
    aArrayToTruncate.TruncateLength(length);
  }
}











static SVGTextFrame*
FrameIfAnonymousChildReflowed(SVGTextFrame* aFrame)
{
  NS_PRECONDITION(aFrame, "aFrame must not be null");
  nsIFrame* kid = aFrame->GetFirstPrincipalChild();
  if (NS_SUBTREE_DIRTY(kid)) {
    MOZ_ASSERT(false, "should have already reflowed the anonymous block child");
    return nullptr;
  }
  return aFrame;
}

static double
GetContextScale(const gfxMatrix& aMatrix)
{
  
  
  
  gfxPoint p = aMatrix.Transform(gfxPoint(1, 1)) -
               aMatrix.Transform(gfxPoint(0, 0));
  return SVGContentUtils::ComputeNormalizedHypotenuse(p.x, p.y);
}




namespace mozilla {


















struct TextRenderedRun
{
  



  TextRenderedRun()
    : mFrame(nullptr)
  {
  }

  




  TextRenderedRun(nsTextFrame* aFrame, const gfxPoint& aPosition,
                  float aLengthAdjustScaleFactor, double aRotate,
                  float aFontSizeScaleFactor, nscoord aBaseline,
                  uint32_t aTextFrameContentOffset,
                  uint32_t aTextFrameContentLength,
                  uint32_t aTextElementCharIndex)
    : mFrame(aFrame),
      mPosition(aPosition),
      mLengthAdjustScaleFactor(aLengthAdjustScaleFactor),
      mRotate(static_cast<float>(aRotate)),
      mFontSizeScaleFactor(aFontSizeScaleFactor),
      mBaseline(aBaseline),
      mTextFrameContentOffset(aTextFrameContentOffset),
      mTextFrameContentLength(aTextFrameContentLength),
      mTextElementCharIndex(aTextElementCharIndex)
  {
  }

  


  gfxTextRun* GetTextRun() const
  {
    mFrame->EnsureTextRun(nsTextFrame::eInflated);
    return mFrame->GetTextRun(nsTextFrame::eInflated);
  }

  


  bool IsRightToLeft() const
  {
    return GetTextRun()->IsRightToLeft();
  }

  


  bool IsVertical() const
  {
    return GetTextRun()->IsVertical();
  }

  





















































































  gfxMatrix GetTransformFromUserSpaceForPainting(
                                      nsPresContext* aContext,
                                      const nsCharClipDisplayItem& aItem) const;

  







  gfxMatrix GetTransformFromRunUserSpaceToUserSpace(nsPresContext* aContext) const;

  





  gfxMatrix GetTransformFromRunUserSpaceToFrameUserSpace(nsPresContext* aContext) const;

  



  enum {
    
    eIncludeFill = 1,
    
    eIncludeStroke = 2,
    
    eIncludeTextShadow = 4,
    
    eNoHorizontalOverflow = 8
  };

  








  SVGBBox GetRunUserSpaceRect(nsPresContext* aContext, uint32_t aFlags) const;

  


































  SVGBBox GetFrameUserSpaceRect(nsPresContext* aContext, uint32_t aFlags) const;

  










  SVGBBox GetUserSpaceRect(nsPresContext* aContext, uint32_t aFlags,
                           const gfxMatrix* aAdditionalTransform = nullptr) const;

  








  void GetClipEdges(nscoord& aVisIStartEdge, nscoord& aVisIEndEdge) const;

  


  nscoord GetAdvanceWidth() const;

  







  int32_t GetCharNumAtPosition(nsPresContext* aContext,
                               const gfxPoint& aPoint) const;

  


  nsTextFrame* mFrame;

  









  gfxPoint mPosition;

  



  float mLengthAdjustScaleFactor;

  


  float mRotate;

  



  double mFontSizeScaleFactor;

  



  nscoord mBaseline;

  



  uint32_t mTextFrameContentOffset;
  uint32_t mTextFrameContentLength;

  



  uint32_t mTextElementCharIndex;
};

gfxMatrix
TextRenderedRun::GetTransformFromUserSpaceForPainting(
                                       nsPresContext* aContext,
                                       const nsCharClipDisplayItem& aItem) const
{
  
  

  gfxMatrix m;
  if (!mFrame) {
    return m;
  }

  float cssPxPerDevPx = aContext->
    AppUnitsToFloatCSSPixels(aContext->AppUnitsPerDevPixel());

  
  m.Translate(mPosition / cssPxPerDevPx);

  
  m.Scale(1.0 / mFontSizeScaleFactor, 1.0 / mFontSizeScaleFactor);

  
  m.Rotate(mRotate);

  m.Scale(mLengthAdjustScaleFactor, 1.0);

  
  nsPoint t;
  if (IsVertical()) {
    t = nsPoint(-mBaseline,
                IsRightToLeft()
                  ? -mFrame->GetRect().height + aItem.mVisIEndEdge
                  : -aItem.mVisIStartEdge);
  } else {
    t = nsPoint(IsRightToLeft()
                  ? -mFrame->GetRect().width + aItem.mVisIEndEdge
                  : -aItem.mVisIStartEdge,
                -mBaseline);
  }
  m.Translate(AppUnitsToGfxUnits(t, aContext));

  return m;
}

gfxMatrix
TextRenderedRun::GetTransformFromRunUserSpaceToUserSpace(
                                                  nsPresContext* aContext) const
{
  gfxMatrix m;
  if (!mFrame) {
    return m;
  }

  float cssPxPerDevPx = aContext->
    AppUnitsToFloatCSSPixels(aContext->AppUnitsPerDevPixel());

  nscoord start, end;
  GetClipEdges(start, end);

  
  m.Translate(mPosition);

  
  m.Rotate(mRotate);

  
  m.Scale(mLengthAdjustScaleFactor, 1.0);

  
  nsPoint t;
  if (IsVertical()) {
    t = nsPoint(-mBaseline,
                IsRightToLeft()
                  ? -mFrame->GetRect().height + start + end
                  : 0);
  } else {
    t = nsPoint(IsRightToLeft()
                  ? -mFrame->GetRect().width + start + end
                  : 0,
                -mBaseline);
  }
  m.Translate(AppUnitsToGfxUnits(t, aContext) *
                cssPxPerDevPx / mFontSizeScaleFactor);

  return m;
}

gfxMatrix
TextRenderedRun::GetTransformFromRunUserSpaceToFrameUserSpace(
                                                  nsPresContext* aContext) const
{
  gfxMatrix m;
  if (!mFrame) {
    return m;
  }

  nscoord start, end;
  GetClipEdges(start, end);

  
  
  gfxFloat appPerCssPx = aContext->AppUnitsPerCSSPixel();
  gfxPoint t = IsVertical() ? gfxPoint(0, start / appPerCssPx)
                            : gfxPoint(start / appPerCssPx, 0);
  return m.Translate(t);
}

SVGBBox
TextRenderedRun::GetRunUserSpaceRect(nsPresContext* aContext,
                                     uint32_t aFlags) const
{
  SVGBBox r;
  if (!mFrame) {
    return r;
  }

  
  
  
  
  
  
  nsRect self = mFrame->GetVisualOverflowRectRelativeToSelf();
  nsRect rect = mFrame->GetRect();
  bool vertical = IsVertical();
  nscoord above = vertical ? -self.x : -self.y;
  nscoord below = vertical ? self.XMost() - rect.width
                           : self.YMost() - rect.height;

  gfxSkipCharsIterator it = mFrame->EnsureTextRun(nsTextFrame::eInflated);
  gfxTextRun* textRun = mFrame->GetTextRun(nsTextFrame::eInflated);

  
  uint32_t offset, length;
  ConvertOriginalToSkipped(it, mTextFrameContentOffset, mTextFrameContentLength,
                           offset, length);
  if (length == 0) {
    return r;
  }

  
  gfxTextRun::Metrics metrics =
    textRun->MeasureText(offset, length, gfxFont::LOOSE_INK_EXTENTS,
                         nullptr, nullptr);
  
  gfxRect fontBox(0, -metrics.mAscent,
      metrics.mAdvanceWidth, metrics.mAscent + metrics.mDescent);
  metrics.mBoundingBox.UnionRect(metrics.mBoundingBox, fontBox);

  
  
  
  nscoord baseline = metrics.mBoundingBox.y + metrics.mAscent;
  gfxFloat x, width;
  if (aFlags & eNoHorizontalOverflow) {
    x = 0.0;
    width = textRun->GetAdvanceWidth(offset, length, nullptr);
  } else {
    x = metrics.mBoundingBox.x;
    width = metrics.mBoundingBox.width;
  }
  nsRect fillInAppUnits(x, baseline - above,
                        width, metrics.mBoundingBox.height + above + below);
  if (textRun->IsVertical()) {
    
    Swap(fillInAppUnits.x, fillInAppUnits.y);
    Swap(fillInAppUnits.width, fillInAppUnits.height);
  }

  
  if (aFlags & eIncludeTextShadow) {
    fillInAppUnits =
      nsLayoutUtils::GetTextShadowRectsUnion(fillInAppUnits, mFrame);
  }

  
  gfxRect fill = AppUnitsToFloatCSSPixels(gfxRect(fillInAppUnits.x,
                                                  fillInAppUnits.y,
                                                  fillInAppUnits.width,
                                                  fillInAppUnits.height),
                                          aContext);

  
  
  ScaleAround(fill,
              textRun->IsVertical()
                ? gfxPoint(aContext->AppUnitsToFloatCSSPixels(baseline), 0.0)
                : gfxPoint(0.0, aContext->AppUnitsToFloatCSSPixels(baseline)),
              1.0 / mFontSizeScaleFactor);

  
  if (aFlags & eIncludeFill) {
    r = fill;
  }

  
  if ((aFlags & eIncludeStroke) &&
      !fill.IsEmpty() &&
      nsSVGUtils::GetStrokeWidth(mFrame) > 0) {
    r.UnionEdges(nsSVGUtils::PathExtentsToMaxStrokeExtents(fill, mFrame,
                                                           gfxMatrix()));
  }

  return r;
}

SVGBBox
TextRenderedRun::GetFrameUserSpaceRect(nsPresContext* aContext,
                                       uint32_t aFlags) const
{
  SVGBBox r = GetRunUserSpaceRect(aContext, aFlags);
  if (r.IsEmpty()) {
    return r;
  }
  gfxMatrix m = GetTransformFromRunUserSpaceToFrameUserSpace(aContext);
  return m.TransformBounds(r.ToThebesRect());
}

SVGBBox
TextRenderedRun::GetUserSpaceRect(nsPresContext* aContext,
                                  uint32_t aFlags,
                                  const gfxMatrix* aAdditionalTransform) const
{
  SVGBBox r = GetRunUserSpaceRect(aContext, aFlags);
  if (r.IsEmpty()) {
    return r;
  }
  gfxMatrix m = GetTransformFromRunUserSpaceToUserSpace(aContext);
  if (aAdditionalTransform) {
    m *= *aAdditionalTransform;
  }
  return m.TransformBounds(r.ToThebesRect());
}

void
TextRenderedRun::GetClipEdges(nscoord& aVisIStartEdge,
                              nscoord& aVisIEndEdge) const
{
  uint32_t contentLength = mFrame->GetContentLength();
  if (mTextFrameContentOffset == 0 &&
      mTextFrameContentLength == contentLength) {
    
    
    aVisIStartEdge = 0;
    aVisIEndEdge = 0;
    return;
  }

  gfxSkipCharsIterator it = mFrame->EnsureTextRun(nsTextFrame::eInflated);
  gfxTextRun* textRun = mFrame->GetTextRun(nsTextFrame::eInflated);

  
  
  uint32_t runOffset, runLength, frameOffset, frameLength;
  ConvertOriginalToSkipped(it, mTextFrameContentOffset, mTextFrameContentLength,
                               runOffset, runLength);

  
  frameOffset = mFrame->GetContentOffset();
  frameLength = mFrame->GetContentLength();

  
  
  
  nsTextFrame::TrimmedOffsets trimmedOffsets =
    mFrame->GetTrimmedOffsets(mFrame->GetContent()->GetText(), true);
  TrimOffsets(frameOffset, frameLength, trimmedOffsets);

  
  
  ConvertOriginalToSkipped(it, frameOffset, frameLength);

  
  
  nscoord startEdge =
    textRun->GetAdvanceWidth(frameOffset, runOffset - frameOffset, nullptr);

  
  
  nscoord endEdge =
    textRun->GetAdvanceWidth(runOffset + runLength,
                             frameOffset + frameLength - (runOffset + runLength),
                             nullptr);

  if (textRun->IsRightToLeft()) {
    aVisIStartEdge = endEdge;
    aVisIEndEdge = startEdge;
  } else {
    aVisIStartEdge = startEdge;
    aVisIEndEdge = endEdge;
  }
}

nscoord
TextRenderedRun::GetAdvanceWidth() const
{
  gfxSkipCharsIterator it = mFrame->EnsureTextRun(nsTextFrame::eInflated);
  gfxTextRun* textRun = mFrame->GetTextRun(nsTextFrame::eInflated);

  uint32_t offset, length;
  ConvertOriginalToSkipped(it, mTextFrameContentOffset, mTextFrameContentLength,
                           offset, length);

  return textRun->GetAdvanceWidth(offset, length, nullptr);
}

int32_t
TextRenderedRun::GetCharNumAtPosition(nsPresContext* aContext,
                                      const gfxPoint& aPoint) const
{
  if (mTextFrameContentLength == 0) {
    return -1;
  }

  float cssPxPerDevPx = aContext->
    AppUnitsToFloatCSSPixels(aContext->AppUnitsPerDevPixel());

  
  
  gfxMatrix m = GetTransformFromRunUserSpaceToUserSpace(aContext);
  if (!m.Invert()) {
    return -1;
  }
  gfxPoint p = m.Transform(aPoint) / cssPxPerDevPx * mFontSizeScaleFactor;

  
  
  gfxFloat ascent, descent;
  GetAscentAndDescentInAppUnits(mFrame, ascent, descent);

  WritingMode writingMode = mFrame->GetWritingMode();
  if (writingMode.IsVertical()) {
    gfxFloat leftEdge =
      mFrame->GetLogicalBaseline(writingMode) -
        (writingMode.IsVerticalRL() ? ascent : descent);
    gfxFloat rightEdge = leftEdge + ascent + descent;
    if (p.x < aContext->AppUnitsToGfxUnits(leftEdge) ||
        p.x > aContext->AppUnitsToGfxUnits(rightEdge)) {
      return -1;
    }
  } else {
    gfxFloat topEdge = mFrame->GetLogicalBaseline(writingMode) - ascent;
    gfxFloat bottomEdge = topEdge + ascent + descent;
    if (p.y < aContext->AppUnitsToGfxUnits(topEdge) ||
        p.y > aContext->AppUnitsToGfxUnits(bottomEdge)) {
      return -1;
    }
  }

  gfxSkipCharsIterator it = mFrame->EnsureTextRun(nsTextFrame::eInflated);
  gfxTextRun* textRun = mFrame->GetTextRun(nsTextFrame::eInflated);

  
  
  uint32_t offset, length;
  ConvertOriginalToSkipped(it, mTextFrameContentOffset, mTextFrameContentLength,
                           offset, length);
  gfxFloat runAdvance =
    aContext->AppUnitsToGfxUnits(textRun->GetAdvanceWidth(offset, length,
                                                          nullptr));

  gfxFloat pos = writingMode.IsVertical() ? p.y : p.x;
  if (pos < 0 || pos >= runAdvance) {
    return -1;
  }

  
  
  
  bool rtl = textRun->IsRightToLeft();
  for (int32_t i = mTextFrameContentLength - 1; i >= 0; i--) {
    ConvertOriginalToSkipped(it, mTextFrameContentOffset, i, offset, length);
    gfxFloat advance =
      aContext->AppUnitsToGfxUnits(textRun->GetAdvanceWidth(offset, length,
                                                            nullptr));
    if ((rtl && pos < runAdvance - advance) ||
        (!rtl && pos >= advance)) {
      return i;
    }
  }
  return -1;
}




enum SubtreePosition
{
  eBeforeSubtree,
  eWithinSubtree,
  eAfterSubtree
};







class TextNodeIterator
{
public:
  



  explicit TextNodeIterator(nsIContent* aRoot, nsIContent* aSubtree = nullptr)
    : mRoot(aRoot),
      mSubtree(aSubtree == aRoot ? nullptr : aSubtree),
      mCurrent(aRoot),
      mSubtreePosition(mSubtree ? eBeforeSubtree : eWithinSubtree)
  {
    NS_ASSERTION(aRoot, "expected non-null root");
    if (!aRoot->IsNodeOfType(nsINode::eTEXT)) {
      Next();
    }
  }

  


  nsTextNode* Current() const
  {
    return static_cast<nsTextNode*>(mCurrent);
  }

  



  nsTextNode* Next();

  




  bool IsWithinSubtree() const
  {
    return mSubtreePosition == eWithinSubtree;
  }

  



  bool IsAfterSubtree() const
  {
    return mSubtreePosition == eAfterSubtree;
  }

private:
  


  nsIContent* mRoot;

  


  nsIContent* mSubtree;

  


  nsIContent* mCurrent;

  


  SubtreePosition mSubtreePosition;
};

nsTextNode*
TextNodeIterator::Next()
{
  
  
  
  if (mCurrent) {
    do {
      nsIContent* next = IsTextContentElement(mCurrent) ?
                           mCurrent->GetFirstChild() :
                           nullptr;
      if (next) {
        mCurrent = next;
        if (mCurrent == mSubtree) {
          mSubtreePosition = eWithinSubtree;
        }
      } else {
        for (;;) {
          if (mCurrent == mRoot) {
            mCurrent = nullptr;
            break;
          }
          if (mCurrent == mSubtree) {
            mSubtreePosition = eAfterSubtree;
          }
          next = mCurrent->GetNextSibling();
          if (next) {
            mCurrent = next;
            if (mCurrent == mSubtree) {
              mSubtreePosition = eWithinSubtree;
            }
            break;
          }
          if (mCurrent == mSubtree) {
            mSubtreePosition = eAfterSubtree;
          }
          mCurrent = mCurrent->GetParent();
        }
      }
    } while (mCurrent && !mCurrent->IsNodeOfType(nsINode::eTEXT));
  }

  return static_cast<nsTextNode*>(mCurrent);
}















struct TextNodeCorrespondence
{
  explicit TextNodeCorrespondence(uint32_t aUndisplayedCharacters)
    : mUndisplayedCharacters(aUndisplayedCharacters)
  {
  }

  uint32_t mUndisplayedCharacters;
};

NS_DECLARE_FRAME_PROPERTY(TextNodeCorrespondenceProperty,
                          DeleteValue<TextNodeCorrespondence>)





static uint32_t
GetUndisplayedCharactersBeforeFrame(nsTextFrame* aFrame)
{
  void* value = aFrame->Properties().Get(TextNodeCorrespondenceProperty());
  TextNodeCorrespondence* correspondence =
    static_cast<TextNodeCorrespondence*>(value);
  if (!correspondence) {
    NS_NOTREACHED("expected a TextNodeCorrespondenceProperty on nsTextFrame "
                  "used for SVG text");
    return 0;
  }
  return correspondence->mUndisplayedCharacters;
}








class TextNodeCorrespondenceRecorder
{
public:
  


  static void RecordCorrespondence(SVGTextFrame* aRoot);

private:
  explicit TextNodeCorrespondenceRecorder(SVGTextFrame* aRoot)
    : mNodeIterator(aRoot->GetContent()),
      mPreviousNode(nullptr),
      mNodeCharIndex(0)
  {
  }

  void Record(SVGTextFrame* aRoot);
  void TraverseAndRecord(nsIFrame* aFrame);

  


  nsTextNode* NextNode();

  



  TextNodeIterator mNodeIterator;

  


  nsTextNode* mPreviousNode;

  


  uint32_t mNodeCharIndex;
};

 void
TextNodeCorrespondenceRecorder::RecordCorrespondence(SVGTextFrame* aRoot)
{
  TextNodeCorrespondenceRecorder recorder(aRoot);
  recorder.Record(aRoot);
}

void
TextNodeCorrespondenceRecorder::Record(SVGTextFrame* aRoot)
{
  if (!mNodeIterator.Current()) {
    
    return;
  }

  
  
  TraverseAndRecord(aRoot);

  
  uint32_t undisplayed = 0;
  if (mNodeIterator.Current()) {
    if (mPreviousNode && mPreviousNode->TextLength() != mNodeCharIndex) {
      
      
      NS_ASSERTION(mNodeCharIndex < mPreviousNode->TextLength(),
                   "incorrect tracking of undisplayed characters in "
                   "text nodes");
      undisplayed += mPreviousNode->TextLength() - mNodeCharIndex;
    }
    
    for (nsTextNode* textNode = mNodeIterator.Current();
         textNode;
         textNode = NextNode()) {
      undisplayed += textNode->TextLength();
    }
  }

  
  
  aRoot->mTrailingUndisplayedCharacters = undisplayed;
}

nsTextNode*
TextNodeCorrespondenceRecorder::NextNode()
{
  mPreviousNode = mNodeIterator.Current();
  nsTextNode* next;
  do {
    next = mNodeIterator.Next();
  } while (next && next->TextLength() == 0);
  return next;
}

void
TextNodeCorrespondenceRecorder::TraverseAndRecord(nsIFrame* aFrame)
{
  
  
  if (IsTextContentElement(aFrame->GetContent())) {
    for (nsIFrame* f = aFrame->GetFirstPrincipalChild();
         f;
         f = f->GetNextSibling()) {
      TraverseAndRecord(f);
    }
    return;
  }

  nsTextFrame* frame;  
  nsTextNode* node;    
  if (!GetNonEmptyTextFrameAndNode(aFrame, frame, node)) {
    
    return;
  }

  NS_ASSERTION(frame->GetContentOffset() >= 0,
               "don't know how to handle negative content indexes");

  uint32_t undisplayed = 0;
  if (!mPreviousNode) {
    
    NS_ASSERTION(mNodeCharIndex == 0, "incorrect tracking of undisplayed "
                                      "characters in text nodes");
    if (!mNodeIterator.Current()) {
      NS_NOTREACHED("incorrect tracking of correspondence between text frames "
                    "and text nodes");
    } else {
      
      
      while (mNodeIterator.Current() != node) {
        undisplayed += mNodeIterator.Current()->TextLength();
        NextNode();
      }
      
      
      undisplayed += frame->GetContentOffset();
      NextNode();
    }
  } else if (mPreviousNode == node) {
    
    if (static_cast<uint32_t>(frame->GetContentOffset()) != mNodeCharIndex) {
      
      
      NS_ASSERTION(mNodeCharIndex <
                     static_cast<uint32_t>(frame->GetContentOffset()),
                   "incorrect tracking of undisplayed characters in "
                   "text nodes");
      undisplayed = frame->GetContentOffset() - mNodeCharIndex;
    }
  } else {
    
    if (mPreviousNode->TextLength() != mNodeCharIndex) {
      NS_ASSERTION(mNodeCharIndex < mPreviousNode->TextLength(),
                   "incorrect tracking of undisplayed characters in "
                   "text nodes");
      
      
      undisplayed = mPreviousNode->TextLength() - mNodeCharIndex;
    }
    
    
    while (mNodeIterator.Current() != node) {
      undisplayed += mNodeIterator.Current()->TextLength();
      NextNode();
    }
    
    
    undisplayed += frame->GetContentOffset();
    NextNode();
  }

  
  frame->Properties().Set(TextNodeCorrespondenceProperty(),
                          new TextNodeCorrespondence(undisplayed));

  
  mNodeCharIndex = frame->GetContentEnd();
}
























class TextFrameIterator
{
public:
  



  explicit TextFrameIterator(SVGTextFrame* aRoot, nsIFrame* aSubtree = nullptr)
    : mRootFrame(aRoot),
      mSubtree(aSubtree),
      mCurrentFrame(aRoot),
      mCurrentPosition(),
      mSubtreePosition(mSubtree ? eBeforeSubtree : eWithinSubtree)
  {
    Init();
  }

  



  TextFrameIterator(SVGTextFrame* aRoot, nsIContent* aSubtree)
    : mRootFrame(aRoot),
      mSubtree(aRoot && aSubtree && aSubtree != aRoot->GetContent() ?
                 aSubtree->GetPrimaryFrame() :
                 nullptr),
      mCurrentFrame(aRoot),
      mCurrentPosition(),
      mSubtreePosition(mSubtree ? eBeforeSubtree : eWithinSubtree)
  {
    Init();
  }

  


  SVGTextFrame* Root() const
  {
    return mRootFrame;
  }

  


  nsTextFrame* Current() const
  {
    return do_QueryFrame(mCurrentFrame);
  }

  



  uint32_t UndisplayedCharacters() const;

  



  nsPoint Position() const
  {
    return mCurrentPosition;
  }

  


  nsTextFrame* Next();

  


  bool IsWithinSubtree() const
  {
    return mSubtreePosition == eWithinSubtree;
  }

  


  bool IsAfterSubtree() const
  {
    return mSubtreePosition == eAfterSubtree;
  }

  



  nsIFrame* TextPathFrame() const
  {
    return mTextPathFrames.IsEmpty() ?
             nullptr :
             mTextPathFrames.ElementAt(mTextPathFrames.Length() - 1);
  }

  


  uint8_t DominantBaseline() const
  {
    return mBaselines.ElementAt(mBaselines.Length() - 1);
  }

  


  void Close()
  {
    mCurrentFrame = nullptr;
  }

private:
  


  void Init()
  {
    if (!mRootFrame) {
      return;
    }

    mBaselines.AppendElement(mRootFrame->StyleSVGReset()->mDominantBaseline);
    Next();
  }

  




  void PushBaseline(nsIFrame* aNextFrame);

  


  void PopBaseline();

  


  SVGTextFrame* mRootFrame;

  


  nsIFrame* mSubtree;

  


  nsIFrame* mCurrentFrame;

  


  nsPoint mCurrentPosition;

  



  nsAutoTArray<nsIFrame*, 1> mTextPathFrames;

  



  nsAutoTArray<uint8_t, 8> mBaselines;

  


  SubtreePosition mSubtreePosition;
};

uint32_t
TextFrameIterator::UndisplayedCharacters() const
{
  MOZ_ASSERT(!(mRootFrame->GetFirstPrincipalChild() &&
               NS_SUBTREE_DIRTY(mRootFrame->GetFirstPrincipalChild())),
             "should have already reflowed the anonymous block child");

  if (!mCurrentFrame) {
    return mRootFrame->mTrailingUndisplayedCharacters;
  }

  nsTextFrame* frame = do_QueryFrame(mCurrentFrame);
  return GetUndisplayedCharactersBeforeFrame(frame);
}

nsTextFrame*
TextFrameIterator::Next()
{
  
  
  
  if (mCurrentFrame) {
    do {
      nsIFrame* next = IsTextContentElement(mCurrentFrame->GetContent()) ?
                         mCurrentFrame->GetFirstPrincipalChild() :
                         nullptr;
      if (next) {
        
        mCurrentPosition += next->GetPosition();
        if (next->GetContent()->IsSVGElement(nsGkAtoms::textPath)) {
          
          mTextPathFrames.AppendElement(next);
        }
        
        PushBaseline(next);
        mCurrentFrame = next;
        if (mCurrentFrame == mSubtree) {
          
          mSubtreePosition = eWithinSubtree;
        }
      } else {
        for (;;) {
          
          if (mCurrentFrame == mRootFrame) {
            
            mCurrentFrame = nullptr;
            break;
          }
          
          mCurrentPosition -= mCurrentFrame->GetPosition();
          if (mCurrentFrame->GetContent()->IsSVGElement(nsGkAtoms::textPath)) {
            
            mTextPathFrames.TruncateLength(mTextPathFrames.Length() - 1);
          }
          
          PopBaseline();
          if (mCurrentFrame == mSubtree) {
            
            mSubtreePosition = eAfterSubtree;
          }
          next = mCurrentFrame->GetNextSibling();
          if (next) {
            
            mCurrentPosition += next->GetPosition();
            if (next->GetContent()->IsSVGElement(nsGkAtoms::textPath)) {
              
              mTextPathFrames.AppendElement(next);
            }
            
            PushBaseline(next);
            mCurrentFrame = next;
            if (mCurrentFrame == mSubtree) {
              
              mSubtreePosition = eWithinSubtree;
            }
            break;
          }
          if (mCurrentFrame == mSubtree) {
            
            
            mSubtreePosition = eAfterSubtree;
          }
          
          mCurrentFrame = mCurrentFrame->GetParent();
        }
      }
    } while (mCurrentFrame &&
             !IsNonEmptyTextFrame(mCurrentFrame));
  }

  return Current();
}

void
TextFrameIterator::PushBaseline(nsIFrame* aNextFrame)
{
  uint8_t baseline = aNextFrame->StyleSVGReset()->mDominantBaseline;
  if (baseline == NS_STYLE_DOMINANT_BASELINE_AUTO) {
    baseline = mBaselines.LastElement();
  }
  mBaselines.AppendElement(baseline);
}

void
TextFrameIterator::PopBaseline()
{
  NS_ASSERTION(!mBaselines.IsEmpty(), "popped too many baselines");
  mBaselines.TruncateLength(mBaselines.Length() - 1);
}







class TextRenderedRunIterator
{
public:
  



  enum RenderedRunFilter {
    
    eAllFrames,
    
    
    eVisibleFrames
  };

  










  explicit TextRenderedRunIterator(SVGTextFrame* aSVGTextFrame,
                                   RenderedRunFilter aFilter = eAllFrames,
                                   nsIFrame* aSubtree = nullptr)
    : mFrameIterator(FrameIfAnonymousChildReflowed(aSVGTextFrame), aSubtree),
      mFilter(aFilter),
      mTextElementCharIndex(0),
      mFrameStartTextElementCharIndex(0),
      mFontSizeScaleFactor(aSVGTextFrame->mFontSizeScaleFactor),
      mCurrent(First())
  {
  }

  









  TextRenderedRunIterator(SVGTextFrame* aSVGTextFrame,
                          RenderedRunFilter aFilter,
                          nsIContent* aSubtree)
    : mFrameIterator(FrameIfAnonymousChildReflowed(aSVGTextFrame), aSubtree),
      mFilter(aFilter),
      mTextElementCharIndex(0),
      mFrameStartTextElementCharIndex(0),
      mFontSizeScaleFactor(aSVGTextFrame->mFontSizeScaleFactor),
      mCurrent(First())
  {
  }

  


  TextRenderedRun Current() const
  {
    return mCurrent;
  }

  


  TextRenderedRun Next();

private:
  


  SVGTextFrame* Root() const
  {
    return mFrameIterator.Root();
  }

  


  TextRenderedRun First();

  


  TextFrameIterator mFrameIterator;

  


  RenderedRunFilter mFilter;

  



  uint32_t mTextElementCharIndex;

  



  uint32_t mFrameStartTextElementCharIndex;

  


  double mFontSizeScaleFactor;

  


  TextRenderedRun mCurrent;
};

TextRenderedRun
TextRenderedRunIterator::Next()
{
  if (!mFrameIterator.Current()) {
    
    
    mCurrent = TextRenderedRun();
    return mCurrent;
  }

  
  nsTextFrame* frame;
  gfxPoint pt;
  double rotate;
  nscoord baseline;
  uint32_t offset, length;
  uint32_t charIndex;

  
  
  
  
  for (;;) {
    if (mFrameIterator.IsAfterSubtree()) {
      mCurrent = TextRenderedRun();
      return mCurrent;
    }

    frame = mFrameIterator.Current();

    charIndex = mTextElementCharIndex;

    
    
    
    uint32_t runStart, runEnd;  
    runStart = mTextElementCharIndex;
    runEnd = runStart + 1;
    while (runEnd < Root()->mPositions.Length() &&
           !Root()->mPositions[runEnd].mRunBoundary) {
      runEnd++;
    }

    
    
    offset = frame->GetContentOffset() + runStart -
             mFrameStartTextElementCharIndex;
    length = runEnd - runStart;

    
    
    uint32_t contentEnd = frame->GetContentEnd();
    if (offset + length > contentEnd) {
      length = contentEnd - offset;
    }

    NS_ASSERTION(offset >= uint32_t(frame->GetContentOffset()), "invalid offset");
    NS_ASSERTION(offset + length <= contentEnd, "invalid offset or length");

    
    frame->EnsureTextRun(nsTextFrame::eInflated);
    baseline = GetBaselinePosition(frame,
                                   frame->GetTextRun(nsTextFrame::eInflated),
                                   mFrameIterator.DominantBaseline(),
                                   mFontSizeScaleFactor);

    
    uint32_t untrimmedOffset = offset;
    uint32_t untrimmedLength = length;
    nsTextFrame::TrimmedOffsets trimmedOffsets =
      frame->GetTrimmedOffsets(frame->GetContent()->GetText(), true);
    TrimOffsets(offset, length, trimmedOffsets);
    charIndex += offset - untrimmedOffset;

    
    
    pt = Root()->mPositions[charIndex].mPosition;
    rotate = Root()->mPositions[charIndex].mAngle;

    
    bool skip = !mFrameIterator.IsWithinSubtree() ||
                Root()->mPositions[mTextElementCharIndex].mHidden;
    if (mFilter == eVisibleFrames) {
      skip = skip || !frame->StyleVisibility()->IsVisible();
    }

    
    
    mTextElementCharIndex += untrimmedLength;

    
    
    if (offset + untrimmedLength >= contentEnd) {
      mFrameIterator.Next();
      mTextElementCharIndex += mFrameIterator.UndisplayedCharacters();
      mFrameStartTextElementCharIndex = mTextElementCharIndex;
    }

    if (!mFrameIterator.Current()) {
      if (skip) {
        
        
        mCurrent = TextRenderedRun();
        return mCurrent;
      }
      break;
    }

    if (length && !skip) {
      
      
      break;
    }
  }

  mCurrent = TextRenderedRun(frame, pt, Root()->mLengthAdjustScaleFactor,
                             rotate, mFontSizeScaleFactor, baseline,
                             offset, length, charIndex);
  return mCurrent;
}

TextRenderedRun
TextRenderedRunIterator::First()
{
  if (!mFrameIterator.Current()) {
    return TextRenderedRun();
  }

  if (Root()->mPositions.IsEmpty()) {
    mFrameIterator.Close();
    return TextRenderedRun();
  }

  
  
  mTextElementCharIndex = mFrameIterator.UndisplayedCharacters();
  mFrameStartTextElementCharIndex = mTextElementCharIndex;

  return Next();
}







class CharIterator
{
public:
  



  enum CharacterFilter {
    
    
    eOriginal,
    
    
    
    eAddressable,
    
    
    eClusterAndLigatureGroupStart,
    
    
    eClusterOrLigatureGroupMiddle
  };

  








  CharIterator(SVGTextFrame* aSVGTextFrame,
               CharacterFilter aFilter,
               nsIContent* aSubtree = nullptr);

  


  bool AtEnd() const
  {
    return !mFrameIterator.Current();
  }

  



  bool Next();

  



  bool Next(uint32_t aCount);

  


  void NextWithinSubtree(uint32_t aCount);

  




  bool AdvanceToCharacter(uint32_t aTextElementCharIndex);

  


  bool AdvancePastCurrentFrame();

  



  bool AdvancePastCurrentTextPathFrame();

  




  bool AdvanceToSubtree();

  


  nsTextFrame* TextFrame() const
  {
    return mFrameIterator.Current();
  }

  


  bool IsWithinSubtree() const
  {
    return mFrameIterator.IsWithinSubtree();
  }

  


  bool IsAfterSubtree() const
  {
    return mFrameIterator.IsAfterSubtree();
  }

  


  bool IsOriginalCharSkipped() const
  {
    return mSkipCharsIterator.IsOriginalCharSkipped();
  }

  



  bool IsClusterAndLigatureGroupStart() const;

  



  bool IsOriginalCharTrimmed() const;

  



  bool IsOriginalCharUnaddressable() const
  {
    return IsOriginalCharSkipped() || IsOriginalCharTrimmed();
  }

  


  gfxTextRun* TextRun() const
  {
    return mTextRun;
  }

  


  uint32_t TextElementCharIndex() const
  {
    return mTextElementCharIndex;
  }

  



  uint32_t GlyphStartTextElementCharIndex() const
  {
    return mGlyphStartTextElementCharIndex;
  }

  



  uint32_t GlyphUndisplayedCharacters() const
  {
    return mGlyphUndisplayedCharacters;
  }

  







  void GetOriginalGlyphOffsets(uint32_t& aOriginalOffset,
                               uint32_t& aOriginalLength) const;

  





  gfxFloat GetGlyphAdvance(nsPresContext* aContext) const;

  






  gfxFloat GetAdvance(nsPresContext* aContext) const;

  




















  gfxFloat GetGlyphPartialAdvance(uint32_t aPartLength,
                                  nsPresContext* aContext) const;

  



  nsIFrame* TextPathFrame() const
  {
    return mFrameIterator.TextPathFrame();
  }

private:
  




  bool NextCharacter();

  


  bool MatchesFilter() const;

  


  void UpdateGlyphStartTextElementCharIndex() {
    if (!IsOriginalCharSkipped() && IsClusterAndLigatureGroupStart()) {
      mGlyphStartTextElementCharIndex = mTextElementCharIndex;
      mGlyphUndisplayedCharacters = 0;
    }
  }

  


  CharacterFilter mFilter;

  


  TextFrameIterator mFrameIterator;

  



  gfxSkipCharsIterator mSkipCharsIterator;

  
  mutable nsTextFrame* mFrameForTrimCheck;
  mutable uint32_t mTrimmedOffset;
  mutable uint32_t mTrimmedLength;

  


  gfxTextRun* mTextRun;

  


  uint32_t mTextElementCharIndex;

  



  uint32_t mGlyphStartTextElementCharIndex;

  





  uint32_t mGlyphUndisplayedCharacters;

  



  float mLengthAdjustScaleFactor;
};

CharIterator::CharIterator(SVGTextFrame* aSVGTextFrame,
                           CharIterator::CharacterFilter aFilter,
                           nsIContent* aSubtree)
  : mFilter(aFilter),
    mFrameIterator(FrameIfAnonymousChildReflowed(aSVGTextFrame), aSubtree),
    mFrameForTrimCheck(nullptr),
    mTrimmedOffset(0),
    mTrimmedLength(0),
    mTextElementCharIndex(0),
    mGlyphStartTextElementCharIndex(0),
    mLengthAdjustScaleFactor(aSVGTextFrame->mLengthAdjustScaleFactor)
{
  if (!AtEnd()) {
    mSkipCharsIterator = TextFrame()->EnsureTextRun(nsTextFrame::eInflated);
    mTextRun = TextFrame()->GetTextRun(nsTextFrame::eInflated);
    mTextElementCharIndex = mFrameIterator.UndisplayedCharacters();
    UpdateGlyphStartTextElementCharIndex();
    if (!MatchesFilter()) {
      Next();
    }
  }
}

bool
CharIterator::Next()
{
  while (NextCharacter()) {
    if (MatchesFilter()) {
      return true;
    }
  }
  return false;
}

bool
CharIterator::Next(uint32_t aCount)
{
  if (aCount == 0 && AtEnd()) {
    return false;
  }
  while (aCount) {
    if (!Next()) {
      return false;
    }
    aCount--;
  }
  return true;
}

void
CharIterator::NextWithinSubtree(uint32_t aCount)
{
  while (IsWithinSubtree() && aCount) {
    --aCount;
    if (!Next()) {
      return;
    }
  }
}

bool
CharIterator::AdvanceToCharacter(uint32_t aTextElementCharIndex)
{
  while (mTextElementCharIndex < aTextElementCharIndex) {
    if (!Next()) {
      return false;
    }
  }
  return true;
}

bool
CharIterator::AdvancePastCurrentFrame()
{
  
  nsTextFrame* currentFrame = TextFrame();
  do {
    if (!Next()) {
      return false;
    }
  } while (TextFrame() == currentFrame);
  return true;
}

bool
CharIterator::AdvancePastCurrentTextPathFrame()
{
  nsIFrame* currentTextPathFrame = TextPathFrame();
  NS_ASSERTION(currentTextPathFrame,
               "expected AdvancePastCurrentTextPathFrame to be called only "
               "within a text path frame");
  do {
    if (!AdvancePastCurrentFrame()) {
      return false;
    }
  } while (TextPathFrame() == currentTextPathFrame);
  return true;
}

bool
CharIterator::AdvanceToSubtree()
{
  while (!IsWithinSubtree()) {
    if (IsAfterSubtree()) {
      return false;
    }
    if (!AdvancePastCurrentFrame()) {
      return false;
    }
  }
  return true;
}

bool
CharIterator::IsClusterAndLigatureGroupStart() const
{
  return mTextRun->IsLigatureGroupStart(mSkipCharsIterator.GetSkippedOffset()) &&
         mTextRun->IsClusterStart(mSkipCharsIterator.GetSkippedOffset());
}

bool
CharIterator::IsOriginalCharTrimmed() const
{
  if (mFrameForTrimCheck != TextFrame()) {
    
    
    mFrameForTrimCheck = TextFrame();
    uint32_t offset = mFrameForTrimCheck->GetContentOffset();
    uint32_t length = mFrameForTrimCheck->GetContentLength();
    nsIContent* content = mFrameForTrimCheck->GetContent();
    nsTextFrame::TrimmedOffsets trim =
      mFrameForTrimCheck->GetTrimmedOffsets(content->GetText(), true);
    TrimOffsets(offset, length, trim);
    mTrimmedOffset = offset;
    mTrimmedLength = length;
  }

  
  
  uint32_t index = mSkipCharsIterator.GetOriginalOffset();
  return !((index >= mTrimmedOffset &&
            index < mTrimmedOffset + mTrimmedLength) ||
           (index >= mTrimmedOffset + mTrimmedLength &&
            mFrameForTrimCheck->StyleText()->
              NewlineIsSignificant(mFrameForTrimCheck) &&
            mFrameForTrimCheck->GetContent()->GetText()->CharAt(index) == '\n'));
}

void
CharIterator::GetOriginalGlyphOffsets(uint32_t& aOriginalOffset,
                                      uint32_t& aOriginalLength) const
{
  gfxSkipCharsIterator it = TextFrame()->EnsureTextRun(nsTextFrame::eInflated);
  it.SetOriginalOffset(mSkipCharsIterator.GetOriginalOffset() -
                         (mTextElementCharIndex -
                          mGlyphStartTextElementCharIndex -
                          mGlyphUndisplayedCharacters));

  while (it.GetSkippedOffset() > 0 &&
         (!mTextRun->IsClusterStart(it.GetSkippedOffset()) ||
          !mTextRun->IsLigatureGroupStart(it.GetSkippedOffset()))) {
    it.AdvanceSkipped(-1);
  }

  aOriginalOffset = it.GetOriginalOffset();

  
  it.SetOriginalOffset(mSkipCharsIterator.GetOriginalOffset());
  do {
    it.AdvanceSkipped(1);
  } while (it.GetSkippedOffset() < mTextRun->GetLength() &&
           (!mTextRun->IsClusterStart(it.GetSkippedOffset()) ||
            !mTextRun->IsLigatureGroupStart(it.GetSkippedOffset())));

  aOriginalLength = it.GetOriginalOffset() - aOriginalOffset;
}

gfxFloat
CharIterator::GetGlyphAdvance(nsPresContext* aContext) const
{
  uint32_t offset, length;
  GetOriginalGlyphOffsets(offset, length);

  gfxSkipCharsIterator it = TextFrame()->EnsureTextRun(nsTextFrame::eInflated);
  ConvertOriginalToSkipped(it, offset, length);

  float cssPxPerDevPx = aContext->
    AppUnitsToFloatCSSPixels(aContext->AppUnitsPerDevPixel());

  gfxFloat advance = mTextRun->GetAdvanceWidth(offset, length, nullptr);
  return aContext->AppUnitsToGfxUnits(advance) *
         mLengthAdjustScaleFactor * cssPxPerDevPx;
}

gfxFloat
CharIterator::GetAdvance(nsPresContext* aContext) const
{
  float cssPxPerDevPx = aContext->
    AppUnitsToFloatCSSPixels(aContext->AppUnitsPerDevPixel());

  gfxFloat advance =
    mTextRun->GetAdvanceWidth(mSkipCharsIterator.GetSkippedOffset(), 1, nullptr);
  return aContext->AppUnitsToGfxUnits(advance) *
         mLengthAdjustScaleFactor * cssPxPerDevPx;
}

gfxFloat
CharIterator::GetGlyphPartialAdvance(uint32_t aPartLength,
                                     nsPresContext* aContext) const
{
  uint32_t offset, length;
  GetOriginalGlyphOffsets(offset, length);

  NS_ASSERTION(aPartLength <= length, "invalid aPartLength value");
  length = aPartLength;

  gfxSkipCharsIterator it = TextFrame()->EnsureTextRun(nsTextFrame::eInflated);
  ConvertOriginalToSkipped(it, offset, length);

  float cssPxPerDevPx = aContext->
    AppUnitsToFloatCSSPixels(aContext->AppUnitsPerDevPixel());

  gfxFloat advance = mTextRun->GetAdvanceWidth(offset, length, nullptr);
  return aContext->AppUnitsToGfxUnits(advance) *
         mLengthAdjustScaleFactor * cssPxPerDevPx;
}

bool
CharIterator::NextCharacter()
{
  if (AtEnd()) {
    return false;
  }

  mTextElementCharIndex++;

  
  mSkipCharsIterator.AdvanceOriginal(1);
  if (mSkipCharsIterator.GetOriginalOffset() < TextFrame()->GetContentEnd()) {
    
    UpdateGlyphStartTextElementCharIndex();
    return true;
  }

  
  mFrameIterator.Next();

  
  uint32_t undisplayed = mFrameIterator.UndisplayedCharacters();
  mGlyphUndisplayedCharacters += undisplayed;
  mTextElementCharIndex += undisplayed;
  if (!TextFrame()) {
    
    mSkipCharsIterator = gfxSkipCharsIterator();
    return false;
  }

  mSkipCharsIterator = TextFrame()->EnsureTextRun(nsTextFrame::eInflated);
  mTextRun = TextFrame()->GetTextRun(nsTextFrame::eInflated);
  UpdateGlyphStartTextElementCharIndex();
  return true;
}

bool
CharIterator::MatchesFilter() const
{
  if (mFilter == eOriginal) {
    return true;
  }

  if (IsOriginalCharSkipped()) {
    return false;
  }

  if (mFilter == eAddressable) {
    return !IsOriginalCharUnaddressable();
  }

  return (mFilter == eClusterAndLigatureGroupStart) ==
         IsClusterAndLigatureGroupStart();
}








class SVGCharClipDisplayItem : public nsCharClipDisplayItem {
public:
  explicit SVGCharClipDisplayItem(const TextRenderedRun& aRun)
    : nsCharClipDisplayItem(aRun.mFrame)
  {
    aRun.GetClipEdges(mVisIStartEdge, mVisIEndEdge);
  }

  NS_DISPLAY_DECL_NAME("SVGText", TYPE_TEXT)
};













class SVGTextDrawPathCallbacks : public nsTextFrame::DrawPathCallbacks
{
public:
  









  SVGTextDrawPathCallbacks(nsRenderingContext* aContext,
                           nsTextFrame* aFrame,
                           const gfxMatrix& aCanvasTM,
                           bool aShouldPaintSVGGlyphs)
    : DrawPathCallbacks(aShouldPaintSVGGlyphs),
      gfx(aContext->ThebesContext()),
      mFrame(aFrame),
      mCanvasTM(aCanvasTM)
  {
  }

  void NotifySelectionBackgroundNeedsFill(const Rect& aBackgroundRect,
                                          nscolor aColor,
                                          DrawTarget& aDrawTarget) override;
  void PaintDecorationLine(Rect aPath, nscolor aColor) override;
  void PaintSelectionDecorationLine(Rect aPath, nscolor aColor) override;
  void NotifyBeforeText(nscolor aColor) override;
  void NotifyGlyphPathEmitted() override;
  void NotifyAfterText() override;

private:
  void SetupContext();

  bool IsClipPathChild() const {
    return nsLayoutUtils::GetClosestFrameOfType
             (mFrame->GetParent(), nsGkAtoms::svgTextFrame)->GetStateBits() &
             NS_STATE_SVG_CLIPPATH_CHILD;
  }

  



  void HandleTextGeometry();

  



  void MakeFillPattern(GeneralPattern* aOutPattern);

  



  void FillAndStrokeGeometry();

  


  void FillGeometry();

  


  void StrokeGeometry();

  gfxContext* gfx;
  nsTextFrame* mFrame;
  const gfxMatrix& mCanvasTM;

  





  nscolor mColor;
};

void
SVGTextDrawPathCallbacks::NotifySelectionBackgroundNeedsFill(
                                                      const Rect& aBackgroundRect,
                                                      nscolor aColor,
                                                      DrawTarget& aDrawTarget)
{
  if (IsClipPathChild()) {
    
    return;
  }

  mColor = aColor; 

  GeneralPattern fillPattern;
  MakeFillPattern(&fillPattern);
  if (fillPattern.GetPattern()) {
    DrawOptions drawOptions(aColor == NS_40PERCENT_FOREGROUND_COLOR ? 0.4 : 1.0);
    aDrawTarget.FillRect(aBackgroundRect, fillPattern, drawOptions);
  }
}

void
SVGTextDrawPathCallbacks::NotifyBeforeText(nscolor aColor)
{
  mColor = aColor;
  SetupContext();
  gfx->NewPath();
}

void
SVGTextDrawPathCallbacks::NotifyGlyphPathEmitted()
{
  HandleTextGeometry();
  gfx->NewPath();
}

void
SVGTextDrawPathCallbacks::NotifyAfterText()
{
  gfx->Restore();
}

void
SVGTextDrawPathCallbacks::PaintDecorationLine(Rect aPath, nscolor aColor)
{
  mColor = aColor;
  AntialiasMode aaMode =
    nsSVGUtils::ToAntialiasMode(mFrame->StyleSVG()->mTextRendering);

  gfx->Save();
  gfx->NewPath();
  gfx->SetAntialiasMode(aaMode);
  gfx->Rectangle(ThebesRect(aPath));
  HandleTextGeometry();
  gfx->NewPath();
  gfx->Restore();
}

void
SVGTextDrawPathCallbacks::PaintSelectionDecorationLine(Rect aPath,
                                                       nscolor aColor)
{
  if (IsClipPathChild()) {
    
    return;
  }

  mColor = aColor;

  gfx->Save();
  gfx->NewPath();
  gfx->Rectangle(ThebesRect(aPath));
  FillAndStrokeGeometry();
  gfx->Restore();
}

void
SVGTextDrawPathCallbacks::SetupContext()
{
  gfx->Save();

  
  
  
  switch (mFrame->StyleSVG()->mTextRendering) {
  case NS_STYLE_TEXT_RENDERING_OPTIMIZESPEED:
    gfx->SetAntialiasMode(AntialiasMode::NONE);
    break;
  default:
    gfx->SetAntialiasMode(AntialiasMode::SUBPIXEL);
    break;
  }
}

void
SVGTextDrawPathCallbacks::HandleTextGeometry()
{
  if (IsClipPathChild()) {
    RefPtr<Path> path = gfx->GetPath();
    ColorPattern white(Color(1.f, 1.f, 1.f, 1.f)); 
    gfx->GetDrawTarget()->Fill(path, white);
  } else {
    
    gfxContextMatrixAutoSaveRestore saveMatrix(gfx);
    gfx->SetMatrix(mCanvasTM);

    FillAndStrokeGeometry();
  }
}

void
SVGTextDrawPathCallbacks::MakeFillPattern(GeneralPattern* aOutPattern)
{
  if (mColor == NS_SAME_AS_FOREGROUND_COLOR ||
      mColor == NS_40PERCENT_FOREGROUND_COLOR) {
    nsSVGUtils::MakeFillPatternFor(mFrame, gfx, aOutPattern);
    return;
  }

  if (mColor == NS_TRANSPARENT) {
    return;
  }

  aOutPattern->InitColorPattern(ToDeviceColor(mColor));
}

void
SVGTextDrawPathCallbacks::FillAndStrokeGeometry()
{
  bool pushedGroup = false;
  if (mColor == NS_40PERCENT_FOREGROUND_COLOR) {
    pushedGroup = true;
    gfx->PushGroup(gfxContentType::COLOR_ALPHA);
  }

  uint32_t paintOrder = mFrame->StyleSVG()->mPaintOrder;
  if (paintOrder == NS_STYLE_PAINT_ORDER_NORMAL) {
    FillGeometry();
    StrokeGeometry();
  } else {
    while (paintOrder) {
      uint32_t component =
        paintOrder & ((1 << NS_STYLE_PAINT_ORDER_BITWIDTH) - 1);
      switch (component) {
        case NS_STYLE_PAINT_ORDER_FILL:
          FillGeometry();
          break;
        case NS_STYLE_PAINT_ORDER_STROKE:
          StrokeGeometry();
          break;
      }
      paintOrder >>= NS_STYLE_PAINT_ORDER_BITWIDTH;
    }
  }

  if (pushedGroup) {
    gfx->PopGroupToSource();
    gfx->Paint(0.4);
  }
}

void
SVGTextDrawPathCallbacks::FillGeometry()
{
  GeneralPattern fillPattern;
  MakeFillPattern(&fillPattern);
  if (fillPattern.GetPattern()) {
    RefPtr<Path> path = gfx->GetPath();
    FillRule fillRule = nsSVGUtils::ToFillRule(IsClipPathChild() ?
                          mFrame->StyleSVG()->mClipRule :
                          mFrame->StyleSVG()->mFillRule);
    if (fillRule != path->GetFillRule()) {
      RefPtr<PathBuilder> builder = path->CopyToBuilder(fillRule);
      path = builder->Finish();
    }
    gfx->GetDrawTarget()->Fill(path, fillPattern);
  }
}

void
SVGTextDrawPathCallbacks::StrokeGeometry()
{
  
  if (mColor == NS_SAME_AS_FOREGROUND_COLOR ||
      mColor == NS_40PERCENT_FOREGROUND_COLOR) {
    if (nsSVGUtils::HasStroke(mFrame,  nullptr)) {
      GeneralPattern strokePattern;
      nsSVGUtils::MakeStrokePatternFor(mFrame, gfx, &strokePattern,  nullptr);
      if (strokePattern.GetPattern()) {
        if (!mFrame->GetParent()->GetContent()->IsSVGElement()) {
          
          MOZ_ASSERT(false, "Our nsTextFrame's parent's content should be SVG");
          return;
        }
        nsSVGElement* svgOwner =
          static_cast<nsSVGElement*>(mFrame->GetParent()->GetContent());

        
        gfxMatrix outerSVGToUser;
        if (nsSVGUtils::GetNonScalingStrokeTransform(mFrame, &outerSVGToUser) &&
            outerSVGToUser.Invert()) {
          gfx->Multiply(outerSVGToUser);
        }

        RefPtr<Path> path = gfx->GetPath();
        SVGContentUtils::AutoStrokeOptions strokeOptions;
        SVGContentUtils::GetStrokeOptions(&strokeOptions, svgOwner,
                                          mFrame->StyleContext(),
                                           nullptr);
        DrawOptions drawOptions;
        drawOptions.mAntialiasMode =
          nsSVGUtils::ToAntialiasMode(mFrame->StyleSVG()->mTextRendering);
        gfx->GetDrawTarget()->Stroke(path, strokePattern, strokeOptions);
      }
    }
  }
}




already_AddRefed<gfxPattern>
SVGTextContextPaint::GetFillPattern(const DrawTarget* aDrawTarget,
                                    float aOpacity,
                                    const gfxMatrix& aCTM)
{
  return mFillPaint.GetPattern(aDrawTarget, aOpacity, &nsStyleSVG::mFill, aCTM);
}

already_AddRefed<gfxPattern>
SVGTextContextPaint::GetStrokePattern(const DrawTarget* aDrawTarget,
                                      float aOpacity,
                                      const gfxMatrix& aCTM)
{
  return mStrokePaint.GetPattern(aDrawTarget, aOpacity, &nsStyleSVG::mStroke, aCTM);
}

already_AddRefed<gfxPattern>
SVGTextContextPaint::Paint::GetPattern(const DrawTarget* aDrawTarget,
                                       float aOpacity,
                                       nsStyleSVGPaint nsStyleSVG::*aFillOrStroke,
                                       const gfxMatrix& aCTM)
{
  nsRefPtr<gfxPattern> pattern;
  if (mPatternCache.Get(aOpacity, getter_AddRefs(pattern))) {
    
    
    
    pattern->SetMatrix(aCTM * mPatternMatrix);
    return pattern.forget();
  }

  switch (mPaintType) {
  case eStyleSVGPaintType_None:
    pattern = new gfxPattern(gfxRGBA(0.0f, 0.0f, 0.0f, 0.0f));
    mPatternMatrix = gfxMatrix();
    break;
  case eStyleSVGPaintType_Color:
    pattern = new gfxPattern(gfxRGBA(NS_GET_R(mPaintDefinition.mColor) / 255.0,
                                     NS_GET_G(mPaintDefinition.mColor) / 255.0,
                                     NS_GET_B(mPaintDefinition.mColor) / 255.0,
                                     NS_GET_A(mPaintDefinition.mColor) / 255.0 * aOpacity));
    mPatternMatrix = gfxMatrix();
    break;
  case eStyleSVGPaintType_Server:
    pattern = mPaintDefinition.mPaintServerFrame->GetPaintServerPattern(mFrame,
                                                                        aDrawTarget,
                                                                        mContextMatrix,
                                                                        aFillOrStroke,
                                                                        aOpacity);
    {
      
      gfxMatrix m = pattern->GetMatrix();
      gfxMatrix deviceToOriginalUserSpace = mContextMatrix;
      if (!deviceToOriginalUserSpace.Invert()) {
        return nullptr;
      }
      
      mPatternMatrix = deviceToOriginalUserSpace * m;
    }
    pattern->SetMatrix(aCTM * mPatternMatrix);
    break;
  case eStyleSVGPaintType_ContextFill:
    pattern = mPaintDefinition.mContextPaint->GetFillPattern(aDrawTarget,
                                                             aOpacity, aCTM);
    
    
    return pattern.forget();
  case eStyleSVGPaintType_ContextStroke:
    pattern = mPaintDefinition.mContextPaint->GetStrokePattern(aDrawTarget,
                                                               aOpacity, aCTM);
    
    
    return pattern.forget();
  default:
    MOZ_ASSERT(false, "invalid paint type");
    return nullptr;
  }

  mPatternCache.Put(aOpacity, pattern);
  return pattern.forget();
}

} 








class nsDisplaySVGText : public nsDisplayItem {
public:
  nsDisplaySVGText(nsDisplayListBuilder* aBuilder,
                   SVGTextFrame* aFrame)
    : nsDisplayItem(aBuilder, aFrame),
      mDisableSubpixelAA(false)
  {
    MOZ_COUNT_CTOR(nsDisplaySVGText);
    MOZ_ASSERT(aFrame, "Must have a frame!");
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplaySVGText() {
    MOZ_COUNT_DTOR(nsDisplaySVGText);
  }
#endif

  NS_DISPLAY_DECL_NAME("nsDisplaySVGText", TYPE_SVG_TEXT)

  virtual void DisableComponentAlpha() override {
    mDisableSubpixelAA = true;
  }
  virtual void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                       HitTestState* aState,
                       nsTArray<nsIFrame*> *aOutFrames) override;
  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx) override;
  virtual nsRect GetComponentAlphaBounds(nsDisplayListBuilder* aBuilder) override {
    bool snap;
    return GetBounds(aBuilder, &snap);
  }
private:
  bool mDisableSubpixelAA;
};

void
nsDisplaySVGText::HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                          HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames)
{
  SVGTextFrame *frame = static_cast<SVGTextFrame*>(mFrame);
  nsPoint pointRelativeToReferenceFrame = aRect.Center();
  
  nsPoint userSpacePtInAppUnits = pointRelativeToReferenceFrame -
                                    (ToReferenceFrame() - frame->GetPosition());

  gfxPoint userSpacePt =
    gfxPoint(userSpacePtInAppUnits.x, userSpacePtInAppUnits.y) /
      frame->PresContext()->AppUnitsPerCSSPixel();

  nsIFrame* target = frame->GetFrameForPoint(userSpacePt);
  if (target) {
    aOutFrames->AppendElement(target);
  }
}

void
nsDisplaySVGText::Paint(nsDisplayListBuilder* aBuilder,
                        nsRenderingContext* aCtx)
{
  gfxContext* ctx = aCtx->ThebesContext();

  gfxContextAutoDisableSubpixelAntialiasing
    disable(ctx, mDisableSubpixelAA);

  uint32_t appUnitsPerDevPixel = mFrame->PresContext()->AppUnitsPerDevPixel();

  
  
  
  nsPoint offset = ToReferenceFrame() - mFrame->GetPosition();

  gfxPoint devPixelOffset =
    nsLayoutUtils::PointToGfxPoint(offset, appUnitsPerDevPixel);

  gfxMatrix tm = nsSVGIntegrationUtils::GetCSSPxToDevPxMatrix(mFrame) *
                   gfxMatrix::Translation(devPixelOffset);

  ctx->Save();
  static_cast<SVGTextFrame*>(mFrame)->PaintSVG(*ctx, tm);
  ctx->Restore();
}




NS_QUERYFRAME_HEAD(SVGTextFrame)
  NS_QUERYFRAME_ENTRY(SVGTextFrame)
NS_QUERYFRAME_TAIL_INHERITING(SVGTextFrameBase)




nsIFrame*
NS_NewSVGTextFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) SVGTextFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(SVGTextFrame)




void
SVGTextFrame::Init(nsIContent*       aContent,
                   nsContainerFrame* aParent,
                   nsIFrame*         aPrevInFlow)
{
  NS_ASSERTION(aContent->IsSVGElement(nsGkAtoms::text), "Content is not an SVG text");

  SVGTextFrameBase::Init(aContent, aParent, aPrevInFlow);
  AddStateBits((aParent->GetStateBits() & NS_STATE_SVG_CLIPPATH_CHILD) |
               NS_FRAME_SVG_LAYOUT | NS_FRAME_IS_SVG_TEXT);

  mMutationObserver = new MutationObserver(this);
}

void
SVGTextFrame::BuildDisplayList(nsDisplayListBuilder* aBuilder,
                               const nsRect& aDirtyRect,
                               const nsDisplayListSet& aLists)
{
  if (NS_SUBTREE_DIRTY(this)) {
    
    
    
    return;
  }
  aLists.Content()->AppendNewToTop(
    new (aBuilder) nsDisplaySVGText(aBuilder, this));
}

nsresult
SVGTextFrame::AttributeChanged(int32_t aNameSpaceID,
                               nsIAtom* aAttribute,
                               int32_t aModType)
{
  if (aNameSpaceID != kNameSpaceID_None)
    return NS_OK;

  if (aAttribute == nsGkAtoms::transform) {
    
    
    
    

    if (!(mState & NS_FRAME_FIRST_REFLOW) &&
        mCanvasTM && mCanvasTM->IsSingular()) {
      
      NotifyGlyphMetricsChange();
    }
    mCanvasTM = nullptr;
  } else if (IsGlyphPositioningAttribute(aAttribute) ||
             aAttribute == nsGkAtoms::textLength ||
             aAttribute == nsGkAtoms::lengthAdjust) {
    NotifyGlyphMetricsChange();
  }

  return NS_OK;
}

nsIAtom *
SVGTextFrame::GetType() const
{
  return nsGkAtoms::svgTextFrame;
}

void
SVGTextFrame::DidSetStyleContext(nsStyleContext* aOldStyleContext)
{
  if (mState & NS_FRAME_IS_NONDISPLAY) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    ScheduleReflowSVGNonDisplayText();
  }
}

void
SVGTextFrame::ReflowSVGNonDisplayText()
{
  MOZ_ASSERT(nsSVGUtils::AnyOuterSVGIsCallingReflowSVG(this),
             "only call ReflowSVGNonDisplayText when an outer SVG frame is "
             "under ReflowSVG");
  MOZ_ASSERT(mState & NS_FRAME_IS_NONDISPLAY,
             "only call ReflowSVGNonDisplayText if the frame is "
             "NS_FRAME_IS_NONDISPLAY");

  
  
  AddStateBits(NS_FRAME_IS_DIRTY);

  
  
  
  
  nsLayoutUtils::PostRestyleEvent(
    mContent->AsElement(), nsRestyleHint(0),
    nsChangeHint_InvalidateRenderingObservers);

  
  
  
  MaybeReflowAnonymousBlockChild();
  UpdateGlyphPositioning();
}

void
SVGTextFrame::ScheduleReflowSVGNonDisplayText()
{
  MOZ_ASSERT(!nsSVGUtils::OuterSVGIsCallingReflowSVG(this),
             "do not call ScheduleReflowSVGNonDisplayText when the outer SVG "
             "frame is under ReflowSVG");
  MOZ_ASSERT(!(mState & NS_STATE_SVG_TEXT_IN_REFLOW),
             "do not call ScheduleReflowSVGNonDisplayText while reflowing the "
             "anonymous block child");

  
  
  
  
  
  
  
  
  

  nsIFrame* f = this;
  while (f) {
    if (!(f->GetStateBits() & NS_FRAME_IS_NONDISPLAY)) {
      if (NS_SUBTREE_DIRTY(f)) {
        
        
        return;
      }
      if (!f->IsFrameOfType(eSVG) ||
          (f->GetStateBits() & NS_STATE_IS_OUTER_SVG)) {
        break;
      }
      f->AddStateBits(NS_FRAME_HAS_DIRTY_CHILDREN);
    }
    f = f->GetParent();
  }

  MOZ_ASSERT(f, "should have found an ancestor frame to reflow");

  PresContext()->PresShell()->FrameNeedsReflow(
    f, nsIPresShell::eStyleChange, NS_FRAME_IS_DIRTY);
}

NS_IMPL_ISUPPORTS(SVGTextFrame::MutationObserver, nsIMutationObserver)

void
SVGTextFrame::MutationObserver::ContentAppended(nsIDocument* aDocument,
                                                nsIContent* aContainer,
                                                nsIContent* aFirstNewContent,
                                                int32_t aNewIndexInContainer)
{
  mFrame->NotifyGlyphMetricsChange();
}

void
SVGTextFrame::MutationObserver::ContentInserted(
                                        nsIDocument* aDocument,
                                        nsIContent* aContainer,
                                        nsIContent* aChild,
                                        int32_t aIndexInContainer)
{
  mFrame->NotifyGlyphMetricsChange();
}

void
SVGTextFrame::MutationObserver::ContentRemoved(
                                       nsIDocument *aDocument,
                                       nsIContent* aContainer,
                                       nsIContent* aChild,
                                       int32_t aIndexInContainer,
                                       nsIContent* aPreviousSibling)
{
  mFrame->NotifyGlyphMetricsChange();
}

void
SVGTextFrame::MutationObserver::CharacterDataChanged(
                                                 nsIDocument* aDocument,
                                                 nsIContent* aContent,
                                                 CharacterDataChangeInfo* aInfo)
{
  mFrame->NotifyGlyphMetricsChange();
}

void
SVGTextFrame::MutationObserver::AttributeChanged(
                                                nsIDocument* aDocument,
                                                mozilla::dom::Element* aElement,
                                                int32_t aNameSpaceID,
                                                nsIAtom* aAttribute,
                                                int32_t aModType)
{
  if (!aElement->IsSVGElement()) {
    return;
  }

  
  
  if (aElement == mFrame->GetContent()) {
    return;
  }

  mFrame->HandleAttributeChangeInDescendant(aElement, aNameSpaceID, aAttribute);
}

void
SVGTextFrame::HandleAttributeChangeInDescendant(Element* aElement,
                                                int32_t aNameSpaceID,
                                                nsIAtom* aAttribute)
{
  if (aElement->IsSVGElement(nsGkAtoms::textPath)) {
    if (aNameSpaceID == kNameSpaceID_None &&
        aAttribute == nsGkAtoms::startOffset) {
      NotifyGlyphMetricsChange();
    } else if (aNameSpaceID == kNameSpaceID_XLink &&
               aAttribute == nsGkAtoms::href) {
      
      nsIFrame* childElementFrame = aElement->GetPrimaryFrame();
      if (childElementFrame) {
        childElementFrame->Properties().Delete(nsSVGEffects::HrefProperty());
        NotifyGlyphMetricsChange();
      }
    }
  } else {
    if (aNameSpaceID == kNameSpaceID_None &&
        IsGlyphPositioningAttribute(aAttribute)) {
      NotifyGlyphMetricsChange();
    }
  }
}

void
SVGTextFrame::FindCloserFrameForSelection(
                                 nsPoint aPoint,
                                 nsIFrame::FrameWithDistance* aCurrentBestFrame)
{
  if (GetStateBits() & NS_FRAME_IS_NONDISPLAY) {
    return;
  }

  UpdateGlyphPositioning();

  nsPresContext* presContext = PresContext();

  
  TextRenderedRunIterator it(this);
  for (TextRenderedRun run = it.Current(); run.mFrame; run = it.Next()) {
    uint32_t flags = TextRenderedRun::eIncludeFill |
                     TextRenderedRun::eIncludeStroke |
                     TextRenderedRun::eNoHorizontalOverflow;
    SVGBBox userRect = run.GetUserSpaceRect(presContext, flags);
    float devPxPerCSSPx = presContext->CSSPixelsToDevPixels(1.f);
    userRect.Scale(devPxPerCSSPx);

    if (!userRect.IsEmpty()) {
      gfxMatrix m;
      if (!NS_SVGDisplayListHitTestingEnabled()) {
        m = GetCanvasTM();
      }
      nsRect rect = nsSVGUtils::ToCanvasBounds(userRect.ToThebesRect(), m,
                                               presContext);

      if (nsLayoutUtils::PointIsCloserToRect(aPoint, rect,
                                             aCurrentBestFrame->mXDistance,
                                             aCurrentBestFrame->mYDistance)) {
        aCurrentBestFrame->mFrame = run.mFrame;
      }
    }
  }
}




void
SVGTextFrame::NotifySVGChanged(uint32_t aFlags)
{
  MOZ_ASSERT(aFlags & (TRANSFORM_CHANGED | COORD_CONTEXT_CHANGED),
             "Invalidation logic may need adjusting");

  bool needNewBounds = false;
  bool needGlyphMetricsUpdate = false;
  bool needNewCanvasTM = false;

  if ((aFlags & COORD_CONTEXT_CHANGED) &&
      (mState & NS_STATE_SVG_POSITIONING_MAY_USE_PERCENTAGES)) {
    needGlyphMetricsUpdate = true;
  }

  if (aFlags & TRANSFORM_CHANGED) {
    needNewCanvasTM = true;
    if (mCanvasTM && mCanvasTM->IsSingular()) {
      
      needNewBounds = true;
      needGlyphMetricsUpdate = true;
    }
    if (StyleSVGReset()->HasNonScalingStroke()) {
      
      
      needNewBounds = true;
    }
  }

  
  
  
  
  if (needNewCanvasTM && mLastContextScale != 0.0f) {
    mCanvasTM = nullptr;
    
    
    gfxMatrix newTM =
      (mState & NS_FRAME_IS_NONDISPLAY) ? gfxMatrix() :
                                          GetCanvasTM();
    
    float scale = GetContextScale(newTM);
    float change = scale / mLastContextScale;
    if (change >= 2.0f || change <= 0.5f) {
      needNewBounds = true;
      needGlyphMetricsUpdate = true;
    }
  }

  if (needNewBounds) {
    
    
    
    
    
    ScheduleReflowSVG();
  }

  if (needGlyphMetricsUpdate) {
    
    
    
    
    if (!(mState & NS_FRAME_FIRST_REFLOW)) {
      NotifyGlyphMetricsChange();
    }
  }
}




static int32_t
GetCaretOffset(nsCaret* aCaret)
{
  nsCOMPtr<nsISelection> selection = aCaret->GetSelection();
  if (!selection) {
    return -1;
  }

  int32_t offset = -1;
  selection->GetAnchorOffset(&offset);
  return offset;
}








static bool
ShouldPaintCaret(const TextRenderedRun& aThisRun, nsCaret* aCaret)
{
  int32_t caretOffset = GetCaretOffset(aCaret);

  if (caretOffset < 0) {
    return false;
  }

  if (uint32_t(caretOffset) >= aThisRun.mTextFrameContentOffset &&
      uint32_t(caretOffset) < aThisRun.mTextFrameContentOffset +
                                aThisRun.mTextFrameContentLength) {
    return true;
  }

  return false;
}

nsresult
SVGTextFrame::PaintSVG(gfxContext& aContext,
                       const gfxMatrix& aTransform,
                       const nsIntRect *aDirtyRect)
{
  DrawTarget& aDrawTarget = *aContext.GetDrawTarget();

  nsIFrame* kid = GetFirstPrincipalChild();
  if (!kid)
    return NS_OK;

  nsPresContext* presContext = PresContext();

  gfxMatrix initialMatrix = aContext.CurrentMatrix();

  if (mState & NS_FRAME_IS_NONDISPLAY) {
    
    
    
    
    if (presContext->PresShell()->InDrawWindowNotFlushing() &&
        NS_SUBTREE_DIRTY(this)) {
      return NS_OK;
    }
    
    
    UpdateGlyphPositioning();
  } else if (NS_SUBTREE_DIRTY(this)) {
    
    
    
    return NS_OK;
  }

  if (aTransform.IsSingular()) {
    NS_WARNING("Can't render text element!");
    return NS_ERROR_FAILURE;
  }

  gfxMatrix matrixForPaintServers = aTransform * initialMatrix;

  
  if (aDirtyRect) {
    NS_ASSERTION(!NS_SVGDisplayListPaintingEnabled() ||
                 (mState & NS_FRAME_IS_NONDISPLAY),
                 "Display lists handle dirty rect intersection test");
    nsRect dirtyRect(aDirtyRect->x, aDirtyRect->y,
                     aDirtyRect->width, aDirtyRect->height);

    gfxFloat appUnitsPerDevPixel = presContext->AppUnitsPerDevPixel();
    gfxRect frameRect(mRect.x / appUnitsPerDevPixel,
                      mRect.y / appUnitsPerDevPixel,
                      mRect.width / appUnitsPerDevPixel,
                      mRect.height / appUnitsPerDevPixel);

    nsRect canvasRect = nsLayoutUtils::RoundGfxRectToAppRect(
        GetCanvasTM().TransformBounds(frameRect), 1);
    if (!canvasRect.Intersects(dirtyRect)) {
      return NS_OK;
    }
  }

  
  
  
  float cssPxPerDevPx = presContext->
    AppUnitsToFloatCSSPixels(presContext->AppUnitsPerDevPixel());
  gfxMatrix canvasTMForChildren = aTransform;
  canvasTMForChildren.Scale(cssPxPerDevPx, cssPxPerDevPx);
  initialMatrix.Scale(1 / cssPxPerDevPx, 1 / cssPxPerDevPx);

  gfxContextAutoSaveRestore save(&aContext);
  aContext.NewPath();
  aContext.Multiply(canvasTMForChildren);
  gfxMatrix currentMatrix = aContext.CurrentMatrix();

  nsRefPtr<nsCaret> caret = presContext->PresShell()->GetCaret();
  nsRect caretRect;
  nsIFrame* caretFrame = caret->GetPaintGeometry(&caretRect);

  TextRenderedRunIterator it(this, TextRenderedRunIterator::eVisibleFrames);
  TextRenderedRun run = it.Current();

  gfxTextContextPaint *outerContextPaint =
    (gfxTextContextPaint*)aDrawTarget.GetUserData(&gfxTextContextPaint::sUserDataKey);

  nsRenderingContext rendCtx(&aContext);

  while (run.mFrame) {
    nsTextFrame* frame = run.mFrame;

    
    
    SVGCharClipDisplayItem item(run);

    
    
    aContext.SetMatrix(initialMatrix);

    SVGTextContextPaint contextPaint;
    DrawMode drawMode =
      SetupContextPaint(&aDrawTarget, aContext.CurrentMatrix(),
                        frame, outerContextPaint, &contextPaint);

    if (int(drawMode) & int(DrawMode::GLYPH_STROKE)) {
      
      
      nsSVGUtils::SetupCairoStrokeGeometry(frame, &aContext, outerContextPaint);
    }

    
    
    gfxMatrix runTransform =
      run.GetTransformFromUserSpaceForPainting(presContext, item) *
      currentMatrix;
    aContext.SetMatrix(runTransform);

    if (drawMode != DrawMode(0)) {
      nsRect frameRect = frame->GetVisualOverflowRect();
      bool paintSVGGlyphs;
      if (ShouldRenderAsPath(frame, paintSVGGlyphs)) {
        SVGTextDrawPathCallbacks callbacks(&rendCtx, frame,
                                           matrixForPaintServers,
                                           paintSVGGlyphs);
        frame->PaintText(&rendCtx, nsPoint(), frameRect, item,
                         &contextPaint, &callbacks);
      } else {
        frame->PaintText(&rendCtx, nsPoint(), frameRect, item,
                         &contextPaint, nullptr);
      }
    }

    if (frame == caretFrame && ShouldPaintCaret(run, caret)) {
      
      
      caret->PaintCaret(nullptr, aDrawTarget, frame, nsPoint());
      aContext.NewPath();
    }

    run = it.Next();
  }

  return NS_OK;
}

nsIFrame*
SVGTextFrame::GetFrameForPoint(const gfxPoint& aPoint)
{
  NS_ASSERTION(GetFirstPrincipalChild(), "must have a child frame");

  if (mState & NS_FRAME_IS_NONDISPLAY) {
    
    
    
    
    UpdateGlyphPositioning();
  } else {
    NS_ASSERTION(!NS_SUBTREE_DIRTY(this), "reflow should have happened");
  }

  
  
  
  if (!nsSVGUtils::HitTestClip(this, aPoint)) {
    return nullptr;
  }

  nsPresContext* presContext = PresContext();

  
  
  
  

  TextRenderedRunIterator it(this);
  nsIFrame* hit = nullptr;
  for (TextRenderedRun run = it.Current(); run.mFrame; run = it.Next()) {
    uint16_t hitTestFlags = nsSVGUtils::GetGeometryHitTestFlags(run.mFrame);
    if (!(hitTestFlags & (SVG_HIT_TEST_FILL | SVG_HIT_TEST_STROKE))) {
      continue;
    }

    gfxMatrix m = run.GetTransformFromRunUserSpaceToUserSpace(presContext);
    if (!m.Invert()) {
      return nullptr;
    }

    gfxPoint pointInRunUserSpace = m.Transform(aPoint);
    gfxRect frameRect =
      run.GetRunUserSpaceRect(presContext, TextRenderedRun::eIncludeFill |
                                           TextRenderedRun::eIncludeStroke).ToThebesRect();

    if (Inside(frameRect, pointInRunUserSpace)) {
      hit = run.mFrame;
    }
  }
  return hit;
}

nsRect
SVGTextFrame::GetCoveredRegion()
{
  return nsSVGUtils::TransformFrameRectToOuterSVG(
           mRect, GetCanvasTM(), PresContext());
}

void
SVGTextFrame::ReflowSVG()
{
  NS_ASSERTION(nsSVGUtils::OuterSVGIsCallingReflowSVG(this),
               "This call is probaby a wasteful mistake");

  MOZ_ASSERT(!(GetStateBits() & NS_FRAME_IS_NONDISPLAY),
             "ReflowSVG mechanism not designed for this");

  if (!nsSVGUtils::NeedsReflowSVG(this)) {
    NS_ASSERTION(!(mState & NS_STATE_SVG_POSITIONING_DIRTY), "How did this happen?");
    return;
  }

  MaybeReflowAnonymousBlockChild();
  UpdateGlyphPositioning();

  nsPresContext* presContext = PresContext();

  SVGBBox r;
  TextRenderedRunIterator it(this, TextRenderedRunIterator::eAllFrames);
  for (TextRenderedRun run = it.Current(); run.mFrame; run = it.Next()) {
    uint32_t runFlags = 0;
    if (run.mFrame->StyleSVG()->mFill.mType != eStyleSVGPaintType_None) {
      runFlags |= TextRenderedRun::eIncludeFill |
                  TextRenderedRun::eIncludeTextShadow;
    }
    if (nsSVGUtils::HasStroke(run.mFrame)) {
      runFlags |= TextRenderedRun::eIncludeFill |
                  TextRenderedRun::eIncludeTextShadow;
    }
    
    
    
    
    
    
    uint16_t hitTestFlags = nsSVGUtils::GetGeometryHitTestFlags(run.mFrame);
    if (hitTestFlags & SVG_HIT_TEST_FILL) {
      runFlags |= TextRenderedRun::eIncludeFill;
    }
    if (hitTestFlags & SVG_HIT_TEST_STROKE) {
      runFlags |= TextRenderedRun::eIncludeStroke;
    }

    if (runFlags) {
      r.UnionEdges(run.GetUserSpaceRect(presContext, runFlags));
    }
  }

  if (r.IsEmpty()) {
    mRect.SetEmpty();
  } else {
    mRect =
      nsLayoutUtils::RoundGfxRectToAppRect(r.ToThebesRect(), presContext->AppUnitsPerCSSPixel());

    
    
    
    mRect.Inflate(presContext->AppUnitsPerDevPixel());
  }

  if (mState & NS_FRAME_FIRST_REFLOW) {
    
    
    
    nsSVGEffects::UpdateEffects(this);
  }

  nsRect overflow = nsRect(nsPoint(0,0), mRect.Size());
  nsOverflowAreas overflowAreas(overflow, overflow);
  FinishAndStoreOverflow(overflowAreas, mRect.Size());

  
  mState &= ~(NS_FRAME_FIRST_REFLOW | NS_FRAME_IS_DIRTY |
              NS_FRAME_HAS_DIRTY_CHILDREN);

  
  
  
  SVGTextFrameBase::ReflowSVG();
}





static uint32_t
TextRenderedRunFlagsForBBoxContribution(const TextRenderedRun& aRun,
                                        uint32_t aBBoxFlags)
{
  uint32_t flags = 0;
  if ((aBBoxFlags & nsSVGUtils::eBBoxIncludeFillGeometry) ||
      ((aBBoxFlags & nsSVGUtils::eBBoxIncludeFill) &&
       aRun.mFrame->StyleSVG()->mFill.mType != eStyleSVGPaintType_None)) {
    flags |= TextRenderedRun::eIncludeFill;
  }
  if ((aBBoxFlags & nsSVGUtils::eBBoxIncludeStrokeGeometry) ||
      ((aBBoxFlags & nsSVGUtils::eBBoxIncludeStroke) &&
       nsSVGUtils::HasStroke(aRun.mFrame))) {
    flags |= TextRenderedRun::eIncludeStroke;
  }
  return flags;
}

SVGBBox
SVGTextFrame::GetBBoxContribution(const gfx::Matrix &aToBBoxUserspace,
                                  uint32_t aFlags)
{
  NS_ASSERTION(GetFirstPrincipalChild(), "must have a child frame");

  UpdateGlyphPositioning();

  SVGBBox bbox;
  nsPresContext* presContext = PresContext();

  TextRenderedRunIterator it(this);
  for (TextRenderedRun run = it.Current(); run.mFrame; run = it.Next()) {
    uint32_t flags = TextRenderedRunFlagsForBBoxContribution(run, aFlags);
    gfxMatrix m = ThebesMatrix(aToBBoxUserspace);
    SVGBBox bboxForRun =
      run.GetUserSpaceRect(presContext, flags, &m);
    bbox.UnionEdges(bboxForRun);
  }

  return bbox;
}




gfxMatrix
SVGTextFrame::GetCanvasTM()
{
  if (!mCanvasTM) {
    NS_ASSERTION(GetParent(), "null parent");
    NS_ASSERTION(!(GetStateBits() & NS_FRAME_IS_NONDISPLAY),
                 "should not call GetCanvasTM() when we are non-display");

    nsSVGContainerFrame *parent = static_cast<nsSVGContainerFrame*>(GetParent());
    dom::SVGTextContentElement *content = static_cast<dom::SVGTextContentElement*>(mContent);

    gfxMatrix tm = content->PrependLocalTransformsTo(parent->GetCanvasTM());

    mCanvasTM = new gfxMatrix(tm);
  }
  return *mCanvasTM;
}








static bool
HasTextContent(nsIContent* aContent)
{
  NS_ASSERTION(aContent, "expected non-null aContent");

  TextNodeIterator it(aContent);
  for (nsTextNode* text = it.Current(); text; text = it.Next()) {
    if (text->TextLength() != 0) {
      return true;
    }
  }
  return false;
}




static uint32_t
GetTextContentLength(nsIContent* aContent)
{
  NS_ASSERTION(aContent, "expected non-null aContent");

  uint32_t length = 0;
  TextNodeIterator it(aContent);
  for (nsTextNode* text = it.Current(); text; text = it.Next()) {
    length += text->TextLength();
  }
  return length;
}

int32_t
SVGTextFrame::ConvertTextElementCharIndexToAddressableIndex(
                                                           int32_t aIndex,
                                                           nsIContent* aContent)
{
  CharIterator it(this, CharIterator::eOriginal, aContent);
  if (!it.AdvanceToSubtree()) {
    return -1;
  }
  int32_t result = 0;
  int32_t textElementCharIndex;
  while (!it.AtEnd() &&
         it.IsWithinSubtree()) {
    bool addressable = !it.IsOriginalCharUnaddressable();
    textElementCharIndex = it.TextElementCharIndex();
    it.Next();
    uint32_t delta = it.TextElementCharIndex() - textElementCharIndex;
    aIndex -= delta;
    if (addressable) {
      if (aIndex < 0) {
        return result;
      }
      result += delta;
    }
  }
  return -1;
}





uint32_t
SVGTextFrame::GetNumberOfChars(nsIContent* aContent)
{
  UpdateGlyphPositioning();

  uint32_t n = 0;
  CharIterator it(this, CharIterator::eAddressable, aContent);
  if (it.AdvanceToSubtree()) {
    while (!it.AtEnd() && it.IsWithinSubtree()) {
      n++;
      it.Next();
    }
  }
  return n;
}





float
SVGTextFrame::GetComputedTextLength(nsIContent* aContent)
{
  UpdateGlyphPositioning();

  float cssPxPerDevPx = PresContext()->
    AppUnitsToFloatCSSPixels(PresContext()->AppUnitsPerDevPixel());

  nscoord length = 0;
  TextRenderedRunIterator it(this, TextRenderedRunIterator::eAllFrames,
                             aContent);
  for (TextRenderedRun run = it.Current(); run.mFrame; run = it.Next()) {
    length += run.GetAdvanceWidth();
  }

  return PresContext()->AppUnitsToGfxUnits(length) *
           cssPxPerDevPx * mLengthAdjustScaleFactor / mFontSizeScaleFactor;
}





nsresult
SVGTextFrame::SelectSubString(nsIContent* aContent,
                              uint32_t charnum, uint32_t nchars)
{
  UpdateGlyphPositioning();

  
  
  CharIterator chit(this, CharIterator::eAddressable, aContent);
  if (!chit.AdvanceToSubtree() ||
      !chit.Next(charnum) ||
      chit.IsAfterSubtree()) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }
  charnum = chit.TextElementCharIndex();
  nsIContent* content = chit.TextFrame()->GetContent();
  chit.NextWithinSubtree(nchars);
  nchars = chit.TextElementCharIndex() - charnum;

  nsRefPtr<nsFrameSelection> frameSelection = GetFrameSelection();

  frameSelection->HandleClick(content, charnum, charnum + nchars,
                              false, false, CARET_ASSOCIATE_BEFORE);
  return NS_OK;
}





nsresult
SVGTextFrame::GetSubStringLength(nsIContent* aContent,
                                 uint32_t charnum, uint32_t nchars,
                                 float* aResult)
{
  UpdateGlyphPositioning();

  
  
  CharIterator chit(this, CharIterator::eAddressable, aContent);
  if (!chit.AdvanceToSubtree() ||
      !chit.Next(charnum) ||
      chit.IsAfterSubtree()) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  if (nchars == 0) {
    *aResult = 0.0f;
    return NS_OK;
  }

  charnum = chit.TextElementCharIndex();
  chit.NextWithinSubtree(nchars);
  nchars = chit.TextElementCharIndex() - charnum;

  
  
  nscoord textLength = 0;
  TextRenderedRunIterator it(this, TextRenderedRunIterator::eAllFrames);
  TextRenderedRun run = it.Current();
  while (run.mFrame) {
    
    
    uint32_t offset = run.mTextElementCharIndex;
    if (offset >= charnum + nchars) {
      break;
    }

    
    
    uint32_t length = run.mTextFrameContentLength;
    IntersectInterval(offset, length, charnum, nchars);

    if (length != 0) {
      
      offset += run.mTextFrameContentOffset - run.mTextElementCharIndex;

      gfxSkipCharsIterator it =
        run.mFrame->EnsureTextRun(nsTextFrame::eInflated);
      gfxTextRun* textRun = run.mFrame->GetTextRun(nsTextFrame::eInflated);
      ConvertOriginalToSkipped(it, offset, length);

      
      textLength += textRun->GetAdvanceWidth(offset, length, nullptr);
    }

    run = it.Next();
  }

  nsPresContext* presContext = PresContext();
  float cssPxPerDevPx = presContext->
    AppUnitsToFloatCSSPixels(presContext->AppUnitsPerDevPixel());

  *aResult = presContext->AppUnitsToGfxUnits(textLength) *
               cssPxPerDevPx / mFontSizeScaleFactor;
  return NS_OK;
}





int32_t
SVGTextFrame::GetCharNumAtPosition(nsIContent* aContent,
                                   mozilla::nsISVGPoint* aPoint)
{
  UpdateGlyphPositioning();

  nsPresContext* context = PresContext();

  gfxPoint p(aPoint->X(), aPoint->Y());

  int32_t result = -1;

  TextRenderedRunIterator it(this, TextRenderedRunIterator::eAllFrames, aContent);
  for (TextRenderedRun run = it.Current(); run.mFrame; run = it.Next()) {
    
    int32_t index = run.GetCharNumAtPosition(context, p);
    if (index != -1) {
      result = index + run.mTextElementCharIndex;
    }
  }

  if (result == -1) {
    return result;
  }

  return ConvertTextElementCharIndexToAddressableIndex(result, aContent);
}





nsresult
SVGTextFrame::GetStartPositionOfChar(nsIContent* aContent,
                                     uint32_t aCharNum,
                                     mozilla::nsISVGPoint** aResult)
{
  UpdateGlyphPositioning();

  CharIterator it(this, CharIterator::eAddressable, aContent);
  if (!it.AdvanceToSubtree() ||
      !it.Next(aCharNum)) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  
  uint32_t startIndex = it.GlyphStartTextElementCharIndex();

  NS_ADDREF(*aResult =
    new DOMSVGPoint(ToPoint(mPositions[startIndex].mPosition)));
  return NS_OK;
}





nsresult
SVGTextFrame::GetEndPositionOfChar(nsIContent* aContent,
                                   uint32_t aCharNum,
                                   mozilla::nsISVGPoint** aResult)
{
  UpdateGlyphPositioning();

  CharIterator it(this, CharIterator::eAddressable, aContent);
  if (!it.AdvanceToSubtree() ||
      !it.Next(aCharNum)) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  
  uint32_t startIndex = it.GlyphStartTextElementCharIndex();

  
  gfxFloat advance = it.GetGlyphAdvance(PresContext());
  if (it.TextRun()->IsRightToLeft()) {
    advance = -advance;
  }

  
  
  Matrix m =
    Matrix::Rotation(mPositions[startIndex].mAngle) *
    Matrix::Translation(ToPoint(mPositions[startIndex].mPosition));
  Point p = m * Point(advance / mFontSizeScaleFactor, 0);

  NS_ADDREF(*aResult = new DOMSVGPoint(p));
  return NS_OK;
}





nsresult
SVGTextFrame::GetExtentOfChar(nsIContent* aContent,
                              uint32_t aCharNum,
                              dom::SVGIRect** aResult)
{
  UpdateGlyphPositioning();

  CharIterator it(this, CharIterator::eAddressable, aContent);
  if (!it.AdvanceToSubtree() ||
      !it.Next(aCharNum)) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  nsPresContext* presContext = PresContext();

  float cssPxPerDevPx = presContext->
    AppUnitsToFloatCSSPixels(presContext->AppUnitsPerDevPixel());

  
  uint32_t startIndex = it.GlyphStartTextElementCharIndex();

  
  gfxFloat ascent, descent;
  GetAscentAndDescentInAppUnits(it.TextFrame(), ascent, descent);

  
  gfxFloat advance = it.GetGlyphAdvance(presContext);
  gfxFloat x = it.TextRun()->IsRightToLeft() ? -advance : 0.0;

  
  
  gfxMatrix m;
  m.Translate(mPositions[startIndex].mPosition);
  m.Rotate(mPositions[startIndex].mAngle);
  m.Scale(1 / mFontSizeScaleFactor, 1 / mFontSizeScaleFactor);

  gfxRect glyphRect;
  if (it.TextRun()->IsVertical()) {
    glyphRect =
      gfxRect(-presContext->AppUnitsToGfxUnits(descent) * cssPxPerDevPx, x,
              presContext->AppUnitsToGfxUnits(ascent + descent) * cssPxPerDevPx,
              advance);
  } else {
    glyphRect =
      gfxRect(x, -presContext->AppUnitsToGfxUnits(ascent) * cssPxPerDevPx,
              advance,
              presContext->AppUnitsToGfxUnits(ascent + descent) * cssPxPerDevPx);
  }

  
  gfxRect r = m.TransformBounds(glyphRect);

  NS_ADDREF(*aResult = new dom::SVGRect(aContent, r.x, r.y, r.width, r.height));
  return NS_OK;
}





nsresult
SVGTextFrame::GetRotationOfChar(nsIContent* aContent,
                                uint32_t aCharNum,
                                float* aResult)
{
  UpdateGlyphPositioning();

  CharIterator it(this, CharIterator::eAddressable, aContent);
  if (!it.AdvanceToSubtree() ||
      !it.Next(aCharNum)) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  *aResult = mPositions[it.TextElementCharIndex()].mAngle * 180.0 / M_PI;
  return NS_OK;
}














static bool
ShouldStartRunAtIndex(const nsTArray<CharPosition>& aPositions,
                      const nsTArray<gfxPoint>& aDeltas,
                      uint32_t aIndex)
{
  if (aIndex == 0) {
    return true;
  }

  if (aIndex < aPositions.Length()) {
    
    if (aPositions[aIndex].IsXSpecified() ||
        aPositions[aIndex].IsYSpecified()) {
      return true;
    }

    
    
    if ((aPositions[aIndex].IsAngleSpecified() &&
         aPositions[aIndex].mAngle != 0.0f) ||
        (aPositions[aIndex - 1].IsAngleSpecified() &&
         (aPositions[aIndex - 1].mAngle != 0.0f))) {
      return true;
    }
  }

  if (aIndex < aDeltas.Length()) {
    
    if (aDeltas[aIndex].x != 0.0 ||
        aDeltas[aIndex].y != 0.0) {
      return true;
    }
  }

  return false;
}

bool
SVGTextFrame::ResolvePositionsForNode(nsIContent* aContent,
                                      uint32_t& aIndex,
                                      bool aInTextPath,
                                      bool& aForceStartOfChunk,
                                      nsTArray<gfxPoint>& aDeltas)
{
  if (aContent->IsNodeOfType(nsINode::eTEXT)) {
    
    uint32_t length = static_cast<nsTextNode*>(aContent)->TextLength();
    if (length) {
      uint32_t end = aIndex + length;
      if (MOZ_UNLIKELY(end > mPositions.Length())) {
        MOZ_ASSERT_UNREACHABLE("length of mPositions does not match characters "
                               "found by iterating content");
        return false;
      }
      if (aForceStartOfChunk) {
        
        mPositions[aIndex].mStartOfChunk = true;
        aForceStartOfChunk = false;
      }
      while (aIndex < end) {
        
        
        
        
        
        if (aInTextPath || ShouldStartRunAtIndex(mPositions, aDeltas, aIndex)) {
          mPositions[aIndex].mRunBoundary = true;
        }
        aIndex++;
      }
    }
    return true;
  }

  
  if (!IsTextContentElement(aContent)) {
    return true;
  }

  if (aContent->IsSVGElement(nsGkAtoms::textPath)) {
    
    
    if (HasTextContent(aContent)) {
      if (MOZ_UNLIKELY(aIndex >= mPositions.Length())) {
        MOZ_ASSERT_UNREACHABLE("length of mPositions does not match characters "
                               "found by iterating content");
        return false;
      }
      mPositions[aIndex].mPosition = gfxPoint();
      mPositions[aIndex].mStartOfChunk = true;
    }
  } else if (!aContent->IsSVGElement(nsGkAtoms::a)) {
    
    nsSVGElement* element = static_cast<nsSVGElement*>(aContent);

    
    SVGUserUnitList x, y, dx, dy;
    element->GetAnimatedLengthListValues(&x, &y, &dx, &dy, nullptr);

    
    const SVGNumberList* rotate = nullptr;
    SVGAnimatedNumberList* animatedRotate =
      element->GetAnimatedNumberList(nsGkAtoms::rotate);
    if (animatedRotate) {
      rotate = &animatedRotate->GetAnimValue();
    }

    bool percentages = false;
    uint32_t count = GetTextContentLength(aContent);

    if (MOZ_UNLIKELY(aIndex + count > mPositions.Length())) {
      MOZ_ASSERT_UNREACHABLE("length of mPositions does not match characters "
                             "found by iterating content");
      return false;
    }

    
    
    
    uint32_t newChunkCount = std::max(x.Length(), y.Length());
    if (!newChunkCount && aForceStartOfChunk) {
      newChunkCount = 1;
    }
    for (uint32_t i = 0, j = 0; i < newChunkCount && j < count; j++) {
      if (!mPositions[aIndex + j].mUnaddressable) {
        mPositions[aIndex + j].mStartOfChunk = true;
        i++;
      }
    }

    
    if (!dx.IsEmpty() || !dy.IsEmpty()) {
      
      aDeltas.EnsureLengthAtLeast(aIndex + count);
      for (uint32_t i = 0, j = 0; i < dx.Length() && j < count; j++) {
        if (!mPositions[aIndex + j].mUnaddressable) {
          aDeltas[aIndex + j].x = dx[i];
          percentages = percentages || dx.HasPercentageValueAt(i);
          i++;
        }
      }
      for (uint32_t i = 0, j = 0; i < dy.Length() && j < count; j++) {
        if (!mPositions[aIndex + j].mUnaddressable) {
          aDeltas[aIndex + j].y = dy[i];
          percentages = percentages || dy.HasPercentageValueAt(i);
          i++;
        }
      }
    }

    
    for (uint32_t i = 0, j = 0; i < x.Length() && j < count; j++) {
      if (!mPositions[aIndex + j].mUnaddressable) {
        mPositions[aIndex + j].mPosition.x = x[i];
        percentages = percentages || x.HasPercentageValueAt(i);
        i++;
      }
    }
    for (uint32_t i = 0, j = 0; i < y.Length() && j < count; j++) {
      if (!mPositions[aIndex + j].mUnaddressable) {
        mPositions[aIndex + j].mPosition.y = y[i];
        percentages = percentages || y.HasPercentageValueAt(i);
        i++;
      }
    }

    
    if (rotate && !rotate->IsEmpty()) {
      uint32_t i = 0, j = 0;
      while (i < rotate->Length() && j < count) {
        if (!mPositions[aIndex + j].mUnaddressable) {
          mPositions[aIndex + j].mAngle = M_PI * (*rotate)[i] / 180.0;
          i++;
        }
        j++;
      }
      
      while (j < count) {
        mPositions[aIndex + j].mAngle = mPositions[aIndex + j - 1].mAngle;
        j++;
      }
    }

    if (percentages) {
      AddStateBits(NS_STATE_SVG_POSITIONING_MAY_USE_PERCENTAGES);
    }
  }

  
  bool inTextPath = aInTextPath || aContent->IsSVGElement(nsGkAtoms::textPath);
  for (nsIContent* child = aContent->GetFirstChild();
       child;
       child = child->GetNextSibling()) {
    bool ok = ResolvePositionsForNode(child, aIndex, inTextPath,
                                      aForceStartOfChunk, aDeltas);
    if (!ok) {
      return false;
    }
  }

  if (aContent->IsSVGElement(nsGkAtoms::textPath)) {
    
    aForceStartOfChunk = true;
  }

  return true;
}

bool
SVGTextFrame::ResolvePositions(nsTArray<gfxPoint>& aDeltas,
                               bool aRunPerGlyph)
{
  NS_ASSERTION(mPositions.IsEmpty(), "expected mPositions to be empty");
  RemoveStateBits(NS_STATE_SVG_POSITIONING_MAY_USE_PERCENTAGES);

  CharIterator it(this, CharIterator::eOriginal);
  if (it.AtEnd()) {
    return false;
  }

  
  
  bool firstCharUnaddressable = it.IsOriginalCharUnaddressable();
  mPositions.AppendElement(CharPosition::Unspecified(firstCharUnaddressable));

  
  
  uint32_t index = 0;
  while (it.Next()) {
    while (++index < it.TextElementCharIndex()) {
      mPositions.AppendElement(CharPosition::Unspecified(false));
    }
    mPositions.AppendElement(CharPosition::Unspecified(
                                             it.IsOriginalCharUnaddressable()));
  }
  while (++index < it.TextElementCharIndex()) {
    mPositions.AppendElement(CharPosition::Unspecified(false));
  }

  
  bool forceStartOfChunk = false;
  index = 0;
  bool ok = ResolvePositionsForNode(mContent, index, aRunPerGlyph,
                                    forceStartOfChunk, aDeltas);
  return ok && index > 0;
}

void
SVGTextFrame::DetermineCharPositions(nsTArray<nsPoint>& aPositions)
{
  NS_ASSERTION(aPositions.IsEmpty(), "expected aPositions to be empty");

  nsPoint position, lastPosition;

  TextFrameIterator frit(this);
  for (nsTextFrame* frame = frit.Current(); frame; frame = frit.Next()) {
    gfxSkipCharsIterator it = frame->EnsureTextRun(nsTextFrame::eInflated);
    gfxTextRun* textRun = frame->GetTextRun(nsTextFrame::eInflated);

    
    position = frit.Position();
    if (textRun->IsVertical()) {
      if (textRun->IsRightToLeft()) {
        position.y += frame->GetRect().height;
      }
      position.x += GetBaselinePosition(frame, textRun,
                                        frit.DominantBaseline(),
                                        mFontSizeScaleFactor);
    } else {
      if (textRun->IsRightToLeft()) {
        position.x += frame->GetRect().width;
      }
      position.y += GetBaselinePosition(frame, textRun,
                                        frit.DominantBaseline(),
                                        mFontSizeScaleFactor);
    }

    
    for (uint32_t i = 0; i < frit.UndisplayedCharacters(); i++) {
      aPositions.AppendElement(position);
    }

    
    nsTextFrame::TrimmedOffsets trimmedOffsets =
      frame->GetTrimmedOffsets(frame->GetContent()->GetText(), true);
    while (it.GetOriginalOffset() < trimmedOffsets.mStart) {
      aPositions.AppendElement(position);
      it.AdvanceOriginal(1);
    }

    
    
    while (it.GetOriginalOffset() < frame->GetContentEnd() &&
           !it.IsOriginalCharSkipped() &&
           (!textRun->IsLigatureGroupStart(it.GetSkippedOffset()) ||
            !textRun->IsClusterStart(it.GetSkippedOffset()))) {
      nscoord advance = textRun->GetAdvanceWidth(it.GetSkippedOffset(), 1,
                                                 nullptr);
      (textRun->IsVertical() ? position.y : position.x) +=
        textRun->IsRightToLeft() ? -advance : advance;
      aPositions.AppendElement(lastPosition);
      it.AdvanceOriginal(1);
    }

    
    while (it.GetOriginalOffset() < frame->GetContentEnd()) {
      aPositions.AppendElement(position);
      if (!it.IsOriginalCharSkipped() &&
          textRun->IsLigatureGroupStart(it.GetSkippedOffset()) &&
          textRun->IsClusterStart(it.GetSkippedOffset())) {
        
        uint32_t length = ClusterLength(textRun, it);
        nscoord advance = textRun->GetAdvanceWidth(it.GetSkippedOffset(),
                                                   length, nullptr);
        (textRun->IsVertical() ? position.y : position.x) +=
          textRun->IsRightToLeft() ? -advance : advance;
        lastPosition = position;
      }
      it.AdvanceOriginal(1);
    }
  }

  
  for (uint32_t i = 0; i < frit.UndisplayedCharacters(); i++) {
    aPositions.AppendElement(position);
  }
}




enum TextAnchorSide {
  eAnchorLeft,
  eAnchorMiddle,
  eAnchorRight
};





static TextAnchorSide
ConvertLogicalTextAnchorToPhysical(uint8_t aTextAnchor, bool aIsRightToLeft)
{
  NS_ASSERTION(aTextAnchor <= 3, "unexpected value for aTextAnchor");
  if (!aIsRightToLeft)
    return TextAnchorSide(aTextAnchor);
  return TextAnchorSide(2 - aTextAnchor);
}















static void
ShiftAnchoredChunk(nsTArray<mozilla::CharPosition>& aCharPositions,
                   uint32_t aChunkStart,
                   uint32_t aChunkEnd,
                   gfxFloat aVisIStartEdge,
                   gfxFloat aVisIEndEdge,
                   TextAnchorSide aAnchorSide,
                   bool aVertical)
{
  NS_ASSERTION(aVisIStartEdge <= aVisIEndEdge,
               "unexpected anchored chunk edges");
  NS_ASSERTION(aChunkStart < aChunkEnd,
               "unexpected values for aChunkStart and aChunkEnd");

  gfxFloat shift = aVertical ? aCharPositions[aChunkStart].mPosition.y
                             : aCharPositions[aChunkStart].mPosition.x;
  switch (aAnchorSide) {
    case eAnchorLeft:
      shift -= aVisIStartEdge;
      break;
    case eAnchorMiddle:
      shift -= (aVisIStartEdge + aVisIEndEdge) / 2;
      break;
    case eAnchorRight:
      shift -= aVisIEndEdge;
      break;
    default:
      NS_NOTREACHED("unexpected value for aAnchorSide");
  }

  if (shift != 0.0) {
    if (aVertical) {
      for (uint32_t i = aChunkStart; i < aChunkEnd; i++) {
        aCharPositions[i].mPosition.y += shift;
      }
    } else {
      for (uint32_t i = aChunkStart; i < aChunkEnd; i++) {
        aCharPositions[i].mPosition.x += shift;
      }
    }
  }
}

void
SVGTextFrame::AdjustChunksForLineBreaks()
{
  nsBlockFrame* block = nsLayoutUtils::GetAsBlock(GetFirstPrincipalChild());
  NS_ASSERTION(block, "expected block frame");

  nsBlockFrame::line_iterator line = block->begin_lines();

  CharIterator it(this, CharIterator::eOriginal);
  while (!it.AtEnd() && line != block->end_lines()) {
    if (it.TextFrame() == line->mFirstChild) {
      mPositions[it.TextElementCharIndex()].mStartOfChunk = true;
      line++;
    }
    it.AdvancePastCurrentFrame();
  }
}

void
SVGTextFrame::AdjustPositionsForClusters()
{
  nsPresContext* presContext = PresContext();

  CharIterator it(this, CharIterator::eClusterOrLigatureGroupMiddle);
  while (!it.AtEnd()) {
    
    uint32_t charIndex = it.TextElementCharIndex();
    uint32_t startIndex = it.GlyphStartTextElementCharIndex();

    mPositions[charIndex].mClusterOrLigatureGroupMiddle = true;

    
    bool rotationAdjusted = false;
    double angle = mPositions[startIndex].mAngle;
    if (mPositions[charIndex].mAngle != angle) {
      mPositions[charIndex].mAngle = angle;
      rotationAdjusted = true;
    }

    
    
    uint32_t partLength =
      charIndex - startIndex - it.GlyphUndisplayedCharacters();
    gfxFloat advance =
      it.GetGlyphPartialAdvance(partLength, presContext) / mFontSizeScaleFactor;
    gfxPoint direction = gfxPoint(cos(angle), sin(angle)) *
                         (it.TextRun()->IsRightToLeft() ? -1.0 : 1.0);
    if (it.TextRun()->IsVertical()) {
      Swap(direction.x, direction.y);
    }
    mPositions[charIndex].mPosition = mPositions[startIndex].mPosition +
                                      direction * advance;

    
    
    if (mPositions[charIndex].mRunBoundary) {
      mPositions[charIndex].mRunBoundary = false;
      if (charIndex + 1 < mPositions.Length()) {
        mPositions[charIndex + 1].mRunBoundary = true;
      }
    } else if (rotationAdjusted) {
      if (charIndex + 1 < mPositions.Length()) {
        mPositions[charIndex + 1].mRunBoundary = true;
      }
    }

    
    
    if (mPositions[charIndex].mStartOfChunk) {
      mPositions[charIndex].mStartOfChunk = false;
      if (charIndex + 1 < mPositions.Length()) {
        mPositions[charIndex + 1].mStartOfChunk = true;
      }
    }

    it.Next();
  }
}

SVGPathElement*
SVGTextFrame::GetTextPathPathElement(nsIFrame* aTextPathFrame)
{
  nsSVGTextPathProperty *property = static_cast<nsSVGTextPathProperty*>
    (aTextPathFrame->Properties().Get(nsSVGEffects::HrefProperty()));

  if (!property) {
    nsIContent* content = aTextPathFrame->GetContent();
    dom::SVGTextPathElement* tp = static_cast<dom::SVGTextPathElement*>(content);
    nsAutoString href;
    tp->mStringAttributes[dom::SVGTextPathElement::HREF].GetAnimValue(href, tp);
    if (href.IsEmpty()) {
      return nullptr; 
    }

    nsCOMPtr<nsIURI> targetURI;
    nsCOMPtr<nsIURI> base = content->GetBaseURI();
    nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(targetURI), href,
                                              content->GetCurrentDoc(), base);

    property = nsSVGEffects::GetTextPathProperty(targetURI, aTextPathFrame,
                                                 nsSVGEffects::HrefProperty());
    if (!property)
      return nullptr;
  }

  Element* element = property->GetReferencedElement();
  return (element && element->IsSVGElement(nsGkAtoms::path)) ?
    static_cast<SVGPathElement*>(element) : nullptr;
}

TemporaryRef<Path>
SVGTextFrame::GetTextPath(nsIFrame* aTextPathFrame)
{
  SVGPathElement* element = GetTextPathPathElement(aTextPathFrame);
  if (!element) {
    return nullptr;
  }

  RefPtr<Path> path = element->GetOrBuildPathForMeasuring();
  if (!path) {
    return nullptr;
  }

  gfxMatrix matrix = element->PrependLocalTransformsTo(gfxMatrix());
  if (!matrix.IsIdentity()) {
    RefPtr<PathBuilder> builder =
      path->TransformedCopyToBuilder(ToMatrix(matrix));
    path = builder->Finish();
  }

  return path.forget();
}

gfxFloat
SVGTextFrame::GetOffsetScale(nsIFrame* aTextPathFrame)
{
  SVGPathElement* pathElement = GetTextPathPathElement(aTextPathFrame);
  if (!pathElement)
    return 1.0;

  return pathElement->GetPathLengthScale(dom::SVGPathElement::eForTextPath);
}

gfxFloat
SVGTextFrame::GetStartOffset(nsIFrame* aTextPathFrame)
{
  dom::SVGTextPathElement *tp =
    static_cast<dom::SVGTextPathElement*>(aTextPathFrame->GetContent());
  nsSVGLength2 *length =
    &tp->mLengthAttributes[dom::SVGTextPathElement::STARTOFFSET];

  if (length->IsPercentage()) {
    RefPtr<Path> data = GetTextPath(aTextPathFrame);
    return data ?
      length->GetAnimValInSpecifiedUnits() * data->ComputeLength() / 100.0 :
      0.0;
  }
  return length->GetAnimValue(tp) * GetOffsetScale(aTextPathFrame);
}

void
SVGTextFrame::DoTextPathLayout()
{
  nsPresContext* context = PresContext();

  CharIterator it(this, CharIterator::eClusterAndLigatureGroupStart);
  while (!it.AtEnd()) {
    nsIFrame* textPathFrame = it.TextPathFrame();
    if (!textPathFrame) {
      
      it.AdvancePastCurrentFrame();
      continue;
    }

    
    RefPtr<Path> path = GetTextPath(textPathFrame);
    if (!path) {
      it.AdvancePastCurrentTextPathFrame();
      continue;
    }

    nsIContent* textPath = textPathFrame->GetContent();

    gfxFloat offset = GetStartOffset(textPathFrame);
    Float pathLength = path->ComputeLength();

    
    do {
      uint32_t i = it.TextElementCharIndex();
      gfxFloat halfAdvance =
        it.GetGlyphAdvance(context) / mFontSizeScaleFactor / 2.0;
      gfxFloat sign = it.TextRun()->IsRightToLeft() ? -1.0 : 1.0;
      bool vertical = it.TextRun()->IsVertical();
      gfxFloat midx = (vertical ? mPositions[i].mPosition.y
                                : mPositions[i].mPosition.x) +
                      sign * halfAdvance + offset;

      
      mPositions[i].mHidden = midx < 0 || midx > pathLength;

      
      Point tangent; 
      Point pt = path->ComputePointAtLength(Float(midx), &tangent);
      Float rotation = vertical ? atan2f(-tangent.x, tangent.y)
                                : atan2f(tangent.y, tangent.x);
      Point normal(-tangent.y, tangent.x); 
      Point offsetFromPath = normal * (vertical ? mPositions[i].mPosition.x
                                                : mPositions[i].mPosition.y);
      pt += offsetFromPath;
      Point direction = tangent * sign;
      mPositions[i].mPosition = ThebesPoint(pt) - ThebesPoint(direction) * halfAdvance;
      mPositions[i].mAngle += rotation;

      
      for (uint32_t j = i + 1;
           j < mPositions.Length() && mPositions[j].mClusterOrLigatureGroupMiddle;
           j++) {
        gfxPoint partialAdvance =
          ThebesPoint(direction) * it.GetGlyphPartialAdvance(j - i, context) /
                                                         mFontSizeScaleFactor;
        mPositions[j].mPosition = mPositions[i].mPosition + partialAdvance;
        mPositions[j].mAngle = mPositions[i].mAngle;
        mPositions[j].mHidden = mPositions[i].mHidden;
      }
      it.Next();
    } while (it.TextPathFrame() &&
             it.TextPathFrame()->GetContent() == textPath);
  }
}

void
SVGTextFrame::DoAnchoring()
{
  nsPresContext* presContext = PresContext();

  CharIterator it(this, CharIterator::eOriginal);

  
  while (!it.AtEnd() &&
         (it.IsOriginalCharSkipped() || it.IsOriginalCharTrimmed())) {
    it.Next();
  }

  bool vertical = GetWritingMode().IsVertical();
  uint32_t start = it.TextElementCharIndex();
  while (start < mPositions.Length()) {
    it.AdvanceToCharacter(start);
    nsTextFrame* chunkFrame = it.TextFrame();

    
    
    uint32_t index = it.TextElementCharIndex();
    uint32_t end = start;
    gfxFloat left = std::numeric_limits<gfxFloat>::infinity();
    gfxFloat right = -std::numeric_limits<gfxFloat>::infinity();
    do {
      if (!it.IsOriginalCharSkipped() && !it.IsOriginalCharTrimmed()) {
        gfxFloat advance = it.GetAdvance(presContext) / mFontSizeScaleFactor;
        gfxFloat pos =
          it.TextRun()->IsVertical() ? mPositions[index].mPosition.y
                                     : mPositions[index].mPosition.x;
        if (it.TextRun()->IsRightToLeft()) {
          left  = std::min(left,  pos - advance);
          right = std::max(right, pos);
        } else {
          left  = std::min(left,  pos);
          right = std::max(right, pos + advance);
        }
      }
      it.Next();
      index = end = it.TextElementCharIndex();
    } while (!it.AtEnd() && !mPositions[end].mStartOfChunk);

    if (left != std::numeric_limits<gfxFloat>::infinity()) {
      bool isRTL =
        chunkFrame->StyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL;
      TextAnchorSide anchor =
        ConvertLogicalTextAnchorToPhysical(chunkFrame->StyleSVG()->mTextAnchor,
                                           isRTL);

      ShiftAnchoredChunk(mPositions, start, end, left, right, anchor,
                         vertical);
    }

    start = it.TextElementCharIndex();
  }
}

void
SVGTextFrame::DoGlyphPositioning()
{
  mPositions.Clear();
  RemoveStateBits(NS_STATE_SVG_POSITIONING_DIRTY);

  nsIFrame* kid = GetFirstPrincipalChild();
  if (kid && NS_SUBTREE_DIRTY(kid)) {
    MOZ_ASSERT(false, "should have already reflowed the kid");
    return;
  }

  
  nsTArray<nsPoint> charPositions;
  DetermineCharPositions(charPositions);

  if (charPositions.IsEmpty()) {
    
    return;
  }

  
  
  SVGTextContentElement* element = static_cast<SVGTextContentElement*>(mContent);
  nsSVGLength2* textLengthAttr =
    element->GetAnimatedLength(nsGkAtoms::textLength);
  bool adjustingTextLength = textLengthAttr->IsExplicitlySet();
  float expectedTextLength = textLengthAttr->GetAnimValue(element);

  if (adjustingTextLength && expectedTextLength < 0.0f) {
    
    adjustingTextLength = false;
  }

  
  nsTArray<gfxPoint> deltas;
  if (!ResolvePositions(deltas, adjustingTextLength)) {
    
    
    
    
    mPositions.Clear();
    return;
  }

  
  

  
  TruncateTo(deltas, charPositions);
  TruncateTo(mPositions, charPositions);

  
  if (!mPositions[0].IsXSpecified()) {
    mPositions[0].mPosition.x = 0.0;
  }
  if (!mPositions[0].IsYSpecified()) {
    mPositions[0].mPosition.y = 0.0;
  }
  if (!mPositions[0].IsAngleSpecified()) {
    mPositions[0].mAngle = 0.0;
  }

  nsPresContext* presContext = PresContext();
  bool vertical = GetWritingMode().IsVertical();

  float cssPxPerDevPx = presContext->
    AppUnitsToFloatCSSPixels(presContext->AppUnitsPerDevPixel());
  double factor = cssPxPerDevPx / mFontSizeScaleFactor;

  
  
  double adjustment = 0.0;
  mLengthAdjustScaleFactor = 1.0f;
  if (adjustingTextLength) {
    nscoord frameLength = vertical ? GetFirstPrincipalChild()->GetRect().height
                                   : GetFirstPrincipalChild()->GetRect().width;
    float actualTextLength =
      static_cast<float>(presContext->AppUnitsToGfxUnits(frameLength) * factor);

    nsRefPtr<SVGAnimatedEnumeration> lengthAdjustEnum = element->LengthAdjust();
    uint16_t lengthAdjust = lengthAdjustEnum->AnimVal();
    switch (lengthAdjust) {
      case SVG_LENGTHADJUST_SPACINGANDGLYPHS:
        
        if (actualTextLength > 0) {
          mLengthAdjustScaleFactor = expectedTextLength / actualTextLength;
        }
        break;

      default:
        MOZ_ASSERT(lengthAdjust == SVG_LENGTHADJUST_SPACING);
        
        int32_t adjustableSpaces = 0;
        for (uint32_t i = 1; i < mPositions.Length(); i++) {
          if (!mPositions[i].mUnaddressable) {
            adjustableSpaces++;
          }
        }
        if (adjustableSpaces) {
          adjustment = (expectedTextLength - actualTextLength) / adjustableSpaces;
        }
        break;
    }
  }

  
  
  if (!deltas.IsEmpty()) {
    mPositions[0].mPosition += deltas[0];
  }

  gfxFloat xLengthAdjustFactor = vertical ? 1.0 : mLengthAdjustScaleFactor;
  gfxFloat yLengthAdjustFactor = vertical ? mLengthAdjustScaleFactor : 1.0;
  for (uint32_t i = 1; i < mPositions.Length(); i++) {
    
    if (!mPositions[i].IsXSpecified()) {
      nscoord d = charPositions[i].x - charPositions[i - 1].x;
      mPositions[i].mPosition.x =
        mPositions[i - 1].mPosition.x +
        presContext->AppUnitsToGfxUnits(d) * factor * xLengthAdjustFactor;
      if (!vertical && !mPositions[i].mUnaddressable) {
        mPositions[i].mPosition.x += adjustment;
      }
    }
    
    if (!mPositions[i].IsYSpecified()) {
      nscoord d = charPositions[i].y - charPositions[i - 1].y;
      mPositions[i].mPosition.y =
        mPositions[i - 1].mPosition.y +
        presContext->AppUnitsToGfxUnits(d) * factor * yLengthAdjustFactor;
      if (vertical && !mPositions[i].mUnaddressable) {
        mPositions[i].mPosition.y += adjustment;
      }
    }
    
    if (i < deltas.Length()) {
      mPositions[i].mPosition += deltas[i];
    }
    
    if (!mPositions[i].IsAngleSpecified()) {
      mPositions[i].mAngle = 0.0f;
    }
  }

  MOZ_ASSERT(mPositions.Length() == charPositions.Length());

  AdjustChunksForLineBreaks();
  AdjustPositionsForClusters();
  DoAnchoring();
  DoTextPathLayout();
}

bool
SVGTextFrame::ShouldRenderAsPath(nsTextFrame* aFrame,
                                 bool& aShouldPaintSVGGlyphs)
{
  
  if (aFrame->GetParent()->GetParent()->GetStateBits() & NS_STATE_SVG_CLIPPATH_CHILD) {
    aShouldPaintSVGGlyphs = false;
    return true;
  }

  aShouldPaintSVGGlyphs = true;

  const nsStyleSVG* style = aFrame->StyleSVG();

  
  
  if (!(style->mFill.mType == eStyleSVGPaintType_None ||
        (style->mFill.mType == eStyleSVGPaintType_Color &&
         style->mFillOpacity == 1))) {
    return true;
  }

  
  if (style->HasStroke() &&
      SVGContentUtils::CoordToFloat(static_cast<nsSVGElement*>(mContent),
                                    style->mStrokeWidth) > 0) {
    return true;
  }

  return false;
}

void
SVGTextFrame::ScheduleReflowSVG()
{
  if (mState & NS_FRAME_IS_NONDISPLAY) {
    ScheduleReflowSVGNonDisplayText();
  } else {
    nsSVGUtils::ScheduleReflowSVG(this);
  }
}

void
SVGTextFrame::NotifyGlyphMetricsChange()
{
  AddStateBits(NS_STATE_SVG_POSITIONING_DIRTY);
  nsLayoutUtils::PostRestyleEvent(
    mContent->AsElement(), nsRestyleHint(0),
    nsChangeHint_InvalidateRenderingObservers);
  ScheduleReflowSVG();
}

void
SVGTextFrame::UpdateGlyphPositioning()
{
  nsIFrame* kid = GetFirstPrincipalChild();
  if (!kid) {
    return;
  }

  if (mState & NS_STATE_SVG_POSITIONING_DIRTY) {
    DoGlyphPositioning();
  }
}

void
SVGTextFrame::MaybeReflowAnonymousBlockChild()
{
  nsIFrame* kid = GetFirstPrincipalChild();
  if (!kid)
    return;

  NS_ASSERTION(!(kid->GetStateBits() & NS_FRAME_IN_REFLOW),
               "should not be in reflow when about to reflow again");

  if (NS_SUBTREE_DIRTY(this)) {
    if (mState & NS_FRAME_IS_DIRTY) {
      
      
      
      
      kid->AddStateBits(NS_FRAME_IS_DIRTY);
    }
    MOZ_ASSERT(nsSVGUtils::AnyOuterSVGIsCallingReflowSVG(this),
               "should be under ReflowSVG");
    nsPresContext::InterruptPreventer noInterrupts(PresContext());
    DoReflow();
  }
}

void
SVGTextFrame::DoReflow()
{
  
  
  AddStateBits(NS_STATE_SVG_POSITIONING_DIRTY);

  if (mState & NS_FRAME_IS_NONDISPLAY) {
    
    
    
    
    
    
    
    
    
    mState &= ~(NS_FRAME_IS_DIRTY | NS_FRAME_HAS_DIRTY_CHILDREN);
  }

  nsPresContext *presContext = PresContext();
  nsIFrame* kid = GetFirstPrincipalChild();
  if (!kid)
    return;

  nsRenderingContext renderingContext(
    presContext->PresShell()->CreateReferenceRenderingContext());

  if (UpdateFontSizeScaleFactor()) {
    
    
    kid->MarkIntrinsicISizesDirty();
  }

  mState |= NS_STATE_SVG_TEXT_IN_REFLOW;

  nscoord inlineSize = kid->GetPrefISize(&renderingContext);
  WritingMode wm = kid->GetWritingMode();
  nsHTMLReflowState reflowState(presContext, kid,
                                &renderingContext,
                                LogicalSize(wm, inlineSize,
                                            NS_UNCONSTRAINEDSIZE));
  nsHTMLReflowMetrics desiredSize(reflowState);
  nsReflowStatus status;

  NS_ASSERTION(reflowState.ComputedPhysicalBorderPadding() == nsMargin(0, 0, 0, 0) &&
               reflowState.ComputedPhysicalMargin() == nsMargin(0, 0, 0, 0),
               "style system should ensure that :-moz-svg-text "
               "does not get styled");

  kid->Reflow(presContext, desiredSize, reflowState, status);
  kid->DidReflow(presContext, &reflowState, nsDidReflowStatus::FINISHED);
  kid->SetSize(wm, desiredSize.Size(wm));

  mState &= ~NS_STATE_SVG_TEXT_IN_REFLOW;

  TextNodeCorrespondenceRecorder::RecordCorrespondence(this);
}


#define CLAMP_MIN_SIZE 8.0
#define CLAMP_MAX_SIZE 200.0
#define PRECISE_SIZE   200.0

bool
SVGTextFrame::UpdateFontSizeScaleFactor()
{
  double oldFontSizeScaleFactor = mFontSizeScaleFactor;

  nsPresContext* presContext = PresContext();

  bool geometricPrecision = false;
  nscoord min = nscoord_MAX,
          max = nscoord_MIN;

  
  
  TextFrameIterator it(this);
  nsTextFrame* f = it.Current();
  while (f) {
    if (!geometricPrecision) {
      
      
      geometricPrecision = f->StyleSVG()->mTextRendering ==
                             NS_STYLE_TEXT_RENDERING_GEOMETRICPRECISION;
    }
    nscoord size = f->StyleFont()->mFont.size;
    if (size) {
      min = std::min(min, size);
      max = std::max(max, size);
    }
    f = it.Next();
  }

  if (min == nscoord_MAX) {
    
    mFontSizeScaleFactor = 1.0;
    return mFontSizeScaleFactor != oldFontSizeScaleFactor;
  }

  double minSize = presContext->AppUnitsToFloatCSSPixels(min);

  if (geometricPrecision) {
    
    mFontSizeScaleFactor = PRECISE_SIZE / minSize;
    return mFontSizeScaleFactor != oldFontSizeScaleFactor;
  }

  
  
  
  
  
  double contextScale = 1.0;
  if (!(mState & NS_FRAME_IS_NONDISPLAY)) {
    gfxMatrix m(GetCanvasTM());
    if (!m.IsSingular()) {
      contextScale = GetContextScale(m);
    }
  }
  mLastContextScale = contextScale;

  double maxSize = presContext->AppUnitsToFloatCSSPixels(max);

  
  
  
  
  float cssPxPerDevPx =
    presContext->AppUnitsToFloatCSSPixels(presContext->AppUnitsPerDevPixel());
  contextScale *= cssPxPerDevPx;

  double minTextRunSize = minSize * contextScale;
  double maxTextRunSize = maxSize * contextScale;

  if (minTextRunSize >= CLAMP_MIN_SIZE &&
      maxTextRunSize <= CLAMP_MAX_SIZE) {
    
    
    mFontSizeScaleFactor = contextScale;
  } else if (maxSize / minSize > CLAMP_MAX_SIZE / CLAMP_MIN_SIZE) {
    
    
    
    mFontSizeScaleFactor = CLAMP_MIN_SIZE / minTextRunSize;
  } else if (minTextRunSize < CLAMP_MIN_SIZE) {
    mFontSizeScaleFactor = CLAMP_MIN_SIZE / minTextRunSize;
  } else {
    mFontSizeScaleFactor = CLAMP_MAX_SIZE / maxTextRunSize;
  }

  return mFontSizeScaleFactor != oldFontSizeScaleFactor;
}

double
SVGTextFrame::GetFontSizeScaleFactor() const
{
  return mFontSizeScaleFactor;
}






Point
SVGTextFrame::TransformFramePointToTextChild(const Point& aPoint,
                                             nsIFrame* aChildFrame)
{
  NS_ASSERTION(aChildFrame &&
               nsLayoutUtils::GetClosestFrameOfType
                 (aChildFrame->GetParent(), nsGkAtoms::svgTextFrame) == this,
               "aChildFrame must be a descendant of this frame");

  UpdateGlyphPositioning();

  nsPresContext* presContext = PresContext();

  
  
  
  float cssPxPerDevPx = presContext->
    AppUnitsToFloatCSSPixels(presContext->AppUnitsPerDevPixel());
  float factor = presContext->AppUnitsPerCSSPixel();
  Point framePosition(NSAppUnitsToFloatPixels(mRect.x, factor),
                      NSAppUnitsToFloatPixels(mRect.y, factor));
  Point pointInUserSpace = aPoint * cssPxPerDevPx + framePosition;

  
  TextRenderedRunIterator it(this, TextRenderedRunIterator::eAllFrames,
                             aChildFrame);
  TextRenderedRun hit;
  gfxPoint pointInRun;
  nscoord dx = nscoord_MAX;
  nscoord dy = nscoord_MAX;
  for (TextRenderedRun run = it.Current(); run.mFrame; run = it.Next()) {
    uint32_t flags = TextRenderedRun::eIncludeFill |
                     TextRenderedRun::eIncludeStroke |
                     TextRenderedRun::eNoHorizontalOverflow;
    gfxRect runRect = run.GetRunUserSpaceRect(presContext, flags).ToThebesRect();

    gfxMatrix m = run.GetTransformFromRunUserSpaceToUserSpace(presContext);
    if (!m.Invert()) {
      return aPoint;
    }
    gfxPoint pointInRunUserSpace = m.Transform(ThebesPoint(pointInUserSpace));

    if (Inside(runRect, pointInRunUserSpace)) {
      
      dx = 0;
      dy = 0;
      pointInRun = pointInRunUserSpace;
      hit = run;
    } else if (nsLayoutUtils::PointIsCloserToRect(pointInRunUserSpace,
                                                  runRect, dx, dy)) {
      
      
      pointInRun.x = clamped(pointInRunUserSpace.x,
                             runRect.X(), runRect.XMost());
      pointInRun.y = clamped(pointInRunUserSpace.y,
                             runRect.Y(), runRect.YMost());
      hit = run;
    }
  }

  if (!hit.mFrame) {
    
    return aPoint;
  }

  
  
  gfxMatrix m = hit.GetTransformFromRunUserSpaceToFrameUserSpace(presContext);
  m.Scale(mFontSizeScaleFactor, mFontSizeScaleFactor);
  return ToPoint(m.Transform(pointInRun) / cssPxPerDevPx);
}








gfxRect
SVGTextFrame::TransformFrameRectToTextChild(const gfxRect& aRect,
                                            nsIFrame* aChildFrame)
{
  NS_ASSERTION(aChildFrame &&
               nsLayoutUtils::GetClosestFrameOfType
                 (aChildFrame->GetParent(), nsGkAtoms::svgTextFrame) == this,
               "aChildFrame must be a descendant of this frame");

  UpdateGlyphPositioning();

  nsPresContext* presContext = PresContext();

  
  
  
  float cssPxPerDevPx = presContext->
    AppUnitsToFloatCSSPixels(presContext->AppUnitsPerDevPixel());
  float factor = presContext->AppUnitsPerCSSPixel();
  gfxPoint framePosition(NSAppUnitsToFloatPixels(mRect.x, factor),
                         NSAppUnitsToFloatPixels(mRect.y, factor));
  gfxRect incomingRectInUserSpace(aRect.x * cssPxPerDevPx + framePosition.x,
                                  aRect.y * cssPxPerDevPx + framePosition.y,
                                  aRect.width * cssPxPerDevPx,
                                  aRect.height * cssPxPerDevPx);

  
  gfxRect result;
  TextRenderedRunIterator it(this, TextRenderedRunIterator::eAllFrames,
                             aChildFrame);
  for (TextRenderedRun run = it.Current(); run.mFrame; run = it.Next()) {
    
    gfxMatrix userSpaceToRunUserSpace =
      run.GetTransformFromRunUserSpaceToUserSpace(presContext);
    if (!userSpaceToRunUserSpace.Invert()) {
      return result;
    }
    gfxMatrix m = run.GetTransformFromRunUserSpaceToFrameUserSpace(presContext) *
                    userSpaceToRunUserSpace;
    gfxRect incomingRectInFrameUserSpace =
      m.TransformBounds(incomingRectInUserSpace);

    
    uint32_t flags = TextRenderedRun::eIncludeFill |
                     TextRenderedRun::eIncludeStroke;
    SVGBBox runRectInFrameUserSpace = run.GetFrameUserSpaceRect(presContext, flags);
    if (runRectInFrameUserSpace.IsEmpty()) {
      continue;
    }
    gfxRect runIntersectionInFrameUserSpace =
      incomingRectInFrameUserSpace.Intersect(runRectInFrameUserSpace.ToThebesRect());

    if (!runIntersectionInFrameUserSpace.IsEmpty()) {
      
      runIntersectionInFrameUserSpace.x *= mFontSizeScaleFactor;
      runIntersectionInFrameUserSpace.y *= mFontSizeScaleFactor;
      runIntersectionInFrameUserSpace.width *= mFontSizeScaleFactor;
      runIntersectionInFrameUserSpace.height *= mFontSizeScaleFactor;

      
      nsPoint offset = run.mFrame->GetOffsetTo(aChildFrame);
      gfxRect runIntersection =
        runIntersectionInFrameUserSpace +
          gfxPoint(NSAppUnitsToFloatPixels(offset.x, factor),
                   NSAppUnitsToFloatPixels(offset.y, factor));

      
      result.UnionRect(result, runIntersection);
    }
  }

  return result;
}








gfxRect
SVGTextFrame::TransformFrameRectFromTextChild(const nsRect& aRect,
                                              nsIFrame* aChildFrame)
{
  NS_ASSERTION(aChildFrame &&
               nsLayoutUtils::GetClosestFrameOfType
                 (aChildFrame->GetParent(), nsGkAtoms::svgTextFrame) == this,
               "aChildFrame must be a descendant of this frame");

  UpdateGlyphPositioning();

  nsPresContext* presContext = PresContext();

  gfxRect result;
  TextRenderedRunIterator it(this, TextRenderedRunIterator::eAllFrames,
                             aChildFrame);
  for (TextRenderedRun run = it.Current(); run.mFrame; run = it.Next()) {
    
    nsRect rectInTextFrame = aRect + aChildFrame->GetOffsetTo(run.mFrame);

    
    gfxRect rectInFrameUserSpace =
      AppUnitsToFloatCSSPixels(gfxRect(rectInTextFrame.x,
                                       rectInTextFrame.y,
                                       rectInTextFrame.width,
                                       rectInTextFrame.height), presContext);

    
    uint32_t flags = TextRenderedRun::eIncludeFill |
                     TextRenderedRun::eIncludeStroke;
    rectInFrameUserSpace.IntersectRect
      (rectInFrameUserSpace, run.GetFrameUserSpaceRect(presContext, flags).ToThebesRect());

    if (!rectInFrameUserSpace.IsEmpty()) {
      
      
      gfxMatrix m = run.GetTransformFromRunUserSpaceToUserSpace(presContext);
      m.Scale(mFontSizeScaleFactor, mFontSizeScaleFactor);
      gfxRect rectInUserSpace = m.Transform(rectInFrameUserSpace);

      
      result.UnionRect(result, rectInUserSpace);
    }
  }

  
  
  float factor = presContext->AppUnitsPerCSSPixel();
  gfxPoint framePosition(NSAppUnitsToFloatPixels(mRect.x, factor),
                         NSAppUnitsToFloatPixels(mRect.y, factor));

  return result - framePosition;
}











static void
SetupInheritablePaint(const DrawTarget* aDrawTarget,
                      const gfxMatrix& aContextMatrix,
                      nsIFrame* aFrame,
                      float& aOpacity,
                      gfxTextContextPaint* aOuterContextPaint,
                      SVGTextContextPaint::Paint& aTargetPaint,
                      nsStyleSVGPaint nsStyleSVG::*aFillOrStroke,
                      const FramePropertyDescriptor* aProperty)
{
  const nsStyleSVG *style = aFrame->StyleSVG();
  nsSVGPaintServerFrame *ps =
    nsSVGEffects::GetPaintServer(aFrame, &(style->*aFillOrStroke), aProperty);

  if (ps) {
    nsRefPtr<gfxPattern> pattern =
      ps->GetPaintServerPattern(aFrame, aDrawTarget, aContextMatrix,
                                aFillOrStroke, aOpacity);
    if (pattern) {
      aTargetPaint.SetPaintServer(aFrame, aContextMatrix, ps);
      return;
    }
  }
  if (aOuterContextPaint) {
    nsRefPtr<gfxPattern> pattern;
    switch ((style->*aFillOrStroke).mType) {
    case eStyleSVGPaintType_ContextFill:
      pattern = aOuterContextPaint->GetFillPattern(aDrawTarget, aOpacity,
                                                   aContextMatrix);
      break;
    case eStyleSVGPaintType_ContextStroke:
      pattern = aOuterContextPaint->GetStrokePattern(aDrawTarget, aOpacity,
                                                     aContextMatrix);
      break;
    default:
      ;
    }
    if (pattern) {
      aTargetPaint.SetContextPaint(aOuterContextPaint, (style->*aFillOrStroke).mType);
      return;
    }
  }
  nscolor color =
    nsSVGUtils::GetFallbackOrPaintColor(aFrame->StyleContext(), aFillOrStroke);
  aTargetPaint.SetColor(color);
}

DrawMode
SVGTextFrame::SetupContextPaint(const DrawTarget* aDrawTarget,
                                const gfxMatrix& aContextMatrix,
                                nsIFrame* aFrame,
                                gfxTextContextPaint* aOuterContextPaint,
                                SVGTextContextPaint* aThisContextPaint)
{
  DrawMode toDraw = DrawMode(0);

  const nsStyleSVG *style = aFrame->StyleSVG();

  
  if (style->mFill.mType == eStyleSVGPaintType_None) {
    aThisContextPaint->SetFillOpacity(0.0f);
  } else {
    float opacity = nsSVGUtils::GetOpacity(style->mFillOpacitySource,
                                           style->mFillOpacity,
                                           aOuterContextPaint);

    SetupInheritablePaint(aDrawTarget, aContextMatrix, aFrame,
                          opacity, aOuterContextPaint,
                          aThisContextPaint->mFillPaint, &nsStyleSVG::mFill,
                          nsSVGEffects::FillProperty());

    aThisContextPaint->SetFillOpacity(opacity);

    toDraw = DrawMode(int(toDraw) | int(DrawMode::GLYPH_FILL));
  }

  
  if (style->mStroke.mType == eStyleSVGPaintType_None) {
    aThisContextPaint->SetStrokeOpacity(0.0f);
  } else {
    float opacity = nsSVGUtils::GetOpacity(style->mStrokeOpacitySource,
                                           style->mStrokeOpacity,
                                           aOuterContextPaint);

    SetupInheritablePaint(aDrawTarget, aContextMatrix, aFrame,
                          opacity, aOuterContextPaint,
                          aThisContextPaint->mStrokePaint, &nsStyleSVG::mStroke,
                          nsSVGEffects::StrokeProperty());

    aThisContextPaint->SetStrokeOpacity(opacity);

    toDraw = DrawMode(int(toDraw) | int(DrawMode::GLYPH_STROKE));
  }

  return toDraw;
}
