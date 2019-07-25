




































#include "gfxUnicodeProperties.h"

#include "gfxUnicodePropertyData.cpp"

#include "harfbuzz/hb-unicode.h"

#define UNICODE_BMP_LIMIT 0x10000
#define UNICODE_LIMIT     0x110000






















PRUint32
gfxUnicodeProperties::GetMirroredChar(PRUint32 aCh)
{
    
    if (aCh < UNICODE_BMP_LIMIT) {
        int v = sMirrorValues[sMirrorPages[0][aCh >> kMirrorCharBits]]
                             [aCh & ((1 << kMirrorCharBits) - 1)];
        
        
        
        
        
        if (v < kSmallMirrorOffset) {
            return aCh + v;
        }
        return sDistantMirrors[v - kSmallMirrorOffset];
    }
    return aCh;
}

PRUint8
gfxUnicodeProperties::GetCombiningClass(PRUint32 aCh)
{
    if (aCh < UNICODE_BMP_LIMIT) {
        return sCClassValues[sCClassPages[0][aCh >> kCClassCharBits]]
                            [aCh & ((1 << kCClassCharBits) - 1)];
    }
    if (aCh >= UNICODE_LIMIT) {
        return 0;
    }
    return sCClassValues[sCClassPages[sCClassPlanes[(aCh >> 16) - 1]]
                                     [(aCh & 0xffff) >> kCClassCharBits]]
                        [aCh & ((1 << kCClassCharBits) - 1)];
}

PRUint8
gfxUnicodeProperties::GetGeneralCategory(PRUint32 aCh)
{
    if (aCh < UNICODE_BMP_LIMIT) {
        return sCatEAWValues[sCatEAWPages[0][aCh >> kCatEAWCharBits]]
                            [aCh & ((1 << kCatEAWCharBits) - 1)].mCategory;
    }
    if (aCh >= UNICODE_LIMIT) {
        return PRUint8(HB_CATEGORY_UNASSIGNED);
    }
    return sCatEAWValues[sCatEAWPages[sCatEAWPlanes[(aCh >> 16) - 1]]
                                     [(aCh & 0xffff) >> kCatEAWCharBits]]
                        [aCh & ((1 << kCatEAWCharBits) - 1)].mCategory;
}

PRUint8
gfxUnicodeProperties::GetEastAsianWidth(PRUint32 aCh)
{
    if (aCh < UNICODE_BMP_LIMIT) {
        return sCatEAWValues[sCatEAWPages[0][aCh >> kCatEAWCharBits]]
                            [aCh & ((1 << kCatEAWCharBits) - 1)].mEAW;
    }
    if (aCh >= UNICODE_LIMIT) {
        return 0;
    }
    return sCatEAWValues[sCatEAWPages[sCatEAWPlanes[(aCh >> 16) - 1]]
                                     [(aCh & 0xffff) >> kCatEAWCharBits]]
                        [aCh & ((1 << kCatEAWCharBits) - 1)].mEAW;
}

PRInt32
gfxUnicodeProperties::GetScriptCode(PRUint32 aCh)
{
    if (aCh < UNICODE_BMP_LIMIT) {
        return sScriptValues[sScriptPages[0][aCh >> kScriptCharBits]]
                            [aCh & ((1 << kScriptCharBits) - 1)];
    }
    if (aCh >= UNICODE_LIMIT) {
        return PRInt32(HB_SCRIPT_UNKNOWN);
    }
    return sScriptValues[sScriptPages[sScriptPlanes[(aCh >> 16) - 1]]
                                     [(aCh & 0xffff) >> kScriptCharBits]]
                        [aCh & ((1 << kScriptCharBits) - 1)];
}

gfxUnicodeProperties::HSType
gfxUnicodeProperties::GetHangulSyllableType(PRUint32 aCh)
{
    
    if (aCh < UNICODE_BMP_LIMIT) {
        return HSType(sHangulValues[sHangulPages[0][aCh >> kHangulCharBits]]
                                   [aCh & ((1 << kHangulCharBits) - 1)]);
    }
    return HST_NONE;
}









PRInt32
gfxUnicodeProperties::ScriptShapingType(PRInt32 aScriptCode)
{
    switch (aScriptCode) {
    default:
        return SHAPING_DEFAULT; 
                                

    case HB_SCRIPT_ARABIC:
    case HB_SCRIPT_SYRIAC:
    case HB_SCRIPT_NKO:
    case HB_SCRIPT_MANDAIC:
        return SHAPING_ARABIC; 

    case HB_SCRIPT_HEBREW:
        return SHAPING_HEBREW;

    case HB_SCRIPT_HANGUL:
        return SHAPING_HANGUL;

    case HB_SCRIPT_MONGOLIAN: 
        return SHAPING_MONGOLIAN;

    case HB_SCRIPT_BENGALI:
    case HB_SCRIPT_DEVANAGARI:
    case HB_SCRIPT_GUJARATI:
    case HB_SCRIPT_GURMUKHI:
    case HB_SCRIPT_KANNADA:
    case HB_SCRIPT_MALAYALAM:
    case HB_SCRIPT_ORIYA:
    case HB_SCRIPT_SINHALA:
    case HB_SCRIPT_TAMIL:
    case HB_SCRIPT_TELUGU:
    case HB_SCRIPT_KHMER:
    case HB_SCRIPT_THAI:
    case HB_SCRIPT_LAO:
    case HB_SCRIPT_TIBETAN:
    case HB_SCRIPT_NEW_TAI_LUE:
    case HB_SCRIPT_TAI_LE:
    case HB_SCRIPT_MYANMAR:
    case HB_SCRIPT_PHAGS_PA:
    case HB_SCRIPT_BATAK:
    case HB_SCRIPT_BRAHMI:
        return SHAPING_INDIC; 
    }
}
