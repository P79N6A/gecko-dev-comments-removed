








































#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsPresContext.h"
#include "nsUnitConversion.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIRenderingContext.h"
#include "nsIFontMetrics.h"

#include "nsMathMLmfencedFrame.h"





nsIFrame*
NS_NewMathMLmfencedFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLmfencedFrame(aContext);
}

nsMathMLmfencedFrame::~nsMathMLmfencedFrame()
{
  RemoveFencesAndSeparators();
}

NS_IMETHODIMP
nsMathMLmfencedFrame::InheritAutomaticData(nsIFrame* aParent)
{
  
  nsMathMLContainerFrame::InheritAutomaticData(aParent);

  mPresentationData.flags |= NS_MATHML_STRETCH_ALL_CHILDREN_VERTICALLY;

  return NS_OK;
}

NS_IMETHODIMP
nsMathMLmfencedFrame::SetInitialChildList(nsIAtom*        aListName,
                                          nsIFrame*       aChildList)
{
  
  nsresult rv = nsMathMLContainerFrame::SetInitialChildList(aListName, aChildList);
  if (NS_FAILED(rv)) return rv;

  
  
  
  return CreateFencesAndSeparators(PresContext());
}

NS_IMETHODIMP
nsMathMLmfencedFrame::AttributeChanged(PRInt32         aNameSpaceID,
                                       nsIAtom*        aAttribute,
                                       PRInt32         aModType)
{
  RemoveFencesAndSeparators();
  CreateFencesAndSeparators(PresContext());

  return nsMathMLContainerFrame::
         AttributeChanged(aNameSpaceID, aAttribute, aModType);
}

nsresult
nsMathMLmfencedFrame::ChildListChanged(PRInt32 aModType)
{
  RemoveFencesAndSeparators();
  CreateFencesAndSeparators(PresContext());

  return nsMathMLContainerFrame::ChildListChanged(aModType);
}

void
nsMathMLmfencedFrame::RemoveFencesAndSeparators()
{
  if (mOpenChar) delete mOpenChar;
  if (mCloseChar) delete mCloseChar;
  if (mSeparatorsChar) delete[] mSeparatorsChar;

  mOpenChar = nsnull;
  mCloseChar = nsnull;
  mSeparatorsChar = nsnull;
  mSeparatorsCount = 0;
}

nsresult
nsMathMLmfencedFrame::CreateFencesAndSeparators(nsPresContext* aPresContext)
{
  nsAutoString value;
  PRBool isMutable = PR_FALSE;

  
  
  if (!GetAttribute(mContent, mPresentationData.mstyle, nsGkAtoms::open,
                    value)) {
    value = PRUnichar('('); 
  }
  else {
    value.Trim(" ");
  }

  if (!value.IsEmpty()) {
    mOpenChar = new nsMathMLChar;
    if (!mOpenChar) return NS_ERROR_OUT_OF_MEMORY;
    mOpenChar->SetData(aPresContext, value);
    isMutable = nsMathMLOperators::IsMutableOperator(value);
    ResolveMathMLCharStyle(aPresContext, mContent, mStyleContext, mOpenChar, isMutable);
  }

  
  
  if(!GetAttribute(mContent, mPresentationData.mstyle,
                    nsGkAtoms::close, value)) {
    value = PRUnichar(')'); 
  }
  else {
    value.Trim(" ");
  }

  if (!value.IsEmpty()) {
    mCloseChar = new nsMathMLChar;
    if (!mCloseChar) return NS_ERROR_OUT_OF_MEMORY;
    mCloseChar->SetData(aPresContext, value);
    isMutable = nsMathMLOperators::IsMutableOperator(value);
    ResolveMathMLCharStyle(aPresContext, mContent, mStyleContext, mCloseChar, isMutable);
  }

  
  
  if(!GetAttribute(mContent, mPresentationData.mstyle, 
                   nsGkAtoms::separators_, value)) {
    value = PRUnichar(','); 
  }
  else {
    value.Trim(" ");
  }

  mSeparatorsCount = value.Length();
  if (0 < mSeparatorsCount) {
    PRInt32 sepCount = mFrames.GetLength() - 1;
    if (0 < sepCount) {
      mSeparatorsChar = new nsMathMLChar[sepCount];
      if (!mSeparatorsChar) return NS_ERROR_OUT_OF_MEMORY;
      nsAutoString sepChar;
      for (PRInt32 i = 0; i < sepCount; i++) {
        if (i < mSeparatorsCount) {
          sepChar = value[i];
          isMutable = nsMathMLOperators::IsMutableOperator(sepChar);
        }
        else {
          sepChar = value[mSeparatorsCount-1];
          
        }
        mSeparatorsChar[i].SetData(aPresContext, sepChar);
        ResolveMathMLCharStyle(aPresContext, mContent, mStyleContext, &mSeparatorsChar[i], isMutable);
      }
    }
    mSeparatorsCount = sepCount;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsMathMLmfencedFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                       const nsRect&           aDirtyRect,
                                       const nsDisplayListSet& aLists)
{
  
  
  nsresult rv = nsMathMLContainerFrame::BuildDisplayList(aBuilder, aDirtyRect, aLists);
  NS_ENSURE_SUCCESS(rv, rv);
  
  
  
  if (mOpenChar) {
    rv = mOpenChar->Display(aBuilder, this, aLists);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  if (mCloseChar) {
    rv = mCloseChar->Display(aBuilder, this, aLists);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  for (PRInt32 i = 0; i < mSeparatorsCount; i++) {
    rv = mSeparatorsChar[i].Display(aBuilder, this, aLists);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsMathMLmfencedFrame::Reflow(nsPresContext*          aPresContext,
                             nsHTMLReflowMetrics&     aDesiredSize,
                             const nsHTMLReflowState& aReflowState,
                             nsReflowStatus&          aStatus)
{
  return doReflow(aPresContext, aReflowState, aDesiredSize, aStatus, this,
                  mOpenChar, mCloseChar, mSeparatorsChar, mSeparatorsCount);
}



 nsresult
nsMathMLmfencedFrame::doReflow(nsPresContext*          aPresContext,
                               const nsHTMLReflowState& aReflowState,
                               nsHTMLReflowMetrics&     aDesiredSize,
                               nsReflowStatus&          aStatus,
                               nsIFrame*                aForFrame,
                               nsMathMLChar*            aOpenChar,
                               nsMathMLChar*            aCloseChar,
                               nsMathMLChar*            aSeparatorsChar,
                               PRInt32                  aSeparatorsCount)

{
  nsresult rv;
  aDesiredSize.width = aDesiredSize.height = 0;
  aDesiredSize.ascent = 0;
  aDesiredSize.mBoundingMetrics.Clear();

  nsMathMLContainerFrame* mathMLFrame =
    static_cast<nsMathMLContainerFrame*>(aForFrame);

  PRInt32 i;
  nsCOMPtr<nsIFontMetrics> fm;
  aReflowState.rendContext->SetFont(aForFrame->GetStyleFont()->mFont, nsnull);
  aReflowState.rendContext->GetFontMetrics(*getter_AddRefs(fm));
  nscoord axisHeight, em;
  GetAxisHeight(*aReflowState.rendContext, fm, axisHeight);
  GetEmHeight(fm, em);
  
  nscoord leading = NSToCoordRound(0.2f * em); 

  
  
  

  
  
  
  
  
  
  

  PRInt32 count = 0;
  nsReflowStatus childStatus;
  nsSize availSize(aReflowState.ComputedWidth(), aReflowState.mComputedHeight);
  nsHTMLReflowMetrics childDesiredSize(
                      aDesiredSize.mFlags | NS_REFLOW_CALC_BOUNDING_METRICS);
  nsIFrame* firstChild = aForFrame->GetFirstChild(nsnull);
  nsIFrame* childFrame = firstChild;
  nscoord ascent = 0, descent = 0;
  if (firstChild || aOpenChar || aCloseChar || aSeparatorsCount > 0) {
    
    
    fm->GetMaxAscent(ascent);
    fm->GetMaxDescent(descent);
  }
  while (childFrame) {
    nsHTMLReflowState childReflowState(aPresContext, aReflowState,
                                       childFrame, availSize);
    rv = mathMLFrame->ReflowChild(childFrame, aPresContext, childDesiredSize,
                                  childReflowState, childStatus);
    
    if (NS_FAILED(rv)) {
      
      mathMLFrame->DidReflowChildren(firstChild, childFrame);
      return rv;
    }

    
    
    
    childFrame->SetRect(nsRect(0, childDesiredSize.ascent,
                               childDesiredSize.width, childDesiredSize.height));

    
    nscoord childDescent = childDesiredSize.height - childDesiredSize.ascent;
    if (descent < childDescent)
      descent = childDescent;
    if (ascent < childDesiredSize.ascent)
      ascent = childDesiredSize.ascent;
    if (0 == count++)
      aDesiredSize.mBoundingMetrics  = childDesiredSize.mBoundingMetrics;
    else
      aDesiredSize.mBoundingMetrics += childDesiredSize.mBoundingMetrics;

    childFrame = childFrame->GetNextSibling();
  }

  
  

  nsBoundingMetrics containerSize;
  nsStretchDirection stretchDir = NS_STRETCH_DIRECTION_VERTICAL;

  nsPresentationData presentationData;
  mathMLFrame->GetPresentationData(presentationData);
  if (!NS_MATHML_WILL_STRETCH_ALL_CHILDREN_VERTICALLY(presentationData.flags)) {
    
    containerSize = aDesiredSize.mBoundingMetrics; 
  }
  else {
    
    mathMLFrame->GetPreferredStretchSize(*aReflowState.rendContext,
                                         0, 
                                         stretchDir, containerSize);
    childFrame = firstChild;
    while (childFrame) {
      nsIMathMLFrame* mathmlChild;
      childFrame->QueryInterface(NS_GET_IID(nsIMathMLFrame), (void**)&mathmlChild);
      if (mathmlChild) {
        
        GetReflowAndBoundingMetricsFor(childFrame, childDesiredSize, childDesiredSize.mBoundingMetrics);

        mathmlChild->Stretch(*aReflowState.rendContext, 
                             stretchDir, containerSize, childDesiredSize);
        
        childFrame->SetRect(nsRect(0, childDesiredSize.ascent,
                                   childDesiredSize.width, childDesiredSize.height));

        nscoord childDescent = childDesiredSize.height - childDesiredSize.ascent;
        if (descent < childDescent)
          descent = childDescent;
        if (ascent < childDesiredSize.ascent)
          ascent = childDesiredSize.ascent;
      }
      childFrame = childFrame->GetNextSibling();
    }
    
    mathMLFrame->GetPreferredStretchSize(*aReflowState.rendContext,
                                         STRETCH_CONSIDER_EMBELLISHMENTS,
                                         stretchDir, containerSize);
  }

  
  
  

  
  if (firstChild) { 
    nscoord delta = PR_MAX(containerSize.ascent - axisHeight, 
                           containerSize.descent + axisHeight);
    containerSize.ascent = delta + axisHeight;
    containerSize.descent = delta - axisHeight;
  }

  
  
  ReflowChar(aPresContext, *aReflowState.rendContext, aOpenChar,
             NS_MATHML_OPERATOR_FORM_PREFIX, presentationData.scriptLevel, 
             axisHeight, leading, em, containerSize, ascent, descent);
  
  
  for (i = 0; i < aSeparatorsCount; i++) {
    ReflowChar(aPresContext, *aReflowState.rendContext, &aSeparatorsChar[i],
               NS_MATHML_OPERATOR_FORM_INFIX, presentationData.scriptLevel,
               axisHeight, leading, em, containerSize, ascent, descent);
  }
  
  
  ReflowChar(aPresContext, *aReflowState.rendContext, aCloseChar,
             NS_MATHML_OPERATOR_FORM_POSTFIX, presentationData.scriptLevel,
             axisHeight, leading, em, containerSize, ascent, descent);

  
  
  

  i = 0;
  nscoord dx = 0;
  nsBoundingMetrics bm;
  PRBool firstTime = PR_TRUE;
  if (aOpenChar) {
    PlaceChar(aOpenChar, ascent, bm, dx);
    aDesiredSize.mBoundingMetrics = bm;
    firstTime = PR_FALSE;
  }

  childFrame = firstChild;
  while (childFrame) {
    nsHTMLReflowMetrics childSize;
    GetReflowAndBoundingMetricsFor(childFrame, childSize, bm);
    if (firstTime) {
      firstTime = PR_FALSE;
      aDesiredSize.mBoundingMetrics  = bm;
    }
    else  
      aDesiredSize.mBoundingMetrics += bm;

    mathMLFrame->FinishReflowChild(childFrame, aPresContext, nsnull, childSize, 
                                   dx, ascent - childSize.ascent, 0);
    dx += childSize.width;

    if (i < aSeparatorsCount) {
      PlaceChar(&aSeparatorsChar[i], ascent, bm, dx);
      aDesiredSize.mBoundingMetrics += bm;
    }
    i++;

    childFrame = childFrame->GetNextSibling();
  }

  if (aCloseChar) {
    PlaceChar(aCloseChar, ascent, bm, dx);
    if (firstTime)
      aDesiredSize.mBoundingMetrics  = bm;
    else  
      aDesiredSize.mBoundingMetrics += bm;
  }

  aDesiredSize.width = aDesiredSize.mBoundingMetrics.width;
  aDesiredSize.height = ascent + descent;
  aDesiredSize.ascent = ascent;

  mathMLFrame->SetBoundingMetrics(aDesiredSize.mBoundingMetrics);
  mathMLFrame->SetReference(nsPoint(0, aDesiredSize.ascent));

  
  mathMLFrame->FixInterFrameSpacing(aDesiredSize);

  aStatus = NS_FRAME_COMPLETE;
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  return NS_OK;
}


 nsresult
nsMathMLmfencedFrame::ReflowChar(nsPresContext*      aPresContext,
                                 nsIRenderingContext& aRenderingContext,
                                 nsMathMLChar*        aMathMLChar,
                                 nsOperatorFlags      aForm,
                                 PRInt32              aScriptLevel,
                                 nscoord              axisHeight,
                                 nscoord              leading,
                                 nscoord              em,
                                 nsBoundingMetrics&   aContainerSize,
                                 nscoord&             aAscent,
                                 nscoord&             aDescent)
{
  if (aMathMLChar && 0 < aMathMLChar->Length()) {
    nsOperatorFlags flags = 0;
    float leftSpace = 0.0f;
    float rightSpace = 0.0f;

    nsAutoString data;
    aMathMLChar->GetData(data);
    PRBool found = nsMathMLOperators::LookupOperator(data, aForm,              
                                           &flags, &leftSpace, &rightSpace);

    
    if (found && aScriptLevel > 0) {
      leftSpace /= 2.0f;
      rightSpace /= 2.0f;
    }

    
    nsBoundingMetrics charSize;
    nsresult res = aMathMLChar->Stretch(aPresContext, aRenderingContext,
                                        NS_STRETCH_DIRECTION_VERTICAL,
                                        aContainerSize, charSize);

    if (NS_STRETCH_DIRECTION_UNSUPPORTED != aMathMLChar->GetStretchDirection()) {
      
      nscoord height = charSize.ascent + charSize.descent;
      charSize.ascent = height/2 + axisHeight;
      charSize.descent = height - charSize.ascent;
    }
    else {
      
      
      leading = 0;
      if (NS_FAILED(res)) {
        nsTextDimensions dimensions;
        aRenderingContext.GetTextDimensions(data.get(), data.Length(), dimensions);
        charSize.ascent = dimensions.ascent;
        charSize.descent = dimensions.descent;
        charSize.width = dimensions.width;
        
        
        aMathMLChar->SetBoundingMetrics(charSize);
      }
    }

    if (aAscent < charSize.ascent + leading) 
      aAscent = charSize.ascent + leading;
    if (aDescent < charSize.descent + leading) 
      aDescent = charSize.descent + leading;

    
    charSize.width += NSToCoordRound((leftSpace + rightSpace) * em);

    
    
    aMathMLChar->SetRect(nsRect(NSToCoordRound(leftSpace * em), 
                                charSize.ascent, charSize.width,
                                charSize.ascent + charSize.descent));
  }
  return NS_OK;
}

 void
nsMathMLmfencedFrame::PlaceChar(nsMathMLChar*      aMathMLChar,
                                nscoord            aDesiredAscent,
                                nsBoundingMetrics& bm,
                                nscoord&           dx)
{
  aMathMLChar->GetBoundingMetrics(bm);

  
  
  nsRect rect;
  aMathMLChar->GetRect(rect);

  nscoord dy = aDesiredAscent - rect.y;
  if (aMathMLChar->GetStretchDirection() != NS_STRETCH_DIRECTION_UNSUPPORTED) {
    
    
    bm.descent = (bm.ascent + bm.descent) - rect.y;
    bm.ascent = rect.y;
  }

  aMathMLChar->SetRect(nsRect(dx + rect.x, dy, bm.width, rect.height));

  bm.leftBearing += rect.x;
  bm.rightBearing += rect.x;

  
  bm.width = rect.width;
  dx += rect.width;
}

nscoord
nsMathMLmfencedFrame::FixInterFrameSpacing(nsHTMLReflowMetrics& aDesiredSize)
{
  nscoord gap = nsMathMLContainerFrame::FixInterFrameSpacing(aDesiredSize);
  if (!gap) return 0;

  nsRect rect;
  if (mOpenChar) {
    mOpenChar->GetRect(rect);
    rect.MoveBy(gap, 0);
    mOpenChar->SetRect(rect);
  }
  if (mCloseChar) {
    mCloseChar->GetRect(rect);
    rect.MoveBy(gap, 0);
    mCloseChar->SetRect(rect);
  }
  for (PRInt32 i = 0; i < mSeparatorsCount; i++) {
    mSeparatorsChar[i].GetRect(rect);
    rect.MoveBy(gap, 0);
    mSeparatorsChar[i].SetRect(rect);
  }
  return gap;
}



nsStyleContext*
nsMathMLmfencedFrame::GetAdditionalStyleContext(PRInt32 aIndex) const
{
  PRInt32 openIndex = -1;
  PRInt32 closeIndex = -1;
  PRInt32 lastIndex = mSeparatorsCount-1;

  if (mOpenChar) { 
    lastIndex++; 
    openIndex = lastIndex; 
  }
  if (mCloseChar) { 
    lastIndex++;
    closeIndex = lastIndex;
  }
  if (aIndex < 0 || aIndex > lastIndex) {
    return nsnull;
  }

  if (aIndex < mSeparatorsCount) {
    return mSeparatorsChar[aIndex].GetStyleContext();
  }
  else if (aIndex == openIndex) {
    return mOpenChar->GetStyleContext();
  }
  else if (aIndex == closeIndex) {
    return mCloseChar->GetStyleContext();
  }
  return nsnull;
}

void
nsMathMLmfencedFrame::SetAdditionalStyleContext(PRInt32          aIndex, 
                                                nsStyleContext*  aStyleContext)
{
  PRInt32 openIndex = -1;
  PRInt32 closeIndex = -1;
  PRInt32 lastIndex = mSeparatorsCount-1;

  if (mOpenChar) {
    lastIndex++;
    openIndex = lastIndex;
  }
  if (mCloseChar) {
    lastIndex++;
    closeIndex = lastIndex;
  }
  if (aIndex < 0 || aIndex > lastIndex) {
    return;
  }

  if (aIndex < mSeparatorsCount) {
    mSeparatorsChar[aIndex].SetStyleContext(aStyleContext);
  }
  else if (aIndex == openIndex) {
    mOpenChar->SetStyleContext(aStyleContext);
  }
  else if (aIndex == closeIndex) {
    mCloseChar->SetStyleContext(aStyleContext);
  }
}
