







#include "SkTwoPointConicalGradient.h"

static int valid_divide(float numer, float denom, float* ratio) {
    SkASSERT(ratio);
    if (0 == denom) {
        return 0;
    }
    *ratio = numer / denom;
    return 1;
}



static int find_quad_roots(float A, float B, float C, float roots[2]) {
    SkASSERT(roots);

    if (A == 0) {
        return valid_divide(-C, B, roots);
    }

    float R = B*B - 4*A*C;
    if (R < 0) {
        return 0;
    }
    R = sk_float_sqrt(R);

#if 1
    float Q = B;
    if (Q < 0) {
        Q -= R;
    } else {
        Q += R;
    }
#else
    
    float Q = B + copysignf(R, B);
#endif
    Q *= -0.5f;
    if (0 == Q) {
        roots[0] = 0;
        return 1;
    }

    float r0 = Q / A;
    float r1 = C / Q;
    roots[0] = r0 < r1 ? r0 : r1;
    roots[1] = r0 > r1 ? r0 : r1;
    return 2;
}

static float lerp(float x, float dx, float t) {
    return x + t * dx;
}

static float sqr(float x) { return x * x; }

void TwoPtRadial::init(const SkPoint& center0, SkScalar rad0,
                       const SkPoint& center1, SkScalar rad1) {
    fCenterX = SkScalarToFloat(center0.fX);
    fCenterY = SkScalarToFloat(center0.fY);
    fDCenterX = SkScalarToFloat(center1.fX) - fCenterX;
    fDCenterY = SkScalarToFloat(center1.fY) - fCenterY;
    fRadius = SkScalarToFloat(rad0);
    fDRadius = SkScalarToFloat(rad1) - fRadius;

    fA = sqr(fDCenterX) + sqr(fDCenterY) - sqr(fDRadius);
    fRadius2 = sqr(fRadius);
    fRDR = fRadius * fDRadius;
}

void TwoPtRadial::setup(SkScalar fx, SkScalar fy, SkScalar dfx, SkScalar dfy) {
    fRelX = SkScalarToFloat(fx) - fCenterX;
    fRelY = SkScalarToFloat(fy) - fCenterY;
    fIncX = SkScalarToFloat(dfx);
    fIncY = SkScalarToFloat(dfy);
    fB = -2 * (fDCenterX * fRelX + fDCenterY * fRelY + fRDR);
    fDB = -2 * (fDCenterX * fIncX + fDCenterY * fIncY);
}

SkFixed TwoPtRadial::nextT() {
    float roots[2];

    float C = sqr(fRelX) + sqr(fRelY) - fRadius2;
    int countRoots = find_quad_roots(fA, fB, C, roots);

    fRelX += fIncX;
    fRelY += fIncY;
    fB += fDB;

    if (0 == countRoots) {
        return kDontDrawT;
    }

    
    
    float t = roots[countRoots - 1];
    float r = lerp(fRadius, fDRadius, t);
    if (r <= 0) {
        t = roots[0];   
        r = lerp(fRadius, fDRadius, t);
        if (r <= 0) {
            return kDontDrawT;
        }
    }
    return SkFloatToFixed(t);
}

typedef void (*TwoPointConicalProc)(TwoPtRadial* rec, SkPMColor* dstC,
                                    const SkPMColor* cache, int toggle, int count);

static void twopoint_clamp(TwoPtRadial* rec, SkPMColor* SK_RESTRICT dstC,
                           const SkPMColor* SK_RESTRICT cache, int toggle,
                           int count) {
    for (; count > 0; --count) {
        SkFixed t = rec->nextT();
        if (TwoPtRadial::DontDrawT(t)) {
            *dstC++ = 0;
        } else {
            SkFixed index = SkClampMax(t, 0xFFFF);
            SkASSERT(index <= 0xFFFF);
            *dstC++ = cache[toggle +
                            (index >> SkGradientShaderBase::kCache32Shift)];
        }
        toggle = next_dither_toggle(toggle);
    }
}

static void twopoint_repeat(TwoPtRadial* rec, SkPMColor* SK_RESTRICT dstC,
                            const SkPMColor* SK_RESTRICT cache, int toggle,
                            int count) {
    for (; count > 0; --count) {
        SkFixed t = rec->nextT();
        if (TwoPtRadial::DontDrawT(t)) {
            *dstC++ = 0;
        } else {
            SkFixed index = repeat_tileproc(t);
            SkASSERT(index <= 0xFFFF);
            *dstC++ = cache[toggle +
                            (index >> SkGradientShaderBase::kCache32Shift)];
        }
        toggle = next_dither_toggle(toggle);
    }
}

static void twopoint_mirror(TwoPtRadial* rec, SkPMColor* SK_RESTRICT dstC,
                            const SkPMColor* SK_RESTRICT cache, int toggle,
                            int count) {
    for (; count > 0; --count) {
        SkFixed t = rec->nextT();
        if (TwoPtRadial::DontDrawT(t)) {
            *dstC++ = 0;
        } else {
            SkFixed index = mirror_tileproc(t);
            SkASSERT(index <= 0xFFFF);
            *dstC++ = cache[toggle +
                            (index >> SkGradientShaderBase::kCache32Shift)];
        }
        toggle = next_dither_toggle(toggle);
    }
}

void SkTwoPointConicalGradient::init() {
    fRec.init(fCenter1, fRadius1, fCenter2, fRadius2);
    fPtsToUnit.reset();
}



SkTwoPointConicalGradient::SkTwoPointConicalGradient(
    const SkPoint& start, SkScalar startRadius,
    const SkPoint& end, SkScalar endRadius,
    const SkColor colors[], const SkScalar pos[],
    int colorCount, SkShader::TileMode mode,
    SkUnitMapper* mapper)
    : SkGradientShaderBase(colors, pos, colorCount, mode, mapper),
    fCenter1(start),
    fCenter2(end),
    fRadius1(startRadius),
    fRadius2(endRadius) {
    
    SkASSERT(fCenter1 != fCenter2 || fRadius1 != fRadius2);
    this->init();
}

bool SkTwoPointConicalGradient::isOpaque() const {
    
    
    
    return false;
}

void SkTwoPointConicalGradient::shadeSpan(int x, int y, SkPMColor* dstCParam,
                                          int count) {
    int toggle = init_dither_toggle(x, y);

    SkASSERT(count > 0);

    SkPMColor* SK_RESTRICT dstC = dstCParam;

    SkMatrix::MapXYProc dstProc = fDstToIndexProc;

    const SkPMColor* SK_RESTRICT cache = this->getCache32();

    TwoPointConicalProc shadeProc = twopoint_repeat;
    if (SkShader::kClamp_TileMode == fTileMode) {
        shadeProc = twopoint_clamp;
    } else if (SkShader::kMirror_TileMode == fTileMode) {
        shadeProc = twopoint_mirror;
    } else {
        SkASSERT(SkShader::kRepeat_TileMode == fTileMode);
    }

    if (fDstToIndexClass != kPerspective_MatrixClass) {
        SkPoint srcPt;
        dstProc(fDstToIndex, SkIntToScalar(x) + SK_ScalarHalf,
                SkIntToScalar(y) + SK_ScalarHalf, &srcPt);
        SkScalar dx, fx = srcPt.fX;
        SkScalar dy, fy = srcPt.fY;

        if (fDstToIndexClass == kFixedStepInX_MatrixClass) {
            SkFixed fixedX, fixedY;
            (void)fDstToIndex.fixedStepInX(SkIntToScalar(y), &fixedX, &fixedY);
            dx = SkFixedToScalar(fixedX);
            dy = SkFixedToScalar(fixedY);
        } else {
            SkASSERT(fDstToIndexClass == kLinear_MatrixClass);
            dx = fDstToIndex.getScaleX();
            dy = fDstToIndex.getSkewY();
        }

        fRec.setup(fx, fy, dx, dy);
        (*shadeProc)(&fRec, dstC, cache, toggle, count);
    } else {    
        SkScalar dstX = SkIntToScalar(x);
        SkScalar dstY = SkIntToScalar(y);
        for (; count > 0; --count) {
            SkPoint srcPt;
            dstProc(fDstToIndex, dstX, dstY, &srcPt);
            dstX += SK_Scalar1;

            fRec.setup(srcPt.fX, srcPt.fY, 0, 0);
            (*shadeProc)(&fRec, dstC, cache, toggle, 1);
            toggle = next_dither_toggle(toggle);
        }
    }
}

bool SkTwoPointConicalGradient::setContext(const SkBitmap& device,
                                           const SkPaint& paint,
                                           const SkMatrix& matrix) {
    if (!this->INHERITED::setContext(device, paint, matrix)) {
        return false;
    }

    
    fFlags &= ~kHasSpan16_Flag;

    
    
    fFlags &= ~kOpaqueAlpha_Flag;

    return true;
}

SkShader::BitmapType SkTwoPointConicalGradient::asABitmap(
    SkBitmap* bitmap, SkMatrix* matrix, SkShader::TileMode* xy) const {
    SkPoint diff = fCenter2 - fCenter1;
    SkScalar diffLen = 0;

    if (bitmap) {
        this->getGradientTableBitmap(bitmap);
    }
    if (matrix) {
        diffLen = diff.length();
    }
    if (matrix) {
        if (diffLen) {
            SkScalar invDiffLen = SkScalarInvert(diffLen);
            
            matrix->setSinCos(-SkScalarMul(invDiffLen, diff.fY),
                              SkScalarMul(invDiffLen, diff.fX));
        } else {
            matrix->reset();
        }
        matrix->preTranslate(-fCenter1.fX, -fCenter1.fY);
    }
    if (xy) {
        xy[0] = fTileMode;
        xy[1] = kClamp_TileMode;
    }
    return kTwoPointConical_BitmapType;
}

SkShader::GradientType SkTwoPointConicalGradient::asAGradient(
    GradientInfo* info) const {
    if (info) {
        commonAsAGradient(info);
        info->fPoint[0] = fCenter1;
        info->fPoint[1] = fCenter2;
        info->fRadius[0] = fRadius1;
        info->fRadius[1] = fRadius2;
    }
    return kConical_GradientType;
}

SkTwoPointConicalGradient::SkTwoPointConicalGradient(
    SkFlattenableReadBuffer& buffer)
    : INHERITED(buffer),
    fCenter1(buffer.readPoint()),
    fCenter2(buffer.readPoint()),
    fRadius1(buffer.readScalar()),
    fRadius2(buffer.readScalar()) {
    this->init();
};

void SkTwoPointConicalGradient::flatten(
    SkFlattenableWriteBuffer& buffer) const {
    this->INHERITED::flatten(buffer);
    buffer.writePoint(fCenter1);
    buffer.writePoint(fCenter2);
    buffer.writeScalar(fRadius1);
    buffer.writeScalar(fRadius2);
}



#if SK_SUPPORT_GPU

#include "GrTBackendEffectFactory.h"


typedef GrGLUniformManager::UniformHandle UniformHandle;
static const UniformHandle kInvalidUniformHandle = GrGLUniformManager::kInvalidUniformHandle;

class GrGLConical2Gradient : public GrGLGradientEffect {
public:

    GrGLConical2Gradient(const GrBackendEffectFactory& factory, const GrDrawEffect&);
    virtual ~GrGLConical2Gradient() { }

    virtual void emitCode(GrGLShaderBuilder*,
                          const GrDrawEffect&,
                          EffectKey,
                          const char* outputColor,
                          const char* inputColor,
                          const TextureSamplerArray&) SK_OVERRIDE;
    virtual void setData(const GrGLUniformManager&, const GrDrawEffect&) SK_OVERRIDE;

    static EffectKey GenKey(const GrDrawEffect&, const GrGLCaps& caps);

protected:

    UniformHandle           fVSParamUni;
    UniformHandle           fFSParamUni;

    const char* fVSVaryingName;
    const char* fFSVaryingName;

    bool fIsDegenerate;

    
    

    SkScalar fCachedCenter;
    SkScalar fCachedRadius;
    SkScalar fCachedDiffRadius;

    

private:

    typedef GrGLGradientEffect INHERITED;

};



class GrConical2Gradient : public GrGradientEffect {
public:

    static GrEffectRef* Create(GrContext* ctx,
                               const SkTwoPointConicalGradient& shader,
                               const SkMatrix& matrix,
                               SkShader::TileMode tm) {
        AutoEffectUnref effect(SkNEW_ARGS(GrConical2Gradient, (ctx, shader, matrix, tm)));
        return CreateEffectRef(effect);
    }

    virtual ~GrConical2Gradient() { }

    static const char* Name() { return "Two-Point Conical Gradient"; }
    virtual const GrBackendEffectFactory& getFactory() const SK_OVERRIDE {
        return GrTBackendEffectFactory<GrConical2Gradient>::getInstance();
    }

    
    bool isDegenerate() const { return SkScalarAbs(fDiffRadius) == SkScalarAbs(fCenterX1); }
    SkScalar center() const { return fCenterX1; }
    SkScalar diffRadius() const { return fDiffRadius; }
    SkScalar radius() const { return fRadius0; }

    typedef GrGLConical2Gradient GLEffect;

private:
    virtual bool onIsEqual(const GrEffect& sBase) const SK_OVERRIDE {
        const GrConical2Gradient& s = CastEffect<GrConical2Gradient>(sBase);
        return (INHERITED::onIsEqual(sBase) &&
                this->fCenterX1 == s.fCenterX1 &&
                this->fRadius0 == s.fRadius0 &&
                this->fDiffRadius == s.fDiffRadius);
    }

    GrConical2Gradient(GrContext* ctx,
                       const SkTwoPointConicalGradient& shader,
                       const SkMatrix& matrix,
                       SkShader::TileMode tm)
        : INHERITED(ctx, shader, matrix, tm)
        , fCenterX1(shader.getCenterX1())
        , fRadius0(shader.getStartRadius())
        , fDiffRadius(shader.getDiffRadius()) { }

    GR_DECLARE_EFFECT_TEST;

    
    
    

    SkScalar fCenterX1;
    SkScalar fRadius0;
    SkScalar fDiffRadius;

    

    typedef GrGradientEffect INHERITED;
};

GR_DEFINE_EFFECT_TEST(GrConical2Gradient);

GrEffectRef* GrConical2Gradient::TestCreate(SkMWCRandom* random,
                                            GrContext* context,
                                            const GrDrawTargetCaps&,
                                            GrTexture**) {
    SkPoint center1 = {random->nextUScalar1(), random->nextUScalar1()};
    SkScalar radius1 = random->nextUScalar1();
    SkPoint center2;
    SkScalar radius2;
    do {
        center2.set(random->nextUScalar1(), random->nextUScalar1());
        radius2 = random->nextUScalar1 ();
        
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
    return shader->asNewEffect(context, paint);
}




GrGLConical2Gradient::GrGLConical2Gradient(const GrBackendEffectFactory& factory,
                                           const GrDrawEffect& drawEffect)
    : INHERITED(factory)
    , fVSParamUni(kInvalidUniformHandle)
    , fFSParamUni(kInvalidUniformHandle)
    , fVSVaryingName(NULL)
    , fFSVaryingName(NULL)
    , fCachedCenter(SK_ScalarMax)
    , fCachedRadius(-SK_ScalarMax)
    , fCachedDiffRadius(-SK_ScalarMax) {

    const GrConical2Gradient& data = drawEffect.castEffect<GrConical2Gradient>();
    fIsDegenerate = data.isDegenerate();
}

void GrGLConical2Gradient::emitCode(GrGLShaderBuilder* builder,
                                    const GrDrawEffect&,
                                    EffectKey key,
                                    const char* outputColor,
                                    const char* inputColor,
                                    const TextureSamplerArray& samplers) {
    const char* fsCoords;
    const char* vsCoordsVarying;
    GrSLType coordsVaryingType;
    this->setupMatrix(builder, key, &fsCoords, &vsCoordsVarying, &coordsVaryingType);

    this->emitYCoordUniform(builder);
    
    
    
    fVSParamUni = builder->addUniformArray(GrGLShaderBuilder::kVertex_ShaderType,
                                           kFloat_GrSLType, "Conical2VSParams", 6);
    fFSParamUni = builder->addUniformArray(GrGLShaderBuilder::kFragment_ShaderType,
                                           kFloat_GrSLType, "Conical2FSParams", 6);

    
    
    if (kVec2f_GrSLType == coordsVaryingType) {
        builder->addVarying(kFloat_GrSLType, "Conical2BCoeff",
                            &fVSVaryingName, &fFSVaryingName);
    }

    
    {
        SkString p2; 
        SkString p3; 
        SkString p5; 
        builder->getUniformVariable(fVSParamUni).appendArrayAccess(2, &p2);
        builder->getUniformVariable(fVSParamUni).appendArrayAccess(3, &p3);
        builder->getUniformVariable(fVSParamUni).appendArrayAccess(5, &p5);

        
        
        if (kVec2f_GrSLType == coordsVaryingType) {
            
            builder->vsCodeAppendf("\t%s = -2.0 * (%s * %s.x + %s * %s);\n",
                                   fVSVaryingName, p2.c_str(),
                                   vsCoordsVarying, p3.c_str(), p5.c_str());
        }
    }

    
    {

        SkString cName("c");
        SkString ac4Name("ac4");
        SkString dName("d");
        SkString qName("q");
        SkString r0Name("r0");
        SkString r1Name("r1");
        SkString tName("t");
        SkString p0; 
        SkString p1; 
        SkString p2; 
        SkString p3; 
        SkString p4; 
        SkString p5; 

        builder->getUniformVariable(fFSParamUni).appendArrayAccess(0, &p0);
        builder->getUniformVariable(fFSParamUni).appendArrayAccess(1, &p1);
        builder->getUniformVariable(fFSParamUni).appendArrayAccess(2, &p2);
        builder->getUniformVariable(fFSParamUni).appendArrayAccess(3, &p3);
        builder->getUniformVariable(fFSParamUni).appendArrayAccess(4, &p4);
        builder->getUniformVariable(fFSParamUni).appendArrayAccess(5, &p5);

        
        
        SkString bVar;
        if (kVec2f_GrSLType == coordsVaryingType) {
            bVar = fFSVaryingName;
        } else {
            bVar = "b";
            builder->fsCodeAppendf("\tfloat %s = -2.0 * (%s * %s.x + %s * %s);\n",
                                   bVar.c_str(), p2.c_str(), fsCoords,
                                   p3.c_str(), p5.c_str());
        }

        
        
        builder->fsCodeAppendf("\t%s = vec4(0.0,0.0,0.0,0.0);\n", outputColor);

        
        builder->fsCodeAppendf("\tfloat %s = dot(%s, %s) - %s;\n", cName.c_str(),
                               fsCoords, fsCoords,
                               p4.c_str());

        
        if (!fIsDegenerate) {

            
            builder->fsCodeAppendf("\tfloat %s = %s * %s;\n", ac4Name.c_str(), p0.c_str(),
                                   cName.c_str());

            
            builder->fsCodeAppendf("\tfloat %s = %s * %s - %s;\n", dName.c_str(),
                                   bVar.c_str(), bVar.c_str(), ac4Name.c_str());

            
            builder->fsCodeAppendf("\tif (%s >= 0.0) {\n", dName.c_str());

            
            
            builder->fsCodeAppendf("\t\tfloat %s = -0.5 * (%s + (%s < 0.0 ? -1.0 : 1.0)"
                                   " * sqrt(%s));\n", qName.c_str(), bVar.c_str(),
                                   bVar.c_str(), dName.c_str());

            
            
            builder->fsCodeAppendf("\t\tfloat %s = %s * %s;\n", r0Name.c_str(),
                                   qName.c_str(), p1.c_str());
            
            builder->fsCodeAppendf("\t\tfloat %s = %s / %s;\n", r1Name.c_str(),
                                   cName.c_str(), qName.c_str());

            
            

            
            builder->fsCodeAppendf("\t\tfloat %s = max(%s, %s);\n", tName.c_str(),
                                   r0Name.c_str(), r1Name.c_str());

            
            builder->fsCodeAppendf("\t\tif (%s * %s + %s > 0.0) {\n", tName.c_str(),
                                   p5.c_str(), p3.c_str());

            builder->fsCodeAppend("\t\t");
            this->emitColorLookup(builder, tName.c_str(), outputColor, inputColor, samplers[0]);

            
            builder->fsCodeAppend("\t\t} else {\n");
            builder->fsCodeAppendf("\t\t\t%s = min(%s, %s);\n", tName.c_str(),
                                   r0Name.c_str(), r1Name.c_str());

            
            builder->fsCodeAppendf("\t\t\tif (%s * %s + %s > 0.0) {\n",
                                   tName.c_str(), p5.c_str(), p3.c_str());

            builder->fsCodeAppend("\t\t\t");
            this->emitColorLookup(builder, tName.c_str(), outputColor, inputColor, samplers[0]);

            
            builder->fsCodeAppend("\t\t\t}\n");
            
            builder->fsCodeAppend("\t\t}\n");
            
            builder->fsCodeAppend("\t}\n");
        } else {

            
            builder->fsCodeAppendf("\tfloat %s = -(%s / %s);\n", tName.c_str(),
                                   cName.c_str(), bVar.c_str());

            
            builder->fsCodeAppendf("\tif (%s * %s + %s > 0.0) {\n", tName.c_str(),
                                   p5.c_str(), p3.c_str());
            builder->fsCodeAppend("\t");
            this->emitColorLookup(builder, tName.c_str(), outputColor, inputColor, samplers[0]);
            builder->fsCodeAppend("\t}\n");
        }
    }
}

void GrGLConical2Gradient::setData(const GrGLUniformManager& uman,
                                   const GrDrawEffect& drawEffect) {
    INHERITED::setData(uman, drawEffect);
    const GrConical2Gradient& data = drawEffect.castEffect<GrConical2Gradient>();
    GrAssert(data.isDegenerate() == fIsDegenerate);
    SkScalar centerX1 = data.center();
    SkScalar radius0 = data.radius();
    SkScalar diffRadius = data.diffRadius();

    if (fCachedCenter != centerX1 ||
        fCachedRadius != radius0 ||
        fCachedDiffRadius != diffRadius) {

        SkScalar a = SkScalarMul(centerX1, centerX1) - diffRadius * diffRadius;

        
        
        
        
        
        float values[6] = {
            SkScalarToFloat(a * 4),
            1.f / (SkScalarToFloat(a)),
            SkScalarToFloat(centerX1),
            SkScalarToFloat(radius0),
            SkScalarToFloat(SkScalarMul(radius0, radius0)),
            SkScalarToFloat(diffRadius)
        };

        uman.set1fv(fVSParamUni, 0, 6, values);
        uman.set1fv(fFSParamUni, 0, 6, values);
        fCachedCenter = centerX1;
        fCachedRadius = radius0;
        fCachedDiffRadius = diffRadius;
    }
}

GrGLEffect::EffectKey GrGLConical2Gradient::GenKey(const GrDrawEffect& drawEffect,
                                                   const GrGLCaps&) {
    enum {
        kIsDegenerate = 1 << kMatrixKeyBitCnt,
    };

    EffectKey key = GenMatrixKey(drawEffect);
    if (drawEffect.castEffect<GrConical2Gradient>().isDegenerate()) {
        key |= kIsDegenerate;
    }
    return key;
}



GrEffectRef* SkTwoPointConicalGradient::asNewEffect(GrContext* context, const SkPaint&) const {
    SkASSERT(NULL != context);
    SkASSERT(fPtsToUnit.isIdentity());
    
    SkMatrix matrix;
    if (!this->getLocalMatrix().invert(&matrix)) {
        return NULL;
    }
    matrix.postTranslate(-fCenter1.fX, -fCenter1.fY);

    SkPoint diff = fCenter2 - fCenter1;
    SkScalar diffLen = diff.length();
    if (0 != diffLen) {
        SkScalar invDiffLen = SkScalarInvert(diffLen);
        SkMatrix rot;
        rot.setSinCos(-SkScalarMul(invDiffLen, diff.fY),
                       SkScalarMul(invDiffLen, diff.fX));
        matrix.postConcat(rot);
    }

    return GrConical2Gradient::Create(context, *this, matrix, fTileMode);
}

#else

GrEffectRef* SkTwoPointConicalGradient::asNewEffect(GrContext*, const SkPaint&) const {
    SkDEBUGFAIL("Should not call in GPU-less build");
    return NULL;
}

#endif

#ifdef SK_DEVELOPER
void SkTwoPointConicalGradient::toString(SkString* str) const {
    str->append("SkTwoPointConicalGradient: (");

    str->append("center1: (");
    str->appendScalar(fCenter1.fX);
    str->append(", ");
    str->appendScalar(fCenter1.fY);
    str->append(") radius1: ");
    str->appendScalar(fRadius1);
    str->append(" ");

    str->append("center2: (");
    str->appendScalar(fCenter2.fX);
    str->append(", ");
    str->appendScalar(fCenter2.fY);
    str->append(") radius2: ");
    str->appendScalar(fRadius2);
    str->append(" ");

    this->INHERITED::toString(str);

    str->append(")");
}
#endif
