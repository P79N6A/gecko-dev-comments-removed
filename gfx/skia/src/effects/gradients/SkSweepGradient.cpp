







#include "SkSweepGradient.h"

SkSweepGradient::SkSweepGradient(SkScalar cx, SkScalar cy, const SkColor colors[],
               const SkScalar pos[], int count, SkUnitMapper* mapper)
: SkGradientShaderBase(colors, pos, count, SkShader::kClamp_TileMode, mapper),
  fCenter(SkPoint::Make(cx, cy))
{
    fPtsToUnit.setTranslate(-cx, -cy);
}

SkShader::BitmapType SkSweepGradient::asABitmap(SkBitmap* bitmap,
    SkMatrix* matrix, SkShader::TileMode* xy) const {
    if (bitmap) {
        this->getGradientTableBitmap(bitmap);
    }
    if (matrix) {
        *matrix = fPtsToUnit;
    }
    if (xy) {
        xy[0] = fTileMode;
        xy[1] = kClamp_TileMode;
    }
    return kSweep_BitmapType;
}

SkShader::GradientType SkSweepGradient::asAGradient(GradientInfo* info) const {
    if (info) {
        commonAsAGradient(info);
        info->fPoint[0] = fCenter;
    }
    return kSweep_GradientType;
}

SkSweepGradient::SkSweepGradient(SkFlattenableReadBuffer& buffer)
    : INHERITED(buffer),
      fCenter(buffer.readPoint()) {
}

void SkSweepGradient::flatten(SkFlattenableWriteBuffer& buffer) const {
    this->INHERITED::flatten(buffer);
    buffer.writePoint(fCenter);
}

#ifndef SK_SCALAR_IS_FLOAT
#ifdef COMPUTE_SWEEP_TABLE
#define PI  3.14159265
static bool gSweepTableReady;
static uint8_t gSweepTable[65];




static const uint8_t* build_sweep_table() {
    if (!gSweepTableReady) {
        const int N = 65;
        const double DENOM = N - 1;

        for (int i = 0; i < N; i++)
        {
            double arg = i / DENOM;
            double v = atan(arg);
            int iv = (int)round(v * DENOM * 2 / PI);

            printf("%d, ", iv);
            gSweepTable[i] = iv;
        }
        gSweepTableReady = true;
    }
    return gSweepTable;
}
#else
static const uint8_t gSweepTable[] = {
    0, 1, 1, 2, 3, 3, 4, 4, 5, 6, 6, 7, 8, 8, 9, 9,
    10, 11, 11, 12, 12, 13, 13, 14, 15, 15, 16, 16, 17, 17, 18, 18,
    19, 19, 20, 20, 21, 21, 22, 22, 23, 23, 24, 24, 25, 25, 25, 26,
    26, 27, 27, 27, 28, 28, 29, 29, 29, 30, 30, 30, 31, 31, 31, 32,
    32
};
static const uint8_t* build_sweep_table() { return gSweepTable; }
#endif
#endif






#ifndef SK_SCALAR_IS_FLOAT
static unsigned div_64(int numer, int denom) {
    SkASSERT(numer <= denom);
    SkASSERT(numer > 0);
    SkASSERT(denom > 0);

    int nbits = SkCLZ(numer);
    int dbits = SkCLZ(denom);
    int bits = 6 - nbits + dbits;
    SkASSERT(bits <= 6);

    if (bits < 0) {  
        return 0;
    }

    denom <<= dbits - 1;
    numer <<= nbits - 1;

    unsigned result = 0;

    
    if ((numer -= denom) >= 0) {
        result = 1;
    } else {
        numer += denom;
    }

    
    if (bits > 0) {
        
        result <<= bits;
        switch (bits) {
        case 6:
            if ((numer = (numer << 1) - denom) >= 0)
                result |= 32;
            else
                numer += denom;
        case 5:
            if ((numer = (numer << 1) - denom) >= 0)
                result |= 16;
            else
                numer += denom;
        case 4:
            if ((numer = (numer << 1) - denom) >= 0)
                result |= 8;
            else
                numer += denom;
        case 3:
            if ((numer = (numer << 1) - denom) >= 0)
                result |= 4;
            else
                numer += denom;
        case 2:
            if ((numer = (numer << 1) - denom) >= 0)
                result |= 2;
            else
                numer += denom;
        case 1:
        default:    
            if ((numer = (numer << 1) - denom) >= 0)
                result |= 1;
            else
                numer += denom;
        }
    }
    return result;
}
#endif


#ifndef SK_SCALAR_IS_FLOAT
static unsigned atan_0_90(SkFixed y, SkFixed x) {
#ifdef SK_DEBUG
    {
        static bool gOnce;
        if (!gOnce) {
            gOnce = true;
            SkASSERT(div_64(55, 55) == 64);
            SkASSERT(div_64(128, 256) == 32);
            SkASSERT(div_64(2326528, 4685824) == 31);
            SkASSERT(div_64(753664, 5210112) == 9);
            SkASSERT(div_64(229376, 4882432) == 3);
            SkASSERT(div_64(2, 64) == 2);
            SkASSERT(div_64(1, 64) == 1);
            
            SkASSERT(div_64(12345, 0x54321234) == 0);
        }
    }
#endif

    SkASSERT(y > 0 && x > 0);
    const uint8_t* table = build_sweep_table();

    unsigned result;
    bool swap = (x < y);
    if (swap) {
        
        
        SkTSwap<SkFixed>(x, y);
    }

    result = div_64(y, x);

#ifdef SK_DEBUG
    {
        unsigned result2 = SkDivBits(y, x, 6);
        SkASSERT(result2 == result ||
                 (result == 1 && result2 == 0));
    }
#endif

    SkASSERT(result < SK_ARRAY_COUNT(gSweepTable));
    result = table[result];

    if (swap) {
        
        result = 64 - result;
        
        result -= result >> 6;
    }

    SkASSERT(result <= 63);
    return result;
}
#endif


#ifdef SK_SCALAR_IS_FLOAT
static unsigned SkATan2_255(float y, float x) {
    
    static const float g255Over2PI = 40.584510488433314f;

    float result = sk_float_atan2(y, x);
    if (result < 0) {
        result += 2 * SK_ScalarPI;
    }
    SkASSERT(result >= 0);
    
    
    int ir = (int)(result * g255Over2PI);
    SkASSERT(ir >= 0 && ir <= 255);
    return ir;
}
#else
static unsigned SkATan2_255(SkFixed y, SkFixed x) {
    if (x == 0) {
        if (y == 0) {
            return 0;
        }
        return y < 0 ? 192 : 64;
    }
    if (y == 0) {
        return x < 0 ? 128 : 0;
    }

    












    int xsign = x >> 31;
    int ysign = y >> 31;
    int add = ((-xsign) ^ (ysign & 3)) << 6;

#ifdef SK_DEBUG
    if (0 == add)
        SkASSERT(x > 0 && y > 0);
    else if (64 == add)
        SkASSERT(x < 0 && y > 0);
    else if (128 == add)
        SkASSERT(x < 0 && y < 0);
    else if (192 == add)
        SkASSERT(x > 0 && y < 0);
    else
        SkDEBUGFAIL("bad value for add");
#endif

    


    x = (x ^ xsign) - xsign;
    y = (y ^ ysign) - ysign;
    if (add & 64) {             
        SkTSwap<SkFixed>(x, y);
    }

    unsigned result = add + atan_0_90(y, x);
    SkASSERT(result < 256);
    return result;
}
#endif

void SkSweepGradient::shadeSpan(int x, int y, SkPMColor* SK_RESTRICT dstC,
                               int count) {
    SkMatrix::MapXYProc proc = fDstToIndexProc;
    const SkMatrix&     matrix = fDstToIndex;
    const SkPMColor* SK_RESTRICT cache = this->getCache32();
    SkPoint             srcPt;

    if (fDstToIndexClass != kPerspective_MatrixClass) {
        proc(matrix, SkIntToScalar(x) + SK_ScalarHalf,
                     SkIntToScalar(y) + SK_ScalarHalf, &srcPt);
        SkScalar dx, fx = srcPt.fX;
        SkScalar dy, fy = srcPt.fY;

        if (fDstToIndexClass == kFixedStepInX_MatrixClass) {
            SkFixed storage[2];
            (void)matrix.fixedStepInX(SkIntToScalar(y) + SK_ScalarHalf,
                                      &storage[0], &storage[1]);
            dx = SkFixedToScalar(storage[0]);
            dy = SkFixedToScalar(storage[1]);
        } else {
            SkASSERT(fDstToIndexClass == kLinear_MatrixClass);
            dx = matrix.getScaleX();
            dy = matrix.getSkewY();
        }

        for (; count > 0; --count) {
            *dstC++ = cache[SkATan2_255(fy, fx)];
            fx += dx;
            fy += dy;
        }
    } else {  
        for (int stop = x + count; x < stop; x++) {
            proc(matrix, SkIntToScalar(x) + SK_ScalarHalf,
                         SkIntToScalar(y) + SK_ScalarHalf, &srcPt);
            *dstC++ = cache[SkATan2_255(srcPt.fY, srcPt.fX)];
        }
    }
}

void SkSweepGradient::shadeSpan16(int x, int y, uint16_t* SK_RESTRICT dstC,
                                 int count) {
    SkMatrix::MapXYProc proc = fDstToIndexProc;
    const SkMatrix&     matrix = fDstToIndex;
    const uint16_t* SK_RESTRICT cache = this->getCache16();
    int                 toggle = ((x ^ y) & 1) * kDitherStride16;
    SkPoint             srcPt;

    if (fDstToIndexClass != kPerspective_MatrixClass) {
        proc(matrix, SkIntToScalar(x) + SK_ScalarHalf,
                     SkIntToScalar(y) + SK_ScalarHalf, &srcPt);
        SkScalar dx, fx = srcPt.fX;
        SkScalar dy, fy = srcPt.fY;

        if (fDstToIndexClass == kFixedStepInX_MatrixClass) {
            SkFixed storage[2];
            (void)matrix.fixedStepInX(SkIntToScalar(y) + SK_ScalarHalf,
                                      &storage[0], &storage[1]);
            dx = SkFixedToScalar(storage[0]);
            dy = SkFixedToScalar(storage[1]);
        } else {
            SkASSERT(fDstToIndexClass == kLinear_MatrixClass);
            dx = matrix.getScaleX();
            dy = matrix.getSkewY();
        }

        for (; count > 0; --count) {
            int index = SkATan2_255(fy, fx) >> (8 - kCache16Bits);
            *dstC++ = cache[toggle + index];
            toggle ^= kDitherStride16;
            fx += dx;
            fy += dy;
        }
    } else {  
        for (int stop = x + count; x < stop; x++) {
            proc(matrix, SkIntToScalar(x) + SK_ScalarHalf,
                         SkIntToScalar(y) + SK_ScalarHalf, &srcPt);

            int index = SkATan2_255(srcPt.fY, srcPt.fX);
            index >>= (8 - kCache16Bits);
            *dstC++ = cache[toggle + index];
            toggle ^= kDitherStride16;
        }
    }
}



#if SK_SUPPORT_GPU

class GrGLSweepGradient : public GrGLGradientStage {
public:

    GrGLSweepGradient(const GrProgramStageFactory& factory,
                      const GrCustomStage&) : INHERITED (factory) { }
    virtual ~GrGLSweepGradient() { }

    virtual void emitVS(GrGLShaderBuilder* builder,
                        const char* vertexCoords) SK_OVERRIDE { }
    virtual void emitFS(GrGLShaderBuilder* builder,
                        const char* outputColor,
                        const char* inputColor,
                        const TextureSamplerArray&) SK_OVERRIDE;

    static StageKey GenKey(const GrCustomStage& s, const GrGLCaps& caps) { return 0; }

private:

    typedef GrGLGradientStage INHERITED;

};



class GrSweepGradient : public GrGradientEffect {
public:

    GrSweepGradient(GrContext* ctx,
                    const SkSweepGradient& shader,
                    GrSamplerState* sampler)
    : INHERITED(ctx, shader, sampler) { }
    virtual ~GrSweepGradient() { }

    static const char* Name() { return "Sweep Gradient"; }
    virtual const GrProgramStageFactory& getFactory() const SK_OVERRIDE {
        return GrTProgramStageFactory<GrSweepGradient>::getInstance();
    }

    typedef GrGLSweepGradient GLProgramStage;

private:
    GR_DECLARE_CUSTOM_STAGE_TEST;

    typedef GrGradientEffect INHERITED;
};



GR_DEFINE_CUSTOM_STAGE_TEST(GrSweepGradient);

GrCustomStage* GrSweepGradient::TestCreate(SkRandom* random,
                                           GrContext* context,
                                           GrTexture**) {
    SkPoint center = {random->nextUScalar1(), random->nextUScalar1()};

    SkColor colors[kMaxRandomGradientColors];
    SkScalar stopsArray[kMaxRandomGradientColors];
    SkScalar* stops = stopsArray;
    SkShader::TileMode tmIgnored;
    int colorCount = RandomGradientParams(random, colors, &stops, &tmIgnored);
    SkAutoTUnref<SkShader> shader(SkGradientShader::CreateSweep(center.fX, center.fY,
                                                                colors, stops, colorCount));
    GrSamplerState sampler;
    GrCustomStage* stage = shader->asNewCustomStage(context, &sampler);
    GrAssert(NULL != stage);
    return stage;
}



void GrGLSweepGradient::emitFS(GrGLShaderBuilder* builder,
                              const char* outputColor,
                              const char* inputColor,
                              const TextureSamplerArray& samplers) {
    SkString t;
    t.printf("atan(- %s.y, - %s.x) * 0.1591549430918 + 0.5",
        builder->defaultTexCoordsName(), builder->defaultTexCoordsName());
    this->emitColorLookup(builder, t.c_str(), outputColor, inputColor, samplers[0]);
}



GrCustomStage* SkSweepGradient::asNewCustomStage(GrContext* context,
    GrSamplerState* sampler) const {
    sampler->matrix()->preConcat(fPtsToUnit);
    sampler->textureParams()->setTileModeX(fTileMode);
    sampler->textureParams()->setTileModeY(kClamp_TileMode);
    sampler->textureParams()->setBilerp(true);
    return SkNEW_ARGS(GrSweepGradient, (context, *this, sampler));
}

#else

GrCustomStage* SkSweepGradient::asNewCustomStage(GrContext* context,
    GrSamplerState* sampler) const {
    SkDEBUGFAIL("Should not call in GPU-less build");
    return NULL;
}

#endif
