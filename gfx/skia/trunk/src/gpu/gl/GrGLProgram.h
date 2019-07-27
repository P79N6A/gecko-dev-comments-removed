







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

    


    GrGLuint programID() const { return fBuilderOutput.fProgramID; }

    bool hasVertexShader() const { return fBuilderOutput.fHasVertexShader; }

    





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
            GrGLGetMatrix<Size>(destMatrix, fViewMatrix);
        }

        


        template<int Size> void getRTAdjustedGLMatrix(GrGLfloat* destMatrix) {
            SkMatrix combined;
            if (kBottomLeft_GrSurfaceOrigin == fRenderTargetOrigin) {
                combined.setAll(SkIntToScalar(2) / fRenderTargetSize.fWidth, 0, -SK_Scalar1,
                                0, -SkIntToScalar(2) / fRenderTargetSize.fHeight, SK_Scalar1,
                                0, 0, 1);
            } else {
                combined.setAll(SkIntToScalar(2) / fRenderTargetSize.fWidth, 0, -SK_Scalar1,
                                0, SkIntToScalar(2) / fRenderTargetSize.fHeight, -SK_Scalar1,
                                0, 0, 1);
            }
            combined.preConcat(fViewMatrix);
            GrGLGetMatrix<Size>(destMatrix, combined);
        }

        






        void getRTAdjustmentVec(GrGLfloat* destVec) {
            destVec[0] = 2.f / fRenderTargetSize.fWidth;
            destVec[1] = -1.f;
            if (kBottomLeft_GrSurfaceOrigin == fRenderTargetOrigin) {
                destVec[2] = -2.f / fRenderTargetSize.fHeight;
                destVec[3] = 1.f;
            } else {
                destVec[2] = 2.f / fRenderTargetSize.fHeight;
                destVec[3] = -1.f;
            }
        }
    };

    





    void setData(GrDrawState::BlendOptFlags,
                 const GrEffectStage* colorStages[],
                 const GrEffectStage* coverageStages[],
                 const GrDeviceCoordTexture* dstCopy, 
                 SharedGLState*);

private:
    typedef GrGLUniformManager::UniformHandle UniformHandle;

    GrGLProgram(GrGpuGL*,
                const GrGLProgramDesc&,
                GrGLUniformManager*,
                const GrGLShaderBuilder::GenProgramOutput&);

    
    void initSamplerUniforms();

    
    
    void setColor(const GrDrawState&, GrColor color, SharedGLState*);

    
    
    void setCoverage(const GrDrawState&, GrColor coverage, SharedGLState*);

    
    void setMatrixAndRenderTargetHeight(const GrDrawState&);

    
    MatrixState                         fMatrixState;
    GrColor                             fColor;
    GrColor                             fCoverage;
    int                                 fDstCopyTexUnit;

    GrGLShaderBuilder::GenProgramOutput fBuilderOutput;

    GrGLProgramDesc                     fDesc;
    GrGpuGL*                            fGpu;

    SkAutoTUnref<GrGLUniformManager>    fUniformManager;

    typedef SkRefCnt INHERITED;
};

#endif
