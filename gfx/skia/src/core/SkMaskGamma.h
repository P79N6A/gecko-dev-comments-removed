






#ifndef SkMaskGamma_DEFINED
#define SkMaskGamma_DEFINED

#include "SkTypes.h"
#include "SkColor.h"
#include "SkColorPriv.h"
#include "SkRefCnt.h"








class SkColorSpaceLuminance : SkNoncopyable {
public:
    virtual ~SkColorSpaceLuminance() {};

    
    virtual SkScalar toLuma(SkScalar luminance) const = 0;
    
    virtual SkScalar fromLuma(SkScalar luma) const = 0;

    
    U8CPU computeLuminance(SkColor c) const {
        SkScalar r = toLuma(SkIntToScalar(SkColorGetR(c)) / 255);
        SkScalar g = toLuma(SkIntToScalar(SkColorGetG(c)) / 255);
        SkScalar b = toLuma(SkIntToScalar(SkColorGetB(c)) / 255);
        SkScalar luma = r * SkFloatToScalar(SK_LUM_COEFF_R) +
                        g * SkFloatToScalar(SK_LUM_COEFF_G) +
                        b * SkFloatToScalar(SK_LUM_COEFF_B);
        SkASSERT(luma <= SK_Scalar1);
        return SkScalarRoundToInt(fromLuma(luma) * 255);
    }
};

class SkSRGBLuminance : public SkColorSpaceLuminance {
public:
    SkScalar toLuma(SkScalar luminance) const SK_OVERRIDE;
    SkScalar fromLuma(SkScalar luma) const SK_OVERRIDE;
};

class SkGammaLuminance : public SkColorSpaceLuminance {
public:
    SkGammaLuminance(SkScalar gamma);
    SkScalar toLuma(SkScalar luminance) const SK_OVERRIDE;
    SkScalar fromLuma(SkScalar luma) const SK_OVERRIDE;
private:
    SkScalar fGamma;
    SkScalar fGammaInverse;
};

class SkLinearLuminance : public SkColorSpaceLuminance {
public:
    SkScalar toLuma(SkScalar luminance) const SK_OVERRIDE;
    SkScalar fromLuma(SkScalar luma) const SK_OVERRIDE;
};







template<U8CPU N> static inline U8CPU sk_t_scale255(U8CPU base) {
    base <<= (8 - N);
    U8CPU lum = base;
    for (unsigned int i = N; i < 8; i += N) {
        lum |= base >> i;
    }
    return lum;
}
template<>  inline U8CPU sk_t_scale255<1>(U8CPU base) {
    return base * 0xFF;
}
template<>  inline U8CPU sk_t_scale255<2>(U8CPU base) {
    return base * 0x55;
}
template<>  inline U8CPU sk_t_scale255<4>(U8CPU base) {
    return base * 0x11;
}
template<>  inline U8CPU sk_t_scale255<8>(U8CPU base) {
    return base;
}


template <int R_LUM_BITS, int G_LUM_BITS, int B_LUM_BITS> class SkTMaskPreBlend;

void SkTMaskGamma_build_correcting_lut(uint8_t table[256], U8CPU srcI, SkScalar contrast,
                                       const SkColorSpaceLuminance& srcConvert,
                                       const SkColorSpaceLuminance& dstConvert);












template <int R_LUM_BITS, int G_LUM_BITS, int B_LUM_BITS> class SkTMaskGamma : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT_TEMPLATE(SkTMaskGamma)

    SkTMaskGamma() : fIsLinear(true) {
    }

    








    SkTMaskGamma(SkScalar contrast,
                 const SkColorSpaceLuminance& paint,
                 const SkColorSpaceLuminance& device) : fIsLinear(false) {
        for (U8CPU i = 0; i < (1 << kLuminanceBits_Max); ++i) {
            U8CPU lum = sk_t_scale255<kLuminanceBits_Max>(i);
            SkTMaskGamma_build_correcting_lut(fGammaTables[i], lum, contrast, paint, device);
        }
    }

    
    static SkColor cannonicalColor(SkColor color) {
        return SkColorSetRGB(
                   sk_t_scale255<kLuminanceBits_R>(SkColorGetR(color) >> (8 - kLuminanceBits_R)),
                   sk_t_scale255<kLuminanceBits_G>(SkColorGetG(color) >> (8 - kLuminanceBits_G)),
                   sk_t_scale255<kLuminanceBits_B>(SkColorGetB(color) >> (8 - kLuminanceBits_B)));
    }

    
    typedef SkTMaskPreBlend<R_LUM_BITS, G_LUM_BITS, B_LUM_BITS> PreBlend;

    




    PreBlend preBlend(SkColor color);

private:
    enum LuminanceBits {
        kLuminanceBits_R = R_LUM_BITS,
        kLuminanceBits_G = G_LUM_BITS,
        kLuminanceBits_B = B_LUM_BITS,
        kLuminanceBits_Max = B_LUM_BITS > (R_LUM_BITS > G_LUM_BITS ? R_LUM_BITS : G_LUM_BITS)
                           ? B_LUM_BITS
                           : (R_LUM_BITS > G_LUM_BITS ? R_LUM_BITS : G_LUM_BITS)
    };
    uint8_t fGammaTables[1 << kLuminanceBits_Max][256];
    bool fIsLinear;

    typedef SkRefCnt INHERITED;
};


#define MacroComma ,
SK_DEFINE_INST_COUNT_TEMPLATE(
    template <int R_LUM_BITS MacroComma int G_LUM_BITS MacroComma int B_LUM_BITS>,
    SkTMaskGamma<R_LUM_BITS MacroComma G_LUM_BITS MacroComma B_LUM_BITS>);









template <int R_LUM_BITS, int G_LUM_BITS, int B_LUM_BITS> class SkTMaskPreBlend {
private:
    SkTMaskPreBlend(SkTMaskGamma<R_LUM_BITS, G_LUM_BITS, B_LUM_BITS>* parent,
                    const uint8_t* r,
                    const uint8_t* g,
                    const uint8_t* b)
    : fParent(parent), fR(r), fG(g), fB(b) {
        SkSafeRef(parent);
    }
    SkAutoTUnref<SkTMaskGamma<R_LUM_BITS, G_LUM_BITS, B_LUM_BITS> > fParent;
    friend class SkTMaskGamma<R_LUM_BITS, G_LUM_BITS, B_LUM_BITS>;
public:
    



    SkTMaskPreBlend(const SkTMaskPreBlend<R_LUM_BITS, G_LUM_BITS, B_LUM_BITS>& that)
    : fParent(that.fParent.get()), fR(that.fR), fG(that.fG), fB(that.fB) {
        SkSafeRef(fParent.get());
    }
    ~SkTMaskPreBlend() { }
    const uint8_t* fR;
    const uint8_t* fG;
    const uint8_t* fB;
};

template <int R_LUM_BITS, int G_LUM_BITS, int B_LUM_BITS>
SkTMaskPreBlend<R_LUM_BITS, G_LUM_BITS, B_LUM_BITS>
SkTMaskGamma<R_LUM_BITS, G_LUM_BITS, B_LUM_BITS>::preBlend(SkColor color) {
    return fIsLinear ? SkTMaskPreBlend<R_LUM_BITS, G_LUM_BITS, B_LUM_BITS>(
                          NULL, NULL, NULL, NULL)
                      : SkTMaskPreBlend<R_LUM_BITS, G_LUM_BITS, B_LUM_BITS>(
                          this,
                          fGammaTables[SkColorGetR(color) >> (8 - kLuminanceBits_Max)],
                          fGammaTables[SkColorGetG(color) >> (8 - kLuminanceBits_Max)],
                          fGammaTables[SkColorGetB(color) >> (8 - kLuminanceBits_Max)]);
}









template<bool APPLY_LUT> static inline U8CPU sk_apply_lut_if(U8CPU component, const uint8_t*) {
    return component;
}
template<>  inline U8CPU sk_apply_lut_if<true>(U8CPU component, const uint8_t* lut) {
    return lut[component];
}


#endif
