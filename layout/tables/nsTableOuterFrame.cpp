



#include "nsTableOuterFrame.h"
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
#include <algorithm>

using namespace mozilla;
using namespace mozilla::layout;



#define NS_TABLE_FRAME_CAPTION_LIST_INDEX 1
#define NO_SIDE 100


nsTableCaptionFrame::nsTableCaptionFrame(nsStyleContext* aContext):
  nsBlockFrame(aContext)
{
  
  SetFlags(NS_BLOCK_FLOAT_MGR);
}

nsTableCaptionFrame::~nsTableCaptionFrame()
{
}

nsIAtom*
nsTableCaptionFrame::GetType() const
{
  return nsGkAtoms::tableCaptionFrame;
}

 nscoord
nsTableOuterFrame::GetBaseline() const
{
  nsIFrame* kid = mFrames.FirstChild();
  if (!kid) {
    NS_NOTREACHED("no inner table");
    return nsContainerFrame::GetBaseline();
  }

  return kid->GetBaseline() + kid->GetPosition().y;
}

 nsSize
nsTableCaptionFrame::ComputeAutoSize(nsRenderingContext *aRenderingContext,
                                     nsSize aCBSize, nscoord aAvailableWidth,
                                     nsSize aMargin, nsSize aBorder,
                                     nsSize aPadding, bool aShrinkWrap)
{
  nsSize result = nsBlockFrame::ComputeAutoSize(aRenderingContext, aCBSize,
                    aAvailableWidth, aMargin, aBorder, aPadding, aShrinkWrap);

  
  
  AutoMaybeDisableFontInflation an(this);

  uint8_t captionSide = GetStyleTableBorder()->mCaptionSide;
  if (captionSide == NS_STYLE_CAPTION_SIDE_LEFT ||
      captionSide == NS_STYLE_CAPTION_SIDE_RIGHT) {
    result.width = GetMinWidth(aRenderingContext);
  } else if (captionSide == NS_STYLE_CAPTION_SIDE_TOP ||
             captionSide == NS_STYLE_CAPTION_SIDE_BOTTOM) {
    
    
    
    
    
    nscoord min = GetMinWidth(aRenderingContext);
    if (min > aCBSize.width)
      min = aCBSize.width;
    if (min > result.width)
      result.width = min;
  }
  return result;
}

nsIFrame*
nsTableCaptionFrame::GetParentStyleContextFrame() const
{
  NS_PRECONDITION(mContent->GetParent(),
                  "How could we not have a parent here?");
    
  
  
  nsIFrame* outerFrame = GetParent();
  if (outerFrame && outerFrame->GetType() == nsGkAtoms::tableOuterFrame) {
    nsIFrame* innerFrame = outerFrame->GetFirstPrincipalChild();
    if (innerFrame) {
      return nsFrame::CorrectStyleParentFrame(innerFrame,
                                              StyleContext()->GetPseudo());
    }
  }

  NS_NOTREACHED("Where is our inner table frame?");
  return nsBlockFrame::GetParentStyleContextFrame();
}

#ifdef ACCESSIBILITY
a11y::AccType
nsTableCaptionFrame::AccessibleType()
{
  if (!GetRect().IsEmpty()) {
    return a11y::eHTMLCaptionType;
  }

  return a11y::eNoType;
}
#endif

#ifdef DEBUG
NS_IMETHODIMP
nsTableCaptionFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("Caption"), aResult);
}
#endif

nsIFrame* 
NS_NewTableCaptionFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsTableCaptionFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsTableCaptionFrame)



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

NS_IMETHODIMP 
nsTableOuterFrame::SetInitialChildList(ChildListID     aListID,
                                       nsFrameList&    aChildList)
{
  if (kCaptionList == aListID) {
    
    mCaptionFrames.SetFrames(aChildList);
  }
  else {
    NS_ASSERTION(aListID == kPrincipalList, "wrong childlist");
    NS_ASSERTION(mFrames.IsEmpty(), "Frame leak!");
    NS_ASSERTION(aChildList.FirstChild() &&
                 nsGkAtoms::tableFrame == aChildList.FirstChild()->GetType(),
                 "expected a table frame");
    mFrames.SetFrames(aChildList);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsTableOuterFrame::AppendFrames(ChildListID     aListID,
                                nsFrameList&    aFrameList)
{
  nsresult rv;

  
  
  if (kCaptionList == aListID) {
    NS_ASSERTION(aFrameList.IsEmpty() ||
                 aFrameList.FirstChild()->GetType() == nsGkAtoms::tableCaptionFrame,
                 "appending non-caption frame to captionList");
    mCaptionFrames.AppendFrames(this, aFrameList);
    rv = NS_OK;

    
    
    PresContext()->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                       NS_FRAME_HAS_DIRTY_CHILDREN);
  }
  else {
    NS_PRECONDITION(false, "unexpected child list");
    rv = NS_ERROR_UNEXPECTED;
  }

  return rv;
}

NS_IMETHODIMP
nsTableOuterFrame::InsertFrames(ChildListID     aListID,
                                nsIFrame*       aPrevFrame,
                                nsFrameList&    aFrameList)
{
  if (kCaptionList == aListID) {
    NS_ASSERTION(!aPrevFrame || aPrevFrame->GetParent() == this,
                 "inserting after sibling frame with different parent");
    NS_ASSERTION(aFrameList.IsEmpty() ||
                 aFrameList.FirstChild()->GetType() == nsGkAtoms::tableCaptionFrame,
                 "inserting non-caption frame into captionList");
    mCaptionFrames.InsertFrames(nullptr, aPrevFrame, aFrameList);

    
    
    PresContext()->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                       NS_FRAME_HAS_DIRTY_CHILDREN);
    return NS_OK;
  }
  else {
    NS_PRECONDITION(!aPrevFrame, "invalid previous frame");
    return AppendFrames(aListID, aFrameList);
  }
}

NS_IMETHODIMP
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

  return NS_OK;
}

NS_METHOD 
nsTableOuterFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                    const nsRect&           aDirtyRect,
                                    const nsDisplayListSet& aLists)
{
  
  

  
  
  if (mCaptionFrames.IsEmpty())
    return BuildDisplayListForInnerTable(aBuilder, aDirtyRect, aLists);
    
  nsDisplayListCollection set;
  nsresult rv = BuildDisplayListForInnerTable(aBuilder, aDirtyRect, set);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsDisplayListSet captionSet(set, set.BlockBorderBackgrounds());
  rv = BuildDisplayListForChild(aBuilder, mCaptionFrames.FirstChild(),
                                aDirtyRect, captionSet);
  NS_ENSURE_SUCCESS(rv, rv);
  
  
  
  set.SortAllByContentOrder(aBuilder, GetContent());
  set.MoveTo(aLists);
  return NS_OK;
}

nsresult
nsTableOuterFrame::BuildDisplayListForInnerTable(nsDisplayListBuilder*   aBuilder,
                                                 const nsRect&           aDirtyRect,
                                                 const nsDisplayListSet& aLists)
{
  
  
  nsIFrame* kid = mFrames.FirstChild();
  
  while (kid) {
    nsresult rv = BuildDisplayListForChild(aBuilder, kid, aDirtyRect, aLists);
    NS_ENSURE_SUCCESS(rv, rv);
    kid = kid->GetNextSibling();
  }
  return NS_OK;
}

nsIFrame*
nsTableOuterFrame::GetParentStyleContextFrame() const
{
  
  
  
  
  
  
  
  
  

  return InnerTableFrame();
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
                                  nscoord                  aAvailWidth,
                                  nsMargin&                aMargin)
{
  
  

  
  
  nsHTMLReflowState childRS(aPresContext, aOuterRS, aChildFrame,
                            nsSize(aAvailWidth, aOuterRS.availableHeight),
                            -1, -1, false);
  InitChildReflowState(*aPresContext, childRS);

  aMargin = childRS.mComputedMargin;
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
nsTableOuterFrame::GetMinWidth(nsRenderingContext *aRenderingContext)
{
  nscoord width = nsLayoutUtils::IntrinsicForContainer(aRenderingContext,
                    InnerTableFrame(), nsLayoutUtils::MIN_WIDTH);
  DISPLAY_MIN_WIDTH(this, width);
  if (mCaptionFrames.NotEmpty()) {
    nscoord capWidth =
      nsLayoutUtils::IntrinsicForContainer(aRenderingContext,
                                           mCaptionFrames.FirstChild(),
                                           nsLayoutUtils::MIN_WIDTH);
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
nsTableOuterFrame::GetPrefWidth(nsRenderingContext *aRenderingContext)
{
  nscoord maxWidth;
  DISPLAY_PREF_WIDTH(this, maxWidth);

  maxWidth = nsLayoutUtils::IntrinsicForContainer(aRenderingContext,
               InnerTableFrame(), nsLayoutUtils::PREF_WIDTH);
  if (mCaptionFrames.NotEmpty()) {
    uint8_t captionSide = GetCaptionSide();
    switch(captionSide) {
    case NS_STYLE_CAPTION_SIDE_LEFT:
    case NS_STYLE_CAPTION_SIDE_RIGHT:
      {
        nscoord capMin =
          nsLayoutUtils::IntrinsicForContainer(aRenderingContext,
                                               mCaptionFrames.FirstChild(),
                                               nsLayoutUtils::MIN_WIDTH);
        maxWidth += capMin;
      }
      break;
    default:
      {
        nsLayoutUtils::IntrinsicWidthType iwt;
        if (captionSide == NS_STYLE_CAPTION_SIDE_TOP ||
            captionSide == NS_STYLE_CAPTION_SIDE_BOTTOM) {
          
          
          iwt = nsLayoutUtils::MIN_WIDTH;
        } else {
          NS_ASSERTION(captionSide == NS_STYLE_CAPTION_SIDE_TOP_OUTSIDE ||
                       captionSide == NS_STYLE_CAPTION_SIDE_BOTTOM_OUTSIDE,
                       "unexpected caption side");
          iwt = nsLayoutUtils::PREF_WIDTH;
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
  nsSize size = aChildFrame->ComputeSize(aRenderingContext, aCBSize,
                  aAvailableWidth,
                  nsSize(offsets.mComputedMargin.LeftRight(),
                         offsets.mComputedMargin.TopBottom()),
                  nsSize(offsets.mComputedBorderPadding.LeftRight() -
                           offsets.mComputedPadding.LeftRight(),
                         offsets.mComputedBorderPadding.TopBottom() -
                           offsets.mComputedPadding.TopBottom()),
                  nsSize(offsets.mComputedPadding.LeftRight(),
                         offsets.mComputedPadding.TopBottom()),
                  true);
  if (aMarginResult)
    *aMarginResult = offsets.mComputedMargin.LeftRight();
  return size.width + offsets.mComputedMargin.LeftRight() +
                      offsets.mComputedBorderPadding.LeftRight();
}

 nsSize
nsTableOuterFrame::ComputeAutoSize(nsRenderingContext *aRenderingContext,
                                   nsSize aCBSize, nscoord aAvailableWidth,
                                   nsSize aMargin, nsSize aBorder,
                                   nsSize aPadding, bool aShrinkWrap)
{
  nscoord kidAvailableWidth = aAvailableWidth - aMargin.width;
  NS_ASSERTION(aBorder == nsSize(0, 0) &&
               aPadding == nsSize(0, 0),
               "Table outer frames cannot hae borders or paddings");

  
  
  
  

  
  uint8_t captionSide = GetCaptionSide();
  nscoord width;
  if (captionSide == NO_SIDE) {
    width = ChildShrinkWrapWidth(aRenderingContext, InnerTableFrame(),
                                 aCBSize, kidAvailableWidth);
  } else if (captionSide == NS_STYLE_CAPTION_SIDE_LEFT ||
             captionSide == NS_STYLE_CAPTION_SIDE_RIGHT) {
    nscoord capWidth = ChildShrinkWrapWidth(aRenderingContext,
                                            mCaptionFrames.FirstChild(),
                                            aCBSize, kidAvailableWidth);
    width = capWidth + ChildShrinkWrapWidth(aRenderingContext,
                                            InnerTableFrame(), aCBSize,
                                            kidAvailableWidth - capWidth);
  } else if (captionSide == NS_STYLE_CAPTION_SIDE_TOP ||
             captionSide == NS_STYLE_CAPTION_SIDE_BOTTOM) {
    nscoord margin;
    width = ChildShrinkWrapWidth(aRenderingContext, InnerTableFrame(),
                                 aCBSize, kidAvailableWidth, &margin);
    nscoord capWidth = ChildShrinkWrapWidth(aRenderingContext,
                                            mCaptionFrames.FirstChild(), aCBSize,
                                            width - margin);
    if (capWidth > width)
      width = capWidth;
  } else {
    NS_ASSERTION(captionSide == NS_STYLE_CAPTION_SIDE_TOP_OUTSIDE ||
                 captionSide == NS_STYLE_CAPTION_SIDE_BOTTOM_OUTSIDE,
                 "unexpected caption-side");
    width = ChildShrinkWrapWidth(aRenderingContext, InnerTableFrame(),
                                 aCBSize, kidAvailableWidth);
    nscoord capWidth = ChildShrinkWrapWidth(aRenderingContext,
                                            mCaptionFrames.FirstChild(),
                                            aCBSize, kidAvailableWidth);
    if (capWidth > width)
      width = capWidth;
  }

  return nsSize(width, NS_UNCONSTRAINEDSIZE);
}

uint8_t
nsTableOuterFrame::GetCaptionSide()
{
  if (mCaptionFrames.NotEmpty()) {
    return mCaptionFrames.FirstChild()->GetStyleTableBorder()->mCaptionSide;
  }
  else {
    return NO_SIDE; 
  }
}

uint8_t
nsTableOuterFrame::GetCaptionVerticalAlign()
{
  const nsStyleCoord& va =
    mCaptionFrames.FirstChild()->GetStyleTextReset()->mVerticalAlign;
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
nsTableOuterFrame::OuterBeginReflowChild(nsPresContext*           aPresContext,
                                         nsIFrame*                aChildFrame,
                                         const nsHTMLReflowState& aOuterRS,
                                         void*                    aChildRSSpace,
                                         nscoord                  aAvailWidth)
{ 
  
  nscoord availHeight = aOuterRS.availableHeight;
  if (NS_UNCONSTRAINEDSIZE != availHeight) {
    if (mCaptionFrames.FirstChild() == aChildFrame) {
      availHeight = NS_UNCONSTRAINEDSIZE;
    } else {
      nsMargin margin;
      GetChildMargin(aPresContext, aOuterRS, aChildFrame,
                     aOuterRS.availableWidth, margin);
    
      NS_ASSERTION(NS_UNCONSTRAINEDSIZE != margin.top, "No unconstrainedsize arithmetic, please");
      availHeight -= margin.top;
 
      NS_ASSERTION(NS_UNCONSTRAINEDSIZE != margin.bottom, "No unconstrainedsize arithmetic, please");
      availHeight -= margin.bottom;
    }
  }
  nsSize availSize(aAvailWidth, availHeight);
  
  
  
  nsHTMLReflowState &childRS = * new (aChildRSSpace)
    nsHTMLReflowState(aPresContext, aOuterRS, aChildFrame, availSize,
                      -1, -1, false);
  InitChildReflowState(*aPresContext, childRS);

  
  if (mCaptionFrames.NotEmpty()) {
    uint8_t captionSide = GetCaptionSide();
    if (((captionSide == NS_STYLE_CAPTION_SIDE_BOTTOM ||
          captionSide == NS_STYLE_CAPTION_SIDE_BOTTOM_OUTSIDE) &&
         mCaptionFrames.FirstChild() == aChildFrame) || 
        ((captionSide == NS_STYLE_CAPTION_SIDE_TOP ||
          captionSide == NS_STYLE_CAPTION_SIDE_TOP_OUTSIDE) &&
         InnerTableFrame() == aChildFrame)) {
      childRS.mFlags.mIsTopOfPage = false;
    }
  }
}

nsresult
nsTableOuterFrame::OuterDoReflowChild(nsPresContext*             aPresContext,
                                      nsIFrame*                  aChildFrame,
                                      const nsHTMLReflowState&   aChildRS,
                                      nsHTMLReflowMetrics&       aMetrics,
                                      nsReflowStatus&            aStatus)
{ 

  
  nsPoint childPt = aChildFrame->GetPosition();
  return ReflowChild(aChildFrame, aPresContext, aMetrics, aChildRS,
                     childPt.x, childPt.y, NS_FRAME_NO_MOVE_FRAME, aStatus);
}

void 
nsTableOuterFrame::UpdateReflowMetrics(uint8_t              aCaptionSide,
                                       nsHTMLReflowMetrics& aMet,
                                       const nsMargin&      aInnerMargin,
                                       const nsMargin&      aCaptionMargin)
{
  SetDesiredSize(aCaptionSide, aInnerMargin, aCaptionMargin,
                 aMet.width, aMet.height);

  aMet.SetOverflowAreasToDesiredBounds();
  ConsiderChildOverflow(aMet.mOverflowAreas, InnerTableFrame());
  if (mCaptionFrames.NotEmpty()) {
    ConsiderChildOverflow(aMet.mOverflowAreas, mCaptionFrames.FirstChild());
  }
}

NS_METHOD nsTableOuterFrame::Reflow(nsPresContext*           aPresContext,
                                    nsHTMLReflowMetrics&     aDesiredSize,
                                    const nsHTMLReflowState& aOuterRS,
                                    nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsTableOuterFrame");
  DISPLAY_REFLOW(aPresContext, this, aOuterRS, aDesiredSize, aStatus);

  nsresult rv = NS_OK;
  uint8_t captionSide = GetCaptionSide();

  
  aDesiredSize.width = aDesiredSize.height = 0;
  aStatus = NS_FRAME_COMPLETE;

  if (!(GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
    
    
    MoveOverflowToChildList(aPresContext);
  }

  
  #define LONGS_IN_HTMLRS \
    ((sizeof(nsHTMLReflowState) + sizeof(long) - 1) / sizeof(long))
  long captionRSSpace[LONGS_IN_HTMLRS];
  nsHTMLReflowState *captionRS =
    static_cast<nsHTMLReflowState*>((void*)captionRSSpace);
  long innerRSSpace[LONGS_IN_HTMLRS];
  nsHTMLReflowState *innerRS =
    static_cast<nsHTMLReflowState*>((void*) innerRSSpace);

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
  
  
  if (captionSide == NO_SIDE) {
    
    OuterBeginReflowChild(aPresContext, InnerTableFrame(), aOuterRS,
                          innerRSSpace, aOuterRS.ComputedWidth());
  } else if (captionSide == NS_STYLE_CAPTION_SIDE_LEFT ||
             captionSide == NS_STYLE_CAPTION_SIDE_RIGHT) {
    
    
    
    OuterBeginReflowChild(aPresContext, mCaptionFrames.FirstChild(), aOuterRS,
                          captionRSSpace, aOuterRS.ComputedWidth());
    nscoord innerAvailWidth = aOuterRS.ComputedWidth() -
      (captionRS->ComputedWidth() + captionRS->mComputedMargin.LeftRight() +
       captionRS->mComputedBorderPadding.LeftRight());
    OuterBeginReflowChild(aPresContext, InnerTableFrame(), aOuterRS,
                          innerRSSpace, innerAvailWidth);

  } else if (captionSide == NS_STYLE_CAPTION_SIDE_TOP ||
             captionSide == NS_STYLE_CAPTION_SIDE_BOTTOM) {
    
    
    
    
    
    
    
    
    OuterBeginReflowChild(aPresContext, InnerTableFrame(), aOuterRS,
                          innerRSSpace, aOuterRS.ComputedWidth());
    
    
    
    
    
    nscoord innerBorderWidth = innerRS->ComputedWidth() +
                               innerRS->mComputedBorderPadding.LeftRight();
    OuterBeginReflowChild(aPresContext, mCaptionFrames.FirstChild(), aOuterRS,
                          captionRSSpace, innerBorderWidth);
  } else {
    NS_ASSERTION(captionSide == NS_STYLE_CAPTION_SIDE_TOP_OUTSIDE ||
                 captionSide == NS_STYLE_CAPTION_SIDE_BOTTOM_OUTSIDE,
                 "unexpected caption-side");
    
    OuterBeginReflowChild(aPresContext, mCaptionFrames.FirstChild(), aOuterRS,
                          captionRSSpace, aOuterRS.ComputedWidth());
    OuterBeginReflowChild(aPresContext, InnerTableFrame(), aOuterRS,
                          innerRSSpace, aOuterRS.ComputedWidth());
  }

  
  nsHTMLReflowMetrics captionMet;
  nsSize captionSize;
  nsMargin captionMargin;
  if (mCaptionFrames.NotEmpty()) {
    nsReflowStatus capStatus; 
    rv = OuterDoReflowChild(aPresContext, mCaptionFrames.FirstChild(),
                            *captionRS, captionMet, capStatus);
    if (NS_FAILED(rv)) return rv;
    captionSize.width = captionMet.width;
    captionSize.height = captionMet.height;
    captionMargin = captionRS->mComputedMargin;
    
    
    
    if (NS_UNCONSTRAINEDSIZE != aOuterRS.availableHeight) {
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
      innerRS->availableHeight =
        std::max(0, innerRS->availableHeight - captionHeight);
    }
  } else {
    captionSize.SizeTo(0,0);
    captionMargin.SizeTo(0,0,0,0);
  }

  
  
  nsHTMLReflowMetrics innerMet;
  rv = OuterDoReflowChild(aPresContext, InnerTableFrame(), *innerRS,
                          innerMet, aStatus);
  if (NS_FAILED(rv)) return rv;
  nsSize innerSize;
  innerSize.width = innerMet.width;
  innerSize.height = innerMet.height;
  nsMargin innerMargin = innerRS->mComputedMargin;

  nsSize   containSize = GetContainingBlockSize(aOuterRS);

  
  

  
  

  if (mCaptionFrames.NotEmpty()) {
    nsPoint captionOrigin;
    GetCaptionOrigin(captionSide, containSize, innerSize, 
                     innerMargin, captionSize, captionMargin, captionOrigin);
    FinishReflowChild(mCaptionFrames.FirstChild(), aPresContext, captionRS,
                      captionMet, captionOrigin.x, captionOrigin.y, 0);
    captionRS->~nsHTMLReflowState();
  }
  
  

  nsPoint innerOrigin;
  GetInnerOrigin(captionSide, containSize, captionSize, 
                 captionMargin, innerSize, innerMargin, innerOrigin);
  FinishReflowChild(InnerTableFrame(), aPresContext, innerRS, innerMet,
                    innerOrigin.x, innerOrigin.y, 0);
  innerRS->~nsHTMLReflowState();

  nsTableFrame::InvalidateTableFrame(InnerTableFrame(), origInnerRect,
                                     origInnerVisualOverflow, innerFirstReflow);
  if (mCaptionFrames.NotEmpty()) {
    nsTableFrame::InvalidateTableFrame(mCaptionFrames.FirstChild(), origCaptionRect,
                                       origCaptionVisualOverflow,
                                       captionFirstReflow);
  }

  UpdateReflowMetrics(captionSide, aDesiredSize, innerMargin, captionMargin);
  FinishReflowWithAbsoluteFrames(aPresContext, aDesiredSize, aOuterRS, aStatus);

  

  NS_FRAME_SET_TRUNCATION(aStatus, aOuterRS, aDesiredSize);
  return rv;
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


nsIFrame*
NS_NewTableOuterFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsTableOuterFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsTableOuterFrame)

#ifdef DEBUG
NS_IMETHODIMP
nsTableOuterFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("TableOuter"), aResult);
}
#endif

