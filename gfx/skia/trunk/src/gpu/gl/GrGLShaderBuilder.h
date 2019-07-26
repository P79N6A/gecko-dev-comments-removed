






#ifndef GrGLShaderBuilder_DEFINED
#define GrGLShaderBuilder_DEFINED

#include "GrAllocator.h"
#include "GrBackendEffectFactory.h"
#include "GrColor.h"
#include "GrEffect.h"
#include "SkTypes.h"
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
    typedef GrBackendEffectFactory::EffectKey EffectKey;
    typedef GrGLProgramEffects::TextureSampler TextureSampler;
    typedef GrGLProgramEffects::TransformedCoordsArray TransformedCoordsArray;
    typedef GrGLUniformManager::BuilderUniform BuilderUniform;

    enum ShaderVisibility {
        kVertex_Visibility   = 0x1,
        kGeometry_Visibility = 0x2,
        kFragment_Visibility = 0x4,
    };

    GrGLShaderBuilder(GrGpuGL*, GrGLUniformManager&, const GrGLProgramDesc&);
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
        return fUniformManager.getBuilderUniform(fUniforms, u).fVariable;
    }

    


    const char* getUniformCStr(GrGLUniformManager::UniformHandle u) const {
        return this->getUniformVariable(u).c_str();
    }

    




    SkString ensureFSCoords2D(const TransformedCoordsArray&, int index);

    

    const char* fragmentPosition();

    

    const char* dstColor();

    


    const GrGLSLExpr4& getInputColor() const {
        return fInputColor;
    }
    const GrGLSLExpr4& getInputCoverage() const {
        return fInputCoverage;
    }

    







    virtual GrGLProgramEffects* createAndEmitEffects(const GrEffectStage* effectStages[],
                                                     const EffectKey effectKeys[],
                                                     int effectCnt,
                                                     GrGLSLExpr4* inOutFSColor) = 0;

    const char* getColorOutputName() const;
    const char* enableSecondaryOutput();

    GrGLUniformManager::UniformHandle getRTHeightUniform() const { return fRTHeightUniform; }
    GrGLUniformManager::UniformHandle getDstCopyTopLeftUniform() const {
        return fDstCopyTopLeftUniform;
    }
    GrGLUniformManager::UniformHandle getDstCopyScaleUniform() const {
        return fDstCopyScaleUniform;
    }
    GrGLUniformManager::UniformHandle getColorUniform() const { return fColorUniform; }
    GrGLUniformManager::UniformHandle getCoverageUniform() const { return fCoverageUniform; }
    GrGLUniformManager::UniformHandle getDstCopySamplerUniform() const {
        return fDstCopySamplerUniform;
    }

    bool finish(GrGLuint* outProgramId);

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
    GrGpuGL* gpu() const { return fGpu; }

    void setInputColor(const GrGLSLExpr4& inputColor) { fInputColor = inputColor; }
    void setInputCoverage(const GrGLSLExpr4& inputCoverage) { fInputCoverage = inputCoverage; }

    
    GrGLShaderVar& fsInputAppend() { return fFSInputs.push_back(); }

    
    
    
    void nameVariable(SkString* out, char prefix, const char* name);

    
    void createAndEmitEffects(GrGLProgramEffectsBuilder*,
                              const GrEffectStage* effectStages[],
                              const EffectKey effectKeys[],
                              int effectCnt,
                              GrGLSLExpr4* inOutFSColor);

    virtual bool compileAndAttachShaders(GrGLuint programId) const;
    virtual void bindProgramLocations(GrGLuint programId) const;

    void appendDecls(const VarArray&, SkString*) const;
    void appendUniformDecls(ShaderVisibility, SkString*) const;

private:
    class CodeStage : public SkNoncopyable {
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

        class AutoStageRestore : public SkNoncopyable {
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

    


    enum GLSLPrivateFeature {
        kFragCoordConventions_GLSLPrivateFeature = kLastGLSLFeature + 1,
        kEXTShaderFramebufferFetch_GLSLPrivateFeature,
        kNVShaderFramebufferFetch_GLSLPrivateFeature,
    };
    bool enablePrivateFeature(GLSLPrivateFeature);

    
    
    void addFSFeature(uint32_t featureBit, const char* extensionName);

    
    enum {
        kNoDstRead_DstReadKey         = 0,
        kYesDstRead_DstReadKeyBit     = 0x1, 
        kUseAlphaConfig_DstReadKeyBit = 0x2, 
        kTopLeftOrigin_DstReadKeyBit  = 0x4, 
    };

    enum {
        kNoFragPosRead_FragPosKey           = 0,  
        kTopLeftFragPosRead_FragPosKey      = 0x1,
        kBottomLeftFragPosRead_FragPosKey   = 0x2,
    };

    GrGpuGL*                                fGpu;
    GrGLUniformManager&                     fUniformManager;
    uint32_t                                fFSFeaturesAddedMask;
    SkString                                fFSFunctions;
    SkString                                fFSExtensions;
    VarArray                                fFSInputs;
    VarArray                                fFSOutputs;
    GrGLUniformManager::BuilderUniformArray fUniforms;

    SkString                                fFSCode;

    bool                                    fSetupFragPosition;
    GrGLUniformManager::UniformHandle       fDstCopySamplerUniform;

    GrGLSLExpr4                             fInputColor;
    GrGLSLExpr4                             fInputCoverage;

    bool                                    fHasCustomColorOutput;
    bool                                    fHasSecondaryOutput;

    GrGLUniformManager::UniformHandle       fRTHeightUniform;
    GrGLUniformManager::UniformHandle       fDstCopyTopLeftUniform;
    GrGLUniformManager::UniformHandle       fDstCopyScaleUniform;
    GrGLUniformManager::UniformHandle       fColorUniform;
    GrGLUniformManager::UniformHandle       fCoverageUniform;

    bool                                    fTopLeftFragPosRead;
};



class GrGLFullShaderBuilder : public GrGLShaderBuilder {
public:
    GrGLFullShaderBuilder(GrGpuGL*, GrGLUniformManager&, const GrGLProgramDesc&);

    


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

    virtual GrGLProgramEffects* createAndEmitEffects(
                const GrEffectStage* effectStages[],
                const EffectKey effectKeys[],
                int effectCnt,
                GrGLSLExpr4* inOutFSColor) SK_OVERRIDE;

    GrGLUniformManager::UniformHandle getViewMatrixUniform() const {
        return fViewMatrixUniform;
    }

protected:
    virtual bool compileAndAttachShaders(GrGLuint programId) const SK_OVERRIDE;
    virtual void bindProgramLocations(GrGLuint programId) const SK_OVERRIDE;

private:
    const GrGLProgramDesc&              fDesc;
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

    GrGLUniformManager::UniformHandle   fViewMatrixUniform;

    GrGLShaderVar*                      fPositionVar;
    GrGLShaderVar*                      fLocalCoordsVar;

    typedef GrGLShaderBuilder INHERITED;
};



class GrGLFragmentOnlyShaderBuilder : public GrGLShaderBuilder {
public:
    GrGLFragmentOnlyShaderBuilder(GrGpuGL*, GrGLUniformManager&, const GrGLProgramDesc&);

    int getNumTexCoordSets() const { return fNumTexCoordSets; }
    int addTexCoordSets(int count);

    virtual GrGLProgramEffects* createAndEmitEffects(
                const GrEffectStage* effectStages[],
                const EffectKey effectKeys[],
                int effectCnt,
                GrGLSLExpr4* inOutFSColor) SK_OVERRIDE;

private:
    int fNumTexCoordSets;

    typedef GrGLShaderBuilder INHERITED;
};

#endif
