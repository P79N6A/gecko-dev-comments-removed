






































#include "nsCanvasFrame.h"
#include "nsIServiceManager.h"
#include "nsHTMLParts.h"
#include "nsHTMLContainerFrame.h"
#include "nsCSSRendering.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsIRenderingContext.h"
#include "nsGUIEvent.h"
#include "nsStyleConsts.h"
#include "nsGkAtoms.h"
#include "nsIEventStateManager.h"
#include "nsIDeviceContext.h"
#include "nsIPresShell.h"
#include "nsIScrollPositionListener.h"
#include "nsDisplayList.h"
#include "nsAbsoluteContainingBlock.h"
#include "nsCSSFrameConstructor.h"
#include "nsFrameManager.h"


#include "nsIDOMWindowInternal.h"
#include "nsIScrollableFrame.h"
#include "nsIDocShell.h"

#ifdef DEBUG_rods

#endif

#define CANVAS_ABS_POS_CHILD_LIST NS_CONTAINER_LIST_COUNT_INCL_OC


nsIFrame*
NS_NewCanvasFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsCanvasFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsCanvasFrame)

NS_QUERYFRAME_HEAD(nsCanvasFrame)
  NS_QUERYFRAME_ENTRY(nsCanvasFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsHTMLContainerFrame)

void
nsCanvasFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  mAbsoluteContainer.DestroyFrames(this, aDestructRoot);

  nsIScrollableFrame* sf =
    PresContext()->GetPresShell()->GetRootScrollFrameAsScrollable();
  if (sf) {
    sf->RemoveScrollPositionListener(this);
  }

  nsHTMLContainerFrame::DestroyFrom(aDestructRoot);
}

void
nsCanvasFrame::ScrollPositionWillChange(nscoord aX, nscoord aY)
{
  if (mDoPaintFocus) {
    mDoPaintFocus = PR_FALSE;
    PresContext()->FrameManager()->GetRootFrame()->InvalidateOverflowRect();
  }
}

NS_IMETHODIMP
nsCanvasFrame::SetHasFocus(PRBool aHasFocus)
{
  if (mDoPaintFocus != aHasFocus) {
    mDoPaintFocus = aHasFocus;
    PresContext()->FrameManager()->GetRootFrame()->InvalidateOverflowRect();

    if (!mAddedScrollPositionListener) {
      nsIScrollableFrame* sf =
        PresContext()->GetPresShell()->GetRootScrollFrameAsScrollable();
      if (sf) {
        sf->AddScrollPositionListener(this);
        mAddedScrollPositionListener = PR_TRUE;
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasFrame::SetInitialChildList(nsIAtom*        aListName,
                                   nsFrameList&    aChildList)
{
  if (nsGkAtoms::absoluteList == aListName)
    return mAbsoluteContainer.SetInitialChildList(this, aListName, aChildList);

  NS_ASSERTION(aListName || aChildList.IsEmpty() || aChildList.OnlyChild(),
               "Primary child list can have at most one frame in it");
  return nsHTMLContainerFrame::SetInitialChildList(aListName, aChildList);
}

NS_IMETHODIMP
nsCanvasFrame::AppendFrames(nsIAtom*        aListName,
                            nsFrameList&    aFrameList)
{
  if (nsGkAtoms::absoluteList == aListName)
    return mAbsoluteContainer.AppendFrames(this, aListName, aFrameList);

  NS_ASSERTION(!aListName, "unexpected child list name");
  NS_PRECONDITION(mFrames.IsEmpty(), "already have a child frame");
  if (aListName) {
    
    return NS_ERROR_INVALID_ARG;
  }

  if (!mFrames.IsEmpty()) {
    
    return NS_ERROR_INVALID_ARG;
  }

  
  NS_ASSERTION(aFrameList.FirstChild() == aFrameList.LastChild(),
               "Only one principal child frame allowed");
#ifdef NS_DEBUG
  nsFrame::VerifyDirtyBitSet(aFrameList);
#endif
  mFrames.AppendFrames(nsnull, aFrameList);

  PresContext()->PresShell()->
    FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                     NS_FRAME_HAS_DIRTY_CHILDREN);

  return NS_OK;
}

NS_IMETHODIMP
nsCanvasFrame::InsertFrames(nsIAtom*        aListName,
                            nsIFrame*       aPrevFrame,
                            nsFrameList&    aFrameList)
{
  if (nsGkAtoms::absoluteList == aListName)
    return mAbsoluteContainer.InsertFrames(this, aListName, aPrevFrame, aFrameList);

  
  
  NS_PRECONDITION(!aPrevFrame, "unexpected previous sibling frame");
  if (aPrevFrame)
    return NS_ERROR_UNEXPECTED;

  return AppendFrames(aListName, aFrameList);
}

NS_IMETHODIMP
nsCanvasFrame::RemoveFrame(nsIAtom*        aListName,
                           nsIFrame*       aOldFrame)
{
  if (nsGkAtoms::absoluteList == aListName) {
    mAbsoluteContainer.RemoveFrame(this, aListName, aOldFrame);
    return NS_OK;
  }

  NS_ASSERTION(!aListName, "unexpected child list name");
  if (aListName) {
    
    return NS_ERROR_INVALID_ARG;
  }

  if (aOldFrame != mFrames.FirstChild())
    return NS_ERROR_FAILURE;

  
  
  
  
  Invalidate(aOldFrame->GetOverflowRect() + aOldFrame->GetPosition());

  
  mFrames.DestroyFrame(aOldFrame);

  PresContext()->PresShell()->
    FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                     NS_FRAME_HAS_DIRTY_CHILDREN);
  return NS_OK;
}

nsIAtom*
nsCanvasFrame::GetAdditionalChildListName(PRInt32 aIndex) const
{
  if (CANVAS_ABS_POS_CHILD_LIST == aIndex)
    return nsGkAtoms::absoluteList;

  return nsHTMLContainerFrame::GetAdditionalChildListName(aIndex);
}

nsFrameList
nsCanvasFrame::GetChildList(nsIAtom* aListName) const
{
  if (nsGkAtoms::absoluteList == aListName)
    return mAbsoluteContainer.GetChildList();

  return nsHTMLContainerFrame::GetChildList(aListName);
}

nsRect nsCanvasFrame::CanvasArea() const
{
  nsRect result(GetOverflowRect());

  nsIScrollableFrame *scrollableFrame = do_QueryFrame(GetParent());
  if (scrollableFrame) {
    nsRect portRect = scrollableFrame->GetScrollPortRect();
    result.UnionRect(result, nsRect(nsPoint(0, 0), portRect.Size()));
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
    nsCanvasFrame* frame = static_cast<nsCanvasFrame*>(mFrame);
    return frame->CanvasArea() + aBuilder->ToReferenceFrame(mFrame);
  }

  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsIRenderingContext* aCtx)
  {
    nsCanvasFrame* frame = static_cast<nsCanvasFrame*>(mFrame);
    nsPoint offset = aBuilder->ToReferenceFrame(mFrame);
    nsRect bgClipRect = frame->CanvasArea() + offset;
    nsCSSRendering::PaintBackground(mFrame->PresContext(), *aCtx, mFrame,
                                    mVisibleRect,
                                    nsRect(offset, mFrame->GetSize()),
                                    aBuilder->GetBackgroundPaintFlags(),
                                    &bgClipRect);
  }

  NS_DISPLAY_DECL_NAME("CanvasBackground", TYPE_CANVAS_BACKGROUND)
};






class nsDisplayCanvasFocus : public nsDisplayItem {
public:
  nsDisplayCanvasFocus(nsCanvasFrame *aFrame)
    : nsDisplayItem(aFrame)
  {
    MOZ_COUNT_CTOR(nsDisplayCanvasFocus);
  }
  virtual ~nsDisplayCanvasFocus() {
    MOZ_COUNT_DTOR(nsDisplayCanvasFocus);
  }

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder)
  {
    
    nsCanvasFrame* frame = static_cast<nsCanvasFrame*>(mFrame);
    return frame->CanvasArea() + aBuilder->ToReferenceFrame(mFrame);
  }

  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsIRenderingContext* aCtx)
  {
    nsCanvasFrame* frame = static_cast<nsCanvasFrame*>(mFrame);
    frame->PaintFocus(*aCtx, aBuilder->ToReferenceFrame(mFrame));
  }

  NS_DISPLAY_DECL_NAME("CanvasFocus", TYPE_CANVAS_FOCUS)
};

NS_IMETHODIMP
nsCanvasFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists)
{
  nsresult rv;

  if (GetPrevInFlow()) {
    DisplayOverflowContainers(aBuilder, aDirtyRect, aLists);
  }

  aBuilder->MarkFramesForDisplayList(this, mAbsoluteContainer.GetChildList(),
                                     aDirtyRect);
  
  
  
  
  
  
  
  
  if (IsVisibleForPainting(aBuilder)) { 
    rv = aLists.BorderBackground()->AppendNewToTop(new (aBuilder)
           nsDisplayCanvasBackground(this));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsIFrame* kid;
  for (kid = GetFirstChild(nsnull); kid; kid = kid->GetNextSibling()) {
    
    rv = BuildDisplayListForChild(aBuilder, kid, aDirtyRect, aLists);
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
    printf("%p - nsCanvasFrame::Paint R:%d,%d,%d,%d  DR: %d,%d,%d,%d\n", this, 
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
nsCanvasFrame::PaintFocus(nsIRenderingContext& aRenderingContext, nsPoint aPt)
{
  nsRect focusRect(aPt, GetSize());

  nsIScrollableFrame *scrollableFrame = do_QueryFrame(GetParent());
  if (scrollableFrame) {
    nsRect portRect = scrollableFrame->GetScrollPortRect();
    focusRect.width = portRect.width;
    focusRect.height = portRect.height;
    focusRect.MoveBy(scrollableFrame->GetScrollPosition());
  }

 
 
  nsIFrame* root = mFrames.FirstChild();
  const nsStyleColor* color =
    root ? root->GetStyleContext()->GetStyleColor() :
           mStyleContext->GetStyleColor();
  if (!color) {
    NS_ERROR("current color cannot be found");
    return;
  }

  nsCSSRendering::PaintFocus(PresContext(), aRenderingContext,
                             focusRect, color->mColor);
}

 nscoord
nsCanvasFrame::GetMinWidth(nsIRenderingContext *aRenderingContext)
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
nsCanvasFrame::GetPrefWidth(nsIRenderingContext *aRenderingContext)
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
nsCanvasFrame::Reflow(nsPresContext*           aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsCanvasFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);
  NS_FRAME_TRACE_REFLOW_IN("nsCanvasFrame::Reflow");

  
  aStatus = NS_FRAME_COMPLETE;

  nsCanvasFrame* prevCanvasFrame = static_cast<nsCanvasFrame*>
                                               (GetPrevInFlow());
  if (prevCanvasFrame) {
    nsAutoPtr<nsFrameList> overflow(prevCanvasFrame->StealOverflowFrames());
    if (overflow) {
      NS_ASSERTION(overflow->OnlyChild(),
                   "must have doc root as canvas frame's only child");
      nsHTMLContainerFrame::ReparentFrameViewList(aPresContext, *overflow,
                                                  prevCanvasFrame, this);
      
      
      
      mFrames.InsertFrames(this, nsnull, *overflow);
    }
  }

  
  
  
  
  
  
  
  nsHTMLReflowMetrics kidDesiredSize;
  if (mFrames.IsEmpty()) {
    
    aDesiredSize.width = aDesiredSize.height = 0;
  } else {
    nsIFrame* kidFrame = mFrames.FirstChild();
    nsRect oldKidRect = kidFrame->GetRect();
    PRBool kidDirty = (kidFrame->GetStateBits() & NS_FRAME_IS_DIRTY) != 0;

    nsHTMLReflowState kidReflowState(aPresContext, aReflowState, kidFrame,
                                     nsSize(aReflowState.availableWidth,
                                            aReflowState.availableHeight));

    if (aReflowState.mFlags.mVResize &&
        (kidFrame->GetStateBits() & NS_FRAME_CONTAINS_RELATIVE_HEIGHT)) {
      
      
      kidReflowState.mFlags.mVResize = PR_TRUE;
    }

    nsPoint kidPt(kidReflowState.mComputedMargin.left,
                  kidReflowState.mComputedMargin.top);
    
    const nsStyleDisplay* styleDisp = kidFrame->GetStyleDisplay();
    if (NS_STYLE_POSITION_RELATIVE == styleDisp->mPosition) {
      kidPt += nsPoint(kidReflowState.mComputedOffsets.left,
                       kidReflowState.mComputedOffsets.top);
    }

    
    ReflowChild(kidFrame, aPresContext, kidDesiredSize, kidReflowState,
                kidPt.x, kidPt.y, 0, aStatus);

    
    FinishReflowChild(kidFrame, aPresContext, &kidReflowState, kidDesiredSize,
                      kidPt.x, kidPt.y, 0);

    if (!NS_FRAME_IS_FULLY_COMPLETE(aStatus)) {
      nsIFrame* nextFrame = kidFrame->GetNextInFlow();
      NS_ASSERTION(nextFrame || aStatus & NS_FRAME_REFLOW_NEXTINFLOW,
        "If it's incomplete and has no nif yet, it must flag a nif reflow.");
      if (!nextFrame) {
        nsresult rv = aPresContext->PresShell()->FrameConstructor()->
          CreateContinuingFrame(aPresContext, kidFrame, this, &nextFrame);
        NS_ENSURE_SUCCESS(rv, rv);
        SetOverflowFrames(aPresContext, nsFrameList(nextFrame, nextFrame));
        
        
        
        
      }
      if (NS_FRAME_OVERFLOW_IS_INCOMPLETE(aStatus)) {
        nextFrame->AddStateBits(NS_FRAME_IS_OVERFLOW_CONTAINER);
      }
    }

    
    
    if (kidDirty) {
      
      
      
      
      
      
      
      
      nsIFrame* viewport = PresContext()->GetPresShell()->GetRootFrame();
      viewport->Invalidate(nsRect(nsPoint(0, 0), viewport->GetSize()));
    } else {
      nsRect newKidRect = kidFrame->GetRect();
      if (newKidRect.TopLeft() == oldKidRect.TopLeft()) {
        InvalidateRectDifference(oldKidRect, kidFrame->GetRect());
      } else {
        Invalidate(oldKidRect);
        Invalidate(newKidRect);
      }
    }
    
    
    
    
    aDesiredSize.width = aReflowState.ComputedWidth();
    if (aReflowState.ComputedHeight() == NS_UNCONSTRAINEDSIZE) {
      aDesiredSize.height = kidFrame->GetRect().height +
        kidReflowState.mComputedMargin.TopBottom();
    } else {
      aDesiredSize.height = aReflowState.ComputedHeight();
    }

    aDesiredSize.mOverflowArea.UnionRect(
      nsRect(0, 0, aDesiredSize.width, aDesiredSize.height),
      kidDesiredSize.mOverflowArea + kidPt);

    if (mAbsoluteContainer.HasAbsoluteFrames()) {
      PRBool widthChanged = aDesiredSize.width != mRect.width;
      PRBool heightChanged = aDesiredSize.height != mRect.height;
      nsRect absPosBounds;
      mAbsoluteContainer.Reflow(this, aPresContext, aReflowState, aStatus,
                                aDesiredSize.width, aDesiredSize.height,
                                PR_TRUE, widthChanged, heightChanged,
                                &absPosBounds);
      aDesiredSize.mOverflowArea.UnionRect(aDesiredSize.mOverflowArea, absPosBounds);
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (nsSize(aDesiredSize.width, aDesiredSize.height) != GetSize()) {
      nsIFrame* rootElementFrame =
        aPresContext->PresShell()->FrameConstructor()->GetRootElementStyleFrame();
      nsStyleContext* bgSC =
        nsCSSRendering::FindCanvasBackground(this, rootElementFrame);
      const nsStyleBackground* bg = bgSC->GetStyleBackground();
      if (!bg->IsTransparent()) {
        NS_FOR_VISIBLE_BACKGROUND_LAYERS_BACK_TO_FRONT(i, bg) {
          const nsStyleBackground::Layer& layer = bg->mLayers[i];
          if (layer.mAttachment == NS_STYLE_BG_ATTACHMENT_FIXED &&
              layer.RenderingMightDependOnFrameSize()) {
            Invalidate(nsRect(nsPoint(0, 0), GetSize()));
            break;
          }
        }
      }
    }
  }

  if (prevCanvasFrame) {
    ReflowOverflowContainerChildren(aPresContext, aReflowState,
                                    aDesiredSize.mOverflowArea, 0,
                                    aStatus);
  }

  FinishAndStoreOverflow(&aDesiredSize);

  NS_FRAME_TRACE_REFLOW_OUT("nsCanvasFrame::Reflow", aStatus);
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  return NS_OK;
}

PRIntn
nsCanvasFrame::GetSkipSides() const
{
  return 0;
}

nsIAtom*
nsCanvasFrame::GetType() const
{
  return nsGkAtoms::canvasFrame;
}

NS_IMETHODIMP 
nsCanvasFrame::GetContentForEvent(nsPresContext* aPresContext,
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
nsCanvasFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("Canvas"), aResult);
}
#endif
