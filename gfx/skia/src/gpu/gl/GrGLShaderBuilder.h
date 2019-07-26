






#ifndef GrGLShaderBuilder_DEFINED
#define GrGLShaderBuilder_DEFINED

#include "GrAllocator.h"
#include "GrCustomStage.h"
#include "gl/GrGLShaderVar.h"
#include "gl/GrGLSL.h"
#include "gl/GrGLUniformManager.h"

class GrGLContextInfo;





class GrGLShaderBuilder {
public:
    


    class TextureSampler {
    public:
        TextureSampler()
            : fTextureAccess(NULL)
            , fSamplerUniform(GrGLUniformManager::kInvalidUniformHandle) {}

        TextureSampler(const TextureSampler& other) { *this = other; }

        TextureSampler& operator= (const TextureSampler& other) {
            GrAssert(NULL == fTextureAccess);
            GrAssert(GrGLUniformManager::kInvalidUniformHandle == fSamplerUniform);

            fTextureAccess = other.fTextureAccess;
            fSamplerUniform = other.fSamplerUniform;
            return *this;
        }

        const GrTextureAccess* textureAccess() const { return fTextureAccess; }

    private:
        void init(GrGLShaderBuilder* builder, const GrTextureAccess* access) {
            GrAssert(NULL == fTextureAccess);
            GrAssert(GrGLUniformManager::kInvalidUniformHandle == fSamplerUniform);

            GrAssert(NULL != builder);
            GrAssert(NULL != access);
            fSamplerUniform = builder->addUniform(GrGLShaderBuilder::kFragment_ShaderType,
                                                  kSampler2D_GrSLType,
                                                  "Sampler");
            GrAssert(GrGLUniformManager::kInvalidUniformHandle != fSamplerUniform);

            fTextureAccess = access;
        }

        const GrTextureAccess*            fTextureAccess;
        GrGLUniformManager::UniformHandle fSamplerUniform;

        friend class GrGLShaderBuilder; 
        friend class GrGLProgram;       
    };

    typedef SkTArray<TextureSampler> TextureSamplerArray;

    enum ShaderType {
        kVertex_ShaderType   = 0x1,
        kGeometry_ShaderType = 0x2,
        kFragment_ShaderType = 0x4,
    };

    GrGLShaderBuilder(const GrGLContextInfo&, GrGLUniformManager&);

    


    void setupTextureAccess(const char* varyingFSName, GrSLType varyingType);

    



    void appendTextureLookup(SkString* out,
                             const TextureSampler&,
                             const char* coordName = NULL,
                             GrSLType coordType = kVec2f_GrSLType) const;

    



    void appendTextureLookupAndModulate(SkString* out,
                                        const char* modulation,
                                        const TextureSampler&,
                                        const char* coordName = NULL,
                                        GrSLType coordType = kVec2f_GrSLType) const;

    
    const char* defaultTexCoordsName() const { return fDefaultTexCoordsName.c_str(); }

    

    bool defaultTextureMatrixIsPerspective() const {
        return fTexCoordVaryingType == kVec3f_GrSLType;
    }

    

    void emitFunction(ShaderType shader,
                      GrSLType returnType,
                      const char* name,
                      int argCnt,
                      const GrGLShaderVar* args,
                      const char* body,
                      SkString* outName);

    


    static GrCustomStage::StageKey KeyForTextureAccess(const GrTextureAccess& access,
                                                       const GrGLCaps& caps);

    

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

    


    void addVarying(GrSLType type,
                    const char* name,
                    const char** vsOutName = NULL,
                    const char** fsInName = NULL);

    
    void getShader(ShaderType, SkString*) const;

    


    void finished(GrGLuint programID);

    



    void setCurrentStage(int stage) { fCurrentStage = stage; }
    void setNonStage() { fCurrentStage = kNonStageIdx; }

private:

    typedef GrTAllocator<GrGLShaderVar> VarArray;

    void appendDecls(const VarArray&, SkString*) const;
    void appendUniformDecls(ShaderType, SkString*) const;

    typedef GrGLUniformManager::BuilderUniform BuilderUniform;
    GrGLUniformManager::BuilderUniformArray fUniforms;

    
public:

    SkString    fHeader; 
    VarArray    fVSAttrs;
    VarArray    fVSOutputs;
    VarArray    fGSInputs;
    VarArray    fGSOutputs;
    VarArray    fFSInputs;
    SkString    fGSHeader; 
    VarArray    fFSOutputs;
    SkString    fVSCode;
    SkString    fGSCode;
    SkString    fFSCode;
    bool        fUsesGS;

private:
    enum {
        kNonStageIdx = -1,
    };

    const GrGLContextInfo&  fContext;
    GrGLUniformManager&     fUniformManager;
    int                     fCurrentStage;
    SkString                fFSFunctions;

    
    
    GrSLType         fTexCoordVaryingType;  
                                            
    SkString         fDefaultTexCoordsName; 
    

};

#endif
