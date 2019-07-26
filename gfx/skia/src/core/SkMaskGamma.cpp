






#include "SkTypes.h"

#include "SkColor.h"
#include "SkFloatingPoint.h"
#include "SkMaskGamma.h"

SkScalar SkSRGBLuminance::toLuma(SkScalar luminance) const {
    
    
    if (luminance <= SkFloatToScalar(0.04045f)) {
        return luminance / SkFloatToScalar(12.92f);
    }
    return SkScalarPow((luminance + SkFloatToScalar(0.055f)) / SkFloatToScalar(1.055f),
                       SkFloatToScalar(2.4f));
}

SkScalar SkSRGBLuminance::fromLuma(SkScalar luma) const {
    
    
    if (luma <= SkFloatToScalar(0.0031308f)) {
        return luma * SkFloatToScalar(12.92f);
    }
    return SkFloatToScalar(1.055f) * SkScalarPow(luma, SkScalarInvert(SkFloatToScalar(2.4f)))
           - SkFloatToScalar(0.055f);
}

SkGammaLuminance::SkGammaLuminance(SkScalar gamma)
    : fGamma(gamma)
    , fGammaInverse(SkScalarInvert(gamma)) {
}

SkScalar SkGammaLuminance::toLuma(SkScalar luminance) const {
    return SkScalarPow(luminance, fGamma);
}

SkScalar SkGammaLuminance::fromLuma(SkScalar luma) const {
    return SkScalarPow(luma, fGammaInverse);
}

SkScalar SkLinearLuminance::toLuma(SkScalar luminance) const {
    return luminance;
}

SkScalar SkLinearLuminance::fromLuma(SkScalar luma) const {
    return luma;
}

static float apply_contrast(float srca, float contrast) {
    return srca + ((1.0f - srca) * contrast * srca);
}

void SkTMaskGamma_build_correcting_lut(uint8_t table[256], U8CPU srcI, SkScalar contrast,
                                       const SkColorSpaceLuminance& srcConvert,
                                       const SkColorSpaceLuminance& dstConvert) {
    const float src = (float)srcI / 255.0f;
    const float linSrc = srcConvert.toLuma(src);
    
    
    
    
    const float dst = 1.0f - src;
    const float linDst = dstConvert.toLuma(dst);

    
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
            float out = dstConvert.fromLuma(linOut);

            
            float result = (out - dst) / (src - dst);
            SkASSERT(sk_float_round2int(255.0f * result) <= 255);

            table[i] = SkToU8(sk_float_round2int(255.0f * result));
        }
    }
}
