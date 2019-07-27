






#ifndef SkMaskGamma_DEFINED
#define SkMaskGamma_DEFINED

#include "SkTypes.h"
#include "SkColor.h"
#include "SkColorPriv.h"
#include "SkRefCnt.h"








class SkColorSpaceLuminance : SkNoncopyable {
public:
    virtual ~SkColorSpaceLuminance() { }

    
    virtual SkScalar toLuma(SkScalar gamma, SkScalar luminance) const = 0;
    
    virtual SkScalar fromLuma(SkScalar gamma, SkScalar luma) const = 0;

    
    static U8CPU computeLuminance(SkScalar gamma, SkColor c) {
        const SkColorSpaceLuminance& luminance = Fetch(gamma);
        SkScalar r = luminance.toLuma(gamma, SkIntToScalar(SkColorGetR(c)) / 255);
        SkScalar g = luminance.toLuma(gamma, SkIntToScalar(SkColorGetG(c)) / 255);
        SkScalar b = luminance.toLuma(gamma, SkIntToScalar(SkColorGetB(c)) / 255);
        SkScalar luma = r * SK_LUM_COEFF_R +
                        g * SK_LUM_COEFF_G +
                        b * SK_LUM_COEFF_B;
        SkASSERT(luma <= SK_Scalar1);
        return SkScalarRoundToInt(luminance.fromLuma(gamma, luma) * 255);
    }

    
    static const SkColorSpaceLuminance& Fetch(SkScalar gamma);
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
                                       const SkColorSpaceLuminance& srcConvert, SkScalar srcGamma,
                                       const SkColorSpaceLuminance& dstConvert, SkScalar dstGamma);












template <int R_LUM_BITS, int G_LUM_BITS, int B_LUM_BITS> class SkTMaskGamma : public SkRefCnt {
    SK_DECLARE_INST_COUNT(SkTMaskGamma)
public:

    
    SkTMaskGamma() : fIsLinear(true) { }

    








    SkTMaskGamma(SkScalar contrast, SkScalar paintGamma, SkScalar deviceGamma) : fIsLinear(false) {
        const SkColorSpaceLuminance& paintConvert = SkColorSpaceLuminance::Fetch(paintGamma);
        const SkColorSpaceLuminance& deviceConvert = SkColorSpaceLuminance::Fetch(deviceGamma);
        for (U8CPU i = 0; i < (1 << MAX_LUM_BITS); ++i) {
            U8CPU lum = sk_t_scale255<MAX_LUM_BITS>(i);
            SkTMaskGamma_build_correcting_lut(fGammaTables[i], lum, contrast,
                                              paintConvert, paintGamma,
                                              deviceConvert, deviceGamma);
        }
    }

    
    static SkColor CanonicalColor(SkColor color) {
        return SkColorSetRGB(
                   sk_t_scale255<R_LUM_BITS>(SkColorGetR(color) >> (8 - R_LUM_BITS)),
                   sk_t_scale255<G_LUM_BITS>(SkColorGetG(color) >> (8 - G_LUM_BITS)),
                   sk_t_scale255<B_LUM_BITS>(SkColorGetB(color) >> (8 - B_LUM_BITS)));
    }

    
    typedef SkTMaskPreBlend<R_LUM_BITS, G_LUM_BITS, B_LUM_BITS> PreBlend;

    




    PreBlend preBlend(SkColor color) const;

    


    void getGammaTableDimensions(int* tableWidth, int* numTables) const {
        *tableWidth = 256;
        *numTables = (1 << MAX_LUM_BITS);
    }

    



    const uint8_t* getGammaTables() const {
        return (const uint8_t*) fGammaTables;
    }
    
private:
    static const int MAX_LUM_BITS =
          B_LUM_BITS > (R_LUM_BITS > G_LUM_BITS ? R_LUM_BITS : G_LUM_BITS)
        ? B_LUM_BITS : (R_LUM_BITS > G_LUM_BITS ? R_LUM_BITS : G_LUM_BITS);
    uint8_t fGammaTables[1 << MAX_LUM_BITS][256];
    bool fIsLinear;

    typedef SkRefCnt INHERITED;
};











template <int R_LUM_BITS, int G_LUM_BITS, int B_LUM_BITS> class SkTMaskPreBlend {
private:
    SkTMaskPreBlend(const SkTMaskGamma<R_LUM_BITS, G_LUM_BITS, B_LUM_BITS>* parent,
                    const uint8_t* r, const uint8_t* g, const uint8_t* b)
    : fParent(SkSafeRef(parent)), fR(r), fG(g), fB(b) { }

    SkAutoTUnref<const SkTMaskGamma<R_LUM_BITS, G_LUM_BITS, B_LUM_BITS> > fParent;
    friend class SkTMaskGamma<R_LUM_BITS, G_LUM_BITS, B_LUM_BITS>;
public:
    
    SkTMaskPreBlend() : fParent(), fR(NULL), fG(NULL), fB(NULL) { }

    



    SkTMaskPreBlend(const SkTMaskPreBlend<R_LUM_BITS, G_LUM_BITS, B_LUM_BITS>& that)
    : fParent(SkSafeRef(that.fParent.get())), fR(that.fR), fG(that.fG), fB(that.fB) { }

    ~SkTMaskPreBlend() { }

    
    bool isApplicable() const {
        return NULL != this->fG;
    }

    const uint8_t* fR;
    const uint8_t* fG;
    const uint8_t* fB;
};

template <int R_LUM_BITS, int G_LUM_BITS, int B_LUM_BITS>
SkTMaskPreBlend<R_LUM_BITS, G_LUM_BITS, B_LUM_BITS>
SkTMaskGamma<R_LUM_BITS, G_LUM_BITS, B_LUM_BITS>::preBlend(SkColor color) const {
    return fIsLinear ? SkTMaskPreBlend<R_LUM_BITS, G_LUM_BITS, B_LUM_BITS>()
                     : SkTMaskPreBlend<R_LUM_BITS, G_LUM_BITS, B_LUM_BITS>(this,
                         fGammaTables[SkColorGetR(color) >> (8 - MAX_LUM_BITS)],
                         fGammaTables[SkColorGetG(color) >> (8 - MAX_LUM_BITS)],
                         fGammaTables[SkColorGetB(color) >> (8 - MAX_LUM_BITS)]);
}









template<bool APPLY_LUT> static inline U8CPU sk_apply_lut_if(U8CPU component, const uint8_t*) {
    return component;
}
template<>  inline U8CPU sk_apply_lut_if<true>(U8CPU component, const uint8_t* lut) {
    return lut[component];
}


#endif
