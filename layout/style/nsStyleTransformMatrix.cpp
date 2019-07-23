







































#include "nsStyleTransformMatrix.h"
#include "nsAutoPtr.h"
#include "nsCSSValue.h"
#include "nsStyleContext.h"
#include "nsPresContext.h"
#include "nsRuleNode.h"
#include "nsCSSKeywords.h"
#include <math.h>


const float kPi      = 3.1415926535897932384626433832795f;
const float kTwoPi   = 6.283185307179586476925286766559f;
const float kEpsilon = 0.0001f;





static float SafeTangent(float aTheta)
{
  



  float sinTheta = sin(aTheta);
  float cosTheta = cos(aTheta);
  
  
  if (cosTheta >= 0 && cosTheta < kEpsilon)
    cosTheta = kEpsilon;
  else if (cosTheta < 0 && cosTheta >= -kEpsilon)
    cosTheta = -kEpsilon;
  
  return sinTheta / cosTheta;
}





static inline float ConstrainFloatValue(float aValue)
{
  
  aValue = fmod(aValue, kTwoPi);
  return aValue >= kPi ? aValue - kTwoPi : aValue;
}





static float CSSToRadians(const nsCSSValue &aValue)
{
  NS_PRECONDITION(aValue.IsAngularUnit(),
                  "Expected an angle, but didn't find one!");
  
  switch (aValue.GetUnit()) {
  case eCSSUnit_Degree:
    
    return
      ConstrainFloatValue(aValue.GetFloatValue() * kPi / 180.0f);
  case eCSSUnit_Grad:
    
    return
      ConstrainFloatValue(aValue.GetFloatValue() * kPi / 200.0f);
  case eCSSUnit_Radian:
    
    return ConstrainFloatValue(aValue.GetFloatValue());
  default:
    NS_NOTREACHED("Unexpected angular unit!");
    return 0.0f;
  }
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


static void SetCoordToValue(const nsCSSValue &aValue,
                            nsStyleContext* aContext,
                            nsPresContext* aPresContext,
                            PRBool &aCanStoreInRuleTree, nscoord &aOut)
{
  aOut = nsRuleNode::CalcLength(aValue, aContext, aPresContext,
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
    SetCoordToValue(aData->Item(5), aContext, aPresContext, aCanStoreInRuleTree,
                    aDelta[0]);

  


  if (aData->Item(6).GetUnit() == eCSSUnit_Percent)
    aY[1] = aData->Item(6).GetPercentValue();
  else
    SetCoordToValue(aData->Item(6), aContext, aPresContext, aCanStoreInRuleTree,
                    aDelta[1]);
}


static void ProcessTranslateX(nscoord aDelta[2], float aX[2],
                              const nsCSSValue::Array* aData,
                              nsStyleContext* aContext,
                              nsPresContext* aPresContext,
                              PRBool& aCanStoreInRuleTree)
{
  NS_PRECONDITION(aData->Count() == 2, "Invalid array!");

  










  if (aData->Item(1).GetUnit() != eCSSUnit_Percent)
    SetCoordToValue(aData->Item(1), aContext, aPresContext, aCanStoreInRuleTree,
                    aDelta[0]);
  else
    aX[0] = aData->Item(1).GetPercentValue();
}


static void ProcessTranslateY(nscoord aDelta[2], float aY[2],
                              const nsCSSValue::Array* aData,
                              nsStyleContext* aContext,
                              nsPresContext* aPresContext,
                              PRBool& aCanStoreInRuleTree)
{
  NS_PRECONDITION(aData->Count() == 2, "Invalid array!");

  










  if (aData->Item(1).GetUnit() != eCSSUnit_Percent)
    SetCoordToValue(aData->Item(1), aContext, aPresContext, aCanStoreInRuleTree,
                    aDelta[1]);
  else
    aY[1] = aData->Item(1).GetPercentValue();
}


static void ProcessTranslate(nscoord aDelta[2], float aX[2], float aY[2],
                             const nsCSSValue::Array* aData,
                             nsStyleContext* aContext,
                             nsPresContext* aPresContext,
                             PRBool& aCanStoreInRuleTree)
{
  NS_PRECONDITION(aData->Count() == 2 || aData->Count() == 3, "Invalid array!");

  







  const nsCSSValue &dx = aData->Item(1);
  if (dx.GetUnit() == eCSSUnit_Percent)
    aX[0] = dx.GetPercentValue();
  else
    SetCoordToValue(dx, aContext, aPresContext, aCanStoreInRuleTree, aDelta[0]);

  
  if (aData->Count() == 3) {
    const nsCSSValue &dy = aData->Item(2);
    if (dy.GetUnit() == eCSSUnit_Percent)
      aY[1] = dy.GetPercentValue();
    else
      SetCoordToValue(dy, aContext, aPresContext, aCanStoreInRuleTree,
                      aDelta[1]); 
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




static void ProcessSkewHelper(float aXAngle, float aYAngle, float aMain[4])
{
  






  aMain[2] = SafeTangent(aXAngle);
  aMain[1] = SafeTangent(aYAngle);
}


static void ProcessSkewX(float aMain[4], const nsCSSValue::Array* aData)
{
  NS_ASSERTION(aData->Count() == 2, "Bad array!");
  ProcessSkewHelper(CSSToRadians(aData->Item(1)), 0.0f, aMain);
}


static void ProcessSkewY(float aMain[4], const nsCSSValue::Array* aData)
{
  NS_ASSERTION(aData->Count() == 2, "Bad array!");
  ProcessSkewHelper(0.0f, CSSToRadians(aData->Item(1)), aMain);
}


static void ProcessSkew(float aMain[4], const nsCSSValue::Array* aData)
{
  NS_ASSERTION(aData->Count() == 2 || aData->Count() == 3, "Bad array!");
  
  float xSkew = CSSToRadians(aData->Item(1));
  float ySkew = (aData->Count() == 2 ? 0.0f : CSSToRadians(aData->Item(2)));

  ProcessSkewHelper(xSkew, ySkew, aMain);
}


static void ProcessRotate(float aMain[4], const nsCSSValue::Array* aData)
{
  NS_PRECONDITION(aData->Count() == 2, "Invalid array!");

  





  float theta = CSSToRadians(aData->Item(1));
  float cosTheta = cos(theta);
  float sinTheta = sin(theta);

  aMain[0] = cosTheta;
  aMain[1] = sinTheta;
  aMain[2] = -sinTheta;
  aMain[3] = cosTheta;
}





void
nsStyleTransformMatrix::SetToTransformFunction(const nsCSSValue::Array * aData,
                                               nsStyleContext* aContext,
                                               nsPresContext* aPresContext,
                                               PRBool& aCanStoreInRuleTree)
{
  NS_PRECONDITION(aData, "Why did you want to get data from a null array?");
  NS_PRECONDITION(aContext, "Need a context for unit conversion!");
  NS_PRECONDITION(aPresContext, "Need a context for unit conversion!");
  
  


  SetToIdentity();

  
  nsAutoString keyword;
  aData->Item(0).GetStringValue(keyword);
  switch (nsCSSKeywords::LookupKeyword(keyword)) {
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
