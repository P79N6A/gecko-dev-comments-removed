















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
#include "DecodeStrategy.h"
#include "DiscardTracker.h"
#include "Orientation.h"
#include "nsIObserver.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/Mutex.h"
#include "mozilla/ReentrantMonitor.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/WeakPtr.h"
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







































































class ScaleRequest;

namespace mozilla {

namespace layers {
class LayerManager;
class ImageContainer;
class Image;
}

namespace image {

class Decoder;
class FrameAnimator;

class RasterImage : public ImageResource
                  , public nsIProperties
                  , public SupportsWeakPtr<RasterImage>
#ifdef DEBUG
                  , public imgIContainerDebug
#endif
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIPROPERTIES
  NS_DECL_IMGICONTAINER
#ifdef DEBUG
  NS_DECL_IMGICONTAINERDEBUG
#endif

  
  virtual ~RasterImage();

  virtual nsresult StartAnimation();
  virtual nsresult StopAnimation();

  
  nsresult Init(const char* aMimeType,
                uint32_t aFlags);
  virtual nsIntRect FrameRect(uint32_t aWhichFrame) MOZ_OVERRIDE;

  
  static NS_METHOD WriteToRasterImage(nsIInputStream* aIn, void* aClosure,
                                      const char* aFromRawSegment,
                                      uint32_t aToOffset, uint32_t aCount,
                                      uint32_t* aWriteCount);

  

  uint32_t GetCurrentFrameIndex();

  
  uint32_t GetNumFrames() const;

  virtual size_t HeapSizeOfSourceWithComputedFallback(mozilla::MallocSizeOf aMallocSizeOf) const;
  virtual size_t HeapSizeOfDecodedWithComputedFallback(mozilla::MallocSizeOf aMallocSizeOf) const;
  virtual size_t NonHeapSizeOfDecoded() const;
  virtual size_t OutOfProcessSizeOfDecoded() const;

  
  void Discard(bool force = false);
  void ForceDiscard() { Discard( true); }

  
  nsresult SetFrameAsNonPremult(uint32_t aFrameNum, bool aIsNonPremult);

  



  nsresult SetSize(int32_t aWidth, int32_t aHeight, Orientation aOrientation);

  





  nsresult EnsureFrame(uint32_t aFramenum, int32_t aX, int32_t aY,
                       int32_t aWidth, int32_t aHeight,
                       gfxImageFormat aFormat,
                       uint8_t aPaletteDepth,
                       uint8_t** imageData,
                       uint32_t* imageLength,
                       uint32_t** paletteData,
                       uint32_t* paletteLength,
                       imgFrame** aFrame);

  



  nsresult EnsureFrame(uint32_t aFramenum, int32_t aX, int32_t aY,
                       int32_t aWidth, int32_t aHeight,
                       gfxImageFormat aFormat,
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

  static already_AddRefed<nsIEventTarget> GetEventTarget() {
    return DecodePool::Singleton()->GetEventTarget();
  }

  










  nsresult SetSourceSizeHint(uint32_t sizeHint);

  
  void SetRequestedResolution(const nsIntSize requestedResolution) {
    mRequestedResolution = requestedResolution;
  }

  nsIntSize GetRequestedResolution() {
    return mRequestedResolution;
  }

 nsCString GetURIString() {
    nsCString spec;
    if (GetURI()) {
      GetURI()->GetSpec(spec);
    }
    return spec;
  }

  
  static void Initialize();

  enum ScaleStatus
  {
    SCALE_INVALID,
    SCALE_PENDING,
    SCALE_DONE
  };

  
  
  
  void ScalingStart(ScaleRequest* request);

  
  
  
  void ScalingDone(ScaleRequest* request, ScaleStatus status);

  
  enum eShutdownIntent {
    eShutdownIntent_Done        = 0,
    eShutdownIntent_NotNeeded   = 1,
    eShutdownIntent_Error       = 2,
    eShutdownIntent_AllCount    = 3
  };

  

private:
  already_AddRefed<imgStatusTracker> CurrentStatusTracker()
  {
    mDecodingMonitor.AssertCurrentThreadIn();
    nsRefPtr<imgStatusTracker> statusTracker;
    statusTracker = mDecodeRequest ? mDecodeRequest->mStatusTracker
                                   : mStatusTracker;
    MOZ_ASSERT(statusTracker);
    return statusTracker.forget();
  }

  nsresult OnImageDataCompleteCore(nsIRequest* aRequest, nsISupports*, nsresult aStatus);

  



  struct DecodeRequest
  {
    DecodeRequest(RasterImage* aImage)
      : mImage(aImage)
      , mBytesToDecode(0)
      , mRequestStatus(REQUEST_INACTIVE)
      , mChunkCount(0)
      , mAllocatedNewFrame(false)
    {
      MOZ_ASSERT(aImage, "aImage cannot be null");
      MOZ_ASSERT(aImage->mStatusTracker,
                 "aImage should have an imgStatusTracker");
      mStatusTracker = aImage->mStatusTracker->CloneForRecording();
    }

    NS_INLINE_DECL_THREADSAFE_REFCOUNTING(DecodeRequest)

    
    
    nsRefPtr<imgStatusTracker> mStatusTracker;

    RasterImage* mImage;

    uint32_t mBytesToDecode;

    enum DecodeRequestStatus
    {
      REQUEST_INACTIVE,
      REQUEST_PENDING,
      REQUEST_ACTIVE,
      REQUEST_WORK_DONE,
      REQUEST_STOPPED
    } mRequestStatus;

    

    TimeDuration mDecodeTime;

    
    int32_t mChunkCount;

    

    bool mAllocatedNewFrame;
  };

  












  class DecodePool : public nsIObserver
  {
  public:
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSIOBSERVER

    static DecodePool* Singleton();

    


    void RequestDecode(RasterImage* aImg);

    



    void DecodeABitOf(RasterImage* aImg, DecodeStrategy aStrategy);

    






    static void StopDecoding(RasterImage* aImg);

    







    nsresult DecodeUntilSizeAvailable(RasterImage* aImg);

    





    already_AddRefed<nsIEventTarget> GetEventTarget();

    virtual ~DecodePool();

  private: 
    static StaticRefPtr<DecodePool> sSingleton;

  private: 
    DecodePool();

    enum DecodeType {
      DECODE_TYPE_UNTIL_TIME,
      DECODE_TYPE_UNTIL_SIZE,
      DECODE_TYPE_UNTIL_DONE_BYTES
    };

    




    nsresult DecodeSomeOfImage(RasterImage* aImg,
                               DecodeStrategy aStrategy,
                               DecodeType aDecodeType = DECODE_TYPE_UNTIL_TIME,
                               uint32_t bytesToDecode = 0);

    

    class DecodeJob : public nsRunnable
    {
    public:
      DecodeJob(DecodeRequest* aRequest, RasterImage* aImg)
        : mRequest(aRequest)
        , mImage(aImg)
      {}

      NS_IMETHOD Run();

    private:
      nsRefPtr<DecodeRequest> mRequest;
      nsRefPtr<RasterImage> mImage;
    };

  private: 

    
    
    
    mozilla::Mutex            mThreadPoolMutex;
    nsCOMPtr<nsIThreadPool>   mThreadPool;
  };

  class DecodeDoneWorker : public nsRunnable
  {
  public:
    






    static void NotifyFinishedSomeDecoding(RasterImage* image, DecodeRequest* request);

    NS_IMETHOD Run();

  private: 
    DecodeDoneWorker(RasterImage* image, DecodeRequest* request);

  private: 

    nsRefPtr<RasterImage> mImage;
    nsRefPtr<DecodeRequest> mRequest;
  };

  class FrameNeededWorker : public nsRunnable
  {
  public:
    






    static void GetNewFrame(RasterImage* image);

    NS_IMETHOD Run();

  private: 
    FrameNeededWorker(RasterImage* image);

  private: 

    nsRefPtr<RasterImage> mImage;
  };

  nsresult FinishedSomeDecoding(eShutdownIntent intent = eShutdownIntent_Done,
                                DecodeRequest* request = nullptr);

  void DrawWithPreDownscaleIfNeeded(imgFrame *aFrame,
                                    gfxContext *aContext,
                                    GraphicsFilter aFilter,
                                    const gfxMatrix &aUserSpaceToImageSpace,
                                    const gfxRect &aFill,
                                    const nsIntRect &aSubimage,
                                    uint32_t aFlags);

  nsresult CopyFrame(uint32_t aWhichFrame,
                     uint32_t aFlags,
                     gfxImageSurface **_retval);

  







  void DeleteImgFrame(uint32_t framenum);

  imgFrame* GetImgFrameNoDecode(uint32_t framenum);
  imgFrame* GetImgFrame(uint32_t framenum);
  imgFrame* GetDrawableImgFrame(uint32_t framenum);
  imgFrame* GetCurrentImgFrame();
  uint32_t GetCurrentImgFrameIndex() const;

  size_t SizeOfDecodedWithComputedFallbackIfHeap(gfxMemoryLocation aLocation,
                                                 mozilla::MallocSizeOf aMallocSizeOf) const;

  void EnsureAnimExists();

  nsresult InternalAddFrameHelper(uint32_t framenum, imgFrame *frame,
                                  uint8_t **imageData, uint32_t *imageLength,
                                  uint32_t **paletteData, uint32_t *paletteLength,
                                  imgFrame** aRetFrame);
  nsresult InternalAddFrame(uint32_t framenum, int32_t aX, int32_t aY, int32_t aWidth, int32_t aHeight,
                            gfxImageFormat aFormat, uint8_t aPaletteDepth,
                            uint8_t **imageData, uint32_t *imageLength,
                            uint32_t **paletteData, uint32_t *paletteLength,
                            imgFrame** aRetFrame);

  nsresult DoImageDataComplete();

  bool ApplyDecodeFlags(uint32_t aNewFlags);

  already_AddRefed<layers::Image> GetCurrentImage();
  void UpdateImageContainer();

  void SetInUpdateImageContainer(bool aInUpdate) { mInUpdateImageContainer = aInUpdate; }
  bool IsInUpdateImageContainer() { return mInUpdateImageContainer; }
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

  
  imgFrame*                  mMultipartDecodedFrame;

  nsCOMPtr<nsIProperties>    mProperties;

  
  
  
  FrameAnimator* mAnim;

  
  uint32_t                   mLockCount;
  DiscardTracker::Node       mDiscardTrackerNode;

  
  nsCString                  mSourceDataMimeType;

  friend class DiscardTracker;

  
  
  int32_t                        mDecodeCount;

  
  nsIntSize                  mRequestedResolution;

  
  nsRefPtr<mozilla::layers::ImageContainer> mImageContainer;

#ifdef DEBUG
  uint32_t                       mFramesNotified;
#endif

  
  

  
  mozilla::ReentrantMonitor  mDecodingMonitor;

  FallibleTArray<char>       mSourceData;

  
  nsRefPtr<Decoder>          mDecoder;
  nsRefPtr<DecodeRequest>    mDecodeRequest;
  uint32_t                   mBytesDecoded;

  bool                       mInDecoder;
  

  
  ImageStatusDiff            mStatusDiff;
  bool                       mNotifying:1;

  
  bool                       mHasSize:1;       
  bool                       mDecodeOnDraw:1;  
  bool                       mMultipart:1;     
  bool                       mDiscardable:1;   
  bool                       mHasSourceData:1; 

  
  bool                       mDecoded:1;
  bool                       mHasBeenDecoded:1;


  
  
  bool                       mAnimationFinished:1;

  
  bool                       mFinishing:1;

  bool                       mInUpdateImageContainer:1;

  
  
  bool                       mWantFullDecode:1;

  
  
  
  bool                       mPendingError:1;

  
  nsresult RequestDecodeIfNeeded(nsresult aStatus,
                                 eShutdownIntent aIntent,
                                 bool aDone,
                                 bool aWasSize);
  nsresult WantDecodedFrames();
  nsresult SyncDecode();
  nsresult InitDecoder(bool aDoSizeDecode);
  nsresult WriteToDecoder(const char *aBuffer, uint32_t aCount, DecodeStrategy aStrategy);
  nsresult DecodeSomeData(uint32_t aMaxBytes, DecodeStrategy aStrategy);
  bool     IsDecodeFinished();
  TimeStamp mDrawStartTime;

  inline bool CanQualityScale(const gfx::Size& scale);
  inline bool CanScale(GraphicsFilter aFilter, gfx::Size aScale, uint32_t aFlags);

  struct ScaleResult
  {
    ScaleResult()
     : status(SCALE_INVALID)
    {}

    gfx::Size scale;
    nsAutoPtr<imgFrame> frame;
    ScaleStatus status;
  };

  ScaleResult mScaleResult;

  
  
  
  ScaleRequest* mScaleRequest;

  
  nsAutoPtr<imgStatusTrackerInit> mStatusTrackerInit;

  nsresult ShutdownDecoder(eShutdownIntent aIntent);

  
  void DoError();

  class HandleErrorWorker : public nsRunnable
  {
  public:
    




    static void DispatchIfNeeded(RasterImage* aImage);

    NS_IMETHOD Run();

  private:
    HandleErrorWorker(RasterImage* aImage);

    nsRefPtr<RasterImage> mImage;
  };

  
  bool CanDiscard();
  bool CanForciblyDiscard();
  bool DiscardingActive();
  bool StoringSourceData() const;

protected:
  RasterImage(imgStatusTracker* aStatusTracker = nullptr,
              ImageURL* aURI = nullptr);

  bool ShouldAnimate();

  friend class ImageFactory;
};

inline NS_IMETHODIMP RasterImage::GetAnimationMode(uint16_t *aAnimationMode) {
  return GetAnimationModeInternal(aAnimationMode);
}







class imgDecodeRequestor : public nsRunnable
{
  public:
    imgDecodeRequestor(RasterImage &aContainer) {
      mContainer = aContainer.asWeakPtr();
    }
    NS_IMETHOD Run() {
      if (mContainer)
        mContainer->StartDecoding();
      return NS_OK;
    }

  private:
    WeakPtr<RasterImage> mContainer;
};

} 
} 

#endif
