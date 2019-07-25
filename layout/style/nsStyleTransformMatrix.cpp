








































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

namespace nsStyleTransformMatrix {










static double FlushToZero(double aVal)
{
  if (-FLT_EPSILON < aVal && aVal < FLT_EPSILON)
    return 0.0f;
  else
    return aVal;
}


static nscoord CalcLength(const nsCSSValue &aValue,
                          nsStyleContext* aContext,
                          nsPresContext* aPresContext,
                          bool &aCanStoreInRuleTree)
{
  if (aValue.GetUnit() == eCSSUnit_Pixel ||
      aValue.GetUnit() == eCSSUnit_Number) {
    
    
    
    
    
    
    
    return nsPresContext::CSSPixelsToAppUnits(aValue.GetFloatValue());
  }
  return nsRuleNode::CalcLength(aValue, aContext, aPresContext,
                                aCanStoreInRuleTree);
}

static float
ProcessTranslatePart(const nsCSSValue& aValue,
                     nsStyleContext* aContext,
                     nsPresContext* aPresContext,
                     bool& aCanStoreInRuleTree,
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

  return (percent * NSAppUnitsToFloatPixels(aSize, aAppUnitsPerMatrixUnit)) + 
         NSAppUnitsToFloatPixels(offset, aAppUnitsPerMatrixUnit);
}








static void
ProcessMatrix(gfx3DMatrix& aMatrix,
              const nsCSSValue::Array* aData,
              nsStyleContext* aContext,
              nsPresContext* aPresContext,
              bool& aCanStoreInRuleTree,
              nsRect& aBounds, float aAppUnitsPerMatrixUnit)
{
  NS_PRECONDITION(aData->Count() == 7, "Invalid array!");

  gfxMatrix result;

  


  result.xx = aData->Item(1).GetFloatValue();
  result.yx = aData->Item(2).GetFloatValue();
  result.xy = aData->Item(3).GetFloatValue();
  result.yy = aData->Item(4).GetFloatValue();

  


  result.x0 = ProcessTranslatePart(aData->Item(5),
                                   aContext, aPresContext, aCanStoreInRuleTree,
                                   aBounds.Width(), aAppUnitsPerMatrixUnit);
  result.y0 = ProcessTranslatePart(aData->Item(6),
                                   aContext, aPresContext, aCanStoreInRuleTree,
                                   aBounds.Height(), aAppUnitsPerMatrixUnit);

  aMatrix.PreMultiply(result);
}

static void 
ProcessMatrix3D(gfx3DMatrix& aMatrix,
                const nsCSSValue::Array* aData,
                nsStyleContext* aContext,
                nsPresContext* aPresContext,
                bool& aCanStoreInRuleTree,
                nsRect& aBounds, float aAppUnitsPerMatrixUnit)
{
  NS_PRECONDITION(aData->Count() == 17, "Invalid array!");

  gfx3DMatrix temp;

  temp._11 = aData->Item(1).GetFloatValue();
  temp._12 = aData->Item(2).GetFloatValue();
  temp._13 = aData->Item(3).GetFloatValue();
  temp._14 = aData->Item(4).GetFloatValue();
  temp._21 = aData->Item(5).GetFloatValue();
  temp._22 = aData->Item(6).GetFloatValue();
  temp._23 = aData->Item(7).GetFloatValue();
  temp._24 = aData->Item(8).GetFloatValue();
  temp._31 = aData->Item(9).GetFloatValue();
  temp._32 = aData->Item(10).GetFloatValue();
  temp._33 = aData->Item(11).GetFloatValue();
  temp._34 = aData->Item(12).GetFloatValue();
  temp._44 = aData->Item(16).GetFloatValue();

  temp._41 = ProcessTranslatePart(aData->Item(13),
                                  aContext, aPresContext, aCanStoreInRuleTree,
                                  aBounds.Width(), aAppUnitsPerMatrixUnit);
  temp._42 = ProcessTranslatePart(aData->Item(14),
                                  aContext, aPresContext, aCanStoreInRuleTree,
                                  aBounds.Height(), aAppUnitsPerMatrixUnit);
  temp._43 = ProcessTranslatePart(aData->Item(15),
                                  aContext, aPresContext, aCanStoreInRuleTree,
                                  aBounds.Height(), aAppUnitsPerMatrixUnit);

  aMatrix.PreMultiply(temp);
}


static void
ProcessInterpolateMatrix(gfx3DMatrix& aMatrix,
                         const nsCSSValue::Array* aData,
                         nsStyleContext* aContext,
                         nsPresContext* aPresContext,
                         bool& aCanStoreInRuleTree,
                         nsRect& aBounds, float aAppUnitsPerMatrixUnit)
{
  NS_PRECONDITION(aData->Count() == 4, "Invalid array!");

  gfx3DMatrix matrix1, matrix2;
  if (aData->Item(1).GetUnit() == eCSSUnit_List) {
    matrix1 = nsStyleTransformMatrix::ReadTransforms(aData->Item(1).GetListValue(),
                             aContext, aPresContext,
                             aCanStoreInRuleTree,
                             aBounds, aAppUnitsPerMatrixUnit);
  }
  if (aData->Item(2).GetUnit() == eCSSUnit_List) {
    matrix2 = ReadTransforms(aData->Item(2).GetListValue(),
                             aContext, aPresContext,
                             aCanStoreInRuleTree,
                             aBounds, aAppUnitsPerMatrixUnit);
  }
  double progress = aData->Item(3).GetPercentValue();

  aMatrix = nsStyleAnimation::InterpolateTransformMatrix(matrix1, matrix2, progress) * aMatrix;
}


static void
ProcessTranslateX(gfx3DMatrix& aMatrix, 
                  const nsCSSValue::Array* aData,
                  nsStyleContext* aContext,
                  nsPresContext* aPresContext,
                  bool& aCanStoreInRuleTree,
                  nsRect& aBounds, float aAppUnitsPerMatrixUnit)
{
  NS_PRECONDITION(aData->Count() == 2, "Invalid array!");

  gfxPoint3D temp;

  temp.x = ProcessTranslatePart(aData->Item(1),
                                aContext, aPresContext, aCanStoreInRuleTree,
                                aBounds.Width(), aAppUnitsPerMatrixUnit);
  aMatrix.Translate(temp);
}


static void
ProcessTranslateY(gfx3DMatrix& aMatrix,
                  const nsCSSValue::Array* aData,
                  nsStyleContext* aContext,
                  nsPresContext* aPresContext,
                  bool& aCanStoreInRuleTree,
                  nsRect& aBounds, float aAppUnitsPerMatrixUnit)
{
  NS_PRECONDITION(aData->Count() == 2, "Invalid array!");

  gfxPoint3D temp;

  temp.y = ProcessTranslatePart(aData->Item(1),
                                aContext, aPresContext, aCanStoreInRuleTree,
                                aBounds.Height(), aAppUnitsPerMatrixUnit);
  aMatrix.Translate(temp);
}

static void 
ProcessTranslateZ(gfx3DMatrix& aMatrix,
                  const nsCSSValue::Array* aData,
                  nsStyleContext* aContext,
                                          nsPresContext* aPresContext,
                                          bool& aCanStoreInRuleTree,
                                          float aAppUnitsPerMatrixUnit)
{
  NS_PRECONDITION(aData->Count() == 2, "Invalid array!");

  gfxPoint3D temp;

  temp.z = ProcessTranslatePart(aData->Item(1),
                                aContext, aPresContext, aCanStoreInRuleTree,
                                0, aAppUnitsPerMatrixUnit);
  aMatrix.Translate(temp);
}


static void
ProcessTranslate(gfx3DMatrix& aMatrix,
                 const nsCSSValue::Array* aData,
                 nsStyleContext* aContext,
                 nsPresContext* aPresContext,
                 bool& aCanStoreInRuleTree,
                 nsRect& aBounds, float aAppUnitsPerMatrixUnit)
{
  NS_PRECONDITION(aData->Count() == 2 || aData->Count() == 3, "Invalid array!");

  gfxPoint3D temp;

  temp.x = ProcessTranslatePart(aData->Item(1),
                                aContext, aPresContext, aCanStoreInRuleTree,
                                aBounds.Width(), aAppUnitsPerMatrixUnit);

  
  if (aData->Count() == 3) {
    temp.y = ProcessTranslatePart(aData->Item(2),
                                  aContext, aPresContext, aCanStoreInRuleTree,
                                  aBounds.Height(), aAppUnitsPerMatrixUnit);
  }
  aMatrix.Translate(temp);
}

static void
ProcessTranslate3D(gfx3DMatrix& aMatrix,
                   const nsCSSValue::Array* aData,
                   nsStyleContext* aContext,
                   nsPresContext* aPresContext,
                   bool& aCanStoreInRuleTree,
                   nsRect& aBounds, float aAppUnitsPerMatrixUnit)
{
  NS_PRECONDITION(aData->Count() == 4, "Invalid array!");

  gfxPoint3D temp;

  temp.x = ProcessTranslatePart(aData->Item(1),
                                aContext, aPresContext, aCanStoreInRuleTree,
                                aBounds.Width(), aAppUnitsPerMatrixUnit);

  temp.y = ProcessTranslatePart(aData->Item(2),
                                aContext, aPresContext, aCanStoreInRuleTree,
                                aBounds.Height(), aAppUnitsPerMatrixUnit);

  temp.z = ProcessTranslatePart(aData->Item(3),
                                aContext, aPresContext, aCanStoreInRuleTree,
                                0, aAppUnitsPerMatrixUnit);

  aMatrix.Translate(temp);
}


static void
ProcessScaleHelper(gfx3DMatrix& aMatrix,
                   float aXScale, 
                   float aYScale, 
                   float aZScale)
{
  aMatrix.Scale(aXScale, aYScale, aZScale);
}


static void
ProcessScaleX(gfx3DMatrix& aMatrix, const nsCSSValue::Array* aData)
{
  NS_PRECONDITION(aData->Count() == 2, "Bad array!");
  ProcessScaleHelper(aMatrix, aData->Item(1).GetFloatValue(), 1.0f, 1.0f);
}


static void
ProcessScaleY(gfx3DMatrix& aMatrix, const nsCSSValue::Array* aData)
{
  NS_PRECONDITION(aData->Count() == 2, "Bad array!");
  ProcessScaleHelper(aMatrix, 1.0f, aData->Item(1).GetFloatValue(), 1.0f);
}

static void
ProcessScaleZ(gfx3DMatrix& aMatrix, const nsCSSValue::Array* aData)
{
  NS_PRECONDITION(aData->Count() == 2, "Bad array!");
  ProcessScaleHelper(aMatrix, 1.0f, 1.0f, aData->Item(1).GetFloatValue());
}

static void
ProcessScale3D(gfx3DMatrix& aMatrix, const nsCSSValue::Array* aData)
{
  NS_PRECONDITION(aData->Count() == 4, "Bad array!");
  ProcessScaleHelper(aMatrix,
                     aData->Item(1).GetFloatValue(),
                     aData->Item(2).GetFloatValue(),
                     aData->Item(3).GetFloatValue());
}


static void
ProcessScale(gfx3DMatrix& aMatrix, const nsCSSValue::Array* aData)
{
  NS_PRECONDITION(aData->Count() == 2 || aData->Count() == 3, "Bad array!");
  


  const nsCSSValue& scaleX = aData->Item(1);
  const nsCSSValue& scaleY = (aData->Count() == 2 ? scaleX :
                              aData->Item(2));

  ProcessScaleHelper(aMatrix, 
                     scaleX.GetFloatValue(),
                     scaleY.GetFloatValue(),
                     1.0f);
}




static void
ProcessSkewHelper(gfx3DMatrix& aMatrix, double aXAngle, double aYAngle)
{
  aMatrix.SkewXY(aXAngle, aYAngle);
}


static void
ProcessSkewX(gfx3DMatrix& aMatrix, const nsCSSValue::Array* aData)
{
  NS_ASSERTION(aData->Count() == 2, "Bad array!");
  ProcessSkewHelper(aMatrix, aData->Item(1).GetAngleValueInRadians(), 0.0);
}


static void
ProcessSkewY(gfx3DMatrix& aMatrix, const nsCSSValue::Array* aData)
{
  NS_ASSERTION(aData->Count() == 2, "Bad array!");
  ProcessSkewHelper(aMatrix, 0.0, aData->Item(1).GetAngleValueInRadians());
}


static void
ProcessSkew(gfx3DMatrix& aMatrix, const nsCSSValue::Array* aData)
{
  NS_ASSERTION(aData->Count() == 2 || aData->Count() == 3, "Bad array!");

  double xSkew = aData->Item(1).GetAngleValueInRadians();
  double ySkew = (aData->Count() == 2
                  ? 0.0 : aData->Item(2).GetAngleValueInRadians());

  ProcessSkewHelper(aMatrix, xSkew, ySkew);
}


static void
ProcessRotateZ(gfx3DMatrix& aMatrix, const nsCSSValue::Array* aData)
{
  NS_PRECONDITION(aData->Count() == 2, "Invalid array!");
  double theta = aData->Item(1).GetAngleValueInRadians();
  aMatrix.RotateZ(theta);
}

static void
ProcessRotateX(gfx3DMatrix& aMatrix, const nsCSSValue::Array* aData)
{
  NS_PRECONDITION(aData->Count() == 2, "Invalid array!");
  double theta = aData->Item(1).GetAngleValueInRadians();
  aMatrix.RotateX(theta);
}

static void
ProcessRotateY(gfx3DMatrix& aMatrix, const nsCSSValue::Array* aData)
{
  NS_PRECONDITION(aData->Count() == 2, "Invalid array!");
  double theta = aData->Item(1).GetAngleValueInRadians();
  aMatrix.RotateY(theta);
}

static void
ProcessRotate3D(gfx3DMatrix& aMatrix, const nsCSSValue::Array* aData)
{
  NS_PRECONDITION(aData->Count() == 5, "Invalid array!");

  







  



  double theta = -aData->Item(4).GetAngleValueInRadians();
  float cosTheta = FlushToZero(cos(theta));
  float sinTheta = FlushToZero(sin(theta));

  float x = aData->Item(1).GetFloatValue();
  float y = aData->Item(2).GetFloatValue();
  float z = aData->Item(3).GetFloatValue();

  
  float length = sqrt(x*x + y*y + z*z);
  if (length == 0.0) {
    return;
  }
  x /= length;
  y /= length;
  z /= length;

  gfx3DMatrix temp;

  
  temp._11 = 1 + (1 - cosTheta) * (x * x - 1);
  temp._12 = -z * sinTheta + (1 - cosTheta) * x * y;
  temp._13 = y * sinTheta + (1 - cosTheta) * x * z;
  temp._14 = 0.0f;
  temp._21 = z * sinTheta + (1 - cosTheta) * x * y;
  temp._22 = 1 + (1 - cosTheta) * (y * y - 1);
  temp._23 = -x * sinTheta + (1 - cosTheta) * y * z;
  temp._24 = 0.0f;
  temp._31 = -y * sinTheta + (1 - cosTheta) * x * z;
  temp._32 = x * sinTheta + (1 - cosTheta) * y * z;
  temp._33 = 1 + (1 - cosTheta) * (z * z - 1);
  temp._34 = 0.0f;
  temp._41 = 0.0f;
  temp._42 = 0.0f;
  temp._43 = 0.0f;
  temp._44 = 1.0f;

  aMatrix = temp * aMatrix;
}

static void 
ProcessPerspective(gfx3DMatrix& aMatrix, 
                   const nsCSSValue::Array* aData,
                   nsStyleContext *aContext,
                   nsPresContext *aPresContext,
                   bool &aCanStoreInRuleTree,
                   float aAppUnitsPerMatrixUnit)
{
  NS_PRECONDITION(aData->Count() == 2, "Invalid array!");

  float depth = ProcessTranslatePart(aData->Item(1), aContext,
                                     aPresContext, aCanStoreInRuleTree,
                                     0, aAppUnitsPerMatrixUnit);
  aMatrix.Perspective(depth);
}






static void
MatrixForTransformFunction(gfx3DMatrix& aMatrix,
                           const nsCSSValue::Array * aData,
                           nsStyleContext* aContext,
                           nsPresContext* aPresContext,
                           bool& aCanStoreInRuleTree,
                           nsRect& aBounds, 
                           float aAppUnitsPerMatrixUnit)
{
  NS_PRECONDITION(aData, "Why did you want to get data from a null array?");
  
  
  


  
  switch (TransformFunctionOf(aData)) {
  case eCSSKeyword_translatex:
    ProcessTranslateX(aMatrix, aData, aContext, aPresContext,
                      aCanStoreInRuleTree, aBounds, aAppUnitsPerMatrixUnit);
    break;
  case eCSSKeyword_translatey:
    ProcessTranslateY(aMatrix, aData, aContext, aPresContext,
                      aCanStoreInRuleTree, aBounds, aAppUnitsPerMatrixUnit);
    break;
  case eCSSKeyword_translatez:
    ProcessTranslateZ(aMatrix, aData, aContext, aPresContext,
                      aCanStoreInRuleTree, aAppUnitsPerMatrixUnit);
    break;
  case eCSSKeyword_translate:
    ProcessTranslate(aMatrix, aData, aContext, aPresContext,
                     aCanStoreInRuleTree, aBounds, aAppUnitsPerMatrixUnit);
    break;
  case eCSSKeyword_translate3d:
    ProcessTranslate3D(aMatrix, aData, aContext, aPresContext,
                       aCanStoreInRuleTree, aBounds, aAppUnitsPerMatrixUnit);
    break;
  case eCSSKeyword_scalex:
    ProcessScaleX(aMatrix, aData);
    break;
  case eCSSKeyword_scaley:
    ProcessScaleY(aMatrix, aData);
    break;
  case eCSSKeyword_scalez:
    ProcessScaleZ(aMatrix, aData);
    break;
  case eCSSKeyword_scale:
    ProcessScale(aMatrix, aData);
    break;
  case eCSSKeyword_scale3d:
    ProcessScale3D(aMatrix, aData);
    break;
  case eCSSKeyword_skewx:
    ProcessSkewX(aMatrix, aData);
    break;
  case eCSSKeyword_skewy:
    ProcessSkewY(aMatrix, aData);
    break;
  case eCSSKeyword_skew:
    ProcessSkew(aMatrix, aData);
    break;
  case eCSSKeyword_rotatex:
    ProcessRotateX(aMatrix, aData);
    break;
  case eCSSKeyword_rotatey:
    ProcessRotateY(aMatrix, aData);
    break;
  case eCSSKeyword_rotatez:
  case eCSSKeyword_rotate:
    ProcessRotateZ(aMatrix, aData);
    break;
  case eCSSKeyword_rotate3d:
    ProcessRotate3D(aMatrix, aData);
    break;
  case eCSSKeyword_matrix:
    ProcessMatrix(aMatrix, aData, aContext, aPresContext,
                  aCanStoreInRuleTree, aBounds, aAppUnitsPerMatrixUnit);
    break;
  case eCSSKeyword_matrix3d:
    ProcessMatrix3D(aMatrix, aData, aContext, aPresContext,
                    aCanStoreInRuleTree, aBounds, aAppUnitsPerMatrixUnit);
    break;
  case eCSSKeyword_interpolatematrix:
    ProcessInterpolateMatrix(aMatrix, aData, aContext, aPresContext,
                             aCanStoreInRuleTree, aBounds, aAppUnitsPerMatrixUnit);
    break;
  case eCSSKeyword_perspective:
    ProcessPerspective(aMatrix, aData, aContext, aPresContext, 
                       aCanStoreInRuleTree, aAppUnitsPerMatrixUnit);
    break;
  default:
    NS_NOTREACHED("Unknown transform function!");
  }
}





nsCSSKeyword
TransformFunctionOf(const nsCSSValue::Array* aData)
{
  nsAutoString keyword;
  aData->Item(0).GetStringValue(keyword);
  return nsCSSKeywords::LookupKeyword(keyword);
}

gfx3DMatrix
ReadTransforms(const nsCSSValueList* aList,
               nsStyleContext* aContext,
               nsPresContext* aPresContext,
               bool &aCanStoreInRuleTree,
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

    
    MatrixForTransformFunction(result, currElem.GetArrayValue(), aContext,
                               aPresContext, aCanStoreInRuleTree,
                               aBounds, aAppUnitsPerMatrixUnit);
  }
  
  return result;
}

} 
