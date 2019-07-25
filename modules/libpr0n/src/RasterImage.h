



















































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

class RasterImage : public mozilla::imagelib::Image,
                    public nsITimerCallback,
                    public nsIProperties,
                    public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IMGICONTAINER
  NS_DECL_NSITIMERCALLBACK
  NS_DECL_NSIPROPERTIES

  RasterImage();
  virtual ~RasterImage();

  
  virtual PRUint16 GetType() { return imgIContainer::TYPE_RASTER; }

  
  nsresult Init(imgIDecoderObserver *aObserver,
                const char* aMimeType,
                PRUint32 aFlags);
  nsresult GetCurrentFrameRect(nsIntRect& aRect);
  PRUint32 GetCurrentFrameIndex();
  PRUint32 GetNumFrames();
  PRUint32 GetDataSize();

  
  static NS_METHOD WriteToRasterImage(nsIInputStream* aIn, void* aClosure,
                                      const char* aFromRawSegment,
                                      PRUint32 aToOffset, PRUint32 aCount,
                                      PRUint32* aWriteCount);

  PRUint32 GetDecodedDataSize();
  PRUint32 GetSourceDataSize();

  
  void Discard();

  
  nsresult SetFrameDisposalMethod(PRUint32 aFrameNum,
                                  PRInt32 aDisposalMethod);
  nsresult SetFrameTimeout(PRUint32 aFrameNum, PRInt32 aTimeout);
  nsresult SetFrameBlendMethod(PRUint32 aFrameNum, PRInt32 aBlendMethod);
  nsresult SetFrameHasNoAlpha(PRUint32 aFrameNum);

  




  nsresult SetSize(PRInt32 aWidth, PRInt32 aHeight);

  nsresult EnsureCleanFrame(PRUint32 aFramenum, PRInt32 aX, PRInt32 aY,
                            PRInt32 aWidth, PRInt32 aHeight,
                            gfxASurface::gfxImageFormat aFormat,
                            PRUint8** imageData,
                            PRUint32* imageLength);

  


  nsresult AppendFrame(PRInt32 aX, PRInt32 aY,
                       PRInt32 aWidth, PRInt32 aHeight,
                       gfxASurface::gfxImageFormat aFormat,
                       PRUint8** imageData,
                       PRUint32* imageLength);

  nsresult AppendPalettedFrame(PRInt32 aX, PRInt32 aY,
                               PRInt32 aWidth, PRInt32 aHeight,
                               gfxASurface::gfxImageFormat aFormat,
                               PRUint8 aPaletteDepth,
                               PRUint8**  imageData,
                               PRUint32*  imageLength,
                               PRUint32** paletteData,
                               PRUint32*  paletteLength);

  nsresult FrameUpdated(PRUint32 aFrameNum, nsIntRect& aUpdatedRect);

  
  nsresult EndFrameDecode(PRUint32 aFrameNum);

  
  nsresult DecodingComplete();

  



  void     SetLoopCount(PRInt32 aLoopCount);

  







  nsresult AddSourceData(const char *aBuffer, PRUint32 aCount);

  
  virtual nsresult SourceDataComplete();

  
  virtual nsresult NewSourceData();

  










  virtual nsresult SetSourceSizeHint(PRUint32 sizeHint);

  
  
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

private:
  struct Anim
  {
    
    nsIntRect                  firstFrameRefreshArea;
    
    PRUint32                   currentDecodingFrameIndex; 
    PRUint32                   currentAnimationFrameIndex; 
    
    PRInt32                    lastCompositedFrameIndex;
    







    nsAutoPtr<imgFrame>        compositingFrame;
    





    nsAutoPtr<imgFrame>        compositingPrevFrame;
    
    nsCOMPtr<nsITimer>         timer;
    
    
    PRPackedBool               doneDecoding;
    
    PRPackedBool               animating;

    Anim() :
      firstFrameRefreshArea(),
      currentDecodingFrameIndex(0),
      currentAnimationFrameIndex(0),
      lastCompositedFrameIndex(-1),
      doneDecoding(PR_FALSE),
      animating(PR_FALSE)
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

  imgFrame* GetImgFrame(PRUint32 framenum);
  imgFrame* GetDrawableImgFrame(PRUint32 framenum);
  imgFrame* GetCurrentImgFrame();
  imgFrame* GetCurrentDrawableImgFrame();
  PRUint32 GetCurrentImgFrameIndex() const;
  
  inline Anim* ensureAnimExists()
  {
    if (!mAnim) {

      
      mAnim = new Anim();

      
      
      
      
      
      
      
      
      
      LockImage();
    }
    return mAnim;
  }
  
  








  nsresult DoComposite(imgFrame** aFrameToUse, nsIntRect* aDirtyRect,
                       imgFrame* aPrevFrame,
                       imgFrame* aNextFrame,
                       PRInt32 aNextFrameIndex);
  
  





  static void ClearFrame(imgFrame* aFrame);
  
  
  static void ClearFrame(imgFrame* aFrame, nsIntRect &aRect);
  
  
  static PRBool CopyFrameImage(imgFrame *aSrcFrame,
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
  
  
  
  
  
  nsTArray<imgFrame *>       mFrames;
  
  nsCOMPtr<nsIProperties>    mProperties;

  
  
  
  RasterImage::Anim*        mAnim;
  
  
  PRUint16                   mAnimationMode;
  
  
  PRInt32                    mLoopCount;
  
  
  nsWeakPtr                  mObserver;

  
  PRUint32                   mLockCount;
  DiscardTrackerNode         mDiscardTrackerNode;

  
  nsTArray<char>             mSourceData;
  nsCString                  mSourceDataMimeType;

  friend class imgDecodeWorker;
  friend class DiscardTracker;

  
  nsRefPtr<Decoder>              mDecoder;
  nsRefPtr<imgDecodeWorker>      mWorker;
  PRUint32                       mBytesDecoded;
  PRUint32                       mDecoderFlags;

  
  PRPackedBool               mHasSize:1;       
  PRPackedBool               mDecodeOnDraw:1;  
  PRPackedBool               mMultipart:1;     
  PRPackedBool               mDiscardable:1;   
  PRPackedBool               mHasSourceData:1; 

  
  PRPackedBool               mDecoded:1;
  PRPackedBool               mHasBeenDecoded:1;

  
  PRPackedBool               mWorkerPending:1;
  PRPackedBool               mInDecoder:1;

  PRPackedBool               mError:1;  

  
  nsresult WantDecodedFrames();
  nsresult SyncDecode();
  nsresult InitDecoder(PRUint32 dFlags);
  nsresult WriteToDecoder(const char *aBuffer, PRUint32 aCount);
  nsresult DecodeSomeData(PRUint32 aMaxBytes);
  PRBool   IsDecodeFinished();

  
  enum eShutdownIntent {
    eShutdownIntent_Done        = 0,
    eShutdownIntent_Interrupted = 1,
    eShutdownIntent_Error       = 2,
    eShutdownIntent_AllCount    = 3
  };
  nsresult ShutdownDecoder(eShutdownIntent aIntent);

  
  void DoError();
  PRBool CanDiscard();
  PRBool DiscardingActive();
  PRBool StoringSourceData();

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
