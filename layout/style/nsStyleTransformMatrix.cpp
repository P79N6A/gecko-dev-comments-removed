








































#include "nsStyleTransformMatrix.h"
#include "nsAutoPtr.h"
#include "nsCSSValue.h"
#include "nsStyleContext.h"
#include "nsPresContext.h"
#include "nsRuleNode.h"
#include "nsCSSKeywords.h"
#include "nsMathUtils.h"
#include "CSSCalc.h"
#include "nsStyleAnimation.h"

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

static void
ProcessTranslatePart(float& aResult,
                     const nsCSSValue& aValue,
                     nsStyleContext* aContext,
                     nsPresContext* aPresContext,
                     PRBool& aCanStoreInRuleTree,
                     nscoord aSize, float aAppUnitsPerMatrixUnit)
{
  nscoord offset = 0;
  float percent = 0.0f;

  if (aValue.GetUnit() == eCSSUnit_Percent) {
    percent = aValue.GetPercentValue();
  } else if (aValue.IsCalcUnit()) {
    nsRuleNode::ComputedCalc result =
      nsRuleNode::SpecifiedCalcToComputedCalc(aValue, aContext, aPresContext,
                                              aCanStoreInRuleTree);
    percent = result.mPercent;
    offset = result.mLength;
  } else {
    offset = CalcLength(aValue, aContext, aPresContext,
                         aCanStoreInRuleTree);
  }

  aResult = (percent * NSAppUnitsToFloatPixels(aSize, aAppUnitsPerMatrixUnit)) + 
            NSAppUnitsToFloatPixels(offset, aAppUnitsPerMatrixUnit);
}


 gfx3DMatrix
nsStyleTransformMatrix::ProcessMatrix(const nsCSSValue::Array* aData,
                                      nsStyleContext* aContext,
                                      nsPresContext* aPresContext,
                                      PRBool& aCanStoreInRuleTree,
                                      nsRect& aBounds, float aAppUnitsPerMatrixUnit,
                                      PRBool *aPercentX, PRBool *aPercentY)
{
  NS_PRECONDITION(aData->Count() == 7, "Invalid array!");

  gfx3DMatrix result;

  


  result._11 = aData->Item(1).GetFloatValue();
  result._12 = aData->Item(2).GetFloatValue();
  result._21 = aData->Item(3).GetFloatValue();
  result._22 = aData->Item(4).GetFloatValue();

  


  ProcessTranslatePart(result._41, aData->Item(5),
                       aContext, aPresContext, aCanStoreInRuleTree,
                       aBounds.Width(), aAppUnitsPerMatrixUnit);
  ProcessTranslatePart(result._42, aData->Item(6),
                       aContext, aPresContext, aCanStoreInRuleTree,
                       aBounds.Height(), aAppUnitsPerMatrixUnit);

  return result;
}


 gfx3DMatrix
nsStyleTransformMatrix::ProcessInterpolateMatrix(const nsCSSValue::Array* aData,
                                                 nsStyleContext* aContext,
                                                 nsPresContext* aPresContext,
                                                 PRBool& aCanStoreInRuleTree,
                                                 nsRect& aBounds, float aAppUnitsPerMatrixUnit)
{
  NS_PRECONDITION(aData->Count() == 5, "Invalid array!");

  double coeff1 = aData->Item(1).GetPercentValue();
  gfx3DMatrix matrix1 = ReadTransforms(aData->Item(2).GetListValue(),
                                       aContext, aPresContext,
                                       aCanStoreInRuleTree,
                                       aBounds, aAppUnitsPerMatrixUnit);
  double coeff2 = aData->Item(3).GetPercentValue();
  gfx3DMatrix matrix2 = ReadTransforms(aData->Item(4).GetListValue(),
                                       aContext, aPresContext,
                                       aCanStoreInRuleTree,
                                       aBounds, aAppUnitsPerMatrixUnit);

  gfxMatrix matrix2d1, matrix2d2;
#ifdef DEBUG
  PRBool is2d =
#endif
  matrix1.Is2D(&matrix2d1);
  NS_ABORT_IF_FALSE(is2d, "Can't do animations with 3d transforms!");
#ifdef DEBUG
  is2d =
#endif
  matrix2.Is2D(&matrix2d2);
  NS_ABORT_IF_FALSE(is2d, "Can't do animations with 3d transforms!");

  return gfx3DMatrix::From2D(
    nsStyleAnimation::InterpolateTransformMatrix(matrix2d1, coeff1, 
                                                 matrix2d2, coeff2));
}


 gfx3DMatrix
nsStyleTransformMatrix::ProcessTranslateX(const nsCSSValue::Array* aData,
                                          nsStyleContext* aContext,
                                          nsPresContext* aPresContext,
                                          PRBool& aCanStoreInRuleTree,
                                          nsRect& aBounds, float aAppUnitsPerMatrixUnit)
{
  NS_PRECONDITION(aData->Count() == 2, "Invalid array!");

  gfx3DMatrix temp;

  











  ProcessTranslatePart(temp._41, aData->Item(1),
                       aContext, aPresContext, aCanStoreInRuleTree,
                       aBounds.Width(), aAppUnitsPerMatrixUnit);
  return temp;
}


 gfx3DMatrix
nsStyleTransformMatrix::ProcessTranslateY(const nsCSSValue::Array* aData,
                                          nsStyleContext* aContext,
                                          nsPresContext* aPresContext,
                                          PRBool& aCanStoreInRuleTree,
                                          nsRect& aBounds, float aAppUnitsPerMatrixUnit)
{
  NS_PRECONDITION(aData->Count() == 2, "Invalid array!");

  gfx3DMatrix temp;

  











  ProcessTranslatePart(temp._42, aData->Item(1),
                       aContext, aPresContext, aCanStoreInRuleTree,
                       aBounds.Height(), aAppUnitsPerMatrixUnit);
  return temp;
}


 gfx3DMatrix
nsStyleTransformMatrix::ProcessTranslate(const nsCSSValue::Array* aData,
                                         nsStyleContext* aContext,
                                         nsPresContext* aPresContext,
                                         PRBool& aCanStoreInRuleTree,
                                         nsRect& aBounds, float aAppUnitsPerMatrixUnit)
{
  NS_PRECONDITION(aData->Count() == 2 || aData->Count() == 3, "Invalid array!");

  gfx3DMatrix temp;

  







  ProcessTranslatePart(temp._41, aData->Item(1),
                       aContext, aPresContext, aCanStoreInRuleTree,
                       aBounds.Width(), aAppUnitsPerMatrixUnit);

  
  if (aData->Count() == 3) {
    ProcessTranslatePart(temp._42, aData->Item(2),
                         aContext, aPresContext, aCanStoreInRuleTree,
                         aBounds.Height(), aAppUnitsPerMatrixUnit);
  }
  return temp;
}


 gfx3DMatrix
nsStyleTransformMatrix::ProcessScaleHelper(float aXScale, float aYScale, float aZScale)
{
  






  gfx3DMatrix temp;
  temp._11 = aXScale;
  temp._22 = aYScale;
  temp._33 = aZScale;
  return temp;
}


 gfx3DMatrix
nsStyleTransformMatrix::ProcessScaleX(const nsCSSValue::Array* aData)
{
  NS_PRECONDITION(aData->Count() == 2, "Bad array!");
  return ProcessScaleHelper(aData->Item(1).GetFloatValue(), 1.0f, 1.0f);
}


 gfx3DMatrix
nsStyleTransformMatrix::ProcessScaleY(const nsCSSValue::Array* aData)
{
  NS_PRECONDITION(aData->Count() == 2, "Bad array!");
  return ProcessScaleHelper(1.0f, aData->Item(1).GetFloatValue(), 1.0f);
}


 gfx3DMatrix
nsStyleTransformMatrix::ProcessScale(const nsCSSValue::Array* aData)
{
  NS_PRECONDITION(aData->Count() == 2 || aData->Count() == 3, "Bad array!");
  


  const nsCSSValue& scaleX = aData->Item(1);
  const nsCSSValue& scaleY = (aData->Count() == 2 ? scaleX :
                              aData->Item(2));

  return ProcessScaleHelper(scaleX.GetFloatValue(),
                            scaleY.GetFloatValue(),
                            1.0f);
}




 gfx3DMatrix
nsStyleTransformMatrix::ProcessSkewHelper(double aXAngle, double aYAngle)
{
  







  gfx3DMatrix temp;
  temp._12 = SafeTangent(aYAngle);
  temp._21 = SafeTangent(aXAngle);
  return temp;
}


 gfx3DMatrix
nsStyleTransformMatrix::ProcessSkewX(const nsCSSValue::Array* aData)
{
  NS_ASSERTION(aData->Count() == 2, "Bad array!");
  return ProcessSkewHelper(aData->Item(1).GetAngleValueInRadians(), 0.0);
}


 gfx3DMatrix
nsStyleTransformMatrix::ProcessSkewY(const nsCSSValue::Array* aData)
{
  NS_ASSERTION(aData->Count() == 2, "Bad array!");
  return ProcessSkewHelper(0.0, aData->Item(1).GetAngleValueInRadians());
}


 gfx3DMatrix
nsStyleTransformMatrix::ProcessSkew(const nsCSSValue::Array* aData)
{
  NS_ASSERTION(aData->Count() == 2 || aData->Count() == 3, "Bad array!");

  double xSkew = aData->Item(1).GetAngleValueInRadians();
  double ySkew = (aData->Count() == 2
                  ? 0.0 : aData->Item(2).GetAngleValueInRadians());

  return ProcessSkewHelper(xSkew, ySkew);
}


 gfx3DMatrix
nsStyleTransformMatrix::ProcessRotateZ(const nsCSSValue::Array* aData)
{
  NS_PRECONDITION(aData->Count() == 2, "Invalid array!");

  






  double theta = aData->Item(1).GetAngleValueInRadians();
  float cosTheta = FlushToZero(cos(theta));
  float sinTheta = FlushToZero(sin(theta));

  gfx3DMatrix temp;

  temp._11 = cosTheta;
  temp._12 = sinTheta;
  temp._21 = -sinTheta;
  temp._22 = cosTheta;
  return temp;
}





 nsCSSKeyword
nsStyleTransformMatrix::TransformFunctionOf(const nsCSSValue::Array* aData)
{
  nsAutoString keyword;
  aData->Item(0).GetStringValue(keyword);
  return nsCSSKeywords::LookupKeyword(keyword);
}





gfx3DMatrix
nsStyleTransformMatrix::MatrixForTransformFunction(const nsCSSValue::Array * aData,
                                                   nsStyleContext* aContext,
                                                   nsPresContext* aPresContext,
                                                   PRBool& aCanStoreInRuleTree,
                                                   nsRect& aBounds, 
                                                   float aAppUnitsPerMatrixUnit)
{
  NS_PRECONDITION(aData, "Why did you want to get data from a null array?");
  
  
  


  
  switch (TransformFunctionOf(aData)) {
  case eCSSKeyword_translatex:
    return ProcessTranslateX(aData, aContext, aPresContext,
                             aCanStoreInRuleTree, aBounds, aAppUnitsPerMatrixUnit);
  case eCSSKeyword_translatey:
    return ProcessTranslateY(aData, aContext, aPresContext,
                             aCanStoreInRuleTree, aBounds, aAppUnitsPerMatrixUnit);
  case eCSSKeyword_translate:
    return ProcessTranslate(aData, aContext, aPresContext,
                            aCanStoreInRuleTree, aBounds, aAppUnitsPerMatrixUnit);
  case eCSSKeyword_scalex:
    return ProcessScaleX(aData);
  case eCSSKeyword_scaley:
    return ProcessScaleY(aData);
  case eCSSKeyword_scale:
    return ProcessScale(aData);
  case eCSSKeyword_skewx:
    return ProcessSkewX(aData);
  case eCSSKeyword_skewy:
    return ProcessSkewY(aData);
  case eCSSKeyword_skew:
    return ProcessSkew(aData);
  case eCSSKeyword_rotate:
    return ProcessRotateZ(aData);
  case eCSSKeyword_matrix:
    return ProcessMatrix(aData, aContext, aPresContext,
                         aCanStoreInRuleTree, aBounds, aAppUnitsPerMatrixUnit);
  case eCSSKeyword_interpolatematrix:
    return ProcessInterpolateMatrix(aData, aContext, aPresContext,
                                    aCanStoreInRuleTree, aBounds, aAppUnitsPerMatrixUnit);
  default:
    NS_NOTREACHED("Unknown transform function!");
  }
  return gfx3DMatrix();
}

 gfx3DMatrix
nsStyleTransformMatrix::ReadTransforms(const nsCSSValueList* aList,
                                       nsStyleContext* aContext,
                                       nsPresContext* aPresContext,
                                       PRBool &aCanStoreInRuleTree,
                                       nsRect& aBounds,
                                       float aAppUnitsPerMatrixUnit)
{
  gfx3DMatrix result;

  for (const nsCSSValueList* curr = aList; curr != nsnull; curr = curr->mNext) {
    const nsCSSValue &currElem = curr->mValue;
    NS_ASSERTION(currElem.GetUnit() == eCSSUnit_Function,
                 "Stream should consist solely of functions!");
    NS_ASSERTION(currElem.GetArrayValue()->Count() >= 1,
                 "Incoming function is too short!");

    
    result = MatrixForTransformFunction(currElem.GetArrayValue(), aContext,
                                        aPresContext, aCanStoreInRuleTree,
                                        aBounds, aAppUnitsPerMatrixUnit) * result;
  }
  
  return result;
}

