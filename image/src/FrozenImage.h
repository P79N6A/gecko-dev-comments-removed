




#ifndef MOZILLA_IMAGELIB_FROZENIMAGE_H_
#define MOZILLA_IMAGELIB_FROZENIMAGE_H_

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

  virtual nsIntRect FrameRect(uint32_t aWhichFrame) MOZ_OVERRIDE;
  virtual void IncrementAnimationConsumers() MOZ_OVERRIDE;
  virtual void DecrementAnimationConsumers() MOZ_OVERRIDE;

  NS_IMETHOD GetAnimated(bool* aAnimated) MOZ_OVERRIDE;
  NS_IMETHOD_(TemporaryRef<SourceSurface>)
    GetFrame(uint32_t aWhichFrame, uint32_t aFlags) MOZ_OVERRIDE;
  NS_IMETHOD_(bool) FrameIsOpaque(uint32_t aWhichFrame) MOZ_OVERRIDE;
  NS_IMETHOD GetImageContainer(layers::LayerManager* aManager,
                               layers::ImageContainer** _retval) MOZ_OVERRIDE;
  NS_IMETHOD Draw(gfxContext* aContext,
                  GraphicsFilter aFilter,
                  const gfxMatrix& aUserSpaceToImageSpace,
                  const gfxRect& aFill,
                  const nsIntRect& aSubimage,
                  const nsIntSize& aViewportSize,
                  const SVGImageContext* aSVGContext,
                  uint32_t aWhichFrame,
                  uint32_t aFlags) MOZ_OVERRIDE;
  NS_IMETHOD_(void) RequestRefresh(const TimeStamp& aTime) MOZ_OVERRIDE;
  NS_IMETHOD GetAnimationMode(uint16_t* aAnimationMode) MOZ_OVERRIDE;
  NS_IMETHOD SetAnimationMode(uint16_t aAnimationMode) MOZ_OVERRIDE;
  NS_IMETHOD ResetAnimation() MOZ_OVERRIDE;
  NS_IMETHOD_(float) GetFrameIndex(uint32_t aWhichFrame) MOZ_OVERRIDE;

protected:
  FrozenImage(Image* aImage) : ImageWrapper(aImage) { }
  virtual ~FrozenImage() { }

private:
  friend class ImageOps;
};

} 
} 

#endif 
