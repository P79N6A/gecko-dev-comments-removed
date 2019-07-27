






#include "GrSWMaskHelper.h"

#include "GrDrawState.h"
#include "GrDrawTargetCaps.h"
#include "GrGpu.h"

#include "SkData.h"
#include "SkStrokeRec.h"
#include "SkTextureCompressor.h"


#include "GrContext.h"

namespace {




SkXfermode::Mode op_to_mode(SkRegion::Op op) {

    static const SkXfermode::Mode modeMap[] = {
        SkXfermode::kDstOut_Mode,   
        SkXfermode::kModulate_Mode, 
        SkXfermode::kSrcOver_Mode,  
        SkXfermode::kXor_Mode,      
        SkXfermode::kClear_Mode,    
        SkXfermode::kSrc_Mode,      
    };

    return modeMap[op];
}

}




void GrSWMaskHelper::draw(const SkRect& rect, SkRegion::Op op,
                          bool antiAlias, uint8_t alpha) {
    SkPaint paint;

    SkXfermode* mode = SkXfermode::Create(op_to_mode(op));

    paint.setXfermode(mode);
    paint.setAntiAlias(antiAlias);
    paint.setColor(SkColorSetARGB(alpha, alpha, alpha, alpha));

    fDraw.drawRect(rect, paint);

    SkSafeUnref(mode);
}




void GrSWMaskHelper::draw(const SkPath& path, const SkStrokeRec& stroke, SkRegion::Op op,
                          bool antiAlias, uint8_t alpha) {

    SkPaint paint;
    if (stroke.isHairlineStyle()) {
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(SK_Scalar1);
    } else {
        if (stroke.isFillStyle()) {
            paint.setStyle(SkPaint::kFill_Style);
        } else {
            paint.setStyle(SkPaint::kStroke_Style);
            paint.setStrokeJoin(stroke.getJoin());
            paint.setStrokeCap(stroke.getCap());
            paint.setStrokeWidth(stroke.getWidth());
        }
    }
    paint.setAntiAlias(antiAlias);

    if (SkRegion::kReplace_Op == op && 0xFF == alpha) {
        SkASSERT(0xFF == paint.getAlpha());
        fDraw.drawPathCoverage(path, paint);
    } else {
        paint.setXfermodeMode(op_to_mode(op));
        paint.setColor(SkColorSetARGB(alpha, alpha, alpha, alpha));
        fDraw.drawPath(path, paint);
    }
}

bool GrSWMaskHelper::init(const SkIRect& resultBounds,
                          const SkMatrix* matrix) {
    if (NULL != matrix) {
        fMatrix = *matrix;
    } else {
        fMatrix.setIdentity();
    }

    
    fMatrix.postTranslate(-resultBounds.fLeft * SK_Scalar1,
                          -resultBounds.fTop * SK_Scalar1);
    SkIRect bounds = SkIRect::MakeWH(resultBounds.width(),
                                     resultBounds.height());
#if GR_COMPRESS_ALPHA_MASK
    
    
    const int cmpWidth = (bounds.fRight + 15) & ~15;
    const int cmpHeight = (bounds.fBottom + 3) & ~3;
#else
    const int cmpWidth = bounds.fRight;
    const int cmpHeight = bounds.fBottom;
#endif

    if (!fBM.allocPixels(SkImageInfo::MakeA8(cmpWidth, cmpHeight))) {
        return false;
    }

    sk_bzero(fBM.getPixels(), fBM.getSafeSize());

    sk_bzero(&fDraw, sizeof(fDraw));
    fRasterClip.setRect(bounds);
    fDraw.fRC    = &fRasterClip;
    fDraw.fClip  = &fRasterClip.bwRgn();
    fDraw.fMatrix = &fMatrix;
    fDraw.fBitmap = &fBM;
    return true;
}





bool GrSWMaskHelper::getTexture(GrAutoScratchTexture* texture) {
    GrTextureDesc desc;
    desc.fWidth = fBM.width();
    desc.fHeight = fBM.height();
    desc.fConfig = kAlpha_8_GrPixelConfig;

#if GR_COMPRESS_ALPHA_MASK
    static const int kCompressedBlockSize = 4;
    static const GrPixelConfig kCompressedConfig = kR11_EAC_GrPixelConfig;

    if (desc.fWidth % kCompressedBlockSize == 0 &&
        desc.fHeight % kCompressedBlockSize == 0) {
        desc.fConfig = kCompressedConfig;
    }

    
    if (!(fContext->getGpu()->caps()->isConfigTexturable(desc.fConfig))) {
        desc.fConfig = kAlpha_8_GrPixelConfig;
    }
#endif

    texture->set(fContext, desc);
    return NULL != texture->texture();
}

void GrSWMaskHelper::sendTextureData(GrTexture *texture, const GrTextureDesc& desc,
                                     const void *data, int rowbytes) {
    
    
    bool reuseScratch = fContext->getGpu()->caps()->reuseScratchTextures();

    
    
    SkASSERT(NULL == texture->asRenderTarget());

    texture->writePixels(0, 0, desc.fWidth, desc.fHeight,
                         desc.fConfig, data, rowbytes,
                         reuseScratch ? 0 : GrContext::kDontFlush_PixelOpsFlag);
}

void GrSWMaskHelper::compressTextureData(GrTexture *texture, const GrTextureDesc& desc) {

    SkASSERT(GrPixelConfigIsCompressed(desc.fConfig));

    SkTextureCompressor::Format format = SkTextureCompressor::kLATC_Format;

    
    switch(desc.fConfig) {
        case kLATC_GrPixelConfig:
            format = SkTextureCompressor::kLATC_Format;
            break;
        case kR11_EAC_GrPixelConfig:
            format = SkTextureCompressor::kR11_EAC_Format;
            break;
        default:
            SkFAIL("Unrecognized texture compression format.");
            break;
    }

    SkAutoDataUnref cmpData(SkTextureCompressor::CompressBitmapToFormat(fBM, format));
    SkASSERT(NULL != cmpData);

    this->sendTextureData(texture, desc, cmpData->data(), 0);
}




void GrSWMaskHelper::toTexture(GrTexture *texture) {
    SkAutoLockPixels alp(fBM);

    GrTextureDesc desc;
    desc.fWidth = fBM.width();
    desc.fHeight = fBM.height();
    desc.fConfig = texture->config();
        
    
    if (GrPixelConfigIsCompressed(texture->config())) {
        this->compressTextureData(texture, desc);
    } else {
        
        this->sendTextureData(texture, desc, fBM.getPixels(), fBM.rowBytes());
    }
}







GrTexture* GrSWMaskHelper::DrawPathMaskToTexture(GrContext* context,
                                                 const SkPath& path,
                                                 const SkStrokeRec& stroke,
                                                 const SkIRect& resultBounds,
                                                 bool antiAlias,
                                                 SkMatrix* matrix) {
    GrSWMaskHelper helper(context);

    if (!helper.init(resultBounds, matrix)) {
        return NULL;
    }

    helper.draw(path, stroke, SkRegion::kReplace_Op, antiAlias, 0xFF);

    GrAutoScratchTexture ast;
    if (!helper.getTexture(&ast)) {
        return NULL;
    }

    helper.toTexture(ast.texture());

    return ast.detach();
}

void GrSWMaskHelper::DrawToTargetWithPathMask(GrTexture* texture,
                                              GrDrawTarget* target,
                                              const SkIRect& rect) {
    GrDrawState* drawState = target->drawState();

    GrDrawState::AutoViewMatrixRestore avmr;
    if (!avmr.setIdentity(drawState)) {
        return;
    }
    GrDrawState::AutoRestoreEffects are(drawState);

    SkRect dstRect = SkRect::MakeLTRB(SK_Scalar1 * rect.fLeft,
                                      SK_Scalar1 * rect.fTop,
                                      SK_Scalar1 * rect.fRight,
                                      SK_Scalar1 * rect.fBottom);

    
    
    
    
    SkMatrix maskMatrix;
    maskMatrix.setIDiv(texture->width(), texture->height());
    maskMatrix.preTranslate(SkIntToScalar(-rect.fLeft), SkIntToScalar(-rect.fTop));
    maskMatrix.preConcat(drawState->getViewMatrix());

    drawState->addCoverageEffect(
                         GrSimpleTextureEffect::Create(texture,
                                                       maskMatrix,
                                                       GrTextureParams::kNone_FilterMode,
                                                       kPosition_GrCoordSet))->unref();

    target->drawSimpleRect(dstRect);
}
