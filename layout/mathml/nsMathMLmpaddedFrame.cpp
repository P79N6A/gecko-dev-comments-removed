








































#include "nsCOMPtr.h"
#include "nsCRT.h"  
#include "nsFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIRenderingContext.h"
#include "nsIFontMetrics.h"

#include "nsMathMLmpaddedFrame.h"





#define NS_MATHML_SIGN_INVALID           -1 // if the attribute is not there
#define NS_MATHML_SIGN_UNSPECIFIED        0
#define NS_MATHML_SIGN_MINUS              1
#define NS_MATHML_SIGN_PLUS               2

#define NS_MATHML_PSEUDO_UNIT_UNSPECIFIED 0
#define NS_MATHML_PSEUDO_UNIT_ITSELF      1 // special
#define NS_MATHML_PSEUDO_UNIT_WIDTH       2
#define NS_MATHML_PSEUDO_UNIT_HEIGHT      3
#define NS_MATHML_PSEUDO_UNIT_DEPTH       4
#define NS_MATHML_PSEUDO_UNIT_LSPACE      5
#define NS_MATHML_PSEUDO_UNIT_NAMEDSPACE  6

nsIFrame*
NS_NewMathMLmpaddedFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLmpaddedFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsMathMLmpaddedFrame)

nsMathMLmpaddedFrame::~nsMathMLmpaddedFrame()
{
}

NS_IMETHODIMP
nsMathMLmpaddedFrame::InheritAutomaticData(nsIFrame* aParent) 
{
  
  nsMathMLContainerFrame::InheritAutomaticData(aParent);

  mPresentationData.flags |= NS_MATHML_STRETCH_ALL_CHILDREN_VERTICALLY;

  return NS_OK;
}

void
nsMathMLmpaddedFrame::ProcessAttributes()
{
  








  nsAutoString value;

  






  

  
  mWidthSign = NS_MATHML_SIGN_INVALID;
  GetAttribute(mContent, nsnull, nsGkAtoms::width, value);
  if (!value.IsEmpty()) {
    ParseAttribute(value, mWidthSign, mWidth, mWidthPseudoUnit);
  }

  
  mHeightSign = NS_MATHML_SIGN_INVALID;
  GetAttribute(mContent, nsnull, nsGkAtoms::height, value);
  if (!value.IsEmpty()) {
    ParseAttribute(value, mHeightSign, mHeight, mHeightPseudoUnit);
  }

  
  mDepthSign = NS_MATHML_SIGN_INVALID;
  GetAttribute(mContent, nsnull, nsGkAtoms::depth_, value);
  if (!value.IsEmpty()) {
    ParseAttribute(value, mDepthSign, mDepth, mDepthPseudoUnit);
  }

  
  mLeftSpaceSign = NS_MATHML_SIGN_INVALID;
  GetAttribute(mContent, nsnull, nsGkAtoms::lspace_, value);
  if (!value.IsEmpty()) {
    ParseAttribute(value, mLeftSpaceSign, mLeftSpace, mLeftSpacePseudoUnit);
  }
}



PRBool
nsMathMLmpaddedFrame::ParseAttribute(nsString&   aString,
                                     PRInt32&    aSign,
                                     nsCSSValue& aCSSValue,
                                     PRInt32&    aPseudoUnit)
{
  aCSSValue.Reset();
  aSign = NS_MATHML_SIGN_INVALID;
  aPseudoUnit = NS_MATHML_PSEUDO_UNIT_UNSPECIFIED;
  aString.CompressWhitespace(); 

  PRInt32 stringLength = aString.Length();
  if (!stringLength)
    return PR_FALSE;

  nsAutoString number, unit;

  
  

  PRInt32 i = 0;

  if (aString[0] == '+') {
    aSign = NS_MATHML_SIGN_PLUS;
    i++;
  }
  else if (aString[0] == '-') {
    aSign = NS_MATHML_SIGN_MINUS;
    i++;
  }
  else
    aSign = NS_MATHML_SIGN_UNSPECIFIED;

  
  if (i < stringLength && nsCRT::IsAsciiSpace(aString[i]))
    i++;

  
  PRBool gotDot = PR_FALSE, gotPercent = PR_FALSE;
  for (; i < stringLength; i++) {
    PRUnichar c = aString[i];
    if (gotDot && c == '.') {
      
      aSign = NS_MATHML_SIGN_INVALID;
      return PR_FALSE;
    }

    if (c == '.')
      gotDot = PR_TRUE;
    else if (!nsCRT::IsAsciiDigit(c)) {
      break;
    }
    number.Append(c);
  }

  
  
  
  if (number.IsEmpty()) {
#ifdef NS_DEBUG
    printf("mpadded: attribute with bad numeric value: %s\n",
            NS_LossyConvertUTF16toASCII(aString).get());
#endif
    aSign = NS_MATHML_SIGN_INVALID;
    return PR_FALSE;
  }

  PRInt32 errorCode;
  float floatValue = number.ToFloat(&errorCode);
  if (errorCode) {
    aSign = NS_MATHML_SIGN_INVALID;
    return PR_FALSE;
  }

  
  if (i < stringLength && nsCRT::IsAsciiSpace(aString[i]))
    i++;

  
  if (i < stringLength && aString[i] == '%') {
    i++;
    gotPercent = PR_TRUE;

    
    if (i < stringLength && nsCRT::IsAsciiSpace(aString[i]))
      i++;
  }

  
  aString.Right(unit, stringLength - i);

  if (unit.IsEmpty()) {
    
    if (gotPercent || !floatValue) {
      aCSSValue.SetPercentValue(floatValue / 100.0f);
      aPseudoUnit = NS_MATHML_PSEUDO_UNIT_ITSELF;
      return PR_TRUE;
    }
    









  }
  else if (unit.EqualsLiteral("width"))  aPseudoUnit = NS_MATHML_PSEUDO_UNIT_WIDTH;
  else if (unit.EqualsLiteral("height")) aPseudoUnit = NS_MATHML_PSEUDO_UNIT_HEIGHT;
  else if (unit.EqualsLiteral("depth"))  aPseudoUnit = NS_MATHML_PSEUDO_UNIT_DEPTH;
  else if (unit.EqualsLiteral("lspace")) aPseudoUnit = NS_MATHML_PSEUDO_UNIT_LSPACE;
  else if (!gotPercent) { 

    
    
    if (ParseNamedSpaceValue(nsnull, unit, aCSSValue)) {
      
      floatValue *= aCSSValue.GetFloatValue();
      aCSSValue.SetFloatValue(floatValue, eCSSUnit_EM);
      aPseudoUnit = NS_MATHML_PSEUDO_UNIT_NAMEDSPACE;
      return PR_TRUE;
    }

    
    number.Append(unit); 
    if (ParseNumericValue(number, aCSSValue))
      return PR_TRUE;
  }

  
  if (aPseudoUnit != NS_MATHML_PSEUDO_UNIT_UNSPECIFIED) {
    if (gotPercent)
      aCSSValue.SetPercentValue(floatValue / 100.0f);
    else
      aCSSValue.SetFloatValue(floatValue, eCSSUnit_Number);

    return PR_TRUE;
  }


#ifdef NS_DEBUG
  printf("mpadded: attribute with bad numeric value: %s\n",
          NS_LossyConvertUTF16toASCII(aString).get());
#endif
  
  aSign = NS_MATHML_SIGN_INVALID;
  return PR_FALSE;
}

void
nsMathMLmpaddedFrame::UpdateValue(PRInt32                  aSign,
                                  PRInt32                  aPseudoUnit,
                                  const nsCSSValue&        aCSSValue,
                                  nscoord                  aLeftSpace,
                                  const nsBoundingMetrics& aBoundingMetrics,
                                  nscoord&                 aValueToUpdate) const
{
  nsCSSUnit unit = aCSSValue.GetUnit();
  if (NS_MATHML_SIGN_INVALID != aSign && eCSSUnit_Null != unit) {
    nscoord scaler = 0, amount = 0;

    if (eCSSUnit_Percent == unit || eCSSUnit_Number == unit) {
      switch(aPseudoUnit) {
        case NS_MATHML_PSEUDO_UNIT_WIDTH:
             scaler = aBoundingMetrics.width;
             break;

        case NS_MATHML_PSEUDO_UNIT_HEIGHT:
             scaler = aBoundingMetrics.ascent;
             break;

        case NS_MATHML_PSEUDO_UNIT_DEPTH:
             scaler = aBoundingMetrics.descent;
             break;

        case NS_MATHML_PSEUDO_UNIT_LSPACE:
             scaler = aLeftSpace;
             break;

        default:
          
          
          NS_ERROR("Unexpected Pseudo Unit");
          return;
      }
    }

    if (eCSSUnit_Number == unit)
      amount = NSToCoordRound(float(scaler) * aCSSValue.GetFloatValue());
    else if (eCSSUnit_Percent == unit)
      amount = NSToCoordRound(float(scaler) * aCSSValue.GetPercentValue());
    else
      amount = CalcLength(PresContext(), mStyleContext, aCSSValue);

    nscoord oldValue = aValueToUpdate;
    if (NS_MATHML_SIGN_PLUS == aSign)
      aValueToUpdate += amount;
    else if (NS_MATHML_SIGN_MINUS == aSign)
      aValueToUpdate -= amount;
    else
      aValueToUpdate  = amount;

    





    if (0 < oldValue && 0 > aValueToUpdate)
      aValueToUpdate = 0;
  }
}

NS_IMETHODIMP
nsMathMLmpaddedFrame::Reflow(nsPresContext*          aPresContext,
                             nsHTMLReflowMetrics&     aDesiredSize,
                             const nsHTMLReflowState& aReflowState,
                             nsReflowStatus&          aStatus)
{
  ProcessAttributes();

  
  
  nsresult rv = nsMathMLContainerFrame::Reflow(aPresContext, aDesiredSize,
                                               aReflowState, aStatus);
  
  return rv;
}

 nsresult
nsMathMLmpaddedFrame::Place(nsIRenderingContext& aRenderingContext,
                            PRBool               aPlaceOrigin,
                            nsHTMLReflowMetrics& aDesiredSize)
{
  nsresult rv =
    nsMathMLContainerFrame::Place(aRenderingContext, PR_FALSE, aDesiredSize);
  if (NS_MATHML_HAS_ERROR(mPresentationData.flags) || NS_FAILED(rv)) {
    DidReflowChildren(GetFirstChild(nsnull));
    return rv;
  }

  nscoord height = mBoundingMetrics.ascent;
  nscoord depth  = mBoundingMetrics.descent;
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  nscoord lspace = 0;
  
  
  
  
  nscoord width  = mBoundingMetrics.width;

  PRInt32 pseudoUnit;

  
  pseudoUnit = (mWidthPseudoUnit == NS_MATHML_PSEUDO_UNIT_ITSELF)
             ? NS_MATHML_PSEUDO_UNIT_WIDTH : mWidthPseudoUnit;
  UpdateValue(mWidthSign, pseudoUnit, mWidth,
              lspace, mBoundingMetrics, width);

  
  pseudoUnit = (mHeightPseudoUnit == NS_MATHML_PSEUDO_UNIT_ITSELF)
             ? NS_MATHML_PSEUDO_UNIT_HEIGHT : mHeightPseudoUnit;
  UpdateValue(mHeightSign, pseudoUnit, mHeight,
              lspace, mBoundingMetrics, height);

  
  pseudoUnit = (mDepthPseudoUnit == NS_MATHML_PSEUDO_UNIT_ITSELF)
             ? NS_MATHML_PSEUDO_UNIT_DEPTH : mDepthPseudoUnit;
  UpdateValue(mDepthSign, pseudoUnit, mDepth,
              lspace, mBoundingMetrics, depth);

  
  pseudoUnit = (mLeftSpacePseudoUnit == NS_MATHML_PSEUDO_UNIT_ITSELF)
             ? NS_MATHML_PSEUDO_UNIT_LSPACE : mLeftSpacePseudoUnit;
  UpdateValue(mLeftSpaceSign, pseudoUnit, mLeftSpace,
              lspace, mBoundingMetrics, lspace);

  
  
  
  
  

  if (mLeftSpaceSign != NS_MATHML_SIGN_INVALID) { 
    
    mBoundingMetrics.leftBearing = 0;
  }

  if (mLeftSpaceSign != NS_MATHML_SIGN_INVALID ||
      mWidthSign != NS_MATHML_SIGN_INVALID) { 
    
    mBoundingMetrics.width = NS_MAX(0, lspace + width);
    mBoundingMetrics.rightBearing = mBoundingMetrics.width;
  }

  nscoord dy = height - mBoundingMetrics.ascent;
  nscoord dx = lspace;

  mBoundingMetrics.ascent = height;
  mBoundingMetrics.descent = depth;

  aDesiredSize.ascent += dy;
  aDesiredSize.width = mBoundingMetrics.width;
  aDesiredSize.height += dy + depth - mBoundingMetrics.descent;
  aDesiredSize.mBoundingMetrics = mBoundingMetrics;

  mReference.x = 0;
  mReference.y = aDesiredSize.ascent;

  if (aPlaceOrigin) {
    
    PositionRowChildFrames(dx, aDesiredSize.ascent);
  }

  return NS_OK;
}
