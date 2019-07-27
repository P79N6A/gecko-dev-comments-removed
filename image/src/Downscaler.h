










#ifndef mozilla_image_src_Downscaler_h
#define mozilla_image_src_Downscaler_h

#include "mozilla/UniquePtr.h"
#include "nsRect.h"

#ifdef MOZ_ENABLE_SKIA

namespace skia {
  class ConvolutionFilter1D;
} 

namespace mozilla {
namespace image {















class Downscaler
{
public:
  
  explicit Downscaler(const nsIntSize& aTargetSize);
  ~Downscaler();

  const nsIntSize& OriginalSize() const { return mOriginalSize; }
  const nsIntSize& TargetSize() const { return mTargetSize; }

  










  nsresult BeginFrame(const nsIntSize& aOriginalSize,
                      uint8_t* aOutputBuffer,
                      bool aHasAlpha);

  
  uint8_t* RowBuffer() { return mRowBuffer.get(); }

  
  void CommitRow();

  
  bool HasInvalidation() const;

  
  nsIntRect TakeInvalidRect();

  




  void ResetForNextProgressivePass();

private:
  void DownscaleInputLine();
  void ReleaseWindow();

  nsIntSize mOriginalSize;
  nsIntSize mTargetSize;

  uint8_t* mOutputBuffer;

  UniquePtr<uint8_t[]> mRowBuffer;
  UniquePtr<uint8_t*[]> mWindow;

  UniquePtr<skia::ConvolutionFilter1D> mXFilter;
  UniquePtr<skia::ConvolutionFilter1D> mYFilter;

  int32_t mWindowCapacity;

  int32_t mLinesInBuffer;
  int32_t mPrevInvalidatedLine;
  int32_t mCurrentOutLine;
  int32_t mCurrentInLine;

  bool mHasAlpha;
};

} 
} 


#else







namespace mozilla {
namespace image {

class Downscaler
{
public:
  explicit Downscaler(const nsIntSize&)
  {
    MOZ_RELEASE_ASSERT(false, "Skia is not enabled");
  }

  const nsIntSize& OriginalSize() const { return nsIntSize(); }
  const nsIntSize& TargetSize() const { return nsIntSize(); }

  nsresult BeginFrame(const nsIntSize&, uint8_t*, bool)
  {
    return NS_ERROR_FAILURE;
  }

  uint8_t* RowBuffer() { return nullptr; }
  void CommitRow() { }
  bool HasInvalidation() const { return false; }
  nsIntRect TakeInvalidRect() { return nsIntRect(); }
  void ResetForNextProgressivePass() { }
};


} 
} 

#endif 

#endif 
