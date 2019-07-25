




































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

  

      
  virtual PRUint32 GetDecodedHeapSize() = 0;
  virtual PRUint32 GetDecodedNonheapSize() = 0;
  virtual PRUint32 GetDecodedOutOfProcessSize() = 0;
  virtual PRUint32 GetSourceHeapSize() = 0;

  
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

  void SetInnerWindowID(PRUint64 aInnerWindowId) {
    mInnerWindowId = aInnerWindowId;
  }
  PRUint64 InnerWindowID() const { return mInnerWindowId; }

  bool HasError() { return mError; }

protected:
  Image(imgStatusTracker* aStatusTracker);

  



  virtual void EvaluateAnimation();

  virtual nsresult StartAnimation() = 0;
  virtual nsresult StopAnimation() = 0;

  PRUint64 mInnerWindowId;

  
  nsAutoPtr<imgStatusTracker> mStatusTracker;
  PRUint32                    mAnimationConsumers;
  PRUint16                    mAnimationMode;   
  bool                        mInitialized:1;   
  bool                        mAnimating:1;     
  bool                        mError:1;         

  



  virtual bool ShouldAnimate() {
    return mAnimationConsumers > 0 && mAnimationMode != kDontAnimMode;
  }
};

} 
} 

#endif 
