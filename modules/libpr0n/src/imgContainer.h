


















































#ifndef __imgContainer_h__
#define __imgContainer_h__

#include "nsCOMArray.h"
#include "nsCOMPtr.h"
#include "imgIContainer.h"
#include "gfxIImageFrame.h"
#include "nsIProperties.h"
#include "nsITimer.h"
#include "nsWeakReference.h"
#include "nsTArray.h"

#define NS_IMGCONTAINER_CID \
{ /* 27f0682c-ff64-4dd2-ae7a-668e59f2fd38 */         \
     0x27f0682c,                                     \
     0xff64,                                         \
     0x4dd2,                                         \
    {0xae, 0x7a, 0x66, 0x8e, 0x59, 0xf2, 0xfd, 0x38} \
}































































class imgContainer : public imgIContainer, 
                     public nsITimerCallback, 
                     public nsIProperties
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IMGICONTAINER
  NS_DECL_NSITIMERCALLBACK
  NS_DECL_NSIPROPERTIES

  imgContainer();
  virtual ~imgContainer();

private:
  struct Anim
  {
    
    nsIntRect                  firstFrameRefreshArea;
    
    PRInt32                    currentDecodingFrameIndex; 
    PRInt32                    currentAnimationFrameIndex; 
    
    PRInt32                    lastCompositedFrameIndex;
    
    
    PRBool                     doneDecoding;
    
    PRBool                     animating;
    







    nsCOMPtr<gfxIImageFrame>   compositingFrame;
    





    nsCOMPtr<gfxIImageFrame>   compositingPrevFrame;
    
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

  gfxIImageFrame* GetCurrentFrameNoRef();
  
  inline Anim* ensureAnimExists() {
    if (!mAnim)
      mAnim = new Anim();
    return mAnim;
  }
  
  








  nsresult DoComposite(gfxIImageFrame** aFrameToUse, nsIntRect* aDirtyRect,
                       gfxIImageFrame* aPrevFrame,
                       gfxIImageFrame* aNextFrame,
                       PRInt32 aNextFrameIndex);
  
  









  void BuildCompositeMask(gfxIImageFrame* aCompositingFrame,
                          gfxIImageFrame* aOverlayFrame);
  
  






  void SetMaskVisibility(gfxIImageFrame *aFrame, PRBool aVisible);
  
  void SetMaskVisibility(gfxIImageFrame *aFrame,
                         PRInt32 aX, PRInt32 aY,
                         PRInt32 aWidth, PRInt32 aHeight,
                         PRBool aVisible);
  
  void SetMaskVisibility(gfxIImageFrame *aFrame,
                         nsIntRect &aRect, PRBool aVisible) {
    SetMaskVisibility(aFrame, aRect.x, aRect.y,
                      aRect.width, aRect.height, aVisible);
  }
  
  





  static void ClearFrame(gfxIImageFrame* aFrame);
  
  
  static void ClearFrame(gfxIImageFrame* aFrame, nsIntRect &aRect);
  
  
  static PRBool CopyFrameImage(gfxIImageFrame *aSrcFrame,
                               gfxIImageFrame *aDstFrame);
  
  






  static nsresult DrawFrameTo(gfxIImageFrame *aSrcFrame,
                              gfxIImageFrame *aDstFrame,
                              nsIntRect& aRect);

  nsIntSize                  mSize;
  
  
  
  
  nsCOMArray<gfxIImageFrame> mFrames;
  int                        mNumFrames; 
  
  nsCOMPtr<nsIProperties>    mProperties;

  
  
  imgContainer::Anim*        mAnim;
  
  
  PRUint16                   mAnimationMode;
  
  
  PRInt32                    mLoopCount;
  
  
  nsWeakPtr                  mObserver;

  PRBool                     mDiscardable;
  PRBool                     mDiscarded;
  nsCString                  mDiscardableMimeType;

  nsTArray<char>             mRestoreData;
  PRBool                     mRestoreDataDone;
  nsCOMPtr<nsITimer>         mDiscardTimer;

  nsresult ResetDiscardTimer (void);
  nsresult RestoreDiscardedData (void);
  nsresult ReloadImages (void);
  static void sDiscardTimerCallback (nsITimer *aTimer, void *aClosure);
};

#endif 
