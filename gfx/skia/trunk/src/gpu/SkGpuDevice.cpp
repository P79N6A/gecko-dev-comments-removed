






#include "SkGpuDevice.h"

#include "effects/GrBicubicEffect.h"
#include "effects/GrTextureDomain.h"
#include "effects/GrSimpleTextureEffect.h"

#include "GrContext.h"
#include "GrBitmapTextContext.h"
#include "GrDistanceFieldTextContext.h"

#include "SkGrTexturePixelRef.h"

#include "SkBounder.h"
#include "SkColorFilter.h"
#include "SkDeviceImageFilterProxy.h"
#include "SkDrawProcs.h"
#include "SkGlyphCache.h"
#include "SkImageFilter.h"
#include "SkMaskFilter.h"
#include "SkPathEffect.h"
#include "SkPicture.h"
#include "SkRRect.h"
#include "SkStroke.h"
#include "SkSurface.h"
#include "SkTLazy.h"
#include "SkUtils.h"
#include "SkErrorInternals.h"

#define CACHE_COMPATIBLE_DEVICE_TEXTURES 1

#if 0
    extern bool (*gShouldDrawProc)();
    #define CHECK_SHOULD_DRAW(draw, forceI)                     \
        do {                                                    \
            if (gShouldDrawProc && !gShouldDrawProc()) return;  \
            this->prepareDraw(draw, forceI);                    \
        } while (0)
#else
    #define CHECK_SHOULD_DRAW(draw, forceI) this->prepareDraw(draw, forceI)
#endif




#define COLOR_BLEED_TOLERANCE 0.001f

#define DO_DEFERRED_CLEAR()             \
    do {                                \
        if (fNeedClear) {               \
            this->clear(SK_ColorTRANSPARENT); \
        }                               \
    } while (false)                     \



#define CHECK_FOR_ANNOTATION(paint) \
    do { if (paint.getAnnotation()) { return; } } while (0)




class SkGpuDevice::SkAutoCachedTexture : public ::SkNoncopyable {
public:
    SkAutoCachedTexture()
        : fDevice(NULL)
        , fTexture(NULL) {
    }

    SkAutoCachedTexture(SkGpuDevice* device,
                        const SkBitmap& bitmap,
                        const GrTextureParams* params,
                        GrTexture** texture)
        : fDevice(NULL)
        , fTexture(NULL) {
        SkASSERT(NULL != texture);
        *texture = this->set(device, bitmap, params);
    }

    ~SkAutoCachedTexture() {
        if (NULL != fTexture) {
            GrUnlockAndUnrefCachedBitmapTexture(fTexture);
        }
    }

    GrTexture* set(SkGpuDevice* device,
                   const SkBitmap& bitmap,
                   const GrTextureParams* params) {
        if (NULL != fTexture) {
            GrUnlockAndUnrefCachedBitmapTexture(fTexture);
            fTexture = NULL;
        }
        fDevice = device;
        GrTexture* result = (GrTexture*)bitmap.getTexture();
        if (NULL == result) {
            
            fTexture = GrLockAndRefCachedBitmapTexture(device->context(), bitmap, params);
            result = fTexture;
        }
        return result;
    }

private:
    SkGpuDevice* fDevice;
    GrTexture*   fTexture;
};



struct GrSkDrawProcs : public SkDrawProcs {
public:
    GrContext* fContext;
    GrTextContext* fTextContext;
    GrFontScaler* fFontScaler;  
};



static SkBitmap::Config grConfig2skConfig(GrPixelConfig config, bool* isOpaque) {
    switch (config) {
        case kAlpha_8_GrPixelConfig:
            *isOpaque = false;
            return SkBitmap::kA8_Config;
        case kRGB_565_GrPixelConfig:
            *isOpaque = true;
            return SkBitmap::kRGB_565_Config;
        case kRGBA_4444_GrPixelConfig:
            *isOpaque = false;
            return SkBitmap::kARGB_4444_Config;
        case kSkia8888_GrPixelConfig:
            
            
            *isOpaque = false;
            return SkBitmap::kARGB_8888_Config;
        default:
            *isOpaque = false;
            return SkBitmap::kNo_Config;
    }
}





static SkBitmap make_bitmap(GrContext* context, GrRenderTarget* renderTarget) {
    bool isOpaque;
    SkBitmap::Config config = grConfig2skConfig(renderTarget->config(), &isOpaque);

    SkBitmap bitmap;
    bitmap.setConfig(config, renderTarget->width(), renderTarget->height(), 0,
                     isOpaque ? kOpaque_SkAlphaType : kPremul_SkAlphaType);
    return bitmap;
}

SkGpuDevice* SkGpuDevice::Create(GrSurface* surface) {
    SkASSERT(NULL != surface);
    if (NULL == surface->asRenderTarget() || NULL == surface->getContext()) {
        return NULL;
    }
    if (surface->asTexture()) {
        return SkNEW_ARGS(SkGpuDevice, (surface->getContext(), surface->asTexture()));
    } else {
        return SkNEW_ARGS(SkGpuDevice, (surface->getContext(), surface->asRenderTarget()));
    }
}

SkGpuDevice::SkGpuDevice(GrContext* context, GrTexture* texture)
    : SkBitmapDevice(make_bitmap(context, texture->asRenderTarget())) {
    this->initFromRenderTarget(context, texture->asRenderTarget(), false);
}

SkGpuDevice::SkGpuDevice(GrContext* context, GrRenderTarget* renderTarget)
    : SkBitmapDevice(make_bitmap(context, renderTarget)) {
    this->initFromRenderTarget(context, renderTarget, false);
}

void SkGpuDevice::initFromRenderTarget(GrContext* context,
                                       GrRenderTarget* renderTarget,
                                       bool cached) {
    fDrawProcs = NULL;

    fContext = context;
    fContext->ref();

    fMainTextContext = SkNEW_ARGS(GrDistanceFieldTextContext, (fContext, fLeakyProperties));
    fFallbackTextContext = SkNEW_ARGS(GrBitmapTextContext, (fContext, fLeakyProperties));

    fRenderTarget = NULL;
    fNeedClear = false;

    SkASSERT(NULL != renderTarget);
    fRenderTarget = renderTarget;
    fRenderTarget->ref();

    
    
    
    
    GrSurface* surface = fRenderTarget->asTexture();
    if (NULL == surface) {
        surface = fRenderTarget;
    }

    SkImageInfo info;
    surface->asImageInfo(&info);
    SkPixelRef* pr = SkNEW_ARGS(SkGrPixelRef, (info, surface, cached));

    this->setPixelRef(pr)->unref();
}

SkGpuDevice* SkGpuDevice::Create(GrContext* context, const SkImageInfo& origInfo,
                                 int sampleCount) {
    if (kUnknown_SkColorType == origInfo.colorType() ||
        origInfo.width() < 0 || origInfo.height() < 0) {
        return NULL;
    }

    SkImageInfo info = origInfo;
    
    
    if (kRGB_565_SkColorType == info.colorType()) {
        info.fAlphaType = kOpaque_SkAlphaType;  
    } else {
        info.fColorType = kPMColor_SkColorType;
        if (kOpaque_SkAlphaType != info.alphaType()) {
            info.fAlphaType = kPremul_SkAlphaType;  
        }
    }

    GrTextureDesc desc;
    desc.fFlags = kRenderTarget_GrTextureFlagBit;
    desc.fWidth = info.width();
    desc.fHeight = info.height();
    desc.fConfig = SkImageInfo2GrPixelConfig(info.colorType(), info.alphaType());
    desc.fSampleCnt = sampleCount;

    SkAutoTUnref<GrTexture> texture(context->createUncachedTexture(desc, NULL, 0));
    if (!texture.get()) {
        return NULL;
    }

    return SkNEW_ARGS(SkGpuDevice, (context, texture.get()));
}

#ifdef SK_SUPPORT_LEGACY_COMPATIBLEDEVICE_CONFIG
static SkBitmap make_bitmap(SkBitmap::Config config, int width, int height) {
    SkBitmap bm;
    bm.setConfig(SkImageInfo::Make(width, height,
                                   SkBitmapConfigToColorType(config),
                                   kPremul_SkAlphaType));
    return bm;
}
SkGpuDevice::SkGpuDevice(GrContext* context,
                         SkBitmap::Config config,
                         int width,
                         int height,
                         int sampleCount)
    : SkBitmapDevice(make_bitmap(config, width, height))
{
    fDrawProcs = NULL;

    fContext = context;
    fContext->ref();

    fMainTextContext = SkNEW_ARGS(GrDistanceFieldTextContext, (fContext, fLeakyProperties));
    fFallbackTextContext = SkNEW_ARGS(GrBitmapTextContext, (fContext, fLeakyProperties));

    fRenderTarget = NULL;
    fNeedClear = false;

    if (config != SkBitmap::kRGB_565_Config) {
        config = SkBitmap::kARGB_8888_Config;
    }

    GrTextureDesc desc;
    desc.fFlags = kRenderTarget_GrTextureFlagBit;
    desc.fWidth = width;
    desc.fHeight = height;
    desc.fConfig = SkBitmapConfig2GrPixelConfig(config);
    desc.fSampleCnt = sampleCount;

    SkImageInfo info;
    if (!GrPixelConfig2ColorType(desc.fConfig, &info.fColorType)) {
        sk_throw();
    }
    info.fWidth = width;
    info.fHeight = height;
    info.fAlphaType = kPremul_SkAlphaType;

    SkAutoTUnref<GrTexture> texture(fContext->createUncachedTexture(desc, NULL, 0));

    if (NULL != texture) {
        fRenderTarget = texture->asRenderTarget();
        fRenderTarget->ref();

        SkASSERT(NULL != fRenderTarget);

        
        SkGrPixelRef* pr = SkNEW_ARGS(SkGrPixelRef, (info, texture));
        this->setPixelRef(pr)->unref();
    } else {
        GrPrintf("--- failed to create gpu-offscreen [%d %d]\n",
                 width, height);
        SkASSERT(false);
    }
}
#endif

SkGpuDevice::~SkGpuDevice() {
    if (fDrawProcs) {
        delete fDrawProcs;
    }

    delete fMainTextContext;
    delete fFallbackTextContext;

    
    
    if (fContext->getRenderTarget() == fRenderTarget) {
        fContext->setRenderTarget(NULL);
    }

    if (fContext->getClip() == &fClipData) {
        fContext->setClip(NULL);
    }

    SkSafeUnref(fRenderTarget);
    fContext->unref();
}



void SkGpuDevice::makeRenderTargetCurrent() {
    DO_DEFERRED_CLEAR();
    fContext->setRenderTarget(fRenderTarget);
}



namespace {
GrPixelConfig config8888_to_grconfig_and_flags(SkCanvas::Config8888 config8888, uint32_t* flags) {
    switch (config8888) {
        case SkCanvas::kNative_Premul_Config8888:
            *flags = 0;
            return kSkia8888_GrPixelConfig;
        case SkCanvas::kNative_Unpremul_Config8888:
            *flags = GrContext::kUnpremul_PixelOpsFlag;
            return kSkia8888_GrPixelConfig;
        case SkCanvas::kBGRA_Premul_Config8888:
            *flags = 0;
            return kBGRA_8888_GrPixelConfig;
        case SkCanvas::kBGRA_Unpremul_Config8888:
            *flags = GrContext::kUnpremul_PixelOpsFlag;
            return kBGRA_8888_GrPixelConfig;
        case SkCanvas::kRGBA_Premul_Config8888:
            *flags = 0;
            return kRGBA_8888_GrPixelConfig;
        case SkCanvas::kRGBA_Unpremul_Config8888:
            *flags = GrContext::kUnpremul_PixelOpsFlag;
            return kRGBA_8888_GrPixelConfig;
        default:
            GrCrash("Unexpected Config8888.");
            *flags = 0; 
            return kSkia8888_GrPixelConfig;
    }
}
}

bool SkGpuDevice::onReadPixels(const SkBitmap& bitmap,
                               int x, int y,
                               SkCanvas::Config8888 config8888) {
    DO_DEFERRED_CLEAR();
    SkASSERT(SkBitmap::kARGB_8888_Config == bitmap.config());
    SkASSERT(!bitmap.isNull());
    SkASSERT(SkIRect::MakeWH(this->width(), this->height()).contains(SkIRect::MakeXYWH(x, y, bitmap.width(), bitmap.height())));

    SkAutoLockPixels alp(bitmap);
    GrPixelConfig config;
    uint32_t flags;
    config = config8888_to_grconfig_and_flags(config8888, &flags);
    return fContext->readRenderTargetPixels(fRenderTarget,
                                            x, y,
                                            bitmap.width(),
                                            bitmap.height(),
                                            config,
                                            bitmap.getPixels(),
                                            bitmap.rowBytes(),
                                            flags);
}

#ifdef SK_SUPPORT_LEGACY_WRITEPIXELSCONFIG
void SkGpuDevice::writePixels(const SkBitmap& bitmap, int x, int y,
                              SkCanvas::Config8888 config8888) {
    SkAutoLockPixels alp(bitmap);
    if (!bitmap.readyToDraw()) {
        return;
    }

    GrPixelConfig config;
    uint32_t flags;
    if (SkBitmap::kARGB_8888_Config == bitmap.config()) {
        config = config8888_to_grconfig_and_flags(config8888, &flags);
    } else {
        flags = 0;
        config= SkBitmapConfig2GrPixelConfig(bitmap.config());
    }

    fRenderTarget->writePixels(x, y, bitmap.width(), bitmap.height(),
                               config, bitmap.getPixels(), bitmap.rowBytes(), flags);
}
#endif

bool SkGpuDevice::onWritePixels(const SkImageInfo& info, const void* pixels, size_t rowBytes,
                                int x, int y) {
    
    GrPixelConfig config = SkImageInfo2GrPixelConfig(info.colorType(), info.alphaType());
    if (kUnknown_GrPixelConfig == config) {
        return false;
    }
    uint32_t flags = 0;
    if (kUnpremul_SkAlphaType == info.alphaType()) {
        flags = GrContext::kUnpremul_PixelOpsFlag;
    }
    fRenderTarget->writePixels(x, y, info.width(), info.height(), config, pixels, rowBytes, flags);

    
    this->onAccessBitmap().notifyPixelsChanged();

    return true;
}

void SkGpuDevice::onAttachToCanvas(SkCanvas* canvas) {
    INHERITED::onAttachToCanvas(canvas);

    
    fClipData.fClipStack = canvas->getClipStack();
}

void SkGpuDevice::onDetachFromCanvas() {
    INHERITED::onDetachFromCanvas();
    fClipData.fClipStack = NULL;
}



void SkGpuDevice::prepareDraw(const SkDraw& draw, bool forceIdentity) {
    SkASSERT(NULL != fClipData.fClipStack);

    fContext->setRenderTarget(fRenderTarget);

    SkASSERT(draw.fClipStack && draw.fClipStack == fClipData.fClipStack);

    if (forceIdentity) {
        fContext->setIdentityMatrix();
    } else {
        fContext->setMatrix(*draw.fMatrix);
    }
    fClipData.fOrigin = this->getOrigin();

    fContext->setClip(&fClipData);

    DO_DEFERRED_CLEAR();
}

GrRenderTarget* SkGpuDevice::accessRenderTarget() {
    DO_DEFERRED_CLEAR();
    return fRenderTarget;
}



SK_COMPILE_ASSERT(SkShader::kNone_BitmapType == 0, shader_type_mismatch);
SK_COMPILE_ASSERT(SkShader::kDefault_BitmapType == 1, shader_type_mismatch);
SK_COMPILE_ASSERT(SkShader::kRadial_BitmapType == 2, shader_type_mismatch);
SK_COMPILE_ASSERT(SkShader::kSweep_BitmapType == 3, shader_type_mismatch);
SK_COMPILE_ASSERT(SkShader::kTwoPointRadial_BitmapType == 4,
                  shader_type_mismatch);
SK_COMPILE_ASSERT(SkShader::kTwoPointConical_BitmapType == 5,
                  shader_type_mismatch);
SK_COMPILE_ASSERT(SkShader::kLinear_BitmapType == 6, shader_type_mismatch);
SK_COMPILE_ASSERT(SkShader::kLast_BitmapType == 6, shader_type_mismatch);

namespace {







inline bool skPaint2GrPaintNoShader(SkGpuDevice* dev,
                                    const SkPaint& skPaint,
                                    bool justAlpha,
                                    bool constantColor,
                                    GrPaint* grPaint) {

    grPaint->setDither(skPaint.isDither());
    grPaint->setAntiAlias(skPaint.isAntiAlias());

    SkXfermode::Coeff sm;
    SkXfermode::Coeff dm;

    SkXfermode* mode = skPaint.getXfermode();
    GrEffectRef* xferEffect = NULL;
    if (SkXfermode::AsNewEffectOrCoeff(mode, &xferEffect, &sm, &dm)) {
        if (NULL != xferEffect) {
            grPaint->addColorEffect(xferEffect)->unref();
            sm = SkXfermode::kOne_Coeff;
            dm = SkXfermode::kZero_Coeff;
        }
    } else {
        
#if 0
        return false;
#else
        
        sm = SkXfermode::kOne_Coeff;
        dm = SkXfermode::kISA_Coeff;
#endif
    }
    grPaint->setBlendFunc(sk_blend_to_grblend(sm), sk_blend_to_grblend(dm));

    if (justAlpha) {
        uint8_t alpha = skPaint.getAlpha();
        grPaint->setColor(GrColorPackRGBA(alpha, alpha, alpha, alpha));
        
        
        SkASSERT(!constantColor);
    } else {
        grPaint->setColor(SkColor2GrColor(skPaint.getColor()));
    }

    SkColorFilter* colorFilter = skPaint.getColorFilter();
    if (NULL != colorFilter) {
        
        
        if (constantColor) {
            SkColor filtered = colorFilter->filterColor(skPaint.getColor());
            grPaint->setColor(SkColor2GrColor(filtered));
        } else {
            SkAutoTUnref<GrEffectRef> effect(colorFilter->asNewEffect(dev->context()));
            if (NULL != effect.get()) {
                grPaint->addColorEffect(effect);
            }
        }
    }

    return true;
}





inline bool skPaint2GrPaintShader(SkGpuDevice* dev,
                                  const SkPaint& skPaint,
                                  bool constantColor,
                                  GrPaint* grPaint) {
    SkShader* shader = skPaint.getShader();
    if (NULL == shader) {
        return skPaint2GrPaintNoShader(dev, skPaint, false, constantColor, grPaint);
    }

    
    
    GrContext::AutoWideOpenIdentityDraw awo(dev->context(), NULL);

    
    SkAutoTUnref<GrEffectRef> effect(shader->asNewEffect(dev->context(), skPaint));
    if (NULL != effect.get()) {
        grPaint->addColorEffect(effect);
        
        return skPaint2GrPaintNoShader(dev, skPaint, true, false, grPaint);
    } else {
        
        SkShader::GradientInfo info;
        SkColor                color;

        info.fColors = &color;
        info.fColorOffsets = NULL;
        info.fColorCount = 1;
        if (SkShader::kColor_GradientType == shader->asAGradient(&info)) {
            SkPaint copy(skPaint);
            copy.setShader(NULL);
            
            U8CPU newA = SkMulDiv255Round(SkColorGetA(color), copy.getAlpha());
            copy.setColor(SkColorSetA(color, newA));
            return skPaint2GrPaintNoShader(dev, copy, false, constantColor, grPaint);
        } else {
            return false;
        }
    }
}
}



SkBitmap::Config SkGpuDevice::config() const {
    if (NULL == fRenderTarget) {
        return SkBitmap::kNo_Config;
    }

    bool isOpaque;
    return grConfig2skConfig(fRenderTarget->config(), &isOpaque);
}

void SkGpuDevice::clear(SkColor color) {
    SkIRect rect = SkIRect::MakeWH(this->width(), this->height());
    fContext->clear(&rect, SkColor2GrColor(color), true, fRenderTarget);
    fNeedClear = false;
}

void SkGpuDevice::drawPaint(const SkDraw& draw, const SkPaint& paint) {
    CHECK_SHOULD_DRAW(draw, false);

    GrPaint grPaint;
    if (!skPaint2GrPaintShader(this, paint, true, &grPaint)) {
        return;
    }

    fContext->drawPaint(grPaint);
}


static const GrPrimitiveType gPointMode2PrimtiveType[] = {
    kPoints_GrPrimitiveType,
    kLines_GrPrimitiveType,
    kLineStrip_GrPrimitiveType
};

void SkGpuDevice::drawPoints(const SkDraw& draw, SkCanvas::PointMode mode,
                             size_t count, const SkPoint pts[], const SkPaint& paint) {
    CHECK_FOR_ANNOTATION(paint);
    CHECK_SHOULD_DRAW(draw, false);

    SkScalar width = paint.getStrokeWidth();
    if (width < 0) {
        return;
    }

    
    
    if (width > 0 || paint.getPathEffect() || paint.getMaskFilter()) {
        draw.drawPoints(mode, count, pts, paint, true);
        return;
    }

    GrPaint grPaint;
    if (!skPaint2GrPaintShader(this, paint, true, &grPaint)) {
        return;
    }

    fContext->drawVertices(grPaint,
                           gPointMode2PrimtiveType[mode],
                           SkToS32(count),
                           (GrPoint*)pts,
                           NULL,
                           NULL,
                           NULL,
                           0);
}



void SkGpuDevice::drawRect(const SkDraw& draw, const SkRect& rect,
                           const SkPaint& paint) {
    CHECK_FOR_ANNOTATION(paint);
    CHECK_SHOULD_DRAW(draw, false);

    bool doStroke = paint.getStyle() != SkPaint::kFill_Style;
    SkScalar width = paint.getStrokeWidth();

    



    bool usePath = doStroke && width > 0 &&
                   (paint.getStrokeJoin() == SkPaint::kRound_Join ||
                    (paint.getStrokeJoin() == SkPaint::kBevel_Join && rect.isEmpty()));
    
    if (paint.getMaskFilter() || paint.getPathEffect()) {
        usePath = true;
    }
    if (!usePath && paint.isAntiAlias() && !fContext->getMatrix().rectStaysRect()) {
#if defined(SHADER_AA_FILL_RECT) || !defined(IGNORE_ROT_AA_RECT_OPT)
        if (doStroke) {
#endif
            usePath = true;
#if defined(SHADER_AA_FILL_RECT) || !defined(IGNORE_ROT_AA_RECT_OPT)
        } else {
            usePath = !fContext->getMatrix().preservesRightAngles();
        }
#endif
    }
    
    if (paint.getStyle() == SkPaint::kStrokeAndFill_Style) {
        usePath = true;
    }

    if (usePath) {
        SkPath path;
        path.addRect(rect);
        this->drawPath(draw, path, paint, NULL, true);
        return;
    }

    GrPaint grPaint;
    if (!skPaint2GrPaintShader(this, paint, true, &grPaint)) {
        return;
    }

    if (!doStroke) {
        fContext->drawRect(grPaint, rect);
    } else {
        SkStrokeRec stroke(paint);
        fContext->drawRect(grPaint, rect, &stroke);
    }
}



void SkGpuDevice::drawRRect(const SkDraw& draw, const SkRRect& rect,
                           const SkPaint& paint) {
    CHECK_FOR_ANNOTATION(paint);
    CHECK_SHOULD_DRAW(draw, false);

    GrPaint grPaint;
    if (!skPaint2GrPaintShader(this, paint, true, &grPaint)) {
        return;
    }

    SkStrokeRec stroke(paint);
    if (paint.getMaskFilter()) {
        

        SkRRect devRRect;
        if (rect.transform(fContext->getMatrix(), &devRRect)) {
            if (devRRect.allCornersCircular()) {
                SkRect maskRect;
                if (paint.getMaskFilter()->canFilterMaskGPU(devRRect.rect(),
                                            draw.fClip->getBounds(),
                                            fContext->getMatrix(),
                                            &maskRect)) {
                    SkIRect finalIRect;
                    maskRect.roundOut(&finalIRect);
                    if (draw.fClip->quickReject(finalIRect)) {
                        
                        return;
                    }
                    if (NULL != draw.fBounder && !draw.fBounder->doIRect(finalIRect)) {
                        
                        return;
                    }
                    if (paint.getMaskFilter()->directFilterRRectMaskGPU(fContext, &grPaint,
                                                                        stroke, devRRect)) {
                        return;
                    }
                }

            }
        }

    }

    bool usePath = !rect.isSimple();
    
    if (paint.getMaskFilter() || paint.getPathEffect()) {
        usePath = true;
    }
    
    if (!usePath && !fContext->getMatrix().rectStaysRect()) {
        usePath = true;
    }

    if (usePath) {
        SkPath path;
        path.addRRect(rect);
        this->drawPath(draw, path, paint, NULL, true);
        return;
    }

    fContext->drawRRect(grPaint, rect, stroke);
}



void SkGpuDevice::drawOval(const SkDraw& draw, const SkRect& oval,
                           const SkPaint& paint) {
    CHECK_FOR_ANNOTATION(paint);
    CHECK_SHOULD_DRAW(draw, false);

    bool usePath = false;
    
    if (paint.getMaskFilter() || paint.getPathEffect()) {
        usePath = true;
    }

    if (usePath) {
        SkPath path;
        path.addOval(oval);
        this->drawPath(draw, path, paint, NULL, true);
        return;
    }

    GrPaint grPaint;
    if (!skPaint2GrPaintShader(this, paint, true, &grPaint)) {
        return;
    }
    SkStrokeRec stroke(paint);

    fContext->drawOval(grPaint, oval, stroke);
}

#include "SkMaskFilter.h"
#include "SkBounder.h"




namespace {




bool draw_mask(GrContext* context, const SkRect& maskRect,
               GrPaint* grp, GrTexture* mask) {
    GrContext::AutoMatrix am;
    if (!am.setIdentity(context, grp)) {
        return false;
    }

    SkMatrix matrix;
    matrix.setTranslate(-maskRect.fLeft, -maskRect.fTop);
    matrix.postIDiv(mask->width(), mask->height());

    grp->addCoverageEffect(GrSimpleTextureEffect::Create(mask, matrix))->unref();
    context->drawRect(*grp, maskRect);
    return true;
}

bool draw_with_mask_filter(GrContext* context, const SkPath& devPath,
                           SkMaskFilter* filter, const SkRegion& clip, SkBounder* bounder,
                           GrPaint* grp, SkPaint::Style style) {
    SkMask  srcM, dstM;

    if (!SkDraw::DrawToMask(devPath, &clip.getBounds(), filter, &context->getMatrix(), &srcM,
                            SkMask::kComputeBoundsAndRenderImage_CreateMode, style)) {
        return false;
    }
    SkAutoMaskFreeImage autoSrc(srcM.fImage);

    if (!filter->filterMask(&dstM, srcM, context->getMatrix(), NULL)) {
        return false;
    }
    
    SkAutoMaskFreeImage autoDst(dstM.fImage);

    if (clip.quickReject(dstM.fBounds)) {
        return false;
    }
    if (bounder && !bounder->doIRect(dstM.fBounds)) {
        return false;
    }

    
    
    GrTextureDesc desc;
    desc.fWidth = dstM.fBounds.width();
    desc.fHeight = dstM.fBounds.height();
    desc.fConfig = kAlpha_8_GrPixelConfig;

    GrAutoScratchTexture ast(context, desc);
    GrTexture* texture = ast.texture();

    if (NULL == texture) {
        return false;
    }
    texture->writePixels(0, 0, desc.fWidth, desc.fHeight, desc.fConfig,
                               dstM.fImage, dstM.fRowBytes);

    SkRect maskRect = SkRect::Make(dstM.fBounds);

    return draw_mask(context, maskRect, grp, texture);
}



bool create_mask_GPU(GrContext* context,
                     const SkRect& maskRect,
                     const SkPath& devPath,
                     const SkStrokeRec& stroke,
                     bool doAA,
                     GrAutoScratchTexture* mask) {
    GrTextureDesc desc;
    desc.fFlags = kRenderTarget_GrTextureFlagBit;
    desc.fWidth = SkScalarCeilToInt(maskRect.width());
    desc.fHeight = SkScalarCeilToInt(maskRect.height());
    
    
    desc.fConfig = kRGBA_8888_GrPixelConfig;
    if (context->isConfigRenderable(kAlpha_8_GrPixelConfig, false)) {
        desc.fConfig = kAlpha_8_GrPixelConfig;
    }

    mask->set(context, desc);
    if (NULL == mask->texture()) {
        return false;
    }

    GrTexture* maskTexture = mask->texture();
    SkRect clipRect = SkRect::MakeWH(maskRect.width(), maskRect.height());

    GrContext::AutoRenderTarget art(context, maskTexture->asRenderTarget());
    GrContext::AutoClip ac(context, clipRect);

    context->clear(NULL, 0x0, true);

    GrPaint tempPaint;
    if (doAA) {
        tempPaint.setAntiAlias(true);
        
        
        
        
        
        
        tempPaint.setBlendFunc(kOne_GrBlendCoeff, kISC_GrBlendCoeff);
    }

    GrContext::AutoMatrix am;

    
    SkMatrix translate;
    translate.setTranslate(-maskRect.fLeft, -maskRect.fTop);
    am.set(context, translate);
    context->drawPath(tempPaint, devPath, stroke);
    return true;
}

SkBitmap wrap_texture(GrTexture* texture) {
    SkImageInfo info;
    texture->asImageInfo(&info);

    SkBitmap result;
    result.setConfig(info);
    result.setPixelRef(SkNEW_ARGS(SkGrPixelRef, (info, texture)))->unref();
    return result;
}

};

void SkGpuDevice::drawPath(const SkDraw& draw, const SkPath& origSrcPath,
                           const SkPaint& paint, const SkMatrix* prePathMatrix,
                           bool pathIsMutable) {
    CHECK_FOR_ANNOTATION(paint);
    CHECK_SHOULD_DRAW(draw, false);

    GrPaint grPaint;
    if (!skPaint2GrPaintShader(this, paint, true, &grPaint)) {
        return;
    }

    
    
    
    SkPath* pathPtr = const_cast<SkPath*>(&origSrcPath);
    SkTLazy<SkPath> tmpPath;
    SkTLazy<SkPath> effectPath;

    if (prePathMatrix) {
        SkPath* result = pathPtr;

        if (!pathIsMutable) {
            result = tmpPath.init();
            pathIsMutable = true;
        }
        
        
        pathPtr->transform(*prePathMatrix, result);
        pathPtr = result;
    }
    
    SkDEBUGCODE(prePathMatrix = (const SkMatrix*)0x50FF8001;)

    SkStrokeRec stroke(paint);
    SkPathEffect* pathEffect = paint.getPathEffect();
    const SkRect* cullRect = NULL;  
    if (pathEffect && pathEffect->filterPath(effectPath.init(), *pathPtr, &stroke,
                                             cullRect)) {
        pathPtr = effectPath.get();
        pathIsMutable = true;
    }

    if (paint.getMaskFilter()) {
        if (!stroke.isHairlineStyle()) {
            SkPath* strokedPath = pathIsMutable ? pathPtr : tmpPath.init();
            if (stroke.applyToPath(strokedPath, *pathPtr)) {
                pathPtr = strokedPath;
                pathIsMutable = true;
                stroke.setFillStyle();
            }
        }

        
        SkPath* devPathPtr = pathIsMutable ? pathPtr : tmpPath.init();

        
        pathPtr->transform(fContext->getMatrix(), devPathPtr);

        SkRect maskRect;
        if (paint.getMaskFilter()->canFilterMaskGPU(devPathPtr->getBounds(),
                                                    draw.fClip->getBounds(),
                                                    fContext->getMatrix(),
                                                    &maskRect)) {
            
            
            const SkMatrix ctm = fContext->getMatrix();

            SkIRect finalIRect;
            maskRect.roundOut(&finalIRect);
            if (draw.fClip->quickReject(finalIRect)) {
                
                return;
            }
            if (NULL != draw.fBounder && !draw.fBounder->doIRect(finalIRect)) {
                
                return;
            }

            if (paint.getMaskFilter()->directFilterMaskGPU(fContext, &grPaint,
                                                           stroke, *devPathPtr)) {
                
                
                return;
            }

            GrAutoScratchTexture mask;

            if (create_mask_GPU(fContext, maskRect, *devPathPtr, stroke,
                                grPaint.isAntiAlias(), &mask)) {
                GrTexture* filtered;

                if (paint.getMaskFilter()->filterMaskGPU(mask.texture(),
                                                         ctm, maskRect, &filtered, true)) {
                    
                    SkAutoTUnref<GrTexture> atu(filtered);

                    
                    
                    
                    if (filtered == mask.texture()) {
                        mask.detach();
                        filtered->unref(); 
                    }

                    if (draw_mask(fContext, maskRect, &grPaint, filtered)) {
                        
                        return;
                    }
                }
            }
        }

        
        
        SkPaint::Style style = stroke.isHairlineStyle() ? SkPaint::kStroke_Style :
                                                          SkPaint::kFill_Style;
        draw_with_mask_filter(fContext, *devPathPtr, paint.getMaskFilter(),
                              *draw.fClip, draw.fBounder, &grPaint, style);
        return;
    }

    fContext->drawPath(grPaint, *pathPtr, stroke);
}

static const int kBmpSmallTileSize = 1 << 10;

static inline int get_tile_count(const SkIRect& srcRect, int tileSize)  {
    int tilesX = (srcRect.fRight / tileSize) - (srcRect.fLeft / tileSize) + 1;
    int tilesY = (srcRect.fBottom / tileSize) - (srcRect.fTop / tileSize) + 1;
    return tilesX * tilesY;
}

static int determine_tile_size(const SkBitmap& bitmap, const SkIRect& src, int maxTileSize) {
    if (maxTileSize <= kBmpSmallTileSize) {
        return maxTileSize;
    }

    size_t maxTileTotalTileSize = get_tile_count(src, maxTileSize);
    size_t smallTotalTileSize = get_tile_count(src, kBmpSmallTileSize);

    maxTileTotalTileSize *= maxTileSize * maxTileSize;
    smallTotalTileSize *= kBmpSmallTileSize * kBmpSmallTileSize;

    if (maxTileTotalTileSize > 2 * smallTotalTileSize) {
        return kBmpSmallTileSize;
    } else {
        return maxTileSize;
    }
}



static void determine_clipped_src_rect(const GrContext* context,
                                       const SkBitmap& bitmap,
                                       const SkRect* srcRectPtr,
                                       SkIRect* clippedSrcIRect) {
    const GrClipData* clip = context->getClip();
    clip->getConservativeBounds(context->getRenderTarget(), clippedSrcIRect, NULL);
    SkMatrix inv;
    if (!context->getMatrix().invert(&inv)) {
        clippedSrcIRect->setEmpty();
        return;
    }
    SkRect clippedSrcRect = SkRect::Make(*clippedSrcIRect);
    inv.mapRect(&clippedSrcRect);
    if (NULL != srcRectPtr) {
        
        clippedSrcRect.offset(srcRectPtr->fLeft, srcRectPtr->fTop);
        if (!clippedSrcRect.intersect(*srcRectPtr)) {
            clippedSrcIRect->setEmpty();
            return;
        }
    }
    clippedSrcRect.roundOut(clippedSrcIRect);
    SkIRect bmpBounds = SkIRect::MakeWH(bitmap.width(), bitmap.height());
    if (!clippedSrcIRect->intersect(bmpBounds)) {
        clippedSrcIRect->setEmpty();
    }
}

bool SkGpuDevice::shouldTileBitmap(const SkBitmap& bitmap,
                                   const GrTextureParams& params,
                                   const SkRect* srcRectPtr,
                                   int maxTileSize,
                                   int* tileSize,
                                   SkIRect* clippedSrcRect) const {
    
    if (NULL != bitmap.getTexture()) {
        return false;
    }

    
    if (bitmap.width() > maxTileSize || bitmap.height() > maxTileSize) {
        determine_clipped_src_rect(fContext, bitmap, srcRectPtr, clippedSrcRect);
        *tileSize = determine_tile_size(bitmap, *clippedSrcRect, maxTileSize);
        return true;
    }

    if (bitmap.width() * bitmap.height() < 4 * kBmpSmallTileSize * kBmpSmallTileSize) {
        return false;
    }

    
    if (GrIsBitmapInCache(fContext, bitmap, &params)) {
        return false;
    }

    
    
    
    

    
    
    size_t bmpSize = bitmap.getSize();
    size_t cacheSize;
    fContext->getTextureCacheLimits(NULL, &cacheSize);
    if (bmpSize < cacheSize / 2) {
        return false;
    }

    
    determine_clipped_src_rect(fContext, bitmap, srcRectPtr, clippedSrcRect);
    *tileSize = kBmpSmallTileSize; 
    size_t usedTileBytes = get_tile_count(*clippedSrcRect, kBmpSmallTileSize) *
                           kBmpSmallTileSize * kBmpSmallTileSize;

    return usedTileBytes < 2 * bmpSize;
}

void SkGpuDevice::drawBitmap(const SkDraw& origDraw,
                             const SkBitmap& bitmap,
                             const SkMatrix& m,
                             const SkPaint& paint) {
    SkMatrix concat;
    SkTCopyOnFirstWrite<SkDraw> draw(origDraw);
    if (!m.isIdentity()) {
        concat.setConcat(*draw->fMatrix, m);
        draw.writable()->fMatrix = &concat;
    }
    this->drawBitmapCommon(*draw, bitmap, NULL, NULL, paint, SkCanvas::kNone_DrawBitmapRectFlag);
}




static inline void clamped_outset_with_offset(SkIRect* iRect,
                                              int outset,
                                              SkPoint* offset,
                                              const SkIRect& clamp) {
    iRect->outset(outset, outset);

    int leftClampDelta = clamp.fLeft - iRect->fLeft;
    if (leftClampDelta > 0) {
        offset->fX -= outset - leftClampDelta;
        iRect->fLeft = clamp.fLeft;
    } else {
        offset->fX -= outset;
    }

    int topClampDelta = clamp.fTop - iRect->fTop;
    if (topClampDelta > 0) {
        offset->fY -= outset - topClampDelta;
        iRect->fTop = clamp.fTop;
    } else {
        offset->fY -= outset;
    }

    if (iRect->fRight > clamp.fRight) {
        iRect->fRight = clamp.fRight;
    }
    if (iRect->fBottom > clamp.fBottom) {
        iRect->fBottom = clamp.fBottom;
    }
}

void SkGpuDevice::drawBitmapCommon(const SkDraw& draw,
                                   const SkBitmap& bitmap,
                                   const SkRect* srcRectPtr,
                                   const SkSize* dstSizePtr,
                                   const SkPaint& paint,
                                   SkCanvas::DrawBitmapRectFlags flags) {
    CHECK_SHOULD_DRAW(draw, false);

    SkRect srcRect;
    SkSize dstSize;
    
    
    if (NULL == srcRectPtr) {
        SkScalar w = SkIntToScalar(bitmap.width());
        SkScalar h = SkIntToScalar(bitmap.height());
        dstSize.fWidth = w;
        dstSize.fHeight = h;
        srcRect.set(0, 0, w, h);
        flags = (SkCanvas::DrawBitmapRectFlags) (flags | SkCanvas::kBleed_DrawBitmapRectFlag);
    } else {
        SkASSERT(NULL != dstSizePtr);
        srcRect = *srcRectPtr;
        dstSize = *dstSizePtr;
        if (srcRect.fLeft <= 0 && srcRect.fTop <= 0 &&
            srcRect.fRight >= bitmap.width() && srcRect.fBottom >= bitmap.height()) {
            flags = (SkCanvas::DrawBitmapRectFlags) (flags | SkCanvas::kBleed_DrawBitmapRectFlag);
        }
    }

    if (paint.getMaskFilter()){
        
        
        SkBitmap        tmp;    
        const SkBitmap* bitmapPtr = &bitmap;
        SkMatrix localM;
        if (NULL != srcRectPtr) {
            localM.setTranslate(-srcRectPtr->fLeft, -srcRectPtr->fTop);
            localM.postScale(dstSize.fWidth / srcRectPtr->width(),
                             dstSize.fHeight / srcRectPtr->height());
            
            
            
            
            if (!(SkCanvas::kBleed_DrawBitmapRectFlag & flags)) {
                SkIRect iSrc;
                srcRect.roundOut(&iSrc);

                SkPoint offset = SkPoint::Make(SkIntToScalar(iSrc.fLeft),
                                               SkIntToScalar(iSrc.fTop));

                if (!bitmap.extractSubset(&tmp, iSrc)) {
                    return;     
                }
                bitmapPtr = &tmp;
                srcRect.offset(-offset.fX, -offset.fY);

                
                localM.preTranslate(offset.fX, offset.fY);
            }
        } else {
            localM.reset();
        }

        SkPaint paintWithShader(paint);
        paintWithShader.setShader(SkShader::CreateBitmapShader(*bitmapPtr,
            SkShader::kClamp_TileMode, SkShader::kClamp_TileMode))->unref();
        paintWithShader.getShader()->setLocalMatrix(localM);
        SkRect dstRect = {0, 0, dstSize.fWidth, dstSize.fHeight};
        this->drawRect(draw, dstRect, paintWithShader);

        return;
    }

    
    
    SkMatrix m;
    m.setScale(dstSize.fWidth / srcRect.width(),
               dstSize.fHeight / srcRect.height());
    fContext->concatMatrix(m);

    GrTextureParams params;
    SkPaint::FilterLevel paintFilterLevel = paint.getFilterLevel();
    GrTextureParams::FilterMode textureFilterMode;

    int tileFilterPad;
    bool doBicubic = false;

    switch(paintFilterLevel) {
        case SkPaint::kNone_FilterLevel:
            tileFilterPad = 0;
            textureFilterMode = GrTextureParams::kNone_FilterMode;
            break;
        case SkPaint::kLow_FilterLevel:
            tileFilterPad = 1;
            textureFilterMode = GrTextureParams::kBilerp_FilterMode;
            break;
        case SkPaint::kMedium_FilterLevel:
            tileFilterPad = 1;
            if (fContext->getMatrix().getMinStretch() < SK_Scalar1) {
                textureFilterMode = GrTextureParams::kMipMap_FilterMode;
            } else {
                
                textureFilterMode = GrTextureParams::kBilerp_FilterMode;
            }
            break;
        case SkPaint::kHigh_FilterLevel:
            
            if (fContext->getMatrix().getMinStretch() >= SK_Scalar1) {
                
                textureFilterMode = GrTextureParams::kNone_FilterMode;
                tileFilterPad = GrBicubicEffect::kFilterTexelPad;
                doBicubic = true;
            } else {
                textureFilterMode = GrTextureParams::kMipMap_FilterMode;
                tileFilterPad = 1;
            }
            break;
        default:
            SkErrorInternals::SetError( kInvalidPaint_SkError,
                                        "Sorry, I don't understand the filtering "
                                        "mode you asked for.  Falling back to "
                                        "MIPMaps.");
            tileFilterPad = 1;
            textureFilterMode = GrTextureParams::kMipMap_FilterMode;
            break;
    }

    params.setFilterMode(textureFilterMode);

    int maxTileSize = fContext->getMaxTextureSize() - 2 * tileFilterPad;
    int tileSize;

    SkIRect clippedSrcRect;
    if (this->shouldTileBitmap(bitmap, params, srcRectPtr, maxTileSize, &tileSize,
                               &clippedSrcRect)) {
        this->drawTiledBitmap(bitmap, srcRect, clippedSrcRect, params, paint, flags, tileSize,
                              doBicubic);
    } else {
        
        this->internalDrawBitmap(bitmap, srcRect, params, paint, flags, doBicubic);
    }
}



void SkGpuDevice::drawTiledBitmap(const SkBitmap& bitmap,
                                  const SkRect& srcRect,
                                  const SkIRect& clippedSrcIRect,
                                  const GrTextureParams& params,
                                  const SkPaint& paint,
                                  SkCanvas::DrawBitmapRectFlags flags,
                                  int tileSize,
                                  bool bicubic) {
    SkRect clippedSrcRect = SkRect::Make(clippedSrcIRect);

    int nx = bitmap.width() / tileSize;
    int ny = bitmap.height() / tileSize;
    for (int x = 0; x <= nx; x++) {
        for (int y = 0; y <= ny; y++) {
            SkRect tileR;
            tileR.set(SkIntToScalar(x * tileSize),
                      SkIntToScalar(y * tileSize),
                      SkIntToScalar((x + 1) * tileSize),
                      SkIntToScalar((y + 1) * tileSize));

            if (!SkRect::Intersects(tileR, clippedSrcRect)) {
                continue;
            }

            if (!tileR.intersect(srcRect)) {
                continue;
            }

            SkBitmap tmpB;
            SkIRect iTileR;
            tileR.roundOut(&iTileR);
            SkPoint offset = SkPoint::Make(SkIntToScalar(iTileR.fLeft),
                                           SkIntToScalar(iTileR.fTop));

            
            SkMatrix tmpM;
            GrContext::AutoMatrix am;
            tmpM.setTranslate(offset.fX - srcRect.fLeft, offset.fY - srcRect.fTop);
            am.setPreConcat(fContext, tmpM);

            if (SkPaint::kNone_FilterLevel != paint.getFilterLevel() || bicubic) {
                SkIRect iClampRect;

                if (SkCanvas::kBleed_DrawBitmapRectFlag & flags) {
                    
                    
                    iClampRect = SkIRect::MakeWH(bitmap.width(), bitmap.height());
                } else {
                    
                    
                    
                    srcRect.roundOut(&iClampRect);
                }
                int outset = bicubic ? GrBicubicEffect::kFilterTexelPad : 1;
                clamped_outset_with_offset(&iTileR, outset, &offset, iClampRect);
            }

            if (bitmap.extractSubset(&tmpB, iTileR)) {
                
                tileR.offset(-offset.fX, -offset.fY);

                this->internalDrawBitmap(tmpB, tileR, params, paint, flags, bicubic);
            }
        }
    }
}

static bool has_aligned_samples(const SkRect& srcRect,
                                const SkRect& transformedRect) {
    
    if (SkScalarAbs(SkScalarRoundToScalar(transformedRect.left()) -
            transformedRect.left()) < COLOR_BLEED_TOLERANCE &&
        SkScalarAbs(SkScalarRoundToScalar(transformedRect.top()) -
            transformedRect.top()) < COLOR_BLEED_TOLERANCE &&
        SkScalarAbs(transformedRect.width() - srcRect.width()) <
            COLOR_BLEED_TOLERANCE &&
        SkScalarAbs(transformedRect.height() - srcRect.height()) <
            COLOR_BLEED_TOLERANCE) {
        return true;
    }
    return false;
}

static bool may_color_bleed(const SkRect& srcRect,
                            const SkRect& transformedRect,
                            const SkMatrix& m) {
    
    
    SkASSERT(!has_aligned_samples(srcRect, transformedRect));
    SkRect innerSrcRect(srcRect), innerTransformedRect,
        outerTransformedRect(transformedRect);
    innerSrcRect.inset(SK_ScalarHalf, SK_ScalarHalf);
    m.mapRect(&innerTransformedRect, innerSrcRect);

    
    
    
    
    outerTransformedRect.inset(COLOR_BLEED_TOLERANCE, COLOR_BLEED_TOLERANCE);
    innerTransformedRect.outset(COLOR_BLEED_TOLERANCE, COLOR_BLEED_TOLERANCE);
    SkIRect outer, inner;
    outerTransformedRect.round(&outer);
    innerTransformedRect.round(&inner);
    
    
    return inner != outer;
}









void SkGpuDevice::internalDrawBitmap(const SkBitmap& bitmap,
                                     const SkRect& srcRect,
                                     const GrTextureParams& params,
                                     const SkPaint& paint,
                                     SkCanvas::DrawBitmapRectFlags flags,
                                     bool bicubic) {
    SkASSERT(bitmap.width() <= fContext->getMaxTextureSize() &&
             bitmap.height() <= fContext->getMaxTextureSize());

    GrTexture* texture;
    SkAutoCachedTexture act(this, bitmap, &params, &texture);
    if (NULL == texture) {
        return;
    }

    SkRect dstRect = {0, 0, srcRect.width(), srcRect.height() };
    SkRect paintRect;
    SkScalar wInv = SkScalarInvert(SkIntToScalar(texture->width()));
    SkScalar hInv = SkScalarInvert(SkIntToScalar(texture->height()));
    paintRect.setLTRB(SkScalarMul(srcRect.fLeft,   wInv),
                      SkScalarMul(srcRect.fTop,    hInv),
                      SkScalarMul(srcRect.fRight,  wInv),
                      SkScalarMul(srcRect.fBottom, hInv));

    bool needsTextureDomain = false;
    if (!(flags & SkCanvas::kBleed_DrawBitmapRectFlag) &&
        (bicubic || params.filterMode() != GrTextureParams::kNone_FilterMode)) {
        
        needsTextureDomain = srcRect.width() < bitmap.width() ||
                             srcRect.height() < bitmap.height();
        if (!bicubic && needsTextureDomain && fContext->getMatrix().rectStaysRect()) {
            const SkMatrix& matrix = fContext->getMatrix();
            
            SkRect transformedRect;
            matrix.mapRect(&transformedRect, srcRect);

            if (has_aligned_samples(srcRect, transformedRect)) {
                
                
                needsTextureDomain = false;
            } else {
                needsTextureDomain = may_color_bleed(srcRect, transformedRect, matrix);
            }
        }
    }

    SkRect textureDomain = SkRect::MakeEmpty();
    SkAutoTUnref<GrEffectRef> effect;
    if (needsTextureDomain) {
        
        SkScalar left, top, right, bottom;
        if (srcRect.width() > SK_Scalar1) {
            SkScalar border = SK_ScalarHalf / texture->width();
            left = paintRect.left() + border;
            right = paintRect.right() - border;
        } else {
            left = right = SkScalarHalf(paintRect.left() + paintRect.right());
        }
        if (srcRect.height() > SK_Scalar1) {
            SkScalar border = SK_ScalarHalf / texture->height();
            top = paintRect.top() + border;
            bottom = paintRect.bottom() - border;
        } else {
            top = bottom = SkScalarHalf(paintRect.top() + paintRect.bottom());
        }
        textureDomain.setLTRB(left, top, right, bottom);
        if (bicubic) {
            effect.reset(GrBicubicEffect::Create(texture, SkMatrix::I(), textureDomain));
        } else {
            effect.reset(GrTextureDomainEffect::Create(texture,
                                                       SkMatrix::I(),
                                                       textureDomain,
                                                       GrTextureDomain::kClamp_Mode,
                                                       params.filterMode()));
        }
    } else if (bicubic) {
        SkASSERT(GrTextureParams::kNone_FilterMode == params.filterMode());
        SkShader::TileMode tileModes[2] = { params.getTileModeX(), params.getTileModeY() };
        effect.reset(GrBicubicEffect::Create(texture, SkMatrix::I(), tileModes));
    } else {
        effect.reset(GrSimpleTextureEffect::Create(texture, SkMatrix::I(), params));
    }

    
    
    GrPaint grPaint;
    grPaint.addColorEffect(effect);
    bool alphaOnly = !(SkBitmap::kA8_Config == bitmap.config());
    if (!skPaint2GrPaintNoShader(this, paint, alphaOnly, false, &grPaint)) {
        return;
    }

    fContext->drawRectToRect(grPaint, dstRect, paintRect, NULL);
}

static bool filter_texture(SkBaseDevice* device, GrContext* context,
                           GrTexture* texture, const SkImageFilter* filter,
                           int w, int h, const SkImageFilter::Context& ctx,
                           SkBitmap* result, SkIPoint* offset) {
    SkASSERT(filter);
    SkDeviceImageFilterProxy proxy(device);

    if (filter->canFilterImageGPU()) {
        
        
        GrContext::AutoWideOpenIdentityDraw awo(context, NULL);
        return filter->filterImageGPU(&proxy, wrap_texture(texture), ctx, result, offset);
    } else {
        return false;
    }
}

void SkGpuDevice::drawSprite(const SkDraw& draw, const SkBitmap& bitmap,
                             int left, int top, const SkPaint& paint) {
    
    CHECK_SHOULD_DRAW(draw, true);

    SkAutoLockPixels alp(bitmap, !bitmap.getTexture());
    if (!bitmap.getTexture() && !bitmap.readyToDraw()) {
        return;
    }

    int w = bitmap.width();
    int h = bitmap.height();

    GrTexture* texture;
    
    SkAutoCachedTexture act(this, bitmap, NULL, &texture);

    SkImageFilter* filter = paint.getImageFilter();
    
    SkBitmap filteredBitmap;

    if (NULL != filter) {
        SkIPoint offset = SkIPoint::Make(0, 0);
        SkMatrix matrix(*draw.fMatrix);
        matrix.postTranslate(SkIntToScalar(-left), SkIntToScalar(-top));
        SkIRect clipBounds = SkIRect::MakeWH(bitmap.width(), bitmap.height());
        SkImageFilter::Context ctx(matrix, clipBounds);
        if (filter_texture(this, fContext, texture, filter, w, h, ctx, &filteredBitmap,
                           &offset)) {
            texture = (GrTexture*) filteredBitmap.getTexture();
            w = filteredBitmap.width();
            h = filteredBitmap.height();
            left += offset.x();
            top += offset.y();
        } else {
            return;
        }
    }

    GrPaint grPaint;
    grPaint.addColorTextureEffect(texture, SkMatrix::I());

    if(!skPaint2GrPaintNoShader(this, paint, true, false, &grPaint)) {
        return;
    }

    fContext->drawRectToRect(grPaint,
                             SkRect::MakeXYWH(SkIntToScalar(left),
                                              SkIntToScalar(top),
                                              SkIntToScalar(w),
                                              SkIntToScalar(h)),
                             SkRect::MakeXYWH(0,
                                              0,
                                              SK_Scalar1 * w / texture->width(),
                                              SK_Scalar1 * h / texture->height()));
}

void SkGpuDevice::drawBitmapRect(const SkDraw& origDraw, const SkBitmap& bitmap,
                                 const SkRect* src, const SkRect& dst,
                                 const SkPaint& paint,
                                 SkCanvas::DrawBitmapRectFlags flags) {
    SkMatrix    matrix;
    SkRect      bitmapBounds, tmpSrc;

    bitmapBounds.set(0, 0,
                     SkIntToScalar(bitmap.width()),
                     SkIntToScalar(bitmap.height()));

    
    if (NULL != src) {
        tmpSrc = *src;
    } else {
        tmpSrc = bitmapBounds;
    }

    matrix.setRectToRect(tmpSrc, dst, SkMatrix::kFill_ScaleToFit);

    
    if (NULL != src) {
        if (!bitmapBounds.contains(tmpSrc)) {
            if (!tmpSrc.intersect(bitmapBounds)) {
                return; 
            }
        }
    }

    SkRect tmpDst;
    matrix.mapRect(&tmpDst, tmpSrc);

    SkTCopyOnFirstWrite<SkDraw> draw(origDraw);
    if (0 != tmpDst.fLeft || 0 != tmpDst.fTop) {
        
        matrix = *origDraw.fMatrix;
        matrix.preTranslate(tmpDst.fLeft, tmpDst.fTop);
        draw.writable()->fMatrix = &matrix;
    }
    SkSize dstSize;
    dstSize.fWidth = tmpDst.width();
    dstSize.fHeight = tmpDst.height();

    this->drawBitmapCommon(*draw, bitmap, &tmpSrc, &dstSize, paint, flags);
}

void SkGpuDevice::drawDevice(const SkDraw& draw, SkBaseDevice* device,
                             int x, int y, const SkPaint& paint) {
    
    SkGpuDevice* dev = static_cast<SkGpuDevice*>(device);
    if (dev->fNeedClear) {
        
        dev->clear(0x0);
    }

    
    CHECK_SHOULD_DRAW(draw, true);

    GrRenderTarget* devRT = dev->accessRenderTarget();
    GrTexture* devTex;
    if (NULL == (devTex = devRT->asTexture())) {
        return;
    }

    const SkBitmap& bm = dev->accessBitmap(false);
    int w = bm.width();
    int h = bm.height();

    SkImageFilter* filter = paint.getImageFilter();
    
    SkBitmap filteredBitmap;

    if (NULL != filter) {
        SkIPoint offset = SkIPoint::Make(0, 0);
        SkMatrix matrix(*draw.fMatrix);
        matrix.postTranslate(SkIntToScalar(-x), SkIntToScalar(-y));
        SkIRect clipBounds = SkIRect::MakeWH(devTex->width(), devTex->height());
        SkImageFilter::Context ctx(matrix, clipBounds);
        if (filter_texture(this, fContext, devTex, filter, w, h, ctx, &filteredBitmap,
                           &offset)) {
            devTex = filteredBitmap.getTexture();
            w = filteredBitmap.width();
            h = filteredBitmap.height();
            x += offset.fX;
            y += offset.fY;
        } else {
            return;
        }
    }

    GrPaint grPaint;
    grPaint.addColorTextureEffect(devTex, SkMatrix::I());

    if (!skPaint2GrPaintNoShader(this, paint, true, false, &grPaint)) {
        return;
    }

    SkRect dstRect = SkRect::MakeXYWH(SkIntToScalar(x),
                                      SkIntToScalar(y),
                                      SkIntToScalar(w),
                                      SkIntToScalar(h));

    
    
    SkRect srcRect = SkRect::MakeWH(SK_Scalar1 * w / devTex->width(),
                                    SK_Scalar1 * h / devTex->height());

    fContext->drawRectToRect(grPaint, dstRect, srcRect);
}

bool SkGpuDevice::canHandleImageFilter(const SkImageFilter* filter) {
    return filter->canFilterImageGPU();
}

bool SkGpuDevice::filterImage(const SkImageFilter* filter, const SkBitmap& src,
                              const SkImageFilter::Context& ctx,
                              SkBitmap* result, SkIPoint* offset) {
    
    if (!this->SkGpuDevice::canHandleImageFilter(filter)) {
        return false;
    }

    SkAutoLockPixels alp(src, !src.getTexture());
    if (!src.getTexture() && !src.readyToDraw()) {
        return false;
    }

    GrTexture* texture;
    
    
    SkAutoCachedTexture act(this, src, NULL, &texture);

    return filter_texture(this, fContext, texture, filter, src.width(), src.height(), ctx,
                          result, offset);
}




static const GrPrimitiveType gVertexMode2PrimitiveType[] = {
    kTriangles_GrPrimitiveType,
    kTriangleStrip_GrPrimitiveType,
    kTriangleFan_GrPrimitiveType,
};

void SkGpuDevice::drawVertices(const SkDraw& draw, SkCanvas::VertexMode vmode,
                              int vertexCount, const SkPoint vertices[],
                              const SkPoint texs[], const SkColor colors[],
                              SkXfermode* xmode,
                              const uint16_t indices[], int indexCount,
                              const SkPaint& paint) {
    CHECK_SHOULD_DRAW(draw, false);

    GrPaint grPaint;
    
    if (NULL == texs) {
        if (!skPaint2GrPaintNoShader(this, paint, false, NULL == colors, &grPaint)) {
            return;
        }
    } else {
        if (!skPaint2GrPaintShader(this, paint, NULL == colors, &grPaint)) {
            return;
        }
    }

    if (NULL != xmode && NULL != texs && NULL != colors) {
        if (!SkXfermode::IsMode(xmode, SkXfermode::kModulate_Mode)) {
            SkDebugf("Unsupported vertex-color/texture xfer mode.\n");
#if 0
            return
#endif
        }
    }

    SkAutoSTMalloc<128, GrColor> convertedColors(0);
    if (NULL != colors) {
        
        convertedColors.reset(vertexCount);
        for (int i = 0; i < vertexCount; ++i) {
            convertedColors[i] = SkColor2GrColor(colors[i]);
        }
        colors = convertedColors.get();
    }
    fContext->drawVertices(grPaint,
                           gVertexMode2PrimitiveType[vmode],
                           vertexCount,
                           (GrPoint*) vertices,
                           (GrPoint*) texs,
                           colors,
                           indices,
                           indexCount);
}



void SkGpuDevice::drawText(const SkDraw& draw, const void* text,
                          size_t byteLength, SkScalar x, SkScalar y,
                          const SkPaint& paint) {
    CHECK_SHOULD_DRAW(draw, false);

    if (fMainTextContext->canDraw(paint)) {
        GrPaint grPaint;
        if (!skPaint2GrPaintShader(this, paint, true, &grPaint)) {
            return;
        }

        SkDEBUGCODE(this->validate();)

        fMainTextContext->drawText(grPaint, paint, (const char *)text, byteLength, x, y);
    } else if (fFallbackTextContext && fFallbackTextContext->canDraw(paint)) {
        GrPaint grPaint;
        if (!skPaint2GrPaintShader(this, paint, true, &grPaint)) {
            return;
        }

        SkDEBUGCODE(this->validate();)

        fFallbackTextContext->drawText(grPaint, paint, (const char *)text, byteLength, x, y);
    } else {
        
        draw.drawText_asPaths((const char*)text, byteLength, x, y, paint);
    }
}

void SkGpuDevice::drawPosText(const SkDraw& draw, const void* text,
                             size_t byteLength, const SkScalar pos[],
                             SkScalar constY, int scalarsPerPos,
                             const SkPaint& paint) {
    CHECK_SHOULD_DRAW(draw, false);

    if (fMainTextContext->canDraw(paint)) {
        GrPaint grPaint;
        if (!skPaint2GrPaintShader(this, paint, true, &grPaint)) {
            return;
        }

        SkDEBUGCODE(this->validate();)

        fMainTextContext->drawPosText(grPaint, paint, (const char *)text, byteLength, pos,
                                      constY, scalarsPerPos);
    } else if (fFallbackTextContext && fFallbackTextContext->canDraw(paint)) {
        GrPaint grPaint;
        if (!skPaint2GrPaintShader(this, paint, true, &grPaint)) {
            return;
        }

        SkDEBUGCODE(this->validate();)

        fFallbackTextContext->drawPosText(grPaint, paint, (const char *)text, byteLength, pos,
                                          constY, scalarsPerPos);
    } else {
        draw.drawPosText_asPaths((const char*)text, byteLength, pos, constY,
                                 scalarsPerPos, paint);
    }
}

void SkGpuDevice::drawTextOnPath(const SkDraw& draw, const void* text,
                                size_t len, const SkPath& path,
                                const SkMatrix* m, const SkPaint& paint) {
    CHECK_SHOULD_DRAW(draw, false);

    SkASSERT(draw.fDevice == this);
    draw.drawTextOnPath((const char*)text, len, path, m, paint);
}



bool SkGpuDevice::filterTextFlags(const SkPaint& paint, TextFlags* flags) {
    if (!paint.isLCDRenderText()) {
        
        return false;
    }

    if (paint.getShader() ||
        paint.getXfermode() || 
        paint.getMaskFilter() ||
        paint.getRasterizer() ||
        paint.getColorFilter() ||
        paint.getPathEffect() ||
        paint.isFakeBoldText() ||
        paint.getStyle() != SkPaint::kFill_Style) {
        
        flags->fFlags = paint.getFlags() & ~SkPaint::kLCDRenderText_Flag;
        flags->fHinting = paint.getHinting();
        return true;
    }
    
    return false;
}

void SkGpuDevice::flush() {
    DO_DEFERRED_CLEAR();
    fContext->resolveRenderTarget(fRenderTarget);
}



SkBaseDevice* SkGpuDevice::onCreateDevice(const SkImageInfo& info, Usage usage) {
    GrTextureDesc desc;
    desc.fConfig = fRenderTarget->config();
    desc.fFlags = kRenderTarget_GrTextureFlagBit;
    desc.fWidth = info.width();
    desc.fHeight = info.height();
    desc.fSampleCnt = fRenderTarget->numSamples();

    SkAutoTUnref<GrTexture> texture;
    
    bool needClear = !info.isOpaque();

#if CACHE_COMPATIBLE_DEVICE_TEXTURES
    
    
    const GrContext::ScratchTexMatch match = (kSaveLayer_Usage == usage) ?
                                                GrContext::kApprox_ScratchTexMatch :
                                                GrContext::kExact_ScratchTexMatch;
    texture.reset(fContext->lockAndRefScratchTexture(desc, match));
#else
    texture.reset(fContext->createUncachedTexture(desc, NULL, 0));
#endif
    if (NULL != texture.get()) {
        return SkNEW_ARGS(SkGpuDevice,(fContext, texture, needClear));
    } else {
        GrPrintf("---- failed to create compatible device texture [%d %d]\n",
                 info.width(), info.height());
        return NULL;
    }
}

SkSurface* SkGpuDevice::newSurface(const SkImageInfo& info) {
    return SkSurface::NewRenderTarget(fContext, info, fRenderTarget->numSamples());
}

SkGpuDevice::SkGpuDevice(GrContext* context,
                         GrTexture* texture,
                         bool needClear)
    : SkBitmapDevice(make_bitmap(context, texture->asRenderTarget())) {

    SkASSERT(texture && texture->asRenderTarget());
    
    
    this->initFromRenderTarget(context, texture->asRenderTarget(), true);
    fNeedClear = needClear;
}

class GPUAccelData : public SkPicture::AccelData {
public:
    GPUAccelData(Key key) : INHERITED(key) { }

protected:

private:
    typedef SkPicture::AccelData INHERITED;
};



SkPicture::AccelData::Key SkGpuDevice::ComputeAccelDataKey() {
    static const SkPicture::AccelData::Key gGPUID = SkPicture::AccelData::GenerateDomain();

    return gGPUID;
}

void SkGpuDevice::EXPERIMENTAL_optimize(SkPicture* picture) {
    SkPicture::AccelData::Key key = ComputeAccelDataKey();

    GPUAccelData* data = SkNEW_ARGS(GPUAccelData, (key));

    picture->EXPERIMENTAL_addAccelData(data);
}

bool SkGpuDevice::EXPERIMENTAL_drawPicture(const SkPicture& picture) {
    SkPicture::AccelData::Key key = ComputeAccelDataKey();

    const SkPicture::AccelData* data = picture.EXPERIMENTAL_getAccelData(key);
    if (NULL == data) {
        return false;
    }

#if 0
    const GPUAccelData *gpuData = static_cast<const GPUAccelData*>(data);
#endif

    return false;
}
