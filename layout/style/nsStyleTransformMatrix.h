








































#ifndef nsStyleTransformMatrix_h_
#define nsStyleTransformMatrix_h_

#include "nsCSSValue.h"
#include "gfxMatrix.h"
#include "gfx3DMatrix.h"
#include "nsRect.h"

struct nsCSSValueList;
class nsStyleContext;
class nsPresContext;




namespace nsStyleTransformMatrix {
  
  



  nsCSSKeyword TransformFunctionOf(const nsCSSValue::Array* aData);

  















  gfx3DMatrix ReadTransforms(const nsCSSValueList* aList,
                             nsStyleContext* aContext,
                             nsPresContext* aPresContext,
                             PRBool &aCanStoreInRuleTree,
                             nsRect& aBounds,
                             float aAppUnitsPerMatrixUnit);

} 

#endif
