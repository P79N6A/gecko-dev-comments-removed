







#include "GrSoftwarePathRenderer.h"
#include "GrContext.h"
#include "GrSWMaskHelper.h"


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





bool get_path_and_clip_bounds(const GrDrawTarget* target,
                              const SkPath& path,
                              const GrMatrix& matrix,
                              GrIRect* devPathBounds,
                              GrIRect* devClipBounds) {
    
    const GrRenderTarget* rt = target->getDrawState().getRenderTarget();
    if (NULL == rt) {
        return false;
    }
    *devPathBounds = GrIRect::MakeWH(rt->width(), rt->height());

    target->getClip()->getConservativeBounds(rt, devClipBounds);

    
    
    if (!devPathBounds->intersect(*devClipBounds)) {
        return false;
    }

    if (!path.getBounds().isEmpty()) {
        GrRect pathSBounds;
        matrix.mapRect(&pathSBounds, path.getBounds());
        GrIRect pathIBounds;
        pathSBounds.roundOut(&pathIBounds);
        if (!devPathBounds->intersect(pathIBounds)) {
            
            *devPathBounds = pathIBounds;
            return false;
        }
    } else {
        *devPathBounds = GrIRect::EmptyIRect();
        return false;
    }
    return true;
}


void draw_around_inv_path(GrDrawTarget* target,
                          const GrIRect& devClipBounds,
                          const GrIRect& devPathBounds) {
    GrDrawTarget::AutoDeviceCoordDraw adcd(target);
    if (!adcd.succeeded()) {
        return;
    }
    GrRect rect;
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
                                        GrPathFill fill,
                                        const GrVec* translate,
                                        GrDrawTarget* target,
                                        bool antiAlias) {

    if (NULL == fContext) {
        return false;
    }

    GrDrawState* drawState = target->drawState();

    GrMatrix vm = drawState->getViewMatrix();
    if (NULL != translate) {
        vm.postTranslate(translate->fX, translate->fY);
    }

    GrIRect devPathBounds, devClipBounds;
    if (!get_path_and_clip_bounds(target, path, vm,
                                  &devPathBounds, &devClipBounds)) {
        if (GrIsFillInverted(fill)) {
            draw_around_inv_path(target, devClipBounds, devPathBounds);
        }
        return true;
    }

    SkAutoTUnref<GrTexture> texture(
            GrSWMaskHelper::DrawPathMaskToTexture(fContext, path,
                                                  devPathBounds, fill,
                                                  antiAlias, &vm));
    if (NULL == texture) {
        return false;
    }

    GrSWMaskHelper::DrawToTargetWithPathMask(texture, target, devPathBounds);

    if (GrIsFillInverted(fill)) {
        draw_around_inv_path(target, devClipBounds, devPathBounds);
    }

    return true;
}
