






#include "SkColorMatrixFilter.h"
#include "SkColorMatrix.h"
#include "SkColorPriv.h"
#include "SkFlattenableBuffers.h"
#include "SkUnPreMultiply.h"

static int32_t rowmul4(const int32_t array[], unsigned r, unsigned g,
                          unsigned b, unsigned a) {
    return array[0] * r + array[1] * g  + array[2] * b + array[3] * a + array[4];
}

static int32_t rowmul3(const int32_t array[], unsigned r, unsigned g,
                       unsigned b) {
    return array[0] * r + array[1] * g  + array[2] * b + array[4];
}

static void General(const SkColorMatrixFilter::State& state,
                    unsigned r, unsigned g, unsigned b, unsigned a,
                    int32_t* SK_RESTRICT result) {
    const int32_t* SK_RESTRICT array = state.fArray;
    const int shift = state.fShift;

    result[0] = rowmul4(&array[0], r, g, b, a) >> shift;
    result[1] = rowmul4(&array[5], r, g, b, a) >> shift;
    result[2] = rowmul4(&array[10], r, g, b, a) >> shift;
    result[3] = rowmul4(&array[15], r, g, b, a) >> shift;
}

static void General16(const SkColorMatrixFilter::State& state,
                      unsigned r, unsigned g, unsigned b, unsigned a,
                      int32_t* SK_RESTRICT result) {
    const int32_t* SK_RESTRICT array = state.fArray;

    result[0] = rowmul4(&array[0], r, g, b, a) >> 16;
    result[1] = rowmul4(&array[5], r, g, b, a) >> 16;
    result[2] = rowmul4(&array[10], r, g, b, a) >> 16;
    result[3] = rowmul4(&array[15], r, g, b, a) >> 16;
}

static void AffineAdd(const SkColorMatrixFilter::State& state,
                      unsigned r, unsigned g, unsigned b, unsigned a,
                      int32_t* SK_RESTRICT result) {
    const int32_t* SK_RESTRICT array = state.fArray;
    const int shift = state.fShift;

    result[0] = rowmul3(&array[0], r, g, b) >> shift;
    result[1] = rowmul3(&array[5], r, g, b) >> shift;
    result[2] = rowmul3(&array[10], r, g, b) >> shift;
    result[3] = a;
}

static void AffineAdd16(const SkColorMatrixFilter::State& state,
                        unsigned r, unsigned g, unsigned b, unsigned a,
                        int32_t* SK_RESTRICT result) {
    const int32_t* SK_RESTRICT array = state.fArray;

    result[0] = rowmul3(&array[0], r, g, b) >> 16;
    result[1] = rowmul3(&array[5], r, g, b) >> 16;
    result[2] = rowmul3(&array[10], r, g, b) >> 16;
    result[3] = a;
}

static void ScaleAdd(const SkColorMatrixFilter::State& state,
                     unsigned r, unsigned g, unsigned b, unsigned a,
                     int32_t* SK_RESTRICT result) {
    const int32_t* SK_RESTRICT array = state.fArray;
    const int shift = state.fShift;

    
    result[0] = (array[0] * (int)r + array[4]) >> shift;
    result[1] = (array[6] * (int)g + array[9]) >> shift;
    result[2] = (array[12] * (int)b + array[14]) >> shift;
    result[3] = a;
}

static void ScaleAdd16(const SkColorMatrixFilter::State& state,
                       unsigned r, unsigned g, unsigned b, unsigned a,
                       int32_t* SK_RESTRICT result) {
    const int32_t* SK_RESTRICT array = state.fArray;

    
    result[0] = (array[0] * (int)r + array[4]) >> 16;
    result[1] = (array[6] * (int)g + array[9]) >> 16;
    result[2] = (array[12] * (int)b + array[14]) >> 16;
    result[3] = a;
}

static void Add(const SkColorMatrixFilter::State& state,
                unsigned r, unsigned g, unsigned b, unsigned a,
                int32_t* SK_RESTRICT result) {
    const int32_t* SK_RESTRICT array = state.fArray;
    const int shift = state.fShift;

    result[0] = r + (array[4] >> shift);
    result[1] = g + (array[9] >> shift);
    result[2] = b + (array[14] >> shift);
    result[3] = a;
}

static void Add16(const SkColorMatrixFilter::State& state,
                  unsigned r, unsigned g, unsigned b, unsigned a,
                  int32_t* SK_RESTRICT result) {
    const int32_t* SK_RESTRICT array = state.fArray;

    result[0] = r + (array[4] >> 16);
    result[1] = g + (array[9] >> 16);
    result[2] = b + (array[14] >> 16);
    result[3] = a;
}

#define kNO_ALPHA_FLAGS (SkColorFilter::kAlphaUnchanged_Flag |  \
                         SkColorFilter::kHasFilter16_Flag)



void SkColorMatrixFilter::initState(const SkScalar* SK_RESTRICT src) {
    int32_t* array = fState.fArray;
    SkFixed max = 0;
    for (int i = 0; i < 20; i++) {
        SkFixed value = SkScalarToFixed(src[i]);
        array[i] = value;
        value = SkAbs32(value);
        max = SkMax32(max, value);
    }

    



    int bits = SkCLZ(max);
    int32_t one = SK_Fixed1;

    fState.fShift = 16; 
    if (bits < 9) {
        bits = 9 - bits;
        fState.fShift -= bits;
        for (int i = 0; i < 20; i++) {
            array[i] >>= bits;
        }
        one >>= bits;
    }

    
    int32_t changesAlpha = (array[15] | array[16] | array[17] |
                            (array[18] - one) | array[19]);
    int32_t usesAlpha = (array[3] | array[8] | array[13]);
    bool shiftIs16 = (16 == fState.fShift);

    if (changesAlpha | usesAlpha) {
        fProc = shiftIs16 ? General16 : General;
        fFlags = changesAlpha ? 0 : SkColorFilter::kAlphaUnchanged_Flag;
    } else {
        fFlags = kNO_ALPHA_FLAGS;

        int32_t needsScale = (array[0] - one) |       
                             (array[6] - one) |       
                             (array[12] - one);       

        int32_t needs3x3 =  array[1] | array[2] |     
                            array[5] | array[7] |     
                            array[10] | array[11];    

        if (needs3x3) {
            fProc = shiftIs16 ? AffineAdd16 : AffineAdd;
        } else if (needsScale) {
            fProc = shiftIs16 ? ScaleAdd16 : ScaleAdd;
        } else if (array[4] | array[9] | array[14]) {   
            fProc = shiftIs16 ? Add16 : Add;
        } else {
            fProc = NULL;   
        }
    }

    



    if (NULL != fProc) {
        int32_t add = 1 << (fState.fShift - 1);
        array[4] += add;
        array[9] += add;
        array[14] += add;
        array[19] += add;
    }
}



static int32_t pin(int32_t value, int32_t max) {
    if (value < 0) {
        value = 0;
    }
    if (value > max) {
        value = max;
    }
    return value;
}

SkColorMatrixFilter::SkColorMatrixFilter(const SkColorMatrix& cm) : fMatrix(cm) {
    this->initState(cm.fMat);
}

SkColorMatrixFilter::SkColorMatrixFilter(const SkScalar array[20]) {
    memcpy(fMatrix.fMat, array, 20 * sizeof(SkScalar));
    this->initState(array);
}

uint32_t SkColorMatrixFilter::getFlags() const {
    return this->INHERITED::getFlags() | fFlags;
}

void SkColorMatrixFilter::filterSpan(const SkPMColor src[], int count,
                                     SkPMColor dst[]) const {
    Proc proc = fProc;
    const State& state = fState;
    int32_t result[4];

    if (NULL == proc) {
        if (src != dst) {
            memcpy(dst, src, count * sizeof(SkPMColor));
        }
        return;
    }

    const SkUnPreMultiply::Scale* table = SkUnPreMultiply::GetScaleTable();

    for (int i = 0; i < count; i++) {
        SkPMColor c = src[i];

        unsigned r = SkGetPackedR32(c);
        unsigned g = SkGetPackedG32(c);
        unsigned b = SkGetPackedB32(c);
        unsigned a = SkGetPackedA32(c);

        
        if (255 != a) {
            SkUnPreMultiply::Scale scale = table[a];
            r = SkUnPreMultiply::ApplyScale(scale, r);
            g = SkUnPreMultiply::ApplyScale(scale, g);
            b = SkUnPreMultiply::ApplyScale(scale, b);

            SkASSERT(r <= 255);
            SkASSERT(g <= 255);
            SkASSERT(b <= 255);
        }

        proc(state, r, g, b, a, result);

        r = pin(result[0], SK_R32_MASK);
        g = pin(result[1], SK_G32_MASK);
        b = pin(result[2], SK_B32_MASK);
        a = pin(result[3], SK_A32_MASK);
        
        dst[i] = SkPremultiplyARGBInline(a, r, g, b);
    }
}

void SkColorMatrixFilter::filterSpan16(const uint16_t src[], int count,
                                       uint16_t dst[]) const {
    SkASSERT(fFlags & SkColorFilter::kHasFilter16_Flag);

    Proc   proc = fProc;
    const State& state = fState;
    int32_t result[4];

    if (NULL == proc) {
        if (src != dst) {
            memcpy(dst, src, count * sizeof(uint16_t));
        }
        return;
    }

    for (int i = 0; i < count; i++) {
        uint16_t c = src[i];

        
        unsigned r = SkPacked16ToR32(c);
        unsigned g = SkPacked16ToG32(c);
        unsigned b = SkPacked16ToB32(c);

        proc(state, r, g, b, 0, result);

        r = pin(result[0], SK_R32_MASK);
        g = pin(result[1], SK_G32_MASK);
        b = pin(result[2], SK_B32_MASK);

        
        dst[i] = SkPack888ToRGB16(r, g, b);
    }
}



void SkColorMatrixFilter::flatten(SkFlattenableWriteBuffer& buffer) const {
    this->INHERITED::flatten(buffer);
    SkASSERT(sizeof(fMatrix.fMat)/sizeof(SkScalar) == 20);
    buffer.writeScalarArray(fMatrix.fMat, 20);
}

SkColorMatrixFilter::SkColorMatrixFilter(SkFlattenableReadBuffer& buffer)
        : INHERITED(buffer) {
    SkASSERT(buffer.getArrayCount() == 20);
    buffer.readScalarArray(fMatrix.fMat);
    this->initState(fMatrix.fMat);
}

bool SkColorMatrixFilter::asColorMatrix(SkScalar matrix[20]) const {
    if (matrix) {
        memcpy(matrix, fMatrix.fMat, 20 * sizeof(SkScalar));
    }
    return true;
}

#if SK_SUPPORT_GPU
#include "GrEffect.h"
#include "GrTBackendEffectFactory.h"
#include "gl/GrGLEffect.h"

class ColorMatrixEffect : public GrEffect {
public:
    static GrEffectRef* Create(const SkColorMatrix& matrix) {
        AutoEffectUnref effect(SkNEW_ARGS(ColorMatrixEffect, (matrix)));
        return CreateEffectRef(effect);
    }

    static const char* Name() { return "Color Matrix"; }

    virtual const GrBackendEffectFactory& getFactory() const SK_OVERRIDE {
        return GrTBackendEffectFactory<ColorMatrixEffect>::getInstance();
    }

    virtual void getConstantColorComponents(GrColor* color,
                                            uint32_t* validFlags) const SK_OVERRIDE {
        
        

        
        
        
        static const uint32_t kRGBAFlags[] = {
            kR_GrColorComponentFlag,
            kG_GrColorComponentFlag,
            kB_GrColorComponentFlag,
            kA_GrColorComponentFlag
        };
        static const int kShifts[] = {
            GrColor_SHIFT_R, GrColor_SHIFT_G, GrColor_SHIFT_B, GrColor_SHIFT_A,
        };
        enum {
            kAlphaRowStartIdx = 15,
            kAlphaRowTranslateIdx = 19,
        };

        SkScalar outputA = 0;
        for (int i = 0; i < 4; ++i) {
            
            
            if (0 != fMatrix.fMat[kAlphaRowStartIdx + i]) {
                if (!(*validFlags & kRGBAFlags[i])) {
                    *validFlags = 0;
                    return;
                } else {
                    uint32_t component = (*color >> kShifts[i]) & 0xFF;
                    outputA += fMatrix.fMat[kAlphaRowStartIdx + i] * component;
                }
            }
        }
        outputA += fMatrix.fMat[kAlphaRowTranslateIdx];
        *validFlags = kA_GrColorComponentFlag;
        
        
        
        
        *color = static_cast<uint8_t>(SkScalarPin(outputA, 0, 255)) << GrColor_SHIFT_A;
    }

    GR_DECLARE_EFFECT_TEST;

    class GLEffect : public GrGLEffect {
    public:
        
        static EffectKey GenKey(const GrDrawEffect&, const GrGLCaps&) { return 0; }

        GLEffect(const GrBackendEffectFactory& factory,
                 const GrDrawEffect&)
        : INHERITED(factory)
        , fMatrixHandle(GrGLUniformManager::kInvalidUniformHandle)
        , fVectorHandle(GrGLUniformManager::kInvalidUniformHandle) {}

        virtual void emitCode(GrGLShaderBuilder* builder,
                              const GrDrawEffect&,
                              EffectKey,
                              const char* outputColor,
                              const char* inputColor,
                              const TextureSamplerArray&) SK_OVERRIDE {
            fMatrixHandle = builder->addUniform(GrGLShaderBuilder::kFragment_ShaderType,
                                                kMat44f_GrSLType,
                                                "ColorMatrix");
            fVectorHandle = builder->addUniform(GrGLShaderBuilder::kFragment_ShaderType,
                                                kVec4f_GrSLType,
                                                "ColorMatrixVector");

            if (NULL == inputColor) {
                
                inputColor = GrGLSLOnesVecf(4);
            }
            
            
            builder->fsCodeAppendf("\tfloat nonZeroAlpha = max(%s.a, 0.00001);\n", inputColor);
            builder->fsCodeAppendf("\t%s = %s * vec4(%s.rgb / nonZeroAlpha, nonZeroAlpha) + %s;\n",
                                   outputColor,
                                   builder->getUniformCStr(fMatrixHandle),
                                   inputColor,
                                   builder->getUniformCStr(fVectorHandle));
            builder->fsCodeAppendf("\t%s.rgb *= %s.a;\n", outputColor, outputColor);
        }

        virtual void setData(const GrGLUniformManager& uniManager,
                             const GrDrawEffect& drawEffect) SK_OVERRIDE {
            const ColorMatrixEffect& cme = drawEffect.castEffect<ColorMatrixEffect>();
            const float* m = cme.fMatrix.fMat;
            
            GrGLfloat mt[]  = {
                m[0], m[5], m[10], m[15],
                m[1], m[6], m[11], m[16],
                m[2], m[7], m[12], m[17],
                m[3], m[8], m[13], m[18],
            };
            static const float kScale = 1.0f / 255.0f;
            GrGLfloat vec[] = {
                m[4] * kScale, m[9] * kScale, m[14] * kScale, m[19] * kScale,
            };
            uniManager.setMatrix4fv(fMatrixHandle, 0, 1, mt);
            uniManager.set4fv(fVectorHandle, 0, 1, vec);
        }

    private:
        GrGLUniformManager::UniformHandle fMatrixHandle;
        GrGLUniformManager::UniformHandle fVectorHandle;
    };

private:
    ColorMatrixEffect(const SkColorMatrix& matrix) : fMatrix(matrix) {}

    virtual bool onIsEqual(const GrEffect& s) const {
        const ColorMatrixEffect& cme = CastEffect<ColorMatrixEffect>(s);
        return cme.fMatrix == fMatrix;
    }

    SkColorMatrix fMatrix;

    typedef GrGLEffect INHERITED;
};

GR_DEFINE_EFFECT_TEST(ColorMatrixEffect);

GrEffectRef* ColorMatrixEffect::TestCreate(SkMWCRandom* random,
                                           GrContext*,
                                           const GrDrawTargetCaps&,
                                           GrTexture* dummyTextures[2]) {
    SkColorMatrix colorMatrix;
    for (size_t i = 0; i < SK_ARRAY_COUNT(colorMatrix.fMat); ++i) {
        colorMatrix.fMat[i] = random->nextSScalar1();
    }
    return ColorMatrixEffect::Create(colorMatrix);
}

GrEffectRef* SkColorMatrixFilter::asNewEffect(GrContext*) const {
    return ColorMatrixEffect::Create(fMatrix);
}

#endif
