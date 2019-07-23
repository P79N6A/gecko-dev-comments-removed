







































#include "nsHTMLContainerFrame.h"
#include "nsLegendFrame.h"
#include "nsIDOMNode.h"
#include "nsIDOMHTMLFieldSetElement.h"
#include "nsIDOMHTMLLegendElement.h"
#include "nsCSSRendering.h"

#include "nsIContent.h"
#include "nsIFrame.h"
#include "nsISupports.h"
#include "nsIAtom.h"
#include "nsPresContext.h"
#include "nsFrameManager.h"
#include "nsHTMLParts.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsFont.h"
#include "nsCOMPtr.h"
#ifdef ACCESSIBILITY
#include "nsIAccessibilityService.h"
#endif
#include "nsIServiceManager.h"
#include "nsDisplayList.h"

class nsLegendFrame;

class nsFieldSetFrame : public nsHTMLContainerFrame {
public:

  nsFieldSetFrame(nsStyleContext* aContext);

  NS_IMETHOD SetInitialChildList(nsIAtom*       aListName,
                                 nsIFrame*      aChildList);

  NS_HIDDEN_(nscoord)
    GetLegendPrefWidth(nsIRenderingContext* aRenderingContext);
  NS_HIDDEN_(nscoord)
    GetContentMinWidth(nsIRenderingContext* aRenderingContext);
  virtual nscoord GetMinWidth(nsIRenderingContext* aRenderingContext);
  virtual nscoord GetPrefWidth(nsIRenderingContext* aRenderingContext);
  virtual nsSize ComputeSize(nsIRenderingContext *aRenderingContext,
                             nsSize aCBSize, nscoord aAvailableWidth,
                             nsSize aMargin, nsSize aBorder, nsSize aPadding,
                             PRBool aShrinkWrap);

  NS_IMETHOD Reflow(nsPresContext*           aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);
                               
  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  void PaintBorderBackground(nsIRenderingContext& aRenderingContext,
    nsPoint aPt, const nsRect& aDirtyRect);

  NS_IMETHOD AppendFrames(nsIAtom*       aListName,
                          nsIFrame*      aFrameList);
  NS_IMETHOD InsertFrames(nsIAtom*       aListName,
                          nsIFrame*      aPrevFrame,
                          nsIFrame*      aFrameList);
  NS_IMETHOD RemoveFrame(nsIAtom*       aListName,
                         nsIFrame*      aOldFrame);

  virtual nsIAtom* GetType() const;
  virtual PRBool IsContainingBlock() const;

#ifdef ACCESSIBILITY  
  NS_IMETHOD  GetAccessible(nsIAccessible** aAccessible);
#endif

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const {
    return MakeFrameName(NS_LITERAL_STRING("FieldSet"), aResult);
  }
#endif

protected:

  virtual PRIntn GetSkipSides() const;
  nsIFrame* MaybeSetLegend(nsIFrame* aFrameList, nsIAtom* aListName);
  void ReParentFrameList(nsIFrame* aFrameList);

  nsIFrame* mLegendFrame;
  nsIFrame* mContentFrame;
  nsRect    mLegendRect;
  nscoord   mLegendSpace;
};

nsIFrame*
NS_NewFieldSetFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsFieldSetFrame(aContext);
}

nsFieldSetFrame::nsFieldSetFrame(nsStyleContext* aContext)
  : nsHTMLContainerFrame(aContext)
{
  mContentFrame = nsnull;
  mLegendFrame  = nsnull;
  mLegendSpace  = 0;
}

nsIAtom*
nsFieldSetFrame::GetType() const
{
  return nsGkAtoms::fieldSetFrame;
}

PRBool
nsFieldSetFrame::IsContainingBlock() const
{
  return PR_TRUE;
}

NS_IMETHODIMP
nsFieldSetFrame::SetInitialChildList(nsIAtom*       aListName,
                                     nsIFrame*      aChildList)
{
  
  if (aChildList->GetNextSibling()) {
    mContentFrame = aChildList->GetNextSibling();
    mLegendFrame  = aChildList;
  } else {
    mContentFrame = aChildList;
    mLegendFrame  = nsnull;
  }

  
  return nsHTMLContainerFrame::SetInitialChildList(nsnull, aChildList);
}

class nsDisplayFieldSetBorderBackground : public nsDisplayItem {
public:
  nsDisplayFieldSetBorderBackground(nsFieldSetFrame* aFrame)
    : nsDisplayItem(aFrame) {
    MOZ_COUNT_CTOR(nsDisplayFieldSetBorderBackground);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayFieldSetBorderBackground() {
    MOZ_COUNT_DTOR(nsDisplayFieldSetBorderBackground);
  }
#endif

  virtual nsIFrame* HitTest(nsDisplayListBuilder* aBuilder, nsPoint aPt);
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsIRenderingContext* aCtx,
     const nsRect& aDirtyRect);
  NS_DISPLAY_DECL_NAME("FieldSetBorderBackground")
};

nsIFrame* nsDisplayFieldSetBorderBackground::HitTest(nsDisplayListBuilder* aBuilder,
    nsPoint aPt)
{
  
  
  
  return mFrame;
}

void
nsDisplayFieldSetBorderBackground::Paint(nsDisplayListBuilder* aBuilder,
     nsIRenderingContext* aCtx, const nsRect& aDirtyRect)
{
  static_cast<nsFieldSetFrame*>(mFrame)->
    PaintBorderBackground(*aCtx, aBuilder->ToReferenceFrame(mFrame), aDirtyRect);
}

NS_IMETHODIMP
nsFieldSetFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                  const nsRect&           aDirtyRect,
                                  const nsDisplayListSet& aLists) {
  
  
  
  
  if (IsVisibleForPainting(aBuilder)) {
    
    
    nsresult rv = aLists.BorderBackground()->AppendNewToTop(new (aBuilder)
        nsDisplayFieldSetBorderBackground(this));
    NS_ENSURE_SUCCESS(rv, rv);
  
    rv = DisplayOutlineUnconditional(aBuilder, aLists);
    NS_ENSURE_SUCCESS(rv, rv);

    DO_GLOBAL_REFLOW_COUNT_DSP("nsFieldSetFrame");
  }

  nsDisplayListCollection contentDisplayItems;
  if (mContentFrame) {
    
    
    
    
    
    
    nsresult rv = BuildDisplayListForChild(aBuilder, mContentFrame, aDirtyRect,
                                           contentDisplayItems);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  if (mLegendFrame) {
    nsDisplayListSet set(aLists, aLists.Content());
    nsresult rv = BuildDisplayListForChild(aBuilder, mLegendFrame, aDirtyRect, set);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  
  
  
  contentDisplayItems.MoveTo(aLists);
  return NS_OK;
}

void
nsFieldSetFrame::PaintBorderBackground(nsIRenderingContext& aRenderingContext,
    nsPoint aPt, const nsRect& aDirtyRect)
{
  PRIntn skipSides = GetSkipSides();
  const nsStyleBorder* borderStyle = GetStyleBorder();
  const nsStylePadding* paddingStyle = GetStylePadding();
       
  nscoord topBorder = borderStyle->GetBorderWidth(NS_SIDE_TOP);
  nscoord yoff = 0;
  nsPresContext* presContext = PresContext();
     
  
  
  if (topBorder < mLegendRect.height)
    yoff = (mLegendRect.height - topBorder)/2;
      
  nsRect rect(aPt.x, aPt.y + yoff, mRect.width, mRect.height - yoff);

  nsCSSRendering::PaintBackground(presContext, aRenderingContext, this,
                                  aDirtyRect, rect, *borderStyle,
                                  *paddingStyle, PR_TRUE);

   if (mLegendFrame) {

    
    
    nsRect legendRect = mLegendFrame->GetRect() + aPt;
    
    
    

    
    nsRect clipRect(rect);
    clipRect.width = legendRect.x - rect.x;
    clipRect.height = topBorder;

    aRenderingContext.PushState();
    aRenderingContext.SetClipRect(clipRect, nsClipCombine_kIntersect);
    nsCSSRendering::PaintBorder(presContext, aRenderingContext, this,
                                aDirtyRect, rect, *borderStyle, mStyleContext, skipSides);

    aRenderingContext.PopState();


    
    clipRect = rect;
    clipRect.x = legendRect.XMost();
    clipRect.width = rect.XMost() - legendRect.XMost();
    clipRect.height = topBorder;

    aRenderingContext.PushState();
    aRenderingContext.SetClipRect(clipRect, nsClipCombine_kIntersect);
    nsCSSRendering::PaintBorder(presContext, aRenderingContext, this,
                                aDirtyRect, rect, *borderStyle, mStyleContext, skipSides);

    aRenderingContext.PopState();

    
    
    clipRect = rect;
    clipRect.y += topBorder;
    clipRect.height = mRect.height - (yoff + topBorder);
    
    aRenderingContext.PushState();
    aRenderingContext.SetClipRect(clipRect, nsClipCombine_kIntersect);
    nsCSSRendering::PaintBorder(presContext, aRenderingContext, this,
                                aDirtyRect, rect, *borderStyle, mStyleContext, skipSides);

    aRenderingContext.PopState();
  } else {

    nsCSSRendering::PaintBorder(presContext, aRenderingContext, this,
                                aDirtyRect,
                                nsRect(aPt, mRect.Size()),
                                *borderStyle, mStyleContext, skipSides);
  }
}



static nscoord GetCoord(const nsStyleCoord& aCoord, nscoord aIfNotCoord)
{
  return aCoord.GetUnit() == eStyleUnit_Coord
           ? aCoord.GetCoordValue()
           : aIfNotCoord;
}

nscoord
nsFieldSetFrame::GetLegendPrefWidth(nsIRenderingContext* aRenderingContext)
{
  NS_ASSERTION(mLegendFrame, "Don't call me if there is no legend frame!");

  
  
  nscoord result = mLegendFrame->GetPrefWidth(aRenderingContext);

  nsStyleCoord tmp;

  const nsStylePadding *stylePadding = mLegendFrame->GetStylePadding();
  result += GetCoord(stylePadding->mPadding.GetLeft(tmp), 0);
  result += GetCoord(stylePadding->mPadding.GetRight(tmp), 0);

  const nsStyleBorder *styleBorder = mLegendFrame->GetStyleBorder();
  result += styleBorder->GetBorderWidth(NS_SIDE_LEFT);
  result += styleBorder->GetBorderWidth(NS_SIDE_RIGHT);

  const nsStyleMargin *styleMargin = mLegendFrame->GetStyleMargin();
  result += GetCoord(styleMargin->mMargin.GetLeft(tmp), 0);
  result += GetCoord(styleMargin->mMargin.GetRight(tmp), 0);

  return result;
}

nscoord
nsFieldSetFrame::GetContentMinWidth(nsIRenderingContext* aRenderingContext)
{
  NS_ASSERTION(mContentFrame, "Don't call me if there is no legend frame!");

  return nsLayoutUtils::IntrinsicForContainer(aRenderingContext, mContentFrame,
                                              nsLayoutUtils::MIN_WIDTH);
}

nscoord
nsFieldSetFrame::GetMinWidth(nsIRenderingContext* aRenderingContext)
{
  nscoord result = 0;
  DISPLAY_MIN_WIDTH(this, result);

  nscoord legendPrefWidth = 0;
  nscoord contentMinWidth = 0;
  if (mLegendFrame) {
    legendPrefWidth = GetLegendPrefWidth(aRenderingContext);
  }

  if (mContentFrame) {
    contentMinWidth = GetContentMinWidth(aRenderingContext);
  }
      
  result = PR_MAX(legendPrefWidth, contentMinWidth);
  return result;
}

nscoord
nsFieldSetFrame::GetPrefWidth(nsIRenderingContext* aRenderingContext)
{
  nscoord result = 0;
  DISPLAY_PREF_WIDTH(this, result);

  nscoord legendPrefWidth = 0;
  nscoord contentPrefWidth = 0;
  if (mLegendFrame) {
    legendPrefWidth = GetLegendPrefWidth(aRenderingContext);
  }

  if (mContentFrame) {
    contentPrefWidth =
      nsLayoutUtils::IntrinsicForContainer(aRenderingContext, mContentFrame,
                                           nsLayoutUtils::PREF_WIDTH);
  }
      
  result = PR_MAX(legendPrefWidth, contentPrefWidth);
  return result;
}

 nsSize
nsFieldSetFrame::ComputeSize(nsIRenderingContext *aRenderingContext,
                             nsSize aCBSize, nscoord aAvailableWidth,
                             nsSize aMargin, nsSize aBorder, nsSize aPadding,
                             PRBool aShrinkWrap)
{
  nsSize result =
    nsHTMLContainerFrame::ComputeSize(aRenderingContext, aCBSize,
                                      aAvailableWidth,
                                      aMargin, aBorder, aPadding, aShrinkWrap);

  
  nscoord minWidth = GetMinWidth(aRenderingContext);
  if (minWidth > result.width)
    result.width = minWidth;

  return result;
}

NS_IMETHODIMP 
nsFieldSetFrame::Reflow(nsPresContext*           aPresContext,
                        nsHTMLReflowMetrics&     aDesiredSize,
                        const nsHTMLReflowState& aReflowState,
                        nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsFieldSetFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);

  NS_PRECONDITION(aReflowState.ComputedWidth() != NS_INTRINSICSIZE,
                  "Should have a precomputed width!");      
  
  
  aStatus = NS_FRAME_COMPLETE;

  
  PRBool reflowContent;
  PRBool reflowLegend;

  if (aReflowState.ShouldReflowAllKids()) {
    reflowContent = mContentFrame != nsnull;
    reflowLegend = mLegendFrame != nsnull;
  } else {
    reflowContent = mContentFrame && NS_SUBTREE_DIRTY(mContentFrame);

    reflowLegend = mLegendFrame && NS_SUBTREE_DIRTY(mLegendFrame);
  }

  nsSize availSize(aReflowState.ComputedWidth(), aReflowState.availableHeight);
  NS_ASSERTION(!mContentFrame ||
               GetContentMinWidth(aReflowState.rendContext) <= availSize.width,
               "Bogus availSize.width; should be bigger");

  
  const nsMargin &borderPadding = aReflowState.mComputedBorderPadding;
  const nsMargin &padding       = aReflowState.mComputedPadding;
  nsMargin border = borderPadding - padding;  

  
  
  nsMargin legendMargin(0,0,0,0);
  
  if (reflowLegend) {
    const nsStyleMargin* marginStyle = mLegendFrame->GetStyleMargin();
    marginStyle->GetMargin(legendMargin);

    
    nsSize legendAvailSize(GetLegendPrefWidth(aReflowState.rendContext),
                           NS_INTRINSICSIZE);

    nsHTMLReflowState legendReflowState(aPresContext, aReflowState,
                                        mLegendFrame,
                                        legendAvailSize);

    
    legendReflowState.
      SetComputedWidth(mLegendFrame->GetPrefWidth(aReflowState.rendContext));
    legendReflowState.SetComputedHeight(NS_INTRINSICSIZE);

    nsHTMLReflowMetrics legendDesiredSize;

    ReflowChild(mLegendFrame, aPresContext, legendDesiredSize, legendReflowState,
                0, 0, NS_FRAME_NO_MOVE_FRAME, aStatus);
#ifdef NOISY_REFLOW
    printf("  returned (%d, %d)\n", legendDesiredSize.width, legendDesiredSize.height);
#endif
    
    mLegendRect.width  = legendDesiredSize.width + legendMargin.left + legendMargin.right;
    mLegendRect.height = legendDesiredSize.height + legendMargin.top + legendMargin.bottom;
    mLegendRect.x = borderPadding.left;
    mLegendRect.y = 0;

    nscoord oldSpace = mLegendSpace;
    mLegendSpace = 0;
    if (mLegendRect.height > border.top) {
      
      mLegendSpace = mLegendRect.height - border.top;
    } else {
      mLegendRect.y = (border.top - mLegendRect.height)/2;
    }

    
    
    if (mLegendSpace != oldSpace && mContentFrame) {
      reflowContent = PR_TRUE;
    }

    
    if (NS_INTRINSICSIZE != availSize.height) {
      availSize.height -= mLegendSpace;
      availSize.height = PR_MAX(availSize.height, 0);
    }
  
    NS_ASSERTION(availSize.width >= mLegendRect.width,
                 "Bogus availSize.width.  Should be bigger");

    FinishReflowChild(mLegendFrame, aPresContext, &legendReflowState, 
                      legendDesiredSize, 0, 0, NS_FRAME_NO_MOVE_FRAME);    
  } else if (!mLegendFrame) {
    mLegendRect.Empty();
    mLegendSpace = 0;
  } 

  nsRect contentRect;

  
  if (reflowContent) {
    nsHTMLReflowState kidReflowState(aPresContext, aReflowState, mContentFrame,
                                     availSize);

    nsHTMLReflowMetrics kidDesiredSize(aDesiredSize.mFlags);
    
    ReflowChild(mContentFrame, aPresContext, kidDesiredSize, kidReflowState,
                borderPadding.left + kidReflowState.mComputedMargin.left,
                borderPadding.top + mLegendSpace + kidReflowState.mComputedMargin.top,
                0, aStatus);

    
    contentRect.SetRect(borderPadding.left,borderPadding.top + mLegendSpace,kidDesiredSize.width ,kidDesiredSize.height);
    if (aReflowState.ComputedHeight() != NS_INTRINSICSIZE &&
        borderPadding.top + mLegendSpace+kidDesiredSize.height > aReflowState.ComputedHeight()) {
      kidDesiredSize.height = aReflowState.ComputedHeight()-(borderPadding.top + mLegendSpace);
    }

    FinishReflowChild(mContentFrame, aPresContext, &kidReflowState, 
                      kidDesiredSize, contentRect.x, contentRect.y, 0);
    NS_FRAME_TRACE_REFLOW_OUT("FieldSet::Reflow", aStatus);

  } else if (mContentFrame) {
    
    
    
    contentRect = mContentFrame->GetRect();
    const nsStyleMargin* marginStyle = mContentFrame->GetStyleMargin();

    nsMargin m(0,0,0,0);
    marginStyle->GetMargin(m);
    contentRect.Inflate(m);
  } else {
    contentRect.Empty();
  }

  
  if (aReflowState.ComputedWidth() > contentRect.width) {
    contentRect.width = aReflowState.ComputedWidth();
  }

  if (mLegendFrame) {
    
    if (contentRect.width > mLegendRect.width) {
      PRInt32 align = ((nsLegendFrame*)mLegendFrame)->GetAlign();

      switch(align) {
        case NS_STYLE_TEXT_ALIGN_RIGHT:
          mLegendRect.x = contentRect.width - mLegendRect.width + borderPadding.left;
          break;
        case NS_STYLE_TEXT_ALIGN_CENTER:
          
          mLegendRect.x = contentRect.width / 2 - mLegendRect.width / 2 + borderPadding.left;
          break;
      }
  
    } else {
      
      contentRect.width = mLegendRect.width;
    }
    
    nsRect actualLegendRect(mLegendRect);
    actualLegendRect.Deflate(legendMargin);

    nsPoint curOrigin = mLegendFrame->GetPosition();

    
    if ((curOrigin.x != mLegendRect.x) || (curOrigin.y != mLegendRect.y)) {
      mLegendFrame->SetPosition(nsPoint(actualLegendRect.x , actualLegendRect.y));
      nsContainerFrame::PositionFrameView(mLegendFrame);

      
      
      nsContainerFrame::PositionChildViews(mLegendFrame);
    }
  }

  
  if (aReflowState.ComputedHeight() == NS_INTRINSICSIZE) {
    aDesiredSize.height = mLegendSpace + 
                          borderPadding.TopBottom() +
                          contentRect.height;
  } else {
    nscoord min = borderPadding.TopBottom() + mLegendRect.height;
    aDesiredSize.height =
      aReflowState.ComputedHeight() + borderPadding.TopBottom();
    if (aDesiredSize.height < min)
      aDesiredSize.height = min;
  }
  aDesiredSize.width = contentRect.width + borderPadding.LeftRight();
  aDesiredSize.mOverflowArea = nsRect(0, 0, aDesiredSize.width, aDesiredSize.height);
  if (mLegendFrame)
    ConsiderChildOverflow(aDesiredSize.mOverflowArea, mLegendFrame);
  if (mContentFrame)
    ConsiderChildOverflow(aDesiredSize.mOverflowArea, mContentFrame);
  FinishAndStoreOverflow(&aDesiredSize);

  Invalidate(aDesiredSize.mOverflowArea);

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  return NS_OK;
}

PRIntn
nsFieldSetFrame::GetSkipSides() const
{
  return 0;
}

NS_IMETHODIMP
nsFieldSetFrame::AppendFrames(nsIAtom*       aListName,
                              nsIFrame*      aFrameList)
{
  aFrameList = MaybeSetLegend(aFrameList, aListName);
  if (aFrameList) {
    ReParentFrameList(aFrameList);
    return mContentFrame->AppendFrames(aListName, aFrameList);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsFieldSetFrame::InsertFrames(nsIAtom*       aListName,
                              nsIFrame*      aPrevFrame,
                              nsIFrame*      aFrameList)
{
  NS_ASSERTION(!aPrevFrame || aPrevFrame->GetParent() == this ||
               aPrevFrame->GetParent() == mContentFrame,
               "inserting after sibling frame with different parent");

  aFrameList = MaybeSetLegend(aFrameList, aListName);
  if (aFrameList) {
    ReParentFrameList(aFrameList);
    if (NS_UNLIKELY(aPrevFrame == mLegendFrame)) {
      aPrevFrame = nsnull;
    }
    return mContentFrame->InsertFrames(aListName, aPrevFrame, aFrameList);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsFieldSetFrame::RemoveFrame(nsIAtom*       aListName,
                             nsIFrame*      aOldFrame)
{
  
  if (aOldFrame == mLegendFrame) {
    NS_ASSERTION(!aListName, "Unexpected frame list when removing legend frame");
    NS_ASSERTION(mLegendFrame->GetParent() == this, "Legend Parent has wrong parent");
    NS_ASSERTION(mLegendFrame->GetNextSibling() == mContentFrame, "mContentFrame is not next sibling");

    mFrames.DestroyFrame(mLegendFrame);
    mLegendFrame = nsnull;
    PresContext()->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eTreeChange, NS_FRAME_IS_DIRTY);
    return NS_OK;
  }
  return mContentFrame->RemoveFrame(aListName, aOldFrame);
}

#ifdef ACCESSIBILITY
NS_IMETHODIMP nsFieldSetFrame::GetAccessible(nsIAccessible** aAccessible)
{
  nsCOMPtr<nsIAccessibilityService> accService = do_GetService("@mozilla.org/accessibilityService;1");

  if (accService) {
    return accService->CreateHTMLGroupboxAccessible(static_cast<nsIFrame*>(this), aAccessible);
  }

  return NS_ERROR_FAILURE;
}
#endif

nsIFrame*
nsFieldSetFrame::MaybeSetLegend(nsIFrame* aFrameList, nsIAtom* aListName)
{
  if (!mLegendFrame && aFrameList->GetType() == nsGkAtoms::legendFrame) {
    NS_ASSERTION(!aListName, "Unexpected frame list when adding legend frame");
    mLegendFrame = aFrameList;
    aFrameList = mLegendFrame->GetNextSibling();
    mLegendFrame->SetNextSibling(mContentFrame);
    mFrames.SetFrames(mLegendFrame);
    PresContext()->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                       NS_FRAME_HAS_DIRTY_CHILDREN);
  }
  return aFrameList;
}

void
nsFieldSetFrame::ReParentFrameList(nsIFrame* aFrameList)
{
  nsFrameManager* frameManager = PresContext()->FrameManager();
  for (nsIFrame* frame = aFrameList; frame; frame = frame->GetNextSibling()) {
    frame->SetParent(mContentFrame);
    frameManager->ReParentStyleContext(frame);
  }
  mContentFrame->AddStateBits(GetStateBits() & NS_FRAME_HAS_CHILD_WITH_VIEW);
}
