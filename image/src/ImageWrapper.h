




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

  
  virtual nsresult Init(const char* aMimeType, uint32_t aFlags) MOZ_OVERRIDE;

  virtual already_AddRefed<ProgressTracker> GetProgressTracker() MOZ_OVERRIDE;
  virtual nsIntRect FrameRect(uint32_t aWhichFrame) MOZ_OVERRIDE;

  virtual size_t
  SizeOfSourceWithComputedFallback( MallocSizeOf aMallocSizeOf) const
      MOZ_OVERRIDE;
  virtual size_t
  SizeOfDecoded(gfxMemoryLocation aLocation,
                MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE;

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

  virtual void OnSurfaceDiscarded() MOZ_OVERRIDE;

  virtual void SetInnerWindowID(uint64_t aInnerWindowId) MOZ_OVERRIDE;
  virtual uint64_t InnerWindowID() const MOZ_OVERRIDE;

  virtual bool HasError() MOZ_OVERRIDE;
  virtual void SetHasError() MOZ_OVERRIDE;

  virtual ImageURL* GetURI() MOZ_OVERRIDE;

protected:
  explicit ImageWrapper(Image* aInnerImage)
    : mInnerImage(aInnerImage)
  {
    MOZ_ASSERT(aInnerImage, "Need an image to wrap");
  }

  virtual ~ImageWrapper() { }

  


  Image* InnerImage() const { return mInnerImage.get(); }

  void SetInnerImage(Image* aInnerImage)
  {
    MOZ_ASSERT(aInnerImage, "Need an image to wrap");
    mInnerImage = aInnerImage;
  }

private:
  nsRefPtr<Image> mInnerImage;
};

} 
} 

#endif 
