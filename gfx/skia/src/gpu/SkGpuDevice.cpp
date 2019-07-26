






#include "SkGpuDevice.h"

#include "effects/GrColorTableEffect.h"
#include "effects/GrTextureDomainEffect.h"

#include "GrContext.h"
#include "GrTextContext.h"

#include "SkGrTexturePixelRef.h"

#include "SkColorFilter.h"
#include "SkDrawProcs.h"
#include "SkGlyphCache.h"
#include "SkImageFilter.h"
#include "SkTLazy.h"
#include "SkUtils.h"

#define CACHE_COMPATIBLE_DEVICE_TEXTURES 1

#if 0
    extern bool (*gShouldDrawProc)();
    #define CHECK_SHOULD_DRAW(draw)                             \
        do {                                                    \
            if (gShouldDrawProc && !gShouldDrawProc()) return;  \
            this->prepareRenderTarget(draw);                    \
            GrAssert(!fNeedClear)                               \
        } while (0)
#else
    #define CHECK_SHOULD_DRAW(draw) this->prepareRenderTarget(draw); \
                                    GrAssert(!fNeedClear)
#endif



enum {
    kBitmapTextureIdx = 0,
    kShaderTextureIdx = 0,
    kColorFilterTextureIdx = 1
};


#define MAX_BLUR_SIGMA 4.0f


#define MAX_BLUR_RADIUS SkIntToScalar(128)







#define BLUR_SIGMA_SCALE 0.6f



#define COLOR_BLEED_TOLERANCE SkFloatToScalar(0.001f)

#define DO_DEFERRED_CLEAR       \
    do {                        \
        if (fNeedClear) {       \
            this->clear(0x0);   \
            fNeedClear = false; \
        }                       \
    } while (false)             \



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
            GrUnlockCachedBitmapTexture(fTexture);
        }
    }

    GrTexture* set(SkGpuDevice* device,
                   const SkBitmap& bitmap,
                   const GrTextureParams* params) {
        if (NULL != fTexture) {
            GrUnlockCachedBitmapTexture(fTexture);
            fTexture = NULL;
        }
        fDevice = device;
        GrTexture* result = (GrTexture*)bitmap.getTexture();
        if (NULL == result) {
            
            fTexture = GrLockCachedBitmapTexture(device->context(), bitmap, params);
            result = fTexture;
        }
        return result;
    }

private:
    SkGpuDevice* fDevice;
    GrTexture*   fTexture;
};



bool gDoTraceDraw;

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
        case kSkia8888_PM_GrPixelConfig:
            
            
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
    fNeedPrepareRenderTarget = false;
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
                         int height)
    : SkDevice(config, width, height, false ) {

    fNeedPrepareRenderTarget = false;
    fDrawProcs = NULL;

    fContext = context;
    fContext->ref();

    fRenderTarget = NULL;
    fNeedClear = false;

    if (config != SkBitmap::kRGB_565_Config) {
        config = SkBitmap::kARGB_8888_Config;
    }
    SkBitmap bm;
    bm.setConfig(config, width, height);

    GrTextureDesc desc;
    desc.fFlags = kRenderTarget_GrTextureFlagBit;
    desc.fWidth = width;
    desc.fHeight = height;
    desc.fConfig = SkBitmapConfig2GrPixelConfig(bm.config());

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

    
    
    fContext->setRenderTarget(NULL);

    SkSafeUnref(fRenderTarget);
    fContext->unref();
}



void SkGpuDevice::makeRenderTargetCurrent() {
    DO_DEFERRED_CLEAR;
    fContext->setRenderTarget(fRenderTarget);
    fNeedPrepareRenderTarget = true;
}



namespace {
GrPixelConfig config8888_to_grconfig_and_flags(SkCanvas::Config8888 config8888, uint32_t* flags) {
    switch (config8888) {
        case SkCanvas::kNative_Premul_Config8888:
            *flags = 0;
            return kSkia8888_GrPixelConfig;
        case SkCanvas::kNative_Unpremul_Config8888:
            *flags = GrContext::kUnpremul_PixelOpsFlag;
            return kSkia8888_PM_GrPixelConfig;
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
            return kSkia8888_PM_GrPixelConfig;
    }
}
}

bool SkGpuDevice::onReadPixels(const SkBitmap& bitmap,
                               int x, int y,
                               SkCanvas::Config8888 config8888) {
    DO_DEFERRED_CLEAR;
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
void purgeClipCB(int genID, void* data) {
    GrContext* context = (GrContext*) data;

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



static void set_matrix_and_clip(GrContext* context, const SkMatrix& matrix,
                                GrClipData& clipData,
                                const SkRegion& clipRegion,
                                const SkIPoint& origin,
                                int renderTargetWidth, int renderTargetHeight) {
    context->setMatrix(matrix);

    clipData.fOrigin = origin;

#ifdef SK_DEBUG
    check_bounds(clipData, clipRegion,
                 renderTargetWidth, renderTargetHeight);
#endif

    context->setClip(&clipData);
}



void SkGpuDevice::prepareRenderTarget(const SkDraw& draw) {
    GrAssert(NULL != fClipData.fClipStack);

    if (fNeedPrepareRenderTarget ||
        fContext->getRenderTarget() != fRenderTarget) {

        fContext->setRenderTarget(fRenderTarget);
        SkASSERT(draw.fClipStack && draw.fClipStack == fClipData.fClipStack);

        set_matrix_and_clip(fContext, *draw.fMatrix,
                            fClipData, *draw.fClip, this->getOrigin(),
                            fRenderTarget->width(), fRenderTarget->height());
        fNeedPrepareRenderTarget = false;
    }
}

void SkGpuDevice::setMatrixClip(const SkMatrix& matrix, const SkRegion& clip,
                                const SkClipStack& clipStack) {
    this->INHERITED::setMatrixClip(matrix, clip, clipStack);
    
    fNeedPrepareRenderTarget = true;
}

void SkGpuDevice::gainFocus(const SkMatrix& matrix, const SkRegion& clip) {

    GrAssert(NULL != fClipData.fClipStack);

    fContext->setRenderTarget(fRenderTarget);

    this->INHERITED::gainFocus(matrix, clip);

    set_matrix_and_clip(fContext, matrix, fClipData, clip, this->getOrigin(),
                        fRenderTarget->width(), fRenderTarget->height());

    DO_DEFERRED_CLEAR;
}

SkGpuRenderTarget* SkGpuDevice::accessRenderTarget() {
    DO_DEFERRED_CLEAR;
    return (SkGpuRenderTarget*)fRenderTarget;
}

bool SkGpuDevice::bindDeviceAsTexture(GrPaint* paint) {
    GrTexture* texture = fRenderTarget->asTexture();
    if (NULL != texture) {
        paint->textureSampler(kBitmapTextureIdx)->setCustomStage(
            SkNEW_ARGS(GrSingleTextureEffect, (texture)))->unref();
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
                                    SkGpuDevice::SkAutoCachedTexture* act,
                                    GrPaint* grPaint) {

    grPaint->fDither    = skPaint.isDither();
    grPaint->fAntiAlias = skPaint.isAntiAlias();
    grPaint->fCoverage = 0xFF;

    SkXfermode::Coeff sm = SkXfermode::kOne_Coeff;
    SkXfermode::Coeff dm = SkXfermode::kISA_Coeff;

    SkXfermode* mode = skPaint.getXfermode();
    if (mode) {
        if (!mode->asCoeff(&sm, &dm)) {
            
#if 0
            return false;
#endif
        }
    }
    grPaint->fSrcBlendCoeff = sk_blend_to_grblend(sm);
    grPaint->fDstBlendCoeff = sk_blend_to_grblend(dm);

    if (justAlpha) {
        uint8_t alpha = skPaint.getAlpha();
        grPaint->fColor = GrColorPackRGBA(alpha, alpha, alpha, alpha);
        
        
        GrAssert(!constantColor);
    } else {
        grPaint->fColor = SkColor2GrColor(skPaint.getColor());
        GrAssert(!grPaint->isTextureStageEnabled(kShaderTextureIdx));
    }
    SkColorFilter* colorFilter = skPaint.getColorFilter();
    SkColor color;
    SkXfermode::Mode filterMode;
    SkScalar matrix[20];
    SkBitmap colorTransformTable;
    grPaint->resetColorFilter();
    if (colorFilter != NULL && colorFilter->asColorMode(&color, &filterMode)) {
        grPaint->fColorMatrixEnabled = false;
        if (!constantColor) {
            grPaint->fColorFilterColor = SkColor2GrColor(color);
            grPaint->fColorFilterXfermode = filterMode;
        } else {
            SkColor filtered = colorFilter->filterColor(skPaint.getColor());
            grPaint->fColor = SkColor2GrColor(filtered);
        }
    } else if (colorFilter != NULL && colorFilter->asColorMatrix(matrix)) {
        grPaint->fColorMatrixEnabled = true;
        memcpy(grPaint->fColorMatrix, matrix, sizeof(matrix));
        grPaint->fColorFilterXfermode = SkXfermode::kDst_Mode;
    } else if (colorFilter != NULL && colorFilter->asComponentTable(
        &colorTransformTable)) {
        grPaint->resetColorFilter();

        GrSamplerState* colorSampler = grPaint->textureSampler(kColorFilterTextureIdx);
        GrTexture* texture = act->set(dev, colorTransformTable, colorSampler->textureParams());

        colorSampler->reset();
        colorSampler->setCustomStage(SkNEW_ARGS(GrColorTableEffect, (texture)))->unref();
    }
    return true;
}





inline bool skPaint2GrPaintShader(SkGpuDevice* dev,
                                  const SkPaint& skPaint,
                                  bool constantColor,
                                  SkGpuDevice::SkAutoCachedTexture textures[GrPaint::kMaxTextures],
                                  GrPaint* grPaint) {
    SkShader* shader = skPaint.getShader();
    if (NULL == shader) {
        return skPaint2GrPaintNoShader(dev,
                                       skPaint,
                                       false,
                                       constantColor,
                                       &textures[kColorFilterTextureIdx],
                                       grPaint);
    } else if (!skPaint2GrPaintNoShader(dev, skPaint, true, false,
                                        &textures[kColorFilterTextureIdx], grPaint)) {
        return false;
    }

    GrSamplerState* sampler = grPaint->textureSampler(kShaderTextureIdx);
    GrCustomStage* stage = shader->asNewCustomStage(dev->context(), sampler);

    if (NULL != stage) {
        sampler->setCustomStage(stage)->unref();
        SkMatrix localM;
        if (shader->getLocalMatrix(&localM)) {
            SkMatrix inverse;
            if (localM.invert(&inverse)) {
                sampler->matrix()->preConcat(inverse);
            }
        }
        return true;
    }

    SkBitmap bitmap;
    SkMatrix* matrix = sampler->matrix();
    SkShader::TileMode tileModes[2];
    SkShader::BitmapType bmptype = shader->asABitmap(&bitmap, matrix,
                                                     tileModes);

    if (SkShader::kNone_BitmapType == bmptype) {
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
            return skPaint2GrPaintNoShader(dev,
                                           copy,
                                           false,
                                           constantColor,
                                           &textures[kColorFilterTextureIdx],
                                           grPaint);
        }
        return false;
    }

    
    sampler->textureParams()->reset(tileModes, skPaint.isFilterBitmap());
    GrTexture* texture = textures[kShaderTextureIdx].set(dev, bitmap, sampler->textureParams());

    if (NULL == texture) {
        SkDebugf("Couldn't convert bitmap to texture.\n");
        return false;
    }

    sampler->setCustomStage(SkNEW_ARGS(GrSingleTextureEffect, (texture)))->unref();

    
    
    SkMatrix localM;
    if (shader->getLocalMatrix(&localM)) {
        SkMatrix inverse;
        if (localM.invert(&inverse)) {
            matrix->preConcat(inverse);
        }
    }
    if (SkShader::kDefault_BitmapType == bmptype) {
        GrScalar sx = SkFloatToScalar(1.f / bitmap.width());
        GrScalar sy = SkFloatToScalar(1.f / bitmap.height());
        matrix->postScale(sx, sy);
    }

    return true;
}
}


void SkGpuDevice::clear(SkColor color) {
    fContext->clear(NULL, color, fRenderTarget);
}

void SkGpuDevice::drawPaint(const SkDraw& draw, const SkPaint& paint) {
    CHECK_SHOULD_DRAW(draw);

    GrPaint grPaint;
    SkAutoCachedTexture textures[GrPaint::kMaxTextures];
    if (!skPaint2GrPaintShader(this,
                               paint,
                               true,
                               textures,
                               &grPaint)) {
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
    CHECK_SHOULD_DRAW(draw);

    SkScalar width = paint.getStrokeWidth();
    if (width < 0) {
        return;
    }

    
    
    if (width > 0 || paint.getPathEffect() || paint.getMaskFilter()) {
        draw.drawPoints(mode, count, pts, paint, true);
        return;
    }

    GrPaint grPaint;
    SkAutoCachedTexture textures[GrPaint::kMaxTextures];
    if (!skPaint2GrPaintShader(this,
                               paint,
                               true,
                               textures,
                               &grPaint)) {
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
    CHECK_SHOULD_DRAW(draw);

    bool doStroke = paint.getStyle() != SkPaint::kFill_Style;
    SkScalar width = paint.getStrokeWidth();

    



    bool usePath = doStroke && width > 0 &&
                    paint.getStrokeJoin() != SkPaint::kMiter_Join;
    
    if (paint.getMaskFilter() || paint.getPathEffect()) {
        usePath = true;
    }
    
    if (!usePath && paint.isAntiAlias() && !draw.fMatrix->rectStaysRect()) {
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
    SkAutoCachedTexture textures[GrPaint::kMaxTextures];
    if (!skPaint2GrPaintShader(this,
                               paint,
                               true,
                               textures,
                               &grPaint)) {
        return;
    }
    fContext->drawRect(grPaint, rect, doStroke ? width : -1);
}

#include "SkMaskFilter.h"
#include "SkBounder.h"




namespace {

GrPathFill skToGrFillType(SkPath::FillType fillType) {
    switch (fillType) {
        case SkPath::kWinding_FillType:
            return kWinding_GrPathFill;
        case SkPath::kEvenOdd_FillType:
            return kEvenOdd_GrPathFill;
        case SkPath::kInverseWinding_FillType:
            return kInverseWinding_GrPathFill;
        case SkPath::kInverseEvenOdd_FillType:
            return kInverseEvenOdd_GrPathFill;
        default:
            SkDebugf("Unsupported path fill type\n");
            return kHairLine_GrPathFill;
    }
}


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

bool drawWithGPUMaskFilter(GrContext* context, const SkPath& path,
                           SkMaskFilter* filter, const SkMatrix& matrix,
                           const SkRegion& clip, SkBounder* bounder,
                           GrPaint* grp, GrPathFill pathFillType) {
#ifdef SK_DISABLE_GPU_BLUR
    return false;
#endif
    SkMaskFilter::BlurInfo info;
    SkMaskFilter::BlurType blurType = filter->asABlur(&info);
    if (SkMaskFilter::kNone_BlurType == blurType) {
        return false;
    }
    SkScalar radius = info.fIgnoreTransform ? info.fRadius
                                            : matrix.mapRadius(info.fRadius);
    radius = SkMinScalar(radius, MAX_BLUR_RADIUS);
    if (radius <= 0) {
        return false;
    }

    SkRect srcRect = path.getBounds();
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
    GrRenderTarget* oldRenderTarget = context->getRenderTarget();
    
    
    const GrClipData* oldClipData = context->getClip();

    context->setRenderTarget(pathTexture->asRenderTarget());

    SkClipStack newClipStack(srcRect);
    GrClipData newClipData;
    newClipData.fClipStack = &newClipStack;
    context->setClip(&newClipData);

    context->clear(NULL, 0);
    GrPaint tempPaint;
    tempPaint.reset();

    GrContext::AutoMatrix avm(context, GrMatrix::I());
    tempPaint.fAntiAlias = grp->fAntiAlias;
    if (tempPaint.fAntiAlias) {
        
        
        
        
        
        
        tempPaint.fSrcBlendCoeff = kOne_GrBlendCoeff;
        tempPaint.fDstBlendCoeff = kISC_GrBlendCoeff;
    }
    
    context->drawPath(tempPaint, path, pathFillType, &offset);

    
    
    bool isNormalBlur = blurType == SkMaskFilter::kNormal_BlurType;
    SkAutoTUnref<GrTexture> blurTexture(context->gaussianBlur(
        pathTexture, isNormalBlur, srcRect, sigma, sigma));

    if (!isNormalBlur) {
        GrPaint paint;
        paint.reset();
        paint.textureSampler(0)->textureParams()->setClampNoFilter();
        paint.textureSampler(0)->matrix()->setIDiv(pathTexture->width(),
                                                   pathTexture->height());
        
        context->setRenderTarget(blurTexture->asRenderTarget());
        paint.textureSampler(0)->setCustomStage(SkNEW_ARGS
            (GrSingleTextureEffect, (pathTexture)))->unref();
        if (SkMaskFilter::kInner_BlurType == blurType) {
            
            paint.fSrcBlendCoeff = kDC_GrBlendCoeff;
            paint.fDstBlendCoeff = kZero_GrBlendCoeff;
        } else if (SkMaskFilter::kSolid_BlurType == blurType) {
            
            
            paint.fSrcBlendCoeff = kIDC_GrBlendCoeff;
            paint.fDstBlendCoeff = kOne_GrBlendCoeff;
        } else if (SkMaskFilter::kOuter_BlurType == blurType) {
            
            
            paint.fSrcBlendCoeff = kZero_GrBlendCoeff;
            paint.fDstBlendCoeff = kISC_GrBlendCoeff;
        }
        context->drawRect(paint, srcRect);
    }
    context->setRenderTarget(oldRenderTarget);
    context->setClip(oldClipData);

    if (!grp->preConcatSamplerMatricesWithInverse(matrix)) {
        return false;
    }

    static const int MASK_IDX = GrPaint::kMaxMasks - 1;
    
    GrAssert(!grp->isMaskStageEnabled(MASK_IDX));
    grp->maskSampler(MASK_IDX)->reset();
    grp->maskSampler(MASK_IDX)->setCustomStage(
        SkNEW_ARGS(GrSingleTextureEffect, (blurTexture)))->unref();
    grp->maskSampler(MASK_IDX)->matrix()->setTranslate(-finalRect.fLeft,
                                                       -finalRect.fTop);
    grp->maskSampler(MASK_IDX)->matrix()->postIDiv(blurTexture->width(),
                                                   blurTexture->height());
    context->drawRect(*grp, finalRect);
    return true;
}

bool drawWithMaskFilter(GrContext* context, const SkPath& path,
                        SkMaskFilter* filter, const SkMatrix& matrix,
                        const SkRegion& clip, SkBounder* bounder,
                        GrPaint* grp, SkPaint::Style style) {
    SkMask  srcM, dstM;

    if (!SkDraw::DrawToMask(path, &clip.getBounds(), filter, &matrix, &srcM,
                            SkMask::kComputeBoundsAndRenderImage_CreateMode,
                            style)) {
        return false;
    }
    SkAutoMaskFreeImage autoSrc(srcM.fImage);

    if (!filter->filterMask(&dstM, srcM, matrix, NULL)) {
        return false;
    }
    
    SkAutoMaskFreeImage autoDst(dstM.fImage);

    if (clip.quickReject(dstM.fBounds)) {
        return false;
    }
    if (bounder && !bounder->doIRect(dstM.fBounds)) {
        return false;
    }

    
    

    GrContext::AutoMatrix avm(context, GrMatrix::I());

    if (!grp->preConcatSamplerMatricesWithInverse(matrix)) {
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

    static const int MASK_IDX = GrPaint::kMaxMasks - 1;
    
    GrAssert(!grp->isMaskStageEnabled(MASK_IDX));
    grp->maskSampler(MASK_IDX)->reset();
    grp->maskSampler(MASK_IDX)->setCustomStage(
        SkNEW_ARGS(GrSingleTextureEffect, (texture)))->unref();
    GrRect d;
    d.setLTRB(GrIntToScalar(dstM.fBounds.fLeft),
              GrIntToScalar(dstM.fBounds.fTop),
              GrIntToScalar(dstM.fBounds.fRight),
              GrIntToScalar(dstM.fBounds.fBottom));

    GrMatrix* m = grp->maskSampler(MASK_IDX)->matrix();
    m->setTranslate(-dstM.fBounds.fLeft*SK_Scalar1,
                         -dstM.fBounds.fTop*SK_Scalar1);
    m->postIDiv(texture->width(), texture->height());
    context->drawRect(*grp, d);
    return true;
}

}



void SkGpuDevice::drawPath(const SkDraw& draw, const SkPath& origSrcPath,
                           const SkPaint& paint, const SkMatrix* prePathMatrix,
                           bool pathIsMutable) {
    CHECK_FOR_NODRAW_ANNOTATION(paint);
    CHECK_SHOULD_DRAW(draw);

    bool             doFill = true;

    GrPaint grPaint;
    SkAutoCachedTexture textures[GrPaint::kMaxTextures];
    if (!skPaint2GrPaintShader(this,
                               paint,
                               true,
                               textures,
                               &grPaint)) {
        return;
    }

    
    
    SkScalar hairlineCoverage;
    if (SkDrawTreatAsHairline(paint, *draw.fMatrix, &hairlineCoverage)) {
        doFill = false;
        grPaint.fCoverage = SkScalarRoundToInt(hairlineCoverage *
                                               grPaint.fCoverage);
    }

    
    
    
    SkPath* pathPtr = const_cast<SkPath*>(&origSrcPath);
    SkPath  tmpPath;

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

    if (paint.getPathEffect() ||
        (doFill && paint.getStyle() != SkPaint::kFill_Style)) {
        
        
        
        doFill = paint.getFillPath(*pathPtr, &tmpPath);
        pathPtr = &tmpPath;
    }

    if (paint.getMaskFilter()) {
        
        SkPath* devPathPtr = pathIsMutable ? pathPtr : &tmpPath;

        
        pathPtr->transform(*draw.fMatrix, devPathPtr);
        GrPathFill pathFillType = doFill ?
            skToGrFillType(devPathPtr->getFillType()) : kHairLine_GrPathFill;
        if (!drawWithGPUMaskFilter(fContext, *devPathPtr, paint.getMaskFilter(),
                                   *draw.fMatrix, *draw.fClip, draw.fBounder,
                                   &grPaint, pathFillType)) {
            SkPaint::Style style = doFill ? SkPaint::kFill_Style :
                SkPaint::kStroke_Style;
            drawWithMaskFilter(fContext, *devPathPtr, paint.getMaskFilter(),
                               *draw.fMatrix, *draw.fClip, draw.fBounder,
                               &grPaint, style);
        }
        return;
    }

    GrPathFill fill = kHairLine_GrPathFill;

    if (doFill) {
        switch (pathPtr->getFillType()) {
            case SkPath::kWinding_FillType:
                fill = kWinding_GrPathFill;
                break;
            case SkPath::kEvenOdd_FillType:
                fill = kEvenOdd_GrPathFill;
                break;
            case SkPath::kInverseWinding_FillType:
                fill = kInverseWinding_GrPathFill;
                break;
            case SkPath::kInverseEvenOdd_FillType:
                fill = kInverseEvenOdd_GrPathFill;
                break;
            default:
                SkDebugf("Unsupported path fill type\n");
                return;
        }
    }

    fContext->drawPath(grPaint, *pathPtr, fill);
}

namespace {

inline int get_tile_count(int l, int t, int r, int b, int tileSize)  {
    int tilesX = (r / tileSize) - (l / tileSize) + 1;
    int tilesY = (b / tileSize) - (t / tileSize) + 1;
    return tilesX * tilesY;
}

inline int determine_tile_size(const SkBitmap& bitmap,
                               const SkIRect* srcRectPtr,
                               int maxTextureSize) {
    static const int kSmallTileSize = 1 << 10;
    if (maxTextureSize <= kSmallTileSize) {
        return maxTextureSize;
    }

    size_t maxTexTotalTileSize;
    size_t smallTotalTileSize;

    if (NULL == srcRectPtr) {
        int w = bitmap.width();
        int h = bitmap.height();
        maxTexTotalTileSize = get_tile_count(0, 0, w, h, maxTextureSize);
        smallTotalTileSize = get_tile_count(0, 0, w, h, kSmallTileSize);
    } else {
        maxTexTotalTileSize = get_tile_count(srcRectPtr->fLeft,
                                             srcRectPtr->fTop,
                                             srcRectPtr->fRight,
                                             srcRectPtr->fBottom,
                                             maxTextureSize);
        smallTotalTileSize = get_tile_count(srcRectPtr->fLeft,
                                            srcRectPtr->fTop,
                                            srcRectPtr->fRight,
                                            srcRectPtr->fBottom,
                                            kSmallTileSize);
    }
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
                                   const SkIRect* srcRectPtr,
                                   int* tileSize) const {
    SkASSERT(NULL != tileSize);

    
    if (NULL != bitmap.getTexture()) {
        return false;
    }
    
    
    const int maxTextureSize = fContext->getMaxTextureSize();
    if (bitmap.width() > maxTextureSize ||
        bitmap.height() > maxTextureSize) {
        *tileSize = determine_tile_size(bitmap, srcRectPtr, maxTextureSize);
        return true;
    }
    
    if (NULL == srcRectPtr) {
        return false;
    }
    
    if (this->isBitmapInTextureCache(bitmap, params)) {
        return false;
    }

    
    
    
    

    
    
    size_t bmpSize = bitmap.getSize();
    size_t cacheSize;
    fContext->getTextureCacheLimits(NULL, &cacheSize);
    if (bmpSize < cacheSize / 2) {
        return false;
    }

    SkFixed fracUsed =
        SkFixedMul((srcRectPtr->width() << 16) / bitmap.width(),
                   (srcRectPtr->height() << 16) / bitmap.height());
    if (fracUsed <= SK_FixedHalf) {
        *tileSize = determine_tile_size(bitmap, srcRectPtr, maxTextureSize);
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
    CHECK_SHOULD_DRAW(draw);

    SkIRect srcRect;
    if (NULL == srcRectPtr) {
        srcRect.set(0, 0, bitmap.width(), bitmap.height());
    } else {
        srcRect = *srcRectPtr;
    }

    if (paint.getMaskFilter()){
        
        
        SkBitmap        tmp;    
        const SkBitmap* bitmapPtr = &bitmap;
        if (srcRectPtr) {
            if (!bitmap.extractSubset(&tmp, srcRect)) {
                return;     
            }
            bitmapPtr = &tmp;
            srcRect.set(0,0, srcRect.width(), srcRect.height());
        }
        SkPaint paintWithTexture(paint);
        paintWithTexture.setShader(SkShader::CreateBitmapShader( *bitmapPtr,
            SkShader::kClamp_TileMode, SkShader::kClamp_TileMode))->unref();
        SkRect ScalarRect;
        ScalarRect.set(srcRect);

        
        
        
        SkMatrix drawMatrix;
        drawMatrix.setConcat(*draw.fMatrix, m);
        SkDraw transformedDraw(draw);
        transformedDraw.fMatrix = &drawMatrix;

        this->drawRect(transformedDraw, ScalarRect, paintWithTexture);

        return;
    }

    GrPaint grPaint;
    SkAutoCachedTexture colorLutTexture;
    if (!skPaint2GrPaintNoShader(this, paint, true, false, &colorLutTexture, &grPaint)) {
        return;
    }
    GrTextureParams* params = grPaint.textureSampler(kBitmapTextureIdx)->textureParams();
    params->setBilerp(paint.isFilterBitmap());

    int tileSize;
    if (!this->shouldTileBitmap(bitmap, *params, srcRectPtr, &tileSize)) {
        
        this->internalDrawBitmap(draw, bitmap, srcRect, m, &grPaint);
        return;
    }

    
    int DX = SkMax32(0, srcRect.fLeft);
    int DY = SkMax32(0, srcRect.fTop);
    
    SkIRect clipRect;
    {
        SkRect r;
        r.set(draw.fClip->getBounds());
        SkMatrix matrix, inverse;
        matrix.setConcat(*draw.fMatrix, m);
        if (!matrix.invert(&inverse)) {
            return;
        }
        inverse.mapRect(&r);
        r.roundOut(&clipRect);
        
        clipRect.offset(DX, DY);
    }

    int nx = bitmap.width() / tileSize;
    int ny = bitmap.height() / tileSize;
    for (int x = 0; x <= nx; x++) {
        for (int y = 0; y <= ny; y++) {
            SkIRect tileR;
            tileR.set(x * tileSize, y * tileSize,
                      (x + 1) * tileSize, (y + 1) * tileSize);
            if (!SkIRect::Intersects(tileR, clipRect)) {
                continue;
            }

            SkIRect srcR = tileR;
            if (!srcR.intersect(srcRect)) {
                continue;
            }

            SkBitmap tmpB;
            if (bitmap.extractSubset(&tmpB, tileR)) {
                
                srcR.offset(-tileR.fLeft, -tileR.fTop);

                SkMatrix tmpM(m);
                {
                    int dx = tileR.fLeft - DX + SkMax32(0, srcR.fLeft);
                    int dy = tileR.fTop -  DY + SkMax32(0, srcR.fTop);
                    tmpM.preTranslate(SkIntToScalar(dx), SkIntToScalar(dy));
                }
                this->internalDrawBitmap(draw, tmpB, srcR, tmpM, &grPaint);
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








void SkGpuDevice::internalDrawBitmap(const SkDraw& draw,
                                     const SkBitmap& bitmap,
                                     const SkIRect& srcRect,
                                     const SkMatrix& m,
                                     GrPaint* grPaint) {
    SkASSERT(bitmap.width() <= fContext->getMaxTextureSize() &&
             bitmap.height() <= fContext->getMaxTextureSize());

    SkAutoLockPixels alp(bitmap, !bitmap.getTexture());
    if (!bitmap.getTexture() && !bitmap.readyToDraw()) {
        SkDebugf("nothing to draw\n");
        return;
    }

    GrSamplerState* sampler = grPaint->textureSampler(kBitmapTextureIdx);

    sampler->textureParams()->setClamp();
    sampler->matrix()->reset();

    GrTexture* texture;
    SkAutoCachedTexture act(this, bitmap, sampler->textureParams(), &texture);
    if (NULL == texture) {
        return;
    }

    grPaint->textureSampler(kBitmapTextureIdx)->setCustomStage(SkNEW_ARGS
        (GrSingleTextureEffect, (texture)))->unref();

    GrRect dstRect = SkRect::MakeWH(GrIntToScalar(srcRect.width()),
                                    GrIntToScalar(srcRect.height()));
    GrRect paintRect;
    float wInv = 1.f / bitmap.width();
    float hInv = 1.f / bitmap.height();
    paintRect.setLTRB(SkFloatToScalar(srcRect.fLeft * wInv),
                      SkFloatToScalar(srcRect.fTop * hInv),
                      SkFloatToScalar(srcRect.fRight * wInv),
                      SkFloatToScalar(srcRect.fBottom * hInv));

    bool needsTextureDomain = false;
    if (sampler->textureParams()->isBilerp()) {
        
        needsTextureDomain = srcRect.width() < bitmap.width() || srcRect.height() < bitmap.height();
        if (m.rectStaysRect() && draw.fMatrix->rectStaysRect()) {
            
            GrRect floatSrcRect, transformedRect;
            floatSrcRect.set(srcRect);
            SkMatrix srcToDeviceMatrix(m);
            srcToDeviceMatrix.postConcat(*draw.fMatrix);
            srcToDeviceMatrix.mapRect(&transformedRect, floatSrcRect);

            if (hasAlignedSamples(floatSrcRect, transformedRect)) {
                
                sampler->textureParams()->setBilerp(false);
                needsTextureDomain = false;
            } else {
                needsTextureDomain = needsTextureDomain &&
                    mayColorBleed(floatSrcRect, transformedRect, m);
            }
        }
    }

    GrRect textureDomain = GrRect::MakeEmpty();

    if (needsTextureDomain) {
        
        GrScalar left, top, right, bottom;
        if (srcRect.width() > 1) {
            GrScalar border = GR_ScalarHalf / bitmap.width();
            left = paintRect.left() + border;
            right = paintRect.right() - border;
        } else {
            left = right = GrScalarHalf(paintRect.left() + paintRect.right());
        }
        if (srcRect.height() > 1) {
            GrScalar border = GR_ScalarHalf / bitmap.height();
            top = paintRect.top() + border;
            bottom = paintRect.bottom() - border;
        } else {
            top = bottom = GrScalarHalf(paintRect.top() + paintRect.bottom());
        }
        textureDomain.setLTRB(left, top, right, bottom);
        sampler->setCustomStage(SkNEW_ARGS(GrTextureDomainEffect,
                     (texture,
                      textureDomain)))->unref();
    }

    fContext->drawRectToRect(*grPaint, dstRect, paintRect, &m);
}

namespace {

void apply_custom_stage(GrContext* context,
                        GrTexture* srcTexture,
                        GrTexture* dstTexture,
                        const GrRect& rect,
                        GrCustomStage* stage) {
    SkASSERT(srcTexture && srcTexture->getContext() == context);
    GrContext::AutoMatrix avm(context, GrMatrix::I());
    GrContext::AutoRenderTarget art(context, dstTexture->asRenderTarget());
    GrContext::AutoClip acs(context, rect);

    GrMatrix sampleM;
    sampleM.setIDiv(srcTexture->width(), srcTexture->height());
    GrPaint paint;
    paint.reset();
    paint.textureSampler(0)->textureParams()->setBilerp(true);
    paint.textureSampler(0)->reset(sampleM);
    paint.textureSampler(0)->setCustomStage(stage);
    context->drawRect(paint, rect);
}

};

static GrTexture* filter_texture(GrContext* context, GrTexture* texture,
                                 SkImageFilter* filter, const GrRect& rect) {
    GrAssert(filter);

    GrTextureDesc desc;
    desc.fFlags = kRenderTarget_GrTextureFlagBit,
    desc.fWidth = SkScalarCeilToInt(rect.width());
    desc.fHeight = SkScalarCeilToInt(rect.height());
    desc.fConfig = kRGBA_8888_GrPixelConfig;
    GrCustomStage* stage;

    if (filter->canFilterImageGPU()) {
        texture = filter->onFilterImageGPU(texture, rect);
    } else if (filter->asNewCustomStage(&stage, texture)) {
        GrAutoScratchTexture dst(context, desc);
        apply_custom_stage(context, texture, dst.texture(), rect, stage);
        texture = dst.detach();
        stage->unref();
    }
    return texture;
}

void SkGpuDevice::drawSprite(const SkDraw& draw, const SkBitmap& bitmap,
                            int left, int top, const SkPaint& paint) {
    CHECK_SHOULD_DRAW(draw);

    SkAutoLockPixels alp(bitmap, !bitmap.getTexture());
    if (!bitmap.getTexture() && !bitmap.readyToDraw()) {
        return;
    }

    int w = bitmap.width();
    int h = bitmap.height();

    GrPaint grPaint;
    SkAutoCachedTexture colorLutTexture;
    if(!skPaint2GrPaintNoShader(this, paint, true, false, &colorLutTexture, &grPaint)) {
        return;
    }

    GrContext::AutoMatrix avm(fContext, GrMatrix::I());

    GrSamplerState* sampler = grPaint.textureSampler(kBitmapTextureIdx);

    GrTexture* texture;
    sampler->reset();
    SkAutoCachedTexture act(this, bitmap, sampler->textureParams(), &texture);
    grPaint.textureSampler(kBitmapTextureIdx)->setCustomStage(SkNEW_ARGS
        (GrSingleTextureEffect, (texture)))->unref();

    SkImageFilter* filter = paint.getImageFilter();
    if (NULL != filter) {
        GrTexture* filteredTexture = filter_texture(fContext, texture, filter,
                 GrRect::MakeWH(SkIntToScalar(w), SkIntToScalar(h)));
        if (filteredTexture) {
            grPaint.textureSampler(kBitmapTextureIdx)->setCustomStage(SkNEW_ARGS
                (GrSingleTextureEffect, (filteredTexture)))->unref();
            texture = filteredTexture;
            filteredTexture->unref();
        }
    }

    fContext->drawRectToRect(grPaint,
                            GrRect::MakeXYWH(GrIntToScalar(left),
                                            GrIntToScalar(top),
                                            GrIntToScalar(w),
                                            GrIntToScalar(h)),
                            GrRect::MakeWH(GR_Scalar1 * w / texture->width(),
                                        GR_Scalar1 * h / texture->height()));
}

void SkGpuDevice::drawDevice(const SkDraw& draw, SkDevice* device,
                            int x, int y, const SkPaint& paint) {
    
    SkGpuDevice* dev = static_cast<SkGpuDevice*>(device);
    if (dev->fNeedClear) {
        
        dev->clear(0x0);
    }

    CHECK_SHOULD_DRAW(draw);

    GrPaint grPaint;
    SkAutoCachedTexture colorLutTexture;
    grPaint.textureSampler(kBitmapTextureIdx)->reset();
    if (!dev->bindDeviceAsTexture(&grPaint) ||
        !skPaint2GrPaintNoShader(this, paint, true, false, &colorLutTexture, &grPaint)) {
        return;
    }

    GrTexture* devTex = grPaint.getTextureSampler(kBitmapTextureIdx).getCustomStage()->texture(0);
    SkASSERT(NULL != devTex);

    SkImageFilter* filter = paint.getImageFilter();
    if (NULL != filter) {
        GrRect rect = GrRect::MakeWH(SkIntToScalar(devTex->width()),
                                     SkIntToScalar(devTex->height()));
        GrTexture* filteredTexture = filter_texture(fContext, devTex, filter, rect);
        if (filteredTexture) {
            grPaint.textureSampler(kBitmapTextureIdx)->setCustomStage(SkNEW_ARGS
                (GrSingleTextureEffect, (filteredTexture)))->unref();
            devTex = filteredTexture;
            filteredTexture->unref();
        }
    }

    const SkBitmap& bm = dev->accessBitmap(false);
    int w = bm.width();
    int h = bm.height();

    GrContext::AutoMatrix avm(fContext, GrMatrix::I());
    GrRect dstRect = GrRect::MakeXYWH(GrIntToScalar(x),
                                      GrIntToScalar(y),
                                      GrIntToScalar(w),
                                      GrIntToScalar(h));

    
    
    GrRect srcRect = GrRect::MakeWH(GR_Scalar1 * w / devTex->width(),
                                    GR_Scalar1 * h / devTex->height());

    fContext->drawRectToRect(grPaint, dstRect, srcRect);
}

bool SkGpuDevice::canHandleImageFilter(SkImageFilter* filter) {
    if (!filter->asNewCustomStage(NULL, NULL) &&
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
    paint.reset();

    GrSamplerState* sampler = paint.textureSampler(kBitmapTextureIdx);

    GrTexture* texture;
    SkAutoCachedTexture act(this, src, sampler->textureParams(), &texture);

    result->setConfig(src.config(), src.width(), src.height());
    GrRect rect = GrRect::MakeWH(SkIntToScalar(src.width()),
                                 SkIntToScalar(src.height()));
    GrTexture* resultTexture = filter_texture(fContext, texture, filter, rect);
    if (resultTexture) {
        result->setPixelRef(SkNEW_ARGS(SkGrTexturePixelRef,
                                       (resultTexture)))->unref();
        resultTexture->unref();
    }
    return true;
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
    CHECK_SHOULD_DRAW(draw);

    GrPaint grPaint;
    SkAutoCachedTexture textures[GrPaint::kMaxTextures];
    
    if (NULL == texs) {
        if (!skPaint2GrPaintNoShader(this,
                                     paint,
                                     false,
                                     NULL == colors,
                                     &textures[kColorFilterTextureIdx],
                                     &grPaint)) {
            return;
        }
    } else {
        if (!skPaint2GrPaintShader(this,
                                   paint,
                                   NULL == colors,
                                   textures,
                                   &grPaint)) {
            return;
        }
    }

    if (NULL != xmode && NULL != texs && NULL != colors) {
        if (!SkXfermode::IsMode(xmode, SkXfermode::kMultiply_Mode)) {
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
    CHECK_SHOULD_DRAW(draw);

    if (draw.fMatrix->hasPerspective()) {
        
        draw.drawText((const char*)text, byteLength, x, y, paint);
    } else {
        SkDraw myDraw(draw);

        GrPaint grPaint;
        SkAutoCachedTexture textures[GrPaint::kMaxTextures];
        if (!skPaint2GrPaintShader(this,
                                   paint,
                                   true,
                                   textures,
                                   &grPaint)) {
            return;
        }
        GrTextContext context(fContext, grPaint, draw.fExtMatrix);
        myDraw.fProcs = this->initDrawForText(&context);
        this->INHERITED::drawText(myDraw, text, byteLength, x, y, paint);
    }
}

void SkGpuDevice::drawPosText(const SkDraw& draw, const void* text,
                             size_t byteLength, const SkScalar pos[],
                             SkScalar constY, int scalarsPerPos,
                             const SkPaint& paint) {
    CHECK_SHOULD_DRAW(draw);

    if (draw.fMatrix->hasPerspective()) {
        
        draw.drawPosText((const char*)text, byteLength, pos, constY,
                         scalarsPerPos, paint);
    } else {
        SkDraw myDraw(draw);

        GrPaint grPaint;
        SkAutoCachedTexture textures[GrPaint::kMaxTextures];
        if (!skPaint2GrPaintShader(this,
                                   paint,
                                   true,
                                   textures,
                                   &grPaint)) {
            return;
        }
        GrTextContext context(fContext, grPaint, draw.fExtMatrix);
        myDraw.fProcs = this->initDrawForText(&context);
        this->INHERITED::drawPosText(myDraw, text, byteLength, pos, constY,
                                     scalarsPerPos, paint);
    }
}

void SkGpuDevice::drawTextOnPath(const SkDraw& draw, const void* text,
                                size_t len, const SkPath& path,
                                const SkMatrix* m, const SkPaint& paint) {
    CHECK_SHOULD_DRAW(draw);

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
    DO_DEFERRED_CLEAR;
    fContext->resolveRenderTarget(fRenderTarget);
}



bool SkGpuDevice::isBitmapInTextureCache(const SkBitmap& bitmap,
                                         const GrTextureParams& params) const {
    uint64_t key = bitmap.getGenerationID();
    key |= ((uint64_t) bitmap.pixelRefOffset()) << 32;

    GrTextureDesc desc;
    desc.fWidth = bitmap.width();
    desc.fHeight = bitmap.height();
    desc.fConfig = SkBitmapConfig2GrPixelConfig(bitmap.config());

    GrCacheData cacheData(key);

    return this->context()->isTextureInCache(desc, cacheData, &params);
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

    GrTexture* texture;
    SkAutoTUnref<GrTexture> tunref;
    
    bool needClear = !isOpaque;

#if CACHE_COMPATIBLE_DEVICE_TEXTURES
    
    
    GrContext::ScratchTexMatch matchType = (kSaveLayer_Usage == usage) ?
                                    GrContext::kApprox_ScratchTexMatch :
                                    GrContext::kExact_ScratchTexMatch;
    texture = fContext->lockScratchTexture(desc, matchType);
#else
    tunref.reset(fContext->createUncachedTexture(desc, NULL, 0));
    texture = tunref.get();
#endif
    if (texture) {
        return SkNEW_ARGS(SkGpuDevice,(fContext,
                                       texture,
                                       needClear));
    } else {
        GrPrintf("---- failed to create compatible device texture [%d %d]\n",
                    width, height);
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
