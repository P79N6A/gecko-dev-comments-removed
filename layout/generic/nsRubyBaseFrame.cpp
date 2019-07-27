







#include "nsRubyBaseFrame.h"
#include "nsLineLayout.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "WritingModes.h"

using namespace mozilla;






NS_QUERYFRAME_HEAD(nsRubyBaseFrame)
  NS_QUERYFRAME_ENTRY(nsRubyBaseFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsContainerFrame)

NS_IMPL_FRAMEARENA_HELPERS(nsRubyBaseFrame)

nsContainerFrame*
NS_NewRubyBaseFrame(nsIPresShell* aPresShell,
                    nsStyleContext* aContext)
{
  return new (aPresShell) nsRubyBaseFrame(aContext);
}







nsIAtom*
nsRubyBaseFrame::GetType() const
{
  return nsGkAtoms::rubyBaseFrame;
}

#ifdef DEBUG_FRAME_DUMP
nsresult
nsRubyBaseFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("RubyBase"), aResult);
}
#endif

nscoord
nsRubyBaseFrame::GetMinISize(nsRenderingContext *aRenderingContext)
{
  return nsLayoutUtils::MinISizeFromInline(this, aRenderingContext);
}

nscoord
nsRubyBaseFrame::GetPrefISize(nsRenderingContext *aRenderingContext)
{
  return nsLayoutUtils::PrefISizeFromInline(this, aRenderingContext);
}

 void
nsRubyBaseFrame::AddInlineMinISize(nsRenderingContext *aRenderingContext,
                                   nsIFrame::InlineMinISizeData *aData)
{
  for (nsFrameList::Enumerator e(PrincipalChildList()); !e.AtEnd(); e.Next()) {
    e.get()->AddInlineMinISize(aRenderingContext, aData);
  }
}

 void
nsRubyBaseFrame::AddInlinePrefISize(nsRenderingContext *aRenderingContext,
                                    nsIFrame::InlinePrefISizeData *aData)
{
  for (nsFrameList::Enumerator e(PrincipalChildList()); !e.AtEnd(); e.Next()) {
    e.get()->AddInlinePrefISize(aRenderingContext, aData);
  }
}

 bool 
nsRubyBaseFrame::IsFrameOfType(uint32_t aFlags) const 
{
  return nsContainerFrame::IsFrameOfType(aFlags & 
         ~(nsIFrame::eLineParticipant));
}

 nscoord
nsRubyBaseFrame::GetLogicalBaseline(WritingMode aWritingMode) const
{
  return mBaseline;
}

 void
nsRubyBaseFrame::Reflow(nsPresContext* aPresContext,
                        nsHTMLReflowMetrics& aDesiredSize,
                        const nsHTMLReflowState& aReflowState,
                        nsReflowStatus& aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsRubyBaseFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);
  
  if (!aReflowState.mLineLayout) {
    NS_ASSERTION(aReflowState.mLineLayout,
                 "No line layout provided to RubyBaseFrame reflow method.");
    aStatus = NS_FRAME_COMPLETE;
    return;
  }

  WritingMode lineWM = aReflowState.mLineLayout->GetWritingMode();
  WritingMode frameWM = aReflowState.GetWritingMode();
  aStatus = NS_FRAME_COMPLETE;
  LogicalSize availSize(lineWM, aReflowState.AvailableWidth(),
                        aReflowState.AvailableHeight());
  LogicalMargin borderPadding = aReflowState.ComputedLogicalBorderPadding();

  
  nscoord availableISize = aReflowState.AvailableISize();
  NS_ASSERTION(availableISize != NS_UNCONSTRAINEDSIZE,
               "should no longer use available widths");
  
  availableISize -= borderPadding.IStartEnd(frameWM);
  aReflowState.mLineLayout->BeginSpan(this, &aReflowState,
                                      borderPadding.IStart(frameWM),
                                      availableISize, &mBaseline);

  for (nsFrameList::Enumerator e(mFrames); !e.AtEnd(); e.Next()) {
    nsReflowStatus frameReflowStatus;
    nsHTMLReflowMetrics metrics(aReflowState, aDesiredSize.mFlags);

    bool pushedFrame;
    aReflowState.mLineLayout->ReflowFrame(e.get(), frameReflowStatus,
                                          &metrics, pushedFrame);
    NS_ASSERTION(!pushedFrame,
                 "Ruby line breaking is not yet implemented");

    e.get()->SetSize(LogicalSize(lineWM, metrics.ISize(lineWM),
                                 metrics.BSize(lineWM)));
  }

  aDesiredSize.ISize(lineWM) = aReflowState.mLineLayout->EndSpan(this);
  nsLayoutUtils::SetBSizeFromFontMetrics(this, aDesiredSize, aReflowState,
                                         borderPadding, lineWM, frameWM);
}
