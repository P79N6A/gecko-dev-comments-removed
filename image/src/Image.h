




#ifndef mozilla_image_src_Image_h
#define mozilla_image_src_Image_h

#include "mozilla/MemoryReporting.h"
#include "mozilla/TimeStamp.h"
#include "gfx2DGlue.h"                
#include "imgIContainer.h"
#include "ImageURL.h"
#include "nsStringFwd.h"
#include "ProgressTracker.h"
#include "SurfaceCache.h"

class nsIRequest;
class nsIInputStream;

namespace mozilla {
namespace image {

class Image;





struct MemoryCounter
{
  MemoryCounter()
    : mSource(0)
    , mDecodedHeap(0)
    , mDecodedNonHeap(0)
  { }

  void SetSource(size_t aCount) { mSource = aCount; }
  size_t Source() const { return mSource; }
  void SetDecodedHeap(size_t aCount) { mDecodedHeap = aCount; }
  size_t DecodedHeap() const { return mDecodedHeap; }
  void SetDecodedNonHeap(size_t aCount) { mDecodedNonHeap = aCount; }
  size_t DecodedNonHeap() const { return mDecodedNonHeap; }

  MemoryCounter& operator+=(const MemoryCounter& aOther)
  {
    mSource += aOther.mSource;
    mDecodedHeap += aOther.mDecodedHeap;
    mDecodedNonHeap += aOther.mDecodedNonHeap;
    return *this;
  }

private:
  size_t mSource;
  size_t mDecodedHeap;
  size_t mDecodedNonHeap;
};

enum class SurfaceMemoryCounterType
{
  NORMAL,
  COMPOSITING,
  COMPOSITING_PREV
};

struct SurfaceMemoryCounter
{
  SurfaceMemoryCounter(const SurfaceKey& aKey,
                       bool aIsLocked,
                       SurfaceMemoryCounterType aType =
                         SurfaceMemoryCounterType::NORMAL)
    : mKey(aKey)
    , mType(aType)
    , mIsLocked(aIsLocked)
  { }

  const SurfaceKey& Key() const { return mKey; }
  Maybe<gfx::IntSize>& SubframeSize() { return mSubframeSize; }
  const Maybe<gfx::IntSize>& SubframeSize() const { return mSubframeSize; }
  MemoryCounter& Values() { return mValues; }
  const MemoryCounter& Values() const { return mValues; }
  SurfaceMemoryCounterType Type() const { return mType; }
  bool IsLocked() const { return mIsLocked; }

private:
  const SurfaceKey mKey;
  Maybe<gfx::IntSize> mSubframeSize;
  MemoryCounter mValues;
  const SurfaceMemoryCounterType mType;
  const bool mIsLocked;
};

struct ImageMemoryCounter
{
  ImageMemoryCounter(Image* aImage,
                     MallocSizeOf aMallocSizeOf,
                     bool aIsUsed);

  nsCString& URI() { return mURI; }
  const nsCString& URI() const { return mURI; }
  const nsTArray<SurfaceMemoryCounter>& Surfaces() const { return mSurfaces; }
  const gfx::IntSize IntrinsicSize() const { return mIntrinsicSize; }
  const MemoryCounter& Values() const { return mValues; }
  uint16_t Type() const { return mType; }
  bool IsUsed() const { return mIsUsed; }

  bool IsNotable() const
  {
    const size_t NotableThreshold = 16 * 1024;
    size_t total = mValues.Source() + mValues.DecodedHeap()
                                    + mValues.DecodedNonHeap();
    return total >= NotableThreshold;
  }

private:
  nsCString mURI;
  nsTArray<SurfaceMemoryCounter> mSurfaces;
  gfx::IntSize mIntrinsicSize;
  MemoryCounter mValues;
  uint16_t mType;
  const bool mIsUsed;
};






class Image : public imgIContainer
{
public:
  
  enum eDecoderType {
    eDecoderType_png     = 0,
    eDecoderType_gif     = 1,
    eDecoderType_jpeg    = 2,
    eDecoderType_bmp     = 3,
    eDecoderType_ico     = 4,
    eDecoderType_icon    = 5,
    eDecoderType_unknown = 6
  };
  static eDecoderType GetDecoderType(const char* aMimeType);

  
























  static const uint32_t INIT_FLAG_NONE                     = 0x0;
  static const uint32_t INIT_FLAG_DISCARDABLE              = 0x1;
  static const uint32_t INIT_FLAG_DECODE_ONLY_ON_DRAW      = 0x2;
  static const uint32_t INIT_FLAG_DECODE_IMMEDIATELY       = 0x4;
  static const uint32_t INIT_FLAG_TRANSIENT                = 0x8;
  static const uint32_t INIT_FLAG_DOWNSCALE_DURING_DECODE  = 0x10;

  





  virtual nsresult Init(const char* aMimeType,
                        uint32_t aFlags) = 0;

  virtual already_AddRefed<ProgressTracker> GetProgressTracker() = 0;
  virtual void SetProgressTracker(ProgressTracker* aProgressTracker) {}

  




  virtual size_t
    SizeOfSourceWithComputedFallback(MallocSizeOf aMallocSizeOf) const = 0;

  




  virtual void CollectSizeOfSurfaces(nsTArray<SurfaceMemoryCounter>& aCounters,
                                     MallocSizeOf aMallocSizeOf) const = 0;

  virtual void IncrementAnimationConsumers() = 0;
  virtual void DecrementAnimationConsumers() = 0;
#ifdef DEBUG
  virtual uint32_t GetAnimationConsumers() = 0;
#endif

  










  virtual nsresult OnImageDataAvailable(nsIRequest* aRequest,
                                        nsISupports* aContext,
                                        nsIInputStream* aInStr,
                                        uint64_t aSourceOffset,
                                        uint32_t aCount) = 0;

  







  virtual nsresult OnImageDataComplete(nsIRequest* aRequest,
                                       nsISupports* aContext,
                                       nsresult aStatus,
                                       bool aLastPart) = 0;

  



  virtual void OnSurfaceDiscarded() = 0;

  virtual void SetInnerWindowID(uint64_t aInnerWindowId) = 0;
  virtual uint64_t InnerWindowID() const = 0;

  virtual bool HasError() = 0;
  virtual void SetHasError() = 0;

  virtual ImageURL* GetURI() = 0;
};

class ImageResource : public Image
{
public:
  already_AddRefed<ProgressTracker> GetProgressTracker() override
  {
    nsRefPtr<ProgressTracker> progressTracker = mProgressTracker;
    MOZ_ASSERT(progressTracker);
    return progressTracker.forget();
  }

  void SetProgressTracker(
                       ProgressTracker* aProgressTracker) override final
  {
    MOZ_ASSERT(aProgressTracker);
    MOZ_ASSERT(!mProgressTracker);
    mProgressTracker = aProgressTracker;
  }

  virtual void IncrementAnimationConsumers() override;
  virtual void DecrementAnimationConsumers() override;
#ifdef DEBUG
  virtual uint32_t GetAnimationConsumers() override
  {
    return mAnimationConsumers;
  }
#endif

  virtual void OnSurfaceDiscarded() override { }

  virtual void SetInnerWindowID(uint64_t aInnerWindowId) override
  {
    mInnerWindowId = aInnerWindowId;
  }
  virtual uint64_t InnerWindowID() const override { return mInnerWindowId; }

  virtual bool HasError() override    { return mError; }
  virtual void SetHasError() override { mError = true; }

  



  virtual ImageURL* GetURI() override { return mURI.get(); }

protected:
  explicit ImageResource(ImageURL* aURI);

  
  
  nsresult GetAnimationModeInternal(uint16_t* aAnimationMode);
  nsresult SetAnimationModeInternal(uint16_t aAnimationMode);

  








  bool HadRecentRefresh(const TimeStamp& aTime);

  



  virtual void EvaluateAnimation();

  



  virtual bool ShouldAnimate() {
    return mAnimationConsumers > 0 && mAnimationMode != kDontAnimMode;
  }

  virtual nsresult StartAnimation() = 0;
  virtual nsresult StopAnimation() = 0;

  
  nsRefPtr<ProgressTracker>     mProgressTracker;
  nsRefPtr<ImageURL>            mURI;
  TimeStamp                     mLastRefreshTime;
  uint64_t                      mInnerWindowId;
  uint32_t                      mAnimationConsumers;
  uint16_t                      mAnimationMode; 
  bool                          mInitialized:1; 
  bool                          mAnimating:1;   
  bool                          mError:1;       
};

} 
} 

#endif 
