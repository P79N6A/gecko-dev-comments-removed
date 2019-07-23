





































#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsPresContext.h"
#include "nsUnitConversion.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIRenderingContext.h"
#include "nsIFontMetrics.h"
#include "nsContentUtils.h"
#include "nsFrameManager.h"
#include "nsStyleChangeList.h"
#include "nsMathMLTokenFrame.h"

nsIFrame*
NS_NewMathMLTokenFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLTokenFrame(aContext);
}
nsMathMLTokenFrame::~nsMathMLTokenFrame()
{
}

eMathMLFrameType
nsMathMLTokenFrame::GetMathMLFrameType()
{
  
  if (mContent->Tag() != nsGkAtoms::mi_) {
    return eMathMLFrameType_Ordinary;
  }

  
  
  return mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::MOZfontstyle,
                               nsGkAtoms::normal, eCaseMatters)
    ? eMathMLFrameType_UprightIdentifier
    : eMathMLFrameType_ItalicIdentifier;
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
                                        nsIFrame*       aChildList)
{
  
  nsresult rv = nsMathMLContainerFrame::SetInitialChildList(aListName, aChildList);
  if (NS_FAILED(rv))
    return rv;

  SetQuotes();
  ProcessTextData(PR_FALSE);
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

  
  nsHTMLReflowMetrics childDesiredSize(
                      aDesiredSize.mFlags | NS_REFLOW_CALC_BOUNDING_METRICS);
  nsSize availSize(aReflowState.ComputedWidth(), aReflowState.mComputedHeight);
  PRInt32 count = 0;
  nsIFrame* childFrame = GetFirstChild(nsnull);
  while (childFrame) {
    nsHTMLReflowState childReflowState(aPresContext, aReflowState,
                                       childFrame, availSize);
    rv = ReflowChild(childFrame, aPresContext, childDesiredSize,
                     childReflowState, aStatus);
    
    if (NS_FAILED(rv)) {
      
      DidReflowChildren(GetFirstChild(nsnull), childFrame);
      return rv;
    }

    
    childFrame->SetRect(nsRect(0, childDesiredSize.ascent,
                               childDesiredSize.width, childDesiredSize.height));
    
    if (0 == count)
      aDesiredSize.mBoundingMetrics  = childDesiredSize.mBoundingMetrics;
    else
      aDesiredSize.mBoundingMetrics += childDesiredSize.mBoundingMetrics;

    count++;
    childFrame = childFrame->GetNextSibling();
  }

  
  mBoundingMetrics = aDesiredSize.mBoundingMetrics;

  
  FinalizeReflow(*aReflowState.rendContext, aDesiredSize);

  
  
  aDesiredSize.mOverflowArea.SetRect(0, 0, aDesiredSize.width, aDesiredSize.height);
  FinishAndStoreOverflow(&aDesiredSize);

  aStatus = NS_FRAME_COMPLETE;
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  return NS_OK;
}




nsresult
nsMathMLTokenFrame::Place(nsIRenderingContext& aRenderingContext,
                          PRBool               aPlaceOrigin,
                          nsHTMLReflowMetrics& aDesiredSize)
{
  nsCOMPtr<nsIFontMetrics> fm =
    GetPresContext()->GetMetricsFor(GetStyleFont()->mFont);
  nscoord ascent, descent;
  fm->GetMaxAscent(ascent);
  fm->GetMaxDescent(descent);

  aDesiredSize.mBoundingMetrics = mBoundingMetrics;
  aDesiredSize.width = mBoundingMetrics.width;
  aDesiredSize.ascent = PR_MAX(mBoundingMetrics.ascent, ascent);
  aDesiredSize.height = aDesiredSize.ascent +
                        PR_MAX(mBoundingMetrics.descent, descent);

  if (aPlaceOrigin) {
    nscoord dy, dx = 0;
    nsIFrame* childFrame = GetFirstChild(nsnull);
    while (childFrame) {
      nsRect rect = childFrame->GetRect();
      nsHTMLReflowMetrics childSize;
      childSize.width = rect.width;
      childSize.height = aDesiredSize.height; 

      
      dy = rect.IsEmpty() ? 0 : aDesiredSize.ascent - rect.y;
      FinishReflowChild(childFrame, GetPresContext(), nsnull, childSize, dx, dy, 0);
      dx += rect.width;
      childFrame = childFrame->GetNextSibling();
    }
  }

  SetReference(nsPoint(0, aDesiredSize.ascent));

  return NS_OK;
}

 void
nsMathMLTokenFrame::MarkIntrinsicWidthsDirty()
{
  
  
  ProcessTextData(PR_TRUE);

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
nsMathMLTokenFrame::ProcessTextData(PRBool aComputeStyleChange)
{
  
  if (!SetTextStyle())
    return;

  
  if (!aComputeStyleChange)
    return;

  
  nsFrameManager* fm = GetPresContext()->FrameManager();
  nsStyleChangeList changeList;
  fm->ComputeStyleChangeFor(this, &changeList, NS_STYLE_HINT_NONE);
#ifdef DEBUG
  
  nsIFrame* parentFrame = GetParent();
  fm->DebugVerifyStyleTree(parentFrame ? parentFrame : this);
#endif
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
  GetAttribute(mContent, mPresentationData.mstyle, nsGkAtoms::fontstyle_, fontstyle);
  if (1 == length && nsMathMLOperators::LookupInvariantChar(data[0])) {
    
    fontstyle.AssignLiteral("invariant");
  }
  if (fontstyle.IsEmpty()) {
    fontstyle.AssignASCII((1 == length) ? "italic" : "normal"); 
  }

  
  if (!mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::MOZfontstyle,
                             fontstyle, eCaseMatters)) {
    mContent->SetAttr(kNameSpaceID_None, nsGkAtoms::MOZfontstyle, fontstyle, PR_FALSE);
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
