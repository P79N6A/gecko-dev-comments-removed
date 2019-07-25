







#include "GrSoftwarePathRenderer.h"
#include "GrPaint.h"
#include "SkPaint.h"
#include "GrRenderTarget.h" 
#include "GrContext.h"
#include "SkDraw.h"
#include "SkRasterClip.h"


bool GrSoftwarePathRenderer::canDrawPath(const SkPath& path,
                                         GrPathFill fill,
                                         const GrDrawTarget* target,
                                         bool antiAlias) const {
    if (!antiAlias || NULL == fContext) {
        
        
        
        
        
        
        return false;
    }

    return true;
}

namespace {


SkPath::FillType gr_fill_to_sk_fill(GrPathFill fill) {
    switch (fill) {
        case kWinding_PathFill:
            return SkPath::kWinding_FillType;
        case kEvenOdd_PathFill:
            return SkPath::kEvenOdd_FillType;
        case kInverseWinding_PathFill:
            return SkPath::kInverseWinding_FillType;
        case kInverseEvenOdd_PathFill:
            return SkPath::kInverseEvenOdd_FillType;
        default:
            GrCrash("Unexpected fill.");
            return SkPath::kWinding_FillType;
    }
}





bool get_path_and_clip_bounds(const GrDrawTarget* target,
                              const SkPath& path,
                              const GrVec* translate,
                              GrIRect* pathBounds,
                              GrIRect* clipBounds) {
    
    const GrRenderTarget* rt = target->getDrawState().getRenderTarget();
    if (NULL == rt) {
        return false;
    }
    *pathBounds = GrIRect::MakeWH(rt->width(), rt->height());
    const GrClip& clip = target->getClip();
    if (clip.hasConservativeBounds()) {
        clip.getConservativeBounds().roundOut(clipBounds);
        if (!pathBounds->intersect(*clipBounds)) {
            return false;
        }
    } else {
        
        *clipBounds = *pathBounds;
    }
    GrRect pathSBounds = path.getBounds();
    if (!pathSBounds.isEmpty()) {
        if (NULL != translate) {
            pathSBounds.offset(*translate);
        }
        target->getDrawState().getViewMatrix().mapRect(&pathSBounds,
                                                        pathSBounds);
        GrIRect pathIBounds;
        pathSBounds.roundOut(&pathIBounds);
        if (!pathBounds->intersect(pathIBounds)) {
            return false;
        }
    } else {
        return false;
    }
    return true;
}





SkXfermode::Mode op_to_mode(SkRegion::Op op) {

    static const SkXfermode::Mode modeMap[] = {
        SkXfermode::kDstOut_Mode,   
        SkXfermode::kMultiply_Mode, 
        SkXfermode::kSrcOver_Mode,  
        SkXfermode::kXor_Mode,      
        SkXfermode::kClear_Mode,    
        SkXfermode::kSrc_Mode,      
    };

    return modeMap[op];
}

}




void GrSWMaskHelper::draw(const GrRect& clientRect, SkRegion::Op op, 
                          bool antiAlias, GrColor color) {
    SkPaint paint;

    SkXfermode* mode = SkXfermode::Create(op_to_mode(op));

    paint.setXfermode(mode);
    paint.setAntiAlias(antiAlias);
    paint.setColor(color);

    fDraw.drawRect(clientRect, paint);

    SkSafeUnref(mode);
}




void GrSWMaskHelper::draw(const SkPath& clientPath, SkRegion::Op op,
                          GrPathFill fill, bool antiAlias, GrColor color) {

    SkPaint paint;
    SkPath tmpPath;
    const SkPath* pathToDraw = &clientPath;
    if (kHairLine_PathFill == fill) {
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(SK_Scalar1);
    } else {
        paint.setStyle(SkPaint::kFill_Style);
        SkPath::FillType skfill = gr_fill_to_sk_fill(fill);
        if (skfill != pathToDraw->getFillType()) {
            tmpPath = *pathToDraw;
            tmpPath.setFillType(skfill);
            pathToDraw = &tmpPath;
        }
    }
    SkXfermode* mode = SkXfermode::Create(op_to_mode(op));

    paint.setXfermode(mode);
    paint.setAntiAlias(antiAlias);
    paint.setColor(color);

    fDraw.drawPath(*pathToDraw, paint);

    SkSafeUnref(mode);
}

bool GrSWMaskHelper::init(const GrIRect& pathDevBounds, 
                          const GrPoint* translate,
                          bool useMatrix) {
    if (useMatrix) {    
        fMatrix = fContext->getMatrix();
    } else {
        fMatrix.setIdentity();
    }

    if (NULL != translate) {
        fMatrix.postTranslate(translate->fX, translate->fY);
    }

    fMatrix.postTranslate(-pathDevBounds.fLeft * SK_Scalar1,
                          -pathDevBounds.fTop * SK_Scalar1);
    GrIRect bounds = GrIRect::MakeWH(pathDevBounds.width(),
                                     pathDevBounds.height());

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




bool GrSWMaskHelper::getTexture(GrAutoScratchTexture* tex) {
    const GrTextureDesc desc = {
        kNone_GrTextureFlags,
        fBM.width(),
        fBM.height(),
        kAlpha_8_GrPixelConfig,
        0 
    };

    tex->set(fContext, desc);
    GrTexture* texture = tex->texture();

    if (NULL == texture) {
        return false;
    }

    return true;
}




void GrSWMaskHelper::toTexture(GrTexture *texture) {
    SkAutoLockPixels alp(fBM);
    texture->writePixels(0, 0, fBM.width(), fBM.height(), 
                         kAlpha_8_GrPixelConfig,
                         fBM.getPixels(), fBM.rowBytes());
}

namespace {





bool sw_draw_path_to_mask_texture(const SkPath& clientPath,
                                  const GrIRect& pathDevBounds,
                                  GrPathFill fill,
                                  GrContext* context,
                                  const GrPoint* translate,
                                  GrAutoScratchTexture* tex,
                                  bool antiAlias) {
    GrSWMaskHelper helper(context);

    if (!helper.init(pathDevBounds, translate, true)) {
        return false;
    }

    helper.draw(clientPath, SkRegion::kReplace_Op, 
                fill, antiAlias, SK_ColorWHITE);

    if (!helper.getTexture(tex)) {
        return false;
    }

    helper.toTexture(tex->texture());

    return true;
}


void draw_around_inv_path(GrDrawTarget* target,
                          GrDrawState::StageMask stageMask,
                          const GrIRect& clipBounds,
                          const GrIRect& pathBounds) {
    GrDrawTarget::AutoDeviceCoordDraw adcd(target, stageMask);
    GrRect rect;
    if (clipBounds.fTop < pathBounds.fTop) {
        rect.iset(clipBounds.fLeft, clipBounds.fTop, 
                    clipBounds.fRight, pathBounds.fTop);
        target->drawSimpleRect(rect, NULL, stageMask);
    }
    if (clipBounds.fLeft < pathBounds.fLeft) {
        rect.iset(clipBounds.fLeft, pathBounds.fTop, 
                    pathBounds.fLeft, pathBounds.fBottom);
        target->drawSimpleRect(rect, NULL, stageMask);
    }
    if (clipBounds.fRight > pathBounds.fRight) {
        rect.iset(pathBounds.fRight, pathBounds.fTop, 
                    clipBounds.fRight, pathBounds.fBottom);
        target->drawSimpleRect(rect, NULL, stageMask);
    }
    if (clipBounds.fBottom > pathBounds.fBottom) {
        rect.iset(clipBounds.fLeft, pathBounds.fBottom, 
                    clipBounds.fRight, clipBounds.fBottom);
        target->drawSimpleRect(rect, NULL, stageMask);
    }
}

}



bool GrSoftwarePathRenderer::onDrawPath(const SkPath& path,
                                        GrPathFill fill,
                                        const GrVec* translate,
                                        GrDrawTarget* target,
                                        GrDrawState::StageMask stageMask,
                                        bool antiAlias) {

    if (NULL == fContext) {
        return false;
    }

    GrAutoScratchTexture ast;
    GrIRect pathBounds, clipBounds;
    if (!get_path_and_clip_bounds(target, path, translate,
                                  &pathBounds, &clipBounds)) {
        return true;    
    }
    if (sw_draw_path_to_mask_texture(path, pathBounds,
                                     fill, fContext,
                                     translate, &ast, antiAlias)) {
        GrTexture* texture = ast.texture();
        GrAssert(NULL != texture);
        GrDrawTarget::AutoDeviceCoordDraw adcd(target, stageMask);
        enum {
            
            
            kPathMaskStage = GrPaint::kTotalStages,
        };
        GrAssert(NULL == target->drawState()->getTexture(kPathMaskStage));
        target->drawState()->setTexture(kPathMaskStage, texture);
        target->drawState()->sampler(kPathMaskStage)->reset();
        GrScalar w = GrIntToScalar(pathBounds.width());
        GrScalar h = GrIntToScalar(pathBounds.height());
        GrRect maskRect = GrRect::MakeWH(w / texture->width(),
                                         h / texture->height());
        const GrRect* srcRects[GrDrawState::kNumStages] = {NULL};
        srcRects[kPathMaskStage] = &maskRect;
        stageMask |= 1 << kPathMaskStage;
        GrRect dstRect = GrRect::MakeLTRB(
                              SK_Scalar1* pathBounds.fLeft,
                              SK_Scalar1* pathBounds.fTop,
                              SK_Scalar1* pathBounds.fRight,
                              SK_Scalar1* pathBounds.fBottom);
        target->drawRect(dstRect, NULL, stageMask, srcRects, NULL);
        target->drawState()->setTexture(kPathMaskStage, NULL);
        if (GrIsFillInverted(fill)) {
            draw_around_inv_path(target, stageMask,
                                 clipBounds, pathBounds);
        }
        return true;
    }

    return false;
}
