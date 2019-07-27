




#ifndef MOZILLA_IMAGELIB_CLIPPEDIMAGE_H_
#define MOZILLA_IMAGELIB_CLIPPEDIMAGE_H_

#include "ImageWrapper.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/Maybe.h"
#include "mozilla/RefPtr.h"

namespace mozilla {
namespace image {

class ClippedImageCachedSurface;
class DrawSingleTileCallback;








class ClippedImage : public ImageWrapper
{
  typedef gfx::SourceSurface SourceSurface;

public:
  NS_DECL_ISUPPORTS_INHERITED

  NS_IMETHOD GetWidth(int32_t* aWidth) MOZ_OVERRIDE;
  NS_IMETHOD GetHeight(int32_t* aHeight) MOZ_OVERRIDE;
  NS_IMETHOD GetIntrinsicSize(nsSize* aSize) MOZ_OVERRIDE;
  NS_IMETHOD GetIntrinsicRatio(nsSize* aRatio) MOZ_OVERRIDE;
  NS_IMETHOD_(TemporaryRef<SourceSurface>)
    GetFrame(uint32_t aWhichFrame, uint32_t aFlags) MOZ_OVERRIDE;
  NS_IMETHOD GetImageContainer(layers::LayerManager* aManager,
                               layers::ImageContainer** _retval) MOZ_OVERRIDE;
  NS_IMETHOD_(DrawResult) Draw(gfxContext* aContext,
                               const nsIntSize& aSize,
                               const ImageRegion& aRegion,
                               uint32_t aWhichFrame,
                               GraphicsFilter aFilter,
                               const Maybe<SVGImageContext>& aSVGContext,
                               uint32_t aFlags) MOZ_OVERRIDE;
  NS_IMETHOD RequestDiscard() MOZ_OVERRIDE;
  NS_IMETHOD_(Orientation) GetOrientation() MOZ_OVERRIDE;
  NS_IMETHOD_(nsIntRect) GetImageSpaceInvalidationRect(const nsIntRect& aRect) MOZ_OVERRIDE;
  nsIntSize OptimalImageSizeForDest(const gfxSize& aDest,
                                    uint32_t aWhichFrame,
                                    GraphicsFilter aFilter,
                                    uint32_t aFlags) MOZ_OVERRIDE;

protected:
  ClippedImage(Image* aImage, nsIntRect aClip);

  virtual ~ClippedImage();

private:
  TemporaryRef<SourceSurface>
    GetFrameInternal(const nsIntSize& aSize,
                     const Maybe<SVGImageContext>& aSVGContext,
                     uint32_t aWhichFrame,
                     uint32_t aFlags);
  bool ShouldClip();
  DrawResult DrawSingleTile(gfxContext* aContext,
                            const nsIntSize& aSize,
                            const ImageRegion& aRegion,
                            uint32_t aWhichFrame,
                            GraphicsFilter aFilter,
                            const Maybe<SVGImageContext>& aSVGContext,
                            uint32_t aFlags);

  
  nsAutoPtr<ClippedImageCachedSurface> mCachedSurface;

  nsIntRect   mClip;              
  Maybe<bool> mShouldClip;        

  friend class DrawSingleTileCallback;
  friend class ImageOps;
};

} 
} 

#endif 
