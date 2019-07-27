





#ifndef mozilla_imagelib_FrameSequence_h_
#define mozilla_imagelib_FrameSequence_h_

#include "nsTArray.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/Move.h"
#include "gfxTypes.h"
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
    : mFrameData(nullptr)
  {}

  FrameDataPair(const FrameDataPair& aOther)
    : mFrame(aOther.mFrame)
    , mFrameData(nullptr)
  {}

  FrameDataPair(FrameDataPair&& aOther)
    : mFrame(Move(aOther.mFrame))
    , mFrameData(aOther.mFrameData)
  {
    aOther.mFrameData = nullptr;
  }

  ~FrameDataPair()
  {
    if (mFrameData) {
      mFrame->UnlockImageData();
    }
  }

  FrameDataPair& operator=(const FrameDataPair& aOther)
  {
    if (&aOther != this) {
      mFrame = aOther.mFrame;
      mFrameData = nullptr;
    }
    return *this;
  }

  FrameDataPair& operator=(FrameDataPair&& aOther)
  {
    MOZ_ASSERT(&aOther != this, "Moving to self");
    mFrame = Move(aOther.mFrame);
    mFrameData = aOther.mFrameData;
    aOther.mFrameData = nullptr;
    return *this;
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

  
  
  already_AddRefed<imgFrame> Forget()
  {
    if (mFrameData) {
      mFrame->UnlockImageData();
    }

    mFrameData = nullptr;
    return mFrame.forget();
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

  already_AddRefed<imgFrame> GetFrame() const
  {
    nsRefPtr<imgFrame> frame = mFrame;
    return frame.forget();
  }

  
  
  void SetFrame(imgFrame* frame)
  {
    if (mFrameData) {
      mFrame->UnlockImageData();
    }

    mFrame = frame;
    mFrameData = nullptr;
  }

  imgFrame* operator->() const
  {
    return mFrame.get();
  }

  bool operator==(imgFrame* other) const
  {
    return mFrame == other;
  }

  operator bool() const
  {
    return mFrame != nullptr;
  }

private:
  nsRefPtr<imgFrame> mFrame;
  uint8_t* mFrameData;
};





class FrameSequence
{
  ~FrameSequence();

public:

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(FrameSequence)

  


  const FrameDataPair& GetFrame(uint32_t aIndex) const;

  


  void InsertFrame(uint32_t framenum, imgFrame* aFrame);

  


  void RemoveFrame(uint32_t framenum);

  



  already_AddRefed<imgFrame> SwapFrame(uint32_t framenum, imgFrame* aFrame);

  


  void ClearFrames();

  
  uint32_t GetNumFrames() const;

  size_t SizeOfDecodedWithComputedFallbackIfHeap(gfxMemoryLocation aLocation,
                                                 MallocSizeOf aMallocSizeOf) const;

private: 
  
  nsTArray<FrameDataPair> mFrames;
};

} 
} 

#endif 
