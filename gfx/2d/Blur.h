



#ifndef MOZILLA_GFX_BLUR_H_
#define MOZILLA_GFX_BLUR_H_

#include "mozilla/gfx/Rect.h"
#include "mozilla/gfx/Point.h"

namespace mozilla {
namespace gfx {



















class GFX2D_API AlphaBoxBlur
{
public:

  

















  AlphaBoxBlur(const Rect& aRect,
               const IntSize& aSpreadRadius,
               const IntSize& aBlurRadius,
               const Rect* aDirtyRect,
               const Rect* aSkipRect);

  AlphaBoxBlur(uint8_t* aData,
               const Rect& aRect,
               int32_t aStride,
               float aSigma);

  ~AlphaBoxBlur();

  




  unsigned char* GetData();

  



  IntSize GetSize();

  



  int32_t GetStride();

  


  IntRect GetRect();

  



  Rect* GetDirtyRect();

  



  void Blur();

  





  static IntSize CalculateBlurRadius(const Point& aStandardDeviation);

private:

  



  IntRect mSkipRect;

  


  IntRect mRect;

  



  Rect mDirtyRect;

  


  IntSize mSpreadRadius;

  


  IntSize mBlurRadius;

  


  uint8_t* mData;

  


  bool mFreeData;

  


  int32_t mStride;

  


  bool mHasDirtyRect;
};

}
}

#endif 
