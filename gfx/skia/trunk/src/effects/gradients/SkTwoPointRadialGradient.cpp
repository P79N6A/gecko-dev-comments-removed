







#include "SkTwoPointRadialGradient.h"









































































namespace {

inline SkFixed two_point_radial(SkScalar b, SkScalar fx, SkScalar fy,
                                SkScalar sr2d2, SkScalar foura,
                                SkScalar oneOverTwoA, bool posRoot) {
    SkScalar c = SkScalarSquare(fx) + SkScalarSquare(fy) - sr2d2;
    if (0 == foura) {
        return SkScalarToFixed(SkScalarDiv(-c, b));
    }

    SkScalar discrim = SkScalarSquare(b) - SkScalarMul(foura, c);
    if (discrim < 0) {
        discrim = -discrim;
    }
    SkScalar rootDiscrim = SkScalarSqrt(discrim);
    SkScalar result;
    if (posRoot) {
        result = SkScalarMul(-b + rootDiscrim, oneOverTwoA);
    } else {
        result = SkScalarMul(-b - rootDiscrim, oneOverTwoA);
    }
    return SkScalarToFixed(result);
}

typedef void (* TwoPointRadialShadeProc)(SkScalar fx, SkScalar dx,
        SkScalar fy, SkScalar dy,
        SkScalar b, SkScalar db,
        SkScalar fSr2D2, SkScalar foura, SkScalar fOneOverTwoA, bool posRoot,
        SkPMColor* SK_RESTRICT dstC, const SkPMColor* SK_RESTRICT cache,
        int count);

void shadeSpan_twopoint_clamp(SkScalar fx, SkScalar dx,
        SkScalar fy, SkScalar dy,
        SkScalar b, SkScalar db,
        SkScalar fSr2D2, SkScalar foura, SkScalar fOneOverTwoA, bool posRoot,
        SkPMColor* SK_RESTRICT dstC, const SkPMColor* SK_RESTRICT cache,
        int count) {
    for (; count > 0; --count) {
        SkFixed t = two_point_radial(b, fx, fy, fSr2D2, foura,
                                     fOneOverTwoA, posRoot);
        SkFixed index = SkClampMax(t, 0xFFFF);
        SkASSERT(index <= 0xFFFF);
        *dstC++ = cache[index >> SkGradientShaderBase::kCache32Shift];
        fx += dx;
        fy += dy;
        b += db;
    }
}
void shadeSpan_twopoint_mirror(SkScalar fx, SkScalar dx,
        SkScalar fy, SkScalar dy,
        SkScalar b, SkScalar db,
        SkScalar fSr2D2, SkScalar foura, SkScalar fOneOverTwoA, bool posRoot,
        SkPMColor* SK_RESTRICT dstC, const SkPMColor* SK_RESTRICT cache,
        int count) {
    for (; count > 0; --count) {
        SkFixed t = two_point_radial(b, fx, fy, fSr2D2, foura,
                                     fOneOverTwoA, posRoot);
        SkFixed index = mirror_tileproc(t);
        SkASSERT(index <= 0xFFFF);
        *dstC++ = cache[index >> SkGradientShaderBase::kCache32Shift];
        fx += dx;
        fy += dy;
        b += db;
    }
}

void shadeSpan_twopoint_repeat(SkScalar fx, SkScalar dx,
        SkScalar fy, SkScalar dy,
        SkScalar b, SkScalar db,
        SkScalar fSr2D2, SkScalar foura, SkScalar fOneOverTwoA, bool posRoot,
        SkPMColor* SK_RESTRICT dstC, const SkPMColor* SK_RESTRICT cache,
        int count) {
    for (; count > 0; --count) {
        SkFixed t = two_point_radial(b, fx, fy, fSr2D2, foura,
                                     fOneOverTwoA, posRoot);
        SkFixed index = repeat_tileproc(t);
        SkASSERT(index <= 0xFFFF);
        *dstC++ = cache[index >> SkGradientShaderBase::kCache32Shift];
        fx += dx;
        fy += dy;
        b += db;
    }
}
}



SkTwoPointRadialGradient::SkTwoPointRadialGradient(
    const SkPoint& start, SkScalar startRadius,
    const SkPoint& end, SkScalar endRadius,
    const Descriptor& desc, const SkMatrix* localMatrix)
    : SkGradientShaderBase(desc, localMatrix),
      fCenter1(start),
      fCenter2(end),
      fRadius1(startRadius),
      fRadius2(endRadius) {
    init();
}

SkShader::BitmapType SkTwoPointRadialGradient::asABitmap(
    SkBitmap* bitmap,
    SkMatrix* matrix,
    SkShader::TileMode* xy) const {
    if (bitmap) {
        this->getGradientTableBitmap(bitmap);
    }
    SkScalar diffL = 0; 
    if (matrix) {
        diffL = SkScalarSqrt(SkScalarSquare(fDiff.fX) +
                             SkScalarSquare(fDiff.fY));
    }
    if (matrix) {
        if (diffL) {
            SkScalar invDiffL = SkScalarInvert(diffL);
            matrix->setSinCos(-SkScalarMul(invDiffL, fDiff.fY),
                              SkScalarMul(invDiffL, fDiff.fX));
        } else {
            matrix->reset();
        }
        matrix->preConcat(fPtsToUnit);
    }
    if (xy) {
        xy[0] = fTileMode;
        xy[1] = kClamp_TileMode;
    }
    return kTwoPointRadial_BitmapType;
}

SkShader::GradientType SkTwoPointRadialGradient::asAGradient(
    SkShader::GradientInfo* info) const {
    if (info) {
        commonAsAGradient(info);
        info->fPoint[0] = fCenter1;
        info->fPoint[1] = fCenter2;
        info->fRadius[0] = fRadius1;
        info->fRadius[1] = fRadius2;
    }
    return kRadial2_GradientType;
}

size_t SkTwoPointRadialGradient::contextSize() const {
    return sizeof(TwoPointRadialGradientContext);
}

SkShader::Context* SkTwoPointRadialGradient::onCreateContext(const ContextRec& rec,
                                                             void* storage) const {
    
    if (0 == fDiffRadius) {
        return NULL;
    }
    return SkNEW_PLACEMENT_ARGS(storage, TwoPointRadialGradientContext, (*this, rec));
}

SkTwoPointRadialGradient::TwoPointRadialGradientContext::TwoPointRadialGradientContext(
        const SkTwoPointRadialGradient& shader, const ContextRec& rec)
    : INHERITED(shader, rec)
{
    
    fFlags &= ~kHasSpan16_Flag;
}

void SkTwoPointRadialGradient::TwoPointRadialGradientContext::shadeSpan(
        int x, int y, SkPMColor* dstCParam, int count) {
    SkASSERT(count > 0);

    const SkTwoPointRadialGradient& twoPointRadialGradient =
            static_cast<const SkTwoPointRadialGradient&>(fShader);

    SkPMColor* SK_RESTRICT dstC = dstCParam;

    
    if (twoPointRadialGradient.fDiffRadius == 0) {
      sk_bzero(dstC, count * sizeof(*dstC));
      return;
    }
    SkMatrix::MapXYProc dstProc = fDstToIndexProc;
    TileProc            proc = twoPointRadialGradient.fTileProc;
    const SkPMColor* SK_RESTRICT cache = fCache->getCache32();

    SkScalar foura = twoPointRadialGradient.fA * 4;
    bool posRoot = twoPointRadialGradient.fDiffRadius < 0;
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
        SkScalar b = (SkScalarMul(twoPointRadialGradient.fDiff.fX, fx) +
                     SkScalarMul(twoPointRadialGradient.fDiff.fY, fy) -
                     twoPointRadialGradient.fStartRadius) * 2;
        SkScalar db = (SkScalarMul(twoPointRadialGradient.fDiff.fX, dx) +
                      SkScalarMul(twoPointRadialGradient.fDiff.fY, dy)) * 2;

        TwoPointRadialShadeProc shadeProc = shadeSpan_twopoint_repeat;
        if (SkShader::kClamp_TileMode == twoPointRadialGradient.fTileMode) {
            shadeProc = shadeSpan_twopoint_clamp;
        } else if (SkShader::kMirror_TileMode == twoPointRadialGradient.fTileMode) {
            shadeProc = shadeSpan_twopoint_mirror;
        } else {
            SkASSERT(SkShader::kRepeat_TileMode == twoPointRadialGradient.fTileMode);
        }
        (*shadeProc)(fx, dx, fy, dy, b, db,
                     twoPointRadialGradient.fSr2D2, foura,
                     twoPointRadialGradient.fOneOverTwoA, posRoot,
                     dstC, cache, count);
    } else {    
        SkScalar dstX = SkIntToScalar(x);
        SkScalar dstY = SkIntToScalar(y);
        for (; count > 0; --count) {
            SkPoint             srcPt;
            dstProc(fDstToIndex, dstX, dstY, &srcPt);
            SkScalar fx = srcPt.fX;
            SkScalar fy = srcPt.fY;
            SkScalar b = (SkScalarMul(twoPointRadialGradient.fDiff.fX, fx) +
                         SkScalarMul(twoPointRadialGradient.fDiff.fY, fy) -
                         twoPointRadialGradient.fStartRadius) * 2;
            SkFixed t = two_point_radial(b, fx, fy, twoPointRadialGradient.fSr2D2, foura,
                                         twoPointRadialGradient.fOneOverTwoA, posRoot);
            SkFixed index = proc(t);
            SkASSERT(index <= 0xFFFF);
            *dstC++ = cache[index >> SkGradientShaderBase::kCache32Shift];
            dstX += SK_Scalar1;
        }
    }
}

#ifndef SK_IGNORE_TO_STRING
void SkTwoPointRadialGradient::toString(SkString* str) const {
    str->append("SkTwoPointRadialGradient: (");

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

SkTwoPointRadialGradient::SkTwoPointRadialGradient(
    SkReadBuffer& buffer)
    : INHERITED(buffer),
      fCenter1(buffer.readPoint()),
      fCenter2(buffer.readPoint()),
      fRadius1(buffer.readScalar()),
      fRadius2(buffer.readScalar()) {
    init();
};

void SkTwoPointRadialGradient::flatten(
    SkWriteBuffer& buffer) const {
    this->INHERITED::flatten(buffer);
    buffer.writePoint(fCenter1);
    buffer.writePoint(fCenter2);
    buffer.writeScalar(fRadius1);
    buffer.writeScalar(fRadius2);
}

void SkTwoPointRadialGradient::init() {
    fDiff = fCenter1 - fCenter2;
    fDiffRadius = fRadius2 - fRadius1;
    
    SkScalar inv = fDiffRadius ? SkScalarInvert(fDiffRadius) : 0;
    fDiff.fX = SkScalarMul(fDiff.fX, inv);
    fDiff.fY = SkScalarMul(fDiff.fY, inv);
    fStartRadius = SkScalarMul(fRadius1, inv);
    fSr2D2 = SkScalarSquare(fStartRadius);
    fA = SkScalarSquare(fDiff.fX) + SkScalarSquare(fDiff.fY) - SK_Scalar1;
    fOneOverTwoA = fA ? SkScalarInvert(fA * 2) : 0;

    fPtsToUnit.setTranslate(-fCenter1.fX, -fCenter1.fY);
    fPtsToUnit.postScale(inv, inv);
}



#if SK_SUPPORT_GPU

#include "GrTBackendEffectFactory.h"
#include "gl/GrGLShaderBuilder.h"
#include "SkGr.h"


typedef GrGLUniformManager::UniformHandle UniformHandle;

class GrGLRadial2Gradient : public GrGLGradientEffect {

public:

    GrGLRadial2Gradient(const GrBackendEffectFactory& factory, const GrDrawEffect&);
    virtual ~GrGLRadial2Gradient() { }

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

    bool fIsDegenerate;

    
    

    SkScalar fCachedCenter;
    SkScalar fCachedRadius;
    bool     fCachedPosRoot;

    

private:

    typedef GrGLGradientEffect INHERITED;

};



class GrRadial2Gradient : public GrGradientEffect {
public:
    static GrEffect* Create(GrContext* ctx,
                            const SkTwoPointRadialGradient& shader,
                            const SkMatrix& matrix,
                            SkShader::TileMode tm) {
        return SkNEW_ARGS(GrRadial2Gradient, (ctx, shader, matrix, tm));
    }

    virtual ~GrRadial2Gradient() { }

    static const char* Name() { return "Two-Point Radial Gradient"; }
    virtual const GrBackendEffectFactory& getFactory() const SK_OVERRIDE {
        return GrTBackendEffectFactory<GrRadial2Gradient>::getInstance();
    }

    
    bool isDegenerate() const { return SK_Scalar1 == fCenterX1; }
    SkScalar center() const { return fCenterX1; }
    SkScalar radius() const { return fRadius0; }
    bool isPosRoot() const { return SkToBool(fPosRoot); }

    typedef GrGLRadial2Gradient GLEffect;

private:
    virtual bool onIsEqual(const GrEffect& sBase) const SK_OVERRIDE {
        const GrRadial2Gradient& s = CastEffect<GrRadial2Gradient>(sBase);
        return (INHERITED::onIsEqual(sBase) &&
                this->fCenterX1 == s.fCenterX1 &&
                this->fRadius0 == s.fRadius0 &&
                this->fPosRoot == s.fPosRoot);
    }

    GrRadial2Gradient(GrContext* ctx,
                      const SkTwoPointRadialGradient& shader,
                      const SkMatrix& matrix,
                      SkShader::TileMode tm)
        : INHERITED(ctx, shader, matrix, tm)
        , fCenterX1(shader.getCenterX1())
        , fRadius0(shader.getStartRadius())
        , fPosRoot(shader.getDiffRadius() < 0) {
        
        
        fBTransform = this->getCoordTransform();
        SkMatrix& bMatrix = *fBTransform.accessMatrix();
        bMatrix[SkMatrix::kMScaleX] = 2 * (SkScalarMul(fCenterX1, bMatrix[SkMatrix::kMScaleX]) -
                                           SkScalarMul(fRadius0, bMatrix[SkMatrix::kMPersp0]));
        bMatrix[SkMatrix::kMSkewX] = 2 * (SkScalarMul(fCenterX1, bMatrix[SkMatrix::kMSkewX]) -
                                          SkScalarMul(fRadius0, bMatrix[SkMatrix::kMPersp1]));
        bMatrix[SkMatrix::kMTransX] = 2 * (SkScalarMul(fCenterX1, bMatrix[SkMatrix::kMTransX]) -
                                           SkScalarMul(fRadius0, bMatrix[SkMatrix::kMPersp2]));
        this->addCoordTransform(&fBTransform);
    }

    GR_DECLARE_EFFECT_TEST;

    
    
    

    GrCoordTransform fBTransform;
    SkScalar         fCenterX1;
    SkScalar         fRadius0;
    SkBool8          fPosRoot;

    

    typedef GrGradientEffect INHERITED;
};



GR_DEFINE_EFFECT_TEST(GrRadial2Gradient);

GrEffect* GrRadial2Gradient::TestCreate(SkRandom* random,
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
        
    } while (radius1 == radius2);

    SkColor colors[kMaxRandomGradientColors];
    SkScalar stopsArray[kMaxRandomGradientColors];
    SkScalar* stops = stopsArray;
    SkShader::TileMode tm;
    int colorCount = RandomGradientParams(random, colors, &stops, &tm);
    SkAutoTUnref<SkShader> shader(SkGradientShader::CreateTwoPointRadial(center1, radius1,
                                                                         center2, radius2,
                                                                         colors, stops, colorCount,
                                                                         tm));
    SkPaint paint;
    GrEffect* effect;
    GrColor paintColor;
    SkAssertResult(shader->asNewEffect(context, paint, NULL, &paintColor, &effect));
    return effect;
}



GrGLRadial2Gradient::GrGLRadial2Gradient(const GrBackendEffectFactory& factory,
                                         const GrDrawEffect& drawEffect)
    : INHERITED(factory)
    , fVSVaryingName(NULL)
    , fFSVaryingName(NULL)
    , fCachedCenter(SK_ScalarMax)
    , fCachedRadius(-SK_ScalarMax)
    , fCachedPosRoot(0) {

    const GrRadial2Gradient& data = drawEffect.castEffect<GrRadial2Gradient>();
    fIsDegenerate = data.isDegenerate();
}

void GrGLRadial2Gradient::emitCode(GrGLShaderBuilder* builder,
                                   const GrDrawEffect& drawEffect,
                                   const GrEffectKey& key,
                                   const char* outputColor,
                                   const char* inputColor,
                                   const TransformedCoordsArray& coords,
                                   const TextureSamplerArray& samplers) {
    uint32_t baseKey = key.get32(0);
    this->emitUniforms(builder, baseKey);
    fParamUni = builder->addUniformArray(GrGLShaderBuilder::kFragment_Visibility,
                                         kFloat_GrSLType, "Radial2FSParams", 6);

    SkString cName("c");
    SkString ac4Name("ac4");
    SkString rootName("root");
    SkString t;
    SkString p0;
    SkString p1;
    SkString p2;
    SkString p3;
    SkString p4;
    SkString p5;
    builder->getUniformVariable(fParamUni).appendArrayAccess(0, &p0);
    builder->getUniformVariable(fParamUni).appendArrayAccess(1, &p1);
    builder->getUniformVariable(fParamUni).appendArrayAccess(2, &p2);
    builder->getUniformVariable(fParamUni).appendArrayAccess(3, &p3);
    builder->getUniformVariable(fParamUni).appendArrayAccess(4, &p4);
    builder->getUniformVariable(fParamUni).appendArrayAccess(5, &p5);

    
    SkASSERT(coords[0].type() == coords[1].type());
    const char* coords2D;
    SkString bVar;
    if (kVec3f_GrSLType == coords[0].type()) {
        builder->fsCodeAppendf("\tvec3 interpolants = vec3(%s.xy, %s.x) / %s.z;\n",
                               coords[0].c_str(), coords[1].c_str(), coords[0].c_str());
        coords2D = "interpolants.xy";
        bVar = "interpolants.z";
    } else {
        coords2D = coords[0].c_str();
        bVar.printf("%s.x", coords[1].c_str());
    }

    
    builder->fsCodeAppendf("\tfloat %s = dot(%s, %s) - %s;\n",
                           cName.c_str(), coords2D, coords2D, p4.c_str());

    
    
    if (!fIsDegenerate) {

        
        builder->fsCodeAppendf("\tfloat %s = %s * 4.0 * %s;\n",
                               ac4Name.c_str(), p0.c_str(),
                               cName.c_str());

        
        
        builder->fsCodeAppendf("\tfloat %s = sqrt(abs(%s*%s - %s));\n",
                               rootName.c_str(), bVar.c_str(), bVar.c_str(),
                               ac4Name.c_str());

        
        t.printf("(-%s + %s * %s) * %s", bVar.c_str(), p5.c_str(),
                 rootName.c_str(), p1.c_str());
    } else {
        
        t.printf("-%s / %s", cName.c_str(), bVar.c_str());
    }

    this->emitColor(builder, t.c_str(), baseKey, outputColor, inputColor, samplers);
}

void GrGLRadial2Gradient::setData(const GrGLUniformManager& uman,
                                  const GrDrawEffect& drawEffect) {
    INHERITED::setData(uman, drawEffect);
    const GrRadial2Gradient& data = drawEffect.castEffect<GrRadial2Gradient>();
    SkASSERT(data.isDegenerate() == fIsDegenerate);
    SkScalar centerX1 = data.center();
    SkScalar radius0 = data.radius();
    if (fCachedCenter != centerX1 ||
        fCachedRadius != radius0 ||
        fCachedPosRoot != data.isPosRoot()) {

        SkScalar a = SkScalarMul(centerX1, centerX1) - SK_Scalar1;

        
        
        
        
        
        float values[6] = {
            SkScalarToFloat(a),
            1 / (2.f * SkScalarToFloat(a)),
            SkScalarToFloat(centerX1),
            SkScalarToFloat(radius0),
            SkScalarToFloat(SkScalarMul(radius0, radius0)),
            data.isPosRoot() ? 1.f : -1.f
        };

        uman.set1fv(fParamUni, 6, values);
        fCachedCenter = centerX1;
        fCachedRadius = radius0;
        fCachedPosRoot = data.isPosRoot();
    }
}

void GrGLRadial2Gradient::GenKey(const GrDrawEffect& drawEffect,
                                 const GrGLCaps&, GrEffectKeyBuilder* b) {
    uint32_t* key = b->add32n(2);
    key[0] = GenBaseGradientKey(drawEffect);
    key[1] = drawEffect.castEffect<GrRadial2Gradient>().isDegenerate();
}



bool SkTwoPointRadialGradient::asNewEffect(GrContext* context, const SkPaint& paint,
                                           const SkMatrix* localMatrix, GrColor* paintColor,
                                           GrEffect** effect)  const {
    SkASSERT(NULL != context);
    
    
    SkMatrix matrix;
    if (!this->getLocalMatrix().invert(&matrix)) {
        return false;
    }
    if (localMatrix) {
        SkMatrix inv;
        if (!localMatrix->invert(&inv)) {
            return false;
        }
        matrix.postConcat(inv);
    }
    matrix.postConcat(fPtsToUnit);

    SkScalar diffLen = fDiff.length();
    if (0 != diffLen) {
        SkScalar invDiffLen = SkScalarInvert(diffLen);
        SkMatrix rot;
        rot.setSinCos(-SkScalarMul(invDiffLen, fDiff.fY),
                       SkScalarMul(invDiffLen, fDiff.fX));
        matrix.postConcat(rot);
    }

    *paintColor = SkColor2GrColorJustAlpha(paint.getColor());
    *effect = GrRadial2Gradient::Create(context, *this, matrix, fTileMode);
    
    return true;
}

#else

bool SkTwoPointRadialGradient::asNewEffect(GrContext* context, const SkPaint& paint,
                                           const SkMatrix* localMatrix, GrColor* paintColor,
                                           GrEffect** effect)  const {
    SkDEBUGFAIL("Should not call in GPU-less build");
    return false;
}

#endif
