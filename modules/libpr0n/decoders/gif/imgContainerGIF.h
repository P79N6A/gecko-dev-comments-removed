














































#ifndef _imgContainerGIF_h_
#define _imgContainerGIF_h_

#include "imgIContainerObserver.h"
#include "imgIContainer.h"
#include "nsCOMArray.h"
#include "nsCOMPtr.h"
#include "nsITimer.h"
#include "imgIDecoderObserver.h"
#include "gfxIImageFrame.h"
#include "nsWeakReference.h"

#define NS_GIFCONTAINER_CID \
{ /* da72e7ee-4821-4452-802d-5eb2d865dd3c */         \
     0xda72e7ee,                                     \
     0x4821,                                         \
     0x4452,                                         \
    {0x80, 0x2d, 0x5e, 0xb2, 0xd8, 0x65, 0xdd, 0x3c} \
}




























































class imgContainerGIF : public imgIContainer,
                        public nsITimerCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IMGICONTAINER
  NS_DECL_NSITIMERCALLBACK

  imgContainerGIF();
  virtual ~imgContainerGIF();

private:
  friend class nsGIFDecoder2;

  


  enum {
    DISPOSE_CLEAR_ALL       = -1, 
                                  
    DISPOSE_NOT_SPECIFIED    = 0, 
    DISPOSE_KEEP             = 1, 
    DISPOSE_CLEAR            = 2, 
    DISPOSE_RESTORE_PREVIOUS = 3  
  };

  inline gfxIImageFrame* inlinedGetCurrentFrame() {
    if (mLastCompositedFrameIndex == mCurrentAnimationFrameIndex)
      return mCompositingFrame;

    return mFrames.SafeObjectAt(mCurrentAnimationFrameIndex);
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

  





  static void BlackenFrame(gfxIImageFrame* aFrame);
  
  static void BlackenFrame(gfxIImageFrame* aFrame,
                    PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight);
  
  static inline void BlackenFrame(gfxIImageFrame* aFrame, nsIntRect &aRect) {
    BlackenFrame(aFrame, aRect.x, aRect.y, aRect.width, aRect.height);
  }

  
  static PRBool CopyFrameImage(gfxIImageFrame *aSrcFrame,
                               gfxIImageFrame *aDstFrame);


  
  nsWeakPtr                  mObserver;
  
  nsCOMArray<gfxIImageFrame> mFrames;

  
  nsIntSize                  mSize;
  
  nsIntRect                  mFirstFrameRefreshArea;

  PRInt32                    mCurrentDecodingFrameIndex; 
  PRInt32                    mCurrentAnimationFrameIndex; 
  
  PRInt32                    mLastCompositedFrameIndex;
  
  
  PRBool                     mDoneDecoding;


  
  PRBool                     mAnimating;
  
  PRUint16                   mAnimationMode;
  
  PRInt32                    mLoopCount;

  
  nsCOMPtr<nsITimer>         mTimer;

  







  nsCOMPtr<gfxIImageFrame>   mCompositingFrame;

  





  nsCOMPtr<gfxIImageFrame>   mCompositingPrevFrame;
};

#endif 
