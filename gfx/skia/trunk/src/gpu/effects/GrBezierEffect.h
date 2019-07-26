






#ifndef GrBezierEffect_DEFINED
#define GrBezierEffect_DEFINED

#include "GrDrawTargetCaps.h"
#include "GrEffect.h"
#include "GrVertexEffect.h"

enum GrBezierEdgeType {
    kFillAA_GrBezierEdgeType,
    kHairAA_GrBezierEdgeType,
    kFillNoAA_GrBezierEdgeType,
};

static inline bool GrBezierEdgeTypeIsFill(const GrBezierEdgeType edgeType) {
    return (kHairAA_GrBezierEdgeType != edgeType);
}

static inline bool GrBezierEdgeTypeIsAA(const GrBezierEdgeType edgeType) {
    return (kFillNoAA_GrBezierEdgeType != edgeType);
}









































class GrGLConicEffect;

class GrConicEffect : public GrVertexEffect {
public:
    static GrEffectRef* Create(const GrBezierEdgeType edgeType, const GrDrawTargetCaps& caps) {
        GR_CREATE_STATIC_EFFECT(gConicFillAA, GrConicEffect, (kFillAA_GrBezierEdgeType));
        GR_CREATE_STATIC_EFFECT(gConicHairAA, GrConicEffect, (kHairAA_GrBezierEdgeType));
        GR_CREATE_STATIC_EFFECT(gConicFillNoAA, GrConicEffect, (kFillNoAA_GrBezierEdgeType));
        if (kFillAA_GrBezierEdgeType == edgeType) {
            if (!caps.shaderDerivativeSupport()) {
                return NULL;
            }
            gConicFillAA->ref();
            return gConicFillAA;
        } else if (kHairAA_GrBezierEdgeType == edgeType) {
            if (!caps.shaderDerivativeSupport()) {
                return NULL;
            }
            gConicHairAA->ref();
            return gConicHairAA;
        } else {
            gConicFillNoAA->ref();
            return gConicFillNoAA;
        }
    }

    virtual ~GrConicEffect();

    static const char* Name() { return "Conic"; }

    inline bool isAntiAliased() const { return GrBezierEdgeTypeIsAA(fEdgeType); }
    inline bool isFilled() const { return GrBezierEdgeTypeIsFill(fEdgeType); }
    inline GrBezierEdgeType getEdgeType() const { return fEdgeType; }

    typedef GrGLConicEffect GLEffect;

    virtual void getConstantColorComponents(GrColor* color,
                                            uint32_t* validFlags) const SK_OVERRIDE {
        *validFlags = 0;
    }

    virtual const GrBackendEffectFactory& getFactory() const SK_OVERRIDE;

private:
    GrConicEffect(GrBezierEdgeType);

    virtual bool onIsEqual(const GrEffect& other) const SK_OVERRIDE;

    GrBezierEdgeType fEdgeType;

    GR_DECLARE_EFFECT_TEST;

    typedef GrVertexEffect INHERITED;
};










class GrGLQuadEffect;

class GrQuadEffect : public GrVertexEffect {
public:
    static GrEffectRef* Create(const GrBezierEdgeType edgeType, const GrDrawTargetCaps& caps) {
        GR_CREATE_STATIC_EFFECT(gQuadFillAA, GrQuadEffect, (kFillAA_GrBezierEdgeType));
        GR_CREATE_STATIC_EFFECT(gQuadHairAA, GrQuadEffect, (kHairAA_GrBezierEdgeType));
        GR_CREATE_STATIC_EFFECT(gQuadFillNoAA, GrQuadEffect, (kFillNoAA_GrBezierEdgeType));
        if (kFillAA_GrBezierEdgeType == edgeType) {
            if (!caps.shaderDerivativeSupport()) {
                return NULL;
            }
            gQuadFillAA->ref();
            return gQuadFillAA;
        } else if (kHairAA_GrBezierEdgeType == edgeType) {
            if (!caps.shaderDerivativeSupport()) {
                return NULL;
            }
            gQuadHairAA->ref();
            return gQuadHairAA;
        } else {
            gQuadFillNoAA->ref();
            return gQuadFillNoAA;
        }
    }

    virtual ~GrQuadEffect();

    static const char* Name() { return "Quad"; }

    inline bool isAntiAliased() const { return GrBezierEdgeTypeIsAA(fEdgeType); }
    inline bool isFilled() const { return GrBezierEdgeTypeIsFill(fEdgeType); }
    inline GrBezierEdgeType getEdgeType() const { return fEdgeType; }

    typedef GrGLQuadEffect GLEffect;

    virtual void getConstantColorComponents(GrColor* color,
                                            uint32_t* validFlags) const SK_OVERRIDE {
        *validFlags = 0;
    }

    virtual const GrBackendEffectFactory& getFactory() const SK_OVERRIDE;

private:
    GrQuadEffect(GrBezierEdgeType);

    virtual bool onIsEqual(const GrEffect& other) const SK_OVERRIDE;

    GrBezierEdgeType fEdgeType;

    GR_DECLARE_EFFECT_TEST;

    typedef GrVertexEffect INHERITED;
};












class GrGLCubicEffect;

class GrCubicEffect : public GrVertexEffect {
public:
    static GrEffectRef* Create(const GrBezierEdgeType edgeType, const GrDrawTargetCaps& caps) {
        GR_CREATE_STATIC_EFFECT(gCubicFillAA, GrCubicEffect, (kFillAA_GrBezierEdgeType));
        GR_CREATE_STATIC_EFFECT(gCubicHairAA, GrCubicEffect, (kHairAA_GrBezierEdgeType));
        GR_CREATE_STATIC_EFFECT(gCubicFillNoAA, GrCubicEffect, (kFillNoAA_GrBezierEdgeType));
        if (kFillAA_GrBezierEdgeType == edgeType) {
            if (!caps.shaderDerivativeSupport()) {
                return NULL;
            }
            gCubicFillAA->ref();
            return gCubicFillAA;
        } else if (kHairAA_GrBezierEdgeType == edgeType) {
            if (!caps.shaderDerivativeSupport()) {
                return NULL;
            }
            gCubicHairAA->ref();
            return gCubicHairAA;
        } else {
            gCubicFillNoAA->ref();
            return gCubicFillNoAA;
        }
    }

    virtual ~GrCubicEffect();

    static const char* Name() { return "Cubic"; }

    inline bool isAntiAliased() const { return GrBezierEdgeTypeIsAA(fEdgeType); }
    inline bool isFilled() const { return GrBezierEdgeTypeIsFill(fEdgeType); }
    inline GrBezierEdgeType getEdgeType() const { return fEdgeType; }

    typedef GrGLCubicEffect GLEffect;

    virtual void getConstantColorComponents(GrColor* color,
                                            uint32_t* validFlags) const SK_OVERRIDE {
        *validFlags = 0;
    }

    virtual const GrBackendEffectFactory& getFactory() const SK_OVERRIDE;

private:
    GrCubicEffect(GrBezierEdgeType);

    virtual bool onIsEqual(const GrEffect& other) const SK_OVERRIDE;

    GrBezierEdgeType fEdgeType;

    GR_DECLARE_EFFECT_TEST;

    typedef GrVertexEffect INHERITED;
};

#endif
