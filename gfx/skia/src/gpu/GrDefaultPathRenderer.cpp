







#include "GrDefaultPathRenderer.h"

#include "GrContext.h"
#include "GrDrawState.h"
#include "GrPathUtils.h"
#include "SkString.h"
#include "SkTrace.h"


GrDefaultPathRenderer::GrDefaultPathRenderer(bool separateStencilSupport,
                                             bool stencilWrapOpsSupport)
    : fSeparateStencil(separateStencilSupport)
    , fStencilWrapOps(stencilWrapOpsSupport)
    , fSubpathCount(0)
    , fSubpathVertCount(0)
    , fPreviousSrcTol(-GR_Scalar1)
    , fPreviousStages(-1) {
    fTarget = NULL;
}

bool GrDefaultPathRenderer::canDrawPath(const GrDrawTarget::Caps& targetCaps,
                                        const SkPath& path,
                                        GrPathFill fill,
                                        bool antiAlias) const {
    
    
    return !antiAlias; 
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




static GrConvexHint getConvexHint(const SkPath& path) {
    return path.isConvex() ? kConvex_ConvexHint : kConcave_ConvexHint;
}

#define STENCIL_OFF     0   // Always disable stencil (even when needed)

static inline bool single_pass_path(const GrDrawTarget& target,
                                    const GrPath& path,
                                    GrPathFill fill) {
#if STENCIL_OFF
    return true;
#else
    if (kEvenOdd_PathFill == fill) {
        GrConvexHint hint = getConvexHint(path);
        return hint == kConvex_ConvexHint ||
               hint == kNonOverlappingConvexPieces_ConvexHint;
    } else if (kWinding_PathFill == fill) {
        GrConvexHint hint = getConvexHint(path);
        return hint == kConvex_ConvexHint ||
               hint == kNonOverlappingConvexPieces_ConvexHint ||
               (hint == kSameWindingConvexPieces_ConvexHint &&
                !target.drawWillReadDst() &&
                !target.getDrawState().isDitherState());

    }
    return false;
#endif
}

bool GrDefaultPathRenderer::requiresStencilPass(const GrDrawTarget* target,
                                                const GrPath& path,
                                                GrPathFill fill) const {
    return !single_pass_path(*target, path, fill);
}

void GrDefaultPathRenderer::pathWillClear() {
    fSubpathVertCount.reset(0);
    fTarget->resetVertexSource();
    if (fUseIndexedDraw) {
        fTarget->resetIndexSource();
    }
    fPreviousSrcTol = -GR_Scalar1;
    fPreviousStages = -1;
}

static inline void append_countour_edge_indices(GrPathFill fillType,
                                                uint16_t fanCenterIdx,
                                                uint16_t edgeV0Idx,
                                                uint16_t** indices) {
    
    
    
    if (kHairLine_PathFill != fillType) {
        *((*indices)++) = fanCenterIdx;
    }
    *((*indices)++) = edgeV0Idx;
    *((*indices)++) = edgeV0Idx + 1;
}

bool GrDefaultPathRenderer::createGeom(GrScalar srcSpaceTol,
                                       GrDrawState::StageMask stageMask) {
    {
    SK_TRACE_EVENT0("GrDefaultPathRenderer::createGeom");

    GrScalar srcSpaceTolSqd = GrMul(srcSpaceTol, srcSpaceTol);
    int maxPts = GrPathUtils::worstCasePointCount(*fPath, &fSubpathCount,
                                                  srcSpaceTol);

    if (maxPts <= 0) {
        return false;
    }
    if (maxPts > ((int)SK_MaxU16 + 1)) {
        GrPrintf("Path not rendered, too many verts (%d)\n", maxPts);
        return false;
    }

    GrVertexLayout layout = 0;
    for (int s = 0; s < GrDrawState::kNumStages; ++s) {
        if ((1 << s) & stageMask) {
            layout |= GrDrawTarget::StagePosAsTexCoordVertexLayoutBit(s);
        }
    }

    fUseIndexedDraw = fSubpathCount > 1;

    int maxIdxs = 0;
    if (kHairLine_PathFill == fFill) {
        if (fUseIndexedDraw) {
            maxIdxs = 2 * maxPts;
            fPrimitiveType = kLines_PrimitiveType;
        } else {
            fPrimitiveType = kLineStrip_PrimitiveType;
        }
    } else {
        if (fUseIndexedDraw) {
            maxIdxs = 3 * maxPts;
            fPrimitiveType = kTriangles_PrimitiveType;
        } else {
            fPrimitiveType = kTriangleFan_PrimitiveType;
        }
    }

    GrPoint* base;
    if (!fTarget->reserveVertexSpace(layout, maxPts, (void**)&base)) {
        return false;
    }
    GrAssert(NULL != base);
    GrPoint* vert = base;

    uint16_t* idxBase = NULL;
    uint16_t* idx = NULL;
    uint16_t subpathIdxStart = 0;
    if (fUseIndexedDraw) {
        if (!fTarget->reserveIndexSpace(maxIdxs, (void**)&idxBase)) {
            fTarget->resetVertexSource();
            return false;
        }
        GrAssert(NULL != idxBase);
        idx = idxBase;
    }

    fSubpathVertCount.reset(fSubpathCount);

    GrPoint pts[4];

    bool first = true;
    int subpath = 0;

    SkPath::Iter iter(*fPath, false);

    for (;;) {
        GrPathCmd cmd = (GrPathCmd)iter.next(pts);
        switch (cmd) {
            case kMove_PathCmd:
                if (!first) {
                    uint16_t currIdx = (uint16_t) (vert - base);
                    fSubpathVertCount[subpath] = currIdx - subpathIdxStart;
                    subpathIdxStart = currIdx;
                    ++subpath;
                }
                *vert = pts[0];
                vert++;
                break;
            case kLine_PathCmd:
                if (fUseIndexedDraw) {
                    uint16_t prevIdx = (uint16_t)(vert - base) - 1;
                    append_countour_edge_indices(fFill, subpathIdxStart,
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
                if (fUseIndexedDraw) {
                    for (uint16_t i = 0; i < numPts; ++i) {
                        append_countour_edge_indices(fFill, subpathIdxStart,
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
                if (fUseIndexedDraw) {
                    for (uint16_t i = 0; i < numPts; ++i) {
                        append_countour_edge_indices(fFill, subpathIdxStart,
                                                     firstCPtIdx + i, &idx);
                    }
                }
                break;
            }
            case kClose_PathCmd:
                break;
            case kEnd_PathCmd:
                uint16_t currIdx = (uint16_t) (vert - base);
                fSubpathVertCount[subpath] = currIdx - subpathIdxStart;
                goto FINISHED;
        }
        first = false;
    }
FINISHED:
    GrAssert((vert - base) <= maxPts);
    GrAssert((idx - idxBase) <= maxIdxs);

    fVertexCnt = vert - base;
    fIndexCnt = idx - idxBase;

    if (fTranslate.fX || fTranslate.fY) {
        int count = vert - base;
        for (int i = 0; i < count; i++) {
            base[i].offset(fTranslate.fX, fTranslate.fY);
        }
    }
    }
    
    
    
    fPreviousSrcTol = srcSpaceTol;
    fPreviousStages = stageMask;
    return true;
}

void GrDefaultPathRenderer::onDrawPath(GrDrawState::StageMask stageMask,
                                       bool stencilOnly) {

    GrMatrix viewM = fTarget->getDrawState().getViewMatrix();
    GrScalar tol = GR_Scalar1;
    tol = GrPathUtils::scaleToleranceToSrc(tol, viewM, fPath->getBounds());
    GrDrawState* drawState = fTarget->drawState();

    
    
    
    
    
    
    
    
    if (tol != fPreviousSrcTol ||
        stageMask != fPreviousStages) {
        if (!this->createGeom(tol, stageMask)) {
            return;
        }
    }

    GrAssert(NULL != fTarget);
    GrDrawTarget::AutoStateRestore asr(fTarget);
    bool colorWritesWereDisabled = drawState->isColorWriteDisabled();
    
    GrAssert(GrDrawState::kBoth_DrawFace == drawState->getDrawFace());

    int                         passCount = 0;
    const GrStencilSettings*    passes[3];
    GrDrawState::DrawFace       drawFace[3];
    bool                        reverse = false;
    bool                        lastPassIsBounds;

    if (kHairLine_PathFill == fFill) {
        passCount = 1;
        if (stencilOnly) {
            passes[0] = &gDirectToStencil;
        } else {
            passes[0] = NULL;
        }
        lastPassIsBounds = false;
        drawFace[0] = GrDrawState::kBoth_DrawFace;
    } else {
        if (single_pass_path(*fTarget, *fPath, fFill)) {
            passCount = 1;
            if (stencilOnly) {
                passes[0] = &gDirectToStencil;
            } else {
                passes[0] = NULL;
            }
            drawFace[0] = GrDrawState::kBoth_DrawFace;
            lastPassIsBounds = false;
        } else {
            switch (fFill) {
                case kInverseEvenOdd_PathFill:
                    reverse = true;
                    
                case kEvenOdd_PathFill:
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

                case kInverseWinding_PathFill:
                    reverse = true;
                    
                case kWinding_PathFill:
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
                    return;
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
            if (reverse) {
                GrAssert(NULL != drawState->getRenderTarget());
                
                bounds.setLTRB(0, 0,
                               GrIntToScalar(drawState->getRenderTarget()->width()),
                               GrIntToScalar(drawState->getRenderTarget()->height()));
                GrMatrix vmi;
                
                if (!drawState->getViewMatrix().hasPerspective() &&
                    drawState->getViewInverse(&vmi)) {
                    vmi.mapRect(&bounds);
                } else {
                    if (stageMask) {
                        if (!drawState->getViewInverse(&vmi)) {
                            GrPrintf("Could not invert matrix.");
                            return;
                        }
                        drawState->preConcatSamplerMatrices(stageMask, vmi);
                    }
                    drawState->setViewMatrix(GrMatrix::I());
                }
            } else {
                bounds = fPath->getBounds();
                bounds.offset(fTranslate);
            }
            GrDrawTarget::AutoGeometryPush agp(fTarget);
            fTarget->drawSimpleRect(bounds, NULL, stageMask);
        } else {
            if (passCount > 1) {
                drawState->enableState(GrDrawState::kNoColorWrites_StateBit);
            }
            if (fUseIndexedDraw) {
                fTarget->drawIndexed(fPrimitiveType, 0, 0, 
                                     fVertexCnt, fIndexCnt);
            } else {
                int baseVertex = 0;
                for (int sp = 0; sp < fSubpathCount; ++sp) {
                    fTarget->drawNonIndexed(fPrimitiveType, baseVertex,
                                            fSubpathVertCount[sp]);
                    baseVertex += fSubpathVertCount[sp];
                }
            }
        }
    }
    }
}

void GrDefaultPathRenderer::drawPath(GrDrawState::StageMask stageMask) {
    this->onDrawPath(stageMask, false);
}

void GrDefaultPathRenderer::drawPathToStencil() {
    GrAssert(kInverseEvenOdd_PathFill != fFill);
    GrAssert(kInverseWinding_PathFill != fFill);
    this->onDrawPath(0, true);
}
