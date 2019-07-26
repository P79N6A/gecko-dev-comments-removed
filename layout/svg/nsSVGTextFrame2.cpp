





#include "nsSVGTextFrame2.h"


#include "DOMSVGPoint.h"
#include "gfxFont.h"
#include "gfxSkipChars.h"
#include "gfxTypes.h"
#include "LookAndFeel.h"
#include "nsAlgorithm.h"
#include "nsBlockFrame.h"
#include "nsCaret.h"
#include "nsContentUtils.h"
#include "nsGkAtoms.h"
#include "nsIDOMSVGAnimatedNumber.h"
#include "nsIDOMSVGLength.h"
#include "nsISVGGlyphFragmentNode.h"
#include "nsISelection.h"
#include "nsQuickSort.h"
#include "nsRenderingContext.h"
#include "nsSVGEffects.h"
#include "nsSVGGlyphFrame.h"
#include "nsSVGOuterSVGFrame.h"
#include "nsSVGPaintServerFrame.h"
#include "mozilla/dom/SVGRect.h"
#include "nsSVGIntegrationUtils.h"
#include "nsSVGTextFrame.h"
#include "nsSVGTextPathFrame.h"
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
#include <algorithm>
#include <cmath>
#include <limits>

using namespace mozilla;














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
  while (aContent && aContent->IsSVG(nsGkAtoms::a)) {
    aContent = aContent->GetParent();
  }
  return aContent;
}






















static bool
IsTextContentElement(nsIContent* aContent)
{
  if (!aContent->IsSVG()) {
    return false;
  }

  if (aContent->Tag() == nsGkAtoms::text) {
    nsIContent* parent = GetFirstNonAAncestor(aContent->GetParent());
    return !parent || !IsTextContentElement(parent);
  }

  if (aContent->Tag() == nsGkAtoms::textPath) {
    nsIContent* parent = GetFirstNonAAncestor(aContent->GetParent());
    return parent && parent->IsSVG(nsGkAtoms::text);
  }

  if (aContent->Tag() == nsGkAtoms::a ||
      aContent->Tag() == nsGkAtoms::tspan ||
      aContent->Tag() == nsGkAtoms::altGlyph) {
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
  if (!text) {
    return false;
  }

  nsIContent* content = text->GetContent();
  NS_ASSERTION(content && content->IsNodeOfType(nsINode::eTEXT),
               "unexpected content type for nsTextFrame");

  nsTextNode* node = static_cast<nsTextNode*>(content);
  if (node->TextLength() == 0) {
    return false;
  }

  aTextFrame = text;
  aTextNode = node;
  return true;
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
                    uint8_t aDominantBaseline)
{
  switch (aDominantBaseline) {
    case NS_STYLE_DOMINANT_BASELINE_HANGING:
    case NS_STYLE_DOMINANT_BASELINE_TEXT_BEFORE_EDGE:
      return 0;
    case NS_STYLE_DOMINANT_BASELINE_USE_SCRIPT:
    case NS_STYLE_DOMINANT_BASELINE_NO_CHANGE:
    case NS_STYLE_DOMINANT_BASELINE_RESET_SIZE:
      
      
      
      
    case NS_STYLE_DOMINANT_BASELINE_AUTO:
    case NS_STYLE_DOMINANT_BASELINE_ALPHABETIC:
      return aFrame->GetBaseline();
  }

  gfxTextRun::Metrics metrics =
    aTextRun->MeasureText(0, aTextRun->GetLength(), gfxFont::LOOSE_INK_EXTENTS,
                          nullptr, nullptr);

  switch (aDominantBaseline) {
    case NS_STYLE_DOMINANT_BASELINE_TEXT_AFTER_EDGE:
    case NS_STYLE_DOMINANT_BASELINE_IDEOGRAPHIC:
      return metrics.mAscent + metrics.mDescent;
    case NS_STYLE_DOMINANT_BASELINE_CENTRAL:
    case NS_STYLE_DOMINANT_BASELINE_MIDDLE:
    case NS_STYLE_DOMINANT_BASELINE_MATHEMATICAL:
      return (metrics.mAscent + metrics.mDescent) / 2.0;
  }

  NS_NOTREACHED("unexpected dominant-baseline value");
  return aFrame->GetBaseline();
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





namespace mozilla {


















struct TextRenderedRun
{
  



  TextRenderedRun()
    : mFrame(nullptr)
  {
  }

  




  TextRenderedRun(nsTextFrame* aFrame, const gfxPoint& aPosition,
                  double aRotate, float aFontSizeScaleFactor, nscoord aBaseline,
                  uint32_t aTextFrameContentOffset,
                  uint32_t aTextFrameContentLength,
                  uint32_t aTextElementCharIndex)
    : mFrame(aFrame),
      mPosition(aPosition),
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

  





















































































  gfxMatrix GetTransformFromUserSpaceForPainting(
                                      nsPresContext* aContext,
                                      const nsCharClipDisplayItem& aItem) const;

  







  gfxMatrix GetTransformFromRunUserSpaceToUserSpace(nsPresContext* aContext) const;

  





  gfxMatrix GetTransformFromRunUserSpaceToFrameUserSpace(nsPresContext* aContext) const;

  



  enum {
    
    eIncludeFill = 1,
    
    eIncludeStroke = 2,
    
    eNoHorizontalOverflow = 4
  };

  








  gfxRect GetRunUserSpaceRect(nsPresContext* aContext, uint32_t aFlags) const;

  


































  gfxRect GetFrameUserSpaceRect(nsPresContext* aContext, uint32_t aFlags) const;

  










  gfxRect GetUserSpaceRect(nsPresContext* aContext, uint32_t aFlags,
                           const gfxMatrix* aAdditionalTransform = nullptr) const;

  








  void GetClipEdges(nscoord& aLeftEdge, nscoord& aRightEdge) const;

  


  nscoord GetAdvanceWidth() const;

  







  int32_t GetCharNumAtPosition(nsPresContext* aContext,
                               const gfxPoint& aPoint) const;

  


  nsTextFrame* mFrame;

  





  gfxPoint mPosition;

  


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

  
  nsPoint t(IsRightToLeft() ?
              -mFrame->GetRect().width + aItem.mRightEdge :
              -aItem.mLeftEdge,
            -mBaseline);
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

  nscoord left, right;
  GetClipEdges(left, right);

  
  m.Translate(mPosition);

  
  m.Rotate(mRotate);

  
  nsPoint t(IsRightToLeft() ?
              -mFrame->GetRect().width + left + right :
              0,
            -mBaseline);
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

  nscoord left, right;
  GetClipEdges(left, right);

  
  
  return m.Translate(gfxPoint(gfxFloat(left) / aContext->AppUnitsPerCSSPixel(),
                              0));
}

gfxRect
TextRenderedRun::GetRunUserSpaceRect(nsPresContext* aContext,
                                     uint32_t aFlags) const
{
  gfxRect r;
  if (!mFrame) {
    return r;
  }

  
  
  
  
  
  
  nsRect self = mFrame->GetVisualOverflowRectRelativeToSelf();
  nsRect rect = mFrame->GetRect();
  nscoord above = -self.y;
  nscoord below = self.YMost() - rect.height;

  gfxSkipCharsIterator it = mFrame->EnsureTextRun(nsTextFrame::eInflated);
  gfxTextRun* textRun = mFrame->GetTextRun(nsTextFrame::eInflated);

  
  uint32_t offset, length;
  ConvertOriginalToSkipped(it, mTextFrameContentOffset, mTextFrameContentLength,
                           offset, length);

  
  gfxTextRun::Metrics metrics =
    textRun->MeasureText(offset, length, gfxFont::LOOSE_INK_EXTENTS,
                         nullptr, nullptr);

  
  
  
  nscoord baseline = metrics.mBoundingBox.y + metrics.mAscent;
  gfxFloat x, width;
  if (aFlags & eNoHorizontalOverflow) {
    x = 0.0;
    width = textRun->GetAdvanceWidth(offset, length, nullptr);
  } else {
    x = metrics.mBoundingBox.x;
    width = metrics.mBoundingBox.width;
  }
  gfxRect fillInAppUnits(x, baseline - above,
                         width, metrics.mBoundingBox.height + above + below);

  
  gfxRect fill = AppUnitsToFloatCSSPixels(fillInAppUnits, aContext);

  
  
  ScaleAround(fill,
              gfxPoint(0.0, aContext->AppUnitsToFloatCSSPixels(baseline)),
              1.0 / mFontSizeScaleFactor);

  
  if (aFlags & eIncludeFill) {
    r = fill;
  }

  
  if ((aFlags & eIncludeStroke) &&
      nsSVGUtils::GetStrokeWidth(mFrame) > 0) {
    r = r.Union(nsSVGUtils::PathExtentsToMaxStrokeExtents(fill, mFrame,
                                                          gfxMatrix()));
  }

  return r;
}

gfxRect
TextRenderedRun::GetFrameUserSpaceRect(nsPresContext* aContext,
                                       uint32_t aFlags) const
{
  gfxRect r = GetRunUserSpaceRect(aContext, aFlags);
  gfxMatrix m = GetTransformFromRunUserSpaceToFrameUserSpace(aContext);
  return m.TransformBounds(r);
}

gfxRect
TextRenderedRun::GetUserSpaceRect(nsPresContext* aContext,
                                  uint32_t aFlags,
                                  const gfxMatrix* aAdditionalTransform) const
{
  gfxRect r = GetRunUserSpaceRect(aContext, aFlags);
  gfxMatrix m = GetTransformFromRunUserSpaceToUserSpace(aContext);
  if (aAdditionalTransform) {
    m.Multiply(*aAdditionalTransform);
  }
  return m.TransformBounds(r);
}

void
TextRenderedRun::GetClipEdges(nscoord& aLeftEdge, nscoord& aRightEdge) const
{
  uint32_t contentLength = mFrame->GetContentLength();
  if (mTextFrameContentOffset == 0 &&
      mTextFrameContentLength == contentLength) {
    
    
    aLeftEdge = 0;
    aRightEdge = 0;
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

  
  
  nscoord leftEdge =
    textRun->GetAdvanceWidth(frameOffset, runOffset - frameOffset, nullptr);

  
  
  nscoord rightEdge =
    textRun->GetAdvanceWidth(runOffset + runLength,
                             frameOffset + frameLength - (runOffset + runLength),
                             nullptr);

  if (textRun->IsRightToLeft()) {
    aLeftEdge  = rightEdge;
    aRightEdge = leftEdge;
  } else {
    aLeftEdge  = leftEdge;
    aRightEdge = rightEdge;
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

  
  
  gfxMatrix m = GetTransformFromRunUserSpaceToUserSpace(aContext).Invert();
  gfxPoint p = m.Transform(aPoint) * mFontSizeScaleFactor;

  
  
  gfxFloat ascent, descent;
  GetAscentAndDescentInAppUnits(mFrame, ascent, descent);

  gfxFloat topEdge = mFrame->GetBaseline() - ascent;
  gfxFloat bottomEdge = topEdge + ascent + descent;

  if (p.y < aContext->AppUnitsToGfxUnits(topEdge) ||
      p.y >= aContext->AppUnitsToGfxUnits(bottomEdge)) {
    return -1;
  }

  gfxSkipCharsIterator it = mFrame->EnsureTextRun(nsTextFrame::eInflated);
  gfxTextRun* textRun = mFrame->GetTextRun(nsTextFrame::eInflated);

  
  
  uint32_t offset, length;
  ConvertOriginalToSkipped(it, mTextFrameContentOffset, mTextFrameContentLength,
                           offset, length);
  gfxFloat runAdvance =
    aContext->AppUnitsToGfxUnits(textRun->GetAdvanceWidth(offset, length,
                                                          nullptr));

  if (p.x < 0 || p.x >= runAdvance) {
    return -1;
  }

  
  
  
  bool rtl = textRun->IsRightToLeft();
  for (int32_t i = mTextFrameContentLength - 1; i >= 0; i--) {
    ConvertOriginalToSkipped(it, mTextFrameContentOffset, i, offset, length);
    gfxFloat advance =
      aContext->AppUnitsToGfxUnits(textRun->GetAdvanceWidth(offset, length,
                                                            nullptr));
    if ((rtl && p.x < runAdvance - advance) ||
        (!rtl && p.x >= advance)) {
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
  



  TextNodeIterator(nsIContent* aRoot, nsIContent* aSubtree = nullptr)
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
  TextNodeCorrespondence(uint32_t aUndisplayedCharacters)
    : mUndisplayedCharacters(aUndisplayedCharacters)
  {
  }

  uint32_t mUndisplayedCharacters;
};

static void DestroyTextNodeCorrespondence(void* aPropertyValue)
{
  delete static_cast<TextNodeCorrespondence*>(aPropertyValue);
}

NS_DECLARE_FRAME_PROPERTY(TextNodeCorrespondenceProperty, DestroyTextNodeCorrespondence)





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
  


  static void RecordCorrespondence(nsSVGTextFrame2* aRoot);

private:
  TextNodeCorrespondenceRecorder(nsSVGTextFrame2* aRoot)
    : mNodeIterator(aRoot->GetContent()),
      mPreviousNode(nullptr),
      mNodeCharIndex(0)
  {
  }

  void Record(nsSVGTextFrame2* aRoot);
  void TraverseAndRecord(nsIFrame* aFrame);

  


  nsTextNode* NextNode();

  



  TextNodeIterator mNodeIterator;

  


  nsTextNode* mPreviousNode;

  


  uint32_t mNodeCharIndex;
};

 void
TextNodeCorrespondenceRecorder::RecordCorrespondence(nsSVGTextFrame2* aRoot)
{
  TextNodeCorrespondenceRecorder recorder(aRoot);
  recorder.Record(aRoot);
}

void
TextNodeCorrespondenceRecorder::Record(nsSVGTextFrame2* aRoot)
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
  



  TextFrameIterator(nsSVGTextFrame2* aRoot, nsIFrame* aSubtree = nullptr)
    : mRootFrame(aRoot),
      mSubtree(aSubtree),
      mCurrentFrame(aRoot),
      mCurrentPosition(),
      mSubtreePosition(mSubtree ? eBeforeSubtree : eWithinSubtree)
  {
    Init();
  }

  



  TextFrameIterator(nsSVGTextFrame2* aRoot, nsIContent* aSubtree)
    : mRootFrame(aRoot),
      mSubtree(aSubtree && aSubtree != aRoot->GetContent() ?
                 aSubtree->GetPrimaryFrame() :
                 nullptr),
      mCurrentFrame(aRoot),
      mCurrentPosition(),
      mSubtreePosition(mSubtree ? eBeforeSubtree : eWithinSubtree)
  {
    Init();
  }

  


  nsSVGTextFrame2* Root() const
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
    mBaselines.AppendElement(mRootFrame->StyleSVGReset()->mDominantBaseline);
    Next();
  }

  




  void PushBaseline(nsIFrame* aNextFrame);

  


  void PopBaseline();

  


  nsSVGTextFrame2* mRootFrame;

  


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
        if (next->GetContent()->Tag() == nsGkAtoms::textPath) {
          
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
          if (mCurrentFrame->GetContent()->Tag() == nsGkAtoms::textPath) {
            
            mTextPathFrames.TruncateLength(mTextPathFrames.Length() - 1);
          }
          
          PopBaseline();
          if (mCurrentFrame == mSubtree) {
            
            mSubtreePosition = eAfterSubtree;
          }
          next = mCurrentFrame->GetNextSibling();
          if (next) {
            
            mCurrentPosition += next->GetPosition();
            if (next->GetContent()->Tag() == nsGkAtoms::textPath) {
              
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
  if (baseline != NS_STYLE_DOMINANT_BASELINE_AUTO) {
    mBaselines.AppendElement(baseline);
  } else {
    mBaselines.AppendElement(mBaselines[mBaselines.Length() - 1]);
  }
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

  










  TextRenderedRunIterator(nsSVGTextFrame2* aSVGTextFrame,
                          RenderedRunFilter aFilter = eAllFrames,
                          nsIFrame* aSubtree = nullptr)
    : mFrameIterator(aSVGTextFrame, aSubtree),
      mFilter(aFilter),
      mTextElementCharIndex(0),
      mFrameStartTextElementCharIndex(0),
      mFontSizeScaleFactor(aSVGTextFrame->mFontSizeScaleFactor),
      mCurrent(First())
  {
  }

  









  TextRenderedRunIterator(nsSVGTextFrame2* aSVGTextFrame,
                          RenderedRunFilter aFilter,
                          nsIContent* aSubtree)
    : mFrameIterator(aSVGTextFrame, aSubtree),
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
  


  nsSVGTextFrame2* Root() const
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

    
    
    pt = Root()->mPositions[mTextElementCharIndex].mPosition;
    rotate = Root()->mPositions[mTextElementCharIndex].mAngle;

    
    
    
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
                                   mFrameIterator.DominantBaseline());

    
    uint32_t untrimmedOffset = offset;
    uint32_t untrimmedLength = length;
    nsTextFrame::TrimmedOffsets trimmedOffsets =
      frame->GetTrimmedOffsets(frame->GetContent()->GetText(), true);
    TrimOffsets(offset, length, trimmedOffsets);
    charIndex += offset - untrimmedOffset;

    
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

  mCurrent = TextRenderedRun(frame, pt, rotate, mFontSizeScaleFactor, baseline,
                             offset, length, charIndex);
  return mCurrent;
}

TextRenderedRun
TextRenderedRunIterator::First()
{
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

  








  CharIterator(nsSVGTextFrame2* aSVGTextFrame,
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

  







  void GetOriginalGlyphOffsets(uint32_t& aOriginalOffset,
                               uint32_t& aOriginalLength) const;

  





  gfxFloat GetGlyphAdvance(nsPresContext* aContext) const;

  






  gfxFloat GetAdvance(nsPresContext* aContext) const;

  









  gfxFloat GetGlyphPartialAdvance(uint32_t aPartOffset, uint32_t aPartLength,
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
};

CharIterator::CharIterator(nsSVGTextFrame2* aSVGTextFrame,
                           CharIterator::CharacterFilter aFilter,
                           nsIContent* aSubtree)
  : mFilter(aFilter),
    mFrameIterator(aSVGTextFrame, aSubtree),
    mFrameForTrimCheck(nullptr),
    mTrimmedOffset(0),
    mTrimmedLength(0),
    mTextElementCharIndex(0),
    mGlyphStartTextElementCharIndex(0)
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
    if (!Next()) {
      break;
    }
    aCount--;
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
            mFrameForTrimCheck->StyleText()->NewlineIsSignificant() &&
            mFrameForTrimCheck->GetContent()->GetText()->CharAt(index) == '\n'));
}

void
CharIterator::GetOriginalGlyphOffsets(uint32_t& aOriginalOffset,
                                      uint32_t& aOriginalLength) const
{
  gfxSkipCharsIterator it = TextFrame()->EnsureTextRun(nsTextFrame::eInflated);
  it.SetOriginalOffset(mSkipCharsIterator.GetOriginalOffset() -
                         (mTextElementCharIndex - mGlyphStartTextElementCharIndex));

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
  return aContext->AppUnitsToGfxUnits(advance) * cssPxPerDevPx;
}

gfxFloat
CharIterator::GetAdvance(nsPresContext* aContext) const
{
  float cssPxPerDevPx = aContext->
    AppUnitsToFloatCSSPixels(aContext->AppUnitsPerDevPixel());

  gfxFloat advance =
    mTextRun->GetAdvanceWidth(mSkipCharsIterator.GetSkippedOffset(), 1, nullptr);
  return aContext->AppUnitsToGfxUnits(advance) * cssPxPerDevPx;
}

gfxFloat
CharIterator::GetGlyphPartialAdvance(uint32_t aPartOffset, uint32_t aPartLength,
                                     nsPresContext* aContext) const
{
  uint32_t offset, length;
  GetOriginalGlyphOffsets(offset, length);

  NS_ASSERTION(aPartOffset <= length && aPartOffset + aPartLength <= length,
               "invalid aPartOffset / aPartLength values");
  offset += aPartOffset;
  length = aPartLength;

  gfxSkipCharsIterator it = TextFrame()->EnsureTextRun(nsTextFrame::eInflated);
  ConvertOriginalToSkipped(it, offset, length);

  float cssPxPerDevPx = aContext->
    AppUnitsToFloatCSSPixels(aContext->AppUnitsPerDevPixel());

  gfxFloat advance = mTextRun->GetAdvanceWidth(offset, length, nullptr);
  return aContext->AppUnitsToGfxUnits(advance) * cssPxPerDevPx;
}

bool
CharIterator::NextCharacter()
{
  mTextElementCharIndex++;

  
  mSkipCharsIterator.AdvanceOriginal(1);
  if (mSkipCharsIterator.GetOriginalOffset() < TextFrame()->GetContentEnd()) {
    
    UpdateGlyphStartTextElementCharIndex();
    return true;
  }

  
  mFrameIterator.Next();

  
  mTextElementCharIndex += mFrameIterator.UndisplayedCharacters();
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
  SVGCharClipDisplayItem(const TextRenderedRun& aRun)
    : nsCharClipDisplayItem(aRun.mFrame)
  {
    aRun.GetClipEdges(mLeftEdge, mRightEdge);
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
      mRenderMode(SVGAutoRenderState::GetRenderMode(aContext)),
      mFrame(aFrame),
      mCanvasTM(aCanvasTM)
  {
  }

  void NotifyBeforeText(nscolor aColor);
  void NotifyGlyphPathEmitted();
  void NotifyBeforeSVGGlyphPainted();
  void NotifyAfterSVGGlyphPainted();
  void NotifyAfterText();
  void NotifyBeforeSelectionBackground(nscolor aColor);
  void NotifySelectionBackgroundPathEmitted();
  void NotifyBeforeDecorationLine(nscolor aColor);
  void NotifyDecorationLinePathEmitted();
  void NotifyBeforeSelectionDecorationLine(nscolor aColor);
  void NotifySelectionDecorationLinePathEmitted();

private:
  void FillWithOpacity();

  void SetupContext();

  



  void HandleTextGeometry();

  



  bool SetFillColor();

  


  void FillAndStroke();

  gfxContext* gfx;
  uint16_t mRenderMode;
  nsTextFrame* mFrame;
  const gfxMatrix& mCanvasTM;

  





  nscolor mColor;
};

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
SVGTextDrawPathCallbacks::NotifyBeforeSVGGlyphPainted()
{
  gfx->Save();
}

void
SVGTextDrawPathCallbacks::NotifyAfterSVGGlyphPainted()
{
  gfx->Restore();
  gfx->NewPath();
}

void
SVGTextDrawPathCallbacks::NotifyAfterText()
{
  gfx->Restore();
}

void
SVGTextDrawPathCallbacks::NotifyBeforeSelectionBackground(nscolor aColor)
{
  if (mRenderMode != SVGAutoRenderState::NORMAL) {
    
    return;
  }

  mColor = aColor;
  gfx->Save();
}

void
SVGTextDrawPathCallbacks::NotifySelectionBackgroundPathEmitted()
{
  if (mRenderMode != SVGAutoRenderState::NORMAL) {
    
    return;
  }

  if (SetFillColor()) {
    FillWithOpacity();
  }
  gfx->Restore();
}

void
SVGTextDrawPathCallbacks::NotifyBeforeDecorationLine(nscolor aColor)
{
  mColor = aColor;
  SetupContext();
}

void
SVGTextDrawPathCallbacks::NotifyDecorationLinePathEmitted()
{
  HandleTextGeometry();
  gfx->NewPath();
  gfx->Restore();
}

void
SVGTextDrawPathCallbacks::NotifyBeforeSelectionDecorationLine(nscolor aColor)
{
  if (mRenderMode != SVGAutoRenderState::NORMAL) {
    
    return;
  }

  mColor = aColor;
  gfx->Save();
}

void
SVGTextDrawPathCallbacks::NotifySelectionDecorationLinePathEmitted()
{
  if (mRenderMode != SVGAutoRenderState::NORMAL) {
    
    return;
  }

  FillAndStroke();
  gfx->Restore();
}

void
SVGTextDrawPathCallbacks::FillWithOpacity()
{
  gfx->FillWithOpacity(mColor == NS_40PERCENT_FOREGROUND_COLOR ? 0.4 : 1.0);
}

void
SVGTextDrawPathCallbacks::SetupContext()
{
  gfx->Save();

  
  
  
  switch (mFrame->StyleSVG()->mTextRendering) {
  case NS_STYLE_TEXT_RENDERING_OPTIMIZESPEED:
    gfx->SetAntialiasMode(gfxContext::MODE_ALIASED);
    break;
  default:
    gfx->SetAntialiasMode(gfxContext::MODE_COVERAGE);
    break;
  }
}

void
SVGTextDrawPathCallbacks::HandleTextGeometry()
{
  if (mRenderMode != SVGAutoRenderState::NORMAL) {
    
    if (mFrame->StyleSVG()->mClipRule == NS_STYLE_FILL_RULE_EVENODD)
      gfx->SetFillRule(gfxContext::FILL_RULE_EVEN_ODD);
    else
      gfx->SetFillRule(gfxContext::FILL_RULE_WINDING);

    if (mRenderMode == SVGAutoRenderState::CLIP_MASK) {
      gfx->SetColor(gfxRGBA(1.0f, 1.0f, 1.0f, 1.0f));
      gfx->Fill();
    }
  } else {
    
    gfxContextMatrixAutoSaveRestore saveMatrix(gfx);
    gfx->SetMatrix(mCanvasTM);

    FillAndStroke();
  }
}

bool
SVGTextDrawPathCallbacks::SetFillColor()
{
  if (mColor == NS_SAME_AS_FOREGROUND_COLOR ||
      mColor == NS_40PERCENT_FOREGROUND_COLOR) {
    return nsSVGUtils::SetupCairoFillPaint(mFrame, gfx);
  }

  if (mColor == NS_TRANSPARENT) {
    return false;
  }

  gfx->SetColor(gfxRGBA(mColor));
  return true;
}

void
SVGTextDrawPathCallbacks::FillAndStroke()
{
  bool pushedGroup = false;
  if (mColor == NS_40PERCENT_FOREGROUND_COLOR) {
    pushedGroup = true;
    gfx->PushGroup(gfxASurface::CONTENT_COLOR_ALPHA);
  }

  if (SetFillColor()) {
    gfx->Fill();
  }

  if (mColor == NS_SAME_AS_FOREGROUND_COLOR ||
      mColor == NS_40PERCENT_FOREGROUND_COLOR) {
    
    if (nsSVGUtils::SetupCairoStroke(mFrame, gfx)) {
      gfx->Stroke();
    }
  }

  if (pushedGroup) {
    gfx->PopGroupToSource();
    gfx->Paint(0.4);
  }
}




NS_IMETHODIMP
GlyphMetricsUpdater::Run()
{
  if (mFrame) {
    mFrame->mPositioningDirty = true;
    nsSVGUtils::InvalidateBounds(mFrame, false);
    nsSVGUtils::ScheduleReflowSVG(mFrame);
    mFrame->mGlyphMetricsUpdater = nullptr;
  }
  return NS_OK;
}

}







class nsDisplaySVGText : public nsDisplayItem {
public:
  nsDisplaySVGText(nsDisplayListBuilder* aBuilder,
                   nsSVGTextFrame2* aFrame)
    : nsDisplayItem(aBuilder, aFrame)
  {
    MOZ_COUNT_CTOR(nsDisplaySVGText);
    NS_ABORT_IF_FALSE(aFrame, "Must have a frame!");
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplaySVGText() {
    MOZ_COUNT_DTOR(nsDisplaySVGText);
  }
#endif

  NS_DISPLAY_DECL_NAME("nsDisplaySVGText", TYPE_SVG_TEXT)

  virtual void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                       HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames);
  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx);
};

void
nsDisplaySVGText::HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                          HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames)
{
  nsSVGTextFrame2 *frame = static_cast<nsSVGTextFrame2*>(mFrame);
  nsPoint pointRelativeToReferenceFrame = aRect.Center();
  
  nsPoint userSpacePt = pointRelativeToReferenceFrame -
                          (ToReferenceFrame() - frame->GetPosition());

  nsIFrame* target = frame->GetFrameForPoint(userSpacePt);
  if (target) {
    aOutFrames->AppendElement(target);
  }
}

void
nsDisplaySVGText::Paint(nsDisplayListBuilder* aBuilder,
                        nsRenderingContext* aCtx)
{
  
  
  
  nsPoint offset = ToReferenceFrame() - mFrame->GetPosition();

  aCtx->PushState();
  aCtx->Translate(offset);
  static_cast<nsSVGTextFrame2*>(mFrame)->PaintSVG(aCtx, nullptr);
  aCtx->PopState();
}




NS_QUERYFRAME_HEAD(nsSVGTextFrame2)
  NS_QUERYFRAME_ENTRY(nsSVGTextFrame2)
NS_QUERYFRAME_TAIL_INHERITING(nsSVGTextFrame2Base)




nsIFrame*
NS_NewSVGTextFrame2(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGTextFrame2(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGTextFrame2)




void
nsSVGTextFrame2::Init(nsIContent* aContent,
                      nsIFrame* aParent,
                      nsIFrame* aPrevInFlow)
{
  NS_ASSERTION(aContent->IsSVG(nsGkAtoms::text), "Content is not an SVG text");

  nsSVGTextFrame2Base::Init(aContent, aParent, aPrevInFlow);
  AddStateBits((aParent->GetStateBits() &
                (NS_STATE_SVG_NONDISPLAY_CHILD | NS_STATE_SVG_CLIPPATH_CHILD)) |
               NS_FRAME_SVG_LAYOUT | NS_FRAME_IS_SVG_TEXT);

  mMutationObserver.StartObserving(this);
}

void
nsSVGTextFrame2::DestroyFrom(nsIFrame* aDestructRoot)
{
  if (mGlyphMetricsUpdater) {
    mGlyphMetricsUpdater->Revoke();
  }
  nsSVGTextFrame2Base::DestroyFrom(aDestructRoot);
}

void
nsSVGTextFrame2::BuildDisplayList(nsDisplayListBuilder* aBuilder,
                                  const nsRect& aDirtyRect,
                                  const nsDisplayListSet& aLists)
{
  if (NS_SUBTREE_DIRTY(this)) {
    
    
    
    return;
  }
  aLists.Content()->AppendNewToTop(
    new (aBuilder) nsDisplaySVGText(aBuilder, this));
}

NS_IMETHODIMP
nsSVGTextFrame2::AttributeChanged(int32_t aNameSpaceID,
                                  nsIAtom* aAttribute,
                                  int32_t aModType)
{
  if (aNameSpaceID != kNameSpaceID_None)
    return NS_OK;

  if (aAttribute == nsGkAtoms::transform) {
    NotifySVGChanged(TRANSFORM_CHANGED);
  } else if (IsGlyphPositioningAttribute(aAttribute)) {
    NotifyGlyphMetricsChange();
  }

  return NS_OK;
}

nsIAtom *
nsSVGTextFrame2::GetType() const
{
  return nsGkAtoms::svgTextFrame2;
}

NS_IMPL_ISUPPORTS1(nsSVGTextFrame2::MutationObserver, nsIMutationObserver)

void
nsSVGTextFrame2::MutationObserver::ContentAppended(nsIDocument* aDocument,
                                                   nsIContent* aContainer,
                                                   nsIContent* aFirstNewContent,
                                                   int32_t aNewIndexInContainer)
{
  mFrame->NotifyGlyphMetricsChange();
}

void
nsSVGTextFrame2::MutationObserver::ContentInserted(
                                        nsIDocument* aDocument,
                                        nsIContent* aContainer,
                                        nsIContent* aChild,
                                        int32_t aIndexInContainer)
{
  mFrame->NotifyGlyphMetricsChange();
}

void
nsSVGTextFrame2::MutationObserver::ContentRemoved(
                                       nsIDocument *aDocument,
                                       nsIContent* aContainer,
                                       nsIContent* aChild,
                                       int32_t aIndexInContainer,
                                       nsIContent* aPreviousSibling)
{
  mFrame->NotifyGlyphMetricsChange();
}

void
nsSVGTextFrame2::MutationObserver::AttributeChanged(
                                                nsIDocument* aDocument,
                                                mozilla::dom::Element* aElement,
                                                int32_t aNameSpaceID,
                                                nsIAtom* aAttribute,
                                                int32_t aModType)
{
  if (!aElement->IsSVG()) {
    return;
  }

  
  
  if (aElement == mFrame->GetContent()) {
    return;
  }

  
  if (aElement->Tag() == nsGkAtoms::textPath) {
    if (aNameSpaceID == kNameSpaceID_None &&
        aAttribute == nsGkAtoms::startOffset) {
      mFrame->NotifyGlyphMetricsChange();
    } else if (aNameSpaceID == kNameSpaceID_XLink &&
               aAttribute == nsGkAtoms::href) {
      
      nsIFrame* childElementFrame = aElement->GetPrimaryFrame();
      if (childElementFrame) {
        childElementFrame->Properties().Delete(nsSVGEffects::HrefProperty());
        mFrame->NotifyGlyphMetricsChange();
      }
    }
  } else {
    if (aNameSpaceID == kNameSpaceID_None &&
        IsGlyphPositioningAttribute(aAttribute)) {
      mFrame->NotifyGlyphMetricsChange();
    }
  }
}

NS_IMETHODIMP
nsSVGTextFrame2::Reflow(nsPresContext*           aPresContext,
                        nsHTMLReflowMetrics&     aDesiredSize,
                        const nsHTMLReflowState& aReflowState,
                        nsReflowStatus&          aStatus)
{
  NS_ABORT_IF_FALSE(!(GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD),
                    "Should not have been called");

  
  
  
  
  
  NS_ASSERTION(!(GetStateBits() & NS_FRAME_IS_DIRTY),
               "Reflowing while a resize is pending is wasteful");

  

  NS_ASSERTION(!aReflowState.parentReflowState,
               "should only get reflow from being reflow root");
  NS_ASSERTION(aReflowState.ComputedWidth() == GetSize().width &&
               aReflowState.ComputedHeight() == GetSize().height,
               "reflow roots should be reflowed at existing size and "
               "svg.css should ensure we have no padding/border/margin");

  DoReflow(false);

  aDesiredSize.width = aReflowState.ComputedWidth();
  aDesiredSize.height = aReflowState.ComputedHeight();
  aDesiredSize.SetOverflowAreasToDesiredBounds();
  aStatus = NS_FRAME_COMPLETE;

  return NS_OK;
}

void
nsSVGTextFrame2::FindCloserFrameForSelection(
                                 nsPoint aPoint,
                                 nsIFrame::FrameWithDistance* aCurrentBestFrame)
{
  if (GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD) {
    return;
  }
  UpdateGlyphPositioning(true);

  nsPresContext* presContext = PresContext();

  
  TextRenderedRunIterator it(this);
  for (TextRenderedRun run = it.Current(); run.mFrame; run = it.Next()) {
    uint32_t flags = TextRenderedRun::eIncludeFill |
                     TextRenderedRun::eIncludeStroke |
                     TextRenderedRun::eNoHorizontalOverflow;
    gfxRect userRect = run.GetUserSpaceRect(presContext, flags);

    nsRect rect = nsSVGUtils::ToCanvasBounds(userRect,
                                             GetCanvasTM(FOR_HIT_TESTING),
                                             presContext);

    if (nsLayoutUtils::PointIsCloserToRect(aPoint, rect,
                                           aCurrentBestFrame->mXDistance,
                                           aCurrentBestFrame->mYDistance)) {
      aCurrentBestFrame->mFrame = run.mFrame;
    }
  }
}




void
nsSVGTextFrame2::NotifySVGChanged(uint32_t aFlags)
{
  NS_ABORT_IF_FALSE(aFlags & (TRANSFORM_CHANGED | COORD_CONTEXT_CHANGED),
                    "Invalidation logic may need adjusting");

  bool needNewBounds = false;
  bool needGlyphMetricsUpdate = false;
  bool needNewCanvasTM = false;

  if (aFlags & COORD_CONTEXT_CHANGED) {
    needGlyphMetricsUpdate = true;
  }

  if (aFlags & TRANSFORM_CHANGED) {
    needNewCanvasTM = true;
    if (mCanvasTM && mCanvasTM->IsSingular()) {
      
      needNewBounds = true;
      needGlyphMetricsUpdate = true;
    }
  }

  if (needNewBounds) {
    
    
    
    
    
    nsSVGUtils::ScheduleReflowSVG(this);
  }

  if (needGlyphMetricsUpdate) {
    
    
    
    
    if (!(mState & NS_FRAME_FIRST_REFLOW)) {
      NotifyGlyphMetricsChange();
    }
  }

  if (needNewCanvasTM) {
    
    
    mCanvasTM = nullptr;
  }
}




static int32_t
GetCaretOffset(nsCaret* aCaret)
{
  nsCOMPtr<nsISelection> selection = aCaret->GetCaretDOMSelection();
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

NS_IMETHODIMP
nsSVGTextFrame2::PaintSVG(nsRenderingContext* aContext,
                          const nsIntRect *aDirtyRect)
{
  nsIFrame* kid = GetFirstPrincipalChild();
  if (!kid)
    return NS_OK;

  gfxContext *gfx = aContext->ThebesContext();
  gfxMatrix initialMatrix = gfx->CurrentMatrix();

  AutoCanvasTMForMarker autoCanvasTMFor(this, FOR_PAINTING);

  if (mState & NS_STATE_SVG_NONDISPLAY_CHILD) {
    
    
    UpdateGlyphPositioning(true);
  } else if (NS_SUBTREE_DIRTY(this)) {
    
    
    
    return NS_OK;
  }

  gfxMatrix canvasTM = GetCanvasTM(FOR_PAINTING);
  if (canvasTM.IsSingular()) {
    NS_WARNING("Can't render text element!");
    return NS_ERROR_FAILURE;
  }

  gfxMatrix matrixForPaintServers(canvasTM);
  matrixForPaintServers.Multiply(initialMatrix);

  nsPresContext* presContext = PresContext();

  
  if (aDirtyRect) {
    NS_ASSERTION(!NS_SVGDisplayListPaintingEnabled() ||
                 (mState & NS_STATE_SVG_NONDISPLAY_CHILD),
                 "Display lists handle dirty rect intersection test");
    nsRect dirtyRect(aDirtyRect->x, aDirtyRect->y,
                     aDirtyRect->width, aDirtyRect->height);

    gfxFloat appUnitsPerDevPixel = presContext->AppUnitsPerDevPixel();
    gfxRect frameRect(mRect.x / appUnitsPerDevPixel,
                      mRect.y / appUnitsPerDevPixel,
                      mRect.width / appUnitsPerDevPixel,
                      mRect.height / appUnitsPerDevPixel);

    nsRect canvasRect = nsLayoutUtils::RoundGfxRectToAppRect(
        GetCanvasTM(FOR_OUTERSVG_TM).TransformBounds(frameRect), 1);
    if (!canvasRect.Intersects(dirtyRect)) {
      return NS_OK;
    }
  }

  
  
  
  float cssPxPerDevPx = presContext->
    AppUnitsToFloatCSSPixels(presContext->AppUnitsPerDevPixel());
  gfxMatrix canvasTMForChildren = canvasTM;
  canvasTMForChildren.Scale(cssPxPerDevPx, cssPxPerDevPx);
  initialMatrix.Scale(1 / cssPxPerDevPx, 1 / cssPxPerDevPx);

  gfxContextAutoSaveRestore save(gfx);
  gfx->NewPath();
  gfx->Multiply(canvasTMForChildren);
  gfxMatrix currentMatrix = gfx->CurrentMatrix();

  nsRefPtr<nsCaret> caret = presContext->PresShell()->GetCaret();
  nsIFrame* caretFrame = caret->GetCaretFrame();

  TextRenderedRunIterator it(this, TextRenderedRunIterator::eVisibleFrames);
  TextRenderedRun run = it.Current();
  while (run.mFrame) {
    nsTextFrame* frame = run.mFrame;

    
    
    SVGCharClipDisplayItem item(run);

    
    
    gfx->SetMatrix(initialMatrix);
    gfxTextObjectPaint *outerObjectPaint =
      (gfxTextObjectPaint*)aContext->GetUserData(&gfxTextObjectPaint::sUserDataKey);

    nsAutoPtr<gfxTextObjectPaint> objectPaint;
    SetupCairoState(gfx, frame, outerObjectPaint, getter_Transfers(objectPaint));

    
    
    gfxMatrix runTransform =
      run.GetTransformFromUserSpaceForPainting(presContext, item);
    runTransform.Multiply(currentMatrix);
    gfx->SetMatrix(runTransform);

    nsRect frameRect = frame->GetVisualOverflowRect();
    bool paintSVGGlyphs;
    if (ShouldRenderAsPath(aContext, frame, paintSVGGlyphs)) {
      SVGTextDrawPathCallbacks callbacks(aContext, frame, matrixForPaintServers,
                                         paintSVGGlyphs);
      frame->PaintText(aContext, nsPoint(), frameRect, item,
                       objectPaint, &callbacks);
    } else {
      frame->PaintText(aContext, nsPoint(), frameRect, item,
                       objectPaint, nullptr);
    }

    if (frame == caretFrame && ShouldPaintCaret(run, caret)) {
      
      
      caret->PaintCaret(nullptr, aContext, frame, nsPoint());
      gfx->NewPath();
    }

    run = it.Next();
  }

  return NS_OK;
}

NS_IMETHODIMP_(nsIFrame*)
nsSVGTextFrame2::GetFrameForPoint(const nsPoint& aPoint)
{
  NS_ASSERTION(GetFirstPrincipalChild(), "must have a child frame");

  AutoCanvasTMForMarker autoCanvasTMFor(this, FOR_HIT_TESTING);

  if (mState & NS_STATE_SVG_NONDISPLAY_CHILD) {
    
    
    
    
    UpdateGlyphPositioning(true);
  } else {
    NS_ASSERTION(!NS_SUBTREE_DIRTY(this), "reflow should have happened");
  }

  nsPresContext* presContext = PresContext();

  gfxPoint pointInOuterSVGUserUnits = AppUnitsToGfxUnits(aPoint, presContext);

  TextRenderedRunIterator it(this);
  nsIFrame* hit = nullptr;
  for (TextRenderedRun run = it.Current(); run.mFrame; run = it.Next()) {
    uint16_t hitTestFlags = nsSVGUtils::GetGeometryHitTestFlags(run.mFrame);
    if (!(hitTestFlags & (SVG_HIT_TEST_FILL | SVG_HIT_TEST_STROKE))) {
      continue;
    }

    gfxMatrix m = GetCanvasTM(FOR_HIT_TESTING);
    m.PreMultiply(run.GetTransformFromRunUserSpaceToUserSpace(presContext));
    m.Invert();

    gfxPoint pointInRunUserSpace = m.Transform(pointInOuterSVGUserUnits);
    gfxRect frameRect =
      run.GetRunUserSpaceRect(presContext, TextRenderedRun::eIncludeFill |
                                           TextRenderedRun::eIncludeStroke);

    if (Inside(frameRect, pointInRunUserSpace) &&
        nsSVGUtils::HitTestClip(this, aPoint)) {
      hit = run.mFrame;
    }
  }
  return hit;
}

NS_IMETHODIMP_(nsRect)
nsSVGTextFrame2::GetCoveredRegion()
{
  return nsSVGUtils::TransformFrameRectToOuterSVG(
           mRect, GetCanvasTM(FOR_OUTERSVG_TM), PresContext());
}

void
nsSVGTextFrame2::ReflowSVG()
{
  NS_ASSERTION(nsSVGUtils::OuterSVGIsCallingReflowSVG(this),
               "This call is probaby a wasteful mistake");

  NS_ABORT_IF_FALSE(!(GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD),
                    "ReflowSVG mechanism not designed for this");

  if (!nsSVGUtils::NeedsReflowSVG(this)) {
    NS_ASSERTION(!mPositioningDirty, "How did this happen?");
    return;
  }

  
  
  
  mPositioningDirty = true;

  
  UpdateGlyphPositioning(false);

  nsPresContext* presContext = PresContext();

  gfxRect r;
  TextRenderedRunIterator it(this, TextRenderedRunIterator::eAllFrames);
  for (TextRenderedRun run = it.Current(); run.mFrame; run = it.Next()) {
    uint32_t runFlags = 0;
    uint16_t hitTestFlags = nsSVGUtils::GetGeometryHitTestFlags(run.mFrame);

    if ((hitTestFlags & SVG_HIT_TEST_FILL) ||
        run.mFrame->StyleSVG()->mFill.mType != eStyleSVGPaintType_None) {
      runFlags |= TextRenderedRun::eIncludeFill;
    }
    if ((hitTestFlags & SVG_HIT_TEST_STROKE) ||
        nsSVGUtils::HasStroke(run.mFrame)) {
      runFlags |= TextRenderedRun::eIncludeStroke;
    }

    if (runFlags) {
      r = r.Union(run.GetUserSpaceRect(presContext, runFlags));
    }
  }
  mRect =
    nsLayoutUtils::RoundGfxRectToAppRect(r, presContext->AppUnitsPerCSSPixel());


  if (mState & NS_FRAME_FIRST_REFLOW) {
    
    
    
    nsSVGEffects::UpdateEffects(this);
  }

  
  
  
  
  
  bool invalidate = (mState & NS_FRAME_IS_DIRTY) &&
    !(GetParent()->GetStateBits() &
       (NS_FRAME_FIRST_REFLOW | NS_FRAME_IS_DIRTY));

  nsRect overflow = nsRect(nsPoint(0,0), mRect.Size());
  nsOverflowAreas overflowAreas(overflow, overflow);
  FinishAndStoreOverflow(overflowAreas, mRect.Size());

  
  mState &= ~(NS_FRAME_FIRST_REFLOW | NS_FRAME_IS_DIRTY |
              NS_FRAME_HAS_DIRTY_CHILDREN);

  
  
  
  nsSVGTextFrame2Base::ReflowSVG();

  if (invalidate) {
    
    nsSVGUtils::InvalidateBounds(this, true);
  }
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
nsSVGTextFrame2::GetBBoxContribution(const gfxMatrix &aToBBoxUserspace,
                                     uint32_t aFlags)
{
  NS_ASSERTION(GetFirstPrincipalChild(), "must have a child frame");

  UpdateGlyphPositioning(true);

  gfxRect bbox;
  nsPresContext* presContext = PresContext();

  TextRenderedRunIterator it(this);
  for (TextRenderedRun run = it.Current(); run.mFrame; run = it.Next()) {
    uint32_t flags = TextRenderedRunFlagsForBBoxContribution(run, aFlags);
    gfxRect bboxForRun =
      run.GetUserSpaceRect(presContext, flags, &aToBBoxUserspace);
    bbox = bbox.Union(bboxForRun);
  }

  return bbox;
}




gfxMatrix
nsSVGTextFrame2::GetCanvasTM(uint32_t aFor)
{
  if (!(GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD)) {
    if ((aFor == FOR_PAINTING && NS_SVGDisplayListPaintingEnabled()) ||
        (aFor == FOR_HIT_TESTING && NS_SVGDisplayListHitTestingEnabled())) {
      return nsSVGIntegrationUtils::GetCSSPxToDevPxMatrix(this);
    }
  }
  if (!mCanvasTM) {
    NS_ASSERTION(mParent, "null parent");

    nsSVGContainerFrame *parent = static_cast<nsSVGContainerFrame*>(mParent);
    dom::SVGTextContentElement *content = static_cast<dom::SVGTextContentElement*>(mContent);

    gfxMatrix tm = content->PrependLocalTransformsTo(parent->GetCanvasTM(aFor));

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
nsSVGTextFrame2::ConvertTextElementCharIndexToAddressableIndex(
                                                           int32_t aIndex,
                                                           nsIContent* aContent)
{
  CharIterator it(this, CharIterator::eAddressable, aContent);
  if (!it.AdvanceToSubtree()) {
    return -1;
  }
  uint32_t result = 0;
  while (!it.AtEnd() &&
         it.IsWithinSubtree() &&
         it.TextElementCharIndex() < static_cast<uint32_t>(aIndex)) {
    result++;
    it.Next();
  }
  return result;
}





uint32_t
nsSVGTextFrame2::GetNumberOfChars(nsIContent* aContent)
{
  UpdateGlyphPositioning(false);

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
nsSVGTextFrame2::GetComputedTextLength(nsIContent* aContent)
{
  UpdateGlyphPositioning(false);

  float cssPxPerDevPx = PresContext()->
    AppUnitsToFloatCSSPixels(PresContext()->AppUnitsPerDevPixel());

  nscoord length = 0;
  TextRenderedRunIterator it(this, TextRenderedRunIterator::eAllFrames,
                             aContent);
  for (TextRenderedRun run = it.Current(); run.mFrame; run = it.Next()) {
    length += run.GetAdvanceWidth();
  }

  return PresContext()->AppUnitsToGfxUnits(length) *
           cssPxPerDevPx / mFontSizeScaleFactor;
}





float
nsSVGTextFrame2::GetSubStringLength(nsIContent* aContent,
                                    uint32_t charnum, uint32_t nchars)
{
  UpdateGlyphPositioning(false);

  
  
  CharIterator chit(this, CharIterator::eAddressable, aContent);
  if (!chit.AdvanceToSubtree() ||
      chit.AtEnd() ||
      !chit.Next(charnum) ||
      chit.IsAfterSubtree()) {
    return 0.0f;
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

  return presContext->AppUnitsToGfxUnits(textLength) *
           cssPxPerDevPx / mFontSizeScaleFactor;
}





int32_t
nsSVGTextFrame2::GetCharNumAtPosition(nsIContent* aContent,
                                      mozilla::nsISVGPoint* aPoint)
{
  UpdateGlyphPositioning(false);

  nsPresContext* context = PresContext();

  gfxPoint p(aPoint->X(), aPoint->Y());

  int32_t result = -1;

  TextRenderedRunIterator it(this, TextRenderedRunIterator::eAllFrames, aContent);
  for (TextRenderedRun run = it.Current(); run.mFrame; run = it.Next()) {
    
    int32_t index = run.GetCharNumAtPosition(context, p);
    if (index != -1) {
      result = index + run.mTextElementCharIndex - run.mTextFrameContentOffset;
    }
  }

  if (result == -1) {
    return result;
  }

  return ConvertTextElementCharIndexToAddressableIndex(result, aContent);
}





nsresult
nsSVGTextFrame2::GetStartPositionOfChar(nsIContent* aContent,
                                        uint32_t aCharNum,
                                        mozilla::nsISVGPoint** aResult)
{
  UpdateGlyphPositioning(false);

  CharIterator it(this, CharIterator::eAddressable, aContent);
  if (!it.AdvanceToSubtree() ||
      it.AtEnd() ||
      !it.Next(aCharNum)) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  
  uint32_t startIndex = it.GlyphStartTextElementCharIndex();

  NS_ADDREF(*aResult = new DOMSVGPoint(mPositions[startIndex].mPosition));
  return NS_OK;
}





nsresult
nsSVGTextFrame2::GetEndPositionOfChar(nsIContent* aContent,
                                      uint32_t aCharNum,
                                      mozilla::nsISVGPoint** aResult)
{
  UpdateGlyphPositioning(false);

  CharIterator it(this, CharIterator::eAddressable, aContent);
  if (!it.AdvanceToSubtree() ||
      it.AtEnd() ||
      !it.Next(aCharNum)) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  
  uint32_t startIndex = it.GlyphStartTextElementCharIndex();

  
  gfxFloat advance = it.GetGlyphAdvance(PresContext());
  if (it.TextRun()->IsRightToLeft()) {
    advance = -advance;
  }

  
  
  gfxMatrix m;
  m.Translate(mPositions[startIndex].mPosition);
  m.Rotate(mPositions[startIndex].mAngle);
  gfxPoint p = m.Transform(gfxPoint(advance / mFontSizeScaleFactor, 0));

  NS_ADDREF(*aResult = new DOMSVGPoint(p));
  return NS_OK;
}





nsresult
nsSVGTextFrame2::GetExtentOfChar(nsIContent* aContent,
                                 uint32_t aCharNum,
                                 dom::SVGIRect** aResult)
{
  UpdateGlyphPositioning(false);

  CharIterator it(this, CharIterator::eAddressable, aContent);
  if (!it.AdvanceToSubtree() ||
      it.AtEnd() ||
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

  gfxRect glyphRect
    (x, -presContext->AppUnitsToGfxUnits(ascent) * cssPxPerDevPx,
     advance, presContext->AppUnitsToGfxUnits(ascent + descent) * cssPxPerDevPx);

  
  gfxRect r = m.TransformBounds(glyphRect);

  NS_ADDREF(*aResult = new dom::SVGRect(r.x, r.y, r.width, r.height));
  return NS_OK;
}





nsresult
nsSVGTextFrame2::GetRotationOfChar(nsIContent* aContent,
                                   uint32_t aCharNum,
                                   float* aResult)
{
  UpdateGlyphPositioning(false);

  CharIterator it(this, CharIterator::eAddressable, aContent);
  if (!it.AdvanceToSubtree() ||
      it.AtEnd() ||
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

uint32_t
nsSVGTextFrame2::ResolvePositions(nsIContent* aContent,
                                  uint32_t aIndex,
                                  bool aInTextPath,
                                  bool& aForceStartOfChunk,
                                  nsTArray<gfxPoint>& aDeltas)
{
  if (aContent->IsNodeOfType(nsINode::eTEXT)) {
    
    uint32_t length = static_cast<nsTextNode*>(aContent)->TextLength();
    if (length) {
      if (aForceStartOfChunk) {
        
        mPositions[aIndex].mStartOfChunk = true;
        aForceStartOfChunk = false;
      }
      uint32_t end = aIndex + length;
      while (aIndex < end) {
        
        
        
        
        
        if (aInTextPath || ShouldStartRunAtIndex(mPositions, aDeltas, aIndex)) {
          mPositions[aIndex].mRunBoundary = true;
        }
        aIndex++;
      }
    }
    return aIndex;
  }

  
  if (!IsTextContentElement(aContent)) {
    return aIndex;
  }

  if (aContent->Tag() == nsGkAtoms::textPath) {
    
    
    if (HasTextContent(aContent)) {
      mPositions[aIndex].mPosition = gfxPoint();
    }
  } else if (aContent->Tag() != nsGkAtoms::a) {
    
    nsSVGElement* element = static_cast<nsSVGElement*>(aContent);

    
    SVGUserUnitList x, y, dx, dy;
    element->GetAnimatedLengthListValues(&x, &y, &dx, &dy);

    
    const SVGNumberList* rotate = nullptr;
    SVGAnimatedNumberList* animatedRotate =
      element->GetAnimatedNumberList(nsGkAtoms::rotate);
    if (animatedRotate) {
      rotate = &animatedRotate->GetAnimValue();
    }

    uint32_t count = GetTextContentLength(aContent);

    
    
    
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
          i++;
        }
      }
      for (uint32_t i = 0, j = 0; i < dy.Length() && j < count; j++) {
        if (!mPositions[aIndex + j].mUnaddressable) {
          aDeltas[aIndex + j].y = dy[i];
          i++;
        }
      }
    }

    
    for (uint32_t i = 0, j = 0; i < x.Length() && j < count; j++) {
      if (!mPositions[aIndex + j].mUnaddressable) {
        mPositions[aIndex + j].mPosition.x = x[i];
        i++;
      }
    }
    for (uint32_t i = 0, j = 0; i < y.Length() && j < count; j++) {
      if (!mPositions[aIndex + j].mUnaddressable) {
        mPositions[aIndex + j].mPosition.y = y[i];
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
  }

  
  bool inTextPath = aInTextPath || aContent->Tag() == nsGkAtoms::textPath;
  for (nsIContent* child = aContent->GetFirstChild();
       child;
       child = child->GetNextSibling()) {
    aIndex = ResolvePositions(child, aIndex, inTextPath, aForceStartOfChunk,
                              aDeltas);
  }

  if (aContent->Tag() == nsGkAtoms::textPath) {
    
    aForceStartOfChunk = true;
  }

  return aIndex;
}

bool
nsSVGTextFrame2::ResolvePositions(nsTArray<gfxPoint>& aDeltas)
{
  NS_ASSERTION(mPositions.IsEmpty(), "expected mPositions to be empty");

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
  return ResolvePositions(mContent, 0, false, forceStartOfChunk, aDeltas) != 0;
}

void
nsSVGTextFrame2::DetermineCharPositions(nsTArray<nsPoint>& aPositions)
{
  NS_ASSERTION(aPositions.IsEmpty(), "expected aPositions to be empty");

  nsPoint position, lastPosition;

  float cssPxPerDevPx = PresContext()->
    AppUnitsToFloatCSSPixels(PresContext()->AppUnitsPerDevPixel());

  TextFrameIterator frit(this);
  for (nsTextFrame* frame = frit.Current(); frame; frame = frit.Next()) {
    gfxSkipCharsIterator it = frame->EnsureTextRun(nsTextFrame::eInflated);
    gfxTextRun* textRun = frame->GetTextRun(nsTextFrame::eInflated);

    
    position = frit.Position();
    if (textRun->IsRightToLeft()) {
      position.x += frame->GetRect().width;
    }
    position.y += GetBaselinePosition(frame, textRun, frit.DominantBaseline());
    position = nsPoint(nscoord(position.x * cssPxPerDevPx),
                       nscoord(position.y * cssPxPerDevPx));

    
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
      gfxFloat advance =
        textRun->GetAdvanceWidth(it.GetSkippedOffset(), 1,
                                 nullptr) * cssPxPerDevPx;
      position.x += textRun->IsRightToLeft() ? -advance : advance;
      aPositions.AppendElement(lastPosition);
      it.AdvanceOriginal(1);
    }

    
    while (it.GetOriginalOffset() < frame->GetContentEnd()) {
      aPositions.AppendElement(position);
      if (!it.IsOriginalCharSkipped() &&
          textRun->IsLigatureGroupStart(it.GetSkippedOffset()) &&
          textRun->IsClusterStart(it.GetSkippedOffset())) {
        
        uint32_t length = ClusterLength(textRun, it);
        gfxFloat advance =
          textRun->GetAdvanceWidth(it.GetSkippedOffset(), length,
                                   nullptr) * cssPxPerDevPx;
        position.x += textRun->IsRightToLeft() ? -advance : advance;
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
                   gfxFloat aLeftEdge,
                   gfxFloat aRightEdge,
                   TextAnchorSide aAnchorSide)
{
  NS_ASSERTION(aLeftEdge <= aRightEdge, "unexpected anchored chunk edges");
  NS_ASSERTION(aChunkStart < aChunkEnd, "unexpected values for aChunkStart and "
                                        "aChunkEnd");

  gfxFloat shift = aCharPositions[aChunkStart].mPosition.x;
  switch (aAnchorSide) {
    case eAnchorLeft:
      shift -= aLeftEdge;
      break;
    case eAnchorMiddle:
      shift -= (aLeftEdge + aRightEdge) / 2;
      break;
    case eAnchorRight:
      shift -= aRightEdge;
      break;
    default:
      NS_NOTREACHED("unexpected value for aAnchorSide");
  }

  if (shift != 0.0) {
    for (uint32_t i = aChunkStart; i < aChunkEnd; i++) {
      aCharPositions[i].mPosition.x += shift;
    }
  }
}

void
nsSVGTextFrame2::AdjustChunksForLineBreaks()
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
nsSVGTextFrame2::AdjustPositionsForClusters()
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

    
    
    gfxFloat advance =
      it.GetGlyphPartialAdvance(0, charIndex - startIndex, presContext) /
        mFontSizeScaleFactor;
    gfxPoint direction = gfxPoint(cos(angle), sin(angle)) *
                         (it.TextRun()->IsRightToLeft() ? -1.0 : 1.0);
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

nsIFrame*
nsSVGTextFrame2::GetTextPathPathFrame(nsIFrame* aTextPathFrame)
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

  return property->GetReferencedFrame(nsGkAtoms::svgPathGeometryFrame, nullptr);
}

already_AddRefed<gfxFlattenedPath>
nsSVGTextFrame2::GetFlattenedTextPath(nsIFrame* aTextPathFrame)
{
  nsIFrame *path = GetTextPathPathFrame(aTextPathFrame);

  if (path) {
    nsSVGPathGeometryElement *element =
      static_cast<nsSVGPathGeometryElement*>(path->GetContent());

    return element->GetFlattenedPath(element->PrependLocalTransformsTo(gfxMatrix()));
  }
  return nullptr;
}

gfxFloat
nsSVGTextFrame2::GetOffsetScale(nsIFrame* aTextPathFrame)
{
  nsIFrame *pathFrame = GetTextPathPathFrame(aTextPathFrame);
  if (!pathFrame)
    return 1.0;

  return static_cast<dom::SVGPathElement*>(pathFrame->GetContent())->
    GetPathLengthScale(dom::SVGPathElement::eForTextPath);
}

gfxFloat
nsSVGTextFrame2::GetStartOffset(nsIFrame* aTextPathFrame)
{
  dom::SVGTextPathElement *tp =
    static_cast<dom::SVGTextPathElement*>(aTextPathFrame->GetContent());
  nsSVGLength2 *length =
    &tp->mLengthAttributes[dom::SVGTextPathElement::STARTOFFSET];

  if (length->IsPercentage()) {
    nsRefPtr<gfxFlattenedPath> data = GetFlattenedTextPath(aTextPathFrame);
    return data ?
             length->GetAnimValInSpecifiedUnits() * data->GetLength() / 100.0 :
             0.0;
  }
  return length->GetAnimValue(tp) * GetOffsetScale(aTextPathFrame);
}

void
nsSVGTextFrame2::DoTextPathLayout()
{
  nsPresContext* context = PresContext();

  CharIterator it(this, CharIterator::eClusterAndLigatureGroupStart);
  while (!it.AtEnd()) {
    nsIFrame* textPathFrame = it.TextPathFrame();
    if (!textPathFrame) {
      
      it.AdvancePastCurrentFrame();
      continue;
    }

    
    nsRefPtr<gfxFlattenedPath> data = GetFlattenedTextPath(textPathFrame);
    if (!data) {
      it.AdvancePastCurrentTextPathFrame();
      continue;
    }

    nsIContent* textPath = textPathFrame->GetContent();

    gfxFloat offset = GetStartOffset(textPathFrame);
    gfxFloat pathLength = data->GetLength();

    
    do {
      uint32_t i = it.TextElementCharIndex();
      gfxFloat halfAdvance =
        it.GetGlyphAdvance(context) / mFontSizeScaleFactor / 2.0;
      gfxFloat sign = it.TextRun()->IsRightToLeft() ? -1.0 : 1.0;
      gfxFloat midx = mPositions[i].mPosition.x + sign * halfAdvance + offset;

      
      mPositions[i].mHidden = midx < 0 || midx > pathLength;

      
      double angle;
      gfxPoint pt =
        data->FindPoint(gfxPoint(midx, mPositions[i].mPosition.y), &angle);
      gfxPoint direction = gfxPoint(cos(angle), sin(angle)) * sign;
      mPositions[i].mPosition = pt - direction * halfAdvance;
      mPositions[i].mAngle += angle;

      
      for (uint32_t j = i + 1;
           j < mPositions.Length() && mPositions[j].mClusterOrLigatureGroupMiddle;
           j++) {
        gfxPoint partialAdvance =
          direction * it.GetGlyphPartialAdvance(0, j - i, context) /
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
nsSVGTextFrame2::DoAnchoring()
{
  nsPresContext* presContext = PresContext();

  CharIterator it(this, CharIterator::eOriginal);

  
  while (!it.AtEnd() &&
         (it.IsOriginalCharSkipped() || it.IsOriginalCharTrimmed())) {
    it.Next();
  }

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
        if (it.TextRun()->IsRightToLeft()) {
          left  = std::min(left,  mPositions[index].mPosition.x - advance);
          right = std::max(right, mPositions[index].mPosition.x);
        } else {
          left  = std::min(left,  mPositions[index].mPosition.x);
          right = std::max(right, mPositions[index].mPosition.x + advance);
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

      ShiftAnchoredChunk(mPositions, start, end, left, right, anchor);
    }

    start = it.TextElementCharIndex();
  }
}

void
nsSVGTextFrame2::DoGlyphPositioning()
{
  mPositions.Clear();
  mPositioningDirty = false;

  
  nsTArray<nsPoint> charPositions;
  DetermineCharPositions(charPositions);

  if (charPositions.IsEmpty()) {
    
    return;
  }

  nsPresContext* presContext = PresContext();

  
  nsTArray<gfxPoint> deltas;
  if (!ResolvePositions(deltas)) {
    
    
    
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

  
  
  if (!deltas.IsEmpty()) {
    mPositions[0].mPosition += deltas[0];
  }
  for (uint32_t i = 1; i < mPositions.Length(); i++) {
    
    if (!mPositions[i].IsXSpecified()) {
      nscoord d = charPositions[i].x - charPositions[i - 1].x;
      mPositions[i].mPosition.x =
        mPositions[i - 1].mPosition.x +
        presContext->AppUnitsToGfxUnits(d) / mFontSizeScaleFactor;
    }
    
    if (!mPositions[i].IsYSpecified()) {
      nscoord d = charPositions[i].y - charPositions[i - 1].y;
      mPositions[i].mPosition.y =
        mPositions[i - 1].mPosition.y +
        presContext->AppUnitsToGfxUnits(d) / mFontSizeScaleFactor;
    }
    
    if (i < deltas.Length()) {
      mPositions[i].mPosition += deltas[i];
    }
    
    if (!mPositions[i].IsAngleSpecified()) {
      mPositions[i].mAngle = mPositions[i - 1].mAngle;
      if (mPositions[i].mAngle != 0.0f) {
        
        mPositions[i].mRunBoundary = true;
      }
    }
  }

  
  
  
  
  
  for (uint32_t i = mPositions.Length(); i < charPositions.Length(); i++) {
    nscoord dx = charPositions[i].x - charPositions[i - 1].x;
    nscoord dy = charPositions[i].y - charPositions[i - 1].y;

    gfxPoint pt(mPositions[i - 1].mPosition.x +
                  presContext->AppUnitsToGfxUnits(dx),
                mPositions[i - 1].mPosition.y +
                  presContext->AppUnitsToGfxUnits(dy));

    mPositions.AppendElement(CharPosition(pt / mFontSizeScaleFactor,
                                          mPositions[i - 1].mAngle));
    if (i < deltas.Length()) {
      mPositions[i].mPosition += deltas[i];
    }
  }

  AdjustChunksForLineBreaks();
  AdjustPositionsForClusters();
  DoAnchoring();
  DoTextPathLayout();
}

bool
nsSVGTextFrame2::ShouldRenderAsPath(nsRenderingContext* aContext,
                                    nsTextFrame* aFrame,
                                    bool& aShouldPaintSVGGlyphs)
{
  
  if (SVGAutoRenderState::GetRenderMode(aContext) != SVGAutoRenderState::NORMAL) {
    aShouldPaintSVGGlyphs = false;
    return true;
  }

  aShouldPaintSVGGlyphs = true;

  const nsStyleSVG* style = aFrame->StyleSVG();

  
  
  if (!(style->mFill.mType == eStyleSVGPaintType_None ||
        (style->mFill.mType == eStyleSVGPaintType_Color &&
         style->mFillRule == NS_STYLE_FILL_RULE_NONZERO &&
         style->mFillOpacity == 1))) {
    return true;
  }

  
  if (!(style->mStroke.mType == eStyleSVGPaintType_None ||
        style->mStrokeOpacity == 0 ||
        nsSVGUtils::CoordToFloat(PresContext(),
                                 static_cast<nsSVGElement*>(mContent),
                                 style->mStrokeWidth) == 0)) {
    return true;
  }

  return false;
}

void
nsSVGTextFrame2::NotifyGlyphMetricsChange()
{
  
  
  
  
  
  
  
  
  
  
  
  
  if (mGlyphMetricsUpdater) {
    return;
  }
  mGlyphMetricsUpdater = new GlyphMetricsUpdater(this);
  nsContentUtils::AddScriptRunner(mGlyphMetricsUpdater.get());
}

void
nsSVGTextFrame2::UpdateGlyphPositioning(bool aForceGlobalTransform)
{
  nsIFrame* kid = GetFirstPrincipalChild();
  if (!kid)
    return;

  bool needsReflow =
    (mState & (NS_FRAME_IS_DIRTY | NS_FRAME_HAS_DIRTY_CHILDREN));

  NS_ASSERTION(!(kid->GetStateBits() & NS_FRAME_IN_REFLOW),
               "should not be in reflow when about to reflow again");

  if (!needsReflow)
    return;

  if (mState & NS_FRAME_IS_DIRTY) {
    
    kid->AddStateBits(NS_FRAME_IS_DIRTY);
  }

  if (needsReflow) {
    nsPresContext::InterruptPreventer noInterrupts(PresContext());
    DoReflow(aForceGlobalTransform);
  }

  DoGlyphPositioning();
}

void
nsSVGTextFrame2::DoReflow(bool aForceGlobalTransform)
{
  mPositioningDirty = true;

  nsPresContext *presContext = PresContext();
  nsIFrame* kid = GetFirstPrincipalChild();
  if (!kid)
    return;

  nsIPresShell* presShell = presContext->PresShell();
  NS_ASSERTION(presShell, "null presShell");
  nsRefPtr<nsRenderingContext> renderingContext =
    presShell->GetReferenceRenderingContext();
  if (!renderingContext)
    return;

  UpdateFontSizeScaleFactor(aForceGlobalTransform);

  nscoord width = kid->GetPrefWidth(renderingContext);
  nsHTMLReflowState reflowState(presContext, kid,
                                renderingContext,
                                nsSize(width, NS_UNCONSTRAINEDSIZE));
  nsHTMLReflowMetrics desiredSize;
  nsReflowStatus status;

  NS_ASSERTION(reflowState.mComputedBorderPadding == nsMargin(0, 0, 0, 0) &&
               reflowState.mComputedMargin == nsMargin(0, 0, 0, 0),
               "style system should ensure that :-moz-svg-text "
               "does not get styled");

  kid->WillReflow(presContext);
  kid->Reflow(presContext, desiredSize, reflowState, status);
  kid->DidReflow(presContext, &reflowState, nsDidReflowStatus::FINISHED);
  kid->SetSize(nsSize(desiredSize.width, desiredSize.height));

  TextNodeCorrespondenceRecorder::RecordCorrespondence(this);
}


#define CLAMP_MIN_SIZE 8.0
#define CLAMP_MAX_SIZE 200.0
#define PRECISE_SIZE   200.0

void
nsSVGTextFrame2::UpdateFontSizeScaleFactor(bool aForceGlobalTransform)
{
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
    return;
  }

  gfxMatrix m;
  if (aForceGlobalTransform ||
      !(GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD)) {
    m = GetCanvasTM(mGetCanvasTMForFlag);
    if (m.IsSingular()) {
      mFontSizeScaleFactor = 1.0;
      return;
    }
  }

  float textZoom = presContext->TextZoom();
  double minSize = presContext->AppUnitsToFloatCSSPixels(min) / textZoom;

  if (geometricPrecision) {
    
    mFontSizeScaleFactor = PRECISE_SIZE / minSize;
    return;
  }

  double maxSize = presContext->AppUnitsToFloatCSSPixels(max) / textZoom;

  
  
  
  gfxPoint p = m.Transform(gfxPoint(1, 1)) - m.Transform(gfxPoint(0, 0));
  double contextScale = SVGContentUtils::ComputeNormalizedHypotenuse(p.x, p.y);

  double minTextRunSize = minSize * contextScale;
  double maxTextRunSize = maxSize * contextScale;

  if (minTextRunSize >= CLAMP_MIN_SIZE &&
      maxTextRunSize <= CLAMP_MAX_SIZE) {
    
    
    mFontSizeScaleFactor = contextScale;
    return;
  }

  if (maxSize / minSize > CLAMP_MAX_SIZE / CLAMP_MIN_SIZE) {
    
    
    
    mFontSizeScaleFactor = CLAMP_MIN_SIZE / minTextRunSize;
    return;
  }

  if (minTextRunSize < CLAMP_MIN_SIZE) {
    mFontSizeScaleFactor = CLAMP_MIN_SIZE / minTextRunSize;
    return;
  }

  mFontSizeScaleFactor = CLAMP_MAX_SIZE / maxTextRunSize;
}

double
nsSVGTextFrame2::GetFontSizeScaleFactor() const
{
  return mFontSizeScaleFactor;
}






gfxPoint
nsSVGTextFrame2::TransformFramePointToTextChild(const gfxPoint& aPoint,
                                                nsIFrame* aChildFrame)
{
  NS_ASSERTION(aChildFrame &&
               nsLayoutUtils::GetClosestFrameOfType
                 (aChildFrame->GetParent(), nsGkAtoms::svgTextFrame2) == this,
               "aChildFrame must be a descendant of this frame");

  UpdateGlyphPositioning(true);

  nsPresContext* presContext = PresContext();

  
  
  
  float cssPxPerDevPx = presContext->
    AppUnitsToFloatCSSPixels(presContext->AppUnitsPerDevPixel());
  float factor = presContext->AppUnitsPerCSSPixel();
  gfxPoint framePosition(NSAppUnitsToFloatPixels(mRect.x, factor),
                         NSAppUnitsToFloatPixels(mRect.y, factor));
  gfxPoint pointInUserSpace = aPoint * cssPxPerDevPx + framePosition;

  
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
    gfxRect runRect = run.GetRunUserSpaceRect(presContext, flags);

    gfxPoint pointInRunUserSpace =
      run.GetTransformFromRunUserSpaceToUserSpace(presContext).Invert().
          Transform(pointInUserSpace);

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
  return m.Transform(pointInRun) / cssPxPerDevPx;
}








gfxRect
nsSVGTextFrame2::TransformFrameRectToTextChild(const gfxRect& aRect,
                                               nsIFrame* aChildFrame)
{
  NS_ASSERTION(aChildFrame &&
               nsLayoutUtils::GetClosestFrameOfType
                 (aChildFrame->GetParent(), nsGkAtoms::svgTextFrame2) == this,
               "aChildFrame must be a descendant of this frame");

  UpdateGlyphPositioning(true);

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
    
    gfxMatrix m;
    m.PreMultiply(run.GetTransformFromRunUserSpaceToUserSpace(presContext).Invert());
    m.PreMultiply(run.GetTransformFromRunUserSpaceToFrameUserSpace(presContext));
    gfxRect incomingRectInFrameUserSpace =
      m.TransformBounds(incomingRectInUserSpace);

    
    uint32_t flags = TextRenderedRun::eIncludeFill |
                     TextRenderedRun::eIncludeStroke;
    gfxRect runRectInFrameUserSpace = run.GetFrameUserSpaceRect(presContext, flags);
    gfxRect runIntersectionInFrameUserSpace =
      incomingRectInFrameUserSpace.Intersect(runRectInFrameUserSpace);

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
nsSVGTextFrame2::TransformFrameRectFromTextChild(const nsRect& aRect,
                                                 nsIFrame* aChildFrame)
{
  NS_ASSERTION(aChildFrame &&
               nsLayoutUtils::GetClosestFrameOfType
                 (aChildFrame->GetParent(), nsGkAtoms::svgTextFrame2) == this,
               "aChildFrame must be a descendant of this frame");

  UpdateGlyphPositioning(true);

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
      (rectInFrameUserSpace, run.GetFrameUserSpaceRect(presContext, flags));

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

gfxFont::DrawMode
nsSVGTextFrame2::SetupCairoState(gfxContext* aContext,
                                 nsIFrame* aFrame,
                                 gfxTextObjectPaint* aOuterObjectPaint,
                                 gfxTextObjectPaint** aThisObjectPaint)
{
  gfxFont::DrawMode toDraw = gfxFont::DrawMode(0);
  SVGTextObjectPaint *thisObjectPaint = new SVGTextObjectPaint();

  if (SetupCairoStroke(aContext, aFrame, aOuterObjectPaint, thisObjectPaint)) {
    toDraw = gfxFont::DrawMode(toDraw | gfxFont::GLYPH_STROKE);
  }

  if (SetupCairoFill(aContext, aFrame, aOuterObjectPaint, thisObjectPaint)) {
    toDraw = gfxFont::DrawMode(toDraw | gfxFont::GLYPH_FILL);
  }

  *aThisObjectPaint = thisObjectPaint;

  return toDraw;
}

bool
nsSVGTextFrame2::SetupCairoStroke(gfxContext* aContext,
                                  nsIFrame* aFrame,
                                  gfxTextObjectPaint* aOuterObjectPaint,
                                  SVGTextObjectPaint* aThisObjectPaint)
{
  const nsStyleSVG *style = aFrame->StyleSVG();
  if (style->mStroke.mType == eStyleSVGPaintType_None) {
    aThisObjectPaint->SetStrokeOpacity(0.0f);
    return false;
  }

  gfxContextMatrixAutoSaveRestore matrixRestore(aContext);
  aContext->IdentityMatrix();

  nsSVGUtils::SetupCairoStrokeHitGeometry(aFrame, aContext, aOuterObjectPaint);
  float opacity = nsSVGUtils::GetOpacity(style->mStrokeOpacitySource,
                                         style->mStrokeOpacity,
                                         aOuterObjectPaint);

  SetupInheritablePaint(aContext, aFrame, opacity, aOuterObjectPaint,
                        aThisObjectPaint->mStrokePaint, &nsStyleSVG::mStroke,
                        nsSVGEffects::StrokeProperty());

  aThisObjectPaint->SetStrokeOpacity(opacity);

  return opacity != 0.0f;
}

bool
nsSVGTextFrame2::SetupCairoFill(gfxContext* aContext,
                                nsIFrame* aFrame,
                                gfxTextObjectPaint* aOuterObjectPaint,
                                SVGTextObjectPaint* aThisObjectPaint)
{
  const nsStyleSVG *style = aFrame->StyleSVG();
  if (style->mFill.mType == eStyleSVGPaintType_None) {
    aThisObjectPaint->SetFillOpacity(0.0f);
    return false;
  }

  float opacity = nsSVGUtils::GetOpacity(style->mFillOpacitySource,
                                         style->mFillOpacity,
                                         aOuterObjectPaint);

  SetupInheritablePaint(aContext, aFrame, opacity, aOuterObjectPaint,
                        aThisObjectPaint->mFillPaint, &nsStyleSVG::mFill,
                        nsSVGEffects::FillProperty());

  aThisObjectPaint->SetFillOpacity(opacity);

  return true;
}

void
nsSVGTextFrame2::SetupInheritablePaint(gfxContext* aContext,
                                       nsIFrame* aFrame,
                                       float& aOpacity,
                                       gfxTextObjectPaint* aOuterObjectPaint,
                                       SVGTextObjectPaint::Paint& aTargetPaint,
                                       nsStyleSVGPaint nsStyleSVG::*aFillOrStroke,
                                       const FramePropertyDescriptor* aProperty)
{
  const nsStyleSVG *style = aFrame->StyleSVG();
  nsSVGPaintServerFrame *ps =
    nsSVGEffects::GetPaintServer(aFrame, &(style->*aFillOrStroke), aProperty);

  if (ps && ps->SetupPaintServer(aContext, aFrame, aFillOrStroke, aOpacity)) {
    aTargetPaint.SetPaintServer(aFrame, aContext->CurrentMatrix(), ps);
  } else if (SetupObjectPaint(aContext, aFrame, aFillOrStroke, aOpacity, aOuterObjectPaint)) {
    aTargetPaint.SetObjectPaint(aOuterObjectPaint, (style->*aFillOrStroke).mType);
  } else {
    nscolor color = nsSVGUtils::GetFallbackOrPaintColor(aContext,
                                                        aFrame->StyleContext(),
                                                        aFillOrStroke);
    aTargetPaint.SetColor(color);

    aContext->SetPattern(new gfxPattern(gfxRGBA(NS_GET_R(color) / 255.0,
                                                NS_GET_G(color) / 255.0,
                                                NS_GET_B(color) / 255.0,
                                                NS_GET_A(color) / 255.0 * aOpacity)));
  }
}

bool
nsSVGTextFrame2::SetupObjectPaint(gfxContext* aContext,
                                  nsIFrame* aFrame,
                                  nsStyleSVGPaint nsStyleSVG::*aFillOrStroke,
                                  float& aOpacity,
                                  gfxTextObjectPaint* aOuterObjectPaint)
{
  if (!aOuterObjectPaint) {
    return false;
  }

  const nsStyleSVG *style = aFrame->StyleSVG();
  const nsStyleSVGPaint &paint = style->*aFillOrStroke;

  if (paint.mType != eStyleSVGPaintType_ObjectFill &&
      paint.mType != eStyleSVGPaintType_ObjectStroke) {
    return false;
  }

  gfxMatrix current = aContext->CurrentMatrix();
  nsRefPtr<gfxPattern> pattern =
    paint.mType == eStyleSVGPaintType_ObjectFill ?
      aOuterObjectPaint->GetFillPattern(aOpacity, current) :
      aOuterObjectPaint->GetStrokePattern(aOpacity, current);
  if (!pattern) {
    return false;
  }

  aContext->SetPattern(pattern);
  return true;
}
