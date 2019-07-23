






































#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIRenderingContext.h"
#include "nsIFontMetrics.h"
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

eMathMLFrameType
nsMathMLTokenFrame::GetMathMLFrameType()
{
  
  if (mContent->Tag() != nsGkAtoms::mi_) {
    return eMathMLFrameType_Ordinary;
  }

  
  
  
  nsAutoString style;
  
  
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::MOZfontstyle, style) ||
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::mathvariant_, style) ||
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::fontstyle_, style);

  if (style.EqualsLiteral("italic") || style.EqualsLiteral("bold-italic") ||
      style.EqualsLiteral("script") || style.EqualsLiteral("bold-script") ||
      style.EqualsLiteral("sans-serif-italic") ||
      style.EqualsLiteral("sans-serif-bold-italic")) {
    return eMathMLFrameType_ItalicIdentifier;
  }
  else if(style.EqualsLiteral("invariant")) {
    nsAutoString data;
    nsContentUtils::GetNodeTextContent(mContent, PR_FALSE, data);
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
      cont->SetText(text, PR_FALSE); 
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
nsMathMLTokenFrame::SetInitialChildList(nsIAtom*        aListName,
                                        nsFrameList&    aChildList)
{
  
  nsresult rv = nsMathMLContainerFrame::SetInitialChildList(aListName, aChildList);
  if (NS_FAILED(rv))
    return rv;

  SetQuotes();
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
  aDesiredSize.mBoundingMetrics.Clear();

  nsSize availSize(aReflowState.ComputedWidth(), NS_UNCONSTRAINEDSIZE);
  nsIFrame* childFrame = GetFirstChild(nsnull);
  while (childFrame) {
    
    nsHTMLReflowMetrics childDesiredSize(aDesiredSize.mFlags
                                         | NS_REFLOW_CALC_BOUNDING_METRICS);
    nsHTMLReflowState childReflowState(aPresContext, aReflowState,
                                       childFrame, availSize);
    rv = ReflowChild(childFrame, aPresContext, childDesiredSize,
                     childReflowState, aStatus);
    
    if (NS_FAILED(rv)) {
      
      DidReflowChildren(GetFirstChild(nsnull), childFrame);
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
nsMathMLTokenFrame::Place(nsIRenderingContext& aRenderingContext,
                          PRBool               aPlaceOrigin,
                          nsHTMLReflowMetrics& aDesiredSize)
{
  mBoundingMetrics.Clear();
  for (nsIFrame* childFrame = GetFirstChild(nsnull); childFrame;
       childFrame = childFrame->GetNextSibling()) {
    nsHTMLReflowMetrics childSize;
    GetReflowAndBoundingMetricsFor(childFrame, childSize,
                                   childSize.mBoundingMetrics, nsnull);
    
    mBoundingMetrics += childSize.mBoundingMetrics;
  }

  nsCOMPtr<nsIFontMetrics> fm =
    PresContext()->GetMetricsFor(GetStyleFont()->mFont);
  nscoord ascent, descent;
  fm->GetMaxAscent(ascent);
  fm->GetMaxDescent(descent);

  aDesiredSize.mBoundingMetrics = mBoundingMetrics;
  aDesiredSize.width = mBoundingMetrics.width;
  aDesiredSize.ascent = NS_MAX(mBoundingMetrics.ascent, ascent);
  aDesiredSize.height = aDesiredSize.ascent +
                        NS_MAX(mBoundingMetrics.descent, descent);

  if (aPlaceOrigin) {
    nscoord dy, dx = 0;
    for (nsIFrame* childFrame = GetFirstChild(nsnull); childFrame;
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
    SetQuotes();
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
    PostRestyleEvent(mContent, eReStyle_Self, NS_STYLE_HINT_NONE);
}




























PRBool
nsMathMLTokenFrame::SetTextStyle()
{
  if (mContent->Tag() != nsGkAtoms::mi_)
    return PR_FALSE;

  if (!mFrames.FirstChild())
    return PR_FALSE;

  
  nsAutoString data;
  nsContentUtils::GetNodeTextContent(mContent, PR_FALSE, data);
  PRInt32 length = data.Length();
  if (!length)
    return PR_FALSE;

  nsAutoString fontstyle;
  PRBool isSingleCharacter =
    length == 1 ||
    (length == 2 && NS_IS_HIGH_SURROGATE(data[0]));
  if (isSingleCharacter &&
      nsMathMLOperators::LookupInvariantChar(data) != eMATHVARIANT_NONE) {
    
    fontstyle.AssignLiteral("invariant");
  }
  else {
    
    if (!(mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::mathvariant_) ||
          mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::fontstyle_))) {
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
    if (mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::MOZfontstyle)) {
      mContent->UnsetAttr(kNameSpaceID_None, nsGkAtoms::MOZfontstyle, PR_FALSE);
      return PR_TRUE;
    }
  }
  else if (!mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::MOZfontstyle,
                                  fontstyle, eCaseMatters)) {
    mContent->SetAttr(kNameSpaceID_None, nsGkAtoms::MOZfontstyle,
                      fontstyle, PR_FALSE);
    return PR_TRUE;
  }

  return PR_FALSE;
}















static void
SetQuote(nsIFrame*       aFrame, 
         nsString&       aValue)
{
  nsIFrame* textFrame;
  do {
    
    textFrame = aFrame->GetFirstChild(nsnull);
    if (textFrame) {
      if (textFrame->GetType() == nsGkAtoms::textFrame)
        break;
    }
    aFrame = textFrame;
  } while (textFrame);
  if (textFrame) {
    nsIContent* quoteContent = textFrame->GetContent();
    if (quoteContent && quoteContent->IsNodeOfType(nsINode::eTEXT)) {
      quoteContent->SetText(aValue, PR_FALSE); 
    }
  }
}

void
nsMathMLTokenFrame::SetQuotes()
{
  if (mContent->Tag() != nsGkAtoms::ms_)
    return;

  nsIFrame* rightFrame = nsnull;
  nsIFrame* baseFrame = nsnull;
  nsIFrame* leftFrame = mFrames.FirstChild();
  if (leftFrame)
    baseFrame = leftFrame->GetNextSibling();
  if (baseFrame)
    rightFrame = baseFrame->GetNextSibling();
  if (!leftFrame || !baseFrame || !rightFrame)
    return;

  nsAutoString value;
  
  if (GetAttribute(mContent, mPresentationData.mstyle,
                   nsGkAtoms::lquote_, value)) {
    SetQuote(leftFrame, value);
  }
  
  if (GetAttribute(mContent, mPresentationData.mstyle,
                   nsGkAtoms::rquote_, value)) {
    SetQuote(rightFrame, value);
  }
}
