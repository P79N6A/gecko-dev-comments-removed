









#include "nsPlaceholderFrame.h"

#include "nsDisplayList.h"
#include "nsFrameManager.h"
#include "nsLayoutUtils.h"
#include "nsPresContext.h"
#include "nsRenderingContext.h"

nsIFrame*
NS_NewPlaceholderFrame(nsIPresShell* aPresShell, nsStyleContext* aContext,
                       nsFrameState aTypeBit)
{
  return new (aPresShell) nsPlaceholderFrame(aContext, aTypeBit);
}

NS_IMPL_FRAMEARENA_HELPERS(nsPlaceholderFrame)

 nsSize
nsPlaceholderFrame::GetMinSize(nsBoxLayoutState& aBoxLayoutState)
{
  nsSize size(0, 0);
  DISPLAY_MIN_SIZE(this, size);
  return size;
}

 nsSize
nsPlaceholderFrame::GetPrefSize(nsBoxLayoutState& aBoxLayoutState)
{
  nsSize size(0, 0);
  DISPLAY_PREF_SIZE(this, size);
  return size;
}

 nsSize
nsPlaceholderFrame::GetMaxSize(nsBoxLayoutState& aBoxLayoutState)
{
  nsSize size(NS_INTRINSICSIZE, NS_INTRINSICSIZE);
  DISPLAY_MAX_SIZE(this, size);
  return size;
}

 void
nsPlaceholderFrame::AddInlineMinWidth(nsRenderingContext* aRenderingContext,
                                      nsIFrame::InlineMinWidthData* aData)
{
  
  
  
  
  

  
  if (mOutOfFlowFrame->IsFloating()) {
    nscoord floatWidth =
      nsLayoutUtils::IntrinsicForContainer(aRenderingContext,
                                           mOutOfFlowFrame,
                                           nsLayoutUtils::MIN_WIDTH);
    aData->floats.AppendElement(
      InlineIntrinsicWidthData::FloatInfo(mOutOfFlowFrame, floatWidth));
  }
}

 void
nsPlaceholderFrame::AddInlinePrefWidth(nsRenderingContext* aRenderingContext,
                                       nsIFrame::InlinePrefWidthData* aData)
{
  
  
  
  
  

  
  if (mOutOfFlowFrame->IsFloating()) {
    nscoord floatWidth =
      nsLayoutUtils::IntrinsicForContainer(aRenderingContext,
                                           mOutOfFlowFrame,
                                           nsLayoutUtils::PREF_WIDTH);
    aData->floats.AppendElement(
      InlineIntrinsicWidthData::FloatInfo(mOutOfFlowFrame, floatWidth));
  }
}

NS_IMETHODIMP
nsPlaceholderFrame::Reflow(nsPresContext*           aPresContext,
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
nsPlaceholderFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  nsIFrame* oof = mOutOfFlowFrame;
  if (oof) {
    
    nsFrameManager* fm = PresContext()->GetPresShell()->FrameManager();
    fm->UnregisterPlaceholderFrame(this);
    mOutOfFlowFrame = nullptr;
    
    
    
    if ((GetStateBits() & PLACEHOLDER_FOR_POPUP) ||
        !nsLayoutUtils::IsProperAncestorFrame(aDestructRoot, oof)) {
      ChildListID listId = nsLayoutUtils::GetChildListNameFor(oof);
      fm->RemoveFrame(listId, oof);
    }
    
  }

  nsFrame::DestroyFrom(aDestructRoot);
}

nsIAtom*
nsPlaceholderFrame::GetType() const
{
  return nsGkAtoms::placeholderFrame;
}

 bool
nsPlaceholderFrame::CanContinueTextRun() const
{
  if (!mOutOfFlowFrame) {
    return false;
  }
  
  
  return mOutOfFlowFrame->CanContinueTextRun();
}

nsIFrame*
nsPlaceholderFrame::GetParentStyleContextFrame() const
{
  NS_PRECONDITION(GetParent(), "How can we not have a parent here?");

  
  
  
  return CorrectStyleParentFrame(GetParent(), nsGkAtoms::placeholderFrame);
}


#ifdef DEBUG
static void
PaintDebugPlaceholder(nsIFrame* aFrame, nsRenderingContext* aCtx,
                      const nsRect& aDirtyRect, nsPoint aPt)
{
  aCtx->SetColor(NS_RGB(0, 255, 255));
  nscoord x = nsPresContext::CSSPixelsToAppUnits(-5);
  aCtx->FillRect(aPt.x + x, aPt.y,
                 nsPresContext::CSSPixelsToAppUnits(13),
                 nsPresContext::CSSPixelsToAppUnits(3));
  nscoord y = nsPresContext::CSSPixelsToAppUnits(-10);
  aCtx->FillRect(aPt.x, aPt.y + y,
                 nsPresContext::CSSPixelsToAppUnits(3),
                 nsPresContext::CSSPixelsToAppUnits(10));
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
  if (GetShowFrameBorders()) {
    return aLists.Outlines()->AppendNewToTop(
      new (aBuilder) nsDisplayGeneric(aBuilder, this, PaintDebugPlaceholder,
                                      "DebugPlaceholder",
                                      nsDisplayItem::TYPE_DEBUG_PLACEHOLDER));
  }
#endif
  return NS_OK;
}
#endif 

#ifdef DEBUG
NS_IMETHODIMP
nsPlaceholderFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("Placeholder"), aResult);
}

NS_IMETHODIMP
nsPlaceholderFrame::List(FILE* out, int32_t aIndent, uint32_t aFlags) const
{
  IndentBy(out, aIndent);
  ListTag(out);
  if (HasView()) {
    fprintf(out, " [view=%p]", (void*)GetView());
  }
  fprintf(out, " {%d,%d,%d,%d}", mRect.x, mRect.y, mRect.width, mRect.height);
  if (0 != mState) {
    fprintf(out, " [state=%016llx]", (unsigned long long)mState);
  }
  nsIFrame* prevInFlow = GetPrevInFlow();
  nsIFrame* nextInFlow = GetNextInFlow();
  if (prevInFlow) {
    fprintf(out, " prev-in-flow=%p", static_cast<void*>(prevInFlow));
  }
  if (nextInFlow) {
    fprintf(out, " next-in-flow=%p", static_cast<void*>(nextInFlow));
  }
  if (mContent) {
    fprintf(out, " [content=%p]", static_cast<void*>(mContent));
  }
  if (mStyleContext) {
    fprintf(out, " [sc=%p]", static_cast<void*>(mStyleContext));
  }
  if (mOutOfFlowFrame) {
    fprintf(out, " outOfFlowFrame=");
    nsFrame::ListTag(out, mOutOfFlowFrame);
  }
  fputs("\n", out);
  return NS_OK;
}
#endif 
