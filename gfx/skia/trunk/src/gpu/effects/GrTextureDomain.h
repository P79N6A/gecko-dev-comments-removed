






#ifndef GrTextureDomainEffect_DEFINED
#define GrTextureDomainEffect_DEFINED

#include "GrSingleTextureEffect.h"
#include "gl/GrGLEffect.h"

class GrGLShaderBuilder;
struct SkRect;







class GrTextureDomain {
public:
    enum Mode {
        kIgnore_Mode,  
        kClamp_Mode,   
        kDecal_Mode,   

        kLastMode = kDecal_Mode
    };
    static const int kModeCount = kLastMode + 1;

    static const GrTextureDomain& IgnoredDomain() {
        static const SkRect gDummyRect = {0, 0, 0, 0};
        static const GrTextureDomain gDomain(gDummyRect, kIgnore_Mode);
        return gDomain;
    }

    



    GrTextureDomain(const SkRect& domain, Mode, int index = -1);

    const SkRect& domain() const { return fDomain; }
    Mode mode() const { return fMode; }

    

    static const SkRect MakeTexelDomain(const GrTexture* texture, const SkIRect& texelRect) {
        SkScalar wInv = SK_Scalar1 / texture->width();
        SkScalar hInv = SK_Scalar1 / texture->height();
        SkRect result = {
            texelRect.fLeft * wInv,
            texelRect.fTop * hInv,
            texelRect.fRight * wInv,
            texelRect.fBottom * hInv
        };
        return result;
    }

    bool operator== (const GrTextureDomain& that) const {
        return fMode == that.fMode && fDomain == that.fDomain;
    }

    





    class GLDomain {
    public:
        GLDomain() {
            fPrevDomain[0] = SK_FloatNaN;
            SkDEBUGCODE(fMode = (Mode) -1;)
        }

        








        void sampleTexture(GrGLShaderBuilder* builder,
                           const GrTextureDomain& textureDomain,
                           const char* outColor,
                           const SkString& inCoords,
                           const GrGLEffect::TextureSampler sampler,
                           const char* inModulateColor = NULL);

        



        void setData(const GrGLUniformManager& uman, const GrTextureDomain& textureDomain,
                     GrSurfaceOrigin textureOrigin);

        enum {
            kDomainKeyBits = 2, 
        };

        



        static uint32_t DomainKey(const GrTextureDomain& domain) {
            GR_STATIC_ASSERT(kModeCount <= 4);
            return domain.mode();
        }

    private:
        SkDEBUGCODE(Mode                  fMode;)
        GrGLUniformManager::UniformHandle fDomainUni;
        SkString                          fDomainName;
        GrGLfloat                         fPrevDomain[4];
    };

protected:
    Mode    fMode;
    SkRect  fDomain;
    int     fIndex;

    typedef GrSingleTextureEffect INHERITED;
};

class GrGLTextureDomainEffect;




class GrTextureDomainEffect : public GrSingleTextureEffect {

public:
    static GrEffect* Create(GrTexture*,
                            const SkMatrix&,
                            const SkRect& domain,
                            GrTextureDomain::Mode,
                            GrTextureParams::FilterMode filterMode,
                            GrCoordSet = kLocal_GrCoordSet);

    virtual ~GrTextureDomainEffect();

    static const char* Name() { return "TextureDomain"; }

    typedef GrGLTextureDomainEffect GLEffect;

    virtual const GrBackendEffectFactory& getFactory() const SK_OVERRIDE;
    virtual void getConstantColorComponents(GrColor* color, uint32_t* validFlags) const SK_OVERRIDE;

    const GrTextureDomain& textureDomain() const { return fTextureDomain; }

protected:
    GrTextureDomain fTextureDomain;

private:
    GrTextureDomainEffect(GrTexture*,
                          const SkMatrix&,
                          const SkRect& domain,
                          GrTextureDomain::Mode,
                          GrTextureParams::FilterMode,
                          GrCoordSet);

    virtual bool onIsEqual(const GrEffect&) const SK_OVERRIDE;

    GR_DECLARE_EFFECT_TEST;

    typedef GrSingleTextureEffect INHERITED;
};

#endif
