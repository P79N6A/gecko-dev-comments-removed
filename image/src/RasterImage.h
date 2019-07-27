















#ifndef mozilla_image_src_RasterImage_h
#define mozilla_image_src_RasterImage_h

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
#include "mozilla/Attributes.h"
#include "mozilla/Maybe.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/Pair.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/WeakPtr.h"
#include "mozilla/UniquePtr.h"
#ifdef DEBUG
  #include "imgIContainerDebug.h"
#endif

class nsIInputStream;
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
class ImageContainer;
class Image;
}

namespace image {

class Decoder;
class FrameAnimator;
class SourceBuffer;





inline MOZ_CONSTEXPR uint32_t
DecodeFlags(uint32_t aFlags)
{
  return aFlags & (imgIContainer::FLAG_DECODE_NO_PREMULTIPLY_ALPHA |
                   imgIContainer::FLAG_DECODE_NO_COLORSPACE_CONVERSION);
}

class RasterImage final : public ImageResource
                        , public nsIProperties
                        , public SupportsWeakPtr<RasterImage>
#ifdef DEBUG
                        , public imgIContainerDebug
#endif
{
  
  virtual ~RasterImage();

public:
  MOZ_DECLARE_WEAKREFERENCE_TYPENAME(RasterImage)
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIPROPERTIES
  NS_DECL_IMGICONTAINER
#ifdef DEBUG
  NS_DECL_IMGICONTAINERDEBUG
#endif

  virtual nsresult StartAnimation() override;
  virtual nsresult StopAnimation() override;

  
  virtual void OnSurfaceDiscarded() override;

  
  static NS_METHOD WriteToSourceBuffer(nsIInputStream* aIn, void* aClosure,
                                       const char* aFromRawSegment,
                                       uint32_t aToOffset, uint32_t aCount,
                                       uint32_t* aWriteCount);

  
  uint32_t GetNumFrames() const { return mFrameCount; }

  virtual size_t SizeOfSourceWithComputedFallback(MallocSizeOf aMallocSizeOf)
    const override;
  virtual void CollectSizeOfSurfaces(nsTArray<SurfaceMemoryCounter>& aCounters,
                                     MallocSizeOf aMallocSizeOf) const override;

  
  void Discard();


  
  
  

  void OnAddedFrame(uint32_t aNewFrameCount, const nsIntRect& aNewRefreshArea);

  



  nsresult SetSize(int32_t aWidth, int32_t aHeight, Orientation aOrientation);

  



  void     SetLoopCount(int32_t aLoopCount);

  
  void OnDecodingComplete();

  










  void NotifyProgress(Progress aProgress,
                      const nsIntRect& aInvalidRect = nsIntRect(),
                      uint32_t aFlags = DECODE_FLAGS_DEFAULT);

  




  void FinalizeDecoder(Decoder* aDecoder);


  
  
  

  virtual nsresult OnImageDataAvailable(nsIRequest* aRequest,
                                        nsISupports* aContext,
                                        nsIInputStream* aInStr,
                                        uint64_t aSourceOffset,
                                        uint32_t aCount) override;
  virtual nsresult OnImageDataComplete(nsIRequest* aRequest,
                                       nsISupports* aContext,
                                       nsresult aStatus,
                                       bool aLastPart) override;

  void NotifyForDecodeOnlyOnDraw();

  










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
  nsresult Init(const char* aMimeType, uint32_t aFlags);

  DrawResult DrawWithPreDownscaleIfNeeded(DrawableFrameRef&& aFrameRef,
                                          gfxContext* aContext,
                                          const nsIntSize& aSize,
                                          const ImageRegion& aRegion,
                                          GraphicsFilter aFilter,
                                          uint32_t aFlags);

  TemporaryRef<gfx::SourceSurface> CopyFrame(uint32_t aWhichFrame,
                                             uint32_t aFlags);

  Pair<DrawResult, RefPtr<gfx::SourceSurface>>
    GetFrameInternal(uint32_t aWhichFrame, uint32_t aFlags);

  DrawableFrameRef LookupFrameInternal(uint32_t aFrameNum,
                                       const gfx::IntSize& aSize,
                                       uint32_t aFlags);
  DrawableFrameRef LookupFrame(uint32_t aFrameNum,
                               const nsIntSize& aSize,
                               uint32_t aFlags);
  uint32_t GetCurrentFrameIndex() const;
  uint32_t GetRequestedFrameIndex(uint32_t aWhichFrame) const;

  nsIntRect GetFirstFrameRect();

  size_t
    SizeOfDecodedWithComputedFallbackIfHeap(gfxMemoryLocation aLocation,
                                            MallocSizeOf aMallocSizeOf) const;

  Pair<DrawResult, nsRefPtr<layers::Image>>
    GetCurrentImage(layers::ImageContainer* aContainer, uint32_t aFlags);

  void UpdateImageContainer();

  
  
  
  
  bool IsUnlocked() {
    return (mLockCount == 0 || (mAnim && mAnimationConsumers == 0));
  }


  
  
  

  





  NS_IMETHOD Decode(const Maybe<nsIntSize>& aSize, uint32_t aFlags);

  




  already_AddRefed<Decoder> CreateDecoder(const Maybe<nsIntSize>& aSize,
                                          uint32_t aFlags);

  




  void RecoverFromLossOfFrames(const nsIntSize& aSize, uint32_t aFlags);

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

  
  
  DrawResult mLastImageContainerDrawResult;

#ifdef DEBUG
  uint32_t                       mFramesNotified;
#endif

  
  nsRefPtr<SourceBuffer>     mSourceBuffer;

  
  uint32_t                   mFrameCount;

  
  bool                       mHasSize:1;       
  bool                       mDecodeOnlyOnDraw:1; 
  bool                       mTransient:1;     
  bool                       mDiscardable:1;   
  bool                       mHasSourceData:1; 
  bool                       mHasBeenDecoded:1; 
  bool                       mDownscaleDuringDecode:1;

  
  
  
  bool                       mPendingAnimation:1;

  
  
  bool                       mAnimationFinished:1;

  
  
  bool                       mWantFullDecode:1;

  TimeStamp mDrawStartTime;


  
  
  

  
  void RequestScale(imgFrame* aFrame, uint32_t aFlags, const nsIntSize& aSize);

  
  bool CanScale(GraphicsFilter aFilter, const nsIntSize& aSize,
                uint32_t aFlags);

  
  
  bool CanDownscaleDuringDecode(const nsIntSize& aSize, uint32_t aFlags);

  
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
  explicit RasterImage(ImageURL* aURI = nullptr);

  bool ShouldAnimate() override;

  friend class ImageFactory;
};

inline NS_IMETHODIMP
RasterImage::GetAnimationMode(uint16_t* aAnimationMode) {
  return GetAnimationModeInternal(aAnimationMode);
}

} 
} 

#endif 
