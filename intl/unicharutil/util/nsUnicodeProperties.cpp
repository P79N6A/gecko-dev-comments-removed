




#define HB_DONT_DEFINE_STDINT  1

#include "nsUnicodeProperties.h"
#include "nsUnicodePropertyData.cpp"

#include "mozilla/Util.h"
#include "nsMemory.h"
#include "nsCharTraits.h"

#define UNICODE_BMP_LIMIT 0x10000
#define UNICODE_LIMIT     0x110000


const nsCharProps1&
GetCharProps1(PRUint32 aCh)
{
    if (aCh < UNICODE_BMP_LIMIT) {
        return sCharProp1Values[sCharProp1Pages[0][aCh >> kCharProp1CharBits]]
                               [aCh & ((1 << kCharProp1CharBits) - 1)];
    }
    if (aCh < (kCharProp1MaxPlane + 1) * 0x10000) {
        return sCharProp1Values[sCharProp1Pages[sCharProp1Planes[(aCh >> 16) - 1]]
                                               [(aCh & 0xffff) >> kCharProp1CharBits]]
                               [aCh & ((1 << kCharProp1CharBits) - 1)];
    }

    
    static const nsCharProps1 undefined = {
        0,       
        0,       
        0        
    };
    return undefined;
}

const nsCharProps2&
GetCharProps2(PRUint32 aCh)
{
    if (aCh < UNICODE_BMP_LIMIT) {
        return sCharProp2Values[sCharProp2Pages[0][aCh >> kCharProp2CharBits]]
                              [aCh & ((1 << kCharProp2CharBits) - 1)];
    }
    if (aCh < (kCharProp2MaxPlane + 1) * 0x10000) {
        return sCharProp2Values[sCharProp2Pages[sCharProp2Planes[(aCh >> 16) - 1]]
                                               [(aCh & 0xffff) >> kCharProp2CharBits]]
                               [aCh & ((1 << kCharProp2CharBits) - 1)];
    }

    NS_NOTREACHED("Getting CharProps for codepoint outside Unicode range");
    
    static const nsCharProps2 undefined = {
        MOZ_SCRIPT_UNKNOWN,                      
        0,                                       
        HB_UNICODE_GENERAL_CATEGORY_UNASSIGNED,  
        eCharType_LeftToRight,                   
        mozilla::unicode::XIDMOD_NOT_CHARS,      
        -1,                                      
        mozilla::unicode::HVT_NotHan             
    };
    return undefined;
}

namespace mozilla {

namespace unicode {






















nsIUGenCategory::nsUGenCategory sDetailedToGeneralCategory[] = {
  



               nsIUGenCategory::kOther,
                nsIUGenCategory::kOther,
            nsIUGenCategory::kOther,
           nsIUGenCategory::kOther,
             nsIUGenCategory::kOther,
      nsIUGenCategory::kLetter,
       nsIUGenCategory::kLetter,
          nsIUGenCategory::kLetter,
      nsIUGenCategory::kLetter,
      nsIUGenCategory::kLetter,
        nsIUGenCategory::kMark,
        nsIUGenCategory::kMark,
      nsIUGenCategory::kMark,
        nsIUGenCategory::kNumber,
         nsIUGenCategory::kNumber,
          nsIUGenCategory::kNumber,
   nsIUGenCategory::kPunctuation,
      nsIUGenCategory::kPunctuation,
     nsIUGenCategory::kPunctuation,
     nsIUGenCategory::kPunctuation,
   nsIUGenCategory::kPunctuation,
     nsIUGenCategory::kPunctuation,
      nsIUGenCategory::kPunctuation,
       nsIUGenCategory::kSymbol,
       nsIUGenCategory::kSymbol,
           nsIUGenCategory::kSymbol,
          nsIUGenCategory::kSymbol,
        nsIUGenCategory::kSeparator,
   nsIUGenCategory::kSeparator,
       nsIUGenCategory::kSeparator
};

PRUint32
GetMirroredChar(PRUint32 aCh)
{
    return aCh + sMirrorOffsets[GetCharProps1(aCh).mMirrorOffsetIndex];
}

PRUint32
GetScriptTagForCode(PRInt32 aScriptCode)
{
    
    if (PRUint32(aScriptCode) > ArrayLength(sScriptCodeToTag)) {
        return 0;
    }
    return sScriptCodeToTag[aScriptCode];
}

static inline PRUint32
GetCaseMapValue(PRUint32 aCh)
{
    if (aCh < UNICODE_BMP_LIMIT) {
        return sCaseMapValues[sCaseMapPages[0][aCh >> kCaseMapCharBits]]
                             [aCh & ((1 << kCaseMapCharBits) - 1)];
    }
    if (aCh < (kCaseMapMaxPlane + 1) * 0x10000) {
        return sCaseMapValues[sCaseMapPages[sCaseMapPlanes[(aCh >> 16) - 1]]
                                           [(aCh & 0xffff) >> kCaseMapCharBits]]
                             [aCh & ((1 << kCaseMapCharBits) - 1)];
    }
    return 0;
}

PRUint32
GetUppercase(PRUint32 aCh)
{
    PRUint32 mapValue = GetCaseMapValue(aCh);
    if (mapValue & (kLowerToUpper | kTitleToUpper)) {
        return aCh ^ (mapValue & kCaseMapCharMask);
    }
    if (mapValue & kLowerToTitle) {
        return GetUppercase(aCh ^ (mapValue & kCaseMapCharMask));
    }
    return aCh;
}

PRUint32
GetLowercase(PRUint32 aCh)
{
    PRUint32 mapValue = GetCaseMapValue(aCh);
    if (mapValue & kUpperToLower) {
        return aCh ^ (mapValue & kCaseMapCharMask);
    }
    if (mapValue & kTitleToUpper) {
        return GetLowercase(aCh ^ (mapValue & kCaseMapCharMask));
    }
    return aCh;
}

PRUint32
GetTitlecaseForLower(PRUint32 aCh)
{
    PRUint32 mapValue = GetCaseMapValue(aCh);
    if (mapValue & (kLowerToTitle | kLowerToUpper)) {
        return aCh ^ (mapValue & kCaseMapCharMask);
    }
    return aCh;
}

PRUint32
GetTitlecaseForAll(PRUint32 aCh)
{
    PRUint32 mapValue = GetCaseMapValue(aCh);
    if (mapValue & (kLowerToTitle | kLowerToUpper)) {
        return aCh ^ (mapValue & kCaseMapCharMask);
    }
    if (mapValue & kUpperToLower) {
        return GetTitlecaseForLower(aCh ^ (mapValue & kCaseMapCharMask));
    }
    return aCh;
}

HanVariantType
GetHanVariant(PRUint32 aCh)
{
    
    
    
    PRUint8 v = 0;
    if (aCh < UNICODE_BMP_LIMIT) {
        v = sHanVariantValues[sHanVariantPages[0][aCh >> kHanVariantCharBits]]
                             [(aCh & ((1 << kHanVariantCharBits) - 1)) >> 2];
    } else if (aCh < (kHanVariantMaxPlane + 1) * 0x10000) {
        v = sHanVariantValues[sHanVariantPages[sHanVariantPlanes[(aCh >> 16) - 1]]
                                              [(aCh & 0xffff) >> kHanVariantCharBits]]
                             [(aCh & ((1 << kHanVariantCharBits) - 1)) >> 2];
    }
    
    return HanVariantType((v >> ((aCh & 3) * 2)) & 3);
}

bool
IsClusterExtender(PRUint32 aCh, PRUint8 aCategory)
{
    return ((aCategory >= HB_UNICODE_GENERAL_CATEGORY_SPACING_MARK &&
             aCategory <= HB_UNICODE_GENERAL_CATEGORY_NON_SPACING_MARK) ||
            (aCh >= 0x200c && aCh <= 0x200d) || 
            (aCh >= 0xff9e && aCh <= 0xff9f));  
}









PRInt32
ScriptShapingType(PRInt32 aScriptCode)
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

void
ClusterIterator::Next()
{
    if (AtEnd()) {
        NS_WARNING("ClusterIterator has already reached the end");
        return;
    }

    PRUint32 ch = *mPos++;

    if (NS_IS_HIGH_SURROGATE(ch) && mPos < mLimit &&
        NS_IS_LOW_SURROGATE(*mPos)) {
        ch = SURROGATE_TO_UCS4(ch, *mPos++);
    } else if ((ch & ~0xff) == 0x1100 ||
        (ch >= 0xa960 && ch <= 0xa97f) ||
        (ch >= 0xac00 && ch <= 0xd7ff)) {
        
        HSType hangulState = GetHangulSyllableType(ch);
        while (mPos < mLimit) {
            ch = *mPos;
            HSType hangulType = GetHangulSyllableType(ch);
            switch (hangulType) {
            case HST_L:
            case HST_LV:
            case HST_LVT:
                if (hangulState == HST_L) {
                    hangulState = hangulType;
                    mPos++;
                    continue;
                }
                break;
            case HST_V:
                if ((hangulState != HST_NONE) && !(hangulState & HST_T)) {
                    hangulState = hangulType;
                    mPos++;
                    continue;
                }
                break;
            case HST_T:
                if (hangulState & (HST_V | HST_T)) {
                    hangulState = hangulType;
                    mPos++;
                    continue;
                }
                break;
            default:
                break;
            }
            break;
        }
    }

    while (mPos < mLimit) {
        ch = *mPos;

        
        
        
        if (NS_IS_HIGH_SURROGATE(ch) && mPos < mLimit - 1 &&
            NS_IS_LOW_SURROGATE(*(mPos + 1))) {
            ch = SURROGATE_TO_UCS4(ch, *(mPos + 1));
        }

        if (!IsClusterExtender(ch)) {
            break;
        }

        mPos++;
        if (!IS_IN_BMP(ch)) {
            mPos++;
        }
    }

    NS_ASSERTION(mText < mPos && mPos <= mLimit,
                 "ClusterIterator::Next has overshot the string!");
}

} 

} 
