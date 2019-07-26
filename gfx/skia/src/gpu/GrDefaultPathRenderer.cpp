







#include "GrDefaultPathRenderer.h"

#include "GrContext.h"
#include "GrDrawState.h"
#include "GrPathUtils.h"
#include "SkString.h"
#include "SkStrokeRec.h"
#include "SkTrace.h"


GrDefaultPathRenderer::GrDefaultPathRenderer(bool separateStencilSupport,
                                             bool stencilWrapOpsSupport)
    : fSeparateStencil(separateStencilSupport)
    , fStencilWrapOps(stencilWrapOpsSupport) {
}







GR_STATIC_CONST_SAME_STENCIL(gEOStencilPass,
    kInvert_StencilOp,
    kKeep_StencilOp,
    kAlwaysIfInClip_StencilFunc,
    0xffff,
    0xffff,
    0xffff);


GR_STATIC_CONST_SAME_STENCIL(gEOColorPass,
    kZero_StencilOp,
    kZero_StencilOp,
    kNotEqual_StencilFunc,
    0xffff,
    0x0000,
    0xffff);


GR_STATIC_CONST_SAME_STENCIL(gInvEOColorPass,
    kZero_StencilOp,
    kZero_StencilOp,
    kEqualIfInClip_StencilFunc,
    0xffff,
    0x0000,
    0xffff);







GR_STATIC_CONST_STENCIL(gWindStencilSeparateWithWrap,
    kIncWrap_StencilOp,             kDecWrap_StencilOp,
    kKeep_StencilOp,                kKeep_StencilOp,
    kAlwaysIfInClip_StencilFunc,    kAlwaysIfInClip_StencilFunc,
    0xffff,                         0xffff,
    0xffff,                         0xffff,
    0xffff,                         0xffff);





GR_STATIC_CONST_STENCIL(gWindStencilSeparateNoWrap,
    kInvert_StencilOp,              kInvert_StencilOp,
    kIncClamp_StencilOp,            kDecClamp_StencilOp,
    kEqual_StencilFunc,             kEqual_StencilFunc,
    0xffff,                         0xffff,
    0xffff,                         0x0000,
    0xffff,                         0xffff);





GR_STATIC_CONST_SAME_STENCIL(gWindSingleStencilWithWrapInc,
    kIncWrap_StencilOp,
    kKeep_StencilOp,
    kAlwaysIfInClip_StencilFunc,
    0xffff,
    0xffff,
    0xffff);

GR_STATIC_CONST_SAME_STENCIL(gWindSingleStencilWithWrapDec,
    kDecWrap_StencilOp,
    kKeep_StencilOp,
    kAlwaysIfInClip_StencilFunc,
    0xffff,
    0xffff,
    0xffff);

GR_STATIC_CONST_SAME_STENCIL(gWindSingleStencilNoWrapInc,
    kInvert_StencilOp,
    kIncClamp_StencilOp,
    kEqual_StencilFunc,
    0xffff,
    0xffff,
    0xffff);

GR_STATIC_CONST_SAME_STENCIL(gWindSingleStencilNoWrapDec,
    kInvert_StencilOp,
    kDecClamp_StencilOp,
    kEqual_StencilFunc,
    0xffff,
    0x0000,
    0xffff);



GR_STATIC_CONST_SAME_STENCIL(gWindColorPass,
    kZero_StencilOp,
    kZero_StencilOp,
    kNonZeroIfInClip_StencilFunc,
    0xffff,
    0x0000,
    0xffff);

GR_STATIC_CONST_SAME_STENCIL(gInvWindColorPass,
    kZero_StencilOp,
    kZero_StencilOp,
    kEqualIfInClip_StencilFunc,
    0xffff,
    0x0000,
    0xffff);





GR_STATIC_CONST_SAME_STENCIL(gDirectToStencil,
    kZero_StencilOp,
    kIncClamp_StencilOp,
    kAlwaysIfInClip_StencilFunc,
    0xffff,
    0x0000,
    0xffff);




#define STENCIL_OFF     0   // Always disable stencil (even when needed)

static inline bool single_pass_path(const SkPath& path, const SkStrokeRec& stroke) {
#if STENCIL_OFF
    return true;
#else
    if (!stroke.isHairlineStyle() && !path.isInverseFillType()) {
        return path.isConvex();
    }
    return false;
#endif
}

GrPathRenderer::StencilSupport GrDefaultPathRenderer::onGetStencilSupport(
                                                            const SkPath& path,
                                                            const SkStrokeRec& stroke,
                                                            const GrDrawTarget*) const {
    if (single_pass_path(path, stroke)) {
        return GrPathRenderer::kNoRestriction_StencilSupport;
    } else {
        return GrPathRenderer::kStencilOnly_StencilSupport;
    }
}

static inline void append_countour_edge_indices(bool hairLine,
                                                uint16_t fanCenterIdx,
                                                uint16_t edgeV0Idx,
                                                uint16_t** indices) {
    
    
    
    if (!hairLine) {
        *((*indices)++) = fanCenterIdx;
    }
    *((*indices)++) = edgeV0Idx;
    *((*indices)++) = edgeV0Idx + 1;
}

bool GrDefaultPathRenderer::createGeom(const SkPath& path,
                                       const SkStrokeRec& stroke,
                                       SkScalar srcSpaceTol,
                                       GrDrawTarget* target,
                                       GrPrimitiveType* primType,
                                       int* vertexCnt,
                                       int* indexCnt,
                                       GrDrawTarget::AutoReleaseGeometry* arg) {
    {
    SK_TRACE_EVENT0("GrDefaultPathRenderer::createGeom");

    SkScalar srcSpaceTolSqd = SkScalarMul(srcSpaceTol, srcSpaceTol);
    int contourCnt;
    int maxPts = GrPathUtils::worstCasePointCount(path, &contourCnt,
                                                  srcSpaceTol);

    if (maxPts <= 0) {
        return false;
    }
    if (maxPts > ((int)SK_MaxU16 + 1)) {
        GrPrintf("Path not rendered, too many verts (%d)\n", maxPts);
        return false;
    }

    bool indexed = contourCnt > 1;

    const bool isHairline = stroke.isHairlineStyle();

    int maxIdxs = 0;
    if (isHairline) {
        if (indexed) {
            maxIdxs = 2 * maxPts;
            *primType = kLines_GrPrimitiveType;
        } else {
            *primType = kLineStrip_GrPrimitiveType;
        }
    } else {
        if (indexed) {
            maxIdxs = 3 * maxPts;
            *primType = kTriangles_GrPrimitiveType;
        } else {
            *primType = kTriangleFan_GrPrimitiveType;
        }
    }

    target->drawState()->setDefaultVertexAttribs();
    if (!arg->set(target, maxPts, maxIdxs)) {
        return false;
    }

    uint16_t* idxBase = reinterpret_cast<uint16_t*>(arg->indices());
    uint16_t* idx = idxBase;
    uint16_t subpathIdxStart = 0;

    GrPoint* base = reinterpret_cast<GrPoint*>(arg->vertices());
    GrAssert(NULL != base);
    GrPoint* vert = base;

    GrPoint pts[4];

    bool first = true;
    int subpath = 0;

    SkPath::Iter iter(path, false);

    for (;;) {
        GrPathCmd cmd = (GrPathCmd)iter.next(pts);
        switch (cmd) {
            case kMove_PathCmd:
                if (!first) {
                    uint16_t currIdx = (uint16_t) (vert - base);
                    subpathIdxStart = currIdx;
                    ++subpath;
                }
                *vert = pts[0];
                vert++;
                break;
            case kLine_PathCmd:
                if (indexed) {
                    uint16_t prevIdx = (uint16_t)(vert - base) - 1;
                    append_countour_edge_indices(isHairline, subpathIdxStart,
                                                 prevIdx, &idx);
                }
                *(vert++) = pts[1];
                break;
            case kQuadratic_PathCmd: {
                
                uint16_t firstQPtIdx = (uint16_t)(vert - base) - 1;
                uint16_t numPts =  (uint16_t)
                    GrPathUtils::generateQuadraticPoints(
                            pts[0], pts[1], pts[2],
                            srcSpaceTolSqd, &vert,
                            GrPathUtils::quadraticPointCount(pts, srcSpaceTol));
                if (indexed) {
                    for (uint16_t i = 0; i < numPts; ++i) {
                        append_countour_edge_indices(isHairline, subpathIdxStart,
                                                     firstQPtIdx + i, &idx);
                    }
                }
                break;
            }
            case kCubic_PathCmd: {
                
                uint16_t firstCPtIdx = (uint16_t)(vert - base) - 1;
                uint16_t numPts = (uint16_t) GrPathUtils::generateCubicPoints(
                                pts[0], pts[1], pts[2], pts[3],
                                srcSpaceTolSqd, &vert,
                                GrPathUtils::cubicPointCount(pts, srcSpaceTol));
                if (indexed) {
                    for (uint16_t i = 0; i < numPts; ++i) {
                        append_countour_edge_indices(isHairline, subpathIdxStart,
                                                     firstCPtIdx + i, &idx);
                    }
                }
                break;
            }
            case kClose_PathCmd:
                break;
            case kEnd_PathCmd:
             
                goto FINISHED;
        }
        first = false;
    }
FINISHED:
    GrAssert((vert - base) <= maxPts);
    GrAssert((idx - idxBase) <= maxIdxs);

    *vertexCnt = vert - base;
    *indexCnt = idx - idxBase;

    }
    return true;
}

bool GrDefaultPathRenderer::internalDrawPath(const SkPath& path,
                                             const SkStrokeRec& stroke,
                                             GrDrawTarget* target,
                                             bool stencilOnly) {

    SkMatrix viewM = target->getDrawState().getViewMatrix();
    SkScalar tol = SK_Scalar1;
    tol = GrPathUtils::scaleToleranceToSrc(tol, viewM, path.getBounds());

    int vertexCnt;
    int indexCnt;
    GrPrimitiveType primType;
    GrDrawTarget::AutoReleaseGeometry arg;
    if (!this->createGeom(path,
                          stroke,
                          tol,
                          target,
                          &primType,
                          &vertexCnt,
                          &indexCnt,
                          &arg)) {
        return false;
    }

    GrAssert(NULL != target);
    GrDrawTarget::AutoStateRestore asr(target, GrDrawTarget::kPreserve_ASRInit);
    GrDrawState* drawState = target->drawState();
    bool colorWritesWereDisabled = drawState->isColorWriteDisabled();
    
    GrAssert(GrDrawState::kBoth_DrawFace == drawState->getDrawFace());

    int                         passCount = 0;
    const GrStencilSettings*    passes[3];
    GrDrawState::DrawFace       drawFace[3];
    bool                        reverse = false;
    bool                        lastPassIsBounds;

    if (stroke.isHairlineStyle()) {
        passCount = 1;
        if (stencilOnly) {
            passes[0] = &gDirectToStencil;
        } else {
            passes[0] = NULL;
        }
        lastPassIsBounds = false;
        drawFace[0] = GrDrawState::kBoth_DrawFace;
    } else {
        if (single_pass_path(path, stroke)) {
            passCount = 1;
            if (stencilOnly) {
                passes[0] = &gDirectToStencil;
            } else {
                passes[0] = NULL;
            }
            drawFace[0] = GrDrawState::kBoth_DrawFace;
            lastPassIsBounds = false;
        } else {
            switch (path.getFillType()) {
                case SkPath::kInverseEvenOdd_FillType:
                    reverse = true;
                    
                case SkPath::kEvenOdd_FillType:
                    passes[0] = &gEOStencilPass;
                    if (stencilOnly) {
                        passCount = 1;
                        lastPassIsBounds = false;
                    } else {
                        passCount = 2;
                        lastPassIsBounds = true;
                        if (reverse) {
                            passes[1] = &gInvEOColorPass;
                        } else {
                            passes[1] = &gEOColorPass;
                        }
                    }
                    drawFace[0] = drawFace[1] = GrDrawState::kBoth_DrawFace;
                    break;

                case SkPath::kInverseWinding_FillType:
                    reverse = true;
                    
                case SkPath::kWinding_FillType:
                    if (fSeparateStencil) {
                        if (fStencilWrapOps) {
                            passes[0] = &gWindStencilSeparateWithWrap;
                        } else {
                            passes[0] = &gWindStencilSeparateNoWrap;
                        }
                        passCount = 2;
                        drawFace[0] = GrDrawState::kBoth_DrawFace;
                    } else {
                        if (fStencilWrapOps) {
                            passes[0] = &gWindSingleStencilWithWrapInc;
                            passes[1] = &gWindSingleStencilWithWrapDec;
                        } else {
                            passes[0] = &gWindSingleStencilNoWrapInc;
                            passes[1] = &gWindSingleStencilNoWrapDec;
                        }
                        
                        drawFace[0] = GrDrawState::kCW_DrawFace;
                        drawFace[1] = GrDrawState::kCCW_DrawFace;
                        passCount = 3;
                    }
                    if (stencilOnly) {
                        lastPassIsBounds = false;
                        --passCount;
                    } else {
                        lastPassIsBounds = true;
                        drawFace[passCount-1] = GrDrawState::kBoth_DrawFace;
                        if (reverse) {
                            passes[passCount-1] = &gInvWindColorPass;
                        } else {
                            passes[passCount-1] = &gWindColorPass;
                        }
                    }
                    break;
                default:
                    GrAssert(!"Unknown path fFill!");
                    return false;
            }
        }
    }

    {
    for (int p = 0; p < passCount; ++p) {
        drawState->setDrawFace(drawFace[p]);
        if (NULL != passes[p]) {
            *drawState->stencil() = *passes[p];
        }

        if (lastPassIsBounds && (p == passCount-1)) {
            if (!colorWritesWereDisabled) {
                drawState->disableState(GrDrawState::kNoColorWrites_StateBit);
            }
            GrRect bounds;
            GrDrawState::AutoDeviceCoordDraw adcd;
            if (reverse) {
                GrAssert(NULL != drawState->getRenderTarget());
                
                bounds.setLTRB(0, 0,
                               SkIntToScalar(drawState->getRenderTarget()->width()),
                               SkIntToScalar(drawState->getRenderTarget()->height()));
                SkMatrix vmi;
                
                if (!drawState->getViewMatrix().hasPerspective() &&
                    drawState->getViewInverse(&vmi)) {
                    vmi.mapRect(&bounds);
                } else {
                    adcd.set(drawState);
                }
            } else {
                bounds = path.getBounds();
            }
            GrDrawTarget::AutoGeometryAndStatePush agasp(target, GrDrawTarget::kPreserve_ASRInit);
            target->drawSimpleRect(bounds, NULL);
        } else {
            if (passCount > 1) {
                drawState->enableState(GrDrawState::kNoColorWrites_StateBit);
            }
            if (indexCnt) {
                target->drawIndexed(primType, 0, 0,
                                    vertexCnt, indexCnt);
            } else {
                target->drawNonIndexed(primType, 0, vertexCnt);
            }
        }
    }
    }
    return true;
}

bool GrDefaultPathRenderer::canDrawPath(const SkPath& path,
                                        const SkStrokeRec& stroke,
                                        const GrDrawTarget* target,
                                        bool antiAlias) const {
    
    return (stroke.isFillStyle() || stroke.isHairlineStyle()) && !antiAlias;
}

bool GrDefaultPathRenderer::onDrawPath(const SkPath& path,
                                       const SkStrokeRec& stroke,
                                       GrDrawTarget* target,
                                       bool antiAlias) {
    return this->internalDrawPath(path,
                                  stroke,
                                  target,
                                  false);
}

void GrDefaultPathRenderer::onStencilPath(const SkPath& path,
                                          const SkStrokeRec& stroke,
                                          GrDrawTarget* target) {
    GrAssert(SkPath::kInverseEvenOdd_FillType != path.getFillType());
    GrAssert(SkPath::kInverseWinding_FillType != path.getFillType());
    this->internalDrawPath(path, stroke, target, true);
}
