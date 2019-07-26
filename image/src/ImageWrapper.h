




#ifndef MOZILLA_IMAGELIB_IMAGEWRAPPER_H_
#define MOZILLA_IMAGELIB_IMAGEWRAPPER_H_

#include "mozilla/MemoryReporting.h"
#include "Image.h"

namespace mozilla {
namespace image {




class ImageWrapper : public Image
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IMGICONTAINER

  virtual ~ImageWrapper() { }

  
  virtual nsresult Init(const char* aMimeType, uint32_t aFlags) MOZ_OVERRIDE;

  virtual imgStatusTracker& GetStatusTracker() MOZ_OVERRIDE;
  virtual nsIntRect FrameRect(uint32_t aWhichFrame) MOZ_OVERRIDE;

  virtual uint32_t SizeOfData() MOZ_OVERRIDE;
  virtual size_t HeapSizeOfSourceWithComputedFallback(mozilla::MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE;
  virtual size_t HeapSizeOfDecodedWithComputedFallback(mozilla::MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE;
  virtual size_t NonHeapSizeOfDecoded() const MOZ_OVERRIDE;
  virtual size_t OutOfProcessSizeOfDecoded() const MOZ_OVERRIDE;

  virtual void IncrementAnimationConsumers() MOZ_OVERRIDE;
  virtual void DecrementAnimationConsumers() MOZ_OVERRIDE;
#ifdef DEBUG
  virtual uint32_t GetAnimationConsumers() MOZ_OVERRIDE;
#endif

  virtual nsresult OnImageDataAvailable(nsIRequest* aRequest,
                                        nsISupports* aContext,
                                        nsIInputStream* aInStr,
                                        uint64_t aSourceOffset,
                                        uint32_t aCount) MOZ_OVERRIDE;
  virtual nsresult OnImageDataComplete(nsIRequest* aRequest,
                                       nsISupports* aContext,
                                       nsresult aStatus,
                                       bool aLastPart) MOZ_OVERRIDE;
  virtual nsresult OnNewSourceData() MOZ_OVERRIDE;

  virtual void SetInnerWindowID(uint64_t aInnerWindowId) MOZ_OVERRIDE;
  virtual uint64_t InnerWindowID() const MOZ_OVERRIDE;

  virtual bool HasError() MOZ_OVERRIDE;
  virtual void SetHasError() MOZ_OVERRIDE;

  virtual ImageURL* GetURI() MOZ_OVERRIDE;

protected:
  ImageWrapper(Image* aInnerImage)
    : mInnerImage(aInnerImage)
  {
    NS_ABORT_IF_FALSE(aInnerImage, "Cannot wrap a null image");
  }

  


  Image* InnerImage() { return mInnerImage.get(); }

private:
  nsRefPtr<Image> mInnerImage;
};

} 
} 

#endif 
