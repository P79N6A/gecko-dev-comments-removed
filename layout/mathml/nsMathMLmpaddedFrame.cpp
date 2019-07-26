





#include "nsCOMPtr.h"
#include "nsCRT.h"  
#include "nsFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"

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
#define NS_MATHML_PSEUDO_UNIT_NAMEDSPACE  5

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
  GetAttribute(mContent, nullptr, nsGkAtoms::width, value);
  if (!value.IsEmpty()) {
    ParseAttribute(value, mWidthSign, mWidth, mWidthPseudoUnit);
  }

  
  mHeightSign = NS_MATHML_SIGN_INVALID;
  GetAttribute(mContent, nullptr, nsGkAtoms::height, value);
  if (!value.IsEmpty()) {
    ParseAttribute(value, mHeightSign, mHeight, mHeightPseudoUnit);
  }

  
  mDepthSign = NS_MATHML_SIGN_INVALID;
  GetAttribute(mContent, nullptr, nsGkAtoms::depth_, value);
  if (!value.IsEmpty()) {
    ParseAttribute(value, mDepthSign, mDepth, mDepthPseudoUnit);
  }

  
  mLeadingSpaceSign = NS_MATHML_SIGN_INVALID;
  GetAttribute(mContent, nullptr, nsGkAtoms::lspace_, value);
  if (!value.IsEmpty()) {
    ParseAttribute(value, mLeadingSpaceSign, mLeadingSpace,
                   mLeadingSpacePseudoUnit);
  }

  
  mVerticalOffsetSign = NS_MATHML_SIGN_INVALID;
  GetAttribute(mContent, nullptr, nsGkAtoms::voffset_, value);
  if (!value.IsEmpty()) {
    ParseAttribute(value, mVerticalOffsetSign, mVerticalOffset, 
                   mVerticalOffsetPseudoUnit);
  }
  
}



bool
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
    return false;

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

  
  bool gotDot = false, gotPercent = false;
  for (; i < stringLength; i++) {
    PRUnichar c = aString[i];
    if (gotDot && c == '.') {
      
      aSign = NS_MATHML_SIGN_INVALID;
      return false;
    }

    if (c == '.')
      gotDot = true;
    else if (!nsCRT::IsAsciiDigit(c)) {
      break;
    }
    number.Append(c);
  }

  
  
  
  if (number.IsEmpty()) {
#ifdef DEBUG
    printf("mpadded: attribute with bad numeric value: %s\n",
            NS_LossyConvertUTF16toASCII(aString).get());
#endif
    aSign = NS_MATHML_SIGN_INVALID;
    return false;
  }

  nsresult errorCode;
  float floatValue = number.ToFloat(&errorCode);
  if (NS_FAILED(errorCode)) {
    aSign = NS_MATHML_SIGN_INVALID;
    return false;
  }

  
  if (i < stringLength && aString[i] == '%') {
    i++;
    gotPercent = true;
  }

  
  aString.Right(unit, stringLength - i);

  if (unit.IsEmpty()) {
    if (gotPercent) {
      
      aCSSValue.SetPercentValue(floatValue / 100.0f);
      aPseudoUnit = NS_MATHML_PSEUDO_UNIT_ITSELF;
      return true;
    } else {
      
      
      if (!floatValue) {
        aCSSValue.SetFloatValue(floatValue, eCSSUnit_Number);
        aPseudoUnit = NS_MATHML_PSEUDO_UNIT_ITSELF;
        return true;
      }
    }
  }
  else if (unit.EqualsLiteral("width"))  aPseudoUnit = NS_MATHML_PSEUDO_UNIT_WIDTH;
  else if (unit.EqualsLiteral("height")) aPseudoUnit = NS_MATHML_PSEUDO_UNIT_HEIGHT;
  else if (unit.EqualsLiteral("depth"))  aPseudoUnit = NS_MATHML_PSEUDO_UNIT_DEPTH;
  else if (!gotPercent) { 

    
    if (nsMathMLElement::ParseNamedSpaceValue(unit, aCSSValue,
                                              nsMathMLElement::
                                              PARSE_ALLOW_NEGATIVE)) {
      
      floatValue *= aCSSValue.GetFloatValue();
      aCSSValue.SetFloatValue(floatValue, eCSSUnit_EM);
      aPseudoUnit = NS_MATHML_PSEUDO_UNIT_NAMEDSPACE;
      return true;
    }

    
    
    
    number.Append(unit); 
    if (nsMathMLElement::ParseNumericValue(number, aCSSValue, 0))
      return true;
  }

  
  if (aPseudoUnit != NS_MATHML_PSEUDO_UNIT_UNSPECIFIED) {
    if (gotPercent)
      aCSSValue.SetPercentValue(floatValue / 100.0f);
    else
      aCSSValue.SetFloatValue(floatValue, eCSSUnit_Number);

    return true;
  }


#ifdef DEBUG
  printf("mpadded: attribute with bad numeric value: %s\n",
          NS_LossyConvertUTF16toASCII(aString).get());
#endif
  
  aSign = NS_MATHML_SIGN_INVALID;
  return false;
}

void
nsMathMLmpaddedFrame::UpdateValue(PRInt32                  aSign,
                                  PRInt32                  aPseudoUnit,
                                  const nsCSSValue&        aCSSValue,
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

    if (NS_MATHML_SIGN_PLUS == aSign)
      aValueToUpdate += amount;
    else if (NS_MATHML_SIGN_MINUS == aSign)
      aValueToUpdate -= amount;
    else
      aValueToUpdate  = amount;
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
nsMathMLmpaddedFrame::Place(nsRenderingContext& aRenderingContext,
                            bool                 aPlaceOrigin,
                            nsHTMLReflowMetrics& aDesiredSize)
{
  nsresult rv =
    nsMathMLContainerFrame::Place(aRenderingContext, false, aDesiredSize);
  if (NS_MATHML_HAS_ERROR(mPresentationData.flags) || NS_FAILED(rv)) {
    DidReflowChildren(GetFirstPrincipalChild());
    return rv;
  }

  nscoord height = mBoundingMetrics.ascent;
  nscoord depth  = mBoundingMetrics.descent;
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  nscoord lspace = 0;
  
  
  
  
  nscoord width  = mBoundingMetrics.width;
  nscoord voffset = 0;

  PRInt32 pseudoUnit;
  nscoord initialWidth = width;

  
  pseudoUnit = (mWidthPseudoUnit == NS_MATHML_PSEUDO_UNIT_ITSELF)
             ? NS_MATHML_PSEUDO_UNIT_WIDTH : mWidthPseudoUnit;
  UpdateValue(mWidthSign, pseudoUnit, mWidth,
              mBoundingMetrics, width);
  width = NS_MAX(0, width);

  
  pseudoUnit = (mHeightPseudoUnit == NS_MATHML_PSEUDO_UNIT_ITSELF)
             ? NS_MATHML_PSEUDO_UNIT_HEIGHT : mHeightPseudoUnit;
  UpdateValue(mHeightSign, pseudoUnit, mHeight,
              mBoundingMetrics, height);
  height = NS_MAX(0, height);

  
  pseudoUnit = (mDepthPseudoUnit == NS_MATHML_PSEUDO_UNIT_ITSELF)
             ? NS_MATHML_PSEUDO_UNIT_DEPTH : mDepthPseudoUnit;
  UpdateValue(mDepthSign, pseudoUnit, mDepth,
              mBoundingMetrics, depth);
  depth = NS_MAX(0, depth);

  
  if (mLeadingSpacePseudoUnit != NS_MATHML_PSEUDO_UNIT_ITSELF) {
    pseudoUnit = mLeadingSpacePseudoUnit;
    UpdateValue(mLeadingSpaceSign, pseudoUnit, mLeadingSpace,
                mBoundingMetrics, lspace);
  }

  
  if (mVerticalOffsetPseudoUnit != NS_MATHML_PSEUDO_UNIT_ITSELF) {
    pseudoUnit = mVerticalOffsetPseudoUnit;
    UpdateValue(mVerticalOffsetSign, pseudoUnit, mVerticalOffset,
                mBoundingMetrics, voffset);
  }
  
  
  
  
  

  if ((NS_MATHML_IS_RTL(mPresentationData.flags) ?
       mWidthSign : mLeadingSpaceSign) != NS_MATHML_SIGN_INVALID) {
    
    
    mBoundingMetrics.leftBearing = 0;
  }

  if ((NS_MATHML_IS_RTL(mPresentationData.flags) ?
       mLeadingSpaceSign : mWidthSign) != NS_MATHML_SIGN_INVALID) {
    
    
    mBoundingMetrics.width = width;
    mBoundingMetrics.rightBearing = mBoundingMetrics.width;
  }

  nscoord dy = height - mBoundingMetrics.ascent;
  nscoord dx = NS_MATHML_IS_RTL(mPresentationData.flags) ?
    width - initialWidth - lspace : lspace;
    
  aDesiredSize.ascent += dy;
  aDesiredSize.width = mBoundingMetrics.width;
  aDesiredSize.height += dy + depth - mBoundingMetrics.descent;
  mBoundingMetrics.ascent = height;
  mBoundingMetrics.descent = depth;
  aDesiredSize.mBoundingMetrics = mBoundingMetrics;

  mReference.x = 0;
  mReference.y = aDesiredSize.ascent;

  if (aPlaceOrigin) {
    
    PositionRowChildFrames(dx, aDesiredSize.ascent - voffset);
  }

  return NS_OK;
}
