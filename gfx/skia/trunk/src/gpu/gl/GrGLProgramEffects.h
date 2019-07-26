






#ifndef GrGLProgramEffects_DEFINED
#define GrGLProgramEffects_DEFINED

#include "GrBackendEffectFactory.h"
#include "GrTexture.h"
#include "GrTextureAccess.h"
#include "GrGLUniformManager.h"

class GrEffectStage;
class GrGLVertexProgramEffectsBuilder;
class GrGLShaderBuilder;
class GrGLFullShaderBuilder;
class GrGLFragmentOnlyShaderBuilder;






class GrGLProgramEffects {
public:
    typedef GrBackendEffectFactory::EffectKey EffectKey;
    typedef GrGLUniformManager::UniformHandle UniformHandle;

    


    static EffectKey GenAttribKey(const GrDrawEffect&);
    static EffectKey GenTransformKey(const GrDrawEffect&);
    static EffectKey GenTextureKey(const GrDrawEffect&, const GrGLCaps&);

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
    GrGLProgramEffects(int reserveCount)
        : fGLEffects(reserveCount)
        , fSamplers(reserveCount) {
    }

    




    void emitSamplers(GrGLShaderBuilder*, const GrEffectRef&, TextureSamplerArray*);

    


    void bindTextures(GrGpuGL*, const GrEffectRef&, int effectIdx);

    struct Sampler {
        SkDEBUGCODE(Sampler() : fTextureUnit(-1) {})
        UniformHandle fUniform;
        int           fTextureUnit;
    };

    SkTArray<GrGLEffect*>                  fGLEffects;
    SkTArray<SkSTArray<4, Sampler, true> > fSamplers;
};




class GrGLProgramEffectsBuilder {
public:
    virtual ~GrGLProgramEffectsBuilder() { }

    


    virtual void emitEffect(const GrEffectStage&,
                            GrGLProgramEffects::EffectKey,
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
                    GrGLProgramEffects::EffectKey,
                    const char* outColor,
                    const char* inColor,
                    int stageIndex);

    


    void emitAttributes(GrGLFullShaderBuilder*, const GrEffectStage&);

    






    void emitTransforms(GrGLFullShaderBuilder*,
                        const GrEffectRef&,
                        EffectKey,
                        TransformedCoordsArray*);

    


    void setTransformData(const GrGLUniformManager&, const GrDrawEffect&, int effectIdx);

    struct Transform {
        Transform() { fCurrentValue = SkMatrix::InvalidMatrix(); }
        UniformHandle fHandle;
        GrSLType      fType;
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
                            GrGLProgramEffects::EffectKey,
                            const char* outColor,
                            const char* inColor,
                            int stageIndex) SK_OVERRIDE;

    



    GrGLProgramEffects* finish() { return fProgramEffects.detach(); }

private:
    GrGLFullShaderBuilder*                  fBuilder;
    SkAutoTDelete<GrGLVertexProgramEffects> fProgramEffects;

    typedef GrGLProgramEffectsBuilder INHERITED;
};







class GrGLTexGenProgramEffects : public GrGLProgramEffects {
public:
    virtual void setData(GrGpuGL*,
                         const GrGLUniformManager&,
                         const GrEffectStage* effectStages[]) SK_OVERRIDE;

private:
    friend class GrGLTexGenProgramEffectsBuilder;

    GrGLTexGenProgramEffects(int reserveCount)
        : INHERITED(reserveCount)
        , fTransforms(reserveCount) {
    }

    



    void emitEffect(GrGLFragmentOnlyShaderBuilder*,
                    const GrEffectStage&,
                    GrGLProgramEffects::EffectKey,
                    const char* outColor,
                    const char* inColor,
                    int stageIndex);

    







    void setupTexGen(GrGLFragmentOnlyShaderBuilder*,
                     const GrEffectRef&,
                     EffectKey,
                     TransformedCoordsArray*);

    


    void setTexGenState(GrGpuGL*, const GrDrawEffect&, int effectIdx);

    struct Transforms {
        Transforms(EffectKey transformKey, int texCoordIndex)
            : fTransformKey(transformKey), fTexCoordIndex(texCoordIndex) {}
        EffectKey fTransformKey;
        int fTexCoordIndex;
    };

    SkTArray<Transforms> fTransforms;

    typedef GrGLProgramEffects INHERITED;
};




class GrGLTexGenProgramEffectsBuilder : public GrGLProgramEffectsBuilder {
public:
    GrGLTexGenProgramEffectsBuilder(GrGLFragmentOnlyShaderBuilder*, int reserveCount);
    virtual ~GrGLTexGenProgramEffectsBuilder() { }

    virtual void emitEffect(const GrEffectStage&,
                            GrGLProgramEffects::EffectKey,
                            const char* outColor,
                            const char* inColor,
                            int stageIndex) SK_OVERRIDE;

    



    GrGLProgramEffects* finish() { return fProgramEffects.detach(); }

private:
    GrGLFragmentOnlyShaderBuilder*          fBuilder;
    SkAutoTDelete<GrGLTexGenProgramEffects> fProgramEffects;

    typedef GrGLProgramEffectsBuilder INHERITED;
};

#endif
