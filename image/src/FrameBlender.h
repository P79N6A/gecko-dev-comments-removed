





#ifndef mozilla_imagelib_FrameBlender_h_
#define mozilla_imagelib_FrameBlender_h_

#include "nsTArray.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/TimeStamp.h"
#include "gfxASurface.h"
#include "imgFrame.h"

namespace mozilla {
namespace image {









class FrameDataPair
{
public:
  explicit FrameDataPair(imgFrame* frame)
    : mFrame(frame)
    , mFrameData(nullptr)
  {}

  FrameDataPair()
    : mFrame(nullptr)
    , mFrameData(nullptr)
  {}

  FrameDataPair(FrameDataPair& other)
  {
    mFrame = other.mFrame;
    mFrameData = other.mFrameData;

    
    
    
    other.mFrameData = nullptr;
  }

  ~FrameDataPair()
  {
    if (mFrameData) {
      mFrame->UnlockImageData();
    }
  }

  
  
  void LockAndGetData()
  {
    if (mFrame) {
      if (NS_SUCCEEDED(mFrame->LockImageData())) {
        if (mFrame->GetIsPaletted()) {
          mFrameData = reinterpret_cast<uint8_t*>(mFrame->GetPaletteData());
        } else {
          mFrameData = mFrame->GetImageData();
        }
      }
    }
  }

  
  
  imgFrame* Forget()
  {
    if (mFrameData) {
      mFrame->UnlockImageData();
    }

    imgFrame* frame = mFrame.forget();
    mFrameData = nullptr;
    return frame;
  }

  bool HasFrameData() const
  {
    if (mFrameData) {
      MOZ_ASSERT(!!mFrame);
    }
    return !!mFrameData;
  }

  uint8_t* GetFrameData() const
  {
    return mFrameData;
  }

  imgFrame* GetFrame() const
  {
    return mFrame;
  }

  
  
  void SetFrame(imgFrame* frame)
  {
    if (mFrameData) {
      mFrame->UnlockImageData();
    }

    mFrame = frame;
    mFrameData = nullptr;
  }

  operator imgFrame*() const
  {
    return GetFrame();
  }

  imgFrame* operator->() const
  {
    return GetFrame();
  }

  bool operator==(imgFrame* other) const
  {
    return mFrame == other;
  }

private:
  nsAutoPtr<imgFrame> mFrame;
  uint8_t* mFrameData;
};







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
  
  nsTArray<FrameDataPair> mFrames;
  nsIntSize mSize;
  Anim* mAnim;
};

} 
} 

#endif 
