






#include "SkTypes.h"

#include "SkColor.h"
#include "SkFloatingPoint.h"
#include "SkMaskGamma.h"

class SkLinearColorSpaceLuminance : public SkColorSpaceLuminance {
    virtual SkScalar toLuma(SkScalar SkDEBUGCODE(gamma), SkScalar luminance) const SK_OVERRIDE {
        SkASSERT(SK_Scalar1 == gamma);
        return luminance;
    }
    virtual SkScalar fromLuma(SkScalar SkDEBUGCODE(gamma), SkScalar luma) const SK_OVERRIDE {
        SkASSERT(SK_Scalar1 == gamma);
        return luma;
    }
};

class SkGammaColorSpaceLuminance : public SkColorSpaceLuminance {
    virtual SkScalar toLuma(SkScalar gamma, SkScalar luminance) const SK_OVERRIDE {
        return SkScalarPow(luminance, gamma);
    }
    virtual SkScalar fromLuma(SkScalar gamma, SkScalar luma) const SK_OVERRIDE {
        return SkScalarPow(luma, SkScalarInvert(gamma));
    }
};

class SkSRGBColorSpaceLuminance : public SkColorSpaceLuminance {
    virtual SkScalar toLuma(SkScalar SkDEBUGCODE(gamma), SkScalar luminance) const SK_OVERRIDE {
        SkASSERT(0 == gamma);
        
        
        if (luminance <= SkFloatToScalar(0.04045f)) {
            return luminance / SkFloatToScalar(12.92f);
        }
        return SkScalarPow((luminance + SkFloatToScalar(0.055f)) / SkFloatToScalar(1.055f),
                        SkFloatToScalar(2.4f));
    }
    virtual SkScalar fromLuma(SkScalar SkDEBUGCODE(gamma), SkScalar luma) const SK_OVERRIDE {
        SkASSERT(0 == gamma);
        
        
        if (luma <= SkFloatToScalar(0.0031308f)) {
            return luma * SkFloatToScalar(12.92f);
        }
        return SkFloatToScalar(1.055f) * SkScalarPow(luma, SkScalarInvert(SkFloatToScalar(2.4f)))
               - SkFloatToScalar(0.055f);
    }
};

 const SkColorSpaceLuminance& SkColorSpaceLuminance::Fetch(SkScalar gamma) {
    static SkLinearColorSpaceLuminance gSkLinearColorSpaceLuminance;
    static SkGammaColorSpaceLuminance gSkGammaColorSpaceLuminance;
    static SkSRGBColorSpaceLuminance gSkSRGBColorSpaceLuminance;

    if (0 == gamma) {
        return gSkSRGBColorSpaceLuminance;
    } else if (SK_Scalar1 == gamma) {
        return gSkLinearColorSpaceLuminance;
    } else {
        return gSkGammaColorSpaceLuminance;
    }
}

static float apply_contrast(float srca, float contrast) {
    return srca + ((1.0f - srca) * contrast * srca);
}

void SkTMaskGamma_build_correcting_lut(uint8_t table[256], U8CPU srcI, SkScalar contrast,
                                       const SkColorSpaceLuminance& srcConvert, SkScalar srcGamma,
                                       const SkColorSpaceLuminance& dstConvert, SkScalar dstGamma) {
    const float src = (float)srcI / 255.0f;
    const float linSrc = srcConvert.toLuma(srcGamma, src);
    
    
    
    
    const float dst = 1.0f - src;
    const float linDst = dstConvert.toLuma(dstGamma, dst);

    
    const float adjustedContrast = SkScalarToFloat(contrast) * linDst;

    
    
    if (fabs(src - dst) < (1.0f / 256.0f)) {
        float ii = 0.0f;
        for (int i = 0; i < 256; ++i, ii += 1.0f) {
            float rawSrca = ii / 255.0f;
            float srca = apply_contrast(rawSrca, adjustedContrast);
            table[i] = SkToU8(sk_float_round2int(255.0f * srca));
        }
    } else {
        
        float ii = 0.0f;
        for (int i = 0; i < 256; ++i, ii += 1.0f) {
            
            
            
            
            float rawSrca = ii / 255.0f;
            float srca = apply_contrast(rawSrca, adjustedContrast);
            SkASSERT(srca <= 1.0f);
            float dsta = 1.0f - srca;

            
            float linOut = (linSrc * srca + dsta * linDst);
            SkASSERT(linOut <= 1.0f);
            float out = dstConvert.fromLuma(dstGamma, linOut);

            
            float result = (out - dst) / (src - dst);
            SkASSERT(sk_float_round2int(255.0f * result) <= 255);

            table[i] = SkToU8(sk_float_round2int(255.0f * result));
        }
    }
}
