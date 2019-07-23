







































#ifndef nsStyleTransformMatrix_h_
#define nsStyleTransformMatrix_h_

#include "nsCSSValue.h"
#include "gfxMatrix.h"
#include "nsRect.h"
















class nsStyleContext;
class nsPresContext;
class nsStyleTransformMatrix
{
 public:
  


  nsStyleTransformMatrix();

  










  gfxMatrix GetThebesMatrix(const nsRect& aBounds, float aFactor) const;

  







  nsStyleTransformMatrix& operator *= (const nsStyleTransformMatrix &aOther);

  








  const nsStyleTransformMatrix
    operator * (const nsStyleTransformMatrix &aOther) const;

  









  void SetToTransformFunction(const nsCSSValue::Array* aData,
                              nsStyleContext* aContext,
                              nsPresContext* aPresContext,
                              PRBool& aCanStoreInRuleTree);

  


  void SetToIdentity();

  









  float GetMainMatrixEntry(PRInt32 aIndex) const
  {
    NS_PRECONDITION(aIndex >= 0 && aIndex < 4, "Index out of bounds!");
    return mMain[aIndex];
  }

  






  nscoord GetXTranslation(const nsRect& aBounds) const;
  nscoord GetYTranslation(const nsRect& aBounds) const;

  





  PRBool operator== (const nsStyleTransformMatrix& aOther) const;
  PRBool operator!= (const nsStyleTransformMatrix& aOther) const
  {
    return !(*this == aOther);
  }

 private:
  












  float mMain[4];
  nscoord mDelta[2];
  float mX[2];
  float mY[2];
};

#endif
