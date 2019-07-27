




#include <cmath>

#include "gfxDrawable.h"
#include "gfxPlatform.h"
#include "gfxUtils.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/RefPtr.h"

#include "ClippedImage.h"
#include "Orientation.h"
#include "SVGImageContext.h"


namespace mozilla {

using namespace gfx;
using layers::LayerManager;
using layers::ImageContainer;
using std::modf;

namespace image {

class ClippedImageCachedSurface
{
public:
  ClippedImageCachedSurface(TemporaryRef<SourceSurface> aSurface,
                            const nsIntSize& aViewportSize,
                            const SVGImageContext* aSVGContext,
                            float aFrame,
                            uint32_t aFlags)
    : mSurface(aSurface)
    , mViewportSize(aViewportSize)
    , mFrame(aFrame)
    , mFlags(aFlags)
  {
    MOZ_ASSERT(mSurface, "Must have a valid surface");
    if (aSVGContext) {
      mSVGContext.emplace(*aSVGContext);
    }
  }

  bool Matches(const nsIntSize& aViewportSize,
               const SVGImageContext* aSVGContext,
               float aFrame,
               uint32_t aFlags)
  {
    bool matchesSVGContext = (!aSVGContext && !mSVGContext) ||
                             (*aSVGContext == *mSVGContext);
    return mViewportSize == aViewportSize &&
           matchesSVGContext &&
           mFrame == aFrame &&
           mFlags == aFlags;
  }

  TemporaryRef<SourceSurface> Surface() {
    return mSurface;
  }

private:
  RefPtr<SourceSurface>              mSurface;
  const nsIntSize                    mViewportSize;
  Maybe<SVGImageContext>             mSVGContext;
  const float                        mFrame;
  const uint32_t                     mFlags;
};

class DrawSingleTileCallback : public gfxDrawingCallback
{
public:
  DrawSingleTileCallback(ClippedImage* aImage,
                         const nsIntRect& aClip,
                         const nsIntSize& aViewportSize,
                         const SVGImageContext* aSVGContext,
                         uint32_t aWhichFrame,
                         uint32_t aFlags)
    : mImage(aImage)
    , mClip(aClip)
    , mViewportSize(aViewportSize)
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
    
    
    mImage->DrawSingleTile(aContext, aFilter, aTransform, aFillRect, mClip,
                           mViewportSize, mSVGContext, mWhichFrame, mFlags);

    return true;
  }

private:
  nsRefPtr<ClippedImage> mImage;
  const nsIntRect        mClip;
  const nsIntSize        mViewportSize;
  const SVGImageContext* mSVGContext;
  const uint32_t         mWhichFrame;
  const uint32_t         mFlags;
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
    nsRefPtr<imgStatusTracker> innerImageStatusTracker =
      InnerImage()->GetStatusTracker();
    if (InnerImage()->HasError()) {
      
      mShouldClip.emplace(false);
    } else if (NS_SUCCEEDED(InnerImage()->GetWidth(&width)) && width > 0 &&
               NS_SUCCEEDED(InnerImage()->GetHeight(&height)) && height > 0) {
      
      mClip = mClip.Intersect(nsIntRect(0, 0, width, height));

      
      
      mShouldClip.emplace(!mClip.IsEqualInterior(nsIntRect(0, 0, width, height)));
    } else if (innerImageStatusTracker &&
               innerImageStatusTracker->IsLoading()) {
      
      
      
      return false;
    } else {
      
      
      mShouldClip.emplace(false);
    }
  }

  MOZ_ASSERT(mShouldClip.isSome(), "Should have computed a result");
  return *mShouldClip;
}

NS_IMPL_ISUPPORTS_INHERITED0(ClippedImage, ImageWrapper)

nsIntRect
ClippedImage::FrameRect(uint32_t aWhichFrame)
{
  if (!ShouldClip()) {
    return InnerImage()->FrameRect(aWhichFrame);
  }

  return nsIntRect(0, 0, mClip.width, mClip.height);
}

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
  return GetFrameInternal(mClip.Size(), nullptr, aWhichFrame, aFlags);
}

TemporaryRef<SourceSurface>
ClippedImage::GetFrameInternal(const nsIntSize& aViewportSize,
                               const SVGImageContext* aSVGContext,
                               uint32_t aWhichFrame,
                               uint32_t aFlags)
{
  if (!ShouldClip()) {
    return InnerImage()->GetFrame(aWhichFrame, aFlags);
  }

  float frameToDraw = InnerImage()->GetFrameIndex(aWhichFrame);
  if (!mCachedSurface || !mCachedSurface->Matches(aViewportSize,
                                                  aSVGContext,
                                                  frameToDraw,
                                                  aFlags)) {
    
    RefPtr<DrawTarget> target = gfxPlatform::GetPlatform()->
      CreateOffscreenContentDrawTarget(IntSize(mClip.width, mClip.height),
                                       SurfaceFormat::B8G8R8A8);
    if (!target) {
      NS_ERROR("Could not create a DrawTarget");
      return nullptr;
    }

    nsRefPtr<gfxContext> ctx = new gfxContext(target);

    
    nsRefPtr<gfxDrawingCallback> drawTileCallback =
      new DrawSingleTileCallback(this, mClip, aViewportSize, aSVGContext, aWhichFrame, aFlags);
    nsRefPtr<gfxDrawable> drawable =
      new gfxCallbackDrawable(drawTileCallback, mClip.Size());

    
    gfxRect imageRect(0, 0, mClip.width, mClip.height);
    gfxUtils::DrawPixelSnapped(ctx, drawable, gfxMatrix(),
                               imageRect, imageRect, imageRect, imageRect,
                               SurfaceFormat::B8G8R8A8,
                               GraphicsFilter::FILTER_FAST);

    
    mCachedSurface = new ClippedImageCachedSurface(target->Snapshot(),
                                                   aViewportSize,
                                                   aSVGContext,
                                                   frameToDraw,
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

bool
ClippedImage::MustCreateSurface(gfxContext* aContext,
                                const gfxMatrix& aTransform,
                                const gfxRect& aSourceRect,
                                const nsIntRect& aSubimage,
                                const uint32_t aFlags) const
{
  gfxRect gfxImageRect(0, 0, mClip.width, mClip.height);
  nsIntRect intImageRect(0, 0, mClip.width, mClip.height);
  bool willTile = !gfxImageRect.Contains(aSourceRect) &&
                  !(aFlags & imgIContainer::FLAG_CLAMP);
  bool willResample = (aContext->CurrentMatrix().HasNonIntegerTranslation() ||
                       aTransform.HasNonIntegerTranslation()) &&
                      (willTile || !aSubimage.Contains(intImageRect));
  return willTile || willResample;
}

NS_IMETHODIMP
ClippedImage::Draw(gfxContext* aContext,
                   GraphicsFilter aFilter,
                   const gfxMatrix& aUserSpaceToImageSpace,
                   const gfxRect& aFill,
                   const nsIntRect& aSubimage,
                   const nsIntSize& aViewportSize,
                   const SVGImageContext* aSVGContext,
                   uint32_t aWhichFrame,
                   uint32_t aFlags)
{
  if (!ShouldClip()) {
    return InnerImage()->Draw(aContext, aFilter, aUserSpaceToImageSpace,
                              aFill, aSubimage, aViewportSize, aSVGContext,
                              aWhichFrame, aFlags);
  }

  
  
  gfxRect sourceRect = aUserSpaceToImageSpace.Transform(aFill);
  if (MustCreateSurface(aContext, aUserSpaceToImageSpace, sourceRect, aSubimage, aFlags)) {
    
    
    RefPtr<SourceSurface> surface =
      GetFrameInternal(aViewportSize, aSVGContext, aWhichFrame, aFlags);
    NS_ENSURE_TRUE(surface, NS_ERROR_FAILURE);

    
    nsRefPtr<gfxSurfaceDrawable> drawable =
      new gfxSurfaceDrawable(surface, gfxIntSize(mClip.width, mClip.height));

    
    gfxRect imageRect(0, 0, mClip.width, mClip.height);
    gfxRect subimage(aSubimage.x, aSubimage.y, aSubimage.width, aSubimage.height);
    gfxUtils::DrawPixelSnapped(aContext, drawable, aUserSpaceToImageSpace,
                               subimage, sourceRect, imageRect, aFill,
                               SurfaceFormat::B8G8R8A8, aFilter);

    return NS_OK;
  }

  
  nsIntRect innerSubimage(aSubimage);
  innerSubimage.MoveBy(mClip.x, mClip.y);
  innerSubimage.Intersect(mClip);

  return DrawSingleTile(aContext, aFilter, aUserSpaceToImageSpace, aFill, innerSubimage,
                        aViewportSize, aSVGContext, aWhichFrame, aFlags);
}

gfxFloat
ClippedImage::ClampFactor(const gfxFloat aToClamp, const int aReference) const
{
  return aToClamp > aReference ? aReference / aToClamp
                               : 1.0;
}

nsresult
ClippedImage::DrawSingleTile(gfxContext* aContext,
                             GraphicsFilter aFilter,
                             const gfxMatrix& aUserSpaceToImageSpace,
                             const gfxRect& aFill,
                             const nsIntRect& aSubimage,
                             const nsIntSize& aViewportSize,
                             const SVGImageContext* aSVGContext,
                             uint32_t aWhichFrame,
                             uint32_t aFlags)
{
  MOZ_ASSERT(!MustCreateSurface(aContext, aUserSpaceToImageSpace,
                                aUserSpaceToImageSpace.Transform(aFill),
                                aSubimage - nsIntPoint(mClip.x, mClip.y), aFlags),
             "DrawSingleTile shouldn't need to create a surface");

  
  nsIntSize viewportSize(aViewportSize);
  int32_t imgWidth, imgHeight;
  if (NS_SUCCEEDED(InnerImage()->GetWidth(&imgWidth)) &&
      NS_SUCCEEDED(InnerImage()->GetHeight(&imgHeight))) {
    viewportSize = nsIntSize(imgWidth, imgHeight);
  } else {
    MOZ_ASSERT(false, "If ShouldClip() led us to draw then we should never get here");
  }

  
  gfxMatrix transform =
    aUserSpaceToImageSpace * gfxMatrix::Translation(mClip.x, mClip.y);

  
  
  gfxRect sourceRect = transform.Transform(aFill);
  if (sourceRect.width > mClip.width || sourceRect.height > mClip.height) {
    gfxMatrix clampSource = gfxMatrix::Translation(sourceRect.TopLeft());
    clampSource.Scale(ClampFactor(sourceRect.width, mClip.width),
                      ClampFactor(sourceRect.height, mClip.height));
    clampSource.Translate(-sourceRect.TopLeft());
    transform *= clampSource;
  }

  return InnerImage()->Draw(aContext, aFilter, transform, aFill, aSubimage,
                            viewportSize, aSVGContext, aWhichFrame, aFlags);
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
