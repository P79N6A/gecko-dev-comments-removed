









#include "GrDrawTarget.h"
#include "GrGpuVertex.h"
#include "GrIndexBuffer.h"
#include "GrRenderTarget.h"
#include "GrTexture.h"
#include "GrVertexBuffer.h"

SK_DEFINE_INST_COUNT(GrDrawTarget)

namespace {










void gen_mask_arrays(GrVertexLayout* stageTexCoordMasks,
                     GrVertexLayout* texCoordMasks) {
    for (int s = 0; s < GrDrawState::kNumStages; ++s) {
        stageTexCoordMasks[s] = 0;
        for (int t = 0; t < GrDrawState::kMaxTexCoords; ++t) {
            stageTexCoordMasks[s] |= GrDrawTarget::StageTexCoordVertexLayoutBit(s, t);
        }
    }
    for (int t = 0; t < GrDrawState::kMaxTexCoords; ++t) {
        texCoordMasks[t] = 0;
        for (int s = 0; s < GrDrawState::kNumStages; ++s) {
            texCoordMasks[t] |= GrDrawTarget::StageTexCoordVertexLayoutBit(s, t);
        }
    }
}




void gen_globals() {
    GrVertexLayout stageTexCoordMasks[GrDrawState::kNumStages];
    GrVertexLayout texCoordMasks[GrDrawState::kMaxTexCoords];
    gen_mask_arrays(stageTexCoordMasks, texCoordMasks);

    GrPrintf("const GrVertexLayout gStageTexCoordMasks[] = {\n");
    for (int s = 0; s < GrDrawState::kNumStages; ++s) {
        GrPrintf("    0x%x,\n", stageTexCoordMasks[s]);
    }
    GrPrintf("};\n");
    GrPrintf("GR_STATIC_ASSERT(GrDrawState::kNumStages == GR_ARRAY_COUNT(gStageTexCoordMasks));\n\n");
    GrPrintf("const GrVertexLayout gTexCoordMasks[] = {\n");
    for (int t = 0; t < GrDrawState::kMaxTexCoords; ++t) {
        GrPrintf("    0x%x,\n", texCoordMasks[t]);
    }
    GrPrintf("};\n");
    GrPrintf("GR_STATIC_ASSERT(GrDrawState::kMaxTexCoords == GR_ARRAY_COUNT(gTexCoordMasks));\n");
}



const GrVertexLayout gStageTexCoordMasks[] = {
    0x108421,
    0x210842,
    0x421084,
    0x842108,
    0x1084210,
};
GR_STATIC_ASSERT(GrDrawState::kNumStages == GR_ARRAY_COUNT(gStageTexCoordMasks));

const GrVertexLayout gTexCoordMasks[] = {
    0x1f,
    0x3e0,
    0x7c00,
    0xf8000,
    0x1f00000,
};
GR_STATIC_ASSERT(GrDrawState::kMaxTexCoords == GR_ARRAY_COUNT(gTexCoordMasks));

bool check_layout(GrVertexLayout layout) {
    
    for (int s = 0; s < GrDrawState::kNumStages; ++s) {
        int stageBits = layout & gStageTexCoordMasks[s];
        if (stageBits && !GrIsPow2(stageBits)) {
            return false;
        }
    }
    return true;
}

int num_tex_coords(GrVertexLayout layout) {
    int cnt = 0;
    
    for (int t = 0; t < GrDrawState::kMaxTexCoords; ++t) {
        if (gTexCoordMasks[t] & layout) {
            ++cnt;
        }
    }
    return cnt;
}

} 

size_t GrDrawTarget::VertexSize(GrVertexLayout vertexLayout) {
    GrAssert(check_layout(vertexLayout));

    size_t vecSize = (vertexLayout & kTextFormat_VertexLayoutBit) ?
                        sizeof(GrGpuTextVertex) :
                        sizeof(GrPoint);

    size_t size = vecSize; 
    size += num_tex_coords(vertexLayout) * vecSize;
    if (vertexLayout & kColor_VertexLayoutBit) {
        size += sizeof(GrColor);
    }
    if (vertexLayout & kCoverage_VertexLayoutBit) {
        size += sizeof(GrColor);
    }
    if (vertexLayout & kEdge_VertexLayoutBit) {
        size += 4 * sizeof(GrScalar);
    }
    return size;
}
















int GrDrawTarget::VertexStageCoordOffset(int stage, GrVertexLayout vertexLayout) {
    GrAssert(check_layout(vertexLayout));

    if (!StageUsesTexCoords(vertexLayout, stage)) {
        return 0;
    }
    int tcIdx = VertexTexCoordsForStage(stage, vertexLayout);
    if (tcIdx >= 0) {

        int vecSize = (vertexLayout & kTextFormat_VertexLayoutBit) ?
                                    sizeof(GrGpuTextVertex) :
                                    sizeof(GrPoint);
        int offset = vecSize; 
        
        for (int t = 0; t < tcIdx; ++t) {
            if (gTexCoordMasks[t] & vertexLayout) {
                offset += vecSize;
            }
        }
        return offset;
    }

    return -1;
}

int GrDrawTarget::VertexColorOffset(GrVertexLayout vertexLayout) {
    GrAssert(check_layout(vertexLayout));

    if (vertexLayout & kColor_VertexLayoutBit) {
        int vecSize = (vertexLayout & kTextFormat_VertexLayoutBit) ?
                                    sizeof(GrGpuTextVertex) :
                                    sizeof(GrPoint);
        return vecSize * (num_tex_coords(vertexLayout) + 1); 
    }
    return -1;
}

int GrDrawTarget::VertexCoverageOffset(GrVertexLayout vertexLayout) {
    GrAssert(check_layout(vertexLayout));

    if (vertexLayout & kCoverage_VertexLayoutBit) {
        int vecSize = (vertexLayout & kTextFormat_VertexLayoutBit) ?
                                    sizeof(GrGpuTextVertex) :
                                    sizeof(GrPoint);

        int offset = vecSize * (num_tex_coords(vertexLayout) + 1);
        if (vertexLayout & kColor_VertexLayoutBit) {
            offset += sizeof(GrColor);
        }
        return offset;
    }
    return -1;
}

int GrDrawTarget::VertexEdgeOffset(GrVertexLayout vertexLayout) {
    GrAssert(check_layout(vertexLayout));

    
    if (vertexLayout & kEdge_VertexLayoutBit) {
        int vecSize = (vertexLayout & kTextFormat_VertexLayoutBit) ?
                                    sizeof(GrGpuTextVertex) :
                                    sizeof(GrPoint);
        int offset = vecSize * (num_tex_coords(vertexLayout) + 1); 
        if (vertexLayout & kColor_VertexLayoutBit) {
            offset += sizeof(GrColor);
        }
        if (vertexLayout & kCoverage_VertexLayoutBit) {
            offset += sizeof(GrColor);
        }
        return offset;
    }
    return -1;
}

int GrDrawTarget::VertexSizeAndOffsetsByIdx(
        GrVertexLayout vertexLayout,
        int texCoordOffsetsByIdx[GrDrawState::kMaxTexCoords],
        int* colorOffset,
        int* coverageOffset,
        int* edgeOffset) {
    GrAssert(check_layout(vertexLayout));

    int vecSize = (vertexLayout & kTextFormat_VertexLayoutBit) ?
                                                    sizeof(GrGpuTextVertex) :
                                                    sizeof(GrPoint);
    int size = vecSize; 

    for (int t = 0; t < GrDrawState::kMaxTexCoords; ++t) {
        if (gTexCoordMasks[t] & vertexLayout) {
            if (NULL != texCoordOffsetsByIdx) {
                texCoordOffsetsByIdx[t] = size;
            }
            size += vecSize;
        } else {
            if (NULL != texCoordOffsetsByIdx) {
                texCoordOffsetsByIdx[t] = -1;
            }
        }
    }
    if (kColor_VertexLayoutBit & vertexLayout) {
        if (NULL != colorOffset) {
            *colorOffset = size;
        }
        size += sizeof(GrColor);
    } else {
        if (NULL != colorOffset) {
            *colorOffset = -1;
        }
    }
    if (kCoverage_VertexLayoutBit & vertexLayout) {
        if (NULL != coverageOffset) {
            *coverageOffset = size;
        }
        size += sizeof(GrColor);
    } else {
        if (NULL != coverageOffset) {
            *coverageOffset = -1;
        }
    }
    if (kEdge_VertexLayoutBit & vertexLayout) {
        if (NULL != edgeOffset) {
            *edgeOffset = size;
        }
        size += 4 * sizeof(GrScalar);
    } else {
        if (NULL != edgeOffset) {
            *edgeOffset = -1;
        }
    }
    return size;
}

int GrDrawTarget::VertexSizeAndOffsetsByStage(
        GrVertexLayout vertexLayout,
        int texCoordOffsetsByStage[GrDrawState::kNumStages],
        int* colorOffset,
        int* coverageOffset,
        int* edgeOffset) {
    GrAssert(check_layout(vertexLayout));

    int texCoordOffsetsByIdx[GrDrawState::kMaxTexCoords];
    int size = VertexSizeAndOffsetsByIdx(vertexLayout,
                                         (NULL == texCoordOffsetsByStage) ?
                                               NULL :
                                               texCoordOffsetsByIdx,
                                         colorOffset,
                                         coverageOffset,
                                         edgeOffset);
    if (NULL != texCoordOffsetsByStage) {
        for (int s = 0; s < GrDrawState::kNumStages; ++s) {
            int tcIdx = VertexTexCoordsForStage(s, vertexLayout);
            texCoordOffsetsByStage[s] =
                tcIdx < 0 ? 0 : texCoordOffsetsByIdx[tcIdx];
        }
    }
    return size;
}



bool GrDrawTarget::VertexUsesTexCoordIdx(int coordIndex,
                                         GrVertexLayout vertexLayout) {
    GrAssert(coordIndex < GrDrawState::kMaxTexCoords);
    GrAssert(check_layout(vertexLayout));
    return !!(gTexCoordMasks[coordIndex] & vertexLayout);
}

int GrDrawTarget::VertexTexCoordsForStage(int stage,
                                          GrVertexLayout vertexLayout) {
    GrAssert(stage < GrDrawState::kNumStages);
    GrAssert(check_layout(vertexLayout));
    int bit = vertexLayout & gStageTexCoordMasks[stage];
    if (bit) {
        
        
        
        GR_STATIC_ASSERT(sizeof(GrVertexLayout) <= sizeof(uint32_t));
        return (32 - Gr_clz(bit) - 1) / GrDrawState::kNumStages;
    }
    return -1;
}



void GrDrawTarget::VertexLayoutUnitTest() {
    
    GrVertexLayout stageTexCoordMasks[GrDrawState::kNumStages];
    GrVertexLayout texCoordMasks[GrDrawState::kMaxTexCoords];
    gen_mask_arrays(stageTexCoordMasks, texCoordMasks);
    for (int s = 0; s < GrDrawState::kNumStages; ++s) {
        GrAssert(stageTexCoordMasks[s] == gStageTexCoordMasks[s]);
    }
    for (int t = 0; t < GrDrawState::kMaxTexCoords; ++t) {
        GrAssert(texCoordMasks[t] == gTexCoordMasks[t]);
    }

    
    static bool run;
    if (!run) {
        run = true;
        for (int s = 0; s < GrDrawState::kNumStages; ++s) {

            GrVertexLayout stageMask = 0;
            for (int t = 0; t < GrDrawState::kMaxTexCoords; ++t) {
                stageMask |= StageTexCoordVertexLayoutBit(s,t);
            }
            GrAssert(1 == GrDrawState::kMaxTexCoords ||
                     !check_layout(stageMask));
            GrAssert(gStageTexCoordMasks[s] == stageMask);
            GrAssert(!check_layout(stageMask));
        }
        for (int t = 0; t < GrDrawState::kMaxTexCoords; ++t) {
            GrVertexLayout tcMask = 0;
            GrAssert(!VertexUsesTexCoordIdx(t, 0));
            for (int s = 0; s < GrDrawState::kNumStages; ++s) {
                tcMask |= StageTexCoordVertexLayoutBit(s,t);
                GrAssert(sizeof(GrPoint) == VertexStageCoordOffset(s, tcMask));
                GrAssert(VertexUsesTexCoordIdx(t, tcMask));
                GrAssert(2*sizeof(GrPoint) == VertexSize(tcMask));
                GrAssert(t == VertexTexCoordsForStage(s, tcMask));
                for (int s2 = s + 1; s2 < GrDrawState::kNumStages; ++s2) {
                    GrAssert(-1 == VertexTexCoordsForStage(s2, tcMask));

                #if GR_DEBUG
                    GrVertexLayout posAsTex = tcMask;
                #endif
                    GrAssert(0 == VertexStageCoordOffset(s2, posAsTex));
                    GrAssert(2*sizeof(GrPoint) == VertexSize(posAsTex));
                    GrAssert(-1 == VertexTexCoordsForStage(s2, posAsTex));
                    GrAssert(-1 == VertexEdgeOffset(posAsTex));
                }
                GrAssert(-1 == VertexEdgeOffset(tcMask));
                GrAssert(-1 == VertexColorOffset(tcMask));
                GrAssert(-1 == VertexCoverageOffset(tcMask));
            #if GR_DEBUG
                GrVertexLayout withColor = tcMask | kColor_VertexLayoutBit;
            #endif
                GrAssert(-1 == VertexCoverageOffset(withColor));
                GrAssert(2*sizeof(GrPoint) == VertexColorOffset(withColor));
                GrAssert(2*sizeof(GrPoint) + sizeof(GrColor) == VertexSize(withColor));
            #if GR_DEBUG
                GrVertexLayout withEdge = tcMask | kEdge_VertexLayoutBit;
            #endif
                GrAssert(-1 == VertexColorOffset(withEdge));
                GrAssert(2*sizeof(GrPoint) == VertexEdgeOffset(withEdge));
                GrAssert(4*sizeof(GrPoint) == VertexSize(withEdge));
            #if GR_DEBUG
                GrVertexLayout withColorAndEdge = withColor | kEdge_VertexLayoutBit;
            #endif
                GrAssert(2*sizeof(GrPoint) == VertexColorOffset(withColorAndEdge));
                GrAssert(2*sizeof(GrPoint) + sizeof(GrColor) == VertexEdgeOffset(withColorAndEdge));
                GrAssert(4*sizeof(GrPoint) + sizeof(GrColor) == VertexSize(withColorAndEdge));
            #if GR_DEBUG
                GrVertexLayout withCoverage = tcMask | kCoverage_VertexLayoutBit;
            #endif
                GrAssert(-1 == VertexColorOffset(withCoverage));
                GrAssert(2*sizeof(GrPoint) == VertexCoverageOffset(withCoverage));
                GrAssert(2*sizeof(GrPoint) + sizeof(GrColor) == VertexSize(withCoverage));
            #if GR_DEBUG
                GrVertexLayout withCoverageAndColor = tcMask | kCoverage_VertexLayoutBit |
                                                      kColor_VertexLayoutBit;
            #endif
                GrAssert(2*sizeof(GrPoint) == VertexColorOffset(withCoverageAndColor));
                GrAssert(2*sizeof(GrPoint) + sizeof(GrColor) == VertexCoverageOffset(withCoverageAndColor));
                GrAssert(2*sizeof(GrPoint) + 2 * sizeof(GrColor) == VertexSize(withCoverageAndColor));
            }
            GrAssert(gTexCoordMasks[t] == tcMask);
            GrAssert(check_layout(tcMask));

            int stageOffsets[GrDrawState::kNumStages];
            int colorOffset;
            int edgeOffset;
            int coverageOffset;
            int size;
            size = VertexSizeAndOffsetsByStage(tcMask,
                                               stageOffsets, &colorOffset,
                                               &coverageOffset, &edgeOffset);
            GrAssert(2*sizeof(GrPoint) == size);
            GrAssert(-1 == colorOffset);
            GrAssert(-1 == coverageOffset);
            GrAssert(-1 == edgeOffset);
            for (int s = 0; s < GrDrawState::kNumStages; ++s) {
                GrAssert(sizeof(GrPoint) == stageOffsets[s]);
                GrAssert(sizeof(GrPoint) == VertexStageCoordOffset(s, tcMask));
            }
        }
    }
}



#define DEBUG_INVAL_BUFFER 0xdeadcafe
#define DEBUG_INVAL_START_IDX -1

GrDrawTarget::GrDrawTarget() : fClip(NULL) {
#if GR_DEBUG
    VertexLayoutUnitTest();
#endif
    fDrawState = &fDefaultDrawState;
    
    fDefaultDrawState.ref();
    GeometrySrcState& geoSrc = fGeoSrcStateStack.push_back();
#if GR_DEBUG
    geoSrc.fVertexCount = DEBUG_INVAL_START_IDX;
    geoSrc.fVertexBuffer = (GrVertexBuffer*)DEBUG_INVAL_BUFFER;
    geoSrc.fIndexCount = DEBUG_INVAL_START_IDX;
    geoSrc.fIndexBuffer = (GrIndexBuffer*)DEBUG_INVAL_BUFFER;
#endif
    geoSrc.fVertexSrc = kNone_GeometrySrcType;
    geoSrc.fIndexSrc  = kNone_GeometrySrcType;
}

GrDrawTarget::~GrDrawTarget() {
    GrAssert(1 == fGeoSrcStateStack.count());
    GeometrySrcState& geoSrc = fGeoSrcStateStack.back();
    GrAssert(kNone_GeometrySrcType == geoSrc.fIndexSrc);
    GrAssert(kNone_GeometrySrcType == geoSrc.fVertexSrc);
    fDrawState->unref();
}

void GrDrawTarget::releaseGeometry() {
    int popCnt = fGeoSrcStateStack.count() - 1;
    while (popCnt) {
        this->popGeometrySource();
        --popCnt;
    }
    this->resetVertexSource();
    this->resetIndexSource();
}

void GrDrawTarget::setClip(const GrClipData* clip) {
    clipWillBeSet(clip);
    fClip = clip;
}

const GrClipData* GrDrawTarget::getClip() const {
    return fClip;
}

void GrDrawTarget::setDrawState(GrDrawState*  drawState) {
    GrAssert(NULL != fDrawState);
    if (NULL == drawState) {
        drawState = &fDefaultDrawState;
    }
    if (fDrawState != drawState) {
        fDrawState->unref();
        drawState->ref();
        fDrawState = drawState;
    }
}

bool GrDrawTarget::reserveVertexSpace(GrVertexLayout vertexLayout,
                                      int vertexCount,
                                      void** vertices) {
    GeometrySrcState& geoSrc = fGeoSrcStateStack.back();
    bool acquired = false;
    if (vertexCount > 0) {
        GrAssert(NULL != vertices);
        this->releasePreviousVertexSource();
        geoSrc.fVertexSrc = kNone_GeometrySrcType;

        acquired = this->onReserveVertexSpace(vertexLayout,
                                              vertexCount,
                                              vertices);
    }
    if (acquired) {
        geoSrc.fVertexSrc = kReserved_GeometrySrcType;
        geoSrc.fVertexCount = vertexCount;
        geoSrc.fVertexLayout = vertexLayout;
    } else if (NULL != vertices) {
        *vertices = NULL;
    }
    return acquired;
}

bool GrDrawTarget::reserveIndexSpace(int indexCount,
                                     void** indices) {
    GeometrySrcState& geoSrc = fGeoSrcStateStack.back();
    bool acquired = false;
    if (indexCount > 0) {
        GrAssert(NULL != indices);
        this->releasePreviousIndexSource();
        geoSrc.fIndexSrc = kNone_GeometrySrcType;

        acquired = this->onReserveIndexSpace(indexCount, indices);
    }
    if (acquired) {
        geoSrc.fIndexSrc = kReserved_GeometrySrcType;
        geoSrc.fIndexCount = indexCount;
    } else if (NULL != indices) {
        *indices = NULL;
    }
    return acquired;

}

bool GrDrawTarget::StageUsesTexCoords(GrVertexLayout layout, int stage) {
    return SkToBool(layout & gStageTexCoordMasks[stage]);
}

bool GrDrawTarget::reserveVertexAndIndexSpace(GrVertexLayout vertexLayout,
                                              int vertexCount,
                                              int indexCount,
                                              void** vertices,
                                              void** indices) {
    this->willReserveVertexAndIndexSpace(vertexLayout, vertexCount, indexCount);
    if (vertexCount) {
        if (!this->reserveVertexSpace(vertexLayout, vertexCount, vertices)) {
            if (indexCount) {
                this->resetIndexSource();
            }
            return false;
        }
    }
    if (indexCount) {
        if (!this->reserveIndexSpace(indexCount, indices)) {
            if (vertexCount) {
                this->resetVertexSource();
            }
            return false;
        }
    }
    return true;
}

bool GrDrawTarget::geometryHints(GrVertexLayout vertexLayout,
                                 int32_t* vertexCount,
                                 int32_t* indexCount) const {
    if (NULL != vertexCount) {
        *vertexCount = -1;
    }
    if (NULL != indexCount) {
        *indexCount = -1;
    }
    return false;
}

void GrDrawTarget::releasePreviousVertexSource() {
    GeometrySrcState& geoSrc = fGeoSrcStateStack.back();
    switch (geoSrc.fVertexSrc) {
        case kNone_GeometrySrcType:
            break;
        case kArray_GeometrySrcType:
            this->releaseVertexArray();
            break;
        case kReserved_GeometrySrcType:
            this->releaseReservedVertexSpace();
            break;
        case kBuffer_GeometrySrcType:
            geoSrc.fVertexBuffer->unref();
#if GR_DEBUG
            geoSrc.fVertexBuffer = (GrVertexBuffer*)DEBUG_INVAL_BUFFER;
#endif
            break;
        default:
            GrCrash("Unknown Vertex Source Type.");
            break;
    }
}

void GrDrawTarget::releasePreviousIndexSource() {
    GeometrySrcState& geoSrc = fGeoSrcStateStack.back();
    switch (geoSrc.fIndexSrc) {
        case kNone_GeometrySrcType:   
            break;
        case kArray_GeometrySrcType:
            this->releaseIndexArray();
            break;
        case kReserved_GeometrySrcType:
            this->releaseReservedIndexSpace();
            break;
        case kBuffer_GeometrySrcType:
            geoSrc.fIndexBuffer->unref();
#if GR_DEBUG
            geoSrc.fIndexBuffer = (GrIndexBuffer*)DEBUG_INVAL_BUFFER;
#endif
            break;
        default:
            GrCrash("Unknown Index Source Type.");
            break;
    }
}

void GrDrawTarget::setVertexSourceToArray(GrVertexLayout vertexLayout,
                                          const void* vertexArray,
                                          int vertexCount) {
    this->releasePreviousVertexSource();
    GeometrySrcState& geoSrc = fGeoSrcStateStack.back();
    geoSrc.fVertexSrc = kArray_GeometrySrcType;
    geoSrc.fVertexLayout = vertexLayout;
    geoSrc.fVertexCount = vertexCount;
    this->onSetVertexSourceToArray(vertexArray, vertexCount);
}

void GrDrawTarget::setIndexSourceToArray(const void* indexArray,
                                         int indexCount) {
    this->releasePreviousIndexSource();
    GeometrySrcState& geoSrc = fGeoSrcStateStack.back();
    geoSrc.fIndexSrc = kArray_GeometrySrcType;
    geoSrc.fIndexCount = indexCount;
    this->onSetIndexSourceToArray(indexArray, indexCount);
}

void GrDrawTarget::setVertexSourceToBuffer(GrVertexLayout vertexLayout,
                                           const GrVertexBuffer* buffer) {
    this->releasePreviousVertexSource();
    GeometrySrcState& geoSrc = fGeoSrcStateStack.back();
    geoSrc.fVertexSrc    = kBuffer_GeometrySrcType;
    geoSrc.fVertexBuffer = buffer;
    buffer->ref();
    geoSrc.fVertexLayout = vertexLayout;
}

void GrDrawTarget::setIndexSourceToBuffer(const GrIndexBuffer* buffer) {
    this->releasePreviousIndexSource();
    GeometrySrcState& geoSrc = fGeoSrcStateStack.back();
    geoSrc.fIndexSrc     = kBuffer_GeometrySrcType;
    geoSrc.fIndexBuffer  = buffer;
    buffer->ref();
}

void GrDrawTarget::resetVertexSource() {
    this->releasePreviousVertexSource();
    GeometrySrcState& geoSrc = fGeoSrcStateStack.back();
    geoSrc.fVertexSrc = kNone_GeometrySrcType;
}

void GrDrawTarget::resetIndexSource() {
    this->releasePreviousIndexSource();
    GeometrySrcState& geoSrc = fGeoSrcStateStack.back();
    geoSrc.fIndexSrc = kNone_GeometrySrcType;
}

void GrDrawTarget::pushGeometrySource() {
    this->geometrySourceWillPush();
    GeometrySrcState& newState = fGeoSrcStateStack.push_back();
    newState.fIndexSrc = kNone_GeometrySrcType;
    newState.fVertexSrc = kNone_GeometrySrcType;
#if GR_DEBUG
    newState.fVertexCount  = ~0;
    newState.fVertexBuffer = (GrVertexBuffer*)~0;
    newState.fIndexCount   = ~0;
    newState.fIndexBuffer = (GrIndexBuffer*)~0;
#endif
}

void GrDrawTarget::popGeometrySource() {
    
    GrAssert(fGeoSrcStateStack.count() > 1);

    this->geometrySourceWillPop(fGeoSrcStateStack.fromBack(1));
    this->releasePreviousVertexSource();
    this->releasePreviousIndexSource();
    fGeoSrcStateStack.pop_back();
}



bool GrDrawTarget::checkDraw(GrPrimitiveType type, int startVertex,
                             int startIndex, int vertexCount,
                             int indexCount) const {
    const GrDrawState& drawState = this->getDrawState();
#if GR_DEBUG
    const GeometrySrcState& geoSrc = fGeoSrcStateStack.back();
    int maxVertex = startVertex + vertexCount;
    int maxValidVertex;
    switch (geoSrc.fVertexSrc) {
        case kNone_GeometrySrcType:
            GrCrash("Attempting to draw without vertex src.");
        case kReserved_GeometrySrcType: 
        case kArray_GeometrySrcType:
            maxValidVertex = geoSrc.fVertexCount;
            break;
        case kBuffer_GeometrySrcType:
            maxValidVertex = geoSrc.fVertexBuffer->sizeInBytes() / VertexSize(geoSrc.fVertexLayout);
            break;
    }
    if (maxVertex > maxValidVertex) {
        GrCrash("Drawing outside valid vertex range.");
    }
    if (indexCount > 0) {
        int maxIndex = startIndex + indexCount;
        int maxValidIndex;
        switch (geoSrc.fIndexSrc) {
            case kNone_GeometrySrcType:
                GrCrash("Attempting to draw indexed geom without index src.");
            case kReserved_GeometrySrcType: 
            case kArray_GeometrySrcType:
                maxValidIndex = geoSrc.fIndexCount;
                break;
            case kBuffer_GeometrySrcType:
                maxValidIndex = geoSrc.fIndexBuffer->sizeInBytes() / sizeof(uint16_t);
                break;
        }
        if (maxIndex > maxValidIndex) {
            GrCrash("Index reads outside valid index range.");
        }
    }

    GrAssert(NULL != drawState.getRenderTarget());
    for (int s = 0; s < GrDrawState::kNumStages; ++s) {
        if (drawState.isStageEnabled(s)) {
            const GrCustomStage* stage = drawState.getSampler(s).getCustomStage();
            int numTextures = stage->numTextures();
            for (int t = 0; t < numTextures; ++t) {
                GrTexture* texture = stage->texture(t);
                GrAssert(texture->asRenderTarget() != drawState.getRenderTarget());
            }
        }
    }
#endif
    if (NULL == drawState.getRenderTarget()) {
        return false;
    }
    return true;
}

void GrDrawTarget::drawIndexed(GrPrimitiveType type, int startVertex,
                               int startIndex, int vertexCount,
                               int indexCount) {
    if (indexCount > 0 &&
        this->checkDraw(type, startVertex, startIndex,
                        vertexCount, indexCount)) {
        this->onDrawIndexed(type, startVertex, startIndex,
                            vertexCount, indexCount);
    }
}

void GrDrawTarget::drawNonIndexed(GrPrimitiveType type,
                                  int startVertex,
                                  int vertexCount) {
    if (vertexCount > 0 &&
        this->checkDraw(type, startVertex, -1, vertexCount, -1)) {
        this->onDrawNonIndexed(type, startVertex, vertexCount);
    }
}

void GrDrawTarget::stencilPath(const GrPath* path, GrPathFill fill) {
    
    GrAssert(NULL != path);
    GrAssert(fCaps.pathStencilingSupport());
    GrAssert(kHairLine_GrPathFill != fill);
    GrAssert(!GrIsFillInverted(fill));
    this->onStencilPath(path, fill);
}





bool GrDrawTarget::canTweakAlphaForCoverage() const {
    










    GrBlendCoeff dstCoeff = this->getDrawState().getDstBlendCoeff();
    return kOne_GrBlendCoeff == dstCoeff ||
           kISA_GrBlendCoeff == dstCoeff ||
           kISC_GrBlendCoeff == dstCoeff;
}

bool GrDrawTarget::srcAlphaWillBeOne(GrVertexLayout layout) const {
    const GrDrawState& drawState = this->getDrawState();

    
    if ((layout & kColor_VertexLayoutBit) ||
        0xff != GrColorUnpackA(drawState.getColor())) {
        return false;
    }
    
    
    
    if (SkXfermode::kDst_Mode != drawState.getColorFilterMode()) {
        return false;
    }
    
    for (int s = 0; s < drawState.getFirstCoverageStage(); ++s) {
        if (this->isStageEnabled(s)) {
            const GrCustomStage* stage = drawState.getSampler(s).getCustomStage();
            
            
            
            if (!stage->isOpaque(true)) {
                return false;
            }
        }
    }
    return true;
}

namespace {
GrVertexLayout default_blend_opts_vertex_layout() {
    GrVertexLayout layout = 0;
    return layout;
}
}

GrDrawTarget::BlendOptFlags
GrDrawTarget::getBlendOpts(bool forceCoverage,
                           GrBlendCoeff* srcCoeff,
                           GrBlendCoeff* dstCoeff) const {

    GrVertexLayout layout;
    if (kNone_GeometrySrcType == this->getGeomSrc().fVertexSrc) {
        layout = default_blend_opts_vertex_layout();
    } else {
        layout = this->getVertexLayout();
    }

    const GrDrawState& drawState = this->getDrawState();

    GrBlendCoeff bogusSrcCoeff, bogusDstCoeff;
    if (NULL == srcCoeff) {
        srcCoeff = &bogusSrcCoeff;
    }
    *srcCoeff = drawState.getSrcBlendCoeff();

    if (NULL == dstCoeff) {
        dstCoeff = &bogusDstCoeff;
    }
    *dstCoeff = drawState.getDstBlendCoeff();

    
    GrAssert(kSA_GrBlendCoeff != *srcCoeff &&
             kISA_GrBlendCoeff != *srcCoeff &&
             kSC_GrBlendCoeff != *srcCoeff &&
             kISC_GrBlendCoeff != *srcCoeff);
    
    GrAssert(kDA_GrBlendCoeff != *dstCoeff &&
             kIDA_GrBlendCoeff != *dstCoeff &&
             kDC_GrBlendCoeff != *dstCoeff &&
             kIDC_GrBlendCoeff != *dstCoeff);

    if (drawState.isColorWriteDisabled()) {
        *srcCoeff = kZero_GrBlendCoeff;
        *dstCoeff = kOne_GrBlendCoeff;
    }

    bool srcAIsOne = this->srcAlphaWillBeOne(layout);
    bool dstCoeffIsOne = kOne_GrBlendCoeff == *dstCoeff ||
                         (kSA_GrBlendCoeff == *dstCoeff && srcAIsOne);
    bool dstCoeffIsZero = kZero_GrBlendCoeff == *dstCoeff ||
                         (kISA_GrBlendCoeff == *dstCoeff && srcAIsOne);


    
    
    
    if ((kZero_GrBlendCoeff == *srcCoeff && dstCoeffIsOne) ||
        (!(layout & kCoverage_VertexLayoutBit) &&
         0 == drawState.getCoverage())) {
        if (drawState.getStencil().doesWrite()) {
            return kDisableBlend_BlendOptFlag |
                   kEmitTransBlack_BlendOptFlag;
        } else {
            return kSkipDraw_BlendOptFlag;
        }
    }

    
    
    bool hasCoverage = forceCoverage ||
                       0xffffffff != drawState.getCoverage() ||
                       (layout & kCoverage_VertexLayoutBit) ||
                       (layout & kEdge_VertexLayoutBit);
    for (int s = drawState.getFirstCoverageStage();
         !hasCoverage && s < GrDrawState::kNumStages;
         ++s) {
        if (this->isStageEnabled(s)) {
            hasCoverage = true;
        }
    }

    
    
    if (!hasCoverage) {
        if (dstCoeffIsZero) {
            if (kOne_GrBlendCoeff == *srcCoeff) {
                
                
                return kDisableBlend_BlendOptFlag;
            } else if (kZero_GrBlendCoeff == *srcCoeff) {
                
                
                *srcCoeff = kOne_GrBlendCoeff;
                *dstCoeff = kZero_GrBlendCoeff;
                return kDisableBlend_BlendOptFlag |
                       kEmitTransBlack_BlendOptFlag;
            }
        }
    } else {
        
        
        if (this->canTweakAlphaForCoverage()) {
            return kCoverageAsAlpha_BlendOptFlag;
        }
        if (dstCoeffIsZero) {
            if (kZero_GrBlendCoeff == *srcCoeff) {
                
                
                
                *dstCoeff = kISA_GrBlendCoeff;
                return  kEmitCoverage_BlendOptFlag;
            } else if (srcAIsOne) {
                
                
                
                
                *dstCoeff = kISA_GrBlendCoeff;
                return  kCoverageAsAlpha_BlendOptFlag;
            }
        } else if (dstCoeffIsOne) {
            
            
            *dstCoeff = kOne_GrBlendCoeff;
            return  kCoverageAsAlpha_BlendOptFlag;
        }
    }
    return kNone_BlendOpt;
}

bool GrDrawTarget::willUseHWAALines() const {
    
    
    
    
    if (!fCaps.hwAALineSupport() ||
        !this->getDrawState().isHWAntialiasState()) {
        return false;
    }
    BlendOptFlags opts = this->getBlendOpts();
    return (kDisableBlend_BlendOptFlag & opts) &&
           (kCoverageAsAlpha_BlendOptFlag & opts);
}

bool GrDrawTarget::canApplyCoverage() const {
    
    
    return this->getCaps().dualSourceBlendingSupport() ||
           kNone_BlendOpt != this->getBlendOpts(true);
}



void GrDrawTarget::drawIndexedInstances(GrPrimitiveType type,
                                        int instanceCount,
                                        int verticesPerInstance,
                                        int indicesPerInstance) {
    if (!verticesPerInstance || !indicesPerInstance) {
        return;
    }

    int instancesPerDraw = this->indexCountInCurrentSource() /
                           indicesPerInstance;
    if (!instancesPerDraw) {
        return;
    }

    instancesPerDraw = GrMin(instanceCount, instancesPerDraw);
    int startVertex = 0;
    while (instanceCount) {
        this->drawIndexed(type,
                          startVertex,
                          0,
                          verticesPerInstance * instancesPerDraw,
                          indicesPerInstance * instancesPerDraw);
        startVertex += verticesPerInstance;
        instanceCount -= instancesPerDraw;
    }
}



void GrDrawTarget::drawRect(const GrRect& rect,
                            const GrMatrix* matrix,
                            const GrRect* srcRects[],
                            const GrMatrix* srcMatrices[]) {
    GrVertexLayout layout = GetRectVertexLayout(srcRects);

    AutoReleaseGeometry geo(this, layout, 4, 0);
    if (!geo.succeeded()) {
        GrPrintf("Failed to get space for vertices!\n");
        return;
    }

    SetRectVertices(rect, matrix, srcRects,
                    srcMatrices, layout, geo.vertices());

    drawNonIndexed(kTriangleFan_GrPrimitiveType, 0, 4);
}

GrVertexLayout GrDrawTarget::GetRectVertexLayout(const GrRect* srcRects[]) {
    if (NULL == srcRects) {
        return 0;
    }

    GrVertexLayout layout = 0;
    for (int i = 0; i < GrDrawState::kNumStages; ++i) {
        int numTC = 0;
        if (NULL != srcRects[i]) {
            layout |= StageTexCoordVertexLayoutBit(i, numTC);
            ++numTC;
        }
    }
    return layout;
}

void GrDrawTarget::SetRectVertices(const GrRect& rect,
                                   const GrMatrix* matrix,
                                   const GrRect* srcRects[],
                                   const GrMatrix* srcMatrices[],
                                   GrVertexLayout layout,
                                   void* vertices) {
#if GR_DEBUG
    
    for (int i = 0; i < GrDrawState::kNumStages; ++i) {
        if (VertexTexCoordsForStage(i, layout) >= 0) {
            GR_DEBUGASSERT(NULL != srcRects && NULL != srcRects[i]);
        } else {
            GR_DEBUGASSERT(NULL == srcRects || NULL == srcRects[i]);
        }
    }
#endif

    int stageOffsets[GrDrawState::kNumStages];
    int vsize = VertexSizeAndOffsetsByStage(layout, stageOffsets,
                                            NULL, NULL, NULL);

    GrTCast<GrPoint*>(vertices)->setRectFan(rect.fLeft, rect.fTop,
                                            rect.fRight, rect.fBottom,
                                            vsize);
    if (NULL != matrix) {
        matrix->mapPointsWithStride(GrTCast<GrPoint*>(vertices), vsize, 4);
    }

    for (int i = 0; i < GrDrawState::kNumStages; ++i) {
        if (stageOffsets[i] > 0) {
            GrPoint* coords = GrTCast<GrPoint*>(GrTCast<intptr_t>(vertices) +
                                                stageOffsets[i]);
            coords->setRectFan(srcRects[i]->fLeft, srcRects[i]->fTop,
                               srcRects[i]->fRight, srcRects[i]->fBottom,
                               vsize);
            if (NULL != srcMatrices && NULL != srcMatrices[i]) {
                srcMatrices[i]->mapPointsWithStride(coords, vsize, 4);
            }
        }
    }
}



GrDrawTarget::AutoStateRestore::AutoStateRestore() {
    fDrawTarget = NULL;
}

GrDrawTarget::AutoStateRestore::AutoStateRestore(GrDrawTarget* target,
                                                 ASRInit init) {
    fDrawTarget = NULL;
    this->set(target, init);
}

GrDrawTarget::AutoStateRestore::~AutoStateRestore() {
    if (NULL != fDrawTarget) {
        fDrawTarget->setDrawState(fSavedState);
        fSavedState->unref();
    }
}

void GrDrawTarget::AutoStateRestore::set(GrDrawTarget* target, ASRInit init) {
    GrAssert(NULL == fDrawTarget);
    fDrawTarget = target;
    fSavedState = target->drawState();
    GrAssert(fSavedState);
    fSavedState->ref();
    if (kReset_ASRInit == init) {
        
        fTempState.init();
    } else {
        GrAssert(kPreserve_ASRInit == init);
        
        fTempState.set(*fSavedState);
    }
    target->setDrawState(fTempState.get());
}



GrDrawTarget::AutoDeviceCoordDraw::AutoDeviceCoordDraw(GrDrawTarget* target,
                                                       uint32_t explicitCoordStageMask) {
    GrAssert(NULL != target);
    GrDrawState* drawState = target->drawState();

    fDrawTarget = target;
    fViewMatrix = drawState->getViewMatrix();
    fRestoreMask = 0;
    GrMatrix invVM;
    bool inverted = false;

    for (int s = 0; s < GrDrawState::kNumStages; ++s) {
        if (!(explicitCoordStageMask & (1 << s)) && drawState->isStageEnabled(s)) {
            if (!inverted && !fViewMatrix.invert(&invVM)) {
                
                fDrawTarget = NULL;
                return;
            } else {
                inverted = true;
            }
            fRestoreMask |= (1 << s);
            GrSamplerState* sampler = drawState->sampler(s);
            fSamplerMatrices[s] = sampler->getMatrix();
            sampler->preConcatMatrix(invVM);
        }
    }
    drawState->viewMatrix()->reset();
}

GrDrawTarget::AutoDeviceCoordDraw::~AutoDeviceCoordDraw() {
    GrDrawState* drawState = fDrawTarget->drawState();
    drawState->setViewMatrix(fViewMatrix);
    for (int s = 0; s < GrDrawState::kNumStages; ++s) {
        if (fRestoreMask & (1 << s)) {
            *drawState->sampler(s)->matrix() = fSamplerMatrices[s];
        }
    }
}



GrDrawTarget::AutoReleaseGeometry::AutoReleaseGeometry(
                                         GrDrawTarget*  target,
                                         GrVertexLayout vertexLayout,
                                         int vertexCount,
                                         int indexCount) {
    fTarget = NULL;
    this->set(target, vertexLayout, vertexCount, indexCount);
}

GrDrawTarget::AutoReleaseGeometry::AutoReleaseGeometry() {
    fTarget = NULL;
}

GrDrawTarget::AutoReleaseGeometry::~AutoReleaseGeometry() {
    this->reset();
}

bool GrDrawTarget::AutoReleaseGeometry::set(GrDrawTarget*  target,
                                            GrVertexLayout vertexLayout,
                                            int vertexCount,
                                            int indexCount) {
    this->reset();
    fTarget = target;
    bool success = true;
    if (NULL != fTarget) {
        fTarget = target;
        success = target->reserveVertexAndIndexSpace(vertexLayout,
                                                     vertexCount,
                                                     indexCount,
                                                     &fVertices,
                                                     &fIndices);
        if (!success) {
            fTarget = NULL;
            this->reset();
        }
    }
    GrAssert(success == (NULL != fTarget));
    return success;
}

void GrDrawTarget::AutoReleaseGeometry::reset() {
    if (NULL != fTarget) {
        if (NULL != fVertices) {
            fTarget->resetVertexSource();
        }
        if (NULL != fIndices) {
            fTarget->resetIndexSource();
        }
        fTarget = NULL;
    }
    fVertices = NULL;
    fIndices = NULL;
}

void GrDrawTarget::Caps::print() const {
    static const char* gNY[] = {"NO", "YES"};
    GrPrintf("8 Bit Palette Support       : %s\n", gNY[fInternals.f8BitPaletteSupport]);
    GrPrintf("NPOT Texture Tile Support   : %s\n", gNY[fInternals.fNPOTTextureTileSupport]);
    GrPrintf("Two Sided Stencil Support   : %s\n", gNY[fInternals.fTwoSidedStencilSupport]);
    GrPrintf("Stencil Wrap Ops  Support   : %s\n", gNY[fInternals.fStencilWrapOpsSupport]);
    GrPrintf("HW AA Lines Support         : %s\n", gNY[fInternals.fHWAALineSupport]);
    GrPrintf("Shader Derivative Support   : %s\n", gNY[fInternals.fShaderDerivativeSupport]);
    GrPrintf("Geometry Shader Support     : %s\n", gNY[fInternals.fGeometryShaderSupport]);
    GrPrintf("FSAA Support                : %s\n", gNY[fInternals.fFSAASupport]);
    GrPrintf("Dual Source Blending Support: %s\n", gNY[fInternals.fDualSourceBlendingSupport]);
    GrPrintf("Buffer Lock Support         : %s\n", gNY[fInternals.fBufferLockSupport]);
    GrPrintf("Path Stenciling Support     : %s\n", gNY[fInternals.fPathStencilingSupport]);
    GrPrintf("Max Texture Size            : %d\n", fInternals.fMaxTextureSize);
    GrPrintf("Max Render Target Size      : %d\n", fInternals.fMaxRenderTargetSize);
}
