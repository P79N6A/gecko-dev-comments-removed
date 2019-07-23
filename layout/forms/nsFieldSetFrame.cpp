







































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
  NS_DECL_FRAMEARENA_HELPERS

  nsFieldSetFrame(nsStyleContext* aContext);

  NS_IMETHOD SetInitialChildList(nsIAtom*       aListName,
                                 nsFrameList&   aChildList);

  NS_HIDDEN_(nscoord)
    GetIntrinsicWidth(nsIRenderingContext* aRenderingContext,
                      nsLayoutUtils::IntrinsicWidthType);
  virtual nscoord GetMinWidth(nsIRenderingContext* aRenderingContext);
  virtual nscoord GetPrefWidth(nsIRenderingContext* aRenderingContext);
  virtual nsSize ComputeSize(nsIRenderingContext *aRenderingContext,
                             nsSize aCBSize, nscoord aAvailableWidth,
                             nsSize aMargin, nsSize aBorder, nsSize aPadding,
                             PRBool aShrinkWrap);
  virtual nscoord GetBaseline() const;

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
                          nsFrameList&   aFrameList);
  NS_IMETHOD InsertFrames(nsIAtom*       aListName,
                          nsIFrame*      aPrevFrame,
                          nsFrameList&   aFrameList);
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
  void ReParentFrameList(const nsFrameList& aFrameList);

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

NS_IMPL_FRAMEARENA_HELPERS(nsFieldSetFrame)

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
                                     nsFrameList&   aChildList)
{
  
  if (!aChildList.OnlyChild()) {
    NS_ASSERTION(aChildList.GetLength() == 2, "Unexpected child list");
    mContentFrame = aChildList.LastChild();
    mLegendFrame  = aChildList.FirstChild();
  } else {
    mContentFrame = aChildList.FirstChild();
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

  virtual nsIFrame* HitTest(nsDisplayListBuilder* aBuilder, nsPoint aPt,
                            HitTestState* aState);
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsIRenderingContext* aCtx,
     const nsRect& aDirtyRect);
  NS_DISPLAY_DECL_NAME("FieldSetBorderBackground")
};

nsIFrame* nsDisplayFieldSetBorderBackground::HitTest(nsDisplayListBuilder* aBuilder,
    nsPoint aPt, HitTestState* aState)
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
    if (GetStyleBorder()->mBoxShadow) {
      nsresult rv = aLists.BorderBackground()->AppendNewToTop(new (aBuilder)
          nsDisplayBoxShadowOuter(this));
      NS_ENSURE_SUCCESS(rv, rv);
    }

    
    
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
    
    
    nsDisplayListSet set(aLists, aLists.BlockBorderBackgrounds());
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
       
  nscoord topBorder = borderStyle->GetActualBorderWidth(NS_SIDE_TOP);
  nscoord yoff = 0;
  nsPresContext* presContext = PresContext();
     
  
  
  if (topBorder < mLegendRect.height)
    yoff = (mLegendRect.height - topBorder)/2;
      
  nsRect rect(aPt.x, aPt.y + yoff, mRect.width, mRect.height - yoff);

  nsCSSRendering::PaintBackground(presContext, aRenderingContext, this,
                                  aDirtyRect, rect, 0);

  nsCSSRendering::PaintBoxShadowInner(presContext, aRenderingContext,
                                      this, rect, aDirtyRect);

   if (mLegendFrame) {

    
    
    nsRect legendRect = mLegendFrame->GetRect() + aPt;
    
    
    

    
    nsRect clipRect(rect);
    clipRect.width = legendRect.x - rect.x;
    clipRect.height = topBorder;

    aRenderingContext.PushState();
    aRenderingContext.SetClipRect(clipRect, nsClipCombine_kIntersect);
    nsCSSRendering::PaintBorder(presContext, aRenderingContext, this,
                                aDirtyRect, rect, *borderStyle, mStyleContext,
                                skipSides);

    aRenderingContext.PopState();


    
    clipRect = rect;
    clipRect.x = legendRect.XMost();
    clipRect.width = rect.XMost() - legendRect.XMost();
    clipRect.height = topBorder;

    aRenderingContext.PushState();
    aRenderingContext.SetClipRect(clipRect, nsClipCombine_kIntersect);
    nsCSSRendering::PaintBorder(presContext, aRenderingContext, this,
                                aDirtyRect, rect, *borderStyle, mStyleContext,
                                skipSides);

    aRenderingContext.PopState();

    
    
    clipRect = rect;
    clipRect.y += topBorder;
    clipRect.height = mRect.height - (yoff + topBorder);
    
    aRenderingContext.PushState();
    aRenderingContext.SetClipRect(clipRect, nsClipCombine_kIntersect);
    nsCSSRendering::PaintBorder(presContext, aRenderingContext, this,
                                aDirtyRect, rect, *borderStyle, mStyleContext,
                                skipSides);

    aRenderingContext.PopState();
  } else {

    nsCSSRendering::PaintBorder(presContext, aRenderingContext, this,
                                aDirtyRect,
                                nsRect(aPt, mRect.Size()),
                                *borderStyle, mStyleContext, skipSides);
  }
}

nscoord
nsFieldSetFrame::GetIntrinsicWidth(nsIRenderingContext* aRenderingContext,
                                   nsLayoutUtils::IntrinsicWidthType aType)
{
  nscoord legendWidth = 0;
  nscoord contentWidth = 0;
  if (mLegendFrame) {
    legendWidth =
      nsLayoutUtils::IntrinsicForContainer(aRenderingContext, mLegendFrame,
                                           aType);
  }

  if (mContentFrame) {
    contentWidth =
      nsLayoutUtils::IntrinsicForContainer(aRenderingContext, mContentFrame,
                                           aType);
  }
      
  return PR_MAX(legendWidth, contentWidth);
}


nscoord
nsFieldSetFrame::GetMinWidth(nsIRenderingContext* aRenderingContext)
{
  nscoord result = 0;
  DISPLAY_MIN_WIDTH(this, result);

  result = GetIntrinsicWidth(aRenderingContext, nsLayoutUtils::MIN_WIDTH);
  return result;
}

nscoord
nsFieldSetFrame::GetPrefWidth(nsIRenderingContext* aRenderingContext)
{
  nscoord result = 0;
  DISPLAY_PREF_WIDTH(this, result);

  result = GetIntrinsicWidth(aRenderingContext, nsLayoutUtils::PREF_WIDTH);
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

  
  
  nsSize availSize(aReflowState.ComputedWidth(), NS_UNCONSTRAINEDSIZE);
  NS_ASSERTION(!mContentFrame ||
      nsLayoutUtils::IntrinsicForContainer(aReflowState.rendContext,
                                           mContentFrame,
                                           nsLayoutUtils::MIN_WIDTH) <=
               availSize.width,
               "Bogus availSize.width; should be bigger");
  NS_ASSERTION(!mLegendFrame ||
      nsLayoutUtils::IntrinsicForContainer(aReflowState.rendContext,
                                           mLegendFrame,
                                           nsLayoutUtils::MIN_WIDTH) <=
               availSize.width,
               "Bogus availSize.width; should be bigger");

  
  const nsMargin &borderPadding = aReflowState.mComputedBorderPadding;
  nsMargin border = borderPadding - aReflowState.mComputedPadding;  

  
  
  nsMargin legendMargin(0,0,0,0);
  
  if (reflowLegend) {
    nsHTMLReflowState legendReflowState(aPresContext, aReflowState,
                                        mLegendFrame, availSize);

    nsHTMLReflowMetrics legendDesiredSize;

    ReflowChild(mLegendFrame, aPresContext, legendDesiredSize, legendReflowState,
                0, 0, NS_FRAME_NO_MOVE_FRAME, aStatus);
#ifdef NOISY_REFLOW
    printf("  returned (%d, %d)\n", legendDesiredSize.width, legendDesiredSize.height);
#endif
    
    legendMargin = mLegendFrame->GetUsedMargin();
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

    FinishReflowChild(mLegendFrame, aPresContext, &legendReflowState, 
                      legendDesiredSize, 0, 0, NS_FRAME_NO_MOVE_FRAME);    
  } else if (!mLegendFrame) {
    mLegendRect.Empty();
    mLegendSpace = 0;
  } else {
    
    
    legendMargin = mLegendFrame->GetUsedMargin();
  }

  
  if (reflowContent) {
    nsHTMLReflowState kidReflowState(aPresContext, aReflowState, mContentFrame,
                                     availSize);
    
    
    
    if (aReflowState.ComputedHeight() != NS_UNCONSTRAINEDSIZE) {
      kidReflowState.SetComputedHeight(PR_MAX(0, aReflowState.ComputedHeight() - mLegendSpace));
    }

    kidReflowState.mComputedMinHeight =
      PR_MAX(0, aReflowState.mComputedMinHeight - mLegendSpace);

    if (aReflowState.mComputedMaxHeight != NS_UNCONSTRAINEDSIZE) {
      kidReflowState.mComputedMaxHeight =
        PR_MAX(0, aReflowState.mComputedMaxHeight - mLegendSpace);
    }

    nsHTMLReflowMetrics kidDesiredSize(aDesiredSize.mFlags);
    
    NS_ASSERTION(kidReflowState.mComputedMargin == nsMargin(0,0,0,0),
                 "Margins on anonymous fieldset child not supported!");
    nsPoint pt(borderPadding.left, borderPadding.top + mLegendSpace);
    ReflowChild(mContentFrame, aPresContext, kidDesiredSize, kidReflowState,
                pt.x, pt.y, 0, aStatus);

    FinishReflowChild(mContentFrame, aPresContext, &kidReflowState, 
                      kidDesiredSize, pt.x, pt.y, 0);
    NS_FRAME_TRACE_REFLOW_OUT("FieldSet::Reflow", aStatus);
  }

  nsRect contentRect(0,0,0,0);
  if (mContentFrame) {
    
    
    contentRect = mContentFrame->GetRect();
  }

  
  if (aReflowState.ComputedWidth() > contentRect.width) {
    contentRect.width = aReflowState.ComputedWidth();
  }

  if (mLegendFrame) {
    
    if (contentRect.width > mLegendRect.width) {
      PRInt32 align = static_cast<nsLegendFrame*>(mLegendFrame)->GetAlign();

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
                              nsFrameList&   aFrameList)
{
  
  ReParentFrameList(aFrameList);
  return mContentFrame->AppendFrames(aListName, aFrameList);
}

NS_IMETHODIMP
nsFieldSetFrame::InsertFrames(nsIAtom*       aListName,
                              nsIFrame*      aPrevFrame,
                              nsFrameList&   aFrameList)
{
  NS_ASSERTION(!aPrevFrame || aPrevFrame->GetParent() == this ||
               aPrevFrame->GetParent() == mContentFrame,
               "inserting after sibling frame with different parent");

  
  ReParentFrameList(aFrameList);
  if (NS_UNLIKELY(aPrevFrame == mLegendFrame)) {
    aPrevFrame = nsnull;
  }
  return mContentFrame->InsertFrames(aListName, aPrevFrame, aFrameList);
}

NS_IMETHODIMP
nsFieldSetFrame::RemoveFrame(nsIAtom*       aListName,
                             nsIFrame*      aOldFrame)
{
  
  NS_ASSERTION(aOldFrame != mLegendFrame, "Cannot remove mLegendFrame here");
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

void
nsFieldSetFrame::ReParentFrameList(const nsFrameList& aFrameList)
{
  nsFrameManager* frameManager = PresContext()->FrameManager();
  for (nsFrameList::Enumerator e(aFrameList); !e.AtEnd(); e.Next()) {
    NS_ASSERTION(mLegendFrame || e.get()->GetType() != nsGkAtoms::legendFrame,
                 "The fieldset's legend is not allowed in this list");
    e.get()->SetParent(mContentFrame);
    frameManager->ReParentStyleContext(e.get());
  }
  mContentFrame->AddStateBits(GetStateBits() & NS_FRAME_HAS_CHILD_WITH_VIEW);
}

nscoord
nsFieldSetFrame::GetBaseline() const
{
  
  
  NS_ASSERTION(nsLayoutUtils::GetAsBlock(mContentFrame),
               "Unexpected mContentFrame");
  return mContentFrame->GetPosition().y + mContentFrame->GetBaseline();
}
