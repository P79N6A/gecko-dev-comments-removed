







































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
#include "nsAccessibilityService.h"
#endif
#include "nsIServiceManager.h"
#include "nsDisplayList.h"
#include "nsRenderingContext.h"

class nsLegendFrame;

class nsFieldSetFrame : public nsHTMLContainerFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  nsFieldSetFrame(nsStyleContext* aContext);

  NS_IMETHOD SetInitialChildList(ChildListID    aListID,
                                 nsFrameList&   aChildList);

  NS_HIDDEN_(nscoord)
    GetIntrinsicWidth(nsRenderingContext* aRenderingContext,
                      nsLayoutUtils::IntrinsicWidthType);
  virtual nscoord GetMinWidth(nsRenderingContext* aRenderingContext);
  virtual nscoord GetPrefWidth(nsRenderingContext* aRenderingContext);
  virtual nsSize ComputeSize(nsRenderingContext *aRenderingContext,
                             nsSize aCBSize, nscoord aAvailableWidth,
                             nsSize aMargin, nsSize aBorder, nsSize aPadding,
                             bool aShrinkWrap);
  virtual nscoord GetBaseline() const;

  NS_IMETHOD Reflow(nsPresContext*           aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);
                               
  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  void PaintBorderBackground(nsRenderingContext& aRenderingContext,
    nsPoint aPt, const nsRect& aDirtyRect, PRUint32 aBGFlags);

  NS_IMETHOD AppendFrames(ChildListID    aListID,
                          nsFrameList&   aFrameList);
  NS_IMETHOD InsertFrames(ChildListID    aListID,
                          nsIFrame*      aPrevFrame,
                          nsFrameList&   aFrameList);
  NS_IMETHOD RemoveFrame(ChildListID    aListID,
                         nsIFrame*      aOldFrame);

  virtual nsIAtom* GetType() const;

#ifdef ACCESSIBILITY  
  virtual already_AddRefed<nsAccessible> CreateAccessible();
#endif

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const {
    return MakeFrameName(NS_LITERAL_STRING("FieldSet"), aResult);
  }
#endif

protected:

  virtual PRIntn GetSkipSides() const;
  void ReparentFrameList(const nsFrameList& aFrameList);

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

NS_IMETHODIMP
nsFieldSetFrame::SetInitialChildList(ChildListID    aListID,
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

  
  return nsHTMLContainerFrame::SetInitialChildList(kPrincipalList, aChildList);
}

class nsDisplayFieldSetBorderBackground : public nsDisplayItem {
public:
  nsDisplayFieldSetBorderBackground(nsDisplayListBuilder* aBuilder,
                                    nsFieldSetFrame* aFrame)
    : nsDisplayItem(aBuilder, aFrame) {
    MOZ_COUNT_CTOR(nsDisplayFieldSetBorderBackground);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayFieldSetBorderBackground() {
    MOZ_COUNT_DTOR(nsDisplayFieldSetBorderBackground);
  }
#endif

  virtual void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                       HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames);
  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx);
  NS_DISPLAY_DECL_NAME("FieldSetBorderBackground", TYPE_FIELDSET_BORDER_BACKGROUND)
};

void nsDisplayFieldSetBorderBackground::HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                                                HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames)
{
  
  
  
  aOutFrames->AppendElement(mFrame);
}

void
nsDisplayFieldSetBorderBackground::Paint(nsDisplayListBuilder* aBuilder,
                                         nsRenderingContext* aCtx)
{
  static_cast<nsFieldSetFrame*>(mFrame)->
    PaintBorderBackground(*aCtx, ToReferenceFrame(),
                          mVisibleRect, aBuilder->GetBackgroundPaintFlags());
}

NS_IMETHODIMP
nsFieldSetFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                  const nsRect&           aDirtyRect,
                                  const nsDisplayListSet& aLists) {
  
  
  
  
  if (IsVisibleForPainting(aBuilder)) {
    if (GetStyleBorder()->mBoxShadow) {
      nsresult rv = aLists.BorderBackground()->AppendNewToTop(new (aBuilder)
          nsDisplayBoxShadowOuter(aBuilder, this));
      NS_ENSURE_SUCCESS(rv, rv);
    }

    
    
    nsresult rv = aLists.BorderBackground()->AppendNewToTop(new (aBuilder)
        nsDisplayFieldSetBorderBackground(aBuilder, this));
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
nsFieldSetFrame::PaintBorderBackground(nsRenderingContext& aRenderingContext,
    nsPoint aPt, const nsRect& aDirtyRect, PRUint32 aBGFlags)
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
                                  aDirtyRect, rect, aBGFlags);

  nsCSSRendering::PaintBoxShadowInner(presContext, aRenderingContext,
                                      this, rect, aDirtyRect);

   if (mLegendFrame) {

    
    
    nsRect legendRect = mLegendFrame->GetRect() + aPt;
    
    
    

    
    nsRect clipRect(rect);
    clipRect.width = legendRect.x - rect.x;
    clipRect.height = topBorder;

    aRenderingContext.PushState();
    aRenderingContext.IntersectClip(clipRect);
    nsCSSRendering::PaintBorder(presContext, aRenderingContext, this,
                                aDirtyRect, rect, mStyleContext, skipSides);

    aRenderingContext.PopState();


    
    clipRect = rect;
    clipRect.x = legendRect.XMost();
    clipRect.width = rect.XMost() - legendRect.XMost();
    clipRect.height = topBorder;

    aRenderingContext.PushState();
    aRenderingContext.IntersectClip(clipRect);
    nsCSSRendering::PaintBorder(presContext, aRenderingContext, this,
                                aDirtyRect, rect, mStyleContext, skipSides);

    aRenderingContext.PopState();

    
    
    clipRect = rect;
    clipRect.y += topBorder;
    clipRect.height = mRect.height - (yoff + topBorder);
    
    aRenderingContext.PushState();
    aRenderingContext.IntersectClip(clipRect);
    nsCSSRendering::PaintBorder(presContext, aRenderingContext, this,
                                aDirtyRect, rect, mStyleContext, skipSides);

    aRenderingContext.PopState();
  } else {

    nsCSSRendering::PaintBorder(presContext, aRenderingContext, this,
                                aDirtyRect,
                                nsRect(aPt, mRect.Size()),
                                mStyleContext, skipSides);
  }
}

nscoord
nsFieldSetFrame::GetIntrinsicWidth(nsRenderingContext* aRenderingContext,
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
      
  return NS_MAX(legendWidth, contentWidth);
}


nscoord
nsFieldSetFrame::GetMinWidth(nsRenderingContext* aRenderingContext)
{
  nscoord result = 0;
  DISPLAY_MIN_WIDTH(this, result);

  result = GetIntrinsicWidth(aRenderingContext, nsLayoutUtils::MIN_WIDTH);
  return result;
}

nscoord
nsFieldSetFrame::GetPrefWidth(nsRenderingContext* aRenderingContext)
{
  nscoord result = 0;
  DISPLAY_PREF_WIDTH(this, result);

  result = GetIntrinsicWidth(aRenderingContext, nsLayoutUtils::PREF_WIDTH);
  return result;
}

 nsSize
nsFieldSetFrame::ComputeSize(nsRenderingContext *aRenderingContext,
                             nsSize aCBSize, nscoord aAvailableWidth,
                             nsSize aMargin, nsSize aBorder, nsSize aPadding,
                             bool aShrinkWrap)
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

  
  bool reflowContent;
  bool reflowLegend;

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
    mLegendRect.SetEmpty();
    mLegendSpace = 0;
  } else {
    
    
    legendMargin = mLegendFrame->GetUsedMargin();
  }

  
  if (reflowContent) {
    nsHTMLReflowState kidReflowState(aPresContext, aReflowState, mContentFrame,
                                     availSize);
    
    
    
    if (aReflowState.ComputedHeight() != NS_UNCONSTRAINEDSIZE) {
      kidReflowState.SetComputedHeight(NS_MAX(0, aReflowState.ComputedHeight() - mLegendSpace));
    }

    kidReflowState.mComputedMinHeight =
      NS_MAX(0, aReflowState.mComputedMinHeight - mLegendSpace);

    if (aReflowState.mComputedMaxHeight != NS_UNCONSTRAINEDSIZE) {
      kidReflowState.mComputedMaxHeight =
        NS_MAX(0, aReflowState.mComputedMaxHeight - mLegendSpace);
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
  aDesiredSize.SetOverflowAreasToDesiredBounds();
  if (mLegendFrame)
    ConsiderChildOverflow(aDesiredSize.mOverflowAreas, mLegendFrame);
  if (mContentFrame)
    ConsiderChildOverflow(aDesiredSize.mOverflowAreas, mContentFrame);
  FinishAndStoreOverflow(&aDesiredSize);

  Invalidate(aDesiredSize.VisualOverflow());

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  return NS_OK;
}

PRIntn
nsFieldSetFrame::GetSkipSides() const
{
  return 0;
}

NS_IMETHODIMP
nsFieldSetFrame::AppendFrames(ChildListID    aListID,
                              nsFrameList&   aFrameList)
{
  
  ReparentFrameList(aFrameList);
  return mContentFrame->AppendFrames(aListID, aFrameList);
}

NS_IMETHODIMP
nsFieldSetFrame::InsertFrames(ChildListID    aListID,
                              nsIFrame*      aPrevFrame,
                              nsFrameList&   aFrameList)
{
  NS_ASSERTION(!aPrevFrame || aPrevFrame->GetParent() == this ||
               aPrevFrame->GetParent() == mContentFrame,
               "inserting after sibling frame with different parent");

  
  ReparentFrameList(aFrameList);
  if (NS_UNLIKELY(aPrevFrame == mLegendFrame)) {
    aPrevFrame = nsnull;
  }
  return mContentFrame->InsertFrames(aListID, aPrevFrame, aFrameList);
}

NS_IMETHODIMP
nsFieldSetFrame::RemoveFrame(ChildListID    aListID,
                             nsIFrame*      aOldFrame)
{
  
  NS_ASSERTION(aOldFrame != mLegendFrame, "Cannot remove mLegendFrame here");
  return mContentFrame->RemoveFrame(aListID, aOldFrame);
}

#ifdef ACCESSIBILITY
already_AddRefed<nsAccessible>
nsFieldSetFrame::CreateAccessible()
{
  nsAccessibilityService* accService = nsIPresShell::AccService();
  if (accService) {
    return accService->CreateHTMLGroupboxAccessible(mContent,
                                                    PresContext()->PresShell());
  }

  return nsnull;
}
#endif

void
nsFieldSetFrame::ReparentFrameList(const nsFrameList& aFrameList)
{
  nsFrameManager* frameManager = PresContext()->FrameManager();
  for (nsFrameList::Enumerator e(aFrameList); !e.AtEnd(); e.Next()) {
    NS_ASSERTION(mLegendFrame || e.get()->GetType() != nsGkAtoms::legendFrame,
                 "The fieldset's legend is not allowed in this list");
    e.get()->SetParent(mContentFrame);
    frameManager->ReparentStyleContext(e.get());
  }
}

nscoord
nsFieldSetFrame::GetBaseline() const
{
  
  
  NS_ASSERTION(nsLayoutUtils::GetAsBlock(mContentFrame),
               "Unexpected mContentFrame");
  return mContentFrame->GetPosition().y + mContentFrame->GetBaseline();
}
