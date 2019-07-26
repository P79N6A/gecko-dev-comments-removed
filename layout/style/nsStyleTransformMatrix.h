








#ifndef nsStyleTransformMatrix_h_
#define nsStyleTransformMatrix_h_

#include "nsCSSValue.h"
#include "gfx3DMatrix.h"

class nsStyleContext;
class nsPresContext;
struct nsRect;




namespace nsStyleTransformMatrix {
  
  



  nsCSSKeyword TransformFunctionOf(const nsCSSValue::Array* aData);

  float ProcessTranslatePart(const nsCSSValue& aValue,
                             nsStyleContext* aContext,
                             nsPresContext* aPresContext,
                             bool& aCanStoreInRuleTree,
                             nscoord aSize);

  void
  ProcessInterpolateMatrix(gfx3DMatrix& aMatrix,
                            const nsCSSValue::Array* aData,
                            nsStyleContext* aContext,
                            nsPresContext* aPresContext,
                            bool& aCanStoreInRuleTree,
                            nsRect& aBounds);

  















  gfx3DMatrix ReadTransforms(const nsCSSValueList* aList,
                             nsStyleContext* aContext,
                             nsPresContext* aPresContext,
                             bool &aCanStoreInRuleTree,
                             nsRect& aBounds,
                             float aAppUnitsPerMatrixUnit);

} 

#endif
