





#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsRenderingContext.h"

#include "nsMathMLmfencedFrame.h"
#include <algorithm>





nsIFrame*
NS_NewMathMLmfencedFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLmfencedFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsMathMLmfencedFrame)

nsMathMLmfencedFrame::~nsMathMLmfencedFrame()
{
  RemoveFencesAndSeparators();
}

NS_IMETHODIMP
nsMathMLmfencedFrame::InheritAutomaticData(nsIFrame* aParent)
{
  
  nsMathMLContainerFrame::InheritAutomaticData(aParent);

  mPresentationData.flags |= NS_MATHML_STRETCH_ALL_CHILDREN_VERTICALLY;

  RemoveFencesAndSeparators();
  CreateFencesAndSeparators(PresContext());

  return NS_OK;
}

NS_IMETHODIMP
nsMathMLmfencedFrame::SetInitialChildList(ChildListID     aListID,
                                          nsFrameList&    aChildList)
{
  
  nsresult rv = nsMathMLContainerFrame::SetInitialChildList(aListID, aChildList);
  if (NS_FAILED(rv)) return rv;

  
  
  
  mPresentationData.flags |= NS_MATHML_STRETCH_ALL_CHILDREN_VERTICALLY;
  
  
  
  CreateFencesAndSeparators(PresContext());
  return NS_OK;
}

NS_IMETHODIMP
nsMathMLmfencedFrame::AttributeChanged(int32_t         aNameSpaceID,
                                       nsIAtom*        aAttribute,
                                       int32_t         aModType)
{
  RemoveFencesAndSeparators();
  CreateFencesAndSeparators(PresContext());

  return nsMathMLContainerFrame::
         AttributeChanged(aNameSpaceID, aAttribute, aModType);
}

nsresult
nsMathMLmfencedFrame::ChildListChanged(int32_t aModType)
{
  RemoveFencesAndSeparators();
  CreateFencesAndSeparators(PresContext());

  return nsMathMLContainerFrame::ChildListChanged(aModType);
}

void
nsMathMLmfencedFrame::RemoveFencesAndSeparators()
{
  delete mOpenChar;
  delete mCloseChar;
  if (mSeparatorsChar) delete[] mSeparatorsChar;

  mOpenChar = nullptr;
  mCloseChar = nullptr;
  mSeparatorsChar = nullptr;
  mSeparatorsCount = 0;
}

void
nsMathMLmfencedFrame::CreateFencesAndSeparators(nsPresContext* aPresContext)
{
  nsAutoString value;
  bool isMutable = false;

  
  
  if (!GetAttribute(mContent, mPresentationData.mstyle, nsGkAtoms::open,
                    value)) {
    value = PRUnichar('('); 
  } else {
    value.CompressWhitespace();
  }

  if (!value.IsEmpty()) {
    mOpenChar = new nsMathMLChar;
    mOpenChar->SetData(aPresContext, value);
    isMutable = nsMathMLOperators::IsMutableOperator(value);
    ResolveMathMLCharStyle(aPresContext, mContent, mStyleContext, mOpenChar, isMutable);
  }

  
  
  if(!GetAttribute(mContent, mPresentationData.mstyle,
                    nsGkAtoms::close, value)) {
    value = PRUnichar(')'); 
  } else {
    value.CompressWhitespace();
  }

  if (!value.IsEmpty()) {
    mCloseChar = new nsMathMLChar;
    mCloseChar->SetData(aPresContext, value);
    isMutable = nsMathMLOperators::IsMutableOperator(value);
    ResolveMathMLCharStyle(aPresContext, mContent, mStyleContext, mCloseChar, isMutable);
  }

  
  
  if (!GetAttribute(mContent, mPresentationData.mstyle, 
                    nsGkAtoms::separators_, value)) {
    value = PRUnichar(','); 
  } else {
    value.StripWhitespace();
  }

  mSeparatorsCount = value.Length();
  if (0 < mSeparatorsCount) {
    int32_t sepCount = mFrames.GetLength() - 1;
    if (0 < sepCount) {
      mSeparatorsChar = new nsMathMLChar[sepCount];
      nsAutoString sepChar;
      for (int32_t i = 0; i < sepCount; i++) {
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
      mSeparatorsCount = sepCount;
    } else {
      
      
      mSeparatorsCount = 0;
    }
  }
}

void
nsMathMLmfencedFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                       const nsRect&           aDirtyRect,
                                       const nsDisplayListSet& aLists)
{
  
  
  nsMathMLContainerFrame::BuildDisplayList(aBuilder, aDirtyRect, aLists);
  
  
  
  uint32_t count = 0;
  if (mOpenChar) {
    mOpenChar->Display(aBuilder, this, aLists, count++);
  }
  
  if (mCloseChar) {
    mCloseChar->Display(aBuilder, this, aLists, count++);
  }
  
  for (int32_t i = 0; i < mSeparatorsCount; i++) {
    mSeparatorsChar[i].Display(aBuilder, this, aLists, count++);
  }
}

NS_IMETHODIMP
nsMathMLmfencedFrame::Reflow(nsPresContext*          aPresContext,
                             nsHTMLReflowMetrics&     aDesiredSize,
                             const nsHTMLReflowState& aReflowState,
                             nsReflowStatus&          aStatus)
{
  nsresult rv;
  aDesiredSize.width = aDesiredSize.height = 0;
  aDesiredSize.ascent = 0;
  aDesiredSize.mBoundingMetrics = nsBoundingMetrics();

  int32_t i;
  const nsStyleFont* font = StyleFont();
  nsRefPtr<nsFontMetrics> fm;
  nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fm));
  aReflowState.rendContext->SetFont(fm);
  nscoord axisHeight, em;
  GetAxisHeight(*aReflowState.rendContext, fm, axisHeight);
  GetEmHeight(fm, em);
  
  nscoord leading = NSToCoordRound(0.2f * em); 

  
  
  

  
  
  
  
  
  
  

  nsReflowStatus childStatus;
  nsSize availSize(aReflowState.ComputedWidth(), NS_UNCONSTRAINEDSIZE);
  nsIFrame* firstChild = GetFirstPrincipalChild();
  nsIFrame* childFrame = firstChild;
  nscoord ascent = 0, descent = 0;
  if (firstChild || mOpenChar || mCloseChar || mSeparatorsCount > 0) {
    
    
    
    ascent = fm->MaxAscent();
    descent = fm->MaxDescent();
  }
  while (childFrame) {
    nsHTMLReflowMetrics childDesiredSize(aDesiredSize.mFlags
                                         | NS_REFLOW_CALC_BOUNDING_METRICS);
    nsHTMLReflowState childReflowState(aPresContext, aReflowState,
                                       childFrame, availSize);
    rv = ReflowChild(childFrame, aPresContext, childDesiredSize,
                     childReflowState, childStatus);
    
    if (NS_FAILED(rv)) {
      
      DidReflowChildren(firstChild, childFrame);
      return rv;
    }

    SaveReflowAndBoundingMetricsFor(childFrame, childDesiredSize,
                                    childDesiredSize.mBoundingMetrics);

    nscoord childDescent = childDesiredSize.height - childDesiredSize.ascent;
    if (descent < childDescent)
      descent = childDescent;
    if (ascent < childDesiredSize.ascent)
      ascent = childDesiredSize.ascent;

    childFrame = childFrame->GetNextSibling();
  }

  
  

  nsBoundingMetrics containerSize;
  nsStretchDirection stretchDir = NS_STRETCH_DIRECTION_VERTICAL;

  GetPreferredStretchSize(*aReflowState.rendContext,
                          0, 
                          stretchDir, containerSize);
  childFrame = firstChild;
  while (childFrame) {
    nsIMathMLFrame* mathmlChild = do_QueryFrame(childFrame);
    if (mathmlChild) {
      nsHTMLReflowMetrics childDesiredSize;
      
      GetReflowAndBoundingMetricsFor(childFrame, childDesiredSize,
                                     childDesiredSize.mBoundingMetrics);
      
      mathmlChild->Stretch(*aReflowState.rendContext, 
                           stretchDir, containerSize, childDesiredSize);
      
      SaveReflowAndBoundingMetricsFor(childFrame, childDesiredSize,
                                      childDesiredSize.mBoundingMetrics);
      
      nscoord childDescent = childDesiredSize.height - childDesiredSize.ascent;
      if (descent < childDescent)
        descent = childDescent;
      if (ascent < childDesiredSize.ascent)
        ascent = childDesiredSize.ascent;
    }
    childFrame = childFrame->GetNextSibling();
  }

  
  GetPreferredStretchSize(*aReflowState.rendContext,
                          STRETCH_CONSIDER_EMBELLISHMENTS,
                          stretchDir, containerSize);

  
  
  

  
  nscoord delta = std::max(containerSize.ascent - axisHeight, 
                         containerSize.descent + axisHeight);
  containerSize.ascent = delta + axisHeight;
  containerSize.descent = delta - axisHeight;

  bool isRTL = NS_MATHML_IS_RTL(mPresentationData.flags);

  
  
  ReflowChar(aPresContext, *aReflowState.rendContext, mOpenChar,
             NS_MATHML_OPERATOR_FORM_PREFIX, font->mScriptLevel, 
             axisHeight, leading, em, containerSize, ascent, descent, isRTL);
  
  
  for (i = 0; i < mSeparatorsCount; i++) {
    ReflowChar(aPresContext, *aReflowState.rendContext, &mSeparatorsChar[i],
               NS_MATHML_OPERATOR_FORM_INFIX, font->mScriptLevel,
               axisHeight, leading, em, containerSize, ascent, descent, isRTL);
  }
  
  
  ReflowChar(aPresContext, *aReflowState.rendContext, mCloseChar,
             NS_MATHML_OPERATOR_FORM_POSTFIX, font->mScriptLevel,
             axisHeight, leading, em, containerSize, ascent, descent, isRTL);

  
  
  

  i = 0;
  nscoord dx = 0;
  nsBoundingMetrics bm;
  bool firstTime = true;
  nsMathMLChar *leftChar, *rightChar;
  if (isRTL) {
    leftChar = mCloseChar;
    rightChar = mOpenChar;
  } else {
    leftChar = mOpenChar;
    rightChar = mCloseChar;
  }

  if (leftChar) {
    PlaceChar(leftChar, ascent, bm, dx);
    aDesiredSize.mBoundingMetrics = bm;
    firstTime = false;
  }

  if (isRTL) {
    childFrame = this->GetLastChild(nsIFrame::kPrincipalList);
  } else {
    childFrame = firstChild;
  }
  while (childFrame) {
    nsHTMLReflowMetrics childSize;
    GetReflowAndBoundingMetricsFor(childFrame, childSize, bm);
    if (firstTime) {
      firstTime = false;
      aDesiredSize.mBoundingMetrics  = bm;
    }
    else  
      aDesiredSize.mBoundingMetrics += bm;

    FinishReflowChild(childFrame, aPresContext, nullptr, childSize, 
                      dx, ascent - childSize.ascent, 0);
    dx += childSize.width;

    if (i < mSeparatorsCount) {
      PlaceChar(&mSeparatorsChar[isRTL ? mSeparatorsCount - 1 - i : i],
                ascent, bm, dx);
      aDesiredSize.mBoundingMetrics += bm;
    }
    i++;

    if (isRTL) {
      childFrame = childFrame->GetPrevSibling();
    } else {
      childFrame = childFrame->GetNextSibling();
    }
  }

  if (rightChar) {
    PlaceChar(rightChar, ascent, bm, dx);
    if (firstTime)
      aDesiredSize.mBoundingMetrics  = bm;
    else  
      aDesiredSize.mBoundingMetrics += bm;
  }

  aDesiredSize.width = aDesiredSize.mBoundingMetrics.width;
  aDesiredSize.height = ascent + descent;
  aDesiredSize.ascent = ascent;

  SetBoundingMetrics(aDesiredSize.mBoundingMetrics);
  SetReference(nsPoint(0, aDesiredSize.ascent));

  
  FixInterFrameSpacing(aDesiredSize);

  
  ClearSavedChildMetrics();

  
  GatherAndStoreOverflow(&aDesiredSize);

  aStatus = NS_FRAME_COMPLETE;
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  return NS_OK;
}

static void
GetCharSpacing(nsMathMLChar*        aMathMLChar,
               nsOperatorFlags      aForm,
               int32_t              aScriptLevel,
               nscoord              em,
               nscoord&             aLeftSpace,
               nscoord&             aRightSpace)
{
  nsAutoString data;
  aMathMLChar->GetData(data);
  nsOperatorFlags flags = 0;
  float lspace = 0.0f;
  float rspace = 0.0f;
  bool found = nsMathMLOperators::LookupOperator(data, aForm,
                                                   &flags, &lspace, &rspace);

  
  if (found && aScriptLevel > 0) {
    lspace /= 2.0f;
    rspace /= 2.0f;
  }

  aLeftSpace = NSToCoordRound(lspace * em);
  aRightSpace = NSToCoordRound(rspace * em);
}


 nsresult
nsMathMLmfencedFrame::ReflowChar(nsPresContext*      aPresContext,
                                 nsRenderingContext& aRenderingContext,
                                 nsMathMLChar*        aMathMLChar,
                                 nsOperatorFlags      aForm,
                                 int32_t              aScriptLevel,
                                 nscoord              axisHeight,
                                 nscoord              leading,
                                 nscoord              em,
                                 nsBoundingMetrics&   aContainerSize,
                                 nscoord&             aAscent,
                                 nscoord&             aDescent,
                                 bool                 aRTL)
{
  if (aMathMLChar && 0 < aMathMLChar->Length()) {
    nscoord leftSpace;
    nscoord rightSpace;
    GetCharSpacing(aMathMLChar, aForm, aScriptLevel, em, leftSpace, rightSpace);

    
    nsBoundingMetrics charSize;
    nsresult res = aMathMLChar->Stretch(aPresContext, aRenderingContext,
                                        NS_STRETCH_DIRECTION_VERTICAL,
                                        aContainerSize, charSize,
                                        NS_STRETCH_NORMAL, aRTL);

    if (NS_STRETCH_DIRECTION_UNSUPPORTED != aMathMLChar->GetStretchDirection()) {
      
      nscoord height = charSize.ascent + charSize.descent;
      charSize.ascent = height/2 + axisHeight;
      charSize.descent = height - charSize.ascent;
    }
    else {
      
      
      leading = 0;
      if (NS_FAILED(res)) {
        nsAutoString data;
        aMathMLChar->GetData(data);
        nsBoundingMetrics metrics =
          aRenderingContext.GetBoundingMetrics(data.get(), data.Length());
        charSize.ascent = metrics.ascent;
        charSize.descent = metrics.descent;
        charSize.width = metrics.width;
        
        
        aMathMLChar->SetBoundingMetrics(charSize);
      }
    }

    if (aAscent < charSize.ascent + leading) 
      aAscent = charSize.ascent + leading;
    if (aDescent < charSize.descent + leading) 
      aDescent = charSize.descent + leading;

    
    charSize.width += leftSpace + rightSpace;

    
    
    aMathMLChar->SetRect(nsRect(leftSpace, 
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

static nscoord
GetMaxCharWidth(nsPresContext*       aPresContext,
                nsRenderingContext* aRenderingContext,
                nsMathMLChar*        aMathMLChar,
                nsOperatorFlags      aForm,
                int32_t              aScriptLevel,
                nscoord              em)
{
  nscoord width = aMathMLChar->GetMaxWidth(aPresContext, *aRenderingContext);

  if (0 < aMathMLChar->Length()) {
    nscoord leftSpace;
    nscoord rightSpace;
    GetCharSpacing(aMathMLChar, aForm, aScriptLevel, em, leftSpace, rightSpace);

    width += leftSpace + rightSpace;
  }
  
  return width;
}

 nscoord
nsMathMLmfencedFrame::GetIntrinsicWidth(nsRenderingContext* aRenderingContext)
{
  nscoord width = 0;

  nsPresContext* presContext = PresContext();
  const nsStyleFont* font = StyleFont();
  nsRefPtr<nsFontMetrics> fm;
  nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fm));
  nscoord em;
  GetEmHeight(fm, em);

  if (mOpenChar) {
    width +=
      GetMaxCharWidth(presContext, aRenderingContext, mOpenChar,
                      NS_MATHML_OPERATOR_FORM_PREFIX, font->mScriptLevel, em);
  }

  int32_t i = 0;
  nsIFrame* childFrame = GetFirstPrincipalChild();
  while (childFrame) {
    
    
    
    width += nsLayoutUtils::IntrinsicForContainer(aRenderingContext, childFrame,
                                                  nsLayoutUtils::PREF_WIDTH);

    if (i < mSeparatorsCount) {
      width +=
        GetMaxCharWidth(presContext, aRenderingContext, &mSeparatorsChar[i],
                        NS_MATHML_OPERATOR_FORM_INFIX, font->mScriptLevel, em);
    }
    i++;

    childFrame = childFrame->GetNextSibling();
  }

  if (mCloseChar) {
    width +=
      GetMaxCharWidth(presContext, aRenderingContext, mCloseChar,
                      NS_MATHML_OPERATOR_FORM_POSTFIX, font->mScriptLevel, em);
  }

  return width;
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
  for (int32_t i = 0; i < mSeparatorsCount; i++) {
    mSeparatorsChar[i].GetRect(rect);
    rect.MoveBy(gap, 0);
    mSeparatorsChar[i].SetRect(rect);
  }
  return gap;
}



nsStyleContext*
nsMathMLmfencedFrame::GetAdditionalStyleContext(int32_t aIndex) const
{
  int32_t openIndex = -1;
  int32_t closeIndex = -1;
  int32_t lastIndex = mSeparatorsCount-1;

  if (mOpenChar) { 
    lastIndex++; 
    openIndex = lastIndex; 
  }
  if (mCloseChar) { 
    lastIndex++;
    closeIndex = lastIndex;
  }
  if (aIndex < 0 || aIndex > lastIndex) {
    return nullptr;
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
  return nullptr;
}

void
nsMathMLmfencedFrame::SetAdditionalStyleContext(int32_t          aIndex, 
                                                nsStyleContext*  aStyleContext)
{
  int32_t openIndex = -1;
  int32_t closeIndex = -1;
  int32_t lastIndex = mSeparatorsCount-1;

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
