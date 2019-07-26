






#ifndef GrGLShaderBuilder_DEFINED
#define GrGLShaderBuilder_DEFINED

#include "GrAllocator.h"
#include "GrBackendEffectFactory.h"
#include "GrColor.h"
#include "GrEffect.h"
#include "gl/GrGLSL.h"
#include "gl/GrGLUniformManager.h"

#include <stdarg.h>

class GrGLContextInfo;
class GrEffectStage;
class GrGLProgramDesc;





class GrGLShaderBuilder {
public:
    


    class TextureSampler {
    public:
        TextureSampler()
            : fConfigComponentMask(0)
            , fSamplerUniform(GrGLUniformManager::kInvalidUniformHandle) {
            
            
            fSwizzle[4] = '\0';
        }

        TextureSampler(const TextureSampler& other) { *this = other; }

        TextureSampler& operator= (const TextureSampler& other) {
            GrAssert(0 == fConfigComponentMask);
            GrAssert(GrGLUniformManager::kInvalidUniformHandle == fSamplerUniform);

            fConfigComponentMask = other.fConfigComponentMask;
            fSamplerUniform = other.fSamplerUniform;
            return *this;
        }

        
        uint32_t configComponentMask() const { return fConfigComponentMask; }

        const char* swizzle() const { return fSwizzle; }

        bool isInitialized() const { return 0 != fConfigComponentMask; }

    private:
        
        
        void init(GrGLShaderBuilder* builder,
                  uint32_t configComponentMask,
                  const char* swizzle,
                  int idx) {
            GrAssert(!this->isInitialized());
            GrAssert(0 != configComponentMask);
            GrAssert(GrGLUniformManager::kInvalidUniformHandle == fSamplerUniform);

            GrAssert(NULL != builder);
            SkString name;
            name.printf("Sampler%d_", idx);
            fSamplerUniform = builder->addUniform(GrGLShaderBuilder::kFragment_ShaderType,
                                                  kSampler2D_GrSLType,
                                                  name.c_str());
            GrAssert(GrGLUniformManager::kInvalidUniformHandle != fSamplerUniform);

            fConfigComponentMask = configComponentMask;
            memcpy(fSwizzle, swizzle, 4);
        }

        void init(GrGLShaderBuilder* builder, const GrTextureAccess* access, int idx) {
            GrAssert(NULL != access);
            this->init(builder,
                       GrPixelConfigComponentMask(access->getTexture()->config()),
                       access->getSwizzle(),
                       idx);
        }

        uint32_t                          fConfigComponentMask;
        char                              fSwizzle[5];
        GrGLUniformManager::UniformHandle fSamplerUniform;

        friend class GrGLShaderBuilder; 
    };

    typedef SkTArray<TextureSampler> TextureSamplerArray;

    enum ShaderType {
        kVertex_ShaderType   = 0x1,
        kGeometry_ShaderType = 0x2,
        kFragment_ShaderType = 0x4,
    };

    GrGLShaderBuilder(const GrGLContextInfo&, GrGLUniformManager&, const GrGLProgramDesc&);

    



    enum GLSLFeature {
        kStandardDerivatives_GLSLFeature = 0,

        kLastGLSLFeature = kStandardDerivatives_GLSLFeature
    };

    



    bool enableFeature(GLSLFeature);

    


    void vsCodeAppendf(const char format[], ...) SK_PRINTF_LIKE(2, 3) {
        va_list args;
        va_start(args, format);
        this->codeAppendf(kVertex_ShaderType, format, args);
        va_end(args);
    }

    void gsCodeAppendf(const char format[], ...) SK_PRINTF_LIKE(2, 3) {
        va_list args;
        va_start(args, format);
        this->codeAppendf(kGeometry_ShaderType, format, args);
        va_end(args);
    }

    void fsCodeAppendf(const char format[], ...) SK_PRINTF_LIKE(2, 3) {
        va_list args;
        va_start(args, format);
        this->codeAppendf(kFragment_ShaderType, format, args);
        va_end(args);
    }

    void vsCodeAppend(const char* str) { this->codeAppend(kVertex_ShaderType, str); }
    void gsCodeAppend(const char* str) { this->codeAppend(kGeometry_ShaderType, str); }
    void fsCodeAppend(const char* str) { this->codeAppend(kFragment_ShaderType, str); }

    


    void appendTextureLookup(SkString* out,
                             const TextureSampler&,
                             const char* coordName,
                             GrSLType coordType = kVec2f_GrSLType) const;

    

    void appendTextureLookup(ShaderType,
                             const TextureSampler&,
                             const char* coordName,
                             GrSLType coordType = kVec2f_GrSLType);


    



    void appendTextureLookupAndModulate(ShaderType,
                                        const char* modulation,
                                        const TextureSampler&,
                                        const char* coordName,
                                        GrSLType coordType = kVec2f_GrSLType);

    

    void emitFunction(ShaderType shader,
                      GrSLType returnType,
                      const char* name,
                      int argCnt,
                      const GrGLShaderVar* args,
                      const char* body,
                      SkString* outName);

    


    static GrBackendEffectFactory::EffectKey KeyForTextureAccess(const GrTextureAccess&,
                                                                 const GrGLCaps&);

    typedef uint8_t DstReadKey;

    


    static DstReadKey KeyForDstRead(const GrTexture* dstCopy, const GrGLCaps&);

    

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

    const GrGLShaderVar& getUniformVariable(GrGLUniformManager::UniformHandle) const;

    


    const char* getUniformCStr(GrGLUniformManager::UniformHandle u) const {
        return this->getUniformVariable(u).c_str();
    }

   

    bool addAttribute(GrSLType type, const char* name);

   


    void addVarying(GrSLType type,
                    const char* name,
                    const char** vsOutName = NULL,
                    const char** fsInName = NULL);

    

    const char* fragmentPosition();

    


    const GrGLShaderVar& positionAttribute() const { return *fPositionVar; }

    


    const GrGLShaderVar& localCoordsAttribute() const { return *fLocalCoordsVar; }

    

    const char* dstColor() const;

    


    bool hasExplicitLocalCoords() const { return (fLocalCoordsVar != fPositionVar); }

    





    
    void getShader(ShaderType, SkString*) const;

    void setCurrentStage(int stageIdx) { fCurrentStageIdx = stageIdx; }
    void setNonStage() { fCurrentStageIdx = kNonStageIdx; }
    
    
    GrGLEffect* createAndEmitGLEffect(
                                const GrEffectStage& stage,
                                GrBackendEffectFactory::EffectKey key,
                                const char* fsInColor, 
                                const char* fsOutColor,
                                SkTArray<GrGLUniformManager::UniformHandle, true>* samplerHandles);

    GrGLUniformManager::UniformHandle getRTHeightUniform() const { return fRTHeightUniform; }
    GrGLUniformManager::UniformHandle getDstCopyTopLeftUniform() const {
        return fDstCopyTopLeftUniform;
    }
    GrGLUniformManager::UniformHandle getDstCopyScaleUniform() const {
        return fDstCopyScaleUniform;
    }
    GrGLUniformManager::UniformHandle getDstCopySamplerUniform() const {
        return fDstCopySampler.fSamplerUniform;
    }

    struct AttributePair {
        void set(int index, const SkString& name) {
            fIndex = index; fName = name;
        }
        int      fIndex;
        SkString fName;
    };
    const SkTArray<AttributePair, true>& getEffectAttributes() const {
        return fEffectAttributes;
    }
    const SkString* getEffectAttributeName(int attributeIndex) const;

    
    void finished(GrGLuint programID);

    const GrGLContextInfo& ctxInfo() const { return fCtxInfo; }

private:
    void codeAppendf(ShaderType type, const char format[], va_list args);
    void codeAppend(ShaderType type, const char* str);

    typedef GrTAllocator<GrGLShaderVar> VarArray;

    void appendDecls(const VarArray&, SkString*) const;
    void appendUniformDecls(ShaderType, SkString*) const;

    typedef GrGLUniformManager::BuilderUniform BuilderUniform;
    GrGLUniformManager::BuilderUniformArray fUniforms;

    
public:

    VarArray    fVSAttrs;
    VarArray    fVSOutputs;
    VarArray    fGSInputs;
    VarArray    fGSOutputs;
    VarArray    fFSInputs;
    SkString    fGSHeader; 
    VarArray    fFSOutputs;

private:
    enum {
        kNonStageIdx = -1,
    };

    


    enum GLSLPrivateFeature {
        kFragCoordConventions_GLSLPrivateFeature = kLastGLSLFeature + 1
    };
    bool enablePrivateFeature(GLSLPrivateFeature);

    
    
    void addFSFeature(uint32_t featureBit, const char* extensionName);

    
    enum {
        kNoDstRead_DstReadKey         = 0,
        kYesDstRead_DstReadKeyBit     = 0x1, 
        kUseAlphaConfig_DstReadKeyBit = 0x2, 
        kTopLeftOrigin_DstReadKeyBit  = 0x4, 
    };

    const GrGLContextInfo&              fCtxInfo;
    GrGLUniformManager&                 fUniformManager;
    int                                 fCurrentStageIdx;
    uint32_t                            fFSFeaturesAddedMask;
    SkString                            fFSFunctions;
    SkString                            fFSExtensions;

    bool                                fUsesGS;

    SkString                            fFSCode;
    SkString                            fVSCode;
    SkString                            fGSCode;

    bool                                fSetupFragPosition;
    TextureSampler                      fDstCopySampler;

    GrGLUniformManager::UniformHandle   fRTHeightUniform;
    GrGLUniformManager::UniformHandle   fDstCopyTopLeftUniform;
    GrGLUniformManager::UniformHandle   fDstCopyScaleUniform;

    SkSTArray<10, AttributePair, true>  fEffectAttributes;

    GrGLShaderVar*                      fPositionVar;
    GrGLShaderVar*                      fLocalCoordsVar;

};

#endif
