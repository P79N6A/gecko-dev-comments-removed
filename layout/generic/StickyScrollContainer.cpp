










#include "StickyScrollContainer.h"
#include "nsIFrame.h"
#include "nsIScrollableFrame.h"
#include "nsLayoutUtils.h"
#include "RestyleTracker.h"

namespace mozilla {

NS_DECLARE_FRAME_PROPERTY(StickyScrollContainerProperty,
                          DeleteValue<StickyScrollContainer>)

StickyScrollContainer::StickyScrollContainer(nsIScrollableFrame* aScrollFrame)
  : mScrollFrame(aScrollFrame)
  , mScrollPosition()
{
  mScrollFrame->AddScrollPositionListener(this);
}

StickyScrollContainer::~StickyScrollContainer()
{
  mScrollFrame->RemoveScrollPositionListener(this);
}


StickyScrollContainer*
StickyScrollContainer::GetStickyScrollContainerForFrame(nsIFrame* aFrame)
{
  nsIScrollableFrame* scrollFrame =
    nsLayoutUtils::GetNearestScrollableFrame(aFrame->GetParent(),
      nsLayoutUtils::SCROLLABLE_SAME_DOC |
      nsLayoutUtils::SCROLLABLE_INCLUDE_HIDDEN);
  if (!scrollFrame) {
    
    
    return nullptr;
  }
  FrameProperties props = static_cast<nsIFrame*>(do_QueryFrame(scrollFrame))->
    Properties();
  StickyScrollContainer* s = static_cast<StickyScrollContainer*>
    (props.Get(StickyScrollContainerProperty()));
  if (!s) {
    s = new StickyScrollContainer(scrollFrame);
    props.Set(StickyScrollContainerProperty(), s);
  }
  return s;
}


void
StickyScrollContainer::NotifyReparentedFrameAcrossScrollFrameBoundary(nsIFrame* aFrame,
                                                                      nsIFrame* aOldParent)
{
  nsIScrollableFrame* oldScrollFrame =
    nsLayoutUtils::GetNearestScrollableFrame(aOldParent,
      nsLayoutUtils::SCROLLABLE_SAME_DOC |
      nsLayoutUtils::SCROLLABLE_INCLUDE_HIDDEN);
  if (!oldScrollFrame) {
    
    
    return;
  }
  FrameProperties props = static_cast<nsIFrame*>(do_QueryFrame(oldScrollFrame))->
    Properties();
  StickyScrollContainer* oldSSC = static_cast<StickyScrollContainer*>
    (props.Get(StickyScrollContainerProperty()));
  if (!oldSSC) {
    
    
    return;
  }

  auto i = oldSSC->mFrames.Length();
  while (i-- > 0) {
    nsIFrame* f = oldSSC->mFrames[i];
    StickyScrollContainer* newSSC = GetStickyScrollContainerForFrame(f);
    if (newSSC != oldSSC) {
      oldSSC->RemoveFrame(f);
      if (newSSC) {
        newSSC->AddFrame(f);
      }
    }
  }
}


StickyScrollContainer*
StickyScrollContainer::GetStickyScrollContainerForScrollFrame(nsIFrame* aFrame)
{
  FrameProperties props = aFrame->Properties();
  return static_cast<StickyScrollContainer*>
    (props.Get(StickyScrollContainerProperty()));
}

static nscoord
ComputeStickySideOffset(Side aSide, const nsStyleSides& aOffset,
                        nscoord aPercentBasis)
{
  if (eStyleUnit_Auto == aOffset.GetUnit(aSide)) {
    return NS_AUTOOFFSET;
  } else {
    return nsLayoutUtils::ComputeCBDependentValue(aPercentBasis,
                                                  aOffset.Get(aSide));
  }
}


void
StickyScrollContainer::ComputeStickyOffsets(nsIFrame* aFrame)
{
  nsIScrollableFrame* scrollableFrame =
    nsLayoutUtils::GetNearestScrollableFrame(aFrame->GetParent(),
      nsLayoutUtils::SCROLLABLE_SAME_DOC |
      nsLayoutUtils::SCROLLABLE_INCLUDE_HIDDEN);

  if (!scrollableFrame) {
    
    return;
  }

  nsSize scrollContainerSize = scrollableFrame->GetScrolledFrame()->
    GetContentRectRelativeToSelf().Size();

  nsMargin computedOffsets;
  const nsStylePosition* position = aFrame->StylePosition();

  computedOffsets.left   = ComputeStickySideOffset(eSideLeft, position->mOffset,
                                                   scrollContainerSize.width);
  computedOffsets.right  = ComputeStickySideOffset(eSideRight, position->mOffset,
                                                   scrollContainerSize.width);
  computedOffsets.top    = ComputeStickySideOffset(eSideTop, position->mOffset,
                                                   scrollContainerSize.height);
  computedOffsets.bottom = ComputeStickySideOffset(eSideBottom, position->mOffset,
                                                   scrollContainerSize.height);

  
  FrameProperties props = aFrame->Properties();
  nsMargin* offsets = static_cast<nsMargin*>
    (props.Get(nsIFrame::ComputedOffsetProperty()));
  if (offsets) {
    *offsets = computedOffsets;
  } else {
    props.Set(nsIFrame::ComputedOffsetProperty(),
              new nsMargin(computedOffsets));
  }
}

void
StickyScrollContainer::ComputeStickyLimits(nsIFrame* aFrame, nsRect* aStick,
                                           nsRect* aContain) const
{
  NS_ASSERTION(nsLayoutUtils::IsFirstContinuationOrIBSplitSibling(aFrame),
               "Can't sticky position individual continuations");

  aStick->SetRect(nscoord_MIN/2, nscoord_MIN/2, nscoord_MAX, nscoord_MAX);
  aContain->SetRect(nscoord_MIN/2, nscoord_MIN/2, nscoord_MAX, nscoord_MAX);

  const nsMargin* computedOffsets = static_cast<nsMargin*>(
    aFrame->Properties().Get(nsIFrame::ComputedOffsetProperty()));
  if (!computedOffsets) {
    
    
    return;
  }

  nsIFrame* scrolledFrame = mScrollFrame->GetScrolledFrame();
  nsIFrame* cbFrame = aFrame->GetContainingBlock();
  NS_ASSERTION(cbFrame == scrolledFrame ||
    nsLayoutUtils::IsProperAncestorFrame(scrolledFrame, cbFrame),
    "Scroll frame should be an ancestor of the containing block");

  nsRect rect =
    nsLayoutUtils::GetAllInFlowRectsUnion(aFrame, aFrame->GetParent());

  
  
  
  if (cbFrame != scrolledFrame) {
    *aContain = nsLayoutUtils::
      GetAllInFlowRectsUnion(cbFrame, aFrame->GetParent(),
                             nsLayoutUtils::RECTS_USE_CONTENT_BOX);
    nsRect marginRect = nsLayoutUtils::
      GetAllInFlowRectsUnion(aFrame, aFrame->GetParent(),
                             nsLayoutUtils::RECTS_USE_MARGIN_BOX);

    
    
    
    
    aContain->Deflate(marginRect - rect);

    
    
    aContain->Deflate(nsMargin(0, rect.width, rect.height, 0));
  }

  nsMargin sfPadding = scrolledFrame->GetUsedPadding();
  nsPoint sfOffset = aFrame->GetParent()->GetOffsetTo(scrolledFrame);

  
  if (computedOffsets->top != NS_AUTOOFFSET) {
    aStick->SetTopEdge(mScrollPosition.y + sfPadding.top +
                       computedOffsets->top - sfOffset.y);
  }

  nsSize sfSize = scrolledFrame->GetContentRectRelativeToSelf().Size();

  
  if (computedOffsets->bottom != NS_AUTOOFFSET &&
      (computedOffsets->top == NS_AUTOOFFSET ||
       rect.height <= sfSize.height - computedOffsets->TopBottom())) {
    aStick->SetBottomEdge(mScrollPosition.y + sfPadding.top + sfSize.height -
                          computedOffsets->bottom - rect.height - sfOffset.y);
  }

  uint8_t direction = cbFrame->StyleVisibility()->mDirection;

  
  if (computedOffsets->left != NS_AUTOOFFSET &&
      (computedOffsets->right == NS_AUTOOFFSET ||
       direction == NS_STYLE_DIRECTION_LTR ||
       rect.width <= sfSize.width - computedOffsets->LeftRight())) {
    aStick->SetLeftEdge(mScrollPosition.x + sfPadding.left +
                        computedOffsets->left - sfOffset.x);
  }

  
  if (computedOffsets->right != NS_AUTOOFFSET &&
      (computedOffsets->left == NS_AUTOOFFSET ||
       direction == NS_STYLE_DIRECTION_RTL ||
       rect.width <= sfSize.width - computedOffsets->LeftRight())) {
    aStick->SetRightEdge(mScrollPosition.x + sfPadding.left + sfSize.width -
                         computedOffsets->right - rect.width - sfOffset.x);
  }

  
  
  nsPoint frameOffset = aFrame->GetPosition() - rect.TopLeft();
  aStick->MoveBy(frameOffset);
  aContain->MoveBy(frameOffset);
}

nsPoint
StickyScrollContainer::ComputePosition(nsIFrame* aFrame) const
{
  nsRect stick;
  nsRect contain;
  ComputeStickyLimits(aFrame, &stick, &contain);

  nsPoint position = aFrame->GetNormalPosition();

  
  
  
  position.y = std::max(position.y, std::min(stick.y, contain.YMost()));
  position.y = std::min(position.y, std::max(stick.YMost(), contain.y));
  position.x = std::max(position.x, std::min(stick.x, contain.XMost()));
  position.x = std::min(position.x, std::max(stick.XMost(), contain.x));

  return position;
}

void
StickyScrollContainer::GetScrollRanges(nsIFrame* aFrame, nsRect* aOuter,
                                       nsRect* aInner) const
{
  
  
  
  nsIFrame *firstCont =
    nsLayoutUtils::FirstContinuationOrIBSplitSibling(aFrame);

  nsRect stick;
  nsRect contain;
  ComputeStickyLimits(firstCont, &stick, &contain);

  aOuter->SetRect(nscoord_MIN/2, nscoord_MIN/2, nscoord_MAX, nscoord_MAX);
  aInner->SetRect(nscoord_MIN/2, nscoord_MIN/2, nscoord_MAX, nscoord_MAX);

  const nsPoint normalPosition = aFrame->GetNormalPosition();

  
  if (stick.YMost() != nscoord_MAX/2) {
    aOuter->SetTopEdge(contain.y - stick.YMost());
    aInner->SetTopEdge(normalPosition.y - stick.YMost());
  }

  if (stick.y != nscoord_MIN/2) {
    aInner->SetBottomEdge(normalPosition.y - stick.y);
    aOuter->SetBottomEdge(contain.YMost() - stick.y);
  }

  
  if (stick.XMost() != nscoord_MAX/2) {
    aOuter->SetLeftEdge(contain.x - stick.XMost());
    aInner->SetLeftEdge(normalPosition.x - stick.XMost());
  }

  if (stick.x != nscoord_MIN/2) {
    aInner->SetRightEdge(normalPosition.x - stick.x);
    aOuter->SetRightEdge(contain.XMost() - stick.x);
  }
}

void
StickyScrollContainer::PositionContinuations(nsIFrame* aFrame)
{
  NS_ASSERTION(nsLayoutUtils::IsFirstContinuationOrIBSplitSibling(aFrame),
               "Should be starting from the first continuation");
  nsPoint translation = ComputePosition(aFrame) - aFrame->GetNormalPosition();

  
  for (nsIFrame* cont = aFrame; cont;
       cont = nsLayoutUtils::GetNextContinuationOrIBSplitSibling(cont)) {
    cont->SetPosition(cont->GetNormalPosition() + translation);
  }
}

void
StickyScrollContainer::UpdatePositions(nsPoint aScrollPosition,
                                       nsIFrame* aSubtreeRoot)
{
#ifdef DEBUG
  {
    nsIFrame* scrollFrameAsFrame = do_QueryFrame(mScrollFrame);
    NS_ASSERTION(!aSubtreeRoot || aSubtreeRoot == scrollFrameAsFrame,
                 "If reflowing, should be reflowing the scroll frame");
  }
#endif
  mScrollPosition = aScrollPosition;

  OverflowChangedTracker oct;
  oct.SetSubtreeRoot(aSubtreeRoot);
  for (nsTArray<nsIFrame*>::size_type i = 0; i < mFrames.Length(); i++) {
    nsIFrame* f = mFrames[i];
    if (!nsLayoutUtils::IsFirstContinuationOrIBSplitSibling(f)) {
      
      
      mFrames.RemoveElementAt(i);
      --i;
      continue;
    }

    if (aSubtreeRoot) {
      
      ComputeStickyOffsets(f);
    }
    
    
    PositionContinuations(f);

    f = f->GetParent();
    if (f != aSubtreeRoot) {
      for (nsIFrame* cont = f; cont;
           cont = nsLayoutUtils::GetNextContinuationOrIBSplitSibling(cont)) {
        oct.AddFrame(cont, OverflowChangedTracker::CHILDREN_CHANGED);
      }
    }
  }
  oct.Flush();
}

void
StickyScrollContainer::ScrollPositionWillChange(nscoord aX, nscoord aY)
{
}

void
StickyScrollContainer::ScrollPositionDidChange(nscoord aX, nscoord aY)
{
  UpdatePositions(nsPoint(aX, aY), nullptr);
}

} 
