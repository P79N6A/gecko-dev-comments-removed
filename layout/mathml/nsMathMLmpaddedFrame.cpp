





#include "nsMathMLmpaddedFrame.h"
#include "nsMathMLElement.h"
#include "mozilla/gfx/2D.h"
#include <algorithm>





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
    if (!ParseAttribute(value, mWidthSign, mWidth, mWidthPseudoUnit)) {      
      ReportParseError(nsGkAtoms::width->GetUTF16String(), value.get());
    }
  }

  
  mHeightSign = NS_MATHML_SIGN_INVALID;
  GetAttribute(mContent, nullptr, nsGkAtoms::height, value);
  if (!value.IsEmpty()) {
    if (!ParseAttribute(value, mHeightSign, mHeight, mHeightPseudoUnit)) {
      ReportParseError(nsGkAtoms::height->GetUTF16String(), value.get());
    }
  }

  
  mDepthSign = NS_MATHML_SIGN_INVALID;
  GetAttribute(mContent, nullptr, nsGkAtoms::depth_, value);
  if (!value.IsEmpty()) {
    if (!ParseAttribute(value, mDepthSign, mDepth, mDepthPseudoUnit)) {
      ReportParseError(nsGkAtoms::depth_->GetUTF16String(), value.get());
    }
  }

  
  mLeadingSpaceSign = NS_MATHML_SIGN_INVALID;
  GetAttribute(mContent, nullptr, nsGkAtoms::lspace_, value);
  if (!value.IsEmpty()) {
    if (!ParseAttribute(value, mLeadingSpaceSign, mLeadingSpace, 
                        mLeadingSpacePseudoUnit)) {
      ReportParseError(nsGkAtoms::lspace_->GetUTF16String(), value.get());
    }
  }

  
  mVerticalOffsetSign = NS_MATHML_SIGN_INVALID;
  GetAttribute(mContent, nullptr, nsGkAtoms::voffset_, value);
  if (!value.IsEmpty()) {
    if (!ParseAttribute(value, mVerticalOffsetSign, mVerticalOffset,
                        mVerticalOffsetPseudoUnit)) {
      ReportParseError(nsGkAtoms::voffset_->GetUTF16String(), value.get());
    }
  }
  
}



bool
nsMathMLmpaddedFrame::ParseAttribute(nsString&   aString,
                                     int32_t&    aSign,
                                     nsCSSValue& aCSSValue,
                                     int32_t&    aPseudoUnit)
{
  aCSSValue.Reset();
  aSign = NS_MATHML_SIGN_INVALID;
  aPseudoUnit = NS_MATHML_PSEUDO_UNIT_UNSPECIFIED;
  aString.CompressWhitespace(); 

  int32_t stringLength = aString.Length();
  if (!stringLength)
    return false;

  nsAutoString number, unit;

  
  

  int32_t i = 0;

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
    char16_t c = aString[i];
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
    if (nsMathMLElement::ParseNumericValue(number, aCSSValue, 
                                           nsMathMLElement::
                                           PARSE_SUPPRESS_WARNINGS, nullptr))
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
nsMathMLmpaddedFrame::UpdateValue(int32_t                  aSign,
                                  int32_t                  aPseudoUnit,
                                  const nsCSSValue&        aCSSValue,
                                  const nsHTMLReflowMetrics& aDesiredSize,
                                  nscoord&                 aValueToUpdate) const
{
  nsCSSUnit unit = aCSSValue.GetUnit();
  if (NS_MATHML_SIGN_INVALID != aSign && eCSSUnit_Null != unit) {
    nscoord scaler = 0, amount = 0;

    if (eCSSUnit_Percent == unit || eCSSUnit_Number == unit) {
      switch(aPseudoUnit) {
        case NS_MATHML_PSEUDO_UNIT_WIDTH:
             scaler = aDesiredSize.Width();
             break;

        case NS_MATHML_PSEUDO_UNIT_HEIGHT:
             scaler = aDesiredSize.TopAscent();
             break;

        case NS_MATHML_PSEUDO_UNIT_DEPTH:
             scaler = aDesiredSize.Height() - aDesiredSize.TopAscent();
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

  nscoord height = aDesiredSize.TopAscent();
  nscoord depth  = aDesiredSize.Height() - aDesiredSize.TopAscent();
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  nscoord lspace = 0;
  
  
  
  
  nscoord width  = aDesiredSize.Width();
  nscoord voffset = 0;

  int32_t pseudoUnit;
  nscoord initialWidth = width;

  
  pseudoUnit = (mWidthPseudoUnit == NS_MATHML_PSEUDO_UNIT_ITSELF)
             ? NS_MATHML_PSEUDO_UNIT_WIDTH : mWidthPseudoUnit;
  UpdateValue(mWidthSign, pseudoUnit, mWidth,
              aDesiredSize, width);
  width = std::max(0, width);

  
  pseudoUnit = (mHeightPseudoUnit == NS_MATHML_PSEUDO_UNIT_ITSELF)
             ? NS_MATHML_PSEUDO_UNIT_HEIGHT : mHeightPseudoUnit;
  UpdateValue(mHeightSign, pseudoUnit, mHeight,
              aDesiredSize, height);
  height = std::max(0, height);

  
  pseudoUnit = (mDepthPseudoUnit == NS_MATHML_PSEUDO_UNIT_ITSELF)
             ? NS_MATHML_PSEUDO_UNIT_DEPTH : mDepthPseudoUnit;
  UpdateValue(mDepthSign, pseudoUnit, mDepth,
              aDesiredSize, depth);
  depth = std::max(0, depth);

  
  if (mLeadingSpacePseudoUnit != NS_MATHML_PSEUDO_UNIT_ITSELF) {
    pseudoUnit = mLeadingSpacePseudoUnit;
    UpdateValue(mLeadingSpaceSign, pseudoUnit, mLeadingSpace,
                aDesiredSize, lspace);
  }

  
  if (mVerticalOffsetPseudoUnit != NS_MATHML_PSEUDO_UNIT_ITSELF) {
    pseudoUnit = mVerticalOffsetPseudoUnit;
    UpdateValue(mVerticalOffsetSign, pseudoUnit, mVerticalOffset,
                aDesiredSize, voffset);
  }
  
  
  
  
  

  if ((StyleVisibility()->mDirection ?
       mWidthSign : mLeadingSpaceSign) != NS_MATHML_SIGN_INVALID) {
    
    
    mBoundingMetrics.leftBearing = 0;
  }

  if ((StyleVisibility()->mDirection ?
       mLeadingSpaceSign : mWidthSign) != NS_MATHML_SIGN_INVALID) {
    
    
    mBoundingMetrics.width = width;
    mBoundingMetrics.rightBearing = mBoundingMetrics.width;
  }

  nscoord dx = (StyleVisibility()->mDirection ?
                width - initialWidth - lspace : lspace);
    
  aDesiredSize.SetTopAscent(height);
  aDesiredSize.Width() = mBoundingMetrics.width;
  aDesiredSize.Height() = depth + aDesiredSize.TopAscent();
  mBoundingMetrics.ascent = height;
  mBoundingMetrics.descent = depth;
  aDesiredSize.mBoundingMetrics = mBoundingMetrics;

  mReference.x = 0;
  mReference.y = aDesiredSize.TopAscent();

  if (aPlaceOrigin) {
    
    PositionRowChildFrames(dx, aDesiredSize.TopAscent() - voffset);
  }

  return NS_OK;
}

 nsresult
nsMathMLmpaddedFrame::MeasureForWidth(nsRenderingContext& aRenderingContext,
                                      nsHTMLReflowMetrics& aDesiredSize)
{
  ProcessAttributes();
  return Place(aRenderingContext, false, aDesiredSize);
}
