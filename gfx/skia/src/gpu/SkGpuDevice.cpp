






#include "SkGpuDevice.h"

#include "effects/GrTextureDomainEffect.h"
#include "effects/GrSimpleTextureEffect.h"

#include "GrContext.h"
#include "GrTextContext.h"

#include "SkGrTexturePixelRef.h"

#include "SkColorFilter.h"
#include "SkDeviceImageFilterProxy.h"
#include "SkDrawProcs.h"
#include "SkGlyphCache.h"
#include "SkImageFilter.h"
#include "SkPathEffect.h"
#include "SkStroke.h"
#include "SkUtils.h"

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



enum {
    kShaderEffectIdx = 0,
    kBitmapEffectIdx = 0,
    kColorFilterEffectIdx = 1,
    kXfermodeEffectIdx = 2,
};

#define MAX_BLUR_SIGMA 4.0f


#define MAX_BLUR_RADIUS SkIntToScalar(128)







#define BLUR_SIGMA_SCALE 0.6f



#define COLOR_BLEED_TOLERANCE SkFloatToScalar(0.001f)

#define DO_DEFERRED_CLEAR()             \
    do {                                \
        if (fNeedClear) {               \
            this->clear(SK_ColorTRANSPARENT); \
        }                               \
    } while (false)                     \



#define CHECK_FOR_NODRAW_ANNOTATION(paint) \
    do { if (paint.isNoDrawAnnotation()) { return; } } while (0)




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
        GrAssert(NULL != texture);
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
    GrPixelConfig config = renderTarget->config();

    bool isOpaque;
    SkBitmap bitmap;
    bitmap.setConfig(grConfig2skConfig(config, &isOpaque),
                     renderTarget->width(), renderTarget->height());
    bitmap.setIsOpaque(isOpaque);
    return bitmap;
}

SkGpuDevice* SkGpuDevice::Create(GrSurface* surface) {
    GrAssert(NULL != surface);
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
: SkDevice(make_bitmap(context, texture->asRenderTarget())) {
    this->initFromRenderTarget(context, texture->asRenderTarget(), false);
}

SkGpuDevice::SkGpuDevice(GrContext* context, GrRenderTarget* renderTarget)
: SkDevice(make_bitmap(context, renderTarget)) {
    this->initFromRenderTarget(context, renderTarget, false);
}

void SkGpuDevice::initFromRenderTarget(GrContext* context,
                                       GrRenderTarget* renderTarget,
                                       bool cached) {
    fDrawProcs = NULL;

    fContext = context;
    fContext->ref();

    fRenderTarget = NULL;
    fNeedClear = false;

    GrAssert(NULL != renderTarget);
    fRenderTarget = renderTarget;
    fRenderTarget->ref();

    
    
    
    
    GrSurface* surface = fRenderTarget->asTexture();
    if (NULL == surface) {
        surface = fRenderTarget;
    }
    SkPixelRef* pr = SkNEW_ARGS(SkGrPixelRef, (surface, cached));

    this->setPixelRef(pr, 0)->unref();
}

SkGpuDevice::SkGpuDevice(GrContext* context,
                         SkBitmap::Config config,
                         int width,
                         int height,
                         int sampleCount)
    : SkDevice(config, width, height, false ) {

    fDrawProcs = NULL;

    fContext = context;
    fContext->ref();

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

    SkAutoTUnref<GrTexture> texture(fContext->createUncachedTexture(desc, NULL, 0));

    if (NULL != texture) {
        fRenderTarget = texture->asRenderTarget();
        fRenderTarget->ref();

        GrAssert(NULL != fRenderTarget);

        
        SkGrPixelRef* pr = SkNEW_ARGS(SkGrPixelRef, (texture));
        this->setPixelRef(pr, 0)->unref();
    } else {
        GrPrintf("--- failed to create gpu-offscreen [%d %d]\n",
                 width, height);
        GrAssert(false);
    }
}

SkGpuDevice::~SkGpuDevice() {
    if (fDrawProcs) {
        delete fDrawProcs;
    }

    
    
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

namespace {
void purgeClipCB(int genID, void* ) {

    if (SkClipStack::kInvalidGenID == genID ||
        SkClipStack::kEmptyGenID == genID ||
        SkClipStack::kWideOpenGenID == genID) {
        
        return;
    }

}
};

void SkGpuDevice::onAttachToCanvas(SkCanvas* canvas) {
    INHERITED::onAttachToCanvas(canvas);

    
    fClipData.fClipStack = canvas->getClipStack();

    fClipData.fClipStack->addPurgeClipCallback(purgeClipCB, fContext);
}

void SkGpuDevice::onDetachFromCanvas() {
    INHERITED::onDetachFromCanvas();

    
    fClipData.fClipStack->removePurgeClipCallback(purgeClipCB, fContext);

    fClipData.fClipStack = NULL;
}

#ifdef SK_DEBUG
static void check_bounds(const GrClipData& clipData,
                         const SkRegion& clipRegion,
                         int renderTargetWidth,
                         int renderTargetHeight) {

    SkIRect devBound;

    devBound.setLTRB(0, 0, renderTargetWidth, renderTargetHeight);

    SkClipStack::BoundsType boundType;
    SkRect canvTemp;

    clipData.fClipStack->getBounds(&canvTemp, &boundType);
    if (SkClipStack::kNormal_BoundsType == boundType) {
        SkIRect devTemp;

        canvTemp.roundOut(&devTemp);

        devTemp.offset(-clipData.fOrigin.fX, -clipData.fOrigin.fY);

        if (!devBound.intersect(devTemp)) {
            devBound.setEmpty();
        }
    }

    GrAssert(devBound.contains(clipRegion.getBounds()));
}
#endif





void SkGpuDevice::prepareDraw(const SkDraw& draw, bool forceIdentity) {
    GrAssert(NULL != fClipData.fClipStack);

    fContext->setRenderTarget(fRenderTarget);

    SkASSERT(draw.fClipStack && draw.fClipStack == fClipData.fClipStack);

    if (forceIdentity) {
        fContext->setIdentityMatrix();
    } else {
        fContext->setMatrix(*draw.fMatrix);
    }
    fClipData.fOrigin = this->getOrigin();

#ifdef SK_DEBUG
    check_bounds(fClipData, *draw.fClip, fRenderTarget->width(), fRenderTarget->height());
#endif

    fContext->setClip(&fClipData);

    DO_DEFERRED_CLEAR();
}

SkGpuRenderTarget* SkGpuDevice::accessRenderTarget() {
    DO_DEFERRED_CLEAR();
    return (SkGpuRenderTarget*)fRenderTarget;
}

bool SkGpuDevice::bindDeviceAsTexture(GrPaint* paint) {
    GrTexture* texture = fRenderTarget->asTexture();
    if (NULL != texture) {
        paint->colorStage(kBitmapEffectIdx)->setEffect(
            GrSimpleTextureEffect::Create(texture, SkMatrix::I()))->unref();
        return true;
    }
    return false;
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
    if (SkXfermode::AsNewEffectOrCoeff(mode, dev->context(), &xferEffect, &sm, &dm)) {
        if (NULL != xferEffect) {
            grPaint->colorStage(kXfermodeEffectIdx)->setEffect(xferEffect)->unref();
            
            
            sm = SkXfermode::kOne_Coeff;
            dm = SkXfermode::kISA_Coeff;
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
        
        
        GrAssert(!constantColor);
    } else {
        grPaint->setColor(SkColor2GrColor(skPaint.getColor()));
        GrAssert(!grPaint->isColorStageEnabled(kShaderEffectIdx));
    }

    SkColorFilter* colorFilter = skPaint.getColorFilter();
    if (NULL != colorFilter) {
        
        
        if (constantColor) {
            SkColor filtered = colorFilter->filterColor(skPaint.getColor());
            grPaint->setColor(SkColor2GrColor(filtered));
        } else {
            SkAutoTUnref<GrEffectRef> effect(colorFilter->asNewEffect(dev->context()));
            if (NULL != effect.get()) {
                grPaint->colorStage(kColorFilterEffectIdx)->setEffect(effect);
            } else {
                
                SkColor color;
                SkXfermode::Mode filterMode;
                if (colorFilter->asColorMode(&color, &filterMode)) {
                    grPaint->setXfermodeColorFilter(filterMode, SkColor2GrColor(color));
                }
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
    } else if (!skPaint2GrPaintNoShader(dev, skPaint, true, false, grPaint)) {
        return false;
    }

    SkAutoTUnref<GrEffectRef> effect(shader->asNewEffect(dev->context(), skPaint));
    if (NULL != effect.get()) {
        grPaint->colorStage(kShaderEffectIdx)->setEffect(effect);
        return true;
    }

    
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
    }
    return false;
}
}


void SkGpuDevice::clear(SkColor color) {
    SkIRect rect = SkIRect::MakeWH(this->width(), this->height());
    fContext->clear(&rect, SkColor2GrColor(color), fRenderTarget);
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
    CHECK_FOR_NODRAW_ANNOTATION(paint);
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
                           count,
                           (GrPoint*)pts,
                           NULL,
                           NULL,
                           NULL,
                           0);
}



void SkGpuDevice::drawRect(const SkDraw& draw, const SkRect& rect,
                           const SkPaint& paint) {
    CHECK_FOR_NODRAW_ANNOTATION(paint);
    CHECK_SHOULD_DRAW(draw, false);

    bool doStroke = paint.getStyle() != SkPaint::kFill_Style;
    SkScalar width = paint.getStrokeWidth();

    



    bool usePath = doStroke && width > 0 &&
                    paint.getStrokeJoin() != SkPaint::kMiter_Join;
    
    if (paint.getMaskFilter() || paint.getPathEffect()) {
        usePath = true;
    }
    
    if (!usePath && paint.isAntiAlias() && !fContext->getMatrix().rectStaysRect()) {
        usePath = true;
    }
    
    if (SkPaint::kMiter_Join == paint.getStrokeJoin() &&
        paint.getStrokeMiter() < SK_ScalarSqrt2)
    {
        usePath = true;
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
    fContext->drawRect(grPaint, rect, doStroke ? width : -1);
}



void SkGpuDevice::drawOval(const SkDraw& draw, const SkRect& oval,
                           const SkPaint& paint) {
    CHECK_FOR_NODRAW_ANNOTATION(paint);
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


#define MIN_GPU_BLUR_SIZE SkIntToScalar(64)
#define MIN_GPU_BLUR_RADIUS SkIntToScalar(32)
inline bool shouldDrawBlurWithCPU(const SkRect& rect, SkScalar radius) {
    if (rect.width() <= MIN_GPU_BLUR_SIZE &&
        rect.height() <= MIN_GPU_BLUR_SIZE &&
        radius <= MIN_GPU_BLUR_RADIUS) {
        return true;
    }
    return false;
}

bool drawWithGPUMaskFilter(GrContext* context, const SkPath& devPath, const SkStrokeRec& stroke,
                           SkMaskFilter* filter, const SkRegion& clip,
                           SkBounder* bounder, GrPaint* grp) {
    SkMaskFilter::BlurInfo info;
    SkMaskFilter::BlurType blurType = filter->asABlur(&info);
    if (SkMaskFilter::kNone_BlurType == blurType) {
        return false;
    }
    SkScalar radius = info.fIgnoreTransform ? info.fRadius
                                            : context->getMatrix().mapRadius(info.fRadius);
    radius = SkMinScalar(radius, MAX_BLUR_RADIUS);
    if (radius <= 0) {
        return false;
    }

    SkRect srcRect = devPath.getBounds();
    if (shouldDrawBlurWithCPU(srcRect, radius)) {
        return false;
    }

    float sigma = SkScalarToFloat(radius) * BLUR_SIGMA_SCALE;
    float sigma3 = sigma * 3.0f;

    SkRect clipRect;
    clipRect.set(clip.getBounds());

    
    srcRect.inset(SkFloatToScalar(-sigma3), SkFloatToScalar(-sigma3));
    clipRect.inset(SkFloatToScalar(-sigma3), SkFloatToScalar(-sigma3));
    srcRect.intersect(clipRect);
    SkRect finalRect = srcRect;
    SkIRect finalIRect;
    finalRect.roundOut(&finalIRect);
    if (clip.quickReject(finalIRect)) {
        return true;
    }
    if (bounder && !bounder->doIRect(finalIRect)) {
        return true;
    }
    GrPoint offset = GrPoint::Make(-srcRect.fLeft, -srcRect.fTop);
    srcRect.offset(offset);
    GrTextureDesc desc;
    desc.fFlags = kRenderTarget_GrTextureFlagBit;
    desc.fWidth = SkScalarCeilToInt(srcRect.width());
    desc.fHeight = SkScalarCeilToInt(srcRect.height());
    
    
    desc.fConfig = kRGBA_8888_GrPixelConfig;

    if (context->isConfigRenderable(kAlpha_8_GrPixelConfig)) {
        desc.fConfig = kAlpha_8_GrPixelConfig;
    }

    GrAutoScratchTexture pathEntry(context, desc);
    GrTexture* pathTexture = pathEntry.texture();
    if (NULL == pathTexture) {
        return false;
    }

    SkAutoTUnref<GrTexture> blurTexture;

    {
        GrContext::AutoRenderTarget art(context, pathTexture->asRenderTarget());
        GrContext::AutoClip ac(context, srcRect);

        context->clear(NULL, 0);

        GrPaint tempPaint;
        if (grp->isAntiAlias()) {
            tempPaint.setAntiAlias(true);
            
            
            
            
            
            
            tempPaint.setBlendFunc(kOne_GrBlendCoeff, kISC_GrBlendCoeff);
        }

        GrContext::AutoMatrix am;

        
        SkMatrix translate;
        translate.setTranslate(offset.fX, offset.fY);
        am.set(context, translate);
        context->drawPath(tempPaint, devPath, stroke);

        
        
        bool isNormalBlur = blurType == SkMaskFilter::kNormal_BlurType;
        blurTexture.reset(context->gaussianBlur(pathTexture, isNormalBlur,
                                                srcRect, sigma, sigma));
        if (NULL == blurTexture) {
            return false;
        }

        if (!isNormalBlur) {
            context->setIdentityMatrix();
            GrPaint paint;
            SkMatrix matrix;
            matrix.setIDiv(pathTexture->width(), pathTexture->height());
            
            context->setRenderTarget(blurTexture->asRenderTarget());
            paint.colorStage(0)->setEffect(
                GrSimpleTextureEffect::Create(pathTexture, matrix))->unref();
            if (SkMaskFilter::kInner_BlurType == blurType) {
                
                paint.setBlendFunc(kDC_GrBlendCoeff, kZero_GrBlendCoeff);
            } else if (SkMaskFilter::kSolid_BlurType == blurType) {
                
                
                paint.setBlendFunc(kIDC_GrBlendCoeff, kOne_GrBlendCoeff);
            } else if (SkMaskFilter::kOuter_BlurType == blurType) {
                
                
                paint.setBlendFunc(kZero_GrBlendCoeff, kISC_GrBlendCoeff);
            }
            context->drawRect(paint, srcRect);
        }
    }

    GrContext::AutoMatrix am;
    if (!am.setIdentity(context, grp)) {
        return false;
    }

    static const int MASK_IDX = GrPaint::kMaxCoverageStages - 1;
    
    GrAssert(!grp->isCoverageStageEnabled(MASK_IDX));

    SkMatrix matrix;
    matrix.setTranslate(-finalRect.fLeft, -finalRect.fTop);
    matrix.postIDiv(blurTexture->width(), blurTexture->height());

    grp->coverageStage(MASK_IDX)->reset();
    grp->coverageStage(MASK_IDX)->setEffect(
        GrSimpleTextureEffect::Create(blurTexture, matrix))->unref();
    context->drawRect(*grp, finalRect);
    return true;
}

bool drawWithMaskFilter(GrContext* context, const SkPath& devPath,
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

    
    
    GrContext::AutoMatrix am;
    am.setIdentity(context, grp);

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

    static const int MASK_IDX = GrPaint::kMaxCoverageStages - 1;
    
    GrAssert(!grp->isCoverageStageEnabled(MASK_IDX));

    SkMatrix m;
    m.setTranslate(-dstM.fBounds.fLeft*SK_Scalar1, -dstM.fBounds.fTop*SK_Scalar1);
    m.postIDiv(texture->width(), texture->height());

    grp->coverageStage(MASK_IDX)->setEffect(GrSimpleTextureEffect::Create(texture, m))->unref();
    GrRect d;
    d.setLTRB(SkIntToScalar(dstM.fBounds.fLeft),
              SkIntToScalar(dstM.fBounds.fTop),
              SkIntToScalar(dstM.fBounds.fRight),
              SkIntToScalar(dstM.fBounds.fBottom));

    context->drawRect(*grp, d);
    return true;
}

}



void SkGpuDevice::drawPath(const SkDraw& draw, const SkPath& origSrcPath,
                           const SkPaint& paint, const SkMatrix* prePathMatrix,
                           bool pathIsMutable) {
    CHECK_FOR_NODRAW_ANNOTATION(paint);
    CHECK_SHOULD_DRAW(draw, false);

    GrPaint grPaint;
    if (!skPaint2GrPaintShader(this, paint, true, &grPaint)) {
        return;
    }

    
    
    SkScalar hairlineCoverage;
    bool doHairLine = SkDrawTreatAsHairline(paint, fContext->getMatrix(), &hairlineCoverage);
    if (doHairLine) {
        grPaint.setCoverage(SkScalarRoundToInt(hairlineCoverage * grPaint.getCoverage()));
    }

    
    
    
    SkPath* pathPtr = const_cast<SkPath*>(&origSrcPath);
    SkPath  tmpPath, effectPath;

    if (prePathMatrix) {
        SkPath* result = pathPtr;

        if (!pathIsMutable) {
            result = &tmpPath;
            pathIsMutable = true;
        }
        
        
        pathPtr->transform(*prePathMatrix, result);
        pathPtr = result;
    }
    
    SkDEBUGCODE(prePathMatrix = (const SkMatrix*)0x50FF8001;)

    SkStrokeRec stroke(paint);
    SkPathEffect* pathEffect = paint.getPathEffect();
    const SkRect* cullRect = NULL;  
    if (pathEffect && pathEffect->filterPath(&effectPath, *pathPtr, &stroke,
                                             cullRect)) {
        pathPtr = &effectPath;
    }

    if (!pathEffect && doHairLine) {
        stroke.setHairlineStyle();
    }

    if (paint.getMaskFilter()) {
        if (!stroke.isHairlineStyle()) {
            if (stroke.applyToPath(&tmpPath, *pathPtr)) {
                pathPtr = &tmpPath;
                stroke.setFillStyle();
            }
        }

        
        SkPath* devPathPtr = pathIsMutable ? pathPtr : &tmpPath;

        
        pathPtr->transform(fContext->getMatrix(), devPathPtr);
        if (!drawWithGPUMaskFilter(fContext, *devPathPtr, stroke, paint.getMaskFilter(),
                                   *draw.fClip, draw.fBounder, &grPaint)) {
            SkPaint::Style style = stroke.isHairlineStyle() ? SkPaint::kStroke_Style :
                                                              SkPaint::kFill_Style;
            drawWithMaskFilter(fContext, *devPathPtr, paint.getMaskFilter(),
                               *draw.fClip, draw.fBounder, &grPaint, style);
        }
        return;
    }

    fContext->drawPath(grPaint, *pathPtr, stroke);
}

namespace {

inline int get_tile_count(int l, int t, int r, int b, int tileSize)  {
    int tilesX = (r / tileSize) - (l / tileSize) + 1;
    int tilesY = (b / tileSize) - (t / tileSize) + 1;
    return tilesX * tilesY;
}

inline int determine_tile_size(const SkBitmap& bitmap,
                               const SkRect& src,
                               int maxTextureSize) {
    static const int kSmallTileSize = 1 << 10;
    if (maxTextureSize <= kSmallTileSize) {
        return maxTextureSize;
    }

    size_t maxTexTotalTileSize;
    size_t smallTotalTileSize;

    SkIRect iSrc;
    src.roundOut(&iSrc);

    maxTexTotalTileSize = get_tile_count(iSrc.fLeft,
                                         iSrc.fTop,
                                         iSrc.fRight,
                                         iSrc.fBottom,
                                         maxTextureSize);
    smallTotalTileSize = get_tile_count(iSrc.fLeft,
                                        iSrc.fTop,
                                        iSrc.fRight,
                                        iSrc.fBottom,
                                        kSmallTileSize);

    maxTexTotalTileSize *= maxTextureSize * maxTextureSize;
    smallTotalTileSize *= kSmallTileSize * kSmallTileSize;

    if (maxTexTotalTileSize > 2 * smallTotalTileSize) {
        return kSmallTileSize;
    } else {
        return maxTextureSize;
    }
}
}

bool SkGpuDevice::shouldTileBitmap(const SkBitmap& bitmap,
                                   const GrTextureParams& params,
                                   const SkRect* srcRectPtr) const {
    
    if (NULL != bitmap.getTexture()) {
        return false;
    }
    
    
    const int maxTextureSize = fContext->getMaxTextureSize();
    if (bitmap.width() > maxTextureSize ||
        bitmap.height() > maxTextureSize) {
        return true;
    }
    
    if (NULL == srcRectPtr) {
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

    SkScalar fracUsed = SkScalarMul(srcRectPtr->width() / bitmap.width(),
                                    srcRectPtr->height() / bitmap.height());
    if (fracUsed <= SK_ScalarHalf) {
        return true;
    } else {
        return false;
    }
}

void SkGpuDevice::drawBitmap(const SkDraw& draw,
                             const SkBitmap& bitmap,
                             const SkIRect* srcRectPtr,
                             const SkMatrix& m,
                             const SkPaint& paint) {

    SkRect  tmp;
    SkRect* tmpPtr = NULL;

    
    if (NULL != srcRectPtr) {
        tmp.set(*srcRectPtr);
        tmpPtr = &tmp;
    }

    
    this->drawBitmapCommon(draw, bitmap, tmpPtr, m, paint);
}

void SkGpuDevice::drawBitmapCommon(const SkDraw& draw,
                                   const SkBitmap& bitmap,
                                   const SkRect* srcRectPtr,
                                   const SkMatrix& m,
                                   const SkPaint& paint) {
    CHECK_SHOULD_DRAW(draw, false);

    SkRect srcRect;
    if (NULL == srcRectPtr) {
        srcRect.set(0, 0, SkIntToScalar(bitmap.width()), SkIntToScalar(bitmap.height()));
    } else {
        srcRect = *srcRectPtr;
    }

    if (paint.getMaskFilter()){
        
        
        SkMatrix        newM(m);
        SkBitmap        tmp;    
        const SkBitmap* bitmapPtr = &bitmap;
        if (NULL != srcRectPtr) {
            SkIRect iSrc;
            srcRect.roundOut(&iSrc);
            if (!bitmap.extractSubset(&tmp, iSrc)) {
                return;     
            }
            bitmapPtr = &tmp;
            srcRect.offset(SkIntToScalar(-iSrc.fLeft), SkIntToScalar(-iSrc.fTop));
            
            newM.preTranslate(SkIntToScalar(iSrc.fLeft), SkIntToScalar(iSrc.fTop));
        }

        SkPaint paintWithTexture(paint);
        paintWithTexture.setShader(SkShader::CreateBitmapShader(*bitmapPtr,
            SkShader::kClamp_TileMode, SkShader::kClamp_TileMode))->unref();

        
        
        
        SkMatrix drawMatrix;
        drawMatrix.setConcat(fContext->getMatrix(), newM);
        SkDraw transformedDraw(draw);
        transformedDraw.fMatrix = &drawMatrix;

        this->drawRect(transformedDraw, srcRect, paintWithTexture);

        return;
    }

    GrPaint grPaint;

    bool alphaOnly = !(SkBitmap::kA8_Config == bitmap.config());
    if (!skPaint2GrPaintNoShader(this, paint, alphaOnly, false, &grPaint)) {
        return;
    }
    GrTextureParams params;
    params.setBilerp(paint.isFilterBitmap());

    if (!this->shouldTileBitmap(bitmap, params, srcRectPtr)) {
        
        this->internalDrawBitmap(bitmap, srcRect, m, params, &grPaint);
    } else {
        this->drawTiledBitmap(bitmap, srcRect, m, params, &grPaint);
    }
}



void SkGpuDevice::drawTiledBitmap(const SkBitmap& bitmap,
                                  const SkRect& srcRect,
                                  const SkMatrix& m,
                                  const GrTextureParams& params,
                                  GrPaint* grPaint) {
    const int maxTextureSize = fContext->getMaxTextureSize();

    int tileSize = determine_tile_size(bitmap, srcRect, maxTextureSize);

    
    SkRect clipRect;
    {
        const GrRenderTarget* rt = fContext->getRenderTarget();
        clipRect.setWH(SkIntToScalar(rt->width()), SkIntToScalar(rt->height()));
        if (!fContext->getClip()->fClipStack->intersectRectWithClip(&clipRect)) {
            return;
        }
        SkMatrix matrix, inverse;
        matrix.setConcat(fContext->getMatrix(), m);
        if (!matrix.invert(&inverse)) {
            return;
        }
        inverse.mapRect(&clipRect);
    }

    int nx = bitmap.width() / tileSize;
    int ny = bitmap.height() / tileSize;
    for (int x = 0; x <= nx; x++) {
        for (int y = 0; y <= ny; y++) {
            SkRect tileR;
            tileR.set(SkIntToScalar(x * tileSize),
                      SkIntToScalar(y * tileSize),
                      SkIntToScalar((x + 1) * tileSize),
                      SkIntToScalar((y + 1) * tileSize));

            if (!SkRect::Intersects(tileR, clipRect)) {
                continue;
            }

            if (!tileR.intersect(srcRect)) {
                continue;
            }

            SkBitmap tmpB;
            SkIRect iTileR;
            tileR.roundOut(&iTileR);
            if (bitmap.extractSubset(&tmpB, iTileR)) {
                
                tileR.offset(SkIntToScalar(-iTileR.fLeft), SkIntToScalar(-iTileR.fTop));
                SkMatrix tmpM(m);
                tmpM.preTranslate(SkIntToScalar(iTileR.fLeft),
                                  SkIntToScalar(iTileR.fTop));

                this->internalDrawBitmap(tmpB, tileR, tmpM, params, grPaint);
            }
        }
    }
}

namespace {

bool hasAlignedSamples(const SkRect& srcRect, const SkRect& transformedRect) {
    
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

bool mayColorBleed(const SkRect& srcRect, const SkRect& transformedRect,
                   const SkMatrix& m) {
    
    
    GrAssert(!hasAlignedSamples(srcRect, transformedRect));
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

} 








void SkGpuDevice::internalDrawBitmap(const SkBitmap& bitmap,
                                     const SkRect& srcRect,
                                     const SkMatrix& m,
                                     const GrTextureParams& params,
                                     GrPaint* grPaint) {
    SkASSERT(bitmap.width() <= fContext->getMaxTextureSize() &&
             bitmap.height() <= fContext->getMaxTextureSize());

    SkAutoLockPixels alp(bitmap, !bitmap.getTexture());
    if (!bitmap.getTexture() && !bitmap.readyToDraw()) {
        SkDebugf("nothing to draw\n");
        return;
    }

    GrTexture* texture;
    SkAutoCachedTexture act(this, bitmap, &params, &texture);
    if (NULL == texture) {
        return;
    }

    GrRect dstRect(srcRect);
    GrRect paintRect;
    SkScalar wInv = SkScalarInvert(SkIntToScalar(bitmap.width()));
    SkScalar hInv = SkScalarInvert(SkIntToScalar(bitmap.height()));
    paintRect.setLTRB(SkScalarMul(srcRect.fLeft,   wInv),
                      SkScalarMul(srcRect.fTop,    hInv),
                      SkScalarMul(srcRect.fRight,  wInv),
                      SkScalarMul(srcRect.fBottom, hInv));

    bool needsTextureDomain = false;
    if (params.isBilerp()) {
        
        needsTextureDomain = srcRect.width() < bitmap.width() ||
                             srcRect.height() < bitmap.height();
        if (m.rectStaysRect() && fContext->getMatrix().rectStaysRect()) {
            
            GrRect transformedRect;
            SkMatrix srcToDeviceMatrix(m);
            srcToDeviceMatrix.postConcat(fContext->getMatrix());
            srcToDeviceMatrix.mapRect(&transformedRect, srcRect);

            if (hasAlignedSamples(srcRect, transformedRect)) {
                
                
                needsTextureDomain = false;
            } else {
                needsTextureDomain = needsTextureDomain &&
                    mayColorBleed(srcRect, transformedRect, m);
            }
        }
    }

    GrRect textureDomain = GrRect::MakeEmpty();
    SkAutoTUnref<GrEffectRef> effect;
    if (needsTextureDomain) {
        
        SkScalar left, top, right, bottom;
        if (srcRect.width() > SK_Scalar1) {
            SkScalar border = SK_ScalarHalf / bitmap.width();
            left = paintRect.left() + border;
            right = paintRect.right() - border;
        } else {
            left = right = SkScalarHalf(paintRect.left() + paintRect.right());
        }
        if (srcRect.height() > SK_Scalar1) {
            SkScalar border = SK_ScalarHalf / bitmap.height();
            top = paintRect.top() + border;
            bottom = paintRect.bottom() - border;
        } else {
            top = bottom = SkScalarHalf(paintRect.top() + paintRect.bottom());
        }
        textureDomain.setLTRB(left, top, right, bottom);
        effect.reset(GrTextureDomainEffect::Create(texture,
                                                   SkMatrix::I(),
                                                   textureDomain,
                                                   GrTextureDomainEffect::kClamp_WrapMode,
                                                   params.isBilerp()));
    } else {
        effect.reset(GrSimpleTextureEffect::Create(texture, SkMatrix::I(), params));
    }
    grPaint->colorStage(kBitmapEffectIdx)->setEffect(effect);
    fContext->drawRectToRect(*grPaint, dstRect, paintRect, &m);
}

namespace {

void apply_effect(GrContext* context,
                  GrTexture* srcTexture,
                  GrTexture* dstTexture,
                  const GrRect& rect,
                  GrEffectRef* effect) {
    SkASSERT(srcTexture && srcTexture->getContext() == context);
    GrContext::AutoMatrix am;
    am.setIdentity(context);
    GrContext::AutoRenderTarget art(context, dstTexture->asRenderTarget());
    GrContext::AutoClip acs(context, rect);

    GrPaint paint;
    paint.colorStage(0)->setEffect(effect);
    context->drawRect(paint, rect);
}

};

static SkBitmap wrap_texture(GrTexture* texture) {
    SkBitmap result;
    bool dummy;
    SkBitmap::Config config = grConfig2skConfig(texture->config(), &dummy);
    result.setConfig(config, texture->width(), texture->height());
    result.setPixelRef(SkNEW_ARGS(SkGrPixelRef, (texture)))->unref();
    return result;
}

static bool filter_texture(SkDevice* device, GrContext* context,
                           GrTexture* texture, SkImageFilter* filter,
                           int w, int h, SkBitmap* result) {
    GrAssert(filter);
    SkDeviceImageFilterProxy proxy(device);

    GrTextureDesc desc;
    desc.fFlags = kRenderTarget_GrTextureFlagBit,
    desc.fWidth = w;
    desc.fHeight = h;
    desc.fConfig = kRGBA_8888_GrPixelConfig;
    GrEffectRef* effect;

    if (filter->canFilterImageGPU()) {
        
        
        GrContext::AutoWideOpenIdentityDraw awo(context, NULL);
        return filter->filterImageGPU(&proxy, wrap_texture(texture), result);
    } else if (filter->asNewEffect(&effect, texture)) {
        GrAutoScratchTexture dst(context, desc);
        SkRect r = SkRect::MakeWH(SkIntToScalar(w), SkIntToScalar(h));
        apply_effect(context, texture, dst.texture(), r, effect);
        SkAutoTUnref<GrTexture> resultTex(dst.detach());
        effect->unref();
        *result = wrap_texture(resultTex.get());
        return true;
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

    GrPaint grPaint;
    if(!skPaint2GrPaintNoShader(this, paint, true, false, &grPaint)) {
        return;
    }

    GrEffectStage* stage = grPaint.colorStage(kBitmapEffectIdx);

    GrTexture* texture;
    stage->reset();
    
    SkAutoCachedTexture act(this, bitmap, NULL, &texture);
    grPaint.colorStage(kBitmapEffectIdx)->setEffect(
        GrSimpleTextureEffect::Create(texture, SkMatrix::I()))->unref();

    SkImageFilter* filter = paint.getImageFilter();
    if (NULL != filter) {
        SkBitmap filterBitmap;
        if (filter_texture(this, fContext, texture, filter, w, h, &filterBitmap)) {
            grPaint.colorStage(kBitmapEffectIdx)->setEffect(
                GrSimpleTextureEffect::Create((GrTexture*) filterBitmap.getTexture(), SkMatrix::I()))->unref();
            texture = (GrTexture*) filterBitmap.getTexture();
            w = filterBitmap.width();
            h = filterBitmap.height();
        }
    }

    fContext->drawRectToRect(grPaint,
                            GrRect::MakeXYWH(SkIntToScalar(left),
                                            SkIntToScalar(top),
                                            SkIntToScalar(w),
                                            SkIntToScalar(h)),
                            GrRect::MakeWH(SK_Scalar1 * w / texture->width(),
                                        SK_Scalar1 * h / texture->height()));
}

void SkGpuDevice::drawBitmapRect(const SkDraw& draw, const SkBitmap& bitmap,
                                 const SkRect* src, const SkRect& dst,
                                 const SkPaint& paint) {
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

    this->drawBitmapCommon(draw, bitmap, &tmpSrc, matrix, paint);
}

void SkGpuDevice::drawDevice(const SkDraw& draw, SkDevice* device,
                             int x, int y, const SkPaint& paint) {
    
    SkGpuDevice* dev = static_cast<SkGpuDevice*>(device);
    if (dev->fNeedClear) {
        
        dev->clear(0x0);
    }

    
    CHECK_SHOULD_DRAW(draw, true);

    GrPaint grPaint;
    grPaint.colorStage(kBitmapEffectIdx)->reset();
    if (!dev->bindDeviceAsTexture(&grPaint) ||
        !skPaint2GrPaintNoShader(this, paint, true, false, &grPaint)) {
        return;
    }

    GrTexture* devTex = (*grPaint.getColorStage(kBitmapEffectIdx).getEffect())->texture(0);
    SkASSERT(NULL != devTex);

    const SkBitmap& bm = dev->accessBitmap(false);
    int w = bm.width();
    int h = bm.height();

    SkImageFilter* filter = paint.getImageFilter();
    if (NULL != filter) {
        SkBitmap filterBitmap;
        if (filter_texture(this, fContext, devTex, filter, w, h, &filterBitmap)) {
            grPaint.colorStage(kBitmapEffectIdx)->setEffect(
                GrSimpleTextureEffect::Create((GrTexture*) filterBitmap.getTexture(), SkMatrix::I()))->unref();
            devTex = (GrTexture*) filterBitmap.getTexture();
            w = filterBitmap.width();
            h = filterBitmap.height();
        }
    }

    GrRect dstRect = GrRect::MakeXYWH(SkIntToScalar(x),
                                      SkIntToScalar(y),
                                      SkIntToScalar(w),
                                      SkIntToScalar(h));

    
    
    GrRect srcRect = GrRect::MakeWH(SK_Scalar1 * w / devTex->width(),
                                    SK_Scalar1 * h / devTex->height());

    fContext->drawRectToRect(grPaint, dstRect, srcRect);
}

bool SkGpuDevice::canHandleImageFilter(SkImageFilter* filter) {
    if (!filter->asNewEffect(NULL, NULL) &&
        !filter->canFilterImageGPU()) {
        return false;
    }
    return true;
}

bool SkGpuDevice::filterImage(SkImageFilter* filter, const SkBitmap& src,
                              const SkMatrix& ctm,
                              SkBitmap* result, SkIPoint* offset) {
    
    if (!this->SkGpuDevice::canHandleImageFilter(filter)) {
        return false;
    }

    SkAutoLockPixels alp(src, !src.getTexture());
    if (!src.getTexture() && !src.readyToDraw()) {
        return false;
    }

    GrPaint paint;

    GrTexture* texture;
    
    
    SkAutoCachedTexture act(this, src, NULL, &texture);

    return filter_texture(this, fContext, texture, filter, src.width(), src.height(), result);
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



static void GlyphCacheAuxProc(void* data) {
    GrFontScaler* scaler = (GrFontScaler*)data;
    SkSafeUnref(scaler);
}

static GrFontScaler* get_gr_font_scaler(SkGlyphCache* cache) {
    void* auxData;
    GrFontScaler* scaler = NULL;
    if (cache->getAuxProcData(GlyphCacheAuxProc, &auxData)) {
        scaler = (GrFontScaler*)auxData;
    }
    if (NULL == scaler) {
        scaler = SkNEW_ARGS(SkGrFontScaler, (cache));
        cache->setAuxProc(GlyphCacheAuxProc, scaler);
    }
    return scaler;
}

static void SkGPU_Draw1Glyph(const SkDraw1Glyph& state,
                             SkFixed fx, SkFixed fy,
                             const SkGlyph& glyph) {
    SkASSERT(glyph.fWidth > 0 && glyph.fHeight > 0);

    GrSkDrawProcs* procs = static_cast<GrSkDrawProcs*>(state.fDraw->fProcs);

    if (NULL == procs->fFontScaler) {
        procs->fFontScaler = get_gr_font_scaler(state.fCache);
    }

    procs->fTextContext->drawPackedGlyph(GrGlyph::Pack(glyph.getGlyphID(),
                                                       glyph.getSubXFixed(),
                                                       glyph.getSubYFixed()),
                                         SkFixedFloorToFixed(fx),
                                         SkFixedFloorToFixed(fy),
                                         procs->fFontScaler);
}

SkDrawProcs* SkGpuDevice::initDrawForText(GrTextContext* context) {

    
    if (NULL == fDrawProcs) {
        fDrawProcs = SkNEW(GrSkDrawProcs);
        fDrawProcs->fD1GProc = SkGPU_Draw1Glyph;
        fDrawProcs->fContext = fContext;
    }

    
    fDrawProcs->fTextContext = context;
    fDrawProcs->fFontScaler = NULL;
    return fDrawProcs;
}

void SkGpuDevice::drawText(const SkDraw& draw, const void* text,
                          size_t byteLength, SkScalar x, SkScalar y,
                          const SkPaint& paint) {
    CHECK_SHOULD_DRAW(draw, false);

    if (fContext->getMatrix().hasPerspective()) {
        
        draw.drawText((const char*)text, byteLength, x, y, paint);
    } else {
        SkDraw myDraw(draw);

        GrPaint grPaint;
        if (!skPaint2GrPaintShader(this, paint, true, &grPaint)) {
            return;
        }
        GrTextContext context(fContext, grPaint);
        myDraw.fProcs = this->initDrawForText(&context);
        this->INHERITED::drawText(myDraw, text, byteLength, x, y, paint);
    }
}

void SkGpuDevice::drawPosText(const SkDraw& draw, const void* text,
                             size_t byteLength, const SkScalar pos[],
                             SkScalar constY, int scalarsPerPos,
                             const SkPaint& paint) {
    CHECK_SHOULD_DRAW(draw, false);

    if (fContext->getMatrix().hasPerspective()) {
        
        draw.drawPosText((const char*)text, byteLength, pos, constY,
                         scalarsPerPos, paint);
    } else {
        SkDraw myDraw(draw);

        GrPaint grPaint;
        if (!skPaint2GrPaintShader(this, paint, true, &grPaint)) {
            return;
        }
        GrTextContext context(fContext, grPaint);
        myDraw.fProcs = this->initDrawForText(&context);
        this->INHERITED::drawPosText(myDraw, text, byteLength, pos, constY,
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



SkDevice* SkGpuDevice::onCreateCompatibleDevice(SkBitmap::Config config,
                                                int width, int height,
                                                bool isOpaque,
                                                Usage usage) {
    GrTextureDesc desc;
    desc.fConfig = fRenderTarget->config();
    desc.fFlags = kRenderTarget_GrTextureFlagBit;
    desc.fWidth = width;
    desc.fHeight = height;
    desc.fSampleCnt = fRenderTarget->numSamples();

    SkAutoTUnref<GrTexture> texture;
    
    bool needClear = !isOpaque;

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
        GrPrintf("---- failed to create compatible device texture [%d %d]\n", width, height);
        return NULL;
    }
}

SkGpuDevice::SkGpuDevice(GrContext* context,
                         GrTexture* texture,
                         bool needClear)
    : SkDevice(make_bitmap(context, texture->asRenderTarget())) {

    GrAssert(texture && texture->asRenderTarget());
    
    
    this->initFromRenderTarget(context, texture->asRenderTarget(), true);
    fNeedClear = needClear;
}
