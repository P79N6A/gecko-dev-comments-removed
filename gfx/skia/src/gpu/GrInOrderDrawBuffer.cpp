








#include "GrInOrderDrawBuffer.h"
#include "GrRenderTarget.h"
#include "GrTexture.h"
#include "GrBufferAllocPool.h"
#include "GrIndexBuffer.h"
#include "GrVertexBuffer.h"
#include "GrGpu.h"

GrInOrderDrawBuffer::GrInOrderDrawBuffer(const GrGpu* gpu,
                                         GrVertexBufferAllocPool* vertexPool,
                                         GrIndexBufferAllocPool* indexPool)
    : fAutoFlushTarget(NULL)
    , fClipSet(true)
    , fVertexPool(*vertexPool)
    , fIndexPool(*indexPool)
    , fLastRectVertexLayout(0)
    , fQuadIndexBuffer(NULL)
    , fMaxQuads(0)
    , fCurrQuad(0) {

    fCaps = gpu->getCaps();

    GrAssert(NULL != vertexPool);
    GrAssert(NULL != indexPool);

    GeometryPoolState& poolState = fGeoPoolStateStack.push_back();
    poolState.fUsedPoolVertexBytes = 0;
    poolState.fUsedPoolIndexBytes = 0;
#if GR_DEBUG
    poolState.fPoolVertexBuffer = (GrVertexBuffer*)~0;
    poolState.fPoolStartVertex = ~0;
    poolState.fPoolIndexBuffer = (GrIndexBuffer*)~0;
    poolState.fPoolStartIndex = ~0;
#endif
    fInstancedDrawTracker.reset();
}

GrInOrderDrawBuffer::~GrInOrderDrawBuffer() {
    this->reset();
    
    this->releaseGeometry();
    GrSafeUnref(fQuadIndexBuffer);
    GrSafeUnref(fAutoFlushTarget);
}

void GrInOrderDrawBuffer::setQuadIndexBuffer(const GrIndexBuffer* indexBuffer) {
    bool newIdxBuffer = fQuadIndexBuffer != indexBuffer;
    if (newIdxBuffer) {
        GrSafeUnref(fQuadIndexBuffer);
        fQuadIndexBuffer = indexBuffer;
        GrSafeRef(fQuadIndexBuffer);
        fCurrQuad = 0;
        fMaxQuads = (NULL == indexBuffer) ? 0 : indexBuffer->maxQuads();
    } else {
        GrAssert((NULL == indexBuffer && 0 == fMaxQuads) ||
                 (indexBuffer->maxQuads() == fMaxQuads));
    }
}



void GrInOrderDrawBuffer::resetDrawTracking() {
    fCurrQuad = 0;
    fInstancedDrawTracker.reset();
}

void GrInOrderDrawBuffer::drawRect(const GrRect& rect,
                                   const GrMatrix* matrix,
                                   StageMask stageMask,
                                   const GrRect* srcRects[],
                                   const GrMatrix* srcMatrices[]) {

    GrAssert(!(NULL == fQuadIndexBuffer && fCurrQuad));
    GrAssert(!(fDraws.empty() && fCurrQuad));
    GrAssert(!(0 != fMaxQuads && NULL == fQuadIndexBuffer));

    GrDrawState* drawState = this->drawState();

    
    
    if (fMaxQuads) {

        bool appendToPreviousDraw = false;
        GrVertexLayout layout = GetRectVertexLayout(stageMask, srcRects);
        AutoReleaseGeometry geo(this, layout, 4, 0);
        if (!geo.succeeded()) {
            GrPrintf("Failed to get space for vertices!\n");
            return;
        }
        GrMatrix combinedMatrix = drawState->getViewMatrix();
        
        
        
        
        
        StageMask devCoordMask = (NULL == srcRects) ? stageMask : 0;
        GrDrawTarget::AutoDeviceCoordDraw adcd(this, devCoordMask);
        if (NULL != matrix) {
            combinedMatrix.preConcat(*matrix);
        }

        SetRectVertices(rect, &combinedMatrix, srcRects, srcMatrices, layout, geo.vertices());

        
        
        
        bool disabledClip = false;
        if (drawState->isClipState() && fClip.isRect()) {

            GrRect clipRect = fClip.getRect(0);
            
            
            
            
            const GrRenderTarget* target = drawState->getRenderTarget();
            if (0 >= clipRect.fLeft) {
                clipRect.fLeft = GR_ScalarMin;
            }
            if (target->width() <= clipRect.fRight) {
                clipRect.fRight = GR_ScalarMax;
            }
            if (0 >= clipRect.top()) {
                clipRect.fTop = GR_ScalarMin;
            }
            if (target->height() <= clipRect.fBottom) {
                clipRect.fBottom = GR_ScalarMax;
            }
            int stride = VertexSize(layout);
            bool insideClip = true;
            for (int v = 0; v < 4; ++v) {
                const GrPoint& p = *GetVertexPoint(geo.vertices(), v, stride);
                if (!clipRect.contains(p)) {
                    insideClip = false;
                    break;
                }
            }
            if (insideClip) {
                drawState->disableState(GrDrawState::kClip_StateBit);
                disabledClip = true;
            }
        }
        if (!needsNewClip() && !needsNewState() && fCurrQuad > 0 &&
            fCurrQuad < fMaxQuads && layout == fLastRectVertexLayout) {

            int vsize = VertexSize(layout);

            Draw& lastDraw = fDraws.back();

            GrAssert(lastDraw.fIndexBuffer == fQuadIndexBuffer);
            GrAssert(kTriangles_PrimitiveType == lastDraw.fPrimitiveType);
            GrAssert(0 == lastDraw.fVertexCount % 4);
            GrAssert(0 == lastDraw.fIndexCount % 6);
            GrAssert(0 == lastDraw.fStartIndex);

            GeometryPoolState& poolState = fGeoPoolStateStack.back();
            bool clearSinceLastDraw =
                            fClears.count() && 
                            fClears.back().fBeforeDrawIdx == fDraws.count();

            appendToPreviousDraw =  
                !clearSinceLastDraw &&
                lastDraw.fVertexBuffer == poolState.fPoolVertexBuffer &&
                (fCurrQuad * 4 + lastDraw.fStartVertex) == poolState.fPoolStartVertex;

            if (appendToPreviousDraw) {
                lastDraw.fVertexCount += 4;
                lastDraw.fIndexCount += 6;
                fCurrQuad += 1;
                
                
                GrAssert(0 == poolState.fUsedPoolVertexBytes);
                poolState.fUsedPoolVertexBytes = 4 * vsize;
            }
        }
        if (!appendToPreviousDraw) {
            this->setIndexSourceToBuffer(fQuadIndexBuffer);
            this->drawIndexed(kTriangles_PrimitiveType, 0, 0, 4, 6);
            fCurrQuad = 1;
            fLastRectVertexLayout = layout;
        }
        if (disabledClip) {
            drawState->enableState(GrDrawState::kClip_StateBit);
        }
        fInstancedDrawTracker.reset();
    } else {
        INHERITED::drawRect(rect, matrix, stageMask, srcRects, srcMatrices);
    }
}

void GrInOrderDrawBuffer::drawIndexedInstances(GrPrimitiveType type,
                                               int instanceCount,
                                               int verticesPerInstance,
                                               int indicesPerInstance) {
    if (!verticesPerInstance || !indicesPerInstance) {
        return;
    }

    const GeometrySrcState& geomSrc = this->getGeomSrc();

    
    
    if (kReserved_GeometrySrcType == geomSrc.fVertexSrc &&
        kBuffer_GeometrySrcType == geomSrc.fIndexSrc) {

        Draw* draw = NULL;
        
        
        if (verticesPerInstance == fInstancedDrawTracker.fVerticesPerInstance &&
            indicesPerInstance == fInstancedDrawTracker.fIndicesPerInstance) {
            GrAssert(fDraws.count());
            draw = &fDraws.back();
        }

        bool clipChanged = this->needsNewClip();
        bool stateChanged = this->needsNewState();
        if (clipChanged) {
            this->storeClip();
        }
        if (stateChanged) {
            this->pushState();
        }

        GeometryPoolState& poolState = fGeoPoolStateStack.back();
        const GrVertexBuffer* vertexBuffer = poolState.fPoolVertexBuffer;

        
        
        if (NULL == draw ||
            clipChanged ||
            stateChanged ||
            draw->fIndexBuffer != geomSrc.fIndexBuffer ||
            draw->fPrimitiveType != type ||
            draw->fVertexBuffer != vertexBuffer) {

            draw = &fDraws.push_back();
            draw->fClipChanged = clipChanged;
            draw->fStateChanged = stateChanged;
            draw->fIndexBuffer = geomSrc.fIndexBuffer;
            geomSrc.fIndexBuffer->ref();
            draw->fVertexBuffer = vertexBuffer;
            vertexBuffer->ref();
            draw->fPrimitiveType = type;
            draw->fStartIndex = 0;
            draw->fIndexCount = 0;
            draw->fStartVertex = poolState.fPoolStartVertex;
            draw->fVertexCount = 0;
            draw->fVertexLayout = geomSrc.fVertexLayout;
        } else {
            GrAssert(!(draw->fIndexCount % indicesPerInstance));
            GrAssert(!(draw->fVertexCount % verticesPerInstance));
            GrAssert(poolState.fPoolStartVertex == draw->fStartVertex +
                                                   draw->fVertexCount);
        }

        
        int maxInstancesPerDraw = this->indexCountInCurrentSource() /
                                  indicesPerInstance;
        if (!maxInstancesPerDraw) {
            return;
        }
        
        int instancesToConcat = maxInstancesPerDraw - draw->fVertexCount /
                                                      verticesPerInstance;
        if (maxInstancesPerDraw > instanceCount) {
            maxInstancesPerDraw = instanceCount;
            if (instancesToConcat > instanceCount) {
                instancesToConcat = instanceCount;
            }
        }

        
        size_t vertexBytes = instanceCount * verticesPerInstance *
                             VertexSize(draw->fVertexLayout);
        poolState.fUsedPoolVertexBytes =
                            GrMax(poolState.fUsedPoolVertexBytes, vertexBytes);

        while (instanceCount) {
            if (!instancesToConcat) {
                int startVertex = draw->fStartVertex + draw->fVertexCount;
                draw = &fDraws.push_back();
                draw->fClipChanged = false;
                draw->fStateChanged = false;
                draw->fIndexBuffer = geomSrc.fIndexBuffer;
                geomSrc.fIndexBuffer->ref();
                draw->fVertexBuffer = vertexBuffer;
                vertexBuffer->ref();
                draw->fPrimitiveType = type;
                draw->fStartIndex = 0;
                draw->fStartVertex = startVertex;
                draw->fVertexCount = 0;
                draw->fVertexLayout = geomSrc.fVertexLayout;
                instancesToConcat = maxInstancesPerDraw;
            }
            draw->fVertexCount += instancesToConcat * verticesPerInstance;
            draw->fIndexCount += instancesToConcat * indicesPerInstance;
            instanceCount -= instancesToConcat;
            instancesToConcat = 0;
        }

        
        fCurrQuad = 0;
        fInstancedDrawTracker.fVerticesPerInstance = verticesPerInstance;
        fInstancedDrawTracker.fIndicesPerInstance = indicesPerInstance;
    } else {
        this->INHERITED::drawIndexedInstances(type,
                                              instanceCount,
                                              verticesPerInstance,
                                              indicesPerInstance);
    }

}

void GrInOrderDrawBuffer::onDrawIndexed(GrPrimitiveType primitiveType,
                                        int startVertex,
                                        int startIndex,
                                        int vertexCount,
                                        int indexCount) {

    if (!vertexCount || !indexCount) {
        return;
    }

    this->resetDrawTracking();

    GeometryPoolState& poolState = fGeoPoolStateStack.back();

    Draw& draw = fDraws.push_back();
    draw.fPrimitiveType = primitiveType;
    draw.fStartVertex   = startVertex;
    draw.fStartIndex    = startIndex;
    draw.fVertexCount   = vertexCount;
    draw.fIndexCount    = indexCount;

    draw.fClipChanged = this->needsNewClip();
    if (draw.fClipChanged) {
       this->storeClip();
    }

    draw.fStateChanged = this->needsNewState();
    if (draw.fStateChanged) {
        this->pushState();
    }

    draw.fVertexLayout = this->getVertexLayout();
    switch (this->getGeomSrc().fVertexSrc) {
    case kBuffer_GeometrySrcType:
        draw.fVertexBuffer = this->getGeomSrc().fVertexBuffer;
        break;
    case kReserved_GeometrySrcType: 
    case kArray_GeometrySrcType: {
        size_t vertexBytes = (vertexCount + startVertex) *
                             VertexSize(draw.fVertexLayout);
        poolState.fUsedPoolVertexBytes = 
                            GrMax(poolState.fUsedPoolVertexBytes, vertexBytes);
        draw.fVertexBuffer = poolState.fPoolVertexBuffer;
        draw.fStartVertex += poolState.fPoolStartVertex;
        break;
    }
    default:
        GrCrash("unknown geom src type");
    }
    draw.fVertexBuffer->ref();

    switch (this->getGeomSrc().fIndexSrc) {
    case kBuffer_GeometrySrcType:
        draw.fIndexBuffer = this->getGeomSrc().fIndexBuffer;
        break;
    case kReserved_GeometrySrcType: 
    case kArray_GeometrySrcType: {
        size_t indexBytes = (indexCount + startIndex) * sizeof(uint16_t);
        poolState.fUsedPoolIndexBytes = 
                            GrMax(poolState.fUsedPoolIndexBytes, indexBytes);
        draw.fIndexBuffer = poolState.fPoolIndexBuffer;
        draw.fStartIndex += poolState.fPoolStartIndex;
        break;
    }
    default:
        GrCrash("unknown geom src type");
    }
    draw.fIndexBuffer->ref();
}

void GrInOrderDrawBuffer::onDrawNonIndexed(GrPrimitiveType primitiveType,
                                           int startVertex,
                                           int vertexCount) {
    if (!vertexCount) {
        return;
    }

    this->resetDrawTracking();

    GeometryPoolState& poolState = fGeoPoolStateStack.back();

    Draw& draw = fDraws.push_back();
    draw.fPrimitiveType = primitiveType;
    draw.fStartVertex   = startVertex;
    draw.fStartIndex    = 0;
    draw.fVertexCount   = vertexCount;
    draw.fIndexCount    = 0;

    draw.fClipChanged = this->needsNewClip();
    if (draw.fClipChanged) {
        this->storeClip();
    }

    draw.fStateChanged = this->needsNewState();
    if (draw.fStateChanged) {
        this->pushState();
    }

    draw.fVertexLayout = this->getVertexLayout();
    switch (this->getGeomSrc().fVertexSrc) {
    case kBuffer_GeometrySrcType:
        draw.fVertexBuffer = this->getGeomSrc().fVertexBuffer;
        break;
    case kReserved_GeometrySrcType: 
    case kArray_GeometrySrcType: {
        size_t vertexBytes = (vertexCount + startVertex) *
                             VertexSize(draw.fVertexLayout);
        poolState.fUsedPoolVertexBytes = 
                            GrMax(poolState.fUsedPoolVertexBytes, vertexBytes);
        draw.fVertexBuffer = poolState.fPoolVertexBuffer;
        draw.fStartVertex += poolState.fPoolStartVertex;
        break;
    }
    default:
        GrCrash("unknown geom src type");
    }
    draw.fVertexBuffer->ref();
    draw.fIndexBuffer = NULL;
}

void GrInOrderDrawBuffer::clear(const GrIRect* rect, GrColor color) {
    GrIRect r;
    if (NULL == rect) {
        
        
        
        r.setLTRB(0, 0, 
                  this->getDrawState().getRenderTarget()->width(), 
                  this->getDrawState().getRenderTarget()->height());
        rect = &r;
    }
    Clear& clr = fClears.push_back();
    clr.fColor = color;
    clr.fBeforeDrawIdx = fDraws.count();
    clr.fRect = *rect;
}

void GrInOrderDrawBuffer::reset() {
    GrAssert(1 == fGeoPoolStateStack.count());
    this->resetVertexSource();
    this->resetIndexSource();
    uint32_t numStates = fStates.count();
    for (uint32_t i = 0; i < numStates; ++i) {
        for (int s = 0; s < GrDrawState::kNumStages; ++s) {
            GrSafeUnref(fStates[i].getTexture(s));
        }
        GrSafeUnref(fStates[i].getRenderTarget());

        
        
        fStates[i].disableBehavior(GrDrawState::kTexturesNeedRef_BehaviorBit);
    }
    int numDraws = fDraws.count();
    for (int d = 0; d < numDraws; ++d) {
        
        GrAssert(NULL != fDraws[d].fVertexBuffer);
        fDraws[d].fVertexBuffer->unref();
        GrSafeUnref(fDraws[d].fIndexBuffer);
    }
    fDraws.reset();
    fStates.reset();

    fClears.reset();

    fVertexPool.reset();
    fIndexPool.reset();

    fClips.reset();

    this->resetDrawTracking();
}

void GrInOrderDrawBuffer::playback(GrDrawTarget* target) {
    GrAssert(kReserved_GeometrySrcType != this->getGeomSrc().fVertexSrc);
    GrAssert(kReserved_GeometrySrcType != this->getGeomSrc().fIndexSrc);

    GrAssert(NULL != target);
    GrAssert(target != this); 

    int numDraws = fDraws.count();
    if (!numDraws) {
        return;
    }

    fVertexPool.unlock();
    fIndexPool.unlock();

    GrDrawTarget::AutoClipRestore acr(target);
    AutoGeometryPush agp(target);
    GrDrawState* prevDrawState = target->drawState();
    prevDrawState->ref();

    int currState = ~0;
    int currClip  = ~0;
    int currClear = 0;

    for (int i = 0; i < numDraws; ++i) {
        while (currClear < fClears.count() && 
               i == fClears[currClear].fBeforeDrawIdx) {
            target->clear(&fClears[currClear].fRect, fClears[currClear].fColor);
            ++currClear;
        }

        const Draw& draw = fDraws[i];
        if (draw.fStateChanged) {
            ++currState;
            target->setDrawState(&fStates[currState]);
        }
        if (draw.fClipChanged) {
            ++currClip;
            target->setClip(fClips[currClip]);
        }

        target->setVertexSourceToBuffer(draw.fVertexLayout, draw.fVertexBuffer);

        if (draw.fIndexCount) {
            target->setIndexSourceToBuffer(draw.fIndexBuffer);
        }

        if (draw.fIndexCount) {
            target->drawIndexed(draw.fPrimitiveType,
                                draw.fStartVertex,
                                draw.fStartIndex,
                                draw.fVertexCount,
                                draw.fIndexCount);
        } else {
            target->drawNonIndexed(draw.fPrimitiveType,
                                   draw.fStartVertex,
                                   draw.fVertexCount);
        }
    }
    while (currClear < fClears.count()) {
        GrAssert(fDraws.count() == fClears[currClear].fBeforeDrawIdx);
        target->clear(&fClears[currClear].fRect, fClears[currClear].fColor);
        ++currClear;
    }
    target->setDrawState(prevDrawState);
    prevDrawState->unref();
}

void GrInOrderDrawBuffer::setAutoFlushTarget(GrDrawTarget* target) {
    GrSafeAssign(fAutoFlushTarget, target);
}

void GrInOrderDrawBuffer::willReserveVertexAndIndexSpace(
                                GrVertexLayout vertexLayout,
                                int vertexCount,
                                int indexCount) {
    if (NULL != fAutoFlushTarget) {
        
        
        
        
        
        
        bool insideGeoPush = fGeoPoolStateStack.count() > 1;

        bool unreleasedVertexSpace =
            !vertexCount &&
            kReserved_GeometrySrcType == this->getGeomSrc().fVertexSrc;

        bool unreleasedIndexSpace =
            !indexCount &&
            kReserved_GeometrySrcType == this->getGeomSrc().fIndexSrc;

        
        
        bool targetHasReservedGeom =
            fAutoFlushTarget->hasReservedVerticesOrIndices();
        
        int vcount = vertexCount;
        int icount = indexCount;
        
        if (!insideGeoPush &&
            !unreleasedVertexSpace &&
            !unreleasedIndexSpace &&
            !targetHasReservedGeom &&
            this->geometryHints(vertexLayout, &vcount, &icount)) {

            this->flushTo(fAutoFlushTarget);
        }
    }
}

bool GrInOrderDrawBuffer::geometryHints(GrVertexLayout vertexLayout,
                                        int* vertexCount,
                                        int* indexCount) const {
    
    
    
    bool flush = false;
    if (NULL != indexCount) {
        int32_t currIndices = fIndexPool.currentBufferIndices();
        if (*indexCount > currIndices &&
            (!fIndexPool.preallocatedBuffersRemaining() &&
             *indexCount <= fIndexPool.preallocatedBufferIndices())) {

            flush = true;
        }
        *indexCount = currIndices;
    }
    if (NULL != vertexCount) {
        int32_t currVertices = fVertexPool.currentBufferVertices(vertexLayout);
        if (*vertexCount > currVertices &&
            (!fVertexPool.preallocatedBuffersRemaining() &&
             *vertexCount <= fVertexPool.preallocatedBufferVertices(vertexLayout))) {

            flush = true;
        }
        *vertexCount = currVertices;
    }
    return flush;
}

bool GrInOrderDrawBuffer::onReserveVertexSpace(GrVertexLayout vertexLayout,
                                               int vertexCount,
                                               void** vertices) {
    GeometryPoolState& poolState = fGeoPoolStateStack.back();
    GrAssert(vertexCount > 0);
    GrAssert(NULL != vertices);
    GrAssert(0 == poolState.fUsedPoolVertexBytes);
    
    *vertices = fVertexPool.makeSpace(vertexLayout,
                                      vertexCount,
                                      &poolState.fPoolVertexBuffer,
                                      &poolState.fPoolStartVertex);
    return NULL != *vertices;
}
    
bool GrInOrderDrawBuffer::onReserveIndexSpace(int indexCount, void** indices) {
    GeometryPoolState& poolState = fGeoPoolStateStack.back();
    GrAssert(indexCount > 0);
    GrAssert(NULL != indices);
    GrAssert(0 == poolState.fUsedPoolIndexBytes);

    *indices = fIndexPool.makeSpace(indexCount,
                                    &poolState.fPoolIndexBuffer,
                                    &poolState.fPoolStartIndex);
    return NULL != *indices;
}

void GrInOrderDrawBuffer::releaseReservedVertexSpace() {
    GeometryPoolState& poolState = fGeoPoolStateStack.back();
    const GeometrySrcState& geoSrc = this->getGeomSrc(); 
    
    GrAssert(kReserved_GeometrySrcType == geoSrc.fVertexSrc);

    
    
    
    
    size_t reservedVertexBytes = VertexSize(geoSrc.fVertexLayout) *
                                 geoSrc.fVertexCount;
    fVertexPool.putBack(reservedVertexBytes - 
                        poolState.fUsedPoolVertexBytes);
    poolState.fUsedPoolVertexBytes = 0;
    poolState.fPoolVertexBuffer = NULL;
    poolState.fPoolStartVertex = 0;
}

void GrInOrderDrawBuffer::releaseReservedIndexSpace() {
    GeometryPoolState& poolState = fGeoPoolStateStack.back();
    const GeometrySrcState& geoSrc = this->getGeomSrc(); 

    GrAssert(kReserved_GeometrySrcType == geoSrc.fIndexSrc);

    
    
    size_t reservedIndexBytes = sizeof(uint16_t) * geoSrc.fIndexCount;
    fIndexPool.putBack(reservedIndexBytes - poolState.fUsedPoolIndexBytes);
    poolState.fUsedPoolIndexBytes = 0;
    poolState.fPoolIndexBuffer = NULL;
    poolState.fPoolStartIndex = 0;
}
    
void GrInOrderDrawBuffer::onSetVertexSourceToArray(const void* vertexArray,
                                                   int vertexCount) {

    GeometryPoolState& poolState = fGeoPoolStateStack.back();
    GrAssert(0 == poolState.fUsedPoolVertexBytes);
#if GR_DEBUG
    bool success =
#endif
    fVertexPool.appendVertices(this->getVertexLayout(),
                               vertexCount,
                               vertexArray,
                               &poolState.fPoolVertexBuffer,
                               &poolState.fPoolStartVertex);
    GR_DEBUGASSERT(success);
}

void GrInOrderDrawBuffer::onSetIndexSourceToArray(const void* indexArray,
                                                  int indexCount) {
    GeometryPoolState& poolState = fGeoPoolStateStack.back();
    GrAssert(0 == poolState.fUsedPoolIndexBytes);
#if GR_DEBUG
    bool success =
#endif
    fIndexPool.appendIndices(indexCount,
                             indexArray,
                             &poolState.fPoolIndexBuffer,
                             &poolState.fPoolStartIndex);
    GR_DEBUGASSERT(success);
}

void GrInOrderDrawBuffer::releaseVertexArray() {
    
    
    this->GrInOrderDrawBuffer::releaseReservedVertexSpace();
}

void GrInOrderDrawBuffer::releaseIndexArray() {
    
    
    this->GrInOrderDrawBuffer::releaseReservedIndexSpace();
}

void GrInOrderDrawBuffer::geometrySourceWillPush() {
    GeometryPoolState& poolState = fGeoPoolStateStack.push_back();
    poolState.fUsedPoolVertexBytes = 0;
    poolState.fUsedPoolIndexBytes = 0;
    this->resetDrawTracking();
#if GR_DEBUG
    poolState.fPoolVertexBuffer = (GrVertexBuffer*)~0;
    poolState.fPoolStartVertex = ~0;
    poolState.fPoolIndexBuffer = (GrIndexBuffer*)~0;
    poolState.fPoolStartIndex = ~0;
#endif
}

void GrInOrderDrawBuffer::geometrySourceWillPop(
                                        const GeometrySrcState& restoredState) {
    GrAssert(fGeoPoolStateStack.count() > 1);
    fGeoPoolStateStack.pop_back();
    GeometryPoolState& poolState = fGeoPoolStateStack.back();
    
    
    
    if (kReserved_GeometrySrcType == restoredState.fVertexSrc ||
        kArray_GeometrySrcType == restoredState.fVertexSrc) {
        poolState.fUsedPoolVertexBytes = 
            VertexSize(restoredState.fVertexLayout) * 
            restoredState.fVertexCount;
    }
    if (kReserved_GeometrySrcType == restoredState.fIndexSrc ||
        kArray_GeometrySrcType == restoredState.fIndexSrc) {
        poolState.fUsedPoolIndexBytes = sizeof(uint16_t) * 
                                         restoredState.fIndexCount;
    }
    this->resetDrawTracking();
}

bool GrInOrderDrawBuffer::needsNewState() const {
     if (fStates.empty()) {
        return true;
     } else {
        return fStates.back() != this->getDrawState();
     }
}

void GrInOrderDrawBuffer::pushState() {
    const GrDrawState& drawState = this->getDrawState();
    for (int s = 0; s < GrDrawState::kNumStages; ++s) {
        GrSafeRef(drawState.getTexture(s));
    }
    GrSafeRef(drawState.getRenderTarget());
    fStates.push_back(this->getDrawState());

    
    
    fStates.back().enableBehavior(GrDrawState::kTexturesNeedRef_BehaviorBit);
 }

bool GrInOrderDrawBuffer::needsNewClip() const {
   if (this->getDrawState().isClipState()) {
       if (fClips.empty() || (fClipSet && fClips.back() != fClip)) {
           return true;
       }
    }
    return false;
}

void GrInOrderDrawBuffer::storeClip() {
    fClips.push_back() = fClip;
    fClipSet = false;
}

void GrInOrderDrawBuffer::clipWillBeSet(const GrClip& newClip) {
    INHERITED::clipWillBeSet(newClip);
    fClipSet = true;
}
