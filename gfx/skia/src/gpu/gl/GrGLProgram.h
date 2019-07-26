







#ifndef GrGLProgram_DEFINED
#define GrGLProgram_DEFINED

#include "GrDrawState.h"
#include "GrGLContext.h"
#include "GrGLProgramDesc.h"
#include "GrGLSL.h"
#include "GrGLTexture.h"
#include "GrGLUniformManager.h"

#include "SkString.h"
#include "SkXfermode.h"

class GrBinHashKeyBuilder;
class GrGLEffect;
class GrGLShaderBuilder;










class GrGLProgram : public GrRefCnt {
public:
    SK_DECLARE_INST_COUNT(GrGLProgram)

    static GrGLProgram* Create(const GrGLContext& gl,
                               const GrGLProgramDesc& desc,
                               const GrEffectStage* stages[]);

    virtual ~GrGLProgram();

    


    void abandon();

    


    void overrideBlend(GrBlendCoeff* srcCoeff, GrBlendCoeff* dstCoeff) const;

    const GrGLProgramDesc& getDesc() { return fDesc; }

    


    GrGLuint programID() const { return fProgramID; }

    





    struct SharedGLState {
        GrColor fConstAttribColor;
        int     fConstAttribColorIndex;
        GrColor fConstAttribCoverage;
        int     fConstAttribCoverageIndex;

        SharedGLState() { this->invalidate(); }
        void invalidate() {
            fConstAttribColor = GrColor_ILLEGAL;
            fConstAttribColorIndex = -1;
            fConstAttribCoverage = GrColor_ILLEGAL;
            fConstAttribCoverageIndex = -1;
        }
    };

    





    struct MatrixState {
        SkMatrix        fViewMatrix;
        SkISize         fRenderTargetSize;
        GrSurfaceOrigin fRenderTargetOrigin;

        MatrixState() { this->invalidate(); }
        void invalidate() {
            fViewMatrix = SkMatrix::InvalidMatrix();
            fRenderTargetSize.fWidth = -1;
            fRenderTargetSize.fHeight = -1;
            fRenderTargetOrigin = (GrSurfaceOrigin) -1;
        }
    };

    






    void setData(GrGpuGL*,
                 GrColor color,
                 GrColor coverage,
                 const GrDeviceCoordTexture* dstCopy, 
                 SharedGLState*);

private:
    GrGLProgram(const GrGLContext& gl,
                const GrGLProgramDesc& desc,
                const GrEffectStage* stages[]);

    bool succeeded() const { return 0 != fProgramID; }

    


    bool genProgram(const GrEffectStage* stages[]);

    void genInputColor(GrGLShaderBuilder* builder, SkString* inColor);

    void genGeometryShader(GrGLShaderBuilder* segments) const;

    typedef GrGLUniformManager::UniformHandle UniformHandle;

    void genUniformCoverage(GrGLShaderBuilder* segments, SkString* inOutCoverage);

    
    bool bindOutputsAttribsAndLinkProgram(const GrGLShaderBuilder& builder,
                                          bool bindColorOut,
                                          bool bindDualSrcOut);

    
    void initSamplerUniforms();

    bool compileShaders(const GrGLShaderBuilder& builder);

    const char* adjustInColor(const SkString& inColor) const;

    
    
    void setColor(const GrDrawState&, GrColor color, SharedGLState*);

    
    
    void setCoverage(const GrDrawState&, GrColor coverage, SharedGLState*);

    
    void setMatrixAndRenderTargetHeight(const GrDrawState&);

    typedef SkSTArray<4, UniformHandle, true> SamplerUniSArray;

    struct UniformHandles {
        UniformHandle       fViewMatrixUni;
        UniformHandle       fColorUni;
        UniformHandle       fCoverageUni;
        UniformHandle       fColorFilterUni;

        
        
        UniformHandle       fRTHeightUni;

        
        UniformHandle       fDstCopyTopLeftUni;
        UniformHandle       fDstCopyScaleUni;
        UniformHandle       fDstCopySamplerUni;

        
        SamplerUniSArray    fEffectSamplerUnis[GrDrawState::kNumStages];

        UniformHandles() {
            fViewMatrixUni = GrGLUniformManager::kInvalidUniformHandle;
            fColorUni = GrGLUniformManager::kInvalidUniformHandle;
            fCoverageUni = GrGLUniformManager::kInvalidUniformHandle;
            fColorFilterUni = GrGLUniformManager::kInvalidUniformHandle;
            fRTHeightUni = GrGLUniformManager::kInvalidUniformHandle;
            fDstCopyTopLeftUni = GrGLUniformManager::kInvalidUniformHandle;
            fDstCopyScaleUni = GrGLUniformManager::kInvalidUniformHandle;
            fDstCopySamplerUni = GrGLUniformManager::kInvalidUniformHandle;
        }
    };

    
    GrGLuint                    fVShaderID;
    GrGLuint                    fGShaderID;
    GrGLuint                    fFShaderID;
    GrGLuint                    fProgramID;

    
    MatrixState                 fMatrixState;
    GrColor                     fColor;
    GrColor                     fCoverage;
    GrColor                     fColorFilterColor;

    GrGLEffect*                 fEffects[GrDrawState::kNumStages];

    GrGLProgramDesc             fDesc;
    const GrGLContext&          fContext;

    GrGLUniformManager          fUniformManager;
    UniformHandles              fUniformHandles;

    typedef GrRefCnt INHERITED;
};

#endif
