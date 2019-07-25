






































#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsContentUtils.h"
#include "nsCSSFrameConstructor.h"
#include "nsMathMLTokenFrame.h"

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

  ProcessTextData();

  return NS_OK;
}

eMathMLFrameType
nsMathMLTokenFrame::GetMathMLFrameType()
{
  
  if (mContent->Tag() != nsGkAtoms::mi_) {
    return eMathMLFrameType_Ordinary;
  }

  
  nsAutoString style;
  
  
  mContent->GetAttr(kNameSpaceID_None,
                    nsGkAtoms::_moz_math_fontstyle_, style) ||
    GetAttribute(mContent, mPresentationData.mstyle, nsGkAtoms::mathvariant_,
                 style) ||
    GetAttribute(mContent, mPresentationData.mstyle, nsGkAtoms::fontstyle_,
                 style);

  if (style.EqualsLiteral("italic") || style.EqualsLiteral("bold-italic") ||
      style.EqualsLiteral("script") || style.EqualsLiteral("bold-script") ||
      style.EqualsLiteral("sans-serif-italic") ||
      style.EqualsLiteral("sans-serif-bold-italic")) {
    return eMathMLFrameType_ItalicIdentifier;
  }
  else if(style.EqualsLiteral("invariant")) {
    nsAutoString data;
    nsContentUtils::GetNodeTextContent(mContent, false, data);
    eMATHVARIANT variant = nsMathMLOperators::LookupInvariantChar(data);

    switch (variant) {
    case eMATHVARIANT_italic:
    case eMATHVARIANT_bold_italic:
    case eMATHVARIANT_script:
    case eMATHVARIANT_bold_script:
    case eMATHVARIANT_sans_serif_italic:
    case eMATHVARIANT_sans_serif_bold_italic:
      return eMathMLFrameType_ItalicIdentifier;
    default:
      ; 
    }
  }
  return eMathMLFrameType_UprightIdentifier;
}

static void
CompressWhitespace(nsIContent* aContent)
{
  PRUint32 numKids = aContent->GetChildCount();
  for (PRUint32 kid = 0; kid < numKids; kid++) {
    nsIContent* cont = aContent->GetChildAt(kid);
    if (cont && cont->IsNodeOfType(nsINode::eTEXT)) {
      nsAutoString text;
      cont->AppendTextTo(text);
      text.CompressWhitespace();
      cont->SetText(text, false); 
    }
  }
}

NS_IMETHODIMP
nsMathMLTokenFrame::Init(nsIContent*      aContent,
                         nsIFrame*        aParent,
                         nsIFrame*        aPrevInFlow)
{
  
  
  
  CompressWhitespace(aContent);

  
  return nsMathMLContainerFrame::Init(aContent, aParent, aPrevInFlow);
}

NS_IMETHODIMP
nsMathMLTokenFrame::SetInitialChildList(ChildListID     aListID,
                                        nsFrameList&    aChildList)
{
  
  nsresult rv = nsMathMLContainerFrame::SetInitialChildList(aListID, aChildList);
  if (NS_FAILED(rv))
    return rv;

  SetQuotes(false);
  ProcessTextData();
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
                                   childSize.mBoundingMetrics, nsnull);
    
    mBoundingMetrics += childSize.mBoundingMetrics;
  }

  nsRefPtr<nsFontMetrics> fm;
  nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fm));
  nscoord ascent = fm->MaxAscent();
  nscoord descent = fm->MaxDescent();

  aDesiredSize.mBoundingMetrics = mBoundingMetrics;
  aDesiredSize.width = mBoundingMetrics.width;
  aDesiredSize.ascent = NS_MAX(mBoundingMetrics.ascent, ascent);
  aDesiredSize.height = aDesiredSize.ascent +
                        NS_MAX(mBoundingMetrics.descent, descent);

  if (aPlaceOrigin) {
    nscoord dy, dx = 0;
    for (nsIFrame* childFrame = GetFirstPrincipalChild(); childFrame;
         childFrame = childFrame->GetNextSibling()) {
      nsHTMLReflowMetrics childSize;
      GetReflowAndBoundingMetricsFor(childFrame, childSize,
                                     childSize.mBoundingMetrics);

      
      dy = childSize.height == 0 ? 0 : aDesiredSize.ascent - childSize.ascent;
      FinishReflowChild(childFrame, PresContext(), nsnull, childSize, dx, dy, 0);
      dx += childSize.width;
    }
  }

  SetReference(nsPoint(0, aDesiredSize.ascent));

  return NS_OK;
}

 void
nsMathMLTokenFrame::MarkIntrinsicWidthsDirty()
{
  
  
  ProcessTextData();

  nsMathMLContainerFrame::MarkIntrinsicWidthsDirty();
}

NS_IMETHODIMP
nsMathMLTokenFrame::AttributeChanged(PRInt32         aNameSpaceID,
                                     nsIAtom*        aAttribute,
                                     PRInt32         aModType)
{
  if (nsGkAtoms::lquote_ == aAttribute ||
      nsGkAtoms::rquote_ == aAttribute) {
    SetQuotes(true);
  }

  return nsMathMLContainerFrame::
         AttributeChanged(aNameSpaceID, aAttribute, aModType);
}

void
nsMathMLTokenFrame::ProcessTextData()
{
  
  if (!SetTextStyle())
    return;

  
  PresContext()->PresShell()->FrameConstructor()->
    PostRestyleEvent(mContent->AsElement(), eRestyle_Subtree, NS_STYLE_HINT_NONE);
}




























bool
nsMathMLTokenFrame::SetTextStyle()
{
  if (mContent->Tag() != nsGkAtoms::mi_)
    return false;

  if (!mFrames.FirstChild())
    return false;

  
  nsAutoString data;
  nsContentUtils::GetNodeTextContent(mContent, false, data);
  PRInt32 length = data.Length();
  if (!length)
    return false;

  nsAutoString fontstyle;
  bool isSingleCharacter =
    length == 1 ||
    (length == 2 && NS_IS_HIGH_SURROGATE(data[0]));
  if (isSingleCharacter &&
      nsMathMLOperators::LookupInvariantChar(data) != eMATHVARIANT_NONE) {
    
    fontstyle.AssignLiteral("invariant");
  }
  else {
    
    nsAutoString value;
    if (!(GetAttribute(mContent, mPresentationData.mstyle,
                       nsGkAtoms::mathvariant_, value) ||
          GetAttribute(mContent, mPresentationData.mstyle,
                       nsGkAtoms::fontstyle_, value))) {
      if (!isSingleCharacter) {
        fontstyle.AssignLiteral("normal");
      }
      else if (length == 1 && 
               !nsMathMLOperators::
                TransformVariantChar(data[0], eMATHVARIANT_italic).
                Equals(data)) {
        
        
        fontstyle.AssignLiteral("italic");
      }
      
      
      
    }
  }

  
  if (fontstyle.IsEmpty()) {
    if (mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::_moz_math_fontstyle_)) {
      mContent->UnsetAttr(kNameSpaceID_None, nsGkAtoms::_moz_math_fontstyle_,
                          false);
      return true;
    }
  }
  else if (!mContent->AttrValueIs(kNameSpaceID_None,
                                  nsGkAtoms::_moz_math_fontstyle_,
                                  fontstyle, eCaseMatters)) {
    mContent->SetAttr(kNameSpaceID_None, nsGkAtoms::_moz_math_fontstyle_,
                      fontstyle, false);
    return true;
  }

  return false;
}

















static void
SetQuote(nsIFrame* aFrame, nsString& aValue, bool aNotify)
{
  if (!aFrame)
    return;

  nsIFrame* textFrame = aFrame->GetFirstPrincipalChild();
  if (!textFrame)
    return;

  nsIContent* quoteContent = textFrame->GetContent();
  if (!quoteContent->IsNodeOfType(nsINode::eTEXT))
    return;

  quoteContent->SetText(aValue, aNotify);
}

void
nsMathMLTokenFrame::SetQuotes(bool aNotify)
{
  if (mContent->Tag() != nsGkAtoms::ms_)
    return;

  nsAutoString value;
  
  if (GetAttribute(mContent, mPresentationData.mstyle,
                   nsGkAtoms::lquote_, value)) {
    SetQuote(nsLayoutUtils::GetBeforeFrame(this), value, aNotify);
  }
  
  if (GetAttribute(mContent, mPresentationData.mstyle,
                   nsGkAtoms::rquote_, value)) {
    SetQuote(nsLayoutUtils::GetAfterFrame(this), value, aNotify);
  }
}
