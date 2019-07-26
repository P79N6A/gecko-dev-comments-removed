




#ifndef MOZILLA_IMAGELIB_CLIPPEDIMAGE_H_
#define MOZILLA_IMAGELIB_CLIPPEDIMAGE_H_

#include "ImageWrapper.h"
#include "mozilla/Maybe.h"

namespace mozilla {
namespace image {

class ClippedImageCachedSurface;
class DrawSingleTileCallback;








class ClippedImage : public ImageWrapper
{
public:
  NS_DECL_ISUPPORTS

  virtual ~ClippedImage();

  virtual nsIntRect FrameRect(uint32_t aWhichFrame) MOZ_OVERRIDE;

  NS_IMETHOD GetWidth(int32_t* aWidth) MOZ_OVERRIDE;
  NS_IMETHOD GetHeight(int32_t* aHeight) MOZ_OVERRIDE;
  NS_IMETHOD GetIntrinsicSize(nsSize* aSize) MOZ_OVERRIDE;
  NS_IMETHOD GetIntrinsicRatio(nsSize* aRatio) MOZ_OVERRIDE;
  NS_IMETHOD GetFrame(uint32_t aWhichFrame,
                      uint32_t aFlags,
                      gfxASurface** _retval) MOZ_OVERRIDE;
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

protected:
  ClippedImage(Image* aImage, nsIntRect aClip);

private:
  nsresult GetFrameInternal(const nsIntSize& aViewportSize,
                            const SVGImageContext* aSVGContext,
                            uint32_t aWhichFrame,
                            uint32_t aFlags,
                            gfxASurface** _retval);
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
