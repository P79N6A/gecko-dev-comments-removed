















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
#include "mozilla/ReentrantMonitor.h"
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

  
  static NS_METHOD WriteToRasterImage(nsIInputStream* aIn, void* aClosure,
                                      const char* aFromRawSegment,
                                      uint32_t aToOffset, uint32_t aCount,
                                      uint32_t* aWriteCount);

  
  uint32_t GetNumFrames() const { return mFrameCount; }

  virtual size_t SizeOfSourceWithComputedFallback(MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE;
  virtual size_t SizeOfDecoded(gfxMemoryLocation aLocation,
                               MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE;

  
  void Discard();

  

  



  nsresult SetSize(int32_t aWidth, int32_t aHeight, Orientation aOrientation);

  







  RawAccessFrameRef EnsureFrame(uint32_t aFrameNum,
                                const nsIntRect& aFrameRect,
                                uint32_t aDecodeFlags,
                                gfx::SurfaceFormat aFormat,
                                uint8_t aPaletteDepth,
                                imgFrame* aPreviousFrame);

  
  void DecodingComplete(imgFrame* aFinalFrame);

  



  void     SetLoopCount(int32_t aLoopCount);

  







  nsresult AddSourceData(const char *aBuffer, uint32_t aCount);

  virtual nsresult OnImageDataAvailable(nsIRequest* aRequest,
                                        nsISupports* aContext,
                                        nsIInputStream* aInStr,
                                        uint64_t aSourceOffset,
                                        uint32_t aCount) MOZ_OVERRIDE;
  virtual nsresult OnImageDataComplete(nsIRequest* aRequest,
                                       nsISupports* aContext,
                                       nsresult aStatus,
                                       bool aLastPart) MOZ_OVERRIDE;

  static already_AddRefed<nsIEventTarget> GetEventTarget();

  










  nsresult SetSourceSizeHint(uint32_t sizeHint);

  
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

  static void Initialize();

private:
  friend class DecodePool;
  friend class DecodeWorker;
  friend class FrameNeededWorker;
  friend class NotifyProgressWorker;

  nsresult FinishedSomeDecoding(ShutdownReason aReason = ShutdownReason::DONE,
                                Progress aProgress = NoProgress);

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

  RawAccessFrameRef InternalAddFrame(uint32_t aFrameNum,
                                     const nsIntRect& aFrameRect,
                                     uint32_t aDecodeFlags,
                                     gfx::SurfaceFormat aFormat,
                                     uint8_t aPaletteDepth,
                                     imgFrame* aPreviousFrame);
  nsresult DoImageDataComplete();

  already_AddRefed<layers::Image> GetCurrentImage();
  void UpdateImageContainer();

  enum RequestDecodeType {
      ASYNCHRONOUS,
      SYNCHRONOUS_NOTIFY,
      SYNCHRONOUS_NOTIFY_AND_SOME_DECODE
  };
  NS_IMETHOD RequestDecodeCore(RequestDecodeType aDecodeType);

  
  
  
  
  bool IsUnlocked() { return (mLockCount == 0 || (mAnim && mAnimationConsumers == 0)); }

private: 
  nsIntSize                  mSize;
  Orientation                mOrientation;

  
  
  
  
  
  
  
  
  uint32_t                   mFrameDecodeFlags;

  nsCOMPtr<nsIProperties>   mProperties;

  
  
  
  
  UniquePtr<FrameAnimator> mAnim;

  
  uint32_t                   mLockCount;

  
  nsCString                  mSourceDataMimeType;

  
  
  int32_t                        mDecodeCount;

  
  nsIntSize                  mRequestedResolution;

  
  int                        mRequestedSampleSize;

  
  nsRefPtr<layers::ImageContainer> mImageContainer;

  
  WeakPtr<layers::ImageContainer> mImageContainerCache;

#ifdef DEBUG
  uint32_t                       mFramesNotified;
#endif

  
  

  
  ReentrantMonitor           mDecodingMonitor;

  FallibleTArray<char>       mSourceData;

  
  nsRefPtr<Decoder>          mDecoder;
  DecodeStatus               mDecodeStatus;
  

  
  uint32_t                   mFrameCount;

  
  Progress                   mNotifyProgress;
  nsIntRect                  mNotifyInvalidRect;
  bool                       mNotifying:1;

  
  bool                       mHasSize:1;       
  bool                       mDecodeOnDraw:1;  
  bool                       mTransient:1;     
  bool                       mDiscardable:1;   
  bool                       mHasSourceData:1; 

  
  bool                       mDecoded:1;
  bool                       mHasBeenDecoded:1;

  
  
  
  bool                       mPendingAnimation:1;

  
  
  bool                       mAnimationFinished:1;

  
  
  bool                       mWantFullDecode:1;

  
  
  
  bool                       mPendingError:1;

  
  nsresult RequestDecodeIfNeeded(nsresult aStatus, ShutdownReason aReason,
                                 bool aDone, bool aWasSize);
  nsresult WantDecodedFrames(uint32_t aFlags, bool aShouldSyncNotify);
  nsresult SyncDecode();
  nsresult InitDecoder(bool aDoSizeDecode);
  nsresult WriteToDecoder(const char *aBuffer, uint32_t aCount, DecodeStrategy aStrategy);
  nsresult DecodeSomeData(size_t aMaxBytes, DecodeStrategy aStrategy);
  bool     IsDecodeFinished();
  TimeStamp mDrawStartTime;

  
  nsAutoPtr<ProgressTrackerInit> mProgressTrackerInit;

  nsresult ShutdownDecoder(ShutdownReason aReason);


  
  
  

  
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
  bool StoringSourceData() const;

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
