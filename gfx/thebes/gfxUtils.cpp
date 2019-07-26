




#include "gfxUtils.h"
#include "gfxContext.h"
#include "gfxPlatform.h"
#include "gfxDrawable.h"
#include "nsRegion.h"
#include "yuv_convert.h"
#include "ycbcr_to_rgb565.h"
#include "GeckoProfiler.h"

#ifdef XP_WIN
#include "gfxWindowsPlatform.h"
#endif

using namespace mozilla;
using namespace mozilla::layers;
using namespace mozilla::gfx;

#include "PremultiplyTables.h"

static const uint8_t PremultiplyValue(uint8_t a, uint8_t v) {
    return gfxUtils::sPremultiplyTable[a*256+v];
}

static const uint8_t UnpremultiplyValue(uint8_t a, uint8_t v) {
    return gfxUtils::sUnpremultiplyTable[a*256+v];
}

void
gfxUtils::PremultiplyImageSurface(gfxImageSurface *aSourceSurface,
                                  gfxImageSurface *aDestSurface)
{
    if (!aDestSurface)
        aDestSurface = aSourceSurface;

    MOZ_ASSERT(aSourceSurface->Format() == aDestSurface->Format() &&
               aSourceSurface->Width()  == aDestSurface->Width() &&
               aSourceSurface->Height() == aDestSurface->Height() &&
               aSourceSurface->Stride() == aDestSurface->Stride(),
               "Source and destination surfaces don't have identical characteristics");

    MOZ_ASSERT(aSourceSurface->Stride() == aSourceSurface->Width() * 4,
               "Source surface stride isn't tightly packed");

    
    if (aSourceSurface->Format() != gfxASurface::ImageFormatARGB32) {
        if (aDestSurface != aSourceSurface) {
            memcpy(aDestSurface->Data(), aSourceSurface->Data(),
                   aSourceSurface->Stride() * aSourceSurface->Height());
        }
        return;
    }

    uint8_t *src = aSourceSurface->Data();
    uint8_t *dst = aDestSurface->Data();

    uint32_t dim = aSourceSurface->Width() * aSourceSurface->Height();
    for (uint32_t i = 0; i < dim; ++i) {
#ifdef IS_LITTLE_ENDIAN
        uint8_t b = *src++;
        uint8_t g = *src++;
        uint8_t r = *src++;
        uint8_t a = *src++;

        *dst++ = PremultiplyValue(a, b);
        *dst++ = PremultiplyValue(a, g);
        *dst++ = PremultiplyValue(a, r);
        *dst++ = a;
#else
        uint8_t a = *src++;
        uint8_t r = *src++;
        uint8_t g = *src++;
        uint8_t b = *src++;

        *dst++ = a;
        *dst++ = PremultiplyValue(a, r);
        *dst++ = PremultiplyValue(a, g);
        *dst++ = PremultiplyValue(a, b);
#endif
    }
}

void
gfxUtils::UnpremultiplyImageSurface(gfxImageSurface *aSourceSurface,
                                    gfxImageSurface *aDestSurface)
{
    if (!aDestSurface)
        aDestSurface = aSourceSurface;

    MOZ_ASSERT(aSourceSurface->Format() == aDestSurface->Format() &&
               aSourceSurface->Width()  == aDestSurface->Width() &&
               aSourceSurface->Height() == aDestSurface->Height() &&
               aSourceSurface->Stride() == aDestSurface->Stride(),
               "Source and destination surfaces don't have identical characteristics");

    MOZ_ASSERT(aSourceSurface->Stride() == aSourceSurface->Width() * 4,
               "Source surface stride isn't tightly packed");

    
    if (aSourceSurface->Format() != gfxASurface::ImageFormatARGB32) {
        if (aDestSurface != aSourceSurface) {
            memcpy(aDestSurface->Data(), aSourceSurface->Data(),
                   aSourceSurface->Stride() * aSourceSurface->Height());
        }
        return;
    }

    uint8_t *src = aSourceSurface->Data();
    uint8_t *dst = aDestSurface->Data();

    uint32_t dim = aSourceSurface->Width() * aSourceSurface->Height();
    for (uint32_t i = 0; i < dim; ++i) {
#ifdef IS_LITTLE_ENDIAN
        uint8_t b = *src++;
        uint8_t g = *src++;
        uint8_t r = *src++;
        uint8_t a = *src++;

        *dst++ = UnpremultiplyValue(a, b);
        *dst++ = UnpremultiplyValue(a, g);
        *dst++ = UnpremultiplyValue(a, r);
        *dst++ = a;
#else
        uint8_t a = *src++;
        uint8_t r = *src++;
        uint8_t g = *src++;
        uint8_t b = *src++;

        *dst++ = a;
        *dst++ = UnpremultiplyValue(a, r);
        *dst++ = UnpremultiplyValue(a, g);
        *dst++ = UnpremultiplyValue(a, b);
#endif
    }
}

void
gfxUtils::ConvertBGRAtoRGBA(gfxImageSurface *aSourceSurface,
                            gfxImageSurface *aDestSurface) {
    if (!aDestSurface)
        aDestSurface = aSourceSurface;

    MOZ_ASSERT(aSourceSurface->Format() == aDestSurface->Format() &&
               aSourceSurface->Width()  == aDestSurface->Width() &&
               aSourceSurface->Height() == aDestSurface->Height() &&
               aSourceSurface->Stride() == aDestSurface->Stride(),
               "Source and destination surfaces don't have identical characteristics");

    MOZ_ASSERT(aSourceSurface->Stride() == aSourceSurface->Width() * 4,
               "Source surface stride isn't tightly packed");

    MOZ_ASSERT(aSourceSurface->Format() == gfxASurface::ImageFormatARGB32,
               "Surfaces must be ARGB32");

    uint8_t *src = aSourceSurface->Data();
    uint8_t *dst = aDestSurface->Data();

    uint32_t dim = aSourceSurface->Width() * aSourceSurface->Height();
    uint8_t *srcEnd = src + 4*dim;

    if (src == dst) {
        uint8_t buffer[4];
        for (; src != srcEnd; src += 4) {
            buffer[0] = src[2];
            buffer[1] = src[1];
            buffer[2] = src[0];

            src[0] = buffer[0];
            src[1] = buffer[1];
            src[2] = buffer[2];
        }
    } else {
        for (; src != srcEnd; src += 4, dst += 4) {
            dst[0] = src[2];
            dst[1] = src[1];
            dst[2] = src[0];
            dst[3] = src[3];
        }
    }
}

static bool
IsSafeImageTransformComponent(gfxFloat aValue)
{
  return aValue >= -32768 && aValue <= 32767;
}






static gfxContext::GraphicsOperator
OptimalFillOperator()
{
#ifdef XP_WIN
    if (gfxWindowsPlatform::GetPlatform()->GetRenderMode() ==
        gfxWindowsPlatform::RENDER_DIRECT2D) {
        
        return gfxContext::OPERATOR_OVER;
    } else {
#endif
        return gfxContext::OPERATOR_SOURCE;
#ifdef XP_WIN
    }
#endif
}

#ifndef MOZ_GFX_OPTIMIZE_MOBILE


static already_AddRefed<gfxDrawable>
CreateSamplingRestrictedDrawable(gfxDrawable* aDrawable,
                                 gfxContext* aContext,
                                 const gfxMatrix& aUserSpaceToImageSpace,
                                 const gfxRect& aSourceRect,
                                 const gfxRect& aSubimage,
                                 const gfxImageSurface::gfxImageFormat aFormat)
{
    PROFILER_LABEL("gfxUtils", "CreateSamplingRestricedDrawable");
    gfxRect userSpaceClipExtents = aContext->GetClipExtents();
    
    
    
    
    
    gfxRect imageSpaceClipExtents =
        aUserSpaceToImageSpace.TransformBounds(userSpaceClipExtents);
    
    
    imageSpaceClipExtents.Inflate(1.0);

    gfxRect needed = imageSpaceClipExtents.Intersect(aSourceRect);
    needed = needed.Intersect(aSubimage);
    needed.RoundOut();

    
    
    
    
    if (needed.IsEmpty())
        return nullptr;

    gfxIntSize size(int32_t(needed.Width()), int32_t(needed.Height()));
    nsRefPtr<gfxASurface> temp =
        gfxPlatform::GetPlatform()->CreateOffscreenSurface(size, gfxASurface::ContentFromFormat(aFormat));
    if (!temp || temp->CairoStatus())
        return nullptr;

    nsRefPtr<gfxContext> tmpCtx = new gfxContext(temp);
    tmpCtx->SetOperator(OptimalFillOperator());
    aDrawable->Draw(tmpCtx, needed - needed.TopLeft(), true,
                    gfxPattern::FILTER_FAST, gfxMatrix().Translate(needed.TopLeft()));

    nsRefPtr<gfxDrawable> drawable = 
        new gfxSurfaceDrawable(temp, size, gfxMatrix().Translate(-needed.TopLeft()));
    return drawable.forget();
}
#endif 



struct NS_STACK_CLASS AutoCairoPixmanBugWorkaround
{
    AutoCairoPixmanBugWorkaround(gfxContext*      aContext,
                                 const gfxMatrix& aDeviceSpaceToImageSpace,
                                 const gfxRect&   aFill,
                                 const gfxASurface* aSurface)
     : mContext(aContext), mSucceeded(true), mPushedGroup(false)
    {
        
        if (!aSurface || aSurface->GetType() == gfxASurface::SurfaceTypeQuartz)
            return;

        if (!IsSafeImageTransformComponent(aDeviceSpaceToImageSpace.xx) ||
            !IsSafeImageTransformComponent(aDeviceSpaceToImageSpace.xy) ||
            !IsSafeImageTransformComponent(aDeviceSpaceToImageSpace.yx) ||
            !IsSafeImageTransformComponent(aDeviceSpaceToImageSpace.yy)) {
            NS_WARNING("Scaling up too much, bailing out");
            mSucceeded = false;
            return;
        }

        if (IsSafeImageTransformComponent(aDeviceSpaceToImageSpace.x0) &&
            IsSafeImageTransformComponent(aDeviceSpaceToImageSpace.y0))
            return;

        
        
        gfxMatrix currentMatrix = mContext->CurrentMatrix();
        mContext->Save();

        
        
        
        mContext->IdentityMatrix();
        gfxRect bounds = currentMatrix.TransformBounds(aFill);
        bounds.RoundOut();
        mContext->Clip(bounds);
        mContext->SetMatrix(currentMatrix);
        mContext->PushGroup(gfxASurface::CONTENT_COLOR_ALPHA);
        mContext->SetOperator(gfxContext::OPERATOR_OVER);

        mPushedGroup = true;
    }

    ~AutoCairoPixmanBugWorkaround()
    {
        if (mPushedGroup) {
            mContext->PopGroupToSource();
            mContext->Paint();
            mContext->Restore();
        }
    }

    bool PushedGroup() { return mPushedGroup; }
    bool Succeeded() { return mSucceeded; }

private:
    gfxContext* mContext;
    bool mSucceeded;
    bool mPushedGroup;
};

static gfxMatrix
DeviceToImageTransform(gfxContext* aContext,
                       const gfxMatrix& aUserSpaceToImageSpace)
{
    gfxFloat deviceX, deviceY;
    nsRefPtr<gfxASurface> currentTarget =
        aContext->CurrentSurface(&deviceX, &deviceY);
    gfxMatrix currentMatrix = aContext->CurrentMatrix();
    gfxMatrix deviceToUser = gfxMatrix(currentMatrix).Invert();
    deviceToUser.Translate(-gfxPoint(-deviceX, -deviceY));
    return gfxMatrix(deviceToUser).Multiply(aUserSpaceToImageSpace);
}


#ifdef MOZ_GFX_OPTIMIZE_MOBILE
static gfxPattern::GraphicsFilter ReduceResamplingFilter(gfxPattern::GraphicsFilter aFilter,
                                                         int aImgWidth, int aImgHeight,
                                                         float aSourceWidth, float aSourceHeight)
{
    
    
    const int kSmallImageSizeThreshold = 8;

    
    
    
    const float kLargeStretch = 3.0f;

    if (aImgWidth <= kSmallImageSizeThreshold
        || aImgHeight <= kSmallImageSizeThreshold) {
        
        
        return gfxPattern::FILTER_NEAREST;
    }

    if (aImgHeight * kLargeStretch <= aSourceHeight || aImgWidth * kLargeStretch <= aSourceWidth) {
        

        
        
        
        
        if (fabs(aSourceWidth - aImgWidth)/aImgWidth < 0.5 || fabs(aSourceHeight - aImgHeight)/aImgHeight < 0.5)
            return gfxPattern::FILTER_NEAREST;

        
        
        return aFilter;
    }

    

















    return aFilter;
}
#else
static gfxPattern::GraphicsFilter ReduceResamplingFilter(gfxPattern::GraphicsFilter aFilter,
                                                          int aImgWidth, int aImgHeight,
                                                          int aSourceWidth, int aSourceHeight)
{
    
    return aFilter;
}
#endif

 void
gfxUtils::DrawPixelSnapped(gfxContext*      aContext,
                           gfxDrawable*     aDrawable,
                           const gfxMatrix& aUserSpaceToImageSpace,
                           const gfxRect&   aSubimage,
                           const gfxRect&   aSourceRect,
                           const gfxRect&   aImageRect,
                           const gfxRect&   aFill,
                           const gfxImageSurface::gfxImageFormat aFormat,
                           gfxPattern::GraphicsFilter aFilter,
                           uint32_t         aImageFlags)
{
    PROFILER_LABEL("gfxUtils", "DrawPixelSnapped");
    bool doTile = !aImageRect.Contains(aSourceRect) &&
                  !(aImageFlags & imgIContainer::FLAG_CLAMP);

    nsRefPtr<gfxASurface> currentTarget = aContext->CurrentSurface();
    gfxMatrix deviceSpaceToImageSpace =
        DeviceToImageTransform(aContext, aUserSpaceToImageSpace);

    AutoCairoPixmanBugWorkaround workaround(aContext, deviceSpaceToImageSpace,
                                            aFill, currentTarget);
    if (!workaround.Succeeded())
        return;

    nsRefPtr<gfxDrawable> drawable = aDrawable;

    aFilter = ReduceResamplingFilter(aFilter, aImageRect.Width(), aImageRect.Height(), aSourceRect.Width(), aSourceRect.Height());

    gfxMatrix userSpaceToImageSpace = aUserSpaceToImageSpace;

    
    
    
#ifdef MOZ_GFX_OPTIMIZE_MOBILE
    
    
    
    
    
    if (doTile && (userSpaceToImageSpace.y0 > 16384.0 || userSpaceToImageSpace.x0 > 16384.0)) {
        userSpaceToImageSpace.x0 = fmod(userSpaceToImageSpace.x0, aImageRect.width);
        userSpaceToImageSpace.y0 = fmod(userSpaceToImageSpace.y0, aImageRect.height);
    }
#else
    
    
    
    
    
    if (aContext->CurrentMatrix().HasNonIntegerTranslation() ||
        aUserSpaceToImageSpace.HasNonIntegerTranslation()) {
        if (doTile || !aSubimage.Contains(aImageRect)) {
            nsRefPtr<gfxDrawable> restrictedDrawable =
              CreateSamplingRestrictedDrawable(aDrawable, aContext,
                                               aUserSpaceToImageSpace, aSourceRect,
                                               aSubimage, aFormat);
            if (restrictedDrawable) {
                drawable.swap(restrictedDrawable);
            }
        }
        
        
        
        doTile = false;
    }
#endif

    gfxContext::GraphicsOperator op = aContext->CurrentOperator();
    if ((op == gfxContext::OPERATOR_OVER || workaround.PushedGroup()) &&
        aFormat == gfxASurface::ImageFormatRGB24) {
        aContext->SetOperator(OptimalFillOperator());
    }

    drawable->Draw(aContext, aFill, doTile, aFilter, userSpaceToImageSpace);

    aContext->SetOperator(op);
}

 int
gfxUtils::ImageFormatToDepth(gfxASurface::gfxImageFormat aFormat)
{
    switch (aFormat) {
        case gfxASurface::ImageFormatARGB32:
            return 32;
        case gfxASurface::ImageFormatRGB24:
            return 24;
        case gfxASurface::ImageFormatRGB16_565:
            return 16;
        default:
            break;
    }
    return 0;
}

static void
PathFromRegionInternal(gfxContext* aContext, const nsIntRegion& aRegion,
                       bool aSnap)
{
  aContext->NewPath();
  nsIntRegionRectIterator iter(aRegion);
  const nsIntRect* r;
  while ((r = iter.Next()) != nullptr) {
    aContext->Rectangle(gfxRect(r->x, r->y, r->width, r->height), aSnap);
  }
}

static void
ClipToRegionInternal(gfxContext* aContext, const nsIntRegion& aRegion,
                     bool aSnap)
{
  PathFromRegionInternal(aContext, aRegion, aSnap);
  aContext->Clip();
}

 void
gfxUtils::ClipToRegion(gfxContext* aContext, const nsIntRegion& aRegion)
{
  ClipToRegionInternal(aContext, aRegion, false);
}

 void
gfxUtils::ClipToRegionSnapped(gfxContext* aContext, const nsIntRegion& aRegion)
{
  ClipToRegionInternal(aContext, aRegion, true);
}

 gfxFloat
gfxUtils::ClampToScaleFactor(gfxFloat aVal)
{
  
  
  
  static const gfxFloat kScaleResolution = 2;

  
  
  if (aVal < 0.0) {
    aVal = -aVal;
  }

  bool inverse = false;
  if (aVal < 1.0) {
    inverse = true;
    aVal = 1 / aVal;
  }

  gfxFloat power = log(aVal)/log(kScaleResolution);

  
  
  
  if (fabs(power - NS_round(power)) < 1e-6) {
    power = NS_round(power);
  } else if (inverse) {
    power = floor(power);
  } else {
    power = ceil(power);
  }

  gfxFloat scale = pow(kScaleResolution, power);

  if (inverse) {
    scale = 1 / scale;
  }

  return scale;
}


 void
gfxUtils::PathFromRegion(gfxContext* aContext, const nsIntRegion& aRegion)
{
  PathFromRegionInternal(aContext, aRegion, false);
}

 void
gfxUtils::PathFromRegionSnapped(gfxContext* aContext, const nsIntRegion& aRegion)
{
  PathFromRegionInternal(aContext, aRegion, true);
}

gfxMatrix
gfxUtils::TransformRectToRect(const gfxRect& aFrom, const gfxPoint& aToTopLeft,
                              const gfxPoint& aToTopRight, const gfxPoint& aToBottomRight)
{
  gfxMatrix m;
  if (aToTopRight.y == aToTopLeft.y && aToTopRight.x == aToBottomRight.x) {
    
    m.xy = m.yx = 0.0;
    m.xx = (aToBottomRight.x - aToTopLeft.x)/aFrom.width;
    m.yy = (aToBottomRight.y - aToTopLeft.y)/aFrom.height;
    m.x0 = aToTopLeft.x - m.xx*aFrom.x;
    m.y0 = aToTopLeft.y - m.yy*aFrom.y;
  } else {
    NS_ASSERTION(aToTopRight.y == aToBottomRight.y && aToTopRight.x == aToTopLeft.x,
                 "Destination rectangle not axis-aligned");
    m.xx = m.yy = 0.0;
    m.xy = (aToBottomRight.x - aToTopLeft.x)/aFrom.height;
    m.yx = (aToBottomRight.y - aToTopLeft.y)/aFrom.width;
    m.x0 = aToTopLeft.x - m.xy*aFrom.y;
    m.y0 = aToTopLeft.y - m.yx*aFrom.x;
  }
  return m;
}

bool
gfxUtils::GfxRectToIntRect(const gfxRect& aIn, nsIntRect* aOut)
{
  *aOut = nsIntRect(int32_t(aIn.X()), int32_t(aIn.Y()),
  int32_t(aIn.Width()), int32_t(aIn.Height()));
  return gfxRect(aOut->x, aOut->y, aOut->width, aOut->height).IsEqualEdges(aIn);
}

void
gfxUtils::GetYCbCrToRGBDestFormatAndSize(const PlanarYCbCrImage::Data& aData,
                                         gfxASurface::gfxImageFormat& aSuggestedFormat,
                                         gfxIntSize& aSuggestedSize)
{
  gfx::YUVType yuvtype =
    gfx::TypeFromSize(aData.mYSize.width,
                      aData.mYSize.height,
                      aData.mCbCrSize.width,
                      aData.mCbCrSize.height);

  
  
  bool prescale = aSuggestedSize.width > 0 && aSuggestedSize.height > 0 &&
                    aSuggestedSize != aData.mPicSize;

  if (aSuggestedFormat == gfxASurface::ImageFormatRGB16_565) {
#if defined(HAVE_YCBCR_TO_RGB565)
    if (prescale &&
        !gfx::IsScaleYCbCrToRGB565Fast(aData.mPicX,
                                       aData.mPicY,
                                       aData.mPicSize.width,
                                       aData.mPicSize.height,
                                       aSuggestedSize.width,
                                       aSuggestedSize.height,
                                       yuvtype,
                                       gfx::FILTER_BILINEAR) &&
        gfx::IsConvertYCbCrToRGB565Fast(aData.mPicX,
                                        aData.mPicY,
                                        aData.mPicSize.width,
                                        aData.mPicSize.height,
                                        yuvtype)) {
      prescale = false;
    }
#else
    
    aSuggestedFormat = gfxASurface::ImageFormatRGB24;
#endif
  }
  else if (aSuggestedFormat != gfxASurface::ImageFormatRGB24) {
    
    aSuggestedFormat = gfxASurface::ImageFormatRGB24;
  }
  if (aSuggestedFormat == gfxASurface::ImageFormatRGB24) {
    

    if (aData.mPicX != 0 || aData.mPicY != 0 || yuvtype == gfx::YV24)
      prescale = false;
  }
  if (!prescale) {
    aSuggestedSize = aData.mPicSize;
  }
}

void
gfxUtils::ConvertYCbCrToRGB(const PlanarYCbCrImage::Data& aData,
                            const gfxASurface::gfxImageFormat& aDestFormat,
                            const gfxIntSize& aDestSize,
                            unsigned char* aDestBuffer,
                            int32_t aStride)
{
  
  
  MOZ_ASSERT((aData.mCbCrSize.width == aData.mYSize.width ||
              aData.mCbCrSize.width == (aData.mYSize.width + 1) >> 1) &&
             (aData.mCbCrSize.height == aData.mYSize.height ||
              aData.mCbCrSize.height == (aData.mYSize.height + 1) >> 1));
  gfx::YUVType yuvtype =
    gfx::TypeFromSize(aData.mYSize.width,
                      aData.mYSize.height,
                      aData.mCbCrSize.width,
                      aData.mCbCrSize.height);

  
  if (aDestSize != aData.mPicSize) {
#if defined(HAVE_YCBCR_TO_RGB565)
    if (aDestFormat == gfxASurface::ImageFormatRGB16_565) {
      gfx::ScaleYCbCrToRGB565(aData.mYChannel,
                              aData.mCbChannel,
                              aData.mCrChannel,
                              aDestBuffer,
                              aData.mPicX,
                              aData.mPicY,
                              aData.mPicSize.width,
                              aData.mPicSize.height,
                              aDestSize.width,
                              aDestSize.height,
                              aData.mYStride,
                              aData.mCbCrStride,
                              aStride,
                              yuvtype,
                              gfx::FILTER_BILINEAR);
    } else
#endif
      gfx::ScaleYCbCrToRGB32(aData.mYChannel,
                             aData.mCbChannel,
                             aData.mCrChannel,
                             aDestBuffer,
                             aData.mPicSize.width,
                             aData.mPicSize.height,
                             aDestSize.width,
                             aDestSize.height,
                             aData.mYStride,
                             aData.mCbCrStride,
                             aStride,
                             yuvtype,
                             gfx::ROTATE_0,
                             gfx::FILTER_BILINEAR);
  } else { 
#if defined(HAVE_YCBCR_TO_RGB565)
    if (aDestFormat == gfxASurface::ImageFormatRGB16_565) {
      gfx::ConvertYCbCrToRGB565(aData.mYChannel,
                                aData.mCbChannel,
                                aData.mCrChannel,
                                aDestBuffer,
                                aData.mPicX,
                                aData.mPicY,
                                aData.mPicSize.width,
                                aData.mPicSize.height,
                                aData.mYStride,
                                aData.mCbCrStride,
                                aStride,
                                yuvtype);
    } else 
#endif
      gfx::ConvertYCbCrToRGB32(aData.mYChannel,
                               aData.mCbChannel,
                               aData.mCrChannel,
                               aDestBuffer,
                               aData.mPicX,
                               aData.mPicY,
                               aData.mPicSize.width,
                               aData.mPicSize.height,
                               aData.mYStride,
                               aData.mCbCrStride,
                               aStride,
                               yuvtype);
  }
}

#ifdef MOZ_DUMP_PAINTING
 void
gfxUtils::WriteAsPNG(DrawTarget* aDT, const char* aFile)
{
  aDT->Flush();
  nsRefPtr<gfxASurface> surf = gfxPlatform::GetPlatform()->GetThebesSurfaceForDrawTarget(aDT);
  if (surf) {
    surf->WriteAsPNG(aFile);
  } else {
    NS_WARNING("Failed to get Thebes surface!");
  }
}

 void
gfxUtils::DumpAsDataURL(DrawTarget* aDT)
{
  aDT->Flush();
  nsRefPtr<gfxASurface> surf = gfxPlatform::GetPlatform()->GetThebesSurfaceForDrawTarget(aDT);
  if (surf) {
    surf->DumpAsDataURL();
  } else {
    NS_WARNING("Failed to get Thebes surface!");
  }
}

 void
gfxUtils::CopyAsDataURL(DrawTarget* aDT)
{
  aDT->Flush();
  nsRefPtr<gfxASurface> surf = gfxPlatform::GetPlatform()->GetThebesSurfaceForDrawTarget(aDT);
  if (surf) {
    surf->CopyAsDataURL();
  } else {
    NS_WARNING("Failed to get Thebes surface!");
  }
}

bool gfxUtils::sDumpPaintList = getenv("MOZ_DUMP_PAINT_LIST") != 0;
bool gfxUtils::sDumpPainting = getenv("MOZ_DUMP_PAINT") != 0;
bool gfxUtils::sDumpPaintingToFile = getenv("MOZ_DUMP_PAINT_TO_FILE") != 0;
FILE *gfxUtils::sDumpPaintFile = NULL;
#endif
