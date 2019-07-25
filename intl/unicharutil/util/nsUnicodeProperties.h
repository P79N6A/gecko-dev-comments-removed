




#ifndef NS_UNICODEPROPERTIES_H
#define NS_UNICODEPROPERTIES_H

#include "prtypes.h"
#include "nsBidiUtils.h"
#include "nsIUGenCategory.h"
#include "nsUnicodeScriptCodes.h"

const nsCharProps1& GetCharProps1(uint32_t aCh);
const nsCharProps2& GetCharProps2(uint32_t aCh);

namespace mozilla {

namespace unicode {

extern nsIUGenCategory::nsUGenCategory sDetailedToGeneralCategory[];

uint32_t GetMirroredChar(uint32_t aCh);

inline uint8_t GetCombiningClass(uint32_t aCh) {
  return GetCharProps1(aCh).mCombiningClass;
}


inline uint8_t GetGeneralCategory(uint32_t aCh) {
  return GetCharProps2(aCh).mCategory;
}


inline nsIUGenCategory::nsUGenCategory GetGenCategory(uint32_t aCh) {
  return sDetailedToGeneralCategory[GetGeneralCategory(aCh)];
}

inline uint8_t GetEastAsianWidth(uint32_t aCh) {
  return GetCharProps2(aCh).mEAW;
}

inline uint8_t GetScriptCode(uint32_t aCh) {
  return GetCharProps2(aCh).mScriptCode;
}

uint32_t GetScriptTagForCode(int32_t aScriptCode);

inline nsCharType GetBidiCat(uint32_t aCh) {
  return nsCharType(GetCharProps2(aCh).mBidiCategory);
}

enum XidmodType {
  XIDMOD_INCLUSION,
  XIDMOD_RECOMMENDED,
  XIDMOD_DEFAULT_IGNORABLE,
  XIDMOD_HISTORIC,
  XIDMOD_LIMITED_USE,
  XIDMOD_NOT_NFKC,
  XIDMOD_NOT_XID,
  XIDMOD_OBSOLETE,
  XIDMOD_TECHNICAL,
  XIDMOD_NOT_CHARS
};

inline XidmodType GetIdentifierModification(uint32_t aCh) {
  return XidmodType(GetCharProps2(aCh).mXidmod);
}

inline bool IsRestrictedForIdentifiers(uint32_t aCh) {
  XidmodType xm = GetIdentifierModification(aCh);
  return (xm > XIDMOD_RECOMMENDED);
}







inline int8_t GetNumericValue(uint32_t aCh) {
  return GetCharProps2(aCh).mNumericValue;
}

enum HanVariantType {
  HVT_NotHan = 0x0,
  HVT_SimplifiedOnly = 0x1,
  HVT_TraditionalOnly = 0x2,
  HVT_AnyHan = 0x3
};

HanVariantType GetHanVariant(uint32_t aCh);

bool IsClusterExtender(uint32_t aCh, uint8_t aCategory);

inline bool IsClusterExtender(uint32_t aCh) {
  return IsClusterExtender(aCh, GetGeneralCategory(aCh));
}

enum HSType {
  HST_NONE = 0x00,
  HST_L    = 0x01,
  HST_V    = 0x02,
  HST_T    = 0x04,
  HST_LV   = 0x03,
  HST_LVT  = 0x07
};

inline HSType GetHangulSyllableType(uint32_t aCh) {
  return HSType(GetCharProps1(aCh).mHangulType);
}




uint32_t GetUppercase(uint32_t aCh);
uint32_t GetLowercase(uint32_t aCh);
uint32_t GetTitlecaseForLower(uint32_t aCh); 
uint32_t GetTitlecaseForAll(uint32_t aCh); 

enum ShapingType {
  SHAPING_DEFAULT   = 0x0001,
  SHAPING_ARABIC    = 0x0002,
  SHAPING_HEBREW    = 0x0004,
  SHAPING_HANGUL    = 0x0008,
  SHAPING_MONGOLIAN = 0x0010,
  SHAPING_INDIC     = 0x0020,
  SHAPING_THAI      = 0x0040
};

int32_t ScriptShapingType(int32_t aScriptCode);



class ClusterIterator
{
public:
    ClusterIterator(const PRUnichar* aText, uint32_t aLength)
        : mPos(aText), mLimit(aText + aLength)
#ifdef DEBUG
        , mText(aText)
#endif
    { }

    operator const PRUnichar* () const {
        return mPos;
    }

    bool AtEnd() const {
        return mPos >= mLimit;
    }

    void Next();

private:
    const PRUnichar* mPos;
    const PRUnichar* mLimit;
#ifdef DEBUG
    const PRUnichar* mText;
#endif
};

} 

} 

#endif 
