







#ifndef GrGLProgram_DEFINED
#define GrGLProgram_DEFINED

#include "GrDrawState.h"
#include "GrGLContext.h"
#include "GrGLProgramDesc.h"
#include "GrGLShaderBuilder.h"
#include "GrGLSL.h"
#include "GrGLTexture.h"
#include "GrGLUniformManager.h"

#include "SkString.h"
#include "SkXfermode.h"

class GrBinHashKeyBuilder;
class GrGLEffect;
class GrGLProgramEffects;
class GrGLShaderBuilder;










class GrGLProgram : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(GrGLProgram)

    static GrGLProgram* Create(GrGpuGL* gpu,
                               const GrGLProgramDesc& desc,
                               const GrEffectStage* colorStages[],
                               const GrEffectStage* coverageStages[]);

    virtual ~GrGLProgram();

    


    void abandon();

    


    void overrideBlend(GrBlendCoeff* srcCoeff, GrBlendCoeff* dstCoeff) const;

    const GrGLProgramDesc& getDesc() { return fDesc; }

    


    GrGLuint programID() const { return fProgramID; }

    bool hasVertexShader() const { return fHasVertexShader; }

    





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
        template<int Size> void getGLMatrix(GrGLfloat* destMatrix) {
            SkMatrix combined;
            if (kBottomLeft_GrSurfaceOrigin == fRenderTargetOrigin) {
                combined.setAll(SkIntToScalar(2) / fRenderTargetSize.fWidth, 0, -SK_Scalar1,
                                0, -SkIntToScalar(2) / fRenderTargetSize.fHeight, SK_Scalar1,
                                0, 0, SkMatrix::I()[8]);
            } else {
                combined.setAll(SkIntToScalar(2) / fRenderTargetSize.fWidth, 0, -SK_Scalar1,
                                0, SkIntToScalar(2) / fRenderTargetSize.fHeight, -SK_Scalar1,
                                0, 0, SkMatrix::I()[8]);
            }
            combined.setConcat(combined, fViewMatrix);
            GrGLGetMatrix<Size>(destMatrix, combined);
        }
    };

    





    void setData(GrDrawState::BlendOptFlags,
                 const GrEffectStage* colorStages[],
                 const GrEffectStage* coverageStages[],
                 const GrDeviceCoordTexture* dstCopy, 
                 SharedGLState*);

private:
    typedef GrGLUniformManager::UniformHandle UniformHandle;

    
    struct UniformHandles {
        UniformHandle       fViewMatrixUni;
        UniformHandle       fColorUni;
        UniformHandle       fCoverageUni;

        
        
        UniformHandle       fRTHeightUni;

        
        UniformHandle       fDstCopyTopLeftUni;
        UniformHandle       fDstCopyScaleUni;
        UniformHandle       fDstCopySamplerUni;
    };

    GrGLProgram(GrGpuGL* gpu,
                const GrGLProgramDesc& desc,
                const GrEffectStage* colorStages[],
                const GrEffectStage* coverageStages[]);

    bool succeeded() const { return 0 != fProgramID; }

    



    bool genProgram(GrGLShaderBuilder* builder,
                    const GrEffectStage* colorStages[],
                    const GrEffectStage* coverageStages[]);

    
    void initSamplerUniforms();

    
    
    void setColor(const GrDrawState&, GrColor color, SharedGLState*);

    
    
    void setCoverage(const GrDrawState&, GrColor coverage, SharedGLState*);

    
    void setMatrixAndRenderTargetHeight(const GrDrawState&);

    
    GrGLuint                    fProgramID;

    
    MatrixState                       fMatrixState;
    GrColor                           fColor;
    GrColor                           fCoverage;
    int                               fDstCopyTexUnit;

    SkAutoTDelete<GrGLProgramEffects> fColorEffects;
    SkAutoTDelete<GrGLProgramEffects> fCoverageEffects;

    GrGLProgramDesc                   fDesc;
    GrGpuGL*                          fGpu;

    GrGLUniformManager                fUniformManager;
    UniformHandles                    fUniformHandles;

    bool                              fHasVertexShader;
    int                               fNumTexCoordSets;

    typedef SkRefCnt INHERITED;
};

#endif
