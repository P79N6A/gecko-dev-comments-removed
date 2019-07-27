




#ifndef mozilla_image_src_ClippedImage_h
#define mozilla_image_src_ClippedImage_h

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
  NS_IMETHOD RequestDiscard() override;
  NS_IMETHOD_(Orientation) GetOrientation() override;
  NS_IMETHOD_(nsIntRect) GetImageSpaceInvalidationRect(const nsIntRect& aRect)
    override;
  nsIntSize OptimalImageSizeForDest(const gfxSize& aDest,
                                    uint32_t aWhichFrame,
                                    GraphicsFilter aFilter,
                                    uint32_t aFlags) override;

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
