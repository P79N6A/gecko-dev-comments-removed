





































#include "TextOverflow.h"


#include "nsBlockFrame.h"
#include "nsCaret.h"
#include "nsContentUtils.h"
#include "nsIScrollableFrame.h"
#include "nsLayoutUtils.h"
#include "nsPresContext.h"
#include "nsRect.h"
#include "nsRenderingContext.h"
#include "nsTextFrame.h"

namespace mozilla {
namespace css {

static const PRUnichar kEllipsisChar[] = { 0x2026, 0x0 };
static const PRUnichar kASCIIPeriodsChar[] = { '.', '.', '.', 0x0 };



static nsDependentString GetEllipsis(nsIFrame* aFrame)
{
  
  nsRefPtr<nsFontMetrics> fm;
  nsLayoutUtils::GetFontMetricsForFrame(aFrame, getter_AddRefs(fm));
  gfxFontGroup* fontGroup = fm->GetThebesFontGroup();
  gfxFont* firstFont = fontGroup->GetFontAt(0);
  return firstFont && firstFont->HasCharacter(kEllipsisChar[0])
    ? nsDependentString(kEllipsisChar,
                        NS_ARRAY_LENGTH(kEllipsisChar) - 1)
    : nsDependentString(kASCIIPeriodsChar,
                        NS_ARRAY_LENGTH(kASCIIPeriodsChar) - 1);
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
  NS_PRECONDITION(!aFrame->GetStyleDisplay()->IsBlockOutside(),
                  "unexpected block frame");

  if (aFrame->IsFrameOfType(nsIFrame::eReplaced)) {
    if (aFrameType != nsGkAtoms::textFrame &&
        aFrameType != nsGkAtoms::brFrame) {
      return true;
    }
  }
  return aFrame->GetStyleDisplay()->mDisplay != NS_STYLE_DISPLAY_INLINE;
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
  nsRefPtr<nsRenderingContext> rc =
    aFrame->PresContext()->PresShell()->GetReferenceRenderingContext();
  return rc &&
    !aFrame->MeasureCharClippedText(rc->ThebesContext(), aLeft, aRight,
                                    aSnappedLeft, aSnappedRight);
}

static bool
IsHorizontalOverflowVisible(nsIFrame* aFrame)
{
  NS_PRECONDITION(nsLayoutUtils::GetAsBlock(aFrame) != nsnull,
                  "expected a block frame");

  nsIFrame* f = aFrame;
  while (f && f->GetStyleContext()->GetPseudo()) {
    f = f->GetParent();
  }
  return !f || f->GetStyleDisplay()->mOverflowX == NS_STYLE_OVERFLOW_VISIBLE;
}

static nsDisplayItem*
ClipMarker(nsDisplayListBuilder* aBuilder,
           nsIFrame*             aFrame,
           nsDisplayItem*        aMarker,
           const nsRect&         aContentArea,
           nsRect*               aMarkerRect)
{
  nsDisplayItem* item = aMarker;
  nscoord rightOverflow = aMarkerRect->XMost() - aContentArea.XMost();
  if (rightOverflow > 0) {
    
    aMarkerRect->width -= rightOverflow;
    item = new (aBuilder)
      nsDisplayClip(aBuilder, aFrame, aMarker, *aMarkerRect);
  } else {
    nscoord leftOverflow = aContentArea.x - aMarkerRect->x;
    if (leftOverflow > 0) {
      
      aMarkerRect->width -= leftOverflow;
      aMarkerRect->x += leftOverflow;
      item = new (aBuilder)
        nsDisplayClip(aBuilder, aFrame, aMarker, *aMarkerRect);
    }
  }
  return item;
}

static void
InflateLeft(nsRect* aRect, bool aInfinity, nscoord aDelta)
{
  nscoord xmost = aRect->XMost();
  if (aInfinity) {
    aRect->x = nscoord_MIN;
  } else {
    aRect->x -= aDelta;
  }
  aRect->width = NS_MAX(xmost - aRect->x, 0);
}

static void
InflateRight(nsRect* aRect, bool aInfinity, nscoord aDelta)
{
  if (aInfinity) {
    aRect->width = nscoord_MAX;
  } else {
    aRect->width = NS_MAX(aRect->width + aDelta, 0);
  }
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
                              const nsString& aString)
    : nsDisplayItem(aBuilder, aFrame), mRect(aRect), mString(aString),
      mAscent(aAscent) {
    MOZ_COUNT_CTOR(nsDisplayTextOverflowMarker);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayTextOverflowMarker() {
    MOZ_COUNT_DTOR(nsDisplayTextOverflowMarker);
  }
#endif
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder) {
    return mRect;
  }
  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx);
  NS_DISPLAY_DECL_NAME("TextOverflow", TYPE_TEXT_OVERFLOW)
private:
  nsRect          mRect;   
  const nsString  mString; 
  nscoord         mAscent; 
};

void
nsDisplayTextOverflowMarker::Paint(nsDisplayListBuilder* aBuilder,
                                   nsRenderingContext*   aCtx)
{
  nsStyleContext* sc = mFrame->GetStyleContext();
  nsLayoutUtils::SetFontFromStyle(aCtx, sc);
  aCtx->SetColor(nsLayoutUtils::GetTextColor(mFrame));
  nsPoint baselinePt = mRect.TopLeft();
  baselinePt.y += mAscent;
  nsLayoutUtils::DrawString(mFrame, aCtx, mString.get(), mString.Length(),
                            baselinePt);
}

 TextOverflow*
TextOverflow::WillProcessLines(nsDisplayListBuilder*   aBuilder,
                               const nsDisplayListSet& aLists,
                               nsIFrame*               aBlockFrame)
{
  if (!CanHaveTextOverflow(aBuilder, aBlockFrame)) {
    return nsnull;
  }

  nsAutoPtr<TextOverflow> textOverflow(new TextOverflow);
  textOverflow->mBuilder = aBuilder;
  textOverflow->mBlock = aBlockFrame;
  textOverflow->mMarkerList = aLists.PositionedDescendants();
  textOverflow->mContentArea = aBlockFrame->GetContentRectRelativeToSelf();
  nsIScrollableFrame* scroll =
    nsLayoutUtils::GetScrollableFrameFor(aBlockFrame);
  textOverflow->mCanHaveHorizontalScrollbar = false;
  if (scroll) {
    textOverflow->mCanHaveHorizontalScrollbar =
      scroll->GetScrollbarStyles().mHorizontal != NS_STYLE_OVERFLOW_HIDDEN;
    textOverflow->mContentArea.MoveBy(scroll->GetScrollPosition());
  }
  textOverflow->mBlockIsRTL =
    aBlockFrame->GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL;
  const nsStyleTextReset* style = aBlockFrame->GetStyleTextReset();
  textOverflow->mLeft.Init(style->mTextOverflow);
  textOverflow->mRight.Init(style->mTextOverflow);
  
  

  return textOverflow.forget();
}

void
TextOverflow::DidProcessLines()
{
  nsIScrollableFrame* scroll = nsLayoutUtils::GetScrollableFrameFor(mBlock);
  if (scroll) {
    
    
    nsIFrame* scrollFrame = do_QueryFrame(scroll);
    nsDisplayItem* marker = new (mBuilder)
      nsDisplayForcePaintOnScroll(mBuilder, scrollFrame);
    if (marker) {
      mMarkerList->AppendNewToBottom(marker);
      mBlock->PresContext()->SetHasFixedBackgroundFrame();
    }
  }
}

void
TextOverflow::ExamineFrameSubtree(nsIFrame*       aFrame,
                                  const nsRect&   aContentArea,
                                  const nsRect&   aInsideMarkersArea,
                                  FrameHashtable* aFramesToHide,
                                  AlignmentEdges* aAlignmentEdges)
{
  const nsIAtom* frameType = aFrame->GetType();
  if (frameType == nsGkAtoms::brFrame) {
    return;
  }
  const bool isAtomic = IsAtomicElement(aFrame, frameType);
  if (aFrame->GetStyleVisibility()->IsVisible()) {
    nsRect childRect = aFrame->GetScrollableOverflowRect() +
                       aFrame->GetOffsetTo(mBlock);
    bool overflowLeft = childRect.x < aContentArea.x;
    bool overflowRight = childRect.XMost() > aContentArea.XMost();
    if (overflowLeft) {
      mLeft.mHasOverflow = true;
    }
    if (overflowRight) {
      mRight.mHasOverflow = true;
    }
    if (isAtomic && (overflowLeft || overflowRight)) {
      aFramesToHide->PutEntry(aFrame);
    } else if (isAtomic || frameType == nsGkAtoms::textFrame) {
      AnalyzeMarkerEdges(aFrame, frameType, aInsideMarkersArea,
                         aFramesToHide, aAlignmentEdges);
    }
  }
  if (isAtomic) {
    return;
  }

  nsIFrame* child = aFrame->GetFirstChild(nsnull);
  while (child) {
    ExamineFrameSubtree(child, aContentArea, aInsideMarkersArea,
                        aFramesToHide, aAlignmentEdges);
    child = child->GetNextSibling();
  }
}

void
TextOverflow::AnalyzeMarkerEdges(nsIFrame*       aFrame,
                                 const nsIAtom*  aFrameType,
                                 const nsRect&   aInsideMarkersArea,
                                 FrameHashtable* aFramesToHide,
                                 AlignmentEdges* aAlignmentEdges)
{
  nsRect borderRect(aFrame->GetOffsetTo(mBlock), aFrame->GetSize());
  nscoord leftOverlap =
    NS_MAX(aInsideMarkersArea.x - borderRect.x, 0);
  nscoord rightOverlap =
    NS_MAX(borderRect.XMost() - aInsideMarkersArea.XMost(), 0);
  bool insideLeftEdge = aInsideMarkersArea.x <= borderRect.XMost();
  bool insideRightEdge = borderRect.x <= aInsideMarkersArea.XMost();

  if ((leftOverlap > 0 && insideLeftEdge) ||
      (rightOverlap > 0 && insideRightEdge)) {
    if (aFrameType == nsGkAtoms::textFrame &&
        aInsideMarkersArea.x < aInsideMarkersArea.XMost()) {
      
      nscoord snappedLeft, snappedRight;
      bool isFullyClipped =
        IsFullyClipped(static_cast<nsTextFrame*>(aFrame),
                       leftOverlap, rightOverlap, &snappedLeft, &snappedRight);
      if (!isFullyClipped) {
        nsRect snappedRect = borderRect;
        if (leftOverlap > 0) {
          snappedRect.x += snappedLeft;
          snappedRect.width -= snappedLeft;
        }
        if (rightOverlap > 0) {
          snappedRect.width -= snappedRight;
        }
        aAlignmentEdges->Accumulate(snappedRect);
      }
    } else if (IsAtomicElement(aFrame, aFrameType)) {
      aFramesToHide->PutEntry(aFrame);
    }
  } else if (!insideLeftEdge || !insideRightEdge) {
    
    if (IsAtomicElement(aFrame, aFrameType)) {
      aFramesToHide->PutEntry(aFrame);
    }
  } else {
    
    aAlignmentEdges->Accumulate(borderRect);
  }
}

void
TextOverflow::ExamineLineFrames(nsLineBox*      aLine,
                                FrameHashtable* aFramesToHide,
                                AlignmentEdges* aAlignmentEdges)
{
  
  
  
  nsRect contentArea = mContentArea;
  const nscoord scrollAdjust = mCanHaveHorizontalScrollbar ?
    mBlock->PresContext()->AppUnitsPerDevPixel() : 0;
  InflateLeft(&contentArea,
              mLeft.mStyle->mType == NS_STYLE_TEXT_OVERFLOW_CLIP,
              scrollAdjust);
  InflateRight(&contentArea,
               mRight.mStyle->mType == NS_STYLE_TEXT_OVERFLOW_CLIP,
               scrollAdjust);
  nsRect lineRect = aLine->GetScrollableOverflowArea();
  const bool leftOverflow = lineRect.x < contentArea.x;
  const bool rightOverflow = lineRect.XMost() > contentArea.XMost();
  if (!leftOverflow && !rightOverflow) {
    
    return;
  }

  PRUint32 pass = 0;
  bool guessLeft =
    mLeft.mStyle->mType != NS_STYLE_TEXT_OVERFLOW_CLIP && leftOverflow;
  bool guessRight =
    mRight.mStyle->mType != NS_STYLE_TEXT_OVERFLOW_CLIP && rightOverflow;
  do {
    
    if (guessLeft || guessRight) {
      mLeft.SetupString(mBlock);
      mRight.mMarkerString = mLeft.mMarkerString;
      mRight.mWidth = mLeft.mWidth;
      mRight.mInitialized = mLeft.mInitialized;
    }
    
    
    
    nscoord rightMarkerWidth = mRight.mWidth;
    nscoord leftMarkerWidth = mLeft.mWidth;
    if (leftOverflow && rightOverflow &&
        leftMarkerWidth + rightMarkerWidth > contentArea.width) {
      if (mBlockIsRTL) {
        rightMarkerWidth = 0;
      } else {
        leftMarkerWidth = 0;
      }
    }

    
    
    nsRect insideMarkersArea = mContentArea;
    InflateLeft(&insideMarkersArea, !guessLeft, -leftMarkerWidth);
    InflateRight(&insideMarkersArea, !guessRight, -rightMarkerWidth);

    
    
    PRInt32 n = aLine->GetChildCount();
    nsIFrame* child = aLine->mFirstChild;
    for (; n-- > 0; child = child->GetNextSibling()) {
      ExamineFrameSubtree(child, contentArea, insideMarkersArea,
                          aFramesToHide, aAlignmentEdges);
    }
    if (guessLeft == mLeft.IsNeeded() && guessRight == mRight.IsNeeded()) {
      break;
    } else {
      guessLeft = mLeft.IsNeeded();
      guessRight = mRight.IsNeeded();
      mLeft.Reset();
      mRight.Reset();
      aFramesToHide->Clear();
    }
    NS_ASSERTION(pass == 0, "2nd pass should never guess wrong");
  } while (++pass != 2);
}

void
TextOverflow::ProcessLine(const nsDisplayListSet& aLists,
                          nsLineBox*              aLine)
{
  NS_ASSERTION(mLeft.mStyle->mType != NS_STYLE_TEXT_OVERFLOW_CLIP ||
               mRight.mStyle->mType != NS_STYLE_TEXT_OVERFLOW_CLIP,
               "TextOverflow with 'clip' for both sides");
  mLeft.Reset();
  mRight.Reset();
  FrameHashtable framesToHide;
  if (!framesToHide.Init(100)) {
    return;
  }
  AlignmentEdges alignmentEdges;
  ExamineLineFrames(aLine, &framesToHide, &alignmentEdges);
  bool needLeft = mLeft.IsNeeded();
  bool needRight = mRight.IsNeeded();
  if (!needLeft && !needRight) {
    return;
  }
  NS_ASSERTION(mLeft.mStyle->mType != NS_STYLE_TEXT_OVERFLOW_CLIP ||
               !needLeft, "left marker for 'clip'");
  NS_ASSERTION(mRight.mStyle->mType != NS_STYLE_TEXT_OVERFLOW_CLIP ||
               !needRight, "right marker for 'clip'");

  
  
  if (needLeft && needRight &&
      mLeft.mWidth + mRight.mWidth > mContentArea.width) {
    if (mBlockIsRTL) {
      needRight = false;
    } else {
      needLeft = false;
    }
  }
  nsRect insideMarkersArea = mContentArea;
  if (needLeft) {
    InflateLeft(&insideMarkersArea, false, -mLeft.mWidth);
  }
  if (needRight) {
    InflateRight(&insideMarkersArea, false, -mRight.mWidth);
  }
  if (!mCanHaveHorizontalScrollbar && alignmentEdges.mAssigned) {
    nsRect alignmentRect = nsRect(alignmentEdges.x, insideMarkersArea.y,
                                  alignmentEdges.Width(), 1);
    insideMarkersArea.IntersectRect(insideMarkersArea, alignmentRect);
  }

  
  nsDisplayList* lists[] = { aLists.Content(), aLists.PositionedDescendants() };
  for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(lists); ++i) {
    PruneDisplayListContents(lists[i], framesToHide, insideMarkersArea);
  }
  CreateMarkers(aLine, needLeft, needRight, insideMarkersArea);
}

void
TextOverflow::PruneDisplayListContents(nsDisplayList*        aList,
                                       const FrameHashtable& aFramesToHide,
                                       const nsRect&         aInsideMarkersArea)
{
  nsDisplayList saved;
  nsDisplayItem* item;
  while ((item = aList->RemoveBottom())) {
    nsIFrame* itemFrame = item->GetUnderlyingFrame();
    if (itemFrame && IsFrameDescendantOfAny(itemFrame, aFramesToHide, mBlock)) {
      item->~nsDisplayItem();
      continue;
    }

    nsDisplayList* wrapper = item->GetList();
    if (wrapper) {
      if (!itemFrame || GetSelfOrNearestBlock(itemFrame) == mBlock) {
        PruneDisplayListContents(wrapper, aFramesToHide, aInsideMarkersArea);
      }
    }

    nsCharClipDisplayItem* charClip = itemFrame ? 
      nsCharClipDisplayItem::CheckCast(item) : nsnull;
    if (charClip && GetSelfOrNearestBlock(itemFrame) == mBlock) {
      nsRect rect = itemFrame->GetScrollableOverflowRect() +
                    itemFrame->GetOffsetTo(mBlock);
      if (mLeft.IsNeeded() && rect.x < aInsideMarkersArea.x) {
        charClip->mLeftEdge = aInsideMarkersArea.x - rect.x;
      }
      if (mRight.IsNeeded() && rect.XMost() > aInsideMarkersArea.XMost()) {
        charClip->mRightEdge = rect.XMost() - aInsideMarkersArea.XMost();
      }
    }

    saved.AppendToTop(item);
  }
  aList->AppendToTop(&saved);
}

 bool
TextOverflow::CanHaveTextOverflow(nsDisplayListBuilder* aBuilder,
                                  nsIFrame*             aBlockFrame)
{
  const nsStyleTextReset* style = aBlockFrame->GetStyleTextReset();
  
  
  if ((style->mTextOverflow.mType == NS_STYLE_TEXT_OVERFLOW_CLIP) ||
      IsHorizontalOverflowVisible(aBlockFrame) ||
      aBuilder->IsForEventDelivery()) {
    return false;
  }

  
  nsRefPtr<nsCaret> caret = aBlockFrame->PresContext()->PresShell()->GetCaret();
  PRBool visible = PR_FALSE;
  if (caret && NS_SUCCEEDED(caret->GetCaretVisible(&visible)) && visible) {
    nsCOMPtr<nsISelection> domSelection = caret->GetCaretDOMSelection();
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
                            bool             aCreateLeft,
                            bool             aCreateRight,
                            const nsRect&    aInsideMarkersArea) const
{
  if (aCreateLeft) {
    nsRect markerRect = nsRect(aInsideMarkersArea.x - mLeft.mWidth,
                               aLine->mBounds.y,
                               mLeft.mWidth, aLine->mBounds.height);
    markerRect += mBuilder->ToReferenceFrame(mBlock);
    nsDisplayItem* marker = new (mBuilder)
      nsDisplayTextOverflowMarker(mBuilder, mBlock, markerRect,
                                  aLine->GetAscent(), mLeft.mMarkerString);
    if (marker) {
      marker = ClipMarker(mBuilder, mBlock, marker,
                          mContentArea + mBuilder->ToReferenceFrame(mBlock),
                          &markerRect);
    }
    mMarkerList->AppendNewToTop(marker);
  }

  if (aCreateRight) {
    nsRect markerRect = nsRect(aInsideMarkersArea.XMost(),
                               aLine->mBounds.y,
                               mRight.mWidth, aLine->mBounds.height);
    markerRect += mBuilder->ToReferenceFrame(mBlock);
    nsDisplayItem* marker = new (mBuilder)
      nsDisplayTextOverflowMarker(mBuilder, mBlock, markerRect,
                                  aLine->GetAscent(), mRight.mMarkerString);
    if (marker) {
      marker = ClipMarker(mBuilder, mBlock, marker,
                          mContentArea + mBuilder->ToReferenceFrame(mBlock),
                          &markerRect);
    }
    mMarkerList->AppendNewToTop(marker);
  }
}

void
TextOverflow::Marker::SetupString(nsIFrame* aFrame)
{
  if (mInitialized) {
    return;
  }
  nsRefPtr<nsRenderingContext> rc =
    aFrame->PresContext()->PresShell()->GetReferenceRenderingContext();
  nsLayoutUtils::SetFontFromStyle(rc, aFrame->GetStyleContext());

  mMarkerString = mStyle->mType == NS_STYLE_TEXT_OVERFLOW_ELLIPSIS ?
                    GetEllipsis(aFrame) : mStyle->mString;
  mWidth = nsLayoutUtils::GetStringWidth(aFrame, rc, mMarkerString.get(),
                                         mMarkerString.Length());
  mInitialized = true;
}

}  
}  
