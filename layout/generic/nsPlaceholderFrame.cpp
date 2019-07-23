










































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
    NS_ASSERTION(!shell->FrameManager()->GetPlaceholderFrameFor(mOutOfFlowFrame),
                 "Placeholder relationship should have been torn down; see "
                 "comments in nsPlaceholderFrame.h");
  }

  nsSplittableFrame::Destroy();
}

nsSplittableType
nsPlaceholderFrame::GetSplittableType() const
{
  NS_ASSERTION(mOutOfFlowFrame, "GetSplittableType called at the wrong time");
  return mOutOfFlowFrame->GetSplittableType();
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
  fprintf(out, " [parent=%p]", NS_STATIC_CAST(void*, mParent));
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
    fprintf(out, " prev-in-flow=%p", NS_STATIC_CAST(void*, prevInFlow));
  }
  if (nsnull != nextInFlow) {
    fprintf(out, " next-in-flow=%p", NS_STATIC_CAST(void*, nextInFlow));
  }
  if (mOutOfFlowFrame) {
    fprintf(out, " outOfFlowFrame=");
    nsFrame::ListTag(out, mOutOfFlowFrame);
  }
  fputs("\n", out);
  return NS_OK;
}
#endif
