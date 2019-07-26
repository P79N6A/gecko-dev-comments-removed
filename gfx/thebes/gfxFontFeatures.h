





#ifndef GFX_FONT_FEATURES_H
#define GFX_FONT_FEATURES_H

#include "prtypes.h"


struct gfxFontFeature {
    uint32_t mTag; 
    uint32_t mValue; 
                     
};

inline bool
operator<(const gfxFontFeature& a, const gfxFontFeature& b)
{
    return (a.mTag < b.mTag) || ((a.mTag == b.mTag) && (a.mValue < b.mValue));
}

inline bool
operator==(const gfxFontFeature& a, const gfxFontFeature& b)
{
    return (a.mTag == b.mTag) && (a.mValue == b.mValue);
}

#endif
