




































#ifndef MOZILLA_IMAGELIB_IMAGE_H_
#define MOZILLA_IMAGELIB_IMAGE_H_

#include "imgIContainer.h"
#include "imgStatusTracker.h"
#include "prtypes.h"

namespace mozilla {
namespace imagelib {

class Image : public imgIContainer
{
public:
  
  NS_SCRIPTABLE NS_IMETHOD GetAnimationMode(PRUint16 *aAnimationMode);
  NS_SCRIPTABLE NS_IMETHOD SetAnimationMode(PRUint16 aAnimationMode);

  imgStatusTracker& GetStatusTracker() { return *mStatusTracker; }

  















  static const PRUint32 INIT_FLAG_NONE           = 0x0;
  static const PRUint32 INIT_FLAG_DISCARDABLE    = 0x1;
  static const PRUint32 INIT_FLAG_DECODE_ON_DRAW = 0x2;
  static const PRUint32 INIT_FLAG_MULTIPART      = 0x4;

  






  virtual nsresult Init(imgIDecoderObserver* aObserver,
                        const char* aMimeType,
                        const char* aURIString,
                        PRUint32 aFlags) = 0;

  



  virtual void GetCurrentFrameRect(nsIntRect& aRect) = 0;

  



  PRUint32 GetDataSize();

  

      
  virtual PRUint32 GetDecodedDataSize() = 0;
  virtual PRUint32 GetSourceDataSize() = 0;

  
  enum eDecoderType {
    eDecoderType_png     = 0,
    eDecoderType_gif     = 1,
    eDecoderType_jpeg    = 2,
    eDecoderType_bmp     = 3,
    eDecoderType_ico     = 4,
    eDecoderType_icon    = 5,
    eDecoderType_unknown = 6
  };
  static eDecoderType GetDecoderType(const char *aMimeType);

  void IncrementAnimationConsumers();
  void DecrementAnimationConsumers();
#ifdef DEBUG
  PRUint32 GetAnimationConsumers() { return mAnimationConsumers; }
#endif

  void SetWindowID(PRUint64 aWindowId) {
    mWindowId = aWindowId;
  }
  PRUint64 WindowID() const { return mWindowId; }

  PRBool HasError() { return mError; }

protected:
  Image(imgStatusTracker* aStatusTracker);

  



  virtual void EvaluateAnimation();

  virtual nsresult StartAnimation() = 0;
  virtual nsresult StopAnimation() = 0;

  PRUint64 mWindowId;

  
  nsAutoPtr<imgStatusTracker> mStatusTracker;
  PRUint32                    mAnimationConsumers;
  PRUint16                    mAnimationMode;   
  PRPackedBool                mInitialized:1;   
  PRPackedBool                mAnimating:1;     
  PRPackedBool                mError:1;         

  



  virtual PRBool ShouldAnimate() {
    return mAnimationConsumers > 0 && mAnimationMode != kDontAnimMode;
  }
};

} 
} 

#endif 
