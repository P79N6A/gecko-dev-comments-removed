







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
    const SkColor colors[], const SkScalar pos[],
    int colorCount, SkShader::TileMode mode,
    SkUnitMapper* mapper)
    : SkGradientShaderBase(colors, pos, colorCount, mode, mapper),
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

void SkTwoPointRadialGradient::shadeSpan(int x, int y, SkPMColor* dstCParam,
                                         int count) {
    SkASSERT(count > 0);

    SkPMColor* SK_RESTRICT dstC = dstCParam;

    
    if (fDiffRadius == 0) {
      sk_bzero(dstC, count * sizeof(*dstC));
      return;
    }
    SkMatrix::MapXYProc dstProc = fDstToIndexProc;
    TileProc            proc = fTileProc;
    const SkPMColor* SK_RESTRICT cache = this->getCache32();

    SkScalar foura = fA * 4;
    bool posRoot = fDiffRadius < 0;
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
        SkScalar b = (SkScalarMul(fDiff.fX, fx) +
                     SkScalarMul(fDiff.fY, fy) - fStartRadius) * 2;
        SkScalar db = (SkScalarMul(fDiff.fX, dx) +
                      SkScalarMul(fDiff.fY, dy)) * 2;

        TwoPointRadialShadeProc shadeProc = shadeSpan_twopoint_repeat;
        if (SkShader::kClamp_TileMode == fTileMode) {
            shadeProc = shadeSpan_twopoint_clamp;
        } else if (SkShader::kMirror_TileMode == fTileMode) {
            shadeProc = shadeSpan_twopoint_mirror;
        } else {
            SkASSERT(SkShader::kRepeat_TileMode == fTileMode);
        }
        (*shadeProc)(fx, dx, fy, dy, b, db,
                     fSr2D2, foura, fOneOverTwoA, posRoot,
                     dstC, cache, count);
    } else {    
        SkScalar dstX = SkIntToScalar(x);
        SkScalar dstY = SkIntToScalar(y);
        for (; count > 0; --count) {
            SkPoint             srcPt;
            dstProc(fDstToIndex, dstX, dstY, &srcPt);
            SkScalar fx = srcPt.fX;
            SkScalar fy = srcPt.fY;
            SkScalar b = (SkScalarMul(fDiff.fX, fx) +
                         SkScalarMul(fDiff.fY, fy) - fStartRadius) * 2;
            SkFixed t = two_point_radial(b, fx, fy, fSr2D2, foura,
                                         fOneOverTwoA, posRoot);
            SkFixed index = proc(t);
            SkASSERT(index <= 0xFFFF);
            *dstC++ = cache[index >> SkGradientShaderBase::kCache32Shift];
            dstX += SK_Scalar1;
        }
    }
}

bool SkTwoPointRadialGradient::setContext(
    const SkBitmap& device,
    const SkPaint& paint,
    const SkMatrix& matrix){
    if (!this->INHERITED::setContext(device, paint, matrix)) {
        return false;
    }

    
    if (0 == fDiffRadius) {
        return false;
    }

    
    fFlags &= ~kHasSpan16_Flag;
    return true;
}

SkTwoPointRadialGradient::SkTwoPointRadialGradient(
    SkFlattenableReadBuffer& buffer)
    : INHERITED(buffer),
      fCenter1(buffer.readPoint()),
      fCenter2(buffer.readPoint()),
      fRadius1(buffer.readScalar()),
      fRadius2(buffer.readScalar()) {
    init();
};

void SkTwoPointRadialGradient::flatten(
    SkFlattenableWriteBuffer& buffer) const {
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


typedef GrGLUniformManager::UniformHandle UniformHandle;
static const UniformHandle kInvalidUniformHandle = GrGLUniformManager::kInvalidUniformHandle;

class GrGLRadial2Gradient : public GrGLGradientStage {

public:

    GrGLRadial2Gradient(const GrProgramStageFactory& factory,
                        const GrCustomStage&);
    virtual ~GrGLRadial2Gradient() { }

    virtual void setupVariables(GrGLShaderBuilder* builder) SK_OVERRIDE;
    virtual void emitVS(GrGLShaderBuilder* builder,
                        const char* vertexCoords) SK_OVERRIDE;
    virtual void emitFS(GrGLShaderBuilder* builder,
                        const char* outputColor,
                        const char* inputColor,
                        const TextureSamplerArray&) SK_OVERRIDE;
    virtual void setData(const GrGLUniformManager&,
                         const GrCustomStage&,
                         const GrRenderTarget*,
                         int stageNum) SK_OVERRIDE;

    static StageKey GenKey(const GrCustomStage& s, const GrGLCaps& caps);

protected:

    UniformHandle   fVSParamUni;
    UniformHandle   fFSParamUni;

    const char* fVSVaryingName;
    const char* fFSVaryingName;

    bool fIsDegenerate;

    
    

    GrScalar fCachedCenter;
    GrScalar fCachedRadius;
    bool     fCachedPosRoot;

    

private:

    typedef GrGLGradientStage INHERITED;

};



class GrRadial2Gradient : public GrGradientEffect {
public:

    GrRadial2Gradient(GrContext* ctx, const SkTwoPointRadialGradient& shader,
                      GrSamplerState* sampler)
        : INHERITED(ctx, shader, sampler)
        , fCenterX1(shader.getCenterX1())
        , fRadius0(shader.getStartRadius())
        , fPosRoot(shader.getDiffRadius() < 0) { }
    virtual ~GrRadial2Gradient() { }

    static const char* Name() { return "Two-Point Radial Gradient"; }
    virtual const GrProgramStageFactory& getFactory() const SK_OVERRIDE {
        return GrTProgramStageFactory<GrRadial2Gradient>::getInstance();
    }
    virtual bool isEqual(const GrCustomStage& sBase) const SK_OVERRIDE {
        const GrRadial2Gradient& s = static_cast<const GrRadial2Gradient&>(sBase);
        return (INHERITED::isEqual(sBase) &&
                this->fCenterX1 == s.fCenterX1 &&
                this->fRadius0 == s.fRadius0 &&
                this->fPosRoot == s.fPosRoot);
    }

    
    bool isDegenerate() const { return GR_Scalar1 == fCenterX1; }
    GrScalar center() const { return fCenterX1; }
    GrScalar radius() const { return fRadius0; }
    bool isPosRoot() const { return SkToBool(fPosRoot); }

    typedef GrGLRadial2Gradient GLProgramStage;

private:
    GR_DECLARE_CUSTOM_STAGE_TEST;

    
    
    

    GrScalar fCenterX1;
    GrScalar fRadius0;
    SkBool8  fPosRoot;

    

    typedef GrGradientEffect INHERITED;
};



GR_DEFINE_CUSTOM_STAGE_TEST(GrRadial2Gradient);

GrCustomStage* GrRadial2Gradient::TestCreate(SkRandom* random,
                                             GrContext* context,
                                             GrTexture**) {
    SkPoint center1 = {random->nextUScalar1(), random->nextUScalar1()};
    SkScalar radius1 = random->nextUScalar1();
    SkPoint center2;
    SkScalar radius2;
    do {
        center1.set(random->nextUScalar1(), random->nextUScalar1());
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
    GrSamplerState sampler;
    GrCustomStage* stage = shader->asNewCustomStage(context, &sampler);
    GrAssert(NULL != stage);
    return stage;
}



GrGLRadial2Gradient::GrGLRadial2Gradient(
        const GrProgramStageFactory& factory,
        const GrCustomStage& baseData)
    : INHERITED(factory)
    , fVSParamUni(kInvalidUniformHandle)
    , fFSParamUni(kInvalidUniformHandle)
    , fVSVaryingName(NULL)
    , fFSVaryingName(NULL)
    , fCachedCenter(GR_ScalarMax)
    , fCachedRadius(-GR_ScalarMax)
    , fCachedPosRoot(0) {

    const GrRadial2Gradient& data =
        static_cast<const GrRadial2Gradient&>(baseData);
    fIsDegenerate = data.isDegenerate();
}

void GrGLRadial2Gradient::setupVariables(GrGLShaderBuilder* builder) {
    INHERITED::setupVariables(builder);
    
    
    
    fVSParamUni = builder->addUniformArray(GrGLShaderBuilder::kVertex_ShaderType,
                                           kFloat_GrSLType, "Radial2VSParams", 6);
    fFSParamUni = builder->addUniformArray(GrGLShaderBuilder::kFragment_ShaderType,
                                           kFloat_GrSLType, "Radial2FSParams", 6);

    
    
    if (!builder->defaultTextureMatrixIsPerspective()) {
        builder->addVarying(kFloat_GrSLType, "Radial2BCoeff",
                          &fVSVaryingName, &fFSVaryingName);
    }
}

void GrGLRadial2Gradient::emitVS(GrGLShaderBuilder* builder,
                                 const char* vertexCoords) {
    SkString* code = &builder->fVSCode;
    SkString p2;
    SkString p3;
    builder->getUniformVariable(fVSParamUni).appendArrayAccess(2, &p2);
    builder->getUniformVariable(fVSParamUni).appendArrayAccess(3, &p3);

    
    
    if (!builder->defaultTextureMatrixIsPerspective()) {
        
        code->appendf("\t%s = 2.0 *(%s * %s.x - %s);\n",
                      fVSVaryingName, p2.c_str(),
                      vertexCoords, p3.c_str());
    }
}

void GrGLRadial2Gradient::emitFS(GrGLShaderBuilder* builder,
                                 const char* outputColor,
                                 const char* inputColor,
                                 const TextureSamplerArray& samplers) {
    SkString* code = &builder->fFSCode;
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
    builder->getUniformVariable(fFSParamUni).appendArrayAccess(0, &p0);
    builder->getUniformVariable(fFSParamUni).appendArrayAccess(1, &p1);
    builder->getUniformVariable(fFSParamUni).appendArrayAccess(2, &p2);
    builder->getUniformVariable(fFSParamUni).appendArrayAccess(3, &p3);
    builder->getUniformVariable(fFSParamUni).appendArrayAccess(4, &p4);
    builder->getUniformVariable(fFSParamUni).appendArrayAccess(5, &p5);

    
    
    SkString bVar;
    if (!builder->defaultTextureMatrixIsPerspective()) {
        bVar = fFSVaryingName;
    } else {
        bVar = "b";
        
        code->appendf("\tfloat %s = 2.0 * (%s * %s.x - %s);\n",
                      bVar.c_str(), p2.c_str(),
                      builder->defaultTexCoordsName(), p3.c_str());
    }

    
    code->appendf("\tfloat %s = dot(%s, %s) - %s;\n",
                  cName.c_str(),
                  builder->defaultTexCoordsName(),
                  builder->defaultTexCoordsName(),
                  p4.c_str());

    
    
    if (!fIsDegenerate) {

        
        code->appendf("\tfloat %s = %s * 4.0 * %s;\n",
                      ac4Name.c_str(), p0.c_str(),
                      cName.c_str());

        
        
        code->appendf("\tfloat %s = sqrt(abs(%s*%s - %s));\n",
                      rootName.c_str(), bVar.c_str(), bVar.c_str(),
                      ac4Name.c_str());

        
        t.printf("(-%s + %s * %s) * %s", bVar.c_str(), p5.c_str(),
                 rootName.c_str(), p1.c_str());
    } else {
        
        t.printf("-%s / %s", cName.c_str(), bVar.c_str());
    }

    this->emitColorLookup(builder, t.c_str(), outputColor, inputColor, samplers[0]);
}

void GrGLRadial2Gradient::setData(const GrGLUniformManager& uman,
                                  const GrCustomStage& baseData,
                                  const GrRenderTarget* target,
                                  int stageNum) {
    INHERITED::setData(uman, baseData, target, stageNum);
    const GrRadial2Gradient& data =
        static_cast<const GrRadial2Gradient&>(baseData);
    GrAssert(data.isDegenerate() == fIsDegenerate);
    GrScalar centerX1 = data.center();
    GrScalar radius0 = data.radius();
    if (fCachedCenter != centerX1 ||
        fCachedRadius != radius0 ||
        fCachedPosRoot != data.isPosRoot()) {

        GrScalar a = GrMul(centerX1, centerX1) - GR_Scalar1;

        
        
        
        
        
        float values[6] = {
            GrScalarToFloat(a),
            1 / (2.f * GrScalarToFloat(a)),
            GrScalarToFloat(centerX1),
            GrScalarToFloat(radius0),
            GrScalarToFloat(GrMul(radius0, radius0)),
            data.isPosRoot() ? 1.f : -1.f
        };

        uman.set1fv(fVSParamUni, 0, 6, values);
        uman.set1fv(fFSParamUni, 0, 6, values);
        fCachedCenter = centerX1;
        fCachedRadius = radius0;
        fCachedPosRoot = data.isPosRoot();
    }
}

GrCustomStage::StageKey GrGLRadial2Gradient::GenKey(const GrCustomStage& s, const GrGLCaps& caps) {
    return (static_cast<const GrRadial2Gradient&>(s).isDegenerate());
}



GrCustomStage* SkTwoPointRadialGradient::asNewCustomStage(
    GrContext* context, GrSamplerState* sampler) const {
    SkASSERT(NULL != context && NULL != sampler);
    SkScalar diffLen = fDiff.length();
    if (0 != diffLen) {
        SkScalar invDiffLen = SkScalarInvert(diffLen);
        sampler->matrix()->setSinCos(-SkScalarMul(invDiffLen, fDiff.fY),
                                     SkScalarMul(invDiffLen, fDiff.fX));
    } else {
        sampler->matrix()->reset();
    }
    sampler->matrix()->preConcat(fPtsToUnit);
    sampler->textureParams()->setTileModeX(fTileMode);
    sampler->textureParams()->setTileModeY(kClamp_TileMode);
    sampler->textureParams()->setBilerp(true);
    return SkNEW_ARGS(GrRadial2Gradient, (context, *this, sampler));
}

#else

GrCustomStage* SkTwoPointRadialGradient::asNewCustomStage(
    GrContext* context, GrSamplerState* sampler) const {
    SkDEBUGFAIL("Should not call in GPU-less build");
    return NULL;
}

#endif
