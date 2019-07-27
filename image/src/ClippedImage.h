




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
  typedef mozilla::gfx::SourceSurface SourceSurface;

public:
  NS_DECL_ISUPPORTS_INHERITED

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
  NS_IMETHOD RequestDiscard() MOZ_OVERRIDE;
  NS_IMETHOD_(Orientation) GetOrientation() MOZ_OVERRIDE;
  NS_IMETHOD_(nsIntRect) GetImageSpaceInvalidationRect(const nsIntRect& aRect) MOZ_OVERRIDE;

protected:
  ClippedImage(Image* aImage, nsIntRect aClip);

  virtual ~ClippedImage();

private:
  mozilla::TemporaryRef<SourceSurface>
    GetFrameInternal(const nsIntSize& aViewportSize,
                     const SVGImageContext* aSVGContext,
                     uint32_t aWhichFrame,
                     uint32_t aFlags);
  bool ShouldClip();
  bool MustCreateSurface(gfxContext* aContext,
                         const gfxMatrix& aTransform,
                         const gfxRect& aSourceRect,
                         const nsIntRect& aSubimage,
                         const uint32_t aFlags) const;
  gfxFloat ClampFactor(const gfxFloat aToClamp, const int aReference) const;
  nsresult DrawSingleTile(gfxContext* aContext,
                          GraphicsFilter aFilter,
                          const gfxMatrix& aUserSpaceToImageSpace,
                          const gfxRect& aFill,
                          const nsIntRect& aSubimage,
                          const nsIntSize& aViewportSize,
                          const SVGImageContext* aSVGContext,
                          uint32_t aWhichFrame,
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
