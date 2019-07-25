




































#ifndef NS_UNICODEPROPERTIES_H
#define NS_UNICODEPROPERTIES_H

#include "prtypes.h"
#include "nsIUGenCategory.h"

namespace mozilla {

namespace unicode {

extern nsIUGenCategory::nsUGenCategory sDetailedToGeneralCategory[];

PRUint32 GetMirroredChar(PRUint32 aCh);

PRUint8 GetCombiningClass(PRUint32 aCh);


PRUint8 GetGeneralCategory(PRUint32 aCh);


inline nsIUGenCategory::nsUGenCategory GetGenCategory(PRUint32 aCh) {
  return sDetailedToGeneralCategory[GetGeneralCategory(aCh)];
}

PRUint8 GetEastAsianWidth(PRUint32 aCh);

PRInt32 GetScriptCode(PRUint32 aCh);

PRUint32 GetScriptTagForCode(PRInt32 aScriptCode);

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

HSType GetHangulSyllableType(PRUint32 aCh);




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
