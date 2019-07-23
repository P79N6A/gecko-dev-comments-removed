










































#include "nsPlaceholderFrame.h"
#include "nsLineLayout.h"
#include "nsIContent.h"
#include "nsPresContext.h"
#include "nsIRenderingContext.h"
#include "nsGkAtoms.h"
#include "nsFrameManager.h"
#include "nsDisplayList.h"

nsIFrame*
NS_NewPlaceholderFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsPlaceholderFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsPlaceholderFrame)

nsPlaceholderFrame::~nsPlaceholderFrame()
{
}

 nscoord
nsPlaceholderFrame::GetMinWidth(nsIRenderingContext *aRenderingContext)
{
  nscoord result = 0;
  DISPLAY_MIN_WIDTH(this, result);
  return result;
}

 nscoord
nsPlaceholderFrame::GetPrefWidth(nsIRenderingContext *aRenderingContext)
{
  nscoord result = 0;
  DISPLAY_PREF_WIDTH(this, result);
  return result;
}

 void
nsPlaceholderFrame::AddInlineMinWidth(nsIRenderingContext *aRenderingContext,
                                      nsIFrame::InlineMinWidthData *aData)
{
  
  
  
  
  

  
  if (mOutOfFlowFrame->GetStyleDisplay()->mFloats != NS_STYLE_FLOAT_NONE)
    aData->floats.AppendElement(mOutOfFlowFrame);
}

 void
nsPlaceholderFrame::AddInlinePrefWidth(nsIRenderingContext *aRenderingContext,
                                       nsIFrame::InlinePrefWidthData *aData)
{
  
  
  
  
  

  
  if (mOutOfFlowFrame->GetStyleDisplay()->mFloats != NS_STYLE_FLOAT_NONE)
    aData->floats.AppendElement(mOutOfFlowFrame);
}

NS_IMETHODIMP
nsPlaceholderFrame::Reflow(nsPresContext*          aPresContext,
                           nsHTMLReflowMetrics&     aDesiredSize,
                           const nsHTMLReflowState& aReflowState,
                           nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsPlaceholderFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);
  aDesiredSize.width = 0;
  aDesiredSize.height = 0;

  aStatus = NS_FRAME_COMPLETE;
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  return NS_OK;
}

void
nsPlaceholderFrame::Destroy()
{
  nsIPresShell* shell = PresContext()->GetPresShell();
  if (shell && mOutOfFlowFrame) {
    if (shell->FrameManager()->GetPlaceholderFrameFor(mOutOfFlowFrame)) {
      NS_ERROR("Placeholder relationship should have been torn down; see "
               "comments in nsPlaceholderFrame.h.  Unregistering ourselves, "
               "but this might cause our out-of-flow to be unable to destroy "
               "itself properly.  Not that it could anyway, with us dead.");
      shell->FrameManager()->UnregisterPlaceholderFrame(this);
    }
  }

  nsFrame::Destroy();
}

nsIAtom*
nsPlaceholderFrame::GetType() const
{
  return nsGkAtoms::placeholderFrame; 
}

 PRBool
nsPlaceholderFrame::CanContinueTextRun() const
{
  if (!mOutOfFlowFrame) {
    return PR_FALSE;
  }
  
  
  return mOutOfFlowFrame->CanContinueTextRun();
}

NS_IMETHODIMP
nsPlaceholderFrame::GetParentStyleContextFrame(nsPresContext* aPresContext,
                                               nsIFrame**      aProviderFrame,
                                               PRBool*         aIsChild)
{
  NS_PRECONDITION(GetParent(), "How can we not have a parent here?");
  *aIsChild = PR_FALSE;

  
  
  
  *aProviderFrame =
    CorrectStyleParentFrame(GetParent(), nsGkAtoms::placeholderFrame);
  return NS_OK;
}


#ifdef DEBUG
static void
PaintDebugPlaceholder(nsIFrame* aFrame, nsIRenderingContext* aCtx,
                      const nsRect& aDirtyRect, nsPoint aPt)
{
  aCtx->SetColor(NS_RGB(0, 255, 255));
  nscoord x = nsPresContext::CSSPixelsToAppUnits(-5);
  aCtx->FillRect(aPt.x + x, aPt.y,
                 nsPresContext::CSSPixelsToAppUnits(13), nsPresContext::CSSPixelsToAppUnits(3));
  nscoord y = nsPresContext::CSSPixelsToAppUnits(-10);
  aCtx->FillRect(aPt.x, aPt.y + y,
                 nsPresContext::CSSPixelsToAppUnits(3), nsPresContext::CSSPixelsToAppUnits(10));
}
#endif 

#if defined(DEBUG) || (defined(MOZ_REFLOW_PERF_DSP) && defined(MOZ_REFLOW_PERF))

NS_IMETHODIMP
nsPlaceholderFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                     const nsRect&           aDirtyRect,
                                     const nsDisplayListSet& aLists)
{
  DO_GLOBAL_REFLOW_COUNT_DSP("nsPlaceholderFrame");
  
#ifdef DEBUG
  if (!GetShowFrameBorders())
    return NS_OK;
  
  return aLists.Outlines()->AppendNewToTop(new (aBuilder)
      nsDisplayGeneric(this, PaintDebugPlaceholder, "DebugPlaceholder"));
#else 
  return NS_OK;
#endif 
}
#endif 

#ifdef DEBUG
NS_IMETHODIMP
nsPlaceholderFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("Placeholder"), aResult);
}

NS_IMETHODIMP
nsPlaceholderFrame::List(FILE* out, PRInt32 aIndent) const
{
  IndentBy(out, aIndent);
  ListTag(out);
#ifdef DEBUG_waterson
  fprintf(out, " [parent=%p]", static_cast<void*>(mParent));
#endif
  if (HasView()) {
    fprintf(out, " [view=%p]", (void*)GetView());
  }
  fprintf(out, " {%d,%d,%d,%d}", mRect.x, mRect.y, mRect.width, mRect.height);
  if (0 != mState) {
    fprintf(out, " [state=%08x]", mState);
  }
  nsIFrame* prevInFlow = GetPrevInFlow();
  nsIFrame* nextInFlow = GetNextInFlow();
  if (nsnull != prevInFlow) {
    fprintf(out, " prev-in-flow=%p", static_cast<void*>(prevInFlow));
  }
  if (nsnull != nextInFlow) {
    fprintf(out, " next-in-flow=%p", static_cast<void*>(nextInFlow));
  }
  if (nsnull != mContent) {
    fprintf(out, " [content=%p]", static_cast<void*>(mContent));
  }
  if (mOutOfFlowFrame) {
    fprintf(out, " outOfFlowFrame=");
    nsFrame::ListTag(out, mOutOfFlowFrame);
  }
  fputs("\n", out);
  return NS_OK;
}
#endif
