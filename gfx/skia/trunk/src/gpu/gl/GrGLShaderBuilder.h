






#ifndef GrGLShaderBuilder_DEFINED
#define GrGLShaderBuilder_DEFINED

#include "GrAllocator.h"
#include "GrBackendEffectFactory.h"
#include "GrColor.h"
#include "GrEffect.h"
#include "SkTypes.h"
#include "gl/GrGLProgramDesc.h"
#include "gl/GrGLProgramEffects.h"
#include "gl/GrGLSL.h"
#include "gl/GrGLUniformManager.h"

#include <stdarg.h>

class GrGLContextInfo;
class GrEffectStage;
class GrGLProgramDesc;





class GrGLShaderBuilder {
public:
    typedef GrTAllocator<GrGLShaderVar> VarArray;
    typedef GrGLProgramEffects::TextureSampler TextureSampler;
    typedef GrGLProgramEffects::TransformedCoordsArray TransformedCoordsArray;
    typedef GrGLUniformManager::BuilderUniform BuilderUniform;

    enum ShaderVisibility {
        kVertex_Visibility   = 0x1,
        kGeometry_Visibility = 0x2,
        kFragment_Visibility = 0x4,
    };

    typedef GrGLUniformManager::UniformHandle UniformHandle;

    
    struct UniformHandles {
        UniformHandle       fViewMatrixUni;
        UniformHandle       fRTAdjustmentUni;
        UniformHandle       fColorUni;
        UniformHandle       fCoverageUni;

        
        
        UniformHandle       fRTHeightUni;

        
        UniformHandle       fDstCopyTopLeftUni;
        UniformHandle       fDstCopyScaleUni;
        UniformHandle       fDstCopySamplerUni;
    };

    struct GenProgramOutput {
        GenProgramOutput()
            : fColorEffects(NULL)
            , fCoverageEffects(NULL)
            , fHasVertexShader(false)
            , fTexCoordSetCnt(0)
            , fProgramID(0) {}

        GenProgramOutput(const GenProgramOutput& other) {
            *this = other;
        }

        GenProgramOutput& operator=(const GenProgramOutput& other) {
            fColorEffects.reset(SkRef(other.fColorEffects.get()));
            fCoverageEffects.reset(SkRef(other.fCoverageEffects.get()));
            fUniformHandles = other.fUniformHandles;
            fHasVertexShader = other.fHasVertexShader;
            fTexCoordSetCnt = other.fTexCoordSetCnt;
            fProgramID = other.fProgramID;
            return *this;
        }

        SkAutoTUnref<GrGLProgramEffects> fColorEffects;
        SkAutoTUnref<GrGLProgramEffects> fCoverageEffects;
        UniformHandles                   fUniformHandles;
        bool                             fHasVertexShader;
        int                              fTexCoordSetCnt;
        GrGLuint                         fProgramID;
    };

    static bool GenProgram(GrGpuGL* gpu,
                           GrGLUniformManager* uman,
                           const GrGLProgramDesc& desc,
                           const GrEffectStage* inColorStages[],
                           const GrEffectStage* inCoverageStages[],
                           GenProgramOutput* output);

    virtual ~GrGLShaderBuilder() {}

    



    enum GLSLFeature {
        kStandardDerivatives_GLSLFeature = 0,

        kLastGLSLFeature = kStandardDerivatives_GLSLFeature
    };

    



    bool enableFeature(GLSLFeature);

    


    void fsCodeAppendf(const char format[], ...) SK_PRINTF_LIKE(2, 3) {
        va_list args;
        va_start(args, format);
        fFSCode.appendVAList(format, args);
        va_end(args);
    }

    void fsCodeAppend(const char* str) { fFSCode.append(str); }

    


    void appendTextureLookup(SkString* out,
                             const TextureSampler&,
                             const char* coordName,
                             GrSLType coordType = kVec2f_GrSLType) const;

    
    void fsAppendTextureLookup(const TextureSampler&,
                               const char* coordName,
                               GrSLType coordType = kVec2f_GrSLType);


    



    void fsAppendTextureLookupAndModulate(const char* modulation,
                                          const TextureSampler&,
                                          const char* coordName,
                                          GrSLType coordType = kVec2f_GrSLType);

    
    void fsEmitFunction(GrSLType returnType,
                        const char* name,
                        int argCnt,
                        const GrGLShaderVar* args,
                        const char* body,
                        SkString* outName);

    typedef uint8_t DstReadKey;
    typedef uint8_t FragPosKey;

    


    static DstReadKey KeyForDstRead(const GrTexture* dstCopy, const GrGLCaps&);

    


    static FragPosKey KeyForFragmentPosition(const GrRenderTarget* dst, const GrGLCaps&);

    

    static const GrGLenum* GetTexParamSwizzle(GrPixelConfig config, const GrGLCaps& caps);

    





    GrGLUniformManager::UniformHandle addUniform(uint32_t visibility,
                                                 GrSLType type,
                                                 const char* name,
                                                 const char** outName = NULL) {
        return this->addUniformArray(visibility, type, name, GrGLShaderVar::kNonArray, outName);
    }
    GrGLUniformManager::UniformHandle addUniformArray(uint32_t visibility,
                                                      GrSLType type,
                                                      const char* name,
                                                      int arrayCount,
                                                      const char** outName = NULL);

    const GrGLShaderVar& getUniformVariable(GrGLUniformManager::UniformHandle u) const {
        return fUniformManager->getBuilderUniform(fUniforms, u).fVariable;
    }

    


    const char* getUniformCStr(GrGLUniformManager::UniformHandle u) const {
        return this->getUniformVariable(u).c_str();
    }

    




    SkString ensureFSCoords2D(const TransformedCoordsArray&, int index);

    

    const char* fragmentPosition();

    

    const char* dstColor();

    const GrGLContextInfo& ctxInfo() const;

    




    class FSBlock {
    public:
        FSBlock(GrGLShaderBuilder* builder) : fBuilder(builder) {
            SkASSERT(NULL != builder);
            fBuilder->fsCodeAppend("\t{\n");
        }

        ~FSBlock() {
            fBuilder->fsCodeAppend("\t}\n");
        }
    private:
        GrGLShaderBuilder* fBuilder;
    };

protected:
    GrGLShaderBuilder(GrGpuGL*, GrGLUniformManager*, const GrGLProgramDesc&);

    GrGpuGL* gpu() const { return fGpu; }

    const GrGLProgramDesc& desc() const { return fDesc; }

    
    GrGLShaderVar& fsInputAppend() { return fFSInputs.push_back(); }

    
    void createAndEmitEffects(GrGLProgramEffectsBuilder*,
                              const GrEffectStage* effectStages[],
                              int effectCnt,
                              const GrGLProgramDesc::EffectKeyProvider&,
                              GrGLSLExpr4* inOutFSColor);

    
    
    
    void nameVariable(SkString* out, char prefix, const char* name);

    virtual bool compileAndAttachShaders(GrGLuint programId, SkTDArray<GrGLuint>* shaderIds) const;

    virtual void bindProgramLocations(GrGLuint programId) const;

    void appendDecls(const VarArray&, SkString*) const;
    void appendUniformDecls(ShaderVisibility, SkString*) const;

    const GenProgramOutput& getOutput() const { return fOutput; }

    GenProgramOutput fOutput;

private:
    class CodeStage : SkNoncopyable {
    public:
        CodeStage() : fNextIndex(0), fCurrentIndex(-1), fEffectStage(NULL) {}

        bool inStageCode() const {
            this->validate();
            return NULL != fEffectStage;
        }

        const GrEffectStage* effectStage() const {
            this->validate();
            return fEffectStage;
        }

        int stageIndex() const {
            this->validate();
            return fCurrentIndex;
        }

        class AutoStageRestore : SkNoncopyable {
        public:
            AutoStageRestore(CodeStage* codeStage, const GrEffectStage* newStage) {
                SkASSERT(NULL != codeStage);
                fSavedIndex = codeStage->fCurrentIndex;
                fSavedEffectStage = codeStage->fEffectStage;

                if (NULL == newStage) {
                    codeStage->fCurrentIndex = -1;
                } else {
                    codeStage->fCurrentIndex = codeStage->fNextIndex++;
                }
                codeStage->fEffectStage = newStage;

                fCodeStage = codeStage;
            }
            ~AutoStageRestore() {
                fCodeStage->fCurrentIndex = fSavedIndex;
                fCodeStage->fEffectStage = fSavedEffectStage;
            }
        private:
            CodeStage*              fCodeStage;
            int                     fSavedIndex;
            const GrEffectStage*    fSavedEffectStage;
        };
    private:
        void validate() const { SkASSERT((NULL == fEffectStage) == (-1 == fCurrentIndex)); }
        int                     fNextIndex;
        int                     fCurrentIndex;
        const GrEffectStage*    fEffectStage;
    } fCodeStage;

    bool genProgram(const GrEffectStage* colorStages[], const GrEffectStage* coverageStages[]);

    






    virtual void emitCodeBeforeEffects(GrGLSLExpr4* color, GrGLSLExpr4* coverage) = 0;

    







    virtual GrGLProgramEffects* createAndEmitEffects(const GrEffectStage* effectStages[],
                                                     int effectCnt,
                                                     const GrGLProgramDesc::EffectKeyProvider&,
                                                     GrGLSLExpr4* inOutFSColor) = 0;

    


    virtual void emitCodeAfterEffects() = 0;

    

    const char* enableSecondaryOutput();
    
    const char* getColorOutputName() const;

    



    bool finish();

    


    enum GLSLPrivateFeature {
        kFragCoordConventions_GLSLPrivateFeature = kLastGLSLFeature + 1,
        kEXTShaderFramebufferFetch_GLSLPrivateFeature,
        kNVShaderFramebufferFetch_GLSLPrivateFeature,
    };
    bool enablePrivateFeature(GLSLPrivateFeature);

    
    
    void addFSFeature(uint32_t featureBit, const char* extensionName);

    
    enum {
        kNoDstRead_DstReadKey           = 0,
        kYesDstRead_DstReadKeyBit       = 0x1, 
        kUseAlphaConfig_DstReadKeyBit   = 0x2, 
        kTopLeftOrigin_DstReadKeyBit    = 0x4, 
    };

    enum {
        kNoFragPosRead_FragPosKey           = 0,  
        kTopLeftFragPosRead_FragPosKey      = 0x1,
        kBottomLeftFragPosRead_FragPosKey   = 0x2,
    };

    const GrGLProgramDesc&                  fDesc;
    GrGpuGL*                                fGpu;
    SkAutoTUnref<GrGLUniformManager>        fUniformManager;
    uint32_t                                fFSFeaturesAddedMask;
    SkString                                fFSFunctions;
    SkString                                fFSExtensions;
    VarArray                                fFSInputs;
    VarArray                                fFSOutputs;
    GrGLUniformManager::BuilderUniformArray fUniforms;

    SkString                                fFSCode;

    bool                                    fSetupFragPosition;
    bool                                    fTopLeftFragPosRead;

    bool                                    fHasCustomColorOutput;
    bool                                    fHasSecondaryOutput;
};



class GrGLFullShaderBuilder : public GrGLShaderBuilder {
public:
    GrGLFullShaderBuilder(GrGpuGL*, GrGLUniformManager*, const GrGLProgramDesc&);

    


    void vsCodeAppendf(const char format[], ...) SK_PRINTF_LIKE(2, 3) {
        va_list args;
        va_start(args, format);
        fVSCode.appendVAList(format, args);
        va_end(args);
    }

    void vsCodeAppend(const char* str) { fVSCode.append(str); }

   

    bool addAttribute(GrSLType type, const char* name);

   


    void addVarying(GrSLType type,
                    const char* name,
                    const char** vsOutName = NULL,
                    const char** fsInName = NULL);

    


    const GrGLShaderVar& positionAttribute() const { return *fPositionVar; }

    


    const GrGLShaderVar& localCoordsAttribute() const { return *fLocalCoordsVar; }

    


    bool hasExplicitLocalCoords() const { return (fLocalCoordsVar != fPositionVar); }

    bool addEffectAttribute(int attributeIndex, GrSLType type, const SkString& name);
    const SkString* getEffectAttributeName(int attributeIndex) const;

private:
    virtual void emitCodeBeforeEffects(GrGLSLExpr4* color, GrGLSLExpr4* coverage) SK_OVERRIDE;

    virtual GrGLProgramEffects* createAndEmitEffects(const GrEffectStage* effectStages[],
                                                     int effectCnt,
                                                     const GrGLProgramDesc::EffectKeyProvider&,
                                                     GrGLSLExpr4* inOutFSColor) SK_OVERRIDE;

    virtual void emitCodeAfterEffects() SK_OVERRIDE;

    virtual bool compileAndAttachShaders(GrGLuint programId,
                                         SkTDArray<GrGLuint>* shaderIds) const SK_OVERRIDE;

    virtual void bindProgramLocations(GrGLuint programId) const SK_OVERRIDE;

    VarArray                            fVSAttrs;
    VarArray                            fVSOutputs;
    VarArray                            fGSInputs;
    VarArray                            fGSOutputs;

    SkString                            fVSCode;

    struct AttributePair {
        void set(int index, const SkString& name) {
            fIndex = index; fName = name;
        }
        int      fIndex;
        SkString fName;
    };
    SkSTArray<10, AttributePair, true>  fEffectAttributes;

    GrGLShaderVar*                      fPositionVar;
    GrGLShaderVar*                      fLocalCoordsVar;

    typedef GrGLShaderBuilder INHERITED;
};



class GrGLFragmentOnlyShaderBuilder : public GrGLShaderBuilder {
public:
    GrGLFragmentOnlyShaderBuilder(GrGpuGL*, GrGLUniformManager*, const GrGLProgramDesc&);

    int addTexCoordSets(int count);

private:
    virtual void emitCodeBeforeEffects(GrGLSLExpr4* color, GrGLSLExpr4* coverage) SK_OVERRIDE {}

    virtual GrGLProgramEffects* createAndEmitEffects(const GrEffectStage* effectStages[],
                                                     int effectCnt,
                                                     const GrGLProgramDesc::EffectKeyProvider&,
                                                     GrGLSLExpr4* inOutFSColor) SK_OVERRIDE;

    virtual void emitCodeAfterEffects() SK_OVERRIDE {}

    typedef GrGLShaderBuilder INHERITED;
};

#endif
