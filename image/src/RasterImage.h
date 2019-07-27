















#ifndef mozilla_imagelib_RasterImage_h_
#define mozilla_imagelib_RasterImage_h_

#include "Image.h"
#include "FrameBlender.h"
#include "nsCOMPtr.h"
#include "imgIContainer.h"
#include "nsIProperties.h"
#include "nsTArray.h"
#include "imgFrame.h"
#include "nsThreadUtils.h"
#include "DecodePool.h"
#include "DiscardTracker.h"
#include "Orientation.h"
#include "nsIObserver.h"
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

  virtual nsresult StartAnimation();
  virtual nsresult StopAnimation();

  
  nsresult Init(const char* aMimeType,
                uint32_t aFlags);
  virtual nsIntRect FrameRect(uint32_t aWhichFrame) MOZ_OVERRIDE;

  
  static NS_METHOD WriteToRasterImage(nsIInputStream* aIn, void* aClosure,
                                      const char* aFromRawSegment,
                                      uint32_t aToOffset, uint32_t aCount,
                                      uint32_t* aWriteCount);

  
  uint32_t GetNumFrames() const;

  virtual size_t HeapSizeOfSourceWithComputedFallback(MallocSizeOf aMallocSizeOf) const;
  virtual size_t HeapSizeOfDecodedWithComputedFallback(MallocSizeOf aMallocSizeOf) const;
  virtual size_t NonHeapSizeOfDecoded() const;
  virtual size_t OutOfProcessSizeOfDecoded() const;

  virtual size_t HeapSizeOfVectorImageDocument(nsACString* aDocURL = nullptr) const MOZ_OVERRIDE {
    return 0;
  }

  
  void Discard(bool force = false);
  void ForceDiscard() { Discard( true); }

  
  nsresult SetFrameAsNonPremult(uint32_t aFrameNum, bool aIsNonPremult);

  



  nsresult SetSize(int32_t aWidth, int32_t aHeight, Orientation aOrientation);

  





  nsresult EnsureFrame(uint32_t aFramenum, int32_t aX, int32_t aY,
                       int32_t aWidth, int32_t aHeight,
                       gfx::SurfaceFormat aFormat,
                       uint8_t aPaletteDepth,
                       uint8_t** imageData,
                       uint32_t* imageLength,
                       uint32_t** paletteData,
                       uint32_t* paletteLength,
                       imgFrame** aFrame);

  



  nsresult EnsureFrame(uint32_t aFramenum, int32_t aX, int32_t aY,
                       int32_t aWidth, int32_t aHeight,
                       gfx::SurfaceFormat aFormat,
                       uint8_t** imageData,
                       uint32_t* imageLength,
                       imgFrame** aFrame);

  
  nsresult DecodingComplete();

  



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
  virtual nsresult OnNewSourceData() MOZ_OVERRIDE;

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

  already_AddRefed<imgFrame> LookupFrameNoDecode(uint32_t aFrameNum);
  DrawableFrameRef LookupFrame(uint32_t aFrameNum, uint32_t aFlags, bool aShouldSyncNotify = true);
  uint32_t GetCurrentFrameIndex() const;
  uint32_t GetRequestedFrameIndex(uint32_t aWhichFrame) const;

  nsIntRect GetFirstFrameRect();

  size_t SizeOfDecodedWithComputedFallbackIfHeap(gfxMemoryLocation aLocation,
                                                 MallocSizeOf aMallocSizeOf) const;

  void EnsureAnimExists();

  nsresult InternalAddFrameHelper(uint32_t framenum, imgFrame *frame,
                                  uint8_t **imageData, uint32_t *imageLength,
                                  uint32_t **paletteData, uint32_t *paletteLength,
                                  imgFrame** aRetFrame);
  nsresult InternalAddFrame(uint32_t framenum, int32_t aX, int32_t aY, int32_t aWidth, int32_t aHeight,
                            gfx::SurfaceFormat aFormat, uint8_t aPaletteDepth,
                            uint8_t **imageData, uint32_t *imageLength,
                            uint32_t **paletteData, uint32_t *paletteLength,
                            imgFrame** aRetFrame);

  nsresult DoImageDataComplete();

  bool ApplyDecodeFlags(uint32_t aNewFlags, uint32_t aWhichFrame);

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

  
  FrameBlender              mFrameBlender;

  
  nsRefPtr<imgFrame>        mMultipartDecodedFrame;

  nsCOMPtr<nsIProperties>   mProperties;

  
  
  
  UniquePtr<FrameAnimator> mAnim;

  
  uint32_t                   mLockCount;
  DiscardTracker::Node       mDiscardTrackerNode;

  
  nsCString                  mSourceDataMimeType;

  friend class DiscardTracker;

  
  
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
  

  
  Progress                   mNotifyProgress;
  nsIntRect                  mNotifyInvalidRect;
  bool                       mNotifying:1;

  
  bool                       mHasSize:1;       
  bool                       mDecodeOnDraw:1;  
  bool                       mMultipart:1;     
  bool                       mDiscardable:1;   
  bool                       mHasSourceData:1; 

  
  bool                       mDecoded:1;
  bool                       mHasBeenDecoded:1;


  
  
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
  bool CanForciblyDiscard();
  bool CanForciblyDiscardAndRedecode();
  bool DiscardingActive();
  bool StoringSourceData() const;

protected:
  explicit RasterImage(ProgressTracker* aProgressTracker = nullptr,
                       ImageURL* aURI = nullptr);

  bool ShouldAnimate();

  friend class ImageFactory;
};

inline NS_IMETHODIMP RasterImage::GetAnimationMode(uint16_t *aAnimationMode) {
  return GetAnimationModeInternal(aAnimationMode);
}

} 
} 

#endif 
