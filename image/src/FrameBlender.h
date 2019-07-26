





#ifndef mozilla_imagelib_FrameBlender_h_
#define mozilla_imagelib_FrameBlender_h_

#include "mozilla/MemoryReporting.h"
#include "gfxASurface.h"
#include "FrameSequence.h"
#include "nsCOMPtr.h"

class imgFrame;

namespace mozilla {
namespace image {







class FrameBlender
{
public:

  




  FrameBlender(FrameSequence* aSequenceToUse = nullptr);
  ~FrameBlender();

  bool DoBlend(nsIntRect* aDirtyRect, uint32_t aPrevFrameIndex,
               uint32_t aNextFrameIndex);

  already_AddRefed<FrameSequence> GetFrameSequence();

  



  imgFrame* GetFrame(uint32_t aIndex) const;

  


  imgFrame* RawGetFrame(uint32_t aIndex) const;

  void InsertFrame(uint32_t framenum, imgFrame* aFrame);
  void RemoveFrame(uint32_t framenum);
  imgFrame* SwapFrame(uint32_t framenum, imgFrame* aFrame);
  void ClearFrames();

  
  uint32_t GetNumFrames() const;

  void Discard();

  void SetSize(nsIntSize aSize) { mSize = aSize; }

  size_t SizeOfDecodedWithComputedFallbackIfHeap(gfxMemoryLocation aLocation,
                                                 mozilla::MallocSizeOf aMallocSizeOf) const;

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
    
    int32_t lastCompositedFrameIndex;

    







    FrameDataPair compositingFrame;

    





    FrameDataPair compositingPrevFrame;

    Anim() :
      lastCompositedFrameIndex(-1)
    {}
  };

  void EnsureAnimExists();

  






  static void ClearFrame(uint8_t* aFrameData, const nsIntRect& aFrameRect);

  
  static void ClearFrame(uint8_t* aFrameData, const nsIntRect& aFrameRect, const nsIntRect &aRectToClear);

  
  static bool CopyFrameImage(const uint8_t *aDataSrc, const nsIntRect& aRectSrc,
                             uint8_t *aDataDest, const nsIntRect& aRectDest);

  














  static nsresult DrawFrameTo(const uint8_t *aSrcData, const nsIntRect& aSrcRect,
                              uint32_t aSrcPaletteLength, bool aSrcHasAlpha,
                              uint8_t *aDstPixels, const nsIntRect& aDstRect,
                              FrameBlendMethod aBlendMethod);

private: 
  
  nsRefPtr<FrameSequence> mFrames;
  nsIntSize mSize;
  Anim* mAnim;
};

} 
} 

#endif 
