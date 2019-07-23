






































#include "nsIServiceManager.h"
#include "nsHTMLParts.h"
#include "nsHTMLContainerFrame.h"
#include "nsCSSRendering.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsIRenderingContext.h"
#include "nsGUIEvent.h"
#include "nsStyleConsts.h"
#include "nsGkAtoms.h"
#include "nsIEventStateManager.h"
#include "nsIDeviceContext.h"
#include "nsIPresShell.h"
#include "nsIScrollPositionListener.h"
#include "nsDisplayList.h"


#include "nsIDOMWindowInternal.h"
#include "nsIFocusController.h"
#include "nsIScrollableFrame.h"
#include "nsIScrollableView.h"
#include "nsIDocShell.h"
#include "nsICanvasFrame.h"

#ifdef DEBUG_rods

#endif










class CanvasFrame : public nsHTMLContainerFrame, 
                    public nsIScrollPositionListener, 
                    public nsICanvasFrame {
public:
  CanvasFrame(nsStyleContext* aContext)
  : nsHTMLContainerFrame(aContext), mDoPaintFocus(PR_FALSE) {}

   
  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);

  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);
  virtual void Destroy();

  NS_IMETHOD AppendFrames(nsIAtom*        aListName,
                          nsIFrame*       aFrameList);
  NS_IMETHOD InsertFrames(nsIAtom*        aListName,
                          nsIFrame*       aPrevFrame,
                          nsIFrame*       aFrameList);
  NS_IMETHOD RemoveFrame(nsIAtom*        aListName,
                         nsIFrame*       aOldFrame);

  virtual nscoord GetMinWidth(nsIRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsIRenderingContext *aRenderingContext);
  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);
  virtual PRBool IsContainingBlock() const { return PR_TRUE; }

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  void PaintFocus(nsIRenderingContext& aRenderingContext, nsPoint aPt);

  
  NS_IMETHOD ScrollPositionWillChange(nsIScrollableView* aScrollable, nscoord aX, nscoord aY);
	NS_IMETHOD ScrollPositionDidChange(nsIScrollableView* aScrollable, nscoord aX, nscoord aY);

  
  NS_IMETHOD SetHasFocus(PRBool aHasFocus);

  




  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif
  NS_IMETHOD GetContentForEvent(nsPresContext* aPresContext,
                                nsEvent* aEvent,
                                nsIContent** aContent);

  nsRect CanvasArea() const;

protected:
  virtual PRIntn GetSkipSides() const;

  
  PRPackedBool             mDoPaintFocus;
  nsCOMPtr<nsIViewManager> mViewManager;

private:
  NS_IMETHOD_(nsrefcnt) AddRef() { return NS_OK; }
  NS_IMETHOD_(nsrefcnt) Release() { return NS_OK; }
};




nsIFrame*
NS_NewCanvasFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell)CanvasFrame(aContext);
}



NS_IMETHODIMP
CanvasFrame::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  NS_PRECONDITION(0 != aInstancePtr, "null ptr");
  if (NULL == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }

  if (aIID.Equals(NS_GET_IID(nsIScrollPositionListener))) {
    *aInstancePtr = (void*) ((nsIScrollPositionListener*) this);
    return NS_OK;
  } 
  
  if (aIID.Equals(NS_GET_IID(nsICanvasFrame))) {
    *aInstancePtr = (void*) ((nsICanvasFrame*) this);
    return NS_OK;
  } 
  

  return nsHTMLContainerFrame::QueryInterface(aIID, aInstancePtr);
}

NS_IMETHODIMP
CanvasFrame::Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow)
{
  nsresult rv = nsHTMLContainerFrame::Init(aContent, aParent, aPrevInFlow);

  mViewManager = GetPresContext()->GetViewManager();

  nsIScrollableView* scrollingView = nsnull;
  mViewManager->GetRootScrollableView(&scrollingView);
  if (scrollingView) {
    scrollingView->AddScrollPositionListener(this);
  }

  return rv;
}

void
CanvasFrame::Destroy()
{
  nsIScrollableView* scrollingView = nsnull;
  mViewManager->GetRootScrollableView(&scrollingView);
  if (scrollingView) {
    scrollingView->RemoveScrollPositionListener(this);
  }

  nsHTMLContainerFrame::Destroy();
}

NS_IMETHODIMP
CanvasFrame::ScrollPositionWillChange(nsIScrollableView* aScrollable, nscoord aX, nscoord aY)
{
#ifdef DEBUG_CANVAS_FOCUS
  {
    PRBool hasFocus = PR_FALSE;
    nsCOMPtr<nsIViewObserver> observer;
    mViewManager->GetViewObserver(*getter_AddRefs(observer));
    nsCOMPtr<nsIPresShell> shell = do_QueryInterface(observer);
    nsCOMPtr<nsPresContext> context;
    shell->GetPresContext(getter_AddRefs(context));
    nsCOMPtr<nsISupports> container;
    context->GetContainer(getter_AddRefs(container));
    nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(container));
    if (docShell) {
      docShell->GetHasFocus(&hasFocus);
    }
    printf("SPWC: %p  HF: %s  mDoPaintFocus: %s\n", docShell.get(), hasFocus?"Y":"N", mDoPaintFocus?"Y":"N");
  }
#endif

  if (mDoPaintFocus) {
    mDoPaintFocus = PR_FALSE;
    mViewManager->UpdateAllViews(NS_VMREFRESH_NO_SYNC);
  }
  return NS_OK;
}

NS_IMETHODIMP
CanvasFrame::ScrollPositionDidChange(nsIScrollableView* aScrollable, nscoord aX, nscoord aY)
{
  return NS_OK;
}

NS_IMETHODIMP
CanvasFrame::SetHasFocus(PRBool aHasFocus)
{
  if (mDoPaintFocus != aHasFocus) {
    mDoPaintFocus = aHasFocus;
    nsIViewManager* vm = GetPresContext()->PresShell()->GetViewManager();
    if (vm) {
      vm->UpdateAllViews(NS_VMREFRESH_NO_SYNC);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
CanvasFrame::AppendFrames(nsIAtom*        aListName,
                          nsIFrame*       aFrameList)
{
  nsresult  rv;

  NS_ASSERTION(!aListName, "unexpected child list name");
  NS_PRECONDITION(mFrames.IsEmpty(), "already have a child frame");
  if (aListName) {
    
    rv = NS_ERROR_INVALID_ARG;

  } else if (!mFrames.IsEmpty()) {
    
    rv = NS_ERROR_FAILURE;

  } else {
    
#ifdef NS_DEBUG
    nsFrame::VerifyDirtyBitSet(aFrameList);
#endif
    mFrames.AppendFrame(nsnull, aFrameList);

    rv = GetPresContext()->PresShell()->
           FrameNeedsReflow(this, nsIPresShell::eTreeChange);
  }

  return rv;
}

NS_IMETHODIMP
CanvasFrame::InsertFrames(nsIAtom*        aListName,
                          nsIFrame*       aPrevFrame,
                          nsIFrame*       aFrameList)
{
  nsresult  rv;

  
  
  NS_PRECONDITION(!aPrevFrame, "unexpected previous sibling frame");
  if (aPrevFrame) {
    rv = NS_ERROR_UNEXPECTED;
  } else {
    rv = AppendFrames(aListName, aFrameList);
  }

  return rv;
}

NS_IMETHODIMP
CanvasFrame::RemoveFrame(nsIAtom*        aListName,
                         nsIFrame*       aOldFrame)
{
  nsresult  rv;

  NS_ASSERTION(!aListName, "unexpected child list name");
  if (aListName) {
    
    rv = NS_ERROR_INVALID_ARG;
  
  } else if (aOldFrame == mFrames.FirstChild()) {
    
    
    
    
    Invalidate(aOldFrame->GetOverflowRect() + aOldFrame->GetPosition(), PR_FALSE);

    
    mFrames.DestroyFrame(aOldFrame);

    AddStateBits(NS_FRAME_HAS_DIRTY_CHILDREN);
    rv = GetPresContext()->PresShell()->
           FrameNeedsReflow(this, nsIPresShell::eTreeChange);
  } else {
    rv = NS_ERROR_FAILURE;
  }

  return rv;
}

nsRect CanvasFrame::CanvasArea() const
{
  nsRect result(GetOverflowRect());

  nsIScrollableFrame *scrollableFrame;
  CallQueryInterface(GetParent(), &scrollableFrame);
  if (scrollableFrame) {
    nsIScrollableView* scrollableView = scrollableFrame->GetScrollableView();
    nsRect vcr = scrollableView->View()->GetBounds();
    result.UnionRect(result, nsRect(nsPoint(0, 0), vcr.Size()));
  }
  return result;
}





class nsDisplayCanvasBackground : public nsDisplayBackground {
public:
  nsDisplayCanvasBackground(nsIFrame *aFrame)
    : nsDisplayBackground(aFrame)
  {
  }

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder)
  {
    CanvasFrame* frame = NS_STATIC_CAST(CanvasFrame*, mFrame);
    return frame->CanvasArea() + aBuilder->ToReferenceFrame(mFrame);
  }

  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsIRenderingContext* aCtx, const nsRect& aDirtyRect)
  {
    CanvasFrame* frame = NS_STATIC_CAST(CanvasFrame*, mFrame);
    nsPoint offset = aBuilder->ToReferenceFrame(mFrame);
    nsRect bgClipRect = frame->CanvasArea() + offset;
    nsCSSRendering::PaintBackground(mFrame->GetPresContext(), *aCtx, mFrame,
                                    aDirtyRect,
                                    nsRect(offset, mFrame->GetSize()),
                                    *mFrame->GetStyleBorder(),
                                    *mFrame->GetStylePadding(),
                                    mFrame->HonorPrintBackgroundSettings(),
                                    &bgClipRect);
  }

  NS_DISPLAY_DECL_NAME("CanvasBackground")
};






class nsDisplayCanvasFocus : public nsDisplayItem {
public:
  nsDisplayCanvasFocus(CanvasFrame *aFrame)
    : nsDisplayItem(aFrame)
  {
  }

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder)
  {
    
    CanvasFrame* frame = NS_STATIC_CAST(CanvasFrame*, mFrame);
    return frame->CanvasArea() + aBuilder->ToReferenceFrame(mFrame);
  }

  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsIRenderingContext* aCtx, const nsRect& aDirtyRect)
  {
    CanvasFrame* frame = NS_STATIC_CAST(CanvasFrame*, mFrame);
    frame->PaintFocus(*aCtx, aBuilder->ToReferenceFrame(mFrame));
  }

  NS_DISPLAY_DECL_NAME("CanvasFocus")
};

NS_IMETHODIMP
CanvasFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists)
{
  nsresult rv;
  
  
  
  
  
  
  
  if (IsVisibleForPainting(aBuilder)) { 
    rv = aLists.BorderBackground()->AppendNewToTop(new (aBuilder)
           nsDisplayCanvasBackground(this));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsIFrame* kid = GetFirstChild(nsnull);
  if (kid) {
    
    rv = BuildDisplayListForChild(aBuilder, kid, aDirtyRect, aLists,
                                  DISPLAY_CHILD_FORCE_PSEUDO_STACKING_CONTEXT);
    NS_ENSURE_SUCCESS(rv, rv);
  }

#ifdef DEBUG_CANVAS_FOCUS
  nsCOMPtr<nsIContent> focusContent;
  aPresContext->EventStateManager()->
    GetFocusedContent(getter_AddRefs(focusContent));

  PRBool hasFocus = PR_FALSE;
  nsCOMPtr<nsISupports> container;
  aPresContext->GetContainer(getter_AddRefs(container));
  nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(container));
  if (docShell) {
    docShell->GetHasFocus(&hasFocus);
    printf("%p - CanvasFrame::Paint R:%d,%d,%d,%d  DR: %d,%d,%d,%d\n", this, 
            mRect.x, mRect.y, mRect.width, mRect.height,
            aDirtyRect.x, aDirtyRect.y, aDirtyRect.width, aDirtyRect.height);
  }
  printf("%p - Focus: %s   c: %p  DoPaint:%s\n", docShell.get(), hasFocus?"Y":"N", 
         focusContent.get(), mDoPaintFocus?"Y":"N");
#endif

  if (!mDoPaintFocus)
    return NS_OK;
  
  if (!GetStyleVisibility()->IsVisible())
    return NS_OK;
  
  return aLists.Outlines()->AppendNewToTop(new (aBuilder)
      nsDisplayCanvasFocus(this));
}

void
CanvasFrame::PaintFocus(nsIRenderingContext& aRenderingContext, nsPoint aPt)
{
  nsRect focusRect(aPt, GetSize());

  nsIScrollableFrame *scrollableFrame;
  CallQueryInterface(GetParent(), &scrollableFrame);

  if (scrollableFrame) {
    nsIScrollableView* scrollableView = scrollableFrame->GetScrollableView();
    nsRect vcr = scrollableView->View()->GetBounds();
    focusRect.width = vcr.width;
    focusRect.height = vcr.height;
    nscoord x,y;
    scrollableView->GetScrollPosition(x, y);
    focusRect.x += x;
    focusRect.y += y;
  }

  nsStyleOutline outlineStyle(GetPresContext());
  outlineStyle.SetOutlineStyle(NS_STYLE_BORDER_STYLE_DOTTED);
  outlineStyle.SetOutlineInitialColor();

 
 
  nsIFrame* root = mFrames.FirstChild();
  const nsStyleColor* color =
    root ? root->GetStyleContext()->GetStyleColor() :
           mStyleContext->GetStyleColor();
  if (!color) {
    NS_ERROR("current color cannot be found");
    return;
  }

  
  
  nscoord onePixel = nsPresContext::CSSPixelsToAppUnits(1);

  nsRect borderInside(focusRect.x + onePixel,
                      focusRect.y + onePixel,
                      focusRect.width - 2 * onePixel,
                      focusRect.height - 2 * onePixel);

  nsCSSRendering::DrawDashedSides(0, aRenderingContext, 
                                  focusRect, color,
                                  nsnull, &outlineStyle,
                                  PR_TRUE, focusRect,
                                  borderInside, 0, 
                                  nsnull);
}

 nscoord
CanvasFrame::GetMinWidth(nsIRenderingContext *aRenderingContext)
{
  nscoord result;
  DISPLAY_MIN_WIDTH(this, result);
  if (mFrames.IsEmpty())
    result = 0;
  else
    result = mFrames.FirstChild()->GetMinWidth(aRenderingContext);
  return result;
}

 nscoord
CanvasFrame::GetPrefWidth(nsIRenderingContext *aRenderingContext)
{
  nscoord result;
  DISPLAY_PREF_WIDTH(this, result);
  if (mFrames.IsEmpty())
    result = 0;
  else
    result = mFrames.FirstChild()->GetPrefWidth(aRenderingContext);
  return result;
}

NS_IMETHODIMP
CanvasFrame::Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("CanvasFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);
  NS_FRAME_TRACE_REFLOW_IN("CanvasFrame::Reflow");

  
  aStatus = NS_FRAME_COMPLETE;

  
  nsHTMLReflowMetrics kidDesiredSize;
  if (mFrames.IsEmpty()) {
    
    aDesiredSize.width = aDesiredSize.height = 0;
  } else {
    nsIFrame* kidFrame = mFrames.FirstChild();
    PRBool kidDirty = (kidFrame->GetStateBits() & NS_FRAME_IS_DIRTY) != 0;

    
    
    nsHTMLReflowState kidReflowState(aPresContext, aReflowState, kidFrame,
                                     nsSize(aReflowState.availableWidth,
                                            NS_UNCONSTRAINEDSIZE));

    
    ReflowChild(kidFrame, aPresContext, kidDesiredSize, kidReflowState,
                kidReflowState.mComputedMargin.left, kidReflowState.mComputedMargin.top,
                0, aStatus);

    
    FinishReflowChild(kidFrame, aPresContext, &kidReflowState, kidDesiredSize,
                      kidReflowState.mComputedMargin.left,
                      kidReflowState.mComputedMargin.top, 0);

    
    
    if (kidDirty) {
      
      
      
      
      
      
      
      
      Invalidate(GetRect(), PR_FALSE);
    }

    
    aDesiredSize.width = aReflowState.availableWidth;
    aDesiredSize.height = kidDesiredSize.height +
                          kidReflowState.mComputedMargin.TopBottom();

    aDesiredSize.mOverflowArea.UnionRect(
      nsRect(0, 0, aDesiredSize.width, aDesiredSize.height),
      kidDesiredSize.mOverflowArea +
        nsPoint(kidReflowState.mComputedMargin.left,
                kidReflowState.mComputedMargin.top));
    FinishAndStoreOverflow(&aDesiredSize);
  }

  NS_FRAME_TRACE_REFLOW_OUT("CanvasFrame::Reflow", aStatus);
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  return NS_OK;
}

PRIntn
CanvasFrame::GetSkipSides() const
{
  return 0;
}

nsIAtom*
CanvasFrame::GetType() const
{
  return nsGkAtoms::canvasFrame;
}

NS_IMETHODIMP 
CanvasFrame::GetContentForEvent(nsPresContext* aPresContext,
                                nsEvent* aEvent,
                                nsIContent** aContent)
{
  NS_ENSURE_ARG_POINTER(aContent);
  nsresult rv = nsFrame::GetContentForEvent(aPresContext,
                                            aEvent,
                                            aContent);
  if (NS_FAILED(rv) || !*aContent) {
    nsIFrame* kid = mFrames.FirstChild();
    if (kid) {
      rv = kid->GetContentForEvent(aPresContext,
                                   aEvent,
                                   aContent);
    }
  }

  return rv;
}

#ifdef DEBUG
NS_IMETHODIMP
CanvasFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("Canvas"), aResult);
}
#endif
