


















































#ifndef __imgContainer_h__
#define __imgContainer_h__

#include "nsCOMArray.h"
#include "nsCOMPtr.h"
#include "imgIContainer.h"
#include "nsIProperties.h"
#include "nsITimer.h"
#include "nsWeakReference.h"
#include "nsTArray.h"
#include "imgFrame.h"

#define NS_IMGCONTAINER_CID \
{ /* c76ff2c1-9bf6-418a-b143-3340c00112f7 */         \
     0x376ff2c1,                                     \
     0x9bf6,                                         \
     0x418a,                                         \
    {0xb1, 0x43, 0x33, 0x40, 0xc0, 0x01, 0x12, 0xf7} \
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
  PRInt32 GetCurrentImgFrameIndex() const;
  
  inline Anim* ensureAnimExists() {
    if (!mAnim)
      mAnim = new Anim();
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
