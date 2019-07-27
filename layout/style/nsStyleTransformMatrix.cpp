








#include "nsStyleTransformMatrix.h"
#include "nsCSSValue.h"
#include "nsPresContext.h"
#include "nsRuleNode.h"
#include "nsCSSKeywords.h"
#include "mozilla/StyleAnimationValue.h"
#include "gfxMatrix.h"

using namespace mozilla;
using namespace mozilla::gfx;

namespace nsStyleTransformMatrix {

















void
TransformReferenceBox::EnsureDimensionsAreCached()
{
  if (mIsCached) {
    return;
  }

  MOZ_ASSERT(mFrame);

  mIsCached = true;

  if (mFrame->GetStateBits() & NS_FRAME_SVG_LAYOUT) {
    
    mWidth = 0;
    mHeight = 0;
    return;
  }

  
  
  
  
  
  

  nsRect rect;

#ifndef UNIFIED_CONTINUATIONS
  rect = mFrame->GetRect();
#else
  
  for (const nsIFrame *currFrame = mFrame->FirstContinuation();
       currFrame != nullptr;
       currFrame = currFrame->GetNextContinuation())
  {
    
    
    rect.UnionRect(result, nsRect(currFrame->GetOffsetTo(mFrame),
                                  currFrame->GetSize()));
  }
#endif

  mWidth = rect.Width();
  mHeight = rect.Height();
}

void
TransformReferenceBox::Init(const nsSize& aDimensions)
{
  MOZ_ASSERT(!mFrame && !mIsCached);

  mWidth = aDimensions.width;
  mHeight = aDimensions.height;
  mIsCached = true;
}




static double FlushToZero(double aVal)
{
  if (-FLT_EPSILON < aVal && aVal < FLT_EPSILON)
    return 0.0f;
  else
    return aVal;
}

float
ProcessTranslatePart(const nsCSSValue& aValue,
                     nsStyleContext* aContext,
                     nsPresContext* aPresContext,
                     bool& aCanStoreInRuleTree,
                     TransformReferenceBox* aRefBox,
                     TransformReferenceBox::DimensionGetter aDimensionGetter)
{
  nscoord offset = 0;
  float percent = 0.0f;

  if (aValue.GetUnit() == eCSSUnit_Percent) {
    percent = aValue.GetPercentValue();
  } else if (aValue.GetUnit() == eCSSUnit_Pixel ||
             aValue.GetUnit() == eCSSUnit_Number) {
    
    
    
    
    
    
    
    
    
    return aValue.GetFloatValue();
  } else if (aValue.IsCalcUnit()) {
    nsRuleNode::ComputedCalc result =
      nsRuleNode::SpecifiedCalcToComputedCalc(aValue, aContext, aPresContext,
                                              aCanStoreInRuleTree);
    percent = result.mPercent;
    offset = result.mLength;
  } else {
    offset = nsRuleNode::CalcLength(aValue, aContext, aPresContext,
                                    aCanStoreInRuleTree);
  }

  float translation = NSAppUnitsToFloatPixels(offset,
                                              nsPresContext::AppUnitsPerCSSPixel());
  
  
  if (percent != 0.0f && aRefBox) {
    translation += percent *
                     NSAppUnitsToFloatPixels((aRefBox->*aDimensionGetter)(),
                                             nsPresContext::AppUnitsPerCSSPixel());
  }
  return translation;
}








static void
ProcessMatrix(gfx3DMatrix& aMatrix,
              const nsCSSValue::Array* aData,
              nsStyleContext* aContext,
              nsPresContext* aPresContext,
              bool& aCanStoreInRuleTree,
              TransformReferenceBox& aRefBox)
{
  NS_PRECONDITION(aData->Count() == 7, "Invalid array!");

  gfxMatrix result;

  


  result._11 = aData->Item(1).GetFloatValue();
  result._12 = aData->Item(2).GetFloatValue();
  result._21 = aData->Item(3).GetFloatValue();
  result._22 = aData->Item(4).GetFloatValue();

  


  result._31 = ProcessTranslatePart(aData->Item(5),
                                   aContext, aPresContext, aCanStoreInRuleTree,
                                   &aRefBox, &TransformReferenceBox::Width);
  result._32 = ProcessTranslatePart(aData->Item(6),
                                   aContext, aPresContext, aCanStoreInRuleTree,
                                   &aRefBox, &TransformReferenceBox::Height);

  aMatrix.PreMultiply(result);
}

static void 
ProcessMatrix3D(gfx3DMatrix& aMatrix,
                const nsCSSValue::Array* aData,
                nsStyleContext* aContext,
                nsPresContext* aPresContext,
                bool& aCanStoreInRuleTree,
                TransformReferenceBox& aRefBox)
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
                                  &aRefBox, &TransformReferenceBox::Width);
  temp._42 = ProcessTranslatePart(aData->Item(14),
                                  aContext, aPresContext, aCanStoreInRuleTree,
                                  &aRefBox, &TransformReferenceBox::Height);
  temp._43 = ProcessTranslatePart(aData->Item(15),
                                  aContext, aPresContext, aCanStoreInRuleTree,
                                  &aRefBox, &TransformReferenceBox::Height);
  

  aMatrix.PreMultiply(temp);
}


void
ProcessInterpolateMatrix(gfx3DMatrix& aMatrix,
                         const nsCSSValue::Array* aData,
                         nsStyleContext* aContext,
                         nsPresContext* aPresContext,
                         bool& aCanStoreInRuleTree,
                         TransformReferenceBox& aRefBox)
{
  NS_PRECONDITION(aData->Count() == 4, "Invalid array!");

  gfx3DMatrix matrix1, matrix2;
  if (aData->Item(1).GetUnit() == eCSSUnit_List) {
    matrix1 = nsStyleTransformMatrix::ReadTransforms(aData->Item(1).GetListValue(),
                             aContext, aPresContext,
                             aCanStoreInRuleTree,
                             aRefBox, nsPresContext::AppUnitsPerCSSPixel());
  }
  if (aData->Item(2).GetUnit() == eCSSUnit_List) {
    matrix2 = ReadTransforms(aData->Item(2).GetListValue(),
                             aContext, aPresContext,
                             aCanStoreInRuleTree,
                             aRefBox, nsPresContext::AppUnitsPerCSSPixel());
  }
  double progress = aData->Item(3).GetPercentValue();

  aMatrix =
    StyleAnimationValue::InterpolateTransformMatrix(matrix1, matrix2, progress)
    * aMatrix;
}


static void
ProcessTranslateX(gfx3DMatrix& aMatrix,
                  const nsCSSValue::Array* aData,
                  nsStyleContext* aContext,
                  nsPresContext* aPresContext,
                  bool& aCanStoreInRuleTree,
                  TransformReferenceBox& aRefBox)
{
  NS_PRECONDITION(aData->Count() == 2, "Invalid array!");

  Point3D temp;

  temp.x = ProcessTranslatePart(aData->Item(1),
                                aContext, aPresContext, aCanStoreInRuleTree,
                                &aRefBox, &TransformReferenceBox::Width);
  aMatrix.Translate(temp);
}


static void
ProcessTranslateY(gfx3DMatrix& aMatrix,
                  const nsCSSValue::Array* aData,
                  nsStyleContext* aContext,
                  nsPresContext* aPresContext,
                  bool& aCanStoreInRuleTree,
                  TransformReferenceBox& aRefBox)
{
  NS_PRECONDITION(aData->Count() == 2, "Invalid array!");

  Point3D temp;

  temp.y = ProcessTranslatePart(aData->Item(1),
                                aContext, aPresContext, aCanStoreInRuleTree,
                                &aRefBox, &TransformReferenceBox::Height);
  aMatrix.Translate(temp);
}

static void 
ProcessTranslateZ(gfx3DMatrix& aMatrix,
                  const nsCSSValue::Array* aData,
                  nsStyleContext* aContext,
                  nsPresContext* aPresContext,
                  bool& aCanStoreInRuleTree)
{
  NS_PRECONDITION(aData->Count() == 2, "Invalid array!");

  Point3D temp;

  temp.z = ProcessTranslatePart(aData->Item(1), aContext,
                                aPresContext, aCanStoreInRuleTree,
                                nullptr);
  aMatrix.Translate(temp);
}


static void
ProcessTranslate(gfx3DMatrix& aMatrix,
                 const nsCSSValue::Array* aData,
                 nsStyleContext* aContext,
                 nsPresContext* aPresContext,
                 bool& aCanStoreInRuleTree,
                 TransformReferenceBox& aRefBox)
{
  NS_PRECONDITION(aData->Count() == 2 || aData->Count() == 3, "Invalid array!");

  Point3D temp;

  temp.x = ProcessTranslatePart(aData->Item(1),
                                aContext, aPresContext, aCanStoreInRuleTree,
                                &aRefBox, &TransformReferenceBox::Width);

  
  if (aData->Count() == 3) {
    temp.y = ProcessTranslatePart(aData->Item(2),
                                  aContext, aPresContext, aCanStoreInRuleTree,
                                  &aRefBox, &TransformReferenceBox::Height);
  }
  aMatrix.Translate(temp);
}

static void
ProcessTranslate3D(gfx3DMatrix& aMatrix,
                   const nsCSSValue::Array* aData,
                   nsStyleContext* aContext,
                   nsPresContext* aPresContext,
                   bool& aCanStoreInRuleTree,
                   TransformReferenceBox& aRefBox)
{
  NS_PRECONDITION(aData->Count() == 4, "Invalid array!");

  Point3D temp;

  temp.x = ProcessTranslatePart(aData->Item(1),
                                aContext, aPresContext, aCanStoreInRuleTree,
                                &aRefBox, &TransformReferenceBox::Width);

  temp.y = ProcessTranslatePart(aData->Item(2),
                                aContext, aPresContext, aCanStoreInRuleTree,
                                &aRefBox, &TransformReferenceBox::Height);

  temp.z = ProcessTranslatePart(aData->Item(3),
                                aContext, aPresContext, aCanStoreInRuleTree,
                                nullptr);

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

  Point3D vector(aData->Item(1).GetFloatValue(),
                 aData->Item(2).GetFloatValue(),
                 aData->Item(3).GetFloatValue());

  if (!vector.Length()) {
    return;
  }
  vector.Normalize();

  gfx3DMatrix temp;

  
  temp._11 = 1 + (1 - cosTheta) * (vector.x * vector.x - 1);
  temp._12 = -vector.z * sinTheta + (1 - cosTheta) * vector.x * vector.y;
  temp._13 = vector.y * sinTheta + (1 - cosTheta) * vector.x * vector.z;
  temp._14 = 0.0f;
  temp._21 = vector.z * sinTheta + (1 - cosTheta) * vector.x * vector.y;
  temp._22 = 1 + (1 - cosTheta) * (vector.y * vector.y - 1);
  temp._23 = -vector.x * sinTheta + (1 - cosTheta) * vector.y * vector.z;
  temp._24 = 0.0f;
  temp._31 = -vector.y * sinTheta + (1 - cosTheta) * vector.x * vector.z;
  temp._32 = vector.x * sinTheta + (1 - cosTheta) * vector.y * vector.z;
  temp._33 = 1 + (1 - cosTheta) * (vector.z * vector.z - 1);
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
                   bool &aCanStoreInRuleTree)
{
  NS_PRECONDITION(aData->Count() == 2, "Invalid array!");

  float depth = ProcessTranslatePart(aData->Item(1), aContext,
                                     aPresContext, aCanStoreInRuleTree,
                                     nullptr);
  aMatrix.Perspective(depth);
}






static void
MatrixForTransformFunction(gfx3DMatrix& aMatrix,
                           const nsCSSValue::Array * aData,
                           nsStyleContext* aContext,
                           nsPresContext* aPresContext,
                           bool& aCanStoreInRuleTree,
                           TransformReferenceBox& aRefBox)
{
  NS_PRECONDITION(aData, "Why did you want to get data from a null array?");
  
  
  


  
  switch (TransformFunctionOf(aData)) {
  case eCSSKeyword_translatex:
    ProcessTranslateX(aMatrix, aData, aContext, aPresContext,
                      aCanStoreInRuleTree, aRefBox);
    break;
  case eCSSKeyword_translatey:
    ProcessTranslateY(aMatrix, aData, aContext, aPresContext,
                      aCanStoreInRuleTree, aRefBox);
    break;
  case eCSSKeyword_translatez:
    ProcessTranslateZ(aMatrix, aData, aContext, aPresContext,
                      aCanStoreInRuleTree);
    break;
  case eCSSKeyword_translate:
    ProcessTranslate(aMatrix, aData, aContext, aPresContext,
                     aCanStoreInRuleTree, aRefBox);
    break;
  case eCSSKeyword_translate3d:
    ProcessTranslate3D(aMatrix, aData, aContext, aPresContext,
                       aCanStoreInRuleTree, aRefBox);
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
                  aCanStoreInRuleTree, aRefBox);
    break;
  case eCSSKeyword_matrix3d:
    ProcessMatrix3D(aMatrix, aData, aContext, aPresContext,
                    aCanStoreInRuleTree, aRefBox);
    break;
  case eCSSKeyword_interpolatematrix:
    ProcessInterpolateMatrix(aMatrix, aData, aContext, aPresContext,
                             aCanStoreInRuleTree, aRefBox);
    break;
  case eCSSKeyword_perspective:
    ProcessPerspective(aMatrix, aData, aContext, aPresContext, 
                       aCanStoreInRuleTree);
    break;
  default:
    NS_NOTREACHED("Unknown transform function!");
  }
}





nsCSSKeyword
TransformFunctionOf(const nsCSSValue::Array* aData)
{
  MOZ_ASSERT(aData->Item(0).GetUnit() == eCSSUnit_Enumerated);
  return aData->Item(0).GetKeywordValue();
}

gfx3DMatrix
ReadTransforms(const nsCSSValueList* aList,
               nsStyleContext* aContext,
               nsPresContext* aPresContext,
               bool &aCanStoreInRuleTree,
               TransformReferenceBox& aRefBox,
               float aAppUnitsPerMatrixUnit)
{
  gfx3DMatrix result;

  for (const nsCSSValueList* curr = aList; curr != nullptr; curr = curr->mNext) {
    const nsCSSValue &currElem = curr->mValue;
    if (currElem.GetUnit() != eCSSUnit_Function) {
      NS_ASSERTION(currElem.GetUnit() == eCSSUnit_None &&
                   !aList->mNext,
                   "stream should either be a list of functions or a "
                   "lone None");
      continue;
    }
    NS_ASSERTION(currElem.GetArrayValue()->Count() >= 1,
                 "Incoming function is too short!");

    
    MatrixForTransformFunction(result, currElem.GetArrayValue(), aContext,
                               aPresContext, aCanStoreInRuleTree, aRefBox);
  }

  float scale = float(nsPresContext::AppUnitsPerCSSPixel()) / aAppUnitsPerMatrixUnit;
  result.Scale(1/scale, 1/scale, 1/scale);
  result.ScalePost(scale, scale, scale);
  
  return result;
}

} 
