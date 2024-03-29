





#include "TextOverflow.h"
#include <algorithm>


#include "nsBlockFrame.h"
#include "nsCaret.h"
#include "nsContentUtils.h"
#include "nsCSSAnonBoxes.h"
#include "nsFontMetrics.h"
#include "nsGfxScrollFrame.h"
#include "nsIScrollableFrame.h"
#include "nsLayoutUtils.h"
#include "nsPresContext.h"
#include "nsRect.h"
#include "nsRenderingContext.h"
#include "nsTextFrame.h"
#include "nsIFrameInlines.h"
#include "mozilla/ArrayUtils.h"
#include "mozilla/Likely.h"
#include "nsISelection.h"

namespace mozilla {
namespace css {

class LazyReferenceRenderingContextGetterFromFrame final :
    public gfxFontGroup::LazyReferenceContextGetter {
public:
  explicit LazyReferenceRenderingContextGetterFromFrame(nsIFrame* aFrame)
    : mFrame(aFrame) {}
  virtual already_AddRefed<gfxContext> GetRefContext() override
  {
    return mFrame->PresContext()->PresShell()->CreateReferenceRenderingContext();
  }
private:
  nsIFrame* mFrame;
};

static gfxTextRun*
GetEllipsisTextRun(nsIFrame* aFrame)
{
  nsRefPtr<nsFontMetrics> fm;
  nsLayoutUtils::GetFontMetricsForFrame(aFrame, getter_AddRefs(fm),
    nsLayoutUtils::FontSizeInflationFor(aFrame));
  LazyReferenceRenderingContextGetterFromFrame lazyRefContextGetter(aFrame);
  return fm->GetThebesFontGroup()->GetEllipsisTextRun(
    aFrame->PresContext()->AppUnitsPerDevPixel(),
    nsLayoutUtils::GetTextRunOrientFlagsForStyle(aFrame->StyleContext()),
    lazyRefContextGetter);
}

static nsIFrame*
GetSelfOrNearestBlock(nsIFrame* aFrame)
{
  return nsLayoutUtils::GetAsBlock(aFrame) ? aFrame :
         nsLayoutUtils::FindNearestBlockAncestor(aFrame);
}




static bool
IsAtomicElement(nsIFrame* aFrame, const nsIAtom* aFrameType)
{
  NS_PRECONDITION(!nsLayoutUtils::GetAsBlock(aFrame) ||
                  !aFrame->IsBlockOutside(),
                  "unexpected block frame");
  NS_PRECONDITION(aFrameType != nsGkAtoms::placeholderFrame,
                  "unexpected placeholder frame");
  return !aFrame->IsFrameOfType(nsIFrame::eLineParticipant);
}

static bool
IsFullyClipped(nsTextFrame* aFrame, nscoord aLeft, nscoord aRight,
               nscoord* aSnappedLeft, nscoord* aSnappedRight)
{
  *aSnappedLeft = aLeft;
  *aSnappedRight = aRight;
  if (aLeft <= 0 && aRight <= 0) {
    return false;
  }
  return !aFrame->MeasureCharClippedText(aLeft, aRight,
                                         aSnappedLeft, aSnappedRight);
}

static bool
IsInlineAxisOverflowVisible(nsIFrame* aFrame)
{
  NS_PRECONDITION(nsLayoutUtils::GetAsBlock(aFrame) != nullptr,
                  "expected a block frame");

  nsIFrame* f = aFrame;
  while (f && f->StyleContext()->GetPseudo() &&
         f->GetType() != nsGkAtoms::scrollFrame) {
    f = f->GetParent();
  }
  if (!f) {
    return true;
  }
  auto overflow = aFrame->GetWritingMode().IsVertical() ?
    f->StyleDisplay()->mOverflowY : f->StyleDisplay()->mOverflowX;
  return overflow == NS_STYLE_OVERFLOW_VISIBLE;
}

static void
ClipMarker(const nsRect&                          aContentArea,
           const nsRect&                          aMarkerRect,
           DisplayListClipState::AutoSaveRestore& aClipState)
{
  nscoord rightOverflow = aMarkerRect.XMost() - aContentArea.XMost();
  nsRect markerRect = aMarkerRect;
  if (rightOverflow > 0) {
    
    markerRect.width -= rightOverflow;
    aClipState.ClipContentDescendants(markerRect);
  } else {
    nscoord leftOverflow = aContentArea.x - aMarkerRect.x;
    if (leftOverflow > 0) {
      
      markerRect.width -= leftOverflow;
      markerRect.x += leftOverflow;
      aClipState.ClipContentDescendants(markerRect);
    }
  }
}

static void
InflateIStart(WritingMode aWM, LogicalRect* aRect, nscoord aDelta)
{
  nscoord iend = aRect->IEnd(aWM);
  aRect->IStart(aWM) -= aDelta;
  aRect->ISize(aWM) = std::max(iend - aRect->IStart(aWM), 0);
}

static void
InflateIEnd(WritingMode aWM, LogicalRect* aRect, nscoord aDelta)
{
  aRect->ISize(aWM) = std::max(aRect->ISize(aWM) + aDelta, 0);
}

static bool
IsFrameDescendantOfAny(nsIFrame* aChild,
                       const TextOverflow::FrameHashtable& aSetOfFrames,
                       nsIFrame* aCommonAncestor)
{
  for (nsIFrame* f = aChild; f && f != aCommonAncestor;
       f = nsLayoutUtils::GetCrossDocParentFrame(f)) {
    if (aSetOfFrames.GetEntry(f)) {
      return true;
    }
  }
  return false;
}

class nsDisplayTextOverflowMarker : public nsDisplayItem
{
public:
  nsDisplayTextOverflowMarker(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                              const nsRect& aRect, nscoord aAscent,
                              const nsStyleTextOverflowSide* aStyle,
                              uint32_t aIndex)
    : nsDisplayItem(aBuilder, aFrame), mRect(aRect),
      mStyle(aStyle), mAscent(aAscent), mIndex(aIndex) {
    MOZ_COUNT_CTOR(nsDisplayTextOverflowMarker);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayTextOverflowMarker() {
    MOZ_COUNT_DTOR(nsDisplayTextOverflowMarker);
  }
#endif
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder,
                           bool* aSnap) override {
    *aSnap = false;
    nsRect shadowRect =
      nsLayoutUtils::GetTextShadowRectsUnion(mRect, mFrame);
    return mRect.Union(shadowRect);
  }
  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx) override;

  virtual uint32_t GetPerFrameKey() override { 
    return (mIndex << nsDisplayItem::TYPE_BITS) | nsDisplayItem::GetPerFrameKey(); 
  }
  void PaintTextToContext(nsRenderingContext* aCtx,
                          nsPoint aOffsetFromRect);
  NS_DISPLAY_DECL_NAME("TextOverflow", TYPE_TEXT_OVERFLOW)
private:
  nsRect          mRect;   
  const nsStyleTextOverflowSide* mStyle;
  nscoord         mAscent; 
  uint32_t        mIndex;
};

static void
PaintTextShadowCallback(nsRenderingContext* aCtx,
                        nsPoint aShadowOffset,
                        const nscolor& aShadowColor,
                        void* aData)
{
  reinterpret_cast<nsDisplayTextOverflowMarker*>(aData)->
           PaintTextToContext(aCtx, aShadowOffset);
}

void
nsDisplayTextOverflowMarker::Paint(nsDisplayListBuilder* aBuilder,
                                   nsRenderingContext*   aCtx)
{
  nscolor foregroundColor =
    nsLayoutUtils::GetColor(mFrame, eCSSProperty_color);

  
  nsLayoutUtils::PaintTextShadow(mFrame, aCtx, mRect, mVisibleRect,
                                 foregroundColor, PaintTextShadowCallback,
                                 (void*)this);
  aCtx->ThebesContext()->SetColor(foregroundColor);
  PaintTextToContext(aCtx, nsPoint(0, 0));
}

void
nsDisplayTextOverflowMarker::PaintTextToContext(nsRenderingContext* aCtx,
                                                nsPoint aOffsetFromRect)
{
  WritingMode wm = mFrame->GetWritingMode();
  nsPoint pt(mRect.x, mRect.y);
  if (wm.IsVertical()) {
    if (wm.IsVerticalLR()) {
      pt.x = NSToCoordFloor(nsLayoutUtils::GetSnappedBaselineX(
        mFrame, aCtx->ThebesContext(), pt.x, mAscent));
    } else {
      pt.x = NSToCoordFloor(nsLayoutUtils::GetSnappedBaselineX(
        mFrame, aCtx->ThebesContext(), pt.x + mRect.width, -mAscent));
    }
  } else {
    pt.y = NSToCoordFloor(nsLayoutUtils::GetSnappedBaselineY(
      mFrame, aCtx->ThebesContext(), pt.y, mAscent));
  }
  pt += aOffsetFromRect;

  if (mStyle->mType == NS_STYLE_TEXT_OVERFLOW_ELLIPSIS) {
    gfxTextRun* textRun = GetEllipsisTextRun(mFrame);
    if (textRun) {
      NS_ASSERTION(!textRun->IsRightToLeft(),
                   "Ellipsis textruns should always be LTR!");
      gfxPoint gfxPt(pt.x, pt.y);
      textRun->Draw(aCtx->ThebesContext(), gfxPt, DrawMode::GLYPH_FILL,
                    0, textRun->GetLength(), nullptr, nullptr, nullptr);
    }
  } else {
    nsRefPtr<nsFontMetrics> fm;
    nsLayoutUtils::GetFontMetricsForFrame(mFrame, getter_AddRefs(fm),
      nsLayoutUtils::FontSizeInflationFor(mFrame));
    nsLayoutUtils::DrawString(mFrame, *fm, aCtx, mStyle->mString.get(),
                              mStyle->mString.Length(), pt);
  }
}

TextOverflow::TextOverflow(nsDisplayListBuilder* aBuilder,
                           nsIFrame* aBlockFrame)
  : mContentArea(aBlockFrame->GetWritingMode(),
                 aBlockFrame->GetContentRectRelativeToSelf(),
                 aBlockFrame->GetSize())
  , mBuilder(aBuilder)
  , mBlock(aBlockFrame)
  , mScrollableFrame(nsLayoutUtils::GetScrollableFrameFor(aBlockFrame))
  , mBlockSize(aBlockFrame->GetSize())
  , mBlockWM(aBlockFrame->GetWritingMode())
  , mAdjustForPixelSnapping(false)
{
#ifdef MOZ_XUL
  if (!mScrollableFrame) {
    nsIAtom* pseudoType = aBlockFrame->StyleContext()->GetPseudo();
    if (pseudoType == nsCSSAnonBoxes::mozXULAnonymousBlock) {
      mScrollableFrame =
        nsLayoutUtils::GetScrollableFrameFor(aBlockFrame->GetParent());
      
      
      
      
      mAdjustForPixelSnapping = !mBlockWM.IsBidiLTR();
    }
  }
#endif
  mCanHaveInlineAxisScrollbar = false;
  if (mScrollableFrame) {
    auto scrollbarStyle = mBlockWM.IsVertical() ?
      mScrollableFrame->GetScrollbarStyles().mVertical :
      mScrollableFrame->GetScrollbarStyles().mHorizontal;
    mCanHaveInlineAxisScrollbar = scrollbarStyle != NS_STYLE_OVERFLOW_HIDDEN;
    if (!mAdjustForPixelSnapping) {
      
      
      mAdjustForPixelSnapping = mCanHaveInlineAxisScrollbar;
    }
    
    const nsSize nullContainerSize;
    mContentArea.MoveBy(mBlockWM,
                        LogicalPoint(mBlockWM,
                                     mScrollableFrame->GetScrollPosition(),
                                     nullContainerSize));
    nsIFrame* scrollFrame = do_QueryFrame(mScrollableFrame);
    scrollFrame->AddStateBits(NS_SCROLLFRAME_INVALIDATE_CONTENTS_ON_SCROLL);
  }
  uint8_t direction = aBlockFrame->StyleVisibility()->mDirection;
  const nsStyleTextReset* style = aBlockFrame->StyleTextReset();
  if (mBlockWM.IsBidiLTR()) {
    mIStart.Init(style->mTextOverflow.GetLeft(direction));
    mIEnd.Init(style->mTextOverflow.GetRight(direction));
  } else {
    mIStart.Init(style->mTextOverflow.GetRight(direction));
    mIEnd.Init(style->mTextOverflow.GetLeft(direction));
  }
  
  
}

 TextOverflow*
TextOverflow::WillProcessLines(nsDisplayListBuilder*   aBuilder,
                               nsIFrame*               aBlockFrame)
{
  if (!CanHaveTextOverflow(aBuilder, aBlockFrame)) {
    return nullptr;
  }
  nsIScrollableFrame* scrollableFrame = nsLayoutUtils::GetScrollableFrameFor(aBlockFrame);
  if (scrollableFrame && scrollableFrame->IsTransformingByAPZ()) {
    
    return nullptr;
  }
  return new TextOverflow(aBuilder, aBlockFrame);
}

void
TextOverflow::ExamineFrameSubtree(nsIFrame*       aFrame,
                                  const LogicalRect& aContentArea,
                                  const LogicalRect& aInsideMarkersArea,
                                  FrameHashtable* aFramesToHide,
                                  AlignmentEdges* aAlignmentEdges,
                                  bool*           aFoundVisibleTextOrAtomic,
                                  InnerClipEdges* aClippedMarkerEdges)
{
  const nsIAtom* frameType = aFrame->GetType();
  if (frameType == nsGkAtoms::brFrame ||
      frameType == nsGkAtoms::placeholderFrame) {
    return;
  }
  const bool isAtomic = IsAtomicElement(aFrame, frameType);
  if (aFrame->StyleVisibility()->IsVisible()) {
    LogicalRect childRect =
      GetLogicalScrollableOverflowRectRelativeToBlock(aFrame);
    bool overflowIStart =
      childRect.IStart(mBlockWM) < aContentArea.IStart(mBlockWM);
    bool overflowIEnd =
      childRect.IEnd(mBlockWM) > aContentArea.IEnd(mBlockWM);
    if (overflowIStart) {
      mIStart.mHasOverflow = true;
    }
    if (overflowIEnd) {
      mIEnd.mHasOverflow = true;
    }
    if (isAtomic && ((mIStart.mActive && overflowIStart) ||
                     (mIEnd.mActive && overflowIEnd))) {
      aFramesToHide->PutEntry(aFrame);
    } else if (isAtomic || frameType == nsGkAtoms::textFrame) {
      AnalyzeMarkerEdges(aFrame, frameType, aInsideMarkersArea,
                         aFramesToHide, aAlignmentEdges,
                         aFoundVisibleTextOrAtomic,
                         aClippedMarkerEdges);
    }
  }
  if (isAtomic) {
    return;
  }

  nsIFrame* child = aFrame->GetFirstPrincipalChild();
  while (child) {
    ExamineFrameSubtree(child, aContentArea, aInsideMarkersArea,
                        aFramesToHide, aAlignmentEdges,
                        aFoundVisibleTextOrAtomic,
                        aClippedMarkerEdges);
    child = child->GetNextSibling();
  }
}

void
TextOverflow::AnalyzeMarkerEdges(nsIFrame*       aFrame,
                                 const nsIAtom*  aFrameType,
                                 const LogicalRect& aInsideMarkersArea,
                                 FrameHashtable* aFramesToHide,
                                 AlignmentEdges* aAlignmentEdges,
                                 bool*           aFoundVisibleTextOrAtomic,
                                 InnerClipEdges* aClippedMarkerEdges)
{
  LogicalRect borderRect(mBlockWM,
                         nsRect(aFrame->GetOffsetTo(mBlock),
                                aFrame->GetSize()),
                         mBlockSize);
  nscoord istartOverlap = std::max(
    aInsideMarkersArea.IStart(mBlockWM) - borderRect.IStart(mBlockWM), 0);
  nscoord iendOverlap = std::max(
    borderRect.IEnd(mBlockWM) - aInsideMarkersArea.IEnd(mBlockWM), 0);
  bool insideIStartEdge =
    aInsideMarkersArea.IStart(mBlockWM) <= borderRect.IEnd(mBlockWM);
  bool insideIEndEdge =
    borderRect.IStart(mBlockWM) <= aInsideMarkersArea.IEnd(mBlockWM);

  if (istartOverlap > 0) {
    aClippedMarkerEdges->AccumulateIStart(mBlockWM, borderRect);
    if (!mIStart.mActive) {
      istartOverlap = 0;
    }
  }
  if (iendOverlap > 0) {
    aClippedMarkerEdges->AccumulateIEnd(mBlockWM, borderRect);
    if (!mIEnd.mActive) {
      iendOverlap = 0;
    }
  }

  if ((istartOverlap > 0 && insideIStartEdge) ||
      (iendOverlap > 0 && insideIEndEdge)) {
    if (aFrameType == nsGkAtoms::textFrame) {
      if (aInsideMarkersArea.IStart(mBlockWM) <
          aInsideMarkersArea.IEnd(mBlockWM)) {
        
        nscoord snappedIStart, snappedIEnd;
        auto textFrame = static_cast<nsTextFrame*>(aFrame);
        bool isFullyClipped = mBlockWM.IsBidiLTR() ?
          IsFullyClipped(textFrame, istartOverlap, iendOverlap,
                         &snappedIStart, &snappedIEnd) :
          IsFullyClipped(textFrame, iendOverlap, istartOverlap,
                         &snappedIEnd, &snappedIStart);
        if (!isFullyClipped) {
          LogicalRect snappedRect = borderRect;
          if (istartOverlap > 0) {
            snappedRect.IStart(mBlockWM) += snappedIStart;
            snappedRect.ISize(mBlockWM) -= snappedIStart;
          }
          if (iendOverlap > 0) {
            snappedRect.ISize(mBlockWM) -= snappedIEnd;
          }
          aAlignmentEdges->Accumulate(mBlockWM, snappedRect);
          *aFoundVisibleTextOrAtomic = true;
        }
      }
    } else {
      aFramesToHide->PutEntry(aFrame);
    }
  } else if (!insideIStartEdge || !insideIEndEdge) {
    
    if (IsAtomicElement(aFrame, aFrameType)) {
      aFramesToHide->PutEntry(aFrame);
    }
  } else {
    
    aAlignmentEdges->Accumulate(mBlockWM, borderRect);
    *aFoundVisibleTextOrAtomic = true;
  }
}

void
TextOverflow::ExamineLineFrames(nsLineBox*      aLine,
                                FrameHashtable* aFramesToHide,
                                AlignmentEdges* aAlignmentEdges)
{
  
  bool suppressIStart = mIStart.mStyle->mType == NS_STYLE_TEXT_OVERFLOW_CLIP;
  bool suppressIEnd = mIEnd.mStyle->mType == NS_STYLE_TEXT_OVERFLOW_CLIP;
  if (mCanHaveInlineAxisScrollbar) {
    LogicalPoint pos(mBlockWM, mScrollableFrame->GetScrollPosition(),
                     mBlockSize);
    LogicalRect scrollRange(mBlockWM, mScrollableFrame->GetScrollRange(),
                            mBlockSize);
    
    
    if (pos.I(mBlockWM) <= scrollRange.IStart(mBlockWM)) {
      suppressIStart = true;
    }
    if (pos.I(mBlockWM) >= scrollRange.IEnd(mBlockWM)) {
      suppressIEnd = true;
    }
  }

  LogicalRect contentArea = mContentArea;
  const nscoord scrollAdjust = mAdjustForPixelSnapping ?
    mBlock->PresContext()->AppUnitsPerDevPixel() : 0;
  InflateIStart(mBlockWM, &contentArea, scrollAdjust);
  InflateIEnd(mBlockWM, &contentArea, scrollAdjust);
  LogicalRect lineRect(mBlockWM, aLine->GetScrollableOverflowArea(),
                       mBlockSize);
  const bool istartOverflow =
    !suppressIStart && lineRect.IStart(mBlockWM) < contentArea.IStart(mBlockWM);
  const bool iendOverflow =
    !suppressIEnd && lineRect.IEnd(mBlockWM) > contentArea.IEnd(mBlockWM);
  if (!istartOverflow && !iendOverflow) {
    
    return;
  }

  int pass = 0;
  bool retryEmptyLine = true;
  bool guessIStart = istartOverflow;
  bool guessIEnd = iendOverflow;
  mIStart.mActive = istartOverflow;
  mIEnd.mActive = iendOverflow;
  bool clippedIStartMarker = false;
  bool clippedIEndMarker = false;
  do {
    
    if (guessIStart) {
      mIStart.SetupString(mBlock);
    }
    if (guessIEnd) {
      mIEnd.SetupString(mBlock);
    }
    
    
    
    nscoord istartMarkerISize = mIStart.mActive ? mIStart.mISize : 0;
    nscoord iendMarkerISize = mIEnd.mActive ? mIEnd.mISize : 0;
    if (istartMarkerISize && iendMarkerISize &&
        istartMarkerISize + iendMarkerISize > contentArea.ISize(mBlockWM)) {
      istartMarkerISize = 0;
    }

    
    
    LogicalRect insideMarkersArea = mContentArea;
    if (guessIStart) {
      InflateIStart(mBlockWM, &insideMarkersArea, -istartMarkerISize);
    }
    if (guessIEnd) {
      InflateIEnd(mBlockWM, &insideMarkersArea, -iendMarkerISize);
    }

    
    
    bool foundVisibleTextOrAtomic = false;
    int32_t n = aLine->GetChildCount();
    nsIFrame* child = aLine->mFirstChild;
    InnerClipEdges clippedMarkerEdges;
    for (; n-- > 0; child = child->GetNextSibling()) {
      ExamineFrameSubtree(child, contentArea, insideMarkersArea,
                          aFramesToHide, aAlignmentEdges,
                          &foundVisibleTextOrAtomic,
                          &clippedMarkerEdges);
    }
    if (!foundVisibleTextOrAtomic && retryEmptyLine) {
      aAlignmentEdges->mAssigned = false;
      aFramesToHide->Clear();
      pass = -1;
      if (mIStart.IsNeeded() && mIStart.mActive && !clippedIStartMarker) {
        if (clippedMarkerEdges.mAssignedIStart &&
            clippedMarkerEdges.mIStart > mContentArea.IStart(mBlockWM)) {
          mIStart.mISize =
            clippedMarkerEdges.mIStart - mContentArea.IStart(mBlockWM);
          NS_ASSERTION(mIStart.mISize < mIStart.mIntrinsicISize,
                      "clipping a marker should make it strictly smaller");
          clippedIStartMarker = true;
        } else {
          mIStart.mActive = guessIStart = false;
        }
        continue;
      }
      if (mIEnd.IsNeeded() && mIEnd.mActive && !clippedIEndMarker) {
        if (clippedMarkerEdges.mAssignedIEnd &&
            mContentArea.IEnd(mBlockWM) > clippedMarkerEdges.mIEnd) {
          mIEnd.mISize = mContentArea.IEnd(mBlockWM) - clippedMarkerEdges.mIEnd;
          NS_ASSERTION(mIEnd.mISize < mIEnd.mIntrinsicISize,
                      "clipping a marker should make it strictly smaller");
          clippedIEndMarker = true;
        } else {
          mIEnd.mActive = guessIEnd = false;
        }
        continue;
      }
      
      
      retryEmptyLine = false;
      mIStart.mISize = mIStart.mIntrinsicISize;
      mIStart.mActive = guessIStart = istartOverflow;
      mIEnd.mISize = mIEnd.mIntrinsicISize;
      mIEnd.mActive = guessIEnd = iendOverflow;
      continue;
    }
    if (guessIStart == (mIStart.mActive && mIStart.IsNeeded()) &&
        guessIEnd == (mIEnd.mActive && mIEnd.IsNeeded())) {
      break;
    } else {
      guessIStart = mIStart.mActive && mIStart.IsNeeded();
      guessIEnd = mIEnd.mActive && mIEnd.IsNeeded();
      mIStart.Reset();
      mIEnd.Reset();
      aFramesToHide->Clear();
    }
    NS_ASSERTION(pass == 0, "2nd pass should never guess wrong");
  } while (++pass != 2);
  if (!istartOverflow || !mIStart.mActive) {
    mIStart.Reset();
  }
  if (!iendOverflow || !mIEnd.mActive) {
    mIEnd.Reset();
  }
}

void
TextOverflow::ProcessLine(const nsDisplayListSet& aLists,
                          nsLineBox*              aLine)
{
  NS_ASSERTION(mIStart.mStyle->mType != NS_STYLE_TEXT_OVERFLOW_CLIP ||
               mIEnd.mStyle->mType != NS_STYLE_TEXT_OVERFLOW_CLIP,
               "TextOverflow with 'clip' for both sides");
  mIStart.Reset();
  mIStart.mActive = mIStart.mStyle->mType != NS_STYLE_TEXT_OVERFLOW_CLIP;
  mIEnd.Reset();
  mIEnd.mActive = mIEnd.mStyle->mType != NS_STYLE_TEXT_OVERFLOW_CLIP;

  FrameHashtable framesToHide(64);
  AlignmentEdges alignmentEdges;
  ExamineLineFrames(aLine, &framesToHide, &alignmentEdges);
  bool needIStart = mIStart.IsNeeded();
  bool needIEnd = mIEnd.IsNeeded();
  if (!needIStart && !needIEnd) {
    return;
  }
  NS_ASSERTION(mIStart.mStyle->mType != NS_STYLE_TEXT_OVERFLOW_CLIP ||
               !needIStart, "left marker for 'clip'");
  NS_ASSERTION(mIEnd.mStyle->mType != NS_STYLE_TEXT_OVERFLOW_CLIP ||
               !needIEnd, "right marker for 'clip'");

  
  
  if (needIStart && needIEnd &&
      mIStart.mISize + mIEnd.mISize > mContentArea.ISize(mBlockWM)) {
    needIStart = false;
  }
  LogicalRect insideMarkersArea = mContentArea;
  if (needIStart) {
    InflateIStart(mBlockWM, &insideMarkersArea, -mIStart.mISize);
  }
  if (needIEnd) {
    InflateIEnd(mBlockWM, &insideMarkersArea, -mIEnd.mISize);
  }
  if (!mCanHaveInlineAxisScrollbar && alignmentEdges.mAssigned) {
    LogicalRect alignmentRect(mBlockWM, alignmentEdges.mIStart,
                              insideMarkersArea.BStart(mBlockWM),
                              alignmentEdges.ISize(), 1);
    insideMarkersArea.IntersectRect(insideMarkersArea, alignmentRect);
  }

  
  nsDisplayList* lists[] = { aLists.Content(), aLists.PositionedDescendants() };
  for (uint32_t i = 0; i < ArrayLength(lists); ++i) {
    PruneDisplayListContents(lists[i], framesToHide, insideMarkersArea);
  }
  CreateMarkers(aLine, needIStart, needIEnd, insideMarkersArea);
}

void
TextOverflow::PruneDisplayListContents(nsDisplayList* aList,
                                       const FrameHashtable& aFramesToHide,
                                       const LogicalRect& aInsideMarkersArea)
{
  nsDisplayList saved;
  nsDisplayItem* item;
  while ((item = aList->RemoveBottom())) {
    nsIFrame* itemFrame = item->Frame();
    if (IsFrameDescendantOfAny(itemFrame, aFramesToHide, mBlock)) {
      item->~nsDisplayItem();
      continue;
    }

    nsDisplayList* wrapper = item->GetSameCoordinateSystemChildren();
    if (wrapper) {
      if (!itemFrame || GetSelfOrNearestBlock(itemFrame) == mBlock) {
        PruneDisplayListContents(wrapper, aFramesToHide, aInsideMarkersArea);
      }
    }

    nsCharClipDisplayItem* charClip = itemFrame ? 
      nsCharClipDisplayItem::CheckCast(item) : nullptr;
    if (charClip && GetSelfOrNearestBlock(itemFrame) == mBlock) {
      LogicalRect rect =
        GetLogicalScrollableOverflowRectRelativeToBlock(itemFrame);
      if (mIStart.IsNeeded()) {
        nscoord istart =
          aInsideMarkersArea.IStart(mBlockWM) - rect.IStart(mBlockWM);
        if (istart > 0) {
          (mBlockWM.IsBidiLTR() ?
           charClip->mVisIStartEdge : charClip->mVisIEndEdge) = istart;
        }
      }
      if (mIEnd.IsNeeded()) {
        nscoord iend = rect.IEnd(mBlockWM) - aInsideMarkersArea.IEnd(mBlockWM);
        if (iend > 0) {
          (mBlockWM.IsBidiLTR() ?
           charClip->mVisIEndEdge : charClip->mVisIStartEdge) = iend;
        }
      }
    }

    saved.AppendToTop(item);
  }
  aList->AppendToTop(&saved);
}

 bool
TextOverflow::HasClippedOverflow(nsIFrame* aBlockFrame)
{
  const nsStyleTextReset* style = aBlockFrame->StyleTextReset();
  return style->mTextOverflow.mLeft.mType == NS_STYLE_TEXT_OVERFLOW_CLIP &&
         style->mTextOverflow.mRight.mType == NS_STYLE_TEXT_OVERFLOW_CLIP;
}

 bool
TextOverflow::CanHaveTextOverflow(nsDisplayListBuilder* aBuilder,
                                  nsIFrame*             aBlockFrame)
{
  
  
  if (HasClippedOverflow(aBlockFrame) ||
      IsInlineAxisOverflowVisible(aBlockFrame) ||
      aBuilder->IsForEventDelivery() || aBuilder->IsForImageVisibility()) {
    return false;
  }

  
  
  if (aBlockFrame->GetType() == nsGkAtoms::comboboxControlFrame) {
    return false;
  }

  
  nsRefPtr<nsCaret> caret = aBlockFrame->PresContext()->PresShell()->GetCaret();
  if (caret && caret->IsVisible()) {
    nsCOMPtr<nsISelection> domSelection = caret->GetSelection();
    if (domSelection) {
      nsCOMPtr<nsIDOMNode> node;
      domSelection->GetFocusNode(getter_AddRefs(node));
      nsCOMPtr<nsIContent> content = do_QueryInterface(node);
      if (content && nsContentUtils::ContentIsDescendantOf(content,
                       aBlockFrame->GetContent())) {
        return false;
      }
    }
  }
  return true;
}

void
TextOverflow::CreateMarkers(const nsLineBox* aLine,
                            bool aCreateIStart, bool aCreateIEnd,
                            const mozilla::LogicalRect& aInsideMarkersArea)
{
  if (aCreateIStart) {
    DisplayListClipState::AutoSaveRestore clipState(mBuilder);

    LogicalRect markerLogicalRect(
      mBlockWM, aInsideMarkersArea.IStart(mBlockWM) - mIStart.mIntrinsicISize,
      aLine->BStart(), mIStart.mIntrinsicISize, aLine->BSize());
    nsPoint offset = mBuilder->ToReferenceFrame(mBlock);
    nsRect markerRect =
      markerLogicalRect.GetPhysicalRect(mBlockWM, mBlockSize) + offset;
    ClipMarker(mContentArea.GetPhysicalRect(mBlockWM, mBlockSize) + offset,
               markerRect, clipState);
    nsDisplayItem* marker = new (mBuilder)
      nsDisplayTextOverflowMarker(mBuilder, mBlock, markerRect,
                                  aLine->GetLogicalAscent(), mIStart.mStyle, 0);
    mMarkerList.AppendNewToTop(marker);
  }

  if (aCreateIEnd) {
    DisplayListClipState::AutoSaveRestore clipState(mBuilder);

    LogicalRect markerLogicalRect(
      mBlockWM, aInsideMarkersArea.IEnd(mBlockWM), aLine->BStart(),
      mIEnd.mIntrinsicISize, aLine->BSize());
    nsPoint offset = mBuilder->ToReferenceFrame(mBlock);
    nsRect markerRect =
      markerLogicalRect.GetPhysicalRect(mBlockWM, mBlockSize) + offset;
    ClipMarker(mContentArea.GetPhysicalRect(mBlockWM, mBlockSize) + offset,
               markerRect, clipState);
    nsDisplayItem* marker = new (mBuilder)
      nsDisplayTextOverflowMarker(mBuilder, mBlock, markerRect,
                                  aLine->GetLogicalAscent(), mIEnd.mStyle, 1);
    mMarkerList.AppendNewToTop(marker);
  }
}

void
TextOverflow::Marker::SetupString(nsIFrame* aFrame)
{
  if (mInitialized) {
    return;
  }

  if (mStyle->mType == NS_STYLE_TEXT_OVERFLOW_ELLIPSIS) {
    gfxTextRun* textRun = GetEllipsisTextRun(aFrame);
    if (textRun) {
      mISize = textRun->GetAdvanceWidth(0, textRun->GetLength(), nullptr);
    } else {
      mISize = 0;
    }
  } else {
    nsRenderingContext rc(
      aFrame->PresContext()->PresShell()->CreateReferenceRenderingContext());
    nsRefPtr<nsFontMetrics> fm;
    nsLayoutUtils::GetFontMetricsForFrame(aFrame, getter_AddRefs(fm),
      nsLayoutUtils::FontSizeInflationFor(aFrame));
    mISize = nsLayoutUtils::AppUnitWidthOfStringBidi(mStyle->mString, aFrame,
                                                     *fm, rc);
  }
  mIntrinsicISize = mISize;
  mInitialized = true;
}

} 
} 
