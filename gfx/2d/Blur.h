





#ifndef MOZILLA_GFX_BLUR_H_
#define MOZILLA_GFX_BLUR_H_

#include "mozilla/gfx/Rect.h"
#include "mozilla/gfx/Point.h"
#include "mozilla/CheckedInt.h"

namespace mozilla {
namespace gfx {

#ifdef _MSC_VER
#pragma warning( disable : 4251 )
#endif



















class GFX2D_API AlphaBoxBlur
{
public:

  

















  AlphaBoxBlur(const Rect& aRect,
               const IntSize& aSpreadRadius,
               const IntSize& aBlurRadius,
               const Rect* aDirtyRect,
               const Rect* aSkipRect);

  AlphaBoxBlur(const Rect& aRect,
               int32_t aStride,
               float aSigmaX,
               float aSigmaY);

  ~AlphaBoxBlur();

  


  IntSize GetSize();

  


  int32_t GetStride();

  


  IntRect GetRect();

  



  Rect* GetDirtyRect();

  






  size_t GetSurfaceAllocationSize() const;

  




  void Blur(uint8_t* aData);

  





  static IntSize CalculateBlurRadius(const Point& aStandardDeviation);

private:

  void BoxBlur_C(uint8_t* aData,
                 int32_t aLeftLobe, int32_t aRightLobe, int32_t aTopLobe,
                 int32_t aBottomLobe, uint32_t *aIntegralImage, size_t aIntegralImageStride);
  void BoxBlur_SSE2(uint8_t* aData,
                    int32_t aLeftLobe, int32_t aRightLobe, int32_t aTopLobe,
                    int32_t aBottomLobe, uint32_t *aIntegralImage, size_t aIntegralImageStride);

  static CheckedInt<int32_t> RoundUpToMultipleOf4(int32_t aVal);

  



  IntRect mSkipRect;

  


  IntRect mRect;

  



  Rect mDirtyRect;

  


  IntSize mSpreadRadius;

  


  IntSize mBlurRadius;

  


  int32_t mStride;

  


  size_t mSurfaceAllocationSize;

  


  bool mHasDirtyRect;
};

}
}

#endif 
