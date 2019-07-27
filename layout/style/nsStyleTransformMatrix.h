








#ifndef nsStyleTransformMatrix_h_
#define nsStyleTransformMatrix_h_

#include "nsCSSValue.h"
#include "gfx3DMatrix.h"

class nsIFrame;
class nsStyleContext;
class nsPresContext;
struct nsRect;




namespace nsStyleTransformMatrix {

  






















  class MOZ_STACK_CLASS TransformReferenceBox final {
  public:
    typedef nscoord (TransformReferenceBox::*DimensionGetter)();

    explicit TransformReferenceBox()
      : mFrame(nullptr)
      , mIsCached(false)
    {}

    explicit TransformReferenceBox(const nsIFrame* aFrame)
      : mFrame(aFrame)
      , mIsCached(false)
    {
      MOZ_ASSERT(mFrame);
    }

    explicit TransformReferenceBox(const nsIFrame* aFrame,
                                   const nsSize& aFallbackDimensions)
    {
      mFrame = aFrame;
      mIsCached = false;
      if (!mFrame) {
        Init(aFallbackDimensions);
      }
    }

    void Init(const nsIFrame* aFrame) {
      MOZ_ASSERT(!mFrame && !mIsCached);
      mFrame = aFrame;
    }

    void Init(const nsSize& aDimensions);

    nscoord Width() {
      EnsureDimensionsAreCached();
      return mWidth;
    }

    nscoord Height() {
      EnsureDimensionsAreCached();
      return mHeight;
    }

  private:
    
    
    
    TransformReferenceBox(const TransformReferenceBox&) = delete;

    void EnsureDimensionsAreCached();

    const nsIFrame* mFrame;
    nscoord mWidth, mHeight;
    bool mIsCached;
  };

  



  nsCSSKeyword TransformFunctionOf(const nsCSSValue::Array* aData);

  float ProcessTranslatePart(const nsCSSValue& aValue,
                             nsStyleContext* aContext,
                             nsPresContext* aPresContext,
                             bool& aCanStoreInRuleTree,
                             TransformReferenceBox* aRefBox,
                             TransformReferenceBox::DimensionGetter aDimensionGetter = nullptr);

  void
  ProcessInterpolateMatrix(gfx3DMatrix& aMatrix,
                            const nsCSSValue::Array* aData,
                            nsStyleContext* aContext,
                            nsPresContext* aPresContext,
                            bool& aCanStoreInRuleTree,
                            TransformReferenceBox& aBounds);

  















  gfx3DMatrix ReadTransforms(const nsCSSValueList* aList,
                             nsStyleContext* aContext,
                             nsPresContext* aPresContext,
                             bool &aCanStoreInRuleTree,
                             TransformReferenceBox& aBounds,
                             float aAppUnitsPerMatrixUnit);

} 

#endif
