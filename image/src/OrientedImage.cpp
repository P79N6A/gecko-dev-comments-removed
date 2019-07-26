




#include <algorithm>

#include "gfxDrawable.h"
#include "gfxPlatform.h"
#include "gfxUtils.h"

#include "OrientedImage.h"

using namespace mozilla::gfx;

using std::swap;
using mozilla::layers::LayerManager;
using mozilla::layers::ImageContainer;

namespace mozilla {
namespace image {

NS_IMPL_ISUPPORTS(OrientedImage, imgIContainer)

nsIntRect
OrientedImage::FrameRect(uint32_t aWhichFrame)
{
  if (mOrientation.SwapsWidthAndHeight()) {
    nsIntRect innerRect = InnerImage()->FrameRect(aWhichFrame);
    return nsIntRect(innerRect.x, innerRect.y, innerRect.height, innerRect.width);
  } else {
    return InnerImage()->FrameRect(aWhichFrame);
  }
}

NS_IMETHODIMP
OrientedImage::GetWidth(int32_t* aWidth)
{
  if (mOrientation.SwapsWidthAndHeight()) {
    return InnerImage()->GetHeight(aWidth);
  } else {
    return InnerImage()->GetWidth(aWidth);
  }
}

NS_IMETHODIMP
OrientedImage::GetHeight(int32_t* aHeight)
{
  if (mOrientation.SwapsWidthAndHeight()) {
    return InnerImage()->GetWidth(aHeight);
  } else {
    return InnerImage()->GetHeight(aHeight);
  }
}

NS_IMETHODIMP
OrientedImage::GetIntrinsicSize(nsSize* aSize)
{
  nsresult rv = InnerImage()->GetIntrinsicSize(aSize);

  if (mOrientation.SwapsWidthAndHeight()) {
    swap(aSize->width, aSize->height);
  }

  return rv;
}

NS_IMETHODIMP
OrientedImage::GetIntrinsicRatio(nsSize* aRatio)
{
  nsresult rv = InnerImage()->GetIntrinsicRatio(aRatio);

  if (mOrientation.SwapsWidthAndHeight()) {
    swap(aRatio->width, aRatio->height);
  }

  return rv;
}

NS_IMETHODIMP_(TemporaryRef<SourceSurface>)
OrientedImage::GetFrame(uint32_t aWhichFrame,
                        uint32_t aFlags)
{
  nsresult rv;

  if (mOrientation.IsIdentity()) {
    return InnerImage()->GetFrame(aWhichFrame, aFlags);
  }

  
  int32_t width, height;
  rv = InnerImage()->GetWidth(&width);
  NS_ENSURE_SUCCESS(rv, nullptr);
  rv = InnerImage()->GetHeight(&height);
  NS_ENSURE_SUCCESS(rv, nullptr);
  if (mOrientation.SwapsWidthAndHeight()) {
    swap(width, height);
  }
  NS_ENSURE_SUCCESS(rv, nullptr);

  
  gfx::SurfaceFormat surfaceFormat;
  gfxImageFormat imageFormat;
  if (InnerImage()->FrameIsOpaque(aWhichFrame)) {
    surfaceFormat = gfx::SurfaceFormat::B8G8R8X8;
    imageFormat = gfxImageFormat::ARGB32;
  } else {
    surfaceFormat = gfx::SurfaceFormat::B8G8R8A8;
    imageFormat = gfxImageFormat::ARGB32;
  }

  
  mozilla::RefPtr<DrawTarget> target =
    gfxPlatform::GetPlatform()->
      CreateOffscreenContentDrawTarget(IntSize(width, height), surfaceFormat);
  if (!target) {
    NS_ERROR("Could not create a DrawTarget");
    return nullptr;
  }


  
  RefPtr<SourceSurface> innerSurface =
    InnerImage()->GetFrame(aWhichFrame, aFlags);
  NS_ENSURE_TRUE(innerSurface, nullptr);
  nsRefPtr<gfxDrawable> drawable =
    new gfxSurfaceDrawable(innerSurface, gfxIntSize(width, height));

  
  nsRefPtr<gfxContext> ctx = new gfxContext(target);
  gfxRect imageRect(0, 0, width, height);
  gfxUtils::DrawPixelSnapped(ctx, drawable, OrientationMatrix(nsIntSize(width, height)),
                             imageRect, imageRect, imageRect, imageRect,
                             imageFormat, GraphicsFilter::FILTER_FAST);
  
  return target->Snapshot();
}

NS_IMETHODIMP
OrientedImage::GetImageContainer(LayerManager* aManager, ImageContainer** _retval)
{
  
  
  
  
  

  if (mOrientation.IsIdentity()) {
    return InnerImage()->GetImageContainer(aManager, _retval);
  }

  *_retval = nullptr;
  return NS_OK;
}

gfxMatrix
OrientedImage::OrientationMatrix(const nsIntSize& aViewportSize)
{
    gfxMatrix matrix;

    int32_t width, height;
    if (InnerImage()->GetType() == imgIContainer::TYPE_VECTOR) {
      width = mOrientation.SwapsWidthAndHeight() ? aViewportSize.height : aViewportSize.width;
      height = mOrientation.SwapsWidthAndHeight() ? aViewportSize.width : aViewportSize.height;
    } else {
      nsresult rv = InnerImage()->GetWidth(&width);
      rv = NS_FAILED(rv) ? rv : InnerImage()->GetHeight(&height);
      if (NS_FAILED(rv)) {
        
        return matrix;
      }
    }

    
    
    
    switch (mOrientation.flip) {
      case Flip::Unflipped:
        break;
      case Flip::Horizontal:
        if (mOrientation.SwapsWidthAndHeight()) {
          
          
          matrix.Translate(gfxPoint(0, height));
          matrix.Scale(1.0, -1.0);
        } else {
          matrix.Translate(gfxPoint(width, 0));
          matrix.Scale(-1.0, 1.0);
        }
        break;
      default:
        MOZ_ASSERT(false, "Invalid flip value");
    }

    
    
    switch (mOrientation.rotation) {
      case Angle::D0:
        break;
      case Angle::D90:
        matrix.Translate(gfxPoint(0, height));
        matrix.Rotate(-0.5 * M_PI);
        break;
      case Angle::D180:
        matrix.Translate(gfxPoint(width, height));
        matrix.Rotate(-1.0 * M_PI);
        break;
      case Angle::D270:
        matrix.Translate(gfxPoint(width, 0));
        matrix.Rotate(-1.5 * M_PI);
        break;
      default:
        MOZ_ASSERT(false, "Invalid rotation value");
    }

    return matrix;
}

NS_IMETHODIMP
OrientedImage::Draw(gfxContext* aContext,
                    GraphicsFilter aFilter,
                    const gfxMatrix& aUserSpaceToImageSpace,
                    const gfxRect& aFill,
                    const nsIntRect& aSubimage,
                    const nsIntSize& aViewportSize,
                    const SVGImageContext* aSVGContext,
                    uint32_t aWhichFrame,
                    uint32_t aFlags)
{
  if (mOrientation.IsIdentity()) {
    return InnerImage()->Draw(aContext, aFilter, aUserSpaceToImageSpace,
                              aFill, aSubimage, aViewportSize, aSVGContext,
                              aWhichFrame, aFlags);
  }

  
  gfxMatrix matrix(OrientationMatrix(aViewportSize));
  gfxMatrix userSpaceToImageSpace(aUserSpaceToImageSpace * matrix);

  
  gfxRect gfxSubimage(matrix.TransformBounds(gfxRect(aSubimage.x, aSubimage.y, aSubimage.width, aSubimage.height)));
  nsIntRect subimage(gfxSubimage.x, gfxSubimage.y, gfxSubimage.width, gfxSubimage.height);

  
  
  nsIntSize viewportSize(aViewportSize);
  if (mOrientation.SwapsWidthAndHeight()) {
    swap(viewportSize.width, viewportSize.height);
  }

  return InnerImage()->Draw(aContext, aFilter, userSpaceToImageSpace,
                            aFill, subimage, viewportSize, aSVGContext,
                            aWhichFrame, aFlags);
}

} 
} 
