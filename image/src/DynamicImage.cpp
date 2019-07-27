




#include "DynamicImage.h"
#include "gfxPlatform.h"
#include "gfxUtils.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/RefPtr.h"
#include "ImageRegion.h"
#include "Orientation.h"
#include "SVGImageContext.h"

#include "mozilla/MemoryReporting.h"

using namespace mozilla;
using namespace mozilla::gfx;
using mozilla::layers::LayerManager;
using mozilla::layers::ImageContainer;

namespace mozilla {
namespace image {



already_AddRefed<ProgressTracker>
DynamicImage::GetProgressTracker()
{
  return nullptr;
}

size_t
DynamicImage::SizeOfSourceWithComputedFallback(MallocSizeOf aMallocSizeOf) const
{
  return 0;
}

void
DynamicImage::CollectSizeOfSurfaces(nsTArray<SurfaceMemoryCounter>& aCounters,
                                    MallocSizeOf aMallocSizeOf) const
{
  
  
}

void
DynamicImage::IncrementAnimationConsumers()
{ }

void
DynamicImage::DecrementAnimationConsumers()
{ }

#ifdef DEBUG
uint32_t
DynamicImage::GetAnimationConsumers()
{
  return 0;
}
#endif

nsresult
DynamicImage::OnImageDataAvailable(nsIRequest* aRequest,
                                   nsISupports* aContext,
                                   nsIInputStream* aInStr,
                                   uint64_t aSourceOffset,
                                   uint32_t aCount)
{
  return NS_OK;
}

nsresult
DynamicImage::OnImageDataComplete(nsIRequest* aRequest,
                                  nsISupports* aContext,
                                  nsresult aStatus,
                                  bool aLastPart)
{
  return NS_OK;
}

void
DynamicImage::OnSurfaceDiscarded()
{ }

void
DynamicImage::SetInnerWindowID(uint64_t aInnerWindowId)
{ }

uint64_t
DynamicImage::InnerWindowID() const
{
  return 0;
}

bool
DynamicImage::HasError()
{
  return !mDrawable;
}

void
DynamicImage::SetHasError()
{ }

ImageURL*
DynamicImage::GetURI()
{
  return nullptr;
}



NS_IMPL_ISUPPORTS(DynamicImage, imgIContainer)

NS_IMETHODIMP
DynamicImage::GetWidth(int32_t* aWidth)
{
  *aWidth = mDrawable->Size().width;
  return NS_OK;
}

NS_IMETHODIMP
DynamicImage::GetHeight(int32_t* aHeight)
{
  *aHeight = mDrawable->Size().height;
  return NS_OK;
}

NS_IMETHODIMP
DynamicImage::GetIntrinsicSize(nsSize* aSize)
{
  gfxIntSize intSize(mDrawable->Size());
  *aSize = nsSize(intSize.width, intSize.height);
  return NS_OK;
}

NS_IMETHODIMP
DynamicImage::GetIntrinsicRatio(nsSize* aSize)
{
  gfxIntSize intSize(mDrawable->Size());
  *aSize = nsSize(intSize.width, intSize.height);
  return NS_OK;
}

NS_IMETHODIMP_(Orientation)
DynamicImage::GetOrientation()
{
  return Orientation();
}

NS_IMETHODIMP
DynamicImage::GetType(uint16_t* aType)
{
  *aType = imgIContainer::TYPE_RASTER;
  return NS_OK;
}

NS_IMETHODIMP
DynamicImage::GetAnimated(bool* aAnimated)
{
  *aAnimated = false;
  return NS_OK;
}

NS_IMETHODIMP_(TemporaryRef<SourceSurface>)
DynamicImage::GetFrame(uint32_t aWhichFrame,
                       uint32_t aFlags)
{
  gfxIntSize size(mDrawable->Size());

  RefPtr<DrawTarget> dt = gfxPlatform::GetPlatform()->
    CreateOffscreenContentDrawTarget(IntSize(size.width, size.height),
                                     SurfaceFormat::B8G8R8A8);
  if (!dt) {
    gfxWarning() <<
      "DynamicImage::GetFrame failed in CreateOffscreenContentDrawTarget";
    return nullptr;
  }
  nsRefPtr<gfxContext> context = new gfxContext(dt);

  auto result = Draw(context, size, ImageRegion::Create(size),
                     aWhichFrame, GraphicsFilter::FILTER_NEAREST,
                     Nothing(), aFlags);

  return result == DrawResult::SUCCESS ? dt->Snapshot() : nullptr;
}

NS_IMETHODIMP_(bool)
DynamicImage::IsOpaque()
{
  
  
  return false;
}

NS_IMETHODIMP_(already_AddRefed<ImageContainer>)
DynamicImage::GetImageContainer(LayerManager* aManager, uint32_t aFlags)
{
  return nullptr;
}

NS_IMETHODIMP_(DrawResult)
DynamicImage::Draw(gfxContext* aContext,
                   const nsIntSize& aSize,
                   const ImageRegion& aRegion,
                   uint32_t aWhichFrame,
                   GraphicsFilter aFilter,
                   const Maybe<SVGImageContext>& aSVGContext,
                   uint32_t aFlags)
{
  MOZ_ASSERT(!aSize.IsEmpty(), "Unexpected empty size");

  gfxIntSize drawableSize(mDrawable->Size());

  if (aSize == drawableSize) {
    gfxUtils::DrawPixelSnapped(aContext, mDrawable, drawableSize, aRegion,
                               SurfaceFormat::B8G8R8A8, aFilter);
    return DrawResult::SUCCESS;
  }

  gfxSize scale(double(aSize.width) / drawableSize.width,
                double(aSize.height) / drawableSize.height);

  ImageRegion region(aRegion);
  region.Scale(1.0 / scale.width, 1.0 / scale.height);

  gfxContextMatrixAutoSaveRestore saveMatrix(aContext);
  aContext->Multiply(gfxMatrix::Scaling(scale.width, scale.height));

  gfxUtils::DrawPixelSnapped(aContext, mDrawable, drawableSize, region,
                             SurfaceFormat::B8G8R8A8, aFilter);
  return DrawResult::SUCCESS;
}

NS_IMETHODIMP
DynamicImage::RequestDecode()
{
  return NS_OK;
}

NS_IMETHODIMP
DynamicImage::StartDecoding()
{
  return NS_OK;
}

NS_IMETHODIMP
DynamicImage::RequestDecodeForSize(const nsIntSize& aSize, uint32_t aFlags)
{
  return NS_OK;
}

NS_IMETHODIMP
DynamicImage::LockImage()
{
  return NS_OK;
}

NS_IMETHODIMP
DynamicImage::UnlockImage()
{
  return NS_OK;
}

NS_IMETHODIMP
DynamicImage::RequestDiscard()
{
  return NS_OK;
}

NS_IMETHODIMP_(void)
DynamicImage::RequestRefresh(const mozilla::TimeStamp& aTime)
{ }

NS_IMETHODIMP
DynamicImage::GetAnimationMode(uint16_t* aAnimationMode)
{
  *aAnimationMode = kNormalAnimMode;
  return NS_OK;
}

NS_IMETHODIMP
DynamicImage::SetAnimationMode(uint16_t aAnimationMode)
{
  return NS_OK;
}

NS_IMETHODIMP
DynamicImage::ResetAnimation()
{
  return NS_OK;
}

NS_IMETHODIMP_(float)
DynamicImage::GetFrameIndex(uint32_t aWhichFrame)
{
  return 0;
}

NS_IMETHODIMP_(int32_t)
DynamicImage::GetFirstFrameDelay()
{
  return 0;
}

NS_IMETHODIMP_(void)
DynamicImage::SetAnimationStartTime(const mozilla::TimeStamp& aTime)
{ }

nsIntSize
DynamicImage::OptimalImageSizeForDest(const gfxSize& aDest,
                                      uint32_t aWhichFrame,
                                      GraphicsFilter aFilter, uint32_t aFlags)
{
  gfxIntSize size(mDrawable->Size());
  return nsIntSize(size.width, size.height);
}

NS_IMETHODIMP_(nsIntRect)
DynamicImage::GetImageSpaceInvalidationRect(const nsIntRect& aRect)
{
  return aRect;
}

already_AddRefed<imgIContainer>
DynamicImage::Unwrap()
{
  nsCOMPtr<imgIContainer> self(this);
  return self.forget();
}

} 
} 
