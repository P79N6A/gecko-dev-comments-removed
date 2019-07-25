







































#include "nsStyleTransformMatrix.h"
#include "nsAutoPtr.h"
#include "nsCSSValue.h"
#include "nsStyleContext.h"
#include "nsPresContext.h"
#include "nsRuleNode.h"
#include "nsCSSKeywords.h"
#include "nsMathUtils.h"
#include "CSSCalc.h"
#include "nsCSSStruct.h"

namespace css = mozilla::css;










static double FlushToZero(double aVal)
{
  if (-FLT_EPSILON < aVal && aVal < FLT_EPSILON)
    return 0.0f;
  else
    return aVal;
}





static double SafeTangent(double aTheta)
{
  const double kEpsilon = 0.0001;

  



  double sinTheta = sin(aTheta);
  double cosTheta = cos(aTheta);

  if (cosTheta >= 0 && cosTheta < kEpsilon)
    cosTheta = kEpsilon;
  else if (cosTheta < 0 && cosTheta >= -kEpsilon)
    cosTheta = -kEpsilon;

  return FlushToZero(sinTheta / cosTheta);
}


nsStyleTransformMatrix::nsStyleTransformMatrix()
{
  SetToIdentity();
}


void nsStyleTransformMatrix::SetToIdentity()
{
    
  mMain[0] = 1.0f;
  mMain[1] = 0.0f;
  mMain[2] = 0.0f;
  mMain[3] = 1.0f;
  mDelta[0] = 0;
  mDelta[1] = 0;

  
  mX[0] = 0.0f;
  mX[1] = 0.0f;
  mY[0] = 0.0f;
  mY[1] = 0.0f;
}


nscoord nsStyleTransformMatrix::GetXTranslation(const nsRect& aBounds) const
{
  return NSToCoordRound(aBounds.width * mX[0] + aBounds.height * mY[0]) +
    mDelta[0];
}
nscoord nsStyleTransformMatrix::GetYTranslation(const nsRect& aBounds) const
{
  return NSToCoordRound(aBounds.width * mX[1] + aBounds.height * mY[1]) +
    mDelta[1];
}


gfxMatrix nsStyleTransformMatrix::GetThebesMatrix(const nsRect& aBounds,
                                                  float aScale) const
{
  










  return gfxMatrix(mMain[0], mMain[1], mMain[2], mMain[3],
                   NSAppUnitsToFloatPixels(GetXTranslation(aBounds), aScale),
                   NSAppUnitsToFloatPixels(GetYTranslation(aBounds), aScale));
}




nsStyleTransformMatrix&
nsStyleTransformMatrix::operator *= (const nsStyleTransformMatrix &aOther)
{
  



  float newMatrix[4];
  nscoord newDelta[2];
  float newX[2];
  float newY[2];
  
  




  newMatrix[0] = aOther.mMain[0] * mMain[0] + aOther.mMain[1] * mMain[2];
  newMatrix[1] = aOther.mMain[0] * mMain[1] + aOther.mMain[1] * mMain[3];
  newMatrix[2] = aOther.mMain[2] * mMain[0] + aOther.mMain[3] * mMain[2];
  newMatrix[3] = aOther.mMain[2] * mMain[1] + aOther.mMain[3] * mMain[3];
  newDelta[0] = NSToCoordRound(aOther.mDelta[0] * mMain[0] +
                               aOther.mDelta[1] * mMain[2]) + mDelta[0];
  newDelta[1] = NSToCoordRound(aOther.mDelta[0] * mMain[1] +
                               aOther.mDelta[1] * mMain[3]) + mDelta[1];

  









  newX[0] = mMain[0] * aOther.mX[0] + mMain[2] * aOther.mX[1] + mX[0];
  newX[1] = mMain[1] * aOther.mX[0] + mMain[3] * aOther.mX[1] + mX[1];
  newY[0] = mMain[0] * aOther.mY[0] + mMain[2] * aOther.mY[1] + mY[0];
  newY[1] = mMain[1] * aOther.mY[0] + mMain[3] * aOther.mY[1] + mY[1];

  
  for (PRInt32 index = 0; index < 4; ++index)
    mMain[index] = newMatrix[index];
  for (PRInt32 index = 0; index < 2; ++index) {
    mDelta[index] = newDelta[index];
    mX[index] = newX[index];
    mY[index] = newY[index];
  }

  
  return *this;
}


const nsStyleTransformMatrix
nsStyleTransformMatrix::operator *(const nsStyleTransformMatrix &aOther) const
{
  return nsStyleTransformMatrix(*this) *= aOther;
}


static nscoord CalcLength(const nsCSSValue &aValue,
                          nsStyleContext* aContext,
                          nsPresContext* aPresContext,
                          PRBool &aCanStoreInRuleTree)
{
  if (aValue.GetUnit() == eCSSUnit_Pixel) {
    
    
    
    
    
    return nsPresContext::CSSPixelsToAppUnits(aValue.GetFloatValue());
  }
  return nsRuleNode::CalcLength(aValue, aContext, aPresContext,
                                aCanStoreInRuleTree);
}


static void ProcessMatrix(float aMain[4], nscoord aDelta[2],
                          float aX[2], float aY[2],
                          const nsCSSValue::Array* aData,
                          nsStyleContext* aContext,
                          nsPresContext* aPresContext,
                          PRBool& aCanStoreInRuleTree)
{
  NS_PRECONDITION(aData->Count() == 7, "Invalid array!");

  


  for (PRUint16 index = 1; index <= 4; ++index)
    aMain[index - 1] = aData->Item(index).GetFloatValue();

  


  if (aData->Item(5).GetUnit() == eCSSUnit_Percent)
    aX[0] = aData->Item(5).GetPercentValue();
  else
    aDelta[0] = CalcLength(aData->Item(5), aContext, aPresContext,
                           aCanStoreInRuleTree);

  


  if (aData->Item(6).GetUnit() == eCSSUnit_Percent)
    aY[1] = aData->Item(6).GetPercentValue();
  else
    aDelta[1] = CalcLength(aData->Item(6), aContext, aPresContext,
                           aCanStoreInRuleTree);
}

struct LengthPercentPairCalcOps : public css::NumbersAlreadyNormalizedOps
{
  struct result_type {
    nscoord mLength;
    float mPercent;

    result_type(nscoord aLength, float aPercent)
      : mLength(aLength), mPercent(aPercent) {}
  };

  LengthPercentPairCalcOps(nsStyleContext* aContext,
                           nsPresContext* aPresContext,
                           PRBool& aCanStoreInRuleTree)
    : mContext(aContext),
      mPresContext(aPresContext),
      mCanStoreInRuleTree(aCanStoreInRuleTree) {}

  nsStyleContext* mContext;
  nsPresContext* mPresContext;
  PRBool& mCanStoreInRuleTree;

  result_type ComputeLeafValue(const nsCSSValue& aValue)
  {
    if (aValue.GetUnit() == eCSSUnit_Percent) {
      return result_type(0, aValue.GetPercentValue());
    } else {
      return result_type(CalcLength(aValue, mContext, mPresContext,
                                    mCanStoreInRuleTree),
                         0.0f);
    }
  }

  result_type
  MergeAdditive(nsCSSUnit aCalcFunction,
                result_type aValue1, result_type aValue2)
  {
    if (aCalcFunction == eCSSUnit_Calc_Plus) {
      return result_type(NSCoordSaturatingAdd(aValue1.mLength,
                                              aValue2.mLength),
                         aValue1.mPercent + aValue2.mPercent);
    }
    NS_ABORT_IF_FALSE(aCalcFunction == eCSSUnit_Calc_Minus,
                      "min() and max() are not allowed in calc() on "
                      "transform");
    return result_type(NSCoordSaturatingSubtract(aValue1.mLength,
                                                 aValue2.mLength, 0),
                       aValue1.mPercent - aValue2.mPercent);
  }

  result_type
  MergeMultiplicativeL(nsCSSUnit aCalcFunction,
                       float aValue1, result_type aValue2)
  {
    NS_ABORT_IF_FALSE(aCalcFunction == eCSSUnit_Calc_Times_L,
                      "unexpected unit");
    return result_type(NSCoordSaturatingMultiply(aValue2.mLength, aValue1),
                       aValue1 * aValue2.mPercent);
  }

  result_type
  MergeMultiplicativeR(nsCSSUnit aCalcFunction,
                       result_type aValue1, float aValue2)
  {
    NS_ABORT_IF_FALSE(aCalcFunction == eCSSUnit_Calc_Times_R ||
                      aCalcFunction == eCSSUnit_Calc_Divided,
                      "unexpected unit");
    if (aCalcFunction == eCSSUnit_Calc_Divided) {
      aValue2 = 1.0f / aValue2;
    }
    return result_type(NSCoordSaturatingMultiply(aValue1.mLength, aValue2),
                       aValue1.mPercent * aValue2);
  }

};

static void ProcessTranslatePart(nscoord& aOffset, float& aPercent,
                                 const nsCSSValue& aValue,
                                 nsStyleContext* aContext,
                                 nsPresContext* aPresContext,
                                 PRBool& aCanStoreInRuleTree)
{
  if (aValue.GetUnit() == eCSSUnit_Percent) {
    aPercent = aValue.GetPercentValue();
  } else if (aValue.IsCalcUnit()) {
    LengthPercentPairCalcOps ops(aContext, aPresContext, aCanStoreInRuleTree);
    LengthPercentPairCalcOps::result_type result =
      css::ComputeCalc(aValue, ops);
    aPercent = result.mPercent;
    aOffset = result.mLength;
  } else {
    aOffset = CalcLength(aValue, aContext, aPresContext,
                         aCanStoreInRuleTree);
  }
}


static void ProcessTranslateX(nscoord aDelta[2], float aX[2],
                              const nsCSSValue::Array* aData,
                              nsStyleContext* aContext,
                              nsPresContext* aPresContext,
                              PRBool& aCanStoreInRuleTree)
{
  NS_PRECONDITION(aData->Count() == 2, "Invalid array!");

  










  ProcessTranslatePart(aDelta[0], aX[0], aData->Item(1),
                       aContext, aPresContext, aCanStoreInRuleTree);
}


static void ProcessTranslateY(nscoord aDelta[2], float aY[2],
                              const nsCSSValue::Array* aData,
                              nsStyleContext* aContext,
                              nsPresContext* aPresContext,
                              PRBool& aCanStoreInRuleTree)
{
  NS_PRECONDITION(aData->Count() == 2, "Invalid array!");

  










  ProcessTranslatePart(aDelta[1], aY[1], aData->Item(1),
                       aContext, aPresContext, aCanStoreInRuleTree);
}


static void ProcessTranslate(nscoord aDelta[2], float aX[2], float aY[2],
                             const nsCSSValue::Array* aData,
                             nsStyleContext* aContext,
                             nsPresContext* aPresContext,
                             PRBool& aCanStoreInRuleTree)
{
  NS_PRECONDITION(aData->Count() == 2 || aData->Count() == 3, "Invalid array!");

  







  ProcessTranslatePart(aDelta[0], aX[0], aData->Item(1),
                       aContext, aPresContext, aCanStoreInRuleTree);

  
  if (aData->Count() == 3) {
    ProcessTranslatePart(aDelta[1], aY[1], aData->Item(2),
                         aContext, aPresContext, aCanStoreInRuleTree);
  }
}


static void ProcessScaleHelper(float aXScale, float aYScale, float aMain[4])
{
  





  aMain[0] = aXScale;
  aMain[3] = aYScale;
}


static void ProcessScaleX(float aMain[4], const nsCSSValue::Array* aData)
{
  NS_PRECONDITION(aData->Count() == 2, "Bad array!");
  ProcessScaleHelper(aData->Item(1).GetFloatValue(), 1.0f, aMain);
}


static void ProcessScaleY(float aMain[4], const nsCSSValue::Array* aData)
{
  NS_PRECONDITION(aData->Count() == 2, "Bad array!");
  ProcessScaleHelper(1.0f, aData->Item(1).GetFloatValue(), aMain);
}


static void ProcessScale(float aMain[4], const nsCSSValue::Array* aData)
{
  NS_PRECONDITION(aData->Count() == 2 || aData->Count() == 3, "Bad array!");
  


  const nsCSSValue& scaleX = aData->Item(1);
  const nsCSSValue& scaleY = (aData->Count() == 2 ? scaleX :
                              aData->Item(2));

  ProcessScaleHelper(scaleX.GetFloatValue(),
                     scaleY.GetFloatValue(), aMain);
}




static void ProcessSkewHelper(double aXAngle, double aYAngle, float aMain[4])
{
  






  aMain[2] = SafeTangent(aXAngle);
  aMain[1] = SafeTangent(aYAngle);
}


static void ProcessSkewX(float aMain[4], const nsCSSValue::Array* aData)
{
  NS_ASSERTION(aData->Count() == 2, "Bad array!");
  ProcessSkewHelper(aData->Item(1).GetAngleValueInRadians(), 0.0, aMain);
}


static void ProcessSkewY(float aMain[4], const nsCSSValue::Array* aData)
{
  NS_ASSERTION(aData->Count() == 2, "Bad array!");
  ProcessSkewHelper(0.0, aData->Item(1).GetAngleValueInRadians(), aMain);
}


static void ProcessSkew(float aMain[4], const nsCSSValue::Array* aData)
{
  NS_ASSERTION(aData->Count() == 2 || aData->Count() == 3, "Bad array!");

  double xSkew = aData->Item(1).GetAngleValueInRadians();
  double ySkew = (aData->Count() == 2
                  ? 0.0 : aData->Item(2).GetAngleValueInRadians());

  ProcessSkewHelper(xSkew, ySkew, aMain);
}


static void ProcessRotate(float aMain[4], const nsCSSValue::Array* aData)
{
  NS_PRECONDITION(aData->Count() == 2, "Invalid array!");

  





  double theta = aData->Item(1).GetAngleValueInRadians();
  float cosTheta = FlushToZero(cos(theta));
  float sinTheta = FlushToZero(sin(theta));

  aMain[0] = cosTheta;
  aMain[1] = sinTheta;
  aMain[2] = -sinTheta;
  aMain[3] = cosTheta;
}





 nsCSSKeyword
nsStyleTransformMatrix::TransformFunctionOf(const nsCSSValue::Array* aData)
{
  nsAutoString keyword;
  aData->Item(0).GetStringValue(keyword);
  return nsCSSKeywords::LookupKeyword(keyword);
}





void
nsStyleTransformMatrix::SetToTransformFunction(const nsCSSValue::Array * aData,
                                               nsStyleContext* aContext,
                                               nsPresContext* aPresContext,
                                               PRBool& aCanStoreInRuleTree)
{
  NS_PRECONDITION(aData, "Why did you want to get data from a null array?");
  
  
  

  


  SetToIdentity();

  
  switch (TransformFunctionOf(aData)) {
  case eCSSKeyword_translatex:
    ProcessTranslateX(mDelta, mX, aData, aContext, aPresContext,
                      aCanStoreInRuleTree);
    break;
  case eCSSKeyword_translatey:
    ProcessTranslateY(mDelta, mY, aData, aContext, aPresContext,
                      aCanStoreInRuleTree);
    break;
  case eCSSKeyword_translate:
    ProcessTranslate(mDelta, mX, mY, aData, aContext, aPresContext,
                     aCanStoreInRuleTree);
    break;
  case eCSSKeyword_scalex:
    ProcessScaleX(mMain, aData);
    break;
  case eCSSKeyword_scaley:
    ProcessScaleY(mMain, aData);
    break;
  case eCSSKeyword_scale:
    ProcessScale(mMain, aData);
    break;
  case eCSSKeyword_skewx:
    ProcessSkewX(mMain, aData);
    break;
  case eCSSKeyword_skewy:
    ProcessSkewY(mMain, aData);
    break;
  case eCSSKeyword_skew:
    ProcessSkew(mMain, aData);
    break;
  case eCSSKeyword_rotate:
    ProcessRotate(mMain, aData);
    break;
  case eCSSKeyword_matrix:
    ProcessMatrix(mMain, mDelta, mX, mY, aData, aContext, aPresContext,
                  aCanStoreInRuleTree);
    break;
  default:
    NS_NOTREACHED("Unknown transform function!");
  }
}










 nsStyleTransformMatrix
nsStyleTransformMatrix::ReadTransforms(const nsCSSValueList* aList,
                                       nsStyleContext* aContext,
                                       nsPresContext* aPresContext,
                                       PRBool &aCanStoreInRuleTree)
{
  nsStyleTransformMatrix result;

  for (const nsCSSValueList* curr = aList; curr != nsnull; curr = curr->mNext) {
    const nsCSSValue &currElem = curr->mValue;
    NS_ASSERTION(currElem.GetUnit() == eCSSUnit_Function,
                 "Stream should consist solely of functions!");
    NS_ASSERTION(currElem.GetArrayValue()->Count() >= 1,
                 "Incoming function is too short!");

    
    nsStyleTransformMatrix currMatrix;
    currMatrix.SetToTransformFunction(currElem.GetArrayValue(), aContext,
                                      aPresContext, aCanStoreInRuleTree);
    result *= currMatrix;
  }
  return result;
}




PRBool
nsStyleTransformMatrix::operator ==(const nsStyleTransformMatrix &aOther) const
{
  for (PRInt32 index = 0; index < 4; ++index)
    if (mMain[index] != aOther.mMain[index])
      return PR_FALSE;

  for (PRInt32 index = 0; index < 2; ++index)
    if (mDelta[index] != aOther.mDelta[index] ||
        mX[index] != aOther.mX[index] ||
        mY[index] != aOther.mY[index])
      return PR_FALSE;

  return PR_TRUE;
}
