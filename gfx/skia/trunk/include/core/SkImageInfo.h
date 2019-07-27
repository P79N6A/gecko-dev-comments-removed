






#ifndef SkImageInfo_DEFINED
#define SkImageInfo_DEFINED

#include "SkMath.h"
#include "SkSize.h"

class SkWriteBuffer;
class SkReadBuffer;




enum SkAlphaType {
    




    kIgnore_SkAlphaType,

    





    kOpaque_SkAlphaType,

    



    kPremul_SkAlphaType,

    






    kUnpremul_SkAlphaType,

    kLastEnum_SkAlphaType = kUnpremul_SkAlphaType
};

static inline bool SkAlphaTypeIsOpaque(SkAlphaType at) {
    SK_COMPILE_ASSERT(kIgnore_SkAlphaType < kOpaque_SkAlphaType, bad_alphatype_order);
    SK_COMPILE_ASSERT(kPremul_SkAlphaType > kOpaque_SkAlphaType, bad_alphatype_order);
    SK_COMPILE_ASSERT(kUnpremul_SkAlphaType > kOpaque_SkAlphaType, bad_alphatype_order);

    return (unsigned)at <= kOpaque_SkAlphaType;
}

static inline bool SkAlphaTypeIsValid(unsigned value) {
    return value <= kLastEnum_SkAlphaType;
}










enum SkColorType {
    kUnknown_SkColorType,
    kAlpha_8_SkColorType,
    kRGB_565_SkColorType,
    kARGB_4444_SkColorType,
    kRGBA_8888_SkColorType,
    kBGRA_8888_SkColorType,
    kIndex_8_SkColorType,

    kLastEnum_SkColorType = kIndex_8_SkColorType,

#if SK_PMCOLOR_BYTE_ORDER(B,G,R,A)
    kN32_SkColorType = kBGRA_8888_SkColorType,
#elif SK_PMCOLOR_BYTE_ORDER(R,G,B,A)
    kN32_SkColorType = kRGBA_8888_SkColorType,
#else
#error "SK_*32_SHFIT values must correspond to BGRA or RGBA byte order"
#endif

#ifdef SK_SUPPORT_LEGACY_N32_NAME
    kPMColor_SkColorType = kN32_SkColorType
#endif
};

static int SkColorTypeBytesPerPixel(SkColorType ct) {
    static const uint8_t gSize[] = {
        0,  
        1,  
        2,  
        2,  
        4,  
        4,  
        1,  
    };
    SK_COMPILE_ASSERT(SK_ARRAY_COUNT(gSize) == (size_t)(kLastEnum_SkColorType + 1),
                      size_mismatch_with_SkColorType_enum);

    SkASSERT((size_t)ct < SK_ARRAY_COUNT(gSize));
    return gSize[ct];
}

static inline size_t SkColorTypeMinRowBytes(SkColorType ct, int width) {
    return width * SkColorTypeBytesPerPixel(ct);
}

static inline bool SkColorTypeIsValid(unsigned value) {
    return value <= kLastEnum_SkColorType;
}







bool SkColorTypeValidateAlphaType(SkColorType colorType, SkAlphaType alphaType,
                                  SkAlphaType* canonical = NULL);






struct SkImageInfo {
    int         fWidth;
    int         fHeight;
    SkColorType fColorType;
    SkAlphaType fAlphaType;

    static SkImageInfo Make(int width, int height, SkColorType ct, SkAlphaType at) {
        SkImageInfo info = {
            width, height, ct, at
        };
        return info;
    }

    


    static SkImageInfo MakeN32(int width, int height, SkAlphaType at) {
        SkImageInfo info = {
            width, height, kN32_SkColorType, at
        };
        return info;
    }

    


    static SkImageInfo MakeN32Premul(int width, int height) {
        SkImageInfo info = {
            width, height, kN32_SkColorType, kPremul_SkAlphaType
        };
        return info;
    }

    


    static SkImageInfo MakeN32Premul(const SkISize& size) {
        return MakeN32Premul(size.width(), size.height());
    }

    static SkImageInfo MakeA8(int width, int height) {
        SkImageInfo info = {
            width, height, kAlpha_8_SkColorType, kPremul_SkAlphaType
        };
        return info;
    }

    static SkImageInfo MakeUnknown(int width, int height) {
        SkImageInfo info = {
            width, height, kUnknown_SkColorType, kIgnore_SkAlphaType
        };
        return info;
    }

    static SkImageInfo MakeUnknown() {
        SkImageInfo info = {
            0, 0, kUnknown_SkColorType, kIgnore_SkAlphaType
        };
        return info;
    }

    int width() const { return fWidth; }
    int height() const { return fHeight; }
    SkColorType colorType() const { return fColorType; }
    SkAlphaType alphaType() const { return fAlphaType; }

    bool isEmpty() const { return fWidth <= 0 || fHeight <= 0; }

    bool isOpaque() const {
        return SkAlphaTypeIsOpaque(fAlphaType);
    }

    SkISize dimensions() const { return SkISize::Make(fWidth, fHeight); }

    



    SkImageInfo makeWH(int newWidth, int newHeight) const {
        return SkImageInfo::Make(newWidth, newHeight, fColorType, fAlphaType);
    }

    int bytesPerPixel() const {
        return SkColorTypeBytesPerPixel(fColorType);
    }

    uint64_t minRowBytes64() const {
        return sk_64_mul(fWidth, this->bytesPerPixel());
    }

    size_t minRowBytes() const {
        return (size_t)this->minRowBytes64();
    }

    bool operator==(const SkImageInfo& other) const {
        return 0 == memcmp(this, &other, sizeof(other));
    }
    bool operator!=(const SkImageInfo& other) const {
        return 0 != memcmp(this, &other, sizeof(other));
    }

    void unflatten(SkReadBuffer&);
    void flatten(SkWriteBuffer&) const;

    int64_t getSafeSize64(size_t rowBytes) const {
        if (0 == fHeight) {
            return 0;
        }
        return sk_64_mul(fHeight - 1, rowBytes) + fWidth * this->bytesPerPixel();
    }

    size_t getSafeSize(size_t rowBytes) const {
        return (size_t)this->getSafeSize64(rowBytes);
    }

    bool validRowBytes(size_t rowBytes) const {
        uint64_t rb = sk_64_mul(fWidth, this->bytesPerPixel());
        return rowBytes >= rb;
    }

    SkDEBUGCODE(void validate() const;)
};

#endif
