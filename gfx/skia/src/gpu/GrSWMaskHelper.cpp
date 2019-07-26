






#include "GrSWMaskHelper.h"
#include "GrDrawState.h"
#include "GrGpu.h"

#include "SkStrokeRec.h"


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




void GrSWMaskHelper::draw(const GrRect& rect, SkRegion::Op op,
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

    SkXfermode* mode = SkXfermode::Create(op_to_mode(op));

    paint.setXfermode(mode);
    paint.setAntiAlias(antiAlias);
    paint.setColor(SkColorSetARGB(alpha, alpha, alpha, alpha));

    fDraw.drawPath(path, paint);

    SkSafeUnref(mode);
}

bool GrSWMaskHelper::init(const GrIRect& resultBounds,
                          const SkMatrix* matrix) {
    if (NULL != matrix) {
        fMatrix = *matrix;
    } else {
        fMatrix.setIdentity();
    }

    
    fMatrix.postTranslate(-resultBounds.fLeft * SK_Scalar1,
                          -resultBounds.fTop * SK_Scalar1);
    GrIRect bounds = GrIRect::MakeWH(resultBounds.width(),
                                     resultBounds.height());

    fBM.setConfig(SkBitmap::kA8_Config, bounds.fRight, bounds.fBottom);
    if (!fBM.allocPixels()) {
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

    texture->set(fContext, desc);
    return NULL != texture->texture();
}




void GrSWMaskHelper::toTexture(GrTexture *texture, uint8_t alpha) {
    SkAutoLockPixels alp(fBM);

    
    
    

    
    
    
    
    GrDrawState::AutoRenderTargetRestore artr(fContext->getGpu()->drawState(),
                                              texture->asRenderTarget());

    fContext->getGpu()->clear(NULL, GrColorPackRGBA(alpha, alpha, alpha, alpha));

    texture->writePixels(0, 0, fBM.width(), fBM.height(),
                         kAlpha_8_GrPixelConfig,
                         fBM.getPixels(), fBM.rowBytes());
}







GrTexture* GrSWMaskHelper::DrawPathMaskToTexture(GrContext* context,
                                                 const SkPath& path,
                                                 const SkStrokeRec& stroke,
                                                 const GrIRect& resultBounds,
                                                 bool antiAlias,
                                                 SkMatrix* matrix) {
    GrAutoScratchTexture ast;

    GrSWMaskHelper helper(context);

    if (!helper.init(resultBounds, matrix)) {
        return NULL;
    }

    helper.draw(path, stroke, SkRegion::kReplace_Op, antiAlias, 0xFF);

    if (!helper.getTexture(&ast)) {
        return NULL;
    }

    helper.toTexture(ast.texture(), 0x00);

    return ast.detach();
}

void GrSWMaskHelper::DrawToTargetWithPathMask(GrTexture* texture,
                                              GrDrawTarget* target,
                                              const GrIRect& rect) {
    GrDrawState* drawState = target->drawState();

    GrDrawState::AutoDeviceCoordDraw adcd(drawState);
    if (!adcd.succeeded()) {
        return;
    }
    enum {
        
        
        
        kPathMaskStage = GrPaint::kTotalStages,
    };

    GrRect dstRect = GrRect::MakeLTRB(
                            SK_Scalar1 * rect.fLeft,
                            SK_Scalar1 * rect.fTop,
                            SK_Scalar1 * rect.fRight,
                            SK_Scalar1 * rect.fBottom);

    
    
    
    
    SkMatrix maskMatrix;
    maskMatrix.setIDiv(texture->width(), texture->height());
    maskMatrix.preTranslate(SkIntToScalar(-rect.fLeft), SkIntToScalar(-rect.fTop));
    maskMatrix.preConcat(drawState->getViewMatrix());

    GrAssert(!drawState->isStageEnabled(kPathMaskStage));
    drawState->setEffect(kPathMaskStage,
                         GrSimpleTextureEffect::Create(texture,
                                                       maskMatrix,
                                                       false,
                                                       GrEffect::kPosition_CoordsType))->unref();

    target->drawSimpleRect(dstRect);
    drawState->disableStage(kPathMaskStage);
}
