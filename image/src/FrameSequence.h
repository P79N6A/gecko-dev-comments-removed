





#ifndef mozilla_imagelib_FrameSequence_h_
#define mozilla_imagelib_FrameSequence_h_

#include "nsTArray.h"
#include "mozilla/MemoryReporting.h"
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





class FrameSequence
{
public:

  ~FrameSequence();

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(FrameSequence)

  


  const FrameDataPair& GetFrame(uint32_t aIndex) const;

  


  void InsertFrame(uint32_t framenum, imgFrame* aFrame);

  


  void RemoveFrame(uint32_t framenum);

  



  imgFrame* SwapFrame(uint32_t framenum, imgFrame* aFrame);

  


  void ClearFrames();

  
  uint32_t GetNumFrames() const;

  size_t SizeOfDecodedWithComputedFallbackIfHeap(gfxMemoryLocation aLocation,
                                                 mozilla::MallocSizeOf aMallocSizeOf) const;

private: 
  
  nsTArray<FrameDataPair> mFrames;
};

} 
} 

#endif 
