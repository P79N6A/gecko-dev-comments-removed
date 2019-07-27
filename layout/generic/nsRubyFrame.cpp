






#include "nsRubyFrame.h"
#include "nsLineLayout.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "WritingModes.h"
#include "nsRubyBaseContainerFrame.h"
#include "nsRubyTextContainerFrame.h"

using namespace mozilla;






NS_QUERYFRAME_HEAD(nsRubyFrame)
  NS_QUERYFRAME_ENTRY(nsRubyFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsContainerFrame)

NS_IMPL_FRAMEARENA_HELPERS(nsRubyFrame)

nsContainerFrame*
NS_NewRubyFrame(nsIPresShell* aPresShell,
                nsStyleContext* aContext)
{
  return new (aPresShell) nsRubyFrame(aContext);
}






nsIAtom*
nsRubyFrame::GetType() const
{
  return nsGkAtoms::rubyFrame;
}

 bool
nsRubyFrame::IsFrameOfType(uint32_t aFlags) const
{
  return nsContainerFrame::IsFrameOfType(aFlags &
    ~(nsIFrame::eLineParticipant));
}

#ifdef DEBUG_FRAME_DUMP
nsresult
nsRubyFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("Ruby"), aResult);
}
#endif

class MOZ_STACK_CLASS TextContainerIterator
{
public:
  TextContainerIterator(nsRubyBaseContainerFrame* aBaseContainer);
  void Next();
  bool AtEnd() const { return !mFrame; }
  nsRubyTextContainerFrame* GetTextContainer() const
  {
    return static_cast<nsRubyTextContainerFrame*>(mFrame);
  }

private:
  nsIFrame* mFrame;
};

TextContainerIterator::TextContainerIterator(
    nsRubyBaseContainerFrame* aBaseContainer)
{
  mFrame = aBaseContainer;
  Next();
}

void
TextContainerIterator::Next()
{
  if (mFrame) {
    mFrame = mFrame->GetNextSibling();
    if (mFrame && mFrame->GetType() != nsGkAtoms::rubyTextContainerFrame) {
      mFrame = nullptr;
    }
  }
}





class MOZ_STACK_CLASS AutoSetTextContainers
{
public:
  AutoSetTextContainers(nsRubyBaseContainerFrame* aBaseContainer);
  ~AutoSetTextContainers();

private:
  nsRubyBaseContainerFrame* mBaseContainer;
};

AutoSetTextContainers::AutoSetTextContainers(
    nsRubyBaseContainerFrame* aBaseContainer)
  : mBaseContainer(aBaseContainer)
{
#ifdef DEBUG
  aBaseContainer->AssertTextContainersEmpty();
#endif
  for (TextContainerIterator iter(aBaseContainer);
       !iter.AtEnd(); iter.Next()) {
    aBaseContainer->AppendTextContainer(iter.GetTextContainer());
  }
}

AutoSetTextContainers::~AutoSetTextContainers()
{
  mBaseContainer->ClearTextContainers();
}




class MOZ_STACK_CLASS SegmentEnumerator
{
public:
  SegmentEnumerator(nsRubyFrame* aRubyFrame);

  void Next();
  bool AtEnd() const { return !mBaseContainer; }

  nsRubyBaseContainerFrame* GetBaseContainer() const
  {
    return mBaseContainer;
  }

private:
  nsRubyBaseContainerFrame* mBaseContainer;
};

SegmentEnumerator::SegmentEnumerator(nsRubyFrame* aRubyFrame)
{
  nsIFrame* frame = aRubyFrame->GetFirstPrincipalChild();
  MOZ_ASSERT(!frame ||
             frame->GetType() == nsGkAtoms::rubyBaseContainerFrame);
  mBaseContainer = static_cast<nsRubyBaseContainerFrame*>(frame);
}

void
SegmentEnumerator::Next()
{
  MOZ_ASSERT(mBaseContainer);
  nsIFrame* frame = mBaseContainer->GetNextSibling();
  while (frame && frame->GetType() != nsGkAtoms::rubyBaseContainerFrame) {
    frame = frame->GetNextSibling();
  }
  mBaseContainer = static_cast<nsRubyBaseContainerFrame*>(frame);
}

 void
nsRubyFrame::AddInlineMinISize(nsRenderingContext *aRenderingContext,
                               nsIFrame::InlineMinISizeData *aData)
{
  nscoord max = 0;
  for (SegmentEnumerator e(this); !e.AtEnd(); e.Next()) {
    AutoSetTextContainers holder(e.GetBaseContainer());
    max = std::max(max, e.GetBaseContainer()->GetMinISize(aRenderingContext));
  }
  aData->currentLine += max;
}

 void
nsRubyFrame::AddInlinePrefISize(nsRenderingContext *aRenderingContext,
                                nsIFrame::InlinePrefISizeData *aData)
{
  nscoord sum = 0;
  for (SegmentEnumerator e(this); !e.AtEnd(); e.Next()) {
    AutoSetTextContainers holder(e.GetBaseContainer());
    sum += e.GetBaseContainer()->GetPrefISize(aRenderingContext);
  }
  aData->currentLine += sum;
}

 nscoord
nsRubyFrame::GetLogicalBaseline(WritingMode aWritingMode) const
{
  return mBaseline;
}

 bool
nsRubyFrame::CanContinueTextRun() const
{
  return true;
}

 void
nsRubyFrame::Reflow(nsPresContext* aPresContext,
                    nsHTMLReflowMetrics& aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus& aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsRubyFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);
  
  if (!aReflowState.mLineLayout) {
    NS_ASSERTION(aReflowState.mLineLayout,
                 "No line layout provided to RubyFrame reflow method.");
    aStatus = NS_FRAME_COMPLETE;
    return;
  }

  
  WritingMode frameWM = aReflowState.GetWritingMode();
  WritingMode lineWM = aReflowState.mLineLayout->GetWritingMode();
  LogicalMargin borderPadding = aReflowState.ComputedLogicalBorderPadding();
  nscoord availableISize = aReflowState.AvailableISize();
  NS_ASSERTION(availableISize != NS_UNCONSTRAINEDSIZE,
               "should no longer use available widths");
  
  availableISize -= borderPadding.IStartEnd(frameWM);
  aReflowState.mLineLayout->BeginSpan(this, &aReflowState,
                                      borderPadding.IStart(frameWM),
                                      availableISize, &mBaseline);

  
  aStatus = NS_FRAME_COMPLETE;
  LogicalSize availSize(lineWM, aReflowState.AvailableISize(),
                        aReflowState.AvailableBSize());
  for (SegmentEnumerator e(this); !e.AtEnd(); e.Next()) {
    nsRubyBaseContainerFrame* baseContainer = e.GetBaseContainer();
    AutoSetTextContainers holder(baseContainer);
    nsReflowStatus baseReflowStatus;
    nsHTMLReflowMetrics baseMetrics(aReflowState, aDesiredSize.mFlags);
    nsHTMLReflowState baseReflowState(aPresContext, aReflowState,
                                      baseContainer, availSize);
    baseReflowState.mLineLayout = aReflowState.mLineLayout;
    baseContainer->Reflow(aPresContext, baseMetrics, baseReflowState,
                          baseReflowStatus);
    NS_ASSERTION(baseReflowStatus == NS_FRAME_COMPLETE,
                 "Ruby line breaking is not yet implemented");
    baseContainer->SetSize(LogicalSize(lineWM, baseMetrics.ISize(lineWM),
                                       baseMetrics.BSize(lineWM)));
    FinishReflowChild(baseContainer, aPresContext, baseMetrics,
                      &baseReflowState, 0, 0,
                      NS_FRAME_NO_MOVE_FRAME | NS_FRAME_NO_MOVE_VIEW);

    for (TextContainerIterator iter(baseContainer);
         !iter.AtEnd(); iter.Next()) {
      nsRubyTextContainerFrame* textContainer = iter.GetTextContainer();
      nsReflowStatus textReflowStatus;
      nsHTMLReflowMetrics textMetrics(aReflowState, aDesiredSize.mFlags);
      nsHTMLReflowState textReflowState(aPresContext, aReflowState,
                                        textContainer, availSize);
      textReflowState.mLineLayout = aReflowState.mLineLayout;
      textContainer->Reflow(aPresContext, textMetrics,
                            textReflowState, textReflowStatus);
      NS_ASSERTION(textReflowStatus == NS_FRAME_COMPLETE,
                   "Ruby line breaking is not yet implemented");
      textContainer->SetSize(LogicalSize(lineWM, textMetrics.ISize(lineWM),
                                         textMetrics.BSize(lineWM)));
      
      
      
      
      
      
      nscoord baseContainerBCoord = baseContainer->GetLogicalPosition(
          GetParent()->GetLogicalSize().ISize(lineWM)).B(lineWM);
      FinishReflowChild(textContainer, aPresContext, textMetrics,
                        &textReflowState, 0,
                        baseContainerBCoord - textMetrics.BSize(lineWM), 0);
    }
  }

  aDesiredSize.ISize(lineWM) = aReflowState.mLineLayout->EndSpan(this);
  nsLayoutUtils::SetBSizeFromFontMetrics(this, aDesiredSize, aReflowState,
                                         borderPadding, lineWM, frameWM);
}
