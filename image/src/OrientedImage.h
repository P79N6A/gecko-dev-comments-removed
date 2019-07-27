




#ifndef mozilla_image_src_OrientedImage_h
#define mozilla_image_src_OrientedImage_h

#include "ImageWrapper.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/RefPtr.h"
#include "Orientation.h"

namespace mozilla {
namespace image {








class OrientedImage : public ImageWrapper
{
  typedef gfx::SourceSurface SourceSurface;

public:
  NS_DECL_ISUPPORTS_INHERITED

  NS_IMETHOD GetWidth(int32_t* aWidth) override;
  NS_IMETHOD GetHeight(int32_t* aHeight) override;
  NS_IMETHOD GetIntrinsicSize(nsSize* aSize) override;
  NS_IMETHOD GetIntrinsicRatio(nsSize* aRatio) override;
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
  NS_IMETHOD_(nsIntRect) GetImageSpaceInvalidationRect(
                                           const nsIntRect& aRect) override;
  nsIntSize OptimalImageSizeForDest(const gfxSize& aDest,
                                    uint32_t aWhichFrame,
                                    GraphicsFilter aFilter,
                                    uint32_t aFlags) override;

protected:
  OrientedImage(Image* aImage, Orientation aOrientation)
    : ImageWrapper(aImage)
    , mOrientation(aOrientation)
  { }

  virtual ~OrientedImage() { }

  gfxMatrix OrientationMatrix(const nsIntSize& aSize, bool aInvert = false);

private:
  Orientation mOrientation;

  friend class ImageOps;
};

} 
} 

#endif 
