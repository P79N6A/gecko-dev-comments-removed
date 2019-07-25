





#ifndef GFX_FONT_FEATURES_H
#define GFX_FONT_FEATURES_H

#include "prtypes.h"


struct gfxFontFeature {
    PRUint32 mTag; 
    PRUint32 mValue; 
                     
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
