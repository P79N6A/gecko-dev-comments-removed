




#ifndef MOZILLA_IMAGELIB_IMAGE_H_
#define MOZILLA_IMAGELIB_IMAGE_H_

#include "imgIContainer.h"
#include "imgStatusTracker.h"
#include "nsIURI.h"
#include "nsIRequest.h"
#include "nsIInputStream.h"

namespace mozilla {
namespace image {

class Image : public imgIContainer
{
public:
  
  enum eDecoderType {
    eDecoderType_png     = 0,
    eDecoderType_gif     = 1,
    eDecoderType_jpeg    = 2,
    eDecoderType_bmp     = 3,
    eDecoderType_ico     = 4,
    eDecoderType_icon    = 5,
    eDecoderType_wbmp    = 6,
    eDecoderType_unknown = 7
  };
  static eDecoderType GetDecoderType(const char *aMimeType);

  















  static const uint32_t INIT_FLAG_NONE           = 0x0;
  static const uint32_t INIT_FLAG_DISCARDABLE    = 0x1;
  static const uint32_t INIT_FLAG_DECODE_ON_DRAW = 0x2;
  static const uint32_t INIT_FLAG_MULTIPART      = 0x4;

  





  virtual nsresult Init(const char* aMimeType,
                        uint32_t aFlags) = 0;

  virtual imgStatusTracker& GetStatusTracker() = 0;

  


  virtual nsIntRect FrameRect(uint32_t aWhichFrame) = 0;

  



  virtual uint32_t SizeOfData() = 0;

  

      
  virtual size_t HeapSizeOfSourceWithComputedFallback(nsMallocSizeOfFun aMallocSizeOf) const = 0;
  virtual size_t HeapSizeOfDecodedWithComputedFallback(nsMallocSizeOfFun aMallocSizeOf) const = 0;
  virtual size_t NonHeapSizeOfDecoded() const = 0;
  virtual size_t OutOfProcessSizeOfDecoded() const = 0;

  virtual void IncrementAnimationConsumers() = 0;
  virtual void DecrementAnimationConsumers() = 0;
#ifdef DEBUG
  virtual uint32_t GetAnimationConsumers() = 0;
#endif

  










  virtual nsresult OnImageDataAvailable(nsIRequest* aRequest,
                                        nsISupports* aContext,
                                        nsIInputStream* aInStr,
                                        uint64_t aSourceOffset,
                                        uint32_t aCount) = 0;

  







  virtual nsresult OnImageDataComplete(nsIRequest* aRequest,
                                       nsISupports* aContext,
                                       nsresult aStatus,
                                       bool aLastPart) = 0;

  



  virtual nsresult OnNewSourceData() = 0;

  virtual void SetInnerWindowID(uint64_t aInnerWindowId) = 0;
  virtual uint64_t InnerWindowID() const = 0;

  virtual bool HasError() = 0;
  virtual void SetHasError() = 0;

  virtual nsIURI* GetURI() = 0;
};

class ImageResource : public Image
{
public:
  virtual imgStatusTracker& GetStatusTracker() MOZ_OVERRIDE { return *mStatusTracker; }
  virtual uint32_t SizeOfData() MOZ_OVERRIDE;

  virtual void IncrementAnimationConsumers() MOZ_OVERRIDE;
  virtual void DecrementAnimationConsumers() MOZ_OVERRIDE;
#ifdef DEBUG
  virtual uint32_t GetAnimationConsumers() MOZ_OVERRIDE { return mAnimationConsumers; }
#endif

  virtual void SetInnerWindowID(uint64_t aInnerWindowId) MOZ_OVERRIDE {
    mInnerWindowId = aInnerWindowId;
  }
  virtual uint64_t InnerWindowID() const MOZ_OVERRIDE { return mInnerWindowId; }

  virtual bool HasError() MOZ_OVERRIDE    { return mError; }
  virtual void SetHasError() MOZ_OVERRIDE { mError = true; }

  


  virtual nsIURI* GetURI() MOZ_OVERRIDE { return mURI; }

protected:
  ImageResource(imgStatusTracker* aStatusTracker, nsIURI* aURI);

  
  
  nsresult GetAnimationModeInternal(uint16_t *aAnimationMode);
  nsresult SetAnimationModeInternal(uint16_t aAnimationMode);

  



  virtual void EvaluateAnimation();

  



  virtual bool ShouldAnimate() {
    return mAnimationConsumers > 0 && mAnimationMode != kDontAnimMode;
  }

  virtual nsresult StartAnimation() = 0;
  virtual nsresult StopAnimation() = 0;

  
  nsRefPtr<imgStatusTracker>  mStatusTracker;
  nsCOMPtr<nsIURI>            mURI;
  uint64_t                    mInnerWindowId;
  uint32_t                    mAnimationConsumers;
  uint16_t                    mAnimationMode;   
  bool                        mInitialized:1;   
  bool                        mAnimating:1;     
  bool                        mError:1;         
};

} 
} 

#endif 
