



















































#ifndef __imgContainer_h__
#define __imgContainer_h__

#include "nsCOMArray.h"
#include "nsCOMPtr.h"
#include "imgIContainer.h"
#include "imgIDecoder.h"
#include "nsIProperties.h"
#include "nsITimer.h"
#include "nsWeakReference.h"
#include "nsTArray.h"
#include "nsIStringStream.h"
#include "imgFrame.h"
#include "nsThreadUtils.h"

#define NS_IMGCONTAINER_CID \
{ /* c76ff2c1-9bf6-418a-b143-3340c00112f7 */         \
     0x376ff2c1,                                     \
     0x9bf6,                                         \
     0x418a,                                         \
    {0xb1, 0x43, 0x33, 0x40, 0xc0, 0x01, 0x12, 0xf7} \
}































































class imgDecodeWorker;
class imgContainer : public imgIContainer, 
                     public nsITimerCallback,
                     public nsIProperties,
                     public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IMGICONTAINER
  NS_DECL_NSITIMERCALLBACK
  NS_DECL_NSIPROPERTIES

  imgContainer();
  virtual ~imgContainer();

  static NS_METHOD WriteToContainer(nsIInputStream* in, void* closure,
                                    const char* fromRawSegment,
                                    PRUint32 toOffset, PRUint32 count,
                                    PRUint32 *writeCount);

private:
  struct Anim
  {
    
    nsIntRect                  firstFrameRefreshArea;
    
    PRUint32                   currentDecodingFrameIndex; 
    PRUint32                   currentAnimationFrameIndex; 
    
    PRInt32                    lastCompositedFrameIndex;
    
    
    PRBool                     doneDecoding;
    
    PRBool                     animating;
    







    nsAutoPtr<imgFrame>        compositingFrame;
    





    nsAutoPtr<imgFrame>        compositingPrevFrame;
    
    nsCOMPtr<nsITimer>         timer;
    
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

  imgFrame* GetImgFrame(PRUint32 framenum);
  imgFrame* GetCurrentImgFrame();
  PRUint32 GetCurrentImgFrameIndex() const;
  
  inline Anim* ensureAnimExists()
  {
    if (!mAnim) {

      
      mAnim = new Anim();

      
      
      
      mDiscardable = PR_FALSE;
      if (mDiscardTimer) {
        nsresult rv = mDiscardTimer->Cancel();
        if (!NS_SUCCEEDED(rv))
          NS_WARNING("Discard Timer failed to cancel!");
        mDiscardTimer = nsnull;
      }
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
  PRBool                     mHasSize;
  
  
  
  
  
  nsTArray<imgFrame *>       mFrames;
  
  nsCOMPtr<nsIProperties>    mProperties;

  
  
  
  imgContainer::Anim*        mAnim;
  
  
  PRUint16                   mAnimationMode;
  
  
  PRInt32                    mLoopCount;
  
  
  nsWeakPtr                  mObserver;

  
  PRBool                     mDecodeOnDraw;

  
  PRBool                     mMultipart;

  
  PRBool                     mInitialized;

  
  PRBool                     mDiscardable;
  PRUint32                   mLockCount;
  nsCOMPtr<nsITimer>         mDiscardTimer;

  
  nsTArray<char>             mSourceData;
  PRBool                     mHasSourceData;
  nsCString                  mSourceDataMimeType;

  
  PRBool                     mDecoded;

  friend class imgDecodeWorker;

  
  nsCOMPtr<imgIDecoder>          mDecoder;
  nsRefPtr<imgDecodeWorker>      mWorker;
  PRUint32                       mBytesDecoded;
  nsCOMPtr<nsIStringInputStream> mDecoderInput;
  PRUint32                       mDecoderFlags;
  PRBool                         mWorkerPending;
  PRBool                         mInDecoder;

  
  PRBool                         mError;

  
  nsresult ResetDiscardTimer();
  static void sDiscardTimerCallback(nsITimer *aTimer, void *aClosure);

  
  nsresult WantDecodedFrames();
  nsresult SyncDecode();
  nsresult InitDecoder(PRUint32 dFlags);
  nsresult WriteToDecoder(const char *aBuffer, PRUint32 aCount);
  nsresult DecodeSomeData(PRUint32 aMaxBytes);
  PRBool   IsDecodeFinished();
  nsresult EnableDiscarding();

  
  enum eShutdownIntent {
    eShutdownIntent_Done        = 0,
    eShutdownIntent_Interrupted = 1,
    eShutdownIntent_Error       = 2,
    eShutdownIntent_AllCount    = 3
  };
  nsresult ShutdownDecoder(eShutdownIntent aIntent);

  
  void DoError();
  PRBool CanDiscard();
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



#endif 
