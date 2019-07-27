




#include <algorithm>

#include "gfx2DGlue.h"
#include "gfxDrawable.h"
#include "gfxPlatform.h"
#include "gfxUtils.h"
#include "ImageRegion.h"
#include "SVGImageContext.h"

#include "OrientedImage.h"

using std::swap;

namespace mozilla {

using namespace gfx;
using layers::LayerManager;
using layers::ImageContainer;

namespace image {

NS_IMPL_ISUPPORTS_INHERITED0(OrientedImage, ImageWrapper)

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

  
  gfxIntSize size;
  rv = InnerImage()->GetWidth(&size.width);
  NS_ENSURE_SUCCESS(rv, nullptr);
  rv = InnerImage()->GetHeight(&size.height);
  NS_ENSURE_SUCCESS(rv, nullptr);

  
  gfx::SurfaceFormat surfaceFormat;
  if (InnerImage()->IsOpaque()) {
    surfaceFormat = gfx::SurfaceFormat::B8G8R8X8;
  } else {
    surfaceFormat = gfx::SurfaceFormat::B8G8R8A8;
  }

  
  RefPtr<DrawTarget> target =
    gfxPlatform::GetPlatform()->
      CreateOffscreenContentDrawTarget(ToIntSize(size), surfaceFormat);
  if (!target) {
    NS_ERROR("Could not create a DrawTarget");
    return nullptr;
  }


  
  RefPtr<SourceSurface> innerSurface =
    InnerImage()->GetFrame(aWhichFrame, aFlags);
  NS_ENSURE_TRUE(innerSurface, nullptr);
  nsRefPtr<gfxDrawable> drawable =
    new gfxSurfaceDrawable(innerSurface, size);

  
  nsRefPtr<gfxContext> ctx = new gfxContext(target);
  ctx->Multiply(OrientationMatrix(size));
  gfxUtils::DrawPixelSnapped(ctx, drawable, size,
                             ImageRegion::Create(size),
                             surfaceFormat, GraphicsFilter::FILTER_FAST);

  return target->Snapshot();
}

NS_IMETHODIMP_(already_AddRefed<ImageContainer>)
OrientedImage::GetImageContainer(LayerManager* aManager, uint32_t aFlags)
{
  
  
  
  
  

  if (mOrientation.IsIdentity()) {
    return InnerImage()->GetImageContainer(aManager, aFlags);
  }

  return nullptr;
}

struct MatrixBuilder
{
  explicit MatrixBuilder(bool aInvert) : mInvert(aInvert) { }

  gfxMatrix Build() { return mMatrix; }

  void Scale(gfxFloat aX, gfxFloat aY)
  {
    if (mInvert) {
      mMatrix *= gfxMatrix::Scaling(1.0 / aX, 1.0 / aY);
    } else {
      mMatrix.Scale(aX, aY);
    }
  }

  void Rotate(gfxFloat aPhi)
  {
    if (mInvert) {
      mMatrix *= gfxMatrix::Rotation(-aPhi);
    } else {
      mMatrix.Rotate(aPhi);
    }
  }

  void Translate(gfxPoint aDelta)
  {
    if (mInvert) {
      mMatrix *= gfxMatrix::Translation(-aDelta);
    } else {
      mMatrix.Translate(aDelta);
    }
  }

private:
  gfxMatrix mMatrix;
  bool      mInvert;
};














gfxMatrix
OrientedImage::OrientationMatrix(const nsIntSize& aSize,
                                 bool aInvert )
{
  MatrixBuilder builder(aInvert);

  
  
  
  switch (mOrientation.flip) {
    case Flip::Unflipped:
      break;
    case Flip::Horizontal:
      if (mOrientation.SwapsWidthAndHeight()) {
        builder.Translate(gfxPoint(aSize.height, 0));
      } else {
        builder.Translate(gfxPoint(aSize.width, 0));
      }
      builder.Scale(-1.0, 1.0);
      break;
    default:
      MOZ_ASSERT(false, "Invalid flip value");
  }

  
  
  switch (mOrientation.rotation) {
    case Angle::D0:
      break;
    case Angle::D90:
      builder.Translate(gfxPoint(aSize.height, 0));
      builder.Rotate(-1.5 * M_PI);
      break;
    case Angle::D180:
      builder.Translate(gfxPoint(aSize.width, aSize.height));
      builder.Rotate(-1.0 * M_PI);
      break;
    case Angle::D270:
      builder.Translate(gfxPoint(0, aSize.width));
      builder.Rotate(-0.5 * M_PI);
      break;
    default:
      MOZ_ASSERT(false, "Invalid rotation value");
  }

  return builder.Build();
}

static SVGImageContext
OrientViewport(const SVGImageContext& aOldContext,
               const Orientation& aOrientation)
{
  CSSIntSize viewportSize(aOldContext.GetViewportSize());
  if (aOrientation.SwapsWidthAndHeight()) {
    swap(viewportSize.width, viewportSize.height);
  }
  return SVGImageContext(viewportSize,
                         aOldContext.GetPreserveAspectRatio());
}

NS_IMETHODIMP_(DrawResult)
OrientedImage::Draw(gfxContext* aContext,
                    const nsIntSize& aSize,
                    const ImageRegion& aRegion,
                    uint32_t aWhichFrame,
                    GraphicsFilter aFilter,
                    const Maybe<SVGImageContext>& aSVGContext,
                    uint32_t aFlags)
{
  if (mOrientation.IsIdentity()) {
    return InnerImage()->Draw(aContext, aSize, aRegion,
                              aWhichFrame, aFilter, aSVGContext, aFlags);
  }

  
  
  nsIntSize size(aSize);
  if (mOrientation.SwapsWidthAndHeight()) {
    swap(size.width, size.height);
  }

  
  
  gfxMatrix matrix(OrientationMatrix(size));
  gfxContextMatrixAutoSaveRestore saveMatrix(aContext);
  aContext->Multiply(matrix);

  
  
  
  gfxMatrix inverseMatrix(OrientationMatrix(size,  true));
  ImageRegion region(aRegion);
  region.TransformBoundsBy(inverseMatrix);

  return InnerImage()->Draw(aContext, size, region, aWhichFrame, aFilter,
                            aSVGContext.map(OrientViewport, mOrientation),
                            aFlags);
}

nsIntSize
OrientedImage::OptimalImageSizeForDest(const gfxSize& aDest,
                                       uint32_t aWhichFrame,
                                       GraphicsFilter aFilter, uint32_t aFlags)
{
  if (!mOrientation.SwapsWidthAndHeight()) {
    return InnerImage()->OptimalImageSizeForDest(aDest, aWhichFrame, aFilter,
                                                 aFlags);
  }

  
  gfxSize destSize(aDest.height, aDest.width);
  nsIntSize innerImageSize(InnerImage()->OptimalImageSizeForDest(destSize,
                                                                 aWhichFrame,
                                                                 aFilter,
                                                                 aFlags));
  return nsIntSize(innerImageSize.height, innerImageSize.width);
}

NS_IMETHODIMP_(nsIntRect)
OrientedImage::GetImageSpaceInvalidationRect(const nsIntRect& aRect)
{
  nsIntRect rect(InnerImage()->GetImageSpaceInvalidationRect(aRect));

  if (mOrientation.IsIdentity()) {
    return rect;
  }

  nsIntSize innerSize;
  nsresult rv = InnerImage()->GetWidth(&innerSize.width);
  rv = NS_FAILED(rv) ? rv : InnerImage()->GetHeight(&innerSize.height);
  if (NS_FAILED(rv)) {
    
    return rect;
  }

  
  gfxMatrix matrix(OrientationMatrix(innerSize,  true));
  gfxRect invalidRect(matrix.TransformBounds(gfxRect(rect.x, rect.y,
                                                     rect.width, rect.height)));
  invalidRect.RoundOut();

  return nsIntRect(invalidRect.x, invalidRect.y,
                   invalidRect.width, invalidRect.height);
}

} 
} 
