






#ifndef GrBezierEffect_DEFINED
#define GrBezierEffect_DEFINED

#include "GrDrawTargetCaps.h"
#include "GrEffect.h"
#include "GrVertexEffect.h"
#include "GrTypesPriv.h"









































class GrGLConicEffect;

class GrConicEffect : public GrVertexEffect {
public:
    static GrEffect* Create(const GrEffectEdgeType edgeType, const GrDrawTargetCaps& caps) {
        GR_CREATE_STATIC_EFFECT(gConicFillAA, GrConicEffect, (kFillAA_GrEffectEdgeType));
        GR_CREATE_STATIC_EFFECT(gConicHairAA, GrConicEffect, (kHairlineAA_GrEffectEdgeType));
        GR_CREATE_STATIC_EFFECT(gConicFillBW, GrConicEffect, (kFillBW_GrEffectEdgeType));
        switch (edgeType) {
            case kFillAA_GrEffectEdgeType:
                if (!caps.shaderDerivativeSupport()) {
                    return NULL;
                }
                gConicFillAA->ref();
                return gConicFillAA;
            case kHairlineAA_GrEffectEdgeType:
                if (!caps.shaderDerivativeSupport()) {
                    return NULL;
                }
                gConicHairAA->ref();
                return gConicHairAA;
            case kFillBW_GrEffectEdgeType:
                gConicFillBW->ref();
                return gConicFillBW;
            default:
                return NULL;
        }
    }

    virtual ~GrConicEffect();

    static const char* Name() { return "Conic"; }

    inline bool isAntiAliased() const { return GrEffectEdgeTypeIsAA(fEdgeType); }
    inline bool isFilled() const { return GrEffectEdgeTypeIsFill(fEdgeType); }
    inline GrEffectEdgeType getEdgeType() const { return fEdgeType; }

    typedef GrGLConicEffect GLEffect;

    virtual void getConstantColorComponents(GrColor* color,
                                            uint32_t* validFlags) const SK_OVERRIDE {
        *validFlags = 0;
    }

    virtual const GrBackendEffectFactory& getFactory() const SK_OVERRIDE;

private:
    GrConicEffect(GrEffectEdgeType);

    virtual bool onIsEqual(const GrEffect& other) const SK_OVERRIDE;

    GrEffectEdgeType fEdgeType;

    GR_DECLARE_EFFECT_TEST;

    typedef GrVertexEffect INHERITED;
};










class GrGLQuadEffect;

class GrQuadEffect : public GrVertexEffect {
public:
    static GrEffect* Create(const GrEffectEdgeType edgeType, const GrDrawTargetCaps& caps) {
        GR_CREATE_STATIC_EFFECT(gQuadFillAA, GrQuadEffect, (kFillAA_GrEffectEdgeType));
        GR_CREATE_STATIC_EFFECT(gQuadHairAA, GrQuadEffect, (kHairlineAA_GrEffectEdgeType));
        GR_CREATE_STATIC_EFFECT(gQuadFillBW, GrQuadEffect, (kFillBW_GrEffectEdgeType));
        switch (edgeType) {
            case kFillAA_GrEffectEdgeType:
                if (!caps.shaderDerivativeSupport()) {
                    return NULL;
                }
                gQuadFillAA->ref();
                return gQuadFillAA;
            case kHairlineAA_GrEffectEdgeType:
                if (!caps.shaderDerivativeSupport()) {
                    return NULL;
                }
                gQuadHairAA->ref();
                return gQuadHairAA;
            case kFillBW_GrEffectEdgeType:
                gQuadFillBW->ref();
                return gQuadFillBW;
            default:
                return NULL;
        }
    }

    virtual ~GrQuadEffect();

    static const char* Name() { return "Quad"; }

    inline bool isAntiAliased() const { return GrEffectEdgeTypeIsAA(fEdgeType); }
    inline bool isFilled() const { return GrEffectEdgeTypeIsFill(fEdgeType); }
    inline GrEffectEdgeType getEdgeType() const { return fEdgeType; }

    typedef GrGLQuadEffect GLEffect;

    virtual void getConstantColorComponents(GrColor* color,
                                            uint32_t* validFlags) const SK_OVERRIDE {
        *validFlags = 0;
    }

    virtual const GrBackendEffectFactory& getFactory() const SK_OVERRIDE;

private:
    GrQuadEffect(GrEffectEdgeType);

    virtual bool onIsEqual(const GrEffect& other) const SK_OVERRIDE;

    GrEffectEdgeType fEdgeType;

    GR_DECLARE_EFFECT_TEST;

    typedef GrVertexEffect INHERITED;
};












class GrGLCubicEffect;

class GrCubicEffect : public GrVertexEffect {
public:
    static GrEffect* Create(const GrEffectEdgeType edgeType, const GrDrawTargetCaps& caps) {
        GR_CREATE_STATIC_EFFECT(gCubicFillAA, GrCubicEffect, (kFillAA_GrEffectEdgeType));
        GR_CREATE_STATIC_EFFECT(gCubicHairAA, GrCubicEffect, (kHairlineAA_GrEffectEdgeType));
        GR_CREATE_STATIC_EFFECT(gCubicFillBW, GrCubicEffect, (kFillBW_GrEffectEdgeType));
        switch (edgeType) {
            case kFillAA_GrEffectEdgeType:
                if (!caps.shaderDerivativeSupport()) {
                    return NULL;
                }
                gCubicFillAA->ref();
                return gCubicFillAA;
            case kHairlineAA_GrEffectEdgeType:
                if (!caps.shaderDerivativeSupport()) {
                    return NULL;
                }
                gCubicHairAA->ref();
                return gCubicHairAA;
            case kFillBW_GrEffectEdgeType:
                gCubicFillBW->ref();
                return gCubicFillBW;
            default:
                return NULL;
        }
    }

    virtual ~GrCubicEffect();

    static const char* Name() { return "Cubic"; }

    inline bool isAntiAliased() const { return GrEffectEdgeTypeIsAA(fEdgeType); }
    inline bool isFilled() const { return GrEffectEdgeTypeIsFill(fEdgeType); }
    inline GrEffectEdgeType getEdgeType() const { return fEdgeType; }

    typedef GrGLCubicEffect GLEffect;

    virtual void getConstantColorComponents(GrColor* color,
                                            uint32_t* validFlags) const SK_OVERRIDE {
        *validFlags = 0;
    }

    virtual const GrBackendEffectFactory& getFactory() const SK_OVERRIDE;

private:
    GrCubicEffect(GrEffectEdgeType);

    virtual bool onIsEqual(const GrEffect& other) const SK_OVERRIDE;

    GrEffectEdgeType fEdgeType;

    GR_DECLARE_EFFECT_TEST;

    typedef GrVertexEffect INHERITED;
};

#endif
