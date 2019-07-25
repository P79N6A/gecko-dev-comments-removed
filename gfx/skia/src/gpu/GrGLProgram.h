








#ifndef GrGLProgram_DEFINED
#define GrGLProgram_DEFINED

#include "GrDrawState.h"
#include "GrGLInterface.h"
#include "GrStringBuilder.h"
#include "GrGpu.h"

#include "SkXfermode.h"

class GrBinHashKeyBuilder;

struct ShaderCodeSegments;



#define GR_GL_EXPERIMENTAL_GS GR_DEBUG










class GrGLProgram {
public:
    
    
    enum GLSLVersion {
        


        k110_GLSLVersion,
        


        k130_GLSLVersion,
        


        k150_GLSLVersion,
    };

    class CachedData;

    GrGLProgram();
    ~GrGLProgram();

    




    bool genProgram(const GrGLInterface* gl,
                    GLSLVersion glslVersion,
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

        struct StageDesc {
            enum OptFlagBits {
                kNoPerspective_OptFlagBit       = 1 << 0,
                kIdentityMatrix_OptFlagBit      = 1 << 1,
                kCustomTextureDomain_OptFlagBit = 1 << 2,
                kIsEnabled_OptFlagBit           = 1 << 7
            };
            enum Modulation {
                kColor_Modulation,
                kAlpha_Modulation,

                kModulationCnt
            };
            enum FetchMode {
                kSingle_FetchMode,
                k2x2_FetchMode,
                kConvolution_FetchMode,

                kFetchModeCnt,
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
            uint8_t fModulation;  
            uint8_t fFetchMode;  
            uint8_t fCoordMapping;  
            uint8_t fKernelWidth;

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

        
        
        enum ColorType {
            kSolidWhite_ColorType,
            kTransBlack_ColorType,
            kAttribute_ColorType,
            kUniform_ColorType,

            kColorTypeCnt
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

        uint8_t fColorType;  
        uint8_t fDualSrcOutput;  
        int8_t fFirstCoverageStage;
        SkBool8 fEmitsPointSize;
        SkBool8 fEdgeAAConcave;

        int8_t fEdgeAANumEdges;
        uint8_t fColorFilterXfermode;  

        uint8_t fPadTo32bLengthMultiple [1];

    } fProgramDesc;
    GR_STATIC_ASSERT(!(sizeof(ProgramDesc) % 4));

    
    typedef ProgramDesc::StageDesc StageDesc;

private:

    const ProgramDesc& getDesc() { return fProgramDesc; }

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
        StageUniLocations fStages[GrDrawState::kNumStages];
        void reset() {
            fViewMatrixUni = kUnusedUniform;
            fColorUni = kUnusedUniform;
            fEdgesUni = kUnusedUniform;
            fColorFilterUni = kUnusedUniform;
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
                           GLSLVersion glslVersion,
                           ShaderCodeSegments* segments) const;

    
    void genEdgeCoverage(const GrGLInterface* gl,
                         GrVertexLayout layout,
                         CachedData* programData,
                         GrStringBuilder* coverageVar,
                         ShaderCodeSegments* segments) const;

    static bool CompileShaders(const GrGLInterface* gl,
                               GLSLVersion glslVersion,
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
