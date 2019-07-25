



















































#ifndef mozilla_imagelib_RasterImage_h_
#define mozilla_imagelib_RasterImage_h_

#include "Image.h"
#include "nsCOMArray.h"
#include "nsCOMPtr.h"
#include "imgIContainer.h"
#include "nsIProperties.h"
#include "nsITimer.h"
#include "nsWeakReference.h"
#include "nsTArray.h"
#include "imgFrame.h"
#include "nsThreadUtils.h"
#include "DiscardTracker.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/Telemetry.h"
#ifdef DEBUG
  #include "imgIContainerDebug.h"
#endif

class imgIDecoder;
class nsIInputStream;

#define NS_RASTERIMAGE_CID \
{ /* 376ff2c1-9bf6-418a-b143-3340c00112f7 */         \
     0x376ff2c1,                                     \
     0x9bf6,                                         \
     0x418a,                                         \
    {0xb1, 0x43, 0x33, 0x40, 0xc0, 0x01, 0x12, 0xf7} \
}
































































namespace mozilla {
namespace imagelib {

class imgDecodeWorker;
class Decoder;

class RasterImage : public Image
                  , public nsITimerCallback
                  , public nsIProperties
                  , public nsSupportsWeakReference
#ifdef DEBUG
                  , public imgIContainerDebug
#endif
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSITIMERCALLBACK
  NS_DECL_NSIPROPERTIES
#ifdef DEBUG
  NS_DECL_IMGICONTAINERDEBUG
#endif

  
  
  NS_SCRIPTABLE NS_IMETHOD GetWidth(PRInt32 *aWidth);
  NS_SCRIPTABLE NS_IMETHOD GetHeight(PRInt32 *aHeight);
  NS_SCRIPTABLE NS_IMETHOD GetType(PRUint16 *aType);
  NS_IMETHOD_(PRUint16) GetType(void);
  NS_SCRIPTABLE NS_IMETHOD GetAnimated(bool *aAnimated);
  NS_SCRIPTABLE NS_IMETHOD GetCurrentFrameIsOpaque(bool *aCurrentFrameIsOpaque);
  NS_IMETHOD GetFrame(PRUint32 aWhichFrame, PRUint32 aFlags, gfxASurface **_retval NS_OUTPARAM);
  NS_IMETHOD CopyFrame(PRUint32 aWhichFrame, PRUint32 aFlags, gfxImageSurface **_retval NS_OUTPARAM);
  NS_IMETHOD ExtractFrame(PRUint32 aWhichFrame, const nsIntRect & aRect, PRUint32 aFlags, imgIContainer **_retval NS_OUTPARAM);
  NS_IMETHOD Draw(gfxContext *aContext, gfxPattern::GraphicsFilter aFilter, const gfxMatrix & aUserSpaceToImageSpace, const gfxRect & aFill, const nsIntRect & aSubimage, const nsIntSize & aViewportSize, PRUint32 aFlags);
  NS_IMETHOD_(nsIFrame *) GetRootLayoutFrame(void);
  NS_SCRIPTABLE NS_IMETHOD RequestDecode(void);
  NS_SCRIPTABLE NS_IMETHOD LockImage(void);
  NS_SCRIPTABLE NS_IMETHOD UnlockImage(void);
  NS_SCRIPTABLE NS_IMETHOD ResetAnimation(void);
  NS_IMETHOD_(void) RequestRefresh(const mozilla::TimeStamp& aTime);
  

  RasterImage(imgStatusTracker* aStatusTracker = nsnull);
  virtual ~RasterImage();

  virtual nsresult StartAnimation();
  virtual nsresult StopAnimation();

  
  nsresult Init(imgIDecoderObserver* aObserver,
                const char* aMimeType,
                const char* aURIString,
                PRUint32 aFlags);
  void     GetCurrentFrameRect(nsIntRect& aRect);

  
  static NS_METHOD WriteToRasterImage(nsIInputStream* aIn, void* aClosure,
                                      const char* aFromRawSegment,
                                      PRUint32 aToOffset, PRUint32 aCount,
                                      PRUint32* aWriteCount);

  

  PRUint32 GetCurrentFrameIndex();

  
  PRUint32 GetNumFrames();

  virtual PRUint32 GetDecodedHeapSize();
  virtual PRUint32 GetDecodedNonheapSize();
  virtual PRUint32 GetDecodedOutOfProcessSize();
  virtual PRUint32 GetSourceHeapSize();

  
  void Discard(bool force = false);
  void ForceDiscard() { Discard( true); }

  
  nsresult SetFrameDisposalMethod(PRUint32 aFrameNum,
                                  PRInt32 aDisposalMethod);
  nsresult SetFrameTimeout(PRUint32 aFrameNum, PRInt32 aTimeout);
  nsresult SetFrameBlendMethod(PRUint32 aFrameNum, PRInt32 aBlendMethod);
  nsresult SetFrameHasNoAlpha(PRUint32 aFrameNum);

  




  nsresult SetSize(PRInt32 aWidth, PRInt32 aHeight);


  





  nsresult EnsureFrame(PRUint32 aFramenum, PRInt32 aX, PRInt32 aY,
                       PRInt32 aWidth, PRInt32 aHeight,
                       gfxASurface::gfxImageFormat aFormat,
                       PRUint8 aPaletteDepth,
                       PRUint8** imageData,
                       PRUint32* imageLength,
                       PRUint32** paletteData,
                       PRUint32* paletteLength);

  



  nsresult EnsureFrame(PRUint32 aFramenum, PRInt32 aX, PRInt32 aY,
                       PRInt32 aWidth, PRInt32 aHeight,
                       gfxASurface::gfxImageFormat aFormat,
                       PRUint8** imageData,
                       PRUint32* imageLength);

  void FrameUpdated(PRUint32 aFrameNum, nsIntRect& aUpdatedRect);

  
  nsresult DecodingComplete();

  



  void     SetLoopCount(PRInt32 aLoopCount);

  







  nsresult AddSourceData(const char *aBuffer, PRUint32 aCount);

  
  nsresult SourceDataComplete();

  
  nsresult NewSourceData();

  










  nsresult SetSourceSizeHint(PRUint32 sizeHint);

  
  
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

  
  static void SetDecodeBytesAtATime(PRUint32 aBytesAtATime);
  static void SetMaxMSBeforeYield(PRUint32 aMaxMS);
  static void SetMaxBytesForSyncDecode(PRUint32 aMaxBytes);

  const char* GetURIString() { return mURIString.get();}

private:
  struct Anim
  {
    
    nsIntRect                  firstFrameRefreshArea;
    PRUint32                   currentAnimationFrameIndex; 
    
    PRInt32                    lastCompositedFrameIndex;
    







    nsAutoPtr<imgFrame>        compositingFrame;
    





    nsAutoPtr<imgFrame>        compositingPrevFrame;
    
    nsCOMPtr<nsITimer>         timer;

    Anim() :
      firstFrameRefreshArea(),
      currentAnimationFrameIndex(0),
      lastCompositedFrameIndex(-1)
    {
      ;
    }
    ~Anim()
    {
      if (timer)
        timer->Cancel();
    }
  };

  







  void DeleteImgFrame(PRUint32 framenum);

  imgFrame* GetImgFrameNoDecode(PRUint32 framenum);
  imgFrame* GetImgFrame(PRUint32 framenum);
  imgFrame* GetDrawableImgFrame(PRUint32 framenum);
  imgFrame* GetCurrentImgFrame();
  imgFrame* GetCurrentDrawableImgFrame();
  PRUint32 GetCurrentImgFrameIndex() const;
  
  inline void EnsureAnimExists()
  {
    if (!mAnim) {

      
      mAnim = new Anim();

      
      
      
      
      
      
      
      
      
      LockImage();
    }
  }
  
  








  nsresult DoComposite(imgFrame** aFrameToUse, nsIntRect* aDirtyRect,
                       imgFrame* aPrevFrame,
                       imgFrame* aNextFrame,
                       PRInt32 aNextFrameIndex);
  
  





  static void ClearFrame(imgFrame* aFrame);
  
  
  static void ClearFrame(imgFrame* aFrame, nsIntRect &aRect);
  
  
  static bool CopyFrameImage(imgFrame *aSrcFrame,
                               imgFrame *aDstFrame);
  
  






  static nsresult DrawFrameTo(imgFrame *aSrcFrame,
                              imgFrame *aDstFrame,
                              nsIntRect& aRect);

  nsresult InternalAddFrameHelper(PRUint32 framenum, imgFrame *frame,
                                  PRUint8 **imageData, PRUint32 *imageLength,
                                  PRUint32 **paletteData, PRUint32 *paletteLength);
  nsresult InternalAddFrame(PRUint32 framenum, PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight,
                            gfxASurface::gfxImageFormat aFormat, PRUint8 aPaletteDepth,
                            PRUint8 **imageData, PRUint32 *imageLength,
                            PRUint32 **paletteData, PRUint32 *paletteLength);

private: 

  nsIntSize                  mSize;

  
  
  
  
  
  
  
  
  PRUint32                   mFrameDecodeFlags;

  
  
  
  
  nsTArray<imgFrame *>       mFrames;
  
  nsCOMPtr<nsIProperties>    mProperties;

  
  
  
  RasterImage::Anim*        mAnim;
  
  
  PRInt32                    mLoopCount;
  
  
  nsWeakPtr                  mObserver;

  
  PRUint32                   mLockCount;
  DiscardTrackerNode         mDiscardTrackerNode;

  
  FallibleTArray<char>       mSourceData;
  nsCString                  mSourceDataMimeType;
  nsCString                  mURIString;

  friend class imgDecodeWorker;
  friend class DiscardTracker;

  
  nsRefPtr<Decoder>              mDecoder;
  nsRefPtr<imgDecodeWorker>      mWorker;
  PRUint32                       mBytesDecoded;

  
  
  PRInt32                        mDecodeCount;

#ifdef DEBUG
  PRUint32                       mFramesNotified;
#endif

  
  bool                       mHasSize:1;       
  bool                       mDecodeOnDraw:1;  
  bool                       mMultipart:1;     
  bool                       mDiscardable:1;   
  bool                       mHasSourceData:1; 

  
  bool                       mDecoded:1;
  bool                       mHasBeenDecoded:1;

  
  bool                       mWorkerPending:1;
  bool                       mInDecoder:1;

  
  
  bool                       mAnimationFinished:1;

  
  nsresult WantDecodedFrames();
  nsresult SyncDecode();
  nsresult InitDecoder(bool aDoSizeDecode);
  nsresult WriteToDecoder(const char *aBuffer, PRUint32 aCount);
  nsresult DecodeSomeData(PRUint32 aMaxBytes);
  bool     IsDecodeFinished();
  TimeStamp mDrawStartTime;

  
  enum eShutdownIntent {
    eShutdownIntent_Done        = 0,
    eShutdownIntent_Interrupted = 1,
    eShutdownIntent_Error       = 2,
    eShutdownIntent_AllCount    = 3
  };
  nsresult ShutdownDecoder(eShutdownIntent aIntent);

  
  void DoError();
  bool CanDiscard();
  bool CanForciblyDiscard();
  bool DiscardingActive();
  bool StoringSourceData();

protected:
  bool ShouldAnimate();
};










class imgDecodeWorker : public nsRunnable
{
  public:
    imgDecodeWorker(imgIContainer* aContainer) {
      mContainer = do_GetWeakReference(aContainer);
    }
    NS_IMETHOD Run();
    NS_METHOD  Dispatch();

  private:
    nsWeakPtr mContainer;
    TimeDuration mDecodeTime; 
};







class imgDecodeRequestor : public nsRunnable
{
  public:
    imgDecodeRequestor(imgIContainer *aContainer) {
      mContainer = do_GetWeakReference(aContainer);
    }
    NS_IMETHOD Run() {
      nsCOMPtr<imgIContainer> con = do_QueryReferent(mContainer);
      if (con)
        con->RequestDecode();
      return NS_OK;
    }

  private:
    nsWeakPtr mContainer;
};

} 
} 

#endif 
