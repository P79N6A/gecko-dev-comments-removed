








#ifndef GrGLProgram_DEFINED
#define GrGLProgram_DEFINED

#include "GrDrawState.h"
#include "GrGLInterface.h"
#include "GrGLSL.h"
#include "GrStringBuilder.h"
#include "GrGpu.h"

#include "SkXfermode.h"

class GrBinHashKeyBuilder;

struct ShaderCodeSegments;



#define GR_GL_EXPERIMENTAL_GS GR_DEBUG










class GrGLProgram {
public:

    class CachedData;

    GrGLProgram();
    ~GrGLProgram();

    




    bool genProgram(const GrGLInterface* gl,
                    GrGLSLGeneration glslVersion,
                    CachedData* programData) const;

     


     void overrideBlend(GrBlendCoeff* srcCoeff, GrBlendCoeff* dstCoeff) const;

    


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

public:

    
    
    
    struct ProgramDesc {
        ProgramDesc() {
            
            
            memset(this, 0, sizeof(ProgramDesc));
        }

        enum OutputPM {
            
            kYes_OutputPM,
            
            kNo_OutputPM,

            kOutputPMCnt
        };

        struct StageDesc {
            enum OptFlagBits {
                kNoPerspective_OptFlagBit       = 1 << 0,
                kIdentityMatrix_OptFlagBit      = 1 << 1,
                kCustomTextureDomain_OptFlagBit = 1 << 2,
                kIsEnabled_OptFlagBit           = 1 << 7
            };
            enum FetchMode {
                kSingle_FetchMode,
                k2x2_FetchMode,
                kConvolution_FetchMode,

                kFetchModeCnt,
            };
            



            enum InConfigFlags {
                kNone_InConfigFlag              = 0x0,

                





                kSwapRAndB_InConfigFlag         = 0x1,

                





                kSmearAlpha_InConfigFlag        = 0x2,

                



                kMulRGBByAlpha_InConfigFlag     =  0x4,

                kDummyInConfigFlag,
                kInConfigBitMask = (kDummyInConfigFlag-1) |
                                   (kDummyInConfigFlag-2)
            };
            enum CoordMapping {
                kIdentity_CoordMapping,
                kRadialGradient_CoordMapping,
                kSweepGradient_CoordMapping,
                kRadial2Gradient_CoordMapping,
                
                
                kRadial2GradientDegenerate_CoordMapping,
                kCoordMappingCnt
            };

            uint8_t fOptFlags;
            uint8_t fInConfigFlags; 
            uint8_t fFetchMode;     
            uint8_t fCoordMapping;  
            uint8_t fKernelWidth;

            GR_STATIC_ASSERT((InConfigFlags)(uint8_t)kInConfigBitMask ==
                             kInConfigBitMask);

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
        uint8_t fOutputPM;          
        uint8_t fDualSrcOutput;     
        int8_t fFirstCoverageStage;
        SkBool8 fEmitsPointSize;
        SkBool8 fEdgeAAConcave;
        SkBool8 fColorMatrixEnabled;

        int8_t fEdgeAANumEdges;
        uint8_t fColorFilterXfermode;  
        int8_t fPadding[3];

    } fProgramDesc;
    GR_STATIC_ASSERT(!(sizeof(ProgramDesc) % 4));

    
    typedef ProgramDesc::StageDesc StageDesc;

private:

    const ProgramDesc& getDesc() { return fProgramDesc; }
    const char* adjustInColor(const GrStringBuilder& inColor) const;

public:
    enum {
        kUnusedUniform = -1,
        kSetAsAttribute = 1000,
    };

    struct StageUniLocations {
        GrGLint fTextureMatrixUni;
        GrGLint fNormalizedTexelSizeUni;
        GrGLint fSamplerUni;
        GrGLint fRadial2Uni;
        GrGLint fTexDomUni;
        GrGLint fKernelUni;
        GrGLint fImageIncrementUni;
        void reset() {
            fTextureMatrixUni = kUnusedUniform;
            fNormalizedTexelSizeUni = kUnusedUniform;
            fSamplerUni = kUnusedUniform;
            fRadial2Uni = kUnusedUniform;
            fTexDomUni = kUnusedUniform;
            fKernelUni = kUnusedUniform;
            fImageIncrementUni = kUnusedUniform;
        }
    };

    struct UniLocations {
        GrGLint fViewMatrixUni;
        GrGLint fColorUni;
        GrGLint fEdgesUni;
        GrGLint fColorFilterUni;
        GrGLint fColorMatrixUni;
        GrGLint fColorMatrixVecUni;
        StageUniLocations fStages[GrDrawState::kNumStages];
        void reset() {
            fViewMatrixUni = kUnusedUniform;
            fColorUni = kUnusedUniform;
            fEdgesUni = kUnusedUniform;
            fColorFilterUni = kUnusedUniform;
            fColorMatrixUni = kUnusedUniform;
            fColorMatrixVecUni = kUnusedUniform;
            for (int s = 0; s < GrDrawState::kNumStages; ++s) {
                fStages[s].reset();
            }
        }
    };

    class CachedData : public ::GrNoncopyable {
    public:
        CachedData() {
        }

        ~CachedData() {
        }

        void copyAndTakeOwnership(CachedData& other) {
            memcpy(this, &other, sizeof(*this));
        }

    public:

        
        GrGLuint    fVShaderID;
        GrGLuint    fGShaderID;
        GrGLuint    fFShaderID;
        GrGLuint    fProgramID;
        
        UniLocations fUniLocations;

        GrMatrix  fViewMatrix;

        
        
        GrColor                     fColor;
        GrColor                     fColorFilterColor;
        GrMatrix                    fTextureMatrices[GrDrawState::kNumStages];
        
        int                         fTextureWidth[GrDrawState::kNumStages];
        int                         fTextureHeight[GrDrawState::kNumStages]; 
        GrScalar                    fRadial2CenterX1[GrDrawState::kNumStages];
        GrScalar                    fRadial2Radius0[GrDrawState::kNumStages];
        bool                        fRadial2PosRoot[GrDrawState::kNumStages];
        GrRect                      fTextureDomain[GrDrawState::kNumStages];

    private:
        enum Constants {
            kUniLocationPreAllocSize = 8
        };

    }; 

    enum Constants {
        kProgramKeySize = sizeof(ProgramDesc)
    };

    
    const uint32_t* keyData() const{
        return reinterpret_cast<const uint32_t*>(&fProgramDesc);
    }

private:

    
    void genStageCode(const GrGLInterface* gl,
                      int stageNum,
                      const ProgramDesc::StageDesc& desc,
                      const char* fsInColor, 
                      const char* fsOutColor,
                      const char* vsInCoord,
                      ShaderCodeSegments* segments,
                      StageUniLocations* locations) const;

    void genGeometryShader(const GrGLInterface* gl,
                           GrGLSLGeneration glslVersion,
                           ShaderCodeSegments* segments) const;

    
    void genEdgeCoverage(const GrGLInterface* gl,
                         GrVertexLayout layout,
                         CachedData* programData,
                         GrStringBuilder* coverageVar,
                         ShaderCodeSegments* segments) const;

    static bool CompileShaders(const GrGLInterface* gl,
                               GrGLSLGeneration glslVersion,
                               const ShaderCodeSegments& segments, 
                               CachedData* programData);

    
    
    static GrGLuint CompileShader(const GrGLInterface* gl,
                                  GrGLenum type, int stringCnt,
                                  const char** strings,
                                  int* stringLengths);

    
    
    bool bindOutputsAttribsAndLinkProgram(
                const GrGLInterface* gl,
                GrStringBuilder texCoordAttrNames[GrDrawState::kMaxTexCoords],
                bool bindColorOut,
                bool bindDualSrcOut,
                CachedData* programData) const;

    
    void getUniformLocationsAndInitCache(const GrGLInterface* gl,
                                         CachedData* programData) const;

    friend class GrGpuGLShaders;
};

#endif
