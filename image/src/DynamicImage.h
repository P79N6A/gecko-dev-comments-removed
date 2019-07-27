




#ifndef mozilla_image_src_DynamicImage_h
#define mozilla_image_src_DynamicImage_h

#include "mozilla/MemoryReporting.h"
#include "gfxDrawable.h"
#include "Image.h"

namespace mozilla {
namespace image {






class DynamicImage : public Image
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IMGICONTAINER

  explicit DynamicImage(gfxDrawable* aDrawable)
    : mDrawable(aDrawable)
  {
    MOZ_ASSERT(aDrawable, "Must have a gfxDrawable to wrap");
  }

  
  virtual nsresult Init(const char* aMimeType, uint32_t aFlags) override;

  virtual already_AddRefed<ProgressTracker> GetProgressTracker() override;
  virtual size_t SizeOfSourceWithComputedFallback(
                                 MallocSizeOf aMallocSizeOf) const override;
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

private:
  virtual ~DynamicImage() { }

  nsRefPtr<gfxDrawable> mDrawable;
};

} 
} 

#endif 
