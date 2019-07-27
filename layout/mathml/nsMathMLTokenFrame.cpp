




#include "nsMathMLTokenFrame.h"
#include "nsPresContext.h"
#include "nsContentUtils.h"
#include "nsTextFrame.h"
#include "RestyleManager.h"
#include <algorithm>

using namespace mozilla;

nsIFrame*
NS_NewMathMLTokenFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLTokenFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsMathMLTokenFrame)

nsMathMLTokenFrame::~nsMathMLTokenFrame()
{
}

NS_IMETHODIMP
nsMathMLTokenFrame::InheritAutomaticData(nsIFrame* aParent)
{
  
  nsMathMLContainerFrame::InheritAutomaticData(aParent);

  return NS_OK;
}

eMathMLFrameType
nsMathMLTokenFrame::GetMathMLFrameType()
{
  
  if (!mContent->IsMathMLElement(nsGkAtoms::mi_)) {
    return eMathMLFrameType_Ordinary;
  }

  uint8_t mathVariant = StyleFont()->mMathVariant;
  if ((mathVariant == NS_MATHML_MATHVARIANT_NONE &&
       (StyleFont()->mFont.style == NS_STYLE_FONT_STYLE_ITALIC ||
        HasAnyStateBits(NS_FRAME_IS_IN_SINGLE_CHAR_MI))) ||
      mathVariant == NS_MATHML_MATHVARIANT_ITALIC ||
      mathVariant == NS_MATHML_MATHVARIANT_BOLD_ITALIC ||
      mathVariant == NS_MATHML_MATHVARIANT_SANS_SERIF_ITALIC ||
      mathVariant == NS_MATHML_MATHVARIANT_SANS_SERIF_BOLD_ITALIC) {
    return eMathMLFrameType_ItalicIdentifier;
  }
  return eMathMLFrameType_UprightIdentifier;
}

void
nsMathMLTokenFrame::MarkTextFramesAsTokenMathML()
{
  nsIFrame* child = nullptr;
  uint32_t childCount = 0;

  
  
  
  
  for (nsIFrame* childFrame = GetFirstPrincipalChild(); childFrame;
       childFrame = childFrame->GetNextSibling()) {
    for (nsIFrame* childFrame2 = childFrame->GetFirstPrincipalChild();
         childFrame2; childFrame2 = childFrame2->GetNextSibling()) {
      if (childFrame2->GetType() == nsGkAtoms::textFrame) {
        childFrame2->AddStateBits(TEXT_IS_IN_TOKEN_MATHML);
        child = childFrame2;
        childCount++;
      }
    }
  }
  if (mContent->IsMathMLElement(nsGkAtoms::mi_) && childCount == 1) {
    nsAutoString data;
    if (!nsContentUtils::GetNodeTextContent(mContent, false, data)) {
      NS_RUNTIMEABORT("OOM");
    }

    data.CompressWhitespace();
    int32_t length = data.Length();

    bool isSingleCharacter = length == 1 ||
      (length == 2 && NS_IS_HIGH_SURROGATE(data[0]));

    if (isSingleCharacter) {
      child->AddStateBits(NS_FRAME_IS_IN_SINGLE_CHAR_MI);
      AddStateBits(NS_FRAME_IS_IN_SINGLE_CHAR_MI);
    }
  }
}

void
nsMathMLTokenFrame::SetInitialChildList(ChildListID     aListID,
                                        nsFrameList&    aChildList)
{
  
  nsMathMLContainerFrame::SetInitialChildList(aListID, aChildList);
  MarkTextFramesAsTokenMathML();
}

void
nsMathMLTokenFrame::AppendFrames(ChildListID aListID,
                                 nsFrameList& aChildList)
{
  nsMathMLContainerFrame::AppendFrames(aListID, aChildList);
  MarkTextFramesAsTokenMathML();
}

void
nsMathMLTokenFrame::InsertFrames(ChildListID aListID,
                                 nsIFrame* aPrevFrame,
                                 nsFrameList& aChildList)
{
  nsMathMLContainerFrame::InsertFrames(aListID, aPrevFrame, aChildList);
  MarkTextFramesAsTokenMathML();
}

void
nsMathMLTokenFrame::Reflow(nsPresContext*          aPresContext,
                           nsHTMLReflowMetrics&     aDesiredSize,
                           const nsHTMLReflowState& aReflowState,
                           nsReflowStatus&          aStatus)
{
  
  aDesiredSize.ClearSize();
  aDesiredSize.SetBlockStartAscent(0);
  aDesiredSize.mBoundingMetrics = nsBoundingMetrics();

  nsIFrame* childFrame = GetFirstPrincipalChild();
  while (childFrame) {
    
    nsHTMLReflowMetrics childDesiredSize(aReflowState.GetWritingMode(),
                                         aDesiredSize.mFlags
                                         | NS_REFLOW_CALC_BOUNDING_METRICS);
    WritingMode wm = childFrame->GetWritingMode();
    LogicalSize availSize = aReflowState.ComputedSize(wm);
    availSize.BSize(wm) = NS_UNCONSTRAINEDSIZE;
    nsHTMLReflowState childReflowState(aPresContext, aReflowState,
                                       childFrame, availSize);
    ReflowChild(childFrame, aPresContext, childDesiredSize,
                childReflowState, aStatus);
    
    SaveReflowAndBoundingMetricsFor(childFrame, childDesiredSize,
                                    childDesiredSize.mBoundingMetrics);

    childFrame = childFrame->GetNextSibling();
  }

  
  FinalizeReflow(*aReflowState.rendContext, aDesiredSize);

  aStatus = NS_FRAME_COMPLETE;
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
}




 nsresult
nsMathMLTokenFrame::Place(nsRenderingContext& aRenderingContext,
                          bool                 aPlaceOrigin,
                          nsHTMLReflowMetrics& aDesiredSize)
{
  mBoundingMetrics = nsBoundingMetrics();
  for (nsIFrame* childFrame = GetFirstPrincipalChild(); childFrame;
       childFrame = childFrame->GetNextSibling()) {
    nsHTMLReflowMetrics childSize(aDesiredSize.GetWritingMode());
    GetReflowAndBoundingMetricsFor(childFrame, childSize,
                                   childSize.mBoundingMetrics, nullptr);
    
    mBoundingMetrics += childSize.mBoundingMetrics;
  }

  nsRefPtr<nsFontMetrics> fm;
  nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fm),
                                        nsLayoutUtils::
                                        FontSizeInflationFor(this));
  nscoord ascent = fm->MaxAscent();
  nscoord descent = fm->MaxDescent();

  aDesiredSize.mBoundingMetrics = mBoundingMetrics;
  aDesiredSize.Width() = mBoundingMetrics.width;
  aDesiredSize.SetBlockStartAscent(std::max(mBoundingMetrics.ascent, ascent));
  aDesiredSize.Height() = aDesiredSize.BlockStartAscent() +
                        std::max(mBoundingMetrics.descent, descent);

  if (aPlaceOrigin) {
    nscoord dy, dx = 0;
    for (nsIFrame* childFrame = GetFirstPrincipalChild(); childFrame;
         childFrame = childFrame->GetNextSibling()) {
      nsHTMLReflowMetrics childSize(aDesiredSize.GetWritingMode());
      GetReflowAndBoundingMetricsFor(childFrame, childSize,
                                     childSize.mBoundingMetrics);

      
      dy = childSize.Height() == 0 ? 0 : aDesiredSize.BlockStartAscent() - childSize.BlockStartAscent();
      FinishReflowChild(childFrame, PresContext(), childSize, nullptr, dx, dy, 0);
      dx += childSize.Width();
    }
  }

  SetReference(nsPoint(0, aDesiredSize.BlockStartAscent()));

  return NS_OK;
}

