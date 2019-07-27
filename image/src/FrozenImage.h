




#ifndef mozilla_image_src_FrozenImage_h
#define mozilla_image_src_FrozenImage_h

#include "ImageWrapper.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/RefPtr.h"

namespace mozilla {
namespace image {












class FrozenImage : public ImageWrapper
{
  typedef gfx::SourceSurface SourceSurface;

public:
  NS_DECL_ISUPPORTS_INHERITED

  virtual void IncrementAnimationConsumers() override;
  virtual void DecrementAnimationConsumers() override;

  NS_IMETHOD GetAnimated(bool* aAnimated) override;
  NS_IMETHOD_(TemporaryRef<SourceSurface>)
    GetFrame(uint32_t aWhichFrame, uint32_t aFlags) override;
  NS_IMETHOD_(already_AddRefed<layers::ImageContainer>)
    GetImageContainer(layers::LayerManager* aManager,
                      uint32_t aFlags) override;
  NS_IMETHOD_(DrawResult) Draw(gfxContext* aContext,
                               const nsIntSize& aSize,
                               const ImageRegion& aRegion,
                               uint32_t aWhichFrame,
                               GraphicsFilter aFilter,
                               const Maybe<SVGImageContext>& aSVGContext,
                               uint32_t aFlags) override;
  NS_IMETHOD_(void) RequestRefresh(const TimeStamp& aTime) override;
  NS_IMETHOD GetAnimationMode(uint16_t* aAnimationMode) override;
  NS_IMETHOD SetAnimationMode(uint16_t aAnimationMode) override;
  NS_IMETHOD ResetAnimation() override;
  NS_IMETHOD_(float) GetFrameIndex(uint32_t aWhichFrame) override;

protected:
  explicit FrozenImage(Image* aImage) : ImageWrapper(aImage) { }
  virtual ~FrozenImage() { }

private:
  friend class ImageOps;
};

} 
} 

#endif 
