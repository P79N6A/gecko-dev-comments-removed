





#ifndef mozilla_imagelib_FrameBlender_h_
#define mozilla_imagelib_FrameBlender_h_

#include "nsTArray.h"
#include "mozilla/TimeStamp.h"
#include "gfxASurface.h"

class imgFrame;

namespace mozilla {
namespace image {








class FrameBlender
{
public:

  FrameBlender();
  ~FrameBlender();

  bool DoBlend(nsIntRect* aDirtyRect, uint32_t aPrevFrameIndex,
               uint32_t aNextFrameIndex);

  



  imgFrame* GetFrame(uint32_t aIndex) const;

  


  imgFrame* RawGetFrame(uint32_t aIndex) const;

  void InsertFrame(uint32_t framenum, imgFrame* aFrame);
  void RemoveFrame(uint32_t framenum);
  imgFrame* SwapFrame(uint32_t framenum, imgFrame* aFrame);
  void ClearFrames();

  
  uint32_t GetNumFrames() const;

  void Discard();

  void SetSize(nsIntSize aSize) { mSize = aSize; }

  size_t SizeOfDecodedWithComputedFallbackIfHeap(gfxASurface::MemoryLocation aLocation,
                                                 nsMallocSizeOfFun aMallocSizeOf) const;

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

  struct Anim
  {
    
    int32_t                    lastCompositedFrameIndex;

    







    nsAutoPtr<imgFrame>        compositingFrame;

    





    nsAutoPtr<imgFrame>        compositingPrevFrame;

    Anim() :
      lastCompositedFrameIndex(-1)
    {}
  };

  inline void EnsureAnimExists()
  {
    if (!mAnim) {
      
      mAnim = new Anim();
    }
  }

  






  static void ClearFrame(uint8_t* aFrameData, const nsIntRect& aFrameRect);
  static void ClearFrame(imgFrame* aFrame);

  
  static void ClearFrame(uint8_t* aFrameData, const nsIntRect& aFrameRect, const nsIntRect &aRectToClear);
  static void ClearFrame(imgFrame* aFrame, const nsIntRect& aRectToClear);

  
  static bool CopyFrameImage(uint8_t *aDataSrc, const nsIntRect& aRectSrc,
                             uint8_t *aDataDest, const nsIntRect& aRectDest);
  static bool CopyFrameImage(imgFrame* aSrc, imgFrame* aDst);

  














  static nsresult DrawFrameTo(uint8_t *aSrcData, const nsIntRect& aSrcRect,
                              uint32_t aSrcPaletteLength, bool aSrcHasAlpha,
                              uint8_t *aDstPixels, const nsIntRect& aDstRect,
                              FrameBlendMethod aBlendMethod);
  static nsresult DrawFrameTo(imgFrame* aSrc, imgFrame* aDst, const nsIntRect& aSrcRect);

private: 
  
  
  
  
  nsTArray<imgFrame*> mFrames;

  nsIntSize mSize;

  
  
  
  Anim* mAnim;
};

} 
} 

#endif 
