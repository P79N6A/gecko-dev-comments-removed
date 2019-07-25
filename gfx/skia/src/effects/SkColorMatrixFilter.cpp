






#include "SkColorMatrixFilter.h"
#include "SkColorMatrix.h"
#include "SkColorPriv.h"
#include "SkUnPreMultiply.h"

static int32_t rowmul4(const int32_t array[], unsigned r, unsigned g,
                          unsigned b, unsigned a) {
    return array[0] * r + array[1] * g  + array[2] * b + array[3] * a + array[4];
}

static int32_t rowmul3(const int32_t array[], unsigned r, unsigned g,
                       unsigned b) {
    return array[0] * r + array[1] * g  + array[2] * b + array[4];
}

static void General(SkColorMatrixFilter::State* state,
                    unsigned r, unsigned g, unsigned b, unsigned a) {
    const int32_t* SK_RESTRICT array = state->fArray;
    const int shift = state->fShift;
    int32_t* SK_RESTRICT result = state->fResult;

    result[0] = rowmul4(&array[0], r, g, b, a) >> shift;
    result[1] = rowmul4(&array[5], r, g, b, a) >> shift;
    result[2] = rowmul4(&array[10], r, g, b, a) >> shift;
    result[3] = rowmul4(&array[15], r, g, b, a) >> shift;
}

static void General16(SkColorMatrixFilter::State* state,
                      unsigned r, unsigned g, unsigned b, unsigned a) {
    const int32_t* SK_RESTRICT array = state->fArray;
    int32_t* SK_RESTRICT result = state->fResult;

    result[0] = rowmul4(&array[0], r, g, b, a) >> 16;
    result[1] = rowmul4(&array[5], r, g, b, a) >> 16;
    result[2] = rowmul4(&array[10], r, g, b, a) >> 16;
    result[3] = rowmul4(&array[15], r, g, b, a) >> 16;
}

static void AffineAdd(SkColorMatrixFilter::State* state,
                      unsigned r, unsigned g, unsigned b, unsigned a) {
    const int32_t* SK_RESTRICT array = state->fArray;
    const int shift = state->fShift;
    int32_t* SK_RESTRICT result = state->fResult;

    result[0] = rowmul3(&array[0], r, g, b) >> shift;
    result[1] = rowmul3(&array[5], r, g, b) >> shift;
    result[2] = rowmul3(&array[10], r, g, b) >> shift;
    result[3] = a;
}

static void AffineAdd16(SkColorMatrixFilter::State* state,
                        unsigned r, unsigned g, unsigned b, unsigned a) {
    const int32_t* SK_RESTRICT array = state->fArray;
    int32_t* SK_RESTRICT result = state->fResult;

    result[0] = rowmul3(&array[0], r, g, b) >> 16;
    result[1] = rowmul3(&array[5], r, g, b) >> 16;
    result[2] = rowmul3(&array[10], r, g, b) >> 16;
    result[3] = a;
}

static void ScaleAdd(SkColorMatrixFilter::State* state,
                     unsigned r, unsigned g, unsigned b, unsigned a) {
    const int32_t* SK_RESTRICT array = state->fArray;
    const int shift = state->fShift;
    int32_t* SK_RESTRICT result = state->fResult;

    
    result[0] = (array[0] * (int)r + array[4]) >> shift;
    result[1] = (array[6] * (int)g + array[9]) >> shift;
    result[2] = (array[12] * (int)b + array[14]) >> shift;
    result[3] = a;
}

static void ScaleAdd16(SkColorMatrixFilter::State* state,
                       unsigned r, unsigned g, unsigned b, unsigned a) {
    const int32_t* SK_RESTRICT array = state->fArray;
    int32_t* SK_RESTRICT result = state->fResult;

    
    result[0] = (array[0] * (int)r + array[4]) >> 16;
    result[1] = (array[6] * (int)g + array[9]) >> 16;
    result[2] = (array[12] * (int)b + array[14]) >> 16;
    result[3] = a;
}

static void Add(SkColorMatrixFilter::State* state,
                unsigned r, unsigned g, unsigned b, unsigned a) {
    const int32_t* SK_RESTRICT array = state->fArray;
    const int shift = state->fShift;
    int32_t* SK_RESTRICT result = state->fResult;

    result[0] = r + (array[4] >> shift);
    result[1] = g + (array[9] >> shift);
    result[2] = b + (array[14] >> shift);
    result[3] = a;
}

static void Add16(SkColorMatrixFilter::State* state,
                  unsigned r, unsigned g, unsigned b, unsigned a) {
    const int32_t* SK_RESTRICT array = state->fArray;
    int32_t* SK_RESTRICT result = state->fResult;

    result[0] = r + (array[4] >> 16);
    result[1] = g + (array[9] >> 16);
    result[2] = b + (array[14] >> 16);
    result[3] = a;
}

#define kNO_ALPHA_FLAGS (SkColorFilter::kAlphaUnchanged_Flag |  \
                         SkColorFilter::kHasFilter16_Flag)



void SkColorMatrixFilter::setup(const SkScalar* SK_RESTRICT src) {
    if (NULL == src) {
        fProc = NULL;   
        fFlags  = kNO_ALPHA_FLAGS;
        
        return;
    }

    int32_t* SK_RESTRICT array = fState.fArray;

    int i;
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
        for (i = 0; i < 20; i++) {
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

SkColorMatrixFilter::SkColorMatrixFilter() {
    this->setup(NULL);
}

SkColorMatrixFilter::SkColorMatrixFilter(const SkColorMatrix& cm) {
    this->setup(cm.fMat);
}

SkColorMatrixFilter::SkColorMatrixFilter(const SkScalar array[20]) {
    this->setup(array);
}

uint32_t SkColorMatrixFilter::getFlags() {
    return this->INHERITED::getFlags() | fFlags;
}

void SkColorMatrixFilter::filterSpan(const SkPMColor src[], int count,
                                     SkPMColor dst[]) {
    Proc proc = fProc;
    State* state = &fState;
    int32_t* SK_RESTRICT result = state->fResult;

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

        proc(state, r, g, b, a);

        r = pin(result[0], SK_R32_MASK);
        g = pin(result[1], SK_G32_MASK);
        b = pin(result[2], SK_B32_MASK);
        a = pin(result[3], SK_A32_MASK);
        
        if (255 != a) {
            int scale = SkAlpha255To256(a);
            r = SkAlphaMul(r, scale);
            g = SkAlphaMul(g, scale);
            b = SkAlphaMul(b, scale);
        }
        dst[i] = SkPackARGB32(a, r, g, b);
    }
}

void SkColorMatrixFilter::filterSpan16(const uint16_t src[], int count,
                                       uint16_t dst[]) {
    SkASSERT(fFlags & SkColorFilter::kHasFilter16_Flag);

    Proc   proc = fProc;
    State* state = &fState;
    int32_t* SK_RESTRICT result = state->fResult;

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

        proc(state, r, g, b, 0);

        r = pin(result[0], SK_R32_MASK);
        g = pin(result[1], SK_G32_MASK);
        b = pin(result[2], SK_B32_MASK);

        
        dst[i] = SkPack888ToRGB16(r, g, b);
    }
}



void SkColorMatrixFilter::flatten(SkFlattenableWriteBuffer& buffer)  {
    this->INHERITED::flatten(buffer);

    buffer.writeFunctionPtr((void*)fProc);
    buffer.writeMul4(&fState, sizeof(fState));
    buffer.write32(fFlags);
}

SkFlattenable::Factory SkColorMatrixFilter::getFactory() { return CreateProc;  }

SkColorMatrixFilter::SkColorMatrixFilter(SkFlattenableReadBuffer& buffer)
        : INHERITED(buffer) {
    fProc = (Proc)buffer.readFunctionPtr();
    buffer.read(&fState, sizeof(fState));
    fFlags = buffer.readU32();
}

SkFlattenable* SkColorMatrixFilter::CreateProc(SkFlattenableReadBuffer& buf) {
    return SkNEW_ARGS(SkColorMatrixFilter, (buf));
}

static SkFlattenable::Registrar
  gSkColorMatrixFilterReg("SkColorMatrixFilter",
                          SkColorMatrixFilter::CreateProc);
