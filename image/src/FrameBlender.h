





#ifndef mozilla_imagelib_FrameBlender_h_
#define mozilla_imagelib_FrameBlender_h_

#include "mozilla/MemoryReporting.h"
#include "gfx2DGlue.h"
#include "gfxTypes.h"
#include "imgFrame.h"
#include "nsCOMPtr.h"
#include "SurfaceCache.h"

namespace mozilla {
namespace image {







class FrameBlender
{
public:
  FrameBlender(ImageKey aImageKey, gfx::IntSize aSize)
   : mImageKey(aImageKey)
   , mSize(aSize)
   , mLastCompositedFrameIndex(-1)
   , mLoopCount(-1)
  { }

  bool DoBlend(nsIntRect* aDirtyRect, uint32_t aPrevFrameIndex,
               uint32_t aNextFrameIndex);

  




  DrawableFrameRef GetCompositedFrame(uint32_t aFrameNum);

  


  RawAccessFrameRef GetRawFrame(uint32_t aFrameNum);

  





  int32_t GetTimeoutForFrame(uint32_t aFrameNum);

  



  void SetLoopCount(int32_t aLoopCount) { mLoopCount = aLoopCount; }
  int32_t LoopCount() const { return mLoopCount; }

  size_t SizeOfDecoded(gfxMemoryLocation aLocation,
                       MallocSizeOf aMallocSizeOf) const;

  void ResetAnimation();

  
  
  enum FrameBlendMethod {
    
    
    kBlendSource =  0,

    
    
    kBlendOver
  };

  enum FrameDisposalMethod {
    kDisposeClearAll         = -1, 
                                   
    kDisposeNotSpecified,   
    kDisposeKeep,           
    kDisposeClear,          
    kDisposeRestorePrevious 
  };

  
  
  enum FrameAlpha {
    kFrameHasAlpha,
    kFrameOpaque
  };

private:
  






  static void ClearFrame(uint8_t* aFrameData, const nsIntRect& aFrameRect);

  
  static void ClearFrame(uint8_t* aFrameData, const nsIntRect& aFrameRect,
                         const nsIntRect& aRectToClear);

  
  static bool CopyFrameImage(const uint8_t* aDataSrc, const nsIntRect& aRectSrc,
                             uint8_t* aDataDest, const nsIntRect& aRectDest);

  















  static nsresult DrawFrameTo(const uint8_t* aSrcData,
                              const nsIntRect& aSrcRect,
                              uint32_t aSrcPaletteLength, bool aSrcHasAlpha,
                              uint8_t* aDstPixels, const nsIntRect& aDstRect,
                              FrameBlendMethod aBlendMethod);

private: 
  ImageKey mImageKey;
  gfx::IntSize mSize;

  
  int32_t mLastCompositedFrameIndex;

  







  RawAccessFrameRef mCompositingFrame;

  





  RawAccessFrameRef mCompositingPrevFrame;

  int32_t mLoopCount;
};

} 
} 

#endif 
