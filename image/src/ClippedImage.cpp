




#include <new>      
#include <cmath>
#include <utility>

#include "gfxDrawable.h"
#include "gfxPlatform.h"
#include "gfxUtils.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/RefPtr.h"

#include "ImageRegion.h"
#include "Orientation.h"
#include "SVGImageContext.h"

#include "ClippedImage.h"

namespace mozilla {

using namespace gfx;
using layers::LayerManager;
using layers::ImageContainer;
using std::make_pair;
using std::modf;
using std::pair;

namespace image {

class ClippedImageCachedSurface
{
public:
  ClippedImageCachedSurface(TemporaryRef<SourceSurface> aSurface,
                            const nsIntSize& aSize,
                            const Maybe<SVGImageContext>& aSVGContext,
                            float aFrame,
                            uint32_t aFlags)
    : mSurface(aSurface)
    , mSize(aSize)
    , mFrame(aFrame)
    , mFlags(aFlags)
  {
    MOZ_ASSERT(mSurface, "Must have a valid surface");
    if (aSVGContext) {
      mSVGContext.emplace(*aSVGContext);
    }
  }

  bool Matches(const nsIntSize& aSize,
               const Maybe<SVGImageContext>& aSVGContext,
               float aFrame,
               uint32_t aFlags)
  {
    return mSize == aSize &&
           mSVGContext == aSVGContext &&
           mFrame == aFrame &&
           mFlags == aFlags;
  }

  TemporaryRef<SourceSurface> Surface() {
    return mSurface;
  }

private:
  RefPtr<SourceSurface>  mSurface;
  const nsIntSize        mSize;
  Maybe<SVGImageContext> mSVGContext;
  const float            mFrame;
  const uint32_t         mFlags;
};

class DrawSingleTileCallback : public gfxDrawingCallback
{
public:
  DrawSingleTileCallback(ClippedImage* aImage,
                         const nsIntSize& aSize,
                         const Maybe<SVGImageContext>& aSVGContext,
                         uint32_t aWhichFrame,
                         uint32_t aFlags)
    : mImage(aImage)
    , mSize(aSize)
    , mSVGContext(aSVGContext)
    , mWhichFrame(aWhichFrame)
    , mFlags(aFlags)
  {
    MOZ_ASSERT(mImage, "Must have an image to clip");
  }

  virtual bool operator()(gfxContext* aContext,
                          const gfxRect& aFillRect,
                          const GraphicsFilter& aFilter,
                          const gfxMatrix& aTransform)
  {
    MOZ_ASSERT(aTransform.IsIdentity(),
               "Caller is probably CreateSamplingRestrictedDrawable, "
               "which should not happen");

    
    
    mImage->DrawSingleTile(aContext, mSize, ImageRegion::Create(aFillRect),
                           mWhichFrame, aFilter, mSVGContext, mFlags);

    return true;
  }

private:
  nsRefPtr<ClippedImage>        mImage;
  const nsIntSize               mSize;
  const Maybe<SVGImageContext>& mSVGContext;
  const uint32_t                mWhichFrame;
  const uint32_t                mFlags;
};

ClippedImage::ClippedImage(Image* aImage,
                           nsIntRect aClip)
  : ImageWrapper(aImage)
  , mClip(aClip)
{
  MOZ_ASSERT(aImage != nullptr, "ClippedImage requires an existing Image");
}

ClippedImage::~ClippedImage()
{ }

bool
ClippedImage::ShouldClip()
{
  
  
  
  
  if (mShouldClip.isNothing()) {
    int32_t width, height;
    nsRefPtr<ProgressTracker> progressTracker =
      InnerImage()->GetProgressTracker();
    if (InnerImage()->HasError()) {
      
      mShouldClip.emplace(false);
    } else if (NS_SUCCEEDED(InnerImage()->GetWidth(&width)) && width > 0 &&
               NS_SUCCEEDED(InnerImage()->GetHeight(&height)) && height > 0) {
      
      mClip = mClip.Intersect(nsIntRect(0, 0, width, height));

      
      
      mShouldClip.emplace(!mClip.IsEqualInterior(nsIntRect(0, 0, width, height)));
    } else if (progressTracker &&
               !(progressTracker->GetProgress() & FLAG_LOAD_COMPLETE)) {
      
      
      
      return false;
    } else {
      
      
      mShouldClip.emplace(false);
    }
  }

  MOZ_ASSERT(mShouldClip.isSome(), "Should have computed a result");
  return *mShouldClip;
}

NS_IMPL_ISUPPORTS_INHERITED0(ClippedImage, ImageWrapper)

NS_IMETHODIMP
ClippedImage::GetWidth(int32_t* aWidth)
{
  if (!ShouldClip()) {
    return InnerImage()->GetWidth(aWidth);
  }

  *aWidth = mClip.width;
  return NS_OK;
}

NS_IMETHODIMP
ClippedImage::GetHeight(int32_t* aHeight)
{
  if (!ShouldClip()) {
    return InnerImage()->GetHeight(aHeight);
  }

  *aHeight = mClip.height;
  return NS_OK;
}

NS_IMETHODIMP
ClippedImage::GetIntrinsicSize(nsSize* aSize)
{
  if (!ShouldClip()) {
    return InnerImage()->GetIntrinsicSize(aSize);
  }

  *aSize = nsSize(mClip.width, mClip.height);
  return NS_OK;
}

NS_IMETHODIMP
ClippedImage::GetIntrinsicRatio(nsSize* aRatio)
{
  if (!ShouldClip()) {
    return InnerImage()->GetIntrinsicRatio(aRatio);
  }

  *aRatio = nsSize(mClip.width, mClip.height);
  return NS_OK;
}

NS_IMETHODIMP_(TemporaryRef<SourceSurface>)
ClippedImage::GetFrame(uint32_t aWhichFrame,
                       uint32_t aFlags)
{
  return GetFrameInternal(mClip.Size(), Nothing(), aWhichFrame, aFlags);
}

TemporaryRef<SourceSurface>
ClippedImage::GetFrameInternal(const nsIntSize& aSize,
                               const Maybe<SVGImageContext>& aSVGContext,
                               uint32_t aWhichFrame,
                               uint32_t aFlags)
{
  if (!ShouldClip()) {
    return InnerImage()->GetFrame(aWhichFrame, aFlags);
  }

  float frameToDraw = InnerImage()->GetFrameIndex(aWhichFrame);
  if (!mCachedSurface || !mCachedSurface->Matches(aSize,
                                                  aSVGContext,
                                                  frameToDraw,
                                                  aFlags)) {
    
    RefPtr<DrawTarget> target = gfxPlatform::GetPlatform()->
      CreateOffscreenContentDrawTarget(IntSize(aSize.width, aSize.height),
                                       SurfaceFormat::B8G8R8A8);
    if (!target) {
      NS_ERROR("Could not create a DrawTarget");
      return nullptr;
    }

    nsRefPtr<gfxContext> ctx = new gfxContext(target);

    
    nsRefPtr<gfxDrawingCallback> drawTileCallback =
      new DrawSingleTileCallback(this, aSize, aSVGContext, aWhichFrame, aFlags);
    nsRefPtr<gfxDrawable> drawable =
      new gfxCallbackDrawable(drawTileCallback, aSize);

    
    gfxUtils::DrawPixelSnapped(ctx, drawable, aSize,
                               ImageRegion::Create(aSize),
                               SurfaceFormat::B8G8R8A8,
                               GraphicsFilter::FILTER_FAST,
                               imgIContainer::FLAG_CLAMP);

    
    mCachedSurface = new ClippedImageCachedSurface(target->Snapshot(), aSize,
                                                   aSVGContext, frameToDraw,
                                                   aFlags);
  }

  MOZ_ASSERT(mCachedSurface, "Should have a cached surface now");
  return mCachedSurface->Surface();
}

NS_IMETHODIMP
ClippedImage::GetImageContainer(LayerManager* aManager, ImageContainer** _retval)
{
  
  
  
  
  

  if (!ShouldClip()) {
    return InnerImage()->GetImageContainer(aManager, _retval);
  }

  *_retval = nullptr;
  return NS_OK;
}

static bool
MustCreateSurface(gfxContext* aContext,
                  const nsIntSize& aSize,
                  const ImageRegion& aRegion,
                  const uint32_t aFlags)
{
  gfxRect imageRect(0, 0, aSize.width, aSize.height);
  bool willTile = !imageRect.Contains(aRegion.Rect()) &&
                  !(aFlags & imgIContainer::FLAG_CLAMP);
  bool willResample = aContext->CurrentMatrix().HasNonIntegerTranslation() &&
                      (willTile || !aRegion.RestrictionContains(imageRect));
  return willTile || willResample;
}

NS_IMETHODIMP
ClippedImage::Draw(gfxContext* aContext,
                   const nsIntSize& aSize,
                   const ImageRegion& aRegion,
                   uint32_t aWhichFrame,
                   GraphicsFilter aFilter,
                   const Maybe<SVGImageContext>& aSVGContext,
                   uint32_t aFlags)
{
  if (!ShouldClip()) {
    return InnerImage()->Draw(aContext, aSize, aRegion, aWhichFrame,
                              aFilter, aSVGContext, aFlags);
  }

  
  
  if (MustCreateSurface(aContext, aSize, aRegion, aFlags)) {
    
    
    RefPtr<SourceSurface> surface =
      GetFrameInternal(aSize, aSVGContext, aWhichFrame, aFlags);
    NS_ENSURE_TRUE(surface, NS_ERROR_FAILURE);

    
    nsRefPtr<gfxSurfaceDrawable> drawable =
      new gfxSurfaceDrawable(surface, aSize);

    
    gfxUtils::DrawPixelSnapped(aContext, drawable, aSize, aRegion,
                               SurfaceFormat::B8G8R8A8, aFilter);

    return NS_OK;
  }

  return DrawSingleTile(aContext, aSize, aRegion, aWhichFrame,
                        aFilter, aSVGContext, aFlags);
}

static SVGImageContext
UnclipViewport(const SVGImageContext& aOldContext,
               const pair<nsIntSize, nsIntSize>& aInnerAndClipSize)
{
  nsIntSize innerSize(aInnerAndClipSize.first);
  nsIntSize clipSize(aInnerAndClipSize.second);

  
  
  nsIntSize vSize(aOldContext.GetViewportSize());
  vSize.width = ceil(vSize.width * double(innerSize.width) / clipSize.width);
  vSize.height = ceil(vSize.height * double(innerSize.height) / clipSize.height);

  return SVGImageContext(vSize,
                         aOldContext.GetPreserveAspectRatio());
}

nsresult
ClippedImage::DrawSingleTile(gfxContext* aContext,
                             const nsIntSize& aSize,
                             const ImageRegion& aRegion,
                             uint32_t aWhichFrame,
                             GraphicsFilter aFilter,
                             const Maybe<SVGImageContext>& aSVGContext,
                             uint32_t aFlags)
{
  MOZ_ASSERT(!MustCreateSurface(aContext, aSize, aRegion, aFlags),
             "Shouldn't need to create a surface");

  gfxRect clip(mClip.x, mClip.y, mClip.width, mClip.height);
  nsIntSize size(aSize), innerSize(aSize);
  if (NS_SUCCEEDED(InnerImage()->GetWidth(&innerSize.width)) &&
      NS_SUCCEEDED(InnerImage()->GetHeight(&innerSize.height))) {
    double scaleX = aSize.width / clip.width;
    double scaleY = aSize.height / clip.height;
    
    
    clip.Scale(scaleX, scaleY);
    size = innerSize;
    size.Scale(scaleX, scaleY);
  } else {
    MOZ_ASSERT(false, "If ShouldClip() led us to draw then we should never get here");
  }

  
  
  ImageRegion region(aRegion);
  region.MoveBy(clip.x, clip.y);
  region = region.Intersect(clip);

  gfxContextMatrixAutoSaveRestore saveMatrix(aContext);
  aContext->Multiply(gfxMatrix::Translation(-clip.x, -clip.y));

  return InnerImage()->Draw(aContext, size, region,
                            aWhichFrame, aFilter,
                            aSVGContext.map(UnclipViewport,
                                            make_pair(innerSize, mClip.Size())),
                            aFlags);
}

NS_IMETHODIMP
ClippedImage::RequestDiscard()
{
  
  mCachedSurface = nullptr;

  return InnerImage()->RequestDiscard();
}

NS_IMETHODIMP_(Orientation)
ClippedImage::GetOrientation()
{
  
  
  return InnerImage()->GetOrientation();
}

nsIntSize
ClippedImage::OptimalImageSizeForDest(const gfxSize& aDest, uint32_t aWhichFrame,
                                      GraphicsFilter aFilter, uint32_t aFlags)
{
  if (!ShouldClip()) {
    return InnerImage()->OptimalImageSizeForDest(aDest, aWhichFrame, aFilter, aFlags);
  }

  int32_t imgWidth, imgHeight;
  if (NS_SUCCEEDED(InnerImage()->GetWidth(&imgWidth)) &&
      NS_SUCCEEDED(InnerImage()->GetHeight(&imgHeight))) {
    
    

    
    
    nsIntSize scale(ceil(aDest.width / mClip.width),
                    ceil(aDest.height / mClip.height));

    
    
    gfxSize desiredSize(imgWidth * scale.width, imgHeight * scale.height);
    nsIntSize innerDesiredSize =
      InnerImage()->OptimalImageSizeForDest(desiredSize, aWhichFrame,
                                            aFilter, aFlags);

    
    
    
    nsIntSize finalScale(ceil(double(innerDesiredSize.width) / imgWidth),
                         ceil(double(innerDesiredSize.height) / imgHeight));
    return mClip.Size() * finalScale;
  } else {
    MOZ_ASSERT(false, "If ShouldClip() led us to draw then we should never get here");
    return InnerImage()->OptimalImageSizeForDest(aDest, aWhichFrame, aFilter, aFlags);
  }
}

NS_IMETHODIMP_(nsIntRect)
ClippedImage::GetImageSpaceInvalidationRect(const nsIntRect& aRect)
{
  if (!ShouldClip()) {
    return InnerImage()->GetImageSpaceInvalidationRect(aRect);
  }

  nsIntRect rect(InnerImage()->GetImageSpaceInvalidationRect(aRect));
  rect = rect.Intersect(mClip);
  rect.MoveBy(-mClip.x, -mClip.y);
  return rect;
}

} 
} 
