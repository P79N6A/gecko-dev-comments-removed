




#ifndef MOZILLA_IMAGELIB_IMAGE_H_
#define MOZILLA_IMAGELIB_IMAGE_H_

#include "imgIContainer.h"
#include "imgStatusTracker.h"
#include "prtypes.h"

namespace mozilla {
namespace image {

class Image : public imgIContainer
{
public:
  
  NS_IMETHOD GetAnimationMode(uint16_t *aAnimationMode);
  NS_IMETHOD SetAnimationMode(uint16_t aAnimationMode);

  imgStatusTracker& GetStatusTracker() { return *mStatusTracker; }

  















  static const uint32_t INIT_FLAG_NONE           = 0x0;
  static const uint32_t INIT_FLAG_DISCARDABLE    = 0x1;
  static const uint32_t INIT_FLAG_DECODE_ON_DRAW = 0x2;
  static const uint32_t INIT_FLAG_MULTIPART      = 0x4;

  






  virtual nsresult Init(imgIDecoderObserver* aObserver,
                        const char* aMimeType,
                        const char* aURIString,
                        uint32_t aFlags) = 0;

  



  virtual void GetCurrentFrameRect(nsIntRect& aRect) = 0;

  



  uint32_t SizeOfData();

  

      
  virtual size_t HeapSizeOfSourceWithComputedFallback(nsMallocSizeOfFun aMallocSizeOf) const = 0;
  virtual size_t HeapSizeOfDecodedWithComputedFallback(nsMallocSizeOfFun aMallocSizeOf) const = 0;
  virtual size_t NonHeapSizeOfDecoded() const = 0;
  virtual size_t OutOfProcessSizeOfDecoded() const = 0;

  
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
  uint32_t GetAnimationConsumers() { return mAnimationConsumers; }
#endif

  void SetInnerWindowID(uint64_t aInnerWindowId) {
    mInnerWindowId = aInnerWindowId;
  }
  uint64_t InnerWindowID() const { return mInnerWindowId; }

  bool HasError() { return mError; }

protected:
  Image(imgStatusTracker* aStatusTracker);

  



  virtual void EvaluateAnimation();

  virtual nsresult StartAnimation() = 0;
  virtual nsresult StopAnimation() = 0;

  uint64_t mInnerWindowId;

  
  nsAutoPtr<imgStatusTracker> mStatusTracker;
  uint32_t                    mAnimationConsumers;
  uint16_t                    mAnimationMode;   
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
