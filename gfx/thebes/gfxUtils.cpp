




#include "gfxUtils.h"
#include "gfxContext.h"
#include "gfxPlatform.h"
#include "gfxDrawable.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/RefPtr.h"
#include "nsRegion.h"
#include "yuv_convert.h"
#include "ycbcr_to_rgb565.h"
#include "GeckoProfiler.h"
#include "ImageContainer.h"
#include "gfx2DGlue.h"
#include "gfxPrefs.h"

#ifdef XP_WIN
#include "gfxWindowsPlatform.h"
#endif

using namespace mozilla;
using namespace mozilla::layers;
using namespace mozilla::gfx;

#include "DeprecatedPremultiplyTables.h"

static const uint8_t PremultiplyValue(uint8_t a, uint8_t v) {
    return gfxUtils::sPremultiplyTable[a*256+v];
}

static const uint8_t UnpremultiplyValue(uint8_t a, uint8_t v) {
    return gfxUtils::sUnpremultiplyTable[a*256+v];
}

void
gfxUtils::PremultiplyDataSurface(DataSourceSurface *aSurface)
{
    
    if (aSurface->GetFormat() != SurfaceFormat::B8G8R8A8) {
        return;
    }

    DataSourceSurface::MappedSurface map;
    if (!aSurface->Map(DataSourceSurface::MapType::READ_WRITE, &map)) {
        return;
    }
    MOZ_ASSERT(map.mStride == aSurface->GetSize().width * 4,
               "Source surface stride isn't tightly packed");

    uint8_t *src = map.mData;
    uint8_t *dst = map.mData;

    uint32_t dim = aSurface->GetSize().width * aSurface->GetSize().height;
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

    aSurface->Unmap();
}

TemporaryRef<DataSourceSurface>
gfxUtils::UnpremultiplyDataSurface(DataSourceSurface* aSurface)
{
    
    if (aSurface->GetFormat() != SurfaceFormat::B8G8R8A8) {
        return aSurface;
    }

    DataSourceSurface::MappedSurface map;
    if (!aSurface->Map(DataSourceSurface::MapType::READ, &map)) {
        return nullptr;
    }

    RefPtr<DataSourceSurface> dest = Factory::CreateDataSourceSurfaceWithStride(aSurface->GetSize(),
                                                                                aSurface->GetFormat(),
                                                                                map.mStride);

    DataSourceSurface::MappedSurface destMap;
    if (!dest->Map(DataSourceSurface::MapType::WRITE, &destMap)) {
        aSurface->Unmap();
        return nullptr;
    }

    uint8_t *src = map.mData;
    uint8_t *dst = destMap.mData;

    for (int32_t i = 0; i < aSurface->GetSize().height; ++i) {
        uint8_t *srcRow = src + (i * map.mStride);
        uint8_t *dstRow = dst + (i * destMap.mStride);

        for (int32_t j = 0; j < aSurface->GetSize().width; ++j) {
#ifdef IS_LITTLE_ENDIAN
          uint8_t b = *srcRow++;
          uint8_t g = *srcRow++;
          uint8_t r = *srcRow++;
          uint8_t a = *srcRow++;

          *dstRow++ = UnpremultiplyValue(a, b);
          *dstRow++ = UnpremultiplyValue(a, g);
          *dstRow++ = UnpremultiplyValue(a, r);
          *dstRow++ = a;
#else
          uint8_t a = *srcRow++;
          uint8_t r = *srcRow++;
          uint8_t g = *srcRow++;
          uint8_t b = *srcRow++;

          *dstRow++ = a;
          *dstRow++ = UnpremultiplyValue(a, r);
          *dstRow++ = UnpremultiplyValue(a, g);
          *dstRow++ = UnpremultiplyValue(a, b);
#endif
        }
    }

    aSurface->Unmap();
    dest->Unmap();
    return dest;
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

    MOZ_ASSERT(aSourceSurface->Format() == gfxImageFormat::ARGB32 || aSourceSurface->Format() == gfxImageFormat::RGB24,
               "Surfaces must be ARGB32 or RGB24");

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

void
gfxUtils::ConvertBGRAtoRGBA(uint8_t* aData, uint32_t aLength)
{
    uint8_t *src = aData;
    uint8_t *srcEnd = src + aLength;

    uint8_t buffer[4];
    for (; src != srcEnd; src += 4) {
        buffer[0] = src[2];
        buffer[1] = src[1];
        buffer[2] = src[0];

        src[0] = buffer[0];
        src[1] = buffer[1];
        src[2] = buffer[2];
    }
}

static bool
IsSafeImageTransformComponent(gfxFloat aValue)
{
  return aValue >= -32768 && aValue <= 32767;
}

#ifndef MOZ_GFX_OPTIMIZE_MOBILE





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



static already_AddRefed<gfxDrawable>
CreateSamplingRestrictedDrawable(gfxDrawable* aDrawable,
                                 gfxContext* aContext,
                                 const gfxMatrix& aUserSpaceToImageSpace,
                                 const gfxRect& aSourceRect,
                                 const gfxRect& aSubimage,
                                 const gfxImageFormat aFormat)
{
    PROFILER_LABEL("gfxUtils", "CreateSamplingRestricedDrawable",
      js::ProfileEntry::Category::GRAPHICS);

    gfxRect userSpaceClipExtents = aContext->GetClipExtents();
    
    
    
    
    
    gfxRect imageSpaceClipExtents =
        aUserSpaceToImageSpace.TransformBounds(userSpaceClipExtents);
    
    
    imageSpaceClipExtents.Inflate(1.0);

    gfxRect needed = imageSpaceClipExtents.Intersect(aSourceRect);
    needed = needed.Intersect(aSubimage);
    needed.RoundOut();

    
    
    
    
    if (needed.IsEmpty())
        return nullptr;

    nsRefPtr<gfxDrawable> drawable;
    gfxIntSize size(int32_t(needed.Width()), int32_t(needed.Height()));

    nsRefPtr<gfxImageSurface> image = aDrawable->GetAsImageSurface();
    if (image && gfxRect(0, 0, image->GetSize().width, image->GetSize().height).Contains(needed)) {
      nsRefPtr<gfxASurface> temp = image->GetSubimage(needed);
      drawable = new gfxSurfaceDrawable(temp, size, gfxMatrix().Translate(-needed.TopLeft()));
    } else {
      mozilla::RefPtr<mozilla::gfx::DrawTarget> target =
        gfxPlatform::GetPlatform()->CreateOffscreenContentDrawTarget(ToIntSize(size),
                                                                     ImageFormatToSurfaceFormat(aFormat));
      if (!target) {
        return nullptr;
      }

      nsRefPtr<gfxContext> tmpCtx = new gfxContext(target);
      tmpCtx->SetOperator(OptimalFillOperator());
      aDrawable->Draw(tmpCtx, needed - needed.TopLeft(), true,
                      GraphicsFilter::FILTER_FAST, gfxMatrix().Translate(needed.TopLeft()));
      drawable = new gfxSurfaceDrawable(target, size, gfxMatrix().Translate(-needed.TopLeft()));
    }

    return drawable.forget();
}
#endif 



struct MOZ_STACK_CLASS AutoCairoPixmanBugWorkaround
{
    AutoCairoPixmanBugWorkaround(gfxContext*      aContext,
                                 const gfxMatrix& aDeviceSpaceToImageSpace,
                                 const gfxRect&   aFill,
                                 const gfxASurface* aSurface)
     : mContext(aContext), mSucceeded(true), mPushedGroup(false)
    {
        
        if (!aSurface || aSurface->GetType() == gfxSurfaceType::Quartz)
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
        mContext->PushGroup(gfxContentType::COLOR_ALPHA);
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
static GraphicsFilter ReduceResamplingFilter(GraphicsFilter aFilter,
                                             int aImgWidth, int aImgHeight,
                                             float aSourceWidth, float aSourceHeight)
{
    
    
    const int kSmallImageSizeThreshold = 8;

    
    
    
    const float kLargeStretch = 3.0f;

    if (aImgWidth <= kSmallImageSizeThreshold
        || aImgHeight <= kSmallImageSizeThreshold) {
        
        
        return GraphicsFilter::FILTER_NEAREST;
    }

    if (aImgHeight * kLargeStretch <= aSourceHeight || aImgWidth * kLargeStretch <= aSourceWidth) {
        

        
        
        
        
        if (fabs(aSourceWidth - aImgWidth)/aImgWidth < 0.5 || fabs(aSourceHeight - aImgHeight)/aImgHeight < 0.5)
            return GraphicsFilter::FILTER_NEAREST;

        
        
        return aFilter;
    }

    

















    return aFilter;
}
#else
static GraphicsFilter ReduceResamplingFilter(GraphicsFilter aFilter,
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
                           const gfxImageFormat aFormat,
                           GraphicsFilter aFilter,
                           uint32_t         aImageFlags)
{
    PROFILER_LABEL("gfxUtils", "DrawPixelSnapped",
      js::ProfileEntry::Category::GRAPHICS);

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

    drawable->Draw(aContext, aFill, doTile, aFilter, userSpaceToImageSpace);
}

 int
gfxUtils::ImageFormatToDepth(gfxImageFormat aFormat)
{
    switch (aFormat) {
        case gfxImageFormat::ARGB32:
            return 32;
        case gfxImageFormat::RGB24:
            return 24;
        case gfxImageFormat::RGB16_565:
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

static TemporaryRef<Path>
PathFromRegionInternal(DrawTarget* aTarget, const nsIntRegion& aRegion,
                       bool aSnap)
{
  Matrix mat = aTarget->GetTransform();
  const gfxFloat epsilon = 0.000001;
#define WITHIN_E(a,b) (fabs((a)-(b)) < epsilon)
  
  bool shouldNotSnap = !aSnap || (WITHIN_E(mat._11,1.0) &&
                                  WITHIN_E(mat._22,1.0) &&
                                  WITHIN_E(mat._12,0.0) &&
                                  WITHIN_E(mat._21,0.0));
#undef WITHIN_E

  RefPtr<PathBuilder> pb = aTarget->CreatePathBuilder();
  nsIntRegionRectIterator iter(aRegion);

  const nsIntRect* r;
  if (shouldNotSnap) {
    while ((r = iter.Next()) != nullptr) {
      pb->MoveTo(Point(r->x, r->y));
      pb->LineTo(Point(r->XMost(), r->y));
      pb->LineTo(Point(r->XMost(), r->YMost()));
      pb->LineTo(Point(r->x, r->YMost()));
      pb->Close();
    }
  } else {
    while ((r = iter.Next()) != nullptr) {
      Rect rect(r->x, r->y, r->width, r->height);

      rect.Round();
      pb->MoveTo(rect.TopLeft());
      pb->LineTo(rect.TopRight());
      pb->LineTo(rect.BottomRight());
      pb->LineTo(rect.BottomLeft());
      pb->Close();
    }
  }
  RefPtr<Path> path = pb->Finish();
  return path;
}

static void
ClipToRegionInternal(DrawTarget* aTarget, const nsIntRegion& aRegion,
                     bool aSnap)
{
  RefPtr<Path> path = PathFromRegionInternal(aTarget, aRegion, aSnap);
  aTarget->PushClip(path);
}

 void
gfxUtils::ClipToRegion(gfxContext* aContext, const nsIntRegion& aRegion)
{
  ClipToRegionInternal(aContext, aRegion, false);
}

 void
gfxUtils::ClipToRegion(DrawTarget* aTarget, const nsIntRegion& aRegion)
{
  ClipToRegionInternal(aTarget, aRegion, false);
}

 void
gfxUtils::ClipToRegionSnapped(gfxContext* aContext, const nsIntRegion& aRegion)
{
  ClipToRegionInternal(aContext, aRegion, true);
}

 void
gfxUtils::ClipToRegionSnapped(DrawTarget* aTarget, const nsIntRegion& aRegion)
{
  ClipToRegionInternal(aTarget, aRegion, true);
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

Matrix
gfxUtils::TransformRectToRect(const gfxRect& aFrom, const IntPoint& aToTopLeft,
                              const IntPoint& aToTopRight, const IntPoint& aToBottomRight)
{
  Matrix m;
  if (aToTopRight.y == aToTopLeft.y && aToTopRight.x == aToBottomRight.x) {
    
    m._12 = m._21 = 0.0;
    m._11 = (aToBottomRight.x - aToTopLeft.x)/aFrom.width;
    m._22 = (aToBottomRight.y - aToTopLeft.y)/aFrom.height;
    m._31 = aToTopLeft.x - m._11*aFrom.x;
    m._32 = aToTopLeft.y - m._22*aFrom.y;
  } else {
    NS_ASSERTION(aToTopRight.y == aToBottomRight.y && aToTopRight.x == aToTopLeft.x,
                 "Destination rectangle not axis-aligned");
    m._11 = m._22 = 0.0;
    m._21 = (aToBottomRight.x - aToTopLeft.x)/aFrom.height;
    m._12 = (aToBottomRight.y - aToTopLeft.y)/aFrom.width;
    m._31 = aToTopLeft.x - m._21*aFrom.y;
    m._32 = aToTopLeft.y - m._12*aFrom.x;
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
gfxUtils::GetYCbCrToRGBDestFormatAndSize(const PlanarYCbCrData& aData,
                                         gfxImageFormat& aSuggestedFormat,
                                         gfxIntSize& aSuggestedSize)
{
  YUVType yuvtype =
    TypeFromSize(aData.mYSize.width,
                      aData.mYSize.height,
                      aData.mCbCrSize.width,
                      aData.mCbCrSize.height);

  
  
  bool prescale = aSuggestedSize.width > 0 && aSuggestedSize.height > 0 &&
                    ToIntSize(aSuggestedSize) != aData.mPicSize;

  if (aSuggestedFormat == gfxImageFormat::RGB16_565) {
#if defined(HAVE_YCBCR_TO_RGB565)
    if (prescale &&
        !IsScaleYCbCrToRGB565Fast(aData.mPicX,
                                       aData.mPicY,
                                       aData.mPicSize.width,
                                       aData.mPicSize.height,
                                       aSuggestedSize.width,
                                       aSuggestedSize.height,
                                       yuvtype,
                                       FILTER_BILINEAR) &&
        IsConvertYCbCrToRGB565Fast(aData.mPicX,
                                        aData.mPicY,
                                        aData.mPicSize.width,
                                        aData.mPicSize.height,
                                        yuvtype)) {
      prescale = false;
    }
#else
    
    aSuggestedFormat = gfxImageFormat::RGB24;
#endif
  }
  else if (aSuggestedFormat != gfxImageFormat::RGB24) {
    
    aSuggestedFormat = gfxImageFormat::RGB24;
  }
  if (aSuggestedFormat == gfxImageFormat::RGB24) {
    

    if (aData.mPicX != 0 || aData.mPicY != 0 || yuvtype == YV24)
      prescale = false;
  }
  if (!prescale) {
    ToIntSize(aSuggestedSize) = aData.mPicSize;
  }
}

void
gfxUtils::ConvertYCbCrToRGB(const PlanarYCbCrData& aData,
                            const gfxImageFormat& aDestFormat,
                            const gfxIntSize& aDestSize,
                            unsigned char* aDestBuffer,
                            int32_t aStride)
{
  
  
  MOZ_ASSERT((aData.mCbCrSize.width == aData.mYSize.width ||
              aData.mCbCrSize.width == (aData.mYSize.width + 1) >> 1) &&
             (aData.mCbCrSize.height == aData.mYSize.height ||
              aData.mCbCrSize.height == (aData.mYSize.height + 1) >> 1));
  YUVType yuvtype =
    TypeFromSize(aData.mYSize.width,
                      aData.mYSize.height,
                      aData.mCbCrSize.width,
                      aData.mCbCrSize.height);

  
  if (ToIntSize(aDestSize) != aData.mPicSize) {
#if defined(HAVE_YCBCR_TO_RGB565)
    if (aDestFormat == gfxImageFormat::RGB16_565) {
      ScaleYCbCrToRGB565(aData.mYChannel,
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
                              FILTER_BILINEAR);
    } else
#endif
      ScaleYCbCrToRGB32(aData.mYChannel,
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
                             ROTATE_0,
                             FILTER_BILINEAR);
  } else { 
#if defined(HAVE_YCBCR_TO_RGB565)
    if (aDestFormat == gfxImageFormat::RGB16_565) {
      ConvertYCbCrToRGB565(aData.mYChannel,
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
      ConvertYCbCrToRGB32(aData.mYChannel,
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

 TemporaryRef<DataSourceSurface>
gfxUtils::CopySurfaceToDataSourceSurfaceWithFormat(SourceSurface* aSurface,
                                                   SurfaceFormat aFormat)
{
  MOZ_ASSERT(aFormat != aSurface->GetFormat(),
             "Unnecessary - and very expersive - surface format conversion");

  Rect bounds(0, 0, aSurface->GetSize().width, aSurface->GetSize().height);

  if (aSurface->GetType() != SurfaceType::DATA) {
    
    
    
    
    
    
    RefPtr<DrawTarget> dt = gfxPlatform::GetPlatform()->
      CreateOffscreenContentDrawTarget(aSurface->GetSize(), aFormat);
    
    
    
    
    dt->DrawSurface(aSurface, bounds, bounds, DrawSurfaceOptions(),
                    DrawOptions(1.0f, CompositionOp::OP_OVER));
    RefPtr<SourceSurface> surface = dt->Snapshot();
    return surface->GetDataSurface();
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  RefPtr<DataSourceSurface> dataSurface =
    Factory::CreateDataSourceSurface(aSurface->GetSize(), aFormat);
  DataSourceSurface::MappedSurface map;
  if (!dataSurface ||
      !dataSurface->Map(DataSourceSurface::MapType::READ_WRITE, &map)) {
    return nullptr;
  }
  RefPtr<DrawTarget> dt =
    Factory::CreateDrawTargetForData(BackendType::CAIRO,
                                     map.mData,
                                     dataSurface->GetSize(),
                                     map.mStride,
                                     aFormat);
  if (!dt) {
    dataSurface->Unmap();
    return nullptr;
  }
  
  
  
  
  dt->DrawSurface(aSurface, bounds, bounds, DrawSurfaceOptions(),
                  DrawOptions(1.0f, CompositionOp::OP_OVER));
  dataSurface->Unmap();
  return dataSurface.forget();
}

const uint32_t gfxUtils::sNumFrameColors = 8;

 const gfx::Color&
gfxUtils::GetColorForFrameNumber(uint64_t aFrameNumber)
{
    static bool initialized = false;
    static gfx::Color colors[sNumFrameColors];

    if (!initialized) {
        uint32_t i = 0;
        colors[i++] = gfx::Color::FromABGR(0xffff0000);
        colors[i++] = gfx::Color::FromABGR(0xffcc00ff);
        colors[i++] = gfx::Color::FromABGR(0xff0066cc);
        colors[i++] = gfx::Color::FromABGR(0xff00ff00);
        colors[i++] = gfx::Color::FromABGR(0xff33ffff);
        colors[i++] = gfx::Color::FromABGR(0xffff0099);
        colors[i++] = gfx::Color::FromABGR(0xff0000ff);
        colors[i++] = gfx::Color::FromABGR(0xff999999);
        MOZ_ASSERT(i == sNumFrameColors);
        initialized = true;
    }

    return colors[aFrameNumber % sNumFrameColors];
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

 void
gfxUtils::WriteAsPNG(RefPtr<gfx::SourceSurface> aSourceSurface, const char* aFile)
{
  RefPtr<gfx::DataSourceSurface> dataSurface = aSourceSurface->GetDataSurface();
  RefPtr<gfx::DrawTarget> dt
            = gfxPlatform::GetPlatform()
                ->CreateDrawTargetForData(dataSurface->GetData(),
                                          dataSurface->GetSize(),
                                          dataSurface->Stride(),
                                          aSourceSurface->GetFormat());
  gfxUtils::WriteAsPNG(dt.get(), aFile);
}

 void
gfxUtils::DumpAsDataURL(RefPtr<gfx::SourceSurface> aSourceSurface)
{
  RefPtr<gfx::DataSourceSurface> dataSurface = aSourceSurface->GetDataSurface();
  RefPtr<gfx::DrawTarget> dt
            = gfxPlatform::GetPlatform()
                ->CreateDrawTargetForData(dataSurface->GetData(),
                                          dataSurface->GetSize(),
                                          dataSurface->Stride(),
                                          aSourceSurface->GetFormat());
  gfxUtils::DumpAsDataURL(dt.get());
}

 void
gfxUtils::CopyAsDataURL(RefPtr<gfx::SourceSurface> aSourceSurface)
{
  RefPtr<gfx::DataSourceSurface> dataSurface = aSourceSurface->GetDataSurface();
  RefPtr<gfx::DrawTarget> dt
            = gfxPlatform::GetPlatform()
                ->CreateDrawTargetForData(dataSurface->GetData(),
                                          dataSurface->GetSize(),
                                          dataSurface->Stride(),
                                          aSourceSurface->GetFormat());

  gfxUtils::CopyAsDataURL(dt.get());
}

static bool sDumpPaintList = getenv("MOZ_DUMP_PAINT_LIST") != 0;

 bool
gfxUtils::DumpPaintList() {
  return sDumpPaintList || gfxPrefs::LayoutDumpDisplayList();
}

bool gfxUtils::sDumpPainting = getenv("MOZ_DUMP_PAINT") != 0;
bool gfxUtils::sDumpPaintingToFile = getenv("MOZ_DUMP_PAINT_TO_FILE") != 0;
FILE *gfxUtils::sDumpPaintFile = nullptr;
#endif
