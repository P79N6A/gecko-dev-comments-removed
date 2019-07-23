




































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
  
  SetFlags(NS_BLOCK_SPACE_MGR);
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

inline PRBool IsSideCaption(nsIFrame* aCaptionFrame)
{
  PRUint8 captionSide = aCaptionFrame->GetStyleTableBorder()->mCaptionSide;
  return captionSide == NS_SIDE_LEFT || captionSide == NS_SIDE_RIGHT;
}

 nsSize
nsTableCaptionFrame::ComputeAutoSize(nsIRenderingContext *aRenderingContext,
                                     nsSize aCBSize, nscoord aAvailableWidth,
                                     nsSize aMargin, nsSize aBorder,
                                     nsSize aPadding, PRBool aShrinkWrap)
{
  nsSize result = nsBlockFrame::ComputeAutoSize(aRenderingContext, aCBSize,
                    aAvailableWidth, aMargin, aBorder, aPadding, aShrinkWrap);
  if (IsSideCaption(this)) {
    result.width = GetMinWidth(aRenderingContext);
  }
  return result;
}

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



NS_IMPL_ADDREF_INHERITED(nsTableOuterFrame, nsHTMLContainerFrame)
NS_IMPL_RELEASE_INHERITED(nsTableOuterFrame, nsHTMLContainerFrame)

nsTableOuterFrame::nsTableOuterFrame(nsStyleContext* aContext):
  nsHTMLContainerFrame(aContext)
{
}

nsTableOuterFrame::~nsTableOuterFrame()
{
}

nsresult nsTableOuterFrame::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  if (NULL == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }

  if (aIID.Equals(NS_GET_IID(nsITableLayout))) 
  { 
    *aInstancePtr = (void*)(nsITableLayout*)this;
    return NS_OK;
  }

  return nsHTMLContainerFrame::QueryInterface(aIID, aInstancePtr);
}

#ifdef ACCESSIBILITY
NS_IMETHODIMP nsTableOuterFrame::GetAccessible(nsIAccessible** aAccessible)
{
  nsCOMPtr<nsIAccessibilityService> accService = do_GetService("@mozilla.org/accessibilityService;1");

  if (accService) {
    return accService->CreateHTMLTableAccessible(NS_STATIC_CAST(nsIFrame*, this), aAccessible);
  }

  return NS_ERROR_FAILURE;
}
#endif

 PRBool
nsTableOuterFrame::IsContainingBlock() const
{
  return PR_FALSE;
}

NS_IMETHODIMP
nsTableOuterFrame::Init(
                   nsIContent*           aContent,
                   nsIFrame*             aParent,
                   nsIFrame*             aPrevInFlow)
{
  nsresult rv = nsHTMLContainerFrame::Init(aContent, aParent, aPrevInFlow);
  
  
  mState |= NS_FRAME_EXCLUDE_IGNORABLE_WHITESPACE;

  return rv;
}

void
nsTableOuterFrame::Destroy()
{
  mCaptionFrames.DestroyFrames();
  nsHTMLContainerFrame::Destroy();
}

nsIFrame*
nsTableOuterFrame::GetFirstChild(nsIAtom* aListName) const
{
  if (nsGkAtoms::captionList == aListName) {
    return mCaptionFrames.FirstChild();
  }
  if (!aListName) {
    return mFrames.FirstChild();
  }
  return nsnull;
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
                                       nsIFrame*       aChildList)
{
  if (nsGkAtoms::captionList == aListName) {
    
    mCaptionFrames.SetFrames(aChildList);
    mCaptionFrame  = mCaptionFrames.FirstChild();
  }
  else {
    NS_ASSERTION(!aListName, "wrong childlist");
    NS_ASSERTION(mFrames.IsEmpty(), "Frame leak!");
    mFrames.SetFrames(aChildList);
    mInnerTableFrame = nsnull;
    if (aChildList) {
      if (nsGkAtoms::tableFrame == aChildList->GetType()) {
        mInnerTableFrame = (nsTableFrame*)aChildList;
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
                                nsIFrame*       aFrameList)
{
  nsresult rv;

  
  
  if (nsGkAtoms::captionList == aListName) {
    NS_ASSERTION(!aFrameList ||
                 aFrameList->GetType() == nsGkAtoms::tableCaptionFrame,
                 "appending non-caption frame to captionList");
    mCaptionFrames.AppendFrames(this, aFrameList);
    mCaptionFrame = mCaptionFrames.FirstChild();
    rv = NS_OK;

    
    
    AddStateBits(NS_FRAME_HAS_DIRTY_CHILDREN);
    PresContext()->PresShell()->
      FrameNeedsReflow(mCaptionFrame, nsIPresShell::eTreeChange);
    
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
                                nsIFrame*       aFrameList)
{
  if (nsGkAtoms::captionList == aListName) {
    NS_ASSERTION(!aPrevFrame || aPrevFrame->GetParent() == this,
                 "inserting after sibling frame with different parent");
    NS_ASSERTION(!aFrameList ||
                 aFrameList->GetType() == nsGkAtoms::tableCaptionFrame,
                 "inserting non-caption frame into captionList");
    mCaptionFrames.InsertFrames(nsnull, aPrevFrame, aFrameList);
    mCaptionFrame = mCaptionFrames.FirstChild();
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

  PRUint8 captionSide = GetCaptionSide();

  if (NS_SIDE_LEFT == captionSide || NS_SIDE_RIGHT == captionSide) {
    
    
    mInnerTableFrame->AddStateBits(NS_FRAME_IS_DIRTY);
  }

  
  mCaptionFrames.DestroyFrame(aOldFrame);
  mCaptionFrame = mCaptionFrames.FirstChild();
  
  AddStateBits(NS_FRAME_HAS_DIRTY_CHILDREN); 
  PresContext()->PresShell()->
    FrameNeedsReflow(this, nsIPresShell::eTreeChange);

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

NS_IMETHODIMP nsTableOuterFrame::SetSelected(nsPresContext* aPresContext,
                                             nsIDOMRange *aRange,
                                             PRBool aSelected,
                                             nsSpread aSpread)
{
  nsresult result = nsFrame::SetSelected(aPresContext, aRange,aSelected, aSpread);
  if (NS_SUCCEEDED(result) && mInnerTableFrame)
    return mInnerTableFrame->SetSelected(aPresContext, aRange,aSelected, aSpread);
  return result;
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

static
nscoord CalcAutoMargin(nscoord aAutoMargin,
                       nscoord aOppositeMargin,
                       nscoord aContainBlockSize,
                       nscoord aFrameSize)
{
  nscoord margin;
  if (NS_AUTOMARGIN == aOppositeMargin) 
    margin = (aContainBlockSize - aFrameSize) / 2;
  else {
    margin = aContainBlockSize - aFrameSize - aOppositeMargin;
  }
  return PR_MAX(0, margin);
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
    size.height = containRS->mComputedHeight;
    if (NS_UNCONSTRAINEDSIZE == size.height) {
      size.height = 0;
    }
  }
  return size;
}

void
nsTableOuterFrame::InvalidateDamage(PRUint8         aCaptionSide,
                                    const nsSize&   aOuterSize,
                                    PRBool          aInnerChanged,
                                    PRBool          aCaptionChanged,
                                    nsRect*         aOldOverflowArea)
{
  if (!aInnerChanged && !aCaptionChanged) return;

  nsRect damage;
  if (aInnerChanged && aCaptionChanged) {
    damage = nsRect(0, 0, aOuterSize.width, aOuterSize.height);
    if (aOldOverflowArea) {
      damage.UnionRect(damage, *aOldOverflowArea);
    }
    nsRect* overflowArea = GetOverflowAreaProperty();
    if (overflowArea) {
      damage.UnionRect(damage, *overflowArea);
    }
  }
  else {
    nsRect captionRect(0,0,0,0);
    nsRect innerRect = mInnerTableFrame->GetRect();
    if (mCaptionFrame) {
      captionRect = mCaptionFrame->GetRect();
    }
    
    damage.x = 0;
    damage.width  = aOuterSize.width;
    switch(aCaptionSide) {
    case NS_SIDE_BOTTOM:
      if (aCaptionChanged) {
        damage.y = innerRect.y;
        damage.height = aOuterSize.height - damage.y;
      }
      else { 
        damage.y = 0;
        damage.height = captionRect.y;
      }
      break;
    case NS_SIDE_LEFT:
      if (aCaptionChanged) {
        damage.width = innerRect.x;
        damage.y = 0;
        damage.height = captionRect.YMost();
      }
      else { 
        damage.x = captionRect.XMost();
        damage.width = innerRect.XMost() - damage.x;
        damage.y = 0;
        damage.height = innerRect.YMost();
      }
      break;
    case NS_SIDE_RIGHT:
     if (aCaptionChanged) {
        damage.x = innerRect.XMost();
        damage.width -= damage.x;
        damage.y = 0;
        damage.height = captionRect.YMost();
      }
     else { 
        damage.width -= captionRect.width;
        damage.y = 0;
        damage.height = innerRect.YMost();
      }
      break;
    default: 
      if (aCaptionChanged) {
        damage.y = 0;
        damage.height = innerRect.y;
      }
      else { 
        damage.y = captionRect.y;
        damage.height = aOuterSize.height - damage.y;
      }
      break;
    }
     
    nsIFrame* kidFrame = aCaptionChanged ? mCaptionFrame : mInnerTableFrame;
    ConsiderChildOverflow(damage, kidFrame);
    if (aOldOverflowArea) {
      damage.UnionRect(damage, *aOldOverflowArea);
    }
  }
  Invalidate(damage);
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
    switch(GetCaptionSide()) {
    case NS_SIDE_LEFT:
    case NS_SIDE_RIGHT:
      width += capWidth;
      break;
    default:
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
    case NS_SIDE_LEFT:
    case NS_SIDE_RIGHT:
      {
        nscoord capMin =
          nsLayoutUtils::IntrinsicForContainer(aRenderingContext, mCaptionFrame,
                                               nsLayoutUtils::MIN_WIDTH);
        maxWidth += capMin;
      }
      break;
    case NS_SIDE_TOP:
    case NS_SIDE_BOTTOM:
    default:  
      {
        nscoord capPref =
          nsLayoutUtils::IntrinsicForContainer(aRenderingContext, mCaptionFrame,
                                               nsLayoutUtils::PREF_WIDTH);
        maxWidth = PR_MAX(maxWidth, capPref);
      }
    }
  }
  return maxWidth;
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

  
  
  
  

  
  nsCSSOffsetState innerOffsets(mInnerTableFrame, aRenderingContext,
                                aCBSize.width);
  nsSize tableSize = mInnerTableFrame->ComputeSize(aRenderingContext, aCBSize,
                       aAvailableWidth,
                       nsSize(innerOffsets.mComputedMargin.LeftRight(),
                              innerOffsets.mComputedMargin.TopBottom()),
                       nsSize(innerOffsets.mComputedBorderPadding.LeftRight() -
                                innerOffsets.mComputedPadding.LeftRight(),
                              innerOffsets.mComputedBorderPadding.TopBottom() -
                                innerOffsets.mComputedPadding.TopBottom()),
                       nsSize(innerOffsets.mComputedPadding.LeftRight(),
                              innerOffsets.mComputedPadding.TopBottom()),
                       aShrinkWrap);
  nscoord width = tableSize.width + innerOffsets.mComputedMargin.LeftRight() +
                  innerOffsets.mComputedBorderPadding.LeftRight();

  if (mCaptionFrame) {
    nsCSSOffsetState capOffsets(mCaptionFrame, aRenderingContext,
                                aCBSize.width);
    nsSize capSize = mCaptionFrame->ComputeSize(aRenderingContext, aCBSize,
                       aAvailableWidth,
                       nsSize(capOffsets.mComputedMargin.LeftRight(),
                              capOffsets.mComputedMargin.TopBottom()),
                       nsSize(capOffsets.mComputedBorderPadding.LeftRight() -
                                capOffsets.mComputedPadding.LeftRight(),
                              capOffsets.mComputedBorderPadding.TopBottom() -
                                capOffsets.mComputedPadding.TopBottom()),
                       nsSize(capOffsets.mComputedPadding.LeftRight(),
                              capOffsets.mComputedPadding.TopBottom()),
                       aShrinkWrap);
    PRUint8 captionSide = GetCaptionSide();
    nscoord capWidth = capSize.width + capOffsets.mComputedMargin.LeftRight() +
                       capOffsets.mComputedBorderPadding.LeftRight();
    if (captionSide == NS_SIDE_LEFT || captionSide == NS_SIDE_RIGHT) {
      width += capWidth;
    } else {
      if (capWidth > width)
        width = capWidth;
    }
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
    case NS_SIDE_LEFT:
      aWidth = PR_MAX(aInnerMargin.left, aCaptionMargin.left + captionWidth + aCaptionMargin.right) +
               innerWidth + aInnerMargin.right;
      break;
    case NS_SIDE_RIGHT:
      aWidth = PR_MAX(aInnerMargin.right, aCaptionMargin.left + captionWidth + aCaptionMargin.right) +
               innerWidth + aInnerMargin.left;
      break;
    default:
      aWidth = aInnerMargin.left + innerWidth + aInnerMargin.right;
      aWidth = PR_MAX(aWidth, captionRect.XMost() + aCaptionMargin.right);
  }
  aHeight = innerRect.YMost() + aInnerMargin.bottom;
  aHeight = PR_MAX(aHeight, captionRect.YMost() + aCaptionMargin.bottom);

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
    if (NS_SIDE_LEFT == aCaptionSide) 
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

  switch(aCaptionSide) {
  case NS_SIDE_BOTTOM: {
    if (NS_AUTOMARGIN == aCaptionMargin.left) {
      aCaptionMargin.left = CalcAutoMargin(aCaptionMargin.left, aCaptionMargin.right,
                                           aContainBlockSize.width, aCaptionSize.width);
    }
    aOrigin.x = aCaptionMargin.left;
    if (NS_AUTOMARGIN == aCaptionMargin.top) {
      aCaptionMargin.top = 0;
    }
    nsCollapsingMargin marg;
    marg.Include(aCaptionMargin.top);
    marg.Include(aInnerMargin.bottom);
    nscoord collapseMargin = marg.get();
    if (NS_AUTOMARGIN == aCaptionMargin.bottom) {
      nscoord height = aInnerSize.height + collapseMargin + aCaptionSize.height;
      aCaptionMargin.bottom = CalcAutoMargin(aCaptionMargin.bottom, aInnerMargin.top,
                                             aContainBlockSize.height, height);
    }
    aOrigin.y = aInnerMargin.top + aInnerSize.height + collapseMargin;
  } break;
  case NS_SIDE_LEFT: {
    if (NS_AUTOMARGIN == aCaptionMargin.left) {
      if (NS_AUTOMARGIN != aInnerMargin.left) {
        aCaptionMargin.left = CalcAutoMargin(aCaptionMargin.left, aCaptionMargin.right,
                                             aInnerMargin.left, aCaptionSize.width);
      } 
      else {
        
        aCaptionMargin.left = 0;
      } 
    }
    aOrigin.x = aCaptionMargin.left;
    aOrigin.y = aInnerMargin.top;
    switch(GetCaptionVerticalAlign()) {
      case NS_STYLE_VERTICAL_ALIGN_MIDDLE:
        aOrigin.y = PR_MAX(0, aInnerMargin.top + ((aInnerSize.height - aCaptionSize.height) / 2));
        break;
      case NS_STYLE_VERTICAL_ALIGN_BOTTOM:
        aOrigin.y = PR_MAX(0, aInnerMargin.top + aInnerSize.height - aCaptionSize.height);
        break;
      default:
        break;
    }
  } break;
  case NS_SIDE_RIGHT: {
    if (NS_AUTOMARGIN == aCaptionMargin.left) {
      if (NS_AUTOMARGIN != aInnerMargin.right) {
        aCaptionMargin.left = CalcAutoMargin(aCaptionMargin.left, aCaptionMargin.right,
                                             aInnerMargin.right, aCaptionSize.width);
      }
      else {
       
       aCaptionMargin.left = 0;
      } 
    }
    aOrigin.x = aInnerMargin.left + aInnerSize.width + aCaptionMargin.left;
    aOrigin.y = aInnerMargin.top;
    switch(GetCaptionVerticalAlign()) {
      case NS_STYLE_VERTICAL_ALIGN_MIDDLE:
        aOrigin.y += PR_MAX(0, (aInnerSize.height - aCaptionSize.height) / 2);
        break;
      case NS_STYLE_VERTICAL_ALIGN_BOTTOM:
        aOrigin.y += PR_MAX(0, aInnerSize.height - aCaptionSize.height);
        break;
      default:
        break;
    }
  } break;
  default: { 
    if (NS_AUTOMARGIN == aCaptionMargin.left) {
      aCaptionMargin.left = CalcAutoMargin(aCaptionMargin.left, aCaptionMargin.right,
                                           aContainBlockSize.width, aCaptionSize.width);
    }
    aOrigin.x = aCaptionMargin.left;
    if (NS_AUTOMARGIN == aCaptionMargin.bottom) {
      aCaptionMargin.bottom = 0;
    }
    if (NS_AUTOMARGIN == aCaptionMargin.top) {
      nsCollapsingMargin marg;
      marg.Include(aCaptionMargin.bottom);
      marg.Include(aInnerMargin.top);
      nscoord collapseMargin = marg.get();
      nscoord height = aCaptionSize.height + collapseMargin + aInnerSize.height;
      aCaptionMargin.top = CalcAutoMargin(aCaptionMargin.top, aInnerMargin.bottom,
                                          aContainBlockSize.height, height);
    }
    aOrigin.y = aCaptionMargin.top;
  } break;
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
  aOrigin.x = aOrigin.y = 0;
  if ((NS_UNCONSTRAINEDSIZE == aInnerSize.width) || (NS_UNCONSTRAINEDSIZE == aInnerSize.height) ||  
      (NS_UNCONSTRAINEDSIZE == aCaptionSize.width) || (NS_UNCONSTRAINEDSIZE == aCaptionSize.height)) {
    return NS_OK;
  }

  nscoord minCapWidth = aCaptionSize.width;
  if (NS_AUTOMARGIN != aCaptionMargin.left)
    minCapWidth += aCaptionMargin.left;
  if (NS_AUTOMARGIN != aCaptionMargin.right)
    minCapWidth += aCaptionMargin.right;

  switch(aCaptionSide) {
  case NS_SIDE_BOTTOM: {
    if (NS_AUTOMARGIN == aInnerMargin.left) {
      aInnerMargin.left = CalcAutoMargin(aInnerMargin.left, aInnerMargin.right,
                                         aContainBlockSize.width, aInnerSize.width);
    }
    aOrigin.x = aInnerMargin.left;
    if (NS_AUTOMARGIN == aInnerMargin.bottom) {
      aInnerMargin.bottom = 0;
    }
    if (NS_AUTOMARGIN == aInnerMargin.top) {
      nsCollapsingMargin marg;
      marg.Include(aInnerMargin.bottom);
      marg.Include(aCaptionMargin.top);
      nscoord collapseMargin = marg.get();
      nscoord height = aInnerSize.height + collapseMargin + aCaptionSize.height;
      aInnerMargin.top = CalcAutoMargin(aInnerMargin.top, aCaptionMargin.bottom,
                                        aContainBlockSize.height, height);
    }
    aOrigin.y = aInnerMargin.top;
  } break;
  case NS_SIDE_LEFT: {
    
    if (NS_AUTOMARGIN == aInnerMargin.left) {
      aInnerMargin.left = CalcAutoMargin(aInnerMargin.left, aInnerMargin.right,
                                         aContainBlockSize.width, aInnerSize.width);
      
    }
    if (aInnerMargin.left < minCapWidth) {
      
      aInnerMargin.right += aInnerMargin.left - minCapWidth;
      aInnerMargin.right  = PR_MAX(0, aInnerMargin.right);
      aInnerMargin.left   = minCapWidth;
    }
    aOrigin.x = aInnerMargin.left;
    if (NS_AUTOMARGIN == aInnerMargin.top) {
      aInnerMargin.top = 0;
    }
    aOrigin.y = aInnerMargin.top;
    switch(GetCaptionVerticalAlign()) {
      case NS_STYLE_VERTICAL_ALIGN_MIDDLE:
        aOrigin.y = PR_MAX(aInnerMargin.top, (aCaptionSize.height - aInnerSize.height) / 2);
        break;
      case NS_STYLE_VERTICAL_ALIGN_BOTTOM:
        aOrigin.y = PR_MAX(aInnerMargin.top, aCaptionSize.height - aInnerSize.height);
        break;
      default:
        break;
    }
  } break;
  case NS_SIDE_RIGHT: {
    if (NS_AUTOMARGIN == aInnerMargin.right) {
      aInnerMargin.right = CalcAutoMargin(aInnerMargin.left, aInnerMargin.right,
                                          aContainBlockSize.width, aInnerSize.width);
      if (aInnerMargin.right < minCapWidth) {
        
        aInnerMargin.left -= aInnerMargin.right - minCapWidth;
        aInnerMargin.left  = PR_MAX(0, aInnerMargin.left);
        aInnerMargin.right = minCapWidth;
      }
    }
    aOrigin.x = aInnerMargin.left;
    if (NS_AUTOMARGIN == aInnerMargin.top) {
      aInnerMargin.top = 0;
    }
    aOrigin.y = aInnerMargin.top;
    switch(GetCaptionVerticalAlign()) {
      case NS_STYLE_VERTICAL_ALIGN_MIDDLE:
        aOrigin.y = PR_MAX(aInnerMargin.top, (aCaptionSize.height - aInnerSize.height) / 2);
        break;
      case NS_STYLE_VERTICAL_ALIGN_BOTTOM:
        aOrigin.y = PR_MAX(aInnerMargin.top, aCaptionSize.height - aInnerSize.height);
        break;
      default:
        break;
    }
  } break;
  default: { 
    if (NS_AUTOMARGIN == aInnerMargin.left) {
      aInnerMargin.left = CalcAutoMargin(aInnerMargin.left, aInnerMargin.right,
                                         aContainBlockSize.width, aInnerSize.width);
    }
    aOrigin.x = aInnerMargin.left;
    if (NS_AUTOMARGIN == aInnerMargin.top) {
      aInnerMargin.top = 0;
    }
    nsCollapsingMargin marg;
    marg.Include(aCaptionMargin.bottom);
    marg.Include(aInnerMargin.top);
    nscoord collapseMargin = marg.get();
    if (NS_AUTOMARGIN == aInnerMargin.bottom) {
      nscoord height = aCaptionSize.height + collapseMargin + aInnerSize.height;
      aInnerMargin.bottom = CalcAutoMargin(aCaptionMargin.bottom, aInnerMargin.top,
                                           aContainBlockSize.height, height);
    }
    aOrigin.y = aCaptionMargin.top + aCaptionSize.height + collapseMargin;
  } break;
  }
  return NS_OK;
}


PRBool 
nsTableOuterFrame::IsNested(const nsHTMLReflowState& aReflowState) const
{
  
  const nsHTMLReflowState* rs = aReflowState.parentReflowState;
  while (rs) {
    if (nsGkAtoms::tableFrame == rs->frame->GetType()) {
      return PR_TRUE;
    }
    rs = rs->parentReflowState;
  }
  return PR_FALSE;
}

nsresult
nsTableOuterFrame::OuterReflowChild(nsPresContext*             aPresContext,
                                    nsIFrame*                  aChildFrame,
                                    const nsHTMLReflowState&   aOuterRS,
                                    void*                      aChildRSSpace,
                                    nsHTMLReflowMetrics&       aMetrics,
                                    nscoord                    aAvailWidth, 
                                    nsSize&                    aDesiredSize,
                                    nsMargin&                  aMargin,
                                    nsReflowStatus&            aStatus)
{ 
  aMargin = nsMargin(0,0,0,0);

  
  nscoord availHeight = aOuterRS.availableHeight;
  if (NS_UNCONSTRAINEDSIZE != availHeight) {
    nsMargin margin;
    GetMargin(aPresContext, aOuterRS, aChildFrame, aOuterRS.availableWidth,
              margin);
    
    NS_ASSERTION(NS_UNCONSTRAINEDSIZE != margin.top, "No unconstrainedsize arithmetic, please");
    availHeight -= margin.top;
    
    NS_ASSERTION(NS_UNCONSTRAINEDSIZE != margin.bottom, "No unconstrainedsize arithmetic, please");
    availHeight -= margin.bottom;
  }
  nsSize availSize(aAvailWidth, availHeight);
  
  
  
  nsHTMLReflowState &childRS = * new (aChildRSSpace)
    nsHTMLReflowState(aPresContext, aOuterRS, aChildFrame, availSize,
                      -1, -1, PR_FALSE);
  InitChildReflowState(*aPresContext, childRS);

  
  if (mCaptionFrame) {
    PRUint8 captionSide = GetCaptionSide();
    if (((NS_SIDE_BOTTOM == captionSide) && (mCaptionFrame == aChildFrame)) || 
        ((NS_SIDE_TOP == captionSide) && (mInnerTableFrame == aChildFrame))) {
      childRS.mFlags.mIsTopOfPage = PR_FALSE;
    }
    if ((mCaptionFrame == aChildFrame) && (NS_SIDE_LEFT  != captionSide) 
                                       && (NS_SIDE_RIGHT != captionSide)) {
      aAvailWidth = aOuterRS.availableWidth;
    }
  }

  
  nsPoint childPt = aChildFrame->GetPosition();
  nsresult rv = ReflowChild(aChildFrame, aPresContext, aMetrics, childRS,
                            childPt.x, childPt.y, NS_FRAME_NO_MOVE_FRAME, aStatus);
  if (NS_FAILED(rv)) return rv;
  
  aMargin = childRS.mComputedMargin;

  aDesiredSize.width  = aMetrics.width;
  aDesiredSize.height = aMetrics.height;

  return rv;
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

  PRBool reflowAllKids = aOuterRS.ShouldReflowAllKids();

  if (captionSide == NS_SIDE_LEFT || captionSide == NS_SIDE_RIGHT)
    reflowAllKids = PR_TRUE;

  if (!(GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
    
    
    MoveOverflowToChildList(aPresContext);
  }

  PRBool reflowCaption = mCaptionFrame && (reflowAllKids || (mCaptionFrame->
    GetStateBits() & (NS_FRAME_IS_DIRTY | NS_FRAME_HAS_DIRTY_CHILDREN)));
  PRBool reflowInner = reflowAllKids || (mInnerTableFrame->
    GetStateBits() & (NS_FRAME_IS_DIRTY | NS_FRAME_HAS_DIRTY_CHILDREN));

  
  
  nsHTMLReflowMetrics captionMet;
  nsSize captionSize;
  nsMargin captionMargin;
  
  #define LONGS_IN_HTMLRS \
    ((sizeof(nsHTMLReflowState) + sizeof(long) - 1) / sizeof(long))
  long captionRSSpace[LONGS_IN_HTMLRS];
  if (reflowCaption) {
    nsReflowStatus capStatus; 
    rv = OuterReflowChild(aPresContext, mCaptionFrame, aOuterRS,
                          captionRSSpace, captionMet,
                          aOuterRS.ComputedWidth(), captionSize,
                          captionMargin, capStatus);
    if (NS_FAILED(rv)) return rv;
  } else if (mCaptionFrame) {
    captionSize = mCaptionFrame->GetSize();
    GetMargin(aPresContext, aOuterRS, mCaptionFrame, aOuterRS.ComputedWidth(),
              captionMargin);
  } else {
    captionSize.SizeTo(0,0);
    captionMargin.SizeTo(0,0,0,0);
  }

  nscoord innerAvailWidth = aOuterRS.ComputedWidth();
  if (captionSide == NS_SIDE_LEFT || captionSide == NS_SIDE_RIGHT)
    
    
    innerAvailWidth -= captionMet.width + captionMargin.LeftRight();

  
  
  nsHTMLReflowMetrics innerMet;
  nsSize innerSize;
  nsMargin innerMargin;
  long innerRSSpace[LONGS_IN_HTMLRS];
  if (reflowInner) {
    rv = OuterReflowChild(aPresContext, mInnerTableFrame, aOuterRS,
                          innerRSSpace, innerMet, innerAvailWidth,
                          innerSize, innerMargin, aStatus);
    if (NS_FAILED(rv)) return rv;
  } else {
    innerSize = mInnerTableFrame->GetSize();
    GetMargin(aPresContext, aOuterRS, mInnerTableFrame,
              aOuterRS.ComputedWidth(), innerMargin);
  }

  nsSize   containSize = GetContainingBlockSize(aOuterRS);

  
  

  
  

  if (mCaptionFrame) {
    nsPoint captionOrigin;
    GetCaptionOrigin(captionSide, containSize, innerSize, 
                     innerMargin, captionSize, captionMargin, captionOrigin);
    if (reflowCaption) {
      nsHTMLReflowState *captionRS =
        NS_STATIC_CAST(nsHTMLReflowState*, (void*)captionRSSpace);
      FinishReflowChild(mCaptionFrame, aPresContext, captionRS, captionMet,
                        captionOrigin.x, captionOrigin.y, 0);
      captionRS->~nsHTMLReflowState();
    } else {
      mCaptionFrame->SetPosition(captionOrigin);
      nsTableFrame::RePositionViews(mCaptionFrame);
    }
  }
  
  

  nsPoint innerOrigin;
  GetInnerOrigin(captionSide, containSize, captionSize, 
                 captionMargin, innerSize, innerMargin, innerOrigin);
  if (reflowInner) {
    nsHTMLReflowState *innerRS =
      NS_STATIC_CAST(nsHTMLReflowState*, (void*) innerRSSpace);
    FinishReflowChild(mInnerTableFrame, aPresContext, innerRS, innerMet,
                      innerOrigin.x, innerOrigin.y, 0);
    innerRS->~nsHTMLReflowState();
  } else {
    mInnerTableFrame->SetPosition(innerOrigin);
    nsTableFrame::RePositionViews(mInnerTableFrame);
  }

  UpdateReflowMetrics(captionSide, aDesiredSize, innerMargin, captionMargin);
  
  

  NS_FRAME_SET_TRUNCATION(aStatus, aOuterRS, aDesiredSize);
  return rv;
}

#ifdef NS_DEBUG
NS_METHOD nsTableOuterFrame::VerifyTree() const
{
  return NS_OK;
}
#endif














void nsTableOuterFrame::DeleteChildsNextInFlow(nsPresContext* aPresContext, 
                                               nsIFrame*       aChild)
{
  if (!aChild) return;
  NS_PRECONDITION(mFrames.ContainsFrame(aChild), "bad geometric parent");

  nsIFrame* nextInFlow = aChild->GetNextInFlow();
  if (!nextInFlow) {
    NS_ASSERTION(PR_FALSE, "null next-in-flow");
    return;
  }

  nsTableOuterFrame* parent = NS_STATIC_CAST(nsTableOuterFrame*,
                                             nextInFlow->GetParent());
  if (!parent) {
    NS_ASSERTION(PR_FALSE, "null parent");
    return;
  }
  
  
  nsIFrame* nextNextInFlow = nextInFlow->GetNextInFlow();
  if (nextNextInFlow) {
    parent->DeleteChildsNextInFlow(aPresContext, nextInFlow);
  }

  
  nsSplittableFrame::BreakFromPrevFlow(nextInFlow);

  
  if (parent->mFrames.FirstChild() == nextInFlow) {
    parent->mFrames.SetFrames(nextInFlow->GetNextSibling());
  } else {
    
    
    
    
    
    NS_ASSERTION(aChild->GetNextSibling() == nextInFlow, "unexpected sibling");

    aChild->SetNextSibling(nextInFlow->GetNextSibling());
  }

  
  nextInFlow->Destroy();

  NS_POSTCONDITION(!aChild->GetNextInFlow(), "non null next-in-flow");
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
  if (!mInnerTableFrame) { return NS_ERROR_NOT_INITIALIZED; }
  nsITableLayout *inner;
  if (NS_SUCCEEDED(CallQueryInterface(mInnerTableFrame, &inner))) {
    return (inner->GetCellDataAt(aRowIndex, aColIndex, aCell,
                                 aStartRowIndex, aStartColIndex, 
                                 aRowSpan, aColSpan, aActualRowSpan, aActualColSpan, 
                                 aIsSelected));
  }
  return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP nsTableOuterFrame::GetTableSize(PRInt32& aRowCount, PRInt32& aColCount)
{
  if (!mInnerTableFrame) { return NS_ERROR_NOT_INITIALIZED; }
  nsITableLayout *inner;
  if (NS_SUCCEEDED(CallQueryInterface(mInnerTableFrame, &inner))) {
    return (inner->GetTableSize(aRowCount, aColCount));
  }
  return NS_ERROR_NULL_POINTER;
}




nsIFrame*
NS_NewTableOuterFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsTableOuterFrame(aContext);
}

#ifdef DEBUG
NS_IMETHODIMP
nsTableOuterFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("TableOuter"), aResult);
}
#endif

