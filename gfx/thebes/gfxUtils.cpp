




































#include "gfxUtils.h"
#include "gfxContext.h"
#include "gfxPlatform.h"
#include "gfxDrawable.h"
#include "nsRegion.h"
#include "yuv_convert.h"
#include "ycbcr_to_rgb565.h"

#ifdef XP_WIN
#include "gfxWindowsPlatform.h"
#endif

using namespace mozilla;
using namespace mozilla::layers;

static PRUint8 sUnpremultiplyTable[256*256];
static PRUint8 sPremultiplyTable[256*256];
static bool sTablesInitialized = false;

static const PRUint8 PremultiplyValue(PRUint8 a, PRUint8 v) {
    return sPremultiplyTable[a*256+v];
}

static const PRUint8 UnpremultiplyValue(PRUint8 a, PRUint8 v) {
    return sUnpremultiplyTable[a*256+v];
}

static void
CalculateTables()
{
    
    
    
    
    

    

    
    for (PRUint32 c = 0; c <= 255; c++) {
        sUnpremultiplyTable[c] = c;
    }

    for (int a = 1; a <= 255; a++) {
        for (int c = 0; c <= 255; c++) {
            sUnpremultiplyTable[a*256+c] = (PRUint8)((c * 255) / a);
        }
    }

    

    for (int a = 0; a <= 255; a++) {
        for (int c = 0; c <= 255; c++) {
            sPremultiplyTable[a*256+c] = (a * c + 254) / 255;
        }
    }

    sTablesInitialized = true;
}

void
gfxUtils::PremultiplyImageSurface(gfxImageSurface *aSourceSurface,
                                  gfxImageSurface *aDestSurface)
{
    if (!aDestSurface)
        aDestSurface = aSourceSurface;

    NS_ASSERTION(aSourceSurface->Format() == aDestSurface->Format() &&
                 aSourceSurface->Width() == aDestSurface->Width() &&
                 aSourceSurface->Height() == aDestSurface->Height() &&
                 aSourceSurface->Stride() == aDestSurface->Stride(),
                 "Source and destination surfaces don't have identical characteristics");

    NS_ASSERTION(aSourceSurface->Stride() == aSourceSurface->Width() * 4,
                 "Source surface stride isn't tightly packed");

    
    if (aSourceSurface->Format() != gfxASurface::ImageFormatARGB32) {
        if (aDestSurface != aSourceSurface) {
            memcpy(aDestSurface->Data(), aSourceSurface->Data(),
                   aSourceSurface->Stride() * aSourceSurface->Height());
        }
        return;
    }

    if (!sTablesInitialized)
        CalculateTables();

    PRUint8 *src = aSourceSurface->Data();
    PRUint8 *dst = aDestSurface->Data();

    PRUint32 dim = aSourceSurface->Width() * aSourceSurface->Height();
    for (PRUint32 i = 0; i < dim; ++i) {
#ifdef IS_LITTLE_ENDIAN
        PRUint8 b = *src++;
        PRUint8 g = *src++;
        PRUint8 r = *src++;
        PRUint8 a = *src++;

        *dst++ = PremultiplyValue(a, b);
        *dst++ = PremultiplyValue(a, g);
        *dst++ = PremultiplyValue(a, r);
        *dst++ = a;
#else
        PRUint8 a = *src++;
        PRUint8 r = *src++;
        PRUint8 g = *src++;
        PRUint8 b = *src++;

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

    NS_ASSERTION(aSourceSurface->Format() == aDestSurface->Format() &&
                 aSourceSurface->Width() == aDestSurface->Width() &&
                 aSourceSurface->Height() == aDestSurface->Height() &&
                 aSourceSurface->Stride() == aDestSurface->Stride(),
                 "Source and destination surfaces don't have identical characteristics");

    NS_ASSERTION(aSourceSurface->Stride() == aSourceSurface->Width() * 4,
                 "Source surface stride isn't tightly packed");

    
    if (aSourceSurface->Format() != gfxASurface::ImageFormatARGB32) {
        if (aDestSurface != aSourceSurface) {
            memcpy(aDestSurface->Data(), aSourceSurface->Data(),
                   aSourceSurface->Stride() * aSourceSurface->Height());
        }
        return;
    }

    if (!sTablesInitialized)
        CalculateTables();

    PRUint8 *src = aSourceSurface->Data();
    PRUint8 *dst = aDestSurface->Data();

    PRUint32 dim = aSourceSurface->Width() * aSourceSurface->Height();
    for (PRUint32 i = 0; i < dim; ++i) {
#ifdef IS_LITTLE_ENDIAN
        PRUint8 b = *src++;
        PRUint8 g = *src++;
        PRUint8 r = *src++;
        PRUint8 a = *src++;

        *dst++ = UnpremultiplyValue(a, b);
        *dst++ = UnpremultiplyValue(a, g);
        *dst++ = UnpremultiplyValue(a, r);
        *dst++ = a;
#else
        PRUint8 a = *src++;
        PRUint8 r = *src++;
        PRUint8 g = *src++;
        PRUint8 b = *src++;

        *dst++ = a;
        *dst++ = UnpremultiplyValue(a, r);
        *dst++ = UnpremultiplyValue(a, g);
        *dst++ = UnpremultiplyValue(a, b);
#endif
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



static already_AddRefed<gfxDrawable>
CreateSamplingRestrictedDrawable(gfxDrawable* aDrawable,
                                 gfxContext* aContext,
                                 const gfxMatrix& aUserSpaceToImageSpace,
                                 const gfxRect& aSourceRect,
                                 const gfxRect& aSubimage,
                                 const gfxImageSurface::gfxImageFormat aFormat)
{
    gfxRect userSpaceClipExtents = aContext->GetClipExtents();
    
    
    
    
    
    gfxRect imageSpaceClipExtents =
        aUserSpaceToImageSpace.TransformBounds(userSpaceClipExtents);
    
    
    imageSpaceClipExtents.Inflate(1.0);

    gfxRect needed = imageSpaceClipExtents.Intersect(aSourceRect);
    needed = needed.Intersect(aSubimage);
    needed.RoundOut();

    
    
    
    
    if (needed.IsEmpty())
        return nsnull;

    gfxIntSize size(PRInt32(needed.Width()), PRInt32(needed.Height()));
    nsRefPtr<gfxASurface> temp =
        gfxPlatform::GetPlatform()->CreateOffscreenSurface(size, gfxASurface::ContentFromFormat(aFormat));
    if (!temp || temp->CairoStatus())
        return nsnull;

    nsRefPtr<gfxContext> tmpCtx = new gfxContext(temp);
    tmpCtx->SetOperator(OptimalFillOperator());
    aDrawable->Draw(tmpCtx, needed - needed.TopLeft(), true,
                    gfxPattern::FILTER_FAST, gfxMatrix().Translate(needed.TopLeft()));

    nsRefPtr<gfxPattern> resultPattern = new gfxPattern(temp);
    if (!resultPattern)
        return nsnull;

    nsRefPtr<gfxDrawable> drawable = 
        new gfxSurfaceDrawable(temp, size, gfxMatrix().Translate(-needed.TopLeft()));
    return drawable.forget();
}



struct NS_STACK_CLASS AutoCairoPixmanBugWorkaround
{
    AutoCairoPixmanBugWorkaround(gfxContext*      aContext,
                                 const gfxMatrix& aDeviceSpaceToImageSpace,
                                 const gfxRect&   aFill,
                                 const gfxASurface::gfxSurfaceType& aSurfaceType)
     : mContext(aContext), mSucceeded(true), mPushedGroup(false)
    {
        
        if (aSurfaceType == gfxASurface::SurfaceTypeQuartz)
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

 void
gfxUtils::DrawPixelSnapped(gfxContext*      aContext,
                           gfxDrawable*     aDrawable,
                           const gfxMatrix& aUserSpaceToImageSpace,
                           const gfxRect&   aSubimage,
                           const gfxRect&   aSourceRect,
                           const gfxRect&   aImageRect,
                           const gfxRect&   aFill,
                           const gfxImageSurface::gfxImageFormat aFormat,
                           const gfxPattern::GraphicsFilter& aFilter)
{
    bool doTile = !aImageRect.Contains(aSourceRect);

    nsRefPtr<gfxASurface> currentTarget = aContext->CurrentSurface();
    gfxASurface::gfxSurfaceType surfaceType = currentTarget->GetType();
    gfxMatrix deviceSpaceToImageSpace =
        DeviceToImageTransform(aContext, aUserSpaceToImageSpace);

    AutoCairoPixmanBugWorkaround workaround(aContext, deviceSpaceToImageSpace,
                                            aFill, surfaceType);
    if (!workaround.Succeeded())
        return;

    nsRefPtr<gfxDrawable> drawable = aDrawable;

    
    
    
    
    
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

    gfxContext::GraphicsOperator op = aContext->CurrentOperator();
    if ((op == gfxContext::OPERATOR_OVER || workaround.PushedGroup()) &&
        aFormat == gfxASurface::ImageFormatRGB24) {
        aContext->SetOperator(OptimalFillOperator());
    }

    drawable->Draw(aContext, aFill, doTile, aFilter, aUserSpaceToImageSpace);

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
  while ((r = iter.Next()) != nsnull) {
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

  gfxFloat power = log(aVal)/log(kScaleResolution);

  
  
  
  if (fabs(power - NS_round(power)) < 1e-6) {
    power = NS_round(power);
  } else {
    power = ceil(power);
  }

  return pow(kScaleResolution, power);
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


bool
gfxUtils::GfxRectToIntRect(const gfxRect& aIn, nsIntRect* aOut)
{
  *aOut = nsIntRect(PRInt32(aIn.X()), PRInt32(aIn.Y()),
  PRInt32(aIn.Width()), PRInt32(aIn.Height()));
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
                            PRInt32 aStride)
{
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
