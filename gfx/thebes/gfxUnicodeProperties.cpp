




































#include "gfxUnicodeProperties.h"
#include "gfxUnicodePropertyData.cpp"

#include "mozilla/Util.h"
#include "nsMemory.h"

#include "harfbuzz/hb-unicode.h"

using namespace mozilla;

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
    if (aCh < UNICODE_LIMIT) {
        return sCClassValues[sCClassPages[sCClassPlanes[(aCh >> 16) - 1]]
                                         [(aCh & 0xffff) >> kCClassCharBits]]
                            [aCh & ((1 << kCClassCharBits) - 1)];
    }
    NS_NOTREACHED("invalid Unicode character!");
    return 0;
}

PRUint8
gfxUnicodeProperties::GetGeneralCategory(PRUint32 aCh)
{
    if (aCh < UNICODE_BMP_LIMIT) {
        return sCatEAWValues[sCatEAWPages[0][aCh >> kCatEAWCharBits]]
                            [aCh & ((1 << kCatEAWCharBits) - 1)].mCategory;
    }
    if (aCh < UNICODE_LIMIT) {
        return sCatEAWValues[sCatEAWPages[sCatEAWPlanes[(aCh >> 16) - 1]]
                                         [(aCh & 0xffff) >> kCatEAWCharBits]]
                            [aCh & ((1 << kCatEAWCharBits) - 1)].mCategory;
    }
    NS_NOTREACHED("invalid Unicode character!");
    return PRUint8(HB_UNICODE_GENERAL_CATEGORY_UNASSIGNED);
}

PRUint8
gfxUnicodeProperties::GetEastAsianWidth(PRUint32 aCh)
{
    if (aCh < UNICODE_BMP_LIMIT) {
        return sCatEAWValues[sCatEAWPages[0][aCh >> kCatEAWCharBits]]
                            [aCh & ((1 << kCatEAWCharBits) - 1)].mEAW;
    }
    if (aCh < UNICODE_LIMIT) {
        return sCatEAWValues[sCatEAWPages[sCatEAWPlanes[(aCh >> 16) - 1]]
                                         [(aCh & 0xffff) >> kCatEAWCharBits]]
                            [aCh & ((1 << kCatEAWCharBits) - 1)].mEAW;
    }
    NS_NOTREACHED("invalid Unicode character!");
    return 0;
}

PRInt32
gfxUnicodeProperties::GetScriptCode(PRUint32 aCh)
{
    if (aCh < UNICODE_BMP_LIMIT) {
        return sScriptValues[sScriptPages[0][aCh >> kScriptCharBits]]
                            [aCh & ((1 << kScriptCharBits) - 1)];
    }
    if (aCh < UNICODE_LIMIT) {
        return sScriptValues[sScriptPages[sScriptPlanes[(aCh >> 16) - 1]]
                                         [(aCh & 0xffff) >> kScriptCharBits]]
                            [aCh & ((1 << kScriptCharBits) - 1)];
    }
    NS_NOTREACHED("invalid Unicode character!");
    return MOZ_SCRIPT_UNKNOWN;
}

PRUint32
gfxUnicodeProperties::GetScriptTagForCode(PRInt32 aScriptCode)
{
    
    if (PRUint32(aScriptCode) > ArrayLength(sScriptCodeToTag)) {
        return 0;
    }
    return sScriptCodeToTag[aScriptCode];
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
                                

    case MOZ_SCRIPT_ARABIC:
    case MOZ_SCRIPT_SYRIAC:
    case MOZ_SCRIPT_NKO:
    case MOZ_SCRIPT_MANDAIC:
        return SHAPING_ARABIC; 

    case MOZ_SCRIPT_HEBREW:
        return SHAPING_HEBREW;

    case MOZ_SCRIPT_HANGUL:
        return SHAPING_HANGUL;

    case MOZ_SCRIPT_MONGOLIAN: 
        return SHAPING_MONGOLIAN;

    case MOZ_SCRIPT_THAI: 
                          
        return SHAPING_THAI;

    case MOZ_SCRIPT_BENGALI:
    case MOZ_SCRIPT_DEVANAGARI:
    case MOZ_SCRIPT_GUJARATI:
    case MOZ_SCRIPT_GURMUKHI:
    case MOZ_SCRIPT_KANNADA:
    case MOZ_SCRIPT_MALAYALAM:
    case MOZ_SCRIPT_ORIYA:
    case MOZ_SCRIPT_SINHALA:
    case MOZ_SCRIPT_TAMIL:
    case MOZ_SCRIPT_TELUGU:
    case MOZ_SCRIPT_KHMER:
    case MOZ_SCRIPT_LAO:
    case MOZ_SCRIPT_TIBETAN:
    case MOZ_SCRIPT_NEW_TAI_LUE:
    case MOZ_SCRIPT_TAI_LE:
    case MOZ_SCRIPT_MYANMAR:
    case MOZ_SCRIPT_PHAGS_PA:
    case MOZ_SCRIPT_BATAK:
    case MOZ_SCRIPT_BRAHMI:
        return SHAPING_INDIC; 
    }
}
