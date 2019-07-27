




#include "nsTableOuterFrame.h"

#include "nsFrameManager.h"
#include "nsTableFrame.h"
#include "nsTableCellFrame.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsCSSRendering.h"
#include "nsIContent.h"
#include "prinrval.h"
#include "nsGkAtoms.h"
#include "nsHTMLParts.h"
#include "nsIPresShell.h"
#include "nsIServiceManager.h"
#include "nsIDOMNode.h"
#include "nsDisplayList.h"
#include "nsLayoutUtils.h"
#include "nsIFrameInlines.h"
#include <algorithm>

using namespace mozilla;
using namespace mozilla::layout;

#define NO_SIDE 100

 nscoord
nsTableOuterFrame::GetLogicalBaseline(WritingMode aWritingMode) const
{
  nsIFrame* kid = mFrames.FirstChild();
  if (!kid) {
    NS_NOTREACHED("no inner table");
    return nsContainerFrame::GetLogicalBaseline(aWritingMode);
  }

  return kid->GetLogicalBaseline(aWritingMode) +
         kid->BStart(aWritingMode, mRect.width);
}

nsTableOuterFrame::nsTableOuterFrame(nsStyleContext* aContext):
  nsContainerFrame(aContext)
{
}

nsTableOuterFrame::~nsTableOuterFrame()
{
}

NS_QUERYFRAME_HEAD(nsTableOuterFrame)
  NS_QUERYFRAME_ENTRY(nsTableOuterFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsContainerFrame)

#ifdef ACCESSIBILITY
a11y::AccType
nsTableOuterFrame::AccessibleType()
{
  return a11y::eHTMLTableType;
}
#endif

void
nsTableOuterFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  DestroyAbsoluteFrames(aDestructRoot);
  mCaptionFrames.DestroyFramesFrom(aDestructRoot);
  nsContainerFrame::DestroyFrom(aDestructRoot);
}

const nsFrameList&
nsTableOuterFrame::GetChildList(ChildListID aListID) const
{
  if (aListID == kCaptionList) {
    return mCaptionFrames;
  }

  return nsContainerFrame::GetChildList(aListID);
}

void
nsTableOuterFrame::GetChildLists(nsTArray<ChildList>* aLists) const
{
  nsContainerFrame::GetChildLists(aLists);
  mCaptionFrames.AppendIfNonempty(aLists, kCaptionList);
}

void 
nsTableOuterFrame::SetInitialChildList(ChildListID     aListID,
                                       nsFrameList&    aChildList)
{
  MOZ_ASSERT(kCaptionList == aListID || aListID == kPrincipalList,
             "unexpected child list");
  MOZ_ASSERT(GetChildList(aListID).IsEmpty(),
             "already have child frames in SetInitialChildList");

  if (kCaptionList == aListID) {
    
    mCaptionFrames.SetFrames(aChildList);
  } else {
    MOZ_ASSERT(aChildList.FirstChild() &&
               aChildList.FirstChild() == aChildList.LastChild() &&
               nsGkAtoms::tableFrame == aChildList.FirstChild()->GetType(),
               "expected a single table frame");
    mFrames.SetFrames(aChildList);
  }
}

void
nsTableOuterFrame::AppendFrames(ChildListID     aListID,
                                nsFrameList&    aFrameList)
{
  
  
  MOZ_ASSERT(kCaptionList == aListID, "unexpected child list");
  MOZ_ASSERT(aFrameList.IsEmpty() ||
             aFrameList.FirstChild()->IsTableCaption(),
             "appending non-caption frame to captionList");
  mCaptionFrames.AppendFrames(this, aFrameList);

  
  
  PresContext()->PresShell()->
    FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                     NS_FRAME_HAS_DIRTY_CHILDREN);
}

void
nsTableOuterFrame::InsertFrames(ChildListID     aListID,
                                nsIFrame*       aPrevFrame,
                                nsFrameList&    aFrameList)
{
  MOZ_ASSERT(kCaptionList == aListID, "unexpected child list");
  MOZ_ASSERT(aFrameList.IsEmpty() ||
             aFrameList.FirstChild()->IsTableCaption(),
             "inserting non-caption frame into captionList");
  MOZ_ASSERT(!aPrevFrame || aPrevFrame->GetParent() == this,
             "inserting after sibling frame with different parent");
  mCaptionFrames.InsertFrames(nullptr, aPrevFrame, aFrameList);

  
  
  PresContext()->PresShell()->
    FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                     NS_FRAME_HAS_DIRTY_CHILDREN);
}

void
nsTableOuterFrame::RemoveFrame(ChildListID     aListID,
                               nsIFrame*       aOldFrame)
{
  
  
  NS_PRECONDITION(kCaptionList == aListID, "can't remove inner frame");

  if (HasSideCaption()) {
    
    
    InnerTableFrame()->AddStateBits(NS_FRAME_IS_DIRTY);
  }

  
  mCaptionFrames.DestroyFrame(aOldFrame);
  
  PresContext()->PresShell()->
    FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                     NS_FRAME_HAS_DIRTY_CHILDREN); 
}

void
nsTableOuterFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                    const nsRect&           aDirtyRect,
                                    const nsDisplayListSet& aLists)
{
  
  

  
  
  if (mCaptionFrames.IsEmpty()) {
    BuildDisplayListForInnerTable(aBuilder, aDirtyRect, aLists);
    return;
  }

  nsDisplayListCollection set;
  BuildDisplayListForInnerTable(aBuilder, aDirtyRect, set);
  
  nsDisplayListSet captionSet(set, set.BlockBorderBackgrounds());
  BuildDisplayListForChild(aBuilder, mCaptionFrames.FirstChild(),
                           aDirtyRect, captionSet);
  
  
  
  set.SortAllByContentOrder(aBuilder, GetContent());
  set.MoveTo(aLists);
}

void
nsTableOuterFrame::BuildDisplayListForInnerTable(nsDisplayListBuilder*   aBuilder,
                                                 const nsRect&           aDirtyRect,
                                                 const nsDisplayListSet& aLists)
{
  
  
  nsIFrame* kid = mFrames.FirstChild();
  
  while (kid) {
    BuildDisplayListForChild(aBuilder, kid, aDirtyRect, aLists);
    kid = kid->GetNextSibling();
  }
}

nsStyleContext*
nsTableOuterFrame::GetParentStyleContext(nsIFrame** aProviderFrame) const
{
  
  
  
  
  
  
  
  
  

  return (*aProviderFrame = InnerTableFrame())->StyleContext();
}



void
nsTableOuterFrame::InitChildReflowState(nsPresContext&    aPresContext,                     
                                        nsHTMLReflowState& aReflowState)
                                    
{
  nsMargin collapseBorder;
  nsMargin collapsePadding(0,0,0,0);
  nsMargin* pCollapseBorder  = nullptr;
  nsMargin* pCollapsePadding = nullptr;
  if (aReflowState.frame == InnerTableFrame() &&
      InnerTableFrame()->IsBorderCollapse()) {
    collapseBorder  = InnerTableFrame()->GetIncludedOuterBCBorder();
    pCollapseBorder = &collapseBorder;
    pCollapsePadding = &collapsePadding;
  }
  aReflowState.Init(&aPresContext, -1, -1, pCollapseBorder, pCollapsePadding);
}



void
nsTableOuterFrame::GetChildMargin(nsPresContext*           aPresContext,
                                  const nsHTMLReflowState& aOuterRS,
                                  nsIFrame*                aChildFrame,
                                  nscoord                  aAvailISize,
                                  LogicalMargin&           aMargin)
{
  
  

  
  
  WritingMode wm = aChildFrame->GetWritingMode();
  LogicalSize availSize(wm, aAvailISize, aOuterRS.AvailableSize(wm).BSize(wm));
  nsHTMLReflowState childRS(aPresContext, aOuterRS, aChildFrame, availSize,
                            -1, -1, nsHTMLReflowState::CALLER_WILL_INIT);
  InitChildReflowState(*aPresContext, childRS);

  aMargin = childRS.ComputedLogicalMargin();
}

static nsSize
GetContainingBlockSize(const nsHTMLReflowState& aOuterRS)
{
  nsSize size(0,0);
  const nsHTMLReflowState* containRS =
    aOuterRS.mCBReflowState;

  if (containRS) {
    size.width = containRS->ComputedWidth();
    if (NS_UNCONSTRAINEDSIZE == size.width) {
      size.width = 0;
    }
    size.height = containRS->ComputedHeight();
    if (NS_UNCONSTRAINEDSIZE == size.height) {
      size.height = 0;
    }
  }
  return size;
}

 nscoord
nsTableOuterFrame::GetMinISize(nsRenderingContext *aRenderingContext)
{
  nscoord width = nsLayoutUtils::IntrinsicForContainer(aRenderingContext,
                    InnerTableFrame(), nsLayoutUtils::MIN_ISIZE);
  DISPLAY_MIN_WIDTH(this, width);
  if (mCaptionFrames.NotEmpty()) {
    nscoord capWidth =
      nsLayoutUtils::IntrinsicForContainer(aRenderingContext,
                                           mCaptionFrames.FirstChild(),
                                           nsLayoutUtils::MIN_ISIZE);
    if (HasSideCaption()) {
      width += capWidth;
    } else {
      if (capWidth > width) {
        width = capWidth;
      }
    }
  }
  return width;
}

 nscoord
nsTableOuterFrame::GetPrefISize(nsRenderingContext *aRenderingContext)
{
  nscoord maxWidth;
  DISPLAY_PREF_WIDTH(this, maxWidth);

  maxWidth = nsLayoutUtils::IntrinsicForContainer(aRenderingContext,
               InnerTableFrame(), nsLayoutUtils::PREF_ISIZE);
  if (mCaptionFrames.NotEmpty()) {
    uint8_t captionSide = GetCaptionSide();
    switch(captionSide) {
    case NS_STYLE_CAPTION_SIDE_LEFT:
    case NS_STYLE_CAPTION_SIDE_RIGHT:
      {
        nscoord capMin =
          nsLayoutUtils::IntrinsicForContainer(aRenderingContext,
                                               mCaptionFrames.FirstChild(),
                                               nsLayoutUtils::MIN_ISIZE);
        maxWidth += capMin;
      }
      break;
    default:
      {
        nsLayoutUtils::IntrinsicISizeType iwt;
        if (captionSide == NS_STYLE_CAPTION_SIDE_TOP ||
            captionSide == NS_STYLE_CAPTION_SIDE_BOTTOM) {
          
          
          iwt = nsLayoutUtils::MIN_ISIZE;
        } else {
          NS_ASSERTION(captionSide == NS_STYLE_CAPTION_SIDE_TOP_OUTSIDE ||
                       captionSide == NS_STYLE_CAPTION_SIDE_BOTTOM_OUTSIDE,
                       "unexpected caption side");
          iwt = nsLayoutUtils::PREF_ISIZE;
        }
        nscoord capPref =
          nsLayoutUtils::IntrinsicForContainer(aRenderingContext,
                                               mCaptionFrames.FirstChild(),
                                               iwt);
        maxWidth = std::max(maxWidth, capPref);
      }
      break;
    }
  }
  return maxWidth;
}




static nscoord
ChildShrinkWrapWidth(nsRenderingContext *aRenderingContext,
                     nsIFrame *aChildFrame,
                     nsSize aCBSize, nscoord aAvailableWidth,
                     nscoord *aMarginResult = nullptr)
{
  AutoMaybeDisableFontInflation an(aChildFrame);

  nsCSSOffsetState offsets(aChildFrame, aRenderingContext, aCBSize.width);
  WritingMode wm = offsets.GetWritingMode();
  LogicalSize size =
    aChildFrame->ComputeSize(aRenderingContext,
                  wm, LogicalSize(wm, aCBSize),
                  aAvailableWidth,
                  offsets.ComputedLogicalMargin().Size(wm),
                  offsets.ComputedLogicalBorderPadding().Size(wm) -
                    offsets.ComputedLogicalPadding().Size(wm),
                  offsets.ComputedLogicalPadding().Size(wm),
                  nsIFrame::ComputeSizeFlags::eShrinkWrap);
  if (aMarginResult)
    *aMarginResult = offsets.ComputedLogicalMargin().IStartEnd(wm);
  return size.ISize(wm) + offsets.ComputedLogicalMargin().IStartEnd(wm) +
                      offsets.ComputedLogicalBorderPadding().IStartEnd(wm);
}


LogicalSize
nsTableOuterFrame::ComputeAutoSize(nsRenderingContext *aRenderingContext,
                                   WritingMode aWM,
                                   const LogicalSize& aCBSize,
                                   nscoord aAvailableISize,
                                   const LogicalSize& aMargin,
                                   const LogicalSize& aBorder,
                                   const LogicalSize& aPadding,
                                   bool aShrinkWrap)
{
  nscoord kidAvailableWidth = aAvailableISize - aMargin.ISize(aWM);
  NS_ASSERTION(aBorder.IsAllZero() && aPadding.IsAllZero(),
               "Table outer frames cannot have borders or paddings");

  
  
  
  

  
  

  
  uint8_t captionSide = GetCaptionSide();
  nscoord width;
  if (captionSide == NO_SIDE) {
    width = ChildShrinkWrapWidth(aRenderingContext, InnerTableFrame(),
                                 aCBSize.GetPhysicalSize(aWM),
                                 kidAvailableWidth);
  } else if (captionSide == NS_STYLE_CAPTION_SIDE_LEFT ||
             captionSide == NS_STYLE_CAPTION_SIDE_RIGHT) {
    nscoord capWidth = ChildShrinkWrapWidth(aRenderingContext,
                                            mCaptionFrames.FirstChild(),
                                            aCBSize.GetPhysicalSize(aWM),
                                            kidAvailableWidth);
    width = capWidth + ChildShrinkWrapWidth(aRenderingContext,
                                            InnerTableFrame(),
                                            aCBSize.GetPhysicalSize(aWM),
                                            kidAvailableWidth - capWidth);
  } else if (captionSide == NS_STYLE_CAPTION_SIDE_TOP ||
             captionSide == NS_STYLE_CAPTION_SIDE_BOTTOM) {
    nscoord margin;
    width = ChildShrinkWrapWidth(aRenderingContext, InnerTableFrame(),
                                 aCBSize.GetPhysicalSize(aWM),
                                 kidAvailableWidth, &margin);
    nscoord capWidth = ChildShrinkWrapWidth(aRenderingContext,
                                            mCaptionFrames.FirstChild(),
                                            aCBSize.GetPhysicalSize(aWM),
                                            width - margin);
    if (capWidth > width)
      width = capWidth;
  } else {
    NS_ASSERTION(captionSide == NS_STYLE_CAPTION_SIDE_TOP_OUTSIDE ||
                 captionSide == NS_STYLE_CAPTION_SIDE_BOTTOM_OUTSIDE,
                 "unexpected caption-side");
    width = ChildShrinkWrapWidth(aRenderingContext, InnerTableFrame(),
                                 aCBSize.GetPhysicalSize(aWM),
                                 kidAvailableWidth);
    nscoord capWidth = ChildShrinkWrapWidth(aRenderingContext,
                                            mCaptionFrames.FirstChild(),
                                            aCBSize.GetPhysicalSize(aWM),
                                            kidAvailableWidth);
    if (capWidth > width)
      width = capWidth;
  }

  return LogicalSize(aWM, width, NS_UNCONSTRAINEDSIZE);
}

uint8_t
nsTableOuterFrame::GetCaptionSide()
{
  if (mCaptionFrames.NotEmpty()) {
    return mCaptionFrames.FirstChild()->StyleTableBorder()->mCaptionSide;
  }
  else {
    return NO_SIDE; 
  }
}

uint8_t
nsTableOuterFrame::GetCaptionVerticalAlign()
{
  const nsStyleCoord& va =
    mCaptionFrames.FirstChild()->StyleTextReset()->mVerticalAlign;
  return (va.GetUnit() == eStyleUnit_Enumerated)
           ? va.GetIntValue()
           : NS_STYLE_VERTICAL_ALIGN_TOP;
}

void
nsTableOuterFrame::SetDesiredSize(uint8_t         aCaptionSide,
                                  const nsMargin& aInnerMargin,
                                  const nsMargin& aCaptionMargin,
                                  nscoord&        aWidth,
                                  nscoord&        aHeight)
{
  aWidth = aHeight = 0;

  nsRect innerRect = InnerTableFrame()->GetRect();
  nscoord innerWidth = innerRect.width;

  nsRect captionRect(0,0,0,0);
  nscoord captionWidth = 0;
  if (mCaptionFrames.NotEmpty()) {
    captionRect = mCaptionFrames.FirstChild()->GetRect();
    captionWidth = captionRect.width;
  }
  switch(aCaptionSide) {
    case NS_STYLE_CAPTION_SIDE_LEFT:
      aWidth = std::max(aInnerMargin.left, aCaptionMargin.left + captionWidth + aCaptionMargin.right) +
               innerWidth + aInnerMargin.right;
      break;
    case NS_STYLE_CAPTION_SIDE_RIGHT:
      aWidth = std::max(aInnerMargin.right, aCaptionMargin.left + captionWidth + aCaptionMargin.right) +
               innerWidth + aInnerMargin.left;
      break;
    default:
      aWidth = aInnerMargin.left + innerWidth + aInnerMargin.right;
      aWidth = std::max(aWidth, captionRect.XMost() + aCaptionMargin.right);
  }
  aHeight = innerRect.YMost() + aInnerMargin.bottom;
  if (NS_STYLE_CAPTION_SIDE_BOTTOM != aCaptionSide) {
    aHeight = std::max(aHeight, captionRect.YMost() + aCaptionMargin.bottom);
  }
  else {
    aHeight = std::max(aHeight, captionRect.YMost() + aCaptionMargin.bottom +
                              aInnerMargin.bottom);
  }

}

nsresult 
nsTableOuterFrame::GetCaptionOrigin(uint32_t         aCaptionSide,
                                    const nsSize&    aContainBlockSize,
                                    const nsSize&    aInnerSize, 
                                    const nsMargin&  aInnerMargin,
                                    const nsSize&    aCaptionSize,
                                    nsMargin&        aCaptionMargin,
                                    nsPoint&         aOrigin)
{
  aOrigin.x = aOrigin.y = 0;
  if ((NS_UNCONSTRAINEDSIZE == aInnerSize.width) || (NS_UNCONSTRAINEDSIZE == aInnerSize.height) ||  
      (NS_UNCONSTRAINEDSIZE == aCaptionSize.width) || (NS_UNCONSTRAINEDSIZE == aCaptionSize.height)) {
    return NS_OK;
  }
  if (mCaptionFrames.IsEmpty()) return NS_OK;
  
  NS_ASSERTION(NS_AUTOMARGIN != aCaptionMargin.left,   "The computed caption margin is auto?");
  NS_ASSERTION(NS_AUTOMARGIN != aCaptionMargin.top,    "The computed caption margin is auto?");
  NS_ASSERTION(NS_AUTOMARGIN != aCaptionMargin.bottom, "The computed caption margin is auto?");

  
  switch(aCaptionSide) {
  case NS_STYLE_CAPTION_SIDE_BOTTOM:
  case NS_STYLE_CAPTION_SIDE_BOTTOM_OUTSIDE: {
    
    
    aOrigin.x = aCaptionMargin.left;
    if (aCaptionSide == NS_STYLE_CAPTION_SIDE_BOTTOM) {
      
      
      aOrigin.x += aInnerMargin.left;
    }
  } break;
  case NS_STYLE_CAPTION_SIDE_LEFT: {
    aOrigin.x = aCaptionMargin.left;
  } break;
  case NS_STYLE_CAPTION_SIDE_RIGHT: {
    aOrigin.x = aInnerMargin.left + aInnerSize.width + aCaptionMargin.left;
  } break;
  default: { 
    NS_ASSERTION(aCaptionSide == NS_STYLE_CAPTION_SIDE_TOP ||
                 aCaptionSide == NS_STYLE_CAPTION_SIDE_TOP_OUTSIDE,
                 "unexpected caption side");
    
    
    aOrigin.x = aCaptionMargin.left;
    if (aCaptionSide == NS_STYLE_CAPTION_SIDE_TOP) {
      
      
      aOrigin.x += aInnerMargin.left;
    }
    
  } break;
  }
  
  switch (aCaptionSide) {
    case NS_STYLE_CAPTION_SIDE_RIGHT:
    case NS_STYLE_CAPTION_SIDE_LEFT:
      aOrigin.y = aInnerMargin.top;
      switch (GetCaptionVerticalAlign()) {
        case NS_STYLE_VERTICAL_ALIGN_MIDDLE:
          aOrigin.y = std::max(0, aInnerMargin.top + ((aInnerSize.height - aCaptionSize.height) / 2));
          break;
        case NS_STYLE_VERTICAL_ALIGN_BOTTOM:
          aOrigin.y = std::max(0, aInnerMargin.top + aInnerSize.height - aCaptionSize.height);
          break;
        default:
          break;
      }
      break;
    case NS_STYLE_CAPTION_SIDE_BOTTOM_OUTSIDE:
    case NS_STYLE_CAPTION_SIDE_BOTTOM: {
      aOrigin.y = aInnerMargin.top + aInnerSize.height + aCaptionMargin.top;
    } break;
    case NS_STYLE_CAPTION_SIDE_TOP_OUTSIDE:
    case NS_STYLE_CAPTION_SIDE_TOP: {
      aOrigin.y = aInnerMargin.top + aCaptionMargin.top;
    } break;
    default:
      NS_NOTREACHED("Unknown caption alignment type");
      break;
  }
  return NS_OK;
}

nsresult 
nsTableOuterFrame::GetInnerOrigin(uint32_t         aCaptionSide,
                                  const nsSize&    aContainBlockSize,
                                  const nsSize&    aCaptionSize, 
                                  const nsMargin&  aCaptionMargin,
                                  const nsSize&    aInnerSize,
                                  nsMargin&        aInnerMargin,
                                  nsPoint&         aOrigin)
{
  
  NS_ASSERTION(NS_AUTOMARGIN != aCaptionMargin.left,  "The computed caption margin is auto?");
  NS_ASSERTION(NS_AUTOMARGIN != aCaptionMargin.right, "The computed caption margin is auto?");
  NS_ASSERTION(NS_AUTOMARGIN != aInnerMargin.left,    "The computed inner margin is auto?");
  NS_ASSERTION(NS_AUTOMARGIN != aInnerMargin.right,   "The computed inner margin is auto?");
  NS_ASSERTION(NS_AUTOMARGIN != aInnerMargin.top,     "The computed inner margin is auto?");
  NS_ASSERTION(NS_AUTOMARGIN != aInnerMargin.bottom,  "The computed inner margin is auto?");
  
  aOrigin.x = aOrigin.y = 0;
  if ((NS_UNCONSTRAINEDSIZE == aInnerSize.width) || (NS_UNCONSTRAINEDSIZE == aInnerSize.height) ||  
      (NS_UNCONSTRAINEDSIZE == aCaptionSize.width) || (NS_UNCONSTRAINEDSIZE == aCaptionSize.height)) {
    return NS_OK;
  }

  nscoord minCapWidth = aCaptionSize.width;
  
  minCapWidth += aCaptionMargin.left;
  minCapWidth += aCaptionMargin.right;

  
  switch (aCaptionSide) {
  case NS_STYLE_CAPTION_SIDE_LEFT: {
    if (aInnerMargin.left < minCapWidth) {
      
      aInnerMargin.right += aInnerMargin.left - minCapWidth;
      aInnerMargin.right  = std::max(0, aInnerMargin.right);
      aInnerMargin.left   = minCapWidth;
    }
    aOrigin.x = aInnerMargin.left;
  } break;
  default: {
    NS_ASSERTION(aCaptionSide == NS_STYLE_CAPTION_SIDE_TOP ||
                 aCaptionSide == NS_STYLE_CAPTION_SIDE_TOP_OUTSIDE ||
                 aCaptionSide == NS_STYLE_CAPTION_SIDE_BOTTOM ||
                 aCaptionSide == NS_STYLE_CAPTION_SIDE_BOTTOM_OUTSIDE ||
                 aCaptionSide == NS_STYLE_CAPTION_SIDE_RIGHT ||
                 aCaptionSide == NO_SIDE,
                 "unexpected caption side");
    aOrigin.x = aInnerMargin.left;
  } break;
  }
  
  
  switch (aCaptionSide) {
    case NS_STYLE_CAPTION_SIDE_BOTTOM:
    case NS_STYLE_CAPTION_SIDE_BOTTOM_OUTSIDE: {
      aOrigin.y = aInnerMargin.top;
    } break;
    case NS_STYLE_CAPTION_SIDE_LEFT:
    case NS_STYLE_CAPTION_SIDE_RIGHT: {
      aOrigin.y = aInnerMargin.top;
      switch (GetCaptionVerticalAlign()) {
        case NS_STYLE_VERTICAL_ALIGN_MIDDLE:
          aOrigin.y = std::max(aInnerMargin.top, (aCaptionSize.height - aInnerSize.height) / 2);
          break;
        case NS_STYLE_VERTICAL_ALIGN_BOTTOM:
          aOrigin.y = std::max(aInnerMargin.top, aCaptionSize.height - aInnerSize.height);
          break;
        default:
          break;
      }
    } break;
    case NO_SIDE:
    case NS_STYLE_CAPTION_SIDE_TOP_OUTSIDE:
    case NS_STYLE_CAPTION_SIDE_TOP: {
      aOrigin.y = aInnerMargin.top + aCaptionMargin.top + aCaptionSize.height +
                  aCaptionMargin.bottom;
    } break;
    default:
      NS_NOTREACHED("Unknown caption alignment type");
      break;
  }
  return NS_OK;
}

void
nsTableOuterFrame::OuterBeginReflowChild(nsPresContext*            aPresContext,
                                         nsIFrame*                 aChildFrame,
                                         const nsHTMLReflowState&  aOuterRS,
                                         Maybe<nsHTMLReflowState>& aChildRS,
                                         nscoord                   aAvailISize)
{
  
  WritingMode wm = aChildFrame->GetWritingMode();
  LogicalSize outerSize = aOuterRS.AvailableSize(wm);
  nscoord availBSize = outerSize.BSize(wm);
  if (NS_UNCONSTRAINEDSIZE != availBSize) {
    if (mCaptionFrames.FirstChild() == aChildFrame) {
      availBSize = NS_UNCONSTRAINEDSIZE;
    } else {
      LogicalMargin margin(wm);
      GetChildMargin(aPresContext, aOuterRS, aChildFrame,
                     outerSize.ISize(wm), margin);

      NS_ASSERTION(NS_UNCONSTRAINEDSIZE != margin.BStart(wm),
                   "No unconstrainedsize arithmetic, please");
      availBSize -= margin.BStart(wm);

      NS_ASSERTION(NS_UNCONSTRAINEDSIZE != margin.BEnd(wm),
                   "No unconstrainedsize arithmetic, please");
      availBSize -= margin.BEnd(wm);
    }
  }
  LogicalSize availSize(wm, aAvailISize, availBSize);
  
  
  aChildRS.emplace(aPresContext, aOuterRS, aChildFrame, availSize,
                  -1, -1, nsHTMLReflowState::CALLER_WILL_INIT);
  InitChildReflowState(*aPresContext, *aChildRS);

  
  if (aChildRS->mFlags.mIsTopOfPage &&
      mCaptionFrames.FirstChild() == aChildFrame) {
    uint8_t captionSide = GetCaptionSide();
    if (captionSide == NS_STYLE_CAPTION_SIDE_BOTTOM ||
        captionSide == NS_STYLE_CAPTION_SIDE_BOTTOM_OUTSIDE) {
      aChildRS->mFlags.mIsTopOfPage = false;
    }
  }
}

void
nsTableOuterFrame::OuterDoReflowChild(nsPresContext*             aPresContext,
                                      nsIFrame*                  aChildFrame,
                                      const nsHTMLReflowState&   aChildRS,
                                      nsHTMLReflowMetrics&       aMetrics,
                                      nsReflowStatus&            aStatus)
{ 

  
  nsPoint childPt = aChildFrame->GetPosition();
  uint32_t flags = NS_FRAME_NO_MOVE_FRAME;

  
  
  
  
  
  if (aChildFrame == InnerTableFrame()) {
    flags |= NS_FRAME_NO_DELETE_NEXT_IN_FLOW_CHILD;
  }

  ReflowChild(aChildFrame, aPresContext, aMetrics, aChildRS,
              childPt.x, childPt.y, flags, aStatus);
}

void 
nsTableOuterFrame::UpdateReflowMetrics(uint8_t              aCaptionSide,
                                       nsHTMLReflowMetrics& aMet,
                                       const nsMargin&      aInnerMargin,
                                       const nsMargin&      aCaptionMargin)
{
  SetDesiredSize(aCaptionSide, aInnerMargin, aCaptionMargin,
                 aMet.Width(), aMet.Height());

  aMet.SetOverflowAreasToDesiredBounds();
  ConsiderChildOverflow(aMet.mOverflowAreas, InnerTableFrame());
  if (mCaptionFrames.NotEmpty()) {
    ConsiderChildOverflow(aMet.mOverflowAreas, mCaptionFrames.FirstChild());
  }
}

void
nsTableOuterFrame::Reflow(nsPresContext*           aPresContext,
                                    nsHTMLReflowMetrics&     aDesiredSize,
                                    const nsHTMLReflowState& aOuterRS,
                                    nsReflowStatus&          aStatus)
{
  MarkInReflow();
  DO_GLOBAL_REFLOW_COUNT("nsTableOuterFrame");
  DISPLAY_REFLOW(aPresContext, this, aOuterRS, aDesiredSize, aStatus);

  uint8_t captionSide = GetCaptionSide();

  
  aDesiredSize.ClearSize();
  aStatus = NS_FRAME_COMPLETE;

  if (!(GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
    
    
    MoveOverflowToChildList();
  }

  Maybe<nsHTMLReflowState> captionRS;
  Maybe<nsHTMLReflowState> innerRS;

  nsRect origInnerRect = InnerTableFrame()->GetRect();
  nsRect origInnerVisualOverflow = InnerTableFrame()->GetVisualOverflowRect();
  bool innerFirstReflow =
    (InnerTableFrame()->GetStateBits() & NS_FRAME_FIRST_REFLOW) != 0;
  nsRect origCaptionRect;
  nsRect origCaptionVisualOverflow;
  bool captionFirstReflow;
  if (mCaptionFrames.NotEmpty()) {
    origCaptionRect = mCaptionFrames.FirstChild()->GetRect();
    origCaptionVisualOverflow =
      mCaptionFrames.FirstChild()->GetVisualOverflowRect();
    captionFirstReflow =
      (mCaptionFrames.FirstChild()->GetStateBits() & NS_FRAME_FIRST_REFLOW) != 0;
  }
  
  
  WritingMode wm;
  if (captionSide == NO_SIDE) {
    
    wm = InnerTableFrame()->GetWritingMode();
    OuterBeginReflowChild(aPresContext, InnerTableFrame(), aOuterRS,
                          innerRS, aOuterRS.ComputedSize(wm).ISize(wm));
  } else if (captionSide == NS_STYLE_CAPTION_SIDE_LEFT ||
             captionSide == NS_STYLE_CAPTION_SIDE_RIGHT) {
    
    
    wm = mCaptionFrames.FirstChild()->GetWritingMode();
    OuterBeginReflowChild(aPresContext, mCaptionFrames.FirstChild(), aOuterRS,
                          captionRS, aOuterRS.ComputedSize(wm).ISize(wm));
    nscoord innerAvailISize = aOuterRS.ComputedSize(wm).ISize(wm) -
      captionRS->ComputedSizeWithMarginBorderPadding(wm).ISize(wm);
    OuterBeginReflowChild(aPresContext, InnerTableFrame(), aOuterRS,
                          innerRS, innerAvailISize);

  } else if (captionSide == NS_STYLE_CAPTION_SIDE_TOP ||
             captionSide == NS_STYLE_CAPTION_SIDE_BOTTOM) {
    
    
    
    
    
    
    
    
    wm = InnerTableFrame()->GetWritingMode();
    OuterBeginReflowChild(aPresContext, InnerTableFrame(), aOuterRS,
                          innerRS, aOuterRS.ComputedSize(wm).ISize(wm));
    
    
    
    
    
    nscoord innerBorderWidth =
      innerRS->ComputedSizeWithBorderPadding(wm).ISize(wm);
    OuterBeginReflowChild(aPresContext, mCaptionFrames.FirstChild(), aOuterRS,
                          captionRS, innerBorderWidth);
  } else {
    NS_ASSERTION(captionSide == NS_STYLE_CAPTION_SIDE_TOP_OUTSIDE ||
                 captionSide == NS_STYLE_CAPTION_SIDE_BOTTOM_OUTSIDE,
                 "unexpected caption-side");
    
    wm = mCaptionFrames.FirstChild()->GetWritingMode();
    OuterBeginReflowChild(aPresContext, mCaptionFrames.FirstChild(), aOuterRS,
                          captionRS, aOuterRS.ComputedSize(wm).ISize(wm));
    wm = InnerTableFrame()->GetWritingMode();
    OuterBeginReflowChild(aPresContext, InnerTableFrame(), aOuterRS,
                          innerRS, aOuterRS.ComputedSize(wm).ISize(wm));
  }

  
  
  if (!(GetStateBits() & NS_FRAME_FIRST_REFLOW) &&
      aPresContext->IsPaginated() &&
      GetNextInFlow()) {
    nsIFrame* nif = GetNextInFlow();
    bool oc = nif->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER;
    NS_MergeReflowStatusInto(&aStatus,
        oc ? NS_FRAME_OVERFLOW_INCOMPLETE : NS_FRAME_NOT_COMPLETE);
    nsMargin innerMargin = innerRS->ComputedPhysicalMargin();
    nsMargin captionMargin;
    if (mCaptionFrames.NotEmpty()) {
      captionMargin = captionRS->ComputedPhysicalMargin();
    }
    UpdateReflowMetrics(captionSide, aDesiredSize, innerMargin, captionMargin);
    FinishAndStoreOverflow(&aDesiredSize);
    return;
  }

  
  Maybe<nsHTMLReflowMetrics> captionMet;
  nsSize captionSize;
  nsMargin captionMargin;
  if (mCaptionFrames.NotEmpty()) {
    captionMet.emplace(captionRS->GetWritingMode());
    nsReflowStatus capStatus; 
    OuterDoReflowChild(aPresContext, mCaptionFrames.FirstChild(),
                       *captionRS, *captionMet, capStatus);
    captionSize.width = captionMet->Width();
    captionSize.height = captionMet->Height();
    captionMargin = captionRS->ComputedPhysicalMargin();
    
    
    
    if (NS_UNCONSTRAINEDSIZE != aOuterRS.AvailableHeight()) {
      nscoord captionHeight = 0;
      switch (captionSide) {
        case NS_STYLE_CAPTION_SIDE_TOP:
        case NS_STYLE_CAPTION_SIDE_BOTTOM:
        case NS_STYLE_CAPTION_SIDE_TOP_OUTSIDE:
        case NS_STYLE_CAPTION_SIDE_BOTTOM_OUTSIDE: {
          captionHeight = captionSize.height + captionMargin.TopBottom();
          break;
        }
      }
      innerRS->AvailableHeight() =
        std::max(0, innerRS->AvailableHeight() - captionHeight);
    }
  } else {
    captionSize.SizeTo(0,0);
    captionMargin.SizeTo(0,0,0,0);
  }

  
  
  nsHTMLReflowMetrics innerMet(innerRS->GetWritingMode());
  OuterDoReflowChild(aPresContext, InnerTableFrame(), *innerRS,
                     innerMet, aStatus);
  nsSize innerSize;
  innerSize.width = innerMet.Width();
  innerSize.height = innerMet.Height();
  nsMargin innerMargin = innerRS->ComputedPhysicalMargin();

  nsSize   containSize = GetContainingBlockSize(aOuterRS);

  
  

  
  

  if (mCaptionFrames.NotEmpty()) {
    nsPoint captionOrigin;
    GetCaptionOrigin(captionSide, containSize, innerSize, 
                     innerMargin, captionSize, captionMargin, captionOrigin);
    FinishReflowChild(mCaptionFrames.FirstChild(), aPresContext, *captionMet,
                      captionRS.ptr(), captionOrigin.x, captionOrigin.y, 0);
    captionRS.reset();
  }
  
  

  nsPoint innerOrigin;
  GetInnerOrigin(captionSide, containSize, captionSize, 
                 captionMargin, innerSize, innerMargin, innerOrigin);
  FinishReflowChild(InnerTableFrame(), aPresContext, innerMet, innerRS.ptr(),
                    innerOrigin.x, innerOrigin.y, 0);
  innerRS.reset();

  nsTableFrame::InvalidateTableFrame(InnerTableFrame(), origInnerRect,
                                     origInnerVisualOverflow, innerFirstReflow);
  if (mCaptionFrames.NotEmpty()) {
    nsTableFrame::InvalidateTableFrame(mCaptionFrames.FirstChild(), origCaptionRect,
                                       origCaptionVisualOverflow,
                                       captionFirstReflow);
  }

  UpdateReflowMetrics(captionSide, aDesiredSize, innerMargin, captionMargin);

  if (GetPrevInFlow()) {
    ReflowOverflowContainerChildren(aPresContext, aOuterRS,
                                    aDesiredSize.mOverflowAreas, 0,
                                    aStatus);
  }

  FinishReflowWithAbsoluteFrames(aPresContext, aDesiredSize, aOuterRS, aStatus);

  

  NS_FRAME_SET_TRUNCATION(aStatus, aOuterRS, aDesiredSize);
}

nsIAtom*
nsTableOuterFrame::GetType() const
{
  return nsGkAtoms::tableOuterFrame;
}



nsIContent*
nsTableOuterFrame::GetCellAt(uint32_t aRowIdx, uint32_t aColIdx) const
{
  nsTableCellMap* cellMap = InnerTableFrame()->GetCellMap();
  if (!cellMap) {
    return nullptr;
  }

  nsTableCellFrame* cell = cellMap->GetCellInfoAt(aRowIdx, aColIdx);
  if (!cell) {
    return nullptr;
  }

  return cell->GetContent();
}


nsTableOuterFrame*
NS_NewTableOuterFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsTableOuterFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsTableOuterFrame)

#ifdef DEBUG_FRAME_DUMP
nsresult
nsTableOuterFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("TableOuter"), aResult);
}
#endif

