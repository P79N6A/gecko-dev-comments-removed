






#ifndef SkImageTypes_DEFINED
#define SkImageTypes_DEFINED

#include "SkTypes.h"

enum SkColorType {
    kAlpha_8_SkColorType,
    kRGB_565_SkColorType,


    kPMColor_SkColorType,

    kLastEnum_SkColorType = kPMColor_SkColorType
};

enum SkAlphaType {

    kOpaque_SkAlphaType,

    kPremul_SkAlphaType,

    kLastEnum_SkAlphaType = kPremul_SkAlphaType
};

struct SkImageInfo {
    int         fWidth;
    int         fHeight;
    SkColorType fColorType;
    SkAlphaType fAlphaType;
};

#endif
