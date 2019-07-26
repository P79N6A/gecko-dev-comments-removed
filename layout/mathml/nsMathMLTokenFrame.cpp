




#include "nsMathMLTokenFrame.h"
#include "nsPresContext.h"
#include "nsContentUtils.h"
#include "nsTextFrame.h"
#include "RestyleManager.h"
#include <algorithm>

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
  
  if (mContent->Tag() != nsGkAtoms::mi_) {
    return eMathMLFrameType_Ordinary;
  }

  uint8_t mathVariant = StyleFont()->mMathVariant;
  if ((mathVariant == NS_MATHML_MATHVARIANT_NONE &&
       (StyleFont()->mFont.style == NS_STYLE_FONT_STYLE_ITALIC ||
        HasAnyStateBits(TEXT_IS_IN_SINGLE_CHAR_MI))) ||
      mathVariant == NS_MATHML_MATHVARIANT_ITALIC ||
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
  if (mContent->Tag() == nsGkAtoms::mi_ && childCount == 1) {
    nsAutoString data;
    nsContentUtils::GetNodeTextContent(mContent, false, data);
    data.CompressWhitespace();
    int32_t length = data.Length();

    bool isSingleCharacter = length == 1 ||
      (length == 2 && NS_IS_HIGH_SURROGATE(data[0]));

    if (isSingleCharacter) {
      child->AddStateBits(TEXT_IS_IN_SINGLE_CHAR_MI);
    }
  }
}

NS_IMETHODIMP
nsMathMLTokenFrame::SetInitialChildList(ChildListID     aListID,
                                        nsFrameList&    aChildList)
{
  
  nsresult rv = nsMathMLContainerFrame::SetInitialChildList(aListID, aChildList);
  if (NS_FAILED(rv))
    return rv;

  MarkTextFramesAsTokenMathML();

  return rv;
}

NS_IMETHODIMP
nsMathMLTokenFrame::AppendFrames(ChildListID aListID,
                                 nsFrameList& aChildList)
{
  nsresult rv = nsMathMLContainerFrame::AppendFrames(aListID, aChildList);
  if (NS_FAILED(rv))
    return rv;

  MarkTextFramesAsTokenMathML();

  return rv;
}

NS_IMETHODIMP
nsMathMLTokenFrame::InsertFrames(ChildListID aListID,
                                 nsIFrame* aPrevFrame,
                                 nsFrameList& aChildList)
{
  nsresult rv = nsMathMLContainerFrame::InsertFrames(aListID, aPrevFrame,
                                                     aChildList);
  if (NS_FAILED(rv))
    return rv;

  MarkTextFramesAsTokenMathML();

  return rv;
}

nsresult
nsMathMLTokenFrame::Reflow(nsPresContext*          aPresContext,
                           nsHTMLReflowMetrics&     aDesiredSize,
                           const nsHTMLReflowState& aReflowState,
                           nsReflowStatus&          aStatus)
{
  nsresult rv = NS_OK;

  
  aDesiredSize.width = aDesiredSize.height = 0;
  aDesiredSize.ascent = 0;
  aDesiredSize.mBoundingMetrics = nsBoundingMetrics();

  nsSize availSize(aReflowState.ComputedWidth(), NS_UNCONSTRAINEDSIZE);
  nsIFrame* childFrame = GetFirstPrincipalChild();
  while (childFrame) {
    
    nsHTMLReflowMetrics childDesiredSize(aDesiredSize.mFlags
                                         | NS_REFLOW_CALC_BOUNDING_METRICS);
    nsHTMLReflowState childReflowState(aPresContext, aReflowState,
                                       childFrame, availSize);
    rv = ReflowChild(childFrame, aPresContext, childDesiredSize,
                     childReflowState, aStatus);
    
    if (NS_FAILED(rv)) {
      
      DidReflowChildren(GetFirstPrincipalChild(), childFrame);
      return rv;
    }

    SaveReflowAndBoundingMetricsFor(childFrame, childDesiredSize,
                                    childDesiredSize.mBoundingMetrics);

    childFrame = childFrame->GetNextSibling();
  }


  
  FinalizeReflow(*aReflowState.rendContext, aDesiredSize);

  aStatus = NS_FRAME_COMPLETE;
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  return NS_OK;
}




 nsresult
nsMathMLTokenFrame::Place(nsRenderingContext& aRenderingContext,
                          bool                 aPlaceOrigin,
                          nsHTMLReflowMetrics& aDesiredSize)
{
  mBoundingMetrics = nsBoundingMetrics();
  for (nsIFrame* childFrame = GetFirstPrincipalChild(); childFrame;
       childFrame = childFrame->GetNextSibling()) {
    nsHTMLReflowMetrics childSize;
    GetReflowAndBoundingMetricsFor(childFrame, childSize,
                                   childSize.mBoundingMetrics, nullptr);
    
    mBoundingMetrics += childSize.mBoundingMetrics;
  }

  nsRefPtr<nsFontMetrics> fm;
  nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fm));
  nscoord ascent = fm->MaxAscent();
  nscoord descent = fm->MaxDescent();

  aDesiredSize.mBoundingMetrics = mBoundingMetrics;
  aDesiredSize.width = mBoundingMetrics.width;
  aDesiredSize.ascent = std::max(mBoundingMetrics.ascent, ascent);
  aDesiredSize.height = aDesiredSize.ascent +
                        std::max(mBoundingMetrics.descent, descent);

  if (aPlaceOrigin) {
    nscoord dy, dx = 0;
    for (nsIFrame* childFrame = GetFirstPrincipalChild(); childFrame;
         childFrame = childFrame->GetNextSibling()) {
      nsHTMLReflowMetrics childSize;
      GetReflowAndBoundingMetricsFor(childFrame, childSize,
                                     childSize.mBoundingMetrics);

      
      dy = childSize.height == 0 ? 0 : aDesiredSize.ascent - childSize.ascent;
      FinishReflowChild(childFrame, PresContext(), nullptr, childSize, dx, dy, 0);
      dx += childSize.width;
    }
  }

  SetReference(nsPoint(0, aDesiredSize.ascent));

  return NS_OK;
}

