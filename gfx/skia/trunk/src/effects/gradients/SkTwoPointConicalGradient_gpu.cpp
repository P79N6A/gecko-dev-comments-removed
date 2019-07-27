







#include "SkTwoPointConicalGradient_gpu.h"

#include "SkTwoPointConicalGradient.h"

#if SK_SUPPORT_GPU
#include "GrTBackendEffectFactory.h"
#include "gl/GrGLShaderBuilder.h"

typedef GrGLUniformManager::UniformHandle UniformHandle;

static const SkScalar kErrorTol = 0.00001f;
static const SkScalar kEdgeErrorTol = 5.f * kErrorTol;









enum ConicalType {
    kInside_ConicalType,
    kOutside_ConicalType,
    kEdge_ConicalType,
};



static void set_matrix_edge_conical(const SkTwoPointConicalGradient& shader,
                                    SkMatrix* invLMatrix) {
    
    
    const SkPoint& center1 = shader.getStartCenter();
    const SkPoint& center2 = shader.getEndCenter();

    invLMatrix->postTranslate(-center1.fX, -center1.fY);

    SkPoint diff = center2 - center1;
    SkScalar diffLen = diff.length();
    if (0 != diffLen) {
        SkScalar invDiffLen = SkScalarInvert(diffLen);
        SkMatrix rot;
        rot.setSinCos(-SkScalarMul(invDiffLen, diff.fY),
                       SkScalarMul(invDiffLen, diff.fX));
        invLMatrix->postConcat(rot);
    }
}

class GLEdge2PtConicalEffect;

class Edge2PtConicalEffect : public GrGradientEffect {
public:

    static GrEffect* Create(GrContext* ctx,
                            const SkTwoPointConicalGradient& shader,
                            const SkMatrix& matrix,
                            SkShader::TileMode tm) {
        return SkNEW_ARGS(Edge2PtConicalEffect, (ctx, shader, matrix, tm));
    }

    virtual ~Edge2PtConicalEffect() {}

    static const char* Name() { return "Two-Point Conical Gradient Edge Touching"; }
    virtual const GrBackendEffectFactory& getFactory() const SK_OVERRIDE;

    
    SkScalar center() const { return fCenterX1; }
    SkScalar diffRadius() const { return fDiffRadius; }
    SkScalar radius() const { return fRadius0; }

    typedef GLEdge2PtConicalEffect GLEffect;

private:
    virtual bool onIsEqual(const GrEffect& sBase) const SK_OVERRIDE {
        const Edge2PtConicalEffect& s = CastEffect<Edge2PtConicalEffect>(sBase);
        return (INHERITED::onIsEqual(sBase) &&
                this->fCenterX1 == s.fCenterX1 &&
                this->fRadius0 == s.fRadius0 &&
                this->fDiffRadius == s.fDiffRadius);
    }

    Edge2PtConicalEffect(GrContext* ctx,
                         const SkTwoPointConicalGradient& shader,
                         const SkMatrix& matrix,
                         SkShader::TileMode tm)
        : INHERITED(ctx, shader, matrix, tm),
        fCenterX1(shader.getCenterX1()),
        fRadius0(shader.getStartRadius()),
        fDiffRadius(shader.getDiffRadius()){
        
        
        
        SkASSERT(SkScalarAbs(SkScalarAbs(fDiffRadius) - SkScalarAbs(fCenterX1)) <
                 kEdgeErrorTol * (fRadius0 < kErrorTol ? shader.getEndRadius() : fRadius0));

        
        
        fBTransform = this->getCoordTransform();
        SkMatrix& bMatrix = *fBTransform.accessMatrix();
        SkScalar r0dr = SkScalarMul(fRadius0, fDiffRadius);
        bMatrix[SkMatrix::kMScaleX] = -2 * (SkScalarMul(fCenterX1, bMatrix[SkMatrix::kMScaleX]) +
                                            SkScalarMul(r0dr, bMatrix[SkMatrix::kMPersp0]));
        bMatrix[SkMatrix::kMSkewX] = -2 * (SkScalarMul(fCenterX1, bMatrix[SkMatrix::kMSkewX]) +
                                           SkScalarMul(r0dr, bMatrix[SkMatrix::kMPersp1]));
        bMatrix[SkMatrix::kMTransX] = -2 * (SkScalarMul(fCenterX1, bMatrix[SkMatrix::kMTransX]) +
                                            SkScalarMul(r0dr, bMatrix[SkMatrix::kMPersp2]));
        this->addCoordTransform(&fBTransform);
    }

    GR_DECLARE_EFFECT_TEST;

    
    
    

    GrCoordTransform fBTransform;
    SkScalar         fCenterX1;
    SkScalar         fRadius0;
    SkScalar         fDiffRadius;

    

    typedef GrGradientEffect INHERITED;
};

class GLEdge2PtConicalEffect : public GrGLGradientEffect {
public:
    GLEdge2PtConicalEffect(const GrBackendEffectFactory& factory, const GrDrawEffect&);
    virtual ~GLEdge2PtConicalEffect() { }

    virtual void emitCode(GrGLShaderBuilder*,
                          const GrDrawEffect&,
                          const GrEffectKey&,
                          const char* outputColor,
                          const char* inputColor,
                          const TransformedCoordsArray&,
                          const TextureSamplerArray&) SK_OVERRIDE;
    virtual void setData(const GrGLUniformManager&, const GrDrawEffect&) SK_OVERRIDE;

    static void GenKey(const GrDrawEffect&, const GrGLCaps& caps, GrEffectKeyBuilder* b);

protected:
    UniformHandle fParamUni;

    const char* fVSVaryingName;
    const char* fFSVaryingName;

    
    

    SkScalar fCachedRadius;
    SkScalar fCachedDiffRadius;

    

private:
    typedef GrGLGradientEffect INHERITED;

};

const GrBackendEffectFactory& Edge2PtConicalEffect::getFactory() const {
    return GrTBackendEffectFactory<Edge2PtConicalEffect>::getInstance();
}

GR_DEFINE_EFFECT_TEST(Edge2PtConicalEffect);

GrEffect* Edge2PtConicalEffect::TestCreate(SkRandom* random,
                                           GrContext* context,
                                           const GrDrawTargetCaps&,
                                           GrTexture**) {
    SkPoint center1 = {random->nextUScalar1(), random->nextUScalar1()};
    SkScalar radius1 = random->nextUScalar1();
    SkPoint center2;
    SkScalar radius2;
    do {
        center2.set(random->nextUScalar1(), random->nextUScalar1());
        
        
    } while (center1 == center2);

    
    
    SkPoint diff = center2 - center1;
    SkScalar diffLen = diff.length();
    radius2 = radius1 + diffLen;

    SkColor colors[kMaxRandomGradientColors];
    SkScalar stopsArray[kMaxRandomGradientColors];
    SkScalar* stops = stopsArray;
    SkShader::TileMode tm;
    int colorCount = RandomGradientParams(random, colors, &stops, &tm);
    SkAutoTUnref<SkShader> shader(SkGradientShader::CreateTwoPointConical(center1, radius1,
                                                                          center2, radius2,
                                                                          colors, stops, colorCount,
                                                                          tm));
    SkPaint paint;
    GrEffect* effect;
    GrColor paintColor;
    SkAssertResult(shader->asNewEffect(context, paint, NULL, &paintColor, &effect));
    return effect;
}

GLEdge2PtConicalEffect::GLEdge2PtConicalEffect(const GrBackendEffectFactory& factory,
                                               const GrDrawEffect& drawEffect)
    : INHERITED(factory)
    , fVSVaryingName(NULL)
    , fFSVaryingName(NULL)
    , fCachedRadius(-SK_ScalarMax)
    , fCachedDiffRadius(-SK_ScalarMax) {}

void GLEdge2PtConicalEffect::emitCode(GrGLShaderBuilder* builder,
                                      const GrDrawEffect&,
                                      const GrEffectKey& key,
                                      const char* outputColor,
                                      const char* inputColor,
                                      const TransformedCoordsArray& coords,
                                      const TextureSamplerArray& samplers) {
    uint32_t baseKey = key.get32(0);
    this->emitUniforms(builder, baseKey);
    fParamUni = builder->addUniformArray(GrGLShaderBuilder::kFragment_Visibility,
                                         kFloat_GrSLType, "Conical2FSParams", 3);

    SkString cName("c");
    SkString tName("t");
    SkString p0; 
    SkString p1; 
    SkString p2; 

    builder->getUniformVariable(fParamUni).appendArrayAccess(0, &p0);
    builder->getUniformVariable(fParamUni).appendArrayAccess(1, &p1);
    builder->getUniformVariable(fParamUni).appendArrayAccess(2, &p2);

    
    SkASSERT(coords[0].type() == coords[1].type());
    const char* coords2D;
    SkString bVar;
    if (kVec3f_GrSLType == coords[0].type()) {
        builder->fsCodeAppendf("\tvec3 interpolants = vec3(%s.xy / %s.z, %s.x / %s.z);\n",
                               coords[0].c_str(), coords[0].c_str(), coords[1].c_str(), coords[1].c_str());
        coords2D = "interpolants.xy";
        bVar = "interpolants.z";
    } else {
        coords2D = coords[0].c_str();
        bVar.printf("%s.x", coords[1].c_str());
    }

    
    
    builder->fsCodeAppendf("\t%s = vec4(0.0,0.0,0.0,0.0);\n", outputColor);

    
    builder->fsCodeAppendf("\tfloat %s = dot(%s, %s) - %s;\n",
                           cName.c_str(), coords2D, coords2D, p1.c_str());

    
    builder->fsCodeAppendf("\tfloat %s = -(%s / %s);\n", tName.c_str(),
                           cName.c_str(), bVar.c_str());

    
    builder->fsCodeAppendf("\tif (%s * %s + %s > 0.0) {\n", tName.c_str(),
                           p2.c_str(), p0.c_str());
    builder->fsCodeAppend("\t");
    this->emitColor(builder, tName.c_str(), baseKey, outputColor, inputColor, samplers);
    builder->fsCodeAppend("\t}\n");
}

void GLEdge2PtConicalEffect::setData(const GrGLUniformManager& uman,
                                     const GrDrawEffect& drawEffect) {
    INHERITED::setData(uman, drawEffect);
    const Edge2PtConicalEffect& data = drawEffect.castEffect<Edge2PtConicalEffect>();
    SkScalar radius0 = data.radius();
    SkScalar diffRadius = data.diffRadius();

    if (fCachedRadius != radius0 ||
        fCachedDiffRadius != diffRadius) {

        float values[3] = {
            SkScalarToFloat(radius0),
            SkScalarToFloat(SkScalarMul(radius0, radius0)),
            SkScalarToFloat(diffRadius)
        };

        uman.set1fv(fParamUni, 3, values);
        fCachedRadius = radius0;
        fCachedDiffRadius = diffRadius;
    }
}

void GLEdge2PtConicalEffect::GenKey(const GrDrawEffect& drawEffect,
                                    const GrGLCaps&, GrEffectKeyBuilder* b) {
    b->add32(GenBaseGradientKey(drawEffect));
}





static ConicalType set_matrix_focal_conical(const SkTwoPointConicalGradient& shader,
                                            SkMatrix* invLMatrix, SkScalar* focalX) {
    
    
    
    ConicalType conicalType;
    const SkPoint& focal = shader.getStartCenter();
    const SkPoint& centerEnd = shader.getEndCenter();
    SkScalar radius = shader.getEndRadius();
    SkScalar invRadius = 1.f / radius;

    SkMatrix matrix;

    matrix.setTranslate(-centerEnd.fX, -centerEnd.fY);
    matrix.postScale(invRadius, invRadius);

    SkPoint focalTrans;
    matrix.mapPoints(&focalTrans, &focal, 1);
    *focalX = focalTrans.length();

    if (0.f != *focalX) {
        SkScalar invFocalX = SkScalarInvert(*focalX);
        SkMatrix rot;
        rot.setSinCos(-SkScalarMul(invFocalX, focalTrans.fY),
                      SkScalarMul(invFocalX, focalTrans.fX));
        matrix.postConcat(rot);
    }

    matrix.postTranslate(-(*focalX), 0.f);

    
    
    
    
    if (SkScalarAbs(1.f - (*focalX)) < kEdgeErrorTol) {
        return kEdge_ConicalType;
    }

    
    SkScalar oneMinusF2 = 1.f - SkScalarMul(*focalX, *focalX);
    SkScalar s = SkScalarDiv(1.f, oneMinusF2);


    if (s >= 0.f) {
        conicalType = kInside_ConicalType;
        matrix.postScale(s, s * SkScalarSqrt(oneMinusF2));
    } else {
        conicalType = kOutside_ConicalType;
        matrix.postScale(s, s);
    }

    invLMatrix->postConcat(matrix);

    return conicalType;
}



class GLFocalOutside2PtConicalEffect;

class FocalOutside2PtConicalEffect : public GrGradientEffect {
public:

    static GrEffect* Create(GrContext* ctx,
                            const SkTwoPointConicalGradient& shader,
                            const SkMatrix& matrix,
                            SkShader::TileMode tm,
                            SkScalar focalX) {
        return SkNEW_ARGS(FocalOutside2PtConicalEffect, (ctx, shader, matrix, tm, focalX));
    }

    virtual ~FocalOutside2PtConicalEffect() { }

    static const char* Name() { return "Two-Point Conical Gradient Focal Outside"; }
    virtual const GrBackendEffectFactory& getFactory() const SK_OVERRIDE;

    bool isFlipped() const { return fIsFlipped; }
    SkScalar focal() const { return fFocalX; }

    typedef GLFocalOutside2PtConicalEffect GLEffect;

private:
    virtual bool onIsEqual(const GrEffect& sBase) const SK_OVERRIDE {
        const FocalOutside2PtConicalEffect& s = CastEffect<FocalOutside2PtConicalEffect>(sBase);
        return (INHERITED::onIsEqual(sBase) &&
                this->fFocalX == s.fFocalX &&
                this->fIsFlipped == s.fIsFlipped);
    }

    FocalOutside2PtConicalEffect(GrContext* ctx,
                                 const SkTwoPointConicalGradient& shader,
                                 const SkMatrix& matrix,
                                 SkShader::TileMode tm,
                                 SkScalar focalX)
    : INHERITED(ctx, shader, matrix, tm), fFocalX(focalX), fIsFlipped(shader.isFlippedGrad()) {}

    GR_DECLARE_EFFECT_TEST;

    SkScalar         fFocalX;
    bool             fIsFlipped;

    typedef GrGradientEffect INHERITED;
};

class GLFocalOutside2PtConicalEffect : public GrGLGradientEffect {
public:
    GLFocalOutside2PtConicalEffect(const GrBackendEffectFactory& factory, const GrDrawEffect&);
    virtual ~GLFocalOutside2PtConicalEffect() { }

    virtual void emitCode(GrGLShaderBuilder*,
                          const GrDrawEffect&,
                          const GrEffectKey&,
                          const char* outputColor,
                          const char* inputColor,
                          const TransformedCoordsArray&,
                          const TextureSamplerArray&) SK_OVERRIDE;
    virtual void setData(const GrGLUniformManager&, const GrDrawEffect&) SK_OVERRIDE;

    static void GenKey(const GrDrawEffect&, const GrGLCaps& caps, GrEffectKeyBuilder* b);

protected:
    UniformHandle fParamUni;

    const char* fVSVaryingName;
    const char* fFSVaryingName;

    bool fIsFlipped;

    
    

    SkScalar fCachedFocal;

    

private:
    typedef GrGLGradientEffect INHERITED;

};

const GrBackendEffectFactory& FocalOutside2PtConicalEffect::getFactory() const {
    return GrTBackendEffectFactory<FocalOutside2PtConicalEffect>::getInstance();
}

GR_DEFINE_EFFECT_TEST(FocalOutside2PtConicalEffect);

GrEffect* FocalOutside2PtConicalEffect::TestCreate(SkRandom* random,
                                                   GrContext* context,
                                                   const GrDrawTargetCaps&,
                                                   GrTexture**) {
    SkPoint center1 = {random->nextUScalar1(), random->nextUScalar1()};
    SkScalar radius1 = 0.f;
    SkPoint center2;
    SkScalar radius2;
    do {
        center2.set(random->nextUScalar1(), random->nextUScalar1());
        
    } while (center1 == center2);
        SkPoint diff = center2 - center1;
        SkScalar diffLen = diff.length();
        
        radius2 = random->nextRangeF(0.f, diffLen);

    SkColor colors[kMaxRandomGradientColors];
    SkScalar stopsArray[kMaxRandomGradientColors];
    SkScalar* stops = stopsArray;
    SkShader::TileMode tm;
    int colorCount = RandomGradientParams(random, colors, &stops, &tm);
    SkAutoTUnref<SkShader> shader(SkGradientShader::CreateTwoPointConical(center1, radius1,
                                                                          center2, radius2,
                                                                          colors, stops, colorCount,
                                                                          tm));
    SkPaint paint;
    GrEffect* effect;
    GrColor paintColor;
    SkAssertResult(shader->asNewEffect(context, paint, NULL, &paintColor, &effect));
    return effect;
}

GLFocalOutside2PtConicalEffect::GLFocalOutside2PtConicalEffect(const GrBackendEffectFactory& factory,
                                                               const GrDrawEffect& drawEffect)
    : INHERITED(factory)
    , fVSVaryingName(NULL)
    , fFSVaryingName(NULL)
    , fCachedFocal(SK_ScalarMax) {
    const FocalOutside2PtConicalEffect& data = drawEffect.castEffect<FocalOutside2PtConicalEffect>();
    fIsFlipped = data.isFlipped();
}

void GLFocalOutside2PtConicalEffect::emitCode(GrGLShaderBuilder* builder,
                                              const GrDrawEffect&,
                                              const GrEffectKey& key,
                                              const char* outputColor,
                                              const char* inputColor,
                                              const TransformedCoordsArray& coords,
                                              const TextureSamplerArray& samplers) {
    uint32_t baseKey = key.get32(0);
    this->emitUniforms(builder, baseKey);
    fParamUni = builder->addUniformArray(GrGLShaderBuilder::kFragment_Visibility,
                                         kFloat_GrSLType, "Conical2FSParams", 2);
    SkString tName("t");
    SkString p0; 
    SkString p1; 

    builder->getUniformVariable(fParamUni).appendArrayAccess(0, &p0);
    builder->getUniformVariable(fParamUni).appendArrayAccess(1, &p1);

    
    SkString coords2DString = builder->ensureFSCoords2D(coords, 0);
    const char* coords2D = coords2DString.c_str();

    

    
    
    builder->fsCodeAppendf("\t%s = vec4(0.0,0.0,0.0,0.0);\n", outputColor);

    builder->fsCodeAppendf("\tfloat xs = %s.x * %s.x;\n", coords2D, coords2D);
    builder->fsCodeAppendf("\tfloat ys = %s.y * %s.y;\n", coords2D, coords2D);
    builder->fsCodeAppendf("\tfloat d = xs + %s * ys;\n", p1.c_str());

    
    
    if (!fIsFlipped) {
        builder->fsCodeAppendf("\tfloat %s = %s.x * %s  + sqrt(d);\n", tName.c_str(),
                               coords2D, p0.c_str());
    } else {
        builder->fsCodeAppendf("\tfloat %s = %s.x * %s  - sqrt(d);\n", tName.c_str(),
                               coords2D, p0.c_str());
    }

    builder->fsCodeAppendf("\tif (%s >= 0.0 && d >= 0.0) {\n", tName.c_str());
    builder->fsCodeAppend("\t\t");
    this->emitColor(builder, tName.c_str(), baseKey, outputColor, inputColor, samplers);
    builder->fsCodeAppend("\t}\n");
}

void GLFocalOutside2PtConicalEffect::setData(const GrGLUniformManager& uman,
                                             const GrDrawEffect& drawEffect) {
    INHERITED::setData(uman, drawEffect);
    const FocalOutside2PtConicalEffect& data = drawEffect.castEffect<FocalOutside2PtConicalEffect>();
    SkASSERT(data.isFlipped() == fIsFlipped);
    SkScalar focal = data.focal();

    if (fCachedFocal != focal) {
        SkScalar oneMinus2F = 1.f - SkScalarMul(focal, focal);

        float values[2] = {
            SkScalarToFloat(focal),
            SkScalarToFloat(oneMinus2F),
        };

        uman.set1fv(fParamUni, 2, values);
        fCachedFocal = focal;
    }
}

void GLFocalOutside2PtConicalEffect::GenKey(const GrDrawEffect& drawEffect,
                                            const GrGLCaps&, GrEffectKeyBuilder* b) {
    uint32_t* key = b->add32n(2);
    key[0] = GenBaseGradientKey(drawEffect);
    key[1] = drawEffect.castEffect<FocalOutside2PtConicalEffect>().isFlipped();
}



class GLFocalInside2PtConicalEffect;

class FocalInside2PtConicalEffect : public GrGradientEffect {
public:

    static GrEffect* Create(GrContext* ctx,
                            const SkTwoPointConicalGradient& shader,
                            const SkMatrix& matrix,
                            SkShader::TileMode tm,
                            SkScalar focalX) {
        return SkNEW_ARGS(FocalInside2PtConicalEffect, (ctx, shader, matrix, tm, focalX));
    }

    virtual ~FocalInside2PtConicalEffect() {}

    static const char* Name() { return "Two-Point Conical Gradient Focal Inside"; }
    virtual const GrBackendEffectFactory& getFactory() const SK_OVERRIDE;

    SkScalar focal() const { return fFocalX; }

    typedef GLFocalInside2PtConicalEffect GLEffect;

private:
    virtual bool onIsEqual(const GrEffect& sBase) const SK_OVERRIDE {
        const FocalInside2PtConicalEffect& s = CastEffect<FocalInside2PtConicalEffect>(sBase);
        return (INHERITED::onIsEqual(sBase) &&
                this->fFocalX == s.fFocalX);
    }

    FocalInside2PtConicalEffect(GrContext* ctx,
                                const SkTwoPointConicalGradient& shader,
                                const SkMatrix& matrix,
                                SkShader::TileMode tm,
                                SkScalar focalX)
        : INHERITED(ctx, shader, matrix, tm), fFocalX(focalX) {}

    GR_DECLARE_EFFECT_TEST;

    SkScalar         fFocalX;

    typedef GrGradientEffect INHERITED;
};

class GLFocalInside2PtConicalEffect : public GrGLGradientEffect {
public:
    GLFocalInside2PtConicalEffect(const GrBackendEffectFactory& factory, const GrDrawEffect&);
    virtual ~GLFocalInside2PtConicalEffect() {}

    virtual void emitCode(GrGLShaderBuilder*,
                          const GrDrawEffect&,
                          const GrEffectKey&,
                          const char* outputColor,
                          const char* inputColor,
                          const TransformedCoordsArray&,
                          const TextureSamplerArray&) SK_OVERRIDE;
    virtual void setData(const GrGLUniformManager&, const GrDrawEffect&) SK_OVERRIDE;

    static void GenKey(const GrDrawEffect&, const GrGLCaps& caps, GrEffectKeyBuilder* b);

protected:
    UniformHandle fFocalUni;

    const char* fVSVaryingName;
    const char* fFSVaryingName;

    
    

    SkScalar fCachedFocal;

    

private:
    typedef GrGLGradientEffect INHERITED;

};

const GrBackendEffectFactory& FocalInside2PtConicalEffect::getFactory() const {
    return GrTBackendEffectFactory<FocalInside2PtConicalEffect>::getInstance();
}

GR_DEFINE_EFFECT_TEST(FocalInside2PtConicalEffect);

GrEffect* FocalInside2PtConicalEffect::TestCreate(SkRandom* random,
                                                  GrContext* context,
                                                  const GrDrawTargetCaps&,
                                                  GrTexture**) {
    SkPoint center1 = {random->nextUScalar1(), random->nextUScalar1()};
    SkScalar radius1 = 0.f;
    SkPoint center2;
    SkScalar radius2;
    do {
        center2.set(random->nextUScalar1(), random->nextUScalar1());
        
        
        SkScalar increase = random->nextUScalar1();
        SkPoint diff = center2 - center1;
        SkScalar diffLen = diff.length();
        radius2 = diffLen + increase;
        
    } while (radius1 == radius2 && center1 == center2);

    SkColor colors[kMaxRandomGradientColors];
    SkScalar stopsArray[kMaxRandomGradientColors];
    SkScalar* stops = stopsArray;
    SkShader::TileMode tm;
    int colorCount = RandomGradientParams(random, colors, &stops, &tm);
    SkAutoTUnref<SkShader> shader(SkGradientShader::CreateTwoPointConical(center1, radius1,
                                                                          center2, radius2,
                                                                          colors, stops, colorCount,
                                                                          tm));
    SkPaint paint;
    GrColor paintColor;
    GrEffect* effect;
    SkAssertResult(shader->asNewEffect(context, paint, NULL, &paintColor, &effect));
    return effect;
}

GLFocalInside2PtConicalEffect::GLFocalInside2PtConicalEffect(const GrBackendEffectFactory& factory,
                                                             const GrDrawEffect& drawEffect)
    : INHERITED(factory)
    , fVSVaryingName(NULL)
    , fFSVaryingName(NULL)
    , fCachedFocal(SK_ScalarMax) {}

void GLFocalInside2PtConicalEffect::emitCode(GrGLShaderBuilder* builder,
                                             const GrDrawEffect&,
                                             const GrEffectKey& key,
                                             const char* outputColor,
                                             const char* inputColor,
                                             const TransformedCoordsArray& coords,
                                             const TextureSamplerArray& samplers) {
    uint32_t baseKey = key.get32(0);
    this->emitUniforms(builder, baseKey);
    fFocalUni = builder->addUniform(GrGLShaderBuilder::kFragment_Visibility,
                                    kFloat_GrSLType, "Conical2FSParams");
    SkString tName("t");

    
    
    GrGLShaderVar focal = builder->getUniformVariable(fFocalUni);

    
    SkString coords2DString = builder->ensureFSCoords2D(coords, 0);
    const char* coords2D = coords2DString.c_str();

    
    builder->fsCodeAppendf("\tfloat %s = %s.x * %s  + length(%s);\n", tName.c_str(),
                           coords2D, focal.c_str(), coords2D);

    this->emitColor(builder, tName.c_str(), baseKey, outputColor, inputColor, samplers);
}

void GLFocalInside2PtConicalEffect::setData(const GrGLUniformManager& uman,
                                            const GrDrawEffect& drawEffect) {
    INHERITED::setData(uman, drawEffect);
    const FocalInside2PtConicalEffect& data = drawEffect.castEffect<FocalInside2PtConicalEffect>();
    SkScalar focal = data.focal();

    if (fCachedFocal != focal) {
        uman.set1f(fFocalUni, SkScalarToFloat(focal));
        fCachedFocal = focal;
    }
}

void GLFocalInside2PtConicalEffect::GenKey(const GrDrawEffect& drawEffect,
                                           const GrGLCaps&, GrEffectKeyBuilder* b) {
    b->add32(GenBaseGradientKey(drawEffect));
}





struct CircleConicalInfo {
    SkPoint fCenterEnd;
    SkScalar fA;
    SkScalar fB;
    SkScalar fC;
};


static ConicalType set_matrix_circle_conical(const SkTwoPointConicalGradient& shader,
                                             SkMatrix* invLMatrix, CircleConicalInfo* info) {
    
    
    const SkPoint& centerStart = shader.getStartCenter();
    const SkPoint& centerEnd = shader.getEndCenter();
    SkScalar radiusStart = shader.getStartRadius();
    SkScalar radiusEnd = shader.getEndRadius();

    SkMatrix matrix;

    matrix.setTranslate(-centerStart.fX, -centerStart.fY);

    SkScalar invStartRad = 1.f / radiusStart;
    matrix.postScale(invStartRad, invStartRad);

    radiusEnd /= radiusStart;

    SkPoint centerEndTrans;
    matrix.mapPoints(&centerEndTrans, &centerEnd, 1);

    SkScalar A = centerEndTrans.fX * centerEndTrans.fX + centerEndTrans.fY * centerEndTrans.fY
                 - radiusEnd * radiusEnd + 2 * radiusEnd - 1;

    
    
    
    
    
    if (SkScalarAbs(A) < kEdgeErrorTol) {
        return kEdge_ConicalType;
    }

    SkScalar C = 1.f / A;
    SkScalar B = (radiusEnd - 1.f) * C;

    matrix.postScale(C, C);

    invLMatrix->postConcat(matrix);

    info->fCenterEnd = centerEndTrans;
    info->fA = A;
    info->fB = B;
    info->fC = C;

    
    if (A < 0.f) {
        return kInside_ConicalType;
    }
    return kOutside_ConicalType;
}

class GLCircleInside2PtConicalEffect;

class CircleInside2PtConicalEffect : public GrGradientEffect {
public:

    static GrEffect* Create(GrContext* ctx,
                            const SkTwoPointConicalGradient& shader,
                            const SkMatrix& matrix,
                            SkShader::TileMode tm,
                            const CircleConicalInfo& info) {
        return SkNEW_ARGS(CircleInside2PtConicalEffect, (ctx, shader, matrix, tm, info));
    }

    virtual ~CircleInside2PtConicalEffect() {}

    static const char* Name() { return "Two-Point Conical Gradient Inside"; }
    virtual const GrBackendEffectFactory& getFactory() const SK_OVERRIDE;

    SkScalar centerX() const { return fInfo.fCenterEnd.fX; }
    SkScalar centerY() const { return fInfo.fCenterEnd.fY; }
    SkScalar A() const { return fInfo.fA; }
    SkScalar B() const { return fInfo.fB; }
    SkScalar C() const { return fInfo.fC; }

    typedef GLCircleInside2PtConicalEffect GLEffect;

private:
    virtual bool onIsEqual(const GrEffect& sBase) const SK_OVERRIDE {
        const CircleInside2PtConicalEffect& s = CastEffect<CircleInside2PtConicalEffect>(sBase);
        return (INHERITED::onIsEqual(sBase) &&
                this->fInfo.fCenterEnd == s.fInfo.fCenterEnd &&
                this->fInfo.fA == s.fInfo.fA &&
                this->fInfo.fB == s.fInfo.fB &&
                this->fInfo.fC == s.fInfo.fC);
    }

    CircleInside2PtConicalEffect(GrContext* ctx,
                                 const SkTwoPointConicalGradient& shader,
                                 const SkMatrix& matrix,
                                 SkShader::TileMode tm,
                                 const CircleConicalInfo& info)
        : INHERITED(ctx, shader, matrix, tm), fInfo(info) {}

    GR_DECLARE_EFFECT_TEST;

    const CircleConicalInfo fInfo;

    typedef GrGradientEffect INHERITED;
};

class GLCircleInside2PtConicalEffect : public GrGLGradientEffect {
public:
    GLCircleInside2PtConicalEffect(const GrBackendEffectFactory& factory, const GrDrawEffect&);
    virtual ~GLCircleInside2PtConicalEffect() {}

    virtual void emitCode(GrGLShaderBuilder*,
                          const GrDrawEffect&,
                          const GrEffectKey&,
                          const char* outputColor,
                          const char* inputColor,
                          const TransformedCoordsArray&,
                          const TextureSamplerArray&) SK_OVERRIDE;
    virtual void setData(const GrGLUniformManager&, const GrDrawEffect&) SK_OVERRIDE;

    static void GenKey(const GrDrawEffect&, const GrGLCaps& caps, GrEffectKeyBuilder* b);

protected:
    UniformHandle fCenterUni;
    UniformHandle fParamUni;

    const char* fVSVaryingName;
    const char* fFSVaryingName;

    
    

    SkScalar fCachedCenterX;
    SkScalar fCachedCenterY;
    SkScalar fCachedA;
    SkScalar fCachedB;
    SkScalar fCachedC;

    

private:
    typedef GrGLGradientEffect INHERITED;

};

const GrBackendEffectFactory& CircleInside2PtConicalEffect::getFactory() const {
    return GrTBackendEffectFactory<CircleInside2PtConicalEffect>::getInstance();
}

GR_DEFINE_EFFECT_TEST(CircleInside2PtConicalEffect);

GrEffect* CircleInside2PtConicalEffect::TestCreate(SkRandom* random,
                                                   GrContext* context,
                                                   const GrDrawTargetCaps&,
                                                   GrTexture**) {
    SkPoint center1 = {random->nextUScalar1(), random->nextUScalar1()};
    SkScalar radius1 = random->nextUScalar1() + 0.0001f; 
    SkPoint center2;
    SkScalar radius2;
    do {
        center2.set(random->nextUScalar1(), random->nextUScalar1());
        
        SkScalar increase = random->nextUScalar1();
        SkPoint diff = center2 - center1;
        SkScalar diffLen = diff.length();
        radius2 = radius1 + diffLen + increase;
        
    } while (radius1 == radius2 && center1 == center2);

    SkColor colors[kMaxRandomGradientColors];
    SkScalar stopsArray[kMaxRandomGradientColors];
    SkScalar* stops = stopsArray;
    SkShader::TileMode tm;
    int colorCount = RandomGradientParams(random, colors, &stops, &tm);
    SkAutoTUnref<SkShader> shader(SkGradientShader::CreateTwoPointConical(center1, radius1,
                                                                          center2, radius2,
                                                                          colors, stops, colorCount,
                                                                          tm));
    SkPaint paint;
    GrColor paintColor;
    GrEffect* effect;
    SkAssertResult(shader->asNewEffect(context, paint, NULL, &paintColor, &effect));
    return effect;
}

GLCircleInside2PtConicalEffect::GLCircleInside2PtConicalEffect(const GrBackendEffectFactory& factory,
                                                               const GrDrawEffect& drawEffect)
    : INHERITED(factory)
    , fVSVaryingName(NULL)
    , fFSVaryingName(NULL)
    , fCachedCenterX(SK_ScalarMax)
    , fCachedCenterY(SK_ScalarMax)
    , fCachedA(SK_ScalarMax)
    , fCachedB(SK_ScalarMax)
    , fCachedC(SK_ScalarMax) {}

void GLCircleInside2PtConicalEffect::emitCode(GrGLShaderBuilder* builder,
                                              const GrDrawEffect&,
                                              const GrEffectKey& key,
                                              const char* outputColor,
                                              const char* inputColor,
                                              const TransformedCoordsArray& coords,
                                              const TextureSamplerArray& samplers) {
    uint32_t baseKey = key.get32(0);
    this->emitUniforms(builder, baseKey);
    fCenterUni = builder->addUniform(GrGLShaderBuilder::kFragment_Visibility,
                                     kVec2f_GrSLType, "Conical2FSCenter");
    fParamUni = builder->addUniform(GrGLShaderBuilder::kFragment_Visibility,
                                    kVec3f_GrSLType, "Conical2FSParams");
    SkString tName("t");

    GrGLShaderVar center = builder->getUniformVariable(fCenterUni);
    
    
    
    GrGLShaderVar params = builder->getUniformVariable(fParamUni);

    
    SkString coords2DString = builder->ensureFSCoords2D(coords, 0);
    const char* coords2D = coords2DString.c_str();

    
    
    
    
    
    
    
    
    builder->fsCodeAppendf("\tfloat pDotp = dot(%s,  %s);\n", coords2D, coords2D);
    builder->fsCodeAppendf("\tfloat d = dot(%s,  %s) + %s.y;\n", coords2D, center.c_str(), params.c_str());
    builder->fsCodeAppendf("\tfloat %s = d + sqrt(d * d - %s.x * pDotp + %s.z);\n",
                           tName.c_str(), params.c_str(), params.c_str());

    this->emitColor(builder, tName.c_str(), baseKey, outputColor, inputColor, samplers);
}

void GLCircleInside2PtConicalEffect::setData(const GrGLUniformManager& uman,
                                             const GrDrawEffect& drawEffect) {
    INHERITED::setData(uman, drawEffect);
    const CircleInside2PtConicalEffect& data = drawEffect.castEffect<CircleInside2PtConicalEffect>();
    SkScalar centerX = data.centerX();
    SkScalar centerY = data.centerY();
    SkScalar A = data.A();
    SkScalar B = data.B();
    SkScalar C = data.C();

    if (fCachedCenterX != centerX || fCachedCenterY != centerY ||
        fCachedA != A || fCachedB != B || fCachedC != C) {

        uman.set2f(fCenterUni, SkScalarToFloat(centerX), SkScalarToFloat(centerY));
        uman.set3f(fParamUni, SkScalarToFloat(A), SkScalarToFloat(B), SkScalarToFloat(C));

        fCachedCenterX = centerX;
        fCachedCenterY = centerY;
        fCachedA = A;
        fCachedB = B;
        fCachedC = C;
    }
}

void GLCircleInside2PtConicalEffect::GenKey(const GrDrawEffect& drawEffect,
                                            const GrGLCaps&, GrEffectKeyBuilder* b) {
    b->add32(GenBaseGradientKey(drawEffect));
}



class GLCircleOutside2PtConicalEffect;

class CircleOutside2PtConicalEffect : public GrGradientEffect {
public:

    static GrEffect* Create(GrContext* ctx,
                            const SkTwoPointConicalGradient& shader,
                            const SkMatrix& matrix,
                            SkShader::TileMode tm,
                            const CircleConicalInfo& info) {
        return SkNEW_ARGS(CircleOutside2PtConicalEffect, (ctx, shader, matrix, tm, info));
    }

    virtual ~CircleOutside2PtConicalEffect() {}

    static const char* Name() { return "Two-Point Conical Gradient Outside"; }
    virtual const GrBackendEffectFactory& getFactory() const SK_OVERRIDE;

    SkScalar centerX() const { return fInfo.fCenterEnd.fX; }
    SkScalar centerY() const { return fInfo.fCenterEnd.fY; }
    SkScalar A() const { return fInfo.fA; }
    SkScalar B() const { return fInfo.fB; }
    SkScalar C() const { return fInfo.fC; }
    SkScalar tLimit() const { return fTLimit; }
    bool isFlipped() const { return fIsFlipped; }

    typedef GLCircleOutside2PtConicalEffect GLEffect;

private:
    virtual bool onIsEqual(const GrEffect& sBase) const SK_OVERRIDE {
        const CircleOutside2PtConicalEffect& s = CastEffect<CircleOutside2PtConicalEffect>(sBase);
        return (INHERITED::onIsEqual(sBase) &&
                this->fInfo.fCenterEnd == s.fInfo.fCenterEnd &&
                this->fInfo.fA == s.fInfo.fA &&
                this->fInfo.fB == s.fInfo.fB &&
                this->fInfo.fC == s.fInfo.fC &&
                this->fTLimit == s.fTLimit &&
                this->fIsFlipped == s.fIsFlipped);
    }

    CircleOutside2PtConicalEffect(GrContext* ctx,
                                  const SkTwoPointConicalGradient& shader,
                                  const SkMatrix& matrix,
                                  SkShader::TileMode tm,
                                  const CircleConicalInfo& info)
        : INHERITED(ctx, shader, matrix, tm), fInfo(info) {
        if (shader.getStartRadius() != shader.getEndRadius()) {
            fTLimit = SkScalarDiv(shader.getStartRadius(), (shader.getStartRadius() - shader.getEndRadius()));
        } else {
            fTLimit = SK_ScalarMin;
        }

        fIsFlipped = shader.isFlippedGrad();
    }

    GR_DECLARE_EFFECT_TEST;

    const CircleConicalInfo fInfo;
    SkScalar fTLimit;
    bool fIsFlipped;

    typedef GrGradientEffect INHERITED;
};

class GLCircleOutside2PtConicalEffect : public GrGLGradientEffect {
public:
    GLCircleOutside2PtConicalEffect(const GrBackendEffectFactory& factory, const GrDrawEffect&);
    virtual ~GLCircleOutside2PtConicalEffect() {}

    virtual void emitCode(GrGLShaderBuilder*,
                          const GrDrawEffect&,
                          const GrEffectKey&,
                          const char* outputColor,
                          const char* inputColor,
                          const TransformedCoordsArray&,
                          const TextureSamplerArray&) SK_OVERRIDE;
    virtual void setData(const GrGLUniformManager&, const GrDrawEffect&) SK_OVERRIDE;

    static void GenKey(const GrDrawEffect&, const GrGLCaps& caps, GrEffectKeyBuilder* b);

protected:
    UniformHandle fCenterUni;
    UniformHandle fParamUni;

    const char* fVSVaryingName;
    const char* fFSVaryingName;

    bool fIsFlipped;

    
    

    SkScalar fCachedCenterX;
    SkScalar fCachedCenterY;
    SkScalar fCachedA;
    SkScalar fCachedB;
    SkScalar fCachedC;
    SkScalar fCachedTLimit;

    

private:
    typedef GrGLGradientEffect INHERITED;

};

const GrBackendEffectFactory& CircleOutside2PtConicalEffect::getFactory() const {
    return GrTBackendEffectFactory<CircleOutside2PtConicalEffect>::getInstance();
}

GR_DEFINE_EFFECT_TEST(CircleOutside2PtConicalEffect);

GrEffect* CircleOutside2PtConicalEffect::TestCreate(SkRandom* random,
                                                    GrContext* context,
                                                    const GrDrawTargetCaps&,
                                                    GrTexture**) {
    SkPoint center1 = {random->nextUScalar1(), random->nextUScalar1()};
    SkScalar radius1 = random->nextUScalar1() + 0.0001f; 
    SkPoint center2;
    SkScalar radius2;
    SkScalar diffLen;
    do {
        center2.set(random->nextUScalar1(), random->nextUScalar1());
        
    } while (center1 == center2);
        SkPoint diff = center2 - center1;
        diffLen = diff.length();
        
        
        radius2 = radius1 + random->nextRangeF(0.f, diffLen);

    SkColor colors[kMaxRandomGradientColors];
    SkScalar stopsArray[kMaxRandomGradientColors];
    SkScalar* stops = stopsArray;
    SkShader::TileMode tm;
    int colorCount = RandomGradientParams(random, colors, &stops, &tm);
    SkAutoTUnref<SkShader> shader(SkGradientShader::CreateTwoPointConical(center1, radius1,
                                                                          center2, radius2,
                                                                          colors, stops, colorCount,
                                                                          tm));
    SkPaint paint;
    GrColor paintColor;
    GrEffect* effect;
    SkAssertResult(shader->asNewEffect(context, paint, NULL, &paintColor, &effect));
    return effect;
}

GLCircleOutside2PtConicalEffect::GLCircleOutside2PtConicalEffect(const GrBackendEffectFactory& factory,
                                                                 const GrDrawEffect& drawEffect)
    : INHERITED(factory)
    , fVSVaryingName(NULL)
    , fFSVaryingName(NULL)
    , fCachedCenterX(SK_ScalarMax)
    , fCachedCenterY(SK_ScalarMax)
    , fCachedA(SK_ScalarMax)
    , fCachedB(SK_ScalarMax)
    , fCachedC(SK_ScalarMax)
    , fCachedTLimit(SK_ScalarMax) {
    const CircleOutside2PtConicalEffect& data = drawEffect.castEffect<CircleOutside2PtConicalEffect>();
    fIsFlipped = data.isFlipped();
    }

void GLCircleOutside2PtConicalEffect::emitCode(GrGLShaderBuilder* builder,
                                               const GrDrawEffect&,
                                               const GrEffectKey& key,
                                               const char* outputColor,
                                               const char* inputColor,
                                               const TransformedCoordsArray& coords,
                                               const TextureSamplerArray& samplers) {
    uint32_t baseKey = key.get32(0);
    this->emitUniforms(builder, baseKey);
    fCenterUni = builder->addUniform(GrGLShaderBuilder::kFragment_Visibility,
                                     kVec2f_GrSLType, "Conical2FSCenter");
    fParamUni = builder->addUniform(GrGLShaderBuilder::kFragment_Visibility,
                                    kVec4f_GrSLType, "Conical2FSParams");
    SkString tName("t");

    GrGLShaderVar center = builder->getUniformVariable(fCenterUni);
    
    
    
    GrGLShaderVar params = builder->getUniformVariable(fParamUni);

    
    SkString coords2DString = builder->ensureFSCoords2D(coords, 0);
    const char* coords2D = coords2DString.c_str();

    
    
    builder->fsCodeAppendf("\t%s = vec4(0.0,0.0,0.0,0.0);\n", outputColor);

    
    
    
    
    
    
    
    

    builder->fsCodeAppendf("\tfloat pDotp = dot(%s,  %s);\n", coords2D, coords2D);
    builder->fsCodeAppendf("\tfloat d = dot(%s,  %s) + %s.y;\n", coords2D, center.c_str(), params.c_str());
    builder->fsCodeAppendf("\tfloat deter = d * d - %s.x * pDotp + %s.z;\n", params.c_str(), params.c_str());

    
    
    if (!fIsFlipped) {
        builder->fsCodeAppendf("\tfloat %s = d + sqrt(deter);\n", tName.c_str());
    } else {
        builder->fsCodeAppendf("\tfloat %s = d - sqrt(deter);\n", tName.c_str());
    }

    builder->fsCodeAppendf("\tif (%s >= %s.w && deter >= 0.0) {\n", tName.c_str(), params.c_str());
    builder->fsCodeAppend("\t\t");
    this->emitColor(builder, tName.c_str(), baseKey, outputColor, inputColor, samplers);
    builder->fsCodeAppend("\t}\n");
}

void GLCircleOutside2PtConicalEffect::setData(const GrGLUniformManager& uman,
                                              const GrDrawEffect& drawEffect) {
    INHERITED::setData(uman, drawEffect);
    const CircleOutside2PtConicalEffect& data = drawEffect.castEffect<CircleOutside2PtConicalEffect>();
    SkASSERT(data.isFlipped() == fIsFlipped);
    SkScalar centerX = data.centerX();
    SkScalar centerY = data.centerY();
    SkScalar A = data.A();
    SkScalar B = data.B();
    SkScalar C = data.C();
    SkScalar tLimit = data.tLimit();

    if (fCachedCenterX != centerX || fCachedCenterY != centerY ||
        fCachedA != A || fCachedB != B || fCachedC != C || fCachedTLimit != tLimit) {

        uman.set2f(fCenterUni, SkScalarToFloat(centerX), SkScalarToFloat(centerY));
        uman.set4f(fParamUni, SkScalarToFloat(A), SkScalarToFloat(B), SkScalarToFloat(C),
                   SkScalarToFloat(tLimit));

        fCachedCenterX = centerX;
        fCachedCenterY = centerY;
        fCachedA = A;
        fCachedB = B;
        fCachedC = C;
        fCachedTLimit = tLimit;
    }
}

void GLCircleOutside2PtConicalEffect::GenKey(const GrDrawEffect& drawEffect,
                                             const GrGLCaps&, GrEffectKeyBuilder* b) {
    uint32_t* key = b->add32n(2);
    key[0] = GenBaseGradientKey(drawEffect);
    key[1] = drawEffect.castEffect<CircleOutside2PtConicalEffect>().isFlipped();
}



GrEffect* Gr2PtConicalGradientEffect::Create(GrContext* ctx,
                                             const SkTwoPointConicalGradient& shader,
                                             SkShader::TileMode tm,
                                             const SkMatrix* localMatrix) {
    SkMatrix matrix;
    if (!shader.getLocalMatrix().invert(&matrix)) {
        return NULL;
    }
    if (localMatrix) {
        SkMatrix inv;
        if (!localMatrix->invert(&inv)) {
            return NULL;
        }
        matrix.postConcat(inv);
    }

    if (shader.getStartRadius() < kErrorTol) {
        SkScalar focalX;
        ConicalType type = set_matrix_focal_conical(shader, &matrix, &focalX);
        if (type == kInside_ConicalType) {
            return FocalInside2PtConicalEffect::Create(ctx, shader, matrix, tm, focalX);
        } else if(type == kEdge_ConicalType) {
            set_matrix_edge_conical(shader, &matrix);
            return Edge2PtConicalEffect::Create(ctx, shader, matrix, tm);
        } else {
            return FocalOutside2PtConicalEffect::Create(ctx, shader, matrix, tm, focalX);
        }
    }

    CircleConicalInfo info;
    ConicalType type = set_matrix_circle_conical(shader, &matrix, &info);

    if (type == kInside_ConicalType) {
        return CircleInside2PtConicalEffect::Create(ctx, shader, matrix, tm, info);
    } else if (type == kEdge_ConicalType) {
        set_matrix_edge_conical(shader, &matrix);
        return Edge2PtConicalEffect::Create(ctx, shader, matrix, tm);
    } else {
        return CircleOutside2PtConicalEffect::Create(ctx, shader, matrix, tm, info);
    }
}

#endif
