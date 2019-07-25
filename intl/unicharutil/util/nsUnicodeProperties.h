




































#ifndef NS_UNICODEPROPERTIES_H
#define NS_UNICODEPROPERTIES_H

#include "prtypes.h"
#include "nsBidiUtils.h"
#include "nsIUGenCategory.h"
#include "nsUnicodeScriptCodes.h"

nsCharProps1 GetCharProps1(PRUint32 aCh);
nsCharProps2 GetCharProps2(PRUint32 aCh);

namespace mozilla {

namespace unicode {

extern nsIUGenCategory::nsUGenCategory sDetailedToGeneralCategory[];

PRUint32 GetMirroredChar(PRUint32 aCh);

inline PRUint8 GetCombiningClass(PRUint32 aCh) {
  return GetCharProps1(aCh).mCombiningClass;
}


inline PRUint8 GetGeneralCategory(PRUint32 aCh) {
  return GetCharProps2(aCh).mCategory;
}


inline nsIUGenCategory::nsUGenCategory GetGenCategory(PRUint32 aCh) {
  return sDetailedToGeneralCategory[GetGeneralCategory(aCh)];
}

inline PRUint8 GetEastAsianWidth(PRUint32 aCh) {
  return GetCharProps2(aCh).mEAW;
}

inline PRUint8 GetScriptCode(PRUint32 aCh) {
  return GetCharProps2(aCh).mScriptCode;
}

PRUint32 GetScriptTagForCode(PRInt32 aScriptCode);

inline nsCharType GetBidiCat(PRUint32 aCh) {
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

inline XidmodType GetIdentifierModification(PRUint32 aCh) {
  return XidmodType(GetCharProps2(aCh).mXidmod);
}

inline bool IsRestrictedForIdentifiers(PRUint32 aCh) {
  XidmodType xm = GetIdentifierModification(aCh);
  return (xm > XIDMOD_RECOMMENDED);
}







inline PRInt8 GetNumericValue(PRUint32 aCh) {
  return GetCharProps2(aCh).mNumericValue;
}

enum HanVariantType {
  HVT_NotHan = 0x0,
  HVT_SimplifiedOnly = 0x1,
  HVT_TraditionalOnly = 0x2,
  HVT_AnyHan = 0x3
};

HanVariantType GetHanVariant(PRUint32 aCh);

bool IsClusterExtender(PRUint32 aCh, PRUint8 aCategory);

inline bool IsClusterExtender(PRUint32 aCh) {
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

inline HSType GetHangulSyllableType(PRUint32 aCh) {
  return HSType(GetCharProps1(aCh).mHangulType);
}




PRUint32 GetUppercase(PRUint32 aCh);
PRUint32 GetLowercase(PRUint32 aCh);
PRUint32 GetTitlecaseForLower(PRUint32 aCh); 
PRUint32 GetTitlecaseForAll(PRUint32 aCh); 

enum ShapingType {
  SHAPING_DEFAULT   = 0x0001,
  SHAPING_ARABIC    = 0x0002,
  SHAPING_HEBREW    = 0x0004,
  SHAPING_HANGUL    = 0x0008,
  SHAPING_MONGOLIAN = 0x0010,
  SHAPING_INDIC     = 0x0020,
  SHAPING_THAI      = 0x0040
};

PRInt32 ScriptShapingType(PRInt32 aScriptCode);



class ClusterIterator
{
public:
    ClusterIterator(const PRUnichar* aText, PRUint32 aLength)
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
