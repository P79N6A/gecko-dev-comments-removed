







#ifndef GrGLProgram_DEFINED
#define GrGLProgram_DEFINED

#include "GrDrawState.h"
#include "GrGLContextInfo.h"
#include "GrGLSL.h"
#include "GrGLTexture.h"
#include "GrGLUniformManager.h"

#include "SkString.h"
#include "SkXfermode.h"

class GrBinHashKeyBuilder;
class GrGLProgramStage;
class GrGLShaderBuilder;



#define GR_GL_EXPERIMENTAL_GS GR_DEBUG










class GrGLProgram : public GrRefCnt {
public:
    SK_DECLARE_INST_COUNT(GrGLProgram)

    struct Desc;

    static GrGLProgram* Create(const GrGLContextInfo& gl,
                               const Desc& desc,
                               const GrCustomStage** customStages);

    virtual ~GrGLProgram();

    
    void abandon();

    


    void overrideBlend(GrBlendCoeff* srcCoeff, GrBlendCoeff* dstCoeff) const;

    const Desc& getDesc() { return fDesc; }

    


    static int PositionAttributeIdx() { return 0; }
    static int TexCoordAttributeIdx(int tcIdx) { return 1 + tcIdx; }
    static int ColorAttributeIdx() { return 1 + GrDrawState::kMaxTexCoords; }
    static int CoverageAttributeIdx() {
        return 2 + GrDrawState::kMaxTexCoords;
    }
    static int EdgeAttributeIdx() { return 3 + GrDrawState::kMaxTexCoords; }

    static int ViewMatrixAttributeIdx() {
        return 4 + GrDrawState::kMaxTexCoords;
    }
    static int TextureMatrixAttributeIdx(int stage) {
        return 7 + GrDrawState::kMaxTexCoords + 3 * stage;
    }

    
    
    
    struct Desc {
        Desc() {
            
            
            memset(this, 0, sizeof(Desc));
        }

        
        const uint32_t* asKey() const {
            return reinterpret_cast<const uint32_t*>(this);
        }

        struct StageDesc {
            enum OptFlagBits {
                kNoPerspective_OptFlagBit       = 1 << 0,
                kIdentityMatrix_OptFlagBit      = 1 << 1,
                kIsEnabled_OptFlagBit           = 1 << 7
            };

            uint8_t fOptFlags;

            

            GrProgramStageFactory::StageKey fCustomStageKey;

            inline bool isEnabled() const {
                return SkToBool(fOptFlags & kIsEnabled_OptFlagBit);
            }
            inline void setEnabled(bool newValue) {
                if (newValue) {
                    fOptFlags |= kIsEnabled_OptFlagBit;
                } else {
                    fOptFlags &= ~kIsEnabled_OptFlagBit;
                }
            }
        };

        
        
        enum ColorInput {
            kSolidWhite_ColorInput,
            kTransBlack_ColorInput,
            kAttribute_ColorInput,
            kUniform_ColorInput,

            kColorInputCnt
        };
        
        
        
        enum DualSrcOutput {
            kNone_DualSrcOutput,
            kCoverage_DualSrcOutput,
            kCoverageISA_DualSrcOutput,
            kCoverageISC_DualSrcOutput,

            kDualSrcOutputCnt
        };

        GrDrawState::VertexEdgeType fVertexEdgeType;

        
        GrVertexLayout fVertexLayout;

        StageDesc fStages[GrDrawState::kNumStages];

        
        
#if GR_GL_EXPERIMENTAL_GS
        bool fExperimentalGS;
#endif

        uint8_t fColorInput;        
        uint8_t fCoverageInput;     
        uint8_t fDualSrcOutput;     
        int8_t fFirstCoverageStage;
        SkBool8 fEmitsPointSize;
        SkBool8 fColorMatrixEnabled;

        uint8_t fColorFilterXfermode;  
    };
    GR_STATIC_ASSERT(!(sizeof(Desc) % 4));

    
    typedef Desc::StageDesc StageDesc;

private:
    struct StageUniforms;

    GrGLProgram(const GrGLContextInfo& gl,
                const Desc& desc,
                const GrCustomStage** customStages);

    bool succeeded() const { return 0 != fProgramID; }

    


    bool genProgram(const GrCustomStage** customStages);

    void genInputColor(GrGLShaderBuilder* builder, SkString* inColor);

    static GrGLProgramStage* GenStageCode(const GrCustomStage* stage,
                                          const StageDesc& desc, 
                                          StageUniforms* stageUniforms, 
                                          const char* fsInColor, 
                                          const char* fsOutColor,
                                          const char* vsInCoord,
                                          GrGLShaderBuilder* builder);

    void genGeometryShader(GrGLShaderBuilder* segments) const;

    typedef GrGLUniformManager::UniformHandle UniformHandle;

    void genUniformCoverage(GrGLShaderBuilder* segments, SkString* inOutCoverage);

    
    
    
    bool genEdgeCoverage(SkString* coverageVar, GrGLShaderBuilder* builder) const;

    
    bool bindOutputsAttribsAndLinkProgram(SkString texCoordAttrNames[GrDrawState::kMaxTexCoords],
                                          bool bindColorOut,
                                          bool bindDualSrcOut);

    
    void initSamplerUniforms();

    bool compileShaders(const GrGLShaderBuilder& builder);

    const char* adjustInColor(const SkString& inColor) const;

    struct StageUniforms {
        UniformHandle fTextureMatrixUni;
        SkTArray<UniformHandle, true> fSamplerUniforms;
        StageUniforms() {
            fTextureMatrixUni = GrGLUniformManager::kInvalidUniformHandle;
        }
    };

    struct Uniforms {
        UniformHandle fViewMatrixUni;
        UniformHandle fColorUni;
        UniformHandle fCoverageUni;
        UniformHandle fColorFilterUni;
        UniformHandle fColorMatrixUni;
        UniformHandle fColorMatrixVecUni;
        StageUniforms fStages[GrDrawState::kNumStages];
        Uniforms() {
            fViewMatrixUni = GrGLUniformManager::kInvalidUniformHandle;
            fColorUni = GrGLUniformManager::kInvalidUniformHandle;
            fCoverageUni = GrGLUniformManager::kInvalidUniformHandle;
            fColorFilterUni = GrGLUniformManager::kInvalidUniformHandle;
            fColorMatrixUni = GrGLUniformManager::kInvalidUniformHandle;
            fColorMatrixVecUni = GrGLUniformManager::kInvalidUniformHandle;
        }
    };

    
    GrGLuint    fVShaderID;
    GrGLuint    fGShaderID;
    GrGLuint    fFShaderID;
    GrGLuint    fProgramID;

    
    
    GrMatrix  fViewMatrix;
    SkISize   fViewportSize;

    
    
    GrColor                     fColor;
    GrColor                     fCoverage;
    GrColor                     fColorFilterColor;
    
    
    GrMatrix                    fTextureMatrices[GrDrawState::kNumStages];
    GrGLTexture::Orientation    fTextureOrientation[GrDrawState::kNumStages];

    GrGLProgramStage*           fProgramStage[GrDrawState::kNumStages];

    Desc fDesc;
    const GrGLContextInfo&      fContextInfo;

    GrGLUniformManager          fUniformManager;
    Uniforms                    fUniforms;

    friend class GrGpuGL; 

    typedef GrRefCnt INHERITED;
};

#endif
