















#ifndef mozilla_imagelib_RasterImage_h_
#define mozilla_imagelib_RasterImage_h_

#include "Image.h"
#include "nsCOMPtr.h"
#include "imgIContainer.h"
#include "nsIProperties.h"
#include "nsTArray.h"
#include "imgFrame.h"
#include "nsThreadUtils.h"
#include "DecodePool.h"
#include "Orientation.h"
#include "nsIObserver.h"
#include "mozilla/Maybe.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/TypedEnum.h"
#include "mozilla/WeakPtr.h"
#include "mozilla/UniquePtr.h"
#ifdef DEBUG
  #include "imgIContainerDebug.h"
#endif

class nsIInputStream;
class nsIThreadPool;
class nsIRequest;

#define NS_RASTERIMAGE_CID \
{ /* 376ff2c1-9bf6-418a-b143-3340c00112f7 */         \
     0x376ff2c1,                                     \
     0x9bf6,                                         \
     0x418a,                                         \
    {0xb1, 0x43, 0x33, 0x40, 0xc0, 0x01, 0x12, 0xf7} \
}







































































namespace mozilla {

namespace layers {
class LayerManager;
class ImageContainer;
class Image;
}

namespace image {

class Decoder;
class FrameAnimator;
class SourceBuffer;

MOZ_BEGIN_ENUM_CLASS(DecodeStrategy, uint8_t)
  ASYNC,
  SYNC_FOR_SMALL_IMAGES,
  SYNC_IF_POSSIBLE
MOZ_END_ENUM_CLASS(DecodeStrategy)

class RasterImage MOZ_FINAL : public ImageResource
                            , public nsIProperties
                            , public SupportsWeakPtr<RasterImage>
#ifdef DEBUG
                            , public imgIContainerDebug
#endif
{
  
  virtual ~RasterImage();

public:
  MOZ_DECLARE_REFCOUNTED_TYPENAME(RasterImage)
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIPROPERTIES
  NS_DECL_IMGICONTAINER
#ifdef DEBUG
  NS_DECL_IMGICONTAINERDEBUG
#endif

  virtual nsresult StartAnimation() MOZ_OVERRIDE;
  virtual nsresult StopAnimation() MOZ_OVERRIDE;

  
  nsresult Init(const char* aMimeType,
                uint32_t aFlags) MOZ_OVERRIDE;

  virtual void OnSurfaceDiscarded() MOZ_OVERRIDE;

  
  static NS_METHOD WriteToSourceBuffer(nsIInputStream* aIn, void* aClosure,
                                       const char* aFromRawSegment,
                                       uint32_t aToOffset, uint32_t aCount,
                                       uint32_t* aWriteCount);

  
  uint32_t GetNumFrames() const { return mFrameCount; }

  virtual size_t SizeOfSourceWithComputedFallback(MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE;
  virtual size_t SizeOfDecoded(gfxMemoryLocation aLocation,
                               MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE;

  
  void Discard();


  
  
  

  void OnAddedFrame(uint32_t aNewFrameCount, const nsIntRect& aNewRefreshArea);

  



  nsresult SetSize(int32_t aWidth, int32_t aHeight, Orientation aOrientation);

  



  void     SetLoopCount(int32_t aLoopCount);

  
  void OnDecodingComplete();

  










  void NotifyProgress(Progress aProgress,
                      const nsIntRect& aInvalidRect = nsIntRect(),
                      uint32_t aFlags = 0);

  




  void FinalizeDecoder(Decoder* aDecoder);


  
  
  

  virtual nsresult OnImageDataAvailable(nsIRequest* aRequest,
                                        nsISupports* aContext,
                                        nsIInputStream* aInStr,
                                        uint64_t aSourceOffset,
                                        uint32_t aCount) MOZ_OVERRIDE;
  virtual nsresult OnImageDataComplete(nsIRequest* aRequest,
                                       nsISupports* aContext,
                                       nsresult aStatus,
                                       bool aLastPart) MOZ_OVERRIDE;

  










  nsresult SetSourceSizeHint(uint32_t aSizeHint);

  
  void SetRequestedResolution(const nsIntSize requestedResolution) {
    mRequestedResolution = requestedResolution;
  }

  nsIntSize GetRequestedResolution() {
    return mRequestedResolution;
  }
  
  void SetRequestedSampleSize(int requestedSampleSize) {
    mRequestedSampleSize = requestedSampleSize;
  }

  int GetRequestedSampleSize() {
    return mRequestedSampleSize;
  }

 nsCString GetURIString() {
    nsCString spec;
    if (GetURI()) {
      GetURI()->GetSpec(spec);
    }
    return spec;
  }

private:
  void DrawWithPreDownscaleIfNeeded(DrawableFrameRef&& aFrameRef,
                                    gfxContext* aContext,
                                    const nsIntSize& aSize,
                                    const ImageRegion& aRegion,
                                    GraphicsFilter aFilter,
                                    uint32_t aFlags);

  TemporaryRef<gfx::SourceSurface> CopyFrame(uint32_t aWhichFrame,
                                             uint32_t aFlags,
                                             bool aShouldSyncNotify = true);
  TemporaryRef<gfx::SourceSurface> GetFrameInternal(uint32_t aWhichFrame,
                                                    uint32_t aFlags,
                                                    bool aShouldSyncNotify = true);

  DrawableFrameRef LookupFrameInternal(uint32_t aFrameNum,
                                       const nsIntSize& aSize,
                                       uint32_t aFlags);
  DrawableFrameRef LookupFrame(uint32_t aFrameNum,
                               const nsIntSize& aSize,
                               uint32_t aFlags,
                               bool aShouldSyncNotify = true);
  uint32_t GetCurrentFrameIndex() const;
  uint32_t GetRequestedFrameIndex(uint32_t aWhichFrame) const;

  nsIntRect GetFirstFrameRect();

  size_t SizeOfDecodedWithComputedFallbackIfHeap(gfxMemoryLocation aLocation,
                                                 MallocSizeOf aMallocSizeOf) const;

  already_AddRefed<layers::Image>
    GetCurrentImage(layers::ImageContainer* aContainer);
  void UpdateImageContainer();

  
  
  
  
  bool IsUnlocked() { return (mLockCount == 0 || (mAnim && mAnimationConsumers == 0)); }


  
  
  

  





  NS_IMETHOD Decode(DecodeStrategy aStrategy,
                    const Maybe<nsIntSize>& aSize,
                    uint32_t aFlags);

  




  already_AddRefed<Decoder> CreateDecoder(const Maybe<nsIntSize>& aSize,
                                          uint32_t aFlags);

  void WantDecodedFrames(const nsIntSize& aSize, uint32_t aFlags,
                         bool aShouldSyncNotify);

private: 
  nsIntSize                  mSize;
  Orientation                mOrientation;

  nsCOMPtr<nsIProperties>   mProperties;

  
  UniquePtr<FrameAnimator> mAnim;

  
  uint32_t                   mLockCount;

  
  nsCString                  mSourceDataMimeType;

  
  
  int32_t                        mDecodeCount;

  
  nsIntSize                  mRequestedResolution;

  
  int                        mRequestedSampleSize;

  
  
  WeakPtr<layers::ImageContainer> mImageContainer;

#ifdef DEBUG
  uint32_t                       mFramesNotified;
#endif

  
  nsRefPtr<SourceBuffer>     mSourceBuffer;

  
  uint32_t                   mFrameCount;

  
  bool                       mHasSize:1;       
  bool                       mDecodeOnDraw:1;  
  bool                       mTransient:1;     
  bool                       mDiscardable:1;   
  bool                       mHasSourceData:1; 
  bool                       mHasBeenDecoded:1; 
  bool                       mDownscaleDuringDecode:1;

  
  
  
  bool                       mPendingAnimation:1;

  
  
  bool                       mAnimationFinished:1;

  
  
  bool                       mWantFullDecode:1;

  TimeStamp mDrawStartTime;

  
  nsAutoPtr<ProgressTrackerInit> mProgressTrackerInit;


  
  
  

  
  void RequestScale(imgFrame* aFrame, uint32_t aFlags, const nsIntSize& aSize);

  
  bool CanScale(GraphicsFilter aFilter, const nsIntSize& aSize, uint32_t aFlags);

  
  void NotifyNewScaledFrame();

  friend class ScaleRunner;


  
  void DoError();

  class HandleErrorWorker : public nsRunnable
  {
  public:
    




    static void DispatchIfNeeded(RasterImage* aImage);

    NS_IMETHOD Run();

  private:
    explicit HandleErrorWorker(RasterImage* aImage);

    nsRefPtr<RasterImage> mImage;
  };

  
  bool CanDiscard();

protected:
  explicit RasterImage(ProgressTracker* aProgressTracker = nullptr,
                       ImageURL* aURI = nullptr);

  bool ShouldAnimate() MOZ_OVERRIDE;

  friend class ImageFactory;
};

inline NS_IMETHODIMP RasterImage::GetAnimationMode(uint16_t *aAnimationMode) {
  return GetAnimationModeInternal(aAnimationMode);
}

} 
} 

#endif 
