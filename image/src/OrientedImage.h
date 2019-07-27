




#ifndef MOZILLA_IMAGELIB_ORIENTEDIMAGE_H_
#define MOZILLA_IMAGELIB_ORIENTEDIMAGE_H_

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

  virtual nsIntRect FrameRect(uint32_t aWhichFrame) MOZ_OVERRIDE;

  NS_IMETHOD GetWidth(int32_t* aWidth) MOZ_OVERRIDE;
  NS_IMETHOD GetHeight(int32_t* aHeight) MOZ_OVERRIDE;
  NS_IMETHOD GetIntrinsicSize(nsSize* aSize) MOZ_OVERRIDE;
  NS_IMETHOD GetIntrinsicRatio(nsSize* aRatio) MOZ_OVERRIDE;
  NS_IMETHOD_(TemporaryRef<SourceSurface>)
    GetFrame(uint32_t aWhichFrame, uint32_t aFlags) MOZ_OVERRIDE;
  NS_IMETHOD GetImageContainer(layers::LayerManager* aManager,
                               layers::ImageContainer** _retval) MOZ_OVERRIDE;
  NS_IMETHOD Draw(gfxContext* aContext,
                  const nsIntSize& aSize,
                  const ImageRegion& aRegion,
                  uint32_t aWhichFrame,
                  GraphicsFilter aFilter,
                  const Maybe<SVGImageContext>& aSVGContext,
                  uint32_t aFlags) MOZ_OVERRIDE;
  NS_IMETHOD_(nsIntRect) GetImageSpaceInvalidationRect(const nsIntRect& aRect) MOZ_OVERRIDE;
  nsIntSize OptimalImageSizeForDest(const gfxSize& aDest,
                                    uint32_t aWhichFrame,
                                    GraphicsFilter aFilter,
                                    uint32_t aFlags) MOZ_OVERRIDE;
 
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
