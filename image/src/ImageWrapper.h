




#ifndef mozilla_image_src_ImageWrapper_h
#define mozilla_image_src_ImageWrapper_h

#include "mozilla/MemoryReporting.h"
#include "Image.h"

namespace mozilla {
namespace image {




class ImageWrapper : public Image
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IMGICONTAINER

  
  virtual already_AddRefed<ProgressTracker> GetProgressTracker() override;

  virtual size_t
    SizeOfSourceWithComputedFallback(MallocSizeOf aMallocSizeOf) const override;
  virtual void CollectSizeOfSurfaces(nsTArray<SurfaceMemoryCounter>& aCounters,
                                     MallocSizeOf aMallocSizeOf) const override;

  virtual void IncrementAnimationConsumers() override;
  virtual void DecrementAnimationConsumers() override;
#ifdef DEBUG
  virtual uint32_t GetAnimationConsumers() override;
#endif

  virtual nsresult OnImageDataAvailable(nsIRequest* aRequest,
                                        nsISupports* aContext,
                                        nsIInputStream* aInStr,
                                        uint64_t aSourceOffset,
                                        uint32_t aCount) override;
  virtual nsresult OnImageDataComplete(nsIRequest* aRequest,
                                       nsISupports* aContext,
                                       nsresult aStatus,
                                       bool aLastPart) override;

  virtual void OnSurfaceDiscarded() override;

  virtual void SetInnerWindowID(uint64_t aInnerWindowId) override;
  virtual uint64_t InnerWindowID() const override;

  virtual bool HasError() override;
  virtual void SetHasError() override;

  virtual ImageURL* GetURI() override;

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
