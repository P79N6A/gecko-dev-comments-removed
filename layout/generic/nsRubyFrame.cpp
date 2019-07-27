






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

void
nsRubyFrame::CalculateColSizes(nsRenderingContext* aRenderingContext,
                               nsTArray<nscoord>& aColSizes)
{
  nsFrameList::Enumerator e(this->PrincipalChildList());
  uint32_t annotationNum = 0;
  int segmentNum = -1;

  nsTArray<int> segmentBaseCounts;

  for(; !e.AtEnd(); e.Next()) {
    nsIFrame* childFrame = e.get();
    if (childFrame->GetType() == nsGkAtoms::rubyBaseContainerFrame) {
      segmentNum++;
      segmentBaseCounts.AppendElement(0);
      nsFrameList::Enumerator bases(childFrame->PrincipalChildList());
      for(; !bases.AtEnd(); bases.Next()) {
        aColSizes.AppendElement(bases.get()->GetPrefISize(aRenderingContext));
        segmentBaseCounts.ElementAt(segmentNum)++;
      }
    } else if (childFrame->GetType() == nsGkAtoms::rubyTextContainerFrame) {
      if (segmentNum == -1) {
        
        segmentNum++;
        segmentBaseCounts.AppendElement(1);
        aColSizes.AppendElement(0);
      }
      nsFrameList::Enumerator annotations(childFrame->PrincipalChildList());
      uint32_t baseCount = segmentBaseCounts.ElementAt(segmentNum);
      for(; !annotations.AtEnd(); annotations.Next()) {
        nsIFrame* annotationFrame = annotations.get();
        if (annotationNum > baseCount) {
          aColSizes.AppendElement(annotationFrame->
            GetPrefISize(aRenderingContext));
          baseCount++;
          segmentBaseCounts.ElementAt(segmentNum) = baseCount;
          annotationNum++;
        } else if (annotationNum < baseCount - 1) {
          
          
          
          
          int baseSum = 0;
          for (uint32_t i = annotationNum; i < annotationNum + baseCount; i++) {
            baseSum += aColSizes.ElementAt(i);
            if (i > annotationNum) {
              aColSizes.ElementAt(i) = 0;
            }
          }
          aColSizes.ElementAt(annotationNum) =
            std::max(baseSum, annotationFrame->GetPrefISize(aRenderingContext));
          annotationNum = baseCount;
        } else {
          aColSizes.ElementAt(annotationNum) =
            std::max(aColSizes.ElementAt(annotationNum),
                     annotationFrame->GetPrefISize(aRenderingContext));
          annotationNum++;
        }
      }
    } else {
      NS_ASSERTION(false, "Unrecognized child type for ruby frame.");
    }
  }
}

 void
nsRubyFrame::AddInlineMinISize(nsRenderingContext *aRenderingContext,
                               nsIFrame::InlineMinISizeData *aData)
{
  
  
  nsTArray<int> colSizes;
  CalculateColSizes(aRenderingContext, colSizes);

  nscoord max = 0;
  for (uint32_t i = 0; i < colSizes.Length(); i++) {
    if (colSizes.ElementAt(i) > max) {
      max = colSizes.ElementAt(i);
    }
  }

  aData->currentLine += max;
}

 void
nsRubyFrame::AddInlinePrefISize(nsRenderingContext *aRenderingContext,
                                nsIFrame::InlinePrefISizeData *aData)
{
  nsTArray<int> colSizes;
  CalculateColSizes(aRenderingContext, colSizes);

  nscoord sum = 0;
  for (uint32_t i = 0; i < colSizes.Length(); i++) {
    sum += colSizes.ElementAt(i);
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
  
  nsRubyBaseContainerFrame* segmentRBC = nullptr;
  nscoord annotationBSize = 0;
  for (nsFrameList::Enumerator e(mFrames); !e.AtEnd(); e.Next()) {
    nsIFrame* childFrame = e.get();
    if (e.get()->GetType() == nsGkAtoms::rubyBaseContainerFrame) {
      if (segmentRBC) {
        annotationBSize = 0;
      }

      
      
      segmentRBC = do_QueryFrame(childFrame);
      nsFrameList::Enumerator segment(e);
      segment.Next();
      while (!segment.AtEnd() && (segment.get()->GetType() !=
             nsGkAtoms::rubyBaseContainerFrame)) {
        if (segment.get()->GetType() == nsGkAtoms::rubyTextContainerFrame) {
          segmentRBC->AppendTextContainer(segment.get());
        }
        segment.Next();
      }

      nsReflowStatus frameReflowStatus;
      nsHTMLReflowMetrics metrics(aReflowState, aDesiredSize.mFlags);
      nsHTMLReflowState childReflowState(aPresContext, aReflowState,
                                         childFrame, availSize);
      childReflowState.mLineLayout = aReflowState.mLineLayout;
      childFrame->Reflow(aPresContext, metrics, childReflowState,
                         frameReflowStatus);
      NS_ASSERTION(frameReflowStatus == NS_FRAME_COMPLETE,
                 "Ruby line breaking is not yet implemented");
      childFrame->SetSize(LogicalSize(lineWM, metrics.ISize(lineWM),
                                      metrics.BSize(lineWM)));
      FinishReflowChild(childFrame, aPresContext, metrics, &childReflowState, 0,
                        0, NS_FRAME_NO_MOVE_FRAME | NS_FRAME_NO_MOVE_VIEW);

    } else if (childFrame->GetType() == nsGkAtoms::rubyTextContainerFrame) {
      nsReflowStatus frameReflowStatus;
      nsHTMLReflowMetrics metrics(aReflowState, aDesiredSize.mFlags);
      nsHTMLReflowState childReflowState(aPresContext, aReflowState, childFrame,
                                         availSize);
      childReflowState.mLineLayout = aReflowState.mLineLayout;
      childFrame->Reflow(aPresContext, metrics, childReflowState,
                      frameReflowStatus);
      NS_ASSERTION(frameReflowStatus == NS_FRAME_COMPLETE,
                 "Ruby line breaking is not yet implemented");
      annotationBSize += metrics.BSize(lineWM);
      childFrame->SetSize(LogicalSize(lineWM, metrics.ISize(lineWM),
                                      metrics.BSize(lineWM)));
      
      
      
      nscoord baseContainerBCoord; 
      if (segmentRBC) {
        
        
        
        baseContainerBCoord = segmentRBC->
          GetLogicalPosition(this->GetParent()->GetLogicalSize().ISize(lineWM)).
          B(lineWM);
      } else {
        baseContainerBCoord = 0;
      }
      FinishReflowChild(childFrame, aPresContext, metrics, &childReflowState, 0,
                        baseContainerBCoord - metrics.BSize(lineWM), 0);
    } else {
      NS_NOTREACHED(
        "Unrecognized child type for ruby frame will not be reflowed."); 
    }
  }

  
  for (nsFrameList::Enumerator children(mFrames); !children.AtEnd();
       children.Next()) {
    nsRubyBaseContainerFrame* rbcFrame = do_QueryFrame(children.get());
    if (rbcFrame) {
      rbcFrame->ClearTextContainers();
    }
  }

  aDesiredSize.ISize(lineWM) = aReflowState.mLineLayout->EndSpan(this);
  nsLayoutUtils::SetBSizeFromFontMetrics(this, aDesiredSize, aReflowState,
                                         borderPadding, lineWM, frameWM);
}
