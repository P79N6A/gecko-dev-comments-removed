




#ifndef MOZILLA_IMAGELIB_FROZENIMAGE_H_
#define MOZILLA_IMAGELIB_FROZENIMAGE_H_

#include "ImageWrapper.h"

namespace mozilla {
namespace image {












class FrozenImage : public ImageWrapper
{
public:
  NS_DECL_ISUPPORTS

  virtual ~FrozenImage() { }

  virtual nsIntRect FrameRect(uint32_t aWhichFrame) MOZ_OVERRIDE;
  virtual void IncrementAnimationConsumers() MOZ_OVERRIDE;
  virtual void DecrementAnimationConsumers() MOZ_OVERRIDE;

  NS_IMETHOD GetAnimated(bool* aAnimated) MOZ_OVERRIDE;
  NS_IMETHOD GetFrame(uint32_t aWhichFrame,
                      uint32_t aFlags,
                      gfxASurface** _retval) MOZ_OVERRIDE;
  NS_IMETHOD_(bool) FrameIsOpaque(uint32_t aWhichFrame) MOZ_OVERRIDE;
  NS_IMETHOD GetImageContainer(layers::LayerManager* aManager,
                               layers::ImageContainer** _retval) MOZ_OVERRIDE;
  NS_IMETHOD ExtractFrame(uint32_t aWhichFrame,
                          const nsIntRect& aRegion,
                          uint32_t aFlags,
                          imgIContainer** _retval) MOZ_OVERRIDE;
  NS_IMETHOD Draw(gfxContext* aContext,
                  gfxPattern::GraphicsFilter aFilter,
                  const gfxMatrix& aUserSpaceToImageSpace,
                  const gfxRect& aFill,
                  const nsIntRect& aSubimage,
                  const nsIntSize& aViewportSize,
                  const SVGImageContext* aSVGContext,
                  uint32_t aWhichFrame,
                  uint32_t aFlags) MOZ_OVERRIDE;
  NS_IMETHOD_(void) RequestRefresh(const mozilla::TimeStamp& aTime) MOZ_OVERRIDE;
  NS_IMETHOD GetAnimationMode(uint16_t* aAnimationMode) MOZ_OVERRIDE;
  NS_IMETHOD SetAnimationMode(uint16_t aAnimationMode) MOZ_OVERRIDE;
  NS_IMETHOD ResetAnimation() MOZ_OVERRIDE;

protected:
  FrozenImage(Image* aImage) : ImageWrapper(aImage) { }

private:
  friend class ImageFactory;
};

} 
} 

#endif 
