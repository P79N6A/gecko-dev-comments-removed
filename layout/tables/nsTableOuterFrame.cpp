




































#include "nsTableOuterFrame.h"
#include "nsTableFrame.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsIRenderingContext.h"
#include "nsCSSRendering.h"
#include "nsIContent.h"
#include "prinrval.h"
#include "nsGkAtoms.h"
#include "nsHTMLParts.h"
#include "nsIPresShell.h"
#ifdef ACCESSIBILITY
#include "nsIAccessibilityService.h"
#endif
#include "nsIServiceManager.h"
#include "nsIDOMNode.h"
#include "nsDisplayList.h"
#include "nsLayoutUtils.h"



#define NS_TABLE_FRAME_CAPTION_LIST_INDEX 0
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
    return nsHTMLContainerFrame::GetBaseline();
  }

  return kid->GetBaseline() + kid->GetPosition().y;
}

 nsSize
nsTableCaptionFrame::ComputeAutoSize(nsIRenderingContext *aRenderingContext,
                                     nsSize aCBSize, nscoord aAvailableWidth,
                                     nsSize aMargin, nsSize aBorder,
                                     nsSize aPadding, PRBool aShrinkWrap)
{
  nsSize result = nsBlockFrame::ComputeAutoSize(aRenderingContext, aCBSize,
                    aAvailableWidth, aMargin, aBorder, aPadding, aShrinkWrap);
  PRUint8 captionSide = GetStyleTableBorder()->mCaptionSide;
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

NS_IMETHODIMP 
nsTableCaptionFrame::GetParentStyleContextFrame(nsPresContext* aPresContext,
                                                nsIFrame**      aProviderFrame,
                                                PRBool*         aIsChild)
{
  NS_PRECONDITION(mContent->GetParent(),
                  "How could we not have a parent here?");
    
  
  
  nsIFrame* outerFrame = GetParent();
  if (outerFrame && outerFrame->GetType() == nsGkAtoms::tableOuterFrame) {
    nsIFrame* innerFrame = outerFrame->GetFirstChild(nsnull);
    if (innerFrame) {
      *aProviderFrame =
        nsFrame::CorrectStyleParentFrame(innerFrame,
                                         GetStyleContext()->GetPseudo());
      *aIsChild = PR_FALSE;
      return NS_OK;
    }
  }

  NS_NOTREACHED("Where is our inner table frame?");
  return nsBlockFrame::GetParentStyleContextFrame(aPresContext, aProviderFrame,
                                                  aIsChild);
}

#ifdef ACCESSIBILITY
NS_IMETHODIMP nsTableCaptionFrame::GetAccessible(nsIAccessible** aAccessible)
{
  *aAccessible = nsnull;
  if (!GetRect().IsEmpty()) {
    nsCOMPtr<nsIAccessibilityService> accService = do_GetService("@mozilla.org/accessibilityService;1");
    if (accService) {
      return accService->CreateHTMLCaptionAccessible(static_cast<nsIFrame*>(this), aAccessible);
    }
  }

  return NS_ERROR_FAILURE;
}
#endif

#ifdef NS_DEBUG
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
  nsHTMLContainerFrame(aContext)
{
}

nsTableOuterFrame::~nsTableOuterFrame()
{
}

NS_QUERYFRAME_HEAD(nsTableOuterFrame)
  NS_QUERYFRAME_ENTRY(nsITableLayout)
NS_QUERYFRAME_TAIL_INHERITING(nsHTMLContainerFrame)

#ifdef ACCESSIBILITY
NS_IMETHODIMP nsTableOuterFrame::GetAccessible(nsIAccessible** aAccessible)
{
  nsCOMPtr<nsIAccessibilityService> accService = do_GetService("@mozilla.org/accessibilityService;1");

  if (accService) {
    return accService->CreateHTMLTableAccessible(static_cast<nsIFrame*>(this), aAccessible);
  }

  return NS_ERROR_FAILURE;
}
#endif

 PRBool
nsTableOuterFrame::IsContainingBlock() const
{
  return PR_FALSE;
}

void
nsTableOuterFrame::Destroy()
{
  mCaptionFrames.DestroyFrames();
  nsHTMLContainerFrame::Destroy();
}

nsFrameList
nsTableOuterFrame::GetChildList(nsIAtom* aListName) const
{
  if (nsGkAtoms::captionList == aListName) {
    return mCaptionFrames;
  }
  if (!aListName) {
    return mFrames;
  }
  return nsFrameList::EmptyList();
}

nsIAtom*
nsTableOuterFrame::GetAdditionalChildListName(PRInt32 aIndex) const
{
  if (aIndex == NS_TABLE_FRAME_CAPTION_LIST_INDEX) {
    return nsGkAtoms::captionList;
  }
  return nsnull;
}

NS_IMETHODIMP 
nsTableOuterFrame::SetInitialChildList(nsIAtom*        aListName,
                                       nsFrameList&    aChildList)
{
  if (nsGkAtoms::captionList == aListName) {
    
    mCaptionFrames.SetFrames(aChildList);
    mCaptionFrame = mCaptionFrames.FirstChild();
  }
  else {
    NS_ASSERTION(!aListName, "wrong childlist");
    NS_ASSERTION(mFrames.IsEmpty(), "Frame leak!");
    mInnerTableFrame = nsnull;
    if (aChildList.NotEmpty()) {
      if (nsGkAtoms::tableFrame == aChildList.FirstChild()->GetType()) {
        mInnerTableFrame = (nsTableFrame*)aChildList.FirstChild();
        mFrames.SetFrames(aChildList);
      }
      else {
        NS_ERROR("expected a table frame");
        return NS_ERROR_INVALID_ARG;
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsTableOuterFrame::AppendFrames(nsIAtom*        aListName,
                                nsFrameList&    aFrameList)
{
  nsresult rv;

  
  
  if (nsGkAtoms::captionList == aListName) {
    NS_ASSERTION(aFrameList.IsEmpty() ||
                 aFrameList.FirstChild()->GetType() == nsGkAtoms::tableCaptionFrame,
                 "appending non-caption frame to captionList");
    mCaptionFrames.AppendFrames(this, aFrameList);
    mCaptionFrame = mCaptionFrames.FirstChild();
    rv = NS_OK;

    
    
    PresContext()->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                       NS_FRAME_HAS_DIRTY_CHILDREN);
  }
  else {
    NS_PRECONDITION(PR_FALSE, "unexpected child list");
    rv = NS_ERROR_UNEXPECTED;
  }

  return rv;
}

NS_IMETHODIMP
nsTableOuterFrame::InsertFrames(nsIAtom*        aListName,
                                nsIFrame*       aPrevFrame,
                                nsFrameList&    aFrameList)
{
  if (nsGkAtoms::captionList == aListName) {
    NS_ASSERTION(!aPrevFrame || aPrevFrame->GetParent() == this,
                 "inserting after sibling frame with different parent");
    NS_ASSERTION(aFrameList.IsEmpty() ||
                 aFrameList.FirstChild()->GetType() == nsGkAtoms::tableCaptionFrame,
                 "inserting non-caption frame into captionList");
    mCaptionFrames.InsertFrames(nsnull, aPrevFrame, aFrameList);
    mCaptionFrame = mCaptionFrames.FirstChild();

    
    
    PresContext()->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                       NS_FRAME_HAS_DIRTY_CHILDREN);
    return NS_OK;
  }
  else {
    NS_PRECONDITION(!aPrevFrame, "invalid previous frame");
    return AppendFrames(aListName, aFrameList);
  }
}

NS_IMETHODIMP
nsTableOuterFrame::RemoveFrame(nsIAtom*        aListName,
                               nsIFrame*       aOldFrame)
{
  
  
  NS_PRECONDITION(nsGkAtoms::captionList == aListName, "can't remove inner frame");

  if (HasSideCaption()) {
    
    
    mInnerTableFrame->AddStateBits(NS_FRAME_IS_DIRTY);
  }

  
  mCaptionFrames.DestroyFrame(aOldFrame);
  mCaptionFrame = mCaptionFrames.FirstChild();
  
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
  
  
  if (!IsVisibleInSelection(aBuilder))
    return NS_OK;

  
  
  if (!mCaptionFrame)
    return BuildDisplayListForInnerTable(aBuilder, aDirtyRect, aLists);
    
  nsDisplayListCollection set;
  nsresult rv = BuildDisplayListForInnerTable(aBuilder, aDirtyRect, set);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsDisplayListSet captionSet(set, set.BlockBorderBackgrounds());
  rv = BuildDisplayListForChild(aBuilder, mCaptionFrame, aDirtyRect, captionSet);
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

void
nsTableOuterFrame::SetSelected(PRBool        aSelected,
                               SelectionType aType)
{
  nsFrame::SetSelected(aSelected, aType);
  if (mInnerTableFrame) {
    mInnerTableFrame->SetSelected(aSelected, aType);
  }
}

NS_IMETHODIMP 
nsTableOuterFrame::GetParentStyleContextFrame(nsPresContext* aPresContext,
                                              nsIFrame**      aProviderFrame,
                                              PRBool*         aIsChild)
{
  
  
  
  
  
  
  
  
  

  if (!mInnerTableFrame) {
    *aProviderFrame = this;
    *aIsChild = PR_FALSE;
    return NS_ERROR_FAILURE;
  }
  *aProviderFrame = mInnerTableFrame;
  *aIsChild = PR_TRUE;
  return NS_OK;
}



void
nsTableOuterFrame::InitChildReflowState(nsPresContext&    aPresContext,                     
                                        nsHTMLReflowState& aReflowState)
                                    
{
  nsMargin collapseBorder;
  nsMargin collapsePadding(0,0,0,0);
  nsMargin* pCollapseBorder  = nsnull;
  nsMargin* pCollapsePadding = nsnull;
  if ((aReflowState.frame == mInnerTableFrame) && (mInnerTableFrame->IsBorderCollapse())) {
    collapseBorder  = mInnerTableFrame->GetIncludedOuterBCBorder();
    pCollapseBorder = &collapseBorder;
    pCollapsePadding = &collapsePadding;
  }
  aReflowState.Init(&aPresContext, -1, -1, pCollapseBorder, pCollapsePadding);
}



void
nsTableOuterFrame::GetMargin(nsPresContext*           aPresContext,
                             const nsHTMLReflowState& aOuterRS,
                             nsIFrame*                aChildFrame,
                             nscoord                  aAvailWidth,
                             nsMargin&                aMargin)
{
  
  

  
  
  nsHTMLReflowState childRS(aPresContext, aOuterRS, aChildFrame,
                            nsSize(aAvailWidth, aOuterRS.availableHeight),
                            -1, -1, PR_FALSE);
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
nsTableOuterFrame::GetMinWidth(nsIRenderingContext *aRenderingContext)
{
  nscoord width = nsLayoutUtils::IntrinsicForContainer(aRenderingContext,
                    mInnerTableFrame, nsLayoutUtils::MIN_WIDTH);
  DISPLAY_MIN_WIDTH(this, width);
  if (mCaptionFrame) {
    nscoord capWidth =
      nsLayoutUtils::IntrinsicForContainer(aRenderingContext, mCaptionFrame,
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
nsTableOuterFrame::GetPrefWidth(nsIRenderingContext *aRenderingContext)
{
  nscoord maxWidth;
  DISPLAY_PREF_WIDTH(this, maxWidth);

  maxWidth = nsLayoutUtils::IntrinsicForContainer(aRenderingContext,
               mInnerTableFrame, nsLayoutUtils::PREF_WIDTH);
  if (mCaptionFrame) {
    PRUint8 captionSide = GetCaptionSide();
    switch(captionSide) {
    case NS_STYLE_CAPTION_SIDE_LEFT:
    case NS_STYLE_CAPTION_SIDE_RIGHT:
      {
        nscoord capMin =
          nsLayoutUtils::IntrinsicForContainer(aRenderingContext, mCaptionFrame,
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
          nsLayoutUtils::IntrinsicForContainer(aRenderingContext, mCaptionFrame,
                                               iwt);
        maxWidth = NS_MAX(maxWidth, capPref);
      }
      break;
    }
  }
  return maxWidth;
}




static nscoord
ChildShrinkWrapWidth(nsIRenderingContext *aRenderingContext,
                     nsIFrame *aChildFrame,
                     nsSize aCBSize, nscoord aAvailableWidth,
                     nscoord *aMarginResult = nsnull)
{
  
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
                  PR_TRUE);
  if (aMarginResult)
    *aMarginResult = offsets.mComputedMargin.LeftRight();
  return size.width + offsets.mComputedMargin.LeftRight() +
                      offsets.mComputedBorderPadding.LeftRight();
}

 nsSize
nsTableOuterFrame::ComputeAutoSize(nsIRenderingContext *aRenderingContext,
                                   nsSize aCBSize, nscoord aAvailableWidth,
                                   nsSize aMargin, nsSize aBorder,
                                   nsSize aPadding, PRBool aShrinkWrap)
{
  if (!aShrinkWrap)
    return nsHTMLContainerFrame::ComputeAutoSize(aRenderingContext, aCBSize,
               aAvailableWidth, aMargin, aBorder, aPadding, aShrinkWrap);

  
  
  
  

  
  PRUint8 captionSide = GetCaptionSide();
  nscoord width;
  if (captionSide == NO_SIDE) {
    width = ChildShrinkWrapWidth(aRenderingContext, mInnerTableFrame,
                                 aCBSize, aAvailableWidth);
  } else if (captionSide == NS_STYLE_CAPTION_SIDE_LEFT ||
             captionSide == NS_STYLE_CAPTION_SIDE_RIGHT) {
    nscoord capWidth = ChildShrinkWrapWidth(aRenderingContext, mCaptionFrame,
                                            aCBSize, aAvailableWidth);
    width = capWidth + ChildShrinkWrapWidth(aRenderingContext,
                                            mInnerTableFrame, aCBSize,
                                            aAvailableWidth - capWidth);
  } else if (captionSide == NS_STYLE_CAPTION_SIDE_TOP ||
             captionSide == NS_STYLE_CAPTION_SIDE_BOTTOM) {
    nscoord margin;
    width = ChildShrinkWrapWidth(aRenderingContext, mInnerTableFrame,
                                 aCBSize, aAvailableWidth, &margin);
    nscoord capWidth = ChildShrinkWrapWidth(aRenderingContext,
                                            mCaptionFrame, aCBSize,
                                            width - margin);
    if (capWidth > width)
      width = capWidth;
  } else {
    NS_ASSERTION(captionSide == NS_STYLE_CAPTION_SIDE_TOP_OUTSIDE ||
                 captionSide == NS_STYLE_CAPTION_SIDE_BOTTOM_OUTSIDE,
                 "unexpected caption-side");
    width = ChildShrinkWrapWidth(aRenderingContext, mInnerTableFrame,
                                 aCBSize, aAvailableWidth);
    nscoord capWidth = ChildShrinkWrapWidth(aRenderingContext,
                                            mCaptionFrame, aCBSize,
                                            aAvailableWidth);
    if (capWidth > width)
      width = capWidth;
  }

  return nsSize(width, NS_UNCONSTRAINEDSIZE);
}

PRUint8
nsTableOuterFrame::GetCaptionSide()
{
  if (mCaptionFrame) {
    return mCaptionFrame->GetStyleTableBorder()->mCaptionSide;
  }
  else {
    return NO_SIDE; 
  }
}

PRUint8
nsTableOuterFrame::GetCaptionVerticalAlign()
{
  const nsStyleCoord& va = mCaptionFrame->GetStyleTextReset()->mVerticalAlign;
  return (va.GetUnit() == eStyleUnit_Enumerated)
           ? va.GetIntValue()
           : NS_STYLE_VERTICAL_ALIGN_TOP;
}

void
nsTableOuterFrame::SetDesiredSize(PRUint8         aCaptionSide,
                                  const nsMargin& aInnerMargin,
                                  const nsMargin& aCaptionMargin,
                                  nscoord&        aWidth,
                                  nscoord&        aHeight)
{
  aWidth = aHeight = 0;

  nsRect innerRect = mInnerTableFrame->GetRect();
  nscoord innerWidth = innerRect.width;

  nsRect captionRect(0,0,0,0);
  nscoord captionWidth = 0;
  if (mCaptionFrame) {
    captionRect = mCaptionFrame->GetRect();
    captionWidth = captionRect.width;
  }
  switch(aCaptionSide) {
    case NS_STYLE_CAPTION_SIDE_LEFT:
      aWidth = NS_MAX(aInnerMargin.left, aCaptionMargin.left + captionWidth + aCaptionMargin.right) +
               innerWidth + aInnerMargin.right;
      break;
    case NS_STYLE_CAPTION_SIDE_RIGHT:
      aWidth = NS_MAX(aInnerMargin.right, aCaptionMargin.left + captionWidth + aCaptionMargin.right) +
               innerWidth + aInnerMargin.left;
      break;
    default:
      aWidth = aInnerMargin.left + innerWidth + aInnerMargin.right;
      aWidth = NS_MAX(aWidth, captionRect.XMost() + aCaptionMargin.right);
  }
  aHeight = innerRect.YMost() + aInnerMargin.bottom;
  if (NS_STYLE_CAPTION_SIDE_BOTTOM != aCaptionSide) {
    aHeight = NS_MAX(aHeight, captionRect.YMost() + aCaptionMargin.bottom);
  }
  else {
    aHeight = NS_MAX(aHeight, captionRect.YMost() + aCaptionMargin.bottom +
                              aInnerMargin.bottom);
  }

}


void
nsTableOuterFrame::BalanceLeftRightCaption(PRUint8         aCaptionSide,
                                           const nsMargin& aInnerMargin,
                                           const nsMargin& aCaptionMargin,
                                           nscoord&        aInnerWidth, 
                                           nscoord&        aCaptionWidth)
{
  
  




















    

  float capPercent   = -1.0;
  float innerPercent = -1.0;
  const nsStylePosition* position = mCaptionFrame->GetStylePosition();
  if (eStyleUnit_Percent == position->mWidth.GetUnit()) {
    capPercent = position->mWidth.GetPercentValue();
    if (capPercent >= 1.0)
      return;
  }

  position = mInnerTableFrame->GetStylePosition();
  if (eStyleUnit_Percent == position->mWidth.GetUnit()) {
    innerPercent = position->mWidth.GetPercentValue();
    if (innerPercent >= 1.0)
      return;
  }

  if ((capPercent <= 0.0) && (innerPercent <= 0.0))
    return;

  
  if (innerPercent <= 0.0) {
    if (NS_STYLE_CAPTION_SIDE_LEFT == aCaptionSide) 
      aCaptionWidth= (nscoord) ((capPercent / (1.0 - capPercent)) * (aCaptionMargin.left + aCaptionMargin.right + 
                                                          aInnerWidth + aInnerMargin.right));
    else
      aCaptionWidth= (nscoord) ((capPercent / (1.0 - capPercent)) * (aCaptionMargin.left + aCaptionMargin.right + 
                                                          aInnerWidth + aInnerMargin.left)); 
  } 
  else {
    aCaptionWidth = (nscoord) ((capPercent / innerPercent) * aInnerWidth);
  }
}

nsresult 
nsTableOuterFrame::GetCaptionOrigin(PRUint32         aCaptionSide,
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
  if (!mCaptionFrame) return NS_OK;
  
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
          aOrigin.y = NS_MAX(0, aInnerMargin.top + ((aInnerSize.height - aCaptionSize.height) / 2));
          break;
        case NS_STYLE_VERTICAL_ALIGN_BOTTOM:
          aOrigin.y = NS_MAX(0, aInnerMargin.top + aInnerSize.height - aCaptionSize.height);
          break;
        default:
          break;
      }
      break;
    case NS_STYLE_CAPTION_SIDE_BOTTOM: {
      aOrigin.y = aInnerMargin.top + aInnerSize.height + aCaptionMargin.top;
    } break;
    case NS_STYLE_CAPTION_SIDE_BOTTOM_OUTSIDE: {
      nsCollapsingMargin marg;
      marg.Include(aCaptionMargin.top);
      marg.Include(aInnerMargin.bottom);
      nscoord collapseMargin = marg.get();
      aOrigin.y = aInnerMargin.top + aInnerSize.height + collapseMargin;
    } break;
    case NS_STYLE_CAPTION_SIDE_TOP: {
      aOrigin.y = aInnerMargin.top + aCaptionMargin.top;
    } break;
    case NS_STYLE_CAPTION_SIDE_TOP_OUTSIDE: {
      aOrigin.y = aCaptionMargin.top;
    } break;
    default:
      NS_NOTREACHED("Unknown caption alignment type");
      break;
  }
  return NS_OK;
}

nsresult 
nsTableOuterFrame::GetInnerOrigin(PRUint32         aCaptionSide,
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
      aInnerMargin.right  = NS_MAX(0, aInnerMargin.right);
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
          aOrigin.y = NS_MAX(aInnerMargin.top, (aCaptionSize.height - aInnerSize.height) / 2);
          break;
        case NS_STYLE_VERTICAL_ALIGN_BOTTOM:
          aOrigin.y = NS_MAX(aInnerMargin.top, aCaptionSize.height - aInnerSize.height);
          break;
        default:
          break;
      }
    } break;
    case NO_SIDE:
    case NS_STYLE_CAPTION_SIDE_TOP: {
      aOrigin.y = aInnerMargin.top + aCaptionMargin.top + aCaptionSize.height +
                  aCaptionMargin.bottom;
    } break;
    case NS_STYLE_CAPTION_SIDE_TOP_OUTSIDE: {
      nsCollapsingMargin marg;
      marg.Include(aCaptionMargin.bottom);
      marg.Include(aInnerMargin.top);
      nscoord collapseMargin = marg.get();
      aOrigin.y = aCaptionMargin.top + aCaptionSize.height + collapseMargin;
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
    if (mCaptionFrame == aChildFrame) {
      availHeight = NS_UNCONSTRAINEDSIZE;
    } else {
      nsMargin margin;
      GetMargin(aPresContext, aOuterRS, aChildFrame, aOuterRS.availableWidth,
                margin);
    
      NS_ASSERTION(NS_UNCONSTRAINEDSIZE != margin.top, "No unconstrainedsize arithmetic, please");
      availHeight -= margin.top;
 
      NS_ASSERTION(NS_UNCONSTRAINEDSIZE != margin.bottom, "No unconstrainedsize arithmetic, please");
      availHeight -= margin.bottom;
    }
  }
  nsSize availSize(aAvailWidth, availHeight);
  
  
  
  nsHTMLReflowState &childRS = * new (aChildRSSpace)
    nsHTMLReflowState(aPresContext, aOuterRS, aChildFrame, availSize,
                      -1, -1, PR_FALSE);
  InitChildReflowState(*aPresContext, childRS);

  
  if (mCaptionFrame) {
    PRUint8 captionSide = GetCaptionSide();
    if (((captionSide == NS_STYLE_CAPTION_SIDE_BOTTOM ||
          captionSide == NS_STYLE_CAPTION_SIDE_BOTTOM_OUTSIDE) &&
         mCaptionFrame == aChildFrame) || 
        ((captionSide == NS_STYLE_CAPTION_SIDE_TOP ||
          captionSide == NS_STYLE_CAPTION_SIDE_TOP_OUTSIDE) &&
         mInnerTableFrame == aChildFrame)) {
      childRS.mFlags.mIsTopOfPage = PR_FALSE;
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
nsTableOuterFrame::UpdateReflowMetrics(PRUint8              aCaptionSide,
                                       nsHTMLReflowMetrics& aMet,
                                       const nsMargin&      aInnerMargin,
                                       const nsMargin&      aCaptionMargin)
{
  SetDesiredSize(aCaptionSide, aInnerMargin, aCaptionMargin,
                 aMet.width, aMet.height);

  aMet.mOverflowArea = nsRect(0, 0, aMet.width, aMet.height);
  ConsiderChildOverflow(aMet.mOverflowArea, mInnerTableFrame);
  if (mCaptionFrame) {
    ConsiderChildOverflow(aMet.mOverflowArea, mCaptionFrame);
  }
  FinishAndStoreOverflow(&aMet);
}

NS_METHOD nsTableOuterFrame::Reflow(nsPresContext*           aPresContext,
                                    nsHTMLReflowMetrics&     aDesiredSize,
                                    const nsHTMLReflowState& aOuterRS,
                                    nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsTableOuterFrame");
  DISPLAY_REFLOW(aPresContext, this, aOuterRS, aDesiredSize, aStatus);

  
  
  if (mFrames.IsEmpty() || !mInnerTableFrame) {
    NS_ERROR("incomplete children");
    return NS_ERROR_FAILURE;
  }
  nsresult rv = NS_OK;
  PRUint8 captionSide = GetCaptionSide();

  
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

  nsRect origInnerRect = mInnerTableFrame->GetRect();
  nsRect origInnerOverflowRect = mInnerTableFrame->GetOverflowRect();
  PRBool innerFirstReflow =
    (mInnerTableFrame->GetStateBits() & NS_FRAME_FIRST_REFLOW) != 0;
  nsRect origCaptionRect;
  nsRect origCaptionOverflowRect;
  PRBool captionFirstReflow;
  if (mCaptionFrame) {
    origCaptionRect = mCaptionFrame->GetRect();
    origCaptionOverflowRect = mCaptionFrame->GetOverflowRect();
    captionFirstReflow =
      (mCaptionFrame->GetStateBits() & NS_FRAME_FIRST_REFLOW) != 0;
  }
  
  
  if (captionSide == NO_SIDE) {
    
    OuterBeginReflowChild(aPresContext, mInnerTableFrame, aOuterRS,
                          innerRSSpace, aOuterRS.ComputedWidth());
  } else if (captionSide == NS_STYLE_CAPTION_SIDE_LEFT ||
             captionSide == NS_STYLE_CAPTION_SIDE_RIGHT) {
    
    
    
    OuterBeginReflowChild(aPresContext, mCaptionFrame, aOuterRS,
                          captionRSSpace, aOuterRS.ComputedWidth());
    nscoord innerAvailWidth = aOuterRS.ComputedWidth() -
      (captionRS->ComputedWidth() + captionRS->mComputedMargin.LeftRight() +
       captionRS->mComputedBorderPadding.LeftRight());
    OuterBeginReflowChild(aPresContext, mInnerTableFrame, aOuterRS,
                          innerRSSpace, innerAvailWidth);

  } else if (captionSide == NS_STYLE_CAPTION_SIDE_TOP ||
             captionSide == NS_STYLE_CAPTION_SIDE_BOTTOM) {
    
    
    
    
    
    
    
    
    OuterBeginReflowChild(aPresContext, mInnerTableFrame, aOuterRS,
                          innerRSSpace, aOuterRS.ComputedWidth());
    
    
    
    
    
    nscoord innerBorderWidth = innerRS->ComputedWidth() +
                               innerRS->mComputedBorderPadding.LeftRight();
    OuterBeginReflowChild(aPresContext, mCaptionFrame, aOuterRS,
                          captionRSSpace, innerBorderWidth);
  } else {
    NS_ASSERTION(captionSide == NS_STYLE_CAPTION_SIDE_TOP_OUTSIDE ||
                 captionSide == NS_STYLE_CAPTION_SIDE_BOTTOM_OUTSIDE,
                 "unexpected caption-side");
    
    OuterBeginReflowChild(aPresContext, mCaptionFrame, aOuterRS,
                          captionRSSpace, aOuterRS.ComputedWidth());
    OuterBeginReflowChild(aPresContext, mInnerTableFrame, aOuterRS,
                          innerRSSpace, aOuterRS.ComputedWidth());
  }

  
  nsHTMLReflowMetrics captionMet;
  nsSize captionSize;
  nsMargin captionMargin;
  if (mCaptionFrame) {
    nsReflowStatus capStatus; 
    rv = OuterDoReflowChild(aPresContext, mCaptionFrame, *captionRS,
                            captionMet, capStatus);
    if (NS_FAILED(rv)) return rv;
    captionSize.width = captionMet.width;
    captionSize.height = captionMet.height;
    captionMargin = captionRS->mComputedMargin;
  } else {
    captionSize.SizeTo(0,0);
    captionMargin.SizeTo(0,0,0,0);
  }

  
  
  nsHTMLReflowMetrics innerMet;
  rv = OuterDoReflowChild(aPresContext, mInnerTableFrame, *innerRS,
                          innerMet, aStatus);
  if (NS_FAILED(rv)) return rv;
  nsSize innerSize;
  innerSize.width = innerMet.width;
  innerSize.height = innerMet.height;
  nsMargin innerMargin = innerRS->mComputedMargin;

  nsSize   containSize = GetContainingBlockSize(aOuterRS);

  
  

  
  

  if (mCaptionFrame) {
    nsPoint captionOrigin;
    GetCaptionOrigin(captionSide, containSize, innerSize, 
                     innerMargin, captionSize, captionMargin, captionOrigin);
    FinishReflowChild(mCaptionFrame, aPresContext, captionRS, captionMet,
                      captionOrigin.x, captionOrigin.y, 0);
    captionRS->~nsHTMLReflowState();
  }
  
  

  nsPoint innerOrigin;
  GetInnerOrigin(captionSide, containSize, captionSize, 
                 captionMargin, innerSize, innerMargin, innerOrigin);
  FinishReflowChild(mInnerTableFrame, aPresContext, innerRS, innerMet,
                    innerOrigin.x, innerOrigin.y, 0);
  innerRS->~nsHTMLReflowState();

  nsTableFrame::InvalidateFrame(mInnerTableFrame, origInnerRect,
                                origInnerOverflowRect, innerFirstReflow);
  if (mCaptionFrame) {
    nsTableFrame::InvalidateFrame(mCaptionFrame, origCaptionRect,
                                  origCaptionOverflowRect, captionFirstReflow);
  }

  UpdateReflowMetrics(captionSide, aDesiredSize, innerMargin, captionMargin);
  
  

  NS_FRAME_SET_TRUNCATION(aStatus, aOuterRS, aDesiredSize);
  return rv;
}

nsIAtom*
nsTableOuterFrame::GetType() const
{
  return nsGkAtoms::tableOuterFrame;
}




NS_IMETHODIMP 
nsTableOuterFrame::GetCellDataAt(PRInt32 aRowIndex, PRInt32 aColIndex, 
                                 nsIDOMElement* &aCell,   
                                 PRInt32& aStartRowIndex, PRInt32& aStartColIndex, 
                                 PRInt32& aRowSpan, PRInt32& aColSpan,
                                 PRInt32& aActualRowSpan, PRInt32& aActualColSpan,
                                 PRBool& aIsSelected)
{
  NS_ASSERTION(mInnerTableFrame, "no inner table frame yet?");
  
  return mInnerTableFrame->GetCellDataAt(aRowIndex, aColIndex, aCell,
                                        aStartRowIndex, aStartColIndex, 
                                        aRowSpan, aColSpan, aActualRowSpan,
                                        aActualColSpan, aIsSelected);
}

NS_IMETHODIMP
nsTableOuterFrame::GetTableSize(PRInt32& aRowCount, PRInt32& aColCount)
{
  NS_ASSERTION(mInnerTableFrame, "no inner table frame yet?");

  return mInnerTableFrame->GetTableSize(aRowCount, aColCount);
}

NS_IMETHODIMP
nsTableOuterFrame::GetIndexByRowAndColumn(PRInt32 aRow, PRInt32 aColumn,
                                          PRInt32 *aIndex)
{
  NS_ENSURE_ARG_POINTER(aIndex);

  NS_ASSERTION(mInnerTableFrame, "no inner table frame yet?");
  return mInnerTableFrame->GetIndexByRowAndColumn(aRow, aColumn, aIndex);
}

NS_IMETHODIMP
nsTableOuterFrame::GetRowAndColumnByIndex(PRInt32 aIndex,
                                          PRInt32 *aRow, PRInt32 *aColumn)
{
  NS_ENSURE_ARG_POINTER(aRow);
  NS_ENSURE_ARG_POINTER(aColumn);

  NS_ASSERTION(mInnerTableFrame, "no inner table frame yet?");
  return mInnerTableFrame->GetRowAndColumnByIndex(aIndex, aRow, aColumn);
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

