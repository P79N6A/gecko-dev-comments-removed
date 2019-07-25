






#ifndef GrDrawState_DEFINED
#define GrDrawState_DEFINED

#include "GrColor.h"
#include "GrMatrix.h"
#include "GrSamplerState.h"
#include "GrStencil.h"

#include "SkXfermode.h"

class GrRenderTarget;
class GrTexture;

struct GrDrawState {

    













    enum {
        kNumStages = 3,
        kMaxTexCoords = kNumStages
    };

    enum DrawFace {
        kBoth_DrawFace,
        kCCW_DrawFace,
        kCW_DrawFace,
    };

     




    enum VertexEdgeType {
        

        kHairLine_EdgeType,
        

        kHairQuad_EdgeType
    };

    





    enum {
        kMaxEdges = 32
    };

    class Edge {
      public:
        Edge() {}
        Edge(float x, float y, float z) : fX(x), fY(y), fZ(z) {}
        GrPoint intersect(const Edge& other) {
            return GrPoint::Make(
                (fY * other.fZ - other.fY * fZ) /
                  (fX * other.fY - other.fX * fY),
                (fX * other.fZ - other.fX * fZ) /
                  (other.fX * fY - fX * other.fY));
        }
        float fX, fY, fZ;
    };

    GrDrawState() {
        
        
        
        memset(this, 0, sizeof(GrDrawState));
            
        
        fColorFilterXfermode = SkXfermode::kDstIn_Mode;
        fFirstCoverageStage = kNumStages;

        
        
        GrAssert((intptr_t)(void*)NULL == 0LL);

        
        GrAssert(fStencilSettings.isDisabled());
        fFirstCoverageStage = kNumStages;
    }

    uint32_t                fFlagBits;
    GrBlendCoeff            fSrcBlend;
    GrBlendCoeff            fDstBlend;
    GrColor                 fBlendConstant;
    GrTexture*              fTextures[kNumStages];
    GrSamplerState          fSamplerStates[kNumStages];
    int                     fFirstCoverageStage;
    GrRenderTarget*         fRenderTarget;
    GrColor                 fColor;
    DrawFace                fDrawFace;
    GrColor                 fColorFilterColor;
    SkXfermode::Mode        fColorFilterXfermode;

    GrStencilSettings       fStencilSettings;
    GrMatrix                fViewMatrix;
    VertexEdgeType          fVertexEdgeType;
    Edge                    fEdgeAAEdges[kMaxEdges];
    int                     fEdgeAANumEdges;
    bool operator ==(const GrDrawState& s) const {
        return 0 == memcmp(this, &s, sizeof(GrDrawState));
    }
    bool operator !=(const GrDrawState& s) const { return !(*this == s); }
};

#endif
