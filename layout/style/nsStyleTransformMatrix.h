








































#ifndef nsStyleTransformMatrix_h_
#define nsStyleTransformMatrix_h_

#include "nsCSSValue.h"
#include "gfxMatrix.h"
#include "gfx3DMatrix.h"
#include "nsRect.h"

struct nsCSSValueList;
class nsStyleContext;
class nsPresContext;




class nsStyleTransformMatrix
{
 public:
  



  static nsCSSKeyword TransformFunctionOf(const nsCSSValue::Array* aData);

  















  static gfx3DMatrix MatrixForTransformFunction(const nsCSSValue::Array* aData,
                                                nsStyleContext* aContext,
                                                nsPresContext* aPresContext,
                                                PRBool& aCanStoreInRuleTree,
                                                nsRect& aBounds, 
                                                float aAppUnitsPerMatrixUnit);

  



  static gfx3DMatrix ReadTransforms(const nsCSSValueList* aList,
                                    nsStyleContext* aContext,
                                    nsPresContext* aPresContext,
                                    PRBool &aCanStoreInRuleTree,
                                    nsRect& aBounds,
                                    float aAppUnitsPerMatrixUnit);

 private:
  static gfx3DMatrix ProcessMatrix(const nsCSSValue::Array *aData,
                                   nsStyleContext *aContext,
                                   nsPresContext *aPresContext,
                                   PRBool &aCanStoreInRuleTree,
                                   nsRect& aBounds, float aAppUnitsPerMatrixUnit,
                                   PRBool *aPercentX = nsnull, 
                                   PRBool *aPercentY = nsnull);
  static gfx3DMatrix ProcessMatrix3D(const nsCSSValue::Array *aData,
                                     nsStyleContext *aContext,
                                     nsPresContext *aPresContext,
                                     PRBool &aCanStoreInRuleTree,
                                     nsRect& aBounds, float aAppUnitsPerMatrixUnit,
                                     PRBool *aPercentX = nsnull, 
                                     PRBool *aPercentY = nsnull);
  static gfx3DMatrix ProcessInterpolateMatrix(const nsCSSValue::Array *aData,
                                              nsStyleContext *aContext,
                                              nsPresContext *aPresContext,
                                              PRBool &aCanStoreInRuleTree,
                                              nsRect& aBounds, float aAppUnitsPerMatrixUnit);
  static gfx3DMatrix ProcessTranslateX(const nsCSSValue::Array *aData,
                                       nsStyleContext *aContext,
                                       nsPresContext *aPresContext,
                                       PRBool &aCanStoreInRuleTree,
                                       nsRect& aBounds, float aAppUnitsPerMatrixUnit);
  static gfx3DMatrix ProcessTranslateY(const nsCSSValue::Array *aData,
                                       nsStyleContext *aContext,
                                       nsPresContext *aPresContext,
                                       PRBool &aCanStoreInRuleTree,
                                       nsRect& aBounds, float aAppUnitsPerMatrixUnit);
  static gfx3DMatrix ProcessTranslateZ(const nsCSSValue::Array *aData,
                                       nsStyleContext *aContext,
                                       nsPresContext *aPresContext,
                                       PRBool &aCanStoreInRuleTree,
                                       float aAppUnitsPerMatrixUnit);
  static gfx3DMatrix ProcessTranslate(const nsCSSValue::Array *aData,
                                      nsStyleContext *aContext,
                                      nsPresContext *aPresContext,
                                      PRBool &aCanStoreInRuleTree,
                                      nsRect& aBounds, float aAppUnitsPerMatrixUnit);
  static gfx3DMatrix ProcessTranslate3D(const nsCSSValue::Array *aData,
                                        nsStyleContext *aContext,
                                        nsPresContext *aPresContext,
                                        PRBool &aCanStoreInRuleTree,
                                        nsRect& aBounds, float aAppUnitsPerMatrixUnit);
  static gfx3DMatrix ProcessScaleHelper(float aXScale, float aYScale, float aZScale);
  static gfx3DMatrix ProcessScaleX(const nsCSSValue::Array *aData);
  static gfx3DMatrix ProcessScaleY(const nsCSSValue::Array *aData);
  static gfx3DMatrix ProcessScaleZ(const nsCSSValue::Array *aData);
  static gfx3DMatrix ProcessScale(const nsCSSValue::Array *aData);
  static gfx3DMatrix ProcessScale3D(const nsCSSValue::Array *aData);
  static gfx3DMatrix ProcessSkewHelper(double aXAngle, double aYAngle);
  static gfx3DMatrix ProcessSkewX(const nsCSSValue::Array *aData);
  static gfx3DMatrix ProcessSkewY(const nsCSSValue::Array *aData);
  static gfx3DMatrix ProcessSkew(const nsCSSValue::Array *aData);
  static gfx3DMatrix ProcessRotateX(const nsCSSValue::Array *aData);
  static gfx3DMatrix ProcessRotateY(const nsCSSValue::Array *aData);
  static gfx3DMatrix ProcessRotateZ(const nsCSSValue::Array *aData);
  static gfx3DMatrix ProcessRotate3D(const nsCSSValue::Array *aData);
  static gfx3DMatrix ProcessPerspective(const nsCSSValue::Array *aData,
                                        nsStyleContext *aContext,
                                        nsPresContext *aPresContext,
                                        PRBool &aCanStoreInRuleTree,
                                        float aAppUnitsPerMatrixUnit);
};

#endif
