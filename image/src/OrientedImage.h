




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
  typedef mozilla::gfx::SourceSurface SourceSurface;

public:
  NS_DECL_ISUPPORTS

  virtual nsIntRect FrameRect(uint32_t aWhichFrame) MOZ_OVERRIDE;

  NS_IMETHOD GetWidth(int32_t* aWidth) MOZ_OVERRIDE;
  NS_IMETHOD GetHeight(int32_t* aHeight) MOZ_OVERRIDE;
  NS_IMETHOD GetIntrinsicSize(nsSize* aSize) MOZ_OVERRIDE;
  NS_IMETHOD GetIntrinsicRatio(nsSize* aRatio) MOZ_OVERRIDE;
  NS_IMETHOD_(mozilla::TemporaryRef<SourceSurface>)
    GetFrame(uint32_t aWhichFrame, uint32_t aFlags) MOZ_OVERRIDE;
  NS_IMETHOD GetImageContainer(mozilla::layers::LayerManager* aManager,
                               mozilla::layers::ImageContainer** _retval) MOZ_OVERRIDE;
  NS_IMETHOD Draw(gfxContext* aContext,
                  GraphicsFilter aFilter,
                  const gfxMatrix& aUserSpaceToImageSpace,
                  const gfxRect& aFill,
                  const nsIntRect& aSubimage,
                  const nsIntSize& aViewportSize,
                  const SVGImageContext* aSVGContext,
                  uint32_t aWhichFrame,
                  uint32_t aFlags) MOZ_OVERRIDE;
  NS_IMETHOD_(nsIntRect) GetImageSpaceInvalidationRect(const nsIntRect& aRect) MOZ_OVERRIDE;
 
protected:
  OrientedImage(Image* aImage, Orientation aOrientation)
    : ImageWrapper(aImage)
    , mOrientation(aOrientation)
  { }

  virtual ~OrientedImage() { }

  gfxMatrix OrientationMatrix(const nsIntSize& aViewportSize);

private:
  Orientation mOrientation;

  friend class ImageOps;
};

} 
} 

#endif 
