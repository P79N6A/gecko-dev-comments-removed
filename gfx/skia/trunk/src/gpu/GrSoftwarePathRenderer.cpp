







#include "GrSoftwarePathRenderer.h"
#include "GrContext.h"
#include "GrSWMaskHelper.h"


bool GrSoftwarePathRenderer::canDrawPath(const SkPath&,
                                         const SkStrokeRec&,
                                         const GrDrawTarget*,
                                         bool antiAlias) const {
    if (NULL == fContext) {
        return false;
    }

    return true;
}

GrPathRenderer::StencilSupport GrSoftwarePathRenderer::onGetStencilSupport(
    const SkPath&,
    const SkStrokeRec&,
    const GrDrawTarget*) const {
    return GrPathRenderer::kNoSupport_StencilSupport;
}

namespace {





bool get_path_and_clip_bounds(const GrDrawTarget* target,
                              const SkPath& path,
                              const SkMatrix& matrix,
                              SkIRect* devPathBounds,
                              SkIRect* devClipBounds) {
    
    const GrRenderTarget* rt = target->getDrawState().getRenderTarget();
    if (NULL == rt) {
        return false;
    }
    *devPathBounds = SkIRect::MakeWH(rt->width(), rt->height());

    target->getClip()->getConservativeBounds(rt, devClipBounds);

    
    
    if (!devPathBounds->intersect(*devClipBounds)) {
        return false;
    }

    if (!path.getBounds().isEmpty()) {
        SkRect pathSBounds;
        matrix.mapRect(&pathSBounds, path.getBounds());
        SkIRect pathIBounds;
        pathSBounds.roundOut(&pathIBounds);
        if (!devPathBounds->intersect(pathIBounds)) {
            
            *devPathBounds = pathIBounds;
            return false;
        }
    } else {
        *devPathBounds = SkIRect::EmptyIRect();
        return false;
    }
    return true;
}


void draw_around_inv_path(GrDrawTarget* target,
                          const SkIRect& devClipBounds,
                          const SkIRect& devPathBounds) {
    GrDrawState::AutoViewMatrixRestore avmr;
    if (!avmr.setIdentity(target->drawState())) {
        return;
    }
    SkRect rect;
    if (devClipBounds.fTop < devPathBounds.fTop) {
        rect.iset(devClipBounds.fLeft, devClipBounds.fTop,
                  devClipBounds.fRight, devPathBounds.fTop);
        target->drawSimpleRect(rect, NULL);
    }
    if (devClipBounds.fLeft < devPathBounds.fLeft) {
        rect.iset(devClipBounds.fLeft, devPathBounds.fTop,
                  devPathBounds.fLeft, devPathBounds.fBottom);
        target->drawSimpleRect(rect, NULL);
    }
    if (devClipBounds.fRight > devPathBounds.fRight) {
        rect.iset(devPathBounds.fRight, devPathBounds.fTop,
                  devClipBounds.fRight, devPathBounds.fBottom);
        target->drawSimpleRect(rect, NULL);
    }
    if (devClipBounds.fBottom > devPathBounds.fBottom) {
        rect.iset(devClipBounds.fLeft, devPathBounds.fBottom,
                  devClipBounds.fRight, devClipBounds.fBottom);
        target->drawSimpleRect(rect, NULL);
    }
}

}



bool GrSoftwarePathRenderer::onDrawPath(const SkPath& path,
                                        const SkStrokeRec& stroke,
                                        GrDrawTarget* target,
                                        bool antiAlias) {

    if (NULL == fContext) {
        return false;
    }

    GrDrawState* drawState = target->drawState();

    SkMatrix vm = drawState->getViewMatrix();

    SkIRect devPathBounds, devClipBounds;
    if (!get_path_and_clip_bounds(target, path, vm,
                                  &devPathBounds, &devClipBounds)) {
        if (path.isInverseFillType()) {
            draw_around_inv_path(target, devClipBounds, devPathBounds);
        }
        return true;
    }

    SkAutoTUnref<GrTexture> texture(
            GrSWMaskHelper::DrawPathMaskToTexture(fContext, path, stroke,
                                                  devPathBounds,
                                                  antiAlias, &vm));
    if (NULL == texture) {
        return false;
    }

    GrSWMaskHelper::DrawToTargetWithPathMask(texture, target, devPathBounds);

    if (path.isInverseFillType()) {
        draw_around_inv_path(target, devClipBounds, devPathBounds);
    }

    return true;
}
