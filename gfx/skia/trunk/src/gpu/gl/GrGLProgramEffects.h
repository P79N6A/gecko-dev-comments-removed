






#ifndef GrGLProgramEffects_DEFINED
#define GrGLProgramEffects_DEFINED

#include "GrBackendEffectFactory.h"
#include "GrGLUniformManager.h"
#include "GrTexture.h"
#include "GrTextureAccess.h"

class GrEffect;
class GrEffectStage;
class GrGLVertexProgramEffectsBuilder;
class GrGLShaderBuilder;
class GrGLFullShaderBuilder;
class GrGLFragmentOnlyShaderBuilder;






class GrGLProgramEffects : public SkRefCnt {
public:
    typedef GrGLUniformManager::UniformHandle UniformHandle;

    







    static bool GenEffectMetaKey(const GrDrawEffect&, const GrGLCaps&, GrEffectKeyBuilder*);

    virtual ~GrGLProgramEffects();

    



    void initSamplers(const GrGLUniformManager&, int* texUnitIdx);

    


    virtual void setData(GrGpuGL*,
                         const GrGLUniformManager&,
                         const GrEffectStage* effectStages[]) = 0;

    


    class TransformedCoords {
    public:
        TransformedCoords(const SkString& name, GrSLType type)
            : fName(name), fType(type) {
        }

        const char* c_str() const { return fName.c_str(); }
        GrSLType type() const { return fType; }
        const SkString& getName() const { return fName; }

    private:
        SkString fName;
        GrSLType fType;
    };

    typedef SkTArray<TransformedCoords> TransformedCoordsArray;

    


    class TextureSampler {
    public:
        TextureSampler(UniformHandle uniform, const GrTextureAccess& access)
            : fSamplerUniform(uniform)
            , fConfigComponentMask(GrPixelConfigComponentMask(access.getTexture()->config())) {
            SkASSERT(0 != fConfigComponentMask);
            memcpy(fSwizzle, access.getSwizzle(), 5);
        }

        UniformHandle samplerUniform() const { return fSamplerUniform; }
        
        uint32_t configComponentMask() const { return fConfigComponentMask; }
        const char* swizzle() const { return fSwizzle; }

    private:
        UniformHandle fSamplerUniform;
        uint32_t      fConfigComponentMask;
        char          fSwizzle[5];
    };

    typedef SkTArray<TextureSampler> TextureSamplerArray;

protected:
    


    static uint32_t GenAttribKey(const GrDrawEffect&);
    static uint32_t GenTransformKey(const GrDrawEffect&);
    static uint32_t GenTextureKey(const GrDrawEffect&, const GrGLCaps&);

    GrGLProgramEffects(int reserveCount)
        : fGLEffects(reserveCount)
        , fSamplers(reserveCount) {
    }

    




    void emitSamplers(GrGLShaderBuilder*, const GrEffect*, TextureSamplerArray*);

    


    void bindTextures(GrGpuGL*, const GrEffect*, int effectIdx);

    struct Sampler {
        SkDEBUGCODE(Sampler() : fTextureUnit(-1) {})
        UniformHandle fUniform;
        int           fTextureUnit;
    };

    SkTArray<GrGLEffect*>                  fGLEffects;
    SkTArray<SkSTArray<4, Sampler, true> > fSamplers;

private:
    typedef SkRefCnt INHERITED;
};




class GrGLProgramEffectsBuilder {
public:
    virtual ~GrGLProgramEffectsBuilder() { }

    


    virtual void emitEffect(const GrEffectStage&,
                            const GrEffectKey&,
                            const char* outColor,
                            const char* inColor,
                            int stageIndex) = 0;
};






class GrGLVertexProgramEffects : public GrGLProgramEffects {
public:
    virtual void setData(GrGpuGL*,
                         const GrGLUniformManager&,
                         const GrEffectStage* effectStages[]) SK_OVERRIDE;

private:
    friend class GrGLVertexProgramEffectsBuilder;

    GrGLVertexProgramEffects(int reserveCount, bool explicitLocalCoords)
        : INHERITED(reserveCount)
        , fTransforms(reserveCount)
        , fHasExplicitLocalCoords(explicitLocalCoords) {
    }

    



    void emitEffect(GrGLFullShaderBuilder*,
                    const GrEffectStage&,
                    const GrEffectKey&,
                    const char* outColor,
                    const char* inColor,
                    int stageIndex);

    


    void emitAttributes(GrGLFullShaderBuilder*, const GrEffectStage&);

    






    void emitTransforms(GrGLFullShaderBuilder*,
                        const GrDrawEffect&,
                        TransformedCoordsArray*);

    


    void setTransformData(const GrGLUniformManager&, const GrDrawEffect&, int effectIdx);

    struct Transform {
        Transform() { fCurrentValue = SkMatrix::InvalidMatrix(); }
        UniformHandle fHandle;
        SkMatrix      fCurrentValue;
    };

    SkTArray<SkSTArray<2, Transform, true> > fTransforms;
    bool                                     fHasExplicitLocalCoords;

    typedef GrGLProgramEffects INHERITED;
};




class GrGLVertexProgramEffectsBuilder : public GrGLProgramEffectsBuilder {
public:
    GrGLVertexProgramEffectsBuilder(GrGLFullShaderBuilder*, int reserveCount);
    virtual ~GrGLVertexProgramEffectsBuilder() { }

    virtual void emitEffect(const GrEffectStage&,
                            const GrEffectKey&,
                            const char* outColor,
                            const char* inColor,
                            int stageIndex) SK_OVERRIDE;

    



    GrGLProgramEffects* finish() { return fProgramEffects.detach(); }

private:
    GrGLFullShaderBuilder*                  fBuilder;
    SkAutoTDelete<GrGLVertexProgramEffects> fProgramEffects;

    typedef GrGLProgramEffectsBuilder INHERITED;
};







class GrGLPathTexGenProgramEffects : public GrGLProgramEffects {
public:
    virtual void setData(GrGpuGL*,
                         const GrGLUniformManager&,
                         const GrEffectStage* effectStages[]) SK_OVERRIDE;

private:
    friend class GrGLPathTexGenProgramEffectsBuilder;

    GrGLPathTexGenProgramEffects(int reserveCount)
        : INHERITED(reserveCount)
        , fTransforms(reserveCount) {
    }

    



    void emitEffect(GrGLFragmentOnlyShaderBuilder*,
                    const GrEffectStage&,
                    const GrEffectKey&,
                    const char* outColor,
                    const char* inColor,
                    int stageIndex);

    







    void setupPathTexGen(GrGLFragmentOnlyShaderBuilder*,
                         const GrDrawEffect&,
                         TransformedCoordsArray*);

    


    void setPathTexGenState(GrGpuGL*, const GrDrawEffect&, int effectIdx);

    struct Transforms {
        Transforms(uint32_t transformKey, int texCoordIndex)
            : fTransformKey(transformKey), fTexCoordIndex(texCoordIndex) {}
        uint32_t    fTransformKey;
        int         fTexCoordIndex;
    };

    SkTArray<Transforms> fTransforms;

    typedef GrGLProgramEffects INHERITED;
};




class GrGLPathTexGenProgramEffectsBuilder : public GrGLProgramEffectsBuilder {
public:
    GrGLPathTexGenProgramEffectsBuilder(GrGLFragmentOnlyShaderBuilder*, int reserveCount);
    virtual ~GrGLPathTexGenProgramEffectsBuilder() { }

    virtual void emitEffect(const GrEffectStage&,
                            const GrEffectKey&,
                            const char* outColor,
                            const char* inColor,
                            int stageIndex) SK_OVERRIDE;

    



    GrGLProgramEffects* finish() { return fProgramEffects.detach(); }

private:
    GrGLFragmentOnlyShaderBuilder*          fBuilder;
    SkAutoTDelete<GrGLPathTexGenProgramEffects> fProgramEffects;

    typedef GrGLProgramEffectsBuilder INHERITED;
};

#endif
