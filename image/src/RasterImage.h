















#ifndef mozilla_imagelib_RasterImage_h_
#define mozilla_imagelib_RasterImage_h_

#include "Image.h"
#include "nsCOMArray.h"
#include "nsCOMPtr.h"
#include "imgIContainer.h"
#include "nsIProperties.h"
#include "nsITimer.h"
#include "nsIRequest.h"
#include "nsTArray.h"
#include "imgFrame.h"
#include "nsThreadUtils.h"
#include "DiscardTracker.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/Telemetry.h"
#include "mozilla/LinkedList.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/WeakPtr.h"
#include "gfx2DGlue.h"
#ifdef DEBUG
  #include "imgIContainerDebug.h"
#endif

class nsIInputStream;

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

class RasterImage : public Image
                  , public nsIProperties
                  , public SupportsWeakPtr<RasterImage>
#ifdef DEBUG
                  , public imgIContainerDebug
#endif
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPROPERTIES
  NS_DECL_IMGICONTAINER
#ifdef DEBUG
  NS_DECL_IMGICONTAINERDEBUG
#endif

  
  virtual ~RasterImage();

  virtual nsresult StartAnimation();
  virtual nsresult StopAnimation();

  
  nsresult Init(imgDecoderObserver* aObserver,
                const char* aMimeType,
                const char* aURIString,
                uint32_t aFlags);
  virtual void  GetCurrentFrameRect(nsIntRect& aRect) MOZ_OVERRIDE;

  
  static NS_METHOD WriteToRasterImage(nsIInputStream* aIn, void* aClosure,
                                      const char* aFromRawSegment,
                                      uint32_t aToOffset, uint32_t aCount,
                                      uint32_t* aWriteCount);

  

  uint32_t GetCurrentFrameIndex();

  
  uint32_t GetNumFrames();

  virtual size_t HeapSizeOfSourceWithComputedFallback(nsMallocSizeOfFun aMallocSizeOf) const;
  virtual size_t HeapSizeOfDecodedWithComputedFallback(nsMallocSizeOfFun aMallocSizeOf) const;
  virtual size_t NonHeapSizeOfDecoded() const;
  virtual size_t OutOfProcessSizeOfDecoded() const;

  
  void Discard(bool force = false);
  void ForceDiscard() { Discard( true); }

  
  nsresult SetFrameDisposalMethod(uint32_t aFrameNum,
                                  int32_t aDisposalMethod);
  nsresult SetFrameTimeout(uint32_t aFrameNum, int32_t aTimeout);
  nsresult SetFrameBlendMethod(uint32_t aFrameNum, int32_t aBlendMethod);
  nsresult SetFrameHasNoAlpha(uint32_t aFrameNum);
  nsresult SetFrameAsNonPremult(uint32_t aFrameNum, bool aIsNonPremult);

  




  nsresult SetSize(int32_t aWidth, int32_t aHeight);


  





  nsresult EnsureFrame(uint32_t aFramenum, int32_t aX, int32_t aY,
                       int32_t aWidth, int32_t aHeight,
                       gfxASurface::gfxImageFormat aFormat,
                       uint8_t aPaletteDepth,
                       uint8_t** imageData,
                       uint32_t* imageLength,
                       uint32_t** paletteData,
                       uint32_t* paletteLength);

  



  nsresult EnsureFrame(uint32_t aFramenum, int32_t aX, int32_t aY,
                       int32_t aWidth, int32_t aHeight,
                       gfxASurface::gfxImageFormat aFormat,
                       uint8_t** imageData,
                       uint32_t* imageLength);

  void FrameUpdated(uint32_t aFrameNum, nsIntRect& aUpdatedRect);

  
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
                                       nsresult aResult) MOZ_OVERRIDE;
  virtual nsresult OnNewSourceData() MOZ_OVERRIDE;

  










  nsresult SetSourceSizeHint(uint32_t sizeHint);

  
  
  enum {
    
    
    kBlendSource =  0,

    
    
    kBlendOver
  };

  enum {
    kDisposeClearAll         = -1, 
                                   
    kDisposeNotSpecified,   
    kDisposeKeep,           
    kDisposeClear,          
    kDisposeRestorePrevious 
  };

  const char* GetURIString() { return mURIString.get();}

  
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
    eShutdownIntent_Interrupted = 1,
    eShutdownIntent_Error       = 2,
    eShutdownIntent_AllCount    = 3
  };

private:
  struct Anim
  {
    
    nsIntRect                  firstFrameRefreshArea;
    uint32_t                   currentAnimationFrameIndex; 

    
    TimeStamp                  currentAnimationFrameTime;

    
    int32_t                    lastCompositedFrameIndex;
    







    nsAutoPtr<imgFrame>        compositingFrame;
    





    nsAutoPtr<imgFrame>        compositingPrevFrame;

    Anim() :
      firstFrameRefreshArea(),
      currentAnimationFrameIndex(0),
      lastCompositedFrameIndex(-1) {}
    ~Anim() {}
  };

  





  struct DecodeRequest : public LinkedListElement<DecodeRequest>
  {
    DecodeRequest(RasterImage* aImage)
      : mImage(aImage)
      , mIsASAP(false)
    {
    }

    RasterImage* const mImage;

    

    TimeDuration mDecodeTime;

    
    bool mIsASAP;
  };

  
















  class DecodeWorker : public nsRunnable
  {
  public:
    static DecodeWorker* Singleton();

    


    void RequestDecode(RasterImage* aImg);

    



    void DecodeABitOf(RasterImage* aImg);

    







    void MarkAsASAP(RasterImage* aImg);

    






    void StopDecoding(RasterImage* aImg);

    







    nsresult DecodeUntilSizeAvailable(RasterImage* aImg);

    NS_IMETHOD Run();

  private: 
    static StaticRefPtr<DecodeWorker> sSingleton;

  private: 
    DecodeWorker()
      : mPendingInEventLoop(false)
    {}

    
    void EnsurePendingInEventLoop();

    

    void AddDecodeRequest(DecodeRequest* aRequest);

    enum DecodeType {
      DECODE_TYPE_NORMAL,
      DECODE_TYPE_UNTIL_SIZE
    };

    

    nsresult DecodeSomeOfImage(RasterImage* aImg,
                               DecodeType aDecodeType = DECODE_TYPE_NORMAL);

  private: 

    LinkedList<DecodeRequest> mASAPDecodeRequests;
    LinkedList<DecodeRequest> mNormalDecodeRequests;

    

    bool mPendingInEventLoop;
  };

  void DrawWithPreDownscaleIfNeeded(imgFrame *aFrame,
                                    gfxContext *aContext,
                                    gfxPattern::GraphicsFilter aFilter,
                                    const gfxMatrix &aUserSpaceToImageSpace,
                                    const gfxRect &aFill,
                                    const nsIntRect &aSubimage);

  
















  bool AdvanceFrame(mozilla::TimeStamp aTime, nsIntRect* aDirtyRect);

  







  void DeleteImgFrame(uint32_t framenum);

  imgFrame* GetImgFrameNoDecode(uint32_t framenum);
  imgFrame* GetImgFrame(uint32_t framenum);
  imgFrame* GetDrawableImgFrame(uint32_t framenum);
  imgFrame* GetCurrentImgFrame();
  imgFrame* GetCurrentDrawableImgFrame();
  uint32_t GetCurrentImgFrameIndex() const;
  mozilla::TimeStamp GetCurrentImgFrameEndTime() const;

  size_t SizeOfDecodedWithComputedFallbackIfHeap(gfxASurface::MemoryLocation aLocation,
                                                 nsMallocSizeOfFun aMallocSizeOf) const;

  inline void EnsureAnimExists()
  {
    if (!mAnim) {

      
      mAnim = new Anim();

      
      
      
      
      
      
      
      
      
      LockImage();

      
      mStatusTracker->RecordImageIsAnimated();
    }
  }

  






  nsresult DoComposite(nsIntRect* aDirtyRect,
                       imgFrame* aPrevFrame,
                       imgFrame* aNextFrame,
                       int32_t aNextFrameIndex);

  





  static void ClearFrame(imgFrame* aFrame);

  
  static void ClearFrame(imgFrame* aFrame, nsIntRect &aRect);
  
  
  static bool CopyFrameImage(imgFrame *aSrcFrame,
                               imgFrame *aDstFrame);
  
  






  static nsresult DrawFrameTo(imgFrame *aSrcFrame,
                              imgFrame *aDstFrame,
                              nsIntRect& aRect);

  nsresult InternalAddFrameHelper(uint32_t framenum, imgFrame *frame,
                                  uint8_t **imageData, uint32_t *imageLength,
                                  uint32_t **paletteData, uint32_t *paletteLength);
  nsresult InternalAddFrame(uint32_t framenum, int32_t aX, int32_t aY, int32_t aWidth, int32_t aHeight,
                            gfxASurface::gfxImageFormat aFormat, uint8_t aPaletteDepth,
                            uint8_t **imageData, uint32_t *imageLength,
                            uint32_t **paletteData, uint32_t *paletteLength);

  bool ApplyDecodeFlags(uint32_t aNewFlags);

  already_AddRefed<layers::Image> GetCurrentImage();
  void UpdateImageContainer();

  void SetInUpdateImageContainer(bool aInUpdate) { mInUpdateImageContainer = aInUpdate; }
  bool IsInUpdateImageContainer() { return mInUpdateImageContainer; }
  enum RequestDecodeType {
      ASYNCHRONOUS,
      SOMEWHAT_SYNCHRONOUS
  };
  NS_IMETHOD RequestDecodeCore(RequestDecodeType aDecodeType);

private: 

  nsIntSize                  mSize;

  
  
  
  
  
  
  
  
  uint32_t                   mFrameDecodeFlags;

  
  
  
  
  nsTArray<imgFrame *>       mFrames;
  
  nsCOMPtr<nsIProperties>    mProperties;

  
  
  
  RasterImage::Anim*        mAnim;
  
  
  int32_t                    mLoopCount;
  
  mozilla::WeakPtr<imgDecoderObserver> mObserver;

  
  uint32_t                   mLockCount;
  DiscardTracker::Node       mDiscardTrackerNode;

  
  FallibleTArray<char>       mSourceData;
  nsCString                  mSourceDataMimeType;
  nsCString                  mURIString;

  friend class DiscardTracker;

  
  nsRefPtr<Decoder>              mDecoder;
  DecodeRequest                  mDecodeRequest;
  uint32_t                       mBytesDecoded;

  
  
  int32_t                        mDecodeCount;

  
  nsRefPtr<mozilla::layers::ImageContainer> mImageContainer;

#ifdef DEBUG
  uint32_t                       mFramesNotified;
#endif

  
  bool                       mHasSize:1;       
  bool                       mDecodeOnDraw:1;  
  bool                       mMultipart:1;     
  bool                       mDiscardable:1;   
  bool                       mHasSourceData:1; 

  
  bool                       mDecoded:1;
  bool                       mHasBeenDecoded:1;

  bool                       mInDecoder:1;

  
  
  bool                       mAnimationFinished:1;

  
  bool                       mFinishing:1;

  bool                       mInUpdateImageContainer:1;

  
  nsresult WantDecodedFrames();
  nsresult SyncDecode();
  nsresult InitDecoder(bool aDoSizeDecode);
  nsresult WriteToDecoder(const char *aBuffer, uint32_t aCount);
  nsresult DecodeSomeData(uint32_t aMaxBytes);
  bool     IsDecodeFinished();
  TimeStamp mDrawStartTime;

  inline bool CanScale(gfxPattern::GraphicsFilter aFilter, gfxSize aScale);

  struct ScaleResult
  {
    ScaleResult()
     : status(SCALE_INVALID)
    {}

    gfxSize scale;
    nsAutoPtr<imgFrame> frame;
    ScaleStatus status;
  };

  ScaleResult mScaleResult;

  
  
  
  ScaleRequest* mScaleRequest;

  nsresult ShutdownDecoder(eShutdownIntent aIntent);

  
  void DoError();
  bool CanDiscard();
  bool CanForciblyDiscard();
  bool DiscardingActive();
  bool StoringSourceData() const;

protected:
  RasterImage(imgStatusTracker* aStatusTracker = nullptr);

  bool ShouldAnimate();

  friend class ImageFactory;
};

inline NS_IMETHODIMP RasterImage::GetAnimationMode(uint16_t *aAnimationMode) {
  return GetAnimationModeInternal(aAnimationMode);
}

inline NS_IMETHODIMP RasterImage::SetAnimationMode(uint16_t aAnimationMode) {
  return SetAnimationModeInternal(aAnimationMode);
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
